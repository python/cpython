#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgsTstate()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_dict.h"          // _PyDict_FromItems()
#include "pycore_function.h"      // _PyFunction_Vectorcall() definition
#include "pycore_modsupport.h"    // _Py_VaBuildStack()
#include "pycore_object.h"        // _PyCFunctionWithKeywords_TrampolineCall()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()


static PyObject *
null_error(PyThreadState *tstate)
{
    if (!_PyErr_Occurred(tstate)) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                         "null argument to internal routine");
    }
    return NULL;
}


PyObject*
_Py_CheckFunctionResult(PyThreadState *tstate, PyObject *callable,
                        PyObject *result, const char *where)
{
    assert((callable != NULL) ^ (where != NULL));

    if (result == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            if (callable)
                _PyErr_Format(tstate, PyExc_SystemError,
                              "%R returned NULL without setting an exception",
                              callable);
            else
                _PyErr_Format(tstate, PyExc_SystemError,
                              "%s returned NULL without setting an exception",
                              where);
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode.
               Py_FatalError() logs the SystemError exception raised above. */
            Py_FatalError("a function returned NULL without setting an exception");
#endif
            return NULL;
        }
    }
    else {
        if (_PyErr_Occurred(tstate)) {
            Py_DECREF(result);

            if (callable) {
                _PyErr_FormatFromCauseTstate(
                    tstate, PyExc_SystemError,
                    "%R returned a result with an exception set", callable);
            }
            else {
                _PyErr_FormatFromCauseTstate(
                    tstate, PyExc_SystemError,
                    "%s returned a result with an exception set", where);
            }
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode.
               Py_FatalError() logs the SystemError exception raised above. */
            Py_FatalError("a function returned a result with an exception set");
#endif
            return NULL;
        }
    }
    return result;
}


int
_Py_CheckSlotResult(PyObject *obj, const char *slot_name, int success)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!success) {
        if (!_PyErr_Occurred(tstate)) {
            _Py_FatalErrorFormat(__func__,
                                 "Slot %s of type %s failed "
                                 "without setting an exception",
                                 slot_name, Py_TYPE(obj)->tp_name);
        }
    }
    else {
        if (_PyErr_Occurred(tstate)) {
            _Py_FatalErrorFormat(__func__,
                                 "Slot %s of type %s succeeded "
                                 "with an exception set",
                                 slot_name, Py_TYPE(obj)->tp_name);
        }
    }
    return 1;
}


/* --- Core PyObject call functions ------------------------------- */

/* Call a callable Python object without any arguments */
PyObject *
PyObject_CallNoArgs(PyObject *func)
{
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, func, NULL, 0, NULL);
}


PyObject *
_PyObject_VectorcallDictTstate(PyThreadState *tstate, PyObject *callable,
                               PyObject *const *args, size_t nargsf,
                               PyObject *kwargs)
{
    assert(callable != NULL);

    /* PyObject_VectorcallDict() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(kwargs == NULL || PyDict_Check(kwargs));

    vectorcallfunc func = PyVectorcall_Function(callable);
    if (func == NULL) {
        /* Use tp_call instead */
        return _PyObject_MakeTpCall(tstate, callable, args, nargs, kwargs);
    }

    PyObject *res;
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        res = func(callable, args, nargsf, NULL);
    }
    else {
        PyObject *kwnames;
        PyObject *const *newargs;
        newargs = _PyStack_UnpackDict(tstate,
                                      args, nargs,
                                      kwargs, &kwnames);
        if (newargs == NULL) {
            return NULL;
        }
        res = func(callable, newargs,
                   nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
        _PyStack_UnpackDict_Free(newargs, nargs, kwnames);
    }
    return _Py_CheckFunctionResult(tstate, callable, res, NULL);
}


PyObject *
PyObject_VectorcallDict(PyObject *callable, PyObject *const *args,
                       size_t nargsf, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallDictTstate(tstate, callable, args, nargsf, kwargs);
}

static void
object_is_not_callable(PyThreadState *tstate, PyObject *callable)
{
    if (Py_IS_TYPE(callable, &PyModule_Type)) {
        // >>> import pprint
        // >>> pprint(thing)
        // Traceback (most recent call last):
        //   File "<stdin>", line 1, in <module>
        // TypeError: 'module' object is not callable. Did you mean: 'pprint.pprint(...)'?
        PyObject *name = PyModule_GetNameObject(callable);
        if (name == NULL) {
            _PyErr_Clear(tstate);
            goto basic_type_error;
        }
        PyObject *attr;
        int res = PyObject_GetOptionalAttr(callable, name, &attr);
        if (res < 0) {
            _PyErr_Clear(tstate);
        }
        else if (res > 0 && PyCallable_Check(attr)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'%.200s' object is not callable. "
                          "Did you mean: '%U.%U(...)'?",
                          Py_TYPE(callable)->tp_name, name, name);
            Py_DECREF(attr);
            Py_DECREF(name);
            return;
        }
        Py_XDECREF(attr);
        Py_DECREF(name);
    }
basic_type_error:
    _PyErr_Format(tstate, PyExc_TypeError, "'%.200s' object is not callable",
                  Py_TYPE(callable)->tp_name);
}


PyObject *
_PyObject_MakeTpCall(PyThreadState *tstate, PyObject *callable,
                     PyObject *const *args, Py_ssize_t nargs,
                     PyObject *keywords)
{
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(keywords == NULL || PyTuple_Check(keywords) || PyDict_Check(keywords));

    /* Slow path: build a temporary tuple for positional arguments and a
     * temporary dictionary for keyword arguments (if any) */
    ternaryfunc call = Py_TYPE(callable)->tp_call;
    if (call == NULL) {
        object_is_not_callable(tstate, callable);
        return NULL;
    }

    PyObject *argstuple = PyTuple_FromArray(args, nargs);
    if (argstuple == NULL) {
        return NULL;
    }

    PyObject *kwdict;
    if (keywords == NULL || PyDict_Check(keywords)) {
        kwdict = keywords;
    }
    else {
        if (PyTuple_GET_SIZE(keywords)) {
            assert(args != NULL);
            kwdict = _PyStack_AsDict(args + nargs, keywords);
            if (kwdict == NULL) {
                Py_DECREF(argstuple);
                return NULL;
            }
        }
        else {
            keywords = kwdict = NULL;
        }
    }

    PyObject *result = NULL;
    if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object") == 0)
    {
        result = _PyCFunctionWithKeywords_TrampolineCall(
            (PyCFunctionWithKeywords)call, callable, argstuple, kwdict);
        _Py_LeaveRecursiveCallTstate(tstate);
    }

    Py_DECREF(argstuple);
    if (kwdict != keywords) {
        Py_DECREF(kwdict);
    }

    return _Py_CheckFunctionResult(tstate, callable, result, NULL);
}


vectorcallfunc
PyVectorcall_Function(PyObject *callable)
{
    return _PyVectorcall_FunctionInline(callable);
}


static PyObject *
_PyVectorcall_Call(PyThreadState *tstate, vectorcallfunc func,
                   PyObject *callable, PyObject *tuple, PyObject *kwargs)
{
    assert(func != NULL);

    Py_ssize_t nargs = PyTuple_GET_SIZE(tuple);

    /* Fast path for no keywords */
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return func(callable, _PyTuple_ITEMS(tuple), nargs, NULL);
    }

    /* Convert arguments & call */
    PyObject *const *args;
    PyObject *kwnames;
    args = _PyStack_UnpackDict(tstate,
                               _PyTuple_ITEMS(tuple), nargs,
                               kwargs, &kwnames);
    if (args == NULL) {
        return NULL;
    }
    PyObject *result = func(callable, args,
                            nargs | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames);
    _PyStack_UnpackDict_Free(args, nargs, kwnames);

    return _Py_CheckFunctionResult(tstate, callable, result, NULL);
}


PyObject *
PyVectorcall_Call(PyObject *callable, PyObject *tuple, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();

    /* get vectorcallfunc as in _PyVectorcall_Function, but without
     * the Py_TPFLAGS_HAVE_VECTORCALL check */
    Py_ssize_t offset = Py_TYPE(callable)->tp_vectorcall_offset;
    if (offset <= 0) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object does not support vectorcall",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }
    assert(PyCallable_Check(callable));

    vectorcallfunc func;
    memcpy(&func, (char *) callable + offset, sizeof(func));
    if (func == NULL) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object does not support vectorcall",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }

    return _PyVectorcall_Call(tstate, func, callable, tuple, kwargs);
}


PyObject *
PyObject_Vectorcall(PyObject *callable, PyObject *const *args,
                     size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_VectorcallTstate(tstate, callable,
                                      args, nargsf, kwnames);
}


PyObject *
_PyObject_Call(PyThreadState *tstate, PyObject *callable,
               PyObject *args, PyObject *kwargs)
{
    ternaryfunc call;
    PyObject *result;

    /* PyObject_Call() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
    assert(PyTuple_Check(args));
    assert(kwargs == NULL || PyDict_Check(kwargs));
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    vectorcallfunc vector_func = PyVectorcall_Function(callable);
    if (vector_func != NULL) {
        return _PyVectorcall_Call(tstate, vector_func, callable, args, kwargs);
    }
    else {
        call = Py_TYPE(callable)->tp_call;
        if (call == NULL) {
            object_is_not_callable(tstate, callable);
            return NULL;
        }

        if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
            return NULL;
        }

        result = (*call)(callable, args, kwargs);

        _Py_LeaveRecursiveCallTstate(tstate);

        return _Py_CheckFunctionResult(tstate, callable, result, NULL);
    }
}

PyObject *
PyObject_Call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_Call(tstate, callable, args, kwargs);
}


/* Function removed in the Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject *)
PyCFunction_Call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    return PyObject_Call(callable, args, kwargs);
}


PyObject *
PyObject_CallOneArg(PyObject *func, PyObject *arg)
{
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, func);
    assert(arg != NULL);
    PyObject *_args[2];
    PyObject **args = _args + 1;  // For PY_VECTORCALL_ARGUMENTS_OFFSET
    args[0] = arg;
    PyThreadState *tstate = _PyThreadState_GET();
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return _PyObject_VectorcallTstate(tstate, func, args, nargsf, NULL);
}


/* --- PyFunction call functions ---------------------------------- */

PyObject *
_PyFunction_Vectorcall(PyObject *func, PyObject* const* stack,
                       size_t nargsf, PyObject *kwnames)
{
    assert(PyFunction_Check(func));
    PyFunctionObject *f = (PyFunctionObject *)func;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    PyThreadState *tstate = _PyThreadState_GET();
    assert(nargs == 0 || stack != NULL);
    EVAL_CALL_STAT_INC(EVAL_CALL_FUNCTION_VECTORCALL);
    if (((PyCodeObject *)f->func_code)->co_flags & CO_OPTIMIZED) {
        return _PyEval_Vector(tstate, f, NULL, stack, nargs, kwnames);
    }
    else {
        return _PyEval_Vector(tstate, f, f->func_globals, stack, nargs, kwnames);
    }
}

/* --- More complex call functions -------------------------------- */

/* External interface to call any callable object.
   The args must be a tuple or NULL.  The kwargs must be a dict or NULL.
   Function removed in Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject*)
PyEval_CallObjectWithKeywords(PyObject *callable,
                              PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyEval_CallObjectWithKeywords() must not be called with an exception
       set. It raises a new exception if parameters are invalid or if
       PyTuple_New() fails, and so the original exception is lost. */
    assert(!_PyErr_Occurred(tstate));
#endif

    if (args != NULL && !PyTuple_Check(args)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "argument list must be a tuple");
        return NULL;
    }

    if (kwargs != NULL && !PyDict_Check(kwargs)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "keyword list must be a dictionary");
        return NULL;
    }

    if (args == NULL) {
        return _PyObject_VectorcallDictTstate(tstate, callable,
                                              NULL, 0, kwargs);
    }
    else {
        return _PyObject_Call(tstate, callable, args, kwargs);
    }
}


PyObject *
PyObject_CallObject(PyObject *callable, PyObject *args)
{
    PyThreadState *tstate = _PyThreadState_GET();
    assert(!_PyErr_Occurred(tstate));
    if (args == NULL) {
        return _PyObject_CallNoArgsTstate(tstate, callable);
    }
    if (!PyTuple_Check(args)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "argument list must be a tuple");
        return NULL;
    }
    return _PyObject_Call(tstate, callable, args, NULL);
}


/* Call callable(obj, *args, **kwargs). */
PyObject *
_PyObject_Call_Prepend(PyThreadState *tstate, PyObject *callable,
                       PyObject *obj, PyObject *args, PyObject *kwargs)
{
    assert(PyTuple_Check(args));

    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **stack;

    Py_ssize_t argcount = PyTuple_GET_SIZE(args);
    if (argcount + 1 <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc((argcount + 1) * sizeof(PyObject *));
        if (stack == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    /* use borrowed references */
    stack[0] = obj;
    memcpy(&stack[1],
           _PyTuple_ITEMS(args),
           argcount * sizeof(PyObject *));

    PyObject *result = _PyObject_VectorcallDictTstate(tstate, callable,
                                                      stack, argcount + 1,
                                                      kwargs);
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}


/* --- Call with a format string ---------------------------------- */

static PyObject *
_PyObject_CallFunctionVa(PyThreadState *tstate, PyObject *callable,
                         const char *format, va_list va)
{
    PyObject* small_stack[_PY_FASTCALL_SMALL_STACK];
    const Py_ssize_t small_stack_len = Py_ARRAY_LENGTH(small_stack);
    PyObject **stack;
    Py_ssize_t nargs, i;
    PyObject *result;

    if (callable == NULL) {
        return null_error(tstate);
    }

    if (!format || !*format) {
        return _PyObject_CallNoArgsTstate(tstate, callable);
    }

    stack = _Py_VaBuildStack(small_stack, small_stack_len,
                             format, va, &nargs);
    if (stack == NULL) {
        return NULL;
    }
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_API, callable);
    if (nargs == 1 && PyTuple_Check(stack[0])) {
        /* Special cases for backward compatibility:
           - PyObject_CallFunction(func, "O", tuple) calls func(*tuple)
           - PyObject_CallFunction(func, "(OOO)", arg1, arg2, arg3) calls
             func(*(arg1, arg2, arg3)): func(arg1, arg2, arg3) */
        PyObject *args = stack[0];
        result = _PyObject_VectorcallTstate(tstate, callable,
                                            _PyTuple_ITEMS(args),
                                            PyTuple_GET_SIZE(args),
                                            NULL);
    }
    else {
        result = _PyObject_VectorcallTstate(tstate, callable,
                                            stack, nargs, NULL);
    }

    for (i = 0; i < nargs; ++i) {
        Py_DECREF(stack[i]);
    }
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}


PyObject *
PyObject_CallFunction(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *result;
    PyThreadState *tstate = _PyThreadState_GET();

    va_start(va, format);
    result = _PyObject_CallFunctionVa(tstate, callable, format, va);
    va_end(va);

    return result;
}


/* PyEval_CallFunction is exact copy of PyObject_CallFunction.
   Function removed in Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject*)
PyEval_CallFunction(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *result;
    PyThreadState *tstate = _PyThreadState_GET();

    va_start(va, format);
    result = _PyObject_CallFunctionVa(tstate, callable, format, va);
    va_end(va);

    return result;
}


/* _PyObject_CallFunction_SizeT is exact copy of PyObject_CallFunction.
 * This function must be kept because it is part of the stable ABI.
 */
PyAPI_FUNC(PyObject *)  /* abi_only */
_PyObject_CallFunction_SizeT(PyObject *callable, const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();

    va_list va;
    va_start(va, format);
    PyObject *result = _PyObject_CallFunctionVa(tstate, callable, format, va);
    va_end(va);

    return result;
}


static PyObject*
callmethod(PyThreadState *tstate, PyObject* callable, const char *format, va_list va)
{
    assert(callable != NULL);
    if (!PyCallable_Check(callable)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "attribute of type '%.200s' is not callable",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }

    return _PyObject_CallFunctionVa(tstate, callable, format, va);
}

PyObject *
PyObject_CallMethod(PyObject *obj, const char *name, const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *callable = PyObject_GetAttrString(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


/* PyEval_CallMethod is exact copy of PyObject_CallMethod.
   Function removed in Python 3.13 API but kept in the stable ABI. */
PyAPI_FUNC(PyObject*)
PyEval_CallMethod(PyObject *obj, const char *name, const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *callable = PyObject_GetAttrString(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject *
_PyObject_CallMethod(PyObject *obj, PyObject *name,
                     const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *callable = PyObject_GetAttr(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject *
_PyObject_CallMethodId(PyObject *obj, _Py_Identifier *name,
                       const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PyObject *callable = _PyObject_GetAttrId(obj, name);
_Py_COMP_DIAG_POP
    if (callable == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject * _PyObject_CallMethodFormat(PyThreadState *tstate, PyObject *callable,
                                      const char *format, ...)
{
    assert(callable != NULL);
    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va);
    va_end(va);
    return retval;
}


// _PyObject_CallMethod_SizeT is exact copy of PyObject_CallMethod.
// This function must be kept because it is part of the stable ABI.
PyAPI_FUNC(PyObject *)  /* abi_only */
_PyObject_CallMethod_SizeT(PyObject *obj, const char *name,
                           const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *callable = PyObject_GetAttrString(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


/* --- Call with "..." arguments ---------------------------------- */

static PyObject *
object_vacall(PyThreadState *tstate, PyObject *base,
              PyObject *callable, va_list vargs)
{
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **stack;
    Py_ssize_t nargs;
    PyObject *result;
    Py_ssize_t i;
    va_list countva;

    if (callable == NULL) {
        return null_error(tstate);
    }

    /* Count the number of arguments */
    va_copy(countva, vargs);
    nargs = base ? 1 : 0;
    while (1) {
        PyObject *arg = va_arg(countva, PyObject *);
        if (arg == NULL) {
            break;
        }
        nargs++;
    }
    va_end(countva);

    /* Copy arguments */
    if (nargs <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc(nargs * sizeof(stack[0]));
        if (stack == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    i = 0;
    if (base) {
        stack[i++] = base;
    }

    for (; i < nargs; ++i) {
        stack[i] = va_arg(vargs, PyObject *);
    }

#ifdef Py_STATS
    if (PyFunction_Check(callable)) {
        EVAL_CALL_STAT_INC(EVAL_CALL_API);
    }
#endif
    /* Call the function */
    result = _PyObject_VectorcallTstate(tstate, callable, stack, nargs, NULL);

    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}


PyObject *
PyObject_VectorcallMethod(PyObject *name, PyObject *const *args,
                           size_t nargsf, PyObject *kwnames)
{
    assert(name != NULL);
    assert(args != NULL);
    assert(PyVectorcall_NARGS(nargsf) >= 1);

    PyThreadState *tstate = _PyThreadState_GET();
    _PyCStackRef method;
    _PyThreadState_PushCStackRef(tstate, &method);
    /* Use args[0] as "self" argument */
    int unbound = _PyObject_GetMethodStackRef(tstate, args[0], name, &method.ref);
    if (PyStackRef_IsNull(method.ref)) {
        _PyThreadState_PopCStackRef(tstate, &method);
        return NULL;
    }
    PyObject *callable = PyStackRef_AsPyObjectBorrow(method.ref);

    if (unbound) {
        /* We must remove PY_VECTORCALL_ARGUMENTS_OFFSET since
         * that would be interpreted as allowing to change args[-1] */
        nargsf &= ~PY_VECTORCALL_ARGUMENTS_OFFSET;
    }
    else {
        /* Skip "self". We can keep PY_VECTORCALL_ARGUMENTS_OFFSET since
         * args[-1] in the onward call is args[0] here. */
        args++;
        nargsf--;
    }
    EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_METHOD, callable);
    PyObject *result = _PyObject_VectorcallTstate(tstate, callable,
                                                  args, nargsf, kwnames);
    _PyThreadState_PopCStackRef(tstate, &method);
    return result;
}


PyObject *
PyObject_CallMethodObjArgs(PyObject *obj, PyObject *name, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    _PyCStackRef method;
    _PyThreadState_PushCStackRef(tstate, &method);
    int is_method = _PyObject_GetMethodStackRef(tstate, obj, name, &method.ref);
    if (PyStackRef_IsNull(method.ref)) {
        _PyThreadState_PopCStackRef(tstate, &method);
        return NULL;
    }
    PyObject *callable = PyStackRef_AsPyObjectBorrow(method.ref);
    obj = is_method ? obj : NULL;

    va_list vargs;
    va_start(vargs, name);
    PyObject *result = object_vacall(tstate, obj, callable, vargs);
    va_end(vargs);

    _PyThreadState_PopCStackRef(tstate, &method);
    return result;
}


PyObject *
PyObject_CallFunctionObjArgs(PyObject *callable, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    va_list vargs;
    PyObject *result;

    va_start(vargs, callable);
    result = object_vacall(tstate, NULL, callable, vargs);
    va_end(vargs);

    return result;
}


/* --- PyStack functions ------------------------------------------ */

PyObject *
_PyStack_AsDict(PyObject *const *values, PyObject *kwnames)
{
    Py_ssize_t nkwargs;

    assert(kwnames != NULL);
    nkwargs = PyTuple_GET_SIZE(kwnames);
    return _PyDict_FromItems(&PyTuple_GET_ITEM(kwnames, 0), 1,
                             values, 1, nkwargs);
}


/* Convert (args, nargs, kwargs: dict) into a (stack, nargs, kwnames: tuple).

   Allocate a new argument vector and keyword names tuple. Return the argument
   vector; return NULL with exception set on error. Return the keyword names
   tuple in *p_kwnames.

   This also checks that all keyword names are strings. If not, a TypeError is
   raised.

   The newly allocated argument vector supports PY_VECTORCALL_ARGUMENTS_OFFSET.

   The positional arguments are borrowed references from the input array
   (which must be kept alive by the caller). The keyword argument values
   are new references.

   When done, you must call _PyStack_UnpackDict_Free(stack, nargs, kwnames) */
PyObject *const *
_PyStack_UnpackDict(PyThreadState *tstate,
                    PyObject *const *args, Py_ssize_t nargs,
                    PyObject *kwargs, PyObject **p_kwnames)
{
    assert(nargs >= 0);
    assert(kwargs != NULL);
    assert(PyDict_Check(kwargs));

    Py_ssize_t nkwargs = PyDict_GET_SIZE(kwargs);
    /* Check for overflow in the PyMem_Malloc() call below. The subtraction
     * in this check cannot overflow: both maxnargs and nkwargs are
     * non-negative signed integers, so their difference fits in the type. */
    Py_ssize_t maxnargs = PY_SSIZE_T_MAX / sizeof(args[0]) - 1;
    if (nargs > maxnargs - nkwargs) {
        _PyErr_NoMemory(tstate);
        return NULL;
    }

    /* Add 1 to support PY_VECTORCALL_ARGUMENTS_OFFSET */
    PyObject **stack = PyMem_Malloc((1 + nargs + nkwargs) * sizeof(args[0]));
    if (stack == NULL) {
        _PyErr_NoMemory(tstate);
        return NULL;
    }

    PyObject *kwnames = PyTuple_New(nkwargs);
    if (kwnames == NULL) {
        PyMem_Free(stack);
        return NULL;
    }

    stack++;  /* For PY_VECTORCALL_ARGUMENTS_OFFSET */

    /* Copy positional arguments (borrowed references) */
    for (Py_ssize_t i = 0; i < nargs; i++) {
        stack[i] = args[i];
    }

    PyObject **kwstack = stack + nargs;
    /* This loop doesn't support lookup function mutating the dictionary
       to change its size. It's a deliberate choice for speed, this function is
       called in the performance critical hot code. */
    Py_ssize_t pos = 0, i = 0;
    PyObject *key, *value;
    unsigned long keys_are_strings = Py_TPFLAGS_UNICODE_SUBCLASS;
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        keys_are_strings &= Py_TYPE(key)->tp_flags;
        PyTuple_SET_ITEM(kwnames, i, Py_NewRef(key));
        kwstack[i] = Py_NewRef(value);
        i++;
    }

    /* keys_are_strings has the value Py_TPFLAGS_UNICODE_SUBCLASS if that
     * flag is set for all keys. Otherwise, keys_are_strings equals 0.
     * We do this check once at the end instead of inside the loop above
     * because it simplifies the deallocation in the failing case.
     * It happens to also make the loop above slightly more efficient. */
    if (!keys_are_strings) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "keywords must be strings");
        _PyStack_UnpackDict_Free(stack, nargs, kwnames);
        return NULL;
    }

    *p_kwnames = kwnames;
    return stack;
}

void
_PyStack_UnpackDict_Free(PyObject *const *stack, Py_ssize_t nargs,
                         PyObject *kwnames)
{
    /* Only decref kwargs values, positional args are borrowed */
    Py_ssize_t nkwargs = PyTuple_GET_SIZE(kwnames);
    for (Py_ssize_t i = 0; i < nkwargs; i++) {
        Py_DECREF(stack[nargs + i]);
    }
    _PyStack_UnpackDict_FreeNoDecRef(stack, kwnames);
}

void
_PyStack_UnpackDict_FreeNoDecRef(PyObject *const *stack, PyObject *kwnames)
{
    PyMem_Free((PyObject **)stack - 1);
    Py_DECREF(kwnames);
}

// Export for the stable ABI
#undef PyVectorcall_NARGS
Py_ssize_t
PyVectorcall_NARGS(size_t n)
{
    return _PyVectorcall_NARGS(n);
}
