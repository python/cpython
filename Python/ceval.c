/* Evaluate compiled expression nodes */

#include <stdio.h>
#include <ctype.h>
#include "string.h"

#include "PROTO.h"
#include "object.h"
#include "objimpl.h"
#include "intobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "builtinobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "context.h"
#include "funcobject.h"
#include "classobject.h"
#include "token.h"
#include "graminit.h"
#include "run.h"
#include "import.h"
#include "support.h"
#include "sysmodule.h"
#include "compile.h"
#include "opcode.h"

/* List access macros */
#ifdef NDEBUG
#define GETITEM(v, i) GETLISTITEM((listobject *)(v), (i))
#define GETITEMNAME(v, i) GETSTRINGVALUE((stringobject *)GETITEM((v), (i)))
#else
#define GETITEM(v, i) getlistitem((v), (i))
#define GETITEMNAME(v, i) getstringvalue(getlistitem((v), (i)))
#endif

typedef struct {
	int b_type;		/* what kind of block this is */
	int b_handler;		/* where to jump to find handler */
	int b_level;		/* value stack level to pop to */
} block;

typedef struct _frame {
	OB_HEAD
	struct _frame *f_back;	/* previous frame, or NULL */
	codeobject *f_code;	/* code segment */
	object *f_locals;	/* local symbol table (dictobject) */
	object *f_globals;	/* global symbol table (dictobject) */
	object **f_valuestack;	/* malloc'ed array */
	block *f_blockstack;	/* malloc'ed array */
	int f_nvalues;		/* size of f_valuestack */
	int f_nblocks;		/* size of f_blockstack */
	int f_ivalue;		/* index in f_valuestack */
	int f_iblock;		/* index in f_blockstack */
	int f_nexti;		/* index in f_code (next instruction) */
} frameobject;

#define is_frameobject(op) ((op)->ob_type == &Frametype)

static void
frame_dealloc(f)
	frameobject *f;
{
	XDECREF(f->f_back);
	XDECREF(f->f_code);
	XDECREF(f->f_locals);
	XDECREF(f->f_globals);
	XDEL(f->f_valuestack);
	XDEL(f->f_blockstack);
	DEL(f);
}
typeobject Frametype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"frame",
	sizeof(frameobject),
	0,
	frame_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

static frameobject * newframeobject PROTO(
	(frameobject *, codeobject *, object *, object *, int, int));

static frameobject *
newframeobject(back, code, locals, globals, nvalues, nblocks)
	frameobject *back;
	codeobject *code;
	object *locals;
	object *globals;
	int nvalues;
	int nblocks;
{
	frameobject *f;
	if ((back != NULL && !is_frameobject(back)) ||
		code == NULL || !is_codeobject(code) ||
		locals == NULL || !is_dictobject(locals) ||
		globals == NULL || !is_dictobject(globals) ||
		nvalues < 0 || nblocks < 0) {
		err_badcall();
		return NULL;
	}
	f = NEWOBJ(frameobject, &Frametype);
	if (f != NULL) {
		if (back)
			INCREF(back);
		f->f_back = back;
		INCREF(code);
		f->f_code = code;
		INCREF(locals);
		f->f_locals = locals;
		INCREF(globals);
		f->f_globals = globals;
		f->f_valuestack = NEW(object *, nvalues+1);
		f->f_blockstack = NEW(block, nblocks+1);
		f->f_nvalues = nvalues;
		f->f_nblocks = nblocks;
		f->f_ivalue = f->f_iblock = f->f_nexti = 0;
		if (f->f_valuestack == NULL || f->f_blockstack == NULL) {
			err_nomem();
			DECREF(f);
			f = NULL;
		}
	}
	return f;
}

#define GETUSTRINGVALUE(s) ((unsigned char *)GETSTRINGVALUE(s))

#define Push(f, v)	((f)->f_valuestack[(f)->f_ivalue++] = (v))
#define Pop(f)		((f)->f_valuestack[--(f)->f_ivalue])
#define Top(f)		((f)->f_valuestack[(f)->f_ivalue-1])
#define Empty(f)	((f)->f_ivalue == 0)
#define Full(f)		((f)->f_ivalue == (f)->f_nvalues)
#define Nextbyte(f)	(GETUSTRINGVALUE((f)->f_code->co_code)[(f)->f_nexti++])
#define Peekbyte(f)	(GETUSTRINGVALUE((f)->f_code->co_code)[(f)->f_nexti])
#define Peekint(f)	\
	(GETUSTRINGVALUE((f)->f_code->co_code)[(f)->f_nexti] + \
	 GETUSTRINGVALUE((f)->f_code->co_code)[(f)->f_nexti+1])
#define Prevbyte(f)	(GETUSTRINGVALUE((f)->f_code->co_code)[(f)->f_nexti-1])
#define Jumpto(f, x)	((f)->f_nexti = (x))
#define Jumpby(f, x)	((f)->f_nexti += (x))
#define Getconst(f, i)	(GETITEM((f)->f_code->co_consts, (i)))
#define Getname(f, i)	(GETITEMNAME((f)->f_code->co_names, (i)))

/* Corresponding functions, for debugging */

static void
push(f, v)
	frameobject *f;
	object *v;
{
	if (Full(f)) {
		printf("stack overflow\n");
		abort();
	}
	Push(f, v);
}

static object *
pop(f)
	frameobject *f;
{
	if (Empty(f)) {
		printf("stack underflow\n");
		abort();
	}
	return Pop(f);
}

static object *
top(f)
	frameobject *f;
{
	if (Empty(f)) {
		printf("stack underflow\n");
		abort();
	}
	return Top(f);
}

static int
nextbyte(f)
	frameobject *f;
{
	stringobject *code = (f)->f_code->co_code;
	if (f->f_nexti >= getstringsize((object *)code)) {
		printf("ran off end of instructions\n");
		abort();
	}
	return GETUSTRINGVALUE(code)[f->f_nexti++];
}

static int
nextint(f)
	frameobject *f;
{
	int a, b;
#ifdef NDEBUG
	a = Nextbyte(f);
	b = Nextbyte(f);
#else
	a = nextbyte(f);
	b = nextbyte(f);
#endif
	return a + (b << 8);
}

/* Tracing versions */

static void
trace_push(f, v)
	frameobject *f;
	object *v;
{
	printf("\tpush ");
	printobject(v, stdout, 0);
	printf("\n");
	push(f, v);
}

static object *
trace_pop(f)
	frameobject *f;
{
	object *v;
	v = pop(f);
	printf("\tpop ");
	printobject(v, stdout, 0);
	printf("\n");
	return v;
}

static object *
trace_top(f)
	frameobject *f;
{
	object *v;
	v = top(f);
	printf("\ttop ");
	printobject(v, stdout, 0);
	printf("\n");
	return v;
}

static int
trace_nextop(f)
	frameobject *f;
{
	int op;
	int arg;
	
	printf("%d: ", f->f_nexti);
	op = nextbyte(f);
	if (op < HAVE_ARGUMENT)
		printf("op %3d\n", op);
	else {
		arg = Peekint(f);
		printf("op %d arg %d\n", op, arg);
	}
	return op;
}

/* Block management */

static void
setup_block(f, type, handler)
	frameobject *f;
	int type;
	int handler;
{
	block *b;
	if (f->f_iblock >= f->f_nblocks) {
		printf("block stack overflow\n");
		abort();
	}
	b = &f->f_blockstack[f->f_iblock++];
	b->b_type = type;
	b->b_level = f->f_ivalue;
	b->b_handler = handler + f->f_nexti;
}

static block *
pop_block(f)
	frameobject *f;
{
	block *b;
	if (f->f_iblock <= 0) {
		printf("block stack underflow\n");
		abort();
	}
	b = &f->f_blockstack[--f->f_iblock];
	while (f->f_ivalue > b->b_level) {
		object *v = Pop(f);
		XDECREF(v);
	}
	return b;
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

static object *
checkerror(ctx, v)
	context *ctx;
	object *v;
{
	if (v == NULL)
		puterrno(ctx);
	return v;
}

static object *
add(ctx, v, w)
	context *ctx;
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL)
		v = (*v->ob_type->tp_as_number->nb_add)(v, w);
	else if (v->ob_type->tp_as_sequence != NULL)
		v = (*v->ob_type->tp_as_sequence->sq_concat)(v, w);
	else {
		type_error(ctx, "+ not supported by operands");
		return NULL;
	}
	return checkerror(ctx, v);
}

static object *
sub(ctx, v, w)
	context *ctx;
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL)
		return checkerror(ctx,
			(*v->ob_type->tp_as_number->nb_subtract)(v, w));
	type_error(ctx, "bad operand type(s) for -");
	return NULL;
}

static object *
mul(ctx, v, w)
	context *ctx;
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
		return checkerror(ctx, (*tp->tp_as_number->nb_multiply)(v, w));
	if (tp->tp_as_sequence != NULL) {
		if (!is_intobject(w)) {
			type_error(ctx, "can't multiply sequence with non-int");
			return NULL;
		}
		if (tp->tp_as_sequence->sq_repeat == NULL) {
			type_error(ctx, "sequence does not support *");
			return NULL;
		}
		return checkerror(ctx, (*tp->tp_as_sequence->sq_repeat)
						(v, (int)getintvalue(w)));
	}
	type_error(ctx, "bad operand type(s) for *");
	return NULL;
}

static object *
div(ctx, v, w)
	context *ctx;
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL)
		return checkerror(ctx,
			(*v->ob_type->tp_as_number->nb_divide)(v, w));
	type_error(ctx, "bad operand type(s) for /");
	return NULL;
}

static object *
rem(ctx, v, w)
	context *ctx;
	object *v, *w;
{
	if (v->ob_type->tp_as_number != NULL)
		return checkerror(ctx,
			(*v->ob_type->tp_as_number->nb_remainder)(v, w));
	type_error(ctx, "bad operand type(s) for %");
	return NULL;
}

static object *
neg(ctx, v)
	context *ctx;
	object *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return checkerror(ctx,
			(*v->ob_type->tp_as_number->nb_negative)(v));
	type_error(ctx, "bad operand type(s) for unary -");
	return NULL;
}

static object *
pos(ctx, v)
	context *ctx;
	object *v;
{
	if (v->ob_type->tp_as_number != NULL)
		return checkerror(ctx,
			(*v->ob_type->tp_as_number->nb_positive)(v));
	type_error(ctx, "bad operand type(s) for unary +");
	return NULL;
}

static object *
not(ctx, v)
	context *ctx;
	object *v;
{
	int outcome = testbool(ctx, v);
	if (ctx->ctx_exception)
		return NULL;
	return checkerror(ctx, newintobject((long) !outcome));
}

static object *
call_builtin(ctx, func, args)
	context *ctx;
	object *func;
	object *args;
{
	if (is_builtinobject(func)) {
		function funcptr = getbuiltinfunction(func);
		return (*funcptr)(ctx, args);
	}
	if (is_methodobject(func)) {
		method meth = getmethod(func);
		object *self = getself(func);
		return checkerror(ctx, (*meth)(self, args));
	}
	if (is_classobject(func)) {
		if (args != NULL) {
			type_error(ctx, "classobject() allows no arguments");
			return NULL;
		}
		return checkerror(ctx, newclassmemberobject(func));
	}
	type_error(ctx, "call of non-function");
	return NULL;
}

static object *eval_compiled PROTO((context *, codeobject *, object *, int));

/* XXX Eventually, this should not call eval_compiled recursively
   but create a new frame */

static object *
call_function(ctx, func, args)
	context *ctx;
	object *func;
	object *args;
{
	object *newargs = NULL;
	object *savelocals, *newlocals, *saveglobals;
	object *c, *v;
	
	if (is_classmethodobject(func)) {
		object *self = classmethodgetself(func);
		func = classmethodgetfunc(func);
		if (args == NULL) {
			args = self;
		}
		else {
			newargs = checkerror(ctx, newtupleobject(2));
			if (newargs == NULL)
				return NULL;
			INCREF(self);
			INCREF(args);
			settupleitem(newargs, 0, self);
			settupleitem(newargs, 1, args);
			args = newargs;
		}
	}
	else {
		if (!is_funcobject(func)) {
			type_error(ctx, "call of non-function");
			return NULL;
		}
	}
	
	c = checkerror(ctx, getfunccode(func));
	if (c == NULL) {
		XDECREF(newargs);
		return NULL;
	}
	if (!is_codeobject(c)) {
		printf("Bad code\n");
		abort();
	}
	newlocals = checkerror(ctx, newdictobject());
	if (newlocals == NULL) {
		XDECREF(newargs);
		return NULL;
	}
	
	savelocals = ctx->ctx_locals;
	ctx->ctx_locals = newlocals;
	saveglobals = ctx->ctx_globals;
	ctx->ctx_globals = getfuncglobals(func);
	
	v = eval_compiled(ctx, (codeobject *)c, args, 1);
	
	DECREF(ctx->ctx_locals);
	ctx->ctx_locals = savelocals;
	ctx->ctx_globals = saveglobals;
	
	XDECREF(newargs);
	
	return v;
}

static object *
apply_subscript(ctx, v, w)
	context *ctx;
	object *v, *w;
{
	typeobject *tp = v->ob_type;
	if (tp->tp_as_sequence == NULL && tp->tp_as_mapping == NULL) {
		type_error(ctx, "unsubscriptable object");
		return NULL;
	}
	if (tp->tp_as_sequence != NULL) {
		int i;
		if (!is_intobject(w)) {
			type_error(ctx, "sequence subscript not int");
			return NULL;
		}
		i = getintvalue(w);
		return checkerror(ctx, (*tp->tp_as_sequence->sq_item)(v, i));
	}
	return checkerror(ctx, (*tp->tp_as_mapping->mp_subscript)(v, w));
}

static object *
loop_subscript(ctx, v, w)
	context *ctx;
	object *v, *w;
{
	sequence_methods *sq = v->ob_type->tp_as_sequence;
	int i, n;
	if (sq == NULL) {
		type_error(ctx, "loop over non-sequence");
		return NULL;
	}
	i = getintvalue(w);
	n = (*sq->sq_length)(v);
	if (i >= n)
		return NULL; /* End of loop */
	return checkerror(ctx, (*sq->sq_item)(v, i));
}

static int
slice_index(ctx, v, isize, pi)
	context *ctx;
	object *v;
	int isize;
	int *pi;
{
	if (v != NULL) {
		if (!is_intobject(v)) {
			type_error(ctx, "slice index must be int");
			return 0;
		}
		*pi = getintvalue(v);
		if (*pi < 0)
			*pi += isize;
	}
	return 1;
}

static object *
apply_slice(ctx, u, v, w) /* u[v:w] */
	context *ctx;
	object *u, *v, *w;
{
	typeobject *tp = u->ob_type;
	int ilow, ihigh, isize;
	if (tp->tp_as_sequence == NULL) {
		type_error(ctx, "only sequences can be sliced");
		return NULL;
	}
	ilow = 0;
	isize = ihigh = (*tp->tp_as_sequence->sq_length)(u);
	if (!slice_index(ctx, v, isize, &ilow))
		return NULL;
	if (!slice_index(ctx, w, isize, &ihigh))
		return NULL;
	return checkerror(ctx, (*tp->tp_as_sequence->sq_slice)(u, ilow, ihigh));
}
static void
assign_subscript(ctx, w, key, v)
	context *ctx;
	object *w;
	object *key;
	object *v;
{
	typeobject *tp = w->ob_type;
	sequence_methods *sq;
	mapping_methods *mp;
	int (*func)();
	int err;
	if ((sq = tp->tp_as_sequence) != NULL &&
			(func = sq->sq_ass_item) != NULL) {
		if (!is_intobject(key)) {
			type_error(ctx, "sequence subscript must be integer");
			return;
		}
		err = (*func)(w, (int)getintvalue(key), v);
	}
	else if ((mp = tp->tp_as_mapping) != NULL &&
			(func = mp->mp_ass_subscript) != NULL) {
		err = (*func)(w, key, v);
	}
	else {
		type_error(ctx, "can't assign to this subscripted object");
		return;
	}
	if (err != 0)
		puterrno(ctx);
}

static void
assign_slice(ctx, u, v, w, x) /* u[v:w] = x */
	context *ctx;
	object *u, *v, *w, *x;
{
	typeobject *tp = u->ob_type;
	int ilow, ihigh, isize;
	if (tp->tp_as_sequence == NULL ||
			tp->tp_as_sequence->sq_ass_slice == NULL) {
		type_error(ctx, "unassignable slice");
		return;
	}
	ilow = 0;
	isize = ihigh = (*tp->tp_as_sequence->sq_length)(u);
	if (!slice_index(ctx, v, isize, &ilow))
		return;
	if (!slice_index(ctx, w, isize, &ihigh))
		return;
	if ((*tp->tp_as_sequence->sq_ass_slice)(u, ilow, ihigh, x) != 0)
		puterrno(ctx);
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

static object *
cmp_outcome(ctx, op, v, w)
	register context *ctx;
	enum cmp_op op;
	register object *v;
	register object *w;
{
	register int cmp;
	register int res = 0;
	switch (op) {
	case IN:
	case NOT_IN:
		cmp = cmp_member(ctx, v, w);
		break;
	case IS:
	case IS_NOT:
		cmp = (v == w);
		break;
	case EXC_MATCH:
		cmp = cmp_exception(v, w);
		break;
	default:
		cmp = cmp_values(ctx, v, w);
	}
	if (ctx->ctx_exception)
		return NULL;
	switch (op) {
	case EXC_MATCH:
	case IS:
	case IN:     res =  cmp; break;
	case IS_NOT:
	case NOT_IN: res = !cmp; break;
	case LT: res = cmp <  0; break;
	case LE: res = cmp <= 0; break;
	case EQ: res = cmp == 0; break;
	case NE: res = cmp != 0; break;
	case GT: res = cmp >  0; break;
	case GE: res = cmp >= 0; break;
	/* XXX no default? */
	}
	v = res ? True : False;
	INCREF(v);
	return v;
}

static void
import_from(ctx, v, name)
	context *ctx;
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
				name_error(ctx, name);
				break;
			}
			if (dictinsert(ctx->ctx_locals, name, x) != 0) {
				puterrno(ctx);
				break;
			}
		}
	}
	else {
		x = dictlookup(w, name);
		if (x == NULL)
			name_error(ctx, name);
		else if (dictinsert(ctx->ctx_locals, name, x) != 0)
			puterrno(ctx);
	}
}

static object *
build_class(ctx, v, w)
	context *ctx;
	object *v; /* None or tuple containing base classes */
	object *w; /* dictionary */
{
	if (is_tupleobject(v)) {
		int i;
		for (i = gettuplesize(v); --i >= 0; ) {
			object *x = gettupleitem(v, i);
			if (!is_classobject(x)) {
				type_error(ctx, "base is not a class object");
				return NULL;
			}
		}
	}
	else {
		v = NULL;
	}
	if (!is_dictobject(w)) {
		sys_error(ctx, "build_class with non-dictionary");
		return NULL;
	}
	return checkerror(ctx, newclassobject(v, w));
}

static object *
eval_compiled(ctx, co, arg, needvalue)
	context *ctx;
	codeobject *co;
	object *arg;
	int needvalue;
{
	frameobject *f;
	register object *v;
	register object *w;
	register object *u;
	register object *x;
	char *name;
	int i, op;
	FILE *fp;
#ifndef NDEBUG
	int trace = dictlookup(ctx->ctx_globals, "__trace") != NULL;
#endif

	f = newframeobject(
			(frameobject *)NULL,	/*back*/
			co,			/*code*/
			ctx->ctx_locals,	/*locals*/
			ctx->ctx_globals,	/*globals*/
			50,			/*nvalues*/
			20);			/*nblocks*/
	if (f == NULL) {
		puterrno(ctx);
		return NULL;
	}

#define EMPTY()		Empty(f)
#define FULL()		Full(f)
#define GETCONST(i)	Getconst(f, i)
#define GETNAME(i)	Getname(f, i)
#define JUMPTO(x)	Jumpto(f, x)
#define JUMPBY(x)	Jumpby(f, x)

#ifdef NDEBUG

#define PUSH(v) 	Push(f, v)
#define TOP()		Top(f)
#define POP()		Pop(f)

#else

#define PUSH(v) if(trace) trace_push(f, v); else push(f, v)
#define TOP() (trace ? trace_top(f) : top(f))
#define POP() (trace ? trace_pop(f) : pop(f))

#endif

	if (arg != NULL) {
		INCREF(arg);
		PUSH(arg);
	}
	
	while (f->f_nexti < getstringsize((object *)f->f_code->co_code) &&
				!ctx->ctx_exception) {
#ifdef NDEBUG
		op = Nextbyte(f);
#else
		op = trace ? trace_nextop(f) : nextbyte(f);
#endif
		if (op >= HAVE_ARGUMENT)
			i = nextint(f);
		switch (op) {
		
		case DUP_TOP:
			v = TOP();
			INCREF(v);
			PUSH(v);
			break;
		
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
		
		case UNARY_POSITIVE:
			v = POP();
			u = pos(ctx, v);
			DECREF(v);
			PUSH(u);
			break;
		
		case UNARY_NEGATIVE:
			v = POP();
			u = neg(ctx, v);
			DECREF(v);
			PUSH(u);
			break;
		
		case UNARY_NOT:
			v = POP();
			u = not(ctx, v);
			DECREF(v);
			PUSH(u);
			break;
		
		case UNARY_CONVERT:
			v = POP();
			u = checkerror(ctx, reprobject(v));
			DECREF(v);
			PUSH(u);
			break;
		
		case UNARY_CALL:
			v = POP();
			if (is_classmethodobject(v) || is_funcobject(v))
				u = call_function(ctx, v, (object *)NULL);
			else
				u = call_builtin(ctx, v, (object *)NULL);
			DECREF(v);
			PUSH(u);
			break;
		
		case BINARY_MULTIPLY:
			w = POP();
			v = POP();
			u = mul(ctx, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case BINARY_DIVIDE:
			w = POP();
			v = POP();
			u = div(ctx, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case BINARY_MODULO:
			w = POP();
			v = POP();
			u = rem(ctx, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case BINARY_ADD:
			w = POP();
			v = POP();
			u = add(ctx, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case BINARY_SUBTRACT:
			w = POP();
			v = POP();
			u = sub(ctx, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case BINARY_SUBSCR:
			w = POP();
			v = POP();
			u = apply_subscript(ctx, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case BINARY_CALL:
			w = POP();
			v = POP();
			if (is_classmethodobject(v) || is_funcobject(v))
				u = call_function(ctx, v, w);
			else
				u = call_builtin(ctx, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case SLICE+0:
		case SLICE+1:
		case SLICE+2:
		case SLICE+3:
			op -= SLICE;
			if (op & 2)
				w = POP();
			else
				w = NULL;
			if (op & 1)
				v = POP();
			else
				v = NULL;
			u = POP();
			x = apply_slice(ctx, u, v, w);
			DECREF(u);
			XDECREF(v);
			XDECREF(w);
			PUSH(x);
			break;
		
		case STORE_SLICE+0:
		case STORE_SLICE+1:
		case STORE_SLICE+2:
		case STORE_SLICE+3:
			op -= SLICE;
			if (op & 2)
				w = POP();
			else
				w = NULL;
			if (op & 1)
				v = POP();
			else
				v = NULL;
			u = POP();
			x = POP();
			assign_slice(ctx, u, v, w, x); /* u[v:w] = x */
			DECREF(x);
			DECREF(u);
			XDECREF(v);
			XDECREF(w);
			break;
		
		case DELETE_SLICE+0:
		case DELETE_SLICE+1:
		case DELETE_SLICE+2:
		case DELETE_SLICE+3:
			op -= SLICE;
			if (op & 2)
				w = POP();
			else
				w = NULL;
			if (op & 1)
				v = POP();
			else
				v = NULL;
			u = POP();
			x = NULL;
			assign_slice(ctx, u, v, w, x); /* del u[v:w] */
			DECREF(u);
			XDECREF(v);
			XDECREF(w);
			break;
		
		case STORE_SUBSCR:
			w = POP();
			v = POP();
			u = POP();
			/* v[w] = u */
			assign_subscript(ctx, v, w, u);
			DECREF(u);
			DECREF(v);
			DECREF(w);
			break;
		
		case DELETE_SUBSCR:
			w = POP();
			v = POP();
			/* del v[w] */
			assign_subscript(ctx, v, w, (object *)NULL);
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
			raise_pseudo(ctx, BREAK_PSEUDO);
			break;
		
		case RAISE_EXCEPTION:
			v = POP();
			w = POP();
			if (!is_stringobject(w)) {
				DECREF(v);
				DECREF(v);
				type_error(ctx, "exceptions must be strings");
			}
			else {
				raise_exception(ctx, w, v);
			}
			break;
		
		case LOAD_LOCALS:
			v = ctx->ctx_locals;
			INCREF(v);
			PUSH(v);
			break;
		
		case RETURN_VALUE:
			v = POP();
			raise_pseudo(ctx, RETURN_PSEUDO);
			ctx->ctx_errval = v;
			break;
		
		case REQUIRE_ARGS:
			if (EMPTY())
				type_error(ctx,
					"function expects argument(s)");
			break;
		
		case REFUSE_ARGS:
			if (!EMPTY())
				type_error(ctx,
					"function expects no argument(s)");
			break;
		
		case BUILD_FUNCTION:
			v = POP();
			x = checkerror(ctx, newfuncobject(v, ctx->ctx_globals));
			DECREF(v);
			PUSH(x);
			break;
		
		case POP_BLOCK:
			(void) pop_block(f);
			break;
		
		case END_FINALLY:
			v = POP();
			w = POP();
			if (is_intobject(v)) {
				raise_pseudo(ctx, (int)getintvalue(v));
				DECREF(v);
				if (w == None)
					DECREF(w);
				else
					ctx->ctx_errval = w;
			}
			else if (is_stringobject(v))
				raise_exception(ctx, v, w);
			else if (v == None) {
				DECREF(v);
				DECREF(w);
			}
			else {
				sys_error(ctx, "'finally' pops bad exception");
			}
			break;
		
		case BUILD_CLASS:
			w = POP();
			v = POP();
			x = build_class(ctx, v, w);
			PUSH(x);
			DECREF(v);
			DECREF(w);
			break;
		
		case STORE_NAME:
			name = GETNAME(i);
			v = POP();
			if (dictinsert(ctx->ctx_locals, name, v) != 0)
				mem_error(ctx, "insert in symbol table");
			DECREF(v);
			break;
		
		case DELETE_NAME:
			name = GETNAME(i);
			if (dictremove(ctx->ctx_locals, name) != 0)
				name_error(ctx, name);
			break;
		
		case UNPACK_TUPLE:
			v = POP();
			if (!is_tupleobject(v)) {
				type_error(ctx, "unpack non-tuple");
			}
			else if (gettuplesize(v) != i) {
				error(ctx, "unpack tuple of wrong size");
			}
			else {
				for (; --i >= 0; ) {
					w = gettupleitem(v, i);
					INCREF(w);
					PUSH(w);
				}
			}
			DECREF(v);
			break;
		
		case UNPACK_LIST:
			v = POP();
			if (!is_listobject(v)) {
				type_error(ctx, "unpack non-list");
			}
			else if (getlistsize(v) != i) {
				error(ctx, "unpack tuple of wrong size");
			}
			else {
				for (; --i >= 0; ) {
					w = getlistitem(v, i);
					INCREF(w);
					PUSH(w);
				}
			}
			DECREF(v);
			break;
		
		case STORE_ATTR:
			name = GETNAME(i);
			v = POP();
			u = POP();
			/* v.name = u */
			if (v->ob_type->tp_setattr == NULL) {
				type_error(ctx, "object without writable attributes");
			}
			else {
				if ((*v->ob_type->tp_setattr)(v, name, u) != 0)
					puterrno(ctx);
			}
			DECREF(v);
			DECREF(u);
			break;
		
		case DELETE_ATTR:
			name = GETNAME(i);
			v = POP();
			/* del v.name */
			if (v->ob_type->tp_setattr == NULL) {
				type_error(ctx,
					"object without writable attributes");
			}
			else {
				if ((*v->ob_type->tp_setattr)
						(v, name, (object *)NULL) != 0)
					puterrno(ctx);
			}
			DECREF(v);
			break;
		
		case LOAD_CONST:
			v = GETCONST(i);
			INCREF(v);
			PUSH(v);
			break;
		
		case LOAD_NAME:
			name = GETNAME(i);
			v = dictlookup(ctx->ctx_locals, name);
			if (v == NULL) {
				v = dictlookup(ctx->ctx_globals, name);
				if (v == NULL)
					v = dictlookup(ctx->ctx_builtins, name);
			}
			if (v == NULL)
				name_error(ctx, name);
				/* XXX could optimize */
			else
				INCREF(v);
			PUSH(v);
			break;
		
		case BUILD_TUPLE:
			v = checkerror(ctx, newtupleobject(i));
			if (v != NULL) {
				for (; --i >= 0;) {
					w = POP();
					settupleitem(v, i, w);
				}
			}
			PUSH(v);
			break;
		
		case BUILD_LIST:
			v = checkerror(ctx, newlistobject(i));
			if (v != NULL) {
				for (; --i >= 0;) {
					w = POP();
					setlistitem(v, i, w);
				}
			}
			PUSH(v);
			break;
		
		case BUILD_MAP:
			v = checkerror(ctx, newdictobject());
			PUSH(v);
			break;
		
		case LOAD_ATTR:
			name = GETNAME(i);
			v = POP();
			if (v->ob_type->tp_getattr == NULL) {
				type_error(ctx, "attribute-less object");
				u = NULL;
			}
			else {
				u = checkerror(ctx,
					(*v->ob_type->tp_getattr)(v, name));
			}
			DECREF(v);
			PUSH(u);
			break;
		
		case COMPARE_OP:
			w = POP();
			v = POP();
			u = cmp_outcome(ctx, (enum cmp_op)i, v, w);
			DECREF(v);
			DECREF(w);
			PUSH(u);
			break;
		
		case IMPORT_NAME:
			name = GETNAME(i);
			u = import_module(ctx, name);
			if (u != NULL) {
				INCREF(u);
				PUSH(u);
			}
			break;
		
		case IMPORT_FROM:
			name = GETNAME(i);
			v = TOP();
			import_from(ctx, v, name);
			break;
		
		case JUMP_FORWARD:
			JUMPBY(i);
			break;
		
		case JUMP_IF_FALSE:
			if (!testbool(ctx, TOP()))
				JUMPBY(i);
			break;
		
		case JUMP_IF_TRUE:
			if (testbool(ctx, TOP()))
				JUMPBY(i);
			break;
		
		case JUMP_ABSOLUTE:
			JUMPTO(i);
			/* XXX Should check for interrupts more often? */
			if (intrcheck())
				intr_error(ctx);
			break;
		
		case FOR_LOOP:
			/* for v in s: ...
			   On entry: stack contains s, i.
			   On exit: stack contains s, i+1, s[i];
			   but if loop exhausted:
			   	s, i are popped, and we jump */
			w = POP(); /* Loop index */
			v = POP(); /* Sequence object */
			x = loop_subscript(ctx, v, w);
			if (x != NULL) {
				PUSH(v);
				u = checkerror(ctx,
					newintobject(getintvalue(w)+1));
				PUSH(u);
				DECREF(w);
				PUSH(x);
			}
			else {
				DECREF(v);
				DECREF(w);
				JUMPBY(i);
			}
			break;
		
		case SETUP_LOOP:
			setup_block(f, SETUP_LOOP, i);
			break;
		
		case SETUP_EXCEPT:
			setup_block(f, SETUP_EXCEPT, i);
			break;
		
		case SETUP_FINALLY:
			setup_block(f, SETUP_FINALLY, i);
			break;
		
		default:
			printf("opcode %d\n", op);
			sys_error(ctx, "eval_compiled: unknown opcode");
			break;
		
		}
		
		/* Unwind block stack if an exception occurred */
		
		while (ctx->ctx_exception && f->f_iblock > 0) {
			block *b = pop_block(f);
			if (b->b_type == SETUP_LOOP &&
					ctx->ctx_exception == BREAK_PSEUDO) {
				clear_exception(ctx);
				JUMPTO(b->b_handler);
				break;
			}
			else if (b->b_type == SETUP_FINALLY ||
				b->b_type == SETUP_EXCEPT &&
				ctx->ctx_exception == CATCHABLE_EXCEPTION) {
				v = ctx->ctx_errval;
				if (v == NULL)
					v = None;
				INCREF(v);
				PUSH(v);
				v = ctx->ctx_error;
				if (v == NULL)
					v = newintobject(ctx->ctx_exception);
				else
					INCREF(v);
				PUSH(v);
				clear_exception(ctx);
				JUMPTO(b->b_handler);
				break;
			}
		}
	}
	
	if (ctx->ctx_exception) {
		while (!EMPTY()) {
			v = POP();
			XDECREF(v);
		}
		v = NULL;
		if (ctx->ctx_exception == RETURN_PSEUDO) {
			if (needvalue) {
				v = ctx->ctx_errval;
				INCREF(v);
				clear_exception(ctx);
			}
			else {
				/* XXX Can detect this statically! */
				type_error(ctx, "unexpected return statement");
			}
		}
	}
	else {
		if (needvalue)
			v = POP();
		else
			v = NULL;
		if (!EMPTY()) {
			sys_error(ctx, "stack not cleaned up");
			XDECREF(v);
			while (!EMPTY()) {
				v = POP();
				XDECREF(v);
			}
			v = NULL;
		}
	}

#undef EMPTY
#undef FULL
#undef GETCONST
#undef GETNAME
#undef JUMPTO
#undef JUMPBY

#undef POP
#undef TOP
#undef PUSH
	
	DECREF(f);
	return v;

}

/* Provisional interface until everything is compilable */

#include "node.h"

static object *
eval_or_exec(ctx, n, arg, needvalue)
	context *ctx;
	node *n;
	object *arg;
	int needvalue;
{
	object *v;
	codeobject *co = compile(n);
	freenode(n); /* XXX A rather strange place to do this! */
	if (co == NULL) {
		puterrno(ctx);
		return NULL;
	}
	v = eval_compiled(ctx, co, arg, needvalue);
	DECREF(co);
	return v;
}

object *
eval_node(ctx, n)
	context *ctx;
	node *n;
{
	return eval_or_exec(ctx, n, (object *)NULL, 1/*needvalue*/);
}

void
exec_node(ctx, n)
	context *ctx;
	node *n;
{
	(void) eval_or_exec(ctx, n, (object *)NULL, 0/*needvalue*/);
}
