#include "Python.h"
#include "osdefs.h"
#include <windows.h>

/* PREFIX and EXEC_PREFIX are meaningless on Windows */

#ifndef PREFIX
#define PREFIX ""
#endif

#ifndef EXEC_PREFIX
#define EXEC_PREFIX ""
#endif

/*
This is a special Win32 version of getpath.

* There is no default path.  There is nothing even remotely resembling
  a standard location.  Maybe later "Program Files/Python", but not yet.

* The Registry is used as the primary store for the Python path.

* The environment variable PYTHONPATH _overrides_ the registry.  This should
  allow a "standard" Python environment, but allow you to manually setup
  another (eg, a beta version).

*/

BOOL PyWin_IsWin32s()
{
	static BOOL bIsWin32s = -1; // flag as "not yet looked"

	if (bIsWin32s==-1) {
		OSVERSIONINFO ver;
		ver.dwOSVersionInfoSize = sizeof(ver);
		GetVersionEx(&ver);
		bIsWin32s = ver.dwPlatformId == VER_PLATFORM_WIN32s;
	}
	return bIsWin32s;
}

/* Load a PYTHONPATH value from the registry
   Load from either HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER

   Returns NULL, or a pointer that should be free'd.
*/
static char *
getpythonregpath(HKEY keyBase, BOOL bWin32s)
{
	HKEY newKey = 0;
	DWORD nameSize = 0;
	DWORD dataSize = 0;
	DWORD numEntries = 0;
	LONG rc;
	char *retval = NULL;
	char *dataBuf;
	if ((rc=RegOpenKey(keyBase, "Software\\Python\\PythonCore\\" MS_DLL_ID "\\PythonPath", 
	                   &newKey))==ERROR_SUCCESS) {
		RegQueryInfoKey(newKey, NULL, NULL, NULL, NULL, NULL, NULL, 
		                &numEntries, &nameSize, &dataSize, NULL, NULL );
	}
	if (bWin32s && numEntries==0 && dataSize==0) { /* must hardcode for Win32s */
		numEntries = 1;
		dataSize = 511;
	}
	if (numEntries) {
		/* Loop over all subkeys. */
		/* Win32s doesnt know how many subkeys, so we do
		   it twice */
		char keyBuf[MAX_PATH+1];
		int index = 0;
		int off = 0;
		for(index=0;;index++) {
			long reqdSize = 0;
			DWORD rc = RegEnumKey(newKey, index, keyBuf,MAX_PATH+1);
			if (rc) break;
			rc = RegQueryValue(newKey, keyBuf, NULL, &reqdSize);
			if (rc) break;
			if (bWin32s && reqdSize==0) reqdSize = 512;
			dataSize += reqdSize + 1; /* 1 for the ";" */
		}
		dataBuf = malloc(dataSize+1);
		if (dataBuf==NULL) return NULL; /* pretty serious?  Raise error? */
		/* Now loop over, grabbing the paths.  Subkeys before main library */
		for(index=0;;index++) {
			int adjust;
			long reqdSize = dataSize;
			DWORD rc = RegEnumKey(newKey, index, keyBuf,MAX_PATH+1);
			if (rc) break;
			rc = RegQueryValue(newKey, keyBuf, dataBuf+off, &reqdSize);
			if (rc) break;
			if (reqdSize>1) { // If Nothing, or only '\0' copied.
				adjust = strlen(dataBuf+off);
				dataSize -= adjust;
				off += adjust;
				dataBuf[off++] = ';';
				dataBuf[off] = '\0';
				dataSize--;
			}
		}
		/* Additionally, win32s doesnt work as expected, so
		   the specific strlen() is required for 3.1. */
		rc = RegQueryValue(newKey, "", dataBuf+off, &dataSize);
		if (rc==ERROR_SUCCESS) {
			if (strlen(dataBuf)==0)
				free(dataBuf);
			else
				retval = dataBuf; // caller will free
		}
		else
			free(dataBuf);
	}

	if (newKey)
		RegCloseKey(newKey);
	return retval;
}
/* Return the initial python search path.  This is called once from
   initsys() to initialize sys.path.  The environment variable
   PYTHONPATH is fetched and the default path appended.  The default
   path may be passed to the preprocessor; if not, a system-dependent
   default is used. */

char *
Py_GetPath()
{
	char *path = getenv("PYTHONPATH");
	char *defpath = PYTHONPATH;
	static char *buf = NULL;
	char *p;
	int n;
	extern char *Py_GetProgramName();

	if (buf != NULL) {
		free(buf);
		buf = NULL;
	}

	if (path == NULL) {
		char *machinePath, *userPath;
		int machineLen, userLen;
		/* lookup the registry */
		BOOL bWin32s = PyWin_IsWin32s();

		if (bWin32s) { /* are we running under Windows 3.1 Win32s */
			/* only CLASSES_ROOT is supported */
			machinePath = getpythonregpath(HKEY_CLASSES_ROOT, TRUE); 
			userPath = NULL;
		} else {
			machinePath = getpythonregpath(HKEY_LOCAL_MACHINE, FALSE);
			userPath = getpythonregpath(HKEY_CURRENT_USER, FALSE);
		}
		if (machinePath==NULL && userPath==NULL) return defpath;
		machineLen = machinePath ? strlen(machinePath) : 0;
		userLen = userPath ? strlen(userPath) : 0;
		n = machineLen + userLen + 1;
		// this is a memory leak, as Python never frees it.  Only ever called once, so big deal!
		buf = malloc(n);
		if (buf == NULL)
			Py_FatalError("not enough memory to copy module search path");
		p = buf;
		*p = '\0';
		if (machineLen) {
			strcpy(p, machinePath);
			p += machineLen;
		}
		if (userLen) {
			if (machineLen)
				*p++ = DELIM;
			strcpy(p, userPath);
		}
		if (userPath) free(userPath);
		if (machinePath) free(machinePath);
	} else {
		
		buf = malloc(strlen(path)+1);
		if (buf == NULL)
			Py_FatalError("not enough memory to copy module search path");
		strcpy(buf, path);
	}
	return buf;
}

/* Similar for Makefile variables $prefix and $exec_prefix */

char *
Py_GetPrefix()
{
	return PREFIX;
}

char *
Py_GetExecPrefix()
{
	return EXEC_PREFIX;
}
