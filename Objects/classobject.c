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

/* Class object implementation */

#include "allobjects.h"

#include "structmember.h"

typedef struct {
	OB_HEAD
	object	*cl_bases;	/* A tuple */
	object	*cl_methods;	/* A dictionary */
} classobject;

object *
newclassobject(bases, methods)
	object *bases; /* NULL or tuple of classobjects! */
	object *methods;
{
	classobject *op;
	op = NEWOBJ(classobject, &Classtype);
	if (op == NULL)
		return NULL;
	if (bases != NULL)
		INCREF(bases);
	op->cl_bases = bases;
	INCREF(methods);
	op->cl_methods = methods;
	return (object *) op;
}

/* Class methods */

static void
class_dealloc(op)
	classobject *op;
{
	int i;
	if (op->cl_bases != NULL)
		DECREF(op->cl_bases);
	DECREF(op->cl_methods);
	free((ANY *)op);
}

static object *
class_getattr(op, name)
	register classobject *op;
	register char *name;
{
	register object *v;
	v = dictlookup(op->cl_methods, name);
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	if (op->cl_bases != NULL) {
		int n = gettuplesize(op->cl_bases);
		int i;
		for (i = 0; i < n; i++) {
			v = class_getattr(gettupleitem(op->cl_bases, i), name);
			if (v != NULL)
				return v;
			err_clear();
		}
	}
	err_setstr(NameError, name);
	return NULL;
}

typeobject Classtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"class",
	sizeof(classobject),
	0,
	class_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	class_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};


/* We're not done yet: next, we define class member objects... */

typedef struct {
	OB_HEAD
	classobject	*cm_class;	/* The class object */
	object		*cm_attr;	/* A dictionary */
} classmemberobject;

object *
newclassmemberobject(class)
	register object *class;
{
	register classmemberobject *cm;
	if (!is_classobject(class)) {
		err_badcall();
		return NULL;
	}
	cm = NEWOBJ(classmemberobject, &Classmembertype);
	if (cm == NULL)
		return NULL;
	INCREF(class);
	cm->cm_class = (classobject *)class;
	cm->cm_attr = newdictobject();
	if (cm->cm_attr == NULL) {
		DECREF(cm);
		return NULL;
	}
	return (object *)cm;
}

/* Class member methods */

static void
classmember_dealloc(cm)
	register classmemberobject *cm;
{
	DECREF(cm->cm_class);
	if (cm->cm_attr != NULL)
		DECREF(cm->cm_attr);
	free((ANY *)cm);
}

static object *
classmember_getattr(cm, name)
	register classmemberobject *cm;
	register char *name;
{
	register object *v = dictlookup(cm->cm_attr, name);
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	v = class_getattr(cm->cm_class, name);
	if (v == NULL)
		return v; /* class_getattr() has set the error */
	if (is_funcobject(v)) {
		object *w = newclassmethodobject(v, (object *)cm);
		DECREF(v);
		return w;
	}
	DECREF(v);
	err_setstr(NameError, name);
	return NULL;
}

static int
classmember_setattr(cm, name, v)
	classmemberobject *cm;
	char *name;
	object *v;
{
	if (v == NULL)
		return dictremove(cm->cm_attr, name);
	else
		return dictinsert(cm->cm_attr, name, v);
}

typeobject Classmembertype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"class member",
	sizeof(classmemberobject),
	0,
	classmember_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	classmember_getattr,	/*tp_getattr*/
	classmember_setattr,	/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};


/* And finally, here are class method objects */
/* (Really methods of class members) */

typedef struct {
	OB_HEAD
	object	*cm_func;	/* The method function */
	object	*cm_self;	/* The object to which this applies */
} classmethodobject;

object *
newclassmethodobject(func, self)
	object *func;
	object *self;
{
	register classmethodobject *cm;
	if (!is_funcobject(func)) {
		err_badcall();
		return NULL;
	}
	cm = NEWOBJ(classmethodobject, &Classmethodtype);
	if (cm == NULL)
		return NULL;
	INCREF(func);
	cm->cm_func = func;
	INCREF(self);
	cm->cm_self = self;
	return (object *)cm;
}

object *
classmethodgetfunc(cm)
	register object *cm;
{
	if (!is_classmethodobject(cm)) {
		err_badcall();
		return NULL;
	}
	return ((classmethodobject *)cm)->cm_func;
}

object *
classmethodgetself(cm)
	register object *cm;
{
	if (!is_classmethodobject(cm)) {
		err_badcall();
		return NULL;
	}
	return ((classmethodobject *)cm)->cm_self;
}

/* Class method methods */

#define OFF(x) offsetof(classmethodobject, x)

static struct memberlist classmethod_memberlist[] = {
	{"cm_func",	T_OBJECT,	OFF(cm_func)},
	{"cm_self",	T_OBJECT,	OFF(cm_self)},
	{NULL}	/* Sentinel */
};

static object *
classmethod_getattr(cm, name)
	register classmethodobject *cm;
	char *name;
{
	return getmember((char *)cm, classmethod_memberlist, name);
}

static void
classmethod_dealloc(cm)
	register classmethodobject *cm;
{
	DECREF(cm->cm_func);
	DECREF(cm->cm_self);
	free((ANY *)cm);
}

typeobject Classmethodtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"class method",
	sizeof(classmethodobject),
	0,
	classmethod_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	classmethod_getattr,	/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};
