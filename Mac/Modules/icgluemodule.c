/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
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

extern int ResObj_Convert(PyObject *, Handle *); /* From Resmodule.c */

#ifdef WITHOUT_FRAMEWORKS
#if !TARGET_API_MAC_OS8
/* The Carbon headers define PRAGMA_ALIGN_SUPPORT to something illegal,
** because you shouldn't use it for Carbon. All good and well, but portable
** code still needs it. So, we undefine it here.
*/
#undef PRAGMA_ALIGN_SUPPORTED
#define PRAGMA_ALIGN_SUPPORTED 0
#endif /* !TARGET_API_MAC_OS8 */

#include "ICAPI.h"
#else
#include <Carbon/Carbon.h>
typedef OSStatus ICError;
/* Some fields in ICMapEntry have changed names. */
#define file_type fileType
#define file_creator fileCreator
#define post_creator postCreator
#define creator_app_name creatorAppName
#define post_app_name postAppName
#define MIME_type MIMEType
#define entry_name entryName
#endif

static PyObject *ErrorObject;

static PyObject *
ici_error(ICError err)
{
	PyErr_SetObject(ErrorObject, PyInt_FromLong((long)err));
	return NULL;
}

/* ----------------------------------------------------- */

/* Declarations for objects of type ic_instance */

typedef struct {
	PyObject_HEAD
	ICInstance inst;
} iciobject;

staticforward PyTypeObject Icitype;



/* ---------------------------------------------------------------- */

#if TARGET_API_MAC_OS8
static char ici_ICFindConfigFile__doc__[] = 
"()->None; Find config file in standard places"
;

static PyObject *
ici_ICFindConfigFile(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ((err=ICFindConfigFile(self->inst, 0, NULL)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}


static char ici_ICFindUserConfigFile__doc__[] = 
"(vRefNum, dirID)->None; Find config file in specified place"
;

static PyObject *
ici_ICFindUserConfigFile(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	ICDirSpec where;	

	if (!PyArg_ParseTuple(args, "sl", &where.vRefNum, &where.dirID))
		return NULL;
	if ((err=ICFindUserConfigFile(self->inst, &where)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}


static char ici_ICChooseConfig__doc__[] = 
"()->None; Let the user choose a config file"
;

static PyObject *
ici_ICChooseConfig(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ((err=ICChooseConfig(self->inst)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}

static char ici_ICChooseNewConfig__doc__[] = 
"()->None; Let the user choose a new config file"
;

static PyObject *
ici_ICChooseNewConfig(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ((err=ICChooseNewConfig(self->inst)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* TARGET_API_MAC_OS8 */


static char ici_ICGetSeed__doc__[] = 
"()->int; Returns int that changes when configuration does"
;

static PyObject *
ici_ICGetSeed(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	long seed;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ((err=ICGetSeed(self->inst, &seed)) != 0 )
		return ici_error(err);
	return Py_BuildValue("i", (int)seed);
}


static char ici_ICBegin__doc__[] = 
"(perm)->None; Lock config file for read/write"
;

static PyObject *
ici_ICBegin(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	int perm;
	
	if (!PyArg_ParseTuple(args, "i", &perm))
		return NULL;
	if ((err=ICBegin(self->inst, (ICPerm)perm)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}


static char ici_ICFindPrefHandle__doc__[] = 
"(key, handle)->attrs; Lookup key, store result in handle, return attributes"
;

static PyObject *
ici_ICFindPrefHandle(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	Str255 key;
	ICAttr attr;
	Handle h;
	
	if (!PyArg_ParseTuple(args, "O&O&", PyMac_GetStr255, &key, ResObj_Convert, &h))
		return NULL;
	if ((err=ICFindPrefHandle(self->inst, key, &attr, h)) != 0 )
		return ici_error(err);
	return Py_BuildValue("i", (int)attr);
}


static char ici_ICSetPref__doc__[] = 
"(key, attr, data)->None; Set preference key to data with attributes"
;

static PyObject *
ici_ICSetPref(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	Str255 key;
	int attr;
	char *data;
	int datalen;
	
	if (!PyArg_ParseTuple(args, "O&is#", PyMac_GetStr255, &key, &attr, 
					&data, &datalen))
		return NULL;
	if ((err=ICSetPref(self->inst, key, (ICAttr)attr, (Ptr)data, 
			(long)datalen)) != 0)
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}


static char ici_ICCountPref__doc__[] = 
"()->int; Return number of preferences"
;

static PyObject *
ici_ICCountPref(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	long count;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ((err=ICCountPref(self->inst, &count)) != 0 )
		return ici_error(err);
	return Py_BuildValue("i", (int)count);
}


static char ici_ICGetIndPref__doc__[] = 
"(num)->key; Return key of preference with given index"
;

static PyObject *
ici_ICGetIndPref(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	long num;
	Str255 key;
	
	if (!PyArg_ParseTuple(args, "l", &num))
		return NULL;
	if ((err=ICGetIndPref(self->inst, num, key)) != 0 )
		return ici_error(err);
	return Py_BuildValue("O&", PyMac_BuildStr255, key);
}


static char ici_ICDeletePref__doc__[] = 
"(key)->None; Delete preference"
;

static PyObject *
ici_ICDeletePref(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	Str255 key;

	if (!PyArg_ParseTuple(args, "O&", PyMac_GetStr255, key))
		return NULL;
	if ((err=ICDeletePref(self->inst, key)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}


static char ici_ICEnd__doc__[] = 
"()->None; Unlock file after ICBegin call"
;

static PyObject *
ici_ICEnd(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ((err=ICEnd(self->inst)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}


static char ici_ICEditPreferences__doc__[] = 
"(key)->None; Ask user to edit preferences, staring with key"
;

static PyObject *
ici_ICEditPreferences(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	Str255 key;
	
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetStr255, key))
		return NULL;
	if ((err=ICEditPreferences(self->inst, key)) != 0 )
		return ici_error(err);
	Py_INCREF(Py_None);
	return Py_None;
}


static char ici_ICParseURL__doc__[] = 
"(hint, data, selStart, selEnd, handle)->selStart, selEnd; Find an URL, return in handle"
;

static PyObject *
ici_ICParseURL(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	Str255 hint;
	char *data;
	int datalen;
	long selStart, selEnd;
	Handle h;
	
	if (!PyArg_ParseTuple(args, "O&s#llO&", PyMac_GetStr255, hint, &data, &datalen,
				&selStart, &selEnd, ResObj_Convert, &h))
		return NULL;
	if ((err=ICParseURL(self->inst, hint, (Ptr)data, (long)datalen,
				&selStart, &selEnd, h)) != 0 )
		return ici_error(err);
	return Py_BuildValue("ii", (int)selStart, (int)selEnd);
}


static char ici_ICLaunchURL__doc__[] = 
"(hint, data, selStart, selEnd)->None; Find an URL and launch the correct app"
;

static PyObject *
ici_ICLaunchURL(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	Str255 hint;
	char *data;
	int datalen;
	long selStart, selEnd;
	
	if (!PyArg_ParseTuple(args, "O&s#ll", PyMac_GetStr255, hint, &data, &datalen,
				&selStart, &selEnd))
		return NULL;
	if ((err=ICLaunchURL(self->inst, hint, (Ptr)data, (long)datalen,
				&selStart, &selEnd)) != 0 )
		return ici_error(err);
	return Py_BuildValue("ii", (int)selStart, (int)selEnd);
}


static char ici_ICMapFilename__doc__[] = 
"(filename)->mapinfo; Get filemap info for given filename"
;

static PyObject *
ici_ICMapFilename(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	Str255 filename;
	ICMapEntry entry;
	
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetStr255, filename))
		return NULL;
	if ((err=ICMapFilename(self->inst, filename, &entry)) != 0 )
		return ici_error(err);
	return Py_BuildValue("hO&O&O&lO&O&O&O&O&", entry.version, 
		PyMac_BuildOSType, entry.file_type,
		PyMac_BuildOSType, entry.file_creator, 
		PyMac_BuildOSType, entry.post_creator, 
		entry.flags,
		PyMac_BuildStr255, entry.extension,
		PyMac_BuildStr255, entry.creator_app_name,
		PyMac_BuildStr255, entry.post_app_name,
		PyMac_BuildStr255, entry.MIME_type,
		PyMac_BuildStr255, entry.entry_name);
}


static char ici_ICMapTypeCreator__doc__[] = 
"(type, creator, filename)->mapinfo; Get filemap info for given tp/cr/filename"
;

static PyObject *
ici_ICMapTypeCreator(self, args)
	iciobject *self;
	PyObject *args;
{
	ICError err;
	OSType type, creator;
	Str255 filename;
	ICMapEntry entry;
	
	if (!PyArg_ParseTuple(args, "O&O&O&",
			PyMac_GetOSType, &type,
			PyMac_GetOSType, &creator,
			PyMac_GetStr255, filename))
		return NULL;
	if ((err=ICMapTypeCreator(self->inst, type, creator, filename, &entry)) != 0 )
		return ici_error(err);
	return Py_BuildValue("hO&O&O&lO&O&O&O&O&", entry.version, 
		PyMac_BuildOSType, entry.file_type,
		PyMac_BuildOSType, entry.file_creator, 
		PyMac_BuildOSType, entry.post_creator, 
		entry.flags,
		PyMac_BuildStr255, entry.extension,
		PyMac_BuildStr255, entry.creator_app_name,
		PyMac_BuildStr255, entry.post_app_name,
		PyMac_BuildStr255, entry.MIME_type,
		PyMac_BuildStr255, entry.entry_name);
}


static struct PyMethodDef ici_methods[] = {
#if TARGET_API_MAC_OS8
 {"ICFindConfigFile",	(PyCFunction)ici_ICFindConfigFile,	METH_VARARGS,	ici_ICFindConfigFile__doc__},
 {"ICFindUserConfigFile",	(PyCFunction)ici_ICFindUserConfigFile,	METH_VARARGS,	ici_ICFindUserConfigFile__doc__},
 {"ICChooseConfig",	(PyCFunction)ici_ICChooseConfig,	METH_VARARGS,	ici_ICChooseConfig__doc__},
 {"ICChooseNewConfig",	(PyCFunction)ici_ICChooseNewConfig,	METH_VARARGS,	ici_ICChooseNewConfig__doc__},
#endif /* TARGET_API_MAC_OS8 */
 {"ICGetSeed",	(PyCFunction)ici_ICGetSeed,	METH_VARARGS,	ici_ICGetSeed__doc__},
 {"ICBegin",	(PyCFunction)ici_ICBegin,	METH_VARARGS,	ici_ICBegin__doc__},
 {"ICFindPrefHandle",	(PyCFunction)ici_ICFindPrefHandle,	METH_VARARGS,	ici_ICFindPrefHandle__doc__},
 {"ICSetPref",	(PyCFunction)ici_ICSetPref,	METH_VARARGS,	ici_ICSetPref__doc__},
 {"ICCountPref",	(PyCFunction)ici_ICCountPref,	METH_VARARGS,	ici_ICCountPref__doc__},
 {"ICGetIndPref",	(PyCFunction)ici_ICGetIndPref,	METH_VARARGS,	ici_ICGetIndPref__doc__},
 {"ICDeletePref",	(PyCFunction)ici_ICDeletePref,	METH_VARARGS,	ici_ICDeletePref__doc__},
 {"ICEnd",	(PyCFunction)ici_ICEnd,	METH_VARARGS,	ici_ICEnd__doc__},
 {"ICEditPreferences",	(PyCFunction)ici_ICEditPreferences,	METH_VARARGS,	ici_ICEditPreferences__doc__},
 {"ICParseURL",	(PyCFunction)ici_ICParseURL,	METH_VARARGS,	ici_ICParseURL__doc__},
 {"ICLaunchURL",	(PyCFunction)ici_ICLaunchURL,	METH_VARARGS,	ici_ICLaunchURL__doc__},
 {"ICMapFilename",	(PyCFunction)ici_ICMapFilename,	METH_VARARGS,	ici_ICMapFilename__doc__},
 {"ICMapTypeCreator",	(PyCFunction)ici_ICMapTypeCreator,	METH_VARARGS,	ici_ICMapTypeCreator__doc__},
 
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */


static iciobject *
newiciobject(OSType creator)
{
	iciobject *self;
	ICError err;
	
	self = PyObject_NEW(iciobject, &Icitype);
	if (self == NULL)
		return NULL;
	if ((err=ICStart(&self->inst, creator)) != 0 ) {
		(void)ici_error(err);
		PyMem_DEL(self);
		return NULL;
	}
	return self;
}


static void
ici_dealloc(self)
	iciobject *self;
{
	(void)ICStop(self->inst);
	PyMem_DEL(self);
}

static PyObject *
ici_getattr(self, name)
	iciobject *self;
	char *name;
{
	return Py_FindMethod(ici_methods, (PyObject *)self, name);
}

static char Icitype__doc__[] = 
"Internet Config instance"
;

static PyTypeObject Icitype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"ic_instance",			/*tp_name*/
	sizeof(iciobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)ici_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)ici_getattr,	/*tp_getattr*/
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
	Icitype__doc__ /* Documentation string */
};

/* End of code for ic_instance objects */
/* -------------------------------------------------------- */


static char ic_ICStart__doc__[] =
"(OSType)->ic_instance; Create an Internet Config instance"
;

static PyObject *
ic_ICStart(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	OSType creator;

	if (!PyArg_ParseTuple(args, "O&", PyMac_GetOSType, &creator))
		return NULL;
	return (PyObject *)newiciobject(creator);
}

/* List of methods defined in the module */

static struct PyMethodDef ic_methods[] = {
	{"ICStart",	(PyCFunction)ic_ICStart,	METH_VARARGS,	ic_ICStart__doc__},
 
	{NULL,	 (PyCFunction)NULL, 0, NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initicglue) */

static char icglue_module_documentation[] = 
"Implements low-level Internet Config interface"
;

void
initicglue()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule4("icglue", ic_methods,
		icglue_module_documentation,
		(PyObject*)NULL,PYTHON_API_VERSION);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("icglue.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* XXXX Add constants here */
	
	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module icglue");
}

