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

#include "allobjects.h"

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

static object *eval_code2 PROTO((codeobject *,
				 object *, object *,
				 object **, int,
				 object **, int,
				 object **, int,
				 object *));
#ifdef LLTRACE
static int prtrace PROTO((object *, char *));
#endif
static void call_exc_trace PROTO((object **, object**, frameobject *));
static int call_trace
	PROTO((object **, object **, frameobject *, char *, object *));
static object *add PROTO((object *, object *));
static object *sub PROTO((object *, object *));
static object *powerop PROTO((object *, object *));
static object *mul PROTO((object *, object *));
static object *divide PROTO((object *, object *));
static object *mod PROTO((object *, object *));
static object *neg PROTO((object *));
static object *pos PROTO((object *));
static object *not PROTO((object *));
static object *invert PROTO((object *));
static object *lshift PROTO((object *, object *));
static object *rshift PROTO((object *, object *));
static object *and PROTO((object *, object *));
static object *xor PROTO((object *, object *));
static object *or PROTO((object *, object *));
static object *call_builtin PROTO((object *, object *, object *));
static object *call_function PROTO((object *, object *, object *));
static object *apply_subscript PROTO((object *, object *));
static object *loop_subscript PROTO((object *, object *));
static int slice_index PROTO((object *, int, int *));
static object *apply_slice PROTO((object *, object *, object *));
static int assign_subscript PROTO((object *, object *, object *));
static int assign_slice PROTO((object *, object *, object *, object *));
static int cmp_exception PROTO((object *, object *));
static int cmp_member PROTO((object *, object *));
static object *cmp_outcome PROTO((int, object *, object *));
static int import_from PROTO((object *, object *, object *));
static object *build_class PROTO((object *, object *, object *));
static int exec_statement PROTO((object *, object *, object *));
static object *find_from_args PROTO((frameobject *, int));


/* Pointer to current frame, used to link new frames to */

static frameobject *current_frame;

#ifdef WITH_THREAD

#include <errno.h>
#include "thread.h"

static type_lock interpreter_lock = 0;
static long main_thread = 0;

void
init_save_thread()
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

object *
save_thread()
{
#ifdef WITH_THREAD
	if (interpreter_lock) {
		object *res;
		res = (object *)current_frame;
		current_frame = NULL;
		release_lock(interpreter_lock);
		return res;
	}
#endif
	return NULL;
}

void
restore_thread(x)
	object *x;
{
#ifdef WITH_THREAD
	if (interpreter_lock) {
		int err;
		err = errno;
		acquire_lock(interpreter_lock, 1);
		errno = err;
		current_frame = (frameobject *)x;
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
	int (*func) PROTO((ANY *));
	ANY *arg;
} pendingcalls[NPENDINGCALLS];
static volatile int pendingfirst = 0;
static volatile int pendinglast = 0;

int
Py_AddPendingCall(func, arg)
	int (*func) PROTO((ANY *));
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
		int (*func) PROTO((ANY *));
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

static enum why_code do_raise PROTO((object *, object *, object *));


/* Backward compatible interface */

object *
eval_code(co, globals, locals)
	codeobject *co;
	object *globals;
	object *locals;
{
	return eval_code2(co,
			  globals, locals,
			  (object **)NULL, 0,
			  (object **)NULL, 0,
			  (object **)NULL, 0,
			  (object *)NULL);
}


/* Interpreter main loop */

#ifndef MAX_RECURSION_DEPTH
#define MAX_RECURSION_DEPTH 10000
#endif

static int recursion_depth = 0;

static object *
eval_code2(co, globals, locals,
	   args, argcount, kws, kwcount, defs, defcount, owner)
	codeobject *co;
	object *globals;
	object *locals;
	object **args;
	int argcount;
	object **kws; /* length: 2*kwcount */
	int kwcount;
	object **defs;
	int defcount;
	object *owner;
{
	register unsigned char *next_instr;
	register int opcode = 0; /* Current opcode */
	register int oparg = 0;	/* Current opcode argument, if any */
	register object **stack_pointer;
	register enum why_code why; /* Reason for block stack unwind */
	register int err;	/* Error status -- nonzero if error */
	register object *x;	/* Result object -- NULL if error */
	register object *v;	/* Temporary objects popped off stack */
	register object *w;
	register object *u;
	register object *t;
	register frameobject *f; /* Current frame */
	register object **fastlocals = NULL;
	object *retval = NULL;	/* Return value */
#ifdef LLTRACE
	int lltrace;
#endif
#if defined(Py_DEBUG) || defined(LLTRACE)
	/* Make it easier to find out where we are with a debugger */
	char *filename = getstringvalue(co->co_filename);
#endif

/* Code access macros */

#define GETCONST(i)	Getconst(f, i)
#define GETNAME(i)	Getname(f, i)
#define GETNAMEV(i)	Getnamev(f, i)
#define FIRST_INSTR()	(GETUSTRINGVALUE(f->f_code->co_code))
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
#define SETLOCAL(i, value)	do { XDECREF(GETLOCAL(i)); \
				     GETLOCAL(i) = value; } while (0)

#ifdef USE_STACKCHECK
	if (recursion_depth%10 == 0 && PyOS_CheckStack()) {
		err_setstr(MemoryError, "Stack overflow");
		return NULL;
	}
#endif

	if (globals == NULL) {
		err_setstr(SystemError, "eval_code2: NULL globals");
		return NULL;
	}

#ifdef LLTRACE
	lltrace = dictlookup(globals, "__lltrace__") != NULL;
#endif

	f = newframeobject(
			current_frame,		/*back*/
			co,			/*code*/
			globals,		/*globals*/
			locals);		/*locals*/
	if (f == NULL)
		return NULL;

	current_frame = f;

	if (co->co_nlocals > 0)
		fastlocals = f->f_localsplus;

	if (co->co_argcount > 0 ||
	    co->co_flags & (CO_VARARGS | CO_VARKEYWORDS)) {
		int i;
		int n = argcount;
		object *kwdict = NULL;
		if (co->co_flags & CO_VARKEYWORDS) {
			kwdict = newmappingobject();
			if (kwdict == NULL)
				goto fail;
		}
		if (argcount > co->co_argcount) {
			if (!(co->co_flags & CO_VARARGS)) {
				err_setstr(TypeError, "too many arguments");
				goto fail;
			}
			n = co->co_argcount;
		}
		for (i = 0; i < n; i++) {
			x = args[i];
			INCREF(x);
			SETLOCAL(i, x);
		}
		if (co->co_flags & CO_VARARGS) {
			u = newtupleobject(argcount - n);
			for (i = n; i < argcount; i++) {
				x = args[i];
				INCREF(x);
				SETTUPLEITEM(u, i-n, x);
			}
			SETLOCAL(co->co_argcount, u);
		}
		for (i = 0; i < kwcount; i++) {
			object *keyword = kws[2*i];
			object *value = kws[2*i + 1];
			int j;
			/* XXX slow -- speed up using dictionary? */
			for (j = 0; j < co->co_argcount; j++) {
				object *nm = GETTUPLEITEM(co->co_varnames, j);
				if (cmpobject(keyword, nm) == 0)
					break;
			}
			if (j >= co->co_argcount) {
				if (kwdict == NULL) {
					err_setval(TypeError, keyword);
					goto fail;
				}
				mappinginsert(kwdict, keyword, value);
			}
			else {
				if (GETLOCAL(j) != NULL) {
					err_setstr(TypeError,
						"keyword parameter redefined");
					goto fail;
				}
				INCREF(value);
				SETLOCAL(j, value);
			}
		}
		if (argcount < co->co_argcount) {
			int m = co->co_argcount - defcount;
			for (i = argcount; i < m; i++) {
				if (GETLOCAL(i) == NULL) {
					err_setstr(TypeError,
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
					object *def = defs[i];
					INCREF(def);
					SETLOCAL(m+i, def);
				}
			}
		}
		if (kwdict != NULL) {
			i = co->co_argcount;
			if (co->co_flags & CO_VARARGS)
				i++;
			SETLOCAL(i, kwdict);
		}
		if (0) {
	 fail:
			XDECREF(kwdict);
			goto fail2;
		}
	}
	else {
		if (argcount > 0 || kwcount > 0) {
			err_setstr(TypeError, "no arguments expected");
 fail2:
			current_frame = f->f_back;
			DECREF(f);
			return NULL;
		}
	}

	if (sys_trace != NULL) {
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
		if (call_trace(&sys_trace, &f->f_trace, f, "call",
			       None/*XXX how to compute arguments now?*/)) {
			/* Trace function raised an error */
			current_frame = f->f_back;
			DECREF(f);
			return NULL;
		}
	}

	if (sys_profile != NULL) {
		/* Similar for sys_profile, except it needn't return
		   itself and isn't called for "line" events */
		if (call_trace(&sys_profile, (object**)0, f, "call",
			       None/*XXX*/)) {
			current_frame = f->f_back;
			DECREF(f);
			return NULL;
		}
	}

	if (++recursion_depth > MAX_RECURSION_DEPTH) {
		--recursion_depth;
		err_setstr(RuntimeError, "Maximum recursion depth exceeded");
		current_frame = f->f_back;
		DECREF(f);
		return NULL;
	}

	next_instr = GETUSTRINGVALUE(f->f_code->co_code);
	stack_pointer = f->f_valuestack;
	
	why = WHY_NOT;
	err = 0;
	x = None;	/* Not a reference, just anything non-NULL */
	
	for (;;) {
		/* Do periodic things.
		   Doing this every time through the loop would add
		   too much overhead (a function call per instruction).
		   So we do it only every Nth instruction.

		   The ticker is reset to zero if there are pending
		   calls (see Py_AddPendingCall() and
		   Py_MakePendingCalls() above). */
		
		if (--ticker < 0) {
			ticker = sys_checkinterval;
			if (pendingfirst != pendinglast) {
				if (Py_MakePendingCalls() < 0) {
					why = WHY_EXCEPTION;
					goto on_error;
				}
			}
#ifndef HAVE_SIGNAL_H /* Is this the right #define? */
/* If we have true signals, the signal handler will call
   Py_AddPendingCall() so we don't have to call sigcheck().
   On the Mac and DOS, alas, we have to call it. */
			if (sigcheck()) {
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
			DECREF(v);
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
			INCREF(v);
			PUSH(v);
			continue;
		
		case UNARY_POSITIVE:
			v = POP();
			x = pos(v);
			DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case UNARY_NEGATIVE:
			v = POP();
			x = neg(v);
			DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case UNARY_NOT:
			v = POP();
			x = not(v);
			DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case UNARY_CONVERT:
			v = POP();
			x = reprobject(v);
			DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
			
		case UNARY_INVERT:
			v = POP();
			x = invert(v);
			DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_POWER:
			w = POP();
			v = POP();
			x = powerop(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_MULTIPLY:
			w = POP();
			v = POP();
			x = mul(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_DIVIDE:
			w = POP();
			v = POP();
			x = divide(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_MODULO:
			w = POP();
			v = POP();
			x = mod(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_ADD:
			w = POP();
			v = POP();
			x = add(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_SUBTRACT:
			w = POP();
			v = POP();
			x = sub(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_SUBSCR:
			w = POP();
			v = POP();
			x = apply_subscript(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_LSHIFT:
			w = POP();
			v = POP();
			x = lshift(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_RSHIFT:
			w = POP();
			v = POP();
			x = rshift(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_AND:
			w = POP();
			v = POP();
			x = and(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_XOR:
			w = POP();
			v = POP();
			x = xor(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case BINARY_OR:
			w = POP();
			v = POP();
			x = or(v, w);
			DECREF(v);
			DECREF(w);
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
			DECREF(u);
			XDECREF(v);
			XDECREF(w);
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
			DECREF(t);
			DECREF(u);
			XDECREF(v);
			XDECREF(w);
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
			err = assign_slice(u, v, w, (object *)NULL);
							/* del u[v:w] */
			DECREF(u);
			XDECREF(v);
			XDECREF(w);
			if (err == 0) continue;
			break;
		
		case STORE_SUBSCR:
			w = POP();
			v = POP();
			u = POP();
			/* v[w] = u */
			err = assign_subscript(v, w, u);
			DECREF(u);
			DECREF(v);
			DECREF(w);
			if (err == 0) continue;
			break;
		
		case DELETE_SUBSCR:
			w = POP();
			v = POP();
			/* del v[w] */
			err = assign_subscript(v, w, (object *)NULL);
			DECREF(v);
			DECREF(w);
			if (err == 0) continue;
			break;
		
		case PRINT_EXPR:
			v = POP();
			/* Print value except if procedure result */
			/* Before printing, also assign to '_' */
			if (v != None &&
			    (err = dictinsert(f->f_builtins, "_", v)) == 0 &&
			    !suppress_print) {
				flushline();
				x = sysget("stdout");
				err = writeobject(v, x, 0);
				softspace(x, 1);
				flushline();
			}
			DECREF(v);
			break;
		
		case PRINT_ITEM:
			v = POP();
			w = sysget("stdout");
			if (softspace(w, 1))
				writestring(" ", w);
			err = writeobject(v, w, PRINT_RAW);
			if (err == 0 && is_stringobject(v)) {
				/* XXX move into writeobject() ? */
				char *s = getstringvalue(v);
				int len = getstringsize(v);
				if (len > 0 &&
				    isspace(Py_CHARMASK(s[len-1])) &&
				    s[len-1] != ' ')
					softspace(w, 0);
			}
			DECREF(v);
			if (err == 0) continue;
			break;
		
		case PRINT_NEWLINE:
			x = sysget("stdout");
			if (x == NULL)
				err_setstr(RuntimeError, "lost sys.stdout");
			else {
				writestring("\n", x);
				softspace(x, 0);
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
				err_setstr(SystemError,
					   "bad RAISE_VARARGS oparg");
				why = WHY_EXCEPTION;
				break;
			}
			break;
		
		case LOAD_LOCALS:
			if ((x = f->f_locals) == NULL) {
				err_setstr(SystemError, "no locals");
				break;
			}
			INCREF(x);
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
			DECREF(u);
			DECREF(v);
			DECREF(w);
			break;
		
		case POP_BLOCK:
			{
				block *b = pop_block(f);
				while (STACK_LEVEL() > b->b_level) {
					v = POP();
					DECREF(v);
				}
			}
			break;
		
		case END_FINALLY:
			v = POP();
			if (is_intobject(v)) {
				why = (enum why_code) getintvalue(v);
				if (why == WHY_RETURN)
					retval = POP();
			}
			else if (is_stringobject(v) || is_classobject(v)) {
				w = POP();
				u = POP();
				err_restore(v, w, u);
				why = WHY_RERAISE;
				break;
			}
			else if (v != None) {
				err_setstr(SystemError,
					"'finally' pops bad exception");
				why = WHY_EXCEPTION;
			}
			DECREF(v);
			break;
		
		case BUILD_CLASS:
			u = POP();
			v = POP();
			w = POP();
			x = build_class(u, v, w);
			PUSH(x);
			DECREF(u);
			DECREF(v);
			DECREF(w);
			break;
		
		case STORE_NAME:
			w = GETNAMEV(oparg);
			v = POP();
			if ((x = f->f_locals) == NULL) {
				err_setstr(SystemError, "no locals");
				break;
			}
			err = dict2insert(x, w, v);
			DECREF(v);
			break;
		
		case DELETE_NAME:
			w = GETNAMEV(oparg);
			if ((x = f->f_locals) == NULL) {
				err_setstr(SystemError, "no locals");
				break;
			}
			if ((err = dict2remove(x, w)) != 0)
				err_setval(NameError, w);
			break;

#ifdef CASE_TOO_BIG
		default: switch (opcode) {
#endif
		
		case UNPACK_TUPLE:
			v = POP();
			if (!is_tupleobject(v)) {
				err_setstr(TypeError, "unpack non-tuple");
				why = WHY_EXCEPTION;
			}
			else if (gettuplesize(v) != oparg) {
				err_setstr(ValueError,
					"unpack tuple of wrong size");
				why = WHY_EXCEPTION;
			}
			else {
				for (; --oparg >= 0; ) {
					w = GETTUPLEITEM(v, oparg);
					INCREF(w);
					PUSH(w);
				}
			}
			DECREF(v);
			break;
		
		case UNPACK_LIST:
			v = POP();
			if (!is_listobject(v)) {
				err_setstr(TypeError, "unpack non-list");
				why = WHY_EXCEPTION;
			}
			else if (getlistsize(v) != oparg) {
				err_setstr(ValueError,
					"unpack list of wrong size");
				why = WHY_EXCEPTION;
			}
			else {
				for (; --oparg >= 0; ) {
					w = getlistitem(v, oparg);
					INCREF(w);
					PUSH(w);
				}
			}
			DECREF(v);
			break;
		
		case STORE_ATTR:
			w = GETNAMEV(oparg);
			v = POP();
			u = POP();
			err = setattro(v, w, u); /* v.w = u */
			DECREF(v);
			DECREF(u);
			break;
		
		case DELETE_ATTR:
			w = GETNAMEV(oparg);
			v = POP();
			err = setattro(v, w, (object *)NULL); /* del v.w */
			DECREF(v);
			break;
		
		case STORE_GLOBAL:
			w = GETNAMEV(oparg);
			v = POP();
			err = dict2insert(f->f_globals, w, v);
			DECREF(v);
			break;
		
		case DELETE_GLOBAL:
			w = GETNAMEV(oparg);
			if ((err = dict2remove(f->f_globals, w)) != 0)
				err_setval(NameError, w);
			break;
		
		case LOAD_CONST:
			x = GETCONST(oparg);
			INCREF(x);
			PUSH(x);
			break;
		
		case LOAD_NAME:
			w = GETNAMEV(oparg);
			if ((x = f->f_locals) == NULL) {
				err_setstr(SystemError, "no locals");
				break;
			}
			x = dict2lookup(x, w);
			if (x == NULL) {
				err_clear();
				x = dict2lookup(f->f_globals, w);
				if (x == NULL) {
					err_clear();
					x = dict2lookup(f->f_builtins, w);
					if (x == NULL) {
						err_setval(NameError, w);
						break;
					}
				}
			}
			INCREF(x);
			PUSH(x);
			break;
		
		case LOAD_GLOBAL:
			w = GETNAMEV(oparg);
			x = dict2lookup(f->f_globals, w);
			if (x == NULL) {
				err_clear();
				x = dict2lookup(f->f_builtins, w);
				if (x == NULL) {
					err_setval(NameError, w);
					break;
				}
			}
			INCREF(x);
			PUSH(x);
			break;

		case LOAD_FAST:
			x = GETLOCAL(oparg);
			if (x == NULL) {
				err_setval(NameError,
					   gettupleitem(co->co_varnames,
							oparg));
				break;
			}
			INCREF(x);
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
			x = newtupleobject(oparg);
			if (x != NULL) {
				for (; --oparg >= 0;) {
					w = POP();
					SETTUPLEITEM(x, oparg, w);
				}
				PUSH(x);
				continue;
			}
			break;
		
		case BUILD_LIST:
			x =  newlistobject(oparg);
			if (x != NULL) {
				for (; --oparg >= 0;) {
					w = POP();
					err = setlistitem(x, oparg, w);
					if (err != 0)
						break;
				}
				PUSH(x);
				continue;
			}
			break;
		
		case BUILD_MAP:
			x = newdictobject();
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case LOAD_ATTR:
			w = GETNAMEV(oparg);
			v = POP();
			x = getattro(v, w);
			DECREF(v);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case COMPARE_OP:
			w = POP();
			v = POP();
			x = cmp_outcome(oparg, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case IMPORT_NAME:
			w = GETNAMEV(oparg);
			x = dictlookup(f->f_builtins, "__import__");
			if (x == NULL) {
				err_setstr(ImportError,
					   "__import__ not found");
				break;
			}
			if (is_methodobject(x)) {
				u = None;
				INCREF(u);
			}
			else {
				u = find_from_args(f, INSTR_OFFSET());
				if (u == NULL) {
					x = u;
					break;
				}
			}
			w = mkvalue("(OOOO)",
				    w,
				    f->f_globals,
				    f->f_locals == NULL ? None : f->f_locals,
				    u);
			DECREF(u);
			if (w == NULL) {
				x = NULL;
				break;
			}
			x = call_object(x, w);
			DECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;
		
		case IMPORT_FROM:
			w = GETNAMEV(oparg);
			v = TOP();
			fast_2_locals(f);
			if ((x = f->f_locals) == NULL) {
				err_setstr(SystemError, "no locals");
				break;
			}
			err = import_from(x, v, w);
			locals_2_fast(f, 0);
			if (err == 0) continue;
			break;

		case JUMP_FORWARD:
			JUMPBY(oparg);
			continue;
		
		case JUMP_IF_FALSE:
			err = testbool(TOP());
			if (err > 0)
				err = 0;
			else if (err == 0)
				JUMPBY(oparg);
			else
				break;
			continue;
		
		case JUMP_IF_TRUE:
			err = testbool(TOP());
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
				x = newintobject(getintvalue(w)+1);
				PUSH(x);
				DECREF(w);
				PUSH(u);
				if (x != NULL) continue;
			}
			else {
				DECREF(v);
				DECREF(w);
				/* A NULL can mean "s exhausted"
				   but also an error: */
				if (err_occurred())
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
			setup_block(f, opcode, INSTR_OFFSET() + oparg,
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
					 f, "line", None);
			break;

		case CALL_FUNCTION:
		{
			int na = oparg & 0xff;
			int nk = (oparg>>8) & 0xff;
			int n = na + 2*nk;
			object **pfunc = stack_pointer - n - 1;
			object *func = *pfunc;
			object *self = NULL;
			object *class = NULL;
			f->f_lasti = INSTR_OFFSET() - 3; /* For tracing */
			if (is_instancemethodobject(func)) {
				self = instancemethodgetself(func);
				class = instancemethodgetclass(func);
				func = instancemethodgetfunc(func);
				INCREF(func);
				if (self != NULL) {
					INCREF(self);
					DECREF(*pfunc);
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
					    is_instanceobject(self) &&
					    issubclass(
						    (object *)
						    (((instanceobject *)self)
						     ->in_class),
						    class))
						/* Handy-dandy */ ;
					else {
						err_setstr(TypeError,
	   "unbound method must be called with class instance 1st argument");
						x = NULL;
						break;
					}
				}
			}
			else
				INCREF(func);
			if (is_funcobject(func)) {
				object *co = getfunccode(func);
				object *globals = getfuncglobals(func);
				object *argdefs = PyFunction_GetDefaults(func);
				object **d;
				int nd;
				if (argdefs != NULL) {
					d = &GETTUPLEITEM(argdefs, 0);
					nd = ((tupleobject *)argdefs)->ob_size;
				}
				else {
					d = NULL;
					nd = 0;
				}
				x = eval_code2(
					(codeobject *)co,
					globals, (object *)NULL,
					stack_pointer-n, na,
					stack_pointer-2*nk, nk,
					d, nd,
					class);
			}
			else {
				object *args = newtupleobject(na);
				object *kwdict = NULL;
				if (args == NULL) {
					x = NULL;
					break;
				}
				if (nk > 0) {
					kwdict = newdictobject();
					if (kwdict == NULL) {
						x = NULL;
						break;
					}
					err = 0;
					while (--nk >= 0) {
						object *value = POP();
						object *key = POP();
						err = mappinginsert(
							kwdict, key, value);
						if (err) {
							DECREF(key);
							DECREF(value);
							break;
						}
					}
					if (err) {
						DECREF(args);
						DECREF(kwdict);
						break;
					}
				}
				while (--na >= 0) {
					w = POP();
					SETTUPLEITEM(args, na, w);
				}
				x = PyEval_CallObjectWithKeywords(
					func, args, kwdict);
				DECREF(args);
				XDECREF(kwdict);
			}
			DECREF(func);
			while (stack_pointer > pfunc) {
				w = POP();
				DECREF(w);
			}
			PUSH(x);
			if (x != NULL) continue;
			break;
		}
		
		case MAKE_FUNCTION:
			v = POP(); /* code object */
			x = newfuncobject(v, f->f_globals);
			DECREF(v);
			/* XXX Maybe this should be a separate opcode? */
			if (x != NULL && oparg > 0) {
				v = newtupleobject(oparg);
				if (v == NULL) {
					DECREF(x);
					x = NULL;
					break;
				}
				while (--oparg >= 0) {
					w = POP();
					SETTUPLEITEM(v, oparg, w);
				}
				err = PyFunction_SetDefaults(x, v);
				DECREF(v);
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
			DECREF(u);
			DECREF(v);
			XDECREF(w);
			PUSH(x);
			if (x != NULL) continue;
			break;


		default:
			fprintf(stderr,
				"XXX lineno: %d, opcode: %d\n",
				f->f_lineno, opcode);
			err_setstr(SystemError, "unknown opcode");
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
				if (err_occurred())
					fprintf(stderr,
						"XXX undetected error\n");
				else
#endif
					continue; /* Normal, fast path */
			}
			why = WHY_EXCEPTION;
			x = None;
			err = 0;
		}

#ifdef CHECKEXC
		/* Double-check exception status */
		
		if (why == WHY_EXCEPTION || why == WHY_RERAISE) {
			if (!err_occurred()) {
				fprintf(stderr, "XXX ghost error\n");
				err_setstr(SystemError, "ghost error");
				why = WHY_EXCEPTION;
			}
		}
		else {
			if (err_occurred()) {
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
			tb_here(f);

			if (f->f_trace)
				call_exc_trace(&f->f_trace, &f->f_trace, f);
			if (sys_profile)
				call_exc_trace(&sys_profile, (object**)0, f);
		}
		
		/* For the rest, treat WHY_RERAISE as WHY_EXCEPTION */
		
		if (why == WHY_RERAISE)
			why = WHY_EXCEPTION;

		/* Unwind stacks if a (pseudo) exception occurred */
		
		while (why != WHY_NOT && f->f_iblock > 0) {
			block *b = pop_block(f);
			while (STACK_LEVEL() > b->b_level) {
				v = POP();
				XDECREF(v);
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
					object *exc, *val, *tb;
					err_fetch(&exc, &val, &tb);
					if (val == NULL) {
						val = None;
						INCREF(val);
					}
					/* Make the raw exception data
					   available to the handler,
					   so a program can emulate the
					   Python main loop.  Don't do
					   this for 'finally'. */
					if (b->b_type == SETUP_EXCEPT) {
						sysset("exc_traceback", tb);
						sysset("exc_value", val);
						sysset("exc_type", exc);
					}
					PUSH(tb);
					PUSH(val);
					PUSH(exc);
				}
				else {
					if (why == WHY_RETURN)
						PUSH(retval);
					v = newintobject((long)why);
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
		XDECREF(v);
	}
	
	if (why != WHY_RETURN)
		retval = NULL;
	
	if (f->f_trace) {
		if (why == WHY_RETURN) {
			if (call_trace(&f->f_trace, &f->f_trace, f,
				       "return", retval)) {
				XDECREF(retval);
				retval = NULL;
				why = WHY_EXCEPTION;
			}
		}
	}
	
	if (sys_profile && why == WHY_RETURN) {
		if (call_trace(&sys_profile, (object**)0,
			       f, "return", retval)) {
			XDECREF(retval);
			retval = NULL;
			why = WHY_EXCEPTION;
		}
	}
	
	/* Restore previous frame and release the current one */

	current_frame = f->f_back;
	DECREF(f);
	--recursion_depth;
	
	return retval;
}

/* Logic for the raise statement (too complicated for inlining).
   This *consumes* a reference count to each of its arguments. */
static enum why_code
do_raise(type, value, tb)
	object *type, *value, *tb;
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
	if (tb == None) {
		DECREF(tb);
		tb = NULL;
	}
	else if (tb != NULL && !PyTraceBack_Check(tb)) {
		err_setstr(TypeError,
			   "raise 3rd arg must be traceback or None");
		goto raise_error;
	}

	/* Next, replace a missing value with None */
	if (value == NULL) {
		value = None;
		INCREF(value);
	}

	/* Next, repeatedly, replace a tuple exception with its first item */
	while (is_tupleobject(type) && gettuplesize(type) > 0) {
		object *tmp = type;
		type = GETTUPLEITEM(type, 0);
		INCREF(type);
		DECREF(tmp);
	}

	/* Now switch on the exception's type */
	if (is_stringobject(type)) {
		;
	}
	else if (is_classobject(type)) {
		/* Raising a class.  If the value is an instance, it
		   better be an instance of the class.  If it is not,
		   it will be used to create an instance. */
		if (is_instanceobject(value)) {
			object *inclass = (object*)
				(((instanceobject*)value)->in_class);
			if (!issubclass(inclass, type)) {
				err_setstr(TypeError,
 "raise <class>, <instance> requires that <instance> is a member of <class>");
				goto raise_error;
			}
		}
		else {
			/* Go instantiate the class */
			object *args, *res;
			if (value == None)
				args = mkvalue("()");
			else if (is_tupleobject(value)) {
				INCREF(value);
				args = value;
			}
			else
				args = mkvalue("(O)", value);
			if (args == NULL)
				goto raise_error;
			res = call_object(type, args);
			DECREF(args);
			if (res == NULL)
				goto raise_error;
			DECREF(value);
			value = res;
		}
	}
	else if (is_instanceobject(type)) {
		/* Raising an instance.  The value should be a dummy. */
		if (value != None) {
			err_setstr(TypeError,
			  "instance exception may not have a separate value");
			goto raise_error;
		}
		else {
			/* Normalize to raise <class>, <instance> */
			DECREF(value);
			value = type;
			type = (object*) ((instanceobject*)type)->in_class;
			INCREF(type);
		}
	}
	else {
		/* Not something you can raise.  You get an exception
		   anyway, just not what you specified :-) */
		err_setstr(TypeError,
		    "exceptions must be strings, classes, or instances");
		goto raise_error;
	}
	err_restore(type, value, tb);
	if (tb == NULL)
		return WHY_EXCEPTION;
	else
		return WHY_RERAISE;
 raise_error:
	XDECREF(value);
	XDECREF(type);
	XDECREF(tb);
	return WHY_EXCEPTION;
}

#ifdef LLTRACE
static int
prtrace(v, str)
	object *v;
	char *str;
{
	printf("%s ", str);
	if (printobject(v, stdout, 0) != 0)
		err_clear(); /* Don't know what else to do */
	printf("\n");
}
#endif

static void
call_exc_trace(p_trace, p_newtrace, f)
	object **p_trace, **p_newtrace;
	frameobject *f;
{
	object *type, *value, *traceback, *arg;
	int err;
	err_fetch(&type, &value, &traceback);
	if (value == NULL) {
		value = None;
		INCREF(value);
	}
	arg = mkvalue("(OOO)", type, value, traceback);
	if (arg == NULL) {
		err_restore(type, value, traceback);
		return;
	}
	err = call_trace(p_trace, p_newtrace, f, "exception", arg);
	DECREF(arg);
	if (err == 0)
		err_restore(type, value, traceback);
	else {
		XDECREF(type);
		XDECREF(value);
		XDECREF(traceback);
	}
}

static int
call_trace(p_trace, p_newtrace, f, msg, arg)
	object **p_trace; /* in/out; may not be NULL;
			     may not point to NULL variable initially */
	object **p_newtrace; /* in/out; may be NULL;
				may point to NULL variable;
				may be same variable as p_newtrace */
	frameobject *f;
	char *msg;
	object *arg;
{
	object *args, *what;
	object *res = NULL;
	static int tracing = 0;
	
	if (tracing) {
		/* Don't do recursive traces */
		if (p_newtrace) {
			XDECREF(*p_newtrace);
			*p_newtrace = NULL;
		}
		return 0;
	}
	
	args = newtupleobject(3);
	if (args == NULL)
		goto cleanup;
	what = newstringobject(msg);
	if (what == NULL)
		goto cleanup;
	INCREF(f);
	SETTUPLEITEM(args, 0, (object *)f);
	SETTUPLEITEM(args, 1, what);
	if (arg == NULL)
		arg = None;
	INCREF(arg);
	SETTUPLEITEM(args, 2, arg);
	tracing++;
	fast_2_locals(f);
	res = call_object(*p_trace, args); /* May clear *p_trace! */
	locals_2_fast(f, 1);
	tracing--;
 cleanup:
	XDECREF(args);
	if (res == NULL) {
		/* The trace proc raised an exception */
		tb_here(f);
		XDECREF(*p_trace);
		*p_trace = NULL;
		if (p_newtrace) {
			XDECREF(*p_newtrace);
			*p_newtrace = NULL;
		}
		return -1;
	}
	else {
		if (p_newtrace) {
			XDECREF(*p_newtrace);
			if (res == None)
				*p_newtrace = NULL;
			else {
				INCREF(res);
				*p_newtrace = res;
			}
		}
		DECREF(res);
		return 0;
	}
}

object *
getbuiltins()
{
	if (current_frame == NULL)
		return getbuiltinmod();
	else
		return current_frame->f_builtins;
}

object *
getlocals()
{
	if (current_frame == NULL)
		return NULL;
	fast_2_locals(current_frame);
	return current_frame->f_locals;
}

object *
getglobals()
{
	if (current_frame == NULL)
		return NULL;
	else
		return current_frame->f_globals;
}

object *
getframe()
{
	return (object *)current_frame;
}

int
getrestricted()
{
	return current_frame == NULL ? 0 : current_frame->f_restricted;
}

void
flushline()
{
	object *f = sysget("stdout");
	if (softspace(f, 0))
		writestring("\n", f);
}


#define BINOP(opname, ropname, thisfunc) \
	if (!is_instanceobject(v) && !is_instanceobject(w)) \
		; \
	else \
		return instancebinop(v, w, opname, ropname, thisfunc)


static object *
or(v, w)
	object *v, *w;
{
	BINOP("__or__", "__ror__", or);
	if (v->ob_type->tp_as_number != NULL) {
		object *x = NULL;
		object * (*f) FPROTO((object *, object *));
		if (coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_or) != NULL)
			x = (*f)(v, w);
		DECREF(v);
		DECREF(w);
		if (f != NULL)
			return x;
	}
	err_setstr(TypeError, "bad operand type(s) for |");
	return NULL;
}

static object *
xor(v, w)
	object *v, *w;
{
	BINOP("__xor__", "__rxor__", xor);
	if (v->ob_type->tp_as_number != NULL) {
		object *x = NULL;
		object * (*f) FPROTO((object *, object *));
		if (coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_xor) != NULL)
			x = (*f)(v, w);
		DECREF(v);
		DECREF(w);
		if (f != NULL)
			return x;
	}
	err_setstr(TypeError, "bad operand type(s) for ^");
	return NULL;
}

static object *
and(v, w)
	object *v, *w;
{
	BINOP("__and__", "__rand__", and);
	if (v->ob_type->tp_as_number != NULL) {
		object *x = NULL;
		object * (*f) FPROTO((object *, object *));
		if (coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_and) != NULL)
			x = (*f)(v, w);
		DECREF(v);
		DECREF(w);
		if (f != NULL)
			return x;
	}
	err_setstr(TypeError, "bad operand type(s) for &");
	return NULL;
}

static object *
lshift(v, w)
	object *v, *w;
{
	BINOP("__lshift__", "__rlshift__", lshift);
	if (v->ob_type->tp_as_number != NULL) {
		object *x = NULL;
		object * (*f) FPROTO((object *, object *));
		if (coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_lshift) != NULL)
			x = (*f)(v, w);
		DECREF(v);
		DECREF(w);
		if (f != NULL)
			return x;
	}
	err_setstr(TypeError, "bad operand type(s) for <<");
	return NULL;
}

static object *
rshift(v, w)
	object *v, *w;
{
	BINOP("__rshift__", "__rrshift__", rshift);
	if (v->ob_type->tp_as_number != NULL) {
		object *x = NULL;
		object * (*f) FPROTO((object *, object *));
		if (coerce(&v, &w) != 0)
			return NULL;
		if ((f = v->ob_type->tp_as_number->nb_rshift) != NULL)
			x = (*f)(v, w);
		DECREF(v);
		DECREF(w);
		if (f != NULL)
			return x;
	}
	err_setstr(TypeError, "bad operand type(s) for >>");
	return NULL;
}

static object *
add(v, w)
	object *v, *w;
{
	BINOP("__add__", "__radd__", add);
	if (v->ob_type->tp_as_sequence != NULL)
		return (*v->ob_type->tp_as_sequence->sq_concat)(v, w);
	else if (v->ob_type->tp_as_number != NULL) {
		object *x;
		if (coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_add)(v, w);
		DECREF(v);
		DECREF(w);
		return x;
	}
	err_setstr(TypeError, "bad operand type(s) for +");
	return NULL;
}

static object *
sub(v, w)
	object *v, *w;
{
	BINOP("__sub__", "__rsub__", sub);
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
		if (coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_subtract)(v, w);
		DECREF(v);
		DECREF(w);
		return x;
	}
	err_setstr(TypeError, "bad operand type(s) for -");
	return NULL;
}

static object *
mul(v, w)
	object *v, *w;
{
	typeobject *tp;
	tp = v->ob_type;
	BINOP("__mul__", "__rmul__", mul);
	if (tp->tp_as_number != NULL &&
	    w->ob_type->tp_as_sequence != NULL &&
	    !is_instanceobject(v)) {
		/* number*sequence -- swap v and w */
		object *tmp = v;
		v = w;
		w = tmp;
		tp = v->ob_type;
	}
	if (tp->tp_as_number != NULL) {
		object *x;
		if (is_instanceobject(v)) {
			/* Instances of user-defined classes get their
			   other argument uncoerced, so they may
			   implement sequence*number as well as
			   number*number. */
			INCREF(v);
			INCREF(w);
		}
		else if (coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_multiply)(v, w);
		DECREF(v);
		DECREF(w);
		return x;
	}
	if (tp->tp_as_sequence != NULL) {
		if (!is_intobject(w)) {
			err_setstr(TypeError,
				"can't multiply sequence with non-int");
			return NULL;
		}
		return (*tp->tp_as_sequence->sq_repeat)
						(v, (int)getintvalue(w));
	}
	err_setstr(TypeError, "bad operand type(s) for *");
	return NULL;
}

static object *
divide(v, w)
	object *v, *w;
{
	BINOP("__div__", "__rdiv__", divide);
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
		if (coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_divide)(v, w);
		DECREF(v);
		DECREF(w);
		return x;
	}
	err_setstr(TypeError, "bad operand type(s) for /");
	return NULL;
}

static object *
mod(v, w)
	object *v, *w;
{
	if (is_stringobject(v)) {
		return formatstring(v, w);
	}
	BINOP("__mod__", "__rmod__", mod);
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
		if (coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_remainder)(v, w);
		DECREF(v);
		DECREF(w);
		return x;
	}
	err_setstr(TypeError, "bad operand type(s) for %");
	return NULL;
}

static object *
powerop(v, w)
	object *v, *w;
{
	object *res;
	BINOP("__pow__", "__rpow__", powerop);
	if (v->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		err_setstr(TypeError, "pow() requires numeric arguments");
		return NULL;
	}
	if (coerce(&v, &w) != 0)
		return NULL;
	res = (*v->ob_type->tp_as_number->nb_power)(v, w, None);
	DECREF(v);
	DECREF(w);
	return res;
}

static object *
neg(v)
	object *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_negative)(v);
	err_setstr(TypeError, "bad operand type(s) for unary -");
	return NULL;
}

static object *
pos(v)
	object *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_positive)(v);
	err_setstr(TypeError, "bad operand type(s) for unary +");
	return NULL;
}

static object *
invert(v)
	object *v;
{
	object * (*f) FPROTO((object *));
	if (v->ob_type->tp_as_number != NULL &&
		(f = v->ob_type->tp_as_number->nb_invert) != NULL)
		return (*f)(v);
	err_setstr(TypeError, "bad operand type(s) for unary ~");
	return NULL;
}

static object *
not(v)
	object *v;
{
	int outcome = testbool(v);
	object *w;
	if (outcome < 0)
		return NULL;
	if (outcome == 0)
		w = True;
	else
		w = False;
	INCREF(w);
	return w;
}


/* External interface to call any callable object.
   The arg must be a tuple or NULL. */

object *
call_object(func, arg)
	object *func;
	object *arg;
{
	return PyEval_CallObjectWithKeywords(func, arg, (object *)NULL);
}

object *
PyEval_CallObjectWithKeywords(func, arg, kw)
	object *func;
	object *arg;
	object *kw;
{
        ternaryfunc call;
        object *result;

	if (arg == NULL)
		arg = newtupleobject(0);
	else if (!is_tupleobject(arg)) {
		err_setstr(TypeError, "argument list must be a tuple");
		return NULL;
	}
	else
		INCREF(arg);

	if (kw != NULL && !is_dictobject(kw)) {
		err_setstr(TypeError, "keyword list must be a dictionary");
		return NULL;
	}

        if ((call = func->ob_type->tp_call) != NULL)
                result = (*call)(func, arg, kw);
        else if (is_instancemethodobject(func) || is_funcobject(func))
		result = call_function(func, arg, kw);
	else
		result = call_builtin(func, arg, kw);

	DECREF(arg);
	
        if (result == NULL && !err_occurred())
		err_setstr(SystemError,
			   "NULL result without error in call_object");
        
        return result;
}

static object *
call_builtin(func, arg, kw)
	object *func;
	object *arg;
	object *kw;
{
	if (is_methodobject(func)) {
		method meth = getmethod(func);
		object *self = getself(func);
		int flags = getflags(func);
		if (!(flags & METH_VARARGS)) {
			int size = gettuplesize(arg);
			if (size == 1)
				arg = GETTUPLEITEM(arg, 0);
			else if (size == 0)
				arg = NULL;
		}
		if (flags & METH_KEYWORDS)
			return (*(PyCFunctionWithKeywords)meth)(self, arg, kw);
		if (kw != NULL && getmappingsize(kw) != 0) {
			err_setstr(TypeError,
				   "this function takes no keyword arguments");
			return NULL;
		}
		return (*meth)(self, arg);
	}
	if (is_classobject(func)) {
		return newinstanceobject(func, arg, kw);
	}
	if (is_instanceobject(func)) {
	        object *res, *call = getattr(func,"__call__");
		if (call == NULL) {
			err_clear();
			err_setstr(AttributeError,
				   "no __call__ method defined");
			return NULL;
		}
		res = PyEval_CallObjectWithKeywords(call, arg, kw);
		DECREF(call);
		return res;
	}
	err_setstr(TypeError, "call of non-function");
	return NULL;
}

static object *
call_function(func, arg, kw)
	object *func;
	object *arg;
	object *kw;
{
	object *class = NULL; /* == owner */
	object *argdefs;
	object **d, **k;
	int nk, nd;
	object *result;
	
	if (kw != NULL && !is_dictobject(kw)) {
		err_badcall();
		return NULL;
	}
	
	if (is_instancemethodobject(func)) {
		object *self = instancemethodgetself(func);
		class = instancemethodgetclass(func);
		func = instancemethodgetfunc(func);
		if (self == NULL) {
			/* Unbound methods must be called with an instance of
			   the class (or a derived class) as first argument */
			if (gettuplesize(arg) >= 1) {
				self = GETTUPLEITEM(arg, 0);
				if (self != NULL &&
				    is_instanceobject(self) &&
				    issubclass((object *)
				      (((instanceobject *)self)->in_class),
					       class))
					/* Handy-dandy */ ;
				else
					self = NULL;
			}
			if (self == NULL) {
				err_setstr(TypeError,
	   "unbound method must be called with class instance 1st argument");
				return NULL;
			}
			INCREF(arg);
		}
		else {
			int argcount = gettuplesize(arg);
			object *newarg = newtupleobject(argcount + 1);
			int i;
			if (newarg == NULL)
				return NULL;
			INCREF(self);
			SETTUPLEITEM(newarg, 0, self);
			for (i = 0; i < argcount; i++) {
				object *v = GETTUPLEITEM(arg, i);
				XINCREF(v);
				SETTUPLEITEM(newarg, i+1, v);
			}
			arg = newarg;
		}
	}
	else {
		if (!is_funcobject(func)) {
			err_setstr(TypeError, "call of non-function");
			return NULL;
		}
		INCREF(arg);
	}
	
	argdefs = PyFunction_GetDefaults(func);
	if (argdefs != NULL && is_tupleobject(argdefs)) {
		d = &GETTUPLEITEM((tupleobject *)argdefs, 0);
		nd = gettuplesize(argdefs);
	}
	else {
		d = NULL;
		nd = 0;
	}
	
	if (kw != NULL) {
		int pos, i;
		nk = getmappingsize(kw);
		k = NEW(object *, 2*nk);
		if (k == NULL) {
			err_nomem();
			DECREF(arg);
			return NULL;
		}
		pos = i = 0;
		while (mappinggetnext(kw, &pos, &k[i], &k[i+1]))
			i += 2;
		nk = i/2;
		/* XXX This is broken if the caller deletes dict items! */
	}
	else {
		k = NULL;
		nk = 0;
	}
	
	result = eval_code2(
		(codeobject *)getfunccode(func),
		getfuncglobals(func), (object *)NULL,
		&GETTUPLEITEM(arg, 0), gettuplesize(arg),
		k, nk,
		d, nd,
		class);
	
	DECREF(arg);
	XDEL(k);
	
	return result;
}

#define SLICE_ERROR_MSG \
	"standard sequence type does not support step size other than one"

static object *
apply_subscript(v, w)
	object *v, *w;
{
	typeobject *tp = v->ob_type;
	if (tp->tp_as_sequence == NULL && tp->tp_as_mapping == NULL) {
		err_setstr(TypeError, "unsubscriptable object");
		return NULL;
	}
	if (tp->tp_as_mapping != NULL) {
		return (*tp->tp_as_mapping->mp_subscript)(v, w);
	}
	else {
		int i;
		if (!is_intobject(w)) {		  
			if (PySlice_Check(w)) {
			        err_setstr(ValueError, SLICE_ERROR_MSG); 
			} else {
				err_setstr(TypeError,
					   "sequence subscript not int");
			}	
			return NULL;
		}
		i = getintvalue(w);
		if (i < 0) {
			int len = (*tp->tp_as_sequence->sq_length)(v);
			if (len < 0)
				return NULL;
			i += len;
		}
		return (*tp->tp_as_sequence->sq_item)(v, i);
	}
}

static object *
loop_subscript(v, w)
	object *v, *w;
{
	sequence_methods *sq = v->ob_type->tp_as_sequence;
	int i;
	if (sq == NULL) {
		err_setstr(TypeError, "loop over non-sequence");
		return NULL;
	}
	i = getintvalue(w);
	v = (*sq->sq_item)(v, i);
	if (v)
		return v;
	if (err_occurred() == IndexError)
		err_clear();
	return NULL;
}

static int
slice_index(v, isize, pi)
	object *v;
	int isize;
	int *pi;
{
	if (v != NULL) {
		if (!is_intobject(v)) {
			err_setstr(TypeError, "slice index must be int");
			return -1;
		}
		*pi = getintvalue(v);
		if (*pi < 0)
			*pi += isize;
	}
	return 0;
}

static object *
apply_slice(u, v, w) /* return u[v:w] */
	object *u, *v, *w;
{
	typeobject *tp = u->ob_type;
	int ilow, ihigh, isize;
	if (tp->tp_as_sequence == NULL) {
		err_setstr(TypeError, "only sequences can be sliced");
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
	object *w;
	object *key;
	object *v;
{
	typeobject *tp = w->ob_type;
	sequence_methods *sq;
	mapping_methods *mp;
	int (*func1)();
	int (*func2)();
	if ((mp = tp->tp_as_mapping) != NULL &&
			(func1 = mp->mp_ass_subscript) != NULL) {
		return (*func1)(w, key, v);
	}
	else if ((sq = tp->tp_as_sequence) != NULL &&
			(func2 = sq->sq_ass_item) != NULL) {
		if (!is_intobject(key)) {
			err_setstr(TypeError,
			"sequence subscript must be integer (assign or del)");
			return -1;
		}
		else {
			int i = getintvalue(key);
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
		err_setstr(TypeError,
				"can't assign to this subscripted object");
		return -1;
	}
}

static int
assign_slice(u, v, w, x) /* u[v:w] = x */
	object *u, *v, *w, *x;
{
	sequence_methods *sq = u->ob_type->tp_as_sequence;
	int ilow, ihigh, isize;
	if (sq == NULL) {
		err_setstr(TypeError, "assign to slice of non-sequence");
		return -1;
	}
	if (sq == NULL || sq->sq_ass_slice == NULL) {
		err_setstr(TypeError, "unassignable slice");
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
	object *err, *v;
{
	if (is_tupleobject(v)) {
		int i, n;
		n = gettuplesize(v);
		for (i = 0; i < n; i++) {
			/* Test recursively */
			if (cmp_exception(err, GETTUPLEITEM(v, i)))
				return 1;
		}
		return 0;
	}
	if (is_classobject(v) && is_classobject(err))
		return issubclass(err, v);
	return err == v;
}

static int
cmp_member(v, w)
	object *v, *w;
{
	int i, cmp;
	object *x;
	sequence_methods *sq;
	/* Special case for char in string */
	if (is_stringobject(w)) {
		register char *s, *end;
		register char c;
		if (!is_stringobject(v) || getstringsize(v) != 1) {
			err_setstr(TypeError,
			    "string member test needs char left operand");
			return -1;
		}
		c = getstringvalue(v)[0];
		s = getstringvalue(w);
		end = s + getstringsize(w);
		while (s < end) {
			if (c == *s++)
				return 1;
		}
		return 0;
	}
	sq = w->ob_type->tp_as_sequence;
	if (sq == NULL) {
		err_setstr(TypeError,
			"'in' or 'not in' needs sequence right argument");
		return -1;
	}
	for (i = 0; ; i++) {
		x = (*sq->sq_item)(w, i);
		if (x == NULL) {
			if (err_occurred() == IndexError) {
				err_clear();
				break;
			}
			return -1;
		}
		cmp = cmpobject(v, x);
		XDECREF(x);
		if (cmp == 0)
			return 1;
	}
	return 0;
}

static object *
cmp_outcome(op, v, w)
	int op;
	register object *v;
	register object *w;
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
		cmp = cmpobject(v, w);
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
	v = res ? True : False;
	INCREF(v);
	return v;
}

static int
import_from(locals, v, name)
	object *locals;
	object *v;
	object *name;
{
	object *w, *x;
	if (!is_moduleobject(v)) {
		err_setstr(TypeError, "import-from requires module object");
		return -1;
	}
	w = getmoduledict(v);
	if (getstringvalue(name)[0] == '*') {
		int pos, err;
		object *name, *value;
		pos = 0;
		while (mappinggetnext(w, &pos, &name, &value)) {
			if (!is_stringobject(name) ||
			    getstringvalue(name)[0] == '_')
				continue;
			INCREF(value);
			err = dict2insert(locals, name, value);
			DECREF(value);
			if (err != 0)
				return -1;
		}
		return 0;
	}
	else {
		x = dict2lookup(w, name);
		if (x == NULL) {
			char buf[250];
			sprintf(buf, "cannot import name %.230s",
				getstringvalue(name));
			err_setstr(ImportError, buf);
			return -1;
		}
		else
			return dict2insert(locals, name, x);
	}
}

static object *
build_class(methods, bases, name)
	object *methods; /* dictionary */
	object *bases;  /* tuple containing classes */
	object *name;   /* string */
{
	int i;
	if (!is_tupleobject(bases)) {
		err_setstr(SystemError, "build_class with non-tuple bases");
		return NULL;
	}
	if (!is_dictobject(methods)) {
		err_setstr(SystemError, "build_class with non-dictionary");
		return NULL;
	}
	if (!is_stringobject(name)) {
		err_setstr(SystemError, "build_class witn non-string name");
		return NULL;
	}
	for (i = gettuplesize(bases); --i >= 0; ) {
		object *base = GETTUPLEITEM(bases, i);
		if (!is_classobject(base)) {
			/* Call the base's *type*, if it is callable.
			   This code is a hook for Donald Beaudry's
			   and Jim Fulton's type extensions.  In
			   unexended Python it will never be triggered
			   since its types are not callable. */
			if (base->ob_type->ob_type->tp_call) {
			  	object *args;
				object *class;
				args = mkvalue("(OOO)", name, bases, methods);
				class = call_object((object *)base->ob_type,
						    args);
				DECREF(args);
				return class;
			}
			err_setstr(TypeError,
				"base is not a class object");
			return NULL;
		}
	}
	return newclassobject(bases, methods, name);
}

static int
exec_statement(prog, globals, locals)
	object *prog;
	object *globals;
	object *locals;
{
	char *s;
	int n;
	object *v;
	int plain = 0;

	if (is_tupleobject(prog) && globals == None && locals == None &&
	    ((n = gettuplesize(prog)) == 2 || n == 3)) {
		/* Backward compatibility hack */
		globals = gettupleitem(prog, 1);
		if (n == 3)
			locals = gettupleitem(prog, 2);
		prog = gettupleitem(prog, 0);
	}
	if (globals == None) {
		globals = getglobals();
		if (locals == None) {
			locals = getlocals();
			plain = 1;
		}
	}
	else if (locals == None)
		locals = globals;
	if (!is_stringobject(prog) &&
	    !is_codeobject(prog) &&
	    !is_fileobject(prog)) {
		err_setstr(TypeError,
			   "exec 1st arg must be string, code or file object");
		return -1;
	}
	if (!is_dictobject(globals) || !is_dictobject(locals)) {
		err_setstr(TypeError,
		    "exec 2nd/3rd args must be dict or None");
		return -1;
	}
	if (dictlookup(globals, "__builtins__") == NULL)
		dictinsert(globals, "__builtins__", current_frame->f_builtins);
	if (is_codeobject(prog)) {
		if (eval_code((codeobject *) prog, globals, locals) == NULL)
			return -1;
		return 0;
	}
	if (is_fileobject(prog)) {
		FILE *fp = getfilefile(prog);
		char *name = getstringvalue(getfilename(prog));
		if (run_file(fp, name, file_input, globals, locals) == NULL)
			return -1;
		return 0;
	}
	s = getstringvalue(prog);
	if (strlen(s) != getstringsize(prog)) {
		err_setstr(ValueError, "embedded '\\0' in exec string");
		return -1;
	}
	v = run_string(s, file_input, globals, locals);
	if (v == NULL)
		return -1;
	DECREF(v);
	if (plain)
		locals_2_fast(current_frame, 0);
	return 0;
}

/* Hack for ni.py */
static object *
find_from_args(f, nexti)
	frameobject *f;
	int nexti;
{
	int opcode;
	int oparg;
	object *list, *name;
	unsigned char *next_instr;
	
	next_instr = GETUSTRINGVALUE(f->f_code->co_code) + nexti;
	opcode = (*next_instr++);
	if (opcode != IMPORT_FROM) {
		INCREF(None);
		return None;
	}
	
	list = newlistobject(0);
	if (list == NULL)
		return NULL;
	
	do {
		oparg = (next_instr[1]<<8) + next_instr[0];
		next_instr += 2;
		name = Getnamev(f, oparg);
		if (addlistitem(list, name) < 0) {
			DECREF(list);
			break;
		}
		opcode = (*next_instr++);
	} while (opcode == IMPORT_FROM);
	
	return list;
}
