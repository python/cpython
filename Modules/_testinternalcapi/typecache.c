// Test wrappers for the per-type lookup cache (pycore_typecache.h).
//
// Insertion is exercised indirectly through normal attribute access (which
// calls _PyType_Lookup); only Lookup and Invalidate need direct wrappers.

#include "parts.h"

#include "pycore_critical_section.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_stackref.h"      // PyStackRef_AsPyObjectSteal()
#include "pycore_typecache.h"     // _PyTypeCache_Lookup()


static int
require_type(PyObject *obj)
{
    if (!PyType_Check(obj)) {
        PyErr_SetString(PyExc_TypeError, "expected a type");
        return -1;
    }
    return 0;
}

static PyObject *
intern_name(PyObject *name)
{
    if (!PyUnicode_CheckExact(name)) {
        PyErr_SetString(PyExc_TypeError, "name must be a str");
        return NULL;
    }
    Py_INCREF(name);
    PyUnicode_InternInPlace(&name);
    return name;
}

// type_cache_lookup(type, name) -> (cache_hit, value_or_None, version_tag)
static PyObject *
type_cache_lookup(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *type_obj, *name;
    if (!PyArg_ParseTuple(args, "OU", &type_obj, &name)) {
        return NULL;
    }
    if (require_type(type_obj) < 0) {
        return NULL;
    }
    name = intern_name(name);
    if (name == NULL) {
        return NULL;
    }
    struct _PyTypeCacheLookupResult r =
        _PyTypeCache_Lookup((PyTypeObject *)type_obj, name);
    Py_DECREF(name);
    PyObject *value;
    if (PyStackRef_IsNull(r.value)) {
        value = Py_NewRef(Py_None);
    }
    else {
        value = PyStackRef_AsPyObjectSteal(r.value);
    }
    return Py_BuildValue("(iNk)",
                         r.cache_hit, value,
                         (unsigned long)r.version_tag);
}

static PyObject *
type_cache_invalidate(PyObject *Py_UNUSED(self), PyObject *type_obj)
{
    if (require_type(type_obj) < 0) {
        return NULL;
    }
    Py_BEGIN_CRITICAL_SECTION_MUTEX(&_PyInterpreterState_GET()->types.mutex);
    _PyTypeCache_Invalidate((PyTypeObject *)type_obj);
    Py_END_CRITICAL_SECTION();
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"type_cache_lookup", type_cache_lookup, METH_VARARGS},
    {"type_cache_invalidate", type_cache_invalidate, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_TypeCache(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(m, _Py_TYPECACHE_MINSIZE) < 0) {
        return -1;
    }
    return 0;
}
