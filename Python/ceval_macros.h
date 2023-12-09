// Macros needed by ceval.c and bytecodes.c

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
#define INSTRUCTION_START(op) \
    do { \
        frame->prev_instr = next_instr++; \
        OPCODE_EXE_INC(op); \
        if (_py_stats) _py_stats->opcode_stats[lastopcode].pair_count[op]++; \
        lastopcode = op; \
    } while (0)
#else
#define INSTRUCTION_START(op) (frame->prev_instr = next_instr++)
#endif

#if USE_COMPUTED_GOTOS
#  define TARGET(op) TARGET_##op: INSTRUCTION_START(op);
#  define DISPATCH_GOTO() goto *opcode_targets[opcode]
#else
#  define TARGET(op) case op: TARGET_##op: INSTRUCTION_START(op);
#  define DISPATCH_GOTO() goto dispatch_opcode
#endif

/* PRE_DISPATCH_GOTO() does lltrace if enabled. Normally a no-op */
#ifdef LLTRACE
#define PRE_DISPATCH_GOTO() if (lltrace) { \
    lltrace_instruction(frame, stack_pointer, next_instr); }
#else
#define PRE_DISPATCH_GOTO() ((void)0)
#endif


/* Do interpreter dispatch accounting for tracing and instrumentation */
#define DISPATCH() \
    { \
        NEXTOPARG(); \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_SAME_OPARG() \
    { \
        opcode = next_instr->op.code; \
        PRE_DISPATCH_GOTO(); \
        DISPATCH_GOTO(); \
    }

#define DISPATCH_INLINED(NEW_FRAME)                     \
    do {                                                \
        assert(tstate->interp->eval_frame == NULL);     \
        _PyFrame_SetStackPointer(frame, stack_pointer); \
        frame->prev_instr = next_instr - 1;             \
        (NEW_FRAME)->previous = frame;                  \
        frame = cframe.current_frame = (NEW_FRAME);     \
        CALL_STAT_INC(inlined_py_calls);                \
        goto start_frame;                               \
    } while (0)

#define CHECK_EVAL_BREAKER() \
    _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY(); \
    if (_Py_atomic_load_relaxed_int32(&tstate->interp->ceval.eval_breaker)) { \
        goto handle_eval_breaker; \
    }


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
#define INSTR_OFFSET() ((int)(next_instr - _PyCode_CODE(frame->f_code)))
#define NEXTOPARG()  do { \
        _Py_CODEUNIT word = *next_instr; \
        opcode = word.op.code; \
        oparg = word.op.arg; \
    } while (0)
#define JUMPTO(x)       (next_instr = _PyCode_CODE(frame->f_code) + (x))
#define JUMPBY(x)       (next_instr += (x))

/* OpCode prediction macros
    Some opcodes tend to come in pairs thus making it possible to
    predict the second code when the first is run.  For example,
    COMPARE_OP is often followed by POP_JUMP_IF_FALSE or POP_JUMP_IF_TRUE.

    Verifying the prediction costs a single high-speed test of a register
    variable against a constant.  If the pairing was good, then the
    processor's own internal branch predication has a high likelihood of
    success, resulting in a nearly zero-overhead transition to the
    next opcode.  A successful prediction saves a trip through the eval-loop
    including its unpredictable switch-case branch.  Combined with the
    processor's internal branch prediction, a successful PREDICT has the
    effect of making the two opcodes run as if they were a single new opcode
    with the bodies combined.

    If collecting opcode statistics, your choices are to either keep the
    predictions turned-on and interpret the results as if some opcodes
    had been combined or turn-off predictions so that the opcode frequency
    counter updates for both opcodes.

    Opcode prediction is disabled with threaded code, since the latter allows
    the CPU to record separate branch prediction information for each
    opcode.

*/

#define PREDICT_ID(op)          PRED_##op

#if USE_COMPUTED_GOTOS
#define PREDICT(op)             if (0) goto PREDICT_ID(op)
#else
#define PREDICT(next_op) \
    do { \
        _Py_CODEUNIT word = *next_instr; \
        opcode = word.op.code; \
        if (opcode == next_op) { \
            oparg = word.op.arg; \
            INSTRUCTION_START(next_op); \
            goto PREDICT_ID(next_op); \
        } \
    } while(0)
#endif
#define PREDICTED(op)           PREDICT_ID(op):


/* Stack manipulation macros */

/* The stack can grow at most MAXINT deep, as co_nlocals and
   co_stacksize are ints. */
#define STACK_LEVEL()     ((int)(stack_pointer - _PyFrame_Stackbase(frame)))
#define STACK_SIZE()      (frame->f_code->co_stacksize)
#define EMPTY()           (STACK_LEVEL() == 0)
#define TOP()             (stack_pointer[-1])
#define SECOND()          (stack_pointer[-2])
#define THIRD()           (stack_pointer[-3])
#define FOURTH()          (stack_pointer[-4])
#define PEEK(n)           (stack_pointer[-(n)])
#define POKE(n, v)        (stack_pointer[-(n)] = (v))
#define SET_TOP(v)        (stack_pointer[-1] = (v))
#define SET_SECOND(v)     (stack_pointer[-2] = (v))
#define BASIC_STACKADJ(n) (stack_pointer += n)
#define BASIC_PUSH(v)     (*stack_pointer++ = (v))
#define BASIC_POP()       (*--stack_pointer)

#ifdef Py_DEBUG
#define PUSH(v)         do { \
                            BASIC_PUSH(v); \
                            assert(STACK_LEVEL() <= STACK_SIZE()); \
                        } while (0)
#define POP()           (assert(STACK_LEVEL() > 0), BASIC_POP())
#define STACK_GROW(n)   do { \
                            assert(n >= 0); \
                            BASIC_STACKADJ(n); \
                            assert(STACK_LEVEL() <= STACK_SIZE()); \
                        } while (0)
#define STACK_SHRINK(n) do { \
                            assert(n >= 0); \
                            assert(STACK_LEVEL() >= n); \
                            BASIC_STACKADJ(-(n)); \
                        } while (0)
#else
#define PUSH(v)                BASIC_PUSH(v)
#define POP()                  BASIC_POP()
#define STACK_GROW(n)          BASIC_STACKADJ(n)
#define STACK_SHRINK(n)        BASIC_STACKADJ(-(n))
#endif

/* Local variable macros */

#define GETLOCAL(i)     (frame->localsplus[i])

/* The SETLOCAL() macro must not DECREF the local variable in-place and
   then store the new value; it must copy the old value to a temporary
   value, then store the new value, and then DECREF the temporary value.
   This is because it is possible that during the DECREF the frame is
   accessed by other code (e.g. a __del__ method or gc.collect()) and the
   variable would be pointing to already-freed memory. */
#define SETLOCAL(i, value)      do { PyObject *tmp = GETLOCAL(i); \
                                     GETLOCAL(i) = value; \
                                     Py_XDECREF(tmp); } while (0)

#define GO_TO_INSTRUCTION(op) goto PREDICT_ID(op)

#ifdef Py_STATS
#define UPDATE_MISS_STATS(INSTNAME)                              \
    do {                                                         \
        STAT_INC(opcode, miss);                                  \
        STAT_INC((INSTNAME), miss);                              \
        /* The counter is always the first cache entry: */       \
        if (ADAPTIVE_COUNTER_IS_ZERO(next_instr->cache)) {       \
            STAT_INC((INSTNAME), deopt);                         \
        }                                                        \
        else {                                                   \
            /* This is about to be (incorrectly) incremented: */ \
            STAT_DEC((INSTNAME), deferred);                      \
        }                                                        \
    } while (0)
#else
#define UPDATE_MISS_STATS(INSTNAME) ((void)0)
#endif

#define DEOPT_IF(COND, INSTNAME)                            \
    if ((COND)) {                                           \
        /* This is only a single jump on release builds! */ \
        UPDATE_MISS_STATS((INSTNAME));                      \
        assert(_PyOpcode_Deopt[opcode] == (INSTNAME));      \
        GO_TO_INSTRUCTION(INSTNAME);                        \
    }


#define GLOBALS() frame->f_globals
#define BUILTINS() frame->f_builtins
#define LOCALS() frame->f_locals

#define DTRACE_FUNCTION_ENTRY()  \
    if (PyDTrace_FUNCTION_ENTRY_ENABLED()) { \
        dtrace_function_entry(frame); \
    }

#define ADAPTIVE_COUNTER_IS_ZERO(COUNTER) \
    (((COUNTER) >> ADAPTIVE_BACKOFF_BITS) == 0)

#define ADAPTIVE_COUNTER_IS_MAX(COUNTER) \
    (((COUNTER) >> ADAPTIVE_BACKOFF_BITS) == ((1 << MAX_BACKOFF_VALUE) - 1))

#define DECREMENT_ADAPTIVE_COUNTER(COUNTER)           \
    do {                                              \
        assert(!ADAPTIVE_COUNTER_IS_ZERO((COUNTER))); \
        (COUNTER) -= (1 << ADAPTIVE_BACKOFF_BITS);    \
    } while (0);

#define INCREMENT_ADAPTIVE_COUNTER(COUNTER)          \
    do {                                             \
        assert(!ADAPTIVE_COUNTER_IS_MAX((COUNTER))); \
        (COUNTER) += (1 << ADAPTIVE_BACKOFF_BITS);   \
    } while (0);

#define NAME_ERROR_MSG "name '%.200s' is not defined"

#define KWNAMES_LEN() \
    (kwnames == NULL ? 0 : ((int)PyTuple_GET_SIZE(kwnames)))

#define DECREF_INPUTS_AND_REUSE_FLOAT(left, right, dval, result) \
do { \
    if (Py_REFCNT(left) == 1) { \
        ((PyFloatObject *)left)->ob_fval = (dval); \
        _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);\
        result = (left); \
    } \
    else if (Py_REFCNT(right) == 1)  {\
        ((PyFloatObject *)right)->ob_fval = (dval); \
        _Py_DECREF_NO_DEALLOC(left); \
        result = (right); \
    }\
    else { \
        result = PyFloat_FromDouble(dval); \
        if ((result) == NULL) goto error; \
        _Py_DECREF_NO_DEALLOC(left); \
        _Py_DECREF_NO_DEALLOC(right); \
    } \
} while (0)

// If a trace function sets a new f_lineno and
// *then* raises, we use the destination when searching
// for an exception handler, displaying the traceback, and so on
#define INSTRUMENTED_JUMP(src, dest, event) \
do { \
    _PyFrame_SetStackPointer(frame, stack_pointer); \
    next_instr = _Py_call_instrumentation_jump(tstate, event, frame, src, dest); \
    stack_pointer = _PyFrame_GetStackPointer(frame); \
    if (next_instr == NULL) { \
        next_instr = (dest)+1; \
        goto error; \
    } \
} while (0);
