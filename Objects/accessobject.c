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

/* Access object implementation */

#include "allobjects.h"

#include "structmember.h"
#include "modsupport.h"		/* For getargs() etc. */

typedef struct {
	OB_HEAD
	object		*ac_value;
	object		*ac_owner;
	typeobject	*ac_type;
	int		ac_mode;
} accessobject;

/* Forward */
static int typecheck PROTO((object *, typeobject *));
static int ownercheck PROTO((object *, object *, int, int));

object *
newaccessobject(value, owner, type, mode)
	object *value;
	object *owner;
	typeobject *type;
	int mode;
{
	accessobject *ap;
	if (!typecheck(value, type)) {
		err_setstr(AccessError,
		"access: initial value has inappropriate type");
		return NULL;
	}
	ap = NEWOBJ(accessobject, &Accesstype);
	if (ap == NULL)
		return NULL;
	XINCREF(value);
	ap->ac_value = value;
	XINCREF(owner);
	ap->ac_owner = owner;
	XINCREF(type);
	ap->ac_type = (typeobject *)type;
	ap->ac_mode = mode;
	return (object *)ap;
}

object *
cloneaccessobject(op)
	object *op;
{
	register accessobject *ap;
	if (!is_accessobject(op)) {
		err_badcall();
		return NULL;
	}
	ap = (accessobject *)op;
	return newaccessobject(ap->ac_value, ap->ac_owner,
			       ap->ac_type, ap->ac_mode);
}

void
setaccessowner(op, owner)
	object *op;
	object *owner;
{
	register accessobject *ap;
	if (!is_accessobject(op))
		return; /* XXX no error */
	ap = (accessobject *)op;
	XDECREF(ap->ac_owner);
	XINCREF(owner);
	ap->ac_owner = owner;
}

int
hasaccessvalue(op)
	object *op;
{	
	if (!is_accessobject(op))
		return 0;
	return ((accessobject *)op)->ac_value != NULL;
}

object *
getaccessvalue(op, owner)
	object *op;
	object *owner;
{
	register accessobject *ap;
	if (!is_accessobject(op)) {
		err_badcall();
		return NULL;
	}
	ap = (accessobject *)op;
	
	if (!ownercheck(owner, ap->ac_owner, AC_R, ap->ac_mode)) {
		err_setstr(AccessError, "read access denied");
		return NULL;
	}
	
	if (ap->ac_value == NULL) {
		err_setstr(AccessError, "no current value");
		return NULL;
	}
	INCREF(ap->ac_value);
	return ap->ac_value;
}

int
setaccessvalue(op, owner, value)
	object *op;
	object *owner;
	object *value;
{
	register accessobject *ap;
	if (!is_accessobject(op)) {
		err_badcall();
		return -1;
	}
	ap = (accessobject *)op;
	
	if (!ownercheck(owner, ap->ac_owner, AC_W, ap->ac_mode)) {
		err_setstr(AccessError, "write access denied");
		return -1;
	}
	
	if (!typecheck(value, ap->ac_type)) {
		err_setstr(AccessError, "assign value of inappropriate type");
		return -1;
	}
	
	if (value == NULL) { /* Delete it */
		if (ap->ac_value == NULL) {
			err_setstr(AccessError, "no current value");
			return -1;
		}
		DECREF(ap->ac_value);
		ap->ac_value = NULL;
		return 0;
	}
	XDECREF(ap->ac_value);
	INCREF(value);
	ap->ac_value = value;
	return 0;
}

static int
typecheck(value, type)
	object *value;
	typeobject *type;
{
	object *x;
	if (value == NULL || type == NULL)
		return 1; /* No check */
	if (value->ob_type == type)
		return 1; /* Exact match */
	if (type == &Anynumbertype) {
		if (value->ob_type->tp_as_number == NULL)
			return 0;
		if (!is_instanceobject(value))
			return 1;
		/* For instances, make sure it really looks like a number */
		x = getattr(value, "__sub__");
		if (x == NULL) {
			err_clear();
			return 0;
		}
		DECREF(x);
		return 1;
	}
	if (type == &Anysequencetype) {
		if (value->ob_type->tp_as_sequence == NULL)
			return 0;
		if (!is_instanceobject(value))
			return 1;
		/* For instances, make sure it really looks like a sequence */
		x = getattr(value, "__getslice__");
		if (x == NULL) {
			err_clear();
			return 0;
		}
		DECREF(x);
		return 1;
	}
	if (type == &Anymappingtype) {
		if (value->ob_type->tp_as_mapping == NULL)
			return 0;
		if (!is_instanceobject(value))
			return 1;
		/* For instances, make sure it really looks like a mapping */
		x = getattr(value, "__getitem__");
		if (x == NULL) {
			err_clear();
			return 0;
		}
		DECREF(x);
		return 1;
	}
	return 0;
}

static int
ownercheck(caller, owner, access, mode)
	object *caller;
	object *owner;
	int access;
	int mode;
{
	int mask = AC_PUBLIC;
	if (owner != NULL) {
		if (caller == owner)
			mask |= AC_PRIVATE | AC_PROTECTED;
		else if (is_classobject(owner) && issubclass(caller, owner))
			mask |= AC_PROTECTED;
	}
	return access & mode & mask;
}

/* Access methods */

static void
access_dealloc(ap)
	accessobject *ap;
{
	XDECREF(ap->ac_value);
	XDECREF(ap->ac_owner);
	XDECREF(ap->ac_type);
	DEL(ap);
}

#define OFF(x) offsetof(accessobject, x)

static struct memberlist access_memberlist[] = {
	{"ac_value",	T_OBJECT,	OFF(ac_value)},
	{"ac_owner",	T_OBJECT,	OFF(ac_owner)},
	{"ac_type",	T_OBJECT,	OFF(ac_type)},
	{"ac_mode",	T_INT,		OFF(ac_mode)},
	{NULL}	/* Sentinel */
};

static object *
access_getattr(ap, name)
	accessobject *ap;
	char *name;
{
	return getmember((char *)ap, access_memberlist, name);
}

static object *
access_repr(ap)
	accessobject *ap;
{
	char buf[300];
	char buf2[20];
	char *ownername;
	typeobject *type = ap->ac_type;
	if (is_classobject(ap->ac_owner)) {
		ownername =
			getstringvalue(((classobject *)ap->ac_owner)->cl_name);
	}
	else {
		sprintf(buf2, "0x%lx", (long)ap->ac_owner);
		ownername = buf2;
	}
	sprintf(buf,
	"<access object, value 0x%lx, owner %.100s, type %.100s, mode %04o>",
		(long)(ap->ac_value),
		ownername,
		type ? type->tp_name : "-",
		ap->ac_mode);
	return newstringobject(buf);
}

typeobject Accesstype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"access",		/*tp_name*/
	sizeof(accessobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	access_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	access_getattr,		/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	access_repr,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};


/* Pseudo type objects to indicate collections of types */

/* XXX This should be replaced by a more general "subclassing"
   XXX mechanism for type objects... */

typeobject Anynumbertype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"*number*",		/*tp_name*/
};

/* XXX Should really distinguish mutable and immutable sequences as well */

typeobject Anysequencetype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"*sequence*",		/*tp_name*/
};

typeobject Anymappingtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"*mapping*",		/*tp_name*/
};
