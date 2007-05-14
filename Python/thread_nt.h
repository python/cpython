
/* This code implemented by Dag.Gruneau@elsa.preseco.comm.se */
/* Fast NonRecursiveMutex support by Yakov Markovitch, markovitch@iso.ru */
/* Eliminated some memory leaks, gsw@agere.com */

#include <windows.h>
#include <limits.h>
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

typedef struct NRMUTEX {
	LONG   owned ;
	DWORD  thread_id ;
	HANDLE hevent ;
} NRMUTEX, *PNRMUTEX ;


BOOL
InitializeNonRecursiveMutex(PNRMUTEX mutex)
{
	mutex->owned = -1 ;  /* No threads have entered NonRecursiveMutex */
	mutex->thread_id = 0 ;
	mutex->hevent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
	return mutex->hevent != NULL ;	/* TRUE if the mutex is created */
}

VOID
DeleteNonRecursiveMutex(PNRMUTEX mutex)
{
	/* No in-use check */
	CloseHandle(mutex->hevent) ;
	mutex->hevent = NULL ; /* Just in case */
}

DWORD
EnterNonRecursiveMutex(PNRMUTEX mutex, BOOL wait)
{
	/* Assume that the thread waits successfully */
	DWORD ret ;

	/* InterlockedIncrement(&mutex->owned) == 0 means that no thread currently owns the mutex */
	if (!wait)
	{
		if (InterlockedCompareExchange(&mutex->owned, 0, -1) != -1)
			return WAIT_TIMEOUT ;
		ret = WAIT_OBJECT_0 ;
	}
	else
		ret = InterlockedIncrement(&mutex->owned) ?
			/* Some thread owns the mutex, let's wait... */
			WaitForSingleObject(mutex->hevent, INFINITE) : WAIT_OBJECT_0 ;

	mutex->thread_id = GetCurrentThreadId() ; /* We own it */
	return ret ;
}

BOOL
LeaveNonRecursiveMutex(PNRMUTEX mutex)
{
	/* We don't own the mutex */
	mutex->thread_id = 0 ;
	return
		InterlockedDecrement(&mutex->owned) < 0 ||
		SetEvent(mutex->hevent) ; /* Other threads are waiting, wake one on them up */
}

PNRMUTEX
AllocNonRecursiveMutex(void)
{
	PNRMUTEX mutex = (PNRMUTEX)malloc(sizeof(NRMUTEX)) ;
	if (mutex && !InitializeNonRecursiveMutex(mutex))
	{
		free(mutex) ;
		mutex = NULL ;
	}
	return mutex ;
}

void
FreeNonRecursiveMutex(PNRMUTEX mutex)
{
	if (mutex)
	{
		DeleteNonRecursiveMutex(mutex) ;
		free(mutex) ;
	}
}

long PyThread_get_thread_ident(void);

/*
 * Initialization of the C package, should not be needed.
 */
static void
PyThread__init_thread(void)
{
}

/*
 * Thread support.
 */

typedef struct {
	void (*func)(void*);
	void *arg;
	long id;
	HANDLE done;
} callobj;

static int
bootstrap(void *call)
{
	callobj *obj = (callobj*)call;
	/* copy callobj since other thread might free it before we're done */
	void (*func)(void*) = obj->func;
	void *arg = obj->arg;

	obj->id = PyThread_get_thread_ident();
	ReleaseSemaphore(obj->done, 1, NULL);
	func(arg);
	return 0;
}

long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
	Py_uintptr_t rv;
	callobj obj;

	dprintf(("%ld: PyThread_start_new_thread called\n",
		 PyThread_get_thread_ident()));
	if (!initialized)
		PyThread_init_thread();

	obj.id = -1;	/* guilty until proved innocent */
	obj.func = func;
	obj.arg = arg;
	obj.done = CreateSemaphore(NULL, 0, 1, NULL);
	if (obj.done == NULL)
		return -1;

	rv = _beginthread(bootstrap,
			  Py_SAFE_DOWNCAST(_pythread_stacksize,
					   Py_ssize_t, int),
			  &obj);
	if (rv == (Py_uintptr_t)-1) {
		/* I've seen errno == EAGAIN here, which means "there are
		 * too many threads".
		 */
		dprintf(("%ld: PyThread_start_new_thread failed: %p errno %d\n",
		         PyThread_get_thread_ident(), (void*)rv, errno));
		obj.id = -1;
	}
	else {
		dprintf(("%ld: PyThread_start_new_thread succeeded: %p\n",
		         PyThread_get_thread_ident(), (void*)rv));
		/* wait for thread to initialize, so we can get its id */
		WaitForSingleObject(obj.done, INFINITE);
		assert(obj.id != -1);
	}
	CloseHandle((HANDLE)obj.done);
	return obj.id;
}

/*
 * Return the thread Id instead of an handle. The Id is said to uniquely identify the
 * thread in the system
 */
long
PyThread_get_thread_ident(void)
{
	if (!initialized)
		PyThread_init_thread();

	return GetCurrentThreadId();
}

static void
do_PyThread_exit_thread(int no_cleanup)
{
	dprintf(("%ld: PyThread_exit_thread called\n", PyThread_get_thread_ident()));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	_endthread();
}

void
PyThread_exit_thread(void)
{
	do_PyThread_exit_thread(0);
}

void
PyThread__exit_thread(void)
{
	do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void
do_PyThread_exit_prog(int status, int no_cleanup)
{
	dprintf(("PyThread_exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
}

void
PyThread_exit_prog(int status)
{
	do_PyThread_exit_prog(status, 0);
}

void
PyThread__exit_prog(int status)
{
	do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support. It has too be implemented as semaphores.
 * I [Dag] tried to implement it with mutex but I could find a way to
 * tell whether a thread already own the lock or not.
 */
PyThread_type_lock
PyThread_allocate_lock(void)
{
	PNRMUTEX aLock;

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	aLock = AllocNonRecursiveMutex() ;

	dprintf(("%ld: PyThread_allocate_lock() -> %p\n", PyThread_get_thread_ident(), aLock));

	return (PyThread_type_lock) aLock;
}

void
PyThread_free_lock(PyThread_type_lock aLock)
{
	dprintf(("%ld: PyThread_free_lock(%p) called\n", PyThread_get_thread_ident(),aLock));

	FreeNonRecursiveMutex(aLock) ;
}

/*
 * Return 1 on success if the lock was acquired
 *
 * and 0 if the lock was not acquired. This means a 0 is returned
 * if the lock has already been acquired by this thread!
 */
int
PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
	int success ;

	dprintf(("%ld: PyThread_acquire_lock(%p, %d) called\n", PyThread_get_thread_ident(),aLock, waitflag));

	success = aLock && EnterNonRecursiveMutex((PNRMUTEX) aLock, (waitflag ? INFINITE : 0)) == WAIT_OBJECT_0 ;

	dprintf(("%ld: PyThread_acquire_lock(%p, %d) -> %d\n", PyThread_get_thread_ident(),aLock, waitflag, success));

	return success;
}

void
PyThread_release_lock(PyThread_type_lock aLock)
{
	dprintf(("%ld: PyThread_release_lock(%p) called\n", PyThread_get_thread_ident(),aLock));

	if (!(aLock && LeaveNonRecursiveMutex((PNRMUTEX) aLock)))
		dprintf(("%ld: Could not PyThread_release_lock(%p) error: %ld\n", PyThread_get_thread_ident(), aLock, GetLastError()));
}

/* minimum/maximum thread stack sizes supported */
#define THREAD_MIN_STACKSIZE	0x8000		/* 32kB */
#define THREAD_MAX_STACKSIZE	0x10000000	/* 256MB */

/* set the thread stack size.
 * Return 0 if size is valid, -1 otherwise.
 */
static int
_pythread_nt_set_stacksize(size_t size)
{
	/* set to default */
	if (size == 0) {
		_pythread_stacksize = 0;
		return 0;
	}

	/* valid range? */
	if (size >= THREAD_MIN_STACKSIZE && size < THREAD_MAX_STACKSIZE) {
		_pythread_stacksize = size;
		return 0;
	}

	return -1;
}

#define THREAD_SET_STACKSIZE(x)	_pythread_nt_set_stacksize(x)
