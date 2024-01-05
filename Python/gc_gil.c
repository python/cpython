#ifndef Py_GIL_DISABLED

#include "Python.h"
#include "pycore_pystate.h"   // _PyFreeListState_GET()

/* Clear all free lists
 * All free lists are cleared during the collection of the highest generation.
 * Allocated items in the free list may keep a pymalloc arena occupied.
 * Clearing the free lists may give back memory to the OS earlier.
 */
void
_PyGC_Clear_FreeList(PyInterpreterState *interp)
{
    _PyTuple_ClearFreeList(interp);
    _PyFloat_ClearFreeList(interp);
    _PyList_ClearFreeList(interp);
    _PyDict_ClearFreeList(interp);
    _PyAsyncGen_ClearFreeLists(interp);
    _PyContext_ClearFreeList(interp);

    _PyFreeListState* state = _PyFreeListState_GET();
    _PyList_ClearFreeList(state);
}

#endif
