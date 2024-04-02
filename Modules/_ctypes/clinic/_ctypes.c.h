/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_ctypes_CType_Type___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return memory consumption of the type object.");

#define _CTYPES_CTYPE_TYPE___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(_ctypes_CType_Type___sizeof__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _ctypes_CType_Type___sizeof____doc__},

static PyObject *
_ctypes_CType_Type___sizeof___impl(PyObject *self, PyTypeObject *cls);

static PyObject *
_ctypes_CType_Type___sizeof__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__sizeof__() takes no arguments");
        return NULL;
    }
    return _ctypes_CType_Type___sizeof___impl(self, cls);
}
/*[clinic end generated code: output=beb4e4935b16dab4 input=a9049054013a1b77]*/
