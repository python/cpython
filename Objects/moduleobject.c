/* Module object implementation */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_EnableGILTransient()
#include "pycore_dict.h"          // _PyDict_EnablePerThreadRefcounting()
#include "pycore_fileutils.h"     // _Py_wgetcwd
#include "pycore_import.h"        // _PyImport_GetNextModuleIndex()
#include "pycore_interp.h"        // PyInterpreterState.importlib
#include "pycore_long.h"          // _PyLong_GetOne()
#include "pycore_modsupport.h"    // _PyModule_CreateInitialized()
#include "pycore_moduleobject.h"  // _PyModule_GetDefOrNull()
#include "pycore_object.h"        // _PyType_AllocNoTrack
#include "pycore_pyerrors.h"      // _PyErr_FormatFromCause()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

#include "osdefs.h"               // MAXPATHLEN


#define _PyModule_CAST(op) \
    (assert(PyModule_Check(op)), _Py_CAST(PyModuleObject*, (op)))


static PyMemberDef module_members[] = {
    {"__dict__", _Py_T_OBJECT, offsetof(PyModuleObject, md_dict), Py_READONLY},
    {0}
};

static void
assert_def_missing_or_redundant(PyModuleObject *m)
{
    /* We copy all relevant info into the module object.
     * Modules created using a def keep a reference to that (statically
     * allocated) def; the info there should match what we have in the module.
     */
#ifndef NDEBUG
    if (m->md_token_is_def) {
        PyModuleDef *def = (PyModuleDef *)m->md_token;
        assert(def);
#define DO_ASSERT(F) assert (def->m_ ## F == m->md_state_ ## F);
        DO_ASSERT(size);
        DO_ASSERT(traverse);
        DO_ASSERT(clear);
        DO_ASSERT(free);
#undef DO_ASSERT
    }
#endif // NDEBUG
}


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

    if (module->md_exec) {
        return 1;
    }
    if (module->md_token_is_def) {
        PyModuleDef *def = (PyModuleDef *)module->md_token;
        return (module->md_token_is_def && def->m_methods != NULL);
    }
    return 0;
}


PyObject*
PyModuleDef_Init(PyModuleDef* def)
{
#ifdef Py_GIL_DISABLED
    // Check that this def does not come from a non-free-threading ABI.
    //
    // This is meant as a "sanity check"; users should never rely on it.
    // In particular, if we run out of ob_flags bits, or otherwise need to
    // change some of the internals, this check can go away. Still, it
    // would be nice to keep it for the free-threading transition.
    //
    // A PyModuleDef must be initialized with PyModuleDef_HEAD_INIT,
    // which (via PyObject_HEAD_INIT) sets _Py_STATICALLY_ALLOCATED_FLAG
    // and not _Py_LEGACY_ABI_CHECK_FLAG. For PyModuleDef, these flags never
    // change.
    // This means that the lower nibble of a valid PyModuleDef's ob_flags is
    // always `_10_` (in binary; `_` is don't care).
    //
    // So, a check for these bits won't reject valid PyModuleDef.
    // Rejecting incompatible extensions is slightly less important; here's
    // how that works:
    //
    // In the pre-free-threading stable ABI, PyModuleDef_HEAD_INIT is big
    // enough to overlap with free-threading ABI's ob_flags, is all zeros
    // except for the refcount field.
    // The refcount field can be:
    // - 1 (3.11 and below)
    // - UINT_MAX >> 2 (32-bit 3.12 & 3.13)
    // - UINT_MAX (64-bit 3.12 & 3.13)
    // - 7L << 28 (3.14)
    //
    // This means that the lower nibble of *any byte* in PyModuleDef_HEAD_INIT
    // is not `_10_` -- it can be:
    // - 0b0000
    // - 0b0001
    // - 0b0011 (from UINT_MAX >> 2)
    // - 0b0111 (from 7L << 28)
    // - 0b1111 (e.g. from UINT_MAX)
    // (The values may change at runtime as the PyModuleDef is used, but
    // PyModuleDef_Init is required before using the def as a Python object,
    // so we check at least once with the initial values.
    uint16_t flags = ((PyObject*)def)->ob_flags;
    uint16_t bits = _Py_STATICALLY_ALLOCATED_FLAG | _Py_LEGACY_ABI_CHECK_FLAG;
    if ((flags & bits) != _Py_STATICALLY_ALLOCATED_FLAG) {
        const char *message = "invalid PyModuleDef, extension possibly "
            "compiled for non-free-threaded Python";
        // Write the error as unraisable: if the extension tries calling
        // any API, it's likely to segfault and lose the exception.
        PyErr_SetString(PyExc_SystemError, message);
        PyErr_WriteUnraisable(NULL);
        // But also raise the exception normally -- this is technically
        // a recoverable state.
        PyErr_SetString(PyExc_SystemError, message);
        return NULL;
    }
#endif
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
    m->md_state = NULL;
    m->md_weaklist = NULL;
    m->md_name = NULL;
    m->md_token_is_def = false;
#ifdef Py_GIL_DISABLED
    m->md_requires_gil = true;
#endif
    m->md_state_size = 0;
    m->md_state_traverse = NULL;
    m->md_state_clear = NULL;
    m->md_state_free = NULL;
    m->md_exec = NULL;
    m->md_token = NULL;
    m->md_dict = PyDict_New();
    if (m->md_dict == NULL) {
        Py_DECREF(m);
        return NULL;
    }
    return m;
}

static void
track_module(PyModuleObject *m)
{
    _PyDict_EnablePerThreadRefcounting(m->md_dict);
    _PyObject_SetDeferredRefcount((PyObject *)m);
    PyObject_GC_Track(m);
}

static PyObject *
new_module(PyTypeObject *mt, PyObject *args, PyObject *kws)
{
    PyModuleObject *m = new_module_notrack(mt);
    if (m != NULL) {
        track_module(m);
    }
    return (PyObject *)m;
}

PyObject *
PyModule_NewObject(PyObject *name)
{
    PyModuleObject *m = new_module_notrack(&PyModule_Type);
    if (m == NULL)
        return NULL;
    if (module_init_dict(m, m->md_dict, name, NULL) != 0)
        goto fail;
    track_module(m);
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
        _PyObject_SetDeferredRefcount(func);
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

static void
module_copy_members_from_deflike(
    PyModuleObject *md,
    PyModuleDef *def_like /* not necessarily a valid Python object */)
{
    md->md_state_size = def_like->m_size;
    md->md_state_traverse = def_like->m_traverse;
    md->md_state_clear = def_like->m_clear;
    md->md_state_free = def_like->m_free;
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

    m = (PyModuleObject*)PyModule_New(name);
    if (m == NULL)
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
    m->md_token = module;
    m->md_token_is_def = true;
    module_copy_members_from_deflike(m, module);
#ifdef Py_GIL_DISABLED
    m->md_requires_gil = true;
#endif
    return (PyObject*)m;
}

static PyObject *
module_from_def_and_spec(
    PyModuleDef* def_like, /* not necessarily a valid Python object */
    PyObject *spec,
    int module_api_version,
    PyModuleDef* original_def /* NULL if not defined by a def */)
{
    PyModuleDef_Slot* cur_slot;
    PyObject *(*create)(PyObject *, PyModuleDef*) = NULL;
    PyObject *nameobj;
    PyObject *m = NULL;
    int has_multiple_interpreters_slot = 0;
    void *multiple_interpreters = (void *)0;
    int has_gil_slot = 0;
    bool requires_gil = true;
    int has_execution_slots = 0;
    const char *name;
    int ret;
    void *token = NULL;
    _Py_modexecfunc m_exec = NULL;
    PyInterpreterState *interp = _PyInterpreterState_GET();

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

    if (def_like->m_size < 0) {
        PyErr_Format(
            PyExc_SystemError,
            "module %s: m_size may not be negative for multi-phase initialization",
            name);
        goto error;
    }

    for (cur_slot = def_like->m_slots; cur_slot && cur_slot->slot; cur_slot++) {
        // Macro to copy a non-NULL, non-repeatable slot that's unusable with
        // PyModuleDef. The destination must be initially NULL.
#define COPY_COMMON_SLOT(SLOT, TYPE, DEST)                              \
        do {                                                            \
            if (!(TYPE)(cur_slot->value)) {                             \
                PyErr_Format(                                           \
                    PyExc_SystemError,                                  \
                    "module %s: " #SLOT " must not be NULL",            \
                    name);                                              \
                goto error;                                             \
            }                                                           \
            if (original_def) {                                         \
                PyErr_Format(                                           \
                    PyExc_SystemError,                                  \
                    "module %s: " #SLOT " used with PyModuleDef",       \
                    name);                                              \
                goto error;                                             \
            }                                                           \
            if (DEST) {                                                 \
                PyErr_Format(                                           \
                    PyExc_SystemError,                                  \
                    "module %s has more than one " #SLOT " slot",       \
                    name);                                              \
                goto error;                                             \
            }                                                           \
            DEST = (TYPE)(cur_slot->value);                             \
        } while (0);                                                    \
        /////////////////////////////////////////////////////////////////
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
                if (!original_def) {
                    COPY_COMMON_SLOT(Py_mod_exec, _Py_modexecfunc, m_exec);
                }
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
            case Py_mod_gil:
                if (has_gil_slot) {
                    PyErr_Format(
                       PyExc_SystemError,
                       "module %s has more than one 'gil' slot",
                       name);
                    goto error;
                }
                requires_gil = (cur_slot->value != Py_MOD_GIL_NOT_USED);
                has_gil_slot = 1;
                break;
            case Py_mod_abi:
                if (PyABIInfo_Check((PyABIInfo *)cur_slot->value, name) < 0) {
                    goto error;
                }
                break;
            case Py_mod_name:
                COPY_COMMON_SLOT(Py_mod_name, char*, def_like->m_name);
                break;
            case Py_mod_doc:
                COPY_COMMON_SLOT(Py_mod_doc, char*, def_like->m_doc);
                break;
            case Py_mod_state_size:
                COPY_COMMON_SLOT(Py_mod_state_size, Py_ssize_t,
                                 def_like->m_size);
                break;
            case Py_mod_methods:
                COPY_COMMON_SLOT(Py_mod_methods, PyMethodDef*,
                                 def_like->m_methods);
                break;
            case Py_mod_state_traverse:
                COPY_COMMON_SLOT(Py_mod_state_traverse, traverseproc,
                                 def_like->m_traverse);
                break;
            case Py_mod_state_clear:
                COPY_COMMON_SLOT(Py_mod_state_clear, inquiry,
                                 def_like->m_clear);
                break;
            case Py_mod_state_free:
                COPY_COMMON_SLOT(Py_mod_state_free, freefunc,
                                 def_like->m_free);
                break;
            case Py_mod_token:
                COPY_COMMON_SLOT(Py_mod_token, void*, token);
                break;
            default:
                assert(cur_slot->slot < 0 || cur_slot->slot > _Py_mod_LAST_SLOT);
                PyErr_Format(
                    PyExc_SystemError,
                    "module %s uses unknown slot ID %i",
                    name, cur_slot->slot);
                goto error;
        }
#undef COPY_COMMON_SLOT
    }

#ifdef Py_GIL_DISABLED
    // For modules created directly from slots (not from a def), we enable
    // the GIL here (pairing `_PyEval_EnableGILTransient` with
    // an immediate `_PyImport_EnableGILAndWarn`).
    // For modules created from a def, the caller is responsible for this.
    if (!original_def && requires_gil) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyEval_EnableGILTransient(tstate) < 0) {
            goto error;
        }
        if (_PyImport_EnableGILAndWarn(tstate, nameobj) < 0) {
            goto error;
        }
    }
#endif

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
        m = create(spec, original_def);
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
        PyModuleObject *mod = (PyModuleObject*)m;
        mod->md_state = NULL;
        module_copy_members_from_deflike(mod, def_like);
        if (original_def) {
            assert (!token);
            mod->md_token = original_def;
            mod->md_token_is_def = 1;
        }
        else {
            mod->md_token = token;
        }
#ifdef Py_GIL_DISABLED
        mod->md_requires_gil = requires_gil;
#else
        (void)requires_gil;
#endif
        mod->md_exec = m_exec;
    } else {
        if (def_like->m_size > 0 || def_like->m_traverse || def_like->m_clear
            || def_like->m_free)
        {
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
        if (token) {
            PyErr_Format(
                PyExc_SystemError,
                "module %s specifies a token, but did not create "
                    "a ModuleType instance",
                name);
            goto error;
        }
    }

    if (def_like->m_methods != NULL) {
        ret = _add_methods_to_object(m, nameobj, def_like->m_methods);
        if (ret != 0) {
            goto error;
        }
    }

    if (def_like->m_doc != NULL) {
        ret = PyModule_SetDocString(m, def_like->m_doc);
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

PyObject *
PyModule_FromDefAndSpec2(PyModuleDef* def, PyObject *spec, int module_api_version)
{
    PyModuleDef_Init(def);
    return module_from_def_and_spec(def, spec, module_api_version, def);
}

PyObject *
PyModule_FromSlotsAndSpec(const PyModuleDef_Slot *slots, PyObject *spec)
{
    if (!slots) {
        PyErr_SetString(
            PyExc_SystemError,
            "PyModule_FromSlotsAndSpec called with NULL slots");
        return NULL;
    }
    // Fill in enough of a PyModuleDef to pass to common machinery
    PyModuleDef def_like = {.m_slots = (PyModuleDef_Slot *)slots};

    return module_from_def_and_spec(&def_like, spec, PYTHON_API_VERSION,
                                    NULL);
}

#ifdef Py_GIL_DISABLED
int
PyUnstable_Module_SetGIL(PyObject *module, void *gil)
{
    bool requires_gil = (gil != Py_MOD_GIL_NOT_USED);
    if (!PyModule_Check(module)) {
        PyErr_BadInternalCall();
        return -1;
    }
    ((PyModuleObject *)module)->md_requires_gil = requires_gil;
    return 0;
}
#endif

static int
run_exec_func(PyObject *module, int (*exec)(PyObject *))
{
    int ret = exec(module);
    if (ret != 0) {
        if (!PyErr_Occurred()) {
            PyErr_Format(
                PyExc_SystemError,
                "execution of %R failed without setting an exception",
                module);
        }
        return -1;
    }
    if (PyErr_Occurred()) {
        _PyErr_FormatFromCause(
            PyExc_SystemError,
            "execution of module %R raised unreported exception",
            module);
        return -1;
    }
    return 0;
}

static int
alloc_state(PyObject *module)
{
    if (!PyModule_Check(module)) {
        PyErr_Format(PyExc_TypeError, "expected module, got %T", module);
        return -1;
    }
    PyModuleObject *md = (PyModuleObject*)module;

    if (md->md_state_size >= 0) {
        if (md->md_state == NULL) {
            /* Always set a state pointer; this serves as a marker to skip
             * multiple initialization (importlib.reload() is no-op) */
            md->md_state = PyMem_Malloc(md->md_state_size);
            if (!md->md_state) {
                PyErr_NoMemory();
                return -1;
            }
            memset(md->md_state, 0, md->md_state_size);
        }
    }
    return 0;
}

int
PyModule_Exec(PyObject *module)
{
    if (alloc_state(module) < 0) {
        return -1;
    }
    PyModuleObject *md = (PyModuleObject*)module;
    if (md->md_exec) {
        assert(!md->md_token_is_def);
        return run_exec_func(module, md->md_exec);
    }

    PyModuleDef *def = _PyModule_GetDefOrNull(module);
    if (def) {
        return PyModule_ExecDef(module, def);
    }
    return 0;
}

int
PyModule_ExecDef(PyObject *module, PyModuleDef *def)
{
    PyModuleDef_Slot *cur_slot;

    if (alloc_state(module) < 0) {
        return -1;
    }

    assert(PyModule_Check(module));

    if (def->m_slots == NULL) {
        return 0;
    }

    for (cur_slot = def->m_slots; cur_slot && cur_slot->slot; cur_slot++) {
        if (cur_slot->slot == Py_mod_exec) {
            int (*func)(PyObject *) = cur_slot->value;
            if (run_exec_func(module, func) < 0) {
                return -1;
            }
            continue;
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
    return _PyModule_GetDict(m);  // borrowed reference
}

int
PyModule_GetStateSize(PyObject *m, Py_ssize_t *size_p)
{
    *size_p = -1;
    if (!PyModule_Check(m)) {
        PyErr_Format(PyExc_TypeError, "expected module, got %T", m);
        return -1;
    }
    PyModuleObject *mod = (PyModuleObject *)m;
    *size_p = mod->md_state_size;
    return 0;
}

int
PyModule_GetToken(PyObject *m, void **token_p)
{
    *token_p = NULL;
    if (!PyModule_Check(m)) {
        PyErr_Format(PyExc_TypeError, "expected module, got %T", m);
        return -1;
    }
    *token_p = _PyModule_GetToken(m);
    return 0;
}

PyObject*
PyModule_GetNameObject(PyObject *mod)
{
    if (!PyModule_Check(mod)) {
        PyErr_BadArgument();
        return NULL;
    }
    PyObject *dict = ((PyModuleObject *)mod)->md_dict;  // borrowed reference
    if (dict == NULL || !PyDict_Check(dict)) {
        goto error;
    }
    PyObject *name;
    if (PyDict_GetItemRef(dict, &_Py_ID(__name__), &name) <= 0) {
        // error or not found
        goto error;
    }
    if (!PyUnicode_Check(name)) {
        Py_DECREF(name);
        goto error;
    }
    return name;

error:
    if (!PyErr_Occurred()) {
        PyErr_SetString(PyExc_SystemError, "nameless module");
    }
    return NULL;
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
_PyModule_GetFilenameObject(PyObject *mod)
{
    // We return None to indicate "not found" or "bogus".
    if (!PyModule_Check(mod)) {
        PyErr_BadArgument();
        return NULL;
    }
    PyObject *dict = ((PyModuleObject *)mod)->md_dict;  // borrowed reference
    if (dict == NULL) {
        // The module has been tampered with.
        Py_RETURN_NONE;
    }
    PyObject *fileobj;
    int res = PyDict_GetItemRef(dict, &_Py_ID(__file__), &fileobj);
    if (res < 0) {
        return NULL;
    }
    if (res == 0) {
        // __file__ isn't set.  There are several reasons why this might
        // be so, most of them valid reasons.  If it's the __main__
        // module then we're running the REPL or with -c.  Otherwise
        // it's a namespace package or other module with a loader that
        // isn't disk-based.  It could also be that a user created
        // a module manually but without manually setting __file__.
        Py_RETURN_NONE;
    }
    if (!PyUnicode_Check(fileobj)) {
        Py_DECREF(fileobj);
        Py_RETURN_NONE;
    }
    return fileobj;
}

PyObject*
PyModule_GetFilenameObject(PyObject *mod)
{
    PyObject *fileobj = _PyModule_GetFilenameObject(mod);
    if (fileobj == NULL) {
        return NULL;
    }
    if (fileobj == Py_None) {
        PyErr_SetString(PyExc_SystemError, "module filename missing");
        return NULL;
    }
    return fileobj;
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

Py_ssize_t
_PyModule_GetFilenameUTF8(PyObject *mod, char *buffer, Py_ssize_t maxlen)
{
    // We "return" an empty string for an invalid module
    // and for a missing, empty, or invalid filename.
    assert(maxlen >= 0);
    Py_ssize_t size = -1;
    PyObject *filenameobj = _PyModule_GetFilenameObject(mod);
    if (filenameobj == NULL) {
        return -1;
    }
    if (filenameobj == Py_None) {
        // It is missing or invalid.
        buffer[0] = '\0';
        size = 0;
    }
    else {
        const char *filename = PyUnicode_AsUTF8AndSize(filenameobj, &size);
        assert(size >= 0);
        if (size > maxlen) {
            size = -1;
            PyErr_SetString(PyExc_ValueError, "__file__ too long");
        }
        else {
            (void)strcpy(buffer, filename);
        }
    }
    Py_DECREF(filenameobj);
    return size;
}

PyModuleDef*
PyModule_GetDef(PyObject* m)
{
    if (!PyModule_Check(m)) {
        PyErr_BadArgument();
        return NULL;
    }
    return _PyModule_GetDefOrNull(m);
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
                    PyErr_FormatUnraisable("Exception ignored while "
                                           "clearing module dict");
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
                    PyErr_FormatUnraisable("Exception ignored while "
                                           "clearing module dict");
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
    return module_init_dict(self, self->md_dict, name, doc);
}

static void
module_dealloc(PyObject *self)
{
    PyModuleObject *m = _PyModule_CAST(self);

    PyObject_GC_UnTrack(m);

    int verbose = _Py_GetConfig()->verbose;
    if (verbose && m->md_name) {
        PySys_FormatStderr("# destroy %U\n", m->md_name);
    }
    FT_CLEAR_WEAKREFS(self, m->md_weaklist);

    assert_def_missing_or_redundant(m);
    /* bpo-39824: Don't call m_free() if m_size > 0 and md_state=NULL */
    if (m->md_state_free && (m->md_state_size <= 0 || m->md_state != NULL))
    {
        m->md_state_free(m);
    }

    Py_XDECREF(m->md_dict);
    Py_XDECREF(m->md_name);
    if (m->md_state != NULL) {
        PyMem_Free(m->md_state);
    }
    Py_TYPE(m)->tp_free((PyObject *)m);
}

static PyObject *
module_repr(PyObject *self)
{
    PyModuleObject *m = _PyModule_CAST(self);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return _PyImport_ImportlibModuleRepr(interp, (PyObject *)m);
}

/* Check if the "_initializing" attribute of the module spec is set to true.
 */
int
_PyModuleSpec_IsInitializing(PyObject *spec)
{
    if (spec == NULL) {
        return 0;
    }
    PyObject *value;
    int rc = PyObject_GetOptionalAttr(spec, &_Py_ID(_initializing), &value);
    if (rc > 0) {
        rc = PyObject_IsTrue(value);
        Py_DECREF(value);
    }
    return rc;
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

    PyObject *value;
    int rc = PyObject_GetOptionalAttr(spec, &_Py_ID(_uninitialized_submodules), &value);
    if (rc > 0) {
        rc = PySequence_Contains(value, name);
        Py_DECREF(value);
    }
    return rc;
}

int
_PyModuleSpec_GetFileOrigin(PyObject *spec, PyObject **p_origin)
{
    PyObject *has_location = NULL;
    int rc = PyObject_GetOptionalAttr(spec, &_Py_ID(has_location), &has_location);
    if (rc <= 0) {
        return rc;
    }
    // If origin is not a location, or doesn't exist, or is not a str, we could consider falling
    // back to module.__file__. But the cases in which module.__file__ is not __spec__.origin
    // are cases in which we probably shouldn't be guessing.
    rc = PyObject_IsTrue(has_location);
    Py_DECREF(has_location);
    if (rc <= 0) {
        return rc;
    }
    // has_location is true, so origin is a location
    PyObject *origin = NULL;
    rc = PyObject_GetOptionalAttr(spec, &_Py_ID(origin), &origin);
    if (rc <= 0) {
        return rc;
    }
    assert(origin != NULL);
    if (!PyUnicode_Check(origin)) {
        Py_DECREF(origin);
        return 0;
    }
    *p_origin = origin;
    return 1;
}

int
_PyModule_IsPossiblyShadowing(PyObject *origin)
{
    // origin must be a unicode subtype
    // Returns 1 if the module at origin could be shadowing a module of the
    // same name later in the module search path. The condition we check is basically:
    // root = os.path.dirname(origin.removesuffix(os.sep + "__init__.py"))
    // return not sys.flags.safe_path and root == (sys.path[0] or os.getcwd())
    // Returns 0 otherwise (or if we aren't sure)
    // Returns -1 if an error occurred that should be propagated
    if (origin == NULL) {
        return 0;
    }

    // not sys.flags.safe_path
    const PyConfig *config = _Py_GetConfig();
    if (config->safe_path) {
        return 0;
    }

    // root = os.path.dirname(origin.removesuffix(os.sep + "__init__.py"))
    wchar_t root[MAXPATHLEN + 1];
    Py_ssize_t size = PyUnicode_AsWideChar(origin, root, MAXPATHLEN);
    if (size < 0) {
        return -1;
    }
    assert(size <= MAXPATHLEN);
    root[size] = L'\0';

    wchar_t *sep = wcsrchr(root, SEP);
    if (sep == NULL) {
        return 0;
    }
    // If it's a package then we need to look one directory further up
    if (wcscmp(sep + 1, L"__init__.py") == 0) {
        *sep = L'\0';
        sep = wcsrchr(root, SEP);
        if (sep == NULL) {
            return 0;
        }
    }
    *sep = L'\0';

    // sys.path[0] or os.getcwd()
    wchar_t *sys_path_0 = config->sys_path_0;
    if (!sys_path_0) {
        return 0;
    }

    wchar_t sys_path_0_buf[MAXPATHLEN];
    if (sys_path_0[0] == L'\0') {
        // if sys.path[0] == "", treat it as if it were the current directory
        if (!_Py_wgetcwd(sys_path_0_buf, MAXPATHLEN)) {
            // If we failed to getcwd, don't raise an exception and instead
            // let the caller proceed assuming no shadowing
            return 0;
        }
        sys_path_0 = sys_path_0_buf;
    }

    int result = wcscmp(sys_path_0, root) == 0;
    return result;
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
    if (PyDict_GetItemRef(m->md_dict, &_Py_ID(__getattr__), &getattr) < 0) {
        return NULL;
    }
    if (getattr) {
        PyObject *result = PyObject_CallOneArg(getattr, name);
        if (result == NULL && suppress == 1 && PyErr_ExceptionMatches(PyExc_AttributeError)) {
            // suppress AttributeError
            PyErr_Clear();
        }
        Py_DECREF(getattr);
        return result;
    }

    // The attribute was not found.  We make a best effort attempt at a useful error message,
    // but only if we're not suppressing AttributeError.
    if (suppress == 1) {
        return NULL;
    }
    if (PyDict_GetItemRef(m->md_dict, &_Py_ID(__name__), &mod_name) < 0) {
        return NULL;
    }
    if (!mod_name || !PyUnicode_Check(mod_name)) {
        Py_XDECREF(mod_name);
        PyErr_Format(PyExc_AttributeError,
                    "module has no attribute '%U'", name);
        return NULL;
    }
    PyObject *spec;
    if (PyDict_GetItemRef(m->md_dict, &_Py_ID(__spec__), &spec) < 0) {
        Py_DECREF(mod_name);
        return NULL;
    }
    if (spec == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "module '%U' has no attribute '%U'",
                     mod_name, name);
        Py_DECREF(mod_name);
        return NULL;
    }

    PyObject *origin = NULL;
    if (_PyModuleSpec_GetFileOrigin(spec, &origin) < 0) {
        goto done;
    }

    int is_possibly_shadowing = _PyModule_IsPossiblyShadowing(origin);
    if (is_possibly_shadowing < 0) {
        goto done;
    }
    int is_possibly_shadowing_stdlib = 0;
    if (is_possibly_shadowing) {
        PyObject *stdlib_modules;
        if (PySys_GetOptionalAttrString("stdlib_module_names", &stdlib_modules) < 0) {
            goto done;
        }
        if (stdlib_modules && PyAnySet_Check(stdlib_modules)) {
            is_possibly_shadowing_stdlib = PySet_Contains(stdlib_modules, mod_name);
            if (is_possibly_shadowing_stdlib < 0) {
                Py_DECREF(stdlib_modules);
                goto done;
            }
        }
        Py_XDECREF(stdlib_modules);
    }

    if (is_possibly_shadowing_stdlib) {
        assert(origin);
        PyErr_Format(PyExc_AttributeError,
                    "module '%U' has no attribute '%U' "
                    "(consider renaming '%U' since it has the same "
                    "name as the standard library module named '%U' "
                    "and prevents importing that standard library module)",
                    mod_name, name, origin, mod_name);
    }
    else {
        int rc = _PyModuleSpec_IsInitializing(spec);
        if (rc < 0) {
            goto done;
        }
        else if (rc > 0) {
            if (is_possibly_shadowing) {
                assert(origin);
                // For non-stdlib modules, only mention the possibility of
                // shadowing if the module is being initialized.
                PyErr_Format(PyExc_AttributeError,
                            "module '%U' has no attribute '%U' "
                            "(consider renaming '%U' if it has the same name "
                            "as a library you intended to import)",
                            mod_name, name, origin);
            }
            else if (origin) {
                PyErr_Format(PyExc_AttributeError,
                            "partially initialized "
                            "module '%U' from '%U' has no attribute '%U' "
                            "(most likely due to a circular import)",
                            mod_name, origin, name);
            }
            else {
                PyErr_Format(PyExc_AttributeError,
                            "partially initialized "
                            "module '%U' has no attribute '%U' "
                            "(most likely due to a circular import)",
                            mod_name, name);
            }
        }
        else {
            assert(rc == 0);
            rc = _PyModuleSpec_IsUninitializedSubmodule(spec, name);
            if (rc > 0) {
                PyErr_Format(PyExc_AttributeError,
                            "cannot access submodule '%U' of module '%U' "
                            "(most likely due to a circular import)",
                            name, mod_name);
            }
            else if (rc == 0) {
                PyErr_Format(PyExc_AttributeError,
                            "module '%U' has no attribute '%U'",
                            mod_name, name);
            }
        }
    }

done:
    Py_XDECREF(origin);
    Py_DECREF(spec);
    Py_DECREF(mod_name);
    return NULL;
}


PyObject*
_Py_module_getattro(PyObject *self, PyObject *name)
{
    PyModuleObject *m = _PyModule_CAST(self);
    return _Py_module_getattro_impl(m, name, 0);
}

static int
module_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyModuleObject *m = _PyModule_CAST(self);

    assert_def_missing_or_redundant(m);
    /* bpo-39824: Don't call m_traverse() if m_size > 0 and md_state=NULL */
    if (m->md_state_traverse && (m->md_state_size <= 0 || m->md_state != NULL))
    {
        int res = m->md_state_traverse((PyObject*)m, visit, arg);
        if (res)
            return res;
    }

    Py_VISIT(m->md_dict);
    return 0;
}

static int
module_clear(PyObject *self)
{
    PyModuleObject *m = _PyModule_CAST(self);

    assert_def_missing_or_redundant(m);
    /* bpo-39824: Don't call m_clear() if m_size > 0 and md_state=NULL */
    if (m->md_state_clear && (m->md_state_size <= 0 || m->md_state != NULL))
    {
        int res = m->md_state_clear((PyObject*)m);
        if (PyErr_Occurred()) {
            PyErr_FormatUnraisable("Exception ignored in m_clear of module%s%V",
                                   m->md_name ? " " : "",
                                   m->md_name, "");
        }
        if (res) {
            return res;
        }
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
module_get_dict(PyModuleObject *m)
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
    return dict;
}

static PyObject *
module_get_annotate(PyObject *self, void *Py_UNUSED(ignored))
{
    PyModuleObject *m = _PyModule_CAST(self);

    PyObject *dict = module_get_dict(m);
    if (dict == NULL) {
        return NULL;
    }

    PyObject *annotate;
    if (PyDict_GetItemRef(dict, &_Py_ID(__annotate__), &annotate) == 0) {
        annotate = Py_None;
        if (PyDict_SetItem(dict, &_Py_ID(__annotate__), annotate) == -1) {
            Py_CLEAR(annotate);
        }
    }
    Py_DECREF(dict);
    return annotate;
}

static int
module_set_annotate(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyModuleObject *m = _PyModule_CAST(self);
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete __annotate__ attribute");
        return -1;
    }

    PyObject *dict = module_get_dict(m);
    if (dict == NULL) {
        return -1;
    }

    if (!Py_IsNone(value) && !PyCallable_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "__annotate__ must be callable or None");
        Py_DECREF(dict);
        return -1;
    }

    if (PyDict_SetItem(dict, &_Py_ID(__annotate__), value) == -1) {
        Py_DECREF(dict);
        return -1;
    }
    if (!Py_IsNone(value)) {
        if (PyDict_Pop(dict, &_Py_ID(__annotations__), NULL) == -1) {
            Py_DECREF(dict);
            return -1;
        }
    }
    Py_DECREF(dict);
    return 0;
}

static PyObject *
module_get_annotations(PyObject *self, void *Py_UNUSED(ignored))
{
    PyModuleObject *m = _PyModule_CAST(self);

    PyObject *dict = module_get_dict(m);
    if (dict == NULL) {
        return NULL;
    }

    PyObject *annotations;
    if (PyDict_GetItemRef(dict, &_Py_ID(__annotations__), &annotations) == 0) {
        PyObject *spec;
        if (PyDict_GetItemRef(m->md_dict, &_Py_ID(__spec__), &spec) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        bool is_initializing = false;
        if (spec != NULL) {
            int rc = _PyModuleSpec_IsInitializing(spec);
            if (rc < 0) {
                Py_DECREF(spec);
                Py_DECREF(dict);
                return NULL;
            }
            Py_DECREF(spec);
            if (rc) {
                is_initializing = true;
            }
        }

        PyObject *annotate;
        int annotate_result = PyDict_GetItemRef(dict, &_Py_ID(__annotate__), &annotate);
        if (annotate_result < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        if (annotate_result == 1 && PyCallable_Check(annotate)) {
            PyObject *one = _PyLong_GetOne();
            annotations = _PyObject_CallOneArg(annotate, one);
            if (annotations == NULL) {
                Py_DECREF(annotate);
                Py_DECREF(dict);
                return NULL;
            }
            if (!PyDict_Check(annotations)) {
                PyErr_Format(PyExc_TypeError,
                             "__annotate__() must return a dict, not %T",
                             annotations);
                Py_DECREF(annotate);
                Py_DECREF(annotations);
                Py_DECREF(dict);
                return NULL;
            }
        }
        else {
            annotations = PyDict_New();
        }
        Py_XDECREF(annotate);
        // Do not cache annotations if the module is still initializing
        if (annotations && !is_initializing) {
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
module_set_annotations(PyObject *self, PyObject *value, void *Py_UNUSED(ignored))
{
    PyModuleObject *m = _PyModule_CAST(self);

    PyObject *dict = module_get_dict(m);
    if (dict == NULL) {
        return -1;
    }

    int ret = -1;
    if (value != NULL) {
        /* set */
        ret = PyDict_SetItem(dict, &_Py_ID(__annotations__), value);
    }
    else {
        /* delete */
        ret = PyDict_Pop(dict, &_Py_ID(__annotations__), NULL);
        if (ret == 0) {
            PyErr_SetObject(PyExc_AttributeError, &_Py_ID(__annotations__));
            ret = -1;
        }
        else if (ret > 0) {
            ret = 0;
        }
    }
    if (ret == 0 && PyDict_Pop(dict, &_Py_ID(__annotate__), NULL) < 0) {
        ret = -1;
    }

    Py_DECREF(dict);
    return ret;
}


static PyGetSetDef module_getsets[] = {
    {"__annotations__", module_get_annotations, module_set_annotations},
    {"__annotate__", module_get_annotate, module_set_annotate},
    {NULL}
};

PyTypeObject PyModule_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "module",                                   /* tp_name */
    sizeof(PyModuleObject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    module_dealloc,                             /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    module_repr,                                /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    _Py_module_getattro,                        /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    module___init____doc__,                     /* tp_doc */
    module_traverse,                            /* tp_traverse */
    module_clear,                               /* tp_clear */
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
