#include "thread.h"

#ifdef __sgi
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <ulocks.h>

static usptr_t *shared_arena;
static int exit_status;
static int do_exit;
static int exiting;
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

int start_new_thread _P2(func, void (*func) _P((void *)), arg, void *arg)
{
#ifdef sun
	thread_t tid;
#endif
#ifdef DEBUG
	printf("start_new_thread called\n");
#endif
	if (!initialized)
		init_thread();
#ifdef __sgi
	if (sproc(func, PR_SALL, arg) < 0)
		return 0;
	return 1;
#endif
#ifdef SOLARIS
	(void) thread_create(0, 0, func, arg, THREAD_NEW_LWP);
#endif
#ifdef sun
	if (lwp_create(&tid, func, MINPRIO, 0, lwp_newstk(), 1, arg) < 0)
		return 0;
	return 1;
#endif
#ifdef C_THREADS
	(void) cthread_fork(func, arg);
#endif
}

#ifdef __sgi
void maybe_exit _P0()
{
	if (exiting)
		return;
	exit_prog(0);
}
#endif

void exit_thread _P0()
{
#ifdef DEBUG
	printf("exit_thread called\n");
#endif
	if (!initialized)
		exit(0);
#ifdef __sgi
	exiting = 1;
	exit(0);
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

#ifdef __sgi
static void exit_sig _P0()
{
#ifdef DEBUG
	printf("exit_sig called\n");
#endif
	if (do_exit) {
#ifdef DEBUG
		printf("exiting in exit_sig\n");
#endif
		exit(exit_status);
	}
}
#endif

void init_thread _P0()
{
#ifdef __sgi
	struct sigaction s;
#endif

#ifdef DEBUG
	printf("init_thread called\n");
#endif
	initialized = 1;

#ifdef __sgi
	atexit(maybe_exit);
	s.sa_handler = exit_sig;
	sigemptyset(&s.sa_mask);
	sigaddset(&s.sa_mask, SIGUSR1);
	s.sa_flags = 0;
	sigaction(SIGUSR1, &s, 0);
	prctl(PR_SETEXITSIG, SIGUSR1);
	usconfig(CONF_ARENATYPE, US_SHAREDONLY);
	/*usconfig(CONF_LOCKTYPE, US_DEBUGPLUS);*/
	shared_arena = usinit(tmpnam(0));
#endif
#ifdef sun
	lwp_setstkcache(STACKSIZE, NSTACKS);
#endif
#ifdef C_THREADS
	cthread_init();
#endif
}

type_lock allocate_lock _P0()
{
#ifdef __sgi
	ulock_t lock;
#endif
#ifdef sun
	struct lock *lock;
	extern char *malloc();
#endif

#ifdef DEBUG
	printf("allocate_lock called\n");
#endif
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
#ifdef DEBUG
	printf("allocate_lock() -> %lx\n", (long)lock);
#endif
	return (type_lock) lock;
}

void free_lock _P1(lock, type_lock lock)
{
#ifdef DEBUG
	printf("free_lock(%lx) called\n", (long)lock);
#endif
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

#ifdef DEBUG
	printf("acquire_lock(%lx, %d) called\n", (long)lock, waitflag);
#endif
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
#ifdef DEBUG
	printf("acquire_lock(%lx, %d) -> %d\n", (long)lock, waitflag, success);
#endif
	return success;
}

void release_lock _P1(lock, type_lock lock)
{
#ifdef DEBUG
	printf("release lock(%lx) called\n", (long)lock);
#endif
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

void exit_prog _P1(status, int status)
{
#ifdef DEBUG
	printf("exit_prog(%d) called\n", status);
#endif
	if (!initialized)
		exit(status);
#ifdef __sgi
	exiting = 1;
	do_exit = 1;
	exit_status = status;
	exit(status);
#endif
#ifdef sun
	pod_exit(status);
#endif
}
