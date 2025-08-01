/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_tuple.h"         // _PyTuple_FromArray()

PyDoc_STRVAR(_typing__idfunc__doc__,
"_idfunc($module, x, /)\n"
"--\n"
"\n");

#define _TYPING__IDFUNC_METHODDEF    \
    {"_idfunc", (PyCFunction)_typing__idfunc, METH_O, _typing__idfunc__doc__},

PyDoc_STRVAR(_typing__make_union__doc__,
"_make_union($module, /, *args)\n"
"--\n"
"\n");

#define _TYPING__MAKE_UNION_METHODDEF    \
    {"_make_union", _PyCFunction_CAST(_typing__make_union), METH_FASTCALL, _typing__make_union__doc__},

static PyObject *
_typing__make_union_impl(PyObject *module, PyObject *args);

static PyObject *
_typing__make_union(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *__clinic_args = NULL;

    __clinic_args = _PyTuple_FromArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = _typing__make_union_impl(module, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}
/*[clinic end generated code: output=5ad3be515f99ee8a input=a9049054013a1b77]*/
