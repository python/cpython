#include "Python.h"
#include "pycore_pystate.h"   // _PyFreeListState_GET()
#include "pycore_tstate.h"    // _PyThreadStateImpl

#ifdef Py_GIL_DISABLED

/* Clear all free lists
 * All free lists are cleared during the collection of the highest generation.
 * Allocated items in the free list may keep a pymalloc arena occupied.
 * Clearing the free lists may give back memory to the OS earlier.
 * Free-threading version: Since freelists are managed per thread,
 * GC should clear all freelists by traversing all threads.
 */
void
_PyGC_ClearAllFreeLists(PyInterpreterState *interp)
{
    _PyTuple_ClearFreeList(interp);
    _PyFloat_ClearFreeList(interp);
    _PyDict_ClearFreeList(interp);
    _PyAsyncGen_ClearFreeLists(interp);
    _PyContext_ClearFreeList(interp);

    HEAD_LOCK(&_PyRuntime);
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)interp->threads.head;
    while (tstate != NULL) {
        _Py_ClearFreeLists(&tstate->freelist_state, 0);
        tstate = (_PyThreadStateImpl *)tstate->base.next;
    }
    HEAD_UNLOCK(&_PyRuntime);
}

#endif
