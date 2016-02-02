
#include "Python.h"
#include "structmember.h"

/* _functools module written and maintained
   by Hye-Shik Chang <perky@FreeBSD.org>
   with adaptations by Raymond Hettinger <python@rcn.com>
   Copyright (c) 2004, 2005, 2006 Python Software Foundation.
   All rights reserved.
*/

/* reduce() *************************************************************/

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

PyDoc_STRVAR(reduce_doc,
"reduce(function, sequence[, initial]) -> value\n\
\n\
Apply a function of two arguments cumulatively to the items of a sequence,\n\
from left to right, so as to reduce the sequence to a single value.\n\
For example, reduce(lambda x, y: x+y, [1, 2, 3, 4, 5]) calculates\n\
((((1+2)+3)+4)+5).  If initial is present, it is placed before the items\n\
of the sequence in the calculation, and serves as a default when the\n\
sequence is empty.");




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
        Py_DECREF(pto);
        return NULL;
    }
    pto->kw = (kw != NULL) ? PyDict_Copy(kw) : PyDict_New();
    if (pto->kw == NULL) {
        Py_DECREF(pto);
        return NULL;
    }

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
    PyObject *argappl, *kwappl;

    assert (PyCallable_Check(pto->fn));
    assert (PyTuple_Check(pto->args));
    assert (PyDict_Check(pto->kw));

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
        assert(PyTuple_Check(argappl));
    }

    if (PyDict_Size(pto->kw) == 0) {
        kwappl = kw;
        Py_XINCREF(kwappl);
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

/* Pickle strategy:
   __reduce__ by itself doesn't support getting kwargs in the unpickle
   operation so we define a __setstate__ that replaces all the information
   about the partial.  If we only replaced part of it someone would use
   it as a hook to do strange things.
 */

PyObject *
partial_reduce(partialobject *pto, PyObject *unused)
{
    return Py_BuildValue("O(O)(OOOO)", Py_TYPE(pto), pto->fn, pto->fn,
                         pto->args, pto->kw,
                         pto->dict ? pto->dict : Py_None);
}

PyObject *
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

    Py_INCREF(fn);
    if (dict == Py_None)
        dict = NULL;
    else
        Py_INCREF(dict);

    Py_SETREF(pto->fn, fn);
    Py_SETREF(pto->args, fnargs);
    Py_SETREF(pto->kw, kw);
    Py_SETREF(pto->dict, dict);
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
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
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
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_WEAKREFS,         /* tp_flags */
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


/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"Tools that operate on functions.");

static PyMethodDef module_methods[] = {
    {"reduce",          functools_reduce,     METH_VARARGS, reduce_doc},
    {NULL,              NULL}           /* sentinel */
};

PyMODINIT_FUNC
init_functools(void)
{
    int i;
    PyObject *m;
    char *name;
    PyTypeObject *typelist[] = {
        &partial_type,
        NULL
    };

    m = Py_InitModule3("_functools", module_methods, module_doc);
    if (m == NULL)
        return;

    for (i=0 ; typelist[i] != NULL ; i++) {
        if (PyType_Ready(typelist[i]) < 0)
            return;
        name = strchr(typelist[i]->tp_name, '.');
        assert (name != NULL);
        Py_INCREF(typelist[i]);
        PyModule_AddObject(m, name+1, (PyObject *)typelist[i]);
    }
}
