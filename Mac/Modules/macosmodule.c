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

#include <errno.h>

#include <OSUtils.h> /* for Set(Current)A5 */
#include <Resources.h>
#include <Memory.h>
#include <Sound.h>

#ifndef __MWERKS__
#define SndCallBackUPP ProcPtr
#define NewSndCallBackProc(x) (x)
#endif

/*----------------------------------------------------------------------*/
/* General tools */

static PyObject *MacOS_Error; /* Exception MacOS.Error */

/*----------------------------------------------------------------------*/
/* Resource objects */

typedef struct {
	PyObject_HEAD
	Handle h;
} RsrcObject;

staticforward PyTypeObject RsrcType;

#define Rsrc_Check(r) ((r)->ob_type == &RsrcType)

static RsrcObject *
Rsrc_FromHandle(Handle h)
{
	RsrcObject *r;
	if (h == NULL)
		return (RsrcObject *)PyErr_Mac(MacOS_Error, (int)ResError());
	r = PyObject_NEW(RsrcObject, &RsrcType);
	if (r != NULL)
		r->h = h;
	return r;
}

static void
Rsrc_Dealloc(RsrcObject *r)
{
	/* XXXX Note by Jack: shouldn't we ReleaseResource here? */
	PyMem_DEL(r);
}

static PyObject *
Rsrc_GetResInfo(RsrcObject *r, PyObject *args)
{
	short id;
	ResType type;
	Str255 name;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	GetResInfo(r->h, &id, &type, name);
	return Py_BuildValue("(is#s#)",
		(int)id, (char *)&type, 4, name+1, (int)name[0]);
}

static PyObject *
Rsrc_AsBytes(RsrcObject *r, PyObject *args)
{
	long len;
	PyObject *rv;
	char *cp;
	
	if (!PyArg_ParseTuple(args, "l", &len))
		return NULL;
	HLock(r->h);
	cp = (char *)*r->h;
	rv = PyString_FromStringAndSize(cp, len);
	HUnlock(r->h);
	return rv;
}

static PyObject *
Rsrc_AsString(RsrcObject *r, PyObject *args)
{
	PyObject *rv;
	unsigned char *cp;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	HLock(r->h);
	cp = (unsigned char *)*r->h;
	rv = PyString_FromStringAndSize((char *)cp+1, cp[0]);
	HUnlock(r->h);
	return rv;
}

static PyMethodDef Rsrc_Methods[] = {
	{"GetResInfo",	(PyCFunction)Rsrc_GetResInfo, 1},
	{"AsString",	(PyCFunction)Rsrc_AsString, 1},
	{"AsBytes",		(PyCFunction)Rsrc_AsBytes, 1},
	{NULL,			NULL}		 /* Sentinel */
};

static PyObject *
Rsrc_GetAttr(PyObject *r, char *name)
{
	return Py_FindMethod(Rsrc_Methods, r, name);
}

static PyTypeObject RsrcType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"Resource",			/*tp_name*/
	sizeof(RsrcObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)Rsrc_Dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)Rsrc_GetAttr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

static PyObject *
MacOS_GetResource(PyObject *self, PyObject *args)
{
	ResType rt;
	int id;
	Handle h;
	if (!PyArg_ParseTuple(args, "O&i", PyMac_GetOSType, &rt, &id))
		return NULL;
	h = GetResource(rt, id);
	return (PyObject *)Rsrc_FromHandle(h);
}

static PyObject *
MacOS_GetNamedResource(PyObject *self, PyObject *args)
{
	ResType rt;
	Str255 name;
	Handle h;
	if (!PyArg_ParseTuple(args, "O&O&", PyMac_GetOSType, &rt, PyMac_GetStr255, &name))
		return NULL;
	h = GetNamedResource(rt, name);
	return (PyObject *)Rsrc_FromHandle(h);
}

/*----------------------------------------------------------------------*/
/* Miscellaneous File System Operations */

static PyObject *
MacOS_GetFileType(PyObject *self, PyObject *args)
{
	Str255 name;
	FInfo info;
	PyObject *type, *creator, *res;
	OSErr err;
	
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetStr255, &name))
		return NULL;
	if ((err = GetFInfo(name, 0, &info)) != noErr) {
		errno = err;
		PyErr_SetFromErrno(MacOS_Error);
		return NULL;
	}
	type = PyString_FromStringAndSize((char *)&info.fdType, 4);
	creator = PyString_FromStringAndSize((char *)&info.fdCreator, 4);
	res = Py_BuildValue("OO", type, creator);
	Py_DECREF(type);
	Py_DECREF(creator);
	return res;
}

static PyObject *
MacOS_SetFileType(PyObject *self, PyObject *args)
{
	Str255 name;
	ResType type, creator;
	FInfo info;
	OSErr err;
	
	if (!PyArg_ParseTuple(args, "O&O&O&",
			PyMac_GetStr255, &name, PyMac_GetOSType, &type, PyMac_GetOSType, &creator))
		return NULL;
	if ((err = GetFInfo(name, 0, &info)) != noErr) {
		errno = err;
		PyErr_SetFromErrno(MacOS_Error);
		return NULL;
	}
	info.fdType = type;
	info.fdCreator = creator;
	if ((err = SetFInfo(name, 0, &info)) != noErr) {
		errno = err;
		PyErr_SetFromErrno(MacOS_Error);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/*----------------------------------------------------------------------*/
/* SoundChannel objects */

/* Convert a SndCommand argument */
static int
GetSndCommand(PyObject *v, SndCommand *pc)
{
	int len;
	pc->param1 = 0;
	pc->param2 = 0;
	if (PyTuple_Check(v)) {
		if (PyArg_ParseTuple(v, "h|hl", &pc->cmd, &pc->param1, &pc->param2))
			return 1;
		PyErr_Clear();
		return PyArg_ParseTuple(v, "hhs#", &pc->cmd, &pc->param1, &pc->param2, &len);
	}
	return PyArg_Parse(v, "h", &pc->cmd);
}

typedef struct {
	PyObject_HEAD
	SndChannelPtr chan;
} SndChObject;

staticforward PyTypeObject SndChType;

#define SndCh_Check(s) ((s)->ob_type == &SndChType)

static SndChObject *
SndCh_FromSndChannelPtr(SndChannelPtr chan)
{
	SndChObject *s = PyObject_NEW(SndChObject, &SndChType);
	if (s != NULL)
		s->chan = chan;
	return s;
}

static void
SndCh_Cleanup(SndChObject *s, int quitNow)
{
	SndChannelPtr chan = s->chan;
	if (chan != NULL) {
		void *userInfo = (void *)chan->userInfo;
		s->chan = NULL;
		SndDisposeChannel(chan, quitNow);
		if (userInfo != 0)
			PyMem_DEL(userInfo);
	}
}

static void
SndCh_Dealloc(SndChObject *s)
{
	SndCh_Cleanup(s, 1);
	PyMem_DEL(s);
}

static PyObject *
SndCh_DisposeChannel(SndChObject *s, PyObject *args)
{
	int quitNow = 1;
	if (PyTuple_Size(args) > 0) {
		if (!PyArg_ParseTuple(args, "i", &quitNow))
			return NULL;
	}
	SndCh_Cleanup(s, quitNow);
	Py_INCREF(Py_None);
	return Py_None;
}

static int
SndCh_OK(SndChObject *s)
{
	if (s->chan == NULL) {
		PyErr_SetString(MacOS_Error, "channel is closed");
		return 0;
	}
	return 1;
}

static PyObject *
SndCh_SndPlay(SndChObject *s, PyObject *args)
{
	RsrcObject *r;
	int async = 0;
	if (!PyArg_ParseTuple(args, "O!|i", RsrcType, &r, &async))
		return NULL;
	if (!SndCh_OK(s))
		return NULL;
	SndPlay(s->chan, (SndListHandle)r->h, async);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SndCh_SndDoCommand(SndChObject *s, PyObject *args)
{
	SndCommand c;
	int noWait = 0;
	OSErr err;
	if (!PyArg_ParseTuple(args, "O&|i", GetSndCommand, &c, &noWait))
		return NULL;
	if (!SndCh_OK(s))
		return NULL;
	err = SndDoCommand(s->chan, &c, noWait);
	return PyErr_Mac(MacOS_Error, (int)err);
}

static PyObject *
SndCh_SndDoImmediate(SndChObject *s, PyObject *args)
{
	SndCommand c;
	OSErr err;
	if (!PyArg_ParseTuple(args, "O&", GetSndCommand, &c))
		return 0;
	if (!SndCh_OK(s))
		return NULL;
	err = SndDoImmediate(s->chan, &c);
	return PyErr_Mac(MacOS_Error, (int)err);
}

static PyMethodDef SndCh_Methods[] = {
	{"close",				(PyCFunction)SndCh_DisposeChannel, 1},
	{"SndDisposeChannel",	(PyCFunction)SndCh_DisposeChannel, 1},
	{"SndPlay",				(PyCFunction)SndCh_SndPlay, 1},
	{"SndDoCommand",		(PyCFunction)SndCh_SndDoCommand, 1},
	{"SndDoImmediate",		(PyCFunction)SndCh_SndDoImmediate, 1},
	{NULL,					NULL}		 /* Sentinel */
};

static PyObject *
SndCh_GetAttr(PyObject *s, char *name)
{
	return Py_FindMethod(SndCh_Methods, s, name);
}

static PyTypeObject SndChType = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"SoundChannel",			/*tp_name*/
	sizeof(SndChObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)SndCh_Dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)SndCh_GetAttr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

/*----------------------------------------------------------------------*/
/* Module */

typedef struct {
	long A5;
	PyObject *callback;
	PyObject *channel;
	SndCommand cmd;
} cbinfo;

static int
MySafeCallback(arg)
	void *arg;
{
	cbinfo *p = (cbinfo *)arg;
	PyObject *args;
	PyObject *res;
	args = Py_BuildValue("(O(hhl))",
						 p->channel, p->cmd.cmd, p->cmd.param1, p->cmd.param2);
	res = PyEval_CallObject(p->callback, args);
	Py_DECREF(args);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static pascal void
#ifdef __MWERKS__
MyUserRoutine(SndChannelPtr chan, SndCommand *cmd)
#else
MyUserRoutine(SndChannelPtr chan, SndCommand cmd)
#endif
{
	cbinfo *p = (cbinfo *)chan->userInfo;
	long A5 = SetA5(p->A5);
#ifdef __MWERKS__
	p->cmd = *cmd;
#else
	p->cmd = cmd;
#endif
	Py_AddPendingCall(MySafeCallback, (void *)p);
	SetA5(A5);
}

static PyObject *
MacOS_SndNewChannel(PyObject *self, PyObject *args)
{
	SndChannelPtr chan;
	short synth;
	long init = 0;
	PyObject *callback = NULL;
	cbinfo *p = NULL;
	SndCallBackUPP userroutine = 0;
	OSErr err;
	PyObject *res;
	if (!PyArg_ParseTuple(args, "h|lO", &synth, &init, &callback))
		return NULL;
	if (callback != NULL) {
		p = PyMem_NEW(cbinfo, 1);
		if (p == NULL)
			return PyErr_NoMemory();
		p->A5 = SetCurrentA5();
		p->callback = callback;
		userroutine = NewSndCallBackProc(MyUserRoutine);
	}
	chan = NULL;
	err = SndNewChannel(&chan, synth, init, userroutine);
	if (err) {
		if (p)
			PyMem_DEL(p);
		return PyErr_Mac(MacOS_Error, (int)err);
	}
	res = (PyObject *)SndCh_FromSndChannelPtr(chan);
	if (res == NULL) {
		SndDisposeChannel(chan, 1);
		PyMem_DEL(p);
	}
	else {
		chan->userInfo = (long)p;
		p->channel = res;
	}
	return res;
}

static PyObject *
MacOS_SndPlay(PyObject *self, PyObject *args)
{
	RsrcObject *r;
	OSErr err;
	if (!PyArg_ParseTuple(args, "O!", &RsrcType, &r))
		return NULL;
	err = SndPlay((SndChannelPtr)NULL, (SndListHandle)r->h, 0);
	return PyErr_Mac(MacOS_Error, (int)err);
}

static PyObject *
MacOS_SndControl(PyObject *self, PyObject *args)
{
	int id;
	SndCommand c;
	OSErr err;
	if (!PyArg_ParseTuple(args, "iO&", &id, GetSndCommand, &c))
		return NULL;
	err = SndControl(id, &c);
	if (err)
		return PyErr_Mac(MacOS_Error, (int)err);
	return Py_BuildValue("(hhl)", c.cmd, c.param1, c.param2);
}

/*----------------------------------------------------------------------*/
/* STDWIN High Level Event interface */

#include <EPPC.h>
#include <Events.h>

#ifdef USE_STDWIN

extern void (*_w_high_level_event_proc)(EventRecord *);

static PyObject *MacOS_HighLevelEventHandler = NULL;

static void
MacOS_HighLevelEventProc(EventRecord *erp)
{
	if (MacOS_HighLevelEventHandler != NULL) {
		PyObject *args = Py_BuildValue("(s#)", (char *)erp, (int)sizeof(*erp));
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

static PyMethodDef MacOS_Methods[] = {
	{"AcceptHighLevelEvent",	MacOS_AcceptHighLevelEvent, 1},
	{"GetResource",			MacOS_GetResource, 1},
	{"GetNamedResource",		MacOS_GetNamedResource, 1},
	{"GetFileType",			MacOS_GetFileType, 1},
	{"SetFileType",			MacOS_SetFileType, 1},
	{"SndNewChannel",		MacOS_SndNewChannel, 1},
	{"SndPlay",			MacOS_SndPlay, 1},
	{"SndControl",			MacOS_SndControl, 1},
#ifdef USE_STDWIN
	{"SetHighLevelEventHandler",	MacOS_SetHighLevelEventHandler, 1},
#endif
	{NULL,				NULL}		 /* Sentinel */
};


void
MacOS_Init()
{
	PyObject *m, *d;
	
	m = Py_InitModule("MacOS", MacOS_Methods);
	d = PyModule_GetDict(m);
	
	/* Initialize MacOS.Error exception */
	MacOS_Error = PyString_FromString("MacOS.Error");
	if (MacOS_Error == NULL || PyDict_SetItemString(d, "Error", MacOS_Error) != 0)
		Py_FatalError("can't define MacOS.Error");
}
