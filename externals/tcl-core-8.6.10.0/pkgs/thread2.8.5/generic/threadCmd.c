/*
 * threadCmd.c --
 *
 * This file implements the Tcl thread commands that allow script
 * level access to threading. It will not load into a core that was
 * not compiled for thread support.
 *
 * See http://www.tcl.tk/doc/howto/thread_model.html
 *
 * Some of this code is based on work done by Richard Hipp on behalf of
 * Conservation Through Innovation, Limited, with their permission.
 *
 * Copyright (c) 1998 by Sun Microsystems, Inc.
 * Copyright (c) 1999,2000 by Scriptics Corporation.
 * Copyright (c) 2002 by Zoran Vasiljevic.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ----------------------------------------------------------------------------
 */

#include "tclThreadInt.h"

/*
 * Provide package version in build contexts which do not provide
 * -DPACKAGE_VERSION, like building a shell with the Thread object
 * files built as part of that shell. Example: basekits.
 */
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "2.8.5"
#endif

/*
 * Check if this is Tcl 8.5 or higher.  In that case, we will have the TIP
 * #143 APIs (i.e. interpreter resource limiting) available.
 */

#ifndef TCL_TIP143
# if TCL_MINIMUM_VERSION(8,5)
#  define TCL_TIP143
# endif
#endif

/*
 * If TIP #143 support is enabled and we are compiling against a pre-Tcl 8.5
 * core, hard-wire the necessary APIs using the "well-known" offsets into the
 * stubs table.
 */

#define haveInterpLimit (threadTclVersion>=85)
#if defined(TCL_TIP143) && !TCL_MINIMUM_VERSION(8,5)
# if defined(USE_TCL_STUBS)
#  define Tcl_LimitExceeded ((int (*)(Tcl_Interp *)) \
     ((&(tclStubsPtr->tcl_PkgProvideEx))[524]))
# else
#  error "Supporting TIP #143 requires USE_TCL_STUBS before Tcl 8.5"
# endif
#endif

/*
 * Check if this is Tcl 8.6 or higher.  In that case, we will have the TIP
 * #285 APIs (i.e. asynchronous script cancellation) available.
 */

#define haveInterpCancel (threadTclVersion>=86)
#ifndef TCL_TIP285
# if TCL_MINIMUM_VERSION(8,6)
#  define TCL_TIP285
# endif
#endif

/*
 * If TIP #285 support is enabled and we are compiling against a pre-Tcl 8.6
 * core, hard-wire the necessary APIs using the "well-known" offsets into the
 * stubs table.
 */

#if defined(TCL_TIP285) && !TCL_MINIMUM_VERSION(8,6)
# if defined(USE_TCL_STUBS)
#  define TCL_CANCEL_UNWIND 0x100000
#  define Tcl_CancelEval ((int (*)(Tcl_Interp *, Tcl_Obj *, ClientData, int)) \
     ((&(tclStubsPtr->tcl_PkgProvideEx))[580]))
#  define Tcl_Canceled ((int (*)(Tcl_Interp *, int)) \
     ((&(tclStubsPtr->tcl_PkgProvideEx))[581]))
# else
#  error "Supporting TIP #285 requires USE_TCL_STUBS before Tcl 8.6"
# endif
#endif

/*
 * Access to the list of threads and to the thread send results
 * (defined below) is guarded by this mutex.
 */

TCL_DECLARE_MUTEX(threadMutex)

/*
 * Each thread has an single instance of the following structure. There
 * is one instance of this structure per thread even if that thread contains
 * multiple interpreters. The interpreter identified by this structure is
 * the main interpreter for the thread. The main interpreter is the one that
 * will process any messages received by a thread. Any interpreter can send
 * messages but only the main interpreter can receive them, unless you're
 * not doing asynchronous script backfiring. In such cases the caller might
 * signal the thread to which interpreter the result should be delivered.
 */

typedef struct ThreadSpecificData {
    Tcl_ThreadId threadId;                /* The real ID of this thread */
    Tcl_Interp *interp;                   /* Main interp for this thread */
    Tcl_Condition doOneEvent;             /* Signalled just before running
                                             an event from the event loop */
    int flags;                            /* One of the ThreadFlags below */
    size_t refCount;                      /* Used for thread reservation */
    int eventsPending;                    /* # of unprocessed events */
    int maxEventsCount;                   /* Maximum # of pending events */
    struct ThreadEventResult  *result;
    struct ThreadSpecificData *nextPtr;
    struct ThreadSpecificData *prevPtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

#define THREAD_FLAGS_NONE          0      /* None */
#define THREAD_FLAGS_STOPPED       1      /* Thread is being stopped */
#define THREAD_FLAGS_INERROR       2      /* Thread is in error */
#define THREAD_FLAGS_UNWINDONERROR 4      /* Thread unwinds on script error */

#define THREAD_RESERVE             1      /* Reserves the thread */
#define THREAD_RELEASE             2      /* Releases the thread */

/*
 * Length of storage for building the Tcl handle for the thread.
 */

#define THREAD_HNDLPREFIX  "tid"
#define THREAD_HNDLMAXLEN  32

/*
 * This list is used to list all threads that have interpreters.
 */

static struct ThreadSpecificData *threadList = NULL;

/*
 * Used to represent the empty result.
 */

static char *threadEmptyResult = (char *)"";

int threadTclVersion = 0;

/*
 * An instance of the following structure contains all information that is
 * passed into a new thread when the thread is created using either the
 * "thread create" Tcl command or the ThreadCreate() C function.
 */

typedef struct ThreadCtrl {
    char *script;                         /* Script to execute */
    int flags;                            /* Initial value of the "flags"
                                           * field in ThreadSpecificData */
    Tcl_Condition condWait;               /* Condition variable used to
                                           * sync parent and child threads */
    ClientData cd;                        /* Opaque ptr to pass to thread */
} ThreadCtrl;

/*
 * Structure holding result of the command executed in target thread.
 */

typedef struct ThreadEventResult {
    Tcl_Condition done;                   /* Set when the script completes */
    int code;                             /* Return value of the function */
    char *result;                         /* Result from the function */
    char *errorInfo;                      /* Copy of errorInfo variable */
    char *errorCode;                      /* Copy of errorCode variable */
    Tcl_ThreadId srcThreadId;             /* Id of sender, if it dies */
    Tcl_ThreadId dstThreadId;             /* Id of target, if it dies */
    struct ThreadEvent *eventPtr;         /* Back pointer */
    struct ThreadEventResult *nextPtr;    /* List for cleanup */
    struct ThreadEventResult *prevPtr;
} ThreadEventResult;

/*
 * This list links all active ThreadEventResult structures. This way
 * an exiting thread can inform all threads waiting on jobs posted to
 * his event queue that it is dying, so they might stop waiting.
 */

static ThreadEventResult *resultList;

/*
 * This is the event used to send commands to other threads.
 */

typedef struct ThreadEvent {
    Tcl_Event event;                      /* Must be first */
    struct ThreadSendData *sendData;      /* See below */
    struct ThreadClbkData *clbkData;      /* See below */
    struct ThreadEventResult *resultPtr;  /* To communicate the result back.
                                           * NULL if we don't care about it */
} ThreadEvent;

typedef int  (ThreadSendProc) (Tcl_Interp*, ClientData);
typedef void (ThreadSendFree) (ClientData);

static ThreadSendProc ThreadSendEval;     /* Does a regular Tcl_Eval */
static ThreadSendProc ThreadClbkSetVar;   /* Sets the named variable */

/*
 * These structures are used to communicate commands between source and target
 * threads. The ThreadSendData is used for source->target command passing,
 * while the ThreadClbkData is used for doing asynchronous callbacks.
 *
 * Important: structures below must have first three elements identical!
 */

typedef struct ThreadSendData {
    ThreadSendProc *execProc;             /* Func to exec in remote thread */
    ClientData clientData;                /* Ptr to pass to send function */
    ThreadSendFree *freeProc;             /* Function to free client data */
     /* ---- */
    Tcl_Interp *interp;                   /* Interp to run the command */
} ThreadSendData;

typedef struct ThreadClbkData {
    ThreadSendProc *execProc;             /* The callback function */
    ClientData clientData;                /* Ptr to pass to clbk function */
    ThreadSendFree *freeProc;             /* Function to free client data */
    /* ---- */
    Tcl_Interp *interp;                   /* Interp to run the command */
    Tcl_ThreadId threadId;                /* Thread where to post callback */
    ThreadEventResult result;             /* Returns result asynchronously */
} ThreadClbkData;

/*
 * Event used to transfer a channel between threads.
 */
typedef struct TransferEvent {
    Tcl_Event event;                      /* Must be first */
    Tcl_Channel chan;                     /* The channel to transfer */
    struct TransferResult *resultPtr;     /* To communicate the result */
} TransferEvent;

typedef struct TransferResult {
    Tcl_Condition done;                   /* Set when transfer is done */
    int resultCode;                       /* Set to TCL_OK or TCL_ERROR when
                                             the transfer is done. Def = -1 */
    char *resultMsg;                      /* Initialized to NULL. Set to a
                                             allocated string by the target
                                             thread in case of an error  */
    Tcl_ThreadId srcThreadId;             /* Id of src thread, if it dies */
    Tcl_ThreadId dstThreadId;             /* Id of tgt thread, if it dies */
    struct TransferEvent *eventPtr;       /* Back pointer */
    struct TransferResult *nextPtr;       /* Next in the linked list */
    struct TransferResult *prevPtr;       /* Previous in the linked list */
} TransferResult;

static TransferResult *transferList;

/*
 * This is for simple error handling when a thread script exits badly.
 */

static Tcl_ThreadId errorThreadId; /* Id of thread to post error message */
static char *errorProcString;      /* Tcl script to run when reporting error */

/*
 * Definition of flags for ThreadSend.
 */

#define THREAD_SEND_WAIT (1<<1)
#define THREAD_SEND_HEAD (1<<2)
#define THREAD_SEND_CLBK (1<<3)

#ifdef BUILD_thread
# undef  TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * Miscellaneous functions used within this file
 */

static Tcl_EventDeleteProc ThreadDeleteEvent;

static Tcl_ThreadCreateType
NewThread(ClientData clientData);

static ThreadSpecificData*
ThreadExistsInner(Tcl_ThreadId id);

static int
ThreadInit(Tcl_Interp *interp);

static int
ThreadCreate(Tcl_Interp *interp,
                               const char *script,
                               int stacksize,
                               int flags,
                               int preserve);
static int
ThreadSend(Tcl_Interp *interp,
                               Tcl_ThreadId id,
                               ThreadSendData *sendPtr,
                               ThreadClbkData *clbkPtr,
                               int flags);
static void
ThreadSetResult(Tcl_Interp *interp,
                               int code,
                               ThreadEventResult *resultPtr);
static int
ThreadGetOption(Tcl_Interp *interp,
                               Tcl_ThreadId id,
                               char *option,
                               Tcl_DString *ds);
static int
ThreadSetOption(Tcl_Interp *interp,
                               Tcl_ThreadId id,
                               char *option,
                               char *value);
static int
ThreadReserve(Tcl_Interp *interp,
                               Tcl_ThreadId id,
                               int operation,
                               int wait);
static int
ThreadEventProc(Tcl_Event *evPtr,
                               int mask);
static int
ThreadWait(Tcl_Interp *interp);

static int
ThreadExists(Tcl_ThreadId id);

static int
ThreadList(Tcl_Interp *interp,
                               Tcl_ThreadId **thrIdArray);
static void
ThreadErrorProc(Tcl_Interp *interp);

static void
ThreadFreeProc(ClientData clientData);

static void
ThreadIdleProc(ClientData clientData);

static void
ThreadExitProc(ClientData clientData);

static void
ThreadFreeError(ClientData clientData);

static void
ListRemove(ThreadSpecificData *tsdPtr);

static void
ListRemoveInner(ThreadSpecificData *tsdPtr);

static void
ListUpdate(ThreadSpecificData *tsdPtr);

static void
ListUpdateInner(ThreadSpecificData *tsdPtr);

static int
ThreadJoin(Tcl_Interp *interp,
                               Tcl_ThreadId id);
static int
ThreadTransfer(Tcl_Interp *interp,
                               Tcl_ThreadId id,
                               Tcl_Channel chan);
static int
ThreadDetach(Tcl_Interp *interp,
                               Tcl_Channel chan);
static int
ThreadAttach(Tcl_Interp *interp,
                               char *chanName);
static int
TransferEventProc(Tcl_Event *evPtr,
                               int mask);

static void
ThreadGetHandle(Tcl_ThreadId,
                               char *handlePtr);

static int
ThreadGetId(Tcl_Interp *interp,
                               Tcl_Obj *handleObj,
                               Tcl_ThreadId *thrIdPtr);
static void
ErrorNoSuchThread(Tcl_Interp *interp,
                               Tcl_ThreadId thrId);
static void
ThreadCutChannel(Tcl_Interp *interp,
                               Tcl_Channel channel);

#ifdef TCL_TIP285
static int
ThreadCancel(Tcl_Interp *interp,
                               Tcl_ThreadId thrId,
                               const char *result,
                               int flags);
#endif

/*
 * Functions implementing Tcl commands
 */

static Tcl_ObjCmdProc ThreadCreateObjCmd;
static Tcl_ObjCmdProc ThreadReserveObjCmd;
static Tcl_ObjCmdProc ThreadReleaseObjCmd;
static Tcl_ObjCmdProc ThreadSendObjCmd;
static Tcl_ObjCmdProc ThreadBroadcastObjCmd;
static Tcl_ObjCmdProc ThreadUnwindObjCmd;
static Tcl_ObjCmdProc ThreadExitObjCmd;
static Tcl_ObjCmdProc ThreadIdObjCmd;
static Tcl_ObjCmdProc ThreadNamesObjCmd;
static Tcl_ObjCmdProc ThreadWaitObjCmd;
static Tcl_ObjCmdProc ThreadExistsObjCmd;
static Tcl_ObjCmdProc ThreadConfigureObjCmd;
static Tcl_ObjCmdProc ThreadErrorProcObjCmd;
static Tcl_ObjCmdProc ThreadJoinObjCmd;
static Tcl_ObjCmdProc ThreadTransferObjCmd;
static Tcl_ObjCmdProc ThreadDetachObjCmd;
static Tcl_ObjCmdProc ThreadAttachObjCmd;

#ifdef TCL_TIP285
static Tcl_ObjCmdProc ThreadCancelObjCmd;
#endif

static int
ThreadInit(
    Tcl_Interp *interp /* The current Tcl interpreter */
) {
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }

    if (!threadTclVersion) {

        /*
         * Check whether we are running threaded Tcl.
         * Get the current core version to decide whether to use
         * some lately introduced core features or to back-off.
         */

        int major, minor;

        Tcl_MutexLock(&threadMutex);
        if (threadMutex == NULL){
            /* If threadMutex==NULL here, it means that Tcl_MutexLock() is
             * a dummy function, which is the case in unthreaded Tcl */
            const char *msg = "Tcl core wasn't compiled for threading";
            Tcl_SetObjResult(interp, Tcl_NewStringObj(msg, -1));
            return TCL_ERROR;
        }
        Tcl_GetVersion(&major, &minor, NULL, NULL);
        threadTclVersion = 10 * major + minor;
        Tcl_MutexUnlock(&threadMutex);
    }

    TCL_CMD(interp, THREAD_CMD_PREFIX"create",    ThreadCreateObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"send",      ThreadSendObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"broadcast", ThreadBroadcastObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"exit",      ThreadExitObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"unwind",    ThreadUnwindObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"id",        ThreadIdObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"names",     ThreadNamesObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"exists",    ThreadExistsObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"wait",      ThreadWaitObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"configure", ThreadConfigureObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"errorproc", ThreadErrorProcObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"preserve",  ThreadReserveObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"release",   ThreadReleaseObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"join",      ThreadJoinObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"transfer",  ThreadTransferObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"detach",    ThreadDetachObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"attach",    ThreadAttachObjCmd);
#ifdef TCL_TIP285
    TCL_CMD(interp, THREAD_CMD_PREFIX"cancel",    ThreadCancelObjCmd);
#endif

    /*
     * Add shared variable commands
     */

    Sv_Init(interp);

    /*
     * Add commands to access thread
     * synchronization primitives.
     */

    Sp_Init(interp);

    /*
     * Add threadpool commands.
     */

    Tpool_Init(interp);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Thread_Init --
 *
 *  Initialize the thread commands.
 *
 * Results:
 *  TCL_OK if the package was properly initialized.
 *
 * Side effects:
 *  Adds package commands to the current interp.
 *
 *----------------------------------------------------------------------
 */

DLLEXPORT int
Thread_Init(
    Tcl_Interp *interp /* The current Tcl interpreter */
) {
    int status = ThreadInit(interp);

    if (status != TCL_OK) {
        return status;
    }

    return Tcl_PkgProvideEx(interp, "Thread", PACKAGE_VERSION, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * Init --
 *
 *  Make sure internal list of threads references the current thread.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  The list of threads is initialized to include the current thread.
 *
 *----------------------------------------------------------------------
 */

static void
Init(
    Tcl_Interp *interp         /* Current interpreter. */
) {
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->interp == NULL) {
        memset(tsdPtr, 0, sizeof(ThreadSpecificData));
        tsdPtr->interp = interp;
        ListUpdate(tsdPtr);
        Tcl_CreateThreadExitHandler(ThreadExitProc,
                                    threadEmptyResult);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadCreateObjCmd --
 *
 *  This procedure is invoked to process the "thread::create" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadCreateObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int argc, rsrv = 0;
    const char *arg, *script;
    int flags = TCL_THREAD_NOFLAGS;

    Init(interp);

    /*
     * Syntax: thread::create ?-joinable? ?-preserved? ?script?
     */

    script = THREAD_CMD_PREFIX"wait";

    for (argc = 1; argc < objc; argc++) {
        arg = Tcl_GetString(objv[argc]);
        if (OPT_CMP(arg, "--")) {
            argc++;
            if ((argc + 1) == objc) {
                script = Tcl_GetString(objv[argc]);
            } else {
                goto usage;
            }
            break;
        } else if (OPT_CMP(arg, "-joinable")) {
            flags |= TCL_THREAD_JOINABLE;
        } else if (OPT_CMP(arg, "-preserved")) {
            rsrv = 1;
        } else if ((argc + 1) == objc) {
            script = Tcl_GetString(objv[argc]);
        } else {
            goto usage;
        }
    }

    return ThreadCreate(interp, script, TCL_THREAD_STACK_DEFAULT, flags, rsrv);

 usage:
    Tcl_WrongNumArgs(interp, 1, objv, "?-joinable? ?script?");
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadReserveObjCmd --
 *
 *  This procedure is invoked to process the "thread::preserve" and
 *  "thread::release" Tcl commands, depending on the flag passed by
 *  the ClientData argument. See the user documentation for details
 *  on what those command do.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadReserveObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    Tcl_ThreadId thrId = NULL;

    Init(interp);

    if (objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?threadId?");
        return TCL_ERROR;
    }
    if (objc == 2) {
        if (ThreadGetId(interp, objv[1], &thrId) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    return ThreadReserve(interp, thrId, THREAD_RESERVE, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadReleaseObjCmd --
 *
 *  This procedure is invoked to process the "thread::release" Tcl
 *  command. See the user documentation for details on what this
 *  command does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadReleaseObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int wait = 0;
    Tcl_ThreadId thrId = NULL;

    Init(interp);

    if (objc > 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-wait? ?threadId?");
        return TCL_ERROR;
    }
    if (objc > 1) {
        if (OPT_CMP(Tcl_GetString(objv[1]), "-wait")) {
            wait = 1;
            if (objc > 2) {
                if (ThreadGetId(interp, objv[2], &thrId) != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        } else if (ThreadGetId(interp, objv[1], &thrId) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    return ThreadReserve(interp, thrId, THREAD_RELEASE, wait);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadUnwindObjCmd --
 *
 *  This procedure is invoked to process the "thread::unwind" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadUnwindObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    Init(interp);

    if (objc > 1) {
        Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    return ThreadReserve(interp, 0, THREAD_RELEASE, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadExitObjCmd --
 *
 *  This procedure is invoked to process the "thread::exit" Tcl
 *  command.  This causes an unconditional close of the thread
 *  and is GUARANTEED to cause memory leaks.  Use this with caution.
 *
 * Results:
 *  Doesn't actually return.
 *
 * Side effects:
 *  Lots.  improper clean up of resources.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadExitObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int status = 666;

    Init(interp);

    if (objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?status?");
        return TCL_ERROR;
    }

    if (objc == 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &status) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    ListRemove(NULL);

    Tcl_ExitThread(status);

    return TCL_OK; /* NOT REACHED */
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadIdObjCmd --
 *
 *  This procedure is invoked to process the "thread::id" Tcl command.
 *  This returns the ID of the current thread.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadIdObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    char thrHandle[THREAD_HNDLMAXLEN];

    Init(interp);

    if (objc > 1) {
        Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    ThreadGetHandle(Tcl_GetCurrentThread(), thrHandle);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(thrHandle, -1));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadNamesObjCmd --
 *
 *  This procedure is invoked to process the "thread::names" Tcl
 *  command. This returns a list of all known thread IDs.
 *  These are only threads created via this module (e.g., not
 *  driver threads or the notifier).
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadNamesObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int ii, length;
    char *result, thrHandle[THREAD_HNDLMAXLEN];
    Tcl_ThreadId *thrIdArray;
    Tcl_DString threadNames;

    Init(interp);

    if (objc > 1) {
        Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    length = ThreadList(interp, &thrIdArray);

    if (length == 0) {
        return TCL_OK;
    }

    Tcl_DStringInit(&threadNames);

    for (ii = 0; ii < length; ii++) {
        ThreadGetHandle(thrIdArray[ii], thrHandle);
        Tcl_DStringAppendElement(&threadNames, thrHandle);
    }

    length = Tcl_DStringLength(&threadNames);
    result = Tcl_DStringValue(&threadNames);

    Tcl_SetObjResult(interp, Tcl_NewStringObj(result, length));

    Tcl_DStringFree(&threadNames);
    ckfree((char*)thrIdArray);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadSendObjCmd --
 *
 *  This procedure is invoked to process the "thread::send" Tcl
 *  command. This sends a script to another thread for execution.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void
threadSendFree(ClientData ptr)
{
    ckfree((char *)ptr);
}

static int
ThreadSendObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    size_t size;
    int ret, ii = 0, flags = 0;
    Tcl_ThreadId thrId;
    const char *script, *arg;
    Tcl_Obj *var = NULL;

    ThreadClbkData *clbkPtr = NULL;
    ThreadSendData *sendPtr = NULL;

    Init(interp);

    /*
     * Syntax: thread::send ?-async? ?-head? threadId script ?varName?
     */

    if (objc < 3 || objc > 6) {
        goto usage;
    }

    flags = THREAD_SEND_WAIT;

    for (ii = 1; ii < objc; ii++) {
        arg = Tcl_GetString(objv[ii]);
        if (OPT_CMP(arg, "-async")) {
            flags &= ~THREAD_SEND_WAIT;
        } else if (OPT_CMP(arg, "-head")) {
            flags |= THREAD_SEND_HEAD;
        } else {
            break;
        }
    }
    if (ii >= objc) {
        goto usage;
    }
    if (ThreadGetId(interp, objv[ii], &thrId) != TCL_OK) {
        return TCL_ERROR;
    }
    if (++ii >= objc) {
        goto usage;
    }

    script = Tcl_GetString(objv[ii]);
    size = objv[ii]->length+1;
    if (++ii < objc) {
        var = objv[ii];
    }
    if (var && (flags & THREAD_SEND_WAIT) == 0) {
        const char *varName = Tcl_GetString(var);
        size_t vsize = var->length + 1;

        if (thrId == Tcl_GetCurrentThread()) {
            /*
             * FIXME: Do something for callbacks to self
             */
            Tcl_SetObjResult(interp, Tcl_NewStringObj("can't notify self", -1));
            return TCL_ERROR;
        }

        /*
         * Prepare record for the callback. This is asynchronously
         * posted back to us when the target thread finishes processing.
         * We should do a vwait on the "var" to get notified.
         */

        clbkPtr = (ThreadClbkData*)ckalloc(sizeof(ThreadClbkData));
        clbkPtr->execProc   = ThreadClbkSetVar;
        clbkPtr->freeProc   = threadSendFree;
        clbkPtr->interp     = interp;
        clbkPtr->threadId   = Tcl_GetCurrentThread();
        clbkPtr->clientData = memcpy(ckalloc(vsize), varName, vsize);
    }

    /*
     * Prepare job record for the target thread
     */

    sendPtr = (ThreadSendData*)ckalloc(sizeof(ThreadSendData));
    sendPtr->interp     = NULL; /* Signal to use thread main interp */
    sendPtr->execProc   = ThreadSendEval;
    sendPtr->freeProc   = threadSendFree;
    sendPtr->clientData = memcpy(ckalloc(size), script, size);

    ret = ThreadSend(interp, thrId, sendPtr, clbkPtr, flags);

    if (var && (flags & THREAD_SEND_WAIT)) {

        /*
         * Leave job's result in passed variable
         * and return the code, like "catch" does.
         */

        Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
        if (!Tcl_ObjSetVar2(interp, var, NULL, resultObj, TCL_LEAVE_ERR_MSG)) {
            return TCL_ERROR;
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(ret));
        return TCL_OK;
    }

    return ret;

usage:
    Tcl_WrongNumArgs(interp,1,objv,"?-async? ?-head? id script ?varName?");
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadBroadcastObjCmd --
 *
 *  This procedure is invoked to process the "thread::broadcast" Tcl
 *  command. This asynchronously sends a script to all known threads.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  Script is sent to all known threads except the caller thread.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadBroadcastObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int ii, nthreads;
    size_t size;
    const char *script;
    Tcl_ThreadId *thrIdArray;
    ThreadSendData *sendPtr, job;

    Init(interp);

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "script");
        return TCL_ERROR;
    }

    script = Tcl_GetString(objv[1]);
    size = objv[1]->length + 1;

    /*
     * Get the list of known threads. Note that this one may
     * actually change (thread may exit or otherwise cease to
     * exist) while we circle in the loop below. We really do
     * not care about that here since we don't return any
     * script results to the caller.
     */

    nthreads = ThreadList(interp, &thrIdArray);

    if (nthreads == 0) {
        return TCL_OK;
    }

    /*
     * Prepare the structure with the job description
     * to be sent asynchronously to each known thread.
     */

    job.interp     = NULL; /* Signal to use thread's main interp */
    job.execProc   = ThreadSendEval;
    job.freeProc   = threadSendFree;
    job.clientData = NULL;

    /*
     * Now, circle this list and send each thread the script.
     * This is sent asynchronously, since we do not care what
     * are they going to do with it. Also, the event is queued
     * to the head of the event queue (as out-of-band message).
     */

    for (ii = 0; ii < nthreads; ii++) {
        if (thrIdArray[ii] == Tcl_GetCurrentThread()) {
            continue; /* Do not broadcast self */
        }
        sendPtr  = (ThreadSendData*)ckalloc(sizeof(ThreadSendData));
        *sendPtr = job;
        sendPtr->clientData = memcpy(ckalloc(size), script, size);
        ThreadSend(interp, thrIdArray[ii], sendPtr, NULL, THREAD_SEND_HEAD);
    }

    ckfree((char*)thrIdArray);
    Tcl_ResetResult(interp);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadWaitObjCmd --
 *
 *  This procedure is invoked to process the "thread::wait" Tcl
 *  command. This enters the event loop.
 *
 * Results:
 *  Standard Tcl result.
 *
 * Side effects:
 *  Enters the event loop.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadWaitObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    Init(interp);

    if (objc > 1) {
        Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    return ThreadWait(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadErrorProcObjCmd --
 *
 *  This procedure is invoked to process the "thread::errorproc"
 *  command. This registers a procedure to handle thread errors.
 *  Empty string as the name of the procedure will reset the
 *  default behaviour, which is writing to standard error channel.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  Registers an errorproc.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadErrorProcObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    size_t len;
    char *proc;

    Init(interp);

    if (objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?proc?");
        return TCL_ERROR;
    }
    Tcl_MutexLock(&threadMutex);
    if (objc == 1) {
        if (errorProcString) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(errorProcString, -1));
        }
    } else {
        if (errorProcString) {
            ckfree(errorProcString);
        }
        proc = Tcl_GetString(objv[1]);
        len = objv[1]->length;
        if (len == 0) {
            errorThreadId = NULL;
            errorProcString = NULL;
        } else {
            errorThreadId = Tcl_GetCurrentThread();
            errorProcString = (char *)ckalloc(1+strlen(proc));
            strcpy(errorProcString, proc);
            Tcl_DeleteThreadExitHandler(ThreadFreeError, NULL);
            Tcl_CreateThreadExitHandler(ThreadFreeError, NULL);
        }
    }
    Tcl_MutexUnlock(&threadMutex);

    return TCL_OK;
}

static void
ThreadFreeError(
    ClientData clientData
) {
    Tcl_MutexLock(&threadMutex);
    if (errorThreadId != Tcl_GetCurrentThread()) {
        Tcl_MutexUnlock(&threadMutex);
        return;
    }
    ckfree(errorProcString);
    errorThreadId = NULL;
    errorProcString = NULL;
    Tcl_MutexUnlock(&threadMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadJoinObjCmd --
 *
 *  This procedure is invoked to process the "thread::join" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadJoinObjCmd(
    ClientData  dummy,          /* Not used. */
    Tcl_Interp *interp,         /* Current interpreter. */
    int         objc,           /* Number of arguments. */
    Tcl_Obj    *const objv[]    /* Argument objects. */
) {
    Tcl_ThreadId thrId;

    Init(interp);

    /*
     * Syntax of 'join': id
     */

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "id");
        return TCL_ERROR;
    }

    if (ThreadGetId(interp, objv[1], &thrId) != TCL_OK) {
        return TCL_ERROR;
    }

    return ThreadJoin(interp, thrId);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadTransferObjCmd --
 *
 *  This procedure is invoked to process the "thread::transfer" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadTransferObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {

    Tcl_ThreadId thrId;
    Tcl_Channel chan;

    Init(interp);

    /*
     * Syntax of 'transfer': id channel
     */

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "id channel");
        return TCL_ERROR;
    }
    if (ThreadGetId(interp, objv[1], &thrId) != TCL_OK) {
        return TCL_ERROR;
    }

    chan = Tcl_GetChannel(interp, Tcl_GetString(objv[2]), NULL);
    if (chan == NULL) {
        return TCL_ERROR;
    }

    return ThreadTransfer(interp, thrId, Tcl_GetTopChannel(chan));
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadDetachObjCmd --
 *
 *  This procedure is invoked to process the "thread::detach" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadDetachObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    Tcl_Channel chan;

    Init(interp);

    /*
     * Syntax: thread::detach channel
     */

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "channel");
        return TCL_ERROR;
    }

    chan = Tcl_GetChannel(interp, Tcl_GetString(objv[1]), NULL);
    if (chan == NULL) {
        return TCL_ERROR;
    }

    return ThreadDetach(interp, Tcl_GetTopChannel(chan));
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadAttachObjCmd --
 *
 *  This procedure is invoked to process the "thread::attach" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadAttachObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    char *chanName;

    Init(interp);

    /*
     * Syntax: thread::attach channel
     */

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "channel");
        return TCL_ERROR;
    }

    chanName = Tcl_GetString(objv[1]);
    if (Tcl_IsChannelExisting(chanName)) {
        return TCL_OK;
    }

    return ThreadAttach(interp, chanName);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadExistsObjCmd --
 *
 *  This procedure is invoked to process the "thread::exists" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadExistsObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    Tcl_ThreadId thrId;

    Init(interp);

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "id");
        return TCL_ERROR;
    }

    if (ThreadGetId(interp, objv[1], &thrId) != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_SetIntObj(Tcl_GetObjResult(interp), ThreadExists(thrId)!=0);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadConfigureObjCmd --
 *
 *  This procedure is invoked to process the Tcl "thread::configure"
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *----------------------------------------------------------------------
 */
static int
ThreadConfigureObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    char *option, *value;
    Tcl_ThreadId thrId;         /* Id of the thread to configure */
    int i;                      /* Iterate over arg-value pairs. */
    Tcl_DString ds;             /* DString to hold result of
                                 * calling GetThreadOption. */

    if (objc < 2 || (objc % 2 == 1 && objc != 3)) {
        Tcl_WrongNumArgs(interp, 1, objv, "threadlId ?optionName? "
                         "?value? ?optionName value?...");
        return TCL_ERROR;
    }

    Init(interp);

    if (ThreadGetId(interp, objv[1], &thrId) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 2) {
        Tcl_DStringInit(&ds);
        if (ThreadGetOption(interp, thrId, NULL, &ds) != TCL_OK) {
            Tcl_DStringFree(&ds);
            return TCL_ERROR;
        }
        Tcl_DStringResult(interp, &ds);
        return TCL_OK;
    }
    if (objc == 3) {
        Tcl_DStringInit(&ds);
        option = Tcl_GetString(objv[2]);
        if (ThreadGetOption(interp, thrId, option, &ds) != TCL_OK) {
            Tcl_DStringFree(&ds);
            return TCL_ERROR;
        }
        Tcl_DStringResult(interp, &ds);
        return TCL_OK;
    }
    for (i = 3; i < objc; i += 2) {
        option = Tcl_GetString(objv[i-1]);
        value  = Tcl_GetString(objv[i]);
        if (ThreadSetOption(interp, thrId, option, value) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    return TCL_OK;
}

#ifdef TCL_TIP285
/*
 *----------------------------------------------------------------------
 *
 * ThreadCancelObjCmd --
 *
 *  This procedure is invoked to process the "thread::cancel" Tcl
 *  command. See the user documentation for details on what it does.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadCancelObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    Tcl_ThreadId thrId;
    int ii, flags;
    const char *result;

    if ((objc < 2) || (objc > 4)) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-unwind? id ?result?");
        return TCL_ERROR;
    }

    flags = 0;
    ii = 1;
    if ((objc == 3) || (objc == 4)) {
        if (OPT_CMP(Tcl_GetString(objv[ii]), "-unwind")) {
            flags |= TCL_CANCEL_UNWIND;
            ii++;
        }
    }

    if (ThreadGetId(interp, objv[ii], &thrId) != TCL_OK) {
        return TCL_ERROR;
    }

    ii++;
    if (ii < objc) {
        result = Tcl_GetString(objv[ii]);
    } else {
        result = NULL;
    }

    return ThreadCancel(interp, thrId, result, flags);
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * ThreadSendEval --
 *
 *  Evaluates Tcl script passed from source to target thread.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static int
ThreadSendEval(
    Tcl_Interp *interp,
    ClientData clientData
) {
    ThreadSendData *sendPtr = (ThreadSendData*)clientData;
    char *script = (char*)sendPtr->clientData;

    return Tcl_EvalEx(interp, script, -1, TCL_EVAL_GLOBAL);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadClbkSetVar --
 *
 *  Sets the Tcl variable in the source thread, as the result
 *  of the asynchronous callback.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  New Tcl variable may be created
 *
 *----------------------------------------------------------------------
 */

static int
ThreadClbkSetVar(
    Tcl_Interp *interp,
    ClientData clientData
) {
    ThreadClbkData *clbkPtr = (ThreadClbkData*)clientData;
    const char *var = (const char *)clbkPtr->clientData;
    Tcl_Obj *valObj;
    ThreadEventResult *resultPtr = &clbkPtr->result;
    int rc = TCL_OK;

    /*
     * Get the result of the posted command.
     * We will use it to fill-in the result variable.
     */

    valObj = Tcl_NewStringObj(resultPtr->result, -1);
    Tcl_IncrRefCount(valObj);

    if (resultPtr->result != threadEmptyResult) {
        ckfree(resultPtr->result);
    }

    /*
     * Set the result variable
     */

    if (Tcl_SetVar2Ex(interp, var, NULL, valObj,
                      TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
        rc = TCL_ERROR;
        goto cleanup;
    }

    /*
     * In case of error, trigger the bgerror mechansim
     */

    if (resultPtr->code == TCL_ERROR) {
        if (resultPtr->errorCode) {
            var = "errorCode";
            Tcl_SetVar2Ex(interp, var, NULL, Tcl_NewStringObj(resultPtr->errorCode, -1), TCL_GLOBAL_ONLY);
            ckfree((char*)resultPtr->errorCode);
        }
        if (resultPtr->errorInfo) {
            var = "errorInfo";
            Tcl_SetVar2Ex(interp, var, NULL, Tcl_NewStringObj(resultPtr->errorInfo, -1), TCL_GLOBAL_ONLY);
            ckfree((char*)resultPtr->errorInfo);
        }
        Tcl_SetObjResult(interp, valObj);
        Tcl_BackgroundException(interp, TCL_ERROR);
    }

cleanup:
    Tcl_DecrRefCount(valObj);
    return rc;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadCreate --
 *
 *  This procedure is invoked to create a thread containing an
 *  interp to run a script. This returns after the thread has
 *  started executing.
 *
 * Results:
 *  A standard Tcl result, which is the thread ID.
 *
 * Side effects:
 *  Create a thread.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadCreate(
    Tcl_Interp *interp,        /* Current interpreter. */
    const char *script,        /* Script to evaluate */
    int         stacksize,     /* Zero for default size */
    int         flags,         /* Zero for no flags */
    int         preserve       /* If true, reserve the thread */
) {
    char thrHandle[THREAD_HNDLMAXLEN];
    ThreadCtrl ctrl;
    Tcl_ThreadId thrId;

    ctrl.cd = Tcl_GetAssocData(interp, "thread:nsd", NULL);
    ctrl.script   = (char *)script;
    ctrl.condWait = NULL;
    ctrl.flags    = 0;

    Tcl_MutexLock(&threadMutex);
    if (Tcl_CreateThread(&thrId, NewThread, &ctrl,
            stacksize, flags) != TCL_OK) {
        Tcl_MutexUnlock(&threadMutex);
        Tcl_SetObjResult(interp, Tcl_NewStringObj("can't create a new thread", -1));
        return TCL_ERROR;
    }

    /*
     * Wait for the thread to start because it is using
     * the ThreadCtrl argument which is on our stack.
     */

    while (ctrl.script != NULL) {
        Tcl_ConditionWait(&ctrl.condWait, &threadMutex, NULL);
    }
    if (preserve) {
        ThreadSpecificData *tsdPtr = ThreadExistsInner(thrId);
        if (tsdPtr == NULL) {
            Tcl_MutexUnlock(&threadMutex);
            Tcl_ConditionFinalize(&ctrl.condWait);
            ErrorNoSuchThread(interp, thrId);
            return TCL_ERROR;
        }
        tsdPtr->refCount++;
    }

    Tcl_MutexUnlock(&threadMutex);
    Tcl_ConditionFinalize(&ctrl.condWait);

    ThreadGetHandle(thrId, thrHandle);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(thrHandle, -1));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NewThread --
 *
 *    This routine is the "main()" for a new thread whose task is to
 *    execute a single TCL script. The argument to this function is
 *    a pointer to a structure that contains the text of the Tcl script
 *    to be executed, plus some synchronization primitives. Those are
 *    used so the caller gets signalized when the new thread has
 *    done its initialization.
 *
 *    Space to hold the ThreadControl structure itself is reserved on
 *    the stack of the calling function. The two condition variables
 *    in the ThreadControl structure are destroyed by the calling
 *    function as well. The calling function will destroy the
 *    ThreadControl structure and the condition variable as soon as
 *    ctrlPtr->condWait is signaled, so this routine must make copies
 *    of any data it might need after that point.
 *
 * Results:
 *    none
 *
 * Side effects:
 *    A Tcl script is executed in a new thread.
 *
 *----------------------------------------------------------------------
 */

Tcl_ThreadCreateType
NewThread(
    ClientData clientData
) {
    ThreadCtrl *ctrlPtr = (ThreadCtrl *)clientData;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Tcl_Interp *interp;
    int result = TCL_OK;
    size_t scriptLen;
    char *evalScript;

    /*
     * Initialize the interpreter. The bad thing here is that we
     * assume that initialization of the Tcl interp will be
     * error free, which it may not. In the future we must recover
     * from this and exit gracefully (this is not that easy as
     * it seems on the first glance...)
     */

#ifdef NS_AOLSERVER
    NsThreadInterpData *md = (NsThreadInterpData *)ctrlPtr->cd;
    Ns_ThreadSetName("-tclthread-");
    interp = (Tcl_Interp*)Ns_TclAllocateInterp(md ? md->server : NULL);
#else
    interp = Tcl_CreateInterp();
    result = Tcl_Init(interp);
#endif

#if !defined(NS_AOLSERVER) || (defined(NS_MAJOR_VERSION) && NS_MAJOR_VERSION >= 4)
    result = Thread_Init(interp);
#endif

    tsdPtr->interp = interp;

    Tcl_MutexLock(&threadMutex);

    /*
     * Update the list of threads.
     */

    ListUpdateInner(tsdPtr);

    /*
     * We need to keep a pointer to the alloc'ed mem of the script
     * we are eval'ing, for the case that we exit during evaluation
     */

    scriptLen = strlen(ctrlPtr->script);
    evalScript = strcpy((char*)ckalloc(scriptLen+1), ctrlPtr->script);
    Tcl_CreateThreadExitHandler(ThreadExitProc,evalScript);

    /*
     * Notify the parent we are alive.
     */

    ctrlPtr->script = NULL;
    Tcl_ConditionNotify(&ctrlPtr->condWait);

    Tcl_MutexUnlock(&threadMutex);

    /*
     * Run the script.
     */

    Tcl_Preserve(tsdPtr->interp);
    result = Tcl_EvalEx(tsdPtr->interp, evalScript,scriptLen,TCL_EVAL_GLOBAL);
    if (result != TCL_OK) {
        ThreadErrorProc(tsdPtr->interp);
    }

    /*
     * Clean up. Note: add something like TlistRemove for the transfer list.
     */

    if (tsdPtr->doOneEvent) {
        Tcl_ConditionFinalize(&tsdPtr->doOneEvent);
    }

    ListRemove(tsdPtr);

    /*
     * It is up to all other extensions, including Tk, to be responsible
     * for their own events when they receive their Tcl_CallWhenDeleted
     * notice when we delete this interp.
     */

#ifdef NS_AOLSERVER
    Ns_TclMarkForDelete(tsdPtr->interp);
    Ns_TclDeAllocateInterp(tsdPtr->interp);
#else
    Tcl_DeleteInterp(tsdPtr->interp);
#endif
    Tcl_Release(tsdPtr->interp);

    /*tsdPtr->interp = NULL;*/

    /*
     * Tcl_ExitThread calls Tcl_FinalizeThread() indirectly which calls
     * ThreadExitHandlers and cleans the notifier as well as other sub-
     * systems that save thread state data.
     */

    Tcl_ExitThread(result);

    TCL_THREAD_CREATE_RETURN;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadErrorProc --
 *
 *  Send a message to the thread willing to hear about errors.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  Send an event.
 *
 *----------------------------------------------------------------------
 */

static void
ThreadErrorProc(
    Tcl_Interp *interp         /* Interp that failed */
) {
    ThreadSendData *sendPtr;
    const char *argv[3];
    char buf[THREAD_HNDLMAXLEN];
    const char *errorInfo;

    errorInfo = Tcl_GetVar2(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY);
    if (errorInfo == NULL) {
        errorInfo = "";
    }

    if (errorProcString == NULL) {
#ifdef NS_AOLSERVER
        Ns_Log(Error, "%s\n%s", Tcl_GetString(Tcl_GetObjResult(interp)), errorInfo);
#else
        Tcl_Channel errChannel = Tcl_GetStdChannel(TCL_STDERR);
        if (errChannel == NULL) {
            /* Fixes the [#634845] bug; credits to
             * Wojciech Kocjan <wojciech@kocjan.org> */
            return;
        }
        ThreadGetHandle(Tcl_GetCurrentThread(), buf);
        Tcl_WriteChars(errChannel, "Error from thread ", -1);
        Tcl_WriteChars(errChannel, buf, -1);
        Tcl_WriteChars(errChannel, "\n", 1);
        Tcl_WriteChars(errChannel, errorInfo, -1);
        Tcl_WriteChars(errChannel, "\n", 1);
#endif
    } else {
        ThreadGetHandle(Tcl_GetCurrentThread(), buf);
        argv[0] = errorProcString;
        argv[1] = buf;
        argv[2] = errorInfo;

        sendPtr = (ThreadSendData*)ckalloc(sizeof(ThreadSendData));
        sendPtr->execProc   = ThreadSendEval;
        sendPtr->freeProc   = threadSendFree;
        sendPtr->clientData = Tcl_Merge(3, argv);
        sendPtr->interp     = NULL;

        ThreadSend(interp, errorThreadId, sendPtr, NULL, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ListUpdate --
 *
 *  Add the thread local storage to the list. This grabs the
 *  mutex to protect the list.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void
ListUpdate(
    ThreadSpecificData *tsdPtr
) {
    if (tsdPtr == NULL) {
        tsdPtr = TCL_TSD_INIT(&dataKey);
    }

    Tcl_MutexLock(&threadMutex);
    ListUpdateInner(tsdPtr);
    Tcl_MutexUnlock(&threadMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * ListUpdateInner --
 *
 *  Add the thread local storage to the list. This assumes the caller
 *  has obtained the threadMutex.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  Add the thread local storage to its list.
 *
 *----------------------------------------------------------------------
 */

static void
ListUpdateInner(
    ThreadSpecificData *tsdPtr
) {
    if (threadList) {
        threadList->prevPtr = tsdPtr;
    }

    tsdPtr->nextPtr  = threadList;
    tsdPtr->prevPtr  = NULL;
    tsdPtr->threadId = Tcl_GetCurrentThread();

    threadList = tsdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ListRemove --
 *
 *  Remove the thread local storage from its list. This grabs the
 *  mutex to protect the list.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  Remove the thread local storage from its list.
 *
 *----------------------------------------------------------------------
 */

static void
ListRemove(
    ThreadSpecificData *tsdPtr
) {
    if (tsdPtr == NULL) {
        tsdPtr = TCL_TSD_INIT(&dataKey);
    }

    Tcl_MutexLock(&threadMutex);
    ListRemoveInner(tsdPtr);
    Tcl_MutexUnlock(&threadMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * ListRemoveInner --
 *
 *  Remove the thread local storage from its list.
 *
 * Results:
 *  None
 *
 * Side effects:
 *  Remove the thread local storage from its list.
 *
 *----------------------------------------------------------------------
 */

static void
ListRemoveInner(
    ThreadSpecificData *tsdPtr
) {
    if (tsdPtr->prevPtr || tsdPtr->nextPtr) {
        if (tsdPtr->prevPtr) {
            tsdPtr->prevPtr->nextPtr = tsdPtr->nextPtr;
        } else {
            threadList = tsdPtr->nextPtr;
        }
        if (tsdPtr->nextPtr) {
            tsdPtr->nextPtr->prevPtr = tsdPtr->prevPtr;
        }
        tsdPtr->nextPtr = NULL;
        tsdPtr->prevPtr = NULL;
    } else if (tsdPtr == threadList) {
        threadList = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadList --
 *
 *  Return a list of threads running Tcl interpreters.
 *
 * Results:
 *  Number of threads.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadList(
    Tcl_Interp *interp,
    Tcl_ThreadId **thrIdArray
) {
    int ii, count = 0;
    ThreadSpecificData *tsdPtr;

    Tcl_MutexLock(&threadMutex);

    /*
     * First walk; find out how many threads are registered.
     * We may avoid this and gain some speed by maintaining
     * the counter of allocated structs in the threadList.
     */

    for (tsdPtr = threadList; tsdPtr; tsdPtr = tsdPtr->nextPtr) {
        count++;
    }

    if (count == 0) {
        Tcl_MutexUnlock(&threadMutex);
        return 0;
    }

    /*
     * Allocate storage for passing thread id's to caller
     */

    *thrIdArray = (Tcl_ThreadId*)ckalloc(count * sizeof(Tcl_ThreadId));

    /*
     * Second walk; fill-in the array with thread ID's
     */

    for (tsdPtr = threadList, ii = 0; tsdPtr; tsdPtr = tsdPtr->nextPtr, ii++) {
        (*thrIdArray)[ii] = tsdPtr->threadId;
    }

    Tcl_MutexUnlock(&threadMutex);

    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadExists --
 *
 *  Test whether a thread given by it's id is known to us.
 *
 * Results:
 *  Pointer to thread specific data structure or
 *  NULL if no thread with given ID found
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadExists(
     Tcl_ThreadId thrId
) {
    ThreadSpecificData *tsdPtr;

    Tcl_MutexLock(&threadMutex);
    tsdPtr = ThreadExistsInner(thrId);
    Tcl_MutexUnlock(&threadMutex);

    return tsdPtr != NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadExistsInner --
 *
 *  Test whether a thread given by it's id is known to us. Assumes
 *  caller holds the thread mutex.
 *
 * Results:
 *  Pointer to thread specific data structure or
 *  NULL if no thread with given ID found
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static ThreadSpecificData *
ThreadExistsInner(
    Tcl_ThreadId thrId              /* Thread id to look for. */
) {
    ThreadSpecificData *tsdPtr;

    for (tsdPtr = threadList; tsdPtr; tsdPtr = tsdPtr->nextPtr) {
        if (tsdPtr->threadId == thrId) {
            return tsdPtr;
        }
    }

    return NULL;
}

#ifdef TCL_TIP285
/*
 *----------------------------------------------------------------------
 *
 * ThreadCancel --
 *
 *    Cancels a script in another thread.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadCancel(
    Tcl_Interp  *interp,       /* The current interpreter. */
    Tcl_ThreadId thrId,        /* Thread ID of other interpreter. */
    const char *result,        /* The error message or NULL for default. */
    int flags                  /* Flags for Tcl_CancelEval. */
) {
    int code;
    Tcl_Obj *resultObj = NULL;
    ThreadSpecificData *tsdPtr; /* ... of the target thread */

    Tcl_MutexLock(&threadMutex);

    tsdPtr = ThreadExistsInner(thrId);
    if (tsdPtr == NULL) {
        Tcl_MutexUnlock(&threadMutex);
        ErrorNoSuchThread(interp, thrId);
        return TCL_ERROR;
    }

    if (!haveInterpCancel) {
        Tcl_MutexUnlock(&threadMutex);
        Tcl_AppendResult(interp, "not supported with this Tcl version", NULL);
        return TCL_ERROR;
    }

    if (result != NULL) {
        resultObj = Tcl_NewStringObj(result, -1);
    }

    code = Tcl_CancelEval(tsdPtr->interp, resultObj, NULL, flags);

    Tcl_MutexUnlock(&threadMutex);
    return code;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * ThreadJoin --
 *
 *  Wait for the exit of a different thread.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  The status of the exiting thread is left in the interp result
 *  area, but only in the case of success.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadJoin(
    Tcl_Interp  *interp,       /* The current interpreter. */
    Tcl_ThreadId thrId         /* Thread ID of other interpreter. */
) {
    int ret, state;

    ret = Tcl_JoinThread(thrId, &state);

    if (ret == TCL_OK) {
        Tcl_SetIntObj(Tcl_GetObjResult (interp), state);
    } else {
        char thrHandle[THREAD_HNDLMAXLEN];
        ThreadGetHandle(thrId, thrHandle);
        Tcl_AppendResult(interp, "cannot join thread ", thrHandle, NULL);
    }

    return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadTransfer --
 *
 *  Transfers the specified channel which must not be shared and has
 *  to be registered in the given interp from that location to the
 *  main interp of the specified thread.
 *
 *  Thanks to Anreas Kupries for the initial implementation.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  The thread-global lists of all known channels of both threads
 *  involved (specified and current) are modified. The channel is
 *  moved, all event handling for the channel is killed.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadTransfer(
    Tcl_Interp *interp,        /* The current interpreter. */
    Tcl_ThreadId thrId,        /* Thread Id of other interpreter. */
    Tcl_Channel  chan          /* The channel to transfer */
) {
    /* Steps to perform for the transfer:
     *
     * i.   Sanity checks: chan has to registered in interp, must not be
     *      shared. This automatically excludes the special channels for
     *      stdin, stdout and stderr!
     * ii.  Clear event handling.
     * iii. Bump reference counter up to prevent destruction during the
     *      following unregister, then unregister the channel from the
     *      interp. Remove it from the thread-global list of all channels
     *      too.
     * iv.  Wrap the channel into an event and send that to the other
     *      thread, then wait for the other thread to process our message.
     * v.   The event procedure called by the other thread is
     *      'TransferEventProc'. It links the channel into the
     *      thread-global list of channels for that thread, registers it
     *      in the main interp of the other thread, removes the artificial
     *      reference, at last notifies this thread of the sucessful
     *      transfer. This allows this thread then to proceed.
     */

    TransferEvent *evPtr;
    TransferResult *resultPtr;

    if (!Tcl_IsChannelRegistered(interp, chan)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("channel is not registered here", -1));
    }
    if (Tcl_IsChannelShared(chan)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("channel is shared", -1));
        return TCL_ERROR;
    }

    /*
     * Short circuit transfers to ourself.  Nothing to do.
     */

    if (thrId == Tcl_GetCurrentThread()) {
        return TCL_OK;
    }

    Tcl_MutexLock(&threadMutex);

    /*
     * Verify the thread exists.
     */

    if (ThreadExistsInner(thrId) == NULL) {
        Tcl_MutexUnlock(&threadMutex);
        ErrorNoSuchThread(interp, thrId);
        return TCL_ERROR;
    }

    /*
     * Cut the channel out of the interp/thread
     */

    ThreadCutChannel(interp, chan);

    /*
     * Wrap it into an event.
     */

    resultPtr = (TransferResult*)ckalloc(sizeof(TransferResult));
    evPtr     = (TransferEvent *)ckalloc(sizeof(TransferEvent));

    evPtr->chan       = chan;
    evPtr->event.proc = TransferEventProc;
    evPtr->resultPtr  = resultPtr;

    /*
     * Initialize the result fields.
     */

    resultPtr->done       = (Tcl_Condition) NULL;
    resultPtr->resultCode = -1;
    resultPtr->resultMsg  = (char *) NULL;

    /*
     * Maintain the cleanup list.
     */

    resultPtr->srcThreadId = Tcl_GetCurrentThread();
    resultPtr->dstThreadId = thrId;
    resultPtr->eventPtr    = evPtr;

    SpliceIn(resultPtr, transferList);

    /*
     * Queue the event and poke the other thread's notifier.
     */

    Tcl_ThreadQueueEvent(thrId, (Tcl_Event*)evPtr, TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(thrId);

    /*
     * (*) Block until the other thread has either processed the transfer
     * or rejected it.
     */

    while (resultPtr->resultCode < 0) {
        Tcl_ConditionWait(&resultPtr->done, &threadMutex, NULL);
    }

    /*
     * Unlink result from the result list.
     */

    SpliceOut(resultPtr, transferList);

    resultPtr->eventPtr = NULL;
    resultPtr->nextPtr  = NULL;
    resultPtr->prevPtr  = NULL;

    Tcl_MutexUnlock(&threadMutex);

    Tcl_ConditionFinalize(&resultPtr->done);

    /*
     * Process the result now.
     */

    if (resultPtr->resultCode != TCL_OK) {

        /*
         * Transfer failed, restore old state of channel with respect
         * to current thread and specified interp.
         */

        Tcl_SpliceChannel(chan);
        Tcl_RegisterChannel(interp, chan);
        Tcl_UnregisterChannel((Tcl_Interp *) NULL, chan);
        Tcl_AppendResult(interp, "transfer failed: ", NULL);

        if (resultPtr->resultMsg) {
            Tcl_AppendResult(interp, resultPtr->resultMsg, NULL);
            ckfree(resultPtr->resultMsg);
        } else {
            Tcl_AppendResult(interp, "for reasons unknown", NULL);
        }
        ckfree((char *)resultPtr);

        return TCL_ERROR;
    }

    if (resultPtr->resultMsg) {
        ckfree(resultPtr->resultMsg);
    }
    ckfree((char *)resultPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadDetach --
 *
 *  Detaches the specified channel which must not be shared and has
 *  to be registered in the given interp. The detached channel is
 *  left in the transfer list until some other thread attaches it
 +  by calling the "thread::attach" command.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  The thread-global lists of all known channels (transferList)
 *  is modified. All event handling for the channel is killed.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadDetach(
    Tcl_Interp *interp,        /* The current interpreter. */
    Tcl_Channel chan           /* The channel to detach */
) {
    TransferEvent *evPtr;
    TransferResult *resultPtr;

    if (!Tcl_IsChannelRegistered(interp, chan)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("channel is not registered here", -1));
    }
    if (Tcl_IsChannelShared(chan)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("channel is shared", -1));
        return TCL_ERROR;
    }

    /*
     * Cut the channel out of the interp/thread
     */

    ThreadCutChannel(interp, chan);

    /*
     * Wrap it into the list of transfered channels. We generate no
     * events associated with the detached channel, thus really not
     * needing the transfer event structure allocated here. This
     * is done purely to avoid having yet another wrapper.
     */

    resultPtr = (TransferResult*)ckalloc(sizeof(TransferResult));
    evPtr     = (TransferEvent*)ckalloc(sizeof(TransferEvent));

    evPtr->chan       = chan;
    evPtr->event.proc = NULL;
    evPtr->resultPtr  = resultPtr;

    /*
     * Initialize the result fields. This is not used.
     */

    resultPtr->done       = NULL;
    resultPtr->resultCode = -1;
    resultPtr->resultMsg  = NULL;

    /*
     * Maintain the cleanup list. By setting the dst/srcThreadId
     * to zero we signal the code in ThreadAttach that this is the
     * detached channel. Therefore it should not be mistaken for
     * some regular TransferChannel operation underway. Also, this
     * will prevent the code in ThreadExitProc to splice out this
     * record from the list when the threads are exiting.
     * A side effect of this is that we may have entries in this
     * list which may never be removed (i.e. nobody attaches the
     * channel later on). This will result in both Tcl channel and
     * memory leak.
     */

    resultPtr->srcThreadId = NULL;
    resultPtr->dstThreadId = NULL;
    resultPtr->eventPtr    = evPtr;

    Tcl_MutexLock(&threadMutex);
    SpliceIn(resultPtr, transferList);
    Tcl_MutexUnlock(&threadMutex);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadAttach --
 *
 *  Attaches the previously detached channel into the current
 *  interpreter.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  The thread-global lists of all known channels (transferList)
 *  is modified.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadAttach(
    Tcl_Interp *interp,        /* The current interpreter. */
    char *chanName             /* The name of the channel to detach */
) {
    int found = 0;
    Tcl_Channel chan = NULL;
    TransferResult *resPtr;

    /*
     * Locate the channel to attach by looking up its name in
     * the list of transfered channels. Watch that we don't
     * hit the regular channel transfer event.
     */

    Tcl_MutexLock(&threadMutex);
    for (resPtr = transferList; resPtr; resPtr = resPtr->nextPtr) {
        chan = resPtr->eventPtr->chan;
        if (!strcmp(Tcl_GetChannelName(chan),chanName)
                && !resPtr->dstThreadId) {
            if (Tcl_IsChannelExisting(chanName)) {
                Tcl_MutexUnlock(&threadMutex);
                Tcl_AppendResult(interp, "channel already exists", NULL);
                return TCL_ERROR;
            }
            SpliceOut(resPtr, transferList);
            ckfree((char*)resPtr->eventPtr);
            ckfree((char*)resPtr);
            found = 1;
            break;
        }
    }
    Tcl_MutexUnlock(&threadMutex);

    if (found == 0) {
        Tcl_AppendResult(interp, "channel not detached", NULL);
        return TCL_ERROR;
    }

    /*
     * Splice channel into the current interpreter
     */

    Tcl_SpliceChannel(chan);
    Tcl_RegisterChannel(interp, chan);
    Tcl_UnregisterChannel(NULL, chan);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadSend --
 *
 *  Run the procedure in other thread.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadSend(
    Tcl_Interp     *interp,     /* The current interpreter. */
    Tcl_ThreadId    thrId,      /* Thread Id of other thread. */
    ThreadSendData *send,       /* Pointer to structure with work to do */
    ThreadClbkData *clbk,       /* Opt. callback structure (may be NULL) */
    int             flags       /* Wait or queue to tail */
) {
    ThreadSpecificData *tsdPtr = NULL; /* ... of the target thread */

    int code;
    ThreadEvent *eventPtr;
    ThreadEventResult *resultPtr;

    /*
     * Verify the thread exists and is not in the error state.
     * The thread is in the error state only if we've configured
     * it to unwind on script evaluation error and last script
     * evaluation resulted in error actually.
     */

    Tcl_MutexLock(&threadMutex);

    tsdPtr = ThreadExistsInner(thrId);

    if (tsdPtr == NULL
            || (tsdPtr->flags & THREAD_FLAGS_INERROR)) {
        int inerror = tsdPtr && (tsdPtr->flags & THREAD_FLAGS_INERROR);
        Tcl_MutexUnlock(&threadMutex);
        ThreadFreeProc(send);
        if (clbk) {
            ThreadFreeProc(clbk);
        }
        if (inerror) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("thread is in error", -1));
        } else {
            ErrorNoSuchThread(interp, thrId);
        }
        return TCL_ERROR;
    }

    /*
     * Short circuit sends to ourself.
     */

    if (thrId == Tcl_GetCurrentThread()) {
        Tcl_MutexUnlock(&threadMutex);
        if ((flags & THREAD_SEND_WAIT)) {
            int code = (*send->execProc)(interp, send);
            ThreadFreeProc(send);
            return code;
        } else {
            send->interp = interp;
            Tcl_Preserve(send->interp);
            Tcl_DoWhenIdle((Tcl_IdleProc*)ThreadIdleProc, send);
            return TCL_OK;
        }
    }

    /*
     * Create the event for target thread event queue.
     */

    eventPtr = (ThreadEvent*)ckalloc(sizeof(ThreadEvent));
    eventPtr->sendData = send;
    eventPtr->clbkData = clbk;

    /*
     * Target thread about to service
     * another event
     */

    if (tsdPtr->maxEventsCount) {
        tsdPtr->eventsPending++;
    }

    /*
     * Caller wants to be notified, so we must take care
     * it's interpreter stays alive until we've finished.
     */

    if (eventPtr->clbkData) {
        Tcl_Preserve(eventPtr->clbkData->interp);
    }
    if ((flags & THREAD_SEND_WAIT) == 0) {
        resultPtr              = NULL;
        eventPtr->resultPtr    = NULL;
    } else {
        resultPtr = (ThreadEventResult*)ckalloc(sizeof(ThreadEventResult));
        resultPtr->done        = NULL;
        resultPtr->result      = NULL;
        resultPtr->errorCode   = NULL;
        resultPtr->errorInfo   = NULL;
        resultPtr->dstThreadId = thrId;
        resultPtr->srcThreadId = Tcl_GetCurrentThread();
        resultPtr->eventPtr    = eventPtr;

        eventPtr->resultPtr    = resultPtr;

        SpliceIn(resultPtr, resultList);
    }

    /*
     * Queue the event and poke the other thread's notifier.
     */

    eventPtr->event.proc = ThreadEventProc;
    if ((flags & THREAD_SEND_HEAD)) {
        Tcl_ThreadQueueEvent(thrId, (Tcl_Event*)eventPtr, TCL_QUEUE_HEAD);
    } else {
        Tcl_ThreadQueueEvent(thrId, (Tcl_Event*)eventPtr, TCL_QUEUE_TAIL);
    }
    Tcl_ThreadAlert(thrId);

    if ((flags & THREAD_SEND_WAIT) == 0) {
        /*
         * Might potentially spend some time here, until the
         * worker thread cleans up its queue a little bit.
         */
        if ((flags & THREAD_SEND_CLBK) == 0) {
            while (tsdPtr->maxEventsCount &&
                   tsdPtr->eventsPending > tsdPtr->maxEventsCount) {
                Tcl_ConditionWait(&tsdPtr->doOneEvent, &threadMutex, NULL);
            }
        }
        Tcl_MutexUnlock(&threadMutex);
        return TCL_OK;
    }

    /*
     * Block on the result indefinitely.
     */

    Tcl_ResetResult(interp);

    while (resultPtr->result == NULL) {
        Tcl_ConditionWait(&resultPtr->done, &threadMutex, NULL);
    }

    SpliceOut(resultPtr, resultList);

    Tcl_MutexUnlock(&threadMutex);

    /*
     * Return result to caller
     */

    if (resultPtr->code == TCL_ERROR) {
        if (resultPtr->errorCode) {
            Tcl_SetErrorCode(interp, resultPtr->errorCode, NULL);
            ckfree(resultPtr->errorCode);
        }
        if (resultPtr->errorInfo) {
            Tcl_AddErrorInfo(interp, resultPtr->errorInfo);
            ckfree(resultPtr->errorInfo);
        }
    }

    code = resultPtr->code;
    Tcl_SetObjResult(interp, Tcl_NewStringObj(resultPtr->result, -1));

    /*
     * Cleanup
     */

    Tcl_ConditionFinalize(&resultPtr->done);
    if (resultPtr->result != threadEmptyResult) {
        ckfree(resultPtr->result);
    }
    ckfree((char*)resultPtr);

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadWait --
 *
 *  Waits for events and process them as they come, until signaled
 *  to stop.
 *
 * Results:
 *  Standard Tcl result.
 *
 * Side effects:
 *  Deletes any thread::send or thread::transfer events that are
 *  pending.
 *
 *----------------------------------------------------------------------
 */
static int
ThreadWait(Tcl_Interp *interp)
{
    int code = TCL_OK;
    int canrun = 1;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Process events until signaled to stop.
     */

    while (canrun) {

        /*
         * About to service another event.
         * Wake-up eventual sleepers.
         */

        if (tsdPtr->maxEventsCount) {
            Tcl_MutexLock(&threadMutex);
            tsdPtr->eventsPending--;
            Tcl_ConditionNotify(&tsdPtr->doOneEvent);
            Tcl_MutexUnlock(&threadMutex);
        }

        /*
         * Attempt to process one event, blocking forever until an
         * event is actually received.  The event processed may cause
         * a script in progress to be canceled or exceed its limit;
         * therefore, check for these conditions if we are able to
         * (i.e. we are running in a high enough version of Tcl).
         */

        Tcl_DoOneEvent(TCL_ALL_EVENTS);

#ifdef TCL_TIP285
        if (haveInterpCancel) {

            /*
             * If the script has been unwound, bail out immediately. This does
             * not follow the recommended guidelines for how extensions should
             * handle the script cancellation functionality because this is
             * not a "normal" extension. Most extensions do not have a command
             * that simply enters an infinite Tcl event loop. Normal extensions
             * should not specify the TCL_CANCEL_UNWIND when calling the
             * Tcl_Canceled function to check if the command has been canceled.
             */

            if (Tcl_Canceled(tsdPtr->interp,
                    TCL_LEAVE_ERR_MSG | TCL_CANCEL_UNWIND) == TCL_ERROR) {
                code = TCL_ERROR;
                break;
            }
        }
#endif
#ifdef TCL_TIP143
        if (haveInterpLimit) {
            if (Tcl_LimitExceeded(tsdPtr->interp)) {
                code = TCL_ERROR;
                break;
            }
        }
#endif

        /*
         * Test stop condition under mutex since
         * some other thread may flip our flags.
         */

        Tcl_MutexLock(&threadMutex);
        canrun = (tsdPtr->flags & THREAD_FLAGS_STOPPED) == 0;
        Tcl_MutexUnlock(&threadMutex);
    }

#if defined(TCL_TIP143) || defined(TCL_TIP285)
    /*
     * If the event processing loop above was terminated due to a
     * script in progress being canceled or exceeding its limits,
     * transfer the error to the current interpreter.
     */

    if (code != TCL_OK) {
        char buf[THREAD_HNDLMAXLEN];
        const char *errorInfo;

        errorInfo = Tcl_GetVar2(tsdPtr->interp, "errorInfo", NULL, TCL_GLOBAL_ONLY);
        if (errorInfo == NULL) {
            errorInfo = Tcl_GetString(Tcl_GetObjResult(tsdPtr->interp));
        }

        ThreadGetHandle(Tcl_GetCurrentThread(), buf);
        Tcl_AppendResult(interp, "Error from thread ", buf, "\n",
                errorInfo, NULL);
    }
#endif

    /*
     * Remove from the list of active threads, so nobody can post
     * work to this thread, since it is just about to terminate.
     */

    ListRemove(tsdPtr);

    /*
     * Now that the event processor for this thread is closing,
     * delete all pending thread::send and thread::transfer events.
     * These events are owned by us.  We don't delete anyone else's
     * events, but ours.
     */

    Tcl_DeleteEvents((Tcl_EventDeleteProc*)ThreadDeleteEvent, NULL);

    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadReserve --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static int
ThreadReserve(
    Tcl_Interp *interp,                /* Current interpreter */
    Tcl_ThreadId thrId,                /* Target thread ID */
    int operation,                     /* THREAD_RESERVE | THREAD_RELEASE */
    int wait                           /* Wait for thread to exit */
) {
    int users, dowait = 0;
    ThreadEvent *evPtr;
    ThreadSpecificData *tsdPtr;

    Tcl_MutexLock(&threadMutex);

    /*
     * Check the given thread
     */

    if (thrId == NULL) {
        tsdPtr = TCL_TSD_INIT(&dataKey);
    } else {
        tsdPtr = ThreadExistsInner(thrId);
        if (tsdPtr == NULL) {
            Tcl_MutexUnlock(&threadMutex);
            ErrorNoSuchThread(interp, thrId);
            return TCL_ERROR;
        }
    }

    switch (operation) {
    case THREAD_RESERVE: ++tsdPtr->refCount;                break;
    case THREAD_RELEASE: --tsdPtr->refCount; dowait = wait; break;
    }

    users = tsdPtr->refCount;

    if (users <= 0) {

        /*
         * We're last attached user, so tear down the *target* thread
         */

        tsdPtr->flags |= THREAD_FLAGS_STOPPED;

        if (thrId && thrId != Tcl_GetCurrentThread() /* Not current! */) {
            ThreadEventResult *resultPtr = NULL;

            /*
             * Remove from the list of active threads, so nobody can post
             * work to this thread, since it is just about to terminate.
             */

            ListRemoveInner(tsdPtr);

            /*
             * Send an dummy event, just to wake-up target thread.
             * It should immediately exit thereafter. We might get
             * stuck here for long time if user really wants to
             * be absolutely sure that the thread has exited.
             */

            if (dowait) {
                resultPtr = (ThreadEventResult*)
                    ckalloc(sizeof(ThreadEventResult));
                resultPtr->done        = NULL;
                resultPtr->result      = NULL;
                resultPtr->code        = TCL_OK;
                resultPtr->errorCode   = NULL;
                resultPtr->errorInfo   = NULL;
                resultPtr->dstThreadId = thrId;
                resultPtr->srcThreadId = Tcl_GetCurrentThread();
                SpliceIn(resultPtr, resultList);
            }

            evPtr = (ThreadEvent*)ckalloc(sizeof(ThreadEvent));
            evPtr->event.proc = ThreadEventProc;
            evPtr->sendData   = NULL;
            evPtr->clbkData   = NULL;
            evPtr->resultPtr  = resultPtr;

            Tcl_ThreadQueueEvent(thrId, (Tcl_Event*)evPtr, TCL_QUEUE_TAIL);
            Tcl_ThreadAlert(thrId);

            if (dowait) {
                while (resultPtr->result == NULL) {
                    Tcl_ConditionWait(&resultPtr->done, &threadMutex, NULL);
                }
                SpliceOut(resultPtr, resultList);
                Tcl_ConditionFinalize(&resultPtr->done);
                if (resultPtr->result != threadEmptyResult) {
                    ckfree(resultPtr->result); /* Will be ignored anyway */
                }
                ckfree((char*)resultPtr);
            }
        }
    }

    Tcl_MutexUnlock(&threadMutex);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), (users > 0) ? users : 0);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadEventProc --
 *
 *  Handle the event in the target thread.
 *
 * Results:
 *  Returns 1 to indicate that the event was processed.
 *
 * Side effects:
 *  Fills out the ThreadEventResult struct.
 *
 *----------------------------------------------------------------------
 */
static int
ThreadEventProc(
    Tcl_Event *evPtr,          /* Really ThreadEvent */
    int mask
) {
    ThreadSpecificData* tsdPtr = TCL_TSD_INIT(&dataKey);

    Tcl_Interp           *interp = NULL;
    Tcl_ThreadId           thrId = Tcl_GetCurrentThread();
    ThreadEvent        *eventPtr = (ThreadEvent*)evPtr;
    ThreadSendData      *sendPtr = eventPtr->sendData;
    ThreadClbkData      *clbkPtr = eventPtr->clbkData;
    ThreadEventResult* resultPtr = eventPtr->resultPtr;

    int code = TCL_ERROR; /* Pessimistic assumption */

    /*
     * See whether user has any preferences about which interpreter
     * to use for running this job. The job structure might identify
     * one. If not, just use the thread's main interpreter which is
     * stored in the thread specific data structure.
     * Note that later on we might discover that we're running the
     * async callback script. In this case, interpreter will be
     * changed to one given in the callback.
     */

    interp = (sendPtr && sendPtr->interp) ? sendPtr->interp : tsdPtr->interp;

    if (interp != NULL) {
        Tcl_Preserve(interp);

        if (clbkPtr && clbkPtr->threadId == thrId) {
            Tcl_Release(interp);
            /* Watch: this thread evaluates its own callback. */
            interp = clbkPtr->interp;
            Tcl_Preserve(interp);
        }

        Tcl_ResetResult(interp);

        if (sendPtr) {
            Tcl_CreateThreadExitHandler(ThreadFreeProc, sendPtr);
            if (clbkPtr) {
                Tcl_CreateThreadExitHandler(ThreadFreeProc,
                                            clbkPtr);
            }
            code = (*sendPtr->execProc)(interp, sendPtr);
            Tcl_DeleteThreadExitHandler(ThreadFreeProc, sendPtr);
            if (clbkPtr) {
                Tcl_DeleteThreadExitHandler(ThreadFreeProc,
                                            clbkPtr);
            }
        } else {
            code = TCL_OK;
        }
    }

    if (sendPtr) {
        ThreadFreeProc(sendPtr);
        eventPtr->sendData = NULL;
    }

    if (resultPtr) {

        /*
         * Report job result synchronously to waiting caller
         */

        Tcl_MutexLock(&threadMutex);
        ThreadSetResult(interp, code, resultPtr);
        Tcl_ConditionNotify(&resultPtr->done);
        Tcl_MutexUnlock(&threadMutex);

        /*
         * We still need to release the reference to the Tcl
         * interpreter added by ThreadSend whenever the callback
         * data is not NULL.
         */

        if (clbkPtr) {
            Tcl_Release(clbkPtr->interp);
        }
    } else if (clbkPtr && clbkPtr->threadId != thrId) {

        ThreadSendData *tmpPtr = (ThreadSendData*)clbkPtr;

        /*
         * Route the callback back to it's originator.
         * Do not wait for the result.
         */

        if (code != TCL_OK) {
            ThreadErrorProc(interp);
        }

        ThreadSetResult(interp, code, &clbkPtr->result);
        ThreadSend(interp, clbkPtr->threadId, tmpPtr, NULL, THREAD_SEND_CLBK);

    } else if (code != TCL_OK) {
        /*
         * Only pass errors onto the registered error handler
         * when we don't have a result target for this event.
         */
        ThreadErrorProc(interp);

        /*
         * We still need to release the reference to the Tcl
         * interpreter added by ThreadSend whenever the callback
         * data is not NULL.
         */

        if (clbkPtr) {
            Tcl_Release(clbkPtr->interp);
        }
    } else {
        /*
         * We still need to release the reference to the Tcl
         * interpreter added by ThreadSend whenever the callback
         * data is not NULL.
         */

        if (clbkPtr) {
            Tcl_Release(clbkPtr->interp);
        }
    }

    if (interp != NULL) {
        Tcl_Release(interp);
    }

    /*
     * Mark unwind scenario for this thread if the script resulted
     * in error condition and thread has been marked to unwind.
     * This will cause thread to disappear from the list of active
     * threads, clean-up its event queue and exit.
     */

    if (code != TCL_OK) {
        Tcl_MutexLock(&threadMutex);
        if (tsdPtr->flags & THREAD_FLAGS_UNWINDONERROR) {
            tsdPtr->flags |= THREAD_FLAGS_INERROR;
            if (tsdPtr->refCount == 0) {
                tsdPtr->flags |= THREAD_FLAGS_STOPPED;
            }
        }
        Tcl_MutexUnlock(&threadMutex);
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadSetResult --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static void
ThreadSetResult(
    Tcl_Interp *interp,
    int code,
    ThreadEventResult *resultPtr
) {
    size_t size;
    const char *errorCode, *errorInfo, *result;

    if (interp == NULL) {
        code      = TCL_ERROR;
        errorInfo = "";
        errorCode = "THREAD";
        result    = "no target interp!";
        size    = strlen(result);
        resultPtr->result = (size) ?
            (char *)memcpy(ckalloc(1+size), result, 1+size) : threadEmptyResult;
    } else {
        result = Tcl_GetString(Tcl_GetObjResult(interp));
        size = Tcl_GetObjResult(interp)->length;
        resultPtr->result = (size) ?
            (char *)memcpy(ckalloc(1+size), result, 1+size) : threadEmptyResult;
        if (code == TCL_ERROR) {
            errorCode = Tcl_GetVar2(interp, "errorCode", NULL, TCL_GLOBAL_ONLY);
            errorInfo = Tcl_GetVar2(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY);
        } else {
            errorCode = NULL;
            errorInfo = NULL;
        }
    }

    resultPtr->code = code;

    if (errorCode != NULL) {
        size = strlen(errorCode) + 1;
        resultPtr->errorCode = (char *)memcpy(ckalloc(size), errorCode, size);
    } else {
        resultPtr->errorCode = NULL;
    }
    if (errorInfo != NULL) {
        size = strlen(errorInfo) + 1;
        resultPtr->errorInfo = (char *)memcpy(ckalloc(size), errorInfo, size);
    } else {
        resultPtr->errorInfo = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadGetOption --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static int
ThreadGetOption(
    Tcl_Interp *interp,
    Tcl_ThreadId thrId,
    char *option,
    Tcl_DString *dsPtr
) {
    size_t len;
    ThreadSpecificData *tsdPtr = NULL;

    /*
     * If the optionName is NULL it means that we want
     * a list of all options and values.
     */

    len = (option == NULL) ? 0 : strlen(option);

    Tcl_MutexLock(&threadMutex);

    tsdPtr = ThreadExistsInner(thrId);

    if (tsdPtr == NULL) {
        Tcl_MutexUnlock(&threadMutex);
        ErrorNoSuchThread(interp, thrId);
        return TCL_ERROR;
    }

    if (len == 0 || (len > 3 && option[1] == 'e' && option[2] == 'v'
                     && !strncmp(option,"-eventmark", len))) {
        char buf[16];
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-eventmark");
        }
        sprintf(buf, "%d", tsdPtr->maxEventsCount);
        Tcl_DStringAppendElement(dsPtr, buf);
        if (len != 0) {
            Tcl_MutexUnlock(&threadMutex);
            return TCL_OK;
        }
    }

    if (len == 0 || (len > 2 && option[1] == 'u'
                     && !strncmp(option,"-unwindonerror", len))) {
        int flag = tsdPtr->flags & THREAD_FLAGS_UNWINDONERROR;
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-unwindonerror");
        }
        Tcl_DStringAppendElement(dsPtr, flag ? "1" : "0");
        if (len != 0) {
            Tcl_MutexUnlock(&threadMutex);
            return TCL_OK;
        }
    }

    if (len == 0 || (len > 3 && option[1] == 'e' && option[2] == 'r'
                     && !strncmp(option,"-errorstate", len))) {
        int flag = tsdPtr->flags & THREAD_FLAGS_INERROR;
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-errorstate");
        }
        Tcl_DStringAppendElement(dsPtr, flag ? "1" : "0");
        if (len != 0) {
            Tcl_MutexUnlock(&threadMutex);
            return TCL_OK;
        }
    }

    if (len != 0) {
        Tcl_AppendResult(interp, "bad option \"", option,
                         "\", should be one of -eventmark, "
                         "-unwindonerror or -errorstate", NULL);
        Tcl_MutexUnlock(&threadMutex);
        return TCL_ERROR;
    }

    Tcl_MutexUnlock(&threadMutex);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadSetOption --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static int
ThreadSetOption(
    Tcl_Interp *interp,
    Tcl_ThreadId thrId,
    char *option,
    char *value
) {
    size_t len = strlen(option);
    ThreadSpecificData *tsdPtr = NULL;

    Tcl_MutexLock(&threadMutex);

    tsdPtr = ThreadExistsInner(thrId);

    if (tsdPtr == NULL) {
        Tcl_MutexUnlock(&threadMutex);
        ErrorNoSuchThread(interp, thrId);
        return TCL_ERROR;
    }
    if (len > 3 && option[1] == 'e' && option[2] == 'v'
        && !strncmp(option,"-eventmark", len)) {
        if (sscanf(value, "%d", &tsdPtr->maxEventsCount) != 1) {
            Tcl_AppendResult(interp, "expected integer but got \"",
                             value, "\"", NULL);
            Tcl_MutexUnlock(&threadMutex);
            return TCL_ERROR;
        }
    } else if (len > 2 && option[1] == 'u'
               && !strncmp(option,"-unwindonerror", len)) {
        int flag = 0;
        if (Tcl_GetBoolean(interp, value, &flag) != TCL_OK) {
            Tcl_MutexUnlock(&threadMutex);
            return TCL_ERROR;
        }
        if (flag) {
            tsdPtr->flags |=  THREAD_FLAGS_UNWINDONERROR;
        } else {
            tsdPtr->flags &= ~THREAD_FLAGS_UNWINDONERROR;
        }
    } else if (len > 3 && option[1] == 'e' && option[2] == 'r'
               && !strncmp(option,"-errorstate", len)) {
        int flag = 0;
        if (Tcl_GetBoolean(interp, value, &flag) != TCL_OK) {
            Tcl_MutexUnlock(&threadMutex);
            return TCL_ERROR;
        }
        if (flag) {
            tsdPtr->flags |=  THREAD_FLAGS_INERROR;
        } else {
            tsdPtr->flags &= ~THREAD_FLAGS_INERROR;
        }
    }

    Tcl_MutexUnlock(&threadMutex);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadIdleProc --
 *
 * Results:
 *
 * Side effects.
 *
 *----------------------------------------------------------------------
 */

static void
ThreadIdleProc(
    ClientData clientData
) {
    int ret;
    ThreadSendData *sendPtr = (ThreadSendData*)clientData;

    ret = (*sendPtr->execProc)(sendPtr->interp, sendPtr);
    if (ret != TCL_OK) {
        ThreadErrorProc(sendPtr->interp);
    }

    Tcl_Release(sendPtr->interp);
    ThreadFreeProc(clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * TransferEventProc --
 *
 *  Handle a transfer event in the target thread.
 *
 * Results:
 *  Returns 1 to indicate that the event was processed.
 *
 * Side effects:
 *  Fills out the TransferResult struct.
 *
 *----------------------------------------------------------------------
 */

static int
TransferEventProc(
    Tcl_Event *evPtr,          /* Really ThreadEvent */
    int mask
) {
    ThreadSpecificData    *tsdPtr = TCL_TSD_INIT(&dataKey);
    TransferEvent       *eventPtr = (TransferEvent *)evPtr;
    TransferResult     *resultPtr = eventPtr->resultPtr;
    Tcl_Interp            *interp = tsdPtr->interp;
    int code;
    const char* msg = NULL;

    if (interp == NULL) {
        /*
         * Reject transfer in case of a missing target.
         */
        code = TCL_ERROR;
        msg  = "target interp missing";
    } else {
        /*
         * Add channel to current thread and interp.
         * See ThreadTransfer for more explanations.
         */
        if (Tcl_IsChannelExisting(Tcl_GetChannelName(eventPtr->chan))) {
            /*
             * Reject transfer. Channel of same name already exists in target.
             */
            code = TCL_ERROR;
            msg  = "channel already exists in target";
        } else {
            Tcl_SpliceChannel(eventPtr->chan);
            Tcl_RegisterChannel(interp, eventPtr->chan);
            Tcl_UnregisterChannel((Tcl_Interp *) NULL, eventPtr->chan);
            code = TCL_OK; /* Return success. */
        }
    }
    if (resultPtr) {
        Tcl_MutexLock(&threadMutex);
        resultPtr->resultCode = code;
        if (msg != NULL) {
            size_t size = strlen(msg)+1;
            resultPtr->resultMsg = (char *)memcpy(ckalloc(size), msg, size);
        }
        Tcl_ConditionNotify(&resultPtr->done);
        Tcl_MutexUnlock(&threadMutex);
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadFreeProc --
 *
 *  Called when we are exiting and memory needs to be freed.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Clears up mem specified in ClientData
 *
 *----------------------------------------------------------------------
 */
static void
ThreadFreeProc(
    ClientData clientData
) {
    /*
     * This will free send and/or callback structures
     * since both are the same in the beginning.
     */

    ThreadSendData *anyPtr = (ThreadSendData*)clientData;

    if (anyPtr) {
        if (anyPtr->clientData) {
            (*anyPtr->freeProc)(anyPtr->clientData);
        }
        ckfree((char*)anyPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadDeleteEvent --
 *
 *  This is called from the ThreadExitProc to delete memory related
 *  to events that we put on the queue.
 *
 * Results:
 *  1 it was our event and we want it removed, 0 otherwise.
 *
 * Side effects:
 *  It cleans up our events in the event queue for this thread.
 *
 *----------------------------------------------------------------------
 */
static int
ThreadDeleteEvent(
    Tcl_Event *eventPtr,       /* Really ThreadEvent */
    ClientData clientData      /* dummy */
) {
    if (eventPtr->proc == ThreadEventProc) {
        /*
         * Regular script event. Just dispose memory
         */
        ThreadEvent *evPtr = (ThreadEvent*)eventPtr;
        if (evPtr->sendData) {
            ThreadFreeProc(evPtr->sendData);
            evPtr->sendData = NULL;
        }
        if (evPtr->clbkData) {
            ThreadFreeProc(evPtr->clbkData);
            evPtr->clbkData = NULL;
        }
        return 1;
    }
    if (eventPtr->proc == TransferEventProc) {
        /*
         * A channel is in flight toward the thread just exiting.
         * Pass it back to the originator, if possible.
         * Else kill it.
         */
        TransferEvent* evPtr = (TransferEvent *) eventPtr;

        if (evPtr->resultPtr == (TransferResult *) NULL) {
            /* No thread to pass the channel back to. Kill it.
             * This requires to splice it temporarily into our channel
             * list and then forcing the ref.counter down to the real
             * value of zero. This destroys the channel.
             */

            Tcl_SpliceChannel(evPtr->chan);
            Tcl_UnregisterChannel((Tcl_Interp *) NULL, evPtr->chan);
            return 1;
        }

        /* Our caller (ThreadExitProc) will pass the channel back.
         */

        return 1;
    }

    /*
     * If it was NULL, we were in the middle of servicing the event
     * and it should be removed
     */

    return (eventPtr->proc == NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadExitProc --
 *
 *  This is called when the thread exits.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  It unblocks anyone that is waiting on a send to this thread.
 *  It cleans up any events in the event queue for this thread.
 *
 *----------------------------------------------------------------------
 */
static void
ThreadExitProc(
    ClientData clientData
) {
    char *threadEvalScript = (char*)clientData;
    const char *diemsg = "target thread died";
    ThreadEventResult *resultPtr, *nextPtr;
    Tcl_ThreadId self = Tcl_GetCurrentThread();
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    TransferResult *tResultPtr, *tNextPtr;

    if (threadEvalScript && threadEvalScript != threadEmptyResult) {
        ckfree((char*)threadEvalScript);
    }

    Tcl_MutexLock(&threadMutex);

    /*
     * NaviServer/AOLserver and threadpool threads get started/stopped
     * out of the control of this interface so this is
     * the first chance to split them out of the thread list.
     */

    ListRemoveInner(tsdPtr);

    /*
     * Delete events posted to our queue while we were running.
     * For threads exiting from the thread::wait command, this
     * has already been done in ThreadWait() function.
     * For one-shot threads, having something here is a very
     * strange condition. It *may* happen if somebody posts us
     * an event while we were in the middle of processing some
     * lengthly user script. It is unlikely to happen, though.
     */

    Tcl_DeleteEvents((Tcl_EventDeleteProc*)ThreadDeleteEvent, NULL);

    /*
     * Walk the list of threads waiting for result from us
     * and inform them that we're about to exit.
     */

    for (resultPtr = resultList; resultPtr; resultPtr = nextPtr) {
        nextPtr = resultPtr->nextPtr;
        if (resultPtr->srcThreadId == self) {

            /*
             * We are going away. By freeing up the result we signal
             * to the other thread we don't care about the result.
             */

            SpliceOut(resultPtr, resultList);
            ckfree((char*)resultPtr);

        } else if (resultPtr->dstThreadId == self) {

            /*
             * Dang. The target is going away. Unblock the caller.
             * The result string must be dynamically allocated
             * because the main thread is going to call free on it.
             */

            resultPtr->result = strcpy((char *)ckalloc(1+strlen(diemsg)), diemsg);
            resultPtr->code = TCL_ERROR;
            resultPtr->errorCode = resultPtr->errorInfo = NULL;
            Tcl_ConditionNotify(&resultPtr->done);
        }
    }
    for (tResultPtr = transferList; tResultPtr; tResultPtr = tNextPtr) {
        tNextPtr = tResultPtr->nextPtr;
        if (tResultPtr->srcThreadId == self) {
            /*
             * We are going away. By freeing up the result we signal
             * to the other thread we don't care about the result.
             *
             * This should not happen, as this thread should be in
             * ThreadTransfer at location (*).
             */

            SpliceOut(tResultPtr, transferList);
            ckfree((char*)tResultPtr);

        } else if (tResultPtr->dstThreadId == self) {
            /*
             * Dang. The target is going away. Unblock the caller.
             * The result string must be dynamically allocated
             * because the main thread is going to call free on it.
             */

            tResultPtr->resultMsg = strcpy((char *)ckalloc(1+strlen(diemsg)),
                                           diemsg);
            tResultPtr->resultCode = TCL_ERROR;
            Tcl_ConditionNotify(&tResultPtr->done);
        }
    }
    Tcl_MutexUnlock(&threadMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadGetHandle --
 *
 *  Construct the handle of the thread which is suitable
 *  to pass to Tcl.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void
ThreadGetHandle(
    Tcl_ThreadId thrId,
    char *handlePtr
) {
    sprintf(handlePtr, THREAD_HNDLPREFIX "%p", thrId);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadGetId --
 *
 *  Returns the ID of thread given it's Tcl handle.
 *
 * Results:
 *  Thread ID.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadGetId(
     Tcl_Interp *interp,
     Tcl_Obj *handleObj,
     Tcl_ThreadId *thrIdPtr
) {
    const char *thrHandle = Tcl_GetString(handleObj);

    if (sscanf(thrHandle, THREAD_HNDLPREFIX "%p", thrIdPtr) == 1) {
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "invalid thread handle \"",
                     thrHandle, "\"", NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 *  ErrorNoSuchThread --
 *
 *  Convenience function to set interpreter result when the thread
 *  given by it's ID cannot be found.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void
ErrorNoSuchThread(
    Tcl_Interp *interp,
    Tcl_ThreadId thrId
) {
    char thrHandle[THREAD_HNDLMAXLEN];

    ThreadGetHandle(thrId, thrHandle);
    Tcl_AppendResult(interp, "thread \"", thrHandle,
                     "\" does not exist", NULL);
}

/*
 *----------------------------------------------------------------------
 *
 *  ThreadCutChannel --
 *
 *  Dissociate a Tcl channel from the current thread/interp.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Events still pending in the thread event queue and ready to fire
 *  are not processed.
 *
 *----------------------------------------------------------------------
 */

static void
ThreadCutChannel(
    Tcl_Interp *interp,
    Tcl_Channel chan
) {
    Tcl_DriverWatchProc *watchProc;

    Tcl_ClearChannelHandlers(chan);

    watchProc   = Tcl_ChannelWatchProc(Tcl_GetChannelType(chan));

    /*
     * This effectively disables processing of pending
     * events which are ready to fire for the given
     * channel. If we do not do this, events will hit
     * the detached channel which is potentially being
     * owned by some other thread. This will wreck havoc
     * on our memory and eventually badly hurt us...
     */

    if (watchProc) {
        (*watchProc)(Tcl_GetChannelInstanceData(chan), 0);
    }

    /*
     * Artificially bump the channel reference count
     * which protects us from channel being closed
     * during the Tcl_UnregisterChannel().
     */

    Tcl_RegisterChannel((Tcl_Interp *) NULL, chan);
    Tcl_UnregisterChannel(interp, chan);

    Tcl_CutChannel(chan);
}

/* EOF $RCSfile: threadCmd.c,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */
