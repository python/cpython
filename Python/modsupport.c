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

/* Module support implementation */

#include "allobjects.h"
#include "import.h"

#ifdef MPW /* MPW pushes 'extended' for float and double types with varargs */
typedef extended va_double;
#else
typedef double va_double;
#endif

/* initmodule4() parameters:
   - name is the module name
   - methods is the list of top-level functions
   - doc is the documentation string
   - passthrough is passed as self to functions defined in the module
   - api_version is the value of PYTHON_API_VERSION at the time the
     module was compiled
*/

static char api_version_warning[] =
"WARNING: Python C API version mismatch for module %s:\n\
  This Python has API version %d, module %s has version %s.\n";

object *
initmodule4(name, methods, doc, passthrough, module_api_version)
	char *name;
	struct methodlist *methods;
	char *doc;
	object *passthrough;
	int module_api_version;
{
	object *m, *d, *v;
	struct methodlist *ml;
	if (module_api_version != PYTHON_API_VERSION)
		fprintf(stderr, api_version_warning,
			name, PYTHON_API_VERSION, name, module_api_version);
	if ((m = add_module(name)) == NULL) {
		fprintf(stderr, "initializing module: %s\n", name);
		fatal("can't create a module");
	}
	d = getmoduledict(m);
	for (ml = methods; ml->ml_name != NULL; ml++) {
		v = newmethodobject(ml, passthrough);
		if (v == NULL || dictinsert(d, ml->ml_name, v) != 0) {
			fprintf(stderr, "initializing module: %s\n", name);
			fatal("can't initialize module");
		}
		DECREF(v);
	}
	if (doc != NULL) {
		v = newstringobject(doc);
		if (v == NULL || dictinsert(d, "__doc__", v) != 0)
			fatal("can't add doc string");
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
		switch (*format) {
		case '\0':
			/* Premature end */
			err_setstr(SystemError, "unmatched paren in format");
			return -1;
		case '(':
		case '[':
		case '{':
			if (level == 0)
				count++;
			level++;
			break;
		case ')':
		case ']':
		case '}':
			level--;
			break;
		case '#':
		case ',':
		case ':':
		case ' ':
		case '\t':
			break;
		default:
			if (level == 0)
				count++;
		}
		format++;
	}
	return count;
}


/* Generic function to create a value -- the inverse of getargs() */
/* After an original idea and first implementation by Steven Miale */

static object *do_mktuple PROTO((char**, va_list *, int, int));
static object *do_mklist PROTO((char**, va_list *, int, int));
static object *do_mkdict PROTO((char**, va_list *, int, int));
static object *do_mkvalue PROTO((char**, va_list *));


static object *
do_mkdict(p_format, p_va, endchar, n)
	char **p_format;
	va_list *p_va;
	int endchar;
	int n;
{
	object *d;
	int i;
	if (n < 0)
		return NULL;
	if ((d = newdictobject()) == NULL)
		return NULL;
	for (i = 0; i < n; i+= 2) {
		object *k, *v;
		k = do_mkvalue(p_format, p_va);
		if (k == NULL) {
			DECREF(d);
			return NULL;
		}
		v = do_mkvalue(p_format, p_va);
		if (v == NULL) {
			DECREF(k);
			DECREF(d);
			return NULL;
		}
		if (dict2insert(d, k, v) < 0) {
			DECREF(k);
			DECREF(v);
			DECREF(d);
			return NULL;
		}
	}
	if (d != NULL && **p_format != endchar) {
		DECREF(d);
		d = NULL;
		err_setstr(SystemError, "Unmatched paren in format");
	}
	else if (endchar)
		++*p_format;
	return d;
}

static object *
do_mklist(p_format, p_va, endchar, n)
	char **p_format;
	va_list *p_va;
	int endchar;
	int n;
{
	object *v;
	int i;
	if (n < 0)
		return NULL;
	if ((v = newlistobject(n)) == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		object *w = do_mkvalue(p_format, p_va);
		if (w == NULL) {
			DECREF(v);
			return NULL;
		}
		setlistitem(v, i, w);
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
	for (;;) {
		switch (*(*p_format)++) {
		case '(':
			return do_mktuple(p_format, p_va, ')',
					  countformat(*p_format, ')'));

		case '[':
			return do_mklist(p_format, p_va, ']',
					 countformat(*p_format, ']'));

		case '{':
			return do_mkdict(p_format, p_va, '}',
					 countformat(*p_format, '}'));

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
				/* If a NULL was passed
				 * because a call that should
				 * have constructed a value
				 * failed, that's OK, and we
				 * pass the error on; but if
				 * no error occurred it's not
				 * clear that the caller knew
				 * what she was doing. */
				err_setstr(SystemError,
					   "NULL object passed to mkvalue");
			return v;
		}

		case ':':
		case ',':
		case ' ':
		case '\t':
			break;

		default:
			err_setstr(SystemError,
				   "bad format char passed to mkvalue");
			return NULL;

		}
	}
}


#ifdef HAVE_STDARG_PROTOTYPES
/* VARARGS 2 */
object *mkvalue(char *format, ...)
#else
/* VARARGS */
object *mkvalue(va_alist) va_dcl
#endif
{
	va_list va;
	object* retval;
#ifdef HAVE_STDARG_PROTOTYPES
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
	va_list lva;

#ifdef VA_LIST_IS_ARRAY
	memcpy(lva, va, sizeof(va_list));
#else
	lva = va;
#endif

	if (n < 0)
		return NULL;
	if (n == 0) {
		INCREF(None);
		return None;
	}
	if (n == 1)
		return do_mkvalue(&f, &lva);
	return do_mktuple(&f, &lva, '\0', n);
}


#ifdef HAVE_STDARG_PROTOTYPES
object *
PyEval_CallFunction(object *obj, char *format, ...)
#else
object *
PyEval_CallFunction(obj, format, va_alist)
	object *obj;
	char *format;
	va_dcl
#endif
{
	va_list vargs;
	object *args;
	object *res;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif

	args = vmkvalue(format, vargs);
	va_end(vargs);

	if (args == NULL)
		return NULL;

	res = call_object(obj, args);
	DECREF(args);

	return res;
}


#ifdef HAVE_STDARG_PROTOTYPES
object *
PyEval_CallMethod(object *obj, char *methonname, char *format, ...)
#else
object *
PyEval_CallMethod(obj, methonname, format, va_alist)
	object *obj;
	char *methonname;
	char *format;
	va_dcl
#endif
{
	va_list vargs;
	object *meth;
	object *args;
	object *res;

	meth = getattr(obj, methonname);
	if (meth == NULL)
		return NULL;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif

	args = vmkvalue(format, vargs);
	va_end(vargs);

	if (args == NULL) {
		DECREF(meth);
		return NULL;
	}

	res = call_object(meth, args);
	DECREF(meth);
	DECREF(args);

	return res;
}
