/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#ifdef sun
#define FLORIDA_HACKS
#endif

#ifdef FLORIDA_HACKS
/* Hacks for Florida State Posix threads implementation */
#undef _POSIX_THREADS
#include "/ufs/guido/src/python/Contrib/pthreads/src/pthread.h"
#define pthread_attr_default ((pthread_attr_t *)0)
#define pthread_mutexattr_default ((pthread_mutexattr_t *)0)
#define pthread_condattr_default ((pthread_condattr_t *)0)
#define TRYLOCK_OFFSET 1
#else /* !FLORIDA_HACKS */
#include <pthread.h>
#define TRYLOCK_OFFSET 0
#endif /* FLORIDA_HACKS */
#include <stdlib.h>

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
#if defined(SGI_THREADS) && defined(USE_DL)
	long addr, size;
	static int local_initialized = 0;
#endif /* SGI_THREADS and USE_DL */
	pthread_t th;
	int success;
	dprintf(("start_new_thread called\n"));
	if (!initialized)
		init_thread();
	success = pthread_create(&th, pthread_attr_default, func, arg);
	return success < 0 ? 0 : 1;
}

long get_thread_ident _P0()
{
	if (!initialized)
		init_thread();
	return (long) pthread_self();
}

static void do_exit_thread _P1(no_cleanup, int no_cleanup)
{
	dprintf(("exit_thread called\n"));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
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
/* NOTE: 100% non-functional at this time - tim */

type_sema allocate_sema _P1(value, int value)
{
	char *sema = 0;
	dprintf(("allocate_sema called\n"));
	if (!initialized)
		init_thread();

	dprintf(("allocate_sema() -> %lx\n", (long) sema));
	return (type_sema) sema;
}

void free_sema _P1(sema, type_sema sema)
{
	dprintf(("free_sema(%lx) called\n", (long) sema));
}

void down_sema _P1(sema, type_sema sema)
{
	dprintf(("down_sema(%lx) called\n", (long) sema));
	dprintf(("down_sema(%lx) return\n", (long) sema));
}

void up_sema _P1(sema, type_sema sema)
{
	dprintf(("up_sema(%lx)\n", (long) sema));
}
