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

#include "allobjects.h"
#include "modsupport.h"		/* For getargs() etc. */
#include "macglue.h"

#include <Files.h>
#include <StandardFile.h>
#include <Aliases.h>

#include "nfullpath.h"

#ifdef THINK_C
#define FileFilterUPP FileFilterProcPtr
#endif


static object *ErrorObject;

/* ----------------------------------------------------- */
/* Declarations for objects of type Alias */

typedef struct {
	OB_HEAD
	AliasHandle alias;
} mfsaobject;

staticforward typeobject Mfsatype;

#define is_mfsaobject(v)		((v)->ob_type == &Mfsatype)

/* ---------------------------------------------------------------- */
/* Declarations for objects of type FSSpec */

typedef struct {
	OB_HEAD
	FSSpec fsspec;
} mfssobject;

staticforward typeobject Mfsstype;

#define is_mfssobject(v)		((v)->ob_type == &Mfsstype)


mfssobject *newmfssobject(FSSpec *fss); /* Forward */

/* ---------------------------------------------------------------- */

static object *
mfsa_Resolve(self, args)
	mfsaobject *self;
	object *args;
{
	FSSpec from, *fromp, result;
	Boolean changed;
	OSErr err;
	
	from.name[0] = 0;
	if (!newgetargs(args, "|O&", PyMac_GetFSSpec, &from))
		return NULL;
	if (from.name[0] )
		fromp = &from;
	else
		fromp = NULL;
	err = ResolveAlias(fromp, self->alias, &result, &changed);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return mkvalue("(Oi)", newmfssobject(&result), (int)changed);
}

static object *
mfsa_GetInfo(self, args)
	mfsaobject *self;
	object *args;
{
	Str63 value;
	int i;
	OSErr err;
	
	if (!newgetargs(args, "i", &i))
		return NULL;
	err = GetAliasInfo(self->alias, (AliasInfoType)i, value);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return 0;
	}
	return newsizedstringobject((char *)&value[1], value[0]);
}

static object *
mfsa_Update(self, args)
	mfsaobject *self;
	object *args;
{
	FSSpec target, fromfile, *fromfilep;
	OSErr err;
	Boolean changed;
	
	fromfile.name[0] = 0;
	if (!newgetargs(args, "O&|O&",  PyMac_GetFSSpec, &target,
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
	return mkvalue("i", (int)changed);
}

static struct methodlist mfsa_methods[] = {
	{"Resolve",	(method)mfsa_Resolve,	1},
	{"GetInfo",	(method)mfsa_GetInfo,	1},
	{"Update",	(method)mfsa_Update,	1},
 
	{NULL,		NULL}		/* sentinel */
};

/* ---------- */

static object *
mfsa_getattr(self, name)
	mfsaobject *self;
	char *name;
{
	return findmethod(mfsa_methods, (object *)self, name);
}

mfsaobject *
newmfsaobject(alias)
	AliasHandle alias;
{
	mfsaobject *self;

	self = NEWOBJ(mfsaobject, &Mfsatype);
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
		
	DEL(self);
}

static typeobject Mfsatype = {
	OB_HEAD_INIT(&Typetype)
	0,				/*ob_size*/
	"Alias",			/*tp_name*/
	sizeof(mfsaobject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)mfsa_dealloc,	/*tp_dealloc*/
	(printfunc)0,		/*tp_print*/
	(getattrfunc)mfsa_getattr,	/*tp_getattr*/
	(setattrfunc)0,	/*tp_setattr*/
	(cmpfunc)0,		/*tp_compare*/
	(reprfunc)0,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)0,		/*tp_hash*/
};

/* End of code for Alias objects */
/* -------------------------------------------------------- */

/*
** Helper routine for other modules: return an FSSpec * if the
** object is a python fsspec object, else NULL
*/
FSSpec *
mfs_GetFSSpecFSSpec(self)
	object *self;
{
	if ( is_mfssobject(self) )
		return &((mfssobject *)self)->fsspec;
	return NULL;
}

static object *
mfss_as_pathname(self, args)
	mfssobject *self;
	object *args;
{
	char strbuf[257];
	OSErr err;

	if (!newgetargs(args, ""))
		return NULL;
	err = nfullpath(&self->fsspec, strbuf);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return newstringobject(strbuf);
}

static object *
mfss_as_tuple(self, args)
	mfssobject *self;
	object *args;
{
	if (!newgetargs(args, ""))
		return NULL;
	return Py_BuildValue("(iis#)", self->fsspec.vRefNum, self->fsspec.parID, 
						&self->fsspec.name[1], self->fsspec.name[0]);
}

static object *
mfss_NewAlias(self, args)
	mfssobject *self;
	object *args;
{
	FSSpec src, *srcp;
	OSErr err;
	AliasHandle alias;
	
	src.name[0] = 0;
	if (!newgetargs(args, "|O&", PyMac_GetFSSpec, &src))
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
	
	return (object *)newmfsaobject(alias);
}

static object *
mfss_NewAliasMinimal(self, args)
	mfssobject *self;
	object *args;
{
	OSErr err;
	AliasHandle alias;
	
	if (!newgetargs(args, ""))
		return NULL;
	err = NewAliasMinimal(&self->fsspec, &alias);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return (object *)newmfsaobject(alias);
}

static struct methodlist mfss_methods[] = {
	{"as_pathname",		(method)mfss_as_pathname,			1},
	{"as_tuple",		(method)mfss_as_tuple,				1},
	{"NewAlias",		(method)mfss_NewAlias,				1},
	{"NewAliasMinimal",	(method)mfss_NewAliasMinimal,		1},
 
	{NULL,			NULL}		/* sentinel */
};

/* ---------- */

static object *
mfss_getattr(self, name)
	mfssobject *self;
	char *name;
{
	return findmethod(mfss_methods, (object *)self, name);
}

mfssobject *
newmfssobject(fss)
	FSSpec *fss;
{
	mfssobject *self;
	
	self = NEWOBJ(mfssobject, &Mfsstype);
	if (self == NULL)
		return NULL;
	self->fsspec = *fss;
	return self;
}

static void
mfss_dealloc(self)
	mfssobject *self;
{
	DEL(self);
}

static object *
mfss_repr(self)
	mfssobject *self;
{
	char buf[512];

	/* XXXX Does %#s work for all compilers? */
	sprintf(buf, "FSSpec((%d, %d, '%#s'))", self->fsspec.vRefNum, 
				self->fsspec.parID, self->fsspec.name);
	return newstringobject(buf);
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

static typeobject Mfsstype = {
	OB_HEAD_INIT(&Typetype)
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

static object *
mfs_ResolveAliasFile(self, args)
	object *self;	/* Not used */
	object *args;
{
	FSSpec fss;
	Boolean chain = 1, isfolder, wasaliased;
	OSErr err;

	if (!newgetargs(args, "O&|i", PyMac_GetFSSpec, &fss, &chain))
		return NULL;
	err = ResolveAliasFile(&fss, chain, &isfolder, &wasaliased);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return mkvalue("Oii", newmfssobject(&fss), (int)isfolder, (int)wasaliased);
}

static object *
mfs_StandardGetFile(self, args)
	object *self;	/* Not used */
	object *args;
{
	char *list[4];
	SFTypeList typelist;
	short numtypes;
	StandardFileReply reply;
	
	list[0] = list[1] = list[2] = list[3] = 0;
	numtypes = 0;
#if 0
	/* XXXX Why doesn't this work? */
	if (!newgetargs(args, "|O&|O&|O&|O&", PyMac_GetOSType, &list[0],
			 PyMac_GetOSType, &list[1], PyMac_GetOSType, &list[2],
			  PyMac_GetOSType, &list[3]) )
		return NULL;
#else
	if (!newgetargs(args, "|O&", PyMac_GetOSType, &list[0]) )
		return NULL;
#endif
	while ( list[numtypes] && numtypes < 4 ) {
		numtypes++;
	}
	StandardGetFile((FileFilterUPP)0, numtypes, typelist, &reply);
	return mkvalue("(Oi)", newmfssobject(&reply.sfFile), reply.sfGood);
}

static object *
mfs_StandardPutFile(self, args)
	object *self;	/* Not used */
	object *args;
{
	Str255 prompt, dft;
	StandardFileReply reply;
	
	dft[0] = 0;
	if (!newgetargs(args, "O&|O&", PyMac_GetStr255, &prompt, PyMac_GetStr255, &dft) )
		return NULL;
	StandardPutFile(prompt, dft, &reply);
	return mkvalue("(Oi)",newmfssobject(&reply.sfFile), reply.sfGood);
}

static object *
mfs_FSSpec(self, args)
	object *self;	/* Not used */
	object *args;
{
	FSSpec fss;

	if (!newgetargs(args, "O&", PyMac_GetFSSpec, &fss))
		return NULL;
	return (object *)newmfssobject(&fss);
}

/* List of methods defined in the module */

static struct methodlist mfs_methods[] = {
	{"ResolveAliasFile",	mfs_ResolveAliasFile,	1},
	{"StandardGetFile",		mfs_StandardGetFile,	1},
	{"StandardPutFile",		mfs_StandardPutFile,	1},
	{"FSSpec",				mfs_FSSpec,				1},
 
	{NULL,		NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initmacfs) */

void
initmacfs()
{
	object *m, *d;

	/* Create the module and add the functions */
	m = initmodule("macfs", mfs_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	ErrorObject = newstringobject("macfs.error");
	dictinsert(d, "error", ErrorObject);

	/* XXXX Add constants here */
	
	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module macfs");
}
