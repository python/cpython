#ifndef PYCORE_FUNCTION_H
#define PYCORE_FUNCTION_H

#define COMMON_FIELDS(PREFIX) \
    PyObject *PREFIX ## globals; \
    PyObject *PREFIX ## builtins; \
    PyObject *PREFIX ## name; \
    PyObject *PREFIX ## qualname; \
    PyObject *PREFIX ## code;        /* A code object, the __code__ attribute */ \
    PyObject *PREFIX ## defaults;    /* NULL or a tuple */ \
    PyObject *PREFIX ## kwdefaults;  /* NULL or a dict */ \
    PyObject *PREFIX ## closure;     /* NULL or a tuple of cell objects */

typedef struct {
    COMMON_FIELDS(fc_)
} PyFrameConstructor;

/* Function objects and code objects should not be confused with each other:
 *
 * Function objects are created by the execution of the 'def' statement.
 * They reference a code object in their __code__ attribute, which is a
 * purely syntactic object, i.e. nothing more than a compiled version of some
 * source code lines.  There is one code object per source code "fragment",
 * but each code object can be referenced by zero or many function objects
 * depending only on how many times the 'def' statement in the source was
 * executed so far.
 */

typedef struct _functionobject {
    PyObject_HEAD
    COMMON_FIELDS(func_)
    PyObject *func_doc;         /* The __doc__ attribute, can be anything */
    PyObject *func_dict;        /* The __dict__ attribute, a dict or NULL */
    PyObject *func_weakreflist; /* List of weak references */
    PyObject *func_module;      /* The __module__ attribute, can be anything */
    PyObject *func_annotations; /* Annotations, a dict or NULL */
    vectorcallfunc vectorcall;
    /* Version number for use by specializer.
     * Can set to non-zero when we want to specialize.
     * Will be set to zero if any of these change:
     *     defaults
     *     kwdefaults (only if the object changes, not the contents of the dict)
     *     code
     *     annotations */
    uint32_t func_version;

    /* Invariant:
     *     func_closure contains the bindings for func_code->co_freevars, so
     *     PyTuple_Size(func_closure) == PyCode_GetNumFree(func_code)
     *     (func_closure may be NULL if PyCode_GetNumFree(func_code) == 0).
     */
} PyFunctionObject;

#define PyFunction_AS_FRAME_CONSTRUCTOR(func) \
        ((PyFrameConstructor *)&((PyFunctionObject *)(func))->func_globals)

uint32_t _PyFunction_GetVersionForCurrentState(PyFunctionObject *func);


#ifdef __cplusplus
}
#endif
#endif // PYCORE_FUNCTION_H
