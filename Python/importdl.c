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
/* If no dynamic linking is supported, this file still generates some code! */

#include "Python.h"

#ifdef HAVE_SYS_PARAM_H
/* osdefs.h will define MAXPATHLEN if it's not already defined. */
#include <sys/param.h>
#endif
#include "osdefs.h"
#include "importdl.h"

/* Explanation of some of the the various #defines used by dynamic linking...

   symbol	-- defined for:

   DYNAMIC_LINK -- any kind of dynamic linking
   USE_RLD	-- NeXT dynamic linking
   USE_DL	-- Jack's dl for IRIX 4 or GNU dld with emulation for Jack's dl
   USE_SHLIB	-- SunOS or IRIX 5 (SVR4?) shared libraries
   _AIX		-- AIX style dynamic linking
   MS_WIN32	-- Windows NT style dynamic linking (using DLLs)
   MS_WIN16	-- Windows 16-bit dynamic linking (using DLLs)
   PYOS_OS2 -- IBM OS/2 dynamic linking (using DLLs)
   _DL_FUNCPTR_DEFINED	-- if the typedef dl_funcptr has been defined
   USE_MAC_DYNAMIC_LOADING -- Mac CFM shared libraries
   SHORT_EXT	-- short extension for dynamic module, e.g. ".so"
   LONG_EXT	-- long extension, e.g. "module.so"
   hpux		-- HP-UX Dynamic Linking - defined by the compiler
   __NetBSD__	-- NetBSD shared libraries
		   (assuming dlerror() was introduced between 1.2 and 1.3)
   __FreeBSD__	-- FreeBSD shared libraries

   (The other WITH_* symbols are used only once, to set the
   appropriate symbols.)
*/

/* Configure dynamic linking */

#ifdef __hpux
#ifndef hpux
#define hpux
#endif
#endif

#ifdef hpux
#define DYNAMIC_LINK
#include <errno.h>
typedef void (*dl_funcptr)();
#define _DL_FUNCPTR_DEFINED 1
#define SHORT_EXT ".sl"
#define LONG_EXT "module.sl"
#endif 

#if defined(PYOS_OS2)
#define DYNAMIC_LINK
#define  INCL_DOSERRORS
#define  INCL_DOSMODULEMGR
#include <os2.h>
typedef int (* APIENTRY dl_funcptr)();
#define _DL_FUNCPTR_DEFINED 1
#define SHORT_EXT ".pyd"
#define LONG_EXT ".dll"
#endif

#if defined(__NetBSD__) && (NetBSD < 199712)
#define DYNAMIC_LINK
#define USE_SHLIB

#define dlerror() "error in dynamic linking"
#endif

#ifdef MS_WINDOWS /* i.e. MS_WIN32 or MS_WIN16 */
#define DYNAMIC_LINK
#include <windows.h>
typedef FARPROC dl_funcptr;
#define _DL_FUNCPTR_DEFINED
#define SHORT_EXT ".pyd"
#define LONG_EXT ".dll"
#endif

#ifdef NeXT
#define DYNAMIC_LINK
#define USE_RLD
#define SHORT_EXT ".so"
#define LONG_EXT "module.so"
#endif

#ifdef WITH_SGI_DL
#define DYNAMIC_LINK
#define USE_DL
#endif

#ifdef WITH_DL_DLD
#define DYNAMIC_LINK
#define USE_DL
#endif

#ifdef USE_MAC_DYNAMIC_LOADING
#define DYNAMIC_LINK
#define SHORT_EXT ".slb"
#ifdef __CFM68K__
#define LONG_EXT ".CFM68K.slb"
#else
#define LONG_EXT ".ppc.slb"
#endif
#ifndef _DL_FUNCPTR_DEFINED
typedef void (*dl_funcptr)();
#endif
#endif

#if !defined(DYNAMIC_LINK) && (defined(HAVE_DLOPEN) || defined(M_UNIX))
#define DYNAMIC_LINK
#define USE_SHLIB
#endif

#ifdef _AIX
#undef USE_SHLIB /* AIX 4.2 and higher have dlfcn.h but we don't want it */
#define DYNAMIC_LINK
#define SHORT_EXT ".so"
#define LONG_EXT "module.so"
#include <sys/ldr.h>
typedef void (*dl_funcptr)();
#define _DL_FUNCPTR_DEFINED
static int  aix_getoldmodules(void **);
static int  aix_bindnewmodule(void *, void *);
static void aix_loaderror(char *);
#endif

#ifdef DYNAMIC_LINK

#ifdef USE_SHLIB
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__NetBSD__) && (NetBSD < 199712)
#include <nlist.h>
#include <link.h>
#else
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#endif
#ifndef _DL_FUNCPTR_DEFINED
typedef void (*dl_funcptr)();
#endif
#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#define SHORT_EXT ".so"
#define LONG_EXT "module.so"
#endif /* USE_SHLIB */

#if defined(USE_DL) || defined(hpux)
#include "dl.h"
#endif

#ifdef USE_MAC_DYNAMIC_LOADING
#include <Aliases.h>
#include <CodeFragments.h>
#ifdef SYMANTEC__CFM68K__ /* Really an older version of Universal Headers */
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

extern char *Py_GetProgramName();

#ifndef FUNCNAME_PATTERN
#if defined(__hp9000s300) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__BORLANDC__)
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

struct filedescr _PyImport_Filetab[] = {
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

#ifdef NO_DYNAMIC_LINK
#undef DYNAMIC_LINK
#endif

PyObject *
_PyImport_LoadDynamicModule(name, pathname, fp)
	char *name;
	char *pathname;
	FILE *fp;
{
#ifndef DYNAMIC_LINK
	PyErr_SetString(PyExc_ImportError,
			"dynamically linked modules not supported");
	return NULL;
#else
	PyObject *m, *d, *s;
	char funcname[258];
	char *lastdot, *shortname, *packagecontext;
	dl_funcptr p = NULL;
#ifdef USE_SHLIB
	static struct {
		dev_t dev;
		ino_t ino;
		void *handle;
	} handles[128];
	static int nhandles = 0;
	char pathbuf[260];
	if (strchr(pathname, '/') == NULL) {
		/* Prefix bare filename with "./" */
		sprintf(pathbuf, "./%-.255s", pathname);
		pathname = pathbuf;
	}
#endif
	if ((m = _PyImport_FindExtension(name, pathname)) != NULL) {
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
#ifdef USE_MAC_DYNAMIC_LOADING
	/*
	** Dynamic loading of CFM shared libraries on the Mac.  The
	** code has become more convoluted than it was, because we
	** want to be able to put multiple modules in a single
	** file. For this reason, we have to determine the fragment
	** name, and we cannot use the library entry point but we have
	** to locate the correct init routine "by hand".
	*/
	{
		FSSpec libspec;
		CFragConnectionID connID;
		Ptr mainAddr;
		Str255 errMessage;
		OSErr err;
		Boolean isfolder, didsomething;
		char buf[512];
		Str63 fragname;
		Ptr symAddr;
		CFragSymbolClass class;
		
		/* First resolve any aliases to find the real file */
		(void)FSMakeFSSpec(0, 0, Pstring(pathname), &libspec);
		err = ResolveAliasFile(&libspec, 1, &isfolder, &didsomething);
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
	}
#endif /* USE_MAC_DYNAMIC_LOADING */
#ifdef USE_SHLIB
	{
#ifdef RTLD_NOW
		/* RTLD_NOW: resolve externals now
		   (i.e. core dump now if some are missing) */
		void *handle = dlopen(pathname, RTLD_NOW | RTLD_GLOBAL);
#else
		void *handle;
		if (Py_VerboseFlag)
			printf("dlopen(\"%s\", %d);\n", pathname,
			       RTLD_LAZY | RTLD_GLOBAL);
		handle = dlopen(pathname, RTLD_LAZY | RTLD_GLOBAL);
#endif /* RTLD_NOW */
		if (handle == NULL) {
			PyErr_SetString(PyExc_ImportError, dlerror());
			return NULL;
		}
		if (fp != NULL && nhandles < 128)
			handles[nhandles++].handle = handle;
		p = (dl_funcptr) dlsym(handle, funcname);
	}
#endif /* USE_SHLIB */
#ifdef _AIX
	/*
	-- Invoke load() with L_NOAUTODEFER leaving the imported symbols
	-- of the shared module unresolved. Thus we have to resolve them
	-- explicitely with loadbind. The new module is loaded, then we
	-- resolve its symbols using the list of already loaded modules
	-- (only those that belong to the python executable). Get these
	-- with loadquery(L_GETINFO).
	*/
	{
		static void *staticmodlistptr = NULL;

		if (!staticmodlistptr)
			if (aix_getoldmodules(&staticmodlistptr) == -1)
				return NULL;
		p = (dl_funcptr) load(pathname, L_NOAUTODEFER, 0);
		if (p == NULL) {
			aix_loaderror(pathname);
			return NULL;
		}
		if (aix_bindnewmodule((void *)p, staticmodlistptr) == -1) {
			aix_loaderror(pathname);
			return NULL;
		}
	}
#endif /* _AIX */
#ifdef MS_WIN32
	{
		HINSTANCE hDLL;
		hDLL = LoadLibrary(pathname);
		if (hDLL==NULL){
			char errBuf[256];
			unsigned int errorCode;

			/* Get an error string from Win32 error code */
			char theInfo[256]; /* Pointer to error text
					      from system */
			int theLength; /* Length of error text */

			errorCode = GetLastError();

			theLength = FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM, /* flags */
				NULL, /* message source */
				errorCode, /* the message (error) ID */
				0, /* default language environment */
				(LPTSTR) theInfo, /* the buffer */
				sizeof(theInfo), /* the buffer size */
				NULL); /* no additional format args. */

			/* Problem: could not get the error message.
			   This should not happen if called correctly. */
			if (theLength == 0) {
				sprintf(errBuf,
					"DLL load failed with error code %d",
					errorCode);
			} else {
				int len;
				/* For some reason a \r\n
				   is appended to the text */
				if (theLength >= 2 &&
				    theInfo[theLength-2] == '\r' &&
				    theInfo[theLength-1] == '\n') {
					theLength -= 2;
					theInfo[theLength] = '\0';
				}
				strcpy(errBuf, "DLL load failed: ");
				len = strlen(errBuf);
				strncpy(errBuf+len, theInfo,
					sizeof(errBuf)-len);
				errBuf[sizeof(errBuf)-1] = '\0';
			}
			PyErr_SetString(PyExc_ImportError, errBuf);
		return NULL;
		}
		p = GetProcAddress(hDLL, funcname);
	}
#endif /* MS_WIN32 */
#ifdef MS_WIN16
	{
		HINSTANCE hDLL;
		hDLL = LoadLibrary(pathname);
		if (hDLL < HINSTANCE_ERROR){
			char errBuf[256];
			sprintf(errBuf,
				"DLL load failed with error code %d", hDLL);
			PyErr_SetString(PyExc_ImportError, errBuf);
			return NULL;
		}
		p = GetProcAddress(hDLL, funcname);
	}
#endif /* MS_WIN16 */

#if defined(PYOS_OS2)
	{
		APIRET  rc;
		HMODULE hDLL;
        char failreason[256];

		rc = DosLoadModule(failreason,
                           sizeof(failreason),
                           pathname,
                           &hDLL);

		if (rc != NO_ERROR) {
			char errBuf[256];
			sprintf(errBuf,
				"DLL load failed, rc = %d, problem '%s': %s", rc, failreason);
			PyErr_SetString(PyExc_ImportError, errBuf);
			return NULL;
		}

        rc = DosQueryProcAddr(hDLL, 0L, funcname, &p);
        if (rc != NO_ERROR)
            p = NULL; /* Signify Failure to Acquire Entrypoint */
	}
#endif /* PYOS_OS2 */

#ifdef USE_DL
	p =  dl_loadmod(Py_GetProgramName(), pathname, funcname);
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
			PyErr_SetString(PyExc_ImportError, streamBuf);
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

		flags = BIND_FIRST | BIND_DEFERRED;
		if (Py_VerboseFlag)
                {
                        flags = DYNAMIC_PATH | BIND_FIRST | BIND_IMMEDIATE |
				BIND_NONFATAL | BIND_VERBOSE;
                        printf("shl_load %s\n",pathname);
                }
                lib = shl_load(pathname, flags, 0);
                if (lib == NULL)
                {
                        char buf[256];
                        if (Py_VerboseFlag)
                                perror(pathname);
                        sprintf(buf, "Failed to load %.200s", pathname);
                        PyErr_SetString(PyExc_ImportError, buf);
                        return NULL;
                }
                if (Py_VerboseFlag)
                        printf("shl_findsym %s\n", funcname);
                shl_findsym(&lib, funcname, TYPE_UNDEFINED, (void *) &p);
                if (p == NULL && Py_VerboseFlag)
                        perror(funcname);
	}
#endif /* hpux */
#ifdef USE_SHLIB
  got_it:
#endif
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
	if (_PyImport_FixupExtension(name, pathname) == NULL)
		return NULL;

	m = PyDict_GetItemString(PyImport_GetModuleDict(), name);
	if (m == NULL) {
		PyErr_SetString(PyExc_SystemError,
				"dynamic module not initialized properly");
		return NULL;
	}
	/* Remember the filename as the __file__ attribute */
	d = PyModule_GetDict(m);
	s = PyString_FromString(pathname);
	if (s == NULL || PyDict_SetItemString(d, "__file__", s) != 0)
		PyErr_Clear(); /* Not important enough to report */
	Py_XDECREF(s);
	if (Py_VerboseFlag)
		fprintf(stderr,
			"import %s # dynamically loaded from %s\n",
			name, pathname);
	Py_INCREF(m);
	return m;
#endif /* DYNAMIC_LINK */
}


#ifdef _AIX

#include <ctype.h>	/*  for isdigit()	  */
#include <errno.h>	/*  for global errno      */
#include <string.h>	/*  for strerror()        */
#include <stdlib.h>	/*  for malloc(), free()  */

typedef struct Module {
	struct Module *next;
	void          *entry;
} Module, *ModulePtr;

static int
aix_getoldmodules(modlistptr)
	void **modlistptr;
{
	register ModulePtr       modptr, prevmodptr;
	register struct ld_info  *ldiptr;
	register char            *ldibuf;
	register int             errflag, bufsize = 1024;
	register unsigned int    offset;
	
	/*
	-- Get the list of loaded modules into ld_info structures.
	*/
	if ((ldibuf = malloc(bufsize)) == NULL) {
		PyErr_SetString(PyExc_ImportError, strerror(errno));
		return -1;
	}
	while ((errflag = loadquery(L_GETINFO, ldibuf, bufsize)) == -1
	       && errno == ENOMEM) {
		free(ldibuf);
		bufsize += 1024;
		if ((ldibuf = malloc(bufsize)) == NULL) {
			PyErr_SetString(PyExc_ImportError, strerror(errno));
			return -1;
		}
	}
	if (errflag == -1) {
		PyErr_SetString(PyExc_ImportError, strerror(errno));
		return -1;
	}
	/*
	-- Make the modules list from the ld_info structures.
	*/
	ldiptr = (struct ld_info *)ldibuf;
	prevmodptr = NULL;
	do {
		if (strstr(ldiptr->ldinfo_filename, "python") == NULL) {
			/*
			-- Extract only the modules containing "python" as a
			-- substring, like the "python[version]" executable or
			-- "libpython[version].a" in case python is embedded.
			*/
			offset = (unsigned int)ldiptr->ldinfo_next;
			ldiptr = (struct ld_info *)((unsigned int)
						    ldiptr + offset);
			continue;
		}
		if ((modptr = (ModulePtr)malloc(sizeof(Module))) == NULL) {
			PyErr_SetString(PyExc_ImportError, strerror(errno));
			while (*modlistptr) {
				modptr = (ModulePtr)*modlistptr;
				*modlistptr = (void *)modptr->next;
				free(modptr);
			}
			return -1;
		}
		modptr->entry = ldiptr->ldinfo_dataorg;
		modptr->next  = NULL;
		if (prevmodptr == NULL)
			*modlistptr = (void *)modptr;
		else
			prevmodptr->next = modptr;
		prevmodptr = modptr;
		offset = (unsigned int)ldiptr->ldinfo_next;
		ldiptr = (struct ld_info *)((unsigned int)ldiptr + offset);
	} while (offset);
	free(ldibuf);
	return 0;
}

static int
aix_bindnewmodule(newmoduleptr, modlistptr)
	void *newmoduleptr;
	void *modlistptr;        
{
	register ModulePtr modptr;

	/*
	-- Bind the new module with the list of loaded modules.
	*/
	for (modptr = (ModulePtr)modlistptr; modptr; modptr = modptr->next)
		if (loadbind(0, modptr->entry, newmoduleptr) != 0)
			return -1;
	return 0;
}

static void
aix_loaderror(pathname)
	char *pathname;
{

	char *message[1024], errbuf[1024];
	register int i,j;

	struct errtab { 
		int errNo;
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
		{L_ERROR_ALIGN,		"text alignment in file is wrong."},
		{L_ERROR_SYSTEM,	"System error:"},
		{L_ERROR_ERRNO,		NULL}
	};

#define LOAD_ERRTAB_LEN	(sizeof(load_errtab)/sizeof(load_errtab[0]))
#define ERRBUF_APPEND(s) strncat(errbuf, s, sizeof(errbuf)-strlen(errbuf)-1)

	sprintf(errbuf, "from module %.200s ", pathname);

	if (!loadquery(L_GETMESSAGES, &message[0], sizeof(message))) {
		ERRBUF_APPEND(strerror(errno));
		ERRBUF_APPEND("\n");
	}
	for(i = 0; message[i] && *message[i]; i++) {
		int nerr = atoi(message[i]);
		for (j=0; j<LOAD_ERRTAB_LEN ; j++) {
		    if (nerr == load_errtab[j].errNo && load_errtab[j].errstr)
			ERRBUF_APPEND(load_errtab[j].errstr);
		}
		while (isdigit(*message[i])) message[i]++ ; 
		ERRBUF_APPEND(message[i]);
		ERRBUF_APPEND("\n");
	}
	errbuf[strlen(errbuf)-1] = '\0';	/* trim off last newline */
	PyErr_SetString(PyExc_ImportError, errbuf); 
	return; 
}

#endif /* _AIX */
