/*
 * threadPoolCmd.c --
 *
 * This file implements the Tcl thread pools.
 *
 * Copyright (c) 2002 by Zoran Vasiljevic.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ----------------------------------------------------------------------------
 */

#include "tclThreadInt.h"

/*
 * Structure to maintain idle poster threads
 */

typedef struct TpoolWaiter {
    Tcl_ThreadId threadId;         /* Thread id of the current thread */
    struct TpoolWaiter *nextPtr;   /* Next structure in the list */
    struct TpoolWaiter *prevPtr;   /* Previous structure in the list */
} TpoolWaiter;

/*
 * Structure describing an instance of a thread pool.
 */

typedef struct ThreadPool {
    Tcl_WideInt jobId;              /* Job counter */
    int idleTime;                   /* Time in secs a worker thread idles */
    int tearDown;                   /* Set to 1 to tear down the pool */
    int suspend;                    /* Set to 1 to suspend pool processing */
    char *initScript;               /* Script to initialize worker thread */
    char *exitScript;               /* Script to cleanup the worker */
    int minWorkers;                 /* Minimum number or worker threads */
    int maxWorkers;                 /* Maximum number of worker threads */
    int numWorkers;                 /* Current number of worker threads */
    int idleWorkers;                /* Number of idle workers */
    size_t refCount;                /* Reference counter for reserve/release */
    Tcl_Mutex mutex;                /* Pool mutex */
    Tcl_Condition cond;             /* Pool condition variable */
    Tcl_HashTable jobsDone;         /* Stores processed job results */
    struct TpoolResult *workTail;   /* Tail of the list with jobs pending*/
    struct TpoolResult *workHead;   /* Head of the list with jobs pending*/
    struct TpoolWaiter *waitTail;   /* Tail of the thread waiters list */
    struct TpoolWaiter *waitHead;   /* Head of the thread waiters list */
    struct ThreadPool *nextPtr;     /* Next structure in the threadpool list */
    struct ThreadPool *prevPtr;     /* Previous structure in threadpool list */
} ThreadPool;

#define TPOOL_HNDLPREFIX  "tpool"   /* Prefix to generate Tcl pool handles */
#define TPOOL_MINWORKERS  0         /* Default minimum # of worker threads */
#define TPOOL_MAXWORKERS  4         /* Default maximum # of worker threads */
#define TPOOL_IDLETIMER   0         /* Default worker thread idle timer */

/*
 * Structure for passing evaluation results
 */

typedef struct TpoolResult {
    int detached;                   /* Result is to be ignored */
    Tcl_WideInt jobId;              /* The job id of the current job */
    char *script;                   /* Script to evaluate in worker thread */
    size_t scriptLen;               /* Length of the script */
    int retcode;                    /* Tcl return code of the current job */
    char *result;                   /* Tcl result of the current job */
    char *errorCode;                /* On error: content of the errorCode */
    char *errorInfo;                /* On error: content of the errorInfo */
    Tcl_ThreadId threadId;          /* Originating thread id */
    ThreadPool *tpoolPtr;           /* Current thread pool */
    struct TpoolResult *nextPtr;
    struct TpoolResult *prevPtr;
} TpoolResult;

/*
 * Private structure for each worker/poster thread.
 */

typedef struct ThreadSpecificData {
    int stop;                       /* Set stop event; exit from event loop */
    TpoolWaiter *waitPtr;           /* Threads private idle structure */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * This global list maintains thread pools.
 */

static ThreadPool *tpoolList;
static Tcl_Mutex listMutex;
static Tcl_Mutex startMutex;

/*
 * Used to represent the empty result.
 */

static char *threadEmptyResult = (char *)"";

/*
 * Functions implementing Tcl commands
 */

static Tcl_ObjCmdProc TpoolCreateObjCmd;
static Tcl_ObjCmdProc TpoolPostObjCmd;
static Tcl_ObjCmdProc TpoolWaitObjCmd;
static Tcl_ObjCmdProc TpoolCancelObjCmd;
static Tcl_ObjCmdProc TpoolGetObjCmd;
static Tcl_ObjCmdProc TpoolReserveObjCmd;
static Tcl_ObjCmdProc TpoolReleaseObjCmd;
static Tcl_ObjCmdProc TpoolSuspendObjCmd;
static Tcl_ObjCmdProc TpoolResumeObjCmd;
static Tcl_ObjCmdProc TpoolNamesObjCmd;

/*
 * Miscelaneous functions used within this file
 */

static int
CreateWorker(Tcl_Interp *interp, ThreadPool *tpoolPtr);

static Tcl_ThreadCreateType
TpoolWorker(ClientData clientData);

static int
RunStopEvent(Tcl_Event *evPtr, int mask);

static void
PushWork(TpoolResult *rPtr, ThreadPool *tpoolPtr);

static TpoolResult*
PopWork(ThreadPool *tpoolPtr);

static void
PushWaiter(ThreadPool *tpoolPtr);

static TpoolWaiter*
PopWaiter(ThreadPool *tpoolPtr);

static void
SignalWaiter(ThreadPool *tpoolPtr);

static int
TpoolEval(Tcl_Interp *interp, char *script, size_t scriptLen,
                            TpoolResult *rPtr);
static void
SetResult(Tcl_Interp *interp, TpoolResult *rPtr);

static ThreadPool*
GetTpool(const char *tpoolName);

static ThreadPool*
GetTpoolUnl(const char *tpoolName);

static void
ThrExitHandler(ClientData clientData);

static void
AppExitHandler(ClientData clientData);

static int
TpoolReserve(ThreadPool *tpoolPtr);

static size_t
TpoolRelease(ThreadPool *tpoolPtr);

static void
TpoolSuspend(ThreadPool *tpoolPtr);

static void
TpoolResume(ThreadPool *tpoolPtr);

static void
InitWaiter(void);


/*
 *----------------------------------------------------------------------
 *
 * TpoolCreateObjCmd --
 *
 *  This procedure is invoked to process the "tpool::create" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolCreateObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int ii, minw, maxw, idle;
    char buf[64], *exs = NULL, *cmd = NULL;
    ThreadPool *tpoolPtr;

    /*
     * Syntax:  tpool::create ?-minworkers count?
     *                        ?-maxworkers count?
     *                        ?-initcmd script?
     *                        ?-exitcmd script?
     *                        ?-idletime seconds?
     */

    if (((objc-1) % 2)) {
        goto usage;
    }

    minw = TPOOL_MINWORKERS;
    maxw = TPOOL_MAXWORKERS;
    idle = TPOOL_IDLETIMER;

    /*
     * Parse the optional arguments
     */

    for (ii = 1; ii < objc; ii += 2) {
        char *opt = Tcl_GetString(objv[ii]);
        if (OPT_CMP(opt, "-minworkers")) {
            if (Tcl_GetIntFromObj(interp, objv[ii+1], &minw) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (OPT_CMP(opt, "-maxworkers")) {
            if (Tcl_GetIntFromObj(interp, objv[ii+1], &maxw) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (OPT_CMP(opt, "-idletime")) {
            if (Tcl_GetIntFromObj(interp, objv[ii+1], &idle) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (OPT_CMP(opt, "-initcmd")) {
            const char *val = Tcl_GetString(objv[ii+1]);
            cmd  = strcpy((char *)ckalloc(objv[ii+1]->length+1), val);
        } else if (OPT_CMP(opt, "-exitcmd")) {
            const char *val = Tcl_GetString(objv[ii+1]);
            exs  = strcpy((char *)ckalloc(objv[ii+1]->length+1), val);
        } else {
            goto usage;
        }
    }

    /*
     * Do some consistency checking
     */

    if (minw < 0) {
        minw = 0;
    }
    if (maxw < 0) {
        maxw = TPOOL_MAXWORKERS;
    }
    if (minw > maxw) {
        maxw = minw;
    }

    /*
     * Allocate and initialize thread pool structure
     */

    tpoolPtr = (ThreadPool*)ckalloc(sizeof(ThreadPool));
    memset(tpoolPtr, 0, sizeof(ThreadPool));

    tpoolPtr->minWorkers  = minw;
    tpoolPtr->maxWorkers  = maxw;
    tpoolPtr->idleTime    = idle;
    tpoolPtr->initScript  = cmd;
    tpoolPtr->exitScript  = exs;
    Tcl_InitHashTable(&tpoolPtr->jobsDone, TCL_ONE_WORD_KEYS);

    Tcl_MutexLock(&listMutex);
    SpliceIn(tpoolPtr, tpoolList);
    Tcl_MutexUnlock(&listMutex);

    /*
     * Start the required number of worker threads.
     * On failure to start any of them, tear-down
     * partially initialized pool.
     */

    Tcl_MutexLock(&tpoolPtr->mutex);
    for (ii = 0; ii < tpoolPtr->minWorkers; ii++) {
        if (CreateWorker(interp, tpoolPtr) != TCL_OK) {
            Tcl_MutexUnlock(&tpoolPtr->mutex);
            Tcl_MutexLock(&listMutex);
            TpoolRelease(tpoolPtr);
            Tcl_MutexUnlock(&listMutex);
            return TCL_ERROR;
        }
    }
    Tcl_MutexUnlock(&tpoolPtr->mutex);

    sprintf(buf, "%s%p", TPOOL_HNDLPREFIX, tpoolPtr);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, -1));

    return TCL_OK;

 usage:
    Tcl_WrongNumArgs(interp, 1, objv,
                     "?-minworkers count? ?-maxworkers count? "
                     "?-initcmd script? ?-exitcmd script? "
                     "?-idletime seconds?");
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolPostObjCmd --
 *
 *  This procedure is invoked to process the "tpool::post" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolPostObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    Tcl_WideInt jobId = 0;
    int ii, detached = 0, nowait = 0;
    size_t len;
    const char *tpoolName, *script;
    TpoolResult *rPtr;
    ThreadPool *tpoolPtr;

    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Syntax: tpool::post ?-detached? ?-nowait? tpoolId script
     */

    if (objc < 3 || objc > 5) {
        goto usage;
    }
    for (ii = 1; ii < objc; ii++) {
        char *opt = Tcl_GetString(objv[ii]);
        if (*opt != '-') {
            break;
        } else if (OPT_CMP(opt, "-detached")) {
            detached  = 1;
        } else if (OPT_CMP(opt, "-nowait")) {
            nowait = 1;
        } else {
            goto usage;
        }
    }

    /*
     * We expect exactly two arguments remaining after options
     */
    if (objc - ii != 2)
    {
        goto usage;
    }

    tpoolName = Tcl_GetString(objv[ii]);
    script    = Tcl_GetString(objv[ii+1]);
    len = objv[ii+1]->length;
    tpoolPtr  = GetTpool(tpoolName);
    if (tpoolPtr == NULL) {
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    /*
     * Initialize per-thread private data for this caller
     */

    InitWaiter();

    /*
     * See if any worker available to run the job.
     */

    Tcl_MutexLock(&tpoolPtr->mutex);
    if (nowait) {
        if (tpoolPtr->numWorkers == 0) {

            /*
             * Assure there is at least one worker running.
             */

            PushWaiter(tpoolPtr);
            if (CreateWorker(interp, tpoolPtr) != TCL_OK) {
                Tcl_MutexUnlock(&tpoolPtr->mutex);
                return TCL_ERROR;
            }

            /*
             * Wait for worker to start while servicing the event loop
             */

            Tcl_MutexUnlock(&tpoolPtr->mutex);
            tsdPtr->stop = -1;
            while(tsdPtr->stop == -1) {
                Tcl_DoOneEvent(TCL_ALL_EVENTS);
            }
            Tcl_MutexLock(&tpoolPtr->mutex);
        }
    } else {

        /*
         * If there are no idle worker threads, start some new
         * unless we are already running max number of workers.
         * In that case wait for the next one to become idle.
         */

        while (tpoolPtr->idleWorkers == 0) {
            PushWaiter(tpoolPtr);
            if (tpoolPtr->numWorkers < tpoolPtr->maxWorkers) {

                /*
                 * No more free workers; start new one
                 */

                if (CreateWorker(interp, tpoolPtr) != TCL_OK) {
                    Tcl_MutexUnlock(&tpoolPtr->mutex);
                    return TCL_ERROR;
                }
            }

            /*
             * Wait for worker to start while servicing the event loop
             */

            Tcl_MutexUnlock(&tpoolPtr->mutex);
            tsdPtr->stop = -1;
            while(tsdPtr->stop == -1) {
                Tcl_DoOneEvent(TCL_ALL_EVENTS);
            }
            Tcl_MutexLock(&tpoolPtr->mutex);
        }
    }

    /*
     * Create new job ticket and put it on the list.
     */

    rPtr = (TpoolResult*)ckalloc(sizeof(TpoolResult));
    memset(rPtr, 0, sizeof(TpoolResult));

    if (detached == 0) {
        jobId = ++tpoolPtr->jobId;
        rPtr->jobId = jobId;
    }

    rPtr->script    = strcpy((char *)ckalloc(len+1), script);
    rPtr->scriptLen = len;
    rPtr->detached  = detached;
    rPtr->threadId  = Tcl_GetCurrentThread();

    PushWork(rPtr, tpoolPtr);
    Tcl_ConditionNotify(&tpoolPtr->cond);
    Tcl_MutexUnlock(&tpoolPtr->mutex);

    if (detached == 0) {
        Tcl_SetObjResult(interp, Tcl_NewWideIntObj(jobId));
    }

    return TCL_OK;

  usage:
    Tcl_WrongNumArgs(interp, 1, objv, "?-detached? ?-nowait? tpoolId script");
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolWaitObjCmd --
 *
 *  This procedure is invoked to process the "tpool::wait" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolWaitObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int ii, done, wObjc;
    Tcl_WideInt jobId;
    char *tpoolName;
    Tcl_Obj *listVar = NULL;
    Tcl_Obj *waitList, *doneList, **wObjv;
    ThreadPool *tpoolPtr;
    TpoolResult *rPtr;
    Tcl_HashEntry *hPtr;

    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Syntax: tpool::wait tpoolId jobIdList ?listVar?
     */

    if (objc < 3 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "tpoolId jobIdList ?listVar");
        return TCL_ERROR;
    }
    if (objc == 4) {
        listVar = objv[3];
    }
    if (Tcl_ListObjGetElements(interp, objv[2], &wObjc, &wObjv) != TCL_OK) {
        return TCL_ERROR;
    }
    tpoolName = Tcl_GetString(objv[1]);
    tpoolPtr  = GetTpool(tpoolName);
    if (tpoolPtr == NULL) {
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    InitWaiter();
    done = 0; /* Number of elements in the done list */
    doneList = Tcl_NewListObj(0, NULL);

    Tcl_MutexLock(&tpoolPtr->mutex);
    while (1) {
        waitList = Tcl_NewListObj(0, NULL);
        for (ii = 0; ii < wObjc; ii++) {
            if (Tcl_GetWideIntFromObj(interp, wObjv[ii], &jobId) != TCL_OK) {
                Tcl_MutexUnlock(&tpoolPtr->mutex);
                return TCL_ERROR;
            }
            hPtr = Tcl_FindHashEntry(&tpoolPtr->jobsDone, (void *)(size_t)jobId);
            if (hPtr) {
                rPtr = (TpoolResult*)Tcl_GetHashValue(hPtr);
            } else {
                rPtr = NULL;
            }
            if (rPtr == NULL) {
                if (listVar) {
                    Tcl_ListObjAppendElement(interp, waitList, wObjv[ii]);
                }
            } else if (!rPtr->detached && rPtr->result) {
                done++;
                Tcl_ListObjAppendElement(interp, doneList, wObjv[ii]);
            } else if (listVar) {
                Tcl_ListObjAppendElement(interp, waitList, wObjv[ii]);
            }
        }
        if (done) {
            break;
        }

        /*
         * None of the jobs done, wait for completion
         * of the next job and try again.
         */

        Tcl_DecrRefCount(waitList);
        PushWaiter(tpoolPtr);

        Tcl_MutexUnlock(&tpoolPtr->mutex);
        tsdPtr->stop = -1;
        while (tsdPtr->stop == -1) {
            Tcl_DoOneEvent(TCL_ALL_EVENTS);
        }
        Tcl_MutexLock(&tpoolPtr->mutex);
    }
    Tcl_MutexUnlock(&tpoolPtr->mutex);

    if (listVar) {
        Tcl_ObjSetVar2(interp, listVar, NULL, waitList, 0);
    }

    Tcl_SetObjResult(interp, doneList);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolCancelObjCmd --
 *
 *  This procedure is invoked to process the "tpool::cancel" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolCancelObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int ii, wObjc;
    Tcl_WideInt jobId;
    char *tpoolName;
    Tcl_Obj *listVar = NULL;
    Tcl_Obj *doneList, *waitList, **wObjv;
    ThreadPool *tpoolPtr;
    TpoolResult *rPtr;

    /*
     * Syntax: tpool::cancel tpoolId jobIdList ?listVar?
     */

    if (objc < 3 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "tpoolId jobIdList ?listVar");
        return TCL_ERROR;
    }
    if (objc == 4) {
        listVar = objv[3];
    }
    if (Tcl_ListObjGetElements(interp, objv[2], &wObjc, &wObjv) != TCL_OK) {
        return TCL_ERROR;
    }
    tpoolName = Tcl_GetString(objv[1]);
    tpoolPtr  = GetTpool(tpoolName);
    if (tpoolPtr == NULL) {
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    InitWaiter();
    doneList = Tcl_NewListObj(0, NULL);
    waitList = Tcl_NewListObj(0, NULL);

    Tcl_MutexLock(&tpoolPtr->mutex);
    for (ii = 0; ii < wObjc; ii++) {
        if (Tcl_GetWideIntFromObj(interp, wObjv[ii], &jobId) != TCL_OK) {
            return TCL_ERROR;
        }
        for (rPtr = tpoolPtr->workHead; rPtr; rPtr = rPtr->nextPtr) {
            if (rPtr->jobId == jobId) {
                if (rPtr->prevPtr != NULL) {
                    rPtr->prevPtr->nextPtr = rPtr->nextPtr;
                } else {
                    tpoolPtr->workHead = rPtr->nextPtr;
                }
                if (rPtr->nextPtr != NULL) {
                    rPtr->nextPtr->prevPtr = rPtr->prevPtr;
                } else {
                    tpoolPtr->workTail = rPtr->prevPtr;
                }
                SetResult(NULL, rPtr); /* Just to free the result */
                ckfree(rPtr->script);
                ckfree((char*)rPtr);
                Tcl_ListObjAppendElement(interp, doneList, wObjv[ii]);
                break;
            }
        }
        if (rPtr == NULL && listVar) {
            Tcl_ListObjAppendElement(interp, waitList, wObjv[ii]);
        }
    }
    Tcl_MutexUnlock(&tpoolPtr->mutex);

    if (listVar) {
        Tcl_ObjSetVar2(interp, listVar, NULL, waitList, 0);
    }

    Tcl_SetObjResult(interp, doneList);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolGetObjCmd --
 *
 *  This procedure is invoked to process the "tpool::get" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolGetObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int ret;
    Tcl_WideInt jobId;
    char *tpoolName;
    Tcl_Obj *resVar = NULL;
    ThreadPool *tpoolPtr;
    TpoolResult *rPtr;
    Tcl_HashEntry *hPtr;

    /*
     * Syntax: tpool::get tpoolId jobId ?result?
     */

    if (objc < 3 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "tpoolId jobId ?result?");
        return TCL_ERROR;
    }
    if (Tcl_GetWideIntFromObj(interp, objv[2], &jobId) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        resVar = objv[3];
    }

    /*
     * Locate the threadpool
     */

    tpoolName = Tcl_GetString(objv[1]);
    tpoolPtr  = GetTpool(tpoolName);
    if (tpoolPtr == NULL) {
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    /*
     * Locate the job in question. It is an error to
     * do a "get" on bogus job handle or on the job
     * which did not complete yet.
     */

    Tcl_MutexLock(&tpoolPtr->mutex);
    hPtr = Tcl_FindHashEntry(&tpoolPtr->jobsDone, (void *)(size_t)jobId);
    if (hPtr == NULL) {
        Tcl_MutexUnlock(&tpoolPtr->mutex);
        Tcl_AppendResult(interp, "no such job", NULL);
        return TCL_ERROR;
    }
    rPtr = (TpoolResult*)Tcl_GetHashValue(hPtr);
    if (rPtr->result == NULL) {
        Tcl_MutexUnlock(&tpoolPtr->mutex);
        Tcl_AppendResult(interp, "job not completed", NULL);
        return TCL_ERROR;
    }

    Tcl_DeleteHashEntry(hPtr);
    Tcl_MutexUnlock(&tpoolPtr->mutex);

    ret = rPtr->retcode;
    SetResult(interp, rPtr);
    ckfree((char*)rPtr);

    if (resVar) {
        Tcl_ObjSetVar2(interp, resVar, NULL, Tcl_GetObjResult(interp), 0);
        Tcl_SetObjResult(interp, Tcl_NewIntObj(ret));
        ret = TCL_OK;
    }

    return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolReserveObjCmd --
 *
 *  This procedure is invoked to process the "tpool::preserve" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolReserveObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    int ret;
    char *tpoolName;
    ThreadPool *tpoolPtr;

    /*
     * Syntax: tpool::preserve tpoolId
     */

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "tpoolId");
        return TCL_ERROR;
    }

    tpoolName = Tcl_GetString(objv[1]);

    Tcl_MutexLock(&listMutex);
    tpoolPtr  = GetTpoolUnl(tpoolName);
    if (tpoolPtr == NULL) {
        Tcl_MutexUnlock(&listMutex);
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    ret = TpoolReserve(tpoolPtr);
    Tcl_MutexUnlock(&listMutex);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(ret));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolReleaseObjCmd --
 *
 *  This procedure is invoked to process the "tpool::release" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolReleaseObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    size_t ret;
    char *tpoolName;
    ThreadPool *tpoolPtr;

    /*
     * Syntax: tpool::release tpoolId
     */

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "tpoolId");
        return TCL_ERROR;
    }

    tpoolName = Tcl_GetString(objv[1]);

    Tcl_MutexLock(&listMutex);
    tpoolPtr  = GetTpoolUnl(tpoolName);
    if (tpoolPtr == NULL) {
        Tcl_MutexUnlock(&listMutex);
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    ret = TpoolRelease(tpoolPtr);
    Tcl_MutexUnlock(&listMutex);
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(ret));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolSuspendObjCmd --
 *
 *  This procedure is invoked to process the "tpool::suspend" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolSuspendObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    char *tpoolName;
    ThreadPool *tpoolPtr;

    /*
     * Syntax: tpool::suspend tpoolId
     */

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "tpoolId");
        return TCL_ERROR;
    }

    tpoolName = Tcl_GetString(objv[1]);
    tpoolPtr  = GetTpool(tpoolName);

    if (tpoolPtr == NULL) {
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    TpoolSuspend(tpoolPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolResumeObjCmd --
 *
 *  This procedure is invoked to process the "tpool::resume" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolResumeObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    char *tpoolName;
    ThreadPool *tpoolPtr;

    /*
     * Syntax: tpool::resume tpoolId
     */

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "tpoolId");
        return TCL_ERROR;
    }

    tpoolName = Tcl_GetString(objv[1]);
    tpoolPtr  = GetTpool(tpoolName);

    if (tpoolPtr == NULL) {
        Tcl_AppendResult(interp, "can not find threadpool \"", tpoolName,
                         "\"", NULL);
        return TCL_ERROR;
    }

    TpoolResume(tpoolPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolNamesObjCmd --
 *
 *  This procedure is invoked to process the "tpool::names" Tcl
 *  command. See the user documentation for details on what it does.
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
TpoolNamesObjCmd(
    ClientData  dummy,         /* Not used. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int         objc,          /* Number of arguments. */
    Tcl_Obj    *const objv[]   /* Argument objects. */
) {
    ThreadPool *tpoolPtr;
    Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);

    Tcl_MutexLock(&listMutex);
    for (tpoolPtr = tpoolList; tpoolPtr; tpoolPtr = tpoolPtr->nextPtr) {
        char buf[32];
        sprintf(buf, "%s%p", TPOOL_HNDLPREFIX, tpoolPtr);
        Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(buf,-1));
    }
    Tcl_MutexUnlock(&listMutex);
    Tcl_SetObjResult(interp, listObj);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateWorker --
 *
 *  Creates new worker thread for the given pool. Assumes the caller
 *  holds the pool mutex.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Informs waiter thread (if any) about the new worker thread.
 *
 *----------------------------------------------------------------------
 */
static int
CreateWorker(
    Tcl_Interp *interp,
    ThreadPool *tpoolPtr
) {
    Tcl_ThreadId id;
    TpoolResult result;

    /*
     * Initialize the result structure to be
     * passed to the new thread. This is used
     * as communication to and from the thread.
     */

    memset(&result, 0, sizeof(TpoolResult));
    result.retcode  = -1;
    result.tpoolPtr = tpoolPtr;

    /*
     * Create new worker thread here. Wait for the thread to start
     * because it's using the ThreadResult arg which is on our stack.
     */

    Tcl_MutexLock(&startMutex);
    if (Tcl_CreateThread(&id, TpoolWorker, &result,
                         TCL_THREAD_STACK_DEFAULT, 0) != TCL_OK) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("can't create a new thread", -1));
        Tcl_MutexUnlock(&startMutex);
        return TCL_ERROR;
    }
    while(result.retcode == -1) {
        Tcl_ConditionWait(&tpoolPtr->cond, &startMutex, NULL);
    }
    Tcl_MutexUnlock(&startMutex);

    /*
     * Set error-related information if the thread
     * failed to initialize correctly.
     */

    if (result.retcode == 1) {
        result.retcode = TCL_ERROR;
        SetResult(interp, &result);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolWorker --
 *
 *  This is the main function of each of the threads in the pool.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_ThreadCreateType
TpoolWorker(
    ClientData clientData
) {
    TpoolResult          *rPtr = (TpoolResult*)clientData;
    ThreadPool       *tpoolPtr = rPtr->tpoolPtr;

    int tout = 0;
    Tcl_Interp *interp;
    Tcl_Time waitTime, *idlePtr;
    const char *errMsg;

    Tcl_MutexLock(&startMutex);

    /*
     * Initialize the Tcl interpreter
     */

#ifdef NS_AOLSERVER
    interp = (Tcl_Interp*)Ns_TclAllocateInterp(NULL);
    rPtr->retcode = 0;
#else
    interp = Tcl_CreateInterp();
    if (Tcl_Init(interp) != TCL_OK) {
        rPtr->retcode = 1;
    } else if (Thread_Init(interp) != TCL_OK) {
        rPtr->retcode = 1;
    } else {
        rPtr->retcode = 0;
    }
#endif

    if (rPtr->retcode == 1) {
        errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
        rPtr->result = strcpy((char *)ckalloc(strlen(errMsg)+1), errMsg);
        Tcl_ConditionNotify(&tpoolPtr->cond);
        Tcl_MutexUnlock(&startMutex);
        goto out;
    }

    /*
     * Initialize the interpreter
     */

    if (tpoolPtr->initScript) {
        TpoolEval(interp, tpoolPtr->initScript, -1, rPtr);
        if (rPtr->retcode != TCL_OK) {
            rPtr->retcode = 1;
            errMsg = Tcl_GetString(Tcl_GetObjResult(interp));
            rPtr->result  = strcpy((char *)ckalloc(strlen(errMsg)+1), errMsg);
            Tcl_ConditionNotify(&tpoolPtr->cond);
            Tcl_MutexUnlock(&startMutex);
            goto out;
        }
    }

    /*
     * Setup idle timer
     */

    if (tpoolPtr->idleTime == 0) {
        idlePtr = NULL;
    } else {
        waitTime.sec  = tpoolPtr->idleTime;
        waitTime.usec = 0;
        idlePtr = &waitTime;
    }

    /*
     * Tell caller we've started
     */

    tpoolPtr->numWorkers++;
    Tcl_ConditionNotify(&tpoolPtr->cond);
    Tcl_MutexUnlock(&startMutex);

    /*
     * Wait for jobs to arrive. Note the handcrafted time test.
     * Tcl API misses the return value of the Tcl_ConditionWait().
     * Hence, we do not know why the call returned. Was it someone
     * signalled the variable or has the idle timer expired?
     */

    Tcl_MutexLock(&tpoolPtr->mutex);
    while (!tpoolPtr->tearDown) {
        SignalWaiter(tpoolPtr);
        tpoolPtr->idleWorkers++;
        rPtr = NULL;
        tout = 0;
        while (tpoolPtr->suspend
               || (!tpoolPtr->tearDown && !tout
                   && (rPtr = PopWork(tpoolPtr)) == NULL)) {
            if (tpoolPtr->suspend && rPtr == NULL) {
                Tcl_ConditionWait(&tpoolPtr->cond, &tpoolPtr->mutex, NULL);
            } else if (rPtr == NULL) {
                Tcl_Time t1, t2;
                Tcl_GetTime(&t1);
                Tcl_ConditionWait(&tpoolPtr->cond, &tpoolPtr->mutex, idlePtr);
                Tcl_GetTime(&t2);
                if (tpoolPtr->idleTime > 0) {
                    tout = (t2.sec - t1.sec) >= tpoolPtr->idleTime;
                }
            }
        }
        tpoolPtr->idleWorkers--;
        if (rPtr == NULL) {
            if (tpoolPtr->numWorkers > tpoolPtr->minWorkers) {
                break; /* Enough workers, can safely kill this one */
            } else {
                continue; /* Worker count at min, leave this one alive */
            }
        } else if (tpoolPtr->tearDown) {
            PushWork(rPtr, tpoolPtr);
            break; /* Kill worker because pool is going down */
        }
        Tcl_MutexUnlock(&tpoolPtr->mutex);
        TpoolEval(interp, rPtr->script, rPtr->scriptLen, rPtr);
        ckfree(rPtr->script);
        Tcl_MutexLock(&tpoolPtr->mutex);
        if (!rPtr->detached) {
            int isNew;
            Tcl_SetHashValue(Tcl_CreateHashEntry(&tpoolPtr->jobsDone,
                                                 (void *)(size_t)rPtr->jobId, &isNew),
                             rPtr);
            SignalWaiter(tpoolPtr);
        } else {
            ckfree((char*)rPtr);
        }
    }

    /*
     * Tear down the worker
     */

    if (tpoolPtr->exitScript) {
        TpoolEval(interp, tpoolPtr->exitScript, -1, NULL);
    }

    tpoolPtr->numWorkers--;
    SignalWaiter(tpoolPtr);
    Tcl_MutexUnlock(&tpoolPtr->mutex);

 out:

#ifdef NS_AOLSERVER
    Ns_TclMarkForDelete(interp);
    Ns_TclDeAllocateInterp(interp);
#else
    Tcl_DeleteInterp(interp);
#endif
    Tcl_ExitThread(0);

    TCL_THREAD_CREATE_RETURN;
}

/*
 *----------------------------------------------------------------------
 *
 * RunStopEvent --
 *
 *  Signalizes the waiter thread to stop waiting.
 *
 * Results:
 *  1 (always)
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static int
RunStopEvent(
    Tcl_Event *eventPtr,
    int mask
) {
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    tsdPtr->stop = 1;
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * PushWork --
 *
 *  Adds a worker thread to the end of the workers list.
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
PushWork(
    TpoolResult *rPtr,
    ThreadPool *tpoolPtr
) {
    SpliceIn(rPtr, tpoolPtr->workHead);
    if (tpoolPtr->workTail == NULL) {
        tpoolPtr->workTail = rPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PopWork --
 *
 *  Pops the work ticket from the list
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static TpoolResult *
PopWork(
    ThreadPool *tpoolPtr
) {
    TpoolResult *rPtr = tpoolPtr->workTail;

    if (rPtr == NULL) {
        return NULL;
    }

    tpoolPtr->workTail = rPtr->prevPtr;
    SpliceOut(rPtr, tpoolPtr->workHead);

    rPtr->nextPtr = rPtr->prevPtr = NULL;

    return rPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * PushWaiter --
 *
 *  Adds a waiter thread to the end of the waiters list.
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
PushWaiter(
    ThreadPool *tpoolPtr
) {
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    SpliceIn(tsdPtr->waitPtr, tpoolPtr->waitHead);
    if (tpoolPtr->waitTail == NULL) {
        tpoolPtr->waitTail = tsdPtr->waitPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PopWaiter --
 *
 *  Pops the first waiter from the head of the waiters list.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static TpoolWaiter*
PopWaiter(
    ThreadPool *tpoolPtr
) {
    TpoolWaiter *waitPtr =  tpoolPtr->waitTail;

    if (waitPtr == NULL) {
        return NULL;
    }

    tpoolPtr->waitTail = waitPtr->prevPtr;
    SpliceOut(waitPtr, tpoolPtr->waitHead);

    waitPtr->prevPtr = waitPtr->nextPtr = NULL;

    return waitPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTpool
 *
 *  Parses the Tcl threadpool handle and locates the
 *  corresponding threadpool maintenance structure.
 *
 * Results:
 *  Pointer to the threadpool struct or NULL if none found,
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static ThreadPool*
GetTpool(
    const char *tpoolName
) {
    ThreadPool *tpoolPtr;

    Tcl_MutexLock(&listMutex);
    tpoolPtr = GetTpoolUnl(tpoolName);
    Tcl_MutexUnlock(&listMutex);

    return tpoolPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTpoolUnl
 *
 *  Parses the threadpool handle and locates the
 *  corresponding threadpool maintenance structure.
 *  Assumes caller holds the listMutex,
 *
 * Results:
 *  Pointer to the threadpool struct or NULL if none found,
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static ThreadPool*
GetTpoolUnl (
    const char *tpoolName
) {
    ThreadPool *tpool;
    ThreadPool *tpoolPtr = NULL;

    if (sscanf(tpoolName, TPOOL_HNDLPREFIX"%p", &tpool) != 1) {
        return NULL;
    }
    for (tpoolPtr = tpoolList; tpoolPtr; tpoolPtr = tpoolPtr->nextPtr) {
        if (tpoolPtr == tpool) {
            break;
        }
    }

    return tpoolPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolEval
 *
 *  Evaluates the script and fills in the result structure.
 *
 * Results:
 *  Standard Tcl result,
 *
 * Side effects:
 *  Many, depending on the script.
 *
 *----------------------------------------------------------------------
 */
static int
TpoolEval(
    Tcl_Interp *interp,
    char *script,
    size_t scriptLen,
    TpoolResult *rPtr
) {
    int ret;
    size_t reslen;
    const char *result;
    const char *errorCode, *errorInfo;

    ret = Tcl_EvalEx(interp, script, scriptLen, TCL_EVAL_GLOBAL);
    if (rPtr == NULL || rPtr->detached) {
        return ret;
    }
    rPtr->retcode = ret;
    if (ret == TCL_ERROR) {
        errorCode = Tcl_GetVar2(interp, "errorCode", NULL, TCL_GLOBAL_ONLY);
        errorInfo = Tcl_GetVar2(interp, "errorInfo", NULL, TCL_GLOBAL_ONLY);
        if (errorCode != NULL) {
            rPtr->errorCode = (char *)ckalloc(1 + strlen(errorCode));
            strcpy(rPtr->errorCode, errorCode);
        }
        if (errorInfo != NULL) {
            rPtr->errorInfo = (char *)ckalloc(1 + strlen(errorInfo));
            strcpy(rPtr->errorInfo, errorInfo);
        }
    }

    result = Tcl_GetString(Tcl_GetObjResult(interp));
    reslen = Tcl_GetObjResult(interp)->length;

    if (reslen == 0) {
        rPtr->result = threadEmptyResult;
    } else {
        rPtr->result = strcpy((char *)ckalloc(1 + reslen), result);
    }

    return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * SetResult
 *
 *  Sets the result in current interpreter.
 *
 * Results:
 *  Standard Tcl result,
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static void
SetResult(
    Tcl_Interp *interp,
    TpoolResult *rPtr
) {
    if (rPtr->retcode == TCL_ERROR) {
        if (rPtr->errorCode) {
            if (interp) {
                Tcl_SetObjErrorCode(interp,Tcl_NewStringObj(rPtr->errorCode,-1));
            }
            ckfree(rPtr->errorCode);
            rPtr->errorCode = NULL;
        }
        if (rPtr->errorInfo) {
            if (interp) {
                Tcl_AddErrorInfo(interp, rPtr->errorInfo);
            }
            ckfree(rPtr->errorInfo);
            rPtr->errorInfo = NULL;
        }
    }
    if (rPtr->result) {
        if (rPtr->result == threadEmptyResult) {
            if (interp) {
                Tcl_ResetResult(interp);
            }
        } else {
            if (interp) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj(rPtr->result,-1));
            }
            ckfree(rPtr->result);
            rPtr->result = NULL;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolReserve --
 *
 *  Does the pool preserve and/or release. Assumes caller holds
 *  the listMutex.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  May tear-down the threadpool if refcount drops to 0 or below.
 *
 *----------------------------------------------------------------------
 */
static int
TpoolReserve(
    ThreadPool *tpoolPtr
) {
    return ++tpoolPtr->refCount;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolRelease --
 *
 *  Does the pool preserve and/or release. Assumes caller holds
 *  the listMutex.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  May tear-down the threadpool if refcount drops to 0 or below.
 *
 *----------------------------------------------------------------------
 */
static size_t
TpoolRelease(
    ThreadPool *tpoolPtr
) {
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    TpoolResult *rPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    if (tpoolPtr->refCount-- > 1) {
        return tpoolPtr->refCount;
    }

    /*
     * Pool is going away; remove from the list of pools,
     */

    SpliceOut(tpoolPtr, tpoolList);
    InitWaiter();

    /*
     * Signal and wait for all workers to die.
     */

    Tcl_MutexLock(&tpoolPtr->mutex);
    tpoolPtr->tearDown = 1;
    while (tpoolPtr->numWorkers > 0) {
        PushWaiter(tpoolPtr);
        Tcl_ConditionNotify(&tpoolPtr->cond);
        Tcl_MutexUnlock(&tpoolPtr->mutex);
        tsdPtr->stop = -1;
        while(tsdPtr->stop == -1) {
            Tcl_DoOneEvent(TCL_ALL_EVENTS);
        }
        Tcl_MutexLock(&tpoolPtr->mutex);
    }
    Tcl_MutexUnlock(&tpoolPtr->mutex);

    /*
     * Tear down the pool structure
     */

    if (tpoolPtr->initScript) {
        ckfree(tpoolPtr->initScript);
    }
    if (tpoolPtr->exitScript) {
        ckfree(tpoolPtr->exitScript);
    }

    /*
     * Cleanup completed but not collected jobs
     */

    hPtr = Tcl_FirstHashEntry(&tpoolPtr->jobsDone, &search);
    while (hPtr != NULL) {
        rPtr = (TpoolResult*)Tcl_GetHashValue(hPtr);
        if (rPtr->result && rPtr->result != threadEmptyResult) {
            ckfree(rPtr->result);
        }
        if (rPtr->retcode == TCL_ERROR) {
            if (rPtr->errorInfo) {
                ckfree(rPtr->errorInfo);
            }
            if (rPtr->errorCode) {
                ckfree(rPtr->errorCode);
            }
        }
        ckfree((char*)rPtr);
        Tcl_DeleteHashEntry(hPtr);
        hPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&tpoolPtr->jobsDone);

    /*
     * Cleanup jobs posted but never completed.
     */

    for (rPtr = tpoolPtr->workHead; rPtr; rPtr = rPtr->nextPtr) {
        ckfree(rPtr->script);
        ckfree((char*)rPtr);
    }
    Tcl_MutexFinalize(&tpoolPtr->mutex);
    Tcl_ConditionFinalize(&tpoolPtr->cond);
    ckfree((char*)tpoolPtr);

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolSuspend --
 *
 *  Marks the pool as suspended. This prevents pool workers to drain
 *  the pool work queue.
 *
 * Results:
 *  Value of the suspend flag (1 always).
 *
 * Side effects:
 *  During the suspended state, pool worker threads wlll not timeout
 *  even if the worker inactivity timer has been configured.
 *
 *----------------------------------------------------------------------
 */
static void
TpoolSuspend(
    ThreadPool *tpoolPtr
) {
    Tcl_MutexLock(&tpoolPtr->mutex);
    tpoolPtr->suspend = 1;
    Tcl_MutexUnlock(&tpoolPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * TpoolResume --
 *
 *  Clears the pool suspended state. This allows pool workers to drain
 *  the pool work queue again.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Pool workers may be started or awaken.
 *
 *----------------------------------------------------------------------
 */
static void
TpoolResume(
    ThreadPool *tpoolPtr
) {
    Tcl_MutexLock(&tpoolPtr->mutex);
    tpoolPtr->suspend = 0;
    Tcl_ConditionNotify(&tpoolPtr->cond);
    Tcl_MutexUnlock(&tpoolPtr->mutex);
}

/*
 *----------------------------------------------------------------------
 *
 * SignalWaiter --
 *
 *  Signals the waiter thread.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  The waiter thread will exit from the event loop.
 *
 *----------------------------------------------------------------------
 */
static void
SignalWaiter(
    ThreadPool *tpoolPtr
) {
    TpoolWaiter *waitPtr;
    Tcl_Event *evPtr;

    waitPtr = PopWaiter(tpoolPtr);
    if (waitPtr == NULL) {
        return;
    }

    evPtr = (Tcl_Event*)ckalloc(sizeof(Tcl_Event));
    evPtr->proc = RunStopEvent;

    Tcl_ThreadQueueEvent(waitPtr->threadId,(Tcl_Event*)evPtr,TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(waitPtr->threadId);
}

/*
 *----------------------------------------------------------------------
 *
 * InitWaiter --
 *
 *  Setup poster thread to be able to wait in the event loop.
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
InitWaiter ()
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->waitPtr == NULL) {
        tsdPtr->waitPtr = (TpoolWaiter*)ckalloc(sizeof(TpoolWaiter));
        tsdPtr->waitPtr->prevPtr  = NULL;
        tsdPtr->waitPtr->nextPtr  = NULL;
        tsdPtr->waitPtr->threadId = Tcl_GetCurrentThread();
        Tcl_CreateThreadExitHandler(ThrExitHandler, tsdPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ThrExitHandler --
 *
 *  Performs cleanup when a caller (poster) thread exits.
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
ThrExitHandler(
    ClientData clientData
) {
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)clientData;

    ckfree((char*)tsdPtr->waitPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * AppExitHandler
 *
 *  Deletes all threadpools on application exit.
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
AppExitHandler(
    ClientData clientData
) {
    ThreadPool *tpoolPtr;

    Tcl_MutexLock(&listMutex);
    /*
     * Restart with head of list each time until empty. [Bug 1427570]
     */
    for (tpoolPtr = tpoolList; tpoolPtr; tpoolPtr = tpoolList) {
        TpoolRelease(tpoolPtr);
    }
    Tcl_MutexUnlock(&listMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tpool_Init --
 *
 *  Create commands in current interpreter.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  On first load, creates application exit handler to clean up
 *  any threadpools left.
 *
 *----------------------------------------------------------------------
 */

int
Tpool_Init (
    Tcl_Interp *interp                 /* Interp where to create cmds */
) {
    static int initialized;

    TCL_CMD(interp, TPOOL_CMD_PREFIX"create",   TpoolCreateObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"names",    TpoolNamesObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"post",     TpoolPostObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"wait",     TpoolWaitObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"cancel",   TpoolCancelObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"get",      TpoolGetObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"preserve", TpoolReserveObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"release",  TpoolReleaseObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"suspend",  TpoolSuspendObjCmd);
    TCL_CMD(interp, TPOOL_CMD_PREFIX"resume",   TpoolResumeObjCmd);

    if (initialized == 0) {
        Tcl_MutexLock(&listMutex);
        if (initialized == 0) {
            Tcl_CreateExitHandler(AppExitHandler, (ClientData)-1);
            initialized = 1;
        }
        Tcl_MutexUnlock(&listMutex);
    }
    return TCL_OK;
}

/* EOF $RCSfile: threadPoolCmd.c,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */
