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

/* Macintosh OS-specific interface */

#include "Python.h"
#include "macglue.h"

#include <Windows.h>

static PyObject *MacOS_Error; /* Exception MacOS.Error */

#ifdef MPW
#define bufferIsSmall -607	/*error returns from Post and Accept */
#endif


/*----------------------------------------------------------------------*/
/* Miscellaneous File System Operations */

static PyObject *
MacOS_GetCreatorAndType(PyObject *self, PyObject *args)
{
	Str255 name;
	FInfo info;
	PyObject *creator, *type, *res;
	OSErr err;
	
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetStr255, &name))
		return NULL;
	if ((err = GetFInfo(name, 0, &info)) != noErr)
		return PyErr_Mac(MacOS_Error, err);
	creator = PyString_FromStringAndSize((char *)&info.fdCreator, 4);
	type = PyString_FromStringAndSize((char *)&info.fdType, 4);
	res = Py_BuildValue("OO", creator, type);
	Py_DECREF(creator);
	Py_DECREF(type);
	return res;
}

static PyObject *
MacOS_SetCreatorAndType(PyObject *self, PyObject *args)
{
	Str255 name;
	ResType creator, type;
	FInfo info;
	OSErr err;
	
	if (!PyArg_ParseTuple(args, "O&O&O&",
			PyMac_GetStr255, &name, PyMac_GetOSType, &creator, PyMac_GetOSType, &type))
		return NULL;
	if ((err = GetFInfo(name, 0, &info)) != noErr)
		return PyErr_Mac(MacOS_Error, err);
	info.fdCreator = creator;
	info.fdType = type;
	if ((err = SetFInfo(name, 0, &info)) != noErr)
		return PyErr_Mac(MacOS_Error, err);
	Py_INCREF(Py_None);
	return Py_None;
}

/*----------------------------------------------------------------------*/
/* STDWIN High Level Event interface */

#include <EPPC.h>
#include <Events.h>

#ifdef USE_STDWIN

extern void (*_w_high_level_event_proc)(EventRecord *);

static PyObject *MacOS_HighLevelEventHandler = NULL;

static void
MacOS_HighLevelEventProc(EventRecord *e)
{
	if (MacOS_HighLevelEventHandler != NULL) {
		PyObject *args = PyMac_BuildEventRecord(e);
		PyObject *res;
		if (args == NULL)
			res = NULL;
		else {
			res = PyEval_CallObject(MacOS_HighLevelEventHandler, args);
			Py_DECREF(args);
		}
		if (res == NULL) {
			fprintf(stderr, "Exception in MacOS_HighLevelEventProc:\n");
			PyErr_Print();
		}
		else
			Py_DECREF(res);
	}
}

/* XXXX Need to come here from PyMac_DoYield too... */

static PyObject *
MacOS_SetHighLevelEventHandler(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *previous = MacOS_HighLevelEventHandler;
	PyObject *next = NULL;
	if (!PyArg_ParseTuple(args, "|O", &next))
		return NULL;
	if (next == Py_None)
		next = NULL;
	Py_INCREF(next);
	MacOS_HighLevelEventHandler = next;
	if (next == NULL)
		_w_high_level_event_proc = NULL;
	else
		_w_high_level_event_proc = MacOS_HighLevelEventProc;
	if (previous == NULL) {
		Py_INCREF(Py_None);
		previous = Py_None;
	}
	return previous;
}

#endif /* USE_STDWIN */

static PyObject *
MacOS_AcceptHighLevelEvent(self, args)
	PyObject *self;
	PyObject *args;
{
	TargetID sender;
	unsigned long refcon;
	Ptr buf;
	unsigned long len;
	OSErr err;
	PyObject *res;
	
	buf = NULL;
	len = 0;
	err = AcceptHighLevelEvent(&sender, &refcon, buf, &len);
	if (err == bufferIsSmall) {
		buf = malloc(len);
		if (buf == NULL)
			return PyErr_NoMemory();
		err = AcceptHighLevelEvent(&sender, &refcon, buf, &len);
		if (err != noErr) {
			free(buf);
			return PyErr_Mac(MacOS_Error, (int)err);
		}
	}
	else if (err != noErr)
		return PyErr_Mac(MacOS_Error, (int)err);
	res = Py_BuildValue("s#ls#",
		(char *)&sender, (int)(sizeof sender), refcon, (char *)buf, (int)len);
	free(buf);
	return res;
}

/*
** Set poll frequency and cpu-yield-time
*/
static PyObject *
MacOS_SetScheduleTimes(PyObject *self, PyObject *args)
{
	long fgi, fgy, bgi, bgy;
	
	bgi = bgy = -2;	
	if (!PyArg_ParseTuple(args, "ll|ll", &fgi, &fgy, &bgi, &bgy))
		return NULL;
	if ( bgi == -2 || bgy == -2 ) {
		bgi = fgi;
		bgy = fgy;
	}
	PyMac_SetYield(fgi, fgy, bgi, bgy);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
MacOS_EnableAppswitch(PyObject *self, PyObject *args)
{
	int enable;
	
	if (!PyArg_ParseTuple(args, "i", &enable))
		return NULL;
	PyMac_DoYieldEnabled = enable;
	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *
MacOS_HandleEvent(PyObject *self, PyObject *args)
{
	EventRecord ev;
	
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetEventRecord, &ev))
		return NULL;
	PyMac_HandleEvent(&ev);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef MacOS_Methods[] = {
	{"AcceptHighLevelEvent",	MacOS_AcceptHighLevelEvent, 1},
	{"GetCreatorAndType",		MacOS_GetCreatorAndType, 1},
	{"SetCreatorAndType",		MacOS_SetCreatorAndType, 1},
#ifdef USE_STDWIN
	{"SetHighLevelEventHandler",	MacOS_SetHighLevelEventHandler, 1},
#endif
	{"SetScheduleTimes",	MacOS_SetScheduleTimes, 1},
	{"EnableAppswitch",		MacOS_EnableAppswitch, 1},
	{"HandleEvent",			MacOS_HandleEvent, 1},
	{NULL,				NULL}		 /* Sentinel */
};


void
MacOS_Init()
{
	PyObject *m, *d;
	
	m = Py_InitModule("MacOS", MacOS_Methods);
	d = PyModule_GetDict(m);
	
	/* Initialize MacOS.Error exception */
	MacOS_Error = PyMac_GetOSErrException();
	if (MacOS_Error == NULL || PyDict_SetItemString(d, "Error", MacOS_Error) != 0)
		Py_FatalError("can't define MacOS.Error");
}
