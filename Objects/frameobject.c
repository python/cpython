
/* Frame object implementation */

#include "Python.h"

#include "compile.h"
#include "frameobject.h"
#include "opcode.h"
#include "structmember.h"

#undef MIN
#undef MAX
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define OFF(x) offsetof(PyFrameObject, x)

static PyMemberDef frame_memberlist[] = {
	{"f_back",	T_OBJECT,	OFF(f_back),	RO},
	{"f_code",	T_OBJECT,	OFF(f_code),	RO},
	{"f_builtins",	T_OBJECT,	OFF(f_builtins),RO},
	{"f_globals",	T_OBJECT,	OFF(f_globals),	RO},
	{"f_lasti",	T_INT,		OFF(f_lasti),	RO},
	{"f_restricted",T_INT,		OFF(f_restricted),RO},
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

	if (f->f_trace)
		lineno = f->f_lineno;
	else
		lineno = PyCode_Addr2Line(f->f_code, f->f_lasti);

	return PyInt_FromLong(lineno);
}

/* Setter for f_lineno - you can set f_lineno from within a trace function in
 * order to jump to a given line of code, subject to some restrictions.  Most
 * lines are OK to jump to because they don't make any assumptions about the
 * state of the stack (obvious because you could remove the line and the code
 * would still work without any stack errors), but there are some constructs
 * that limit jumping:
 *
 *  o Lines with an 'except' statement on them can't be jumped to, because
 *    they expect an exception to be on the top of the stack.
 *  o Lines that live in a 'finally' block can't be jumped from or to, since
 *    the END_FINALLY expects to clean up the stack after the 'try' block.
 *  o 'try'/'for'/'while' blocks can't be jumped into because the blockstack
 *    needs to be set up before their code runs, and for 'for' loops the
 *    iterator needs to be on the stack.
 */
static int
frame_setlineno(PyFrameObject *f, PyObject* p_new_lineno)
{
	int new_lineno = 0;		/* The new value of f_lineno */
	int new_lasti = 0;		/* The new value of f_lasti */
	int new_iblock = 0;		/* The new value of f_iblock */
	char *code = NULL;		/* The bytecode for the frame... */
	int code_len = 0;		/* ...and its length */
	char *lnotab = NULL;		/* Iterating over co_lnotab */
	int lnotab_len = 0;		/* (ditto) */
	int offset = 0;			/* (ditto) */
	int line = 0;			/* (ditto) */
	int addr = 0;			/* (ditto) */
	int min_addr = 0;		/* Scanning the SETUPs and POPs */
	int max_addr = 0;		/* (ditto) */
	int delta_iblock = 0;		/* (ditto) */
	int min_delta_iblock = 0;	/* (ditto) */
	int min_iblock = 0;		/* (ditto) */
	int f_lasti_setup_addr = 0;	/* Policing no-jump-into-finally */
	int new_lasti_setup_addr = 0;	/* (ditto) */
	int blockstack[CO_MAXBLOCKS];	/* Walking the 'finally' blocks */
	int in_finally[CO_MAXBLOCKS];	/* (ditto) */
	int blockstack_top = 0;		/* (ditto) */
	int setup_op = 0;               /* (ditto) */

	/* f_lineno must be an integer. */
	if (!PyInt_Check(p_new_lineno)) {
		PyErr_SetString(PyExc_ValueError,
				"lineno must be an integer");
		return -1;
	}

	/* You can only do this from within a trace function, not via
	 * _getframe or similar hackery. */
	if (!f->f_trace)
	{
		PyErr_Format(PyExc_ValueError,
			     "f_lineno can only be set by a trace function");
		return -1;
	}

	/* Fail if the line comes before the start of the code block. */
	new_lineno = (int) PyInt_AsLong(p_new_lineno);
	if (new_lineno < f->f_code->co_firstlineno) {
		PyErr_Format(PyExc_ValueError,
			     "line %d comes before the current code block",
			     new_lineno);
		return -1;
	}

	/* Find the bytecode offset for the start of the given line, or the
	 * first code-owning line after it. */
	PyString_AsStringAndSize(f->f_code->co_lnotab, &lnotab, &lnotab_len);
	addr = 0;
	line = f->f_code->co_firstlineno;
	new_lasti = -1;
	for (offset = 0; offset < lnotab_len; offset += 2) {
		addr += lnotab[offset];
		line += lnotab[offset+1];
		if (line >= new_lineno) {
			new_lasti = addr;
			new_lineno = line;
			break;
		}
	}

	/* If we didn't reach the requested line, return an error. */
	if (new_lasti == -1) {
		PyErr_Format(PyExc_ValueError,
			     "line %d comes after the current code block",
			     new_lineno);
		return -1;
	}

	/* We're now ready to look at the bytecode. */
	PyString_AsStringAndSize(f->f_code->co_code, &code, &code_len);
	min_addr = MIN(new_lasti, f->f_lasti);
	max_addr = MAX(new_lasti, f->f_lasti);

	/* You can't jump onto a line with an 'except' statement on it -
	 * they expect to have an exception on the top of the stack, which
	 * won't be true if you jump to them.  They always start with code
	 * that either pops the exception using POP_TOP (plain 'except:'
	 * lines do this) or duplicates the exception on the stack using
	 * DUP_TOP (if there's an exception type specified).  See compile.c,
	 * 'com_try_except' for the full details.  There aren't any other
	 * cases (AFAIK) where a line's code can start with DUP_TOP or
	 * POP_TOP, but if any ever appear, they'll be subject to the same
	 * restriction (but with a different error message). */
	if (code[new_lasti] == DUP_TOP || code[new_lasti] == POP_TOP) {
		PyErr_SetString(PyExc_ValueError,
		    "can't jump to 'except' line as there's no exception");
		return -1;
	}

	/* You can't jump into or out of a 'finally' block because the 'try'
	 * block leaves something on the stack for the END_FINALLY to clean
	 * up.  So we walk the bytecode, maintaining a simulated blockstack.
	 * When we reach the old or new address and it's in a 'finally' block
	 * we note the address of the corresponding SETUP_FINALLY.  The jump
	 * is only legal if neither address is in a 'finally' block or
	 * they're both in the same one.  'blockstack' is a stack of the
	 * bytecode addresses of the SETUP_X opcodes, and 'in_finally' tracks
	 * whether we're in a 'finally' block at each blockstack level. */
	f_lasti_setup_addr = -1;
	new_lasti_setup_addr = -1;
	memset(blockstack, '\0', sizeof(blockstack));
	memset(in_finally, '\0', sizeof(in_finally));
	blockstack_top = 0;
	for (addr = 0; addr < code_len; addr++) {
		unsigned char op = code[addr];
		switch (op) {
		case SETUP_LOOP:
		case SETUP_EXCEPT:
		case SETUP_FINALLY:
			blockstack[blockstack_top++] = addr;
			in_finally[blockstack_top-1] = 0;
			break;

		case POP_BLOCK:
			assert(blockstack_top > 0);
			setup_op = code[blockstack[blockstack_top-1]];
			if (setup_op == SETUP_FINALLY) {
				in_finally[blockstack_top-1] = 1;
			}
			else {
				blockstack_top--;
			}
			break;

		case END_FINALLY:
			/* Ignore END_FINALLYs for SETUP_EXCEPTs - they exist
			 * in the bytecode but don't correspond to an actual
			 * 'finally' block.  (If blockstack_top is 0, we must
			 * be seeing such an END_FINALLY.) */
			if (blockstack_top > 0) {
				setup_op = code[blockstack[blockstack_top-1]];
				if (setup_op == SETUP_FINALLY) {
					blockstack_top--;
				}
			}
			break;
		}

		/* For the addresses we're interested in, see whether they're
		 * within a 'finally' block and if so, remember the address
		 * of the SETUP_FINALLY. */
		if (addr == new_lasti || addr == f->f_lasti) {
			int i = 0;
			int setup_addr = -1;
			for (i = blockstack_top-1; i >= 0; i--) {
				if (in_finally[i]) {
					setup_addr = blockstack[i];
					break;
				}
			}

			if (setup_addr != -1) {
				if (addr == new_lasti) {
					new_lasti_setup_addr = setup_addr;
				}

				if (addr == f->f_lasti) {
					f_lasti_setup_addr = setup_addr;
				}
			}
		}

		if (op >= HAVE_ARGUMENT) {
			addr += 2;
		}
	}

	/* Verify that the blockstack tracking code didn't get lost. */
	assert(blockstack_top == 0);

	/* After all that, are we jumping into / out of a 'finally' block? */
	if (new_lasti_setup_addr != f_lasti_setup_addr) {
		PyErr_SetString(PyExc_ValueError,
			    "can't jump into or out of a 'finally' block");
		return -1;
	}


	/* Police block-jumping (you can't jump into the middle of a block)
	 * and ensure that the blockstack finishes up in a sensible state (by
	 * popping any blocks we're jumping out of).  We look at all the
	 * blockstack operations between the current position and the new
	 * one, and keep track of how many blocks we drop out of on the way.
	 * By also keeping track of the lowest blockstack position we see, we
	 * can tell whether the jump goes into any blocks without coming out
	 * again - in that case we raise an exception below. */
	delta_iblock = 0;
	for (addr = min_addr; addr < max_addr; addr++) {
		unsigned char op = code[addr];
		switch (op) {
		case SETUP_LOOP:
		case SETUP_EXCEPT:
		case SETUP_FINALLY:
			delta_iblock++;
			break;

		case POP_BLOCK:
			delta_iblock--;
			break;
		}

		min_delta_iblock = MIN(min_delta_iblock, delta_iblock);

		if (op >= HAVE_ARGUMENT) {
			addr += 2;
		}
	}

	/* Derive the absolute iblock values from the deltas. */
	min_iblock = f->f_iblock + min_delta_iblock;
	if (new_lasti > f->f_lasti) {
		/* Forwards jump. */
		new_iblock = f->f_iblock + delta_iblock;
	}
	else {
		/* Backwards jump. */
		new_iblock = f->f_iblock - delta_iblock;
	}

	/* Are we jumping into a block? */
	if (new_iblock > min_iblock) {
		PyErr_SetString(PyExc_ValueError,
				"can't jump into the middle of a block");
		return -1;
	}

	/* Pop any blocks that we're jumping out of. */
	while (f->f_iblock > new_iblock) {
		PyTryBlock *b = &f->f_blockstack[--f->f_iblock];
		while ((f->f_stacktop - f->f_valuestack) > b->b_level) {
			PyObject *v = (*--f->f_stacktop);
			Py_DECREF(v);
		}
	}

	/* Finally set the new f_lineno and f_lasti and return OK. */
	f->f_lineno = new_lineno;
	f->f_lasti = new_lasti;
	return 0;
}

static PyObject *
frame_gettrace(PyFrameObject *f, void *closure)
{
	PyObject* trace = f->f_trace;

	if (trace == NULL)
		trace = Py_None;

	Py_INCREF(trace);

	return trace;
}

static int
frame_settrace(PyFrameObject *f, PyObject* v, void *closure)
{
	/* We rely on f_lineno being accurate when f_trace is set. */

	PyObject* old_value = f->f_trace;

	Py_XINCREF(v);
	f->f_trace = v;
	
	if (v != NULL)
		f->f_lineno = PyCode_Addr2Line(f->f_code, f->f_lasti);

	Py_XDECREF(old_value);

	return 0;
}

static PyGetSetDef frame_getsetlist[] = {
	{"f_locals",	(getter)frame_getlocals, NULL, NULL},
	{"f_lineno",	(getter)frame_getlineno,
			(setter)frame_setlineno, NULL},
	{"f_trace",	(getter)frame_gettrace, (setter)frame_settrace, NULL},
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
	Py_DECREF(f->f_code);
	Py_DECREF(f->f_builtins);
	Py_DECREF(f->f_globals);
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

static PyObject *builtin_object;

int _PyFrame_Init()
{
	builtin_object = PyString_InternFromString("__builtins__");
	return (builtin_object != NULL);
}

PyFrameObject *
PyFrame_New(PyThreadState *tstate, PyCodeObject *code, PyObject *globals, 
	    PyObject *locals)
{
	PyFrameObject *back = tstate->frame;
	PyFrameObject *f;
	PyObject *builtins;
	int extras, ncells, nfrees, i;

#ifdef Py_DEBUG
	if (code == NULL || globals == NULL || !PyDict_Check(globals) ||
	    (locals != NULL && !PyMapping_Check(locals))) {
		PyErr_BadInternalCall();
		return NULL;
	}
#endif
	ncells = PyTuple_GET_SIZE(code->co_cellvars);
	nfrees = PyTuple_GET_SIZE(code->co_freevars);
	extras = code->co_stacksize + code->co_nlocals + ncells + nfrees;
	if (back == NULL || back->f_globals != globals) {
		builtins = PyDict_GetItem(globals, builtin_object);
		if (builtins) {
			if (PyModule_Check(builtins)) {
				builtins = PyModule_GetDict(builtins);
				assert(!builtins || PyDict_Check(builtins));
			}
			else if (!PyDict_Check(builtins))
				builtins = NULL;
		}
		if (builtins == NULL) {
			/* No builtins!  Make up a minimal one 
			   Give them 'None', at least. */
			builtins = PyDict_New();
			if (builtins == NULL || 
			    PyDict_SetItemString(
				    builtins, "None", Py_None) < 0)
				return NULL;
		}
		else
			Py_INCREF(builtins);

	}
	else {
		/* If we share the globals, we share the builtins.
		   Save a lookup and a call. */
		builtins = back->f_builtins;
		assert(builtins != NULL && PyDict_Check(builtins));
		Py_INCREF(builtins);
	}
	if (free_list == NULL) {
		f = PyObject_GC_NewVar(PyFrameObject, &PyFrame_Type, extras);
		if (f == NULL) {
			Py_DECREF(builtins);
			return NULL;
		}
	}
	else {
		assert(numfree > 0);
		--numfree;
		f = free_list;
		free_list = free_list->f_back;
		if (f->ob_size < extras) {
			f = PyObject_GC_Resize(PyFrameObject, f, extras);
			if (f == NULL) {
				Py_DECREF(builtins);
				return NULL;
			}
		}
		_Py_NewReference((PyObject *)f);
	}
	f->f_builtins = builtins;
	Py_XINCREF(back);
	f->f_back = back;
	Py_INCREF(code);
	f->f_code = code;
	Py_INCREF(globals);
	f->f_globals = globals;
	/* Most functions have CO_NEWLOCALS and CO_OPTIMIZED set. */
	if ((code->co_flags & (CO_NEWLOCALS | CO_OPTIMIZED)) == 
		(CO_NEWLOCALS | CO_OPTIMIZED))
		locals = NULL; /* PyFrame_FastToLocals() will set. */
	else if (code->co_flags & CO_NEWLOCALS) {
		locals = PyDict_New();
		if (locals == NULL) {
			Py_DECREF(f);
			return NULL;
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
	/* Tim said it's ok to replace memset */
	for (i=0; i<extras; i++)
		f->f_localsplus[i] = NULL;

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
			if (PyObject_DelItem(dict, key) != 0)
				PyErr_Clear();
		}
		else {
			if (PyObject_SetItem(dict, key, value) != 0)
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
		PyObject *value = PyObject_GetItem(dict, key);
		if (value == NULL)
			PyErr_Clear();
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
		Py_XDECREF(value);
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
	if (!PyTuple_Check(map))
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
	if (!PyTuple_Check(map))
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
	Py_XDECREF(builtin_object);
	builtin_object = NULL;
}
