#include "Python.h"
#include "pycore_pystate.h"   // _Py_ClearFreeLists()

#ifndef Py_GIL_DISABLED

/* Clear all free lists
 * All free lists are cleared during the collection of the highest generation.
 * Allocated items in the free list may keep a pymalloc arena occupied.
 * Clearing the free lists may give back memory to the OS earlier.
 */
void
_PyGC_ClearAllFreeLists(PyInterpreterState *interp)
{
    _PyTuple_ClearFreeList(interp);
    _PyFloat_ClearFreeList(interp);
    _PyDict_ClearFreeList(interp);
    _PyAsyncGen_ClearFreeLists(interp);
    _PyContext_ClearFreeList(interp);

    _Py_ClearFreeLists(&interp->freelist_state, 0);
}

#endif
