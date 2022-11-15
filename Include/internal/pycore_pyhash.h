#ifndef Py_INTERNAL_HASH_H
#define Py_INTERNAL_HASH_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#ifndef MS_WINDOWS
struct pyhash_runtime_state {
    struct {
        int fd;
        dev_t st_dev;
        ino_t st_ino;
    } urandom_cache;
};
#endif


uint64_t _Py_KeyedHash(uint64_t, const char *, Py_ssize_t);


#endif  // Py_INTERNAL_HASH_H
