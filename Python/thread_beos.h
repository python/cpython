#include <kernel/OS.h>
#include <support/SupportDefs.h>
#include <errno.h>

/* ----------------------------------------------------------------------
 * Fast locking mechanism described by Benoit Schillings (benoit@be.com)
 * in the Be Developer's Newsletter, Issue #26 (http://www.be.com/).
 */
typedef struct benaphore {
	sem_id _sem;
	int32  _atom;
} benaphore_t;

static status_t benaphore_create( const char *name, benaphore_t *ben );
static status_t benaphore_destroy( benaphore_t *ben );
static status_t benaphore_lock( benaphore_t *ben );
static status_t benaphore_timedlock( benaphore_t *ben, bigtime_t micros );
static status_t benaphore_unlock( benaphore_t *ben );

static status_t benaphore_create( const char *name, benaphore_t *ben )
{
	if( ben != NULL ) {
		ben->_atom = 0;
		ben->_sem = create_sem( 0, name );
		
		if( ben->_sem < B_NO_ERROR ) {
			return B_BAD_SEM_ID;
		}
	} else {
		return EFAULT;
	}
	
	return EOK;
}

static status_t benaphore_destroy( benaphore_t *ben )
{
	if( ben->_sem >= B_NO_ERROR ) {
		status_t retval = benaphore_timedlock( ben, 0 );
		
		if( retval == EOK || retval == EWOULDBLOCK ) {
			status_t del_retval = delete_sem( ben->_sem );
			
			return del_retval;
		}
	}

	return B_BAD_SEM_ID;
}

static status_t benaphore_lock( benaphore_t *ben )
{
	int32 prev = atomic_add( &(ben->_atom), 1 );
	
	if( prev > 0 ) {
		return acquire_sem( ben->_sem );
	}
	
	return EOK;
}

static status_t benaphore_timedlock( benaphore_t *ben, bigtime_t micros )
{
	int32 prev = atomic_add( &(ben->_atom), 1 );
	
	if( prev > 0 ) {
		status_t retval = acquire_sem_etc( ben->_sem, 1, B_TIMEOUT, micros );
		
		switch( retval ) {
		case B_WOULD_BLOCK:	/* Fall through... */
		case B_TIMED_OUT:
			return EWOULDBLOCK;
			break;
		case B_OK:
			return EOK;
			break;
		default:
			return retval;
			break;
		}
	}
	
	return EOK;
}

static status_t benaphore_unlock( benaphore_t *ben )
{
	int32 prev = atomic_add( &(ben->_atom), -1 );
	
	if( prev > 1 ) {
		return release_sem( ben->_sem );
	}
	
	return EOK;
}

/* ----------------------------------------------------------------------
 * Initialization.
 */
static void PyThread__init_thread( void )
{
	/* Do nothing. */
	return;
}

/* ----------------------------------------------------------------------
 * Thread support.
 *
 * Only ANSI C, renamed functions here; you can't use K&R on BeOS,
 * and there's no legacy thread module to support.
 */

static int32 thread_count = 0;

long PyThread_start_new_thread( void (*func)(void *), void *arg )
{
	status_t success = 0;
	thread_id tid;
	char name[B_OS_NAME_LENGTH];
	int32 this_thread;

	dprintf(("PyThread_start_new_thread called\n"));

	/* We are so very thread-safe... */
	this_thread = atomic_add( &thread_count, 1 );
	PyOS_snprintf(name, sizeof(name),
		      "python thread (%d)", this_thread );

	tid = spawn_thread( (thread_func)func, name,
	                    B_NORMAL_PRIORITY, arg );
	if( tid > B_NO_ERROR ) {
		success = resume_thread( tid );
	}

	return ( success == B_NO_ERROR ? tid : -1 );
}

long PyThread_get_thread_ident( void )
{
	/* Presumed to return the current thread's ID... */
	thread_id tid;
	tid = find_thread( NULL );
	
	return ( tid != B_NAME_NOT_FOUND ? tid : -1 );
}

static void do_PyThread_exit_thread( int no_cleanup )
{
	int32 threads;

	dprintf(("PyThread_exit_thread called\n"));

	/* Thread-safe way to read a variable without a mutex: */
	threads = atomic_add( &thread_count, 0 );

	if( threads == 0 ) {
		/* No threads around, so exit main(). */
		if( no_cleanup ) {
			_exit(0);
		} else {
			exit(0);
		}
	} else {
		/* Oh, we're a thread, let's try to exit gracefully... */
		exit_thread( B_NO_ERROR );
	}
}

void PyThread_exit_thread( void )
{
	do_PyThread_exit_thread(0);
}

void PyThread__exit_thread( void )
{
	do_PyThread_exit_thread(1);
}

#ifndef NO_EXIT_PROG
static void do_PyThread_exit_prog( int status, int no_cleanup )
{
	dprintf(("PyThread_exit_prog(%d) called\n", status));

	/* No need to do anything, the threads get torn down if main() exits. */

	if (no_cleanup) {
		_exit(status);
	} else {
		exit(status);
	}
}

void PyThread_exit_prog( int status )
{
	do_PyThread_exit_prog(status, 0);
}

void PyThread__exit_prog( int status )
{
	do_PyThread_exit_prog(status, 1);
}
#endif /* NO_EXIT_PROG */

/* ----------------------------------------------------------------------
 * Lock support.
 */

static int32 lock_count = 0;

PyThread_type_lock PyThread_allocate_lock( void )
{
	benaphore_t *lock;
	status_t retval;
	char name[B_OS_NAME_LENGTH];
	int32 this_lock;
	
	dprintf(("PyThread_allocate_lock called\n"));

	lock = (benaphore_t *)malloc( sizeof( benaphore_t ) );
	if( lock == NULL ) {
		/* TODO: that's bad, raise MemoryError */
		return (PyThread_type_lock)NULL;
	}

	this_lock = atomic_add( &lock_count, 1 );
	PyOS_snprintf(name, sizeof(name), "python lock (%d)", this_lock);

	retval = benaphore_create( name, lock );
	if( retval != EOK ) {
		/* TODO: that's bad, raise an exception */
		return (PyThread_type_lock)NULL;
	}

	dprintf(("PyThread_allocate_lock() -> %p\n", lock));
	return (PyThread_type_lock) lock;
}

void PyThread_free_lock( PyThread_type_lock lock )
{
	status_t retval;

	dprintf(("PyThread_free_lock(%p) called\n", lock));
	
	retval = benaphore_destroy( (benaphore_t *)lock );
	if( retval != EOK ) {
		/* TODO: that's bad, raise an exception */
		return;
	}
}

int PyThread_acquire_lock( PyThread_type_lock lock, int waitflag )
{
	int success;
	status_t retval;

	dprintf(("PyThread_acquire_lock(%p, %d) called\n", lock, waitflag));

	if( waitflag ) {
		retval = benaphore_lock( (benaphore_t *)lock );
	} else {
		retval = benaphore_timedlock( (benaphore_t *)lock, 0 );
	}
	
	if( retval == EOK ) {
		success = 1;
	} else {
		success = 0;
		
		/* TODO: that's bad, raise an exception */
	}

	dprintf(("PyThread_acquire_lock(%p, %d) -> %d\n", lock, waitflag, success));
	return success;
}

void PyThread_release_lock( PyThread_type_lock lock )
{
	status_t retval;
	
	dprintf(("PyThread_release_lock(%p) called\n", lock));
	
	retval = benaphore_unlock( (benaphore_t *)lock );
	if( retval != EOK ) {
		/* TODO: that's bad, raise an exception */
		return;
	}
}
