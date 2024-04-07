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
} _PyStackRef;

#define Py_TAG (0b0)
#define Py_TEST_TAG (0b1)

#if defined(Py_TEST_TAG)
    #define Py_STACK_UNTAG_BORROWED(tagged) ((PyObject *)(uintptr_t)((tagged).bits & (~Py_TEST_TAG)))
#elif defined(Py_GIL_DISABLED)
    #define Py_STACK_UNTAG_BORROWED(tagged) ((PyObject *)((tagged).bits & (~Py_TAG)))
#else
    #define Py_STACK_UNTAG_BORROWED(tagged) ((PyObject *)(uintptr_t)((tagged).bits))
#endif

#if defined(Py_TEST_TAG)
    #define Py_STACK_TAG(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj) | Py_TEST_TAG)})
#elif defined(Py_GIL_DISABLED)
    #define Py_STACK_TAG(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj) | Py_TAG}))
#else
    #define Py_STACK_TAG(obj) ((_PyStackRef){.bits = ((uintptr_t)(obj))})
#endif

#define MAX_UNTAG_SCRATCH 10

static inline void
_Py_untag_stack(PyObject **dst, const _PyStackRef *src, size_t length) {
    for (size_t i = 0; i < length; i++) {
        dst[i] = Py_STACK_UNTAG_BORROWED(src[i]);
    }
}


#define Py_XSETREF_STACKREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = _Py_CAST(_PyStackRef*, &(dst)); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        Py_XDECREF(Py_STACK_UNTAG_BORROWED(_tmp_old_dst)); \
    } while (0)

#define Py_SETREF_STACKREF(dst, src) \
    do { \
        _PyStackRef *_tmp_dst_ptr = _Py_CAST(_PyStackRef*, &(dst)); \
        _PyStackRef _tmp_old_dst = (*_tmp_dst_ptr); \
        *_tmp_dst_ptr = (src); \
        Py_DECREF(Py_STACK_UNTAG_BORROWED(_tmp_old_dst)); \
    } while (0)

#define Py_CLEAR_STACKREF(op) \
    do { \
        _PyStackRef *_tmp_op_ptr = _Py_CAST(_PyStackRef*, &(op)); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        if (Py_STACK_UNTAG_BORROWED(_tmp_old_op) != NULL) { \
            *_tmp_op_ptr = Py_STACK_TAG(_Py_NULL); \
            Py_DECREF(Py_STACK_UNTAG_BORROWED(_tmp_old_op)); \
        } \
    } while (0)

// KJ: These can be replaced with a more efficient routine in the future with
// deferred reference counting.
#define Py_DECREF_STACKREF(op) Py_DECREF(Py_STACK_UNTAG_BORROWED(op))
#define Py_INCREF_STACKREF(op) Py_INCREF(Py_STACK_UNTAG_BORROWED(op))

#define Py_XDECREF_STACKREF(op) \
    do {                      \
        if (Py_STACK_UNTAG_BORROWED(op) != NULL) { \
            Py_DECREF_STACKREF(op); \
        } \
    } while (0)

static inline _PyStackRef Py_NewRef_StackRef(_PyStackRef obj)
{
    Py_INCREF_STACKREF(obj);
    return obj;
}

#define Py_NewRef_Tagged(op) Py_NewRef_StackRef(op)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TAGGED_H */
