/**********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* CD module -- interface to Mark Callow's and Roger Chickering's */
 /* CD Audio Library (CD). */

#include <sys/types.h>
#include <cdaudio.h>
#include "Python.h"

#define NCALLBACKS	8

typedef struct {
	PyObject_HEAD
	CDPLAYER *ob_cdplayer;
} cdplayerobject;

static PyObject *CdError;		/* exception cd.error */

static PyObject *
CD_allowremoval(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	CDallowremoval(self->ob_cdplayer);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CD_preventremoval(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	CDpreventremoval(self->ob_cdplayer);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CD_bestreadsize(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	return PyInt_FromLong((long) CDbestreadsize(self->ob_cdplayer));
}

static PyObject *
CD_close(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	if (!CDclose(self->ob_cdplayer)) {
		PyErr_SetFromErrno(CdError); /* XXX - ??? */
		return NULL;
	}
	self->ob_cdplayer = NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CD_eject(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	if (!CDeject(self->ob_cdplayer)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "eject failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
	
static PyObject *
CD_getstatus(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	if (!CDgetstatus(self->ob_cdplayer, &status)) {
		PyErr_SetFromErrno(CdError); /* XXX - ??? */
		return NULL;
	}

	return Py_BuildValue("(ii(iii)(iii)(iii)iiii)", status.state,
		       status.track, status.min, status.sec, status.frame,
		       status.abs_min, status.abs_sec, status.abs_frame,
		       status.total_min, status.total_sec, status.total_frame,
		       status.first, status.last, status.scsi_audio,
		       status.cur_block);
}
	
static PyObject *
CD_gettrackinfo(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int track;
	CDTRACKINFO info;
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, "i", &track))
		return NULL;

	if (!CDgettrackinfo(self->ob_cdplayer, track, &info)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "gettrackinfo failed");
		return NULL;
	}

	return Py_BuildValue("((iii)(iii))",
		       info.start_min, info.start_sec, info.start_frame,
		       info.total_min, info.total_sec, info.total_frame);
}
	
static PyObject *
CD_msftoblock(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int min, sec, frame;

	if (!PyArg_ParseTuple(args, "iii", &min, &sec, &frame))
		return NULL;

	return PyInt_FromLong((long) CDmsftoblock(self->ob_cdplayer,
						min, sec, frame));
}
	
static PyObject *
CD_play(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int start, play;
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, "ii", &start, &play))
		return NULL;

	if (!CDplay(self->ob_cdplayer, start, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "play failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
	
static PyObject *
CD_playabs(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int min, sec, frame, play;
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, "iiii", &min, &sec, &frame, &play))
		return NULL;

	if (!CDplayabs(self->ob_cdplayer, min, sec, frame, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "playabs failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
	
static PyObject *
CD_playtrack(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int start, play;
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, "ii", &start, &play))
		return NULL;

	if (!CDplaytrack(self->ob_cdplayer, start, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "playtrack failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
	
static PyObject *
CD_playtrackabs(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int track, min, sec, frame, play;
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, "iiiii", &track, &min, &sec,
			      &frame, &play))
		return NULL;

	if (!CDplaytrackabs(self->ob_cdplayer, track, min, sec, frame, play)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "playtrackabs failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
	
static PyObject *
CD_readda(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int numframes, n;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "i", &numframes))
		return NULL;

	result = PyString_FromStringAndSize(NULL, numframes * sizeof(CDFRAME));
	if (result == NULL)
		return NULL;

	n = CDreadda(self->ob_cdplayer,
		       (CDFRAME *) PyString_AsString(result), numframes);
	if (n == -1) {
		Py_DECREF(result);
		PyErr_SetFromErrno(CdError);
		return NULL;
	}
	if (n < numframes)
		if (_PyString_Resize(&result, n * sizeof(CDFRAME)))
			return NULL;

	return result;
}

static PyObject *
CD_seek(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int min, sec, frame;
	long PyTryBlock;

	if (!PyArg_ParseTuple(args, "iii", &min, &sec, &frame))
		return NULL;

	PyTryBlock = CDseek(self->ob_cdplayer, min, sec, frame);
	if (PyTryBlock == -1) {
		PyErr_SetFromErrno(CdError);
		return NULL;
	}

	return PyInt_FromLong(PyTryBlock);
}
	
static PyObject *
CD_seektrack(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	int track;
	long PyTryBlock;

	if (!PyArg_ParseTuple(args, "i", &track))
		return NULL;

	PyTryBlock = CDseektrack(self->ob_cdplayer, track);
	if (PyTryBlock == -1) {
		PyErr_SetFromErrno(CdError);
		return NULL;
	}

	return PyInt_FromLong(PyTryBlock);
}
	
static PyObject *
CD_seekblock(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	unsigned long PyTryBlock;

	if (!PyArg_ParseTuple(args, "l", &PyTryBlock))
		return NULL;

	PyTryBlock = CDseekblock(self->ob_cdplayer, PyTryBlock);
	if (PyTryBlock == (unsigned long) -1) {
		PyErr_SetFromErrno(CdError);
		return NULL;
	}

	return PyInt_FromLong(PyTryBlock);
}
	
static PyObject *
CD_stop(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	if (!CDstop(self->ob_cdplayer)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "stop failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
	
static PyObject *
CD_togglepause(self, args)
	cdplayerobject *self;
	PyObject *args;
{
	CDSTATUS status;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	if (!CDtogglepause(self->ob_cdplayer)) {
		if (CDgetstatus(self->ob_cdplayer, &status) &&
		    status.state == CD_NODISC)
			PyErr_SetString(CdError, "no disc in player");
		else
			PyErr_SetString(CdError, "togglepause failed");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
	
static PyMethodDef cdplayer_methods[] = {
	{"allowremoval",	(PyCFunction)CD_allowremoval,	1},
	{"bestreadsize",	(PyCFunction)CD_bestreadsize,	1},
	{"close",		(PyCFunction)CD_close,		1},
	{"eject",		(PyCFunction)CD_eject,		1},
	{"getstatus",		(PyCFunction)CD_getstatus,		1},
	{"gettrackinfo",	(PyCFunction)CD_gettrackinfo,	1},
	{"msftoblock",		(PyCFunction)CD_msftoblock,		1},
	{"play",		(PyCFunction)CD_play,		1},
	{"playabs",		(PyCFunction)CD_playabs,		1},
	{"playtrack",		(PyCFunction)CD_playtrack,		1},
	{"playtrackabs",	(PyCFunction)CD_playtrackabs,	1},
	{"preventremoval",	(PyCFunction)CD_preventremoval,	1},
	{"readda",		(PyCFunction)CD_readda,		1},
	{"seek",		(PyCFunction)CD_seek,		1},
	{"seekblock",		(PyCFunction)CD_seekblock,		1},
	{"seektrack",		(PyCFunction)CD_seektrack,		1},
	{"stop",		(PyCFunction)CD_stop,		1},
	{"togglepause",		(PyCFunction)CD_togglepause,   	1},
	{NULL,			NULL} 		/* sentinel */
};

static void
cdplayer_dealloc(self)
	cdplayerobject *self;
{
	if (self->ob_cdplayer != NULL)
		CDclose(self->ob_cdplayer);
	PyMem_DEL(self);
}

static PyObject *
cdplayer_getattr(self, name)
	cdplayerobject *self;
	char *name;
{
	if (self->ob_cdplayer == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "no player active");
		return NULL;
	}
	return Py_FindMethod(cdplayer_methods, (PyObject *)self, name);
}

PyTypeObject CdPlayertype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"cdplayer",		/*tp_name*/
	sizeof(cdplayerobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)cdplayer_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)cdplayer_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static PyObject *
newcdplayerobject(cdp)
	CDPLAYER *cdp;
{
	cdplayerobject *p;

	p = PyObject_NEW(cdplayerobject, &CdPlayertype);
	if (p == NULL)
		return NULL;
	p->ob_cdplayer = cdp;
	return (PyObject *) p;
}

static PyObject *
CD_open(self, args)
	PyObject *self, *args;
{
	char *dev, *direction;
	CDPLAYER *cdp;

	/*
	 * Variable number of args.
	 * First defaults to "None", second defaults to "r".
	 */
	dev = NULL;
	direction = "r";
	if (!PyArg_ParseTuple(args, "|zs", &dev, &direction))
		return NULL;

	cdp = CDopen(dev, direction);
	if (cdp == NULL) {
		PyErr_SetFromErrno(CdError);
		return NULL;
	}

	return newcdplayerobject(cdp);
}

typedef struct {
	PyObject_HEAD
	CDPARSER *ob_cdparser;
	struct {
		PyObject *ob_cdcallback;
		PyObject *ob_cdcallbackarg;
	} ob_cdcallbacks[NCALLBACKS];
} cdparserobject;

static void
CD_callback(arg, type, data)
	void *arg;
	CDDATATYPES type;
	void *data;
{
	PyObject *result, *args, *v = NULL;
	char *p;
	int i;
	cdparserobject *self;

	self = (cdparserobject *) arg;
	args = PyTuple_New(3);
	if (args == NULL)
		return;
	Py_INCREF(self->ob_cdcallbacks[type].ob_cdcallbackarg);
	PyTuple_SetItem(args, 0, self->ob_cdcallbacks[type].ob_cdcallbackarg);
	PyTuple_SetItem(args, 1, PyInt_FromLong((long) type));
	switch (type) {
	case cd_audio:
		v = PyString_FromStringAndSize(data, CDDA_DATASIZE);
		break;
	case cd_pnum:
	case cd_index:
		v = PyInt_FromLong(((CDPROGNUM *) data)->value);
		break;
	case cd_ptime:
	case cd_atime:
#define ptr ((struct cdtimecode *) data)
		v = Py_BuildValue("(iii)",
			    ptr->mhi * 10 + ptr->mlo,
			    ptr->shi * 10 + ptr->slo,
			    ptr->fhi * 10 + ptr->flo);
#undef ptr
		break;
	case cd_catalog:
		v = PyString_FromStringAndSize(NULL, 13);
		p = PyString_AsString(v);
		for (i = 0; i < 13; i++)
			*p++ = ((char *) data)[i] + '0';
		break;
	case cd_ident:
#define ptr ((struct cdident *) data)
		v = PyString_FromStringAndSize(NULL, 12);
		p = PyString_AsString(v);
		CDsbtoa(p, ptr->country, 2);
		p += 2;
		CDsbtoa(p, ptr->owner, 3);
		p += 3;
		*p++ = ptr->year[0] + '0';
		*p++ = ptr->year[1] + '0';
		*p++ = ptr->serial[0] + '0';
		*p++ = ptr->serial[1] + '0';
		*p++ = ptr->serial[2] + '0';
		*p++ = ptr->serial[3] + '0';
		*p++ = ptr->serial[4] + '0';
#undef ptr
		break;
	case cd_control:
		v = PyInt_FromLong((long) *((unchar *) data));
		break;
	}
	PyTuple_SetItem(args, 2, v);
	if (PyErr_Occurred()) {
		Py_DECREF(args);
		return;
	}
	
	result = PyEval_CallObject(self->ob_cdcallbacks[type].ob_cdcallback,
				   args);
	Py_DECREF(args);
	Py_XDECREF(result);
}

static PyObject *
CD_deleteparser(self, args)
	cdparserobject *self;
	PyObject *args;
{
	int i;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	CDdeleteparser(self->ob_cdparser);
	self->ob_cdparser = NULL;

	/* no sense in keeping the callbacks, so remove them */
	for (i = 0; i < NCALLBACKS; i++) {
		Py_XDECREF(self->ob_cdcallbacks[i].ob_cdcallback);
		self->ob_cdcallbacks[i].ob_cdcallback = NULL;
		Py_XDECREF(self->ob_cdcallbacks[i].ob_cdcallbackarg);
		self->ob_cdcallbacks[i].ob_cdcallbackarg = NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CD_parseframe(self, args)
	cdparserobject *self;
	PyObject *args;
{
	char *cdfp;
	int length;
	CDFRAME *p;

	if (!PyArg_ParseTuple(args, "s#", &cdfp, &length))
		return NULL;

	if (length % sizeof(CDFRAME) != 0) {
		PyErr_SetString(PyExc_TypeError, "bad length");
		return NULL;
	}

	p = (CDFRAME *) cdfp;
	while (length > 0) {
		CDparseframe(self->ob_cdparser, p);
		length -= sizeof(CDFRAME);
		p++;
		if (PyErr_Occurred())
			return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CD_removecallback(self, args)
	cdparserobject *self;
	PyObject *args;
{
	int type;

	if (!PyArg_ParseTuple(args, "i", &type))
		return NULL;

	if (type < 0 || type >= NCALLBACKS) {
		PyErr_SetString(PyExc_TypeError, "bad type");
		return NULL;
	}

	CDremovecallback(self->ob_cdparser, (CDDATATYPES) type);

	Py_XDECREF(self->ob_cdcallbacks[type].ob_cdcallback);
	self->ob_cdcallbacks[type].ob_cdcallback = NULL;

	Py_XDECREF(self->ob_cdcallbacks[type].ob_cdcallbackarg);
	self->ob_cdcallbacks[type].ob_cdcallbackarg = NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CD_resetparser(self, args)
	cdparserobject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	CDresetparser(self->ob_cdparser);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CD_addcallback(self, args)
	cdparserobject *self;
	PyObject *args;
{
	int type;
	PyObject *func, *funcarg;

	/* XXX - more work here */
	if (!PyArg_ParseTuple(args, "iOO", &type, &func, &funcarg))
		return NULL;

	if (type < 0 || type >= NCALLBACKS) {
		PyErr_SetString(PyExc_TypeError, "argument out of range");
		return NULL;
	}

#ifdef CDsetcallback
	CDaddcallback(self->ob_cdparser, (CDDATATYPES) type, CD_callback,
		      (void *) self);
#else
	CDsetcallback(self->ob_cdparser, (CDDATATYPES) type, CD_callback,
		      (void *) self);
#endif
	Py_XDECREF(self->ob_cdcallbacks[type].ob_cdcallback);
	Py_INCREF(func);
	self->ob_cdcallbacks[type].ob_cdcallback = func;
	Py_XDECREF(self->ob_cdcallbacks[type].ob_cdcallbackarg);
	Py_INCREF(funcarg);
	self->ob_cdcallbacks[type].ob_cdcallbackarg = funcarg;

/*
	if (type == cd_audio) {
		sigfpe_[_UNDERFL].repls = _ZERO;
		handle_sigfpes(_ON, _EN_UNDERFL, NULL,
		                        _ABORT_ON_ERROR, NULL);
	}
*/

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef cdparser_methods[] = {
	{"addcallback",		(PyCFunction)CD_addcallback,   	1},
	{"deleteparser",	(PyCFunction)CD_deleteparser,	1},
	{"parseframe",		(PyCFunction)CD_parseframe,	1},
	{"removecallback",	(PyCFunction)CD_removecallback,	1},
	{"resetparser",		(PyCFunction)CD_resetparser,	1},
		                                /* backward compatibility */
	{"setcallback",		(PyCFunction)CD_addcallback,   	1},
	{NULL,			NULL} 		/* sentinel */
};

static void
cdparser_dealloc(self)
	cdparserobject *self;
{
	int i;

	for (i = 0; i < NCALLBACKS; i++) {
		Py_XDECREF(self->ob_cdcallbacks[i].ob_cdcallback);
		self->ob_cdcallbacks[i].ob_cdcallback = NULL;
		Py_XDECREF(self->ob_cdcallbacks[i].ob_cdcallbackarg);
		self->ob_cdcallbacks[i].ob_cdcallbackarg = NULL;
	}
	CDdeleteparser(self->ob_cdparser);
	PyMem_DEL(self);
}

static PyObject *
cdparser_getattr(self, name)
	cdparserobject *self;
	char *name;
{
	if (self->ob_cdparser == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "no parser active");
		return NULL;
	}

	return Py_FindMethod(cdparser_methods, (PyObject *)self, name);
}

PyTypeObject CdParsertype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"cdparser",		/*tp_name*/
	sizeof(cdparserobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)cdparser_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)cdparser_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

static PyObject *
newcdparserobject(cdp)
	CDPARSER *cdp;
{
	cdparserobject *p;
	int i;

	p = PyObject_NEW(cdparserobject, &CdParsertype);
	if (p == NULL)
		return NULL;
	p->ob_cdparser = cdp;
	for (i = 0; i < NCALLBACKS; i++) {
		p->ob_cdcallbacks[i].ob_cdcallback = NULL;
		p->ob_cdcallbacks[i].ob_cdcallbackarg = NULL;
	}
	return (PyObject *) p;
}

static PyObject *
CD_createparser(self, args)
	PyObject *self, *args;
{
	CDPARSER *cdp;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	cdp = CDcreateparser();
	if (cdp == NULL) {
		PyErr_SetString(CdError, "createparser failed");
		return NULL;
	}

	return newcdparserobject(cdp);
}

static PyObject *
CD_msftoframe(self, args)
	PyObject *self, *args;
{
	int min, sec, frame;

	if (!PyArg_ParseTuple(args, "iii", &min, &sec, &frame))
		return NULL;

	return PyInt_FromLong((long) CDmsftoframe(min, sec, frame));
}
	
static PyMethodDef CD_methods[] = {
	{"open",		(PyCFunction)CD_open,		1},
	{"createparser",	(PyCFunction)CD_createparser,	1},
	{"msftoframe",		(PyCFunction)CD_msftoframe,	1},
	{NULL,		NULL}	/* Sentinel */
};

void
initcd()
{
	PyObject *m, *d;

	m = Py_InitModule("cd", CD_methods);
	d = PyModule_GetDict(m);

	CdError = PyErr_NewException("cd.error", NULL, NULL);
	PyDict_SetItemString(d, "error", CdError);

	/* Identifiers for the different types of callbacks from the parser */
	PyDict_SetItemString(d, "audio", PyInt_FromLong((long) cd_audio));
	PyDict_SetItemString(d, "pnum", PyInt_FromLong((long) cd_pnum));
	PyDict_SetItemString(d, "index", PyInt_FromLong((long) cd_index));
	PyDict_SetItemString(d, "ptime", PyInt_FromLong((long) cd_ptime));
	PyDict_SetItemString(d, "atime", PyInt_FromLong((long) cd_atime));
	PyDict_SetItemString(d, "catalog", PyInt_FromLong((long) cd_catalog));
	PyDict_SetItemString(d, "ident", PyInt_FromLong((long) cd_ident));
	PyDict_SetItemString(d, "control", PyInt_FromLong((long) cd_control));

	/* Block size information for digital audio data */
	PyDict_SetItemString(d, "DATASIZE",
			   PyInt_FromLong((long) CDDA_DATASIZE));
	PyDict_SetItemString(d, "BLOCKSIZE",
			   PyInt_FromLong((long) CDDA_BLOCKSIZE));

	/* Possible states for the cd player */
	PyDict_SetItemString(d, "ERROR", PyInt_FromLong((long) CD_ERROR));
	PyDict_SetItemString(d, "NODISC", PyInt_FromLong((long) CD_NODISC));
	PyDict_SetItemString(d, "READY", PyInt_FromLong((long) CD_READY));
	PyDict_SetItemString(d, "PLAYING", PyInt_FromLong((long) CD_PLAYING));
	PyDict_SetItemString(d, "PAUSED", PyInt_FromLong((long) CD_PAUSED));
	PyDict_SetItemString(d, "STILL", PyInt_FromLong((long) CD_STILL));
#ifdef CD_CDROM			/* only newer versions of the library */
	PyDict_SetItemString(d, "CDROM", PyInt_FromLong((long) CD_CDROM));
#endif

	if (PyErr_Occurred())
		Py_FatalError("can't initialize module cd");
}
