/* ABCMeta implementation */

/* TODO:

   Use fast paths where possible.
   In particular use capitals like PyList_GET_SIZE.
   Think (ask) about inlining some calls, like __subclasses__.
   Fix refleaks. */

#include "Python.h"
#include "structmember.h"

PyDoc_STRVAR(_abc__doc__,
"Module contains faster C implementation of abc.ABCMeta");
#define DEFERRED_ADDRESS(ADDR) 0

_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(__abstractmethods__);
_Py_IDENTIFIER(__class__);


/* A global counter that is incremented each time a class is
   registered as a virtual subclass of anything.  It forces the
   negative cache to be cleared before its next use.
   Note: this counter is private. Use `abc.get_cache_token()` for
   external code. */
static Py_ssize_t abc_invalidation_counter = 0;

typedef struct {
    PyHeapTypeObject tp;
    PyObject *abc_registry; /* these three are normal set of weakrefs without callback */
    PyObject *abc_cache;
    PyObject *abc_negative_cache;
    Py_ssize_t abc_negative_cache_version;
} abc;

static void
abcmeta_dealloc(abc *tp)
{
    Py_CLEAR(tp->abc_registry);
    Py_CLEAR(tp->abc_cache);
    Py_CLEAR(tp->abc_negative_cache);
    Py_TYPE(tp)->tp_free((PyObject *)tp);
}

static int
abcmeta_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((abc *)self)->abc_registry);
    Py_VISIT(((abc *)self)->abc_cache);
    Py_VISIT(((abc *)self)->abc_negative_cache);
    return PyType_Type.tp_traverse(self, visit, arg);
}

static int
abcmeta_clear(abc *tp)
{
    if (tp->abc_registry) {
        PySet_Clear(tp->abc_registry);
    }
    if (tp->abc_cache) {
        PySet_Clear(tp->abc_cache);
    }
    if (tp->abc_negative_cache) {
        PySet_Clear(tp->abc_negative_cache);
    }
    return PyType_Type.tp_clear((PyObject *)tp);
}

static PyObject *
abcmeta_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    abc *result = NULL;
    PyObject *ns, *bases, *items, *abstracts, *base_abstracts;
    PyObject *key, *value, *item, *iter;
    Py_ssize_t pos = 0;
    result = (abc *)PyType_Type.tp_new(type, args, kwds);
    if (!result) {
        return NULL;
    }
    /* Set up inheritance registry. */
    result->abc_registry = PySet_New(NULL);
    result->abc_cache = PySet_New(NULL);
    result->abc_negative_cache = PySet_New(NULL);
    if (!result->abc_registry || !result->abc_cache || !result->abc_negative_cache) {
        Py_DECREF(result);
        return NULL;
    }
    result->abc_negative_cache_version = abc_invalidation_counter;
    if (!(abstracts = PyFrozenSet_New(NULL))) {
        Py_DECREF(result);
        return NULL;
    }

    /* Compute set of abstract method names in two stages:
       Stage 1: direct abstract methods.
       (It is safe to assume everything is fine since type.__new__ succeeded.) */
    ns = PyTuple_GET_ITEM(args, 2);
    items = PyMapping_Items(ns); /* TODO: Fast path for exact dicts with PyDict_Next */
    if (!items) {
        Py_DECREF(result);
        return NULL;
    }
    for (pos = 0; pos < PySequence_Size(items); pos++) {
        item = PySequence_GetItem(items, pos);
        if (!PyTuple_Check(item) || (PyTuple_GET_SIZE(item) != 2)) {
            PyErr_SetString(PyExc_TypeError, "Iteration over class namespace must"
                                             " yield length-two tuples");
        }
        key = PyTuple_GetItem(item, 0);
        value = PyTuple_GetItem(item, 1);
        int is_abstract = _PyObject_IsAbstract(value);
        if (is_abstract < 0) {
            Py_DECREF(item);
            Py_DECREF(items);
            goto error;
        }
        if (is_abstract && PySet_Add(abstracts, key) < 0) {
            Py_DECREF(item);
            Py_DECREF(items);
            goto error;
        }
        Py_DECREF(item);
    }
    Py_DECREF(items);

    /* Stage 2: inherited abstract methods. */
    bases = PyTuple_GET_ITEM(args, 1);
    for (pos = 0; pos < PyTuple_Size(bases); pos++) {
        item = PyTuple_GET_ITEM(bases, pos);
        base_abstracts = _PyObject_GetAttrId(item, &PyId___abstractmethods__);
        if (!base_abstracts) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
                goto error;
            }
            PyErr_Clear();
            continue;
        }
        if (!(iter = PyObject_GetIter(base_abstracts))) {
            goto error;
        }
        while ((key = PyIter_Next(iter))) {
            value = PyObject_GetAttr((PyObject *)result, key);
            if (!value) {
                if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
                    Py_DECREF(key);
                    Py_DECREF(iter);
                    goto error;
                }
                PyErr_Clear();
                Py_DECREF(key);
                continue;
            }
            int is_abstract = _PyObject_IsAbstract(value);
            Py_DECREF(value);
            if (is_abstract < 0) {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            }
            if (is_abstract && PySet_Add(abstracts, key) < 0) {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            }
            Py_DECREF(key);
        }
        Py_DECREF(iter);
        if (PyErr_Occurred()) {
            goto error;
        }
    }
    if (_PyObject_SetAttrId((PyObject *)result, &PyId___abstractmethods__, abstracts) < 0) {
        goto error;
    }
    return (PyObject *)result;
error:
    Py_DECREF(result);
    Py_DECREF(abstracts);
    return NULL;
}

int
_in_weak_set(PyObject *set, PyObject *obj)
{
    PyObject *ref;
    int res;
    ref = PyWeakref_NewRef(obj, NULL);
    if (!ref) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            return 0;
        }
        return -1;
    }
    res = PySet_Contains(set, ref);
    Py_DECREF(ref);
    return res;
}

int
_add_weak_set(PyObject *set, PyObject *obj)
{
    PyObject *ref;
    ref = PyWeakref_NewRef(obj, NULL);
    if (!ref) {
        return 0;
    }
    if (PySet_Add(set, ref) < 0) {
        return 0;
    }
    return 1;
}

static PyObject *
abcmeta_register(abc *self, PyObject *args)
{
    PyObject *subclass = NULL;
    int result;
    if (!PyArg_UnpackTuple(args, "register", 1, 1, &subclass)) {
        return NULL;
    }
    if (!PyType_Check(subclass)) {
        PyErr_SetString(PyExc_TypeError, "Can only register classes");
        return NULL;
    }
    result = PyObject_IsSubclass(subclass, (PyObject *)self);
    if (result > 0) {
        Py_INCREF(subclass);
        return subclass;  /* Already a subclass. */
    }
    if (result < 0) {
        return NULL;
    }
    /* Subtle: test for cycles *after* testing for "already a subclass";
       this means we allow X.register(X) and interpret it as a no-op. */
    result = PyObject_IsSubclass((PyObject *)self, subclass);
    if (result > 0) {
        /* This would create a cycle, which is bad for the algorithm below. */
        PyErr_SetString(PyExc_RuntimeError, "Refusing to create an inheritance cycle");
        return NULL;
    }
    if (result < 0) {
        return NULL;
    }
    if (!_add_weak_set(self->abc_registry, subclass)) {
        return NULL;
    }
    Py_INCREF(subclass);
    abc_invalidation_counter++;  /* Invalidate negative cache */
    return subclass;
}

static PyObject *
abcmeta_subclasscheck(abc *self, PyObject *args); /* Forward */

static PyObject *
abcmeta_instancecheck(abc *self, PyObject *args)
{
    PyObject *result, *subclass, *subtype, *instance = NULL;
    int incache;
    if (!PyArg_UnpackTuple(args, "__instancecheck__", 1, 1, &instance)) {
        return NULL;
    }
    subclass = _PyObject_GetAttrId(instance, &PyId___class__);
    /* Inline the cache checking. */
    incache = _in_weak_set(self->abc_cache, subclass);
    if (incache < 0) {
        return NULL;
    }
    if (incache > 0) {
        Py_RETURN_TRUE;
    }
    subtype = (PyObject *)Py_TYPE(instance);
    if (subtype == subclass) {
        incache = _in_weak_set(self->abc_negative_cache, subclass);
        if (incache < 0) {
            return NULL;
        }
        if (self->abc_negative_cache_version == abc_invalidation_counter && incache) {
            Py_RETURN_FALSE;
        }
        /* Fall back to the subclass check. */
        return PyObject_CallMethod((PyObject *)self, "__subclasscheck__", "O", subclass);
    }
    result = PyObject_CallMethod((PyObject *)self, "__subclasscheck__", "O", subclass);
    if (!result) {
        return NULL;
    }
    if (result == Py_True) {
        return Py_True;
    }
    Py_DECREF(result);
    return PyObject_CallMethod((PyObject *)self, "__subclasscheck__", "O", subtype);
}

static PyObject *
abcmeta_subclasscheck(abc *self, PyObject *args)
{
    PyObject *subclasses, *subclass = NULL;
    PyObject *ok, *mro, *key, *rkey;
    Py_ssize_t pos;
    int incache, result;
    if (!PyArg_UnpackTuple(args, "__subclasscheck__", 1, 1, &subclass)) {
        return NULL;
    }
    /* TODO: clear the registry from dead refs from time to time
       on iteration here (have a counter for this) or maybe during a .register() call */
    /* TODO: Reset caches every n-th succes/failure correspondingly
       so that they don't grow too large */

    /* 1. Check cache. */
    incache = _in_weak_set(self->abc_cache, subclass);
    if (incache < 0) {
        return NULL;
    }
    if (incache > 0) {
        Py_RETURN_TRUE;
    }
    /* 2. Check negative cache; may have to invalidate. */
    incache = _in_weak_set(self->abc_negative_cache, subclass);
    if (incache < 0) {
        return NULL;
    }
    if (self->abc_negative_cache_version < abc_invalidation_counter) {
        /* Invalidate the negative cache. */
        self->abc_negative_cache = PySet_New(NULL);
        if (!self->abc_negative_cache) {
            return NULL;
        }
        self->abc_negative_cache_version = abc_invalidation_counter;
    } else if (incache) {
        Py_RETURN_FALSE;
    }
    /* 3. Check the subclass hook. */
    ok = PyObject_CallMethod((PyObject *)self, "__subclasshook__", "O", subclass);
    if (!ok) {
        return NULL;
    }
    if (ok == Py_True) {
        if (!_add_weak_set(self->abc_cache, subclass)) {
            Py_DECREF(ok);
            return NULL;
        }
        return Py_True;
    }
    if (ok == Py_False) {
        if (!_add_weak_set(self->abc_negative_cache, subclass)) {
            Py_DECREF(ok);
            return NULL;
        }
        return Py_False;
    }
    Py_DECREF(ok);
    /* 4. Check if it's a direct subclass. */
    mro = ((PyTypeObject *)subclass)->tp_mro;
    for (pos = 0; pos < PyTuple_Size(mro); pos++) {
        if ((PyObject *)self == PyTuple_GetItem(mro, pos)) {
            if (!_add_weak_set(self->abc_cache, subclass)) {
                return NULL;
            }
            Py_RETURN_TRUE;
        }
    }

    /* 5. Check if it's a subclass of a registered class (recursive). */
    pos = 0;
    Py_hash_t hash;
    while (_PySet_NextEntry(self->abc_registry, &pos, &key, &hash)) {
        rkey = PyWeakref_GetObject(key);
        if (rkey == Py_None) {
            continue;
        }
        result = PyObject_IsSubclass(subclass, rkey);
        if (result < 0) {
            return NULL;
        }
        if (result > 0) {
            if (!_add_weak_set(self->abc_cache, subclass)) {
                return NULL;
            }
            Py_RETURN_TRUE;
        }
    }

    /* 6. Check if it's a subclass of a subclass (recursive). */
    subclasses = PyObject_CallMethod((PyObject *)self, "__subclasses__", NULL);
    for (pos = 0; pos < PyList_GET_SIZE(subclasses); pos++) {
        result = PyObject_IsSubclass(subclass, PyList_GET_ITEM(subclasses, pos));
        if (result > 0) {
            if (!_add_weak_set(self->abc_cache, subclass)) {
                return NULL;
            }
            Py_DECREF(subclasses);
            Py_RETURN_TRUE;
        }
        if (result < 0) {
            Py_DECREF(subclasses);
            return NULL;
        }
    }
    Py_DECREF(subclasses);
    /* No dice; update negative cache. */
    if (!_add_weak_set(self->abc_negative_cache, subclass)) {
        return NULL;
    }
    Py_RETURN_FALSE;
}

int
_print_message(PyObject *file, const char* message)
{
    PyObject *mo = PyUnicode_FromString(message);
    if (!mo) {
        return -1;
    }
    if (PyFile_WriteObject(mo, file, Py_PRINT_RAW)) {
        Py_DECREF(mo);
        return -1;
    }
    Py_DECREF(mo);
    return 0;
}

int
_dump_attr(PyObject *file, PyObject *obj, char *msg)
{
    if (_print_message(file, msg)) {
        return 0;
    }
    if (PyFile_WriteObject(obj, file, Py_PRINT_RAW)) {
        return 0;
    }
    if (_print_message(file, "\n")) {
        return 0;
    }
    return 1;
}

static PyObject *
abcmeta_dump(abc *self, PyObject *args)
{
    PyObject *file = NULL;
    PyObject *version;
    if (!PyArg_UnpackTuple(args, "_dump_registry", 0, 1, &file)) {
        return NULL;
    }
    if (file == NULL || file == Py_None) {
        file = _PySys_GetObjectId(&PyId_stdout);
        if (file == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
            return NULL;
        }
    }
    if (_print_message(file, "Class: ")) {
        return NULL;
    }
    if (_print_message(file, ((PyTypeObject *)self)->tp_name)) {
        return NULL;
    }
    if (_print_message(file, "\n")) {
        return NULL;
    }
    if (!_dump_attr(file, self->abc_registry, "Registry: ")) {
        return NULL;
    }
    if (!_dump_attr(file, self->abc_cache, "Positive cache: ")) {
        return NULL;
    }
    if (!_dump_attr(file, self->abc_negative_cache, "Negative cache: ")) {
        return NULL;
    }
    version = PyLong_FromSsize_t(self->abc_negative_cache_version);
    if (!version) {
        return NULL;
    }
    if (_print_message(file, "Negative cache version: ")) {
        Py_DECREF(version);
        return NULL;
    }
    if (PyFile_WriteObject(version, file, Py_PRINT_RAW)) {
        Py_DECREF(version);
        return NULL;
    }
    if (_print_message(file, "\n")) {
        Py_DECREF(version);
        return NULL;
    }
    Py_DECREF(version);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(_register_doc,
"Register a virtual subclass of an ABC.\n\
\n\
Returns the subclass, to allow usage as a class decorator.");

static PyMethodDef abcmeta_methods[] = {
    {"register", (PyCFunction)abcmeta_register, METH_VARARGS,
        _register_doc},
    {"__instancecheck__", (PyCFunction)abcmeta_instancecheck, METH_VARARGS,
        PyDoc_STR("Override for isinstance(instance, cls).")},
    {"__subclasscheck__", (PyCFunction)abcmeta_subclasscheck, METH_VARARGS,
        PyDoc_STR("Override for issubclass(subclass, cls).")},
    {"_dump_registry", (PyCFunction)abcmeta_dump, METH_VARARGS,
        PyDoc_STR("Debug helper to print the ABC registry.")},
    {NULL,      NULL},
};

static PyMemberDef abc_members[] = {
    {"_abc_registry", T_OBJECT, offsetof(abc, abc_registry), READONLY,
     PyDoc_STR("ABC registry (private).")},
    {"_abc_cache", T_OBJECT, offsetof(abc, abc_cache), READONLY,
     PyDoc_STR("ABC positive cache (private).")},
    {"_abc_negative_cache", T_OBJECT, offsetof(abc, abc_negative_cache), READONLY,
     PyDoc_STR("ABC negative cache (private).")},
    {"_abc_negative_cache_version", T_OBJECT, offsetof(abc, abc_negative_cache), READONLY,
     PyDoc_STR("ABC negative cache version (private).")},
    {NULL}
};

PyDoc_STRVAR(abcmeta_doc,
 "Metaclass for defining Abstract Base Classes (ABCs).\n\
\n\
Use this metaclass to create an ABC.  An ABC can be subclassed\n\
directly, and then acts as a mix-in class.  You can also register\n\
unrelated concrete classes (even built-in classes) and unrelated\n\
ABCs as 'virtual subclasses' -- these and their descendants will\n\
be considered subclasses of the registering ABC by the built-in\n\
issubclass() function, but the registering ABC won't show up in\n\
their MRO (Method Resolution Order) nor will method\n\
implementations defined by the registering ABC be callable (not\n\
even via super()).");

PyTypeObject ABCMeta = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "ABCMeta",                                  /* tp_name */
    sizeof(abc),                                /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)abcmeta_dealloc,                /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_VERSION_TAG |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_TYPE_SUBCLASS,         /* tp_flags */
    abcmeta_doc,                                /* tp_doc */
    abcmeta_traverse,                           /* tp_traverse */
    (inquiry)abcmeta_clear,                     /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    abcmeta_methods,                            /* tp_methods */
    abc_members,                                /* tp_members */
    0,                                          /* tp_getset */
    DEFERRED_ADDRESS(&PyType_Type),             /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    abcmeta_new,                                /* tp_new */
    0,                                          /* tp_free */
    0,                                          /* tp_is_gc */
};

PyDoc_STRVAR(_cache_token_doc,
"Returns the current ABC cache token.\n\
\n\
The token is an opaque object (supporting equality testing) identifying the\n\
current version of the ABC cache for virtual subclasses. The token changes\n\
with every call to ``register()`` on any ABC.");

static PyObject *
get_cache_token(void)
{
    return PyLong_FromSsize_t(abc_invalidation_counter);
}

static struct PyMethodDef module_functions[] = {
    {"get_cache_token", (PyCFunction)get_cache_token, METH_NOARGS,
        _cache_token_doc},
    {NULL,       NULL}          /* sentinel */
};

static struct PyModuleDef _abcmodule = {
    PyModuleDef_HEAD_INIT,
    "_abc",
    _abc__doc__,
    -1,
    module_functions,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__abc(void)
{
    PyObject *m;

    m = PyModule_Create(&_abcmodule);
    if (m == NULL)
        return NULL;
    ABCMeta.tp_base = &PyType_Type;
    if (PyType_Ready(&ABCMeta) < 0) {
        return NULL;
    }
    Py_INCREF(&ABCMeta);
    if (PyModule_AddObject(m, "ABCMeta",
                           (PyObject *) &ABCMeta) < 0) {
        return NULL;
    }
    return m;
}
