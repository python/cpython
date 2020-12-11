/*
 * --------------------------------------------------------------------------
 * tclthreadInt.h --
 *
 * Global internal header file for the thread extension.
 *
 * Copyright (c) 2002 ActiveState Corporation.
 * Copyright (c) 2002 by Zoran Vasiljevic.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ---------------------------------------------------------------------------
 */

#ifndef _TCL_THREAD_INT_H_
#define _TCL_THREAD_INT_H_

#include "tclThread.h"
#include <stdlib.h> /* For strtoul */
#include <string.h> /* For memset and friends */

/*
 * MSVC 8.0 started to mark many standard C library functions depreciated
 * including the *printf family and others. Tell it to shut up.
 * (_MSC_VER is 1200 for VC6, 1300 or 1310 for vc7.net, 1400 for 8.0)
 */
#if defined(_MSC_VER)
#   pragma warning(disable:4244)
#   if _MSC_VER >= 1400
#	pragma warning(disable:4267)
#	pragma warning(disable:4996)
#   endif
#endif

/*
 * Used to tag functions that are only to be visible within the module being
 * built and not outside it (where this is supported by the linker).
 */

#ifndef MODULE_SCOPE
#   ifdef __cplusplus
#       define MODULE_SCOPE extern "C"
#   else
#       define MODULE_SCOPE extern
#   endif
#endif

/*
 * For linking against NaviServer/AOLserver require V4 at least
 */

#ifdef NS_AOLSERVER
# include <ns.h>
# if !defined(NS_MAJOR_VERSION) || NS_MAJOR_VERSION < 4
#  error "unsupported NaviServer/AOLserver version"
# endif
#endif

/*
 * Allow for some command names customization.
 * Only thread:: and tpool:: are handled here.
 * Shared variable commands are more complicated.
 * Look into the threadSvCmd.h for more info.
 */

#define THREAD_CMD_PREFIX "thread::"
#define TPOOL_CMD_PREFIX  "tpool::"

/*
 * Exported from threadSvCmd.c file.
 */

MODULE_SCOPE int Sv_Init(Tcl_Interp *interp);

/*
 * Exported from threadSpCmd.c file.
 */

MODULE_SCOPE int Sp_Init(Tcl_Interp *interp);

/*
 * Exported from threadPoolCmd.c file.
 */

MODULE_SCOPE int Tpool_Init(Tcl_Interp *interp);

/*
 * Macros for splicing in/out of linked lists
 */

#define SpliceIn(a,b)                          \
    (a)->nextPtr = (b);                        \
    if ((b) != NULL)                           \
        (b)->prevPtr = (a);                    \
    (a)->prevPtr = NULL, (b) = (a)

#define SpliceOut(a,b)                         \
    if ((a)->prevPtr != NULL)                  \
        (a)->prevPtr->nextPtr = (a)->nextPtr;  \
    else                                       \
        (b) = (a)->nextPtr;                    \
    if ((a)->nextPtr != NULL)                  \
        (a)->nextPtr->prevPtr = (a)->prevPtr

/*
 * Version macros
 */

#define TCL_MINIMUM_VERSION(major,minor) \
  ((TCL_MAJOR_VERSION > (major)) || \
    ((TCL_MAJOR_VERSION == (major)) && (TCL_MINOR_VERSION >= (minor))))

/*
 * Utility macros
 */

#define TCL_CMD(a,b,c) \
  if (Tcl_CreateObjCommand((a),(b),(c),NULL, NULL) == NULL) \
    return TCL_ERROR

#define OPT_CMP(a,b) \
  ((a) && (b) && (*(a)==*(b)) && (*(a+1)==*(b+1)) && (!strcmp((a),(b))))

#ifndef TCL_TSD_INIT
#define TCL_TSD_INIT(keyPtr) \
  (ThreadSpecificData*)Tcl_GetThreadData((keyPtr),sizeof(ThreadSpecificData))
#endif

/*
 * Structure to pass to NsThread_Init. This holds the module
 * and virtual server name for proper interp initializations.
 */

typedef struct {
    char *modname;
    char *server;
} NsThreadInterpData;

/*
 * Handle binary compatibility regarding
 * Tcl_GetErrorLine in 8.x
 * See Tcl bug #3562640.
 */

MODULE_SCOPE int threadTclVersion;

typedef struct {
    void *unused1;
    void *unused2;
    int errorLine;
} tclInterpType;

#if defined(TCL_TIP285) && defined(USE_TCL_STUBS)
# undef Tcl_GetErrorLine
# define Tcl_GetErrorLine(interp) ((threadTclVersion>85)? \
    ((int (*)(Tcl_Interp *))((&(tclStubsPtr->tcl_PkgProvideEx))[605]))(interp): \
    (((tclInterpType *)(interp))->errorLine))
/* TIP #270 */
# undef Tcl_AddErrorInfo
# define Tcl_AddErrorInfo(interp, msg) ((threadTclVersion>85)? \
    ((void (*)(Tcl_Interp *, Tcl_Obj *))((&(tclStubsPtr->tcl_PkgProvideEx))[574]))(interp, Tcl_NewStringObj(msg, -1)): \
    ((void (*)(Tcl_Interp *, const char *))((&(tclStubsPtr->tcl_PkgProvideEx))[66]))(interp, msg))
/* TIP #337 */
# undef Tcl_BackgroundException
# define Tcl_BackgroundException(interp, result) ((threadTclVersion>85)? \
    ((void (*)(Tcl_Interp *, int))((&(tclStubsPtr->tcl_PkgProvideEx))[609]))(interp, result): \
    ((void (*)(Tcl_Interp *))((&(tclStubsPtr->tcl_PkgProvideEx))[76]))(interp))
#elif !TCL_MINIMUM_VERSION(8,6)
  /* 8.5, 8.4, or less - Emulate access to the error-line information */
# define Tcl_GetErrorLine(interp) (((tclInterpType *)(interp))->errorLine)
#endif

/* When running on Tcl >= 8.7, make sure that Thread still runs when Tcl is compiled
 * with -DTCL_NO_DEPRECATED=1. Stub entries for Tcl_SetIntObj/Tcl_NewIntObj are NULL then.
 * Just use Tcl_SetWideIntObj/Tcl_NewWideIntObj in stead. We don't simply want to use
 * Tcl_SetWideIntObj/Tcl_NewWideIntObj always, since extensions might not expect to
 * get an actual "wideInt".
 */
#if defined(USE_TCL_STUBS)
# undef Tcl_SetIntObj
# define Tcl_SetIntObj(objPtr, value) ((threadTclVersion>86)? \
  ((void (*)(Tcl_Obj *, Tcl_WideInt))((&(tclStubsPtr->tcl_PkgProvideEx))[489]))(objPtr, (int)(value)): \
  ((void (*)(Tcl_Obj *, int))((&(tclStubsPtr->tcl_PkgProvideEx))[61]))(objPtr, value))
# undef Tcl_NewIntObj
# define Tcl_NewIntObj(value) ((threadTclVersion>86)? \
  ((Tcl_Obj * (*)(Tcl_WideInt))((&(tclStubsPtr->tcl_PkgProvideEx))[488]))((int)(value)): \
  ((Tcl_Obj * (*)(int))((&(tclStubsPtr->tcl_PkgProvideEx))[52]))(value))
# undef Tcl_GetUnicodeFromObj
# define Tcl_GetUnicodeFromObj ((((&(tclStubsPtr->tcl_PkgProvideEx))[378]) != ((&(tclStubsPtr->tcl_PkgProvideEx))[434])) ? \
  ((void (*)(Tcl_Obj *, int *))((&(tclStubsPtr->tcl_PkgProvideEx))[434])) : ((void (*)(Tcl_Obj *, int *)) NULL))
#endif

#endif /* _TCL_THREAD_INT_H_ */
