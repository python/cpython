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


#ifdef WITH_DYLD

#define USE_DYLD

#include <mach-o/dyld.h>

#else /* WITH_DYLD */

#define USE_RLD
/* Define this to 1 if you want be able to load ObjC modules as well:
   it switches between two different way of loading modules on the NeXT,
   one that directly interfaces with the dynamic loader (rld_load(), which
   does not correctly load ObjC object files), and another that uses the
   ObjC runtime (objc_loadModules()) to do the job.
   You'll have to add ``-ObjC'' to the compiler flags if you set this to 1.
*/
#define HANDLE_OBJC_MODULES 1
#if HANDLE_OBJC_MODULES
#include <objc/Object.h>
#include <objc/objc-load.h>
#endif

#include <mach-o/rld.h>

#endif /* WITH_DYLD */


const struct filedescr _PyImport_DynLoadFiletab[] = {
	{".so", "rb", C_EXTENSION},
	{"module.so", "rb", C_EXTENSION},
	{0, 0}
};

dl_funcptr _PyImport_GetDynLoadFunc(const char *name, const char *funcname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p = NULL;

#ifdef USE_RLD
	{
		NXStream *errorStream;
		struct mach_header *new_header;
		const char *filenames[2];
		long ret;
		unsigned long ptr;

		errorStream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
		filenames[0] = pathname;
		filenames[1] = NULL;

#if HANDLE_OBJC_MODULES

/* The following very bogus line of code ensures that
   objc_msgSend, etc are linked into the binary.  Without
   it, dynamic loading of a module that includes objective-c
   method calls will fail with "undefined symbol _objc_msgSend()".
   This remains true even in the presence of the -ObjC flag
   to the compiler
*/

		[Object name];

/* objc_loadModules() dynamically loads the object files
   indicated by the paths in filenames.  If there are any
   errors generated during loading -- typically due to the
   inability to find particular symbols -- an error message
   will be written to errorStream.
   It returns 0 if the module is successfully loaded, 1
   otherwise.
*/

		ret = !objc_loadModules(filenames, errorStream,
					NULL, &new_header, NULL);

#else /* !HANDLE_OBJC_MODULES */

		ret = rld_load(errorStream, &new_header, 
				filenames, NULL);

#endif /* HANDLE_OBJC_MODULES */

		/* extract the error messages for the exception */
		if(!ret) {
			char *streamBuf;
			int len, maxLen;

			NXPutc(errorStream, (char)0);

			NXGetMemoryBuffer(errorStream,
				&streamBuf, &len, &maxLen);
			PyErr_SetString(PyExc_ImportError, streamBuf);
		}

		if(ret && rld_lookup(errorStream, funcname, &ptr))
			p = (dl_funcptr) ptr;

		NXCloseMemory(errorStream, NX_FREEBUFFER);

		if(!ret)
			return NULL;
	}
#endif /* USE_RLD */
#ifdef USE_DYLD
	/* This is also NeXT-specific. However, frameworks (the new style
	of shared library) and rld() can't be used in the same program;
	instead, you have to use dyld, which is mostly unimplemented. */
	{
		NSObjectFileImageReturnCode rc;
		NSObjectFileImage image;
		NSModule newModule;
		NSSymbol theSym;
		void *symaddr;
		const char *errString;
	
		rc = NSCreateObjectFileImageFromFile(pathname, &image);
		switch(rc) {
		    default:
		    case NSObjectFileImageFailure:
		    NSObjectFileImageFormat:
		    /* for these a message is printed on stderr by dyld */
			errString = "Can't create object file image";
			break;
		    case NSObjectFileImageSuccess:
			errString = NULL;
			break;
		    case NSObjectFileImageInappropriateFile:
			errString = "Inappropriate file type for dynamic loading";
			break;
		    case NSObjectFileImageArch:
			errString = "Wrong CPU type in object file";
			break;
		    NSObjectFileImageAccess:
			errString = "Can't read object file (no access)";
			break;
		}
		if (errString == NULL) {
			newModule = NSLinkModule(image, pathname, TRUE);
			if (!newModule)
				errString = "Failure linking new module";
		}
		if (errString != NULL) {
			PyErr_SetString(PyExc_ImportError, errString);
			return NULL;
		}
		if (!NSIsSymbolNameDefined(funcname)) {
			/* UnlinkModule() isn't implimented in current versions, but calling it does no harm */
			NSUnLinkModule(newModule, FALSE);
			PyErr_Format(PyExc_ImportError, "Loaded module does not contain symbol %s", funcname);
			return NULL;
		}
		theSym = NSLookupAndBindSymbol(funcname);
		p = (dl_funcptr)NSAddressOfSymbol(theSym);
 	}
#endif /* USE_DYLD */

	return p;
}
