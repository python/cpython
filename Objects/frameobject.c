
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
frame_getattr(PyFrameObject *f, char *name)
{
	if (strcmp(name, "f_locals") == 0)
		PyFrame_FastToLocals(f);
	return PyMember_Get((char *)f, frame_memberlist, name);
}

static int
frame_setattr(PyFrameObject *f, char *name, PyObject *value)
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
frame_dealloc(PyFrameObject *f)
{
	int i;
	PyObject **fastlocals;

	Py_TRASHCAN_SAFE_BEGIN(f)
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
	Py_TRASHCAN_SAFE_END(f)
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
PyFrame_New(PyThreadState *tstate, PyCodeObject *code,
            PyObject *globals, PyObject *locals)
{
	PyFrameObject *back = tstate->frame;
	static PyObject *builtin_object;
	PyFrameObject *f;
	PyObject *builtins;
	int extras;

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
	extras = code->co_stacksize + code->co_nlocals;
	if (back == NULL || back->f_globals != globals) {
		builtins = PyDict_GetItem(globals, builtin_object);
		if (builtins != NULL && PyModule_Check(builtins))
			builtins = PyModule_GetDict(builtins);
	}
	else {
		/* If we share the globals, we share the builtins.
		   Save a lookup and a call. */
		builtins = back->f_builtins;
	}
	if (builtins != NULL && !PyDict_Check(builtins))
		builtins = NULL;
	if (free_list == NULL) {
		/* PyObject_New is inlined */
		f = (PyFrameObject *)
			PyObject_MALLOC(sizeof(PyFrameObject) +
					extras*sizeof(PyObject *));
		if (f == NULL)
			return (PyFrameObject *)PyErr_NoMemory();
		PyObject_INIT(f, &PyFrame_Type);
	}
	else {
		f = free_list;
		free_list = free_list->f_back;
		if (f->f_nlocals + f->f_stacksize < extras) {
			f = (PyFrameObject *)
				PyObject_REALLOC(f, sizeof(PyFrameObject) +
						 extras*sizeof(PyObject *));
			if (f == NULL)
				return (PyFrameObject *)PyErr_NoMemory();
		}
		else
			extras = f->f_nlocals + f->f_stacksize;
		PyObject_INIT(f, &PyFrame_Type);
	}
	if (builtins == NULL) {
		/* No builtins!  Make up a minimal one. */
		builtins = PyDict_New();
		if (builtins == NULL || /* Give them 'None', at least. */
		    PyDict_SetItemString(builtins, "None", Py_None) < 0) {
			Py_DECREF(f);
			return NULL;
		}
	}
	else
		Py_XINCREF(builtins);
	f->f_builtins = builtins;
	Py_XINCREF(back);
	f->f_back = back;
	Py_INCREF(code);
	f->f_code = code;
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
	f->f_tstate = tstate;

	f->f_lasti = 0;
	f->f_lineno = code->co_firstlineno;
	f->f_restricted = (builtins != tstate->interp->builtins);
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
PyFrame_BlockSetup(PyFrameObject *f, int type, int handler, int level)
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
PyFrame_BlockPop(PyFrameObject *f)
{
	PyTryBlock *b;
	if (f->f_iblock <= 0)
		Py_FatalError("XXX block stack underflow");
	b = &f->f_blockstack[--f->f_iblock];
	return b;
}

/* Convert between "fast" version of locals and dictionary version */

void
PyFrame_FastToLocals(PyFrameObject *f)
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
PyFrame_LocalsToFast(PyFrameObject *f, int clear)
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
		Py_XINCREF(value);
		if (value != NULL || clear) {
			Py_XDECREF(fast[j]);
			fast[j] = value;
		}
	}
	PyErr_Restore(error_type, error_value, error_traceback);
}

/* Clear out the free list */

void
PyFrame_Fini(void)
{
	while (free_list != NULL) {
		PyFrameObject *f = free_list;
		free_list = free_list->f_back;
		PyObject_DEL(f);
	}
}
