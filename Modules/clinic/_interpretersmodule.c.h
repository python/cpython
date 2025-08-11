/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_interpreters_create__doc__,
"create($module, /, config=\'isolated\', *, reqrefs=False)\n"
"--\n"
"\n"
"Create a new interpreter and return a unique generated ID.\n"
"\n"
"The caller is responsible for destroying the interpreter before exiting,\n"
"typically by using _interpreters.destroy().  This can be managed\n"
"automatically by passing \"reqrefs=True\" and then using _incref() and\n"
"_decref() appropriately.\n"
"\n"
"\"config\" must be a valid interpreter config or the name of a\n"
"predefined config (\"isolated\" or \"legacy\").  The default\n"
"is \"isolated\".");

#define _INTERPRETERS_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_interpreters_create), METH_FASTCALL|METH_KEYWORDS, _interpreters_create__doc__},

static PyObject *
_interpreters_create_impl(PyObject *module, PyObject *configobj, int reqrefs);

static PyObject *
_interpreters_create(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(config), &_Py_ID(reqrefs), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"config", "reqrefs", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *configobj = NULL;
    int reqrefs = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        configobj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    reqrefs = PyObject_IsTrue(args[1]);
    if (reqrefs < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_create_impl(module, configobj, reqrefs);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_destroy__doc__,
"destroy($module, /, id, *, restrict=False)\n"
"--\n"
"\n"
"Destroy the identified interpreter.\n"
"\n"
"Attempting to destroy the current interpreter raises InterpreterError.\n"
"So does an unrecognized ID.");

#define _INTERPRETERS_DESTROY_METHODDEF    \
    {"destroy", _PyCFunction_CAST(_interpreters_destroy), METH_FASTCALL|METH_KEYWORDS, _interpreters_destroy__doc__},

static PyObject *
_interpreters_destroy_impl(PyObject *module, PyObject *id, int restricted);

static PyObject *
_interpreters_destroy(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(id), &_Py_ID(restrict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "destroy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *id;
    int restricted = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    restricted = PyObject_IsTrue(args[1]);
    if (restricted < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpreters_destroy_impl(module, id, restricted);

exit:
    return return_value;
}
/*[clinic end generated code: output=02a5b963f63a723f input=a9049054013a1b77]*/
