
/*
 * Initialization.
 */
static void
PyThread__init_thread(void)
{
}

/*
 * Thread support.
 */
int
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
	int success = 0;	/* init not needed when SOLARIS_THREADS and */
				/* C_THREADS implemented properly */

	dprintf(("PyThread_start_new_thread called\n"));
	if (!initialized)
		PyThread_init_thread();
	return success < 0 ? 0 : 1;
}

long
PyThread_get_thread_ident(void)
{
	if (!initialized)
		PyThread_init_thread();
}

static
void do_PyThread_exit_thread(int no_cleanup)
{
	dprintf(("PyThread_exit_thread called\n"));
	if (!initialized)
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
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

	dprintf(("PyThread_allocate_lock called\n"));
	if (!initialized)
		PyThread_init_thread();

	dprintf(("PyThread_allocate_lock() -> %p\n", lock));
	return (PyThread_type_lock) lock;
}

void
PyThread_free_lock(PyThread_type_lock lock)
{
	dprintf(("PyThread_free_lock(%p) called\n", lock));
}

int
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
	int success;

	dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));
	dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
	return success;
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
	dprintf(("PyThread_release_lock(%p) called\n", lock));
}

/*
 * Semaphore support.
 */
PyThread_type_sema
PyThread_allocate_sema(int value)
{
	dprintf(("PyThread_allocate_sema called\n"));
	if (!initialized)
		PyThread_init_thread();

	dprintf(("PyThread_allocate_sema() -> %p\n",  sema));
	return (PyThread_type_sema) sema;
}

void
PyThread_free_sema(PyThread_type_sema sema)
{
	dprintf(("PyThread_free_sema(%p) called\n",  sema));
}

int
PyThread_down_sema(PyThread_type_sema sema, int waitflag)
{
	dprintf(("PyThread_down_sema(%p, %d) called\n",  sema, waitflag));
	dprintf(("PyThread_down_sema(%p) return\n",  sema));
	return -1;
}

void
PyThread_up_sema(PyThread_type_sema sema)
{
	dprintf(("PyThread_up_sema(%p)\n",  sema));
}
