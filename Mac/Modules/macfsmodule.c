/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

static object *
mfs_NewAlias(self, args)
	object *self;	/* Not used */
	object *args;
{
	FSSpec src, dst, *dstptr;
	
	src.name[0] = 0;
	if (!newgetargs(args, "O&|O&", PyMac_GetFSSpec, &dst, PyMac_GetFSSpec, &src))
		return NULL;
		
	/* XXXX */

	INCREF(None);
	return None;
}

static object *
mfs_ResolveAlias(self, args)
	object *self;	/* Not used */
	object *args;
{

	if (!newgetargs(args, ""))
		return NULL;
	INCREF(None);
	return None;
}

static object *
mfs_ResolveAliasFile(self, args)
	object *self;	/* Not used */
	object *args;
{

	if (!newgetargs(args, ""))
		return NULL;
	INCREF(None);
	return None;
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
	return mkvalue("(iO)", reply.sfGood, PyMac_BuildFSSpec(&reply.sfFile));
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
	/* XXXX I don't understand newgetargs, why doesn't |s|s|s|s work? */
	if (!newgetargs(args, "|s", &list[0] /*, &list[1], &list[2], &list[3]*/) )
		return NULL;
	while ( list[numtypes] && numtypes < 4 ) {
		memcpy((char *)&typelist[numtypes], list[numtypes], 4);
		numtypes++;
	}
	StandardGetFile((FileFilterUPP)0, numtypes, typelist, &reply);
	return mkvalue("(iO)", reply.sfGood, PyMac_BuildFSSpec(&reply.sfFile));
}

static object *
mfs_FSSpecNormalize(self, args)
	object *self;	/* Not used */
	object *args;
{
	FSSpec fss;

	if (!newgetargs(args, "O&", PyMac_GetFSSpec, &fss))
		return NULL;
	return PyMac_BuildFSSpec(&fss);
}

static object *
mfs_FSSpecPath(self, args)
	object *self;	/* Not used */
	object *args;
{
	FSSpec fss;
	char strbuf[257];
	OSErr err;

	if (!newgetargs(args, "O&", PyMac_GetFSSpec, &fss))
		return NULL;
	err = nfullpath(&fss, strbuf);
	if ( err ) {
		PyErr_Mac(ErrorObject, err);
		return NULL;
	}
	return newstringobject(strbuf);
}

/* List of methods defined in the module */

static struct methodlist mfs_methods[] = {
	{"NewAlias",		mfs_NewAlias,			1},
	{"ResolveAlias",	mfs_ResolveAlias,		1},
	{"ResolveAliasFile",mfs_ResolveAliasFile,	1},
	{"StandardPutFile",	mfs_StandardPutFile,	1},
	{"StandardGetFile",	mfs_StandardGetFile,	1},
	{"FSSpecNormalize",	mfs_FSSpecNormalize,	1},
	{"FSSpecPath",		mfs_FSSpecPath,	1},
 
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
