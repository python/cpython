#ifndef Py_INTERNAL_TAGGED_H
#define Py_INTERNAL_TAGGED_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stddef.h>


typedef union {
    uintptr_t bits;
} _PyTaggedPtr;

#define Py_TAG (0b0)
#define Py_TEST_TAG (0b1)

#if defined(Py_TEST_TAG)
    #define Py_OBJ_UNTAG(tagged) ((PyObject *)(uintptr_t)((tagged).bits & (~Py_TEST_TAG)))
#elif defined(Py_GIL_DISABLED)
    #define Py_OBJ_UNTAG(tagged) ((PyObject *)((tagged).bits & (~Py_TAG)))
#else
    #define Py_OBJ_UNTAG(tagged) ((PyObject *)(uintptr_t)((tagged).bits))
#endif

#if defined(Py_TEST_TAG)
    #define Py_OBJ_TAG(obj) ((_PyTaggedPtr){.bits = ((uintptr_t)(obj) | Py_TEST_TAG)})
#elif defined(Py_GIL_DISABLED)
    #define Py_OBJ_TAG(obj) ((_PyTaggedPtr){.bits = ((uintptr_t)(obj) | Py_TAG}))
#else
    #define Py_OBJ_TAG(obj) ((_PyTaggedPtr){.bits = ((uintptr_t)(obj))})
#endif

#define MAX_UNTAG_SCRATCH 10

static inline void
_Py_untag_stack(PyObject **dst, const _PyTaggedPtr *src, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dst[i] = Py_OBJ_UNTAG(src[i]);
    }
}


#define Py_XSETREF_TAGGED(dst, src) \
    do { \
        _PyTaggedPtr *_tmp_dst_ptr = _Py_CAST(_PyTaggedPtr*, &(dst)); \
        _PyTaggedPtr _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        Py_XDECREF(Py_OBJ_UNTAG(_tmp_old_dst)); \
    } while (0)

#define Py_SETREF_TAGGED(dst, src) \
    do { \
        _PyTaggedPtr *_tmp_dst_ptr = _Py_CAST(_PyTaggedPtr*, &(dst)); \
        _PyTaggedPtr _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        Py_DECREF(Py_OBJ_UNTAG(_tmp_old_dst)); \
    } while (0)

#define Py_CLEAR_TAGGED(op) \
    do { \
        _PyTaggedPtr *_tmp_op_ptr = _Py_CAST(_PyTaggedPtr*, &(op)); \
        _PyTaggedPtr _tmp_old_op = (*_tmp_op_ptr); \
        if (Py_OBJ_UNTAG(_tmp_old_op) != NULL) { \
            *_tmp_op_ptr = Py_OBJ_TAG(_Py_NULL); \
            Py_DECREF(Py_OBJ_UNTAG(_tmp_old_op)); \
        } \
    } while (0)

// KJ: These can be replaced with a more efficient routine in the future with
// deferred reference counting.
#define Py_DECREF_TAGGED(op) Py_DECREF(Py_OBJ_UNTAG(op))
#define Py_INCREF_TAGGED(op) Py_INCREF(Py_OBJ_UNTAG(op))

#define Py_XDECREF_TAGGED(op) \
    do {                      \
        if (Py_OBJ_UNTAG(op) != NULL) { \
            Py_DECREF_TAGGED(op); \
        } \
    } while (0)

static inline _PyTaggedPtr _Py_NewRef_Tagged(_PyTaggedPtr obj)
{
    Py_INCREF_TAGGED(obj);
    return obj;
}

#define Py_NewRef_Tagged(op) _Py_NewRef_Tagged(op);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TAGGED_H */
