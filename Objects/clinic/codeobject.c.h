/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(code_new__doc__,
"code(argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize,\n"
"     flags, codestring, constants, names, varnames, filename, name,\n"
"     firstlineno, linetable, exceptiontable, freevars=(), cellvars=(),\n"
"     /)\n"
"--\n"
"\n"
"Create a code object.  Not for the faint of heart.");

static PyObject *
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              int firstlineno, PyObject *linetable, PyObject *exceptiontable,
              PyObject *freevars, PyObject *cellvars);

static PyObject *
code_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    int argcount;
    int posonlyargcount;
    int kwonlyargcount;
    int nlocals;
    int stacksize;
    int flags;
    PyObject *code;
    PyObject *consts;
    PyObject *names;
    PyObject *varnames;
    PyObject *filename;
    PyObject *name;
    int firstlineno;
    PyObject *linetable;
    PyObject *exceptiontable;
    PyObject *freevars = NULL;
    PyObject *cellvars = NULL;

    if ((type == &PyCode_Type) &&
        !_PyArg_NoKeywords("code", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("code", PyTuple_GET_SIZE(args), 15, 17)) {
        goto exit;
    }
    argcount = _PyLong_AsInt(PyTuple_GET_ITEM(args, 0));
    if (argcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    posonlyargcount = _PyLong_AsInt(PyTuple_GET_ITEM(args, 1));
    if (posonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    kwonlyargcount = _PyLong_AsInt(PyTuple_GET_ITEM(args, 2));
    if (kwonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    nlocals = _PyLong_AsInt(PyTuple_GET_ITEM(args, 3));
    if (nlocals == -1 && PyErr_Occurred()) {
        goto exit;
    }
    stacksize = _PyLong_AsInt(PyTuple_GET_ITEM(args, 4));
    if (stacksize == -1 && PyErr_Occurred()) {
        goto exit;
    }
    flags = _PyLong_AsInt(PyTuple_GET_ITEM(args, 5));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyBytes_Check(PyTuple_GET_ITEM(args, 6))) {
        _PyArg_BadArgument("code", "argument 7", "bytes", PyTuple_GET_ITEM(args, 6));
        goto exit;
    }
    code = PyTuple_GET_ITEM(args, 6);
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 7))) {
        _PyArg_BadArgument("code", "argument 8", "tuple", PyTuple_GET_ITEM(args, 7));
        goto exit;
    }
    consts = PyTuple_GET_ITEM(args, 7);
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 8))) {
        _PyArg_BadArgument("code", "argument 9", "tuple", PyTuple_GET_ITEM(args, 8));
        goto exit;
    }
    names = PyTuple_GET_ITEM(args, 8);
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 9))) {
        _PyArg_BadArgument("code", "argument 10", "tuple", PyTuple_GET_ITEM(args, 9));
        goto exit;
    }
    varnames = PyTuple_GET_ITEM(args, 9);
    if (!PyUnicode_Check(PyTuple_GET_ITEM(args, 10))) {
        _PyArg_BadArgument("code", "argument 11", "str", PyTuple_GET_ITEM(args, 10));
        goto exit;
    }
    if (PyUnicode_READY(PyTuple_GET_ITEM(args, 10)) == -1) {
        goto exit;
    }
    filename = PyTuple_GET_ITEM(args, 10);
    if (!PyUnicode_Check(PyTuple_GET_ITEM(args, 11))) {
        _PyArg_BadArgument("code", "argument 12", "str", PyTuple_GET_ITEM(args, 11));
        goto exit;
    }
    if (PyUnicode_READY(PyTuple_GET_ITEM(args, 11)) == -1) {
        goto exit;
    }
    name = PyTuple_GET_ITEM(args, 11);
    firstlineno = _PyLong_AsInt(PyTuple_GET_ITEM(args, 12));
    if (firstlineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyBytes_Check(PyTuple_GET_ITEM(args, 13))) {
        _PyArg_BadArgument("code", "argument 14", "bytes", PyTuple_GET_ITEM(args, 13));
        goto exit;
    }
    linetable = PyTuple_GET_ITEM(args, 13);
    if (!PyBytes_Check(PyTuple_GET_ITEM(args, 14))) {
        _PyArg_BadArgument("code", "argument 15", "bytes", PyTuple_GET_ITEM(args, 14));
        goto exit;
    }
    exceptiontable = PyTuple_GET_ITEM(args, 14);
    if (PyTuple_GET_SIZE(args) < 16) {
        goto skip_optional;
    }
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 15))) {
        _PyArg_BadArgument("code", "argument 16", "tuple", PyTuple_GET_ITEM(args, 15));
        goto exit;
    }
    freevars = PyTuple_GET_ITEM(args, 15);
    if (PyTuple_GET_SIZE(args) < 17) {
        goto skip_optional;
    }
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 16))) {
        _PyArg_BadArgument("code", "argument 17", "tuple", PyTuple_GET_ITEM(args, 16));
        goto exit;
    }
    cellvars = PyTuple_GET_ITEM(args, 16);
skip_optional:
    return_value = code_new_impl(type, argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize, flags, code, consts, names, varnames, filename, name, firstlineno, linetable, exceptiontable, freevars, cellvars);

exit:
    return return_value;
}

PyDoc_STRVAR(code_replace__doc__,
"replace($self, /, *, co_argcount=-1, co_posonlyargcount=-1,\n"
"        co_kwonlyargcount=-1, co_nlocals=-1, co_stacksize=-1,\n"
"        co_flags=-1, co_firstlineno=-1, co_code=None, co_consts=None,\n"
"        co_names=None, co_varnames=None, co_freevars=None,\n"
"        co_cellvars=None, co_filename=None, co_name=None,\n"
"        co_linetable=None, co_exceptiontable=None)\n"
"--\n"
"\n"
"Return a copy of the code object with new values for the specified fields.");

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
                  PyObject *co_name, PyBytesObject *co_linetable,
                  PyBytesObject *co_exceptiontable);

static PyObject *
code_replace(PyCodeObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"co_argcount", "co_posonlyargcount", "co_kwonlyargcount", "co_nlocals", "co_stacksize", "co_flags", "co_firstlineno", "co_code", "co_consts", "co_names", "co_varnames", "co_freevars", "co_cellvars", "co_filename", "co_name", "co_linetable", "co_exceptiontable", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "replace", 0};
    PyObject *argsbuf[17];
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
    PyBytesObject *co_linetable = (PyBytesObject *)self->co_linetable;
    PyBytesObject *co_exceptiontable = (PyBytesObject *)self->co_exceptiontable;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[0]) {
        co_argcount = _PyLong_AsInt(args[0]);
        if (co_argcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[1]) {
        co_posonlyargcount = _PyLong_AsInt(args[1]);
        if (co_posonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        co_kwonlyargcount = _PyLong_AsInt(args[2]);
        if (co_kwonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        co_nlocals = _PyLong_AsInt(args[3]);
        if (co_nlocals == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[4]) {
        co_stacksize = _PyLong_AsInt(args[4]);
        if (co_stacksize == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[5]) {
        co_flags = _PyLong_AsInt(args[5]);
        if (co_flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[6]) {
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
            _PyArg_BadArgument("replace", "argument 'co_code'", "bytes", args[7]);
            goto exit;
        }
        co_code = (PyBytesObject *)args[7];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[8]) {
        if (!PyTuple_Check(args[8])) {
            _PyArg_BadArgument("replace", "argument 'co_consts'", "tuple", args[8]);
            goto exit;
        }
        co_consts = args[8];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[9]) {
        if (!PyTuple_Check(args[9])) {
            _PyArg_BadArgument("replace", "argument 'co_names'", "tuple", args[9]);
            goto exit;
        }
        co_names = args[9];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[10]) {
        if (!PyTuple_Check(args[10])) {
            _PyArg_BadArgument("replace", "argument 'co_varnames'", "tuple", args[10]);
            goto exit;
        }
        co_varnames = args[10];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[11]) {
        if (!PyTuple_Check(args[11])) {
            _PyArg_BadArgument("replace", "argument 'co_freevars'", "tuple", args[11]);
            goto exit;
        }
        co_freevars = args[11];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[12]) {
        if (!PyTuple_Check(args[12])) {
            _PyArg_BadArgument("replace", "argument 'co_cellvars'", "tuple", args[12]);
            goto exit;
        }
        co_cellvars = args[12];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[13]) {
        if (!PyUnicode_Check(args[13])) {
            _PyArg_BadArgument("replace", "argument 'co_filename'", "str", args[13]);
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
            _PyArg_BadArgument("replace", "argument 'co_name'", "str", args[14]);
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
    if (args[15]) {
        if (!PyBytes_Check(args[15])) {
            _PyArg_BadArgument("replace", "argument 'co_linetable'", "bytes", args[15]);
            goto exit;
        }
        co_linetable = (PyBytesObject *)args[15];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyBytes_Check(args[16])) {
        _PyArg_BadArgument("replace", "argument 'co_exceptiontable'", "bytes", args[16]);
        goto exit;
    }
    co_exceptiontable = (PyBytesObject *)args[16];
skip_optional_kwonly:
    return_value = code_replace_impl(self, co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals, co_stacksize, co_flags, co_firstlineno, co_code, co_consts, co_names, co_varnames, co_freevars, co_cellvars, co_filename, co_name, co_linetable, co_exceptiontable);

exit:
    return return_value;
}

PyDoc_STRVAR(code__varname_from_oparg__doc__,
"_varname_from_oparg($self, /, oparg)\n"
"--\n"
"\n"
"(internal-only) Return the local variable name for the given oparg.\n"
"\n"
"WARNING: this method is for internal use only and may change or go away.");

#define CODE__VARNAME_FROM_OPARG_METHODDEF    \
    {"_varname_from_oparg", (PyCFunction)(void(*)(void))code__varname_from_oparg, METH_FASTCALL|METH_KEYWORDS, code__varname_from_oparg__doc__},

static PyObject *
code__varname_from_oparg_impl(PyCodeObject *self, int oparg);

static PyObject *
code__varname_from_oparg(PyCodeObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"oparg", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_varname_from_oparg", 0};
    PyObject *argsbuf[1];
    int oparg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    oparg = _PyLong_AsInt(args[0]);
    if (oparg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = code__varname_from_oparg_impl(self, oparg);

exit:
    return return_value;
}
/*[clinic end generated code: output=ba4c5487e0364ce8 input=a9049054013a1b77]*/
