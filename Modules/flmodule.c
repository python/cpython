/**********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
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

/* FL module -- interface to Mark Overmars' FORMS Library. */

/* This code works with FORMS version 2.2a.
   FORMS can be ftp'ed from ftp.cs.ruu.nl (131.211.80.17), directory
   /pub/SGI/FORMS. */

/* A half-hearted attempt has been made to allow programs using this
 * module to exploit parallelism (through the threads module). No provisions
 * have been made for multiple threads to use this module at the same time,
 * though. So, a program with a forms thread and a non-forms thread will work
 * fine but a program with two threads using forms will probably crash (unless
 * the program takes precaution to ensure that only one thread can be in
 * this module at any time). This will have to be fixed some time.
 * (A fix will probably also have to synchronise with the gl module).
 */

#include "forms.h"

#include "allobjects.h"
#include "import.h"
#include "modsupport.h"
#include "structmember.h"
#include "ceval.h"

/* Generic Forms Objects */

typedef struct {
	OB_HEAD
	FL_OBJECT *ob_generic;
	struct methodlist *ob_methods;
	object *ob_callback;
	object *ob_callback_arg;
} genericobject;

extern typeobject GenericObjecttype; /* Forward */

#define is_genericobject(g) ((g)->ob_type == &GenericObjecttype)

/* List of all objects (XXX this should be a hash table on address...) */

static object *allgenerics = NULL;
static int nfreeslots = 0;

/* Add an object to the list of known objects */

static void
knowgeneric(g)
	genericobject *g;
{
	int i, n;
	/* Create the list if it doesn't already exist */
	if (allgenerics == NULL) {
		allgenerics = newlistobject(0);
		if (allgenerics == NULL) {
			err_clear();
			return; /* Too bad, live without allgenerics... */
		}
	}
	if (nfreeslots > 0) {
		/* Search the list for reusable slots (NULL items) */
		/* XXX This can be made faster! */
		n = getlistsize(allgenerics);
		for (i = 0; i < n; i++) {
			if (getlistitem(allgenerics, i) == NULL) {
				INCREF(g);
				setlistitem(allgenerics, i, (object *)g);
				nfreeslots--;
				return;
			}
		}
		/* Strange... no free slots found... */
		nfreeslots = 0;
	}
	/* No free entries, append new item to the end */
	addlistitem(allgenerics, (object *)g);
}

/* Find an object in the list of known objects */

static genericobject *
findgeneric(generic)
	FL_OBJECT *generic;
{
	int i, n;
	genericobject *g;
	
	if (allgenerics == NULL)
		return NULL; /* No objects known yet */
	n = getlistsize(allgenerics);
	for (i = 0; i < n; i++) {
		g = (genericobject *)getlistitem(allgenerics, i);
		if (g != NULL && g->ob_generic == generic)
			return g;
	}
	return NULL; /* Unknown object */
}

/* Remove an object from the list of known objects */

static void
forgetgeneric(g)
	genericobject *g;
{
	int i, n;
	
	XDECREF(g->ob_callback);
	g->ob_callback = NULL;
	XDECREF(g->ob_callback_arg);
	g->ob_callback_arg = NULL;
	if (allgenerics == NULL)
		return; /* No objects known yet */
	n = getlistsize(allgenerics);
	for (i = 0; i < n; i++) {
		if (g == (genericobject *)getlistitem(allgenerics, i)) {
			setlistitem(allgenerics, i, (object *)NULL);
			nfreeslots++;
			break;
		}
	}
}

/* Called when a form is about to be freed --
   remove all the objects that we know about from it. */

static void
releaseobjects(form)
	FL_FORM *form;
{
	int i, n;
	genericobject *g;
	
	if (allgenerics == NULL)
		return; /* No objects known yet */
	n = getlistsize(allgenerics);
	for (i = 0; i < n; i++) {
		g = (genericobject *)getlistitem(allgenerics, i);
		if (g != NULL && g->ob_generic->form == form) {
			fl_delete_object(g->ob_generic);
			/* The object is now unreachable for
			   do_forms and check_forms, so
			   delete it from the list of known objects */
			XDECREF(g->ob_callback);
			g->ob_callback = NULL;
			XDECREF(g->ob_callback_arg);
			g->ob_callback_arg = NULL;
			setlistitem(allgenerics, i, (object *)NULL);
			nfreeslots++;
		}
	}
}


/* Methods of generic objects */

static object *
generic_set_call_back(g, args)
	genericobject *g;
	object *args;
{
	if (args == NULL) {
		XDECREF(g->ob_callback);
		XDECREF(g->ob_callback_arg);
		g->ob_callback = NULL;
		g->ob_callback_arg = NULL;
	}
	else {
		if (!is_tupleobject(args) || gettuplesize(args) != 2) {
			err_badarg();
			return NULL;
		}
		XDECREF(g->ob_callback);
		XDECREF(g->ob_callback_arg);
		g->ob_callback = gettupleitem(args, 0);
		INCREF(g->ob_callback);
		g->ob_callback_arg = gettupleitem(args, 1);
		INCREF(g->ob_callback_arg);
	}
	INCREF(None);
	return None;
}

static object *
generic_call(g, args, func)
	genericobject *g;
	object *args;
	void (*func)(FL_OBJECT *);
{
	if (!getnoarg(args))
		return NULL;
	(*func)(g->ob_generic);
	INCREF(None);
	return None;
}

static object *
generic_delete_object(g, args)
	genericobject *g;
	object *args;
{
	object *res;
	res = generic_call(g, args, fl_delete_object);
	if (res != NULL)
		forgetgeneric(g);
	return res;
}

static object *
generic_show_object(g, args)
	genericobject *g;
	object *args;
{
	return generic_call(g, args, fl_show_object);
}

static object *
generic_hide_object(g, args)
	genericobject *g;
	object *args;
{
	return generic_call(g, args, fl_hide_object);
}

static object *
generic_redraw_object(g, args)
	genericobject *g;
	object *args;
{
	return generic_call(g, args, fl_redraw_object);
}

static object *
generic_freeze_object(g, args)
	genericobject *g;
	object *args;
{
	return generic_call(g, args, fl_freeze_object);
}

static object *
generic_unfreeze_object(g, args)
	genericobject *g;
	object *args;
{
	return generic_call(g, args, fl_unfreeze_object);
}

static object *
generic_activate_object(g, args)
	genericobject *g;
	object *args;
{
	return generic_call(g, args, fl_activate_object);
}

static object *
generic_deactivate_object(g, args)
	genericobject *g;
	object *args;
{
	return generic_call(g, args, fl_deactivate_object);
}

static object *
generic_set_object_shortcut(g, args)
	genericobject *g;
	object *args;
{
	char *str;
	if (!getargs(args, "s", &str))
		return NULL;
	fl_set_object_shortcut(g->ob_generic, str);
	INCREF(None);
	return None;
}

static struct methodlist generic_methods[] = {
	{"set_call_back",	generic_set_call_back},
	{"delete_object",	generic_delete_object},
	{"show_object",		generic_show_object},
	{"hide_object",		generic_hide_object},
	{"redraw_object",	generic_redraw_object},
	{"freeze_object",	generic_freeze_object},
	{"unfreeze_object",	generic_unfreeze_object},
	{"activate_object",	generic_activate_object},
	{"deactivate_object",	generic_deactivate_object},
	{"set_object_shortcut",	generic_set_object_shortcut},
	{NULL,			NULL}		/* sentinel */
};

static void
generic_dealloc(g)
	genericobject *g;
{
	fl_free_object(g->ob_generic);
	XDECREF(g->ob_callback);
	XDECREF(g->ob_callback_arg);
	DEL(g);
}

#define OFF(x) offsetof(FL_OBJECT, x)

static struct memberlist generic_memberlist[] = {
	{"objclass",	T_INT,		OFF(objclass),	RO},
	{"type",	T_INT,		OFF(type),	RO},
	{"boxtype",	T_INT,		OFF(boxtype)},
	{"x",		T_FLOAT,	OFF(x)},
	{"y",		T_FLOAT,	OFF(y)},
	{"w",		T_FLOAT,	OFF(w)},
	{"h",		T_FLOAT,	OFF(h)},
	{"col1",	T_INT,		OFF(col1)},
	{"col2",	T_INT,		OFF(col2)},
	{"align",	T_INT,		OFF(align)},
	{"lcol",	T_INT,		OFF(lcol)},
	{"lsize",	T_FLOAT,	OFF(lsize)},
	/* "label" is treated specially! */
	{"lstyle",	T_INT,		OFF(lstyle)},
	{"pushed",	T_INT,		OFF(pushed),	RO},
	{"focus",	T_INT,		OFF(focus),	RO},
	{"belowmouse",	T_INT,		OFF(belowmouse),RO},
/*	{"frozen",	T_INT,		OFF(frozen),	RO},	*/
	{"active",	T_INT,		OFF(active)},
	{"input",	T_INT,		OFF(input)},
	{"visible",	T_INT,		OFF(visible),	RO},
	{"radio",	T_INT,		OFF(radio)},
	{"automatic",	T_INT,		OFF(automatic)},
	{NULL}	/* Sentinel */
};

#undef OFF

static object *
generic_getattr(g, name)
	genericobject *g;
	char *name;
{
	object *meth;

	/* XXX Ought to special-case name "__methods__" */
	if (g-> ob_methods) {
		meth = findmethod(g->ob_methods, (object *)g, name);
		if (meth != NULL) return meth;
		err_clear();
	}

	meth = findmethod(generic_methods, (object *)g, name);
	if (meth != NULL)
		return meth;
	err_clear();

	/* "label" is an exception, getmember only works for char pointers,
	   not for char arrays */
	if (strcmp(name, "label") == 0)
		return newstringobject(g->ob_generic->label);

	return getmember((char *)g->ob_generic, generic_memberlist, name);
}

static int
generic_setattr(g, name, v)
	genericobject *g;
	char *name;
	object *v;
{
	int ret;

	if (v == NULL) {
		err_setstr(TypeError, "can't delete forms object attributes");
		return -1;
	}

	/* "label" is an exception: setmember doesn't set strings;
	   and FORMS wants you to call a function to set the label */
	if (strcmp(name, "label") == 0) {
		if (!is_stringobject(v)) {
			err_setstr(TypeError, "label attr must be string");
			return -1;
		}
		fl_set_object_label(g->ob_generic, getstringvalue(v));
		return 0;
	}

	ret = setmember((char *)g->ob_generic, generic_memberlist, name, v);

	/* Rather than calling all the various set_object_* functions,
	   we call fl_redraw_object here.  This is sometimes redundant
	   but I doubt that's a big problem */
	if (ret == 0)
		fl_redraw_object(g->ob_generic);

	return ret;
}

static object *
generic_repr(g)
	genericobject *g;
{
	char buf[100];
	sprintf(buf, "<FORMS_object at %lx, objclass=%d>",
		(long)g, g->ob_generic->objclass);
	return newstringobject(buf);
}

typeobject GenericObjecttype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"FORMS_object",		/*tp_name*/
	sizeof(genericobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	generic_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	generic_getattr,	/*tp_getattr*/
	generic_setattr,	/*tp_setattr*/
	0,			/*tp_compare*/
	generic_repr,		/*tp_repr*/
};

static object *
newgenericobject(generic, methods)
	FL_OBJECT *generic;
	struct methodlist *methods;
{
	genericobject *g;
	g = NEWOBJ(genericobject, &GenericObjecttype);
	if (g == NULL)
		return NULL;
	g-> ob_generic = generic;
	g->ob_methods = methods;
	g->ob_callback = NULL;
	g->ob_callback_arg = NULL;
	knowgeneric(g);
	return (object *)g;
}

/**********************************************************************/
/* Some common calling sequences */

/* void func (object, float) */
static object *
call_forms_INf (func, obj, args)
	void (*func)(FL_OBJECT *, float);
	FL_OBJECT *obj;
	object *args;
{
	float parameter;

	if (!getargs(args, "f", &parameter)) return NULL;

	(*func) (obj, parameter);

	INCREF(None);
	return None;
}

/* void func (object, float) */
static object *
call_forms_INfINf (func, obj, args)
	void (*func)(FL_OBJECT *, float, float);
	FL_OBJECT *obj;
	object *args;
{
	float par1, par2;

	if (!getargs(args, "(ff)", &par1, &par2)) return NULL;

	(*func) (obj, par1, par2);

	INCREF(None);
	return None;
}

/* void func (object, int) */
static object *
call_forms_INi (func, obj, args)
	void (*func)(FL_OBJECT *, int);
	FL_OBJECT *obj;
	object *args;
{
	int parameter;

	if (!getintarg(args, &parameter)) return NULL;

	(*func) (obj, parameter);

	INCREF(None);
	return None;
}

/* void func (object, char) */
static object *
call_forms_INc (func, obj, args)
	void (*func)(FL_OBJECT *, int);
	FL_OBJECT *obj;
	object *args;
{
	char *a;

	if (!getstrarg(args, &a)) return NULL;

	(*func) (obj, a[0]);

	INCREF(None);
	return None;
}

/* void func (object, string) */
static object *
call_forms_INstr (func, obj, args)
	void (*func)(FL_OBJECT *, char *);
	FL_OBJECT *obj;
	object *args;
{
	char *a;

	if (!getstrarg(args, &a)) return NULL;

	(*func) (obj, a);

	INCREF(None);
	return None;
}


/* void func (object, int, string) */
static object *
call_forms_INiINstr (func, obj, args)
	void (*func)(FL_OBJECT *, int, char *);
	FL_OBJECT *obj;
	object *args;
{
	char *b;
	int a;
	
	if (!getargs(args, "(is)", &a, &b)) return NULL;
	
	(*func) (obj, a, b);
	
	INCREF(None);
	return None;
}

#ifdef UNUSED
/* void func (object, int, int) */
static object *
call_forms_INiINi (func, obj, args)
	void (*func)(FL_OBJECT *, int, int);
	FL_OBJECT *obj;
	object *args;
{
	int par1, par2;
	
	if (!getargs(args, "(ii)", &par1, &par2)) return NULL;
	
	(*func) (obj, par1, par2);
	
	INCREF(None);
	return None;
}
#endif

/* int func (object) */
static object *
call_forms_Ri (func, obj, args)
	int (*func)(FL_OBJECT *);
	FL_OBJECT *obj;
	object *args;
{
	int retval;
	
	if (!getnoarg(args)) return NULL;
	
	retval = (*func) (obj);
	
	return newintobject ((long) retval);
}

/* char * func (object) */
static object *
call_forms_Rstr (func, obj, args)
	char * (*func)(FL_OBJECT *);
	FL_OBJECT *obj;
	object *args;
{
	char *str;
	
	if (!getnoarg(args)) return NULL;
	
	str = (*func) (obj);
	
	if (str == NULL) {
		INCREF(None);
		return None;
	}
	return newstringobject (str);
}

/* int func (object) */
static object *
call_forms_Rf (func, obj, args)
	float (*func)(FL_OBJECT *);
	FL_OBJECT *obj;
	object *args;
{
	float retval;
	
	if (!getnoarg(args)) return NULL;
	
	retval = (*func) (obj);
	
	return newfloatobject (retval);
}

static object *
call_forms_OUTfOUTf (func, obj, args)
	void (*func)(FL_OBJECT *, float *, float *);
	FL_OBJECT *obj;
	object *args;
{
	float f1, f2;
	
	if (!getnoarg(args)) return NULL;
	
	(*func) (obj, &f1, &f2);

	return mkvalue("(ff)", f1, f2);
}

#ifdef UNUSED
static object *
call_forms_OUTf (func, obj, args)
	void (*func)(FL_OBJECT *, float *);
	FL_OBJECT *obj;
	object *args;
{
	float f;

	if (!getnoarg(args)) return NULL;

	(*func) (obj, &f);

	return newfloatobject (f);
}
#endif

/**********************************************************************/
/* Class : browser */

static object *
set_browser_topline(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_browser_topline, g-> ob_generic, args);
}

static object *
clear_browser(g, args)
	genericobject *g;
	object *args;
{
	return generic_call (g, args, fl_clear_browser);
}

static object *
add_browser_line (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_add_browser_line, g-> ob_generic, args);
}

static object *
addto_browser (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_addto_browser, g-> ob_generic, args);
}

static object *
insert_browser_line (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INiINstr (fl_insert_browser_line,
				    g-> ob_generic, args);
}

static object *
delete_browser_line (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_delete_browser_line, g-> ob_generic, args);
}

static object *
replace_browser_line (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INiINstr (fl_replace_browser_line,
				    g-> ob_generic, args);
}

static object *
get_browser_line(g, args)
	genericobject *g;
	object *args;
{
	int i;
	char *str;

	if (!getintarg(args, &i))
		return NULL;

	str = fl_get_browser_line (g->ob_generic, i);

	if (str == NULL) {
		INCREF(None);
		return None;
	}
	return newstringobject (str);
}

static object *
load_browser (g, args)
	genericobject *g;
	object *args;
{
	/* XXX strictly speaking this is wrong since fl_load_browser
	   XXX returns int, not void */
	return call_forms_INstr (fl_load_browser, g-> ob_generic, args);
}

static object *
get_browser_maxline(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Ri (fl_get_browser_maxline, g-> ob_generic, args);
}

static object *
select_browser_line (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_select_browser_line, g-> ob_generic, args);
}

static object *
deselect_browser_line (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_deselect_browser_line, g-> ob_generic, args);
}

static object *
deselect_browser (g, args)
	genericobject *g;
	object *args;
{
	return generic_call (g, args, fl_deselect_browser);
}

static object *
isselected_browser_line (g, args)
	genericobject *g;
	object *args;
{
	int i, j;
	
	if (!getintarg(args, &i))
		return NULL;
	
	j = fl_isselected_browser_line (g->ob_generic, i);
	
	return newintobject (j);
}

static object *
get_browser (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Ri (fl_get_browser, g-> ob_generic, args);
}

static object *
set_browser_fontsize (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_browser_fontsize, g-> ob_generic, args);
}

static object *
set_browser_fontstyle (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_browser_fontstyle, g-> ob_generic, args);
}

static object *
set_browser_specialkey (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INc(fl_set_browser_specialkey, g-> ob_generic, args);
}

static struct methodlist browser_methods[] = {
	{"set_browser_topline",		set_browser_topline},
	{"clear_browser",		clear_browser},
	{"add_browser_line",		add_browser_line},
	{"addto_browser",		addto_browser},
	{"insert_browser_line",		insert_browser_line},
	{"delete_browser_line",		delete_browser_line},
	{"replace_browser_line",	replace_browser_line},
	{"get_browser_line",		get_browser_line},
	{"load_browser",		load_browser},
	{"get_browser_maxline",		get_browser_maxline},
	{"select_browser_line",		select_browser_line},
	{"deselect_browser_line",	deselect_browser_line},
	{"deselect_browser",		deselect_browser},
	{"isselected_browser_line",	isselected_browser_line},
	{"get_browser",			get_browser},
	{"set_browser_fontsize",	set_browser_fontsize},
	{"set_browser_fontstyle",	set_browser_fontstyle},
	{"set_browser_specialkey",	set_browser_specialkey},
	{NULL,				NULL}		/* sentinel */
};

/* Class: button */

static object *
set_button(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_button, g-> ob_generic, args);
}

static object *
get_button(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Ri (fl_get_button, g-> ob_generic, args);
}

static object *
get_button_numb(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Ri (fl_get_button_numb, g-> ob_generic, args);
}

static object *
set_button_shortcut(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_set_button_shortcut, g-> ob_generic, args);
}

static struct methodlist button_methods[] = {
	{"set_button",		set_button},
	{"get_button",		get_button},
	{"get_button_numb",	get_button_numb},
	{"set_button_shortcut",	set_button_shortcut},
	{NULL,			NULL}		/* sentinel */
};

/* Class: choice */

static object *
set_choice(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_choice, g-> ob_generic, args);
}

static object *
get_choice(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Ri (fl_get_choice, g-> ob_generic, args);
}

static object *
clear_choice (g, args)
	genericobject *g;
	object *args;
{
	return generic_call (g, args, fl_clear_choice);
}

static object *
addto_choice (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_addto_choice, g-> ob_generic, args);
}

static object *
replace_choice (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INiINstr (fl_replace_choice, g-> ob_generic, args);
}

static object *
delete_choice (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_delete_choice, g-> ob_generic, args);
}

static object *
get_choice_text (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rstr (fl_get_choice_text, g-> ob_generic, args);
}

static object *
set_choice_fontsize (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_choice_fontsize, g-> ob_generic, args);
}

static object *
set_choice_fontstyle (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_choice_fontstyle, g-> ob_generic, args);
}

static struct methodlist choice_methods[] = {
	{"set_choice",		set_choice},
	{"get_choice",		get_choice},
	{"clear_choice",	clear_choice},
	{"addto_choice",	addto_choice},
	{"replace_choice",	replace_choice},
	{"delete_choice",	delete_choice},
	{"get_choice_text",	get_choice_text},
	{"set_choice_fontsize", set_choice_fontsize},
	{"set_choice_fontstyle",set_choice_fontstyle},
	{NULL,			NULL}		/* sentinel */
};

/* Class : Clock */

static object *
get_clock(g, args)
	genericobject *g;
	object *args;
{
	int i0, i1, i2;

	if (!getnoarg(args))
		return NULL;

	fl_get_clock (g->ob_generic, &i0, &i1, &i2);

	return mkvalue("(iii)", i0, i1, i2);
}

static struct methodlist clock_methods[] = {
	{"get_clock",		get_clock},
	{NULL,			NULL}		/* sentinel */
};

/* CLass : Counters */

static object *
get_counter_value(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rf (fl_get_counter_value, g-> ob_generic, args);
}

static object *
set_counter_value (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_counter_value, g-> ob_generic, args);
}

static object *
set_counter_precision (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_counter_precision, g-> ob_generic, args);
}

static object *
set_counter_bounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INfINf (fl_set_counter_bounds, g-> ob_generic, args);
}

static object *
set_counter_step (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INfINf (fl_set_counter_step, g-> ob_generic, args);
}

static object *
set_counter_return (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_counter_return, g-> ob_generic, args);
}

static struct methodlist counter_methods[] = {
	{"set_counter_value",		set_counter_value},
	{"get_counter_value",		get_counter_value},
	{"set_counter_bounds",		set_counter_bounds},
	{"set_counter_step",		set_counter_step},
	{"set_counter_precision",	set_counter_precision},
	{"set_counter_return",		set_counter_return},
	{NULL,				NULL}		/* sentinel */
};


/* Class: Dials */

static object *
get_dial_value(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rf (fl_get_dial_value, g-> ob_generic, args);
}

static object *
set_dial_value (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_dial_value, g-> ob_generic, args);
}

static object *
set_dial_bounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INfINf (fl_set_dial_bounds, g-> ob_generic, args);
}

static object *
get_dial_bounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_OUTfOUTf (fl_get_dial_bounds, g-> ob_generic, args);
}

static object *
set_dial_step (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_dial_step, g-> ob_generic, args);
}

static struct methodlist dial_methods[] = {
	{"set_dial_value",	set_dial_value},
	{"get_dial_value",	get_dial_value},
	{"set_dial_bounds",	set_dial_bounds},
	{"get_dial_bounds",	get_dial_bounds},
	{"set_dial_step",	set_dial_step},
	{NULL,			NULL}		/* sentinel */
};

/* Class : Input */

static object *
set_input (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_set_input, g-> ob_generic, args);
}

static object *
get_input (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rstr (fl_get_input, g-> ob_generic, args);
}

static object *
set_input_color (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INfINf (fl_set_input_color, g-> ob_generic, args);
}

static object *
set_input_return (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_input_return, g-> ob_generic, args);
}

static struct methodlist input_methods[] = {
	{"set_input",		set_input},
	{"get_input",		get_input},
	{"set_input_color",	set_input_color},
	{"set_input_return",	set_input_return},
	{NULL,			NULL}		/* sentinel */
};


/* Class : Menu */

static object *
set_menu (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_set_menu, g-> ob_generic, args);
}

static object *
get_menu (g, args)
	genericobject *g;
	object *args;
{
	/* XXX strictly speaking this is wrong since fl_get_menu
	   XXX returns long, not int */
	return call_forms_Ri (fl_get_menu, g-> ob_generic, args);
}

static object *
get_menu_text (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rstr (fl_get_menu_text, g-> ob_generic, args);
}

static object *
addto_menu (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_addto_menu, g-> ob_generic, args);
}

static struct methodlist menu_methods[] = {
	{"set_menu",		set_menu},
	{"get_menu",		get_menu},
	{"get_menu_text",	get_menu_text},
	{"addto_menu",		addto_menu},
	{NULL,			NULL}		/* sentinel */
};


/* Class: Sliders */

static object *
get_slider_value(g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rf (fl_get_slider_value, g-> ob_generic, args);
}

static object *
set_slider_value (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_slider_value, g-> ob_generic, args);
}

static object *
set_slider_bounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INfINf (fl_set_slider_bounds, g-> ob_generic, args);
}

static object *
get_slider_bounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_OUTfOUTf(fl_get_slider_bounds, g-> ob_generic, args);
}

static object *
set_slider_return (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_slider_return, g-> ob_generic, args);
}

static object *
set_slider_size (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_slider_size, g-> ob_generic, args);
}

static object *
set_slider_precision (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INi (fl_set_slider_precision, g-> ob_generic, args);
}

static object *
set_slider_step (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_slider_step, g-> ob_generic, args);
}


static struct methodlist slider_methods[] = {
	{"set_slider_value",	set_slider_value},
	{"get_slider_value",	get_slider_value},
	{"set_slider_bounds",	set_slider_bounds},
	{"get_slider_bounds",	get_slider_bounds},
	{"set_slider_return",	set_slider_return},
	{"set_slider_size",	set_slider_size},
	{"set_slider_precision",set_slider_precision},
	{"set_slider_step",	set_slider_step},
	{NULL,			NULL}		/* sentinel */
};

static object *
set_positioner_xvalue (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_positioner_xvalue, g-> ob_generic, args);
}

static object *
set_positioner_xbounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INfINf (fl_set_positioner_xbounds,
				  g-> ob_generic, args);
}

static object *
set_positioner_yvalue (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_positioner_yvalue, g-> ob_generic, args);
}

static object *
set_positioner_ybounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INfINf (fl_set_positioner_ybounds,
				  g-> ob_generic, args);
}

static object *
get_positioner_xvalue (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rf (fl_get_positioner_xvalue, g-> ob_generic, args);
}

static object *
get_positioner_xbounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_OUTfOUTf (fl_get_positioner_xbounds,
				    g-> ob_generic, args);
}

static object *
get_positioner_yvalue (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rf (fl_get_positioner_yvalue, g-> ob_generic, args);
}

static object *
get_positioner_ybounds (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_OUTfOUTf (fl_get_positioner_ybounds,
				    g-> ob_generic, args);
}

static struct methodlist positioner_methods[] = {
	{"set_positioner_xvalue",		set_positioner_xvalue},
	{"set_positioner_yvalue",		set_positioner_yvalue},
	{"set_positioner_xbounds",		set_positioner_xbounds},
	{"set_positioner_ybounds",		set_positioner_ybounds},
	{"get_positioner_xvalue",		get_positioner_xvalue},
	{"get_positioner_yvalue",		get_positioner_yvalue},
	{"get_positioner_xbounds",		get_positioner_xbounds},
	{"get_positioner_ybounds",		get_positioner_ybounds},
	{NULL,			NULL}		/* sentinel */
};

/* Class timer */

static object *
set_timer (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INf (fl_set_timer, g-> ob_generic, args);
}

static object *
get_timer (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_Rf (fl_get_timer, g-> ob_generic, args);
}

static struct methodlist timer_methods[] = {
	{"set_timer",		set_timer},
	{"get_timer",		get_timer},
	{NULL,			NULL}		/* sentinel */
};

/* Form objects */

typedef struct {
	OB_HEAD
	FL_FORM *ob_form;
} formobject;

extern typeobject Formtype; /* Forward */

#define is_formobject(v) ((v)->ob_type == &Formtype)

static object *
form_show_form(f, args)
	formobject *f;
	object *args;
{
	int place, border;
	char *name;
	if (!getargs(args, "(iis)", &place, &border, &name))
		return NULL;
	fl_show_form(f->ob_form, place, border, name);
	INCREF(None);
	return None;
}

static object *
form_call(func, f, args)
	FL_FORM *f;
	object *args;
	void (*func)(FL_FORM *);
{
	if (!getnoarg(args)) return NULL;

	(*func)(f);

	INCREF(None);
	return None;
}

static object *
form_call_INiINi(func, f, args)
	FL_FORM *f;
	object *args;
	void (*func)(FL_FORM *, int, int);
{
	int a, b;

	if (!getargs(args, "(ii)", &a, &b)) return NULL;

	(*func)(f, a, b);

	INCREF(None);
	return None;
}

static object *
form_call_INfINf(func, f, args)
	FL_FORM *f;
	object *args;
	void (*func)(FL_FORM *, float, float);
{
	float a, b;

	if (!getargs(args, "(ff)", &a, &b)) return NULL;

	(*func)(f, a, b);

	INCREF(None);
	return None;
}

static object *
form_hide_form(f, args)
	formobject *f;
	object *args;
{
	return form_call(fl_hide_form, f-> ob_form, args);
}

static object *
form_redraw_form(f, args)
	formobject *f;
	object *args;
{
	return form_call(fl_redraw_form, f-> ob_form, args);
}

static object *
form_add_object(f, args)
	formobject *f;
	object *args;
{
	genericobject *g;
	if (args == NULL || !is_genericobject(args)) {
		err_badarg();
		return NULL;
	}
	g = (genericobject *)args;
	if (findgeneric(g->ob_generic) != NULL) {
		err_setstr(RuntimeError,
			   "add_object of object already in a form");
		return NULL;
	}
	fl_add_object(f->ob_form, g->ob_generic);
	knowgeneric(g);

	INCREF(None);
	return None;
}

static object *
form_set_form_position(f, args)
	formobject *f;
	object *args;
{
	return form_call_INiINi(fl_set_form_position, f-> ob_form, args);
}

static object *
form_set_form_size(f, args)
	formobject *f;
	object *args;
{
	return form_call_INiINi(fl_set_form_size, f-> ob_form, args);
}

static object *
form_scale_form(f, args)
	formobject *f;
	object *args;
{
	return form_call_INfINf(fl_scale_form, f-> ob_form, args);
}

static object *
generic_add_object(f, args, func, internal_methods)
	formobject *f;
	object *args;
	FL_OBJECT *(*func)(int, float, float, float, float, char*);
	struct methodlist *internal_methods;
{
	int type;
	float x, y, w, h;
	char *name;
	FL_OBJECT *obj;

	if (!getargs(args,"(iffffs)", &type,&x,&y,&w,&h,&name))
		return NULL;

	fl_addto_form (f-> ob_form);

	obj = (*func) (type, x, y, w, h, name);

	fl_end_form();

	if (obj == NULL) {
		err_nomem();
		return NULL;
	}

	return newgenericobject (obj, internal_methods);
}

static object *
form_add_button(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_button, button_methods);
}

static object *
form_add_lightbutton(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_lightbutton, button_methods);
}

static object *
form_add_roundbutton(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_roundbutton, button_methods);
}

static object *
form_add_menu (f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_menu, menu_methods);
}

static object *
form_add_slider(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_slider, slider_methods);
}

static object *
form_add_valslider(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_valslider, slider_methods);
}

static object *
form_add_dial(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_dial, dial_methods);
}

static object *
form_add_counter(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_counter, counter_methods);
}

static object *
form_add_clock(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_clock, clock_methods);
}

static object *
form_add_box(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_box,
				  (struct methodlist *)NULL);
}

static object *
form_add_choice(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_choice, choice_methods);
}

static object *
form_add_browser(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_browser, browser_methods);
}

static object *
form_add_positioner(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_positioner, positioner_methods);
}

static object *
form_add_input(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_input, input_methods);
}

static object *
form_add_text(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_text,
				  (struct methodlist *)NULL);
}

static object *
form_add_timer(f, args)
	formobject *f;
	object *args;
{
	return generic_add_object(f, args, fl_add_timer, timer_methods);
}

static object *
form_freeze_form(f, args)
	formobject *f;
	object *args;
{
	return form_call(fl_freeze_form, f-> ob_form, args);
}

static object *
form_unfreeze_form(f, args)
	formobject *f;
	object *args;
{
	return form_call(fl_unfreeze_form, f-> ob_form, args);
}

static object *
form_activate_form(f, args)
	formobject *f;
	object *args;
{
	return form_call(fl_activate_form, f-> ob_form, args);
}

static object *
form_deactivate_form(f, args)
	formobject *f;
	object *args;
{
	return form_call(fl_deactivate_form, f-> ob_form, args);
}

static object *
form_bgn_group(f, args)
	formobject *f;
	object *args;
{
	FL_OBJECT *obj;

	fl_addto_form(f-> ob_form);
	obj = fl_bgn_group();
	fl_end_form();

	if (obj == NULL) {
		err_nomem();
		return NULL;
	}

	return newgenericobject (obj, (struct methodlist *) NULL);
}

static object *
form_end_group(f, args)
	formobject *f;
	object *args;
{
	fl_addto_form(f-> ob_form);
	fl_end_group();
	fl_end_form();
	INCREF(None);
	return None;
}

static object *
forms_find_first_or_last(func, f, args)
	FL_OBJECT *(*func)(FL_FORM *, int, float, float);
	formobject *f;
	object *args;
{
	int type;
	float mx, my;
	FL_OBJECT *generic;
	genericobject *g;
	
	if (!getargs(args, "(iff)", &type, &mx, &my)) return NULL;

	generic = (*func) (f-> ob_form, type, mx, my);

	if (generic == NULL)
	{
		INCREF(None);
		return None;
	}

	g = findgeneric(generic);
	if (g == NULL) {
		err_setstr(RuntimeError,
			   "forms_find_{first|last} returns unknown object");
		return NULL;
	}
	INCREF(g);
	return (object *) g;
}

static object *
form_find_first(f, args)
	formobject *f;
	object *args;
{
	return forms_find_first_or_last(fl_find_first, f, args);
}

static object *
form_find_last(f, args)
	formobject *f;
	object *args;
{
	return forms_find_first_or_last(fl_find_last, f, args);
}

static object *
form_set_object_focus(f, args)
	formobject *f;
	object *args;
{
	genericobject *g;
	if (args == NULL || !is_genericobject(args)) {
		err_badarg();
		return NULL;
	}
	g = (genericobject *)args;
	fl_set_object_focus(f->ob_form, g->ob_generic);
	INCREF(None);
	return None;
}

static struct methodlist form_methods[] = {
/* adm */
	{"show_form",		form_show_form},
	{"hide_form",		form_hide_form},
	{"redraw_form",		form_redraw_form},
	{"set_form_position",	form_set_form_position},
	{"set_form_size",	form_set_form_size},
	{"scale_form",		form_scale_form},
	{"freeze_form",		form_freeze_form},
	{"unfreeze_form",	form_unfreeze_form},
	{"activate_form",	form_activate_form},
	{"deactivate_form",	form_deactivate_form},
	{"bgn_group",		form_bgn_group},
	{"end_group",		form_end_group},
	{"find_first",		form_find_first},
	{"find_last",		form_find_last},
	{"set_object_focus",	form_set_object_focus},

/* basic objects */
	{"add_button",		form_add_button},
/*	{"add_bitmap",		form_add_bitmap}, */
	{"add_lightbutton",	form_add_lightbutton},
	{"add_roundbutton",	form_add_roundbutton},
	{"add_menu",		form_add_menu},
	{"add_slider",		form_add_slider},
	{"add_positioner",	form_add_positioner},
	{"add_valslider",	form_add_valslider},
	{"add_dial",		form_add_dial},
	{"add_counter",		form_add_counter},
	{"add_box",		form_add_box},
	{"add_clock",		form_add_clock},
	{"add_choice",		form_add_choice},
	{"add_browser",		form_add_browser},
	{"add_input",		form_add_input},
	{"add_timer",		form_add_timer},
	{"add_text",		form_add_text},
	{NULL,			NULL}		/* sentinel */
};

static void
form_dealloc(f)
	formobject *f;
{
	releaseobjects(f->ob_form);
	if (f->ob_form->visible)
		fl_hide_form(f->ob_form);
	fl_free_form(f->ob_form);
	DEL(f);
}

#define OFF(x) offsetof(FL_FORM, x)

static struct memberlist form_memberlist[] = {
	{"window",	T_LONG,		OFF(window),	RO},
	{"w",		T_FLOAT,	OFF(w)},
	{"h",		T_FLOAT,	OFF(h)},
	{"x",		T_FLOAT,	OFF(x),		RO},
	{"y",		T_FLOAT,	OFF(y),		RO},
	{"deactivated",	T_INT,		OFF(deactivated)},
	{"visible",	T_INT,		OFF(visible),	RO},
	{"frozen",	T_INT,		OFF(frozen),	RO},
	{"doublebuf",	T_INT,		OFF(doublebuf)},
	{NULL}	/* Sentinel */
};

#undef OFF

static object *
form_getattr(f, name)
	formobject *f;
	char *name;
{
	object *meth;

	meth = findmethod(form_methods, (object *)f, name);
	if (meth != NULL)
		return meth;
	err_clear();
	return getmember((char *)f->ob_form, form_memberlist, name);
}

static int
form_setattr(f, name, v)
	formobject *f;
	char *name;
	object *v;
{
	int ret;

	if (v == NULL) {
		err_setstr(TypeError, "can't delete form attributes");
		return 0;
	}

	return setmember((char *)f->ob_form, form_memberlist, name, v);
}

static object *
form_repr(f)
	formobject *f;
{
	char buf[100];
	sprintf(buf, "<FORMS_form at %lx, window=%ld>",
		(long)f, f->ob_form->window);
	return newstringobject(buf);
}

typeobject Formtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"FORMS_form",		/*tp_name*/
	sizeof(formobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	form_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	form_getattr,		/*tp_getattr*/
	form_setattr,		/*tp_setattr*/
	0,			/*tp_compare*/
	form_repr,		/*tp_repr*/
};

static object *
newformobject(form)
	FL_FORM *form;
{
	formobject *f;
	f = NEWOBJ(formobject, &Formtype);
	if (f == NULL)
		return NULL;
	f->ob_form = form;
	return (object *)f;
}


/* The "fl" module */

static object *
forms_make_form(dummy, args)
	object *dummy;
	object *args;
{
	int type;
	float w, h;
	FL_FORM *form;
	if (!getargs(args, "(iff)", &type, &w, &h))
		return NULL;
	form = fl_bgn_form(type, w, h);
	if (form == NULL) {
		/* XXX Actually, cannot happen! */
		err_nomem();
		return NULL;
	}
	fl_end_form();
	return newformobject(form);
}

static object *
forms_activate_all_forms(f, args)
	object *f;
	object *args;
{
	fl_activate_all_forms();
	INCREF(None);
	return None;
}

static object *
forms_deactivate_all_forms(f, args)
	object *f;
	object *args;
{
	fl_deactivate_all_forms();
	INCREF(None);
	return None;
}

static object *my_event_callback = NULL;

static object *
forms_set_event_call_back(dummy, args)
	object *dummy;
	object *args;
{
	if (args == None)
		args = NULL;
	my_event_callback = args;
	XINCREF(args);
	INCREF(None);
	return None;
}

static object *
forms_do_or_check_forms(dummy, args, func)
	object *dummy;
	object *args;
	FL_OBJECT *(*func)();
{
	FL_OBJECT *generic;
	genericobject *g;
	object *arg, *res;
	
	if (!getnoarg(args))
		return NULL;

	for (;;) {
		BGN_SAVE
		generic = (*func)();
		END_SAVE
		if (generic == NULL) {
			INCREF(None);
			return None;
		}
		if (generic == FL_EVENT) {
			int dev;
			short val;
			if (my_event_callback == NULL)
				return newintobject(-1L);
			dev = fl_qread(&val);
			arg = mkvalue("(ih)", dev, val);
			if (arg == NULL)
				return NULL;
			res = call_object(my_event_callback, arg);
			XDECREF(res);
			DECREF(arg);
			if (res == NULL)
				return NULL; /* Callback raised exception */
			continue;
		}
		g = findgeneric(generic);
		if (g == NULL) {
			/* Object not known to us (some dialogs cause this) */
			continue; /* Ignore it */
		}
		if (g->ob_callback == NULL) {
			INCREF(g);
			return ((object *) g);
		}
		arg = mkvalue("(OO)", (object *)g, g->ob_callback_arg);
		if (arg == NULL)
			return NULL;
		res = call_object(g->ob_callback, arg);
		XDECREF(res);
		DECREF(arg);
		if (res == NULL)
			return NULL; /* Callback raised exception */
	}
}

static object *
forms_do_forms(dummy, args)
	object *dummy;
	object *args;
{
	return forms_do_or_check_forms(dummy, args, fl_do_forms);
}

static object *
forms_check_forms(dummy, args)
	object *dummy;
	object *args;
{
	return forms_do_or_check_forms(dummy, args, fl_check_forms);
}

static object *
forms_do_only_forms(dummy, args)
	object *dummy;
	object *args;
{
	return forms_do_or_check_forms(dummy, args, fl_do_only_forms);
}

static object *
forms_check_only_forms(dummy, args)
	object *dummy;
	object *args;
{
	return forms_do_or_check_forms(dummy, args, fl_check_only_forms);
}

#ifdef UNUSED
static object *
fl_call(func, args)
	object *args;
	void (*func)();
{
	if (!getnoarg(args))
		return NULL;
	(*func)();
	INCREF(None);
	return None;
}
#endif

static object *
forms_set_graphics_mode(dummy, args)
	object *dummy;
	object *args;
{
	int rgbmode, doublebuf;

	if (!getargs(args, "(ii)", &rgbmode, &doublebuf))
		return NULL;
	fl_set_graphics_mode(rgbmode,doublebuf);
	INCREF(None);
	return None;
}

static object *
forms_get_rgbmode(dummy, args)
	object *dummy;
	object *args;
{
	extern int fl_rgbmode;

	if (args != NULL) {
		err_badarg();
		return NULL;
	}
	return newintobject((long)fl_rgbmode);
}

static object *
forms_show_errors(dummy, args)
	object *dummy;
	object *args;
{
	int show;
	if (!getargs(args, "i", &show))
		return NULL;
	fl_show_errors(show);
	INCREF(None);
	return None;
}

static object *
forms_set_font_name(dummy, args)
	object *dummy;
	object *args;
{
	int numb;
	char *name;
	if (!getargs(args, "(is)", &numb, &name))
		return NULL;
	fl_set_font_name(numb, name);
	INCREF(None);
	return None;
}


static object *
forms_qdevice(self, args)
	object *self;
	object *args;
{
	short arg1;
	if (!getargs(args, "h", &arg1))
		return NULL;
	fl_qdevice(arg1);
	INCREF(None);
	return None;
}

static object *
forms_unqdevice(self, args)
	object *self;
	object *args;
{
	short arg1;
	if (!getargs(args, "h", &arg1))
		return NULL;
	fl_unqdevice(arg1);
	INCREF(None);
	return None;
}

static object *
forms_isqueued(self, args)
	object *self;
	object *args;
{
	long retval;
	short arg1;
	if (!getargs(args, "h", &arg1))
		return NULL;
	retval = fl_isqueued(arg1);

	return newintobject(retval);
}

static object *
forms_qtest(self, args)
	object *self;
	object *args;
{
	long retval;
	retval = fl_qtest();
	return newintobject(retval);
}


static object *
forms_qread(self, args)
	object *self;
	object *args;
{
	int dev;
	short val;
	BGN_SAVE
	dev = fl_qread(&val);
	END_SAVE
	return mkvalue("(ih)", dev, val);
}

static object *
forms_qreset(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args)) return NULL;

	fl_qreset();
	INCREF(None);
	return None;
}

static object *
forms_qenter(self, args)
	object *self;
	object *args;
{
	short arg1, arg2;
	if (!getargs(args, "(hh)", &arg1, &arg2))
		return NULL;
	fl_qenter(arg1, arg2);
	INCREF(None);
	return None;
}

static object *
forms_color(self, args)
	object *self;
	object *args;
{
	int arg;

	if (!getintarg(args, &arg)) return NULL;

	fl_color((short) arg);

	INCREF(None);
	return None;
}

static object *
forms_mapcolor(self, args)
	object *self;
	object *args;
{
	int arg0, arg1, arg2, arg3;

	if (!getargs(args, "(iiii)", &arg0, &arg1, &arg2, &arg3))
		return NULL;

	fl_mapcolor(arg0, (short) arg1, (short) arg2, (short) arg3);

	INCREF(None);
	return None;
}

static object *
forms_getmcolor(self, args)
	object *self;
	object *args;
{
	int arg;
	short r, g, b;

	if (!getintarg(args, &arg)) return NULL;

	fl_getmcolor(arg, &r, &g, &b);

	return mkvalue("(hhh)", r, g, b);
}

static object *
forms_get_mouse(self, args)
	object *self;
	object *args;
{
	float x, y;

	if (!getnoarg(args)) return NULL;
	
	fl_get_mouse(&x, &y);

	return mkvalue("(ff)", x, y);
}

static object *
forms_tie(self, args)
	object *self;
	object *args;
{
	short arg1, arg2, arg3;
	if (!getargs(args, "(hhh)", &arg1, &arg2, &arg3))
		return NULL;
	fl_tie(arg1, arg2, arg3);
	INCREF(None);
	return None;
}

static object *
forms_show_message(f, args)
	object *f;
	object *args;
{
	char *a, *b, *c;

	if (!getargs(args, "(sss)", &a, &b, &c)) return NULL;

	BGN_SAVE
	fl_show_message(a, b, c);
	END_SAVE

	INCREF(None);
	return None;
}

static object *
forms_show_choice(f, args)
	object *f;
	object *args;
{
	char *m1, *m2, *m3, *b1, *b2, *b3;
	int nb;
	char *format;
	long rv;

	if (args == NULL || !is_tupleobject(args)) {
		err_badarg();
		return NULL;
	}
	nb = gettuplesize(args) - 3;
	if (nb <= 0) {
		err_setstr(TypeError, "need at least one button label");
		return NULL;
	}
	if (is_intobject(gettupleitem(args, 3))) {
		err_setstr(TypeError,
			   "'number-of-buttons' argument not needed");
		return NULL;
	}
	switch (nb) {
	case 1: format = "(ssss)"; break;
	case 2: format = "(sssss)"; break;
	case 3: format = "(ssssss)"; break;
	default:
		err_setstr(TypeError, "too many button labels");
		return NULL;
	}

	if (!getargs(args, format, &m1, &m2, &m3, &b1, &b2, &b3))
		return NULL;

	BGN_SAVE
	rv = fl_show_choice(m1, m2, m3, nb, b1, b2, b3);
	END_SAVE
	return newintobject(rv);
}

static object *
forms_show_question(f, args)
	object *f;
	object *args;
{
	int ret;
	char *a, *b, *c;

	if (!getargs(args, "(sss)", &a, &b, &c)) return NULL;

	BGN_SAVE
	ret = fl_show_question(a, b, c);
	END_SAVE

	return newintobject((long) ret);
}

static object *
forms_show_input(f, args)
	object *f;
	object *args;
{
	char *str;
	char *a, *b;

	if (!getargs(args, "(ss)", &a, &b)) return NULL;

	BGN_SAVE
	str = fl_show_input(a, b);
	END_SAVE

	if (str == NULL) {
		INCREF(None);
		return None;
	}
	return newstringobject(str);
}

static object *
forms_file_selector(f, args)
	object *f;
	object *args;
{
	char *str;
	char *a, *b, *c, *d;

	if (!getargs(args, "(ssss)", &a, &b, &c, &d)) return NULL;

	BGN_SAVE
	str = fl_show_file_selector(a, b, c, d);
	END_SAVE

	if (str == NULL) {
		INCREF(None);
		return None;
	}
	return newstringobject(str);
}


static object *
forms_file_selector_func(args, func)
	object *args;
	char *(*func)();
{
	char *str;

	str = (*func) ();

	if (str == NULL) {
		INCREF(None);
		return None;
	}
	return newstringobject(str);
}

static object *
forms_get_directory(f, args)
	object *f;
	object *args;
{
	return forms_file_selector_func(args, fl_get_directory);
}

static object *
forms_get_pattern(f, args)
	object *f;
	object *args;
{
	return forms_file_selector_func(args, fl_get_pattern);
}

static object *
forms_get_filename(f, args)
	object *f;
	object *args;
{
	return forms_file_selector_func(args, fl_get_filename);
}

static struct methodlist forms_methods[] = {
/* adm */
	{"make_form",		forms_make_form},
	{"activate_all_forms",	forms_activate_all_forms},
	{"deactivate_all_forms",forms_deactivate_all_forms},
/* gl support wrappers */
	{"qdevice",		forms_qdevice},
	{"unqdevice",		forms_unqdevice},
	{"isqueued",		forms_isqueued},
	{"qtest",		forms_qtest},
	{"qread",		forms_qread},
/*	{"blkqread",		forms_blkqread}, */
	{"qreset",		forms_qreset},
	{"qenter",		forms_qenter},
	{"get_mouse",		forms_get_mouse},
	{"tie",			forms_tie},
/*	{"new_events",		forms_new_events}, */
	{"color",		forms_color},
	{"mapcolor",		forms_mapcolor},
	{"getmcolor",		forms_getmcolor},
/* interaction */
	{"do_forms",		forms_do_forms},
	{"do_only_forms",	forms_do_only_forms},
	{"check_forms",		forms_check_forms},
	{"check_only_forms",	forms_check_only_forms},
	{"set_event_call_back",	forms_set_event_call_back},
/* goodies */
	{"show_message",	forms_show_message},
	{"show_question",	forms_show_question},
	{"show_choice",		forms_show_choice},
	{"show_input",		forms_show_input},
	{"show_file_selector",	forms_file_selector},
	{"file_selector",	forms_file_selector}, /* BW compat */
	{"get_directory",	forms_get_directory},
	{"get_pattern",		forms_get_pattern},
	{"get_filename",	forms_get_filename},
	{"set_graphics_mode",	forms_set_graphics_mode},
	{"get_rgbmode",		forms_get_rgbmode},
	{"show_errors",		forms_show_errors},
	{"set_font_name",	forms_set_font_name},
	{NULL,			NULL}		/* sentinel */
};

void
initfl()
{
	initmodule("fl", forms_methods);
	foreground();
	fl_init();
}
