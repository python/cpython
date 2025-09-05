#ifndef Py_INTERNAL_OBMALLOC_INIT_H
#define Py_INTERNAL_OBMALLOC_INIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/****************************************************/
/* the default object allocator's state initializer */

#define PTA(pools, x) \
    ((poolp )((uint8_t *)&(pools.used[2*(x)]) - 2*sizeof(pymem_block *)))
#define PT(p, x)   PTA(p, x), PTA(p, x)

#define PT_8(p, start) \
    PT(p, start), \
    PT(p, start+1), \
    PT(p, start+2), \
    PT(p, start+3), \
    PT(p, start+4), \
    PT(p, start+5), \
    PT(p, start+6), \
    PT(p, start+7)

#if NB_SMALL_SIZE_CLASSES <= 8
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0) }
#elif NB_SMALL_SIZE_CLASSES <= 16
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0), PT_8(p, 8) }
#elif NB_SMALL_SIZE_CLASSES <= 24
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0), PT_8(p, 8), PT_8(p, 16) }
#elif NB_SMALL_SIZE_CLASSES <= 32
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0), PT_8(p, 8), PT_8(p, 16), PT_8(p, 24) }
#elif NB_SMALL_SIZE_CLASSES <= 40
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0), PT_8(p, 8), PT_8(p, 16), PT_8(p, 24), PT_8(p, 32) }
#elif NB_SMALL_SIZE_CLASSES <= 48
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0), PT_8(p, 8), PT_8(p, 16), PT_8(p, 24), PT_8(p, 32), PT_8(p, 40) }
#elif NB_SMALL_SIZE_CLASSES <= 56
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0), PT_8(p, 8), PT_8(p, 16), PT_8(p, 24), PT_8(p, 32), PT_8(p, 40), PT_8(p, 48) }
#elif NB_SMALL_SIZE_CLASSES <= 64
#  define _obmalloc_pools_INIT(p) \
    { PT_8(p, 0), PT_8(p, 8), PT_8(p, 16), PT_8(p, 24), PT_8(p, 32), PT_8(p, 40), PT_8(p, 48), PT_8(p, 56) }
#else
#  error "NB_SMALL_SIZE_CLASSES should be less than 64"
#endif

#define _obmalloc_global_state_INIT \
    { \
        .dump_debug_stats = -1, \
    }


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_OBMALLOC_INIT_H
