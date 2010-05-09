
#ifdef MACH_C_THREADS
#include <mach/cthreads.h>
#endif

#ifdef HURD_C_THREADS
#include <cthreads.h>
#endif

/*
 * Initialization.
 */
static void
PyThread__init_thread(void)
{
#ifndef HURD_C_THREADS
    /* Roland McGrath said this should not be used since this is
    done while linking to threads */
    cthread_init();
#else
/* do nothing */
    ;
#endif
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
    /* looks like solaris detaches the thread to never rejoin
     * so well do it here
     */
    cthread_detach(cthread_fork((cthread_fn_t) func, arg));
    return success < 0 ? -1 : 0;
}

long
PyThread_get_thread_ident(void)
{
    if (!initialized)
        PyThread_init_thread();
    return (long) cthread_self();
}

void
PyThread_exit_thread(void)
{
    dprintf(("PyThread_exit_thread called\n"));
    if (!initialized)
        exit(0);
    cthread_exit(0);
}

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
    if (waitflag) {             /* blocking */
        mutex_lock((mutex_t)lock);
        success = TRUE;
    } else {                    /* non blocking */
        success = mutex_try_lock((mutex_t)lock);
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
