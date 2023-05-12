#ifndef Py_INTERNAL_HASH_H
#define Py_INTERNAL_HASH_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


struct pyhash_runtime_state {
    struct {
#ifndef MS_WINDOWS
        int fd;
        dev_t st_dev;
        ino_t st_ino;
#else
    // This is a placeholder so the struct isn't empty on Windows.
    int _not_used;
#endif
    } urandom_cache;
};

#ifndef MS_WINDOWS
# define _py_urandom_cache_INIT \
    { \
        .fd = -1, \
    }
#else
# define _py_urandom_cache_INIT {0}
#endif

#define pyhash_state_INIT \
    { \
        .urandom_cache = _py_urandom_cache_INIT, \
    }


uint64_t _Py_KeyedHash(uint64_t, const char *, Py_ssize_t);


#endif  // Py_INTERNAL_HASH_H
