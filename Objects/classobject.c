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
	object	*cl_name;	/* A string */
} classobject;

object *
newclassobject(bases, methods, name)
	object *bases; /* NULL or tuple of classobjects! */
	object *methods;
	object *name; /* String; NULL if unknown */
{
	classobject *op;
	if (bases == NULL) {
		bases = newtupleobject(0);
		if (bases == NULL)
			return err_nomem();
	}
	else
		INCREF(bases);
	op = NEWOBJ(classobject, &Classtype);
	if (op == NULL) {
		DECREF(bases);
		return NULL;
	}
	op->cl_bases = bases;
	INCREF(methods);
	op->cl_methods = methods;
	XINCREF(name);
	op->cl_name = name;
	return (object *) op;
}

/* Class methods */

static void
class_dealloc(op)
	classobject *op;
{
	int i;
	DECREF(op->cl_bases);
	DECREF(op->cl_methods);
	XDECREF(op->cl_name);
	free((ANY *)op);
}

static object *
class_getattr(op, name)
	register classobject *op;
	register char *name;
{
	register object *v;
	if (strcmp(name, "__dict__") == 0) {
		INCREF(op->cl_methods);
		return op->cl_methods;
	}
	if (strcmp(name, "__bases__") == 0) {
		INCREF(op->cl_bases);
		return op->cl_bases;
	}
	if (strcmp(name, "__name__") == 0) {
		if (op->cl_name == NULL)
			v = None;
		else
			v = op->cl_name;
		INCREF(v);
		return v;
	}
	v = dictlookup(op->cl_methods, name);
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	{
		int n = gettuplesize(op->cl_bases);
		int i;
		for (i = 0; i < n; i++) {
			v = class_getattr(gettupleitem(op->cl_bases, i), name);
			if (v != NULL)
				return v;
			err_clear();
		}
	}
	err_setstr(AttributeError, name);
	return NULL;
}

static int
class_setattr(op, name, v)
	classobject *op;
	char *name;
	object *v;
{
	if (v == NULL)
		return dictremove(op->cl_methods, name);
	else
		return dictinsert(op->cl_methods, name, v);
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
	class_setattr,	/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};


/* We're not done yet: next, we define instance objects... */

typedef struct {
	OB_HEAD
	classobject	*in_class;	/* The class object */
	object		*in_attr;	/* A dictionary */
} instanceobject;

object *
newinstanceobject(class)
	register object *class;
{
	register instanceobject *inst;
	if (!is_classobject(class)) {
		err_badcall();
		return NULL;
	}
	inst = NEWOBJ(instanceobject, &Instancetype);
	if (inst == NULL)
		return NULL;
	INCREF(class);
	inst->in_class = (classobject *)class;
	inst->in_attr = newdictobject();
	if (inst->in_attr == NULL) {
		DECREF(inst);
		return NULL;
	}
	return (object *)inst;
}

/* Instance methods */

static void
instance_dealloc(inst)
	register instanceobject *inst;
{
	DECREF(inst->in_class);
	if (inst->in_attr != NULL)
		DECREF(inst->in_attr);
	free((ANY *)inst);
}

static object *
instance_getattr(inst, name)
	register instanceobject *inst;
	register char *name;
{
	register object *v;
	if (strcmp(name, "__dict__") == 0) {
		INCREF(inst->in_attr);
		return inst->in_attr;
	}
	if (strcmp(name, "__class__") == 0) {
		INCREF(inst->in_class);
		return (object *)inst->in_class;
	}
	v = dictlookup(inst->in_attr, name);
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	v = class_getattr(inst->in_class, name);
	if (v == NULL)
		return v; /* class_getattr() has set the error */
	if (is_funcobject(v)) {
		object *w = newinstancemethodobject(v, (object *)inst);
		DECREF(v);
		return w;
	}
	DECREF(v);
	err_setstr(AttributeError, name);
	return NULL;
}

static int
instance_setattr(inst, name, v)
	instanceobject *inst;
	char *name;
	object *v;
{
	if (v == NULL)
		return dictremove(inst->in_attr, name);
	else
		return dictinsert(inst->in_attr, name, v);
}

typeobject Instancetype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"instance",
	sizeof(instanceobject),
	0,
	instance_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	instance_getattr,	/*tp_getattr*/
	instance_setattr,	/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};


/* And finally, here are instance method objects */

typedef struct {
	OB_HEAD
	object	*im_func;	/* The method function */
	object	*im_self;	/* The object to which this applies */
} instancemethodobject;

object *
newinstancemethodobject(func, self)
	object *func;
	object *self;
{
	register instancemethodobject *im;
	if (!is_funcobject(func)) {
		err_badcall();
		return NULL;
	}
	im = NEWOBJ(instancemethodobject, &Instancemethodtype);
	if (im == NULL)
		return NULL;
	INCREF(func);
	im->im_func = func;
	INCREF(self);
	im->im_self = self;
	return (object *)im;
}

object *
instancemethodgetfunc(im)
	register object *im;
{
	if (!is_instancemethodobject(im)) {
		err_badcall();
		return NULL;
	}
	return ((instancemethodobject *)im)->im_func;
}

object *
instancemethodgetself(im)
	register object *im;
{
	if (!is_instancemethodobject(im)) {
		err_badcall();
		return NULL;
	}
	return ((instancemethodobject *)im)->im_self;
}

/* Class method methods */

#define OFF(x) offsetof(instancemethodobject, x)

static struct memberlist instancemethod_memberlist[] = {
	{"im_func",	T_OBJECT,	OFF(im_func)},
	{"im_self",	T_OBJECT,	OFF(im_self)},
	{NULL}	/* Sentinel */
};

static object *
instancemethod_getattr(im, name)
	register instancemethodobject *im;
	char *name;
{
	return getmember((char *)im, instancemethod_memberlist, name);
}

static void
instancemethod_dealloc(im)
	register instancemethodobject *im;
{
	DECREF(im->im_func);
	DECREF(im->im_self);
	free((ANY *)im);
}

typeobject Instancemethodtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"instance method",
	sizeof(instancemethodobject),
	0,
	instancemethod_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	instancemethod_getattr,	/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
};
