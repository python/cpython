/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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
#include "sysmodule.h"
#include "bltinmodule.h"
#include "import.h"
#include "pythonrun.h"
#include "ceval.h"
#include "modsupport.h"
#include "compile.h"
#include "eval.h"

/* Forward */
static object *filterstring PROTO((object *, object *));
static object *filtertuple  PROTO((object *, object *));

static object *
builtin___import__(self, args)
	object *self;
	object *args;
{
	char *name;
	object *m;

	if (!newgetargs(args, "s:__import__", &name))
		return NULL;
	m = import_module(name);
	XINCREF(m);

	return m;
}


static object *
builtin_abs(self, args)
	object *self;
	object *args;
{
	object *v;
	number_methods *nm;

	if (!newgetargs(args, "O:abs", &v))
		return NULL;
	if ((nm = v->ob_type->tp_as_number) == NULL) {
		err_setstr(TypeError, "abs() requires numeric argument");
		return NULL;
	}
	return (*nm->nb_absolute)(v);
}

static object *
builtin_apply(self, args)
	object *self;
	object *args;
{
	object *func, *arglist;

	if (!newgetargs(args, "OO:apply", &func, &arglist))
		return NULL;
	if (!is_tupleobject(arglist)) {
		err_setstr(TypeError, "apply() 2nd argument must be tuple");
		return NULL;
	}
	return call_object(func, arglist);
}

static int
callable(x)
	object *x;
{
	if (x == NULL)
		return 0;
	if (x->ob_type->tp_call != NULL ||
	    is_funcobject(x) ||
	    is_instancemethodobject(x) ||
	    is_methodobject(x) ||
	    is_classobject(x))
		return 1;
	if (is_instanceobject(x)) {
		object *call = getattr(x, "__call__");
		if (call == NULL) {
			err_clear();
			return 0;
		}
		/* Could test recursively but don't, for fear of endless
		   recursion if some joker sets self.__call__ = self */
		DECREF(call);
		return 1;
	}
	return 0;
}

static object *
builtin_callable(self, args)
	object *self;
	object *args;
{
	object *v;

	if (!newgetargs(args, "O:callable", &v))
		return NULL;
	return newintobject((long)callable(v));
}

static object *
builtin_filter(self, args)
	object *self;
	object *args;
{
	object *func, *seq, *result;
	sequence_methods *sqf;
	int len;
	register int i, j;

	if (!newgetargs(args, "OO:filter", &func, &seq))
		return NULL;

	if (is_stringobject(seq)) {
		object *r = filterstring(func, seq);
		return r;
	}

	if (is_tupleobject(seq)) {
		object *r = filtertuple(func, seq);
		return r;
	}

	if ((sqf = seq->ob_type->tp_as_sequence) == NULL) {
		err_setstr(TypeError,
			   "argument 2 to filter() must be a sequence type");
		goto Fail_2;
	}

	if ((len = (*sqf->sq_length)(seq)) < 0)
		goto Fail_2;

	if (is_listobject(seq) && seq->ob_refcnt == 1) {
		INCREF(seq);
		result = seq;
	}
	else {
		if ((result = newlistobject(len)) == NULL)
			goto Fail_2;
	}

	for (i = j = 0; ; ++i) {
		object *item, *good;
		int ok;

		if ((item = (*sqf->sq_item)(seq, i)) == NULL) {
			if (i < len)
				goto Fail_1;
			if (err_occurred() == IndexError) {
				err_clear();
				break;
			}
			goto Fail_1;
		}

		if (func == None) {
			good = item;
			INCREF(good);
		}
		else {
			object *arg = mkvalue("(O)", item);
			if (arg == NULL)
				goto Fail_1;
			good = call_object(func, arg);
			DECREF(arg);
			if (good == NULL) {
				DECREF(item);
				goto Fail_1;
			}
		}
		ok = testbool(good);
		DECREF(good);
		if (ok) {
			if (j < len) {
				if (setlistitem(result, j++, item) < 0)
					goto Fail_1;
			}
			else {
				j++;
				if (addlistitem(result, item) < 0)
					goto Fail_1;
			}
		} else {
			DECREF(item);
		}
	}


	if (j < len && setlistslice(result, j, len, NULL) < 0)
		goto Fail_1;

	return result;

Fail_1:
	DECREF(result);
Fail_2:
	return NULL;
}

static object *
builtin_chr(self, args)
	object *self;
	object *args;
{
	long x;
	char s[1];

	if (!newgetargs(args, "l:chr", &x))
		return NULL;
	if (x < 0 || x >= 256) {
		err_setstr(ValueError, "chr() arg not in range(256)");
		return NULL;
	}
	s[0] = x;
	return newsizedstringobject(s, 1);
}

static object *
builtin_cmp(self, args)
	object *self;
	object *args;
{
	object *a, *b;

	if (!newgetargs(args, "OO:cmp", &a, &b))
		return NULL;
	return newintobject((long)cmpobject(a, b));
}

static object *
builtin_coerce(self, args)
	object *self;
	object *args;
{
	object *v, *w;
	object *res;

	if (!newgetargs(args, "OO:coerce", &v, &w))
		return NULL;
	if (coerce(&v, &w) < 0)
		return NULL;
	res = mkvalue("(OO)", v, w);
	DECREF(v);
	DECREF(w);
	return res;
}

static object *
builtin_compile(self, args)
	object *self;
	object *args;
{
	char *str;
	char *filename;
	char *startstr;
	int start;

	if (!newgetargs(args, "sss:compile", &str, &filename, &startstr))
		return NULL;
	if (strcmp(startstr, "exec") == 0)
		start = file_input;
	else if (strcmp(startstr, "eval") == 0)
		start = eval_input;
	else {
		err_setstr(ValueError,
			   "compile() mode must be 'exec' or 'eval'");
		return NULL;
	}
	return compile_string(str, filename, start);
}

static object *
builtin_dir(self, args)
	object *self;
	object *args;
{
	object *v = NULL;
	object *d;

	if (!newgetargs(args, "|O:dir", &v))
		return NULL;
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
do_divmod(v, w)
	object *v, *w;
{
	object *res;

	if (is_instanceobject(v) || is_instanceobject(w))
		return instancebinop(v, w, "__divmod__", "__rdivmod__",
				     do_divmod);
	if (v->ob_type->tp_as_number == NULL ||
				w->ob_type->tp_as_number == NULL) {
		err_setstr(TypeError,
		    "divmod() requires numeric or class instance arguments");
		return NULL;
	}
	if (coerce(&v, &w) != 0)
		return NULL;
	res = (*v->ob_type->tp_as_number->nb_divmod)(v, w);
	DECREF(v);
	DECREF(w);
	return res;
}

static object *
builtin_divmod(self, args)
	object *self;
	object *args;
{
	object *v, *w;

	if (!newgetargs(args, "OO:divmod", &v, &w))
		return NULL;
	return do_divmod(v, w);
}

static object *
builtin_eval(self, args)
	object *self;
	object *args;
{
	object *cmd;
	object *globals = None, *locals = None;
	char *str;

	if (!newgetargs(args, "O|O!O!:eval",
			&cmd,
			&Mappingtype, &globals,
			&Mappingtype, &locals))
		return NULL;
	if (globals == None) {
		globals = getglobals();
		if (locals == None)
			locals = getlocals();
	}
	else if (locals == None)
		locals = globals;
	if (dictlookup(globals, "__builtins__") == NULL) {
		if (dictinsert(globals, "__builtins__", getbuiltins()) != 0)
			return NULL;
	}
	if (is_codeobject(cmd))
		return eval_code((codeobject *) cmd, globals, locals,
				 (object *)NULL, (object *)NULL);
	if (!is_stringobject(cmd)) {
		err_setstr(TypeError,
			   "eval() argument 1 must be string or code object");
		return NULL;
	}
	str = getstringvalue(cmd);
	if (strlen(str) != getstringsize(cmd)) {
		err_setstr(ValueError,
			   "embedded '\\0' in string arg");
		return NULL;
	}
	while (*str == ' ' || *str == '\t')
		str++;
	return run_string(str, eval_input, globals, locals);
}

static object *
builtin_execfile(self, args)
	object *self;
	object *args;
{
	char *filename;
	object *globals = None, *locals = None;
	object *res;
	FILE* fp;
	char *s;
	int n;

	if (!newgetargs(args, "s|O!O!:execfile",
			&filename,
			&Mappingtype, &globals,
			&Mappingtype, &locals))
		return NULL;
	if (globals == None) {
		globals = getglobals();
		if (locals == None)
			locals = getlocals();
	}
	else if (locals == None)
		locals = globals;
	if (dictlookup(globals, "__builtins__") == NULL) {
		if (dictinsert(globals, "__builtins__", getbuiltins()) != 0)
			return NULL;
	}
	BGN_SAVE
	fp = fopen(filename, "r");
	END_SAVE
	if (fp == NULL) {
		err_errno(IOError);
		return NULL;
	}
	res = run_file(fp, filename, file_input, globals, locals);
	BGN_SAVE
	fclose(fp);
	END_SAVE
	return res;
}

static object *
builtin_float(self, args)
	object *self;
	object *args;
{
	object *v;
	number_methods *nb;

	if (!newgetargs(args, "O:float", &v))
		return NULL;
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_float == NULL) {
		err_setstr(TypeError,
			   "float() argument can't be converted to float");
		return NULL;
	}
	return (*nb->nb_float)(v);
}

static object *
builtin_getattr(self, args)
	object *self;
	object *args;
{
	object *v;
	object *name;

	if (!newgetargs(args, "OS:getattr", &v, &name))
		return NULL;
	return getattro(v, name);
}

static object *
builtin_hasattr(self, args)
	object *self;
	object *args;
{
	object *v;
	object *name;

	if (!newgetargs(args, "OS:hasattr", &v, &name))
		return NULL;
	v = getattro(v, name);
	if (v == NULL) {
		err_clear();
		return newintobject(0L);
	}
	DECREF(v);
	return newintobject(1L);
}

static object *
builtin_id(self, args)
	object *self;
	object *args;
{
	object *v;

	if (!newgetargs(args, "O:id", &v))
		return NULL;
	return newintobject((long)v);
}

static object *
builtin_map(self, args)
	object *self;
	object *args;
{
	typedef struct {
		object *seq;
		sequence_methods *sqf;
		int len;
	} sequence;

	object *func, *result;
	sequence *seqs = NULL, *sqp;
	int n, len;
	register int i, j;

	n = gettuplesize(args);
	if (n < 2) {
		err_setstr(TypeError, "map() requires at least two args");
		return NULL;
	}

	func = gettupleitem(args, 0);
	n--;

	if ((seqs = NEW(sequence, n)) == NULL) {
		err_nomem();
		goto Fail_2;
	}

	for (len = 0, i = 0, sqp = seqs; i < n; ++i, ++sqp) {
		int curlen;
	
		if ((sqp->seq = gettupleitem(args, i + 1)) == NULL)
			goto Fail_2;

		if (! (sqp->sqf = sqp->seq->ob_type->tp_as_sequence)) {
			static char errmsg[] =
			    "argument %d to map() must be a sequence object";
			char errbuf[sizeof(errmsg) + 3];

			sprintf(errbuf, errmsg, i+2);
			err_setstr(TypeError, errbuf);
			goto Fail_2;
		}

		if ((curlen = sqp->len = (*sqp->sqf->sq_length)(sqp->seq)) < 0)
			goto Fail_2;

		if (curlen > len)
			len = curlen;
	}

	if ((result = (object *) newlistobject(len)) == NULL)
		goto Fail_2;

	/* XXX Special case map(None, single_list) could be more efficient */
	for (i = 0; ; ++i) {
		object *arglist, *item, *value;
		int any = 0;

		if (func == None && n == 1)
			arglist = NULL;
		else {
			if ((arglist = newtupleobject(n)) == NULL)
				goto Fail_1;
		}

		for (j = 0, sqp = seqs; j < n; ++j, ++sqp) {
			if (sqp->len < 0) {
				INCREF(None);
				item = None;
			}
			else {
				item = (*sqp->sqf->sq_item)(sqp->seq, i);
				if (item == NULL) {
					if (i < sqp->len)
						goto Fail_0;
					if (err_occurred() == IndexError) {
						err_clear();
						INCREF(None);
						item = None;
						sqp->len = -1;
					}
					else {
						goto Fail_0;
					}
				}
				else
					any = 1;

			}
			if (!arglist)
				break;
			if (settupleitem(arglist, j, item) < 0) {
				DECREF(item);
				goto Fail_0;
			}
			continue;

		Fail_0:
			XDECREF(arglist);
			goto Fail_1;
		}

		if (!arglist)
			arglist = item;

		if (!any) {
			DECREF(arglist);
			break;
		}

		if (func == None)
			value = arglist;
		else {
			value = call_object(func, arglist);
			DECREF(arglist);
			if (value == NULL)
				goto Fail_1;
		}
		if (i >= len) {
			if (addlistitem(result, value) < 0)
				goto Fail_1;
		}
		else {
			if (setlistitem(result, i, value) < 0)
				goto Fail_1;
		}
	}

	DEL(seqs);
	return result;

Fail_1:
	DECREF(result);
Fail_2:
	if (seqs) DEL(seqs);
	return NULL;
}

static object *
builtin_setattr(self, args)
	object *self;
	object *args;
{
	object *v;
	object *name;
	object *value;

	if (!newgetargs(args, "OSO:setattr", &v, &name, &value))
		return NULL;
	if (setattro(v, name, value) != 0)
		return NULL;
	INCREF(None);
	return None;
}

static object *
builtin_delattr(self, args)
	object *self;
	object *args;
{
	object *v;
	object *name;

	if (!newgetargs(args, "OS:delattr", &v, &name))
		return NULL;
	if (setattro(v, name, (object *)NULL) != 0)
		return NULL;
	INCREF(None);
	return None;
}

static object *
builtin_hash(self, args)
	object *self;
	object *args;
{
	object *v;
	long x;

	if (!newgetargs(args, "O:hash", &v))
		return NULL;
	x = hashobject(v);
	if (x == -1)
		return NULL;
	return newintobject(x);
}

static object *
builtin_hex(self, args)
	object *self;
	object *args;
{
	object *v;
	number_methods *nb;

	if (!newgetargs(args, "O:hex", &v))
		return NULL;
	
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_hex == NULL) {
		err_setstr(TypeError,
			   "hex() argument can't be converted to hex");
		return NULL;
	}
	return (*nb->nb_hex)(v);
}

static object *builtin_raw_input PROTO((object *, object *));

static object *
builtin_input(self, args)
	object *self;
	object *args;
{
	object *line;
	char *str;
	object *res;
	object *globals, *locals;

	line = builtin_raw_input(self, args);
	if (line == NULL)
		return line;
	if (!getargs(line, "s;embedded '\\0' in input line", &str))
		return NULL;
	while (*str == ' ' || *str == '\t')
			str++;
	globals = getglobals();
	locals = getlocals();
	if (dictlookup(globals, "__builtins__") == NULL) {
		if (dictinsert(globals, "__builtins__", getbuiltins()) != 0)
			return NULL;
	}
	res = run_string(str, eval_input, globals, locals);
	DECREF(line);
	return res;
}

static object *
builtin_int(self, args)
	object *self;
	object *args;
{
	object *v;
	number_methods *nb;

	if (!newgetargs(args, "O:int", &v))
		return NULL;
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_int == NULL) {
		err_setstr(TypeError,
			   "int() argument can't be converted to int");
		return NULL;
	}
	return (*nb->nb_int)(v);
}

static object *
builtin_len(self, args)
	object *self;
	object *args;
{
	object *v;
	long len;
	typeobject *tp;

	if (!newgetargs(args, "O:len", &v))
		return NULL;
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
	if (len < 0)
		return NULL;
	else
		return newintobject(len);
}

static object *
builtin_long(self, args)
	object *self;
	object *args;
{
	object *v;
	number_methods *nb;
	
	if (!newgetargs(args, "O:long", &v))
		return NULL;
	if ((nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_long == NULL) {
		err_setstr(TypeError,
			   "long() argument can't be converted to long");
		return NULL;
	}
	return (*nb->nb_long)(v);
}

static object *
min_max(args, sign)
	object *args;
	int sign;
{
	int i;
	object *v, *w, *x;
	sequence_methods *sq;

	if (gettuplesize(args) > 1)
		v = args;
	else if (!newgetargs(args, "O:min/max", &v))
		return NULL;
	sq = v->ob_type->tp_as_sequence;
	if (sq == NULL) {
		err_setstr(TypeError, "min() or max() of non-sequence");
		return NULL;
	}
	w = NULL;
	for (i = 0; ; i++) {
		x = (*sq->sq_item)(v, i); /* Implies INCREF */
		if (x == NULL) {
			if (err_occurred() == IndexError) {
				err_clear();
				break;
			}
			XDECREF(w);
			return NULL;
		}
		if (w == NULL)
			w = x;
		else {
			if (cmpobject(x, w) * sign > 0) {
				DECREF(w);
				w = x;
			}
			else
				DECREF(x);
		}
	}
	if (w == NULL)
		err_setstr(ValueError, "min() or max() of empty sequence");
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
builtin_oct(self, args)
	object *self;
	object *args;
{
	object *v;
	number_methods *nb;

	if (!newgetargs(args, "O:oct", &v))
		return NULL;
	if (v == NULL || (nb = v->ob_type->tp_as_number) == NULL ||
	    nb->nb_oct == NULL) {
		err_setstr(TypeError,
			   "oct() argument can't be converted to oct");
		return NULL;
	}
	return (*nb->nb_oct)(v);
}

static object *
builtin_open(self, args)
	object *self;
	object *args;
{
	char *name;
	char *mode = "r";
	int bufsize = -1;
	object *f;

	if (!newgetargs(args, "s|si:open", &name, &mode, &bufsize))
		return NULL;
	f = newfileobject(name, mode);
	if (f != NULL)
		setfilebufsize(f, bufsize);
	return f;
}

static object *
builtin_ord(self, args)
	object *self;
	object *args;
{
	char c;

	if (!newgetargs(args, "c:ord", &c))
		return NULL;
	return newintobject((long)(c & 0xff));
}

static object *
do_pow(v, w)
	object *v, *w;
{
	object *res;
	if (is_instanceobject(v) || is_instanceobject(w))
		return instancebinop(v, w, "__pow__", "__rpow__", do_pow);
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
builtin_pow(self, args)
	object *self;
	object *args;
{
	object *v, *w, *z = None, *res;
	object *v1, *z1, *w2, *z2;

	if (!newgetargs(args, "OO|O:pow", &v, &w, &z))
		return NULL;
	if (z == None)
		return do_pow(v, w);
	/* XXX The ternary version doesn't do class instance coercions */
	if (is_instanceobject(v))
		return v->ob_type->tp_as_number->nb_power(v, w, z);
	if (v->ob_type->tp_as_number == NULL ||
	    z->ob_type->tp_as_number == NULL ||
	    w->ob_type->tp_as_number == NULL) {
		err_setstr(TypeError, "pow() requires numeric arguments");
		return NULL;
	}
	if (coerce(&v, &w) != 0)
		return NULL;
	res = NULL;
	v1 = v;
	z1 = z;
	if (coerce(&v1, &z1) != 0)
		goto error2;
	w2 = w;
	z2 = z1;
 	if (coerce(&w2, &z2) != 0)
		goto error1;
	res = (*v1->ob_type->tp_as_number->nb_power)(v1, w2, z2);
	DECREF(w2);
	DECREF(z2);
 error1:
	DECREF(v1);
	DECREF(z1);
 error2:
	DECREF(v);
	DECREF(w);
	return res;
}

static object *
builtin_range(self, args)
	object *self;
	object *args;
{
	long ilow = 0, ihigh = 0, istep = 1;
	int i, n;
	object *v;

	if (gettuplesize(args) <= 1) {
		if (!newgetargs(args,
				"l;range() requires 1-3 int arguments",
				&ihigh))
			return NULL;
	}
	else {
		if (!newgetargs(args,
				"ll|l;range() requires 1-3 int arguments",
				&ilow, &ihigh, &istep))
			return NULL;
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
builtin_xrange(self, args)
	object *self;
	object *args;
{
	long ilow = 0, ihigh = 0, istep = 1;
	long n;
	object *v;

	if (gettuplesize(args) <= 1) {
		if (!newgetargs(args,
				"l;xrange() requires 1-3 int arguments",
				&ihigh))
			return NULL;
	}
	else {
		if (!newgetargs(args,
				"ll|l;xrange() requires 1-3 int arguments",
				&ilow, &ihigh, &istep))
			return NULL;
	}
	if (istep == 0) {
		err_setstr(ValueError, "zero step for xrange()");
		return NULL;
	}
	/* XXX ought to check overflow of subtraction */
	if (istep > 0)
		n = (ihigh - ilow + istep - 1) / istep;
	else
		n = (ihigh - ilow + istep + 1) / istep;
	if (n < 0)
		n = 0;
	return newrangeobject(ilow, n, istep, 1);
}

static object *
builtin_raw_input(self, args)
	object *self;
	object *args;
{
	object *v = NULL;
	object *f;

	if (!newgetargs(args, "|O:[raw_]input", &v))
		return NULL;
	if (v != NULL) {
		f = sysget("stdout");
		if (f == NULL) {
			err_setstr(RuntimeError, "lost sys.stdout");
			return NULL;
		}
		flushline();
		if (writeobject(v, f, PRINT_RAW) != 0)
			return NULL;
	}
	f = sysget("stdin");
	if (f == NULL) {
		err_setstr(RuntimeError, "lost sys.stdin");
		return NULL;
	}
	return filegetline(f, -1);
}

static object *
builtin_reduce(self, args)
	object *self;
	object *args;
{
	object *seq, *func, *result = NULL;
	sequence_methods *sqf;
	register int i;

	if (!newgetargs(args, "OO|O:reduce", &func, &seq, &result))
		return NULL;
	if (result != NULL)
		INCREF(result);

	if ((sqf = seq->ob_type->tp_as_sequence) == NULL) {
		err_setstr(TypeError,
		    "2nd argument to reduce() must be a sequence object");
		return NULL;
	}

	if ((args = newtupleobject(2)) == NULL)
		goto Fail;

	for (i = 0; ; ++i) {
		object *op2;

		if (args->ob_refcnt > 1) {
			DECREF(args);
			if ((args = newtupleobject(2)) == NULL)
				goto Fail;
		}

		if ((op2 = (*sqf->sq_item)(seq, i)) == NULL) {
			if (err_occurred() == IndexError) {
				err_clear();
				break;
			}
			goto Fail;
		}

		if (result == NULL)
			result = op2;
		else {
			settupleitem(args, 0, result);
			settupleitem(args, 1, op2);
			if ((result = call_object(func, args)) == NULL)
				goto Fail;
		}
	}

	DECREF(args);

	if (result == NULL)
		err_setstr(TypeError,
			   "reduce of empty sequence with no initial value");

	return result;

Fail:
	XDECREF(args);
	XDECREF(result);
	return NULL;
}

static object *
builtin_reload(self, args)
	object *self;
	object *args;
{
	object *v;

	if (!newgetargs(args, "O:reload", &v))
		return NULL;
	return reload_module(v);
}

static object *
builtin_repr(self, args)
	object *self;
	object *args;
{
	object *v;

	if (!newgetargs(args, "O:repr", &v))
		return NULL;
	return reprobject(v);
}

static object *
builtin_round(self, args)
	object *self;
	object *args;
{
	extern double floor PROTO((double));
	extern double ceil PROTO((double));
	double x;
	double f;
	int ndigits = 0;
	int i;

	if (!newgetargs(args, "d|i:round", &x, &ndigits))
			return NULL;
	f = 1.0;
	for (i = ndigits; --i >= 0; )
		f = f*10.0;
	for (i = ndigits; ++i <= 0; )
		f = f*0.1;
	if (x >= 0.0)
		return newfloatobject(floor(x*f + 0.5) / f);
	else
		return newfloatobject(ceil(x*f - 0.5) / f);
}

static object *
builtin_str(self, args)
	object *self;
	object *args;
{
	object *v;

	if (!newgetargs(args, "O:str", &v))
		return NULL;
	return strobject(v);
}

static object *
builtin_tuple(self, args)
	object *self;
	object *args;
{
	object *v;
	sequence_methods *sqf;

	if (!newgetargs(args, "O:tuple", &v))
		return NULL;
	if (is_tupleobject(v)) {
		INCREF(v);
		return v;
	}
	if (is_listobject(v))
		return listtuple(v);
	if (is_stringobject(v)) {
		int n = getstringsize(v);
		object *t = newtupleobject(n);
		if (t != NULL) {
			int i;
			char *p = getstringvalue(v);
			for (i = 0; i < n; i++) {
				object *item = newsizedstringobject(p+i, 1);
				if (item == NULL) {
					DECREF(t);
					t = NULL;
					break;
				}
				settupleitem(t, i, item);
			}
		}
		return t;
	}
	/* Generic sequence object */
	if ((sqf = v->ob_type->tp_as_sequence) != NULL) {
		int n = (*sqf->sq_length)(v);
		int i;
		object *t;
		if (n < 0)
			return NULL;
		t = newtupleobject(n);
		if (t == NULL)
			return NULL;
		for (i = 0; i < n; i++) {
			object *item = (*sqf->sq_item)(v, i);
			if (item == NULL) {
				DECREF(t);
				t = NULL;
				break;
			}
			settupleitem(t, i, item);
		}
		/* XXX Should support indefinite-length sequences */
		return t;
	}
	/* None of the above */
	err_setstr(TypeError, "tuple() argument must be a sequence");
	return NULL;
}

static object *
builtin_type(self, args)
	object *self;
	object *args;
{
	object *v;

	if (!newgetargs(args, "O:type", &v))
		return NULL;
	v = (object *)v->ob_type;
	INCREF(v);
	return v;
}

static object *
builtin_vars(self, args)
	object *self;
	object *args;
{
	object *v = NULL;
	object *d;

	if (!newgetargs(args, "|O:vars", &v))
		return NULL;
	if (v == NULL) {
		d = getlocals();
		INCREF(d);
	}
	else {
		d = getattr(v, "__dict__");
		if (d == NULL) {
			err_setstr(TypeError,
			    "vars() argument must have __dict__ attribute");
			return NULL;
		}
	}
	return d;
}

static struct methodlist builtin_methods[] = {
	{"__import__",	builtin___import__, 1},
	{"abs",		builtin_abs, 1},
	{"apply",	builtin_apply, 1},
	{"callable",	builtin_callable, 1},
	{"chr",		builtin_chr, 1},
	{"cmp",		builtin_cmp, 1},
	{"coerce",	builtin_coerce, 1},
	{"compile",	builtin_compile, 1},
	{"delattr",	builtin_delattr, 1},
	{"dir",		builtin_dir, 1},
	{"divmod",	builtin_divmod, 1},
	{"eval",	builtin_eval, 1},
	{"execfile",	builtin_execfile, 1},
	{"filter",	builtin_filter, 1},
	{"float",	builtin_float, 1},
	{"getattr",	builtin_getattr, 1},
	{"hasattr",	builtin_hasattr, 1},
	{"hash",	builtin_hash, 1},
	{"hex",		builtin_hex, 1},
	{"id",		builtin_id, 1},
	{"input",	builtin_input, 1},
	{"int",		builtin_int, 1},
	{"len",		builtin_len, 1},
	{"long",	builtin_long, 1},
	{"map",		builtin_map, 1},
	{"max",		builtin_max, 1},
	{"min",		builtin_min, 1},
	{"oct",		builtin_oct, 1},
	{"open",	builtin_open, 1},
	{"ord",		builtin_ord, 1},
	{"pow",		builtin_pow, 1},
	{"range",	builtin_range, 1},
	{"raw_input",	builtin_raw_input, 1},
	{"reduce",	builtin_reduce, 1},
	{"reload",	builtin_reload, 1},
	{"repr",	builtin_repr, 1},
	{"round",	builtin_round, 1},
	{"setattr",	builtin_setattr, 1},
	{"str",		builtin_str, 1},
	{"tuple",	builtin_tuple, 1},
	{"type",	builtin_type, 1},
	{"vars",	builtin_vars, 1},
	{"xrange",	builtin_xrange, 1},
	{NULL,		NULL},
};

static object *builtin_mod;
static object *builtin_dict;

object *
getbuiltinmod()
{
	return builtin_mod;
}

object *
getbuiltindict()
{
	return builtin_dict;
}

/* Predefined exceptions */

object *AccessError;
object *AttributeError;
object *ConflictError;
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
	AccessError = newstdexception("AccessError");
	AttributeError = newstdexception("AttributeError");
	ConflictError = newstdexception("ConflictError");
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
	builtin_mod = initmodule("__builtin__", builtin_methods);
	builtin_dict = getmoduledict(builtin_mod);
	INCREF(builtin_dict);
	initerrors();
	(void) dictinsert(builtin_dict, "None", None);
}


/* Helper for filter(): filter a tuple through a function */

static object *
filtertuple(func, tuple)
	object *func;
	object *tuple;
{
	object *result;
	register int i, j;
	int len = gettuplesize(tuple);

	if ((result = newtupleobject(len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		object *item, *good;
		int ok;

		if ((item = gettupleitem(tuple, i)) == NULL)
			goto Fail_1;
		if (func == None) {
			INCREF(item);
			good = item;
		}
		else {
			object *arg = mkvalue("(O)", item);
			if (arg == NULL)
				goto Fail_1;
			good = call_object(func, arg);
			DECREF(arg);
			if (good == NULL)
				goto Fail_1;
		}
		ok = testbool(good);
		DECREF(good);
		if (ok) {
			INCREF(item);
			if (settupleitem(result, j++, item) < 0)
				goto Fail_1;
		}
	}

	if (resizetuple(&result, j, 0) < 0)
		return NULL;

	return result;

Fail_1:
	DECREF(result);
	return NULL;
}


/* Helper for filter(): filter a string through a function */

static object *
filterstring(func, strobj)
	object *func;
	object *strobj;
{
	object *result;
	register int i, j;
	int len = getstringsize(strobj);

	if (func == None) {
		/* No character is ever false -- share input string */
		INCREF(strobj);
		return strobj;
	}
	if ((result = newsizedstringobject(NULL, len)) == NULL)
		return NULL;

	for (i = j = 0; i < len; ++i) {
		object *item, *arg, *good;
		int ok;

		item = (*strobj->ob_type->tp_as_sequence->sq_item)(strobj, i);
		if (item == NULL)
			goto Fail_1;
		arg = mkvalue("(O)", item);
		DECREF(item);
		if (arg == NULL)
			goto Fail_1;
		good = call_object(func, arg);
		DECREF(arg);
		if (good == NULL)
			goto Fail_1;
		ok = testbool(good);
		DECREF(good);
		if (ok)
			GETSTRINGVALUE((stringobject *)result)[j++] =
				GETSTRINGVALUE((stringobject *)item)[0];
	}

	if (resizestring(&result, j) < 0)
		return NULL;

	return result;

Fail_1:
	DECREF(result);
	return NULL;
}
