#ifndef Py_INTERNAL_MMAP_H
#define Py_INTERNAL_MMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pystate.h"

#if defined(HAVE_PR_SET_VMA_ANON_NAME) && defined(__linux__)
#  include <linux/prctl.h>
#  include <sys/prctl.h>
#endif

static inline int
_PyAnnotateMemoryMap(void *addr, size_t size, const char *name)
{
#if defined(HAVE_PR_SET_VMA_ANON_NAME) && defined(__linux__)
    if (_Py_GetConfig()->dev_mode) {
        assert(strlen(name) < 80);
        prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, (unsigned long)addr, size, name);
        // Ignore errno from prctl
        // See: https://bugzilla.redhat.com/show_bug.cgi?id=2302746
        errno = 0;
    }
#endif
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_MMAP_H
