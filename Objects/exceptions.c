/*
 * New exceptions.c written in Iceland by Richard Jones and Georg Brandl.
 *
 * Thanks go to Tim Peters and Michael Hudson for debugging.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "pycore_initconfig.h"
#include "pycore_object.h"
#include "structmember.h"         // PyMemberDef
#include "osdefs.h"               // SEP


/* Compatibility aliases */
PyObject *PyExc_EnvironmentError = NULL;
PyObject *PyExc_IOError = NULL;
#ifdef MS_WINDOWS
PyObject *PyExc_WindowsError = NULL;
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
    self->traceback = self->cause = self->context = NULL;
    self->suppress_context = 0;

    if (args) {
        self->args = args;
        Py_INCREF(args);
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

    Py_INCREF(args);
    Py_XSETREF(self->args, args);

    return 0;
}

static int
BaseException_clear(PyBaseExceptionObject *self)
{
    Py_CLEAR(self->dict);
    Py_CLEAR(self->args);
    Py_CLEAR(self->traceback);
    Py_CLEAR(self->cause);
    Py_CLEAR(self->context);
    return 0;
}

static void
BaseException_dealloc(PyBaseExceptionObject *self)
{
    _PyObject_GC_UNTRACK(self);
    BaseException_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
BaseException_traverse(PyBaseExceptionObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    Py_VISIT(self->args);
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
            if (PyObject_SetAttr(self, d_key, d_value) < 0)
                return NULL;
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
BaseException_with_traceback(PyObject *self, PyObject *tb) {
    if (PyException_SetTraceback(self, tb))
        return NULL;

    Py_INCREF(self);
    return self;
}

PyDoc_STRVAR(with_traceback_doc,
"Exception.with_traceback(tb) --\n\
    set self.__traceback__ to tb and return self.");


static PyMethodDef BaseException_methods[] = {
   {"__reduce__", (PyCFunction)BaseException_reduce, METH_NOARGS },
   {"__setstate__", (PyCFunction)BaseException_setstate, METH_O },
   {"with_traceback", (PyCFunction)BaseException_with_traceback, METH_O,
    with_traceback_doc},
   {NULL, NULL, 0, NULL},
};

static PyObject *
BaseException_get_args(PyBaseExceptionObject *self, void *Py_UNUSED(ignored))
{
    if (self->args == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(self->args);
    return self->args;
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
    Py_INCREF(self->traceback);
    return self->traceback;
}

static int
BaseException_set_tb(PyBaseExceptionObject *self, PyObject *tb, void *Py_UNUSED(ignored))
{
    if (tb == NULL) {
        PyErr_SetString(PyExc_TypeError, "__traceback__ may not be deleted");
        return -1;
    }
    else if (!(tb == Py_None || PyTraceBack_Check(tb))) {
        PyErr_SetString(PyExc_TypeError,
                        "__traceback__ must be a traceback or None");
        return -1;
    }

    Py_INCREF(tb);
    Py_XSETREF(self->traceback, tb);
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


static inline PyBaseExceptionObject*
_PyBaseExceptionObject_cast(PyObject *exc)
{
    assert(PyExceptionInstance_Check(exc));
    return (PyBaseExceptionObject *)exc;
}


PyObject *
PyException_GetTraceback(PyObject *self)
{
    PyBaseExceptionObject *base_self = _PyBaseExceptionObject_cast(self);
    Py_XINCREF(base_self->traceback);
    return base_self->traceback;
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
    Py_XINCREF(cause);
    return cause;
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
    Py_XINCREF(context);
    return context;
}

/* Steals a reference to context */
void
PyException_SetContext(PyObject *self, PyObject *context)
{
    Py_XSETREF(_PyBaseExceptionObject_cast(self)->context, context);
}

const char *
PyExceptionClass_Name(PyObject *ob)
{
    assert(PyExceptionClass_Check(ob));
    return ((PyTypeObject*)ob)->tp_name;
}

static struct PyMemberDef BaseException_members[] = {
    {"__suppress_context__", T_BOOL,
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

#define MiddlingExtendsException(EXCBASE, EXCNAME, EXCSTORE, EXCDOC) \
static PyTypeObject _PyExc_ ## EXCNAME = { \
    PyVarObject_HEAD_INIT(NULL, 0) \
    # EXCNAME, \
    sizeof(Py ## EXCSTORE ## Object), \
    0, (destructor)EXCSTORE ## _dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    0, 0, 0, 0, 0, \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, \
    PyDoc_STR(EXCDOC), (traverseproc)EXCSTORE ## _traverse, \
    (inquiry)EXCSTORE ## _clear, 0, 0, 0, 0, 0, 0, 0, &_ ## EXCBASE, \
    0, 0, 0, offsetof(Py ## EXCSTORE ## Object, dict), \
    (initproc)EXCSTORE ## _init, 0, 0, \
}; \
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
    {"value", T_OBJECT, offsetof(PyStopIterationObject, value), 0,
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
    Py_INCREF(value);
    self->value = value;
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
    _PyObject_GC_UNTRACK(self);
    StopIteration_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
StopIteration_traverse(PyStopIterationObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->value);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

ComplexExtendsException(
    PyExc_Exception,       /* base */
    StopIteration,         /* name */
    StopIteration,         /* prefix for *_init, etc */
    0,                     /* new */
    0,                     /* methods */
    StopIteration_members, /* members */
    0,                     /* getset */
    0,                     /* str */
    "Signal the end from iterator.__next__()."
);


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
        Py_INCREF(PyTuple_GET_ITEM(args, 0));
        Py_XSETREF(self->code, PyTuple_GET_ITEM(args, 0));
    }
    else { /* size > 1 */
        Py_INCREF(args);
        Py_XSETREF(self->code, args);
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
    {"code", T_OBJECT, offsetof(PySystemExitObject, code), 0,
        PyDoc_STR("exception code")},
    {NULL}  /* Sentinel */
};

ComplexExtendsException(PyExc_BaseException, SystemExit, SystemExit,
                        0, 0, SystemExit_members, 0, 0,
                        "Request to exit from the interpreter.");

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
    static char *kwlist[] = {"name", "path", 0};
    PyObject *empty_tuple;
    PyObject *msg = NULL;
    PyObject *name = NULL;
    PyObject *path = NULL;

    if (BaseException_init((PyBaseExceptionObject *)self, args, NULL) == -1)
        return -1;

    empty_tuple = PyTuple_New(0);
    if (!empty_tuple)
        return -1;
    if (!PyArg_ParseTupleAndKeywords(empty_tuple, kwds, "|$OO:ImportError", kwlist,
                                     &name, &path)) {
        Py_DECREF(empty_tuple);
        return -1;
    }
    Py_DECREF(empty_tuple);

    Py_XINCREF(name);
    Py_XSETREF(self->name, name);

    Py_XINCREF(path);
    Py_XSETREF(self->path, path);

    if (PyTuple_GET_SIZE(args) == 1) {
        msg = PyTuple_GET_ITEM(args, 0);
        Py_INCREF(msg);
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
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyObject *
ImportError_str(PyImportErrorObject *self)
{
    if (self->msg && PyUnicode_CheckExact(self->msg)) {
        Py_INCREF(self->msg);
        return self->msg;
    }
    else {
        return BaseException_str((PyBaseExceptionObject *)self);
    }
}

static PyObject *
ImportError_getstate(PyImportErrorObject *self)
{
    PyObject *dict = ((PyBaseExceptionObject *)self)->dict;
    if (self->name || self->path) {
        _Py_IDENTIFIER(name);
        _Py_IDENTIFIER(path);
        dict = dict ? PyDict_Copy(dict) : PyDict_New();
        if (dict == NULL)
            return NULL;
        if (self->name && _PyDict_SetItemId(dict, &PyId_name, self->name) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        if (self->path && _PyDict_SetItemId(dict, &PyId_path, self->path) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        return dict;
    }
    else if (dict) {
        Py_INCREF(dict);
        return dict;
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
    {"msg", T_OBJECT, offsetof(PyImportErrorObject, msg), 0,
        PyDoc_STR("exception message")},
    {"name", T_OBJECT, offsetof(PyImportErrorObject, name), 0,
        PyDoc_STR("module name")},
    {"path", T_OBJECT, offsetof(PyImportErrorObject, path), 0,
        PyDoc_STR("module path")},
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
            /* Set errno to the corresponding POSIX errno (overriding
               first argument).  Windows Socket error codes (>= 10000)
               have the same value as their POSIX counterparts.
            */
            if (winerrcode < 10000)
                errcode = winerror_to_errno(winerrcode);
            else
                errcode = winerrcode;
            *myerrno = PyLong_FromLong(errcode);
            if (!*myerrno)
                return -1;
            newargs = PyTuple_New(nargs);
            if (!newargs)
                return -1;
            PyTuple_SET_ITEM(newargs, 0, *myerrno);
            for (i = 1; i < nargs; i++) {
                PyObject *val = PyTuple_GET_ITEM(args, i);
                Py_INCREF(val);
                PyTuple_SET_ITEM(newargs, i, val);
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
            Py_INCREF(filename);
            self->filename = filename;

            if (filename2 && filename2 != Py_None) {
                Py_INCREF(filename2);
                self->filename2 = filename2;
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
    Py_XINCREF(myerrno);
    self->myerrno = myerrno;

    Py_XINCREF(strerror);
    self->strerror = strerror;

#ifdef MS_WINDOWS
    Py_XINCREF(winerror);
    self->winerror = winerror;
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
                assert(PyType_Check(newtype));
                type = (PyTypeObject *) newtype;
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
    PyObject *res = NULL, *tmp;

    /* self->args is only the first two real arguments if there was a
     * file name given to OSError. */
    if (PyTuple_GET_SIZE(args) == 2 && self->filename) {
        Py_ssize_t size = self->filename2 ? 5 : 3;
        args = PyTuple_New(size);
        if (!args)
            return NULL;

        tmp = PyTuple_GET_ITEM(self->args, 0);
        Py_INCREF(tmp);
        PyTuple_SET_ITEM(args, 0, tmp);

        tmp = PyTuple_GET_ITEM(self->args, 1);
        Py_INCREF(tmp);
        PyTuple_SET_ITEM(args, 1, tmp);

        Py_INCREF(self->filename);
        PyTuple_SET_ITEM(args, 2, self->filename);

        if (self->filename2) {
            /*
             * This tuple is essentially used as OSError(*args).
             * So, to recreate filename2, we need to pass in
             * winerror as well.
             */
            Py_INCREF(Py_None);
            PyTuple_SET_ITEM(args, 3, Py_None);

            /* filename2 */
            Py_INCREF(self->filename2);
            PyTuple_SET_ITEM(args, 4, self->filename2);
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
    {"errno", T_OBJECT, offsetof(PyOSErrorObject, myerrno), 0,
        PyDoc_STR("POSIX exception code")},
    {"strerror", T_OBJECT, offsetof(PyOSErrorObject, strerror), 0,
        PyDoc_STR("exception strerror")},
    {"filename", T_OBJECT, offsetof(PyOSErrorObject, filename), 0,
        PyDoc_STR("exception filename")},
    {"filename2", T_OBJECT, offsetof(PyOSErrorObject, filename2), 0,
        PyDoc_STR("second exception filename")},
#ifdef MS_WINDOWS
    {"winerror", T_OBJECT, offsetof(PyOSErrorObject, winerror), 0,
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

    Py_XINCREF(name);
    Py_XSETREF(self->name, name);

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
        {"name", T_OBJECT, offsetof(PyNameErrorObject, name), 0, PyDoc_STR("name")},
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

    Py_XINCREF(name);
    Py_XSETREF(self->name, name);

    Py_XINCREF(obj);
    Py_XSETREF(self->obj, obj);

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

static PyMemberDef AttributeError_members[] = {
    {"name", T_OBJECT, offsetof(PyAttributeErrorObject, name), 0, PyDoc_STR("attribute name")},
    {"obj", T_OBJECT, offsetof(PyAttributeErrorObject, obj), 0, PyDoc_STR("object")},
    {NULL}  /* Sentinel */
};

static PyMethodDef AttributeError_methods[] = {
    {NULL}  /* Sentinel */
};

ComplexExtendsException(PyExc_Exception, AttributeError,
                        AttributeError, 0,
                        AttributeError_methods, AttributeError_members,
                        0, BaseException_str, "Attribute not found.");

/*
 *    SyntaxError extends Exception
 */

/* Helper function to customize error message for some syntax errors */
static int _report_missing_parentheses(PySyntaxErrorObject *self);

static int
SyntaxError_init(PySyntaxErrorObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *info = NULL;
    Py_ssize_t lenargs = PyTuple_GET_SIZE(args);

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    if (lenargs >= 1) {
        Py_INCREF(PyTuple_GET_ITEM(args, 0));
        Py_XSETREF(self->msg, PyTuple_GET_ITEM(args, 0));
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

        /*
         * Issue #21669: Custom error for 'print' & 'exec' as statements
         *
         * Only applies to SyntaxError instances, not to subclasses such
         * as TabError or IndentationError (see issue #31161)
         */
        if (Py_IS_TYPE(self, (PyTypeObject *)PyExc_SyntaxError) &&
                self->text && PyUnicode_Check(self->text) &&
                _report_missing_parentheses(self) < 0) {
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

    if (PyUnicode_READY(name))
        return NULL;
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
        Py_INCREF(name);
        return name;
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
    {"msg", T_OBJECT, offsetof(PySyntaxErrorObject, msg), 0,
        PyDoc_STR("exception msg")},
    {"filename", T_OBJECT, offsetof(PySyntaxErrorObject, filename), 0,
        PyDoc_STR("exception filename")},
    {"lineno", T_OBJECT, offsetof(PySyntaxErrorObject, lineno), 0,
        PyDoc_STR("exception lineno")},
    {"offset", T_OBJECT, offsetof(PySyntaxErrorObject, offset), 0,
        PyDoc_STR("exception offset")},
    {"text", T_OBJECT, offsetof(PySyntaxErrorObject, text), 0,
        PyDoc_STR("exception text")},
    {"end_lineno", T_OBJECT, offsetof(PySyntaxErrorObject, end_lineno), 0,
                   PyDoc_STR("exception end lineno")},
    {"end_offset", T_OBJECT, offsetof(PySyntaxErrorObject, end_offset), 0,
                   PyDoc_STR("exception end offset")},
    {"print_file_and_line", T_OBJECT,
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
    Py_INCREF(attr);
    return attr;
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
    Py_INCREF(attr);
    return attr;
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
    {"encoding", T_OBJECT, offsetof(PyUnicodeErrorObject, encoding), 0,
        PyDoc_STR("exception encoding")},
    {"object", T_OBJECT, offsetof(PyUnicodeErrorObject, object), 0,
        PyDoc_STR("exception object")},
    {"start", T_PYSSIZET, offsetof(PyUnicodeErrorObject, start), 0,
        PyDoc_STR("exception start")},
    {"end", T_PYSSIZET, offsetof(PyUnicodeErrorObject, end), 0,
        PyDoc_STR("exception end")},
    {"reason", T_OBJECT, offsetof(PyUnicodeErrorObject, reason), 0,
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
    PyUnicodeErrorObject *uself = (PyUnicodeErrorObject *)self;
    PyObject *result = NULL;
    PyObject *reason_str = NULL;
    PyObject *encoding_str = NULL;

    if (!uself->object)
        /* Not properly initialized. */
        return PyUnicode_FromString("");

    /* Get reason and encoding as strings, which they might not be if
       they've been modified after we were constructed. */
    reason_str = PyObject_Str(uself->reason);
    if (reason_str == NULL)
        goto done;
    encoding_str = PyObject_Str(uself->encoding);
    if (encoding_str == NULL)
        goto done;

    if (uself->start < PyUnicode_GET_LENGTH(uself->object) && uself->end == uself->start+1) {
        Py_UCS4 badchar = PyUnicode_ReadChar(uself->object, uself->start);
        const char *fmt;
        if (badchar <= 0xff)
            fmt = "'%U' codec can't encode character '\\x%02x' in position %zd: %U";
        else if (badchar <= 0xffff)
            fmt = "'%U' codec can't encode character '\\u%04x' in position %zd: %U";
        else
            fmt = "'%U' codec can't encode character '\\U%08x' in position %zd: %U";
        result = PyUnicode_FromFormat(
            fmt,
            encoding_str,
            (int)badchar,
            uself->start,
            reason_str);
    }
    else {
        result = PyUnicode_FromFormat(
            "'%U' codec can't encode characters in position %zd-%zd: %U",
            encoding_str,
            uself->start,
            uself->end-1,
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
    PyUnicodeErrorObject *uself = (PyUnicodeErrorObject *)self;
    PyObject *result = NULL;
    PyObject *reason_str = NULL;
    PyObject *encoding_str = NULL;

    if (!uself->object)
        /* Not properly initialized. */
        return PyUnicode_FromString("");

    /* Get reason and encoding as strings, which they might not be if
       they've been modified after we were constructed. */
    reason_str = PyObject_Str(uself->reason);
    if (reason_str == NULL)
        goto done;
    encoding_str = PyObject_Str(uself->encoding);
    if (encoding_str == NULL)
        goto done;

    if (uself->start < PyBytes_GET_SIZE(uself->object) && uself->end == uself->start+1) {
        int byte = (int)(PyBytes_AS_STRING(((PyUnicodeErrorObject *)self)->object)[uself->start]&0xff);
        result = PyUnicode_FromFormat(
            "'%U' codec can't decode byte 0x%02x in position %zd: %U",
            encoding_str,
            byte,
            uself->start,
            reason_str);
    }
    else {
        result = PyUnicode_FromFormat(
            "'%U' codec can't decode bytes in position %zd-%zd: %U",
            encoding_str,
            uself->start,
            uself->end-1,
            reason_str
            );
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
    PyUnicodeErrorObject *uself = (PyUnicodeErrorObject *)self;
    PyObject *result = NULL;
    PyObject *reason_str = NULL;

    if (!uself->object)
        /* Not properly initialized. */
        return PyUnicode_FromString("");

    /* Get reason as a string, which it might not be if it's been
       modified after we were constructed. */
    reason_str = PyObject_Str(uself->reason);
    if (reason_str == NULL)
        goto done;

    if (uself->start < PyUnicode_GET_LENGTH(uself->object) && uself->end == uself->start+1) {
        Py_UCS4 badchar = PyUnicode_ReadChar(uself->object, uself->start);
        const char *fmt;
        if (badchar <= 0xff)
            fmt = "can't translate character '\\x%02x' in position %zd: %U";
        else if (badchar <= 0xffff)
            fmt = "can't translate character '\\u%04x' in position %zd: %U";
        else
            fmt = "can't translate character '\\U%08x' in position %zd: %U";
        result = PyUnicode_FromFormat(
            fmt,
            (int)badchar,
            uself->start,
            reason_str
        );
    } else {
        result = PyUnicode_FromFormat(
            "can't translate characters in position %zd-%zd: %U",
            uself->start,
            uself->end-1,
            reason_str
            );
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
                       "Floating point operation failed.");


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

static PyObject *
MemoryError_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyBaseExceptionObject *self;

    /* If this is a subclass of MemoryError, don't use the freelist
     * and just return a fresh object */
    if (type != (PyTypeObject *) PyExc_MemoryError) {
        return BaseException_new(type, args, kwds);
    }

    struct _Py_exc_state *state = get_exc_state();
    if (state->memerrors_freelist == NULL) {
        return BaseException_new(type, args, kwds);
    }

    /* Fetch object from freelist and revive it */
    self = state->memerrors_freelist;
    self->args = PyTuple_New(0);
    /* This shouldn't happen since the empty tuple is persistent */
    if (self->args == NULL) {
        return NULL;
    }

    state->memerrors_freelist = (PyBaseExceptionObject *) self->dict;
    state->memerrors_numfree--;
    self->dict = NULL;
    _Py_NewReference((PyObject *)self);
    _PyObject_GC_TRACK(self);
    return (PyObject *)self;
}

static void
MemoryError_dealloc(PyBaseExceptionObject *self)
{
    BaseException_clear(self);

    /* If this is a subclass of MemoryError, we don't need to
     * do anything in the free-list*/
    if (!Py_IS_TYPE(self, (PyTypeObject *) PyExc_MemoryError)) {
        Py_TYPE(self)->tp_free((PyObject *)self);
        return;
    }

    _PyObject_GC_UNTRACK(self);

    struct _Py_exc_state *state = get_exc_state();
    if (state->memerrors_numfree >= MEMERRORS_SAVE) {
        Py_TYPE(self)->tp_free((PyObject *)self);
    }
    else {
        self->dict = (PyObject *) state->memerrors_freelist;
        state->memerrors_freelist = self;
        state->memerrors_numfree++;
    }
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


static PyTypeObject _PyExc_MemoryError = {
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
#undef ETIMEDOUT
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
#if defined(WSAETIMEDOUT) && !defined(ETIMEDOUT)
#define ETIMEDOUT WSAETIMEDOUT
#endif
#if defined(WSAEWOULDBLOCK) && !defined(EWOULDBLOCK)
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#endif /* MS_WINDOWS */

PyStatus
_PyExc_Init(PyInterpreterState *interp)
{
    struct _Py_exc_state *state = &interp->exc_state;

#define PRE_INIT(TYPE) \
    if (!(_PyExc_ ## TYPE.tp_flags & Py_TPFLAGS_READY)) { \
        if (PyType_Ready(&_PyExc_ ## TYPE) < 0) { \
            return _PyStatus_ERR("exceptions bootstrapping error."); \
        } \
        Py_INCREF(PyExc_ ## TYPE); \
    }

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

    PRE_INIT(BaseException);
    PRE_INIT(Exception);
    PRE_INIT(TypeError);
    PRE_INIT(StopAsyncIteration);
    PRE_INIT(StopIteration);
    PRE_INIT(GeneratorExit);
    PRE_INIT(SystemExit);
    PRE_INIT(KeyboardInterrupt);
    PRE_INIT(ImportError);
    PRE_INIT(ModuleNotFoundError);
    PRE_INIT(OSError);
    PRE_INIT(EOFError);
    PRE_INIT(RuntimeError);
    PRE_INIT(RecursionError);
    PRE_INIT(NotImplementedError);
    PRE_INIT(NameError);
    PRE_INIT(UnboundLocalError);
    PRE_INIT(AttributeError);
    PRE_INIT(SyntaxError);
    PRE_INIT(IndentationError);
    PRE_INIT(TabError);
    PRE_INIT(LookupError);
    PRE_INIT(IndexError);
    PRE_INIT(KeyError);
    PRE_INIT(ValueError);
    PRE_INIT(UnicodeError);
    PRE_INIT(UnicodeEncodeError);
    PRE_INIT(UnicodeDecodeError);
    PRE_INIT(UnicodeTranslateError);
    PRE_INIT(AssertionError);
    PRE_INIT(ArithmeticError);
    PRE_INIT(FloatingPointError);
    PRE_INIT(OverflowError);
    PRE_INIT(ZeroDivisionError);
    PRE_INIT(SystemError);
    PRE_INIT(ReferenceError);
    PRE_INIT(MemoryError);
    PRE_INIT(BufferError);
    PRE_INIT(Warning);
    PRE_INIT(UserWarning);
    PRE_INIT(EncodingWarning);
    PRE_INIT(DeprecationWarning);
    PRE_INIT(PendingDeprecationWarning);
    PRE_INIT(SyntaxWarning);
    PRE_INIT(RuntimeWarning);
    PRE_INIT(FutureWarning);
    PRE_INIT(ImportWarning);
    PRE_INIT(UnicodeWarning);
    PRE_INIT(BytesWarning);
    PRE_INIT(ResourceWarning);

    /* OSError subclasses */
    PRE_INIT(ConnectionError);

    PRE_INIT(BlockingIOError);
    PRE_INIT(BrokenPipeError);
    PRE_INIT(ChildProcessError);
    PRE_INIT(ConnectionAbortedError);
    PRE_INIT(ConnectionRefusedError);
    PRE_INIT(ConnectionResetError);
    PRE_INIT(FileExistsError);
    PRE_INIT(FileNotFoundError);
    PRE_INIT(IsADirectoryError);
    PRE_INIT(NotADirectoryError);
    PRE_INIT(InterruptedError);
    PRE_INIT(PermissionError);
    PRE_INIT(ProcessLookupError);
    PRE_INIT(TimeoutError);

    if (preallocate_memerrors() < 0) {
        return _PyStatus_NO_MEMORY();
    }

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
    ADD_ERRNO(ProcessLookupError, ESRCH);
    ADD_ERRNO(TimeoutError, ETIMEDOUT);

    return _PyStatus_OK();

#undef PRE_INIT
#undef ADD_ERRNO
}


/* Add exception types to the builtins module */
PyStatus
_PyBuiltins_AddExceptions(PyObject *bltinmod)
{
#define POST_INIT(TYPE) \
    if (PyDict_SetItemString(bdict, # TYPE, PyExc_ ## TYPE)) { \
        return _PyStatus_ERR("Module dictionary insertion problem."); \
    }

#define INIT_ALIAS(NAME, TYPE) \
    do { \
        Py_INCREF(PyExc_ ## TYPE); \
        Py_XDECREF(PyExc_ ## NAME); \
        PyExc_ ## NAME = PyExc_ ## TYPE; \
        if (PyDict_SetItemString(bdict, # NAME, PyExc_ ## NAME)) { \
            return _PyStatus_ERR("Module dictionary insertion problem."); \
        } \
    } while (0)

    PyObject *bdict;

    bdict = PyModule_GetDict(bltinmod);
    if (bdict == NULL) {
        return _PyStatus_ERR("exceptions bootstrapping error.");
    }

    POST_INIT(BaseException);
    POST_INIT(Exception);
    POST_INIT(TypeError);
    POST_INIT(StopAsyncIteration);
    POST_INIT(StopIteration);
    POST_INIT(GeneratorExit);
    POST_INIT(SystemExit);
    POST_INIT(KeyboardInterrupt);
    POST_INIT(ImportError);
    POST_INIT(ModuleNotFoundError);
    POST_INIT(OSError);
    INIT_ALIAS(EnvironmentError, OSError);
    INIT_ALIAS(IOError, OSError);
#ifdef MS_WINDOWS
    INIT_ALIAS(WindowsError, OSError);
#endif
    POST_INIT(EOFError);
    POST_INIT(RuntimeError);
    POST_INIT(RecursionError);
    POST_INIT(NotImplementedError);
    POST_INIT(NameError);
    POST_INIT(UnboundLocalError);
    POST_INIT(AttributeError);
    POST_INIT(SyntaxError);
    POST_INIT(IndentationError);
    POST_INIT(TabError);
    POST_INIT(LookupError);
    POST_INIT(IndexError);
    POST_INIT(KeyError);
    POST_INIT(ValueError);
    POST_INIT(UnicodeError);
    POST_INIT(UnicodeEncodeError);
    POST_INIT(UnicodeDecodeError);
    POST_INIT(UnicodeTranslateError);
    POST_INIT(AssertionError);
    POST_INIT(ArithmeticError);
    POST_INIT(FloatingPointError);
    POST_INIT(OverflowError);
    POST_INIT(ZeroDivisionError);
    POST_INIT(SystemError);
    POST_INIT(ReferenceError);
    POST_INIT(MemoryError);
    POST_INIT(BufferError);
    POST_INIT(Warning);
    POST_INIT(UserWarning);
    POST_INIT(EncodingWarning);
    POST_INIT(DeprecationWarning);
    POST_INIT(PendingDeprecationWarning);
    POST_INIT(SyntaxWarning);
    POST_INIT(RuntimeWarning);
    POST_INIT(FutureWarning);
    POST_INIT(ImportWarning);
    POST_INIT(UnicodeWarning);
    POST_INIT(BytesWarning);
    POST_INIT(ResourceWarning);

    /* OSError subclasses */
    POST_INIT(ConnectionError);

    POST_INIT(BlockingIOError);
    POST_INIT(BrokenPipeError);
    POST_INIT(ChildProcessError);
    POST_INIT(ConnectionAbortedError);
    POST_INIT(ConnectionRefusedError);
    POST_INIT(ConnectionResetError);
    POST_INIT(FileExistsError);
    POST_INIT(FileNotFoundError);
    POST_INIT(IsADirectoryError);
    POST_INIT(NotADirectoryError);
    POST_INIT(InterruptedError);
    POST_INIT(PermissionError);
    POST_INIT(ProcessLookupError);
    POST_INIT(TimeoutError);

    return _PyStatus_OK();

#undef POST_INIT
#undef INIT_ALIAS
}

void
_PyExc_Fini(PyInterpreterState *interp)
{
    struct _Py_exc_state *state = &interp->exc_state;
    free_preallocated_memerrors(state);
    Py_CLEAR(state->errnomap);
}

/* Helper to do the equivalent of "raise X from Y" in C, but always using
 * the current exception rather than passing one in.
 *
 * We currently limit this to *only* exceptions that use the BaseException
 * tp_init and tp_new methods, since we can be reasonably sure we can wrap
 * those correctly without losing data and without losing backwards
 * compatibility.
 *
 * We also aim to rule out *all* exceptions that might be storing additional
 * state, whether by having a size difference relative to BaseException,
 * additional arguments passed in during construction or by having a
 * non-empty instance dict.
 *
 * We need to be very careful with what we wrap, since changing types to
 * a broader exception type would be backwards incompatible for
 * existing codecs, and with different init or new method implementations
 * may either not support instantiation with PyErr_Format or lose
 * information when instantiated that way.
 *
 * XXX (ncoghlan): This could be made more comprehensive by exploiting the
 * fact that exceptions are expected to support pickling. If more builtin
 * exceptions (e.g. AttributeError) start to be converted to rich
 * exceptions with additional attributes, that's probably a better approach
 * to pursue over adding special cases for particular stateful subclasses.
 *
 * Returns a borrowed reference to the new exception (if any), NULL if the
 * existing exception was left in place.
 */
PyObject *
_PyErr_TrySetFromCause(const char *format, ...)
{
    PyObject* msg_prefix;
    PyObject *exc, *val, *tb;
    PyTypeObject *caught_type;
    PyObject **dictptr;
    PyObject *instance_args;
    Py_ssize_t num_args, caught_type_size, base_exc_size;
    PyObject *new_exc, *new_val, *new_tb;
    va_list vargs;
    int same_basic_size;

    PyErr_Fetch(&exc, &val, &tb);
    caught_type = (PyTypeObject *)exc;
    /* Ensure type info indicates no extra state is stored at the C level
     * and that the type can be reinstantiated using PyErr_Format
     */
    caught_type_size = caught_type->tp_basicsize;
    base_exc_size = _PyExc_BaseException.tp_basicsize;
    same_basic_size = (
        caught_type_size == base_exc_size ||
        (PyType_SUPPORTS_WEAKREFS(caught_type) &&
            (caught_type_size == base_exc_size + (Py_ssize_t)sizeof(PyObject *))
        )
    );
    if (caught_type->tp_init != (initproc)BaseException_init ||
        caught_type->tp_new != BaseException_new ||
        !same_basic_size ||
        caught_type->tp_itemsize != _PyExc_BaseException.tp_itemsize) {
        /* We can't be sure we can wrap this safely, since it may contain
         * more state than just the exception type. Accordingly, we just
         * leave it alone.
         */
        PyErr_Restore(exc, val, tb);
        return NULL;
    }

    /* Check the args are empty or contain a single string */
    PyErr_NormalizeException(&exc, &val, &tb);
    instance_args = ((PyBaseExceptionObject *)val)->args;
    num_args = PyTuple_GET_SIZE(instance_args);
    if (num_args > 1 ||
        (num_args == 1 &&
         !PyUnicode_CheckExact(PyTuple_GET_ITEM(instance_args, 0)))) {
        /* More than 1 arg, or the one arg we do have isn't a string
         */
        PyErr_Restore(exc, val, tb);
        return NULL;
    }

    /* Ensure the instance dict is also empty */
    dictptr = _PyObject_GetDictPtr(val);
    if (dictptr != NULL && *dictptr != NULL &&
        PyDict_GET_SIZE(*dictptr) > 0) {
        /* While we could potentially copy a non-empty instance dictionary
         * to the replacement exception, for now we take the more
         * conservative path of leaving exceptions with attributes set
         * alone.
         */
        PyErr_Restore(exc, val, tb);
        return NULL;
    }

    /* For exceptions that we can wrap safely, we chain the original
     * exception to a new one of the exact same type with an
     * error message that mentions the additional details and the
     * original exception.
     *
     * It would be nice to wrap OSError and various other exception
     * types as well, but that's quite a bit trickier due to the extra
     * state potentially stored on OSError instances.
     */
    /* Ensure the traceback is set correctly on the existing exception */
    if (tb != NULL) {
        PyException_SetTraceback(val, tb);
        Py_DECREF(tb);
    }

#ifdef HAVE_STDARG_PROTOTYPES
    va_start(vargs, format);
#else
    va_start(vargs);
#endif
    msg_prefix = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg_prefix == NULL) {
        Py_DECREF(exc);
        Py_DECREF(val);
        return NULL;
    }

    PyErr_Format(exc, "%U (%s: %S)",
                 msg_prefix, Py_TYPE(val)->tp_name, val);
    Py_DECREF(exc);
    Py_DECREF(msg_prefix);
    PyErr_Fetch(&new_exc, &new_val, &new_tb);
    PyErr_NormalizeException(&new_exc, &new_val, &new_tb);
    PyException_SetCause(new_val, val);
    PyErr_Restore(new_exc, new_val, new_tb);
    return new_val;
}


/* To help with migration from Python 2, SyntaxError.__init__ applies some
 * heuristics to try to report a more meaningful exception when print and
 * exec are used like statements.
 *
 * The heuristics are currently expected to detect the following cases:
 *   - top level statement
 *   - statement in a nested suite
 *   - trailing section of a one line complex statement
 *
 * They're currently known not to trigger:
 *   - after a semi-colon
 *
 * The error message can be a bit odd in cases where the "arguments" are
 * completely illegal syntactically, but that isn't worth the hassle of
 * fixing.
 *
 * We also can't do anything about cases that are legal Python 3 syntax
 * but mean something entirely different from what they did in Python 2
 * (omitting the arguments entirely, printing items preceded by a unary plus
 * or minus, using the stream redirection syntax).
 */


// Static helper for setting legacy print error message
static int
_set_legacy_print_statement_msg(PySyntaxErrorObject *self, Py_ssize_t start)
{
    // PRINT_OFFSET is to remove the `print ` prefix from the data.
    const int PRINT_OFFSET = 6;
    const int STRIP_BOTH = 2;
    Py_ssize_t start_pos = start + PRINT_OFFSET;
    Py_ssize_t text_len = PyUnicode_GET_LENGTH(self->text);
    Py_UCS4 semicolon = ';';
    Py_ssize_t end_pos = PyUnicode_FindChar(self->text, semicolon,
                                            start_pos, text_len, 1);
    if (end_pos < -1) {
      return -1;
    } else if (end_pos == -1) {
      end_pos = text_len;
    }

    PyObject *data = PyUnicode_Substring(self->text, start_pos, end_pos);
    if (data == NULL) {
        return -1;
    }

    PyObject *strip_sep_obj = PyUnicode_FromString(" \t\r\n");
    if (strip_sep_obj == NULL) {
        Py_DECREF(data);
        return -1;
    }

    PyObject *new_data = _PyUnicode_XStrip(data, STRIP_BOTH, strip_sep_obj);
    Py_DECREF(data);
    Py_DECREF(strip_sep_obj);
    if (new_data == NULL) {
        return -1;
    }
    // gets the modified text_len after stripping `print `
    text_len = PyUnicode_GET_LENGTH(new_data);
    const char *maybe_end_arg = "";
    if (text_len > 0 && PyUnicode_READ_CHAR(new_data, text_len-1) == ',') {
        maybe_end_arg = " end=\" \"";
    }
    PyObject *error_msg = PyUnicode_FromFormat(
        "Missing parentheses in call to 'print'. Did you mean print(%U%s)?",
        new_data, maybe_end_arg
    );
    Py_DECREF(new_data);
    if (error_msg == NULL)
        return -1;

    Py_XSETREF(self->msg, error_msg);
    return 1;
}

static int
_check_for_legacy_statements(PySyntaxErrorObject *self, Py_ssize_t start)
{
    /* Return values:
     *   -1: an error occurred
     *    0: nothing happened
     *    1: the check triggered & the error message was changed
     */
    static PyObject *print_prefix = NULL;
    static PyObject *exec_prefix = NULL;
    Py_ssize_t text_len = PyUnicode_GET_LENGTH(self->text), match;
    int kind = PyUnicode_KIND(self->text);
    const void *data = PyUnicode_DATA(self->text);

    /* Ignore leading whitespace */
    while (start < text_len) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, start);
        if (!Py_UNICODE_ISSPACE(ch))
            break;
        start++;
    }
    /* Checking against an empty or whitespace-only part of the string */
    if (start == text_len) {
        return 0;
    }

    /* Check for legacy print statements */
    if (print_prefix == NULL) {
        print_prefix = PyUnicode_InternFromString("print ");
        if (print_prefix == NULL) {
            return -1;
        }
    }
    match = PyUnicode_Tailmatch(self->text, print_prefix,
                                start, text_len, -1);
    if (match == -1) {
        return -1;
    }
    if (match) {
        return _set_legacy_print_statement_msg(self, start);
    }

    /* Check for legacy exec statements */
    if (exec_prefix == NULL) {
        exec_prefix = PyUnicode_InternFromString("exec ");
        if (exec_prefix == NULL) {
            return -1;
        }
    }
    match = PyUnicode_Tailmatch(self->text, exec_prefix, start, text_len, -1);
    if (match == -1) {
        return -1;
    }
    if (match) {
        PyObject *msg = PyUnicode_FromString("Missing parentheses in call "
                                             "to 'exec'");
        if (msg == NULL) {
            return -1;
        }
        Py_XSETREF(self->msg, msg);
        return 1;
    }
    /* Fall back to the default error message */
    return 0;
}

static int
_report_missing_parentheses(PySyntaxErrorObject *self)
{
    Py_UCS4 left_paren = 40;
    Py_ssize_t left_paren_index;
    Py_ssize_t text_len = PyUnicode_GET_LENGTH(self->text);
    int legacy_check_result = 0;

    /* Skip entirely if there is an opening parenthesis */
    left_paren_index = PyUnicode_FindChar(self->text, left_paren,
                                          0, text_len, 1);
    if (left_paren_index < -1) {
        return -1;
    }
    if (left_paren_index != -1) {
        /* Use default error message for any line with an opening paren */
        return 0;
    }
    /* Handle the simple statement case */
    legacy_check_result = _check_for_legacy_statements(self, 0);
    if (legacy_check_result < 0) {
        return -1;

    }
    if (legacy_check_result == 0) {
        /* Handle the one-line complex statement case */
        Py_UCS4 colon = 58;
        Py_ssize_t colon_index;
        colon_index = PyUnicode_FindChar(self->text, colon,
                                         0, text_len, 1);
        if (colon_index < -1) {
            return -1;
        }
        if (colon_index >= 0 && colon_index < text_len) {
            /* Check again, starting from just after the colon */
            if (_check_for_legacy_statements(self, colon_index+1) < 0) {
                return -1;
            }
        }
    }
    return 0;
}
