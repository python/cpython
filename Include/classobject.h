#ifndef Py_CLASSOBJECT_H
#define Py_CLASSOBJECT_H
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

/* Class object interface */

/* Revealing some structures (not for general use) */

typedef struct {
	PyObject_HEAD
	PyObject	*cl_bases;	/* A tuple of class objects */
	PyObject	*cl_dict;	/* A dictionary */
	PyObject	*cl_name;	/* A string */
	/* The following three are functions or NULL */
	PyObject	*cl_getattr;
	PyObject	*cl_setattr;
	PyObject	*cl_delattr;
} PyClassObject;

typedef struct {
	PyObject_HEAD
	PyClassObject	*in_class;	/* The class object */
	PyObject	*in_dict;	/* A dictionary */
} PyInstanceObject;

typedef struct {
	PyObject_HEAD
	PyObject *im_func;   /* The callable object implementing the method */
	PyObject *im_self;   /* The instance it is bound to, or NULL */
	PyObject *im_class;  /* The class that defined the method */
} PyMethodObject;

extern DL_IMPORT(PyTypeObject) PyClass_Type, PyInstance_Type, PyMethod_Type;

#define PyClass_Check(op) ((op)->ob_type == &PyClass_Type)
#define PyInstance_Check(op) ((op)->ob_type == &PyInstance_Type)
#define PyMethod_Check(op) ((op)->ob_type == &PyMethod_Type)

extern PyObject *PyClass_New Py_PROTO((PyObject *, PyObject *, PyObject *));
extern PyObject *PyInstance_New Py_PROTO((PyObject *, PyObject *, PyObject *));
extern PyObject *PyMethod_New Py_PROTO((PyObject *, PyObject *, PyObject *));

extern PyObject *PyMethod_Function Py_PROTO((PyObject *));
extern PyObject *PyMethod_Self Py_PROTO((PyObject *));
extern PyObject *PyMethod_Class Py_PROTO((PyObject *));

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyMethod_GET_FUNCTION(meth) \
        (((PyMethodObject *)meth) -> im_func)
#define PyMethod_GET_SELF(meth) \
	(((PyMethodObject *)meth) -> im_self)
#define PyMethod_GET_CLASS(meth) \
	(((PyMethodObject *)meth) -> im_class)

extern int PyClass_IsSubclass Py_PROTO((PyObject *, PyObject *));

extern PyObject *PyInstance_DoBinOp
	Py_PROTO((PyObject *, PyObject *,
		  char *, char *,
		  PyObject * (*) Py_PROTO((PyObject *, PyObject *)) ));

#ifdef __cplusplus
}
#endif
#endif /* !Py_CLASSOBJECT_H */
