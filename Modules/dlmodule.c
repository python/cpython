/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* dl module */

#include "allobjects.h"
#include "modsupport.h"

#include <dlfcn.h>

#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif

typedef struct {
	OB_HEAD
	ANY *dl_handle;
} dlobject;

staticforward typeobject Dltype;

static object *Dlerror;

static object *
newdlobject(handle)
	ANY *handle;
{
	dlobject *xp;
	xp = NEWOBJ(dlobject, &Dltype);
	if (xp == NULL)
		return NULL;
	xp->dl_handle = handle;
	return (object *)xp;
}

static void
dl_dealloc(xp)
	dlobject *xp;
{
	if (xp->dl_handle != NULL)
		dlclose(xp->dl_handle);
	DEL(xp);
}

static object *
dl_close(xp, args)
	dlobject *xp;
	object *args;
{
	if (!getargs(args, ""))
		return NULL;
	if (xp->dl_handle != NULL) {
		dlclose(xp->dl_handle);
		xp->dl_handle = NULL;
	}
	INCREF(None);
	return None;
}

static object *
dl_sym(xp, args)
	dlobject *xp;
	object *args;
{
	char *name;
	ANY *func;
	if (!getargs(args, "s", &name))
		return NULL;
	func = dlsym(xp->dl_handle, name);
	if (func == NULL) {
		INCREF(None);
		return None;
	}
	return newintobject((long)func);
}

static object *
dl_call(xp, args)
	dlobject *xp;
	object *args; /* (varargs) */
{
	object *name;
	long (*func)();
	long alist[10];
	long res;
	int i;
	int n = gettuplesize(args);
	if (n < 1) {
		err_setstr(TypeError, "at least a name is needed");
		return NULL;
	}
	name = gettupleitem(args, 0);
	if (!is_stringobject(name)) {
		err_setstr(TypeError, "function name must be a string");
		return NULL;
	}
	func = dlsym(xp->dl_handle, getstringvalue(name));
	if (func == NULL) {
		err_setstr(ValueError, dlerror());
		return NULL;
	}
	if (n-1 > 10) {
		err_setstr(TypeError, "too many arguments (max 10)");
		return NULL;
	}
	for (i = 1; i < n; i++) {
		object *v = gettupleitem(args, i);
		if (is_intobject(v))
			alist[i-1] = getintvalue(v);
		else if (is_stringobject(v))
			alist[i-1] = (long)getstringvalue(v);
		else if (v == None)
			alist[i-1] = (long) ((char *)NULL);
		else {
			err_setstr(TypeError,
				   "arguments must be int, string or None");
			return NULL;
		}
	}
	for (; i <= 10; i++)
		alist[i-1] = 0;
	res = (*func)(alist[0], alist[1], alist[2], alist[3], alist[4],
		      alist[5], alist[6], alist[7], alist[8], alist[9]);
	return newintobject(res);
}

static struct methodlist dlobject_methods[] = {
	{"call",	(method)dl_call,	1 /* varargs */},
	{"sym", 	(method)dl_sym},
	{"close",	(method)dl_close},
	{NULL,  	NULL}			/* Sentinel */
};

static object *
dl_getattr(xp, name)
	dlobject *xp;
	char *name;
{
	return findmethod(dlobject_methods, (object *)xp, name);
}


static typeobject Dltype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"dl",			/*tp_name*/
	sizeof(dlobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)dl_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)dl_getattr,/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

static object *
dl_open(self, args)
	object *self;
	object *args;
{
	char *name;
	int mode;
	ANY *handle;
	if (getargs(args, "z", &name))
		mode = RTLD_LAZY;
	else {
		err_clear();
		if (!getargs(args, "(zi)", &name, &mode))
			return NULL;
#ifndef RTLD_NOW
		if (mode != RTLD_LAZY) {
			err_setstr(ValueError, "mode must be 1");
			return NULL;
		}
#endif
	}
	handle = dlopen(name, mode);
	if (handle == NULL) {
		err_setstr(Dlerror, dlerror());
		return NULL;
	}
	return newdlobject(handle);
}

static struct methodlist dl_methods[] = {
	{"open",	dl_open},
	{NULL,		NULL}		/* sentinel */
};

void
initdl()
{
	object *m, *d, *x;

	if (sizeof(int) != sizeof(long) ||
	    sizeof(long) != sizeof(char *))
		fatal(
 "module dl requires sizeof(int) == sizeof(long) == sizeof(char*)");

	/* Create the module and add the functions */
	m = initmodule("dl", dl_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	Dlerror = x = newstringobject("dl.error");
	dictinsert(d, "error", x);
	x = newintobject((long)RTLD_LAZY);
	dictinsert(d, "RTLD_LAZY", x);
#ifdef RTLD_NOW
	x = newintobject((long)RTLD_NOW);
	dictinsert(d, "RTLD_NOW", x);
#endif

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module dl");
}
