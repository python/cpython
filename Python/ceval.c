/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Execute compiled code */

#include "allobjects.h"

#include "import.h"
#include "sysmodule.h"
#include "compile.h"
#include "frameobject.h"
#include "ceval.h"
#include "opcode.h"
#include "bltinmodule.h"
#include "traceback.h"

#ifndef NDEBUG
/* For debugging the interpreter: */
#define LLTRACE  1	/* Low-level trace feature */
#define CHECKEXC 1	/* Double-check exception checking */
#endif

/* Forward declarations */
/* XXX Need prototypes */

void flushline();
static object *add();
static object *sub();
static object *mul();
static object *divide();
static object *rem();
static object *neg();
static object *pos();
static object *not();
static object *invert();
static object *lshift();
static object *rshift();
static object *and();
static object *xor();
static object *or();
static object *call_builtin();
static object *call_function();
object *call_object();
static object *apply_subscript();
static object *loop_subscript();
static object *apply_slice();
static object *cmp_outcome();
static object *build_class();
static int testbool();
static int assign_subscript PROTO((object *, object *, object *));
static int assign_slice PROTO((object *, object *, object *, object *));
static int import_from PROTO((object *, object *, object *));
static object *call_trace PROTO((object *, frameobject *, char *, object *));


static frameobject *current_frame;


/* Status code for main loop (reason for stack unwind) */

enum why_code {
		WHY_NOT,	/* No error */
		WHY_EXCEPTION,	/* Exception occurred */
		WHY_RERAISE,	/* Exception re-raised by 'finally' */
		WHY_RETURN,	/* 'return' statement */
		WHY_BREAK	/* 'break' statement */
};


/* Interpreter main loop */

object *
eval_code(co, globals, locals, arg)
	codeobject *co;
	object *globals;
	object *locals;
	object *arg;
{
	register unsigned char *next_instr;
	register int opcode;	/* Current opcode */
	register int oparg;	/* Current opcode argument, if any */
	register object **stack_pointer;
	register enum why_code why; /* Reason for block stack unwind */
	register int err;	/* Error status -- nonzero if error */
	register object *x;	/* Result object -- NULL if error */
	register object *v;	/* Temporary objects popped off stack */
	register object *w;
	register object *u;
	register object *t;
	register frameobject *f; /* Current frame */
	object *trace;		/* Trace function or NULL */
	object *retval;		/* Return value iff why == WHY_RETURN */
	char *name;		/* Name used by some instructions */
	FILE *fp;		/* Used by print operations */
#ifdef LLTRACE
	int lltrace = dictlookup(globals, "__lltrace__") != NULL;
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

	f = newframeobject(
			current_frame,		/*back*/
			co,			/*code*/
			globals,		/*globals*/
			locals,			/*locals*/
			50,			/*nvalues*/
			20);			/*nblocks*/
	if (f == NULL)
		return NULL;
	
	current_frame = f;

	trace = sysget("trace");
	if (trace != NULL) {
		/* sys.trace, if defined, is a function that will
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
		trace = call_trace(trace, f, "call", arg);
		if (trace == NULL) {
			/* Trace function raised an error */
			sysset("trace", (object *)NULL);
			current_frame = f->f_back;
			DECREF(f);
			return NULL;
		}
		if (trace == None) {
			/* No need to trace this code block */
			DECREF(trace);
			trace = NULL;
		}
	}
	
	next_instr = GETUSTRINGVALUE(f->f_code->co_code);
	stack_pointer = f->f_valuestack;
	
	if (arg != NULL) {
		INCREF(arg);
		PUSH(arg);
	}
	
	why = WHY_NOT;
	err = 0;
	x = None;	/* Not a reference, just anything non-NULL */
	
	for (;;) {
		static int ticker;
		
		/* Do periodic things */
		
		if (--ticker < 0) {
			ticker = 100;
			if (intrcheck()) {
				err_set(KeyboardInterrupt);
				why = WHY_EXCEPTION;
				goto on_error;
			}
		}
		
		/* Extract opcode and argument */
		
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
			break;
		
		case ROT_TWO:
			v = POP();
			w = POP();
			PUSH(v);
			PUSH(w);
			break;
		
		case ROT_THREE:
			v = POP();
			w = POP();
			x = POP();
			PUSH(v);
			PUSH(x);
			PUSH(w);
			break;
		
		case DUP_TOP:
			v = TOP();
			INCREF(v);
			PUSH(v);
			break;
		
		case UNARY_POSITIVE:
			v = POP();
			x = pos(v);
			DECREF(v);
			PUSH(x);
			break;
		
		case UNARY_NEGATIVE:
			v = POP();
			x = neg(v);
			DECREF(v);
			PUSH(x);
			break;
		
		case UNARY_NOT:
			v = POP();
			x = not(v);
			DECREF(v);
			PUSH(x);
			break;
		
		case UNARY_CONVERT:
			v = POP();
			x = reprobject(v);
			DECREF(v);
			PUSH(x);
			break;
		
		case UNARY_CALL:
			v = POP();
			f->f_lasti = INSTR_OFFSET() - 1; /* For tracing */
			x = call_object(v, (object *)NULL);
			DECREF(v);
			PUSH(x);
			break;
			
		case UNARY_INVERT:
			v = POP();
			x = invert(v);
			DECREF(v);
			PUSH(x);
			break;
		
		case BINARY_MULTIPLY:
			w = POP();
			v = POP();
			x = mul(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_DIVIDE:
			w = POP();
			v = POP();
			x = divide(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_MODULO:
			w = POP();
			v = POP();
			x = rem(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_ADD:
			w = POP();
			v = POP();
			x = add(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_SUBTRACT:
			w = POP();
			v = POP();
			x = sub(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_SUBSCR:
			w = POP();
			v = POP();
			x = apply_subscript(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_CALL:
			w = POP();
			v = POP();
			f->f_lasti = INSTR_OFFSET() - 1; /* For tracing */
			x = call_object(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_LSHIFT:
			w = POP();
			v = POP();
			x = lshift(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_RSHIFT:
			w = POP();
			v = POP();
			x = rshift(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_AND:
			w = POP();
			v = POP();
			x = and(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_XOR:
			w = POP();
			v = POP();
			x = xor(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case BINARY_OR:
			w = POP();
			v = POP();
			x = or(v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
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
			break;
		
		case DELETE_SUBSCR:
			w = POP();
			v = POP();
			/* del v[w] */
			err = assign_subscript(v, w, (object *)NULL);
			DECREF(v);
			DECREF(w);
			break;
		
		case PRINT_EXPR:
			v = POP();
			fp = sysgetfile("stdout", stdout);
			/* Print value except if procedure result */
			if (v != None) {
				flushline();
				softspace(sysget("stdout"), 1);
				err = printobject(v, fp, 0);
				flushline();
			}
			DECREF(v);
			break;
		
		case PRINT_ITEM:
			v = POP();
			fp = sysgetfile("stdout", stdout);
			if (softspace(sysget("stdout"), 1))
				fprintf(fp, " ");
			if (is_stringobject(v)) {
				char *s = getstringvalue(v);
				int len = getstringsize(v);
				fwrite(s, 1, len, fp);
				if (len > 0 && s[len-1] == '\n')
					softspace(sysget("stdout"), 0);
			}
			else {
				err = printobject(v, fp, 0);
			}
			DECREF(v);
			break;
		
		case PRINT_NEWLINE:
			fp = sysgetfile("stdout", stdout);
			fprintf(fp, "\n");
			softspace(sysget("stdout"), 0);
			break;
		
		case BREAK_LOOP:
			why = WHY_BREAK;
			break;
		
		case RAISE_EXCEPTION:
			v = POP();
			w = POP();
			/* A tuple is equivalent to its first element here */
			while (is_tupleobject(w)) {
				u = w;
				w = gettupleitem(u, 0);
				DECREF(u);
			}
			if (!is_stringobject(w))
				err_setstr(TypeError,
					"exceptions must be strings");
			else
				err_setval(w, v);
			DECREF(v);
			DECREF(w);
			why = WHY_EXCEPTION;
			break;
		
		case LOAD_LOCALS:
			v = f->f_locals;
			INCREF(v);
			PUSH(v);
			break;
		
		case RETURN_VALUE:
			retval = POP();
			why = WHY_RETURN;
			break;
		
		case BUILD_FUNCTION:
			v = POP();
			x = newfuncobject(v, f->f_globals);
			DECREF(v);
			PUSH(x);
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
			else if (is_stringobject(v)) {
				w = POP();
				err_setval(v, w);
				DECREF(w);
				w = POP();
				tb_store(w);
				DECREF(w);
				why = WHY_RERAISE;
			}
			else if (v != None) {
				err_setstr(SystemError,
					"'finally' pops bad exception");
				why = WHY_EXCEPTION;
			}
			DECREF(v);
			break;
		
		case BUILD_CLASS:
			w = POP();
			v = POP();
			x = build_class(v, w);
			PUSH(x);
			DECREF(v);
			DECREF(w);
			break;
		
		case STORE_NAME:
			w = GETNAMEV(oparg);
			v = POP();
			err = dict2insert(f->f_locals, w, v);
			DECREF(v);
			break;
		
		case DELETE_NAME:
			w = GETNAMEV(oparg);
			if ((err = dict2remove(f->f_locals, w)) != 0)
				err_setstr(NameError, getstringvalue(w));
			break;
		
		case UNPACK_ARG:
			/* Implement various compatibility hacks:
			   (a) f(a,b,...) should accept f((1,2,...))
			   (b) f((a,b,...)) should accept f(1,2,...)
			   (c) f(self,(a,b,...)) should accept f(x,1,2,...)
			*/
			{
				int n;
				if (EMPTY()) {
					err_setstr(TypeError,
						   "no argument list");
					why = WHY_EXCEPTION;
					break;
				}
				v = POP();
				if (!is_tupleobject(v)) {
					err_setstr(TypeError,
						   "bad argument list");
					why = WHY_EXCEPTION;
					break;
				}
				n = gettuplesize(v);
				if (n == 1 && oparg != 1) {
					/* Rule (a) */
					w = gettupleitem(v, 0);
					if (is_tupleobject(w)) {
						INCREF(w);
						DECREF(v);
						v = w;
						n = gettuplesize(v);
					}
				}
				else if (n != 1 && oparg == 1) {
					/* Rule (b) */
					PUSH(v);
					break;
					/* Don't fall through */
				}
				else if (n > 2 && oparg == 2) {
					/* Rule (c) */
					int i;
					w = newtupleobject(n-1);
					u = newtupleobject(2);
					if (u == NULL || w == NULL) {
						XDECREF(w);
						XDECREF(u);
						DECREF(v);
						why = WHY_EXCEPTION;
						break;
					}
					t = gettupleitem(v, 0);
					INCREF(t);
					settupleitem(u, 0, t);
					for (i = 1; i < n; i++) {
						t = gettupleitem(v, i);
						INCREF(t);
						settupleitem(w, i-1, t);
					}
					settupleitem(u, 1, w);
					DECREF(v);
					v = u;
					n = 2;
				}
				if (n != oparg) {
					err_setstr(TypeError,
						"arg count mismatch");
					why = WHY_EXCEPTION;
					DECREF(v);
					break;
				}
				PUSH(v);
			}
			/* Fall through */
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
					w = gettupleitem(v, oparg);
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
			name = GETNAME(oparg);
			v = POP();
			u = POP();
			err = setattr(v, name, u); /* v.name = u */
			DECREF(v);
			DECREF(u);
			break;
		
		case DELETE_ATTR:
			name = GETNAME(oparg);
			v = POP();
			err = setattr(v, name, (object *)NULL);
							/* del v.name */
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
				err_setstr(NameError, getstringvalue(w));
			break;
		
		case LOAD_CONST:
			x = GETCONST(oparg);
			INCREF(x);
			PUSH(x);
			break;
		
		case LOAD_NAME:
			w = GETNAMEV(oparg);
			x = dict2lookup(f->f_locals, w);
			if (x == NULL) {
				err_clear();
				x = dict2lookup(f->f_globals, w);
				if (x == NULL) {
					err_clear();
					x = getbuiltin(w);
					if (x == NULL) {
						err_setstr(NameError,
							getstringvalue(w));
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
				x = getbuiltin(w);
				if (x == NULL) {
					err_setstr(NameError,
						getstringvalue(w));
					break;
				}
			}
			INCREF(x);
			PUSH(x);
			break;
		
		case LOAD_LOCAL:
			w = GETNAMEV(oparg);
			x = dict2lookup(f->f_locals, w);
			if (x == NULL) {
				err_setstr(NameError, getstringvalue(w));
				break;
			}
			INCREF(x);
			PUSH(x);
			break;
		
		case BUILD_TUPLE:
			x = newtupleobject(oparg);
			if (x != NULL) {
				for (; --oparg >= 0;) {
					w = POP();
					err = settupleitem(x, oparg, w);
					if (err != 0)
						break;
				}
				PUSH(x);
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
			}
			break;
		
		case BUILD_MAP:
			x = newdictobject();
			PUSH(x);
			break;
		
		case LOAD_ATTR:
			name = GETNAME(oparg);
			v = POP();
			x = getattr(v, name);
			DECREF(v);
			PUSH(x);
			break;
		
		case COMPARE_OP:
			w = POP();
			v = POP();
			x = cmp_outcome((enum cmp_op)oparg, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(x);
			break;
		
		case IMPORT_NAME:
			name = GETNAME(oparg);
			x = import_module(name);
			XINCREF(x);
			PUSH(x);
			break;
		
		case IMPORT_FROM:
			w = GETNAMEV(oparg);
			v = TOP();
			err = import_from(f->f_locals, v, w);
			break;
		
		case JUMP_FORWARD:
			JUMPBY(oparg);
			break;
		
		case JUMP_IF_FALSE:
			if (!testbool(TOP()))
				JUMPBY(oparg);
			break;
		
		case JUMP_IF_TRUE:
			if (testbool(TOP()))
				JUMPBY(oparg);
			break;
		
		case JUMP_ABSOLUTE:
			JUMPTO(oparg);
			break;
		
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
			}
			else {
				DECREF(v);
				DECREF(w);
				/* A NULL can mean "s exhausted"
				   but also an error: */
				if (err_occurred())
					why = WHY_EXCEPTION;
				else
					JUMPBY(oparg);
			}
			break;
		
		case SETUP_LOOP:
		case SETUP_EXCEPT:
		case SETUP_FINALLY:
			setup_block(f, opcode, INSTR_OFFSET() + oparg,
						STACK_LEVEL());
			break;
		
		case SET_LINENO:
#ifdef LLTRACE
			if (lltrace)
				printf("--- Line %d ---\n", oparg);
#endif
			f->f_lineno = oparg;
			if (trace != NULL) {
				/* Trace each line of code reached */
				f->f_lasti = INSTR_OFFSET();
				x = call_trace(trace, f, "line", None);
				/* The trace function must return itself
				   in order to continue tracing */
				DECREF(trace);
				if (x == None) {
					DECREF(x);
					trace = NULL;
				}
				else
					trace = x;
			}
			break;
		
		default:
			fprintf(stderr,
				"XXX lineno: %d, opcode: %d\n",
				f->f_lineno, opcode);
			err_setstr(SystemError, "eval_code: unknown opcode");
			why = WHY_EXCEPTION;
			break;
		
		} /* switch */

	    on_error:
		
		/* Quickly continue if no error occurred */
		
		if (why == WHY_NOT) {
			if (err == 0 && x != NULL)
				continue; /* Normal, fast path */
			why = WHY_EXCEPTION;
			x = None;
			err = 0;
		}

#ifndef CHECKEXC
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
				fprintf(stderr, "XXX undetected error\n");
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

			if (trace)
				v = trace;
			else
				v = sysget("trace");
			if (v) {
				object *type, *value, *traceback, *arg;
				err_get(&type, &value);
				traceback = tb_fetch();
				arg = newtupleobject(3);
				if (arg == NULL)
					err_clear();
				else {
					settupleitem(arg, 0, type);
					settupleitem(arg, 1, value);
					settupleitem(arg, 2, traceback);
				}
				v = call_trace(v, f, "exception", arg);
				if (v == NULL) {
					/* Trace function raised error */
					tb_here(f);
					sysset("trace", (object *)NULL);
					XDECREF(trace);
					trace = NULL;
				}
				else {
					/* Restore original exception */
					err_setval(type, value);
					tb_store(traceback);
					if (v == None)
						DECREF(v);
					else {
						/* Set trace function */
						XDECREF(trace);
						trace = v;
					}
				}
				XDECREF(arg);
			}
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
					b->b_type == SETUP_EXCEPT &&
					why == WHY_EXCEPTION) {
				if (why == WHY_EXCEPTION) {
					object *exc, *val;
					err_get(&exc, &val);
					if (val == NULL) {
						val = None;
						INCREF(val);
					}
					v = tb_fetch();
					/* Make the raw exception data
					   available to the handler,
					   so a program can emulate the
					   Python main loop.  Don't do
					   this for 'finally'. */
					if (b->b_type == SETUP_EXCEPT) {
						sysset("exc_traceback", v);
						sysset("exc_value", val);
						sysset("exc_type", exc);
						err_clear();
					}
					PUSH(v);
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
	
	if (trace) {
		if (why == WHY_RETURN) {
			x = call_trace(trace, f, "return", retval);
			if (x == NULL) {
				XDECREF(retval);
				retval = NULL;
			}
			else
				DECREF(x);
		}
		DECREF(trace);
	}
	
	/* Restore previous frame and release the current one */
	
	current_frame = f->f_back;
	DECREF(f);
	
	return retval;
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

static object *
call_trace(trace, f, msg, arg)
	object *trace;
	frameobject *f;
	char *msg;
	object *arg;
{
	object *arglist, *what, *res;
	static int tracing = 0;
	
	if (tracing) {
		/* Don't trace the trace code! */
		INCREF(None);
		return None;
	}
	
	arglist = newtupleobject(3);
	if (arglist == NULL)
		return NULL;
	what = newstringobject(msg);
	if (what == NULL) {
		DECREF(arglist);
		return NULL;
	}
	INCREF(f);
	if (arg == NULL)
		arg = None;
	INCREF(arg);
	settupleitem(arglist, 0, (object *)f);
	settupleitem(arglist, 1, what);
	settupleitem(arglist, 2, arg);
	tracing++;
	res = call_object(trace, arglist);
	tracing--;
	if (res == NULL)
		tb_here(f);
	DECREF(arglist);
	return res;
}

object *
getlocals()
{
	if (current_frame == NULL)
		return NULL;
	else
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

void
printtraceback(fp)
	FILE *fp;
{
	object *v = tb_fetch();
	if (v != NULL) {
		fprintf(fp, "Stack backtrace (innermost last):\n");
		tb_print(v, fp);
		DECREF(v);
	}
}


void
flushline()
{
	if (softspace(sysget("stdout"), 0))
		fprintf(sysgetfile("stdout", stdout), "\n");
}


/* Test a value used as condition, e.g., in a for or if statement */

static int
testbool(v)
	object *v;
{
	if (v == None)
		return 0;
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_nonzero)(v);
	if (v->ob_type->tp_as_sequence != NULL)
		return (*v->ob_type->tp_as_sequence->sq_length)(v) != 0;
	if (v->ob_type->tp_as_mapping != NULL)
		return (*v->ob_type->tp_as_mapping->mp_length)(v) != 0;
	/* All other objects are 'true' */
	return 1;
}

static object *
or(v, w)
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
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
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
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
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
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
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
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
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
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
	if (v->ob_type->tp_as_number != NULL) {
		object *x;
		if (coerce(&v, &w) != 0)
			return NULL;
		x = (*v->ob_type->tp_as_number->nb_add)(v, w);
		DECREF(v);
		DECREF(w);
		return x;
	}
	else if (v->ob_type->tp_as_sequence != NULL)
		return (*v->ob_type->tp_as_sequence->sq_concat)(v, w);
	else {
		err_setstr(TypeError, "+ not supported by operands");
		return NULL;
	}
}

static object *
sub(v, w)
	object *v, *w;
{
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
	if (is_intobject(v) && w->ob_type->tp_as_sequence != NULL) {
		/* int*sequence -- swap v and w */
		object *tmp = v;
		v = w;
		w = tmp;
	}
	tp = v->ob_type;
	if (tp->tp_as_number != NULL) {
		object *x;
		if (coerce(&v, &w) != 0)
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
rem(v, w)
	object *v, *w;
{
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
	object * (*f) FPROTO((object *, object *));
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
	object *w = outcome == 0 ? True : False;
	INCREF(w);
	return w;
}
/* External interface to call any callable object. The arg may be NULL. */

object *
call_object(func, arg)
	object *func;
	object *arg;
{
	if (is_instancemethodobject(func) || is_funcobject(func))
		return call_function(func, arg);
	else
		return call_builtin(func, arg);
}

static object *
call_builtin(func, arg)
	object *func;
	object *arg;
{
	if (is_methodobject(func)) {
		method meth = getmethod(func);
		object *self = getself(func);
		if (!getvarargs(func) && arg != NULL && is_tupleobject(arg)) {
			int size = gettuplesize(arg);
			if (size == 1)
				arg = gettupleitem(arg, 0);
			else if (size == 0)
				arg = NULL;
		}
		return (*meth)(self, arg);
	}
	if (is_classobject(func)) {
		if (arg != NULL &&
				!(is_tupleobject(arg) &&
					gettuplesize(arg) == 0)) {
			err_setstr(TypeError,
				"classobject() allows no arguments");
			return NULL;
		}
		return newinstanceobject(func);
	}
	err_setstr(TypeError, "call of non-function");
	return NULL;
}

static object *
call_function(func, arg)
	object *func;
	object *arg;
{
	object *newarg = NULL;
	object *newlocals, *newglobals;
	object *co, *v;
	
	if (is_instancemethodobject(func)) {
		int argcount;
		object *self = instancemethodgetself(func);
		func = instancemethodgetfunc(func);
		if (arg == NULL)
			argcount = 0;
		else if (is_tupleobject(arg))
			argcount = gettuplesize(arg);
		else
			argcount = 1;
		newarg = newtupleobject(argcount + 1);
		if (newarg == NULL)
			return NULL;
		INCREF(self);
		settupleitem(newarg, 0, self);
		if (arg != NULL && !is_tupleobject(arg)) {
			INCREF(arg);
			settupleitem(newarg, 1, arg);
		}
		else {
			int i;
			object *v;
			for (i = 0; i < argcount; i++) {
				v = gettupleitem(arg, i);
				XINCREF(v);
				settupleitem(newarg, i+1, v);
			}
		}
		arg = newarg;
	}
	else {
		if (!is_funcobject(func)) {
			err_setstr(TypeError, "call of non-function");
			return NULL;
		}
	}
	
	co = getfunccode(func);
	if (co == NULL) {
		XDECREF(newarg);
		return NULL;
	}
	if (!is_codeobject(co)) {
		fprintf(stderr, "XXX Bad code\n");
		abort();
	}
	newlocals = newdictobject();
	if (newlocals == NULL) {
		XDECREF(newarg);
		return NULL;
	}
	
	newglobals = getfuncglobals(func);
	INCREF(newglobals);
	
	v = eval_code((codeobject *)co, newglobals, newlocals, arg);
	
	DECREF(newlocals);
	DECREF(newglobals);
	
	XDECREF(newarg);
	
	return v;
}

static object *
apply_subscript(v, w)
	object *v, *w;
{
	typeobject *tp = v->ob_type;
	if (tp->tp_as_sequence == NULL && tp->tp_as_mapping == NULL) {
		err_setstr(TypeError, "unsubscriptable object");
		return NULL;
	}
	if (tp->tp_as_sequence != NULL) {
		int i;
		if (!is_intobject(w)) {
			err_setstr(TypeError, "sequence subscript not int");
			return NULL;
		}
		i = getintvalue(w);
		if (i < 0)
			i += (*tp->tp_as_sequence->sq_length)(v);
		return (*tp->tp_as_sequence->sq_item)(v, i);
	}
	return (*tp->tp_as_mapping->mp_subscript)(v, w);
}

static object *
loop_subscript(v, w)
	object *v, *w;
{
	sequence_methods *sq = v->ob_type->tp_as_sequence;
	int i, n;
	if (sq == NULL) {
		err_setstr(TypeError, "loop over non-sequence");
		return NULL;
	}
	i = getintvalue(w);
	n = (*sq->sq_length)(v);
	if (i >= n)
		return NULL; /* End of loop */
	return (*sq->sq_item)(v, i);
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
	int (*func)();
	if ((sq = tp->tp_as_sequence) != NULL &&
			(func = sq->sq_ass_item) != NULL) {
		if (!is_intobject(key)) {
			err_setstr(TypeError,
			"sequence subscript must be integer (assign or del)");
			return -1;
		}
		else {
			int i = getintvalue(key);
			if (i < 0)
				i += (*sq->sq_length)(w);
			return (*func)(w, i, v);
		}
	}
	else if ((mp = tp->tp_as_mapping) != NULL &&
			(func = mp->mp_ass_subscript) != NULL) {
		return (*func)(w, key, v);
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
			if (cmp_exception(err, gettupleitem(v, i)))
				return 1;
		}
		return 0;
	}
	return err == v;
}

static int
cmp_member(v, w)
	object *v, *w;
{
	int i, n, cmp;
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
	n = (*sq->sq_length)(w);
	for (i = 0; i < n; i++) {
		x = (*sq->sq_item)(w, i);
		cmp = cmpobject(v, x);
		XDECREF(x);
		if (cmp == 0)
			return 1;
	}
	return 0;
}

static object *
cmp_outcome(op, v, w)
	enum cmp_op op;
	register object *v;
	register object *w;
{
	register int cmp;
	register int res = 0;
	switch (op) {
	case IS:
	case IS_NOT:
		res = (v == w);
		if (op == IS_NOT)
			res = !res;
		break;
	case IN:
	case NOT_IN:
		res = cmp_member(v, w);
		if (res < 0)
			return NULL;
		if (op == NOT_IN)
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
	w = getmoduledict(v);
	if (getstringvalue(name)[0] == '*') {
		int i;
		int n = getdictsize(w);
		for (i = 0; i < n; i++) {
			name = getdict2key(w, i);
			if (name == NULL || getstringvalue(name)[0] == '_')
				continue;
			x = dict2lookup(w, name);
			if (x == NULL) {
				/* XXX can't happen? */
				err_setstr(SystemError, getstringvalue(name));
				return -1;
			}
			if (dict2insert(locals, name, x) != 0)
				return -1;
		}
		return 0;
	}
	else {
		x = dict2lookup(w, name);
		if (x == NULL) {
			char buf[250];
			sprintf(buf, "cannot import name %s",
				getstringvalue(name));
			err_setstr(ImportError, buf);
			return -1;
		}
		else
			return dict2insert(locals, name, x);
	}
}

static object *
build_class(v, w)
	object *v; /* None or tuple containing base classes */
	object *w; /* dictionary */
{
	if (is_tupleobject(v)) {
		int i;
		for (i = gettuplesize(v); --i >= 0; ) {
			object *x = gettupleitem(v, i);
			if (!is_classobject(x)) {
				err_setstr(TypeError,
					"base is not a class object");
				return NULL;
			}
		}
	}
	else {
		v = NULL;
	}
	if (!is_dictobject(w)) {
		err_setstr(SystemError, "build_class with non-dictionary");
		return NULL;
	}
	return newclassobject(v, w, (object *) NULL);
}
