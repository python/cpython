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

/* Macintosh OS-specific interface */

#include "Python.h"
#include "macglue.h"

#include <Windows.h>
#include <Files.h>
#include <LowMem.h>

static PyObject *MacOS_Error; /* Exception MacOS.Error */

#ifdef MPW
#define bufferIsSmall -607	/*error returns from Post and Accept */
#endif

static PyObject *ErrorObject;

/* ----------------------------------------------------- */

/* Declarations for objects of type Resource fork */

typedef struct {
	PyObject_HEAD
	short fRefNum;
	int isclosed;
} rfobject;

staticforward PyTypeObject Rftype;



/* ---------------------------------------------------------------- */

static void
do_close(self)
	rfobject *self;
{
	if (self->isclosed ) return;
	(void)FSClose(self->fRefNum);
	self->isclosed = 1;
}

static char rf_read__doc__[] = 
"Read data from resource fork"
;

static PyObject *
rf_read(self, args)
	rfobject *self;
	PyObject *args;
{
	long n;
	PyObject *v;
	OSErr err;
	
	if (self->isclosed) {
		PyErr_SetString(PyExc_ValueError, "Operation on closed file");
		return NULL;
	}
	
	if (!PyArg_ParseTuple(args, "l", &n))
		return NULL;
		
	v = PyString_FromStringAndSize((char *)NULL, n);
	if (v == NULL)
		return NULL;
		
	err = FSRead(self->fRefNum, &n, PyString_AsString(v));
	if (err && err != eofErr) {
		PyMac_Error(err);
		Py_DECREF(v);
		return NULL;
	}
	_PyString_Resize(&v, n);
	return v;
}


static char rf_write__doc__[] = 
"Write to resource fork"
;

static PyObject *
rf_write(self, args)
	rfobject *self;
	PyObject *args;
{
	char *buffer;
	long size;
	OSErr err;
	
	if (self->isclosed) {
		PyErr_SetString(PyExc_ValueError, "Operation on closed file");
		return NULL;
	}
	if (!PyArg_ParseTuple(args, "s#", &buffer, &size))
		return NULL;
	err = FSWrite(self->fRefNum, &size, buffer);
	if (err) {
		PyMac_Error(err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static char rf_seek__doc__[] = 
"Set file position"
;

static PyObject *
rf_seek(self, args)
	rfobject *self;
	PyObject *args;
{
	long amount, pos;
	int whence = SEEK_SET;
	long eof;
	OSErr err;
	
	if (self->isclosed) {
		PyErr_SetString(PyExc_ValueError, "Operation on closed file");
		return NULL;
	}
	if (!PyArg_ParseTuple(args, "l|i", &amount, &whence))
		return NULL;
	
	if ( err = GetEOF(self->fRefNum, &eof))
		goto ioerr;
	
	switch (whence) {
	case SEEK_CUR:
		if (err = GetFPos(self->fRefNum, &pos))
			goto ioerr; 
		break;
	case SEEK_END:
		pos = eof;
		break;
	case SEEK_SET:
		pos = 0;
		break;
	default:
		PyErr_BadArgument();
		return NULL;
	}
	
	pos += amount;
	
	/* Don't bother implementing seek past EOF */
	if (pos > eof || pos < 0) {
		PyErr_BadArgument();
		return NULL;
	}
	
	if ( err = SetFPos(self->fRefNum, fsFromStart, pos) ) {
ioerr:
		PyMac_Error(err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static char rf_tell__doc__[] = 
"Get file position"
;

static PyObject *
rf_tell(self, args)
	rfobject *self;
	PyObject *args;
{
	long where;
	OSErr err;
	
	if (self->isclosed) {
		PyErr_SetString(PyExc_ValueError, "Operation on closed file");
		return NULL;
	}
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ( err = GetFPos(self->fRefNum, &where) ) {
		PyMac_Error(err);
		return NULL;
	}
	return PyInt_FromLong(where);
}

static char rf_close__doc__[] = 
"Close resource fork"
;

static PyObject *
rf_close(self, args)
	rfobject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	do_close(self);
	Py_INCREF(Py_None);
	return Py_None;
}


static struct PyMethodDef rf_methods[] = {
	{"read",	rf_read,	1,	rf_read__doc__},
 {"write",	rf_write,	1,	rf_write__doc__},
 {"seek",	rf_seek,	1,	rf_seek__doc__},
 {"tell",	rf_tell,	1,	rf_tell__doc__},
 {"close",	rf_close,	1,	rf_close__doc__},
 
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */


static rfobject *
newrfobject()
{
	rfobject *self;
	
	self = PyObject_NEW(rfobject, &Rftype);
	if (self == NULL)
		return NULL;
	self->isclosed = 1;
	return self;
}


static void
rf_dealloc(self)
	rfobject *self;
{
	do_close(self);
	PyMem_DEL(self);
}

static PyObject *
rf_getattr(self, name)
	rfobject *self;
	char *name;
{
	return Py_FindMethod(rf_methods, (PyObject *)self, name);
}

static char Rftype__doc__[] = 
"Resource fork file object"
;

static PyTypeObject Rftype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"Resource fork",			/*tp_name*/
	sizeof(rfobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)rf_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)rf_getattr,	/*tp_getattr*/
	(setattrfunc)0,	/*tp_setattr*/
	(cmpfunc)0,		/*tp_compare*/
	(reprfunc)0,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
	(ternaryfunc)0,		/*tp_call*/
	(reprfunc)0,		/*tp_str*/

	/* Space for future expansion */
	0L,0L,0L,0L,
	Rftype__doc__ /* Documentation string */
};

/* End of code for Resource fork objects */
/* -------------------------------------------------------- */

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
	int old, new;
	
	if (!PyArg_ParseTuple(args, "i", &new))
		return NULL;
	old = PyMac_DoYieldEnabled;
	PyMac_DoYieldEnabled = new;
	return Py_BuildValue("i", old);
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

static PyObject *
MacOS_GetErrorString(PyObject *self, PyObject *args)
{
	int errn;
	
	if (!PyArg_ParseTuple(args, "i", &errn))
		return NULL;
	return Py_BuildValue("s", PyMac_StrError(errn));
}

static char splash_doc[] = "Open a splash-screen dialog by resource-id (0=close)";

static PyObject *
MacOS_splash(PyObject *self, PyObject *args)
{
	int resid = -1;
	static DialogPtr curdialog = NULL;
	WindowRef theWindow;
	CGrafPtr thePort;
	short xpos, ypos, width, height, swidth, sheight;
	
	if (!PyArg_ParseTuple(args, "|i", &resid))
		return NULL;
	if (curdialog) {
		DisposeDialog(curdialog);
		curdialog = NULL;
	}
		
	if ( resid != -1 ) {
		curdialog = GetNewDialog(resid, NULL, (WindowPtr)-1);
		if ( curdialog ) {
			theWindow = GetDialogWindow(curdialog);
			thePort = GetWindowPort(theWindow);
			width = thePort->portRect.right - thePort->portRect.left;
			height = thePort->portRect.bottom - thePort->portRect.top;
			swidth = qd.screenBits.bounds.right - qd.screenBits.bounds.left;
			sheight = qd.screenBits.bounds.bottom - qd.screenBits.bounds.top - LMGetMBarHeight();
			xpos = (swidth-width)/2;
			ypos = (sheight-height)/5 + LMGetMBarHeight();
			MoveWindow(theWindow, xpos, ypos, 0);
			ShowWindow(theWindow);
			DrawDialog(curdialog);
		}
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char DebugStr_doc[] = "Switch to low-level debugger with a message";

static PyObject *
MacOS_DebugStr(PyObject *self, PyObject *args)
{
	Str255 message;
	PyObject *object = 0;
	
	if (!PyArg_ParseTuple(args, "O&|O", PyMac_GetStr255, message, &object))
		return NULL;
	DebugStr(message);
	Py_INCREF(Py_None);
	return Py_None;
}

static char openrf_doc[] = "Open resource fork of a file";

static PyObject *
MacOS_openrf(PyObject *self, PyObject *args)
{
	OSErr err;
	char *mode = "r";
	FSSpec fss;
	SignedByte permission = 1;
	rfobject *fp;
		
	if (!PyArg_ParseTuple(args, "O&|s", PyMac_GetFSSpec, &fss, &mode))
		return NULL;
	while (*mode) {
		switch (*mode++) {
		case '*': break;
		case 'r': permission = 1; break;
		case 'w': permission = 2; break;
		case 'b': break;
		default:
			PyErr_BadArgument();
			return NULL;
		}
	}
	
	if ( (fp = newrfobject()) == NULL )
		return NULL;
		
	err = HOpenRF(fss.vRefNum, fss.parID, fss.name, permission, &fp->fRefNum);
	
	if ( err == fnfErr ) {
		/* In stead of doing complicated things here to get creator/type
		** correct we let the standard i/o library handle it
		*/
		FILE *tfp;
		char pathname[257];
		
		if ( err=PyMac_GetFullPath(&fss, pathname) ) {
			PyMac_Error(err);
			Py_DECREF(fp);
			return NULL;
		}
		
		if ( (tfp = fopen(pathname, "w")) == NULL ) {
			PyMac_Error(fnfErr); /* What else... */
			Py_DECREF(fp);
			return NULL;
		}
		fclose(tfp);
		err = HOpenRF(fss.vRefNum, fss.parID, fss.name, permission, &fp->fRefNum);
	}
	if ( err ) {
		Py_DECREF(fp);
		PyMac_Error(err);
		return NULL;
	}
	fp->isclosed = 0;
	return (PyObject *)fp;
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
	{"GetErrorString",		MacOS_GetErrorString, 1},
	{"openrf",				MacOS_openrf, 1, 	openrf_doc},
	{"splash",				MacOS_splash, 1, 	splash_doc},
	{"DebugStr",			MacOS_DebugStr,	1,	DebugStr_doc},
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
	/*
	** This is a hack: the following constant added to the id() of a string
	** object gives you the address of the data. Unfortunately, it is needed for
	** some of the image and sound processing interfaces on the mac:-(
	*/
	{
		PyStringObject *p = 0;
		long off = (long)&(p->ob_sval[0]);
		
		if( PyDict_SetItemString(d, "string_id_to_buffer", Py_BuildValue("i", off)) != 0)
			Py_FatalError("Can't define MacOS.string_id_to_buffer");
	}
}

