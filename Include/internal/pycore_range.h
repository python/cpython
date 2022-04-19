#ifndef Py_INTERNAL_RANGE_H
#define Py_INTERNAL_RANGE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    sdigit index;
    sdigit start;
    sdigit step;
    sdigit len;
} _PyRangeIterObject;

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_RANGE_H */
