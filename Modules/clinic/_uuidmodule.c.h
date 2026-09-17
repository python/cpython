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
"_install_c_hooks($module, /, *, random_func, time_func)\n"
"--\n"
"\n");

#define _UUID__INSTALL_C_HOOKS_METHODDEF    \
    {"_install_c_hooks", _PyCFunction_CAST(_uuid__install_c_hooks), METH_FASTCALL|METH_KEYWORDS, _uuid__install_c_hooks__doc__},

static PyObject *
_uuid__install_c_hooks_impl(PyObject *module, PyObject *random_func,
                            PyObject *time_func);

static PyObject *
_uuid__install_c_hooks(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(random_func), &_Py_ID(time_func), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"random_func", "time_func", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_install_c_hooks",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *random_func;
    PyObject *time_func;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 2, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    random_func = args[0];
    time_func = args[1];
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _uuid__install_c_hooks_impl(module, random_func, time_func);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_uuid_UUID___init____doc__,
"UUID(hex=<unrepresentable>, bytes=None, bytes_le=None,\n"
"     fields=<unrepresentable>, int=<unrepresentable>,\n"
"     version=<unrepresentable>, *, is_safe=<unrepresentable>)\n"
"--\n"
"\n"
"UUID is a fast base implementation type for uuid.UUID.");

static int
_uuid_UUID___init___impl(uuidobject *self, PyObject *hex, Py_buffer *bytes,
                         Py_buffer *bytes_le, PyObject *fields,
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
    PyObject *hex = NULL;
    Py_buffer bytes = {NULL, NULL};
    Py_buffer bytes_le = {NULL, NULL};
    PyObject *fields = NULL;
    PyObject *int_value = NULL;
    PyObject *version = NULL;
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
        if (!PyUnicode_Check(fastargs[0])) {
            _PyArg_BadArgument("UUID", "argument 'hex'", "str", fastargs[0]);
            goto exit;
        }
        hex = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        if (PyObject_GetBuffer(fastargs[1], &bytes, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        if (PyObject_GetBuffer(fastargs[2], &bytes_le, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
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
    return_value = _uuid_UUID___init___impl((uuidobject *)self, hex, &bytes, &bytes_le, fields, int_value, version, is_safe);

exit:
    /* Cleanup for bytes */
    if (bytes.obj) {
       PyBuffer_Release(&bytes);
    }
    /* Cleanup for bytes_le */
    if (bytes_le.obj) {
       PyBuffer_Release(&bytes_le);
    }

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
"Expects a dictionary with \'int\' and optionally \'is_safe\' keys.");

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
/*[clinic end generated code: output=095610812af4b3bd input=a9049054013a1b77]*/
