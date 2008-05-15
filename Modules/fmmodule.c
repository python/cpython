
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

static PyTypeObject Fhtype;

#define is_fhobject(v)		((v)->ob_type == &Fhtype)

static PyObject *
newfhobject(fmfonthandle fh)
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
fh_scalefont(fhobject *self, PyObject *args)
{
	double size;
	if (!PyArg_ParseTuple(args, "d", &size))
		return NULL;
	return newfhobject(fmscalefont(self->fh_fh, size));
}

/* XXX fmmakefont */

static PyObject *
fh_setfont(fhobject *self)
{
	fmsetfont(self->fh_fh);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
fh_getfontname(fhobject *self)
{
	char fontname[256];
	int len;
	len = fmgetfontname(self->fh_fh, sizeof fontname, fontname);
	if (len < 0) {
		PyErr_SetString(PyExc_RuntimeError, "error in fmgetfontname");
		return NULL;
	}
	return PyString_FromStringAndSize(fontname, len);
}

static PyObject *
fh_getcomment(fhobject *self)
{
	char comment[256];
	int len;
	len = fmgetcomment(self->fh_fh, sizeof comment, comment);
	if (len < 0) {
		PyErr_SetString(PyExc_RuntimeError, "error in fmgetcomment");
		return NULL;
	}
	return PyString_FromStringAndSize(comment, len);
}

static PyObject *
fh_getfontinfo(fhobject *self)
{
	fmfontinfo info;
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
fh_getwholemetrics(fhobject *self, PyObject *args)
{
}
#endif

static PyObject *
fh_getstrwidth(fhobject *self, PyObject *args)
{
	char *str;
	if (!PyArg_ParseTuple(args, "s", &str))
		return NULL;
	return PyInt_FromLong(fmgetstrwidth(self->fh_fh, str));
}

static PyMethodDef fh_methods[] = {
	{"scalefont",	(PyCFunction)fh_scalefont,   METH_VARARGS},
	{"setfont",	(PyCFunction)fh_setfont,     METH_NOARGS},
	{"getfontname",	(PyCFunction)fh_getfontname, METH_NOARGS},
	{"getcomment",	(PyCFunction)fh_getcomment,  METH_NOARGS},
	{"getfontinfo",	(PyCFunction)fh_getfontinfo, METH_NOARGS},
#if 0
	{"getwholemetrics",	(PyCFunction)fh_getwholemetrics, METH_VARARGS},
#endif
	{"getstrwidth",	(PyCFunction)fh_getstrwidth, METH_VARARGS},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
fh_getattr(fhobject *fhp, char *name)
{
	return Py_FindMethod(fh_methods, (PyObject *)fhp, name);
}

static void
fh_dealloc(fhobject *fhp)
{
	fmfreefont(fhp->fh_fh);
	PyObject_Del(fhp);
}

static PyTypeObject Fhtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"fm.font handle",		/*tp_name*/
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
fm_init(PyObject *self)
{
	fminit();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
fm_findfont(PyObject *self, PyObject *args)
{
	char *str;
	if (!PyArg_ParseTuple(args, "s", &str))
		return NULL;
	return newfhobject(fmfindfont(str));
}

static PyObject *
fm_prstr(PyObject *self, PyObject *args)
{
	char *str;
	if (!PyArg_ParseTuple(args, "s", &str))
		return NULL;
	fmprstr(str);
	Py_INCREF(Py_None);
	return Py_None;
}

/* XXX This uses a global variable as temporary! Not re-entrant! */

static PyObject *fontlist;

static void
clientproc(char *fontname)
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
fm_enumerate(PyObject *self)
{
	PyObject *res;
	fontlist = PyList_New(0);
	if (fontlist == NULL)
		return NULL;
	fmenumerate(clientproc);
	res = fontlist;
	fontlist = NULL;
	return res;
}

static PyObject *
fm_setpath(PyObject *self, PyObject *args)
{
	char *str;
	if (!PyArg_ParseTuple(args, "s", &str))
		return NULL;
	fmsetpath(str);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
fm_fontpath(PyObject *self)
{
	return PyString_FromString(fmfontpath());
}

static PyMethodDef fm_methods[] = {
	{"init",	fm_init,      METH_NOARGS},
	{"findfont",	fm_findfont,  METH_VARARGS},
	{"enumerate",	fm_enumerate, METH_NOARGS},
	{"prstr",	fm_prstr,     METH_VARARGS},
	{"setpath",	fm_setpath,   METH_VARARGS},
	{"fontpath",	fm_fontpath,  METH_NOARGS},
	{NULL,		NULL}		/* sentinel */
};


void
initfm(void)
{
    
    if (PyErr_WarnPy3k("the fm module has been removed in "
                       "Python 3.0", 2) < 0)
        return;
    
	Py_InitModule("fm", fm_methods);
	if (m == NULL)
		return;
	fminit();
}
