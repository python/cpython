
#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_frame.h"
#include "pycore_function.h"
#include "pycore_runtime.h"
#include "pycore_global_objects.h"
#include "pycore_intrinsics.h"
#include "pycore_pyerrors.h"
#include "pycore_typevarobject.h"


/******** Unary functions ********/

static PyObject *
no_intrinsic(PyThreadState* tstate, PyObject *unused)
{
    _PyErr_SetString(tstate, PyExc_SystemError, "invalid intrinsic function");
    return NULL;
}

static PyObject *
print_expr(PyThreadState* tstate, PyObject *value)
{
    PyObject *hook = _PySys_GetAttr(tstate, &_Py_ID(displayhook));
    // Can't use ERROR_IF here.
    if (hook == NULL) {
        _PyErr_SetString(tstate, PyExc_RuntimeError,
                            "lost sys.displayhook");
        return NULL;
    }
    return PyObject_CallOneArg(hook, value);
}

static int
import_all_from(PyThreadState *tstate, PyObject *locals, PyObject *v)
{
    PyObject *all, *dict, *name, *value;
    int skip_leading_underscores = 0;
    int pos, err;

    if (_PyObject_LookupAttr(v, &_Py_ID(__all__), &all) < 0) {
        return -1; /* Unexpected error */
    }
    if (all == NULL) {
        if (_PyObject_LookupAttr(v, &_Py_ID(__dict__), &dict) < 0) {
            return -1;
        }
        if (dict == NULL) {
            _PyErr_SetString(tstate, PyExc_ImportError,
                    "from-import-* object has no __dict__ and no __all__");
            return -1;
        }
        all = PyMapping_Keys(dict);
        Py_DECREF(dict);
        if (all == NULL)
            return -1;
        skip_leading_underscores = 1;
    }

    for (pos = 0, err = 0; ; pos++) {
        name = PySequence_GetItem(all, pos);
        if (name == NULL) {
            if (!_PyErr_ExceptionMatches(tstate, PyExc_IndexError)) {
                err = -1;
            }
            else {
                _PyErr_Clear(tstate);
            }
            break;
        }
        if (!PyUnicode_Check(name)) {
            PyObject *modname = PyObject_GetAttr(v, &_Py_ID(__name__));
            if (modname == NULL) {
                Py_DECREF(name);
                err = -1;
                break;
            }
            if (!PyUnicode_Check(modname)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "module __name__ must be a string, not %.100s",
                              Py_TYPE(modname)->tp_name);
            }
            else {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%s in %U.%s must be str, not %.100s",
                              skip_leading_underscores ? "Key" : "Item",
                              modname,
                              skip_leading_underscores ? "__dict__" : "__all__",
                              Py_TYPE(name)->tp_name);
            }
            Py_DECREF(modname);
            Py_DECREF(name);
            err = -1;
            break;
        }
        if (skip_leading_underscores) {
            if (PyUnicode_READY(name) == -1) {
                Py_DECREF(name);
                err = -1;
                break;
            }
            if (PyUnicode_READ_CHAR(name, 0) == '_') {
                Py_DECREF(name);
                continue;
            }
        }
        value = PyObject_GetAttr(v, name);
        if (value == NULL)
            err = -1;
        else if (PyDict_CheckExact(locals))
            err = PyDict_SetItem(locals, name, value);
        else
            err = PyObject_SetItem(locals, name, value);
        Py_DECREF(name);
        Py_XDECREF(value);
        if (err < 0)
            break;
    }
    Py_DECREF(all);
    return err;
}

static PyObject *
import_star(PyThreadState* tstate, PyObject *from)
{
    _PyInterpreterFrame *frame = tstate->cframe->current_frame;
    if (_PyFrame_FastToLocalsWithError(frame) < 0) {
        return NULL;
    }

    PyObject *locals = frame->f_locals;
    if (locals == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                            "no locals found during 'import *'");
        return NULL;
    }
    int err = import_all_from(tstate, locals, from);
    _PyFrame_LocalsToFast(frame, 0);
    if (err < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
stopiteration_error(PyThreadState* tstate, PyObject *exc)
{
    _PyInterpreterFrame *frame = tstate->cframe->current_frame;
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    assert(PyExceptionInstance_Check(exc));
    const char *msg = NULL;
    if (PyErr_GivenExceptionMatches(exc, PyExc_StopIteration)) {
        msg = "generator raised StopIteration";
        if (frame->f_code->co_flags & CO_ASYNC_GENERATOR) {
            msg = "async generator raised StopIteration";
        }
        else if (frame->f_code->co_flags & CO_COROUTINE) {
            msg = "coroutine raised StopIteration";
        }
    }
    else if ((frame->f_code->co_flags & CO_ASYNC_GENERATOR) &&
            PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration))
    {
        /* code in `gen` raised a StopAsyncIteration error:
        raise a RuntimeError.
        */
        msg = "async generator raised StopAsyncIteration";
    }
    if (msg != NULL) {
        PyObject *message = _PyUnicode_FromASCII(msg, strlen(msg));
        if (message == NULL) {
            return NULL;
        }
        PyObject *error = PyObject_CallOneArg(PyExc_RuntimeError, message);
        if (error == NULL) {
            Py_DECREF(message);
            return NULL;
        }
        assert(PyExceptionInstance_Check(error));
        PyException_SetCause(error, Py_NewRef(exc));
        // Steal exc reference, rather than Py_NewRef+Py_DECREF
        PyException_SetContext(error, Py_NewRef(exc));
        Py_DECREF(message);
        return error;
    }
    return Py_NewRef(exc);
}

static PyObject *
unary_pos(PyThreadState* unused, PyObject *value)
{
    return PyNumber_Positive(value);
}

static PyObject *
list_to_tuple(PyThreadState* unused, PyObject *v)
{
    assert(PyList_Check(v));
    return _PyTuple_FromArray(((PyListObject *)v)->ob_item, Py_SIZE(v));
}

static PyObject *
make_typevar(PyThreadState* Py_UNUSED(ignored), PyObject *v)
{
    assert(PyUnicode_Check(v));
    return _Py_make_typevar(v, NULL, NULL);
}

const instrinsic_func1
_PyIntrinsics_UnaryFunctions[] = {
    [0] = no_intrinsic,
    [INTRINSIC_PRINT] = print_expr,
    [INTRINSIC_IMPORT_STAR] = import_star,
    [INTRINSIC_STOPITERATION_ERROR] = stopiteration_error,
    [INTRINSIC_ASYNC_GEN_WRAP] = _PyAsyncGenValueWrapperNew,
    [INTRINSIC_UNARY_POSITIVE] = unary_pos,
    [INTRINSIC_LIST_TO_TUPLE] = list_to_tuple,
    [INTRINSIC_TYPEVAR] = make_typevar,
    [INTRINSIC_PARAMSPEC] = _Py_make_paramspec,
    [INTRINSIC_TYPEVARTUPLE] = _Py_make_typevartuple,
    [INTRINSIC_SUBSCRIPT_GENERIC] = _Py_subscript_generic,
    [INTRINSIC_TYPEALIAS] = _Py_make_typealias,
};


/******** Binary functions ********/


static PyObject *
prep_reraise_star(PyThreadState* unused, PyObject *orig, PyObject *excs)
{
    assert(PyList_Check(excs));
    return _PyExc_PrepReraiseStar(orig, excs);
}

static PyObject *
make_typevar_with_bound(PyThreadState* Py_UNUSED(ignored), PyObject *name,
                        PyObject *evaluate_bound)
{
    assert(PyUnicode_Check(name));
    return _Py_make_typevar(name, evaluate_bound, NULL);
}

static PyObject *
make_typevar_with_constraints(PyThreadState* Py_UNUSED(ignored), PyObject *name,
                              PyObject *evaluate_constraints)
{
    assert(PyUnicode_Check(name));
    return _Py_make_typevar(name, NULL, evaluate_constraints);
}

const instrinsic_func2
_PyIntrinsics_BinaryFunctions[] = {
    [INTRINSIC_PREP_RERAISE_STAR] = prep_reraise_star,
    [INTRINSIC_TYPEVAR_WITH_BOUND] = make_typevar_with_bound,
    [INTRINSIC_TYPEVAR_WITH_CONSTRAINTS] = make_typevar_with_constraints,
    [INTRINSIC_SET_FUNCTION_TYPE_PARAMS] = _Py_set_function_type_params,
};
