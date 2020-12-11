/*
 * This file implements a family of commands for sharing variables
 * between threads.
 *
 * Initial code is taken from nsd/tclvar.c found in AOLserver 3.+
 * distribution and modified to support Tcl 8.0+ command object interface
 * and internal storage in private shared Tcl objects.
 *
 * Copyright (c) 2002 by Zoran Vasiljevic.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ----------------------------------------------------------------------------
 */

#include "tclThreadInt.h"
#include "threadSvCmd.h"

#include "threadSvListCmd.h"    /* Shared variants of list commands */
#include "threadSvKeylistCmd.h" /* Shared variants of list commands */
#include "psGdbm.h"             /* The gdbm persistent store implementation */
#include "psLmdb.h"             /* The lmdb persistent store implementation */

#define SV_FINALIZE

/*
 * Number of buckets to spread shared arrays into. Each bucket is
 * associated with one mutex so locking a bucket locks all arrays
 * in that bucket as well. The number of buckets should be a prime.
 */

#define NUMBUCKETS 31

/*
 * Number of object containers
 * to allocate in one shot.
 */

#define OBJS_TO_ALLOC_EACH_TIME 100

/*
 * Reference to Tcl object types used in object-copy code.
 * Those are referenced read-only, thus no mutex protection.
 */

static const Tcl_ObjType* booleanObjTypePtr = 0;
static const Tcl_ObjType* byteArrayObjTypePtr = 0;
static const Tcl_ObjType* doubleObjTypePtr = 0;
static const Tcl_ObjType* intObjTypePtr = 0;
static const Tcl_ObjType* wideIntObjTypePtr = 0;
static const Tcl_ObjType* stringObjTypePtr = 0;

/*
 * In order to be fully stub enabled, a small
 * hack is needed to query the tclEmptyStringRep
 * global symbol defined by Tcl. See Sv_Init.
 */

static char *Sv_tclEmptyStringRep = NULL;

/*
 * Global variables used within this file.
 */

#ifdef SV_FINALIZE
static size_t     nofThreads;      /* Number of initialized threads */
static Tcl_Mutex  nofThreadsMutex; /* Protects the nofThreads variable */
#endif /* SV_FINALIZE */

static Bucket*    buckets;      /* Array of buckets. */
static Tcl_Mutex  bucketsMutex; /* Protects the array of buckets */

static SvCmdInfo* svCmdInfo;    /* Linked list of registered commands */
static RegType*   regType;      /* Linked list of registered obj types */
static PsStore*   psStore;      /* Linked list of registered pers. stores */

static Tcl_Mutex  svMutex;      /* Protects inserts into above lists */
static Tcl_Mutex  initMutex;    /* Serializes initialization issues */

/*
 * The standard commands found in NaviServer/AOLserver nsv_* interface.
 * For sharp-eye readers: the implementation of the "lappend" command
 * is moved to new list-command package, since it really belongs there.
 */

static Tcl_ObjCmdProc SvObjObjCmd;
static Tcl_ObjCmdProc SvAppendObjCmd;
static Tcl_ObjCmdProc SvIncrObjCmd;
static Tcl_ObjCmdProc SvSetObjCmd;
static Tcl_ObjCmdProc SvExistsObjCmd;
static Tcl_ObjCmdProc SvGetObjCmd;
static Tcl_ObjCmdProc SvArrayObjCmd;
static Tcl_ObjCmdProc SvUnsetObjCmd;
static Tcl_ObjCmdProc SvNamesObjCmd;
static Tcl_ObjCmdProc SvHandlersObjCmd;

/*
 * New commands added to
 * standard set of nsv_*
 */

static Tcl_ObjCmdProc SvPopObjCmd;
static Tcl_ObjCmdProc SvMoveObjCmd;
static Tcl_ObjCmdProc SvLockObjCmd;

/*
 * Forward declarations for functions to
 * manage buckets, arrays and shared objects.
 */

static Container* CreateContainer(Array*, Tcl_HashEntry*, Tcl_Obj*);
static Container* AcquireContainer(Array*, const char*, int);

static Array* CreateArray(Bucket*, const char*);
static Array* LockArray(Tcl_Interp*, const char*, int);

static int ReleaseContainer(Tcl_Interp*, Container*, int);
static int DeleteContainer(Container*);
static int FlushArray(Array*);
static int DeleteArray(Tcl_Interp *, Array*);

static void SvAllocateContainers(Bucket*);
static void SvRegisterStdCommands(void);

#ifdef SV_FINALIZE
static void SvFinalizeContainers(Bucket*);
static void SvFinalize(ClientData);
#endif /* SV_FINALIZE */

static PsStore* GetPsStore(const char *handle);

static int SvObjDispatchObjCmd(ClientData arg,
            Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_RegisterCommand --
 *
 *      Utility to register commands to be loaded at module start.
 *
 * Results:
 *      None.
 *
 * Side effects;
 *      New command will be added to a linked list of registered commands.
 *
 *-----------------------------------------------------------------------------
 */

void
Sv_RegisterCommand(
                   const char *cmdName,                /* Name of command to register */
                   Tcl_ObjCmdProc *objProc,            /* Object-based command procedure */
                   Tcl_CmdDeleteProc *delProc,         /* Command delete procedure */
                   int aolSpecial)
{
    size_t len = strlen(cmdName) + strlen(TSV_CMD_PREFIX) + 1;
    size_t len2 = strlen(cmdName) + strlen(TSV_CMD2_PREFIX) + 1;
    SvCmdInfo *newCmd = (SvCmdInfo*)ckalloc(sizeof(SvCmdInfo) + len + len2);

    /*
     * Setup new command structure
     */

    newCmd->cmdName = (char*)((char*)newCmd + sizeof(SvCmdInfo));
    newCmd->cmdName2 = newCmd->cmdName + len;
    newCmd->aolSpecial = aolSpecial;

    newCmd->objProcPtr = objProc;
    newCmd->delProcPtr = delProc;

    /*
     * Rewrite command name. This is needed so we can
     * easily turn-on the compatiblity with NaviServer/AOLserver
     * command names.
     */

    strcpy(newCmd->cmdName, TSV_CMD_PREFIX);
    strcat(newCmd->cmdName, cmdName);
    newCmd->name = newCmd->cmdName + strlen(TSV_CMD_PREFIX);
    strcpy(newCmd->cmdName2, TSV_CMD2_PREFIX);
    strcat(newCmd->cmdName2, cmdName);

    /*
     * Plug-in in shared list of commands.
     */

    Tcl_MutexLock(&svMutex);
    if (svCmdInfo == NULL) {
        svCmdInfo = newCmd;
        newCmd->nextPtr = NULL;
    } else {
        newCmd->nextPtr = svCmdInfo;
        svCmdInfo = newCmd;
    }
    Tcl_MutexUnlock(&svMutex);

    return;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_RegisterObjType --
 *
 *      Registers custom object duplicator function for a specific
 *      object type. Registered function will be called by the
 *      private object creation routine every time an object is
 *      plugged out or in the shared array. This way we assure that
 *      Tcl objects do not get shared per-reference between threads.
 *
 * Results:
 *      None.
 *
 * Side effects;
 *      Memory gets allocated.
 *
 *-----------------------------------------------------------------------------
 */

void
Sv_RegisterObjType(
                   const Tcl_ObjType *typePtr,               /* Type of object to register */
                   Tcl_DupInternalRepProc *dupProc)    /* Custom object duplicator */
{
    RegType *newType = (RegType*)ckalloc(sizeof(RegType));

    /*
     * Setup new type structure
     */

    newType->typePtr = typePtr;
    newType->dupIntRepProc = dupProc;

    /*
     * Plug-in in shared list
     */

    Tcl_MutexLock(&svMutex);
    newType->nextPtr = regType;
    regType = newType;
    Tcl_MutexUnlock(&svMutex);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_RegisterPsStore --
 *
 *      Registers a handler to the persistent storage.
 *
 * Results:
 *      None.
 *
 * Side effects;
 *      Memory gets allocated.
 *
 *-----------------------------------------------------------------------------
 */

void
Sv_RegisterPsStore(const PsStore *psStorePtr)
{

    PsStore *psPtr = (PsStore*)ckalloc(sizeof(PsStore));

    *psPtr = *psStorePtr;

    /*
     * Plug-in in shared list
     */

    Tcl_MutexLock(&svMutex);
    if (psStore == NULL) {
        psStore = psPtr;
        psStore->nextPtr = NULL;
    } else {
        psPtr->nextPtr = psStore;
        psStore = psPtr;
    }
    Tcl_MutexUnlock(&svMutex);
}

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_GetContainer --
 *
 *      This is the workhorse of the module. It returns the container
 *      with the shared Tcl object. It also locks the container, so
 *      when finished with operation on the Tcl object, one has to
 *      unlock the container by calling the Sv_PutContainer().
 *      If instructed, this command might also create new container
 *      with empty Tcl object.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      New container might be created.
 *
 *-----------------------------------------------------------------------------
 */

int
Sv_GetContainer(
                Tcl_Interp *interp,                 /* Current interpreter. */
                int objc,                           /* Number of arguments */
                Tcl_Obj *const objv[],              /* Argument objects. */
                Container **retObj,                 /* OUT: shared object container */
                int *offset,                        /* Shift in argument list */
                int flags)                          /* Options for locking shared array */
{
    const char *array, *key;

    if (*retObj == NULL) {
        Array *arrayPtr = NULL;

        /*
         * Parse mandatory arguments: <cmd> array key
         */

        if (objc < 3) {
            Tcl_WrongNumArgs(interp, 1, objv, "array key ?args?");
            return TCL_ERROR;
        }

        array = Tcl_GetString(objv[1]);
        key   = Tcl_GetString(objv[2]);

        *offset = 3; /* Consumed three arguments: cmd, array, key */

        /*
         * Lock the shared array and locate the shared object
         */

        arrayPtr = LockArray(interp, array, flags);
        if (arrayPtr == NULL) {
            return TCL_BREAK;
        }
        *retObj = AcquireContainer(arrayPtr, Tcl_GetString(objv[2]), flags);
        if (*retObj == NULL) {
            UnlockArray(arrayPtr);
            Tcl_AppendResult(interp, "no key ", array, "(", key, ")", NULL);
            return TCL_BREAK;
        }
    } else {
        Tcl_HashTable *handles = &((*retObj)->bucketPtr->handles);
        LOCK_CONTAINER(*retObj);
        if (Tcl_FindHashEntry(handles, (char*)(*retObj)) == NULL) {
            UNLOCK_CONTAINER(*retObj);
            Tcl_SetObjResult(interp, Tcl_NewStringObj("key has been deleted", -1));
            return TCL_BREAK;
        }
        *offset = 2; /* Consumed two arguments: object, cmd */
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_PutContainer --
 *
 *      Releases the container obtained by the Sv_GetContainer.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      For bound arrays, update the underlying persistent storage.
 *
 *-----------------------------------------------------------------------------
 */

int
Sv_PutContainer(
                Tcl_Interp *interp,               /* For error reporting; might be NULL */
                Container *svObj,                 /* Shared object container */
                int mode)                         /* One of SV_XXX modes */
{
    int ret;

    ret = ReleaseContainer(interp, svObj, mode);
    UnlockArray(svObj->arrayPtr);

    return ret;
}

/*
 *-----------------------------------------------------------------------------
 *
 * GetPsStore --
 *
 *      Performs a lookup in the list of registered persistent storage
 *      handlers. If the match is found, duplicates the persistent
 *      storage record and passes the copy to the caller.
 *
 * Results:
 *      Pointer to the newly allocated persistent storage handler. Caller
 *      must free this block when done with it. If none found, returns NULL,
 *
 * Side effects;
 *      Memory gets allocated. Caller should free the return value of this
 *      function using ckfree().
 *
 *-----------------------------------------------------------------------------
 */

static PsStore*
GetPsStore(const char *handle)
{
    int i;
    const char *type = handle;
    char *addr, *delimiter = strchr(handle, ':');
    PsStore *tmpPtr, *psPtr = NULL;

    /*
     * Expect the handle in the following format: <type>:<address>
     * where "type" must match one of the registered presistent store
     * types (gdbm, tcl, whatever) and <address> is what is passed to
     * the open procedure of the registered store.
     *
     * Example: gdbm:/path/to/gdbm/file
     */

    /*
     * Try to see wether some array is already bound to the
     * same persistent storage address.
     */

    for (i = 0; i < NUMBUCKETS; i++) {
        Tcl_HashSearch search;
        Tcl_HashEntry *hPtr;
        Bucket *bucketPtr = &buckets[i];
        LOCK_BUCKET(bucketPtr);
        hPtr = Tcl_FirstHashEntry(&bucketPtr->arrays, &search);
        while (hPtr) {
            Array *arrayPtr = (Array*)Tcl_GetHashValue(hPtr);
            if (arrayPtr->bindAddr && arrayPtr->psPtr) {
                if (strcmp(arrayPtr->bindAddr, handle) == 0) {
                    UNLOCK_BUCKET(bucketPtr);
                    return NULL; /* Array already bound */
                }
            }
            hPtr = Tcl_NextHashEntry(&search);
        }
        UNLOCK_BUCKET(bucketPtr);
    }

    /*
     * Split the address and storage handler
     */

    if (delimiter == NULL) {
        addr = NULL;
    } else {
        *delimiter = 0;
        addr = delimiter + 1;
    }

    /*
     * No array was bound to the same persistent storage.
     * Lookup the persistent storage to bind to.
     */

    Tcl_MutexLock(&svMutex);
    for (tmpPtr = psStore; tmpPtr; tmpPtr = tmpPtr->nextPtr) {
        if (strcmp(tmpPtr->type, type) == 0) {
            tmpPtr->psHandle = tmpPtr->psOpen(addr);
            if (tmpPtr->psHandle) {
                psPtr = (PsStore*)ckalloc(sizeof(PsStore));
                *psPtr = *tmpPtr;
                psPtr->nextPtr = NULL;
            }
            break;
        }
    }
    Tcl_MutexUnlock(&svMutex);

    if (delimiter) {
        *delimiter = ':';
    }

    return psPtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * AcquireContainer --
 *
 *      Finds a variable within an array and returns it's container.
 *
 * Results:
 *      Pointer to variable object.
 *
 * Side effects;
 *      New variable may be created. For bound arrays, try to locate
 *      the key in the persistent storage as well.
 *
 *-----------------------------------------------------------------------------
 */

static Container *
AcquireContainer(
                 Array *arrayPtr,
                 const char *key,
                 int flags)
{
    int isNew;
    Tcl_Obj *tclObj = NULL;
    Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&arrayPtr->vars, key);

    if (hPtr == NULL) {
        PsStore *psPtr = arrayPtr->psPtr;
        if (psPtr) {
            char *val = NULL;
            size_t len = 0;
            if (psPtr->psGet(psPtr->psHandle, key, &val, &len) == 0) {
                tclObj = Tcl_NewStringObj(val, len);
                psPtr->psFree(psPtr->psHandle, val);
            }
        }
        if (!(flags & FLAGS_CREATEVAR) && tclObj == NULL) {
            return NULL;
        }
        if (tclObj == NULL) {
            tclObj = Tcl_NewObj();
        }
        hPtr = Tcl_CreateHashEntry(&arrayPtr->vars, key, &isNew);
        Tcl_SetHashValue(hPtr, CreateContainer(arrayPtr, hPtr, tclObj));
    }

    return (Container*)Tcl_GetHashValue(hPtr);
}

/*
 *-----------------------------------------------------------------------------
 *
 * ReleaseContainer --
 *
 *      Does some post-processing on the used container. This is mostly
 *      needed when the container has been modified and needs to be
 *      saved in the bound persistent storage.
 *
 * Results:
 *      A standard Tcl result
 *
 * Side effects:
 *      Persistent storage, if bound, might be modified.
 *
 *-----------------------------------------------------------------------------
 */

static int
ReleaseContainer(
                 Tcl_Interp *interp,
                 Container *svObj,
                 int mode)
{
    const PsStore *psPtr = svObj->arrayPtr->psPtr;
    size_t len;
    char *key, *val;

    switch (mode) {
    case SV_UNCHANGED: return TCL_OK;
    case SV_ERROR:     return TCL_ERROR;
    case SV_CHANGED:
        if (psPtr) {
            key = (char *)Tcl_GetHashKey(&svObj->arrayPtr->vars, svObj->entryPtr);
            val = Tcl_GetString(svObj->tclObj);
            len = svObj->tclObj->length;
            if (psPtr->psPut(psPtr->psHandle, key, val, len) == -1) {
                const char *err = psPtr->psError(psPtr->psHandle);
                Tcl_SetObjResult(interp, Tcl_NewStringObj(err, -1));
                return TCL_ERROR;
            }
        }
        return TCL_OK;
    }

    return TCL_ERROR; /* Should never be reached */
}

/*
 *-----------------------------------------------------------------------------
 *
 * CreateContainer --
 *
 *      Creates new shared container holding Tcl object to be stored
 *      in the shared array
 *
 * Results:
 *      The container pointer.
 *
 * Side effects:
 *      Memory gets allocated.
 *
 *-----------------------------------------------------------------------------
 */

static Container *
CreateContainer(
                Array *arrayPtr,
                Tcl_HashEntry *entryPtr,
                Tcl_Obj *tclObj)
{
    Container *svObj;

    if (arrayPtr->bucketPtr->freeCt == NULL) {
        SvAllocateContainers(arrayPtr->bucketPtr);
    }

    svObj = arrayPtr->bucketPtr->freeCt;
    arrayPtr->bucketPtr->freeCt = svObj->nextPtr;

    svObj->arrayPtr  = arrayPtr;
    svObj->bucketPtr = arrayPtr->bucketPtr;
    svObj->tclObj    = tclObj;
    svObj->entryPtr  = entryPtr;
    svObj->handlePtr = NULL;

    if (svObj->tclObj) {
        Tcl_IncrRefCount(svObj->tclObj);
    }

    return svObj;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteContainer --
 *
 *      Destroys the container and the Tcl object within it. For bound
 *      shared arrays, the underlying persistent store is updated as well.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory gets reclaimed. If the shared array was bound to persistent
 *      storage, it removes the corresponding record.
 *
 *-----------------------------------------------------------------------------
 */

static int
DeleteContainer(
                Container *svObj)
{
    if (svObj->tclObj) {
        Tcl_DecrRefCount(svObj->tclObj);
    }
    if (svObj->handlePtr) {
        Tcl_DeleteHashEntry(svObj->handlePtr);
    }
    if (svObj->entryPtr) {
        PsStore *psPtr = svObj->arrayPtr->psPtr;
        if (psPtr) {
            char *key = (char *)Tcl_GetHashKey(&svObj->arrayPtr->vars,svObj->entryPtr);
            if (psPtr->psDelete(psPtr->psHandle, key) == -1) {
                return TCL_ERROR;
            }
        }
        Tcl_DeleteHashEntry(svObj->entryPtr);
    }

    svObj->arrayPtr  = NULL;
    svObj->entryPtr  = NULL;
    svObj->handlePtr = NULL;
    svObj->tclObj    = NULL;

    svObj->nextPtr = svObj->bucketPtr->freeCt;
    svObj->bucketPtr->freeCt = svObj;

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * LockArray --
 *
 *      Find (or create) the array structure for shared array and lock it.
 *      Array structure must be later unlocked with UnlockArray.
 *
 * Results:
 *      TCL_OK or TCL_ERROR if no such array.
 *
 * Side effects:
 *      Sets *arrayPtrPtr with Array pointer or leave error in given interp.
 *
 *-----------------------------------------------------------------------------
 */

static Array *
LockArray(
          Tcl_Interp *interp,                 /* Interpreter to leave result. */
          const char *array,                  /* Name of array to lock */
          int flags)                          /* FLAGS_CREATEARRAY/FLAGS_NOERRMSG*/
{
    const char *p;
    unsigned int result;
    int i;
    Bucket *bucketPtr;
    Array *arrayPtr;

    /*
     * Compute a hash to map an array to a bucket.
     */

    p = array;
    result = 0;
    while (*p++) {
        i = *p;
        result += (result << 3) + i;
    }
    i = result % NUMBUCKETS;
    bucketPtr = &buckets[i];

    /*
     * Lock the bucket and find the array, or create a new one.
     * The bucket will be left locked on success.
     */

    LOCK_BUCKET(bucketPtr); /* Note: no matching unlock below ! */
    if (flags & FLAGS_CREATEARRAY) {
        arrayPtr = CreateArray(bucketPtr, array);
    } else {
        Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&bucketPtr->arrays, array);
        if (hPtr == NULL) {
            UNLOCK_BUCKET(bucketPtr);
            if (!(flags & FLAGS_NOERRMSG)) {
                Tcl_AppendResult(interp, "\"", array,
                                 "\" is not a thread shared array", NULL);
            }
            return NULL;
        }
        arrayPtr = (Array*)Tcl_GetHashValue(hPtr);
    }

    return arrayPtr;
}
/*
 *-----------------------------------------------------------------------------
 *
 * FlushArray --
 *
 *      Unset all keys in an array.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Array is cleaned but it's variable hash-hable still lives.
 *      For bound arrays, the persistent store is updated accordingly.
 *
 *-----------------------------------------------------------------------------
 */

static int
FlushArray(Array *arrayPtr)                    /* Name of array to flush */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    for (hPtr = Tcl_FirstHashEntry(&arrayPtr->vars, &search); hPtr;
         hPtr = Tcl_NextHashEntry(&search)) {
        if (DeleteContainer((Container*)Tcl_GetHashValue(hPtr)) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * CreateArray --
 *
 *      Creates new shared array instance.
 *
 * Results:
 *      Pointer to the newly created array
 *
 * Side effects:
 *      Memory gets allocated
 *
 *-----------------------------------------------------------------------------
 */

static Array *
CreateArray(
            Bucket *bucketPtr,
            const char *arrayName)
{
    int isNew;
    Array *arrayPtr;
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_CreateHashEntry(&bucketPtr->arrays, arrayName, &isNew);
    if (!isNew) {
        return (Array*)Tcl_GetHashValue(hPtr);
    }

    arrayPtr = (Array*)ckalloc(sizeof(Array));
    arrayPtr->bucketPtr = bucketPtr;
    arrayPtr->entryPtr  = hPtr;
    arrayPtr->psPtr     = NULL;
    arrayPtr->bindAddr  = NULL;

    Tcl_InitHashTable(&arrayPtr->vars, TCL_STRING_KEYS);
    Tcl_SetHashValue(hPtr, arrayPtr);

    return arrayPtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * DeleteArray --
 *
 *      Deletes the shared array.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      Memory gets reclaimed.
 *
 *-----------------------------------------------------------------------------
 */

static int
UnbindArray(Tcl_Interp *interp, Array *arrayPtr)
{
    PsStore *psPtr = arrayPtr->psPtr;
    if (arrayPtr->bindAddr) {
        ckfree(arrayPtr->bindAddr);
        arrayPtr->bindAddr = NULL;
    }
    if (psPtr) {
        if (psPtr->psClose(psPtr->psHandle) == -1) {
            if (interp) {
                const char *err = psPtr->psError(psPtr->psHandle);
                Tcl_SetObjResult(interp, Tcl_NewStringObj(err, -1));
            }
            return TCL_ERROR;
        }
        ckfree((char*)arrayPtr->psPtr), arrayPtr->psPtr = NULL;
        arrayPtr->psPtr = NULL;
    }
    return TCL_OK;
}

static int
DeleteArray(Tcl_Interp *interp, Array *arrayPtr)
{
    if (FlushArray(arrayPtr) == -1) {
        return TCL_ERROR;
    }
    if (arrayPtr->psPtr) {
        if (UnbindArray(interp, arrayPtr) != TCL_OK) {
            return TCL_ERROR;
        };
    }
    if (arrayPtr->entryPtr) {
        Tcl_DeleteHashEntry(arrayPtr->entryPtr);
    }

    Tcl_DeleteHashTable(&arrayPtr->vars);
    ckfree((char*)arrayPtr);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvAllocateContainers --
 *
 *      Any similarity with the Tcl AllocateFreeObj function is purely
 *      coincidental... Just joking; this is (almost) 100% copy of it! :-)
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Allocates memory for many containers at the same time
 *
 *-----------------------------------------------------------------------------
 */

static void
SvAllocateContainers(Bucket *bucketPtr)
{
    Container tmp[2];
    size_t objSizePlusPadding = (size_t)(((char*)(tmp+1))-(char*)tmp);
    size_t bytesToAlloc = (OBJS_TO_ALLOC_EACH_TIME * objSizePlusPadding);
    char *basePtr;
    Container *prevPtr = NULL, *objPtr = NULL;
    int i;

    basePtr = (char*)ckalloc(bytesToAlloc);
    memset(basePtr, 0, bytesToAlloc);

    objPtr = (Container*)basePtr;
    objPtr->chunkAddr = basePtr; /* Mark chunk address for reclaim */

    for (i = 0; i < OBJS_TO_ALLOC_EACH_TIME; i++) {
        objPtr->nextPtr = prevPtr;
        prevPtr = objPtr;
        objPtr = (Container*)(((char*)objPtr) + objSizePlusPadding);
    }
    bucketPtr->freeCt = prevPtr;
}

#ifdef SV_FINALIZE
/*
 *-----------------------------------------------------------------------------
 *
 * SvFinalizeContainers --
 *
 *    Reclaim memory for free object containers per bucket.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Memory gets reclaimed
 *
 *-----------------------------------------------------------------------------
 */

static void
SvFinalizeContainers(Bucket *bucketPtr)
{
   Container *tmpPtr, *objPtr = bucketPtr->freeCt;

    while (objPtr) {
        if (objPtr->chunkAddr == (char*)objPtr) {
            tmpPtr = objPtr->nextPtr;
            ckfree((char*)objPtr);
            objPtr = tmpPtr;
        } else {
            objPtr = objPtr->nextPtr;
        }
    }
}
#endif /* SV_FINALIZE */

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_DuplicateObj --
 *
 *  Create and return a new object that is (mostly) a duplicate of the
 *  argument object. We take care that the duplicate object is either
 *  a proper object copy, i.e. w/o hidden references to original object
 *  elements or a plain string object, i.e one w/o internal representation.
 *
 *  Decision about whether to produce a real duplicate or a string object
 *  is done as follows:
 *
 *     1) Scalar Tcl object types are properly copied by default;
 *        these include: boolean, int double, string and byteArray types.
 *     2) Object registered with Sv_RegisterObjType are duplicated
 *        using custom duplicator function which is guaranteed to
 *        produce a proper deep copy of the object in question.
 *     3) All other object types are stringified; these include
 *        miscelaneous Tcl objects (cmdName, nsName, bytecode, etc, etc)
 *        and all user-defined objects.
 *
 * Results:
 *      The return value is a pointer to a newly created Tcl_Obj. This
 *      object has reference count 0 and the same type, if any, as the
 *      source object objPtr. Also:
 *
 *        1) If the source object has a valid string rep, we copy it;
 *           otherwise, the new string rep is marked invalid.
 *        2) If the source object has an internal representation (i.e. its
 *           typePtr is non-NULL), the new object's internal rep is set to
 *           a copy; otherwise the new internal rep is marked invalid.
 *
 * Side effects:
 *  Some object may, when copied, loose their type, i.e. will become
 *  just plain string objects.
 *
 *-----------------------------------------------------------------------------
 */

Tcl_Obj *
Sv_DuplicateObj(
    Tcl_Obj *objPtr        /* The object to duplicate. */
) {
    Tcl_Obj *dupPtr = Tcl_NewObj();

    /*
     * Handle the internal rep
     */

    if (objPtr->typePtr != NULL) {
        if (objPtr->typePtr->dupIntRepProc == NULL) {
            dupPtr->internalRep = objPtr->internalRep;
            dupPtr->typePtr = objPtr->typePtr;
            Tcl_InvalidateStringRep(dupPtr);
        } else {
            if (   objPtr->typePtr == booleanObjTypePtr    \
                || objPtr->typePtr == byteArrayObjTypePtr  \
                || objPtr->typePtr == doubleObjTypePtr     \
                || objPtr->typePtr == intObjTypePtr        \
                || objPtr->typePtr == wideIntObjTypePtr    \
                || objPtr->typePtr == stringObjTypePtr) {
               /*
                * Cover all "safe" obj types (see header comment)
                */
              (*objPtr->typePtr->dupIntRepProc)(objPtr, dupPtr);
              Tcl_InvalidateStringRep(dupPtr);
            } else {
                int found = 0;
                RegType *regPtr;
               /*
                * Cover special registered types. Assume not
                * very many of those, so this sequential walk
                * should be fast enough.
                */
                for (regPtr = regType; regPtr; regPtr = regPtr->nextPtr) {
                    if (objPtr->typePtr == regPtr->typePtr) {
                        (*regPtr->dupIntRepProc)(objPtr, dupPtr);
                        Tcl_InvalidateStringRep(dupPtr);
                        found = 1;
                        break;
                    }
                }
               /*
                * Assure at least string rep of the source
                * is present, which will be copied below.
                */
                if (found == 0 && objPtr->bytes == NULL
                    && objPtr->typePtr->updateStringProc != NULL) {
                    (*objPtr->typePtr->updateStringProc)(objPtr);
                }
            }
        }
    }

    /*
     * Handle the string rep
     */

    if (objPtr->bytes == NULL) {
        dupPtr->bytes = NULL;
    } else if (objPtr->bytes != Sv_tclEmptyStringRep) {
        /* A copy of TclInitStringRep macro */
        dupPtr->bytes = (char*)ckalloc((unsigned)objPtr->length + 1);
        if (objPtr->length > 0) {
            memcpy((void*)dupPtr->bytes,(void*)objPtr->bytes,
                   (unsigned)objPtr->length);
        }
        dupPtr->length = objPtr->length;
        dupPtr->bytes[objPtr->length] = '\0';
    }

    return dupPtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvObjDispatchObjCmd --
 *
 *      The method command for dispatching sub-commands of the shared
 *      object.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      Depends on the dispatched command
 *
 *-----------------------------------------------------------------------------
 */

static int
SvObjDispatchObjCmd(
                    ClientData arg,                     /* Pointer to object container. */
                    Tcl_Interp *interp,                 /* Current interpreter. */
                    int objc,                           /* Number of arguments. */
                    Tcl_Obj *const objv[])              /* Argument objects. */
{
    const char *cmdName;
    SvCmdInfo *cmdPtr;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "args");
        return TCL_ERROR;
    }

    cmdName = Tcl_GetString(objv[1]);

    /*
     * Do simple linear search. We may later replace this list
     * with the hash table to gain speed. Currently, the list
     * of registered commands is so small, so this will work
     * fast enough.
     */

    for (cmdPtr = svCmdInfo; cmdPtr; cmdPtr = cmdPtr->nextPtr) {
        if (!strcmp(cmdPtr->name, cmdName)) {
            return (*cmdPtr->objProcPtr)(arg, interp, objc, objv);
        }
    }

    Tcl_AppendResult(interp, "invalid command name \"", cmdName, "\"", NULL);
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvObjObjCmd --
 *
 *      Creates the object command for a shared array.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      New Tcl command gets created.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvObjObjCmd(
            ClientData arg,                     /* != NULL if aolSpecial */
            Tcl_Interp *interp,                 /* Current interpreter. */
            int objc,                           /* Number of arguments. */
            Tcl_Obj *const objv[])              /* Argument objects. */
{
    int isNew, off, ret, flg;
    char buf[128];
    Tcl_Obj *val = NULL;
    Container *svObj = NULL;

    /*
     * Syntax: sv::object array key ?var?
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    switch (ret) {
    case TCL_BREAK: /* Shared array was not found */
        if ((objc - off)) {
            val = objv[off];
        }
        Tcl_ResetResult(interp);
        flg = FLAGS_CREATEARRAY | FLAGS_CREATEVAR;
        ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, flg);
        if (ret != TCL_OK) {
            return TCL_ERROR;
        }
        Tcl_DecrRefCount(svObj->tclObj);
        svObj->tclObj = Sv_DuplicateObj(val ? val : Tcl_NewObj());
        Tcl_IncrRefCount(svObj->tclObj);
        break;
    case TCL_ERROR:
        return TCL_ERROR;
    }

    if (svObj->handlePtr == NULL) {
        Tcl_HashTable *handles = &svObj->arrayPtr->bucketPtr->handles;
        svObj->handlePtr = Tcl_CreateHashEntry(handles, (char*)svObj, &isNew);
    }

    /*
     * Format the command name
     */

    sprintf(buf, "::%p", (int*)svObj);
    svObj->aolSpecial = (arg != NULL);
    Tcl_CreateObjCommand(interp, buf, SvObjDispatchObjCmd, svObj, NULL);
    Tcl_ResetResult(interp);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buf, -1));

    return Sv_PutContainer(interp, svObj, SV_UNCHANGED);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvArrayObjCmd --
 *
 *      This procedure is invoked to process the "tsv::array" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvArrayObjCmd(
              ClientData arg,                     /* Pointer to object container. */
              Tcl_Interp *interp,                 /* Current interpreter. */
              int objc,                           /* Number of arguments. */
              Tcl_Obj *const objv[])              /* Argument objects. */
{
    int i, argx = 0, lobjc = 0, index, ret = TCL_OK;
    const char *arrayName = NULL;
    Array *arrayPtr = NULL;
    Tcl_Obj **lobjv = NULL;
    Container *svObj, *elObj = NULL;

    static const char *opts[] = {
        "set",  "reset", "get", "names", "size", "exists", "isbound",
        "bind", "unbind", NULL
    };
    enum options {
        ASET,   ARESET,  AGET,  ANAMES,  ASIZE,  AEXISTS, AISBOUND,
        ABIND,  AUNBIND
    };

    svObj = (Container*)arg;

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "option array");
        return TCL_ERROR;
    }

    arrayName = Tcl_GetString(objv[2]);
    arrayPtr  = LockArray(interp, arrayName, FLAGS_NOERRMSG);

    if (objc > 3) {
        argx = 3;
    }

    Tcl_ResetResult(interp);

    if (Tcl_GetIndexFromObjStruct(interp,objv[1],opts, sizeof(char *),"option",0,&index) != TCL_OK) {
        ret = TCL_ERROR;

    } else if (index == AEXISTS) {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), arrayPtr!=0);

    } else if (index == AISBOUND) {
        if (arrayPtr == NULL) {
            Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
        } else {
            Tcl_SetIntObj(Tcl_GetObjResult(interp), arrayPtr->psPtr!=0);
        }

    } else if (index == ASIZE) {
        if (arrayPtr == NULL) {
            Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
        } else {
            Tcl_SetWideIntObj(Tcl_GetObjResult(interp),arrayPtr->vars.numEntries);
        }

    } else if (index == ASET || index == ARESET) {
        if (argx == (objc - 1)) {
            if (argx && Tcl_ListObjGetElements(interp, objv[argx], &lobjc,
                    &lobjv) != TCL_OK) {
                ret = TCL_ERROR;
                goto cmdExit;
            }
        } else {
            lobjc = objc - 3;
            lobjv = (Tcl_Obj**)objv + 3;
        }
        if (lobjc & 1) {
            Tcl_AppendResult(interp, "list must have an even number"
                    " of elements", NULL);
            ret = TCL_ERROR;
            goto cmdExit;
        }
        if (arrayPtr == NULL) {
            arrayPtr = LockArray(interp, arrayName, FLAGS_CREATEARRAY);
        }
        if (index == ARESET) {
            ret = FlushArray(arrayPtr);
            if (ret != TCL_OK) {
                if (arrayPtr->psPtr) {
                    PsStore *psPtr = arrayPtr->psPtr;
                    const char *err = psPtr->psError(psPtr->psHandle);
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(err, -1));
                }
                goto cmdExit;
            }
        }
        for (i = 0; i < lobjc; i += 2) {
            const char *key = Tcl_GetString(lobjv[i]);
            elObj = AcquireContainer(arrayPtr, key, FLAGS_CREATEVAR);
            Tcl_DecrRefCount(elObj->tclObj);
            elObj->tclObj = Sv_DuplicateObj(lobjv[i+1]);
            Tcl_IncrRefCount(elObj->tclObj);
            if (ReleaseContainer(interp, elObj, SV_CHANGED) != TCL_OK) {
                ret = TCL_ERROR;
                goto cmdExit;
            }
        }

    } else if (index == AGET || index == ANAMES) {
        if (arrayPtr) {
            Tcl_HashSearch search;
            Tcl_Obj *resObj = Tcl_NewListObj(0, NULL);
            const char *pattern = (argx == 0) ? NULL : Tcl_GetString(objv[argx]);
            Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&arrayPtr->vars,&search);
            while (hPtr) {
                char *key = (char *)Tcl_GetHashKey(&arrayPtr->vars, hPtr);
                if (pattern == NULL || Tcl_StringCaseMatch(key, pattern, 0)) {
                    Tcl_ListObjAppendElement(interp, resObj,
                            Tcl_NewStringObj(key, -1));
                    if (index == AGET) {
                        elObj = (Container*)Tcl_GetHashValue(hPtr);
                        Tcl_ListObjAppendElement(interp, resObj,
                                Sv_DuplicateObj(elObj->tclObj));
                    }
                }
                hPtr = Tcl_NextHashEntry(&search);
            }
            Tcl_SetObjResult(interp, resObj);
        }

    } else if (index == ABIND) {

        /*
         * This is more complex operation, requiring some clarification.
         *
         * When binding an already existing array, we walk the array
         * first and store all key/value pairs found there in the
         * persistent storage. Then we proceed with the below.
         *
         * When binding an non-existent array, we open the persistent
         * storage and cache all key/value pairs found there into tne
         * newly created shared array.
         */

        PsStore *psPtr;
        Tcl_HashEntry *hPtr;
        size_t len;
        int isNew;
        char *psurl, *key = NULL, *val = NULL;

        if (objc < 4) {
            Tcl_WrongNumArgs(interp, 2, objv, "array handle");
            ret = TCL_ERROR;
            goto cmdExit;
        }

        if (arrayPtr && arrayPtr->psPtr) {
            Tcl_AppendResult(interp, "array is already bound", NULL);
            ret = TCL_ERROR;
            goto cmdExit;
        }

        psurl = Tcl_GetString(objv[3]);
        len = objv[3]->length;
        psPtr = GetPsStore(psurl);

        if (psPtr == NULL) {
            Tcl_AppendResult(interp, "can't open persistent storage on \"",
                             psurl, "\"", NULL);
            ret = TCL_ERROR;
            goto cmdExit;
        }
        if (arrayPtr) {
            Tcl_HashSearch search;
            hPtr = Tcl_FirstHashEntry(&arrayPtr->vars,&search);
            arrayPtr->psPtr = psPtr;
            arrayPtr->bindAddr = strcpy((char *)ckalloc(len+1), psurl);
            while (hPtr) {
                svObj = (Container *)Tcl_GetHashValue(hPtr);
                if (ReleaseContainer(interp, svObj, SV_CHANGED) != TCL_OK) {
                    ret = TCL_ERROR;
                    goto cmdExit;
                }
                hPtr = Tcl_NextHashEntry(&search);
            }
        } else {
            arrayPtr = LockArray(interp, arrayName, FLAGS_CREATEARRAY);
            arrayPtr->psPtr = psPtr;
            arrayPtr->bindAddr = strcpy((char *)ckalloc(len+1), psurl);
        }
        if (!psPtr->psFirst(psPtr->psHandle, &key, &val, &len)) {
            do {
                Tcl_Obj * tclObj = Tcl_NewStringObj(val, len);
                hPtr = Tcl_CreateHashEntry(&arrayPtr->vars, key, &isNew);
                Tcl_SetHashValue(hPtr, CreateContainer(arrayPtr, hPtr, tclObj));
                psPtr->psFree(psPtr->psHandle, val);
            } while (!psPtr->psNext(psPtr->psHandle, &key, &val, &len));
        }

    } else if (index == AUNBIND) {
        if (!arrayPtr || !arrayPtr->psPtr) {
            Tcl_AppendResult(interp, "shared variable is not bound", NULL);
            ret = TCL_ERROR;
            goto cmdExit;
        }
        if (UnbindArray(interp, arrayPtr) != TCL_OK) {
            ret = TCL_ERROR;
            goto cmdExit;
        }
    }

 cmdExit:
    if (arrayPtr) {
        UnlockArray(arrayPtr);
    }

    return ret;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvUnsetObjCmd --
 *
 *      This procedure is invoked to process the "tsv::unset" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvUnsetObjCmd(
              ClientData dummy,                   /* Not used. */
              Tcl_Interp *interp,                 /* Current interpreter. */
              int objc,                           /* Number of arguments. */
              Tcl_Obj *const objv[])              /* Argument objects. */
{
    int ii;
    const char *arrayName;
    Array *arrayPtr;

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "array ?key ...?");
        return TCL_ERROR;
    }

    arrayName = Tcl_GetString(objv[1]);
    arrayPtr  = LockArray(interp, arrayName, 0);

    if (arrayPtr == NULL) {
        return TCL_ERROR;
    }
    if (objc == 2) {
        UnlockArray(arrayPtr);
        if (DeleteArray(interp, arrayPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        for (ii = 2; ii < objc; ii++) {
            const char *key = Tcl_GetString(objv[ii]);
            Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&arrayPtr->vars, key);
            if (hPtr) {
                if (DeleteContainer((Container*)Tcl_GetHashValue(hPtr))
                    != TCL_OK) {
                    UnlockArray(arrayPtr);
                    return TCL_ERROR;
                }
            } else {
                UnlockArray(arrayPtr);
                Tcl_AppendResult(interp,"no key ",arrayName,"(",key,")",NULL);
                return TCL_ERROR;
            }
        }
        UnlockArray(arrayPtr);
    }

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvNamesObjCmd --
 *
 *      This procedure is invoked to process the "tsv::names" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvNamesObjCmd(
              ClientData arg,                     /* != NULL if aolSpecial */
              Tcl_Interp *interp,                 /* Current interpreter. */
              int objc,                           /* Number of arguments. */
              Tcl_Obj *const objv[])              /* Argument objects. */
{
    int i;
    const char *pattern = NULL;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    Tcl_Obj *resObj;

    if (objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?pattern?");
        return TCL_ERROR;
    }
    if (objc == 2) {
        pattern = Tcl_GetString(objv[1]);
    }

    resObj = Tcl_NewListObj(0, NULL);

    for (i = 0; i < NUMBUCKETS; i++) {
        Bucket *bucketPtr = &buckets[i];
        LOCK_BUCKET(bucketPtr);
        hPtr = Tcl_FirstHashEntry(&bucketPtr->arrays, &search);
        while (hPtr) {
            char *key = (char *)Tcl_GetHashKey(&bucketPtr->arrays, hPtr);
            if ((arg==NULL || (*key != '.')) /* Hide .<name> arrays for AOL*/ &&
                (pattern == NULL || Tcl_StringCaseMatch(key, pattern, 0))) {
                Tcl_ListObjAppendElement(interp, resObj,
                        Tcl_NewStringObj(key, -1));
            }
            hPtr = Tcl_NextHashEntry(&search);
        }
        UNLOCK_BUCKET(bucketPtr);
    }

    Tcl_SetObjResult(interp, resObj);

    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvGetObjCmd --
 *
 *      This procedure is invoked to process "tsv::get" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvGetObjCmd(
            ClientData arg,                     /* Pointer to object container. */
            Tcl_Interp *interp,                 /* Current interpreter. */
            int objc,                           /* Number of arguments. */
            Tcl_Obj *const objv[])              /* Argument objects. */
{
    int off, ret;
    Tcl_Obj *res;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          tsv::get array key ?var?
     *          $object get ?var?
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    switch (ret) {
    case TCL_BREAK:
        if ((objc - off) == 0) {
            return TCL_ERROR;
        } else {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
            return TCL_OK;
        }
    case TCL_ERROR:
        return TCL_ERROR;
    }

    res = Sv_DuplicateObj(svObj->tclObj);

    if ((objc - off) == 0) {
        Tcl_SetObjResult(interp, res);
    } else {
        if (Tcl_ObjSetVar2(interp, objv[off], NULL, res, 0) == NULL) {
            Tcl_DecrRefCount(res);
            goto cmd_err;
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
    }

    return Sv_PutContainer(interp, svObj, SV_UNCHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvExistsObjCmd --
 *
 *      This procedure is invoked to process "tsv::exists" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvExistsObjCmd(
               ClientData arg,                     /* Pointer to object container. */
               Tcl_Interp *interp,                 /* Current interpreter. */
               int objc,                           /* Number of arguments. */
               Tcl_Obj *const objv[])              /* Argument objects. */
{
    int off, ret;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          tsv::exists array key
     *          $object exists
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    switch (ret) {
    case TCL_BREAK: /* Array/key not found */
        Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        return TCL_OK;
    case TCL_ERROR:
        return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));

    return Sv_PutContainer(interp, svObj, SV_UNCHANGED);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvSetObjCmd --
 *
 *      This procedure is invoked to process the "tsv::set" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvSetObjCmd(
            ClientData arg,                     /* Pointer to object container */
            Tcl_Interp *interp,                 /* Current interpreter. */
            int objc,                           /* Number of arguments. */
            Tcl_Obj *const objv[])              /* Argument objects. */
{
    int ret, off, flg, mode;
    Tcl_Obj *val;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          tsv::set array key ?value?
     *          $object set ?value?
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    switch (ret) {
    case TCL_BREAK:
        if ((objc - off) == 0) {
            return TCL_ERROR;
        } else {
            Tcl_ResetResult(interp);
            flg = FLAGS_CREATEARRAY | FLAGS_CREATEVAR;
            ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, flg);
            if (ret != TCL_OK) {
                return TCL_ERROR;
            }
        }
        break;
    case TCL_ERROR:
        return TCL_ERROR;
    }
    if ((objc - off)) {
        val = objv[off];
        Tcl_DecrRefCount(svObj->tclObj);
        svObj->tclObj = Sv_DuplicateObj(val);
        Tcl_IncrRefCount(svObj->tclObj);
        mode = SV_CHANGED;
    } else {
        val = Sv_DuplicateObj(svObj->tclObj);
        mode = SV_UNCHANGED;
    }

    Tcl_SetObjResult(interp, val);

    return Sv_PutContainer(interp, svObj, mode);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvIncrObjCmd --
 *
 *      This procedure is invoked to process the "tsv::incr" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvIncrObjCmd(
             ClientData arg,                     /* Pointer to object container */
             Tcl_Interp *interp,                 /* Current interpreter. */
             int objc,                           /* Number of arguments. */
             Tcl_Obj *const objv[])              /* Argument objects. */
{
    int off, ret, flg, isNew = 0;
    Tcl_WideInt incrValue = 1, currValue = 0;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          tsv::incr array key ?increment?
     *          $object incr ?increment?
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    if (ret != TCL_OK) {
        if (ret != TCL_BREAK) {
            return TCL_ERROR;
        }
        flg = FLAGS_CREATEARRAY | FLAGS_CREATEVAR;
        Tcl_ResetResult(interp);
        ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, flg);
        if (ret != TCL_OK) {
            return TCL_ERROR;
        }
        isNew = 1;
    }
    if ((objc - off)) {
        ret = Tcl_GetWideIntFromObj(interp, objv[off], &incrValue);
        if (ret != TCL_OK) {
            goto cmd_err;
        }
    }
    if (isNew) {
        currValue = 0;
    } else {
        ret = Tcl_GetWideIntFromObj(interp, svObj->tclObj, &currValue);
        if (ret != TCL_OK) {
            goto cmd_err;
        }
    }

    incrValue += currValue;
    Tcl_SetWideIntObj(svObj->tclObj, incrValue);
    Tcl_ResetResult(interp);
    Tcl_SetWideIntObj(Tcl_GetObjResult(interp), incrValue);

    return Sv_PutContainer(interp, svObj, SV_CHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvAppendObjCmd --
 *
 *      This procedure is invoked to process the "tsv::append" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvAppendObjCmd(
               ClientData arg,                     /* Pointer to object container */
               Tcl_Interp *interp,                 /* Current interpreter. */
               int objc,                           /* Number of arguments. */
               Tcl_Obj *const objv[])              /* Argument objects. */
{
    int i, off, flg, ret;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          tsv::append array key value ?value ...?
     *          $object append value ?value ...?
     */

    flg = FLAGS_CREATEARRAY | FLAGS_CREATEVAR;
    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, flg);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc - off) < 1) {
        Tcl_WrongNumArgs(interp, off, objv, "value ?value ...?");
        goto cmd_err;
    }
    for (i = off; i < objc; ++i) {
        Tcl_AppendObjToObj(svObj->tclObj, Sv_DuplicateObj(objv[i]));
    }

    Tcl_SetObjResult(interp, Sv_DuplicateObj(svObj->tclObj));

    return Sv_PutContainer(interp, svObj, SV_CHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvPopObjCmd --
 *
 *      This procedure is invoked to process "tsv::pop" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvPopObjCmd(
            ClientData arg,                     /* Pointer to object container */
            Tcl_Interp *interp,                 /* Current interpreter. */
            int objc,                           /* Number of arguments. */
            Tcl_Obj *const objv[])              /* Argument objects. */
{
    int ret, off;
    Tcl_Obj *retObj;
    Array *arrayPtr = NULL;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          tsv::pop array key ?var?
     *          $object pop ?var?
     *
     * Note: the object command will run into error next time !
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    switch (ret) {
    case TCL_BREAK:
        if ((objc - off) == 0) {
            return TCL_ERROR;
        } else {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
            return TCL_OK;
        }
    case TCL_ERROR:
        return TCL_ERROR;
    }

    arrayPtr = svObj->arrayPtr;

    retObj = svObj->tclObj;
    svObj->tclObj = NULL;

    if (DeleteContainer(svObj) != TCL_OK) {
        if (svObj->arrayPtr->psPtr) {
            PsStore *psPtr = svObj->arrayPtr->psPtr;
            const char *err = psPtr->psError(psPtr->psHandle);
            Tcl_SetObjResult(interp, Tcl_NewStringObj(err,-1));
        }
        ret = TCL_ERROR;
        goto cmd_exit;
    }

    if ((objc - off) == 0) {
        Tcl_SetObjResult(interp, retObj);
    } else {
        if (Tcl_ObjSetVar2(interp, objv[off], NULL, retObj, 0) == NULL) {
            ret = TCL_ERROR;
            goto cmd_exit;
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
    }

  cmd_exit:
    Tcl_DecrRefCount(retObj);
    UnlockArray(arrayPtr);

    return ret;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvMoveObjCmd --
 *
 *      This procedure is invoked to process the "tsv::move" command.
 *      See the user documentation for details on what it does.
 *
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvMoveObjCmd(
             ClientData arg,                     /* Pointer to object container. */
             Tcl_Interp *interp,                 /* Current interpreter. */
             int objc,                           /* Number of arguments. */
             Tcl_Obj *const objv[])              /* Argument objects. */
{
    int ret, off, isNew;
    const char *toKey;
    Tcl_HashEntry *hPtr;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          tsv::move array key to
     *          $object move to
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }

    toKey = Tcl_GetString(objv[off]);
    hPtr = Tcl_CreateHashEntry(&svObj->arrayPtr->vars, toKey, &isNew);

    if (!isNew) {
        Tcl_AppendResult(interp, "key \"", toKey, "\" exists", NULL);
        goto cmd_err;
    }
    if (svObj->entryPtr) {
        char *key = (char *)Tcl_GetHashKey(&svObj->arrayPtr->vars, svObj->entryPtr);
        if (svObj->arrayPtr->psPtr) {
            PsStore *psPtr = svObj->arrayPtr->psPtr;
            if (psPtr->psDelete(psPtr->psHandle, key) == -1) {
                const char *err = psPtr->psError(psPtr->psHandle);
                Tcl_SetObjResult(interp, Tcl_NewStringObj(err, -1));
                return TCL_ERROR;
            }
        }
        Tcl_DeleteHashEntry(svObj->entryPtr);
    }

    svObj->entryPtr = hPtr;
    Tcl_SetHashValue(hPtr, svObj);

    return Sv_PutContainer(interp, svObj, SV_CHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);

}

/*
 *----------------------------------------------------------------------
 *
 * SvLockObjCmd --
 *
 *    This procedure is invoked to process "tsv::lock" Tcl command.
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
SvLockObjCmd(
             ClientData dummy,                   /* Not used. */
             Tcl_Interp *interp,                 /* Current interpreter. */
             int objc,                           /* Number of arguments. */
             Tcl_Obj *const objv[])              /* Argument objects. */
{
    int ret;
    Tcl_Obj *scriptObj;
    Bucket *bucketPtr;
    Array *arrayPtr = NULL;

    /*
     * Syntax:
     *
     *     tsv::lock array arg ?arg ...?
     */

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "array arg ?arg...?");
        return TCL_ERROR;
    }

    arrayPtr  = LockArray(interp, Tcl_GetString(objv[1]), FLAGS_CREATEARRAY);
    bucketPtr = arrayPtr->bucketPtr;

    /*
     * Evaluate passed arguments as Tcl script. Note that
     * Tcl_EvalObjEx throws away the passed object by
     * doing an decrement reference count on it. This also
     * means we need not build object bytecode rep.
     */

    if (objc == 3) {
        scriptObj = Tcl_DuplicateObj(objv[2]);
    } else {
        scriptObj = Tcl_ConcatObj(objc-2, objv + 2);
    }

    Tcl_AllowExceptions(interp);
    ret = Tcl_EvalObjEx(interp, scriptObj, TCL_EVAL_DIRECT);

    if (ret == TCL_ERROR) {
        char msg[32 + TCL_INTEGER_SPACE];
        /* Next line generates a Deprecation warning when compiled with Tcl 8.6.
         * See Tcl bug #3562640 */
        sprintf(msg, "\n    (\"eval\" body line %d)", Tcl_GetErrorLine(interp));
        Tcl_AddErrorInfo(interp, msg);
    }

    /*
     * We unlock the bucket directly, w/o going to Sv_Unlock()
     * since it needs the array which may be unset by the script.
     */

    UNLOCK_BUCKET(bucketPtr);

    return ret;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvHandlersObjCmd --
 *
 *    This procedure is invoked to process "tsv::handlers" Tcl command.
 *    See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */
static int
SvHandlersObjCmd(
             ClientData arg,                     /* Not used. */
             Tcl_Interp *interp,                 /* Current interpreter. */
             int objc,                           /* Number of arguments. */
             Tcl_Obj *const objv[])              /* Argument objects. */
{
    PsStore *tmpPtr = NULL;

    /*
     * Syntax:
     *
     *     tsv::handlers
     */

    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, NULL);
        return TCL_ERROR;
    }

    Tcl_ResetResult(interp);
    Tcl_MutexLock(&svMutex);
    for (tmpPtr = psStore; tmpPtr; tmpPtr = tmpPtr->nextPtr) {
        Tcl_AppendElement(interp, tmpPtr->type);
    }
    Tcl_MutexUnlock(&svMutex);

    return TCL_OK;
}


/*
 *-----------------------------------------------------------------------------
 *
 * Sv_RegisterStdCommands --
 *
 *      Register standard shared variable commands
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      Memory gets allocated
 *
 *-----------------------------------------------------------------------------
 */

static void
SvRegisterStdCommands(void)
{
    static int initialized = 0;

    if (initialized == 0) {
        Tcl_MutexLock(&initMutex);
        if (initialized == 0) {
            Sv_RegisterCommand("var",      SvObjObjCmd,      NULL, 1);
            Sv_RegisterCommand("object",   SvObjObjCmd,      NULL, 1);
            Sv_RegisterCommand("set",      SvSetObjCmd,      NULL, 0);
            Sv_RegisterCommand("unset",    SvUnsetObjCmd,    NULL, 0);
            Sv_RegisterCommand("get",      SvGetObjCmd,      NULL, 0);
            Sv_RegisterCommand("incr",     SvIncrObjCmd,     NULL, 0);
            Sv_RegisterCommand("exists",   SvExistsObjCmd,   NULL, 0);
            Sv_RegisterCommand("append",   SvAppendObjCmd,   NULL, 0);
            Sv_RegisterCommand("array",    SvArrayObjCmd,    NULL, 0);
            Sv_RegisterCommand("names",    SvNamesObjCmd,    NULL, 0);
            Sv_RegisterCommand("pop",      SvPopObjCmd,      NULL, 0);
            Sv_RegisterCommand("move",     SvMoveObjCmd,     NULL, 0);
            Sv_RegisterCommand("lock",     SvLockObjCmd,     NULL, 0);
            Sv_RegisterCommand("handlers", SvHandlersObjCmd, NULL, 0);
            initialized = 1;
        }
        Tcl_MutexUnlock(&initMutex);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Sv_Init --
 *
 *    Creates commands in current interpreter.
 *
 * Results:
 *    None.
 *
 * Side effects
 *    Many new command created in current interpreter. Global data
 *    structures used by them initialized as well.
 *
 *-----------------------------------------------------------------------------
 */
int
Sv_Init (
    Tcl_Interp *interp
) {
    int i;
    Bucket *bucketPtr;
    SvCmdInfo *cmdPtr;
    Tcl_Obj *obj;

#ifdef SV_FINALIZE
    /*
     * Create exit handler for this thread
     */
    Tcl_CreateThreadExitHandler(SvFinalize, NULL);

    /*
     * Increment number of threads
     */
    Tcl_MutexLock(&nofThreadsMutex);
    ++nofThreads;
    Tcl_MutexUnlock(&nofThreadsMutex);
#endif /* SV_FINALIZE */

    /*
     * Add keyed-list datatype
     */

    TclX_KeyedListInit(interp);
    Sv_RegisterKeylistCommands();

    /*
     * Register standard (nsv_* compatible) and our
     * own extensive set of list manipulating commands
     */

    SvRegisterStdCommands();
    Sv_RegisterListCommands();

    /*
     * Get Tcl object types. These are used
     * in custom object duplicator function.
     */

    obj = Tcl_NewStringObj("no", -1);
    Tcl_GetBooleanFromObj(NULL, obj, &i);
    booleanObjTypePtr   = obj->typePtr;

#ifdef USE_TCL_STUBS
    if (Tcl_GetUnicodeFromObj)
#endif
    {
	Tcl_GetUnicodeFromObj(obj, &i);
	stringObjTypePtr = obj->typePtr;
    }
    Tcl_GetByteArrayFromObj(obj, &i);
    byteArrayObjTypePtr = obj->typePtr;
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewDoubleObj(0.0);
    doubleObjTypePtr    = obj->typePtr;
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewIntObj(0);
    intObjTypePtr       = obj->typePtr;
    Tcl_DecrRefCount(obj);

    obj = Tcl_NewWideIntObj(((Tcl_WideInt)1)<<35);
    wideIntObjTypePtr       = obj->typePtr;
    Tcl_DecrRefCount(obj);

    /*
     * Plug-in registered commands in current interpreter
     */

    for (cmdPtr = svCmdInfo; cmdPtr; cmdPtr = cmdPtr->nextPtr) {
        Tcl_CreateObjCommand(interp, cmdPtr->cmdName, cmdPtr->objProcPtr,
                NULL, (Tcl_CmdDeleteProc*)0);
#ifdef NS_AOLSERVER
        Tcl_CreateObjCommand(interp, cmdPtr->cmdName2, cmdPtr->objProcPtr,
                (ClientData)(size_t)cmdPtr->aolSpecial, (Tcl_CmdDeleteProc*)0);
#endif
    }

    /*
     * Create array of buckets and initialize each bucket
     */

    if (buckets == NULL) {
        Tcl_MutexLock(&bucketsMutex);
        if (buckets == NULL) {
            buckets = (Bucket *)ckalloc(sizeof(Bucket) * NUMBUCKETS);

            for (i = 0; i < NUMBUCKETS; ++i) {
                bucketPtr = &buckets[i];
                memset(bucketPtr, 0, sizeof(Bucket));
                Tcl_InitHashTable(&bucketPtr->arrays, TCL_STRING_KEYS);
                Tcl_InitHashTable(&bucketPtr->handles, TCL_ONE_WORD_KEYS);
            }

            /*
             * There is no other way to get Sv_tclEmptyStringRep
             * pointer value w/o this trick.
             */

            {
                Tcl_Obj *dummy = Tcl_NewObj();
                Sv_tclEmptyStringRep = dummy->bytes;
                Tcl_DecrRefCount(dummy);
            }

            /*
             * Register persistent store handlers
             */
#ifdef HAVE_GDBM
            Sv_RegisterGdbmStore();
#endif
#ifdef HAVE_LMDB
            Sv_RegisterLmdbStore();
#endif
        }
        Tcl_MutexUnlock(&bucketsMutex);
    }

    return TCL_OK;
}

#ifdef SV_FINALIZE
/*
 * Left for reference, but unused since multithreaded finalization is
 * unsolvable in the general case. Brave souls can revive this by
 * installing a late exit handler on Thread's behalf, bringing the
 * function back onto the Tcl_Finalize (but not Tcl_Exit) path.
 */

/*
 *-----------------------------------------------------------------------------
 *
 * SvFinalize --
 *
 *    Unset all arrays and reclaim all buckets.
 *
 * Results:
 *    None.
 *
 * Side effects
 *    Memory gets reclaimed.
 *
 *-----------------------------------------------------------------------------
 */

static void
SvFinalize (ClientData clientData)
{
    int i;
    SvCmdInfo *cmdPtr;
    RegType *regPtr;

    Tcl_HashEntry *hashPtr;
    Tcl_HashSearch search;

    /*
     * Decrement number of threads. Proceed only if I was the last one. The
     * mutex is unlocked at the end of this function, so new threads that might
     * want to register in the meanwhile will find a clean environment when
     * they eventually succeed acquiring nofThreadsMutex.
     */
    Tcl_MutexLock(&nofThreadsMutex);
    if (nofThreads > 1)
    {
        goto done;
    }

    /*
     * Reclaim memory for shared arrays
     */

    if (buckets != NULL) {
        Tcl_MutexLock(&bucketsMutex);
        if (buckets != NULL) {
            for (i = 0; i < NUMBUCKETS; ++i) {
                Bucket *bucketPtr = &buckets[i];
                hashPtr = Tcl_FirstHashEntry(&bucketPtr->arrays, &search);
                while (hashPtr != NULL) {
                    Array *arrayPtr = (Array*)Tcl_GetHashValue(hashPtr);
                    UnlockArray(arrayPtr);
                    /* unbind array before delete (avoid flush of persistent storage) */
                    UnbindArray(NULL, arrayPtr);
                    /* flush, delete etc. */
                    DeleteArray(NULL, arrayPtr);
                    hashPtr = Tcl_NextHashEntry(&search);
                }
                if (bucketPtr->lock) {
                    Sp_RecursiveMutexFinalize(&bucketPtr->lock);
                }
                SvFinalizeContainers(bucketPtr);
                Tcl_DeleteHashTable(&bucketPtr->handles);
                Tcl_DeleteHashTable(&bucketPtr->arrays);
            }
            ckfree((char *)buckets), buckets = NULL;
        }
        buckets = NULL;
        Tcl_MutexUnlock(&bucketsMutex);
    }

    Tcl_MutexLock(&svMutex);

    /*
     * Reclaim memory for registered commands
     */

    if (svCmdInfo != NULL) {
        cmdPtr = svCmdInfo;
        while (cmdPtr) {
            SvCmdInfo *tmpPtr = cmdPtr->nextPtr;
            ckfree((char*)cmdPtr);
            cmdPtr = tmpPtr;
        }
        svCmdInfo = NULL;
    }

    /*
     * Reclaim memory for registered object types
     */

    if (regType != NULL) {
        regPtr = regType;
        while (regPtr) {
            RegType *tmpPtr = regPtr->nextPtr;
            ckfree((char*)regPtr);
            regPtr = tmpPtr;
        }
        regType = NULL;
    }

    Tcl_MutexUnlock(&svMutex);

done:
    --nofThreads;
    Tcl_MutexUnlock(&nofThreadsMutex);
}
#endif /* SV_FINALIZE */

/* EOF $RCSfile: threadSvCmd.c,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */

