/* Threading for AtheOS.
   Based on thread_beos.h. */

#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/atomic.h>
#include <errno.h>
#include <string.h>

/* Missing decl from threads.h */
extern int exit_thread(int);


/* Undefine FASTLOCK to play with simple semaphores. */
#define FASTLOCK


#ifdef FASTLOCK

/* Use an atomic counter and a semaphore for maximum speed. */
typedef struct fastmutex {
	sem_id sem;
	atomic_t count;
} fastmutex_t;


static int fastmutex_create(const char *name, fastmutex_t * mutex);
static int fastmutex_destroy(fastmutex_t * mutex);
static int fastmutex_lock(fastmutex_t * mutex);
static int fastmutex_timedlock(fastmutex_t * mutex, bigtime_t timeout);
static int fastmutex_unlock(fastmutex_t * mutex);


static int fastmutex_create(const char *name, fastmutex_t * mutex)
{
	mutex->count = 0;
	mutex->sem = create_semaphore(name, 0, 0);
	return (mutex->sem < 0) ? -1 : 0;
}


static int fastmutex_destroy(fastmutex_t * mutex)
{
	if (fastmutex_timedlock(mutex, 0) == 0 || errno == EWOULDBLOCK) {
		return delete_semaphore(mutex->sem);
	}
	return 0;
}


static int fastmutex_lock(fastmutex_t * mutex)
{
	atomic_t prev = atomic_add(&mutex->count, 1);
	if (prev > 0)
		return lock_semaphore(mutex->sem);
	return 0;
}


static int fastmutex_timedlock(fastmutex_t * mutex, bigtime_t timeout)
{
	atomic_t prev = atomic_add(&mutex->count, 1);
	if (prev > 0)
		return lock_semaphore_x(mutex->sem, 1, 0, timeout);
	return 0;
}


static int fastmutex_unlock(fastmutex_t * mutex)
{
	atomic_t prev = atomic_add(&mutex->count, -1);
	if (prev > 1)
		return unlock_semaphore(mutex->sem);
	return 0;
}


#endif				/* FASTLOCK */


/*
 * Initialization.
 *
 */
static void PyThread__init_thread(void)
{
	/* Do nothing. */
	return;
}


/*
 * Thread support.
 *
 */

static atomic_t thread_count = 0;

long PyThread_start_new_thread(void (*func) (void *), void *arg)
{
	status_t success = -1;
	thread_id tid;
	char name[OS_NAME_LENGTH];
	atomic_t this_thread;

	dprintf(("PyThread_start_new_thread called\n"));

	this_thread = atomic_add(&thread_count, 1);
	PyOS_snprintf(name, sizeof(name), "python thread (%d)", this_thread);

	tid = spawn_thread(name, func, NORMAL_PRIORITY, 0, arg);
	if (tid < 0) {
		dprintf(("PyThread_start_new_thread spawn_thread failed: %s\n", strerror(errno)));
	} else {
		success = resume_thread(tid);
		if (success < 0) {
			dprintf(("PyThread_start_new_thread resume_thread failed: %s\n", strerror(errno)));
		}
	}

	return (success < 0 ? -1 : tid);
}


long PyThread_get_thread_ident(void)
{
	return get_thread_id(NULL);
}


static void do_PyThread_exit_thread(int no_cleanup)
{
	dprintf(("PyThread_exit_thread called\n"));

	/* Thread-safe way to read a variable without a mutex: */
	if (atomic_add(&thread_count, 0) == 0) {
		/* No threads around, so exit main(). */
		if (no_cleanup)
			_exit(0);
		else
			exit(0);
	} else {
		/* We're a thread */
		exit_thread(0);
	}
}


void PyThread_exit_thread(void)
{
	do_PyThread_exit_thread(0);
}


void PyThread__exit_thread(void)
{
	do_PyThread_exit_thread(1);
}


#ifndef NO_EXIT_PROG
static void do_PyThread_exit_prog(int status, int no_cleanup)
{
	dprintf(("PyThread_exit_prog(%d) called\n", status));

	/* No need to do anything, the threads get torn down if main()exits. */
	if (no_cleanup)
		_exit(status);
	else
		exit(status);
}


void PyThread_exit_prog(int status)
{
	do_PyThread_exit_prog(status, 0);
}


void PyThread__exit_prog(int status)
{
	do_PyThread_exit_prog(status, 1);
}
#endif				/* NO_EXIT_PROG */


/*
 * Lock support.
 *
 */

static atomic_t lock_count = 0;

PyThread_type_lock PyThread_allocate_lock(void)
{
#ifdef FASTLOCK
	fastmutex_t *lock;
#else
	sem_id sema;
#endif
	char name[OS_NAME_LENGTH];
	atomic_t this_lock;

	dprintf(("PyThread_allocate_lock called\n"));

#ifdef FASTLOCK
	lock = (fastmutex_t *) malloc(sizeof(fastmutex_t));
	if (lock == NULL) {
		dprintf(("PyThread_allocate_lock failed: out of memory\n"));
		return (PyThread_type_lock) NULL;
	}
#endif
	this_lock = atomic_add(&lock_count, 1);
	PyOS_snprintf(name, sizeof(name), "python lock (%d)", this_lock);

#ifdef FASTLOCK
	if (fastmutex_create(name, lock) < 0) {
		dprintf(("PyThread_allocate_lock failed: %s\n",
			 strerror(errno)));
		free(lock);
		lock = NULL;
	}
	dprintf(("PyThread_allocate_lock()-> %p\n", lock));
	return (PyThread_type_lock) lock;
#else
	sema = create_semaphore(name, 1, 0);
	if (sema < 0) {
		dprintf(("PyThread_allocate_lock failed: %s\n",
			 strerror(errno)));
		sema = 0;
	}
	dprintf(("PyThread_allocate_lock()-> %p\n", sema));
	return (PyThread_type_lock) sema;
#endif
}


void PyThread_free_lock(PyThread_type_lock lock)
{
	dprintf(("PyThread_free_lock(%p) called\n", lock));

#ifdef FASTLOCK
	if (fastmutex_destroy((fastmutex_t *) lock) < 0) {
		dprintf(("PyThread_free_lock(%p) failed: %s\n", lock,
			 strerror(errno)));
	}
	free(lock);
#else
	if (delete_semaphore((sem_id) lock) < 0) {
		dprintf(("PyThread_free_lock(%p) failed: %s\n", lock,
			 strerror(errno)));
	}
#endif
}


int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
	int retval;

	dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock,
		 waitflag));

#ifdef FASTLOCK
	if (waitflag)
		retval = fastmutex_lock((fastmutex_t *) lock);
	else
		retval = fastmutex_timedlock((fastmutex_t *) lock, 0);
#else
	if (waitflag)
		retval = lock_semaphore((sem_id) lock);
	else
		retval = lock_semaphore_x((sem_id) lock, 1, 0, 0);
#endif
	if (retval < 0) {
		dprintf(("PyThread_acquire_lock(%p, %d) failed: %s\n",
			 lock, waitflag, strerror(errno)));
	}
	dprintf(("PyThread_acquire_lock(%p, %d)-> %d\n", lock, waitflag,
		 retval));
	return retval < 0 ? 0 : 1;
}


void PyThread_release_lock(PyThread_type_lock lock)
{
	dprintf(("PyThread_release_lock(%p) called\n", lock));

#ifdef FASTLOCK
	if (fastmutex_unlock((fastmutex_t *) lock) < 0) {
		dprintf(("PyThread_release_lock(%p) failed: %s\n", lock,
			 strerror(errno)));
	}
#else
	if (unlock_semaphore((sem_id) lock) < 0) {
		dprintf(("PyThread_release_lock(%p) failed: %s\n", lock,
			 strerror(errno)));
	}
#endif
}
