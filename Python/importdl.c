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
#include "importdl.h"

/* Explanation of some of the the various #defines used by dynamic linking...

   symbol	-- defined for:

   HAVE_DYNAMIC_LOADING -- any kind of dynamic linking (from ./configure)
   USE_SHLIB	-- SunOS or IRIX 5 (SVR4?) shared libraries
   _AIX		-- AIX style dynamic linking
   __NetBSD__	-- NetBSD shared libraries
		   (assuming dlerror() was introduced between 1.2 and 1.3)
   __BEOS__ -- BeOS shared libraries - defined by the compiler
*/

/* Configure dynamic linking */

#ifdef HAVE_DYNAMIC_LOADING

extern dl_funcptr _PyImport_GetDynLoadFunc(const char *name,
					   const char *funcname,
					   const char *pathname, FILE *fp);

/* ### given NeXT, is WITH_DYLD not necessary? */

#if defined(__hp9000s300) || (defined(__NetBSD__) || defined(__FreeBSD__)) && !defined(__ELF__) || defined(__OpenBSD__) || defined(__BORLANDC__) || defined(NeXT) || defined(WITH_DYLD)
#define FUNCNAME_PATTERN "_init%.200s"
#else
#define FUNCNAME_PATTERN "init%.200s"
#endif

/* ### temporary, for setting USE_SHLIB */
#if defined(__NetBSD__) && (NetBSD < 199712)
#define USE_SHLIB
#else
#if defined(HAVE_DLOPEN) || defined(M_UNIX)
#define USE_SHLIB
#endif
#endif
#ifdef _AIX
#undef USE_SHLIB
#endif
#ifdef __BEOS__
#undef USE_SHLIB
#endif

#endif /* HAVE_DYNAMIC_LOADING */

#ifdef NO_DYNAMIC_LINK
#undef HAVE_DYNAMIC_LOADING
#endif


PyObject *
_PyImport_LoadDynamicModule(name, pathname, fp)
	char *name;
	char *pathname;
	FILE *fp;
{
#ifndef HAVE_DYNAMIC_LOADING
	PyErr_SetString(PyExc_ImportError,
			"dynamically linked modules not supported");
	return NULL;
#else
	PyObject *m, *d, *s;
	char funcname[258];
	char *lastdot, *shortname, *packagecontext;
	dl_funcptr p = NULL;

#ifdef USE_SHLIB
	char pathbuf[260];
	if (strchr(pathname, '/') == NULL) {
		/* Prefix bare filename with "./" */
		sprintf(pathbuf, "./%-.255s", pathname);
		pathname = pathbuf;
	}
#endif /* USE_SHLIB */

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

	p = _PyImport_GetDynLoadFunc(name, funcname, pathname, fp);
	if (PyErr_Occurred())
		return NULL;
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
		PySys_WriteStderr(
			"import %s # dynamically loaded from %s\n",
			name, pathname);
	Py_INCREF(m);
	return m;
#endif /* HAVE_DYNAMIC_LOADING */
}
