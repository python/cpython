#include "thread.h"

#ifdef DEBUG
static int thread_debug = 0;
#define dprintf(args)	(thread_debug && printf args)
#else
#define dprintf(args)
#endif

#ifdef __sgi
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <ulocks.h>

#define MAXPROC		100	/* max # of threads that can be started */

static usptr_t *shared_arena;
static ulock_t count_lock;	/* protection for some variables */
static ulock_t wait_lock;	/* lock used to wait for other threads */
static int waiting_for_threads;	/* protected by count_lock */
static int nthreads;		/* protected by count_lock */
static int exit_status;
static int do_exit;		/* indicates that the program is to exit */
static int exiting;		/* we're already exiting (for maybe_exit) */
static pid_t my_pid;		/* PID of main thread */
static pid_t pidlist[MAXPROC];	/* PIDs of other threads */
static int maxpidindex;		/* # of PIDs in pidlist */
#endif
#ifdef sun
#include <lwp/lwp.h>
#include <lwp/stackdep.h>

#define STACKSIZE	1000	/* stacksize for a thread */
#define NSTACKS		2	/* # stacks to be put in cache initialy */

struct lock {
	int lock_locked;
	cv_t lock_condvar;
	mon_t lock_monitor;
};
#endif
#ifdef C_THREADS
#include <cthreads.h>
#endif

#ifdef __STDC__
#define _P(args)		args
#define _P0()			(void)
#define _P1(v,t)		(t)
#define _P2(v1,t1,v2,t2)	(t1,t2)
#else
#define _P(args)		()
#define _P0()			()
#define _P1(v,t)		(v) t;
#define _P2(v1,t1,v2,t2)	(v1,v2) t1; t2;
#endif

static int initialized;

#ifdef __sgi
/*
 * This routine is called as a signal handler when another thread
 * exits.  When that happens, we must see whether we have to exit as
 * well (because of an exit_prog()) or whether we should continue on.
 */
static void exit_sig _P0()
{
	dprintf(("exit_sig called\n"));
	if (exiting && getpid() == my_pid) {
		dprintf(("already exiting\n"));
		return;
	}
	if (do_exit) {
		dprintf(("exiting in exit_sig\n"));
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
#endif

/*
 * Initialization.
 */
void init_thread _P0()
{
#ifdef __sgi
	struct sigaction s;
#endif

#ifdef DEBUG
	thread_debug = getenv("THREADDEBUG") != 0;
#endif
	if (initialized)
		return;
	initialized = 1;
	dprintf(("init_thread called\n"));

#ifdef __sgi
	if (usconfig(CONF_INITUSERS, 16) < 0)
		perror("usconfig - CONF_INITUSERS");
	my_pid = getpid();	/* so that we know which is the main thread */
	atexit(maybe_exit);
	s.sa_handler = exit_sig;
	sigemptyset(&s.sa_mask);
	/*sigaddset(&s.sa_mask, SIGUSR1);*/
	s.sa_flags = 0;
	sigaction(SIGUSR1, &s, 0);
	if (prctl(PR_SETEXITSIG, SIGUSR1) < 0)
		perror("prctl - PR_SETEXITSIG");
	if (usconfig(CONF_ARENATYPE, US_SHAREDONLY) < 0)
		perror("usconfig - CONF_ARENATYPE");
	/*usconfig(CONF_LOCKTYPE, US_DEBUGPLUS);*/
	if ((shared_arena = usinit(tmpnam(0))) == 0)
		perror("usinit");
	count_lock = usnewlock(shared_arena);
	(void) usinitlock(count_lock);
	wait_lock = usnewlock(shared_arena);
	dprintf(("arena start: %lx, arena size: %ld\n", (long) shared_arena, (long) usconfig(CONF_GETSIZE, shared_arena)));
#ifdef USE_DL			/* for python */
	dl_setrange((long) shared_arena, (long) shared_arena + 64 * 1024);
#endif
#endif
#ifdef sun
	lwp_setstkcache(STACKSIZE, NSTACKS);
#endif
#ifdef C_THREADS
	cthread_init();
#endif
}

/*
 * Thread support.
 */
int start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
#ifdef sun
	thread_t tid;
#endif
	int success = 0;	/* init not needed when SOLARIS and */
				/* C_THREADS implemented properly */

	dprintf(("start_new_thread called\n"));
	if (!initialized)
		init_thread();
#ifdef __sgi
	if (ussetlock(count_lock) == 0)
		return 0;
	if (maxpidindex >= MAXPROC)
		success = -1;
	else {
		success = sproc(func, PR_SALL, arg);
		if (success >= 0) {
			nthreads++;
			pidlist[maxpidindex++] = success;
		}
	}
	(void) usunsetlock(count_lock);
#endif
#ifdef SOLARIS
	(void) thread_create(0, 0, func, arg, THREAD_NEW_LWP);
#endif
#ifdef sun
	success = lwp_create(&tid, func, MINPRIO, 0, lwp_newstk(), 1, arg);
#endif
#ifdef C_THREADS
	(void) cthread_fork(func, arg);
#endif
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
#ifdef __sgi
	(void) ussetlock(count_lock);
	nthreads--;
	if (getpid() == my_pid) {
		/* main thread; wait for other threads to exit */
		exiting = 1;
		if (do_exit) {
			int i;

			/* notify other threads */
			if (nthreads >= 0) {
				dprintf(("kill other threads\n"));
				for (i = 0; i < maxpidindex; i++)
					(void) kill(pidlist[i], SIGKILL);
				_exit(exit_status);
			}
		}
		waiting_for_threads = 1;
		ussetlock(wait_lock);
		for (;;) {
			if (nthreads < 0) {
				dprintf(("really exit (%d)\n", exit_status));
				if (no_cleanup)
					_exit(exit_status);
				else
					exit(exit_status);
			}
			usunsetlock(count_lock);
			dprintf(("waiting for other threads (%d)\n", nthreads));
			ussetlock(wait_lock);
			ussetlock(count_lock);
		}
	}
	/* not the main thread */
	if (waiting_for_threads) {
		dprintf(("main thread is waiting\n"));
		usunsetlock(wait_lock);
	} else if (do_exit)
		(void) kill(my_pid, SIGUSR1);
	(void) usunsetlock(count_lock);
	_exit(0);
#endif
#ifdef SOLARIS
	thread_exit();
#endif
#ifdef sun
	lwp_destroy(SELF);
#endif
#ifdef C_THREADS
	cthread_exit(0);
#endif
}

void exit_thread _P0()
{
	do_exit_thread(0);
}

void _exit_thread _P0()
{
	do_exit_thread(1);
}

static void do_exit_prog _P2(status, int status, no_cleanup, int no_cleanup)
{
	dprintf(("exit_prog(%d) called\n", status));
	if (!initialized)
		if (no_cleanup)
			_exit(status);
		else
			exit(status);
#ifdef __sgi
	do_exit = 1;
	exit_status = status;
	do_exit_thread(no_cleanup);
#endif
#ifdef sun
	pod_exit(status);
#endif
}

void exit_prog _P1(status, int status)
{
	do_exit_prog(status, 0);
}

void _exit_prog _P1(status, int status)
{
	do_exit_prog(status, 1);
}

/*
 * Lock support.
 */
type_lock allocate_lock _P0()
{
#ifdef __sgi
	ulock_t lock;
#endif
#ifdef sun
	struct lock *lock;
	extern char *malloc();
#endif

	dprintf(("allocate_lock called\n"));
	if (!initialized)
		init_thread();

#ifdef __sgi
	lock = usnewlock(shared_arena);
	(void) usinitlock(lock);
#endif
#ifdef sun
	lock = (struct lock *) malloc(sizeof(struct lock));
	lock->lock_locked = 0;
	(void) mon_create(&lock->lock_monitor);
	(void) cv_create(&lock->lock_condvar, lock->lock_monitor);
#endif
	dprintf(("allocate_lock() -> %lx\n", (long)lock));
	return (type_lock) lock;
}

void free_lock _P1(lock, type_lock lock)
{
	dprintf(("free_lock(%lx) called\n", (long)lock));
#ifdef __sgi
	usfreelock((ulock_t) lock, shared_arena);
#endif
#ifdef sun
	mon_destroy(((struct lock *) lock)->lock_monitor);
	free((char *) lock);
#endif
}

int acquire_lock _P2(lock, type_lock lock, waitflag, int waitflag)
{
	int success;

	dprintf(("acquire_lock(%lx, %d) called\n", (long)lock, waitflag));
#ifdef __sgi
	if (waitflag)
		success = ussetlock((ulock_t) lock);
	else
		success = uscsetlock((ulock_t) lock, 1); /* Try it once */
#endif
#ifdef sun
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
#endif
	dprintf(("acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success));
	return success;
}

void release_lock _P1(lock, type_lock lock)
{
	dprintf(("release_lock(%lx) called\n", (long)lock));
#ifdef __sgi
	(void) usunsetlock((ulock_t) lock);
#endif
#ifdef sun
	(void) mon_enter(((struct lock *) lock)->lock_monitor);
	((struct lock *) lock)->lock_locked = 0;
	cv_broadcast(((struct lock *) lock)->lock_condvar);
	mon_exit(((struct lock *) lock)->lock_monitor);
#endif
}

/*
 * Semaphore support.
 */
type_sema allocate_sema _P1(value, int value)
{
#ifdef __sgi
	usema_t *sema;
#endif

	dprintf(("allocate_sema called\n"));
	if (!initialized)
		init_thread();

#ifdef __sgi
	sema = usnewsema(shared_arena, value);
	dprintf(("allocate_sema() -> %lx\n", (long) sema));
	return (type_sema) sema;
#endif
}

void free_sema _P1(sema, type_sema sema)
{
	dprintf(("free_sema(%lx) called\n", (long) sema));
#ifdef __sgi
	usfreesema((usema_t *) sema, shared_arena);
#endif
}

void down_sema _P1(sema, type_sema sema)
{
	dprintf(("down_sema(%lx) called\n", (long) sema));
#ifdef __sgi
	(void) uspsema((usema_t *) sema);
#endif
	dprintf(("down_sema(%lx) return\n", (long) sema));
}

void up_sema _P1(sema, type_sema sema)
{
	dprintf(("up_sema(%lx)\n", (long) sema));
#ifdef __sgi
	(void) usvsema((usema_t *) sema);
#endif
}
