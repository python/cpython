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
#include "pymactoolbox.h"
#include <Carbon/Carbon.h>

static PyObject *ErrorObject;

static NavEventUPP my_eventProcUPP;
static NavPreviewUPP my_previewProcUPP;
static NavObjectFilterUPP my_filterProcUPP;

/* Callback functions */
static pascal void
my_eventProc(NavEventCallbackMessage callBackSelector,
			 NavCBRecPtr callBackParms,
			 NavCallBackUserData callbackUD)
{
	PyObject *dict = (PyObject *)callbackUD;
	PyObject *pyfunc;
	PyObject *rv;
	
	if (!dict) return;
	if ( (pyfunc = PyDict_GetItemString(dict, "eventProc")) == NULL ) {
		PyErr_Print();
		return;
	}
	if ( pyfunc == Py_None ) {
		return;
	}
	rv = PyObject_CallFunction(pyfunc, "ls#", (long)callBackSelector,
			(void *)callBackParms, sizeof(NavCBRec));
	if ( rv )
		Py_DECREF(rv);
	else {
		PySys_WriteStderr("Nav: exception in eventProc callback\n");
		PyErr_Print();
	}
}

static pascal Boolean
my_previewProc(NavCBRecPtr callBackParms,
			   NavCallBackUserData callbackUD)
{
	PyObject *dict = (PyObject *)callbackUD;
	PyObject *pyfunc;
	PyObject *rv;
	Boolean c_rv = false;
	
	if (!dict) return false;
	if ( (pyfunc = PyDict_GetItemString(dict, "previewProc")) == NULL ) {
		PyErr_Print();
		return false;
	}
	rv = PyObject_CallFunction(pyfunc, "s#", (void *)callBackParms, sizeof(NavCBRec));
	if ( rv ) {
		c_rv = PyObject_IsTrue(rv);
		Py_DECREF(rv);
	} else {
		PySys_WriteStderr("Nav: exception in previewProc callback\n");
		PyErr_Print();
	}
	return c_rv;
}

static pascal Boolean
my_filterProc(AEDesc *theItem, void *info,
			  NavCallBackUserData callbackUD,
			  NavFilterModes filterMode)
{
	PyObject *dict = (PyObject *)callbackUD;
	PyObject *pyfunc;
	PyObject *rv;
	Boolean c_rv = false;
	
	if (!dict) return false;
	if ( (pyfunc = PyDict_GetItemString(dict, "filterProc")) == NULL ) {
		PyErr_Print();
		return false;
	}
	rv = PyObject_CallFunction(pyfunc, "O&s#h",
		AEDesc_NewBorrowed, theItem, info, sizeof(NavFileOrFolderInfo), (short)filterMode);
	if ( rv ) {
		c_rv = PyObject_IsTrue(rv);
		Py_DECREF(rv);
	} else {
		PySys_WriteStderr("Nav: exception in filterProc callback\n");
		PyErr_Print();
	}
	return c_rv;
}

/* ----------------------------------------------------- */
static int
filldialogoptions(PyObject *d,
		AEDesc **defaultLocationP,
		NavDialogOptions *opt,
		NavEventUPP *eventProcP,
		NavPreviewUPP *previewProcP,
		NavObjectFilterUPP *filterProcP,
		NavTypeListHandle *typeListP,
		OSType *fileTypeP,
		OSType *fileCreatorP)
{
	Py_ssize_t pos = 0;
	PyObject *key, *value;
	char *keystr;
	AEDesc *defaultLocation_storage;
	
	NavGetDefaultDialogOptions(opt);

	while ( PyDict_Next(d, &pos, &key, &value) ) {
		if ( !key || !value || !PyString_Check(key) ) {
			PyErr_SetString(ErrorObject, "DialogOption has non-string key");
			return 0;
		}
		keystr = PyString_AsString(key);
		if( strcmp(keystr, "defaultLocation") == 0 ) {
			if ( (defaultLocation_storage = PyMem_NEW(AEDesc, 1)) == NULL ) {
				PyErr_NoMemory();
				return 0;
			}
			if ( !PyArg_Parse(value, "O&", AEDesc_Convert, defaultLocation_storage) ) {
				PyMem_DEL(defaultLocation_storage);
				return 0;
			}
			*defaultLocationP = defaultLocation_storage;
		} else if( strcmp(keystr, "version") == 0 ) {
			if ( !PyArg_Parse(value, "H", &opt->version) )
				return 0;
		} else if( strcmp(keystr, "dialogOptionFlags") == 0 ) {
			if ( !PyArg_Parse(value, "k", &opt->dialogOptionFlags) )
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
			*eventProcP = my_eventProcUPP;
		} else if( previewProcP && strcmp(keystr, "previewProc") == 0 ) {
			*previewProcP = my_previewProcUPP;
		} else if( filterProcP && strcmp(keystr, "filterProc") == 0 ) {
			*filterProcP = my_filterProcUPP;
		} else if( typeListP && strcmp(keystr, "typeList") == 0 ) {
			if ( !PyArg_Parse(value, "O&", ResObj_Convert, typeListP) )
				return 0;
		} else if( fileTypeP && strcmp(keystr, "fileType") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetOSType, fileTypeP) )
				return 0;
		} else if( fileCreatorP && strcmp(keystr, "fileCreator") == 0 ) {
			if ( !PyArg_Parse(value, "O&", PyMac_GetOSType, fileCreatorP) )
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

static PyTypeObject Navrrtype;



/* ---------------------------------------------------------------- */

static char nav_NavTranslateFile__doc__[] =
"(NavTranslationOptions)->None"
;

static PyObject *
nav_NavTranslateFile(navrrobject *self, PyObject *args)
{
	NavTranslationOptions howToTranslate;
	OSErr err;

	if (!PyArg_ParseTuple(args, "k", &howToTranslate))
		return NULL;
	err = NavTranslateFile(&self->itself, howToTranslate);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavCompleteSave__doc__[] =
"(NavTranslationOptions)->None"
;

static PyObject *
nav_NavCompleteSave(navrrobject *self, PyObject *args)
{
	NavTranslationOptions howToTranslate;
	OSErr err;

	if (!PyArg_ParseTuple(args, "k", &howToTranslate))
		return NULL;
	err = NavCompleteSave(&self->itself, howToTranslate);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}


static struct PyMethodDef navrr_methods[] = {
 {"NavTranslateFile",	(PyCFunction)nav_NavTranslateFile,	METH_VARARGS,	nav_NavTranslateFile__doc__},
 {"NavCompleteSave",	(PyCFunction)nav_NavCompleteSave,	METH_VARARGS,	nav_NavCompleteSave__doc__},
	
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
navrr_dealloc(navrrobject *self)
{
	NavDisposeReply(&self->itself);
	PyObject_DEL(self);
}

static PyObject *
navrr_getattr(navrrobject *self, char *name)
{
	FSSpec fss;
	FSRef fsr;
	
	if( strcmp(name, "__members__") == 0 )
		return Py_BuildValue("ssssssssss", "version", "validRecord", "replacing",
			"isStationery", "translationNeeded", "selection", "selection_fsr",
			"fileTranslation", "keyScript", "saveFileName");
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
	if( strcmp(name, "selection") == 0 ) {
		SInt32 i, count;
		OSErr err;
		PyObject *rv, *rvitem;
		AEDesc desc;
		
		if ((err=AECountItems(&self->itself.selection, &count))) {
			PyErr_Mac(ErrorObject, err);
			return NULL;
		}
		if ( (rv=PyList_New(count)) == NULL )
			return NULL;
		for(i=0; i<count; i++) {
			desc.dataHandle = NULL;
			if ((err=AEGetNthDesc(&self->itself.selection, i+1, typeFSS, NULL, &desc))) {
				Py_DECREF(rv);
				PyErr_Mac(ErrorObject, err);
				return NULL;
			}
			if ((err=AEGetDescData(&desc, &fss, sizeof(FSSpec)))) {
				Py_DECREF(rv);
				PyErr_Mac(ErrorObject, err);
				return NULL;
			}
			rvitem = PyMac_BuildFSSpec(&fss);
			PyList_SetItem(rv, i, rvitem);
			AEDisposeDesc(&desc);
		}
		return rv;
	}
	if( strcmp(name, "selection_fsr") == 0 ) {
		SInt32 i, count;
		OSErr err;
		PyObject *rv, *rvitem;
		AEDesc desc;
		
		if ((err=AECountItems(&self->itself.selection, &count))) {
			PyErr_Mac(ErrorObject, err);
			return NULL;
		}
		if ( (rv=PyList_New(count)) == NULL )
			return NULL;
		for(i=0; i<count; i++) {
			desc.dataHandle = NULL;
			if ((err=AEGetNthDesc(&self->itself.selection, i+1, typeFSRef, NULL, &desc))) {
				Py_DECREF(rv);
				PyErr_Mac(ErrorObject, err);
				return NULL;
			}
			if ((err=AEGetDescData(&desc, &fsr, sizeof(FSRef)))) {
				Py_DECREF(rv);
				PyErr_Mac(ErrorObject, err);
				return NULL;
			}
			rvitem = PyMac_BuildFSRef(&fsr);
			PyList_SetItem(rv, i, rvitem);
			AEDisposeDesc(&desc);
		}
		return rv;
	}
	if( strcmp(name, "fileTranslation") == 0 )
		return ResObj_New((Handle)self->itself.fileTranslation);
	if( strcmp(name, "keyScript") == 0 )
		return Py_BuildValue("h", (short)self->itself.keyScript);
	if( strcmp(name, "saveFileName") == 0 )
		return Py_BuildValue("O&", CFStringRefObj_New, self->itself.saveFileName);


	return Py_FindMethod(navrr_methods, (PyObject *)self, name);
}

static int
navrr_setattr(navrrobject *self, char *name, PyObject *v)
{
	/* Set attribute 'name' to value 'v'. v==NULL means delete */
	
	/* XXXX Add your own setattr code here */
	return -1;
}

static char Navrrtype__doc__[] = 
"Record containing result of a Nav file selection call. Use dir() for member names."
;

static PyTypeObject Navrrtype = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"Nav.NavReplyRecord",			/*tp_name*/
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
"(DialogOptions dict or kwargs+defaultLocation,eventProc,previewProc,filterProc,typeList) -> NavReplyRecord"
;

static PyObject *
nav_NavGetFile(PyObject *self, PyObject *args, PyObject *kw)
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

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &defaultLocation, &dialogOptions, &eventProc, &previewProc, &filterProc, &typeList, NULL, NULL))
		return NULL;
	err = NavGetFile(defaultLocation, &reply, &dialogOptions,
			eventProc, previewProc, filterProc, typeList, (void *)dict);
	PyMem_DEL(defaultLocation);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

static char nav_NavPutFile__doc__[] =
"(DialogOptions dict or kwargs+defaultLocation,eventProc,fileCreator,fileType) -> NavReplyRecord"
;

static PyObject *
nav_NavPutFile(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	AEDesc	*defaultLocation = NULL;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NULL;
	OSType fileType;
	OSType fileCreator;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &defaultLocation, &dialogOptions, &eventProc, NULL, NULL, NULL, &fileType, &fileCreator))
		return NULL;
	err = NavPutFile(defaultLocation, &reply, &dialogOptions,
			eventProc, fileType, fileCreator, (void *)dict);
	PyMem_DEL(defaultLocation);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

static char nav_NavAskSaveChanges__doc__[] =
"(NavAskSaveChangesAction, DialogOptions dict or kwargs+eventProc) -> NavAskSaveChangesResult"

;

static PyObject *
nav_NavAskSaveChanges(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	NavDialogOptions dialogOptions;
	NavAskSaveChangesAction action;
	NavAskSaveChangesResult reply;
	NavEventUPP eventProc = NULL;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, "k", &action))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "lO!", &action, &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, NULL, &dialogOptions, &eventProc, NULL, NULL, NULL, NULL, NULL))
		return NULL;
	err = NavAskSaveChanges(&dialogOptions, action, &reply, eventProc, (void *)dict);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("l", (long)reply);
}

static char nav_NavCustomAskSaveChanges__doc__[] =
"(DialogOptions dict or kwargs+eventProc) -> NavAskSaveChangesResult"
;

static PyObject *
nav_NavCustomAskSaveChanges(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	NavDialogOptions dialogOptions;
	NavAskSaveChangesResult reply;
	NavEventUPP eventProc = NULL;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, NULL, &dialogOptions, &eventProc, NULL, NULL, NULL, NULL, NULL))
		return NULL;
	err = NavCustomAskSaveChanges(&dialogOptions, &reply, eventProc, (void *)dict);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("l", (long)reply);
}

static char nav_NavAskDiscardChanges__doc__[] =
"(DialogOptions dict or kwargs+eventProc) -> NavAskSaveChangesResult"
;

static PyObject *
nav_NavAskDiscardChanges(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	NavDialogOptions dialogOptions;
	NavAskSaveChangesResult reply;
	NavEventUPP eventProc = NULL;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, NULL, &dialogOptions, &eventProc, NULL, NULL, NULL, NULL, NULL))
		return NULL;
	err = NavAskDiscardChanges(&dialogOptions, &reply, eventProc, (void *)dict);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("l", (long)reply);
}

static char nav_NavChooseFile__doc__[] =
"(DialogOptions dict or kwargs+defaultLocation,eventProc,previewProc,filterProc,typeList) -> NavReplyRecord"
;

static PyObject *
nav_NavChooseFile(PyObject *self, PyObject *args, PyObject *kw)
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

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &defaultLocation, &dialogOptions, &eventProc, &previewProc, &filterProc, &typeList, NULL, NULL))
		return NULL;
	err = NavChooseFile(defaultLocation, &reply, &dialogOptions,
			eventProc, previewProc, filterProc, typeList, (void *)dict);
	PyMem_DEL(defaultLocation);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

static char nav_NavChooseFolder__doc__[] =
"(DialogOptions dict or kwargs+defaultLocation,eventProc,filterProc) -> NavReplyRecord"
;

static PyObject *
nav_NavChooseFolder(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	AEDesc	*defaultLocation = NULL;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NULL;
	NavObjectFilterUPP filterProc = NULL;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &defaultLocation, &dialogOptions, &eventProc, NULL, &filterProc, NULL, NULL, NULL))
		return NULL;
	err = NavChooseFolder(defaultLocation, &reply, &dialogOptions,
			eventProc, filterProc, (void *)dict);
	PyMem_DEL(defaultLocation);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

static char nav_NavChooseVolume__doc__[] =
"(DialogOptions dict or kwargs+defaultLocation,eventProc,filterProc) -> NavReplyRecord"
;

static PyObject *
nav_NavChooseVolume(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	AEDesc	*defaultLocation = NULL;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NULL;
	NavObjectFilterUPP filterProc = NULL;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &defaultLocation, &dialogOptions, &eventProc, NULL, &filterProc, NULL, NULL, NULL))
		return NULL;
	err = NavChooseVolume(defaultLocation, &reply, &dialogOptions,
			eventProc, filterProc, (void *)dict);
	PyMem_DEL(defaultLocation);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

static char nav_NavChooseObject__doc__[] =
"(DialogOptions dict or kwargs+defaultLocation,eventProc,filterProc) -> NavReplyRecord"
;

static PyObject *
nav_NavChooseObject(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	AEDesc	*defaultLocation = NULL;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NULL;
	NavObjectFilterUPP filterProc = NULL;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &defaultLocation, &dialogOptions, &eventProc, NULL, &filterProc, NULL, NULL, NULL))
		return NULL;
	err = NavChooseObject(defaultLocation, &reply, &dialogOptions,
			eventProc, filterProc, (void *)dict);
	PyMem_DEL(defaultLocation);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

static char nav_NavNewFolder__doc__[] =
"(DialogOptions dict or kwargs+defaultLocation,eventProc) -> NavReplyRecord"
;

static PyObject *
nav_NavNewFolder(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *dict;
	AEDesc	*defaultLocation = NULL;
	NavReplyRecord reply;
	NavDialogOptions dialogOptions;
	NavEventUPP eventProc = NULL;
	OSErr err;

	if ( kw && PyObject_IsTrue(kw) ) {
		if (!PyArg_ParseTuple(args, ";either keyword arguments or dictionary expected"))
			return NULL;
		dict = kw;
	} else if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
		return NULL;
	if (!filldialogoptions(dict, &defaultLocation, &dialogOptions, &eventProc, NULL, NULL, NULL, NULL, NULL))
		return NULL;
	err = NavNewFolder(defaultLocation, &reply, &dialogOptions, eventProc, (void *)dict);
	PyMem_DEL(defaultLocation);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newnavrrobject(&reply);
}

#if 0
/* XXXX I don't know what to do with the void * argument */
static char nav_NavCustomControl__doc__[] =
""
;


static PyObject *
nav_NavCustomControl(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

static char nav_NavServicesCanRun__doc__[] =
"()->int"
;

static PyObject *
nav_NavServicesCanRun(PyObject *self, PyObject *args)
{
	Boolean rv;
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	rv = NavServicesCanRun();
	return Py_BuildValue("l", (long)rv);
}

static char nav_NavServicesAvailable__doc__[] =
"()->int"
;

static PyObject *
nav_NavServicesAvailable(PyObject *self, PyObject *args)
{
	Boolean rv;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	rv = NavServicesAvailable();
	return Py_BuildValue("l", (long)rv);
}
/* XX */
static char nav_NavLoad__doc__[] =
"()->None"
;

static PyObject *
nav_NavLoad(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	NavLoad();
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavUnload__doc__[] =
"()->None"
;

static PyObject *
nav_NavUnload(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	NavUnload();
	Py_INCREF(Py_None);
	return Py_None;
}

static char nav_NavLibraryVersion__doc__[] =
"()->int"
;

static PyObject *
nav_NavLibraryVersion(PyObject *self, PyObject *args)
{
	UInt32 rv;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	rv = NavLibraryVersion();
	return Py_BuildValue("l", (long)rv);
}

static char nav_NavGetDefaultDialogOptions__doc__[] =
"()->dict\nPass dict or keyword args with same names to other calls."
;

static PyObject *
nav_NavGetDefaultDialogOptions(PyObject *self, PyObject *args)
{
	NavDialogOptions dialogOptions;
	OSErr err;
	
	err = NavGetDefaultDialogOptions(&dialogOptions);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("{s:h,s:l,s:O&,s:O&,s:O&,s:O&,s:O&,s:O&,s:O&,s:O&,s:O&}",
		"version", dialogOptions.version,
		"dialogOptionFlags", dialogOptions.dialogOptionFlags,
		"location", PyMac_BuildPoint, dialogOptions.location,
		"clientName", PyMac_BuildStr255, &dialogOptions.clientName,
		"windowTitle", PyMac_BuildStr255, &dialogOptions.windowTitle,
		"actionButtonLabel", PyMac_BuildStr255, &dialogOptions.actionButtonLabel,
		"cancelButtonLabel", PyMac_BuildStr255, &dialogOptions.cancelButtonLabel,
		"savedFileName", PyMac_BuildStr255, &dialogOptions.savedFileName,
		"message", PyMac_BuildStr255, &dialogOptions.message,
		"preferenceKey", PyMac_BuildOSType, dialogOptions.preferenceKey,
		"popupExtension", OptResObj_New, dialogOptions.popupExtension);
}

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
#if 0
 {"NavCustomControl",	(PyCFunction)nav_NavCustomControl,	METH_VARARGS,	nav_NavCustomControl__doc__},
#endif
 {"NavServicesCanRun",	(PyCFunction)nav_NavServicesCanRun,	METH_VARARGS,	nav_NavServicesCanRun__doc__},
 {"NavServicesAvailable",	(PyCFunction)nav_NavServicesAvailable,	METH_VARARGS,	nav_NavServicesAvailable__doc__},
 {"NavLoad",	(PyCFunction)nav_NavLoad,	METH_VARARGS,	nav_NavLoad__doc__},
 {"NavUnload",	(PyCFunction)nav_NavUnload,	METH_VARARGS,	nav_NavUnload__doc__},
 {"NavLibraryVersion",	(PyCFunction)nav_NavLibraryVersion,	METH_VARARGS,	nav_NavLibraryVersion__doc__},
 {"NavGetDefaultDialogOptions",	(PyCFunction)nav_NavGetDefaultDialogOptions,	METH_VARARGS,	nav_NavGetDefaultDialogOptions__doc__},
	{NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initNav) */

static char Nav_module_documentation[] = 
"Interface to Navigation Services\n"
"Most calls accept a NavDialogOptions dictionary or keywords with the same names, pass {}\n"
"if you want the default options.\n"
"Use NavGetDefaultDialogOptions() to find out common option names.\n"
"See individual docstrings for additional keyword args/dictentries supported by each call.\n"
"Pass None as eventProc to get movable-modal dialogs that process updates through the standard Python mechanism."
;

void
initNav(void)
{
	PyObject *m, *d;

	/* Test that we have NavServices */
	if ( !NavServicesAvailable() ) {
		PyErr_SetString(PyExc_ImportError, "Navigation Services not available");
		return;
	}
	/* Create the module and add the functions */
	m = Py_InitModule4("Nav", nav_methods,
		Nav_module_documentation,
		(PyObject*)NULL,PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("Nav.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* XXXX Add constants here */
	
	/* Set UPPs */
	my_eventProcUPP = NewNavEventUPP(my_eventProc);
	my_previewProcUPP = NewNavPreviewUPP(my_previewProc);
	my_filterProcUPP = NewNavObjectFilterUPP(my_filterProc);
	
}

