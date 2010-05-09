
/* GNU pth threads interface
   http://www.gnu.org/software/pth
   2000-05-03 Andy Dustman <andy@dustman.net>

   Adapted from Posix threads interface
   12 May 1997 -- david arnold <davida@pobox.com>
 */

#include <stdlib.h>
#include <string.h>
#include <pth.h>

/* A pth mutex isn't sufficient to model the Python lock type
 * because pth mutexes can be acquired multiple times by the
 * same thread.
 *
 * The pth_lock struct implements a Python lock as a "locked?" bit
 * and a <condition, mutex> pair.  In general, if the bit can be acquired
 * instantly, it is, else the pair is used to block the thread until the
 * bit is cleared.
 */

typedef struct {
    char             locked; /* 0=unlocked, 1=locked */
    /* a <cond, mutex> pair to handle an acquire of a locked lock */
    pth_cond_t   lock_released;
    pth_mutex_t  mut;
} pth_lock;

#define CHECK_STATUS(name)  if (status == -1) { printf("%d ", status); perror(name); error = 1; }

pth_attr_t PyThread_attr;

/*
 * Initialization.
 */

static void PyThread__init_thread(void)
{
    pth_init();
    PyThread_attr = pth_attr_new();
    pth_attr_set(PyThread_attr, PTH_ATTR_STACK_SIZE, 1<<18);
    pth_attr_set(PyThread_attr, PTH_ATTR_JOINABLE, FALSE);
}

/*
 * Thread support.
 */


long PyThread_start_new_thread(void (*func)(void *), void *arg)
{
    pth_t th;
    dprintf(("PyThread_start_new_thread called\n"));
    if (!initialized)
        PyThread_init_thread();

    th = pth_spawn(PyThread_attr,
                             (void* (*)(void *))func,
                             (void *)arg
                             );

    return th;
}

long PyThread_get_thread_ident(void)
{
    volatile pth_t threadid;
    if (!initialized)
        PyThread_init_thread();
    /* Jump through some hoops for Alpha OSF/1 */
    threadid = pth_self();
    return (long) *(long *) &threadid;
}

void PyThread_exit_thread(void)
{
    dprintf(("PyThread_exit_thread called\n"));
    if (!initialized) {
        exit(0);
    }
}

/*
 * Lock support.
 */
PyThread_type_lock PyThread_allocate_lock(void)
{
    pth_lock *lock;
    int status, error = 0;

    dprintf(("PyThread_allocate_lock called\n"));
    if (!initialized)
        PyThread_init_thread();

    lock = (pth_lock *) malloc(sizeof(pth_lock));
    memset((void *)lock, '\0', sizeof(pth_lock));
    if (lock) {
        lock->locked = 0;
        status = pth_mutex_init(&lock->mut);
        CHECK_STATUS("pth_mutex_init");
        status = pth_cond_init(&lock->lock_released);
        CHECK_STATUS("pth_cond_init");
        if (error) {
            free((void *)lock);
            lock = NULL;
        }
    }
    dprintf(("PyThread_allocate_lock() -> %p\n", lock));
    return (PyThread_type_lock) lock;
}

void PyThread_free_lock(PyThread_type_lock lock)
{
    pth_lock *thelock = (pth_lock *)lock;

    dprintf(("PyThread_free_lock(%p) called\n", lock));

    free((void *)thelock);
}

int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
    int success;
    pth_lock *thelock = (pth_lock *)lock;
    int status, error = 0;

    dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));

    status = pth_mutex_acquire(&thelock->mut, !waitflag, NULL);
    CHECK_STATUS("pth_mutex_acquire[1]");
    success = thelock->locked == 0;
    if (success) thelock->locked = 1;
    status = pth_mutex_release( &thelock->mut );
    CHECK_STATUS("pth_mutex_release[1]");

    if ( !success && waitflag ) {
        /* continue trying until we get the lock */

        /* mut must be locked by me -- part of the condition
         * protocol */
        status = pth_mutex_acquire( &thelock->mut, !waitflag, NULL );
        CHECK_STATUS("pth_mutex_acquire[2]");
        while ( thelock->locked ) {
            status = pth_cond_await(&thelock->lock_released,
                                    &thelock->mut, NULL);
            CHECK_STATUS("pth_cond_await");
        }
        thelock->locked = 1;
        status = pth_mutex_release( &thelock->mut );
        CHECK_STATUS("pth_mutex_release[2]");
        success = 1;
    }
    if (error) success = 0;
    dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
    return success;
}

void PyThread_release_lock(PyThread_type_lock lock)
{
    pth_lock *thelock = (pth_lock *)lock;
    int status, error = 0;

    dprintf(("PyThread_release_lock(%p) called\n", lock));

    status = pth_mutex_acquire( &thelock->mut, 0, NULL );
    CHECK_STATUS("pth_mutex_acquire[3]");

    thelock->locked = 0;

    status = pth_mutex_release( &thelock->mut );
    CHECK_STATUS("pth_mutex_release[3]");

    /* wake up someone (anyone, if any) waiting on the lock */
    status = pth_cond_notify( &thelock->lock_released, 0 );
    CHECK_STATUS("pth_cond_notify");
}
