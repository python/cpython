/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Font Manager module */

#include "Python.h"

#include <gl.h>
#include <device.h>
#include <fmclient.h>


/* Font Handle object implementation */

typedef struct {
	PyObject_HEAD
	fmfonthandle fh_fh;
} fhobject;

staticforward PyTypeObject Fhtype;

#define is_fhobject(v)		((v)->ob_type == &Fhtype)

static PyObject *
newfhobject(fh)
	fmfonthandle fh;
{
	fhobject *fhp;
	if (fh == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
				"error creating new font handle");
		return NULL;
	}
	fhp = PyObject_New(fhobject, &Fhtype);
	if (fhp == NULL)
		return NULL;
	fhp->fh_fh = fh;
	return (PyObject *)fhp;
}

/* Font Handle methods */

static PyObject *
fh_scalefont(self, args)
	fhobject *self;
PyObject *args;
{
	double size;
	if (!PyArg_Parse(args, "d", &size))
		return NULL;
	return newfhobject(fmscalefont(self->fh_fh, size));
}

/* XXX fmmakefont */

static PyObject *
fh_setfont(self, args)
	fhobject *self;
PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	fmsetfont(self->fh_fh);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
fh_getfontname(self, args)
	fhobject *self;
PyObject *args;
{
	char fontname[256];
	int len;
	if (!PyArg_NoArgs(args))
		return NULL;
	len = fmgetfontname(self->fh_fh, sizeof fontname, fontname);
	if (len < 0) {
		PyErr_SetString(PyExc_RuntimeError, "error in fmgetfontname");
		return NULL;
	}
	return PyString_FromStringAndSize(fontname, len);
}

static PyObject *
fh_getcomment(self, args)
	fhobject *self;
PyObject *args;
{
	char comment[256];
	int len;
	if (!PyArg_NoArgs(args))
		return NULL;
	len = fmgetcomment(self->fh_fh, sizeof comment, comment);
	if (len < 0) {
		PyErr_SetString(PyExc_RuntimeError, "error in fmgetcomment");
		return NULL;
	}
	return PyString_FromStringAndSize(comment, len);
}

static PyObject *
fh_getfontinfo(self, args)
	fhobject *self;
PyObject *args;
{
	fmfontinfo info;
	if (!PyArg_NoArgs(args))
		return NULL;
	if (fmgetfontinfo(self->fh_fh, &info) < 0) {
		PyErr_SetString(PyExc_RuntimeError, "error in fmgetfontinfo");
		return NULL;
	}
	return Py_BuildValue("(llllllll)",
			     info.printermatched,
			     info.fixed_width,
			     info.xorig,
			     info.yorig,
			     info.xsize,
			     info.ysize,
			     info.height,
			     info.nglyphs);
}

#if 0
static PyObject *
fh_getwholemetrics(self, args)
	fhobject *self;
PyObject *args;
{
}
#endif

static PyObject *
fh_getstrwidth(self, args)
	fhobject *self;
PyObject *args;
{
	char *str;
	if (!PyArg_Parse(args, "s", &str))
		return NULL;
	return PyInt_FromLong(fmgetstrwidth(self->fh_fh, str));
}

static PyMethodDef fh_methods[] = {
	{"scalefont",	(PyCFunction)fh_scalefont},
	{"setfont",	(PyCFunction)fh_setfont},
	{"getfontname",	(PyCFunction)fh_getfontname},
	{"getcomment",	(PyCFunction)fh_getcomment},
	{"getfontinfo",	(PyCFunction)fh_getfontinfo},
#if 0
	{"getwholemetrics",	(PyCFunction)fh_getwholemetrics},
#endif
	{"getstrwidth",	(PyCFunction)fh_getstrwidth},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
fh_getattr(fhp, name)
	fhobject *fhp;
char *name;
{
	return Py_FindMethod(fh_methods, (PyObject *)fhp, name);
}

static void
fh_dealloc(fhp)
	fhobject *fhp;
{
	fmfreefont(fhp->fh_fh);
	PyObject_Del(fhp);
}

static PyTypeObject Fhtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"font handle",			/*tp_name*/
	sizeof(fhobject),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)fh_dealloc,		/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)fh_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
};


/* Font Manager functions */

static PyObject *
fm_init(self, args)
	PyObject *self, *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	fminit();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
fm_findfont(self, args)
	PyObject *self, *args;
{
	char *str;
	if (!PyArg_Parse(args, "s", &str))
		return NULL;
	return newfhobject(fmfindfont(str));
}

static PyObject *
fm_prstr(self, args)
	PyObject *self, *args;
{
	char *str;
	if (!PyArg_Parse(args, "s", &str))
		return NULL;
	fmprstr(str);
	Py_INCREF(Py_None);
	return Py_None;
}

/* XXX This uses a global variable as temporary! Not re-entrant! */

static PyObject *fontlist;

static void
clientproc(fontname)
	char *fontname;
{
	int err;
	PyObject *v;
	if (fontlist == NULL)
		return;
	v = PyString_FromString(fontname);
	if (v == NULL)
		err = -1;
	else {
		err = PyList_Append(fontlist, v);
		Py_DECREF(v);
	}
	if (err != 0) {
		Py_DECREF(fontlist);
		fontlist = NULL;
	}
}

static PyObject *
fm_enumerate(self, args)
	PyObject *self, *args;
{
	PyObject *res;
	if (!PyArg_NoArgs(args))
		return NULL;
	fontlist = PyList_New(0);
	if (fontlist == NULL)
		return NULL;
	fmenumerate(clientproc);
	res = fontlist;
	fontlist = NULL;
	return res;
}

static PyObject *
fm_setpath(self, args)
	PyObject *self, *args;
{
	char *str;
	if (!PyArg_Parse(args, "s", &str))
		return NULL;
	fmsetpath(str);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
fm_fontpath(self, args)
	PyObject *self, *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyString_FromString(fmfontpath());
}

static PyMethodDef fm_methods[] = {
	{"init",	fm_init},
	{"findfont",	fm_findfont},
	{"enumerate",	fm_enumerate},
	{"prstr",	fm_prstr},
	{"setpath",	fm_setpath},
	{"fontpath",	fm_fontpath},
	{NULL,		NULL}		/* sentinel */
};


void
initfm()
{
	Py_InitModule("fm", fm_methods);
	fminit();
}
