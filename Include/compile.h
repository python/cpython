#ifndef Py_COMPILE_H
#define Py_COMPILE_H
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

/* Definitions for bytecode */

/* Bytecode object */
typedef struct {
	PyObject_HEAD
	int co_argcount;	/* #arguments, except *args */
	int co_nlocals;		/* #local variables */
	int co_stacksize;	/* #entries needed for evaluation stack */
	int co_flags;		/* CO_..., see below */
	PyObject *co_code;	/* instruction opcodes */
	PyObject *co_consts;	/* list (constants used) */
	PyObject *co_names;	/* list of strings (names used) */
	PyObject *co_varnames;	/* tuple of strings (local variable names) */
	/* The rest doesn't count for hash/cmp */
	PyObject *co_filename;	/* string (where it was loaded from) */
	PyObject *co_name;	/* string (name, for reference) */
	int co_firstlineno;	/* first source line number */
	PyObject *co_lnotab;	/* string (encoding addr<->lineno mapping) */
} PyCodeObject;

/* Masks for co_flags above */
#define CO_OPTIMIZED	0x0001
#define CO_NEWLOCALS	0x0002
#define CO_VARARGS	0x0004
#define CO_VARKEYWORDS	0x0008

extern DL_IMPORT(PyTypeObject) PyCode_Type;

#define PyCode_Check(op) ((op)->ob_type == &PyCode_Type)

#define CO_MAXBLOCKS 20 /* Max static block nesting within a function */

/* Public interface */
struct _node; /* Declare the existence of this type */
PyCodeObject *PyNode_Compile Py_PROTO((struct _node *, char *));
PyCodeObject *PyCode_New Py_PROTO((
	int, int, int, int, PyObject *, PyObject *, PyObject *, PyObject *,
	PyObject *, PyObject *, int, PyObject *)); /* same as struct above */
int PyCode_Addr2Line Py_PROTO((PyCodeObject *, int));

/* for internal use only */
#define _PyCode_GETCODEPTR(co, pp) \
	((*(co)->co_code->ob_type->tp_as_buffer->bf_getreadbuffer) \
	 ((co)->co_code, 0, (void **)(pp)))

#ifdef __cplusplus
}
#endif
#endif /* !Py_COMPILE_H */
