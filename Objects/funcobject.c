/* Function object implementation */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "funcobject.h"
#include "objimpl.h"

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

static void
funcdealloc(op)
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
	funcdealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
};
