/**********************************************************
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

/* FL module -- interface to Mark Overmars' FORMS Library. */

#include "forms.h"

#include "allobjects.h"
#include "import.h"
#include "modsupport.h"
#include "structmember.h"

/* #include "ceval.h" */
extern object *call_object(object *, object *);

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
			if (g->ob_refcnt == 1) {
				/* The object is now unreachable:
				   delete it from the list of known objects */
				setlistitem(allgenerics, i, (object *)NULL);
				nfreeslots++;
			}
		}
	}
	/* XXX Should also get rid of objects with refcnt==1 */
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
	/* XXX Should remove it from the list of known objects */
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

static struct methodlist generic_methods[] = {
	{"set_call_back",	generic_set_call_back},
	{"delete_object",	generic_delete_object},
	{"show_object",		generic_show_object},
	{"hide_object",		generic_hide_object},
	{"redraw_object",	generic_redraw_object},
	{"freeze_object",	generic_freeze_object},
	{"unfreeze_object",	generic_unfreeze_object},
#if 0
	{"handle_object",	generic_handle_object},
	{"handle_object_direct",generic_handle_object_direct},
#endif
	{NULL,			NULL}		/* sentinel */
};

static void
generic_dealloc(g)
	genericobject *g;
{
	fl_free_object(g->ob_generic);
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
	{"frozen",	T_INT,		OFF(frozen),	RO},
	{"active",	T_INT,		OFF(active),	RO},
	{"input",	T_INT,		OFF(input),	RO},
	{"visible",	T_INT,		OFF(visible),	RO},
	{"radio",	T_INT,		OFF(radio),	RO},
	{"automatic",	T_INT,		OFF(automatic),	RO},
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
		return NULL;
	}

	/* "label" is an exception: setmember doesn't set strings;
	   and FORMS wants you to call a function to set the label */
	if (strcmp(name, "label") == 0) {
		if (!is_stringobject(v)) {
			err_setstr(TypeError, "label attr must be string");
			return NULL;
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

typeobject GenericObjecttype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"generic FORMS object",	/*tp_name*/
	sizeof(genericobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	generic_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	generic_getattr,	/*tp_getattr*/
	generic_setattr,	/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
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

	if (!getfloatarg (args, &parameter)) return NULL;

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

	if (!getfloatfloatarg (args, &par1, &par2)) return NULL;

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

	if (!getintarg (args, &parameter)) return NULL;

	(*func) (obj, parameter);

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
	object *a;
     
	if (!getstrarg (args, &a)) return NULL;

	(*func) (obj, getstringvalue (a));

	INCREF(None);
	return None;
}


/* voide func (object, int, string) */
static object *
call_forms_INiINstr (func, obj, args)
	void (*func)(FL_OBJECT *, int, char *);
	FL_OBJECT *obj;
	object *args;
{
	object *a;
	int b;
	
	if (!getintstrarg (args, &b, &a)) return NULL;
	
	(*func) (obj, b, getstringvalue (a));
	
	INCREF(None);
	return None;
}

#ifdef UNUSED
/* void func (object, float) */
static object *
call_forms_INiINi (func, obj, args)
	void (*func)(FL_OBJECT *, float, float);
	FL_OBJECT *obj;
	object *args;
{
	int par1, par2;
	
	if (!getintintarg (args, &par1, &par2)) return NULL;
	
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
	
	if (!getnoarg (args)) return NULL;
	
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
	object *arg;
	
        if (!getnoarg(args)) return NULL;
	
	(*func) (obj, &f1, &f2);
	
	arg = newtupleobject (2);
	if (arg == NULL) return NULL;

	settupleitem (arg, 0, newfloatobject (f1));
	settupleitem (arg, 1, newfloatobject (f2));
	return arg;
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

static struct methodlist browser_methods[] = {
	{"set_browser_topline",	set_browser_topline},
	{"clear_browser",       clear_browser},
	{"add_browser_line",    add_browser_line},
	{"addto_browser",       addto_browser},
	{"insert_browser_line", insert_browser_line},
	{"delete_browser_line", delete_browser_line},
	{"replace_browser_line",replace_browser_line},
	{"get_browser_line",    get_browser_line},
	{"load_browser",        load_browser},
	{"get_browser_maxline", get_browser_maxline},
	{"select_browser_line", select_browser_line},
	{"deselect_browser_line",   deselect_browser_line},
	{"deselect_browser",    deselect_browser},
	{"isselected_browser_line", isselected_browser_line},
	{"get_browser",         get_browser},
	{"set_browser_fontsize",set_browser_fontsize},
	{"set_browser_fontstyle",    set_browser_fontstyle},
	{NULL,			NULL}		/* sentinel */
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

static struct methodlist button_methods[] = {
	{"set_button",		set_button},
	{"get_button",		get_button},
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
	{"addto_choice",        addto_choice},
	{"replace_choice",      replace_choice},
	{"delete_choice",       delete_choice},
	{"get_choice_text",     get_choice_text},
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
	object *arg;

	if (!getnoarg(args))
		return NULL;

	fl_get_clock (g->ob_generic, &i0, &i1, &i2);

	arg = newtupleobject (3);
	if (arg == NULL) return NULL;

	settupleitem (arg, 0, newintobject (i0));
	settupleitem (arg, 1, newintobject (i1));
	settupleitem (arg, 2, newintobject (i2));
	return arg;
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
	{"set_counter_value",          set_counter_value},
	{"get_counter_value",	      get_counter_value},
	{"set_counter_bounds",   set_counter_bounds},
	{"set_counter_step",   set_counter_step},
	{"set_counter_precision",   set_counter_precision},
	{"set_counter_return",   set_counter_return},
	{NULL,			NULL}		/* sentinel */
};

/* Class : Defaults */

static object *
get_default(g, args)
	genericobject *g;
	object *args;
{
	char c;

	if (!getnoarg(args)) return NULL;

	c = fl_get_default (g->ob_generic);

	return ((object *) mknewcharobject (c));     /* in cgensupport.c */
}

static struct methodlist default_methods[] = {
	{"get_default",	      get_default},
	{NULL,			NULL}		/* sentinel */
};


/* Class: Dials */

static object *
set_dial (g, args)
	genericobject *g;
	object *args;
{
	float f1, f2, f3;

	if (!getfloatfloatfloatarg(args, &f1, &f2, &f3))
		return NULL;
	fl_set_dial (g->ob_generic, f1, f2, f3);
	INCREF(None);
	return None;
}

static object *
get_dial(g, args)
	genericobject *g;
	object *args;
{
  return call_forms_Rf (fl_get_dial, g-> ob_generic, args);
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

static struct methodlist dial_methods[] = {
	{"set_dial",          set_dial},
	{"get_dial",	      get_dial},
	{"set_dial_value",    set_dial_value},
	{"get_dial_value",    get_dial},
	{"set_dial_bounds",   set_dial_bounds},
	{"get_dial_bounds",   get_dial_bounds},
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

static struct methodlist input_methods[] = {
	{"set_input",         set_input},
	{"get_input",	      get_input},
	{"set_input_color",   set_input_color},
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
addto_menu (g, args)
	genericobject *g;
	object *args;
{
	return call_forms_INstr (fl_addto_menu, g-> ob_generic, args);
}

static struct methodlist menu_methods[] = {
	{"set_menu",         set_menu},
	{"get_menu",	     get_menu},
	{"addto_menu",       addto_menu},
	{NULL,			NULL}		/* sentinel */
};


/* Class: Sliders */

static object *
set_slider (g, args)
	genericobject *g;
	object *args;
{
	float f1, f2, f3;

	if (!getfloatfloatfloatarg(args, &f1, &f2, &f3))
		return NULL;
	fl_set_slider (g->ob_generic, f1, f2, f3);
	INCREF(None);
	return None;
}

static object *
get_slider(g, args)
	genericobject *g;
	object *args;
{
  return call_forms_Rf (fl_get_slider, g-> ob_generic, args);
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

static struct methodlist slider_methods[] = {
	{"set_slider",		set_slider},
	{"get_slider",		get_slider},
	{"set_slider_value",    set_slider_value},
	{"get_slider_value",    get_slider},
	{"set_slider_bounds",   set_slider_bounds},
	{"get_slider_bounds",   get_slider_bounds},
	{"set_slider_return",   set_slider_return},
	{"set_slider_size",     set_slider_size},
	{"set_slider_precision",set_slider_precision},
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
  return call_forms_INfINf (fl_set_positioner_xbounds, g-> ob_generic, args);
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
  return call_forms_INfINf (fl_set_positioner_ybounds, g-> ob_generic, args);
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
  return call_forms_OUTfOUTf (fl_get_positioner_xbounds, g-> ob_generic, args);
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
  return call_forms_OUTfOUTf (fl_get_positioner_ybounds, g-> ob_generic, args);
}

static struct methodlist positioner_methods[] = {
	{"set_positioner_xvalue",		set_positioner_xvalue},
	{"set_positioner_yvalue",		set_positioner_yvalue},
	{"set_positioner_xbounds",	 	set_positioner_xbounds},
	{"set_positioner_ybounds",	 	set_positioner_ybounds},
	{"get_positioner_xvalue",		get_positioner_xvalue},
	{"get_positioner_yvalue",		get_positioner_yvalue},
	{"get_positioner_xbounds",	 	get_positioner_xbounds},
	{"get_positioner_ybounds",	 	get_positioner_ybounds},
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
	object *name;
	if (!getintintstrarg(args, &place, &border, &name))
		return NULL;
	fl_show_form(f->ob_form, place, border, getstringvalue(name));
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

        if (!getintintarg(args, &a, &b)) return NULL;

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
	if (args == NULL || !is_genericobject(args)) {
		err_badarg();
		return NULL;
	}

	fl_add_object(f->ob_form, ((genericobject *)args) -> ob_generic);

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
generic_add_object(f, args, func, internal_methods)
	formobject *f;
	object *args;
	FL_OBJECT *(*func)(int, float, float, float, float, char*);
        struct methodlist *internal_methods;
{
	int type;
	float x, y, w, h;
	object *name;
	FL_OBJECT *obj;

	if (!getintfloatfloatfloatfloatstrarg(args,&type,&x,&y,&w,&h,&name))
		return NULL;
  
	fl_addto_form (f-> ob_form);
  
	obj = (*func) (type, x, y, w, h, getstringvalue(name));

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
form_add_default(f, args)
     formobject *f;
     object *args;
{
	return generic_add_object(f, args, fl_add_default, default_methods);
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
form_display_form(f, args)
	formobject *f;
	object *args;
{
	int place, border;
	object *name;
	if (!getintintstrarg(args, &place, &border, &name))
		return NULL;
	fl_show_form(f->ob_form, place, border, getstringvalue(name));
	INCREF(None);
	return None;
}

static object *
form_remove_form(f, args)
	formobject *f;
	object *args;
{
	return form_call(fl_remove_form, f-> ob_form, args);
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
	
	if (!getintfloatfloatarg(args, &type, &mx, &my)) return NULL;

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

static struct methodlist form_methods[] = {
/* adm */
	{"show_form",		form_show_form},
	{"hide_form",		form_hide_form},
	{"redraw_form",         form_redraw_form},
	{"set_form_position",   form_set_form_position},
	{"freeze_form",		form_freeze_form},
	{"unfreeze_form",	form_unfreeze_form},
	{"display_form",	form_display_form},
	{"remove_form",		form_remove_form},
	{"activate_form",	form_activate_form},
	{"deactivate_form",	form_deactivate_form},
	{"bgn_group",		form_bgn_group},
	{"end_group",		form_end_group},
	{"find_first",		form_find_first},
	{"find_last",		form_find_last},

/* basic objects */
	{"add_button",          form_add_button},
/*	{"add_bitmap",          form_add_bitmap}, */
	{"add_lightbutton",	form_add_lightbutton},
	{"add_roundbutton",     form_add_roundbutton},
	{"add_menu",            form_add_menu},
	{"add_slider",          form_add_slider},
	{"add_positioner",      form_add_positioner},
	{"add_valslider",       form_add_valslider},
	{"add_dial",            form_add_dial},
	{"add_counter",         form_add_counter},
	{"add_default",         form_add_default},
	{"add_box",             form_add_box},
	{"add_clock",           form_add_clock},
	{"add_choice",          form_add_choice},
	{"add_browser",         form_add_browser},
	{"add_input",           form_add_input},
	{"add_timer",           form_add_timer},
	{"add_text",            form_add_text},
	{NULL,			NULL}		/* sentinel */
};

static void
form_dealloc(f)
	formobject *f;
{
	releaseobjects(f->ob_form);
	fl_free_form(f->ob_form);
	DEL(f);
}

#define OFF(x) offsetof(FL_FORM, x)

static struct memberlist form_memberlist[] = {
	{"window",	T_LONG,		OFF(window),	RO},
	{"w",		T_FLOAT,	OFF(w)},
	{"h",		T_FLOAT,	OFF(h)},
	{"x",		T_FLOAT,	OFF(x)},
	{"y",		T_FLOAT,	OFF(y)},
	{"deactivated",	T_INT,		OFF(deactivated)},
	{"visible",	T_INT,		OFF(visible)},
	{"frozen",	T_INT,		OFF(frozen)},
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
		return NULL;
	}

	return setmember((char *)f->ob_form, form_memberlist, name, v);
}

typeobject Formtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"form",			/*tp_name*/
	sizeof(formobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	form_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	form_getattr,		/*tp_getattr*/
	form_setattr,		/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
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
	if (!getintfloatfloatarg(args, &type, &w, &h))
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

static object *my_event_callback = NULL;

static object *
forms_set_event_call_back(dummy, args)
	object *dummy;
	object *args;
{
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
		generic = (*func)();
		if (generic == NULL) {
			INCREF(None);
			return None;
		}
		if (generic == FL_EVENT) {
			int dev;
			short val;
			if (my_event_callback == NULL)
				return newintobject(-1);
			dev = fl_qread(&val);
			arg = newtupleobject(2);
			if (arg == NULL)
				return NULL;
			settupleitem(arg, 0, newintobject((long)dev));
			settupleitem(arg, 1, newintobject((long)val));
			res = call_object(my_event_callback, arg);
			XDECREF(res);
			DECREF(arg);
			if (res == NULL)
				return NULL; /* Callback raised exception */
			continue;
		}
		g = findgeneric(generic);
		if (g == NULL) {
			err_setstr(RuntimeError,
				   "{do|check}_forms returns unknown object");
			return NULL;
		}
		if (g->ob_callback == NULL) {
			INCREF(g);
			return ((object *) g);
		}
		arg = newtupleobject(2);
		INCREF(g);
		settupleitem(arg, 0, (object *)g);
		INCREF(g->ob_callback_arg);
		settupleitem(arg, 1, g->ob_callback_arg);
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
forms_qdevice(self, args)
	object *self;
	object *args;
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
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
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
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
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
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
	long retval;
	short arg1 ;
	retval = fl_qread(&arg1);
	{ object *v = newtupleobject(2);
	  if (v == NULL) return NULL;
	  settupleitem(v, 0, newintobject(retval));
	  settupleitem(v, 1, newintobject((long)arg1));
	  return v;
	}
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
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
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

	if (!getintintintintarg(args, &arg0, &arg1, &arg2, &arg3))
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
	object *v;

	if (!getintarg(args, &arg)) return NULL;

	fl_getmcolor(arg, &r, &g, &b);

	v = newtupleobject(3);

	if (v == NULL) return NULL;

	settupleitem(v, 0, newintobject((long)r));
	settupleitem(v, 1, newintobject((long)g));
	settupleitem(v, 2, newintobject((long)b));

	return v;
}

static object *
forms_get_mouse(self, args)
	object *self;
	object *args;
{
	float x, y ;
	object *v;

	if (!getnoarg(args)) return NULL;
	
	fl_get_mouse(&x, &y);

	v = newtupleobject(2);

	if (v == NULL) return NULL;

	settupleitem(v, 0, newfloatobject(x));
	settupleitem(v, 1, newfloatobject(y));

	return v;
}

static object *
forms_tie(self, args)
	object *self;
	object *args;
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
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
	object *a, *b, *c;

        if (!getstrstrstrarg(args, &a, &b, &c)) return NULL;

	fl_show_message(
		   getstringvalue(a), getstringvalue(b), getstringvalue(c));

	INCREF(None);
	return None;
}

static object *
forms_show_question(f, args)
     object *f;
     object *args;
{
        int ret;
	object *a, *b, *c;

        if (!getstrstrstrarg(args, &a, &b, &c)) return NULL;

	ret = fl_show_question(
		   getstringvalue(a), getstringvalue(b), getstringvalue(c));
   
        return newintobject((long) ret);
}

static object *
forms_show_input(f, args)
     object *f;
     object *args;
{
        char *str;
	object *a, *b;

        if (!getstrstrarg(args, &a, &b)) return NULL;

	str = fl_show_input(getstringvalue(a), getstringvalue(b));

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
	object *a, *b, *c, *d;

        if (!getstrstrstrstrarg(args, &a, &b, &c, &d)) return NULL;

	str = fl_show_file_selector(getstringvalue(a), getstringvalue(b),
				     getstringvalue(c), getstringvalue(d));
   
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
/* gl support wrappers */
	{"qdevice",		forms_qdevice},
	{"unqdevice",		forms_unqdevice},
	{"isqueued",		forms_isqueued},
	{"qtest",		forms_qtest},
	{"qread",		forms_qread},
/*	{"blkqread",		forms_blkqread},  */
	{"qreset",		forms_qreset},
	{"qenter",		forms_qenter},
	{"get_mouse",		forms_get_mouse},
	{"tie",			forms_tie},
/*	{"new_events",		forms_new_events}, */
	{"color",               forms_color},
	{"mapcolor",		forms_mapcolor},
	{"getmcolor",		forms_getmcolor},
/* interaction */
	{"do_forms",		forms_do_forms},
	{"check_forms",		forms_check_forms},
	{"set_event_call_back",	forms_set_event_call_back},
/* goodies */
	{"show_message",        forms_show_message},
	{"show_question",       forms_show_question},
	{"file_selector",       forms_file_selector},
	{"get_directory",       forms_get_directory},
	{"get_pattern",         forms_get_pattern},
	{"get_filename",        forms_get_filename},
/*
	{"show_choice",         forms_show_choice},
	XXX - draw.c
*/
	{"show_input",          forms_show_input},
	{NULL,			NULL}		/* sentinel */
};

void
initfl()
{
	initmodule("fl", forms_methods);
	foreground();
}


/* Support routines */

int
getintintstrarg(args, a, b, c)
	object *args;
	int *a, *b;
	object **c;
{
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 3) {
		err_badarg();
		return NULL;
	}
	return getintarg(gettupleitem(args, 0), a) &&
		getintarg(gettupleitem(args, 1), b) &&
		getstrarg(gettupleitem(args, 2), c);
}

int
getintfloatfloatarg(args, a, b, c)
	object *args;
	int *a;
	float *b, *c;
{
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 3) {
		err_badarg();
		return NULL;
	}
	return getintarg(gettupleitem(args, 0), a) &&
		getfloatarg(gettupleitem(args, 1), b) &&
		getfloatarg(gettupleitem(args, 2), c);
}

int
getintintintintarg(args, a, b, c, d)
	object *args;
	int *a, *b, *c, *d;
{
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 4) {
		err_badarg();
		return NULL;
	}
	return getintarg(gettupleitem(args, 0), a) &&
		getintarg(gettupleitem(args, 1), b) &&
		getintarg(gettupleitem(args, 2), c) &&
		getintarg(gettupleitem(args, 3), d);
}

int
getfloatarg(args, a)
	object *args;
	float *a;
{
	double x;
	if (!getdoublearg(args, &x))
		return 0;
	*a = x;
	return 1;
}

int
getintfloatfloatfloatfloatstrarg(args, type, x, y, w, h, name)
     object *args;
     int *type;
     float *x, *y, *w, *h;
     object **name;
{
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 6) {
		err_badarg();
		return NULL;
	}
	return  getintarg(gettupleitem(args, 0), type) &&
		getfloatarg(gettupleitem(args, 1), x) &&
		getfloatarg(gettupleitem(args, 2), y) &&
		getfloatarg(gettupleitem(args, 3), w) &&
		getfloatarg(gettupleitem(args, 4), h) &&
		getstrarg(gettupleitem(args, 5), name);
}

int
getfloatfloatfloatarg(args, f1, f2, f3)
     object *args;
     float *f1, *f2, *f3;
{
        if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 3) {
		err_badarg();
		return NULL;
	}
	return  getfloatarg(gettupleitem(args, 0), f1) &&
		getfloatarg(gettupleitem(args, 1), f2) &&
		getfloatarg(gettupleitem(args, 2), f3);
}

int
getfloatfloatarg(args, f1, f2)
     object *args;
     float *f1, *f2;
{
        if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2) {
		err_badarg();
		return NULL;
	}
	return  getfloatarg(gettupleitem(args, 0), f1) &&
		getfloatarg(gettupleitem(args, 1), f2);
}

int
getstrstrstrarg(v, a, b, c)
	object *v;
	object **a;
	object **b;
        object **c;
{
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 3) {
		return err_badarg();
	}
	return getstrarg(gettupleitem(v, 0), a) &&
		getstrarg(gettupleitem(v, 1), b)&&
		getstrarg(gettupleitem(v, 2), c);
}


int
getstrstrstrstrarg(v, a, b, c, d)
	object *v;
	object **a;
	object **b;
        object **c;
        object **d;
{
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 4) {
		return err_badarg();
	}
	return getstrarg(gettupleitem(v, 0), a) &&
		getstrarg(gettupleitem(v, 1), b)&&
		getstrarg(gettupleitem(v, 2), c) &&
		getstrarg(gettupleitem(v, 3),d);
		  
}
