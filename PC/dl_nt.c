/*

Entry point for the Windows NT DLL.

About the only reason for having this, is so initall() can automatically
be called, removing that burden (and possible source of frustration if 
forgotten) from the programmer.

*/
#include "windows.h"

/* NT and Python share these */
#include "pyconfig.h"
#include "Python.h"

char dllVersionBuffer[16] = ""; // a private buffer

// Python Globals
HMODULE PyWin_DLLhModule = NULL;
const char *PyWin_DLLVersionString = dllVersionBuffer;


BOOL	WINAPI	DllMain (HANDLE hInst, 
						ULONG ul_reason_for_call,
						LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			PyWin_DLLhModule = hInst;
			// 1000 is a magic number I picked out of the air.  Could do with a #define, I spose...
			LoadString(hInst, 1000, dllVersionBuffer, sizeof(dllVersionBuffer));
			//initall();
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
