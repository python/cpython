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
#include "windows.h"

static BOOL PyHKEY_AsHKEY(PyObject *ob, HKEY *pRes, BOOL bNoneOK);
static PyObject *PyHKEY_FromHKEY(HKEY h);
static BOOL PyHKEY_Close(PyObject *obHandle);

static char errNotAHandle[] = "Object is not a handle";

/* The win32api module reports the function name that failed,
   but this concept is not in the Python core.
   Hopefully it will one day, and in the meantime I dont
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
"If the function fails, an EnvironmentError exception is raised.");

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
"is removed.  If the method fails, an EnvironmentError exception is raised.");

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
"It is typically called repeatedly until an EnvironmentError exception is\n"
"raised, indicating no more values are available.");

PyDoc_STRVAR(EnumValue_doc,
"tuple = EnumValue(key, index) - Enumerates values of an open registry key.\n"
"key is an already open key, or any one of the predefined HKEY_* constants.\n"
"index is an integer that identifies the index of the value to retrieve.\n"
"\n"
"The function retrieves the name of one subkey each time it is called.\n"
"It is typically called repeatedly, until an EnvironmentError exception\n"
"is raised, indicating no more values.\n"
"\n"
"The result is a tuple of 3 items:\n"
"value_name is a string that identifies the value.\n"
"value_data is an object that holds the value data, and whose type depends\n"
" on the underlying registry type.\n"
"data_type is an integer that identifies the type of the value data.");

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
"If the function fails, an EnvironmentError exception is raised.");

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
"  REG_MULTI_SZ -- An sequence of null-terminated strings, terminated by\n"
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
"__bool__ - Handles with an open object return true, otherwise false.\n"
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
PyHKEY_boolFunc(PyObject *ob)
{
	return ((PyHKEYObject *)ob)->hkey != 0;
}

static PyObject *
PyHKEY_intFunc(PyObject *ob)
{
	PyHKEYObject *pyhkey = (PyHKEYObject *)ob;
	return PyLong_FromVoidPtr(pyhkey->hkey);
}

static PyObject *
PyHKEY_strFunc(PyObject *ob)
{
	PyHKEYObject *pyhkey = (PyHKEYObject *)ob;
	return PyUnicode_FromFormat("<PyHKEY:%p>", pyhkey->hkey);
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
	PyHKEY_binaryFailureFunc,	/* nb_add */
	PyHKEY_binaryFailureFunc,	/* nb_subtract */
	PyHKEY_binaryFailureFunc,	/* nb_multiply */
	PyHKEY_binaryFailureFunc,	/* nb_remainder */
	PyHKEY_binaryFailureFunc,	/* nb_divmod */
	PyHKEY_ternaryFailureFunc,	/* nb_power */
	PyHKEY_unaryFailureFunc,	/* nb_negative */
	PyHKEY_unaryFailureFunc,	/* nb_positive */
	PyHKEY_unaryFailureFunc,	/* nb_absolute */
	PyHKEY_boolFunc,		/* nb_bool */
	PyHKEY_unaryFailureFunc,	/* nb_invert */
	PyHKEY_binaryFailureFunc,	/* nb_lshift */
	PyHKEY_binaryFailureFunc,	/* nb_rshift */
	PyHKEY_binaryFailureFunc,	/* nb_and */
	PyHKEY_binaryFailureFunc,	/* nb_xor */
	PyHKEY_binaryFailureFunc,	/* nb_or */
	0,				/* nb_reserved */
	PyHKEY_intFunc,			/* nb_int */
	PyHKEY_unaryFailureFunc,	/* nb_long */
	PyHKEY_unaryFailureFunc,	/* nb_float */
	PyHKEY_unaryFailureFunc,	/* nb_oct */
	PyHKEY_unaryFailureFunc,	/* nb_hex */
};


/* fwd declare __getattr__ */
static PyObject *PyHKEY_getattr(PyObject *self, const char *name);

/* The type itself */
PyTypeObject PyHKEY_Type =
{
	PyVarObject_HEAD_INIT(0, 0) /* fill in type at module init */
	"PyHKEY",
	sizeof(PyHKEYObject),
	0,
	PyHKEY_deallocFunc,		/* tp_dealloc */
	0,				/* tp_print */
	PyHKEY_getattr,			/* tp_getattr */
	0,				/* tp_setattr */
	PyHKEY_compareFunc,		/* tp_compare */
	0,				/* tp_repr */
	&PyHKEY_NumberMethods,		/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	PyHKEY_hashFunc,		/* tp_hash */
	0,				/* tp_call */
	PyHKEY_strFunc,			/* tp_str */
	0,				/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	0,				/* tp_flags */
	PyHKEY_doc,			/* tp_doc */
};

#define OFF(e) offsetof(PyHKEYObject, e)

static PyMemberDef PyHKEY_memberlist[] = {
	{"handle",      T_INT,      OFF(hkey), READONLY},
	{NULL}    /* Sentinel */
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

static struct PyMethodDef PyHKEY_methods[] = {
	{"Close",  PyHKEY_CloseMethod, METH_VARARGS, PyHKEY_Close_doc},
	{"Detach", PyHKEY_DetachMethod, METH_VARARGS, PyHKEY_Detach_doc},
	{NULL}
};

/*static*/ PyObject *
PyHKEY_getattr(PyObject *self, const char *name)
{
	PyObject *res;

	res = Py_FindMethod(PyHKEY_methods, self, name);
	if (res != NULL)
		return res;
	PyErr_Clear();
	if (strcmp(name, "handle") == 0)
		return PyLong_FromVoidPtr(((PyHKEYObject *)self)->hkey);
	PyErr_Format(PyExc_AttributeError,
                     "'%.50s' object has no attribute '%.400s'",
                     Py_TYPE(self)->tp_name, name);
	return NULL;
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
	else if (PyLong_Check(ob)) {
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
	else if (PyLong_Check(obHandle)) {
		long rc = RegCloseKey((HKEY)PyLong_AsLong(obHandle));
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
** some commercial packages install strings which dont conform,
** causing this code to fail - however, "regedit" etc still work
** with these strings (ie only we dont!).
*/
static void
fixupMultiSZ(wchar_t **str, wchar_t *data, int len)
{
	wchar_t *P;
	int i;
	wchar_t *Q;

	Q = data + len;
	for (P = data, i = 0; P < Q && *P != '\0'; P++, i++) {
		str[i] = P;
		for(; *P != '\0'; P++)
			;
	}
}

static int
countStrings(wchar_t *data, int len)
{
	int strings;
	wchar_t *P;
	wchar_t *Q = data + len;

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
	switch (typ) {
		case REG_DWORD:
			if (value != Py_None && !PyLong_Check(value))
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
				DWORD d = PyLong_AsLong(value);
				memcpy(*retDataBuf, &d, sizeof(DWORD));
			}
			break;
		case REG_SZ:
		case REG_EXPAND_SZ:
			{
			if (value == Py_None)
				*retDataSize = 1;
			else {
				if (!PyUnicode_Check(value))
					return FALSE;

				*retDataSize = 2 + PyUnicode_GET_DATA_SIZE(value);
			}
			*retDataBuf = (BYTE *)PyMem_NEW(DWORD, *retDataSize);
			if (*retDataBuf==NULL){
				PyErr_NoMemory();
				return FALSE;
			}
			if (value == Py_None)
				wcscpy((wchar_t *)*retDataBuf, L"");
			else
				wcscpy((wchar_t *)*retDataBuf,
				       PyUnicode_AS_UNICODE(value));
			break;
			}
		case REG_MULTI_SZ:
			{
				DWORD size = 0;
				wchar_t *P;
				int i,j;

				if (value == Py_None)
					i = 0;
				else {
					if (!PyList_Check(value))
						return FALSE;
					i = PyList_Size(value);
				}
				for (j = 0; j < i; j++)
				{
					PyObject *t;
					t = PyList_GET_ITEM(value, j);
					if (!PyUnicode_Check(t))
						return FALSE;
					size += 2 + PyUnicode_GET_DATA_SIZE(t);
				}

				*retDataSize = size + 2;
				*retDataBuf = (BYTE *)PyMem_NEW(char,
							        *retDataSize);
				if (*retDataBuf==NULL){
					PyErr_NoMemory();
					return FALSE;
				}
				P = (wchar_t *)*retDataBuf;

				for (j = 0; j < i; j++)
				{
					PyObject *t;
					t = PyList_GET_ITEM(value, j);
					wcscpy(P, PyUnicode_AS_UNICODE(t));
					P += 1 + wcslen(
						PyUnicode_AS_UNICODE(t));
				}
				/* And doubly-terminate the list... */
				*P = '\0';
				break;
			}
		case REG_BINARY:
		/* ALSO handle ALL unknown data types here.  Even if we can't
		   support it natively, we should handle the bits. */
		default:
			if (value == Py_None)
				*retDataSize = 0;
			else {
				Py_buffer view;

				if (!PyObject_CheckBuffer(value)) {
					PyErr_Format(PyExc_TypeError,
						"Objects of type '%s' can not "
						"be used as binary registry values",
						value->ob_type->tp_name);
					return FALSE;
				}

				if (PyObject_GetBuffer(value, &view, PyBUF_SIMPLE) < 0)
					return FALSE;

				*retDataBuf = (BYTE *)PyMem_NEW(char, view.len);
				if (*retDataBuf==NULL){
					PyObject_ReleaseBuffer(value, &view);
					PyErr_NoMemory();
					return FALSE;
				}
				*retDataSize = view.len;
				memcpy(*retDataBuf, view.buf, view.len);
				PyObject_ReleaseBuffer(value, &view);
			}
			break;
	}
	return TRUE;
}

/* Convert Registry data into PyObject*/
static PyObject *
Reg2Py(BYTE *retDataBuf, DWORD retDataSize, DWORD typ)
{
	PyObject *obData;

	switch (typ) {
		case REG_DWORD:
			if (retDataSize == 0)
				obData = PyLong_FromLong(0);
			else
				obData = PyLong_FromLong(*(int *)retDataBuf);
			break;
		case REG_SZ:
		case REG_EXPAND_SZ:
			{
				/* the buffer may or may not have a trailing NULL */
				wchar_t *data = (wchar_t *)retDataBuf;
				int len = retDataSize / 2;
				if (retDataSize && data[len-1] == '\0')
					retDataSize -= 2;
				if (retDataSize <= 0)
					data = L"";
				obData = PyUnicode_FromUnicode(data, retDataSize/2);
				break;
			}
		case REG_MULTI_SZ:
			if (retDataSize == 0)
				obData = PyList_New(0);
			else
			{
				int index = 0;
				wchar_t *data = (wchar_t *)retDataBuf;
				int len = retDataSize / 2;
				int s = countStrings(data, len);
				wchar_t **str = (wchar_t **)malloc(sizeof(wchar_t *)*s);
				if (str == NULL)
					return PyErr_NoMemory();

				fixupMultiSZ(str, data, len);
				obData = PyList_New(s);
				if (obData == NULL)
					return NULL;
				for (index = 0; index < s; index++)
				{
					size_t len = wcslen(str[index]);
					if (len > INT_MAX) {
						PyErr_SetString(PyExc_OverflowError,
							"registry string is too long for a Python string");
						Py_DECREF(obData);
						return NULL;
					}
					PyList_SetItem(obData,
						       index,
						       PyUnicode_FromUnicode(str[index], len));
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
				obData = PyBytes_FromStringAndSize(
					     (char *)retDataBuf, retDataSize);
			break;
	}
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
	wchar_t *szCompName = NULL;
	HKEY retKey;
	long rc;
	if (!PyArg_ParseTuple(args, "ZO:ConnectRegistry", &szCompName, &obKey))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	rc = RegConnectRegistryW(szCompName, hKey, &retKey);
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
	wchar_t *subKey;
	HKEY retKey;
	long rc;
	if (!PyArg_ParseTuple(args, "OZ:CreateKey", &obKey, &subKey))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	rc = RegCreateKeyW(hKey, subKey, &retKey);
	if (rc != ERROR_SUCCESS)
		return PyErr_SetFromWindowsErrWithFunction(rc, "CreateKey");
	return PyHKEY_FromHKEY(retKey);
}

static PyObject *
PyDeleteKey(PyObject *self, PyObject *args)
{
	HKEY hKey;
	PyObject *obKey;
	wchar_t *subKey;
	long rc;
	if (!PyArg_ParseTuple(args, "Ou:DeleteKey", &obKey, &subKey))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	rc = RegDeleteKeyW(hKey, subKey );
	if (rc != ERROR_SUCCESS)
		return PyErr_SetFromWindowsErrWithFunction(rc, "RegDeleteKey");
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
PyDeleteValue(PyObject *self, PyObject *args)
{
	HKEY hKey;
	PyObject *obKey;
	wchar_t *subKey;
	long rc;
	if (!PyArg_ParseTuple(args, "OZ:DeleteValue", &obKey, &subKey))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	rc = RegDeleteValueW(hKey, subKey);
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
	wchar_t tmpbuf[256]; /* max key name length is 255 */
	DWORD len = sizeof(tmpbuf); /* includes NULL terminator */

	if (!PyArg_ParseTuple(args, "Oi:EnumKey", &obKey, &index))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	rc = RegEnumKeyExW(hKey, index, tmpbuf, &len, NULL, NULL, NULL, NULL);
	Py_END_ALLOW_THREADS
	if (rc != ERROR_SUCCESS)
		return PyErr_SetFromWindowsErrWithFunction(rc, "RegEnumKeyEx");

	retStr = PyUnicode_FromUnicode(tmpbuf, len);
	return retStr;  /* can be NULL */
}

static PyObject *
PyEnumValue(PyObject *self, PyObject *args)
{
	HKEY hKey;
	PyObject *obKey;
	int index;
	long rc;
	wchar_t *retValueBuf;
	BYTE *retDataBuf;
	DWORD retValueSize;
	DWORD retDataSize;
	DWORD typ;
	PyObject *obData;
	PyObject *retVal;

	if (!PyArg_ParseTuple(args, "Oi:EnumValue", &obKey, &index))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;

	if ((rc = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
				  NULL,
				  &retValueSize, &retDataSize, NULL, NULL))
	    != ERROR_SUCCESS)
		return PyErr_SetFromWindowsErrWithFunction(rc,
							   "RegQueryInfoKey");
	++retValueSize;    /* include null terminators */
	++retDataSize;
	retValueBuf = (wchar_t *)PyMem_Malloc(sizeof(wchar_t) * retValueSize);
	if (retValueBuf == NULL)
		return PyErr_NoMemory();
	retDataBuf = (BYTE *)PyMem_Malloc(retDataSize);
	if (retDataBuf == NULL) {
		PyMem_Free(retValueBuf);
		return PyErr_NoMemory();
	}

	Py_BEGIN_ALLOW_THREADS
	rc = RegEnumValueW(hKey,
			  index,
			  retValueBuf,
			  &retValueSize,
			  NULL,
			  &typ,
			  retDataBuf,
			  &retDataSize);
	Py_END_ALLOW_THREADS

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
	retVal = Py_BuildValue("uOi", retValueBuf, obData, typ);
	Py_DECREF(obData);
  fail:
	PyMem_Free(retValueBuf);
	PyMem_Free(retDataBuf);
	return retVal;
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
	wchar_t *subKey;
	wchar_t *fileName;

	long rc;
	if (!PyArg_ParseTuple(args, "Ouu:LoadKey", &obKey, &subKey, &fileName))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	rc = RegLoadKeyW(hKey, subKey, fileName );
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

	wchar_t *subKey;
	int res = 0;
	HKEY retKey;
	long rc;
	REGSAM sam = KEY_READ;
	if (!PyArg_ParseTuple(args, "OZ|ii:OpenKey", &obKey, &subKey,
	                      &res, &sam))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	rc = RegOpenKeyExW(hKey, subKey, res, sam, &retKey);
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
	wchar_t *subKey;
	long rc;
	PyObject *retStr;
	wchar_t *retBuf;
	long bufSize = 0;

	if (!PyArg_ParseTuple(args, "OZ:QueryValue", &obKey, &subKey))
		return NULL;

	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	if ((rc = RegQueryValueW(hKey, subKey, NULL, &bufSize))
	    != ERROR_SUCCESS)
		return PyErr_SetFromWindowsErrWithFunction(rc,
							   "RegQueryValue");
	retBuf = (wchar_t *)PyMem_Malloc(bufSize);
	if (retBuf == NULL)
		return PyErr_NoMemory();

	if ((rc = RegQueryValueW(hKey, subKey, retBuf, &bufSize))
	    != ERROR_SUCCESS) {
		PyMem_Free(retBuf);
		return PyErr_SetFromWindowsErrWithFunction(rc,
							   "RegQueryValue");
	}

	retStr = PyUnicode_FromUnicode(retBuf, wcslen(retBuf));
	PyMem_Free(retBuf);
	return retStr;
}

static PyObject *
PyQueryValueEx(PyObject *self, PyObject *args)
{
	HKEY hKey;
	PyObject *obKey;
	wchar_t *valueName;

	long rc;
	BYTE *retBuf;
	DWORD bufSize = 0;
	DWORD typ;
	PyObject *obData;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "OZ:QueryValueEx", &obKey, &valueName))
		return NULL;

	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	if ((rc = RegQueryValueExW(hKey, valueName,
				  NULL, NULL, NULL,
				  &bufSize))
	    != ERROR_SUCCESS)
		return PyErr_SetFromWindowsErrWithFunction(rc,
							   "RegQueryValueEx");
	retBuf = (BYTE *)PyMem_Malloc(bufSize);
	if (retBuf == NULL)
		return PyErr_NoMemory();
	if ((rc = RegQueryValueExW(hKey, valueName, NULL,
				  &typ, retBuf, &bufSize))
	    != ERROR_SUCCESS) {
		PyMem_Free(retBuf);
		return PyErr_SetFromWindowsErrWithFunction(rc,
							   "RegQueryValueEx");
	}
	obData = Reg2Py(retBuf, bufSize, typ);
	PyMem_Free(retBuf);
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
	wchar_t *fileName;
	LPSECURITY_ATTRIBUTES pSA = NULL;

	long rc;
	if (!PyArg_ParseTuple(args, "Ou:SaveKey", &obKey, &fileName))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
/*  One day we may get security into the core?
	if (!PyWinObject_AsSECURITY_ATTRIBUTES(obSA, &pSA, TRUE))
		return NULL;
*/
	Py_BEGIN_ALLOW_THREADS
	rc = RegSaveKeyW(hKey, fileName, pSA );
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
	wchar_t *subKey;
	wchar_t *str;
	DWORD typ;
	DWORD len;
	long rc;
	if (!PyArg_ParseTuple(args, "OZiu#:SetValue",
			      &obKey,
			      &subKey,
			      &typ,
			      &str,
			      &len))
		return NULL;
	if (!PyHKEY_AsHKEY(obKey, &hKey, FALSE))
		return NULL;
	if (typ != REG_SZ) {
		PyErr_SetString(PyExc_TypeError,
				"Type must be _winreg.REG_SZ");
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	rc = RegSetValueW(hKey, subKey, REG_SZ, str, len+1);
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
	Py_UNICODE *valueName;
	PyObject *obRes;
	PyObject *value;
	BYTE *data;
	DWORD len;
	DWORD typ;

	LONG rc;

	if (!PyArg_ParseTuple(args, "OZOiO:SetValueEx",
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
	rc = RegSetValueExW(hKey, valueName, 0, typ, data, len);
	Py_END_ALLOW_THREADS
	PyMem_DEL(data);
	if (rc != ERROR_SUCCESS)
		return PyErr_SetFromWindowsErrWithFunction(rc,
							   "RegSetValueEx");
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef winreg_methods[] = {
	{"CloseKey",         PyCloseKey,        METH_VARARGS, CloseKey_doc},
	{"ConnectRegistry",  PyConnectRegistry, METH_VARARGS, ConnectRegistry_doc},
	{"CreateKey",        PyCreateKey,       METH_VARARGS, CreateKey_doc},
	{"DeleteKey",        PyDeleteKey,       METH_VARARGS, DeleteKey_doc},
	{"DeleteValue",      PyDeleteValue,     METH_VARARGS, DeleteValue_doc},
	{"EnumKey",          PyEnumKey,         METH_VARARGS, EnumKey_doc},
	{"EnumValue",        PyEnumValue,       METH_VARARGS, EnumValue_doc},
	{"FlushKey",         PyFlushKey,        METH_VARARGS, FlushKey_doc},
	{"LoadKey",          PyLoadKey,         METH_VARARGS, LoadKey_doc},
	{"OpenKey",          PyOpenKey,         METH_VARARGS, OpenKey_doc},
	{"OpenKeyEx",        PyOpenKey,         METH_VARARGS, OpenKeyEx_doc},
	{"QueryValue",       PyQueryValue,      METH_VARARGS, QueryValue_doc},
	{"QueryValueEx",     PyQueryValueEx,    METH_VARARGS, QueryValueEx_doc},
	{"QueryInfoKey",     PyQueryInfoKey,    METH_VARARGS, QueryInfoKey_doc},
	{"SaveKey",          PySaveKey,         METH_VARARGS, SaveKey_doc},
	{"SetValue",         PySetValue,        METH_VARARGS, SetValue_doc},
	{"SetValueEx",       PySetValueEx,      METH_VARARGS, SetValueEx_doc},
	NULL,
};

static void
insint(PyObject * d, char * name, long value)
{
	PyObject *v = PyLong_FromLong(value);
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
	Py_TYPE(&PyHKEY_Type) = &PyType_Type;
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


