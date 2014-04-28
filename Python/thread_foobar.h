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
    return PyThread_acquire_lock_timed(lock, waitflag ? -1 : 0, 0);
}

PyLockStatus
PyThread_acquire_lock_timed(PyThread_type_lock lock, PY_TIMEOUT_T microseconds,
                            int intr_flag)
{
    int success;

    dprintf(("PyThread_acquire_lock_timed(%p, %lld, %d) called\n", lock, microseconds, intr_flag));
    dprintf(("PyThread_acquire_lock_timed(%p, %lld, %d) -> %d\n",
	     lock, microseconds, intr_flag, success));
    return success;
}

void
PyThread_release_lock(PyThread_type_lock lock)
{
    dprintf(("PyThread_release_lock(%p) called\n", lock));
}

/* The following are only needed if native TLS support exists */
#define Py_HAVE_NATIVE_TLS

#ifdef Py_HAVE_NATIVE_TLS
int
PyThread_create_key(void)
{
    int result;
    return result;
}

void
PyThread_delete_key(int key)
{

}

int
PyThread_set_key_value(int key, void *value)
{
    int ok;

    /* A failure in this case returns -1 */
    if (!ok)
        return -1;
    return 0;
}

void *
PyThread_get_key_value(int key)
{
    void *result;

    return result;
}

void
PyThread_delete_key_value(int key)
{

}

void
PyThread_ReInitTLS(void)
{

}

#endif
