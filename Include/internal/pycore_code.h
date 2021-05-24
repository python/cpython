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

/* Add specialized versions of entries to this union.
 * Do not break this invariant: sizeof(SpecializedCacheEntry) == 8 */
typedef union {
    EntryZero zero;
    PyObject *object;
} SpecializedCacheEntry;

#define INSTRUCTIONS_PER_ENTRY (sizeof(SpecializedCacheEntry)/sizeof(_Py_CODEUNIT))

/* Maximum size of code to quicken */
#define MAX_SIZE_TO_QUICKEN 5000

typedef union _cache_or_instruction {
    _Py_CODEUNIT code[1];
    SpecializedCacheEntry entry;
} SpecializedCacheOrInstruction;

static inline SpecializedCacheEntry *
_GetSpecializedCacheEntry(_Py_CODEUNIT *first_instr, Py_ssize_t index)
{
    SpecializedCacheOrInstruction *last_cache_plus_one = (SpecializedCacheOrInstruction *)first_instr;
    assert(&last_cache_plus_one->code[0] == first_instr);
    return &last_cache_plus_one[-1-index].entry;
}

/* Following two functions determine the index of a cache entry from the
 * instruction index (in the instruction array) and the oparg.
 * oparg_from_offset_and_index must be the inverse of
 * offset_from_oparg_and_index
 */

static inline int
oparg_from_offset_and_index(int offset, int index)
{
    return offset-(index>>1);
}

static inline int
offset_from_oparg_and_index(int oparg, int index)
{
    return (index>>1)+oparg;
}

static inline SpecializedCacheEntry *
_GetSpecializedCacheEntryForInstruction(_Py_CODEUNIT *first_instr, int index, int oparg)
{
    return _GetSpecializedCacheEntry(
        first_instr,
        offset_from_oparg_and_index(oparg, index)
    );
}

#define QUICKENING_INITIAL_WARMUP_DELAY 8
#define QUICKENING_WARMUP_COLDEST 1

static inline void
PyCodeObject_IncrementWarmup(PyCodeObject * co)
{
    co->co_warmup++;
}

static inline int
PyCodeObject_IsWarmedUp(PyCodeObject * co)
{
    return (co->co_warmup == 0);
}

int _Py_Quicken(PyCodeObject *code);

/* Private API */
int _PyCode_InitOpcache(PyCodeObject *co);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
