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

/* ./configure sets HAVE_DYNAMIC_LOADING if dynamic loading of modules is
   supported on this platform. configure will then compile and link in one
   of the dynload_*.c files, as appropriate. We will call a function in
   those modules to get a function pointer to the module's init function.
*/
#ifdef HAVE_DYNAMIC_LOADING

#include "importdl.h"

extern dl_funcptr _PyImport_GetDynLoadFunc(const char *name,
					   const char *shortname,
					   const char *pathname, FILE *fp);



PyObject *
_PyImport_LoadDynamicModule(name, pathname, fp)
	char *name;
	char *pathname;
	FILE *fp;
{
	PyObject *m, *d, *s;
	char *lastdot, *shortname, *packagecontext;
	dl_funcptr p;

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

	p = _PyImport_GetDynLoadFunc(name, shortname, pathname, fp);
	if (PyErr_Occurred())
		return NULL;
	if (p == NULL) {
		PyErr_Format(PyExc_ImportError,
		   "dynamic module does not define init function (init%.200s)",
			     shortname);
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
}

#endif /* HAVE_DYNAMIC_LOADING */
