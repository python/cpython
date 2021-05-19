#ifndef Py_INTERNAL_CODE_H
#define Py_INTERNAL_CODE_H
#ifdef __cplusplus
extern "C" {
#endif

/* Legacy Opcache */

typedef struct {
    PyObject *ptr;  /* Cached pointer (borrowed reference) */
    uint64_t globals_ver;  /* ma_version of global dict */
    uint64_t builtins_ver; /* ma_version of builtin dict */
} _PyOpcache_LoadGlobal;

typedef struct {
    PyTypeObject *type;
    Py_ssize_t hint;
    unsigned int tp_version_tag;
} _PyOpCodeOpt_LoadAttr;

struct _PyOpcache {
    union {
        _PyOpcache_LoadGlobal lg;
        _PyOpCodeOpt_LoadAttr la;
    } u;
    char optimized;
};


/* PEP 659
 * Specialization and quickening structs and helper functions
 */

typedef struct {
    int32_t cache_count;
    int32_t _;
} EntryZero;

typedef union {
    void *place_holder; /* To be removed when other fields are added */
    EntryZero zero;
} HotPyCacheEntry;

#define INSTRUCTIONS_PER_ENTRY (sizeof(void *)/sizeof(_Py_CODEUNIT))

/* Maximum size of code to quicken */
#define MAX_SIZE_TO_QUICKEN 5000

typedef union _hotpy_quickened {
    _Py_CODEUNIT code[1];
    HotPyCacheEntry entry;
} HotPyCacheOrInstruction;

static inline
HotPyCacheEntry *_HotPy_GetCache(HotPyCacheOrInstruction *quickened, Py_ssize_t index)
{
     HotPyCacheEntry *last_plus_one_entry = &quickened->entry;
     return &last_plus_one_entry[-1-index];
}

void _HotPyQuickenedFree(HotPyCacheOrInstruction *quickened);

/* Returns the number of cache entries required by the code */
int _HotPyCacheEntriesNeeded(_Py_CODEUNIT *code, int len);


static inline
HotPyCacheEntry *_HotPy_GetCacheFromFirstInstr(_Py_CODEUNIT *first_instr, Py_ssize_t index)
{
    HotPyCacheOrInstruction *last_cache_plus_one = (HotPyCacheOrInstruction *)first_instr;
    assert(&last_cache_plus_one->code[0] == first_instr);
    return &last_cache_plus_one[-1-index].entry;
}

#define HOTPY_WARMUP 8

static inline void
PyCodeObject_IncrementWarmup(PyCodeObject * co)
{
    co->co_warmup--;
}

static inline int
PyCodeObject_IsWarmedUp(PyCodeObject * co)
{
    return (co->co_warmup == 0);
}

int _HotPy_Quicken(PyCodeObject *code);

/* Private API */
int _PyCode_InitOpcache(PyCodeObject *co);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
