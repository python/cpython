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

/* Method object implementation */

#include "allobjects.h"

#include "token.h"

typedef struct {
	OB_HEAD
	char *m_name;
	method m_meth;
	object *m_self;
} methodobject;

object *
newmethodobject(name, meth, self)
	char *name; /* static string */
	method meth;
	object *self;
{
	methodobject *op = NEWOBJ(methodobject, &Methodtype);
	if (op != NULL) {
		op->m_name = name;
		op->m_meth = meth;
		if (self != NULL)
			INCREF(self);
		op->m_self = self;
	}
	return (object *)op;
}

method
getmethod(op)
	object *op;
{
	if (!is_methodobject(op)) {
		err_badcall();
		return NULL;
	}
	return ((methodobject *)op) -> m_meth;
}

object *
getself(op)
	object *op;
{
	if (!is_methodobject(op)) {
		err_badcall();
		return NULL;
	}
	return ((methodobject *)op) -> m_self;
}

/* Methods (the standard built-in methods, that is) */

static void
meth_dealloc(m)
	methodobject *m;
{
	if (m->m_self != NULL)
		DECREF(m->m_self);
	free((char *)m);
}

static void
meth_print(m, fp, flags)
	methodobject *m;
	FILE *fp;
	int flags;
{
	if (m->m_self == NULL)
		fprintf(fp, "<built-in function '%s'>", m->m_name);
	else
		fprintf(fp, "<built-in method '%s' of some %s object>",
			m->m_name, m->m_self->ob_type->tp_name);
}

static object *
meth_repr(m)
	methodobject *m;
{
	char buf[200];
	if (m->m_self == NULL)
		sprintf(buf, "<built-in function '%.80s'>", m->m_name);
	else
		sprintf(buf,
			"<built-in method '%.80s' of some %.80s object>",
			m->m_name, m->m_self->ob_type->tp_name);
	return newstringobject(buf);
}

typeobject Methodtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"method",
	sizeof(methodobject),
	0,
	meth_dealloc,	/*tp_dealloc*/
	meth_print,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	meth_repr,	/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

/* Find a method in a module's method table.
   Usually called from an object's getattr method. */

object *
findmethod(ml, op, name)
	struct methodlist *ml;
	object *op;
	char *name;
{
	for (; ml->ml_name != NULL; ml++) {
		if (strcmp(name, ml->ml_name) == 0)
			return newmethodobject(ml->ml_name, ml->ml_meth, op);
	}
	err_setstr(NameError, name);
	return NULL;
}
