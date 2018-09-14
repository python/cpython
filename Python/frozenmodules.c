#include "Python.h"
#include <stdbool.h>

static _Bool frozen_modules_disable = false;
static PyObject* frozen_code_objects = NULL;

/* used by frozenmodules_code.c to populate frozen_code_objects */
int _PyFrozenModule_AddModule(const char *name, PyObject *co,
                                _Bool needs_path, const char *parent_name,
                                const char *origin, const char *cached)
{
    PyObject *t, *_needs_path, *_parent_name,
             *_origin, *_cached;
    int r;

    if (frozen_code_objects == NULL) {
        frozen_code_objects = PyDict_New();
        if (frozen_code_objects == NULL) {
            return NULL;
        }
    }

    _needs_path = needs_path ? Py_True : Py_False;
    Py_INCREF(_needs_path);

    if(parent_name == NULL) {
        _parent_name = Py_None;
        Py_INCREF(Py_None);
    } else {
        _parent_name = PyUnicode_FromString(parent_name);
    }
    if(origin == NULL) {
        _origin = Py_None;
        Py_INCREF(_origin);
    } else {
        _origin = PyUnicode_FromString(origin);
    }
    if(cached == NULL) {
        _cached = Py_None;
        Py_INCREF(_cached);
    } else {
        _cached = PyUnicode_FromString(cached);
    }
    t = PyTuple_Pack(5, co, _needs_path, _parent_name, _origin, _cached);
    if (t == NULL) {
        Py_DECREF(_needs_path);
        Py_DECREF(_parent_name);
        return -1;
    }
    r = PyDict_SetItemString(frozen_code_objects, name, t);
    Py_DECREF(_needs_path);
    Py_DECREF(_parent_name);
    Py_DECREF(t);
    return r;
}

void _PyFrozenModules_Disable(void) {
    frozen_modules_disable = true;
}

void _PyFrozenModules_Enable(void) {
    frozen_modules_disable = false;
}

PyObject* _PyFrozenModule_Lookup(PyObject* name) {
    if(frozen_code_objects != NULL && !frozen_modules_disable) {
        PyObject *co, *needs_path,
                 *origin, *cached, *path_str,
                 *module, *module_dict, *t =
            PyDict_GetItem(frozen_code_objects, name);
        if (t == NULL) {
            return NULL;
        }
        origin = PyTuple_GetItem(t, 3);
        if (_PyUnicode_EqualToASCIIString(origin, "frozen")) {
            return NULL; // will be returned from _PyFrozenModule_GetCode
        }
        co = PyTuple_GetItem(t, 0);
        needs_path = PyTuple_GetItem(t, 1);
        cached = PyTuple_GetItem(t, 4);
        if (co != NULL) {
            if (needs_path == Py_True) {
                PyObject *path_sep = PyUnicode_FromString("/");
                Py_ssize_t sz = PyUnicode_Find(origin, path_sep,
                                               0, PyUnicode_GetLength(origin),
                                               -1);
                Py_DECREF(path_sep);
                if (sz == -2) {
                    Py_DECREF(origin);
                    Py_DECREF(cached);
                    return NULL;
                }
                else if (sz == -1) {
                    path_str = origin;
                    Py_INCREF(path_str);
                }
                else {
                    path_str = PyUnicode_Substring(origin, 0, sz);
                }
                module = PyImport_AddModuleObject(name);
                module_dict = PyModule_GetDict(module);
                PyObject *l = PyList_New(1);
                if (l == NULL) {
                    Py_DECREF(origin);
                    Py_DECREF(cached);
                    Py_DECREF(path_str);
                    return NULL;
                }
                PyList_SetItem(l, 0, path_str);
                int err = PyDict_SetItemString(module_dict, "__path__", l);
                //Py_DECREF(l);
                if (err != 0) {
                    Py_DECREF(origin);
                    Py_DECREF(cached);
                    Py_DECREF(path_str);
                    return NULL;
                }
                //Py_DECREF(path_str);
            }
            module = PyImport_ExecCodeModuleObject(
                name, co, origin, cached);
            Py_DECREF(origin);
            Py_DECREF(cached);
            return module;
        }
    }
    return NULL;
}

PyObject *_PyFrozenModule_GetCode(PyObject *name, int *needs_path)
{
    if (frozen_code_objects == NULL) {
        return NULL;
    }
    PyObject *t = PyDict_GetItem(frozen_code_objects, name);
    if (t == NULL) {
        return NULL;
    }
    PyObject *origin = PyTuple_GetItem(t, 3);
    if (!_PyUnicode_EqualToASCIIString(origin, "frozen")) {{
        return NULL; // will be returned from _PyFrozenModule_Lookup
    }}
    PyObject *co = PyTuple_GetItem(t, 0);
    *needs_path = PyObject_IsTrue(PyTuple_GetItem(t, 1));
    return co;
}

void _PyFrozenModules_Finalize(void) {
    if (frozen_code_objects) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(frozen_code_objects, &pos, &key, &value)) {
            PyCodeObject *co = (PyCodeObject*)PyTuple_GetItem(value, 0);
            if (co->co_zombieframe != NULL) {
                PyObject_GC_Del(co->co_zombieframe);
                co->co_zombieframe = NULL;
            }
            if (co->co_weakreflist != NULL) {
                PyObject_ClearWeakRefs((PyObject*)co);
                co->co_weakreflist = NULL;
            }
        }
        PyDict_Clear(frozen_code_objects);
        Py_DECREF(frozen_code_objects);
        frozen_code_objects = NULL;
    }
}
