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
} _Py_TaggedObject;

#define Py_OBJECT_TAG (0b0)
#define Py_OBJECT_TEST_TAG (0b1)

#if defined(Py_OBJECT_TEST_TAG)
    #define Py_CLEAR_TAG(tagged) ((PyObject *)(uintptr_t)((tagged).bits & (~Py_OBJECT_TEST_TAG)))
#elif defined(Py_GIL_DISABLED)
    #define Py_CLEAR_TAG(tagged) ((PyObject *)((tagged).bits & (~Py_OBJECT_TAG)))
#else
    #define Py_CLEAR_TAG(tagged) ((PyObject *)(uintptr_t)((tagged).bits))
#endif

#if defined(Py_OBJECT_TEST_TAG)
    #define Py_OBJ_PACK(obj) ((_Py_TaggedObject){.bits = ((uintptr_t)(obj) | Py_OBJECT_TEST_TAG)})
#elif defined(Py_GIL_DISABLED)
    #define Py_OBJ_PACK(obj) ((_Py_TaggedObject){.bits = ((uintptr_t)(obj) | Py_OBJECT_TAG}))
#else
    #define Py_OBJ_PACK(obj) ((_Py_TaggedObject){.bits = ((uintptr_t)(obj)}))
#endif

#define MAX_UNTAG_SCRATCH 10

static inline void
_Py_untag_stack(PyObject **dst, const _Py_TaggedObject *src, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dst[i] = Py_CLEAR_TAG(src[i]);
    }
}


#define Py_XSETREF_TAGGED(dst, src) \
    do { \
        _Py_TaggedObject *_tmp_dst_ptr = _Py_CAST(_Py_TaggedObject*, &(dst)); \
        _Py_TaggedObject _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        Py_XDECREF(Py_CLEAR_TAG(_tmp_old_dst)); \
    } while (0)

#define Py_CLEAR_TAGGED(op) \
    do { \
        _Py_TaggedObject *_tmp_op_ptr = _Py_CAST(_Py_TaggedObject*, &(op)); \
        _Py_TaggedObject _tmp_old_op = (*_tmp_op_ptr); \
        if (Py_CLEAR_TAG(_tmp_old_op) != NULL) { \
            *_tmp_op_ptr = Py_OBJ_PACK(_Py_NULL); \
            Py_DECREF(Py_CLEAR_TAG(_tmp_old_op)); \
        } \
    } while (0)

// KJ: These can be replaced with a more efficient routine in the future with
// deferred reference counting.
#define Py_DECREF_TAGGED(op) Py_DECREF(Py_CLEAR_TAG(op))
#define Py_INCREF_TAGGED(op) Py_INCREF(Py_CLEAR_TAG(op))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TAGGED_H */
