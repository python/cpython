#ifndef Py_PYTHREAD_H
#define Py_PYTHREAD_H

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

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

#endif /* !Py_PYTHREAD_H */
