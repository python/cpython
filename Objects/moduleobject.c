
/* Module object implementation */

#include "Python.h"
#include "structmember.h"

static Py_ssize_t max_module_number;

typedef struct {
    PyObject_HEAD
    PyObject *md_dict;
    struct PyModuleDef *md_def;
    void *md_state;
    PyObject *md_weaklist;
    PyObject *md_name;  /* for logging purposes after md_dict is cleared */
} PyModuleObject;

static PyMemberDef module_members[] = {
    {"__dict__", T_OBJECT, offsetof(PyModuleObject, md_dict), READONLY},
    {0}
};

PyTypeObject PyModuleDef_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "moduledef",                                /* tp_name */
    sizeof(struct PyModuleDef),                 /* tp_size */
    0,                                          /* tp_itemsize */
};


PyObject*
PyModuleDef_Init(struct PyModuleDef* def)
{
    if (PyType_Ready(&PyModuleDef_Type) < 0)
         return NULL;
    if (def->m_base.m_index == 0) {
        max_module_number++;
        Py_REFCNT(def) = 1;
        Py_TYPE(def) = &PyModuleDef_Type;
        def->m_base.m_index = max_module_number;
    }
    return (PyObject*)def;
}

static int
module_init_dict(PyModuleObject *mod, PyObject *md_dict,
                 PyObject *name, PyObject *doc)
{
    _Py_IDENTIFIER(__name__);
    _Py_IDENTIFIER(__doc__);
    _Py_IDENTIFIER(__package__);
    _Py_IDENTIFIER(__loader__);
    _Py_IDENTIFIER(__spec__);

    if (md_dict == NULL)
        return -1;
    if (doc == NULL)
        doc = Py_None;

    if (_PyDict_SetItemId(md_dict, &PyId___name__, name) != 0)
        return -1;
    if (_PyDict_SetItemId(md_dict, &PyId___doc__, doc) != 0)
        return -1;
    if (_PyDict_SetItemId(md_dict, &PyId___package__, Py_None) != 0)
        return -1;
    if (_PyDict_SetItemId(md_dict, &PyId___loader__, Py_None) != 0)
        return -1;
    if (_PyDict_SetItemId(md_dict, &PyId___spec__, Py_None) != 0)
        return -1;
    if (PyUnicode_CheckExact(name)) {
        Py_INCREF(name);
        Py_XSETREF(mod->md_name, name);
    }

    return 0;
}


PyObject *
PyModule_NewObject(PyObject *name)
{
    PyModuleObject *m;
    m = PyObject_GC_New(PyModuleObject, &PyModule_Type);
    if (m == NULL)
        return NULL;
    m->md_def = NULL;
    m->md_state = NULL;
    m->md_weaklist = NULL;
    m->md_name = NULL;
    m->md_dict = PyDict_New();
    if (module_init_dict(m, m->md_dict, name, NULL) != 0)
        goto fail;
    PyObject_GC_Track(m);
    return (PyObject *)m;

 fail:
    Py_DECREF(m);
    return NULL;
}

PyObject *
PyModule_New(const char *name)
{
    PyObject *nameobj, *module;
    nameobj = PyUnicode_FromString(name);
    if (nameobj == NULL)
        return NULL;
    module = PyModule_NewObject(nameobj);
    Py_DECREF(nameobj);
    return module;
}

/* Check API/ABI version
 * Issues a warning on mismatch, which is usually not fatal.
 * Returns 0 if an exception is raised.
 */
static int
check_api_version(const char *name, int module_api_version)
{
    if (module_api_version != PYTHON_API_VERSION && module_api_version != PYTHON_ABI_VERSION) {
        int err;
        err = PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
            "Python C API version mismatch for module %.100s: "
            "This Python has API version %d, module %.100s has version %d.",
             name,
             PYTHON_API_VERSION, name, module_api_version);
        if (err)
            return 0;
    }
    return 1;
}

static int
_add_methods_to_object(PyObject *module, PyObject *name, PyMethodDef *functions)
{
    PyObject *func;
    PyMethodDef *fdef;

    for (fdef = functions; fdef->ml_name != NULL; fdef++) {
        if ((fdef->ml_flags & METH_CLASS) ||
            (fdef->ml_flags & METH_STATIC)) {
            PyErr_SetString(PyExc_ValueError,
                            "module functions cannot set"
                            " METH_CLASS or METH_STATIC");
            return -1;
        }
        func = PyCFunction_NewEx(fdef, (PyObject*)module, name);
        if (func == NULL) {
            return -1;
        }
        if (PyObject_SetAttrString(module, fdef->ml_name, func) != 0) {
            Py_DECREF(func);
            return -1;
        }
        Py_DECREF(func);
    }

    return 0;
}

PyObject *
PyModule_Create2(struct PyModuleDef* module, int module_api_version)
{
    const char* name;
    PyModuleObject *m;
    PyInterpreterState *interp = PyThreadState_Get()->interp;
    if (interp->modules == NULL)
        Py_FatalError("Python import machinery not initialized");
    if (!PyModuleDef_Init(module))
        return NULL;
    name = module->m_name;
    if (!check_api_version(name, module_api_version)) {
        return NULL;
    }
    if (module->m_slots) {
        PyErr_Format(
            PyExc_SystemError,
            "module %s: PyModule_Create is incompatible with m_slots", name);
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

    if (module->m_methods != NULL) {
        if (PyModule_AddFunctions((PyObject *) m, module->m_methods) != 0) {
            Py_DECREF(m);
            return NULL;
        }
    }
    if (module->m_doc != NULL) {
        if (PyModule_SetDocString((PyObject *) m, module->m_doc) != 0) {
            Py_DECREF(m);
            return NULL;
        }
    }
    m->md_def = module;
    return (PyObject*)m;
}

PyObject *
PyModule_FromDefAndSpec2(struct PyModuleDef* def, PyObject *spec, int module_api_version)
{
    PyModuleDef_Slot* cur_slot;
    PyObject *(*create)(PyObject *, PyModuleDef*) = NULL;
    PyObject *nameobj;
    PyObject *m = NULL;
    int has_execution_slots = 0;
    char *name;
    int ret;

    PyModuleDef_Init(def);

    nameobj = PyObject_GetAttrString(spec, "name");
    if (nameobj == NULL) {
        return NULL;
    }
    name = PyUnicode_AsUTF8(nameobj);
    if (name == NULL) {
        goto error;
    }

    if (!check_api_version(name, module_api_version)) {
        goto error;
    }

    if (def->m_size < 0) {
        PyErr_Format(
            PyExc_SystemError,
            "module %s: m_size may not be negative for multi-phase initialization",
            name);
        goto error;
    }

    for (cur_slot = def->m_slots; cur_slot && cur_slot->slot; cur_slot++) {
        if (cur_slot->slot == Py_mod_create) {
            if (create) {
                PyErr_Format(
                    PyExc_SystemError,
                    "module %s has multiple create slots",
                    name);
                goto error;
            }
            create = cur_slot->value;
        } else if (cur_slot->slot < 0 || cur_slot->slot > _Py_mod_LAST_SLOT) {
            PyErr_Format(
                PyExc_SystemError,
                "module %s uses unknown slot ID %i",
                name, cur_slot->slot);
            goto error;
        } else {
            has_execution_slots = 1;
        }
    }

    if (create) {
        m = create(spec, def);
        if (m == NULL) {
            if (!PyErr_Occurred()) {
                PyErr_Format(
                    PyExc_SystemError,
                    "creation of module %s failed without setting an exception",
                    name);
            }
            goto error;
        } else {
            if (PyErr_Occurred()) {
                PyErr_Format(PyExc_SystemError,
                            "creation of module %s raised unreported exception",
                            name);
                goto error;
            }
        }
    } else {
        m = PyModule_NewObject(nameobj);
        if (m == NULL) {
            goto error;
        }
    }

    if (PyModule_Check(m)) {
        ((PyModuleObject*)m)->md_state = NULL;
        ((PyModuleObject*)m)->md_def = def;
    } else {
        if (def->m_size > 0 || def->m_traverse || def->m_clear || def->m_free) {
            PyErr_Format(
                PyExc_SystemError,
                "module %s is not a module object, but requests module state",
                name);
            goto error;
        }
        if (has_execution_slots) {
            PyErr_Format(
                PyExc_SystemError,
                "module %s specifies execution slots, but did not create "
                    "a ModuleType instance",
                name);
            goto error;
        }
    }

    if (def->m_methods != NULL) {
        ret = _add_methods_to_object(m, nameobj, def->m_methods);
        if (ret != 0) {
            goto error;
        }
    }

    if (def->m_doc != NULL) {
        ret = PyModule_SetDocString(m, def->m_doc);
        if (ret != 0) {
            goto error;
        }
    }

    Py_DECREF(nameobj);
    return m;

error:
    Py_DECREF(nameobj);
    Py_XDECREF(m);
    return NULL;
}

int
PyModule_ExecDef(PyObject *module, PyModuleDef *def)
{
    PyModuleDef_Slot *cur_slot;
    const char *name;
    int ret;

    name = PyModule_GetName(module);
    if (name == NULL) {
        return -1;
    }

    if (def->m_size >= 0) {
        PyModuleObject *md = (PyModuleObject*)module;
        if (md->md_state == NULL) {
            /* Always set a state pointer; this serves as a marker to skip
             * multiple initialization (importlib.reload() is no-op) */
            md->md_state = PyMem_MALLOC(def->m_size);
            if (!md->md_state) {
                PyErr_NoMemory();
                return -1;
            }
            memset(md->md_state, 0, def->m_size);
        }
    }

    if (def->m_slots == NULL) {
        return 0;
    }

    for (cur_slot = def->m_slots; cur_slot && cur_slot->slot; cur_slot++) {
        switch (cur_slot->slot) {
            case Py_mod_create:
                /* handled in PyModule_FromDefAndSpec2 */
                break;
            case Py_mod_exec:
                ret = ((int (*)(PyObject *))cur_slot->value)(module);
                if (ret != 0) {
                    if (!PyErr_Occurred()) {
                        PyErr_Format(
                            PyExc_SystemError,
                            "execution of module %s failed without setting an exception",
                            name);
                    }
                    return -1;
                }
                if (PyErr_Occurred()) {
                    PyErr_Format(
                        PyExc_SystemError,
                        "execution of module %s raised unreported exception",
                        name);
                    return -1;
                }
                break;
            default:
                PyErr_Format(
                    PyExc_SystemError,
                    "module %s initialized with unknown slot %i",
                    name, cur_slot->slot);
                return -1;
        }
    }
    return 0;
}

int
PyModule_AddFunctions(PyObject *m, PyMethodDef *functions)
{
    int res;
    PyObject *name = PyModule_GetNameObject(m);
    if (name == NULL) {
        return -1;
    }

    res = _add_methods_to_object(m, name, functions);
    Py_DECREF(name);
    return res;
}

int
PyModule_SetDocString(PyObject *m, const char *doc)
{
    PyObject *v;
    _Py_IDENTIFIER(__doc__);

    v = PyUnicode_FromString(doc);
    if (v == NULL || _PyObject_SetAttrId(m, &PyId___doc__, v) != 0) {
        Py_XDECREF(v);
        return -1;
    }
    Py_DECREF(v);
    return 0;
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
    assert(d != NULL);
    return d;
}

PyObject*
PyModule_GetNameObject(PyObject *m)
{
    _Py_IDENTIFIER(__name__);
    PyObject *d;
    PyObject *name;
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    d = ((PyModuleObject *)m)->md_dict;
    if (d == NULL ||
        (name = _PyDict_GetItemId(d, &PyId___name__)) == NULL ||
        !PyUnicode_Check(name))
    {
        PyErr_SetString(PyExc_SystemError, "nameless module");
        return NULL;
    }
    Py_INCREF(name);
    return name;
}

const char *
PyModule_GetName(PyObject *m)
{
    PyObject *name = PyModule_GetNameObject(m);
    if (name == NULL)
        return NULL;
    Py_DECREF(name);   /* module dict has still a reference */
    return PyUnicode_AsUTF8(name);
}

PyObject*
PyModule_GetFilenameObject(PyObject *m)
{
    _Py_IDENTIFIER(__file__);
    PyObject *d;
    PyObject *fileobj;
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    d = ((PyModuleObject *)m)->md_dict;
    if (d == NULL ||
        (fileobj = _PyDict_GetItemId(d, &PyId___file__)) == NULL ||
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
    utf8 = PyUnicode_AsUTF8(fileobj);
    Py_DECREF(fileobj);   /* module dict has still a reference */
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
    PyObject *d = ((PyModuleObject *)m)->md_dict;
    if (d != NULL)
        _PyModule_ClearDict(d);
}

void
_PyModule_ClearDict(PyObject *d)
{
    /* To make the execution order of destructors for global
       objects a bit more predictable, we first zap all objects
       whose name starts with a single underscore, before we clear
       the entire dictionary.  We zap them by replacing them with
       None, rather than deleting them from the dictionary, to
       avoid rehashing the dictionary (to some extent). */

    Py_ssize_t pos;
    PyObject *key, *value;

    /* First, clear only names starting with a single underscore */
    pos = 0;
    while (PyDict_Next(d, &pos, &key, &value)) {
        if (value != Py_None && PyUnicode_Check(key)) {
            if (PyUnicode_READ_CHAR(key, 0) == '_' &&
                PyUnicode_READ_CHAR(key, 1) != '_') {
                if (Py_VerboseFlag > 1) {
                    const char *s = PyUnicode_AsUTF8(key);
                    if (s != NULL)
                        PySys_WriteStderr("#   clear[1] %s\n", s);
                    else
                        PyErr_Clear();
                }
                if (PyDict_SetItem(d, key, Py_None) != 0)
                    PyErr_Clear();
            }
        }
    }

    /* Next, clear all names except for __builtins__ */
    pos = 0;
    while (PyDict_Next(d, &pos, &key, &value)) {
        if (value != Py_None && PyUnicode_Check(key)) {
            if (PyUnicode_READ_CHAR(key, 0) != '_' ||
                !_PyUnicode_EqualToASCIIString(key, "__builtins__"))
            {
                if (Py_VerboseFlag > 1) {
                    const char *s = PyUnicode_AsUTF8(key);
                    if (s != NULL)
                        PySys_WriteStderr("#   clear[2] %s\n", s);
                    else
                        PyErr_Clear();
                }
                if (PyDict_SetItem(d, key, Py_None) != 0)
                    PyErr_Clear();
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
    if (module_init_dict(m, dict, name, doc) < 0)
        return -1;
    return 0;
}

static void
module_dealloc(PyModuleObject *m)
{
    PyObject_GC_UnTrack(m);
    if (Py_VerboseFlag && m->md_name) {
        PySys_FormatStderr("# destroy %S\n", m->md_name);
    }
    if (m->md_weaklist != NULL)
        PyObject_ClearWeakRefs((PyObject *) m);
    if (m->md_def && m->md_def->m_free)
        m->md_def->m_free(m);
    Py_XDECREF(m->md_dict);
    Py_XDECREF(m->md_name);
    if (m->md_state != NULL)
        PyMem_FREE(m->md_state);
    Py_TYPE(m)->tp_free((PyObject *)m);
}

static PyObject *
module_repr(PyModuleObject *m)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;

    return PyObject_CallMethod(interp->importlib, "_module_repr", "O", m);
}

static PyObject*
module_getattro(PyModuleObject *m, PyObject *name)
{
    PyObject *attr, *mod_name;
    attr = PyObject_GenericGetAttr((PyObject *)m, name);
    if (attr || !PyErr_ExceptionMatches(PyExc_AttributeError))
        return attr;
    PyErr_Clear();
    if (m->md_dict) {
        _Py_IDENTIFIER(__name__);
        mod_name = _PyDict_GetItemId(m->md_dict, &PyId___name__);
        if (mod_name) {
            PyErr_Format(PyExc_AttributeError,
                        "module '%U' has no attribute '%U'", mod_name, name);
            return NULL;
        }
        else if (PyErr_Occurred()) {
            PyErr_Clear();
        }
    }
    PyErr_Format(PyExc_AttributeError,
                "module has no attribute '%U'", name);
    return NULL;
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

static PyObject *
module_dir(PyObject *self, PyObject *args)
{
    _Py_IDENTIFIER(__dict__);
    PyObject *result = NULL;
    PyObject *dict = _PyObject_GetAttrId(self, &PyId___dict__);

    if (dict != NULL) {
        if (PyDict_Check(dict))
            result = PyDict_Keys(dict);
        else {
            const char *name = PyModule_GetName(self);
            if (name)
                PyErr_Format(PyExc_TypeError,
                             "%.200s.__dict__ is not a dictionary",
                             name);
        }
    }

    Py_XDECREF(dict);
    return result;
}

static PyMethodDef module_methods[] = {
    {"__dir__", module_dir, METH_NOARGS,
     PyDoc_STR("__dir__() -> list\nspecialized dir() implementation")},
    {0}
};

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
    (getattrofunc)module_getattro,              /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    module_doc,                                 /* tp_doc */
    (traverseproc)module_traverse,              /* tp_traverse */
    (inquiry)module_clear,                      /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyModuleObject, md_weaklist),      /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    module_methods,                             /* tp_methods */
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
