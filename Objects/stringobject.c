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

static void
stringprint(op, fp, flags)
	stringobject *op;
	FILE *fp;
	int flags;
{
	int i;
	char c;
	if (flags & PRINT_RAW) {
		fwrite(op->ob_sval, 1, (int) op->ob_size, fp);
		return;
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
}

static object *
stringrepr(op)
	register stringobject *op;
{
	/* XXX overflow? */
	int newsize = 2 + 4 * op->ob_size * sizeof(char);
	object *v = newsizedstringobject((char *)NULL, newsize);
	if (v == NULL) {
		return err_nomem();
	}
	else {
		register int i;
		register char c;
		register char *p;
		NEWREF(v);
		v->ob_type = &Stringtype;
		((stringobject *)v)->ob_size = newsize;
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
stringlength(a)
	stringobject *a;
{
	return a->ob_size;
}

static object *
stringconcat(a, bb)
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
stringrepeat(a, n)
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
stringslice(a, i, j)
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
stringitem(a, i)
	stringobject *a;
	register int i;
{
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "string index out of range");
		return NULL;
	}
	return stringslice(a, i, i+1);
}

static int
stringcompare(a, b)
	stringobject *a, *b;
{
	int len_a = a->ob_size, len_b = b->ob_size;
	int min_len = (len_a < len_b) ? len_a : len_b;
	int cmp = memcmp(a->ob_sval, b->ob_sval, min_len);
	if (cmp != 0)
		return cmp;
	return (len_a < len_b) ? -1 : (len_a > len_b) ? 1 : 0;
}

static sequence_methods string_as_sequence = {
	stringlength,	/*tp_length*/
	stringconcat,	/*tp_concat*/
	stringrepeat,	/*tp_repeat*/
	stringitem,	/*tp_item*/
	stringslice,	/*tp_slice*/
	0,	/*tp_ass_item*/
	0,	/*tp_ass_slice*/
};

typeobject Stringtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"string",
	sizeof(stringobject),
	sizeof(char),
	free,		/*tp_dealloc*/
	stringprint,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	stringcompare,	/*tp_compare*/
	stringrepr,	/*tp_repr*/
	0,		/*tp_as_number*/
	&string_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

void
joinstring(pv, w)
	register object **pv;
	register object *w;
{
	register object *v;
	if (*pv == NULL || w == NULL || !is_stringobject(*pv))
		return;
	v = stringconcat((stringobject *) *pv, w);
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
