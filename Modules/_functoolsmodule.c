
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
    PyObject *func;
    partialobject *pto;

    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError,
                        "type 'partial' takes at least one argument");
        return NULL;
    }

    func = PyTuple_GET_ITEM(args, 0);
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
    pto->args = PyTuple_GetSlice(args, 1, PY_SSIZE_T_MAX);
    if (pto->args == NULL) {
        pto->kw = NULL;
        Py_DECREF(pto);
        return NULL;
    }
    if (kw != NULL) {
        pto->kw = PyDict_Copy(kw);
        if (pto->kw == NULL) {
            Py_DECREF(pto);
            return NULL;
        }
    } else {
        pto->kw = Py_None;
        Py_INCREF(Py_None);
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

static PyObject *
partial_get_dict(partialobject *pto)
{
    if (pto->dict == NULL) {
        pto->dict = PyDict_New();
        if (pto->dict == NULL)
            return NULL;
    }
    Py_INCREF(pto->dict);
    return pto->dict;
}

static int
partial_set_dict(partialobject *pto, PyObject *value)
{
    PyObject *tmp;

    /* It is illegal to del p.__dict__ */
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "a partial object's dictionary may not be deleted");
        return -1;
    }
    /* Can only set __dict__ to a dictionary */
    if (!PyDict_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "setting partial object's dictionary to a non-dict");
        return -1;
    }
    tmp = pto->dict;
    Py_INCREF(value);
    pto->dict = value;
    Py_XDECREF(tmp);
    return 0;
}

static PyGetSetDef partial_getsetlist[] = {
    {"__dict__", (getter)partial_get_dict, (setter)partial_set_dict},
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
partial_setstate(partialobject *pto, PyObject *args)
{
    PyObject *fn, *fnargs, *kw, *dict;
    if (!PyArg_ParseTuple(args, "(OOOO):__setstate__",
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
    {"__setstate__", (PyCFunction)partial_setstate, METH_VARARGS},
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
keyobject_call(keyobject *ko, PyObject *args, PyObject *kw);

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

/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"Tools that operate on functions.");

static PyMethodDef module_methods[] = {
    {"reduce",          functools_reduce,     METH_VARARGS, functools_reduce_doc},
    {"cmp_to_key",      (PyCFunction)functools_cmp_to_key,
     METH_VARARGS | METH_KEYWORDS, functools_cmp_to_key_doc},
    {NULL,              NULL}           /* sentinel */
};


static struct PyModuleDef _functoolsmodule = {
    PyModuleDef_HEAD_INIT,
    "_functools",
    module_doc,
    -1,
    module_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__functools(void)
{
    int i;
    PyObject *m;
    char *name;
    PyTypeObject *typelist[] = {
        &partial_type,
        NULL
    };

    m = PyModule_Create(&_functoolsmodule);
    if (m == NULL)
        return NULL;

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
