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

#include "Python.h"
#include "InterslipLib.h"
#include "macglue.h"

static PyObject *ErrorObject;

/* ----------------------------------------------------- */

static char pyis_open__doc__[] =
"Load the interslip driver (optional)"
;

static PyObject *
pyis_open(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	OSErr err;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = is_open();
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char pyis_connect__doc__[] =
"Tell the driver to start a connect"
;

static PyObject *
pyis_connect(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	OSErr err;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = is_connect();
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char pyis_disconnect__doc__[] =
"Tell the interslip driver to start a disconnect"
;

static PyObject *
pyis_disconnect(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	OSErr err;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = is_disconnect();
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char pyis_status__doc__[] =
"Return (numeric_status, message_seqnum, message_string) status tuple"
;

static PyObject *
pyis_status(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	long status, seqnum;
	StringPtr message;
	OSErr err;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = is_status(&status, &seqnum, &message);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("iiO&", (int)status, (int)seqnum, PyMac_BuildStr255, message);
}

static char pyis_getconfig__doc__[] =
"Return configuration data (ibaud, obaud, flags, idrvname, odrvname, cfgname)"
;

static PyObject *
pyis_getconfig(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	long baudrate, flags;
	Str255 idrvname, odrvname, cfgname;
	OSErr err;
	int ibaud, obaud;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = is_getconfig(&baudrate, &flags, idrvname, odrvname, cfgname);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	ibaud = (baudrate >> 16) & 0xffff;
	obaud = baudrate & 0xffff;
	return Py_BuildValue("iiiO&O&O&", ibaud, obaud, (int)flags, PyMac_BuildStr255, idrvname,
			PyMac_BuildStr255, odrvname, PyMac_BuildStr255, cfgname);
}

static char pyis_setconfig__doc__[] =
"Set configuration data (ibaud, obaud, flags, idrvname, odrvname, cfgname)"
;

static PyObject *
pyis_setconfig(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	long baudrate;
	int flags;
	Str255 idrvname, odrvname, cfgname;
	OSErr err;
	int ibaud, obaud;

	if (!PyArg_ParseTuple(args, "iiiO&O&O&", &ibaud, &obaud, &flags, PyMac_GetStr255, idrvname,
			PyMac_GetStr255, odrvname, PyMac_GetStr255, cfgname))
		return NULL;
	baudrate = (ibaud << 16) | obaud;
	err = is_setconfig(baudrate, (long)flags, idrvname, odrvname, cfgname);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/* List of methods defined in the module */

static struct PyMethodDef pyis_methods[] = {
	{"open",	pyis_open,	1,	pyis_open__doc__},
 {"connect",	pyis_connect,	1,	pyis_connect__doc__},
 {"disconnect",	pyis_disconnect,	1,	pyis_disconnect__doc__},
 {"status",	pyis_status,	1,	pyis_status__doc__},
 {"getconfig",	pyis_getconfig,	1,	pyis_getconfig__doc__},
 {"setconfig",	pyis_setconfig,	1,	pyis_setconfig__doc__},
 
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initinterslip) */

static char interslip_module_documentation[] = 
""
;

void
initinterslip()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule4("interslip", pyis_methods,
		interslip_module_documentation,
		(PyObject*)NULL,PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("interslip.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* XXXX Add constants here */

	PyDict_SetItemString(d, "IDLE", PyInt_FromLong(IS_IDLE));
	PyDict_SetItemString(d, "WMODEM", PyInt_FromLong(IS_WMODEM));
	PyDict_SetItemString(d, "DIAL", PyInt_FromLong(IS_DIAL));
	PyDict_SetItemString(d, "LOGIN", PyInt_FromLong(IS_LOGIN));
	PyDict_SetItemString(d, "RUN", PyInt_FromLong(IS_RUN));
	PyDict_SetItemString(d, "DISC", PyInt_FromLong(IS_DISC));
	
	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module interslip");
}

