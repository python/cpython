/* Object implementation; and 'noobject' implementation */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "stringobject.h"
#include "objimpl.h"
#include "errors.h"

extern object *err_nomem PROTO((void)); /* XXX from modsupport.c */

int StopPrint; /* Flag to indicate printing must be stopped */

/* Object allocation routines used by NEWOBJ and NEWVAROBJ macros */

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

static int prlevel;

void
printobject(op, fp, flags)
	object *op;
	FILE *fp;
	int flags;
{
	/* Hacks to make printing a long or recursive object interruptible */
	prlevel++;
	if (!StopPrint && intrcheck()) {
		fprintf(fp, "\n[print interrupted]\n");
		StopPrint = 1;
	}
	if (!StopPrint) {
		if (op == NULL) {
			fprintf(fp, "<nil>");
		}
		else if (op->ob_type->tp_print == NULL) {
			fprintf(fp, "<%s object at %lx>",
				op->ob_type->tp_name, (long)op);
		}
		else {
			(*op->ob_type->tp_print)(op, fp, flags);
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
	object *w;
	/* Hacks to make converting a long or recursive object interruptible */
	prlevel++;
	if (!StopPrint && intrcheck()) {
		StopPrint = 1;
		w = NULL;
		err_set(KeyboardInterrupt);
	}
	if (!StopPrint) {
		if (v == NULL) {
			w = newstringobject("<nil>");
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


/*
NoObject is usable as a non-NULL undefined value, used by the macro None.
There is (and should be!) no way to create other objects of this type,
so there is exactly one.
*/

static void
noprint(op, fp, flags)
	object *op;
	FILE *fp;
	int flags;
{
	fprintf(fp, "<no value>");
}

static typeobject Notype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"novalue",
	0,
	0,
	0,		/*tp_dealloc*/ /*never called*/
	noprint,	/*tp_print*/
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

DELREF(op)
	object *op;
{
	op->_ob_next->_ob_prev = op->_ob_prev;
	op->_ob_prev->_ob_next = op->_ob_next;
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
