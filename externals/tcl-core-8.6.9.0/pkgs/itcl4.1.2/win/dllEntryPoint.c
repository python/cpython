/* 
 * dllEntryPoint.c --
 *
 *	This file implements the Dll entry point as needed by Windows.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef _MSC_VER
    /* Only do this when MSVC++ is compiling us. */
#   define DllEntryPoint DllMain
#   if defined(USE_TCL_STUBS) && (!defined(_MT) || !defined(_DLL) || defined(_DEBUG))
	/*
	 * This fixes a bug with how the Stubs library was compiled.
	 * The requirement for msvcrt.lib from tclstubXX.lib should
	 * be removed.
	 */
#	pragma comment(linker, "-nodefaultlib:msvcrt.lib")
#   endif
#endif

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *	This wrapper function is used by Windows to invoke the
 *	initialization code for the DLL.  If we are compiling
 *	with Visual C++, this routine will be renamed to DllMain.
 *
 * Results:
 *	Returns TRUE;
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifndef STATIC_BUILD

BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;		/* Library instance handle. */
    DWORD reason;		/* Reason this function is being called. */
    LPVOID reserved;		/* Not used. */
{
    return TRUE;
}

#endif