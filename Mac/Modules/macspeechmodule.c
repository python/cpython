/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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

#include <Gestalt.h>
#include "Speech.h"

#ifdef __MWERKS__
#include <TextUtils.h>
#define c2pstr C2PStr
#define p2cstr P2CStr
#else
#include "pascal.h"
#endif /* __MWERKS__ */

#ifdef __powerc
#include <CodeFragments.h>
int lib_available;
#endif /* __powerc */

/* Somehow the Apple Fix2X and X2Fix don't do what I expect */
#define fixed2double(x) (((double)(x))/32768.0)
#define double2fixed(x) ((Fixed)((x)*32768.0))

char *CurrentSpeech;
PyObject *ms_error_object;
int speech_available;

static
init_available() {
	OSErr err;
	long result;

#ifdef __powerc
	lib_available = ((ProcPtr)SpeakString != (ProcPtr)0);
#endif
	err = Gestalt(gestaltSpeechAttr, &result);
	if ( err == noErr && (result & (1<<gestaltSpeechMgrPresent)))
		return 1;
	return 0;
}

static
check_available() {
	if ( !speech_available ) {
		PyErr_SetString(ms_error_object, "Speech Mgr not available");
		return 0;
	}
#ifdef __powerc
	if ( !lib_available ) {
		PyErr_SetString(ms_error_object, "Speech Mgr available, but shared lib missing");
		return 0;
	}
#endif
	return 1;
}

/* -------------
**
** Part one - the speech channel object
*/
typedef struct {
	PyObject_HEAD
	SpeechChannel chan;
	PyObject *curtext;	/* If non-NULL current text being spoken */
} scobject;

staticforward PyTypeObject sctype;

#define is_scobject(v)		((v)->ob_type == &sctype)

static scobject *
newscobject(arg)
	VoiceSpec *arg;
{
	scobject *self;
	OSErr err;
	
	self = PyObject_NEW(scobject, &sctype);
	if (self == NULL)
		return NULL;
	if ( (err=NewSpeechChannel(arg, &self->chan)) != 0) {
		Py_DECREF(self);
		return (scobject *)PyErr_Mac(ms_error_object, err);
	}
	self->curtext = NULL;
	return self;
}

/* sc methods */

static void
sc_dealloc(self)
	scobject *self;
{
	DisposeSpeechChannel(self->chan);
	PyMem_DEL(self);
}

static PyObject *
sc_Stop(self, args)
	scobject *self;
	PyObject *args;
{
	OSErr err;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if ((err=StopSpeech(self->chan)) != 0) {
		PyErr_Mac(ms_error_object, err);
		return NULL;
	}
	if ( self->curtext ) {
		Py_DECREF(self->curtext);
		self->curtext = NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sc_SpeakText(self, args)
	scobject *self;
	PyObject *args;
{
	OSErr err;
	char *str;
	int len;
	
	if (!PyArg_Parse(args, "s#", &str, &len))
		return NULL;
	if ( self->curtext ) {
		StopSpeech(self->chan);
		Py_DECREF(self->curtext);
		self->curtext = NULL;
	}
	if ((err=SpeakText(self->chan, (Ptr)str, (long)len)) != 0) {
		PyErr_Mac(ms_error_object, err);
		return 0;
	}
	(void)PyArg_Parse(args, "O", &self->curtext);	/* Or should I check this? */
	Py_INCREF(self->curtext);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sc_GetRate(self, args)
	scobject *self;
	PyObject *args;
{
	OSErr err;
	Fixed farg;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if ((err=GetSpeechRate(self->chan, &farg)) != 0) {
		PyErr_Mac(ms_error_object, err);
		return 0;
	}
	return PyFloat_FromDouble(fixed2double(farg));
}

static PyObject *
sc_GetPitch(self, args)
	scobject *self;
	PyObject *args;
{
	OSErr err;
	Fixed farg;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if ((err=GetSpeechPitch(self->chan, &farg)) != 0) {
		PyErr_Mac(ms_error_object, err);
		return 0;
	}
	return PyFloat_FromDouble(fixed2double(farg));
}

static PyObject *
sc_SetRate(self, args)
	scobject *self;
	PyObject *args;
{
	OSErr err;
	double darg;
	
	if (!PyArg_Parse(args, "d", &darg))
		return NULL;
	if ((err=SetSpeechRate(self->chan, double2fixed(darg))) != 0) {
		PyErr_Mac(ms_error_object, err);
		return 0;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
sc_SetPitch(self, args)
	scobject *self;
	PyObject *args;
{
	OSErr err;
	double darg;
	
	if (!PyArg_Parse(args, "d", &darg))
		return NULL;
	if ((err=SetSpeechPitch(self->chan, double2fixed(darg))) != 0) {
		PyErr_Mac(ms_error_object, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef sc_methods[] = {
	{"Stop",		(PyCFunction)sc_Stop},
	{"SetRate",		(PyCFunction)sc_SetRate},
	{"GetRate",		(PyCFunction)sc_GetRate},
	{"SetPitch",	(PyCFunction)sc_SetPitch},
	{"GetPitch",	(PyCFunction)sc_GetPitch},
	{"SpeakText",	(PyCFunction)sc_SpeakText},
	{NULL,			NULL}		/* sentinel */
};

static PyObject *
sc_getattr(self, name)
	scobject *self;
	char *name;
{
	return Py_FindMethod(sc_methods, (PyObject *)self, name);
}

static PyTypeObject sctype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"MacSpeechChannel",			/*tp_name*/
	sizeof(scobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)sc_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)sc_getattr, /*tp_getattr*/
	0, 			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

/* -------------
**
** Part two - the voice object
*/
typedef struct {
	PyObject_HEAD
	int		initialized;
	VoiceSpec	vs;
	VoiceDescription vd;
} mvobject;

staticforward PyTypeObject mvtype;

#define is_mvobject(v)		((v)->ob_type == &mvtype)

static mvobject *
newmvobject()
{
	mvobject *self;
	self = PyObject_NEW(mvobject, &mvtype);
	if (self == NULL)
		return NULL;
	self->initialized = 0;
	return self;
}

static int
initmvobject(self, ind)
	mvobject *self;
	int ind;
{
	OSErr err;
	
	if ( (err=GetIndVoice((short)ind, &self->vs)) != 0 ) {
		PyErr_Mac(ms_error_object, err);
		return 0;
	}
	if ( (err=GetVoiceDescription(&self->vs, &self->vd, sizeof self->vd)) != 0) {
		PyErr_Mac(ms_error_object, err);
		return 0;
	}
	self->initialized = 1;
	return 1;
} 
/* mv methods */

static void
mv_dealloc(self)
	mvobject *self;
{
	PyMem_DEL(self);
}

static PyObject *
mv_getgender(self, args)
	mvobject *self;
	PyObject *args;
{
	PyObject *rv;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if (!self->initialized) {
		PyErr_SetString(ms_error_object, "Uninitialized voice");
		return NULL;
	}
	rv = PyInt_FromLong(self->vd.gender);
	return rv;
}

static PyObject *
mv_newchannel(self, args)
	mvobject *self;
	PyObject *args;
{	
	if (!PyArg_NoArgs(args))
		return NULL;
	if (!self->initialized) {
		PyErr_SetString(ms_error_object, "Uninitialized voice");
		return NULL;
	}
	return (PyObject *)newscobject(&self->vs);
}

static struct PyMethodDef mv_methods[] = {
	{"GetGender",	(PyCFunction)mv_getgender},
	{"NewChannel",	(PyCFunction)mv_newchannel},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
mv_getattr(self, name)
	mvobject *self;
	char *name;
{
	return Py_FindMethod(mv_methods, (PyObject *)self, name);
}

static PyTypeObject mvtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"MacVoice",			/*tp_name*/
	sizeof(mvobject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)mv_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)mv_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};


/* -------------
**
** Part three - The module interface
*/

/* See if Speech manager available */

static PyObject *
ms_Available(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong(speech_available);
}

/* Count number of busy speeches */

static PyObject *
ms_Busy(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	short result;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if ( !check_available() )
		return NULL;
	result = SpeechBusy();
	return PyInt_FromLong(result);
}

/* Say something */

static PyObject *
ms_SpeakString(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	OSErr err;
	char *str;
	int len;
	
	if (!PyArg_Parse(args, "s", &str))
		return NULL;
	if ( !check_available())
		return NULL;
	if (CurrentSpeech) {
		/* Free the old speech, after killing it off
		** (note that speach is async and c2pstr works inplace)
		*/
		SpeakString("\p");
		free(CurrentSpeech);
	}
	len = strlen(str);
	CurrentSpeech = malloc(len+1);
	strcpy(CurrentSpeech, str);
	err = SpeakString(c2pstr(CurrentSpeech));
	if ( err ) {
		PyErr_Mac(ms_error_object, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


/* Count number of available voices */

static PyObject *
ms_CountVoices(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	short result;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if ( !check_available())
		return NULL;
	CountVoices(&result);
	return PyInt_FromLong(result);
}

static PyObject *
ms_GetIndVoice(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	mvobject *rv;
	long ind;
	
	if( !PyArg_Parse(args, "i", &ind))
		return NULL;
	if ( !check_available() )
		return NULL;
	rv = newmvobject();
	if ( !initmvobject(rv, ind) ) {
		Py_DECREF(rv);
		return NULL;
	}
	return (PyObject *)rv;
}


static PyObject *
ms_Version(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	NumVersion v;
	
	if (!PyArg_NoArgs(args))
		return NULL;
	if ( !check_available())
		return NULL;
	v = SpeechManagerVersion();
	return PyInt_FromLong(*(int *)&v);
}


/* List of functions defined in the module */

static struct PyMethodDef ms_methods[] = {
	{"Available",	ms_Available},
	{"CountVoices",	ms_CountVoices},
	{"Busy",		ms_Busy},
	{"SpeakString",	ms_SpeakString},
	{"GetIndVoice", ms_GetIndVoice},
	{"Version",		ms_Version},
	{NULL,		NULL}		/* sentinel */
};

/* Initialization function for the module (*must* be called initmacspeech) */

void
initmacspeech()
{
	PyObject *m, *d;

	speech_available = init_available();
	/* Create the module and add the functions */
	m = Py_InitModule("macspeech", ms_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ms_error_object = PyString_FromString("macspeech.error");
	PyDict_SetItemString(d, "error", ms_error_object);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module macspeech");
}
