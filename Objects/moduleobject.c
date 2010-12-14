
/* Module object implementation */

#include "Python.h"
#include "structmember.h"

static Py_ssize_t max_module_number;

typedef struct {
    PyObject_HEAD
    PyObject *md_dict;
    struct PyModuleDef *md_def;
    void *md_state;
} PyModuleObject;

static PyMemberDef module_members[] = {
    {"__dict__", T_OBJECT, offsetof(PyModuleObject, md_dict), READONLY},
    {0}
};

static PyTypeObject moduledef_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "moduledef",                                /* tp_name */
    sizeof(struct PyModuleDef),                 /* tp_size */
    0,                                          /* tp_itemsize */
};


PyObject *
PyModule_New(const char *name)
{
    PyModuleObject *m;
    PyObject *nameobj;
    m = PyObject_GC_New(PyModuleObject, &PyModule_Type);
    if (m == NULL)
        return NULL;
    m->md_def = NULL;
    m->md_state = NULL;
    nameobj = PyUnicode_FromString(name);
    m->md_dict = PyDict_New();
    if (m->md_dict == NULL || nameobj == NULL)
        goto fail;
    if (PyDict_SetItemString(m->md_dict, "__name__", nameobj) != 0)
        goto fail;
    if (PyDict_SetItemString(m->md_dict, "__doc__", Py_None) != 0)
        goto fail;
    if (PyDict_SetItemString(m->md_dict, "__package__", Py_None) != 0)
        goto fail;
    Py_DECREF(nameobj);
    PyObject_GC_Track(m);
    return (PyObject *)m;

 fail:
    Py_XDECREF(nameobj);
    Py_DECREF(m);
    return NULL;
}

PyObject *
PyModule_Create2(struct PyModuleDef* module, int module_api_version)
{
    PyObject *d, *v, *n;
    PyMethodDef *ml;
    const char* name;
    PyModuleObject *m;
    PyInterpreterState *interp = PyThreadState_Get()->interp;
    if (interp->modules == NULL)
        Py_FatalError("Python import machinery not initialized");
    if (PyType_Ready(&moduledef_type) < 0)
        return NULL;
    if (module->m_base.m_index == 0) {
        max_module_number++;
        Py_REFCNT(module) = 1;
        Py_TYPE(module) = &moduledef_type;
        module->m_base.m_index = max_module_number;
    }
    name = module->m_name;
    if (module_api_version != PYTHON_API_VERSION && module_api_version != PYTHON_ABI_VERSION) {
        int err;
        err = PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
            "Python C API version mismatch for module %.100s: "
            "This Python has API version %d, module %.100s has version %d.",
             name,
             PYTHON_API_VERSION, name, module_api_version);
        if (err)
            return NULL;
    }
    /* Make sure name is fully qualified.

       This is a bit of a hack: when the shared library is loaded,
       the module name is "package.module", but the module calls
       PyModule_Create*() with just "module" for the name.  The shared
       library loader squirrels away the true name of the module in
       _Py_PackageContext, and PyModule_Create*() will substitute this
       (if the name actually matches).
    */
    if (_Py_PackageContext != NULL) {
        char *p = strrchr(_Py_PackageContext, '.');
        if (p != NULL && strcmp(module->m_name, p+1) == 0) {
            name = _Py_PackageContext;
            _Py_PackageContext = NULL;
        }
    }
    if ((m = (PyModuleObject*)PyModule_New(name)) == NULL)
        return NULL;

    if (module->m_size > 0) {
        m->md_state = PyMem_MALLOC(module->m_size);
        if (!m->md_state) {
            PyErr_NoMemory();
            Py_DECREF(m);
            return NULL;
        }
        memset(m->md_state, 0, module->m_size);
    }

    d = PyModule_GetDict((PyObject*)m);
    if (module->m_methods != NULL) {
        n = PyUnicode_FromString(name);
        if (n == NULL)
            return NULL;
        for (ml = module->m_methods; ml->ml_name != NULL; ml++) {
            if ((ml->ml_flags & METH_CLASS) ||
                (ml->ml_flags & METH_STATIC)) {
                PyErr_SetString(PyExc_ValueError,
                                "module functions cannot set"
                                " METH_CLASS or METH_STATIC");
                Py_DECREF(n);
                return NULL;
            }
            v = PyCFunction_NewEx(ml, (PyObject*)m, n);
            if (v == NULL) {
                Py_DECREF(n);
                return NULL;
            }
            if (PyDict_SetItemString(d, ml->ml_name, v) != 0) {
                Py_DECREF(v);
                Py_DECREF(n);
                return NULL;
            }
            Py_DECREF(v);
        }
        Py_DECREF(n);
    }
    if (module->m_doc != NULL) {
        v = PyUnicode_FromString(module->m_doc);
        if (v == NULL || PyDict_SetItemString(d, "__doc__", v) != 0) {
            Py_XDECREF(v);
            return NULL;
        }
        Py_DECREF(v);
    }
    m->md_def = module;
    return (PyObject*)m;
}


PyObject *
PyModule_GetDict(PyObject *m)
{
    PyObject *d;
    if (!PyModule_Check(m)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    d = ((PyModuleObject *)m) -> md_dict;
    if (d == NULL)
        ((PyModuleObject *)m) -> md_dict = d = PyDict_New();
    return d;
}

const char *
PyModule_GetName(PyObject *m)
{
    PyObject *d;
    PyObject *nameobj;
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    d = ((PyModuleObject *)m)->md_dict;
    if (d == NULL ||
        (nameobj = PyDict_GetItemString(d, "__name__")) == NULL ||
        !PyUnicode_Check(nameobj))
    {
        PyErr_SetString(PyExc_SystemError, "nameless module");
        return NULL;
    }
    return _PyUnicode_AsString(nameobj);
}

PyObject*
PyModule_GetFilenameObject(PyObject *m)
{
    PyObject *d;
    PyObject *fileobj;
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    d = ((PyModuleObject *)m)->md_dict;
    if (d == NULL ||
        (fileobj = PyDict_GetItemString(d, "__file__")) == NULL ||
        !PyUnicode_Check(fileobj))
    {
        PyErr_SetString(PyExc_SystemError, "module filename missing");
        return NULL;
    }
    Py_INCREF(fileobj);
    return fileobj;
}

const char *
PyModule_GetFilename(PyObject *m)
{
    PyObject *fileobj;
    char *utf8;
    fileobj = PyModule_GetFilenameObject(m);
    if (fileobj == NULL)
        return NULL;
    utf8 = _PyUnicode_AsString(fileobj);
    Py_DECREF(fileobj);
    return utf8;
}

PyModuleDef*
PyModule_GetDef(PyObject* m)
{
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    return ((PyModuleObject *)m)->md_def;
}

void*
PyModule_GetState(PyObject* m)
{
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    return ((PyModuleObject *)m)->md_state;
}

void
_PyModule_Clear(PyObject *m)
{
    /* To make the execution order of destructors for global
       objects a bit more predictable, we first zap all objects
       whose name starts with a single underscore, before we clear
       the entire dictionary.  We zap them by replacing them with
       None, rather than deleting them from the dictionary, to
       avoid rehashing the dictionary (to some extent). */

    Py_ssize_t pos;
    PyObject *key, *value;
    PyObject *d;

    d = ((PyModuleObject *)m)->md_dict;
    if (d == NULL)
        return;

    /* First, clear only names starting with a single underscore */
    pos = 0;
    while (PyDict_Next(d, &pos, &key, &value)) {
        if (value != Py_None && PyUnicode_Check(key)) {
            Py_UNICODE *u = PyUnicode_AS_UNICODE(key);
            if (u[0] == '_' && u[1] != '_') {
                if (Py_VerboseFlag > 1) {
                    const char *s = _PyUnicode_AsString(key);
                    if (s != NULL)
                        PySys_WriteStderr("#   clear[1] %s\n", s);
                    else
                        PyErr_Clear();
                }
                PyDict_SetItem(d, key, Py_None);
            }
        }
    }

    /* Next, clear all names except for __builtins__ */
    pos = 0;
    while (PyDict_Next(d, &pos, &key, &value)) {
        if (value != Py_None && PyUnicode_Check(key)) {
            Py_UNICODE *u = PyUnicode_AS_UNICODE(key);
            if (u[0] != '_'
                || PyUnicode_CompareWithASCIIString(key, "__builtins__") != 0)
            {
                if (Py_VerboseFlag > 1) {
                    const char *s = _PyUnicode_AsString(key);
                    if (s != NULL)
                        PySys_WriteStderr("#   clear[2] %s\n", s);
                    else
                        PyErr_Clear();
                }
                PyDict_SetItem(d, key, Py_None);
            }
        }
    }

    /* Note: we leave __builtins__ in place, so that destructors
       of non-global objects defined in this module can still use
       builtins, in particularly 'None'. */

}

/* Methods */

static int
module_init(PyModuleObject *m, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", "doc", NULL};
    PyObject *dict, *name = Py_None, *doc = Py_None;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "U|O:module.__init__",
                                     kwlist, &name, &doc))
        return -1;
    dict = m->md_dict;
    if (dict == NULL) {
        dict = PyDict_New();
        if (dict == NULL)
            return -1;
        m->md_dict = dict;
    }
    if (PyDict_SetItemString(dict, "__name__", name) < 0)
        return -1;
    if (PyDict_SetItemString(dict, "__doc__", doc) < 0)
        return -1;
    return 0;
}

static void
module_dealloc(PyModuleObject *m)
{
    PyObject_GC_UnTrack(m);
    if (m->md_def && m->md_def->m_free)
        m->md_def->m_free(m);
    if (m->md_dict != NULL) {
        _PyModule_Clear((PyObject *)m);
        Py_DECREF(m->md_dict);
    }
    if (m->md_state != NULL)
        PyMem_FREE(m->md_state);
    Py_TYPE(m)->tp_free((PyObject *)m);
}

static PyObject *
module_repr(PyModuleObject *m)
{
    const char *name;
    PyObject *filename, *repr;

    name = PyModule_GetName((PyObject *)m);
    if (name == NULL) {
        PyErr_Clear();
        name = "?";
    }
    filename = PyModule_GetFilenameObject((PyObject *)m);
    if (filename == NULL) {
        PyErr_Clear();
        return PyUnicode_FromFormat("<module '%s' (built-in)>", name);
    }
    repr = PyUnicode_FromFormat("<module '%s' from '%U'>", name, filename);
    Py_DECREF(filename);
    return repr;
}

static int
module_traverse(PyModuleObject *m, visitproc visit, void *arg)
{
    if (m->md_def && m->md_def->m_traverse) {
        int res = m->md_def->m_traverse((PyObject*)m, visit, arg);
        if (res)
            return res;
    }
    Py_VISIT(m->md_dict);
    return 0;
}

static int
module_clear(PyModuleObject *m)
{
    if (m->md_def && m->md_def->m_clear) {
        int res = m->md_def->m_clear((PyObject*)m);
        if (res)
            return res;
    }
    Py_CLEAR(m->md_dict);
    return 0;
}


PyDoc_STRVAR(module_doc,
"module(name[, doc])\n\
\n\
Create a module object.\n\
The name must be a string; the optional doc argument can have any type.");

PyTypeObject PyModule_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "module",                                   /* tp_name */
    sizeof(PyModuleObject),                     /* tp_size */
    0,                                          /* tp_itemsize */
    (destructor)module_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)module_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    module_doc,                                 /* tp_doc */
    (traverseproc)module_traverse,              /* tp_traverse */
    (inquiry)module_clear,                      /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    module_members,                             /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(PyModuleObject, md_dict),          /* tp_dictoffset */
    (initproc)module_init,                      /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};
