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

#define NEXITFUNCS 32

typedef struct _is {

	PyObject *import_modules;
	PyObject *sysdict;

	int nthreads;

	void (*exitfuncs[NEXITFUNCS])();
	int nexitfuncs;

} PyInterpreterState;


/* State unique per thread */

struct _frame; /* Avoid including frameobject.h */

typedef struct _ts {

	PyInterpreterState *interpreter_state;

	struct _frame *frame;
	int recursion_depth;
	int ticker;
	int tracing;

	PyObject *sys_profilefunc;
	PyObject *sys_tracefunc;
	int sys_checkinterval;

	PyObject *curexc_type;
	PyObject *curexc_value;
	PyObject *curexc_traceback;

	PyObject *exc_type;
	PyObject *exc_value;
	PyObject *exc_traceback;

	/* XXX Other state that should be here:
	   - signal handlers
	   - low-level "pending calls"
	   Problem with both is that they may be referenced from
	   interrupt handlers where there is no clear concept of a
	   "current thread"???
	*/

} PyThreadState;


PyInterpreterState *PyInterpreterState_New Py_PROTO((void));
void PyInterpreterState_Delete Py_PROTO((PyInterpreterState *));

PyThreadState *PyThreadState_New Py_PROTO((PyInterpreterState *));
void PyThreadState_Delete Py_PROTO((PyThreadState *));

PyThreadState *PyThreadState_Get Py_PROTO((void));
PyThreadState *PyThreadState_Swap Py_PROTO((PyThreadState *));

/* Some background.

   There are lots of issues here.

   First, we can build Python without threads, with threads, or (when
   Greg Stein's mods are out of beta, on some platforms) with free
   threading.

   Next, assuming some form of threading is used, there can be several
   kinds of threads.  Python code can create threads with the thread
   module.  C code can create threads with the interface defined in
   python's "thread.h".  Or C code can create threads directly with
   the OS threads interface (e.g. Solaris threads, SGI threads or
   pthreads, whatever is being used, as long as it's the same that
   Python is configured for).

   Next, let's discuss sharing of interpreter state between threads.
   The exception state (sys.exc_* currently) should never be shared
   between threads, because it is stack frame specific.  The contents
   of the sys module, in particular sys.modules and sys.path, are
   generally shared between threads.  But occasionally it is useful to
   have separate module collections, e.g. when threads originate in C
   code and are used to execute unrelated Python scripts.
   (Traditionally, one would use separate processes for this, but
   there are lots of reasons why threads are attractive.)

*/

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATE_H */
