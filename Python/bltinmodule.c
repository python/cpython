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

/* Built-in functions */

#include "allobjects.h"

#include "node.h"
#include "graminit.h"
#include "errcode.h"
#include "sysmodule.h"
#include "bltinmodule.h"
#include "import.h"
#include "pythonrun.h"
#include "compile.h" /* For ceval.h */
#include "ceval.h"
#include "modsupport.h"

static object *
builtin_abs(self, v)
	object *self;
	object *v;
{
	number_methods *nm;
	if (v == NULL || (nm = v->ob_type->tp_as_number) == NULL) {
		err_setstr(TypeError, "abs() requires numeric argument");
		return NULL;
	}
	return (*nm->nb_absolute)(v);
}

static object *
builtin_apply(self, v)
	object *self;
	object *v;
{
	object *func, *args;
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 2) {
		err_setstr(TypeError, "apply() requires (func,args)");
		return NULL;
	}
	func = gettupleitem(v, 0);
	args = gettupleitem(v, 1);
	return call_object(func, args);
}

static object *
builtin_chr(self, v)
	object *self;
	object *v;
{
	long x;
	char s[1];
	if (v == NULL || !is_intobject(v)) {
		err_setstr(TypeError, "chr() requires int argument");
		return NULL;
	}
	x = getintvalue(v);
	if (x < 0 || x >= 256) {
		err_setstr(ValueError, "chr() arg not in range(256)");
		return NULL;
	}
	s[0] = x;
	return newsizedstringobject(s, 1);
}

static object *
builtin_dir(self, v)
	object *self;
	object *v;
{
	object *d;
	if (v == NULL) {
		d = getlocals();
		INCREF(d);
	}
	else {
		d = getattr(v, "__dict__");
		if (d == NULL) {
			err_setstr(TypeError,
				"dir() argument must have __dict__ attribute");
			return NULL;
		}
	}
	if (is_dictobject(d)) {
		v = getdictkeys(d);
		if (sortlist(v) != 0) {
			DECREF(v);
			v = NULL;
		}
	}
	else {
		v = newlistobject(0);
	}
	DECREF(d);
	return v;
}

static object *
builtin_divmod(self, args)
	object *self;
	object *args;
{
	object *v, *w, *x;
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2) {
		err_setstr(TypeError, "divmod() requires 2 arguments");
		return NULL;
	}
	v = gettupleitem(args, 0);
	w = gettupleitem(args, 1);
	if (v->ob_type->tp_as_number == NULL ||
				w->ob_type->tp_as_number == NULL) {
		err_setstr(TypeError, "divmod() requires numeric arguments");
		return NULL;
	}
	if (coerce(&v, &w) != 0)
		return NULL;
	x = (*v->ob_type->tp_as_number->nb_divmod)(v, w);
	DECREF(v);
	DECREF(w);
	return x;
}

static object *
exec_eval(v, start)
	object *v;
	int start;
{
	object *str = NULL, *globals = NULL, *locals = NULL;
	int n;
	if (v != NULL) {
		if (is_stringobject(v))
			str = v;
		else if (is_tupleobject(v) &&
				((n = gettuplesize(v)) == 2 || n == 3)) {
			str = gettupleitem(v, 0);
			globals = gettupleitem(v, 1);
			if (n == 3)
				locals = gettupleitem(v, 2);
		}
	}
	if (str == NULL || !is_stringobject(str) ||
			globals != NULL && !is_dictobject(globals) ||
			locals != NULL && !is_dictobject(locals)) {
		err_setstr(TypeError,
		    "exec/eval arguments must be string[,dict[,dict]]");
		return NULL;
	}
	return run_string(getstringvalue(str), start, globals, locals);
}

static object *
builtin_eval(self, v)
	object *self;
	object *v;
{
	return exec_eval(v, eval_input);
}

static object *
builtin_exec(self, v)
	object *self;
	object *v;
{
	return exec_eval(v, file_input);
}

static object *
builtin_float(self, v)
	object *self;
	object *v;
{
	if (v == NULL) {
		/* */
	}
	else if (is_intobject(v)) {
		long x = getintvalue(v);
		return newfloatobject((double)x);
	}
	else if (is_longobject(v)) {
		return newfloatobject(dgetlongvalue(v));
	}
	else if (is_floatobject(v)) {
		INCREF(v);
		return v;
	}
	err_setstr(TypeError, "float() argument must be int, long or float");
	return NULL;
}

static object *
builtin_getattr(self, v)
	object *self;
	object *v;
{
	object *name;
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 2 ||
		(name = gettupleitem(v, 1), !is_stringobject(name))) {
		err_setstr(TypeError,
			"getattr() arguments must be (object, string)");
		return NULL;
	}
	return getattr(gettupleitem(v, 0), getstringvalue(name));
}

static object *
builtin_setattr(self, v)
	object *self;
	object *v;
{
	object *name;
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 3 ||
		(name = gettupleitem(v, 1), !is_stringobject(name))) {
		err_setstr(TypeError,
		  "setattr() arguments must be (object, string, object)");
		return NULL;
	}
	if (setattr(gettupleitem(v, 0),
		    getstringvalue(name), gettupleitem(v, 2)) != 0)
		return NULL;
	INCREF(None);
	return None;
}

static object *
builtin_hex(self, v)
	object *self;
	object *v;
{
	if (v != NULL) {
		if (is_intobject(v)) {
			char buf[20];
			long x = getintvalue(v);
			if (x >= 0)
				sprintf(buf, "0x%lx", x);
			else
				sprintf(buf, "-0x%lx", -x);
			return newstringobject(buf);
		}
		if (is_longobject(v)) {
			return long_format(v, 16);
		}
	}
	err_setstr(TypeError, "hex() requires int/long argument");
	return NULL;
}

static object *
builtin_input(self, v)
	object *self;
	object *v;
{
	FILE *in = sysgetfile("stdin", stdin);
	FILE *out = sysgetfile("stdout", stdout);
	node *n;
	int err;
	object *m, *d;
	flushline();
	if (v != NULL) {
		if (printobject(v, out, PRINT_RAW) != 0)
			return NULL;
	}
	m = add_module("__main__");
	d = getmoduledict(m);
	return run_file(in, "<stdin>", expr_input, d, d);
}

static object *
builtin_int(self, v)
	object *self;
	object *v;
{
	if (v == NULL) {
		/* */
	}
	else if (is_intobject(v)) {
		INCREF(v);
		return v;
	}
	else if (is_longobject(v)) {
		long x;
		x = getlongvalue(v);
		if (err_occurred())
			return NULL;
		return newintobject(x);
	}
	else if (is_floatobject(v)) {
		double x = getfloatvalue(v);
		/* XXX should check for overflow */
		return newintobject((long)x);
	}
	err_setstr(TypeError, "int() argument must be int, long or float");
	return NULL;
}

static object *
builtin_len(self, v)
	object *self;
	object *v;
{
	long len;
	typeobject *tp;
	if (v == NULL) {
		err_setstr(TypeError, "len() without argument");
		return NULL;
	}
	tp = v->ob_type;
	if (tp->tp_as_sequence != NULL) {
		len = (*tp->tp_as_sequence->sq_length)(v);
	}
	else if (tp->tp_as_mapping != NULL) {
		len = (*tp->tp_as_mapping->mp_length)(v);
	}
	else {
		err_setstr(TypeError, "len() of unsized object");
		return NULL;
	}
	return newintobject(len);
}

static object *
builtin_long(self, v)
	object *self;
	object *v;
{
	if (v == NULL) {
		/* */
	}
	else if (is_intobject(v)) {
		return newlongobject(getintvalue(v));
	}
	else if (is_longobject(v)) {
		INCREF(v);
		return v;
	}
	else if (is_floatobject(v)) {
		double x = getfloatvalue(v);
		return dnewlongobject(x);
	}
	err_setstr(TypeError, "long() argument must be int, long or float");
	return NULL;
}

static object *
min_max(v, sign)
	object *v;
	int sign;
{
	int i, n, cmp;
	object *w, *x;
	sequence_methods *sq;
	if (v == NULL) {
		err_setstr(TypeError, "min() or max() without argument");
		return NULL;
	}
	sq = v->ob_type->tp_as_sequence;
	if (sq == NULL) {
		err_setstr(TypeError, "min() or max() of non-sequence");
		return NULL;
	}
	n = (*sq->sq_length)(v);
	if (n == 0) {
		err_setstr(ValueError, "min() or max() of empty sequence");
		return NULL;
	}
	w = (*sq->sq_item)(v, 0); /* Implies INCREF */
	for (i = 1; i < n; i++) {
		x = (*sq->sq_item)(v, i); /* Implies INCREF */
		cmp = cmpobject(x, w);
		if (cmp * sign > 0) {
			DECREF(w);
			w = x;
		}
		else
			DECREF(x);
	}
	return w;
}

static object *
builtin_min(self, v)
	object *self;
	object *v;
{
	return min_max(v, -1);
}

static object *
builtin_max(self, v)
	object *self;
	object *v;
{
	return min_max(v, 1);
}

static object *
builtin_oct(self, v)
	object *self;
	object *v;
{
	if (v != NULL) {
		if (is_intobject(v)) {
			char buf[20];
			long x = getintvalue(v);
			if (x >= 0)
				sprintf(buf, "0%lo", x);
			else
				sprintf(buf, "-0%lo", -x);
			return newstringobject(buf);
		}
		if (is_longobject(v)) {
			return long_format(v, 8);
		}
	}
	err_setstr(TypeError, "oct() requires int/long argument");
	return NULL;
}

static object *
builtin_open(self, v)
	object *self;
	object *v;
{
	object *name, *mode;
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 2 ||
		!is_stringobject(name = gettupleitem(v, 0)) ||
		!is_stringobject(mode = gettupleitem(v, 1))) {
		err_setstr(TypeError, "open() requires 2 string arguments");
		return NULL;
	}
	v = newfileobject(getstringvalue(name), getstringvalue(mode));
	return v;
}

static object *
builtin_ord(self, v)
	object *self;
	object *v;
{
	if (v == NULL || !is_stringobject(v)) {
		err_setstr(TypeError, "ord() must have string argument");
		return NULL;
	}
	if (getstringsize(v) != 1) {
		err_setstr(ValueError, "ord() arg must have length 1");
		return NULL;
	}
	return newintobject((long)(getstringvalue(v)[0] & 0xff));
}

static object *
builtin_pow(self, args)
	object *self;
	object *args;
{
	object *v, *w, *x;
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2) {
		err_setstr(TypeError, "pow() requires 2 arguments");
		return NULL;
	}
	v = gettupleitem(args, 0);
	w = gettupleitem(args, 1);
	if (v->ob_type->tp_as_number == NULL ||
				w->ob_type->tp_as_number == NULL) {
		err_setstr(TypeError, "pow() requires numeric arguments");
		return NULL;
	}
	if (coerce(&v, &w) != 0)
		return NULL;
	x = (*v->ob_type->tp_as_number->nb_power)(v, w);
	DECREF(v);
	DECREF(w);
	return x;
}

static object *
builtin_range(self, v)
	object *self;
	object *v;
{
	static char *errmsg = "range() requires 1-3 int arguments";
	int i, n;
	long ilow, ihigh, istep;
	if (v != NULL && is_intobject(v)) {
		ilow = 0; ihigh = getintvalue(v); istep = 1;
	}
	else if (v == NULL || !is_tupleobject(v)) {
		err_setstr(TypeError, errmsg);
		return NULL;
	}
	else {
		n = gettuplesize(v);
		if (n < 1 || n > 3) {
			err_setstr(TypeError, errmsg);
			return NULL;
		}
		for (i = 0; i < n; i++) {
			if (!is_intobject(gettupleitem(v, i))) {
				err_setstr(TypeError, errmsg);
				return NULL;
			}
		}
		if (n == 3) {
			istep = getintvalue(gettupleitem(v, 2));
			--n;
		}
		else
			istep = 1;
		ihigh = getintvalue(gettupleitem(v, --n));
		if (n > 0)
			ilow = getintvalue(gettupleitem(v, 0));
		else
			ilow = 0;
	}
	if (istep == 0) {
		err_setstr(ValueError, "zero step for range()");
		return NULL;
	}
	/* XXX ought to check overflow of subtraction */
	if (istep > 0)
		n = (ihigh - ilow + istep - 1) / istep;
	else
		n = (ihigh - ilow + istep + 1) / istep;
	if (n < 0)
		n = 0;
	v = newlistobject(n);
	if (v == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		object *w = newintobject(ilow);
		if (w == NULL) {
			DECREF(v);
			return NULL;
		}
		setlistitem(v, i, w);
		ilow += istep;
	}
	return v;
}

static object *
builtin_raw_input(self, v)
	object *self;
	object *v;
{
	FILE *out = sysgetfile("stdout", stdout);
	flushline();
	if (v != NULL) {
		if (printobject(v, out, PRINT_RAW) != 0)
			return NULL;
	}
	return filegetline(sysget("stdin"), -1);
}

static object *
builtin_reload(self, v)
	object *self;
	object *v;
{
	return reload_module(v);
}

static object *
builtin_type(self, v)
	object *self;
	object *v;
{
	if (v == NULL) {
		err_setstr(TypeError, "type() requres an argument");
		return NULL;
	}
	v = (object *)v->ob_type;
	INCREF(v);
	return v;
}

static struct methodlist builtin_methods[] = {
	{"abs",		builtin_abs},
	{"apply",	builtin_apply},
	{"chr",		builtin_chr},
	{"dir",		builtin_dir},
	{"divmod",	builtin_divmod},
	{"eval",	builtin_eval},
	{"exec",	builtin_exec},
	{"float",	builtin_float},
	{"getattr",	builtin_getattr},
	{"hex",		builtin_hex},
	{"input",	builtin_input},
	{"int",		builtin_int},
	{"len",		builtin_len},
	{"long",	builtin_long},
	{"max",		builtin_max},
	{"min",		builtin_min},
	{"oct",		builtin_oct},
	{"open",	builtin_open},
	{"ord",		builtin_ord},
	{"pow",		builtin_pow},
	{"range",	builtin_range},
	{"raw_input",	builtin_raw_input},
	{"reload",	builtin_reload},
	{"setattr",	builtin_setattr},
	{"type",	builtin_type},
	{NULL,		NULL},
};

static object *builtin_dict;

object *
getbuiltin(name)
	object *name;
{
	return dict2lookup(builtin_dict, name);
}

/* Predefined exceptions */

object *AttributeError;
object *EOFError;
object *IOError;
object *ImportError;
object *IndexError;
object *KeyError;
object *KeyboardInterrupt;
object *MemoryError;
object *NameError;
object *OverflowError;
object *RuntimeError;
object *SyntaxError;
object *SystemError;
object *SystemExit;
object *TypeError;
object *ValueError;
object *ZeroDivisionError;

static object *
newstdexception(name)
	char *name;
{
	object *v = newstringobject(name);
	if (v == NULL || dictinsert(builtin_dict, name, v) != 0)
		fatal("no mem for new standard exception");
	return v;
}

static void
initerrors()
{
	AttributeError = newstdexception("AttributeError");
	EOFError = newstdexception("EOFError");
	IOError = newstdexception("IOError");
	ImportError = newstdexception("ImportError");
	IndexError = newstdexception("IndexError");
	KeyError = newstdexception("KeyError");
	KeyboardInterrupt = newstdexception("KeyboardInterrupt");
	MemoryError = newstdexception("MemoryError");
	NameError = newstdexception("NameError");
	OverflowError = newstdexception("OverflowError");
	RuntimeError = newstdexception("RuntimeError");
	SyntaxError = newstdexception("SyntaxError");
	SystemError = newstdexception("SystemError");
	SystemExit = newstdexception("SystemExit");
	TypeError = newstdexception("TypeError");
	ValueError = newstdexception("ValueError");
	ZeroDivisionError = newstdexception("ZeroDivisionError");
}

void
initbuiltin()
{
	object *m;
	m = initmodule("builtin", builtin_methods);
	builtin_dict = getmoduledict(m);
	INCREF(builtin_dict);
	initerrors();
	(void) dictinsert(builtin_dict, "None", None);
}

/* Coerce two numeric types to the "larger" one.
   Increment the reference count on each argument.
   Return -1 and raise an exception if no coercion is possible
   (and then no reference count is incremented).
   XXX This should be distributed over the various numeric types,
   XXX but for now I don't see how to implement that.
   XXX So, for now, if you add a new numeric type,
   XXX you must add to this function as well. */

int
coerce(pv, pw)
	object **pv, **pw;
{
	register object *v = *pv;
	register object *w = *pw;
	if (v->ob_type == w->ob_type) {
		INCREF(v);
		INCREF(w);
		return 0;
	}
	if (v->ob_type->tp_as_number == NULL ||
					w->ob_type->tp_as_number == NULL) {
		err_setstr(TypeError, "mixing number and non-number");
		return -1;
	}
	if (is_floatobject(v) || is_floatobject(w)) {
		v = builtin_float((object *)0, v);
		w = builtin_float((object *)0, w);
	}
	else if (is_longobject(v) || is_longobject(w)) {
		v = builtin_long((object *)0, v);
		w = builtin_long((object *)0, w);
	}
	else {
		err_setstr(TypeError, "can't coerce numeric types?!?!?");
		return -1;
	}
	if (v == NULL || w == NULL) {
		XDECREF(v);
		XDECREF(w);
		return -1;
	}
	*pv = v;
	*pw = w;
	return 0;
}
