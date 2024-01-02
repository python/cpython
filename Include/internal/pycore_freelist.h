#ifndef Py_INTERNAL_FREELIST_H
#define Py_INTERNAL_FREELIST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifndef WITH_FREELISTS
// without freelists
#  define PyList_MAXFREELIST 0
#endif

/* Empty list reuse scheme to save calls to malloc and free */
#ifndef PyList_MAXFREELIST
#  define PyList_MAXFREELIST 80
#endif

struct _Py_list_state {
#if PyList_MAXFREELIST > 0
    PyListObject *free_list[PyList_MAXFREELIST];
    int numfree;
#endif
};

typedef struct _Py_freelist_state {
    struct _Py_list_state list;
} _PyFreeListState;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FREELIST_H */
