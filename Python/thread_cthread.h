
#include <mach/cthreads.h>


/*
 * Initialization.
 */
static void
PyThread__init_thread(void)
{
	cthread_init();
}

/*
 * Thread support.
 */
int
PyThread_start_new_thread(func, void (*func)(void *), void *arg)
{
	int success = 0;	/* init not needed when SOLARIS_THREADS and */
				/* C_THREADS implemented properly */

	dprintf(("PyThread_start_new_thread called\n"));
	if (!initialized)
		PyThread_init_thread();
	/* looks like solaris detaches the thread to never rejoin
	 * so well do it here
	 */
	cthread_detach(cthread_fork((cthread_fn_t) func, arg));
	return success < 0 ? 0 : 1;
}

long
PyThread_get_thread_ident(void)
{
	if (!initialized)
		PyThread_init_thread();
	return (long) cthread_self();
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
	cthread_exit(0);
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
static
void do_PyThread_exit_prog(int status, int no_cleanup)
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
	mutex_t lock;

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	lock = mutex_alloc();
	if (mutex_init(lock)) {
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
	mutex_free(lock);
}

int
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
	int success = FALSE;

	dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));
	if (waitflag) { 	/* blocking */
		mutex_lock(lock);
		success = TRUE;
	} else {		/* non blocking */
		success = mutex_try_lock(lock);
	}
	dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
	return success;
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
	dprintf(("PyThread_release_lock(%p) called\n", lock));
	mutex_unlock((mutex_t )lock);
}

/*
 * Semaphore support.
 *
 * This implementation is ripped directly from the pthreads implementation.
 * Which is to say that it is 100% non-functional at this time.
 *
 * Assuming the page is still up, documentation can be found at:
 *
 * http://www.doc.ic.ac.uk/~mac/manuals/solaris-manual-pages/solaris/usr/man/man2/_lwp_sema_wait.2.html
 *
 * Looking at the man page, it seems that one could easily implement a
 * semaphore using a condition.
 *
 */
PyThread_type_sema
PyThread_allocate_sema(int value)
{
	char *sema = 0;
	dprintf(("PyThread_allocate_sema called\n"));
	if (!initialized)
		PyThread_init_thread();

	dprintf(("PyThread_allocate_sema() -> %p\n", sema));
	return (PyThread_type_sema) sema;
}

void
PyThread_free_sema(PyThread_type_sema sema)
{
	dprintf(("PyThread_free_sema(%p) called\n", sema));
}

int
PyThread_down_sema(PyThread_type_sema sema, int waitflag)
{
	dprintf(("PyThread_down_sema(%p, %d) called\n", sema, waitflag));
	dprintf(("PyThread_down_sema(%p) return\n", sema));
	return -1;
}

void
PyThread_up_sema(PyThread_type_sema sema)
{
	dprintf(("PyThread_up_sema(%p)\n", sema));
}
