/********************************************************************

 import_nt.c 

  Win32 specific import code.

*/

#include "Python.h"
#include "osdefs.h"
#include <windows.h>
#include "importdl.h"
#include "malloc.h" // for alloca

extern const char *PyWin_DLLVersionString; // a string loaded from the DLL at startup.

/* Return whether this is Win32s, i.e., Win32 API on Win 3.1(1).
   This function is exported! */

BOOL PyWin_IsWin32s()
{
	static BOOL bIsWin32s = -1; /* flag as "not yet looked" */

	if (bIsWin32s == -1) {
		OSVERSIONINFO ver;
		ver.dwOSVersionInfoSize = sizeof(ver);
		GetVersionEx(&ver);
		bIsWin32s = ver.dwPlatformId == VER_PLATFORM_WIN32s;
	}
	return bIsWin32s;
}

FILE *PyWin_FindRegisteredModule( const char *moduleName, struct filedescr **ppFileDesc, char *pathBuf, int pathLen)
{
	char *moduleKey;
	char *pos;
	const char keyPrefix[] = "Software\\Python\\PythonCore\\";
	const char keySuffix[] = "\\Modules\\";
	struct filedescr *fdp = NULL;
	FILE *fp;
	int modNameSize = pathLen;
	int versionLen, moduleLen;
	HKEY keyBase = PyWin_IsWin32s() ? HKEY_CLASSES_ROOT : HKEY_LOCAL_MACHINE;

	// conceptually, this code is setting up:
	// sprintf(buf, "Software\\Python\\PythonCore\\%s\\Modules\\%s", PyWin_DLLVersionString, moduleName);
	// the sprintf would be clearer, but slower and less "length-safe"
	versionLen = strlen(PyWin_DLLVersionString);
	moduleLen = strlen(moduleName);
	// alloca == no free required, but memory only local to fn, also no heap fragmentation!
	moduleKey = alloca(sizeof(keyPrefix)-1 + versionLen + sizeof(keySuffix) + moduleLen); // chars only, plus 1 NULL.
	pos = moduleKey;
	memcpy(pos, keyPrefix, sizeof(keyPrefix)-1);
	pos += sizeof(keyPrefix)-1;
	memcpy(pos, PyWin_DLLVersionString, versionLen);
	pos +=versionLen;
	memcpy(pos, keySuffix, sizeof(keySuffix));
	pos += sizeof(keySuffix)-1;
	memcpy(pos, moduleName, moduleLen+1); // NULL comes with this one!

	if (RegQueryValue(keyBase, moduleKey, pathBuf, &modNameSize)!=ERROR_SUCCESS)
		return NULL;
	// use the file extension to locate the type entry.
	for (fdp = _PyImport_Filetab; fdp->suffix != NULL; fdp++) {
		int extLen=strlen(fdp->suffix);
		if (modNameSize>extLen && strnicmp(pathBuf+(modNameSize-extLen-1),fdp->suffix,extLen)==0)
			break;
	}
	if (fdp->suffix==NULL)
		return NULL;
	fp = fopen(pathBuf, fdp->mode);
	if (fp != NULL)
		*ppFileDesc = fdp;
	return fp;
}
