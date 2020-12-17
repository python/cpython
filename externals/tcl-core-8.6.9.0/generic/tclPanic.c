/*
 * tclPanic.c --
 *
 *	Source code for the "Tcl_Panic" library procedure for Tcl; individual
 *	applications will probably call Tcl_SetPanicProc() to set an
 *	application-specific panic procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#if defined(_WIN32) || defined(__CYGWIN__)
    MODULE_SCOPE TCL_NORETURN void tclWinDebugPanic(const char *format, ...);
#endif

/*
 * The panicProc variable contains a pointer to an application specific panic
 * procedure.
 */

#if defined(__CYGWIN__)
static TCL_NORETURN Tcl_PanicProc *panicProc = tclWinDebugPanic;
#else
static TCL_NORETURN1 Tcl_PanicProc *panicProc = NULL;
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetPanicProc --
 *
 *	Replace the default panic behavior with the specified function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the panicProc variable.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetPanicProc(
    TCL_NORETURN1 Tcl_PanicProc *proc)
{
#if defined(_WIN32)
    /* tclWinDebugPanic only installs if there is no panicProc yet. */
    if ((proc != tclWinDebugPanic) || (panicProc == NULL))
#elif defined(__CYGWIN__)
    if (proc == NULL)
	panicProc = tclWinDebugPanic;
    else
#endif
    panicProc = proc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PanicVA --
 *
 *	Print an error message and kill the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process dies, entering the debugger if possible.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_PanicVA(
    const char *format,		/* Format string, suitable for passing to
				 * fprintf. */
    va_list argList)		/* Variable argument list. */
{
    char *arg1, *arg2, *arg3;	/* Additional arguments (variable in number)
				 * to pass to fprintf. */
    char *arg4, *arg5, *arg6, *arg7, *arg8;

    arg1 = va_arg(argList, char *);
    arg2 = va_arg(argList, char *);
    arg3 = va_arg(argList, char *);
    arg4 = va_arg(argList, char *);
    arg5 = va_arg(argList, char *);
    arg6 = va_arg(argList, char *);
    arg7 = va_arg(argList, char *);
    arg8 = va_arg(argList, char *);

    if (panicProc != NULL) {
	panicProc(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
#ifdef _WIN32
    } else if (IsDebuggerPresent()) {
	tclWinDebugPanic(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
#endif
    } else {
	fprintf(stderr, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7,
		arg8);
	fprintf(stderr, "\n");
	fflush(stderr);
#if defined(_WIN32) || defined(__CYGWIN__)
#   if defined(__GNUC__)
	__builtin_trap();
#   elif defined(_WIN64)
	__debugbreak();
#   elif defined(_MSC_VER) && defined (_M_IX86)
	_asm {int 3}
#   else
	DebugBreak();
#   endif
#endif
#if defined(_WIN32)
	ExitProcess(1);
#else
	abort();
#endif
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Panic --
 *
 *	Print an error message and kill the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process dies, entering the debugger if possible.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */

/*
 * The following comment is here so that Coverity's static analizer knows that
 * a Tcl_Panic() call can never return and avoids lots of false positives.
 */

/* coverity[+kill] */
void
Tcl_Panic(
    const char *format,
    ...)
{
    va_list argList;

    va_start(argList, format);
    Tcl_PanicVA(format, argList);
    va_end (argList);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
