/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(builtin___import____doc__,
"__import__($module, /, name, globals=None, locals=None, fromlist=(),\n"
"           level=0)\n"
"--\n"
"\n"
"Import a module.\n"
"\n"
"Because this function is meant for use by the Python\n"
"interpreter and not for general use, it is better to use\n"
"importlib.import_module() to programmatically import a module.\n"
"\n"
"The globals argument is only used to determine the context;\n"
"they are not modified.  The locals argument is unused.  The fromlist\n"
"should be a list of names to emulate ``from name import ...``, or an\n"
"empty list to emulate ``import name``.\n"
"When importing a module from a package, note that __import__(\'A.B\', ...)\n"
"returns package A when fromlist is empty, but its submodule B when\n"
"fromlist is not empty.  The level argument is used to determine whether to\n"
"perform absolute or relative imports: 0 is absolute, while a positive number\n"
"is the number of parent directories to search relative to the current module.");

#define BUILTIN___IMPORT___METHODDEF    \
    {"__import__", _PyCFunction_CAST(builtin___import__), METH_FASTCALL|METH_KEYWORDS, builtin___import____doc__},

static PyObject *
builtin___import___impl(PyObject *module, PyObject *name, PyObject *globals,
                        PyObject *locals, PyObject *fromlist, int level);

static PyObject *
builtin___import__(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(globals), &_Py_ID(locals), &_Py_ID(fromlist), &_Py_ID(level), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "globals", "locals", "fromlist", "level", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__import__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *name;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    PyObject *fromlist = NULL;
    int level = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    name = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        globals = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        locals = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        fromlist = args[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    level = PyLong_AsInt(args[4]);
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = builtin___import___impl(module, name, globals, locals, fromlist, level);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_abs__doc__,
"abs($module, x, /)\n"
"--\n"
"\n"
"Return the absolute value of the argument.");

#define BUILTIN_ABS_METHODDEF    \
    {"abs", (PyCFunction)builtin_abs, METH_O, builtin_abs__doc__},

PyDoc_STRVAR(builtin_all__doc__,
"all($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for all values x in the iterable.\n"
"\n"
"If the iterable is empty, return True.");

#define BUILTIN_ALL_METHODDEF    \
    {"all", (PyCFunction)builtin_all, METH_O, builtin_all__doc__},

PyDoc_STRVAR(builtin_any__doc__,
"any($module, iterable, /)\n"
"--\n"
"\n"
"Return True if bool(x) is True for any x in the iterable.\n"
"\n"
"If the iterable is empty, return False.");

#define BUILTIN_ANY_METHODDEF    \
    {"any", (PyCFunction)builtin_any, METH_O, builtin_any__doc__},

PyDoc_STRVAR(builtin_ascii__doc__,
"ascii($module, obj, /)\n"
"--\n"
"\n"
"Return an ASCII-only representation of an object.\n"
"\n"
"As repr(), return a string containing a printable representation of an\n"
"object, but escape the non-ASCII characters in the string returned by\n"
"repr() using \\\\x, \\\\u or \\\\U escapes. This generates a string similar\n"
"to that returned by repr() in Python 2.");

#define BUILTIN_ASCII_METHODDEF    \
    {"ascii", (PyCFunction)builtin_ascii, METH_O, builtin_ascii__doc__},

PyDoc_STRVAR(builtin_bin__doc__,
"bin($module, number, /)\n"
"--\n"
"\n"
"Return the binary representation of an integer.\n"
"\n"
"   >>> bin(2796202)\n"
"   \'0b1010101010101010101010\'");

#define BUILTIN_BIN_METHODDEF    \
    {"bin", (PyCFunction)builtin_bin, METH_O, builtin_bin__doc__},

PyDoc_STRVAR(builtin_callable__doc__,
"callable($module, obj, /)\n"
"--\n"
"\n"
"Return whether the object is callable (i.e., some kind of function).\n"
"\n"
"Note that classes are callable, as are instances of classes with a\n"
"__call__() method.");

#define BUILTIN_CALLABLE_METHODDEF    \
    {"callable", (PyCFunction)builtin_callable, METH_O, builtin_callable__doc__},

PyDoc_STRVAR(builtin_format__doc__,
"format($module, value, format_spec=\'\', /)\n"
"--\n"
"\n"
"Return type(value).__format__(value, format_spec)\n"
"\n"
"Many built-in types implement format_spec according to the\n"
"Format Specification Mini-language. See help(\'FORMATTING\').\n"
"\n"
"If type(value) does not supply a method named __format__\n"
"and format_spec is empty, then str(value) is returned.\n"
"See also help(\'SPECIALMETHODS\').");

#define BUILTIN_FORMAT_METHODDEF    \
    {"format", _PyCFunction_CAST(builtin_format), METH_FASTCALL, builtin_format__doc__},

static PyObject *
builtin_format_impl(PyObject *module, PyObject *value, PyObject *format_spec);

static PyObject *
builtin_format(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *value;
    PyObject *format_spec = NULL;

    if (!_PyArg_CheckPositional("format", nargs, 1, 2)) {
        goto exit;
    }
    value = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("format", "argument 2", "str", args[1]);
        goto exit;
    }
    format_spec = args[1];
skip_optional:
    return_value = builtin_format_impl(module, value, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_chr__doc__,
"chr($module, i, /)\n"
"--\n"
"\n"
"Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.");

#define BUILTIN_CHR_METHODDEF    \
    {"chr", (PyCFunction)builtin_chr, METH_O, builtin_chr__doc__},

PyDoc_STRVAR(builtin_compile__doc__,
"compile($module, /, source, filename, mode, flags=0,\n"
"        dont_inherit=False, optimize=-1, *, _feature_version=-1)\n"
"--\n"
"\n"
"Compile source into a code object that can be executed by exec() or eval().\n"
"\n"
"The source code may represent a Python module, statement or expression.\n"
"The filename will be used for run-time error messages.\n"
"The mode must be \'exec\' to compile a module, \'single\' to compile a\n"
"single (interactive) statement, or \'eval\' to compile an expression.\n"
"The flags argument, if present, controls which future statements influence\n"
"the compilation of the code.\n"
"The dont_inherit argument, if true, stops the compilation inheriting\n"
"the effects of any future statements in effect in the code calling\n"
"compile; if absent or false these statements do influence the compilation,\n"
"in addition to any features explicitly specified.");

#define BUILTIN_COMPILE_METHODDEF    \
    {"compile", _PyCFunction_CAST(builtin_compile), METH_FASTCALL|METH_KEYWORDS, builtin_compile__doc__},

static PyObject *
builtin_compile_impl(PyObject *module, PyObject *source, PyObject *filename,
                     const char *mode, int flags, int dont_inherit,
                     int optimize, int feature_version);

static PyObject *
builtin_compile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 7
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(source), &_Py_ID(filename), &_Py_ID(mode), &_Py_ID(flags), &_Py_ID(dont_inherit), &_Py_ID(optimize), &_Py_ID(_feature_version), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"source", "filename", "mode", "flags", "dont_inherit", "optimize", "_feature_version", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[7];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    PyObject *source;
    PyObject *filename;
    const char *mode;
    int flags = 0;
    int dont_inherit = 0;
    int optimize = -1;
    int feature_version = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    source = args[0];
    if (!PyUnicode_FSDecoder(args[1], &filename)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("compile", "argument 'mode'", "str", args[2]);
        goto exit;
    }
    Py_ssize_t mode_length;
    mode = PyUnicode_AsUTF8AndSize(args[2], &mode_length);
    if (mode == NULL) {
        goto exit;
    }
    if (strlen(mode) != (size_t)mode_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[3]) {
        flags = PyLong_AsInt(args[3]);
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        dont_inherit = PyObject_IsTrue(args[4]);
        if (dont_inherit < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[5]) {
        optimize = PyLong_AsInt(args[5]);
        if (optimize == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    feature_version = PyLong_AsInt(args[6]);
    if (feature_version == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = builtin_compile_impl(module, source, filename, mode, flags, dont_inherit, optimize, feature_version);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_divmod__doc__,
"divmod($module, x, y, /)\n"
"--\n"
"\n"
"Return the tuple (x//y, x%y).  Invariant: div*y + mod == x.");

#define BUILTIN_DIVMOD_METHODDEF    \
    {"divmod", _PyCFunction_CAST(builtin_divmod), METH_FASTCALL, builtin_divmod__doc__},

static PyObject *
builtin_divmod_impl(PyObject *module, PyObject *x, PyObject *y);

static PyObject *
builtin_divmod(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *y;

    if (!_PyArg_CheckPositional("divmod", nargs, 2, 2)) {
        goto exit;
    }
    x = args[0];
    y = args[1];
    return_value = builtin_divmod_impl(module, x, y);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_eval__doc__,
"eval($module, source, /, globals=None, locals=None)\n"
"--\n"
"\n"
"Evaluate the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing a Python expression\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.");

#define BUILTIN_EVAL_METHODDEF    \
    {"eval", _PyCFunction_CAST(builtin_eval), METH_FASTCALL|METH_KEYWORDS, builtin_eval__doc__},

static PyObject *
builtin_eval_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals);

static PyObject *
builtin_eval(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(globals), &_Py_ID(locals), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "globals", "locals", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "eval",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    source = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        globals = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    locals = args[2];
skip_optional_pos:
    return_value = builtin_eval_impl(module, source, globals, locals);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_exec__doc__,
"exec($module, source, /, globals=None, locals=None, *, closure=None)\n"
"--\n"
"\n"
"Execute the given source in the context of globals and locals.\n"
"\n"
"The source may be a string representing one or more Python statements\n"
"or a code object as returned by compile().\n"
"The globals must be a dictionary and locals can be any mapping,\n"
"defaulting to the current globals and locals.\n"
"If only globals is given, locals defaults to it.\n"
"The closure must be a tuple of cellvars, and can only be used\n"
"when source is a code object requiring exactly that many cellvars.");

#define BUILTIN_EXEC_METHODDEF    \
    {"exec", _PyCFunction_CAST(builtin_exec), METH_FASTCALL|METH_KEYWORDS, builtin_exec__doc__},

static PyObject *
builtin_exec_impl(PyObject *module, PyObject *source, PyObject *globals,
                  PyObject *locals, PyObject *closure);

static PyObject *
builtin_exec(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(globals), &_Py_ID(locals), &_Py_ID(closure), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "globals", "locals", "closure", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "exec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *source;
    PyObject *globals = Py_None;
    PyObject *locals = Py_None;
    PyObject *closure = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    source = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        globals = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        locals = args[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    closure = args[3];
skip_optional_kwonly:
    return_value = builtin_exec_impl(module, source, globals, locals, closure);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_globals__doc__,
"globals($module, /)\n"
"--\n"
"\n"
"Return the dictionary containing the current scope\'s global variables.\n"
"\n"
"NOTE: Updates to this dictionary *will* affect name lookups in the current\n"
"global scope and vice-versa.");

#define BUILTIN_GLOBALS_METHODDEF    \
    {"globals", (PyCFunction)builtin_globals, METH_NOARGS, builtin_globals__doc__},

static PyObject *
builtin_globals_impl(PyObject *module);

static PyObject *
builtin_globals(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_globals_impl(module);
}

PyDoc_STRVAR(builtin_hasattr__doc__,
"hasattr($module, obj, name, /)\n"
"--\n"
"\n"
"Return whether the object has an attribute with the given name.\n"
"\n"
"This is done by calling getattr(obj, name) and catching AttributeError.");

#define BUILTIN_HASATTR_METHODDEF    \
    {"hasattr", _PyCFunction_CAST(builtin_hasattr), METH_FASTCALL, builtin_hasattr__doc__},

static PyObject *
builtin_hasattr_impl(PyObject *module, PyObject *obj, PyObject *name);

static PyObject *
builtin_hasattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;

    if (!_PyArg_CheckPositional("hasattr", nargs, 2, 2)) {
        goto exit;
    }
    obj = args[0];
    name = args[1];
    return_value = builtin_hasattr_impl(module, obj, name);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_id__doc__,
"id($module, obj, /)\n"
"--\n"
"\n"
"Return the identity of an object.\n"
"\n"
"This is guaranteed to be unique among simultaneously existing objects.\n"
"(CPython uses the object\'s memory address.)");

#define BUILTIN_ID_METHODDEF    \
    {"id", (PyCFunction)builtin_id, METH_O, builtin_id__doc__},

static PyObject *
builtin_id_impl(PyModuleDef *self, PyObject *v);

static PyObject *
builtin_id(PyObject *self, PyObject *v)
{
    PyObject *return_value = NULL;

    return_value = builtin_id_impl((PyModuleDef *)self, v);

    return return_value;
}

PyDoc_STRVAR(builtin_setattr__doc__,
"setattr($module, obj, name, value, /)\n"
"--\n"
"\n"
"Sets the named attribute on the given object to the specified value.\n"
"\n"
"setattr(x, \'y\', v) is equivalent to ``x.y = v``");

#define BUILTIN_SETATTR_METHODDEF    \
    {"setattr", _PyCFunction_CAST(builtin_setattr), METH_FASTCALL, builtin_setattr__doc__},

static PyObject *
builtin_setattr_impl(PyObject *module, PyObject *obj, PyObject *name,
                     PyObject *value);

static PyObject *
builtin_setattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;
    PyObject *value;

    if (!_PyArg_CheckPositional("setattr", nargs, 3, 3)) {
        goto exit;
    }
    obj = args[0];
    name = args[1];
    value = args[2];
    return_value = builtin_setattr_impl(module, obj, name, value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_delattr__doc__,
"delattr($module, obj, name, /)\n"
"--\n"
"\n"
"Deletes the named attribute from the given object.\n"
"\n"
"delattr(x, \'y\') is equivalent to ``del x.y``");

#define BUILTIN_DELATTR_METHODDEF    \
    {"delattr", _PyCFunction_CAST(builtin_delattr), METH_FASTCALL, builtin_delattr__doc__},

static PyObject *
builtin_delattr_impl(PyObject *module, PyObject *obj, PyObject *name);

static PyObject *
builtin_delattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *name;

    if (!_PyArg_CheckPositional("delattr", nargs, 2, 2)) {
        goto exit;
    }
    obj = args[0];
    name = args[1];
    return_value = builtin_delattr_impl(module, obj, name);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_hash__doc__,
"hash($module, obj, /)\n"
"--\n"
"\n"
"Return the hash value for the given object.\n"
"\n"
"Two objects that compare equal must also have the same hash value, but the\n"
"reverse is not necessarily true.");

#define BUILTIN_HASH_METHODDEF    \
    {"hash", (PyCFunction)builtin_hash, METH_O, builtin_hash__doc__},

PyDoc_STRVAR(builtin_hex__doc__,
"hex($module, number, /)\n"
"--\n"
"\n"
"Return the hexadecimal representation of an integer.\n"
"\n"
"   >>> hex(12648430)\n"
"   \'0xc0ffee\'");

#define BUILTIN_HEX_METHODDEF    \
    {"hex", (PyCFunction)builtin_hex, METH_O, builtin_hex__doc__},

PyDoc_STRVAR(builtin_aiter__doc__,
"aiter($module, async_iterable, /)\n"
"--\n"
"\n"
"Return an AsyncIterator for an AsyncIterable object.");

#define BUILTIN_AITER_METHODDEF    \
    {"aiter", (PyCFunction)builtin_aiter, METH_O, builtin_aiter__doc__},

PyDoc_STRVAR(builtin_anext__doc__,
"anext($module, aiterator, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return the next item from the async iterator.\n"
"\n"
"If default is given and the async iterator is exhausted,\n"
"it is returned instead of raising StopAsyncIteration.");

#define BUILTIN_ANEXT_METHODDEF    \
    {"anext", _PyCFunction_CAST(builtin_anext), METH_FASTCALL, builtin_anext__doc__},

static PyObject *
builtin_anext_impl(PyObject *module, PyObject *aiterator,
                   PyObject *default_value);

static PyObject *
builtin_anext(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *aiterator;
    PyObject *default_value = NULL;

    if (!_PyArg_CheckPositional("anext", nargs, 1, 2)) {
        goto exit;
    }
    aiterator = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = args[1];
skip_optional:
    return_value = builtin_anext_impl(module, aiterator, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_len__doc__,
"len($module, obj, /)\n"
"--\n"
"\n"
"Return the number of items in a container.");

#define BUILTIN_LEN_METHODDEF    \
    {"len", (PyCFunction)builtin_len, METH_O, builtin_len__doc__},

PyDoc_STRVAR(builtin_locals__doc__,
"locals($module, /)\n"
"--\n"
"\n"
"Return a dictionary containing the current scope\'s local variables.\n"
"\n"
"NOTE: Whether or not updates to this dictionary will affect name lookups in\n"
"the local scope and vice-versa is *implementation dependent* and not\n"
"covered by any backwards compatibility guarantees.");

#define BUILTIN_LOCALS_METHODDEF    \
    {"locals", (PyCFunction)builtin_locals, METH_NOARGS, builtin_locals__doc__},

static PyObject *
builtin_locals_impl(PyObject *module);

static PyObject *
builtin_locals(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return builtin_locals_impl(module);
}

PyDoc_STRVAR(builtin_oct__doc__,
"oct($module, number, /)\n"
"--\n"
"\n"
"Return the octal representation of an integer.\n"
"\n"
"   >>> oct(342391)\n"
"   \'0o1234567\'");

#define BUILTIN_OCT_METHODDEF    \
    {"oct", (PyCFunction)builtin_oct, METH_O, builtin_oct__doc__},

PyDoc_STRVAR(builtin_ord__doc__,
"ord($module, c, /)\n"
"--\n"
"\n"
"Return the Unicode code point for a one-character string.");

#define BUILTIN_ORD_METHODDEF    \
    {"ord", (PyCFunction)builtin_ord, METH_O, builtin_ord__doc__},

PyDoc_STRVAR(builtin_pow__doc__,
"pow($module, /, base, exp, mod=None)\n"
"--\n"
"\n"
"Equivalent to base**exp with 2 arguments or base**exp % mod with 3 arguments\n"
"\n"
"Some types, such as ints, are able to use a more efficient algorithm when\n"
"invoked using the three argument form.");

#define BUILTIN_POW_METHODDEF    \
    {"pow", _PyCFunction_CAST(builtin_pow), METH_FASTCALL|METH_KEYWORDS, builtin_pow__doc__},

static PyObject *
builtin_pow_impl(PyObject *module, PyObject *base, PyObject *exp,
                 PyObject *mod);

static PyObject *
builtin_pow(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(base), &_Py_ID(exp), &_Py_ID(mod), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"base", "exp", "mod", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "pow",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *base;
    PyObject *exp;
    PyObject *mod = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    base = args[0];
    exp = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    mod = args[2];
skip_optional_pos:
    return_value = builtin_pow_impl(module, base, exp, mod);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_print__doc__,
"print($module, /, *args, sep=\' \', end=\'\\n\', file=None, flush=False)\n"
"--\n"
"\n"
"Prints the values to a stream, or to sys.stdout by default.\n"
"\n"
"  sep\n"
"    string inserted between values, default a space.\n"
"  end\n"
"    string appended after the last value, default a newline.\n"
"  file\n"
"    a file-like object (stream); defaults to the current sys.stdout.\n"
"  flush\n"
"    whether to forcibly flush the stream.");

#define BUILTIN_PRINT_METHODDEF    \
    {"print", _PyCFunction_CAST(builtin_print), METH_FASTCALL|METH_KEYWORDS, builtin_print__doc__},

static PyObject *
builtin_print_impl(PyObject *module, PyObject * const *args,
                   Py_ssize_t args_length, PyObject *sep, PyObject *end,
                   PyObject *file, int flush);

static PyObject *
builtin_print(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(sep), &_Py_ID(end), &_Py_ID(file), &_Py_ID(flush), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "end", "file", "flush", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "print",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = 0 + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject * const *__clinic_args;
    Py_ssize_t args_length;
    PyObject *sep = Py_None;
    PyObject *end = Py_None;
    PyObject *file = Py_None;
    int flush = 0;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[0]) {
        sep = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[1]) {
        end = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        file = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    flush = PyObject_IsTrue(fastargs[3]);
    if (flush < 0) {
        goto exit;
    }
skip_optional_kwonly:
    __clinic_args = args;
    args_length = nargs;
    return_value = builtin_print_impl(module, __clinic_args, args_length, sep, end, file, flush);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_input__doc__,
"input($module, prompt=\'\', /)\n"
"--\n"
"\n"
"Read a string from standard input.  The trailing newline is stripped.\n"
"\n"
"The prompt string, if given, is printed to standard output without a\n"
"trailing newline before reading input.\n"
"\n"
"If the user hits EOF (*nix: Ctrl-D, Windows: Ctrl-Z+Return), raise EOFError.\n"
"On *nix systems, readline is used if available.");

#define BUILTIN_INPUT_METHODDEF    \
    {"input", _PyCFunction_CAST(builtin_input), METH_FASTCALL, builtin_input__doc__},

static PyObject *
builtin_input_impl(PyObject *module, PyObject *prompt);

static PyObject *
builtin_input(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *prompt = NULL;

    if (!_PyArg_CheckPositional("input", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    prompt = args[0];
skip_optional:
    return_value = builtin_input_impl(module, prompt);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_repr__doc__,
"repr($module, obj, /)\n"
"--\n"
"\n"
"Return the canonical string representation of the object.\n"
"\n"
"For many object types, including most builtins, eval(repr(obj)) == obj.");

#define BUILTIN_REPR_METHODDEF    \
    {"repr", (PyCFunction)builtin_repr, METH_O, builtin_repr__doc__},

PyDoc_STRVAR(builtin_round__doc__,
"round($module, /, number, ndigits=None)\n"
"--\n"
"\n"
"Round a number to a given precision in decimal digits.\n"
"\n"
"The return value is an integer if ndigits is omitted or None.  Otherwise\n"
"the return value has the same type as the number.  ndigits may be negative.");

#define BUILTIN_ROUND_METHODDEF    \
    {"round", _PyCFunction_CAST(builtin_round), METH_FASTCALL|METH_KEYWORDS, builtin_round__doc__},

static PyObject *
builtin_round_impl(PyObject *module, PyObject *number, PyObject *ndigits);

static PyObject *
builtin_round(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(number), &_Py_ID(ndigits), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"number", "ndigits", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "round",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *number;
    PyObject *ndigits = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    number = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    ndigits = args[1];
skip_optional_pos:
    return_value = builtin_round_impl(module, number, ndigits);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_sum__doc__,
"sum($module, iterable, /, start=0)\n"
"--\n"
"\n"
"Return the sum of a \'start\' value (default: 0) plus an iterable of numbers\n"
"\n"
"When the iterable is empty, return the start value.\n"
"This function is intended specifically for use with numeric values and may\n"
"reject non-numeric types.");

#define BUILTIN_SUM_METHODDEF    \
    {"sum", _PyCFunction_CAST(builtin_sum), METH_FASTCALL|METH_KEYWORDS, builtin_sum__doc__},

static PyObject *
builtin_sum_impl(PyObject *module, PyObject *iterable, PyObject *start);

static PyObject *
builtin_sum(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(start), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "start", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sum",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *iterable;
    PyObject *start = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    iterable = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    start = args[1];
skip_optional_pos:
    return_value = builtin_sum_impl(module, iterable, start);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_isinstance__doc__,
"isinstance($module, obj, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether an object is an instance of a class or of a subclass thereof.\n"
"\n"
"A tuple, as in ``isinstance(x, (A, B, ...))``, may be given as the target to\n"
"check against. This is equivalent to ``isinstance(x, A) or isinstance(x, B)\n"
"or ...`` etc.");

#define BUILTIN_ISINSTANCE_METHODDEF    \
    {"isinstance", _PyCFunction_CAST(builtin_isinstance), METH_FASTCALL, builtin_isinstance__doc__},

static PyObject *
builtin_isinstance_impl(PyObject *module, PyObject *obj,
                        PyObject *class_or_tuple);

static PyObject *
builtin_isinstance(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *class_or_tuple;

    if (!_PyArg_CheckPositional("isinstance", nargs, 2, 2)) {
        goto exit;
    }
    obj = args[0];
    class_or_tuple = args[1];
    return_value = builtin_isinstance_impl(module, obj, class_or_tuple);

exit:
    return return_value;
}

PyDoc_STRVAR(builtin_issubclass__doc__,
"issubclass($module, cls, class_or_tuple, /)\n"
"--\n"
"\n"
"Return whether \'cls\' is derived from another class or is the same class.\n"
"\n"
"A tuple, as in ``issubclass(x, (A, B, ...))``, may be given as the target to\n"
"check against. This is equivalent to ``issubclass(x, A) or issubclass(x, B)\n"
"or ...``.");

#define BUILTIN_ISSUBCLASS_METHODDEF    \
    {"issubclass", _PyCFunction_CAST(builtin_issubclass), METH_FASTCALL, builtin_issubclass__doc__},

static PyObject *
builtin_issubclass_impl(PyObject *module, PyObject *cls,
                        PyObject *class_or_tuple);

static PyObject *
builtin_issubclass(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *cls;
    PyObject *class_or_tuple;

    if (!_PyArg_CheckPositional("issubclass", nargs, 2, 2)) {
        goto exit;
    }
    cls = args[0];
    class_or_tuple = args[1];
    return_value = builtin_issubclass_impl(module, cls, class_or_tuple);

exit:
    return return_value;
}
/*[clinic end generated code: output=e7a5d0851d7f2cfb input=a9049054013a1b77]*/
