/********************************************************************

 import_nt.c 

  Win32 specific import code.

*/

#include "Python.h"
#include "osdefs.h"
#include <windows.h>
#include "importdl.h"

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
	char moduleKey[128];
	struct filedescr *fdp = NULL;
	FILE *fp;
	int modNameSize = pathLen;
	HKEY keyBase = PyWin_IsWin32s() ? HKEY_CLASSES_ROOT : HKEY_LOCAL_MACHINE;
	strcpy(moduleKey, "Software\\Python\\PythonCore\\" MS_DLL_ID "\\Modules\\");
	strcat(moduleKey, moduleName);
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
