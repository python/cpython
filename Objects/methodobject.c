
/* Method object implementation */

#include "Python.h"
#include "pycore_ceval.h"         // _Py_EnterRecursiveCall()
#include "pycore_object.h"
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "structmember.h"         // PyMemberDef

/* undefine macro trampoline to PyCFunction_NewEx */
#undef PyCFunction_New

/* Forward declarations */
static PyObject * cfunction_vectorcall_FASTCALL(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames);
static PyObject * cfunction_vectorcall_FASTCALL_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames);
static PyObject * cfunction_vectorcall_NOARGS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames);
static PyObject * cfunction_vectorcall_O(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames);
static PyObject * cfunction_call(
    PyObject *func, PyObject *args, PyObject *kwargs);


PyObject *
PyCFunction_New(PyMethodDef *ml, PyObject *self)
{
    return PyCFunction_NewEx(ml, self, NULL);
}

PyObject *
PyCFunction_NewEx(PyMethodDef *ml, PyObject *self, PyObject *module)
{
    /* Figure out correct vectorcall function to use */
    vectorcallfunc vectorcall;
    switch (ml->ml_flags & (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O | METH_KEYWORDS))
    {
        case METH_VARARGS:
        case METH_VARARGS | METH_KEYWORDS:
            /* For METH_VARARGS functions, it's more efficient to use tp_call
             * instead of vectorcall. */
            vectorcall = NULL;
            break;
        case METH_FASTCALL:
            vectorcall = cfunction_vectorcall_FASTCALL;
            break;
        case METH_FASTCALL | METH_KEYWORDS:
            vectorcall = cfunction_vectorcall_FASTCALL_KEYWORDS;
            break;
        case METH_NOARGS:
            vectorcall = cfunction_vectorcall_NOARGS;
            break;
        case METH_O:
            vectorcall = cfunction_vectorcall_O;
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "%s() method: bad call flags", ml->ml_name);
            return NULL;
    }

    PyCFunctionObject *op =
        PyObject_GC_New(PyCFunctionObject, &PyCFunction_Type);
    if (op == NULL) {
        return NULL;
    }
    op->m_weakreflist = NULL;
    op->m_ml = ml;
    Py_XINCREF(self);
    op->m_self = self;
    Py_XINCREF(module);
    op->m_module = module;
    op->vectorcall = vectorcall;
    _PyObject_GC_TRACK(op);
    return (PyObject *)op;
}

PyCFunction
PyCFunction_GetFunction(PyObject *op)
{
    if (!PyCFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyCFunction_GET_FUNCTION(op);
}

PyObject *
PyCFunction_GetSelf(PyObject *op)
{
    if (!PyCFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyCFunction_GET_SELF(op);
}

int
PyCFunction_GetFlags(PyObject *op)
{
    if (!PyCFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return PyCFunction_GET_FLAGS(op);
}

/* Methods (the standard built-in methods, that is) */

static void
meth_dealloc(PyCFunctionObject *m)
{
    _PyObject_GC_UNTRACK(m);
    if (m->m_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*) m);
    }
    Py_XDECREF(m->m_self);
    Py_XDECREF(m->m_module);
    PyObject_GC_Del(m);
}

static PyObject *
meth_reduce(PyCFunctionObject *m, PyObject *Py_UNUSED(ignored))
{
    _Py_IDENTIFIER(getattr);

    if (m->m_self == NULL || PyModule_Check(m->m_self))
        return PyUnicode_FromString(m->m_ml->ml_name);

    return Py_BuildValue("N(Os)", _PyEval_GetBuiltinId(&PyId_getattr),
                         m->m_self, m->m_ml->ml_name);
}

static PyMethodDef meth_methods[] = {
    {"__reduce__", (PyCFunction)meth_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyObject *
meth_get__text_signature__(PyCFunctionObject *m, void *closure)
{
    return _PyType_GetTextSignatureFromInternalDoc(m->m_ml->ml_name, m->m_ml->ml_doc);
}

static PyObject *
meth_get__doc__(PyCFunctionObject *m, void *closure)
{
    return _PyType_GetDocFromInternalDoc(m->m_ml->ml_name, m->m_ml->ml_doc);
}

static PyObject *
meth_get__name__(PyCFunctionObject *m, void *closure)
{
    return PyUnicode_FromString(m->m_ml->ml_name);
}

static PyObject *
meth_get__qualname__(PyCFunctionObject *m, void *closure)
{
    /* If __self__ is a module or NULL, return m.__name__
       (e.g. len.__qualname__ == 'len')

       If __self__ is a type, return m.__self__.__qualname__ + '.' + m.__name__
       (e.g. dict.fromkeys.__qualname__ == 'dict.fromkeys')

       Otherwise return type(m.__self__).__qualname__ + '.' + m.__name__
       (e.g. [].append.__qualname__ == 'list.append') */
    PyObject *type, *type_qualname, *res;
    _Py_IDENTIFIER(__qualname__);

    if (m->m_self == NULL || PyModule_Check(m->m_self))
        return PyUnicode_FromString(m->m_ml->ml_name);

    type = PyType_Check(m->m_self) ? m->m_self : (PyObject*)Py_TYPE(m->m_self);

    type_qualname = _PyObject_GetAttrId(type, &PyId___qualname__);
    if (type_qualname == NULL)
        return NULL;

    if (!PyUnicode_Check(type_qualname)) {
        PyErr_SetString(PyExc_TypeError, "<method>.__class__."
                        "__qualname__ is not a unicode object");
        Py_XDECREF(type_qualname);
        return NULL;
    }

    res = PyUnicode_FromFormat("%S.%s", type_qualname, m->m_ml->ml_name);
    Py_DECREF(type_qualname);
    return res;
}

static int
meth_traverse(PyCFunctionObject *m, visitproc visit, void *arg)
{
    Py_VISIT(m->m_self);
    Py_VISIT(m->m_module);
    return 0;
}

static PyObject *
meth_get__self__(PyCFunctionObject *m, void *closure)
{
    PyObject *self;

    self = PyCFunction_GET_SELF(m);
    if (self == NULL)
        self = Py_None;
    Py_INCREF(self);
    return self;
}

static PyGetSetDef meth_getsets [] = {
    {"__doc__",  (getter)meth_get__doc__,  NULL, NULL},
    {"__name__", (getter)meth_get__name__, NULL, NULL},
    {"__qualname__", (getter)meth_get__qualname__, NULL, NULL},
    {"__self__", (getter)meth_get__self__, NULL, NULL},
    {"__text_signature__", (getter)meth_get__text_signature__, NULL, NULL},
    {0}
};

#define OFF(x) offsetof(PyCFunctionObject, x)

static PyMemberDef meth_members[] = {
    {"__module__",    T_OBJECT,     OFF(m_module), 0},
    {NULL}
};

static PyObject *
meth_repr(PyCFunctionObject *m)
{
    if (m->m_self == NULL || PyModule_Check(m->m_self))
        return PyUnicode_FromFormat("<built-in function %s>",
                                   m->m_ml->ml_name);
    return PyUnicode_FromFormat("<built-in method %s of %s object at %p>",
                               m->m_ml->ml_name,
                               Py_TYPE(m->m_self)->tp_name,
                               m->m_self);
}

static PyObject *
meth_richcompare(PyObject *self, PyObject *other, int op)
{
    PyCFunctionObject *a, *b;
    PyObject *res;
    int eq;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyCFunction_Check(self) ||
        !PyCFunction_Check(other))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    a = (PyCFunctionObject *)self;
    b = (PyCFunctionObject *)other;
    eq = a->m_self == b->m_self;
    if (eq)
        eq = a->m_ml->ml_meth == b->m_ml->ml_meth;
    if (op == Py_EQ)
        res = eq ? Py_True : Py_False;
    else
        res = eq ? Py_False : Py_True;
    Py_INCREF(res);
    return res;
}

static Py_hash_t
meth_hash(PyCFunctionObject *a)
{
    Py_hash_t x, y;
    x = _Py_HashPointer(a->m_self);
    y = _Py_HashPointer((void*)(a->m_ml->ml_meth));
    x ^= y;
    if (x == -1)
        x = -2;
    return x;
}


PyTypeObject PyCFunction_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "builtin_function_or_method",
    sizeof(PyCFunctionObject),
    0,
    (destructor)meth_dealloc,                   /* tp_dealloc */
    offsetof(PyCFunctionObject, vectorcall),    /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)meth_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)meth_hash,                        /* tp_hash */
    cfunction_call,                             /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_HAVE_VECTORCALL,                 /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)meth_traverse,                /* tp_traverse */
    0,                                          /* tp_clear */
    meth_richcompare,                           /* tp_richcompare */
    offsetof(PyCFunctionObject, m_weakreflist), /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    meth_methods,                               /* tp_methods */
    meth_members,                               /* tp_members */
    meth_getsets,                               /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
};

/* Vectorcall functions for each of the PyCFunction calling conventions,
 * except for METH_VARARGS (possibly combined with METH_KEYWORDS) which
 * doesn't use vectorcall.
 *
 * First, common helpers
 */

static inline int
cfunction_check_kwargs(PyThreadState *tstate, PyObject *func, PyObject *kwnames)
{
    assert(!_PyErr_Occurred(tstate));
    assert(PyCFunction_Check(func));
    if (kwnames && PyTuple_GET_SIZE(kwnames)) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            _PyErr_Format(tstate, PyExc_TypeError,
                         "%U takes no keyword arguments", funcstr);
            Py_DECREF(funcstr);
        }
        return -1;
    }
    return 0;
}

typedef void (*funcptr)(void);

static inline funcptr
cfunction_enter_call(PyThreadState *tstate, PyObject *func)
{
    if (_Py_EnterRecursiveCall(tstate, " while calling a Python object")) {
        return NULL;
    }
    return (funcptr)PyCFunction_GET_FUNCTION(func);
}

/* Now the actual vectorcall functions */
static PyObject *
cfunction_vectorcall_FASTCALL(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (cfunction_check_kwargs(tstate, func, kwnames)) {
        return NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    _PyCFunctionFast meth = (_PyCFunctionFast)
                            cfunction_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(PyCFunction_GET_SELF(func), args, nargs);
    _Py_LeaveRecursiveCall(tstate);
    return result;
}

static PyObject *
cfunction_vectorcall_FASTCALL_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    _PyCFunctionFastWithKeywords meth = (_PyCFunctionFastWithKeywords)
                                        cfunction_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(PyCFunction_GET_SELF(func), args, nargs, kwnames);
    _Py_LeaveRecursiveCall(tstate);
    return result;
}

static PyObject *
cfunction_vectorcall_NOARGS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (cfunction_check_kwargs(tstate, func, kwnames)) {
        return NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 0) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            _PyErr_Format(tstate, PyExc_TypeError,
                "%U takes no arguments (%zd given)", funcstr, nargs);
            Py_DECREF(funcstr);
        }
        return NULL;
    }
    PyCFunction meth = (PyCFunction)cfunction_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(PyCFunction_GET_SELF(func), NULL);
    _Py_LeaveRecursiveCall(tstate);
    return result;
}

static PyObject *
cfunction_vectorcall_O(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (cfunction_check_kwargs(tstate, func, kwnames)) {
        return NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (nargs != 1) {
        PyObject *funcstr = _PyObject_FunctionStr(func);
        if (funcstr != NULL) {
            _PyErr_Format(tstate, PyExc_TypeError,
                "%U takes exactly one argument (%zd given)", funcstr, nargs);
            Py_DECREF(funcstr);
        }
        return NULL;
    }
    PyCFunction meth = (PyCFunction)cfunction_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(PyCFunction_GET_SELF(func), args[0]);
    _Py_LeaveRecursiveCall(tstate);
    return result;
}


static PyObject *
cfunction_call(PyObject *func, PyObject *args, PyObject *kwargs)
{
    assert(kwargs == NULL || PyDict_Check(kwargs));

    PyThreadState *tstate = _PyThreadState_GET();
    assert(!_PyErr_Occurred(tstate));

    int flags = PyCFunction_GET_FLAGS(func);
    if (!(flags & METH_VARARGS)) {
        /* If this is not a METH_VARARGS function, delegate to vectorcall */
        return PyVectorcall_Call(func, args, kwargs);
    }

    /* For METH_VARARGS, we cannot use vectorcall as the vectorcall pointer
     * is NULL. This is intentional, since vectorcall would be slower. */
    PyCFunction meth = PyCFunction_GET_FUNCTION(func);
    PyObject *self = PyCFunction_GET_SELF(func);

    PyObject *result;
    if (flags & METH_KEYWORDS) {
        result = (*(PyCFunctionWithKeywords)(void(*)(void))meth)(self, args, kwargs);
    }
    else {
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%.200s() takes no keyword arguments",
                          ((PyCFunctionObject*)func)->m_ml->ml_name);
            return NULL;
        }
        result = meth(self, args);
    }
    return _Py_CheckFunctionResult(tstate, func, result, NULL);
}
