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
_interpqueues_destroy_impl(PyObject *module, int64_t qid);

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
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_destroy_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_list_all__doc__,
"list_all($module, /)\n"
"--\n"
"\n"
"Return the list of ID triples for all queues.\n"
"\n"
"Each ID triple consists of (ID, default unbound op, default fallback).");

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
_interpqueues_put_impl(PyObject *module, int64_t qid, PyObject *obj,
                       int unboundarg, int fallbackarg);

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
    int64_t qid;
    PyObject *obj;
    int unboundarg = -1;
    int fallbackarg = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
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
    return_value = _interpqueues_put_impl(module, qid, obj, unboundarg, fallbackarg);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get__doc__,
"get($module, /, qid)\n"
"--\n"
"\n"
"Return the (object, unbound op) from the front of the queue.\n"
"\n"
"If there is nothing to receive then raise QueueEmpty.");

#define _INTERPQUEUES_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_interpqueues_get), METH_FASTCALL|METH_KEYWORDS, _interpqueues_get__doc__},

static PyObject *
_interpqueues_get_impl(PyObject *module, int64_t qid);

static PyObject *
_interpqueues_get(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "get",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_bind__doc__,
"bind($module, /, qid)\n"
"--\n"
"\n"
"Take a reference to the identified queue.\n"
"\n"
"The queue is not destroyed until there are no references left.");

#define _INTERPQUEUES_BIND_METHODDEF    \
    {"bind", _PyCFunction_CAST(_interpqueues_bind), METH_FASTCALL|METH_KEYWORDS, _interpqueues_bind__doc__},

static PyObject *
_interpqueues_bind_impl(PyObject *module, int64_t qid);

static PyObject *
_interpqueues_bind(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "bind",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_bind_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_release__doc__,
"release($module, /, qid)\n"
"--\n"
"\n"
"Release a reference to the queue.\n"
"\n"
"The queue is destroyed once there are no references left.");

#define _INTERPQUEUES_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(_interpqueues_release), METH_FASTCALL|METH_KEYWORDS, _interpqueues_release__doc__},

static PyObject *
_interpqueues_release_impl(PyObject *module, int64_t qid);

static PyObject *
_interpqueues_release(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "release",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_release_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get_maxsize__doc__,
"get_maxsize($module, /, qid)\n"
"--\n"
"\n"
"Return the maximum number of items in the queue.");

#define _INTERPQUEUES_GET_MAXSIZE_METHODDEF    \
    {"get_maxsize", _PyCFunction_CAST(_interpqueues_get_maxsize), METH_FASTCALL|METH_KEYWORDS, _interpqueues_get_maxsize__doc__},

static PyObject *
_interpqueues_get_maxsize_impl(PyObject *module, int64_t qid);

static PyObject *
_interpqueues_get_maxsize(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "get_maxsize",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_maxsize_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get_queue_defaults__doc__,
"get_queue_defaults($module, /, qid)\n"
"--\n"
"\n"
"Return the queue\'s default values, set when it was created.");

#define _INTERPQUEUES_GET_QUEUE_DEFAULTS_METHODDEF    \
    {"get_queue_defaults", _PyCFunction_CAST(_interpqueues_get_queue_defaults), METH_FASTCALL|METH_KEYWORDS, _interpqueues_get_queue_defaults__doc__},

static PyObject *
_interpqueues_get_queue_defaults_impl(PyObject *module, int64_t qid);

static PyObject *
_interpqueues_get_queue_defaults(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "get_queue_defaults",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_queue_defaults_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_is_full__doc__,
"is_full($module, /, qid)\n"
"--\n"
"\n"
"Return true if the queue has a maxsize and has reached it.");

#define _INTERPQUEUES_IS_FULL_METHODDEF    \
    {"is_full", _PyCFunction_CAST(_interpqueues_is_full), METH_FASTCALL|METH_KEYWORDS, _interpqueues_is_full__doc__},

static PyObject *
_interpqueues_is_full_impl(PyObject *module, int64_t qid);

static PyObject *
_interpqueues_is_full(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "is_full",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_is_full_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues_get_count__doc__,
"get_count($module, /, qid)\n"
"--\n"
"\n"
"Return the number of items in the queue.");

#define _INTERPQUEUES_GET_COUNT_METHODDEF    \
    {"get_count", _PyCFunction_CAST(_interpqueues_get_count), METH_FASTCALL|METH_KEYWORDS, _interpqueues_get_count__doc__},

static PyObject *
_interpqueues_get_count_impl(PyObject *module, int64_t qid);

static PyObject *
_interpqueues_get_count(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "get_count",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t qid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!qidarg_converter(args[0], &qid)) {
        goto exit;
    }
    return_value = _interpqueues_get_count_impl(module, qid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpqueues__register_heap_types__doc__,
"_register_heap_types($module, /, queuetype, emptyerror, fullerror)\n"
"--\n"
"\n"
"Return the number of items in the queue.");

#define _INTERPQUEUES__REGISTER_HEAP_TYPES_METHODDEF    \
    {"_register_heap_types", _PyCFunction_CAST(_interpqueues__register_heap_types), METH_FASTCALL|METH_KEYWORDS, _interpqueues__register_heap_types__doc__},

static PyObject *
_interpqueues__register_heap_types_impl(PyObject *module,
                                        PyTypeObject *queuetype,
                                        PyObject *emptyerror,
                                        PyObject *fullerror);

static PyObject *
_interpqueues__register_heap_types(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(queuetype), &_Py_ID(emptyerror), &_Py_ID(fullerror), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"queuetype", "emptyerror", "fullerror", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_register_heap_types",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyTypeObject *queuetype;
    PyObject *emptyerror;
    PyObject *fullerror;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], &PyType_Type)) {
        _PyArg_BadArgument("_register_heap_types", "argument 'queuetype'", (&PyType_Type)->tp_name, args[0]);
        goto exit;
    }
    queuetype = (PyTypeObject *)args[0];
    emptyerror = args[1];
    fullerror = args[2];
    return_value = _interpqueues__register_heap_types_impl(module, queuetype, emptyerror, fullerror);

exit:
    return return_value;
}
/*[clinic end generated code: output=64cea8e1063429b6 input=a9049054013a1b77]*/
