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

#include <stdlib.h>
#include <lwp/lwp.h>
#include <lwp/stackdep.h>

#define STACKSIZE	1000	/* stacksize for a thread */
#define NSTACKS		2	/* # stacks to be put in cache initialy */

struct lock {
	int lock_locked;
	cv_t lock_condvar;
	mon_t lock_monitor;
};


/*
 * Initialization.
 */
static void _init_thread _P0()
{
	lwp_setstkcache(STACKSIZE, NSTACKS);
}

/*
 * Thread support.
 */


int start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
	thread_t tid;
#if defined(SGI_THREADS) && defined(USE_DL)
	long addr, size;
	static int local_initialized = 0;
#endif /* SGI_THREADS and USE_DL */
	int success = 0;	/* init not needed when SOLARIS_THREADS and */
				/* C_THREADS implemented properly */

	dprintf(("start_new_thread called\n"));
	if (!initialized)
		init_thread();
	success = lwp_create(&tid, func, MINPRIO, 0, lwp_newstk(), 1, arg);
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
	lwp_destroy(SELF);
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
	pod_exit(status);
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
	struct lock *lock;
	extern char *malloc();

	dprintf(("allocate_lock called\n"));
	if (!initialized)
		init_thread();

	lock = (struct lock *) malloc(sizeof(struct lock));
	lock->lock_locked = 0;
	(void) mon_create(&lock->lock_monitor);
	(void) cv_create(&lock->lock_condvar, lock->lock_monitor);
	dprintf(("allocate_lock() -> %lx\n", (long)lock));
	return (type_lock) lock;
}

void free_lock _P1(lock, type_lock lock)
{
	dprintf(("free_lock(%lx) called\n", (long)lock));
	mon_destroy(((struct lock *) lock)->lock_monitor);
	free((char *) lock);
}

int acquire_lock _P2(lock, type_lock lock, waitflag, int waitflag)
{
	int success;

	dprintf(("acquire_lock(%lx, %d) called\n", (long)lock, waitflag));
	success = 0;

	(void) mon_enter(((struct lock *) lock)->lock_monitor);
	if (waitflag)
		while (((struct lock *) lock)->lock_locked)
			cv_wait(((struct lock *) lock)->lock_condvar);
	if (!((struct lock *) lock)->lock_locked) {
		success = 1;
		((struct lock *) lock)->lock_locked = 1;
	}
	cv_broadcast(((struct lock *) lock)->lock_condvar);
	mon_exit(((struct lock *) lock)->lock_monitor);
	dprintf(("acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void release_lock _P1(lock, type_lock lock)
{
	dprintf(("release_lock(%lx) called\n", (long)lock));
	(void) mon_enter(((struct lock *) lock)->lock_monitor);
	((struct lock *) lock)->lock_locked = 0;
	cv_broadcast(((struct lock *) lock)->lock_condvar);
	mon_exit(((struct lock *) lock)->lock_monitor);
}

/*
 * Semaphore support.
 */
type_sema allocate_sema _P1(value, int value)
{
	type_sema sema = 0;
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
