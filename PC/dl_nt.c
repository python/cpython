/*

Entry point for the Windows NT DLL.

About the only reason for having this, is so initall() can automatically
be called, removing that burden (and possible source of frustration if
forgotten) from the programmer.

*/

#include "Python.h"
#include "windows.h"

#ifdef Py_ENABLE_SHARED

// Python Globals
HMODULE PyWin_DLLhModule = NULL;
const char *PyWin_DLLVersionString = MS_DLL_ID;

BOOL    WINAPI  DllMain (HANDLE hInst,
                                                ULONG ul_reason_for_call,
                                                LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            PyWin_DLLhModule = hInst;
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#endif /* Py_ENABLE_SHARED */
