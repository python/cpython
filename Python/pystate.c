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

#define ZAP(x) { \
	PyObject *tmp = (PyObject *)(x); \
	(x) = NULL; \
	Py_XDECREF(tmp); \
}


static PyInterpreterState *interp_head = NULL;

static PyThreadState *current_tstate = NULL;


PyInterpreterState *
PyInterpreterState_New()
{
	PyInterpreterState *interp = PyMem_NEW(PyInterpreterState, 1);

	if (interp != NULL) {
		interp->modules = NULL;
		interp->sysdict = NULL;
		interp->builtins = NULL;
		interp->checkinterval = 10;
		interp->tstate_head = NULL;

		interp->next = interp_head;
		interp_head = interp;
	}

	return interp;
}


void
PyInterpreterState_Clear(interp)
	PyInterpreterState *interp;
{
	PyThreadState *p;
	for (p = interp->tstate_head; p != NULL; p = p->next)
		PyThreadState_Clear(p);
	ZAP(interp->modules);
	ZAP(interp->sysdict);
	ZAP(interp->builtins);
}


static void
zapthreads(interp)
	PyInterpreterState *interp;
{
	PyThreadState *p, *q;
	p = interp->tstate_head;
	while (p != NULL) {
		q = p->next;
		PyThreadState_Delete(p);
		p = q;
	}
}


void
PyInterpreterState_Delete(interp)
	PyInterpreterState *interp;
{
	PyInterpreterState **p;
	zapthreads(interp);
	for (p = &interp_head; ; p = &(*p)->next) {
		if (*p == NULL)
			Py_FatalError(
				"PyInterpreterState_Delete: invalid interp");
		if (*p == interp)
			break;
	}
	if (interp->tstate_head != NULL)
		Py_FatalError("PyInterpreterState_Delete: remaining threads");
	*p = interp->next;
	PyMem_DEL(interp);
}


PyThreadState *
PyThreadState_New(interp)
	PyInterpreterState *interp;
{
	PyThreadState *tstate = PyMem_NEW(PyThreadState, 1);

	if (tstate != NULL) {
		tstate->interp = interp;

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

		tstate->next = interp->tstate_head;
		interp->tstate_head = tstate;
	}

	return tstate;
}


void
PyThreadState_Clear(tstate)
	PyThreadState *tstate;
{
	if (tstate->frame != NULL)
		fprintf(stderr,
		  "PyThreadState_Clear: warning: thread still has a frame\n");

	ZAP(tstate->frame);

	ZAP(tstate->curexc_type);
	ZAP(tstate->curexc_value);
	ZAP(tstate->curexc_traceback);

	ZAP(tstate->exc_type);
	ZAP(tstate->exc_value);
	ZAP(tstate->exc_traceback);

	ZAP(tstate->sys_profilefunc);
	ZAP(tstate->sys_tracefunc);
}


void
PyThreadState_Delete(tstate)
	PyThreadState *tstate;
{
	PyInterpreterState *interp;
	PyThreadState **p;
	if (tstate == NULL)
		Py_FatalError("PyThreadState_Delete: NULL tstate");
	if (tstate == current_tstate)
		Py_FatalError("PyThreadState_Delete: tstate is still current");
	interp = tstate->interp;
	if (interp == NULL)
		Py_FatalError("PyThreadState_Delete: NULL interp");
	for (p = &interp->tstate_head; ; p = &(*p)->next) {
		if (*p == NULL)
			Py_FatalError(
				"PyThreadState_Delete: invalid tstate");
		if (*p == tstate)
			break;
	}
	*p = tstate->next;
	PyMem_DEL(tstate);
}


PyThreadState *
PyThreadState_Get()
{
	if (current_tstate == NULL)
		Py_FatalError("PyThreadState_Get: no current thread");

	return current_tstate;
}


PyThreadState *
PyThreadState_Swap(new)
	PyThreadState *new;
{
	PyThreadState *old = current_tstate;

	current_tstate = new;

	return old;
}
