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

/* Support for dynamic loading of extension modules */
/* If no dynamic linking is supported, this file still generates some code! */

#include "allobjects.h"
#include "osdefs.h"
#include "importdl.h"

extern int verbose; /* Defined in pythonrun.c */

/* Explanation of some of the the various #defines used by dynamic linking...

   symbol	-- defined for:

   DYNAMIC_LINK -- any kind of dynamic linking
   USE_RLD	-- NeXT dynamic linking (currently disabled)
   USE_DL	-- Jack's dl for IRIX 4 or GNU dld with emulation for Jack's dl
   USE_SHLIB	-- SunOS or IRIX 5 (SVR4?) shared libraries
   _AIX		-- AIX style dynamic linking
   NT		-- NT style dynamic linking (using DLLs)
   _DL_FUNCPTR_DEFINED	-- if the typedef dl_funcptr has been defined
   USE_MAC_SHARED_LIBRARY -- Mac CFM shared libraries
   SHORT_EXT	-- short extension for dynamic module, e.g. ".so"
   LONG_EXT	-- long extension, e.g. "module.so"
   hpux		-- HP-UX Dynamic Linking - defined by the compiler
   __NetBSD__	-- NetBSD shared libraries (not quite SVR4 compatible)

   (The other WITH_* symbols are used only once, to set the
   appropriate symbols.)
*/

/* Configure dynamic linking */

#ifdef hpux
#define DYNAMIC_LINK
#include <errno.h>
typedef void (*dl_funcptr)();
#define _DL_FUNCPTR_DEFINED 1
#define SHORT_EXT ".sl"
#define LONG_EXT "module.sl"
#endif 

#ifdef __NetBSD__
#define DYNAMIC_LINK
#define USE_SHLIB

#define dlerror() "error in dynamic linking"
#endif

#ifdef __WIN32__
#define NT
#endif

#ifdef NT
#define DYNAMIC_LINK
#include <windows.h>
typedef FARPROC dl_funcptr;
#define _DL_FUNCPTR_DEFINED
#define SHORT_EXT ".pyd"
#define LONG_EXT "module.pyd"
#endif

#ifdef WITH_RLD
#define DYNAMIC_LINK
#define USE_RLD
#endif

#ifdef WITH_SGI_DL
#define DYNAMIC_LINK
#define USE_DL
#endif

#ifdef WITH_DL_DLD
#define DYNAMIC_LINK
#define USE_DL
#endif

#ifdef __CFM68K__
#define USE_MAC_SHARED_LIBRARY
#endif

#ifdef USE_MAC_SHARED_LIBRARY
#define DYNAMIC_LINK
#define SHORT_EXT ".slb"
#define LONG_EXT "module.slb"
#ifndef _DL_FUNCPTR_DEFINED
typedef void (*dl_funcptr)();
#endif
#endif

#if !defined(DYNAMIC_LINK) && defined(HAVE_DLFCN_H) && defined(HAVE_DLOPEN)
#define DYNAMIC_LINK
#define USE_SHLIB
#endif

#ifdef _AIX
#define DYNAMIC_LINK
#include <sys/ldr.h>
typedef void (*dl_funcptr)();
#define _DL_FUNCPTR_DEFINED
static void aix_loaderror(char *name);
#endif

#ifdef DYNAMIC_LINK

#ifdef USE_SHLIB
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __NetBSD__
#include <nlist.h>
#include <link.h>
#else
#include <dlfcn.h>
#endif
#ifndef _DL_FUNCPTR_DEFINED
typedef void (*dl_funcptr)();
#endif
#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif
#define SHORT_EXT ".so"
#define LONG_EXT "module.so"
#endif /* USE_SHLIB */

#if defined(USE_DL) || defined(hpux)
#include "dl.h"
#endif

#ifdef USE_MAC_SHARED_LIBRARY
#include <CodeFragments.h>
#ifdef __CFM68K__ /* Really just an older version of Universal Headers */
#define CFragConnectionID ConnectionID
#define kLoadCFrag 0x01
#endif
#include <Files.h>
#include "macdefs.h"
#include "macglue.h"
#endif

#ifdef USE_RLD
#include <mach-o/rld.h>
#define FUNCNAME_PATTERN "_init%.200s"
#ifndef _DL_FUNCPTR_DEFINED
typedef void (*dl_funcptr)();
#endif
#endif /* USE_RLD */

extern char *getprogramname();

#ifndef FUNCNAME_PATTERN
#if defined(__hp9000s300) || defined(__NetBSD__) || defined(__BORLANDC__)
#define FUNCNAME_PATTERN "_init%.200s"
#else
#define FUNCNAME_PATTERN "init%.200s"
#endif
#endif

#if !defined(SHORT_EXT) && !defined(LONG_EXT)
#define SHORT_EXT ".o"
#define LONG_EXT "module.o"
#endif /* !SHORT_EXT && !LONG_EXT */

#endif /* DYNAMIC_LINK */

/* Max length of module suffix searched for -- accommodates "module.slb" */
#ifndef MAXSUFFIXSIZE
#define MAXSUFFIXSIZE 12
#endif

/* Pass it on to import.c */
int import_maxsuffixsize = MAXSUFFIXSIZE;

struct filedescr import_filetab[] = {
#ifdef SHORT_EXT
	{SHORT_EXT, "rb", C_EXTENSION},
#endif /* !SHORT_EXT */
#ifdef LONG_EXT
	{LONG_EXT, "rb", C_EXTENSION},
#endif /* !LONG_EXT */
	{".py", "r", PY_SOURCE},
	{".pyc", "rb", PY_COMPILED},
	{0, 0}
};

object *
load_dynamic_module(name, pathname, fp)
	char *name;
	char *pathname;
	FILE *fp;
{
#ifndef DYNAMIC_LINK
	err_setstr(ImportError, "dynamically linked modules not supported");
	return NULL;
#else
	object *m;
	char funcname[258];
	dl_funcptr p = NULL;
#ifdef USE_SHLIB
	static struct {
		dev_t dev;
		ino_t ino;
		void *handle;
	} handles[128];
	static int nhandles = 0;
#endif
	sprintf(funcname, FUNCNAME_PATTERN, name);
#ifdef USE_SHLIB
	if (fp != NULL) {
		int i;
		struct stat statb;
		fstat(fileno(fp), &statb);
		for (i = 0; i < nhandles; i++) {
			if (statb.st_dev == handles[i].dev &&
			    statb.st_ino == handles[i].ino) {
				p = (dl_funcptr) dlsym(handles[i].handle,
						       funcname);
				goto got_it;
			}
		}
		if (nhandles < 128) {
			handles[nhandles].dev = statb.st_dev;
			handles[nhandles].ino = statb.st_ino;
		}
	}
#endif /* USE_SHLIB */
#ifdef USE_MAC_SHARED_LIBRARY
	/* Dynamic loading of CFM shared libraries on the Mac */
	{
		FSSpec libspec;
		CFragConnectionID connID;
		Ptr mainAddr;
		Str255 errMessage;
		OSErr err;
		
		(void)FSMakeFSSpec(0, 0, Pstring(pathname), &libspec);
		err = GetDiskFragment(&libspec, 0, 0, Pstring(name),
				      kLoadCFrag, &connID, &mainAddr,
				      errMessage);
		if ( err ) {
			char buf[512];
			
			sprintf(buf, "%.*s: %s", errMessage[0], errMessage+1, PyMac_StrError(err));
			err_setstr(ImportError, buf);
			return NULL;
		}
		p = (dl_funcptr)mainAddr;
	}
#endif /* USE_MAC_SHARED_LIBRARY */
#ifdef USE_SHLIB
	{
#ifdef RTLD_NOW
		/* RTLD_NOW: resolve externals now
		   (i.e. core dump now if some are missing) */
		void *handle = dlopen(pathname, RTLD_NOW);
#else
		void *handle;
		if (verbose)
			printf("dlopen(\"%s\", %d);\n", pathname, RTLD_LAZY);
		handle = dlopen(pathname, RTLD_LAZY);
#endif /* RTLD_NOW */
		if (handle == NULL) {
			err_setstr(ImportError, dlerror());
			return NULL;
		}
		if (fp != NULL && nhandles < 128)
			handles[nhandles++].handle = handle;
		p = (dl_funcptr) dlsym(handle, funcname);
	}
#endif /* USE_SHLIB */
#ifdef _AIX
	p = (dl_funcptr) load(pathname, 1, 0);
	if (p == NULL) {
		aix_loaderror(pathname);
		return NULL;
	}
#endif /* _AIX */
#ifdef NT
	{
		HINSTANCE hDLL;
		hDLL = LoadLibrary(pathname);
		if (hDLL==NULL){
			char errBuf[64];
			sprintf(errBuf, "DLL load failed with error code %d",
				GetLastError());
			err_setstr(ImportError, errBuf);
		return NULL;
		}
		p = GetProcAddress(hDLL, funcname);
	}
#endif /* NT */
#ifdef USE_DL
	p =  dl_loadmod(getprogramname(), pathname, funcname);
#endif /* USE_DL */
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
		ret = rld_load(errorStream, &new_header, 
				filenames, NULL);

		/* extract the error messages for the exception */
		if(!ret) {
			char *streamBuf;
			int len, maxLen;

			NXPutc(errorStream, (char)0);

			NXGetMemoryBuffer(errorStream,
				&streamBuf, &len, &maxLen);
			err_setstr(ImportError, streamBuf);
		}

		if(ret && rld_lookup(errorStream, funcname, &ptr))
			p = (dl_funcptr) ptr;

		NXCloseMemory(errorStream, NX_FREEBUFFER);

		if(!ret)
			return NULL;
	}
#endif /* USE_RLD */
#ifdef hpux
	{
		shl_t lib;
		int flags;

		flags = BIND_DEFERRED;
		if (verbose)
                {
                        flags = BIND_IMMEDIATE | BIND_NONFATAL | BIND_VERBOSE;
                        printf("shl_load %s\n",pathname);
                }
                lib = shl_load(pathname, flags, 0);
                if (lib == NULL)
                {
                        char buf[256];
                        if (verbose)
                                perror(pathname);
                        sprintf(buf, "Failed to load %.200s", pathname);
                        err_setstr(ImportError, buf);
                        return NULL;
                }
                if (verbose)
                        printf("shl_findsym %s\n", funcname);
                shl_findsym(&lib, funcname, TYPE_UNDEFINED, (void *) &p);
                if (p == NULL && verbose)
                        perror(funcname);
	}
#endif /* hpux */
  got_it:
	if (p == NULL) {
		err_setstr(ImportError,
		   "dynamic module does not define init function");
		return NULL;
	}
	(*p)();

	m = dictlookup(import_modules, name);
	if (m == NULL) {
		if (err_occurred() == NULL)
			err_setstr(SystemError,
				   "dynamic module not initialized properly");
		return NULL;
	}
	if (verbose)
		fprintf(stderr,
			"import %s # dynamically loaded from %s\n",
			name, pathname);
	INCREF(m);
	return m;
#endif /* DYNAMIC_LINK */
}


#ifdef _AIX

#include <ctype.h>	/* for isdigit()	*/
#include <errno.h>	/* for global errno	*/
#include <string.h>	/* for strerror()	*/

void aix_loaderror(char *pathname)
{

	char *message[8], errbuf[1024];
	int i,j;

	struct errtab { 
		int errno;
		char *errstr;
	} load_errtab[] = {
		{L_ERROR_TOOMANY,	"too many errors, rest skipped."},
		{L_ERROR_NOLIB,		"can't load library:"},
		{L_ERROR_UNDEF,		"can't find symbol in library:"},
		{L_ERROR_RLDBAD,
		 "RLD index out of range or bad relocation type:"},
		{L_ERROR_FORMAT,	"not a valid, executable xcoff file:"},
		{L_ERROR_MEMBER,
		 "file not an archive or does not contain requested member:"},
		{L_ERROR_TYPE,		"symbol table mismatch:"},
		{L_ERROR_ALIGN,		"text allignment in file is wrong."},
		{L_ERROR_SYSTEM,	"System error:"},
		{L_ERROR_ERRNO,		NULL}
	};

#define LOAD_ERRTAB_LEN	(sizeof(load_errtab)/sizeof(load_errtab[0]))
#define ERRBUF_APPEND(s) strncat(errbuf, s, sizeof(errbuf)-strlen(errbuf)-1)

	sprintf(errbuf, " from module %.200s ", pathname);

	if (!loadquery(1, &message[0], sizeof(message))) 
		ERRBUF_APPEND(strerror(errno));
	for(i = 0; message[i] && *message[i]; i++) {
		int nerr = atoi(message[i]);
		for (j=0; j<LOAD_ERRTAB_LEN ; j++) {
		    if (nerr == load_errtab[i].errno && load_errtab[i].errstr)
			ERRBUF_APPEND(load_errtab[i].errstr);
		}
		while (isdigit(*message[i])) message[i]++ ; 
		ERRBUF_APPEND(message[i]);
		ERRBUF_APPEND("\n");
	}
	errbuf[strlen(errbuf)-1] = '\0';	/* trim off last newline */
	err_setstr(ImportError, errbuf); 
	return; 
}

#endif /* _AIX */
