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

/* Execute compiled code */

/* XXX TO DO:
   XXX how to pass arguments to call_trace?
   XXX speed up searching for keywords by using a dictionary
   XXX document it!
   */

#include "Python.h"

#include "compile.h"
#include "frameobject.h"
#include "eval.h"
#include "opcode.h"
#include "graminit.h"

#include <ctype.h>

/* Turn this on if your compiler chokes on the big switch: */
/* #define CASE_TOO_BIG 1 */

#ifdef Py_DEBUG
/* For debugging the interpreter: */
#define LLTRACE  1	/* Low-level trace feature */
#define CHECKEXC 1	/* Double-check exception checking */
#endif


/* Forward declarations */

static PyObject *eval_code2 Py_PROTO((PyCodeObject *,
				 PyObject *, PyObject *,
				 PyObject **, int,
				 PyObject **, int,
				 PyObject **, int,
				 PyObject *));
#ifdef LLTRACE
static int prtrace Py_PROTO((PyObject *, char *));
#endif
static void call_exc_trace Py_PROTO((PyObject **, PyObject**, PyFrameObject *));
static int call_trace
	Py_PROTO((PyObject **, PyObject **, PyFrameObject *, char *, PyObject *));
static PyObject *add Py_PROTO((PyObject *, PyObject *));
static PyObject *sub Py_PROTO((PyObject *, PyObject *));
static PyObject *powerop Py_PROTO((PyObject *, PyObject *));
static PyObject *mul Py_PROTO((PyObject *, PyObject *));
static PyObject *divide Py_PROTO((PyObject *, PyObject *));
static PyObject *mod Py_PROTO((PyObject *, PyObject *));
static PyObject *neg Py_PROTO((PyObject *));
static PyObject *pos Py_PROTO((PyObject *));
static PyObject *not Py_PROTO((PyObject *));
static PyObject *invert Py_PROTO((PyObject *));
static PyObject *lshift Py_PROTO((PyObject *, PyObject *));
static PyObject *rshift Py_PROTO((PyObject *, PyObject *));
static PyObject *and Py_PROTO((PyObject *, PyObject *));
static PyObject *xor Py_PROTO((PyObject *, PyObject *));
static PyObject *or Py_PROTO((PyObject *, PyObject *));
static PyObject *call_builtin Py_PROTO((PyObject *, PyObject *, PyObject *));
static PyObject *call_function Py_PROTO((PyObject *, PyObject *, PyObject *));
static PyObject *apply_subscript Py_PROTO((PyObject *, PyObject *));
static PyObject *loop_subscript Py_PROTO((PyObject *, PyObject *));
static int slice_index Py_PROTO((PyObject *, int, int *));
static PyObject *apply_slice Py_PROTO((PyObject *, PyObject *, PyObject *));
static int assign_subscript Py_PROTO((PyObject *, PyObject *, PyObject *));
static int assign_slice Py_PROTO((PyObject *, PyObject *, PyObject *, PyObject *));
static int cmp_exception Py_PROTO((PyObject *, PyObject *));
static int cmp_member Py_PROTO((PyObject *, PyObject *));
static PyObject *cmp_outcome Py_PROTO((int, PyObject *, PyObject *));
static int import_from Py_PROTO((PyObject *, PyObject *, PyObject *));
static PyObject *build_class Py_PROTO((PyObject *, PyObject *, PyObject *));
static int exec_statement Py_PROTO((PyObject *, PyObject *, PyObject *));
static PyObject *find_from_args Py_PROTO((PyFrameObject *, int));


/* Dynamic execution profile */
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
static long dxpairs[257][256];
#define dxp dxpairs[256]
#else
static long dxp[256];
#endif
#endif


/* Pointer to current frame, used to link new frames to */

static PyFrameObject *current_frame;

#ifdef WITH_THREAD

#include <errno.h>
#include "thread.h"

static type_lock interpreter_lock = 0;
static long main_thread = 0;

void
PyEval_InitThreads()
{
	if (interpreter_lock)
		return;
	interpreter_lock = allocate_lock();
	acquire_lock(interpreter_lock, 1);
	main_thread = get_thread_ident();
}

#endif

/* Functions save_thread and restore_thread are always defined so
   dynamically loaded modules needn't be compiled separately for use
   with and without threads: */

PyObject *
PyEval_SaveThread()
{
#ifdef WITH_THREAD
	if (interpreter_lock) {
		PyObject *res;
		res = (PyObject *)current_frame;
		current_frame = NULL;
		release_lock(interpreter_lock);
		return res;
	}
#endif
	return NULL;
}

void
PyEval_RestoreThread(x)
	PyObject *x;
{
#ifdef WITH_THREAD
	if (interpreter_lock) {
		int err;
		err = errno;
		acquire_lock(interpreter_lock, 1);
		errno = err;
		current_frame = (PyFrameObject *)x;
	}
#endif
}


/* Mechanism whereby asynchronously executing callbacks (e.g. UNIX
   signal handlers or Mac I/O completion routines) can schedule calls
   to a function to be called synchronously.
   The synchronous function is called with one void* argument.
   It should return 0 for success or -1 for failure -- failure should
   be accompanied by an exception.

   If registry succeeds, the registry function returns 0; if it fails
   (e.g. due to too many pending calls) it returns -1 (without setting
   an exception condition).

   Note that because registry may occur from within signal handlers,
   or other asynchronous events, calling malloc() is unsafe!

#ifdef WITH_THREAD
   Any thread can schedule pending calls, but only the main thread
   will execute them.
#endif

   XXX WARNING!  ASYNCHRONOUSLY EXECUTING CODE!
   There are two possible race conditions:
   (1) nested asynchronous registry calls;
   (2) registry calls made while pending calls are being processed.
   While (1) is very unlikely, (2) is a real possibility.
   The current code is safe against (2), but not against (1).
   The safety against (2) is derived from the fact that only one
   thread (the main thread) ever takes things out of the queue.
*/

static int ticker = 0; /* main loop counter to do periodic things */

#define NPENDINGCALLS 32
static struct {
	int (*func) Py_PROTO((ANY *));
	ANY *arg;
} pendingcalls[NPENDINGCALLS];
static volatile int pendingfirst = 0;
static volatile int pendinglast = 0;

int
Py_AddPendingCall(func, arg)
	int (*func) Py_PROTO((ANY *));
	ANY *arg;
{
	static int busy = 0;
	int i, j;
	/* XXX Begin critical section */
	/* XXX If you want this to be safe against nested
	   XXX asynchronous calls, you'll have to work harder! */
	if (busy)
		return -1;
	busy = 1;
	i = pendinglast;
	j = (i + 1) % NPENDINGCALLS;
	if (j == pendingfirst)
		return -1; /* Queue full */
	pendingcalls[i].func = func;
	pendingcalls[i].arg = arg;
	pendinglast = j;
	ticker = 0; /* Signal main loop */
	busy = 0;
	/* XXX End critical section */
	return 0;
}

int
Py_MakePendingCalls()
{
	static int busy = 0;
#ifdef WITH_THREAD
	if (main_thread && get_thread_ident() != main_thread) {
		ticker = 0; /* We're not done yet */
		return 0;
	}
#endif
	if (busy) {
		ticker = 0; /* We're not done yet */
		return 0;
	}
	busy = 1;
	for (;;) {
		int i;
		int (*func) Py_PROTO((ANY *));
		ANY *arg;
		i = pendingfirst;
		if (i == pendinglast)
			break; /* Queue empty */
		func = pendingcalls[i].func;
		arg = pendingcalls[i].arg;
		pendingfirst = (i + 1) % NPENDINGCALLS;
		if (func(arg) < 0) {
			busy = 0;
			ticker = 0; /* We're not done yet */
			return -1;
		}
	}
	busy = 0;
	return 0;
}


/* Status code for main loop (reason for stack unwind) */

enum why_code {
		WHY_NOT,	/* No error */
		WHY_EXCEPTION,	/* Exception occurred */
		WHY_RERAISE,	/* Exception re-raised by 'finally' */
		WHY_RETURN,	/* 'return' statement */
		WHY_BREAK	/* 'break' statement */
};

static enum why_code do_raise Py_PROTO((PyObject *, PyObject *, PyObject *));


/* Backward compatible interface */

PyObject *
PyEval_EvalCode(co, globals, locals)
	PyCodeObject *co;
	PyObject *globals;
	PyObject *locals;
{
	return eval_code2(co,
			  globals, locals,
			  (PyObject **)NULL, 0,
			  (PyObject **)NULL, 0,
			  (PyObject **)NULL, 0,
			  (PyObject *)NULL);
}


/* Interpreter main loop */

#ifndef MAX_RECURSION_DEPTH
#define MAX_RECURSION_DEPTH 10000
#endif

static int recursion_depth = 0;

static PyObject *
eval_code2(co, globals, locals,
	   args, argcount, kws, kwcount, defs, defcount, owner)
	PyCodeObject *co;
	PyObject *globals;
	PyObject *locals;
	PyObject **args;
	int argcount;
	PyObject **kws; /* length: 2*kwcount */
	int kwcount;
	PyObject **defs;
	int defcount;
	PyObject *owner;
{
#ifdef DXPAIRS
	int lastopcode = 0;
#endif
	register unsigned char *next_instr;
	register int opcode = 0; /* Current opcode */
	register int oparg = 0;	/* Current opcode argument, if any */
	register PyObject **stack_pointer;
	register enum why_code why; /* Reason for block stack unwind */
	register int err;	/* Error status -- nonzero if error */
	register PyObject *x;	/* Result object -- NULL if error */
	register PyObject *v;	/* Temporary objects popped off stack */
	register PyObject *w;
	register PyObject *u;
	register PyObject *t;
	register PyFrameObject *f; /* Current frame */
	register PyObject **fastlocals = NULL;
	PyObject *retval = NULL;	/* Return value */
#ifdef LLTRACE
	int lltrace;
#endif
#if defined(Py_DEBUG) || defined(LLTRACE)
	/* Make it easier to find out where we are with a debugger */
	char *filename = PyString_AsString(co->co_filename);
#endif

/* Code access macros */

#define GETCONST(i)	Getconst(f, i)
#define GETNAME(i)	Getname(f, i)
#define GETNAMEV(i)	Getnamev(f, i)
#define FIRST_INSTR()	(GETUSTRINGVALUE(co->co_code))
#define INSTR_OFFSET()	(next_instr - FIRST_INSTR())
#define NEXTOP()	(*next_instr++)
#define NEXTARG()	(next_instr += 2, (next_instr[-1]<<8) + next_instr[-2])
#define JUMPTO(x)	(next_instr = FIRST_INSTR() + (x))
#define JUMPBY(x)	(next_instr += (x))

/* Stack manipulation macros */

#define STACK_LEVEL()	(stack_pointer - f->f_valuestack)
#define EMPTY()		(STACK_LEVEL() == 0)
#define TOP()		(stack_pointer[-1])
#define BASIC_PUSH(v)	(*stack_pointer++ = (v))
#define BASIC_POP()	(*--stack_pointer)

#ifdef LLTRACE
#define PUSH(v)		(BASIC_PUSH(v), lltrace && prtrace(TOP(), "push"))
#define POP()		(lltrace && prtrace(TOP(), "pop"), BASIC_POP())
#else
#define PUSH(v)		BASIC_PUSH(v)
#define POP()		BASIC_POP()
#endif

/* Local variable macros */

#define GETLOCAL(i)	(fastlocals[i])
#define SETLOCAL(i, value)	do { Py_XDECREF(GETLOCAL(i)); \
				     GETLOCAL(i) = value; } while (0)

#ifdef USE_STACKCHECK
	if (recursion_depth%10 == 0 && PyOS_CheckStack()) {
		PyErr_SetString(PyExc_MemoryError, "Stack overflow");
		return NULL;
	}
#endif

	if (globals == NULL) {
		PyErr_SetString(PyExc_SystemError, "eval_code2: NULL globals");
		return NULL;
	}

#ifdef LLTRACE
	lltrace = PyDict_GetItemString(globals, "__lltrace__") != NULL;
#endif

	f = PyFrame_New(
			current_frame,		/*back*/
			co,			/*code*/
			globals,		/*globals*/
			locals);		/*locals*/
	if (f == NULL)
		return NULL;

	current_frame = f;
	fastlocals = f->f_localsplus;

	if (co->co_argcount > 0 ||
	    co->co_flags & (CO_VARARGS | CO_VARKEYWORDS)) {
		int i;
		int n = argcount;
		PyObject *kwdict = NULL;
		if (co->co_flags & CO_VARKEYWORDS) {
			kwdict = PyDict_New();
			if (kwdict == NULL)
				goto fail;
			i = co->co_argcount;
			if (co->co_flags & CO_VARARGS)
				i++;
			SETLOCAL(i, kwdict);
		}
		if (argcount > co->co_argcount) {
			if (!(co->co_flags & CO_VARARGS)) {
				PyErr_SetString(PyExc_TypeError, "too many arguments");
				goto fail;
			}
			n = co->co_argcount;
		}
		for (i = 0; i < n; i++) {
			x = args[i];
			Py_INCREF(x);
			SETLOCAL(i, x);
		}
		if (co->co_flags & CO_VARARGS) {
			u = PyTuple_New(argcount - n);
			if (u == NULL)
				goto fail;
			SETLOCAL(co->co_argcount, u);
			for (i = n; i < argcount; i++) {
				x = args[i];
				Py_INCREF(x);
				PyTuple_SET_ITEM(u, i-n, x);
			}
		}
		for (i = 0; i < kwcount; i++) {
			PyObject *keyword = kws[2*i];
			PyObject *value = kws[2*i + 1];
			int j;
			/* XXX slow -- speed up using dictionary? */
			for (j = 0; j < co->co_argcount; j++) {
				PyObject *nm = PyTuple_GET_ITEM(co->co_varnames, j);
				if (PyObject_Compare(keyword, nm) == 0)
					break;
			}
			if (j >= co->co_argcount) {
				if (kwdict == NULL) {
					PyErr_Format(PyExc_TypeError,
					"unexpected keyword argument: %.400s",
						     PyString_AsString(keyword));
					goto fail;
				}
				PyDict_SetItem(kwdict, keyword, value);
			}
			else {
				if (GETLOCAL(j) != NULL) {
					PyErr_SetString(PyExc_TypeError,
						"keyword parameter redefined");
					goto fail;
				}
				Py_INCREF(value);
				SETLOCAL(j, value);
			}
		}
		if (argcount < co->co_argcount) {
			int m = co->co_argcount - defcount;
			for (i = argcount; i < m; i++) {
				if (GETLOCAL(i) == NULL) {
					PyErr_SetString(PyExc_TypeError,
						   "not enough arguments");
					goto fail;
				}
			}
			if (n > m)
				i = n - m;
			else
				i = 0;
			for (; i < defcount; i++) {
				if (GETLOCAL(m+i) == NULL) {
					PyObject *def = defs[i];
					Py_INCREF(def);
					SETLOCAL(m+i, def);
				}
			}
		}
	}
	else {
		if (argcount > 0 || kwcount > 0) {
			PyErr_SetString(PyExc_TypeError, "no arguments expected");
			goto fail;
		}
	}

	if (_PySys_TraceFunc != NULL) {
		/* sys_trace, if defined, is a function that will
		   be called  on *every* entry to a code block.
		   Its return value, if not None, is a function that
		   will be called at the start of each executed line
		   of code.  (Actually, the function must return
		   itself in order to continue tracing.)
		   The trace functions are called with three arguments:
		   a pointer to the current frame, a string indicating
		   why the function is called, and an argument which
		   depends on the situation.  The global trace function
		   (sys.trace) is also called whenever an exception
		   is detected. */
		if (call_trace(&_PySys_TraceFunc, &f->f_trace, f, "call",
			       Py_None/*XXX how to compute arguments now?*/)) {
			/* Trace function raised an error */
			goto fail;
		}
	}

	if (_PySys_ProfileFunc != NULL) {
		/* Similar for sys_profile, except it needn't return
		   itself and isn't called for "line" events */
		if (call_trace(&_PySys_ProfileFunc, (PyObject**)0, f, "call",
			       Py_None/*XXX*/)) {
			goto fail;
		}
	}

	if (++recursion_depth > MAX_RECURSION_DEPTH) {
		--recursion_depth;
		PyErr_SetString(PyExc_RuntimeError, "Maximum recursion depth exceeded");
		current_frame = f->f_back;
		Py_DECREF(f);
		return NULL;
	}

	next_instr = GETUSTRINGVALUE(co->co_code);
	stack_pointer = f->f_valuestack;
	
	why = WHY_NOT;
	err = 0;
	x = Py_None;	/* Not a reference, just anything non-NULL */
	
	for (;;) {
		/* Do periodic things.
		   Doing this every time through the loop would add
		   too much overhead (a function call per instruction).
		   So we do it only every Nth instruction.

		   The ticker is reset to zero if there are pending
		   calls (see Py_AddPendingCall() and
		   Py_MakePendingCalls() above). */
		
		if (--ticker < 0) {
			ticker = _PySys_CheckInterval;
			if (pendingfirst != pendinglast) {
				if (Py_MakePendingCalls() < 0) {
					why = WHY_EXCEPTION;
					goto on_error;
				}
			}
#ifdef macintosh
#undef HAVE_SIGNAL_H
#endif
#ifndef HAVE_SIGNAL_H /* Is this the right #define? */
/* If we have true signals, the signal handler will call
   Py_AddPendingCall() so we don't have to call sigcheck().
   On the Mac and DOS, alas, we have to call it. */
			if (PyErr_CheckSignals()) {
				why = WHY_EXCEPTION;
				goto on_error;
			}
#endif

#ifdef WITH_THREAD
			if (interpreter_lock) {
				/* Give another thread a chance */

				current_frame = NULL;
				release_lock(interpreter_lock);

				/* Other threads may run now */

				acquire_lock(interpreter_lock, 1);
				current_frame = f;
			}
#endif
		}

		/* Extract opcode and argument */

#if defined(Py_DEBUG) || defined(LLTRACE)
		f->f_lasti = INSTR_OFFSET();
#endif
		
		opcode = NEXTOP();
		if (HAS_ARG(opcode))
			oparg = NEXTARG();
#ifdef DYNAMIC_EXECUTION_PROFILE
#ifdef DXPAIRS
		dxpairs[lastopcode][opcode]++;
		lastopcode = opcode;
#endif
		dxp[opcode]++;
#endif

#ifdef LLTRACE
		/* Instruction tracing */
		
		if (lltrace) {
			if (HAS_ARG(opcode)) {
				printf("%d: %d, %d\n",
					(int) (INSTR_OFFSET() - 3),
					opcode, oparg);
			}
			else {
				printf("%d: %d\n",
					(int) (INSTR_OFFSET() - 1), opcode);
			}
		}
#endif

		/* Main switch on opcode */
		
		switch (opcode) {
		
		/* BEWARE!
		   It is essential that any operation that fails sets either
		   x to NULL, err to nonzero, or why to anything but WHY_NOT,
		   and that no operation that succeeds does this! */
		
		/* case STOP_CODE: this is an error! */
		
		case POP_TOP:
			v = POP();
			Py_DECREF(v);
			continue;
		
		case ROT_TWO:
			v = POP();
			w = POP();
			PUSH(v);
			PUSH(w);
			continue;
		
		case ROT_THREE:
			v = POP();
			w = POP();
			x = POP();
			PUSH(v);
			PUSH(x);
			PUSH(w);
			continue;
		
		case DUP_TOP:
			v = TOP();
			Py_INCREF(v);
			PUSH(v);
			continue;
		
		case UNARY_POSITIVE:
			v = POP();
			x = pos(v);
			Py_DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case UNARY_NEGATIVE:
			v = POP();
			x = neg(v);
			Py_DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case UNARY_NOT:
			v = POP();
			x = not(v);
			Py_DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case UNARY_CONVERT:
			v = POP();
			x = PyObject_Repr(v);
			Py_DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
			
		case UNARY_INVERT:
			v = POP();
			x = invert(v);
			Py_DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_POWER:
			w = POP();
			v = POP();
			x = powerop(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_MULTIPLY:
			w = POP();
			v = POP();
			x = mul(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_DIVIDE:
			w = POP();
			v = POP();
			x = divide(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_MODULO:
			w = POP();
			v = POP();
			x = mod(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_ADD:
			w = POP();
			v = POP();
			x = add(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_SUBTRACT:
			w = POP();
			v = POP();
			x = sub(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_SUBSCR:
			w = POP();
			v = POP();
			x = apply_subscript(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_LSHIFT:
			w = POP();
			v = POP();
			x = lshift(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_RSHIFT:
			w = POP();
			v = POP();
			x = rshift(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_AND:
			w = POP();
			v = POP();
			x = and(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_XOR:
			w = POP();
			v = POP();
			x = xor(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_OR:
			w = POP();
			v = POP();
			x = or(v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case SLICE+0:
		case SLICE+1:
		case SLICE+2:
		case SLICE+3:
			if ((opcode-SLICE) & 2)
				w = POP();
			else
				w = NULL;
			if ((opcode-SLICE) & 1)
				v = POP();
			else
				v = NULL;
			u = POP();
			x = apply_slice(u, v, w);
			Py_DECREF(u);
			Py_XDECREF(v);
			Py_XDECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case STORE_SLICE+0:
		case STORE_SLICE+1:
		case STORE_SLICE+2:
		case STORE_SLICE+3:
			if ((opcode-STORE_SLICE) & 2)
				w = POP();
			else
				w = NULL;
			if ((opcode-STORE_SLICE) & 1)
				v = POP();
			else
				v = NULL;
			u = POP();
			t = POP();
			err = assign_slice(u, v, w, t); /* u[v:w] = t */
			Py_DECREF(t);
			Py_DECREF(u);
			Py_XDECREF(v);
			Py_XDECREF(w);
			if (err == 0) continue;
			break;
		
		case DELETE_SLICE+0:
		case DELETE_SLICE+1:
		case DELETE_SLICE+2:
		case DELETE_SLICE+3:
			if ((opcode-DELETE_SLICE) & 2)
				w = POP();
			else
				w = NULL;
			if ((opcode-DELETE_SLICE) & 1)
				v = POP();
			else
				v = NULL;
			u = POP();
			err = assign_slice(u, v, w, (PyObject *)NULL);
							/* del u[v:w] */
			Py_DECREF(u);
			Py_XDECREF(v);
			Py_XDECREF(w);
			if (err == 0) continue;
			break;
		
		case STORE_SUBSCR:
			w = POP();
			v = POP();
			u = POP();
			/* v[w] = u */
			err = assign_subscript(v, w, u);
			Py_DECREF(u);
			Py_DECREF(v);
			Py_DECREF(w);
			if (err == 0) continue;
			break;
		
		case DELETE_SUBSCR:
			w = POP();
			v = POP();
			/* del v[w] */
			err = assign_subscript(v, w, (PyObject *)NULL);
			Py_DECREF(v);
			Py_DECREF(w);
			if (err == 0) continue;
			break;
		
		case PRINT_EXPR:
			v = POP();
			/* Print value except if procedure result */
			/* Before printing, also assign to '_' */
			if (v != Py_None &&
			    (err = PyDict_SetItemString(f->f_builtins, "_", v)) == 0 &&
			    !Py_SuppressPrintingFlag) {
				Py_FlushLine();
				x = PySys_GetObject("stdout");
				err = PyFile_WriteObject(v, x, 0);
				PyFile_SoftSpace(x, 1);
				Py_FlushLine();
			}
			Py_DECREF(v);
			break;
		
		case PRINT_ITEM:
			v = POP();
			w = PySys_GetObject("stdout");
			if (PyFile_SoftSpace(w, 1))
				PyFile_WriteString(" ", w);
			err = PyFile_WriteObject(v, w, Py_PRINT_RAW);
			if (err == 0 && PyString_Check(v)) {
				/* XXX move into writeobject() ? */
				char *s = PyString_AsString(v);
				int len = PyString_Size(v);
				if (len > 0 &&
				    isspace(Py_CHARMASK(s[len-1])) &&
				    s[len-1] != ' ')
					PyFile_SoftSpace(w, 0);
			}
			Py_DECREF(v);
			if (err == 0) continue;
			break;
		
		case PRINT_NEWLINE:
			x = PySys_GetObject("stdout");
			if (x == NULL)
				PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
			else {
				PyFile_WriteString("\n", x);
				PyFile_SoftSpace(x, 0);
			}
			break;
		
		case BREAK_LOOP:
			why = WHY_BREAK;
			break;

		case RAISE_VARARGS:
			u = v = w = NULL;
			switch (oparg) {
			case 3:
				u = POP(); /* traceback */
				/* Fallthrough */
			case 2:
				v = POP(); /* value */
				/* Fallthrough */
			case 1:
				w = POP(); /* exc */
				why = do_raise(w, v, u);
				break;
			default:
				PyErr_SetString(PyExc_SystemError,
					   "bad RAISE_VARARGS oparg");
				why = WHY_EXCEPTION;
				break;
			}
			break;
		
		case LOAD_LOCALS:
			if ((x = f->f_locals) == NULL) {
				PyErr_SetString(PyExc_SystemError, "no locals");
				break;
			}
			Py_INCREF(x);
			PUSH(x);
			break;
		
		case RETURN_VALUE:
			retval = POP();
			why = WHY_RETURN;
			break;

		case EXEC_STMT:
			w = POP();
			v = POP();
			u = POP();
			err = exec_statement(u, v, w);
			Py_DECREF(u);
			Py_DECREF(v);
			Py_DECREF(w);
			break;
		
		case POP_BLOCK:
			{
				PyTryBlock *b = PyFrame_BlockPop(f);
				while (STACK_LEVEL() > b->b_level) {
					v = POP();
					Py_DECREF(v);
				}
			}
			break;
		
		case END_FINALLY:
			v = POP();
			if (PyInt_Check(v)) {
				why = (enum why_code) PyInt_AsLong(v);
				if (why == WHY_RETURN)
					retval = POP();
			}
			else if (PyString_Check(v) || PyClass_Check(v)) {
				w = POP();
				u = POP();
				PyErr_Restore(v, w, u);
				why = WHY_RERAISE;
				break;
			}
			else if (v != Py_None) {
				PyErr_SetString(PyExc_SystemError,
					"'finally' pops bad exception");
				why = WHY_EXCEPTION;
			}
			Py_DECREF(v);
			break;
		
		case BUILD_CLASS:
			u = POP();
			v = POP();
			w = POP();
			x = build_class(u, v, w);
			PUSH(x);
			Py_DECREF(u);
			Py_DECREF(v);
			Py_DECREF(w);
			break;
		
		case STORE_NAME:
			w = GETNAMEV(oparg);
			v = POP();
			if ((x = f->f_locals) == NULL) {
				PyErr_SetString(PyExc_SystemError, "no locals");
				break;
			}
			err = PyDict_SetItem(x, w, v);
			Py_DECREF(v);
			break;
		
		case DELETE_NAME:
			w = GETNAMEV(oparg);
			if ((x = f->f_locals) == NULL) {
				PyErr_SetString(PyExc_SystemError, "no locals");
				break;
			}
			if ((err = PyDict_DelItem(x, w)) != 0)
				PyErr_SetObject(PyExc_NameError, w);
			break;

#ifdef CASE_TOO_BIG
		default: switch (opcode) {
#endif
		
		case UNPACK_TUPLE:
			v = POP();
			if (!PyTuple_Check(v)) {
				PyErr_SetString(PyExc_TypeError, "unpack non-tuple");
				why = WHY_EXCEPTION;
			}
			else if (PyTuple_Size(v) != oparg) {
				PyErr_SetString(PyExc_ValueError,
					"unpack tuple of wrong size");
				why = WHY_EXCEPTION;
			}
			else {
				for (; --oparg >= 0; ) {
					w = PyTuple_GET_ITEM(v, oparg);
					Py_INCREF(w);
					PUSH(w);
				}
			}
			Py_DECREF(v);
			break;
		
		case UNPACK_LIST:
			v = POP();
			if (!PyList_Check(v)) {
				PyErr_SetString(PyExc_TypeError, "unpack non-list");
				why = WHY_EXCEPTION;
			}
			else if (PyList_Size(v) != oparg) {
				PyErr_SetString(PyExc_ValueError,
					"unpack list of wrong size");
				why = WHY_EXCEPTION;
			}
			else {
				for (; --oparg >= 0; ) {
					w = PyList_GetItem(v, oparg);
					Py_INCREF(w);
					PUSH(w);
				}
			}
			Py_DECREF(v);
			break;
		
		case STORE_ATTR:
			w = GETNAMEV(oparg);
			v = POP();
			u = POP();
			err = PyObject_SetAttr(v, w, u); /* v.w = u */
			Py_DECREF(v);
			Py_DECREF(u);
			break;
		
		case DELETE_ATTR:
			w = GETNAMEV(oparg);
			v = POP();
			err = PyObject_SetAttr(v, w, (PyObject *)NULL); /* del v.w */
			Py_DECREF(v);
			break;
		
		case STORE_GLOBAL:
			w = GETNAMEV(oparg);
			v = POP();
			err = PyDict_SetItem(f->f_globals, w, v);
			Py_DECREF(v);
			break;
		
		case DELETE_GLOBAL:
			w = GETNAMEV(oparg);
			if ((err = PyDict_DelItem(f->f_globals, w)) != 0)
				PyErr_SetObject(PyExc_NameError, w);
			break;
		
		case LOAD_CONST:
			x = GETCONST(oparg);
			Py_INCREF(x);
			PUSH(x);
			break;
		
		case LOAD_NAME:
			w = GETNAMEV(oparg);
			if ((x = f->f_locals) == NULL) {
				PyErr_SetString(PyExc_SystemError, "no locals");
				break;
			}
			x = PyDict_GetItem(x, w);
			if (x == NULL) {
				PyErr_Clear();
				x = PyDict_GetItem(f->f_globals, w);
				if (x == NULL) {
					PyErr_Clear();
					x = PyDict_GetItem(f->f_builtins, w);
					if (x == NULL) {
						PyErr_SetObject(PyExc_NameError, w);
						break;
					}
				}
			}
			Py_INCREF(x);
			PUSH(x);
			break;
		
		case LOAD_GLOBAL:
			w = GETNAMEV(oparg);
			x = PyDict_GetItem(f->f_globals, w);
			if (x == NULL) {
				PyErr_Clear();
				x = PyDict_GetItem(f->f_builtins, w);
				if (x == NULL) {
					PyErr_SetObject(PyExc_NameError, w);
					break;
				}
			}
			Py_INCREF(x);
			PUSH(x);
			break;

		case LOAD_FAST:
			x = GETLOCAL(oparg);
			if (x == NULL) {
				PyErr_SetObject(PyExc_NameError,
					   PyTuple_GetItem(co->co_varnames,
							oparg));
				break;
			}
			Py_INCREF(x);
			PUSH(x);
			if (x != NULL) continue;
			break;

		case STORE_FAST:
			v = POP();
			SETLOCAL(oparg, v);
			continue;

		case DELETE_FAST:
			SETLOCAL(oparg, NULL);
			continue;
		
		case BUILD_TUPLE:
			x = PyTuple_New(oparg);
			if (x != NULL) {
				for (; --oparg >= 0;) {
					w = POP();
					PyTuple_SET_ITEM(x, oparg, w);
				}
				PUSH(x);
				continue;
			}
			break;
		
		case BUILD_LIST:
			x =  PyList_New(oparg);
			if (x != NULL) {
				for (; --oparg >= 0;) {
					w = POP();
					err = PyList_SetItem(x, oparg, w);
					if (err != 0)
						break;
				}
				PUSH(x);
				continue;
			}
			break;
		
		case BUILD_MAP:
			x = PyDict_New();
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case LOAD_ATTR:
			w = GETNAMEV(oparg);
			v = POP();
			x = PyObject_GetAttr(v, w);
			Py_DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case COMPARE_OP:
			w = POP();
			v = POP();
			x = cmp_outcome(oparg, v, w);
			Py_DECREF(v);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case IMPORT_NAME:
			w = GETNAMEV(oparg);
			x = PyDict_GetItemString(f->f_builtins, "__import__");
			if (x == NULL) {
				PyErr_SetString(PyExc_ImportError,
					   "__import__ not found");
				break;
			}
			if (PyCFunction_Check(x)) {
				u = Py_None;
				Py_INCREF(u);
			}
			else {
				u = find_from_args(f, INSTR_OFFSET());
				if (u == NULL) {
					x = u;
					break;
				}
			}
			w = Py_BuildValue("(OOOO)",
				    w,
				    f->f_globals,
				    f->f_locals == NULL ? Py_None : f->f_locals,
				    u);
			Py_DECREF(u);
			if (w == NULL) {
				x = NULL;
				break;
			}
			x = PyEval_CallObject(x, w);
			Py_DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case IMPORT_FROM:
			w = GETNAMEV(oparg);
			v = TOP();
			PyFrame_FastToLocals(f);
			if ((x = f->f_locals) == NULL) {
				PyErr_SetString(PyExc_SystemError, "no locals");
				break;
			}
			err = import_from(x, v, w);
			PyFrame_LocalsToFast(f, 0);
			if (err == 0) continue;
			break;

		case JUMP_FORWARD:
			JUMPBY(oparg);
			continue;
		
		case JUMP_IF_FALSE:
			err = PyObject_IsTrue(TOP());
			if (err > 0)
				err = 0;
			else if (err == 0)
				JUMPBY(oparg);
			else
				break;
			continue;
		
		case JUMP_IF_TRUE:
			err = PyObject_IsTrue(TOP());
			if (err > 0) {
				err = 0;
				JUMPBY(oparg);
			}
			else if (err == 0)
				;
			else
				break;
			continue;
		
		case JUMP_ABSOLUTE:
			JUMPTO(oparg);
			continue;
		
		case FOR_LOOP:
			/* for v in s: ...
			   On entry: stack contains s, i.
			   On exit: stack contains s, i+1, s[i];
			   but if loop exhausted:
			   	s, i are popped, and we jump */
			w = POP(); /* Loop index */
			v = POP(); /* Sequence object */
			u = loop_subscript(v, w);
			if (u != NULL) {
				PUSH(v);
				x = PyInt_FromLong(PyInt_AsLong(w)+1);
				PUSH(x);
				Py_DECREF(w);
				PUSH(u);
				if (x != NULL) continue;
			}
			else {
				Py_DECREF(v);
				Py_DECREF(w);
				/* A NULL can mean "s exhausted"
				   but also an error: */
				if (PyErr_Occurred())
					why = WHY_EXCEPTION;
				else {
					JUMPBY(oparg);
					continue;
				}
			}
			break;
		
		case SETUP_LOOP:
		case SETUP_EXCEPT:
		case SETUP_FINALLY:
			PyFrame_BlockSetup(f, opcode, INSTR_OFFSET() + oparg,
						STACK_LEVEL());
			continue;
		
		case SET_LINENO:
#ifdef LLTRACE
			if (lltrace)
				printf("--- %s:%d \n", filename, oparg);
#endif
			f->f_lineno = oparg;
			if (f->f_trace == NULL)
				continue;
			/* Trace each line of code reached */
			f->f_lasti = INSTR_OFFSET();
			err = call_trace(&f->f_trace, &f->f_trace,
					 f, "line", Py_None);
			break;

		case CALL_FUNCTION:
		{
			int na = oparg & 0xff;
			int nk = (oparg>>8) & 0xff;
			int n = na + 2*nk;
			PyObject **pfunc = stack_pointer - n - 1;
			PyObject *func = *pfunc;
			PyObject *self = NULL;
			PyObject *class = NULL;
			f->f_lasti = INSTR_OFFSET() - 3; /* For tracing */
			if (PyMethod_Check(func)) {
				self = PyMethod_Self(func);
				class = PyMethod_Class(func);
				func = PyMethod_Function(func);
				Py_INCREF(func);
				if (self != NULL) {
					Py_INCREF(self);
					Py_DECREF(*pfunc);
					*pfunc = self;
					na++;
					n++;
				}
				else {
					/* Unbound methods must be
					   called with an instance of
					   the class (or a derived
					   class) as first argument */
					if (na > 0 &&
					    (self = stack_pointer[-n])
					 	!= NULL &&
					    PyInstance_Check(self) &&
					    PyClass_IsSubclass(
						    (PyObject *)
						    (((PyInstanceObject *)self)
						     ->in_class),
						    class))
						/* Handy-dandy */ ;
					else {
						PyErr_SetString(PyExc_TypeError,
	   "unbound method must be called with class instance 1st argument");
						x = NULL;
						break;
					}
				}
			}
			else
				Py_INCREF(func);
			if (PyFunction_Check(func)) {
				PyObject *co = PyFunction_GetCode(func);
				PyObject *globals = PyFunction_GetGlobals(func);
				PyObject *argdefs = PyFunction_GetDefaults(func);
				PyObject **d;
				int nd;
				if (argdefs != NULL) {
					d = &PyTuple_GET_ITEM(argdefs, 0);
					nd = ((PyTupleObject *)argdefs)->ob_size;
				}
				else {
					d = NULL;
					nd = 0;
				}
				x = eval_code2(
					(PyCodeObject *)co,
					globals, (PyObject *)NULL,
					stack_pointer-n, na,
					stack_pointer-2*nk, nk,
					d, nd,
					class);
			}
			else {
				PyObject *args = PyTuple_New(na);
				PyObject *kwdict = NULL;
				if (args == NULL) {
					x = NULL;
					break;
				}
				if (nk > 0) {
					kwdict = PyDict_New();
					if (kwdict == NULL) {
						x = NULL;
						break;
					}
					err = 0;
					while (--nk >= 0) {
						PyObject *value = POP();
						PyObject *key = POP();
						err = PyDict_SetItem(
							kwdict, key, value);
						Py_DECREF(key);
						Py_DECREF(value);
						if (err)
							break;
					}
					if (err) {
						Py_DECREF(args);
						Py_DECREF(kwdict);
						break;
					}
				}
				while (--na >= 0) {
					w = POP();
					PyTuple_SET_ITEM(args, na, w);
				}
				x = PyEval_CallObjectWithKeywords(
					func, args, kwdict);
				Py_DECREF(args);
				Py_XDECREF(kwdict);
			}
			Py_DECREF(func);
			while (stack_pointer > pfunc) {
				w = POP();
				Py_DECREF(w);
			}
			PUSH(x);
			if (x != NULL) continue;
			break;
		}
		
		case MAKE_FUNCTION:
			v = POP(); /* code object */
			x = PyFunction_New(v, f->f_globals);
			Py_DECREF(v);
			/* XXX Maybe this should be a separate opcode? */
			if (x != NULL && oparg > 0) {
				v = PyTuple_New(oparg);
				if (v == NULL) {
					Py_DECREF(x);
					x = NULL;
					break;
				}
				while (--oparg >= 0) {
					w = POP();
					PyTuple_SET_ITEM(v, oparg, w);
				}
				err = PyFunction_SetDefaults(x, v);
				Py_DECREF(v);
			}
			PUSH(x);
			break;

		case BUILD_SLICE:
			if (oparg == 3)
				w = POP();
			else
				w = NULL;
			v = POP();
			u = POP();
			x = PySlice_New(u, v, w);
			Py_DECREF(u);
			Py_DECREF(v);
			Py_XDECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;


		default:
			fprintf(stderr,
				"XXX lineno: %d, opcode: %d\n",
				f->f_lineno, opcode);
			PyErr_SetString(PyExc_SystemError, "unknown opcode");
			why = WHY_EXCEPTION;
			break;

#ifdef CASE_TOO_BIG
		}
#endif

		} /* switch */

	    on_error:
		
		/* Quickly continue if no error occurred */
		
		if (why == WHY_NOT) {
			if (err == 0 && x != NULL) {
#ifdef CHECKEXC
				if (PyErr_Occurred())
					fprintf(stderr,
						"XXX undetected error\n");
				else
#endif
					continue; /* Normal, fast path */
			}
			why = WHY_EXCEPTION;
			x = Py_None;
			err = 0;
		}

#ifdef CHECKEXC
		/* Double-check exception status */
		
		if (why == WHY_EXCEPTION || why == WHY_RERAISE) {
			if (!PyErr_Occurred()) {
				fprintf(stderr, "XXX ghost error\n");
				PyErr_SetString(PyExc_SystemError, "ghost error");
				why = WHY_EXCEPTION;
			}
		}
		else {
			if (PyErr_Occurred()) {
				fprintf(stderr,
					"XXX undetected error (why=%d)\n",
					why);
				why = WHY_EXCEPTION;
			}
		}
#endif

		/* Log traceback info if this is a real exception */
		
		if (why == WHY_EXCEPTION) {
			f->f_lasti = INSTR_OFFSET() - 1;
			if (HAS_ARG(opcode))
				f->f_lasti -= 2;
			PyTraceBack_Here(f);

			if (f->f_trace)
				call_exc_trace(&f->f_trace, &f->f_trace, f);
			if (_PySys_ProfileFunc)
				call_exc_trace(&_PySys_ProfileFunc, (PyObject**)0, f);
		}
		
		/* For the rest, treat WHY_RERAISE as WHY_EXCEPTION */
		
		if (why == WHY_RERAISE)
			why = WHY_EXCEPTION;

		/* Unwind stacks if a (pseudo) exception occurred */
		
		while (why != WHY_NOT && f->f_iblock > 0) {
			PyTryBlock *b = PyFrame_BlockPop(f);
			while (STACK_LEVEL() > b->b_level) {
				v = POP();
				Py_XDECREF(v);
			}
			if (b->b_type == SETUP_LOOP && why == WHY_BREAK) {
				why = WHY_NOT;
				JUMPTO(b->b_handler);
				break;
			}
			if (b->b_type == SETUP_FINALLY ||
			    (b->b_type == SETUP_EXCEPT &&
			     why == WHY_EXCEPTION)) {
				if (why == WHY_EXCEPTION) {
					PyObject *exc, *val, *tb;
					PyErr_Fetch(&exc, &val, &tb);
					if (val == NULL) {
						val = Py_None;
						Py_INCREF(val);
					}
					/* Make the raw exception data
					   available to the handler,
					   so a program can emulate the
					   Python main loop.  Don't do
					   this for 'finally'. */
					if (b->b_type == SETUP_EXCEPT) {
						PySys_SetObject("exc_traceback", tb);
						PySys_SetObject("exc_value", val);
						PySys_SetObject("exc_type", exc);
					}
					PUSH(tb);
					PUSH(val);
					PUSH(exc);
				}
				else {
					if (why == WHY_RETURN)
						PUSH(retval);
					v = PyInt_FromLong((long)why);
					PUSH(v);
				}
				why = WHY_NOT;
				JUMPTO(b->b_handler);
				break;
			}
		} /* unwind stack */

		/* End the loop if we still have an error (or return) */
		
		if (why != WHY_NOT)
			break;
		
	} /* main loop */
	
	/* Pop remaining stack entries */
	
	while (!EMPTY()) {
		v = POP();
		Py_XDECREF(v);
	}
	
	if (why != WHY_RETURN)
		retval = NULL;
	
	if (f->f_trace) {
		if (why == WHY_RETURN) {
			if (call_trace(&f->f_trace, &f->f_trace, f,
				       "return", retval)) {
				Py_XDECREF(retval);
				retval = NULL;
				why = WHY_EXCEPTION;
			}
		}
	}
	
	if (_PySys_ProfileFunc && why == WHY_RETURN) {
		if (call_trace(&_PySys_ProfileFunc, (PyObject**)0,
			       f, "return", retval)) {
			Py_XDECREF(retval);
			retval = NULL;
			why = WHY_EXCEPTION;
		}
	}

	--recursion_depth;

  fail: /* Jump here from prelude on failure */
	
	/* Restore previous frame and release the current one */

	current_frame = f->f_back;
	Py_DECREF(f);
	
	return retval;
}

/* Logic for the raise statement (too complicated for inlining).
   This *consumes* a reference count to each of its arguments. */
static enum why_code
do_raise(type, value, tb)
	PyObject *type, *value, *tb;
{
	/* We support the following forms of raise:
	   raise <class>, <classinstance>
	   raise <class>, <argument tuple>
	   raise <class>, None
	   raise <class>, <argument>
	   raise <classinstance>, None
	   raise <string>, <object>
	   raise <string>, None

	   An omitted second argument is the same as None.

	   In addition, raise <tuple>, <anything> is the same as
	   raising the tuple's first item (and it better have one!);
	   this rule is applied recursively.

	   Finally, an optional third argument can be supplied, which
	   gives the traceback to be substituted (useful when
	   re-raising an exception after examining it).  */

	/* First, check the traceback argument, replacing None with
	   NULL. */
	if (tb == Py_None) {
		Py_DECREF(tb);
		tb = NULL;
	}
	else if (tb != NULL && !PyTraceBack_Check(tb)) {
		PyErr_SetString(PyExc_TypeError,
			   "raise 3rd arg must be traceback or None");
		goto raise_error;
	}

	/* Next, replace a missing value with None */
	if (value == NULL) {
		value = Py_None;
		Py_INCREF(value);
	}

	/* Next, repeatedly, replace a tuple exception with its first item */
	while (PyTuple_Check(type) && PyTuple_Size(type) > 0) {
		PyObject *tmp = type;
		type = PyTuple_GET_ITEM(type, 0);
		Py_INCREF(type);
		Py_DECREF(tmp);
	}

	/* Now switch on the exception's type */
	if (PyString_Check(type)) {
		;
	}
	else if (PyClass_Check(type)) {
		/* Raising a class.  If the value is an instance, it
		   better be an instance of the class.  If it is not,
		   it will be used to create an instance. */
		if (PyInstance_Check(value)) {
			PyObject *inclass = (PyObject*)
				(((PyInstanceObject*)value)->in_class);
			if (!PyClass_IsSubclass(inclass, type)) {
				PyErr_SetString(PyExc_TypeError,
 "raise <class>, <instance> requires that <instance> is a member of <class>");
				goto raise_error;
			}
		}
		else {
			/* Go instantiate the class */
			PyObject *args, *res;
			if (value == Py_None)
				args = Py_BuildValue("()");
			else if (PyTuple_Check(value)) {
				Py_INCREF(value);
				args = value;
			}
			else
				args = Py_BuildValue("(O)", value);
			if (args == NULL)
				goto raise_error;
			res = PyEval_CallObject(type, args);
			Py_DECREF(args);
			if (res == NULL)
				goto raise_error;
			Py_DECREF(value);
			value = res;
		}
	}
	else if (PyInstance_Check(type)) {
		/* Raising an instance.  The value should be a dummy. */
		if (value != Py_None) {
			PyErr_SetString(PyExc_TypeError,
			  "instance exception may not have a separate value");
			goto raise_error;
		}
		else {
			/* Normalize to raise <class>, <instance> */
			Py_DECREF(value);
			value = type;
			type = (PyObject*) ((PyInstanceObject*)type)->in_class;
			Py_INCREF(type);
		}
	}
	else {
		/* Not something you can raise.  You get an exception
		   anyway, just not what you specified :-) */
		PyErr_SetString(PyExc_TypeError,
		    "exceptions must be strings, classes, or instances");
		goto raise_error;
	}
	PyErr_Restore(type, value, tb);
	if (tb == NULL)
		return WHY_EXCEPTION;
	else
		return WHY_RERAISE;
 raise_error:
	Py_XDECREF(value);
	Py_XDECREF(type);
	Py_XDECREF(tb);
	return WHY_EXCEPTION;
}

#ifdef LLTRACE
static int
prtrace(v, str)
	PyObject *v;
	char *str;
{
	printf("%s ", str);
	if (PyObject_Print(v, stdout, 0) != 0)
		PyErr_Clear(); /* Don't know what else to do */
	printf("\n");
}
#endif

static void
call_exc_trace(p_trace, p_newtrace, f)
	PyObject **p_trace, **p_newtrace;
	PyFrameObject *f;
{
	PyObject *type, *value, *traceback, *arg;
	int err;
	PyErr_Fetch(&type, &value, &traceback);
	if (value == NULL) {
		value = Py_None;
		Py_INCREF(value);
	}
	arg = Py_BuildValue("(OOO)", type, value, traceback);
	if (arg == NULL) {
		PyErr_Restore(type, value, traceback);
		return;
	}
	err = call_trace(p_trace, p_newtrace, f, "exception", arg);
	Py_DECREF(arg);
	if (err == 0)
		PyErr_Restore(type, value, traceback);
	else {
		Py_XDECREF(type);
		Py_XDECREF(value);
		Py_XDECREF(traceback);
	}
}

static int
call_trace(p_trace, p_newtrace, f, msg, arg)
	PyObject **p_trace; /* in/out; may not be NULL;
			     may not point to NULL variable initially */
	PyObject **p_newtrace; /* in/out; may be NULL;
				may point to NULL variable;
				may be same variable as p_newtrace */
	PyFrameObject *f;
	char *msg;
	PyObject *arg;
{
	PyObject *args, *what;
	PyObject *res = NULL;
	static int tracing = 0;
	
	if (tracing) {
		/* Don't do recursive traces */
		if (p_newtrace) {
			Py_XDECREF(*p_newtrace);
			*p_newtrace = NULL;
		}
		return 0;
	}
	
	args = PyTuple_New(3);
	if (args == NULL)
		goto Py_Cleanup;
	what = PyString_FromString(msg);
	if (what == NULL)
		goto Py_Cleanup;
	Py_INCREF(f);
	PyTuple_SET_ITEM(args, 0, (PyObject *)f);
	PyTuple_SET_ITEM(args, 1, what);
	if (arg == NULL)
		arg = Py_None;
	Py_INCREF(arg);
	PyTuple_SET_ITEM(args, 2, arg);
	tracing++;
	PyFrame_FastToLocals(f);
	res = PyEval_CallObject(*p_trace, args); /* May clear *p_trace! */
	PyFrame_LocalsToFast(f, 1);
	tracing--;
 Py_Cleanup:
	Py_XDECREF(args);
	if (res == NULL) {
		/* The trace proc raised an exception */
		PyTraceBack_Here(f);
		Py_XDECREF(*p_trace);
		*p_trace = NULL;
		if (p_newtrace) {
			Py_XDECREF(*p_newtrace);
			*p_newtrace = NULL;
		}
		return -1;
	}
	else {
		if (p_newtrace) {
			Py_XDECREF(*p_newtrace);
			if (res == Py_None)
				*p_newtrace = NULL;
			else {
				Py_INCREF(res);
				*p_newtrace = res;
			}
		}
		Py_DECREF(res);
		return 0;
	}
}

PyObject *
PyEval_GetBuiltins()
{
	if (current_frame == NULL)
		return PyBuiltin_GetModule();
	else
		return current_frame->f_builtins;
}

PyObject *
PyEval_GetLocals()
{
	if (current_frame == NULL)
		return NULL;
	PyFrame_FastToLocals(current_frame);
	return current_frame->f_locals;
}

PyObject *
PyEval_GetGlobals()
{
	if (current_frame == NULL)
		return NULL;
	else
		return current_frame->f_globals;
}

PyObject *
PyEval_GetFrame()
{
	return (PyObject *)current_frame;
}

int
PyEval_GetRestricted()
{
	return current_frame == NULL ? 0 : current_frame->f_restricted;
}

void
Py_FlushLine()
{
	PyObject *f = PySys_GetObject("stdout");
	if (PyFile_SoftSpace(f, 0))
		PyFile_WriteString("\n", f);
}


#define BINOP(opname, ropname, thisfunc) \
	if (!PyInstance_Check(v) && !PyInstance_Check(w)) \
		; \
	else \
		return PyInstance_DoBinOp(v, w, opname, ropname, thisfunc)


static PyObject *
or(v, w)
	PyObject *v, *w;
{
	BINOP("__or__", "__ror__", or);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_or) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for |");
	return NULL;
}

static PyObject *
xor(v, w)
	PyObject *v, *w;
{
	BINOP("__xor__", "__rxor__", xor);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_xor) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for ^");
	return NULL;
}

static PyObject *
and(v, w)
	PyObject *v, *w;
{
	BINOP("__and__", "__rand__", and);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_and) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for &");
	return NULL;
}

static PyObject *
lshift(v, w)
	PyObject *v, *w;
{
	BINOP("__lshift__", "__rlshift__", lshift);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_lshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for <<");
	return NULL;
}

static PyObject *
rshift(v, w)
	PyObject *v, *w;
{
	BINOP("__rshift__", "__rrshift__", rshift);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x = NULL;
		PyObject * (*f) Py_FPROTO((PyObject *, PyObject *));
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_rshift) != NULL)
			x = (*f)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		if (f != NULL)
			return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for >>");
	return NULL;
}

static PyObject *
add(v, w)
	PyObject *v, *w;
{
	BINOP("__add__", "__radd__", add);
	if (v->ob_type->tp_as_sequence != NULL)
		return (*v->ob_type->tp_as_sequence->sq_concat)(v, w);
	else if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_add)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for +");
	return NULL;
}

static PyObject *
sub(v, w)
	PyObject *v, *w;
{
	BINOP("__sub__", "__rsub__", sub);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_subtract)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for -");
	return NULL;
}

static PyObject *
mul(v, w)
	PyObject *v, *w;
{
	PyTypeObject *tp;
	tp = v->ob_type;
	BINOP("__mul__", "__rmul__", mul);
	if (tp->tp_as_number != NULL &&
	    w->ob_type->tp_as_sequence != NULL &&
	    !PyInstance_Check(v)) {
		/* number*sequence -- swap v and w */
		PyObject *tmp = v;
		v = w;
		w = tmp;
		tp = v->ob_type;
	}
	if (tp->tp_as_number != NULL) {
		PyObject *x;
		if (PyInstance_Check(v)) {
			/* Instances of user-defined classes get their
			   other argument uncoerced, so they may
			   implement sequence*number as well as
			   number*number. */
			Py_INCREF(v);
			Py_INCREF(w);
		}
		else if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_multiply)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	if (tp->tp_as_sequence != NULL) {
		if (!PyInt_Check(w)) {
			PyErr_SetString(PyExc_TypeError,
				"can't multiply sequence with non-int");
			return NULL;
		}
		return (*tp->tp_as_sequence->sq_repeat)
						(v, (int)PyInt_AsLong(w));
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for *");
	return NULL;
}

static PyObject *
divide(v, w)
	PyObject *v, *w;
{
	BINOP("__div__", "__rdiv__", divide);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_divide)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for /");
	return NULL;
}

static PyObject *
mod(v, w)
	PyObject *v, *w;
{
	if (PyString_Check(v)) {
		return PyString_Format(v, w);
	}
	BINOP("__mod__", "__rmod__", mod);
	if (v->ob_type->tp_as_number != NULL) {
		PyObject *x;
		if (PyNumber_Coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_remainder)(v, w);
		Py_DECREF(v);
		Py_DECREF(w);
		return x;
	}
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for %");
	return NULL;
}

static PyObject *
powerop(v, w)
	PyObject *v, *w;
{
	PyObject *res;
	BINOP("__pow__", "__rpow__", powerop);
	if (v->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		PyErr_SetString(PyExc_TypeError, "pow() requires numeric arguments");
		return NULL;
	}
	if (PyNumber_Coerce(&v, &w) != 0)
		return NULL;
	res = (*v->ob_type->tp_as_number->nb_power)(v, w, Py_None);
	Py_DECREF(v);
	Py_DECREF(w);
	return res;
}

static PyObject *
neg(v)
	PyObject *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_negative)(v);
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for unary -");
	return NULL;
}

static PyObject *
pos(v)
	PyObject *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_positive)(v);
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for unary +");
	return NULL;
}

static PyObject *
invert(v)
	PyObject *v;
{
	PyObject * (*f) Py_FPROTO((PyObject *));
	if (v->ob_type->tp_as_number != NULL &&
		(f = v->ob_type->tp_as_number->nb_invert) != NULL)
		return (*f)(v);
	PyErr_SetString(PyExc_TypeError, "bad operand type(s) for unary ~");
	return NULL;
}

static PyObject *
not(v)
	PyObject *v;
{
	int outcome = PyObject_IsTrue(v);
	PyObject *w;
	if (outcome < 0)
		return NULL;
	if (outcome == 0)
		w = Py_True;
	else
		w = Py_False;
	Py_INCREF(w);
	return w;
}


/* External interface to call any callable object.
   The arg must be a tuple or NULL. */

PyObject *
PyEval_CallObject(func, arg)
	PyObject *func;
	PyObject *arg;
{
	return PyEval_CallObjectWithKeywords(func, arg, (PyObject *)NULL);
}

PyObject *
PyEval_CallObjectWithKeywords(func, arg, kw)
	PyObject *func;
	PyObject *arg;
	PyObject *kw;
{
        ternaryfunc call;
        PyObject *result;

	if (arg == NULL)
		arg = PyTuple_New(0);
	else if (!PyTuple_Check(arg)) {
		PyErr_SetString(PyExc_TypeError, "argument list must be a tuple");
		return NULL;
	}
	else
		Py_INCREF(arg);

	if (kw != NULL && !PyDict_Check(kw)) {
		PyErr_SetString(PyExc_TypeError, "keyword list must be a dictionary");
		return NULL;
	}

        if ((call = func->ob_type->tp_call) != NULL)
                result = (*call)(func, arg, kw);
        else if (PyMethod_Check(func) || PyFunction_Check(func))
		result = call_function(func, arg, kw);
	else
		result = call_builtin(func, arg, kw);

	Py_DECREF(arg);
	
        if (result == NULL && !PyErr_Occurred())
		PyErr_SetString(PyExc_SystemError,
			   "NULL result without error in call_object");
        
        return result;
}

static PyObject *
call_builtin(func, arg, kw)
	PyObject *func;
	PyObject *arg;
	PyObject *kw;
{
	if (PyCFunction_Check(func)) {
		PyCFunction meth = PyCFunction_GetFunction(func);
		PyObject *self = PyCFunction_GetSelf(func);
		int flags = PyCFunction_GetFlags(func);
		if (!(flags & METH_VARARGS)) {
			int size = PyTuple_Size(arg);
			if (size == 1)
				arg = PyTuple_GET_ITEM(arg, 0);
			else if (size == 0)
				arg = NULL;
		}
		if (flags & METH_KEYWORDS)
			return (*(PyCFunctionWithKeywords)meth)(self, arg, kw);
		if (kw != NULL && PyDict_Size(kw) != 0) {
			PyErr_SetString(PyExc_TypeError,
				   "this function takes no keyword arguments");
			return NULL;
		}
		return (*meth)(self, arg);
	}
	if (PyClass_Check(func)) {
		return PyInstance_New(func, arg, kw);
	}
	if (PyInstance_Check(func)) {
	        PyObject *res, *call = PyObject_GetAttrString(func,"__call__");
		if (call == NULL) {
			PyErr_Clear();
			PyErr_SetString(PyExc_AttributeError,
				   "no __call__ method defined");
			return NULL;
		}
		res = PyEval_CallObjectWithKeywords(call, arg, kw);
		Py_DECREF(call);
		return res;
	}
	PyErr_SetString(PyExc_TypeError, "call of non-function");
	return NULL;
}

static PyObject *
call_function(func, arg, kw)
	PyObject *func;
	PyObject *arg;
	PyObject *kw;
{
	PyObject *class = NULL; /* == owner */
	PyObject *argdefs;
	PyObject **d, **k;
	int nk, nd;
	PyObject *result;
	
	if (kw != NULL && !PyDict_Check(kw)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	
	if (PyMethod_Check(func)) {
		PyObject *self = PyMethod_Self(func);
		class = PyMethod_Class(func);
		func = PyMethod_Function(func);
		if (self == NULL) {
			/* Unbound methods must be called with an instance of
			   the class (or a derived class) as first argument */
			if (PyTuple_Size(arg) >= 1) {
				self = PyTuple_GET_ITEM(arg, 0);
				if (self != NULL &&
				    PyInstance_Check(self) &&
				    PyClass_IsSubclass((PyObject *)
				      (((PyInstanceObject *)self)->in_class),
					       class))
					/* Handy-dandy */ ;
				else
					self = NULL;
			}
			if (self == NULL) {
				PyErr_SetString(PyExc_TypeError,
	   "unbound method must be called with class instance 1st argument");
				return NULL;
			}
			Py_INCREF(arg);
		}
		else {
			int argcount = PyTuple_Size(arg);
			PyObject *newarg = PyTuple_New(argcount + 1);
			int i;
			if (newarg == NULL)
				return NULL;
			Py_INCREF(self);
			PyTuple_SET_ITEM(newarg, 0, self);
			for (i = 0; i < argcount; i++) {
				PyObject *v = PyTuple_GET_ITEM(arg, i);
				Py_XINCREF(v);
				PyTuple_SET_ITEM(newarg, i+1, v);
			}
			arg = newarg;
		}
	}
	else {
		if (!PyFunction_Check(func)) {
			PyErr_SetString(PyExc_TypeError, "call of non-function");
			return NULL;
		}
		Py_INCREF(arg);
	}
	
	argdefs = PyFunction_GetDefaults(func);
	if (argdefs != NULL && PyTuple_Check(argdefs)) {
		d = &PyTuple_GET_ITEM((PyTupleObject *)argdefs, 0);
		nd = PyTuple_Size(argdefs);
	}
	else {
		d = NULL;
		nd = 0;
	}
	
	if (kw != NULL) {
		int pos, i;
		nk = PyDict_Size(kw);
		k = PyMem_NEW(PyObject *, 2*nk);
		if (k == NULL) {
			PyErr_NoMemory();
			Py_DECREF(arg);
			return NULL;
		}
		pos = i = 0;
		while (PyDict_Next(kw, &pos, &k[i], &k[i+1]))
			i += 2;
		nk = i/2;
		/* XXX This is broken if the caller deletes dict items! */
	}
	else {
		k = NULL;
		nk = 0;
	}
	
	result = eval_code2(
		(PyCodeObject *)PyFunction_GetCode(func),
		PyFunction_GetGlobals(func), (PyObject *)NULL,
		&PyTuple_GET_ITEM(arg, 0), PyTuple_Size(arg),
		k, nk,
		d, nd,
		class);
	
	Py_DECREF(arg);
	PyMem_XDEL(k);
	
	return result;
}

#define SLICE_ERROR_MSG \
	"standard sequence type does not support step size other than one"

static PyObject *
apply_subscript(v, w)
	PyObject *v, *w;
{
	PyTypeObject *tp = v->ob_type;
	if (tp->tp_as_sequence == NULL && tp->tp_as_mapping == NULL) {
		PyErr_SetString(PyExc_TypeError, "unsubscriptable object");
		return NULL;
	}
	if (tp->tp_as_mapping != NULL) {
		return (*tp->tp_as_mapping->mp_subscript)(v, w);
	}
	else {
		int i;
		if (!PyInt_Check(w)) {		  
			if (PySlice_Check(w)) {
			        PyErr_SetString(PyExc_ValueError, SLICE_ERROR_MSG); 
			} else {
				PyErr_SetString(PyExc_TypeError,
					   "sequence subscript not int");
			}	
			return NULL;
		}
		i = PyInt_AsLong(w);
		if (i < 0) {
			int len = (*tp->tp_as_sequence->sq_length)(v);
			if (len < 0)
				return NULL;
			i += len;
		}
		return (*tp->tp_as_sequence->sq_item)(v, i);
	}
}

static PyObject *
loop_subscript(v, w)
	PyObject *v, *w;
{
	PySequenceMethods *sq = v->ob_type->tp_as_sequence;
	int i;
	if (sq == NULL) {
		PyErr_SetString(PyExc_TypeError, "loop over non-sequence");
		return NULL;
	}
	i = PyInt_AsLong(w);
	v = (*sq->sq_item)(v, i);
	if (v)
		return v;
	if (PyErr_Occurred() == PyExc_IndexError)
		PyErr_Clear();
	return NULL;
}

static int
slice_index(v, isize, pi)
	PyObject *v;
	int isize;
	int *pi;
{
	if (v != NULL) {
		if (!PyInt_Check(v)) {
			PyErr_SetString(PyExc_TypeError, "slice index must be int");
			return -1;
		}
		*pi = PyInt_AsLong(v);
		if (*pi < 0)
			*pi += isize;
	}
	return 0;
}

static PyObject *
apply_slice(u, v, w) /* return u[v:w] */
	PyObject *u, *v, *w;
{
	PyTypeObject *tp = u->ob_type;
	int ilow, ihigh, isize;
	if (tp->tp_as_sequence == NULL) {
		PyErr_SetString(PyExc_TypeError, "only sequences can be sliced");
		return NULL;
	}
	ilow = 0;
	isize = ihigh = (*tp->tp_as_sequence->sq_length)(u);
	if (isize < 0)
		return NULL;
	if (slice_index(v, isize, &ilow) != 0)
		return NULL;
	if (slice_index(w, isize, &ihigh) != 0)
		return NULL;
	return (*tp->tp_as_sequence->sq_slice)(u, ilow, ihigh);
}

static int
assign_subscript(w, key, v) /* w[key] = v */
	PyObject *w;
	PyObject *key;
	PyObject *v;
{
	PyTypeObject *tp = w->ob_type;
	PySequenceMethods *sq;
	PyMappingMethods *mp;
	int (*func1)();
	int (*func2)();
	if ((mp = tp->tp_as_mapping) != NULL &&
			(func1 = mp->mp_ass_subscript) != NULL) {
		return (*func1)(w, key, v);
	}
	else if ((sq = tp->tp_as_sequence) != NULL &&
			(func2 = sq->sq_ass_item) != NULL) {
		if (!PyInt_Check(key)) {
			PyErr_SetString(PyExc_TypeError,
			"sequence subscript must be integer (assign or del)");
			return -1;
		}
		else {
			int i = PyInt_AsLong(key);
			if (i < 0) {
				int len = (*sq->sq_length)(w);
				if (len < 0)
					return -1;
				i += len;
			}
			return (*func2)(w, i, v);
		}
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"can't assign to this subscripted object");
		return -1;
	}
}

static int
assign_slice(u, v, w, x) /* u[v:w] = x */
	PyObject *u, *v, *w, *x;
{
	PySequenceMethods *sq = u->ob_type->tp_as_sequence;
	int ilow, ihigh, isize;
	if (sq == NULL) {
		PyErr_SetString(PyExc_TypeError, "assign to slice of non-sequence");
		return -1;
	}
	if (sq == NULL || sq->sq_ass_slice == NULL) {
		PyErr_SetString(PyExc_TypeError, "unassignable slice");
		return -1;
	}
	ilow = 0;
	isize = ihigh = (*sq->sq_length)(u);
	if (isize < 0)
		return -1;
	if (slice_index(v, isize, &ilow) != 0)
		return -1;
	if (slice_index(w, isize, &ihigh) != 0)
		return -1;
	return (*sq->sq_ass_slice)(u, ilow, ihigh, x);
}

static int
cmp_exception(err, v)
	PyObject *err, *v;
{
	if (PyTuple_Check(v)) {
		int i, n;
		n = PyTuple_Size(v);
		for (i = 0; i < n; i++) {
			/* Test recursively */
			if (cmp_exception(err, PyTuple_GET_ITEM(v, i)))
				return 1;
		}
		return 0;
	}
	if (PyClass_Check(v) && PyClass_Check(err))
		return PyClass_IsSubclass(err, v);
	return err == v;
}

static int
cmp_member(v, w)
	PyObject *v, *w;
{
	int i, cmp;
	PyObject *x;
	PySequenceMethods *sq;
	/* Special case for char in string */
	if (PyString_Check(w)) {
		register char *s, *end;
		register char c;
		if (!PyString_Check(v) || PyString_Size(v) != 1) {
			PyErr_SetString(PyExc_TypeError,
			    "string member test needs char left operand");
			return -1;
		}
		c = PyString_AsString(v)[0];
		s = PyString_AsString(w);
		end = s + PyString_Size(w);
		while (s < end) {
			if (c == *s++)
				return 1;
		}
		return 0;
	}
	sq = w->ob_type->tp_as_sequence;
	if (sq == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"'in' or 'not in' needs sequence right argument");
		return -1;
	}
	for (i = 0; ; i++) {
		x = (*sq->sq_item)(w, i);
		if (x == NULL) {
			if (PyErr_Occurred() == PyExc_IndexError) {
				PyErr_Clear();
				break;
			}
			return -1;
		}
		cmp = PyObject_Compare(v, x);
		Py_XDECREF(x);
		if (cmp == 0)
			return 1;
	}
	return 0;
}

static PyObject *
cmp_outcome(op, v, w)
	int op;
	register PyObject *v;
	register PyObject *w;
{
	register int cmp;
	register int res = 0;
	switch (op) {
	case IS:
	case IS_NOT:
		res = (v == w);
		if (op == (int) IS_NOT)
			res = !res;
		break;
	case IN:
	case NOT_IN:
		res = cmp_member(v, w);
		if (res < 0)
			return NULL;
		if (op == (int) NOT_IN)
			res = !res;
		break;
	case EXC_MATCH:
		res = cmp_exception(v, w);
		break;
	default:
		cmp = PyObject_Compare(v, w);
		switch (op) {
		case LT: res = cmp <  0; break;
		case LE: res = cmp <= 0; break;
		case EQ: res = cmp == 0; break;
		case NE: res = cmp != 0; break;
		case GT: res = cmp >  0; break;
		case GE: res = cmp >= 0; break;
		/* XXX no default? (res is initialized to 0 though) */
		}
	}
	v = res ? Py_True : Py_False;
	Py_INCREF(v);
	return v;
}

static int
import_from(locals, v, name)
	PyObject *locals;
	PyObject *v;
	PyObject *name;
{
	PyObject *w, *x;
	if (!PyModule_Check(v)) {
		PyErr_SetString(PyExc_TypeError, "import-from requires module object");
		return -1;
	}
	w = PyModule_GetDict(v);
	if (PyString_AsString(name)[0] == '*') {
		int pos, err;
		PyObject *name, *value;
		pos = 0;
		while (PyDict_Next(w, &pos, &name, &value)) {
			if (!PyString_Check(name) ||
			    PyString_AsString(name)[0] == '_')
				continue;
			Py_INCREF(value);
			err = PyDict_SetItem(locals, name, value);
			Py_DECREF(value);
			if (err != 0)
				return -1;
		}
		return 0;
	}
	else {
		x = PyDict_GetItem(w, name);
		if (x == NULL) {
			char buf[250];
			sprintf(buf, "cannot import name %.230s",
				PyString_AsString(name));
			PyErr_SetString(PyExc_ImportError, buf);
			return -1;
		}
		else
			return PyDict_SetItem(locals, name, x);
	}
}

static PyObject *
build_class(methods, bases, name)
	PyObject *methods; /* dictionary */
	PyObject *bases;  /* tuple containing classes */
	PyObject *name;   /* string */
{
	int i;
	if (!PyTuple_Check(bases)) {
		PyErr_SetString(PyExc_SystemError, "build_class with non-tuple bases");
		return NULL;
	}
	if (!PyDict_Check(methods)) {
		PyErr_SetString(PyExc_SystemError, "build_class with non-dictionary");
		return NULL;
	}
	if (!PyString_Check(name)) {
		PyErr_SetString(PyExc_SystemError, "build_class witn non-string name");
		return NULL;
	}
	for (i = PyTuple_Size(bases); --i >= 0; ) {
		PyObject *base = PyTuple_GET_ITEM(bases, i);
		if (!PyClass_Check(base)) {
			/* Call the base's *type*, if it is callable.
			   This code is a hook for Donald Beaudry's
			   and Jim Fulton's type extensions.  In
			   unexended Python it will never be triggered
			   since its types are not callable. */
			if (base->ob_type->ob_type->tp_call) {
			  	PyObject *args;
				PyObject *class;
				args = Py_BuildValue("(OOO)", name, bases, methods);
				class = PyEval_CallObject((PyObject *)base->ob_type,
						    args);
				Py_DECREF(args);
				return class;
			}
			PyErr_SetString(PyExc_TypeError,
				"base is not a class object");
			return NULL;
		}
	}
	return PyClass_New(bases, methods, name);
}

static int
exec_statement(prog, globals, locals)
	PyObject *prog;
	PyObject *globals;
	PyObject *locals;
{
	char *s;
	int n;
	PyObject *v;
	int plain = 0;

	if (PyTuple_Check(prog) && globals == Py_None && locals == Py_None &&
	    ((n = PyTuple_Size(prog)) == 2 || n == 3)) {
		/* Backward compatibility hack */
		globals = PyTuple_GetItem(prog, 1);
		if (n == 3)
			locals = PyTuple_GetItem(prog, 2);
		prog = PyTuple_GetItem(prog, 0);
	}
	if (globals == Py_None) {
		globals = PyEval_GetGlobals();
		if (locals == Py_None) {
			locals = PyEval_GetLocals();
			plain = 1;
		}
	}
	else if (locals == Py_None)
		locals = globals;
	if (!PyString_Check(prog) &&
	    !PyCode_Check(prog) &&
	    !PyFile_Check(prog)) {
		PyErr_SetString(PyExc_TypeError,
			   "exec 1st arg must be string, code or file object");
		return -1;
	}
	if (!PyDict_Check(globals) || !PyDict_Check(locals)) {
		PyErr_SetString(PyExc_TypeError,
		    "exec 2nd/3rd args must be dict or None");
		return -1;
	}
	if (PyDict_GetItemString(globals, "__builtins__") == NULL)
		PyDict_SetItemString(globals, "__builtins__", current_frame->f_builtins);
	if (PyCode_Check(prog)) {
		if (PyEval_EvalCode((PyCodeObject *) prog, globals, locals) == NULL)
			return -1;
		return 0;
	}
	if (PyFile_Check(prog)) {
		FILE *fp = PyFile_AsFile(prog);
		char *name = PyString_AsString(PyFile_Name(prog));
		if (PyRun_File(fp, name, file_input, globals, locals) == NULL)
			return -1;
		return 0;
	}
	s = PyString_AsString(prog);
	if (strlen(s) != PyString_Size(prog)) {
		PyErr_SetString(PyExc_ValueError, "embedded '\\0' in exec string");
		return -1;
	}
	v = PyRun_String(s, file_input, globals, locals);
	if (v == NULL)
		return -1;
	Py_DECREF(v);
	if (plain)
		PyFrame_LocalsToFast(current_frame, 0);
	return 0;
}

/* Hack for ni.py */
static PyObject *
find_from_args(f, nexti)
	PyFrameObject *f;
	int nexti;
{
	int opcode;
	int oparg;
	PyObject *list, *name;
	unsigned char *next_instr;
	
	next_instr = GETUSTRINGVALUE(f->f_code->co_code) + nexti;
	opcode = (*next_instr++);
	if (opcode != IMPORT_FROM) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	list = PyList_New(0);
	if (list == NULL)
		return NULL;
	
	do {
		oparg = (next_instr[1]<<8) + next_instr[0];
		next_instr += 2;
		name = Getnamev(f, oparg);
		if (PyList_Append(list, name) < 0) {
			Py_DECREF(list);
			break;
		}
		opcode = (*next_instr++);
	} while (opcode == IMPORT_FROM);
	
	return list;
}


#ifdef DYNAMIC_EXECUTION_PROFILE

PyObject *
getarray(a)
	long a[256];
{
	int i;
	PyObject *l = PyList_New(256);
	if (l == NULL) return NULL;
	for (i = 0; i < 256; i++) {
		PyObject *x = PyInt_FromLong(a[i]);
		if (x == NULL) {
			Py_DECREF(l);
			return NULL;
		}
		PyList_SetItem(l, i, x);
	}
	for (i = 0; i < 256; i++)
		a[i] = 0;
	return l;
}

PyObject *
_Py_GetDXProfile(self, args)
	PyObject *self, *args;
{
#ifndef DXPAIRS
	return getarray(dxp);
#else
	int i;
	PyObject *l = PyList_New(257);
	if (l == NULL) return NULL;
	for (i = 0; i < 257; i++) {
		PyObject *x = getarray(dxpairs[i]);
		if (x == NULL) {
			Py_DECREF(l);
			return NULL;
		}
		PyList_SetItem(l, i, x);
	}
	return l;
#endif
}

#endif
