#ifndef Py_CALL_H
#define Py_CALL_H
#ifdef __cplusplus
extern "C" {
#endif


/* --- PyMethodDef ------------------------------------------------ */

/* Various function types which can appear in ml_meth. The signature
   depends on the METH_xxx flags */
typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                             PyObject *);
typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                   PyObject *const *, Py_ssize_t,
                                                   PyObject *);


struct PyMethodDef {
    const char  *ml_name;   /* The name of the built-in function/method */
    PyCFunction ml_meth;    /* The C function that implements it */
    int         ml_flags;   /* Combination of METH_xxx flags, which mostly
                               describes the args expected by the C func */
    const char  *ml_doc;    /* The __doc__ attribute, or NULL */
};
typedef struct PyMethodDef PyMethodDef;

/* Flags for PyMethodDef */
/* Exactly one of these 4 must be set */
#define METH_VARARGS    0x0001
#ifndef Py_LIMITED_API
#define METH_FASTCALL   0x0080
#endif
#define METH_NOARGS     0x0004
#define METH_O          0x0008

/* Can be combined with METH_VARARGS and METH_FASTCALL */
#define METH_KEYWORDS   0x0002

/* METH_CLASS and METH_STATIC control the construction of methods for a
   class.  These cannot be used for functions in modules. */
#define METH_CLASS      0x0010
#define METH_STATIC     0x0020

/* METH_COEXIST allows a method to be entered even though a slot has
   already filled the entry.  When defined, the flag allows a separate
   method, "__contains__" for example, to coexist with a defined
   slot like sq_contains. */
#define METH_COEXIST    0x0040

/* This bit is preserved for Stackless Python */
#ifdef STACKLESS
#define METH_STACKLESS  0x0100
#else
#define METH_STACKLESS  0x0000
#endif


/* --- Calls for specific classes --------------------------------- */

PyAPI_FUNC(PyObject *) PyCFunction_Call(PyObject *, PyObject *, PyObject *);

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyCFunction_FastCallDict(PyObject *func,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) _PyCFunction_FastCallKeywords(PyObject *func,
    PyObject *const *stack,
    Py_ssize_t nargs,
    PyObject *kwnames);

PyAPI_FUNC(PyObject *) _PyFunction_FastCallDict(
    PyObject *func,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) _PyFunction_FastCallKeywords(
    PyObject *func,
    PyObject *const *stack,
    Py_ssize_t nargs,
    PyObject *kwnames);

PyAPI_FUNC(PyObject *) _PyMethodDef_RawFastCallDict(
    PyMethodDef *method,
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) _PyMethodDef_RawFastCallKeywords(
    PyMethodDef *method,
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames);
#endif


/* --- Convert FASTCALL arguments to other types ------------------ */

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) _PyStack_AsTuple(
    PyObject *const *stack,
    Py_ssize_t nargs);

PyAPI_FUNC(PyObject*) _PyStack_AsTupleSlice(
    PyObject *const *stack,
    Py_ssize_t nargs,
    Py_ssize_t start,
    Py_ssize_t end);

/* Convert keyword arguments from the FASTCALL (stack: C array, kwnames: tuple)
   format to a Python dictionary ("kwargs" dict).

   The type of kwnames keys is not checked. The final function getting
   arguments is responsible to check if all keys are strings, for example using
   PyArg_ParseTupleAndKeywords() or PyArg_ValidateKeywordArguments().

   Duplicate keys are merged using the last value. If duplicate keys must raise
   an exception, the caller is responsible to implement an explicit keys on
   kwnames. */
PyAPI_FUNC(PyObject *) _PyStack_AsDict(
    PyObject *const *values,
    PyObject *kwnames);

/* Convert (args, nargs, kwargs: dict) into a (stack, nargs, kwnames: tuple).

   Return 0 on success, raise an exception and return -1 on error.

   Write the new stack into *p_stack. If *p_stack is differen than args, it
   must be released by PyMem_Free().

   The stack uses borrowed references.

   The type of keyword keys is not checked, these checks should be done
   later (ex: _PyArg_ParseStackAndKeywords). */
PyAPI_FUNC(int) _PyStack_UnpackDict(
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs,
    PyObject *const **p_stack,
    PyObject **p_kwnames);

/* Suggested size (number of positional arguments) for arrays of PyObject*
   allocated on a C stack to avoid allocating memory on the heap memory. Such
   array is used to pass positional arguments to call functions of the
   _PyObject_FastCall() family.

   The size is chosen to not abuse the C stack and so limit the risk of stack
   overflow. The size is also chosen to allow using the small stack for most
   function calls of the Python standard library. On 64-bit CPU, it allocates
   40 bytes on the stack. */
#define _PY_FASTCALL_SMALL_STACK 5
#endif


/* --- Generic call functions ------------------------------------- */

#ifdef PY_SSIZE_T_CLEAN
#  define PyObject_CallFunction _PyObject_CallFunction_SizeT
#  define PyObject_CallMethod _PyObject_CallMethod_SizeT
#  ifndef Py_LIMITED_API
#    define _PyObject_CallMethodId _PyObject_CallMethodId_SizeT
#  endif /* !Py_LIMITED_API */
#endif

/* Call a callable Python object 'callable' with arguments given by the
   tuple 'args' and keywords arguments given by the dictionary 'kwargs'.

   'args' must not be *NULL*, use an empty tuple if no arguments are
   needed. If no named arguments are needed, 'kwargs' can be NULL.

   This is the equivalent of the Python expression:
   callable(*args, **kwargs). */
PyAPI_FUNC(PyObject *) PyObject_Call(PyObject *callable,
                                     PyObject *args, PyObject *kwargs);

#ifndef Py_LIMITED_API
/* Return 1 if callable supports FASTCALL calling convention for positional
   arguments: see _PyObject_FastCallDict() and _PyObject_FastCallKeywords() */
PyAPI_FUNC(int) _PyObject_HasFastCall(PyObject *callable);

/* Call the callable object 'callable' with the "fast call" calling convention:
   args is a C array for positional arguments (nargs is the number of
   positional arguments), kwargs is a dictionary for keyword arguments.

   If nargs is equal to zero, args can be NULL. kwargs can be NULL.
   nargs must be greater or equal to zero.

   Return the result on success. Raise an exception on return NULL on
   error. */
PyAPI_FUNC(PyObject *) _PyObject_FastCallDict(
    PyObject *callable,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs);

/* Call the callable object 'callable' with the "fast call" calling convention:
   args is a C array for positional arguments followed by values of
   keyword arguments. Keys of keyword arguments are stored as a tuple
   of strings in kwnames. nargs is the number of positional parameters at
   the beginning of stack. The size of kwnames gives the number of keyword
   values in the stack after positional arguments.

   kwnames must only contains str strings, no subclass, and all keys must
   be unique.

   If nargs is equal to zero and there is no keyword argument (kwnames is
   NULL or its size is zero), args can be NULL.

   Return the result on success. Raise an exception and return NULL on
   error. */
PyAPI_FUNC(PyObject *) _PyObject_FastCallKeywords(
    PyObject *callable,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames);

#define _PyObject_FastCall(func, args, nargs) \
    _PyObject_FastCallDict((func), (args), (nargs), NULL)

#define _PyObject_CallNoArg(func) \
    _PyObject_FastCallDict((func), NULL, 0, NULL)

PyAPI_FUNC(PyObject *) _PyObject_Call_Prepend(
    PyObject *callable,
    PyObject *obj,
    PyObject *args,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) _PyObject_FastCall_Prepend(
    PyObject *callable,
    PyObject *obj,
    PyObject *const *args,
    Py_ssize_t nargs);
#endif   /* Py_LIMITED_API */


/* Call a callable Python object 'callable', with arguments given by the
   tuple 'args'.  If no arguments are needed, then 'args' can be *NULL*.

   Returns the result of the call on success, or *NULL* on failure.

   This is the equivalent of the Python expression:
   callable(*args). */
PyAPI_FUNC(PyObject *) PyObject_CallObject(PyObject *callable,
                                           PyObject *args);

/* Call a callable Python object, callable, with a variable number of C
   arguments. The C arguments are described using a mkvalue-style format
   string.

   The format may be NULL, indicating that no arguments are provided.

   Returns the result of the call on success, or NULL on failure.

   This is the equivalent of the Python expression:
   callable(arg1, arg2, ...). */
PyAPI_FUNC(PyObject *) PyObject_CallFunction(PyObject *callable,
                                             const char *format, ...);

/* Call the method named 'name' of object 'obj' with a variable number of
   C arguments.  The C arguments are described by a mkvalue format string.

   The format can be NULL, indicating that no arguments are provided.

   Returns the result of the call on success, or NULL on failure.

   This is the equivalent of the Python expression:
   obj.name(arg1, arg2, ...). */
PyAPI_FUNC(PyObject *) PyObject_CallMethod(PyObject *obj,
                                           const char *name,
                                           const char *format, ...);

#ifndef Py_LIMITED_API
/* Like PyObject_CallMethod(), but expect a _Py_Identifier*
   as the method name. */
PyAPI_FUNC(PyObject *) _PyObject_CallMethodId(PyObject *obj,
                                              _Py_Identifier *name,
                                              const char *format, ...);
#endif /* !Py_LIMITED_API */

PyAPI_FUNC(PyObject *) _PyObject_CallFunction_SizeT(PyObject *callable,
                                                    const char *format,
                                                    ...);

PyAPI_FUNC(PyObject *) _PyObject_CallMethod_SizeT(PyObject *obj,
                                                  const char *name,
                                                  const char *format,
                                                  ...);

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyObject_CallMethodId_SizeT(PyObject *obj,
                                                    _Py_Identifier *name,
                                                    const char *format,
                                                    ...);
#endif /* !Py_LIMITED_API */

/* Call a callable Python object 'callable' with a variable number of C
   arguments. The C arguments are provided as PyObject* values, terminated
   by a NULL.

   Returns the result of the call on success, or NULL on failure.

   This is the equivalent of the Python expression:
   callable(arg1, arg2, ...). */
PyAPI_FUNC(PyObject *) PyObject_CallFunctionObjArgs(PyObject *callable,
                                                    ...);

/* Call the method named 'name' of object 'obj' with a variable number of
   C arguments.  The C arguments are provided as PyObject* values, terminated
   by NULL.

   Returns the result of the call on success, or NULL on failure.

   This is the equivalent of the Python expression: obj.name(*args). */

PyAPI_FUNC(PyObject *) PyObject_CallMethodObjArgs(
    PyObject *obj,
    PyObject *name,
    ...);

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyObject_CallMethodIdObjArgs(
    PyObject *obj,
    struct _Py_Identifier *name,
    ...);
#endif /* !Py_LIMITED_API */


/* --- Internal helper function ----------------------------------- */

PyAPI_FUNC(PyObject *) _Py_CheckFunctionResult(PyObject *callable,
                                               PyObject *result,
                                               const char *where);


/* --- Backwards compatibility ------------------------------------ */

typedef PyObject *(*PyNoArgsFunction)(PyObject *);

/* PyEval_CallObjectWithKeywords(), PyEval_CallObject(), PyEval_CallFunction
 * and PyEval_CallMethod are kept for backward compatibility: PyObject_Call(),
 * PyObject_CallFunction() and PyObject_CallMethod() are recommended to call
 * a callable object.
 */
PyAPI_FUNC(PyObject *) PyEval_CallObjectWithKeywords(
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs);

#define PyEval_CallObject(callable, arg) \
    PyEval_CallObjectWithKeywords(callable, arg, (PyObject *)NULL)

PyAPI_FUNC(PyObject *) PyEval_CallFunction(PyObject *callable,
                                           const char *format, ...);
PyAPI_FUNC(PyObject *) PyEval_CallMethod(PyObject *obj,
                                         const char *name,
                                         const char *format, ...);


#ifdef __cplusplus
}
#endif
#endif /* !Py_CALL_H */
