#ifndef Py_METHODOBJECT_H
#define Py_METHODOBJECT_H
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

/* Method object interface */

extern DL_IMPORT(PyTypeObject) PyCFunction_Type;

#define PyCFunction_Check(op) ((op)->ob_type == &PyCFunction_Type)

typedef PyObject *(*PyCFunction) Py_FPROTO((PyObject *, PyObject *));
typedef PyObject *(*PyCFunctionWithKeywords)
	Py_FPROTO((PyObject *, PyObject *, PyObject *));

extern DL_IMPORT(PyCFunction) PyCFunction_GetFunction Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyCFunction_GetSelf Py_PROTO((PyObject *));
extern DL_IMPORT(int) PyCFunction_GetFlags Py_PROTO((PyObject *));

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyCFunction_GET_FUNCTION(func) \
        (((PyCFunctionObject *)func) -> m_ml -> ml_meth)
#define PyCFunction_GET_SELF(func) \
	(((PyCFunctionObject *)func) -> m_self)
#define PyCFunction_GET_FLAGS(func) \
	(((PyCFunctionObject *)func) -> m_ml -> ml_flags)

struct PyMethodDef {
	char		*ml_name;
	PyCFunction	ml_meth;
	int		ml_flags;
	char		*ml_doc;
};
typedef struct PyMethodDef PyMethodDef;

extern DL_IMPORT(PyObject *) Py_FindMethod
	Py_PROTO((PyMethodDef[], PyObject *, char *));

extern DL_IMPORT(PyObject *) PyCFunction_New
	Py_PROTO((PyMethodDef *, PyObject *));

/* Flag passed to newmethodobject */
#define METH_VARARGS  0x0001
#define METH_KEYWORDS 0x0002

typedef struct PyMethodChain {
	PyMethodDef *methods;		/* Methods of this type */
	struct PyMethodChain *link;	/* NULL or base type */
} PyMethodChain;

extern DL_IMPORT(PyObject *) Py_FindMethodInChain
	Py_PROTO((PyMethodChain *, PyObject *, char *));

typedef struct {
	PyObject_HEAD
	PyMethodDef *m_ml;
	PyObject    *m_self;
} PyCFunctionObject;

#ifdef __cplusplus
}
#endif
#endif /* !Py_METHODOBJECT_H */
