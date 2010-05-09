
#include <stdlib.h>
#include <stdio.h>
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


long
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    thread_t tid;
    struct func_arg *funcarg;

    dprintf(("PyThread_start_new_thread called\n"));
    if (!initialized)
        PyThread_init_thread();
    funcarg = (struct func_arg *) malloc(sizeof(struct func_arg));
    funcarg->func = func;
    funcarg->arg = arg;
    if (thr_create(0, 0, new_func, funcarg,
                   THR_DETACHED | THR_NEW_LWP, &tid)) {
        perror("thr_create");
        free((void *) funcarg);
        return -1;
    }
    return tid;
}

long
PyThread_get_thread_ident(void)
{
    if (!initialized)
        PyThread_init_thread();
    return thr_self();
}

void
PyThread_exit_thread(void)
{
    dprintf(("PyThread_exit_thread called\n"));
    if (!initialized)
        exit(0);
    thr_exit(0);
}

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
