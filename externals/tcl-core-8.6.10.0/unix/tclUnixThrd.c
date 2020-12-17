/*
 * tclUnixThrd.c --
 *
 *	This file implements the UNIX-specific thread support.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2008 by George Peter Staplin
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

#ifdef TCL_THREADS

typedef struct ThreadSpecificData {
    char nabuf[16];
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * masterLock is used to serialize creation of mutexes, condition variables,
 * and thread local storage. This is the only place that can count on the
 * ability to statically initialize the mutex.
 */

static pthread_mutex_t masterLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * initLock is used to serialize initialization and finalization of Tcl. It
 * cannot use any dyamically allocated storage.
 */

static pthread_mutex_t initLock = PTHREAD_MUTEX_INITIALIZER;

/*
 * allocLock is used by Tcl's version of malloc for synchronization. For
 * obvious reasons, cannot use any dyamically allocated storage.
 */

static pthread_mutex_t allocLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t *allocLockPtr = &allocLock;

/*
 * These are for the critical sections inside this file.
 */

#define MASTER_LOCK	pthread_mutex_lock(&masterLock)
#define MASTER_UNLOCK	pthread_mutex_unlock(&masterLock)

#endif /* TCL_THREADS */

/*
 *----------------------------------------------------------------------
 *
 * TclpThreadCreate --
 *
 *	This procedure creates a new thread.
 *
 * Results:
 *	TCL_OK if the thread could be created. The thread ID is returned in a
 *	parameter.
 *
 * Side effects:
 *	A new thread is created.
 *
 *----------------------------------------------------------------------
 */

int
TclpThreadCreate(
    Tcl_ThreadId *idPtr,	/* Return, the ID of the thread */
    Tcl_ThreadCreateProc *proc,	/* Main() function of the thread */
    ClientData clientData,	/* The one argument to Main() */
    int stackSize,		/* Size of stack for the new thread */
    int flags)			/* Flags controlling behaviour of the new
				 * thread. */
{
#ifdef TCL_THREADS
    pthread_attr_t attr;
    pthread_t theThread;
    int result;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

#ifdef HAVE_PTHREAD_ATTR_SETSTACKSIZE
    if (stackSize != TCL_THREAD_STACK_DEFAULT) {
	pthread_attr_setstacksize(&attr, (size_t) stackSize);
#ifdef TCL_THREAD_STACK_MIN
    } else {
	/*
	 * Certain systems define a thread stack size that by default is too
	 * small for many operations. The user has the option of defining
	 * TCL_THREAD_STACK_MIN to a value large enough to work for their
	 * needs. This would look like (for 128K min stack):
	 *    make MEM_DEBUG_FLAGS=-DTCL_THREAD_STACK_MIN=131072L
	 *
	 * This solution is not optimal, as we should allow the user to
	 * specify a size at runtime, but we don't want to slow this function
	 * down, and that would still leave the main thread at the default.
	 */

	size_t size;

	result = pthread_attr_getstacksize(&attr, &size);
	if (!result && (size < TCL_THREAD_STACK_MIN)) {
	    pthread_attr_setstacksize(&attr, (size_t) TCL_THREAD_STACK_MIN);
	}
#endif /* TCL_THREAD_STACK_MIN */
    }
#endif /* HAVE_PTHREAD_ATTR_SETSTACKSIZE */

    if (! (flags & TCL_THREAD_JOINABLE)) {
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    }

    if (pthread_create(&theThread, &attr,
	    (void * (*)(void *))proc, (void *)clientData) &&
	    pthread_create(&theThread, NULL,
		    (void * (*)(void *))proc, (void *)clientData)) {
	result = TCL_ERROR;
    } else {
	*idPtr = (Tcl_ThreadId)theThread;
	result = TCL_OK;
    }
    pthread_attr_destroy(&attr);
    return result;
#else
    return TCL_ERROR;
#endif /* TCL_THREADS */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_JoinThread --
 *
 *	This procedure waits upon the exit of the specified thread.
 *
 * Results:
 *	TCL_OK if the wait was successful, TCL_ERROR else.
 *
 * Side effects:
 *	The result area is set to the exit code of the thread we waited upon.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_JoinThread(
    Tcl_ThreadId threadId,	/* Id of the thread to wait upon. */
    int *state)			/* Reference to the storage the result of the
				 * thread we wait upon will be written into.
				 * May be NULL. */
{
#ifdef TCL_THREADS
    int result;
    unsigned long retcode, *retcodePtr = &retcode;

    result = pthread_join((pthread_t) threadId, (void**) retcodePtr);
    if (state) {
	*state = (int) retcode;
    }
    return (result == 0) ? TCL_OK : TCL_ERROR;
#else
    return TCL_ERROR;
#endif
}

#ifdef TCL_THREADS
/*
 *----------------------------------------------------------------------
 *
 * TclpThreadExit --
 *
 *	This procedure terminates the current thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This procedure terminates the current thread.
 *
 *----------------------------------------------------------------------
 */

void
TclpThreadExit(
    int status)
{
    pthread_exit(INT2PTR(status));
}
#endif /* TCL_THREADS */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCurrentThread --
 *
 *	This procedure returns the ID of the currently running thread.
 *
 * Results:
 *	A thread ID.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_ThreadId
Tcl_GetCurrentThread(void)
{
#ifdef TCL_THREADS
    return (Tcl_ThreadId) pthread_self();
#else
    return (Tcl_ThreadId) 0;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclpInitLock
 *
 *	This procedure is used to grab a lock that serializes initialization
 *	and finalization of Tcl. On some platforms this may also initialize
 *	the mutex used to serialize creation of more mutexes and thread local
 *	storage keys.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Acquire the initialization mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpInitLock(void)
{
#ifdef TCL_THREADS
    pthread_mutex_lock(&initLock);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeLock
 *
 *	This procedure is used to destroy all private resources used in this
 *	file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys everything private. TclpInitLock must be held entering this
 *	function.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeLock(void)
{
#ifdef TCL_THREADS
    /*
     * You do not need to destroy mutexes that were created with the
     * PTHREAD_MUTEX_INITIALIZER macro. These mutexes do not need any
     * destruction: masterLock, allocLock, and initLock.
     */

    pthread_mutex_unlock(&initLock);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclpInitUnlock
 *
 *	This procedure is used to release a lock that serializes
 *	initialization and finalization of Tcl.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Release the initialization mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpInitUnlock(void)
{
#ifdef TCL_THREADS
    pthread_mutex_unlock(&initLock);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclpMasterLock
 *
 *	This procedure is used to grab a lock that serializes creation and
 *	finalization of serialization objects. This interface is only needed
 *	in finalization; it is hidden during creation of the objects.
 *
 *	This lock must be different than the initLock because the initLock is
 *	held during creation of synchronization objects.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Acquire the master mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpMasterLock(void)
{
#ifdef TCL_THREADS
    pthread_mutex_lock(&masterLock);
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * TclpMasterUnlock
 *
 *	This procedure is used to release a lock that serializes creation and
 *	finalization of synchronization objects.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Release the master mutex.
 *
 *----------------------------------------------------------------------
 */

void
TclpMasterUnlock(void)
{
#ifdef TCL_THREADS
    pthread_mutex_unlock(&masterLock);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetAllocMutex
 *
 *	This procedure returns a pointer to a statically initialized mutex for
 *	use by the memory allocator. The alloctor must use this lock, because
 *	all other locks are allocated...
 *
 * Results:
 *	A pointer to a mutex that is suitable for passing to Tcl_MutexLock and
 *	Tcl_MutexUnlock.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Mutex *
Tcl_GetAllocMutex(void)
{
#ifdef TCL_THREADS
    pthread_mutex_t **allocLockPtrPtr = &allocLockPtr;
    return (Tcl_Mutex *) allocLockPtrPtr;
#else
    return NULL;
#endif
}

#ifdef TCL_THREADS

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MutexLock --
 *
 *	This procedure is invoked to lock a mutex. This procedure handles
 *	initializing the mutex, if necessary. The caller can rely on the fact
 *	that Tcl_Mutex is an opaque pointer. This routine will change that
 *	pointer from NULL after first use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May block the current thread. The mutex is acquired when this returns.
 *	Will allocate memory for a pthread_mutex_t and initialize this the
 *	first time this Tcl_Mutex is used.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_MutexLock(
    Tcl_Mutex *mutexPtr)	/* Really (pthread_mutex_t **) */
{
    pthread_mutex_t *pmutexPtr;

    if (*mutexPtr == NULL) {
	MASTER_LOCK;
	if (*mutexPtr == NULL) {
	    /*
	     * Double inside master lock check to avoid a race condition.
	     */

	    pmutexPtr = ckalloc(sizeof(pthread_mutex_t));
	    pthread_mutex_init(pmutexPtr, NULL);
	    *mutexPtr = (Tcl_Mutex)pmutexPtr;
	    TclRememberMutex(mutexPtr);
	}
	MASTER_UNLOCK;
    }
    pmutexPtr = *((pthread_mutex_t **)mutexPtr);
    pthread_mutex_lock(pmutexPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MutexUnlock --
 *
 *	This procedure is invoked to unlock a mutex. The mutex must have been
 *	locked by Tcl_MutexLock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mutex is released when this returns.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_MutexUnlock(
    Tcl_Mutex *mutexPtr)	/* Really (pthread_mutex_t **) */
{
    pthread_mutex_t *pmutexPtr = *(pthread_mutex_t **) mutexPtr;

    pthread_mutex_unlock(pmutexPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeMutex --
 *
 *	This procedure is invoked to clean up one mutex. This is only safe to
 *	call at the end of time.
 *
 *	This assumes the Master Lock is held.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mutex list is deallocated.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeMutex(
    Tcl_Mutex *mutexPtr)
{
    pthread_mutex_t *pmutexPtr = *(pthread_mutex_t **) mutexPtr;

    if (pmutexPtr != NULL) {
	pthread_mutex_destroy(pmutexPtr);
	ckfree(pmutexPtr);
	*mutexPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConditionWait --
 *
 *	This procedure is invoked to wait on a condition variable. The mutex
 *	is automically released as part of the wait, and automatically grabbed
 *	when the condition is signaled.
 *
 *	The mutex must be held when this procedure is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May block the current thread. The mutex is acquired when this returns.
 *	Will allocate memory for a pthread_mutex_t and initialize this the
 *	first time this Tcl_Mutex is used.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ConditionWait(
    Tcl_Condition *condPtr,	/* Really (pthread_cond_t **) */
    Tcl_Mutex *mutexPtr,	/* Really (pthread_mutex_t **) */
    const Tcl_Time *timePtr) /* Timeout on waiting period */
{
    pthread_cond_t *pcondPtr;
    pthread_mutex_t *pmutexPtr;
    struct timespec ptime;

    if (*condPtr == NULL) {
	MASTER_LOCK;

	/*
	 * Double check inside mutex to avoid race, then initialize condition
	 * variable if necessary.
	 */

	if (*condPtr == NULL) {
	    pcondPtr = ckalloc(sizeof(pthread_cond_t));
	    pthread_cond_init(pcondPtr, NULL);
	    *condPtr = (Tcl_Condition) pcondPtr;
	    TclRememberCondition(condPtr);
	}
	MASTER_UNLOCK;
    }
    pmutexPtr = *((pthread_mutex_t **)mutexPtr);
    pcondPtr = *((pthread_cond_t **)condPtr);
    if (timePtr == NULL) {
	pthread_cond_wait(pcondPtr, pmutexPtr);
    } else {
	Tcl_Time now;

	/*
	 * Make sure to take into account the microsecond component of the
	 * current time, including possible overflow situations. [Bug #411603]
	 */

	Tcl_GetTime(&now);
	ptime.tv_sec = timePtr->sec + now.sec +
	    (timePtr->usec + now.usec) / 1000000;
	ptime.tv_nsec = 1000 * ((timePtr->usec + now.usec) % 1000000);
	pthread_cond_timedwait(pcondPtr, pmutexPtr, &ptime);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ConditionNotify --
 *
 *	This procedure is invoked to signal a condition variable.
 *
 *	The mutex must be held during this call to avoid races, but this
 *	interface does not enforce that.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May unblock another thread.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_ConditionNotify(
    Tcl_Condition *condPtr)
{
    pthread_cond_t *pcondPtr = *((pthread_cond_t **)condPtr);
    if (pcondPtr != NULL) {
	pthread_cond_broadcast(pcondPtr);
    } else {
	/*
	 * Noone has used the condition variable, so there are no waiters.
	 */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFinalizeCondition --
 *
 *	This procedure is invoked to clean up a condition variable. This is
 *	only safe to call at the end of time.
 *
 *	This assumes the Master Lock is held.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The condition variable is deallocated.
 *
 *----------------------------------------------------------------------
 */

void
TclpFinalizeCondition(
    Tcl_Condition *condPtr)
{
    pthread_cond_t *pcondPtr = *(pthread_cond_t **)condPtr;

    if (pcondPtr != NULL) {
	pthread_cond_destroy(pcondPtr);
	ckfree(pcondPtr);
	*condPtr = NULL;
    }
}
#endif /* TCL_THREADS */

/*
 *----------------------------------------------------------------------
 *
 * TclpReaddir, TclpInetNtoa --
 *
 *	These procedures replace core C versions to be used in a threaded
 *	environment.
 *
 * Results:
 *	See documentation of C functions.
 *
 * Side effects:
 *	See documentation of C functions.
 *
 * Notes:
 *	TclpReaddir is no longer used by the core (see 1095909), but it
 *	appears in the internal stubs table (see #589526).
 *
 *----------------------------------------------------------------------
 */

Tcl_DirEntry *
TclpReaddir(
    TclDIR * dir)
{
    return TclOSreaddir(dir);
}

#undef TclpInetNtoa
char *
TclpInetNtoa(
    struct in_addr addr)
{
#ifdef TCL_THREADS
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    unsigned char *b = (unsigned char*) &addr.s_addr;

    sprintf(tsdPtr->nabuf, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
    return tsdPtr->nabuf;
#else
    return inet_ntoa(addr);
#endif
}

#ifdef TCL_THREADS
/*
 * Additions by AOL for specialized thread memory allocator.
 */

#ifdef USE_THREAD_ALLOC
static volatile int initialized = 0;
static pthread_key_t key;

typedef struct allocMutex {
    Tcl_Mutex tlock;
    pthread_mutex_t plock;
} allocMutex;

Tcl_Mutex *
TclpNewAllocMutex(void)
{
    struct allocMutex *lockPtr;
    register pthread_mutex_t *plockPtr;

    lockPtr = malloc(sizeof(struct allocMutex));
    if (lockPtr == NULL) {
	Tcl_Panic("could not allocate lock");
    }
    plockPtr = &lockPtr->plock;
    lockPtr->tlock = (Tcl_Mutex) plockPtr;
    pthread_mutex_init(&lockPtr->plock, NULL);
    return &lockPtr->tlock;
}

void
TclpFreeAllocMutex(
    Tcl_Mutex *mutex)		/* The alloc mutex to free. */
{
    allocMutex* lockPtr = (allocMutex*) mutex;
    if (!lockPtr) {
	return;
    }
    pthread_mutex_destroy(&lockPtr->plock);
    free(lockPtr);
}

void
TclpFreeAllocCache(
    void *ptr)
{
    if (ptr != NULL) {
	/*
	 * Called by TclFinalizeThreadAllocThread() during the thread
	 * finalization initiated from Tcl_FinalizeThread()
	 */

	TclFreeAllocCache(ptr);
	pthread_setspecific(key, NULL);

    } else if (initialized) {
	/*
	 * Called by TclFinalizeThreadAlloc() during the process
	 * finalization initiated from Tcl_Finalize()
	 */

	pthread_key_delete(key);
	initialized = 0;
    }
}

void *
TclpGetAllocCache(void)
{
    if (!initialized) {
	pthread_mutex_lock(allocLockPtr);
	if (!initialized) {
	    pthread_key_create(&key, NULL);
	    initialized = 1;
	}
	pthread_mutex_unlock(allocLockPtr);
    }
    return pthread_getspecific(key);
}

void
TclpSetAllocCache(
    void *arg)
{
    pthread_setspecific(key, arg);
}
#endif /* USE_THREAD_ALLOC */

void *
TclpThreadCreateKey(void)
{
    pthread_key_t *ptkeyPtr;

    ptkeyPtr = TclpSysAlloc(sizeof *ptkeyPtr, 0);
    if (NULL == ptkeyPtr) {
	Tcl_Panic("unable to allocate thread key!");
    }

    if (pthread_key_create(ptkeyPtr, NULL)) {
	Tcl_Panic("unable to create pthread key!");
    }

    return ptkeyPtr;
}

void
TclpThreadDeleteKey(
    void *keyPtr)
{
    pthread_key_t *ptkeyPtr = keyPtr;

    if (pthread_key_delete(*ptkeyPtr)) {
	Tcl_Panic("unable to delete key!");
    }

    TclpSysFree(keyPtr);
}

void
TclpThreadSetMasterTSD(
    void *tsdKeyPtr,
    void *ptr)
{
    pthread_key_t *ptkeyPtr = tsdKeyPtr;

    if (pthread_setspecific(*ptkeyPtr, ptr)) {
	Tcl_Panic("unable to set master TSD value");
    }
}

void *
TclpThreadGetMasterTSD(
    void *tsdKeyPtr)
{
    pthread_key_t *ptkeyPtr = tsdKeyPtr;

    return pthread_getspecific(*ptkeyPtr);
}

#endif /* TCL_THREADS */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
