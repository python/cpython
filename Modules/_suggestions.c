#include "Python.h"
#include "pycore_pyerrors.h"
#include "clinic/_suggestions.c.h"

/*[clinic input]
module _suggestions
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e58d81fafad5637b]*/

/*[clinic input]
@critical_section candidates
_suggestions._generate_suggestions
    candidates: object
    item: unicode
    /
Returns the candidate in candidates that's closest to item
[clinic start generated code]*/

static PyObject *
_suggestions__generate_suggestions_impl(PyObject *module,
                                        PyObject *candidates, PyObject *item)
/*[clinic end generated code: output=79be7b653ae5e7ca input=92861a6c9bd8f667]*/
{
   // Check if dir is a list
    if (!PyList_CheckExact(candidates)) {
        PyErr_SetString(PyExc_TypeError, "candidates must be a list");
        return NULL;
    }

    // Check if all elements in the list are Unicode
    Py_ssize_t size = PyList_Size(candidates);
    for (Py_ssize_t i = 0; i < size; ++i) {
        PyObject *elem = PyList_GET_ITEM(candidates, i);
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

static PyModuleDef_Slot module_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef suggestions_module = {
    PyModuleDef_HEAD_INIT,
    "_suggestions",
    NULL,
    0,
    module_methods,
    module_slots,
};

PyMODINIT_FUNC PyInit__suggestions(void) {
    return PyModuleDef_Init(&suggestions_module);
}
