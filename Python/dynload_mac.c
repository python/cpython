
/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "importdl.h"

#include <Aliases.h>
#include <CodeFragments.h>
#ifdef USE_GUSI1
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


dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p;
	char funcname[258];

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
#ifndef USE_GUSI1
	Boolean isfolder, didsomething;
#endif
	char buf[512];
	Str63 fragname;
	Ptr symAddr;
	CFragSymbolClass class;
		
	/* First resolve any aliases to find the real file */
#ifdef USE_GUSI1
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
	if ( err == cfragImportTooOldErr || err == cfragImportTooNewErr ) {
		/*
		** Special-case code: if PythonCore is too old or too new this means
		** the dynamic module was meant for a different Python.
		*/
		if (errMessage[0] == 10 && strncmp((char *)errMessage+1, "PythonCore", 10) == 0 ) {
			sprintf(buf, "Dynamic module was built for %s version of MacPython",
				(err == cfragImportTooOldErr ? "a newer" : "an older"));
			PyErr_SetString(PyExc_ImportError, buf);
			return NULL;
		}
	}
	if ( err ) {
		sprintf(buf, "%.*s: %.200s",
			errMessage[0], errMessage+1,
			PyMac_StrError(err));
		PyErr_SetString(PyExc_ImportError, buf);
		return NULL;
	}
	/* Locate the address of the correct init function */
	sprintf(funcname, "init%.200s", shortname);
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
