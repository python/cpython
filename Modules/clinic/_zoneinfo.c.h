/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(zoneinfo_ZoneInfo_from_file__doc__,
"from_file($type, file_obj, /, key=None)\n"
"--\n"
"\n"
"Create a ZoneInfo file from a file object.");

#define ZONEINFO_ZONEINFO_FROM_FILE_METHODDEF    \
    {"from_file", _PyCFunction_CAST(zoneinfo_ZoneInfo_from_file), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_from_file__doc__},

static PyObject *
zoneinfo_ZoneInfo_from_file_impl(PyTypeObject *type, PyObject *file_obj,
                                 PyObject *key);

static PyObject *
zoneinfo_ZoneInfo_from_file(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_file",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *file_obj;
    PyObject *key = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file_obj = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    key = args[1];
skip_optional_pos:
    return_value = zoneinfo_ZoneInfo_from_file_impl(type, file_obj, key);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_no_cache__doc__,
"no_cache($type, /, key)\n"
"--\n"
"\n"
"Get a new instance of ZoneInfo, bypassing the cache.");

#define ZONEINFO_ZONEINFO_NO_CACHE_METHODDEF    \
    {"no_cache", _PyCFunction_CAST(zoneinfo_ZoneInfo_no_cache), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_no_cache__doc__},

static PyObject *
zoneinfo_ZoneInfo_no_cache_impl(PyTypeObject *type, PyObject *key);

static PyObject *
zoneinfo_ZoneInfo_no_cache(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(key), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "no_cache",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *key;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = args[0];
    return_value = zoneinfo_ZoneInfo_no_cache_impl(type, key);

exit:
    return return_value;
}

PyDoc_STRVAR(zoneinfo_ZoneInfo_clear_cache__doc__,
"clear_cache($type, /, *, only_keys=None)\n"
"--\n"
"\n"
"Clear the ZoneInfo cache.");

#define ZONEINFO_ZONEINFO_CLEAR_CACHE_METHODDEF    \
    {"clear_cache", _PyCFunction_CAST(zoneinfo_ZoneInfo_clear_cache), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, zoneinfo_ZoneInfo_clear_cache__doc__},

static PyObject *
zoneinfo_ZoneInfo_clear_cache_impl(PyTypeObject *type, PyObject *only_keys);

static PyObject *
zoneinfo_ZoneInfo_clear_cache(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(only_keys), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"only_keys", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "clear_cache",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *only_keys = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 0, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    only_keys = args[0];
skip_optional_kwonly:
    return_value = zoneinfo_ZoneInfo_clear_cache_impl(type, only_keys);

exit:
    return return_value;
}
/*[clinic end generated code: output=d2da73ef66146b83 input=a9049054013a1b77]*/
