/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Posix threads interface */

#include <stdlib.h>
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

#define CHECK_STATUS(name)  if (status < 0) { perror(name); error=1; }

/*
 * Initialization.
 */
static void _init_thread _P0()
{
}

/*
 * Thread support.
 */


int start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
	pthread_t th;
	int success;
	dprintf(("start_new_thread called\n"));
	if (!initialized)
		init_thread();

	success = pthread_create(&th, 
#if defined(PY_PTHREAD_D4)
				 pthread_attr_default,
				 (pthread_startroutine_t)func, 
				 (pthread_addr_t)arg
#elif defined(PY_PTHREAD_D6)
				 pthread_attr_default,
				 (void* (*)_P((void *)))func,
				 arg
#elif defined(PY_PTHREAD_D7)
				 pthread_attr_default,
				 func,
				 arg
#elif defined(PY_PTHREAD_STD)
				 (pthread_attr_t*)NULL,
				 (void* (*)_P((void *)))func,
				 (void *)arg
#endif
				 );

	if (success >= 0) {
#if defined(PY_PTHREAD_D4) || defined(PY_PTHREAD_D6) || defined(PY_PTHREAD_D7)
		pthread_detach(&th);
#elif defined(PY_PTHREAD_STD)
		pthread_detach(th);
#endif
	}
	return success < 0 ? 0 : 1;
}

long get_thread_ident _P0()
{
	volatile pthread_t threadid;
	if (!initialized)
		init_thread();
	/* Jump through some hoops for Alpha OSF/1 */
	threadid = pthread_self();
	return (long) *(long *) &threadid;
}

static void do_exit_thread _P1(no_cleanup, int no_cleanup)
{
	dprintf(("exit_thread called\n"));
	if (!initialized) {
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	}
}

void exit_thread _P0()
{
	do_exit_thread(0);
}

void _exit_thread _P0()
{
	do_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void do_exit_prog _P2(status, int status, no_cleanup, int no_cleanup)
{
	dprintf(("exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
}

void exit_prog _P1(status, int status)
{
	do_exit_prog(status, 0);
}

void _exit_prog _P1(status, int status)
{
	do_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support.
 */
type_lock allocate_lock _P0()
{
	pthread_lock *lock;
	int status, error = 0;

	dprintf(("allocate_lock called\n"));
	if (!initialized)
		init_thread();

	lock = (pthread_lock *) malloc(sizeof(pthread_lock));
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

	dprintf(("allocate_lock() -> %lx\n", (long)lock));
	return (type_lock) lock;
}

void free_lock _P1(lock, type_lock lock)
{
	pthread_lock *thelock = (pthread_lock *)lock;
	int status, error = 0;

	dprintf(("free_lock(%lx) called\n", (long)lock));

	status = pthread_mutex_destroy( &thelock->mut );
	CHECK_STATUS("pthread_mutex_destroy");

	status = pthread_cond_destroy( &thelock->lock_released );
	CHECK_STATUS("pthread_cond_destroy");

	free((void *)thelock);
}

int acquire_lock _P2(lock, type_lock lock, waitflag, int waitflag)
{
	int success;
	pthread_lock *thelock = (pthread_lock *)lock;
	int status, error = 0;

	dprintf(("acquire_lock(%lx, %d) called\n", (long)lock, waitflag));

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
	dprintf(("acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void release_lock _P1(lock, type_lock lock)
{
	pthread_lock *thelock = (pthread_lock *)lock;
	int status, error = 0;

	dprintf(("release_lock(%lx) called\n", (long)lock));

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

type_sema allocate_sema _P1(value, int value)
{
	struct semaphore *sema;
	int status, error = 0;

	dprintf(("allocate_sema called\n"));
	if (!initialized)
		init_thread();

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
	dprintf(("allocate_sema() -> %lx\n", (long) sema));
	return (type_sema) sema;
}

void free_sema _P1(sema, type_sema sema)
{
	int status, error = 0;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("free_sema(%lx) called\n", (long) sema));
	status = pthread_cond_destroy(&thesema->cond);
	CHECK_STATUS("pthread_cond_destroy");
	status = pthread_mutex_destroy(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_destroy");
	free((void *) thesema);
}

int down_sema _P2(sema, type_sema sema, waitflag, int waitflag)
{
	int status, error = 0, success;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("down_sema(%lx, %d) called\n", (long) sema, waitflag));
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
	dprintf(("down_sema(%lx) return\n", (long) sema));
	return success;
}

void up_sema _P1(sema, type_sema sema)
{
	int status, error = 0;
	struct semaphore *thesema = (struct semaphore *) sema;

	dprintf(("up_sema(%lx)\n", (long) sema));
	status = pthread_mutex_lock(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_lock");
	thesema->value++;
	status = pthread_cond_signal(&thesema->cond);
	CHECK_STATUS("pthread_cond_signal");
	status = pthread_mutex_unlock(&thesema->mutex);
	CHECK_STATUS("pthread_mutex_unlock");
}
