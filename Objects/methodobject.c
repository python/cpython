/* Method object implementation */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "node.h"
#include "stringobject.h"
#include "methodobject.h"
#include "objimpl.h"
#include "token.h"
#include "errors.h"

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
		fprintf(fp, "<%s method>", m->m_name);
	else
		fprintf(fp, "<%s method of %s object at %lx>",
			m->m_name, m->m_self->ob_type->tp_name,
			(long)m->m_self);
}

static object *
meth_repr(m)
	methodobject *m;
{
	char buf[200];
	if (m->m_self == NULL)
		sprintf(buf, "<%.80s method>", m->m_name);
	else
		sprintf(buf, "<%.80s method of %.80s object at %lx>",
			m->m_name, m->m_self->ob_type->tp_name,
			(long)m->m_self);
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
