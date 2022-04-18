#ifndef Py_INTERNAL_LONGOBJECT_H
#define Py_INTERNAL_LONGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef WITH_FREELISTS
// without freelists
#  define PyLong_MAXFREELIST 0
#endif

#ifndef PyLong_MAXFREELIST
#  define PyLong_MAXFREELIST   100
#endif

struct _Py_long_state {
#if PyLong_MAXFREELIST > 0
    /* Special free list
       free_list is a singly-linked list of available PyLongObjects,
       linked via abuse of their ob_type members. */
    int numfree;
    PyLongObject *free_list;
#endif
};

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONGOBJECT_H */
