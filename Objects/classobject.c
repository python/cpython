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

/* Class object implementation */

#include "allobjects.h"
#include "modsupport.h"
#include "structmember.h"
#include "ceval.h"

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
			return NULL;
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
		if (is_accessobject(v))
			v = getaccessvalue(v, (object *)NULL);
		else
			INCREF(v);
		return v;
	}
	{
		int n = gettuplesize(op->cl_bases);
		int i;
		for (i = 0; i < n; i++) {
			v = class_getattr((classobject *)
					gettupleitem(op->cl_bases, i), name);
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
	object *ac;
	if (name[0] == '_' && name[1] == '_') {
		int n = strlen(name);
		if (name[n-1] == '_' && name[n-2] == '_') {
			err_setstr(TypeError, "read-only special attribute");
			return -1;
		}
	}
	ac = dictlookup(op->cl_methods, name);
	if (ac != NULL && is_accessobject(ac))
		return setaccessvalue(ac, (object *)NULL, v);
	if (v == NULL) {
		int rv = dictremove(op->cl_methods, name);
		if (rv < 0)
			err_setstr(AttributeError,
				   "delete non-existing class attribute");
		return rv;
	}
	else
		return dictinsert(op->cl_methods, name, v);
}

static object *
class_repr(op)
	classobject *op;
{
	char buf[140];
	char *name;
	if (op->cl_name == NULL || !is_stringobject(op->cl_name))
		name = "?";
	else
		name = getstringvalue(op->cl_name);
	sprintf(buf, "<class %.100s at %lx>", name, (long)op);
	return newstringobject(buf);
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
	class_repr,	/*tp_repr*/
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

static object *instance_getattr PROTO((instanceobject *, char *));

object *
newinstanceobject(class, arg)
	object *class;
	object *arg;
{
	register instanceobject *inst;
	object *v;
	object *init;
	int pos;
	object *key, *value;
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
	error:
		DECREF(inst);
		return NULL;
	}
	pos = 0;
	while (mappinggetnext(((classobject *)class)->cl_methods,
			      &pos, &key, &value)) {
		if (is_accessobject(value)) {
			object *ac = cloneaccessobject(value);
			int err;
			if (ac == NULL)
				goto error;
			err = dict2insert(inst->in_attr, key, ac);
			DECREF(ac);
			if (err)
				goto error;
		}
	}
	init = instance_getattr(inst, "__init__");
	if (init == NULL) {
		err_clear();
		if (arg != NULL && !(is_tupleobject(arg) &&
				     gettuplesize(arg) == 0)) {
			err_setstr(TypeError,
				"this classobject() takes no arguments");
			DECREF(inst);
			inst = NULL;
		}
	}
	else {
		object *res = call_object(init, arg);
		DECREF(init);
		if (res == NULL) {
			DECREF(inst);
			inst = NULL;
		}
		else {
			if (res != None) {
				err_setstr(TypeError,
					   "__init__() should return None");
				DECREF(inst);
				inst = NULL;
			}
			DECREF(res);
		}
	}
	return (object *)inst;
}

/* Instance methods */

static void
instance_dealloc(inst)
	register instanceobject *inst;
{
	object *error_type, *error_value;
	object *del;
	/* Call the __del__ method if it exists.  First temporarily
	   revive the object and save the current exception, if any. */
	INCREF(inst);
	err_get(&error_type, &error_value);
	if ((del = instance_getattr(inst, "__del__")) != NULL) {
		object *args = newtupleobject(0);
		object *res = args;
		if (res != NULL)
			res = call_object(del, args);
		XDECREF(args);
		DECREF(del);
		XDECREF(res);
		/* XXX If __del__ raised an exception, it is ignored! */
	}
	/* Restore the saved exception and undo the temporary revival */
	err_setval(error_type, error_value);
	/* Can't use DECREF here, it would cause a recursive call */
	if (--inst->ob_refcnt > 0)
		return; /* __del__ added a reference; don't delete now */
	DECREF(inst->in_class);
	XDECREF(inst->in_attr);
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
		if (is_accessobject(v))
			v = getaccessvalue(v, (object *)NULL);
		else
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
	object *ac;
	if (name[0] == '_' && name[1] == '_') {
		int n = strlen(name);
		if (name[n-1] == '_' && name[n-2] == '_') {
			err_setstr(TypeError, "read-only special attribute");
			return -1;
		}
	}
	ac = dictlookup(inst->in_attr, name);
	if (ac != NULL && is_accessobject(ac))
		return setaccessvalue(ac, (object *)NULL, v);
	if (v == NULL) {
		int rv = dictremove(inst->in_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
				   "delete non-existing instance attribute");
		return rv;
	}
	else
		return dictinsert(inst->in_attr, name, v);
}

static object *
instance_repr(inst)
	instanceobject *inst;
{
	object *func;
	object *res;

	func = instance_getattr(inst, "__repr__");
	if (func == NULL) {
		char buf[140];
		object *classname = inst->in_class->cl_name;
		char *cname;
		if (classname != NULL && is_stringobject(classname))
			cname = getstringvalue(classname);
		else
			cname = "?";
		err_clear();
		sprintf(buf, "<%.100s instance at %lx>", cname, (long)inst);
		return newstringobject(buf);
	}
	res = call_object(func, (object *)NULL);
	DECREF(func);
	return res;
}

static int
instance_compare(inst, other)
	instanceobject *inst, *other;
{
	object *func;
	object *res;
	int outcome;

	func = instance_getattr(inst, "__cmp__");
	if (func == NULL) {
		err_clear();
		if (inst < other)
			return -1;
		if (inst > other)
			return 1;
		return 0;
	}
	res = call_object(func, (object *)other);
	DECREF(func);
	if (res == NULL) {
		err_clear(); /* XXX Should report the error, bot how...??? */
		return 0;
	}
	if (is_intobject(res))
		outcome = getintvalue(res);
	else
		outcome = 0; /* XXX Should report the error, bot how...??? */
	DECREF(res);
	return outcome;
}

static long
instance_hash(inst)
	instanceobject *inst;
{
	object *func;
	object *res;
	long outcome;

	func = instance_getattr(inst, "__hash__");
	if (func == NULL) {
		/* If there is no __cmp__ method, we hash on the address.
		   If a __cmp__ method exists, there must be a __hash__. */
		err_clear();
		func = instance_getattr(inst, "__cmp__");
		if (func == NULL) {
			err_clear();
			outcome = (long)inst;
			if (outcome == -1)
				outcome = -2;
			return outcome;
		}
		err_setstr(TypeError, "unhashable instance");
		return -1;
	}
	res = call_object(func, (object *)NULL);
	DECREF(func);
	if (res == NULL)
		return -1;
	if (is_intobject(res)) {
		outcome = getintvalue(res);
		if (outcome == -1)
			outcome = -2;
	}
	else {
		err_setstr(TypeError, "__hash__() should return an int");
		outcome = -1;
	}
	DECREF(res);
	return outcome;
}

static int
instance_length(inst)
	instanceobject *inst;
{
	object *func;
	object *res;
	int outcome;

	func = instance_getattr(inst, "__len__");
	if (func == NULL)
		return -1;
	res = call_object(func, (object *)NULL);
	DECREF(func);
	if (res == NULL)
		return -1;
	if (is_intobject(res)) {
		outcome = getintvalue(res);
		if (outcome < 0)
			err_setstr(ValueError, "__len__() should return >= 0");
	}
	else {
		err_setstr(TypeError, "__len__() should return an int");
		outcome = -1;
	}
	DECREF(res);
	return outcome;
}

static object *
instance_subscript(inst, key)
	instanceobject *inst;
	object *key;
{
	object *func;
	object *arg;
	object *res;

	func = instance_getattr(inst, "__getitem__");
	if (func == NULL)
		return NULL;
	arg = mkvalue("(O)", key);
	if (arg == NULL) {
		DECREF(func);
		return NULL;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	return res;
}

static int
instance_ass_subscript(inst, key, value)
	instanceobject*inst;
	object *key;
	object *value;
{
	object *func;
	object *arg;
	object *res;

	if (value == NULL)
		func = instance_getattr(inst, "__delitem__");
	else
		func = instance_getattr(inst, "__setitem__");
	if (func == NULL)
		return -1;
	if (value == NULL)
		arg = mkvalue("(O)", key);
	else
		arg = mkvalue("(OO)", key, value);
	if (arg == NULL) {
		DECREF(func);
		return -1;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	if (res == NULL)
		return -1;
	DECREF(res);
	return 0;
}

static mapping_methods instance_as_mapping = {
	instance_length,	/*mp_length*/
	instance_subscript,	/*mp_subscript*/
	instance_ass_subscript,	/*mp_ass_subscript*/
};

static object *
instance_concat(inst, other)
	instanceobject *inst, *other;
{
	object *func, *arg, *res;

	func = instance_getattr(inst, "__add__");
	if (func == NULL)
		return NULL;
	arg = mkvalue("(O)", other);
	if (arg == NULL) {
		DECREF(func);
		return NULL;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	return res;
}

static object *
instance_repeat(inst, count)
	instanceobject *inst;
	int count;
{
	object *func, *arg, *res;

	func = instance_getattr(inst, "__mul__");
	if (func == NULL)
		return NULL;
	arg = newintobject((long)count);
	if (arg == NULL) {
		DECREF(func);
		return NULL;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	return res;
}

static object *
instance_item(inst, i)
	instanceobject *inst;
	int i;
{
	object *func, *arg, *res;

	func = instance_getattr(inst, "__getitem__");
	if (func == NULL)
		return NULL;
	arg = newintobject((long)i);
	if (arg == NULL) {
		DECREF(func);
		return NULL;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	return res;
}

static object *
instance_slice(inst, i, j)
	instanceobject *inst;
	int i, j;
{
	object *func, *arg, *res;

	func = instance_getattr(inst, "__getslice__");
	if (func == NULL)
		return NULL;
	arg = mkvalue("(ii)", i, j);
	if (arg == NULL) {
		DECREF(func);
		return NULL;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	return res;
}

static int
instance_ass_item(inst, i, item)
	instanceobject *inst;
	int i;
	object *item;
{
	object *func, *arg, *res;

	if (item == NULL)
		func = instance_getattr(inst, "__delitem__");
	else
		func = instance_getattr(inst, "__setitem__");
	if (func == NULL)
		return -1;
	if (item == NULL)
		arg = mkvalue("i", i);
	else
		arg = mkvalue("(iO)", i, item);
	if (arg == NULL) {
		DECREF(func);
		return -1;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	if (res == NULL)
		return -1;
	DECREF(res);
	return 0;
}

static int
instance_ass_slice(inst, i, j, value)
	instanceobject *inst;
	int i, j;
	object *value;
{
	object *func, *arg, *res;

	if (value == NULL)
		func = instance_getattr(inst, "__delslice__");
	else
		func = instance_getattr(inst, "__setslice__");
	if (func == NULL)
		return -1;
	if (value == NULL)
		arg = mkvalue("(ii)", i, j);
	else
		arg = mkvalue("(iiO)", i, j, value);
	if (arg == NULL) {
		DECREF(func);
		return -1;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	if (res == NULL)
		return -1;
	DECREF(res);
	return 0;
}

static sequence_methods instance_as_sequence = {
	instance_length,	/*sq_length*/
	instance_concat,	/*sq_concat*/
	instance_repeat,	/*sq_repeat*/
	instance_item,		/*sq_item*/
	instance_slice,		/*sq_slice*/
	instance_ass_item,	/*sq_ass_item*/
	instance_ass_slice,	/*sq_ass_slice*/
};

static object *
generic_binary_op(self, other, methodname)
	instanceobject *self;
	object *other;
	char *methodname;
{
	object *func, *arg, *res;

	if ((func = instance_getattr(self, methodname)) == NULL)
		return NULL;
	arg = mkvalue("O", other);
	if (arg == NULL) {
		DECREF(func);
		return NULL;
	}
	res = call_object(func, arg);
	DECREF(func);
	DECREF(arg);
	return res;
}

static object *
generic_unary_op(self, methodname)
	instanceobject *self;
	char *methodname;
{
	object *func, *res;

	if ((func = instance_getattr(self, methodname)) == NULL)
		return NULL;
	res = call_object(func, (object *)NULL);
	DECREF(func);
	return res;
}

#define BINARY(funcname, methodname) \
static object * funcname(self, other) instanceobject *self; object *other; { \
	return generic_binary_op(self, other, methodname); \
}

#define UNARY(funcname, methodname) \
static object *funcname(self) instanceobject *self; { \
	return generic_unary_op(self, methodname); \
}

BINARY(instance_add, "__add__")
BINARY(instance_sub, "__sub__")
BINARY(instance_mul, "__mul__")
BINARY(instance_div, "__div__")
BINARY(instance_mod, "__mod__")
BINARY(instance_divmod, "__divmod__")
BINARY(instance_pow, "__pow__")
UNARY(instance_neg, "__neg__")
UNARY(instance_pos, "__pos__")
UNARY(instance_abs, "__abs__")

static int
instance_nonzero(self)
	instanceobject *self;
{
	object *func, *res;
	long outcome;

	if ((func = instance_getattr(self, "__nonzero__")) == NULL) {
		err_clear();
		if ((func = instance_getattr(self, "__len__")) == NULL) {
			err_clear();
			/* Fall back to the default behavior:
			   all instances are nonzero */
			return 1;
		}
	}
	res = call_object(func, (object *)NULL);
	DECREF(func);
	if (res == NULL)
		return -1;
	if (!is_intobject(res)) {
		DECREF(res);
		err_setstr(TypeError, "__nonzero__ should return an int");
		return -1;
	}
	outcome = getintvalue(res);
	DECREF(res);
	if (outcome < 0) {
		err_setstr(ValueError, "__nonzero__ should return >= 0");
		return -1;
	}
	return outcome > 0;
}

UNARY(instance_invert, "__invert__")
BINARY(instance_lshift, "__lshift__")
BINARY(instance_rshift, "__rshift__")
BINARY(instance_and, "__and__")
BINARY(instance_xor, "__xor__")
BINARY(instance_or, "__or__")

static int
instance_coerce(pv, pw)
	object **pv, **pw;
{
	object *v =  *pv;
	object *w = *pw;
	object *func;
	object *res;
	int outcome;

	if (!is_instanceobject(v))
		return 1; /* XXX shouldn't be possible */
	func = instance_getattr((instanceobject *)v, "__coerce__");
	if (func == NULL) {
		err_clear();
		return 1;
	}
	res = call_object(func, w);
	if (res == NULL)
		return -1;
	if (res == None) {
		DECREF(res);
		return 1;
	}
	outcome = getargs(res, "(OO)", &v, &w);
	if (!outcome || v->ob_type != w->ob_type ||
			v->ob_type->tp_as_number == NULL) {
		DECREF(res);
		err_setstr(TypeError, "bad __coerce__ result");
		return -1;
	}
	INCREF(v);
	INCREF(w);
	DECREF(res);
	*pv = v;
	*pw = w;
	return 0;
}

UNARY(instance_int, "__int__")
UNARY(instance_long, "__long__")
UNARY(instance_float, "__float__")
UNARY(instance_oct, "__oct__")
UNARY(instance_hex, "__hex__")

static number_methods instance_as_number = {
	instance_add,		/*nb_add*/
	instance_sub,		/*nb_subtract*/
	instance_mul,		/*nb_multiply*/
	instance_div,		/*nb_divide*/
	instance_mod,		/*nb_remainder*/
	instance_divmod,	/*nb_divmod*/
	instance_pow,		/*nb_power*/
	instance_neg,		/*nb_negative*/
	instance_pos,		/*nb_positive*/
	instance_abs,		/*nb_absolute*/
	instance_nonzero,	/*nb_nonzero*/
	instance_invert,	/*nb_invert*/
	instance_lshift,	/*nb_lshift*/
	instance_rshift,	/*nb_rshift*/
	instance_and,		/*nb_and*/
	instance_xor,		/*nb_xor*/
	instance_or,		/*nb_or*/
	instance_coerce,	/*nb_coerce*/
	instance_int,		/*nb_int*/
	instance_long,		/*nb_long*/
	instance_float,		/*nb_float*/
	instance_oct,		/*nb_oct*/
	instance_hex,		/*nb_hex*/
};

typeobject Instancetype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"instance",
	sizeof(instanceobject),
	0,
	instance_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	(object * (*) FPROTO((object *, char *)))
	instance_getattr,	/*tp_getattr*/
	instance_setattr,	/*tp_setattr*/
	instance_compare,	/*tp_compare*/
	instance_repr,		/*tp_repr*/
	&instance_as_number,	/*tp_as_number*/
	&instance_as_sequence,	/*tp_as_sequence*/
	&instance_as_mapping,	/*tp_as_mapping*/
	instance_hash,		/*tp_hash*/
};

static object *
instance_convert(inst, methodname)
	object *inst;
	char *methodname;
{
	return generic_unary_op((instanceobject *)inst, methodname);
}


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

static int
instancemethod_compare(a, b)
	instancemethodobject *a, *b;
{
	int cmp = cmpobject(a->im_self, b->im_self);
	if (cmp == 0)
		cmp = cmpobject(a->im_func, b->im_func);
	return cmp;
}

static object *
instancemethod_repr(a)
	instancemethodobject *a;
{
	char buf[240];
	object *classname =
		((instanceobject *)(a->im_self))->in_class->cl_name;
	object *funcname = ((funcobject *)(a->im_func))->func_name;
	char *cname, *fname;
	if (classname != NULL && is_stringobject(classname))
		cname = getstringvalue(classname);
	else
		cname = "?";
	if (funcname != NULL && is_stringobject(funcname))
		fname = getstringvalue(funcname);
	else
		fname = "?";
	sprintf(buf, "<method %.100s of %.100s instance at %lx>",
		fname, cname, (long)a->im_func);
	return newstringobject(buf);
}

static long
instancemethod_hash(a)
	instancemethodobject *a;
{
	long x, y;
	x = hashobject(a->im_self);
	if (x == -1)
		return -1;
	y = hashobject(a->im_func);
	if (y == -1)
		return -1;
	return x ^ y;
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
	instancemethod_compare,	/*tp_compare*/
	instancemethod_repr,	/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	instancemethod_hash,	/*tp_hash*/
};
