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
#include "marshal.h"
#include "import.h"
#include "importdl.h"

#include "pythonresources.h"

#include <Types.h>
#include <Files.h>
#include <Resources.h>
#if 0
#include <OSUtils.h> /* for Set(Current)A5 */
#include <StandardFile.h>
#include <Memory.h>
#include <Windows.h>
#include <Traps.h>
#include <Processes.h>
#include <Fonts.h>
#include <Menus.h>
#include <TextUtils.h>
#endif
#include <CodeFragments.h>
#include <StringCompare.h>

#ifdef USE_GUSI1
#include "TFileSpec.h"	/* for Path2FSSpec() */
#endif

typedef void (*dl_funcptr)();
#define FUNCNAME_PATTERN "init%.200s"

static int
fssequal(FSSpec *fs1, FSSpec *fs2)
{
	if ( fs1->vRefNum != fs2->vRefNum || fs1->parID != fs2->parID )
		return 0;
	return EqualString(fs1->name, fs2->name, false, true);
}
/*
** findnamedresource - Common code for the various *ResourceModule functions.
** Check whether a file contains a resource of the correct name and type, and
** optionally return the value in it.
*/
static int
findnamedresource(
	PyStringObject *obj, 
	char *module, 
	char *filename, 
	OSType restype, 
	StringPtr dataptr)
{
	FSSpec fss;
	FInfo finfo;
	short oldrh, filerh;
	int ok;
	Handle h;

#ifdef INTERN_STRINGS
	/*
	** If we have interning find_module takes care of interning all
	** sys.path components. We then keep a record of all sys.path
	** components for which GetFInfo has failed (usually because the
	** component in question is a folder), and we don't try opening these
	** as resource files again.
	*/
#define MAXPATHCOMPONENTS 32
	static PyStringObject *not_a_file[MAXPATHCOMPONENTS];
	static int max_not_a_file = 0;
	int i;
		
	if (obj && obj->ob_sinterned ) {
		for( i=0; i< max_not_a_file; i++ )
			if ( obj == not_a_file[i] )
				return 0;
	}
#endif /* INTERN_STRINGS */
#ifdef USE_GUSI1
	if ( Path2FSSpec(filename, &fss) != noErr ) {
#else
	if ( FSMakeFSSpec(0, 0, Pstring(filename), &fss) != noErr ) {
#endif
#ifdef INTERN_STRINGS
		if ( obj && max_not_a_file < MAXPATHCOMPONENTS && obj->ob_sinterned )
			not_a_file[max_not_a_file++] = obj;
#endif /* INTERN_STRINGS */
	     	/* doesn't exist or is folder */
		return 0;
	}			
	if ( fssequal(&fss, &PyMac_ApplicationFSSpec) ) {
		/*
		** Special case: the application itself. Use a shortcut to
		** forestall opening and closing the application numerous times
		** (which is dead slow when running from CDROM)
		*/
		oldrh = CurResFile();
		UseResFile(PyMac_AppRefNum);
		filerh = -1;
	} else {
#ifdef INTERN_STRINGS
	     	if ( FSpGetFInfo(&fss, &finfo) != noErr ) {
			if ( obj && max_not_a_file < MAXPATHCOMPONENTS && obj->ob_sinterned )
				not_a_file[max_not_a_file++] = obj;
	     		/* doesn't exist or is folder */
			return 0;
		}			
#endif /* INTERN_STRINGS */
		oldrh = CurResFile();
		filerh = FSpOpenResFile(&fss, fsRdPerm);
		if ( filerh == -1 )
			return 0;
		UseResFile(filerh);
	}
	if ( dataptr == NULL )
		SetResLoad(0);
	h = Get1NamedResource(restype, Pstring(module));
	SetResLoad(1);
	ok = (h != NULL);
	if ( ok && dataptr != NULL ) {
		HLock(h);
		/* XXXX Unsafe if resource not correctly formatted! */
#ifdef __CFM68K__
		/* for cfm68k we take the second pstring */
		*dataptr = *((*h)+(**h)+1);
		memcpy(dataptr+1, (*h)+(**h)+2, (int)*dataptr);
#else
		/* for ppc we take the first pstring */
		*dataptr = **h;
		memcpy(dataptr+1, (*h)+1, (int)*dataptr);
#endif
		HUnlock(h);
	}
	if ( filerh != -1 )
		CloseResFile(filerh);
	UseResFile(oldrh);
	return ok;
}

/*
** Returns true if the argument has a resource fork, and it contains
** a 'PYC ' resource of the correct name
*/
int
PyMac_FindResourceModule(obj, module, filename)
PyStringObject *obj;
char *module;
char *filename;
{
	int ok;
	
	ok = findnamedresource(obj, module, filename, 'PYC ', (StringPtr)0);
	return ok;
}

/*
** Returns true if the argument has a resource fork, and it contains
** a 'PYD ' resource of the correct name
*/
int
PyMac_FindCodeResourceModule(obj, module, filename)
PyStringObject *obj;
char *module;
char *filename;
{
	int ok;
	
	ok = findnamedresource(obj, module, filename, 'PYD ', (StringPtr)0);
	return ok;
}


/*
** Load the specified module from a code resource
*/
PyObject *
PyMac_LoadCodeResourceModule(name, pathname)
	char *name;
	char *pathname;
{
	PyObject *m, *d, *s;
	char funcname[258];
	char *lastdot, *shortname, *packagecontext;
	dl_funcptr p = NULL;
	Str255 fragmentname;
	CFragConnectionID connID;
	Ptr mainAddr;
	Str255 errMessage;
	OSErr err;
	char buf[512];
	Ptr symAddr;
	CFragSymbolClass class;

	if ((m = _PyImport_FindExtension(name, name)) != NULL) {
		Py_INCREF(m);
		return m;
	}
	lastdot = strrchr(name, '.');
	if (lastdot == NULL) {
		packagecontext = NULL;
		shortname = name;
	}
	else {
		packagecontext = name;
		shortname = lastdot+1;
	}
	sprintf(funcname, FUNCNAME_PATTERN, shortname);
	if( !findnamedresource((PyStringObject *)0, name, pathname, 'PYD ', fragmentname)) {
		PyErr_SetString(PyExc_ImportError, "PYD resource not found");
		return NULL;
	}
	
	/* Load the fragment
	   (or return the connID if it is already loaded */
	err = GetSharedLibrary(fragmentname, kCompiledCFragArch,
			      kLoadCFrag, &connID, &mainAddr,
			      errMessage);
	if ( err ) {
		sprintf(buf, "%.*s: %.200s",
			errMessage[0], errMessage+1,
			PyMac_StrError(err));
		PyErr_SetString(PyExc_ImportError, buf);
		return NULL;
	}
	/* Locate the address of the correct init function */
	err = FindSymbol(connID, Pstring(funcname), &symAddr, &class);
	if ( err ) {
		sprintf(buf, "%s: %.200s",
			funcname, PyMac_StrError(err));
		PyErr_SetString(PyExc_ImportError, buf);
		return NULL;
	}
	p = (dl_funcptr)symAddr;
	if (p == NULL) {
		PyErr_Format(PyExc_ImportError,
		   "dynamic module does not define init function (%.200s)",
			     funcname);
		return NULL;
	}
	_Py_PackageContext = packagecontext;
	(*p)();
	_Py_PackageContext = NULL;
	if (PyErr_Occurred())
		return NULL;
	if (_PyImport_FixupExtension(name, name) == NULL)
		return NULL;

	m = PyDict_GetItemString(PyImport_GetModuleDict(), name);
	if (m == NULL) {
		PyErr_SetString(PyExc_SystemError,
				"dynamic module not initialized properly");
		return NULL;
	}
#if 1
	/* Remember the filename as the __file__ attribute */
	d = PyModule_GetDict(m);
	s = PyString_FromString(pathname);
	if (s == NULL || PyDict_SetItemString(d, "__file__", s) != 0)
		PyErr_Clear(); /* Not important enough to report */
	Py_XDECREF(s);
#endif
	if (Py_VerboseFlag)
		PySys_WriteStderr("import %s # pyd fragment %#s loaded from %s\n",
			name, fragmentname, pathname);
	Py_INCREF(m);
	return m;
}

/*
** Load the specified module from a resource
*/
PyObject *
PyMac_LoadResourceModule(module, filename)
char *module;
char *filename;
{
	FSSpec fss;
	FInfo finfo;
	short oldrh, filerh;
	Handle h;
	OSErr err;
	PyObject *m, *co;
	long num, size;
	
#ifdef USE_GUSI1
	if ( (err=Path2FSSpec(filename, &fss)) != noErr ||
	     FSpGetFInfo(&fss, &finfo) != noErr )
#else
	if ( (err=FSMakeFSSpec(0, 0, Pstring(filename), &fss)) != noErr )
#endif
		goto error;
	if ( fssequal(&fss, &PyMac_ApplicationFSSpec) ) {
		/*
		** Special case: the application itself. Use a shortcut to
		** forestall opening and closing the application numerous times
		** (which is dead slow when running from CDROM)
		*/
		oldrh = CurResFile();
		UseResFile(PyMac_AppRefNum);
		filerh = -1;
	} else {
		if ( (err=FSpGetFInfo(&fss, &finfo)) != noErr )
			goto error;
		oldrh = CurResFile();
		filerh = FSpOpenResFile(&fss, fsRdPerm);
		if ( filerh == -1 ) {
			err = ResError();
			goto error;
		}
		UseResFile(filerh);
	}
	h = Get1NamedResource('PYC ', Pstring(module));
	if ( h == NULL ) {
		err = ResError();
		goto error;
	}
	HLock(h);
	/*
	** XXXX The next few lines are intimately tied to the format of pyc
	** files. I'm not sure whether this code should be here or in import.c -- Jack
	*/
	size = GetHandleSize(h);
	if ( size < 8 ) {
		PyErr_SetString(PyExc_ImportError, "Resource too small");
		co = NULL;
	} else {
		num = (*h)[0] & 0xff;
		num = num | (((*h)[1] & 0xff) << 8);
		num = num | (((*h)[2] & 0xff) << 16);
		num = num | (((*h)[3] & 0xff) << 24);
		if ( num != PyImport_GetMagicNumber() ) {
			PyErr_SetString(PyExc_ImportError, "Bad MAGIC in resource");
			co = NULL;
		} else {
			co = PyMarshal_ReadObjectFromString((*h)+8, size-8);
			/*
			** Normally, byte 4-7 are the time stamp, but that is not used
			** for 'PYC ' resources. We abuse byte 4 as a flag to indicate
			** that it is a package rather than an ordinary module. 
			** See also py_resource.py. (jvr)
			*/
			if ((*h)[4] & 0xff) {
				/* it's a package */
				/* Set __path__ to the package name */
				PyObject *d, *s;
				int err;
				
				m = PyImport_AddModule(module);
				if (m == NULL) {
					co = NULL;
					goto packageerror;
				}
				d = PyModule_GetDict(m);
				s = PyString_InternFromString(module);
				if (s == NULL) {
					co = NULL;
					goto packageerror;
				}
				err = PyDict_SetItemString(d, "__path__", s);
				Py_DECREF(s);
				if (err != 0) {
					co = NULL;
					goto packageerror;
				}
			}
		}
	}
packageerror:
	HUnlock(h);
	if ( filerh != -1 )
		CloseResFile(filerh);
	else
		ReleaseResource(h);
	UseResFile(oldrh);
	if ( co ) {
		m = PyImport_ExecCodeModuleEx(module, co, "<pyc resource>");
		Py_DECREF(co);
	} else {
		m = NULL;
	}
	if (Py_VerboseFlag)
		PySys_WriteStderr("import %s # pyc resource from %s\n",
			module, filename);
	return m;
error:
	{
		char buf[512];
		
		sprintf(buf, "%s: %s", filename, PyMac_StrError(err));
		PyErr_SetString(PyExc_ImportError, buf);
		return NULL;
	}
}

/*
** Look for a module in a single folder. Upon entry buf and len
** point to the folder to search, upon exit they refer to the full
** pathname of the module found (if any).
*/
struct filedescr *
PyMac_FindModuleExtension(char *buf, size_t *lenp, char *module)
{
	struct filedescr *fdp;
	unsigned char fnbuf[64];
	int modnamelen = strlen(module);
	FSSpec fss;
#ifdef USE_GUSI1
	FInfo finfo;
#endif
	short refnum;
	long dirid;
	
	/*
	** Copy the module name to the buffer (already :-terminated)
	** We also copy the first suffix, if this matches immedeately we're
	** lucky and return immedeately.
	*/
	if ( !_PyImport_Filetab[0].suffix )
		return 0;
		
#if 0
	/* Pre 1.5a4 */
	strcpy(buf+*lenp, module);
	strcpy(buf+*lenp+modnamelen, _PyImport_Filetab[0].suffix);
#else
	strcpy(buf+*lenp, _PyImport_Filetab[0].suffix);
#endif
#ifdef USE_GUSI1
	if ( Path2FSSpec(buf, &fss) == noErr && 
			FSpGetFInfo(&fss, &finfo) == noErr)
		return _PyImport_Filetab;
#else
	if ( FSMakeFSSpec(0, 0, Pstring(buf), &fss) == noErr )
		return _PyImport_Filetab;
#endif
	/*
	** We cannot check for fnfErr (unfortunately), it can mean either that
	** the file doesn't exist (fine, we try others) or the path leading to it.
	*/
	refnum = fss.vRefNum;
	dirid = fss.parID;
	if ( refnum == 0 || dirid == 0 )	/* Fail on nonexistent dir */
		return 0;
	/*
	** We now have the folder parameters. Setup the field for the filename
	*/
	if ( modnamelen > 54 ) return 0;	/* Leave room for extension */
	strcpy((char *)fnbuf+1, module);
	
	for( fdp = _PyImport_Filetab+1; fdp->suffix; fdp++ ) {
		strcpy((char *)fnbuf+1+modnamelen, fdp->suffix);
		fnbuf[0] = strlen((char *)fnbuf+1);
		if (Py_VerboseFlag > 1)
			PySys_WriteStderr("# trying %s%s\n", buf, fdp->suffix);
		if ( FSMakeFSSpec(refnum, dirid, fnbuf, &fss) == noErr ) {
			/* Found it. */
#if 0
			strcpy(buf+*lenp+modnamelen, fdp->suffix);
#else
			strcpy(buf+*lenp, fdp->suffix);
#endif
			*lenp = strlen(buf);
			return fdp;
		}
	}
	return 0;
}
