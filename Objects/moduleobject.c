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

/* Module object implementation */

#include "allobjects.h"
#include "ceval.h"

typedef struct {
	OB_HEAD
	object *md_dict;
} moduleobject;

object *
newmoduleobject(name)
	char *name;
{
	moduleobject *m;
	object *nameobj;
	m = NEWOBJ(moduleobject, &Moduletype);
	if (m == NULL)
		return NULL;
	nameobj = newstringobject(name);
	m->md_dict = newdictobject();
	if (m->md_dict == NULL || nameobj == NULL)
		goto fail;
	if (dictinsert(m->md_dict, "__name__", nameobj) != 0)
		goto fail;
	DECREF(nameobj);
	return (object *)m;

 fail:
	XDECREF(nameobj);
	DECREF(m);
	return NULL;
}

object *
getmoduledict(m)
	object *m;
{
	if (!is_moduleobject(m)) {
		err_badcall();
		return NULL;
	}
	return ((moduleobject *)m) -> md_dict;
}

char *
getmodulename(m)
	object *m;
{
	object *nameobj;
	if (!is_moduleobject(m)) {
		err_badarg();
		return NULL;
	}
	nameobj = dictlookup(((moduleobject *)m)->md_dict, "__name__");
	if (nameobj == NULL || !is_stringobject(nameobj)) {
		err_setstr(SystemError, "nameless module");
		return NULL;
	}
	return getstringvalue(nameobj);
}

/* Methods */

static void
module_dealloc(m)
	moduleobject *m;
{
	if (m->md_dict != NULL)
		DECREF(m->md_dict);
	free((char *)m);
}

static object *
module_repr(m)
	moduleobject *m;
{
	char buf[100];
	char *name = getmodulename((object *)m);
	if (name == NULL) {
		err_clear();
		name = "?";
	}
	sprintf(buf, "<module '%.80s'>", name);
	return newstringobject(buf);
}

static object *
module_getattr(m, name)
	moduleobject *m;
	char *name;
{
	object *res;
	if (strcmp(name, "__dict__") == 0) {
		INCREF(m->md_dict);
		return m->md_dict;
	}
	res = dictlookup(m->md_dict, name);
	if (res == NULL)
		err_setstr(AttributeError, name);
	else {
		if (is_accessobject(res))
			res = getaccessvalue(res, getglobals());
		else
			INCREF(res);
	}
	return res;
}

static int
module_setattr(m, name, v)
	moduleobject *m;
	char *name;
	object *v;
{
	object *ac;
	if (name[0] == '_' && strcmp(name, "__dict__") == 0) {
		err_setstr(TypeError, "read-only special attribute");
		return -1;
	}
	ac = dictlookup(m->md_dict, name);
	if (ac != NULL && is_accessobject(ac))
		return setaccessvalue(ac, getglobals(), v);
	if (v == NULL) {
		int rv = dictremove(m->md_dict, name);
		if (rv < 0)
			err_setstr(AttributeError,
				   "delete non-existing module attribute");
		return rv;
	}
	else
		return dictinsert(m->md_dict, name, v);
}

typeobject Moduletype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"module",		/*tp_name*/
	sizeof(moduleobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	(destructor)module_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)module_getattr, /*tp_getattr*/
	(setattrfunc)module_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	(reprfunc)module_repr, /*tp_repr*/
};
