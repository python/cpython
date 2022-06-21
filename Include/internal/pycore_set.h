#ifndef Py_INTERNAL_SET_H
#define Py_INTERNAL_SET_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    PySetObject *si_set; /* Set to NULL when iterator is exhausted */
    Py_ssize_t si_used;
    Py_ssize_t si_pos;
    Py_ssize_t len;
} _PySetIterObject;

int _PySetIter_GetNext(_PySetIterObject *si, PyObject**);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_SET_H */
