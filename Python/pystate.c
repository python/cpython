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

#include "Python.h"


static PyThreadState *current_tstate = NULL;


PyInterpreterState *
PyInterpreterState_New()
{
	PyInterpreterState *interp = PyMem_NEW(PyInterpreterState, 1);
	if (interp != NULL) {
		interp->import_modules = NULL;
		interp->sysdict = NULL;
		interp->nthreads = 0;
		interp->nexitfuncs = 0;
	}
	return interp;
}


void
PyInterpreterState_Delete(interp)
	PyInterpreterState *interp;
{
	Py_XDECREF(interp->import_modules);
	Py_XDECREF(interp->sysdict);

	PyMem_DEL(interp);
}


PyThreadState *
PyThreadState_New(interp)
	PyInterpreterState *interp;
{
	PyThreadState *tstate = PyMem_NEW(PyThreadState, 1);
	/* fprintf(stderr, "new tstate -> %p\n", tstate); */
	if (tstate != NULL) {
		tstate->interpreter_state = interp;

		tstate->frame = NULL;
		tstate->recursion_depth = 0;
		tstate->ticker = 0;
		tstate->tracing = 0;

		tstate->curexc_type = NULL;
		tstate->curexc_value = NULL;
		tstate->curexc_traceback = NULL;

		tstate->exc_type = NULL;
		tstate->exc_value = NULL;
		tstate->exc_traceback = NULL;

		tstate->sys_profilefunc = NULL;
		tstate->sys_tracefunc = NULL;
		tstate->sys_checkinterval = 0;

		interp->nthreads++;
	}
	return tstate;
}


void
PyThreadState_Delete(tstate)
	PyThreadState *tstate;
{
	/* fprintf(stderr, "delete tstate %p\n", tstate); */
	if (tstate == current_tstate)
		current_tstate = NULL;
	tstate->interpreter_state->nthreads--;

	Py_XDECREF((PyObject *) (tstate->frame)); /* XXX really? */

	Py_XDECREF(tstate->curexc_type);
	Py_XDECREF(tstate->curexc_value);
	Py_XDECREF(tstate->curexc_traceback);

	Py_XDECREF(tstate->exc_type);
	Py_XDECREF(tstate->exc_value);
	Py_XDECREF(tstate->exc_traceback);

	Py_XDECREF(tstate->sys_profilefunc);
	Py_XDECREF(tstate->sys_tracefunc);

	PyMem_DEL(tstate);
}


PyThreadState *
PyThreadState_Get()
{
	/* fprintf(stderr, "get tstate -> %p\n", current_tstate); */
	return current_tstate;
}


PyThreadState *
PyThreadState_Swap(new)
	PyThreadState *new;
{
	PyThreadState *old = current_tstate;
	/* fprintf(stderr, "swap tstate new=%p <--> old=%p\n", new, old); */
	current_tstate = new;
	return old;
}
