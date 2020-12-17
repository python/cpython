/*
 * threadSpCmd.c --
 *
 * This file implements commands for script-level access to thread
 * synchronization primitives. Currently, the exclusive mutex, the
 * recursive mutex. the reader/writer mutex and condition variable
 * objects are exposed to the script programmer.
 *
 * Additionaly, a locked eval is also implemented. This is a practical
 * convenience function which relieves the programmer from the need
 * to take care about unlocking some mutex after evaluating a protected
 * part of code. The locked eval is recursive-savvy since it used the
 * recursive mutex for internal locking.
 *
 * The Tcl interface to the locking and synchronization primitives
 * attempts to catch some very common problems in thread programming
 * like attempting to lock an exclusive mutex twice from the same
 * thread (deadlock), waiting on the condition variable without
 * locking the mutex, destroying primitives while being used, etc...
 * This all comes with some additional internal locking costs but
 * the benefits outweight the costs, especially considering overall
 * performance (or lack of it) of an interpreted laguage like Tcl is.
 *
 * Copyright (c) 2002 by Zoran Vasiljevic.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ----------------------------------------------------------------------------
 */

#include "tclThreadInt.h"
#include "threadSpCmd.h"

/*
 * Types of synchronization variables we support.
 */

#define EMUTEXID  'm' /* First letter of the exclusive mutex name */
#define RMUTEXID  'r' /* First letter of the recursive mutex name */
#define WMUTEXID  'w' /* First letter of the read/write mutex name */
#define CONDVID   'c' /* First letter of the condition variable name */

#define SP_MUTEX   1  /* Any kind of mutex */
#define SP_CONDV   2  /* The condition variable sync type */

/*
 * Structure representing one sync primitive (mutex, condition variable).
 * We use buckets to manage Tcl names of sync primitives. Each bucket
 * is associated with a mutex. Each time we process the Tcl name of an
 * sync primitive, we compute it's (trivial) hash and use this hash to
 * address one of pre-allocated buckets.
 * The bucket internally utilzes a hash-table to store item pointers.
 * Item pointers are identified by a simple xid1, xid2... counting
 * handle. This format is chosen to simplify distribution of handles
 * across buckets (natural distribution vs. hash-one as in shared vars).
 */

typedef struct _SpItem {
    int refcnt;            /* Number of threads operating on the item */
    SpBucket *bucket;      /* Bucket where this item is stored */
    Tcl_HashEntry *hentry; /* Hash table entry where this item is stored */
} SpItem;

/*
 * Structure representing a mutex.
 */

typedef struct _SpMutex {
    int refcnt;            /* Number of threads operating on the mutex */
    SpBucket *bucket;      /* Bucket where mutex is stored */
    Tcl_HashEntry *hentry; /* Hash table entry where mutex is stored */
    /* --- */
    char type;             /* Type of the mutex */
    Sp_AnyMutex *lock;     /* Exclusive, recursive or read/write mutex */
} SpMutex;

/*
 * Structure representing a condition variable.
 */

typedef struct _SpCondv {
    int refcnt;            /* Number of threads operating on the variable */
    SpBucket *bucket;      /* Bucket where this variable is stored */
    Tcl_HashEntry *hentry; /* Hash table entry where variable is stored */
    /* --- */
    SpMutex *mutex;        /* Set when waiting on the variable  */
    Tcl_Condition cond;    /* The condition variable itself */
} SpCondv;

/*
 * This global data is used to map opaque Tcl-level names
 * to pointers of their corresponding synchronization objects.
 */

static int        initOnce;    /* Flag for initializing tables below */
static Tcl_Mutex  initMutex;   /* Controls initialization of primitives */
static SpBucket  muxBuckets[NUMSPBUCKETS];  /* Maps mutex names/handles */
static SpBucket  varBuckets[NUMSPBUCKETS];  /* Maps condition variable
                                             * names/handles */

/*
 * Functions implementing Tcl commands
 */

static Tcl_ObjCmdProc ThreadMutexObjCmd;
static Tcl_ObjCmdProc ThreadRWMutexObjCmd;
static Tcl_ObjCmdProc ThreadCondObjCmd;
static Tcl_ObjCmdProc ThreadEvalObjCmd;

/*
 * Forward declaration of functions used only within this file
 */

static int       SpMutexLock       (SpMutex *);
static int       SpMutexUnlock     (SpMutex *);
static int       SpMutexFinalize   (SpMutex *);

static int       SpCondvWait       (SpCondv *, SpMutex *, int);
static void      SpCondvNotify     (SpCondv *);
static int       SpCondvFinalize   (SpCondv *);

static void      AddAnyItem        (int, const char *, size_t, SpItem *);
static SpItem*   GetAnyItem        (int, const char *, size_t);
static void      PutAnyItem        (SpItem *);
static SpItem *  RemoveAnyItem     (int, const char*, size_t);

static int       RemoveMutex       (const char *, size_t);
static int       RemoveCondv       (const char *, size_t);

static Tcl_Obj*  GetName           (int, void *);
static SpBucket* GetBucket         (int, const char *, size_t);

static int       AnyMutexIsLocked  (Sp_AnyMutex *mPtr, Tcl_ThreadId);

/*
 * Function-like macros for some frequently used calls
 */

#define AddMutex(a,b,c)  AddAnyItem(SP_MUTEX, (a), (b), (SpItem*)(c))
#define GetMutex(a,b)    (SpMutex*)GetAnyItem(SP_MUTEX, (a), (b))
#define PutMutex(a)      PutAnyItem((SpItem*)(a))

#define AddCondv(a,b,c)  AddAnyItem(SP_CONDV, (a), (b), (SpItem*)(c))
#define GetCondv(a,b)    (SpCondv*)GetAnyItem(SP_CONDV, (a), (b))
#define PutCondv(a)      PutAnyItem((SpItem*)(a))

#define IsExclusive(a)   ((a)->type == EMUTEXID)
#define IsRecursive(a)   ((a)->type == RMUTEXID)
#define IsReadWrite(a)   ((a)->type == WMUTEXID)

/*
 * This macro produces a hash-value for table-lookups given a handle
 * and its length. It is implemented as macro just for speed.
 * It is actually a trivial thing because the handles are simple
 * counting values with a small three-letter prefix.
 */

#define GetHash(a,b) (atoi((a)+((b) < 4 ? 0 : 3)) % NUMSPBUCKETS)


/*
 *----------------------------------------------------------------------
 *
 * ThreadMutexObjCmd --
 *
 *    This procedure is invoked to process "thread::mutex" Tcl command.
 *    See the user documentation for details on what it does.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadMutexObjCmd(dummy, interp, objc, objv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int opt, ret;
    size_t nameLen;
    const char *mutexName;
    char type;
    SpMutex *mutexPtr;

    static const char *cmdOpts[] = {
        "create", "destroy", "lock", "unlock", NULL
    };
    enum options {
        m_CREATE, m_DESTROY, m_LOCK, m_UNLOCK
    };

    /*
     * Syntax:
     *
     *     thread::mutex create ?-recursive?
     *     thread::mutex destroy <mutexHandle>
     *     thread::mutex lock <mutexHandle>
     *     thread::mutex unlock <mutexHandle>
     */

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }
    ret = Tcl_GetIndexFromObjStruct(interp, objv[1], cmdOpts, sizeof(char *), "option", 0, &opt);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Cover the "create" option first. It needs no existing handle.
     */

    if (opt == (int)m_CREATE) {
        Tcl_Obj *nameObj;
        const char *arg;

        /*
         * Parse out which type of mutex to create
         */

        if (objc == 2) {
            type = EMUTEXID;
        } else if (objc > 3) {
            Tcl_WrongNumArgs(interp, 2, objv, "?-recursive?");
            return TCL_ERROR;
        } else {
            arg = Tcl_GetString(objv[2]);
            if (OPT_CMP(arg, "-recursive")) {
                type = RMUTEXID;
            } else {
                Tcl_WrongNumArgs(interp, 2, objv, "?-recursive?");
                return TCL_ERROR;
            }
        }

        /*
         * Create the requested mutex
         */

        mutexPtr = (SpMutex*)ckalloc(sizeof(SpMutex));
        mutexPtr->type   = type;
        mutexPtr->bucket = NULL;
        mutexPtr->hentry = NULL;
        mutexPtr->lock   = NULL; /* Will be auto-initialized */

        /*
         * Generate Tcl name for this mutex
         */

        nameObj = GetName(mutexPtr->type, (void*)mutexPtr);
        mutexName = Tcl_GetString(nameObj);
        nameLen = nameObj->length;
        AddMutex(mutexName, nameLen, mutexPtr);
        Tcl_SetObjResult(interp, nameObj);
        return TCL_OK;
    }

    /*
     * All other options require a valid name.
     */

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "mutexHandle");
        return TCL_ERROR;
    }

    mutexName = Tcl_GetString(objv[2]);
    nameLen = objv[2]->length;

    /*
     * Try mutex destroy
     */

    if (opt == (int)m_DESTROY) {
        ret = RemoveMutex(mutexName, nameLen);
        if (ret <= 0) {
            if (ret == -1) {
            notfound:
                Tcl_AppendResult(interp, "no such mutex \"", mutexName,
                                 "\"", NULL);
                return TCL_ERROR;
            } else {
                Tcl_AppendResult(interp, "mutex is in use", NULL);
                return TCL_ERROR;
            }
        }
        return TCL_OK;
    }

    /*
     * Try all other options
     */

    mutexPtr = GetMutex(mutexName, nameLen);
    if (mutexPtr == NULL) {
        goto notfound;
    }
    if (!IsExclusive(mutexPtr) && !IsRecursive(mutexPtr)) {
        PutMutex(mutexPtr);
        Tcl_AppendResult(interp, "wrong mutex type, must be either"
                         " exclusive or recursive", NULL);
        return TCL_ERROR;
    }

    switch ((enum options)opt) {
    case m_LOCK:
        if (!SpMutexLock(mutexPtr)) {
            PutMutex(mutexPtr);
            Tcl_AppendResult(interp, "locking the same exclusive mutex "
                             "twice from the same thread", NULL);
            return TCL_ERROR;
        }
        break;
    case m_UNLOCK:
        if (!SpMutexUnlock(mutexPtr)) {
            PutMutex(mutexPtr);
            Tcl_AppendResult(interp, "mutex is not locked", NULL);
            return TCL_ERROR;
        }
        break;
    default:
        break;
    }

    PutMutex(mutexPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadRwMutexObjCmd --
 *
 *    This procedure is invoked to process "thread::rwmutex" Tcl command.
 *    See the user documentation for details on what it does.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadRWMutexObjCmd(dummy, interp, objc, objv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int opt, ret;
    size_t nameLen;
    const char *mutexName;
    SpMutex *mutexPtr;
    Sp_ReadWriteMutex *rwPtr;
    Sp_AnyMutex **lockPtr;

    static const char *cmdOpts[] = {
        "create", "destroy", "rlock", "wlock", "unlock", NULL
    };
    enum options {
        w_CREATE, w_DESTROY, w_RLOCK, w_WLOCK, w_UNLOCK
    };

    /*
     * Syntax:
     *
     *     thread::rwmutex create
     *     thread::rwmutex destroy <mutexHandle>
     *     thread::rwmutex rlock <mutexHandle>
     *     thread::rwmutex wlock <mutexHandle>
     *     thread::rwmutex unlock <mutexHandle>
     */

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }
    ret = Tcl_GetIndexFromObjStruct(interp, objv[1], cmdOpts, sizeof(char *), "option", 0, &opt);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Cover the "create" option first, since it needs no existing name.
     */

    if (opt == (int)w_CREATE) {
        Tcl_Obj *nameObj;
        if (objc > 2) {
            Tcl_WrongNumArgs(interp, 1, objv, "create");
            return TCL_ERROR;
        }
        mutexPtr = (SpMutex*)ckalloc(sizeof(SpMutex));
        mutexPtr->type   = WMUTEXID;
        mutexPtr->refcnt = 0;
        mutexPtr->bucket = NULL;
        mutexPtr->hentry = NULL;
        mutexPtr->lock   = NULL; /* Will be auto-initialized */

        nameObj = GetName(mutexPtr->type, (void*)mutexPtr);
        mutexName = Tcl_GetString(nameObj);
        AddMutex(mutexName, nameObj->length, mutexPtr);
        Tcl_SetObjResult(interp, nameObj);
        return TCL_OK;
    }

    /*
     * All other options require a valid name.
     */

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "mutexHandle");
        return TCL_ERROR;
    }

    mutexName = Tcl_GetString(objv[2]);
    nameLen = objv[2]->length;

    /*
     * Try mutex destroy
     */

    if (opt == (int)w_DESTROY) {
        ret = RemoveMutex(mutexName, nameLen);
        if (ret <= 0) {
            if (ret == -1) {
            notfound:
                Tcl_AppendResult(interp, "no such mutex \"", mutexName,
                                 "\"", NULL);
                return TCL_ERROR;
            } else {
                Tcl_AppendResult(interp, "mutex is in use", NULL);
                return TCL_ERROR;
            }
        }
        return TCL_OK;
    }

    /*
     * Try all other options
     */

    mutexPtr = GetMutex(mutexName, nameLen);
    if (mutexPtr == NULL) {
        goto notfound;
    }
    if (!IsReadWrite(mutexPtr)) {
        PutMutex(mutexPtr);
        Tcl_AppendResult(interp, "wrong mutex type, must be readwrite", NULL);
        return TCL_ERROR;
    }

    lockPtr = &mutexPtr->lock;
    rwPtr = (Sp_ReadWriteMutex*) lockPtr;

    switch ((enum options)opt) {
    case w_RLOCK:
        if (!Sp_ReadWriteMutexRLock(rwPtr)) {
            PutMutex(mutexPtr);
            Tcl_AppendResult(interp, "read-locking already write-locked mutex ",
                             "from the same thread", NULL);
            return TCL_ERROR;
        }
        break;
    case w_WLOCK:
        if (!Sp_ReadWriteMutexWLock(rwPtr)) {
            PutMutex(mutexPtr);
            Tcl_AppendResult(interp, "write-locking the same read-write "
                             "mutex twice from the same thread", NULL);
            return TCL_ERROR;
        }
        break;
    case w_UNLOCK:
        if (!Sp_ReadWriteMutexUnlock(rwPtr)) {
            PutMutex(mutexPtr);
            Tcl_AppendResult(interp, "mutex is not locked", NULL);
            return TCL_ERROR;
        }
        break;
    default:
        break;
    }

    PutMutex(mutexPtr);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * ThreadCondObjCmd --
 *
 *    This procedure is invoked to process "thread::cond" Tcl command.
 *    See the user documentation for details on what it does.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadCondObjCmd(dummy, interp, objc, objv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int opt, ret, timeMsec = 0;
    size_t nameLen;
    const char *condvName, *mutexName;
    SpMutex *mutexPtr;
    SpCondv *condvPtr;

    static const char *cmdOpts[] = {
        "create", "destroy", "notify", "wait", NULL
    };
    enum options {
        c_CREATE, c_DESTROY, c_NOTIFY, c_WAIT
    };

    /*
     * Syntax:
     *
     *    thread::cond create
     *    thread::cond destroy <condHandle>
     *    thread::cond notify <condHandle>
     *    thread::cond wait <condHandle> <mutexHandle> ?timeout?
     */

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option ?args?");
        return TCL_ERROR;
    }
    ret = Tcl_GetIndexFromObjStruct(interp, objv[1], cmdOpts, sizeof(char *), "option", 0, &opt);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     * Cover the "create" option since it needs no existing name.
     */

    if (opt == (int)c_CREATE) {
        Tcl_Obj *nameObj;
        if (objc > 2) {
            Tcl_WrongNumArgs(interp, 1, objv, "create");
            return TCL_ERROR;
        }
        condvPtr = (SpCondv*)ckalloc(sizeof(SpCondv));
        condvPtr->refcnt = 0;
        condvPtr->bucket = NULL;
        condvPtr->hentry = NULL;
        condvPtr->mutex  = NULL;
        condvPtr->cond   = NULL; /* Will be auto-initialized */

        nameObj = GetName(CONDVID, (void*)condvPtr);
        condvName = Tcl_GetString(nameObj);
        AddCondv(condvName, nameObj->length, condvPtr);
        Tcl_SetObjResult(interp, nameObj);
        return TCL_OK;
    }

    /*
     * All others require at least a valid handle.
     */

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "condHandle ?args?");
        return TCL_ERROR;
    }

    condvName = Tcl_GetString(objv[2]);
    nameLen = objv[2]->length;

    /*
     * Try variable destroy.
     */

    if (opt == (int)c_DESTROY) {
        ret = RemoveCondv(condvName, nameLen);
        if (ret <= 0) {
            if (ret == -1) {
            notfound:
                Tcl_AppendResult(interp, "no such condition variable \"",
                                 condvName, "\"", NULL);
                return TCL_ERROR;
            } else {
                Tcl_AppendResult(interp, "condition variable is in use", NULL);
                return TCL_ERROR;
            }
        }
        return TCL_OK;
    }

    /*
     * Try all other options
     */

    condvPtr = GetCondv(condvName, nameLen);
    if (condvPtr == NULL) {
        goto notfound;
    }

    switch ((enum options)opt) {
    case c_WAIT:

        /*
         * May improve the Tcl_ConditionWait() to report timeouts so we can
         * inform script programmer about this interesting fact. I think
         * there is still a place for something like Tcl_ConditionWaitEx()
         * or similar in the core.
         */

        if (objc < 4 || objc > 5) {
            PutCondv(condvPtr);
            Tcl_WrongNumArgs(interp, 2, objv, "condHandle mutexHandle ?timeout?");
            return TCL_ERROR;
        }
        if (objc == 5) {
            if (Tcl_GetIntFromObj(interp, objv[4], &timeMsec) != TCL_OK) {
                PutCondv(condvPtr);
                return TCL_ERROR;
            }
        }
        mutexName = Tcl_GetString(objv[3]);
        mutexPtr  = GetMutex(mutexName, objv[3]->length);
        if (mutexPtr == NULL) {
            PutCondv(condvPtr);
            Tcl_AppendResult(interp, "no such mutex \"",mutexName,"\"", NULL);
            return TCL_ERROR;
        }
        if (!IsExclusive(mutexPtr)
            || SpCondvWait(condvPtr, mutexPtr, timeMsec) == 0) {
            PutCondv(condvPtr);
            PutMutex(mutexPtr);
            Tcl_AppendResult(interp, "mutex not locked or wrong type", NULL);
            return TCL_ERROR;
        }
        PutMutex(mutexPtr);
        break;
    case c_NOTIFY:
        SpCondvNotify(condvPtr);
        break;
    default:
        break;
    }

    PutCondv(condvPtr);

    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * ThreadEvalObjCmd --
 *
 *    This procedure is invoked to process "thread::eval" Tcl command.
 *    See the user documentation for details on what it does.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    See the user documentation.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadEvalObjCmd(dummy, interp, objc, objv)
    ClientData dummy;                   /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int ret, optx, internal;
    const char *mutexName;
    Tcl_Obj *scriptObj;
    SpMutex *mutexPtr = NULL;
    static Sp_RecursiveMutex evalMutex;

    /*
     * Syntax:
     *
     *     thread::eval ?-lock <mutexHandle>? arg ?arg ...?
     */

    if (objc < 2) {
      syntax:
        Tcl_WrongNumArgs(interp, 1, objv,
                         "?-lock <mutexHandle>? arg ?arg...?");
        return TCL_ERROR;
    }

    /*
     * Find out wether to use the internal (recursive) mutex
     * or external mutex given on the command line, and lock
     * the corresponding mutex immediately.
     *
     * We are using recursive internal mutex so we can easily
     * support the recursion w/o danger of deadlocking. If
     * however, user gives us an exclusive mutex, we will
     * throw error on attempt to recursively call us.
     */

    if (OPT_CMP(Tcl_GetString(objv[1]), "-lock") == 0) {
        internal = 1;
        optx = 1;
        Sp_RecursiveMutexLock(&evalMutex);
    } else {
        internal = 0;
        optx = 3;
        if ((objc - optx) < 1) {
            goto syntax;
        }
        mutexName = Tcl_GetString(objv[2]);
        mutexPtr  = GetMutex(mutexName, objv[2]->length);
        if (mutexPtr == NULL) {
            Tcl_AppendResult(interp, "no such mutex \"",mutexName,"\"", NULL);
            return TCL_ERROR;
        }
        if (IsReadWrite(mutexPtr)) {
            Tcl_AppendResult(interp, "wrong mutex type, must be exclusive "
                             "or recursive", NULL);
            return TCL_ERROR;
        }
        if (!SpMutexLock(mutexPtr)) {
            Tcl_AppendResult(interp, "locking the same exclusive mutex "
                             "twice from the same thread", NULL);
            return TCL_ERROR;
        }
    }

    objc -= optx;

    /*
     * Evaluate passed arguments as Tcl script. Note that
     * Tcl_EvalObjEx throws away the passed object by
     * doing an decrement reference count on it. This also
     * means we need not build object bytecode rep.
     */

    if (objc == 1) {
        scriptObj = Tcl_DuplicateObj(objv[optx]);
    } else {
        scriptObj = Tcl_ConcatObj(objc, objv + optx);
    }

    Tcl_IncrRefCount(scriptObj);
    ret = Tcl_EvalObjEx(interp, scriptObj, TCL_EVAL_DIRECT);
    Tcl_DecrRefCount(scriptObj);

    if (ret == TCL_ERROR) {
        char msg[32 + TCL_INTEGER_SPACE];
        /* Next line generates a Deprecation warning when compiled with Tcl 8.6.
         * See Tcl bug #3562640 */
        sprintf(msg, "\n    (\"eval\" body line %d)", Tcl_GetErrorLine(interp));
        Tcl_AddErrorInfo(interp, msg);
    }

    /*
     * Unlock the mutex.
     */

    if (internal) {
        Sp_RecursiveMutexUnlock(&evalMutex);
    } else {
        SpMutexUnlock(mutexPtr);
    }

    return ret;
}

/*
 *----------------------------------------------------------------------
 *
 * GetName --
 *
 *      Construct a Tcl name for the given sync primitive.
 *      The name is in the simple counted form: XidN
 *      where "X" designates the type of the primitive
 *      and "N" is a increasing integer.
 *
 * Results:
 *      Tcl string object with the constructed name.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj*
GetName(int type, void *addrPtr)
{
    char name[32];
    unsigned int id;
    static unsigned int idcounter;

    Tcl_MutexLock(&initMutex);
    id = idcounter++;
    Tcl_MutexUnlock(&initMutex);

    sprintf(name, "%cid%d", type, id);

    return Tcl_NewStringObj(name, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * GetBucket --
 *
 *      Returns the bucket for the given name.
 *
 * Results:
 *      Pointer to the bucket.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static SpBucket*
GetBucket(int type, const char *name, size_t len)
{
    switch (type) {
    case SP_MUTEX: return &muxBuckets[GetHash(name, len)];
    case SP_CONDV: return &varBuckets[GetHash(name, len)];
    }

    return NULL; /* Never reached */
}

/*
 *----------------------------------------------------------------------
 *
 * GetAnyItem --
 *
 *      Retrieves the item structure from it's corresponding bucket.
 *
 * Results:
 *      Item pointer or NULL
 *
 * Side effects:
 *      Increment the item's ref count preventing it's deletion.
 *
 *----------------------------------------------------------------------
 */

static SpItem*
GetAnyItem(int type, const char *name, size_t len)
{
    SpItem *itemPtr = NULL;
    SpBucket *bucketPtr = GetBucket(type, name, len);
    Tcl_HashEntry *hashEntryPtr = NULL;

    Tcl_MutexLock(&bucketPtr->lock);
    hashEntryPtr = Tcl_FindHashEntry(&bucketPtr->handles, name);
    if (hashEntryPtr != (Tcl_HashEntry*)NULL) {
        itemPtr = (SpItem*)Tcl_GetHashValue(hashEntryPtr);
        itemPtr->refcnt++;
    }
    Tcl_MutexUnlock(&bucketPtr->lock);

    return itemPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * PutAnyItem --
 *
 *      Current thread detaches from the item.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Decrement item's ref count allowing for it's deletion
 *      and signalize any threads waiting to delete the item.
 *
 *----------------------------------------------------------------------
 */

static void
PutAnyItem(SpItem *itemPtr)
{
    Tcl_MutexLock(&itemPtr->bucket->lock);
    itemPtr->refcnt--;
    Tcl_ConditionNotify(&itemPtr->bucket->cond);
    Tcl_MutexUnlock(&itemPtr->bucket->lock);
}

/*
 *----------------------------------------------------------------------
 *
 * AddAnyItem --
 *
 *      Puts any item in the corresponding bucket.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static void
AddAnyItem(int type, const char *handle, size_t len, SpItem *itemPtr)
{
    int new;
    SpBucket *bucketPtr = GetBucket(type, handle, len);
    Tcl_HashEntry *hashEntryPtr;

    Tcl_MutexLock(&bucketPtr->lock);

    hashEntryPtr = Tcl_CreateHashEntry(&bucketPtr->handles, handle, &new);
    Tcl_SetHashValue(hashEntryPtr, (ClientData)itemPtr);

    itemPtr->refcnt = 0;
    itemPtr->bucket = bucketPtr;
    itemPtr->hentry = hashEntryPtr;

    Tcl_MutexUnlock(&bucketPtr->lock);
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveAnyItem --
 *
 *      Removes the item from it's bucket.
 *
 * Results:
 *      Item's pointer or NULL if none found.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static SpItem *
RemoveAnyItem(int type, const char *name, size_t len)
{
    SpItem *itemPtr = NULL;
    SpBucket *bucketPtr = GetBucket(type, name, len);
    Tcl_HashEntry *hashEntryPtr = NULL;

    Tcl_MutexLock(&bucketPtr->lock);
    hashEntryPtr = Tcl_FindHashEntry(&bucketPtr->handles, name);
    if (hashEntryPtr == (Tcl_HashEntry*)NULL) {
        Tcl_MutexUnlock(&bucketPtr->lock);
        return NULL;
    }
    itemPtr = (SpItem*)Tcl_GetHashValue(hashEntryPtr);
    Tcl_DeleteHashEntry(hashEntryPtr);
    while (itemPtr->refcnt > 0) {
        Tcl_ConditionWait(&bucketPtr->cond, &bucketPtr->lock, NULL);
    }
    Tcl_MutexUnlock(&bucketPtr->lock);

    return itemPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveMutex --
 *
 *      Removes the mutex from it's bucket and finalizes it.
 *
 * Results:
 *      1 - mutex is finalized and removed
 *      0 - mutex is not finalized
 +     -1 - mutex is not found
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
RemoveMutex(const char *name, size_t len)
{
    SpMutex *mutexPtr = GetMutex(name, len);
    if (mutexPtr == NULL) {
        return -1;
    }
    if (!SpMutexFinalize(mutexPtr)) {
        PutMutex(mutexPtr);
        return 0;
    }
    PutMutex(mutexPtr);
    RemoveAnyItem(SP_MUTEX, name, len);
    ckfree((char*)mutexPtr);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveCondv --
 *
 *      Removes the cond variable from it's bucket and finalizes it.
 *
 * Results:
 *      1 - variable is finalized and removed
 *      0 - variable is not finalized
 +     -1 - variable is not found
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
RemoveCondv(const char *name, size_t len)
{
    SpCondv *condvPtr = GetCondv(name, len);
    if (condvPtr == NULL) {
        return -1;
    }
    if (!SpCondvFinalize(condvPtr)) {
        PutCondv(condvPtr);
        return 0;
    }
    PutCondv(condvPtr);
    RemoveAnyItem(SP_CONDV, name, len);
    ckfree((char*)condvPtr);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_Init --
 *
 *      Create commands in current interpreter.
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side effects:
 *      Initializes shared hash table for storing sync primitive
 *      handles and pointers.
 *
 *----------------------------------------------------------------------
 */

int
Sp_Init (interp)
    Tcl_Interp *interp;                 /* Interp where to create cmds */
{
    SpBucket *bucketPtr;

    if (!initOnce) {
        Tcl_MutexLock(&initMutex);
        if (!initOnce) {
            int ii;
            for (ii = 0; ii < NUMSPBUCKETS; ii++) {
                bucketPtr = &muxBuckets[ii];
                memset(bucketPtr, 0, sizeof(SpBucket));
                Tcl_InitHashTable(&bucketPtr->handles, TCL_STRING_KEYS);
            }
            for (ii = 0; ii < NUMSPBUCKETS; ii++) {
                bucketPtr = &varBuckets[ii];
                memset(bucketPtr, 0, sizeof(SpBucket));
                Tcl_InitHashTable(&bucketPtr->handles, TCL_STRING_KEYS);
            }
            initOnce = 1;
        }
        Tcl_MutexUnlock(&initMutex);
    }

    TCL_CMD(interp, THREAD_CMD_PREFIX"::mutex",   ThreadMutexObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"::rwmutex", ThreadRWMutexObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"::cond",    ThreadCondObjCmd);
    TCL_CMD(interp, THREAD_CMD_PREFIX"::eval",    ThreadEvalObjCmd);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SpMutexLock --
 *
 *      Locks the typed mutex.
 *
 * Results:
 *      1 - mutex is locked
 *      0 - mutex is not locked (pending deadlock?)
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
SpMutexLock(SpMutex *mutexPtr)
{
    Sp_AnyMutex **lockPtr = &mutexPtr->lock;

    switch (mutexPtr->type) {
    case EMUTEXID:
        return Sp_ExclusiveMutexLock((Sp_ExclusiveMutex*)lockPtr);
        break;
    case RMUTEXID:
        return Sp_RecursiveMutexLock((Sp_RecursiveMutex*)lockPtr);
        break;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * SpMutexUnlock --
 *
 *      Unlocks the typed mutex.
 *
 * Results:
 *      1 - mutex is unlocked
 *      0 - mutex was not locked
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
SpMutexUnlock(SpMutex *mutexPtr)
{
    Sp_AnyMutex **lockPtr = &mutexPtr->lock;

    switch (mutexPtr->type) {
    case EMUTEXID:
        return Sp_ExclusiveMutexUnlock((Sp_ExclusiveMutex*)lockPtr);
        break;
    case RMUTEXID:
        return Sp_RecursiveMutexUnlock((Sp_RecursiveMutex*)lockPtr);
        break;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * SpMutexFinalize --
 *
 *      Finalizes the typed mutex. This should never be called without
 *      some external mutex protection.
 *
 * Results:
 *      1 - mutex is finalized
 *      0 - mutex is still in use
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
SpMutexFinalize(SpMutex *mutexPtr)
{
    Sp_AnyMutex **lockPtr = &mutexPtr->lock;

    if (AnyMutexIsLocked((Sp_AnyMutex*)mutexPtr->lock, (Tcl_ThreadId)0)) {
        return 0;
    }

    /*
     * At this point, the mutex could be locked again, hence it
     * is important never to call this function unprotected.
     */

    switch (mutexPtr->type) {
    case EMUTEXID:
        Sp_ExclusiveMutexFinalize((Sp_ExclusiveMutex*)lockPtr);
        break;
    case RMUTEXID:
        Sp_RecursiveMutexFinalize((Sp_RecursiveMutex*)lockPtr);
        break;
    case WMUTEXID:
        Sp_ReadWriteMutexFinalize((Sp_ReadWriteMutex*)lockPtr);
        break;
    default:
        break;
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SpCondvWait --
 *
 *      Waits on the condition variable.
 *
 * Results:
 *      1 - wait ok
 *      0 - not waited as mutex is not locked in the same thread
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
SpCondvWait(SpCondv *condvPtr, SpMutex *mutexPtr, int msec)
{
    Sp_AnyMutex **lock = &mutexPtr->lock;
    Sp_ExclusiveMutex_ *emPtr = *(Sp_ExclusiveMutex_**)lock;
    Tcl_Time waitTime, *wt = NULL;
    Tcl_ThreadId threadId = Tcl_GetCurrentThread();

    if (msec > 0) {
        wt = &waitTime;
        wt->sec  = (msec/1000);
        wt->usec = (msec%1000) * 1000;
    }
    if (!AnyMutexIsLocked((Sp_AnyMutex*)mutexPtr->lock, threadId)) {
        return 0; /* Mutex not locked by the current thread */
    }

    /*
     * It is safe to operate on mutex struct because caller
     * is holding the emPtr->mutex locked before we enter
     * the Tcl_ConditionWait and after we return out of it.
     */

    condvPtr->mutex = mutexPtr;

    emPtr->owner = (Tcl_ThreadId)0;
    emPtr->lockcount = 0;

    Tcl_ConditionWait(&condvPtr->cond, &emPtr->mutex, wt);

    emPtr->owner = threadId;
    emPtr->lockcount = 1;

    condvPtr->mutex = NULL;

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * SpCondvNotify --
 *
 *      Signalizes the condition variable.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static void
SpCondvNotify(SpCondv *condvPtr)
{
    if (condvPtr->cond) {
        Tcl_ConditionNotify(&condvPtr->cond);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SpCondvFinalize --
 *
 *      Finalizes the condition variable.
 *
 * Results:
 *      1 - variable is finalized
 *      0 - variable is in use
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static int
SpCondvFinalize(SpCondv *condvPtr)
{
    if (condvPtr->mutex != NULL) {
        return 0; /* Somebody is waiting on the variable */
    }

    if (condvPtr->cond) {
        Tcl_ConditionFinalize(&condvPtr->cond);
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ExclusiveMutexLock --
 *
 *      Locks the exclusive mutex.
 *
 * Results:
 *      1 - mutex is locked
 *      0 - mutex is not locked; same thread tries to locks twice
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_ExclusiveMutexLock(Sp_ExclusiveMutex *muxPtr)
{
    Sp_ExclusiveMutex_ *emPtr;
    Tcl_ThreadId thisThread = Tcl_GetCurrentThread();

    /*
     * Allocate the mutex structure on first access
     */

    if (*muxPtr == (Sp_ExclusiveMutex_*)0) {
        Tcl_MutexLock(&initMutex);
        if (*muxPtr == (Sp_ExclusiveMutex_*)0) {
            *muxPtr = (Sp_ExclusiveMutex_*)
                ckalloc(sizeof(Sp_ExclusiveMutex_));
            memset(*muxPtr, 0, sizeof(Sp_ExclusiveMutex_));
        }
        Tcl_MutexUnlock(&initMutex);
    }

    /*
     * Try locking if not currently locked by anybody.
     */

    emPtr = *(Sp_ExclusiveMutex_**)muxPtr;
    Tcl_MutexLock(&emPtr->lock);
    if (emPtr->lockcount && emPtr->owner == thisThread) {
        Tcl_MutexUnlock(&emPtr->lock);
        return 0; /* Already locked by the same thread */
    }
    Tcl_MutexUnlock(&emPtr->lock);

    /*
     * Many threads can come to this point.
     * Only one will succeed locking the
     * mutex. Others will block...
     */

    Tcl_MutexLock(&emPtr->mutex);

    Tcl_MutexLock(&emPtr->lock);
    emPtr->owner = thisThread;
    emPtr->lockcount = 1;
    Tcl_MutexUnlock(&emPtr->lock);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ExclusiveMutexIsLocked --
 *
 *      Checks wether the mutex is locked or not.
 *
 * Results:
 *      1 - mutex is locked
 *      0 - mutex is not locked
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_ExclusiveMutexIsLocked(Sp_ExclusiveMutex *muxPtr)
{
    return AnyMutexIsLocked((Sp_AnyMutex*)*muxPtr, (Tcl_ThreadId)0);
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ExclusiveMutexUnlock --
 *
 *      Unlock the exclusive mutex.
 *
 * Results:
 *      1 - mutex is unlocked
 ?      0 - mutex was never locked
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_ExclusiveMutexUnlock(Sp_ExclusiveMutex *muxPtr)
{
    Sp_ExclusiveMutex_ *emPtr;

    if (*muxPtr == (Sp_ExclusiveMutex_*)0) {
        return 0; /* Never locked before */
    }

    emPtr = *(Sp_ExclusiveMutex_**)muxPtr;

    Tcl_MutexLock(&emPtr->lock);
    if (emPtr->lockcount == 0) {
        Tcl_MutexUnlock(&emPtr->lock);
        return 0; /* Not locked */
    }
    emPtr->owner = (Tcl_ThreadId)0;
    emPtr->lockcount = 0;
    Tcl_MutexUnlock(&emPtr->lock);

    /*
     * Only one thread should be able
     * to come to this point and unlock...
     */

    Tcl_MutexUnlock(&emPtr->mutex);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ExclusiveMutexFinalize --
 *
 *      Finalize the exclusive mutex. It is not safe for two or
 *      more threads to finalize the mutex at the same time.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Mutex is destroyed.
 *
 *----------------------------------------------------------------------
 */

void
Sp_ExclusiveMutexFinalize(Sp_ExclusiveMutex *muxPtr)
{
    if (*muxPtr != (Sp_ExclusiveMutex_*)0) {
        Sp_ExclusiveMutex_ *emPtr = *(Sp_ExclusiveMutex_**)muxPtr;
        if (emPtr->lock) {
            Tcl_MutexFinalize(&emPtr->lock);
        }
        if (emPtr->mutex) {
            Tcl_MutexFinalize(&emPtr->mutex);
        }
        ckfree((char*)*muxPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_RecursiveMutexLock --
 *
 *      Locks the recursive mutex.
 *
 * Results:
 *      1 - mutex is locked (as it always should be)
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_RecursiveMutexLock(Sp_RecursiveMutex *muxPtr)
{
    Sp_RecursiveMutex_ *rmPtr;
    Tcl_ThreadId thisThread = Tcl_GetCurrentThread();

    /*
     * Allocate the mutex structure on first access
     */

    if (*muxPtr == (Sp_RecursiveMutex_*)0) {
        Tcl_MutexLock(&initMutex);
        if (*muxPtr == (Sp_RecursiveMutex_*)0) {
            *muxPtr = (Sp_RecursiveMutex_*)
                ckalloc(sizeof(Sp_RecursiveMutex_));
            memset(*muxPtr, 0, sizeof(Sp_RecursiveMutex_));
        }
        Tcl_MutexUnlock(&initMutex);
    }

    rmPtr = *(Sp_RecursiveMutex_**)muxPtr;
    Tcl_MutexLock(&rmPtr->lock);

    if (rmPtr->owner == thisThread) {
        /*
         * We are already holding the mutex
         * so just count one more lock.
         */
        rmPtr->lockcount++;
    } else {
        if (rmPtr->owner == (Tcl_ThreadId)0) {
            /*
             * Nobody holds the mutex, we do now.
             */
            rmPtr->owner = thisThread;
            rmPtr->lockcount = 1;
        } else {
            /*
             * Somebody else holds the mutex; wait.
             */
            while (1) {
                Tcl_ConditionWait(&rmPtr->cond, &rmPtr->lock, NULL);
                if (rmPtr->owner == (Tcl_ThreadId)0) {
                    rmPtr->owner = thisThread;
                    rmPtr->lockcount = 1;
                    break;
                }
            }
        }
    }

    Tcl_MutexUnlock(&rmPtr->lock);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_RecursiveMutexIsLocked --
 *
 *      Checks wether the mutex is locked or not.
 *
 * Results:
 *      1 - mutex is locked
 *      0 - mutex is not locked
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_RecursiveMutexIsLocked(Sp_RecursiveMutex *muxPtr)
{
    return AnyMutexIsLocked((Sp_AnyMutex*)*muxPtr, (Tcl_ThreadId)0);
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_RecursiveMutexUnlock --
 *
 *      Unlock the recursive mutex.
 *
 * Results:
 *      1 - mutex unlocked
 *      0 - mutex never locked
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_RecursiveMutexUnlock(Sp_RecursiveMutex *muxPtr)
{
    Sp_RecursiveMutex_ *rmPtr;

    if (*muxPtr == (Sp_RecursiveMutex_*)0) {
        return 0; /* Never locked before */
    }

    rmPtr = *(Sp_RecursiveMutex_**)muxPtr;
    Tcl_MutexLock(&rmPtr->lock);
    if (rmPtr->lockcount == 0) {
        Tcl_MutexUnlock(&rmPtr->lock);
        return 0; /* Not locked now */
    }
    if (--rmPtr->lockcount <= 0) {
        rmPtr->lockcount = 0;
        rmPtr->owner = (Tcl_ThreadId)0;
        if (rmPtr->cond) {
            Tcl_ConditionNotify(&rmPtr->cond);
        }
    }
    Tcl_MutexUnlock(&rmPtr->lock);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_RecursiveMutexFinalize --
 *
 *      Finalize the recursive mutex. It is not safe for two or
 *      more threads to finalize the mutex at the same time.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Mutex is destroyed.
 *
 *----------------------------------------------------------------------
 */

void
Sp_RecursiveMutexFinalize(Sp_RecursiveMutex *muxPtr)
{
    if (*muxPtr != (Sp_RecursiveMutex_*)0) {
        Sp_RecursiveMutex_ *rmPtr = *(Sp_RecursiveMutex_**)muxPtr;
        if (rmPtr->lock) {
            Tcl_MutexFinalize(&rmPtr->lock);
        }
        if (rmPtr->cond) {
            Tcl_ConditionFinalize(&rmPtr->cond);
        }
        ckfree((char*)*muxPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ReadWriteMutexRLock --
 *
 *      Read-locks the reader/writer mutex.
 *
 * Results:
 *      1 - mutex is locked
 *      0 - mutex is not locked as we already hold the write lock
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_ReadWriteMutexRLock(Sp_ReadWriteMutex *muxPtr)
{
    Sp_ReadWriteMutex_ *rwPtr;
    Tcl_ThreadId thisThread = Tcl_GetCurrentThread();

    /*
     * Allocate the mutex structure on first access
     */

    if (*muxPtr == (Sp_ReadWriteMutex_*)0) {
        Tcl_MutexLock(&initMutex);
        if (*muxPtr == (Sp_ReadWriteMutex_*)0) {
            *muxPtr = (Sp_ReadWriteMutex_*)
                ckalloc(sizeof(Sp_ReadWriteMutex_));
            memset(*muxPtr, 0, sizeof(Sp_ReadWriteMutex_));
        }
        Tcl_MutexUnlock(&initMutex);
    }

    rwPtr = *(Sp_ReadWriteMutex_**)muxPtr;
    Tcl_MutexLock(&rwPtr->lock);
    if (rwPtr->lockcount == -1 && rwPtr->owner == thisThread) {
        Tcl_MutexUnlock(&rwPtr->lock);
        return 0; /* We already hold the write lock */
    }
    while (rwPtr->lockcount < 0) {
        rwPtr->numrd++;
        Tcl_ConditionWait(&rwPtr->rcond, &rwPtr->lock, NULL);
        rwPtr->numrd--;
    }
    rwPtr->lockcount++;
    rwPtr->owner = (Tcl_ThreadId)0; /* Many threads can read-lock */
    Tcl_MutexUnlock(&rwPtr->lock);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ReadWriteMutexWLock --
 *
 *      Write-locks the reader/writer mutex.
 *
 * Results:
 *      1 - mutex is locked
 *      0 - same thread attempts to write-lock the mutex twice
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_ReadWriteMutexWLock(Sp_ReadWriteMutex *muxPtr)
{
    Sp_ReadWriteMutex_ *rwPtr;
    Tcl_ThreadId thisThread = Tcl_GetCurrentThread();

    /*
     * Allocate the mutex structure on first access
     */

    if (*muxPtr == (Sp_ReadWriteMutex_*)0) {
        Tcl_MutexLock(&initMutex);
        if (*muxPtr == (Sp_ReadWriteMutex_*)0) {
            *muxPtr = (Sp_ReadWriteMutex_*)
                ckalloc(sizeof(Sp_ReadWriteMutex_));
            memset(*muxPtr, 0, sizeof(Sp_ReadWriteMutex_));
        }
        Tcl_MutexUnlock(&initMutex);
    }

    rwPtr = *(Sp_ReadWriteMutex_**)muxPtr;
    Tcl_MutexLock(&rwPtr->lock);
    if (rwPtr->owner == thisThread && rwPtr->lockcount == -1) {
        Tcl_MutexUnlock(&rwPtr->lock);
        return 0; /* The same thread attempts to write-lock again */
    }
    while (rwPtr->lockcount != 0) {
        rwPtr->numwr++;
        Tcl_ConditionWait(&rwPtr->wcond, &rwPtr->lock, NULL);
        rwPtr->numwr--;
    }
    rwPtr->lockcount = -1;     /* This designates the sole writer */
    rwPtr->owner = thisThread; /* which is our current thread     */
    Tcl_MutexUnlock(&rwPtr->lock);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ReadWriteMutexIsLocked --
 *
 *      Checks wether the mutex is locked or not.
 *
 * Results:
 *      1 - mutex is locked
 *      0 - mutex is not locked
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Sp_ReadWriteMutexIsLocked(Sp_ReadWriteMutex *muxPtr)
{
    return AnyMutexIsLocked((Sp_AnyMutex*)*muxPtr, (Tcl_ThreadId)0);
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ReadWriteMutexUnlock --
 *
 *      Unlock the reader/writer mutex.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

int
Sp_ReadWriteMutexUnlock(Sp_ReadWriteMutex *muxPtr)
{
    Sp_ReadWriteMutex_ *rwPtr;

    if (*muxPtr == (Sp_ReadWriteMutex_*)0) {
        return 0; /* Never locked before */
    }

    rwPtr = *(Sp_ReadWriteMutex_**)muxPtr;
    Tcl_MutexLock(&rwPtr->lock);
    if (rwPtr->lockcount == 0) {
        Tcl_MutexUnlock(&rwPtr->lock);
        return 0; /* Not locked now */
    }
    if (--rwPtr->lockcount <= 0) {
        rwPtr->lockcount = 0;
        rwPtr->owner = (Tcl_ThreadId)0;
    }
    if (rwPtr->numwr) {
        Tcl_ConditionNotify(&rwPtr->wcond);
    } else if (rwPtr->numrd) {
        Tcl_ConditionNotify(&rwPtr->rcond);
    }

    Tcl_MutexUnlock(&rwPtr->lock);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Sp_ReadWriteMutexFinalize --
 *
 *      Finalize the reader/writer mutex. It is not safe for two or
 *      more threads to finalize the mutex at the same time.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Mutex is destroyed.
 *
 *----------------------------------------------------------------------
 */

void
Sp_ReadWriteMutexFinalize(Sp_ReadWriteMutex *muxPtr)
{
    if (*muxPtr != (Sp_ReadWriteMutex_*)0) {
        Sp_ReadWriteMutex_ *rwPtr = *(Sp_ReadWriteMutex_**)muxPtr;
        if (rwPtr->lock) {
            Tcl_MutexFinalize(&rwPtr->lock);
        }
        if (rwPtr->rcond) {
            Tcl_ConditionFinalize(&rwPtr->rcond);
        }
        if (rwPtr->wcond) {
            Tcl_ConditionFinalize(&rwPtr->wcond);
        }
        ckfree((char*)*muxPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AnyMutexIsLocked --
 *
 *      Checks wether the mutex is locked. If optional threadId
 *      is given (i.e. != 0) it checks if the given thread also
 *      holds the lock.
 *
 * Results:
 *      1 - mutex is locked (optionally by the given thread)
 *      0 - mutex is not locked (optionally by the given thread)
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static int
AnyMutexIsLocked(Sp_AnyMutex *mPtr, Tcl_ThreadId threadId)
{
    int locked = 0;

    if (mPtr != NULL) {
        Tcl_MutexLock(&mPtr->lock);
        locked = mPtr->lockcount != 0;
        if (locked && threadId != (Tcl_ThreadId)0) {
            locked = mPtr->owner == threadId;
        }
        Tcl_MutexUnlock(&mPtr->lock);
    }

    return locked;
}


/* EOF $RCSfile: threadSpCmd.c,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */
