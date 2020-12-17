/*
 * tclWinReg.c --
 *
 *	This file contains the implementation of the "registry" Tcl built-in
 *	command. This command is built as a dynamically loadable extension in
 *	a separate DLL.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#undef STATIC_BUILD
#ifndef USE_TCL_STUBS
#   define USE_TCL_STUBS
#endif
#include "tclInt.h"
#ifdef _MSC_VER
#   pragma comment (lib, "advapi32.lib")
#endif
#include <stdlib.h>

/*
 * Ensure that we can say which registry is being accessed.
 */

#ifndef KEY_WOW64_64KEY
#   define KEY_WOW64_64KEY	(0x0100)
#endif
#ifndef KEY_WOW64_32KEY
#   define KEY_WOW64_32KEY	(0x0200)
#endif

/*
 * The maximum length of a sub-key name.
 */

#ifndef MAX_KEY_LENGTH
#   define MAX_KEY_LENGTH	256
#endif

/*
 * The following macros convert between different endian ints.
 */

#define SWAPWORD(x)	MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x)	MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

/*
 * The following flag is used in OpenKeys to indicate that the specified key
 * should be created if it doesn't currently exist.
 */

#define REG_CREATE 1

/*
 * The following tables contain the mapping from registry root names to the
 * system predefined keys.
 */

static const char *const rootKeyNames[] = {
    "HKEY_LOCAL_MACHINE", "HKEY_USERS", "HKEY_CLASSES_ROOT",
    "HKEY_CURRENT_USER", "HKEY_CURRENT_CONFIG",
    "HKEY_PERFORMANCE_DATA", "HKEY_DYN_DATA", NULL
};

static const HKEY rootKeys[] = {
    HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT, HKEY_CURRENT_USER,
    HKEY_CURRENT_CONFIG, HKEY_PERFORMANCE_DATA, HKEY_DYN_DATA
};

static const char REGISTRY_ASSOC_KEY[] = "registry::command";

/*
 * The following table maps from registry types to strings. Note that the
 * indices for this array are the same as the constants for the known registry
 * types so we don't need a separate table to hold the mapping.
 */

static const char *const typeNames[] = {
    "none", "sz", "expand_sz", "binary", "dword",
    "dword_big_endian", "link", "multi_sz", "resource_list", NULL
};

static DWORD lastType = REG_RESOURCE_LIST;

/*
 * Declarations for functions defined in this file.
 */

static void		AppendSystemError(Tcl_Interp *interp, DWORD error);
static int		BroadcastValue(Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static DWORD		ConvertDWORD(DWORD type, DWORD value);
static void		DeleteCmd(ClientData clientData);
static int		DeleteKey(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    REGSAM mode);
static int		DeleteValue(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    Tcl_Obj *valueNameObj, REGSAM mode);
static int		GetKeyNames(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    Tcl_Obj *patternObj, REGSAM mode);
static int		GetType(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    Tcl_Obj *valueNameObj, REGSAM mode);
static int		GetValue(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    Tcl_Obj *valueNameObj, REGSAM mode);
static int		GetValueNames(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    Tcl_Obj *patternObj, REGSAM mode);
static int		OpenKey(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    REGSAM mode, int flags, HKEY *keyPtr);
static DWORD		OpenSubKey(char *hostName, HKEY rootKey,
			    char *keyName, REGSAM mode, int flags,
			    HKEY *keyPtr);
static int		ParseKeyName(Tcl_Interp *interp, char *name,
			    char **hostNamePtr, HKEY *rootKeyPtr,
			    char **keyNamePtr);
static DWORD		RecursiveDeleteKey(HKEY hStartKey,
			    const TCHAR * pKeyName, REGSAM mode);
static int		RegistryObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		SetValue(Tcl_Interp *interp, Tcl_Obj *keyNameObj,
			    Tcl_Obj *valueNameObj, Tcl_Obj *dataObj,
			    Tcl_Obj *typeObj, REGSAM mode);

static unsigned char *
getByteArrayFromObj(
	Tcl_Obj *objPtr,
	size_t *lengthPtr
) {
    int length;

    unsigned char *result = Tcl_GetByteArrayFromObj(objPtr, &length);
#if TCL_MAJOR_VERSION > 8
    if (sizeof(TCL_HASH_TYPE) > sizeof(int)) {
	/* 64-bit and TIP #494 situation: */
	 *lengthPtr = *(TCL_HASH_TYPE *) objPtr->internalRep.twoPtrValue.ptr1;
    } else
#endif
	/* 32-bit or without TIP #494 */
    *lengthPtr = (size_t) (unsigned) length;
    return result;
}

DLLEXPORT int		Registry_Init(Tcl_Interp *interp);
DLLEXPORT int		Registry_Unload(Tcl_Interp *interp, int flags);

/*
 *----------------------------------------------------------------------
 *
 * Registry_Init --
 *
 *	This function initializes the registry command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Registry_Init(
    Tcl_Interp *interp)
{
    Tcl_Command cmd;

    if (Tcl_InitStubs(interp, "8.5", 0) == NULL) {
	return TCL_ERROR;
    }

    cmd = Tcl_CreateObjCommand(interp, "registry", RegistryObjCmd,
	    interp, DeleteCmd);
    Tcl_SetAssocData(interp, REGISTRY_ASSOC_KEY, NULL, cmd);
    return Tcl_PkgProvide(interp, "registry", "1.3.3");
}

/*
 *----------------------------------------------------------------------
 *
 * Registry_Unload --
 *
 *	This function removes the registry command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	The registry command is deleted and the dll may be unloaded.
 *
 *----------------------------------------------------------------------
 */

int
Registry_Unload(
    Tcl_Interp *interp,		/* Interpreter for unloading */
    int flags)			/* Flags passed by the unload system */
{
    Tcl_Command cmd;
    Tcl_Obj *objv[3];

    /*
     * Unregister the registry package. There is no Tcl_PkgForget()
     */

    objv[0] = Tcl_NewStringObj("package", -1);
    objv[1] = Tcl_NewStringObj("forget", -1);
    objv[2] = Tcl_NewStringObj("registry", -1);
    Tcl_EvalObjv(interp, 3, objv, TCL_EVAL_GLOBAL);

    /*
     * Delete the originally registered command.
     */

    cmd = Tcl_GetAssocData(interp, REGISTRY_ASSOC_KEY, NULL);
    if (cmd != NULL) {
	Tcl_DeleteCommandFromToken(interp, cmd);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteCmd --
 *
 *	Cleanup the interp command token so that unloading doesn't try to
 *	re-delete the command (which will crash).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The unload command will not attempt to delete this command.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteCmd(
    ClientData clientData)
{
    Tcl_Interp *interp = clientData;

    Tcl_SetAssocData(interp, REGISTRY_ASSOC_KEY, NULL, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * RegistryObjCmd --
 *
 *	This function implements the Tcl "registry" command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
RegistryObjCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument values. */
{
    int n = 1;
    int index, argc;
    REGSAM mode = 0;
    const char *errString = NULL;

    static const char *const subcommands[] = {
	"broadcast", "delete", "get", "keys", "set", "type", "values", NULL
    };
    enum SubCmdIdx {
	BroadcastIdx, DeleteIdx, GetIdx, KeysIdx, SetIdx, TypeIdx, ValuesIdx
    };
    static const char *const modes[] = {
	"-32bit", "-64bit", NULL
    };

    if (objc < 2) {
    wrongArgs:
	Tcl_WrongNumArgs(interp, 1, objv, "?-32bit|-64bit? option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetString(objv[n])[0] == '-') {
	if (Tcl_GetIndexFromObj(interp, objv[n++], modes, "mode", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch (index) {
	case 0:			/* -32bit */
	    mode |= KEY_WOW64_32KEY;
	    break;
	case 1:			/* -64bit */
	    mode |= KEY_WOW64_64KEY;
	    break;
	}
	if (objc < 3) {
	    goto wrongArgs;
	}
    }

    if (Tcl_GetIndexFromObj(interp, objv[n++], subcommands, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    argc = (objc - n);
    switch (index) {
    case BroadcastIdx:		/* broadcast */
	if (argc == 1 || argc == 3) {
	    int res = BroadcastValue(interp, argc, objv + n);

	    if (res != TCL_BREAK) {
		return res;
	    }
	}
	errString = "keyName ?-timeout milliseconds?";
	break;
    case DeleteIdx:		/* delete */
	if (argc == 1) {
	    return DeleteKey(interp, objv[n], mode);
	} else if (argc == 2) {
	    return DeleteValue(interp, objv[n], objv[n+1], mode);
	}
	errString = "keyName ?valueName?";
	break;
    case GetIdx:		/* get */
	if (argc == 2) {
	    return GetValue(interp, objv[n], objv[n+1], mode);
	}
	errString = "keyName valueName";
	break;
    case KeysIdx:		/* keys */
	if (argc == 1) {
	    return GetKeyNames(interp, objv[n], NULL, mode);
	} else if (argc == 2) {
	    return GetKeyNames(interp, objv[n], objv[n+1], mode);
	}
	errString = "keyName ?pattern?";
	break;
    case SetIdx:		/* set */
	if (argc == 1) {
	    HKEY key;

	    /*
	     * Create the key and then close it immediately.
	     */

	    mode |= KEY_ALL_ACCESS;
	    if (OpenKey(interp, objv[n], mode, 1, &key) != TCL_OK) {
		return TCL_ERROR;
	    }
	    RegCloseKey(key);
	    return TCL_OK;
	} else if (argc == 3) {
	    return SetValue(interp, objv[n], objv[n+1], objv[n+2], NULL,
		    mode);
	} else if (argc == 4) {
	    return SetValue(interp, objv[n], objv[n+1], objv[n+2], objv[n+3],
		    mode);
	}
	errString = "keyName ?valueName data ?type??";
	break;
    case TypeIdx:		/* type */
	if (argc == 2) {
	    return GetType(interp, objv[n], objv[n+1], mode);
	}
	errString = "keyName valueName";
	break;
    case ValuesIdx:		/* values */
	if (argc == 1) {
	    return GetValueNames(interp, objv[n], NULL, mode);
	} else if (argc == 2) {
	    return GetValueNames(interp, objv[n], objv[n+1], mode);
	}
	errString = "keyName ?pattern?";
	break;
    }
    Tcl_WrongNumArgs(interp, (mode ? 3 : 2), objv, errString);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteKey --
 *
 *	This function deletes a registry key.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
DeleteKey(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Name of key to delete. */
    REGSAM mode)		/* Mode flags to pass. */
{
    char *tail, *buffer, *hostName, *keyName;
    const TCHAR *nativeTail;
    HKEY rootKey, subkey;
    DWORD result;
    Tcl_DString buf;
    REGSAM saveMode = mode;

    /*
     * Find the parent of the key being deleted and open it.
     */

    keyName = Tcl_GetString(keyNameObj);
    buffer = Tcl_Alloc(keyNameObj->length + 1);
    strcpy(buffer, keyName);

    if (ParseKeyName(interp, buffer, &hostName, &rootKey,
	    &keyName) != TCL_OK) {
	Tcl_Free(buffer);
	return TCL_ERROR;
    }

    if (*keyName == '\0') {
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj("bad key: cannot delete root keys", -1));
	Tcl_SetErrorCode(interp, "WIN_REG", "DEL_ROOT_KEY", NULL);
	Tcl_Free(buffer);
	return TCL_ERROR;
    }

    tail = strrchr(keyName, '\\');
    if (tail) {
	*tail++ = '\0';
    } else {
	tail = keyName;
	keyName = NULL;
    }

    mode |= KEY_ENUMERATE_SUB_KEYS | DELETE;
    result = OpenSubKey(hostName, rootKey, keyName, mode, 0, &subkey);
    if (result != ERROR_SUCCESS) {
	Tcl_Free(buffer);
	if (result == ERROR_FILE_NOT_FOUND) {
	    return TCL_OK;
	}
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj("unable to delete key: ", -1));
	AppendSystemError(interp, result);
	return TCL_ERROR;
    }

    /*
     * Now we recursively delete the key and everything below it.
     */

    nativeTail = Tcl_WinUtfToTChar(tail, -1, &buf);
    result = RecursiveDeleteKey(subkey, nativeTail, saveMode);
    Tcl_DStringFree(&buf);

    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj("unable to delete key: ", -1));
	AppendSystemError(interp, result);
	result = TCL_ERROR;
    } else {
	result = TCL_OK;
    }

    RegCloseKey(subkey);
    Tcl_Free(buffer);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteValue --
 *
 *	This function deletes a value from a registry key.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
DeleteValue(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Name of key. */
    Tcl_Obj *valueNameObj,	/* Name of value to delete. */
    REGSAM mode)		/* Mode flags to pass. */
{
    HKEY key;
    char *valueName;
    DWORD result;
    Tcl_DString ds;

    /*
     * Attempt to open the key for deletion.
     */

    mode |= KEY_SET_VALUE;
    if (OpenKey(interp, keyNameObj, mode, 0, &key) != TCL_OK) {
	return TCL_ERROR;
    }

    valueName = Tcl_GetString(valueNameObj);
    Tcl_WinUtfToTChar(valueName, valueNameObj->length, &ds);
    result = RegDeleteValue(key, (const TCHAR *)Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    if (result != ERROR_SUCCESS) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unable to delete value \"%s\" from key \"%s\": ",
		Tcl_GetString(valueNameObj), Tcl_GetString(keyNameObj)));
	AppendSystemError(interp, result);
	result = TCL_ERROR;
    } else {
	result = TCL_OK;
    }
    RegCloseKey(key);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * GetKeyNames --
 *
 *	This function enumerates the subkeys of a given key. If the optional
 *	pattern is supplied, then only keys that match the pattern will be
 *	returned.
 *
 * Results:
 *	Returns the list of subkeys in the result object of the interpreter,
 *	or an error message on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetKeyNames(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Key to enumerate. */
    Tcl_Obj *patternObj,	/* Optional match pattern. */
    REGSAM mode)		/* Mode flags to pass. */
{
    const char *pattern;	/* Pattern being matched against subkeys */
    HKEY key;			/* Handle to the key being examined */
    TCHAR buffer[MAX_KEY_LENGTH];
				/* Buffer to hold the subkey name */
    DWORD bufSize;		/* Size of the buffer */
    DWORD index;		/* Position of the current subkey */
    char *name;			/* Subkey name */
    Tcl_Obj *resultPtr;		/* List of subkeys being accumulated */
    int result = TCL_OK;	/* Return value from this command */
    Tcl_DString ds;		/* Buffer to translate subkey name to UTF-8 */

    if (patternObj) {
	pattern = Tcl_GetString(patternObj);
    } else {
	pattern = NULL;
    }

    /*
     * Attempt to open the key for enumeration.
     */

    mode |= KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS;
    if (OpenKey(interp, keyNameObj, mode, 0, &key) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Enumerate the subkeys.
     */

    resultPtr = Tcl_NewObj();
    for (index = 0;; ++index) {
	bufSize = MAX_KEY_LENGTH;
	result = RegEnumKeyEx(key, index, buffer, &bufSize,
		NULL, NULL, NULL, NULL);
	if (result != ERROR_SUCCESS) {
	    if (result == ERROR_NO_MORE_ITEMS) {
		result = TCL_OK;
	    } else {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"unable to enumerate subkeys of \"%s\": ",
			Tcl_GetString(keyNameObj)));
		AppendSystemError(interp, result);
		result = TCL_ERROR;
	    }
	    break;
	}
	name = Tcl_WinTCharToUtf(buffer, bufSize * sizeof(TCHAR), &ds);
	if (pattern && !Tcl_StringMatch(name, pattern)) {
	    Tcl_DStringFree(&ds);
	    continue;
	}
	result = Tcl_ListObjAppendElement(interp, resultPtr,
		Tcl_NewStringObj(name, Tcl_DStringLength(&ds)));
	Tcl_DStringFree(&ds);
	if (result != TCL_OK) {
	    break;
	}
    }
    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, resultPtr);
    } else {
	Tcl_DecrRefCount(resultPtr); /* BUGFIX: Don't leak on failure. */
    }

    RegCloseKey(key);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * GetType --
 *
 *	This function gets the type of a given registry value and places it in
 *	the interpreter result.
 *
 * Results:
 *	Returns a normal Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetType(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Name of key. */
    Tcl_Obj *valueNameObj,	/* Name of value to get. */
    REGSAM mode)		/* Mode flags to pass. */
{
    HKEY key;
    DWORD result, type;
    Tcl_DString ds;
    const char *valueName;
    const TCHAR *nativeValue;

    /*
     * Attempt to open the key for reading.
     */

    mode |= KEY_QUERY_VALUE;
    if (OpenKey(interp, keyNameObj, mode, 0, &key) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Get the type of the value.
     */

    valueName = Tcl_GetString(valueNameObj);
    nativeValue = Tcl_WinUtfToTChar(valueName, valueNameObj->length, &ds);
    result = RegQueryValueEx(key, nativeValue, NULL, &type,
	    NULL, NULL);
    Tcl_DStringFree(&ds);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unable to get type of value \"%s\" from key \"%s\": ",
		Tcl_GetString(valueNameObj), Tcl_GetString(keyNameObj)));
	AppendSystemError(interp, result);
	return TCL_ERROR;
    }

    /*
     * Set the type into the result. Watch out for unknown types. If we don't
     * know about the type, just use the numeric value.
     */

    if (type > lastType) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj((int) type));
    } else {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(typeNames[type], -1));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetValue --
 *
 *	This function gets the contents of a registry value and places a list
 *	containing the data and the type in the interpreter result.
 *
 * Results:
 *	Returns a normal Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetValue(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Name of key. */
    Tcl_Obj *valueNameObj,	/* Name of value to get. */
    REGSAM mode)		/* Mode flags to pass. */
{
    HKEY key;
    const char *valueName;
    const TCHAR *nativeValue;
    DWORD result, length, type;
    Tcl_DString data, buf;

    /*
     * Attempt to open the key for reading.
     */

    mode |= KEY_QUERY_VALUE;
    if (OpenKey(interp, keyNameObj, mode, 0, &key) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Initialize a Dstring to maximum statically allocated size we could get
     * one more byte by avoiding Tcl_DStringSetLength() and just setting
     * length to TCL_DSTRING_STATIC_SIZE, but this should be safer if the
     * implementation of Dstrings changes.
     *
     * This allows short values to be read from the registy in one call.
     * Longer values need a second call with an expanded DString.
     */

    Tcl_DStringInit(&data);
    Tcl_DStringSetLength(&data, TCL_DSTRING_STATIC_SIZE - 1);
    length = TCL_DSTRING_STATIC_SIZE/sizeof(TCHAR) - 1;

    valueName = Tcl_GetString(valueNameObj);
    nativeValue = Tcl_WinUtfToTChar(valueName, valueNameObj->length, &buf);

    result = RegQueryValueEx(key, nativeValue, NULL, &type,
	    (BYTE *) Tcl_DStringValue(&data), &length);
    while (result == ERROR_MORE_DATA) {
	/*
	 * The Windows docs say that in this error case, we just need to
	 * expand our buffer and request more data. Required for
	 * HKEY_PERFORMANCE_DATA
	 */

	length = Tcl_DStringLength(&data) * (2 / sizeof(TCHAR));
	Tcl_DStringSetLength(&data, (int) length * sizeof(TCHAR));
	result = RegQueryValueEx(key, nativeValue,
		NULL, &type, (BYTE *) Tcl_DStringValue(&data), &length);
    }
    Tcl_DStringFree(&buf);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"unable to get value \"%s\" from key \"%s\": ",
		Tcl_GetString(valueNameObj), Tcl_GetString(keyNameObj)));
	AppendSystemError(interp, result);
	Tcl_DStringFree(&data);
	return TCL_ERROR;
    }

    /*
     * If the data is a 32-bit quantity, store it as an integer object. If it
     * is a multi-string, store it as a list of strings. For null-terminated
     * strings, append up the to first null. Otherwise, store it as a binary
     * string.
     */

    if (type == REG_DWORD || type == REG_DWORD_BIG_ENDIAN) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj((int) ConvertDWORD(type,
		*((DWORD *) Tcl_DStringValue(&data)))));
    } else if (type == REG_MULTI_SZ) {
	char *p = Tcl_DStringValue(&data);
	char *end = Tcl_DStringValue(&data) + length;
	Tcl_Obj *resultPtr = Tcl_NewObj();

	/*
	 * Multistrings are stored as an array of null-terminated strings,
	 * terminated by two null characters. Also do a bounds check in case
	 * we get bogus data.
	 */

	while ((p < end) && *((WCHAR *) p) != 0) {
	    WCHAR *wp;

	    Tcl_WinTCharToUtf((TCHAR *) p, -1, &buf);
	    Tcl_ListObjAppendElement(interp, resultPtr,
		    Tcl_NewStringObj(Tcl_DStringValue(&buf),
			    Tcl_DStringLength(&buf)));
	    wp = (WCHAR *) p;

	    while (*wp++ != 0) {/* empty body */}
	    p = (char *) wp;
	    Tcl_DStringFree(&buf);
	}
	Tcl_SetObjResult(interp, resultPtr);
    } else if ((type == REG_SZ) || (type == REG_EXPAND_SZ)) {
	Tcl_WinTCharToUtf((TCHAR *) Tcl_DStringValue(&data), -1, &buf);
	Tcl_DStringResult(interp, &buf);
    } else {
	/*
	 * Save binary data as a byte array.
	 */

	Tcl_SetObjResult(interp, Tcl_NewByteArrayObj(
		(BYTE *) Tcl_DStringValue(&data), (int) length));
    }
    Tcl_DStringFree(&data);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * GetValueNames --
 *
 *	This function enumerates the values of the a given key. If the
 *	optional pattern is supplied, then only value names that match the
 *	pattern will be returned.
 *
 * Results:
 *	Returns the list of value names in the result object of the
 *	interpreter, or an error message on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetValueNames(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Key to enumerate. */
    Tcl_Obj *patternObj,	/* Optional match pattern. */
    REGSAM mode)		/* Mode flags to pass. */
{
    HKEY key;
    Tcl_Obj *resultPtr;
    DWORD index, size, result;
    Tcl_DString buffer, ds;
    const char *pattern, *name;

    /*
     * Attempt to open the key for enumeration.
     */

    mode |= KEY_QUERY_VALUE;
    if (OpenKey(interp, keyNameObj, mode, 0, &key) != TCL_OK) {
	return TCL_ERROR;
    }

    resultPtr = Tcl_NewObj();
    Tcl_DStringInit(&buffer);
    Tcl_DStringSetLength(&buffer, (int) (MAX_KEY_LENGTH * sizeof(TCHAR)));
    index = 0;
    result = TCL_OK;

    if (patternObj) {
	pattern = Tcl_GetString(patternObj);
    } else {
	pattern = NULL;
    }

    /*
     * Enumerate the values under the given subkey until we get an error,
     * indicating the end of the list. Note that we need to reset size after
     * each iteration because RegEnumValue smashes the old value.
     */

    size = MAX_KEY_LENGTH;
    while (RegEnumValue(key,index, (TCHAR *)Tcl_DStringValue(&buffer),
	    &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
	size *= sizeof(TCHAR);

	Tcl_WinTCharToUtf((TCHAR *) Tcl_DStringValue(&buffer), (int) size,
		&ds);
	name = Tcl_DStringValue(&ds);
	if (!pattern || Tcl_StringMatch(name, pattern)) {
	    result = Tcl_ListObjAppendElement(interp, resultPtr,
		    Tcl_NewStringObj(name, Tcl_DStringLength(&ds)));
	    if (result != TCL_OK) {
		Tcl_DStringFree(&ds);
		break;
	    }
	}
	Tcl_DStringFree(&ds);

	index++;
	size = MAX_KEY_LENGTH;
    }
    Tcl_SetObjResult(interp, resultPtr);
    Tcl_DStringFree(&buffer);
    RegCloseKey(key);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * OpenKey --
 *
 *	This function opens the specified key. This function is a simple
 *	wrapper around ParseKeyName and OpenSubKey.
 *
 * Results:
 *	Returns the opened key in the keyPtr argument and a Tcl result code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
OpenKey(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Key to open. */
    REGSAM mode,		/* Access mode. */
    int flags,			/* 0 or REG_CREATE. */
    HKEY *keyPtr)		/* Returned HKEY. */
{
    char *keyName, *buffer, *hostName;
    HKEY rootKey;
    DWORD result;

    keyName = Tcl_GetString(keyNameObj);
    buffer = Tcl_Alloc(keyNameObj->length + 1);
    strcpy(buffer, keyName);

    result = ParseKeyName(interp, buffer, &hostName, &rootKey, &keyName);
    if (result == TCL_OK) {
	result = OpenSubKey(hostName, rootKey, keyName, mode, flags, keyPtr);
	if (result != ERROR_SUCCESS) {
	    Tcl_SetObjResult(interp,
		    Tcl_NewStringObj("unable to open key: ", -1));
	    AppendSystemError(interp, result);
	    result = TCL_ERROR;
	} else {
	    result = TCL_OK;
	}
    }

    Tcl_Free(buffer);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * OpenSubKey --
 *
 *	This function opens a given subkey of a root key on the specified
 *	host.
 *
 * Results:
 *	Returns the opened key in the keyPtr and a Windows error code as the
 *	return value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static DWORD
OpenSubKey(
    char *hostName,		/* Host to access, or NULL for local. */
    HKEY rootKey,		/* Root registry key. */
    char *keyName,		/* Subkey name. */
    REGSAM mode,		/* Access mode. */
    int flags,			/* 0 or REG_CREATE. */
    HKEY *keyPtr)		/* Returned HKEY. */
{
    DWORD result;
    Tcl_DString buf;

    /*
     * Attempt to open the root key on a remote host if necessary.
     */

    if (hostName) {
	hostName = (char *) Tcl_WinUtfToTChar(hostName, -1, &buf);
	result = RegConnectRegistry((TCHAR *)hostName, rootKey,
		&rootKey);
	Tcl_DStringFree(&buf);
	if (result != ERROR_SUCCESS) {
	    return result;
	}
    }

    /*
     * Now open the specified key with the requested permissions. Note that
     * this key must be closed by the caller.
     */

    if (keyName) {
	keyName = (char *) Tcl_WinUtfToTChar(keyName, -1, &buf);
    }
    if (flags & REG_CREATE) {
	DWORD create;

	result = RegCreateKeyEx(rootKey, (TCHAR *)keyName, 0, NULL,
		REG_OPTION_NON_VOLATILE, mode, NULL, keyPtr, &create);
    } else if (rootKey == HKEY_PERFORMANCE_DATA) {
	/*
	 * Here we fudge it for this special root key. See MSDN for more info
	 * on HKEY_PERFORMANCE_DATA and the peculiarities surrounding it.
	 */

	*keyPtr = HKEY_PERFORMANCE_DATA;
	result = ERROR_SUCCESS;
    } else {
	result = RegOpenKeyEx(rootKey, (TCHAR *)keyName, 0, mode,
		keyPtr);
    }
    if (keyName) {
	Tcl_DStringFree(&buf);
    }

    /*
     * Be sure to close the root key since we are done with it now.
     */

    if (hostName) {
	RegCloseKey(rootKey);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseKeyName --
 *
 *	This function parses a key name into the host, root, and subkey parts.
 *
 * Results:
 *	The pointers to the start of the host and subkey names are returned in
 *	the hostNamePtr and keyNamePtr variables. The specified root HKEY is
 *	returned in rootKeyPtr. Returns a standard Tcl result.
 *
 * Side effects:
 *	Modifies the name string by inserting nulls.
 *
 *----------------------------------------------------------------------
 */

static int
ParseKeyName(
    Tcl_Interp *interp,		/* Current interpreter. */
    char *name,
    char **hostNamePtr,
    HKEY *rootKeyPtr,
    char **keyNamePtr)
{
    char *rootName;
    int result, index;
    Tcl_Obj *rootObj;

    /*
     * Split the key into host and root portions.
     */

    *hostNamePtr = *keyNamePtr = rootName = NULL;
    if (name[0] == '\\') {
	if (name[1] == '\\') {
	    *hostNamePtr = name;
	    for (rootName = name+2; *rootName != '\0'; rootName++) {
		if (*rootName == '\\') {
		    *rootName++ = '\0';
		    break;
		}
	    }
	}
    } else {
	rootName = name;
    }
    if (!rootName) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad key \"%s\": must start with a valid root", name));
	Tcl_SetErrorCode(interp, "WIN_REG", "NO_ROOT_KEY", NULL);
	return TCL_ERROR;
    }

    /*
     * Split the root into root and subkey portions.
     */

    for (*keyNamePtr = rootName; **keyNamePtr != '\0'; (*keyNamePtr)++) {
	if (**keyNamePtr == '\\') {
	    **keyNamePtr = '\0';
	    (*keyNamePtr)++;
	    break;
	}
    }

    /*
     * Look for a matching root name.
     */

    rootObj = Tcl_NewStringObj(rootName, -1);
    result = Tcl_GetIndexFromObj(interp, rootObj, rootKeyNames, "root name",
	    TCL_EXACT, &index);
    Tcl_DecrRefCount(rootObj);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    *rootKeyPtr = rootKeys[index];
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RecursiveDeleteKey --
 *
 *	This function recursively deletes all the keys below a starting key.
 *	Although Windows 95 does this automatically, we still need to do this
 *	for Windows NT.
 *
 * Results:
 *	Returns a Windows error code.
 *
 * Side effects:
 *	Deletes all of the keys and values below the given key.
 *
 *----------------------------------------------------------------------
 */

static DWORD
RecursiveDeleteKey(
    HKEY startKey,		/* Parent of key to be deleted. */
    const TCHAR *keyName,	/* Name of key to be deleted in external
				 * encoding, not UTF. */
    REGSAM mode)		/* Mode flags to pass. */
{
    DWORD result, size;
    Tcl_DString subkey;
    HKEY hKey;
    REGSAM saveMode = mode;
    static int checkExProc = 0;
    static FARPROC regDeleteKeyExProc = NULL;

    /*
     * Do not allow NULL or empty key name.
     */

    if (!keyName || *keyName == '\0') {
	return ERROR_BADKEY;
    }

    mode |= KEY_ENUMERATE_SUB_KEYS | DELETE | KEY_QUERY_VALUE;
    result = RegOpenKeyEx(startKey, keyName, 0, mode, &hKey);
    if (result != ERROR_SUCCESS) {
	return result;
    }

    Tcl_DStringInit(&subkey);
    Tcl_DStringSetLength(&subkey, (int) (MAX_KEY_LENGTH * sizeof(TCHAR)));

    mode = saveMode;
    while (result == ERROR_SUCCESS) {
	/*
	 * Always get index 0 because key deletion changes ordering.
	 */

	size = MAX_KEY_LENGTH;
	result = RegEnumKeyEx(hKey, 0, (TCHAR *)Tcl_DStringValue(&subkey),
		&size, NULL, NULL, NULL, NULL);
	if (result == ERROR_NO_MORE_ITEMS) {
	    /*
	     * RegDeleteKeyEx doesn't exist on non-64bit XP platforms, so we
	     * can't compile with it in. We need to check for it at runtime
	     * and use it if we find it.
	     */

	    if (mode && !checkExProc) {
		HMODULE handle;

		checkExProc = 1;
		handle = GetModuleHandle(TEXT("ADVAPI32"));
		regDeleteKeyExProc = (FARPROC)
			GetProcAddress(handle, "RegDeleteKeyExW");
	    }
	    if (mode && regDeleteKeyExProc) {
		result = regDeleteKeyExProc(startKey, keyName, mode, 0);
	    } else {
		result = RegDeleteKey(startKey, keyName);
	    }
	    break;
	} else if (result == ERROR_SUCCESS) {
	    result = RecursiveDeleteKey(hKey,
		    (const TCHAR *) Tcl_DStringValue(&subkey), mode);
	}
    }
    Tcl_DStringFree(&subkey);
    RegCloseKey(hKey);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SetValue --
 *
 *	This function sets the contents of a registry value. If the key or
 *	value does not exist, it will be created. If it does exist, then the
 *	data and type will be replaced.
 *
 * Results:
 *	Returns a normal Tcl result.
 *
 * Side effects:
 *	May create new keys or values.
 *
 *----------------------------------------------------------------------
 */

static int
SetValue(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Obj *keyNameObj,	/* Name of key. */
    Tcl_Obj *valueNameObj,	/* Name of value to set. */
    Tcl_Obj *dataObj,		/* Data to be written. */
    Tcl_Obj *typeObj,		/* Type of data to be written. */
    REGSAM mode)		/* Mode flags to pass. */
{
    int type;
    DWORD result;
    HKEY key;
    const char *valueName;
    Tcl_DString nameBuf;

    if (typeObj == NULL) {
	type = REG_SZ;
    } else if (Tcl_GetIndexFromObj(interp, typeObj, typeNames, "type",
	    0, (int *) &type) != TCL_OK) {
	if (Tcl_GetIntFromObj(NULL, typeObj, (int *) &type) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_ResetResult(interp);
    }
    mode |= KEY_ALL_ACCESS;
    if (OpenKey(interp, keyNameObj, mode, 1, &key) != TCL_OK) {
	return TCL_ERROR;
    }

    valueName = Tcl_GetString(valueNameObj);
    valueName = (char *) Tcl_WinUtfToTChar(valueName, valueNameObj->length, &nameBuf);

    if (type == REG_DWORD || type == REG_DWORD_BIG_ENDIAN) {
	int value;

	if (Tcl_GetIntFromObj(interp, dataObj, &value) != TCL_OK) {
	    RegCloseKey(key);
	    Tcl_DStringFree(&nameBuf);
	    return TCL_ERROR;
	}

	value = ConvertDWORD((DWORD) type, (DWORD) value);
	result = RegSetValueEx(key, (TCHAR *) valueName, 0,
		(DWORD) type, (BYTE *) &value, sizeof(DWORD));
    } else if (type == REG_MULTI_SZ) {
	Tcl_DString data, buf;
	int objc, i;
	Tcl_Obj **objv;

	if (Tcl_ListObjGetElements(interp, dataObj, &objc, &objv) != TCL_OK) {
	    RegCloseKey(key);
	    Tcl_DStringFree(&nameBuf);
	    return TCL_ERROR;
	}

	/*
	 * Append the elements as null terminated strings. Note that we must
	 * not assume the length of the string in case there are embedded
	 * nulls, which aren't allowed in REG_MULTI_SZ values.
	 */

	Tcl_DStringInit(&data);
	for (i = 0; i < objc; i++) {
	    const char *bytes = Tcl_GetString(objv[i]);

	    Tcl_DStringAppend(&data, bytes, objv[i]->length);

	    /*
	     * Add a null character to separate this value from the next.
	     */

	    Tcl_DStringAppend(&data, "", 1);	/* NUL-terminated string */
	}

	Tcl_WinUtfToTChar(Tcl_DStringValue(&data), Tcl_DStringLength(&data)+1,
		&buf);
	result = RegSetValueEx(key, (TCHAR *) valueName, 0,
		(DWORD) type, (BYTE *) Tcl_DStringValue(&buf),
		(DWORD) Tcl_DStringLength(&buf));
	Tcl_DStringFree(&data);
	Tcl_DStringFree(&buf);
    } else if (type == REG_SZ || type == REG_EXPAND_SZ) {
	Tcl_DString buf;
	const char *data = Tcl_GetString(dataObj);

	data = (char *) Tcl_WinUtfToTChar(data, dataObj->length, &buf);

	/*
	 * Include the null in the length, padding if needed for WCHAR.
	 */

	Tcl_DStringSetLength(&buf, Tcl_DStringLength(&buf)+1);

	result = RegSetValueEx(key, (TCHAR *) valueName, 0,
		(DWORD) type, (BYTE *) data, (DWORD) Tcl_DStringLength(&buf) + 1);
	Tcl_DStringFree(&buf);
    } else {
	BYTE *data;
	size_t bytelength;

	/*
	 * Store binary data in the registry.
	 */

	data = (BYTE *) getByteArrayFromObj(dataObj, &bytelength);
	result = RegSetValueEx(key, (TCHAR *) valueName, 0,
		(DWORD) type, data, (DWORD) bytelength);
    }

    Tcl_DStringFree(&nameBuf);
    RegCloseKey(key);

    if (result != ERROR_SUCCESS) {
	Tcl_SetObjResult(interp,
		Tcl_NewStringObj("unable to set value: ", -1));
	AppendSystemError(interp, result);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * BroadcastValue --
 *
 *	This function broadcasts a WM_SETTINGCHANGE message to indicate to
 *	other programs that we have changed the contents of a registry value.
 *
 * Results:
 *	Returns a normal Tcl result.
 *
 * Side effects:
 *	Will cause other programs to reload their system settings.
 *
 *----------------------------------------------------------------------
 */

static int
BroadcastValue(
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument values. */
{
    LRESULT result;
    DWORD_PTR sendResult;
    int timeout = 3000;
    size_t len;
    const char *str;
    Tcl_Obj *objPtr;
    WCHAR *wstr;
    Tcl_DString ds;

    if (objc == 3) {
	str = Tcl_GetString(objv[1]);
	len = objv[1]->length;
	if ((len < 2) || (*str != '-') || strncmp(str, "-timeout", len)) {
	    return TCL_BREAK;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &timeout) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    str = Tcl_GetString(objv[0]);
    wstr = (WCHAR *) Tcl_WinUtfToTChar(str, objv[0]->length, &ds);
    if (Tcl_DStringLength(&ds) == 0) {
	wstr = NULL;
    }

    /*
     * Use the ignore the result.
     */

    result = SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE,
	    (WPARAM) 0, (LPARAM) wstr, SMTO_ABORTIFHUNG, (UINT) timeout, &sendResult);
    Tcl_DStringFree(&ds);

    objPtr = Tcl_NewObj();
    Tcl_ListObjAppendElement(NULL, objPtr, Tcl_NewWideIntObj((Tcl_WideInt) result));
    Tcl_ListObjAppendElement(NULL, objPtr, Tcl_NewWideIntObj((Tcl_WideInt) sendResult));
    Tcl_SetObjResult(interp, objPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AppendSystemError --
 *
 *	This routine formats a Windows system error message and places it into
 *	the interpreter result.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
AppendSystemError(
    Tcl_Interp *interp,		/* Current interpreter. */
    DWORD error)		/* Result code from error. */
{
    int length;
    TCHAR *tMsgPtr, **tMsgPtrPtr = &tMsgPtr;
    const char *msg;
    char id[TCL_INTEGER_SPACE], msgBuf[24 + TCL_INTEGER_SPACE];
    Tcl_DString ds;
    Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);

    if (Tcl_IsShared(resultPtr)) {
	resultPtr = Tcl_DuplicateObj(resultPtr);
    }
    length = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
	    | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, error,
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (TCHAR *) tMsgPtrPtr,
	    0, NULL);
    if (length == 0) {
	sprintf(msgBuf, "unknown error: %ld", error);
	msg = msgBuf;
    } else {
	char *msgPtr;

	Tcl_WinTCharToUtf(tMsgPtr, -1, &ds);
	LocalFree(tMsgPtr);

	msgPtr = Tcl_DStringValue(&ds);
	length = Tcl_DStringLength(&ds);

	/*
	 * Trim the trailing CR/LF from the system message.
	 */

	if (msgPtr[length-1] == '\n') {
	    --length;
	}
	if (msgPtr[length-1] == '\r') {
	    --length;
	}
	msgPtr[length] = 0;
	msg = msgPtr;
    }

    sprintf(id, "%ld", error);
    Tcl_SetErrorCode(interp, "WINDOWS", id, msg, NULL);
    Tcl_AppendToObj(resultPtr, msg, length);
    Tcl_SetObjResult(interp, resultPtr);

    if (length != 0) {
	Tcl_DStringFree(&ds);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConvertDWORD --
 *
 *	This function determines whether a DWORD needs to be byte swapped, and
 *	returns the appropriately swapped value.
 *
 * Results:
 *	Returns a converted DWORD.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static DWORD
ConvertDWORD(
    DWORD type,			/* Either REG_DWORD or REG_DWORD_BIG_ENDIAN */
    DWORD value)		/* The value to be converted. */
{
    const DWORD order = 1;
    DWORD localType;

    /*
     * Check to see if the low bit is in the first byte.
     */

    localType = (*((const char *) &order) == 1)
	    ? REG_DWORD : REG_DWORD_BIG_ENDIAN;
    return (type != localType) ? (DWORD) SWAPLONG(value) : value;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
