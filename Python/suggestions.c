#include "Python.h"

#include "pycore_suggestions.h"

#define MAX_GETATTR_PREDICT_DIST 3
#define MAX_GETATTR_PREDICT_ITEMS 100
#define MAX_GETATTR_STRING_SIZE 20

static size_t
distance(const char *string1, const char *string2)
{
    Py_ssize_t len1 = strlen(string1);
    Py_ssize_t len2 = strlen(string2);
    Py_ssize_t i;
    Py_ssize_t half;
    size_t *row;
    size_t *end;

    /* Get rid of the common prefix */
    while (len1 > 0 && len2 > 0 && *string1 == *string2) {
        len1--;
        len2--;
        string1++;
        string2++;
    }

    /* strip common suffix */
    while (len1 > 0 && len2 > 0 && string1[len1-1] == string2[len2-1]) {
        len1--;
        len2--;
    }

    /* catch trivial cases */
    if (len1 == 0) {
        return len2;
    }
    if (len2 == 0) {
        return len1;
    }

    /* make the inner cycle (i.e. string2) the longer one */
    if (len1 > len2) {
        size_t nx = len1;
        const char *sx = string1;
        len1 = len2;
        len2 = nx;
        string1 = string2;
        string2 = sx;
    }
    /* check len1 == 1 separately */
    if (len1 == 1) {
        return len2 - (memchr(string2, *string1, len2) != NULL);
    }
    len1++;
    len2++;
    half = len1 >> 1;

    /* initalize first row */
    row = (size_t*)PyMem_Malloc(len2*sizeof(size_t));
    if (!row) {
        return (size_t)(-1);
    }
    end = row + len2 - 1;
    for (i = 0; i < len2 - half; i++) {
        row[i] = i;
    }

    /* We don't have to scan two corner triangles (of size len1/2)
     * in the matrix because no best path can go throught them. This is
     * not true when len1 == len2 == 2 so the memchr() special case above is
     * necessary */
    row[0] = len1 - half - 1;
    for (i = 1; i < len1; i++) {
        size_t *p;
        const char char1 = string1[i - 1];
        const char *char2p;
        size_t D, x;
        /* skip the upper triangle */
        if (i >= len1 - half) {
            size_t offset = i - (len1 - half);
            size_t c3;

            char2p = string2 + offset;
            p = row + offset;
            c3 = *(p++) + (char1 != *(char2p++));
            x = *p;
            x++;
            D = x;
            if (x > c3)
              x = c3;
            *(p++) = x;
        }
        else {
            p = row + 1;
            char2p = string2;
            D = x = i;
        }
        /* skip the lower triangle */
        if (i <= half + 1) {
            end = row + len2 + i - half - 2;
        }
        /* main */
        while (p <= end) {
            size_t c3 = --D + (char1 != *(char2p++));
            x++;
            if (x > c3)
              x = c3;
            D = *p;
            D++;
            if (x > D)
              x = D;
            *(p++) = x;
        }
        /* lower triangle sentinel */
        if (i <= half) {
            size_t c3 = --D + (char1 != *char2p);
            x++;
            if (x > c3)
              x = c3;
            *p = x;
        }
    }
    i = *end;
    PyMem_Free(row);
    return i;
}

static inline PyObject*
calculate_suggestions(PyObject* dir,
                      PyObject* name,
                      PyObject* oldexceptionvalue)
{
    assert(PyList_CheckExact(dir));

    Py_ssize_t dir_size = PyList_GET_SIZE(dir);
    if (dir_size >= MAX_GETATTR_PREDICT_ITEMS) {
        return NULL;
    }

    int suggestion_distance = PyUnicode_GetLength(name);
    PyObject* suggestion = NULL;
    for (int i = 0; i < dir_size; ++i) {
        PyObject *item = PyList_GET_ITEM(dir, i);
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            PyErr_Clear();
            continue;
        }
        int current_distance = distance(PyUnicode_AsUTF8(name),
                                        PyUnicode_AsUTF8(item));
        if (current_distance > MAX_GETATTR_PREDICT_DIST){
            continue;
        }
        if (!suggestion || current_distance < suggestion_distance) {
            suggestion = item;
            suggestion_distance = current_distance;
        }       
    }
    if (!suggestion) {
        return NULL;
    }
    return PyUnicode_FromFormat("%S\n\nDid you mean: %U?",
                                oldexceptionvalue, suggestion);
}

int
_Py_offer_suggestions_for_attribute_error(PyAttributeErrorObject* exc) {
    int return_val = 0;

    PyObject* name = exc->name;
    PyObject* v = exc->obj;

    // Aboirt if we don't have an attribute name or we have an invalid one
    if ((name == NULL) || (v == NULL) || !PyUnicode_CheckExact(name)) {
        return -1;
    }
    
    PyObject* oldexceptionvalue = NULL;
    Py_ssize_t nargs = PyTuple_GET_SIZE(exc->args);
    switch (nargs) {
        case 0:
            oldexceptionvalue = PyUnicode_New(0, 0);
            break;
        case 1:
            oldexceptionvalue = PyTuple_GET_ITEM(exc->args, 0);
            Py_INCREF(oldexceptionvalue);
            // Check that the the message is an uncode objects that we can use.
            if (!PyUnicode_CheckExact(oldexceptionvalue)) {
                return_val = -1;
                goto exit;
            }
            break;
        default:
            // Exceptions with more than 1 value in args are not
            // formatted using args, so no need to make a new suggestion.
            return 0;
    }

    PyObject* dir = PyObject_Dir(v);
    if (!dir) {
        goto exit;
    }

    PyObject* newexceptionvalue = calculate_suggestions(dir, name, oldexceptionvalue);
    Py_DECREF(dir);

    if (newexceptionvalue == NULL) {
        // We did't find a suggestion :(
        goto exit;
    }

    PyObject* old_args = exc->args;
    PyObject* new_args = PyTuple_Pack(1, newexceptionvalue);
    Py_DECREF(newexceptionvalue);
    if (new_args == NULL) {
        return_val = -1;
        goto exit;
    }
    exc->args = new_args;
    Py_DECREF(old_args);

exit:
    Py_DECREF(oldexceptionvalue);

    // Clear any error that may have happened.
    PyErr_Clear();
    return return_val;
}


