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

/* Use this file as a template to start implementing a module that
   also declares objects types. All occurrences of 'xxo' should be changed
   to something reasonable for your objects. After that, all other
   occurrences of 'xx' should be changed to something reasonable for your
   module. If your module is named foo your sourcefile should be named
   foomodule.c.
   
   You will probably want to delete all references to 'x_attr' and add
   your own types of attributes instead.  Maybe you want to name your
   local variables other than 'self'.  If your object type is needed in
   other files, you'll have to create a file "foobarobject.h"; see
   intobject.h for an example. */

/* Xxo objects */

#include "allobjects.h"

static object *ErrorObject;

typedef struct {
	OB_HEAD
	object	*x_attr;	/* Attributes dictionary */
} xxoobject;

staticforward typeobject Xxotype;

#define is_xxoobject(v)		((v)->ob_type == &Xxotype)

static xxoobject *
newxxoobject(arg)
	object *arg;
{
	xxoobject *self;
	self = NEWOBJ(xxoobject, &Xxotype);
	if (self == NULL)
		return NULL;
	self->x_attr = NULL;
	return self;
}

/* Xxo methods */

static void
xxo_dealloc(self)
	xxoobject *self;
{
	XDECREF(self->x_attr);
	DEL(self);
}

static object *
xxo_demo(self, args)
	xxoobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	INCREF(None);
	return None;
}

static struct methodlist xxo_methods[] = {
	{"demo",	(method)xxo_demo},
	{NULL,		NULL}		/* sentinel */
};

static object *
xxo_getattr(self, name)
	xxoobject *self;
	char *name;
{
	if (self->x_attr != NULL) {
		object *v = dictlookup(self->x_attr, name);
		if (v != NULL) {
			INCREF(v);
			return v;
		}
	}
	return findmethod(xxo_methods, (object *)self, name);
}

static int
xxo_setattr(self, name, v)
	xxoobject *self;
	char *name;
	object *v;
{
	if (self->x_attr == NULL) {
		self->x_attr = newdictobject();
		if (self->x_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = dictremove(self->x_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
			        "delete non-existing xxo attribute");
		return rv;
	}
	else
		return dictinsert(self->x_attr, name, v);
}

staticforward typeobject Xxotype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"xxo",			/*tp_name*/
	sizeof(xxoobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)xxo_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)xxo_getattr, /*tp_getattr*/
	(setattrfunc)xxo_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
/* --------------------------------------------------------------------- */

/* Function of two integers returning integer */

static object *
xx_foo(self, args)
	object *self; /* Not used */
	object *args;
{
	long i, j;
	long res;
	if (!getargs(args, "(ll)", &i, &j))
		return NULL;
	res = i+j; /* XXX Do something here */
	return newintobject(res);
}


/* Function of no arguments returning new xxo object */

static object *
xx_new(self, args)
	object *self; /* Not used */
	object *args;
{
	int i, j;
	xxoobject *rv;
	
	if (!getnoarg(args))
		return NULL;
	rv = newxxoobject(args);
	if ( rv == NULL )
	    return NULL;
	return (object *)rv;
}


/* List of functions defined in the module */

static struct methodlist xx_methods[] = {
	{"foo",		xx_foo},
	{"new",		xx_new},
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initxx) */

void
initxx()
{
	object *m, *d;

	/* Create the module and add the functions */
	m = initmodule("xx", xx_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	ErrorObject = newstringobject("xx.error");
	dictinsert(d, "error", ErrorObject);

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module xx");
}
