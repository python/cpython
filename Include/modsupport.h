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

/* Module support interface */

#ifdef HAVE_STDARG_PROTOTYPES

#include <stdarg.h>

extern int PyArg_Parse Py_PROTO((PyObject *, char *, ...));
extern int PyArg_ParseTuple Py_PROTO((PyObject *, char *, ...));
extern PyObject *Py_BuildValue Py_PROTO((char *, ...));

#else

#include <varargs.h>

/* Better to have no prototypes at all for varargs functions in this case */
extern int PyArg_Parse();
extern int PyArg_ParseTuple();
extern PyObject *Py_BuildValue();

#endif

extern int PyArg_VaParse Py_PROTO((PyObject *, char *, va_list));
extern PyObject *Py_VaBuildValue Py_PROTO((char *, va_list));

#define PYTHON_API_VERSION 1002
/* The API version is maintained (independently from the Python version)
   so we can detect mismatches between the interpreter and dynamically
   loaded modules.

   Please add a line or two to the top of this log for each API
   version change:

   10-Jan-1995	GvR	Renamed globals to new naming scheme

   9-Jan-1995	GvR	Initial version (incompatible with older API)
*/

extern PyObject *Py_InitModule4 Py_PROTO((char *, PyMethodDef *,
					  char *, PyObject *, int));
#define Py_InitModule(name, methods) \
	Py_InitModule4(name, methods, (char *)NULL, (PyObject *)NULL, \
		       PYTHON_API_VERSION)

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODSUPPORT_H */
