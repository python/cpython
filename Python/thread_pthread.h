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
#include "/ufs/guido/src/python/Contrib/pthreads/pthreads/pthread.h"
#define pthread_attr_default ((pthread_attr_t *)0)
#define pthread_mutexattr_default ((pthread_mutexattr_t *)0)
#define pthread_condattr_default ((pthread_condattr_t *)0)
#define TRYLOCK_OFFSET 1
#else /* !FLORIDA_HACKS */
#include <pthread.h>
#define TRYLOCK_OFFSET 0
#endif /* FLORIDA_HACKS */
#include <stdlib.h>

/* A pthread mutex isn't sufficient to model the Python lock type (at
 * least not the way KSR did 'em -- haven't dug thru the docs to verify),
 * because a thread that locks a mutex can't then do a pthread_mutex_lock
 * on it (to wait for another thread to unlock it).
 * In any case, pthread mutexes are designed for serializing threads over
 * short pieces of code, so wouldn't be an appropriate implementation of
 * Python's locks regardless.
 * The pthread_lock struct below implements a Python lock as a pthread
 * mutex and a <condition, mutex> pair.  In general, if the mutex can be
 * be acquired instantly, it is, else the pair is used to block the
 * thread until the mutex is released.  7 May 1994  tim@ksr.com
 */
typedef struct {
	/* the lock */
	pthread_mutex_t  mutex;
	/* a <cond, mutex> pair to handle an acquire of a locked mutex */
	pthread_cond_t   cond;
	pthread_mutex_t  cmutex;
} pthread_lock;

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
	int success = 0;	/* init not needed when SOLARIS_THREADS and */
				/* C_THREADS implemented properly */

	dprintf(("start_new_thread called\n"));
	if (!initialized)
		init_thread();
	success = pthread_create(&th, pthread_attr_default, func, arg);
	return success < 0 ? 0 : 1;
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

	dprintf(("allocate_lock called\n"));
	if (!initialized)
		init_thread();

	lock = (pthread_lock *) malloc(sizeof(pthread_lock));
	{
		int err = 0;
		if ( pthread_mutex_init(&lock->mutex,
				       pthread_mutexattr_default) ) {
			perror("pthread_mutex_init");
			err = 1;
		}
		if ( pthread_cond_init(&lock->cond,
				      pthread_condattr_default) ) {
			perror("pthread_cond_init");
			err = 1;
		} 
		if ( pthread_mutex_init(&lock->cmutex,
					pthread_mutexattr_default)) {
			perror("pthread_mutex_init");
			err = 1;
		}
		if (err) {
			free((void *)lock);
			lock = 0;
		}
	}

	dprintf(("allocate_lock() -> %lx\n", (long)lock));
	return (type_lock) lock;
}

void free_lock _P1(lock, type_lock lock)
{
	dprintf(("free_lock(%lx) called\n", (long)lock));
	if ( pthread_mutex_destroy(&((pthread_lock *)lock)->mutex) )
		perror("pthread_mutex_destroy");
	if ( pthread_cond_destroy(&((pthread_lock *)lock)->cond) )
		perror("pthread_cond_destroy");
	if ( pthread_mutex_destroy(&((pthread_lock *)lock)->cmutex) )
		perror("pthread_mutex_destroy");
	free((void *)lock);
}

int acquire_lock _P2(lock, type_lock lock, waitflag, int waitflag)
{
	int success;

	dprintf(("acquire_lock(%lx, %d) called\n", (long)lock, waitflag));
	{
		pthread_lock *thelock = (pthread_lock *)lock;
		success = TRYLOCK_OFFSET +
			pthread_mutex_trylock( &thelock->mutex );
		if (success < 0) {
			perror("pthread_mutex_trylock [1]");
			success = 0;
		} else if ( success == 0 && waitflag ) {
			/* continue trying until we get the lock */

			/* cmutex must be locked by me -- part of the condition
			 * protocol */
			if ( pthread_mutex_lock( &thelock->cmutex ) )
				perror("pthread_mutex_lock");
			while ( 0 == (success = TRYLOCK_OFFSET +
				      pthread_mutex_trylock(&thelock->mutex)) ) {
				if ( pthread_cond_wait(&thelock->cond,
						       &thelock->cmutex) )
					perror("pthread_cond_wait");
			}
			if (success < 0)
				perror("pthread_mutex_trylock [2]");
			/* now ->mutex & ->cmutex are both locked by me */
			if ( pthread_mutex_unlock( &thelock->cmutex ) )
				perror("pthread_mutex_unlock");
		}
	}
	dprintf(("acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void release_lock _P1(lock, type_lock lock)
{
	dprintf(("release_lock(%lx) called\n", (long)lock));
	{
		pthread_lock *thelock = (pthread_lock *)lock;

		/* tricky:  if the release & signal occur between the
		 *    pthread_mutex_trylock(&thelock->mutex))
		 * and pthread_cond_wait during the acquire, the acquire
		 * will miss the signal it's waiting for; locking cmutex
		 * around the release prevents that
		 */
		if (pthread_mutex_lock( &thelock->cmutex ))
			perror("pthread_mutex_lock");
		if (pthread_mutex_unlock( &thelock->mutex ))
			perror("pthread_mutex_unlock");
		if (pthread_mutex_unlock( &thelock->cmutex ))
			perror("pthread_mutex_unlock");

		/* wake up someone (anyone, if any) waiting on the lock */
		if (pthread_cond_signal( &thelock->cond ))
			perror("pthread_cond_signal");
	}
}

/*
 * Semaphore support.
 */
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
