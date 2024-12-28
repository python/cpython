#include "Python.h"
#include "pycore_dict.h"              // _PyDict_DelItemIf()
#include "pycore_object.h"            // _PyObject_GET_WEAKREFS_LISTPTR()
#include "pycore_weakref.h"           // _PyWeakref_IS_DEAD()

#define GET_WEAKREFS_LISTPTR(o) \
        ((PyWeakReference **) _PyObject_GET_WEAKREFS_LISTPTR(o))

/*[clinic input]
module _weakref
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ffec73b85846596d]*/

#include "clinic/_weakref.c.h"

/*[clinic input]
_weakref.getweakrefcount -> Py_ssize_t

  object: object
  /

Return the number of weak references to 'object'.
[clinic start generated code]*/

static Py_ssize_t
_weakref_getweakrefcount_impl(PyObject *module, PyObject *object)
/*[clinic end generated code: output=301806d59558ff3e input=7d4d04fcaccf64d5]*/
{
    return _PyWeakref_GetWeakrefCount(object);
}


static int
is_dead_weakref(PyObject *value, void *unused)
{
    if (!PyWeakref_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "not a weakref");
        return -1;
    }
    return _PyWeakref_IS_DEAD(value);
}

/*[clinic input]

_weakref._remove_dead_weakref -> object

  dct: object(subclass_of='&PyDict_Type')
  key: object
  /

Atomically remove key from dict if it points to a dead weakref.
[clinic start generated code]*/

static PyObject *
_weakref__remove_dead_weakref_impl(PyObject *module, PyObject *dct,
                                   PyObject *key)
/*[clinic end generated code: output=d9ff53061fcb875c input=19fc91f257f96a1d]*/
{
    if (_PyDict_DelItemIf(dct, key, is_dead_weakref, NULL) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_weakref.getweakrefs
    object: object
    /

Return a list of all weak reference objects pointing to 'object'.
[clinic start generated code]*/

static PyObject *
_weakref_getweakrefs(PyObject *module, PyObject *object)
/*[clinic end generated code: output=25c7731d8e011824 input=00c6d0e5d3206693]*/
{
    if (!_PyType_SUPPORTS_WEAKREFS(Py_TYPE(object))) {
        return PyList_New(0);
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    LOCK_WEAKREFS(object);
    PyWeakReference *current = *GET_WEAKREFS_LISTPTR(object);
    while (current != NULL) {
        PyObject *curobj = (PyObject *) current;
        if (_Py_TryIncref(curobj)) {
            if (PyList_Append(result, curobj)) {
                UNLOCK_WEAKREFS(object);
                Py_DECREF(curobj);
                Py_DECREF(result);
                return NULL;
            }
            else {
                // Undo our _Py_TryIncref. This is safe to do with the lock
                // held in free-threaded builds; the list holds a reference to
                // curobj so we're guaranteed not to invoke the destructor.
                Py_DECREF(curobj);
            }
        }
        current = current->wr_next;
    }
    UNLOCK_WEAKREFS(object);
    return result;
}


/*[clinic input]

_weakref.proxy
    object: object
    callback: object(c_default="NULL") = None
    /

Create a proxy object that weakly references 'object'.

'callback', if given, is called with a reference to the
proxy when 'object' is about to be finalized.
[clinic start generated code]*/

static PyObject *
_weakref_proxy_impl(PyObject *module, PyObject *object, PyObject *callback)
/*[clinic end generated code: output=d68fa4ad9ea40519 input=4808adf22fd137e7]*/
{
    return PyWeakref_NewProxy(object, callback);
}


static PyMethodDef
weakref_functions[] =  {
    _WEAKREF_GETWEAKREFCOUNT_METHODDEF
    _WEAKREF__REMOVE_DEAD_WEAKREF_METHODDEF
    _WEAKREF_GETWEAKREFS_METHODDEF
    _WEAKREF_PROXY_METHODDEF
    {NULL, NULL, 0, NULL}
};

static int
weakref_exec(PyObject *module)
{
    if (PyModule_AddObjectRef(module, "ref", (PyObject *) &_PyWeakref_RefType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "ReferenceType",
                           (PyObject *) &_PyWeakref_RefType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "ProxyType",
                           (PyObject *) &_PyWeakref_ProxyType) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "CallableProxyType",
                           (PyObject *) &_PyWeakref_CallableProxyType) < 0) {
        return -1;
    }

    return 0;
}

static struct PyModuleDef_Slot weakref_slots[] = {
    {Py_mod_exec, weakref_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef weakrefmodule = {
    PyModuleDef_HEAD_INIT,
    "_weakref",
    "Weak-reference support module.",
    0,
    weakref_functions,
    weakref_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__weakref(void)
{
    return PyModuleDef_Init(&weakrefmodule);
}
