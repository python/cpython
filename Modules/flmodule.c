/* FL module -- interface to Mark Overmars' FORMS Library. */

/* This code works with FORMS version 2.2 (if you defined
   OBSOLETE_FORMS_CALLS), and 2.3.
   FORMS can be ftp'ed from ftp.cs.ruu.nl (131.211.80.17), directory
   /pub/SGI/FORMS. */

/* A half-hearted attempt has been made to allow programs using this
 * module to exploit parallelism (through the threads module). No provisions
 * have been made for multiple threads to use this module at the same time,
 * though. So, a program with a forms thread and a non-forms thread will work
 * fine but a program with two threads using forms will probably crash (unless
 * the program takes precaution to ensure that only one thread can be in
 * this module at any time). This will have to be fixed some time.
 * (A fix will probably also have to synchronize with the gl module).
 */

#include "Python.h"
#include "forms.h"
#include "structmember.h"

/* Generic Forms Objects */

typedef struct {
	PyObject_HEAD
	FL_OBJECT *ob_generic;
	PyMethodDef *ob_methods;
	PyObject *ob_callback;
	PyObject *ob_callback_arg;
} genericobject;

staticforward PyTypeObject GenericObjecttype;

#define is_genericobject(g) ((g)->ob_type == &GenericObjecttype)

/* List of all objects (XXX this should be a hash table on address...) */

static PyObject *allgenerics = NULL;
static int nfreeslots = 0;

/* Add an object to the list of known objects */

static void
knowgeneric(genericobject *g)
{
	int i, n;
	/* Create the list if it doesn't already exist */
	if (allgenerics == NULL) {
		allgenerics = PyList_New(0);
		if (allgenerics == NULL) {
			PyErr_Clear();
			return; /* Too bad, live without allgenerics... */
		}
	}
	if (nfreeslots > 0) {
		/* Search the list for reusable slots (NULL items) */
		/* XXX This can be made faster! */
		n = PyList_Size(allgenerics);
		for (i = 0; i < n; i++) {
			if (PyList_GetItem(allgenerics, i) == NULL) {
				Py_INCREF(g);
				PyList_SetItem(allgenerics, i, (PyObject *)g);
				nfreeslots--;
				return;
			}
		}
		/* Strange... no free slots found... */
		nfreeslots = 0;
	}
	/* No free entries, append new item to the end */
	PyList_Append(allgenerics, (PyObject *)g);
}

/* Find an object in the list of known objects */

static genericobject *
findgeneric(FL_OBJECT *generic)
{
	int i, n;
	genericobject *g;
	
	if (allgenerics == NULL)
		return NULL; /* No objects known yet */
	n = PyList_Size(allgenerics);
	for (i = 0; i < n; i++) {
		g = (genericobject *)PyList_GetItem(allgenerics, i);
		if (g != NULL && g->ob_generic == generic)
			return g;
	}
	return NULL; /* Unknown object */
}

/* Remove an object from the list of known objects */

static void
forgetgeneric(genericobject *g)
{
	int i, n;
	
	Py_XDECREF(g->ob_callback);
	g->ob_callback = NULL;
	Py_XDECREF(g->ob_callback_arg);
	g->ob_callback_arg = NULL;
	if (allgenerics == NULL)
		return; /* No objects known yet */
	n = PyList_Size(allgenerics);
	for (i = 0; i < n; i++) {
		if (g == (genericobject *)PyList_GetItem(allgenerics, i)) {
			PyList_SetItem(allgenerics, i, (PyObject *)NULL);
			nfreeslots++;
			break;
		}
	}
}

/* Called when a form is about to be freed --
   remove all the objects that we know about from it. */

static void
releaseobjects(FL_FORM *form)
{
	int i, n;
	genericobject *g;
	
	if (allgenerics == NULL)
		return; /* No objects known yet */
	n = PyList_Size(allgenerics);
	for (i = 0; i < n; i++) {
		g = (genericobject *)PyList_GetItem(allgenerics, i);
		if (g != NULL && g->ob_generic->form == form) {
			fl_delete_object(g->ob_generic);
			/* The object is now unreachable for
			   do_forms and check_forms, so
			   delete it from the list of known objects */
			Py_XDECREF(g->ob_callback);
			g->ob_callback = NULL;
			Py_XDECREF(g->ob_callback_arg);
			g->ob_callback_arg = NULL;
			PyList_SetItem(allgenerics, i, (PyObject *)NULL);
			nfreeslots++;
		}
	}
}


/* Methods of generic objects */

static PyObject *
generic_set_call_back(genericobject *g, PyObject *args)
{
	if (args == NULL) {
		Py_XDECREF(g->ob_callback);
		Py_XDECREF(g->ob_callback_arg);
		g->ob_callback = NULL;
		g->ob_callback_arg = NULL;
	}
	else {
		if (!PyTuple_Check(args) || PyTuple_Size(args) != 2) {
			PyErr_BadArgument();
			return NULL;
		}
		Py_XDECREF(g->ob_callback);
		Py_XDECREF(g->ob_callback_arg);
		g->ob_callback = PyTuple_GetItem(args, 0);
		Py_INCREF(g->ob_callback);
		g->ob_callback_arg = PyTuple_GetItem(args, 1);
		Py_INCREF(g->ob_callback_arg);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
generic_call(genericobject *g, PyObject *args, void (*func)(FL_OBJECT *))
{
	if (!PyArg_NoArgs(args))
		return NULL;
	(*func)(g->ob_generic);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
generic_delete_object(genericobject *g, PyObject *args)
{
	PyObject *res;
	res = generic_call(g, args, fl_delete_object);
	if (res != NULL)
		forgetgeneric(g);
	return res;
}

static PyObject *
generic_show_object(genericobject *g, PyObject *args)
{
	return generic_call(g, args, fl_show_object);
}

static PyObject *
generic_hide_object(genericobject *g, PyObject *args)
{
	return generic_call(g, args, fl_hide_object);
}

static PyObject *
generic_redraw_object(genericobject *g, PyObject *args)
{
	return generic_call(g, args, fl_redraw_object);
}

#ifdef OBSOLETE_FORMS_CALLS
 
 /* (un)freeze_object() are obsolete in FORMS 2.2 and unsupported
    in 2.3.  Since there's no foolproof way to tell which version we're
    using, we omit them unconditionally. */
 
static PyObject *
generic_freeze_object(genericobject *g, PyObject *args)
{
	return generic_call(g, args, fl_freeze_object);
}

static PyObject *
generic_unfreeze_object(genericobject *g, PyObject *args)
{
	return generic_call(g, args, fl_unfreeze_object);
}

#endif /* OBSOLETE_FORMS_CALLS */

static PyObject *
generic_activate_object(genericobject *g, PyObject *args)
{
	return generic_call(g, args, fl_activate_object);
}

static PyObject *
generic_deactivate_object(genericobject *g, PyObject *args)
{
	return generic_call(g, args, fl_deactivate_object);
}

static PyObject *
generic_set_object_shortcut(genericobject *g, PyObject *args)
{
	char *str;
	if (!PyArg_Parse(args, "s", &str))
		return NULL;
	fl_set_object_shortcut(g->ob_generic, str);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef generic_methods[] = {
	{"set_call_back",	(PyCFunction)generic_set_call_back},
	{"delete_object",	(PyCFunction)generic_delete_object},
	{"show_object",		(PyCFunction)generic_show_object},
	{"hide_object",		(PyCFunction)generic_hide_object},
	{"redraw_object",	(PyCFunction)generic_redraw_object},
#ifdef OBSOLETE_FORMS_CALLS
	{"freeze_object",	(PyCFunction)generic_freeze_object},
	{"unfreeze_object",	(PyCFunction)generic_unfreeze_object},
#endif
	{"activate_object",	(PyCFunction)generic_activate_object},
	{"deactivate_object",	(PyCFunction)generic_deactivate_object},
	{"set_object_shortcut",	(PyCFunction)generic_set_object_shortcut},
	{NULL,			NULL}		/* sentinel */
};

static void
generic_dealloc(genericobject *g)
{
	fl_free_object(g->ob_generic);
	Py_XDECREF(g->ob_callback);
	Py_XDECREF(g->ob_callback_arg);
	PyObject_Del(g);
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

static PyObject *
generic_getattr(genericobject *g, char *name)
{
	PyObject *meth;

	/* XXX Ought to special-case name "__methods__" */
	if (g-> ob_methods) {
		meth = Py_FindMethod(g->ob_methods, (PyObject *)g, name);
		if (meth != NULL) return meth;
		PyErr_Clear();
	}

	meth = Py_FindMethod(generic_methods, (PyObject *)g, name);
	if (meth != NULL)
		return meth;
	PyErr_Clear();

	/* "label" is an exception, getmember only works for char pointers,
	   not for char arrays */
	if (strcmp(name, "label") == 0)
		return PyString_FromString(g->ob_generic->label);

	return PyMember_Get((char *)g->ob_generic, generic_memberlist, name);
}

static int
generic_setattr(genericobject *g, char *name, PyObject *v)
{
	int ret;

	if (v == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete forms object attributes");
		return -1;
	}

	/* "label" is an exception: setmember doesn't set strings;
	   and FORMS wants you to call a function to set the label */
	if (strcmp(name, "label") == 0) {
		if (!PyString_Check(v)) {
			PyErr_SetString(PyExc_TypeError,
					"label attr must be string");
			return -1;
		}
		fl_set_object_label(g->ob_generic, PyString_AsString(v));
		return 0;
	}

	ret = PyMember_Set((char *)g->ob_generic, generic_memberlist, name, v);

	/* Rather than calling all the various set_object_* functions,
	   we call fl_redraw_object here.  This is sometimes redundant
	   but I doubt that's a big problem */
	if (ret == 0)
		fl_redraw_object(g->ob_generic);

	return ret;
}

static PyObject *
generic_repr(genericobject *g)
{
	char buf[100];
	sprintf(buf, "<FORMS_object at %p, objclass=%d>",
		g, g->ob_generic->objclass);
	return PyString_FromString(buf);
}

static PyTypeObject GenericObjecttype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"FORMS_object",			/*tp_name*/
	sizeof(genericobject),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)generic_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)generic_getattr,	/*tp_getattr*/
	(setattrfunc)generic_setattr,	/*tp_setattr*/
	0,				/*tp_compare*/
	(reprfunc)generic_repr,		/*tp_repr*/
};

static PyObject *
newgenericobject(FL_OBJECT *generic, PyMethodDef *methods)
{
	genericobject *g;
	g = PyObject_New(genericobject, &GenericObjecttype);
	if (g == NULL)
		return NULL;
	g-> ob_generic = generic;
	g->ob_methods = methods;
	g->ob_callback = NULL;
	g->ob_callback_arg = NULL;
	knowgeneric(g);
	return (PyObject *)g;
}

/**********************************************************************/
/* Some common calling sequences */

/* void func (object, float) */
static PyObject *
call_forms_INf (void (*func)(FL_OBJECT *, float), FL_OBJECT *obj, PyObject *args)
{
	float parameter;

	if (!PyArg_Parse(args, "f", &parameter)) return NULL;

	(*func) (obj, parameter);

	Py_INCREF(Py_None);
	return Py_None;
}

/* void func (object, float) */
static PyObject *
call_forms_INfINf (void (*func)(FL_OBJECT *, float, float), FL_OBJECT *obj, PyObject *args)
{
	float par1, par2;

	if (!PyArg_Parse(args, "(ff)", &par1, &par2)) return NULL;

	(*func) (obj, par1, par2);

	Py_INCREF(Py_None);
	return Py_None;
}

/* void func (object, int) */
static PyObject *
call_forms_INi (void (*func)(FL_OBJECT *, int), FL_OBJECT *obj, PyObject *args)
{
	int parameter;

	if (!PyArg_Parse(args, "i", &parameter)) return NULL;

	(*func) (obj, parameter);

	Py_INCREF(Py_None);
	return Py_None;
}

/* void func (object, char) */
static PyObject *
call_forms_INc (void (*func)(FL_OBJECT *, int), FL_OBJECT *obj, PyObject *args)
{
	char *a;

	if (!PyArg_Parse(args, "s", &a)) return NULL;

	(*func) (obj, a[0]);

	Py_INCREF(Py_None);
	return Py_None;
}

/* void func (object, string) */
static PyObject *
call_forms_INstr (void (*func)(FL_OBJECT *, char *), FL_OBJECT *obj, PyObject *args)
{
	char *a;

	if (!PyArg_Parse(args, "s", &a)) return NULL;

	(*func) (obj, a);

	Py_INCREF(Py_None);
	return Py_None;
}


/* void func (object, int, string) */
static PyObject *
call_forms_INiINstr (void (*func)(FL_OBJECT *, int, char *), FL_OBJECT *obj, PyObject *args)
{
	char *b;
	int a;
	
	if (!PyArg_Parse(args, "(is)", &a, &b)) return NULL;
	
	(*func) (obj, a, b);
	
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef UNUSED
/* void func (object, int, int) */
static PyObject *
call_forms_INiINi (void (*func)(FL_OBJECT *, int, int), FL_OBJECT *obj, PyObject *args)
{
	int par1, par2;
	
	if (!PyArg_Parse(args, "(ii)", &par1, &par2)) return NULL;
	
	(*func) (obj, par1, par2);
	
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

/* int func (object) */
static PyObject *
call_forms_Ri (int (*func)(FL_OBJECT *), FL_OBJECT *obj, PyObject *args)
{
	int retval;
	
	if (!PyArg_NoArgs(args)) return NULL;
	
	retval = (*func) (obj);
	
	return PyInt_FromLong ((long) retval);
}

/* char * func (object) */
static PyObject *
call_forms_Rstr (char * (*func)(FL_OBJECT *), FL_OBJECT *obj, PyObject *args)
{
	char *str;
	
	if (!PyArg_NoArgs(args)) return NULL;
	
	str = (*func) (obj);
	
	if (str == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString (str);
}

/* int func (object) */
static PyObject *
call_forms_Rf (float (*func)(FL_OBJECT *), FL_OBJECT *obj, PyObject *args)
{
	float retval;
	
	if (!PyArg_NoArgs(args)) return NULL;
	
	retval = (*func) (obj);
	
	return PyFloat_FromDouble (retval);
}

static PyObject *
call_forms_OUTfOUTf (void (*func)(FL_OBJECT *, float *, float *), FL_OBJECT *obj, PyObject *args)
{
	float f1, f2;
	
	if (!PyArg_NoArgs(args)) return NULL;
	
	(*func) (obj, &f1, &f2);

	return Py_BuildValue("(ff)", f1, f2);
}

#ifdef UNUSED
static PyObject *
call_forms_OUTf (void (*func)(FL_OBJECT *, float *), FL_OBJECT *obj, PyObject *args)
{
	float f;

	if (!PyArg_NoArgs(args)) return NULL;

	(*func) (obj, &f);

	return PyFloat_FromDouble (f);
}
#endif

/**********************************************************************/
/* Class : browser */

static PyObject *
set_browser_topline(genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_browser_topline, g-> ob_generic, args);
}

static PyObject *
clear_browser(genericobject *g, PyObject *args)
{
	return generic_call (g, args, fl_clear_browser);
}

static PyObject *
add_browser_line (genericobject *g, PyObject *args)
{
	return call_forms_INstr (fl_add_browser_line, g-> ob_generic, args);
}

static PyObject *
addto_browser (genericobject *g, PyObject *args)
{
	return call_forms_INstr (fl_addto_browser, g-> ob_generic, args);
}

static PyObject *
insert_browser_line (genericobject *g, PyObject *args)
{
	return call_forms_INiINstr (fl_insert_browser_line,
				    g-> ob_generic, args);
}

static PyObject *
delete_browser_line (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_delete_browser_line, g-> ob_generic, args);
}

static PyObject *
replace_browser_line (genericobject *g, PyObject *args)
{
	return call_forms_INiINstr (fl_replace_browser_line,
				    g-> ob_generic, args);
}

static PyObject *
get_browser_line(genericobject *g, PyObject *args)
{
	int i;
	char *str;

	if (!PyArg_Parse(args, "i", &i))
		return NULL;

	str = fl_get_browser_line (g->ob_generic, i);

	if (str == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString (str);
}

static PyObject *
load_browser (genericobject *g, PyObject *args)
{
	/* XXX strictly speaking this is wrong since fl_load_browser
	   XXX returns int, not void */
	return call_forms_INstr (fl_load_browser, g-> ob_generic, args);
}

static PyObject *
get_browser_maxline(genericobject *g, PyObject *args)
{
	return call_forms_Ri (fl_get_browser_maxline, g-> ob_generic, args);
}

static PyObject *
select_browser_line (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_select_browser_line, g-> ob_generic, args);
}

static PyObject *
deselect_browser_line (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_deselect_browser_line, g-> ob_generic, args);
}

static PyObject *
deselect_browser (genericobject *g, PyObject *args)
{
	return generic_call (g, args, fl_deselect_browser);
}

static PyObject *
isselected_browser_line (genericobject *g, PyObject *args)
{
	int i, j;
	
	if (!PyArg_Parse(args, "i", &i))
		return NULL;
	
	j = fl_isselected_browser_line (g->ob_generic, i);
	
	return PyInt_FromLong (j);
}

static PyObject *
get_browser (genericobject *g, PyObject *args)
{
	return call_forms_Ri (fl_get_browser, g-> ob_generic, args);
}

static PyObject *
set_browser_fontsize (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_browser_fontsize, g-> ob_generic, args);
}

static PyObject *
set_browser_fontstyle (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_browser_fontstyle, g-> ob_generic, args);
}

static PyObject *
set_browser_specialkey (genericobject *g, PyObject *args)
{
	return call_forms_INc(fl_set_browser_specialkey, g-> ob_generic, args);
}

static PyMethodDef browser_methods[] = {
	{"set_browser_topline",		(PyCFunction)set_browser_topline},
	{"clear_browser",		(PyCFunction)clear_browser},
	{"add_browser_line",		(PyCFunction)add_browser_line},
	{"addto_browser",		(PyCFunction)addto_browser},
	{"insert_browser_line",		(PyCFunction)insert_browser_line},
	{"delete_browser_line",		(PyCFunction)delete_browser_line},
	{"replace_browser_line",	(PyCFunction)replace_browser_line},
	{"get_browser_line",		(PyCFunction)get_browser_line},
	{"load_browser",		(PyCFunction)load_browser},
	{"get_browser_maxline",		(PyCFunction)get_browser_maxline},
	{"select_browser_line",		(PyCFunction)select_browser_line},
	{"deselect_browser_line",	(PyCFunction)deselect_browser_line},
	{"deselect_browser",		(PyCFunction)deselect_browser},
	{"isselected_browser_line",	(PyCFunction)isselected_browser_line},
	{"get_browser",			(PyCFunction)get_browser},
	{"set_browser_fontsize",	(PyCFunction)set_browser_fontsize},
	{"set_browser_fontstyle",	(PyCFunction)set_browser_fontstyle},
	{"set_browser_specialkey",	(PyCFunction)set_browser_specialkey},
	{NULL,				NULL}		/* sentinel */
};

/* Class: button */

static PyObject *
set_button(genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_button, g-> ob_generic, args);
}

static PyObject *
get_button(genericobject *g, PyObject *args)
{
	return call_forms_Ri (fl_get_button, g-> ob_generic, args);
}

static PyObject *
get_button_numb(genericobject *g, PyObject *args)
{
	return call_forms_Ri (fl_get_button_numb, g-> ob_generic, args);
}

static PyObject *
set_button_shortcut(genericobject *g, PyObject *args)
{
	return call_forms_INstr (fl_set_button_shortcut, g-> ob_generic, args);
}

static PyMethodDef button_methods[] = {
	{"set_button",		(PyCFunction)set_button},
	{"get_button",		(PyCFunction)get_button},
	{"get_button_numb",	(PyCFunction)get_button_numb},
	{"set_button_shortcut",	(PyCFunction)set_button_shortcut},
	{NULL,			NULL}		/* sentinel */
};

/* Class: choice */

static PyObject *
set_choice(genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_choice, g-> ob_generic, args);
}

static PyObject *
get_choice(genericobject *g, PyObject *args)
{
	return call_forms_Ri (fl_get_choice, g-> ob_generic, args);
}

static PyObject *
clear_choice (genericobject *g, PyObject *args)
{
	return generic_call (g, args, fl_clear_choice);
}

static PyObject *
addto_choice (genericobject *g, PyObject *args)
{
	return call_forms_INstr (fl_addto_choice, g-> ob_generic, args);
}

static PyObject *
replace_choice (genericobject *g, PyObject *args)
{
	return call_forms_INiINstr (fl_replace_choice, g-> ob_generic, args);
}

static PyObject *
delete_choice (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_delete_choice, g-> ob_generic, args);
}

static PyObject *
get_choice_text (genericobject *g, PyObject *args)
{
	return call_forms_Rstr (fl_get_choice_text, g-> ob_generic, args);
}

static PyObject *
set_choice_fontsize (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_choice_fontsize, g-> ob_generic, args);
}

static PyObject *
set_choice_fontstyle (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_choice_fontstyle, g-> ob_generic, args);
}

static PyMethodDef choice_methods[] = {
	{"set_choice",		(PyCFunction)set_choice},
	{"get_choice",		(PyCFunction)get_choice},
	{"clear_choice",	(PyCFunction)clear_choice},
	{"addto_choice",	(PyCFunction)addto_choice},
	{"replace_choice",	(PyCFunction)replace_choice},
	{"delete_choice",	(PyCFunction)delete_choice},
	{"get_choice_text",	(PyCFunction)get_choice_text},
	{"set_choice_fontsize", (PyCFunction)set_choice_fontsize},
	{"set_choice_fontstyle",(PyCFunction)set_choice_fontstyle},
	{NULL,			NULL}		/* sentinel */
};

/* Class : Clock */

static PyObject *
get_clock(genericobject *g, PyObject *args)
{
	int i0, i1, i2;

	if (!PyArg_NoArgs(args))
		return NULL;

	fl_get_clock (g->ob_generic, &i0, &i1, &i2);

	return Py_BuildValue("(iii)", i0, i1, i2);
}

static PyMethodDef clock_methods[] = {
	{"get_clock",		(PyCFunction)get_clock},
	{NULL,			NULL}		/* sentinel */
};

/* CLass : Counters */

static PyObject *
get_counter_value(genericobject *g, PyObject *args)
{
	return call_forms_Rf (fl_get_counter_value, g-> ob_generic, args);
}

static PyObject *
set_counter_value (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_counter_value, g-> ob_generic, args);
}

static PyObject *
set_counter_precision (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_counter_precision, g-> ob_generic, args);
}

static PyObject *
set_counter_bounds (genericobject *g, PyObject *args)
{
	return call_forms_INfINf (fl_set_counter_bounds, g-> ob_generic, args);
}

static PyObject *
set_counter_step (genericobject *g, PyObject *args)
{
	return call_forms_INfINf (fl_set_counter_step, g-> ob_generic, args);
}

static PyObject *
set_counter_return (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_counter_return, g-> ob_generic, args);
}

static PyMethodDef counter_methods[] = {
	{"set_counter_value",		(PyCFunction)set_counter_value},
	{"get_counter_value",		(PyCFunction)get_counter_value},
	{"set_counter_bounds",		(PyCFunction)set_counter_bounds},
	{"set_counter_step",		(PyCFunction)set_counter_step},
	{"set_counter_precision",	(PyCFunction)set_counter_precision},
	{"set_counter_return",		(PyCFunction)set_counter_return},
	{NULL,				NULL}		/* sentinel */
};


/* Class: Dials */

static PyObject *
get_dial_value(genericobject *g, PyObject *args)
{
	return call_forms_Rf (fl_get_dial_value, g-> ob_generic, args);
}

static PyObject *
set_dial_value (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_dial_value, g-> ob_generic, args);
}

static PyObject *
set_dial_bounds (genericobject *g, PyObject *args)
{
	return call_forms_INfINf (fl_set_dial_bounds, g-> ob_generic, args);
}

static PyObject *
get_dial_bounds (genericobject *g, PyObject *args)
{
	return call_forms_OUTfOUTf (fl_get_dial_bounds, g-> ob_generic, args);
}

static PyObject *
set_dial_step (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_dial_step, g-> ob_generic, args);
}

static PyMethodDef dial_methods[] = {
	{"set_dial_value",	(PyCFunction)set_dial_value},
	{"get_dial_value",	(PyCFunction)get_dial_value},
	{"set_dial_bounds",	(PyCFunction)set_dial_bounds},
	{"get_dial_bounds",	(PyCFunction)get_dial_bounds},
	{"set_dial_step",	(PyCFunction)set_dial_step},
	{NULL,			NULL}		/* sentinel */
};

/* Class : Input */

static PyObject *
set_input (genericobject *g, PyObject *args)
{
	return call_forms_INstr (fl_set_input, g-> ob_generic, args);
}

static PyObject *
get_input (genericobject *g, PyObject *args)
{
	return call_forms_Rstr (fl_get_input, g-> ob_generic, args);
}

static PyObject *
set_input_color (genericobject *g, PyObject *args)
{
	return call_forms_INfINf (fl_set_input_color, g-> ob_generic, args);
}

static PyObject *
set_input_return (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_input_return, g-> ob_generic, args);
}

static PyMethodDef input_methods[] = {
	{"set_input",		(PyCFunction)set_input},
	{"get_input",		(PyCFunction)get_input},
	{"set_input_color",	(PyCFunction)set_input_color},
	{"set_input_return",	(PyCFunction)set_input_return},
	{NULL,			NULL}		/* sentinel */
};


/* Class : Menu */

static PyObject *
set_menu (genericobject *g, PyObject *args)
{
	return call_forms_INstr (fl_set_menu, g-> ob_generic, args);
}

static PyObject *
get_menu (genericobject *g, PyObject *args)
{
	/* XXX strictly speaking this is wrong since fl_get_menu
	   XXX returns long, not int */
	return call_forms_Ri (fl_get_menu, g-> ob_generic, args);
}

static PyObject *
get_menu_text (genericobject *g, PyObject *args)
{
	return call_forms_Rstr (fl_get_menu_text, g-> ob_generic, args);
}

static PyObject *
addto_menu (genericobject *g, PyObject *args)
{
	return call_forms_INstr (fl_addto_menu, g-> ob_generic, args);
}

static PyMethodDef menu_methods[] = {
	{"set_menu",		(PyCFunction)set_menu},
	{"get_menu",		(PyCFunction)get_menu},
	{"get_menu_text",	(PyCFunction)get_menu_text},
	{"addto_menu",		(PyCFunction)addto_menu},
	{NULL,			NULL}		/* sentinel */
};


/* Class: Sliders */

static PyObject *
get_slider_value(genericobject *g, PyObject *args)
{
	return call_forms_Rf (fl_get_slider_value, g-> ob_generic, args);
}

static PyObject *
set_slider_value (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_slider_value, g-> ob_generic, args);
}

static PyObject *
set_slider_bounds (genericobject *g, PyObject *args)
{
	return call_forms_INfINf (fl_set_slider_bounds, g-> ob_generic, args);
}

static PyObject *
get_slider_bounds (genericobject *g, PyObject *args)
{
	return call_forms_OUTfOUTf(fl_get_slider_bounds, g-> ob_generic, args);
}

static PyObject *
set_slider_return (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_slider_return, g-> ob_generic, args);
}

static PyObject *
set_slider_size (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_slider_size, g-> ob_generic, args);
}

static PyObject *
set_slider_precision (genericobject *g, PyObject *args)
{
	return call_forms_INi (fl_set_slider_precision, g-> ob_generic, args);
}

static PyObject *
set_slider_step (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_slider_step, g-> ob_generic, args);
}


static PyMethodDef slider_methods[] = {
	{"set_slider_value",	(PyCFunction)set_slider_value},
	{"get_slider_value",	(PyCFunction)get_slider_value},
	{"set_slider_bounds",	(PyCFunction)set_slider_bounds},
	{"get_slider_bounds",	(PyCFunction)get_slider_bounds},
	{"set_slider_return",	(PyCFunction)set_slider_return},
	{"set_slider_size",	(PyCFunction)set_slider_size},
	{"set_slider_precision",(PyCFunction)set_slider_precision},
	{"set_slider_step",	(PyCFunction)set_slider_step},
	{NULL,			NULL}		/* sentinel */
};

static PyObject *
set_positioner_xvalue (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_positioner_xvalue, g-> ob_generic, args);
}

static PyObject *
set_positioner_xbounds (genericobject *g, PyObject *args)
{
	return call_forms_INfINf (fl_set_positioner_xbounds,
				  g-> ob_generic, args);
}

static PyObject *
set_positioner_yvalue (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_positioner_yvalue, g-> ob_generic, args);
}

static PyObject *
set_positioner_ybounds (genericobject *g, PyObject *args)
{
	return call_forms_INfINf (fl_set_positioner_ybounds,
				  g-> ob_generic, args);
}

static PyObject *
get_positioner_xvalue (genericobject *g, PyObject *args)
{
	return call_forms_Rf (fl_get_positioner_xvalue, g-> ob_generic, args);
}

static PyObject *
get_positioner_xbounds (genericobject *g, PyObject *args)
{
	return call_forms_OUTfOUTf (fl_get_positioner_xbounds,
				    g-> ob_generic, args);
}

static PyObject *
get_positioner_yvalue (genericobject *g, PyObject *args)
{
	return call_forms_Rf (fl_get_positioner_yvalue, g-> ob_generic, args);
}

static PyObject *
get_positioner_ybounds (genericobject *g, PyObject *args)
{
	return call_forms_OUTfOUTf (fl_get_positioner_ybounds,
				    g-> ob_generic, args);
}

static PyMethodDef positioner_methods[] = {
	{"set_positioner_xvalue",	(PyCFunction)set_positioner_xvalue},
	{"set_positioner_yvalue",	(PyCFunction)set_positioner_yvalue},
	{"set_positioner_xbounds",	(PyCFunction)set_positioner_xbounds},
	{"set_positioner_ybounds",	(PyCFunction)set_positioner_ybounds},
	{"get_positioner_xvalue",	(PyCFunction)get_positioner_xvalue},
	{"get_positioner_yvalue",	(PyCFunction)get_positioner_yvalue},
	{"get_positioner_xbounds",	(PyCFunction)get_positioner_xbounds},
	{"get_positioner_ybounds",	(PyCFunction)get_positioner_ybounds},
	{NULL,			NULL}		/* sentinel */
};

/* Class timer */

static PyObject *
set_timer (genericobject *g, PyObject *args)
{
	return call_forms_INf (fl_set_timer, g-> ob_generic, args);
}

static PyObject *
get_timer (genericobject *g, PyObject *args)
{
	return call_forms_Rf (fl_get_timer, g-> ob_generic, args);
}

static PyMethodDef timer_methods[] = {
	{"set_timer",		(PyCFunction)set_timer},
	{"get_timer",		(PyCFunction)get_timer},
	{NULL,			NULL}		/* sentinel */
};

/* Form objects */

typedef struct {
	PyObject_HEAD
	FL_FORM *ob_form;
} formobject;

staticforward PyTypeObject Formtype;

#define is_formobject(v) ((v)->ob_type == &Formtype)

static PyObject *
form_show_form(formobject *f, PyObject *args)
{
	int place, border;
	char *name;
	if (!PyArg_Parse(args, "(iis)", &place, &border, &name))
		return NULL;
	fl_show_form(f->ob_form, place, border, name);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
form_call(void (*func)(FL_FORM *), FL_FORM *f, PyObject *args)
{
	if (!PyArg_NoArgs(args)) return NULL;

	(*func)(f);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
form_call_INiINi(void (*func)(FL_FORM *, int, int), FL_FORM *f, PyObject *args)
{
	int a, b;

	if (!PyArg_Parse(args, "(ii)", &a, &b)) return NULL;

	(*func)(f, a, b);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
form_call_INfINf(void (*func)(FL_FORM *, float, float), FL_FORM *f, PyObject *args)
{
	float a, b;

	if (!PyArg_Parse(args, "(ff)", &a, &b)) return NULL;

	(*func)(f, a, b);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
form_hide_form(formobject *f, PyObject *args)
{
	return form_call(fl_hide_form, f-> ob_form, args);
}

static PyObject *
form_redraw_form(formobject *f, PyObject *args)
{
	return form_call(fl_redraw_form, f-> ob_form, args);
}

static PyObject *
form_set_form_position(formobject *f, PyObject *args)
{
	return form_call_INiINi(fl_set_form_position, f-> ob_form, args);
}

static PyObject *
form_set_form_size(formobject *f, PyObject *args)
{
	return form_call_INiINi(fl_set_form_size, f-> ob_form, args);
}

static PyObject *
form_scale_form(formobject *f, PyObject *args)
{
	return form_call_INfINf(fl_scale_form, f-> ob_form, args);
}

static PyObject *
generic_add_object(formobject *f, PyObject *args, FL_OBJECT *(*func)(int, float, float, float, float, char*), PyMethodDef *internal_methods)
{
	int type;
	float x, y, w, h;
	char *name;
	FL_OBJECT *obj;

	if (!PyArg_Parse(args,"(iffffs)", &type,&x,&y,&w,&h,&name))
		return NULL;

	fl_addto_form (f-> ob_form);

	obj = (*func) (type, x, y, w, h, name);

	fl_end_form();

	if (obj == NULL) {
		PyErr_NoMemory();
		return NULL;
	}

	return newgenericobject (obj, internal_methods);
}

static PyObject *
form_add_button(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_button, button_methods);
}

static PyObject *
form_add_lightbutton(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_lightbutton, button_methods);
}

static PyObject *
form_add_roundbutton(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_roundbutton, button_methods);
}

static PyObject *
form_add_menu (formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_menu, menu_methods);
}

static PyObject *
form_add_slider(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_slider, slider_methods);
}

static PyObject *
form_add_valslider(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_valslider, slider_methods);
}

static PyObject *
form_add_dial(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_dial, dial_methods);
}

static PyObject *
form_add_counter(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_counter, counter_methods);
}

static PyObject *
form_add_clock(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_clock, clock_methods);
}

static PyObject *
form_add_box(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_box,
				  (PyMethodDef *)NULL);
}

static PyObject *
form_add_choice(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_choice, choice_methods);
}

static PyObject *
form_add_browser(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_browser, browser_methods);
}

static PyObject *
form_add_positioner(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_positioner,
				  positioner_methods);
}

static PyObject *
form_add_input(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_input, input_methods);
}

static PyObject *
form_add_text(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_text,
				  (PyMethodDef *)NULL);
}

static PyObject *
form_add_timer(formobject *f, PyObject *args)
{
	return generic_add_object(f, args, fl_add_timer, timer_methods);
}

static PyObject *
form_freeze_form(formobject *f, PyObject *args)
{
	return form_call(fl_freeze_form, f-> ob_form, args);
}

static PyObject *
form_unfreeze_form(formobject *f, PyObject *args)
{
	return form_call(fl_unfreeze_form, f-> ob_form, args);
}

static PyObject *
form_activate_form(formobject *f, PyObject *args)
{
	return form_call(fl_activate_form, f-> ob_form, args);
}

static PyObject *
form_deactivate_form(formobject *f, PyObject *args)
{
	return form_call(fl_deactivate_form, f-> ob_form, args);
}

static PyObject *
form_bgn_group(formobject *f, PyObject *args)
{
	FL_OBJECT *obj;

	fl_addto_form(f-> ob_form);
	obj = fl_bgn_group();
	fl_end_form();

	if (obj == NULL) {
		PyErr_NoMemory();
		return NULL;
	}

	return newgenericobject (obj, (PyMethodDef *) NULL);
}

static PyObject *
form_end_group(formobject *f, PyObject *args)
{
	fl_addto_form(f-> ob_form);
	fl_end_group();
	fl_end_form();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_find_first_or_last(FL_OBJECT *(*func)(FL_FORM *, int, float, float), formobject *f, PyObject *args)
{
	int type;
	float mx, my;
	FL_OBJECT *generic;
	genericobject *g;
	
	if (!PyArg_Parse(args, "(iff)", &type, &mx, &my)) return NULL;

	generic = (*func) (f-> ob_form, type, mx, my);

	if (generic == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	g = findgeneric(generic);
	if (g == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
			   "forms_find_{first|last} returns unknown object");
		return NULL;
	}
	Py_INCREF(g);
	return (PyObject *) g;
}

static PyObject *
form_find_first(formobject *f, PyObject *args)
{
	return forms_find_first_or_last(fl_find_first, f, args);
}

static PyObject *
form_find_last(formobject *f, PyObject *args)
{
	return forms_find_first_or_last(fl_find_last, f, args);
}

static PyObject *
form_set_object_focus(formobject *f, PyObject *args)
{
	genericobject *g;
	if (args == NULL || !is_genericobject(args)) {
		PyErr_BadArgument();
		return NULL;
	}
	g = (genericobject *)args;
	fl_set_object_focus(f->ob_form, g->ob_generic);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef form_methods[] = {
/* adm */
	{"show_form",		(PyCFunction)form_show_form},
	{"hide_form",		(PyCFunction)form_hide_form},
	{"redraw_form",		(PyCFunction)form_redraw_form},
	{"set_form_position",	(PyCFunction)form_set_form_position},
	{"set_form_size",	(PyCFunction)form_set_form_size},
	{"scale_form",		(PyCFunction)form_scale_form},
	{"freeze_form",		(PyCFunction)form_freeze_form},
	{"unfreeze_form",	(PyCFunction)form_unfreeze_form},
	{"activate_form",	(PyCFunction)form_activate_form},
	{"deactivate_form",	(PyCFunction)form_deactivate_form},
	{"bgn_group",		(PyCFunction)form_bgn_group},
	{"end_group",		(PyCFunction)form_end_group},
	{"find_first",		(PyCFunction)form_find_first},
	{"find_last",		(PyCFunction)form_find_last},
	{"set_object_focus",	(PyCFunction)form_set_object_focus},

/* basic objects */
	{"add_button",		(PyCFunction)form_add_button},
/*	{"add_bitmap",		(method)form_add_bitmap}, */
	{"add_lightbutton",	(PyCFunction)form_add_lightbutton},
	{"add_roundbutton",	(PyCFunction)form_add_roundbutton},
	{"add_menu",		(PyCFunction)form_add_menu},
	{"add_slider",		(PyCFunction)form_add_slider},
	{"add_positioner",	(PyCFunction)form_add_positioner},
	{"add_valslider",	(PyCFunction)form_add_valslider},
	{"add_dial",		(PyCFunction)form_add_dial},
	{"add_counter",		(PyCFunction)form_add_counter},
	{"add_box",		(PyCFunction)form_add_box},
	{"add_clock",		(PyCFunction)form_add_clock},
	{"add_choice",		(PyCFunction)form_add_choice},
	{"add_browser",		(PyCFunction)form_add_browser},
	{"add_input",		(PyCFunction)form_add_input},
	{"add_timer",		(PyCFunction)form_add_timer},
	{"add_text",		(PyCFunction)form_add_text},
	{NULL,			NULL}		/* sentinel */
};

static void
form_dealloc(formobject *f)
{
	releaseobjects(f->ob_form);
	if (f->ob_form->visible)
		fl_hide_form(f->ob_form);
	fl_free_form(f->ob_form);
	PyObject_Del(f);
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

static PyObject *
form_getattr(formobject *f, char *name)
{
	PyObject *meth;

	meth = Py_FindMethod(form_methods, (PyObject *)f, name);
	if (meth != NULL)
		return meth;
	PyErr_Clear();
	return PyMember_Get((char *)f->ob_form, form_memberlist, name);
}

static int
form_setattr(formobject *f, char *name, PyObject *v)
{
	if (v == NULL) {
		PyErr_SetString(PyExc_TypeError,
				"can't delete form attributes");
		return -1;
	}

	return PyMember_Set((char *)f->ob_form, form_memberlist, name, v);
}

static PyObject *
form_repr(formobject *f)
{
	char buf[100];
	sprintf(buf, "<FORMS_form at %p, window=%ld>",
		f, f->ob_form->window);
	return PyString_FromString(buf);
}

static PyTypeObject Formtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"FORMS_form",			/*tp_name*/
	sizeof(formobject),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)form_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)form_getattr,	/*tp_getattr*/
	(setattrfunc)form_setattr,	/*tp_setattr*/
	0,				/*tp_compare*/
	(reprfunc)form_repr,		/*tp_repr*/
};

static PyObject *
newformobject(FL_FORM *form)
{
	formobject *f;
	f = PyObject_New(formobject, &Formtype);
	if (f == NULL)
		return NULL;
	f->ob_form = form;
	return (PyObject *)f;
}


/* The "fl" module */

static PyObject *
forms_make_form(PyObject *dummy, PyObject *args)
{
	int type;
	float w, h;
	FL_FORM *form;
	if (!PyArg_Parse(args, "(iff)", &type, &w, &h))
		return NULL;
	form = fl_bgn_form(type, w, h);
	if (form == NULL) {
		/* XXX Actually, cannot happen! */
		PyErr_NoMemory();
		return NULL;
	}
	fl_end_form();
	return newformobject(form);
}

static PyObject *
forms_activate_all_forms(PyObject *f, PyObject *args)
{
	fl_activate_all_forms();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_deactivate_all_forms(PyObject *f, PyObject *args)
{
	fl_deactivate_all_forms();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *my_event_callback = NULL;

static PyObject *
forms_set_event_call_back(PyObject *dummy, PyObject *args)
{
	if (args == Py_None)
		args = NULL;
	my_event_callback = args;
	Py_XINCREF(args);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_do_or_check_forms(PyObject *dummy, PyObject *args, FL_OBJECT *(*func)(void))
{
	FL_OBJECT *generic;
	genericobject *g;
	PyObject *arg, *res;
	
	if (!PyArg_NoArgs(args))
		return NULL;

	for (;;) {
		Py_BEGIN_ALLOW_THREADS
		generic = (*func)();
		Py_END_ALLOW_THREADS
		if (generic == NULL) {
			Py_INCREF(Py_None);
			return Py_None;
		}
		if (generic == FL_EVENT) {
			int dev;
			short val;
			if (my_event_callback == NULL)
				return PyInt_FromLong(-1L);
			dev = fl_qread(&val);
			arg = Py_BuildValue("(ih)", dev, val);
			if (arg == NULL)
				return NULL;
			res = PyEval_CallObject(my_event_callback, arg);
			Py_XDECREF(res);
			Py_DECREF(arg);
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
			Py_INCREF(g);
			return ((PyObject *) g);
		}
		arg = Py_BuildValue("(OO)", (PyObject *)g, g->ob_callback_arg);
		if (arg == NULL)
			return NULL;
		res = PyEval_CallObject(g->ob_callback, arg);
		Py_XDECREF(res);
		Py_DECREF(arg);
		if (res == NULL)
			return NULL; /* Callback raised exception */
	}
}

static PyObject *
forms_do_forms(PyObject *dummy, PyObject *args)
{
	return forms_do_or_check_forms(dummy, args, fl_do_forms);
}

static PyObject *
forms_check_forms(PyObject *dummy, PyObject *args)
{
	return forms_do_or_check_forms(dummy, args, fl_check_forms);
}

static PyObject *
forms_do_only_forms(PyObject *dummy, PyObject *args)
{
	return forms_do_or_check_forms(dummy, args, fl_do_only_forms);
}

static PyObject *
forms_check_only_forms(PyObject *dummy, PyObject *args)
{
	return forms_do_or_check_forms(dummy, args, fl_check_only_forms);
}

#ifdef UNUSED
static PyObject *
fl_call(void (*func)(void), PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	(*func)();
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

static PyObject *
forms_set_graphics_mode(PyObject *dummy, PyObject *args)
{
	int rgbmode, doublebuf;

	if (!PyArg_Parse(args, "(ii)", &rgbmode, &doublebuf))
		return NULL;
	fl_set_graphics_mode(rgbmode,doublebuf);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_get_rgbmode(PyObject *dummy, PyObject *args)
{
	extern int fl_rgbmode;

	if (args != NULL) {
		PyErr_BadArgument();
		return NULL;
	}
	return PyInt_FromLong((long)fl_rgbmode);
}

static PyObject *
forms_show_errors(PyObject *dummy, PyObject *args)
{
	int show;
	if (!PyArg_Parse(args, "i", &show))
		return NULL;
	fl_show_errors(show);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_set_font_name(PyObject *dummy, PyObject *args)
{
	int numb;
	char *name;
	if (!PyArg_Parse(args, "(is)", &numb, &name))
		return NULL;
	fl_set_font_name(numb, name);
	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *
forms_qdevice(PyObject *self, PyObject *args)
{
	short arg1;
	if (!PyArg_Parse(args, "h", &arg1))
		return NULL;
	fl_qdevice(arg1);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_unqdevice(PyObject *self, PyObject *args)
{
	short arg1;
	if (!PyArg_Parse(args, "h", &arg1))
		return NULL;
	fl_unqdevice(arg1);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_isqueued(PyObject *self, PyObject *args)
{
	long retval;
	short arg1;
	if (!PyArg_Parse(args, "h", &arg1))
		return NULL;
	retval = fl_isqueued(arg1);

	return PyInt_FromLong(retval);
}

static PyObject *
forms_qtest(PyObject *self, PyObject *args)
{
	long retval;
	retval = fl_qtest();
	return PyInt_FromLong(retval);
}


static PyObject *
forms_qread(PyObject *self, PyObject *args)
{
	int dev;
	short val;
	Py_BEGIN_ALLOW_THREADS
	dev = fl_qread(&val);
	Py_END_ALLOW_THREADS
	return Py_BuildValue("(ih)", dev, val);
}

static PyObject *
forms_qreset(PyObject *self, PyObject *args)
{
	if (!PyArg_NoArgs(args)) return NULL;

	fl_qreset();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_qenter(PyObject *self, PyObject *args)
{
	short arg1, arg2;
	if (!PyArg_Parse(args, "(hh)", &arg1, &arg2))
		return NULL;
	fl_qenter(arg1, arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_color(PyObject *self, PyObject *args)
{
	int arg;

	if (!PyArg_Parse(args, "i", &arg)) return NULL;

	fl_color((short) arg);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_mapcolor(PyObject *self, PyObject *args)
{
	int arg0, arg1, arg2, arg3;

	if (!PyArg_Parse(args, "(iiii)", &arg0, &arg1, &arg2, &arg3))
		return NULL;

	fl_mapcolor(arg0, (short) arg1, (short) arg2, (short) arg3);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_getmcolor(PyObject *self, PyObject *args)
{
	int arg;
	short r, g, b;

	if (!PyArg_Parse(args, "i", &arg)) return NULL;

	fl_getmcolor(arg, &r, &g, &b);

	return Py_BuildValue("(hhh)", r, g, b);
}

static PyObject *
forms_get_mouse(PyObject *self, PyObject *args)
{
	float x, y;

	if (!PyArg_NoArgs(args)) return NULL;
	
	fl_get_mouse(&x, &y);

	return Py_BuildValue("(ff)", x, y);
}

static PyObject *
forms_tie(PyObject *self, PyObject *args)
{
	short arg1, arg2, arg3;
	if (!PyArg_Parse(args, "(hhh)", &arg1, &arg2, &arg3))
		return NULL;
	fl_tie(arg1, arg2, arg3);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_show_message(PyObject *f, PyObject *args)
{
	char *a, *b, *c;

	if (!PyArg_Parse(args, "(sss)", &a, &b, &c)) return NULL;

	Py_BEGIN_ALLOW_THREADS
	fl_show_message(a, b, c);
	Py_END_ALLOW_THREADS

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
forms_show_choice(PyObject *f, PyObject *args)
{
	char *m1, *m2, *m3, *b1, *b2, *b3;
	int nb;
	char *format;
	long rv;

	if (args == NULL || !PyTuple_Check(args)) {
		PyErr_BadArgument();
		return NULL;
	}
	nb = PyTuple_Size(args) - 3;
	if (nb <= 0) {
		PyErr_SetString(PyExc_TypeError,
				"need at least one button label");
		return NULL;
	}
	if (PyInt_Check(PyTuple_GetItem(args, 3))) {
		PyErr_SetString(PyExc_TypeError,
			   "'number-of-buttons' argument not needed");
		return NULL;
	}
	switch (nb) {
	case 1: format = "(ssss)"; break;
	case 2: format = "(sssss)"; break;
	case 3: format = "(ssssss)"; break;
	default:
		PyErr_SetString(PyExc_TypeError, "too many button labels");
		return NULL;
	}

	if (!PyArg_Parse(args, format, &m1, &m2, &m3, &b1, &b2, &b3))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	rv = fl_show_choice(m1, m2, m3, nb, b1, b2, b3);
	Py_END_ALLOW_THREADS
	return PyInt_FromLong(rv);
}

static PyObject *
forms_show_question(PyObject *f, PyObject *args)
{
	int ret;
	char *a, *b, *c;

	if (!PyArg_Parse(args, "(sss)", &a, &b, &c)) return NULL;

	Py_BEGIN_ALLOW_THREADS
	ret = fl_show_question(a, b, c);
	Py_END_ALLOW_THREADS

	return PyInt_FromLong((long) ret);
}

static PyObject *
forms_show_input(PyObject *f, PyObject *args)
{
	char *str;
	char *a, *b;

	if (!PyArg_Parse(args, "(ss)", &a, &b)) return NULL;

	Py_BEGIN_ALLOW_THREADS
	str = fl_show_input(a, b);
	Py_END_ALLOW_THREADS

	if (str == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(str);
}

static PyObject *
forms_file_selector(PyObject *f, PyObject *args)
{
	char *str;
	char *a, *b, *c, *d;

	if (!PyArg_Parse(args, "(ssss)", &a, &b, &c, &d)) return NULL;

	Py_BEGIN_ALLOW_THREADS
	str = fl_show_file_selector(a, b, c, d);
	Py_END_ALLOW_THREADS

	if (str == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(str);
}


static PyObject *
forms_file_selector_func(PyObject *args, char *(*func)(void))
{
	char *str;

	str = (*func) ();

	if (str == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return PyString_FromString(str);
}

static PyObject *
forms_get_directory(PyObject *f, PyObject *args)
{
	return forms_file_selector_func(args, fl_get_directory);
}

static PyObject *
forms_get_pattern(PyObject *f, PyObject *args)
{
	return forms_file_selector_func(args, fl_get_pattern);
}

static PyObject *
forms_get_filename(PyObject *f, PyObject *args)
{
	return forms_file_selector_func(args, fl_get_filename);
}

static PyMethodDef forms_methods[] = {
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

DL_EXPORT(void)
initfl(void)
{
	Py_InitModule("fl", forms_methods);
	foreground();
	fl_init();
}



