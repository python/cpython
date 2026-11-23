/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(function___annotate____doc__,
"Get the code object for a function.");

static PyObject *
function___annotate___get_impl(PyFunctionObject *self);

static PyObject *
function___annotate___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotate___get_impl((PyFunctionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
function___annotate___set_impl(PyFunctionObject *self, PyObject *value);

static int
function___annotate___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__annotate__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotate___set_impl((PyFunctionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(function___annotations____doc__,
"Dict of annotations in a function object.");

static PyObject *
function___annotations___get_impl(PyFunctionObject *self);

static PyObject *
function___annotations___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotations___get_impl((PyFunctionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
function___annotations___set_impl(PyFunctionObject *self, PyObject *value);

static int
function___annotations___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value = NULL;

    if (arg != NULL) {
        value = arg;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotations___set_impl((PyFunctionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(function___type_params____doc__,
"Get the declared type parameters for a function.");

static PyObject *
function___type_params___get_impl(PyFunctionObject *self);

static PyObject *
function___type_params___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___type_params___get_impl((PyFunctionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
function___type_params___set_impl(PyFunctionObject *self, PyObject *value);

static int
function___type_params___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__type_params__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    if (!PyTuple_Check(arg)) {
        PyErr_Format(PyExc_TypeError, "attribute '__type_params__' must be tuple, not %T", arg);
        goto exit;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___type_params___set_impl((PyFunctionObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(func_new__doc__,
"function(code, globals, name=None, argdefs=None, closure=None,\n"
"         kwdefaults=None)\n"
"--\n"
"\n"
"Create a function object.\n"
"\n"
"  code\n"
"    a code object\n"
"  globals\n"
"    the globals dictionary\n"
"  name\n"
"    a string that overrides the name from the code object\n"
"  argdefs\n"
"    a tuple that specifies the default argument values\n"
"  closure\n"
"    a tuple that supplies the bindings for free variables\n"
"  kwdefaults\n"
"    a dictionary that specifies the default keyword argument values");

static PyObject *
func_new_impl(PyTypeObject *type, PyCodeObject *code, PyObject *globals,
              PyObject *name, PyObject *defaults, PyObject *closure,
              PyObject *kwdefaults);

static PyObject *
func_new_helper(PyTypeObject *type, PyObject *const *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(code), &_Py_ID(globals), &_Py_ID(name), &_Py_ID(argdefs), &_Py_ID(closure), &_Py_ID(kwdefaults), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"code", "globals", "name", "argdefs", "closure", "kwdefaults", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "function",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    PyObject * const *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 2;
    PyCodeObject *code;
    PyObject *globals;
    PyObject *name = Py_None;
    PyObject *defaults = Py_None;
    PyObject *closure = Py_None;
    PyObject *kwdefaults = Py_None;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyObject_TypeCheck(fastargs[0], &PyCode_Type)) {
        _PyArg_BadArgument("function", "argument 'code'", (&PyCode_Type)->tp_name, fastargs[0]);
        goto exit;
    }
    code = (PyCodeObject *)fastargs[0];
    if (!PyDict_Check(fastargs[1])) {
        _PyArg_BadArgument("function", "argument 'globals'", "dict", fastargs[1]);
        goto exit;
    }
    globals = fastargs[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[2]) {
        name = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[3]) {
        defaults = fastargs[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[4]) {
        closure = fastargs[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    kwdefaults = fastargs[5];
skip_optional_pos:
    return_value = func_new_impl(type, code, globals, name, defaults, closure, kwdefaults);

exit:
    return return_value;
}

static PyObject *
func_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return func_new_helper(type, _PyTuple_CAST(args)->ob_item,
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyObject *
func_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyCodeObject *code;
    PyObject *globals;
    PyObject *name = Py_None;
    PyObject *defaults = Py_None;
    PyObject *closure = Py_None;
    PyObject *kwdefaults = Py_None;

    assert(_PyType_CAST(type)->tp_new == func_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs < 2 || nargs > 6) {
        return func_new_helper(_PyType_CAST(type), args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames);
    }
    if (!PyObject_TypeCheck(args[0], &PyCode_Type)) {
        _PyArg_BadArgument("function", "argument 'code'", (&PyCode_Type)->tp_name, args[0]);
        goto exit;
    }
    code = (PyCodeObject *)args[0];
    if (!PyDict_Check(args[1])) {
        _PyArg_BadArgument("function", "argument 'globals'", "dict", args[1]);
        goto exit;
    }
    globals = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    name = args[2];
    if (nargs < 4) {
        goto skip_optional;
    }
    defaults = args[3];
    if (nargs < 5) {
        goto skip_optional;
    }
    closure = args[4];
    if (nargs < 6) {
        goto skip_optional;
    }
    kwdefaults = args[5];
skip_optional:
    return_value = func_new_impl(_PyType_CAST(type), code, globals, name, defaults, closure, kwdefaults);

exit:
    return return_value;
}
#define FUNCTION___ANNOTATE___GETSETDEF {"__annotate__", (getter)function___annotate___get, (setter)function___annotate___set, function___annotate____doc__},

#define FUNCTION___ANNOTATIONS___GETSETDEF {"__annotations__", (getter)function___annotations___get, (setter)function___annotations___set, function___annotations____doc__},

#define FUNCTION___TYPE_PARAMS___GETSETDEF {"__type_params__", (getter)function___type_params___get, (setter)function___type_params___set, function___type_params____doc__},

/*[clinic end generated code: output=3cd73f64e8bd846e input=a9049054013a1b77]*/
