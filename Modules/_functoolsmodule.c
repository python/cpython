#include "Python.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "structmember.h"         // PyMemberDef

/* _functools module written and maintained
   by Hye-Shik Chang <perky@FreeBSD.org>
   with adaptations by Raymond Hettinger <python@rcn.com>
   Copyright (c) 2004, 2005, 2006 Python Software Foundation.
   All rights reserved.
*/

/* partial object **********************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *fn;
    PyObject *args;
    PyObject *kw;
    PyObject *dict;        /* __dict__ */
    PyObject *weakreflist; /* List of weak references */
    vectorcallfunc vectorcall;
} partialobject;

static PyTypeObject partial_type;

static void partial_setvectorcall(partialobject *pto);

static PyObject *
partial_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *func, *pargs, *nargs, *pkw;
    partialobject *pto;

    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError,
                        "type 'partial' takes at least one argument");
        return NULL;
    }

    pargs = pkw = NULL;
    func = PyTuple_GET_ITEM(args, 0);
    if (Py_IS_TYPE(func, &partial_type) && type == &partial_type) {
        partialobject *part = (partialobject *)func;
        if (part->dict == NULL) {
            pargs = part->args;
            pkw = part->kw;
            func = part->fn;
            assert(PyTuple_Check(pargs));
            assert(PyDict_Check(pkw));
        }
    }
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "the first argument must be callable");
        return NULL;
    }

    /* create partialobject structure */
    pto = (partialobject *)type->tp_alloc(type, 0);
    if (pto == NULL)
        return NULL;

    pto->fn = func;
    Py_INCREF(func);

    nargs = PyTuple_GetSlice(args, 1, PY_SSIZE_T_MAX);
    if (nargs == NULL) {
        Py_DECREF(pto);
        return NULL;
    }
    if (pargs == NULL) {
        pto->args = nargs;
    }
    else {
        pto->args = PySequence_Concat(pargs, nargs);
        Py_DECREF(nargs);
        if (pto->args == NULL) {
            Py_DECREF(pto);
            return NULL;
        }
        assert(PyTuple_Check(pto->args));
    }

    if (pkw == NULL || PyDict_GET_SIZE(pkw) == 0) {
        if (kw == NULL) {
            pto->kw = PyDict_New();
        }
        else if (Py_REFCNT(kw) == 1) {
            Py_INCREF(kw);
            pto->kw = kw;
        }
        else {
            pto->kw = PyDict_Copy(kw);
        }
    }
    else {
        pto->kw = PyDict_Copy(pkw);
        if (kw != NULL && pto->kw != NULL) {
            if (PyDict_Merge(pto->kw, kw, 1) != 0) {
                Py_DECREF(pto);
                return NULL;
            }
        }
    }
    if (pto->kw == NULL) {
        Py_DECREF(pto);
        return NULL;
    }

    partial_setvectorcall(pto);
    return (PyObject *)pto;
}

static void
partial_dealloc(partialobject *pto)
{
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(pto);
    if (pto->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) pto);
    Py_XDECREF(pto->fn);
    Py_XDECREF(pto->args);
    Py_XDECREF(pto->kw);
    Py_XDECREF(pto->dict);
    Py_TYPE(pto)->tp_free(pto);
}


/* Merging keyword arguments using the vectorcall convention is messy, so
 * if we would need to do that, we stop using vectorcall and fall back
 * to using partial_call() instead. */
_Py_NO_INLINE static PyObject *
partial_vectorcall_fallback(PyThreadState *tstate, partialobject *pto,
                            PyObject *const *args, size_t nargsf,
                            PyObject *kwnames)
{
    pto->vectorcall = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    return _PyObject_MakeTpCall(tstate, (PyObject *)pto,
                                args, nargs, kwnames);
}

static PyObject *
partial_vectorcall(partialobject *pto, PyObject *const *args,
                   size_t nargsf, PyObject *kwnames)
{
    PyThreadState *tstate = _PyThreadState_GET();

    /* pto->kw is mutable, so need to check every time */
    if (PyDict_GET_SIZE(pto->kw)) {
        return partial_vectorcall_fallback(tstate, pto, args, nargsf, kwnames);
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    Py_ssize_t nargs_total = nargs;
    if (kwnames != NULL) {
        nargs_total += PyTuple_GET_SIZE(kwnames);
    }

    PyObject **pto_args = _PyTuple_ITEMS(pto->args);
    Py_ssize_t pto_nargs = PyTuple_GET_SIZE(pto->args);

    /* Fast path if we're called without arguments */
    if (nargs_total == 0) {
        return _PyObject_VectorcallTstate(tstate, pto->fn,
                                          pto_args, pto_nargs, NULL);
    }

    /* Fast path using PY_VECTORCALL_ARGUMENTS_OFFSET to prepend a single
     * positional argument */
    if (pto_nargs == 1 && (nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET)) {
        PyObject **newargs = (PyObject **)args - 1;
        PyObject *tmp = newargs[0];
        newargs[0] = pto_args[0];
        PyObject *ret = _PyObject_VectorcallTstate(tstate, pto->fn,
                                                   newargs, nargs + 1, kwnames);
        newargs[0] = tmp;
        return ret;
    }

    Py_ssize_t newnargs_total = pto_nargs + nargs_total;

    PyObject *small_stack[_PY_FASTCALL_SMALL_STACK];
    PyObject *ret;
    PyObject **stack;

    if (newnargs_total <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc(newnargs_total * sizeof(PyObject *));
        if (stack == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    /* Copy to new stack, using borrowed references */
    memcpy(stack, pto_args, pto_nargs * sizeof(PyObject*));
    memcpy(stack + pto_nargs, args, nargs_total * sizeof(PyObject*));

    ret = _PyObject_VectorcallTstate(tstate, pto->fn,
                                     stack, pto_nargs + nargs, kwnames);
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return ret;
}

/* Set pto->vectorcall depending on the parameters of the partial object */
static void
partial_setvectorcall(partialobject *pto)
{
    if (PyVectorcall_Function(pto->fn) == NULL) {
        /* Don't use vectorcall if the underlying function doesn't support it */
        pto->vectorcall = NULL;
    }
    /* We could have a special case if there are no arguments,
     * but that is unlikely (why use partial without arguments?),
     * so we don't optimize that */
    else {
        pto->vectorcall = (vectorcallfunc)partial_vectorcall;
    }
}


static PyObject *
partial_call(partialobject *pto, PyObject *args, PyObject *kwargs)
{
    assert(PyCallable_Check(pto->fn));
    assert(PyTuple_Check(pto->args));
    assert(PyDict_Check(pto->kw));

    /* Merge keywords */
    PyObject *kwargs2;
    if (PyDict_GET_SIZE(pto->kw) == 0) {
        /* kwargs can be NULL */
        kwargs2 = kwargs;
        Py_XINCREF(kwargs2);
    }
    else {
        /* bpo-27840, bpo-29318: dictionary of keyword parameters must be
           copied, because a function using "**kwargs" can modify the
           dictionary. */
        kwargs2 = PyDict_Copy(pto->kw);
        if (kwargs2 == NULL) {
            return NULL;
        }

        if (kwargs != NULL) {
            if (PyDict_Merge(kwargs2, kwargs, 1) != 0) {
                Py_DECREF(kwargs2);
                return NULL;
            }
        }
    }

    /* Merge positional arguments */
    /* Note: tupleconcat() is optimized for empty tuples */
    PyObject *args2 = PySequence_Concat(pto->args, args);
    if (args2 == NULL) {
        Py_XDECREF(kwargs2);
        return NULL;
    }

    PyObject *res = PyObject_Call(pto->fn, args2, kwargs2);
    Py_DECREF(args2);
    Py_XDECREF(kwargs2);
    return res;
}

static int
partial_traverse(partialobject *pto, visitproc visit, void *arg)
{
    Py_VISIT(pto->fn);
    Py_VISIT(pto->args);
    Py_VISIT(pto->kw);
    Py_VISIT(pto->dict);
    return 0;
}

PyDoc_STRVAR(partial_doc,
"partial(func, *args, **keywords) - new function with partial application\n\
    of the given arguments and keywords.\n");

#define OFF(x) offsetof(partialobject, x)
static PyMemberDef partial_memberlist[] = {
    {"func",            T_OBJECT,       OFF(fn),        READONLY,
     "function object to use in future partial calls"},
    {"args",            T_OBJECT,       OFF(args),      READONLY,
     "tuple of arguments to future partial calls"},
    {"keywords",        T_OBJECT,       OFF(kw),        READONLY,
     "dictionary of keyword arguments to future partial calls"},
    {NULL}  /* Sentinel */
};

static PyGetSetDef partial_getsetlist[] = {
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {NULL} /* Sentinel */
};

static PyObject *
partial_repr(partialobject *pto)
{
    PyObject *result = NULL;
    PyObject *arglist;
    Py_ssize_t i, n;
    PyObject *key, *value;
    int status;

    status = Py_ReprEnter((PyObject *)pto);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromString("...");
    }

    arglist = PyUnicode_FromString("");
    if (arglist == NULL)
        goto done;
    /* Pack positional arguments */
    assert (PyTuple_Check(pto->args));
    n = PyTuple_GET_SIZE(pto->args);
    for (i = 0; i < n; i++) {
        Py_SETREF(arglist, PyUnicode_FromFormat("%U, %R", arglist,
                                        PyTuple_GET_ITEM(pto->args, i)));
        if (arglist == NULL)
            goto done;
    }
    /* Pack keyword arguments */
    assert (PyDict_Check(pto->kw));
    for (i = 0; PyDict_Next(pto->kw, &i, &key, &value);) {
        /* Prevent key.__str__ from deleting the value. */
        Py_INCREF(value);
        Py_SETREF(arglist, PyUnicode_FromFormat("%U, %S=%R", arglist,
                                                key, value));
        Py_DECREF(value);
        if (arglist == NULL)
            goto done;
    }
    result = PyUnicode_FromFormat("%s(%R%U)", Py_TYPE(pto)->tp_name,
                                  pto->fn, arglist);
    Py_DECREF(arglist);

 done:
    Py_ReprLeave((PyObject *)pto);
    return result;
}

/* Pickle strategy:
   __reduce__ by itself doesn't support getting kwargs in the unpickle
   operation so we define a __setstate__ that replaces all the information
   about the partial.  If we only replaced part of it someone would use
   it as a hook to do strange things.
 */

static PyObject *
partial_reduce(partialobject *pto, PyObject *unused)
{
    return Py_BuildValue("O(O)(OOOO)", Py_TYPE(pto), pto->fn, pto->fn,
                         pto->args, pto->kw,
                         pto->dict ? pto->dict : Py_None);
}

static PyObject *
partial_setstate(partialobject *pto, PyObject *state)
{
    PyObject *fn, *fnargs, *kw, *dict;

    if (!PyTuple_Check(state) ||
        !PyArg_ParseTuple(state, "OOOO", &fn, &fnargs, &kw, &dict) ||
        !PyCallable_Check(fn) ||
        !PyTuple_Check(fnargs) ||
        (kw != Py_None && !PyDict_Check(kw)))
    {
        PyErr_SetString(PyExc_TypeError, "invalid partial state");
        return NULL;
    }

    if(!PyTuple_CheckExact(fnargs))
        fnargs = PySequence_Tuple(fnargs);
    else
        Py_INCREF(fnargs);
    if (fnargs == NULL)
        return NULL;

    if (kw == Py_None)
        kw = PyDict_New();
    else if(!PyDict_CheckExact(kw))
        kw = PyDict_Copy(kw);
    else
        Py_INCREF(kw);
    if (kw == NULL) {
        Py_DECREF(fnargs);
        return NULL;
    }

    if (dict == Py_None)
        dict = NULL;
    else
        Py_INCREF(dict);

    Py_INCREF(fn);
    Py_SETREF(pto->fn, fn);
    Py_SETREF(pto->args, fnargs);
    Py_SETREF(pto->kw, kw);
    Py_XSETREF(pto->dict, dict);
    partial_setvectorcall(pto);
    Py_RETURN_NONE;
}

static PyMethodDef partial_methods[] = {
    {"__reduce__", (PyCFunction)partial_reduce, METH_NOARGS},
    {"__setstate__", (PyCFunction)partial_setstate, METH_O},
    {"__class_getitem__",    (PyCFunction)Py_GenericAlias,
    METH_O|METH_CLASS,       PyDoc_STR("See PEP 585")},
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject partial_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "functools.partial",                /* tp_name */
    sizeof(partialobject),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)partial_dealloc,        /* tp_dealloc */
    offsetof(partialobject, vectorcall),/* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)partial_repr,             /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)partial_call,          /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_HAVE_VECTORCALL,     /* tp_flags */
    partial_doc,                        /* tp_doc */
    (traverseproc)partial_traverse,     /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    offsetof(partialobject, weakreflist),       /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    partial_methods,                    /* tp_methods */
    partial_memberlist,                 /* tp_members */
    partial_getsetlist,                 /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    offsetof(partialobject, dict),      /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    partial_new,                        /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
};


/* cmp_to_key ***************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *cmp;
    PyObject *object;
} keyobject;

static void
keyobject_dealloc(keyobject *ko)
{
    Py_DECREF(ko->cmp);
    Py_XDECREF(ko->object);
    PyObject_FREE(ko);
}

static int
keyobject_traverse(keyobject *ko, visitproc visit, void *arg)
{
    Py_VISIT(ko->cmp);
    Py_VISIT(ko->object);
    return 0;
}

static int
keyobject_clear(keyobject *ko)
{
    Py_CLEAR(ko->cmp);
    if (ko->object)
        Py_CLEAR(ko->object);
    return 0;
}

static PyMemberDef keyobject_members[] = {
    {"obj", T_OBJECT,
     offsetof(keyobject, object), 0,
     PyDoc_STR("Value wrapped by a key function.")},
    {NULL}
};

static PyObject *
keyobject_call(keyobject *ko, PyObject *args, PyObject *kwds);

static PyObject *
keyobject_richcompare(PyObject *ko, PyObject *other, int op);

static PyTypeObject keyobject_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "functools.KeyWrapper",             /* tp_name */
    sizeof(keyobject),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)keyobject_dealloc,      /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)keyobject_call,        /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /* tp_doc */
    (traverseproc)keyobject_traverse,   /* tp_traverse */
    (inquiry)keyobject_clear,           /* tp_clear */
    keyobject_richcompare,              /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    keyobject_members,                  /* tp_members */
    0,                                  /* tp_getset */
};

static PyObject *
keyobject_call(keyobject *ko, PyObject *args, PyObject *kwds)
{
    PyObject *object;
    keyobject *result;
    static char *kwargs[] = {"obj", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:K", kwargs, &object))
        return NULL;
    result = PyObject_New(keyobject, &keyobject_type);
    if (!result)
        return NULL;
    Py_INCREF(ko->cmp);
    result->cmp = ko->cmp;
    Py_INCREF(object);
    result->object = object;
    return (PyObject *)result;
}

static PyObject *
keyobject_richcompare(PyObject *ko, PyObject *other, int op)
{
    PyObject *res;
    PyObject *x;
    PyObject *y;
    PyObject *compare;
    PyObject *answer;
    PyObject* stack[2];

    if (!Py_IS_TYPE(other, &keyobject_type)) {
        PyErr_Format(PyExc_TypeError, "other argument must be K instance");
        return NULL;
    }
    compare = ((keyobject *) ko)->cmp;
    assert(compare != NULL);
    x = ((keyobject *) ko)->object;
    y = ((keyobject *) other)->object;
    if (!x || !y){
        PyErr_Format(PyExc_AttributeError, "object");
        return NULL;
    }

    /* Call the user's comparison function and translate the 3-way
     * result into true or false (or error).
     */
    stack[0] = x;
    stack[1] = y;
    res = _PyObject_FastCall(compare, stack, 2);
    if (res == NULL) {
        return NULL;
    }

    answer = PyObject_RichCompare(res, _PyLong_Zero, op);
    Py_DECREF(res);
    return answer;
}

static PyObject *
functools_cmp_to_key(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *cmp;
    static char *kwargs[] = {"mycmp", NULL};
    keyobject *object;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:cmp_to_key", kwargs, &cmp))
        return NULL;
    object = PyObject_New(keyobject, &keyobject_type);
    if (!object)
        return NULL;
    Py_INCREF(cmp);
    object->cmp = cmp;
    object->object = NULL;
    return (PyObject *)object;
}

PyDoc_STRVAR(functools_cmp_to_key_doc,
"Convert a cmp= function into a key= function.");

/* reduce (used to be a builtin) ********************************************/

static PyObject *
functools_reduce(PyObject *self, PyObject *args)
{
    PyObject *seq, *func, *result = NULL, *it;

    if (!PyArg_UnpackTuple(args, "reduce", 2, 3, &func, &seq, &result))
        return NULL;
    if (result != NULL)
        Py_INCREF(result);

    it = PyObject_GetIter(seq);
    if (it == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError))
            PyErr_SetString(PyExc_TypeError,
                            "reduce() arg 2 must support iteration");
        Py_XDECREF(result);
        return NULL;
    }

    if ((args = PyTuple_New(2)) == NULL)
        goto Fail;

    for (;;) {
        PyObject *op2;

        if (Py_REFCNT(args) > 1) {
            Py_DECREF(args);
            if ((args = PyTuple_New(2)) == NULL)
                goto Fail;
        }

        op2 = PyIter_Next(it);
        if (op2 == NULL) {
            if (PyErr_Occurred())
                goto Fail;
            break;
        }

        if (result == NULL)
            result = op2;
        else {
            /* Update the args tuple in-place */
            assert(Py_REFCNT(args) == 1);
            Py_XSETREF(_PyTuple_ITEMS(args)[0], result);
            Py_XSETREF(_PyTuple_ITEMS(args)[1], op2);
            if ((result = PyObject_Call(func, args, NULL)) == NULL) {
                goto Fail;
            }
        }
    }

    Py_DECREF(args);

    if (result == NULL)
        PyErr_SetString(PyExc_TypeError,
                   "reduce() of empty iterable with no initial value");

    Py_DECREF(it);
    return result;

Fail:
    Py_XDECREF(args);
    Py_XDECREF(result);
    Py_DECREF(it);
    return NULL;
}

PyDoc_STRVAR(functools_reduce_doc,
"reduce(function, iterable[, initial]) -> value\n\
\n\
Apply a function of two arguments cumulatively to the items of a sequence\n\
or iterable, from left to right, so as to reduce the iterable to a single\n\
value.  For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates\n\
((((1+2)+3)+4)+5).  If initial is present, it is placed before the items\n\
of the iterable in the calculation, and serves as a default when the\n\
iterable is empty.");

/* lru_cache object **********************************************************/

/* There are four principal algorithmic differences from the pure python version:

   1). The C version relies on the GIL instead of having its own reentrant lock.

   2). The prev/next link fields use borrowed references.

   3). For a full cache, the pure python version rotates the location of the
       root entry so that it never has to move individual links and it can
       limit updates to just the key and result fields.  However, in the C
       version, links are temporarily removed while the cache dict updates are
       occurring. Afterwards, they are appended or prepended back into the
       doubly-linked lists.

   4)  In the Python version, the _HashSeq class is used to prevent __hash__
       from being called more than once.  In the C version, the "known hash"
       variants of dictionary calls as used to the same effect.

*/


/* this object is used delimit args and keywords in the cache keys */
static PyObject *kwd_mark = NULL;

struct lru_list_elem;
struct lru_cache_object;

typedef struct lru_list_elem {
    PyObject_HEAD
    struct lru_list_elem *prev, *next;  /* borrowed links */
    Py_hash_t hash;
    PyObject *key, *result;
} lru_list_elem;

static void
lru_list_elem_dealloc(lru_list_elem *link)
{
    Py_XDECREF(link->key);
    Py_XDECREF(link->result);
    PyObject_Del(link);
}

static PyTypeObject lru_list_elem_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "functools._lru_list_elem",         /* tp_name */
    sizeof(lru_list_elem),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)lru_list_elem_dealloc,  /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
};


typedef PyObject *(*lru_cache_ternaryfunc)(struct lru_cache_object *, PyObject *, PyObject *);

typedef struct lru_cache_object {
    lru_list_elem root;  /* includes PyObject_HEAD */
    lru_cache_ternaryfunc wrapper;
    int typed;
    PyObject *cache;
    Py_ssize_t hits;
    PyObject *func;
    Py_ssize_t maxsize;
    Py_ssize_t misses;
    PyObject *cache_info_type;
    PyObject *dict;
    PyObject *weakreflist;
} lru_cache_object;

static PyTypeObject lru_cache_type;

static PyObject *
lru_cache_make_key(PyObject *args, PyObject *kwds, int typed)
{
    PyObject *key, *keyword, *value;
    Py_ssize_t key_size, pos, key_pos, kwds_size;

    kwds_size = kwds ? PyDict_GET_SIZE(kwds) : 0;

    /* short path, key will match args anyway, which is a tuple */
    if (!typed && !kwds_size) {
        if (PyTuple_GET_SIZE(args) == 1) {
            key = PyTuple_GET_ITEM(args, 0);
            if (PyUnicode_CheckExact(key) || PyLong_CheckExact(key)) {
                /* For common scalar keys, save space by
                   dropping the enclosing args tuple  */
                Py_INCREF(key);
                return key;
            }
        }
        Py_INCREF(args);
        return args;
    }

    key_size = PyTuple_GET_SIZE(args);
    if (kwds_size)
        key_size += kwds_size * 2 + 1;
    if (typed)
        key_size += PyTuple_GET_SIZE(args) + kwds_size;

    key = PyTuple_New(key_size);
    if (key == NULL)
        return NULL;

    key_pos = 0;
    for (pos = 0; pos < PyTuple_GET_SIZE(args); ++pos) {
        PyObject *item = PyTuple_GET_ITEM(args, pos);
        Py_INCREF(item);
        PyTuple_SET_ITEM(key, key_pos++, item);
    }
    if (kwds_size) {
        Py_INCREF(kwd_mark);
        PyTuple_SET_ITEM(key, key_pos++, kwd_mark);
        for (pos = 0; PyDict_Next(kwds, &pos, &keyword, &value);) {
            Py_INCREF(keyword);
            PyTuple_SET_ITEM(key, key_pos++, keyword);
            Py_INCREF(value);
            PyTuple_SET_ITEM(key, key_pos++, value);
        }
        assert(key_pos == PyTuple_GET_SIZE(args) + kwds_size * 2 + 1);
    }
    if (typed) {
        for (pos = 0; pos < PyTuple_GET_SIZE(args); ++pos) {
            PyObject *item = (PyObject *)Py_TYPE(PyTuple_GET_ITEM(args, pos));
            Py_INCREF(item);
            PyTuple_SET_ITEM(key, key_pos++, item);
        }
        if (kwds_size) {
            for (pos = 0; PyDict_Next(kwds, &pos, &keyword, &value);) {
                PyObject *item = (PyObject *)Py_TYPE(value);
                Py_INCREF(item);
                PyTuple_SET_ITEM(key, key_pos++, item);
            }
        }
    }
    assert(key_pos == key_size);
    return key;
}

static PyObject *
uncached_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    PyObject *result;

    self->misses++;
    result = PyObject_Call(self->func, args, kwds);
    if (!result)
        return NULL;
    return result;
}

static PyObject *
infinite_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    PyObject *result;
    Py_hash_t hash;
    PyObject *key = lru_cache_make_key(args, kwds, self->typed);
    if (!key)
        return NULL;
    hash = PyObject_Hash(key);
    if (hash == -1) {
        Py_DECREF(key);
        return NULL;
    }
    result = _PyDict_GetItem_KnownHash(self->cache, key, hash);
    if (result) {
        Py_INCREF(result);
        self->hits++;
        Py_DECREF(key);
        return result;
    }
    if (PyErr_Occurred()) {
        Py_DECREF(key);
        return NULL;
    }
    self->misses++;
    result = PyObject_Call(self->func, args, kwds);
    if (!result) {
        Py_DECREF(key);
        return NULL;
    }
    if (_PyDict_SetItem_KnownHash(self->cache, key, result, hash) < 0) {
        Py_DECREF(result);
        Py_DECREF(key);
        return NULL;
    }
    Py_DECREF(key);
    return result;
}

static void
lru_cache_extract_link(lru_list_elem *link)
{
    lru_list_elem *link_prev = link->prev;
    lru_list_elem *link_next = link->next;
    link_prev->next = link->next;
    link_next->prev = link->prev;
}

static void
lru_cache_append_link(lru_cache_object *self, lru_list_elem *link)
{
    lru_list_elem *root = &self->root;
    lru_list_elem *last = root->prev;
    last->next = root->prev = link;
    link->prev = last;
    link->next = root;
}

static void
lru_cache_prepend_link(lru_cache_object *self, lru_list_elem *link)
{
    lru_list_elem *root = &self->root;
    lru_list_elem *first = root->next;
    first->prev = root->next = link;
    link->prev = root;
    link->next = first;
}

/* General note on reentrancy:

   There are four dictionary calls in the bounded_lru_cache_wrapper():
   1) The initial check for a cache match.  2) The post user-function
   check for a cache match.  3) The deletion of the oldest entry.
   4) The addition of the newest entry.

   In all four calls, we have a known hash which lets use avoid a call
   to __hash__().  That leaves only __eq__ as a possible source of a
   reentrant call.

   The __eq__ method call is always made for a cache hit (dict access #1).
   Accordingly, we have make sure not modify the cache state prior to
   this call.

   The __eq__ method call is never made for the deletion (dict access #3)
   because it is an identity match.

   For the other two accesses (#2 and #4), calls to __eq__ only occur
   when some other entry happens to have an exactly matching hash (all
   64-bits).  Though rare, this can happen, so we have to make sure to
   either call it at the top of its code path before any cache
   state modifications (dict access #2) or be prepared to restore
   invariants at the end of the code path (dict access #4).

   Another possible source of reentrancy is a decref which can trigger
   arbitrary code execution.  To make the code easier to reason about,
   the decrefs are deferred to the end of the each possible code path
   so that we know the cache is a consistent state.
 */

static PyObject *
bounded_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    lru_list_elem *link;
    PyObject *key, *result, *testresult;
    Py_hash_t hash;

    key = lru_cache_make_key(args, kwds, self->typed);
    if (!key)
        return NULL;
    hash = PyObject_Hash(key);
    if (hash == -1) {
        Py_DECREF(key);
        return NULL;
    }
    link  = (lru_list_elem *)_PyDict_GetItem_KnownHash(self->cache, key, hash);
    if (link != NULL) {
        lru_cache_extract_link(link);
        lru_cache_append_link(self, link);
        result = link->result;
        self->hits++;
        Py_INCREF(result);
        Py_DECREF(key);
        return result;
    }
    if (PyErr_Occurred()) {
        Py_DECREF(key);
        return NULL;
    }
    self->misses++;
    result = PyObject_Call(self->func, args, kwds);
    if (!result) {
        Py_DECREF(key);
        return NULL;
    }
    testresult = _PyDict_GetItem_KnownHash(self->cache, key, hash);
    if (testresult != NULL) {
        /* Getting here means that this same key was added to the cache
           during the PyObject_Call().  Since the link update is already
           done, we need only return the computed result. */
        Py_DECREF(key);
        return result;
    }
    if (PyErr_Occurred()) {
        /* This is an unusual case since this same lookup
           did not previously trigger an error during lookup.
           Treat it the same as an error in user function
           and return with the error set. */
        Py_DECREF(key);
        Py_DECREF(result);
        return NULL;
    }
    /* This is the normal case.  The new key wasn't found before
       user function call and it is still not there.  So we
       proceed normally and update the cache with the new result. */

    assert(self->maxsize > 0);
    if (PyDict_GET_SIZE(self->cache) < self->maxsize ||
        self->root.next == &self->root)
    {
        /* Cache is not full, so put the result in a new link */
        link = (lru_list_elem *)PyObject_New(lru_list_elem,
                                             &lru_list_elem_type);
        if (link == NULL) {
            Py_DECREF(key);
            Py_DECREF(result);
            return NULL;
        }

        link->hash = hash;
        link->key = key;
        link->result = result;
        /* What is really needed here is a SetItem variant with a "no clobber"
           option.  If the __eq__ call triggers a reentrant call that adds
           this same key, then this setitem call will update the cache dict
           with this new link, leaving the old link as an orphan (i.e. not
           having a cache dict entry that refers to it). */
        if (_PyDict_SetItem_KnownHash(self->cache, key, (PyObject *)link,
                                      hash) < 0) {
            Py_DECREF(link);
            return NULL;
        }
        lru_cache_append_link(self, link);
        Py_INCREF(result); /* for return */
        return result;
    }
    /* Since the cache is full, we need to evict an old key and add
       a new key.  Rather than free the old link and allocate a new
       one, we reuse the link for the new key and result and move it
       to front of the cache to mark it as recently used.

       We try to assure all code paths (including errors) leave all
       of the links in place.  Either the link is successfully
       updated and moved or it is restored to its old position.
       However if an unrecoverable error is found, it doesn't
       make sense to reinsert the link, so we leave it out
       and the cache will no longer register as full.
    */
    PyObject *oldkey, *oldresult, *popresult;

    /* Extract the oldest item. */
    assert(self->root.next != &self->root);
    link = self->root.next;
    lru_cache_extract_link(link);
    /* Remove it from the cache.
       The cache dict holds one reference to the link.
       We created one other reference when the link was created.
       The linked list only has borrowed references. */
    popresult = _PyDict_Pop_KnownHash(self->cache, link->key,
                                      link->hash, Py_None);
    if (popresult == Py_None) {
        /* Getting here means that the user function call or another
           thread has already removed the old key from the dictionary.
           This link is now an orphan.  Since we don't want to leave the
           cache in an inconsistent state, we don't restore the link. */
        Py_DECREF(popresult);
        Py_DECREF(link);
        Py_DECREF(key);
        return result;
    }
    if (popresult == NULL) {
        /* An error arose while trying to remove the oldest key (the one
           being evicted) from the cache.  We restore the link to its
           original position as the oldest link.  Then we allow the
           error propagate upward; treating it the same as an error
           arising in the user function. */
        lru_cache_prepend_link(self, link);
        Py_DECREF(key);
        Py_DECREF(result);
        return NULL;
    }
    /* Keep a reference to the old key and old result to prevent their
       ref counts from going to zero during the update. That will
       prevent potentially arbitrary object clean-up code (i.e. __del__)
       from running while we're still adjusting the links. */
    oldkey = link->key;
    oldresult = link->result;

    link->hash = hash;
    link->key = key;
    link->result = result;
    /* Note:  The link is being added to the cache dict without the
       prev and next fields set to valid values.   We have to wait
       for successful insertion in the cache dict before adding the
       link to the linked list.  Otherwise, the potentially reentrant
       __eq__ call could cause the then orphan link to be visited. */
    if (_PyDict_SetItem_KnownHash(self->cache, key, (PyObject *)link,
                                  hash) < 0) {
        /* Somehow the cache dict update failed.  We no longer can
           restore the old link.  Let the error propagate upward and
           leave the cache short one link. */
        Py_DECREF(popresult);
        Py_DECREF(link);
        Py_DECREF(oldkey);
        Py_DECREF(oldresult);
        return NULL;
    }
    lru_cache_append_link(self, link);
    Py_INCREF(result); /* for return */
    Py_DECREF(popresult);
    Py_DECREF(oldkey);
    Py_DECREF(oldresult);
    return result;
}

static PyObject *
lru_cache_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *func, *maxsize_O, *cache_info_type, *cachedict;
    int typed;
    lru_cache_object *obj;
    Py_ssize_t maxsize;
    PyObject *(*wrapper)(lru_cache_object *, PyObject *, PyObject *);
    static char *keywords[] = {"user_function", "maxsize", "typed",
                               "cache_info_type", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "OOpO:lru_cache", keywords,
                                     &func, &maxsize_O, &typed,
                                     &cache_info_type)) {
        return NULL;
    }

    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "the first argument must be callable");
        return NULL;
    }

    /* select the caching function, and make/inc maxsize_O */
    if (maxsize_O == Py_None) {
        wrapper = infinite_lru_cache_wrapper;
        /* use this only to initialize lru_cache_object attribute maxsize */
        maxsize = -1;
    } else if (PyIndex_Check(maxsize_O)) {
        maxsize = PyNumber_AsSsize_t(maxsize_O, PyExc_OverflowError);
        if (maxsize == -1 && PyErr_Occurred())
            return NULL;
        if (maxsize < 0) {
            maxsize = 0;
        }
        if (maxsize == 0)
            wrapper = uncached_lru_cache_wrapper;
        else
            wrapper = bounded_lru_cache_wrapper;
    } else {
        PyErr_SetString(PyExc_TypeError, "maxsize should be integer or None");
        return NULL;
    }

    if (!(cachedict = PyDict_New()))
        return NULL;

    obj = (lru_cache_object *)type->tp_alloc(type, 0);
    if (obj == NULL) {
        Py_DECREF(cachedict);
        return NULL;
    }

    obj->root.prev = &obj->root;
    obj->root.next = &obj->root;
    obj->wrapper = wrapper;
    obj->typed = typed;
    obj->cache = cachedict;
    Py_INCREF(func);
    obj->func = func;
    obj->misses = obj->hits = 0;
    obj->maxsize = maxsize;
    Py_INCREF(cache_info_type);
    obj->cache_info_type = cache_info_type;
    obj->dict = NULL;
    obj->weakreflist = NULL;
    return (PyObject *)obj;
}

static lru_list_elem *
lru_cache_unlink_list(lru_cache_object *self)
{
    lru_list_elem *root = &self->root;
    lru_list_elem *link = root->next;
    if (link == root)
        return NULL;
    root->prev->next = NULL;
    root->next = root->prev = root;
    return link;
}

static void
lru_cache_clear_list(lru_list_elem *link)
{
    while (link != NULL) {
        lru_list_elem *next = link->next;
        Py_DECREF(link);
        link = next;
    }
}

static void
lru_cache_dealloc(lru_cache_object *obj)
{
    lru_list_elem *list;
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(obj);
    if (obj->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject*)obj);

    list = lru_cache_unlink_list(obj);
    Py_XDECREF(obj->cache);
    Py_XDECREF(obj->func);
    Py_XDECREF(obj->cache_info_type);
    Py_XDECREF(obj->dict);
    lru_cache_clear_list(list);
    Py_TYPE(obj)->tp_free(obj);
}

static PyObject *
lru_cache_call(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    return self->wrapper(self, args, kwds);
}

static PyObject *
lru_cache_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    if (obj == Py_None || obj == NULL) {
        Py_INCREF(self);
        return self;
    }
    return PyMethod_New(self, obj);
}

static PyObject *
lru_cache_cache_info(lru_cache_object *self, PyObject *unused)
{
    if (self->maxsize == -1) {
        return PyObject_CallFunction(self->cache_info_type, "nnOn",
                                     self->hits, self->misses, Py_None,
                                     PyDict_GET_SIZE(self->cache));
    }
    return PyObject_CallFunction(self->cache_info_type, "nnnn",
                                 self->hits, self->misses, self->maxsize,
                                 PyDict_GET_SIZE(self->cache));
}

static PyObject *
lru_cache_cache_clear(lru_cache_object *self, PyObject *unused)
{
    lru_list_elem *list = lru_cache_unlink_list(self);
    self->hits = self->misses = 0;
    PyDict_Clear(self->cache);
    lru_cache_clear_list(list);
    Py_RETURN_NONE;
}

static PyObject *
lru_cache_reduce(PyObject *self, PyObject *unused)
{
    return PyObject_GetAttrString(self, "__qualname__");
}

static PyObject *
lru_cache_copy(PyObject *self, PyObject *unused)
{
    Py_INCREF(self);
    return self;
}

static PyObject *
lru_cache_deepcopy(PyObject *self, PyObject *unused)
{
    Py_INCREF(self);
    return self;
}

static int
lru_cache_tp_traverse(lru_cache_object *self, visitproc visit, void *arg)
{
    lru_list_elem *link = self->root.next;
    while (link != &self->root) {
        lru_list_elem *next = link->next;
        Py_VISIT(link->key);
        Py_VISIT(link->result);
        link = next;
    }
    Py_VISIT(self->func);
    Py_VISIT(self->cache);
    Py_VISIT(self->cache_info_type);
    Py_VISIT(self->dict);
    return 0;
}

static int
lru_cache_tp_clear(lru_cache_object *self)
{
    lru_list_elem *list = lru_cache_unlink_list(self);
    Py_CLEAR(self->func);
    Py_CLEAR(self->cache);
    Py_CLEAR(self->cache_info_type);
    Py_CLEAR(self->dict);
    lru_cache_clear_list(list);
    return 0;
}


PyDoc_STRVAR(lru_cache_doc,
"Create a cached callable that wraps another function.\n\
\n\
user_function:      the function being cached\n\
\n\
maxsize:  0         for no caching\n\
          None      for unlimited cache size\n\
          n         for a bounded cache\n\
\n\
typed:    False     cache f(3) and f(3.0) as identical calls\n\
          True      cache f(3) and f(3.0) as distinct calls\n\
\n\
cache_info_type:    namedtuple class with the fields:\n\
                        hits misses currsize maxsize\n"
);

static PyMethodDef lru_cache_methods[] = {
    {"cache_info", (PyCFunction)lru_cache_cache_info, METH_NOARGS},
    {"cache_clear", (PyCFunction)lru_cache_cache_clear, METH_NOARGS},
    {"__reduce__", (PyCFunction)lru_cache_reduce, METH_NOARGS},
    {"__copy__", (PyCFunction)lru_cache_copy, METH_VARARGS},
    {"__deepcopy__", (PyCFunction)lru_cache_deepcopy, METH_VARARGS},
    {NULL}
};

static PyGetSetDef lru_cache_getsetlist[] = {
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {NULL}
};

static PyTypeObject lru_cache_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "functools._lru_cache_wrapper",     /* tp_name */
    sizeof(lru_cache_object),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)lru_cache_dealloc,      /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    (ternaryfunc)lru_cache_call,        /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_METHOD_DESCRIPTOR,
                                        /* tp_flags */
    lru_cache_doc,                      /* tp_doc */
    (traverseproc)lru_cache_tp_traverse,/* tp_traverse */
    (inquiry)lru_cache_tp_clear,        /* tp_clear */
    0,                                  /* tp_richcompare */
    offsetof(lru_cache_object, weakreflist),
                                        /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    lru_cache_methods,                  /* tp_methods */
    0,                                  /* tp_members */
    lru_cache_getsetlist,               /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    lru_cache_descr_get,                /* tp_descr_get */
    0,                                  /* tp_descr_set */
    offsetof(lru_cache_object, dict),   /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    lru_cache_new,                      /* tp_new */
};

/* module level code ********************************************************/

PyDoc_STRVAR(_functools_doc,
"Tools that operate on functions.");

static PyMethodDef _functools_methods[] = {
    {"reduce",          functools_reduce,     METH_VARARGS, functools_reduce_doc},
    {"cmp_to_key",      (PyCFunction)(void(*)(void))functools_cmp_to_key,
     METH_VARARGS | METH_KEYWORDS, functools_cmp_to_key_doc},
    {NULL,              NULL}           /* sentinel */
};

static void
_functools_free(void *m)
{
    // FIXME: Do not clear kwd_mark to avoid NULL pointer dereferencing if we have
    //        other modules instances that could use it. Will fix when PEP-573 land
    //        and we could move kwd_mark to a per-module state.
    // Py_CLEAR(kwd_mark);
}

static int
_functools_exec(PyObject *module)
{
    PyTypeObject *typelist[] = {
        &partial_type,
        &lru_cache_type
    };

    if (!kwd_mark) {
        kwd_mark = _PyObject_CallNoArg((PyObject *)&PyBaseObject_Type);
        if (!kwd_mark) {
            return -1;
        }
    }

    for (size_t i = 0; i < Py_ARRAY_LENGTH(typelist); i++) {
        if (PyModule_AddType(module, typelist[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

static struct PyModuleDef_Slot _functools_slots[] = {
    {Py_mod_exec, _functools_exec},
    {0, NULL}
};

static struct PyModuleDef _functools_module = {
    PyModuleDef_HEAD_INIT,
    "_functools",
    _functools_doc,
    0,
    _functools_methods,
    _functools_slots,
    NULL,
    NULL,
    _functools_free,
};

PyMODINIT_FUNC
PyInit__functools(void)
{
    return PyModuleDef_Init(&_functools_module);
}
