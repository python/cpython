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

void init_thread Py_PROTO((void));
int start_new_thread Py_PROTO((void (*)(void *), void *));
void exit_thread Py_PROTO((void));
void _exit_thread Py_PROTO((void));
long get_thread_ident Py_PROTO((void));

type_lock allocate_lock Py_PROTO((void));
void free_lock Py_PROTO((type_lock));
int acquire_lock Py_PROTO((type_lock, int));
#define WAIT_LOCK	1
#define NOWAIT_LOCK	0
void release_lock Py_PROTO((type_lock));

type_sema allocate_sema Py_PROTO((int));
void free_sema Py_PROTO((type_sema));
void down_sema Py_PROTO((type_sema));
void up_sema Py_PROTO((type_sema));

#ifndef NO_EXIT_PROG
void exit_prog Py_PROTO((int));
void _exit_prog Py_PROTO((int));
#endif

#ifdef __cplusplus
}
#endif

#endif
