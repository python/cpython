/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_symtable_symtable__doc__,
"symtable($module, str, filename, startstr, /)\n"
"--\n"
"\n"
"Return symbol and scope dictionaries used internally by compiler.");

#define _SYMTABLE_SYMTABLE_METHODDEF    \
    {"symtable", (PyCFunction)_symtable_symtable, METH_FASTCALL, _symtable_symtable__doc__},

static PyObject *
_symtable_symtable_impl(PyObject *module, const char *str,
                        PyObject *filename, const char *startstr);

static PyObject *
_symtable_symtable(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *str;
    PyObject *filename;
    const char *startstr;

    if (!_PyArg_ParseStack(args, nargs, "sO&s:symtable",
        &str, PyUnicode_FSDecoder, &filename, &startstr)) {
        goto exit;
    }
    return_value = _symtable_symtable_impl(module, str, filename, startstr);

exit:
    return return_value;
}
/*[clinic end generated code: output=c18565060a6cae04 input=a9049054013a1b77]*/
