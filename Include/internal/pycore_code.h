#ifndef Py_INTERNAL_CODE_H
#define Py_INTERNAL_CODE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_backoff.h"     // _Py_BackoffCounter
#include "pycore_structs.h"     // _Py_CODEUNIT
#include "pycore_tstate.h"      // _PyThreadStateImpl


#define _PyCode_CODE(CO) _Py_RVALUE((_Py_CODEUNIT *)(CO)->co_code_adaptive)
#define _PyCode_NBYTES(CO) (Py_SIZE(CO) * (Py_ssize_t)sizeof(_Py_CODEUNIT))


/* These macros only remain defined for compatibility. */
#define _Py_OPCODE(word) ((word).op.code)
#define _Py_OPARG(word) ((word).op.arg)

static inline _Py_CODEUNIT
_py_make_codeunit(uint8_t opcode, uint8_t oparg)
{
    // No designated initialisers because of C++ compat
    _Py_CODEUNIT word;
    word.op.code = opcode;
    word.op.arg = oparg;
    return word;
}

static inline void
_py_set_opcode(_Py_CODEUNIT *word, uint8_t opcode)
{
    word->op.code = opcode;
}

#define _Py_MAKE_CODEUNIT(opcode, oparg) _py_make_codeunit((opcode), (oparg))
#define _Py_SET_OPCODE(word, opcode) _py_set_opcode(&(word), (opcode))


// We hide some of the newer PyCodeObject fields behind macros.
// This helps with backporting certain changes to 3.12.
#define _PyCode_HAS_EXECUTORS(CODE) \
    (CODE->co_executors != NULL)
#define _PyCode_HAS_INSTRUMENTATION(CODE) \
    (CODE->_co_instrumentation_version > 0)


extern PyStatus _PyCode_Init(PyInterpreterState *interp);
extern void _PyCode_Fini(PyInterpreterState *interp);


/* PEP 659
 * Specialization and quickening structs and helper functions
 */


// Inline caches. If you change the number of cache entries for an instruction,
// you must *also* update the number of cache entries in Lib/opcode.py and bump
// the magic number in Lib/importlib/_bootstrap_external.py!

#define CACHE_ENTRIES(cache) (sizeof(cache)/sizeof(_Py_CODEUNIT))

typedef struct {
    _Py_BackoffCounter counter;
    uint16_t module_keys_version;
    uint16_t builtin_keys_version;
    uint16_t index;
} _PyLoadGlobalCache;

#define INLINE_CACHE_ENTRIES_LOAD_GLOBAL CACHE_ENTRIES(_PyLoadGlobalCache)

typedef struct {
    _Py_BackoffCounter counter;
    uint16_t external_cache[4];
} _PyBinaryOpCache;

#define INLINE_CACHE_ENTRIES_BINARY_OP CACHE_ENTRIES(_PyBinaryOpCache)

typedef struct {
    _Py_BackoffCounter counter;
} _PyUnpackSequenceCache;

#define INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE \
    CACHE_ENTRIES(_PyUnpackSequenceCache)

typedef struct {
    _Py_BackoffCounter counter;
} _PyCompareOpCache;

#define INLINE_CACHE_ENTRIES_COMPARE_OP CACHE_ENTRIES(_PyCompareOpCache)

typedef struct {
    _Py_BackoffCounter counter;
} _PySuperAttrCache;

#define INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR CACHE_ENTRIES(_PySuperAttrCache)

typedef struct {
    _Py_BackoffCounter counter;
    uint16_t version[2];
    uint16_t index;
} _PyAttrCache;

typedef struct {
    _Py_BackoffCounter counter;
    uint16_t type_version[2];
    union {
        uint16_t keys_version[2];
        uint16_t dict_offset;
    };
    uint16_t descr[4];
} _PyLoadMethodCache;


// MUST be the max(_PyAttrCache, _PyLoadMethodCache)
#define INLINE_CACHE_ENTRIES_LOAD_ATTR CACHE_ENTRIES(_PyLoadMethodCache)

#define INLINE_CACHE_ENTRIES_STORE_ATTR CACHE_ENTRIES(_PyAttrCache)

typedef struct {
    _Py_BackoffCounter counter;
    uint16_t func_version[2];
} _PyCallCache;

#define INLINE_CACHE_ENTRIES_CALL CACHE_ENTRIES(_PyCallCache)
#define INLINE_CACHE_ENTRIES_CALL_KW CACHE_ENTRIES(_PyCallCache)

typedef struct {
    _Py_BackoffCounter counter;
} _PyStoreSubscrCache;

#define INLINE_CACHE_ENTRIES_STORE_SUBSCR CACHE_ENTRIES(_PyStoreSubscrCache)

typedef struct {
    _Py_BackoffCounter counter;
} _PyForIterCache;

#define INLINE_CACHE_ENTRIES_FOR_ITER CACHE_ENTRIES(_PyForIterCache)

typedef struct {
    _Py_BackoffCounter counter;
} _PySendCache;

#define INLINE_CACHE_ENTRIES_SEND CACHE_ENTRIES(_PySendCache)

typedef struct {
    _Py_BackoffCounter counter;
    uint16_t version[2];
} _PyToBoolCache;

#define INLINE_CACHE_ENTRIES_TO_BOOL CACHE_ENTRIES(_PyToBoolCache)

typedef struct {
    _Py_BackoffCounter counter;
} _PyContainsOpCache;

#define INLINE_CACHE_ENTRIES_CONTAINS_OP CACHE_ENTRIES(_PyContainsOpCache)

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
#define CO_FAST_ARG_POS (0x02)  // pos-only, pos-or-kw, varargs
#define CO_FAST_ARG_KW  (0x04)  // kw-only, pos-or-kw, varkwargs
#define CO_FAST_ARG_VAR (0x08)  // varargs, varkwargs
#define CO_FAST_ARG     (CO_FAST_ARG_POS | CO_FAST_ARG_KW | CO_FAST_ARG_VAR)
#define CO_FAST_HIDDEN  (0x10)
#define CO_FAST_LOCAL   (0x20)
#define CO_FAST_CELL    (0x40)
#define CO_FAST_FREE    (0x80)

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
extern int _PyCode_Validate(struct _PyCodeConstructor *);
extern PyCodeObject* _PyCode_New(struct _PyCodeConstructor *);


/* Private API */

/* Getters for internal PyCodeObject data. */
extern PyObject* _PyCode_GetVarnames(PyCodeObject *);
extern PyObject* _PyCode_GetCellvars(PyCodeObject *);
extern PyObject* _PyCode_GetFreevars(PyCodeObject *);
extern PyObject* _PyCode_GetCode(PyCodeObject *);

/** API for initializing the line number tables. */
PyAPI_FUNC(int) _PyCode_InitAddressRange(PyCodeObject* co, PyCodeAddressRange *bounds);

/** Out of process API for initializing the location table. */
extern void _PyLineTable_InitAddressRange(
    const char *linetable,
    Py_ssize_t length,
    int firstlineno,
    PyCodeAddressRange *range);

/** API for traversing the line number table. */
PyAPI_FUNC(int) _PyLineTable_NextAddressRange(PyCodeAddressRange *range);
extern int _PyLineTable_PreviousAddressRange(PyCodeAddressRange *range);

// Similar to PyCode_Addr2Line(), but return -1 if the code object is invalid
// and can be called without an attached tstate. Used by dump_frame() in
// Python/traceback.c. The function uses heuristics to detect freed memory,
// it's not 100% reliable.
extern int _PyCode_SafeAddr2Line(PyCodeObject *co, int addr);


/** API for executors */
extern void _PyCode_Clear_Executors(PyCodeObject *code);


#ifdef Py_GIL_DISABLED
// gh-115999 tracks progress on addressing this.
#define ENABLE_SPECIALIZATION 0
// Use this to enable specialization families once they are thread-safe. All
// uses will be replaced with ENABLE_SPECIALIZATION once all families are
// thread-safe.
#define ENABLE_SPECIALIZATION_FT 1
#else
#define ENABLE_SPECIALIZATION 1
#define ENABLE_SPECIALIZATION_FT ENABLE_SPECIALIZATION
#endif

/* Specialization functions, these are exported only for other re-generated
 * interpreters to call */

PyAPI_FUNC(void) _Py_Specialize_LoadSuperAttr(_PyStackRef global_super, _PyStackRef cls,
                                         _Py_CODEUNIT *instr, int load_method);
PyAPI_FUNC(void) _Py_Specialize_LoadAttr(_PyStackRef owner, _Py_CODEUNIT *instr,
                                    PyObject *name);
PyAPI_FUNC(void) _Py_Specialize_StoreAttr(_PyStackRef owner, _Py_CODEUNIT *instr,
                                     PyObject *name);
PyAPI_FUNC(void) _Py_Specialize_LoadGlobal(PyObject *globals, PyObject *builtins,
                                      _Py_CODEUNIT *instr, PyObject *name);
PyAPI_FUNC(void) _Py_Specialize_StoreSubscr(_PyStackRef container, _PyStackRef sub,
                                       _Py_CODEUNIT *instr);
PyAPI_FUNC(void) _Py_Specialize_Call(_PyStackRef callable, _PyStackRef self_or_null,
                                _Py_CODEUNIT *instr, int nargs);
PyAPI_FUNC(void) _Py_Specialize_CallKw(_PyStackRef callable, _Py_CODEUNIT *instr,
                                  int nargs);
PyAPI_FUNC(void) _Py_Specialize_BinaryOp(_PyStackRef lhs, _PyStackRef rhs, _Py_CODEUNIT *instr,
                                    int oparg, _PyStackRef *locals);
PyAPI_FUNC(void) _Py_Specialize_CompareOp(_PyStackRef lhs, _PyStackRef rhs,
                                     _Py_CODEUNIT *instr, int oparg);
PyAPI_FUNC(void) _Py_Specialize_UnpackSequence(_PyStackRef seq, _Py_CODEUNIT *instr,
                                          int oparg);
PyAPI_FUNC(void) _Py_Specialize_ForIter(_PyStackRef iter, _PyStackRef null_or_index, _Py_CODEUNIT *instr, int oparg);
PyAPI_FUNC(void) _Py_Specialize_Send(_PyStackRef receiver, _Py_CODEUNIT *instr);
PyAPI_FUNC(void) _Py_Specialize_ToBool(_PyStackRef value, _Py_CODEUNIT *instr);
PyAPI_FUNC(void) _Py_Specialize_ContainsOp(_PyStackRef value, _Py_CODEUNIT *instr);
PyAPI_FUNC(void) _Py_GatherStats_GetIter(_PyStackRef iterable);

// Utility functions for reading/writing 32/64-bit values in the inline caches.
// Great care should be taken to ensure that these functions remain correct and
// performant! They should compile to just "move" instructions on all supported
// compilers and platforms.

// We use memcpy to let the C compiler handle unaligned accesses and endianness
// issues for us. It also seems to produce better code than manual copying for
// most compilers (see https://blog.regehr.org/archives/959 for more info).

static inline void
write_u32(uint16_t *p, uint32_t val)
{
    memcpy(p, &val, sizeof(val));
}

static inline void
write_u64(uint16_t *p, uint64_t val)
{
    memcpy(p, &val, sizeof(val));
}

static inline void
write_ptr(uint16_t *p, void *val)
{
    memcpy(p, &val, sizeof(val));
}

static inline uint16_t
read_u16(uint16_t *p)
{
    return *p;
}

static inline uint32_t
read_u32(uint16_t *p)
{
    uint32_t val;
    memcpy(&val, p, sizeof(val));
    return val;
}

static inline uint64_t
read_u64(uint16_t *p)
{
    uint64_t val;
    memcpy(&val, p, sizeof(val));
    return val;
}

static inline PyObject *
read_obj(uint16_t *p)
{
    PyObject *val;
    memcpy(&val, p, sizeof(val));
    return val;
}

/* See InternalDocs/exception_handling.md for details.
 */
static inline unsigned char *
parse_varint(unsigned char *p, int *result) {
    int val = p[0] & 63;
    while (p[0] & 64) {
        p++;
        val = (val << 6) | (p[0] & 63);
    }
    *result = val;
    return p+1;
}

static inline int
write_varint(uint8_t *ptr, unsigned int val)
{
    int written = 1;
    while (val >= 64) {
        *ptr++ = 64 | (val & 63);
        val >>= 6;
        written++;
    }
    *ptr = (uint8_t)val;
    return written;
}

static inline int
write_signed_varint(uint8_t *ptr, int val)
{
    unsigned int uval;
    if (val < 0) {
        // (unsigned int)(-val) has an undefined behavior for INT_MIN
        uval = ((0 - (unsigned int)val) << 1) | 1;
    }
    else {
        uval = (unsigned int)val << 1;
    }
    return write_varint(ptr, uval);
}

static inline int
write_location_entry_start(uint8_t *ptr, int code, int length)
{
    assert((code & 15) == code);
    *ptr = 128 | (uint8_t)(code << 3) | (uint8_t)(length - 1);
    return 1;
}


/** Counters
 * The first 16-bit value in each inline cache is a counter.
 *
 * When counting executions until the next specialization attempt,
 * exponential backoff is used to reduce the number of specialization failures.
 * See pycore_backoff.h for more details.
 * On a specialization failure, the backoff counter is restarted.
 */

// A value of 1 means that we attempt to specialize the *second* time each
// instruction is executed. Executing twice is a much better indicator of
// "hotness" than executing once, but additional warmup delays only prevent
// specialization. Most types stabilize by the second execution, too:
#define ADAPTIVE_WARMUP_VALUE 1
#define ADAPTIVE_WARMUP_BACKOFF 1

// A value of 52 means that we attempt to re-specialize after 53 misses (a prime
// number, useful for avoiding artifacts if every nth value is a different type
// or something). Setting the backoff to 0 means that the counter is reset to
// the same state as a warming-up instruction (value == 1, backoff == 1) after
// deoptimization. This isn't strictly necessary, but it is bit easier to reason
// about when thinking about the opcode transitions as a state machine:
#define ADAPTIVE_COOLDOWN_VALUE 52
#define ADAPTIVE_COOLDOWN_BACKOFF 0

// Can't assert this in pycore_backoff.h because of header order dependencies
#if JUMP_BACKWARD_INITIAL_VALUE <= ADAPTIVE_COOLDOWN_VALUE
#  error  "JIT threshold value should be larger than adaptive cooldown value"
#endif
#if SIDE_EXIT_INITIAL_VALUE <= ADAPTIVE_COOLDOWN_VALUE
#  error  "Cold exit value should be larger than adaptive cooldown value"
#endif

static inline _Py_BackoffCounter
adaptive_counter_bits(uint16_t value, uint16_t backoff) {
    return make_backoff_counter(value, backoff);
}

static inline _Py_BackoffCounter
adaptive_counter_warmup(void) {
    return adaptive_counter_bits(ADAPTIVE_WARMUP_VALUE,
                                 ADAPTIVE_WARMUP_BACKOFF);
}

static inline _Py_BackoffCounter
adaptive_counter_cooldown(void) {
    return adaptive_counter_bits(ADAPTIVE_COOLDOWN_VALUE,
                                 ADAPTIVE_COOLDOWN_BACKOFF);
}

static inline _Py_BackoffCounter
adaptive_counter_backoff(_Py_BackoffCounter counter) {
    return restart_backoff_counter(counter);
}

/* Specialization Extensions */

/* callbacks for an external specialization */
typedef int (*binaryopguardfunc)(PyObject *lhs, PyObject *rhs);
typedef PyObject *(*binaryopactionfunc)(PyObject *lhs, PyObject *rhs);

typedef struct {
    int oparg;
    binaryopguardfunc guard;
    binaryopactionfunc action;
} _PyBinaryOpSpecializationDescr;

/* Comparison bit masks. */

/* Note this evaluates its arguments twice each */
#define COMPARISON_BIT(x, y) (1 << (2 * ((x) >= (y)) + ((x) <= (y))))

/*
 * The following bits are chosen so that the value of
 * COMPARSION_BIT(left, right)
 * masked by the values below will be non-zero if the
 * comparison is true, and zero if it is false */

/* This is for values that are unordered, ie. NaN, not types that are unordered, e.g. sets */
#define COMPARISON_UNORDERED 1

#define COMPARISON_LESS_THAN 2
#define COMPARISON_GREATER_THAN 4
#define COMPARISON_EQUALS 8

#define COMPARISON_NOT_EQUALS (COMPARISON_UNORDERED | COMPARISON_LESS_THAN | COMPARISON_GREATER_THAN)

PyAPI_FUNC(int) _Py_Instrument(PyCodeObject *co, PyInterpreterState *interp);

extern _Py_CODEUNIT _Py_GetBaseCodeUnit(PyCodeObject *code, int offset);

extern int _PyInstruction_GetLength(PyCodeObject *code, int offset);

extern PyObject *_PyInstrumentation_BranchesIterator(PyCodeObject *code);

struct _PyCode8 _PyCode_DEF(8);

PyAPI_DATA(const struct _PyCode8) _Py_InitCleanup;

#ifdef Py_GIL_DISABLED

static inline _PyCodeArray *
_PyCode_GetTLBCArray(PyCodeObject *co)
{
    return _Py_STATIC_CAST(_PyCodeArray *,
                           _Py_atomic_load_ptr_acquire(&co->co_tlbc));
}

// Return a pointer to the thread-local bytecode for the current thread, if it
// exists.
static inline _Py_CODEUNIT *
_PyCode_GetTLBCFast(PyThreadState *tstate, PyCodeObject *co)
{
    _PyCodeArray *code = _PyCode_GetTLBCArray(co);
    int32_t idx = ((_PyThreadStateImpl*) tstate)->tlbc_index;
    if (idx < code->size && code->entries[idx] != NULL) {
        return (_Py_CODEUNIT *) code->entries[idx];
    }
    return NULL;
}

// Return a pointer to the thread-local bytecode for the current thread,
// creating it if necessary.
extern _Py_CODEUNIT *_PyCode_GetTLBC(PyCodeObject *co);

// Reserve an index for the current thread into thread-local bytecode
// arrays
//
// Returns the reserved index or -1 on error.
extern int32_t _Py_ReserveTLBCIndex(PyInterpreterState *interp);

// Release the current thread's index into thread-local bytecode arrays
extern void _Py_ClearTLBCIndex(_PyThreadStateImpl *tstate);

// Free all TLBC copies not associated with live threads.
//
// Returns 0 on success or -1 on error.
extern int _Py_ClearUnusedTLBC(PyInterpreterState *interp);
#endif


typedef struct {
    int total;
    struct co_locals_counts {
        int total;
        struct {
            int total;
            int numposonly;
            int numposorkw;
            int numkwonly;
            int varargs;
            int varkwargs;
        } args;
        int numpure;
        struct {
            int total;
            // numargs does not contribute to locals.total.
            int numargs;
            int numothers;
        } cells;
        struct {
            int total;
            int numpure;
            int numcells;
        } hidden;
    } locals;
    int numfree;  // nonlocal
    struct co_unbound_counts {
        int total;
        struct {
            int total;
            int numglobal;
            int numbuiltin;
            int numunknown;
        } globals;
        int numattrs;
        int numunknown;
    } unbound;
} _PyCode_var_counts_t;

PyAPI_FUNC(void) _PyCode_GetVarCounts(
        PyCodeObject *,
        _PyCode_var_counts_t *);
PyAPI_FUNC(int) _PyCode_SetUnboundVarCounts(
        PyThreadState *,
        PyCodeObject *,
        _PyCode_var_counts_t *,
        PyObject *globalnames,
        PyObject *attrnames,
        PyObject *globalsns,
        PyObject *builtinsns);


/* "Stateless" code is a function or code object which does not rely on
 * external state or internal state.  It may rely on arguments and
 * builtins, but not globals or a closure.  Thus it does not rely
 * on __globals__ or __closure__, and a stateless function
 * is equivalent to its code object.
 *
 * Stateless code also does not keep any persistent state
 * of its own, so it can't have any executors, monitoring,
 * instrumentation, or "extras" (i.e. co_extra).
 *
 * Stateless code may create nested functions, including closures.
 * However, nested functions must themselves be stateless, except they
 * *can* close on the enclosing locals.
 *
 * Stateless code may return any value, including nested functions and closures.
 *
 * Stateless code that takes no arguments and doesn't return anything
 * may be treated like a script.
 *
 * We consider stateless code to be "portable" if it does not return
 * any object that holds a reference to any of the code's locals.  Thus
 * generators and coroutines are not portable.  Likewise a function
 * that returns a closure is not portable.  The concept of
 * portability is useful in cases where the code is run
 * in a different execution context than where
 * the return value will be used. */

PyAPI_FUNC(int) _PyCode_CheckNoInternalState(PyCodeObject *, const char **);
PyAPI_FUNC(int) _PyCode_CheckNoExternalState(
        PyCodeObject *,
        _PyCode_var_counts_t *,
        const char **);
PyAPI_FUNC(int) _PyCode_VerifyStateless(
        PyThreadState *,
        PyCodeObject *,
        PyObject *globalnames,
        PyObject *globalsns,
        PyObject *builtinsns);

PyAPI_FUNC(int) _PyCode_CheckPureFunction(PyCodeObject *, const char **);
PyAPI_FUNC(int) _PyCode_ReturnsOnlyNone(PyCodeObject *);

/* Create a comparable key used to compare constants taking in account the
 * object type. It is used to make sure types are not coerced (e.g., float and
 * complex) _and_ to distinguish 0.0 from -0.0 e.g. on IEEE platforms
 *
 * Return (type(obj), obj, ...): a tuple with variable size (at least 2 items)
 * depending on the type and the value. The type is the first item to not
 * compare bytes and str which can raise a BytesWarning exception. */
extern PyObject* _PyCode_ConstantKey(PyObject *obj);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
