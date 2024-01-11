#ifndef Py_INTERNAL_FREELIST_H
#define Py_INTERNAL_FREELIST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef WITH_FREELISTS
// with freelists
#  define PyList_MAXFREELIST 80
#  define PyFloat_MAXFREELIST 100
#else
#  define PyList_MAXFREELIST 0
#  define PyFloat_MAXFREELIST 0
#endif

struct _Py_list_state {
#ifdef WITH_FREELISTS
    PyListObject *free_list[PyList_MAXFREELIST];
    int numfree;
#endif
};

struct _Py_float_state {
#ifdef WITH_FREELISTS
    /* Special free list
       free_list is a singly-linked list of available PyFloatObjects,
       linked via abuse of their ob_type members. */
    int numfree;
    PyFloatObject *free_list;
#endif
};

typedef struct _Py_freelist_state {
    struct _Py_float_state float_state;
    struct _Py_list_state list;
} _PyFreeListState;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FREELIST_H */
