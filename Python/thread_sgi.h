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

#ifdef WITH_SGI_DL
#define USE_DL
#endif

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <ulocks.h>
#include <errno.h>

#define HDR_SIZE	2680	/* sizeof(ushdr_t) */
#define MAXPROC		100	/* max # of threads that can be started */

static usptr_t *shared_arena;
static ulock_t count_lock;	/* protection for some variables */
static ulock_t wait_lock;	/* lock used to wait for other threads */
static int waiting_for_threads;	/* protected by count_lock */
static int nthreads;		/* protected by count_lock */
static int exit_status;
#ifndef NO_EXIT_PROG
static int do_exit;		/* indicates that the program is to exit */
#endif
static int exiting;		/* we're already exiting (for maybe_exit) */
static pid_t my_pid;		/* PID of main thread */
static pid_t pidlist[MAXPROC];	/* PIDs of other threads */
static int maxpidindex;		/* # of PIDs in pidlist */

#ifndef NO_EXIT_PROG
/*
 * This routine is called as a signal handler when another thread
 * exits.  When that happens, we must see whether we have to exit as
 * well (because of an exit_prog()) or whether we should continue on.
 */
static void exit_sig _P0()
{
	d2printf(("exit_sig called\n"));
	if (exiting && getpid() == my_pid) {
		d2printf(("already exiting\n"));
		return;
	}
	if (do_exit) {
		d2printf(("exiting in exit_sig\n"));
#ifdef DEBUG
		if ((thread_debug & 8) == 0)
			thread_debug &= ~1; /* don't produce debug messages */
#endif
		exit_thread();
	}
}

/*
 * This routine is called when a process calls exit().  If that wasn't
 * done from the library, we do as if an exit_prog() was intended.
 */
static void maybe_exit _P0()
{
	dprintf(("maybe_exit called\n"));
	if (exiting) {
		dprintf(("already exiting\n"));
		return;
	}
	exit_prog(0);
}
#endif /* NO_EXIT_PROG */

/*
 * Initialization.
 */
static void _init_thread _P0()
{
#ifndef NO_EXIT_PROG
	struct sigaction s;
#endif /* NO_EXIT_PROG */
#ifdef USE_DL
	long addr, size;
#endif /* USE_DL */


#ifdef USE_DL
	if ((size = usconfig(CONF_INITSIZE, 64*1024)) < 0)
		perror("usconfig - CONF_INITSIZE (check)");
	if (usconfig(CONF_INITSIZE, size) < 0)
		perror("usconfig - CONF_INITSIZE (reset)");
	addr = (long) dl_getrange(size + HDR_SIZE);
	dprintf(("trying to use addr %lx-%lx for shared arena\n", addr, addr+size));
	errno = 0;
	if ((addr = usconfig(CONF_ATTACHADDR, addr)) < 0 && errno != 0)
		perror("usconfig - CONF_ATTACHADDR (set)");
#endif /* USE_DL */
	if (usconfig(CONF_INITUSERS, 16) < 0)
		perror("usconfig - CONF_INITUSERS");
	my_pid = getpid();	/* so that we know which is the main thread */
#ifndef NO_EXIT_PROG
	atexit(maybe_exit);
	s.sa_handler = exit_sig;
	sigemptyset(&s.sa_mask);
	/*sigaddset(&s.sa_mask, SIGUSR1);*/
	s.sa_flags = 0;
	sigaction(SIGUSR1, &s, 0);
	if (prctl(PR_SETEXITSIG, SIGUSR1) < 0)
		perror("prctl - PR_SETEXITSIG");
#endif /* NO_EXIT_PROG */
	if (usconfig(CONF_ARENATYPE, US_SHAREDONLY) < 0)
		perror("usconfig - CONF_ARENATYPE");
	usconfig(CONF_LOCKTYPE, US_DEBUG); /* XXX */
#ifdef DEBUG
	if (thread_debug & 4)
		usconfig(CONF_LOCKTYPE, US_DEBUGPLUS);
	else if (thread_debug & 2)
		usconfig(CONF_LOCKTYPE, US_DEBUG);
#endif /* DEBUG */
	if ((shared_arena = usinit(tmpnam(0))) == 0)
		perror("usinit");
#ifdef USE_DL
	if (usconfig(CONF_ATTACHADDR, addr) < 0) /* reset address */
		perror("usconfig - CONF_ATTACHADDR (reset)");
#endif /* USE_DL */
	if ((count_lock = usnewlock(shared_arena)) == NULL)
		perror("usnewlock (count_lock)");
	(void) usinitlock(count_lock);
	if ((wait_lock = usnewlock(shared_arena)) == NULL)
		perror("usnewlock (wait_lock)");
	dprintf(("arena start: %lx, arena size: %ld\n", (long) shared_arena, (long) usconfig(CONF_GETSIZE, shared_arena)));
}

/*
 * Thread support.
 */

static void clean_threads _P0()
{
	int i;
	pid_t pid;

	/* clean up any exited threads */
	i = 0;
	while (i < maxpidindex) {
		if ((pid = pidlist[i]) > 0) {
			pid = waitpid(pid, 0, WNOHANG);
			if (pid < 0)
				return;
			if (pid != 0) {
				/* a thread has exited */
				pidlist[i] = pidlist[--maxpidindex];
				continue; /* don't increment i */
			}
		}
		i++;
	}
}

int start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
#ifdef USE_DL
	long addr, size;
	static int local_initialized = 0;
#endif /* USE_DL */
	int success = 0;	/* init not needed when SOLARIS_THREADS and */
				/* C_THREADS implemented properly */

	dprintf(("start_new_thread called\n"));
	if (!initialized)
		init_thread();
	switch (ussetlock(count_lock)) {
	case 0: return 0;
	case -1: perror("ussetlock (count_lock)");
	}
	if (maxpidindex >= MAXPROC)
		success = -1;
	else {
#ifdef USE_DL
		if (!local_initialized) {
			if ((size = usconfig(CONF_INITSIZE, 64*1024)) < 0)
				perror("usconfig - CONF_INITSIZE (check)");
			if (usconfig(CONF_INITSIZE, size) < 0)
				perror("usconfig - CONF_INITSIZE (reset)");
			addr = (long) dl_getrange(size + HDR_SIZE);
			dprintf(("trying to use addr %lx-%lx for sproc\n", addr, addr+size));
			errno = 0;
			if ((addr = usconfig(CONF_ATTACHADDR, addr)) < 0 && errno != 0)
				perror("usconfig - CONF_ATTACHADDR (set)");
		}
#endif /* USE_DL */
		clean_threads();
		if ((success = sproc(func, PR_SALL, arg)) < 0)
			perror("sproc");
#ifdef USE_DL
		if (!local_initialized) {
			if (usconfig(CONF_ATTACHADDR, addr) < 0) /* reset address */
				perror("usconfig - CONF_ATTACHADDR (reset)");
			local_initialized = 1;
		}
#endif /* USE_DL */
		if (success >= 0) {
			nthreads++;
			pidlist[maxpidindex++] = success;
			dprintf(("pidlist[%d] = %d\n", maxpidindex-1, success));
		}
	}
	if (usunsetlock(count_lock) < 0)
		perror("usunsetlock (count_lock)");
	return success < 0 ? 0 : 1;
}

long get_thread_ident _P0()
{
	return getpid();
}

static void do_exit_thread _P1(no_cleanup, int no_cleanup)
{
	dprintf(("exit_thread called\n"));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	if (ussetlock(count_lock) < 0)
		perror("ussetlock (count_lock)");
	nthreads--;
	if (getpid() == my_pid) {
		/* main thread; wait for other threads to exit */
		exiting = 1;
#ifndef NO_EXIT_PROG
		if (do_exit) {
			int i;

			/* notify other threads */
			clean_threads();
			if (nthreads >= 0) {
				dprintf(("kill other threads\n"));
				for (i = 0; i < maxpidindex; i++)
					if (pidlist[i] > 0)
						(void) kill(pidlist[i],
							    SIGKILL);
				_exit(exit_status);
			}
		}
#endif /* NO_EXIT_PROG */
		waiting_for_threads = 1;
		if (ussetlock(wait_lock) < 0)
			perror("ussetlock (wait_lock)");
		for (;;) {
			if (nthreads < 0) {
				dprintf(("really exit (%d)\n", exit_status));
				if (no_cleanup)
					_exit(exit_status);
				else
					exit(exit_status);
			}
			if (usunsetlock(count_lock) < 0)
				perror("usunsetlock (count_lock)");
			dprintf(("waiting for other threads (%d)\n", nthreads));
			if (ussetlock(wait_lock) < 0)
				perror("ussetlock (wait_lock)");
			if (ussetlock(count_lock) < 0)
				perror("ussetlock (count_lock)");
		}
	}
	/* not the main thread */
	if (waiting_for_threads) {
		dprintf(("main thread is waiting\n"));
		if (usunsetlock(wait_lock) < 0)
			perror("usunsetlock (wait_lock)");
	}
#ifndef NO_EXIT_PROG
	else if (do_exit)
		(void) kill(my_pid, SIGUSR1);
#endif /* NO_EXIT_PROG */
	if (usunsetlock(count_lock) < 0)
		perror("usunsetlock (count_lock)");
	_exit(0);
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
	do_exit = 1;
	exit_status = status;
	do_exit_thread(no_cleanup);
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
	ulock_t lock;

	dprintf(("allocate_lock called\n"));
	if (!initialized)
		init_thread();

	if ((lock = usnewlock(shared_arena)) == NULL)
		perror("usnewlock");
	(void) usinitlock(lock);
	dprintf(("allocate_lock() -> %lx\n", (long)lock));
	return (type_lock) lock;
}

void free_lock _P1(lock, type_lock lock)
{
	dprintf(("free_lock(%lx) called\n", (long)lock));
	usfreelock((ulock_t) lock, shared_arena);
}

int acquire_lock _P2(lock, type_lock lock, waitflag, int waitflag)
{
	int success;

	dprintf(("acquire_lock(%lx, %d) called\n", (long)lock, waitflag));
	errno = 0;		/* clear it just in case */
	if (waitflag)
		success = ussetlock((ulock_t) lock);
	else
		success = uscsetlock((ulock_t) lock, 1); /* Try it once */
	if (success < 0)
		perror(waitflag ? "ussetlock" : "uscsetlock");
	dprintf(("acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void release_lock _P1(lock, type_lock lock)
{
	dprintf(("release_lock(%lx) called\n", (long)lock));
	if (usunsetlock((ulock_t) lock) < 0)
		perror("usunsetlock");
}

/*
 * Semaphore support.
 */
type_sema allocate_sema _P1(value, int value)
{
	usema_t *sema;
	dprintf(("allocate_sema called\n"));
	if (!initialized)
		init_thread();

	if ((sema = usnewsema(shared_arena, value)) == NULL)
		perror("usnewsema");
	dprintf(("allocate_sema() -> %lx\n", (long) sema));
	return (type_sema) sema;
}

void free_sema _P1(sema, type_sema sema)
{
	dprintf(("free_sema(%lx) called\n", (long) sema));
	usfreesema((usema_t *) sema, shared_arena);
}

void down_sema _P1(sema, type_sema sema)
{
	dprintf(("down_sema(%lx) called\n", (long) sema));
	if (uspsema((usema_t *) sema) < 0)
		perror("uspsema");
	dprintf(("down_sema(%lx) return\n", (long) sema));
}

void up_sema _P1(sema, type_sema sema)
{
	dprintf(("up_sema(%lx)\n", (long) sema));
	if (usvsema((usema_t *) sema) < 0)
		perror("usvsema");
}
