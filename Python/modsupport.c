/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

/* Module support implementation */

#include "allobjects.h"
#include "modsupport.h"
#include "import.h"

object *
initmodule(name, methods)
	char *name;
	struct methodlist *methods;
{
	object *m, *d, *v;
	struct methodlist *ml;
	char *namebuf;
	if ((m = add_module(name)) == NULL) {
		fprintf(stderr, "initializing module: %s\n", name);
		fatal("can't create a module");
	}
	d = getmoduledict(m);
	for (ml = methods; ml->ml_name != NULL; ml++) {
		namebuf = NEW(char, strlen(name) + strlen(ml->ml_name) + 2);
		if (namebuf == NULL)
			fatal("out of mem for method name");
		sprintf(namebuf, "%s.%s", name, ml->ml_name);
		v = newmethodobject(namebuf, ml->ml_meth,
					(object *)NULL, ml->ml_varargs);
		/* XXX The malloc'ed memory in namebuf is never freed */
		if (v == NULL || dictinsert(d, ml->ml_name, v) != 0) {
			fprintf(stderr, "initializing module: %s\n", name);
			fatal("can't initialize module");
		}
		DECREF(v);
	}
	return m;
}


/* Helper for mkvalue() to scan the length of a format */

static int countformat PROTO((char *format, int endchar));
static int countformat(format, endchar)
	char *format;
	int endchar;
{
	int count = 0;
	int level = 0;
	while (level > 0 || *format != endchar) {
		if (*format == '\0') {
			/* Premature end */
			err_setstr(SystemError, "unmatched paren in format");
			return -1;
		}
		else if (*format == '(') {
			if (level == 0)
				count++;
			level++;
		}
		else if (*format == ')')
			level--;
		else if (level == 0 && *format != '#')
			count++;
		format++;
	}
	return count;
}


/* Generic argument list parser */

static int do_arg PROTO((object *arg, char** p_format, va_list *p_va));
static int
do_arg(arg, p_format, p_va)
	object *arg;
	char** p_format;
	va_list *p_va;
{
	char *format = *p_format;
	va_list va = *p_va;
	
	if (arg == NULL)
		return 0; /* Incomplete tuple or list */
	
	switch (*format++) {
	
	case '(': /* tuple, distributed over C parameters */ {
		int i, n;
		if (!is_tupleobject(arg))
			return 0;
		n = gettuplesize(arg);
		for (i = 0; i < n; i++) {
			if (!do_arg(gettupleitem(arg, i), &format, &va))
				return 0;
		}
		if (*format++ != ')')
			return 0;
		break;
		}

	case ')': /* End of format -- too many arguments */
		return 0;

	case 'b': /* byte -- very short int */ {
		char *p = va_arg(va, char *);
		if (is_intobject(arg))
			*p = getintvalue(arg);
		else
			return 0;
		break;
		}

	case 'h': /* short int */ {
		short *p = va_arg(va, short *);
		if (is_intobject(arg))
			*p = getintvalue(arg);
		else
			return 0;
		break;
		}
	
	case 'i': /* int */ {
		int *p = va_arg(va, int *);
		if (is_intobject(arg))
			*p = getintvalue(arg);
		else
			return 0;
		break;
		}
	
	case 'l': /* long int */ {
		long *p = va_arg(va, long *);
		if (is_intobject(arg))
			*p = getintvalue(arg);
		else
			return 0;
		break;
		}
	
	case 'f': /* float */ {
		float *p = va_arg(va, float *);
		if (is_floatobject(arg))
			*p = getfloatvalue(arg);
		else if (is_intobject(arg))
			*p = (float)getintvalue(arg);
		else
			return 0;
		break;
		}
	
	case 'd': /* double */ {
		double *p = va_arg(va, double *);
		if (is_floatobject(arg))
			*p = getfloatvalue(arg);
		else if (is_intobject(arg))
			*p = (double)getintvalue(arg);
		else
			return 0;
		break;
		}
	
	case 'c': /* char */ {
		char *p = va_arg(va, char *);
		if (is_stringobject(arg) && getstringsize(arg) == 1)
			*p = getstringvalue(arg)[0];
		else
			return 0;
		break;
		}
	
	case 's': /* string */ {
		char **p = va_arg(va, char **);
		if (is_stringobject(arg))
			*p = getstringvalue(arg);
		else
			return 0;
		if (*format == '#') {
			int *q = va_arg(va, int *);
			*q = getstringsize(arg);
			format++;
		}
		else if (strlen(*p) != getstringsize(arg)) {
			err_setstr(ValueError, "embedded '\\0' in string arg");
			return 0;
		}
		break;
		}
	
	case 'z': /* string, may be NULL (None) */ {
		char **p = va_arg(va, char **);
		if (arg == None)
			*p = 0;
		else if (is_stringobject(arg))
			*p = getstringvalue(arg);
		else
			return 0;
		if (*format == '#') {
			int *q = va_arg(va, int *);
			if (arg == None)
				*q = 0;
			else
				*q = getstringsize(arg);
			format++;
		}
		else if (*p != NULL && strlen(*p) != getstringsize(arg)) {
			err_setstr(ValueError, "embedded '\\0' in string arg");
			return 0;
		}
		break;
		}
	
	case 'S': /* string object */ {
		object **p = va_arg(va, object **);
		if (is_stringobject(arg))
			*p = arg;
		else
			return 0;
		break;
		}
	
	case 'O': /* object */ {
		object **p = va_arg(va, object **);
		*p = arg;
		break;
		}

	default:
		fprintf(stderr, "bad do_arg format: x%x '%c'\n",
			format[-1], format[-1]);
		return 0;
	
	}
	
	*p_va = va;
	*p_format = format;
	
	return 1;
}

#ifdef USE_STDARG
/* VARARGS2 */
int getargs(object *arg, char *format, ...)
#else
/* VARARGS */
int getargs(va_alist) va_dcl
#endif
{
	char *f;
	int ok;
	va_list va;
#ifdef USE_STDARG

	va_start(va, format);
#else
	object *arg;
	char *format;

	va_start(va);
	arg = va_arg(va, object *);
	format = va_arg(va, char *);
#endif
	if (*format == '\0' || *format == ';') {
		va_end(va);
		if (arg != NULL) {
			char *str = "no arguments needed";
			if (*format == ';')
				str = format+1;
			err_setstr(TypeError, str);
			return 0;
		}
		return 1;
	}
	
	f = format;
	ok = do_arg(arg, &f, &va) && (*f == '\0' || *f == ';');
	va_end(va);
	if (!ok) {
		if (!err_occurred()) {
			char buf[256];
			char *str;
			f = strchr(format, ';');
			if (f != NULL)
				str = f+1;
			else {
				sprintf(buf, "bad argument list (format '%s')",
					format);
				str = buf;
			}
			err_setstr(TypeError, str);
		}
	}
	return ok;
}

#ifdef UNUSED

int
getlongtuplearg(args, a, n)
	object *args;
	long *a; /* [n] */
	int n;
{
	int i;
	if (!is_tupleobject(args) || gettuplesize(args) != n) {
		return err_badarg();
	}
	for (i = 0; i < n; i++) {
		object *v = gettupleitem(args, i);
		if (!is_intobject(v)) {
			return err_badarg();
		}
		a[i] = getintvalue(v);
	}
	return 1;
}

int
getshorttuplearg(args, a, n)
	object *args;
	short *a; /* [n] */
	int n;
{
	int i;
	if (!is_tupleobject(args) || gettuplesize(args) != n) {
		return err_badarg();
	}
	for (i = 0; i < n; i++) {
		object *v = gettupleitem(args, i);
		if (!is_intobject(v)) {
			return err_badarg();
		}
		a[i] = getintvalue(v);
	}
	return 1;
}

int
getlonglistarg(args, a, n)
	object *args;
	long *a; /* [n] */
	int n;
{
	int i;
	if (!is_listobject(args) || getlistsize(args) != n) {
		return err_badarg();
	}
	for (i = 0; i < n; i++) {
		object *v = getlistitem(args, i);
		if (!is_intobject(v)) {
			return err_badarg();
		}
		a[i] = getintvalue(v);
	}
	return 1;
}

int
getshortlistarg(args, a, n)
	object *args;
	short *a; /* [n] */
	int n;
{
	int i;
	if (!is_listobject(args) || getlistsize(args) != n) {
		return err_badarg();
	}
	for (i = 0; i < n; i++) {
		object *v = getlistitem(args, i);
		if (!is_intobject(v)) {
			return err_badarg();
		}
		a[i] = getintvalue(v);
	}
	return 1;
}

#endif /* UNUSED */


/* Generic function to create a value -- the inverse of getargs() */
/* After an original idea and first implementation by Steven Miale */

static object *do_mktuple PROTO((char**, va_list *, int, int));
static object *do_mkvalue PROTO((char**, va_list *));

static object *
do_mktuple(p_format, p_va, endchar, n)
	char **p_format;
	va_list *p_va;
	int endchar;
	int n;
{
	object *v;
	int i;
	if (n < 0)
		return NULL;
	if ((v = newtupleobject(n)) == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		object *w = do_mkvalue(p_format, p_va);
		if (w == NULL) {
			DECREF(v);
			return NULL;
		}
		settupleitem(v, i, w);
	}
	if (v != NULL && **p_format != endchar) {
		DECREF(v);
		v = NULL;
		err_setstr(SystemError, "Unmatched paren in format");
	}
	else if (endchar)
		++*p_format;
	return v;
}

static object *
do_mkvalue(p_format, p_va)
	char **p_format;
	va_list *p_va;
{
	
	switch (*(*p_format)++) {
	
	case '(':
		return do_mktuple(p_format, p_va, ')',
				  countformat(*p_format, ')'));
		
	case 'b':
	case 'h':
	case 'i':
		return newintobject((long)va_arg(*p_va, int));
		
	case 'l':
		return newintobject((long)va_arg(*p_va, long));
		
	case 'f':
	case 'd':
		return newfloatobject((double)va_arg(*p_va, double));
		
	case 'c':
		{
			char p[1];
			p[0] = va_arg(*p_va, int);
			return newsizedstringobject(p, 1);
		}
	
	case 's':
	case 'z':
		{
			object *v;
			char *str = va_arg(*p_va, char *);
			int n;
			if (**p_format == '#') {
				++*p_format;
				n = va_arg(*p_va, int);
			}
			else
				n = -1;
			if (str == NULL) {
				v = None;
				INCREF(v);
			}
			else {
				if (n < 0)
					n = strlen(str);
				v = newsizedstringobject(str, n);
			}
			return v;
		}
	
	case 'S':
	case 'O':
		{
			object *v;
			v = va_arg(*p_va, object *);
			if (v != NULL)
				INCREF(v);
			else if (!err_occurred())
				/* If a NULL was passed because a call
				   that should have constructed a value
				   failed, that's OK, and we pass the error
				   on; but if no error occurred it's not
				   clear that the caller knew what she
				   was doing. */
				err_setstr(SystemError,
					   "NULL object passed to mkvalue");
			return v;
		}
	
	default:
		err_setstr(SystemError, "bad format char passed to mkvalue");
		return NULL;
	
	}
}

#ifdef USE_STDARG
/* VARARGS 2 */
object *mkvalue(char *format, ...)
#else
/* VARARGS */
object *mkvalue(va_alist) va_dcl
#endif
{
	va_list va;
	object* retval;
#ifdef USE_STDARG
	va_start(va, format);
#else
	char *format;
	va_start(va);
	format = va_arg(va, char *);
#endif
	retval = vmkvalue(format, va);
	va_end(va);
	return retval;
}

object *
vmkvalue(format, va)
	char *format;
	va_list va;
{
	char *f = format;
	int n = countformat(f, '\0');
	if (n < 0)
		return NULL;
	if (n == 0) {
		INCREF(None);
		return None;
	}
	if (n == 1)
		return do_mkvalue(&f, &va);
	return do_mktuple(&f, &va, '\0', n);
}
