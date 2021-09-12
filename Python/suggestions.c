#include "Python.h"
#include "frameobject.h"
#include "pycore_frame.h"

#include "pycore_pyerrors.h"
#include "pycore_code.h"        // _PyCode_GetVarnames()

#define MAX_CANDIDATE_ITEMS 750
#define MAX_STRING_SIZE 40

#define MOVE_COST 2
#define CASE_COST 1

#define LEAST_FIVE_BITS(n) ((n) & 31)

static inline int
substitution_cost(char a, char b)
{
    if (LEAST_FIVE_BITS(a) != LEAST_FIVE_BITS(b)) {
        // Not the same, not a case flip.
        return MOVE_COST;
    }
    if (a == b) {
        return 0;
    }
    if ('A' <= a && a <= 'Z') {
        a += ('a' - 'A');
    }
    if ('A' <= b && b <= 'Z') {
        b += ('a' - 'A');
    }
    if (a == b) {
        return CASE_COST;
    }
    return MOVE_COST;
}

/* Calculate the Levenshtein distance between string1 and string2 */
static Py_ssize_t
levenshtein_distance(const char *a, size_t a_size,
                     const char *b, size_t b_size,
                     size_t max_cost)
{
    static size_t buffer[MAX_STRING_SIZE];

    // Both strings are the same (by identity)
    if (a == b) {
        return 0;
    }

    // Trim away common affixes.
    while (a_size && b_size && a[0] == b[0]) {
        a++; a_size--;
        b++; b_size--;
    }
    while (a_size && b_size && a[a_size-1] == b[b_size-1]) {
        a_size--;
        b_size--;
    }
    if (a_size == 0 || b_size == 0) {
        return (a_size + b_size) * MOVE_COST;
    }
    if (a_size > MAX_STRING_SIZE || b_size > MAX_STRING_SIZE) {
        return max_cost + 1;
    }

    // Prefer shorter buffer
    if (b_size < a_size) {
        const char *t = a; a = b; b = t;
        size_t t_size = a_size; a_size = b_size; b_size = t_size;
    }

    // quick fail when a match is impossible.
    if ((b_size - a_size) * MOVE_COST > max_cost) {
        return max_cost + 1;
    }

    // Instead of producing the whole traditional len(a)-by-len(b)
    // matrix, we can update just one row in place.
    // Initialize the buffer row
    for (size_t i = 0; i < a_size; i++) {
        // cost from b[:0] to a[:i+1]
        buffer[i] = (i + 1) * MOVE_COST;
    }

    size_t result = 0;
    for (size_t b_index = 0; b_index < b_size; b_index++) {
        char code = b[b_index];
        // cost(b[:b_index], a[:0]) == b_index * MOVE_COST
        size_t distance = result = b_index * MOVE_COST;
        size_t minimum = SIZE_MAX;
        for (size_t index = 0; index < a_size; index++) {

            // cost(b[:b_index+1], a[:index+1]) = min(
            //     // 1) substitute
            //     cost(b[:b_index], a[:index])
            //         + substitution_cost(b[b_index], a[index]),
            //     // 2) delete from b
            //     cost(b[:b_index], a[:index+1]) + MOVE_COST,
            //     // 3) delete from a
            //     cost(b[:b_index+1], a[index]) + MOVE_COST
            // )

            // 1) Previous distance in this row is cost(b[:b_index], a[:index])
            size_t substitute = distance + substitution_cost(code, a[index]);
            // 2) cost(b[:b_index], a[:index+1]) from previous row
            distance = buffer[index];
            // 3) existing result is cost(b[:b_index+1], a[index])

            size_t insert_delete = Py_MIN(result, distance) + MOVE_COST;
            result = Py_MIN(insert_delete, substitute);

            // cost(b[:b_index+1], a[:index+1])
            buffer[index] = result;
            if (result < minimum) {
                minimum = result;
            }
        }
        if (minimum > max_cost) {
            // Everything in this row is too big, so bail early.
            return max_cost + 1;
        }
    }
    return result;
}

static inline PyObject *
calculate_suggestions(PyObject *dir,
                      PyObject *name)
{
    assert(!PyErr_Occurred());
    assert(PyList_CheckExact(dir));

    Py_ssize_t dir_size = PyList_GET_SIZE(dir);
    if (dir_size >= MAX_CANDIDATE_ITEMS) {
        return NULL;
    }

    Py_ssize_t suggestion_distance = PY_SSIZE_T_MAX;
    PyObject *suggestion = NULL;
    Py_ssize_t name_size;
    const char *name_str = PyUnicode_AsUTF8AndSize(name, &name_size);
    if (name_str == NULL) {
        return NULL;
    }

    for (int i = 0; i < dir_size; ++i) {
        PyObject *item = PyList_GET_ITEM(dir, i);
        Py_ssize_t item_size;
        const char *item_str = PyUnicode_AsUTF8AndSize(item, &item_size);
        if (item_str == NULL) {
            return NULL;
        }
        if (PyUnicode_CompareWithASCIIString(name, item_str) == 0) {
            continue;
        }
        // No more than 1/3 of the involved characters should need changed.
        Py_ssize_t max_distance = (name_size + item_size + 3) * MOVE_COST / 6;
        // Don't take matches we've already beaten.
        max_distance = Py_MIN(max_distance, suggestion_distance - 1);
        Py_ssize_t current_distance =
            levenshtein_distance(name_str, name_size,
                                 item_str, item_size, max_distance);
        if (current_distance > max_distance) {
            continue;
        }
        if (!suggestion || current_distance < suggestion_distance) {
            suggestion = item;
            suggestion_distance = current_distance;
        }
    }
    Py_XINCREF(suggestion);
    return suggestion;
}

static PyObject *
offer_suggestions_for_attribute_error(PyAttributeErrorObject *exc)
{
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


static PyObject *
offer_suggestions_for_name_error(PyNameErrorObject *exc)
{
    PyObject *name = exc->name; // borrowed reference
    PyTracebackObject *traceback = (PyTracebackObject *) exc->traceback; // borrowed reference
    // Abort if we don't have a variable name or we have an invalid one
    // or if we don't have a traceback to work with
    if (name == NULL || traceback == NULL || !PyUnicode_CheckExact(name)) {
        return NULL;
    }

    // Move to the traceback of the exception
    while (traceback->tb_next != NULL) {
        traceback = traceback->tb_next;
    }

    PyFrameObject *frame = traceback->tb_frame;
    assert(frame != NULL);
    PyCodeObject *code = PyFrame_GetCode(frame);
    assert(code != NULL && code->co_localsplusnames != NULL);
    PyObject *varnames = _PyCode_GetVarnames(code);
    if (varnames == NULL) {
        return NULL;
    }
    PyObject *dir = PySequence_List(varnames);
    Py_DECREF(varnames);
    Py_DECREF(code);
    if (dir == NULL) {
        return NULL;
    }

    PyObject *suggestions = calculate_suggestions(dir, name);
    Py_DECREF(dir);
    if (suggestions != NULL) {
        return suggestions;
    }

    dir = PySequence_List(frame->f_frame->f_globals);
    if (dir == NULL) {
        return NULL;
    }
    suggestions = calculate_suggestions(dir, name);
    Py_DECREF(dir);
    if (suggestions != NULL) {
        return suggestions;
    }

    dir = PySequence_List(frame->f_frame->f_builtins);
    if (dir == NULL) {
        return NULL;
    }
    suggestions = calculate_suggestions(dir, name);
    Py_DECREF(dir);

    return suggestions;
}

// Offer suggestions for a given exception. Returns a python string object containing the
// suggestions. This function returns NULL if no suggestion was found or if an exception happened,
// users must call PyErr_Occurred() to disambiguate.
PyObject *
_Py_Offer_Suggestions(PyObject *exception)
{
    PyObject *result = NULL;
    assert(!PyErr_Occurred());
    if (Py_IS_TYPE(exception, (PyTypeObject*)PyExc_AttributeError)) {
        result = offer_suggestions_for_attribute_error((PyAttributeErrorObject *) exception);
    } else if (Py_IS_TYPE(exception, (PyTypeObject*)PyExc_NameError)) {
        result = offer_suggestions_for_name_error((PyNameErrorObject *) exception);
    }
    return result;
}

Py_ssize_t
_Py_UTF8_Edit_Cost(PyObject *a, PyObject *b, Py_ssize_t max_cost)
{
    assert(PyUnicode_Check(a) && PyUnicode_Check(b));
    Py_ssize_t size_a, size_b;
    const char *utf8_a = PyUnicode_AsUTF8AndSize(a, &size_a);
    if (utf8_a == NULL) {
        return -1;
    }
    const char *utf8_b = PyUnicode_AsUTF8AndSize(b, &size_b);
    if (utf8_b == NULL) {
        return -1;
    }
    if (max_cost == -1) {
        max_cost = MOVE_COST * Py_MAX(size_a, size_b);
    }
    return levenshtein_distance(utf8_a, size_a, utf8_b, size_b, max_cost);
}

