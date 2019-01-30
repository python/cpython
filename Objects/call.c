#include "Python.h"
#include "pycore_object.h"
#include "pycore_pystate.h"
#include "pycore_tupleobject.h"
#include "frameobject.h"


int
_PyObject_HasFastCall(PyObject *callable)
{
    if (PyFunction_Check(callable)) {
        return 1;
    }
    else if (PyCCall_Check(callable)) {
        /* A C call instance is said to support FASTCALL if no temporary
           tuple or dict is needed for calling the object. This is the
           case for CCALL_FASTCALL, CCALL_NOARGS and CCALL_O but not
           for CCALL_VARARGS. */
        uint32_t flags = PyCCall_FLAGS(callable);
        return (flags & CCALL_BASESIGNATURE) != CCALL_VARARGS;
    }
    else {
        assert (PyCallable_Check(callable));
        return 0;
    }
}


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
_PyObject_FastCallDict(PyObject *callable, PyObject *const *args, Py_ssize_t nargs,
                       PyObject *kwargs)
{
    /* _PyObject_FastCallDict() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());

    assert(callable != NULL);
    assert(nargs >= 0);
    assert(nargs == 0 || args != NULL);
    assert(kwargs == NULL || PyDict_Check(kwargs));

    if (PyFunction_Check(callable)) {
        return _PyFunction_FastCallDict(callable, args, nargs, kwargs);
    }
    else if (PyCCall_Check(callable)) {
        return PyCCall_FastCall(callable, args, nargs, kwargs);
    }
    else {
        PyObject *argstuple, *result;
        ternaryfunc call;

        /* Slow-path: build a temporary tuple */
        call = callable->ob_type->tp_call;
        if (call == NULL) {
            PyErr_Format(PyExc_TypeError, "'%.200s' object is not callable",
                         callable->ob_type->tp_name);
            return NULL;
        }

        argstuple = _PyTuple_FromArray(args, nargs);
        if (argstuple == NULL) {
            return NULL;
        }

        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            Py_DECREF(argstuple);
            return NULL;
        }

        result = (*call)(callable, argstuple, kwargs);

        Py_LeaveRecursiveCall();
        Py_DECREF(argstuple);

        result = _Py_CheckFunctionResult(callable, result, NULL);
        return result;
    }
}


PyObject *
_PyObject_FastCallKeywords(PyObject *callable, PyObject *const *stack, Py_ssize_t nargs,
                           PyObject *kwnames)
{
    /* _PyObject_FastCallKeywords() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!PyErr_Occurred());

    assert(nargs >= 0);
    assert(kwnames == NULL || PyTuple_CheckExact(kwnames));

    /* kwnames must only contains str strings, no subclass, and all keys must
       be unique: these checks are implemented in Python/ceval.c and
       _PyArg_ParseStackAndKeywords(). */

    if (PyFunction_Check(callable)) {
        return _PyFunction_FastCallKeywords(callable, stack, nargs, kwnames);
    }
    else if (PyCCall_Check(callable)) {
        return PyCCall_FastCall(callable, stack, nargs, kwnames);
    }
    else {
        /* Slow-path: build a temporary tuple for positional arguments and a
           temporary dictionary for keyword arguments (if any) */

        ternaryfunc call;
        PyObject *argstuple;
        PyObject *kwdict, *result;
        Py_ssize_t nkwargs;

        nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
        assert((nargs == 0 && nkwargs == 0) || stack != NULL);

        call = callable->ob_type->tp_call;
        if (call == NULL) {
            PyErr_Format(PyExc_TypeError, "'%.200s' object is not callable",
                         callable->ob_type->tp_name);
            return NULL;
        }

        argstuple = _PyTuple_FromArray(stack, nargs);
        if (argstuple == NULL) {
            return NULL;
        }

        if (nkwargs > 0) {
            kwdict = _PyStack_AsDict(stack + nargs, kwnames);
            if (kwdict == NULL) {
                Py_DECREF(argstuple);
                return NULL;
            }
        }
        else {
            kwdict = NULL;
        }

        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            Py_DECREF(argstuple);
            Py_XDECREF(kwdict);
            return NULL;
        }

        result = (*call)(callable, argstuple, kwdict);

        Py_LeaveRecursiveCall();

        Py_DECREF(argstuple);
        Py_XDECREF(kwdict);

        result = _Py_CheckFunctionResult(callable, result, NULL);
        return result;
    }
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

    if (PyFunction_Check(callable)) {
        return _PyFunction_FastCallDict(callable,
                                        _PyTuple_ITEMS(args),
                                        PyTuple_GET_SIZE(args),
                                        kwargs);
    }
    else if (PyCCall_Check(callable)) {
        return PyCCall_Call(callable, args, kwargs);
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
_PyFunction_FastCallKeywords(PyObject *func, PyObject *const *stack,
                             Py_ssize_t nargs, PyObject *kwnames)
{
    PyCodeObject *co = (PyCodeObject *)PyFunction_GET_CODE(func);
    PyObject *globals = PyFunction_GET_GLOBALS(func);
    PyObject *argdefs = PyFunction_GET_DEFAULTS(func);
    PyObject *kwdefs, *closure, *name, *qualname;
    PyObject **d;
    Py_ssize_t nkwargs = (kwnames == NULL) ? 0 : PyTuple_GET_SIZE(kwnames);
    Py_ssize_t nd;

    assert(PyFunction_Check(func));
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


/* --- C call protocol (PEP 580) ---------------------------------- */

/* Raise a suitable exception when a C call function is called with a
   wrong number of arguments. This is written with future extensions in
   mind (such as counting the "self" argument for methods), so it has
   more cases than strictly required. */
static void _Py_NO_INLINE
ccall_nargs_error(PyObject *func, Py_ssize_t nargs)
{
    const char *name = PyEval_GetFuncName(func);
    const char *desc = PyEval_GetFuncDesc(func);

    Py_ssize_t required = 0;  /* Minimum number of arguments required */
    int exact = 0;            /* Is the minimum actually the exact number of arguments? */

    uint32_t flags = PyCCall_FLAGS(func);
    switch (flags & CCALL_BASESIGNATURE)
    {
    case CCALL_NOARGS:
        required = 0;
        exact = 1;
        break;
    case CCALL_O:
        required = 1;
        exact = 1;
        break;
    }

    if (exact) {
        switch (required)
        {
        case 0:
            PyErr_Format(PyExc_TypeError,
                "%s%s takes no arguments (%zd given)",
                name, desc, nargs);
            return;
        case 1:
            PyErr_Format(PyExc_TypeError,
                "%s%s takes exactly one argument (%zd given)",
                name, desc, nargs);
            return;
        default:
            /* This case currently does not happen */
            PyErr_Format(PyExc_TypeError,
                "%s%s takes exactly %zd arguments (%zd given)",
                name, desc, required, nargs);
            return;
        }
    }

    if (nargs == 0) {
        PyErr_Format(PyExc_TypeError,
                     "%s%s needs an argument",
                     name, desc);
        return;
    }
    else {
        /* This case currently does not happen */
        PyErr_Format(PyExc_TypeError,
                     "%s%s needs at least %zd arguments (%zd given)",
                     name, desc, required, nargs);
    }
}


static void _Py_NO_INLINE
ccall_kwargs_error(PyObject *func)
{
    PyErr_Format(PyExc_TypeError,
                 "%s%s takes no keyword arguments",
                 PyEval_GetFuncName(func),
                 PyEval_GetFuncDesc(func));
}


/* Internal function for calling a CCALL_VARARGS function, just passing
   the given arguments. The caller is responsible for doing all
   transformations and checks.

   args must be a tuple and kwargs either NULL or a dict */
static PyObject *
ccall_varargs_dispatch(PyObject *func, PyObject *self, PyObject *args, PyObject *kwargs)
{
    const PyCCallDef *cc = PyCCall_CCALLDEF(func);
    PyCFunc cfunc = cc->cc_func;

    switch (cc->cc_flags & CCALL_SIGNATURE) {
    case CCALL_VARARGS:
        return ((PyCFunc_SA)cfunc)(self, args);
    case CCALL_VARARGS | CCALL_DEFARG:
        return ((PyCFunc_DSA)cfunc)(cc, self, args);
    case CCALL_VARARGS | CCALL_KEYWORDS:
        return ((PyCFunc_SAK)cfunc)(self, args, kwargs);
    case CCALL_VARARGS | CCALL_KEYWORDS | CCALL_DEFARG:
        return ((PyCFunc_DSAK)cfunc)(cc, self, args, kwargs);
    default:
        PyErr_Format(PyExc_SystemError,
                     "%.100s object has invalid C call flags",
                     Py_TYPE(func)->tp_name);
        return NULL;
    }
}


/* PyCCall_FastCall: the possibilities for keywords are:

   - NULL: no keyword arguments
   - dict with {name:value} items
   - tuple with keyword names. Names must be unique. The keyword values
     appear in the *args array starting with args[nargs].

   Keyword names must only contains str strings (no subclasses) */
PyObject *
PyCCall_FastCall(PyObject *func,
                 PyObject *const *args, Py_ssize_t nargs,
                 PyObject *keywords)
{
    assert(!PyErr_Occurred());
    assert(func != NULL);
    assert(nargs >= 0);
    assert(args != NULL || nargs == 0);

    const PyCCallRoot *cr = PyCCall_CCALLROOT(func);
    const PyCCallDef *cc = cr->cr_ccall;
    PyObject *self = cr->cr_self;
    uint32_t flags = cc->cc_flags;
    PyCFunc cfunc = cc->cc_func;

    if (self == NULL && (flags & (CCALL_OBJCLASS | CCALL_SELFARG)))
    {
        /* Check __objclass__ and do self slicing */
        if (nargs == 0) {
            ccall_nargs_error(func, 0);
            return NULL;
        }

        PyObject *arg0 = args[0];
        if (flags & CCALL_OBJCLASS) {
            PyTypeObject *objclass = (PyTypeObject *)cc->cc_parent;
            if (!PyObject_TypeCheck(arg0, objclass))
            {
                PyErr_Format(PyExc_TypeError,
                             "%s%s "
                             "requires a '%.100s' object "
                             "but received a '%.100s'",
                             PyEval_GetFuncName(func),
                             PyEval_GetFuncDesc(func),
                             objclass->tp_name,
                             Py_TYPE(arg0)->tp_name);
                return NULL;
            }
        }

        if (flags & CCALL_SELFARG) {
            self = arg0;
            args++;
            nargs--;
        }
    }

    if (Py_EnterRecursiveCall(" while calling a function")) {
        return NULL;
    }

    /* Set keywords = NULL if no keywords are actually passed */
    if (keywords) {
        if (PyTuple_CheckExact(keywords)) {
            if (PyTuple_GET_SIZE(keywords) == 0) {
                keywords = NULL;
            }
        }
        else {
            assert(PyDict_Check(keywords));
            if (PyDict_GET_SIZE(keywords) == 0) {
                keywords = NULL;
            }
        }
    }

    PyObject *result = NULL;
    if (keywords == NULL) {
        /* Case 1: no keyword arguments are passed (or by the above
           check: an empty tuple/dict was passed as keywords).
           This is the case which is most important to optimize.
           Note that the C function may or may not accept keywords. */
        switch (flags & CCALL_SIGNATURE)
        {
        case CCALL_FASTCALL:
            result = ((PyCFunc_SV)cfunc)(self, args, nargs);
            break;
        case CCALL_FASTCALL | CCALL_DEFARG:
            result = ((PyCFunc_DSV)cfunc)(cc, self, args, nargs);
            break;
        case CCALL_FASTCALL | CCALL_KEYWORDS:
            result = ((PyCFunc_SVK)cfunc)(self, args, nargs, NULL);
            break;
        case CCALL_FASTCALL | CCALL_KEYWORDS | CCALL_DEFARG:
            result = ((PyCFunc_DSVK)cfunc)(cc, self, args, nargs, NULL);
            break;
        case CCALL_NOARGS:
            if (nargs != 0) {
                ccall_nargs_error(func, nargs);
                break;
            }
            result = ((PyCFunc_SA)cfunc)(self, NULL);
            break;
        case CCALL_NOARGS | CCALL_DEFARG:
            if (nargs != 0) {
                ccall_nargs_error(func, nargs);
                break;
            }
            result = ((PyCFunc_DS)cfunc)(cc, self);
            break;
        case CCALL_O:
            if (nargs != 1) {
                ccall_nargs_error(func, nargs);
                break;
            }
            result = ((PyCFunc_SA)cfunc)(self, args[0]);
            break;
        case CCALL_O | CCALL_DEFARG:
            if (nargs != 1) {
                ccall_nargs_error(func, nargs);
                break;
            }
            result = ((PyCFunc_DSA)cfunc)(cc, self, args[0]);
            break;
        default:
        {
            /* Something involving CCALL_VARARGS */
            /* Create a temporary tuple for positional arguments */
            PyObject *argstuple = _PyTuple_FromArray(args, nargs);
            if (argstuple != NULL) {
                result = ccall_varargs_dispatch(func, self, argstuple, NULL);
                Py_DECREF(argstuple);
            }
        }
        }
    }
    else {
        /* Case 2: keyword arguments are passed. */
        switch (flags & (CCALL_SIGNATURE & ~CCALL_DEFARG))
        {
        case CCALL_FASTCALL | CCALL_KEYWORDS:
        {
            PyObject *const *stack = args;
            PyObject *kwnames = keywords;

            if (!PyTuple_CheckExact(keywords)) {
                /* Convert keywords from dict to tuple */
                if (_PyStack_UnpackDict(args, nargs, keywords, &stack, &kwnames) < 0) {
                    break;
                }
            }

            if (flags & CCALL_DEFARG) {
                result = ((PyCFunc_DSVK)cfunc)(cc, self, stack, nargs, kwnames);
            }
            else {
                result = ((PyCFunc_SVK)cfunc)(self, stack, nargs, kwnames);
            }
            if (stack != args) {
                PyMem_Free((void *)stack);
            }
            if (kwnames != keywords) {
                Py_DECREF(kwnames);
            }
            break;
        }
        case CCALL_VARARGS | CCALL_KEYWORDS:
        {
            PyObject *kwargs = keywords;
            if (PyTuple_CheckExact(keywords)) {
                /* Convert keywords from tuple to dict */
                kwargs = _PyStack_AsDict(args + nargs, keywords);
                if (kwargs == NULL) {
                    break;
                }
            }

            /* Create a temporary tuple for positional arguments */
            PyObject *argstuple = _PyTuple_FromArray(args, nargs);
            if (argstuple != NULL) {
                result = ccall_varargs_dispatch(func, self, argstuple, kwargs);
                Py_DECREF(argstuple);
            }

            if (kwargs != keywords) {
                Py_DECREF(kwargs);
            }
            break;
        }
        default:
            ccall_kwargs_error(func);
        }
    }

    Py_LeaveRecursiveCall();
    return _Py_CheckFunctionResult(func, result, NULL);
}


PyObject *
PyCCall_Call(PyObject *func, PyObject *args, PyObject *kwargs)
{
    if (!PyCCall_Check(func)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    const PyCCallRoot *cr = PyCCall_CCALLROOT(func);
    uint32_t flags = cr->cr_ccall->cc_flags;

    /* Check whether we can call a CCALL_VARARGS function without
       special processing for "self". In that case, we can just pass
       the args tuple through to the C function */
    if ((flags & CCALL_BASESIGNATURE) == CCALL_VARARGS) {
        PyObject *self = cr->cr_self;
        if ((self != NULL) ||
            (flags & (CCALL_OBJCLASS | CCALL_SELFARG)) == 0)
        {
            if (kwargs) {
                assert(PyDict_Check(kwargs));
                if (PyDict_GET_SIZE(kwargs) == 0) {
                    kwargs = NULL;
                } else if (!(flags & CCALL_KEYWORDS)) {
                    ccall_kwargs_error(func);
                    return NULL;
                }
            }

            if (Py_EnterRecursiveCall(" while calling a function")) {
                return NULL;
            }

            PyObject *result = ccall_varargs_dispatch(func, self, args, kwargs);

            Py_LeaveRecursiveCall();
            return _Py_CheckFunctionResult(func, result, NULL);
        }
    }

    /* General case */
    return PyCCall_FastCall(func,
                            _PyTuple_ITEMS(args),
                            PyTuple_GET_SIZE(args),
                            kwargs);
}


/* --- C call support not directly related to calling ------------- */

PyObject *
PyCCall_GenericGetQualname(PyObject *func, void *closure)
{
    /*
        name = func.__name__
        try:
            parent_qualname = func.__parent__.__qualname__
        except AttributeError:
            return name
        return str(parent_qualname) + "." + name
    */
    _Py_IDENTIFIER(__name__);
    _Py_IDENTIFIER(__qualname__);

    const PyCCallDef *cc = PyCCall_CCALLDEF(func);

    PyObject *name = _PyObject_GetAttrId(func, &PyId___name__);
    if (name == NULL) {
        return NULL;
    }
    PyObject *parent = cc->cc_parent;
    if (parent == NULL) {
        return name;
    }
    PyObject *parent_qualname = _PyObject_GetAttrId(parent, &PyId___qualname__);
    if (parent_qualname == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            return name;
        }
        return NULL;
    }

    PyObject *res = PyUnicode_FromFormat("%S.%U", parent_qualname, name);
    Py_DECREF(parent_qualname);
    return res;
}


PyObject *
PyCCall_GenericGetParent(PyObject *func, void *closure)
{
    if (!PyCCall_Check(func)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyObject *attr = PyCCall_CCALLDEF(func)->cc_parent;

    if (attr == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "'%.100s' object has no attribute '%s'",
                     Py_TYPE(func)->tp_name,
                     closure == NULL ? "__parent__" : (char*)closure);
        return NULL;
    }
    Py_INCREF(attr);
    return attr;
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
object_vacall(PyObject *callable, va_list vargs)
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
    nargs = 0;
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

    for (i = 0; i < nargs; ++i) {
        stack[i] = va_arg(vargs, PyObject *);
    }

    /* Call the function */
    result = _PyObject_FastCall(callable, stack, nargs);

    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}


PyObject *
PyObject_CallMethodObjArgs(PyObject *callable, PyObject *name, ...)
{
    va_list vargs;
    PyObject *result;

    if (callable == NULL || name == NULL) {
        return null_error();
    }

    callable = PyObject_GetAttr(callable, name);
    if (callable == NULL) {
        return NULL;
    }

    va_start(vargs, name);
    result = object_vacall(callable, vargs);
    va_end(vargs);

    Py_DECREF(callable);
    return result;
}


PyObject *
_PyObject_CallMethodIdObjArgs(PyObject *obj,
                              struct _Py_Identifier *name, ...)
{
    va_list vargs;
    PyObject *callable, *result;

    if (obj == NULL || name == NULL) {
        return null_error();
    }

    callable = _PyObject_GetAttrId(obj, name);
    if (callable == NULL) {
        return NULL;
    }

    va_start(vargs, name);
    result = object_vacall(callable, vargs);
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
    result = object_vacall(callable, vargs);
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

    /* Copy position arguments (borrowed references) */
    memcpy(stack, args, nargs * sizeof(stack[0]));

    kwstack = stack + nargs;
    pos = i = 0;
    /* This loop doesn't support lookup function mutating the dictionary
       to change its size. It's a deliberate choice for speed, this function is
       called in the performance critical hot code. */
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        Py_INCREF(key);
        PyTuple_SET_ITEM(kwnames, i, key);
        /* The stack contains borrowed references */
        kwstack[i] = value;
        i++;
    }

    *p_stack = stack;
    *p_kwnames = kwnames;
    return 0;
}
