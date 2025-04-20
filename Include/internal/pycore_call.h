#ifndef Py_INTERNAL_CALL_H
#define Py_INTERNAL_CALL_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_code.h"          // EVAL_CALL_STAT_INC_IF_FUNCTION()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_stats.h"

/* Suggested size (number of positional arguments) for arrays of PyObject*
   allocated on a C stack to avoid allocating memory on the heap memory. Such
   array is used to pass positional arguments to call functions of the
   PyObject_Vectorcall() family.

   The size is chosen to not abuse the C stack and so limit the risk of stack
   overflow. The size is also chosen to allow using the small stack for most
   function calls of the Python standard library. On 64-bit CPU, it allocates
   40 bytes on the stack. */
#define _PY_FASTCALL_SMALL_STACK 5


// Export for 'math' shared extension, used via _PyObject_VectorcallTstate()
// static inline function.
PyAPI_FUNC(PyObject*) _Py_CheckFunctionResult(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *result,
    const char *where);

extern PyObject* _PyObject_Call_Prepend(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *obj,
    PyObject *args,
    PyObject *kwargs);

extern PyObject* _PyObject_VectorcallDictTstate(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *const *args,
    size_t nargsf,
    PyObject *kwargs);

extern PyObject* _PyObject_Call(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *args,
    PyObject *kwargs);

extern PyObject * _PyObject_CallMethodFormat(
    PyThreadState *tstate,
    PyObject *callable,
    const char *format,
    ...);

// Export for 'array' shared extension
PyAPI_FUNC(PyObject*) _PyObject_CallMethod(
    PyObject *obj,
    PyObject *name,
    const char *format, ...);

extern PyObject* _PyObject_CallMethodIdObjArgs(
    PyObject *obj,
    _Py_Identifier *name,
    ...);

static inline PyObject *
_PyObject_VectorcallMethodId(
    _Py_Identifier *name, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
    if (!oname) {
        return _Py_NULL;
    }
    return PyObject_VectorcallMethod(oname, args, nargsf, kwnames);
}

static inline PyObject *
_PyObject_CallMethodIdNoArgs(PyObject *self, _Py_Identifier *name)
{
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return _PyObject_VectorcallMethodId(name, &self, nargsf, _Py_NULL);
}

static inline PyObject *
_PyObject_CallMethodIdOneArg(PyObject *self, _Py_Identifier *name, PyObject *arg)
{
    PyObject *args[2] = {self, arg};
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    assert(arg != NULL);
    return _PyObject_VectorcallMethodId(name, args, nargsf, _Py_NULL);
}


/* === Vectorcall protocol (PEP 590) ============================= */

// Call callable using tp_call. Arguments are like PyObject_Vectorcall(),
// except that nargs is plainly the number of arguments without flags.
//
// Export for 'math' shared extension, used via _PyObject_VectorcallTstate()
// static inline function.
PyAPI_FUNC(PyObject*) _PyObject_MakeTpCall(
    PyThreadState *tstate,
    PyObject *callable,
    PyObject *const *args, Py_ssize_t nargs,
    PyObject *keywords);

// Static inline variant of public PyVectorcall_Function().
static inline vectorcallfunc
_PyVectorcall_FunctionInline(PyObject *callable)
{
    assert(callable != NULL);

    PyTypeObject *tp = Py_TYPE(callable);
    if (!PyType_HasFeature(tp, Py_TPFLAGS_HAVE_VECTORCALL)) {
        return NULL;
    }
    assert(PyCallable_Check(callable));

    Py_ssize_t offset = tp->tp_vectorcall_offset;
    assert(offset > 0);

    vectorcallfunc ptr;
    memcpy(&ptr, (char *) callable + offset, sizeof(ptr));
    return ptr;
}


/* Call the callable object 'callable' with the "vectorcall" calling
   convention.

   args is a C array for positional arguments.

   nargsf is the number of positional arguments plus optionally the flag
   PY_VECTORCALL_ARGUMENTS_OFFSET which means that the caller is allowed to
   modify args[-1].

   kwnames is a tuple of keyword names. The values of the keyword arguments
   are stored in "args" after the positional arguments (note that the number
   of keyword arguments does not change nargsf). kwnames can also be NULL if
   there are no keyword arguments.

   keywords must only contain strings and all keys must be unique.

   Return the result on success. Raise an exception and return NULL on
   error. */
static inline PyObject *
_PyObject_VectorcallTstate(PyThreadState *tstate, PyObject *callable,
                           PyObject *const *args, size_t nargsf,
                           PyObject *kwnames)
{
    vectorcallfunc func;
    PyObject *res;

    assert(kwnames == NULL || PyTuple_Check(kwnames));
    assert(args != NULL || PyVectorcall_NARGS(nargsf) == 0);

    func = _PyVectorcall_FunctionInline(callable);
    if (func == NULL) {
        Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
        return _PyObject_MakeTpCall(tstate, callable, args, nargs, kwnames);
    }
    res = func(callable, args, nargsf, kwnames);
    return _Py_CheckFunctionResult(tstate, callable, res, NULL);
}


static inline PyObject *
_PyObject_CallNoArgsTstate(PyThreadState *tstate, PyObject *func) {
    return _PyObject_VectorcallTstate(tstate, func, NULL, 0, NULL);
}


// Private static inline function variant of public PyObject_CallNoArgs()
static inline PyObject *
_PyObject_CallNoArgs(PyObject *func) {
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, func, NULL, 0, NULL);
}


extern PyObject *const *
_PyStack_UnpackDict(PyThreadState *tstate,
    PyObject *const *args, Py_ssize_t nargs,
    PyObject *kwargs, PyObject **p_kwnames);

extern void _PyStack_UnpackDict_Free(
    PyObject *const *stack,
    Py_ssize_t nargs,
    PyObject *kwnames);

extern void _PyStack_UnpackDict_FreeNoDecRef(
    PyObject *const *stack,
    PyObject *kwnames);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CALL_H */
