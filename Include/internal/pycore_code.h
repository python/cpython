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

typedef struct {
    uint8_t original_oparg;
    uint8_t counter;
    uint16_t index;
} _PyAdaptiveEntry;


typedef struct {
    uint32_t tp_version;
    uint32_t dk_version_or_hint;
} _PyLoadAttrCache;

typedef struct {
    uint32_t module_keys_version;
    uint32_t builtin_keys_version;
} _PyLoadGlobalCache;

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
    _PyAdaptiveEntry adaptive;
    _PyLoadAttrCache load_attr;
    _PyLoadGlobalCache load_global;
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
_GetSpecializedCacheEntry(const _Py_CODEUNIT *first_instr, Py_ssize_t n)
{
    SpecializedCacheOrInstruction *last_cache_plus_one = (SpecializedCacheOrInstruction *)first_instr;
    assert(&last_cache_plus_one->code[0] == first_instr);
    return &last_cache_plus_one[-1-n].entry;
}

/* Following two functions form a pair.
 *
 * oparg_from_offset_and_index() is used to compute the oparg
 * when quickening, so that offset_from_oparg_and_nexti()
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
oparg_from_offset_and_nexti(int offset, int nexti)
{
    return offset-(nexti>>1);
}

static inline int
offset_from_oparg_and_nexti(int oparg, int nexti)
{
    return (nexti>>1)+oparg;
}

/* Get pointer to the cache entry associated with an instruction.
 * nexti is the index of the instruction plus one.
 * nexti is used as it corresponds to the instruction pointer in the interpreter.
 * This doesn't check that an entry has been allocated for that instruction. */
static inline SpecializedCacheEntry *
_GetSpecializedCacheEntryForInstruction(const _Py_CODEUNIT *first_instr, int nexti, int oparg)
{
    return _GetSpecializedCacheEntry(
        first_instr,
        offset_from_oparg_and_nexti(oparg, nexti)
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

extern Py_ssize_t _Py_QuickenedCount;


/* "Locals plus" for a code object is the set of locals + cell vars +
 * free vars.  This relates to variable names as well as offsets into
 * the "fast locals" storage array of execution frames.  The compiler
 * builds the list of names, their offsets, and the corresponding
 * kind of local.
 *
 * Those kinds represent the source of the initial value and the
 * variable's scope (as related to closures).  A "local" is an
 * argument or other variable defined in the current scope.  A "free"
 * variable is one that is defined in an outer scope and comes from
 * the function's closure.  A "cell" variable is a local that escapes
 * into an inner function as part of a closure, and thus must be
 * wrapped in a cell.  Any "local" can also be a "cell", but the
 * "free" kind is mutually exclusive with both.
 */

// Note that these all fit within a byte, as do combinations.
// Later, we will use the smaller numbers to differentiate the different
// kinds of locals (e.g. pos-only arg, varkwargs, local-only).
#define CO_FAST_LOCAL   0x20
#define CO_FAST_CELL    0x40
#define CO_FAST_FREE    0x80

typedef unsigned char _PyLocals_Kind;

static inline _PyLocals_Kind
_PyLocals_GetKind(PyObject *kinds, int i)
{
    assert(PyBytes_Check(kinds));
    assert(0 <= i && i < PyBytes_GET_SIZE(kinds));
    char *ptr = PyBytes_AS_STRING(kinds);
    return (_PyLocals_Kind)(ptr[i]);
}

static inline void
_PyLocals_SetKind(PyObject *kinds, int i, _PyLocals_Kind kind)
{
    assert(PyBytes_Check(kinds));
    assert(0 <= i && i < PyBytes_GET_SIZE(kinds));
    char *ptr = PyBytes_AS_STRING(kinds);
    ptr[i] = (char) kind;
}


struct _PyCodeConstructor {
    /* metadata */
    PyObject *filename;
    PyObject *name;
    PyObject *qualname;
    int flags;

    /* the code */
    PyObject *code;
    int firstlineno;
    PyObject *linetable;
    PyObject *endlinetable;
    PyObject *columntable;

    /* used by the code */
    PyObject *consts;
    PyObject *names;

    /* mapping frame offsets to information */
    PyObject *localsplusnames;  // Tuple of strings
    PyObject *localspluskinds;  // Bytes object, one byte per variable

    /* args (within varnames) */
    int argcount;
    int posonlyargcount;
    // XXX Replace argcount with posorkwargcount (argcount - posonlyargcount).
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

/* Getters for internal PyCodeObject data. */
PyAPI_FUNC(PyObject *) _PyCode_GetVarnames(PyCodeObject *);
PyAPI_FUNC(PyObject *) _PyCode_GetCellvars(PyCodeObject *);
PyAPI_FUNC(PyObject *) _PyCode_GetFreevars(PyCodeObject *);


/* Cache hits and misses */

static inline uint8_t
saturating_increment(uint8_t c)
{
    return c<<1;
}

static inline uint8_t
saturating_decrement(uint8_t c)
{
    return (c>>1) + 128;
}

static inline uint8_t
saturating_zero(void)
{
    return 255;
}

/* Starting value for saturating counter.
 * Technically this should be 1, but that is likely to
 * cause a bit of thrashing when we optimize then get an immediate miss.
 * We want to give the counter a change to stabilize, so we start at 3.
 */
static inline uint8_t
saturating_start(void)
{
    return saturating_zero()<<3;
}

static inline void
record_cache_hit(_PyAdaptiveEntry *entry) {
    entry->counter = saturating_increment(entry->counter);
}

static inline void
record_cache_miss(_PyAdaptiveEntry *entry) {
    entry->counter = saturating_decrement(entry->counter);
}

static inline int
too_many_cache_misses(_PyAdaptiveEntry *entry) {
    return entry->counter == saturating_zero();
}

#define BACKOFF 64

static inline void
cache_backoff(_PyAdaptiveEntry *entry) {
    entry->counter = BACKOFF;
}

/* Specialization functions */

int _Py_Specialize_LoadAttr(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name, SpecializedCacheEntry *cache);
int _Py_Specialize_LoadGlobal(PyObject *globals, PyObject *builtins, _Py_CODEUNIT *instr, PyObject *name, SpecializedCacheEntry *cache);

#define SPECIALIZATION_STATS 0
#define SPECIALIZATION_STATS_DETAILED 0

#if SPECIALIZATION_STATS

typedef struct _stats {
    uint64_t specialization_success;
    uint64_t specialization_failure;
    uint64_t hit;
    uint64_t deferred;
    uint64_t miss;
    uint64_t deopt;
    uint64_t unquickened;
#if SPECIALIZATION_STATS_DETAILED
    PyObject *miss_types;
#endif
} SpecializationStats;

extern SpecializationStats _specialization_stats[256];
#define STAT_INC(opname, name) _specialization_stats[opname].name++
void _Py_PrintSpecializationStats(void);
#else
#define STAT_INC(opname, name) ((void)0)
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
