
#ifndef Py_MODSUPPORT_H
#define Py_MODSUPPORT_H
#ifdef __cplusplus
extern "C" {
#endif

/* Module support interface */

#include <stdarg.h>

extern DL_IMPORT(int) PyArg_Parse(PyObject *, char *, ...);
extern DL_IMPORT(int) PyArg_ParseTuple(PyObject *, char *, ...);
extern DL_IMPORT(int) PyArg_ParseTupleAndKeywords(PyObject *, PyObject *,
                                                  char *, char **, ...);
extern DL_IMPORT(PyObject *) Py_BuildValue(char *, ...);

extern DL_IMPORT(int) PyArg_VaParse(PyObject *, char *, va_list);
extern DL_IMPORT(PyObject *) Py_VaBuildValue(char *, va_list);

extern DL_IMPORT(int) PyModule_AddObject(PyObject *, char *, PyObject *);
extern DL_IMPORT(int) PyModule_AddIntConstant(PyObject *, char *, long);
extern DL_IMPORT(int) PyModule_AddStringConstant(PyObject *, char *, char *);

#define PYTHON_API_VERSION 1009
#define PYTHON_API_STRING "1009"
/* The API version is maintained (independently from the Python version)
   so we can detect mismatches between the interpreter and dynamically
   loaded modules.  These are diagnosed by an error message but
   the module is still loaded (because the mismatch can only be tested
   after loading the module).  The error message is intended to
   explain the core dump a few seconds later.

   The symbol PYTHON_API_STRING defines the same value as a string
   literal.  *** PLEASE MAKE SURE THE DEFINITIONS MATCH. ***

   Please add a line or two to the top of this log for each API
   version change:

   14-Mar-2000  GvR     1009    Unicode API added

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

extern DL_IMPORT(PyObject *) Py_InitModule4(char *name, PyMethodDef *methods,
                                            char *doc, PyObject *self,
                                            int apiver);

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
