
/* Definitions for bytecode */

#ifndef Py_COMPILE_H
#define Py_COMPILE_H
#ifdef __cplusplus
extern "C" {
#endif

/* Bytecode object */
typedef struct {
    PyObject_HEAD
    int co_argcount;		/* #arguments, except *args */
    int co_nlocals;		/* #local variables */
    int co_stacksize;		/* #entries needed for evaluation stack */
    int co_flags;		/* CO_..., see below */
    PyObject *co_code;		/* instruction opcodes */
    PyObject *co_consts;	/* list (constants used) */
    PyObject *co_names;		/* list of strings (names used) */
    PyObject *co_varnames;	/* tuple of strings (local variable names) */
    PyObject *co_freevars;	/* tuple of strings (free variable names) */
    PyObject *co_cellvars;      /* tuple of strings (cell variable names) */
    /* The rest doesn't count for hash/cmp */
    PyObject *co_filename;	/* string (where it was loaded from) */
    PyObject *co_name;		/* string (name, for reference) */
    int co_firstlineno;		/* first source line number */
    PyObject *co_lnotab;	/* string (encoding addr<->lineno mapping) */
} PyCodeObject;

/* Masks for co_flags above */
#define CO_OPTIMIZED	0x0001
#define CO_NEWLOCALS	0x0002
#define CO_VARARGS	0x0004
#define CO_VARKEYWORDS	0x0008
#define CO_NESTED       0x0010
#define CO_GENERATOR    0x0020
/* The CO_NOFREE flag is set if there are no free or cell variables.
   This information is redundant, but it allows a single flag test
   to determine whether there is any extra work to be done when the
   call frame it setup.
*/
#define CO_NOFREE       0x0040
/* XXX Temporary hack.  Until generators are a permanent part of the
   language, we need a way for a code object to record that generators
   were *possible* when it was compiled.  This is so code dynamically
   compiled *by* a code object knows whether to allow yield stmts.  In
   effect, this passes on the "from __future__ import generators" state
   in effect when the code block was compiled. */
#define CO_GENERATOR_ALLOWED    0x1000 /* no longer used in an essential way */
#define CO_FUTURE_DIVISION    	0x2000

PyAPI_DATA(PyTypeObject) PyCode_Type;

#define PyCode_Check(op) ((op)->ob_type == &PyCode_Type)
#define PyCode_GetNumFree(op) (PyTuple_GET_SIZE((op)->co_freevars))

#define CO_MAXBLOCKS 20 /* Max static block nesting within a function */

/* Public interface */
struct _node; /* Declare the existence of this type */
PyAPI_FUNC(PyCodeObject *) PyNode_Compile(struct _node *, const char *);
PyAPI_FUNC(PyCodeObject *) PyCode_New(
	int, int, int, int, PyObject *, PyObject *, PyObject *, PyObject *,
	PyObject *, PyObject *, PyObject *, PyObject *, int, PyObject *); 
        /* same as struct above */
PyAPI_FUNC(int) PyCode_Addr2Line(PyCodeObject *, int);

/* Future feature support */

typedef struct {
    int ff_found_docstring;
    int ff_last_lineno;
    int ff_features;
} PyFutureFeatures;

PyAPI_FUNC(PyFutureFeatures *) PyNode_Future(struct _node *, const char *);
PyAPI_FUNC(PyCodeObject *) PyNode_CompileFlags(struct _node *, const char *,
					      PyCompilerFlags *);

#define FUTURE_NESTED_SCOPES "nested_scopes"
#define FUTURE_GENERATORS "generators"
#define FUTURE_DIVISION "division"

#ifdef __cplusplus
}
#endif
#endif /* !Py_COMPILE_H */
