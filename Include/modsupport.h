#ifndef Py_MODSUPPORT_H
#define Py_MODSUPPORT_H
#ifdef __cplusplus
extern "C" {
#endif

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

/* Module support interface */

#ifdef HAVE_STDARG_PROTOTYPES

#include <stdarg.h>

extern DL_IMPORT(int) PyArg_Parse Py_PROTO((PyObject *, char *, ...));
extern DL_IMPORT(int) PyArg_ParseTuple Py_PROTO((PyObject *, char *, ...));
extern DL_IMPORT(int) PyArg_ParseTupleAndKeywords Py_PROTO((PyObject *, PyObject *,
						 char *, char **, ...));
extern DL_IMPORT(PyObject *) Py_BuildValue Py_PROTO((char *, ...));

#else

#include <varargs.h>

/* Better to have no prototypes at all for varargs functions in this case */
extern DL_IMPORT(int) PyArg_Parse();
extern DL_IMPORT(int) PyArg_ParseTuple();
extern DL_IMPORT(PyObject *) Py_BuildValue();

#endif

extern DL_IMPORT(int) PyArg_VaParse Py_PROTO((PyObject *, char *, va_list));
extern DL_IMPORT(PyObject *) Py_VaBuildValue Py_PROTO((char *, va_list));

#define PYTHON_API_VERSION 1007
#define PYTHON_API_STRING "1007"
/* The API version is maintained (independently from the Python version)
   so we can detect mismatches between the interpreter and dynamically
   loaded modules.  These are diagnosticised by an error message but
   the module is still loaded (because the mismatch can only be tested
   after loading the module).  The error message is intended to
   explain the core dump a few seconds later.

   The symbol PYTHON_API_STRING defines the same value as a string
   literal.  *** PLEASE MAKE SURE THE DEFINITIONS MATCH. ***

   Please add a line or two to the top of this log for each API
   version change:

   3-Jan-1999	GvR	1007	Decided to change back!  (Don't reuse 1008!)

   3-Dec-1998	GvR	1008	Python 1.5.2b1

   18-Jan-1997	GvR	1007	string interning and other speedups

   11-Oct-1996	GvR	renamed Py_Ellipses to Py_Ellipsis :-(

   30-Jul-1996	GvR	Slice and ellipses syntax added

   23-Jul-1996	GvR	For 1.4 -- better safe than sorry this time :-)

   7-Nov-1995	GvR	Keyword arguments (should've been done at 1.3 :-( )

   10-Jan-1995	GvR	Renamed globals to new naming scheme

   9-Jan-1995	GvR	Initial version (incompatible with older API)
*/

#ifdef MS_WINDOWS
/* Special defines for Windows versions used to live here.  Things
   have changed, and the "Version" is now in a global string variable.
   Reason for this is that this for easier branding of a "custom DLL"
   without actually needing a recompile.  */
#endif /* MS_WINDOWS */

#ifdef Py_TRACE_REFS
/* When we are tracing reference counts, rename Py_InitModule4 so
   modules compiled with incompatible settings will generate a
   link-time error. */
#define Py_InitModule4 Py_InitModule4TraceRefs
#endif

extern DL_IMPORT(PyObject *) Py_InitModule4 Py_PROTO((char *, PyMethodDef *,
					  char *, PyObject *, int));
#define Py_InitModule(name, methods) \
	Py_InitModule4(name, methods, (char *)NULL, (PyObject *)NULL, \
		       PYTHON_API_VERSION)

#define Py_InitModule3(name, methods, doc) \
	Py_InitModule4(name, methods, doc, (PyObject *)NULL, \
		       PYTHON_API_VERSION)

extern DL_IMPORT(char *) _Py_PackageContext;

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODSUPPORT_H */
