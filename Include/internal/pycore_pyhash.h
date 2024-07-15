#ifndef Py_INTERNAL_PYHASH_H
#define Py_INTERNAL_PYHASH_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Similar to Py_HashPointer(), but don't replace -1 with -2.
static inline Py_hash_t
_Py_HashPointerRaw(const void *ptr)
{
    uintptr_t x = (uintptr_t)ptr;
    Py_BUILD_ASSERT(sizeof(x) == sizeof(ptr));

    // Bottom 3 or 4 bits are likely to be 0; rotate x by 4 to the right
    // to avoid excessive hash collisions for dicts and sets.
    x = (x >> 4) | (x << (8 * sizeof(uintptr_t) - 4));

    Py_BUILD_ASSERT(sizeof(x) == sizeof(Py_hash_t));
    return (Py_hash_t)x;
}

// Export for '_datetime' shared extension
PyAPI_FUNC(Py_hash_t) _Py_HashBytes(const void*, Py_ssize_t);

/* Hash secret
 *
 * memory layout on 64 bit systems
 *   cccccccc cccccccc cccccccc  uc -- unsigned char[24]
 *   pppppppp ssssssss ........  fnv -- two Py_hash_t
 *   k0k0k0k0 k1k1k1k1 ........  siphash -- two uint64_t
 *   ........ ........ ssssssss  djbx33a -- 16 bytes padding + one Py_hash_t
 *   ........ ........ eeeeeeee  pyexpat XML hash salt
 *
 * memory layout on 32 bit systems
 *   cccccccc cccccccc cccccccc  uc
 *   ppppssss ........ ........  fnv -- two Py_hash_t
 *   k0k0k0k0 k1k1k1k1 ........  siphash -- two uint64_t (*)
 *   ........ ........ ssss....  djbx33a -- 16 bytes padding + one Py_hash_t
 *   ........ ........ eeee....  pyexpat XML hash salt
 *
 * (*) The siphash member may not be available on 32 bit platforms without
 *     an unsigned int64 data type.
 */
typedef union {
    /* ensure 24 bytes */
    unsigned char uc[24];
    /* two Py_hash_t for FNV */
    struct {
        Py_hash_t prefix;
        Py_hash_t suffix;
    } fnv;
    /* two uint64 for SipHash24 */
    struct {
        uint64_t k0;
        uint64_t k1;
    } siphash;
    /* a different (!) Py_hash_t for small string optimization */
    struct {
        unsigned char padding[16];
        Py_hash_t suffix;
    } djbx33a;
    struct {
        unsigned char padding[16];
        Py_hash_t hashsalt;
    } expat;
} _Py_HashSecret_t;

// Export for '_elementtree' shared extension
PyAPI_DATA(_Py_HashSecret_t) _Py_HashSecret;

#ifdef Py_DEBUG
extern int _Py_HashSecret_Initialized;
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


extern uint64_t _Py_KeyedHash(uint64_t key, const void *src, Py_ssize_t src_sz);

#endif  // !Py_INTERNAL_PYHASH_H
