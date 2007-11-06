/*
 * New exceptions.c written in Iceland by Richard Jones and Georg Brandl.
 *
 * Thanks go to Tim Peters and Michael Hudson for debugging.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"
#include "osdefs.h"

#define MAKE_IT_NONE(x) (x) = Py_None; Py_INCREF(Py_None);

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
    if (!_PyArg_NoKeywords(Py_Type(self)->tp_name, kwds))
        return -1;

    Py_DECREF(self->args);
    self->args = args;
    Py_INCREF(self->args);

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
    Py_Type(self)->tp_free((PyObject *)self);
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
        return PyObject_Unicode(PyTuple_GET_ITEM(self->args, 0));
    default:
        return PyObject_Unicode(self->args);
    }
}

static PyObject *
BaseException_repr(PyBaseExceptionObject *self)
{
    char *name;
    char *dot;

    name = (char *)Py_Type(self)->tp_name;
    dot = strrchr(name, '.');
    if (dot != NULL) name = dot+1;

    return PyUnicode_FromFormat("%s%R", name, self->args);
}

/* Pickling support */
static PyObject *
BaseException_reduce(PyBaseExceptionObject *self)
{
    if (self->args && self->dict)
        return PyTuple_Pack(3, Py_Type(self), self->args, self->dict);
    else
        return PyTuple_Pack(2, Py_Type(self), self->args);
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


static PyMethodDef BaseException_methods[] = {
   {"__reduce__", (PyCFunction)BaseException_reduce, METH_NOARGS },
   {"__setstate__", (PyCFunction)BaseException_setstate, METH_O },
   {"with_traceback", (PyCFunction)BaseException_with_traceback, METH_O },
   {NULL, NULL, 0, NULL},
};


static PyObject *
BaseException_get_dict(PyBaseExceptionObject *self)
{
    if (self->dict == NULL) {
        self->dict = PyDict_New();
        if (!self->dict)
            return NULL;
    }
    Py_INCREF(self->dict);
    return self->dict;
}

static int
BaseException_set_dict(PyBaseExceptionObject *self, PyObject *val)
{
    if (val == NULL) {
        PyErr_SetString(PyExc_TypeError, "__dict__ may not be deleted");
        return -1;
    }
    if (!PyDict_Check(val)) {
        PyErr_SetString(PyExc_TypeError, "__dict__ must be a dictionary");
        return -1;
    }
    Py_CLEAR(self->dict);
    Py_INCREF(val);
    self->dict = val;
    return 0;
}

static PyObject *
BaseException_get_args(PyBaseExceptionObject *self)
{
    if (self->args == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_INCREF(self->args);
    return self->args;
}

static int
BaseException_set_args(PyBaseExceptionObject *self, PyObject *val)
{
    PyObject *seq;
    if (val == NULL) {
        PyErr_SetString(PyExc_TypeError, "args may not be deleted");
        return -1;
    }
    seq = PySequence_Tuple(val);
    if (!seq) return -1;
    Py_CLEAR(self->args);
    self->args = seq;
    return 0;
}

static PyObject *
BaseException_get_tb(PyBaseExceptionObject *self)
{
    if (self->traceback == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_INCREF(self->traceback);
    return self->traceback;
}

static int
BaseException_set_tb(PyBaseExceptionObject *self, PyObject *tb)
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

    Py_XINCREF(tb);
    Py_XDECREF(self->traceback);
    self->traceback = tb;
    return 0;
}


static PyGetSetDef BaseException_getset[] = {
    {"__dict__", (getter)BaseException_get_dict, (setter)BaseException_set_dict},
    {"args", (getter)BaseException_get_args, (setter)BaseException_set_args},
    {"__traceback__", (getter)BaseException_get_tb, (setter)BaseException_set_tb},
    {NULL},
};


PyObject *
PyException_GetTraceback(PyObject *self) {
    PyBaseExceptionObject *base_self = (PyBaseExceptionObject *)self;
    Py_XINCREF(base_self->traceback);
    return base_self->traceback;
}


int
PyException_SetTraceback(PyObject *self, PyObject *tb) {
    return BaseException_set_tb((PyBaseExceptionObject *)self, tb);
}

PyObject *
PyException_GetCause(PyObject *self) {
    PyObject *cause = ((PyBaseExceptionObject *)self)->cause;
    Py_XINCREF(cause);
    return cause;
}

/* Steals a reference to cause */
void
PyException_SetCause(PyObject *self, PyObject *cause) {
    PyObject *old_cause = ((PyBaseExceptionObject *)self)->cause;
    ((PyBaseExceptionObject *)self)->cause = cause;
    Py_XDECREF(old_cause);
}

PyObject *
PyException_GetContext(PyObject *self) {
    PyObject *context = ((PyBaseExceptionObject *)self)->context;
    Py_XINCREF(context);
    return context;
}

/* Steals a reference to context */
void
PyException_SetContext(PyObject *self, PyObject *context) {
    PyObject *old_context = ((PyBaseExceptionObject *)self)->context;
    ((PyBaseExceptionObject *)self)->context = context;
    Py_XDECREF(old_context);
}


static PyMemberDef BaseException_members[] = {
    {"__context__", T_OBJECT, offsetof(PyBaseExceptionObject, context), 0,
        PyDoc_STR("exception context")},
    {"__cause__", T_OBJECT, offsetof(PyBaseExceptionObject, cause), 0,
        PyDoc_STR("exception cause")},
    {NULL}  /* Sentinel */
};

static PyTypeObject _PyExc_BaseException = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "BaseException", /*tp_name*/
    sizeof(PyBaseExceptionObject), /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)BaseException_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /* tp_compare; */
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
from the previous implmentation and also allowing Python objects to be used
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
    (initproc)EXCSTORE ## _init, 0, BaseException_new,\
}; \
PyObject *PyExc_ ## EXCNAME = (PyObject *)&_PyExc_ ## EXCNAME

#define ComplexExtendsException(EXCBASE, EXCNAME, EXCSTORE, EXCDEALLOC, EXCMETHODS, EXCMEMBERS, EXCSTR, EXCDOC) \
static PyTypeObject _PyExc_ ## EXCNAME = { \
    PyVarObject_HEAD_INIT(NULL, 0) \
    # EXCNAME, \
    sizeof(Py ## EXCSTORE ## Object), 0, \
    (destructor)EXCSTORE ## _dealloc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    (reprfunc)EXCSTR, 0, 0, 0, \
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, \
    PyDoc_STR(EXCDOC), (traverseproc)EXCSTORE ## _traverse, \
    (inquiry)EXCSTORE ## _clear, 0, 0, 0, 0, EXCMETHODS, \
    EXCMEMBERS, 0, &_ ## EXCBASE, \
    0, 0, 0, offsetof(Py ## EXCSTORE ## Object, dict), \
    (initproc)EXCSTORE ## _init, 0, BaseException_new,\
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
 *    StopIteration extends Exception
 */
SimpleExtendsException(PyExc_Exception, StopIteration,
                       "Signal the end from iterator.__next__().");


/*
 *    GeneratorExit extends Exception
 */
SimpleExtendsException(PyExc_Exception, GeneratorExit,
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
    Py_CLEAR(self->code);
    if (size == 1)
        self->code = PyTuple_GET_ITEM(args, 0);
    else if (size > 1)
        self->code = args;
    Py_INCREF(self->code);
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
    Py_Type(self)->tp_free((PyObject *)self);
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
                        SystemExit_dealloc, 0, SystemExit_members, 0,
                        "Request to exit from the interpreter.");

/*
 *    KeyboardInterrupt extends BaseException
 */
SimpleExtendsException(PyExc_BaseException, KeyboardInterrupt,
                       "Program interrupted by user.");


/*
 *    ImportError extends Exception
 */
SimpleExtendsException(PyExc_Exception, ImportError,
          "Import can't find module, or can't find name in module.");


/*
 *    EnvironmentError extends Exception
 */

/* Where a function has a single filename, such as open() or some
 * of the os module functions, PyErr_SetFromErrnoWithFilename() is
 * called, giving a third argument which is the filename.  But, so
 * that old code using in-place unpacking doesn't break, e.g.:
 *
 * except IOError, (errno, strerror):
 *
 * we hack args so that it only contains two items.  This also
 * means we need our own __str__() which prints out the filename
 * when it was supplied.
 */
static int
EnvironmentError_init(PyEnvironmentErrorObject *self, PyObject *args,
    PyObject *kwds)
{
    PyObject *myerrno = NULL, *strerror = NULL, *filename = NULL;
    PyObject *subslice = NULL;

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    if (PyTuple_GET_SIZE(args) <= 1 || PyTuple_GET_SIZE(args) > 3) {
        return 0;
    }

    if (!PyArg_UnpackTuple(args, "EnvironmentError", 2, 3,
                           &myerrno, &strerror, &filename)) {
        return -1;
    }
    Py_CLEAR(self->myerrno);       /* replacing */
    self->myerrno = myerrno;
    Py_INCREF(self->myerrno);

    Py_CLEAR(self->strerror);      /* replacing */
    self->strerror = strerror;
    Py_INCREF(self->strerror);

    /* self->filename will remain Py_None otherwise */
    if (filename != NULL) {
        Py_CLEAR(self->filename);      /* replacing */
        self->filename = filename;
        Py_INCREF(self->filename);

        subslice = PyTuple_GetSlice(args, 0, 2);
        if (!subslice)
            return -1;

        Py_DECREF(self->args);  /* replacing args */
        self->args = subslice;
    }
    return 0;
}

static int
EnvironmentError_clear(PyEnvironmentErrorObject *self)
{
    Py_CLEAR(self->myerrno);
    Py_CLEAR(self->strerror);
    Py_CLEAR(self->filename);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
EnvironmentError_dealloc(PyEnvironmentErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    EnvironmentError_clear(self);
    Py_Type(self)->tp_free((PyObject *)self);
}

static int
EnvironmentError_traverse(PyEnvironmentErrorObject *self, visitproc visit,
        void *arg)
{
    Py_VISIT(self->myerrno);
    Py_VISIT(self->strerror);
    Py_VISIT(self->filename);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static PyObject *
EnvironmentError_str(PyEnvironmentErrorObject *self)
{
    if (self->filename)
        return PyUnicode_FromFormat("[Errno %S] %S: %R",
                                    self->myerrno ? self->myerrno: Py_None,
                                    self->strerror ? self->strerror: Py_None,
                                    self->filename);
    else if (self->myerrno && self->strerror)
        return PyUnicode_FromFormat("[Errno %S] %S",
                                    self->myerrno ? self->myerrno: Py_None,
                                    self->strerror ? self->strerror: Py_None);
    else
        return BaseException_str((PyBaseExceptionObject *)self);
}

static PyMemberDef EnvironmentError_members[] = {
    {"errno", T_OBJECT, offsetof(PyEnvironmentErrorObject, myerrno), 0,
        PyDoc_STR("exception errno")},
    {"strerror", T_OBJECT, offsetof(PyEnvironmentErrorObject, strerror), 0,
        PyDoc_STR("exception strerror")},
    {"filename", T_OBJECT, offsetof(PyEnvironmentErrorObject, filename), 0,
        PyDoc_STR("exception filename")},
    {NULL}  /* Sentinel */
};


static PyObject *
EnvironmentError_reduce(PyEnvironmentErrorObject *self)
{
    PyObject *args = self->args;
    PyObject *res = NULL, *tmp;

    /* self->args is only the first two real arguments if there was a
     * file name given to EnvironmentError. */
    if (PyTuple_GET_SIZE(args) == 2 && self->filename) {
        args = PyTuple_New(3);
        if (!args) return NULL;

        tmp = PyTuple_GET_ITEM(self->args, 0);
        Py_INCREF(tmp);
        PyTuple_SET_ITEM(args, 0, tmp);

        tmp = PyTuple_GET_ITEM(self->args, 1);
        Py_INCREF(tmp);
        PyTuple_SET_ITEM(args, 1, tmp);

        Py_INCREF(self->filename);
        PyTuple_SET_ITEM(args, 2, self->filename);
    } else
        Py_INCREF(args);

    if (self->dict)
        res = PyTuple_Pack(3, Py_Type(self), args, self->dict);
    else
        res = PyTuple_Pack(2, Py_Type(self), args);
    Py_DECREF(args);
    return res;
}


static PyMethodDef EnvironmentError_methods[] = {
    {"__reduce__", (PyCFunction)EnvironmentError_reduce, METH_NOARGS},
    {NULL}
};

ComplexExtendsException(PyExc_Exception, EnvironmentError,
                        EnvironmentError, EnvironmentError_dealloc,
                        EnvironmentError_methods, EnvironmentError_members,
                        EnvironmentError_str,
                        "Base class for I/O related errors.");


/*
 *    IOError extends EnvironmentError
 */
MiddlingExtendsException(PyExc_EnvironmentError, IOError,
                         EnvironmentError, "I/O operation failed.");


/*
 *    OSError extends EnvironmentError
 */
MiddlingExtendsException(PyExc_EnvironmentError, OSError,
                         EnvironmentError, "OS system call failed.");


/*
 *    WindowsError extends OSError
 */
#ifdef MS_WINDOWS
#include "errmap.h"

static int
WindowsError_clear(PyWindowsErrorObject *self)
{
    Py_CLEAR(self->myerrno);
    Py_CLEAR(self->strerror);
    Py_CLEAR(self->filename);
    Py_CLEAR(self->winerror);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
WindowsError_dealloc(PyWindowsErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    WindowsError_clear(self);
    Py_Type(self)->tp_free((PyObject *)self);
}

static int
WindowsError_traverse(PyWindowsErrorObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->myerrno);
    Py_VISIT(self->strerror);
    Py_VISIT(self->filename);
    Py_VISIT(self->winerror);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

static int
WindowsError_init(PyWindowsErrorObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *o_errcode = NULL;
    long errcode;
    long posix_errno;

    if (EnvironmentError_init((PyEnvironmentErrorObject *)self, args, kwds)
            == -1)
        return -1;

    if (self->myerrno == NULL)
        return 0;

    /* Set errno to the POSIX errno, and winerror to the Win32
       error code. */
    errcode = PyInt_AsLong(self->myerrno);
    if (errcode == -1 && PyErr_Occurred())
        return -1;
    posix_errno = winerror_to_errno(errcode);

    Py_CLEAR(self->winerror);
    self->winerror = self->myerrno;

    o_errcode = PyInt_FromLong(posix_errno);
    if (!o_errcode)
        return -1;

    self->myerrno = o_errcode;

    return 0;
}


static PyObject *
WindowsError_str(PyWindowsErrorObject *self)
{
    if (self->filename)
        return PyUnicode_FromFormat("[Error %S] %S: %R",
                                    self->winerror ? self->winerror: Py_None,
                                    self->strerror ? self->strerror: Py_None,
                                    self->filename);
    else if (self->winerror && self->strerror)
        return PyUnicode_FromFormat("[Error %S] %S",
                                    self->winerror ? self->winerror: Py_None,
                                    self->strerror ? self->strerror: Py_None);
    else
        return EnvironmentError_str((PyEnvironmentErrorObject *)self);
}

static PyMemberDef WindowsError_members[] = {
    {"errno", T_OBJECT, offsetof(PyWindowsErrorObject, myerrno), 0,
        PyDoc_STR("POSIX exception code")},
    {"strerror", T_OBJECT, offsetof(PyWindowsErrorObject, strerror), 0,
        PyDoc_STR("exception strerror")},
    {"filename", T_OBJECT, offsetof(PyWindowsErrorObject, filename), 0,
        PyDoc_STR("exception filename")},
    {"winerror", T_OBJECT, offsetof(PyWindowsErrorObject, winerror), 0,
        PyDoc_STR("Win32 exception code")},
    {NULL}  /* Sentinel */
};

ComplexExtendsException(PyExc_OSError, WindowsError, WindowsError,
                        WindowsError_dealloc, 0, WindowsError_members,
                        WindowsError_str, "MS-Windows OS system call failed.");

#endif /* MS_WINDOWS */


/*
 *    VMSError extends OSError (I think)
 */
#ifdef __VMS
MiddlingExtendsException(PyExc_OSError, VMSError, EnvironmentError,
                         "OpenVMS OS system call failed.");
#endif


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
 *    NotImplementedError extends RuntimeError
 */
SimpleExtendsException(PyExc_RuntimeError, NotImplementedError,
                       "Method or function hasn't been implemented yet.");

/*
 *    NameError extends Exception
 */
SimpleExtendsException(PyExc_Exception, NameError,
                       "Name not found globally.");

/*
 *    UnboundLocalError extends NameError
 */
SimpleExtendsException(PyExc_NameError, UnboundLocalError,
                       "Local name referenced but not bound to a value.");

/*
 *    AttributeError extends Exception
 */
SimpleExtendsException(PyExc_Exception, AttributeError,
                       "Attribute not found.");


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
        Py_CLEAR(self->msg);
        self->msg = PyTuple_GET_ITEM(args, 0);
        Py_INCREF(self->msg);
    }
    if (lenargs == 2) {
        info = PyTuple_GET_ITEM(args, 1);
        info = PySequence_Tuple(info);
        if (!info) return -1;

        if (PyTuple_GET_SIZE(info) != 4) {
            /* not a very good error message, but it's what Python 2.4 gives */
            PyErr_SetString(PyExc_IndexError, "tuple index out of range");
            Py_DECREF(info);
            return -1;
        }

        Py_CLEAR(self->filename);
        self->filename = PyTuple_GET_ITEM(info, 0);
        Py_INCREF(self->filename);

        Py_CLEAR(self->lineno);
        self->lineno = PyTuple_GET_ITEM(info, 1);
        Py_INCREF(self->lineno);

        Py_CLEAR(self->offset);
        self->offset = PyTuple_GET_ITEM(info, 2);
        Py_INCREF(self->offset);

        Py_CLEAR(self->text);
        self->text = PyTuple_GET_ITEM(info, 3);
        Py_INCREF(self->text);

        Py_DECREF(info);
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
    Py_CLEAR(self->text);
    Py_CLEAR(self->print_file_and_line);
    return BaseException_clear((PyBaseExceptionObject *)self);
}

static void
SyntaxError_dealloc(PySyntaxErrorObject *self)
{
    _PyObject_GC_UNTRACK(self);
    SyntaxError_clear(self);
    Py_Type(self)->tp_free((PyObject *)self);
}

static int
SyntaxError_traverse(PySyntaxErrorObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->msg);
    Py_VISIT(self->filename);
    Py_VISIT(self->lineno);
    Py_VISIT(self->offset);
    Py_VISIT(self->text);
    Py_VISIT(self->print_file_and_line);
    return BaseException_traverse((PyBaseExceptionObject *)self, visit, arg);
}

/* This is called "my_basename" instead of just "basename" to avoid name
   conflicts with glibc; basename is already prototyped if _GNU_SOURCE is
   defined, and Python does define that. */
static char *
my_basename(char *name)
{
    char *cp = name;
    char *result = name;

    if (name == NULL)
        return "???";
    while (*cp != '\0') {
        if (*cp == SEP)
            result = cp + 1;
        ++cp;
    }
    return result;
}


static PyObject *
SyntaxError_str(PySyntaxErrorObject *self)
{
    int have_lineno = 0;
    char *filename = 0;

    /* XXX -- do all the additional formatting with filename and
       lineno here */

    if (self->filename && PyUnicode_Check(self->filename)) {
	    filename = PyUnicode_AsString(self->filename);
    }
    have_lineno = (self->lineno != NULL) && PyInt_CheckExact(self->lineno);

    if (!filename && !have_lineno)
        return PyObject_Unicode(self->msg ? self->msg : Py_None);

    if (filename && have_lineno)
        return PyUnicode_FromFormat("%S (%s, line %ld)",
                   self->msg ? self->msg : Py_None,
                   my_basename(filename),
                   PyInt_AsLong(self->lineno));
    else if (filename)
        return PyUnicode_FromFormat("%S (%s)",
                   self->msg ? self->msg : Py_None,
                   my_basename(filename));
    else /* only have_lineno */
        return PyUnicode_FromFormat("%S (line %ld)",
                   self->msg ? self->msg : Py_None,
                   PyInt_AsLong(self->lineno));
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
    {"print_file_and_line", T_OBJECT,
        offsetof(PySyntaxErrorObject, print_file_and_line), 0,
        PyDoc_STR("exception print_file_and_line")},
    {NULL}  /* Sentinel */
};

ComplexExtendsException(PyExc_Exception, SyntaxError, SyntaxError,
                        SyntaxError_dealloc, 0, SyntaxError_members,
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
                        0, 0, 0, KeyError_str, "Mapping key not found.");


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

    if (!PyString_Check(attr)) {
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
    Py_CLEAR(*attr);
    *attr = obj;
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
    size = PyUnicode_GET_SIZE(obj);
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
    size = PyString_GET_SIZE(obj);
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
    size = PyUnicode_GET_SIZE(obj);
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
    size = PyString_GET_SIZE(obj);
    *end = ((PyUnicodeErrorObject *)exc)->end;
    if (*end<1)
        *end = 1;
    if (*end>size)
        *end = size;
    Py_DECREF(obj);
    return 0;
}


int
PyUnicodeTranslateError_GetEnd(PyObject *exc, Py_ssize_t *start)
{
    return PyUnicodeEncodeError_GetEnd(exc, start);
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
    Py_Type(self)->tp_free((PyObject *)self);
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

    if (!PyArg_ParseTuple(args, "O!O!nnO!",
        &PyUnicode_Type, &err->encoding,
        &PyUnicode_Type, &err->object,
        &err->start,
        &err->end,
        &PyUnicode_Type, &err->reason)) {
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

    if (uself->end==uself->start+1) {
        int badchar = (int)PyUnicode_AS_UNICODE(uself->object)[uself->start];
        const char *fmt;
        if (badchar <= 0xff)
            fmt = "'%U' codec can't encode character '\\x%02x' in position %zd: %U";
        else if (badchar <= 0xffff)
            fmt = "'%U' codec can't encode character '\\u%04x' in position %zd: %U";
        else
            fmt = "'%U' codec can't encode character '\\U%08x' in position %zd: %U";
        return PyUnicode_FromFormat(
            fmt,
            ((PyUnicodeErrorObject *)self)->encoding,
            badchar,
            uself->start,
            ((PyUnicodeErrorObject *)self)->reason
        );
    }
    return PyUnicode_FromFormat(
        "'%U' codec can't encode characters in position %zd-%zd: %U",
        ((PyUnicodeErrorObject *)self)->encoding,
        uself->start,
        uself->end-1,
        ((PyUnicodeErrorObject *)self)->reason
    );
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

PyObject *
PyUnicodeEncodeError_Create(
    const char *encoding, const Py_UNICODE *object, Py_ssize_t length,
    Py_ssize_t start, Py_ssize_t end, const char *reason)
{
    return PyObject_CallFunction(PyExc_UnicodeEncodeError, "Uu#nnU",
                                 encoding, object, length, start, end, reason);
}


/*
 *    UnicodeDecodeError extends UnicodeError
 */

static int
UnicodeDecodeError_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyUnicodeErrorObject *ude;
    const char *data;
    Py_ssize_t size;

    if (BaseException_init((PyBaseExceptionObject *)self, args, kwds) == -1)
        return -1;

    ude = (PyUnicodeErrorObject *)self;

    Py_CLEAR(ude->encoding);
    Py_CLEAR(ude->object);
    Py_CLEAR(ude->reason);

    if (!PyArg_ParseTuple(args, "O!OnnO!",
         &PyUnicode_Type, &ude->encoding,
         &ude->object,
         &ude->start,
         &ude->end,
         &PyUnicode_Type, &ude->reason)) {
             ude->encoding = ude->object = ude->reason = NULL;
             return -1;
    }

    if (!PyString_Check(ude->object)) {
        if (PyObject_AsReadBuffer(ude->object, (const void **)&data, &size)) {
            ude->encoding = ude->object = ude->reason = NULL;
            return -1;
        }
        ude->object = PyString_FromStringAndSize(data, size);
    }
    else {
        Py_INCREF(ude->object);
    }

    Py_INCREF(ude->encoding);
    Py_INCREF(ude->reason);

    return 0;
}

static PyObject *
UnicodeDecodeError_str(PyObject *self)
{
    PyUnicodeErrorObject *uself = (PyUnicodeErrorObject *)self;

    if (uself->end==uself->start+1) {
        int byte = (int)(PyString_AS_STRING(((PyUnicodeErrorObject *)self)->object)[uself->start]&0xff);
        return PyUnicode_FromFormat(
            "'%U' codec can't decode byte 0x%02x in position %zd: %U",
            ((PyUnicodeErrorObject *)self)->encoding,
            byte,
            uself->start,
            ((PyUnicodeErrorObject *)self)->reason
        );
    }
    return PyUnicode_FromFormat(
        "'%U' codec can't decode bytes in position %zd-%zd: %U",
        ((PyUnicodeErrorObject *)self)->encoding,
        uself->start,
        uself->end-1,
        ((PyUnicodeErrorObject *)self)->reason
    );
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
    assert(length < INT_MAX);
    assert(start < INT_MAX);
    assert(end < INT_MAX);
    return PyObject_CallFunction(PyExc_UnicodeDecodeError, "Uy#nnU",
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

    if (!PyArg_ParseTuple(args, "O!nnO!",
        &PyUnicode_Type, &self->object,
        &self->start,
        &self->end,
        &PyUnicode_Type, &self->reason)) {
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

    if (uself->end==uself->start+1) {
        int badchar = (int)PyUnicode_AS_UNICODE(uself->object)[uself->start];
        const char *fmt;
        if (badchar <= 0xff)
            fmt = "can't translate character '\\x%02x' in position %zd: %U";
        else if (badchar <= 0xffff)
            fmt = "can't translate character '\\u%04x' in position %zd: %U";
        else
            fmt = "can't translate character '\\U%08x' in position %zd: %U";
        return PyUnicode_FromFormat(
            fmt,
            badchar,
            uself->start,
            uself->reason
        );
    }
    return PyUnicode_FromFormat(
        "can't translate characters in position %zd-%zd: %U",
        uself->start,
        uself->end-1,
        uself->reason
    );
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
PyUnicodeTranslateError_Create(
    const Py_UNICODE *object, Py_ssize_t length,
    Py_ssize_t start, Py_ssize_t end, const char *reason)
{
    return PyObject_CallFunction(PyExc_UnicodeTranslateError, "u#nns",
                                 object, length, start, end, reason);
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
SimpleExtendsException(PyExc_Exception, MemoryError, "Out of memory.");

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



/* Pre-computed MemoryError instance.  Best to create this as early as
 * possible and not wait until a MemoryError is actually raised!
 */
PyObject *PyExc_MemoryErrorInst=NULL;

/* Pre-computed RuntimeError instance for when recursion depth is reached.
   Meant to be used when normalizing the exception for exceeding the recursion
   depth will cause its own infinite recursion.
*/
PyObject *PyExc_RecursionErrorInst = NULL;

#define PRE_INIT(TYPE) if (PyType_Ready(&_PyExc_ ## TYPE) < 0) \
    Py_FatalError("exceptions bootstrapping error.");

#define POST_INIT(TYPE) Py_INCREF(PyExc_ ## TYPE); \
    if (PyDict_SetItemString(bdict, # TYPE, PyExc_ ## TYPE)) \
        Py_FatalError("Module dictionary insertion problem.");

#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
/* crt variable checking in VisualStudio .NET 2005 */
#include <crtdbg.h>

static int	prevCrtReportMode;
static _invalid_parameter_handler	prevCrtHandler;

/* Invalid parameter handler.  Sets a ValueError exception */
static void
InvalidParameterHandler(
    const wchar_t * expression,
    const wchar_t * function,
    const wchar_t * file,
    unsigned int line,
    uintptr_t pReserved)
{
    /* Do nothing, allow execution to continue.  Usually this
     * means that the CRT will set errno to EINVAL
     */
}
#endif


PyMODINIT_FUNC
_PyExc_Init(void)
{
    PyObject *bltinmod, *bdict;

    PRE_INIT(BaseException)
    PRE_INIT(Exception)
    PRE_INIT(TypeError)
    PRE_INIT(StopIteration)
    PRE_INIT(GeneratorExit)
    PRE_INIT(SystemExit)
    PRE_INIT(KeyboardInterrupt)
    PRE_INIT(ImportError)
    PRE_INIT(EnvironmentError)
    PRE_INIT(IOError)
    PRE_INIT(OSError)
#ifdef MS_WINDOWS
    PRE_INIT(WindowsError)
#endif
#ifdef __VMS
    PRE_INIT(VMSError)
#endif
    PRE_INIT(EOFError)
    PRE_INIT(RuntimeError)
    PRE_INIT(NotImplementedError)
    PRE_INIT(NameError)
    PRE_INIT(UnboundLocalError)
    PRE_INIT(AttributeError)
    PRE_INIT(SyntaxError)
    PRE_INIT(IndentationError)
    PRE_INIT(TabError)
    PRE_INIT(LookupError)
    PRE_INIT(IndexError)
    PRE_INIT(KeyError)
    PRE_INIT(ValueError)
    PRE_INIT(UnicodeError)
    PRE_INIT(UnicodeEncodeError)
    PRE_INIT(UnicodeDecodeError)
    PRE_INIT(UnicodeTranslateError)
    PRE_INIT(AssertionError)
    PRE_INIT(ArithmeticError)
    PRE_INIT(FloatingPointError)
    PRE_INIT(OverflowError)
    PRE_INIT(ZeroDivisionError)
    PRE_INIT(SystemError)
    PRE_INIT(ReferenceError)
    PRE_INIT(BufferError)
    PRE_INIT(MemoryError)
    PRE_INIT(Warning)
    PRE_INIT(UserWarning)
    PRE_INIT(DeprecationWarning)
    PRE_INIT(PendingDeprecationWarning)
    PRE_INIT(SyntaxWarning)
    PRE_INIT(RuntimeWarning)
    PRE_INIT(FutureWarning)
    PRE_INIT(ImportWarning)
    PRE_INIT(UnicodeWarning)
    PRE_INIT(BytesWarning)

    bltinmod = PyImport_ImportModule("__builtin__");
    if (bltinmod == NULL)
        Py_FatalError("exceptions bootstrapping error.");
    bdict = PyModule_GetDict(bltinmod);
    if (bdict == NULL)
        Py_FatalError("exceptions bootstrapping error.");

    POST_INIT(BaseException)
    POST_INIT(Exception)
    POST_INIT(TypeError)
    POST_INIT(StopIteration)
    POST_INIT(GeneratorExit)
    POST_INIT(SystemExit)
    POST_INIT(KeyboardInterrupt)
    POST_INIT(ImportError)
    POST_INIT(EnvironmentError)
    POST_INIT(IOError)
    POST_INIT(OSError)
#ifdef MS_WINDOWS
    POST_INIT(WindowsError)
#endif
#ifdef __VMS
    POST_INIT(VMSError)
#endif
    POST_INIT(EOFError)
    POST_INIT(RuntimeError)
    POST_INIT(NotImplementedError)
    POST_INIT(NameError)
    POST_INIT(UnboundLocalError)
    POST_INIT(AttributeError)
    POST_INIT(SyntaxError)
    POST_INIT(IndentationError)
    POST_INIT(TabError)
    POST_INIT(LookupError)
    POST_INIT(IndexError)
    POST_INIT(KeyError)
    POST_INIT(ValueError)
    POST_INIT(UnicodeError)
    POST_INIT(UnicodeEncodeError)
    POST_INIT(UnicodeDecodeError)
    POST_INIT(UnicodeTranslateError)
    POST_INIT(AssertionError)
    POST_INIT(ArithmeticError)
    POST_INIT(FloatingPointError)
    POST_INIT(OverflowError)
    POST_INIT(ZeroDivisionError)
    POST_INIT(SystemError)
    POST_INIT(ReferenceError)
    POST_INIT(BufferError)
    POST_INIT(MemoryError)
    POST_INIT(Warning)
    POST_INIT(UserWarning)
    POST_INIT(DeprecationWarning)
    POST_INIT(PendingDeprecationWarning)
    POST_INIT(SyntaxWarning)
    POST_INIT(RuntimeWarning)
    POST_INIT(FutureWarning)
    POST_INIT(ImportWarning)
    POST_INIT(UnicodeWarning)
    POST_INIT(BytesWarning)

    PyExc_MemoryErrorInst = BaseException_new(&_PyExc_MemoryError, NULL, NULL);
    if (!PyExc_MemoryErrorInst)
        Py_FatalError("Cannot pre-allocate MemoryError instance\n");

    PyExc_RecursionErrorInst = BaseException_new(&_PyExc_RuntimeError, NULL, NULL);
    if (!PyExc_RecursionErrorInst)
	Py_FatalError("Cannot pre-allocate RuntimeError instance for "
			"recursion errors");
    else {
	PyBaseExceptionObject *err_inst =
	    (PyBaseExceptionObject *)PyExc_RecursionErrorInst;
	PyObject *args_tuple;
	PyObject *exc_message;
	exc_message = PyUnicode_FromString("maximum recursion depth exceeded");
	if (!exc_message)
	    Py_FatalError("cannot allocate argument for RuntimeError "
			    "pre-allocation");
	args_tuple = PyTuple_Pack(1, exc_message);
	if (!args_tuple)
	    Py_FatalError("cannot allocate tuple for RuntimeError "
			    "pre-allocation");
	Py_DECREF(exc_message);
	if (BaseException_init(err_inst, args_tuple, NULL))
	    Py_FatalError("init of pre-allocated RuntimeError failed");
	Py_DECREF(args_tuple);
    }

    Py_DECREF(bltinmod);

#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
    /* Set CRT argument error handler */
    prevCrtHandler = _set_invalid_parameter_handler(InvalidParameterHandler);
    /* turn off assertions in debug mode */
    prevCrtReportMode = _CrtSetReportMode(_CRT_ASSERT, 0);
#endif
}

void
_PyExc_Fini(void)
{
    Py_XDECREF(PyExc_MemoryErrorInst);
    PyExc_MemoryErrorInst = NULL;
#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
    /* reset CRT error handling */
    _set_invalid_parameter_handler(prevCrtHandler);
    _CrtSetReportMode(_CRT_ASSERT, prevCrtReportMode);
#endif
}
