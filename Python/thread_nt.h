
/* This code implemented by Dag.Gruneau@elsa.preseco.comm.se */
/* Fast NonRecursiveMutex support by Yakov Markovitch, markovitch@iso.ru */
/* Eliminated some memory leaks, gsw@agere.com */

#include <windows.h>
#include <limits.h>
#include <process.h>

typedef struct NRMUTEX {
	LONG   owned ;
	DWORD  thread_id ;
	HANDLE hevent ;
} NRMUTEX, *PNRMUTEX ;

typedef PVOID WINAPI interlocked_cmp_xchg_t(PVOID *dest, PVOID exc, PVOID comperand) ;

/* Sorry mate, but we haven't got InterlockedCompareExchange in Win95! */
static PVOID WINAPI interlocked_cmp_xchg(PVOID *dest, PVOID exc, PVOID comperand)
{
	static LONG spinlock = 0 ;
	PVOID result ;
	DWORD dwSleep = 0;

	/* Acqire spinlock (yielding control to other threads if cant aquire for the moment) */
	while(InterlockedExchange(&spinlock, 1))
	{
		// Using Sleep(0) can cause a priority inversion.
		// Sleep(0) only yields the processor if there's
		// another thread of the same priority that's
		// ready to run.  If a high-priority thread is
		// trying to acquire the lock, which is held by
		// a low-priority thread, then the low-priority
		// thread may never get scheduled and hence never
		// free the lock.  NT attempts to avoid priority
		// inversions by temporarily boosting the priority
		// of low-priority runnable threads, but the problem
		// can still occur if there's a medium-priority
		// thread that's always runnable.  If Sleep(1) is used,
		// then the thread unconditionally yields the CPU.  We
		// only do this for the second and subsequent even
		// iterations, since a millisecond is a long time to wait
		// if the thread can be scheduled in again sooner
		// (~100,000 instructions).
		// Avoid priority inversion: 0, 1, 0, 1,...
		Sleep(dwSleep);
		dwSleep = !dwSleep;
	}
	result = *dest ;
	if (result == comperand)
		*dest = exc ;
	/* Release spinlock */
	spinlock = 0 ;
	return result ;
} ;

static interlocked_cmp_xchg_t *ixchg ;
BOOL InitializeNonRecursiveMutex(PNRMUTEX mutex)
{
	if (!ixchg)
	{
		/* Sorely, Win95 has no InterlockedCompareExchange API (Win98 has), so we have to use emulation */
		HANDLE kernel = GetModuleHandle("kernel32.dll") ;
		if (!kernel || (ixchg = (interlocked_cmp_xchg_t *)GetProcAddress(kernel, "InterlockedCompareExchange")) == NULL)
			ixchg = interlocked_cmp_xchg ;
	}

	mutex->owned = -1 ;  /* No threads have entered NonRecursiveMutex */
	mutex->thread_id = 0 ;
	mutex->hevent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
	return mutex->hevent != NULL ;	/* TRUE if the mutex is created */
}

#ifdef InterlockedCompareExchange
#undef InterlockedCompareExchange
#endif
#define InterlockedCompareExchange(dest,exchange,comperand) (ixchg((dest), (exchange), (comperand)))

VOID DeleteNonRecursiveMutex(PNRMUTEX mutex)
{
	/* No in-use check */
	CloseHandle(mutex->hevent) ;
	mutex->hevent = NULL ; /* Just in case */
}

DWORD EnterNonRecursiveMutex(PNRMUTEX mutex, BOOL wait)
{
	/* Assume that the thread waits successfully */
	DWORD ret ;

	/* InterlockedIncrement(&mutex->owned) == 0 means that no thread currently owns the mutex */
	if (!wait)
	{
		if (InterlockedCompareExchange((PVOID *)&mutex->owned, (PVOID)0, (PVOID)-1) != (PVOID)-1)
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

BOOL LeaveNonRecursiveMutex(PNRMUTEX mutex)
{
	/* We don't own the mutex */
	mutex->thread_id = 0 ;
	return
		InterlockedDecrement(&mutex->owned) < 0 ||
		SetEvent(mutex->hevent) ; /* Other threads are waiting, wake one on them up */
}

PNRMUTEX AllocNonRecursiveMutex(void)
{
	PNRMUTEX mutex = (PNRMUTEX)malloc(sizeof(NRMUTEX)) ;
	if (mutex && !InitializeNonRecursiveMutex(mutex))
	{
		free(mutex) ;
		mutex = NULL ;
	}
	return mutex ;
}

void FreeNonRecursiveMutex(PNRMUTEX mutex)
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
static void PyThread__init_thread(void)
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
	unsigned long rv;
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

	rv = _beginthread(bootstrap, 0, &obj); /* use default stack size */
	if (rv == (unsigned long)-1) {
		/* I've seen errno == EAGAIN here, which means "there are
		 * too many threads".
		 */
		dprintf(("%ld: PyThread_start_new_thread failed: %p errno %d\n",
		         PyThread_get_thread_ident(), rv, errno));
		obj.id = -1;
	}
	else {
		dprintf(("%ld: PyThread_start_new_thread succeeded: %p\n",
		         PyThread_get_thread_ident(), rv));
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
long PyThread_get_thread_ident(void)
{
	if (!initialized)
		PyThread_init_thread();

	return GetCurrentThreadId();
}

static void do_PyThread_exit_thread(int no_cleanup)
{
	dprintf(("%ld: PyThread_exit_thread called\n", PyThread_get_thread_ident()));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	_endthread();
}

void PyThread_exit_thread(void)
{
	do_PyThread_exit_thread(0);
}

void PyThread__exit_thread(void)
{
	do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void do_PyThread_exit_prog(int status, int no_cleanup)
{
	dprintf(("PyThread_exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
}

void PyThread_exit_prog(int status)
{
	do_PyThread_exit_prog(status, 0);
}

void PyThread__exit_prog(int status)
{
	do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support. It has too be implemented as semaphores.
 * I [Dag] tried to implement it with mutex but I could find a way to
 * tell whether a thread already own the lock or not.
 */
PyThread_type_lock PyThread_allocate_lock(void)
{
	PNRMUTEX aLock;

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	aLock = AllocNonRecursiveMutex() ;

	dprintf(("%ld: PyThread_allocate_lock() -> %p\n", PyThread_get_thread_ident(), aLock));

	return (PyThread_type_lock) aLock;
}

void PyThread_free_lock(PyThread_type_lock aLock)
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
int PyThread_acquire_lock(PyThread_type_lock aLock, int waitflag)
{
	int success ;

	dprintf(("%ld: PyThread_acquire_lock(%p, %d) called\n", PyThread_get_thread_ident(),aLock, waitflag));

	success = aLock && EnterNonRecursiveMutex((PNRMUTEX) aLock, (waitflag == 1 ? INFINITE : 0)) == WAIT_OBJECT_0 ;

	dprintf(("%ld: PyThread_acquire_lock(%p, %d) -> %d\n", PyThread_get_thread_ident(),aLock, waitflag, success));

	return success;
}

void PyThread_release_lock(PyThread_type_lock aLock)
{
	dprintf(("%ld: PyThread_release_lock(%p) called\n", PyThread_get_thread_ident(),aLock));

	if (!(aLock && LeaveNonRecursiveMutex((PNRMUTEX) aLock)))
		dprintf(("%ld: Could not PyThread_release_lock(%p) error: %l\n", PyThread_get_thread_ident(), aLock, GetLastError()));
}
