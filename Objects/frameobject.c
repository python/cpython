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

/* Frame object implementation */

#include "Python.h"

#include "compile.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"

#define OFF(x) offsetof(PyFrameObject, x)

static struct memberlist frame_memberlist[] = {
	{"f_back",	T_OBJECT,	OFF(f_back),	RO},
	{"f_code",	T_OBJECT,	OFF(f_code),	RO},
	{"f_builtins",	T_OBJECT,	OFF(f_builtins),RO},
	{"f_globals",	T_OBJECT,	OFF(f_globals),	RO},
	{"f_locals",	T_OBJECT,	OFF(f_locals),	RO},
	{"f_lasti",	T_INT,		OFF(f_lasti),	RO},
	{"f_lineno",	T_INT,		OFF(f_lineno),	RO},
	{"f_restricted",T_INT,		OFF(f_restricted),RO},
	{"f_trace",	T_OBJECT,	OFF(f_trace)},
	{"f_exc_type",	T_OBJECT,	OFF(f_exc_type)},
	{"f_exc_value",	T_OBJECT,	OFF(f_exc_value)},
	{"f_exc_traceback", T_OBJECT,	OFF(f_exc_traceback)},
	{NULL}	/* Sentinel */
};

static PyObject *
frame_getattr(f, name)
	PyFrameObject *f;
	char *name;
{
	if (strcmp(name, "f_locals") == 0)
		PyFrame_FastToLocals(f);
	return PyMember_Get((char *)f, frame_memberlist, name);
}

static int
frame_setattr(f, name, value)
	PyFrameObject *f;
	char *name;
	PyObject *value;
{
	return PyMember_Set((char *)f, frame_memberlist, name, value);
}

/* Stack frames are allocated and deallocated at a considerable rate.
   In an attempt to improve the speed of function calls, we maintain a
   separate free list of stack frames (just like integers are
   allocated in a special way -- see intobject.c).  When a stack frame
   is on the free list, only the following members have a meaning:
	ob_type		== &Frametype
	f_back		next item on free list, or NULL
	f_nlocals	number of locals
	f_stacksize	size of value stack
   Note that the value and block stacks are preserved -- this can save
   another malloc() call or two (and two free() calls as well!).
   Also note that, unlike for integers, each frame object is a
   malloc'ed object in its own right -- it is only the actual calls to
   malloc() that we are trying to save here, not the administration.
   After all, while a typical program may make millions of calls, a
   call depth of more than 20 or 30 is probably already exceptional
   unless the program contains run-away recursion.  I hope.
*/

static PyFrameObject *free_list = NULL;

static void
frame_dealloc(f)
	PyFrameObject *f;
{
	int i;
	PyObject **fastlocals;

	/* Kill all local variables */
	fastlocals = f->f_localsplus;
	for (i = f->f_nlocals; --i >= 0; ++fastlocals) {
		Py_XDECREF(*fastlocals);
	}

	Py_XDECREF(f->f_back);
	Py_XDECREF(f->f_code);
	Py_XDECREF(f->f_builtins);
	Py_XDECREF(f->f_globals);
	Py_XDECREF(f->f_locals);
	Py_XDECREF(f->f_trace);
	Py_XDECREF(f->f_exc_type);
	Py_XDECREF(f->f_exc_value);
	Py_XDECREF(f->f_exc_traceback);
	f->f_back = free_list;
	free_list = f;
}

PyTypeObject PyFrame_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"frame",
	sizeof(PyFrameObject),
	0,
	(destructor)frame_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)frame_getattr, /*tp_getattr*/
	(setattrfunc)frame_setattr, /*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

PyFrameObject *
PyFrame_New(tstate, code, globals, locals)
	PyThreadState *tstate;
	PyCodeObject *code;
	PyObject *globals;
	PyObject *locals;
{
	PyFrameObject *back = tstate->frame;
	static PyObject *builtin_object;
	PyFrameObject *f;
	PyObject *builtins;
	int extras = code->co_stacksize + code->co_nlocals;

	if (builtin_object == NULL) {
		builtin_object = PyString_InternFromString("__builtins__");
		if (builtin_object == NULL)
			return NULL;
	}
	if ((back != NULL && !PyFrame_Check(back)) ||
	    code == NULL || !PyCode_Check(code) ||
	    globals == NULL || !PyDict_Check(globals) ||
	    (locals != NULL && !PyDict_Check(locals))) {
		PyErr_BadInternalCall();
		return NULL;
	}
	builtins = PyDict_GetItem(globals, builtin_object);
	if (builtins != NULL && PyModule_Check(builtins))
		builtins = PyModule_GetDict(builtins);
	if (builtins == NULL || !PyDict_Check(builtins)) {
		PyErr_SetString(PyExc_TypeError,
				"bad __builtins__ dictionary");
		return NULL;
	}
	if (free_list == NULL) {
		f = (PyFrameObject *)
			malloc(sizeof(PyFrameObject) +
			       extras*sizeof(PyObject *));
		if (f == NULL)
			return (PyFrameObject *)PyErr_NoMemory();
		f->ob_type = &PyFrame_Type;
		_Py_NewReference(f);
	}
	else {
		f = free_list;
		free_list = free_list->f_back;
		if (f->f_nlocals + f->f_stacksize < extras) {
			f = realloc(f, sizeof(PyFrameObject) +
				       extras*sizeof(PyObject *));
			if (f == NULL)
				return (PyFrameObject *)PyErr_NoMemory();
		}
		else
			extras = f->f_nlocals + f->f_stacksize;
		f->ob_type = &PyFrame_Type;
		_Py_NewReference(f);
	}
	Py_XINCREF(back);
	f->f_back = back;
	Py_INCREF(code);
	f->f_code = code;
	Py_XINCREF(builtins);
	f->f_builtins = builtins;
	Py_INCREF(globals);
	f->f_globals = globals;
	if (code->co_flags & CO_NEWLOCALS) {
		if (code->co_flags & CO_OPTIMIZED)
			locals = NULL; /* Let fast_2_locals handle it */
		else {
			locals = PyDict_New();
			if (locals == NULL) {
				Py_DECREF(f);
				return NULL;
			}
		}
	}
	else {
		if (locals == NULL)
			locals = globals;
		Py_INCREF(locals);
	}
	f->f_locals = locals;
	f->f_trace = NULL;
	f->f_exc_type = f->f_exc_value = f->f_exc_traceback = NULL;
	f->f_tstate = PyThreadState_Get();
	if (f->f_tstate == NULL)
		Py_FatalError("can't create new frame without thread");

	f->f_lasti = 0;
	f->f_lineno = code->co_firstlineno;
	f->f_restricted = (builtins != PyBuiltin_GetDict());
	f->f_iblock = 0;
	f->f_nlocals = code->co_nlocals;
	f->f_stacksize = extras - code->co_nlocals;

	while (--extras >= 0)
		f->f_localsplus[extras] = NULL;

	f->f_valuestack = f->f_localsplus + f->f_nlocals;

	return f;
}

/* Block management */

void
PyFrame_BlockSetup(f, type, handler, level)
	PyFrameObject *f;
	int type;
	int handler;
	int level;
{
	PyTryBlock *b;
	if (f->f_iblock >= CO_MAXBLOCKS)
		Py_FatalError("XXX block stack overflow");
	b = &f->f_blockstack[f->f_iblock++];
	b->b_type = type;
	b->b_level = level;
	b->b_handler = handler;
}

PyTryBlock *
PyFrame_BlockPop(f)
	PyFrameObject *f;
{
	PyTryBlock *b;
	if (f->f_iblock <= 0)
		Py_FatalError("XXX block stack underflow");
	b = &f->f_blockstack[--f->f_iblock];
	return b;
}

/* Convert between "fast" version of locals and dictionary version */

void
PyFrame_FastToLocals(f)
	PyFrameObject *f;
{
	/* Merge fast locals into f->f_locals */
	PyObject *locals, *map;
	PyObject **fast;
	PyObject *error_type, *error_value, *error_traceback;
	int j;
	if (f == NULL)
		return;
	locals = f->f_locals;
	if (locals == NULL) {
		locals = f->f_locals = PyDict_New();
		if (locals == NULL) {
			PyErr_Clear(); /* Can't report it :-( */
			return;
		}
	}
	if (f->f_nlocals == 0)
		return;
	map = f->f_code->co_varnames;
	if (!PyDict_Check(locals) || !PyTuple_Check(map))
		return;
	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	fast = f->f_localsplus;
	j = PyTuple_Size(map);
	if (j > f->f_nlocals)
		j = f->f_nlocals;
	for (; --j >= 0; ) {
		PyObject *key = PyTuple_GetItem(map, j);
		PyObject *value = fast[j];
		if (value == NULL) {
			PyErr_Clear();
			if (PyDict_DelItem(locals, key) != 0)
				PyErr_Clear();
		}
		else {
			if (PyDict_SetItem(locals, key, value) != 0)
				PyErr_Clear();
		}
	}
	PyErr_Restore(error_type, error_value, error_traceback);
}

void
PyFrame_LocalsToFast(f, clear)
	PyFrameObject *f;
	int clear;
{
	/* Merge f->f_locals into fast locals */
	PyObject *locals, *map;
	PyObject **fast;
	PyObject *error_type, *error_value, *error_traceback;
	int j;
	if (f == NULL)
		return;
	locals = f->f_locals;
	map = f->f_code->co_varnames;
	if (locals == NULL || f->f_code->co_nlocals == 0)
		return;
	if (!PyDict_Check(locals) || !PyTuple_Check(map))
		return;
	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	fast = f->f_localsplus;
	j = PyTuple_Size(map);
	if (j > f->f_nlocals)
		j = f->f_nlocals;
	for (; --j >= 0; ) {
		PyObject *key = PyTuple_GetItem(map, j);
		PyObject *value = PyDict_GetItem(locals, key);
		if (value == NULL)
			PyErr_Clear();
		else
			Py_INCREF(value);
		if (value != NULL || clear) {
			Py_XDECREF(fast[j]);
			fast[j] = value;
		}
	}
	PyErr_Restore(error_type, error_value, error_traceback);
}
