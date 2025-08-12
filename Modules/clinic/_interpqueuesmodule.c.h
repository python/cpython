/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_interpqueues_create__doc__,
"create($module, /, maxsize, unboundop=-1, fallback=-1)\n"
"--\n"
"\n"
"Create a new cross-interpreter queue and return its unique generated ID.\n"
"\n"
"It is a new reference as though bind() had been called on the queue.\n"
"The caller is responsible for calling destroy() for the new queue\n"
"before the runtime is finalized.");

#define _INTERPQUEUES_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_interpqueues_create), METH_FASTCALL|METH_KEYWORDS, _interpqueues_create__doc__},

static PyObject *
_interpqueues_create_impl(PyObject *module, Py_ssize_t maxsize,
                          int unboundarg, int fallbackarg);

static PyObject *
_interpqueues_create(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(maxsize), &_Py_ID(unboundop), &_Py_ID(fallback), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"maxsize", "unboundop", "fallback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_ssize_t maxsize;
    int unboundarg = -1;
    int fallbackarg = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        maxsize = ival;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        unboundarg = PyLong_AsInt(args[1]);
        if (unboundarg == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    fallbackarg = PyLong_AsInt(args[2]);
    if (fallbackarg == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _interpqueues_create_impl(module, maxsize, unboundarg, fallbackarg);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_destroy__doc__,
"destroy($module, /, qid)\n"
"--\n"
"\n"
"Clear and destroy the queue.\n"
"\n"
"Afterward attempts to use the queue will behave as though it never existed.");

#define _INTERPQUEUES_DESTROY_METHODDEF    \
    {"destroy", _PyCFunction_CAST(_interpqueues_destroy), METH_FASTCALL|METH_KEYWORDS, _interpqueues_destroy__doc__},

static PyObject *
_interpqueues_destroy_impl(PyObject *module, qidarg_converter_data qidarg);

static PyObject *
_interpqueues_destroy(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(qid), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "destroy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    qidarg_converter_data qidarg = {0};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qidarg)) {
        goto exit;
    }
    return_value = _interpqueues_destroy_impl(module, qidarg);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_list_all__doc__,
"list_all($module, /)\n"
"--\n"
"\n"
"Return the list of IDs for all queues.\n"
"\n"
"Each corresponding default unbound op and fallback is also included.");

#define _INTERPQUEUES_LIST_ALL_METHODDEF    \
    {"list_all", (PyCFunction)_interpqueues_list_all, METH_NOARGS, _interpqueues_list_all__doc__},

static PyObject *
_interpqueues_list_all_impl(PyObject *module);

static PyObject *
_interpqueues_list_all(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _interpqueues_list_all_impl(module);
}

PyDoc_STRVAR(_interpqueues_put__doc__,
"put($module, /, qid, obj, unboundop=-1, fallback=-1)\n"
"--\n"
"\n"
"Add the object\'s data to the queue.");

#define _INTERPQUEUES_PUT_METHODDEF    \
    {"put", _PyCFunction_CAST(_interpqueues_put), METH_FASTCALL|METH_KEYWORDS, _interpqueues_put__doc__},

static PyObject *
_interpqueues_put_impl(PyObject *module, qidarg_converter_data qidarg,
                       PyObject *obj, int unboundarg, int fallbackarg);

static PyObject *
_interpqueues_put(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(qid), &_Py_ID(obj), &_Py_ID(unboundop), &_Py_ID(fallback), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"qid", "obj", "unboundop", "fallback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "put",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    qidarg_converter_data qidarg = {0};
    PyObject *obj;
    int unboundarg = -1;
    int fallbackarg = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qidarg)) {
        goto exit;
    }
    obj = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        unboundarg = PyLong_AsInt(args[2]);
        if (unboundarg == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    fallbackarg = PyLong_AsInt(args[3]);
    if (fallbackarg == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _interpqueues_put_impl(module, qidarg, obj, unboundarg, fallbackarg);

exit:
    return return_value;
}
/*[clinic end generated code: output=e56010c88d411c5a input=a9049054013a1b77]*/
