#ifndef _THREAD_H_included
#define _THREAD_H_included

#define NO_EXIT_PROG		/* don't define exit_prog() */
				/* (the result is no use of signals on SGI) */

#ifndef Py_PROTO
#if defined(__STDC__) || defined(__cplusplus)
#define Py_PROTO(args)	args
#else
#define Py_PROTO(args)	()
#endif
#endif

typedef void *type_lock;
typedef void *type_sema;

#ifdef __cplusplus
extern "C" {
#endif

/* Macros defining new names for all these symbols */
/* BeOS note: We have exit_thread(), and no legacy code to
 * support, so we won't allow exit_thread and _exit_thread
 * in here.  Actually, I think these #defines should vanish;
 * aren't they cheesy in the face of the Great Renaming? [cjh]
 */
#define init_thread PyThread_init_thread
#define start_new_thread PyThread_start_new_thread
#ifndef __BEOS__
#define exit_thread PyThread_exit_thread
#define _exit_thread PyThread__exit_thread
#endif
#define get_thread_ident PyThread_get_thread_ident
#define allocate_lock PyThread_allocate_lock
#define free_lock PyThread_free_lock
#define acquire_lock PyThread_acquire_lock
#define release_lock PyThread_release_lock
#define allocate_sema PyThread_allocate_sema
#define free_sema PyThread_free_sema
#define down_sema PyThread_down_sema
#define up_sema PyThread_up_sema
#define exit_prog PyThread_exit_prog
#define _exit_prog PyThread__exit_prog
#define create_key PyThread_create_key
#define delete_key PyThread_delete_key
#define get_key_value PyThread_get_key_value
#define set_key_value PyThread_set_key_value


DL_IMPORT(void) init_thread Py_PROTO((void));
DL_IMPORT(int) start_new_thread Py_PROTO((void (*)(void *), void *));
#ifndef __BEOS__
DL_IMPORT(void) exit_thread Py_PROTO((void));
DL_IMPORT(void) _exit_thread Py_PROTO((void));
#else
DL_IMPORT(void) PyThread_exit_thread Py_PROTO((void));
DL_IMPORT(void) PyThread__exit_thread Py_PROTO((void));
#endif
DL_IMPORT(long) get_thread_ident Py_PROTO((void));

DL_IMPORT(type_lock) allocate_lock Py_PROTO((void));
DL_IMPORT(void) free_lock Py_PROTO((type_lock));
DL_IMPORT(int) acquire_lock Py_PROTO((type_lock, int));
#define WAIT_LOCK	1
#define NOWAIT_LOCK	0
DL_IMPORT(void) release_lock Py_PROTO((type_lock));

DL_IMPORT(type_sema) allocate_sema Py_PROTO((int));
DL_IMPORT(void) free_sema Py_PROTO((type_sema));
DL_IMPORT(int) down_sema Py_PROTO((type_sema, int));
#define WAIT_SEMA	1
#define NOWAIT_SEMA	0
DL_IMPORT(void) up_sema Py_PROTO((type_sema));

#ifndef NO_EXIT_PROG
DL_IMPORT(void) exit_prog Py_PROTO((int));
DL_IMPORT(void) _exit_prog Py_PROTO((int));
#endif

DL_IMPORT(int) create_key Py_PROTO((void));
DL_IMPORT(void) delete_key Py_PROTO((int));
DL_IMPORT(int) set_key_value Py_PROTO((int, void *));
DL_IMPORT(void *) get_key_value Py_PROTO((int));

#ifdef __cplusplus
}
#endif

#endif
