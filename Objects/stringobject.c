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

/* String object implementation */

#include "allobjects.h"

object *
newsizedstringobject(str, size)
	char *str;
	int size;
{
	register stringobject *op = (stringobject *)
		malloc(sizeof(stringobject) + size * sizeof(char));
	if (op == NULL)
		return err_nomem();
	NEWREF(op);
	op->ob_type = &Stringtype;
	op->ob_size = size;
	if (str != NULL)
		memcpy(op->ob_sval, str, size);
	op->ob_sval[size] = '\0';
	return (object *) op;
}

object *
newstringobject(str)
	char *str;
{
	register unsigned int size = strlen(str);
	register stringobject *op = (stringobject *)
		malloc(sizeof(stringobject) + size * sizeof(char));
	if (op == NULL)
		return err_nomem();
	NEWREF(op);
	op->ob_type = &Stringtype;
	op->ob_size = size;
	strcpy(op->ob_sval, str);
	return (object *) op;
}

void
string_dealloc(op)
	object *op;
{
	DEL(op);
}

unsigned int
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
	/* XXX Ought to check for interrupts when writing long strings */
	if (flags & PRINT_RAW) {
		fwrite(op->ob_sval, 1, (int) op->ob_size, fp);
		return 0;
	}
	fprintf(fp, "'");
	for (i = 0; i < op->ob_size; i++) {
		c = op->ob_sval[i];
		if (c == '\'' || c == '\\')
			fprintf(fp, "\\%c", c);
		else if (c < ' ' || c >= 0177)
			fprintf(fp, "\\%03o", c&0377);
		else
			putc(c, fp);
	}
	fprintf(fp, "'");
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
		p = ((stringobject *)v)->ob_sval;
		*p++ = '\'';
		for (i = 0; i < op->ob_size; i++) {
			c = op->ob_sval[i];
			if (c == '\'' || c == '\\')
				*p++ = '\\', *p++ = c;
			else if (c < ' ' || c >= 0177) {
				sprintf(p, "\\%03o", c&0377);
				while (*p != '\0')
					p++;
				
			}
			else
				*p++ = c;
		}
		*p++ = '\'';
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
	NEWREF(op);
	op->ob_type = &Stringtype;
	op->ob_size = size;
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
	NEWREF(op);
	op->ob_type = &Stringtype;
	op->ob_size = size;
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

#ifdef __STDC__
#include <limits.h>
#else
#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif
#endif

static object *characters[UCHAR_MAX + 1];

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
	v = characters[c];
	if (v == NULL) {
		v = newsizedstringobject((char *)NULL, 1);
		if (v == NULL)
			return NULL;
		characters[c] = v;
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
	int cmp = memcmp(a->ob_sval, b->ob_sval, min_len);
	if (cmp != 0)
		return cmp;
	return (len_a < len_b) ? -1 : (len_a > len_b) ? 1 : 0;
}

static long
string_hash(a)
	stringobject *a;
{
	register int len = a->ob_size;
	register unsigned char *p = (unsigned char *) a->ob_sval;
	register long x = *p << 7;
	while (--len >= 0)
		x = (x + x + x) ^ *p++;
	x ^= a->ob_size;
	if (x == -1)
		x = -2;
	return x;
}

static sequence_methods string_as_sequence = {
	string_length,	/*sq_length*/
	string_concat,	/*sq_concat*/
	string_repeat,	/*sq_repeat*/
	string_item,	/*sq_item*/
	string_slice,	/*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
};

typeobject Stringtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"string",
	sizeof(stringobject),
	sizeof(char),
	string_dealloc,	/*tp_dealloc*/
	string_print,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	string_compare,	/*tp_compare*/
	string_repr,	/*tp_repr*/
	0,		/*tp_as_number*/
	&string_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	string_hash,	/*tp_hash*/
};

void
joinstring(pv, w)
	register object **pv;
	register object *w;
{
	register object *v;
	if (*pv == NULL || w == NULL || !is_stringobject(*pv))
		return;
	v = string_concat((stringobject *) *pv, w);
	DECREF(*pv);
	*pv = v;
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
#ifdef REF_DEBUG
	--ref_total;
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

static char *
formatfloat(flags, prec, type, v)
	int flags;
	int prec;
	int type;
	object *v;
{
	char fmt[20];
	static char buf[120];
	double x;
	if (!getargs(v, "d;float argument required", &x))
		return NULL;
	if (prec < 0)
		prec = 6;
	if (prec > 50)
		prec = 50; /* Arbitrary limitation */
	if (type == 'f' && fabs(x)/1e25 >= 1e25)
		type = 'g';
	sprintf(fmt, "%%%s.%d%c", (flags&F_ALT) ? "#" : "", prec, type);
	sprintf(buf, fmt, x);
	return buf;
}

static char *
formatint(flags, prec, type, v)
	int flags;
	int prec;
	int type;
	object *v;
{
	char fmt[20];
	static char buf[50];
	long x;
	if (!getargs(v, "l;int argument required", &x))
		return NULL;
	if (prec < 0)
		prec = 1;
	sprintf(fmt, "%%%s.%dl%c", (flags&F_ALT) ? "#" : "", prec, type);
	sprintf(buf, fmt, x);
	return buf;
}

static char *
formatchar(v)
	object *v;
{
	static char buf[2];
	if (is_stringobject(v)) {
		if (!getargs(v, "c;%c requires int or char", &buf[0]))
			return NULL;
	}
	else {
		if (!getargs(v, "b;%c requires int or char", &buf[0]))
			return NULL;
	}
	buf[1] = '\0';
	return buf;
}

/* fmt%(v1,v2,...) is roughly equivalent to sprintf(fmt, v1, v2, ...) */

object *
formatstring(format, args)
	object *format;
	object *args;
{
	char *fmt, *res;
	int fmtcnt, rescnt, reslen, arglen, argidx;
	object *result;
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
	while (--fmtcnt >= 0) {
		if (*fmt != '%') {
			if (--rescnt < 0) {
				rescnt = fmtcnt + 100;
				reslen += rescnt;
				if (resizestring(&result, reslen) < 0)
					return NULL;
				res = getstringvalue(result) + reslen - rescnt;
			}
			*res++ = *fmt++;
		}
		else {
			/* Got a format specifier */
			int flags = 0;
			char *fmtstart = fmt++;
			int width = -1;
			int prec = -1;
			int size = 0;
			int c;
			int fill;
			object *v;
			char *buf;
			int sign;
			int len;
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
			else if (isdigit(c)) {
				width = c - '0';
				while (--fmtcnt >= 0) {
					c = *fmt++;
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
				else if (isdigit(c)) {
					prec = c - '0';
					while (--fmtcnt >= 0) {
						c = *fmt++;
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
				if (!is_stringobject(v)) {
					err_setstr(TypeError,
						   "%s wants string");
					goto error;
				}
				buf = getstringvalue(v);
				len = getstringsize(v);
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
				buf = formatint(flags, prec, c, v);
				if (buf == NULL)
					goto error;
				len = strlen(buf);
				sign = (c == 'd');
				if (flags&F_ZERO)
					fill = '0';
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				buf = formatfloat(flags, prec, c, v);
				if (buf == NULL)
					goto error;
				len = strlen(buf);
				sign = 1;
				if (flags&F_ZERO)
					fill = '0';
				break;
			case 'c':
				buf = formatchar(v);
				if (buf == NULL)
					goto error;
				len = strlen(buf);
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
			memcpy(res, buf, len);
			res += len;
			rescnt -= len;
			while (--width >= len) {
				--rescnt;
				*res++ = ' ';
			}
		} /* '%' */
	} /* until end */
	if (argidx < arglen) {
		err_setstr(TypeError, "not all arguments converted");
		goto error;
	}
	resizestring(&result, reslen - rescnt);
	return result;
 error:
	DECREF(result);
	return NULL;
}
