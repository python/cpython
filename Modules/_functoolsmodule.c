
#include "Python.h"
#include "structmember.h"

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
    PyObject *dict;
    PyObject *weakreflist; /* List of weak references */
} partialobject;

static PyTypeObject partial_type;

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

    pargs = pkw = Py_None;
    func = PyTuple_GET_ITEM(args, 0);
    if (Py_TYPE(func) == &partial_type && type == &partial_type) {
        partialobject *part = (partialobject *)func;
        if (part->dict == NULL) {
            pargs = part->args;
            pkw = part->kw;
            func = part->fn;
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
        pto->args = NULL;
        pto->kw = NULL;
        Py_DECREF(pto);
        return NULL;
    }
    if (pargs == Py_None || PyTuple_GET_SIZE(pargs) == 0) {
        pto->args = nargs;
        Py_INCREF(nargs);
    }
    else if (PyTuple_GET_SIZE(nargs) == 0) {
        pto->args = pargs;
        Py_INCREF(pargs);
    }
    else {
        pto->args = PySequence_Concat(pargs, nargs);
        if (pto->args == NULL) {
            pto->kw = NULL;
            Py_DECREF(pto);
            return NULL;
        }
    }
    Py_DECREF(nargs);

    if (kw != NULL) {
        if (pkw == Py_None) {
            pto->kw = PyDict_Copy(kw);
        }
        else {
            pto->kw = PyDict_Copy(pkw);
            if (pto->kw != NULL) {
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
    }
    else {
        if (pkw == Py_None) {
            pto->kw = PyDict_New();
            if (pto->kw == NULL) {
                Py_DECREF(pto);
                return NULL;
            }
        }
        else {
            pto->kw = pkw;
            Py_INCREF(pkw);
        }
    }

    pto->weakreflist = NULL;
    pto->dict = NULL;

    return (PyObject *)pto;
}

static void
partial_dealloc(partialobject *pto)
{
    PyObject_GC_UnTrack(pto);
    if (pto->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) pto);
    Py_XDECREF(pto->fn);
    Py_XDECREF(pto->args);
    Py_XDECREF(pto->kw);
    Py_XDECREF(pto->dict);
    Py_TYPE(pto)->tp_free(pto);
}

static PyObject *
partial_call(partialobject *pto, PyObject *args, PyObject *kw)
{
    PyObject *ret;
    PyObject *argappl = NULL, *kwappl = NULL;

    assert (PyCallable_Check(pto->fn));
    assert (PyTuple_Check(pto->args));
    assert (pto->kw == Py_None  ||  PyDict_Check(pto->kw));

    if (PyTuple_GET_SIZE(pto->args) == 0) {
        argappl = args;
        Py_INCREF(args);
    } else if (PyTuple_GET_SIZE(args) == 0) {
        argappl = pto->args;
        Py_INCREF(pto->args);
    } else {
        argappl = PySequence_Concat(pto->args, args);
        if (argappl == NULL)
            return NULL;
    }

    if (pto->kw == Py_None) {
        kwappl = kw;
        Py_XINCREF(kw);
    } else {
        kwappl = PyDict_Copy(pto->kw);
        if (kwappl == NULL) {
            Py_DECREF(argappl);
            return NULL;
        }
        if (kw != NULL) {
            if (PyDict_Merge(kwappl, kw, 1) != 0) {
                Py_DECREF(argappl);
                Py_DECREF(kwappl);
                return NULL;
            }
        }
    }

    ret = PyObject_Call(pto->fn, argappl, kwappl);
    Py_DECREF(argappl);
    Py_XDECREF(kwappl);
    return ret;
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
    PyObject *result;
    PyObject *arglist;
    PyObject *tmp;
    Py_ssize_t i, n;

    arglist = PyUnicode_FromString("");
    if (arglist == NULL) {
        return NULL;
    }
    /* Pack positional arguments */
    assert (PyTuple_Check(pto->args));
    n = PyTuple_GET_SIZE(pto->args);
    for (i = 0; i < n; i++) {
        tmp = PyUnicode_FromFormat("%U, %R", arglist,
                                   PyTuple_GET_ITEM(pto->args, i));
        Py_DECREF(arglist);
        if (tmp == NULL)
            return NULL;
        arglist = tmp;
    }
    /* Pack keyword arguments */
    assert (pto->kw == Py_None  ||  PyDict_Check(pto->kw));
    if (pto->kw != Py_None) {
        PyObject *key, *value;
        for (i = 0; PyDict_Next(pto->kw, &i, &key, &value);) {
            tmp = PyUnicode_FromFormat("%U, %U=%R", arglist,
                                       key, value);
            Py_DECREF(arglist);
            if (tmp == NULL)
                return NULL;
            arglist = tmp;
        }
    }
    result = PyUnicode_FromFormat("%s(%R%U)", Py_TYPE(pto)->tp_name,
                                  pto->fn, arglist);
    Py_DECREF(arglist);
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
    if (!PyArg_ParseTuple(state, "OOOO",
                          &fn, &fnargs, &kw, &dict))
        return NULL;
    Py_XDECREF(pto->fn);
    Py_XDECREF(pto->args);
    Py_XDECREF(pto->kw);
    Py_XDECREF(pto->dict);
    pto->fn = fn;
    pto->args = fnargs;
    pto->kw = kw;
    if (dict != Py_None) {
      pto->dict = dict;
      Py_INCREF(dict);
    } else {
      pto->dict = NULL;
    }
    Py_INCREF(fn);
    Py_INCREF(fnargs);
    Py_INCREF(kw);
    Py_RETURN_NONE;
}

static PyMethodDef partial_methods[] = {
    {"__reduce__", (PyCFunction)partial_reduce, METH_NOARGS},
    {"__setstate__", (PyCFunction)partial_setstate, METH_O},
    {NULL,              NULL}           /* sentinel */
};

static PyTypeObject partial_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "functools.partial",                /* tp_name */
    sizeof(partialobject),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)partial_dealloc,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
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
    if (ko->object)
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
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
    PyObject *args;
    PyObject *x;
    PyObject *y;
    PyObject *compare;
    PyObject *answer;
    static PyObject *zero;

    if (zero == NULL) {
        zero = PyLong_FromLong(0);
        if (!zero)
            return NULL;
    }

    if (Py_TYPE(other) != &keyobject_type){
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
    args = PyTuple_New(2);
    if (args == NULL)
        return NULL;
    Py_INCREF(x);
    Py_INCREF(y);
    PyTuple_SET_ITEM(args, 0, x);
    PyTuple_SET_ITEM(args, 1, y);
    res = PyObject_Call(compare, args, NULL);
    Py_DECREF(args);
    if (res == NULL)
        return NULL;
    answer = PyObject_RichCompare(res, zero, op);
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

        if (args->ob_refcnt > 1) {
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
            PyTuple_SetItem(args, 0, result);
            PyTuple_SetItem(args, 1, op2);
            if ((result = PyEval_CallObject(func, args)) == NULL)
                goto Fail;
        }
    }

    Py_DECREF(args);

    if (result == NULL)
        PyErr_SetString(PyExc_TypeError,
                   "reduce() of empty sequence with no initial value");

    Py_DECREF(it);
    return result;

Fail:
    Py_XDECREF(args);
    Py_XDECREF(result);
    Py_DECREF(it);
    return NULL;
}

PyDoc_STRVAR(functools_reduce_doc,
"reduce(function, sequence[, initial]) -> value\n\
\n\
Apply a function of two arguments cumulatively to the items of a sequence,\n\
from left to right, so as to reduce the sequence to a single value.\n\
For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates\n\
((((1+2)+3)+4)+5).  If initial is present, it is placed before the items\n\
of the sequence in the calculation, and serves as a default when the\n\
sequence is empty.");

/* lru_cache object **********************************************************/

/* this object is used delimit args and keywords in the cache keys */
static PyObject *kwd_mark = NULL;

struct lru_list_elem;
struct lru_cache_object;

typedef struct lru_list_elem {
    PyObject_HEAD
    struct lru_list_elem *prev, *next;  /* borrowed links */
    PyObject *key, *result;
} lru_list_elem;

static void
lru_list_elem_dealloc(lru_list_elem *link)
{
    _PyObject_GC_UNTRACK(link);
    Py_XDECREF(link->key);
    Py_XDECREF(link->result);
    PyObject_GC_Del(link);
}

static int
lru_list_elem_traverse(lru_list_elem *link, visitproc visit, void *arg)
{
    Py_VISIT(link->key);
    Py_VISIT(link->result);
    return 0;
}

static int
lru_list_elem_clear(lru_list_elem *link)
{
    Py_CLEAR(link->key);
    Py_CLEAR(link->result);
    return 0;
}

static PyTypeObject lru_list_elem_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "functools._lru_list_elem",         /* tp_name */
    sizeof(lru_list_elem),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)lru_list_elem_dealloc,  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,  /* tp_flags */
    0,                                  /* tp_doc */
    (traverseproc)lru_list_elem_traverse,  /* tp_traverse */
    (inquiry)lru_list_elem_clear,       /* tp_clear */
};


typedef PyObject *(*lru_cache_ternaryfunc)(struct lru_cache_object *, PyObject *, PyObject *);

typedef struct lru_cache_object {
    lru_list_elem root;  /* includes PyObject_HEAD */
    Py_ssize_t maxsize;
    PyObject *maxsize_O;
    PyObject *func;
    lru_cache_ternaryfunc wrapper;
    PyObject *cache;
    PyObject *cache_info_type;
    Py_ssize_t misses, hits;
    int typed;
    PyObject *dict;
    int full;
} lru_cache_object;

static PyTypeObject lru_cache_type;

static PyObject *
lru_cache_make_key(PyObject *args, PyObject *kwds, int typed)
{
    PyObject *key, *sorted_items;
    Py_ssize_t key_size, pos, key_pos;

    /* short path, key will match args anyway, which is a tuple */
    if (!typed && !kwds) {
        Py_INCREF(args);
        return args;
    }

    if (kwds && PyDict_Size(kwds) > 0) {
        sorted_items = PyDict_Items(kwds);
        if (!sorted_items)
            return NULL;
        if (PyList_Sort(sorted_items) < 0) {
            Py_DECREF(sorted_items);
            return NULL;
        }
    } else
        sorted_items = NULL;

    key_size = PyTuple_GET_SIZE(args);
    if (sorted_items)
        key_size += PyList_GET_SIZE(sorted_items);
    if (typed)
        key_size *= 2;
    if (sorted_items)
        key_size++;

    key = PyTuple_New(key_size);
    if (key == NULL)
        goto done;

    key_pos = 0;
    for (pos = 0; pos < PyTuple_GET_SIZE(args); ++pos) {
        PyObject *item = PyTuple_GET_ITEM(args, pos);
        Py_INCREF(item);
        PyTuple_SET_ITEM(key, key_pos++, item);
    }
    if (sorted_items) {
        Py_INCREF(kwd_mark);
        PyTuple_SET_ITEM(key, key_pos++, kwd_mark);
        for (pos = 0; pos < PyList_GET_SIZE(sorted_items); ++pos) {
            PyObject *item = PyList_GET_ITEM(sorted_items, pos);
            Py_INCREF(item);
            PyTuple_SET_ITEM(key, key_pos++, item);
        }
    }
    if (typed) {
        for (pos = 0; pos < PyTuple_GET_SIZE(args); ++pos) {
            PyObject *item = (PyObject *)Py_TYPE(PyTuple_GET_ITEM(args, pos));
            Py_INCREF(item);
            PyTuple_SET_ITEM(key, key_pos++, item);
        }
        if (sorted_items) {
            for (pos = 0; pos < PyList_GET_SIZE(sorted_items); ++pos) {
                PyObject *tp_items = PyList_GET_ITEM(sorted_items, pos);
                PyObject *item = (PyObject *)Py_TYPE(PyTuple_GET_ITEM(tp_items, 1));
                Py_INCREF(item);
                PyTuple_SET_ITEM(key, key_pos++, item);
            }
        }
    }
    assert(key_pos == key_size);

done:
    if (sorted_items)
        Py_DECREF(sorted_items);
    return key;
}

static PyObject *
uncached_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    PyObject *result = PyObject_Call(self->func, args, kwds);
    if (!result)
        return NULL;
    self->misses++;
    return result;
}

static PyObject *
infinite_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    PyObject *result;
    PyObject *key = lru_cache_make_key(args, kwds, self->typed);
    if (!key)
        return NULL;
    result = PyDict_GetItemWithError(self->cache, key);
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
    result = PyObject_Call(self->func, args, kwds);
    if (!result) {
        Py_DECREF(key);
        return NULL;
    }
    if (PyDict_SetItem(self->cache, key, result) < 0) {
        Py_DECREF(result);
        Py_DECREF(key);
        return NULL;
    }
    Py_DECREF(key);
    self->misses++;
    return result;
}

static void
lru_cache_extricate_link(lru_list_elem *link)
{
    link->prev->next = link->next;
    link->next->prev = link->prev;
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

static PyObject *
bounded_lru_cache_wrapper(lru_cache_object *self, PyObject *args, PyObject *kwds)
{
    lru_list_elem *link;
    PyObject *key, *result;

    key = lru_cache_make_key(args, kwds, self->typed);
    if (!key)
        return NULL;
    link  = (lru_list_elem *)PyDict_GetItemWithError(self->cache, key);
    if (link) {
        lru_cache_extricate_link(link);
        lru_cache_append_link(self, link);
        self->hits++;
        result = link->result;
        Py_INCREF(result);
        Py_DECREF(key);
        return result;
    }
    if (PyErr_Occurred()) {
        Py_DECREF(key);
        return NULL;
    }
    result = PyObject_Call(self->func, args, kwds);
    if (!result) {
        Py_DECREF(key);
        return NULL;
    }
    if (self->full && self->root.next != &self->root) {
        /* Use the oldest item to store the new key and result. */
        PyObject *oldkey, *oldresult;
        /* Extricate the oldest item. */
        link = self->root.next;
        lru_cache_extricate_link(link);
        /* Remove it from the cache.
           The cache dict holds one reference to the link,
           and the linked list holds yet one reference to it. */
        if (PyDict_DelItem(self->cache, link->key) < 0) {
            lru_cache_append_link(self, link);
            Py_DECREF(key);
            Py_DECREF(result);
            return NULL;
        }
        /* Keep a reference to the old key and old result to
           prevent their ref counts from going to zero during the
           update. That will prevent potentially arbitrary object
           clean-up code (i.e. __del__) from running while we're
           still adjusting the links. */
        oldkey = link->key;
        oldresult = link->result;

        link->key = key;
        link->result = result;
        if (PyDict_SetItem(self->cache, key, (PyObject *)link) < 0) {
            Py_DECREF(link);
            Py_DECREF(oldkey);
            Py_DECREF(oldresult);
            return NULL;
        }
        lru_cache_append_link(self, link);
        Py_INCREF(result); /* for return */
        Py_DECREF(oldkey);
        Py_DECREF(oldresult);
    } else {
        /* Put result in a new link at the front of the queue. */
        link = (lru_list_elem *)PyObject_GC_New(lru_list_elem,
                                                &lru_list_elem_type);
        if (link == NULL) {
            Py_DECREF(key);
            Py_DECREF(result);
            return NULL;
        }

        link->key = key;
        link->result = result;
        _PyObject_GC_TRACK(link);
        if (PyDict_SetItem(self->cache, key, (PyObject *)link) < 0) {
            Py_DECREF(link);
            return NULL;
        }
        lru_cache_append_link(self, link);
        Py_INCREF(result); /* for return */
        self->full = (PyDict_Size(self->cache) >= self->maxsize);
    }
    self->misses++;
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

    obj->cache = cachedict;
    obj->root.prev = &obj->root;
    obj->root.next = &obj->root;
    obj->maxsize = maxsize;
    Py_INCREF(maxsize_O);
    obj->maxsize_O = maxsize_O;
    Py_INCREF(func);
    obj->func = func;
    obj->wrapper = wrapper;
    obj->misses = obj->hits = 0;
    obj->typed = typed;
    Py_INCREF(cache_info_type);
    obj->cache_info_type = cache_info_type;

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
    lru_list_elem *list = lru_cache_unlink_list(obj);
    Py_XDECREF(obj->maxsize_O);
    Py_XDECREF(obj->func);
    Py_XDECREF(obj->cache);
    Py_XDECREF(obj->dict);
    Py_XDECREF(obj->cache_info_type);
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
    return PyObject_CallFunction(self->cache_info_type, "nnOn",
                                 self->hits, self->misses, self->maxsize_O,
                                 PyDict_Size(self->cache));
}

static PyObject *
lru_cache_cache_clear(lru_cache_object *self, PyObject *unused)
{
    lru_list_elem *list = lru_cache_unlink_list(self);
    self->hits = self->misses = 0;
    self->full = 0;
    PyDict_Clear(self->cache);
    lru_cache_clear_list(list);
    Py_RETURN_NONE;
}

static int
lru_cache_tp_traverse(lru_cache_object *self, visitproc visit, void *arg)
{
    lru_list_elem *link = self->root.next;
    while (link != &self->root) {
        lru_list_elem *next = link->next;
        Py_VISIT(link);
        link = next;
    }
    Py_VISIT(self->maxsize_O);
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
    Py_CLEAR(self->maxsize_O);
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
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
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
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_GC,
                                        /* tp_flags */
    lru_cache_doc,                      /* tp_doc */
    (traverseproc)lru_cache_tp_traverse,/* tp_traverse */
    (inquiry)lru_cache_tp_clear,        /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
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

PyDoc_STRVAR(module_doc,
"Tools that operate on functions.");

static PyMethodDef module_methods[] = {
    {"reduce",          functools_reduce,     METH_VARARGS, functools_reduce_doc},
    {"cmp_to_key",      (PyCFunction)functools_cmp_to_key,
     METH_VARARGS | METH_KEYWORDS, functools_cmp_to_key_doc},
    {NULL,              NULL}           /* sentinel */
};

static void
module_free(void *m)
{
    Py_CLEAR(kwd_mark);
}

static struct PyModuleDef _functoolsmodule = {
    PyModuleDef_HEAD_INIT,
    "_functools",
    module_doc,
    -1,
    module_methods,
    NULL,
    NULL,
    NULL,
    module_free,
};

PyMODINIT_FUNC
PyInit__functools(void)
{
    int i;
    PyObject *m;
    char *name;
    PyTypeObject *typelist[] = {
        &partial_type,
        &lru_cache_type,
        NULL
    };

    m = PyModule_Create(&_functoolsmodule);
    if (m == NULL)
        return NULL;

    kwd_mark = PyObject_CallObject((PyObject *)&PyBaseObject_Type, NULL);
    if (!kwd_mark) {
        Py_DECREF(m);
        return NULL;
    }

    for (i=0 ; typelist[i] != NULL ; i++) {
        if (PyType_Ready(typelist[i]) < 0) {
            Py_DECREF(m);
            return NULL;
        }
        name = strchr(typelist[i]->tp_name, '.');
        assert (name != NULL);
        Py_INCREF(typelist[i]);
        PyModule_AddObject(m, name+1, (PyObject *)typelist[i]);
    }
    return m;
}
