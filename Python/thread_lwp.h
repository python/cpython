
#include <stdlib.h>
#include <lwp/lwp.h>
#include <lwp/stackdep.h>

#define STACKSIZE       1000    /* stacksize for a thread */
#define NSTACKS         2       /* # stacks to be put in cache initially */

struct lock {
    int lock_locked;
    cv_t lock_condvar;
    mon_t lock_monitor;
};


/*
 * Initialization.
 */
static void PyThread__init_thread(void)
{
    lwp_setstkcache(STACKSIZE, NSTACKS);
}

/*
 * Thread support.
 */


long PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    thread_t tid;
    int success;
    dprintf(("PyThread_start_new_thread called\n"));
    if (!initialized)
        PyThread_init_thread();
    success = lwp_create(&tid, func, MINPRIO, 0, lwp_newstk(), 1, arg);
    return success < 0 ? -1 : 0;
}

long PyThread_get_thread_ident(void)
{
    thread_t tid;
    if (!initialized)
        PyThread_init_thread();
    if (lwp_self(&tid) < 0)
        return -1;
    return tid.thread_id;
}

void PyThread_exit_thread(void)
{
    dprintf(("PyThread_exit_thread called\n"));
    if (!initialized)
        exit(0);
    lwp_destroy(SELF);
}

/*
 * Lock support.
 */
PyThread_type_lock PyThread_allocate_lock(void)
{
    struct lock *lock;
    extern char *malloc(size_t);

    dprintf(("PyThread_allocate_lock called\n"));
    if (!initialized)
        PyThread_init_thread();

    lock = (struct lock *) malloc(sizeof(struct lock));
    lock->lock_locked = 0;
    (void) mon_create(&lock->lock_monitor);
    (void) cv_create(&lock->lock_condvar, lock->lock_monitor);
    dprintf(("PyThread_allocate_lock() -> %p\n", lock));
    return (PyThread_type_lock) lock;
}

void PyThread_free_lock(PyThread_type_lock lock)
{
    dprintf(("PyThread_free_lock(%p) called\n", lock));
    mon_destroy(((struct lock *) lock)->lock_monitor);
    free((char *) lock);
}

int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
    int success;

    dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));
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
    dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
    return success;
}

void PyThread_release_lock(PyThread_type_lock lock)
{
    dprintf(("PyThread_release_lock(%p) called\n", lock));
    (void) mon_enter(((struct lock *) lock)->lock_monitor);
    ((struct lock *) lock)->lock_locked = 0;
    cv_broadcast(((struct lock *) lock)->lock_condvar);
    mon_exit(((struct lock *) lock)->lock_monitor);
}
