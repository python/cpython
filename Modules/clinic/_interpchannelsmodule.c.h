/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_interpchannels_create__doc__,
"create($module, /, unboundop=-1, fallback=-1)\n"
"--\n"
"\n"
"Create a new cross-interpreter channel and return a unique generated ID.");

#define _INTERPCHANNELS_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_interpchannels_create), METH_FASTCALL|METH_KEYWORDS, _interpchannels_create__doc__},

static PyObject *
_interpchannels_create_impl(PyObject *module, int unboundarg,
                            int fallbackarg);

static PyObject *
_interpchannels_create(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(unboundop), &_Py_ID(fallback), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"unboundop", "fallback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int unboundarg = -1;
    int fallbackarg = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        unboundarg = PyLong_AsInt(args[0]);
        if (unboundarg == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    fallbackarg = PyLong_AsInt(args[1]);
    if (fallbackarg == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _interpchannels_create_impl(module, unboundarg, fallbackarg);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_destroy__doc__,
"destroy($module, /, cid)\n"
"--\n"
"\n"
"Close and finalize the channel.\n"
"\n"
"Afterward attempts to use the channel will behave as though it\n"
"never existed.");

#define _INTERPCHANNELS_DESTROY_METHODDEF    \
    {"destroy", _PyCFunction_CAST(_interpchannels_destroy), METH_FASTCALL|METH_KEYWORDS, _interpchannels_destroy__doc__},

static PyObject *
_interpchannels_destroy_impl(PyObject *module, int64_t cid);

static PyObject *
_interpchannels_destroy(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "destroy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t cid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    return_value = _interpchannels_destroy_impl(module, cid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_list_all__doc__,
"list_all($module, /)\n"
"--\n"
"\n"
"Return the list of all IDs for active channels.");

#define _INTERPCHANNELS_LIST_ALL_METHODDEF    \
    {"list_all", (PyCFunction)_interpchannels_list_all, METH_NOARGS, _interpchannels_list_all__doc__},

static PyObject *
_interpchannels_list_all_impl(PyObject *module);

static PyObject *
_interpchannels_list_all(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _interpchannels_list_all_impl(module);
}

PyDoc_STRVAR(_interpchannels_list_interpreters__doc__,
"list_interpreters($module, /, cid, *, send)\n"
"--\n"
"\n"
"Return all interpreter IDs associated with an end of the channel.\n"
"\n"
"The \'send\' argument should be a boolean indicating whether to use\n"
"the send or receive end.");

#define _INTERPCHANNELS_LIST_INTERPRETERS_METHODDEF    \
    {"list_interpreters", _PyCFunction_CAST(_interpchannels_list_interpreters), METH_FASTCALL|METH_KEYWORDS, _interpchannels_list_interpreters__doc__},

static PyObject *
_interpchannels_list_interpreters_impl(PyObject *module, int64_t cid,
                                       int send);

static PyObject *
_interpchannels_list_interpreters(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), &_Py_ID(send), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", "send", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "list_interpreters",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    int64_t cid;
    int send;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    send = PyObject_IsTrue(args[1]);
    if (send < 0) {
        goto exit;
    }
    return_value = _interpchannels_list_interpreters_impl(module, cid, send);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_send__doc__,
"send($module, /, cid, obj, unboundop=-1, fallback=-1, *, blocking=True,\n"
"     timeout=None)\n"
"--\n"
"\n"
"Add the object\'s data to the channel\'s queue.\n"
"\n"
"By default, this waits for the object to be received.");

#define _INTERPCHANNELS_SEND_METHODDEF    \
    {"send", _PyCFunction_CAST(_interpchannels_send), METH_FASTCALL|METH_KEYWORDS, _interpchannels_send__doc__},

static PyObject *
_interpchannels_send_impl(PyObject *module, int64_t cid, PyObject *obj,
                          int unboundarg, int fallbackarg, int blocking,
                          PyObject *timeout_obj);

static PyObject *
_interpchannels_send(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), &_Py_ID(obj), &_Py_ID(unboundop), &_Py_ID(fallback), &_Py_ID(blocking), &_Py_ID(timeout), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", "obj", "unboundop", "fallback", "blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "send",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    int64_t cid;
    PyObject *obj;
    int unboundarg = -1;
    int fallbackarg = -1;
    int blocking = 1;
    PyObject *timeout_obj = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
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
    if (args[3]) {
        fallbackarg = PyLong_AsInt(args[3]);
        if (fallbackarg == -1 && PyErr_Occurred()) {
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
    if (args[4]) {
        blocking = PyObject_IsTrue(args[4]);
        if (blocking < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    timeout_obj = args[5];
skip_optional_kwonly:
    return_value = _interpchannels_send_impl(module, cid, obj, unboundarg, fallbackarg, blocking, timeout_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_send_buffer__doc__,
"send_buffer($module, /, cid, obj, unboundop=-1, fallback=-1, *,\n"
"            blocking=True, timeout=None)\n"
"--\n"
"\n"
"Add the object\'s buffer to the channel\'s queue.\n"
"\n"
"By default, this waits for the object to be received.");

#define _INTERPCHANNELS_SEND_BUFFER_METHODDEF    \
    {"send_buffer", _PyCFunction_CAST(_interpchannels_send_buffer), METH_FASTCALL|METH_KEYWORDS, _interpchannels_send_buffer__doc__},

static PyObject *
_interpchannels_send_buffer_impl(PyObject *module, int64_t cid,
                                 PyObject *obj, int unboundarg,
                                 int fallbackarg, int blocking,
                                 PyObject *timeout_obj);

static PyObject *
_interpchannels_send_buffer(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), &_Py_ID(obj), &_Py_ID(unboundop), &_Py_ID(fallback), &_Py_ID(blocking), &_Py_ID(timeout), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", "obj", "unboundop", "fallback", "blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "send_buffer",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    int64_t cid;
    PyObject *obj;
    int unboundarg = -1;
    int fallbackarg = -1;
    int blocking = 1;
    PyObject *timeout_obj = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
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
    if (args[3]) {
        fallbackarg = PyLong_AsInt(args[3]);
        if (fallbackarg == -1 && PyErr_Occurred()) {
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
    if (args[4]) {
        blocking = PyObject_IsTrue(args[4]);
        if (blocking < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    timeout_obj = args[5];
skip_optional_kwonly:
    return_value = _interpchannels_send_buffer_impl(module, cid, obj, unboundarg, fallbackarg, blocking, timeout_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_recv__doc__,
"recv($module, /, cid, default=<unrepresentable>)\n"
"--\n"
"\n"
"Return a new object pair from the front of the channel\'s queue.\n"
"\n"
"If there is nothing to receive then raise ChannelEmptyError, unless\n"
"a default value is provided, in which case it is returned.\n"
"\n"
"Each object pair consists of (received object, None) or (None, unbound op).");

#define _INTERPCHANNELS_RECV_METHODDEF    \
    {"recv", _PyCFunction_CAST(_interpchannels_recv), METH_FASTCALL|METH_KEYWORDS, _interpchannels_recv__doc__},

static PyObject *
_interpchannels_recv_impl(PyObject *module, int64_t cid,
                          PyObject *default_value);

static PyObject *
_interpchannels_recv(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), &_Py_ID(default), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "recv",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int64_t cid;
    PyObject *default_value = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = args[1];
skip_optional_pos:
    return_value = _interpchannels_recv_impl(module, cid, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_close__doc__,
"close($module, /, cid, *, send=False, recv=False, force=False)\n"
"--\n"
"\n"
"(cid, *, send=None, recv=None, force=False)\n"
"\n"
"Close the channel for all interpreters.\n"
"\n"
"If the channel is empty then the keyword args are ignored and both\n"
"ends are immediately closed.  Otherwise, if \'force\' is True then\n"
"all queued items are released and both ends are immediately\n"
"closed.\n"
"\n"
"If the channel is not empty *and* \'force\' is False then following\n"
"happens:\n"
"\n"
" * recv is True (regardless of send):\n"
"   - raise ChannelNotEmptyError\n"
" * recv is None and send is None:\n"
"   - raise ChannelNotEmptyError\n"
" * send is True and recv is not True:\n"
"   - fully close the \'send\' end\n"
"   - close the \'recv\' end to interpreters not already receiving\n"
"   - fully close it once empty\n"
"\n"
"Closing an already closed channel results in a ChannelClosedError.\n"
"\n"
"Once the channel\'s ID has no more ref counts in any interpreter,\n"
"the channel will be destroyed.");

#define _INTERPCHANNELS_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_interpchannels_close), METH_FASTCALL|METH_KEYWORDS, _interpchannels_close__doc__},

static PyObject *
_interpchannels_close_impl(PyObject *module, int64_t cid, int send, int recv,
                           int force);

static PyObject *
_interpchannels_close(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), &_Py_ID(send), &_Py_ID(recv), &_Py_ID(force), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", "send", "recv", "force", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "close",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int64_t cid;
    int send = 0;
    int recv = 0;
    int force = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        send = PyObject_IsTrue(args[1]);
        if (send < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        recv = PyObject_IsTrue(args[2]);
        if (recv < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    force = PyObject_IsTrue(args[3]);
    if (force < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpchannels_close_impl(module, cid, send, recv, force);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_release__doc__,
"release($module, /, cid, *, send=False, recv=False, force=False)\n"
"--\n"
"\n"
"Close the channel for the current interpreter.\n"
"\n"
"The Boolean \'send\' and \'recv\' parameters may be used to indicate the\n"
"ends to close. By default, both ends are closed. Closing an already\n"
"closed end is a noop.");

#define _INTERPCHANNELS_RELEASE_METHODDEF    \
    {"release", _PyCFunction_CAST(_interpchannels_release), METH_FASTCALL|METH_KEYWORDS, _interpchannels_release__doc__},

static PyObject *
_interpchannels_release_impl(PyObject *module, int64_t cid, int send,
                             int recv, int force);

static PyObject *
_interpchannels_release(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), &_Py_ID(send), &_Py_ID(recv), &_Py_ID(force), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", "send", "recv", "force", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "release",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int64_t cid;
    int send = 0;
    int recv = 0;
    int force = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        send = PyObject_IsTrue(args[1]);
        if (send < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (args[2]) {
        recv = PyObject_IsTrue(args[2]);
        if (recv < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    force = PyObject_IsTrue(args[3]);
    if (force < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _interpchannels_release_impl(module, cid, send, recv, force);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_get_count__doc__,
"get_count($module, /, cid)\n"
"--\n"
"\n"
"Return the number of items in the channel.");

#define _INTERPCHANNELS_GET_COUNT_METHODDEF    \
    {"get_count", _PyCFunction_CAST(_interpchannels_get_count), METH_FASTCALL|METH_KEYWORDS, _interpchannels_get_count__doc__},

static PyObject *
_interpchannels_get_count_impl(PyObject *module, int64_t cid);

static PyObject *
_interpchannels_get_count(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_count",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t cid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    return_value = _interpchannels_get_count_impl(module, cid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_get_info__doc__,
"get_info($module, /, cid)\n"
"--\n"
"\n"
"Return details about the channel.");

#define _INTERPCHANNELS_GET_INFO_METHODDEF    \
    {"get_info", _PyCFunction_CAST(_interpchannels_get_info), METH_FASTCALL|METH_KEYWORDS, _interpchannels_get_info__doc__},

static PyObject *
_interpchannels_get_info_impl(PyObject *module, int64_t cid);

static PyObject *
_interpchannels_get_info(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_info",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t cid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    return_value = _interpchannels_get_info_impl(module, cid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels_get_channel_defaults__doc__,
"get_channel_defaults($module, /, cid)\n"
"--\n"
"\n"
"Return the channel\'s default values, set when it was created.");

#define _INTERPCHANNELS_GET_CHANNEL_DEFAULTS_METHODDEF    \
    {"get_channel_defaults", _PyCFunction_CAST(_interpchannels_get_channel_defaults), METH_FASTCALL|METH_KEYWORDS, _interpchannels_get_channel_defaults__doc__},

static PyObject *
_interpchannels_get_channel_defaults_impl(PyObject *module, int64_t cid);

static PyObject *
_interpchannels_get_channel_defaults(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(cid), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"cid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_channel_defaults",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int64_t cid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!channel_id_arg_converter(module, args[0], &cid)) {
        goto exit;
    }
    return_value = _interpchannels_get_channel_defaults_impl(module, cid);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpchannels__channel_id__doc__,
"_channel_id($module, /, *args, **kwds)\n"
"--\n"
"\n");

#define _INTERPCHANNELS__CHANNEL_ID_METHODDEF    \
    {"_channel_id", _PyCFunction_CAST(_interpchannels__channel_id), METH_VARARGS|METH_KEYWORDS, _interpchannels__channel_id__doc__},

static PyObject *
_interpchannels__channel_id_impl(PyObject *module, PyObject *args,
                                 PyObject *kwds);

static PyObject *
_interpchannels__channel_id(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *__clinic_args = NULL;
    PyObject *__clinic_kwds = NULL;

    __clinic_args = Py_NewRef(args);
    if (kwargs == NULL) {
        __clinic_kwds = PyDict_New();
        if (__clinic_kwds == NULL) {
            goto exit;
        }
    }
    else {
        __clinic_kwds = Py_NewRef(kwargs);
    }
    return_value = _interpchannels__channel_id_impl(module, __clinic_args, __clinic_kwds);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);
    /* Cleanup for kwds */
    Py_XDECREF(__clinic_kwds);

    return return_value;
}

PyDoc_STRVAR(_interpchannels__register_end_types__doc__,
"_register_end_types($module, /, send, recv)\n"
"--\n"
"\n");

#define _INTERPCHANNELS__REGISTER_END_TYPES_METHODDEF    \
    {"_register_end_types", _PyCFunction_CAST(_interpchannels__register_end_types), METH_FASTCALL|METH_KEYWORDS, _interpchannels__register_end_types__doc__},

static PyObject *
_interpchannels__register_end_types_impl(PyObject *module,
                                         PyTypeObject *send,
                                         PyTypeObject *recv);

static PyObject *
_interpchannels__register_end_types(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(send), &_Py_ID(recv), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"send", "recv", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_register_end_types",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyTypeObject *send;
    PyTypeObject *recv;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], &PyType_Type)) {
        _PyArg_BadArgument("_register_end_types", "argument 'send'", (&PyType_Type)->tp_name, args[0]);
        goto exit;
    }
    send = (PyTypeObject *)args[0];
    if (!PyObject_TypeCheck(args[1], &PyType_Type)) {
        _PyArg_BadArgument("_register_end_types", "argument 'recv'", (&PyType_Type)->tp_name, args[1]);
        goto exit;
    }
    recv = (PyTypeObject *)args[1];
    return_value = _interpchannels__register_end_types_impl(module, send, recv);

exit:
    return return_value;
}
/*[clinic end generated code: output=de3f12f7a20c9748 input=a9049054013a1b77]*/
