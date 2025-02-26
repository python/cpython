/*
 * New exceptions.c written in Iceland by Richard Jones and Georg Brandl.
 *
 * Thanks go to Tim Peters and Michael Hudson for debugging.
 */

#include <Python.h>
#include <stdbool.h>
#include "pycore_abstract.h"      // _PyObject_RealIsSubclass()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCall
#include "pycore_exceptions.h"    // struct _Py_exc_state
#include "pycore_initconfig.h"
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_object.h"
#include "pycore_pyerrors.h"      // struct _PyErr_SetRaisedException

#include "osdefs.h"               // SEP


/* Compatibility aliases */
PyObject *PyExc_EnvironmentError = NULL;  // borrowed ref
PyObject *PyExc_IOError = NULL;  // borrowed ref
#ifdef MS_WINDOWS
PyObject *PyExc_WindowsError = NULL;  // borrowed ref
#endif


static struct _Py_exc_state*
get_exc_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->exc_state;
}


/* NOTE: If the exception class hierarchy changes, don't forget to update
 * Lib/test/exception_hierarchy.txt
 */

/*
 *    BaseException
 */
static PyObject *
BaseException_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyBaseExceptionObject *self;

    self = (PyBaseExceptionObject *)type->tp_alloc(type, 0);
    if (!self)
        return NULL;
    /* the dict is created on the fly in PyObject_GenericSetAttr */
    self->dict = NULL;
    self->notes = NULL;
    self->traceback = self->cause = self->context = NULL;
    self->suppress_context = 0;

    if (args) {
        self->args = Py_NewRef(args);
        return (PyObject *)self;
    }

    self->args = PyTuple_New(0);
    if (!self->args) {
        Py_DECREF(self);
        return NULL;
    }

    return (PyObject *)self;
}

static int
BaseException_init(PyBaseExceptionObject *self, PyObject *args, PyObject *kwds)
{
    if (!_PyArg_NoKeywords(Py_TYPE(self)->tp_name, kwds))
        return -1;

    Py_XSETREF(self->args, Py_NewRef(args));
    return 0;
}

static int
BaseException_clear(PyBaseExceptionObject *self)
{
    Py_CLEAR(self->dict);
    Py_CLEAR(self->args);
    Py_CLEAR(self->notes);
    Py_CLEAR(self->traceback);
    Py_CLEAR(self->cause);
    Py_CLEAR(self->context);
    return 0;
}

static void
BaseException_dealloc(PyBaseExceptionObject *self)
{
    PyObject_GC_UnTrack(self);
    // bpo-44348: The trashcan mechanism prevents stack overflow when deleting
    // long chains of exceptions. For example, exceptions can be chained
    // through the __context__ attributes or the __traceback__ attribute.
    Py_TRASHCAN_BEGIN(self, BaseException_dealloc)
    BaseException_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
    Py_TRASHCAN_END
}

static int
BaseException_traverse(PyBaseExceptionObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    Py_VISIT(self->args);
    Py_VISIT(self->notes);
    Py_VISIT(self->traceback);
    Py_VISIT(self->cause);
    Py_VISIT(self->context);
    return 0;
}

static PyObject *
BaseException_str(PyBaseExceptionObject *self)
{
    switch (PyTuple_GET_SIZE(self->args)) {
    case 0:
        return PyUnicode_FromString("");
    case 1:
        return PyObject_Str(PyTuple_GET_ITEM(self->args, 0));
    default:
        return PyObject_Str(self->args);
    }
}

static PyObject *
BaseException_repr(PyBaseExceptionObject *self)
{
    const char *name = _PyType_Name(Py_TYPE(self));
    if (PyTuple_GET_SIZE(self->args) == 1)
        return PyUnicode_FromFormat("%s(%R)", name,
                                    PyTuple_GET_ITEM(self->args, 0));
    else
        return PyUnicode_FromFormat("%s%R", name, self->args);
}

/* Pickling support */
static PyObject *
BaseException_reduce(PyBaseExceptionObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->args && self->dict)
        return PyTuple_Pack(3, Py_TYPE(self), self->args, self->dict);
    else
        return PyTuple_Pack(2, Py_TYPE(self), self->args);
}

/*
 * Needed for backward compatibility, since exceptions used to store
 * all their attributes in the __dict__. Code is taken from cPickle's
 * load_build function.
 */
static PyObject *
BaseException_setstate(PyObject *self, PyObject *state)
{
    PyObject *d_key, *d_value;
    Py_ssize_t i = 0;

    if (state != Py_None) {
        if (!PyDict_Check(state)) {
            PyErr_SetString(PyExc_TypeError, "state is not a dictionary");
            return NULL;
        }
        while (PyDict_Next(state, &i, &d_key, &d_value)) {
            Py_INCREF(d_key);
            Py_INCREF(d_value);
            int res = PyObject_SetAttr(self, d_key, d_value);
            Py_DECREF(d_value);
            Py_DECREF(d_key);
            if (res < 0) {
                return NULL;
            }
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
BaseException_with_traceback(PyObject *self, PyObject *tb) {
    if (PyException_SetTraceback(self, tb))
        return NULL;

    return Py_NewRef(self);
}

PyDoc_STRVAR(with_traceback_doc,
"Exception.with_traceback(tb) --\n\
    set self.__traceback__ to tb and return self.");

static inline PyBaseExceptionObject*
_PyBaseExceptionObject_cast(PyObject *exc)
{
    assert(PyExceptionInstance_Check(exc));
    return (PyBaseExceptionObject *)exc;
}

static PyObject *
BaseException_add_note(PyObject *self, PyObject *note)
{
    if (!PyUnicode_Check(note)) {
        PyErr_Format(PyExc_TypeError,
                     "note must be a str, not '%s'",
                     Py_TYPE(note)->tp_name);
        return NULL;
    }

    PyObject *notes;
    if (PyObject_GetOptionalAttr(self, &_Py_ID(__notes__), &notes) < 0) {
        return NULL;
    }
    if (notes == NULL) {
        notes = PyList_New(0);
        if (notes == NULL) {
            return NULL;
        }
        if (PyObject_SetAttr(self, &_Py_ID(__notes__), notes) < 0) {
            Py_DECREF(notes);
            return NULL;
        }
    }
    else if (!PyList_Check(notes)) {
        Py_DECREF(notes);
        PyErr_SetString(PyExc_TypeError, "Cannot add note: __notes__ is not a list");
        return NULL;
    }
    if (PyList_Append(notes, note) < 0) {
        Py_DECREF(notes);
        return NULL;
    }
    Py_DECREF(notes);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(add_note_doc,
"Exception.add_note(note) --\n\
    add a note to the exception");

static PyMethodDef BaseException_methods[] = {
   {"__reduce__", (PyCFunction)BaseException_reduce, METH_NOARGS },
   {"__setstate__", (PyCFunction)BaseException_setstate, METH_O },
   {"with_traceback", (PyCFunction)BaseException_with_traceback, METH_O,
    with_traceback_doc},
   {"add_note", (PyCFunction)BaseException_add_note, METH_O,
    add_note_doc},
   {NULL, NULL, 0, NULL},
};

static PyObject *
BaseException_get_args(PyBaseExceptionObject *self, void *Py_UNUSED(ignored))
{
    if (self->args == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(self->args);
}

static int
BaseException_set_args(PyBaseExceptionObject *self, PyObject *val, void *Py_UNUSED(ignored))
{
    PyObject *seq;
    if (val == NULL) {
        PyErr_SetString(PyExc_TypeError, "args may not be deleted");
        return -1;
    }
    seq = PySequence_Tuple(val);
    if (!seq)
        return -1;
    Py_XSETREF(self->args, seq);
    return 0;
}

static PyObject *
BaseException_get_tb(PyBaseExceptionObject *self, void *Py_UNUSED(ignored))
{
    if (self->traceback == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(self->traceback);
}

static int
BaseException_set_tb(PyBaseExceptionObject *self, PyObject *tb, void *Py_UNUSED(ignored))
{
    if (tb == NULL) {
        PyErr_SetString(PyExc_TypeError, "__traceback__ may not be deleted");
        return -1;
    }
    if (PyTraceBack_Check(tb)) {
        Py_XSETREF(self->traceback, Py_NewRef(tb));
    }
    else if (tb == Py_None) {
        Py_CLEAR(self->traceback);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "__traceback__ must be a traceback or None");
        return -1;
    }
    return 0;
}

static PyObject *
BaseException_get_context(PyObject *self, void *Py_UNUSED(ignored))
{
    PyObject *res = PyException_GetContext(self);
    if (res)
        return res;  /* new reference already returned above */
    Py_RETURN_NONE;
}

static int
BaseException_set_context(PyObject *self, PyObject *arg, void *Py_UNUSED(ignored))
{
    if (arg == NULL) {
        PyErr_SetString(PyExc_TypeError, "__context__ may not be deleted");
        return -1;
    } else if (arg == Py_None) {
        arg = NULL;
    } else if (!PyExceptionInstance_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "exception context must be None "
                        "or derive from BaseException");
        return -1;
    } else {
        /* PyException_SetContext steals this reference */
        Py_INCREF(arg);
    }
    PyException_SetContext(self, arg);
    return 0;
}

static PyObject *
BaseException_get_cause(PyObject *self, void *Py_UNUSED(ignored))
{
    PyObject *res = PyException_GetCause(self);
    if (res)
        return res;  /* new reference already returned above */
    Py_RETURN_NONE;
}

static int
BaseException_set_cause(PyObject *self, PyObject *arg, void *Py_UNUSED(ignored))
{
    if (arg == NULL) {
        PyErr_SetString(PyExc_TypeError, "__cause__ may not be deleted");
        return -1;
    } else if (arg == Py_None) {
        arg = NULL;
    } else if (!PyExceptionInstance_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "exception cause must be None "
                        "or derive from BaseException");
        return -1;
    } else {
        /* PyException_SetCause steals this reference */
        Py_INCREF(arg);
    }
    PyException_SetCause(self, arg);
    return 0;
}


static PyGetSetDef BaseException_getset[] = {
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {"args", (getter)BaseException_get_args, (setter)BaseException_set_args},
    {"__traceback__", (getter)BaseException_get_tb, (setter)BaseException_set_tb},
    {"__context__", BaseException_get_context,
     BaseException_set_context, PyDoc_STR("exception context")},
    {"__cause__", BaseException_get_cause,
     BaseException_set_cause, PyDoc_STR("exception cause")},
    {NULL},
};


PyObject *
PyException_GetTraceback(PyObject *self)
{
    PyBaseExceptionObject *base_self = _PyBaseExceptionObject_cast(self);
    return Py_XNewRef(base_self->traceback);
}


int
PyException_SetTraceback(PyObject *self, PyObject *tb)
{
    return BaseException_set_tb(_PyBaseExceptionObject_cast(self), tb, NULL);
}

PyObject *
PyException_GetCause(PyObject *self)
{
    PyObject *cause = _PyBaseExceptionObject_cast(self)->cause;
    return Py_XNewRef(cause);
}

/* Steals a reference to cause */
void
PyException_SetCause(PyObject *self, PyObject *cause)
{
    PyBaseExceptionObject *base_self = _PyBaseExceptionObject_cast(self);
    base_self->suppress_context = 1;
    Py_XSETREF(base_self->cause, cause);
}

PyObject *
PyException_GetContext(PyObject *self)
{
    PyObject *context = _PyBaseExceptionObject_cast(self)->context;
    return Py_XNewRef(context);
}

/* Steals a reference to context */
void
PyException_SetContext(PyObject *self, PyObject *context)
{
    Py_XSETREF(_PyBaseExceptionObject_cast(self)->context, context);
}

PyObject *
PyException_GetArgs(PyObject *self)
{
    PyObject *args = _PyBaseExceptionObject_cast(self)->args;
    return Py_NewRef(args);
}

void
PyException_SetArgs(PyObject *self, PyObject *args)
{
    Py_INCREF(args);
    Py_XSETREF(_PyBaseExceptionObject_cast(self)->args, args);
}

const char *
PyExceptionClass_Name(PyObject *ob)
{
    assert(PyExceptionClass_Check(ob));
    return ((PyTypeObject*)ob)->tp_name;
}

static struct PyMemberDef BaseException_members[] = {
    {"__suppress_context__", Py_T_BOOL,
     offsetof(PyBaseExceptionObject, suppress_context)},
    {NULL}
};


static PyTypeObject _PyExc_BaseException = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "BaseException", /*tp_name*/
    sizeof(PyBaseExceptionObject), /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)BaseException_dealloc, /*tp_dealloc*/
    0,                          /*tp_vectorcall_offset*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_as_async*/
    (reprfunc)BaseException_repr, /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    (reprfunc)BaseException_str,  /*tp_str*/
    PyObject_GenericGetAttr,    /*tp_getattro*/
    PyObject_GenericSetAttr,    /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASE_EXC_SUBCLASS,  /*tp_flags*/
    PyDoc_STR("Common base class for all exceptions"), /* tp_doc */
    (traverseproc)BaseException_traverse, /* tp_traverse */
    (inquiry)BaseException_clear, /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    BaseException_methods,      /* tp_methods */
    BaseException_members,      /* tp_members */
    BaseException_getset,       /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof(PyBaseExceptionObject, dict), /* tp_dictoffset */
    (initproc)BaseException_init, /* tp_init */
    0,                          /* tp_alloc */
    BaseException_new,          /* tp_new */
};
/* the CPython API expects exceptions to be (PyObject *) - both a hold-over
from the previous implementation and also allowing Python objects to be used
in the API */
PyObject *PyExc_BaseException = (PyObject *)&_PyExc_BaseException;

/* note these macros omit the last semicolon so the macro invocation may
 * include it and not look strange.
 */
#define SimpleExtendsException(EXCBASE, EXCNAME, EXCDOC) \
static PyTypeObject _PyExc_ ## EXCNAME = { \
    PyVarObject_HEAD_INIT(NULL, 0) \
    # EXCNAME, \
    sizeof(PyBaseExceptionObject), \
    0, (destructor)BaseException_dealloc, 0, 0, 0, 0, 0, 0, 0, \
    0, 0, 0, 0, 0, 0, 0, \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, \
    PyDoc_STR(EXCDOC), (traverseproc)BaseException_traverse, \
    (inquiry)BaseException_clear, 0, 0, 0, 0, 0, 0, 0, &_ ## EXCBASE, \
    0, 0, 0, offsetof(PyBaseExceptionObject, dict), \
    (initproc)BaseException_init, 0, BaseException_new,\
}; \
PyObject *PyExc_ ## EXCNAME = (PyObject *)&_PyExc_ ## EXCNAME

#define MiddlingExtendsExceptionEx(EXCBASE, EXCNAME, PYEXCNAME, EXCSTORE, EXCDOC) \
PyTypeObject _PyExc_ ## EXCNAME = { \
    PyVarObject_HEAD_INIT(NULL, 0) \
    # PYEXCNAME, \
    sizeof(Py ## EXCSTORE ## Object), \
    0, (destructor)EXCSTORE ## _dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    0, 0, 0, 0, 0, \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, \
    PyDoc_STR(EXCDOC), (traverseproc)EXCSTORE ## _traverse, \
    (inquiry)EXCSTORE ## _clear, 0, 0, 0, 0, 0, 0, 0, &_ ## EXCBASE, \
    0, 0, 0, offsetof(Py ## EXCSTORE ## Object, dict), \
    (initproc)EXCSTORE ## _init, 0, 0, \
};

#define MiddlingExtendsException(EXCBASE, EXCNAME, EXCSTORE, EXCDOC) \
    static MiddlingExtendsExceptionEx( \
        EXCBASE, EXCNAME, EXCNAME, EXCSTORE, EXCDOC); \
    PyObject *PyExc_ ## EXCNAME = (PyObject *)&_PyExc_ ## EXCNAME

#define ComplexExtendsException(EXCBASE, EXCNAME, EXCSTORE, EXCNEW, \
                                EXCMETHODS, EXCMEMBERS, EXCGETSET, \
                                EXCSTR, EXCDOC) \
static PyTypeObject _PyExc_ ## EXCNAME = { \
    PyVarObject_HEAD_INIT(NULL, 0) \
    # EXCNAME, \
    sizeof(Py ## EXCSTORE ## Object), 0, \
    (destructor)EXCSTORE ## _dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    (reprfunc)EXCSTR, 0, 0, 0, \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, \
    PyDoc_STR(EXCDOC), (traverseproc)EXCSTORE ## _traverse, \
    (inquiry)EXCSTORE ## _clear, 0, 0, 0, 0, EXCMETHODS, \
    EXCMEMBERS, EXCGETSET, &_ ## EXCBASE, \
    0, 0, 0, offsetof(Py ## EXCSTORE ## Object, dict), \
    (initproc)EXCSTORE ## _init, 0, EXCNEW,\
}; \
PyObject *PyExc_ ## EXCNAME = (PyObject *)&_PyExc_ ## EXCNAME


/*
 *    Exception extends BaseException
 */
SimpleExtendsException(PyExc_BaseException, Exception,
                       "Common base class for all non-exit exceptions.");


/*
 *    TypeError extends Exception
 */
SimpleExtendsException(PyExc_Exception, TypeError,
                       "Inappropriate argument type.");


/*
 *    StopAsyncIteration extends Exception
 */
SimpleExtendsException(PyExc_Exception, StopAsyncIteration,
                       "Signal the end from iterator.__anext__().");


/*
 *    StopIteration extends Exception
 */

static PyMemberDef StopIteration_members[] = {
    {"value", _Py_T_OBJECT, offsetof(PyStopIterationObject, value), 0,
        PyDoc_STR("generator return value")},
    {NULL}  /* Sentinel */
};

static int
StopIteration_init(PyStopIterationObject *self, PyObject *args, PyObject *kwds)
{
    Py_ssize_t size = PyTuple_GET_SIZE(args);
    PyObject *value;

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;
    Py_CLEAR(self->value);
    if (size > 0)
        value = PyTuple_GET_ITEM(args, 0);
    else
        value = Py_None;
    self->value = Py_NewRef(value);
    return 0;
}

static int
StopIteration_clear(PyStopIterationObject *self)
{
    Py_CLEAR(self->value);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
StopIteration_dealloc(PyStopIterationObject *self)
{
    PyObject_GC_UnTrack(self);
    StopIteration_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
StopIteration_traverse(PyStopIterationObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->value);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

ComplexExtendsException(PyExc_Exception, StopIteration, StopIteration,
                        0, 0, StopIteration_members, 0, 0,
                        "Signal the end from iterator.__next__().");


/*
 *    GeneratorExit extends BaseException
 */
SimpleExtendsException(PyExc_BaseException, GeneratorExit,
                       "Request that a generator exit.");


/*
 *    SystemExit extends BaseException
 */

static int
SystemExit_init(PySystemExitObject *self, PyObject *args, PyObject *kwds)
{
    Py_ssize_t size = PyTuple_GET_SIZE(args);

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    if (size == 0)
        return 0;
    if (size == 1) {
        Py_XSETREF(self->code, Py_NewRef(PyTuple_GET_ITEM(args, 0)));
    }
    else { /* size > 1 */
        Py_XSETREF(self->code, Py_NewRef(args));
    }
    return 0;
}

static int
SystemExit_clear(PySystemExitObject *self)
{
    Py_CLEAR(self->code);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
SystemExit_dealloc(PySystemExitObject *self)
{
    _PyObject_GC_UNTRACK(self);
    SystemExit_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
SystemExit_traverse(PySystemExitObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->code);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyMemberDef SystemExit_members[] = {
    {"code", _Py_T_OBJECT, offsetof(PySystemExitObject, code), 0,
        PyDoc_STR("exception code")},
    {NULL}  /* Sentinel */
};

ComplexExtendsException(PyExc_BaseException, SystemExit, SystemExit,
                        0, 0, SystemExit_members, 0, 0,
                        "Request to exit from the interpreter.");

/*
 *    BaseExceptionGroup extends BaseException
 *    ExceptionGroup extends BaseExceptionGroup and Exception
 */


static inline PyBaseExceptionGroupObject*
_PyBaseExceptionGroupObject_cast(PyObject *exc)
{
    assert(_PyBaseExceptionGroup_Check(exc));
    return (PyBaseExceptionGroupObject *)exc;
}

static PyObject *
BaseExceptionGroup_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    struct _Py_exc_state *state = get_exc_state();
    PyTypeObject *PyExc_ExceptionGroup =
        (PyTypeObject*)state->PyExc_ExceptionGroup;

    PyObject *message = NULL;
    PyObject *exceptions = NULL;

    if (!PyArg_ParseTuple(args,
                          "UO:BaseExceptionGroup.__new__",
                          &message,
                          &exceptions)) {
        return NULL;
    }

    if (!PySequence_Check(exceptions)) {
        PyErr_SetString(
            PyExc_TypeError,
            "second argument (exceptions) must be a sequence");
        return NULL;
    }

    exceptions = PySequence_Tuple(exceptions);
    if (!exceptions) {
        return NULL;
    }

    /* We are now holding a ref to the exceptions tuple */

    Py_ssize_t numexcs = PyTuple_GET_SIZE(exceptions);
    if (numexcs == 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "second argument (exceptions) must be a non-empty sequence");
        goto error;
    }

    bool nested_base_exceptions = false;
    for (Py_ssize_t i = 0; i < numexcs; i++) {
        PyObject *exc = PyTuple_GET_ITEM(exceptions, i);
        if (!exc) {
            goto error;
        }
        if (!PyExceptionInstance_Check(exc)) {
            PyErr_Format(
                PyExc_ValueError,
                "Item %d of second argument (exceptions) is not an exception",
                i);
            goto error;
        }
        int is_nonbase_exception = PyObject_IsInstance(exc, PyExc_Exception);
        if (is_nonbase_exception < 0) {
            goto error;
        }
        else if (is_nonbase_exception == 0) {
            nested_base_exceptions = true;
        }
    }

    PyTypeObject *cls = type;
    if (cls == PyExc_ExceptionGroup) {
        if (nested_base_exceptions) {
            PyErr_SetString(PyExc_TypeError,
                "Cannot nest BaseExceptions in an ExceptionGroup");
            goto error;
        }
    }
    else if (cls == (PyTypeObject*)PyExc_BaseExceptionGroup) {
        if (!nested_base_exceptions) {
            /* All nested exceptions are Exception subclasses,
             * wrap them in an ExceptionGroup
             */
            cls = PyExc_ExceptionGroup;
        }
    }
    else {
        /* user-defined subclass */
        if (nested_base_exceptions) {
            int nonbase = PyObject_IsSubclass((PyObject*)cls, PyExc_Exception);
            if (nonbase == -1) {
                goto error;
            }
            else if (nonbase == 1) {
                PyErr_Format(PyExc_TypeError,
                    "Cannot nest BaseExceptions in '%.200s'",
                    cls->tp_name);
                goto error;
            }
        }
    }

    if (!cls) {
        /* Don't crash during interpreter shutdown
         * (PyExc_ExceptionGroup may have been cleared)
         */
        cls = (PyTypeObject*)PyExc_BaseExceptionGroup;
    }
    PyBaseExceptionGroupObject *self =
        _PyBaseExceptionGroupObject_cast(BaseException_new(cls, args, kwds));
    if (!self) {
        goto error;
    }

    self->msg = Py_NewRef(message);
    self->excs = exceptions;
    return (PyObject*)self;
error:
    Py_DECREF(exceptions);
    return NULL;
}

PyObject *
_PyExc_CreateExceptionGroup(const char *msg_str, PyObject *excs)
{
    PyObject *msg = PyUnicode_FromString(msg_str);
    if (!msg) {
        return NULL;
    }
    PyObject *args = PyTuple_Pack(2, msg, excs);
    Py_DECREF(msg);
    if (!args) {
        return NULL;
    }
    PyObject *result = PyObject_CallObject(PyExc_BaseExceptionGroup, args);
    Py_DECREF(args);
    return result;
}

static int
BaseExceptionGroup_init(PyBaseExceptionGroupObject *self,
    PyObject *args, PyObject *kwds)
{
    if (!_PyArg_NoKeywords(Py_TYPE(self)->tp_name, kwds)) {
        return -1;
    }
    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1) {
        return -1;
    }
    return 0;
}

static int
BaseExceptionGroup_clear(PyBaseExceptionGroupObject *self)
{
    Py_CLEAR(self->msg);
    Py_CLEAR(self->excs);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
BaseExceptionGroup_dealloc(PyBaseExceptionGroupObject *self)
{
    _PyObject_GC_UNTRACK(self);
    BaseExceptionGroup_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
BaseExceptionGroup_traverse(PyBaseExceptionGroupObject *self,
     visitproc visit, void *arg)
{
    Py_VISIT(self->msg);
    Py_VISIT(self->excs);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyObject *
BaseExceptionGroup_str(PyBaseExceptionGroupObject *self)
{
    assert(self->msg);
    assert(PyUnicode_Check(self->msg));

    assert(PyTuple_CheckExact(self->excs));
    Py_ssize_t num_excs = PyTuple_Size(self->excs);
    return PyUnicode_FromFormat(
        "%S (%zd sub-exception%s)",
        self->msg, num_excs, num_excs > 1 ? "s" : "");
}

static PyObject *
BaseExceptionGroup_derive(PyObject *self_, PyObject *excs)
{
    PyBaseExceptionGroupObject *self = _PyBaseExceptionGroupObject_cast(self_);
    PyObject *init_args = PyTuple_Pack(2, self->msg, excs);
    if (!init_args) {
        return NULL;
    }
    PyObject *eg = PyObject_CallObject(
        PyExc_BaseExceptionGroup, init_args);
    Py_DECREF(init_args);
    return eg;
}

static int
exceptiongroup_subset(
    PyBaseExceptionGroupObject *_orig, PyObject *excs, PyObject **result)
{
    /* Sets *result to an ExceptionGroup wrapping excs with metadata from
     * _orig. If excs is empty, sets *result to NULL.
     * Returns 0 on success and -1 on error.

     * This function is used by split() to construct the match/rest parts,
     * so excs is the matching or non-matching sub-sequence of orig->excs
     * (this function does not verify that it is a subsequence).
     */
    PyObject *orig = (PyObject *)_orig;

    *result = NULL;
    Py_ssize_t num_excs = PySequence_Size(excs);
    if (num_excs < 0) {
        return -1;
    }
    else if (num_excs == 0) {
        return 0;
    }

    PyObject *eg = PyObject_CallMethod(
        orig, "derive", "(O)", excs);
    if (!eg) {
        return -1;
    }

    if (!_PyBaseExceptionGroup_Check(eg)) {
        PyErr_SetString(PyExc_TypeError,
            "derive must return an instance of BaseExceptionGroup");
        goto error;
    }

    /* Now we hold a reference to the new eg */

    PyObject *tb = PyException_GetTraceback(orig);
    if (tb) {
        int res = PyException_SetTraceback(eg, tb);
        Py_DECREF(tb);
        if (res < 0) {
            goto error;
        }
    }
    PyException_SetContext(eg, PyException_GetContext(orig));
    PyException_SetCause(eg, PyException_GetCause(orig));

    PyObject *notes;
    if (PyObject_GetOptionalAttr(orig, &_Py_ID(__notes__), &notes) < 0) {
        goto error;
    }
    if (notes) {
        if (PySequence_Check(notes)) {
            /* Make a copy so the parts have independent notes lists. */
            PyObject *notes_copy = PySequence_List(notes);
            Py_DECREF(notes);
            if (notes_copy == NULL) {
                goto error;
            }
            int res = PyObject_SetAttr(eg, &_Py_ID(__notes__), notes_copy);
            Py_DECREF(notes_copy);
            if (res < 0) {
                goto error;
            }
        }
        else {
            /* __notes__ is supposed to be a list, and split() is not a
             * good place to report earlier user errors, so we just ignore
             * notes of non-sequence type.
             */
            Py_DECREF(notes);
        }
    }

    *result = eg;
    return 0;
error:
    Py_DECREF(eg);
    return -1;
}

typedef enum {
    /* Exception type or tuple of thereof */
    EXCEPTION_GROUP_MATCH_BY_TYPE = 0,
    /* A PyFunction returning True for matching exceptions */
    EXCEPTION_GROUP_MATCH_BY_PREDICATE = 1,
    /* A set of the IDs of leaf exceptions to include in the result.
     * This matcher type is used internally by the interpreter
     * to construct reraised exceptions.
     */
    EXCEPTION_GROUP_MATCH_INSTANCE_IDS = 2
} _exceptiongroup_split_matcher_type;

static int
get_matcher_type(PyObject *value,
                 _exceptiongroup_split_matcher_type *type)
{
    assert(value);

    if (PyCallable_Check(value) && !PyType_Check(value)) {
        *type = EXCEPTION_GROUP_MATCH_BY_PREDICATE;
        return 0;
    }

    if (PyExceptionClass_Check(value)) {
        *type = EXCEPTION_GROUP_MATCH_BY_TYPE;
        return 0;
    }

    if (PyTuple_CheckExact(value)) {
        Py_ssize_t n = PyTuple_GET_SIZE(value);
        for (Py_ssize_t i=0; i<n; i++) {
            if (!PyExceptionClass_Check(PyTuple_GET_ITEM(value, i))) {
                goto error;
            }
        }
        *type = EXCEPTION_GROUP_MATCH_BY_TYPE;
        return 0;
    }

error:
    PyErr_SetString(
        PyExc_TypeError,
        "expected an exception type, a tuple of exception types, or a callable (other than a class)");
    return -1;
}

static int
exceptiongroup_split_check_match(PyObject *exc,
                                 _exceptiongroup_split_matcher_type matcher_type,
                                 PyObject *matcher_value)
{
    switch (matcher_type) {
    case EXCEPTION_GROUP_MATCH_BY_TYPE: {
        assert(PyExceptionClass_Check(matcher_value) ||
               PyTuple_CheckExact(matcher_value));
        return PyErr_GivenExceptionMatches(exc, matcher_value);
    }
    case EXCEPTION_GROUP_MATCH_BY_PREDICATE: {
        assert(PyCallable_Check(matcher_value) && !PyType_Check(matcher_value));
        PyObject *exc_matches = PyObject_CallOneArg(matcher_value, exc);
        if (exc_matches == NULL) {
            return -1;
        }
        int is_true = PyObject_IsTrue(exc_matches);
        Py_DECREF(exc_matches);
        return is_true;
    }
    case EXCEPTION_GROUP_MATCH_INSTANCE_IDS: {
        assert(PySet_Check(matcher_value));
        if (!_PyBaseExceptionGroup_Check(exc)) {
            PyObject *exc_id = PyLong_FromVoidPtr(exc);
            if (exc_id == NULL) {
                return -1;
            }
            int res = PySet_Contains(matcher_value, exc_id);
            Py_DECREF(exc_id);
            return res;
        }
        return 0;
    }
    }
    return 0;
}

typedef struct {
    PyObject *match;
    PyObject *rest;
} _exceptiongroup_split_result;

static int
exceptiongroup_split_recursive(PyObject *exc,
                               _exceptiongroup_split_matcher_type matcher_type,
                               PyObject *matcher_value,
                               bool construct_rest,
                               _exceptiongroup_split_result *result)
{
    result->match = NULL;
    result->rest = NULL;

    int is_match = exceptiongroup_split_check_match(
        exc, matcher_type, matcher_value);
    if (is_match < 0) {
        return -1;
    }

    if (is_match) {
        /* Full match */
        result->match = Py_NewRef(exc);
        return 0;
    }
    else if (!_PyBaseExceptionGroup_Check(exc)) {
        /* Leaf exception and no match */
        if (construct_rest) {
            result->rest = Py_NewRef(exc);
        }
        return 0;
    }

    /* Partial match */

    PyBaseExceptionGroupObject *eg = _PyBaseExceptionGroupObject_cast(exc);
    assert(PyTuple_CheckExact(eg->excs));
    Py_ssize_t num_excs = PyTuple_Size(eg->excs);
    if (num_excs < 0) {
        return -1;
    }
    assert(num_excs > 0); /* checked in constructor, and excs is read-only */

    int retval = -1;
    PyObject *match_list = PyList_New(0);
    if (!match_list) {
        return -1;
    }

    PyObject *rest_list = NULL;
    if (construct_rest) {
        rest_list = PyList_New(0);
        if (!rest_list) {
            goto done;
        }
    }
    /* recursive calls */
    for (Py_ssize_t i = 0; i < num_excs; i++) {
        PyObject *e = PyTuple_GET_ITEM(eg->excs, i);
        _exceptiongroup_split_result rec_result;
        if (_Py_EnterRecursiveCall(" in exceptiongroup_split_recursive")) {
            goto done;
        }
        if (exceptiongroup_split_recursive(
                e, matcher_type, matcher_value,
                construct_rest, &rec_result) < 0) {
            assert(!rec_result.match);
            assert(!rec_result.rest);
            _Py_LeaveRecursiveCall();
            goto done;
        }
        _Py_LeaveRecursiveCall();
        if (rec_result.match) {
            assert(PyList_CheckExact(match_list));
            if (PyList_Append(match_list, rec_result.match) < 0) {
                Py_DECREF(rec_result.match);
                Py_XDECREF(rec_result.rest);
                goto done;
            }
            Py_DECREF(rec_result.match);
        }
        if (rec_result.rest) {
            assert(construct_rest);
            assert(PyList_CheckExact(rest_list));
            if (PyList_Append(rest_list, rec_result.rest) < 0) {
                Py_DECREF(rec_result.rest);
                goto done;
            }
            Py_DECREF(rec_result.rest);
        }
    }

    /* construct result */
    if (exceptiongroup_subset(eg, match_list, &result->match) < 0) {
        goto done;
    }

    if (construct_rest) {
        assert(PyList_CheckExact(rest_list));
        if (exceptiongroup_subset(eg, rest_list, &result->rest) < 0) {
            Py_CLEAR(result->match);
            goto done;
        }
    }
    retval = 0;
done:
    Py_DECREF(match_list);
    Py_XDECREF(rest_list);
    if (retval < 0) {
        Py_CLEAR(result->match);
        Py_CLEAR(result->rest);
    }
    return retval;
}

static PyObject *
BaseExceptionGroup_split(PyObject *self, PyObject *matcher_value)
{
    _exceptiongroup_split_matcher_type matcher_type;
    if (get_matcher_type(matcher_value, &matcher_type) < 0) {
        return NULL;
    }

    _exceptiongroup_split_result split_result;
    bool construct_rest = true;
    if (exceptiongroup_split_recursive(
            self, matcher_type, matcher_value,
            construct_rest, &split_result) < 0) {
        return NULL;
    }

    PyObject *result = PyTuple_Pack(
            2,
            split_result.match ? split_result.match : Py_None,
            split_result.rest ? split_result.rest : Py_None);

    Py_XDECREF(split_result.match);
    Py_XDECREF(split_result.rest);
    return result;
}

static PyObject *
BaseExceptionGroup_subgroup(PyObject *self, PyObject *matcher_value)
{
    _exceptiongroup_split_matcher_type matcher_type;
    if (get_matcher_type(matcher_value, &matcher_type) < 0) {
        return NULL;
    }

    _exceptiongroup_split_result split_result;
    bool construct_rest = false;
    if (exceptiongroup_split_recursive(
            self, matcher_type, matcher_value,
            construct_rest, &split_result) < 0) {
        return NULL;
    }

    PyObject *result = Py_NewRef(
            split_result.match ? split_result.match : Py_None);

    Py_XDECREF(split_result.match);
    assert(!split_result.rest);
    return result;
}

static int
collect_exception_group_leaf_ids(PyObject *exc, PyObject *leaf_ids)
{
    if (Py_IsNone(exc)) {
        return 0;
    }

    assert(PyExceptionInstance_Check(exc));
    assert(PySet_Check(leaf_ids));

    /* Add IDs of all leaf exceptions in exc to the leaf_ids set */

    if (!_PyBaseExceptionGroup_Check(exc)) {
        PyObject *exc_id = PyLong_FromVoidPtr(exc);
        if (exc_id == NULL) {
            return -1;
        }
        int res = PySet_Add(leaf_ids, exc_id);
        Py_DECREF(exc_id);
        return res;
    }
    PyBaseExceptionGroupObject *eg = _PyBaseExceptionGroupObject_cast(exc);
    Py_ssize_t num_excs = PyTuple_GET_SIZE(eg->excs);
    /* recursive calls */
    for (Py_ssize_t i = 0; i < num_excs; i++) {
        PyObject *e = PyTuple_GET_ITEM(eg->excs, i);
        if (_Py_EnterRecursiveCall(" in collect_exception_group_leaf_ids")) {
            return -1;
        }
        int res = collect_exception_group_leaf_ids(e, leaf_ids);
        _Py_LeaveRecursiveCall();
        if (res < 0) {
            return -1;
        }
    }
    return 0;
}

/* This function is used by the interpreter to construct reraised
 * exception groups. It takes an exception group eg and a list
 * of exception groups keep and returns the sub-exception group
 * of eg which contains all leaf exceptions that are contained
 * in any exception group in keep.
 */
static PyObject *
exception_group_projection(PyObject *eg, PyObject *keep)
{
    assert(_PyBaseExceptionGroup_Check(eg));
    assert(PyList_CheckExact(keep));

    PyObject *leaf_ids = PySet_New(NULL);
    if (!leaf_ids) {
        return NULL;
    }

    Py_ssize_t n = PyList_GET_SIZE(keep);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *e = PyList_GET_ITEM(keep, i);
        assert(e != NULL);
        assert(_PyBaseExceptionGroup_Check(e));
        if (collect_exception_group_leaf_ids(e, leaf_ids) < 0) {
            Py_DECREF(leaf_ids);
            return NULL;
        }
    }

    _exceptiongroup_split_result split_result;
    bool construct_rest = false;
    int err = exceptiongroup_split_recursive(
                eg, EXCEPTION_GROUP_MATCH_INSTANCE_IDS, leaf_ids,
                construct_rest, &split_result);
    Py_DECREF(leaf_ids);
    if (err < 0) {
        return NULL;
    }

    PyObject *result = split_result.match ?
        split_result.match : Py_NewRef(Py_None);
    assert(split_result.rest == NULL);
    return result;
}

static bool
is_same_exception_metadata(PyObject *exc1, PyObject *exc2)
{
    assert(PyExceptionInstance_Check(exc1));
    assert(PyExceptionInstance_Check(exc2));

    PyBaseExceptionObject *e1 = (PyBaseExceptionObject *)exc1;
    PyBaseExceptionObject *e2 = (PyBaseExceptionObject *)exc2;

    return (e1->notes == e2->notes &&
            e1->traceback == e2->traceback &&
            e1->cause == e2->cause &&
            e1->context == e2->context);
}

/*
   This function is used by the interpreter to calculate
   the exception group to be raised at the end of a
   try-except* construct.

   orig: the original except that was caught.
   excs: a list of exceptions that were raised/reraised
         in the except* clauses.

   Calculates an exception group to raise. It contains
   all exceptions in excs, where those that were reraised
   have same nesting structure as in orig, and those that
   were raised (if any) are added as siblings in a new EG.

   Returns NULL and sets an exception on failure.
*/
PyObject *
_PyExc_PrepReraiseStar(PyObject *orig, PyObject *excs)
{
    /* orig must be a raised & caught exception, so it has a traceback */
    assert(PyExceptionInstance_Check(orig));
    assert(_PyBaseExceptionObject_cast(orig)->traceback != NULL);

    assert(PyList_Check(excs));

    Py_ssize_t numexcs = PyList_GET_SIZE(excs);

    if (numexcs == 0) {
        return Py_NewRef(Py_None);
    }

    if (!_PyBaseExceptionGroup_Check(orig)) {
        /* a naked exception was caught and wrapped. Only one except* clause
         * could have executed,so there is at most one exception to raise.
         */

        assert(numexcs == 1 || (numexcs == 2 && PyList_GET_ITEM(excs, 1) == Py_None));

        PyObject *e = PyList_GET_ITEM(excs, 0);
        assert(e != NULL);
        return Py_NewRef(e);
    }

    PyObject *raised_list = PyList_New(0);
    if (raised_list == NULL) {
        return NULL;
    }
    PyObject* reraised_list = PyList_New(0);
    if (reraised_list == NULL) {
        Py_DECREF(raised_list);
        return NULL;
    }

    /* Now we are holding refs to raised_list and reraised_list */

    PyObject *result = NULL;

    /* Split excs into raised and reraised by comparing metadata with orig */
    for (Py_ssize_t i = 0; i < numexcs; i++) {
        PyObject *e = PyList_GET_ITEM(excs, i);
        assert(e != NULL);
        if (Py_IsNone(e)) {
            continue;
        }
        bool is_reraise = is_same_exception_metadata(e, orig);
        PyObject *append_list = is_reraise ? reraised_list : raised_list;
        if (PyList_Append(append_list, e) < 0) {
            goto done;
        }
    }

    PyObject *reraised_eg = exception_group_projection(orig, reraised_list);
    if (reraised_eg == NULL) {
        goto done;
    }

    if (!Py_IsNone(reraised_eg)) {
        assert(is_same_exception_metadata(reraised_eg, orig));
    }
    Py_ssize_t num_raised = PyList_GET_SIZE(raised_list);
    if (num_raised == 0) {
        result = reraised_eg;
    }
    else if (num_raised > 0) {
        int res = 0;
        if (!Py_IsNone(reraised_eg)) {
            res = PyList_Append(raised_list, reraised_eg);
        }
        Py_DECREF(reraised_eg);
        if (res < 0) {
            goto done;
        }
        if (PyList_GET_SIZE(raised_list) > 1) {
            result = _PyExc_CreateExceptionGroup("", raised_list);
        }
        else {
            result = Py_NewRef(PyList_GetItem(raised_list, 0));
        }
        if (result == NULL) {
            goto done;
        }
    }

done:
    Py_XDECREF(raised_list);
    Py_XDECREF(reraised_list);
    return result;
}

PyObject *
PyUnstable_Exc_PrepReraiseStar(PyObject *orig, PyObject *excs)
{
    if (orig == NULL || !PyExceptionInstance_Check(orig)) {
        PyErr_SetString(PyExc_TypeError, "orig must be an exception instance");
        return NULL;
    }
    if (excs == NULL || !PyList_Check(excs)) {
        PyErr_SetString(PyExc_TypeError,
                        "excs must be a list of exception instances");
        return NULL;
    }
    Py_ssize_t numexcs = PyList_GET_SIZE(excs);
    for (Py_ssize_t i = 0; i < numexcs; i++) {
        PyObject *exc = PyList_GET_ITEM(excs, i);
        if (exc == NULL || !(PyExceptionInstance_Check(exc) || Py_IsNone(exc))) {
            PyErr_Format(PyExc_TypeError,
                         "item %d of excs is not an exception", i);
            return NULL;
        }
    }

    /* Make sure that orig has something as traceback, in the interpreter
     * it always does because it's a raised exception.
     */
    PyObject *tb = PyException_GetTraceback(orig);

    if (tb == NULL) {
        PyErr_Format(PyExc_ValueError, "orig must be a raised exception");
        return NULL;
    }
    Py_DECREF(tb);

    return _PyExc_PrepReraiseStar(orig, excs);
}

static PyMemberDef BaseExceptionGroup_members[] = {
    {"message", _Py_T_OBJECT, offsetof(PyBaseExceptionGroupObject, msg), Py_READONLY,
        PyDoc_STR("exception message")},
    {"exceptions", _Py_T_OBJECT, offsetof(PyBaseExceptionGroupObject, excs), Py_READONLY,
        PyDoc_STR("nested exceptions")},
    {NULL}  /* Sentinel */
};

static PyMethodDef BaseExceptionGroup_methods[] = {
    {"__class_getitem__", (PyCFunction)Py_GenericAlias,
      METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {"derive", (PyCFunction)BaseExceptionGroup_derive, METH_O},
    {"split", (PyCFunction)BaseExceptionGroup_split, METH_O},
    {"subgroup", (PyCFunction)BaseExceptionGroup_subgroup, METH_O},
    {NULL}
};

ComplexExtendsException(PyExc_BaseException, BaseExceptionGroup,
    BaseExceptionGroup, BaseExceptionGroup_new /* new */,
    BaseExceptionGroup_methods, BaseExceptionGroup_members,
    0 /* getset */, BaseExceptionGroup_str,
    "A combination of multiple unrelated exceptions.");

/*
 *    ExceptionGroup extends BaseExceptionGroup, Exception
 */
static PyObject*
create_exception_group_class(void) {
    struct _Py_exc_state *state = get_exc_state();

    PyObject *bases = PyTuple_Pack(
        2, PyExc_BaseExceptionGroup, PyExc_Exception);
    if (bases == NULL) {
        return NULL;
    }

    assert(!state->PyExc_ExceptionGroup);
    state->PyExc_ExceptionGroup = PyErr_NewException(
        "builtins.ExceptionGroup", bases, NULL);

    Py_DECREF(bases);
    return state->PyExc_ExceptionGroup;
}

/*
 *    KeyboardInterrupt extends BaseException
 */
SimpleExtendsException(PyExc_BaseException, KeyboardInterrupt,
                       "Program interrupted by user.");


/*
 *    ImportError extends Exception
 */

static int
ImportError_init(PyImportErrorObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", "path", "name_from", 0};
    PyObject *empty_tuple;
    PyObject *msg = NULL;
    PyObject *name = NULL;
    PyObject *path = NULL;
    PyObject *name_from = NULL;

    if (BaseException_init((PyBaseExceptionObject *)self, args, NULL) == -1)
        return -1;

    empty_tuple = PyTuple_New(0);
    if (!empty_tuple)
        return -1;
    if (!PyArg_ParseTupleAndKeywords(empty_tuple, kwds, "|$OOO:ImportError", kwlist,
                                     &name, &path, &name_from)) {
        Py_DECREF(empty_tuple);
        return -1;
    }
    Py_DECREF(empty_tuple);

    Py_XSETREF(self->name, Py_XNewRef(name));
    Py_XSETREF(self->path, Py_XNewRef(path));
    Py_XSETREF(self->name_from, Py_XNewRef(name_from));

    if (PyTuple_GET_SIZE(args) == 1) {
        msg = Py_NewRef(PyTuple_GET_ITEM(args, 0));
    }
    Py_XSETREF(self->msg, msg);

    return 0;
}

static int
ImportError_clear(PyImportErrorObject *self)
{
    Py_CLEAR(self->msg);
    Py_CLEAR(self->name);
    Py_CLEAR(self->path);
    Py_CLEAR(self->name_from);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
ImportError_dealloc(PyImportErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    ImportError_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
ImportError_traverse(PyImportErrorObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->msg);
    Py_VISIT(self->name);
    Py_VISIT(self->path);
    Py_VISIT(self->name_from);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyObject *
ImportError_str(PyImportErrorObject *self)
{
    if (self->msg && PyUnicode_CheckExact(self->msg)) {
        return Py_NewRef(self->msg);
    }
    else {
        return BaseException_str((PyBaseExceptionObject *)self);
    }
}

static PyObject *
ImportError_getstate(PyImportErrorObject *self)
{
    PyObject *dict = ((PyBaseExceptionObject *)self)->dict;
    if (self->name || self->path || self->name_from) {
        dict = dict ? PyDict_Copy(dict) : PyDict_New();
        if (dict == NULL)
            return NULL;
        if (self->name && PyDict_SetItem(dict, &_Py_ID(name), self->name) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        if (self->path && PyDict_SetItem(dict, &_Py_ID(path), self->path) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        if (self->name_from && PyDict_SetItem(dict, &_Py_ID(name_from), self->name_from) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        return dict;
    }
    else if (dict) {
        return Py_NewRef(dict);
    }
    else {
        Py_RETURN_NONE;
    }
}

/* Pickling support */
static PyObject *
ImportError_reduce(PyImportErrorObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *res;
    PyObject *args;
    PyObject *state = ImportError_getstate(self);
    if (state == NULL)
        return NULL;
    args = ((PyBaseExceptionObject *)self)->args;
    if (state == Py_None)
        res = PyTuple_Pack(2, Py_TYPE(self), args);
    else
        res = PyTuple_Pack(3, Py_TYPE(self), args, state);
    Py_DECREF(state);
    return res;
}

static PyMemberDef ImportError_members[] = {
    {"msg", _Py_T_OBJECT, offsetof(PyImportErrorObject, msg), 0,
        PyDoc_STR("exception message")},
    {"name", _Py_T_OBJECT, offsetof(PyImportErrorObject, name), 0,
        PyDoc_STR("module name")},
    {"path", _Py_T_OBJECT, offsetof(PyImportErrorObject, path), 0,
        PyDoc_STR("module path")},
    {"name_from", _Py_T_OBJECT, offsetof(PyImportErrorObject, name_from), 0,
        PyDoc_STR("name imported from module")},
    {NULL}  /* Sentinel */
};

static PyMethodDef ImportError_methods[] = {
    {"__reduce__", (PyCFunction)ImportError_reduce, METH_NOARGS},
    {NULL}
};

ComplexExtendsException(PyExc_Exception, ImportError,
                        ImportError, 0 /* new */,
                        ImportError_methods, ImportError_members,
                        0 /* getset */, ImportError_str,
                        "Import can't find module, or can't find name in "
                        "module.");

/*
 *    ModuleNotFoundError extends ImportError
 */

MiddlingExtendsException(PyExc_ImportError, ModuleNotFoundError, ImportError,
                         "Module not found.");

/*
 *    OSError extends Exception
 */

#ifdef MS_WINDOWS
#include "errmap.h"
#endif

/* Where a function has a single filename, such as open() or some
 * of the os module functions, PyErr_SetFromErrnoWithFilename() is
 * called, giving a third argument which is the filename.  But, so
 * that old code using in-place unpacking doesn't break, e.g.:
 *
 * except OSError, (errno, strerror):
 *
 * we hack args so that it only contains two items.  This also
 * means we need our own __str__() which prints out the filename
 * when it was supplied.
 *
 * (If a function has two filenames, such as rename(), symlink(),
 * or copy(), PyErr_SetFromErrnoWithFilenameObjects() is called,
 * which allows passing in a second filename.)
 */

/* This function doesn't cleanup on error, the caller should */
static int
oserror_parse_args(PyObject **p_args,
                   PyObject **myerrno, PyObject **strerror,
                   PyObject **filename, PyObject **filename2
#ifdef MS_WINDOWS
                   , PyObject **winerror
#endif
                  )
{
    Py_ssize_t nargs;
    PyObject *args = *p_args;
#ifndef MS_WINDOWS
    /*
     * ignored on non-Windows platforms,
     * but parsed so OSError has a consistent signature
     */
    PyObject *_winerror = NULL;
    PyObject **winerror = &_winerror;
#endif /* MS_WINDOWS */

    nargs = PyTuple_GET_SIZE(args);

    if (nargs >= 2 && nargs <= 5) {
        if (!PyArg_UnpackTuple(args, "OSError", 2, 5,
                               myerrno, strerror,
                               filename, winerror, filename2))
            return -1;
#ifdef MS_WINDOWS
        if (*winerror && PyLong_Check(*winerror)) {
            long errcode, winerrcode;
            PyObject *newargs;
            Py_ssize_t i;

            winerrcode = PyLong_AsLong(*winerror);
            if (winerrcode == -1 && PyErr_Occurred())
                return -1;
            errcode = winerror_to_errno(winerrcode);
            *myerrno = PyLong_FromLong(errcode);
            if (!*myerrno)
                return -1;
            newargs = PyTuple_New(nargs);
            if (!newargs)
                return -1;
            PyTuple_SET_ITEM(newargs, 0, *myerrno);
            for (i = 1; i < nargs; i++) {
                PyObject *val = PyTuple_GET_ITEM(args, i);
                PyTuple_SET_ITEM(newargs, i, Py_NewRef(val));
            }
            Py_DECREF(args);
            args = *p_args = newargs;
        }
#endif /* MS_WINDOWS */
    }

    return 0;
}

static int
oserror_init(PyOSErrorObject *self, PyObject **p_args,
             PyObject *myerrno, PyObject *strerror,
             PyObject *filename, PyObject *filename2
#ifdef MS_WINDOWS
             , PyObject *winerror
#endif
             )
{
    PyObject *args = *p_args;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);

    /* self->filename will remain Py_None otherwise */
    if (filename && filename != Py_None) {
        if (Py_IS_TYPE(self, (PyTypeObject *) PyExc_BlockingIOError) &&
            PyNumber_Check(filename)) {
            /* BlockingIOError's 3rd argument can be the number of
             * characters written.
             */
            self->written = PyNumber_AsSsize_t(filename, PyExc_ValueError);
            if (self->written == -1 && PyErr_Occurred())
                return -1;
        }
        else {
            self->filename = Py_NewRef(filename);

            if (filename2 && filename2 != Py_None) {
                self->filename2 = Py_NewRef(filename2);
            }

            if (nargs >= 2 && nargs <= 5) {
                /* filename, filename2, and winerror are removed from the args tuple
                   (for compatibility purposes, see test_exceptions.py) */
                PyObject *subslice = PyTuple_GetSlice(args, 0, 2);
                if (!subslice)
                    return -1;

                Py_DECREF(args);  /* replacing args */
                *p_args = args = subslice;
            }
        }
    }
    self->myerrno = Py_XNewRef(myerrno);
    self->strerror = Py_XNewRef(strerror);
#ifdef MS_WINDOWS
    self->winerror = Py_XNewRef(winerror);
#endif

    /* Steals the reference to args */
    Py_XSETREF(self->args, args);
    *p_args = args = NULL;

    return 0;
}

static PyObject *
OSError_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int
OSError_init(PyOSErrorObject *self, PyObject *args, PyObject *kwds);

static int
oserror_use_init(PyTypeObject *type)
{
    /* When __init__ is defined in an OSError subclass, we want any
       extraneous argument to __new__ to be ignored.  The only reasonable
       solution, given __new__ takes a variable number of arguments,
       is to defer arg parsing and initialization to __init__.

       But when __new__ is overridden as well, it should call our __new__
       with the right arguments.

       (see http://bugs.python.org/issue12555#msg148829 )
    */
    if (type->tp_init != (initproc) OSError_init &&
        type->tp_new == (newfunc) OSError_new) {
        assert((PyObject *) type != PyExc_OSError);
        return 1;
    }
    return 0;
}

static PyObject *
OSError_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyOSErrorObject *self = NULL;
    PyObject *myerrno = NULL, *strerror = NULL;
    PyObject *filename = NULL, *filename2 = NULL;
#ifdef MS_WINDOWS
    PyObject *winerror = NULL;
#endif

    Py_INCREF(args);

    if (!oserror_use_init(type)) {
        if (!_PyArg_NoKeywords(type->tp_name, kwds))
            goto error;

        if (oserror_parse_args(&args, &myerrno, &strerror,
                               &filename, &filename2
#ifdef MS_WINDOWS
                               , &winerror
#endif
            ))
            goto error;

        struct _Py_exc_state *state = get_exc_state();
        if (myerrno && PyLong_Check(myerrno) &&
            state->errnomap && (PyObject *) type == PyExc_OSError) {
            PyObject *newtype;
            newtype = PyDict_GetItemWithError(state->errnomap, myerrno);
            if (newtype) {
                type = _PyType_CAST(newtype);
            }
            else if (PyErr_Occurred())
                goto error;
        }
    }

    self = (PyOSErrorObject *) type->tp_alloc(type, 0);
    if (!self)
        goto error;

    self->dict = NULL;
    self->traceback = self->cause = self->context = NULL;
    self->written = -1;

    if (!oserror_use_init(type)) {
        if (oserror_init(self, &args, myerrno, strerror, filename, filename2
#ifdef MS_WINDOWS
                         , winerror
#endif
            ))
            goto error;
    }
    else {
        self->args = PyTuple_New(0);
        if (self->args == NULL)
            goto error;
    }

    Py_XDECREF(args);
    return (PyObject *) self;

error:
    Py_XDECREF(args);
    Py_XDECREF(self);
    return NULL;
}

static int
OSError_init(PyOSErrorObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *myerrno = NULL, *strerror = NULL;
    PyObject *filename = NULL, *filename2 = NULL;
#ifdef MS_WINDOWS
    PyObject *winerror = NULL;
#endif

    if (!oserror_use_init(Py_TYPE(self)))
        /* Everything already done in OSError_new */
        return 0;

    if (!_PyArg_NoKeywords(Py_TYPE(self)->tp_name, kwds))
        return -1;

    Py_INCREF(args);
    if (oserror_parse_args(&args, &myerrno, &strerror, &filename, &filename2
#ifdef MS_WINDOWS
                           , &winerror
#endif
        ))
        goto error;

    if (oserror_init(self, &args, myerrno, strerror, filename, filename2
#ifdef MS_WINDOWS
                     , winerror
#endif
        ))
        goto error;

    return 0;

error:
    Py_DECREF(args);
    return -1;
}

static int
OSError_clear(PyOSErrorObject *self)
{
    Py_CLEAR(self->myerrno);
    Py_CLEAR(self->strerror);
    Py_CLEAR(self->filename);
    Py_CLEAR(self->filename2);
#ifdef MS_WINDOWS
    Py_CLEAR(self->winerror);
#endif
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
OSError_dealloc(PyOSErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    OSError_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
OSError_traverse(PyOSErrorObject *self, visitproc visit,
        void *arg)
{
    Py_VISIT(self->myerrno);
    Py_VISIT(self->strerror);
    Py_VISIT(self->filename);
    Py_VISIT(self->filename2);
#ifdef MS_WINDOWS
    Py_VISIT(self->winerror);
#endif
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyObject *
OSError_str(PyOSErrorObject *self)
{
#define OR_NONE(x) ((x)?(x):Py_None)
#ifdef MS_WINDOWS
    /* If available, winerror has the priority over myerrno */
    if (self->winerror && self->filename) {
        if (self->filename2) {
            return PyUnicode_FromFormat("[WinError %S] %S: %R -> %R",
                                        OR_NONE(self->winerror),
                                        OR_NONE(self->strerror),
                                        self->filename,
                                        self->filename2);
        } else {
            return PyUnicode_FromFormat("[WinError %S] %S: %R",
                                        OR_NONE(self->winerror),
                                        OR_NONE(self->strerror),
                                        self->filename);
        }
    }
    if (self->winerror && self->strerror)
        return PyUnicode_FromFormat("[WinError %S] %S",
                                    self->winerror ? self->winerror: Py_None,
                                    self->strerror ? self->strerror: Py_None);
#endif
    if (self->filename) {
        if (self->filename2) {
            return PyUnicode_FromFormat("[Errno %S] %S: %R -> %R",
                                        OR_NONE(self->myerrno),
                                        OR_NONE(self->strerror),
                                        self->filename,
                                        self->filename2);
        } else {
            return PyUnicode_FromFormat("[Errno %S] %S: %R",
                                        OR_NONE(self->myerrno),
                                        OR_NONE(self->strerror),
                                        self->filename);
        }
    }
    if (self->myerrno && self->strerror)
        return PyUnicode_FromFormat("[Errno %S] %S",
                                    self->myerrno, self->strerror);
    return BaseException_str((PyBaseExceptionObject *)self);
}

static PyObject *
OSError_reduce(PyOSErrorObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *args = self->args;
    PyObject *res = NULL;

    /* self->args is only the first two real arguments if there was a
     * file name given to OSError. */
    if (PyTuple_GET_SIZE(args) == 2 && self->filename) {
        Py_ssize_t size = self->filename2 ? 5 : 3;
        args = PyTuple_New(size);
        if (!args)
            return NULL;

        PyTuple_SET_ITEM(args, 0, Py_NewRef(PyTuple_GET_ITEM(self->args, 0)));
        PyTuple_SET_ITEM(args, 1, Py_NewRef(PyTuple_GET_ITEM(self->args, 1)));
        PyTuple_SET_ITEM(args, 2, Py_NewRef(self->filename));

        if (self->filename2) {
            /*
             * This tuple is essentially used as OSError(*args).
             * So, to recreate filename2, we need to pass in
             * winerror as well.
             */
            PyTuple_SET_ITEM(args, 3, Py_NewRef(Py_None));

            /* filename2 */
            PyTuple_SET_ITEM(args, 4, Py_NewRef(self->filename2));
        }
    } else
        Py_INCREF(args);

    if (self->dict)
        res = PyTuple_Pack(3, Py_TYPE(self), args, self->dict);
    else
        res = PyTuple_Pack(2, Py_TYPE(self), args);
    Py_DECREF(args);
    return res;
}

static PyObject *
OSError_written_get(PyOSErrorObject *self, void *context)
{
    if (self->written == -1) {
        PyErr_SetString(PyExc_AttributeError, "characters_written");
        return NULL;
    }
    return PyLong_FromSsize_t(self->written);
}

static int
OSError_written_set(PyOSErrorObject *self, PyObject *arg, void *context)
{
    if (arg == NULL) {
        if (self->written == -1) {
            PyErr_SetString(PyExc_AttributeError, "characters_written");
            return -1;
        }
        self->written = -1;
        return 0;
    }
    Py_ssize_t n;
    n = PyNumber_AsSsize_t(arg, PyExc_ValueError);
    if (n == -1 && PyErr_Occurred())
        return -1;
    self->written = n;
    return 0;
}

static PyMemberDef OSError_members[] = {
    {"errno", _Py_T_OBJECT, offsetof(PyOSErrorObject, myerrno), 0,
        PyDoc_STR("POSIX exception code")},
    {"strerror", _Py_T_OBJECT, offsetof(PyOSErrorObject, strerror), 0,
        PyDoc_STR("exception strerror")},
    {"filename", _Py_T_OBJECT, offsetof(PyOSErrorObject, filename), 0,
        PyDoc_STR("exception filename")},
    {"filename2", _Py_T_OBJECT, offsetof(PyOSErrorObject, filename2), 0,
        PyDoc_STR("second exception filename")},
#ifdef MS_WINDOWS
    {"winerror", _Py_T_OBJECT, offsetof(PyOSErrorObject, winerror), 0,
        PyDoc_STR("Win32 exception code")},
#endif
    {NULL}  /* Sentinel */
};

static PyMethodDef OSError_methods[] = {
    {"__reduce__", (PyCFunction)OSError_reduce, METH_NOARGS},
    {NULL}
};

static PyGetSetDef OSError_getset[] = {
    {"characters_written", (getter) OSError_written_get,
                           (setter) OSError_written_set, NULL},
    {NULL}
};


ComplexExtendsException(PyExc_Exception, OSError,
                        OSError, OSError_new,
                        OSError_methods, OSError_members, OSError_getset,
                        OSError_str,
                        "Base class for I/O related errors.");


/*
 *    Various OSError subclasses
 */
MiddlingExtendsException(PyExc_OSError, BlockingIOError, OSError,
                         "I/O operation would block.");
MiddlingExtendsException(PyExc_OSError, ConnectionError, OSError,
                         "Connection error.");
MiddlingExtendsException(PyExc_OSError, ChildProcessError, OSError,
                         "Child process error.");
MiddlingExtendsException(PyExc_ConnectionError, BrokenPipeError, OSError,
                         "Broken pipe.");
MiddlingExtendsException(PyExc_ConnectionError, ConnectionAbortedError, OSError,
                         "Connection aborted.");
MiddlingExtendsException(PyExc_ConnectionError, ConnectionRefusedError, OSError,
                         "Connection refused.");
MiddlingExtendsException(PyExc_ConnectionError, ConnectionResetError, OSError,
                         "Connection reset.");
MiddlingExtendsException(PyExc_OSError, FileExistsError, OSError,
                         "File already exists.");
MiddlingExtendsException(PyExc_OSError, FileNotFoundError, OSError,
                         "File not found.");
MiddlingExtendsException(PyExc_OSError, IsADirectoryError, OSError,
                         "Operation doesn't work on directories.");
MiddlingExtendsException(PyExc_OSError, NotADirectoryError, OSError,
                         "Operation only works on directories.");
MiddlingExtendsException(PyExc_OSError, InterruptedError, OSError,
                         "Interrupted by signal.");
MiddlingExtendsException(PyExc_OSError, PermissionError, OSError,
                         "Not enough permissions.");
MiddlingExtendsException(PyExc_OSError, ProcessLookupError, OSError,
                         "Process not found.");
MiddlingExtendsException(PyExc_OSError, TimeoutError, OSError,
                         "Timeout expired.");

/*
 *    EOFError extends Exception
 */
SimpleExtendsException(PyExc_Exception, EOFError,
                       "Read beyond end of file.");


/*
 *    RuntimeError extends Exception
 */
SimpleExtendsException(PyExc_Exception, RuntimeError,
                       "Unspecified run-time error.");

/*
 *    RecursionError extends RuntimeError
 */
SimpleExtendsException(PyExc_RuntimeError, RecursionError,
                       "Recursion limit exceeded.");

// PythonFinalizationError extends RuntimeError
SimpleExtendsException(PyExc_RuntimeError, PythonFinalizationError,
                       "Operation blocked during Python finalization.");

/*
 *    NotImplementedError extends RuntimeError
 */
SimpleExtendsException(PyExc_RuntimeError, NotImplementedError,
                       "Method or function hasn't been implemented yet.");

/*
 *    NameError extends Exception
 */

static int
NameError_init(PyNameErrorObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", NULL};
    PyObject *name = NULL;

    if (BaseException_init((PyBaseExceptionObject *)self, args, NULL) == -1) {
        return -1;
    }

    PyObject *empty_tuple = PyTuple_New(0);
    if (!empty_tuple) {
        return -1;
    }
    if (!PyArg_ParseTupleAndKeywords(empty_tuple, kwds, "|$O:NameError", kwlist,
                                     &name)) {
        Py_DECREF(empty_tuple);
        return -1;
    }
    Py_DECREF(empty_tuple);

    Py_XSETREF(self->name, Py_XNewRef(name));

    return 0;
}

static int
NameError_clear(PyNameErrorObject *self)
{
    Py_CLEAR(self->name);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
NameError_dealloc(PyNameErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    NameError_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
NameError_traverse(PyNameErrorObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->name);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyMemberDef NameError_members[] = {
        {"name", _Py_T_OBJECT, offsetof(PyNameErrorObject, name), 0, PyDoc_STR("name")},
        {NULL}  /* Sentinel */
};

static PyMethodDef NameError_methods[] = {
        {NULL}  /* Sentinel */
};

ComplexExtendsException(PyExc_Exception, NameError,
                        NameError, 0,
                        NameError_methods, NameError_members,
                        0, BaseException_str, "Name not found globally.");

/*
 *    UnboundLocalError extends NameError
 */

MiddlingExtendsException(PyExc_NameError, UnboundLocalError, NameError,
                       "Local name referenced but not bound to a value.");

/*
 *    AttributeError extends Exception
 */

static int
AttributeError_init(PyAttributeErrorObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", "obj", NULL};
    PyObject *name = NULL;
    PyObject *obj = NULL;

    if (BaseException_init((PyBaseExceptionObject *)self, args, NULL) == -1) {
        return -1;
    }

    PyObject *empty_tuple = PyTuple_New(0);
    if (!empty_tuple) {
        return -1;
    }
    if (!PyArg_ParseTupleAndKeywords(empty_tuple, kwds, "|$OO:AttributeError", kwlist,
                                     &name, &obj)) {
        Py_DECREF(empty_tuple);
        return -1;
    }
    Py_DECREF(empty_tuple);

    Py_XSETREF(self->name, Py_XNewRef(name));
    Py_XSETREF(self->obj, Py_XNewRef(obj));

    return 0;
}

static int
AttributeError_clear(PyAttributeErrorObject *self)
{
    Py_CLEAR(self->obj);
    Py_CLEAR(self->name);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
AttributeError_dealloc(PyAttributeErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    AttributeError_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
AttributeError_traverse(PyAttributeErrorObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->obj);
    Py_VISIT(self->name);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

/* Pickling support */
static PyObject *
AttributeError_getstate(PyAttributeErrorObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *dict = ((PyAttributeErrorObject *)self)->dict;
    if (self->name || self->args) {
        dict = dict ? PyDict_Copy(dict) : PyDict_New();
        if (dict == NULL) {
            return NULL;
        }
        if (self->name && PyDict_SetItemString(dict, "name", self->name) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        /* We specifically are not pickling the obj attribute since there are many
        cases where it is unlikely to be picklable. See GH-103352.
        */
        if (self->args && PyDict_SetItemString(dict, "args", self->args) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        return dict;
    }
    else if (dict) {
        return Py_NewRef(dict);
    }
    Py_RETURN_NONE;
}

static PyObject *
AttributeError_reduce(PyAttributeErrorObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *state = AttributeError_getstate(self, NULL);
    if (state == NULL) {
        return NULL;
    }

    PyObject *return_value = PyTuple_Pack(3, Py_TYPE(self), self->args, state);
    Py_DECREF(state);
    return return_value;
}

static PyMemberDef AttributeError_members[] = {
    {"name", _Py_T_OBJECT, offsetof(PyAttributeErrorObject, name), 0, PyDoc_STR("attribute name")},
    {"obj", _Py_T_OBJECT, offsetof(PyAttributeErrorObject, obj), 0, PyDoc_STR("object")},
    {NULL}  /* Sentinel */
};

static PyMethodDef AttributeError_methods[] = {
    {"__getstate__", (PyCFunction)AttributeError_getstate, METH_NOARGS},
    {"__reduce__", (PyCFunction)AttributeError_reduce, METH_NOARGS },
    {NULL}
};

ComplexExtendsException(PyExc_Exception, AttributeError,
                        AttributeError, 0,
                        AttributeError_methods, AttributeError_members,
                        0, BaseException_str, "Attribute not found.");

/*
 *    SyntaxError extends Exception
 */

static int
SyntaxError_init(PySyntaxErrorObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *info = NULL;
    Py_ssize_t lenargs = PyTuple_GET_SIZE(args);

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    if (lenargs >= 1) {
        Py_XSETREF(self->msg, Py_NewRef(PyTuple_GET_ITEM(args, 0)));
    }
    if (lenargs == 2) {
        info = PyTuple_GET_ITEM(args, 1);
        info = PySequence_Tuple(info);
        if (!info) {
            return -1;
        }

        self->end_lineno = NULL;
        self->end_offset = NULL;
        if (!PyArg_ParseTuple(info, "OOOO|OO",
                              &self->filename, &self->lineno,
                              &self->offset, &self->text,
                              &self->end_lineno, &self->end_offset)) {
            Py_DECREF(info);
            return -1;
        }

        Py_INCREF(self->filename);
        Py_INCREF(self->lineno);
        Py_INCREF(self->offset);
        Py_INCREF(self->text);
        Py_XINCREF(self->end_lineno);
        Py_XINCREF(self->end_offset);
        Py_DECREF(info);

        if (self->end_lineno != NULL && self->end_offset == NULL) {
            PyErr_SetString(PyExc_TypeError, "end_offset must be provided when end_lineno is provided");
            return -1;
        }
    }
    return 0;
}

static int
SyntaxError_clear(PySyntaxErrorObject *self)
{
    Py_CLEAR(self->msg);
    Py_CLEAR(self->filename);
    Py_CLEAR(self->lineno);
    Py_CLEAR(self->offset);
    Py_CLEAR(self->end_lineno);
    Py_CLEAR(self->end_offset);
    Py_CLEAR(self->text);
    Py_CLEAR(self->print_file_and_line);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
SyntaxError_dealloc(PySyntaxErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    SyntaxError_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
SyntaxError_traverse(PySyntaxErrorObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->msg);
    Py_VISIT(self->filename);
    Py_VISIT(self->lineno);
    Py_VISIT(self->offset);
    Py_VISIT(self->end_lineno);
    Py_VISIT(self->end_offset);
    Py_VISIT(self->text);
    Py_VISIT(self->print_file_and_line);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

/* This is called "my_basename" instead of just "basename" to avoid name
   conflicts with glibc; basename is already prototyped if _GNU_SOURCE is
   defined, and Python does define that. */
static PyObject*
my_basename(PyObject *name)
{
    Py_ssize_t i, size, offset;
    int kind;
    const void *data;

    kind = PyUnicode_KIND(name);
    data = PyUnicode_DATA(name);
    size = PyUnicode_GET_LENGTH(name);
    offset = 0;
    for(i=0; i < size; i++) {
        if (PyUnicode_READ(kind, data, i) == SEP) {
            offset = i + 1;
        }
    }
    if (offset != 0) {
        return PyUnicode_Substring(name, offset, size);
    }
    else {
        return Py_NewRef(name);
    }
}


static PyObject *
SyntaxError_str(PySyntaxErrorObject *self)
{
    int have_lineno = 0;
    PyObject *filename;
    PyObject *result;
    /* Below, we always ignore overflow errors, just printing -1.
       Still, we cannot allow an OverflowError to be raised, so
       we need to call PyLong_AsLongAndOverflow. */
    int overflow;

    /* XXX -- do all the additional formatting with filename and
       lineno here */

    if (self->filename && PyUnicode_Check(self->filename)) {
        filename = my_basename(self->filename);
        if (filename == NULL)
            return NULL;
    } else {
        filename = NULL;
    }
    have_lineno = (self->lineno != NULL) && PyLong_CheckExact(self->lineno);

    if (!filename && !have_lineno)
        return PyObject_Str(self->msg ? self->msg : Py_None);

    if (filename && have_lineno)
        result = PyUnicode_FromFormat("%S (%U, line %ld)",
                   self->msg ? self->msg : Py_None,
                   filename,
                   PyLong_AsLongAndOverflow(self->lineno, &overflow));
    else if (filename)
        result = PyUnicode_FromFormat("%S (%U)",
                   self->msg ? self->msg : Py_None,
                   filename);
    else /* only have_lineno */
        result = PyUnicode_FromFormat("%S (line %ld)",
                   self->msg ? self->msg : Py_None,
                   PyLong_AsLongAndOverflow(self->lineno, &overflow));
    Py_XDECREF(filename);
    return result;
}

static PyMemberDef SyntaxError_members[] = {
    {"msg", _Py_T_OBJECT, offsetof(PySyntaxErrorObject, msg), 0,
        PyDoc_STR("exception msg")},
    {"filename", _Py_T_OBJECT, offsetof(PySyntaxErrorObject, filename), 0,
        PyDoc_STR("exception filename")},
    {"lineno", _Py_T_OBJECT, offsetof(PySyntaxErrorObject, lineno), 0,
        PyDoc_STR("exception lineno")},
    {"offset", _Py_T_OBJECT, offsetof(PySyntaxErrorObject, offset), 0,
        PyDoc_STR("exception offset")},
    {"text", _Py_T_OBJECT, offsetof(PySyntaxErrorObject, text), 0,
        PyDoc_STR("exception text")},
    {"end_lineno", _Py_T_OBJECT, offsetof(PySyntaxErrorObject, end_lineno), 0,
                   PyDoc_STR("exception end lineno")},
    {"end_offset", _Py_T_OBJECT, offsetof(PySyntaxErrorObject, end_offset), 0,
                   PyDoc_STR("exception end offset")},
    {"print_file_and_line", _Py_T_OBJECT,
        offsetof(PySyntaxErrorObject, print_file_and_line), 0,
        PyDoc_STR("exception print_file_and_line")},
    {NULL}  /* Sentinel */
};

ComplexExtendsException(PyExc_Exception, SyntaxError, SyntaxError,
                        0, 0, SyntaxError_members, 0,
                        SyntaxError_str, "Invalid syntax.");


/*
 *    IndentationError extends SyntaxError
 */
MiddlingExtendsException(PyExc_SyntaxError, IndentationError, SyntaxError,
                         "Improper indentation.");


/*
 *    TabError extends IndentationError
 */
MiddlingExtendsException(PyExc_IndentationError, TabError, SyntaxError,
                         "Improper mixture of spaces and tabs.");

/*
 *    IncompleteInputError extends SyntaxError
 */
MiddlingExtendsExceptionEx(PyExc_SyntaxError, IncompleteInputError, _IncompleteInputError,
                           SyntaxError, "incomplete input.");

/*
 *    LookupError extends Exception
 */
SimpleExtendsException(PyExc_Exception, LookupError,
                       "Base class for lookup errors.");


/*
 *    IndexError extends LookupError
 */
SimpleExtendsException(PyExc_LookupError, IndexError,
                       "Sequence index out of range.");


/*
 *    KeyError extends LookupError
 */
static PyObject *
KeyError_str(PyBaseExceptionObject *self)
{
    /* If args is a tuple of exactly one item, apply repr to args[0].
       This is done so that e.g. the exception raised by {}[''] prints
         KeyError: ''
       rather than the confusing
         KeyError
       alone.  The downside is that if KeyError is raised with an explanatory
       string, that string will be displayed in quotes.  Too bad.
       If args is anything else, use the default BaseException__str__().
    */
    if (PyTuple_GET_SIZE(self->args) == 1) {
        return PyObject_Repr(PyTuple_GET_ITEM(self->args, 0));
    }
    return BaseException_str(self);
}

ComplexExtendsException(PyExc_LookupError, KeyError, BaseException,
                        0, 0, 0, 0, KeyError_str, "Mapping key not found.");


/*
 *    ValueError extends Exception
 */
SimpleExtendsException(PyExc_Exception, ValueError,
                       "Inappropriate argument value (of correct type).");

/*
 *    UnicodeError extends ValueError
 */

SimpleExtendsException(PyExc_ValueError, UnicodeError,
                       "Unicode related error.");

static PyObject *
get_string(PyObject *attr, const char *name)
{
    if (!attr) {
        PyErr_Format(PyExc_TypeError, "%.200s attribute not set", name);
        return NULL;
    }

    if (!PyBytes_Check(attr)) {
        PyErr_Format(PyExc_TypeError, "%.200s attribute must be bytes", name);
        return NULL;
    }
    return Py_NewRef(attr);
}

static PyObject *
get_unicode(PyObject *attr, const char *name)
{
    if (!attr) {
        PyErr_Format(PyExc_TypeError, "%.200s attribute not set", name);
        return NULL;
    }

    if (!PyUnicode_Check(attr)) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s attribute must be unicode", name);
        return NULL;
    }
    return Py_NewRef(attr);
}

static int
set_unicodefromstring(PyObject **attr, const char *value)
{
    PyObject *obj = PyUnicode_FromString(value);
    if (!obj)
        return -1;
    Py_XSETREF(*attr, obj);
    return 0;
}

PyObject *
PyUnicodeEncodeError_GetEncoding(PyObject *exc)
{
    return get_unicode(((PyUnicodeErrorObject *)exc)->encoding, "encoding");
}

PyObject *
PyUnicodeDecodeError_GetEncoding(PyObject *exc)
{
    return get_unicode(((PyUnicodeErrorObject *)exc)->encoding, "encoding");
}

PyObject *
PyUnicodeEncodeError_GetObject(PyObject *exc)
{
    return get_unicode(((PyUnicodeErrorObject *)exc)->object, "object");
}

PyObject *
PyUnicodeDecodeError_GetObject(PyObject *exc)
{
    return get_string(((PyUnicodeErrorObject *)exc)->object, "object");
}

PyObject *
PyUnicodeTranslateError_GetObject(PyObject *exc)
{
    return get_unicode(((PyUnicodeErrorObject *)exc)->object, "object");
}

int
PyUnicodeEncodeError_GetStart(PyObject *exc, Py_ssize_t *start)
{
    Py_ssize_t size;
    PyObject *obj = get_unicode(((PyUnicodeErrorObject *)exc)->object,
                                "object");
    if (!obj)
        return -1;
    *start = ((PyUnicodeErrorObject *)exc)->start;
    size = PyUnicode_GET_LENGTH(obj);
    if (*start<0)
        *start = 0; /*XXX check for values <0*/
    if (*start>=size)
        *start = size-1;
    Py_DECREF(obj);
    return 0;
}


int
PyUnicodeDecodeError_GetStart(PyObject *exc, Py_ssize_t *start)
{
    Py_ssize_t size;
    PyObject *obj = get_string(((PyUnicodeErrorObject *)exc)->object, "object");
    if (!obj)
        return -1;
    size = PyBytes_GET_SIZE(obj);
    *start = ((PyUnicodeErrorObject *)exc)->start;
    if (*start<0)
        *start = 0;
    if (*start>=size)
        *start = size-1;
    Py_DECREF(obj);
    return 0;
}


int
PyUnicodeTranslateError_GetStart(PyObject *exc, Py_ssize_t *start)
{
    return PyUnicodeEncodeError_GetStart(exc, start);
}


int
PyUnicodeEncodeError_SetStart(PyObject *exc, Py_ssize_t start)
{
    ((PyUnicodeErrorObject *)exc)->start = start;
    return 0;
}


int
PyUnicodeDecodeError_SetStart(PyObject *exc, Py_ssize_t start)
{
    ((PyUnicodeErrorObject *)exc)->start = start;
    return 0;
}


int
PyUnicodeTranslateError_SetStart(PyObject *exc, Py_ssize_t start)
{
    ((PyUnicodeErrorObject *)exc)->start = start;
    return 0;
}


int
PyUnicodeEncodeError_GetEnd(PyObject *exc, Py_ssize_t *end)
{
    Py_ssize_t size;
    PyObject *obj = get_unicode(((PyUnicodeErrorObject *)exc)->object,
                                "object");
    if (!obj)
        return -1;
    *end = ((PyUnicodeErrorObject *)exc)->end;
    size = PyUnicode_GET_LENGTH(obj);
    if (*end<1)
        *end = 1;
    if (*end>size)
        *end = size;
    Py_DECREF(obj);
    return 0;
}


int
PyUnicodeDecodeError_GetEnd(PyObject *exc, Py_ssize_t *end)
{
    Py_ssize_t size;
    PyObject *obj = get_string(((PyUnicodeErrorObject *)exc)->object, "object");
    if (!obj)
        return -1;
    size = PyBytes_GET_SIZE(obj);
    *end = ((PyUnicodeErrorObject *)exc)->end;
    if (*end<1)
        *end = 1;
    if (*end>size)
        *end = size;
    Py_DECREF(obj);
    return 0;
}


int
PyUnicodeTranslateError_GetEnd(PyObject *exc, Py_ssize_t *end)
{
    return PyUnicodeEncodeError_GetEnd(exc, end);
}


int
PyUnicodeEncodeError_SetEnd(PyObject *exc, Py_ssize_t end)
{
    ((PyUnicodeErrorObject *)exc)->end = end;
    return 0;
}


int
PyUnicodeDecodeError_SetEnd(PyObject *exc, Py_ssize_t end)
{
    ((PyUnicodeErrorObject *)exc)->end = end;
    return 0;
}


int
PyUnicodeTranslateError_SetEnd(PyObject *exc, Py_ssize_t end)
{
    ((PyUnicodeErrorObject *)exc)->end = end;
    return 0;
}

PyObject *
PyUnicodeEncodeError_GetReason(PyObject *exc)
{
    return get_unicode(((PyUnicodeErrorObject *)exc)->reason, "reason");
}


PyObject *
PyUnicodeDecodeError_GetReason(PyObject *exc)
{
    return get_unicode(((PyUnicodeErrorObject *)exc)->reason, "reason");
}


PyObject *
PyUnicodeTranslateError_GetReason(PyObject *exc)
{
    return get_unicode(((PyUnicodeErrorObject *)exc)->reason, "reason");
}


int
PyUnicodeEncodeError_SetReason(PyObject *exc, const char *reason)
{
    return set_unicodefromstring(&((PyUnicodeErrorObject *)exc)->reason,
                                 reason);
}


int
PyUnicodeDecodeError_SetReason(PyObject *exc, const char *reason)
{
    return set_unicodefromstring(&((PyUnicodeErrorObject *)exc)->reason,
                                 reason);
}


int
PyUnicodeTranslateError_SetReason(PyObject *exc, const char *reason)
{
    return set_unicodefromstring(&((PyUnicodeErrorObject *)exc)->reason,
                                 reason);
}


static int
UnicodeError_clear(PyUnicodeErrorObject *self)
{
    Py_CLEAR(self->encoding);
    Py_CLEAR(self->object);
    Py_CLEAR(self->reason);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
UnicodeError_dealloc(PyUnicodeErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    UnicodeError_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
UnicodeError_traverse(PyUnicodeErrorObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->encoding);
    Py_VISIT(self->object);
    Py_VISIT(self->reason);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyMemberDef UnicodeError_members[] = {
    {"encoding", _Py_T_OBJECT, offsetof(PyUnicodeErrorObject, encoding), 0,
        PyDoc_STR("exception encoding")},
    {"object", _Py_T_OBJECT, offsetof(PyUnicodeErrorObject, object), 0,
        PyDoc_STR("exception object")},
    {"start", Py_T_PYSSIZET, offsetof(PyUnicodeErrorObject, start), 0,
        PyDoc_STR("exception start")},
    {"end", Py_T_PYSSIZET, offsetof(PyUnicodeErrorObject, end), 0,
        PyDoc_STR("exception end")},
    {"reason", _Py_T_OBJECT, offsetof(PyUnicodeErrorObject, reason), 0,
        PyDoc_STR("exception reason")},
    {NULL}  /* Sentinel */
};


/*
 *    UnicodeEncodeError extends UnicodeError
 */

static int
UnicodeEncodeError_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyUnicodeErrorObject *err;

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    err = (PyUnicodeErrorObject *)self;

    Py_CLEAR(err->encoding);
    Py_CLEAR(err->object);
    Py_CLEAR(err->reason);

    if (!PyArg_ParseTuple(args, "UUnnU",
                          &err->encoding, &err->object,
                          &err->start, &err->end, &err->reason)) {
        err->encoding = err->object = err->reason = NULL;
        return -1;
    }

    Py_INCREF(err->encoding);
    Py_INCREF(err->object);
    Py_INCREF(err->reason);

    return 0;
}

static PyObject *
UnicodeEncodeError_str(PyObject *self)
{
    PyUnicodeErrorObject *exc = (PyUnicodeErrorObject *)self;
    PyObject *result = NULL;
    PyObject *reason_str = NULL;
    PyObject *encoding_str = NULL;

    if (exc->object == NULL) {
        /* Not properly initialized. */
        return PyUnicode_FromString("");
    }

    /* Get reason and encoding as strings, which they might not be if
       they've been modified after we were constructed. */
    reason_str = PyObject_Str(exc->reason);
    if (reason_str == NULL) {
        goto done;
    }
    encoding_str = PyObject_Str(exc->encoding);
    if (encoding_str == NULL) {
        goto done;
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(exc->object);
    Py_ssize_t start = exc->start, end = exc->end;

    if ((start >= 0 && start < len) && (end >= 0 && end <= len) && end == start + 1) {
        Py_UCS4 badchar = PyUnicode_ReadChar(exc->object, start);
        const char *fmt;
        if (badchar <= 0xff) {
            fmt = "'%U' codec can't encode character '\\x%02x' in position %zd: %U";
        }
        else if (badchar <= 0xffff) {
            fmt = "'%U' codec can't encode character '\\u%04x' in position %zd: %U";
        }
        else {
            fmt = "'%U' codec can't encode character '\\U%08x' in position %zd: %U";
        }
        result = PyUnicode_FromFormat(
            fmt,
            encoding_str,
            (int)badchar,
            start,
            reason_str);
    }
    else {
        result = PyUnicode_FromFormat(
            "'%U' codec can't encode characters in position %zd-%zd: %U",
            encoding_str,
            start,
            end - 1,
            reason_str);
    }
done:
    Py_XDECREF(reason_str);
    Py_XDECREF(encoding_str);
    return result;
}

static PyTypeObject _PyExc_UnicodeEncodeError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "UnicodeEncodeError",
    sizeof(PyUnicodeErrorObject), 0,
    (destructor)UnicodeError_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    (reprfunc)UnicodeEncodeError_str, 0, 0, 0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    PyDoc_STR("Unicode encoding error."), (traverseproc)UnicodeError_traverse,
    (inquiry)UnicodeError_clear, 0, 0, 0, 0, 0, UnicodeError_members,
    0, &_PyExc_UnicodeError, 0, 0, 0, offsetof(PyUnicodeErrorObject, dict),
    (initproc)UnicodeEncodeError_init, 0, BaseException_new,
};
PyObject *PyExc_UnicodeEncodeError = (PyObject *)&_PyExc_UnicodeEncodeError;


/*
 *    UnicodeDecodeError extends UnicodeError
 */

static int
UnicodeDecodeError_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyUnicodeErrorObject *ude;

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    ude = (PyUnicodeErrorObject *)self;

    Py_CLEAR(ude->encoding);
    Py_CLEAR(ude->object);
    Py_CLEAR(ude->reason);

    if (!PyArg_ParseTuple(args, "UOnnU",
                          &ude->encoding, &ude->object,
                          &ude->start, &ude->end, &ude->reason)) {
             ude->encoding = ude->object = ude->reason = NULL;
             return -1;
    }

    Py_INCREF(ude->encoding);
    Py_INCREF(ude->object);
    Py_INCREF(ude->reason);

    if (!PyBytes_Check(ude->object)) {
        Py_buffer view;
        if (PyObject_GetBuffer(ude->object, &view, PyBUF_SIMPLE) != 0)
            goto error;
        Py_XSETREF(ude->object, PyBytes_FromStringAndSize(view.buf, view.len));
        PyBuffer_Release(&view);
        if (!ude->object)
            goto error;
    }
    return 0;

error:
    Py_CLEAR(ude->encoding);
    Py_CLEAR(ude->object);
    Py_CLEAR(ude->reason);
    return -1;
}

static PyObject *
UnicodeDecodeError_str(PyObject *self)
{
    PyUnicodeErrorObject *exc = (PyUnicodeErrorObject *)self;
    PyObject *result = NULL;
    PyObject *reason_str = NULL;
    PyObject *encoding_str = NULL;

    if (exc->object == NULL) {
        /* Not properly initialized. */
        return PyUnicode_FromString("");
    }

    /* Get reason and encoding as strings, which they might not be if
       they've been modified after we were constructed. */
    reason_str = PyObject_Str(exc->reason);
    if (reason_str == NULL) {
        goto done;
    }
    encoding_str = PyObject_Str(exc->encoding);
    if (encoding_str == NULL) {
        goto done;
    }

    Py_ssize_t len = PyBytes_GET_SIZE(exc->object);
    Py_ssize_t start = exc->start, end = exc->end;

    if ((start >= 0 && start < len) && (end >= 0 && end <= len) && end == start + 1) {
        int badbyte = (int)(PyBytes_AS_STRING(exc->object)[start] & 0xff);
        result = PyUnicode_FromFormat(
            "'%U' codec can't decode byte 0x%02x in position %zd: %U",
            encoding_str,
            badbyte,
            start,
            reason_str);
    }
    else {
        result = PyUnicode_FromFormat(
            "'%U' codec can't decode bytes in position %zd-%zd: %U",
            encoding_str,
            start,
            end - 1,
            reason_str);
    }
done:
    Py_XDECREF(reason_str);
    Py_XDECREF(encoding_str);
    return result;
}

static PyTypeObject _PyExc_UnicodeDecodeError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "UnicodeDecodeError",
    sizeof(PyUnicodeErrorObject), 0,
    (destructor)UnicodeError_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    (reprfunc)UnicodeDecodeError_str, 0, 0, 0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    PyDoc_STR("Unicode decoding error."), (traverseproc)UnicodeError_traverse,
    (inquiry)UnicodeError_clear, 0, 0, 0, 0, 0, UnicodeError_members,
    0, &_PyExc_UnicodeError, 0, 0, 0, offsetof(PyUnicodeErrorObject, dict),
    (initproc)UnicodeDecodeError_init, 0, BaseException_new,
};
PyObject *PyExc_UnicodeDecodeError = (PyObject *)&_PyExc_UnicodeDecodeError;

PyObject *
PyUnicodeDecodeError_Create(
    const char *encoding, const char *object, Py_ssize_t length,
    Py_ssize_t start, Py_ssize_t end, const char *reason)
{
    return PyObject_CallFunction(PyExc_UnicodeDecodeError, "sy#nns",
                                 encoding, object, length, start, end, reason);
}


/*
 *    UnicodeTranslateError extends UnicodeError
 */

static int
UnicodeTranslateError_init(PyUnicodeErrorObject *self, PyObject *args,
                           PyObject *kwds)
{
    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    Py_CLEAR(self->object);
    Py_CLEAR(self->reason);

    if (!PyArg_ParseTuple(args, "UnnU",
                          &self->object,
                          &self->start, &self->end, &self->reason)) {
        self->object = self->reason = NULL;
        return -1;
    }

    Py_INCREF(self->object);
    Py_INCREF(self->reason);

    return 0;
}


static PyObject *
UnicodeTranslateError_str(PyObject *self)
{
    PyUnicodeErrorObject *exc = (PyUnicodeErrorObject *)self;
    PyObject *result = NULL;
    PyObject *reason_str = NULL;

    if (exc->object == NULL) {
        /* Not properly initialized. */
        return PyUnicode_FromString("");
    }

    /* Get reason as a string, which it might not be if it's been
       modified after we were constructed. */
    reason_str = PyObject_Str(exc->reason);
    if (reason_str == NULL) {
        goto done;
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(exc->object);
    Py_ssize_t start = exc->start, end = exc->end;

    if ((start >= 0 && start < len) && (end >= 0 && end <= len) && end == start + 1) {
        Py_UCS4 badchar = PyUnicode_ReadChar(exc->object, start);
        const char *fmt;
        if (badchar <= 0xff) {
            fmt = "can't translate character '\\x%02x' in position %zd: %U";
        }
        else if (badchar <= 0xffff) {
            fmt = "can't translate character '\\u%04x' in position %zd: %U";
        }
        else {
            fmt = "can't translate character '\\U%08x' in position %zd: %U";
        }
        result = PyUnicode_FromFormat(
            fmt,
            (int)badchar,
            start,
            reason_str);
    }
    else {
        result = PyUnicode_FromFormat(
            "can't translate characters in position %zd-%zd: %U",
            start,
            end - 1,
            reason_str);
    }
done:
    Py_XDECREF(reason_str);
    return result;
}

static PyTypeObject _PyExc_UnicodeTranslateError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "UnicodeTranslateError",
    sizeof(PyUnicodeErrorObject), 0,
    (destructor)UnicodeError_dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    (reprfunc)UnicodeTranslateError_str, 0, 0, 0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    PyDoc_STR("Unicode translation error."), (traverseproc)UnicodeError_traverse,
    (inquiry)UnicodeError_clear, 0, 0, 0, 0, 0, UnicodeError_members,
    0, &_PyExc_UnicodeError, 0, 0, 0, offsetof(PyUnicodeErrorObject, dict),
    (initproc)UnicodeTranslateError_init, 0, BaseException_new,
};
PyObject *PyExc_UnicodeTranslateError = (PyObject *)&_PyExc_UnicodeTranslateError;

PyObject *
_PyUnicodeTranslateError_Create(
    PyObject *object,
    Py_ssize_t start, Py_ssize_t end, const char *reason)
{
    return PyObject_CallFunction(PyExc_UnicodeTranslateError, "Onns",
                                 object, start, end, reason);
}

/*
 *    AssertionError extends Exception
 */
SimpleExtendsException(PyExc_Exception, AssertionError,
                       "Assertion failed.");


/*
 *    ArithmeticError extends Exception
 */
SimpleExtendsException(PyExc_Exception, ArithmeticError,
                       "Base class for arithmetic errors.");


/*
 *    FloatingPointError extends ArithmeticError
 */
SimpleExtendsException(PyExc_ArithmeticError, FloatingPointError,
                       "Floating-point operation failed.");


/*
 *    OverflowError extends ArithmeticError
 */
SimpleExtendsException(PyExc_ArithmeticError, OverflowError,
                       "Result too large to be represented.");


/*
 *    ZeroDivisionError extends ArithmeticError
 */
SimpleExtendsException(PyExc_ArithmeticError, ZeroDivisionError,
          "Second argument to a division or modulo operation was zero.");


/*
 *    SystemError extends Exception
 */
SimpleExtendsException(PyExc_Exception, SystemError,
    "Internal error in the Python interpreter.\n"
    "\n"
    "Please report this to the Python maintainer, along with the traceback,\n"
    "the Python version, and the hardware/OS platform and version.");


/*
 *    ReferenceError extends Exception
 */
SimpleExtendsException(PyExc_Exception, ReferenceError,
                       "Weak ref proxy used after referent went away.");


/*
 *    MemoryError extends Exception
 */

#define MEMERRORS_SAVE 16

#ifdef Py_GIL_DISABLED
# define MEMERRORS_LOCK(state) PyMutex_LockFlags(&state->memerrors_lock, _Py_LOCK_DONT_DETACH)
# define MEMERRORS_UNLOCK(state) PyMutex_Unlock(&state->memerrors_lock)
#else
# define MEMERRORS_LOCK(state) ((void)0)
# define MEMERRORS_UNLOCK(state) ((void)0)
#endif

static PyObject *
get_memory_error(int allow_allocation, PyObject *args, PyObject *kwds)
{
    PyBaseExceptionObject *self = NULL;
    struct _Py_exc_state *state = get_exc_state();

    MEMERRORS_LOCK(state);
    if (state->memerrors_freelist != NULL) {
        /* Fetch MemoryError from freelist and initialize it */
        self = state->memerrors_freelist;
        state->memerrors_freelist = (PyBaseExceptionObject *) self->dict;
        state->memerrors_numfree--;
        self->dict = NULL;
        self->args = (PyObject *)&_Py_SINGLETON(tuple_empty);
        _Py_NewReference((PyObject *)self);
        _PyObject_GC_TRACK(self);
    }
    MEMERRORS_UNLOCK(state);

    if (self != NULL) {
        return (PyObject *)self;
    }

    if (!allow_allocation) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        return Py_NewRef(
            &_Py_INTERP_SINGLETON(interp, last_resort_memory_error));
    }
    return BaseException_new((PyTypeObject *)PyExc_MemoryError, args, kwds);
}

static PyObject *
MemoryError_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    /* If this is a subclass of MemoryError, don't use the freelist
     * and just return a fresh object */
    if (type != (PyTypeObject *) PyExc_MemoryError) {
        return BaseException_new(type, args, kwds);
    }
    return get_memory_error(1, args, kwds);
}

PyObject *
_PyErr_NoMemory(PyThreadState *tstate)
{
    if (Py_IS_TYPE(PyExc_MemoryError, NULL)) {
        /* PyErr_NoMemory() has been called before PyExc_MemoryError has been
           initialized by _PyExc_Init() */
        Py_FatalError("Out of memory and PyExc_MemoryError is not "
                      "initialized yet");
    }
    PyObject *err = get_memory_error(0, NULL, NULL);
    if (err != NULL) {
        _PyErr_SetRaisedException(tstate, err);
    }
    return NULL;
}

static void
MemoryError_dealloc(PyBaseExceptionObject *self)
{
    _PyObject_GC_UNTRACK(self);

    BaseException_clear(self);

    /* If this is a subclass of MemoryError, we don't need to
     * do anything in the free-list*/
    if (!Py_IS_TYPE(self, (PyTypeObject *) PyExc_MemoryError)) {
        Py_TYPE(self)->tp_free((PyObject *)self);
        return;
    }

    struct _Py_exc_state *state = get_exc_state();
    MEMERRORS_LOCK(state);
    if (state->memerrors_numfree < MEMERRORS_SAVE) {
        self->dict = (PyObject *) state->memerrors_freelist;
        state->memerrors_freelist = self;
        state->memerrors_numfree++;
        MEMERRORS_UNLOCK(state);
        return;
    }
    MEMERRORS_UNLOCK(state);

    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
preallocate_memerrors(void)
{
    /* We create enough MemoryErrors and then decref them, which will fill
       up the freelist. */
    int i;

    PyObject *errors[MEMERRORS_SAVE];
    for (i = 0; i < MEMERRORS_SAVE; i++) {
        errors[i] = MemoryError_new((PyTypeObject *) PyExc_MemoryError,
                                    NULL, NULL);
        if (!errors[i]) {
            return -1;
        }
    }
    for (i = 0; i < MEMERRORS_SAVE; i++) {
        Py_DECREF(errors[i]);
    }
    return 0;
}

static void
free_preallocated_memerrors(struct _Py_exc_state *state)
{
    while (state->memerrors_freelist != NULL) {
        PyObject *self = (PyObject *) state->memerrors_freelist;
        state->memerrors_freelist = (PyBaseExceptionObject *)state->memerrors_freelist->dict;
        Py_TYPE(self)->tp_free((PyObject *)self);
    }
}


PyTypeObject _PyExc_MemoryError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MemoryError",
    sizeof(PyBaseExceptionObject),
    0, (destructor)MemoryError_dealloc, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    PyDoc_STR("Out of memory."), (traverseproc)BaseException_traverse,
    (inquiry)BaseException_clear, 0, 0, 0, 0, 0, 0, 0, &_PyExc_Exception,
    0, 0, 0, offsetof(PyBaseExceptionObject, dict),
    (initproc)BaseException_init, 0, MemoryError_new
};
PyObject *PyExc_MemoryError = (PyObject *) &_PyExc_MemoryError;


/*
 *    BufferError extends Exception
 */
SimpleExtendsException(PyExc_Exception, BufferError, "Buffer error.");


/* Warning category docstrings */

/*
 *    Warning extends Exception
 */
SimpleExtendsException(PyExc_Exception, Warning,
                       "Base class for warning categories.");


/*
 *    UserWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, UserWarning,
                       "Base class for warnings generated by user code.");


/*
 *    DeprecationWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, DeprecationWarning,
                       "Base class for warnings about deprecated features.");


/*
 *    PendingDeprecationWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, PendingDeprecationWarning,
    "Base class for warnings about features which will be deprecated\n"
    "in the future.");


/*
 *    SyntaxWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, SyntaxWarning,
                       "Base class for warnings about dubious syntax.");


/*
 *    RuntimeWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, RuntimeWarning,
                 "Base class for warnings about dubious runtime behavior.");


/*
 *    FutureWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, FutureWarning,
    "Base class for warnings about constructs that will change semantically\n"
    "in the future.");


/*
 *    ImportWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, ImportWarning,
          "Base class for warnings about probable mistakes in module imports");


/*
 *    UnicodeWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, UnicodeWarning,
    "Base class for warnings about Unicode related problems, mostly\n"
    "related to conversion problems.");


/*
 *    BytesWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, BytesWarning,
    "Base class for warnings about bytes and buffer related problems, mostly\n"
    "related to conversion from str or comparing to str.");


/*
 *    EncodingWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, EncodingWarning,
    "Base class for warnings about encodings.");


/*
 *    ResourceWarning extends Warning
 */
SimpleExtendsException(PyExc_Warning, ResourceWarning,
    "Base class for warnings about resource usage.");



#ifdef MS_WINDOWS
#include <winsock2.h>
/* The following constants were added to errno.h in VS2010 but have
   preferred WSA equivalents. */
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef EAFNOSUPPORT
#undef EALREADY
#undef ECONNABORTED
#undef ECONNREFUSED
#undef ECONNRESET
#undef EDESTADDRREQ
#undef EHOSTUNREACH
#undef EINPROGRESS
#undef EISCONN
#undef ELOOP
#undef EMSGSIZE
#undef ENETDOWN
#undef ENETRESET
#undef ENETUNREACH
#undef ENOBUFS
#undef ENOPROTOOPT
#undef ENOTCONN
#undef ENOTSOCK
#undef EOPNOTSUPP
#undef EPROTONOSUPPORT
#undef EPROTOTYPE
#undef EWOULDBLOCK

#if defined(WSAEALREADY) && !defined(EALREADY)
#define EALREADY WSAEALREADY
#endif
#if defined(WSAECONNABORTED) && !defined(ECONNABORTED)
#define ECONNABORTED WSAECONNABORTED
#endif
#if defined(WSAECONNREFUSED) && !defined(ECONNREFUSED)
#define ECONNREFUSED WSAECONNREFUSED
#endif
#if defined(WSAECONNRESET) && !defined(ECONNRESET)
#define ECONNRESET WSAECONNRESET
#endif
#if defined(WSAEINPROGRESS) && !defined(EINPROGRESS)
#define EINPROGRESS WSAEINPROGRESS
#endif
#if defined(WSAESHUTDOWN) && !defined(ESHUTDOWN)
#define ESHUTDOWN WSAESHUTDOWN
#endif
#if defined(WSAEWOULDBLOCK) && !defined(EWOULDBLOCK)
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#endif /* MS_WINDOWS */

struct static_exception {
    PyTypeObject *exc;
    const char *name;
};

static struct static_exception static_exceptions[] = {
#define ITEM(NAME) {&_PyExc_##NAME, #NAME}
    // Level 1
    ITEM(BaseException),

    // Level 2: BaseException subclasses
    ITEM(BaseExceptionGroup),
    ITEM(Exception),
    ITEM(GeneratorExit),
    ITEM(KeyboardInterrupt),
    ITEM(SystemExit),

    // Level 3: Exception(BaseException) subclasses
    ITEM(ArithmeticError),
    ITEM(AssertionError),
    ITEM(AttributeError),
    ITEM(BufferError),
    ITEM(EOFError),
    //ITEM(ExceptionGroup),
    ITEM(ImportError),
    ITEM(LookupError),
    ITEM(MemoryError),
    ITEM(NameError),
    ITEM(OSError),
    ITEM(ReferenceError),
    ITEM(RuntimeError),
    ITEM(StopAsyncIteration),
    ITEM(StopIteration),
    ITEM(SyntaxError),
    ITEM(SystemError),
    ITEM(TypeError),
    ITEM(ValueError),
    ITEM(Warning),

    // Level 4: ArithmeticError(Exception) subclasses
    ITEM(FloatingPointError),
    ITEM(OverflowError),
    ITEM(ZeroDivisionError),

    // Level 4: Warning(Exception) subclasses
    ITEM(BytesWarning),
    ITEM(DeprecationWarning),
    ITEM(EncodingWarning),
    ITEM(FutureWarning),
    ITEM(ImportWarning),
    ITEM(PendingDeprecationWarning),
    ITEM(ResourceWarning),
    ITEM(RuntimeWarning),
    ITEM(SyntaxWarning),
    ITEM(UnicodeWarning),
    ITEM(UserWarning),

    // Level 4: OSError(Exception) subclasses
    ITEM(BlockingIOError),
    ITEM(ChildProcessError),
    ITEM(ConnectionError),
    ITEM(FileExistsError),
    ITEM(FileNotFoundError),
    ITEM(InterruptedError),
    ITEM(IsADirectoryError),
    ITEM(NotADirectoryError),
    ITEM(PermissionError),
    ITEM(ProcessLookupError),
    ITEM(TimeoutError),

    // Level 4: Other subclasses
    ITEM(IndentationError), // base: SyntaxError(Exception)
    {&_PyExc_IncompleteInputError, "_IncompleteInputError"}, // base: SyntaxError(Exception)
    ITEM(IndexError),  // base: LookupError(Exception)
    ITEM(KeyError),  // base: LookupError(Exception)
    ITEM(ModuleNotFoundError), // base: ImportError(Exception)
    ITEM(NotImplementedError),  // base: RuntimeError(Exception)
    ITEM(PythonFinalizationError),  // base: RuntimeError(Exception)
    ITEM(RecursionError),  // base: RuntimeError(Exception)
    ITEM(UnboundLocalError), // base: NameError(Exception)
    ITEM(UnicodeError),  // base: ValueError(Exception)

    // Level 5: ConnectionError(OSError) subclasses
    ITEM(BrokenPipeError),
    ITEM(ConnectionAbortedError),
    ITEM(ConnectionRefusedError),
    ITEM(ConnectionResetError),

    // Level 5: IndentationError(SyntaxError) subclasses
    ITEM(TabError),  // base: IndentationError

    // Level 5: UnicodeError(ValueError) subclasses
    ITEM(UnicodeDecodeError),
    ITEM(UnicodeEncodeError),
    ITEM(UnicodeTranslateError),
#undef ITEM
};


int
_PyExc_InitTypes(PyInterpreterState *interp)
{
    for (size_t i=0; i < Py_ARRAY_LENGTH(static_exceptions); i++) {
        PyTypeObject *exc = static_exceptions[i].exc;
        if (_PyStaticType_InitBuiltin(interp, exc) < 0) {
            return -1;
        }
    }
    return 0;
}


static void
_PyExc_FiniTypes(PyInterpreterState *interp)
{
    for (Py_ssize_t i=Py_ARRAY_LENGTH(static_exceptions) - 1; i >= 0; i--) {
        PyTypeObject *exc = static_exceptions[i].exc;
        _PyStaticType_FiniBuiltin(interp, exc);
    }
}


PyStatus
_PyExc_InitGlobalObjects(PyInterpreterState *interp)
{
    if (preallocate_memerrors() < 0) {
        return _PyStatus_NO_MEMORY();
    }
    return _PyStatus_OK();
}

PyStatus
_PyExc_InitState(PyInterpreterState *interp)
{
    struct _Py_exc_state *state = &interp->exc_state;

#define ADD_ERRNO(TYPE, CODE) \
    do { \
        PyObject *_code = PyLong_FromLong(CODE); \
        assert(_PyObject_RealIsSubclass(PyExc_ ## TYPE, PyExc_OSError)); \
        if (!_code || PyDict_SetItem(state->errnomap, _code, PyExc_ ## TYPE)) { \
            Py_XDECREF(_code); \
            return _PyStatus_ERR("errmap insertion problem."); \
        } \
        Py_DECREF(_code); \
    } while (0)

    /* Add exceptions to errnomap */
    assert(state->errnomap == NULL);
    state->errnomap = PyDict_New();
    if (!state->errnomap) {
        return _PyStatus_NO_MEMORY();
    }

    ADD_ERRNO(BlockingIOError, EAGAIN);
    ADD_ERRNO(BlockingIOError, EALREADY);
    ADD_ERRNO(BlockingIOError, EINPROGRESS);
    ADD_ERRNO(BlockingIOError, EWOULDBLOCK);
    ADD_ERRNO(BrokenPipeError, EPIPE);
#ifdef ESHUTDOWN
    ADD_ERRNO(BrokenPipeError, ESHUTDOWN);
#endif
    ADD_ERRNO(ChildProcessError, ECHILD);
    ADD_ERRNO(ConnectionAbortedError, ECONNABORTED);
    ADD_ERRNO(ConnectionRefusedError, ECONNREFUSED);
    ADD_ERRNO(ConnectionResetError, ECONNRESET);
    ADD_ERRNO(FileExistsError, EEXIST);
    ADD_ERRNO(FileNotFoundError, ENOENT);
    ADD_ERRNO(IsADirectoryError, EISDIR);
    ADD_ERRNO(NotADirectoryError, ENOTDIR);
    ADD_ERRNO(InterruptedError, EINTR);
    ADD_ERRNO(PermissionError, EACCES);
    ADD_ERRNO(PermissionError, EPERM);
#ifdef ENOTCAPABLE
    // Extension for WASI capability-based security. Process lacks
    // capability to access a resource.
    ADD_ERRNO(PermissionError, ENOTCAPABLE);
#endif
    ADD_ERRNO(ProcessLookupError, ESRCH);
    ADD_ERRNO(TimeoutError, ETIMEDOUT);
#ifdef WSAETIMEDOUT
    ADD_ERRNO(TimeoutError, WSAETIMEDOUT);
#endif

    return _PyStatus_OK();

#undef ADD_ERRNO
}


/* Add exception types to the builtins module */
int
_PyBuiltins_AddExceptions(PyObject *bltinmod)
{
    PyObject *mod_dict = PyModule_GetDict(bltinmod);
    if (mod_dict == NULL) {
        return -1;
    }

    for (size_t i=0; i < Py_ARRAY_LENGTH(static_exceptions); i++) {
        struct static_exception item = static_exceptions[i];

        if (PyDict_SetItemString(mod_dict, item.name, (PyObject*)item.exc)) {
            return -1;
        }
    }

    PyObject *PyExc_ExceptionGroup = create_exception_group_class();
    if (!PyExc_ExceptionGroup) {
        return -1;
    }
    if (PyDict_SetItemString(mod_dict, "ExceptionGroup", PyExc_ExceptionGroup)) {
        return -1;
    }

#define INIT_ALIAS(NAME, TYPE) \
    do { \
        PyExc_ ## NAME = PyExc_ ## TYPE; \
        if (PyDict_SetItemString(mod_dict, # NAME, PyExc_ ## TYPE)) { \
            return -1; \
        } \
    } while (0)

    INIT_ALIAS(EnvironmentError, OSError);
    INIT_ALIAS(IOError, OSError);
#ifdef MS_WINDOWS
    INIT_ALIAS(WindowsError, OSError);
#endif

#undef INIT_ALIAS

    return 0;
}

void
_PyExc_ClearExceptionGroupType(PyInterpreterState *interp)
{
    struct _Py_exc_state *state = &interp->exc_state;
    Py_CLEAR(state->PyExc_ExceptionGroup);
}

void
_PyExc_Fini(PyInterpreterState *interp)
{
    struct _Py_exc_state *state = &interp->exc_state;
    free_preallocated_memerrors(state);
    Py_CLEAR(state->errnomap);

    _PyExc_FiniTypes(interp);
}

int
_PyException_AddNote(PyObject *exc, PyObject *note)
{
    if (!PyExceptionInstance_Check(exc)) {
        PyErr_Format(PyExc_TypeError,
                     "exc must be an exception, not '%s'",
                     Py_TYPE(exc)->tp_name);
        return -1;
    }
    PyObject *r = BaseException_add_note(exc, note);
    int res = r == NULL ? -1 : 0;
    Py_XDECREF(r);
    return res;
}

