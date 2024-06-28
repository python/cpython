#include "Python.h"

#include "pycore_stackref.h"
#include "pycore_interp.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_critical_section.h"

int
_PyStackRef_IsLive(_PyStackRef stackref)
{
#if defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
    PyInterpreterState *interp = PyInterpreterState_Get();
    struct _Py_stackref_entry *entry = &interp->stackref_state.entries[stackref.bits >> RESERVED_BITS];
    return entry->is_live;
#else
    return 1;
#endif
}

#if defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
PyObject *
_Py_stackref_to_object_transition(_PyStackRef stackref, _PyStackRef_OpKind op)
{
    PyObject *res;
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    switch (op) {
        case STEAL: {
            PyMutex_Lock(&interp->stackref_state.lock);
            struct _Py_stackref_entry *entry = &interp->stackref_state.entries[stackref.bits >> RESERVED_BITS];
            assert(entry->is_live);
            res = entry->obj;
            entry->is_live = _Py_IsImmortal(entry->obj);
            PyMutex_Unlock(&interp->stackref_state.lock);
            break;
        }
        case NEW:
        case BORROW: {
            struct _Py_stackref_entry *entry = &interp->stackref_state.entries[stackref.bits >> RESERVED_BITS];
            assert(entry->is_live);
            res = entry->obj;
            break;
        }
    }
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
            PyMutex_Lock(&interp->stackref_state.lock);
            res.bits = (interp->stackref_state.next_ref << RESERVED_BITS) | tag;
            struct _Py_stackref_entry *entry = &interp->stackref_state.entries[interp->stackref_state.next_ref];
            entry->is_live = 1;
            entry->obj = obj;
            interp->stackref_state.next_ref++;
            // Out of space, allocate new one.
            if (interp->stackref_state.next_ref >= interp->stackref_state.n_entries) {
                size_t old_size = interp->stackref_state.n_entries;
                interp->stackref_state.n_entries *= 2;
                struct _Py_stackref_entry *new_mem = PyMem_Malloc(
                    sizeof(struct _Py_stackref_entry) *
                    interp->stackref_state.n_entries);
                if (new_mem == NULL) {
                    fprintf(stderr, "OOM for PyStackRef\n");
                    Py_FatalError("Cannot allocate memory for stack references.\n");
                    return PyStackRef_NULL;
                }
                memcpy(new_mem,
                       interp->stackref_state.entries,
                       old_size * sizeof(struct _Py_stackref_entry));
                PyMem_Free(interp->stackref_state.entries);
                interp->stackref_state.entries = new_mem;
            }
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
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(interp != NULL);
    PyMutex_Lock(&interp->stackref_state.lock);
    struct _Py_stackref_entry *entry = &interp->stackref_state.entries[stackref.bits >> RESERVED_BITS];
    assert(entry->is_live);
    entry->is_live = _Py_IsImmortal(entry->obj);
    PyMutex_Unlock(&interp->stackref_state.lock);
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
