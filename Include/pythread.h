#ifndef _THREAD_H_included
#define _THREAD_H_included

#define NO_EXIT_PROG		/* don't define PyThread_exit_prog() */
				/* (the result is no use of signals on SGI) */

#ifndef Py_PROTO
#if defined(__STDC__) || defined(__cplusplus)
#define Py_PROTO(args)	args
#else
#define Py_PROTO(args)	()
#endif
#endif

typedef void *PyThread_type_lock;
typedef void *PyThread_type_sema;

#ifdef __cplusplus
extern "C" {
#endif

DL_IMPORT(void) PyThread_init_thread Py_PROTO((void));
DL_IMPORT(int) PyThread_start_new_thread Py_PROTO((void (*)(void *), void *));
DL_IMPORT(void) PyThread_exit_thread Py_PROTO((void));
DL_IMPORT(void) PyThread__PyThread_exit_thread Py_PROTO((void));
DL_IMPORT(long) PyThread_get_thread_ident Py_PROTO((void));

DL_IMPORT(PyThread_type_lock) PyThread_allocate_lock Py_PROTO((void));
DL_IMPORT(void) PyThread_free_lock Py_PROTO((PyThread_type_lock));
DL_IMPORT(int) PyThread_acquire_lock Py_PROTO((PyThread_type_lock, int));
#define WAIT_LOCK	1
#define NOWAIT_LOCK	0
DL_IMPORT(void) PyThread_release_lock Py_PROTO((PyThread_type_lock));

DL_IMPORT(PyThread_type_sema) PyThread_allocate_sema Py_PROTO((int));
DL_IMPORT(void) PyThread_free_sema Py_PROTO((PyThread_type_sema));
DL_IMPORT(int) PyThread_down_sema Py_PROTO((PyThread_type_sema, int));
#define WAIT_SEMA	1
#define NOWAIT_SEMA	0
DL_IMPORT(void) PyThread_up_sema Py_PROTO((PyThread_type_sema));

#ifndef NO_EXIT_PROG
DL_IMPORT(void) PyThread_exit_prog Py_PROTO((int));
DL_IMPORT(void) PyThread__PyThread_exit_prog Py_PROTO((int));
#endif

DL_IMPORT(int) PyThread_create_key Py_PROTO((void));
DL_IMPORT(void) PyThread_delete_key Py_PROTO((int));
DL_IMPORT(int) PyThread_set_key_value Py_PROTO((int, void *));
DL_IMPORT(void *) PyThread_get_key_value Py_PROTO((int));

#ifdef __cplusplus
}
#endif

#endif
