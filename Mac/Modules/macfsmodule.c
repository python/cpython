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
#include "macglue.h"

#include <Memory.h>
#include <Files.h>
#include <Folders.h>
#include <StandardFile.h>
#include <Aliases.h>
#include <LowMem.h>

#include "getapplbycreator.h"

#ifdef THINK_C
#define FileFilterUPP FileFilterProcPtr
#endif

static PyObject *ErrorObject;

/* ----------------------------------------------------- */
/* Declarations for objects of type Alias */

typedef struct {
	PyObject_HEAD
	AliasHandle alias;
} mfsaobject;

staticforward PyTypeObject Mfsatype;

#define is_mfsaobject(v)		((v)->ob_type == &Mfsatype)

/* ---------------------------------------------------------------- */
/* Declarations for objects of type FSSpec */

typedef struct {
	PyObject_HEAD
	FSSpec fsspec;
} mfssobject;

staticforward PyTypeObject Mfsstype;

#define is_mfssobject(v)		((v)->ob_type == &Mfsstype)


/* ---------------------------------------------------------------- */
/* Declarations for objects of type FInfo */

typedef struct {
	PyObject_HEAD
	FInfo finfo;
} mfsiobject;

staticforward PyTypeObject Mfsitype;

#define is_mfsiobject(v)		((v)->ob_type == &Mfsitype)


mfssobject *newmfssobject(FSSpec *fss); /* Forward */

/* ---------------------------------------------------------------- */

static PyObject *
mfsa_Resolve(self, args)
	mfsaobject *self;
	PyObject *args;
{
	FSSpec from, *fromp, result;
	Boolean changed;
	OSErr err;
	
	from.name[0] = 0;
	if (!PyArg_ParseTuple(args, "|O&", PyMac_GetFSSpec, &from))
		return NULL;
	if (from.name[0] )
		fromp = &from;
	else
		fromp = NULL;
	err = ResolveAlias(fromp, self->alias, &result, &changed);
	if ( err && err != fnfErr ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("(Oi)", newmfssobject(&result), (int)changed);
}

static PyObject *
mfsa_GetInfo(self, args)
	mfsaobject *self;
	PyObject *args;
{
	Str63 value;
	int i;
	OSErr err;
	
	if (!PyArg_ParseTuple(args, "i", &i))
		return NULL;
	err = GetAliasInfo(self->alias, (AliasInfoType)i, value);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return 0;
	}
	return PyString_FromStringAndSize((char *)&value[1], value[0]);
}

static PyObject *
mfsa_Update(self, args)
	mfsaobject *self;
	PyObject *args;
{
	FSSpec target, fromfile, *fromfilep;
	OSErr err;
	Boolean changed;
	
	fromfile.name[0] = 0;
	if (!PyArg_ParseTuple(args, "O&|O&",  PyMac_GetFSSpec, &target,
					 PyMac_GetFSSpec, &fromfile))
		return NULL;
	if ( fromfile.name[0] )
		fromfilep = &fromfile;
	else
		fromfilep = NULL;
	err = UpdateAlias(fromfilep, &target, self->alias, &changed);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return 0;
	}
	return Py_BuildValue("i", (int)changed);
}

static struct PyMethodDef mfsa_methods[] = {
	{"Resolve",	(PyCFunction)mfsa_Resolve,	1},
	{"GetInfo",	(PyCFunction)mfsa_GetInfo,	1},
	{"Update",	(PyCFunction)mfsa_Update,	1},
 
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */

static PyObject *
mfsa_getattr(self, name)
	mfsaobject *self;
	char *name;
{
	if ( strcmp(name, "data") == 0 ) {
		int size;
		PyObject *rv;
		
		size = GetHandleSize((Handle)self->alias);
		HLock((Handle)self->alias);
		rv = PyString_FromStringAndSize(*(Handle)self->alias, size);
		HUnlock((Handle)self->alias);
		return rv;
	}
	return Py_FindMethod(mfsa_methods, (PyObject *)self, name);
}

mfsaobject *
newmfsaobject(alias)
	AliasHandle alias;
{
	mfsaobject *self;

	self = PyObject_NEW(mfsaobject, &Mfsatype);
	if (self == NULL)
		return NULL;
	self->alias = alias;
	return self;
}


static void
mfsa_dealloc(self)
	mfsaobject *self;
{
#if 0
	if ( self->alias ) {
		should we do something here?
	}
#endif
		
	PyMem_DEL(self);
}

statichere PyTypeObject Mfsatype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"Alias",			/*tp_name*/
	sizeof(mfsaobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)mfsa_dealloc,	/*tp_dealloc*/
	(printfunc)0,			/*tp_print*/
	(getattrfunc)mfsa_getattr,	/*tp_getattr*/
	(setattrfunc)0,			/*tp_setattr*/
	(cmpfunc)0,			/*tp_compare*/
	(reprfunc)0,			/*tp_repr*/
	0,				/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	(hashfunc)0,			/*tp_hash*/
};

/* End of code for Alias objects */
/* -------------------------------------------------------- */

/* ---------------------------------------------------------------- */

static struct PyMethodDef mfsi_methods[] = {
	
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */

static mfsiobject *
newmfsiobject()
{
	mfsiobject *self;
	
	self = PyObject_NEW(mfsiobject, &Mfsitype);
	if (self == NULL)
		return NULL;
	memset((char *)&self->finfo, '\0', sizeof(self->finfo));
	return self;
}

static void
mfsi_dealloc(self)
	mfsiobject *self;
{
	PyMem_DEL(self);
}

static PyObject *
mfsi_getattr(self, name)
	mfsiobject *self;
	char *name;
{
	if ( strcmp(name, "Type") == 0 )
		return PyMac_BuildOSType(self->finfo.fdType);
	else if ( strcmp(name, "Creator") == 0 )
		return PyMac_BuildOSType(self->finfo.fdCreator);
	else if ( strcmp(name, "Flags") == 0 )
		return Py_BuildValue("i", (int)self->finfo.fdFlags);
	else if ( strcmp(name, "Location") == 0 )
		return PyMac_BuildPoint(self->finfo.fdLocation);
	else if ( strcmp(name, "Fldr") == 0 )
		return Py_BuildValue("i", (int)self->finfo.fdFldr);
	else
		return Py_FindMethod(mfsi_methods, (PyObject *)self, name);
}


static int
mfsi_setattr(self, name, v)
	mfsiobject *self;
	char *name;
	PyObject *v;
{
	int rv;
	int i;
	
	if ( v == NULL ) {
		PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
		return -1;
	}
	if ( strcmp(name, "Type") == 0 )
		rv = PyMac_GetOSType(v, &self->finfo.fdType);
	else if ( strcmp(name, "Creator") == 0 )
		rv = PyMac_GetOSType(v, &self->finfo.fdCreator);
	else if ( strcmp(name, "Flags") == 0 ) {
		rv = PyArg_Parse(v, "i", &i);
		self->finfo.fdFlags = (short)i;
	} else if ( strcmp(name, "Location") == 0 )
		rv = PyMac_GetPoint(v, &self->finfo.fdLocation);
	else if ( strcmp(name, "Fldr") == 0 ) {
		rv = PyArg_Parse(v, "i", &i);
		self->finfo.fdFldr = (short)i;
	} else {
		PyErr_SetString(PyExc_AttributeError, "No such attribute");
		return -1;
	}
	if (rv)
		return 0;
	return -1;
}


static PyTypeObject Mfsitype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"FInfo",			/*tp_name*/
	sizeof(mfsiobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)mfsi_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)mfsi_getattr,	/*tp_getattr*/
	(setattrfunc)mfsi_setattr,	/*tp_setattr*/
	(cmpfunc)0,		/*tp_compare*/
	(reprfunc)0,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
};

/* End of code for FInfo object objects */
/* -------------------------------------------------------- */


/*
** Helper routine for other modules: return an FSSpec * if the
** object is a python fsspec object, else NULL
*/
FSSpec *
mfs_GetFSSpecFSSpec(self)
	PyObject *self;
{
	if ( is_mfssobject(self) )
		return &((mfssobject *)self)->fsspec;
	return NULL;
}

/*
** Two generally useful routines
*/
static OSErr
PyMac_GetFileDates(fss, crdat, mddat, bkdat)
	FSSpec *fss;
	unsigned long *crdat, *mddat, *bkdat;
{
	CInfoPBRec pb;
	OSErr error;
	
	pb.dirInfo.ioNamePtr = fss->name;
	pb.dirInfo.ioFDirIndex = 0;
	pb.dirInfo.ioVRefNum = fss->vRefNum;
	pb.dirInfo.ioDrDirID = fss->parID;
	error = PBGetCatInfoSync(&pb);
	if ( error ) return error;
	*crdat = pb.hFileInfo.ioFlCrDat;
	*mddat = pb.hFileInfo.ioFlMdDat;
	*bkdat = pb.hFileInfo.ioFlBkDat;
	return 0;
}	

static OSErr
PyMac_SetFileDates(fss, crdat, mddat, bkdat)
	FSSpec *fss;
	unsigned long crdat, mddat, bkdat;
{
	CInfoPBRec pb;
	OSErr error;
	
	pb.dirInfo.ioNamePtr = fss->name;
	pb.dirInfo.ioFDirIndex = 0;
	pb.dirInfo.ioVRefNum = fss->vRefNum;
	pb.dirInfo.ioDrDirID = fss->parID;
	error = PBGetCatInfoSync(&pb);
	if ( error ) return error;
	pb.dirInfo.ioNamePtr = fss->name;
	pb.dirInfo.ioFDirIndex = 0;
	pb.dirInfo.ioVRefNum = fss->vRefNum;
	pb.dirInfo.ioDrDirID = fss->parID;
	pb.hFileInfo.ioFlCrDat = crdat;
	pb.hFileInfo.ioFlMdDat = mddat;
	pb.hFileInfo.ioFlBkDat = bkdat;
	error = PBSetCatInfoSync(&pb);
	return error;
}

static PyObject *
mfss_as_pathname(self, args)
	mfssobject *self;
	PyObject *args;
{
	char strbuf[257];
	OSErr err;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = PyMac_GetFullPath(&self->fsspec, strbuf);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return PyString_FromString(strbuf);
}

static PyObject *
mfss_as_tuple(self, args)
	mfssobject *self;
	PyObject *args;
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("(iis#)", self->fsspec.vRefNum, self->fsspec.parID, 
						&self->fsspec.name[1], self->fsspec.name[0]);
}

static PyObject *
mfss_NewAlias(self, args)
	mfssobject *self;
	PyObject *args;
{
	FSSpec src, *srcp;
	OSErr err;
	AliasHandle alias;
	
	src.name[0] = 0;
	if (!PyArg_ParseTuple(args, "|O&", PyMac_GetFSSpec, &src))
		return NULL;
	if ( src.name[0] )
		srcp = &src;
	else
		srcp = NULL;
	err = NewAlias(srcp, &self->fsspec, &alias);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	
	return (PyObject *)newmfsaobject(alias);
}

static PyObject *
mfss_NewAliasMinimal(self, args)
	mfssobject *self;
	PyObject *args;
{
	OSErr err;
	AliasHandle alias;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = NewAliasMinimal(&self->fsspec, &alias);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newmfsaobject(alias);
}

/* XXXX These routines should be replaced by a wrapper to the *FInfo routines */
static PyObject *
mfss_GetCreatorType(self, args)
	mfssobject *self;
	PyObject *args;
{
	OSErr err;
	FInfo info;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = FSpGetFInfo(&self->fsspec, &info);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("(O&O&)",
	           PyMac_BuildOSType, info.fdCreator, PyMac_BuildOSType, info.fdType);
}

static PyObject *
mfss_SetCreatorType(self, args)
	mfssobject *self;
	PyObject *args;
{
	OSErr err;
	OSType creator, type;
	FInfo info;
	
	if (!PyArg_ParseTuple(args, "O&O&", PyMac_GetOSType, &creator, PyMac_GetOSType, &type))
		return NULL;
	err = FSpGetFInfo(&self->fsspec, &info);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	info.fdType = type;
	info.fdCreator = creator;
	err = FSpSetFInfo(&self->fsspec, &info);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
mfss_GetFInfo(self, args)
	mfssobject *self;
	PyObject *args;
{
	OSErr err;
	mfsiobject *fip;
	
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	if ( (fip=newmfsiobject()) == NULL )
		return NULL;
	err = FSpGetFInfo(&self->fsspec, &fip->finfo);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		Py_DECREF(fip);
		return NULL;
	}
	return (PyObject *)fip;
}

static PyObject *
mfss_SetFInfo(self, args)
	mfssobject *self;
	PyObject *args;
{
	OSErr err;
	mfsiobject *fip;
	
	if (!PyArg_ParseTuple(args, "O!", &Mfsitype, &fip))
		return NULL;
	err = FSpSetFInfo(&self->fsspec, &fip->finfo);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
mfss_GetDates(self, args)
	mfssobject *self;
	PyObject *args;
{
	OSErr err;
	unsigned long crdat, mddat, bkdat;
	
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	err = PyMac_GetFileDates(&self->fsspec, &crdat, &mddat, &bkdat);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("ddd", (double)crdat, (double)mddat, (double)bkdat);
}

static PyObject *
mfss_SetDates(self, args)
	mfssobject *self;
	PyObject *args;
{
	OSErr err;
	double crdat, mddat, bkdat;
	
	if (!PyArg_ParseTuple(args, "ddd", &crdat, &mddat, &bkdat))
		return NULL;
	err = PyMac_SetFileDates(&self->fsspec, (unsigned long)crdat, 
				(unsigned long)mddat, (unsigned long)bkdat);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef mfss_methods[] = {
	{"as_pathname",		(PyCFunction)mfss_as_pathname,			1},
	{"as_tuple",		(PyCFunction)mfss_as_tuple,				1},
	{"NewAlias",		(PyCFunction)mfss_NewAlias,				1},
	{"NewAliasMinimal",	(PyCFunction)mfss_NewAliasMinimal,		1},
	{"GetCreatorType",	(PyCFunction)mfss_GetCreatorType,		1},
	{"SetCreatorType",	(PyCFunction)mfss_SetCreatorType,		1},
	{"GetFInfo",		(PyCFunction)mfss_GetFInfo,				1},
	{"SetFInfo",		(PyCFunction)mfss_SetFInfo,				1},
	{"GetDates",		(PyCFunction)mfss_GetDates,				1},
	{"SetDates",		(PyCFunction)mfss_SetDates,				1},
 
	{NULL,			NULL}		/* sentinel */
};

/* ---------- */

static PyObject *
mfss_getattr(self, name)
	mfssobject *self;
	char *name;
{
	if ( strcmp(name, "data") == 0)
		return PyString_FromStringAndSize((char *)&self->fsspec, sizeof(FSSpec));	
	return Py_FindMethod(mfss_methods, (PyObject *)self, name);
}

mfssobject *
newmfssobject(fss)
	FSSpec *fss;
{
	mfssobject *self;
	
	self = PyObject_NEW(mfssobject, &Mfsstype);
	if (self == NULL)
		return NULL;
	self->fsspec = *fss;
	return self;
}

static void
mfss_dealloc(self)
	mfssobject *self;
{
	PyMem_DEL(self);
}

static PyObject *
mfss_repr(self)
	mfssobject *self;
{
	char buf[512];

	sprintf(buf, "FSSpec((%d, %d, '%.*s'))",
		self->fsspec.vRefNum, 
		self->fsspec.parID,
		self->fsspec.name[0], self->fsspec.name+1);
	return PyString_FromString(buf);
}

static int
mfss_compare(v, w)
	mfssobject *v, *w;
{
	int minlen;
	int res;
	
	if ( v->fsspec.vRefNum < w->fsspec.vRefNum ) return -1;
	if ( v->fsspec.vRefNum > w->fsspec.vRefNum ) return 1;
	if ( v->fsspec.parID < w->fsspec.parID ) return -1;
	if ( v->fsspec.parID > w->fsspec.parID ) return 1;
	minlen = v->fsspec.name[0];
	if ( w->fsspec.name[0] < minlen ) minlen = w->fsspec.name[0];
	res = strncmp((char *)v->fsspec.name+1, (char *)w->fsspec.name+1, minlen);
	if ( res ) return res;
	if ( v->fsspec.name[0] < w->fsspec.name[0] ) return -1;
	if ( v->fsspec.name[0] > w->fsspec.name[0] ) return 1;
	return res;
}

statichere PyTypeObject Mfsstype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"FSSpec",			/*tp_name*/
	sizeof(mfssobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)mfss_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)mfss_getattr,	/*tp_getattr*/
	(setattrfunc)0,	/*tp_setattr*/
	(cmpfunc)mfss_compare,		/*tp_compare*/
	(reprfunc)mfss_repr,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
};

/* End of code for FSSpec objects */
/* -------------------------------------------------------- */

static PyObject *
mfs_ResolveAliasFile(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	FSSpec fss;
	Boolean chain = 1, isfolder, wasaliased;
	OSErr err;

	if (!PyArg_ParseTuple(args, "O&|i", PyMac_GetFSSpec, &fss, &chain))
		return NULL;
	err = ResolveAliasFile(&fss, chain, &isfolder, &wasaliased);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("Oii", newmfssobject(&fss), (int)isfolder, (int)wasaliased);
}

static PyObject *
mfs_StandardGetFile(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	SFTypeList list;
	short numtypes;
	StandardFileReply reply;
	
	list[0] = list[1] = list[2] = list[3] = 0;
	numtypes = 0;
	if (!PyArg_ParseTuple(args, "|O&O&O&O&", PyMac_GetOSType, &list[0],
			 PyMac_GetOSType, &list[1], PyMac_GetOSType, &list[2],
			  PyMac_GetOSType, &list[3]) )
		return NULL;
	while ( numtypes < 4 && list[numtypes] ) {
		numtypes++;
	}
	if ( numtypes == 0 )
		numtypes = -1;
	StandardGetFile((FileFilterUPP)0, numtypes, list, &reply);
	return Py_BuildValue("(Oi)", newmfssobject(&reply.sfFile), reply.sfGood);
}

static PyObject *
mfs_PromptGetFile(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	SFTypeList list;
	short numtypes;
	StandardFileReply reply;
	char *prompt = NULL;
	
	list[0] = list[1] = list[2] = list[3] = 0;
	numtypes = 0;
	if (!PyArg_ParseTuple(args, "s|O&O&O&O&", &prompt, PyMac_GetOSType, &list[0],
			 PyMac_GetOSType, &list[1], PyMac_GetOSType, &list[2],
			  PyMac_GetOSType, &list[3]) )
		return NULL;
	while ( numtypes < 4 && list[numtypes] ) {
		numtypes++;
	}
	if ( numtypes == 0 )
		numtypes = -1;
	PyMac_PromptGetFile(numtypes, list, &reply, prompt);
	return Py_BuildValue("(Oi)", newmfssobject(&reply.sfFile), reply.sfGood);
}

static PyObject *
mfs_StandardPutFile(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	Str255 prompt, dft;
	StandardFileReply reply;
	
	dft[0] = 0;
	if (!PyArg_ParseTuple(args, "O&|O&", PyMac_GetStr255, &prompt, PyMac_GetStr255, &dft) )
		return NULL;
	StandardPutFile(prompt, dft, &reply);
	return Py_BuildValue("(Oi)",newmfssobject(&reply.sfFile), reply.sfGood);
}

/*
** Set initial directory for file dialogs */
static PyObject *
mfs_SetFolder(self, args)
	PyObject *self;
	PyObject *args;
{
	FSSpec spec;
	FSSpec ospec;
	short orefnum;
	long oparid;
	
	/* Get old values */
	orefnum = -LMGetSFSaveDisk();
	oparid = LMGetCurDirStore();
	(void)FSMakeFSSpec(orefnum, oparid, "\pplaceholder", &ospec);
	
	/* Go to working directory by default */
	(void)FSMakeFSSpec(0, 0, "\p:placeholder", &spec);
	if (!PyArg_ParseTuple(args, "|O&", PyMac_GetFSSpec, &spec))
		return NULL;
	/* Set standard-file working directory */
	LMSetSFSaveDisk(-spec.vRefNum);
	LMSetCurDirStore(spec.parID);
	return (PyObject *)newmfssobject(&ospec);
}

static PyObject *
mfs_FSSpec(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	FSSpec fss;

	if (!PyArg_ParseTuple(args, "O&", PyMac_GetFSSpec, &fss))
		return NULL;
	return (PyObject *)newmfssobject(&fss);
}

static PyObject *
mfs_RawFSSpec(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	FSSpec *fssp;
	int size;

	if (!PyArg_ParseTuple(args, "s#", &fssp, &size))
		return NULL;
	if ( size != sizeof(FSSpec) ) {
		PyErr_SetString(PyExc_TypeError, "Incorrect size for FSSpec record");
		return NULL;
	}
	return (PyObject *)newmfssobject(fssp);
}

static PyObject *
mfs_RawAlias(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	char *dataptr;
	Handle h;
	int size;

	if (!PyArg_ParseTuple(args, "s#", &dataptr, &size))
		return NULL;
	h = NewHandle(size);
	if ( h == NULL ) {
		PyErr_NoMemory();
		return NULL;
	}
	HLock(h);
	memcpy((char *)*h, dataptr, size);
	HUnlock(h);
	return (PyObject *)newmfsaobject((AliasHandle)h);
}

static PyObject *
mfs_GetDirectory(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	FSSpec fsdir;
	int ok;
	char *prompt = NULL;
		
	if (!PyArg_ParseTuple(args, "|s", &prompt) )
		return NULL;
		
	ok = PyMac_GetDirectory(&fsdir, prompt);
	return Py_BuildValue("(Oi)", newmfssobject(&fsdir), ok);
}

static PyObject *
mfs_FindFolder(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	OSErr err;
	short where;
	OSType which;
	int create;
	short refnum;
	long dirid;
		
	if (!PyArg_ParseTuple(args, "hO&i", &where, PyMac_GetOSType, &which, &create) )
		return NULL;
	err = FindFolder(where, which, (Boolean)create, &refnum, &dirid);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return Py_BuildValue("(ii)", refnum, dirid);
}

static PyObject *
mfs_FindApplication(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	OSErr err;
	OSType which;
	FSSpec	fss;
		
	if (!PyArg_ParseTuple(args, "O&", PyMac_GetOSType, &which) )
		return NULL;
	err = FindApplicationFromCreator(which, &fss);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newmfssobject(&fss);
}

static PyObject *
mfs_FInfo(self, args)
	PyObject *self;
	PyObject *args;
{	
	return (PyObject *)newmfsiobject();
}

static PyObject *
mfs_NewAliasMinimalFromFullPath(self, args)
	PyObject *self;	/* Not used */
	PyObject *args;
{
	OSErr err;
	char *fullpath;
	int fullpathlen;
	AliasHandle alias;
	Str32 zonename;
	Str31 servername;
			
	if (!PyArg_ParseTuple(args, "s#", &fullpath, &fullpathlen) )
		return NULL;
	zonename[0] = 0;
	servername[0] = 0;
	err = NewAliasMinimalFromFullPath(fullpathlen, (Ptr)fullpath, zonename, 
			servername, &alias);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (PyObject *)newmfsaobject(alias);
}


/* List of methods defined in the module */

static struct PyMethodDef mfs_methods[] = {
	{"ResolveAliasFile",	mfs_ResolveAliasFile,	1},
	{"StandardGetFile",		mfs_StandardGetFile,	1},
	{"PromptGetFile",		mfs_PromptGetFile,		1},
	{"StandardPutFile",		mfs_StandardPutFile,	1},
	{"GetDirectory",		mfs_GetDirectory,		1},
	{"SetFolder",			mfs_SetFolder,			1},
	{"FSSpec",				mfs_FSSpec,				1},
	{"RawFSSpec",			mfs_RawFSSpec,			1},
	{"RawAlias",			mfs_RawAlias,			1},
	{"FindFolder",			mfs_FindFolder,			1},
	{"FindApplication",		mfs_FindApplication,	1},
	{"FInfo",				mfs_FInfo,				1},
	{"NewAliasMinimalFromFullPath",	mfs_NewAliasMinimalFromFullPath,	1},
 
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initmacfs) */

void
initmacfs()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule("macfs", mfs_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyErr_NewException("macfs.error", NULL, NULL);
	PyDict_SetItemString(d, "error", ErrorObject);

	Mfsatype.ob_type = &PyType_Type;
	Py_INCREF(&Mfsatype);
	PyDict_SetItemString(d, "AliasType", (PyObject *)&Mfsatype);
	Mfsstype.ob_type = &PyType_Type;
	Py_INCREF(&Mfsstype);
	PyDict_SetItemString(d, "FSSpecType", (PyObject *)&Mfsstype);
	Mfsitype.ob_type = &PyType_Type;
	Py_INCREF(&Mfsitype);
	PyDict_SetItemString(d, "FInfoType", (PyObject *)&Mfsitype);
	/* XXXX Add constants here */
}
