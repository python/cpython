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
    PyObject *start;
    PyObject *stop;
    PyObject *step;
    PyObject *length;
} rangeobject;

typedef struct {
    PyObject_HEAD
    long start;
    long step;
    long len;
} _PyRangeIterObject;

// Does this range have step == 1 and both start and stop in compact int range?
static inline int
_PyRange_IsSimpleCompact(PyObject *range) {
    assert(PyRange_Check(range));
    rangeobject *r = (rangeobject*)range;
    if (_PyLong_IsCompact((PyLongObject *)r->start) &&
        _PyLong_IsCompact((PyLongObject *)r->stop) &&
        r->step == _PyLong_GetOne()
    ) {
        return 1;
    }
    return 0;
}

static inline Py_ssize_t
_PyRange_GetStartIfCompact(PyObject *range)
{
    assert(PyRange_Check(range));
    rangeobject *r = (rangeobject*)range;
    assert(_PyLong_IsCompact((PyLongObject *)r->start));
    return _PyLong_CompactValue((PyLongObject *)r->start);
}

static inline Py_ssize_t
_PyRange_GetStopIfCompact(PyObject *range) {
    assert(PyRange_Check(range));
    rangeobject *r = (rangeobject*)range;
    assert(_PyLong_IsCompact((PyLongObject *)r->stop));
    return _PyLong_CompactValue((PyLongObject *)r->stop);
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_RANGE_H */
