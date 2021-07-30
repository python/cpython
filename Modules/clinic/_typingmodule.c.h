/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_typing__idfunc__doc__,
"_idfunc($module, x, /)\n"
"--\n"
"\n");

#define _TYPING__IDFUNC_METHODDEF    \
    {"_idfunc", (PyCFunction)_typing__idfunc, METH_O, _typing__idfunc__doc__},

PyDoc_STRVAR(_typing_cast__doc__,
"cast($module, typ, val, /)\n"
"--\n"
"\n");

#define _TYPING_CAST_METHODDEF    \
    {"cast", (PyCFunction)(void(*)(void))_typing_cast, METH_FASTCALL, _typing_cast__doc__},

static PyObject *
_typing_cast_impl(PyObject *module, PyObject *typ, PyObject *val);

static PyObject *
_typing_cast(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *typ;
    PyObject *val;

    if (!_PyArg_CheckPositional("cast", nargs, 2, 2)) {
        goto exit;
    }
    typ = args[0];
    val = args[1];
    return_value = _typing_cast_impl(module, typ, val);

exit:
    return return_value;
}
/*[clinic end generated code: output=3ad826092d5a555d input=a9049054013a1b77]*/
