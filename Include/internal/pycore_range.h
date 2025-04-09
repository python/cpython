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
    long len;
    long end;
    long step;
    long stop;
} _PyRangeIterObject;

static inline long
_PyRangeIter_GetLengthAndStart(_PyRangeIterObject *r, long *value)
{
    long len = FT_ATOMIC_LOAD_LONG_RELAXED(r->len);
    *value = r->end - r->step * len;
    return len;
}

static inline void
_PyRangeIter_SetLength(_PyRangeIterObject *r, long len)
{
    FT_ATOMIC_STORE_LONG_RELAXED(r->len, len);
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_RANGE_H */
