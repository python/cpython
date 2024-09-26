/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(code_new__doc__,
"code(argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize,\n"
"     flags, codestring, constants, names, varnames, filename, name,\n"
"     qualname, firstlineno, linetable, exceptiontable, freevars=(),\n"
"     cellvars=(), /)\n"
"--\n"
"\n"
"Create a code object.  Not for the faint of heart.");

static PyObject *
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              PyObject *qualname, int firstlineno, PyObject *linetable,
              PyObject *exceptiontable, PyObject *freevars,
              PyObject *cellvars);

static PyObject *
code_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyCode_Type;
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
    PyObject *qualname;
    int firstlineno;
    PyObject *linetable;
    PyObject *exceptiontable;
    PyObject *freevars = NULL;
    PyObject *cellvars = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("code", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("code", PyTuple_GET_SIZE(args), 16, 18)) {
        goto exit;
    }
    argcount = PyLong_AsInt(PyTuple_GET_ITEM(args, 0));
    if (argcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    posonlyargcount = PyLong_AsInt(PyTuple_GET_ITEM(args, 1));
    if (posonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    kwonlyargcount = PyLong_AsInt(PyTuple_GET_ITEM(args, 2));
    if (kwonlyargcount == -1 && PyErr_Occurred()) {
        goto exit;
    }
    nlocals = PyLong_AsInt(PyTuple_GET_ITEM(args, 3));
    if (nlocals == -1 && PyErr_Occurred()) {
        goto exit;
    }
    stacksize = PyLong_AsInt(PyTuple_GET_ITEM(args, 4));
    if (stacksize == -1 && PyErr_Occurred()) {
        goto exit;
    }
    flags = PyLong_AsInt(PyTuple_GET_ITEM(args, 5));
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
    filename = PyTuple_GET_ITEM(args, 10);
    if (!PyUnicode_Check(PyTuple_GET_ITEM(args, 11))) {
        _PyArg_BadArgument("code", "argument 12", "str", PyTuple_GET_ITEM(args, 11));
        goto exit;
    }
    name = PyTuple_GET_ITEM(args, 11);
    if (!PyUnicode_Check(PyTuple_GET_ITEM(args, 12))) {
        _PyArg_BadArgument("code", "argument 13", "str", PyTuple_GET_ITEM(args, 12));
        goto exit;
    }
    qualname = PyTuple_GET_ITEM(args, 12);
    firstlineno = PyLong_AsInt(PyTuple_GET_ITEM(args, 13));
    if (firstlineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyBytes_Check(PyTuple_GET_ITEM(args, 14))) {
        _PyArg_BadArgument("code", "argument 15", "bytes", PyTuple_GET_ITEM(args, 14));
        goto exit;
    }
    linetable = PyTuple_GET_ITEM(args, 14);
    if (!PyBytes_Check(PyTuple_GET_ITEM(args, 15))) {
        _PyArg_BadArgument("code", "argument 16", "bytes", PyTuple_GET_ITEM(args, 15));
        goto exit;
    }
    exceptiontable = PyTuple_GET_ITEM(args, 15);
    if (PyTuple_GET_SIZE(args) < 17) {
        goto skip_optional;
    }
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 16))) {
        _PyArg_BadArgument("code", "argument 17", "tuple", PyTuple_GET_ITEM(args, 16));
        goto exit;
    }
    freevars = PyTuple_GET_ITEM(args, 16);
    if (PyTuple_GET_SIZE(args) < 18) {
        goto skip_optional;
    }
    if (!PyTuple_Check(PyTuple_GET_ITEM(args, 17))) {
        _PyArg_BadArgument("code", "argument 18", "tuple", PyTuple_GET_ITEM(args, 17));
        goto exit;
    }
    cellvars = PyTuple_GET_ITEM(args, 17);
skip_optional:
    return_value = code_new_impl(type, argcount, posonlyargcount, kwonlyargcount, nlocals, stacksize, flags, code, consts, names, varnames, filename, name, qualname, firstlineno, linetable, exceptiontable, freevars, cellvars);

exit:
    return return_value;
}

PyDoc_STRVAR(code_replace__doc__,
"replace($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy of the code object with new values for the specified fields.");

#define CODE_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(code_replace), METH_FASTCALL|METH_KEYWORDS, code_replace__doc__},

static PyObject *
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyObject *co_code, PyObject *co_consts,
                  PyObject *co_names, PyObject *co_varnames,
                  PyObject *co_freevars, PyObject *co_cellvars,
                  PyObject *co_filename, PyObject *co_name,
                  PyObject *co_qualname, PyObject *co_linetable,
                  PyObject *co_exceptiontable);

static PyObject *
code_replace(PyCodeObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 18
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(co_argcount), &_Py_ID(co_posonlyargcount), &_Py_ID(co_kwonlyargcount), &_Py_ID(co_nlocals), &_Py_ID(co_stacksize), &_Py_ID(co_flags), &_Py_ID(co_firstlineno), &_Py_ID(co_code), &_Py_ID(co_consts), &_Py_ID(co_names), &_Py_ID(co_varnames), &_Py_ID(co_freevars), &_Py_ID(co_cellvars), &_Py_ID(co_filename), &_Py_ID(co_name), &_Py_ID(co_qualname), &_Py_ID(co_linetable), &_Py_ID(co_exceptiontable), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"co_argcount", "co_posonlyargcount", "co_kwonlyargcount", "co_nlocals", "co_stacksize", "co_flags", "co_firstlineno", "co_code", "co_consts", "co_names", "co_varnames", "co_freevars", "co_cellvars", "co_filename", "co_name", "co_qualname", "co_linetable", "co_exceptiontable", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[18];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int co_argcount = self->co_argcount;
    int co_posonlyargcount = self->co_posonlyargcount;
    int co_kwonlyargcount = self->co_kwonlyargcount;
    int co_nlocals = self->co_nlocals;
    int co_stacksize = self->co_stacksize;
    int co_flags = self->co_flags;
    int co_firstlineno = self->co_firstlineno;
    PyObject *co_code = NULL;
    PyObject *co_consts = self->co_consts;
    PyObject *co_names = self->co_names;
    PyObject *co_varnames = NULL;
    PyObject *co_freevars = NULL;
    PyObject *co_cellvars = NULL;
    PyObject *co_filename = self->co_filename;
    PyObject *co_name = self->co_name;
    PyObject *co_qualname = self->co_qualname;
    PyObject *co_linetable = self->co_linetable;
    PyObject *co_exceptiontable = self->co_exceptiontable;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[0]) {
        co_argcount = PyLong_AsInt(args[0]);
        if (co_argcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[1]) {
        co_posonlyargcount = PyLong_AsInt(args[1]);
        if (co_posonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        co_kwonlyargcount = PyLong_AsInt(args[2]);
        if (co_kwonlyargcount == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[3]) {
        co_nlocals = PyLong_AsInt(args[3]);
        if (co_nlocals == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[4]) {
        co_stacksize = PyLong_AsInt(args[4]);
        if (co_stacksize == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[5]) {
        co_flags = PyLong_AsInt(args[5]);
        if (co_flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[6]) {
        co_firstlineno = PyLong_AsInt(args[6]);
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
        co_code = args[7];
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
        co_name = args[14];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[15]) {
        if (!PyUnicode_Check(args[15])) {
            _PyArg_BadArgument("replace", "argument 'co_qualname'", "str", args[15]);
            goto exit;
        }
        co_qualname = args[15];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[16]) {
        if (!PyBytes_Check(args[16])) {
            _PyArg_BadArgument("replace", "argument 'co_linetable'", "bytes", args[16]);
            goto exit;
        }
        co_linetable = args[16];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyBytes_Check(args[17])) {
        _PyArg_BadArgument("replace", "argument 'co_exceptiontable'", "bytes", args[17]);
        goto exit;
    }
    co_exceptiontable = args[17];
skip_optional_kwonly:
    return_value = code_replace_impl(self, co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals, co_stacksize, co_flags, co_firstlineno, co_code, co_consts, co_names, co_varnames, co_freevars, co_cellvars, co_filename, co_name, co_qualname, co_linetable, co_exceptiontable);

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
    {"_varname_from_oparg", _PyCFunction_CAST(code__varname_from_oparg), METH_FASTCALL|METH_KEYWORDS, code__varname_from_oparg__doc__},

static PyObject *
code__varname_from_oparg_impl(PyCodeObject *self, int oparg);

static PyObject *
code__varname_from_oparg(PyCodeObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(oparg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"oparg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_varname_from_oparg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int oparg;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    oparg = PyLong_AsInt(args[0]);
    if (oparg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = code__varname_from_oparg_impl(self, oparg);

exit:
    return return_value;
}
/*[clinic end generated code: output=d604263a3ca72a0f input=a9049054013a1b77]*/
