#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgTstate()
#include "pycore_ceval.h"         // _PyEval_EvalFrame()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "frameobject.h"          // _PyFrame_New_NoTrack()


static PyObject *const *
_PyStack_UnpackDict(PyThreadState *tstate,
                    PyObject *const *args, Py_ssize_t nargs,
                    PyObject *kwargs, PyObject **p_kwnames);

static void
_PyStack_UnpackDict_Free(PyObject *const *stack, Py_ssize_t nargs,
                         PyObject *kwnames);


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
                              "%R returned NULL without setting an error",
                              callable);
            else
                _PyErr_Format(tstate, PyExc_SystemError,
                              "%s returned NULL without setting an error",
                              where);
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode.
               Py_FatalError() logs the SystemError exception raised above. */
            Py_FatalError("a function returned NULL without setting an error");
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
                    "%R returned a result with an error set", callable);
            }
            else {
                _PyErr_FormatFromCauseTstate(
                    tstate, PyExc_SystemError,
                    "%s returned a result with an error set", where);
            }
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode.
               Py_FatalError() logs the SystemError exception raised above. */
            Py_FatalError("a function returned a result with an error set");
#endif
            return NULL;
        }
    }
    return result;
}


/* --- Core PyObject call functions ------------------------------- */

/* Call a callable Python object without any arguments */
PyObject *
PyObject_CallNoArgs(PyObject *func)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_CallNoArgTstate(tstate, func);
}


PyObject *
_PyObject_FastCallDictTstate(PyThreadState *tstate, PyObject *callable,
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
    return _PyObject_FastCallDictTstate(tstate, callable, args, nargsf, kwargs);
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
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object is not callable",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }

    PyObject *argstuple = _PyTuple_FromArray(args, nargs);
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
    if (_Py_EnterRecursiveCall(tstate, " while calling a Python object") == 0)
    {
        result = call(callable, argstuple, kwdict);
        _Py_LeaveRecursiveCall(tstate);
    }

    Py_DECREF(argstuple);
    if (kwdict != keywords) {
        Py_DECREF(kwdict);
    }

    return _Py_CheckFunctionResult(tstate, callable, result, NULL);
}


PyObject *
PyVectorcall_Call(PyObject *callable, PyObject *tuple, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    vectorcallfunc func;

    /* get vectorcallfunc as in PyVectorcall_Function, but without
     * the Py_TPFLAGS_HAVE_VECTORCALL check */
    Py_ssize_t offset = Py_TYPE(callable)->tp_vectorcall_offset;
    if (offset <= 0) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object does not support vectorcall",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }
    memcpy(&func, (char *) callable + offset, sizeof(func));
    if (func == NULL) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%.200s' object does not support vectorcall",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }

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

    if (PyVectorcall_Function(callable) != NULL) {
        return PyVectorcall_Call(callable, args, kwargs);
    }
    else {
        call = Py_TYPE(callable)->tp_call;
        if (call == NULL) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "'%.200s' object is not callable",
                          Py_TYPE(callable)->tp_name);
            return NULL;
        }

        if (_Py_EnterRecursiveCall(tstate, " while calling a Python object")) {
            return NULL;
        }

        result = (*call)(callable, args, kwargs);

        _Py_LeaveRecursiveCall(tstate);

        return _Py_CheckFunctionResult(tstate, callable, result, NULL);
    }
}

PyObject *
PyObject_Call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_Call(tstate, callable, args, kwargs);
}


PyObject *
PyCFunction_Call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return _PyObject_Call(tstate, callable, args, kwargs);
}


/* --- PyFunction call functions ---------------------------------- */

static PyObject* _Py_HOT_FUNCTION
function_code_fastcall(PyThreadState *tstate, PyCodeObject *co,
                       PyObject *const *args, Py_ssize_t nargs,
                       PyObject *globals)
{
    assert(tstate != NULL);
    assert(globals != NULL);

    /* XXX Perhaps we should create a specialized
       _PyFrame_New_NoTrack() that doesn't take locals, but does
       take builtins without sanity checking them.
       */
    PyFrameObject *f = _PyFrame_New_NoTrack(tstate, co, globals, NULL);
    if (f == NULL) {
        return NULL;
    }

    PyObject **fastlocals = f->f_localsplus;

    for (Py_ssize_t i = 0; i < nargs; i++) {
        Py_INCREF(*args);
        fastlocals[i] = *args++;
    }
    PyObject *result = _PyEval_EvalFrame(tstate, f, 0);

    if (Py_REFCNT(f) > 1) {
        Py_DECREF(f);
        _PyObject_GC_TRACK(f);
    }
    else {
        ++tstate->recursion_depth;
        Py_DECREF(f);
        --tstate->recursion_depth;
    }
    return result;
}


PyObject *
_PyFunction_Vectorcall(PyObject *func, PyObject* const* stack,
                       size_t nargsf, PyObject *kwnames)
{
    assert(PyFunction_Check(func));
    assert(kwnames == NULL || PyTuple_CheckExact(kwnames));

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
    assert((nargs == 0 && nkwargs == 0) || stack != NULL);
    /* kwnames must only contain strings and all keys must be unique */

    PyThreadState *tstate = _PyThreadState_GET();
    PyCodeObject *co = (PyCodeObject *)PyFunction_GET_CODE(func);
    PyObject *globals = PyFunction_GET_GLOBALS(func);
    PyObject *argdefs = PyFunction_GET_DEFAULTS(func);

    if (co->co_kwonlyargcount == 0 && nkwargs == 0 &&
        (co->co_flags & ~PyCF_MASK) == (CO_OPTIMIZED | CO_NEWLOCALS | CO_NOFREE))
    {
        if (argdefs == NULL && co->co_argcount == nargs) {
            return function_code_fastcall(tstate, co, stack, nargs, globals);
        }
        else if (nargs == 0 && argdefs != NULL
                 && co->co_argcount == PyTuple_GET_SIZE(argdefs)) {
            /* function called with no arguments, but all parameters have
               a default value: use default values as arguments .*/
            stack = _PyTuple_ITEMS(argdefs);
            return function_code_fastcall(tstate, co,
                                          stack, PyTuple_GET_SIZE(argdefs),
                                          globals);
        }
    }

    PyObject *kwdefs = PyFunction_GET_KW_DEFAULTS(func);
    PyObject *closure = PyFunction_GET_CLOSURE(func);
    PyObject *name = ((PyFunctionObject *)func) -> func_name;
    PyObject *qualname = ((PyFunctionObject *)func) -> func_qualname;

    PyObject **d;
    Py_ssize_t nd;
    if (argdefs != NULL) {
        d = _PyTuple_ITEMS(argdefs);
        nd = PyTuple_GET_SIZE(argdefs);
        assert(nd <= INT_MAX);
    }
    else {
        d = NULL;
        nd = 0;
    }
    return _PyEval_EvalCode(tstate,
                (PyObject*)co, globals, (PyObject *)NULL,
                stack, nargs,
                nkwargs ? _PyTuple_ITEMS(kwnames) : NULL,
                stack + nargs,
                nkwargs, 1,
                d, (int)nd, kwdefs,
                closure, name, qualname);
}


/* --- More complex call functions -------------------------------- */

/* External interface to call any callable object.
   The args must be a tuple or NULL.  The kwargs must be a dict or NULL. */
PyObject *
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
        return _PyObject_FastCallDictTstate(tstate, callable, NULL, 0, kwargs);
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
        return _PyObject_CallNoArgTstate(tstate, callable);
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

    PyObject *result = _PyObject_FastCallDictTstate(tstate, callable,
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
                         const char *format, va_list va, int is_size_t)
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
        return _PyObject_CallNoArgTstate(tstate, callable);
    }

    if (is_size_t) {
        stack = _Py_VaBuildStack_SizeT(small_stack, small_stack_len,
                                       format, va, &nargs);
    }
    else {
        stack = _Py_VaBuildStack(small_stack, small_stack_len,
                                 format, va, &nargs);
    }
    if (stack == NULL) {
        return NULL;
    }

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
    result = _PyObject_CallFunctionVa(tstate, callable, format, va, 0);
    va_end(va);

    return result;
}


/* PyEval_CallFunction is exact copy of PyObject_CallFunction.
 * This function is kept for backward compatibility.
 */
PyObject *
PyEval_CallFunction(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *result;
    PyThreadState *tstate = _PyThreadState_GET();

    va_start(va, format);
    result = _PyObject_CallFunctionVa(tstate, callable, format, va, 0);
    va_end(va);

    return result;
}


PyObject *
_PyObject_CallFunction_SizeT(PyObject *callable, const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();

    va_list va;
    va_start(va, format);
    PyObject *result = _PyObject_CallFunctionVa(tstate, callable, format, va, 1);
    va_end(va);

    return result;
}


static PyObject*
callmethod(PyThreadState *tstate, PyObject* callable, const char *format, va_list va, int is_size_t)
{
    assert(callable != NULL);
    if (!PyCallable_Check(callable)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "attribute of type '%.200s' is not callable",
                      Py_TYPE(callable)->tp_name);
        return NULL;
    }

    return _PyObject_CallFunctionVa(tstate, callable, format, va, is_size_t);
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
    PyObject *retval = callmethod(tstate, callable, format, va, 0);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


/* PyEval_CallMethod is exact copy of PyObject_CallMethod.
 * This function is kept for backward compatibility.
 */
PyObject *
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
    PyObject *retval = callmethod(tstate, callable, format, va, 0);
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

    PyObject *callable = _PyObject_GetAttrId(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va, 0);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject *
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
    PyObject *retval = callmethod(tstate, callable, format, va, 1);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject *
_PyObject_CallMethodId_SizeT(PyObject *obj, _Py_Identifier *name,
                             const char *format, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *callable = _PyObject_GetAttrId(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_list va;
    va_start(va, format);
    PyObject *retval = callmethod(tstate, callable, format, va, 1);
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
    PyObject *callable = NULL;
    /* Use args[0] as "self" argument */
    int unbound = _PyObject_GetMethod(args[0], name, &callable);
    if (callable == NULL) {
        return NULL;
    }

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
    PyObject *result = _PyObject_VectorcallTstate(tstate, callable,
                                                  args, nargsf, kwnames);
    Py_DECREF(callable);
    return result;
}


PyObject *
PyObject_CallMethodObjArgs(PyObject *obj, PyObject *name, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *callable = NULL;
    int is_method = _PyObject_GetMethod(obj, name, &callable);
    if (callable == NULL) {
        return NULL;
    }
    obj = is_method ? obj : NULL;

    va_list vargs;
    va_start(vargs, name);
    PyObject *result = object_vacall(tstate, obj, callable, vargs);
    va_end(vargs);

    Py_DECREF(callable);
    return result;
}


PyObject *
_PyObject_CallMethodIdObjArgs(PyObject *obj,
                              struct _Py_Identifier *name, ...)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (obj == NULL || name == NULL) {
        return null_error(tstate);
    }

    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
    if (!oname) {
        return NULL;
    }

    PyObject *callable = NULL;
    int is_method = _PyObject_GetMethod(obj, oname, &callable);
    if (callable == NULL) {
        return NULL;
    }
    obj = is_method ? obj : NULL;

    va_list vargs;
    va_start(vargs, name);
    PyObject *result = object_vacall(tstate, obj, callable, vargs);
    va_end(vargs);

    Py_DECREF(callable);
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
    PyObject *kwdict;
    Py_ssize_t i;

    assert(kwnames != NULL);
    nkwargs = PyTuple_GET_SIZE(kwnames);
    kwdict = _PyDict_NewPresized(nkwargs);
    if (kwdict == NULL) {
        return NULL;
    }

    for (i = 0; i < nkwargs; i++) {
        PyObject *key = PyTuple_GET_ITEM(kwnames, i);
        PyObject *value = *values++;
        /* If key already exists, replace it with the new value */
        if (PyDict_SetItem(kwdict, key, value)) {
            Py_DECREF(kwdict);
            return NULL;
        }
    }
    return kwdict;
}


/* Convert (args, nargs, kwargs: dict) into a (stack, nargs, kwnames: tuple).

   Allocate a new argument vector and keyword names tuple. Return the argument
   vector; return NULL with exception set on error. Return the keyword names
   tuple in *p_kwnames.

   This also checks that all keyword names are strings. If not, a TypeError is
   raised.

   The newly allocated argument vector supports PY_VECTORCALL_ARGUMENTS_OFFSET.

   When done, you must call _PyStack_UnpackDict_Free(stack, nargs, kwnames) */
static PyObject *const *
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

    /* Copy positional arguments */
    for (Py_ssize_t i = 0; i < nargs; i++) {
        Py_INCREF(args[i]);
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
        Py_INCREF(key);
        Py_INCREF(value);
        PyTuple_SET_ITEM(kwnames, i, key);
        kwstack[i] = value;
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

static void
_PyStack_UnpackDict_Free(PyObject *const *stack, Py_ssize_t nargs,
                         PyObject *kwnames)
{
    Py_ssize_t n = PyTuple_GET_SIZE(kwnames) + nargs;
    for (Py_ssize_t i = 0; i < n; i++) {
        Py_DECREF(stack[i]);
    }
    PyMem_Free((PyObject **)stack - 1);
    Py_DECREF(kwnames);
}
