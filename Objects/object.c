/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* Generic object operations; and implementation of None (NoObject) */

#include "allobjects.h"

#ifdef REF_DEBUG
long ref_total;
#endif

/* Object allocation routines used by NEWOBJ and NEWVAROBJ macros.
   These are used by the individual routines for object creation.
   Do not call them otherwise, they do not initialize the object! */

#ifdef COUNT_ALLOCS
static typeobject *type_list;
extern int tuple_zero_allocs, fast_tuple_allocs;
extern int quick_int_allocs, quick_neg_int_allocs;
extern int null_strings, one_strings;
void
dump_counts()
{
	typeobject *tp;

	for (tp = type_list; tp; tp = tp->tp_next)
		fprintf(stderr, "%s alloc'd: %d, freed: %d, max in use: %d\n",
			tp->tp_name, tp->tp_alloc, tp->tp_free,
			tp->tp_maxalloc);
	fprintf(stderr, "fast tuple allocs: %d, empty: %d\n",
		fast_tuple_allocs, tuple_zero_allocs);
	fprintf(stderr, "fast int allocs: pos: %d, neg: %d\n",
		quick_int_allocs, quick_neg_int_allocs);
	fprintf(stderr, "null strings: %d, 1-strings: %d\n",
		null_strings, one_strings);
}

void
inc_count(tp)
	typeobject *tp;
{
	if (tp->tp_alloc == 0) {
		/* first time; hang in linked list */
		if (tp->tp_next != NULL) /* sanity check */
			abort();
		tp->tp_next = type_list;
		type_list = tp;
	}
	tp->tp_alloc++;
	if (tp->tp_alloc - tp->tp_free > tp->tp_maxalloc)
		tp->tp_maxalloc = tp->tp_alloc - tp->tp_free;
}
#endif

object *
newobject(tp)
	typeobject *tp;
{
	object *op = (object *) malloc(tp->tp_basicsize);
	if (op == NULL)
		return err_nomem();
	op->ob_type = tp;
	NEWREF(op);
	return op;
}

varobject *
newvarobject(tp, size)
	typeobject *tp;
	unsigned int size;
{
	varobject *op = (varobject *)
		malloc(tp->tp_basicsize + size * tp->tp_itemsize);
	if (op == NULL)
		return (varobject *)err_nomem();
	op->ob_type = tp;
	op->ob_size = size;
	NEWREF(op);
	return op;
}

int
printobject(op, fp, flags)
	object *op;
	FILE *fp;
	int flags;
{
	int ret = 0;
	if (sigcheck())
		return -1;
	if (op == NULL) {
		fprintf(fp, "<nil>");
	}
	else {
		if (op->ob_refcnt <= 0)
			fprintf(fp, "<refcnt %u at %lx>",
				op->ob_refcnt, (long)op);
		else if (op->ob_type->tp_print == NULL) {
			if (op->ob_type->tp_repr == NULL) {
				fprintf(fp, "<%s object at %lx>",
					op->ob_type->tp_name, (long)op);
			}
			else {
				object *s;
				if (flags & PRINT_RAW)
					s = strobject(op);
				else
					s = reprobject(op);
				if (s == NULL)
					ret = -1;
				else if (!is_stringobject(s)) {
					err_setstr(TypeError,
						   "repr not string");
					ret = -1;
				}
				else {
					fprintf(fp, "%s", getstringvalue(s));
				}
				XDECREF(s);
			}
		}
		else
			ret = (*op->ob_type->tp_print)(op, fp, flags);
	}
	if (ret == 0) {
		if (ferror(fp)) {
			err_errno(IOError);
			clearerr(fp);
			ret = -1;
		}
	}
	return ret;
}

object *
reprobject(v)
	object *v;
{
	if (sigcheck())
		return NULL;
	if (v == NULL)
		return newstringobject("<NULL>");
	else if (v->ob_type->tp_repr == NULL) {
		char buf[120];
		sprintf(buf, "<%.80s object at %lx>",
			v->ob_type->tp_name, (long)v);
		return newstringobject(buf);
	}
	else
		return (*v->ob_type->tp_repr)(v);
}

object *
strobject(v)
	object *v;
{
	if (v == NULL)
		return newstringobject("<NULL>");
	if (is_stringobject(v)) {
		INCREF(v);
		return v;
	}
	else {
		object *func = getattr(v, "__str__");
		object *args;
		object *res;
		if (func == NULL) {
			err_clear();
			return reprobject(v);
		}
		args = newtupleobject(0);
		if (args == NULL)
			res = NULL;
		else {
			res = call_object(func, args);
			DECREF(args);
		}
		DECREF(func);
		return res;
	}
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
	if ((tp = v->ob_type) != w->ob_type) {
		if (tp->tp_as_number != NULL &&
				w->ob_type->tp_as_number != NULL) {
			if (coerce(&v, &w) != 0) {
				err_clear();
				/* XXX Should report the error,
				   XXX but the interface isn't there... */
			}
			else {
				int cmp = (*v->ob_type->tp_compare)(v, w);
				DECREF(v);
				DECREF(w);
				return cmp;
			}
		}
		return strcmp(tp->tp_name, w->ob_type->tp_name);
	}
	if (tp->tp_compare == NULL)
		return (v < w) ? -1 : 1;
	return (*tp->tp_compare)(v, w);
}

long
hashobject(v)
	object *v;
{
	typeobject *tp = v->ob_type;
	if (tp->tp_hash != NULL)
		return (*tp->tp_hash)(v);
	if (tp->tp_compare == NULL)
		return (long) v; /* Use address as hash value */
	/* If there's a cmp but no hash defined, the object can't be hashed */
	err_setstr(TypeError, "unhashable type");
	return -1;
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
hasattr(v, name)
	object *v;
	char *name;
{
	object *res = getattr(v, name);
	if (res != NULL) {
		DECREF(res);
		return 1;
	}
	err_clear();
	return 0;
}

int
setattr(v, name, w)
	object *v;
	char *name;
	object *w;
{
	if (v->ob_type->tp_setattr == NULL) {
		if (v->ob_type->tp_getattr == NULL)
			err_setstr(TypeError,
				   "attribute-less object (assign or del)");
		else
			err_setstr(TypeError,
				   "object has read-only attributes");
		return -1;
	}
	else {
		return (*v->ob_type->tp_setattr)(v, name, w);
	}
}

/* Test a value used as condition, e.g., in a for or if statement.
   Return -1 if an error occurred */

int
testbool(v)
	object *v;
{
	int res;
	if (v == None)
		res = 0;
	else if (v->ob_type->tp_as_number != NULL)
		res = (*v->ob_type->tp_as_number->nb_nonzero)(v);
	else if (v->ob_type->tp_as_mapping != NULL)
		res = (*v->ob_type->tp_as_mapping->mp_length)(v);
	else if (v->ob_type->tp_as_sequence != NULL)
		res = (*v->ob_type->tp_as_sequence->sq_length)(v);
	else
		res = 1;
	if (res > 0)
		res = 1;
	return res;
}


/*
NoObject is usable as a non-NULL undefined value, used by the macro None.
There is (and should be!) no way to create other objects of this type,
so there is exactly one (which is indestructible, by the way).
*/

/* ARGSUSED */
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
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)none_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/*tp_hash */
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
#ifdef COUNT_ALLOCS
	inc_count(op->ob_type);
#endif
}

UNREF(op)
	register object *op;
{
	register object *p;
	if (op->ob_refcnt < 0) {
		fprintf(stderr, "UNREF negative refcnt\n");
		abort();
	}
	if (op == &refchain ||
	    op->_ob_prev->_ob_next != op || op->_ob_next->_ob_prev != op) {
		fprintf(stderr, "UNREF invalid object\n");
		abort();
	}
#ifdef SLOW_UNREF_CHECK
	for (p = refchain._ob_next; p != &refchain; p = p->_ob_next) {
		if (p == op)
			break;
	}
	if (p == &refchain) { /* Not found */
		fprintf(stderr, "UNREF unknown object\n");
		abort();
	}
#endif
	op->_ob_next->_ob_prev = op->_ob_prev;
	op->_ob_prev->_ob_next = op->_ob_next;
	op->_ob_next = op->_ob_prev = NULL;
}

DELREF(op)
	object *op;
{
	destructor dealloc = op->ob_type->tp_dealloc;
	UNREF(op);
#ifdef COUNT_ALLOCS
	op->ob_type->tp_free++;
#endif
	op->ob_type = NULL;
	(*dealloc)(op);
}

printrefs(fp)
	FILE *fp;
{
	object *op;
	fprintf(fp, "Remaining objects (except strings referenced once):\n");
	for (op = refchain._ob_next; op != &refchain; op = op->_ob_next) {
		if (op->ob_refcnt == 1 && is_stringobject(op))
			continue; /* Will be printed elsewhere */
		fprintf(fp, "[%d] ", op->ob_refcnt);
		if (printobject(op, fp, 0) != 0)
			err_clear();
		putc('\n', fp);
	}
}

#endif
