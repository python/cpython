/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_ctypes_get_errno__doc__,
"get_errno($module, /)\n"
"--\n"
"\n"
"Return the current value of the ctypes-private copy of errno.");

#define _CTYPES_GET_ERRNO_METHODDEF    \
    {"get_errno", (PyCFunction)_ctypes_get_errno, METH_NOARGS, _ctypes_get_errno__doc__},

static PyObject *
_ctypes_get_errno_impl(PyObject *module);

static PyObject *
_ctypes_get_errno(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _ctypes_get_errno_impl(module);
}

PyDoc_STRVAR(_ctypes_set_errno__doc__,
"set_errno($module, value, /)\n"
"--\n"
"\n");

#define _CTYPES_SET_ERRNO_METHODDEF    \
    {"set_errno", (PyCFunction)_ctypes_set_errno, METH_O, _ctypes_set_errno__doc__},

static PyObject *
_ctypes_set_errno_impl(PyObject *module, int value);

static PyObject *
_ctypes_set_errno(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int value;

    value = PyLong_AsInt(arg);
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _ctypes_set_errno_impl(module, value);

exit:
    return return_value;
}

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_get_last_error__doc__,
"get_last_error($module, /)\n"
"--\n"
"\n"
"Return the current value of the ctypes-private copy of the last error.");

#define _CTYPES_GET_LAST_ERROR_METHODDEF    \
    {"get_last_error", (PyCFunction)_ctypes_get_last_error, METH_NOARGS, _ctypes_get_last_error__doc__},

static PyObject *
_ctypes_get_last_error_impl(PyObject *module);

static PyObject *
_ctypes_get_last_error(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _ctypes_get_last_error_impl(module);
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_set_last_error__doc__,
"set_last_error($module, value, /)\n"
"--\n"
"\n");

#define _CTYPES_SET_LAST_ERROR_METHODDEF    \
    {"set_last_error", (PyCFunction)_ctypes_set_last_error, METH_O, _ctypes_set_last_error__doc__},

static PyObject *
_ctypes_set_last_error_impl(PyObject *module, int value);

static PyObject *
_ctypes_set_last_error(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int value;

    value = PyLong_AsInt(arg);
    if (value == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _ctypes_set_last_error_impl(module, value);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes__check_HRESULT__doc__,
"_check_HRESULT($module, hresult, /)\n"
"--\n"
"\n");

#define _CTYPES__CHECK_HRESULT_METHODDEF    \
    {"_check_HRESULT", (PyCFunction)_ctypes__check_HRESULT, METH_O, _ctypes__check_HRESULT__doc__},

static PyObject *
_ctypes__check_HRESULT_impl(PyObject *module, int hr);

static PyObject *
_ctypes__check_HRESULT(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int hr;

    hr = PyLong_AsInt(arg);
    if (hr == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _ctypes__check_HRESULT_impl(module, hr);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_FormatError__doc__,
"FormatError($module, code=0, /)\n"
"--\n"
"\n"
"Convert a win32 error code into a string.\n"
"\n"
"If the error code is not given, the return value of a call to\n"
"GetLastError() is used.");

#define _CTYPES_FORMATERROR_METHODDEF    \
    {"FormatError", _PyCFunction_CAST(_ctypes_FormatError), METH_FASTCALL, _ctypes_FormatError__doc__},

static PyObject *
_ctypes_FormatError_impl(PyObject *module, int code);

static PyObject *
_ctypes_FormatError(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int code = 0;

    if (!_PyArg_CheckPositional("FormatError", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    code = PyLong_AsInt(args[0]);
    if (code == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _ctypes_FormatError_impl(module, code);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_LoadLibrary__doc__,
"LoadLibrary($module, name, load_flags=0, /)\n"
"--\n"
"\n"
"Load an executable (usually a DLL), and return a handle to it.\n"
"\n"
"The handle may be used to locate exported functions in this module.\n"
"load_flags are as defined for LoadLibraryEx in the Windows API.");

#define _CTYPES_LOADLIBRARY_METHODDEF    \
    {"LoadLibrary", _PyCFunction_CAST(_ctypes_LoadLibrary), METH_FASTCALL, _ctypes_LoadLibrary__doc__},

static PyObject *
_ctypes_LoadLibrary_impl(PyObject *module, PyObject *nameobj, int load_flags);

static PyObject *
_ctypes_LoadLibrary(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *nameobj;
    int load_flags = 0;

    if (!_PyArg_CheckPositional("LoadLibrary", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("LoadLibrary", "argument 1", "str", args[0]);
        goto exit;
    }
    nameobj = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    load_flags = PyLong_AsInt(args[1]);
    if (load_flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _ctypes_LoadLibrary_impl(module, nameobj, load_flags);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_FreeLibrary__doc__,
"FreeLibrary($module, handle, /)\n"
"--\n"
"\n"
"Free the handle of an executable previously loaded by LoadLibrary.");

#define _CTYPES_FREELIBRARY_METHODDEF    \
    {"FreeLibrary", (PyCFunction)_ctypes_FreeLibrary, METH_O, _ctypes_FreeLibrary__doc__},

static PyObject *
_ctypes_FreeLibrary_impl(PyObject *module, void *hMod);

static PyObject *
_ctypes_FreeLibrary(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    void *hMod;

    if (!_parse_voidp(arg, &hMod)) {
        goto exit;
    }
    return_value = _ctypes_FreeLibrary_impl(module, hMod);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_CopyComPointer__doc__,
"CopyComPointer($module, src, dst, /)\n"
"--\n"
"\n"
"Copy a COM pointer and return the HRESULT value.");

#define _CTYPES_COPYCOMPOINTER_METHODDEF    \
    {"CopyComPointer", _PyCFunction_CAST(_ctypes_CopyComPointer), METH_FASTCALL, _ctypes_CopyComPointer__doc__},

static PyObject *
_ctypes_CopyComPointer_impl(PyObject *module, PyObject *p1, PyObject *p2);

static PyObject *
_ctypes_CopyComPointer(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *p1;
    PyObject *p2;

    if (!_PyArg_CheckPositional("CopyComPointer", nargs, 2, 2)) {
        goto exit;
    }
    p1 = args[0];
    p2 = args[1];
    return_value = _ctypes_CopyComPointer_impl(module, p1, p2);

exit:
    return return_value;
}

#endif /* defined(MS_WIN32) */

#if !defined(MS_WIN32) && defined(__APPLE__)

PyDoc_STRVAR(_ctypes__dyld_shared_cache_contains_path__doc__,
"_dyld_shared_cache_contains_path($module, path, /)\n"
"--\n"
"\n"
"Check whether a path is in the shared cache.");

#define _CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF    \
    {"_dyld_shared_cache_contains_path", (PyCFunction)_ctypes__dyld_shared_cache_contains_path, METH_O, _ctypes__dyld_shared_cache_contains_path__doc__},

#endif /* !defined(MS_WIN32) && defined(__APPLE__) */

#if !defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_dlopen__doc__,
"dlopen($module, name, mode=RTLD_NOW | RTLD_LOCAL, /)\n"
"--\n"
"\n"
"Open a shared library.");

#define _CTYPES_DLOPEN_METHODDEF    \
    {"dlopen", _PyCFunction_CAST(_ctypes_dlopen), METH_FASTCALL, _ctypes_dlopen__doc__},

static PyObject *
_ctypes_dlopen_impl(PyObject *module, PyObject *name, int mode);

static PyObject *
_ctypes_dlopen(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *name;
    int mode = DLOPEN_DEFAULT_MODE;

    if (!_PyArg_CheckPositional("dlopen", nargs, 1, 2)) {
        goto exit;
    }
    name = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    mode = PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _ctypes_dlopen_impl(module, name, mode);

exit:
    return return_value;
}

#endif /* !defined(MS_WIN32) */

#if !defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_dlclose__doc__,
"dlclose($module, handle, /)\n"
"--\n"
"\n"
"Close a shared library.");

#define _CTYPES_DLCLOSE_METHODDEF    \
    {"dlclose", (PyCFunction)_ctypes_dlclose, METH_O, _ctypes_dlclose__doc__},

static PyObject *
_ctypes_dlclose_impl(PyObject *module, void *handle);

static PyObject *
_ctypes_dlclose(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    void *handle;

    if (!_parse_voidp(arg, &handle)) {
        goto exit;
    }
    return_value = _ctypes_dlclose_impl(module, handle);

exit:
    return return_value;
}

#endif /* !defined(MS_WIN32) */

#if !defined(MS_WIN32)

PyDoc_STRVAR(_ctypes_dlsym__doc__,
"dlsym($module, handle, name, /)\n"
"--\n"
"\n"
"Find a symbol in a shared library.");

#define _CTYPES_DLSYM_METHODDEF    \
    {"dlsym", _PyCFunction_CAST(_ctypes_dlsym), METH_FASTCALL, _ctypes_dlsym__doc__},

static PyObject *
_ctypes_dlsym_impl(PyObject *module, void *handle, const char *name);

static PyObject *
_ctypes_dlsym(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    void *handle;
    const char *name;

    if (!_PyArg_CheckPositional("dlsym", nargs, 2, 2)) {
        goto exit;
    }
    if (!_parse_voidp(args[0], &handle)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("dlsym", "argument 2", "str", args[1]);
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
    return_value = _ctypes_dlsym_impl(module, handle, name);

exit:
    return return_value;
}

#endif /* !defined(MS_WIN32) */

#if !defined(MS_WIN32) && (defined(HAVE_DL_ITERATE_PHDR) && !defined(__APPLE__))

PyDoc_STRVAR(_ctypes_dllist__doc__,
"dllist($module, /)\n"
"--\n"
"\n"
"Return a list of loaded shared libraries.");

#define _CTYPES_DLLIST_METHODDEF    \
    {"dllist", (PyCFunction)_ctypes_dllist, METH_NOARGS, _ctypes_dllist__doc__},

static PyObject *
_ctypes_dllist_impl(PyObject *module);

static PyObject *
_ctypes_dllist(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _ctypes_dllist_impl(module);
}

#endif /* !defined(MS_WIN32) && (defined(HAVE_DL_ITERATE_PHDR) && !defined(__APPLE__)) */

PyDoc_STRVAR(_ctypes_call_function__doc__,
"call_function($module, func, arguments, /)\n"
"--\n"
"\n");

#define _CTYPES_CALL_FUNCTION_METHODDEF    \
    {"call_function", _PyCFunction_CAST(_ctypes_call_function), METH_FASTCALL, _ctypes_call_function__doc__},

static PyObject *
_ctypes_call_function_impl(PyObject *module, void *func, PyObject *arguments);

static PyObject *
_ctypes_call_function(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    void *func;
    PyObject *arguments;

    if (!_PyArg_CheckPositional("call_function", nargs, 2, 2)) {
        goto exit;
    }
    if (!_parse_voidp(args[0], &func)) {
        goto exit;
    }
    if (!PyTuple_Check(args[1])) {
        _PyArg_BadArgument("call_function", "argument 2", "tuple", args[1]);
        goto exit;
    }
    arguments = args[1];
    return_value = _ctypes_call_function_impl(module, func, arguments);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_call_cdeclfunction__doc__,
"call_cdeclfunction($module, func, arguments, /)\n"
"--\n"
"\n");

#define _CTYPES_CALL_CDECLFUNCTION_METHODDEF    \
    {"call_cdeclfunction", _PyCFunction_CAST(_ctypes_call_cdeclfunction), METH_FASTCALL, _ctypes_call_cdeclfunction__doc__},

static PyObject *
_ctypes_call_cdeclfunction_impl(PyObject *module, void *func,
                                PyObject *arguments);

static PyObject *
_ctypes_call_cdeclfunction(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    void *func;
    PyObject *arguments;

    if (!_PyArg_CheckPositional("call_cdeclfunction", nargs, 2, 2)) {
        goto exit;
    }
    if (!_parse_voidp(args[0], &func)) {
        goto exit;
    }
    if (!PyTuple_Check(args[1])) {
        _PyArg_BadArgument("call_cdeclfunction", "argument 2", "tuple", args[1]);
        goto exit;
    }
    arguments = args[1];
    return_value = _ctypes_call_cdeclfunction_impl(module, func, arguments);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_sizeof__doc__,
"sizeof($module, obj, /)\n"
"--\n"
"\n"
"Return the size in bytes of a C instance.");

#define _CTYPES_SIZEOF_METHODDEF    \
    {"sizeof", (PyCFunction)_ctypes_sizeof, METH_O, _ctypes_sizeof__doc__},

PyDoc_STRVAR(_ctypes_alignment__doc__,
"alignment($module, obj, /)\n"
"--\n"
"\n"
"Return the alignment requirements of a C instance.\n"
"\n"
"The argument is a C type or a C instance.");

#define _CTYPES_ALIGNMENT_METHODDEF    \
    {"alignment", (PyCFunction)_ctypes_alignment, METH_O, _ctypes_alignment__doc__},

PyDoc_STRVAR(_ctypes_byref__doc__,
"byref($module, obj, offset=0, /)\n"
"--\n"
"\n"
"Return a pointer lookalike to a C instance, only usable as function argument.");

#define _CTYPES_BYREF_METHODDEF    \
    {"byref", _PyCFunction_CAST(_ctypes_byref), METH_FASTCALL, _ctypes_byref__doc__},

static PyObject *
_ctypes_byref_impl(PyObject *module, PyObject *obj, Py_ssize_t offset);

static PyObject *
_ctypes_byref(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    Py_ssize_t offset = 0;

    if (!_PyArg_CheckPositional("byref", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->PyCData_Type)) {
        _PyArg_BadArgument("byref", "argument 1", (clinic_state()->PyCData_Type)->tp_name, args[0]);
        goto exit;
    }
    obj = args[0];
    if (nargs < 2) {
        goto skip_optional;
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
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(obj);
    return_value = _ctypes_byref_impl(module, obj, offset);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_addressof__doc__,
"addressof($module, obj, /)\n"
"--\n"
"\n"
"Return the address of the C instance internal buffer");

#define _CTYPES_ADDRESSOF_METHODDEF    \
    {"addressof", (PyCFunction)_ctypes_addressof, METH_O, _ctypes_addressof__doc__},

static PyObject *
_ctypes_addressof_impl(PyObject *module, PyObject *obj);

static PyObject *
_ctypes_addressof(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *obj;

    if (!PyObject_TypeCheck(arg, clinic_state()->PyCData_Type)) {
        _PyArg_BadArgument("addressof", "argument", (clinic_state()->PyCData_Type)->tp_name, arg);
        goto exit;
    }
    obj = arg;
    Py_BEGIN_CRITICAL_SECTION(obj);
    return_value = _ctypes_addressof_impl(module, obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_PyObj_FromPtr__doc__,
"PyObj_FromPtr($module, address, /)\n"
"--\n"
"\n");

#define _CTYPES_PYOBJ_FROMPTR_METHODDEF    \
    {"PyObj_FromPtr", (PyCFunction)_ctypes_PyObj_FromPtr, METH_O, _ctypes_PyObj_FromPtr__doc__},

static PyObject *
_ctypes_PyObj_FromPtr_impl(PyObject *module, PyObject *ob);

static PyObject *
_ctypes_PyObj_FromPtr(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *ob;

    if (!_parse_voidp_object(arg, &ob)) {
        goto exit;
    }
    return_value = _ctypes_PyObj_FromPtr_impl(module, ob);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_Py_INCREF__doc__,
"Py_INCREF($module, obj, /)\n"
"--\n"
"\n"
"Increment the reference count of the object and return it.");

#define _CTYPES_PY_INCREF_METHODDEF    \
    {"Py_INCREF", (PyCFunction)_ctypes_Py_INCREF, METH_O, _ctypes_Py_INCREF__doc__},

PyDoc_STRVAR(_ctypes_Py_DECREF__doc__,
"Py_DECREF($module, obj, /)\n"
"--\n"
"\n"
"Decrement the reference count of the object and return it.");

#define _CTYPES_PY_DECREF_METHODDEF    \
    {"Py_DECREF", (PyCFunction)_ctypes_Py_DECREF, METH_O, _ctypes_Py_DECREF__doc__},

PyDoc_STRVAR(_ctypes_resize__doc__,
"resize($module, obj, size, /)\n"
"--\n"
"\n");

#define _CTYPES_RESIZE_METHODDEF    \
    {"resize", _PyCFunction_CAST(_ctypes_resize), METH_FASTCALL, _ctypes_resize__doc__},

static PyObject *
_ctypes_resize_impl(PyObject *module, CDataObject *obj, Py_ssize_t size);

static PyObject *
_ctypes_resize(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    CDataObject *obj;
    Py_ssize_t size;

    if (!_PyArg_CheckPositional("resize", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->PyCData_Type)) {
        _PyArg_BadArgument("resize", "argument 1", (clinic_state()->PyCData_Type)->tp_name, args[0]);
        goto exit;
    }
    obj = (CDataObject *)args[0];
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
        size = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(obj);
    return_value = _ctypes_resize_impl(module, obj, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes__unpickle__doc__,
"_unpickle($module, cls, state, /)\n"
"--\n"
"\n");

#define _CTYPES__UNPICKLE_METHODDEF    \
    {"_unpickle", _PyCFunction_CAST(_ctypes__unpickle), METH_FASTCALL, _ctypes__unpickle__doc__},

static PyObject *
_ctypes__unpickle_impl(PyObject *module, PyObject *typ, PyObject *state);

static PyObject *
_ctypes__unpickle(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *typ;
    PyObject *state;

    if (!_PyArg_CheckPositional("_unpickle", nargs, 2, 2)) {
        goto exit;
    }
    typ = args[0];
    if (!PyTuple_Check(args[1])) {
        _PyArg_BadArgument("_unpickle", "argument 2", "tuple", args[1]);
        goto exit;
    }
    state = args[1];
    return_value = _ctypes__unpickle_impl(module, typ, state);

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_buffer_info__doc__,
"buffer_info($module, obj, /)\n"
"--\n"
"\n"
"Return buffer interface information.");

#define _CTYPES_BUFFER_INFO_METHODDEF    \
    {"buffer_info", (PyCFunction)_ctypes_buffer_info, METH_O, _ctypes_buffer_info__doc__},

#ifndef _CTYPES_GET_LAST_ERROR_METHODDEF
    #define _CTYPES_GET_LAST_ERROR_METHODDEF
#endif /* !defined(_CTYPES_GET_LAST_ERROR_METHODDEF) */

#ifndef _CTYPES_SET_LAST_ERROR_METHODDEF
    #define _CTYPES_SET_LAST_ERROR_METHODDEF
#endif /* !defined(_CTYPES_SET_LAST_ERROR_METHODDEF) */

#ifndef _CTYPES__CHECK_HRESULT_METHODDEF
    #define _CTYPES__CHECK_HRESULT_METHODDEF
#endif /* !defined(_CTYPES__CHECK_HRESULT_METHODDEF) */

#ifndef _CTYPES_FORMATERROR_METHODDEF
    #define _CTYPES_FORMATERROR_METHODDEF
#endif /* !defined(_CTYPES_FORMATERROR_METHODDEF) */

#ifndef _CTYPES_LOADLIBRARY_METHODDEF
    #define _CTYPES_LOADLIBRARY_METHODDEF
#endif /* !defined(_CTYPES_LOADLIBRARY_METHODDEF) */

#ifndef _CTYPES_FREELIBRARY_METHODDEF
    #define _CTYPES_FREELIBRARY_METHODDEF
#endif /* !defined(_CTYPES_FREELIBRARY_METHODDEF) */

#ifndef _CTYPES_COPYCOMPOINTER_METHODDEF
    #define _CTYPES_COPYCOMPOINTER_METHODDEF
#endif /* !defined(_CTYPES_COPYCOMPOINTER_METHODDEF) */

#ifndef _CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF
    #define _CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF
#endif /* !defined(_CTYPES__DYLD_SHARED_CACHE_CONTAINS_PATH_METHODDEF) */

#ifndef _CTYPES_DLOPEN_METHODDEF
    #define _CTYPES_DLOPEN_METHODDEF
#endif /* !defined(_CTYPES_DLOPEN_METHODDEF) */

#ifndef _CTYPES_DLCLOSE_METHODDEF
    #define _CTYPES_DLCLOSE_METHODDEF
#endif /* !defined(_CTYPES_DLCLOSE_METHODDEF) */

#ifndef _CTYPES_DLSYM_METHODDEF
    #define _CTYPES_DLSYM_METHODDEF
#endif /* !defined(_CTYPES_DLSYM_METHODDEF) */

#ifndef _CTYPES_DLLIST_METHODDEF
    #define _CTYPES_DLLIST_METHODDEF
#endif /* !defined(_CTYPES_DLLIST_METHODDEF) */
/*[clinic end generated code: output=71a41a6d90e69821 input=a9049054013a1b77]*/
