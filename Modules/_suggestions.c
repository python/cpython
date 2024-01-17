#include "Python.h"
#include "pycore_pyerrors.h"
#include "clinic/_suggestions.c.h"

/*[clinic input]
module _suggestions
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e58d81fafad5637b]*/

/*[clinic input]
_suggestions._generate_suggestions
    candidates: object
    item: unicode
    /
Returns the candidate in candidates that's closest to item
[clinic start generated code]*/

static PyObject *
_suggestions__generate_suggestions_impl(PyObject *module,
                                        PyObject *candidates, PyObject *item)
/*[clinic end generated code: output=79be7b653ae5e7ca input=ba2a8dddc654e33a]*/
{
   // Check if dir is a list
    if (!PyList_Check(candidates)) {
        PyErr_SetString(PyExc_TypeError, "candidates must be a list");
        return NULL;
    }

    // Check if all elements in the list are Unicode
    Py_ssize_t size = PyList_Size(candidates);
    for (Py_ssize_t i = 0; i < size; ++i) {
        PyObject *elem = PyList_GetItem(candidates, i);
        if (!PyUnicode_Check(elem)) {
            PyErr_SetString(PyExc_TypeError, "all elements in 'candidates' must be strings");
            return NULL;
        }
    }

    PyObject* result =  _Py_CalculateSuggestions(candidates, item);
    if (!result && !PyErr_Occurred()) {
        Py_RETURN_NONE;
    }
    return result;
}


static PyMethodDef module_methods[] = {
    _SUGGESTIONS__GENERATE_SUGGESTIONS_METHODDEF
    {NULL, NULL, 0, NULL} // Sentinel
};

static struct PyModuleDef suggestions_module = {
    PyModuleDef_HEAD_INIT,
    "_suggestions",
    NULL,
    -1,
    module_methods
};

PyMODINIT_FUNC PyInit__suggestions(void) {
    return PyModule_Create(&suggestions_module);
}

