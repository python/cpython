/***********************************************************
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
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
/* MD5 module */

/* This module provides an interface to a message digest algorithm,
   MD5 in this case */

/* MD5 objects */

#include "allobjects.h"
#include "modsupport.h"		/* For getargs() etc. */

#include "md5.h"
typedef struct {
	OB_HEAD
        MD5_CTX	md5;		/* the context holder */
} md5object;

extern typeobject MD5type;	/* Really static, forward */

#define is_md5object(v)		((v)->ob_type == &MD5type)

static const char initialiser_name[] = "md5";

/* #define MD5_DEBUG */

static md5object *
newmd5object()
{
	md5object *md5p;


#ifdef MD5_DEBUG
	fputs( "md5_object() called...\n", stderr );
#endif /* def MD5_DEBUG */
	md5p = NEWOBJ(md5object, &MD5type);
	if (md5p == NULL)
		return NULL;

	MD5Init(&md5p->md5);	/* actual initialisation */
	return md5p;
} /* newmd5object() */


/* MD5 methods */

static void
md5_dealloc(md5p)
	md5object *md5p;
{
#ifdef MD5_DEBUG
	fputs( "md5_dealloc() called...\n", stderr );
#endif /* def MD5_DEBUG */

	DEL(md5p);
} /* md5_dealloc() */


/* MD5 initialisation */

static object *
MD5_md5(self, args)
	object *self;
	object *args;
{
	md5object *md5p;
	char *cp = (char *)NULL;
	int len;
	

#ifdef MD5_DEBUG
	fputs("MD5_md5() called...\n", stderr);
#endif /* def MD5_DEBUG */

	if (!getargs(args, "")) {
		err_clear();
		if (!getargs(args, "s#", &cp, &len))
			return NULL;
	}

	if ((md5p = newmd5object()) == NULL)
		return NULL;

	if (cp)
		MD5Update(&md5p->md5, cp, len);

	return (object *)md5p;
} /* MD5_md5() */


/* MD5 methods-as-attributes */
static object *
md5_update(self, args)
	md5object *self;
	object *args;
{
	char *cp;
	int len;

	
	if (!getargs(args, "s#", &cp, &len))
		return NULL;

	MD5Update(&self->md5, cp, len);

	INCREF(None);
	return None;
} /* md5_update() */

#define DIGESTLEN	16	/* this is used twice--walrus@umich.edu */
static object *
md5_digest(self, args)
	md5object *self;
	object *args;
{

	MD5_CTX mdContext;
	char aDigest[DIGESTLEN];
	

	if (!getnoarg(args))
		return NULL;

	/* make a temporary copy, and perform the final */
	mdContext = self->md5;
	MD5Final(aDigest, &mdContext);

	return newsizedstringobject((char *)aDigest, DIGESTLEN);
} /* md5_digest() */
#undef DIGESTLEN

static object *
md5_copy(self, args)
	md5object *self;
	object *args;
{
	md5object *md5p;

	
	if (!getnoarg(args))
		return NULL;

	if ((md5p = newmd5object()) == NULL)
		return NULL;

	md5p->md5 = self->md5;

	return (object *)md5p;
} /* md5_copy() */

		
static struct methodlist md5_methods[] = {
	{"update",		md5_update},
	{"digest",		md5_digest},
	{"copy",		md5_copy},
	{NULL,			NULL}		/* sentinel */
};

static object *
md5_getattr(self, name)
	md5object *self;
	char *name;
{
	return findmethod(md5_methods, (object *)self, name);
} /* md5_getattr() */

#ifndef _AIX
static
#endif
typeobject MD5type = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"md5",			/*tp_name*/
	sizeof(md5object),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	md5_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	md5_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
        0,		 /*tp_as_number*/
};

/* List of functions exported by this module */

static struct methodlist md5_functions[] = {
	{initialiser_name,	MD5_md5},
	{NULL,			NULL}		 /* Sentinel */
};


/* Initialize this module. */

void
initmd5()
{
#ifdef MD5_DEBUG
	fputs( "initmd5() called...\n", stderr );
#endif /* def MD5_DEBUG */
	(void)initmodule("md5", md5_functions);
} /* initmd5() */

#ifdef MAKEDUMMYINT
int _md5_dummy_int;	/* XXX otherwise, we're .bss-less (DYNLOAD->Jack?) */
#endif /* def MAKEDUMMYINT */
