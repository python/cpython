/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
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

/* Module support implementation */

#include "allobjects.h"
#include "modsupport.h"
#include "import.h"

#ifdef HAVE_PROTOTYPES
#define USE_STDARG
#endif

#ifdef USE_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif


object *
initmodule(name, methods)
	char *name;
	struct methodlist *methods;
{
	object *m, *d, *v;
	struct methodlist *ml;
	char namebuf[256];
	if ((m = add_module(name)) == NULL) {
		fprintf(stderr, "initializing module: %s\n", name);
		fatal("can't create a module");
	}
	d = getmoduledict(m);
	for (ml = methods; ml->ml_name != NULL; ml++) {
		sprintf(namebuf, "%s.%s", name, ml->ml_name);
		v = newmethodobject(strdup(namebuf), ml->ml_meth,
					(object *)NULL, ml->ml_varargs);
		/* XXX The strdup'ed memory is never freed */
		if (v == NULL || dictinsert(d, ml->ml_name, v) != 0) {
			fprintf(stderr, "initializing module: %s\n", name);
			fatal("can't initialize module");
		}
		DECREF(v);
	}
	return m;
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
	
	case '('/*')'*/: /* tuple, distributed over C parameters */ {
		int i, n;
		if (!is_tupleobject(arg))
			return 0;
		n = gettuplesize(arg);
		for (i = 0; i < n; i++) {
			if (!do_arg(gettupleitem(arg, i), &format, &va))
				return 0;
		}
		if (*format++ != /*'('*/')')
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
	if (*format == '\0') {
		va_end(va);
		if (arg != NULL) {
			err_setstr(TypeError, "no arguments needed");
			return 0;
		}
		return 1;
	}
	
	f = format;
	ok = do_arg(arg, &f, &va) && *f == '\0';
	va_end(va);
	if (!ok) {
		char buf[256];
		sprintf(buf, "bad argument list (format '%s')", format);
		err_setstr(TypeError, buf);
	}
	return ok;
}

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

static object *
do_mkval(char **p_format, va_list *p_va) {
	object *v;
	
	switch (*(*p_format)++) {
	
	case '(':
		{
			int n = 0;
			char *p = *p_format;
			int level = 0;
			int i;
			while (level > 0 || *p != ')') {
				if (*p == '\0') {
					err_setstr(SystemError, "missing ')' in mkvalue format");
					return NULL;
				}
				else if (*p == '(') {
					if (level == 0)
						n++;
					level++;
				}
				else if (*p == ')')
					level--;
				else if (level == 0 && *p != '#')
					n++;
				p++;
			}
			v = newtupleobject(n);
			if (v == NULL)
				break;
			for (i = 0; i < n; i++) {
				object *w = do_mkval(p_format, p_va);
				if (w == NULL) {
					DECREF(v);
					v = NULL;
					break;
				}
				settupleitem(v, i, w);
			}
			if (v != NULL && *(*p_format)++ != ')') {
				/* "Cannot happen" */
				err_setstr(SystemError, "inconsistent format in mkvalue???");
				DECREF(v);
				v = NULL;
			}
		}
		break;
		
	case 'h':
		v = newintobject((long)va_arg(*p_va, short));
		break;
		
	case 'i':
		v = newintobject((long)va_arg(*p_va, int));
		break;
		
	case 'l':
		v = newintobject((long)va_arg(*p_va, long));
		break;
		
	case 'f':
		v = newfloatobject((double)va_arg(*p_va, float));
		break;
		
	case 'd':
		v = newfloatobject((double)va_arg(*p_va, double));
		break;
	
	case 's':
	case 'z':
		{
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
		}
		break;
	
	case 'S':
	case 'O':
		v = va_arg(*p_va, object *);
		if (v == NULL) {
			if (!err_occurred())
				err_setstr(SystemError, "NULL object passed to mkvalue");
		}
		else
			INCREF(v);
		break;
	
	default:
		err_setstr(SystemError, "bad format char passed to mkvalue");
		v = NULL;
		break;
	
	}
	
	return v;
}

object *
mkvalue(char *format, ...)
{
	char *fmt = format;
	object *v;
	va_list p;
	va_start(p, format);
	v = do_mkval(&fmt, &p);
	va_end(p);
	if (v == NULL)
		fprintf(stderr, "mkvalue: format = \"%s\" \"%s\"\n", format, fmt);
	return v;
}
