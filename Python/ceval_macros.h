// Macros and other things needed by ceval.c, and bytecodes.c

/* Computed GOTOs, or
       the-optimization-commonly-but-improperly-known-as-"threaded code"
   using gcc's labels-as-values extension
   (http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html).

   The traditional bytecode evaluation loop uses a "switch" statement, which
   decent compilers will optimize as a single indirect branch instruction
   combined with a lookup table of jump addresses. However, since the
   indirect jump instruction is shared by all opcodes, the CPU will have a
   hard time making the right prediction for where to jump next (actually,
   it will be always wrong except in the uncommon case of a sequence of
   several identical opcodes).

   "Threaded code" in contrast, uses an explicit jump table and an explicit
   indirect jump instruction at the end of each opcode. Since the jump
   instruction is at a different address for each opcode, the CPU will make a
   separate prediction for each of these instructions, which is equivalent to
   predicting the second opcode of each opcode pair. These predictions have
   a much better chance to turn out valid, especially in small bytecode loops.

   A mispredicted branch on a modern CPU flushes the whole pipeline and
   can cost several CPU cycles (depending on the pipeline depth),
   and potentially many more instructions (depending on the pipeline width).
   A correctly predicted branch, however, is nearly free.

   At the time of this writing, the "threaded code" version is up to 15-20%
   faster than the normal "switch" version, depending on the compiler and the
   CPU architecture.

   NOTE: care must be taken that the compiler doesn't try to "optimize" the
   indirect jumps by sharing them between all opcodes. Such optimizations
   can be disabled on gcc by using the -fno-gcse flag (or possibly
   -fno-crossjumping).
*/

/* Use macros rather than inline functions, to make it as clear as possible
 * to the C compiler that the tracing check is a simple test then branch.
 * We want to be sure that the compiler knows this before it generates
 * the CFG.
 */

#ifdef WITH_DTRACE
#define OR_DTRACE_LINE | (PyDTrace_LINE_ENABLED() ? 255 : 0)
#else
#define OR_DTRACE_LINE
#endif

#ifdef HAVE_COMPUTED_GOTOS
    #ifndef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 1
    #endif
#else
    #if defined(USE_COMPUTED_GOTOS) && USE_COMPUTED_GOTOS
    #error "Computed gotos are not supported on this compiler."
    #endif
    #undef USE_COMPUTED_GOTOS
    #define USE_COMPUTED_GOTOS 0
#endif

#ifdef Py_STATS
#define INSTRUCTION_STATS(op) \
    do { \
        PyStats *s = _PyStats_GET(); \
        OPCODE_EXE_INC(op); \
        if (s) s->opcode_stats[lastopcode].pair_count[op]++; \
        lastopcode = op; \
    } while (0)
#else
#define INSTRUCTION_STATS(op) ((void)0)
#endif

#ifdef Py_STATS
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _Py_CODEUNIT *next_instr, const void *instruction_funcptr_table, int oparg, int lastopcode
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, instruction_funcptr_table, oparg, lastopcode
#else
#   define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate, _Py_CODEUNIT *next_instr, const void *instruction_funcptr_table, int oparg
#   define TAIL_CALL_ARGS frame, stack_pointer, tstate, next_instr, instruction_funcptr_table, oparg
#endif

#if _Py_TAIL_CALL_INTERP
#   if defined(__clang__) || defined(__GNUC__)
#       if !_Py__has_attribute(preserve_none) || !_Py__has_attribute(musttail)
#           error "This compiler does not have support for efficient tail calling."
#       endif
#   elif defined(_MSC_VER) && (_MSC_VER < 1950)
#       error "You need at least VS 2026 / PlatformToolset v145 for tail calling."
#   endif

    // Note: [[clang::musttail]] works for GCC 15, but not __attribute__((musttail)) at the moment.
#   define Py_MUSTTAIL [[clang::musttail]]
#   define Py_PRESERVE_NONE_CC __attribute__((preserve_none))
    Py_PRESERVE_NONE_CC typedef PyObject* (*py_tail_call_funcptr)(TAIL_CALL_PARAMS);

#   define DISPATCH_TABLE_VAR instruction_funcptr_table
#   define DISPATCH_TABLE instruction_funcptr_handler_table
#   define TRACING_DISPATCH_TABLE instruction_funcptr_tracing_table
#   define TARGET(op) Py_PRESERVE_NONE_CC PyObject *_TAIL_CALL_##op(TAIL_CALL_PARAMS)

#   define DISPATCH_GOTO() \
        do { \
            Py_MUSTTAIL return (((py_tail_call_funcptr *)instruction_funcptr_table)[opcode])(TAIL_CALL_ARGS); \
        } while (0)
#   define DISPATCH_GOTO_NON_TRACING() \
        do { \
            Py_MUSTTAIL return (((py_tail_call_funcptr *)DISPATCH_TABLE)[opcode])(TAIL_CALL_ARGS); \
        } while (0)
#   define JUMP_TO_LABEL(name) \
        do { \
            Py_MUSTTAIL return (_TAIL_CALL_##name)(TAIL_CALL_ARGS); \
        } while (0)
#   ifdef Py_STATS
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, instruction_funcptr_table, oparg, lastopcode); \
            } while (0)
#   else
#       define JUMP_TO_PREDICTED(name) \
            do { \
                Py_MUSTTAIL return (_TAIL_CALL_##name)(frame, stack_pointer, tstate, this_instr, instruction_funcptr_table, oparg); \
            } while (0)
#   endif
#    define LABEL(name) TARGET(name)
#elif USE_COMPUTED_GOTOS
#  define DISPATCH_TABLE_VAR opcode_targets
#  define DISPATCH_TABLE opcode_targets_table
#  define TRACING_DISPATCH_TABLE opcode_tracing_targets_table
#  define TARGET(op) TARGET_##op:
#  define DISPATCH_GOTO() goto *opcode_targets[opcode]
#  define DISPATCH_GOTO_NON_TRACING() goto *DISPATCH_TABLE[opcode];
#  define JUMP_TO_LABEL(name) goto name;
#  define JUMP_TO_PREDICTED(name) goto PREDICTED_##name;
#  define LABEL(name) name:
#else
#  define TARGET(op) case op: TARGET_##op:
#  define DISPATCH_GOTO() dispatch_code = opcode | tracing_mode ; goto dispatch_opcode
#  define DISPATCH_GOTO_NON_TRACING() dispatch_code = opcode; goto dispatch_opcode
#  define JUMP_TO_LABEL(name) goto name;
#  define JUMP_TO_PREDICTED(name) goto PREDICTED_##name;
#  define LABEL(name) name:
#endif

#if (_Py_TAIL_CALL_INTERP || USE_COMPUTED_GOTOS) && _Py_TIER2
#  define IS_JIT_TRACING() (DISPATCH_TABLE_VAR == TRACING_DISPATCH_TABLE)
#  define ENTER_TRACING() \
    DISPATCH_TABLE_VAR = TRACING_DISPATCH_TABLE;
#  define LEAVE_TRACING() \
    DISPATCH_TABLE_VAR = DISPATCH_TABLE;
#else
#  define IS_JIT_TRACING() (tracing_mode != 0)
#  define ENTER_TRACING() tracing_mode = 255
#  define LEAVE_TRACING() tracing_mode = 0
#endif

/* PRE_DISPATCH_GOTO() does lltrace if enabled. Normally a no-op */
#ifdef Py_DEBUG
#define PRE_DISPATCH_GOTO() if (frame->lltrace >= 5) { \
    lltrace_instruction(frame, stack_pointer, next_instr, opcode, oparg); }
#else
#define PRE_DISPATCH_GOTO() ((void)0)
#endif

#ifdef Py_DEBUG
#define LLTRACE_RESUME_FRAME() \
do { \
    _PyFrame_SetStackPointer(frame, stack_pointer); \
    int lltrace = maybe_lltrace_resume_frame(frame, GLOBALS()); \
    stack_pointer = _PyFrame_GetStackPointer(frame); \
    frame->lltrace = lltrace; \
} while (0)
#else
#define LLTRACE_RESUME_FRAME() ((void)0)
#endif

#ifdef Py_GIL_DISABLED
#define QSBR_QUIESCENT_STATE(tstate) _Py_qsbr_quiescent_state(((_PyThreadStateImpl *)tstate)->qsbr)
#else
#define QSBR_QUIESCENT_STATE(tstate)
#endif


/* Do interpreter dispatch accounting for tracing and instrumentation */
#define DISPATCH() \
    { \
        assert(frame->stackpointer == NULL); \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_NON_TRACING() \
    { \
        assert(frame->stackpointer == NULL); \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO_NON_TRACING(); \
    }

#define DISPATCH_SAME_OPARG() \
    { \
        opcode = next_instr->op.code; \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO_NON_TRACING(); \
    }

#define DISPATCH_INLINED(NEW_FRAME)                     \
    do {                                                \
        assert(tstate->interp->eval_frame == NULL);     \
        _PyFrame_SetStackPointer(frame, stack_pointer); \
        assert((NEW_FRAME)->previous == frame);         \
        frame = tstate->current_frame = (NEW_FRAME);     \
        CALL_STAT_INC(inlined_py_calls);                \
        JUMP_TO_LABEL(start_frame);                      \
    } while (0)

/* Tuple access macros */

#ifndef Py_DEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((v), (i))
#else
static inline PyObject *
GETITEM(PyObject *v, Py_ssize_t i) {
    assert(PyTuple_Check(v));
    assert(i >= 0);
    assert(i < PyTuple_GET_SIZE(v));
    return PyTuple_GET_ITEM(v, i);
}
#endif

/* Code access macros */

/* The integer overflow is checked by an assertion below. */
#define INSTR_OFFSET() ((int)(next_instr - _PyFrame_GetBytecode(frame)))
#define NEXTOPARG()  do { \
        _Py_CODEUNIT word  = {.cache = FT_ATOMIC_LOAD_UINT16_RELAXED(*(uint16_t*)next_instr)}; \
        opcode = word.op.code; \
        oparg = word.op.arg; \
    } while (0)

/* JUMPBY makes the generator identify the instruction as a jump. SKIP_OVER is
 * for advancing to the next instruction, taking into account cache entries
 * and skipped instructions.
 */
#define JUMPBY(x)       (next_instr += (x))
#define SKIP_OVER(x)    (next_instr += (x))

#define STACK_LEVEL()     ((int)(stack_pointer - _PyFrame_Stackbase(frame)))
#define STACK_SIZE()      (_PyFrame_GetCode(frame)->co_stacksize)

#define WITHIN_STACK_BOUNDS() \
   (frame->owner == FRAME_OWNED_BY_INTERPRETER || (STACK_LEVEL() >= 0 && STACK_LEVEL() <= STACK_SIZE()))

/* Data access macros */
#define FRAME_CO_CONSTS (_PyFrame_GetCode(frame)->co_consts)
#define FRAME_CO_NAMES  (_PyFrame_GetCode(frame)->co_names)

/* Local variable macros */

#define LOCALS_ARRAY    (frame->localsplus)
#define GETLOCAL(i)     (frame->localsplus[i])


#ifdef Py_STATS
#define UPDATE_MISS_STATS(INSTNAME)                              \
    do {                                                         \
        STAT_INC(opcode, miss);                                  \
        STAT_INC((INSTNAME), miss);                              \
        /* The counter is always the first cache entry: */       \
        if (ADAPTIVE_COUNTER_TRIGGERS(next_instr->cache)) {       \
            STAT_INC((INSTNAME), deopt);                         \
        }                                                        \
    } while (0)
#else
#define UPDATE_MISS_STATS(INSTNAME) ((void)0)
#endif


// Try to lock an object in the free threading build, if it's not already
// locked. Use with a DEOPT_IF() to deopt if the object is already locked.
// These are no-ops in the default GIL build. The general pattern is:
//
// DEOPT_IF(!LOCK_OBJECT(op));
// if (/* condition fails */) {
//     UNLOCK_OBJECT(op);
//     DEOPT_IF(true);
//  }
//  ...
//  UNLOCK_OBJECT(op);
//
// NOTE: The object must be unlocked on every exit code path and you should
// avoid any potentially escaping calls (like PyStackRef_CLOSE) while the
// object is locked.
#ifdef Py_GIL_DISABLED
#  define LOCK_OBJECT(op) PyMutex_LockFast(&(_PyObject_CAST(op))->ob_mutex)
#  define UNLOCK_OBJECT(op) PyMutex_Unlock(&(_PyObject_CAST(op))->ob_mutex)
#else
#  define LOCK_OBJECT(op) (1)
#  define UNLOCK_OBJECT(op) ((void)0)
#endif

#define GLOBALS() frame->f_globals
#define BUILTINS() frame->f_builtins
#define LOCALS() frame->f_locals
#define CONSTS() _PyFrame_GetCode(frame)->co_consts
#define NAMES() _PyFrame_GetCode(frame)->co_names

#define DTRACE_FUNCTION_ENTRY()  \
    if (PyDTrace_FUNCTION_ENTRY_ENABLED()) { \
        dtrace_function_entry(frame); \
    }

/* This takes a uint16_t instead of a _Py_BackoffCounter,
 * because it is used directly on the cache entry in generated code,
 * which is always an integral type. */
// Force re-specialization when tracing a side exit to get good side exits.
#define ADAPTIVE_COUNTER_TRIGGERS(COUNTER) \
    backoff_counter_triggers(forge_backoff_counter((COUNTER)))

#define ADVANCE_ADAPTIVE_COUNTER(COUNTER) \
    do { \
        (COUNTER) = advance_backoff_counter((COUNTER)); \
    } while (0);

#define PAUSE_ADAPTIVE_COUNTER(COUNTER) \
    do { \
        (COUNTER) = pause_backoff_counter((COUNTER)); \
    } while (0);

#ifdef ENABLE_SPECIALIZATION_FT
/* Multiple threads may execute these concurrently if thread-local bytecode is
 * disabled and they all execute the main copy of the bytecode. Specialization
 * is disabled in that case so the value is unused, but the RMW cycle should be
 * free of data races.
 */
#define RECORD_BRANCH_TAKEN(bitset, flag) \
    FT_ATOMIC_STORE_UINT16_RELAXED(       \
        bitset, (FT_ATOMIC_LOAD_UINT16_RELAXED(bitset) << 1) | (flag))
#else
#define RECORD_BRANCH_TAKEN(bitset, flag)
#endif

#define UNBOUNDLOCAL_ERROR_MSG \
    "cannot access local variable '%s' where it is not associated with a value"
#define UNBOUNDFREE_ERROR_MSG \
    "cannot access free variable '%s' where it is not associated with a value" \
    " in enclosing scope"
#define NAME_ERROR_MSG "name '%.200s' is not defined"

// If a trace function sets a new f_lineno and
// *then* raises, we use the destination when searching
// for an exception handler, displaying the traceback, and so on
#define INSTRUMENTED_JUMP(src, dest, event) \
do { \
    if (tstate->tracing) {\
        next_instr = dest; \
    } else { \
        _PyFrame_SetStackPointer(frame, stack_pointer); \
        next_instr = _Py_call_instrumentation_jump(this_instr, tstate, event, frame, src, dest); \
        stack_pointer = _PyFrame_GetStackPointer(frame); \
        if (next_instr == NULL) { \
            next_instr = (dest)+1; \
            JUMP_TO_LABEL(error); \
        } \
    } \
} while (0);


static inline int _Py_EnterRecursivePy(PyThreadState *tstate) {
    return (tstate->py_recursion_remaining-- <= 0) &&
        _Py_CheckRecursiveCallPy(tstate);
}

static inline void _Py_LeaveRecursiveCallPy(PyThreadState *tstate)  {
    tstate->py_recursion_remaining++;
}

/* Implementation of "macros" that modify the instruction pointer,
 * stack pointer, or frame pointer.
 * These need to treated differently by tier 1 and 2.
 * The Tier 1 version is here; Tier 2 is inlined in ceval.c. */

#define LOAD_IP(OFFSET) do { \
        next_instr = frame->instr_ptr + (OFFSET); \
    } while (0)

/* There's no STORE_IP(), it's inlined by the code generator. */

#define LOAD_SP() \
stack_pointer = _PyFrame_GetStackPointer(frame)

#define SAVE_SP() \
_PyFrame_SetStackPointer(frame, stack_pointer)

/* Tier-switching macros. */

#define TIER1_TO_TIER2(EXECUTOR)                        \
do {                                                   \
    OPT_STAT_INC(traces_executed);                     \
    next_instr = _Py_jit_entry((EXECUTOR), frame, stack_pointer, tstate); \
    frame = tstate->current_frame;                     \
    stack_pointer = _PyFrame_GetStackPointer(frame);   \
    int keep_tracing_bit = (uintptr_t)next_instr & 1;   \
    next_instr = (_Py_CODEUNIT *)(((uintptr_t)next_instr) & (~1)); \
    if (next_instr == NULL) {                          \
        /* gh-140104: The exception handler expects frame->instr_ptr
            to after this_instr, not this_instr! */ \
        next_instr = frame->instr_ptr + 1;                 \
        JUMP_TO_LABEL(error);                          \
    }                                                  \
    if (keep_tracing_bit) { \
        assert(((_PyThreadStateImpl *)tstate)->jit_tracer_state.prev_state.code_curr_size == 2); \
        ENTER_TRACING(); \
        DISPATCH_NON_TRACING(); \
    } \
    DISPATCH();                                        \
} while (0)

#define TIER2_TO_TIER2(EXECUTOR) \
do {                                                   \
    OPT_STAT_INC(traces_executed);                     \
    current_executor = (EXECUTOR);                     \
    goto tier2_start;                                  \
} while (0)

#define GOTO_TIER_ONE_SETUP \
    tstate->current_executor = NULL;                              \
    OPT_HIST(trace_uop_execution_counter, trace_run_length_hist); \
    _PyFrame_SetStackPointer(frame, stack_pointer);

#define GOTO_TIER_ONE(TARGET) \
    do \
    { \
        GOTO_TIER_ONE_SETUP \
        return (_Py_CODEUNIT *)(TARGET); \
    } while (0)

#define GOTO_TIER_ONE_CONTINUE_TRACING(TARGET) \
    do \
    { \
        GOTO_TIER_ONE_SETUP \
        return (_Py_CODEUNIT *)(((uintptr_t)(TARGET))| 1); \
    } while (0)

#define CURRENT_OPARG()    (next_uop[-1].oparg)
#define CURRENT_OPERAND0() (next_uop[-1].operand0)
#define CURRENT_OPERAND1() (next_uop[-1].operand1)
#define CURRENT_TARGET()   (next_uop[-1].target)

#define JUMP_TO_JUMP_TARGET() goto jump_to_jump_target
#define JUMP_TO_ERROR() goto jump_to_error_target

/* Stackref macros */

/* How much scratch space to give stackref to PyObject* conversion. */
#define MAX_STACKREF_SCRATCH 10

#define STACKREFS_TO_PYOBJECTS(ARGS, ARG_COUNT, NAME) \
    /* +1 because vectorcall might use -1 to write self */ \
    PyObject *NAME##_temp[MAX_STACKREF_SCRATCH+1]; \
    PyObject **NAME = _PyObjectArray_FromStackRefArray(ARGS, ARG_COUNT, NAME##_temp + 1);

#define STACKREFS_TO_PYOBJECTS_CLEANUP(NAME) \
    /* +1 because we +1 previously */ \
    _PyObjectArray_Free(NAME - 1, NAME##_temp);

#define CONVERSION_FAILED(NAME) ((NAME) == NULL)

static inline int
check_periodics(PyThreadState *tstate) {
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
    QSBR_QUIESCENT_STATE(tstate);
    if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
        return _Py_HandlePending(tstate);
    }
    return 0;
}

