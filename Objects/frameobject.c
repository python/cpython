
/* Frame object implementation */

#include "Python.h"

#include "compile.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"

#define OFF(x) offsetof(PyFrameObject, x)

static PyMemberDef frame_memberlist[] = {
	{"f_back",	T_OBJECT,	OFF(f_back),	RO},
	{"f_code",	T_OBJECT,	OFF(f_code),	RO},
	{"f_builtins",	T_OBJECT,	OFF(f_builtins),RO},
	{"f_globals",	T_OBJECT,	OFF(f_globals),	RO},
	{"f_lasti",	T_INT,		OFF(f_lasti),	RO},
	{"f_restricted",T_INT,		OFF(f_restricted),RO},
	{"f_trace",	T_OBJECT,	OFF(f_trace)},
	{"f_exc_type",	T_OBJECT,	OFF(f_exc_type)},
	{"f_exc_value",	T_OBJECT,	OFF(f_exc_value)},
	{"f_exc_traceback", T_OBJECT,	OFF(f_exc_traceback)},
	{NULL}	/* Sentinel */
};

static PyObject *
frame_getlocals(PyFrameObject *f, void *closure)
{
	PyFrame_FastToLocals(f);
	Py_INCREF(f->f_locals);
	return f->f_locals;
}

static PyObject *
frame_getlineno(PyFrameObject *f, void *closure)
{
	int lineno;

	lineno = PyCode_Addr2Line(f->f_code, f->f_lasti);

	return PyInt_FromLong(lineno);
}

static PyGetSetDef frame_getsetlist[] = {
	{"f_locals",	(getter)frame_getlocals, NULL, NULL},
	{"f_lineno",	(getter)frame_getlineno, NULL, NULL},
	{0}
};

/* Stack frames are allocated and deallocated at a considerable rate.
   In an attempt to improve the speed of function calls, we maintain a
   separate free list of stack frames (just like integers are
   allocated in a special way -- see intobject.c).  When a stack frame
   is on the free list, only the following members have a meaning:
	ob_type		== &Frametype
	f_back		next item on free list, or NULL
	f_nlocals	number of locals
	f_stacksize	size of value stack
        ob_size         size of localsplus
   Note that the value and block stacks are preserved -- this can save
   another malloc() call or two (and two free() calls as well!).
   Also note that, unlike for integers, each frame object is a
   malloc'ed object in its own right -- it is only the actual calls to
   malloc() that we are trying to save here, not the administration.
   After all, while a typical program may make millions of calls, a
   call depth of more than 20 or 30 is probably already exceptional
   unless the program contains run-away recursion.  I hope.

   Later, MAXFREELIST was added to bound the # of frames saved on
   free_list.  Else programs creating lots of cyclic trash involving
   frames could provoke free_list into growing without bound.
*/

static PyFrameObject *free_list = NULL;
static int numfree = 0;		/* number of frames currently in free_list */
#define MAXFREELIST 200		/* max value for numfree */

static void
frame_dealloc(PyFrameObject *f)
{
	int i, slots;
	PyObject **fastlocals;
	PyObject **p;

 	PyObject_GC_UnTrack(f);
	Py_TRASHCAN_SAFE_BEGIN(f)
	/* Kill all local variables */
	slots = f->f_nlocals + f->f_ncells + f->f_nfreevars;
	fastlocals = f->f_localsplus;
	for (i = slots; --i >= 0; ++fastlocals) {
		Py_XDECREF(*fastlocals);
	}

	/* Free stack */
	if (f->f_stacktop != NULL) {
		for (p = f->f_valuestack; p < f->f_stacktop; p++)
			Py_XDECREF(*p);
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
	if (numfree < MAXFREELIST) {
		++numfree;
		f->f_back = free_list;
		free_list = f;
	}
	else
		PyObject_GC_Del(f);
	Py_TRASHCAN_SAFE_END(f)
}

static int
frame_traverse(PyFrameObject *f, visitproc visit, void *arg)
{
	PyObject **fastlocals, **p;
	int i, err, slots;
#define VISIT(o) if (o) {if ((err = visit((PyObject *)(o), arg))) return err;}

	VISIT(f->f_back);
	VISIT(f->f_code);
	VISIT(f->f_builtins);
	VISIT(f->f_globals);
	VISIT(f->f_locals);
	VISIT(f->f_trace);
	VISIT(f->f_exc_type);
	VISIT(f->f_exc_value);
	VISIT(f->f_exc_traceback);

	/* locals */
	slots = f->f_nlocals + f->f_ncells + f->f_nfreevars;
	fastlocals = f->f_localsplus;
	for (i = slots; --i >= 0; ++fastlocals) {
		VISIT(*fastlocals);
	}

	/* stack */
	if (f->f_stacktop != NULL) {
		for (p = f->f_valuestack; p < f->f_stacktop; p++)
			VISIT(*p);
	}
	return 0;
}

static void
frame_clear(PyFrameObject *f)
{
	PyObject **fastlocals, **p;
	int i, slots;

	Py_XDECREF(f->f_exc_type);
	f->f_exc_type = NULL;

	Py_XDECREF(f->f_exc_value);
	f->f_exc_value = NULL;

	Py_XDECREF(f->f_exc_traceback);
	f->f_exc_traceback = NULL;

	Py_XDECREF(f->f_trace);
	f->f_trace = NULL;

	/* locals */
	slots = f->f_nlocals + f->f_ncells + f->f_nfreevars;
	fastlocals = f->f_localsplus;
	for (i = slots; --i >= 0; ++fastlocals) {
		if (*fastlocals != NULL) {
			Py_XDECREF(*fastlocals);
			*fastlocals = NULL;
		}
	}

	/* stack */
	if (f->f_stacktop != NULL) {
		for (p = f->f_valuestack; p < f->f_stacktop; p++) {
			Py_XDECREF(*p);
			*p = NULL;
		}
	}
}


PyTypeObject PyFrame_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"frame",
	sizeof(PyFrameObject),
	sizeof(PyObject *),
	(destructor)frame_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0, 					/* tp_getattr */
	0,			 		/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	PyObject_GenericSetAttr,		/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
	0,             				/* tp_doc */
 	(traverseproc)frame_traverse,		/* tp_traverse */
	(inquiry)frame_clear,			/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	frame_memberlist,			/* tp_members */
	frame_getsetlist,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
};

PyFrameObject *
PyFrame_New(PyThreadState *tstate, PyCodeObject *code, PyObject *globals, 
	    PyObject *locals)
{
	PyFrameObject *back = tstate->frame;
	static PyObject *builtin_object;
	PyFrameObject *f;
	PyObject *builtins;
	int extras, ncells, nfrees;

	if (builtin_object == NULL) {
		builtin_object = PyString_InternFromString("__builtins__");
		if (builtin_object == NULL)
			return NULL;
	}
#ifdef Py_DEBUG
	if (code == NULL || globals == NULL || !PyDict_Check(globals) ||
	    (locals != NULL && !PyDict_Check(locals))) {
		PyErr_BadInternalCall();
		return NULL;
	}
#endif
	ncells = PyTuple_GET_SIZE(code->co_cellvars);
	nfrees = PyTuple_GET_SIZE(code->co_freevars);
	extras = code->co_stacksize + code->co_nlocals + ncells + nfrees;
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
		f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type, extras);
		if (f == NULL)
			return NULL;
	}
	else {
		assert(numfree > 0);
		--numfree;
		f = free_list;
		free_list = free_list->f_back;
		if (f->ob_size < extras) {
			f = PyObject_GC_Resize(PyFrameObject, f, extras);
			if (f == NULL)
				return NULL;
		}
		_Py_NewReference((PyObject *)f);
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

	f->f_lasti = -1;
	f->f_lineno = code->co_firstlineno;
	f->f_restricted = (builtins != tstate->interp->builtins);
	f->f_iblock = 0;
	f->f_nlocals = code->co_nlocals;
	f->f_stacksize = code->co_stacksize;
	f->f_ncells = ncells;
	f->f_nfreevars = nfrees;

	extras = f->f_nlocals + ncells + nfrees;
	memset(f->f_localsplus, 0, extras * sizeof(f->f_localsplus[0]));

	f->f_valuestack = f->f_localsplus + extras;
	f->f_stacktop = f->f_valuestack;
	_PyObject_GC_TRACK(f);
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

static void
map_to_dict(PyObject *map, int nmap, PyObject *dict, PyObject **values,
	    int deref)
{
	int j;
	for (j = nmap; --j >= 0; ) {
		PyObject *key = PyTuple_GET_ITEM(map, j);
		PyObject *value = values[j];
		if (deref)
			value = PyCell_GET(value);
		if (value == NULL) {
			if (PyDict_DelItem(dict, key) != 0)
				PyErr_Clear();
		}
		else {
			if (PyDict_SetItem(dict, key, value) != 0)
				PyErr_Clear();
		}
	}
}

static void
dict_to_map(PyObject *map, int nmap, PyObject *dict, PyObject **values,
	    int deref, int clear)
{
	int j;
	for (j = nmap; --j >= 0; ) {
		PyObject *key = PyTuple_GET_ITEM(map, j);
		PyObject *value = PyDict_GetItem(dict, key);
		if (deref) {
			if (value || clear) {
				if (PyCell_GET(values[j]) != value) {
					if (PyCell_Set(values[j], value) < 0)
						PyErr_Clear();
				}
			}
		} else if (value != NULL || clear) {
			if (values[j] != value) {
				Py_XINCREF(value);
				Py_XDECREF(values[j]);
				values[j] = value;
			}
		}
	}
}

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
	map = f->f_code->co_varnames;
	if (!PyDict_Check(locals) || !PyTuple_Check(map))
		return;
	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	fast = f->f_localsplus;
	j = PyTuple_Size(map);
	if (j > f->f_nlocals)
		j = f->f_nlocals;
	if (f->f_nlocals)
	    map_to_dict(map, j, locals, fast, 0);
	if (f->f_ncells || f->f_nfreevars) {
		if (!(PyTuple_Check(f->f_code->co_cellvars)
		      && PyTuple_Check(f->f_code->co_freevars))) {
			Py_DECREF(locals);
			return;
		}
		map_to_dict(f->f_code->co_cellvars, 
			    PyTuple_GET_SIZE(f->f_code->co_cellvars),
			    locals, fast + f->f_nlocals, 1);
		map_to_dict(f->f_code->co_freevars, 
			    PyTuple_GET_SIZE(f->f_code->co_freevars),
			    locals, fast + f->f_nlocals + f->f_ncells, 1);
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
	if (locals == NULL)
		return;
	if (!PyDict_Check(locals) || !PyTuple_Check(map))
		return;
	PyErr_Fetch(&error_type, &error_value, &error_traceback);
	fast = f->f_localsplus;
	j = PyTuple_Size(map);
	if (j > f->f_nlocals)
		j = f->f_nlocals;
	if (f->f_nlocals)
	    dict_to_map(f->f_code->co_varnames, j, locals, fast, 0, clear);
	if (f->f_ncells || f->f_nfreevars) {
		if (!(PyTuple_Check(f->f_code->co_cellvars)
		      && PyTuple_Check(f->f_code->co_freevars)))
			return;
		dict_to_map(f->f_code->co_cellvars, 
			    PyTuple_GET_SIZE(f->f_code->co_cellvars),
			    locals, fast + f->f_nlocals, 1, clear);
		dict_to_map(f->f_code->co_freevars, 
			    PyTuple_GET_SIZE(f->f_code->co_freevars),
			    locals, fast + f->f_nlocals + f->f_ncells, 1, 
			    clear);
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
		PyObject_GC_Del(f);
		--numfree;
	}
	assert(numfree == 0);
}
