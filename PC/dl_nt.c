/*

Entry point for the Windows NT DLL.

About the only reason for having this, is so initall() can automatically
be called, removing that burden (and possible source of frustration if
forgotten) from the programmer.

*/

#include "Python.h"
#include "windows.h"

#ifdef Py_ENABLE_SHARED
char dllVersionBuffer[16] = ""; // a private buffer

// Python Globals
HMODULE PyWin_DLLhModule = NULL;
const char *PyWin_DLLVersionString = dllVersionBuffer;

// Windows "Activation Context" work:
// Our .pyd extension modules are generally built without a manifest (ie,
// those included with Python and those built with a default distutils.
// This requires we perform some "activation context" magic when loading our
// extensions.  In summary:
// * As our DLL loads we save the context being used.
// * Before loading our extensions we re-activate our saved context.
// * After extension load is complete we restore the old context.
// As an added complication, this magic only works on XP or later - we simply
// use the existence (or not) of the relevant function pointers from kernel32.
// See bug 4566 (http://python.org/sf/4566) for more details.

typedef BOOL (WINAPI * PFN_GETCURRENTACTCTX)(HANDLE *);
typedef BOOL (WINAPI * PFN_ACTIVATEACTCTX)(HANDLE, ULONG_PTR *);
typedef BOOL (WINAPI * PFN_DEACTIVATEACTCTX)(DWORD, ULONG_PTR);
typedef BOOL (WINAPI * PFN_ADDREFACTCTX)(HANDLE);
typedef BOOL (WINAPI * PFN_RELEASEACTCTX)(HANDLE);

// locals and function pointers for this activation context magic.
static HANDLE PyWin_DLLhActivationContext = NULL; // one day it might be public
static PFN_GETCURRENTACTCTX pfnGetCurrentActCtx = NULL;
static PFN_ACTIVATEACTCTX pfnActivateActCtx = NULL;
static PFN_DEACTIVATEACTCTX pfnDeactivateActCtx = NULL;
static PFN_ADDREFACTCTX pfnAddRefActCtx = NULL;
static PFN_RELEASEACTCTX pfnReleaseActCtx = NULL;

void _LoadActCtxPointers()
{
    HINSTANCE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32)
        pfnGetCurrentActCtx = (PFN_GETCURRENTACTCTX) GetProcAddress(hKernel32, "GetCurrentActCtx");
    // If we can't load GetCurrentActCtx (ie, pre XP) , don't bother with the rest.
    if (pfnGetCurrentActCtx) {
        pfnActivateActCtx = (PFN_ACTIVATEACTCTX) GetProcAddress(hKernel32, "ActivateActCtx");
        pfnDeactivateActCtx = (PFN_DEACTIVATEACTCTX) GetProcAddress(hKernel32, "DeactivateActCtx");
        pfnAddRefActCtx = (PFN_ADDREFACTCTX) GetProcAddress(hKernel32, "AddRefActCtx");
        pfnReleaseActCtx = (PFN_RELEASEACTCTX) GetProcAddress(hKernel32, "ReleaseActCtx");
    }
}

ULONG_PTR _Py_ActivateActCtx()
{
    ULONG_PTR ret = 0;
    if (PyWin_DLLhActivationContext && pfnActivateActCtx)
        if (!(*pfnActivateActCtx)(PyWin_DLLhActivationContext, &ret)) {
            OutputDebugString("Python failed to activate the activation context before loading a DLL\n");
            ret = 0; // no promise the failing function didn't change it!
        }
    return ret;
}

void _Py_DeactivateActCtx(ULONG_PTR cookie)
{
    if (cookie && pfnDeactivateActCtx)
        if (!(*pfnDeactivateActCtx)(0, cookie))
            OutputDebugString("Python failed to de-activate the activation context\n");
}

BOOL    WINAPI  DllMain (HANDLE hInst,
                                                ULONG ul_reason_for_call,
                                                LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            PyWin_DLLhModule = hInst;
            // 1000 is a magic number I picked out of the air.  Could do with a #define, I spose...
            LoadString(hInst, 1000, dllVersionBuffer, sizeof(dllVersionBuffer));

            // and capture our activation context for use when loading extensions.
            _LoadActCtxPointers();
            if (pfnGetCurrentActCtx && pfnAddRefActCtx)
                if ((*pfnGetCurrentActCtx)(&PyWin_DLLhActivationContext))
                    if (!(*pfnAddRefActCtx)(PyWin_DLLhActivationContext))
                        OutputDebugString("Python failed to load the default activation context\n");
            break;

        case DLL_PROCESS_DETACH:
            if (pfnReleaseActCtx)
                (*pfnReleaseActCtx)(PyWin_DLLhActivationContext);
            break;
    }
    return TRUE;
}

#endif /* Py_ENABLE_SHARED */
