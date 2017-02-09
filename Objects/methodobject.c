
/* Method object implementation */

#include "Python.h"
#include "structmember.h"

/* Free list for method objects to safe malloc/free overhead
 * The m_self element is used to chain the objects.
 */
static PyCFunctionObject *free_list = NULL;
static int numfree = 0;
#ifndef PyCFunction_MAXFREELIST
#define PyCFunction_MAXFREELIST 256
#endif

/* undefine macro trampoline to PyCFunction_NewEx */
#undef PyCFunction_New

PyAPI_FUNC(PyObject *)
PyCFunction_New(PyMethodDef *ml, PyObject *self)
{
    return PyCFunction_NewEx(ml, self, NULL);
}

PyObject *
PyCFunction_NewEx(PyMethodDef *ml, PyObject *self, PyObject *module)
{
    PyCFunctionObject *op;
    op = free_list;
    if (op != NULL) {
        free_list = (PyCFunctionObject *)(op->m_self);
        (void)PyObject_INIT(op, &PyCFunction_Type);
        numfree--;
    }
    else {
        op = PyObject_GC_New(PyCFunctionObject, &PyCFunction_Type);
        if (op == NULL)
            return NULL;
    }
    op->m_weakreflist = NULL;
    op->m_ml = ml;
    Py_XINCREF(self);
    op->m_self = self;
    Py_XINCREF(module);
    op->m_module = module;
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

static PyObject *
cfunction_call_varargs(PyObject *func, PyObject *args, PyObject *kwargs)
{
    assert(!PyErr_Occurred());

    PyCFunction meth = PyCFunction_GET_FUNCTION(func);
    PyObject *self = PyCFunction_GET_SELF(func);
    PyObject *result;

    if (PyCFunction_GET_FLAGS(func) & METH_KEYWORDS) {
        if (Py_EnterRecursiveCall(" while calling a Python object")) {
            return NULL;
        }

        result = (*(PyCFunctionWithKeywords)meth)(self, args, kwargs);

        Py_LeaveRecursiveCall();
    }
    else {
        if (kwargs != NULL && PyDict_Size(kwargs) != 0) {
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
    /* first try METH_VARARGS to pass directly args tuple unchanged.
       _PyMethodDef_RawFastCallDict() creates a new temporary tuple
       for METH_VARARGS. */
    if (PyCFunction_GET_FLAGS(func) & METH_VARARGS) {
        return cfunction_call_varargs(func, args, kwargs);
    }
    else {
        return _PyCFunction_FastCallDict(func,
                                         &PyTuple_GET_ITEM(args, 0),
                                         PyTuple_GET_SIZE(args),
                                         kwargs);
    }
}

PyObject *
_PyMethodDef_RawFastCallDict(PyMethodDef *method, PyObject *self, PyObject **args,
                             Py_ssize_t nargs, PyObject *kwargs)
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
        if (nargs != 0) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes no arguments (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            goto no_keyword_error;
        }

        result = (*meth) (self, NULL);
        break;

    case METH_O:
        if (nargs != 1) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes exactly one argument (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        if (kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            goto no_keyword_error;
        }

        result = (*meth) (self, args[0]);
        break;

    case METH_VARARGS:
        if (!(flags & METH_KEYWORDS)
                && kwargs != NULL && PyDict_GET_SIZE(kwargs) != 0) {
            goto no_keyword_error;
        }
        /* fall through next case */

    case METH_VARARGS | METH_KEYWORDS:
    {
        /* Slow-path: create a temporary tuple for positional arguments */
        PyObject *argstuple = _PyStack_AsTuple(args, nargs);
        if (argstuple == NULL) {
            goto exit;
        }

        if (flags & METH_KEYWORDS) {
            result = (*(PyCFunctionWithKeywords)meth) (self, argstuple, kwargs);
        }
        else {
            result = (*meth) (self, argstuple);
        }
        Py_DECREF(argstuple);
        break;
    }

    case METH_FASTCALL:
    {
        PyObject **stack;
        PyObject *kwnames;
        _PyCFunctionFast fastmeth = (_PyCFunctionFast)meth;

        if (_PyStack_UnpackDict(args, nargs, kwargs, &stack, &kwnames) < 0) {
            goto exit;
        }

        result = (*fastmeth) (self, stack, nargs, kwnames);
        if (stack != args) {
            PyMem_Free(stack);
        }
        Py_XDECREF(kwnames);
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
                 method->ml_name, nargs);

exit:
    Py_LeaveRecursiveCall();
    return result;
}

PyObject *
_PyCFunction_FastCallDict(PyObject *func, PyObject **args, Py_ssize_t nargs,
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
_PyMethodDef_RawFastCallKeywords(PyMethodDef *method, PyObject *self, PyObject **args,
                                 Py_ssize_t nargs, PyObject *kwnames)
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
    Py_ssize_t nkwargs = kwnames == NULL ? 0 : PyTuple_Size(kwnames);
    PyObject *result = NULL;

    if (Py_EnterRecursiveCall(" while calling a Python object")) {
        return NULL;
    }

    switch (flags)
    {
    case METH_NOARGS:
        if (nargs != 0) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes no arguments (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        if (nkwargs) {
            goto no_keyword_error;
        }

        result = (*meth) (self, NULL);
        break;

    case METH_O:
        if (nargs != 1) {
            PyErr_Format(PyExc_TypeError,
                "%.200s() takes exactly one argument (%zd given)",
                method->ml_name, nargs);
            goto exit;
        }

        if (nkwargs) {
            goto no_keyword_error;
        }

        result = (*meth) (self, args[0]);
        break;

    case METH_FASTCALL:
        /* Fast-path: avoid temporary dict to pass keyword arguments */
        result = ((_PyCFunctionFast)meth) (self, args, nargs, kwnames);
        break;

    case METH_VARARGS:
    case METH_VARARGS | METH_KEYWORDS:
    {
        /* Slow-path: create a temporary tuple for positional arguments
           and a temporary dict for keyword arguments */
        PyObject *argtuple;

        if (!(flags & METH_KEYWORDS) && nkwargs) {
            goto no_keyword_error;
        }

        argtuple = _PyStack_AsTuple(args, nargs);
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

            result = (*(PyCFunctionWithKeywords)meth) (self, argtuple, kwdict);
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
                        "Bad call flags in _PyCFunction_FastCallKeywords. "
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
_PyCFunction_FastCallKeywords(PyObject *func, PyObject **args,
                              Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *result;

    assert(func != NULL);
    assert(PyCFunction_Check(func));

    result = _PyMethodDef_RawFastCallKeywords(((PyCFunctionObject*)func)->m_ml,
                                              PyCFunction_GET_SELF(func),
                                              args, nargs, kwnames);
    result = _Py_CheckFunctionResult(func, result, NULL);
    return result;
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
    if (numfree < PyCFunction_MAXFREELIST) {
        m->m_self = (PyObject *)free_list;
        free_list = m;
        numfree++;
    }
    else {
        PyObject_GC_Del(m);
    }
}

static PyObject *
meth_reduce(PyCFunctionObject *m)
{
    PyObject *builtins;
    PyObject *getattr;
    _Py_IDENTIFIER(getattr);

    if (m->m_self == NULL || PyModule_Check(m->m_self))
        return PyUnicode_FromString(m->m_ml->ml_name);

    builtins = PyEval_GetBuiltins();
    getattr = _PyDict_GetItemId(builtins, &PyId_getattr);
    return Py_BuildValue("O(Os)", getattr, m->m_self, m->m_ml->ml_name);
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
    {"__module__",    T_OBJECT,     OFF(m_module), PY_WRITE_RESTRICTED},
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
                               m->m_self->ob_type->tp_name,
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
    if (a->m_self == NULL)
        x = 0;
    else {
        x = PyObject_Hash(a->m_self);
        if (x == -1)
            return -1;
    }
    y = _Py_HashPointer((void*)(a->m_ml->ml_meth));
    if (y == -1)
        return -1;
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
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)meth_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)meth_hash,                        /* tp_hash */
    PyCFunction_Call,                           /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
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

/* Clear out the free list */

int
PyCFunction_ClearFreeList(void)
{
    int freelist_size = numfree;

    while (free_list) {
        PyCFunctionObject *v = free_list;
        free_list = (PyCFunctionObject *)(v->m_self);
        PyObject_GC_Del(v);
        numfree--;
    }
    assert(numfree == 0);
    return freelist_size;
}

void
PyCFunction_Fini(void)
{
    (void)PyCFunction_ClearFreeList();
}

/* Print summary info about the state of the optimized allocator */
void
_PyCFunction_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out,
                           "free PyCFunctionObject",
                           numfree, sizeof(PyCFunctionObject));
}
