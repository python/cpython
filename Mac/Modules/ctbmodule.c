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

/* Note: This file is partially converted to the new naming standard */
/* ctbcm objects */


#include "Python.h"

#include "macglue.h"

#include <CommResources.h>
#include <Connections.h>
#include <ToolUtils.h>
#include <OSUtils.h>

#ifndef HAVE_UNIVERSAL_HEADERS
#define ConnectionCompletionUPP ProcPtr
#define ConnectionChooseIdleUPP ProcPtr
#define NewConnectionCompletionProc(x) (x)
#define NewConnectionChooseIdleProc(x) (x)
#endif

#define 	_UnimplementedToolTrap	0xA89F
#define 	_CommToolboxTrap		0x8B
#define 	_UnimplementedOSTrap	0x9F

extern PyObject *PyErr_Mac(PyObject *,int);

static PyObject *ErrorObject;

typedef struct {
	PyObject_HEAD
	ConnHandle hdl;		/* The handle to the connection */
	PyObject *callback;	/* Python callback routine */
	int has_callback;	/* True if callback not None */
	int err;			/* Error to pass to the callback */
} ctbcmobject;

staticforward PyTypeObject ctbcmtype;

#define is_ctbcmobject(v)		((v)->ob_type == &ctbcmtype)

static
TrapAvailable(short tNumber, TrapType tType)
{
	short unImplemented;
	
	if (tType == OSTrap)
		unImplemented = _UnimplementedOSTrap;
	else
		unImplemented = _UnimplementedToolTrap;

	return NGetTrapAddress(tNumber, tType) != NGetTrapAddress(unImplemented, tType);
}

static
initialize_ctb()
{
	OSErr err;
	static initialized = -1;
	
	if ( initialized >= 0 )
		return initialized;
	initialized = 0;
	
	if ( !TrapAvailable(_CommToolboxTrap, OSTrap) ) {
		PyErr_SetString(ErrorObject, "CTB not available");
		return 0;
	}
	if ( (err=InitCTBUtilities()) ) {
		PyErr_Mac(ErrorObject, (int)err);
		return 0;
	}
	if ( (err=InitCRM()) ) {
		PyErr_Mac(ErrorObject, (int)err);
		return 0;
	}
	if ( (err=InitCM()) ) {
		PyErr_Mac(ErrorObject, (int)err);
		return 0;
	}
	initialized = 1;
	return 1;
}

static int
ctbcm_pycallback(arg)
	void *arg;
{
	ctbcmobject *self = (ctbcmobject *)arg;
	PyObject *args, *rv;
	
	if ( !self->has_callback )    /* It could have been removed in the meantime */
		return 0;
	args = Py_BuildValue("(i)", self->err);
	rv = PyEval_CallObject(self->callback, args);
	Py_DECREF(args);
	if( rv == NULL )
		return -1;
	Py_DECREF(rv);
	return 0;
}

/*DBG*/int ncallback;
static pascal void
ctbcm_ctbcallback(hconn)
	ConnHandle hconn;
{
	ctbcmobject *self;
	
	/* XXXX Do I have to do the A5 mumbo-jumbo? */
	ncallback++; /*DBG*/
	self = (ctbcmobject *)CMGetUserData(hconn);
	self->err = (int)((*hconn)->errCode);
	Py_AddPendingCall(ctbcm_pycallback, (void *)self);
}

static ctbcmobject *
newctbcmobject(arg)
	PyObject *arg;
{
	ctbcmobject *self;
	self = PyObject_NEW(ctbcmobject, &ctbcmtype);
	if (self == NULL)
		return NULL;
	self->hdl = NULL;
	Py_INCREF(Py_None);
	self->callback = Py_None;
	self->has_callback = 0;
	return self;
}

/* ctbcm methods */

static void
ctbcm_dealloc(self)
	ctbcmobject *self;
{
	if ( self->hdl ) {
		(void)CMClose(self->hdl, 0, (ConnectionCompletionUPP)0, 0, 1);
		/*XXXX Is this safe? */
		CMDispose(self->hdl);
		self->hdl = NULL;
	}
	PyMem_DEL(self);
}

static PyObject *
ctbcm_open(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	long timeout;
	OSErr err;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!PyArg_Parse(args, "l", &timeout))
		return NULL;
	if ( (err=CMOpen(self->hdl, self->has_callback, cb_upp, timeout)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ctbcm_listen(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	long timeout;
	OSErr err;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!PyArg_Parse(args, "l", &timeout))
		return NULL;
	if ( (err=CMListen(self->hdl,self->has_callback, cb_upp, timeout)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ctbcm_accept(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	int accept;
	OSErr err;
	
	if (!PyArg_Parse(args, "i", &accept))
		return NULL;
	if ( (err=CMAccept(self->hdl, accept)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ctbcm_close(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	int now;
	long timeout;
	OSErr err;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!PyArg_Parse(args, "(li)", &timeout, &now))
		return NULL;
	if ( (err=CMClose(self->hdl, self->has_callback, cb_upp, timeout, now)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ctbcm_read(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	long timeout, len;
	int chan;
	CMFlags flags;
	OSErr err;
	PyObject *rv;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!PyArg_Parse(args, "(lil)", &len, &chan, &timeout))
		return NULL;
	if ((rv=PyString_FromStringAndSize(NULL, len)) == NULL)
		return NULL;
	if ((err=CMRead(self->hdl, (Ptr)PyString_AsString(rv), &len, (CMChannel)chan,
				self->has_callback, cb_upp, timeout, &flags)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	_PyString_Resize(&rv, len);
	return Py_BuildValue("(Oi)", rv, (int)flags);
}

static PyObject *
ctbcm_write(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	long timeout, len;
	int chan, ilen, flags;
	OSErr err;
	char *buf;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!PyArg_Parse(args, "(s#ili)", &buf, &ilen, &chan, &timeout, &flags))
		return NULL;
	len = ilen;
	if ((err=CMWrite(self->hdl, (Ptr)buf, &len, (CMChannel)chan,
				self->has_callback, cb_upp, timeout, (CMFlags)flags)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	return PyInt_FromLong((int)len);
}

static PyObject *
ctbcm_status(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	CMBufferSizes sizes;
	CMStatFlags flags;
	OSErr err;
	PyObject *rv;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if ((err=CMStatus(self->hdl, sizes, &flags)) < 0)
		return PyErr_Mac(ErrorObject, (int)err);
	rv = Py_BuildValue("(llllll)", sizes[0], sizes[1], sizes[2], sizes[3], sizes[4], sizes[5]);
	if ( rv == NULL )
		return NULL;
	return Py_BuildValue("(Ol)", rv, (long)flags);
}

static PyObject *
ctbcm_getconfig(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	char *rv;

	if (!PyArg_NoArgs(args))
		return NULL;
	if ((rv=(char *)CMGetConfig(self->hdl)) == NULL ) {
		PyErr_SetString(ErrorObject, "CMGetConfig failed");
		return NULL;
	}
	return PyString_FromString(rv);
}

static PyObject *
ctbcm_setconfig(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	char *cfg;
	OSErr err;
	
	if (!PyArg_Parse(args, "s", &cfg))
		return NULL;
	if ((err=CMSetConfig(self->hdl, (Ptr)cfg)) < 0)
		return PyErr_Mac(ErrorObject, err);
	return PyInt_FromLong((int)err);
}

static PyObject *
ctbcm_choose(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	int rv;
	Point pt;

	if (!PyArg_NoArgs(args))
		return NULL;
	pt.v = 40;
	pt.h = 40;
	rv=CMChoose(&self->hdl, pt, (ConnectionChooseIdleUPP)0);
	return PyInt_FromLong(rv);
}

static PyObject *
ctbcm_idle(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	CMIdle(self->hdl);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ctbcm_abort(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	CMAbort(self->hdl);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ctbcm_reset(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	CMReset(self->hdl);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ctbcm_break(self, args)
	ctbcmobject *self;
	PyObject *args;
{
	long duration;
	ConnectionCompletionUPP cb_upp = NewConnectionCompletionProc(ctbcm_ctbcallback);
	
	if (!PyArg_Parse(args, "l", &duration))
		return NULL;
	CMBreak(self->hdl, duration,self->has_callback, cb_upp);
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef ctbcm_methods[] = {
	{"Open",		(PyCFunction)ctbcm_open},
	{"Close",		(PyCFunction)ctbcm_close},
	{"Read",		(PyCFunction)ctbcm_read},
	{"Write",		(PyCFunction)ctbcm_write},
	{"Status",		(PyCFunction)ctbcm_status},
	{"GetConfig",	(PyCFunction)ctbcm_getconfig},
	{"SetConfig",	(PyCFunction)ctbcm_setconfig},
	{"Choose",		(PyCFunction)ctbcm_choose},
	{"Idle",		(PyCFunction)ctbcm_idle},
	{"Listen",		(PyCFunction)ctbcm_listen},
	{"Accept",		(PyCFunction)ctbcm_accept},
	{"Abort",		(PyCFunction)ctbcm_abort},
	{"Reset",		(PyCFunction)ctbcm_reset},
	{"Break",		(PyCFunction)ctbcm_break},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
ctbcm_getattr(self, name)
	ctbcmobject *self;
	char *name;
{
	if ( strcmp(name, "callback") == 0 ) {
		Py_INCREF(self->callback);
		return self->callback;
	}
	return Py_FindMethod(ctbcm_methods, (PyObject *)self, name);
}

static int
ctbcm_setattr(self, name, v)
	ctbcmobject *self;
	char *name;
	PyObject *v;
{
	if ( strcmp(name, "callback") != 0 ) {
		PyErr_SetString(PyExc_AttributeError, "ctbcm objects have callback attr only");
		return -1;
	}
	if ( v == NULL ) {
		v = Py_None;
	}
	Py_INCREF(v);	/* XXXX Must I do this? */
	self->callback = v;
	self->has_callback = (v != Py_None);
	return 0;
}

statichere PyTypeObject ctbcmtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"ctbcm",			/*tp_name*/
	sizeof(ctbcmobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)ctbcm_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)ctbcm_getattr, /*tp_getattr*/
	(setattrfunc)ctbcm_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
/* --------------------------------------------------------------------- */

/* Function of no arguments returning new ctbcm object */

static PyObject *
ctb_cmnew(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int strlen;
	PyObject *sizes_obj;
	char *c_str;
	unsigned char p_str[255];
	CMBufferSizes sizes;
	short procid;
	ConnHandle hdl;
	ctbcmobject *rv;
	
	if (!PyArg_Parse(args, "(s#O)", &c_str, &strlen, &sizes_obj))
		return NULL;
	strncpy((char *)p_str+1, c_str, strlen);
	p_str[0] = strlen;
	if (!initialize_ctb())
		return NULL;
	if ( sizes_obj == Py_None ) {
		memset(sizes, '\0', sizeof sizes);
	} else {
		if ( !PyArg_Parse(sizes_obj, "(llllll)", &sizes[0], &sizes[1], &sizes[2],
						&sizes[3], &sizes[4], &sizes[5]))
			return NULL;
	}
	if ( (procid=CMGetProcID(p_str)) < 0 )
		return PyErr_Mac(ErrorObject, procid);
	hdl = CMNew(procid, cmNoMenus|cmQuiet, sizes, 0, 0);
	if ( hdl == NULL ) {
		PyErr_SetString(ErrorObject, "CMNew failed");
		return NULL;
	}
	rv = newctbcmobject(args);
	if ( rv == NULL )
	    return NULL;   /* XXXX Should dispose of hdl */
	rv->hdl = hdl;
	CMSetUserData(hdl, (long)rv);
	return (PyObject *)rv;
}

static PyObject *
ctb_available(self, args)
	PyObject *self;
	PyObject *args;
{
	int ok;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	ok = initialize_ctb();
	PyErr_Clear();
	return PyInt_FromLong(ok);
}

/* List of functions defined in the module */

static struct PyMethodDef ctb_methods[] = {
	{"CMNew",		ctb_cmnew},
	{"available",	ctb_available},
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initctb) */

void
initctb()
{
	PyObject *m, *d, *o;

	/* Create the module and add the functions */
	m = Py_InitModule("ctb", ctb_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	
#define CMCONST(name, value) o = PyInt_FromLong(value); PyDict_SetItemString(d, name, o)

	CMCONST("cmData", 1);
	CMCONST("cmCntl", 2);
	CMCONST("cmAttn", 3);
	
	CMCONST("cmFlagsEOM", 1);
	
	CMCONST("chooseDisaster", -2);
	CMCONST("chooseFailed", -1);
	CMCONST("chooseAborted", 0);
	CMCONST("chooseOKMinor", 1);
	CMCONST("chooseOKMajor", 2);
	CMCONST("chooseCancel", 3);
	
	CMCONST("cmStatusOpening", 1);
	CMCONST("cmStatusOpen", 2);
	CMCONST("cmStatusClosing", 4);
	CMCONST("cmStatusDataAvail", 8);
	CMCONST("cmStatusCntlAvail", 0x10);
	CMCONST("cmStatusAttnAvail", 0x20);
	CMCONST("cmStatusDRPend", 0x40);
	CMCONST("cmStatusDWPend", 0x80);
	CMCONST("cmStatusCWPend", 0x100);
	CMCONST("cmStatusCWPend", 0x200);
	CMCONST("cmStatusARPend", 0x400);
	CMCONST("cmStatusAWPend", 0x800);
	CMCONST("cmStatusBreakPending", 0x1000);
	CMCONST("cmStatusListenPend", 0x2000);
	CMCONST("cmStatusIncomingCallPresent", 0x4000);
	
	ErrorObject = PyString_FromString("ctb.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module ctb");
}
