#ifndef Py_INTERNAL_LIST_H
#define Py_INTERNAL_LIST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist.h"  // _PyFreeListState

extern PyObject* _PyList_Extend(PyListObject *, PyObject *);
extern void _PyList_DebugMallocStats(FILE *out);


/* runtime lifecycle */

extern void _PyList_Fini(_PyFreeListState *);


#define _PyList_ITEMS(op) _Py_RVALUE(_PyList_CAST(op)->ob_item)

extern int
_PyList_AppendTakeRefListResize(PyListObject *self, PyObject *newitem);

static inline int
_PyList_AppendTakeRef(PyListObject *self, PyObject *newitem)
{
    assert(self != NULL && newitem != NULL);
    assert(PyList_Check(self));
    Py_ssize_t len = PyList_GET_SIZE(self);
    Py_ssize_t allocated = self->allocated;
    assert((size_t)len + 1 < PY_SSIZE_T_MAX);
    if (allocated > len) {
        PyList_SET_ITEM(self, len, newitem);
        Py_SET_SIZE(self, len + 1);
        return 0;
    }
    return _PyList_AppendTakeRefListResize(self, newitem);
}

// Repeat the bytes of a buffer in place
static inline void
_Py_memory_repeat(char* dest, Py_ssize_t len_dest, Py_ssize_t len_src)
{
    assert(len_src > 0);
    Py_ssize_t copied = len_src;
    while (copied < len_dest) {
        Py_ssize_t bytes_to_copy = Py_MIN(copied, len_dest - copied);
        memcpy(dest + copied, dest, bytes_to_copy);
        copied += bytes_to_copy;
    }
}

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} _PyListIterObject;

extern PyObject *_PyList_FromArraySteal(PyObject *const *src, Py_ssize_t n);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LIST_H */
