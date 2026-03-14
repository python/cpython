#include "Python.h"

#include "pycore_backoff.h"
#include "pycore_call.h"
#include "pycore_cell.h"
#include "pycore_ceval.h"
#include "pycore_code.h"
#include "pycore_descrobject.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"
#include "pycore_floatobject.h"
#include "pycore_frame.h"
#include "pycore_function.h"
#include "pycore_genobject.h"
#include "pycore_import.h"
#include "pycore_interpframe.h"
#include "pycore_interpolation.h"
#include "pycore_intrinsics.h"
#include "pycore_lazyimportobject.h"
#include "pycore_jit.h"
#include "pycore_list.h"
#include "pycore_long.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_optimizer.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_range.h"
#include "pycore_setobject.h"
#include "pycore_sliceobject.h"
#include "pycore_stackref.h"
#include "pycore_template.h"
#include "pycore_tuple.h"
#include "pycore_unicodeobject.h"

#include "ceval_macros.h"

#include "jit.h"


// ── JIT-inlined object allocation helpers ──────────────────────────────
//
// These replace calls to PyFloat_FromDouble / _PyFloat_ExactDealloc with
// fully inlined fast paths.  The freelist pop/push is done directly,
// inlining _Py_NewReference (just sets refcnt=1) and skipping
// _PyReftracerTrack (virtually never active).
//
// Each inliner takes tstate as its first parameter.  Since the JIT's
// preserve_none calling convention keeps tstate in r15, this is a simple
// register read — no TLS call, no spills.  The #define redirects in
// template.c automatically capture tstate from the stencil function scope,
// so calling code doesn't need any special plumbing.
//
// To add a new type's freelist inlining:
//   1. Add _PyJIT_*_Alloc(tstate, ...) / _PyJIT_*_Dealloc(tstate, ...)
//      using _PyJIT_FREELIST_POP / _PyJIT_FREELIST_PUSH
//   2. Add #define redirects in template.c
//
// The slow path (malloc / PyObject_Free) falls back to the original
// C functions.

#include "pycore_freelist.h"
#include "pycore_object.h"

// Inline replacement for _PyObject_Init that avoids the external
// _Py_NewReference function call.  In non-debug builds, _Py_NewReference
// just sets ob_refcnt_full=1 — we do this directly.
static inline void
_PyJIT_Object_Init(PyObject *op, PyTypeObject *typeobj)
{
    Py_SET_TYPE(op, typeobj);
    _Py_INCREF_TYPE(typeobj);
    op->ob_refcnt_full = 1;  // Inline _Py_NewReference
}

// ── Generic freelist helpers ──────────────────────────────────────────
// These macros provide the core freelist pop/push logic shared by all
// type-specific inliners.  They inline _Py_NewReference (just sets
// refcnt=1) and skip _PyReftracerTrack.

// Pop an object from the freelist.  Returns the object with refcnt=1,
// or NULL if the freelist is empty.  NAME is the field name in
// struct _Py_freelists (e.g., floats, ints).
#define _PyJIT_FREELIST_POP(tstate, NAME) ({ \
    struct _Py_freelist *_fl = \
        &(tstate)->interp->object_state.freelists.NAME; \
    void *_obj = _fl->freelist; \
    if (_obj != NULL) { \
        _fl->freelist = *(void **)_obj; \
        _fl->size--; \
        OBJECT_STAT_INC(from_freelist); \
        ((PyObject *)_obj)->ob_refcnt_full = 1; \
    } \
    _obj; \
})

// Push an object onto the freelist.  Returns 1 if pushed, 0 if full.
// NAME is the field name, MAX is the maximum freelist size.
#define _PyJIT_FREELIST_PUSH(tstate, NAME, OP, MAX) ({ \
    struct _Py_freelist *_fl = \
        &(tstate)->interp->object_state.freelists.NAME; \
    int _ok = 0; \
    if (_fl->size >= 0 && _fl->size < (MAX)) { \
        *(void **)(OP) = _fl->freelist; \
        _fl->freelist = (OP); \
        _fl->size++; \
        OBJECT_STAT_INC(to_freelist); \
        _ok = 1; \
    } \
    _ok; \
})

// ── Float freelist inliners ───────────────────────────────────────────

static inline PyObject *
_PyJIT_FloatFromDouble(PyThreadState *tstate, double fval)
{
    PyFloatObject *op = (PyFloatObject *)_PyJIT_FREELIST_POP(tstate, floats);
    if (op != NULL) {
        op->ob_fval = fval;
        return (PyObject *)op;
    }
    return PyFloat_FromDouble(fval);
}

static inline void
_PyJIT_FloatDealloc(PyThreadState *tstate, PyObject *op)
{
    if (!_PyJIT_FREELIST_PUSH(tstate, floats, op, Py_floats_MAXFREELIST)) {
        PyObject_Free(op);
    }
}

// ── Int (long) freelist inliners ──────────────────────────────────────

#include "pycore_long.h"

static inline void
_PyJIT_LongDealloc(PyThreadState *tstate, PyObject *op)
{
    PyLongObject *l = (PyLongObject *)op;
    // Small int recovery: re-immortalize.  This is a cold path — small
    // ints have immortal refcnt and shouldn't normally reach dealloc.
    if (l->long_value.lv_tag & IMMORTALITY_BIT_MASK) {
        _Py_SetImmortal(op);
        return;
    }
    // Compact (single-digit) int: push to freelist.
    if (_PyLong_IsCompact(l)) {
        if (!_PyJIT_FREELIST_PUSH(tstate, ints, op, Py_ints_MAXFREELIST)) {
            PyObject_Free(op);
        }
        return;
    }
    // Non-compact (multi-digit) int: free directly.
    PyObject_Free(op);
}

// Inline version of medium_from_stwodigits: converts an arithmetic result
// back to a PyLongObject.  Returns small int singleton for small values,
// pops from freelist for medium values, falls back to NULL if overflow.
static inline _PyStackRef
_PyJIT_MediumFromStwodigits(PyThreadState *tstate, stwodigits x)
{
    // Small int: return borrowed reference to cached singleton.
    if (-_PY_NSMALLNEGINTS <= x && x < _PY_NSMALLPOSINTS) {
        PyObject *small = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS + (sdigit)x];
        return PyStackRef_FromPyObjectBorrow(small);
    }
    // Check if result fits in a single digit (medium int).
    twodigits x_plus_mask = ((twodigits)x) + PyLong_MASK;
    if (x_plus_mask >= ((twodigits)PyLong_MASK) + PyLong_BASE) {
        return PyStackRef_NULL;  // overflow — caller will deoptimize
    }
    // Pop from freelist or allocate.
    PyLongObject *v = (PyLongObject *)_PyJIT_FREELIST_POP(tstate, ints);
    if (v == NULL) {
        v = PyObject_Malloc(sizeof(PyLongObject));
        if (v == NULL) {
            return PyStackRef_NULL;
        }
        _PyJIT_Object_Init((PyObject *)v, &PyLong_Type);
    }
    digit abs_x = x < 0 ? (digit)(-x) : (digit)x;
    _PyLong_SetSignAndDigitCount(v, x < 0 ? -1 : 1, 1);
    v->long_value.ob_digit[0] = abs_x;
    return PyStackRef_FromPyObjectStealMortal((PyObject *)v);
}

static inline _PyStackRef
_PyJIT_CompactLong_Add(PyThreadState *tstate,
                       PyLongObject *left, PyLongObject *right)
{
    stwodigits v = (stwodigits)_PyLong_CompactValue(left)
                 + (stwodigits)_PyLong_CompactValue(right);
    return _PyJIT_MediumFromStwodigits(tstate, v);
}

static inline _PyStackRef
_PyJIT_CompactLong_Subtract(PyThreadState *tstate,
                            PyLongObject *left, PyLongObject *right)
{
    stwodigits v = (stwodigits)_PyLong_CompactValue(left)
                 - (stwodigits)_PyLong_CompactValue(right);
    return _PyJIT_MediumFromStwodigits(tstate, v);
}

static inline _PyStackRef
_PyJIT_CompactLong_Multiply(PyThreadState *tstate,
                            PyLongObject *left, PyLongObject *right)
{
    stwodigits v = (stwodigits)_PyLong_CompactValue(left)
                 * (stwodigits)_PyLong_CompactValue(right);
    return _PyJIT_MediumFromStwodigits(tstate, v);
}

// Floor division for compact longs.  Caller must ensure right != 0
// (the bytecode guards with DEOPT_IF).
// Python floor division rounds toward negative infinity, unlike C which
// truncates toward zero.  We correct for this when the remainder is
// nonzero and operand signs differ.
static inline _PyStackRef
_PyJIT_CompactLong_FloorDivide(PyThreadState *tstate,
                               PyLongObject *left, PyLongObject *right)
{
    stwodigits a = (stwodigits)_PyLong_CompactValue(left);
    stwodigits b = (stwodigits)_PyLong_CompactValue(right);
    assert(b != 0);
    stwodigits div = a / b;
    stwodigits rem = a % b;
    // Adjust for floor semantics: if remainder nonzero and signs differ
    if (rem != 0 && ((rem ^ b) < 0)) {
        div -= 1;
    }
    return _PyJIT_MediumFromStwodigits(tstate, div);
}

// Modulo for compact longs.  Caller must ensure right != 0.
static inline _PyStackRef
_PyJIT_CompactLong_Modulo(PyThreadState *tstate,
                          PyLongObject *left, PyLongObject *right)
{
    stwodigits a = (stwodigits)_PyLong_CompactValue(left);
    stwodigits b = (stwodigits)_PyLong_CompactValue(right);
    assert(b != 0);
    stwodigits mod = a % b;
    if (mod != 0 && ((mod ^ b) < 0)) {
        mod += b;
    }
    return _PyJIT_MediumFromStwodigits(tstate, mod);
}

// ── Fast dealloc inliner (replaces generic _Py_Dealloc) ──────────────
// _Py_Dealloc goes through reftracer check, recursion limit check,
// and an indirect call to tp_dealloc.  For the vast majority of JIT-hot
// object types (float, compact int) we can inline the freelist push
// directly, saving TWO function calls and the tracer/recursion overhead.
// For unknown types we fall back to a direct tp_dealloc call (still
// faster than _Py_Dealloc because we skip the tracer + recursion checks).

static inline void
_PyJIT_FastDealloc(PyThreadState *tstate, PyObject *op)
{
    PyTypeObject *type = Py_TYPE(op);
    if (type == &PyFloat_Type) {
        _PyJIT_FloatDealloc(tstate, op);
    } else if (type == &PyLong_Type) {
        _PyJIT_LongDealloc(tstate, op);
    } else {
        // Direct tp_dealloc call — skips reftracer and recursion check.
        type->tp_dealloc(op);
    }
}

// Inline replacement for PyLong_FromLong.  Most range() iterators produce
// values in the small int range (-5..1024) which are immortal singletons
// requiring no allocation at all.  Medium values (single digit) go through
// the freelist.  Only large values fall back to the full PyLong_FromLong.
static inline PyObject *
_PyJIT_LongFromLong(PyThreadState *tstate, long ival)
{
    // Small int: return cached singleton (no allocation, no refcount).
    if (ival >= -_PY_NSMALLNEGINTS && ival < _PY_NSMALLPOSINTS) {
        return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS + (int)ival];
    }
    // Medium int (fits in single digit): use freelist.
    if (-(long)PyLong_MASK <= ival && ival <= (long)PyLong_MASK) {
        PyLongObject *v = (PyLongObject *)_PyJIT_FREELIST_POP(tstate, ints);
        if (v == NULL) {
            v = PyObject_Malloc(sizeof(PyLongObject));
            if (v == NULL) {
                PyErr_NoMemory();
                return NULL;
            }
            _PyJIT_Object_Init((PyObject *)v, &PyLong_Type);
        }
        digit abs_x = ival < 0 ? (digit)(-ival) : (digit)ival;
        _PyLong_SetSignAndDigitCount(v, ival < 0 ? -1 : 1, 1);
        v->long_value.ob_digit[0] = abs_x;
        return (PyObject *)v;
    }
    // Large int: fall back to full PyLong_FromLong.
    return PyLong_FromLong(ival);
}

// Replace PyFloat_FromDouble with JIT-inlined version that avoids
// function call overhead for freelist pop and _Py_NewReference.
// tstate is captured from the stencil function scope (r15 in preserve_none).
#define PyFloat_FromDouble(val) _PyJIT_FloatFromDouble(tstate, (val))

// Override PyStackRef_CLOSE_SPECIALIZED to call the destructor directly
// (as a macro expansion) instead of through a function pointer.
// This allows _PyFloat_ExactDealloc to be a macro that inlines the
// freelist push directly.
// Note: We skip _PyReftracerTrack in JIT stencils for performance.
// The reftracer is a debugging/profiling tool that is virtually never
// active.  Our inlined _PyJIT_*Dealloc helpers already skip it, so
// this makes the non-inlined path consistent.
#undef PyStackRef_CLOSE_SPECIALIZED
#define PyStackRef_CLOSE_SPECIALIZED(REF, DESTRUCT) do { \
    _PyStackRef _jit_ref = (REF); \
    if (PyStackRef_RefcountOnObject(_jit_ref)) { \
        PyObject *_jit_obj = BITS_TO_PTR(_jit_ref); \
        _Py_DECREF_STAT_INC(); \
        if (--_jit_obj->ob_refcnt == 0) { \
            DESTRUCT(_jit_obj); \
        } \
    } \
} while(0)

// Replace _PyFloat_ExactDealloc with inline freelist push.
// tstate is captured from the stencil function scope.
#undef _PyFloat_ExactDealloc
#define _PyFloat_ExactDealloc(op) _PyJIT_FloatDealloc(tstate, (op))

// Replace _PyLong_ExactDealloc with inline version that avoids
// function call overhead for compact int freelist push.
#undef _PyLong_ExactDealloc
#define _PyLong_ExactDealloc(op) _PyJIT_LongDealloc(tstate, (op))

// Replace compact long arithmetic with inline versions that avoid
// function call overhead and inline the freelist pop.
#define _PyCompactLong_Add(a, b) _PyJIT_CompactLong_Add(tstate, (a), (b))
#define _PyCompactLong_Subtract(a, b) _PyJIT_CompactLong_Subtract(tstate, (a), (b))
#define _PyCompactLong_Multiply(a, b) _PyJIT_CompactLong_Multiply(tstate, (a), (b))
#define _PyCompactLong_FloorDivide(a, b) _PyJIT_CompactLong_FloorDivide(tstate, (a), (b))
#define _PyCompactLong_Modulo(a, b) _PyJIT_CompactLong_Modulo(tstate, (a), (b))
#define PyLong_FromLong(val) _PyJIT_LongFromLong(tstate, (val))

// ── Fast _Py_Dealloc replacement ────────────────────────────────────
// _Py_Dealloc goes through reftracer check, recursion limit check,
// and an indirect call to tp_dealloc.  For the vast majority of JIT-hot
// object types (float, compact int) we can inline the freelist push
// directly, saving TWO function calls and the tracer/recursion overhead.
// For unknown types we fall back to a direct tp_dealloc call (still
// faster than _Py_Dealloc because we skip the tracer + recursion checks).
//
// We must override at the PyStackRef_CLOSE level because:
// - Py_DECREF / Py_DECREF_MORTAL are inline functions defined in headers
//   BEFORE our overrides, so their bodies already contain _Py_Dealloc
// - PyStackRef_CLOSE also uses Py_DECREF_MORTAL via the header's inline body
// - Only macro-level overrides at the caller level take effect

// Override PyStackRef_CLOSE — the main path for all POP_TOP/CLOSE operations
#undef PyStackRef_CLOSE
static inline Py_ALWAYS_INLINE void
_PyJIT_PyStackRef_CLOSE(PyThreadState *tstate, _PyStackRef ref)
{
    if (PyStackRef_RefcountOnObject(ref)) {
        PyObject *op = BITS_TO_PTR(ref);
        _Py_DECREF_STAT_INC();
        if (--op->ob_refcnt == 0) {
            _PyJIT_FastDealloc(tstate, op);
        }
    }
}
#define PyStackRef_CLOSE(REF) _PyJIT_PyStackRef_CLOSE(tstate, (REF))

// Override PyStackRef_XCLOSE — used by POP_TOP (which calls XCLOSE, not CLOSE)
#undef PyStackRef_XCLOSE
static inline Py_ALWAYS_INLINE void
_PyJIT_PyStackRef_XCLOSE(PyThreadState *tstate, _PyStackRef ref)
{
    if (PyStackRef_RefcountOnObject(ref)) {
        PyObject *op = BITS_TO_PTR(ref);
        _Py_DECREF_STAT_INC();
        if (--op->ob_refcnt == 0) {
            _PyJIT_FastDealloc(tstate, op);
        }
    }
}
#define PyStackRef_XCLOSE(REF) _PyJIT_PyStackRef_XCLOSE(tstate, (REF))

// Override PyStackRef_CLEAR — macro that uses XCLOSE internally
#undef PyStackRef_CLEAR
#define PyStackRef_CLEAR(REF) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(REF); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        *_tmp_op_ptr = PyStackRef_NULL; \
        _PyJIT_PyStackRef_XCLOSE(tstate, _tmp_old_op); \
    } while (0)

// Also override Py_DECREF for direct calls outside PyStackRef_CLOSE
#undef Py_DECREF
static inline Py_ALWAYS_INLINE void
_PyJIT_Py_DECREF(PyThreadState *tstate, PyObject *op)
{
    if (_Py_IsImmortal(op)) {
        _Py_DECREF_IMMORTAL_STAT_INC();
        return;
    }
    _Py_DECREF_STAT_INC();
    if (--op->ob_refcnt == 0) {
        _PyJIT_FastDealloc(tstate, op);
    }
}
#define Py_DECREF(op) _PyJIT_Py_DECREF(tstate, _PyObject_CAST(op))

// Override Py_XDECREF — also an inline function with baked-in Py_DECREF
#undef Py_XDECREF
#define Py_XDECREF(op) \
    do { \
        PyObject *_xdecref_tmp = _PyObject_CAST(op); \
        if (_xdecref_tmp != _Py_NULL) { \
            _PyJIT_Py_DECREF(tstate, _xdecref_tmp); \
        } \
    } while (0)


#undef CURRENT_OPERAND0_64
#define CURRENT_OPERAND0_64() (_operand0_64)

#undef CURRENT_OPERAND1_64
#define CURRENT_OPERAND1_64() (_operand1_64)


#undef CURRENT_OPARG
#undef CURRENT_OPERAND0_16
#undef CURRENT_OPERAND0_32
#undef CURRENT_OPERAND1_16
#undef CURRENT_OPERAND1_32

#define CURRENT_OPARG() (_oparg)
#define CURRENT_OPERAND0_32() (_operand0_64)
#define CURRENT_OPERAND0_16() (_operand0_64)
#define CURRENT_OPERAND1_32() (_operand1_64)
#define CURRENT_OPERAND1_16() (_operand1_64)


#undef CURRENT_TARGET
#define CURRENT_TARGET() (_target)

#undef TIER2_TO_TIER2
#define TIER2_TO_TIER2(EXECUTOR)                                           \
do {                                                                       \
    OPT_STAT_INC(traces_executed);                                         \
    _PyExecutorObject *_executor = (EXECUTOR);                             \
    jit_func_preserve_none jitted = _executor->jit_code;                   \
    __attribute__((musttail)) return jitted(_executor, frame, stack_pointer, tstate,  \
    _tos_cache0, _tos_cache1, _tos_cache2); \
} while (0)

#undef GOTO_TIER_ONE_SETUP
#define GOTO_TIER_ONE_SETUP \
    tstate->current_executor = NULL;                              \
    _PyFrame_SetStackPointer(frame, stack_pointer);

#undef LOAD_IP
#define LOAD_IP(UNUSED) \
    do {                \
    } while (0)

#undef LLTRACE_RESUME_FRAME
#ifdef Py_DEBUG
#define LLTRACE_RESUME_FRAME() (frame->lltrace = 0)
#else
#define LLTRACE_RESUME_FRAME() do {} while (0)
#endif

#define PATCH_JUMP(ALIAS)                                                 \
do {                                                                      \
    DECLARE_TARGET(ALIAS);                                                \
    __attribute__((musttail)) return ALIAS(current_executor, frame, stack_pointer, tstate,  \
    _tos_cache0, _tos_cache1, _tos_cache2); \
} while (0)

#undef JUMP_TO_JUMP_TARGET
#define JUMP_TO_JUMP_TARGET() PATCH_JUMP(_JIT_JUMP_TARGET)

#undef JUMP_TO_ERROR
#define JUMP_TO_ERROR() PATCH_JUMP(_JIT_ERROR_TARGET)

#define TIER_TWO 2

#ifdef Py_DEBUG
#define ASSERT_WITHIN_STACK_BOUNDS(F, L) _Py_assert_within_stack_bounds(frame, stack_pointer, (F), (L))
#else
#define ASSERT_WITHIN_STACK_BOUNDS(F, L) (void)0
#endif

__attribute__((preserve_none)) _Py_CODEUNIT *
_JIT_ENTRY(
    _PyExecutorObject *executor, _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate,
    _PyStackRef _tos_cache0, _PyStackRef _tos_cache1, _PyStackRef _tos_cache2
) {
    // Locals that the instruction implementations expect to exist:
    _PyExecutorObject *current_executor = executor;
    int oparg;
    int uopcode = _JIT_OPCODE;
    _Py_CODEUNIT *next_instr;
    volatile unsigned char _jit_stack_pad[1];
    __asm__ volatile("nop # @@JIT_FRAME_ANCHOR@@" : : "m"(_jit_stack_pad) : "memory");
    // Other stuff we need handy:
#if SIZEOF_VOID_P == 8
    PATCH_VALUE(uint64_t, _operand0_64, _JIT_OPERAND0)
    PATCH_VALUE(uint64_t, _operand1_64, _JIT_OPERAND1)
#else
    assert(SIZEOF_VOID_P == 4);
    PATCH_VALUE(uint32_t, _operand0_hi, _JIT_OPERAND0_HI)
    PATCH_VALUE(uint32_t, _operand0_lo, _JIT_OPERAND0_LO)
    uint64_t _operand0_64 = ((uint64_t)_operand0_hi << 32) | _operand0_lo;
    PATCH_VALUE(uint32_t, _operand1_hi, _JIT_OPERAND1_HI)
    PATCH_VALUE(uint32_t, _operand1_lo, _JIT_OPERAND1_LO)
    uint64_t _operand1_64 = ((uint64_t)_operand1_hi << 32) | _operand1_lo;
#endif
    PATCH_VALUE(uint16_t, _oparg, _JIT_OPARG)
    PATCH_VALUE(uint32_t, _target, _JIT_TARGET)
    OPT_STAT_INC(uops_executed);
    UOP_STAT_INC(uopcode, execution_count);
    switch (uopcode) {
        // The actual instruction definition gets inserted here:
        CASE
        default:
            Py_UNREACHABLE();
    }
    PATCH_JUMP(_JIT_CONTINUE);
}
