/*
  _winreg.c

  Windows Registry access module for Python.

  * Simple registry access written by Mark Hammond in win32api
    module circa 1995.
  * Bill Tutt expanded the support significantly not long after.
  * Numerous other people have submitted patches since then.
  * Ripped from win32api module 03-Feb-2000 by Mark Hammond, and
    basic Unicode support added.

*/

#include "Python.h"
#include "structmember.h"
#include "malloc.h" /* for alloca */
#include "windows.h"

static BOOL PyHKEY_AsHKEY(PyObject *ob, HKEY *pRes, BOOL bNoneOK);
static PyObject *PyHKEY_FromHKEY(HKEY h);
static BOOL PyHKEY_Close(PyObject *obHandle);

static char errNotAHandle[] = "Object is not a handle";

/* The win32api module reports the function name that failed,
   but this concept is not in the Python core.
   Hopefully it will one day, and in the meantime I don't
   want to lose this info...
*/
#define PyErr_SetFromWindowsErrWithFunction(rc, fnname) \
    PyErr_SetFromWindowsErr(rc)

/* Forward declares */

/* Doc strings */
PyDoc_STRVAR(module_doc,
"This module provides access to the Windows registry API.\n"
"\n"
"Functions:\n"
"\n"
"CloseKey() - Closes a registry key.\n"
"ConnectRegistry() - Establishes a connection to a predefined registry handle\n"
"                    on another computer.\n"
"CreateKey() - Creates the specified key, or opens it if it already exists.\n"
"DeleteKey() - Deletes the specified key.\n"
"DeleteValue() - Removes a named value from the specified registry key.\n"
"EnumKey() - Enumerates subkeys of the specified open registry key.\n"
"EnumValue() - Enumerates values of the specified open registry key.\n"
"ExpandEnvironmentStrings() - Expand the env strings in a REG_EXPAND_SZ string.\n"
"FlushKey() - Writes all the attributes of the specified key to the registry.\n"
"LoadKey() - Creates a subkey under HKEY_USER or HKEY_LOCAL_MACHINE and stores\n"
"            registration information from a specified file into that subkey.\n"
"OpenKey() - Alias for <om win32api.RegOpenKeyEx>\n"
"OpenKeyEx() - Opens the specified key.\n"
"QueryValue() - Retrieves the value associated with the unnamed value for a\n"
"               specified key in the registry.\n"
"QueryValueEx() - Retrieves the type and data for a specified value name\n"
"                 associated with an open registry key.\n"
"QueryInfoKey() - Returns information about the specified key.\n"
"SaveKey() - Saves the specified key, and all its subkeys a file.\n"
"SetValue() - Associates a value with a specified key.\n"
"SetValueEx() - Stores data in the value field of an open registry key.\n"
"\n"
"Special objects:\n"
"\n"
"HKEYType -- type object for HKEY objects\n"
"error -- exception raised for Win32 errors\n"
"\n"
"Integer constants:\n"
"Many constants are defined - see the documentation for each function\n"
"to see what constants are used, and where.");


PyDoc_STRVAR(CloseKey_doc,
"CloseKey(hkey) - Closes a previously opened registry key.\n"
"\n"
"The hkey argument specifies a previously opened key.\n"
"\n"
"Note that if the key is not closed using this method, it will be\n"
"closed when the hkey object is destroyed by Python.");

PyDoc_STRVAR(ConnectRegistry_doc,
"key = ConnectRegistry(computer_name, key) - "
"Establishes a connection to a predefined registry handle on another computer.\n"
"\n"
"computer_name is the name of the remote computer, of the form \\\\computername.\n"
" If None, the local computer is used.\n"
"key is the predefined handle to connect to.\n"
"\n"
"The return value is the handle of the opened key.\n"
"If the function fails, a WindowsError exception is raised.");

PyDoc_STRVAR(CreateKey_doc,
"key = CreateKey(key, sub_key) - Creates or opens the specified key.\n"
"\n"
"key is an already open key, or one of the predefined HKEY_* constants\n"
"sub_key is a string that names the key this method opens or creates.\n"
" If key is one of the predefined keys, sub_key may be None. In that case,\n"
" the handle returned is the same key handle passed in to the function.\n"
"\n"
"If the key already exists, this function opens the existing key\n"
"\n"
"The return value is the handle of the opened key.\n"
"If the function fails, an exception is raised.");

PyDoc_STRVAR(CreateKeyEx_doc,
"key = CreateKeyEx(key, sub_key, res, sam) - Creates or opens the specified key.\n"
"\n"
"key is an already open key, or one of the predefined HKEY_* constants\n"
"sub_key is a string that names the key this method opens or creates.\n"
"res is a reserved integer, and must be zero.  Default is zero.\n"
"sam is an integer that specifies an access mask that describes the desired\n"
" If key is one of the predefined keys, sub_key may be None. In that case,\n"
" the handle returned is the same key handle passed in to the function.\n"
"\n"
"If the key already exists, this function opens the existing key\n"
"\n"
"The return value is the handle of the opened key.\n"
"If the function fails, an exception is raised.");

PyDoc_STRVAR(DeleteKey_doc,
"DeleteKey(key, sub_key) - Deletes the specified key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"sub_key is a string that must be a subkey of the key identified by the key parameter.\n"
" This value must not be None, and the key may not have subkeys.\n"
"\n"
"This method can not delete keys with subkeys.\n"
"\n"
"If the method succeeds, the entire key, including all of its values,\n"
"is removed.  If the method fails, a WindowsError exception is raised.");

PyDoc_STRVAR(DeleteKeyEx_doc,
"DeleteKeyEx(key, sub_key, sam, res) - Deletes the specified key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"sub_key is a string that must be a subkey of the key identified by the key parameter.\n"
"res is a reserved integer, and must be zero.  Default is zero.\n"
"sam is an integer that specifies an access mask that describes the desired\n"
" This value must not be None, and the key may not have subkeys.\n"
"\n"
"This method can not delete keys with subkeys.\n"
"\n"
"If the method succeeds, the entire key, including all of its values,\n"
"is removed.  If the method fails, a WindowsError exception is raised.\n"
"On unsupported Windows versions, NotImplementedError is raised.");

PyDoc_STRVAR(DeleteValue_doc,
"DeleteValue(key, value) - Removes a named value from a registry key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"value is a string that identifies the value to remove.");

PyDoc_STRVAR(EnumKey_doc,
"string = EnumKey(key, index) - Enumerates subkeys of an open registry key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"index is an integer that identifies the index of the key to retrieve.\n"
"\n"
"The function retrieves the name of one subkey each time it is called.\n"
"It is typically called repeatedly until a WindowsError exception is\n"
"raised, indicating no more values are available.");

PyDoc_STRVAR(EnumValue_doc,
"tuple = EnumValue(key, index) - Enumerates values of an open registry key.\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"index is an integer that identifies the index of the value to retrieve.\n"
"\n"
"The function retrieves the name of one subkey each time it is called.\n"
"It is typically called repeatedly, until a WindowsError exception\n"
"is raised, indicating no more values.\n"
"\n"
"The result is a tuple of 3 items:\n"
"value_name is a string that identifies the value.\n"
"value_data is an object that holds the value data, and whose type depends\n"
" on the underlying registry type.\n"
"data_type is an integer that identifies the type of the value data.");

PyDoc_STRVAR(ExpandEnvironmentStrings_doc,
"string = ExpandEnvironmentStrings(string) - Expand environment vars.\n");

PyDoc_STRVAR(FlushKey_doc,
"FlushKey(key) - Writes all the attributes of a key to the registry.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"\n"
"It is not necessary to call RegFlushKey to change a key.\n"
"Registry changes are flushed to disk by the registry using its lazy flusher.\n"
"Registry changes are also flushed to disk at system shutdown.\n"
"Unlike CloseKey(), the FlushKey() method returns only when all the data has\n"
"been written to the registry.\n"
"An application should only call FlushKey() if it requires absolute certainty that registry changes are on disk.\n"
"If you don't know whether a FlushKey() call is required, it probably isn't.");

PyDoc_STRVAR(LoadKey_doc,
"LoadKey(key, sub_key, file_name) - Creates a subkey under the specified key\n"
"and stores registration information from a specified file into that subkey.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"sub_key is a string that identifies the sub_key to load\n"
"file_name is the name of the file to load registry data from.\n"
" This file must have been created with the SaveKey() function.\n"
" Under the file allocation table (FAT) file system, the filename may not\n"
"have an extension.\n"
"\n"
"A call to LoadKey() fails if the calling process does not have the\n"
"SE_RESTORE_PRIVILEGE privilege.\n"
"\n"
"If key is a handle returned by ConnectRegistry(), then the path specified\n"
"in fileName is relative to the remote computer.\n"
"\n"
"The docs imply key must be in the HKEY_USER or HKEY_LOCAL_MACHINE tree");

PyDoc_STRVAR(OpenKey_doc,
"key = OpenKey(key, sub_key, res = 0, sam = KEY_READ) - Opens the specified key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"sub_key is a string that identifies the sub_key to open\n"
"res is a reserved integer, and must be zero.  Default is zero.\n"
"sam is an integer that specifies an access mask that describes the desired\n"
" security access for the key.  Default is KEY_READ\n"
"\n"
"The result is a new handle to the specified key\n"
"If the function fails, a WindowsError exception is raised.");

PyDoc_STRVAR(OpenKeyEx_doc, "See OpenKey()");

PyDoc_STRVAR(QueryInfoKey_doc,
"tuple = QueryInfoKey(key) - Returns information about a key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"\n"
"The result is a tuple of 3 items:"
"An integer that identifies the number of sub keys this key has.\n"
"An integer that identifies the number of values this key has.\n"
"A long integer that identifies when the key was last modified (if available)\n"
" as 100's of nanoseconds since Jan 1, 1600.");

PyDoc_STRVAR(QueryValue_doc,
"string = QueryValue(key, sub_key) - retrieves the unnamed value for a key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"sub_key is a string that holds the name of the subkey with which the value\n"
" is associated.  If this parameter is None or empty, the function retrieves\n"
" the value set by the SetValue() method for the key identified by key."
"\n"
"Values in the registry have name, type, and data components. This method\n"
"retrieves the data for a key's first value that has a NULL name.\n"
"But the underlying API call doesn't return the type, Lame Lame Lame, DONT USE THIS!!!");

PyDoc_STRVAR(QueryValueEx_doc,
"value,type_id = QueryValueEx(key, value_name) - Retrieves the type and data for a specified value name associated with an open registry key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"value_name is a string indicating the value to query");

PyDoc_STRVAR(SaveKey_doc,
"SaveKey(key, file_name) - Saves the specified key, and all its subkeys to the specified file.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"file_name is the name of the file to save registry data to.\n"
" This file cannot already exist. If this filename includes an extension,\n"
" it cannot be used on file allocation table (FAT) file systems by the\n"
" LoadKey(), ReplaceKey() or RestoreKey() methods.\n"
"\n"
"If key represents a key on a remote computer, the path described by\n"
"file_name is relative to the remote computer.\n"
"The caller of this method must possess the SeBackupPrivilege security privilege.\n"
"This function passes NULL for security_attributes to the API.");

PyDoc_STRVAR(SetValue_doc,
"SetValue(key, sub_key, type, value) - Associates a value with a specified key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"sub_key is a string that names the subkey with which the value is associated.\n"
"type is an integer that specifies the type of the data.  Currently this\n"
" must be REG_SZ, meaning only strings are supported.\n"
"value is a string that specifies the new value.\n"
"\n"
"If the key specified by the sub_key parameter does not exist, the SetValue\n"
"function creates it.\n"
"\n"
"Value lengths are limited by available memory. Long values (more than\n"
"2048 bytes) should be stored as files with the filenames stored in \n"
"the configuration registry.  This helps the registry perform efficiently.\n"
"\n"
"The key identified by the key parameter must have been opened with\n"
"KEY_SET_VALUE access.");

PyDoc_STRVAR(SetValueEx_doc,
"SetValueEx(key, value_name, reserved, type, value) - Stores data in the value field of an open registry key.\n"
"\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"value_name is a string containing the name of the value to set, or None\n"
"type is an integer that specifies the type of the data.  This should be one of:\n"
"  REG_BINARY -- Binary data in any form.\n"
"  REG_DWORD -- A 32-bit number.\n"
"  REG_DWORD_LITTLE_ENDIAN -- A 32-bit number in little-endian format.\n"
"  REG_DWORD_BIG_ENDIAN -- A 32-bit number in big-endian format.\n"
"  REG_EXPAND_SZ -- A null-terminated string that contains unexpanded references\n"
"                   to environment variables (for example, %PATH%).\n"
"  REG_LINK -- A Unicode symbolic link.\n"
"  REG_MULTI_SZ -- A sequence of null-terminated strings, terminated by\n"
"                  two null characters.  Note that Python handles this\n"
"                  termination automatically.\n"
"  REG_NONE -- No defined value type.\n"
"  REG_RESOURCE_LIST -- A device-driver resource list.\n"
"  REG_SZ -- A null-terminated string.\n"
"reserved can be anything - zero is always passed to the API.\n"
"value is a string that specifies the new value.\n"
"\n"
"This method can also set additional value and type information for the\n"
"specified key.  The key identified by the key parameter must have been\n"
"opened with KEY_SET_VALUE access.\n"
"\n"
"To open the key, use the CreateKeyEx() or OpenKeyEx() methods.\n"
"\n"
"Value lengths are limited by available memory. Long values (more than\n"
"2048 bytes) should be stored as files with the filenames stored in \n"
"the configuration registry.  This helps the registry perform efficiently.");

PyDoc_STRVAR(DisableReflectionKey_doc,
"Disables registry reflection for 32-bit processes running on a 64-bit\n"
"Operating System.  Will generally raise NotImplemented if executed on\n"
"a 32-bit Operating System.\n"
"If the key is not on the reflection list, the function succeeds but has no effect.\n"
"Disabling reflection for a key does not affect reflection of any subkeys.");

PyDoc_STRVAR(EnableReflectionKey_doc,
"Restores registry reflection for the specified disabled key.\n"
"Will generally raise NotImplemented if executed on a 32-bit Operating System.\n"
"Restoring reflection for a key does not affect reflection of any subkeys.");

PyDoc_STRVAR(QueryReflectionKey_doc,
"bool = QueryReflectionKey(hkey) - Determines the reflection state for the specified key.\n"
"Will generally raise NotImplemented if executed on a 32-bit Operating System.\n");

/* PyHKEY docstrings */
PyDoc_STRVAR(PyHKEY_doc,
"PyHKEY Object - A Python object, representing a win32 registry key.\n"
"\n"
"This object wraps a Windows HKEY object, automatically closing it when\n"
"the object is destroyed.  To guarantee cleanup, you can call either\n"
"the Close() method on the PyHKEY, or the CloseKey() method.\n"
"\n"
"All functions which accept a handle object also accept an integer - \n"
"however, use of the handle object is encouraged.\n"
"\n"
"Functions:\n"
"Close() - Closes the underlying handle.\n"
"Detach() - Returns the integer Win32 handle, detaching it from the object\n"
"\n"
"Properties:\n"
"handle - The integer Win32 handle.\n"
"\n"
"Operations:\n"
"__nonzero__ - Handles with an open object return true, otherwise false.\n"
"__int__ - Converting a handle to an integer returns the Win32 handle.\n"
"__cmp__ - Handle objects are compared using the handle value.");


PyDoc_STRVAR(PyHKEY_Close_doc,
"key.Close() - Closes the underlying Windows handle.\n"
"\n"
"If the handle is already closed, no error is raised.");

PyDoc_STRVAR(PyHKEY_Detach_doc,
"int = key.Detach() - Detaches the Windows handle from the handle object.\n"
"\n"
"The result is the value of the handle before it is detached.  If the\n"
"handle is already detached, this will return zero.\n"
"\n"
"After calling this function, the handle is effectively invalidated,\n"
"but the handle is not closed.  You would call this function when you\n"
"need the underlying win32 handle to exist beyond the lifetime of the\n"
"handle object.\n"
"On 64 bit windows, the result of this function is a long integer");


/************************************************************************

  The PyHKEY object definition

************************************************************************/
typedef struct {
    PyObject_VAR_HEAD
    HKEY hkey;
} PyHKEYObject;

#define PyHKEY_Check(op) ((op)->ob_type == &PyHKEY_Type)

static char *failMsg = "bad operand type";

static PyObject *
PyHKEY_unaryFailureFunc(PyObject *ob)
{
    PyErr_SetString(PyExc_TypeError, failMsg);
    return NULL;
}
static PyObject *
PyHKEY_binaryFailureFunc(PyObject *ob1, PyObject *ob2)
{
    PyErr_SetString(PyExc_TypeError, failMsg);
    return NULL;
}
static PyObject *
PyHKEY_ternaryFailureFunc(PyObject *ob1, PyObject *ob2, PyObject *ob3)
{
    PyErr_SetString(PyExc_TypeError, failMsg);
    return NULL;
}

static void
PyHKEY_deallocFunc(PyObject *ob)
{
    /* Can not call PyHKEY_Close, as the ob->tp_type
       has already been cleared, thus causing the type
       check to fail!
    */
    PyHKEYObject *obkey = (PyHKEYObject *)ob;
    if (obkey->hkey)
        RegCloseKey((HKEY)obkey->hkey);
    PyObject_DEL(ob);
}

static int
PyHKEY_nonzeroFunc(PyObject *ob)
{
    return ((PyHKEYObject *)ob)->hkey != 0;
}

static PyObject *
PyHKEY_intFunc(PyObject *ob)
{
    PyHKEYObject *pyhkey = (PyHKEYObject *)ob;
    return PyLong_FromVoidPtr(pyhkey->hkey);
}

static int
PyHKEY_printFunc(PyObject *ob, FILE *fp, int flags)
{
    PyHKEYObject *pyhkey = (PyHKEYObject *)ob;
    fprintf(fp, "<PyHKEY at %p (%p)>",
        ob, pyhkey->hkey);
    return 0;
}

static PyObject *
PyHKEY_strFunc(PyObject *ob)
{
    PyHKEYObject *pyhkey = (PyHKEYObject *)ob;
    return PyString_FromFormat("<PyHKEY:%p>", pyhkey->hkey);
}

static int
PyHKEY_compareFunc(PyObject *ob1, PyObject *ob2)
{
    PyHKEYObject *pyhkey1 = (PyHKEYObject *)ob1;
    PyHKEYObject *pyhkey2 = (PyHKEYObject *)ob2;
    return pyhkey1 == pyhkey2 ? 0 :
         (pyhkey1 < pyhkey2 ? -1 : 1);
}

static long
PyHKEY_hashFunc(PyObject *ob)
{
    /* Just use the address.
       XXX - should we use the handle value?
    */
    return _Py_HashPointer(ob);
}


static PyNumberMethods PyHKEY_NumberMethods =
{
    PyHKEY_binaryFailureFunc,           /* nb_add */
    PyHKEY_binaryFailureFunc,           /* nb_subtract */
    PyHKEY_binaryFailureFunc,           /* nb_multiply */
    PyHKEY_binaryFailureFunc,           /* nb_divide */
    PyHKEY_binaryFailureFunc,           /* nb_remainder */
    PyHKEY_binaryFailureFunc,           /* nb_divmod */
    PyHKEY_ternaryFailureFunc,          /* nb_power */
    PyHKEY_unaryFailureFunc,            /* nb_negative */
    PyHKEY_unaryFailureFunc,            /* nb_positive */
    PyHKEY_unaryFailureFunc,            /* nb_absolute */
    PyHKEY_nonzeroFunc,                 /* nb_nonzero */
    PyHKEY_unaryFailureFunc,            /* nb_invert */
    PyHKEY_binaryFailureFunc,           /* nb_lshift */
    PyHKEY_binaryFailureFunc,           /* nb_rshift */
    PyHKEY_binaryFailureFunc,           /* nb_and */
    PyHKEY_binaryFailureFunc,           /* nb_xor */
    PyHKEY_binaryFailureFunc,           /* nb_or */
    0,                  /* nb_coerce (allowed to be zero) */
    PyHKEY_intFunc,                     /* nb_int */
    PyHKEY_unaryFailureFunc,            /* nb_long */
    PyHKEY_unaryFailureFunc,            /* nb_float */
    PyHKEY_unaryFailureFunc,            /* nb_oct */
    PyHKEY_unaryFailureFunc,            /* nb_hex */
};

static PyObject *PyHKEY_CloseMethod(PyObject *self, PyObject *args);
static PyObject *PyHKEY_DetachMethod(PyObject *self, PyObject *args);
static PyObject *PyHKEY_Enter(PyObject *self);
static PyObject *PyHKEY_Exit(PyObject *self, PyObject *args);

static struct PyMethodDef PyHKEY_methods[] = {
    {"Close",  PyHKEY_CloseMethod, METH_VARARGS, PyHKEY_Close_doc},
    {"Detach", PyHKEY_DetachMethod, METH_VARARGS, PyHKEY_Detach_doc},
    {"__enter__", (PyCFunction)PyHKEY_Enter, METH_NOARGS, NULL},
    {"__exit__", PyHKEY_Exit, METH_VARARGS, NULL},
    {NULL}
};

static PyMemberDef PyHKEY_memberlist[] = {
    {"handle", T_PYSSIZET, offsetof(PyHKEYObject, hkey), READONLY},
    {NULL}    /* Sentinel */
};

/* The type itself */
PyTypeObject PyHKEY_Type =
{
    PyVarObject_HEAD_INIT(0, 0) /* fill in type at module init */
    "PyHKEY",
    sizeof(PyHKEYObject),
    0,
    PyHKEY_deallocFunc,                 /* tp_dealloc */
    PyHKEY_printFunc,                   /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    PyHKEY_compareFunc,                 /* tp_compare */
    0,                                  /* tp_repr */
    &PyHKEY_NumberMethods,              /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    PyHKEY_hashFunc,                    /* tp_hash */
    0,                                  /* tp_call */
    PyHKEY_strFunc,                     /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    PyHKEY_doc,                         /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    PyHKEY_methods,                     /* tp_methods */
    PyHKEY_memberlist,                  /* tp_members */
};

/************************************************************************

  The PyHKEY object methods

************************************************************************/
static PyObject *
PyHKEY_CloseMethod(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":Close"))
        return NULL;
    if (!PyHKEY_Close(self))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyHKEY_DetachMethod(PyObject *self, PyObject *args)
{
    void* ret;
    PyHKEYObject *pThis = (PyHKEYObject *)self;
    if (!PyArg_ParseTuple(args, ":Detach"))
        return NULL;
    ret = (void*)pThis->hkey;
    pThis->hkey = 0;
    return PyLong_FromVoidPtr(ret);
}

static PyObject *
PyHKEY_Enter(PyObject *self)
{
    Py_XINCREF(self);
    return self;
}

static PyObject *
PyHKEY_Exit(PyObject *self, PyObject *args)
{
    if (!PyHKEY_Close(self))
        return NULL;
    Py_RETURN_NONE;
}


/************************************************************************
   The public PyHKEY API (well, not public yet :-)
************************************************************************/
PyObject *
PyHKEY_New(HKEY hInit)
{
    PyHKEYObject *key = PyObject_NEW(PyHKEYObject, &PyHKEY_Type);
    if (key)
        key->hkey = hInit;
    return (PyObject *)key;
}

BOOL
PyHKEY_Close(PyObject *ob_handle)
{
    LONG rc;
    PyHKEYObject *key;

    if (!PyHKEY_Check(ob_handle)) {
        PyErr_SetString(PyExc_TypeError, "bad operand type");
        return FALSE;
    }
    key = (PyHKEYObject *)ob_handle;
    rc = key->hkey ? RegCloseKey((HKEY)key->hkey) : ERROR_SUCCESS;
    key->hkey = 0;
    if (rc != ERROR_SUCCESS)
        PyErr_SetFromWindowsErrWithFunction(rc, "RegCloseKey");
    return rc == ERROR_SUCCESS;
}

BOOL
PyHKEY_AsHKEY(PyObject *ob, HKEY *pHANDLE, BOOL bNoneOK)
{
    if (ob == Py_None) {
        if (!bNoneOK) {
            PyErr_SetString(
                      PyExc_TypeError,
                      "None is not a valid HKEY in this context");
            return FALSE;
        }
        *pHANDLE = (HKEY)0;
    }
    else if (PyHKEY_Check(ob)) {
        PyHKEYObject *pH = (PyHKEYObject *)ob;
        *pHANDLE = pH->hkey;
    }
    else if (PyInt_Check(ob) || PyLong_Check(ob)) {
        /* We also support integers */
        PyErr_Clear();
        *pHANDLE = (HKEY)PyLong_AsVoidPtr(ob);
        if (PyErr_Occurred())
            return FALSE;
    }
    else {
        PyErr_SetString(
                        PyExc_TypeError,
            "The object is not a PyHKEY object");
        return FALSE;
    }
    return TRUE;
}

PyObject *
PyHKEY_FromHKEY(HKEY h)
{
    PyHKEYObject *op;

    /* Inline PyObject_New */
    op = (PyHKEYObject *) PyObject_MALLOC(sizeof(PyHKEYObject));
    if (op == NULL)
        return PyErr_NoMemory();
    PyObject_INIT(op, &PyHKEY_Type);
    op->hkey = h;
    return (PyObject *)op;
}


/************************************************************************
  The module methods
************************************************************************/
BOOL
PyWinObject_CloseHKEY(PyObject *obHandle)
{
    BOOL ok;
    if (PyHKEY_Check(obHandle)) {
        ok = PyHKEY_Close(obHandle);
    }
#if SIZEOF_LONG >= SIZEOF_HKEY
    else if (PyInt_Check(obHandle)) {
        long rc = RegCloseKey((HKEY)PyInt_AsLong(obHandle));
        ok = (rc == ERROR_SUCCESS);
        if (!ok)
            PyErr_SetFromWindowsErrWithFunction(rc, "RegCloseKey");
    }
#else
    else if (PyLong_Check(obHandle)) {
        long rc = RegCloseKey((HKEY)PyLong_AsVoidPtr(obHandle));
        ok = (rc == ERROR_SUCCESS);
        if (!ok)
            PyErr_SetFromWindowsErrWithFunction(rc, "RegCloseKey");
    }
#endif
    else {
        PyErr_SetString(
            PyExc_TypeError,
            "A handle must be a HKEY object or an integer");
        return FALSE;
    }
    return ok;
}


/*
   Private Helper functions for the registry interfaces

** Note that fixupMultiSZ and countString have both had changes
** made to support "incorrect strings".  The registry specification
** calls for strings to be terminated with 2 null bytes.  It seems
** some commercial packages install strings which don't conform,
** causing this code to fail - however, "regedit" etc still work
** with these strings (ie only we don't!).
*/
static void
fixupMultiSZ(char **str, char *data, int len)
{
    char *P;
    int i;
    char *Q;

    Q = data + len;
    for (P = data, i = 0; P < Q && *P != '\0'; P++, i++) {
        str[i] = P;
        for(; *P != '\0'; P++)
            ;
    }
}

static int
countStrings(char *data, int len)
{
    int strings;
    char *P;
    char *Q = data + len;

    for (P = data, strings = 0; P < Q && *P != '\0'; P++, strings++)
        for (; P < Q && *P != '\0'; P++)
            ;
    return strings;
}

/* Convert PyObject into Registry data.
   Allocates space as needed. */
static BOOL
Py2Reg(PyObject *value, DWORD typ, BYTE **retDataBuf, DWORD *retDataSize)
{
    Py_ssize_t i,j;
    switch (typ) {
        case REG_DWORD:
            if (value != Py_None &&
                !(PyInt_Check(value) || PyLong_Check(value)))
                return FALSE;
            *retDataBuf = (BYTE *)PyMem_NEW(DWORD, 1);
            if (*retDataBuf==NULL){
                PyErr_NoMemory();
                return FALSE;
            }
            *retDataSize = sizeof(DWORD);
            if (value == Py_None) {
                DWORD zero = 0;
                memcpy(*retDataBuf, &zero, sizeof(DWORD));
            }
            else {
                DWORD d = PyLong_AsUnsignedLong(value);
                memcpy(*retDataBuf, &d, sizeof(DWORD));
            }
            break;
        case REG_SZ:
        case REG_EXPAND_SZ:
            {
            int need_decref = 0;
            if (value == Py_None)
                *retDataSize = 1;
            else {
                if (PyUnicode_Check(value)) {
                    value = PyUnicode_AsEncodedString(
                                  value,
                                  "mbcs",
                                  NULL);
                    if (value==NULL)
                        return FALSE;
                    need_decref = 1;
                }
                if (!PyString_Check(value))
                    return FALSE;
                *retDataSize = 1 + strlen(
                    PyString_AS_STRING(
                        (PyStringObject *)value));
            }
            *retDataBuf = (BYTE *)PyMem_NEW(DWORD, *retDataSize);
            if (*retDataBuf==NULL){
                PyErr_NoMemory();
                return FALSE;
            }
            if (value == Py_None)
                strcpy((char *)*retDataBuf, "");
            else
                strcpy((char *)*retDataBuf,
                       PyString_AS_STRING(
                                (PyStringObject *)value));
            if (need_decref)
                Py_DECREF(value);
            break;
            }
        case REG_MULTI_SZ:
            {
                DWORD size = 0;
                char *P;
                PyObject **obs = NULL;

                if (value == Py_None)
                    i = 0;
                else {
                    if (!PyList_Check(value))
                        return FALSE;
                    i = PyList_Size(value);
                }
                obs = malloc(sizeof(PyObject *) * i);
                memset(obs, 0, sizeof(PyObject *) * i);
                for (j = 0; j < i; j++)
                {
                    PyObject *t;
                    t = PyList_GET_ITEM(
                        (PyListObject *)value,j);
                    if (PyString_Check(t)) {
                        obs[j] = t;
                        Py_INCREF(t);
                    } else if (PyUnicode_Check(t)) {
                        obs[j] = PyUnicode_AsEncodedString(
                                        t,
                                        "mbcs",
                                        NULL);
                        if (obs[j]==NULL)
                            goto reg_multi_fail;
                    } else
                        goto reg_multi_fail;
                    size += 1 + strlen(
                        PyString_AS_STRING(
                            (PyStringObject *)obs[j]));
                }

                *retDataSize = size + 1;
                *retDataBuf = (BYTE *)PyMem_NEW(char,
                                                *retDataSize);
                if (*retDataBuf==NULL){
                    PyErr_NoMemory();
                    goto reg_multi_fail;
                }
                P = (char *)*retDataBuf;

                for (j = 0; j < i; j++)
                {
                    PyObject *t;
                    t = obs[j];
                    strcpy(P,
                           PyString_AS_STRING(
                                    (PyStringObject *)t));
                    P += 1 + strlen(
                        PyString_AS_STRING(
                            (PyStringObject *)t));
                    Py_DECREF(obs[j]);
                }
                /* And doubly-terminate the list... */
                *P = '\0';
                free(obs);
                break;
            reg_multi_fail:
                if (obs) {
                    for (j = 0; j < i; j++)
                        Py_XDECREF(obs[j]);

                    free(obs);
                }
                return FALSE;
            }
        case REG_BINARY:
        /* ALSO handle ALL unknown data types here.  Even if we can't
           support it natively, we should handle the bits. */
        default:
            if (value == Py_None) {
                *retDataSize = 0;
                *retDataBuf = NULL;
            }
            else {
                void *src_buf;
                PyBufferProcs *pb = value->ob_type->tp_as_buffer;
                if (pb == NULL || pb->bf_getreadbuffer == NULL) {
                    PyErr_Format(PyExc_TypeError,
                        "Objects of type '%s' can not "
                        "be used as binary registry values",
                        value->ob_type->tp_name);
                    return FALSE;
                }
                *retDataSize = (*pb->bf_getreadbuffer)(value, 0, &src_buf);
                if (*retDataSize < 0) {
                    return FALSE;
                }
                *retDataBuf = (BYTE *)PyMem_NEW(char, *retDataSize);
                if (*retDataBuf == NULL){
                    PyErr_NoMemory();
                    return FALSE;
                }
                memcpy(*retDataBuf, src_buf, *retDataSize);
            }
            break;
    }
    return TRUE;
}

/* Convert Registry data into PyObject*/
static PyObject *
Reg2Py(char *retDataBuf, DWORD retDataSize, DWORD typ)
{
    PyObject *obData;

    switch (typ) {
        case REG_DWORD:
            if (retDataSize == 0)
                obData = Py_BuildValue("k", 0);
            else
                obData = Py_BuildValue("k",
                                       *(int *)retDataBuf);
            break;
        case REG_SZ:
        case REG_EXPAND_SZ:
            /* retDataBuf may or may not have a trailing NULL in
               the buffer. */
            if (retDataSize && retDataBuf[retDataSize-1] == '\0')
                --retDataSize;
            if (retDataSize ==0)
                retDataBuf = "";
            obData = PyUnicode_DecodeMBCS(retDataBuf,
                                          retDataSize,
                                          NULL);
            break;
        case REG_MULTI_SZ:
            if (retDataSize == 0)
                obData = PyList_New(0);
            else
            {
                int index = 0;
                int s = countStrings(retDataBuf, retDataSize);
                char **str = (char **)malloc(sizeof(char *)*s);
                if (str == NULL)
                    return PyErr_NoMemory();

                fixupMultiSZ(str, retDataBuf, retDataSize);
                obData = PyList_New(s);
                if (obData == NULL) {
                    free(str);
                    return NULL;
                }
                for (index = 0; index < s; index++)
                {
                    size_t len = _mbstrlen(str[index]);
                    if (len > INT_MAX) {
                        PyErr_SetString(PyExc_OverflowError,
                            "registry string is too long for a Python string");
                        Py_DECREF(obData);
                        free(str);
                        return NULL;
                    }
                    PyList_SetItem(obData,
                                   index,
                                   PyUnicode_DecodeMBCS(
                                        (const char *)str[index],
                                       (int)len,
                                        NULL)
                                   );
                }
                free(str);

                break;
            }
        case REG_BINARY:
        /* ALSO handle ALL unknown data types here.  Even if we can't
           support it natively, we should handle the bits. */
        default:
            if (retDataSize == 0) {
                Py_INCREF(Py_None);
                obData = Py_None;
            }
            else
                obData = Py_BuildValue("s#",
                                       (char *)retDataBuf,
                                       retDataSize);
            break;
    }
    if (obData == NULL)
        return NULL;
    else
        return obData;
}

/* The Python methods */

static PyObject *
PyCloseKey(PyObject *self, PyObject *args)
{
    PyObject *obKey;
    if (!PyArg_ParseTuple(args, "O:CloseKey", &obKey))
        return NULL;
    if (!PyHKEY_Close(obKey))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyConnectRegistry(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *szCompName = NULL;
    HKEY retKey;
    long rc;
    if (!PyArg_ParseTuple(args, "zO:ConnectRegistry", &szCompName, &obKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    rc = RegConnectRegistry(szCompName, hKey, &retKey);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "ConnectRegistry");
    return PyHKEY_FromHKEY(retKey);
}

static PyObject *
PyCreateKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *subKey;
    HKEY retKey;
    long rc;
    if (!PyArg_ParseTuple(args, "Oz:CreateKey", &obKey, &subKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    rc = RegCreateKey(hKey, subKey, &retKey);
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "CreateKey");
    return PyHKEY_FromHKEY(retKey);
}

static PyObject *
PyCreateKeyEx(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *subKey;
    HKEY retKey;
    int res = 0;
    REGSAM sam = KEY_WRITE;
    long rc;
    if (!PyArg_ParseTuple(args, "Oz|ii:CreateKeyEx", &obKey, &subKey,
                                              &res, &sam))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    rc = RegCreateKeyEx(hKey, subKey, res, NULL, (DWORD)NULL,
                                            sam, NULL, &retKey, NULL);
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "CreateKeyEx");
    return PyHKEY_FromHKEY(retKey);
}

static PyObject *
PyDeleteKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *subKey;
    long rc;
    if (!PyArg_ParseTuple(args, "Os:DeleteKey", &obKey, &subKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    rc = RegDeleteKey(hKey, subKey );
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegDeleteKey");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDeleteKeyEx(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    HMODULE hMod;
    typedef LONG (WINAPI *RDKEFunc)(HKEY, const char*, REGSAM, int);
    RDKEFunc pfn = NULL;
    char *subKey;
    long rc;
    int res = 0;
    REGSAM sam = KEY_WOW64_64KEY;

    if (!PyArg_ParseTuple(args, "Os|ii:DeleteKeyEx",
                                              &obKey, &subKey, &sam, &res))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    /* Only available on 64bit platforms, so we must load it
       dynamically. */
    hMod = GetModuleHandle("advapi32.dll");
    if (hMod)
        pfn = (RDKEFunc)GetProcAddress(hMod,
                                                                   "RegDeleteKeyExA");
    if (!pfn) {
        PyErr_SetString(PyExc_NotImplementedError,
                                        "not implemented on this platform");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = (*pfn)(hKey, subKey, sam, res);
    Py_END_ALLOW_THREADS

    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegDeleteKeyEx");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDeleteValue(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *subKey;
    long rc;
    if (!PyArg_ParseTuple(args, "Oz:DeleteValue", &obKey, &subKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    rc = RegDeleteValue(hKey, subKey);
    Py_END_ALLOW_THREADS
    if (rc !=ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegDeleteValue");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyEnumKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    int index;
    long rc;
    PyObject *retStr;

    /* The Windows docs claim that the max key name length is 255
     * characters, plus a terminating nul character.  However,
     * empirical testing demonstrates that it is possible to
     * create a 256 character key that is missing the terminating
     * nul.  RegEnumKeyEx requires a 257 character buffer to
     * retrieve such a key name. */
    char tmpbuf[257];
    DWORD len = sizeof(tmpbuf); /* includes NULL terminator */

    if (!PyArg_ParseTuple(args, "Oi:EnumKey", &obKey, &index))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    rc = RegEnumKeyEx(hKey, index, tmpbuf, &len, NULL, NULL, NULL, NULL);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegEnumKeyEx");

    retStr = PyString_FromStringAndSize(tmpbuf, len);
    return retStr;  /* can be NULL */
}

static PyObject *
PyEnumValue(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    int index;
    long rc;
    char *retValueBuf;
    char *retDataBuf;
    char *tmpBuf;
    DWORD retValueSize, bufValueSize;
    DWORD retDataSize, bufDataSize;
    DWORD typ;
    PyObject *obData;
    PyObject *retVal;

    if (!PyArg_ParseTuple(args, "Oi:EnumValue", &obKey, &index))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    if ((rc = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                              NULL,
                              &retValueSize, &retDataSize, NULL, NULL))
        != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryInfoKey");
    ++retValueSize;    /* include null terminators */
    ++retDataSize;
    bufDataSize = retDataSize;
    bufValueSize = retValueSize;
    retValueBuf = (char *)PyMem_Malloc(retValueSize);
    if (retValueBuf == NULL)
        return PyErr_NoMemory();
    retDataBuf = (char *)PyMem_Malloc(retDataSize);
    if (retDataBuf == NULL) {
        PyMem_Free(retValueBuf);
        return PyErr_NoMemory();
    }

    while (1) {
        Py_BEGIN_ALLOW_THREADS
        rc = RegEnumValue(hKey,
                  index,
                  retValueBuf,
                  &retValueSize,
                  NULL,
                  &typ,
                  (BYTE *)retDataBuf,
                  &retDataSize);
        Py_END_ALLOW_THREADS

        if (rc != ERROR_MORE_DATA)
            break;

        bufDataSize *= 2;
        tmpBuf = (char *)PyMem_Realloc(retDataBuf, bufDataSize);
        if (tmpBuf == NULL) {
            PyErr_NoMemory();
            retVal = NULL;
            goto fail;
        }
        retDataBuf = tmpBuf;
        retDataSize = bufDataSize;
        retValueSize = bufValueSize;
    }

    if (rc != ERROR_SUCCESS) {
        retVal = PyErr_SetFromWindowsErrWithFunction(rc,
                                                     "PyRegEnumValue");
        goto fail;
    }
    obData = Reg2Py(retDataBuf, retDataSize, typ);
    if (obData == NULL) {
        retVal = NULL;
        goto fail;
    }
    retVal = Py_BuildValue("sOi", retValueBuf, obData, typ);
    Py_DECREF(obData);
  fail:
    PyMem_Free(retValueBuf);
    PyMem_Free(retDataBuf);
    return retVal;
}

static PyObject *
PyExpandEnvironmentStrings(PyObject *self, PyObject *args)
{
    Py_UNICODE *retValue = NULL;
    Py_UNICODE *src;
    DWORD retValueSize;
    DWORD rc;
    PyObject *o;

    if (!PyArg_ParseTuple(args, "u:ExpandEnvironmentStrings", &src))
        return NULL;

    retValueSize = ExpandEnvironmentStringsW(src, retValue, 0);
    if (retValueSize == 0) {
        return PyErr_SetFromWindowsErrWithFunction(retValueSize,
                                        "ExpandEnvironmentStrings");
    }
    retValue = (Py_UNICODE *)PyMem_Malloc(retValueSize * sizeof(Py_UNICODE));
    if (retValue == NULL) {
        return PyErr_NoMemory();
    }

    rc = ExpandEnvironmentStringsW(src, retValue, retValueSize);
    if (rc == 0) {
        PyMem_Free(retValue);
        return PyErr_SetFromWindowsErrWithFunction(retValueSize,
                                        "ExpandEnvironmentStrings");
    }
    o = PyUnicode_FromUnicode(retValue, wcslen(retValue));
    PyMem_Free(retValue);
    return o;
}

static PyObject *
PyFlushKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    long rc;
    if (!PyArg_ParseTuple(args, "O:FlushKey", &obKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    rc = RegFlushKey(hKey);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegFlushKey");
    Py_INCREF(Py_None);
    return Py_None;
}
static PyObject *
PyLoadKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *subKey;
    char *fileName;

    long rc;
    if (!PyArg_ParseTuple(args, "Oss:LoadKey", &obKey, &subKey, &fileName))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    rc = RegLoadKey(hKey, subKey, fileName );
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegLoadKey");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyOpenKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;

    char *subKey;
    int res = 0;
    HKEY retKey;
    long rc;
    REGSAM sam = KEY_READ;
    if (!PyArg_ParseTuple(args, "Oz|ii:OpenKey", &obKey, &subKey,
                          &res, &sam))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    rc = RegOpenKeyEx(hKey, subKey, res, sam, &retKey);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegOpenKeyEx");
    return PyHKEY_FromHKEY(retKey);
}


static PyObject *
PyQueryInfoKey(PyObject *self, PyObject *args)
{
  HKEY hKey;
  PyObject *obKey;
  long rc;
  DWORD nSubKeys, nValues;
  FILETIME ft;
  LARGE_INTEGER li;
  PyObject *l;
  PyObject *ret;
  if (!PyArg_ParseTuple(args, "O:QueryInfoKey", &obKey))
    return NULL;
  if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
    return NULL;
  if ((rc = RegQueryInfoKey(hKey, NULL, NULL, 0, &nSubKeys, NULL, NULL,
                            &nValues,  NULL,  NULL, NULL, &ft))
      != ERROR_SUCCESS)
    return PyErr_SetFromWindowsErrWithFunction(rc, "RegQueryInfoKey");
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  l = PyLong_FromLongLong(li.QuadPart);
  if (l == NULL)
    return NULL;
  ret = Py_BuildValue("iiO", nSubKeys, nValues, l);
  Py_DECREF(l);
  return ret;
}

static PyObject *
PyQueryValue(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *subKey;
    long rc;
    PyObject *retStr;
    char *retBuf;
    DWORD bufSize = 0;
    DWORD retSize = 0;
    char *tmp;

    if (!PyArg_ParseTuple(args, "Oz:QueryValue", &obKey, &subKey))
        return NULL;

    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
 
    rc = RegQueryValue(hKey, subKey, NULL, &retSize);
    if (rc == ERROR_MORE_DATA)
        retSize = 256;
    else if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryValue");

    bufSize = retSize;
    retBuf = (char *) PyMem_Malloc(bufSize);
    if (retBuf == NULL)
        return PyErr_NoMemory();

    while (1) {
        retSize = bufSize;
        rc = RegQueryValue(hKey, subKey, retBuf, &retSize);
        if (rc != ERROR_MORE_DATA)
            break;

        bufSize *= 2;
        tmp = (char *) PyMem_Realloc(retBuf, bufSize);
        if (tmp == NULL) {
            PyMem_Free(retBuf);
            return PyErr_NoMemory();
        }
        retBuf = tmp;
    }

    if (rc != ERROR_SUCCESS) {
        PyMem_Free(retBuf);
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryValue");
    }

    if (retBuf[retSize-1] == '\x00')
        retSize--;
    retStr = PyString_FromStringAndSize(retBuf, retSize);
    if (retStr == NULL) {
        PyMem_Free(retBuf);
        return NULL;
    }
    return retStr;
}

static PyObject *
PyQueryValueEx(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *valueName;

    long rc;
    char *retBuf, *tmp;
    DWORD bufSize = 0, retSize;
    DWORD typ;
    PyObject *obData;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "Oz:QueryValueEx", &obKey, &valueName))
        return NULL;

    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    rc = RegQueryValueEx(hKey, valueName, NULL, NULL, NULL, &bufSize);
    if (rc == ERROR_MORE_DATA)
        bufSize = 256;
    else if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryValueEx");
    retBuf = (char *)PyMem_Malloc(bufSize);
    if (retBuf == NULL)
        return PyErr_NoMemory();

    while (1) {
        retSize = bufSize;
        rc = RegQueryValueEx(hKey, valueName, NULL, &typ,
                             (BYTE *)retBuf, &retSize);
        if (rc != ERROR_MORE_DATA)
            break;

        bufSize *= 2;
        tmp = (char *) PyMem_Realloc(retBuf, bufSize);
        if (tmp == NULL) {
            PyMem_Free(retBuf);
            return PyErr_NoMemory();
        }
        retBuf = tmp;
    }

    if (rc != ERROR_SUCCESS) {
        PyMem_Free(retBuf);
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryValueEx");
    }
    obData = Reg2Py(retBuf, bufSize, typ);
    PyMem_Free((void *)retBuf);
    if (obData == NULL)
        return NULL;
    result = Py_BuildValue("Oi", obData, typ);
    Py_DECREF(obData);
    return result;
}


static PyObject *
PySaveKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *fileName;
    LPSECURITY_ATTRIBUTES pSA = NULL;

    long rc;
    if (!PyArg_ParseTuple(args, "Os:SaveKey", &obKey, &fileName))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
/*  One day we may get security into the core?
    if (!PyWinObject_AsSECURITY_ATTRIBUTES(obSA, &pSA, TRUE))
        return NULL;
*/
    Py_BEGIN_ALLOW_THREADS
    rc = RegSaveKey(hKey, fileName, pSA );
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegSaveKey");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PySetValue(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *subKey;
    char *str;
    DWORD typ;
    DWORD len;
    long rc;
    PyObject *obStrVal;
    PyObject *obSubKey;
    if (!PyArg_ParseTuple(args, "OOiO:SetValue",
                          &obKey,
                          &obSubKey,
                          &typ,
                          &obStrVal))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    if (typ != REG_SZ) {
        PyErr_SetString(PyExc_TypeError,
                        "Type must be _winreg.REG_SZ");
        return NULL;
    }
    /* XXX - need Unicode support */
    str = PyString_AsString(obStrVal);
    if (str == NULL)
        return NULL;
    len = PyString_Size(obStrVal);
    if (obSubKey == Py_None)
        subKey = NULL;
    else {
        subKey = PyString_AsString(obSubKey);
        if (subKey == NULL)
            return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegSetValue(hKey, subKey, REG_SZ, str, len+1);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc, "RegSetValue");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PySetValueEx(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    char *valueName;
    PyObject *obRes;
    PyObject *value;
    BYTE *data;
    DWORD len;
    DWORD typ;

    LONG rc;

    if (!PyArg_ParseTuple(args, "OzOiO:SetValueEx",
                          &obKey,
                          &valueName,
                          &obRes,
                          &typ,
                          &value))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;
    if (!Py2Reg(value, typ, &data, &len))
    {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_ValueError,
                     "Could not convert the data to the specified type.");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = RegSetValueEx(hKey, valueName, 0, typ, data, len);
    Py_END_ALLOW_THREADS
    PyMem_DEL(data);
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegSetValueEx");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyDisableReflectionKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    HMODULE hMod;
    typedef LONG (WINAPI *RDRKFunc)(HKEY);
    RDRKFunc pfn = NULL;
    LONG rc;

    if (!PyArg_ParseTuple(args, "O:DisableReflectionKey", &obKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    /* Only available on 64bit platforms, so we must load it
       dynamically. */
    hMod = GetModuleHandle("advapi32.dll");
    if (hMod)
        pfn = (RDRKFunc)GetProcAddress(hMod,
                                       "RegDisableReflectionKey");
    if (!pfn) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "not implemented on this platform");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = (*pfn)(hKey);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegDisableReflectionKey");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyEnableReflectionKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    HMODULE hMod;
    typedef LONG (WINAPI *RERKFunc)(HKEY);
    RERKFunc pfn = NULL;
    LONG rc;

    if (!PyArg_ParseTuple(args, "O:EnableReflectionKey", &obKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    /* Only available on 64bit platforms, so we must load it
       dynamically. */
    hMod = GetModuleHandle("advapi32.dll");
    if (hMod)
        pfn = (RERKFunc)GetProcAddress(hMod,
                                       "RegEnableReflectionKey");
    if (!pfn) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "not implemented on this platform");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = (*pfn)(hKey);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegEnableReflectionKey");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyQueryReflectionKey(PyObject *self, PyObject *args)
{
    HKEY hKey;
    PyObject *obKey;
    HMODULE hMod;
    typedef LONG (WINAPI *RQRKFunc)(HKEY, BOOL *);
    RQRKFunc pfn = NULL;
    BOOL result;
    LONG rc;

    if (!PyArg_ParseTuple(args, "O:QueryReflectionKey", &obKey))
        return NULL;
    if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
        return NULL;

    /* Only available on 64bit platforms, so we must load it
       dynamically. */
    hMod = GetModuleHandle("advapi32.dll");
    if (hMod)
        pfn = (RQRKFunc)GetProcAddress(hMod,
                                       "RegQueryReflectionKey");
    if (!pfn) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "not implemented on this platform");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rc = (*pfn)(hKey, &result);
    Py_END_ALLOW_THREADS
    if (rc != ERROR_SUCCESS)
        return PyErr_SetFromWindowsErrWithFunction(rc,
                                                   "RegQueryReflectionKey");
    return PyBool_FromLong(result);
}

static struct PyMethodDef winreg_methods[] = {
    {"CloseKey",         PyCloseKey,        METH_VARARGS, CloseKey_doc},
    {"ConnectRegistry",  PyConnectRegistry, METH_VARARGS, ConnectRegistry_doc},
    {"CreateKey",        PyCreateKey,       METH_VARARGS, CreateKey_doc},
    {"CreateKeyEx",      PyCreateKeyEx,     METH_VARARGS, CreateKeyEx_doc},
    {"DeleteKey",        PyDeleteKey,       METH_VARARGS, DeleteKey_doc},
    {"DeleteKeyEx",      PyDeleteKeyEx,     METH_VARARGS, DeleteKeyEx_doc},
    {"DeleteValue",      PyDeleteValue,     METH_VARARGS, DeleteValue_doc},
    {"DisableReflectionKey", PyDisableReflectionKey, METH_VARARGS, DisableReflectionKey_doc},
    {"EnableReflectionKey",  PyEnableReflectionKey,  METH_VARARGS, EnableReflectionKey_doc},
    {"EnumKey",          PyEnumKey,         METH_VARARGS, EnumKey_doc},
    {"EnumValue",        PyEnumValue,       METH_VARARGS, EnumValue_doc},
    {"ExpandEnvironmentStrings", PyExpandEnvironmentStrings, METH_VARARGS,
        ExpandEnvironmentStrings_doc },
    {"FlushKey",         PyFlushKey,        METH_VARARGS, FlushKey_doc},
    {"LoadKey",          PyLoadKey,         METH_VARARGS, LoadKey_doc},
    {"OpenKey",          PyOpenKey,         METH_VARARGS, OpenKey_doc},
    {"OpenKeyEx",        PyOpenKey,         METH_VARARGS, OpenKeyEx_doc},
    {"QueryValue",       PyQueryValue,      METH_VARARGS, QueryValue_doc},
    {"QueryValueEx",     PyQueryValueEx,    METH_VARARGS, QueryValueEx_doc},
    {"QueryInfoKey",     PyQueryInfoKey,    METH_VARARGS, QueryInfoKey_doc},
    {"QueryReflectionKey",PyQueryReflectionKey,METH_VARARGS, QueryReflectionKey_doc},
    {"SaveKey",          PySaveKey,         METH_VARARGS, SaveKey_doc},
    {"SetValue",         PySetValue,        METH_VARARGS, SetValue_doc},
    {"SetValueEx",       PySetValueEx,      METH_VARARGS, SetValueEx_doc},
    NULL,
};

static void
insint(PyObject * d, char * name, long value)
{
    PyObject *v = PyInt_FromLong(value);
    if (!v || PyDict_SetItemString(d, name, v))
        PyErr_Clear();
    Py_XDECREF(v);
}

#define ADD_INT(val) insint(d, #val, val)

static void
inskey(PyObject * d, char * name, HKEY key)
{
    PyObject *v = PyLong_FromVoidPtr(key);
    if (!v || PyDict_SetItemString(d, name, v))
        PyErr_Clear();
    Py_XDECREF(v);
}

#define ADD_KEY(val) inskey(d, #val, val)

PyMODINIT_FUNC init_winreg(void)
{
    PyObject *m, *d;
    m = Py_InitModule3("_winreg", winreg_methods, module_doc);
    if (m == NULL)
        return;
    d = PyModule_GetDict(m);
    if (PyType_Ready(&PyHKEY_Type) < 0)
        return;
    PyHKEY_Type.tp_doc = PyHKEY_doc;
    Py_INCREF(&PyHKEY_Type);
    if (PyDict_SetItemString(d, "HKEYType",
                             (PyObject *)&PyHKEY_Type) != 0)
        return;
    Py_INCREF(PyExc_WindowsError);
    if (PyDict_SetItemString(d, "error",
                             PyExc_WindowsError) != 0)
        return;

    /* Add the relevant constants */
    ADD_KEY(HKEY_CLASSES_ROOT);
    ADD_KEY(HKEY_CURRENT_USER);
    ADD_KEY(HKEY_LOCAL_MACHINE);
    ADD_KEY(HKEY_USERS);
    ADD_KEY(HKEY_PERFORMANCE_DATA);
#ifdef HKEY_CURRENT_CONFIG
    ADD_KEY(HKEY_CURRENT_CONFIG);
#endif
#ifdef HKEY_DYN_DATA
    ADD_KEY(HKEY_DYN_DATA);
#endif
    ADD_INT(KEY_QUERY_VALUE);
    ADD_INT(KEY_SET_VALUE);
    ADD_INT(KEY_CREATE_SUB_KEY);
    ADD_INT(KEY_ENUMERATE_SUB_KEYS);
    ADD_INT(KEY_NOTIFY);
    ADD_INT(KEY_CREATE_LINK);
    ADD_INT(KEY_READ);
    ADD_INT(KEY_WRITE);
    ADD_INT(KEY_EXECUTE);
    ADD_INT(KEY_ALL_ACCESS);
#ifdef KEY_WOW64_64KEY
    ADD_INT(KEY_WOW64_64KEY);
#endif
#ifdef KEY_WOW64_32KEY
    ADD_INT(KEY_WOW64_32KEY);
#endif
    ADD_INT(REG_OPTION_RESERVED);
    ADD_INT(REG_OPTION_NON_VOLATILE);
    ADD_INT(REG_OPTION_VOLATILE);
    ADD_INT(REG_OPTION_CREATE_LINK);
    ADD_INT(REG_OPTION_BACKUP_RESTORE);
    ADD_INT(REG_OPTION_OPEN_LINK);
    ADD_INT(REG_LEGAL_OPTION);
    ADD_INT(REG_CREATED_NEW_KEY);
    ADD_INT(REG_OPENED_EXISTING_KEY);
    ADD_INT(REG_WHOLE_HIVE_VOLATILE);
    ADD_INT(REG_REFRESH_HIVE);
    ADD_INT(REG_NO_LAZY_FLUSH);
    ADD_INT(REG_NOTIFY_CHANGE_NAME);
    ADD_INT(REG_NOTIFY_CHANGE_ATTRIBUTES);
    ADD_INT(REG_NOTIFY_CHANGE_LAST_SET);
    ADD_INT(REG_NOTIFY_CHANGE_SECURITY);
    ADD_INT(REG_LEGAL_CHANGE_FILTER);
    ADD_INT(REG_NONE);
    ADD_INT(REG_SZ);
    ADD_INT(REG_EXPAND_SZ);
    ADD_INT(REG_BINARY);
    ADD_INT(REG_DWORD);
    ADD_INT(REG_DWORD_LITTLE_ENDIAN);
    ADD_INT(REG_DWORD_BIG_ENDIAN);
    ADD_INT(REG_LINK);
    ADD_INT(REG_MULTI_SZ);
    ADD_INT(REG_RESOURCE_LIST);
    ADD_INT(REG_FULL_RESOURCE_DESCRIPTOR);
    ADD_INT(REG_RESOURCE_REQUIREMENTS_LIST);
}

