/* Function object implementation */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "node.h"
#include "stringobject.h"
#include "funcobject.h"
#include "objimpl.h"
#include "token.h"

typedef struct {
	OB_HEAD
	node *func_node;
	object *func_globals;
} funcobject;

object *
newfuncobject(n, globals)
	node *n;
	object *globals;
{
	funcobject *op = NEWOBJ(funcobject, &Functype);
	if (op != NULL) {
		op->func_node = n;
		if (globals != NULL)
			INCREF(globals);
		op->func_globals = globals;
	}
	return (object *)op;
}

node *
getfuncnode(op)
	object *op;
{
	if (!is_funcobject(op)) {
		errno = EBADF;
		return NULL;
	}
	return ((funcobject *) op) -> func_node;
}

object *
getfuncglobals(op)
	object *op;
{
	if (!is_funcobject(op)) {
		errno = EBADF;
		return NULL;
	}
	return ((funcobject *) op) -> func_globals;
}

/* Methods */

static void
funcdealloc(op)
	funcobject *op;
{
	/* XXX free node? */
	DECREF(op->func_globals);
	free((char *)op);
}

static void
funcprint(op, fp, flags)
	funcobject *op;
	FILE *fp;
	int flags;
{
	node *n = op->func_node;
	n = CHILD(n, 1);
	fprintf(fp, "<user function %s>", STR(n));
}

static object *
funcrepr(op)
	funcobject *op;
{
	char buf[100];
	node *n = op->func_node;
	n = CHILD(n, 1);
	sprintf(buf, "<user function %.80s>", STR(n));
	return newstringobject(buf);
}

typeobject Functype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"function",
	sizeof(funcobject),
	0,
	funcdealloc,	/*tp_dealloc*/
	funcprint,	/*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	funcrepr,	/*tp_repr*/
};
