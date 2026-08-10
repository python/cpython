#include "Python.h"
#include "pycore_freelist.h"   // _PyObject_ClearFreeLists()

#ifndef Py_GIL_DISABLED

/* Clear all free lists
 * All free lists are cleared during the collection of the highest generation.
 * Allocated items in the free list may keep a pymalloc arena occupied.
 * Clearing the free lists may give back memory to the OS earlier.
 */
void
_PyGC_ClearAllFreeLists(PyInterpreterState *interp)
{
    _PyObject_ClearFreeLists(&interp->object_state.freelists, 0);
}

#endif
