#ifndef Py_PYSTATE_H
#define Py_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

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

/* Thread and interpreter state structures and their interfaces */


/* State shared between threads */

struct _ts; /* Forward */
struct _is; /* Forward */

typedef struct _is {

	struct _is *next;
	struct _ts *tstate_head;

	PyObject *modules;
	PyObject *sysdict;
	PyObject *builtins;

	int checkinterval;

} PyInterpreterState;


/* State unique per thread */

struct _frame; /* Avoid including frameobject.h */

typedef struct _ts {

	struct _ts *next;
	PyInterpreterState *interp;

	struct _frame *frame;
	int recursion_depth;
	int ticker;
	int tracing;

	PyObject *sys_profilefunc;
	PyObject *sys_tracefunc;

	PyObject *curexc_type;
	PyObject *curexc_value;
	PyObject *curexc_traceback;

	PyObject *exc_type;
	PyObject *exc_value;
	PyObject *exc_traceback;

	PyObject *dict;

	/* XXX signal handlers should also be here */

} PyThreadState;


DL_IMPORT(PyInterpreterState *) PyInterpreterState_New Py_PROTO((void));
DL_IMPORT(void) PyInterpreterState_Clear Py_PROTO((PyInterpreterState *));
DL_IMPORT(void) PyInterpreterState_Delete Py_PROTO((PyInterpreterState *));

DL_IMPORT(PyThreadState *) PyThreadState_New Py_PROTO((PyInterpreterState *));
DL_IMPORT(void) PyThreadState_Clear Py_PROTO((PyThreadState *));
DL_IMPORT(void) PyThreadState_Delete Py_PROTO((PyThreadState *));

DL_IMPORT(PyThreadState *) PyThreadState_Get Py_PROTO((void));
DL_IMPORT(PyThreadState *) PyThreadState_Swap Py_PROTO((PyThreadState *));
DL_IMPORT(PyObject *) PyThreadState_GetDict Py_PROTO((void));

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATE_H */
