/* Class object implementation */

#include <stdio.h>

#include "PROTO.h"
#include "node.h"
#include "object.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "dictobject.h"
#include "funcobject.h"
#include "classobject.h"
#include "objimpl.h"

typedef struct {
	OB_HEAD
	node	*cl_tree;	/* The entire classdef parse tree */
	object	*cl_bases;	/* A tuple */
	object	*cl_methods;	/* A dictionary */
} classobject;

object *
newclassobject(tree, bases, methods)
	node *tree;
	object *bases; /* NULL or tuple of classobjects! */
	object *methods;
{
	classobject *op;
	op = NEWOBJ(classobject, &Classtype);
	if (op == NULL)
		return NULL;
	op->cl_tree = tree;
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
		}
	}
	errno = ESRCH;
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
		errno = EINVAL;
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
		return v; /* class_getattr() has set errno */
	if (is_funcobject(v)) {
		object *w = newclassmethodobject(v, (object *)cm);
		DECREF(v);
		return w;
	}
	DECREF(v);
	errno = ESRCH;
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
		errno = EINVAL;
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
		errno = EINVAL;
		return NULL;
	}
	return ((classmethodobject *)cm)->cm_func;
}

object *
classmethodgetself(cm)
	register object *cm;
{
	if (!is_classmethodobject(cm)) {
		errno = EINVAL;
		return NULL;
	}
	return ((classmethodobject *)cm)->cm_self;
}

/* Class method methods */

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
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};
