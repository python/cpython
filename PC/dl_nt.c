/*

Entry point for the Windows NT DLL.

About the only reason for having this, is so initall() can automatically
be called, removing that burden (and possible source of frustration if
forgotten) from the programmer.

*/

#include "Python.h"
#include "windows.h"

#ifdef Py_ENABLE_SHARED
#ifdef MS_DLL_ID
// The string is available at build, so fill the buffer immediately
char dllVersionBuffer[16] = MS_DLL_ID;
#else
char dllVersionBuffer[16] = ""; // a private buffer
#endif

// Python Globals
HMODULE PyWin_DLLhModule = NULL;
const char *PyWin_DLLVersionString = dllVersionBuffer;

BOOL    WINAPI  DllMain (HANDLE hInst,
                                                ULONG ul_reason_for_call,
                                                LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            PyWin_DLLhModule = hInst;
#ifndef MS_DLL_ID
            // If we have MS_DLL_ID, we don't need to load the string.
            // 1000 is a magic number I picked out of the air.  Could do with a #define, I spose...
            LoadString(hInst, 1000, dllVersionBuffer, sizeof(dllVersionBuffer));
#endif
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#endif /* Py_ENABLE_SHARED */
