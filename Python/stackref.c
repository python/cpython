#include "Python.h"

#include "pycore_stackref.h"
#include "pycore_interp.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_critical_section.h"

int
_PyStackRef_IsLive(_PyStackRef stackref)
{
    _Py_stackref_to_object_transition(stackref, BORROW);
    return 1;
}


#if defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
PyObject *
_Py_stackref_to_object_transition(_PyStackRef stackref, _PyStackRef_OpKind op)
{
    PyObject *res = NULL;
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    int is_null = 0;
    void *bits = (void *)(stackref.bits >> RESERVED_BITS);
    switch (op) {
        case STEAL: {
            // We need to distinguish NULL here because _Py_hashtable_get
            // returns NULL if it cannot find a value.
            if (bits == 0) {
                is_null = 1;
                break;
            }
            PyMutex_Lock(&interp->stackref_state.lock);
            res = (PyObject *)_Py_hashtable_get(interp->stackref_state.entries, bits);
            if (res != NULL && !_Py_IsImmortal(res)) {
                res = (PyObject *)_Py_hashtable_steal(interp->stackref_state.entries, bits);
            }
            PyMutex_Unlock(&interp->stackref_state.lock);
            break;
        }
        case NEW:
        case BORROW: {
            if (bits == 0) {
                is_null = 1;
                break;
            }
            PyMutex_Lock(&interp->stackref_state.lock);
            res = (PyObject *)_Py_hashtable_get(interp->stackref_state.entries, bits);
            PyMutex_Unlock(&interp->stackref_state.lock);
            break;
        }
    }
    assert((res != NULL) ^ is_null);
    return res;
}

_PyStackRef
_Py_object_to_stackref_transition(PyObject *obj, char tag, _PyStackRef_OpKind op)
{
    _PyStackRef res;
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    switch (op) {
        case STEAL:
        case NEW:
        {
            if (obj == NULL) {
                return PyStackRef_NULL;
            }
            PyMutex_Lock(&interp->stackref_state.lock);
            uintptr_t key = interp->stackref_state.next_ref;
            res.bits = (key << RESERVED_BITS) | tag;
            int err = _Py_hashtable_set(interp->stackref_state.entries, (void *)key, obj);
            if (err < 0) {
                Py_FatalError("Stackref handle allocation failed.\n");
            }
            interp->stackref_state.next_ref++;
            PyMutex_Unlock(&interp->stackref_state.lock);
            break;
        }
        case BORROW:
            Py_UNREACHABLE();
    }
    return res;
}

void
_PyStackRef_Close(_PyStackRef stackref)
{
    _Py_stackref_to_object_transition(stackref, STEAL);
}

_PyStackRef
_PyStackRef_Dup(_PyStackRef stackref)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    char tag = stackref.bits & RESERVED_BITS;
    PyObject *val = _Py_stackref_to_object_transition(stackref, BORROW);
    return _Py_object_to_stackref_transition(val, tag, NEW);
}
#endif
