/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

/* Forward */
static object *class_lookup PROTO((classobject *, char *, classobject **));
static object *instance_getattr1 PROTO((instanceobject *, char *));

object *
newclassobject(bases, dict, name)
	object *bases; /* NULL or tuple of classobjects! */
	object *dict;
	object *name; /* String; NULL if unknown */
{
	int pos;
	object *key, *value;
	classobject *op, *dummy;
	if (dictlookup(dict, "__doc__") == NULL) {
		if (dictinsert(dict, "__doc__", None) < 0)
			return NULL;
	}
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
	INCREF(dict);
	op->cl_dict = dict;
	XINCREF(name);
	op->cl_name = name;
	op->cl_getattr = class_lookup(op, "__getattr__", &dummy);
	op->cl_setattr = class_lookup(op, "__setattr__", &dummy);
	op->cl_delattr = class_lookup(op, "__delattr__", &dummy);
	XINCREF(op->cl_getattr);
	XINCREF(op->cl_setattr);
	XINCREF(op->cl_delattr);
	pos = 0;
	while (mappinggetnext(dict, &pos, &key, &value)) {
		if (is_accessobject(value))
			setaccessowner(value, (object *)op);
	}
	return (object *) op;
}

/* Class methods */

static void
class_dealloc(op)
	classobject *op;
{
	DECREF(op->cl_bases);
	DECREF(op->cl_dict);
	XDECREF(op->cl_name);
	free((ANY *)op);
}

static object *
class_lookup(cp, name, pclass)
	classobject *cp;
	char *name;
	classobject **pclass;
{
	int i, n;
	object *value = dictlookup(cp->cl_dict, name);
	if (value != NULL) {
		*pclass = cp;
		return value;
	}
	n = gettuplesize(cp->cl_bases);
	for (i = 0; i < n; i++) {
		object *v = class_lookup((classobject *)
				 gettupleitem(cp->cl_bases, i), name, pclass);
		if (v != NULL)
			return v;
	}
	return NULL;
}

static object *
class_getattr(op, name)
	register classobject *op;
	register char *name;
{
	register object *v;
	classobject *class;
	if (name[0] == '_' && name[1] == '_') {
		if (strcmp(name, "__dict__") == 0) {
			if (getrestricted()) {
				err_setstr(RuntimeError,
					   "class.__dict__ not accessible in restricted mode");
				return NULL;
			}
			INCREF(op->cl_dict);
			return op->cl_dict;
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
	}
	v = class_lookup(op, name, &class);
	if (v == NULL) {
		err_setstr(AttributeError, name);
		return NULL;
	}
	if (is_accessobject(v)) {
		v = getaccessvalue(v, getowner());
		if (v == NULL)
			return NULL;
	}
	else
		INCREF(v);
	if (is_funcobject(v)) {
		object *w = newinstancemethodobject(v, (object *)NULL,
						    (object *)class);
		DECREF(v);
		v = w;
	}
	return v;
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
	ac = dictlookup(op->cl_dict, name);
	if (ac != NULL && is_accessobject(ac))
		return setaccessvalue(ac, getowner(), v);
	if (v == NULL) {
		int rv = dictremove(op->cl_dict, name);
		if (rv < 0)
			err_setstr(AttributeError,
				   "delete non-existing class attribute");
		return rv;
	}
	else
		return dictinsert(op->cl_dict, name, v);
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
	(destructor)class_dealloc, /*tp_dealloc*/
	0,		/*tp_print*/
	(getattrfunc)class_getattr, /*tp_getattr*/
	(setattrfunc)class_setattr, /*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)class_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};

int
issubclass(class, base)
	object *class;
	object *base;
{
	int i, n;
	classobject *cp;
	if (class == base)
		return 1;
	if (class == NULL || !is_classobject(class))
		return 0;
	cp = (classobject *)class;
	n = gettuplesize(cp->cl_bases);
	for (i = 0; i < n; i++) {
		if (issubclass(gettupleitem(cp->cl_bases, i), base))
			return 1;
	}
	return 0;
}


/* Instance objects */

static int
addaccess(class, inst)
	classobject *class;
	instanceobject *inst;
{
	int i, n, pos, ret;
	object *key, *value, *ac;
	
	n = gettuplesize(class->cl_bases);
	for (i = 0; i < n; i++) {
		if (addaccess((classobject *)gettupleitem(class->cl_bases, i), inst) < 0)
			return -1;
	}
	
	pos = 0;
	while (mappinggetnext(class->cl_dict, &pos, &key, &value)) {
		if (!is_accessobject(value))
			continue;
		if (hasaccessvalue(value))
			continue;
		ac = dict2lookup(inst->in_dict, key);
		if (ac != NULL && is_accessobject(ac)) {
			err_setval(ConflictError, key);
			return -1;
		}
		ac = cloneaccessobject(value);
		if (ac == NULL)
			return -1;
		ret = dict2insert(inst->in_dict, key, ac);
		DECREF(ac);
		if (ret != 0)
			return -1;
    	}
	return 0;
}

object *
newinstanceobject(class, arg)
	object *class;
	object *arg;
{
	register instanceobject *inst;
	object *init;
	if (!is_classobject(class)) {
		err_badcall();
		return NULL;
	}
	inst = NEWOBJ(instanceobject, &Instancetype);
	if (inst == NULL)
		return NULL;
	INCREF(class);
	inst->in_class = (classobject *)class;
	inst->in_dict = newdictobject();
	if (inst->in_dict == NULL ||
	    addaccess((classobject *)class, inst) != 0) {
		DECREF(inst);
		return NULL;
	}
	init = instance_getattr1(inst, "__init__");
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
	object *error_type, *error_value, *error_traceback;
	object *del;
	/* Call the __del__ method if it exists.  First temporarily
	   revive the object and save the current exception, if any. */
	INCREF(inst);
	err_fetch(&error_type, &error_value, &error_traceback);
	if ((del = instance_getattr1(inst, "__del__")) != NULL) {
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
	err_restore(error_type, error_value, error_traceback);
	/* Can't use DECREF here, it would cause a recursive call */
	if (--inst->ob_refcnt > 0)
		return; /* __del__ added a reference; don't delete now */
	DECREF(inst->in_class);
	XDECREF(inst->in_dict);
	free((ANY *)inst);
}

static object *
instance_getattr1(inst, name)
	register instanceobject *inst;
	register char *name;
{
	register object *v;
	classobject *class;
	if (name[0] == '_' && name[1] == '_') {
		if (strcmp(name, "__dict__") == 0) {
			if (getrestricted()) {
				err_setstr(RuntimeError,
					   "instance.__dict__ not accessible in restricted mode");
				return NULL;
			}
			INCREF(inst->in_dict);
			return inst->in_dict;
		}
		if (strcmp(name, "__class__") == 0) {
			INCREF(inst->in_class);
			return (object *)inst->in_class;
		}
	}
	class = NULL;
	v = dictlookup(inst->in_dict, name);
	if (v == NULL) {
		v = class_lookup(inst->in_class, name, &class);
		if (v == NULL) {
			err_setstr(AttributeError, name);
			return NULL;
		}
	}
	if (is_accessobject(v)) {
		v = getaccessvalue(v, getowner());
		if (v == NULL)
			return NULL;
	}
	else
		INCREF(v);
	if (class != NULL) {
		if (is_funcobject(v)) {
			object *w = newinstancemethodobject(v, (object *)inst,
							    (object *)class);
			DECREF(v);
			v = w;
		}
		else if (is_instancemethodobject(v)) {
			object *im_class = instancemethodgetclass(v);
			/* Only if classes are compatible */
			if (issubclass((object *)class, im_class)) {
				object *im_func = instancemethodgetfunc(v);
				object *w = newinstancemethodobject(im_func,
						(object *)inst, im_class);
				DECREF(v);
				v = w;
			}
		}
	}
	return v;
}

static object *
instance_getattr(inst, name)
	register instanceobject *inst;
	register char *name;
{
	register object *func, *res;
	res = instance_getattr1(inst, name);
	if (res == NULL && (func = inst->in_class->cl_getattr) != NULL) {
		object *args;
		err_clear();
		args = mkvalue("(Os)", inst, name);
		if (args == NULL)
			return NULL;
		res = call_object(func, args);
		DECREF(args);
	}
	return res;
}

static int
instance_setattr1(inst, name, v)
	instanceobject *inst;
	char *name;
	object *v;
{
	object *ac;
	ac = dictlookup(inst->in_dict, name);
	if (ac != NULL && is_accessobject(ac))
		return setaccessvalue(ac, getowner(), v);
	if (v == NULL) {
		int rv = dictremove(inst->in_dict, name);
		if (rv < 0)
			err_setstr(AttributeError,
				   "delete non-existing instance attribute");
		return rv;
	}
	else
		return dictinsert(inst->in_dict, name, v);
}

static int
instance_setattr(inst, name, v)
	instanceobject *inst;
	char *name;
	object *v;
{
	object *func, *args, *res;
	if (name[0] == '_' && name[1] == '_') {
		int n = strlen(name);
		if (name[n-1] == '_' && name[n-2] == '_') {
			err_setstr(TypeError, "read-only special attribute");
			return -1;
		}
	}
	if (v == NULL)
		func = inst->in_class->cl_delattr;
	else
		func = inst->in_class->cl_setattr;
	if (func == NULL)
		return instance_setattr1(inst, name, v);
	if (v == NULL)
		args = mkvalue("(Os)", inst, name);
	else
		args = mkvalue("(OsO)", inst, name, v);
	if (args == NULL)
		return -1;
	res = call_object(func, args);
	DECREF(args);
	if (res == NULL)
		return -1;
	DECREF(res);
	return 0;
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

static object *
instance_compare1(inst, other)
	object *inst, *other;
{
	return instancebinop(inst, other, "__cmp__", "__rcmp__",
			     instance_compare1);
}

static int
instance_compare(inst, other)
	object *inst, *other;
{
	object *result;
	long outcome;
	result = instance_compare1(inst, other);
	if (result == NULL || !is_intobject(result)) {
	error:
		err_clear();
		return (inst < other) ? -1 : 1;
	}
	outcome = getintvalue(result);
	DECREF(result);
	if (outcome < 0)
		return -1;
	else if (outcome > 0)
		return 1;
	return 0;
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
	(inquiry)instance_length, /*mp_length*/
	(binaryfunc)instance_subscript, /*mp_subscript*/
	(objobjargproc)instance_ass_subscript, /*mp_ass_subscript*/
};

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
	(inquiry)instance_length, /*sq_length*/
	0, /*sq_concat*/
	0, /*sq_repeat*/
	(intargfunc)instance_item, /*sq_item*/
	(intintargfunc)instance_slice, /*sq_slice*/
	(intobjargproc)instance_ass_item, /*sq_ass_item*/
	(intintobjargproc)instance_ass_slice, /*sq_ass_slice*/
};

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


/* Forward */
static int halfbinop PROTO((object *, object *, char *, object **,
			    object * (*) PROTO((object *, object *)), int ));


/* Implement a binary operator involving at least one class instance. */

object *
instancebinop(v, w, opname, ropname, thisfunc)
	object *v;
	object *w;
	char *opname;
	char *ropname;
	object * (*thisfunc) PROTO((object *, object *));
{
	char buf[256];
	object *result = NULL;
	if (halfbinop(v, w, opname, &result, thisfunc, 0) <= 0)
		return result;
	if (halfbinop(w, v, ropname, &result, thisfunc, 1) <= 0)
		return result;
	sprintf(buf, "%s nor %s defined for these operands", opname, ropname);
	err_setstr(TypeError, buf);
	return NULL;
}


/* Try one half of a binary operator involving a class instance.
   Return value:
   -1 if an exception is to be reported right away
   0  if we have a valid result
   1  if we could try another operation
*/

static int
halfbinop(v, w, opname, r_result, thisfunc, swapped)
	object *v;
	object *w;
	char *opname;
	object **r_result;
	object * (*thisfunc) PROTO((object *, object *));
	int swapped;
{
	object *func;
	object *args;
	object *coerce;
	object *coerced = NULL;
	object *v1;
	
	if (!is_instanceobject(v))
		return 1;
	coerce = getattr(v, "__coerce__");
	if (coerce == NULL) {
		err_clear();
	}
	else {
		args = mkvalue("(O)", w);
		if (args == NULL) {
			return -1;
		}
		coerced = call_object(coerce, args);
		DECREF(args);
		DECREF(coerce);
		if (coerced == NULL) {
			return -1;
		}
		if (coerced == None) {
			DECREF(coerced);
			return 1;
		}
		if (!is_tupleobject(coerced) || gettuplesize(coerced) != 2) {
			DECREF(coerced);
			err_setstr(TypeError,
				   "coercion should return None or 2-tuple");
			return -1;
		}
		v1 = gettupleitem(coerced, 0);
		w = gettupleitem(coerced, 1);
		if (v1 != v) {
			v = v1;
			if (!is_instanceobject(v) && !is_instanceobject(w)) {
				if (swapped)
					*r_result = (*thisfunc)(w, v);
				else
					*r_result = (*thisfunc)(v, w);
				DECREF(coerced);
				return *r_result == NULL ? -1 : 0;
			}
		}
		w = gettupleitem(coerced, 1);
	}
	func = getattr(v, opname);
	if (func == NULL) {
		XDECREF(coerced);
		if (err_occurred() != AttributeError)
			return -1;
		err_clear();
		return 1;
	}
	args = mkvalue("(O)", w);
	if (args == NULL) {
		DECREF(func);
		XDECREF(coerced);
		return -1;
	}
	*r_result = call_object(func, args);
	DECREF(args);
	DECREF(func);
	XDECREF(coerced);
	return *r_result == NULL ? -1 : 0;
}

static int
instance_coerce(pv, pw)
	object **pv;
	object **pw;
{
	object *v = *pv;
	object *w = *pw;
	object *coerce;
	object *args;
	object *coerced;

	coerce = getattr(v, "__coerce__");
	if (coerce == NULL) {
		/* No __coerce__ method: always OK */
		err_clear();
		INCREF(v);
		INCREF(w);
		return 0;
	}
	/* Has __coerce__ method: call it */
	args = mkvalue("(O)", w);
	if (args == NULL) {
		return -1;
	}
	coerced = call_object(coerce, args);
	DECREF(args);
	DECREF(coerce);
	if (coerced == NULL) {
		/* __coerce__ call raised an exception */
		return -1;
	}
	if (coerced == None) {
		/* __coerce__ says "I can't do it" */
		DECREF(coerced);
		return 1;
	}
	if (!is_tupleobject(coerced) || gettuplesize(coerced) != 2) {
		/* __coerce__ return value is malformed */
		DECREF(coerced);
		err_setstr(TypeError,
			   "coercion should return None or 2-tuple");
		return -1;
	}
	/* __coerce__ returned two new values */
	*pv = gettupleitem(coerced, 0);
	*pw = gettupleitem(coerced, 1);
	INCREF(*pv);
	INCREF(*pw);
	DECREF(coerced);
	return 0;
}


#define UNARY(funcname, methodname) \
static object *funcname(self) instanceobject *self; { \
	return generic_unary_op(self, methodname); \
}

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
UNARY(instance_int, "__int__")
UNARY(instance_long, "__long__")
UNARY(instance_float, "__float__")
UNARY(instance_oct, "__oct__")
UNARY(instance_hex, "__hex__")

/* This version is for ternary calls only (z != None) */
static object *
instance_pow(v, w, z)
	object *v;
	object *w;
	object *z;
{
	/* XXX Doesn't do coercions... */
	object *func;
	object *args;
	object *result;
	func = getattr(v, "__pow__");
	if (func == NULL)
		return NULL;
	args = mkvalue("(OO)", w, z);
	if (args == NULL) {
		DECREF(func);
		return NULL;
	}
	result = call_object(func, args);
	DECREF(func);
	DECREF(args);
	return result;
}

static number_methods instance_as_number = {
	0, /*nb_add*/
	0, /*nb_subtract*/
	0, /*nb_multiply*/
	0, /*nb_divide*/
	0, /*nb_remainder*/
	0, /*nb_divmod*/
	(ternaryfunc)instance_pow, /*nb_power*/
	(unaryfunc)instance_neg, /*nb_negative*/
	(unaryfunc)instance_pos, /*nb_positive*/
	(unaryfunc)instance_abs, /*nb_absolute*/
	(inquiry)instance_nonzero, /*nb_nonzero*/
	(unaryfunc)instance_invert, /*nb_invert*/
	0, /*nb_lshift*/
	0, /*nb_rshift*/
	0, /*nb_and*/
	0, /*nb_xor*/
	0, /*nb_or*/
	(coercion)instance_coerce, /*nb_coerce*/
	(unaryfunc)instance_int, /*nb_int*/
	(unaryfunc)instance_long, /*nb_long*/
	(unaryfunc)instance_float, /*nb_float*/
	(unaryfunc)instance_oct, /*nb_oct*/
	(unaryfunc)instance_hex, /*nb_hex*/
};

typeobject Instancetype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"instance",
	sizeof(instanceobject),
	0,
	(destructor)instance_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)instance_getattr, /*tp_getattr*/
	(setattrfunc)instance_setattr, /*tp_setattr*/
	instance_compare, /*tp_compare*/
	(reprfunc)instance_repr, /*tp_repr*/
	&instance_as_number,	/*tp_as_number*/
	&instance_as_sequence,	/*tp_as_sequence*/
	&instance_as_mapping,	/*tp_as_mapping*/
	(hashfunc)instance_hash, /*tp_hash*/
};


/* Instance method objects are used for two purposes:
   (a) as bound instance methods (returned by instancename.methodname)
   (b) as unbound methods (returned by ClassName.methodname)
   In case (b), im_self is NULL
*/

typedef struct {
	OB_HEAD
	object	*im_func;	/* The function implementing the method */
	object	*im_self;	/* The instance it is bound to, or NULL */
	object	*im_class;	/* The class that defined the method */
	object	*im_doc;	/* The documentation string */
} instancemethodobject;

object *
newinstancemethodobject(func, self, class)
	object *func;
	object *self;
	object *class;
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
	XINCREF(self);
	im->im_self = self;
	INCREF(class);
	im->im_class = class;
	XINCREF(((funcobject *)func)->func_doc);
	im->im_doc = ((funcobject *)func)->func_doc;
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

object *
instancemethodgetclass(im)
	register object *im;
{
	if (!is_instancemethodobject(im)) {
		err_badcall();
		return NULL;
	}
	return ((instancemethodobject *)im)->im_class;
}

/* Class method methods */

#define OFF(x) offsetof(instancemethodobject, x)

static struct memberlist instancemethod_memberlist[] = {
	{"im_func",	T_OBJECT,	OFF(im_func)},
	{"im_self",	T_OBJECT,	OFF(im_self)},
	{"im_class",	T_OBJECT,	OFF(im_class)},
	{"im_doc",	T_OBJECT,	OFF(im_doc)},
	{"__doc__",	T_OBJECT,	OFF(im_doc)},
	{NULL}	/* Sentinel */
};

static object *
instancemethod_getattr(im, name)
	register instancemethodobject *im;
	char *name;
{
	if (name[0] != '_' && getrestricted()) {
		err_setstr(RuntimeError,
			   "instance-method attributes not accessible in restricted mode");
		return NULL;
	}
	return getmember((char *)im, instancemethod_memberlist, name);
}

static void
instancemethod_dealloc(im)
	register instancemethodobject *im;
{
	DECREF(im->im_func);
	XDECREF(im->im_self);
	DECREF(im->im_class);
	XDECREF(im->im_doc);
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
	instanceobject *self = (instanceobject *)(a->im_self);
	funcobject *func = (funcobject *)(a->im_func);
	classobject *class = (classobject *)(a->im_class);
	object *fclassname, *iclassname, *funcname;
	char *fcname, *icname, *fname;
	fclassname = class->cl_name;
	funcname = func->func_name;
	if (fclassname != NULL && is_stringobject(fclassname))
		fcname = getstringvalue(fclassname);
	else
		fcname = "?";
	if (funcname != NULL && is_stringobject(funcname))
		fname = getstringvalue(funcname);
	else
		fname = "?";
	if (self == NULL)
		sprintf(buf, "<unbound method %.100s.%.100s>", fcname, fname);
	else {
		iclassname = self->in_class->cl_name;
		if (iclassname != NULL && is_stringobject(iclassname))
			icname = getstringvalue(iclassname);
		else
			icname = "?";
		sprintf(buf, "<method %.60s.%.60s of %.60s instance at %lx>",
			fcname, fname, icname, (long)self);
	}
	return newstringobject(buf);
}

static long
instancemethod_hash(a)
	instancemethodobject *a;
{
	long x, y;
	if (a->im_self == NULL)
		x = hashobject(None);
	else
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
	(destructor)instancemethod_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)instancemethod_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)instancemethod_compare, /*tp_compare*/
	(reprfunc)instancemethod_repr, /*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc)instancemethod_hash, /*tp_hash*/
};
