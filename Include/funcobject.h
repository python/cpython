
/* Function object interface */
#ifndef Py_LIMITED_API
#ifndef Py_FUNCOBJECT_H
#define Py_FUNCOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* Function guard */

typedef struct {
    PyObject ob_base;

    /* Initialize a guard:

       - Return 0 on success
       - Return 1 if the guard will always fail: PyFunction_Specialize() must
         ignore the specialization
       - Raise an exception and return -1 on error */
    int (*init) (PyObject *guard, PyObject *func);

    /* Check a guard:

       - Return 0 on success
       - Return 1 if the guard failed temporarely
       - Return 2 if the guard will always fail:
         calling the function must remove the specialized code
       - Raise an exception and return -1 on error

       args is a C array for positional arguments followed by values of keyword
       arguments. Keys of keyword arguments are stored as a tuple of strings in
       kwnames. nargs is the number of positional parameters at the beginning
       of stack. The size of kwnames gives the number of keyword values in the
       stack after positional arguments. */
    int (*check) (PyObject *guard, PyObject **args, Py_ssize_t nargs,
                  PyObject *kwnames);
} PyFuncGuardObject;

PyAPI_DATA(PyTypeObject) PyFuncGuard_Type;


/* Specialized function */

typedef struct {
    PyObject *code;        /* callable or code object */
    Py_ssize_t nb_guard;
    PyObject **guards;     /* PyFuncGuardObject objects */
} PySpecializedCode;


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

typedef struct {
    PyObject_HEAD
    PyObject *func_code;	/* A code object, the __code__ attribute */
    PyObject *func_globals;	/* A dictionary (other mappings won't do) */
    PyObject *func_defaults;	/* NULL or a tuple */
    PyObject *func_kwdefaults;	/* NULL or a dict */
    PyObject *func_closure;	/* NULL or a tuple of cell objects */
    PyObject *func_doc;		/* The __doc__ attribute, can be anything */
    PyObject *func_name;	/* The __name__ attribute, a string object */
    PyObject *func_dict;	/* The __dict__ attribute, a dict or NULL */
    PyObject *func_weakreflist;	/* List of weak references */
    PyObject *func_module;	/* The __module__ attribute, can be anything */
    PyObject *func_annotations;	/* Annotations, a dict or NULL */
    PyObject *func_qualname;    /* The qualified name */

    Py_ssize_t func_nb_specialized;
    PySpecializedCode *func_specialized;

    /* Invariant:
     *     func_closure contains the bindings for func_code->co_freevars, so
     *     PyTuple_Size(func_closure) == PyCode_GetNumFree(func_code)
     *     (func_closure may be NULL if PyCode_GetNumFree(func_code) == 0).
     */
} PyFunctionObject;

PyAPI_DATA(PyTypeObject) PyFunction_Type;

#define PyFunction_Check(op) (Py_TYPE(op) == &PyFunction_Type)

PyAPI_FUNC(PyObject *) PyFunction_New(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_NewWithQualName(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetCode(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetGlobals(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetModule(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetDefaults(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetKwDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetKwDefaults(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetClosure(PyObject *);
PyAPI_FUNC(int) PyFunction_SetClosure(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetAnnotations(PyObject *);
PyAPI_FUNC(int) PyFunction_SetAnnotations(PyObject *, PyObject *);

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyFunction_FastCallDict(
    PyObject *func,
    PyObject **args,
    Py_ssize_t nargs,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) _PyFunction_FastCallKeywords(
    PyObject *func,
    PyObject **stack,
    Py_ssize_t nargs,
    PyObject *kwnames);
#endif

/* Specialize a function: add a specialized code with guards. code is a
 * callable or code object, guards must be non-empty sequence of
 * PyFuncGuardObject objects. Result:
 *
 * - Return 0 on success
 * - Return 1 if the specialization has been ignored
 * - Raise an exception and return -1 on error
 *
 * If code is a Python function, the code of the function is used as the
 * specialized code. The specialized function must have the same parameter
 * defaults, the same keyword parameter defaults, and it must not have
 * specialized code.
 *
 * If code is a Python function or a code object, a new code object is created
 * and the code name and first line number of the function code object are
 * copied. The specialized code must have the same cell variables and the same
 * free variables.
 * */
PyAPI_DATA(int) PyFunction_Specialize(PyObject *func,
                                      PyObject *code, PyObject *guards);

/* Get the list of specialized codes.
 *
 * Return a list of (code, guards) tuples where code is a callable or code
 * object and guards is a list of PyFuncGuard objects.
 *
 * Raise an exception and return NULL on error. */
PyAPI_FUNC(PyObject*) PyFunction_GetSpecializedCodes(PyObject *func);

/* Remove one specialized code of a function with its guards by its index.
 * Return 0 on success or if the index does not exist. Raise an exception and
 * return -1 on error. */
PyAPI_FUNC(int) PyFunction_RemoveSpecialized(PyObject *func, Py_ssize_t index);

/* Remove all specialized codes and guards from a function.
 * Return 0 on success. Raise an exception and return -1 if func is not
 * a function. */
PyAPI_FUNC(int) PyFunction_RemoveAllSpecialized(PyObject *func);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyFunction_GET_CODE(func) \
        (((PyFunctionObject *)func) -> func_code)
#define PyFunction_GET_GLOBALS(func) \
	(((PyFunctionObject *)func) -> func_globals)
#define PyFunction_GET_MODULE(func) \
	(((PyFunctionObject *)func) -> func_module)
#define PyFunction_GET_DEFAULTS(func) \
	(((PyFunctionObject *)func) -> func_defaults)
#define PyFunction_GET_KW_DEFAULTS(func) \
	(((PyFunctionObject *)func) -> func_kwdefaults)
#define PyFunction_GET_CLOSURE(func) \
	(((PyFunctionObject *)func) -> func_closure)
#define PyFunction_GET_ANNOTATIONS(func) \
	(((PyFunctionObject *)func) -> func_annotations)

/* The classmethod and staticmethod types lives here, too */
PyAPI_DATA(PyTypeObject) PyClassMethod_Type;
PyAPI_DATA(PyTypeObject) PyStaticMethod_Type;

PyAPI_FUNC(PyObject *) PyClassMethod_New(PyObject *);
PyAPI_FUNC(PyObject *) PyStaticMethod_New(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_FUNCOBJECT_H */
#endif /* Py_LIMITED_API */
