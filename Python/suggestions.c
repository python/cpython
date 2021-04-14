#include "Python.h"

#include "pycore_pyerrors.h"

#define MAX_DISTANCE 3
#define MAX_CANDIDATE_ITEMS 100
#define MAX_STRING_SIZE 20

/* Calculate the Levenshtein distance between string1 and string2 */
static size_t
levenshtein_distance(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }

    const size_t a_size = strlen(a);
    const size_t b_size = strlen(b);

    if (a_size > MAX_STRING_SIZE || b_size > MAX_STRING_SIZE) {
        return 0;
    }

    // Both strings are the same (by identity)
    if (a == b) {
        return 0;
    }

    // The first string is empty
    if (a_size == 0) {
        return b_size;
    }

    // The second string is empty
    if (b_size == 0) {
        return a_size;
    }

    size_t *buffer = PyMem_Calloc(a_size, sizeof(size_t));
    if (buffer == NULL) {
        return 0;
    }

    // Initialize the buffer row
    size_t index = 0;
    while (index < a_size) {
        buffer[index] = index + 1;
        index++;
    }

    size_t b_index = 0;
    size_t result = 0;
    while (b_index < b_size) {
        char code = b[b_index];
        size_t distance = result = b_index++;
        index = SIZE_MAX;
        while (++index < a_size) {
            size_t b_distance = code == a[index] ? distance : distance + 1;
            distance = buffer[index];
            if (distance > result) {
                if (b_distance > result) {
                    result = result + 1;
                } else {
                    result = b_distance;
                }
            } else {
                if (b_distance > distance) {
                    result = distance + 1;
                } else {
                    result = b_distance;
                }
            }
            buffer[index] = result;
        }
    }
    PyMem_Free(buffer);
    return result;
}

static inline PyObject *
calculate_suggestions(PyObject *dir,
                      PyObject *name) {
    assert(!PyErr_Occurred());
    assert(PyList_CheckExact(dir));

    Py_ssize_t dir_size = PyList_GET_SIZE(dir);
    if (dir_size >= MAX_CANDIDATE_ITEMS) {
        return NULL;
    }

    Py_ssize_t suggestion_distance = PyUnicode_GetLength(name);
    PyObject *suggestion = NULL;
    for (int i = 0; i < dir_size; ++i) {
        PyObject *item = PyList_GET_ITEM(dir, i);
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            PyErr_Clear();
            continue;
        }
        Py_ssize_t current_distance = levenshtein_distance(PyUnicode_AsUTF8(name), PyUnicode_AsUTF8(item));
        if (current_distance == 0 || current_distance > MAX_DISTANCE) {
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
    Py_INCREF(suggestion);
    return suggestion;
}

static PyObject *
offer_suggestions_for_attribute_error(PyAttributeErrorObject *exc) {
    PyObject *name = exc->name; // borrowed reference
    PyObject *obj = exc->obj; // borrowed reference

    // Abort if we don't have an attribute name or we have an invalid one
    if (name == NULL || obj == NULL || !PyUnicode_CheckExact(name)) {
        return NULL;
    }

    PyObject *dir = PyObject_Dir(obj);
    if (dir == NULL) {
        return NULL;
    }

    PyObject *suggestions = calculate_suggestions(dir, name);
    Py_DECREF(dir);
    return suggestions;
}

// Offer suggestions for a given exception. Returns a python string object containing the
// suggestions. This function does not raise exceptions and returns NULL if no suggestion was found.
PyObject *_Py_Offer_Suggestions(PyObject *exception) {
    PyObject *result = NULL;
    assert(!PyErr_Occurred()); // Check that we are not going to clean any existing exception
    if (PyErr_GivenExceptionMatches(exception, PyExc_AttributeError)) {
        result = offer_suggestions_for_attribute_error((PyAttributeErrorObject *) exception);
    }
    assert(!PyErr_Occurred());
    return result;
}

