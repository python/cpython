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

/* Range object implementation */

#include "allobjects.h"

typedef struct {
	OB_HEAD
	long	start;
	long	step;
	long	len;
} rangeobject;


object *
newrangeobject(start, len, step)
	long start, len, step;
{
	rangeobject *obj = (rangeobject *) newobject(&Rangetype);

	obj->start = start;
	obj->len   = len;
	obj->step  = step;

	return (object *) obj;
}

static void
range_dealloc(r)
	rangeobject *r;
{
	DEL(r);
}

static object *
range_item(r, i)
	rangeobject *r;
	int i;
{
	if (i < 0 || i >= r->len) {
		err_setstr(IndexError, "range object index out of range");
		return NULL;
	}

	return newintobject(r->start + i * r->step);
}

static int
range_length(r)
	rangeobject *r;
{
	return r->len;
}

static object *
range_repr(r)
	rangeobject *r;
{
	char buf[80];
	sprintf(buf, "xrange(%ld, %ld, %ld)",
		r->start, r->start + r->len * r->step, r->step);
	return newstringobject(buf);
}

static int
range_compare(r1, r2)
	rangeobject *r1, *r2;
{
	if (r1->start != r2->start)
		return r1->start - r2->start;

	else if (r1->step != r2->step)
		return r1->step - r2->step;

	else if (r1->len != r2->len)
		return r1->len - r2->len;
}

static object *
range_concat(r, s)
	rangeobject *r;
	object *s;
{
	err_setstr(TypeError, "concat not supported by xrange object");
	return NULL;
}

static object *
range_repeat(r, n)
	rangeobject *r;
	int n;
{
	err_setstr(TypeError, "repeat not supported by xrange object");
	return NULL;
}

static object *
range_slice(r, i, j)
	rangeobject *r;
	int i, j;
{
	err_setstr(TypeError, "slice not supported by xrange object");
	return NULL;
}

static sequence_methods range_as_sequence = {
	range_length,	/*sq_length*/
	range_concat,	/*sq_concat*/
	range_repeat,	/*sq_repeat*/
	range_item,	/*sq_item*/
	range_slice,	/*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
};

typeobject Rangetype = {
	OB_HEAD_INIT(&Typetype)
	0,			/* Number of items for varobject */
	"xrange",		/* Name of this type */
	sizeof(rangeobject),	/* Basic object size */
	0,			/* Item size for varobject */
	range_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	range_compare,		/*tp_compare*/
	range_repr,		/*tp_repr*/
	0,			/*tp_as_number*/
	&range_as_sequence,	/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};
