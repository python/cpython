
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <ulocks.h>
#include <errno.h>

#define HDR_SIZE        2680    /* sizeof(ushdr_t) */
#define MAXPROC         100     /* max # of threads that can be started */

static usptr_t *shared_arena;
static ulock_t count_lock;      /* protection for some variables */
static ulock_t wait_lock;       /* lock used to wait for other threads */
static int waiting_for_threads; /* protected by count_lock */
static int nthreads;            /* protected by count_lock */
static int exit_status;
static int exiting;             /* we're already exiting (for maybe_exit) */
static pid_t my_pid;            /* PID of main thread */
static struct pidlist {
    pid_t parent;
    pid_t child;
} pidlist[MAXPROC];     /* PIDs of other threads; protected by count_lock */
static int maxpidindex;         /* # of PIDs in pidlist */
/*
 * Initialization.
 */
static void PyThread__init_thread(void)
{
#ifdef USE_DL
    long addr, size;
#endif /* USE_DL */


#ifdef USE_DL
    if ((size = usconfig(CONF_INITSIZE, 64*1024)) < 0)
        perror("usconfig - CONF_INITSIZE (check)");
    if (usconfig(CONF_INITSIZE, size) < 0)
        perror("usconfig - CONF_INITSIZE (reset)");
    addr = (long) dl_getrange(size + HDR_SIZE);
    dprintf(("trying to use addr %p-%p for shared arena\n", addr, addr+size));
    errno = 0;
    if ((addr = usconfig(CONF_ATTACHADDR, addr)) < 0 && errno != 0)
        perror("usconfig - CONF_ATTACHADDR (set)");
#endif /* USE_DL */
    if (usconfig(CONF_INITUSERS, 16) < 0)
        perror("usconfig - CONF_INITUSERS");
    my_pid = getpid();          /* so that we know which is the main thread */
    if (usconfig(CONF_ARENATYPE, US_SHAREDONLY) < 0)
        perror("usconfig - CONF_ARENATYPE");
    usconfig(CONF_LOCKTYPE, US_DEBUG); /* XXX */
#ifdef Py_DEBUG
    if (thread_debug & 4)
        usconfig(CONF_LOCKTYPE, US_DEBUGPLUS);
    else if (thread_debug & 2)
        usconfig(CONF_LOCKTYPE, US_DEBUG);
#endif /* Py_DEBUG */
    if ((shared_arena = usinit(tmpnam(0))) == 0)
        perror("usinit");
#ifdef USE_DL
    if (usconfig(CONF_ATTACHADDR, addr) < 0) /* reset address */
        perror("usconfig - CONF_ATTACHADDR (reset)");
#endif /* USE_DL */
    if ((count_lock = usnewlock(shared_arena)) == NULL)
        perror("usnewlock (count_lock)");
    (void) usinitlock(count_lock);
    if ((wait_lock = usnewlock(shared_arena)) == NULL)
        perror("usnewlock (wait_lock)");
    dprintf(("arena start: %p, arena size: %ld\n",  shared_arena, (long) usconfig(CONF_GETSIZE, shared_arena)));
}

/*
 * Thread support.
 */

static void clean_threads(void)
{
    int i, j;
    pid_t mypid, pid;

    /* clean up any exited threads */
    mypid = getpid();
    i = 0;
    while (i < maxpidindex) {
        if (pidlist[i].parent == mypid && (pid = pidlist[i].child) > 0) {
            pid = waitpid(pid, 0, WNOHANG);
            if (pid > 0) {
                /* a thread has exited */
                pidlist[i] = pidlist[--maxpidindex];
                /* remove references to children of dead proc */
                for (j = 0; j < maxpidindex; j++)
                    if (pidlist[j].parent == pid)
                        pidlist[j].child = -1;
                continue; /* don't increment i */
            }
        }
        i++;
    }
    /* clean up the list */
    i = 0;
    while (i < maxpidindex) {
        if (pidlist[i].child == -1) {
            pidlist[i] = pidlist[--maxpidindex];
            continue; /* don't increment i */
        }
        i++;
    }
}

long PyThread_start_new_thread(void (*func)(void *), void *arg)
{
#ifdef USE_DL
    long addr, size;
    static int local_initialized = 0;
#endif /* USE_DL */
    int success = 0;            /* init not needed when SOLARIS_THREADS and */
                /* C_THREADS implemented properly */

    dprintf(("PyThread_start_new_thread called\n"));
    if (!initialized)
        PyThread_init_thread();
    switch (ussetlock(count_lock)) {
    case 0: return 0;
    case -1: perror("ussetlock (count_lock)");
    }
    if (maxpidindex >= MAXPROC)
        success = -1;
    else {
#ifdef USE_DL
        if (!local_initialized) {
            if ((size = usconfig(CONF_INITSIZE, 64*1024)) < 0)
                perror("usconfig - CONF_INITSIZE (check)");
            if (usconfig(CONF_INITSIZE, size) < 0)
                perror("usconfig - CONF_INITSIZE (reset)");
            addr = (long) dl_getrange(size + HDR_SIZE);
            dprintf(("trying to use addr %p-%p for sproc\n",
                     addr, addr+size));
            errno = 0;
            if ((addr = usconfig(CONF_ATTACHADDR, addr)) < 0 &&
                errno != 0)
                perror("usconfig - CONF_ATTACHADDR (set)");
        }
#endif /* USE_DL */
        clean_threads();
        if ((success = sproc(func, PR_SALL, arg)) < 0)
            perror("sproc");
#ifdef USE_DL
        if (!local_initialized) {
            if (usconfig(CONF_ATTACHADDR, addr) < 0)
                /* reset address */
                perror("usconfig - CONF_ATTACHADDR (reset)");
            local_initialized = 1;
        }
#endif /* USE_DL */
        if (success >= 0) {
            nthreads++;
            pidlist[maxpidindex].parent = getpid();
            pidlist[maxpidindex++].child = success;
            dprintf(("pidlist[%d] = %d\n",
                     maxpidindex-1, success));
        }
    }
    if (usunsetlock(count_lock) < 0)
        perror("usunsetlock (count_lock)");
    return success;
}

long PyThread_get_thread_ident(void)
{
    return getpid();
}

void PyThread_exit_thread(void)
{
    dprintf(("PyThread_exit_thread called\n"));
    if (!initialized)
        exit(0);
    if (ussetlock(count_lock) < 0)
        perror("ussetlock (count_lock)");
    nthreads--;
    if (getpid() == my_pid) {
        /* main thread; wait for other threads to exit */
        exiting = 1;
        waiting_for_threads = 1;
        if (ussetlock(wait_lock) < 0)
            perror("ussetlock (wait_lock)");
        for (;;) {
            if (nthreads < 0) {
                dprintf(("really exit (%d)\n", exit_status));
                exit(exit_status);
            }
            if (usunsetlock(count_lock) < 0)
                perror("usunsetlock (count_lock)");
            dprintf(("waiting for other threads (%d)\n", nthreads));
            if (ussetlock(wait_lock) < 0)
                perror("ussetlock (wait_lock)");
            if (ussetlock(count_lock) < 0)
                perror("ussetlock (count_lock)");
        }
    }
    /* not the main thread */
    if (waiting_for_threads) {
        dprintf(("main thread is waiting\n"));
        if (usunsetlock(wait_lock) < 0)
            perror("usunsetlock (wait_lock)");
    }
    if (usunsetlock(count_lock) < 0)
        perror("usunsetlock (count_lock)");
    _exit(0);
}

/*
 * Lock support.
 */
PyThread_type_lock PyThread_allocate_lock(void)
{
    ulock_t lock;

    dprintf(("PyThread_allocate_lock called\n"));
    if (!initialized)
        PyThread_init_thread();

    if ((lock = usnewlock(shared_arena)) == NULL)
        perror("usnewlock");
    (void) usinitlock(lock);
    dprintf(("PyThread_allocate_lock() -> %p\n", lock));
    return (PyThread_type_lock) lock;
}

void PyThread_free_lock(PyThread_type_lock lock)
{
    dprintf(("PyThread_free_lock(%p) called\n", lock));
    usfreelock((ulock_t) lock, shared_arena);
}

int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
    int success;

    dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));
    errno = 0;                  /* clear it just in case */
    if (waitflag)
        success = ussetlock((ulock_t) lock);
    else
        success = uscsetlock((ulock_t) lock, 1); /* Try it once */
    if (success < 0)
        perror(waitflag ? "ussetlock" : "uscsetlock");
    dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
    return success;
}

void PyThread_release_lock(PyThread_type_lock lock)
{
    dprintf(("PyThread_release_lock(%p) called\n", lock));
    if (usunsetlock((ulock_t) lock) < 0)
        perror("usunsetlock");
}
