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

/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "importdl.h"

#include <Aliases.h>
#include <CodeFragments.h>
#ifdef SYMANTEC__CFM68K__ /* Really an older version of Universal Headers */
#define CFragConnectionID ConnectionID
#define kLoadCFrag 0x01
#endif
#ifdef USE_GUSI
#include "TFileSpec.h"		/* for Path2FSSpec() */
#endif
#include <Files.h>
#include "macdefs.h"
#include "macglue.h"


const struct filedescr _PyImport_DynLoadFiletab[] = {
	{".slb", "rb", C_EXTENSION},
#ifdef __CFM68K__
	{".CFM68K.slb", "rb", C_EXTENSION},
#else
	{".ppc.slb", "rb", C_EXTENSION},
#endif
	{0, 0}
};


dl_funcptr _PyImport_GetDynLoadFunc(const char *name, const char *funcname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p;

	/*
	** Dynamic loading of CFM shared libraries on the Mac.  The
	** code has become more convoluted than it was, because we
	** want to be able to put multiple modules in a single
	** file. For this reason, we have to determine the fragment
	** name, and we cannot use the library entry point but we have
	** to locate the correct init routine "by hand".
	*/
	FSSpec libspec;
	CFragConnectionID connID;
	Ptr mainAddr;
	Str255 errMessage;
	OSErr err;
#ifndef USE_GUSI
	Boolean isfolder, didsomething;
#endif
	char buf[512];
	Str63 fragname;
	Ptr symAddr;
	CFragSymbolClass class;
		
	/* First resolve any aliases to find the real file */
#ifdef USE_GUSI
	err = Path2FSSpec(pathname, &libspec);
#else
	(void)FSMakeFSSpec(0, 0, Pstring(pathname), &libspec);
	err = ResolveAliasFile(&libspec, 1, &isfolder, &didsomething);
#endif
	if ( err ) {
		sprintf(buf, "%.255s: %.200s",
			pathname, PyMac_StrError(err));
		PyErr_SetString(PyExc_ImportError, buf);
		return NULL;
	}
	/* Next, determine the fragment name,
	   by stripping '.slb' and 'module' */
	memcpy(fragname+1, libspec.name+1, libspec.name[0]);
	fragname[0] = libspec.name[0];
	if( strncmp((char *)(fragname+1+fragname[0]-4),
		    ".slb", 4) == 0 )
		fragname[0] -= 4;
	if ( strncmp((char *)(fragname+1+fragname[0]-6),
		     "module", 6) == 0 )
		fragname[0] -= 6;
	/* Load the fragment
	   (or return the connID if it is already loaded */
	err = GetDiskFragment(&libspec, 0, 0, fragname, 
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

	return p;
}
