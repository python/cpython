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

/* Generic object operations; and implementation of None (NoObject) */

#include "allobjects.h"

#ifdef REF_DEBUG
long ref_total;
#endif

/* Object allocation routines used by NEWOBJ and NEWVAROBJ macros.
   These are used by the individual routines for object creation.
   Do not call them otherwise, they do not initialize the object! */

object *
newobject(tp)
	typeobject *tp;
{
	object *op = (object *) malloc(tp->tp_basicsize);
	if (op == NULL)
		return err_nomem();
	NEWREF(op);
	op->ob_type = tp;
	return op;
}

#if 0 /* unused */

varobject *
newvarobject(tp, size)
	typeobject *tp;
	unsigned int size;
{
	varobject *op = (varobject *)
		malloc(tp->tp_basicsize + size * tp->tp_itemsize);
	if (op == NULL)
		return err_nomem();
	NEWREF(op);
	op->ob_type = tp;
	op->ob_size = size;
	return op;
}

#endif

int StopPrint; /* Flag to indicate printing must be stopped */

static int prlevel;

void
printobject(op, fp, flags)
	object *op;
	FILE *fp;
	int flags;
{
	/* Hacks to make printing a long or recursive object interruptible */
	/* XXX Interrupts should leave a more permanent error */
	prlevel++;
	if (!StopPrint && intrcheck()) {
		fprintf(fp, "\n[print interrupted]\n");
		StopPrint = 1;
	}
	if (!StopPrint) {
		if (op == NULL) {
			fprintf(fp, "<nil>");
		}
		else {
			if (op->ob_refcnt <= 0)
				fprintf(fp, "(refcnt %d):", op->ob_refcnt);
			if (op->ob_type->tp_print == NULL) {
				fprintf(fp, "<%s object at %lx>",
					op->ob_type->tp_name, (long)op);
			}
			else {
				(*op->ob_type->tp_print)(op, fp, flags);
			}
		}
	}
	prlevel--;
	if (prlevel == 0)
		StopPrint = 0;
}

object *
reprobject(v)
	object *v;
{
	object *w = NULL;
	/* Hacks to make converting a long or recursive object interruptible */
	prlevel++;
	if (!StopPrint && intrcheck()) {
		StopPrint = 1;
		err_set(KeyboardInterrupt);
	}
	if (!StopPrint) {
		if (v == NULL) {
			w = newstringobject("<NULL>");
		}
		else if (v->ob_type->tp_repr == NULL) {
			char buf[100];
			sprintf(buf, "<%.80s object at %lx>",
				v->ob_type->tp_name, (long)v);
			w = newstringobject(buf);
		}
		else {
			w = (*v->ob_type->tp_repr)(v);
		}
		if (StopPrint) {
			XDECREF(w);
			w = NULL;
		}
	}
	prlevel--;
	if (prlevel == 0)
		StopPrint = 0;
	return w;
}

int
cmpobject(v, w)
	object *v, *w;
{
	typeobject *tp;
	if (v == w)
		return 0;
	if (v == NULL)
		return -1;
	if (w == NULL)
		return 1;
	if ((tp = v->ob_type) != w->ob_type)
		return strcmp(tp->tp_name, w->ob_type->tp_name);
	if (tp->tp_compare == NULL)
		return (v < w) ? -1 : 1;
	return ((*tp->tp_compare)(v, w));
}

object *
getattr(v, name)
	object *v;
	char *name;
{
	if (v->ob_type->tp_getattr == NULL) {
		err_setstr(TypeError, "attribute-less object");
		return NULL;
	}
	else {
		return (*v->ob_type->tp_getattr)(v, name);
	}
}

int
setattr(v, name, w)
	object *v;
	char *name;
	object *w;
{
	if (v->ob_type->tp_setattr == NULL) {
		if (v->ob_type->tp_getattr == NULL)
			err_setstr(TypeError, "attribute-less object");
		else
			err_setstr(TypeError, "object has read-only attributes");
		return -1;
	}
	else {
		return (*v->ob_type->tp_setattr)(v, name, w);
	}
}


/*
NoObject is usable as a non-NULL undefined value, used by the macro None.
There is (and should be!) no way to create other objects of this type,
so there is exactly one (which is indestructible, by the way).
*/

static void
none_print(op, fp, flags)
	object *op;
	FILE *fp;
	int flags;
{
	fprintf(fp, "None");
}

static object *
none_repr(op)
	object *op;
{
	return newstringobject("None");
}

static typeobject Notype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"None",
	0,
	0,
	0,		/*tp_dealloc*/ /*never called*/
	none_print,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	none_repr,	/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

object NoObject = {
	OB_HEAD_INIT(&Notype)
};


#ifdef TRACE_REFS

static object refchain = {&refchain, &refchain};

NEWREF(op)
	object *op;
{
	ref_total++;
	op->ob_refcnt = 1;
	op->_ob_next = refchain._ob_next;
	op->_ob_prev = &refchain;
	refchain._ob_next->_ob_prev = op;
	refchain._ob_next = op;
}

UNREF(op)
	register object *op;
{
	register object *p;
	if (op->ob_refcnt < 0) {
		fprintf(stderr, "UNREF negative refcnt\n");
		abort();
	}
	for (p = refchain._ob_next; p != &refchain; p = p->_ob_next) {
		if (p == op)
			break;
	}
	if (p == &refchain) { /* Not found */
		fprintf(stderr, "UNREF unknown object\n");
		abort();
	}
	op->_ob_next->_ob_prev = op->_ob_prev;
	op->_ob_prev->_ob_next = op->_ob_next;
}

DELREF(op)
	object *op;
{
	UNREF(op);
	(*(op)->ob_type->tp_dealloc)(op);
}

printrefs(fp)
	FILE *fp;
{
	object *op;
	fprintf(fp, "Remaining objects:\n");
	for (op = refchain._ob_next; op != &refchain; op = op->_ob_next) {
		fprintf(fp, "[%d] ", op->ob_refcnt);
		printobject(op, fp, 0);
		putc('\n', fp);
	}
}

#endif
