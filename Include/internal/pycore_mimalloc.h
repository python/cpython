#ifndef Py_INTERNAL_MIMALLOC_H
#define Py_INTERNAL_MIMALLOC_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#if defined(MIMALLOC_H) || defined(MI_TYPES_H)
#  error "pycore_mimalloc.h must be included before mimalloc.h"
#endif

#include "pycore_mimalloc_structs.h"
#include "pycore_pymem.h"

#ifdef WITH_MIMALLOC
#  define MI_NO_PTHREAD_ATFORK 1
// CPython never uses mimalloc's C++ operator new APIs, so don't define a weak
// std::get_new_handler() (_ZSt15get_new_handlerv) in libpython.
#  define MI_NO_CXX_NEW_HANDLER 1
// The aligned OS allocation fallback is benign, but its warning is printed to
// stderr and breaks tests that check output (e.g. test_cmd_line under ASan).
#  define MI_NO_ALIGN_FALLBACK_WARNING 1
#  ifdef Py_GIL_DISABLED
#    define MI_PRIM_THREAD_ID   _Py_ThreadId
#    define MI_CPYTHON 1

struct mi_page_qsbr_s {
    uint64_t goal;
};

struct mi_heap_qsbr_s {
    struct _qsbr_shared *shared;
};
#  else
// Keep full pages in their owning thread's allocation statistics.
#    define MI_DEFAULT_PAGE_FULL_RETAIN -1
#  endif
#  define MI_DEBUG_UNINIT     PYMEM_CLEANBYTE
#  define MI_DEBUG_FREED      PYMEM_DEADBYTE
#  define MI_DEBUG_PADDING    PYMEM_FORBIDDENBYTE
// Do not use transparent huge pages (THP). They increase RSS on Linux.
// See https://github.com/microsoft/mimalloc/blob/v2.5.2/readme.md#environment-options
#  define MI_NO_THP 1
#ifdef Py_DEBUG
#  define MI_DEBUG 2
#else
#  define MI_DEBUG 0
#endif

#ifdef _Py_THREAD_SANITIZER
#  define MI_TSAN 1
#endif

// Used to disable warnings on failed ASAN allocations, Not disabled on
// free-threaded builds because they safely read memory after free
#if defined(_Py_ADDRESS_SANITIZER) && !Py_GIL_DISABLED
#  define MI_TRACK_ASAN 1
#endif

#ifdef __cplusplus
extern "C++" {
#endif

#include "mimalloc/mimalloc.h"
#include "mimalloc/mimalloc/types.h"
#include "mimalloc/mimalloc/internal.h"

#ifdef __cplusplus
}
#endif

#endif

#endif // Py_INTERNAL_MIMALLOC_H
