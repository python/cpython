/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Class object implementation */

#include "allobjects.h"
#include "structmember.h"

/* Forward */
static object *class_lookup PROTO((classobject *, object *, classobject **));
static object *instance_getattr1 PROTO((instanceobject *, object *));

object *
newclassobject(bases, dict, name)
	object *bases; /* NULL or tuple of classobjects! */
	object *dict;
	object *name; /* String; NULL if unknown */
{
#ifdef SUPPORT_OBSOLETE_ACCESS
	int pos;
	object *key, *value;
#endif
	classobject *op, *dummy;
	static object *getattrstr, *setattrstr, *delattrstr;
	static object *docstr;
	if (docstr == NULL) {
		docstr= PyString_InternFromString("__doc__");
		if (docstr == NULL)
			return NULL;
	}
	if (mappinglookup(dict, docstr) == NULL) {
		if (mappinginsert(dict, docstr, None) < 0)
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
	if (getattrstr == NULL) {
		getattrstr = PyString_InternFromString("__getattr__");
		setattrstr = PyString_InternFromString("__setattr__");
		delattrstr = PyString_InternFromString("__delattr__");
	}
	op->cl_getattr = class_lookup(op, getattrstr, &dummy);
	op->cl_setattr = class_lookup(op, setattrstr, &dummy);
	op->cl_delattr = class_lookup(op, delattrstr, &dummy);
	XINCREF(op->cl_getattr);
	XINCREF(op->cl_setattr);
	XINCREF(op->cl_delattr);
#ifdef SUPPORT_OBSOLETE_ACCESS
	pos = 0;
	while (mappinggetnext(dict, &pos, &key, &value)) {
		if (is_accessobject(value))
			setaccessowner(value, (object *)op);
	}
#endif
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
	object *name;
	classobject **pclass;
{
	int i, n;
	object *value = mappinglookup(cp->cl_dict, name);
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
	object *name;
{
	register object *v;
	register char *sname = getstringvalue(name);
	classobject *class;
	if (sname[0] == '_' && sname[1] == '_') {
		if (strcmp(sname, "__dict__") == 0) {
			if (getrestricted()) {
				err_setstr(RuntimeError,
					   "class.__dict__ not accessible in restricted mode");
				return NULL;
			}
			INCREF(op->cl_dict);
			return op->cl_dict;
		}
		if (strcmp(sname, "__bases__") == 0) {
			INCREF(op->cl_bases);
			return op->cl_bases;
		}
		if (strcmp(sname, "__name__") == 0) {
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
		err_setval(AttributeError, name);
		return NULL;
	}
#ifdef SUPPORT_OBSOLETE_ACCESS
	if (is_accessobject(v)) {
		v = getaccessvalue(v, getowner());
		if (v == NULL)
			return NULL;
	}
	else
#endif
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
	object *name;
	object *v;
{
#ifdef SUPPORT_OBSOLETE_ACCESS
	object *ac;
#endif
	char *sname = getstringvalue(name);
	if (sname[0] == '_' && sname[1] == '_') {
		int n = getstringsize(name);
		if (sname[n-1] == '_' && sname[n-2] == '_') {
			err_setstr(TypeError, "read-only special attribute");
			return -1;
		}
	}
	if (getrestricted()) {
		err_setstr(RuntimeError,
			   "classes are read-only in restricted mode");
		return -1;
	}
#ifdef SUPPORT_OBSOLETE_ACCESS
	ac = mappinglookup(op->cl_dict, name);
	if (ac != NULL && is_accessobject(ac))
		return setaccessvalue(ac, getowner(), v);
#endif
	if (v == NULL) {
		int rv = mappingremove(op->cl_dict, name);
		if (rv < 0)
			err_setstr(AttributeError,
				   "delete non-existing class attribute");
		return rv;
	}
	else
		return mappinginsert(op->cl_dict, name, v);
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
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	(reprfunc)class_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	0,		/*tp_hash*/
	0,		/*tp_call*/
	0,		/*tp_str*/
	(getattrofunc)class_getattr, /*tp_getattro*/
	(setattrofunc)class_setattr, /*tp_setattro*/
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

#ifdef SUPPORT_OBSOLETE_ACCESS
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
#endif

object *
newinstanceobject(class, arg, kw)
	object *class;
	object *arg;
	object *kw;
{
	register instanceobject *inst;
	object *init;
	static object *initstr;
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
	if (inst->in_dict == NULL
#ifdef SUPPORT_OBSOLETE_ACCESS
	    || addaccess((classobject *)class, inst) != 0
#endif
		) {
		DECREF(inst);
		return NULL;
	}
	if (initstr == NULL)
		initstr = PyString_InternFromString("__init__");
	init = instance_getattr1(inst, initstr);
	if (init == NULL) {
		err_clear();
		if ((arg != NULL && (!is_tupleobject(arg) ||
				     gettuplesize(arg) != 0))
		    || (kw != NULL && (!is_dictobject(kw) ||
				      getdictsize(kw) != 0))) {
			err_setstr(TypeError,
				   "this constructor takes no arguments");
			DECREF(inst);
			inst = NULL;
		}
	}
	else {
		object *res = PyEval_CallObjectWithKeywords(init, arg, kw);
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
	static object *delstr;
	/* Call the __del__ method if it exists.  First temporarily
	   revive the object and save the current exception, if any. */
#ifdef Py_TRACE_REFS
	/* much too complicated if Py_TRACE_REFS defined */
	extern long ref_total;
	inst->ob_type = &Instancetype;
	NEWREF(inst);
	ref_total--;		/* compensate for increment in NEWREF */
#ifdef COUNT_ALLOCS
	inst->ob_type->tp_alloc--; /* ditto */
#endif
#else /* !Py_TRACE_REFS */
	INCREF(inst);
#endif /* !Py_TRACE_REFS */
	err_fetch(&error_type, &error_value, &error_traceback);
	if (delstr == NULL)
		delstr = PyString_InternFromString("__del__");
	if ((del = instance_getattr1(inst, delstr)) != NULL) {
		object *res = call_object(del, (object *)NULL);
		if (res == NULL) {
			object *f, *t, *v, *tb;
 			err_fetch(&t, &v, &tb);
			f = sysget("stderr");
			err_clear();
			if (f != NULL) {
				writestring("Exception ", f);
				if (t) {
					writeobject(t, f, Py_PRINT_RAW);
					if (v && v != None) {
						writestring(": ", f);
						writeobject(v, f, 0);
					}
				}
				writestring(" in ", f);
				writeobject(del, f, 0);
				writestring(" ignored\n", f);
				err_clear(); /* Just in case */
			}
			Py_XDECREF(t);
			Py_XDECREF(v);
			Py_XDECREF(tb);
		}
		else
			DECREF(res);
		DECREF(del);
	}
	/* Restore the saved exception and undo the temporary revival */
	err_restore(error_type, error_value, error_traceback);
	/* Can't use DECREF here, it would cause a recursive call */
	if (--inst->ob_refcnt > 0) {
#ifdef COUNT_ALLOCS
		inst->ob_type->tp_free--;
#endif
		return; /* __del__ added a reference; don't delete now */
	}
#ifdef Py_TRACE_REFS
#ifdef COUNT_ALLOCS
	inst->ob_type->tp_free--;	/* compensate for increment in UNREF */
#endif
	UNREF(inst);
	inst->ob_type = NULL;
#endif /* Py_TRACE_REFS */
	DECREF(inst->in_class);
	XDECREF(inst->in_dict);
	free((ANY *)inst);
}

static object *
instance_getattr1(inst, name)
	register instanceobject *inst;
	object *name;
{
	register object *v;
	register char *sname = getstringvalue(name);
	classobject *class;
	if (sname[0] == '_' && sname[1] == '_') {
		if (strcmp(sname, "__dict__") == 0) {
			if (getrestricted()) {
				err_setstr(RuntimeError,
					   "instance.__dict__ not accessible in restricted mode");
				return NULL;
			}
			INCREF(inst->in_dict);
			return inst->in_dict;
		}
		if (strcmp(sname, "__class__") == 0) {
			INCREF(inst->in_class);
			return (object *)inst->in_class;
		}
	}
	class = NULL;
	v = mappinglookup(inst->in_dict, name);
	if (v == NULL) {
		v = class_lookup(inst->in_class, name, &class);
		if (v == NULL) {
			err_setval(AttributeError, name);
			return NULL;
		}
	}
#ifdef SUPPORT_OBSOLETE_ACCESS
	if (is_accessobject(v)) {
		v = getaccessvalue(v, getowner());
		if (v == NULL)
			return NULL;
	}
	else
#endif
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
	object *name;
{
	register object *func, *res;
	res = instance_getattr1(inst, name);
	if (res == NULL && (func = inst->in_class->cl_getattr) != NULL) {
		object *args;
		err_clear();
		args = mkvalue("(OO)", inst, name);
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
	object *name;
	object *v;
{
#ifdef SUPPORT_OBSOLETE_ACCESS
	object *ac;
	ac = mappinglookup(inst->in_dict, name);
	if (ac != NULL && is_accessobject(ac))
		return setaccessvalue(ac, getowner(), v);
#endif
	if (v == NULL) {
		int rv = mappingremove(inst->in_dict, name);
		if (rv < 0)
			err_setstr(AttributeError,
				   "delete non-existing instance attribute");
		return rv;
	}
	else
		return mappinginsert(inst->in_dict, name, v);
}

static int
instance_setattr(inst, name, v)
	instanceobject *inst;
	object *name;
	object *v;
{
	object *func, *args, *res;
	char *sname = getstringvalue(name);
	if (sname[0] == '_' && sname[1] == '_'
	    && (strcmp(sname, "__dict__") == 0 ||
		strcmp(sname, "__class__") == 0)) {
	        int n = getstringsize(name);
		if (sname[n-1] == '_' && sname[n-2] == '_') {
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
		args = mkvalue("(OO)", inst, name);
	else
		args = mkvalue("(OOO)", inst, name, v);
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
	static object *reprstr;

	if (reprstr == NULL)
		reprstr = PyString_InternFromString("__repr__");
	func = instance_getattr(inst, reprstr);
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
	static object *hashstr, *cmpstr;

	if (hashstr == NULL)
		hashstr = PyString_InternFromString("__hash__");
	func = instance_getattr(inst, hashstr);
	if (func == NULL) {
		/* If there is no __cmp__ method, we hash on the address.
		   If a __cmp__ method exists, there must be a __hash__. */
		err_clear();
		if (cmpstr == NULL)
			cmpstr = PyString_InternFromString("__cmp__");
		func = instance_getattr(inst, cmpstr);
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

static object *getitemstr, *setitemstr, *delitemstr, *lenstr;

static int
instance_length(inst)
	instanceobject *inst;
{
	object *func;
	object *res;
	int outcome;

	if (lenstr == NULL)
		lenstr = PyString_InternFromString("__len__");
	func = instance_getattr(inst, lenstr);
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

	if (getitemstr == NULL)
		getitemstr = PyString_InternFromString("__getitem__");
	func = instance_getattr(inst, getitemstr);
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

	if (value == NULL) {
		if (delitemstr == NULL)
			delitemstr = PyString_InternFromString("__delitem__");
		func = instance_getattr(inst, delitemstr);
	}
	else {
		if (setitemstr == NULL)
			setitemstr = PyString_InternFromString("__setitem__");
		func = instance_getattr(inst, setitemstr);
	}
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

	if (getitemstr == NULL)
		getitemstr = PyString_InternFromString("__getitem__");
	func = instance_getattr(inst, getitemstr);
	if (func == NULL)
		return NULL;
	arg = mkvalue("(i)", i);
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
	static object *getslicestr;

	if (getslicestr == NULL)
		getslicestr = PyString_InternFromString("__getslice__");
	func = instance_getattr(inst, getslicestr);
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

	if (item == NULL) {
		if (delitemstr == NULL)
			delitemstr = PyString_InternFromString("__delitem__");
		func = instance_getattr(inst, delitemstr);
	}
	else {
		if (setitemstr == NULL)
			setitemstr = PyString_InternFromString("__setitem__");
		func = instance_getattr(inst, setitemstr);
	}
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
	static object *setslicestr, *delslicestr;

	if (value == NULL) {
		if (delslicestr == NULL)
			delslicestr = PyString_InternFromString("__delslice__");
		func = instance_getattr(inst, delslicestr);
	}
	else {
		if (setslicestr == NULL)
			setslicestr = PyString_InternFromString("__setslice__");
		func = instance_getattr(inst, setslicestr);
	}
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
	object *methodname;
{
	object *func, *res;

	if ((func = instance_getattr(self, methodname)) == NULL)
		return NULL;
	res = call_object(func, (object *)NULL);
	DECREF(func);
	return res;
}


/* Forward */
static int halfbinop Py_PROTO((object *, object *, char *, object **,
			  object * (*) Py_PROTO((object *, object *)), int ));


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

static object *coerce_obj;

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
	if (coerce_obj == NULL) {
		coerce_obj = PyString_InternFromString("__coerce__");
		if (coerce_obj == NULL)
			return -1;
	}
	coerce = getattro(v, coerce_obj);
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

	if (coerce_obj == NULL) {
		coerce_obj = PyString_InternFromString("__coerce__");
		if (coerce_obj == NULL)
			return -1;
	}
	coerce = getattro(v, coerce_obj);
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
	static object *o; \
	if (o == NULL) o = PyString_InternFromString(methodname); \
	return generic_unary_op(self, o); \
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
	static object *nonzerostr;

	if (nonzerostr == NULL)
		nonzerostr = PyString_InternFromString("__nonzero__");
	if ((func = instance_getattr(self, nonzerostr)) == NULL) {
		err_clear();
		if (lenstr == NULL)
			lenstr = PyString_InternFromString("__len__");
		if ((func = instance_getattr(self, lenstr)) == NULL) {
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
	static object *powstr;

	if (powstr == NULL)
		powstr = PyString_InternFromString("__pow__");
	func = getattro(v, powstr);
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
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	instance_compare,	/*tp_compare*/
	(reprfunc)instance_repr, /*tp_repr*/
	&instance_as_number,	/*tp_as_number*/
	&instance_as_sequence,	/*tp_as_sequence*/
	&instance_as_mapping,	/*tp_as_mapping*/
	(hashfunc)instance_hash, /*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)instance_getattr, /*tp_getattro*/
	(setattrofunc)instance_setattr, /*tp_setattro*/
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
	/* Dummies that are not handled by getattr() except for __members__ */
	{"__doc__",	T_INT,		0},
	{"__name__",	T_INT,		0},
	{NULL}	/* Sentinel */
};

static object *
instancemethod_getattr(im, name)
	register instancemethodobject *im;
	object *name;
{
	char *sname = getstringvalue(name);
	if (sname[0] == '_') {
		funcobject *func = (funcobject *)(im->im_func);
		if (strcmp(sname, "__name__") == 0) {
			INCREF(func->func_name);
			return func->func_name;
		}
		if (strcmp(sname, "__doc__") == 0) {
			INCREF(func->func_doc);
			return func->func_doc;
		}
	}
	if (getrestricted()) {
		err_setstr(RuntimeError,
			   "instance-method attributes not accessible in restricted mode");
		return NULL;
	}
	return getmember((char *)im, instancemethod_memberlist, sname);
}

static void
instancemethod_dealloc(im)
	register instancemethodobject *im;
{
	DECREF(im->im_func);
	XDECREF(im->im_self);
	DECREF(im->im_class);
	free((ANY *)im);
}

static int
instancemethod_compare(a, b)
	instancemethodobject *a, *b;
{
	if (a->im_self != b->im_self)
		return (a->im_self < b->im_self) ? -1 : 1;
	return cmpobject(a->im_func, b->im_func);
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
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)instancemethod_compare, /*tp_compare*/
	(reprfunc)instancemethod_repr, /*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	(hashfunc)instancemethod_hash, /*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	(getattrofunc)instancemethod_getattr, /*tp_getattro*/
	0,			/*tp_setattro*/
};
