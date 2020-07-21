#ifndef Py_AVAILABLE_H
#define Py_AVAILABLE_H

#include "Python.h"

#ifdef __has_builtin
#if __has_builtin(__builtin_available)
#define HAVE_BUILTIN_AVAILABLE 1
#endif
#endif

#if __APPLE__
#include <TargetConditionals.h>
#endif

#if __APPLE__ && HAVE_BUILTIN_AVAILABLE && !(TARGET_OS_OSX && __arm64__)
#define HAVE_UTIMENSAT_RUNTIME          __builtin_available(macos 10.13, ios 11, tvos 11, watchos 4, *)
#define HAVE_FUTIMENS_RUNTIME           __builtin_available(macos 10.13, ios 11, tvos 11, watchos 4, *)
#define HAVE_PREADV_RUNTIME             __builtin_available(macos 11.0,  ios 14, tvos 14, watchos 7, *)
#define HAVE_PWRITEV_RUNTIME            __builtin_available(macos 11.0,  ios 14, tvos 14, watchos 7, *)
#define HAVE_POSIX_SPAWN_SETSID_RUNTIME __builtin_available(macos 10.15, ios 13, tvos 13, watchos 6, *)
#define HAVE_CLOCK_GETTIME_RUNTIME      __builtin_available(macos 10.12, ios 10, tvos 10, watchos 3, *)
#define HAVE_CLOCK_SETTIME_RUNTIME      __builtin_available(macos 10.12, ios 10, tvos 10, watchos 3, *)
#define HAVE_CLOCK_GETRES_RUNTIME       __builtin_available(macos 10.12, ios 10, tvos 10, watchos 3, *)
#define HAVE_GETENTROPY_RUNTIME         __builtin_available(macos 10.12, ios 10, tvos 10, watchos 3, *)
#else
#define HAVE_UTIMENSAT_RUNTIME 1
#define HAVE_FUTIMENS_RUNTIME 1
#define HAVE_PREADV_RUNTIME 1
#define HAVE_PWRITEV_RUNTIME 1
#define HAVE_POSIX_SPAWN_SETSID_RUNTIME 1
#define HAVE_CLOCK_GETTIME_RUNTIME 1
#define HAVE_CLOCK_SETTIME_RUNTIME 1
#define HAVE_CLOCK_GETRES_RUNTIME 1
#define HAVE_GETENTROPY_RUNTIME 1
#endif

static inline void
Py_delete_method(PyMethodDef *methods, size_t size, const char *name)
{
    int last_method_index = (size /sizeof(PyMethodDef)) - 1;
    for (int i = last_method_index; i >= 0; i--) {
        if ( methods[i].ml_name && 0==strcmp(methods[i].ml_name, name)) {
            for (int j = i; methods[j].ml_name != NULL; j++) {
                methods[j] = methods[j+1];
            }
            break;
        }
    }
}

#endif

