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

/* Tuple object implementation */

#include "allobjects.h"

object *
newtupleobject(size)
	register int size;
{
	register int i;
	register tupleobject *op;
	if (size < 0) {
		err_badcall();
		return NULL;
	}
	op = (tupleobject *)
		malloc(sizeof(tupleobject) + size * sizeof(object *));
	if (op == NULL)
		return err_nomem();
	NEWREF(op);
	op->ob_type = &Tupletype;
	op->ob_size = size;
	for (i = 0; i < size; i++)
		op->ob_item[i] = NULL;
	return (object *) op;
}

int
gettuplesize(op)
	register object *op;
{
	if (!is_tupleobject(op)) {
		err_badcall();
		return -1;
	}
	else
		return ((tupleobject *)op)->ob_size;
}

object *
gettupleitem(op, i)
	register object *op;
	register int i;
{
	if (!is_tupleobject(op)) {
		err_badcall();
		return NULL;
	}
	if (i < 0 || i >= ((tupleobject *)op) -> ob_size) {
		err_setstr(IndexError, "tuple index out of range");
		return NULL;
	}
	return ((tupleobject *)op) -> ob_item[i];
}

int
settupleitem(op, i, newitem)
	register object *op;
	register int i;
	register object *newitem;
{
	register object *olditem;
	if (!is_tupleobject(op)) {
		if (newitem != NULL)
			DECREF(newitem);
		err_badcall();
		return -1;
	}
	if (i < 0 || i >= ((tupleobject *)op) -> ob_size) {
		if (newitem != NULL)
			DECREF(newitem);
		err_setstr(IndexError, "tuple assignment index out of range");
		return -1;
	}
	olditem = ((tupleobject *)op) -> ob_item[i];
	((tupleobject *)op) -> ob_item[i] = newitem;
	if (olditem != NULL)
		DECREF(olditem);
	return 0;
}

/* Methods */

static void
tupledealloc(op)
	register tupleobject *op;
{
	register int i;
	for (i = 0; i < op->ob_size; i++) {
		if (op->ob_item[i] != NULL)
			DECREF(op->ob_item[i]);
	}
	free((ANY *)op);
}

static void
tupleprint(op, fp, flags)
	tupleobject *op;
	FILE *fp;
	int flags;
{
	int i;
	fprintf(fp, "(");
	for (i = 0; i < op->ob_size && !StopPrint; i++) {
		if (i > 0) {
			fprintf(fp, ", ");
		}
		printobject(op->ob_item[i], fp, flags);
	}
	if (op->ob_size == 1)
		fprintf(fp, ",");
	fprintf(fp, ")");
}

object *
tuplerepr(v)
	tupleobject *v;
{
	object *s, *t, *comma;
	int i;
	s = newstringobject("(");
	comma = newstringobject(", ");
	for (i = 0; i < v->ob_size && s != NULL; i++) {
		if (i > 0)
			joinstring(&s, comma);
		t = reprobject(v->ob_item[i]);
		joinstring(&s, t);
		if (t != NULL)
			DECREF(t);
	}
	DECREF(comma);
	if (v->ob_size == 1) {
		t = newstringobject(",");
		joinstring(&s, t);
		DECREF(t);
	}
	t = newstringobject(")");
	joinstring(&s, t);
	DECREF(t);
	return s;
}

static int
tuplecompare(v, w)
	register tupleobject *v, *w;
{
	register int len =
		(v->ob_size < w->ob_size) ? v->ob_size : w->ob_size;
	register int i;
	for (i = 0; i < len; i++) {
		int cmp = cmpobject(v->ob_item[i], w->ob_item[i]);
		if (cmp != 0)
			return cmp;
	}
	return v->ob_size - w->ob_size;
}

static int
tuplelength(a)
	tupleobject *a;
{
	return a->ob_size;
}

static object *
tupleitem(a, i)
	register tupleobject *a;
	register int i;
{
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "tuple index out of range");
		return NULL;
	}
	INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static object *
tupleslice(a, ilow, ihigh)
	register tupleobject *a;
	register int ilow, ihigh;
{
	register tupleobject *np;
	register int i;
	if (ilow < 0)
		ilow = 0;
	if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	if (ihigh < ilow)
		ihigh = ilow;
	if (ilow == 0 && ihigh == a->ob_size) {
		/* XXX can only do this if tuples are immutable! */
		INCREF(a);
		return (object *)a;
	}
	np = (tupleobject *)newtupleobject(ihigh - ilow);
	if (np == NULL)
		return NULL;
	for (i = ilow; i < ihigh; i++) {
		object *v = a->ob_item[i];
		INCREF(v);
		np->ob_item[i - ilow] = v;
	}
	return (object *)np;
}

static object *
tupleconcat(a, bb)
	register tupleobject *a;
	register object *bb;
{
	register int size;
	register int i;
	tupleobject *np;
	if (!is_tupleobject(bb)) {
		err_badarg();
		return NULL;
	}
#define b ((tupleobject *)bb)
	size = a->ob_size + b->ob_size;
	np = (tupleobject *) newtupleobject(size);
	if (np == NULL) {
		return err_nomem();
	}
	for (i = 0; i < a->ob_size; i++) {
		object *v = a->ob_item[i];
		INCREF(v);
		np->ob_item[i] = v;
	}
	for (i = 0; i < b->ob_size; i++) {
		object *v = b->ob_item[i];
		INCREF(v);
		np->ob_item[i + a->ob_size] = v;
	}
	return (object *)np;
#undef b
}

static sequence_methods tuple_as_sequence = {
	tuplelength,	/*sq_length*/
	tupleconcat,	/*sq_concat*/
	0,		/*sq_repeat*/
	tupleitem,	/*sq_item*/
	tupleslice,	/*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
};

typeobject Tupletype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"tuple",
	sizeof(tupleobject) - sizeof(object *),
	sizeof(object *),
	tupledealloc,	/*tp_dealloc*/
	tupleprint,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	tuplecompare,	/*tp_compare*/
	tuplerepr,	/*tp_repr*/
	0,		/*tp_as_number*/
	&tuple_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};
