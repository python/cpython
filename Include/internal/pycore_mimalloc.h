#ifndef Py_INTERNAL_MIMALLOC_H
#define Py_INTERNAL_MIMALLOC_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#if defined(MIMALLOC_H) || defined(MIMALLOC_TYPES_H)
#  error "pycore_mimalloc.h must be included before mimalloc.h"
#endif

#include "pycore_pymem.h"
#define MI_DEBUG_UNINIT     PYMEM_CLEANBYTE
#define MI_DEBUG_FREED      PYMEM_DEADBYTE
#define MI_DEBUG_PADDING    PYMEM_FORBIDDENBYTE

#include "mimalloc.h"

#endif // Py_INTERNAL_MIMALLOC_H
