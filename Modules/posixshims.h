/* posixshims.h: resolve newer libc/syscall wrappers at runtime, not build time.

   CPython gates functions like copy_file_range() on a configure-time
   AC_CHECK_FUNCS probe, compiling them out when the build libc lacks the symbol.
   That permanently disables them in a redistributable built against an old glibc
   even when it later runs on a newer one.  Instead, on Linux we always expose
   _Py_<func>(), resolve the libc symbol once at load time via dlsym(RTLD_DEFAULT)
   from a constructor, and fall back to the raw syscall.  A constructor (vs. lazy
   resolution) keeps dlsym(), which is not async-signal-safe, out of signal
   handlers and the fork()/exec() window, and lets the cache skip synchronization.

   Each _Py_<func>() is declared explicitly; only the shared body is a macro.
   Off Linux, each function keeps its classic build-time HAVE_* direct call. */

#ifndef Py_POSIXSHIMS_H
#define Py_POSIXSHIMS_H

/* Needs ELF constructors, dlsym() and Linux syscall numbers. */
#if defined(__linux__) && (defined(__GNUC__) || defined(__clang__)) \
        && defined(HAVE_DLFCN_H) && defined(HAVE_SYS_SYSCALL_H)
#  define _Py_HAVE_POSIX_SHIMS
#endif

#ifdef _Py_HAVE_POSIX_SHIMS

#include <dlfcn.h>          // dlsym(), RTLD_DEFAULT
#include <sys/syscall.h>    // __NR_* syscall numbers
#include <sys/types.h>      // off_t, ssize_t
#include <unistd.h>         // syscall()

/* The raw-syscall fallback needs a syscall number.  If a stale build header
   predates one, default it to an invalid number so the shim still builds and the
   dlsym'd libc symbol stays usable; only the syscall fallback is lost, returning
   -1/ENOSYS at runtime.  <sys/syscall.h> above is the sole source of these
   numbers in this translation unit, so nothing redefines them afterwards. */
#ifndef __NR_copy_file_range
#  define __NR_copy_file_range (-1)
#endif
#ifndef __NR_memfd_create
#  define __NR_memfd_create (-1)
#endif
#ifndef __NR_pidfd_open
#  define __NR_pidfd_open (-1)
#endif
#ifndef __NR_pidfd_getfd
#  define __NR_pidfd_getfd (-1)
#endif

/* A per-function cache plus a constructor that resolves the libc symbol once at
   load time.  The __asm__ barrier after dlsym() is load-bearing under LTO:
   without it the compiler may tail-call dlsym(), breaking glibc's
   return-address caller lookup and crashing at startup (glibc BZ #34156). */
#define _Py_SHIM_RESOLVER(func)                                               \
    static void *_Py_shim_##func;                                             \
    __attribute__((constructor))                                             \
    static void _Py_shim_##func##_resolve(void) {                             \
        void *p = dlsym(RTLD_DEFAULT, #func);                                 \
        __asm__ volatile("" ::: "memory");                                    \
        _Py_shim_##func = p;                                                  \
    }

/* Body of an explicitly declared _Py_<func>() wrapper: call the resolved libc
   symbol if present, else fall back to syscall(__NR_<func>, ...).  On a kernel
   too old for the syscall that returns -1/ENOSYS, which posixmodule.c reports as
   an ordinary OSError.  The cast is __typeof__ of _Py_<func> (the wrapper
   itself), not of <func>, which may be undeclared at build time. */
#define _Py_SHIM_SYSCALL(func, ...)                                           \
    if (_Py_shim_##func != NULL) {                                            \
        return ((__typeof__(&_Py_##func)) _Py_shim_##func)(__VA_ARGS__);      \
    }                                                                         \
    return syscall(__NR_##func, __VA_ARGS__)

#endif /* _Py_HAVE_POSIX_SHIMS */


/* ---- copy_file_range() (glibc 2.27) ------------------------------------- */

#if defined(_Py_HAVE_POSIX_SHIMS)
#  define _Py_HAVE_COPY_FILE_RANGE
_Py_SHIM_RESOLVER(copy_file_range)
/* off_t is 64-bit (_FILE_OFFSET_BITS=64), matching the kernel's loff_t. */
static inline ssize_t
_Py_copy_file_range(int fd_in, off_t *off_in, int fd_out, off_t *off_out,
                    size_t len, unsigned int flags)
{
    _Py_SHIM_SYSCALL(copy_file_range,
                     fd_in, off_in, fd_out, off_out, len, flags);
}

#elif defined(HAVE_COPY_FILE_RANGE)
#  define _Py_HAVE_COPY_FILE_RANGE
#  define _Py_copy_file_range copy_file_range
#endif


/* ---- memfd_create() (glibc 2.27) ---------------------------------------- */

#if defined(_Py_HAVE_POSIX_SHIMS)
#  define _Py_HAVE_MEMFD_CREATE
_Py_SHIM_RESOLVER(memfd_create)
static inline int
_Py_memfd_create(const char *name, unsigned int flags)
{
    _Py_SHIM_SYSCALL(memfd_create, name, flags);
}

#elif defined(HAVE_MEMFD_CREATE)
#  define _Py_HAVE_MEMFD_CREATE
#  define _Py_memfd_create memfd_create
#endif


/* ---- pidfd_open(), pidfd_getfd() (glibc 2.36) --------------------------- *

   Previously unconditional raw syscalls; the shim now prefers glibc's wrapper.
   Linux-only from the start (no non-Linux branch); the Android API floor from
   the original guards is preserved. */

#if defined(_Py_HAVE_POSIX_SHIMS) \
        && !(defined(__ANDROID__) && __ANDROID_API__ < 31)
#  define _Py_HAVE_PIDFD_OPEN
_Py_SHIM_RESOLVER(pidfd_open)
static inline int
_Py_pidfd_open(pid_t pid, unsigned int flags)
{
    _Py_SHIM_SYSCALL(pidfd_open, pid, flags);
}
#endif

#if defined(_Py_HAVE_POSIX_SHIMS) \
        && !(defined(__ANDROID__) && __ANDROID_API__ < 31)
#  define _Py_HAVE_PIDFD_GETFD
_Py_SHIM_RESOLVER(pidfd_getfd)
static inline int
_Py_pidfd_getfd(int pidfd, int targetfd, unsigned int flags)
{
    _Py_SHIM_SYSCALL(pidfd_getfd, pidfd, targetfd, flags);
}
#endif

#endif /* !Py_POSIXSHIMS_H */
