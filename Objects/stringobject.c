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

/* String object implementation */

#include "allobjects.h"

#include <ctype.h>

#ifdef COUNT_ALLOCS
int null_strings, one_strings;
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif
#endif

static stringobject *characters[UCHAR_MAX + 1];
#ifndef DONT_SHARE_SHORT_STRINGS
static stringobject *nullstring;
#endif

/*
   Newsizedstringobject() and newstringobject() try in certain cases
   to share string objects.  When the size of the string is zero,
   these routines always return a pointer to the same string object;
   when the size is one, they return a pointer to an already existing
   object if the contents of the string is known.  For
   newstringobject() this is always the case, for
   newsizedstringobject() this is the case when the first argument in
   not NULL.
   A common practice to allocate a string and then fill it in or
   change it must be done carefully.  It is only allowed to change the
   contents of the string if the obect was gotten from
   newsizedstringobject() with a NULL first argument, because in the
   future these routines may try to do even more sharing of objects.
*/
object *
newsizedstringobject(str, size)
	const char *str;
	int size;
{
	register stringobject *op;
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0 && (op = nullstring) != NULL) {
#ifdef COUNT_ALLOCS
		null_strings++;
#endif
		INCREF(op);
		return (object *)op;
	}
	if (size == 1 && str != NULL && (op = characters[*str & UCHAR_MAX]) != NULL) {
#ifdef COUNT_ALLOCS
		one_strings++;
#endif
		INCREF(op);
		return (object *)op;
	}
#endif /* DONT_SHARE_SHORT_STRINGS */
	op = (stringobject *)
		malloc(sizeof(stringobject) + size * sizeof(char));
	if (op == NULL)
		return err_nomem();
	op->ob_type = &Stringtype;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	NEWREF(op);
	if (str != NULL)
		memcpy(op->ob_sval, str, size);
	op->ob_sval[size] = '\0';
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0) {
		nullstring = op;
		INCREF(op);
	} else if (size == 1 && str != NULL) {
		characters[*str & UCHAR_MAX] = op;
		INCREF(op);
	}
#endif
	return (object *) op;
}

object *
newstringobject(str)
	const char *str;
{
	register unsigned int size = strlen(str);
	register stringobject *op;
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0 && (op = nullstring) != NULL) {
#ifdef COUNT_ALLOCS
		null_strings++;
#endif
		INCREF(op);
		return (object *)op;
	}
	if (size == 1 && (op = characters[*str & UCHAR_MAX]) != NULL) {
#ifdef COUNT_ALLOCS
		one_strings++;
#endif
		INCREF(op);
		return (object *)op;
	}
#endif /* DONT_SHARE_SHORT_STRINGS */
	op = (stringobject *)
		malloc(sizeof(stringobject) + size * sizeof(char));
	if (op == NULL)
		return err_nomem();
	op->ob_type = &Stringtype;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	NEWREF(op);
	strcpy(op->ob_sval, str);
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0) {
		nullstring = op;
		INCREF(op);
	} else if (size == 1) {
		characters[*str & UCHAR_MAX] = op;
		INCREF(op);
	}
#endif
	return (object *) op;
}

static void
string_dealloc(op)
	object *op;
{
	DEL(op);
}

int
getstringsize(op)
	register object *op;
{
	if (!is_stringobject(op)) {
		err_badcall();
		return -1;
	}
	return ((stringobject *)op) -> ob_size;
}

/*const*/ char *
getstringvalue(op)
	register object *op;
{
	if (!is_stringobject(op)) {
		err_badcall();
		return NULL;
	}
	return ((stringobject *)op) -> ob_sval;
}

/* Methods */

static int
string_print(op, fp, flags)
	stringobject *op;
	FILE *fp;
	int flags;
{
	int i;
	char c;
	int quote;
	/* XXX Ought to check for interrupts when writing long strings */
	if (flags & PRINT_RAW) {
		fwrite(op->ob_sval, 1, (int) op->ob_size, fp);
		return 0;
	}

	/* figure out which quote to use; single is prefered */
	quote = '\'';
	if (strchr(op->ob_sval, '\'') && !strchr(op->ob_sval, '"'))
		quote = '"';

	fputc(quote, fp);
	for (i = 0; i < op->ob_size; i++) {
		c = op->ob_sval[i];
		if (c == quote || c == '\\')
			fprintf(fp, "\\%c", c);
		else if (c < ' ' || c >= 0177)
			fprintf(fp, "\\%03o", c & 0377);
		else
			fputc(c, fp);
	}
	fputc(quote, fp);
	return 0;
}

static object *
string_repr(op)
	register stringobject *op;
{
	/* XXX overflow? */
	int newsize = 2 + 4 * op->ob_size * sizeof(char);
	object *v = newsizedstringobject((char *)NULL, newsize);
	if (v == NULL) {
		return NULL;
	}
	else {
		register int i;
		register char c;
		register char *p;
		int quote;

		/* figure out which quote to use; single is prefered */
		quote = '\'';
		if (strchr(op->ob_sval, '\'') && !strchr(op->ob_sval, '"'))
			quote = '"';

		p = ((stringobject *)v)->ob_sval;
		*p++ = quote;
		for (i = 0; i < op->ob_size; i++) {
			c = op->ob_sval[i];
			if (c == quote || c == '\\')
				*p++ = '\\', *p++ = c;
			else if (c < ' ' || c >= 0177) {
				sprintf(p, "\\%03o", c & 0377);
				while (*p != '\0')
					p++;
			}
			else
				*p++ = c;
		}
		*p++ = quote;
		*p = '\0';
		resizestring(&v, (int) (p - ((stringobject *)v)->ob_sval));
		return v;
	}
}

static int
string_length(a)
	stringobject *a;
{
	return a->ob_size;
}

static object *
string_concat(a, bb)
	register stringobject *a;
	register object *bb;
{
	register unsigned int size;
	register stringobject *op;
	if (!is_stringobject(bb)) {
		err_badarg();
		return NULL;
	}
#define b ((stringobject *)bb)
	/* Optimize cases with empty left or right operand */
	if (a->ob_size == 0) {
		INCREF(bb);
		return bb;
	}
	if (b->ob_size == 0) {
		INCREF(a);
		return (object *)a;
	}
	size = a->ob_size + b->ob_size;
	op = (stringobject *)
		malloc(sizeof(stringobject) + size * sizeof(char));
	if (op == NULL)
		return err_nomem();
	op->ob_type = &Stringtype;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	NEWREF(op);
	memcpy(op->ob_sval, a->ob_sval, (int) a->ob_size);
	memcpy(op->ob_sval + a->ob_size, b->ob_sval, (int) b->ob_size);
	op->ob_sval[size] = '\0';
	return (object *) op;
#undef b
}

static object *
string_repeat(a, n)
	register stringobject *a;
	register int n;
{
	register int i;
	register unsigned int size;
	register stringobject *op;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	if (size == a->ob_size) {
		INCREF(a);
		return (object *)a;
	}
	op = (stringobject *)
		malloc(sizeof(stringobject) + size * sizeof(char));
	if (op == NULL)
		return err_nomem();
	op->ob_type = &Stringtype;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	NEWREF(op);
	for (i = 0; i < size; i += a->ob_size)
		memcpy(op->ob_sval+i, a->ob_sval, (int) a->ob_size);
	op->ob_sval[size] = '\0';
	return (object *) op;
}

/* String slice a[i:j] consists of characters a[i] ... a[j-1] */

static object *
string_slice(a, i, j)
	register stringobject *a;
	register int i, j; /* May be negative! */
{
	if (i < 0)
		i = 0;
	if (j < 0)
		j = 0; /* Avoid signed/unsigned bug in next line */
	if (j > a->ob_size)
		j = a->ob_size;
	if (i == 0 && j == a->ob_size) { /* It's the same as a */
		INCREF(a);
		return (object *)a;
	}
	if (j < i)
		j = i;
	return newsizedstringobject(a->ob_sval + i, (int) (j-i));
}

static object *
string_item(a, i)
	stringobject *a;
	register int i;
{
	int c;
	object *v;
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "string index out of range");
		return NULL;
	}
	c = a->ob_sval[i] & UCHAR_MAX;
	v = (object *) characters[c];
#ifdef COUNT_ALLOCS
	if (v != NULL)
		one_strings++;
#endif
	if (v == NULL) {
		v = newsizedstringobject((char *)NULL, 1);
		if (v == NULL)
			return NULL;
		characters[c] = (stringobject *) v;
		((stringobject *)v)->ob_sval[0] = c;
	}
	INCREF(v);
	return v;
}

static int
string_compare(a, b)
	stringobject *a, *b;
{
	int len_a = a->ob_size, len_b = b->ob_size;
	int min_len = (len_a < len_b) ? len_a : len_b;
	int cmp;
	if (min_len > 0) {
		cmp = Py_CHARMASK(*a->ob_sval) - Py_CHARMASK(*b->ob_sval);
		if (cmp == 0)
			cmp = memcmp(a->ob_sval, b->ob_sval, min_len);
		if (cmp != 0)
			return cmp;
	}
	return (len_a < len_b) ? -1 : (len_a > len_b) ? 1 : 0;
}

static long
string_hash(a)
	stringobject *a;
{
	register int len;
	register unsigned char *p;
	register long x;

#ifdef CACHE_HASH
	if (a->ob_shash != -1)
		return a->ob_shash;
#endif
	len = a->ob_size;
	p = (unsigned char *) a->ob_sval;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= a->ob_size;
	if (x == -1)
		x = -2;
#ifdef CACHE_HASH
	a->ob_shash = x;
#endif
	return x;
}

static sequence_methods string_as_sequence = {
	(inquiry)string_length, /*sq_length*/
	(binaryfunc)string_concat, /*sq_concat*/
	(intargfunc)string_repeat, /*sq_repeat*/
	(intargfunc)string_item, /*sq_item*/
	(intintargfunc)string_slice, /*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
};

typeobject Stringtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"string",
	sizeof(stringobject),
	sizeof(char),
	(destructor)string_dealloc, /*tp_dealloc*/
	(printfunc)string_print, /*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)string_compare, /*tp_compare*/
	(reprfunc)string_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	&string_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)string_hash, /*tp_hash*/
	0,		/*tp_call*/
	0,		/*tp_str*/
	0,		/*tp_getattro*/
	0,		/*tp_setattro*/
	0,		/*tp_xxx3*/
	0,		/*tp_xxx4*/
	0,		/*tp_doc*/
};

void
joinstring(pv, w)
	register object **pv;
	register object *w;
{
	register object *v;
	if (*pv == NULL)
		return;
	if (w == NULL || !is_stringobject(*pv)) {
		DECREF(*pv);
		*pv = NULL;
		return;
	}
	v = string_concat((stringobject *) *pv, w);
	DECREF(*pv);
	*pv = v;
}

void
joinstring_decref(pv, w)
	register object **pv;
	register object *w;
{
	joinstring(pv, w);
	XDECREF(w);
}


/* The following function breaks the notion that strings are immutable:
   it changes the size of a string.  We get away with this only if there
   is only one module referencing the object.  You can also think of it
   as creating a new string object and destroying the old one, only
   more efficiently.  In any case, don't use this if the string may
   already be known to some other part of the code... */

int
resizestring(pv, newsize)
	object **pv;
	int newsize;
{
	register object *v;
	register stringobject *sv;
	v = *pv;
	if (!is_stringobject(v) || v->ob_refcnt != 1) {
		*pv = 0;
		DECREF(v);
		err_badcall();
		return -1;
	}
	/* XXX UNREF/NEWREF interface should be more symmetrical */
#ifdef Py_REF_DEBUG
	--_Py_RefTotal;
#endif
	UNREF(v);
	*pv = (object *)
		realloc((char *)v,
			sizeof(stringobject) + newsize * sizeof(char));
	if (*pv == NULL) {
		DEL(v);
		err_nomem();
		return -1;
	}
	NEWREF(*pv);
	sv = (stringobject *) *pv;
	sv->ob_size = newsize;
	sv->ob_sval[newsize] = '\0';
	return 0;
}

/* Helpers for formatstring */

static object *
getnextarg(args, arglen, p_argidx)
	object *args;
	int arglen;
	int *p_argidx;
{
	int argidx = *p_argidx;
	if (argidx < arglen) {
		(*p_argidx)++;
		if (arglen < 0)
			return args;
		else
			return gettupleitem(args, argidx);
	}
	err_setstr(TypeError, "not enough arguments for format string");
	return NULL;
}

#define F_LJUST (1<<0)
#define F_SIGN	(1<<1)
#define F_BLANK (1<<2)
#define F_ALT	(1<<3)
#define F_ZERO	(1<<4)

extern double fabs PROTO((double));

static int
formatfloat(buf, flags, prec, type, v)
	char *buf;
	int flags;
	int prec;
	int type;
	object *v;
{
	char fmt[20];
	double x;
	if (!getargs(v, "d;float argument required", &x))
		return -1;
	if (prec < 0)
		prec = 6;
	if (prec > 50)
		prec = 50; /* Arbitrary limitation */
	if (type == 'f' && fabs(x)/1e25 >= 1e25)
		type = 'g';
	sprintf(fmt, "%%%s.%d%c", (flags&F_ALT) ? "#" : "", prec, type);
	sprintf(buf, fmt, x);
	return strlen(buf);
}

static int
formatint(buf, flags, prec, type, v)
	char *buf;
	int flags;
	int prec;
	int type;
	object *v;
{
	char fmt[20];
	long x;
	if (!getargs(v, "l;int argument required", &x))
		return -1;
	if (prec < 0)
		prec = 1;
	sprintf(fmt, "%%%s.%dl%c", (flags&F_ALT) ? "#" : "", prec, type);
	sprintf(buf, fmt, x);
	return strlen(buf);
}

static int
formatchar(buf, v)
	char *buf;
	object *v;
{
	if (is_stringobject(v)) {
		if (!getargs(v, "c;%c requires int or char", &buf[0]))
			return -1;
	}
	else {
		if (!getargs(v, "b;%c requires int or char", &buf[0]))
			return -1;
	}
	buf[1] = '\0';
	return 1;
}


/* fmt%(v1,v2,...) is roughly equivalent to sprintf(fmt, v1, v2, ...) */

object *
formatstring(format, args)
	object *format;
	object *args;
{
	char *fmt, *res;
	int fmtcnt, rescnt, reslen, arglen, argidx;
	int args_owned = 0;
	object *result;
	object *dict = NULL;
	if (format == NULL || !is_stringobject(format) || args == NULL) {
		err_badcall();
		return NULL;
	}
	fmt = getstringvalue(format);
	fmtcnt = getstringsize(format);
	reslen = rescnt = fmtcnt + 100;
	result = newsizedstringobject((char *)NULL, reslen);
	if (result == NULL)
		return NULL;
	res = getstringvalue(result);
	if (is_tupleobject(args)) {
		arglen = gettuplesize(args);
		argidx = 0;
	}
	else {
		arglen = -1;
		argidx = -2;
	}
	if (args->ob_type->tp_as_mapping)
		dict = args;
	while (--fmtcnt >= 0) {
		if (*fmt != '%') {
			if (--rescnt < 0) {
				rescnt = fmtcnt + 100;
				reslen += rescnt;
				if (resizestring(&result, reslen) < 0)
					return NULL;
				res = getstringvalue(result) + reslen - rescnt;
				--rescnt;
			}
			*res++ = *fmt++;
		}
		else {
			/* Got a format specifier */
			int flags = 0;
			int width = -1;
			int prec = -1;
			int size = 0;
			int c = '\0';
			int fill;
			object *v = NULL;
			object *temp = NULL;
			char *buf;
			int sign;
			int len;
			char tmpbuf[120]; /* For format{float,int,char}() */
			fmt++;
			if (*fmt == '(') {
				char *keystart;
				int keylen;
				object *key;

				if (dict == NULL) {
					err_setstr(TypeError,
						 "format requires a mapping"); 
					goto error;
				}
				++fmt;
				--fmtcnt;
				keystart = fmt;
				while (--fmtcnt >= 0 && *fmt != ')')
					fmt++;
				keylen = fmt - keystart;
				++fmt;
				if (fmtcnt < 0) {
					err_setstr(ValueError,
						   "incomplete format key");
					goto error;
				}
				key = newsizedstringobject(keystart, keylen);
				if (key == NULL)
					goto error;
				if (args_owned) {
					DECREF(args);
					args_owned = 0;
				}
				args = PyObject_GetItem(dict, key);
				DECREF(key);
				if (args == NULL) {
					goto error;
				}
				args_owned = 1;
				arglen = -1;
				argidx = -2;
			}
			while (--fmtcnt >= 0) {
				switch (c = *fmt++) {
				case '-': flags |= F_LJUST; continue;
				case '+': flags |= F_SIGN; continue;
				case ' ': flags |= F_BLANK; continue;
				case '#': flags |= F_ALT; continue;
				case '0': flags |= F_ZERO; continue;
				}
				break;
			}
			if (c == '*') {
				v = getnextarg(args, arglen, &argidx);
				if (v == NULL)
					goto error;
				if (!is_intobject(v)) {
					err_setstr(TypeError, "* wants int");
					goto error;
				}
				width = getintvalue(v);
				if (width < 0)
					width = 0;
				if (--fmtcnt >= 0)
					c = *fmt++;
			}
			else if (c >= 0 && isdigit(c)) {
				width = c - '0';
				while (--fmtcnt >= 0) {
					c = Py_CHARMASK(*fmt++);
					if (!isdigit(c))
						break;
					if ((width*10) / 10 != width) {
						err_setstr(ValueError,
							   "width too big");
						goto error;
					}
					width = width*10 + (c - '0');
				}
			}
			if (c == '.') {
				prec = 0;
				if (--fmtcnt >= 0)
					c = *fmt++;
				if (c == '*') {
					v = getnextarg(args, arglen, &argidx);
					if (v == NULL)
						goto error;
					if (!is_intobject(v)) {
						err_setstr(TypeError,
							   "* wants int");
						goto error;
					}
					prec = getintvalue(v);
					if (prec < 0)
						prec = 0;
					if (--fmtcnt >= 0)
						c = *fmt++;
				}
				else if (c >= 0 && isdigit(c)) {
					prec = c - '0';
					while (--fmtcnt >= 0) {
						c = Py_CHARMASK(*fmt++);
						if (!isdigit(c))
							break;
						if ((prec*10) / 10 != prec) {
							err_setstr(ValueError,
							    "prec too big");
							goto error;
						}
						prec = prec*10 + (c - '0');
					}
				}
			} /* prec */
			if (fmtcnt >= 0) {
				if (c == 'h' || c == 'l' || c == 'L') {
					size = c;
					if (--fmtcnt >= 0)
						c = *fmt++;
				}
			}
			if (fmtcnt < 0) {
				err_setstr(ValueError, "incomplete format");
				goto error;
			}
			if (c != '%') {
				v = getnextarg(args, arglen, &argidx);
				if (v == NULL)
					goto error;
			}
			sign = 0;
			fill = ' ';
			switch (c) {
			case '%':
				buf = "%";
				len = 1;
				break;
			case 's':
				temp = strobject(v);
				if (temp == NULL)
					goto error;
				buf = getstringvalue(temp);
				len = getstringsize(temp);
				if (prec >= 0 && len > prec)
					len = prec;
				break;
			case 'i':
			case 'd':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				if (c == 'i')
					c = 'd';
				buf = tmpbuf;
				len = formatint(buf, flags, prec, c, v);
				if (len < 0)
					goto error;
				sign = (c == 'd');
				if (flags&F_ZERO) {
					fill = '0';
					if ((flags&F_ALT) &&
					    (c == 'x' || c == 'X') &&
					    buf[0] == '0' && buf[1] == c) {
						*res++ = *buf++;
						*res++ = *buf++;
						rescnt -= 2;
						len -= 2;
						width -= 2;
						if (width < 0)
							width = 0;
					}
				}
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				buf = tmpbuf;
				len = formatfloat(buf, flags, prec, c, v);
				if (len < 0)
					goto error;
				sign = 1;
				if (flags&F_ZERO)
					fill = '0';
				break;
			case 'c':
				buf = tmpbuf;
				len = formatchar(buf, v);
				if (len < 0)
					goto error;
				break;
			default:
				err_setstr(ValueError,
					   "unsupported format character");
				goto error;
			}
			if (sign) {
				if (*buf == '-' || *buf == '+') {
					sign = *buf++;
					len--;
				}
				else if (flags & F_SIGN)
					sign = '+';
				else if (flags & F_BLANK)
					sign = ' ';
				else
					sign = '\0';
			}
			if (width < len)
				width = len;
			if (rescnt < width + (sign != '\0')) {
				reslen -= rescnt;
				rescnt = width + fmtcnt + 100;
				reslen += rescnt;
				if (resizestring(&result, reslen) < 0)
					return NULL;
				res = getstringvalue(result) + reslen - rescnt;
			}
			if (sign) {
				if (fill != ' ')
					*res++ = sign;
				rescnt--;
				if (width > len)
					width--;
			}
			if (width > len && !(flags&F_LJUST)) {
				do {
					--rescnt;
					*res++ = fill;
				} while (--width > len);
			}
			if (sign && fill == ' ')
				*res++ = sign;
			memcpy(res, buf, len);
			res += len;
			rescnt -= len;
			while (--width >= len) {
				--rescnt;
				*res++ = ' ';
			}
                        if (dict && (argidx < arglen) && c != '%') {
                                err_setstr(TypeError,
                                           "not all arguments converted");
                                goto error;
                        }
			XDECREF(temp);
		} /* '%' */
	} /* until end */
	if (argidx < arglen && !dict) {
		err_setstr(TypeError, "not all arguments converted");
		goto error;
	}
	if (args_owned)
		DECREF(args);
	resizestring(&result, reslen - rescnt);
	return result;
 error:
	DECREF(result);
	if (args_owned)
		DECREF(args);
	return NULL;
}


#ifdef INTERN_STRINGS

static PyObject *interned;

void
PyString_InternInPlace(p)
	PyObject **p;
{
	register PyStringObject *s = (PyStringObject *)(*p);
	PyObject *t;
	if (s == NULL || !PyString_Check(s))
		Py_FatalError("PyString_InternInPlace: strings only please!");
	if ((t = s->ob_sinterned) != NULL) {
		if (t == (PyObject *)s)
			return;
		Py_INCREF(t);
		*p = t;
		Py_DECREF(s);
		return;
	}
	if (interned == NULL) {
		interned = PyDict_New();
		if (interned == NULL)
			return;
		/* Force slow lookups: */
		PyDict_SetItem(interned, Py_None, Py_None);
	}
	if ((t = PyDict_GetItem(interned, (PyObject *)s)) != NULL) {
		Py_INCREF(t);
		*p = s->ob_sinterned = t;
		Py_DECREF(s);
		return;
	}
	t = (PyObject *)s;
	if (PyDict_SetItem(interned, t, t) == 0) {
		s->ob_sinterned = t;
		return;
	}
	PyErr_Clear();
}


PyObject *
PyString_InternFromString(cp)
	const char *cp;
{
	PyObject *s = PyString_FromString(cp);
	if (s == NULL)
		return NULL;
	PyString_InternInPlace(&s);
	return s;
}

#endif
