#ifndef Py_INTERNAL_OBMALLOC_INIT_H
#define Py_INTERNAL_OBMALLOC_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#define PTA(x)  ((poolp )((uint8_t *)&(usedpools[2*(x)]) - 2*sizeof(pymem_block *)))
#define PT(x)   PTA(x), PTA(x)

#define PT_8(start) \
    PT(start), PT(start+1), PT(start+2), PT(start+3), PT(start+4), PT(start+5), PT(start+6), PT(start+7)

#if NB_SMALL_SIZE_CLASSES <= 8
#  define _obmalloc_pools_INIT \
    { PT_8(0) }
#elif NB_SMALL_SIZE_CLASSES <= 16
#  define _obmalloc_pools_INIT \
    { PT_8(0), PT_8(8) }
#elif NB_SMALL_SIZE_CLASSES <= 24
#  define _obmalloc_pools_INIT \
    { PT_8(0), PT_8(8), PT_8(16) }
#elif NB_SMALL_SIZE_CLASSES <= 32
#  define _obmalloc_pools_INIT \
    { PT_8(0), PT_8(8), PT_8(16), PT_8(24) }
#elif NB_SMALL_SIZE_CLASSES <= 40
#  define _obmalloc_pools_INIT \
    { PT_8(0), PT_8(8), PT_8(16), PT_8(24), PT_8(32) }
#elif NB_SMALL_SIZE_CLASSES <= 48
#  define _obmalloc_pools_INIT \
    { PT_8(0), PT_8(8), PT_8(16), PT_8(24), PT_8(32), PT_8(40) }
#elif NB_SMALL_SIZE_CLASSES <= 56
#  define _obmalloc_pools_INIT \
    { PT_8(0), PT_8(8), PT_8(16), PT_8(24), PT_8(32), PT_8(40), PT_8(48) }
#elif NB_SMALL_SIZE_CLASSES <= 64
#  define _obmalloc_pools_INIT \
    { PT_8(0), PT_8(8), PT_8(16), PT_8(24), PT_8(32), PT_8(40), PT_8(48), PT_8(56) }
#else
#  error "NB_SMALL_SIZE_CLASSES should be less than 64"
#endif

#define _obmalloc_state_INIT \
    { \
        .pools = { \
            .used = _obmalloc_pools_INIT, \
        }, \
    }


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_OBMALLOC_INIT_H
