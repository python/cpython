#ifndef Py_FRAMEOBJECT_H
#define Py_FRAMEOBJECT_H
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

/* Frame object interface */

typedef struct {
	int b_type;		/* what kind of block this is */
	int b_handler;		/* where to jump to find handler */
	int b_level;		/* value stack level to pop to */
} PyTryBlock;

typedef struct _frame {
	PyObject_HEAD
	struct _frame *f_back;	/* previous frame, or NULL */
	PyCodeObject *f_code;	/* code segment */
	PyObject *f_builtins;	/* builtin symbol table (PyDictObject) */
	PyObject *f_globals;	/* global symbol table (PyDictObject) */
	PyObject *f_locals;	/* local symbol table (PyDictObject) */
	PyObject *f_owner;	/* owner (e.g. class or module) or NULL */
	PyObject *f_fastlocals;	/* fast local variables (PyListObject) */
	PyObject *f_localmap;	/* local variable names (PyDictObject) */
	PyObject **f_valuestack;	/* malloc'ed array */
	PyTryBlock *f_blockstack;	/* malloc'ed array */
	int f_nvalues;		/* size of f_valuestack */
	int f_nblocks;		/* size of f_blockstack */
	int f_iblock;		/* index in f_blockstack */
	int f_lasti;		/* Last instruction if called */
	int f_lineno;		/* Current line number */
	int f_restricted;	/* Flag set if restricted operations
				   in this scope */
	PyObject *f_trace;	/* Trace function */
} PyFrameObject;


/* Standard object interface */

extern DL_IMPORT(PyTypeObject) PyFrame_Type;

#define PyFrame_Check(op) ((op)->ob_type == &PyFrame_Type)

PyFrameObject * PyFrame_New
	Py_PROTO((PyFrameObject *, PyCodeObject *,
		  PyObject *, PyObject *, PyObject *, int, int));


/* The rest of the interface is specific for frame objects */

/* Tuple access macros */

#ifdef NDEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((PyTupleObject *)(v), (i))
#define GETITEMNAME(v, i) \
	PyString_AS_STRING((PyStringObject *)GETITEM((v), (i)))
#else
#define GETITEM(v, i) PyTuple_GetItem((v), (i))
#define GETITEMNAME(v, i) PyString_AsString(GETITEM(v, i))
#endif

#define GETUSTRINGVALUE(s) ((unsigned char *)PyString_AS_STRING(s))

/* Code access macros */

#define Getconst(f, i)	(GETITEM((f)->f_code->co_consts, (i)))
#define Getname(f, i)	(GETITEMNAME((f)->f_code->co_names, (i)))
#define Getnamev(f, i)	(GETITEM((f)->f_code->co_names, (i)))


/* Block management functions */

void PyFrame_BlockSetup Py_PROTO((PyFrameObject *, int, int, int));
PyTryBlock *PyFrame_BlockPop Py_PROTO((PyFrameObject *));

/* Extend the value stack */

PyObject **PyFrame_ExtendStack Py_PROTO((PyFrameObject *, int, int));

/* Conversions between "fast locals" and locals in dictionary */

void PyFrame_LocalsToFast Py_PROTO((PyFrameObject *, int));
void PyFrame_FastToLocals Py_PROTO((PyFrameObject *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_FRAMEOBJECT_H */
