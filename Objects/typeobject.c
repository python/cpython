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

/* Type object implementation */

#include "allobjects.h"

/* Type object implementation */

static object *
type_getattr(t, name)
	typeobject *t;
	char *name;
{
	if (strcmp(name, "__name__") == 0)
		return newstringobject(t->tp_name);
	if (strcmp(name, "__doc__") == 0) {
		char *doc = t->tp_doc;
		if (doc != NULL)
			return newstringobject(doc);
		INCREF(None);
		return None;
	}
	if (strcmp(name, "__members__") == 0)
		return mkvalue("[ss]", "__doc__", "__name__");
	err_setstr(AttributeError, name);
	return NULL;
}

static object *
type_repr(v)
	typeobject *v;
{
	char buf[100];
	sprintf(buf, "<type '%.80s'>", v->tp_name);
	return newstringobject(buf);
}

typeobject Typetype = {
	OB_HEAD_INIT(&Typetype)
	0,			/* Number of items for varobject */
	"type",			/* Name of this type */
	sizeof(typeobject),	/* Basic object size */
	0,			/* Item size for varobject */
	0,			/*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)type_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	(reprfunc)type_repr,	/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/*tp_call*/
	0,			/*tp_str*/
	0,			/*tp_xxx1*/
	0,			/*tp_xxx2*/
	0,			/*tp_xxx3*/
	0,			/*tp_xxx4*/
	"Define the behaviour of a particular type of object.",
};
