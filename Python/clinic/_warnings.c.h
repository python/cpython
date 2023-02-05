/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(warnings_warn__doc__,
"warn($module, /, message, category=None, stacklevel=1, source=None, *,\n"
"     skip_file_prefixes=<unrepresentable>)\n"
"--\n"
"\n"
"Issue a warning, or maybe ignore it or raise an exception.\n"
"\n"
"  message\n"
"    Text of the warning message.\n"
"  category\n"
"    The Warning category subclass. Defaults to UserWarning.\n"
"  stacklevel\n"
"    How far up the call stack to make this warning appear. A value of 2 for\n"
"    example attributes the warning to the caller of the code calling warn().\n"
"  source\n"
"    If supplied, the destroyed object which emitted a ResourceWarning\n"
"  skip_file_prefixes\n"
"    An optional tuple of module filename prefixes indicating frames to skip\n"
"    during stacklevel computations for stack frame attribution.");

#define WARNINGS_WARN_METHODDEF    \
    {"warn", _PyCFunction_CAST(warnings_warn), METH_FASTCALL|METH_KEYWORDS, warnings_warn__doc__},

static PyObject *
warnings_warn_impl(PyObject *module, PyObject *message, PyObject *category,
                   Py_ssize_t stacklevel, PyObject *source,
                   PyTupleObject *skip_file_prefixes);

static PyObject *
warnings_warn(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(message), &_Py_ID(category), &_Py_ID(stacklevel), &_Py_ID(source), &_Py_ID(skip_file_prefixes), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"message", "category", "stacklevel", "source", "skip_file_prefixes", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "warn",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *message;
    PyObject *category = Py_None;
    Py_ssize_t stacklevel = 1;
    PyObject *source = Py_None;
    PyTupleObject *skip_file_prefixes = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    message = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        category = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            stacklevel = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        source = args[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyTuple_Check(args[4])) {
        _PyArg_BadArgument("warn", "argument 'skip_file_prefixes'", "tuple", args[4]);
        goto exit;
    }
    skip_file_prefixes = (PyTupleObject *)args[4];
skip_optional_kwonly:
    return_value = warnings_warn_impl(module, message, category, stacklevel, source, skip_file_prefixes);

exit:
    return return_value;
}

PyDoc_STRVAR(warnings_warn_explicit__doc__,
"warn_explicit($module, /, message, category, filename, lineno,\n"
"              module=<unrepresentable>, registry=None,\n"
"              module_globals=None, source=None)\n"
"--\n"
"\n"
"Issue a warning, or maybe ignore it or raise an exception.");

#define WARNINGS_WARN_EXPLICIT_METHODDEF    \
    {"warn_explicit", _PyCFunction_CAST(warnings_warn_explicit), METH_FASTCALL|METH_KEYWORDS, warnings_warn_explicit__doc__},

static PyObject *
warnings_warn_explicit_impl(PyObject *module, PyObject *message,
                            PyObject *category, PyObject *filename,
                            int lineno, PyObject *mod, PyObject *registry,
                            PyObject *module_globals, PyObject *sourceobj);

static PyObject *
warnings_warn_explicit(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 8
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(message), &_Py_ID(category), &_Py_ID(filename), &_Py_ID(lineno), &_Py_ID(module), &_Py_ID(registry), &_Py_ID(module_globals), &_Py_ID(source), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"message", "category", "filename", "lineno", "module", "registry", "module_globals", "source", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "warn_explicit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[8];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 4;
    PyObject *message;
    PyObject *category;
    PyObject *filename;
    int lineno;
    PyObject *mod = NULL;
    PyObject *registry = Py_None;
    PyObject *module_globals = Py_None;
    PyObject *sourceobj = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 4, 8, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    message = args[0];
    category = args[1];
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("warn_explicit", "argument 'filename'", "str", args[2]);
        goto exit;
    }
    if (PyUnicode_READY(args[2]) == -1) {
        goto exit;
    }
    filename = args[2];
    lineno = _PyLong_AsInt(args[3]);
    if (lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[4]) {
        mod = args[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[5]) {
        registry = args[5];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[6]) {
        module_globals = args[6];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    sourceobj = args[7];
skip_optional_pos:
    return_value = warnings_warn_explicit_impl(module, message, category, filename, lineno, mod, registry, module_globals, sourceobj);

exit:
    return return_value;
}

PyDoc_STRVAR(warnings_filters_mutated__doc__,
"_filters_mutated($module, /)\n"
"--\n"
"\n");

#define WARNINGS_FILTERS_MUTATED_METHODDEF    \
    {"_filters_mutated", (PyCFunction)warnings_filters_mutated, METH_NOARGS, warnings_filters_mutated__doc__},

static PyObject *
warnings_filters_mutated_impl(PyObject *module);

static PyObject *
warnings_filters_mutated(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return warnings_filters_mutated_impl(module);
}
/*[clinic end generated code: output=20429719d7223bdc input=a9049054013a1b77]*/
