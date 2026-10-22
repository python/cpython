/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_uuid_uuid4__doc__,
"uuid4($module, /)\n"
"--\n"
"\n"
"Generate a random UUID (version 4).");

#define _UUID_UUID4_METHODDEF    \
    {"uuid4", (PyCFunction)_uuid_uuid4, METH_NOARGS, _uuid_uuid4__doc__},

static PyObject *
_uuid_uuid4_impl(PyObject *module);

static PyObject *
_uuid_uuid4(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _uuid_uuid4_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_uuid_uuid6__doc__,
"uuid6($module, /, node=None, clock_seq=None)\n"
"--\n"
"\n"
"Generate a UUID with reordered UUIDv1 fields for improved DB locality.\n"
"\n"
"More precisely, given a 60-bit timestamp value as specified for UUIDv1,\n"
"for UUIDv6 the first 48 most significant bits are stored first, followed\n"
"by the 4-bit version (same position), followed by the remaining 12 bits\n"
"of the original 60-bit timestamp.");

#define _UUID_UUID6_METHODDEF    \
    {"uuid6", _PyCFunction_CAST(_uuid_uuid6), METH_FASTCALL|METH_KEYWORDS, _uuid_uuid6__doc__},

static PyObject *
_uuid_uuid6_impl(PyObject *module, PyObject *node, PyObject *clock_seq);

static PyObject *
_uuid_uuid6(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(node), &_Py_ID(clock_seq), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"node", "clock_seq", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "uuid6",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *node = Py_None;
    PyObject *clock_seq = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        node = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    clock_seq = args[1];
skip_optional_pos:
    return_value = _uuid_uuid6_impl(module, node, clock_seq);

exit:
    return return_value;
}

PyDoc_STRVAR(_uuid_uuid7__doc__,
"uuid7($module, /)\n"
"--\n"
"\n"
"Generate a UUID from a Unix timestamp in milliseconds and random bits.\n"
"\n"
"UUIDv7 objects feature monotonicity within a millisecond.");

#define _UUID_UUID7_METHODDEF    \
    {"uuid7", (PyCFunction)_uuid_uuid7, METH_NOARGS, _uuid_uuid7__doc__},

static PyObject *
_uuid_uuid7_impl(PyObject *module);

static PyObject *
_uuid_uuid7(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _uuid_uuid7_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_uuid__install_c_hooks__doc__,
"_install_c_hooks($module, /, *, random_func, time_func, node_func=None)\n"
"--\n"
"\n");

#define _UUID__INSTALL_C_HOOKS_METHODDEF    \
    {"_install_c_hooks", _PyCFunction_CAST(_uuid__install_c_hooks), METH_FASTCALL|METH_KEYWORDS, _uuid__install_c_hooks__doc__},

static PyObject *
_uuid__install_c_hooks_impl(PyObject *module, PyObject *random_func,
                            PyObject *time_func, PyObject *node_func);

static PyObject *
_uuid__install_c_hooks(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(random_func), &_Py_ID(time_func), &_Py_ID(node_func), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"random_func", "time_func", "node_func", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_install_c_hooks",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *random_func;
    PyObject *time_func;
    PyObject *node_func = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 2, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    random_func = args[0];
    time_func = args[1];
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    node_func = args[2];
skip_optional_kwonly:
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _uuid__install_c_hooks_impl(module, random_func, time_func, node_func);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_uuid_UUID___init____doc__,
"UUID(hex=None, bytes=None, bytes_le=None, fields=None, int=None,\n"
"     version=None, *, is_safe=<unrepresentable>)\n"
"--\n"
"\n"
"UUID is a fast base implementation type for uuid.UUID.");

static int
_uuid_UUID___init___impl(uuidobject *self, PyObject *hex, PyObject *bytes,
                         PyObject *bytes_le, PyObject *fields,
                         PyObject *int_value, PyObject *version,
                         PyObject *is_safe);

static int
_uuid_UUID___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
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
        .ob_item = { &_Py_ID(hex), &_Py_ID(bytes), &_Py_ID(bytes_le), &_Py_ID(fields), &_Py_ID(int), &_Py_ID(version), &_Py_ID(is_safe), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hex", "bytes", "bytes_le", "fields", "int", "version", "is_safe", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "UUID",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[7];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *hex = Py_None;
    PyObject *bytes = Py_None;
    PyObject *bytes_le = Py_None;
    PyObject *fields = Py_None;
    PyObject *int_value = Py_None;
    PyObject *version = Py_None;
    PyObject *is_safe = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        hex = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        bytes = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        bytes_le = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[3]) {
        fields = fastargs[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[4]) {
        int_value = fastargs[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[5]) {
        version = fastargs[5];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    is_safe = fastargs[6];
skip_optional_kwonly:
    return_value = _uuid_UUID___init___impl((uuidobject *)self, hex, bytes, bytes_le, fields, int_value, version, is_safe);

exit:
    return return_value;
}

PyDoc_STRVAR(_uuid_UUID__from_int__doc__,
"_from_int($type, value, /)\n"
"--\n"
"\n"
"Create a UUID from an integer value. Internal use only.");

#define _UUID_UUID__FROM_INT_METHODDEF    \
    {"_from_int", (PyCFunction)_uuid_UUID__from_int, METH_O|METH_CLASS, _uuid_UUID__from_int__doc__},

static PyObject *
_uuid_UUID__from_int_impl(PyTypeObject *type, PyObject *value);

static PyObject *
_uuid_UUID__from_int(PyObject *type, PyObject *value)
{
    PyObject *return_value = NULL;

    return_value = _uuid_UUID__from_int_impl((PyTypeObject *)type, value);

    return return_value;
}

PyDoc_STRVAR(_uuid_UUID___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n"
"Return the UUID\'s state for pickling.");

#define _UUID_UUID___GETSTATE___METHODDEF    \
    {"__getstate__", (PyCFunction)_uuid_UUID___getstate__, METH_NOARGS, _uuid_UUID___getstate____doc__},

static PyObject *
_uuid_UUID___getstate___impl(uuidobject *self);

static PyObject *
_uuid_UUID___getstate__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _uuid_UUID___getstate___impl((uuidobject *)self);
}

PyDoc_STRVAR(_uuid_UUID___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Restore the UUID\'s state from pickling.\n"
"\n"
"Expects a mapping with \'int\' and optionally \'is_safe\' keys.");

#define _UUID_UUID___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)_uuid_UUID___setstate__, METH_O, _uuid_UUID___setstate____doc__},

static PyObject *
_uuid_UUID___setstate___impl(uuidobject *self, PyObject *state);

static PyObject *
_uuid_UUID___setstate__(PyObject *self, PyObject *state)
{
    PyObject *return_value = NULL;

    return_value = _uuid_UUID___setstate___impl((uuidobject *)self, state);

    return return_value;
}
/*[clinic end generated code: output=e836f3b61a0fec26 input=a9049054013a1b77]*/
