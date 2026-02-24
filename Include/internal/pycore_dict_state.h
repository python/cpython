#ifndef Py_INTERNAL_DICT_STATE_H
#define Py_INTERNAL_DICT_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"          // PyMutex

#define DICT_MAX_WATCHERS 8
#define DICT_WATCHED_MUTATION_BITS 4

struct _Py_dict_state {
    uint32_t next_keys_version;
    PyDict_WatchCallback watchers[DICT_MAX_WATCHERS];
    PyMutex watcher_mutex;        // Protects the watchers array (free-threaded builds)
};

#define _dict_state_INIT \
    { \
        .next_keys_version = 2, \
    }


#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DICT_STATE_H */
