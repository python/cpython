/*
 * tclWinInt.h --
 *
 *	Declarations of Windows-specific shared variables and procedures.
 *
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TCLWININT
#define _TCLWININT

#include "tclInt.h"

#ifdef HAVE_NO_SEH
/*
 * Unlike Borland and Microsoft, we don't register exception handlers by
 * pushing registration records onto the runtime stack. Instead, we register
 * them by creating an TCLEXCEPTION_REGISTRATION within the activation record.
 */

typedef struct TCLEXCEPTION_REGISTRATION {
    struct TCLEXCEPTION_REGISTRATION *link;
    EXCEPTION_DISPOSITION (*handler)(
	    struct _EXCEPTION_RECORD*, void*, struct _CONTEXT*, void*);
    void *ebp;
    void *esp;
    int status;
} TCLEXCEPTION_REGISTRATION;
#endif

/*
 * Windows version dependend functions
 */
typedef struct TclWinProcs {
    BOOL (WINAPI *cancelSynchronousIo)(HANDLE);
} TclWinProcs;

MODULE_SCOPE TclWinProcs tclWinProcs;

/*
 * Some versions of Borland C have a define for the OSVERSIONINFO for
 * Win32s and for NT, but not for Windows 95.
 * Define VER_PLATFORM_WIN32_CE for those without newer headers.
 */

#ifndef VER_PLATFORM_WIN32_WINDOWS
#define VER_PLATFORM_WIN32_WINDOWS 1
#endif
#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE 3
#endif

#ifdef _WIN64
#         define TCL_I_MODIFIER        "I"
#else
#         define TCL_I_MODIFIER        ""
#endif

/*
 * Declarations of functions that are not accessible by way of the
 * stubs table.
 */

MODULE_SCOPE char	TclWinDriveLetterForVolMountPoint(
			    const TCHAR *mountPoint);
MODULE_SCOPE void	TclWinEncodingsCleanup();
MODULE_SCOPE void	TclWinInit(HINSTANCE hInst);
MODULE_SCOPE TclFile	TclWinMakeFile(HANDLE handle);
MODULE_SCOPE Tcl_Channel TclWinOpenConsoleChannel(HANDLE handle,
			    char *channelName, int permissions);
MODULE_SCOPE Tcl_Channel TclWinOpenFileChannel(HANDLE handle, char *channelName,
			    int permissions, int appendMode);
MODULE_SCOPE Tcl_Channel TclWinOpenSerialChannel(HANDLE handle,
			    char *channelName, int permissions);
MODULE_SCOPE HANDLE	TclWinSerialOpen(HANDLE handle, const TCHAR *name,
			    DWORD access);
MODULE_SCOPE int	TclWinSymLinkCopyDirectory(const TCHAR *LinkOriginal,
			    const TCHAR *LinkCopy);
MODULE_SCOPE int	TclWinSymLinkDelete(const TCHAR *LinkOriginal,
			    int linkOnly);
MODULE_SCOPE int        TclWinFileOwned(Tcl_Obj *);
#if defined(TCL_THREADS) && defined(USE_THREAD_ALLOC)
MODULE_SCOPE void	TclWinFreeAllocCache(void);
MODULE_SCOPE void	TclFreeAllocCache(void *);
MODULE_SCOPE Tcl_Mutex *TclpNewAllocMutex(void);
MODULE_SCOPE void *	TclpGetAllocCache(void);
MODULE_SCOPE void	TclpSetAllocCache(void *);
#endif /* TCL_THREADS */

MODULE_SCOPE const char*TclpGetUserName(Tcl_DString *bufferPtr);

/* Needed by tclWinFile.c and tclWinFCmd.c */
#ifndef FILE_ATTRIBUTE_REPARSE_POINT
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#endif

/*
 *----------------------------------------------------------------------
 * Declarations of helper-workers threaded facilities for a pipe based channel.
 *
 * Corresponding functionality provided in "tclWinPipe.c".
 *----------------------------------------------------------------------
 */

typedef struct TclPipeThreadInfo {
    HANDLE evControl;		/* Auto-reset event used by the main thread to
				 * signal when the pipe thread should attempt
				 * to do read/write operation. Additionally
				 * used as signal to stop (state set to -1) */
    volatile LONG state;	/* Indicates current state of the thread */
    ClientData clientData;	/* Referenced data of the main thread */
    HANDLE evWakeUp;		/* Optional wake-up event worker set by shutdown */
} TclPipeThreadInfo;


/* If pipe-workers will use some tcl subsystem, we can use ckalloc without
 * more overhead for finalize thread (should be executed anyway)
 *
 * #define _PTI_USE_CKALLOC 1
 */

/*
 * State of the pipe-worker.
 *
 * State PTI_STATE_STOP possible from idle state only, worker owns TI structure.
 * Otherwise PTI_STATE_END used (main thread hold ownership of the TI).
 */

#define PTI_STATE_IDLE	0	/* idle or not yet initialzed */
#define PTI_STATE_WORK	1	/* in work */
#define PTI_STATE_STOP	2	/* thread should stop work (owns TI structure) */
#define PTI_STATE_END	4	/* thread should stop work (worker is busy) */
#define PTI_STATE_DOWN  8	/* worker is down */


MODULE_SCOPE
TclPipeThreadInfo *	TclPipeThreadCreateTI(TclPipeThreadInfo **pipeTIPtr,
			    ClientData clientData, HANDLE wakeEvent);
MODULE_SCOPE int	TclPipeThreadWaitForSignal(TclPipeThreadInfo **pipeTIPtr);

static inline void
TclPipeThreadSignal(
    TclPipeThreadInfo **pipeTIPtr)
{
    TclPipeThreadInfo *pipeTI = *pipeTIPtr;
    if (pipeTI) {
	SetEvent(pipeTI->evControl);
    }
};

static inline int
TclPipeThreadIsAlive(
    TclPipeThreadInfo **pipeTIPtr)
{
    TclPipeThreadInfo *pipeTI = *pipeTIPtr;
    return (pipeTI && pipeTI->state != PTI_STATE_DOWN);
};

MODULE_SCOPE int	TclPipeThreadStopSignal(TclPipeThreadInfo **pipeTIPtr, HANDLE wakeEvent);
MODULE_SCOPE void	TclPipeThreadStop(TclPipeThreadInfo **pipeTIPtr, HANDLE hThread);
MODULE_SCOPE void	TclPipeThreadExit(TclPipeThreadInfo **pipeTIPtr);

#endif	/* _TCLWININT */
