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

#if defined(HAVE_PR_SET_VMA_ANON_NAME) && defined(__linux__)
static inline void
_PyAnnotateMemoryMap(void *addr, size_t size, const char *name)
{
#ifndef Py_DEBUG
    if (!_Py_GetConfig()->dev_mode) {
        return;
    }
#endif
    assert(strlen(name) < 80);
    int old_errno = errno;
    prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, (unsigned long)addr, size, name);
    /* Ignore errno from prctl */
    /* See: https://bugzilla.redhat.com/show_bug.cgi?id=2302746 */
    errno = old_errno;
}
#else
static inline void
_PyAnnotateMemoryMap(void *Py_UNUSED(addr), size_t Py_UNUSED(size), const char *Py_UNUSED(name))
{
}
#endif

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_MMAP_H
