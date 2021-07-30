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
"cast($module, /, typ, val)\n"
"--\n"
"\n"
"Cast a value to a type.\n"
"\n"
"This returns the value unchanged.  To the type checker this\n"
"signals that the return value has the designated type, but at\n"
"runtime we intentionally don\'t check anything (we want this\n"
"to be as fast as possible).");

#define _TYPING_CAST_METHODDEF    \
    {"cast", (PyCFunction)(void(*)(void))_typing_cast, METH_FASTCALL|METH_KEYWORDS, _typing_cast__doc__},

static PyObject *
_typing_cast_impl(PyObject *module, PyObject *typ, PyObject *val);

static PyObject *
_typing_cast(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"typ", "val", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "cast", 0};
    PyObject *argsbuf[2];
    PyObject *typ;
    PyObject *val;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    typ = args[0];
    val = args[1];
    return_value = _typing_cast_impl(module, typ, val);

exit:
    return return_value;
}
/*[clinic end generated code: output=bf387753f9744405 input=a9049054013a1b77]*/
