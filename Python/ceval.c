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
#define TRACE
#endif

#ifdef TRACE
static int
prtrace(v, str)
	object *v;
	char *str;
{
	printf("%s ", str);
	printobject(v, stdout, 0);
	printf("\n");
}
#endif

static frameobject *current_frame;

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


/* XXX Mixing "print ...," and direct file I/O on stdin/stdout
   XXX has some bad consequences.  The needspace flag should
   XXX really be part of the file object. */

static int needspace;

void
flushline()
{
	FILE *fp = sysgetfile("stdout", stdout);
	if (needspace) {
		fprintf(fp, "\n");
		needspace = 0;
	}
}


/* Test a value used as condition, e.g., in a for or if statement */

static int
testbool(v)
	object *v;
{
	if (is_intobject(v))
		return getintvalue(v) != 0;
	if (is_floatobject(v))
		return getfloatvalue(v) != 0.0;
	if (v->ob_type->tp_as_sequence != NULL)
		return (*v->ob_type->tp_as_sequence->sq_length)(v) != 0;
	if (v->ob_type->tp_as_mapping != NULL)
		return (*v->ob_type->tp_as_mapping->mp_length)(v) != 0;
	if (v == None)
		return 0;
	/* All other objects are 'true' */
	return 1;
}

static object *
add(v, w)
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL)
		v = (*v->ob_type->tp_as_number->nb_add)(v, w);
	else if (v->ob_type->tp_as_sequence != NULL)
		v = (*v->ob_type->tp_as_sequence->sq_concat)(v, w);
	else {
		err_setstr(TypeError, "+ not supported by operands");
		return NULL;
	}
	return v;
}

static object *
sub(v, w)
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_subtract)(v, w);
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
	if (tp->tp_as_number != NULL)
		return (*tp->tp_as_number->nb_multiply)(v, w);
	if (tp->tp_as_sequence != NULL) {
		if (!is_intobject(w)) {
			err_setstr(TypeError,
				"can't multiply sequence with non-int");
			return NULL;
		}
		if (tp->tp_as_sequence->sq_repeat == NULL) {
			err_setstr(TypeError, "sequence does not support *");
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
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_divide)(v, w);
	err_setstr(TypeError, "bad operand type(s) for /");
	return NULL;
}

static object *
rem(v, w)
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL)
		return (*v->ob_type->tp_as_number->nb_remainder)(v, w);
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
not(v)
	object *v;
{
	int outcome = testbool(v);
	object *w = outcome == 0 ? True : False;
	INCREF(w);
	return w;
}

static object *
call_builtin(func, arg)
	object *func;
	object *arg;
{
	if (is_methodobject(func)) {
		method meth = getmethod(func);
		object *self = getself(func);
		return (*meth)(self, arg);
	}
	if (is_classobject(func)) {
		if (arg != NULL) {
			err_setstr(TypeError,
				"classobject() allows no arguments");
			return NULL;
		}
		return newclassmemberobject(func);
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
	
	if (is_classmethodobject(func)) {
		object *self = classmethodgetself(func);
		func = classmethodgetfunc(func);
		if (arg == NULL) {
			arg = self;
		}
		else {
			newarg = newtupleobject(2);
			if (newarg == NULL)
				return NULL;
			INCREF(self);
			INCREF(arg);
			settupleitem(newarg, 0, self);
			settupleitem(newarg, 1, arg);
			arg = newarg;
		}
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
				"sequence subscript must be integer");
			return -1;
		}
		else
			return (*func)(w, (int)getintvalue(key), v);
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
			if (err == gettupleitem(v, i))
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
	char *name;
{
	object *w, *x;
	w = getmoduledict(v);
	if (name[0] == '*') {
		int i;
		int n = getdictsize(w);
		for (i = 0; i < n; i++) {
			name = getdictkey(w, i);
			if (name == NULL || name[0] == '_')
				continue;
			x = dictlookup(w, name);
			if (x == NULL) {
				/* XXX can't happen? */
				err_setstr(NameError, name);
				return -1;
			}
			if (dictinsert(locals, name, x) != 0)
				return -1;
		}
		return 0;
	}
	else {
		x = dictlookup(w, name);
		if (x == NULL) {
			err_setstr(NameError, name);
			return -1;
		}
		else
			return dictinsert(locals, name, x);
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
	return newclassobject(v, w);
}


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
	int lineno;		/* Current line number */
	object *retval;		/* Return value iff why == WHY_RETURN */
	char *name;		/* Name used by some instructions */
	FILE *fp;		/* Used by print operations */
#ifdef TRACE
	int trace = dictlookup(globals, "__trace__") != NULL;
#endif

/* Code access macros */

#define GETCONST(i)	Getconst(f, i)
#define GETNAME(i)	Getname(f, i)
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

#ifdef TRACE
#define PUSH(v)		(BASIC_PUSH(v), trace && prtrace(TOP(), "push"))
#define POP()		(trace && prtrace(TOP(), "pop"), BASIC_POP())
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
	
	next_instr = GETUSTRINGVALUE(f->f_code->co_code);
	
	stack_pointer = f->f_valuestack;
	
	if (arg != NULL) {
		INCREF(arg);
		PUSH(arg);
	}
	
	why = WHY_NOT;
	err = 0;
	x = None;	/* Not a reference, just anything non-NULL */
	lineno = -1;
	
	for (;;) {
		static ticker;
		
		/* Do periodic things */
		
		if (--ticker < 0) {
			ticker = 100;
			if (intrcheck()) {
				err_set(KeyboardInterrupt);
				why = WHY_EXCEPTION;
				tb_here(f, INSTR_OFFSET(), lineno);
				break;
			}
		}
		
		/* Extract opcode and argument */
		
		opcode = NEXTOP();
		if (HAS_ARG(opcode))
			oparg = NEXTARG();

#ifdef TRACE
		/* Instruction tracing */
		
		if (trace) {
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
			if (is_classmethodobject(v) || is_funcobject(v))
				x = call_function(v, (object *)NULL);
			else
				x = call_builtin(v, (object *)NULL);
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
			if (is_classmethodobject(v) || is_funcobject(v))
				x = call_function(v, w);
			else
				x = call_builtin(v, w);
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
				printobject(v, fp, 0);
				fprintf(fp, "\n");
			}
			DECREF(v);
			break;
		
		case PRINT_ITEM:
			v = POP();
			fp = sysgetfile("stdout", stdout);
			if (needspace)
				fprintf(fp, " ");
			if (is_stringobject(v)) {
				char *s = getstringvalue(v);
				int len = getstringsize(v);
				fwrite(s, 1, len, fp);
				if (len > 0 && s[len-1] == '\n')
					needspace = 0;
				else
					needspace = 1;
			}
			else {
				printobject(v, fp, 0);
				needspace = 1;
			}
			DECREF(v);
			break;
		
		case PRINT_NEWLINE:
			fp = sysgetfile("stdout", stdout);
			fprintf(fp, "\n");
			needspace = 0;
			break;
		
		case BREAK_LOOP:
			why = WHY_BREAK;
			break;
		
		case RAISE_EXCEPTION:
			v = POP();
			w = POP();
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
		
		case REQUIRE_ARGS:
			if (EMPTY()) {
				err_setstr(TypeError,
					"function expects argument(s)");
				why = WHY_EXCEPTION;
			}
			break;
		
		case REFUSE_ARGS:
			if (!EMPTY()) {
				err_setstr(TypeError,
					"function expects no argument(s)");
				why = WHY_EXCEPTION;
			}
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
			name = GETNAME(oparg);
			v = POP();
			err = dictinsert(f->f_locals, name, v);
			DECREF(v);
			break;
		
		case DELETE_NAME:
			name = GETNAME(oparg);
			if ((err = dictremove(f->f_locals, name)) != 0)
				err_setstr(NameError, name);
			break;
		
		case UNPACK_TUPLE:
			v = POP();
			if (!is_tupleobject(v)) {
				err_setstr(TypeError, "unpack non-tuple");
				why = WHY_EXCEPTION;
			}
			else if (gettuplesize(v) != oparg) {
				err_setstr(RuntimeError,
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
				err_setstr(RuntimeError,
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
		
		case LOAD_CONST:
			x = GETCONST(oparg);
			INCREF(x);
			PUSH(x);
			break;
		
		case LOAD_NAME:
			name = GETNAME(oparg);
			x = dictlookup(f->f_locals, name);
			if (x == NULL) {
				x = dictlookup(f->f_globals, name);
				if (x == NULL)
					x = getbuiltin(name);
			}
			if (x == NULL)
				err_setstr(NameError, name);
			else
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
			name = GETNAME(oparg);
			v = TOP();
			err = import_from(f->f_locals, v, name);
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
#ifdef TRACE
			if (trace)
				printf("--- Line %d ---\n", oparg);
#endif
			lineno = oparg;
			break;
		
		default:
			fprintf(stderr,
				"XXX lineno: %d, opcode: %d\n",
				lineno, opcode);
			err_setstr(SystemError, "eval_code: unknown opcode");
			why = WHY_EXCEPTION;
			break;
		
		} /* switch */

		
		/* Quickly continue if no error occurred */
		
		if (why == WHY_NOT) {
			if (err == 0 && x != NULL)
				continue; /* Normal, fast path */
			why = WHY_EXCEPTION;
			x = None;
			err = 0;
		}

#ifndef NDEBUG
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
			int lasti = INSTR_OFFSET() - 1;
			if (HAS_ARG(opcode))
				lasti -= 2;
			tb_here(f, lasti, lineno);
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
#if 0 /* Oops, this breaks too many things */
						sysset("exc_traceback", v);
#endif
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
	
	/* Restore previous frame and release the current one */
	
	current_frame = f->f_back;
	DECREF(f);
	
	if (why == WHY_RETURN)
		return retval;
	else
		return NULL;
}
