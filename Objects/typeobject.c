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

/* Type object implementation */

#include "allobjects.h"

/* Type object implementation */

static void
type_print(v, fp, flags)
	typeobject *v;
	FILE *fp;
	int flags;
{
	fprintf(fp, "<type '%s'>", v->tp_name);
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
	type_print,		/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	type_repr,		/*tp_repr*/
};
