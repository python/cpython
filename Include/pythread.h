
#ifndef Py_PYTHREAD_H
#define Py_PYTHREAD_H

#define NO_EXIT_PROG		/* don't define PyThread_exit_prog() */
				/* (the result is no use of signals on SGI) */

typedef void *PyThread_type_lock;
typedef void *PyThread_type_sema;

#ifdef __cplusplus
extern "C" {
#endif

DL_IMPORT(void) PyThread_init_thread(void);
DL_IMPORT(int) PyThread_start_new_thread(void (*)(void *), void *);
DL_IMPORT(void) PyThread_exit_thread(void);
DL_IMPORT(void) PyThread__PyThread_exit_thread(void);
DL_IMPORT(long) PyThread_get_thread_ident(void);

DL_IMPORT(PyThread_type_lock) PyThread_allocate_lock(void);
DL_IMPORT(void) PyThread_free_lock(PyThread_type_lock);
DL_IMPORT(int) PyThread_acquire_lock(PyThread_type_lock, int);
#define WAIT_LOCK	1
#define NOWAIT_LOCK	0
DL_IMPORT(void) PyThread_release_lock(PyThread_type_lock);

DL_IMPORT(PyThread_type_sema) PyThread_allocate_sema(int);
DL_IMPORT(void) PyThread_free_sema(PyThread_type_sema);
DL_IMPORT(int) PyThread_down_sema(PyThread_type_sema, int);
#define WAIT_SEMA	1
#define NOWAIT_SEMA	0
DL_IMPORT(void) PyThread_up_sema(PyThread_type_sema);

#ifndef NO_EXIT_PROG
DL_IMPORT(void) PyThread_exit_prog(int);
DL_IMPORT(void) PyThread__PyThread_exit_prog(int);
#endif

DL_IMPORT(int) PyThread_create_key(void);
DL_IMPORT(void) PyThread_delete_key(int);
DL_IMPORT(int) PyThread_set_key_value(int, void *);
DL_IMPORT(void *) PyThread_get_key_value(int);

#ifdef __cplusplus
}
#endif

#endif /* !Py_PYTHREAD_H */
