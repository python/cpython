#ifndef Py_FUNCOBJECT_H
#define Py_FUNCOBJECT_H
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

/* Function object interface */

typedef struct {
	PyObject_HEAD
	PyObject *func_code;
	PyObject *func_globals;
	PyObject *func_defaults;
	PyObject *func_doc;
	PyObject *func_name;
} PyFunctionObject;

extern DL_IMPORT(PyTypeObject) PyFunction_Type;

#define PyFunction_Check(op) ((op)->ob_type == &PyFunction_Type)

extern DL_IMPORT(PyObject *) PyFunction_New Py_PROTO((PyObject *, PyObject *));
extern DL_IMPORT(PyObject *) PyFunction_GetCode Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyFunction_GetGlobals Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyFunction_GetDefaults Py_PROTO((PyObject *));
extern DL_IMPORT(int) PyFunction_SetDefaults Py_PROTO((PyObject *, PyObject *));

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyFunction_GET_CODE(func) \
        (((PyFunctionObject *)func) -> func_code)
#define PyFunction_GET_GLOBALS(func) \
	(((PyFunctionObject *)func) -> func_globals)
#define PyFunction_GET_DEFAULTS(func) \
	(((PyFunctionObject *)func) -> func_defaults)

#ifdef __cplusplus
}
#endif
#endif /* !Py_FUNCOBJECT_H */
