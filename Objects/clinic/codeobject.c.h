/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(code_replace__doc__,
"replace($self, /, *, co_argcount=-1, co_posonlyargcount=-1,\n"
"        co_kwonlyargcount=-1, co_nlocals=-1, co_stacksize=-1,\n"
"        co_flags=-1, co_firstlineno=-1, co_code=None, co_consts=None,\n"
"        co_names=None, co_varnames=None, co_freevars=None,\n"
"        co_cellvars=None, co_filename=None, co_name=None,\n"
"        co_lnotab=None)\n"
"--\n"
"\n"
"Return a new code object with new specified fields.");

#define CODE_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)(void(*)(void))code_replace, METH_FASTCALL|METH_KEYWORDS, code_replace__doc__},

static PyObject *
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyBytesObject *co_code,
                  PyObject *co_consts, PyObject *co_names,
                  PyObject *co_varnames, PyObject *co_freevars,
                  PyObject *co_cellvars, PyObject *co_filename,
                  PyObject *co_name, PyBytesObject *co_lnotab);

static PyObject *
code_replace(PyCodeObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"co_argcount", "co_posonlyargcount", "co_kwonlyargcount", "co_nlocals", "co_stacksize", "co_flags", "co_firstlineno", "co_code", "co_consts", "co_names", "co_varnames", "co_freevars", "co_cellvars", "co_filename", "co_name", "co_lnotab", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "replace", 0};
    PyObject *argsbuf[16];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int co_argcount = self->co_argcount;
    int co_posonlyargcount = self->co_posonlyargcount;
    int co_kwonlyargcount = self->co_kwonlyargcount;
    int co_nlocals = self->co_nlocals;
    int co_stacksize = self->co_stacksize;
    int co_flags = self->co_flags;
    int co_firstlineno = self->co_firstlineno;
    PyBytesObject *co_code = (PyBytesObject *)self->co_code;
    PyObject *co_consts = self->co_consts;
    PyObject *co_names = self->co_names;
    PyObject *co_varnames = self->co_varnames;
    PyObject *co_freevars = self->co_freevars;
    PyObject *co_cellvars = self->co_cellvars;
    PyObject *co_filename = self->co_filename;
    PyObject *co_name = self->co_name;
    PyBytesObject *co_lnotab = (PyBytesObject *)self->co_lnotab;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[0]) {
        if (PyFloat_Check(args[0])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        co_argcount = _PyLong_AsInt(args[0]);
        if (co_argcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[1]) {
        if (PyFloat_Check(args[1])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        co_posonlyargcount = _PyLong_AsInt(args[1]);
        if (co_posonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        if (PyFloat_Check(args[2])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        co_kwonlyargcount = _PyLong_AsInt(args[2]);
        if (co_kwonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        if (PyFloat_Check(args[3])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        co_nlocals = _PyLong_AsInt(args[3]);
        if (co_nlocals == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[4]) {
        if (PyFloat_Check(args[4])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        co_stacksize = _PyLong_AsInt(args[4]);
        if (co_stacksize == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[5]) {
        if (PyFloat_Check(args[5])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        co_flags = _PyLong_AsInt(args[5]);
        if (co_flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[6]) {
        if (PyFloat_Check(args[6])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        co_firstlineno = _PyLong_AsInt(args[6]);
        if (co_firstlineno == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[7]) {
        if (!PyBytes_Check(args[7])) {
            _PyArg_BadArgument("replace", 8, "bytes", args[7]);
            goto exit;
        }
        co_code = (PyBytesObject *)args[7];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[8]) {
        if (!PyTuple_Check(args[8])) {
            _PyArg_BadArgument("replace", 9, "tuple", args[8]);
            goto exit;
        }
        co_consts = args[8];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[9]) {
        if (!PyTuple_Check(args[9])) {
            _PyArg_BadArgument("replace", 10, "tuple", args[9]);
            goto exit;
        }
        co_names = args[9];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[10]) {
        if (!PyTuple_Check(args[10])) {
            _PyArg_BadArgument("replace", 11, "tuple", args[10]);
            goto exit;
        }
        co_varnames = args[10];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[11]) {
        if (!PyTuple_Check(args[11])) {
            _PyArg_BadArgument("replace", 12, "tuple", args[11]);
            goto exit;
        }
        co_freevars = args[11];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[12]) {
        if (!PyTuple_Check(args[12])) {
            _PyArg_BadArgument("replace", 13, "tuple", args[12]);
            goto exit;
        }
        co_cellvars = args[12];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[13]) {
        if (!PyUnicode_Check(args[13])) {
            _PyArg_BadArgument("replace", 14, "str", args[13]);
            goto exit;
        }
        if (PyUnicode_READY(args[13]) == -1) {
            goto exit;
        }
        co_filename = args[13];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[14]) {
        if (!PyUnicode_Check(args[14])) {
            _PyArg_BadArgument("replace", 15, "str", args[14]);
            goto exit;
        }
        if (PyUnicode_READY(args[14]) == -1) {
            goto exit;
        }
        co_name = args[14];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyBytes_Check(args[15])) {
        _PyArg_BadArgument("replace", 16, "bytes", args[15]);
        goto exit;
    }
    co_lnotab = (PyBytesObject *)args[15];
skip_optional_kwonly:
    return_value = code_replace_impl(self, co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals, co_stacksize, co_flags, co_firstlineno, co_code, co_consts, co_names, co_varnames, co_freevars, co_cellvars, co_filename, co_name, co_lnotab);

exit:
    return return_value;
}
/*[clinic end generated code: output=624ab6f2ea8f0ea4 input=a9049054013a1b77]*/
