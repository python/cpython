/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_SINGLETON()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_ctypes_CType_Type___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return memory consumption of the type object.");

#define _CTYPES_CTYPE_TYPE___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(_ctypes_CType_Type___sizeof__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _ctypes_CType_Type___sizeof____doc__},

static PyObject *
_ctypes_CType_Type___sizeof___impl(PyObject *self, PyTypeObject *cls);

static PyObject *
_ctypes_CType_Type___sizeof__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__sizeof__() takes no arguments");
        return NULL;
    }
    return _ctypes_CType_Type___sizeof___impl(self, cls);
}

PyDoc_STRVAR(CDataType_from_address__doc__,
"from_address($self, value, /)\n"
"--\n"
"\n"
"C.from_address(integer) -> C instance\n"
"\n"
"Access a C instance at the specified address.");

#define CDATATYPE_FROM_ADDRESS_METHODDEF    \
    {"from_address", _PyCFunction_CAST(CDataType_from_address), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_address__doc__},

static PyObject *
CDataType_from_address_impl(PyObject *type, PyTypeObject *cls,
                            PyObject *value);

static PyObject *
CDataType_from_address(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_address",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = CDataType_from_address_impl(type, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(CDataType_from_buffer__doc__,
"from_buffer($self, obj, offset=0, /)\n"
"--\n"
"\n"
"C.from_buffer(object, offset=0) -> C instance\n"
"\n"
"Create a C instance from a writeable buffer.");

#define CDATATYPE_FROM_BUFFER_METHODDEF    \
    {"from_buffer", _PyCFunction_CAST(CDataType_from_buffer), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_buffer__doc__},

static PyObject *
CDataType_from_buffer_impl(PyObject *type, PyTypeObject *cls, PyObject *obj,
                           Py_ssize_t offset);

static PyObject *
CDataType_from_buffer(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_buffer",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *obj;
    Py_ssize_t offset = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    obj = args[0];
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
skip_optional_posonly:
    return_value = CDataType_from_buffer_impl(type, cls, obj, offset);

exit:
    return return_value;
}

PyDoc_STRVAR(CDataType_from_buffer_copy__doc__,
"from_buffer_copy($self, buffer, offset=0, /)\n"
"--\n"
"\n"
"C.from_buffer_copy(object, offset=0) -> C instance\n"
"\n"
"Create a C instance from a readable buffer.");

#define CDATATYPE_FROM_BUFFER_COPY_METHODDEF    \
    {"from_buffer_copy", _PyCFunction_CAST(CDataType_from_buffer_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_buffer_copy__doc__},

static PyObject *
CDataType_from_buffer_copy_impl(PyObject *type, PyTypeObject *cls,
                                Py_buffer *buffer, Py_ssize_t offset);

static PyObject *
CDataType_from_buffer_copy(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_buffer_copy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
skip_optional_posonly:
    return_value = CDataType_from_buffer_copy_impl(type, cls, &buffer, offset);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(CDataType_in_dll__doc__,
"in_dll($self, dll, name, /)\n"
"--\n"
"\n"
"C.in_dll(dll, name) -> C instance\n"
"\n"
"Access a C instance in a dll.");

#define CDATATYPE_IN_DLL_METHODDEF    \
    {"in_dll", _PyCFunction_CAST(CDataType_in_dll), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_in_dll__doc__},

static PyObject *
CDataType_in_dll_impl(PyObject *type, PyTypeObject *cls, PyObject *dll,
                      const char *name);

static PyObject *
CDataType_in_dll(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "in_dll",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *dll;
    const char *name;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    dll = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("in_dll", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(args[1], &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = CDataType_in_dll_impl(type, cls, dll, name);

exit:
    return return_value;
}

PyDoc_STRVAR(CDataType_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n"
"Convert a Python object into a function call parameter.");

#define CDATATYPE_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(CDataType_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, CDataType_from_param__doc__},

static PyObject *
CDataType_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyObject *
CDataType_from_param(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_param",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = CDataType_from_param_impl(type, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(PyCPointerType_set_type__doc__,
"set_type($self, type, /)\n"
"--\n"
"\n");

#define PYCPOINTERTYPE_SET_TYPE_METHODDEF    \
    {"set_type", _PyCFunction_CAST(PyCPointerType_set_type), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyCPointerType_set_type__doc__},

static PyObject *
PyCPointerType_set_type_impl(PyTypeObject *self, PyTypeObject *cls,
                             PyObject *type);

static PyObject *
PyCPointerType_set_type(PyTypeObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_type",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *type;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    type = args[0];
    return_value = PyCPointerType_set_type_impl(self, cls, type);

exit:
    return return_value;
}

PyDoc_STRVAR(PyCPointerType_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n"
"Convert a Python object into a function call parameter.");

#define PYCPOINTERTYPE_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(PyCPointerType_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyCPointerType_from_param__doc__},

static PyObject *
PyCPointerType_from_param_impl(PyObject *type, PyTypeObject *cls,
                               PyObject *value);

static PyObject *
PyCPointerType_from_param(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_param",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = PyCPointerType_from_param_impl(type, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(c_wchar_p_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n");

#define C_WCHAR_P_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(c_wchar_p_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, c_wchar_p_from_param__doc__},

static PyObject *
c_wchar_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyObject *
c_wchar_p_from_param(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_param",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = c_wchar_p_from_param_impl(type, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(c_char_p_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n");

#define C_CHAR_P_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(c_char_p_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, c_char_p_from_param__doc__},

static PyObject *
c_char_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyObject *
c_char_p_from_param(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_param",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = c_char_p_from_param_impl(type, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(c_void_p_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n");

#define C_VOID_P_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(c_void_p_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, c_void_p_from_param__doc__},

static PyObject *
c_void_p_from_param_impl(PyObject *type, PyTypeObject *cls, PyObject *value);

static PyObject *
c_void_p_from_param(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_param",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = c_void_p_from_param_impl(type, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(PyCSimpleType_from_param__doc__,
"from_param($self, value, /)\n"
"--\n"
"\n"
"Convert a Python object into a function call parameter.");

#define PYCSIMPLETYPE_FROM_PARAM_METHODDEF    \
    {"from_param", _PyCFunction_CAST(PyCSimpleType_from_param), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyCSimpleType_from_param__doc__},

static PyObject *
PyCSimpleType_from_param_impl(PyObject *type, PyTypeObject *cls,
                              PyObject *value);

static PyObject *
PyCSimpleType_from_param(PyObject *type, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_param",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = PyCSimpleType_from_param_impl(type, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(PyCData_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define PYCDATA_REDUCE_METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(PyCData_reduce), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, PyCData_reduce__doc__},

static PyObject *
PyCData_reduce_impl(PyObject *myself, PyTypeObject *cls);

static PyObject *
PyCData_reduce(PyObject *myself, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__reduce__() takes no arguments");
        return NULL;
    }
    return PyCData_reduce_impl(myself, cls);
}

PyDoc_STRVAR(Simple_from_outparm__doc__,
"__ctypes_from_outparam__($self, /)\n"
"--\n"
"\n");

#define SIMPLE_FROM_OUTPARM_METHODDEF    \
    {"__ctypes_from_outparam__", _PyCFunction_CAST(Simple_from_outparm), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, Simple_from_outparm__doc__},

static PyObject *
Simple_from_outparm_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
Simple_from_outparm(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__ctypes_from_outparam__() takes no arguments");
        return NULL;
    }
    return Simple_from_outparm_impl(self, cls);
}
/*[clinic end generated code: output=a90886be2a294ee6 input=a9049054013a1b77]*/
