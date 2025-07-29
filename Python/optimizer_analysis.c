#ifdef _Py_TIER2

/*
 * This file contains the support code for CPython's uops optimizer.
 * It also performs some simple optimizations.
 * It performs a traditional data-flow analysis[1] over the trace of uops.
 * Using the information gained, it chooses to emit, or skip certain instructions
 * if possible.
 *
 * [1] For information on data-flow analysis, please see
 * https://clang.llvm.org/docs/DataFlowAnalysisIntro.html
 *
 * */
#include "Python.h"
#include "opcode.h"
#include "pycore_dict.h"
#include "pycore_interp.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uop_metadata.h"
#include "pycore_long.h"
#include "pycore_interpframe.h"  // _PyFrame_GetCode
#include "pycore_optimizer.h"
#include "pycore_object.h"
#include "pycore_function.h"
#include "pycore_uop_ids.h"
#include "pycore_range.h"
#include "pycore_unicodeobject.h"
#include "pycore_ceval.h"
#include "pycore_floatobject.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef Py_DEBUG
    extern const char *_PyUOpName(int index);
    extern void _PyUOpPrint(const _PyUOpInstruction *uop);
    static const char *const DEBUG_ENV = "PYTHON_OPT_DEBUG";
    static inline int get_lltrace(void) {
        char *uop_debug = Py_GETENV(DEBUG_ENV);
        int lltrace = 0;
        if (uop_debug != NULL && *uop_debug >= '0') {
            lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
        }
        return lltrace;
    }
    #define DPRINTF(level, ...) \
    if (get_lltrace() >= (level)) { printf(__VA_ARGS__); }
#else
    #define DPRINTF(level, ...)
#endif

static int
get_mutations(PyObject* dict) {
    assert(PyDict_CheckExact(dict));
    PyDictObject *d = (PyDictObject *)dict;
    return (d->_ma_watcher_tag >> DICT_MAX_WATCHERS) & ((1 << DICT_WATCHED_MUTATION_BITS)-1);
}

static void
increment_mutations(PyObject* dict) {
    assert(PyDict_CheckExact(dict));
    PyDictObject *d = (PyDictObject *)dict;
    d->_ma_watcher_tag += (1 << DICT_MAX_WATCHERS);
}

/* The first two dict watcher IDs are reserved for CPython,
 * so we don't need to check that they haven't been used */
#define BUILTINS_WATCHER_ID 0
#define GLOBALS_WATCHER_ID  1
#define TYPE_WATCHER_ID  0

static int
globals_watcher_callback(PyDict_WatchEvent event, PyObject* dict,
                         PyObject* key, PyObject* new_value)
{
    RARE_EVENT_STAT_INC(watched_globals_modification);
    assert(get_mutations(dict) < _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS);
    _Py_Executors_InvalidateDependency(_PyInterpreterState_GET(), dict, 1);
    increment_mutations(dict);
    PyDict_Unwatch(GLOBALS_WATCHER_ID, dict);
    return 0;
}

static int
type_watcher_callback(PyTypeObject* type)
{
    _Py_Executors_InvalidateDependency(_PyInterpreterState_GET(), type, 1);
    PyType_Unwatch(TYPE_WATCHER_ID, (PyObject *)type);
    return 0;
}

static PyObject *
convert_global_to_const(_PyUOpInstruction *inst, PyObject *obj, bool pop)
{
    assert(inst->opcode == _LOAD_GLOBAL_MODULE || inst->opcode == _LOAD_GLOBAL_BUILTINS || inst->opcode == _LOAD_ATTR_MODULE);
    assert(PyDict_CheckExact(obj));
    PyDictObject *dict = (PyDictObject *)obj;
    assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
    PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
    int64_t index = inst->operand1;
    assert(index <= UINT16_MAX);
    if ((int)index >= dict->ma_keys->dk_nentries) {
        return NULL;
    }
    PyDictKeysObject *keys = dict->ma_keys;
    if (keys->dk_version != inst->operand0) {
        return NULL;
    }
    PyObject *res = entries[index].me_value;
    if (res == NULL) {
        return NULL;
    }
    if (_Py_IsImmortal(res)) {
        inst->opcode = pop ? _POP_TOP_LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE_BORROW;
    }
    else {
        inst->opcode = pop ? _POP_TOP_LOAD_CONST_INLINE : _LOAD_CONST_INLINE;
    }
    if (inst->oparg & 1) {
        assert(inst[1].opcode == _PUSH_NULL_CONDITIONAL);
        assert(inst[1].oparg & 1);
    }
    inst->operand0 = (uint64_t)res;
    return res;
}

static int
incorrect_keys(_PyUOpInstruction *inst, PyObject *obj)
{
    if (!PyDict_CheckExact(obj)) {
        return 1;
    }
    PyDictObject *dict = (PyDictObject *)obj;
    if (dict->ma_keys->dk_version != inst->operand0) {
        return 1;
    }
    return 0;
}

/* Returns 1 if successfully optimized
 *         0 if the trace is not suitable for optimization (yet)
 *        -1 if there was an error. */
static int
remove_globals(_PyInterpreterFrame *frame, _PyUOpInstruction *buffer,
               int buffer_size, _PyBloomFilter *dependencies)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *builtins = frame->f_builtins;
    if (builtins != interp->builtins) {
        OPT_STAT_INC(remove_globals_builtins_changed);
        return 1;
    }
    PyObject *globals = frame->f_globals;
    PyFunctionObject *function = _PyFrame_GetFunction(frame);
    assert(PyFunction_Check(function));
    assert(function->func_builtins == builtins);
    assert(function->func_globals == globals);
    uint32_t function_version = _PyFunction_GetVersionForCurrentState(function);
    /* In order to treat globals as constants, we need to
     * know that the globals dict is the one we expected, and
     * that it hasn't changed
     * In order to treat builtins as constants,  we need to
     * know that the builtins dict is the one we expected, and
     * that it hasn't changed and that the global dictionary's
     * keys have not changed */

    /* These values represent stacks of booleans (one bool per bit).
     * Pushing a frame shifts left, popping a frame shifts right. */
    uint32_t function_checked = 0;
    uint32_t builtins_watched = 0;
    uint32_t globals_watched = 0;
    uint32_t prechecked_function_version = 0;
    if (interp->dict_state.watchers[GLOBALS_WATCHER_ID] == NULL) {
        interp->dict_state.watchers[GLOBALS_WATCHER_ID] = globals_watcher_callback;
    }
    if (interp->type_watchers[TYPE_WATCHER_ID] == NULL) {
        interp->type_watchers[TYPE_WATCHER_ID] = type_watcher_callback;
    }
    for (int pc = 0; pc < buffer_size; pc++) {
        _PyUOpInstruction *inst = &buffer[pc];
        int opcode = inst->opcode;
        switch(opcode) {
            case _GUARD_GLOBALS_VERSION:
                if (incorrect_keys(inst, globals)) {
                    OPT_STAT_INC(remove_globals_incorrect_keys);
                    return 0;
                }
                if (get_mutations(globals) >= _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                    continue;
                }
                if ((globals_watched & 1) == 0) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, globals);
                    _Py_BloomFilter_Add(dependencies, globals);
                    globals_watched |= 1;
                }
                if (function_checked & 1) {
                    buffer[pc].opcode = NOP;
                }
                else {
                    buffer[pc].opcode = _CHECK_FUNCTION;
                    buffer[pc].operand0 = function_version;
                    function_checked |= 1;
                }
                break;
            case _LOAD_GLOBAL_BUILTINS:
                if (incorrect_keys(inst, builtins)) {
                    OPT_STAT_INC(remove_globals_incorrect_keys);
                    return 0;
                }
                if (interp->rare_events.builtin_dict >= _Py_MAX_ALLOWED_BUILTINS_MODIFICATIONS) {
                    continue;
                }
                if ((builtins_watched & 1) == 0) {
                    PyDict_Watch(BUILTINS_WATCHER_ID, builtins);
                    builtins_watched |= 1;
                }
                if (function_checked & globals_watched & 1) {
                    convert_global_to_const(inst, builtins, false);
                }
                break;
            case _LOAD_GLOBAL_MODULE:
                if (incorrect_keys(inst, globals)) {
                    OPT_STAT_INC(remove_globals_incorrect_keys);
                    return 0;
                }
                if (get_mutations(globals) >= _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                    continue;
                }
                if ((globals_watched & 1) == 0) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, globals);
                    _Py_BloomFilter_Add(dependencies, globals);
                    globals_watched |= 1;
                }
                if ((function_checked & 1) == 0 && buffer[pc-1].opcode == _NOP) {
                    buffer[pc-1].opcode = _CHECK_FUNCTION;
                    buffer[pc-1].operand0 = function_version;
                    function_checked |= 1;
                }
                if (function_checked & 1) {
                    convert_global_to_const(inst, globals, false);
                }
                break;
            case _PUSH_FRAME:
            {
                builtins_watched <<= 1;
                globals_watched <<= 1;
                function_checked <<= 1;
                uint64_t operand = buffer[pc].operand0;
                if (operand == 0 || (operand & 1)) {
                    // It's either a code object or NULL, so bail
                    return 1;
                }
                PyFunctionObject *func = (PyFunctionObject *)operand;
                if (func == NULL) {
                    return 1;
                }
                assert(PyFunction_Check(func));
                function_version = func->func_version;
                if (prechecked_function_version == function_version) {
                    function_checked |= 1;
                }
                prechecked_function_version = 0;
                globals = func->func_globals;
                builtins = func->func_builtins;
                if (builtins != interp->builtins) {
                    OPT_STAT_INC(remove_globals_builtins_changed);
                    return 1;
                }
                break;
            }
            case _RETURN_VALUE:
            {
                builtins_watched >>= 1;
                globals_watched >>= 1;
                function_checked >>= 1;
                uint64_t operand = buffer[pc].operand0;
                if (operand == 0 || (operand & 1)) {
                    // It's either a code object or NULL, so bail
                    return 1;
                }
                PyFunctionObject *func = (PyFunctionObject *)operand;
                if (func == NULL) {
                    return 1;
                }
                assert(PyFunction_Check(func));
                function_version = func->func_version;
                globals = func->func_globals;
                builtins = func->func_builtins;
                break;
            }
            case _CHECK_FUNCTION_EXACT_ARGS:
                prechecked_function_version = (uint32_t)buffer[pc].operand0;
                break;
            default:
                if (is_terminator(inst)) {
                    return 1;
                }
                break;
        }
    }
    return 0;
}



#define STACK_LEVEL()     ((int)(stack_pointer - ctx->frame->stack))
#define STACK_SIZE()      ((int)(ctx->frame->stack_len))

#define WITHIN_STACK_BOUNDS() \
    (STACK_LEVEL() >= 0 && STACK_LEVEL() <= STACK_SIZE())


#define GETLOCAL(idx)          ((ctx->frame->locals[idx]))

#define REPLACE_OP(INST, OP, ARG, OPERAND)    \
    INST->opcode = OP;            \
    INST->oparg = ARG;            \
    INST->operand0 = OPERAND;

/* Shortened forms for convenience, used in optimizer_bytecodes.c */
#define sym_is_not_null _Py_uop_sym_is_not_null
#define sym_is_const _Py_uop_sym_is_const
#define sym_is_safe_const _Py_uop_sym_is_safe_const
#define sym_get_const _Py_uop_sym_get_const
#define sym_new_const_steal _Py_uop_sym_new_const_steal
#define sym_get_const_as_stackref _Py_uop_sym_get_const_as_stackref
#define sym_new_unknown _Py_uop_sym_new_unknown
#define sym_new_not_null _Py_uop_sym_new_not_null
#define sym_new_type _Py_uop_sym_new_type
#define sym_is_null _Py_uop_sym_is_null
#define sym_new_const _Py_uop_sym_new_const
#define sym_new_null _Py_uop_sym_new_null
#define sym_has_type _Py_uop_sym_has_type
#define sym_get_type _Py_uop_sym_get_type
#define sym_matches_type _Py_uop_sym_matches_type
#define sym_matches_type_version _Py_uop_sym_matches_type_version
#define sym_set_null(SYM) _Py_uop_sym_set_null(ctx, SYM)
#define sym_set_non_null(SYM) _Py_uop_sym_set_non_null(ctx, SYM)
#define sym_set_type(SYM, TYPE) _Py_uop_sym_set_type(ctx, SYM, TYPE)
#define sym_set_type_version(SYM, VERSION) _Py_uop_sym_set_type_version(ctx, SYM, VERSION)
#define sym_set_const(SYM, CNST) _Py_uop_sym_set_const(ctx, SYM, CNST)
#define sym_set_compact_int(SYM) _Py_uop_sym_set_compact_int(ctx, SYM)
#define sym_is_bottom _Py_uop_sym_is_bottom
#define sym_truthiness _Py_uop_sym_truthiness
#define frame_new _Py_uop_frame_new
#define frame_pop _Py_uop_frame_pop
#define sym_new_tuple _Py_uop_sym_new_tuple
#define sym_tuple_getitem _Py_uop_sym_tuple_getitem
#define sym_tuple_length _Py_uop_sym_tuple_length
#define sym_is_immortal _Py_uop_symbol_is_immortal
#define sym_is_compact_int _Py_uop_sym_is_compact_int
#define sym_new_compact_int _Py_uop_sym_new_compact_int
#define sym_new_truthiness _Py_uop_sym_new_truthiness

#define JUMP_TO_LABEL(label) goto label;

static int
optimize_to_bool(
    _PyUOpInstruction *this_instr,
    JitOptContext *ctx,
    JitOptRef value,
    JitOptRef *result_ptr)
{
    if (sym_matches_type(value, &PyBool_Type)) {
        REPLACE_OP(this_instr, _NOP, 0, 0);
        *result_ptr = value;
        return 1;
    }
    int truthiness = sym_truthiness(ctx, value);
    if (truthiness >= 0) {
        PyObject *load = truthiness ? Py_True : Py_False;
        REPLACE_OP(this_instr, _POP_TOP_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)load);
        *result_ptr = sym_new_const(ctx, load);
        return 1;
    }
    return 0;
}

static void
eliminate_pop_guard(_PyUOpInstruction *this_instr, bool exit)
{
    REPLACE_OP(this_instr, _POP_TOP, 0, 0);
    if (exit) {
        REPLACE_OP((this_instr+1), _EXIT_TRACE, 0, 0);
        this_instr[1].target = this_instr->target;
    }
}

static JitOptRef
lookup_attr(JitOptContext *ctx, _PyUOpInstruction *this_instr,
            PyTypeObject *type, PyObject *name, uint16_t immortal,
            uint16_t mortal)
{
    // The cached value may be dead, so we need to do the lookup again... :(
    if (type && PyType_Check(type)) {
        PyObject *lookup = _PyType_Lookup(type, name);
        if (lookup) {
            int opcode = _Py_IsImmortal(lookup) ? immortal : mortal;
            REPLACE_OP(this_instr, opcode, 0, (uintptr_t)lookup);
            return sym_new_const(ctx, lookup);
        }
    }
    return sym_new_not_null(ctx);
}

/* _PUSH_FRAME/_RETURN_VALUE's operand can be 0, a PyFunctionObject *, or a
 * PyCodeObject *. Retrieve the code object if possible.
 */
static PyCodeObject *
get_code(_PyUOpInstruction *op)
{
    assert(op->opcode == _PUSH_FRAME || op->opcode == _RETURN_VALUE || op->opcode == _RETURN_GENERATOR);
    PyCodeObject *co = NULL;
    uint64_t operand = op->operand0;
    if (operand == 0) {
        return NULL;
    }
    if (operand & 1) {
        co = (PyCodeObject *)(operand & ~1);
    }
    else {
        PyFunctionObject *func = (PyFunctionObject *)operand;
        assert(PyFunction_Check(func));
        co = (PyCodeObject *)func->func_code;
    }
    assert(PyCode_Check(co));
    return co;
}

static PyCodeObject *
get_code_with_logging(_PyUOpInstruction *op)
{
    PyCodeObject *co = NULL;
    uint64_t push_operand = op->operand0;
    if (push_operand & 1) {
        co = (PyCodeObject *)(push_operand & ~1);
        DPRINTF(3, "code=%p ", co);
        assert(PyCode_Check(co));
    }
    else {
        PyFunctionObject *func = (PyFunctionObject *)push_operand;
        DPRINTF(3, "func=%p ", func);
        if (func == NULL) {
            DPRINTF(3, "\n");
            DPRINTF(1, "Missing function\n");
            return NULL;
        }
        co = (PyCodeObject *)func->func_code;
        DPRINTF(3, "code=%p ", co);
    }
    return co;
}

// TODO (gh-134584) generate most of this table automatically
const uint16_t op_without_decref_inputs[MAX_UOP_ID + 1] = {
    [_BINARY_OP_MULTIPLY_FLOAT] = _BINARY_OP_MULTIPLY_FLOAT__NO_DECREF_INPUTS,
    [_BINARY_OP_ADD_FLOAT] = _BINARY_OP_ADD_FLOAT__NO_DECREF_INPUTS,
    [_BINARY_OP_SUBTRACT_FLOAT] = _BINARY_OP_SUBTRACT_FLOAT__NO_DECREF_INPUTS,
};

/* 1 for success, 0 for not ready, cannot error at the moment. */
static int
optimize_uops(
    PyCodeObject *co,
    _PyUOpInstruction *trace,
    int trace_len,
    int curr_stacklen,
    _PyBloomFilter *dependencies
)
{

    JitOptContext context;
    JitOptContext *ctx = &context;
    uint32_t opcode = UINT16_MAX;
    int curr_space = 0;
    int max_space = 0;
    _PyUOpInstruction *first_valid_check_stack = NULL;
    _PyUOpInstruction *corresponding_check_stack = NULL;

    _Py_uop_abstractcontext_init(ctx);
    _Py_UOpsAbstractFrame *frame = _Py_uop_frame_new(ctx, co, curr_stacklen, NULL, 0);
    if (frame == NULL) {
        return -1;
    }
    ctx->curr_frame_depth++;
    ctx->frame = frame;
    ctx->done = false;
    ctx->out_of_space = false;
    ctx->contradiction = false;

    _PyUOpInstruction *this_instr = NULL;
    for (int i = 0; !ctx->done; i++) {
        assert(i < trace_len);
        this_instr = &trace[i];

        int oparg = this_instr->oparg;
        opcode = this_instr->opcode;
        JitOptRef *stack_pointer = ctx->frame->stack_pointer;

#ifdef Py_DEBUG
        if (get_lltrace() >= 3) {
            printf("%4d abs: ", (int)(this_instr - trace));
            _PyUOpPrint(this_instr);
            printf(" ");
        }
#endif

        switch (opcode) {

#include "optimizer_cases.c.h"

            default:
                DPRINTF(1, "\nUnknown opcode in abstract interpreter\n");
                Py_UNREACHABLE();
        }
        assert(ctx->frame != NULL);
        DPRINTF(3, " stack_level %d\n", STACK_LEVEL());
        ctx->frame->stack_pointer = stack_pointer;
        assert(STACK_LEVEL() >= 0);
    }
    if (ctx->out_of_space) {
        DPRINTF(3, "\n");
        DPRINTF(1, "Out of space in abstract interpreter\n");
    }
    if (ctx->contradiction) {
        // Attempted to push a "bottom" (contradiction) symbol onto the stack.
        // This means that the abstract interpreter has hit unreachable code.
        // We *could* generate an _EXIT_TRACE or _FATAL_ERROR here, but hitting
        // bottom indicates type instability, so we are probably better off
        // retrying later.
        DPRINTF(3, "\n");
        DPRINTF(1, "Hit bottom in abstract interpreter\n");
        _Py_uop_abstractcontext_fini(ctx);
        return 0;
    }

    /* Either reached the end or cannot optimize further, but there
     * would be no benefit in retrying later */
    _Py_uop_abstractcontext_fini(ctx);
    if (first_valid_check_stack != NULL) {
        assert(first_valid_check_stack->opcode == _CHECK_STACK_SPACE);
        assert(max_space > 0);
        assert(max_space <= INT_MAX);
        assert(max_space <= INT32_MAX);
        first_valid_check_stack->opcode = _CHECK_STACK_SPACE_OPERAND;
        first_valid_check_stack->operand0 = max_space;
    }
    return trace_len;

error:
    DPRINTF(3, "\n");
    DPRINTF(1, "Encountered error in abstract interpreter\n");
    if (opcode <= MAX_UOP_ID) {
        OPT_ERROR_IN_OPCODE(opcode);
    }
    _Py_uop_abstractcontext_fini(ctx);
    return -1;

}

const uint16_t op_without_push[MAX_UOP_ID + 1] = {
    [_COPY] = _NOP,
    [_LOAD_CONST_INLINE] = _NOP,
    [_LOAD_CONST_INLINE_BORROW] = _NOP,
    [_LOAD_CONST_UNDER_INLINE] = _POP_TOP_LOAD_CONST_INLINE,
    [_LOAD_CONST_UNDER_INLINE_BORROW] = _POP_TOP_LOAD_CONST_INLINE_BORROW,
    [_LOAD_FAST] = _NOP,
    [_LOAD_FAST_BORROW] = _NOP,
    [_LOAD_SMALL_INT] = _NOP,
    [_POP_TOP_LOAD_CONST_INLINE] = _POP_TOP,
    [_POP_TOP_LOAD_CONST_INLINE_BORROW] = _POP_TOP,
    [_POP_TWO_LOAD_CONST_INLINE_BORROW] = _POP_TWO,
    [_POP_CALL_TWO_LOAD_CONST_INLINE_BORROW] = _POP_CALL_TWO,
};

const bool op_skip[MAX_UOP_ID + 1] = {
    [_NOP] = true,
    [_CHECK_VALIDITY] = true,
    [_CHECK_PERIODIC] = true,
    [_SET_IP] = true,
};

const uint16_t op_without_pop[MAX_UOP_ID + 1] = {
    [_POP_TOP] = _NOP,
    [_POP_TOP_LOAD_CONST_INLINE] = _LOAD_CONST_INLINE,
    [_POP_TOP_LOAD_CONST_INLINE_BORROW] = _LOAD_CONST_INLINE_BORROW,
    [_POP_TWO] = _POP_TOP,
    [_POP_TWO_LOAD_CONST_INLINE_BORROW] = _POP_TOP_LOAD_CONST_INLINE_BORROW,
    [_POP_CALL_TWO_LOAD_CONST_INLINE_BORROW] = _POP_CALL_ONE_LOAD_CONST_INLINE_BORROW,
    [_POP_CALL_ONE_LOAD_CONST_INLINE_BORROW] = _POP_CALL_LOAD_CONST_INLINE_BORROW,
    [_POP_CALL_TWO] = _POP_CALL_ONE,
    [_POP_CALL_ONE] = _POP_CALL,
};

const uint16_t op_without_pop_null[MAX_UOP_ID + 1] = {
    [_POP_CALL] = _POP_TOP,
    [_POP_CALL_LOAD_CONST_INLINE_BORROW] = _POP_TOP_LOAD_CONST_INLINE_BORROW,
};


static int
remove_unneeded_uops(_PyUOpInstruction *buffer, int buffer_size)
{
    /* Remove _SET_IP and _CHECK_VALIDITY where possible.
     * _SET_IP is needed if the following instruction escapes or
     * could error. _CHECK_VALIDITY is needed if the previous
     * instruction could have escaped. */
    int last_set_ip = -1;
    bool may_have_escaped = true;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        switch (opcode) {
            case _START_EXECUTOR:
                may_have_escaped = false;
                break;
            case _SET_IP:
                buffer[pc].opcode = _NOP;
                last_set_ip = pc;
                break;
            case _CHECK_VALIDITY:
                if (may_have_escaped) {
                    may_have_escaped = false;
                }
                else {
                    buffer[pc].opcode = _NOP;
                }
                break;
            default:
            {
                // Cancel out pushes and pops, repeatedly. So:
                //     _LOAD_FAST + _POP_TWO_LOAD_CONST_INLINE_BORROW + _POP_TOP
                // ...becomes:
                //     _NOP + _POP_TOP + _NOP
                while (op_without_pop[opcode] || op_without_pop_null[opcode]) {
                    _PyUOpInstruction *last = &buffer[pc - 1];
                    while (op_skip[last->opcode]) {
                        last--;
                    }
                    if (op_without_push[last->opcode] && op_without_pop[opcode]) {
                        last->opcode = op_without_push[last->opcode];
                        opcode = buffer[pc].opcode = op_without_pop[opcode];
                        if (op_without_pop[last->opcode]) {
                            opcode = last->opcode;
                            pc = last - buffer;
                        }
                    }
                    else if (last->opcode == _PUSH_NULL) {
                        // Handle _POP_CALL and _POP_CALL_LOAD_CONST_INLINE_BORROW separately.
                        // This looks for a preceding _PUSH_NULL instruction and
                        // simplifies to _POP_TOP(_LOAD_CONST_INLINE_BORROW).
                        last->opcode = _NOP;
                        opcode = buffer[pc].opcode = op_without_pop_null[opcode];
                        assert(opcode);
                    }
                    else {
                        break;
                    }
                }
                /* _PUSH_FRAME doesn't escape or error, but it
                 * does need the IP for the return address */
                bool needs_ip = opcode == _PUSH_FRAME;
                if (_PyUop_Flags[opcode] & HAS_ESCAPES_FLAG) {
                    needs_ip = true;
                    may_have_escaped = true;
                }
                if (needs_ip && last_set_ip >= 0) {
                    assert(buffer[last_set_ip].opcode == _NOP);
                    buffer[last_set_ip].opcode = _SET_IP;
                    last_set_ip = -1;
                }
                break;
            }
            case _JUMP_TO_TOP:
            case _EXIT_TRACE:
                return pc + 1;
        }
    }
    Py_UNREACHABLE();
}

//  0 - failure, no error raised, just fall back to Tier 1
// -1 - failure, and raise error
//  > 0 - length of optimized trace
int
_Py_uop_analyze_and_optimize(
    _PyInterpreterFrame *frame,
    _PyUOpInstruction *buffer,
    int length,
    int curr_stacklen,
    _PyBloomFilter *dependencies
)
{
    OPT_STAT_INC(optimizer_attempts);

    int err = remove_globals(frame, buffer, length, dependencies);
    if (err <= 0) {
        return err;
    }

    length = optimize_uops(
        _PyFrame_GetCode(frame), buffer,
        length, curr_stacklen, dependencies);

    if (length <= 0) {
        return length;
    }

    length = remove_unneeded_uops(buffer, length);
    assert(length > 0);

    OPT_STAT_INC(optimizer_successes);
    return length;
}

#endif /* _Py_TIER2 */
