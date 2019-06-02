#include "Python.h"
#include "pycore_object.h"
#include "pycore_pystate.h"
#include "pycore_tupleobject.h"
#include "frameobject.h"


static PyObject *
cfunction_call_varargs(PyObject *func, PyObject *args, PyObject *kwargs);


static PyObject *
null_error(void)
{
    if (!PyErr_Occurred())
        PyErr_SetString(PyExc_SystemError,
                        "null argument to internal routine");
    return NULL;
}


PyObject*
_Py_CheckFunctionResult(PyObject *callable, PyObject *result, const char *where)
{
    int err_occurred = (PyErr_Occurred() != NULL);

    assert((callable != NULL) ^ (where != NULL));

    if (result == NULL) {
        if (!err_occurred) {
            if (callable)
                PyErr_Format(PyExc_SystemError,
                             "%R returned NULL without setting an error",
                             callable);
            else
                PyErr_Format(PyExc_SystemError,
                             "%s returned NULL without setting an error",
                             where);
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode */
            Py_FatalError("a function returned NULL without setting an error");
#endif
            return NULL;
        }
    }
    else {
        if (err_occurred) {
            Py_DECREF(result);

            if (callable) {
                _PyErr_FormatFromCause(PyExc_SystemError,
                        "%R returned a result with an error set",
                        callable);
            }
            else {
                _PyErr_FormatFromCause(PyExc_SystemError,
                        "%s returned a result with an error set",
                        where);
            }
#ifdef Py_DEBUG
            /* Ensure that the bug is caught in debug mode */
            Py_FatalError("a function returned a result with an error set");
#endif
            return NULL;
        }
    }
    return result;
}


/* --- Core PyObject call functions ------------------------------- */

PyObject *
_PyObject_FastCallDict(PyObject *callable, PyObject *const *args,
                       size_t nargsf, PyObject *kwargs)
{
    /* _PyObject_FastCallDict() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());
    assert(callable != NULL);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(kwargs == NULL || PyDict_Check(kwargs));

    vectorcallfunc func = _PyVectorcall_Function(callable);
    if (func == NULL) {
        /* Use tp_call instead */
        return _PyObject_MakeTpCall(callable, args, nargs, kwargs);
    }

    PyObject *res;
    if (kwargs == NULL) {
        res = func(callable, args, nargsf, NULL);
    }
    else {
        PyObject *kwnames;
        PyObject *const *newargs;
        if (_PyStack_UnpackDict(args, nargs, kwargs, &newargs, &kwnames) < 0) {
            return NULL;
        }
        res = func(callable, newargs, nargs, kwnames);
        if (kwnames != NULL) {
            Py_ssize_t i, n = PyTuple_GET_SIZE(kwnames) + nargs;
            for (i = 0; i < n; i++) {
                Py_DECREF(newargs[i]);
            }
            PyMem_Free((PyObject **)newargs);
            Py_DECREF(kwnames);
        }
    }
    return _Py_CheckFunctionResult(callable, res, NULL);
}


PyObject *
_PyObject_MakeTpCall(PyObject *callable, PyObject *const *args, Py_ssize_t nargs, PyObject *keywords)
{
    /* Slow path: build a temporary tuple for positional arguments and a
     * temporary dictionary for keyword arguments (if any) */
    ternaryfunc call = Py_TYPE(callable)->tp_call;
    if (call == NULL) {
        PyErr_Format(PyExc_TypeError, "'%.200s' object is not callable",
                     Py_TYPE(callable)->tp_name);
        return NULL;
    }

    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(keywords == NULL || PyTuple_Check(keywords) || PyDict_Check(keywords));
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
    if (Py_EnterRecursiveCall(" while calling a Python object") == 0)
    {
        result = call(callable, argstuple, kwdict);
        Py_LeaveRecursiveCall();
    }

    Py_DECREF(argstuple);
    if (kwdict != keywords) {
        Py_DECREF(kwdict);
    }

    result = _Py_CheckFunctionResult(callable, result, NULL);
    return result;
}


PyObject *
PyVectorcall_Call(PyObject *callable, PyObject *tuple, PyObject *kwargs)
{
    /* get vectorcallfunc as in _PyVectorcall_Function, but without
     * the _Py_TPFLAGS_HAVE_VECTORCALL check */
    Py_ssize_t offset = Py_TYPE(callable)->tp_vectorcall_offset;
    if ((offset <= 0) || (!Py_TYPE(callable)->tp_call)) {
        PyErr_Format(PyExc_TypeError, "'%.200s' object does not support vectorcall",
                     Py_TYPE(callable)->tp_name);
        return NULL;
    }
    vectorcallfunc func = *(vectorcallfunc *)(((char *)callable) + offset);
    if (func == NULL) {
        PyErr_Format(PyExc_TypeError, "'%.200s' object does not support vectorcall",
                     Py_TYPE(callable)->tp_name);
        return NULL;
    }

    /* Convert arguments & call */
    PyObject *const *args;
    Py_ssize_t nargs = PyTuple_GET_SIZE(tuple);
    PyObject *kwnames;
    if (_PyStack_UnpackDict(_PyTuple_ITEMS(tuple), nargs,
        kwargs, &args, &kwnames) < 0) {
        return NULL;
    }
    PyObject *result = func(callable, args, nargs, kwnames);
    if (kwnames != NULL) {
        Py_ssize_t i, n = PyTuple_GET_SIZE(kwnames) + nargs;
        for (i = 0; i < n; i++) {
            Py_DECREF(args[i]);
        }
        PyMem_Free((PyObject **)args);
        Py_DECREF(kwnames);
    }

    return result;
}


PyObject *
PyObject_Call(PyObject *callable, PyObject *args, PyObject *kwargs)
{
    ternaryfunc call;
    PyObject *result;

    /* PyObject_Call() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());
    assert(PyTuple_Check(args));
    assert(kwargs == NULL || PyDict_Check(kwargs));

    if (_PyVectorcall_Function(callable) != NULL) {
        return PyVectorcall_Call(callable, args, kwargs);
    }
    else if (PyCFunction_Check(callable)) {
        /* This must be a METH_VARARGS function, otherwise we would be
         * in the previous case */
        return cfunction_call_varargs(callable, args, kwargs);
    }
    else {
        call = callable->ob_type->tp_call;
        if (call == NULL) {
            PyErr_Format(PyExc_TypeError, "'%.200s' object is not callable",
                         callable->ob_type->tp_name);
            return NULL;
        }

        if (Py_EnterRecursiveCall(" while calling a Python object"))
            return NULL;

        result = (*call)(callable, args, kwargs);

        Py_LeaveRecursiveCall();

        return _Py_CheckFunctionResult(callable, result, NULL);
    }
}


/* --- PyFunction call functions ---------------------------------- */

static PyObject* _Py_HOT_FUNCTION
function_code_fastcall(PyCodeObject *co, PyObject *const *args, Py_ssize_t nargs,
                       PyObject *globals)
{
    PyFrameObject *f;
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject **fastlocals;
    Py_ssize_t i;
    PyObject *result;

    assert(globals != NULL);
    /* XXX Perhaps we should create a specialized
       _PyFrame_New_NoTrack() that doesn't take locals, but does
       take builtins without sanity checking them.
       */
    assert(tstate != NULL);
    f = _PyFrame_New_NoTrack(tstate, co, globals, NULL);
    if (f == NULL) {
        return NULL;
    }

    fastlocals = f->f_localsplus;

    for (i = 0; i < nargs; i++) {
        Py_INCREF(*args);
        fastlocals[i] = *args++;
    }
    result = PyEval_EvalFrameEx(f,0);

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
_PyFunction_FastCallDict(PyObject *func, PyObject *const *args, Py_ssize_t nargs,
                         PyObject *kwargs)
{
    PyCodeObject *co = (PyCodeObject *)PyFunction_GET_CODE(func);
    PyObject *globals = PyFunction_GET_GLOBALS(func);
    PyObject *argdefs = PyFunction_GET_DEFAULTS(func);
    PyObject *kwdefs, *closure, *name, *qualname;
    PyObject *kwtuple, **k;
    PyObject **d;
    Py_ssize_t nd, nk;
    PyObject *result;

    assert(func != NULL);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(kwargs == NULL || PyDict_Check(kwargs));

    if (co->co_kwonlyargcount == 0 &&
        (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) &&
        (co->co_flags & ~PyCF_MASK) == (CO_OPTIMIZED | CO_NEWLOCALS | CO_NOFREE))
    {
        /* Fast paths */
        if (argdefs == NULL && co->co_argcount == nargs) {
            return function_code_fastcall(co, args, nargs, globals);
        }
        else if (nargs == 0 && argdefs != NULL
                 && co->co_argcount == PyTuple_GET_SIZE(argdefs)) {
            /* function called with no arguments, but all parameters have
               a default value: use default values as arguments .*/
            args = _PyTuple_ITEMS(argdefs);
            return function_code_fastcall(co, args, PyTuple_GET_SIZE(argdefs),
                                          globals);
        }
    }

    nk = (kwargs != NULL) ? PyDict_GET_SIZE(kwargs) : 0;
    if (nk != 0) {
        Py_ssize_t pos, i;

        /* bpo-29318, bpo-27840: Caller and callee functions must not share
           the dictionary: kwargs must be copied. */
        kwtuple = PyTuple_New(2 * nk);
        if (kwtuple == NULL) {
            return NULL;
        }

        k = _PyTuple_ITEMS(kwtuple);
        pos = i = 0;
        while (PyDict_Next(kwargs, &pos, &k[i], &k[i+1])) {
            /* We must hold strong references because keyword arguments can be
               indirectly modified while the function is called:
               see issue #2016 and test_extcall */
            Py_INCREF(k[i]);
            Py_INCREF(k[i+1]);
            i += 2;
        }
        assert(i / 2 == nk);
    }
    else {
        kwtuple = NULL;
        k = NULL;
    }

    kwdefs = PyFunction_GET_KW_DEFAULTS(func);
    closure = PyFunction_GET_CLOSURE(func);
    name = ((PyFunctionObject *)func) -> func_name;
    qualname = ((PyFunctionObject *)func) -> func_qualname;

    if (argdefs != NULL) {
        d = _PyTuple_ITEMS(argdefs);
        nd = PyTuple_GET_SIZE(argdefs);
    }
    else {
        d = NULL;
        nd = 0;
    }

    result = _PyEval_EvalCodeWithName((PyObject*)co, globals, (PyObject *)NULL,
                                      args, nargs,
                                      k, k != NULL ? k + 1 : NULL, nk, 2,
                                      d, nd, kwdefs,
                                      closure, name, qualname);
    Py_XDECREF(kwtuple);
    return result;
}


PyObject *
_PyFunction_Vectorcall(PyObject *func, PyObject* const* stack,
                       size_t nargsf, PyObject *kwnames)
{
    PyCodeObject *co = (PyCodeObject *)PyFunction_GET_CODE(func);
    PyObject *globals = PyFunction_GET_GLOBALS(func);
    PyObject *argdefs = PyFunction_GET_DEFAULTS(func);
    PyObject *kwdefs, *closure, *name, *qualname;
    PyObject **d;
    Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
    Py_ssize_t nd;

    assert(PyFunction_Check(func));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    assert(nargs >= 0);
    assert(kwnames == NULL || PyTuple_CheckExact(kwnames));
    assert((nargs == 0 && nkwargs == 0) || stack != NULL);
    /* kwnames must only contains str strings, no subclass, and all keys must
       be unique */

    if (co->co_kwonlyargcount == 0 && nkwargs == 0 &&
        (co->co_flags & ~PyCF_MASK) == (CO_OPTIMIZED | CO_NEWLOCALS | CO_NOFREE))
    {
        if (argdefs == NULL && co->co_argcount == nargs) {
            return function_code_fastcall(co, stack, nargs, globals);
        }
        else if (nargs == 0 && argdefs != NULL
                 && co->co_argcount == PyTuple_GET_SIZE(argdefs)) {
            /* function called with no arguments, but all parameters have
               a default value: use default values as arguments .*/
            stack = _PyTuple_ITEMS(argdefs);
            return function_code_fastcall(co, stack, PyTuple_GET_SIZE(argdefs),
                                          globals);
        }
    }

    kwdefs = PyFunction_GET_KW_DEFAULTS(func);
    closure = PyFunction_GET_CLOSURE(func);
    name = ((PyFunctionObject *)func) -> func_name;
    qualname = ((PyFunctionObject *)func) -> func_qualname;

    if (argdefs != NULL) {
        d = _PyTuple_ITEMS(argdefs);
        nd = PyTuple_GET_SIZE(argdefs);
    }
    else {
        d = NULL;
        nd = 0;
    }
    return _PyEval_EvalCodeWithName((PyObject*)co, globals, (PyObject *)NULL,
                                    stack, nargs,
                                    nkwargs ? _PyTuple_ITEMS(kwnames) : NULL,
                                    stack + nargs,
                                    nkwargs, 1,
                                    d, (int)nd, kwdefs,
                                    closure, name, qualname);
}


/* --- PyCFunction call functions --------------------------------- */

PyObject *
_PyMethodDef_RawFastCallDict(PyMethodDef *method, PyObject *self,
                             PyObject *const *args, Py_ssize_t nargs,
                             PyObject *kwargs)
{
    /* _PyMethodDef_RawFastCallDict() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());

    assert(method != NULL);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(kwargs == NULL || PyDict_Check(kwargs));

    PyCFunction meth = method->ml_meth;
    int flags = method->ml_flags & ~(METH_CLASS | METH_STATIC | METH_COEXIST);
    PyObject *result = NULL;

    if (Py_EnterRecursiveCall(" while calling a Python object")) {
        return NULL;
    }

    switch (flags)
    {
    case METH_NOARGS:
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            goto no_keyword_error;
        }

        if (nargs != 0) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes no arguments (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        result = (*meth) (self, NULL);
        break;

    case METH_O:
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            goto no_keyword_error;
        }

        if (nargs != 1) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes exactly one argument (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        result = (*meth) (self, args[0]);
        break;

    case METH_VARARGS:
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            goto no_keyword_error;
        }
        /* fall through */

    case METH_VARARGS | METH_KEYWORDS:
    {
        /* Slow-path: create a temporary tuple for positional arguments */
        PyObject *argstuple = _PyTuple_FromArray(args, nargs);
        if (argstuple == NULL) {
            goto exit;
        }

        if (flags & METH_KEYWORDS) {
            result = (*(PyCFunctionWithKeywords)(void(*)(void))meth) (self, argstuple, kwargs);
        }
        else {
            result = (*meth) (self, argstuple);
        }
        Py_DECREF(argstuple);
        break;
    }

    case METH_FASTCALL:
    {
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            goto no_keyword_error;
        }

        result = (*(_PyCFunctionFast)(void(*)(void))meth) (self, args, nargs);
        break;
    }

    case METH_FASTCALL | METH_KEYWORDS:
    {
        PyObject *const *stack;
        PyObject *kwnames;
        _PyCFunctionFastWithKeywords fastmeth = (_PyCFunctionFastWithKeywords)(void(*)(void))meth;

        if (_PyStack_UnpackDict(args, nargs, kwargs, &stack, &kwnames) < 0) {
            goto exit;
        }

        result = (*fastmeth) (self, stack, nargs, kwnames);
        if (kwnames != NULL) {
            Py_ssize_t i, n = nargs + PyTuple_GET_SIZE(kwnames);
            for (i = 0; i < n; i++) {
                Py_DECREF(stack[i]);
            }
            PyMem_Free((PyObject **)stack);
            Py_DECREF(kwnames);
        }
        break;
    }

    default:
        PyErr_SetString(PyExc_SystemError,
                        "Bad call flags in _PyMethodDef_RawFastCallDict. "
                        "METH_OLDARGS is no longer supported!");
        goto exit;
    }

    goto exit;

no_keyword_error:
    PyErr_Format(PyExc_TypeError,
                 "%.200s() takes no keyword arguments",
                 method->ml_name);

exit:
    Py_LeaveRecursiveCall();
    return result;
}


PyObject *
_PyCFunction_FastCallDict(PyObject *func,
                          PyObject *const *args, Py_ssize_t nargs,
                          PyObject *kwargs)
{
    PyObject *result;

    assert(func != NULL);
    assert(PyCFunction_Check(func));

    result = _PyMethodDef_RawFastCallDict(((PyCFunctionObject*)func)->m_ml,
                                          PyCFunction_GET_SELF(func),
                                          args, nargs, kwargs);
    result = _Py_CheckFunctionResult(func, result, NULL);
    return result;
}


PyObject *
_PyMethodDef_RawFastCallKeywords(PyMethodDef *method, PyObject *self,
                                 PyObject *const *args, Py_ssize_t nargs,
                                 PyObject *kwnames)
{
    /* _PyMethodDef_RawFastCallKeywords() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());

    assert(method != NULL);
    assert(nargs >= 0);
    assert(kwnames == NULL || PyTuple_CheckExact(kwnames));
    /* kwnames must only contains str strings, no subclass, and all keys must
       be unique */

    PyCFunction meth = method->ml_meth;
    int flags = method->ml_flags & ~(METH_CLASS | METH_STATIC | METH_COEXIST);
    Py_ssize_t nkwargs = kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames);
    PyObject *result = NULL;

    if (Py_EnterRecursiveCall(" while calling a Python object")) {
        return NULL;
    }

    switch (flags)
    {
    case METH_NOARGS:
        if (nkwargs) {
            goto no_keyword_error;
        }

        if (nargs != 0) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes no arguments (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        result = (*meth) (self, NULL);
        break;

    case METH_O:
        if (nkwargs) {
            goto no_keyword_error;
        }

        if (nargs != 1) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes exactly one argument (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        result = (*meth) (self, args[0]);
        break;

    case METH_FASTCALL:
        if (nkwargs) {
            goto no_keyword_error;
        }
        result = ((_PyCFunctionFast)(void(*)(void))meth) (self, args, nargs);
        break;

    case METH_FASTCALL | METH_KEYWORDS:
        /* Fast-path: avoid temporary dict to pass keyword arguments */
        result = ((_PyCFunctionFastWithKeywords)(void(*)(void))meth) (self, args, nargs, kwnames);
        break;

    case METH_VARARGS:
        if (nkwargs) {
            goto no_keyword_error;
        }
        /* fall through */

    case METH_VARARGS | METH_KEYWORDS:
    {
        /* Slow-path: create a temporary tuple for positional arguments
           and a temporary dict for keyword arguments */
        PyObject *argtuple;

        argtuple = _PyTuple_FromArray(args, nargs);
        if (argtuple == NULL) {
            goto exit;
        }

        if (flags & METH_KEYWORDS) {
            PyObject *kwdict;

            if (nkwargs > 0) {
                kwdict = _PyStack_AsDict(args + nargs, kwnames);
                if (kwdict == NULL) {
                    Py_DECREF(argtuple);
                    goto exit;
                }
            }
            else {
                kwdict = NULL;
            }

            result = (*(PyCFunctionWithKeywords)(void(*)(void))meth) (self, argtuple, kwdict);
            Py_XDECREF(kwdict);
        }
        else {
            result = (*meth) (self, argtuple);
        }
        Py_DECREF(argtuple);
        break;
    }

    default:
        PyErr_SetString(PyExc_SystemError,
                        "Bad call flags in _PyMethodDef_RawFastCallKeywords. "
                        "METH_OLDARGS is no longer supported!");
        goto exit;
    }

    goto exit;

no_keyword_error:
    PyErr_Format(PyExc_TypeError,
                 "%.200s() takes no keyword arguments",
                 method->ml_name);

exit:
    Py_LeaveRecursiveCall();
    return result;
}


PyObject *
_PyCFunction_Vectorcall(PyObject *func,
                        PyObject *const *args, size_t nargsf,
                        PyObject *kwnames)
{
    PyObject *result;

    assert(func != NULL);
    assert(PyCFunction_Check(func));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);

    result = _PyMethodDef_RawFastCallKeywords(((PyCFunctionObject*)func)->m_ml,
                                              PyCFunction_GET_SELF(func),
                                              args, nargs, kwnames);
    result = _Py_CheckFunctionResult(func, result, NULL);
    return result;
}


static PyObject *
cfunction_call_varargs(PyObject *func, PyObject *args, PyObject *kwargs)
{
    assert(!PyErr_Occurred());
    assert(kwargs == NULL || PyDict_Check(kwargs));

    PyCFunction meth = PyCFunction_GET_FUNCTION(func);
    PyObject *self = PyCFunction_GET_SELF(func);
    PyObject *result;

    assert(PyCFunction_GET_FLAGS(func) & METH_VARARGS);
    if (PyCFunction_GET_FLAGS(func) & METH_KEYWORDS) {
        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            return NULL;
        }

        result = (*(PyCFunctionWithKeywords)(void(*)(void))meth)(self, args, kwargs);

        Py_LeaveRecursiveCall();
    }
    else {
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            PyErr_Format(PyExc_TypeError, "%.200s() takes no keyword arguments",
                         ((PyCFunctionObject*)func)->m_ml->ml_name);
            return NULL;
        }

        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            return NULL;
        }

        result = (*meth)(self, args);

        Py_LeaveRecursiveCall();
    }

    return _Py_CheckFunctionResult(func, result, NULL);
}


PyObject *
PyCFunction_Call(PyObject *func, PyObject *args, PyObject *kwargs)
{
    /* For METH_VARARGS, we cannot use vectorcall as the vectorcall pointer
     * is NULL. This is intentional, since vectorcall would be slower. */
    if (PyCFunction_GET_FLAGS(func) & METH_VARARGS) {
        return cfunction_call_varargs(func, args, kwargs);
    }
    return PyVectorcall_Call(func, args, kwargs);
}


/* --- More complex call functions -------------------------------- */

/* External interface to call any callable object.
   The args must be a tuple or NULL.  The kwargs must be a dict or NULL. */
PyObject *
PyEval_CallObjectWithKeywords(PyObject *callable,
                              PyObject *args, PyObject *kwargs)
{
#ifdef Py_DEBUG
    /* PyEval_CallObjectWithKeywords() must not be called with an exception
       set. It raises a new exception if parameters are invalid or if
       PyTuple_New() fails, and so the original exception is lost. */
    assert(!PyErr_Occurred());
#endif

    if (args != NULL && !PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument list must be a tuple");
        return NULL;
    }

    if (kwargs != NULL && !PyDict_Check(kwargs)) {
        PyErr_SetString(PyExc_TypeError,
                        "keyword list must be a dictionary");
        return NULL;
    }

    if (args == NULL) {
        return _PyObject_FastCallDict(callable, NULL, 0, kwargs);
    }
    else {
        return PyObject_Call(callable, args, kwargs);
    }
}


PyObject *
PyObject_CallObject(PyObject *callable, PyObject *args)
{
    return PyEval_CallObjectWithKeywords(callable, args, NULL);
}


/* Positional arguments are obj followed by args:
   call callable(obj, *args, **kwargs) */
PyObject *
_PyObject_FastCall_Prepend(PyObject *callable, PyObject *obj,
                           PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **args2;
    PyObject *result;

    nargs++;
    if (nargs <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        args2 = small_stack;
    }
    else {
        args2 = PyMem_Malloc(nargs * sizeof(PyObject *));
        if (args2 == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    /* use borrowed references */
    args2[0] = obj;
    if (nargs > 1) {
        memcpy(&args2[1], args, (nargs - 1) * sizeof(PyObject *));
    }

    result = _PyObject_FastCall(callable, args2, nargs);
    if (args2 != small_stack) {
        PyMem_Free(args2);
    }
    return result;
}


/* Call callable(obj, *args, **kwargs). */
PyObject *
_PyObject_Call_Prepend(PyObject *callable,
                       PyObject *obj, PyObject *args, PyObject *kwargs)
{
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **stack;
    Py_ssize_t argcount;
    PyObject *result;

    assert(PyTuple_Check(args));

    argcount = PyTuple_GET_SIZE(args);
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

    result = _PyObject_FastCallDict(callable,
                                    stack, argcount + 1,
                                    kwargs);
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}


/* --- Call with a format string ---------------------------------- */

static PyObject *
_PyObject_CallFunctionVa(PyObject *callable, const char *format,
                         va_list va, int is_size_t)
{
    PyObject* small_stack[_PY_FASTCALL_SMALL_STACK];
    const Py_ssize_t small_stack_len = Py_ARRAY_LENGTH(small_stack);
    PyObject **stack;
    Py_ssize_t nargs, i;
    PyObject *result;

    if (callable == NULL) {
        return null_error();
    }

    if (!format || !*format) {
        return _PyObject_CallNoArg(callable);
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
        result = _PyObject_FastCall(callable,
                                    _PyTuple_ITEMS(args),
                                    PyTuple_GET_SIZE(args));
    }
    else {
        result = _PyObject_FastCall(callable, stack, nargs);
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

    va_start(va, format);
    result = _PyObject_CallFunctionVa(callable, format, va, 0);
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

    va_start(va, format);
    result = _PyObject_CallFunctionVa(callable, format, va, 0);
    va_end(va);

    return result;
}


PyObject *
_PyObject_CallFunction_SizeT(PyObject *callable, const char *format, ...)
{
    va_list va;
    PyObject *result;

    va_start(va, format);
    result = _PyObject_CallFunctionVa(callable, format, va, 1);
    va_end(va);

    return result;
}


static PyObject*
callmethod(PyObject* callable, const char *format, va_list va, int is_size_t)
{
    assert(callable != NULL);

    if (!PyCallable_Check(callable)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute of type '%.200s' is not callable",
                     Py_TYPE(callable)->tp_name);
        return NULL;
    }

    return _PyObject_CallFunctionVa(callable, format, va, is_size_t);
}


PyObject *
PyObject_CallMethod(PyObject *obj, const char *name, const char *format, ...)
{
    va_list va;
    PyObject *callable, *retval;

    if (obj == NULL || name == NULL) {
        return null_error();
    }

    callable = PyObject_GetAttrString(obj, name);
    if (callable == NULL)
        return NULL;

    va_start(va, format);
    retval = callmethod(callable, format, va, 0);
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
    va_list va;
    PyObject *callable, *retval;

    if (obj == NULL || name == NULL) {
        return null_error();
    }

    callable = PyObject_GetAttrString(obj, name);
    if (callable == NULL)
        return NULL;

    va_start(va, format);
    retval = callmethod(callable, format, va, 0);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject *
_PyObject_CallMethodId(PyObject *obj, _Py_Identifier *name,
                       const char *format, ...)
{
    va_list va;
    PyObject *callable, *retval;

    if (obj == NULL || name == NULL) {
        return null_error();
    }

    callable = _PyObject_GetAttrId(obj, name);
    if (callable == NULL)
        return NULL;

    va_start(va, format);
    retval = callmethod(callable, format, va, 0);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject *
_PyObject_CallMethod_SizeT(PyObject *obj, const char *name,
                           const char *format, ...)
{
    va_list va;
    PyObject *callable, *retval;

    if (obj == NULL || name == NULL) {
        return null_error();
    }

    callable = PyObject_GetAttrString(obj, name);
    if (callable == NULL)
        return NULL;

    va_start(va, format);
    retval = callmethod(callable, format, va, 1);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


PyObject *
_PyObject_CallMethodId_SizeT(PyObject *obj, _Py_Identifier *name,
                             const char *format, ...)
{
    va_list va;
    PyObject *callable, *retval;

    if (obj == NULL || name == NULL) {
        return null_error();
    }

    callable = _PyObject_GetAttrId(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_start(va, format);
    retval = callmethod(callable, format, va, 1);
    va_end(va);

    Py_DECREF(callable);
    return retval;
}


/* --- Call with "..." arguments ---------------------------------- */

static PyObject *
object_vacall(PyObject *base, PyObject *callable, va_list vargs)
{
    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject **stack;
    Py_ssize_t nargs;
    PyObject *result;
    Py_ssize_t i;
    va_list countva;

    if (callable == NULL) {
        return null_error();
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
    result = _PyObject_FastCall(callable, stack, nargs);

    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}


/* Private API for the LOAD_METHOD opcode. */
extern int _PyObject_GetMethod(PyObject *, PyObject *, PyObject **);

PyObject *
PyObject_CallMethodObjArgs(PyObject *obj, PyObject *name, ...)
{
    if (obj == NULL || name == NULL) {
        return null_error();
    }

    PyObject *callable = NULL;
    int is_method = _PyObject_GetMethod(obj, name, &callable);
    if (callable == NULL) {
        return NULL;
    }
    obj = is_method ? obj : NULL;

    va_list vargs;
    va_start(vargs, name);
    PyObject *result = object_vacall(obj, callable, vargs);
    va_end(vargs);

    Py_DECREF(callable);
    return result;
}


PyObject *
_PyObject_CallMethodIdObjArgs(PyObject *obj,
                              struct _Py_Identifier *name, ...)
{
    if (obj == NULL || name == NULL) {
        return null_error();
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
    PyObject *result = object_vacall(obj, callable, vargs);
    va_end(vargs);

    Py_DECREF(callable);
    return result;
}


PyObject *
PyObject_CallFunctionObjArgs(PyObject *callable, ...)
{
    va_list vargs;
    PyObject *result;

    va_start(vargs, callable);
    result = object_vacall(NULL, callable, vargs);
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


int
_PyStack_UnpackDict(PyObject *const *args, Py_ssize_t nargs, PyObject *kwargs,
                    PyObject *const **p_stack, PyObject **p_kwnames)
{
    PyObject **stack, **kwstack;
    Py_ssize_t nkwargs;
    Py_ssize_t pos, i;
    PyObject *key, *value;
    PyObject *kwnames;

    assert(nargs >= 0);
    assert(kwargs == NULL || PyDict_CheckExact(kwargs));

    if (kwargs == NULL || (nkwargs = PyDict_GET_SIZE(kwargs)) == 0) {
        *p_stack = args;
        *p_kwnames = NULL;
        return 0;
    }

    if ((size_t)nargs > PY_SSIZE_T_MAX / sizeof(stack[0]) - (size_t)nkwargs) {
        PyErr_NoMemory();
        return -1;
    }

    stack = PyMem_Malloc((nargs + nkwargs) * sizeof(stack[0]));
    if (stack == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    kwnames = PyTuple_New(nkwargs);
    if (kwnames == NULL) {
        PyMem_Free(stack);
        return -1;
    }

    /* Copy positional arguments */
    for (i = 0; i < nargs; i++) {
        Py_INCREF(args[i]);
        stack[i] = args[i];
    }

    kwstack = stack + nargs;
    pos = i = 0;
    /* This loop doesn't support lookup function mutating the dictionary
       to change its size. It's a deliberate choice for speed, this function is
       called in the performance critical hot code. */
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        Py_INCREF(key);
        Py_INCREF(value);
        PyTuple_SET_ITEM(kwnames, i, key);
        kwstack[i] = value;
        i++;
    }

    *p_stack = stack;
    *p_kwnames = kwnames;
    return 0;
}
