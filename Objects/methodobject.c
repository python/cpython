
/* Method object implementation */

#include "Python.h"
#include "pycore_call.h"          // _Py_CheckFunctionResult()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_freelist.h"
#include "pycore_object.h"
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()


/* undefine macro trampoline to PyCFunction_NewEx */
#undef PyCFunction_New
/* undefine macro trampoline to PyCMethod_New */
#undef PyCFunction_NewEx

/* Forward declarations */
static PyObject * cfunction_vectorcall_FASTCALL(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames);
static PyObject * cfunction_vectorcall_FASTCALL_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames);
static PyObject * cfunction_vectorcall_FASTCALL_KEYWORDS_METHOD(
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
    return PyCMethod_New(ml, self, module, NULL);
}

PyObject *
PyCMethod_New(PyMethodDef *ml, PyObject *self, PyObject *module, PyTypeObject *cls)
{
    /* Figure out correct vectorcall function to use */
    vectorcallfunc vectorcall;
    switch (ml->ml_flags & (METH_VARARGS | METH_FASTCALL | METH_NOARGS |
                            METH_O | METH_KEYWORDS | METH_METHOD))
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
        case METH_METHOD | METH_FASTCALL | METH_KEYWORDS:
            vectorcall = cfunction_vectorcall_FASTCALL_KEYWORDS_METHOD;
            break;
        default:
            PyErr_Format(PyExc_SystemError,
                         "%s() method: bad call flags", ml->ml_name);
            return NULL;
    }

    PyCFunctionObject *op = NULL;

    if (ml->ml_flags & METH_METHOD) {
        if (!cls) {
            PyErr_SetString(PyExc_SystemError,
                            "attempting to create PyCMethod with a METH_METHOD "
                            "flag but no class");
            return NULL;
        }
        PyCMethodObject *om = _Py_FREELIST_POP(PyCMethodObject, pycmethodobject);
        if (om == NULL) {
            om = PyObject_GC_New(PyCMethodObject, &PyCMethod_Type);
            if (om == NULL) {
                return NULL;
            }
        }
        om->mm_class = (PyTypeObject*)Py_NewRef(cls);
        op = (PyCFunctionObject *)om;
    } else {
        if (cls) {
            PyErr_SetString(PyExc_SystemError,
                            "attempting to create PyCFunction with class "
                            "but no METH_METHOD flag");
            return NULL;
        }
        op = _Py_FREELIST_POP(PyCFunctionObject, pycfunctionobject);
        if (op == NULL) {
            op = PyObject_GC_New(PyCFunctionObject, &PyCFunction_Type);
            if (op == NULL) {
                return NULL;
            }
        }
    }

    op->m_weakreflist = NULL;
    op->m_ml = ml;
    op->m_self = Py_XNewRef(self);
    op->m_module = Py_XNewRef(module);
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

PyTypeObject *
PyCMethod_GetClass(PyObject *op)
{
    if (!PyCFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyCFunction_GET_CLASS(op);
}

/* Methods (the standard built-in methods, that is) */

static void
meth_dealloc(PyObject *self)
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    PyObject_GC_UnTrack(m);
    if (m->m_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*) m);
    }
    // We need to access ml_flags here rather than later.
    // `m->m_ml` might have the same lifetime
    // as `m_self` when it's dynamically allocated.
    int ml_flags = m->m_ml->ml_flags;
    // Dereference class before m_self: PyCFunction_GET_CLASS accesses
    // PyMethodDef m_ml, which could be kept alive by m_self
    Py_XDECREF(PyCFunction_GET_CLASS(m));
    Py_XDECREF(m->m_self);
    Py_XDECREF(m->m_module);
    if (ml_flags & METH_METHOD) {
        assert(Py_IS_TYPE(self, &PyCMethod_Type));
        _Py_FREELIST_FREE(pycmethodobject, m, PyObject_GC_Del);
    }
    else {
        assert(Py_IS_TYPE(self, &PyCFunction_Type));
        _Py_FREELIST_FREE(pycfunctionobject, m, PyObject_GC_Del);
    }
}

static PyObject *
meth_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    if (m->m_self == NULL || PyModule_Check(m->m_self))
        return PyUnicode_FromString(m->m_ml->ml_name);

    return Py_BuildValue("N(Os)", _PyEval_GetBuiltin(&_Py_ID(getattr)),
                         m->m_self, m->m_ml->ml_name);
}

static PyMethodDef meth_methods[] = {
    {"__reduce__", meth_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyObject *
meth_get__text_signature__(PyObject *self, void *closure)
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    return _PyType_GetTextSignatureFromInternalDoc(m->m_ml->ml_name,
                                                   m->m_ml->ml_doc,
                                                   m->m_ml->ml_flags);
}

static PyObject *
meth_get__doc__(PyObject *self, void *closure)
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    return _PyType_GetDocFromInternalDoc(m->m_ml->ml_name, m->m_ml->ml_doc);
}

static PyObject *
meth_get__name__(PyObject *self, void *closure)
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    return PyUnicode_FromString(m->m_ml->ml_name);
}

static PyObject *
meth_get__qualname__(PyObject *self, void *closure)
{
    /* If __self__ is a module or NULL, return m.__name__
       (e.g. len.__qualname__ == 'len')

       If __self__ is a type, return m.__self__.__qualname__ + '.' + m.__name__
       (e.g. dict.fromkeys.__qualname__ == 'dict.fromkeys')

       Otherwise return type(m.__self__).__qualname__ + '.' + m.__name__
       (e.g. [].append.__qualname__ == 'list.append') */

    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    if (m->m_self == NULL || PyModule_Check(m->m_self)) {
        return PyUnicode_FromString(m->m_ml->ml_name);
    }

    PyObject *type = PyType_Check(m->m_self) ? m->m_self : (PyObject*)Py_TYPE(m->m_self);

    PyObject *type_qualname = PyObject_GetAttr(type, &_Py_ID(__qualname__));
    if (type_qualname == NULL)
        return NULL;

    if (!PyUnicode_Check(type_qualname)) {
        PyErr_SetString(PyExc_TypeError, "<method>.__class__."
                        "__qualname__ is not a unicode object");
        Py_XDECREF(type_qualname);
        return NULL;
    }

    PyObject *res = PyUnicode_FromFormat("%S.%s", type_qualname, m->m_ml->ml_name);
    Py_DECREF(type_qualname);
    return res;
}

static int
meth_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    Py_VISIT(PyCFunction_GET_CLASS(m));
    Py_VISIT(m->m_self);
    Py_VISIT(m->m_module);
    return 0;
}

static PyObject *
meth_get__self__(PyObject *meth, void *closure)
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(meth);
    PyObject *self = PyCFunction_GET_SELF(m);
    if (self == NULL) {
        self = Py_None;
    }
    return Py_NewRef(self);
}

static PyGetSetDef meth_getsets[] = {
    {"__doc__",  meth_get__doc__,  NULL, NULL},
    {"__name__", meth_get__name__, NULL, NULL},
    {"__qualname__", meth_get__qualname__, NULL, NULL},
    {"__self__", meth_get__self__, NULL, NULL},
    {"__text_signature__", meth_get__text_signature__, NULL, NULL},
    {0}
};

#define OFF(x) offsetof(PyCFunctionObject, x)

static PyMemberDef meth_members[] = {
    {"__module__",    _Py_T_OBJECT,     OFF(m_module), 0},
    {NULL}
};

static PyObject *
meth_repr(PyObject *self)
{
    PyCFunctionObject *m = _PyCFunctionObject_CAST(self);
    if (m->m_self == NULL || PyModule_Check(m->m_self)) {
        return PyUnicode_FromFormat("<built-in function %s>",
                                    m->m_ml->ml_name);
    }

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
    return Py_NewRef(res);
}

static Py_hash_t
meth_hash(PyObject *self)
{
    PyCFunctionObject *a = _PyCFunctionObject_CAST(self);
    Py_hash_t x = PyObject_GenericHash(a->m_self);
    Py_hash_t y = Py_HashPointer((void*)(a->m_ml->ml_meth));
    x ^= y;
    if (x == -1) {
        x = -2;
    }
    return x;
}


PyTypeObject PyCFunction_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "builtin_function_or_method",
    sizeof(PyCFunctionObject),
    0,
    meth_dealloc,                               /* tp_dealloc */
    offsetof(PyCFunctionObject, vectorcall),    /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    meth_repr,                                  /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    meth_hash,                                  /* tp_hash */
    cfunction_call,                             /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
    Py_TPFLAGS_HAVE_VECTORCALL,                 /* tp_flags */
    0,                                          /* tp_doc */
    meth_traverse,                /* tp_traverse */
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

PyTypeObject PyCMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "builtin_method",
    .tp_basicsize = sizeof(PyCMethodObject),
    .tp_base = &PyCFunction_Type,
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
    if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
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
    PyCFunctionFast meth = (PyCFunctionFast)
                            cfunction_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(PyCFunction_GET_SELF(func), args, nargs);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
cfunction_vectorcall_FASTCALL_KEYWORDS(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyCFunctionFastWithKeywords meth = (PyCFunctionFastWithKeywords)
                                        cfunction_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(PyCFunction_GET_SELF(func), args, nargs, kwnames);
    _Py_LeaveRecursiveCallTstate(tstate);
    return result;
}

static PyObject *
cfunction_vectorcall_FASTCALL_KEYWORDS_METHOD(
    PyObject *func, PyObject *const *args, size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyTypeObject *cls = PyCFunction_GET_CLASS(func);
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyCMethod meth = (PyCMethod)cfunction_enter_call(tstate, func);
    if (meth == NULL) {
        return NULL;
    }
    PyObject *result = meth(PyCFunction_GET_SELF(func), cls, args, nargs, kwnames);
    _Py_LeaveRecursiveCallTstate(tstate);
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
    PyObject *result = _PyCFunction_TrampolineCall(
        meth, PyCFunction_GET_SELF(func), NULL);
    _Py_LeaveRecursiveCallTstate(tstate);
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
    PyObject *result = _PyCFunction_TrampolineCall(
        meth, PyCFunction_GET_SELF(func), args[0]);
    _Py_LeaveRecursiveCallTstate(tstate);
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
        result = _PyCFunctionWithKeywords_TrampolineCall(
            *_PyCFunctionWithKeywords_CAST(meth),
            self, args, kwargs);
    }
    else {
        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%.200s() takes no keyword arguments",
                          ((PyCFunctionObject*)func)->m_ml->ml_name);
            return NULL;
        }
        result = _PyCFunction_TrampolineCall(meth, self, args);
    }
    return _Py_CheckFunctionResult(tstate, func, result, NULL);
}
