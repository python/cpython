/***********************************************************
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

#include "Python.h"
#include "macglue.h"
#include <Navigation.h>

/* Exported by AEModule.c: */
extern PyObject *AEDesc_New(AppleEvent *);
extern int AEDesc_Convert(PyObject *, AppleEvent *);
/* Exported by Resmodule.c */
extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);

static PyObject *ErrorObject;

/* ----------------------------------------------------- */
static int
filldialogoptions(PyObject *d,
		NavDialogOptions *opt,
		NavEventUPP *eventProcP,
		NavPreviewUPP *previewProcP,
		NavObjectFilterUPP *filterProcP,
		NavTypeListHandle *typeListP,
		OSType *creatorP,
		OSType *typeP)
{
	int pos = 0;
	PyObject *key, *value;
	char *keystr;
	
	NavGetDefaultDialogOptions(opt);
	if ( eventProcP ) *eventProcP = NULL;
	if ( previewProcP ) *previewProcP = NULL;
	if ( filterProcP ) *filterProcP = NULL;
	if ( typeListP ) *typeListP = NULL;
	if ( creatorP ) *creatorP = 0;
	if ( typeP ) *typeP = 0;

	while ( PyDict_Next(d, &pos, &key, &value) ) {
		if ( !key || !value || !PyString_Check(key) ) {
			PyErr_SetString(ErrorObject, "DialogOption has non-string key");
			return 0;
		}
		keystr = PyString_AsString(key);
		if( strcmp(keystr, "version") == 0 ) {
			if ( !PyArg_Parse(value, "h", &opt->version) )
				return 0;
		} else if( strcmp(keystr, "dialogOptionFlags") == 0 ) {
			if ( !PyArg_Parse(value, "l", &opt->dialogOptionFlags) )
				return 0;
		} else if( strcmp(keystr, "location") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetPoint, &opt->location) )
				return 0;
		} else if( strcmp(keystr, "clientName") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetStr255, &opt->clientName) )
				return 0;
		} else if( strcmp(keystr, "windowTitle") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetStr255, &opt->windowTitle) )
				return 0;
		} else if( strcmp(keystr, "actionButtonLabel") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetStr255, &opt->actionButtonLabel) )
				return 0;
		} else if( strcmp(keystr, "cancelButtonLabel") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetStr255, &opt->cancelButtonLabel) )
				return 0;
		} else if( strcmp(keystr, "savedFileName") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetStr255, &opt->savedFileName) )
				return 0;
		} else if( strcmp(keystr, "message") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetStr255, &opt->message) )
				return 0;
		} else if( strcmp(keystr, "preferenceKey") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetOSType, &opt->preferenceKey) )
				return 0;
		} else if( strcmp(keystr, "popupExtension") == 0 ) {
			if ( !PyArg_Parse(value, "O&", ResObj_Convert, &opt->popupExtension) )
				return 0;
		} else if( eventProcP && strcmp(keystr, "eventProc") == 0 ) {
			PyErr_SetString(ErrorObject, "No callbacks implemented yet");
			return 0;
		} else if( previewProcP && strcmp(keystr, "previewProc") == 0 ) {
			PyErr_SetString(ErrorObject, "No callbacks implemented yet");
			return 0;
		} else if( filterProcP && strcmp(keystr, "filterProc") == 0 ) {
			PyErr_SetString(ErrorObject, "No callbacks implemented yet");
			return 0;
		} else if( typeListP && strcmp(keystr, "typeList") == 0 ) {
			if ( !PyArg_Parse(value, "O&", ResObj_Convert, typeListP) )
				return 0;
		} else if( creatorP && strcmp(keystr, "creator") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetOSType, creatorP) )
				return 0;
		} else if( typeP && strcmp(keystr, "type") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetOSType, typeP) )
				return 0;
		} else {
			PyErr_Format(ErrorObject, "Unknown DialogOption key: %s", keystr);
			return 0;
		}
	}
	return 1;
}

/* ----------------------------------------------------- */

/* Declarations for objects of type NavReplyRecord */

typedef struct {
	PyObject_HEAD
	NavReplyRecord itself;
} navrrobject;

staticforward PyTypeObject Navrrtype;



/* ---------------------------------------------------------------- */

static struct PyMethodDef navrr_methods[] = {
	
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */


static navrrobject *
newnavrrobject(NavReplyRecord *itself)
{
	navrrobject *self;
	
	self = PyObject_NEW(navrrobject, &Navrrtype);
	if (self == NULL)
		return NULL;
	self->itself = *itself;
	return self;
}


static void
navrr_dealloc(self)
	navrrobject *self;
{
	NavDisposeReply(&self->itself);
	PyMem_DEL(self);
}

static PyObject *
navrr_getattr(self, name)
	navrrobject *self;
	char *name;
{
	if( strcmp(name, "version") == 0 )
		return Py_BuildValue("h", self->itself.version);
	if( strcmp(name, "validRecord") == 0 )
		return Py_BuildValue("l", (long)self->itself.validRecord);
	if( strcmp(name, "replacing") == 0 )
		return Py_BuildValue("l", (long)self->itself.replacing);
	if( strcmp(name, "isStationery") == 0 )
		return Py_BuildValue("l", (long)self->itself.isStationery);
	if( strcmp(name, "translationNeeded") == 0 )
		return Py_BuildValue("l", (long)self->itself.translationNeeded);
	if( strcmp(name, "selection") == 0 )
		return AEDesc_New(&self->itself.selection);	/* XXXX Is this ok? */
	if( strcmp(name, "fileTranslation") == 0 )
		return ResObj_New((Handle)self->itself.fileTranslation);


	return Py_FindMethod(navrr_methods, (PyObject *)self, name);
}

static int
navrr_setattr(self, name, v)
	navrrobject *self;
	char *name;
	PyObject *v;
{
	/* Set attribute 'name' to value 'v'. v==NULL means delete */
	
	/* XXXX Add your own setattr code here */
	return -1;
}

static char Navrrtype__doc__[] = 
""
;

static PyTypeObject Navrrtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"NavReplyRecord",			/*tp_name*/
	sizeof(navrrobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)navrr_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)navrr_getattr,	/*tp_getattr*/
	(setattrfunc)navrr_setattr,	/*tp_setattr*/
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
	Navrrtype__doc__ /* Documentation string */
};

/* End of code for NavReplyRecord objects */
		
/* ----------------------------------------------------- */

static char nav_NavGetFile__doc__[] =
""
;

static PyObject *
nav_NavGetFile(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{
	PyObject *dict;
	AEDesc	*defaultLocation = NULL;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NULL;
	NavPreviewUPP previewProc = NULL;
	NavObjectFilterUPP filterProc = NULL;
	NavTypeListHandle typeList = NULL;
	OSErr err;

	if ( kw ) {
		if (!PyArg_ParseTuple(args, ""))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &dialogOptions, &eventProc, &previewProc, &filterProc, &typeList, NULL, NULL))
		return NULL;
	err = NavGetFile(defaultLocation, &reply, &dialogOptions,
			eventProc, previewProc, filterProc, typeList, (void *)dict);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

static char nav_NavPutFile__doc__[] =
""
;

static PyObject *
nav_NavPutFile(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavAskSaveChanges__doc__[] =
""
;

static PyObject *
nav_NavAskSaveChanges(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavCustomAskSaveChanges__doc__[] =
""
;

static PyObject *
nav_NavCustomAskSaveChanges(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavAskDiscardChanges__doc__[] =
""
;

static PyObject *
nav_NavAskDiscardChanges(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavChooseFile__doc__[] =
""
;

static PyObject *
nav_NavChooseFile(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavChooseFolder__doc__[] =
""
;

static PyObject *
nav_NavChooseFolder(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavChooseVolume__doc__[] =
""
;

static PyObject *
nav_NavChooseVolume(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavChooseObject__doc__[] =
""
;

static PyObject *
nav_NavChooseObject(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavNewFolder__doc__[] =
""
;

static PyObject *
nav_NavNewFolder(self, args, kw)
	PyObject *self;	/* Not used */
	PyObject *args;
	PyObject *kw;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavTranslateFile__doc__[] =
""
;

static PyObject *
nav_NavTranslateFile(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavCompleteSave__doc__[] =
""
;

static PyObject *
nav_NavCompleteSave(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavCustomControl__doc__[] =
""
;

static PyObject *
nav_NavCustomControl(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavServicesCanRun__doc__[] =
""
;

static PyObject *
nav_NavServicesCanRun(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	Boolean rv;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	rv = NavServicesCanRun();
	return Py_BuildValue("l", (long)rv);
}

static char nav_NavServicesAvailable__doc__[] =
""
;

static PyObject *
nav_NavServicesAvailable(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	Boolean rv;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	rv = NavServicesAvailable();
	return Py_BuildValue("l", (long)rv);
}
/* XX */
static char nav_NavLoad__doc__[] =
""
;

static PyObject *
nav_NavLoad(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	NavLoad();
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavUnload__doc__[] =
""
;

static PyObject *
nav_NavUnload(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	NavUnload();
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavLibraryVersion__doc__[] =
""
;

static PyObject *
nav_NavLibraryVersion(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	UInt32 rv;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	rv = NavLibraryVersion();
	return Py_BuildValue("l", (long)rv);
}

#ifdef notyet
static char nav_NavGetDefaultDialogOptions__doc__[] =
""
;

static PyObject *
nav_NavGetDefaultDialogOptions(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}
#endif


/* List of methods defined in the module */

static struct PyMethodDef nav_methods[] = {
	{"NavGetFile",	(PyCFunction)nav_NavGetFile,	METH_VARARGS|METH_KEYWORDS,	nav_NavGetFile__doc__},
 {"NavPutFile",	(PyCFunction)nav_NavPutFile,	METH_VARARGS|METH_KEYWORDS,	nav_NavPutFile__doc__},
 {"NavAskSaveChanges",	(PyCFunction)nav_NavAskSaveChanges,	METH_VARARGS|METH_KEYWORDS,	nav_NavAskSaveChanges__doc__},
 {"NavCustomAskSaveChanges",	(PyCFunction)nav_NavCustomAskSaveChanges,	METH_VARARGS|METH_KEYWORDS,	nav_NavCustomAskSaveChanges__doc__},
 {"NavAskDiscardChanges",	(PyCFunction)nav_NavAskDiscardChanges,	METH_VARARGS|METH_KEYWORDS,	nav_NavAskDiscardChanges__doc__},
 {"NavChooseFile",	(PyCFunction)nav_NavChooseFile,	METH_VARARGS|METH_KEYWORDS,	nav_NavChooseFile__doc__},
 {"NavChooseFolder",	(PyCFunction)nav_NavChooseFolder,	METH_VARARGS|METH_KEYWORDS,	nav_NavChooseFolder__doc__},
 {"NavChooseVolume",	(PyCFunction)nav_NavChooseVolume,	METH_VARARGS|METH_KEYWORDS,	nav_NavChooseVolume__doc__},
 {"NavChooseObject",	(PyCFunction)nav_NavChooseObject,	METH_VARARGS|METH_KEYWORDS,	nav_NavChooseObject__doc__},
 {"NavNewFolder",	(PyCFunction)nav_NavNewFolder,	METH_VARARGS|METH_KEYWORDS,	nav_NavNewFolder__doc__},
 {"NavTranslateFile",	(PyCFunction)nav_NavTranslateFile,	METH_VARARGS,	nav_NavTranslateFile__doc__},
 {"NavCompleteSave",	(PyCFunction)nav_NavCompleteSave,	METH_VARARGS,	nav_NavCompleteSave__doc__},
 {"NavCustomControl",	(PyCFunction)nav_NavCustomControl,	METH_VARARGS,	nav_NavCustomControl__doc__},
 {"NavServicesCanRun",	(PyCFunction)nav_NavServicesCanRun,	METH_VARARGS,	nav_NavServicesCanRun__doc__},
 {"NavServicesAvailable",	(PyCFunction)nav_NavServicesAvailable,	METH_VARARGS,	nav_NavServicesAvailable__doc__},
 {"NavLoad",	(PyCFunction)nav_NavLoad,	METH_VARARGS,	nav_NavLoad__doc__},
 {"NavUnload",	(PyCFunction)nav_NavUnload,	METH_VARARGS,	nav_NavUnload__doc__},
 {"NavLibraryVersion",	(PyCFunction)nav_NavLibraryVersion,	METH_VARARGS,	nav_NavLibraryVersion__doc__},
#ifdef notdef
 {"NavGetDefaultDialogOptions",	(PyCFunction)nav_NavGetDefaultDialogOptions,	METH_VARARGS,	nav_NavGetDefaultDialogOptions__doc__},
#endif
	{NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initNav) */

static char Nav_module_documentation[] = 
""
;

void
initNav()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule4("Nav", nav_methods,
		Nav_module_documentation,
		(PyObject*)NULL,PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("Nav.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* XXXX Add constants here */
	
	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module Nav");
}

