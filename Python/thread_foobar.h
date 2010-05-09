
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
long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    int success = 0;            /* init not needed when SOLARIS_THREADS and */
                /* C_THREADS implemented properly */

    dprintf(("PyThread_start_new_thread called\n"));
    if (!initialized)
        PyThread_init_thread();
    return success < 0 ? -1 : 0;
}

long
PyThread_get_thread_ident(void)
{
    if (!initialized)
        PyThread_init_thread();
}

void
PyThread_exit_thread(void)
{
    dprintf(("PyThread_exit_thread called\n"));
    if (!initialized)
        exit(0);
}

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
