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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include </usr/include/thread.h>
#undef _POSIX_THREADS


/*
 * Initialization.
 */
static void PyThread__init_thread _P0()
{
}

/*
 * Thread support.
 */
struct func_arg {
	void (*func) _P((void *));
	void *arg;
};

static void *new_func _P1(funcarg, void *funcarg)
{
	void (*func) _P((void *));
	void *arg;

	func = ((struct func_arg *) funcarg)->func;
	arg = ((struct func_arg *) funcarg)->arg;
	free(funcarg);
	(*func)(arg);
	return 0;
}


int PyThread_start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
	struct func_arg *funcarg;
	int success = 0;	/* init not needed when SOLARIS_THREADS and */
				/* C_THREADS implemented properly */

	dprintf(("PyThread_start_new_thread called\n"));
	if (!initialized)
		PyThread_init_thread();
	funcarg = (struct func_arg *) malloc(sizeof(struct func_arg));
	funcarg->func = func;
	funcarg->arg = arg;
	if (thr_create(0, 0, new_func, funcarg, THR_DETACHED, 0)) {
		perror("thr_create");
		free((void *) funcarg);
		success = -1;
	}
	return success < 0 ? 0 : 1;
}

long PyThread_get_thread_ident _P0()
{
	if (!initialized)
		PyThread_init_thread();
	return thr_self();
}

static void do_PyThread_exit_thread _P1(no_cleanup, int no_cleanup)
{
	dprintf(("PyThread_exit_thread called\n"));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	thr_exit(0);
}

void PyThread_exit_thread _P0()
{
	do_PyThread_exit_thread(0);
}

void PyThread__exit_thread _P0()
{
	do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void do_PyThread_exit_prog _P2(status, int status, no_cleanup, int no_cleanup)
{
	dprintf(("PyThread_exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
	if (no_cleanup)
		_exit(status);
	else
		exit(status);
}

void PyThread_exit_prog _P1(status, int status)
{
	do_PyThread_exit_prog(status, 0);
}

void PyThread__exit_prog _P1(status, int status)
{
	do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/*
 * Lock support.
 */
PyThread_type_lock PyThread_allocate_lock _P0()
{
	mutex_t *lock;

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	lock = (mutex_t *) malloc(sizeof(mutex_t));
	if (mutex_init(lock, USYNC_THREAD, 0)) {
		perror("mutex_init");
		free((void *) lock);
		lock = 0;
	}
	dprintf(("PyThread_allocate_lock() -> %lx\n", (long)lock));
	return (PyThread_type_lock) lock;
}

void PyThread_free_lock _P1(lock, PyThread_type_lock lock)
{
	dprintf(("PyThread_free_lock(%lx) called\n", (long)lock));
	mutex_destroy((mutex_t *) lock);
	free((void *) lock);
}

int PyThread_acquire_lock _P2(lock, PyThread_type_lock lock, waitflag, int waitflag)
{
	int success;

	dprintf(("PyThread_acquire_lock(%lx, %d) called\n", (long)lock, waitflag));
	if (waitflag)
		success = mutex_lock((mutex_t *) lock);
	else
		success = mutex_trylock((mutex_t *) lock);
	if (success < 0)
		perror(waitflag ? "mutex_lock" : "mutex_trylock");
	else
		success = !success; /* solaris does it the other way round */
	dprintf(("PyThread_acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void PyThread_release_lock _P1(lock, PyThread_type_lock lock)
{
	dprintf(("PyThread_release_lock(%lx) called\n", (long)lock));
	if (mutex_unlock((mutex_t *) lock))
		perror("mutex_unlock");
}

/*
 * Semaphore support.
 */
PyThread_type_sema PyThread_allocate_sema _P1(value, int value)
{
	sema_t *sema;
	dprintf(("PyThread_allocate_sema called\n"));
	if (!initialized)
		PyThread_init_thread();

	sema = (sema_t *) malloc(sizeof(sema_t));
	if (sema_init(sema, value, USYNC_THREAD, 0)) {
		perror("sema_init");
		free((void *) sema);
		sema = 0;
	}
	dprintf(("PyThread_allocate_sema() -> %lx\n", (long) sema));
	return (PyThread_type_sema) sema;
}

void PyThread_free_sema _P1(sema, PyThread_type_sema sema)
{
	dprintf(("PyThread_free_sema(%lx) called\n", (long) sema));
	if (sema_destroy((sema_t *) sema))
		perror("sema_destroy");
	free((void *) sema);
}

int PyThread_down_sema _P2(sema, PyThread_type_sema sema, waitflag, int waitflag)
{
	int success;

	dprintf(("PyThread_down_sema(%lx) called\n", (long) sema));
	if (waitflag)
		success = sema_wait((sema_t *) sema);
	else
		success = sema_trywait((sema_t *) sema);
	if (success < 0) {
		if (errno == EBUSY)
			success = 0;
		else
			perror("sema_wait");
	}
	else
		success = !success;
	dprintf(("PyThread_down_sema(%lx) return %d\n", (long) sema, success));
	return success;
}

void PyThread_up_sema _P1(sema, PyThread_type_sema sema)
{
	dprintf(("PyThread_up_sema(%lx)\n", (long) sema));
	if (sema_post((sema_t *) sema))
		perror("sema_post");
}
