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
    long start;
    long step;
    long len;
} _PyRangeIterObject;

// Does this range have start == 0, step == 1 and step in compact int range?
int _PyRange_IsSimpleCompact(PyObject *range);
Py_ssize_t _PyRange_GetStopIfCompact(PyObject *range);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_RANGE_H */
