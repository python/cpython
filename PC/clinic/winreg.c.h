/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_UnsignedLong_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_HKEYType_Close__doc__,
"Close($self, /)\n"
"--\n"
"\n"
"Closes the underlying Windows handle.\n"
"\n"
"If the handle is already closed, no error is raised.");

#define WINREG_HKEYTYPE_CLOSE_METHODDEF    \
    {"Close", (PyCFunction)winreg_HKEYType_Close, METH_NOARGS, winreg_HKEYType_Close__doc__},

static PyObject *
winreg_HKEYType_Close_impl(PyHKEYObject *self);

static PyObject *
winreg_HKEYType_Close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return winreg_HKEYType_Close_impl((PyHKEYObject *)self);
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_HKEYType_Detach__doc__,
"Detach($self, /)\n"
"--\n"
"\n"
"Detaches the Windows handle from the handle object.\n"
"\n"
"The result is the value of the handle before it is detached.  If the\n"
"handle is already detached, this will return zero.\n"
"\n"
"After calling this function, the handle is effectively invalidated,\n"
"but the handle is not closed.  You would call this function when you\n"
"need the underlying win32 handle to exist beyond the lifetime of the\n"
"handle object.");

#define WINREG_HKEYTYPE_DETACH_METHODDEF    \
    {"Detach", (PyCFunction)winreg_HKEYType_Detach, METH_NOARGS, winreg_HKEYType_Detach__doc__},

static PyObject *
winreg_HKEYType_Detach_impl(PyHKEYObject *self);

static PyObject *
winreg_HKEYType_Detach(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return winreg_HKEYType_Detach_impl((PyHKEYObject *)self);
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_HKEYType___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n");

#define WINREG_HKEYTYPE___ENTER___METHODDEF    \
    {"__enter__", (PyCFunction)winreg_HKEYType___enter__, METH_NOARGS, winreg_HKEYType___enter____doc__},

static PyHKEYObject *
winreg_HKEYType___enter___impl(PyHKEYObject *self);

static PyObject *
winreg_HKEYType___enter__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    PyHKEYObject *_return_value;

    _return_value = winreg_HKEYType___enter___impl((PyHKEYObject *)self);
    return_value = (PyObject *)_return_value;

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_HKEYType___exit____doc__,
"__exit__($self, exc_type, exc_value, traceback, /)\n"
"--\n"
"\n");

#define WINREG_HKEYTYPE___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(winreg_HKEYType___exit__), METH_FASTCALL, winreg_HKEYType___exit____doc__},

static PyObject *
winreg_HKEYType___exit___impl(PyHKEYObject *self, PyObject *exc_type,
                              PyObject *exc_value, PyObject *traceback);

static PyObject *
winreg_HKEYType___exit__(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *traceback;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = args[0];
    exc_value = args[1];
    traceback = args[2];
    return_value = winreg_HKEYType___exit___impl((PyHKEYObject *)self, exc_type, exc_value, traceback);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_CloseKey__doc__,
"CloseKey($module, hkey, /)\n"
"--\n"
"\n"
"Closes a previously opened registry key.\n"
"\n"
"  hkey\n"
"    A previously opened key.\n"
"\n"
"Note that if the key is not closed using this method, it will be\n"
"closed when the hkey object is destroyed by Python.");

#define WINREG_CLOSEKEY_METHODDEF    \
    {"CloseKey", (PyCFunction)winreg_CloseKey, METH_O, winreg_CloseKey__doc__},

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(winreg_ConnectRegistry__doc__,
"ConnectRegistry($module, computer_name, key, /)\n"
"--\n"
"\n"
"Establishes a connection to the registry on another computer.\n"
"\n"
"  computer_name\n"
"    The name of the remote computer, of the form r\"\\\\computername\".  If\n"
"    None, the local computer is used.\n"
"  key\n"
"    The predefined key to connect to.\n"
"\n"
"The return value is the handle of the opened key.\n"
"If the function fails, an OSError exception is raised.");

#define WINREG_CONNECTREGISTRY_METHODDEF    \
    {"ConnectRegistry", _PyCFunction_CAST(winreg_ConnectRegistry), METH_FASTCALL, winreg_ConnectRegistry__doc__},

static HKEY
winreg_ConnectRegistry_impl(PyObject *module, const wchar_t *computer_name,
                            HKEY key);

static PyObject *
winreg_ConnectRegistry(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const wchar_t *computer_name = NULL;
    HKEY key;
    HKEY _return_value;

    if (!_PyArg_CheckPositional("ConnectRegistry", nargs, 2, 2)) {
        goto exit;
    }
    if (args[0] == Py_None) {
        computer_name = NULL;
    }
    else if (PyUnicode_Check(args[0])) {
        computer_name = PyUnicode_AsWideCharString(args[0], NULL);
        if (computer_name == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("ConnectRegistry", "argument 1", "str or None", args[0]);
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[1], &key)) {
        goto exit;
    }
    _return_value = winreg_ConnectRegistry_impl(module, computer_name, key);
    if (_return_value == NULL) {
        goto exit;
    }
    return_value = PyHKEY_FromHKEY(_PyModule_GetState(module), _return_value);

exit:
    /* Cleanup for computer_name */
    PyMem_Free((void *)computer_name);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_CreateKey__doc__,
"CreateKey($module, key, sub_key, /)\n"
"--\n"
"\n"
"Creates or opens the specified key.\n"
"\n"
"  key\n"
"    An already open key, or one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    The name of the key this method opens or creates.\n"
"\n"
"If key is one of the predefined keys, sub_key may be None. In that case,\n"
"the handle returned is the same key handle passed in to the function.\n"
"\n"
"If the key already exists, this function opens the existing key.\n"
"\n"
"The return value is the handle of the opened key.\n"
"If the function fails, an OSError exception is raised.");

#define WINREG_CREATEKEY_METHODDEF    \
    {"CreateKey", _PyCFunction_CAST(winreg_CreateKey), METH_FASTCALL, winreg_CreateKey__doc__},

static HKEY
winreg_CreateKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key);

static PyObject *
winreg_CreateKey(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *sub_key = NULL;
    HKEY _return_value;

    if (!_PyArg_CheckPositional("CreateKey", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        sub_key = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        sub_key = PyUnicode_AsWideCharString(args[1], NULL);
        if (sub_key == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("CreateKey", "argument 2", "str or None", args[1]);
        goto exit;
    }
    _return_value = winreg_CreateKey_impl(module, key, sub_key);
    if (_return_value == NULL) {
        goto exit;
    }
    return_value = PyHKEY_FromHKEY(_PyModule_GetState(module), _return_value);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_CreateKeyEx__doc__,
"CreateKeyEx($module, /, key, sub_key, reserved=0,\n"
"            access=winreg.KEY_WRITE)\n"
"--\n"
"\n"
"Creates or opens the specified key.\n"
"\n"
"  key\n"
"    An already open key, or one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    The name of the key this method opens or creates.\n"
"  reserved\n"
"    A reserved integer, and must be zero.  Default is zero.\n"
"  access\n"
"    An integer that specifies an access mask that describes the\n"
"    desired security access for the key. Default is KEY_WRITE.\n"
"\n"
"If key is one of the predefined keys, sub_key may be None. In that case,\n"
"the handle returned is the same key handle passed in to the function.\n"
"\n"
"If the key already exists, this function opens the existing key\n"
"\n"
"The return value is the handle of the opened key.\n"
"If the function fails, an OSError exception is raised.");

#define WINREG_CREATEKEYEX_METHODDEF    \
    {"CreateKeyEx", _PyCFunction_CAST(winreg_CreateKeyEx), METH_FASTCALL|METH_KEYWORDS, winreg_CreateKeyEx__doc__},

static HKEY
winreg_CreateKeyEx_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                        int reserved, REGSAM access);

static PyObject *
winreg_CreateKeyEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(key), &_Py_ID(sub_key), &_Py_ID(reserved), &_Py_ID(access), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "sub_key", "reserved", "access", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "CreateKeyEx",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    HKEY key;
    const wchar_t *sub_key = NULL;
    int reserved = 0;
    REGSAM access = KEY_WRITE;
    HKEY _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        sub_key = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        sub_key = PyUnicode_AsWideCharString(args[1], NULL);
        if (sub_key == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("CreateKeyEx", "argument 'sub_key'", "str or None", args[1]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        reserved = PyLong_AsInt(args[2]);
        if (reserved == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    access = PyLong_AsInt(args[3]);
    if (access == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = winreg_CreateKeyEx_impl(module, key, sub_key, reserved, access);
    if (_return_value == NULL) {
        goto exit;
    }
    return_value = PyHKEY_FromHKEY(_PyModule_GetState(module), _return_value);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_DeleteKey__doc__,
"DeleteKey($module, key, sub_key, /)\n"
"--\n"
"\n"
"Deletes the specified key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    A string that must be the name of a subkey of the key identified by\n"
"    the key parameter. This value must not be None, and the key may not\n"
"    have subkeys.\n"
"\n"
"This method can not delete keys with subkeys.\n"
"\n"
"If the function succeeds, the entire key, including all of its values,\n"
"is removed.  If the function fails, an OSError exception is raised.");

#define WINREG_DELETEKEY_METHODDEF    \
    {"DeleteKey", _PyCFunction_CAST(winreg_DeleteKey), METH_FASTCALL, winreg_DeleteKey__doc__},

static PyObject *
winreg_DeleteKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key);

static PyObject *
winreg_DeleteKey(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *sub_key = NULL;

    if (!_PyArg_CheckPositional("DeleteKey", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("DeleteKey", "argument 2", "str", args[1]);
        goto exit;
    }
    sub_key = PyUnicode_AsWideCharString(args[1], NULL);
    if (sub_key == NULL) {
        goto exit;
    }
    return_value = winreg_DeleteKey_impl(module, key, sub_key);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_DeleteKeyEx__doc__,
"DeleteKeyEx($module, /, key, sub_key, access=winreg.KEY_WOW64_64KEY,\n"
"            reserved=0)\n"
"--\n"
"\n"
"Deletes the specified key (intended for 64-bit OS).\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    A string that must be the name of a subkey of the key identified by\n"
"    the key parameter. This value must not be None, and the key may not\n"
"    have subkeys.\n"
"  access\n"
"    An integer that specifies an access mask that describes the\n"
"    desired security access for the key. Default is KEY_WOW64_64KEY.\n"
"  reserved\n"
"    A reserved integer, and must be zero.  Default is zero.\n"
"\n"
"While this function is intended to be used for 64-bit OS, it is also\n"
" available on 32-bit systems.\n"
"\n"
"This method can not delete keys with subkeys.\n"
"\n"
"If the function succeeds, the entire key, including all of its values,\n"
"is removed.  If the function fails, an OSError exception is raised.\n"
"On unsupported Windows versions, NotImplementedError is raised.");

#define WINREG_DELETEKEYEX_METHODDEF    \
    {"DeleteKeyEx", _PyCFunction_CAST(winreg_DeleteKeyEx), METH_FASTCALL|METH_KEYWORDS, winreg_DeleteKeyEx__doc__},

static PyObject *
winreg_DeleteKeyEx_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                        REGSAM access, int reserved);

static PyObject *
winreg_DeleteKeyEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(key), &_Py_ID(sub_key), &_Py_ID(access), &_Py_ID(reserved), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "sub_key", "access", "reserved", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "DeleteKeyEx",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    HKEY key;
    const wchar_t *sub_key = NULL;
    REGSAM access = KEY_WOW64_64KEY;
    int reserved = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("DeleteKeyEx", "argument 'sub_key'", "str", args[1]);
        goto exit;
    }
    sub_key = PyUnicode_AsWideCharString(args[1], NULL);
    if (sub_key == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        access = PyLong_AsInt(args[2]);
        if (access == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    reserved = PyLong_AsInt(args[3]);
    if (reserved == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = winreg_DeleteKeyEx_impl(module, key, sub_key, access, reserved);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_DeleteValue__doc__,
"DeleteValue($module, key, value, /)\n"
"--\n"
"\n"
"Removes a named value from a registry key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  value\n"
"    A string that identifies the value to remove.");

#define WINREG_DELETEVALUE_METHODDEF    \
    {"DeleteValue", _PyCFunction_CAST(winreg_DeleteValue), METH_FASTCALL, winreg_DeleteValue__doc__},

static PyObject *
winreg_DeleteValue_impl(PyObject *module, HKEY key, const wchar_t *value);

static PyObject *
winreg_DeleteValue(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *value = NULL;

    if (!_PyArg_CheckPositional("DeleteValue", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        value = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        value = PyUnicode_AsWideCharString(args[1], NULL);
        if (value == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("DeleteValue", "argument 2", "str or None", args[1]);
        goto exit;
    }
    return_value = winreg_DeleteValue_impl(module, key, value);

exit:
    /* Cleanup for value */
    PyMem_Free((void *)value);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_EnumKey__doc__,
"EnumKey($module, key, index, /)\n"
"--\n"
"\n"
"Enumerates subkeys of an open registry key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  index\n"
"    An integer that identifies the index of the key to retrieve.\n"
"\n"
"The function retrieves the name of one subkey each time it is called.\n"
"It is typically called repeatedly until an OSError exception is\n"
"raised, indicating no more values are available.");

#define WINREG_ENUMKEY_METHODDEF    \
    {"EnumKey", _PyCFunction_CAST(winreg_EnumKey), METH_FASTCALL, winreg_EnumKey__doc__},

static PyObject *
winreg_EnumKey_impl(PyObject *module, HKEY key, int index);

static PyObject *
winreg_EnumKey(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    int index;

    if (!_PyArg_CheckPositional("EnumKey", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    index = PyLong_AsInt(args[1]);
    if (index == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = winreg_EnumKey_impl(module, key, index);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_EnumValue__doc__,
"EnumValue($module, key, index, /)\n"
"--\n"
"\n"
"Enumerates values of an open registry key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  index\n"
"    An integer that identifies the index of the value to retrieve.\n"
"\n"
"The function retrieves the name of one subkey each time it is called.\n"
"It is typically called repeatedly, until an OSError exception\n"
"is raised, indicating no more values.\n"
"\n"
"The result is a tuple of 3 items:\n"
"  value_name\n"
"    A string that identifies the value.\n"
"  value_data\n"
"    An object that holds the value data, and whose type depends\n"
"    on the underlying registry type.\n"
"  data_type\n"
"    An integer that identifies the type of the value data.");

#define WINREG_ENUMVALUE_METHODDEF    \
    {"EnumValue", _PyCFunction_CAST(winreg_EnumValue), METH_FASTCALL, winreg_EnumValue__doc__},

static PyObject *
winreg_EnumValue_impl(PyObject *module, HKEY key, int index);

static PyObject *
winreg_EnumValue(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    int index;

    if (!_PyArg_CheckPositional("EnumValue", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    index = PyLong_AsInt(args[1]);
    if (index == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = winreg_EnumValue_impl(module, key, index);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_ExpandEnvironmentStrings__doc__,
"ExpandEnvironmentStrings($module, string, /)\n"
"--\n"
"\n"
"Expand environment vars.");

#define WINREG_EXPANDENVIRONMENTSTRINGS_METHODDEF    \
    {"ExpandEnvironmentStrings", (PyCFunction)winreg_ExpandEnvironmentStrings, METH_O, winreg_ExpandEnvironmentStrings__doc__},

static PyObject *
winreg_ExpandEnvironmentStrings_impl(PyObject *module, const wchar_t *string);

static PyObject *
winreg_ExpandEnvironmentStrings(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const wchar_t *string = NULL;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("ExpandEnvironmentStrings", "argument", "str", arg);
        goto exit;
    }
    string = PyUnicode_AsWideCharString(arg, NULL);
    if (string == NULL) {
        goto exit;
    }
    return_value = winreg_ExpandEnvironmentStrings_impl(module, string);

exit:
    /* Cleanup for string */
    PyMem_Free((void *)string);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(winreg_FlushKey__doc__,
"FlushKey($module, key, /)\n"
"--\n"
"\n"
"Writes all the attributes of a key to the registry.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"\n"
"It is not necessary to call FlushKey to change a key.  Registry changes\n"
"are flushed to disk by the registry using its lazy flusher.  Registry\n"
"changes are also flushed to disk at system shutdown.  Unlike\n"
"CloseKey(), the FlushKey() method returns only when all the data has\n"
"been written to the registry.\n"
"\n"
"An application should only call FlushKey() if it requires absolute\n"
"certainty that registry changes are on disk.  If you don\'t know whether\n"
"a FlushKey() call is required, it probably isn\'t.");

#define WINREG_FLUSHKEY_METHODDEF    \
    {"FlushKey", (PyCFunction)winreg_FlushKey, METH_O, winreg_FlushKey__doc__},

static PyObject *
winreg_FlushKey_impl(PyObject *module, HKEY key);

static PyObject *
winreg_FlushKey(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HKEY key;

    if (!clinic_HKEY_converter(_PyModule_GetState(module), arg, &key)) {
        goto exit;
    }
    return_value = winreg_FlushKey_impl(module, key);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(winreg_LoadKey__doc__,
"LoadKey($module, key, sub_key, file_name, /)\n"
"--\n"
"\n"
"Insert data into the registry from a file.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    A string that identifies the sub-key to load.\n"
"  file_name\n"
"    The name of the file to load registry data from.  This file must\n"
"    have been created with the SaveKey() function.  Under the file\n"
"    allocation table (FAT) file system, the filename may not have an\n"
"    extension.\n"
"\n"
"Creates a subkey under the specified key and stores registration\n"
"information from a specified file into that subkey.\n"
"\n"
"A call to LoadKey() fails if the calling process does not have the\n"
"SE_RESTORE_PRIVILEGE privilege.\n"
"\n"
"If key is a handle returned by ConnectRegistry(), then the path\n"
"specified in fileName is relative to the remote computer.\n"
"\n"
"The MSDN docs imply key must be in the HKEY_USER or HKEY_LOCAL_MACHINE\n"
"tree.");

#define WINREG_LOADKEY_METHODDEF    \
    {"LoadKey", _PyCFunction_CAST(winreg_LoadKey), METH_FASTCALL, winreg_LoadKey__doc__},

static PyObject *
winreg_LoadKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                    const wchar_t *file_name);

static PyObject *
winreg_LoadKey(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *sub_key = NULL;
    const wchar_t *file_name = NULL;

    if (!_PyArg_CheckPositional("LoadKey", nargs, 3, 3)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("LoadKey", "argument 2", "str", args[1]);
        goto exit;
    }
    sub_key = PyUnicode_AsWideCharString(args[1], NULL);
    if (sub_key == NULL) {
        goto exit;
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("LoadKey", "argument 3", "str", args[2]);
        goto exit;
    }
    file_name = PyUnicode_AsWideCharString(args[2], NULL);
    if (file_name == NULL) {
        goto exit;
    }
    return_value = winreg_LoadKey_impl(module, key, sub_key, file_name);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);
    /* Cleanup for file_name */
    PyMem_Free((void *)file_name);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_OpenKey__doc__,
"OpenKey($module, /, key, sub_key, reserved=0, access=winreg.KEY_READ)\n"
"--\n"
"\n"
"Opens the specified key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    A string that identifies the sub_key to open.\n"
"  reserved\n"
"    A reserved integer that must be zero.  Default is zero.\n"
"  access\n"
"    An integer that specifies an access mask that describes the desired\n"
"    security access for the key.  Default is KEY_READ.\n"
"\n"
"The result is a new handle to the specified key.\n"
"If the function fails, an OSError exception is raised.");

#define WINREG_OPENKEY_METHODDEF    \
    {"OpenKey", _PyCFunction_CAST(winreg_OpenKey), METH_FASTCALL|METH_KEYWORDS, winreg_OpenKey__doc__},

static HKEY
winreg_OpenKey_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                    int reserved, REGSAM access);

static PyObject *
winreg_OpenKey(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(key), &_Py_ID(sub_key), &_Py_ID(reserved), &_Py_ID(access), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "sub_key", "reserved", "access", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "OpenKey",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    HKEY key;
    const wchar_t *sub_key = NULL;
    int reserved = 0;
    REGSAM access = KEY_READ;
    HKEY _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        sub_key = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        sub_key = PyUnicode_AsWideCharString(args[1], NULL);
        if (sub_key == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("OpenKey", "argument 'sub_key'", "str or None", args[1]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        reserved = PyLong_AsInt(args[2]);
        if (reserved == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    access = PyLong_AsInt(args[3]);
    if (access == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = winreg_OpenKey_impl(module, key, sub_key, reserved, access);
    if (_return_value == NULL) {
        goto exit;
    }
    return_value = PyHKEY_FromHKEY(_PyModule_GetState(module), _return_value);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_OpenKeyEx__doc__,
"OpenKeyEx($module, /, key, sub_key, reserved=0, access=winreg.KEY_READ)\n"
"--\n"
"\n"
"Opens the specified key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    A string that identifies the sub_key to open.\n"
"  reserved\n"
"    A reserved integer that must be zero.  Default is zero.\n"
"  access\n"
"    An integer that specifies an access mask that describes the desired\n"
"    security access for the key.  Default is KEY_READ.\n"
"\n"
"The result is a new handle to the specified key.\n"
"If the function fails, an OSError exception is raised.");

#define WINREG_OPENKEYEX_METHODDEF    \
    {"OpenKeyEx", _PyCFunction_CAST(winreg_OpenKeyEx), METH_FASTCALL|METH_KEYWORDS, winreg_OpenKeyEx__doc__},

static HKEY
winreg_OpenKeyEx_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                      int reserved, REGSAM access);

static PyObject *
winreg_OpenKeyEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(key), &_Py_ID(sub_key), &_Py_ID(reserved), &_Py_ID(access), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "sub_key", "reserved", "access", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "OpenKeyEx",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    HKEY key;
    const wchar_t *sub_key = NULL;
    int reserved = 0;
    REGSAM access = KEY_READ;
    HKEY _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        sub_key = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        sub_key = PyUnicode_AsWideCharString(args[1], NULL);
        if (sub_key == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("OpenKeyEx", "argument 'sub_key'", "str or None", args[1]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[2]) {
        reserved = PyLong_AsInt(args[2]);
        if (reserved == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    access = PyLong_AsInt(args[3]);
    if (access == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = winreg_OpenKeyEx_impl(module, key, sub_key, reserved, access);
    if (_return_value == NULL) {
        goto exit;
    }
    return_value = PyHKEY_FromHKEY(_PyModule_GetState(module), _return_value);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_QueryInfoKey__doc__,
"QueryInfoKey($module, key, /)\n"
"--\n"
"\n"
"Returns information about a key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"\n"
"The result is a tuple of 3 items:\n"
"An integer that identifies the number of sub keys this key has.\n"
"An integer that identifies the number of values this key has.\n"
"An integer that identifies when the key was last modified (if available)\n"
"as 100\'s of nanoseconds since Jan 1, 1600.");

#define WINREG_QUERYINFOKEY_METHODDEF    \
    {"QueryInfoKey", (PyCFunction)winreg_QueryInfoKey, METH_O, winreg_QueryInfoKey__doc__},

static PyObject *
winreg_QueryInfoKey_impl(PyObject *module, HKEY key);

static PyObject *
winreg_QueryInfoKey(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HKEY key;

    if (!clinic_HKEY_converter(_PyModule_GetState(module), arg, &key)) {
        goto exit;
    }
    return_value = winreg_QueryInfoKey_impl(module, key);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_QueryValue__doc__,
"QueryValue($module, key, sub_key, /)\n"
"--\n"
"\n"
"Retrieves the unnamed value for a key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    A string that holds the name of the subkey with which the value\n"
"    is associated.  If this parameter is None or empty, the function\n"
"    retrieves the value set by the SetValue() method for the key\n"
"    identified by key.\n"
"\n"
"Values in the registry have name, type, and data components. This method\n"
"retrieves the data for a key\'s first value that has a NULL name.\n"
"But since the underlying API call doesn\'t return the type, you\'ll\n"
"probably be happier using QueryValueEx; this function is just here for\n"
"completeness.");

#define WINREG_QUERYVALUE_METHODDEF    \
    {"QueryValue", _PyCFunction_CAST(winreg_QueryValue), METH_FASTCALL, winreg_QueryValue__doc__},

static PyObject *
winreg_QueryValue_impl(PyObject *module, HKEY key, const wchar_t *sub_key);

static PyObject *
winreg_QueryValue(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *sub_key = NULL;

    if (!_PyArg_CheckPositional("QueryValue", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        sub_key = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        sub_key = PyUnicode_AsWideCharString(args[1], NULL);
        if (sub_key == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("QueryValue", "argument 2", "str or None", args[1]);
        goto exit;
    }
    return_value = winreg_QueryValue_impl(module, key, sub_key);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_QueryValueEx__doc__,
"QueryValueEx($module, key, name, /)\n"
"--\n"
"\n"
"Retrieves the type and value of a specified sub-key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  name\n"
"    A string indicating the value to query.\n"
"\n"
"Behaves mostly like QueryValue(), but also returns the type of the\n"
"specified value name associated with the given open registry key.\n"
"\n"
"The return value is a tuple of the value and the type_id.");

#define WINREG_QUERYVALUEEX_METHODDEF    \
    {"QueryValueEx", _PyCFunction_CAST(winreg_QueryValueEx), METH_FASTCALL, winreg_QueryValueEx__doc__},

static PyObject *
winreg_QueryValueEx_impl(PyObject *module, HKEY key, const wchar_t *name);

static PyObject *
winreg_QueryValueEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *name = NULL;

    if (!_PyArg_CheckPositional("QueryValueEx", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        name = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        name = PyUnicode_AsWideCharString(args[1], NULL);
        if (name == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("QueryValueEx", "argument 2", "str or None", args[1]);
        goto exit;
    }
    return_value = winreg_QueryValueEx_impl(module, key, name);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(winreg_SaveKey__doc__,
"SaveKey($module, key, file_name, /)\n"
"--\n"
"\n"
"Saves the specified key, and all its subkeys to the specified file.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  file_name\n"
"    The name of the file to save registry data to.  This file cannot\n"
"    already exist. If this filename includes an extension, it cannot be\n"
"    used on file allocation table (FAT) file systems by the LoadKey(),\n"
"    ReplaceKey() or RestoreKey() methods.\n"
"\n"
"If key represents a key on a remote computer, the path described by\n"
"file_name is relative to the remote computer.\n"
"\n"
"The caller of this method must possess the SeBackupPrivilege\n"
"security privilege.  This function passes NULL for security_attributes\n"
"to the API.");

#define WINREG_SAVEKEY_METHODDEF    \
    {"SaveKey", _PyCFunction_CAST(winreg_SaveKey), METH_FASTCALL, winreg_SaveKey__doc__},

static PyObject *
winreg_SaveKey_impl(PyObject *module, HKEY key, const wchar_t *file_name);

static PyObject *
winreg_SaveKey(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *file_name = NULL;

    if (!_PyArg_CheckPositional("SaveKey", nargs, 2, 2)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("SaveKey", "argument 2", "str", args[1]);
        goto exit;
    }
    file_name = PyUnicode_AsWideCharString(args[1], NULL);
    if (file_name == NULL) {
        goto exit;
    }
    return_value = winreg_SaveKey_impl(module, key, file_name);

exit:
    /* Cleanup for file_name */
    PyMem_Free((void *)file_name);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_SetValue__doc__,
"SetValue($module, key, sub_key, type, value, /)\n"
"--\n"
"\n"
"Associates a value with a specified key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  sub_key\n"
"    A string that names the subkey with which the value is associated.\n"
"  type\n"
"    An integer that specifies the type of the data.  Currently this must\n"
"    be REG_SZ, meaning only strings are supported.\n"
"  value\n"
"    A string that specifies the new value.\n"
"\n"
"If the key specified by the sub_key parameter does not exist, the\n"
"SetValue function creates it.\n"
"\n"
"Value lengths are limited by available memory. Long values (more than\n"
"2048 bytes) should be stored as files with the filenames stored in\n"
"the configuration registry to help the registry perform efficiently.\n"
"\n"
"The key identified by the key parameter must have been opened with\n"
"KEY_SET_VALUE access.");

#define WINREG_SETVALUE_METHODDEF    \
    {"SetValue", _PyCFunction_CAST(winreg_SetValue), METH_FASTCALL, winreg_SetValue__doc__},

static PyObject *
winreg_SetValue_impl(PyObject *module, HKEY key, const wchar_t *sub_key,
                     DWORD type, PyObject *value_obj);

static PyObject *
winreg_SetValue(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *sub_key = NULL;
    DWORD type;
    PyObject *value_obj;

    if (!_PyArg_CheckPositional("SetValue", nargs, 4, 4)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        sub_key = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        sub_key = PyUnicode_AsWideCharString(args[1], NULL);
        if (sub_key == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("SetValue", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (!_PyLong_UnsignedLong_Converter(args[2], &type)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[3])) {
        _PyArg_BadArgument("SetValue", "argument 4", "str", args[3]);
        goto exit;
    }
    value_obj = args[3];
    return_value = winreg_SetValue_impl(module, key, sub_key, type, value_obj);

exit:
    /* Cleanup for sub_key */
    PyMem_Free((void *)sub_key);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES))

PyDoc_STRVAR(winreg_SetValueEx__doc__,
"SetValueEx($module, key, value_name, reserved, type, value, /)\n"
"--\n"
"\n"
"Stores data in the value field of an open registry key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"  value_name\n"
"    A string containing the name of the value to set, or None.\n"
"  reserved\n"
"    Can be anything - zero is always passed to the API.\n"
"  type\n"
"    An integer that specifies the type of the data, one of:\n"
"    REG_BINARY -- Binary data in any form.\n"
"    REG_DWORD -- A 32-bit number.\n"
"    REG_DWORD_LITTLE_ENDIAN -- A 32-bit number in little-endian format. Equivalent to REG_DWORD\n"
"    REG_DWORD_BIG_ENDIAN -- A 32-bit number in big-endian format.\n"
"    REG_EXPAND_SZ -- A null-terminated string that contains unexpanded\n"
"                     references to environment variables (for example,\n"
"                     %PATH%).\n"
"    REG_LINK -- A Unicode symbolic link.\n"
"    REG_MULTI_SZ -- A sequence of null-terminated strings, terminated\n"
"                    by two null characters.  Note that Python handles\n"
"                    this termination automatically.\n"
"    REG_NONE -- No defined value type.\n"
"    REG_QWORD -- A 64-bit number.\n"
"    REG_QWORD_LITTLE_ENDIAN -- A 64-bit number in little-endian format. Equivalent to REG_QWORD.\n"
"    REG_RESOURCE_LIST -- A device-driver resource list.\n"
"    REG_SZ -- A null-terminated string.\n"
"  value\n"
"    A string that specifies the new value.\n"
"\n"
"This method can also set additional value and type information for the\n"
"specified key.  The key identified by the key parameter must have been\n"
"opened with KEY_SET_VALUE access.\n"
"\n"
"To open the key, use the CreateKeyEx() or OpenKeyEx() methods.\n"
"\n"
"Value lengths are limited by available memory. Long values (more than\n"
"2048 bytes) should be stored as files with the filenames stored in\n"
"the configuration registry to help the registry perform efficiently.");

#define WINREG_SETVALUEEX_METHODDEF    \
    {"SetValueEx", _PyCFunction_CAST(winreg_SetValueEx), METH_FASTCALL, winreg_SetValueEx__doc__},

static PyObject *
winreg_SetValueEx_impl(PyObject *module, HKEY key, const wchar_t *value_name,
                       PyObject *reserved, DWORD type, PyObject *value);

static PyObject *
winreg_SetValueEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HKEY key;
    const wchar_t *value_name = NULL;
    PyObject *reserved;
    DWORD type;
    PyObject *value;

    if (!_PyArg_CheckPositional("SetValueEx", nargs, 5, 5)) {
        goto exit;
    }
    if (!clinic_HKEY_converter(_PyModule_GetState(module), args[0], &key)) {
        goto exit;
    }
    if (args[1] == Py_None) {
        value_name = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        value_name = PyUnicode_AsWideCharString(args[1], NULL);
        if (value_name == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("SetValueEx", "argument 2", "str or None", args[1]);
        goto exit;
    }
    reserved = args[2];
    if (!_PyLong_UnsignedLong_Converter(args[3], &type)) {
        goto exit;
    }
    value = args[4];
    return_value = winreg_SetValueEx_impl(module, key, value_name, reserved, type, value);

exit:
    /* Cleanup for value_name */
    PyMem_Free((void *)value_name);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(winreg_DisableReflectionKey__doc__,
"DisableReflectionKey($module, key, /)\n"
"--\n"
"\n"
"Disables registry reflection for 32bit processes running on a 64bit OS.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"\n"
"Will generally raise NotImplementedError if executed on a 32bit OS.\n"
"\n"
"If the key is not on the reflection list, the function succeeds but has\n"
"no effect.  Disabling reflection for a key does not affect reflection\n"
"of any subkeys.");

#define WINREG_DISABLEREFLECTIONKEY_METHODDEF    \
    {"DisableReflectionKey", (PyCFunction)winreg_DisableReflectionKey, METH_O, winreg_DisableReflectionKey__doc__},

static PyObject *
winreg_DisableReflectionKey_impl(PyObject *module, HKEY key);

static PyObject *
winreg_DisableReflectionKey(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HKEY key;

    if (!clinic_HKEY_converter(_PyModule_GetState(module), arg, &key)) {
        goto exit;
    }
    return_value = winreg_DisableReflectionKey_impl(module, key);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(winreg_EnableReflectionKey__doc__,
"EnableReflectionKey($module, key, /)\n"
"--\n"
"\n"
"Restores registry reflection for the specified disabled key.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"\n"
"Will generally raise NotImplementedError if executed on a 32bit OS.\n"
"Restoring reflection for a key does not affect reflection of any\n"
"subkeys.");

#define WINREG_ENABLEREFLECTIONKEY_METHODDEF    \
    {"EnableReflectionKey", (PyCFunction)winreg_EnableReflectionKey, METH_O, winreg_EnableReflectionKey__doc__},

static PyObject *
winreg_EnableReflectionKey_impl(PyObject *module, HKEY key);

static PyObject *
winreg_EnableReflectionKey(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HKEY key;

    if (!clinic_HKEY_converter(_PyModule_GetState(module), arg, &key)) {
        goto exit;
    }
    return_value = winreg_EnableReflectionKey_impl(module, key);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(winreg_QueryReflectionKey__doc__,
"QueryReflectionKey($module, key, /)\n"
"--\n"
"\n"
"Returns the reflection state for the specified key as a bool.\n"
"\n"
"  key\n"
"    An already open key, or any one of the predefined HKEY_* constants.\n"
"\n"
"Will generally raise NotImplementedError if executed on a 32bit OS.");

#define WINREG_QUERYREFLECTIONKEY_METHODDEF    \
    {"QueryReflectionKey", (PyCFunction)winreg_QueryReflectionKey, METH_O, winreg_QueryReflectionKey__doc__},

static PyObject *
winreg_QueryReflectionKey_impl(PyObject *module, HKEY key);

static PyObject *
winreg_QueryReflectionKey(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HKEY key;

    if (!clinic_HKEY_converter(_PyModule_GetState(module), arg, &key)) {
        goto exit;
    }
    return_value = winreg_QueryReflectionKey_impl(module, key);

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)) && (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

#ifndef WINREG_HKEYTYPE_CLOSE_METHODDEF
    #define WINREG_HKEYTYPE_CLOSE_METHODDEF
#endif /* !defined(WINREG_HKEYTYPE_CLOSE_METHODDEF) */

#ifndef WINREG_HKEYTYPE_DETACH_METHODDEF
    #define WINREG_HKEYTYPE_DETACH_METHODDEF
#endif /* !defined(WINREG_HKEYTYPE_DETACH_METHODDEF) */

#ifndef WINREG_HKEYTYPE___ENTER___METHODDEF
    #define WINREG_HKEYTYPE___ENTER___METHODDEF
#endif /* !defined(WINREG_HKEYTYPE___ENTER___METHODDEF) */

#ifndef WINREG_HKEYTYPE___EXIT___METHODDEF
    #define WINREG_HKEYTYPE___EXIT___METHODDEF
#endif /* !defined(WINREG_HKEYTYPE___EXIT___METHODDEF) */

#ifndef WINREG_CLOSEKEY_METHODDEF
    #define WINREG_CLOSEKEY_METHODDEF
#endif /* !defined(WINREG_CLOSEKEY_METHODDEF) */

#ifndef WINREG_CONNECTREGISTRY_METHODDEF
    #define WINREG_CONNECTREGISTRY_METHODDEF
#endif /* !defined(WINREG_CONNECTREGISTRY_METHODDEF) */

#ifndef WINREG_CREATEKEY_METHODDEF
    #define WINREG_CREATEKEY_METHODDEF
#endif /* !defined(WINREG_CREATEKEY_METHODDEF) */

#ifndef WINREG_CREATEKEYEX_METHODDEF
    #define WINREG_CREATEKEYEX_METHODDEF
#endif /* !defined(WINREG_CREATEKEYEX_METHODDEF) */

#ifndef WINREG_DELETEKEY_METHODDEF
    #define WINREG_DELETEKEY_METHODDEF
#endif /* !defined(WINREG_DELETEKEY_METHODDEF) */

#ifndef WINREG_DELETEKEYEX_METHODDEF
    #define WINREG_DELETEKEYEX_METHODDEF
#endif /* !defined(WINREG_DELETEKEYEX_METHODDEF) */

#ifndef WINREG_DELETEVALUE_METHODDEF
    #define WINREG_DELETEVALUE_METHODDEF
#endif /* !defined(WINREG_DELETEVALUE_METHODDEF) */

#ifndef WINREG_ENUMKEY_METHODDEF
    #define WINREG_ENUMKEY_METHODDEF
#endif /* !defined(WINREG_ENUMKEY_METHODDEF) */

#ifndef WINREG_ENUMVALUE_METHODDEF
    #define WINREG_ENUMVALUE_METHODDEF
#endif /* !defined(WINREG_ENUMVALUE_METHODDEF) */

#ifndef WINREG_EXPANDENVIRONMENTSTRINGS_METHODDEF
    #define WINREG_EXPANDENVIRONMENTSTRINGS_METHODDEF
#endif /* !defined(WINREG_EXPANDENVIRONMENTSTRINGS_METHODDEF) */

#ifndef WINREG_FLUSHKEY_METHODDEF
    #define WINREG_FLUSHKEY_METHODDEF
#endif /* !defined(WINREG_FLUSHKEY_METHODDEF) */

#ifndef WINREG_LOADKEY_METHODDEF
    #define WINREG_LOADKEY_METHODDEF
#endif /* !defined(WINREG_LOADKEY_METHODDEF) */

#ifndef WINREG_OPENKEY_METHODDEF
    #define WINREG_OPENKEY_METHODDEF
#endif /* !defined(WINREG_OPENKEY_METHODDEF) */

#ifndef WINREG_OPENKEYEX_METHODDEF
    #define WINREG_OPENKEYEX_METHODDEF
#endif /* !defined(WINREG_OPENKEYEX_METHODDEF) */

#ifndef WINREG_QUERYINFOKEY_METHODDEF
    #define WINREG_QUERYINFOKEY_METHODDEF
#endif /* !defined(WINREG_QUERYINFOKEY_METHODDEF) */

#ifndef WINREG_QUERYVALUE_METHODDEF
    #define WINREG_QUERYVALUE_METHODDEF
#endif /* !defined(WINREG_QUERYVALUE_METHODDEF) */

#ifndef WINREG_QUERYVALUEEX_METHODDEF
    #define WINREG_QUERYVALUEEX_METHODDEF
#endif /* !defined(WINREG_QUERYVALUEEX_METHODDEF) */

#ifndef WINREG_SAVEKEY_METHODDEF
    #define WINREG_SAVEKEY_METHODDEF
#endif /* !defined(WINREG_SAVEKEY_METHODDEF) */

#ifndef WINREG_SETVALUE_METHODDEF
    #define WINREG_SETVALUE_METHODDEF
#endif /* !defined(WINREG_SETVALUE_METHODDEF) */

#ifndef WINREG_SETVALUEEX_METHODDEF
    #define WINREG_SETVALUEEX_METHODDEF
#endif /* !defined(WINREG_SETVALUEEX_METHODDEF) */

#ifndef WINREG_DISABLEREFLECTIONKEY_METHODDEF
    #define WINREG_DISABLEREFLECTIONKEY_METHODDEF
#endif /* !defined(WINREG_DISABLEREFLECTIONKEY_METHODDEF) */

#ifndef WINREG_ENABLEREFLECTIONKEY_METHODDEF
    #define WINREG_ENABLEREFLECTIONKEY_METHODDEF
#endif /* !defined(WINREG_ENABLEREFLECTIONKEY_METHODDEF) */

#ifndef WINREG_QUERYREFLECTIONKEY_METHODDEF
    #define WINREG_QUERYREFLECTIONKEY_METHODDEF
#endif /* !defined(WINREG_QUERYREFLECTIONKEY_METHODDEF) */
/*[clinic end generated code: output=be4b6857b95558b5 input=a9049054013a1b77]*/
