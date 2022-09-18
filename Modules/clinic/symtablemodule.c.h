/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_symtable_symtable__doc__,
"symtable($module, source, filename, startstr, /)\n"
"--\n"
"\n"
"Return symbol and scope dictionaries used internally by compiler.");

#define _SYMTABLE_SYMTABLE_METHODDEF    \
    {"symtable", _PyCFunction_CAST(_symtable_symtable), METH_FASTCALL, _symtable_symtable__doc__},

static PyObject *
_symtable_symtable_impl(PyObject *module, PyObject *source,
                        PyObject *filename, const char *startstr);

static PyObject *
_symtable_symtable(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *source;
    PyObject *filename;
    const char *startstr;

    if (!_PyArg_CheckPositional("symtable", nargs, 3, 3)) {
        goto exit;
    }
    source = args[0];
    if (!PyUnicode_FSDecoder(args[1], &filename)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("symtable", "argument 3", "str", args[2]);
        goto exit;
    }
    Py_ssize_t startstr_length;
    startstr = PyUnicode_AsUTF8AndSize(args[2], &startstr_length);
    if (startstr == NULL) {
        goto exit;
    }
    if (strlen(startstr) != (size_t)startstr_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _symtable_symtable_impl(module, source, filename, startstr);

exit:
    return return_value;
}
/*[clinic end generated code: output=3f7ccf535d750238 input=a9049054013a1b77]*/
