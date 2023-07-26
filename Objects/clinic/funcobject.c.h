/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(func_new__doc__,
"function(code, globals, name=None, argdefs=None, closure=None)\n"
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
"    a tuple that supplies the bindings for free variables");

static PyObject *
func_new_impl(PyTypeObject *type, PyCodeObject *code, PyObject *globals,
              PyObject *name, PyObject *defaults, PyObject *closure);

static PyObject *
func_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(code), &_Py_ID(globals), &_Py_ID(name), &_Py_ID(argdefs), &_Py_ID(closure), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"code", "globals", "name", "argdefs", "closure", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "function",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 2;
    PyCodeObject *code;
    PyObject *globals;
    PyObject *name = Py_None;
    PyObject *defaults = Py_None;
    PyObject *closure = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 2, 5, 0, argsbuf);
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
    closure = fastargs[4];
skip_optional_pos:
    return_value = func_new_impl(type, code, globals, name, defaults, closure);

exit:
    return return_value;
}
/*[clinic end generated code: output=777cead7b1f6fad3 input=a9049054013a1b77]*/
