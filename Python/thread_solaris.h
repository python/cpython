
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include </usr/include/thread.h>
#undef _POSIX_THREADS


/*
 * Initialization.
 */
static void PyThread__init_thread(void)
{
}

/*
 * Thread support.
 */
struct func_arg {
	void (*func)(void *);
	void *arg;
};

static void *
new_func(void *funcarg)
{
	void (*func)(void *);
	void *arg;

	func = ((struct func_arg *) funcarg)->func;
	arg = ((struct func_arg *) funcarg)->arg;
	free(funcarg);
	(*func)(arg);
	return 0;
}


int 
PyThread_start_new_thread(void (*func)(void *), void *arg)
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
	if (thr_create(0, 0, new_func, funcarg,
		       THR_DETACHED | THR_NEW_LWP, 0)) {
		perror("thr_create");
		free((void *) funcarg);
		success = -1;
	}
	return success < 0 ? 0 : 1;
}

long
PyThread_get_thread_ident(void)
{
	if (!initialized)
		PyThread_init_thread();
	return thr_self();
}

static void 
do_PyThread_exit_thread(int no_cleanup)
{
	dprintf(("PyThread_exit_thread called\n"));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	thr_exit(0);
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
	dprintf(("PyThread_allocate_lock() -> %p\n", lock));
	return (PyThread_type_lock) lock;
}

void 
PyThread_free_lock(PyThread_type_lock lock)
{
	dprintf(("PyThread_free_lock(%p) called\n", lock));
	mutex_destroy((mutex_t *) lock);
	free((void *) lock);
}

int 
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
	int success;

	dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));
	if (waitflag)
		success = mutex_lock((mutex_t *) lock);
	else
		success = mutex_trylock((mutex_t *) lock);
	if (success < 0)
		perror(waitflag ? "mutex_lock" : "mutex_trylock");
	else
		success = !success; /* solaris does it the other way round */
	dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
	return success;
}

void 
PyThread_release_lock(PyThread_type_lock lock)
{
	dprintf(("PyThread_release_lock(%p) called\n", lock));
	if (mutex_unlock((mutex_t *) lock))
		perror("mutex_unlock");
}

/*
 * Semaphore support.
 */
PyThread_type_sema 
PyThread_allocate_sema(int value)
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
	dprintf(("PyThread_allocate_sema() -> %p\n",  sema));
	return (PyThread_type_sema) sema;
}

void 
PyThread_free_sema(PyThread_type_sema sema)
{
	dprintf(("PyThread_free_sema(%p) called\n",  sema));
	if (sema_destroy((sema_t *) sema))
		perror("sema_destroy");
	free((void *) sema);
}

int 
PyThread_down_sema(PyThread_type_sema sema, int waitflag)
{
	int success;

	dprintf(("PyThread_down_sema(%p) called\n",  sema));
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
	dprintf(("PyThread_down_sema(%p) return %d\n",  sema, success));
	return success;
}

void 
PyThread_up_sema(PyThread_type_sema sema)
{
	dprintf(("PyThread_up_sema(%p)\n",  sema));
	if (sema_post((sema_t *) sema))
		perror("sema_post");
}
