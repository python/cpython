
/* Posix threads interface */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>


/* try to determine what version of the Pthread Standard is installed.
 * this is important, since all sorts of parameter types changed from
 * draft to draft and there are several (incompatible) drafts in
 * common use.  these macros are a start, at least. 
 * 12 May 1997 -- david arnold <davida@pobox.com>
 */

#if defined(__ultrix) && defined(__mips) && defined(_DECTHREADS_)
/* _DECTHREADS_ is defined in cma.h which is included by pthread.h */
#  define PY_PTHREAD_D4

#elif defined(__osf__) && defined (__alpha)
/* _DECTHREADS_ is defined in cma.h which is included by pthread.h */
#  if !defined(_PTHREAD_ENV_ALPHA) || defined(_PTHREAD_USE_D4) || defined(PTHREAD_USE_D4)
#    define PY_PTHREAD_D4
#  else
#    define PY_PTHREAD_STD
#  endif

#elif defined(_AIX)
/* SCHED_BG_NP is defined if using AIX DCE pthreads
 * but it is unsupported by AIX 4 pthreads. Default
 * attributes for AIX 4 pthreads equal to NULL. For
 * AIX DCE pthreads they should be left unchanged.
 */
#  if !defined(SCHED_BG_NP)
#    define PY_PTHREAD_STD
#  else
#    define PY_PTHREAD_D7
#  endif

#elif defined(__DGUX)
#  define PY_PTHREAD_D6

#elif defined(__hpux) && defined(_DECTHREADS_)
#  define PY_PTHREAD_D4

#else /* Default case */
#  define PY_PTHREAD_STD

#endif


/* set default attribute object for different versions */

#if defined(PY_PTHREAD_D4) || defined(PY_PTHREAD_D7)
#  define pthread_attr_default pthread_attr_default
#  define pthread_mutexattr_default pthread_mutexattr_default
#  define pthread_condattr_default pthread_condattr_default
#elif defined(PY_PTHREAD_STD) || defined(PY_PTHREAD_D6)
#  define pthread_attr_default ((pthread_attr_t *)NULL)
#  define pthread_mutexattr_default ((pthread_mutexattr_t *)NULL)
#  define pthread_condattr_default ((pthread_condattr_t *)NULL)
#endif


/* A pthread mutex isn't sufficient to model the Python lock type
 * because, according to Draft 5 of the docs (P1003.4a/D5), both of the
 * following are undefined:
 *  -> a thread tries to lock a mutex it already has locked
 *  -> a thread tries to unlock a mutex locked by a different thread
 * pthread mutexes are designed for serializing threads over short pieces
 * of code anyway, so wouldn't be an appropriate implementation of
 * Python's locks regardless.
 *
 * The pthread_lock struct implements a Python lock as a "locked?" bit
 * and a <condition, mutex> pair.  In general, if the bit can be acquired
 * instantly, it is, else the pair is used to block the thread until the
 * bit is cleared.     9 May 1994 tim@ksr.com
 */

typedef struct {
	char             locked; /* 0=unlocked, 1=locked */
	/* a <cond, mutex> pair to handle an acquire of a locked lock */
	pthread_cond_t   lock_released;
	pthread_mutex_t  mut;
} pthread_lock;

#define CHECK_STATUS(name)  if (status != 0) { perror(name); error = 1; }

/*
 * Initialization.
 */

#ifdef _HAVE_BSDI
static
void _noop(void)
{
}

static void
PyThread__init_thread(void)
{
	/* DO AN INIT BY STARTING THE THREAD */
	static int dummy = 0;
	pthread_t thread1;
	pthread_create(&thread1, NULL, (void *) _noop, &dummy);
	pthread_join(thread1, NULL);
}

#else /* !_HAVE_BSDI */

static void
PyThread__init_thread(void)
{
#if defined(_AIX) && defined(__GNUC__)
	pthread_init();
#endif
}

#endif /* !_HAVE_BSDI */

/*
 * Thread support.
 */


int 
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
	pthread_t th;
	int success;
	dprintf(("PyThread_start_new_thread called\n"));
	if (!initialized)
		PyThread_init_thread();

	success = pthread_create(&th, 
#if defined(PY_PTHREAD_D4)
				 pthread_attr_default,
				 (pthread_startroutine_t)func, 
				 (pthread_addr_t)arg
#elif defined(PY_PTHREAD_D6)
				 pthread_attr_default,
				 (void* (*)(void *))func,
				 arg
#elif defined(PY_PTHREAD_D7)
				 pthread_attr_default,
				 func,
				 arg
#elif defined(PY_PTHREAD_STD)
				 (pthread_attr_t*)NULL,
				 (void* (*)(void *))func,
				 (void *)arg
#endif
				 );

	if (success == 0) {
#if defined(PY_PTHREAD_D4) || defined(PY_PTHREAD_D6) || defined(PY_PTHREAD_D7)
		pthread_detach(&th);
#elif defined(PY_PTHREAD_STD)
		pthread_detach(th);
#endif
	}
	return success != 0 ? 0 : 1;
}

/* XXX This implementation is considered (to quote Tim Peters) "inherently
   hosed" because:
     - It does not guanrantee the promise that a non-zero integer is returned.
     - The cast to long is inherently unsafe.
     - It is not clear that the 'volatile' (for AIX?) and ugly casting in the
       latter return statement (for Alpha OSF/1) are any longer necessary.
*/
long 
PyThread_get_thread_ident(void)
{
	volatile pthread_t threadid;
	if (!initialized)
		PyThread_init_thread();
	/* Jump through some hoops for Alpha OSF/1 */
	threadid = pthread_self();
#if SIZEOF_PTHREAD_T <= SIZEOF_LONG
	return (long) threadid;
#else
	return (long) *(long *) &threadid;
#endif
}

static void 
do_PyThread_exit_thread(int no_cleanup)
{
	dprintf(("PyThread_exit_thread called\n"));
	if (!initialized) {
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	}
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
 * Lock support.
 */
PyThread_type_lock 
PyThread_allocate_lock(void)
{
	pthread_lock *lock;
	int status, error = 0;

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	lock = (pthread_lock *) malloc(sizeof(pthread_lock));
	memset((void *)lock, '\0', sizeof(pthread_lock));
	if (lock) {
		lock->locked = 0;

		status = pthread_mutex_init(&lock->mut,
					    pthread_mutexattr_default);
		CHECK_STATUS("pthread_mutex_init");

		status = pthread_cond_init(&lock->lock_released,
					   pthread_condattr_default);
		CHECK_STATUS("pthread_cond_init");

		if (error) {
			free((void *)lock);
			lock = 0;
		}
	}

	dprintf(("PyThread_allocate_lock() -> %p\n", lock));
	return (PyThread_type_lock) lock;
}

void 
PyThread_free_lock(PyThread_type_lock lock)
{
	pthread_lock *thelock = (pthread_lock *)lock;
	int status, error = 0;

	dprintf(("PyThread_free_lock(%p) called\n", lock));

	status = pthread_mutex_destroy( &thelock->mut );
	CHECK_STATUS("pthread_mutex_destroy");

	status = pthread_cond_destroy( &thelock->lock_released );
	CHECK_STATUS("pthread_cond_destroy");

	free((void *)thelock);
}

int 
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
	int success;
	pthread_lock *thelock = (pthread_lock *)lock;
	int status, error = 0;

	dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));

	status = pthread_mutex_lock( &thelock->mut );
	CHECK_STATUS("pthread_mutex_lock[1]");
	success = thelock->locked == 0;
	if (success) thelock->locked = 1;
	status = pthread_mutex_unlock( &thelock->mut );
	CHECK_STATUS("pthread_mutex_unlock[1]");

	if ( !success && waitflag ) {
		/* continue trying until we get the lock */

		/* mut must be locked by me -- part of the condition
		 * protocol */
		status = pthread_mutex_lock( &thelock->mut );
		CHECK_STATUS("pthread_mutex_lock[2]");
		while ( thelock->locked ) {
			status = pthread_cond_wait(&thelock->lock_released,
						   &thelock->mut);
			CHECK_STATUS("pthread_cond_wait");
		}
		thelock->locked = 1;
		status = pthread_mutex_unlock( &thelock->mut );
		CHECK_STATUS("pthread_mutex_unlock[2]");
		success = 1;
	}
	if (error) success = 0;
	dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
	return success;
}

void 
PyThread_release_lock(PyThread_type_lock lock)
{
	pthread_lock *thelock = (pthread_lock *)lock;
	int status, error = 0;

	dprintf(("PyThread_release_lock(%p) called\n", lock));

	status = pthread_mutex_lock( &thelock->mut );
	CHECK_STATUS("pthread_mutex_lock[3]");

	thelock->locked = 0;

	status = pthread_mutex_unlock( &thelock->mut );
	CHECK_STATUS("pthread_mutex_unlock[3]");

	/* wake up someone (anyone, if any) waiting on the lock */
	status = pthread_cond_signal( &thelock->lock_released );
	CHECK_STATUS("pthread_cond_signal");
}

/*
 * Semaphore support.
 */

struct semaphore {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int value;
};

PyThread_type_sema 
PyThread_allocate_sema(int value)
{
	struct semaphore *sema;
	int status, error = 0;

	dprintf(("PyThread_allocate_sema called\n"));
	if (!initialized)
		PyThread_init_thread();

	sema = (struct semaphore *) malloc(sizeof(struct semaphore));
	if (sema != NULL) {
		sema->value = value;
		status = pthread_mutex_init(&sema->mutex,
					    pthread_mutexattr_default);
		CHECK_STATUS("pthread_mutex_init");
		status = pthread_cond_init(&sema->cond,
					   pthread_condattr_default);
		CHECK_STATUS("pthread_cond_init");
		if (error) {
			free((void *) sema);
			sema = NULL;
		}
	}
	dprintf(("PyThread_allocate_sema() -> %p\n",  sema));
	return (PyThread_type_sema) sema;
}

void 
PyThread_free_sema(PyThread_type_sema sema)
{
	int status, error = 0;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("PyThread_free_sema(%p) called\n",  sema));
	status = pthread_cond_destroy(&thesema->cond);
	CHECK_STATUS("pthread_cond_destroy");
	status = pthread_mutex_destroy(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_destroy");
	free((void *) thesema);
}

int 
PyThread_down_sema(PyThread_type_sema sema, int waitflag)
{
	int status, error = 0, success;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("PyThread_down_sema(%p, %d) called\n",  sema, waitflag));
	status = pthread_mutex_lock(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_lock");
	if (waitflag) {
		while (!error && thesema->value <= 0) {
			status = pthread_cond_wait(&thesema->cond,
						   &thesema->mutex);
			CHECK_STATUS("pthread_cond_wait");
		}
	}
	if (error)
		success = 0;
	else if (thesema->value > 0) {
		thesema->value--;
		success = 1;
	}
	else
		success = 0;
	status = pthread_mutex_unlock(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_unlock");
	dprintf(("PyThread_down_sema(%p) return\n",  sema));
	return success;
}

void 
PyThread_up_sema(PyThread_type_sema sema)
{
	int status, error = 0;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("PyThread_up_sema(%p)\n",  sema));
	status = pthread_mutex_lock(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_lock");
	thesema->value++;
	status = pthread_cond_signal(&thesema->cond);
	CHECK_STATUS("pthread_cond_signal");
	status = pthread_mutex_unlock(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_unlock");
}
