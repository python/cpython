/*
 * This is the header file for the module that implements some missing
 * synchronization priomitives from the Tcl API.
 *
 * Copyright (c) 2002 by Zoran Vasiljevic.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ---------------------------------------------------------------------------
 */

#ifndef _SP_H_
#define _SP_H_

#include "tclThreadInt.h"

/*
 * The following structure defines a locking bucket. A locking
 * bucket is associated with a mutex and protects access to
 * objects stored in bucket hash table.
 */

typedef struct SpBucket {
    Tcl_Mutex lock;            /* For locking the bucket */
    Tcl_Condition cond;        /* For waiting on threads to release items */
    Tcl_HashTable handles;     /* Hash table of given-out handles in bucket */
} SpBucket;

#define NUMSPBUCKETS 32

/*
 * All types of mutexes share this common part.
 */

typedef struct Sp_AnyMutex_ {
    int lockcount;              /* If !=0 mutex is locked */
    int numlocks;               /* Number of times the mutex got locked */
    Tcl_Mutex lock;             /* Regular mutex */
    Tcl_ThreadId owner;         /* Current lock owner thread (-1 = any) */
} Sp_AnyMutex;

/*
 * Implementation of the exclusive mutex.
 */

typedef struct Sp_ExclusiveMutex_ {
    int lockcount;              /* Flag: 1-locked, 0-not locked */
    int numlocks;               /* Number of times the mutex got locked */
    Tcl_Mutex lock;             /* Regular mutex */
    Tcl_ThreadId owner;         /* Current lock owner thread */
    /* --- */
    Tcl_Mutex mutex;            /* Mutex being locked */
} Sp_ExclusiveMutex_;

typedef Sp_ExclusiveMutex_* Sp_ExclusiveMutex;

/*
 * Implementation of the recursive mutex.
 */

typedef struct Sp_RecursiveMutex_ {
    int lockcount;              /* # of times this mutex is locked */
    int numlocks;               /* Number of time the mutex got locked */
    Tcl_Mutex lock;             /* Regular mutex */
    Tcl_ThreadId owner;         /* Current lock owner thread */
    /* --- */
    Tcl_Condition cond;         /* Wait to be allowed to lock the mutex */
} Sp_RecursiveMutex_;

typedef Sp_RecursiveMutex_* Sp_RecursiveMutex;

/*
 * Implementation of the read/writer mutex.
 */

typedef struct Sp_ReadWriteMutex_ {
    int lockcount;              /* >0: # of readers, -1: sole writer */
    int numlocks;               /* Number of time the mutex got locked */
    Tcl_Mutex lock;             /* Regular mutex */
    Tcl_ThreadId owner;         /* Current lock owner thread */
    /* --- */
    unsigned int numrd;         /* # of readers waiting for lock */
    unsigned int numwr;         /* # of writers waiting for lock */
    Tcl_Condition rcond;        /* Reader lockers wait here */
    Tcl_Condition wcond;        /* Writer lockers wait here */
} Sp_ReadWriteMutex_;

typedef Sp_ReadWriteMutex_* Sp_ReadWriteMutex;


/*
 * API for exclusive mutexes.
 */

MODULE_SCOPE int  Sp_ExclusiveMutexLock(Sp_ExclusiveMutex *mutexPtr);
MODULE_SCOPE int  Sp_ExclusiveMutexIsLocked(Sp_ExclusiveMutex *mutexPtr);
MODULE_SCOPE int  Sp_ExclusiveMutexUnlock(Sp_ExclusiveMutex *mutexPtr);
MODULE_SCOPE void Sp_ExclusiveMutexFinalize(Sp_ExclusiveMutex *mutexPtr);

/*
 * API for recursive mutexes.
 */

MODULE_SCOPE int  Sp_RecursiveMutexLock(Sp_RecursiveMutex *mutexPtr);
MODULE_SCOPE int  Sp_RecursiveMutexIsLocked(Sp_RecursiveMutex *mutexPtr);
MODULE_SCOPE int  Sp_RecursiveMutexUnlock(Sp_RecursiveMutex *mutexPtr);
MODULE_SCOPE void Sp_RecursiveMutexFinalize(Sp_RecursiveMutex *mutexPtr);

/*
 * API for reader/writer mutexes.
 */

MODULE_SCOPE int  Sp_ReadWriteMutexRLock(Sp_ReadWriteMutex *mutexPtr);
MODULE_SCOPE int  Sp_ReadWriteMutexWLock(Sp_ReadWriteMutex *mutexPtr);
MODULE_SCOPE int  Sp_ReadWriteMutexIsLocked(Sp_ReadWriteMutex *mutexPtr);
MODULE_SCOPE int  Sp_ReadWriteMutexUnlock(Sp_ReadWriteMutex *mutexPtr);
MODULE_SCOPE void Sp_ReadWriteMutexFinalize(Sp_ReadWriteMutex *mutexPtr);

#endif /* _SP_H_ */

/* EOF $RCSfile: threadSpCmd.h,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */
