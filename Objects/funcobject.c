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

/* Function object implementation */

#include "allobjects.h"

#include "structmember.h"

typedef struct {
	OB_HEAD
	object *func_code;
	object *func_globals;
} funcobject;

object *
newfuncobject(code, globals)
	object *code;
	object *globals;
{
	funcobject *op = NEWOBJ(funcobject, &Functype);
	if (op != NULL) {
		INCREF(code);
		op->func_code = code;
		INCREF(globals);
		op->func_globals = globals;
	}
	return (object *)op;
}

object *
getfunccode(op)
	object *op;
{
	if (!is_funcobject(op)) {
		err_badcall();
		return NULL;
	}
	return ((funcobject *) op) -> func_code;
}

object *
getfuncglobals(op)
	object *op;
{
	if (!is_funcobject(op)) {
		err_badcall();
		return NULL;
	}
	return ((funcobject *) op) -> func_globals;
}

/* Methods */

#define OFF(x) offsetof(funcobject, x)

static struct memberlist func_memberlist[] = {
	{"func_code",	T_OBJECT,	OFF(func_code)},
	{"func_globals",T_OBJECT,	OFF(func_globals)},
	{NULL}	/* Sentinel */
};

static object *
func_getattr(op, name)
	funcobject *op;
	char *name;
{
	return getmember((char *)op, func_memberlist, name);
}

static void
func_dealloc(op)
	funcobject *op;
{
	DECREF(op->func_code);
	DECREF(op->func_globals);
	DEL(op);
}

typeobject Functype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"function",
	sizeof(funcobject),
	0,
	func_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	func_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
};
