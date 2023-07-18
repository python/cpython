
/* Module object implementation */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_interp.h"        // PyInterpreterState.importlib
#include "pycore_object.h"        // _PyType_AllocNoTrack
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_moduleobject.h"  // _PyModule_GetDef()
#include "structmember.h"         // PyMemberDef


static PyMemberDef module_members[] = {
    {"__dict__", T_OBJECT, offsetof(PyModuleObject, md_dict), READONLY},
    {0}
};


PyTypeObject PyModuleDef_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "moduledef",                                /* tp_name */
    sizeof(PyModuleDef),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
};


int
_PyModule_IsExtension(PyObject *obj)
{
    if (!PyModule_Check(obj)) {
        return 0;
    }
    PyModuleObject *module = (PyModuleObject*)obj;

    PyModuleDef *def = module->md_def;
    return (def != NULL && def->m_methods != NULL);
}


PyObject*
PyModuleDef_Init(PyModuleDef* def)
{
    assert(PyModuleDef_Type.tp_flags & Py_TPFLAGS_READY);
    if (def->m_base.m_index == 0) {
        Py_SET_REFCNT(def, 1);
        Py_SET_TYPE(def, &PyModuleDef_Type);
        def->m_base.m_index = _PyImport_GetNextModuleIndex();
    }
    return (PyObject*)def;
}

static int
module_init_dict(PyModuleObject *mod, PyObject *md_dict,
                 PyObject *name, PyObject *doc)
{
    assert(md_dict != NULL);
    if (doc == NULL)
        doc = Py_None;

    if (PyDict_SetItem(md_dict, &_Py_ID(__name__), name) != 0)
        return -1;
    if (PyDict_SetItem(md_dict, &_Py_ID(__doc__), doc) != 0)
        return -1;
    if (PyDict_SetItem(md_dict, &_Py_ID(__package__), Py_None) != 0)
        return -1;
    if (PyDict_SetItem(md_dict, &_Py_ID(__loader__), Py_None) != 0)
        return -1;
    if (PyDict_SetItem(md_dict, &_Py_ID(__spec__), Py_None) != 0)
        return -1;
    if (PyUnicode_CheckExact(name)) {
        Py_XSETREF(mod->md_name, Py_NewRef(name));
    }

    return 0;
}

static PyModuleObject *
new_module_notrack(PyTypeObject *mt)
{
    PyModuleObject *m;
    m = (PyModuleObject *)_PyType_AllocNoTrack(mt, 0);
    if (m == NULL)
        return NULL;
    m->md_def = NULL;
    m->md_state = NULL;
    m->md_weaklist = NULL;
    m->md_name = NULL;
    m->md_dict = PyDict_New();
    if (m->md_dict != NULL) {
        return m;
    }
    Py_DECREF(m);
    return NULL;
}

static PyObject *
new_module(PyTypeObject *mt, PyObject *args, PyObject *kws)
{
    PyObject *m = (PyObject *)new_module_notrack(mt);
    if (m != NULL) {
        PyObject_GC_Track(m);
    }
    return m;
}

PyObject *
PyModule_NewObject(PyObject *name)
{
    PyModuleObject *m = new_module_notrack(&PyModule_Type);
    if (m == NULL)
        return NULL;
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
PyModule_Create2(PyModuleDef* module, int module_api_version)
{
    if (!_PyImport_IsInitialized(_PyInterpreterState_GET())) {
        PyErr_SetString(PyExc_SystemError,
                        "Python import machinery not initialized");
        return NULL;
    }
    return _PyModule_CreateInitialized(module, module_api_version);
}

PyObject *
_PyModule_CreateInitialized(PyModuleDef* module, int module_api_version)
{
    const char* name;
    PyModuleObject *m;

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
    name = _PyImport_ResolveNameWithPackageContext(name);
    if ((m = (PyModuleObject*)PyModule_New(name)) == NULL)
        return NULL;

    if (module->m_size > 0) {
        m->md_state = PyMem_Malloc(module->m_size);
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
PyModule_FromDefAndSpec2(PyModuleDef* def, PyObject *spec, int module_api_version)
{
    PyModuleDef_Slot* cur_slot;
    PyObject *(*create)(PyObject *, PyModuleDef*) = NULL;
    PyObject *nameobj;
    PyObject *m = NULL;
    int has_multiple_interpreters_slot = 0;
    void *multiple_interpreters = (void *)0;
    int has_execution_slots = 0;
    const char *name;
    int ret;
    PyInterpreterState *interp = _PyInterpreterState_GET();

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
        switch (cur_slot->slot) {
            case Py_mod_create:
                if (create) {
                    PyErr_Format(
                        PyExc_SystemError,
                        "module %s has multiple create slots",
                        name);
                    goto error;
                }
                create = cur_slot->value;
                break;
            case Py_mod_exec:
                has_execution_slots = 1;
                break;
            case Py_mod_multiple_interpreters:
                if (has_multiple_interpreters_slot) {
                    PyErr_Format(
                        PyExc_SystemError,
                        "module %s has more than one 'multiple interpreters' slots",
                        name);
                    goto error;
                }
                multiple_interpreters = cur_slot->value;
                has_multiple_interpreters_slot = 1;
                break;
            default:
                assert(cur_slot->slot < 0 || cur_slot->slot > _Py_mod_LAST_SLOT);
                PyErr_Format(
                    PyExc_SystemError,
                    "module %s uses unknown slot ID %i",
                    name, cur_slot->slot);
                goto error;
        }
    }

    /* By default, multi-phase init modules are expected
       to work under multiple interpreters. */
    if (!has_multiple_interpreters_slot) {
        multiple_interpreters = Py_MOD_MULTIPLE_INTERPRETERS_SUPPORTED;
    }
    if (multiple_interpreters == Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED) {
        if (!_Py_IsMainInterpreter(interp)
            && _PyImport_CheckSubinterpIncompatibleExtensionAllowed(name) < 0)
        {
            goto error;
        }
    }
    else if (multiple_interpreters != Py_MOD_PER_INTERPRETER_GIL_SUPPORTED
             && interp->ceval.own_gil
             && !_Py_IsMainInterpreter(interp)
             && _PyImport_CheckSubinterpIncompatibleExtensionAllowed(name) < 0)
    {
        goto error;
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
                _PyErr_FormatFromCause(
                    PyExc_SystemError,
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
            md->md_state = PyMem_Malloc(def->m_size);
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
                    _PyErr_FormatFromCause(
                        PyExc_SystemError,
                        "execution of module %s raised unreported exception",
                        name);
                    return -1;
                }
                break;
            case Py_mod_multiple_interpreters:
                /* handled in PyModule_FromDefAndSpec2 */
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

    v = PyUnicode_FromString(doc);
    if (v == NULL || PyObject_SetAttr(m, &_Py_ID(__doc__), v) != 0) {
        Py_XDECREF(v);
        return -1;
    }
    Py_DECREF(v);
    return 0;
}

PyObject *
PyModule_GetDict(PyObject *m)
{
    if (!PyModule_Check(m)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return _PyModule_GetDict(m);
}

PyObject*
PyModule_GetNameObject(PyObject *m)
{
    PyObject *d;
    PyObject *name;
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    d = ((PyModuleObject *)m)->md_dict;
    if (d == NULL || !PyDict_Check(d) ||
        (name = PyDict_GetItemWithError(d, &_Py_ID(__name__))) == NULL ||
        !PyUnicode_Check(name))
    {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "nameless module");
        }
        return NULL;
    }
    return Py_NewRef(name);
}

const char *
PyModule_GetName(PyObject *m)
{
    PyObject *name = PyModule_GetNameObject(m);
    if (name == NULL) {
        return NULL;
    }
    assert(Py_REFCNT(name) >= 2);
    Py_DECREF(name);   /* module dict has still a reference */
    return PyUnicode_AsUTF8(name);
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
        (fileobj = PyDict_GetItemWithError(d, &_Py_ID(__file__))) == NULL ||
        !PyUnicode_Check(fileobj))
    {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "module filename missing");
        }
        return NULL;
    }
    return Py_NewRef(fileobj);
}

const char *
PyModule_GetFilename(PyObject *m)
{
    PyObject *fileobj;
    const char *utf8;
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
    return _PyModule_GetDef(m);
}

void*
PyModule_GetState(PyObject* m)
{
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    return _PyModule_GetState(m);
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

    int verbose = _Py_GetConfig()->verbose;

    /* First, clear only names starting with a single underscore */
    pos = 0;
    while (PyDict_Next(d, &pos, &key, &value)) {
        if (value != Py_None && PyUnicode_Check(key)) {
            if (PyUnicode_READ_CHAR(key, 0) == '_' &&
                PyUnicode_READ_CHAR(key, 1) != '_') {
                if (verbose > 1) {
                    const char *s = PyUnicode_AsUTF8(key);
                    if (s != NULL)
                        PySys_WriteStderr("#   clear[1] %s\n", s);
                    else
                        PyErr_Clear();
                }
                if (PyDict_SetItem(d, key, Py_None) != 0) {
                    PyErr_WriteUnraisable(NULL);
                }
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
                if (verbose > 1) {
                    const char *s = PyUnicode_AsUTF8(key);
                    if (s != NULL)
                        PySys_WriteStderr("#   clear[2] %s\n", s);
                    else
                        PyErr_Clear();
                }
                if (PyDict_SetItem(d, key, Py_None) != 0) {
                    PyErr_WriteUnraisable(NULL);
                }
            }
        }
    }

    /* Note: we leave __builtins__ in place, so that destructors
       of non-global objects defined in this module can still use
       builtins, in particularly 'None'. */

}

/*[clinic input]
class module "PyModuleObject *" "&PyModule_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3e35d4f708ecb6af]*/

#include "clinic/moduleobject.c.h"

/* Methods */

/*[clinic input]
module.__init__
    name: unicode
    doc: object = None

Create a module object.

The name must be a string; the optional doc argument can have any type.
[clinic start generated code]*/

static int
module___init___impl(PyModuleObject *self, PyObject *name, PyObject *doc)
/*[clinic end generated code: output=e7e721c26ce7aad7 input=57f9e177401e5e1e]*/
{
    PyObject *dict = self->md_dict;
    if (dict == NULL) {
        dict = PyDict_New();
        if (dict == NULL)
            return -1;
        self->md_dict = dict;
    }
    if (module_init_dict(self, dict, name, doc) < 0)
        return -1;
    return 0;
}

static void
module_dealloc(PyModuleObject *m)
{
    int verbose = _Py_GetConfig()->verbose;

    PyObject_GC_UnTrack(m);
    if (verbose && m->md_name) {
        PySys_FormatStderr("# destroy %U\n", m->md_name);
    }
    if (m->md_weaklist != NULL)
        PyObject_ClearWeakRefs((PyObject *) m);
    /* bpo-39824: Don't call m_free() if m_size > 0 and md_state=NULL */
    if (m->md_def && m->md_def->m_free
        && (m->md_def->m_size <= 0 || m->md_state != NULL))
    {
        m->md_def->m_free(m);
    }
    Py_XDECREF(m->md_dict);
    Py_XDECREF(m->md_name);
    if (m->md_state != NULL)
        PyMem_Free(m->md_state);
    Py_TYPE(m)->tp_free((PyObject *)m);
}

static PyObject *
module_repr(PyModuleObject *m)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return _PyImport_ImportlibModuleRepr(interp, (PyObject *)m);
}

/* Check if the "_initializing" attribute of the module spec is set to true.
   Clear the exception and return 0 if spec is NULL.
 */
int
_PyModuleSpec_IsInitializing(PyObject *spec)
{
    if (spec != NULL) {
        PyObject *value;
        int ok = _PyObject_LookupAttr(spec, &_Py_ID(_initializing), &value);
        if (ok == 0) {
            return 0;
        }
        if (value != NULL) {
            int initializing = PyObject_IsTrue(value);
            Py_DECREF(value);
            if (initializing >= 0) {
                return initializing;
            }
        }
    }
    PyErr_Clear();
    return 0;
}

/* Check if the submodule name is in the "_uninitialized_submodules" attribute
   of the module spec.
 */
int
_PyModuleSpec_IsUninitializedSubmodule(PyObject *spec, PyObject *name)
{
    if (spec == NULL) {
         return 0;
    }

    PyObject *value = PyObject_GetAttr(spec, &_Py_ID(_uninitialized_submodules));
    if (value == NULL) {
        return 0;
    }

    int is_uninitialized = PySequence_Contains(value, name);
    Py_DECREF(value);
    if (is_uninitialized == -1) {
        return 0;
    }
    return is_uninitialized;
}

PyObject*
_Py_module_getattro_impl(PyModuleObject *m, PyObject *name, int suppress)
{
    // When suppress=1, this function suppresses AttributeError.
    PyObject *attr, *mod_name, *getattr;
    attr = _PyObject_GenericGetAttrWithDict((PyObject *)m, name, NULL, suppress);
    if (attr) {
        return attr;
    }
    if (suppress == 1) {
        if (PyErr_Occurred()) {
            // pass up non-AttributeError exception
            return NULL;
        }
    }
    else {
        if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
            // pass up non-AttributeError exception
            return NULL;
        }
        PyErr_Clear();
    }
    assert(m->md_dict != NULL);
    getattr = PyDict_GetItemWithError(m->md_dict, &_Py_ID(__getattr__));
    if (getattr) {
        PyObject *result = PyObject_CallOneArg(getattr, name);
        if (result == NULL && suppress == 1 && PyErr_ExceptionMatches(PyExc_AttributeError)) {
            // suppress AttributeError
            PyErr_Clear();
        }
        return result;
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    mod_name = PyDict_GetItemWithError(m->md_dict, &_Py_ID(__name__));
    if (mod_name && PyUnicode_Check(mod_name)) {
        Py_INCREF(mod_name);
        PyObject *spec = PyDict_GetItemWithError(m->md_dict, &_Py_ID(__spec__));
        if (spec == NULL && PyErr_Occurred()) {
            Py_DECREF(mod_name);
            return NULL;
        }
        if (suppress != 1) {
            Py_XINCREF(spec);
            if (_PyModuleSpec_IsInitializing(spec)) {
                PyErr_Format(PyExc_AttributeError,
                                "partially initialized "
                                "module '%U' has no attribute '%U' "
                                "(most likely due to a circular import)",
                                mod_name, name);
            }
            else if (_PyModuleSpec_IsUninitializedSubmodule(spec, name)) {
                PyErr_Format(PyExc_AttributeError,
                                "cannot access submodule '%U' of module '%U' "
                                "(most likely due to a circular import)",
                                name, mod_name);
            }
            else {
                PyErr_Format(PyExc_AttributeError,
                                "module '%U' has no attribute '%U'",
                                mod_name, name);
            }
            Py_XDECREF(spec);
        }
        Py_DECREF(mod_name);
        return NULL;
    }
    else if (PyErr_Occurred()) {
        return NULL;
    }
    if (suppress != 1) {
        PyErr_Format(PyExc_AttributeError,
                    "module has no attribute '%U'", name);
    }
    return NULL;
}


PyObject*
_Py_module_getattro(PyModuleObject *m, PyObject *name)
{
    return _Py_module_getattro_impl(m, name, 0);
}

static int
module_traverse(PyModuleObject *m, visitproc visit, void *arg)
{
    /* bpo-39824: Don't call m_traverse() if m_size > 0 and md_state=NULL */
    if (m->md_def && m->md_def->m_traverse
        && (m->md_def->m_size <= 0 || m->md_state != NULL))
    {
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
    /* bpo-39824: Don't call m_clear() if m_size > 0 and md_state=NULL */
    if (m->md_def && m->md_def->m_clear
        && (m->md_def->m_size <= 0 || m->md_state != NULL))
    {
        int res = m->md_def->m_clear((PyObject*)m);
        if (PyErr_Occurred()) {
            PySys_FormatStderr("Exception ignored in m_clear of module%s%V\n",
                               m->md_name ? " " : "",
                               m->md_name, "");
            PyErr_WriteUnraisable(NULL);
        }
        if (res)
            return res;
    }
    Py_CLEAR(m->md_dict);
    return 0;
}

static PyObject *
module_dir(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    PyObject *dict = PyObject_GetAttr(self, &_Py_ID(__dict__));

    if (dict != NULL) {
        if (PyDict_Check(dict)) {
            PyObject *dirfunc = PyDict_GetItemWithError(dict, &_Py_ID(__dir__));
            if (dirfunc) {
                result = _PyObject_CallNoArgs(dirfunc);
            }
            else if (!PyErr_Occurred()) {
                result = PyDict_Keys(dict);
            }
        }
        else {
            PyErr_Format(PyExc_TypeError, "<module>.__dict__ is not a dictionary");
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

static PyObject *
module_get_annotations(PyModuleObject *m, void *Py_UNUSED(ignored))
{
    PyObject *dict = PyObject_GetAttr((PyObject *)m, &_Py_ID(__dict__));
    if (dict == NULL) {
        return NULL;
    }
    if (!PyDict_Check(dict)) {
        PyErr_Format(PyExc_TypeError, "<module>.__dict__ is not a dictionary");
        Py_DECREF(dict);
        return NULL;
    }

    PyObject *annotations = PyDict_GetItemWithError(dict, &_Py_ID(__annotations__));
    if (annotations) {
        Py_INCREF(annotations);
    }
    else if (!PyErr_Occurred()) {
        annotations = PyDict_New();
        if (annotations) {
            int result = PyDict_SetItem(
                    dict, &_Py_ID(__annotations__), annotations);
            if (result) {
                Py_CLEAR(annotations);
            }
        }
    }
    Py_DECREF(dict);
    return annotations;
}

static int
module_set_annotations(PyModuleObject *m, PyObject *value, void *Py_UNUSED(ignored))
{
    int ret = -1;
    PyObject *dict = PyObject_GetAttr((PyObject *)m, &_Py_ID(__dict__));
    if (dict == NULL) {
        return -1;
    }
    if (!PyDict_Check(dict)) {
        PyErr_Format(PyExc_TypeError, "<module>.__dict__ is not a dictionary");
        goto exit;
    }

    if (value != NULL) {
        /* set */
        ret = PyDict_SetItem(dict, &_Py_ID(__annotations__), value);
    }
    else {
        /* delete */
        ret = PyDict_DelItem(dict, &_Py_ID(__annotations__));
        if (ret < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_SetString(PyExc_AttributeError, "__annotations__");
        }
    }

exit:
    Py_DECREF(dict);
    return ret;
}


static PyGetSetDef module_getsets[] = {
    {"__annotations__", (getter)module_get_annotations, (setter)module_set_annotations},
    {NULL}
};

PyTypeObject PyModule_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "module",                                   /* tp_name */
    sizeof(PyModuleObject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)module_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)module_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    (getattrofunc)_Py_module_getattro,          /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    module___init____doc__,                     /* tp_doc */
    (traverseproc)module_traverse,              /* tp_traverse */
    (inquiry)module_clear,                      /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyModuleObject, md_weaklist),      /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    module_methods,                             /* tp_methods */
    module_members,                             /* tp_members */
    module_getsets,                             /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(PyModuleObject, md_dict),          /* tp_dictoffset */
    module___init__,                            /* tp_init */
    0,                                          /* tp_alloc */
    new_module,                                 /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};
