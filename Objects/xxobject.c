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

/* Use this file as a template to start implementing a new object type.
   If your objects will be called foobar, start by copying this file to
   foobarobject.c, changing all occurrences of xx to foobar and all
   occurrences of Xx by Foobar.  You will probably want to delete all
   references to 'x_attr' and add your own types of attributes
   instead.  Maybe you want to name your local variables other than
   'xp'.  If your object type is needed in other files, you'll have to
   create a file "foobarobject.h"; see intobject.h for an example. */


/* Xx objects */

#include "allobjects.h"

typedef struct {
	OB_HEAD
	object	*x_attr;	/* Attributes dictionary */
} xxobject;

extern typeobject Xxtype;	/* Really static, forward */

#define is_xxobject(v)		((v)->ob_type == &Xxtype)

static xxobject *
newxxobject(arg)
	object *arg;
{
	xxobject *xp;
	xp = NEWOBJ(xxobject, &Xxtype);
	if (xp == NULL)
		return NULL;
	xp->x_attr = NULL;
	return xp;
}

/* Xx methods */

static void
xx_dealloc(xp)
	xxobject *xp;
{
	XDECREF(xp->x_attr);
	DEL(xp);
}

static object *
xx_demo(self, args)
	xxobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	INCREF(None);
	return None;
}

static struct methodlist xx_methods[] = {
	"demo",		xx_demo,
	{NULL,		NULL}		/* sentinel */
};

static object *
xx_getattr(xp, name)
	xxobject *xp;
	char *name;
{
	if (xp->x_attr != NULL) {
		object *v = dictlookup(xp->x_attr, name);
		if (v != NULL) {
			INCREF(v);
			return v;
		}
	}
	return findmethod(xx_methods, (object *)xp, name);
}

static int
xx_setattr(xp, name, v)
	xxobject *xp;
	char *name;
	object *v;
{
	if (xp->x_attr == NULL) {
		xp->x_attr = newdictobject();
		if (xp->x_attr == NULL)
			return -1;
	}
	if (v == NULL)
		return dictremove(xp->x_attr, name);
	else
		return dictinsert(xp->x_attr, name, v);
}

static typeobject Xxtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"xx",			/*tp_name*/
	sizeof(xxobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	xx_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	xx_getattr,	/*tp_getattr*/
	xx_setattr,	/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
};
