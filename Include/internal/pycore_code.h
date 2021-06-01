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
    int32_t _; /* Force 8 byte size */
} _PyEntryZero;

/* Add specialized versions of entries to this union.
 *
 * Do not break the invariant: sizeof(SpecializedCacheEntry) == 8
 * Preserving this invariant is necessary because:
    - If any one form uses more space, then all must and on 64 bit machines
      this is likely to double the memory consumption of caches
    - The function for calculating the offset of caches assumes a 4:1
      cache:instruction size ratio. Changing that would need careful
      analysis to choose a new function.
 */
typedef union {
    _PyEntryZero zero;
    PyObject *object;
} SpecializedCacheEntry;

#define INSTRUCTIONS_PER_ENTRY (sizeof(SpecializedCacheEntry)/sizeof(_Py_CODEUNIT))

/* Maximum size of code to quicken, in code units. */
#define MAX_SIZE_TO_QUICKEN 5000

typedef union _cache_or_instruction {
    _Py_CODEUNIT code[1];
    SpecializedCacheEntry entry;
} SpecializedCacheOrInstruction;

/* Get pointer to the nth cache entry, from the first instruction and n.
 * Cache entries are indexed backwards, with [count-1] first in memory, and [0] last.
 * The zeroth entry immediately precedes the instructions.
 */
static inline SpecializedCacheEntry *
_GetSpecializedCacheEntry(_Py_CODEUNIT *first_instr, Py_ssize_t n)
{
    SpecializedCacheOrInstruction *last_cache_plus_one = (SpecializedCacheOrInstruction *)first_instr;
    assert(&last_cache_plus_one->code[0] == first_instr);
    return &last_cache_plus_one[-1-n].entry;
}

/* Following two functions form a pair.
 *
 * oparg_from_offset_and_index() is used to compute the oparg
 * when quickening, so that offset_from_oparg_and_index()
 * can be used at runtime to compute the offset.
 *
 * The relationship between the three values is currently
 *     offset == (index>>1) + oparg
 * This relation is chosen based on the following observations:
 * 1. typically 1 in 4 instructions need a cache
 * 2. instructions that need a cache typically use 2 entries
 *  These observations imply:  offset ≈ index/2
 *  We use the oparg to fine tune the relation to avoid wasting space
 * and allow consecutive instructions to use caches.
 *
 * If the number of cache entries < number of instructions/2 we will waste
 * some small amoount of space.
 * If the number of cache entries > (number of instructions/2) + 255, then
 * some instructions will not be able to use a cache.
 * In practice, we expect some small amount of wasted space in a shorter functions
 * and only functions exceeding a 1000 lines or more not to have enugh cache space.
 *
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

/* Get pointer to the cache entry associated with an instruction.
 This doesn't check that an entry has been allocated for that instruction. */
static inline SpecializedCacheEntry *
_GetSpecializedCacheEntryForInstruction(_Py_CODEUNIT *first_instr, int index, int oparg)
{
    return _GetSpecializedCacheEntry(
        first_instr,
        offset_from_oparg_and_index(oparg, index)
    );
}

#define QUICKENING_WARMUP_DELAY 8

/* We want to compare to zero for efficiency, so we offset values accordingly */
#define QUICKENING_INITIAL_WARMUP_VALUE (-QUICKENING_WARMUP_DELAY)
#define QUICKENING_WARMUP_COLDEST 1

static inline void
PyCodeObject_IncrementWarmup(PyCodeObject * co)
{
    co->co_warmup++;
}

/* Used by the interpreter to determine when a code object should be quickened */
static inline int
PyCodeObject_IsWarmedUp(PyCodeObject * co)
{
    return (co->co_warmup == 0);
}

int _Py_Quicken(PyCodeObject *code);


struct _PyCodeConstructor {
    /* metadata */
    PyObject *filename;
    PyObject *name;
    int flags;

    /* the code */
    PyObject *code;
    int firstlineno;
    PyObject *linetable;

    /* used by the code */
    PyObject *consts;
    PyObject *names;

    /* mapping frame offsets to information */
    PyObject *varnames;
    PyObject *cellvars;
    PyObject *freevars;

    /* args (within varnames) */
    int argcount;
    int posonlyargcount;
    int kwonlyargcount;

    /* needed to create the frame */
    int stacksize;

    /* used by the eval loop */
    PyObject *exceptiontable;
};

// Using an "arguments struct" like this is helpful for maintainability
// in a case such as this with many parameters.  It does bear a risk:
// if the struct changes and callers are not updated properly then the
// compiler will not catch problems (like a missing argument).  This can
// cause hard-to-debug problems.  The risk is mitigated by the use of
// check_code() in codeobject.c.  However, we may decide to switch
// back to a regular function signature.  Regardless, this approach
// wouldn't be appropriate if this weren't a strictly internal API.
// (See the comments in https://github.com/python/cpython/pull/26258.)
PyAPI_FUNC(int) _PyCode_Validate(struct _PyCodeConstructor *);
PyAPI_FUNC(PyCodeObject *) _PyCode_New(struct _PyCodeConstructor *);


/* Private API */

int _PyCode_InitOpcache(PyCodeObject *co);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
