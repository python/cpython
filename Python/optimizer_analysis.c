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
#include "pycore_dict.h"
#include "pycore_long.h"
#include "cpython/optimizer.h"
#include "pycore_optimizer.h"
#include "pycore_object.h"
#include "pycore_dict.h"
#include "pycore_function.h"
#include "pycore_uop_metadata.h"
#include "pycore_uop_ids.h"
#include "pycore_range.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef Py_DEBUG
    extern const char *_PyUOpName(int index);
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



static inline bool
op_is_end(uint32_t opcode)
{
    return opcode == _EXIT_TRACE || opcode == _JUMP_TO_TOP;
}

static int
get_mutations(PyObject* dict) {
    assert(PyDict_CheckExact(dict));
    PyDictObject *d = (PyDictObject *)dict;
    return (d->ma_version_tag >> DICT_MAX_WATCHERS) & ((1 << DICT_WATCHED_MUTATION_BITS)-1);
}

static void
increment_mutations(PyObject* dict) {
    assert(PyDict_CheckExact(dict));
    PyDictObject *d = (PyDictObject *)dict;
    d->ma_version_tag += (1 << DICT_MAX_WATCHERS);
}

/* The first two dict watcher IDs are reserved for CPython,
 * so we don't need to check that they haven't been used */
#define BUILTINS_WATCHER_ID 0
#define GLOBALS_WATCHER_ID  1

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

static PyObject *
convert_global_to_const(_PyUOpInstruction *inst, PyObject *obj)
{
    assert(inst->opcode == _LOAD_GLOBAL_MODULE || inst->opcode == _LOAD_GLOBAL_BUILTINS || inst->opcode == _LOAD_ATTR_MODULE);
    assert(PyDict_CheckExact(obj));
    PyDictObject *dict = (PyDictObject *)obj;
    assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
    PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
    assert(inst->operand <= UINT16_MAX);
    if ((int)inst->operand >= dict->ma_keys->dk_nentries) {
        return NULL;
    }
    PyObject *res = entries[inst->operand].me_value;
    if (res == NULL) {
        return NULL;
    }
    if (_Py_IsImmortal(res)) {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_BORROW_WITH_NULL : _LOAD_CONST_INLINE_BORROW;
    }
    else {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_WITH_NULL : _LOAD_CONST_INLINE;
    }
    inst->operand = (uint64_t)res;
    return res;
}

static int
incorrect_keys(_PyUOpInstruction *inst, PyObject *obj)
{
    if (!PyDict_CheckExact(obj)) {
        return 1;
    }
    PyDictObject *dict = (PyDictObject *)obj;
    if (dict->ma_keys->dk_version != inst->operand) {
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
        return 1;
    }
    PyObject *globals = frame->f_globals;
    PyFunctionObject *function = (PyFunctionObject *)frame->f_funcobj;
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
    for (int pc = 0; pc < buffer_size; pc++) {
        _PyUOpInstruction *inst = &buffer[pc];
        int opcode = inst->opcode;
        switch(opcode) {
            case _GUARD_BUILTINS_VERSION:
                if (incorrect_keys(inst, builtins)) {
                    return 0;
                }
                if (interp->rare_events.builtin_dict >= _Py_MAX_ALLOWED_BUILTINS_MODIFICATIONS) {
                    continue;
                }
                if ((builtins_watched & 1) == 0) {
                    PyDict_Watch(BUILTINS_WATCHER_ID, builtins);
                    builtins_watched |= 1;
                }
                if (function_checked & 1) {
                    buffer[pc].opcode = NOP;
                }
                else {
                    buffer[pc].opcode = _CHECK_FUNCTION;
                    buffer[pc].operand = function_version;
                    function_checked |= 1;
                }
                break;
            case _GUARD_GLOBALS_VERSION:
                if (incorrect_keys(inst, globals)) {
                    return 0;
                }
                uint64_t watched_mutations = get_mutations(globals);
                if (watched_mutations >= _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
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
                    buffer[pc].operand = function_version;
                    function_checked |= 1;
                }
                break;
            case _LOAD_GLOBAL_BUILTINS:
                if (function_checked & globals_watched & builtins_watched & 1) {
                    convert_global_to_const(inst, builtins);
                }
                break;
            case _LOAD_GLOBAL_MODULE:
                if (function_checked & globals_watched & 1) {
                    convert_global_to_const(inst, globals);
                }
                break;
            case _PUSH_FRAME:
            {
                builtins_watched <<= 1;
                globals_watched <<= 1;
                function_checked <<= 1;
                PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
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
                    return 1;
                }
                break;
            }
            case _POP_FRAME:
            {
                builtins_watched >>= 1;
                globals_watched >>= 1;
                function_checked >>= 1;
                PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
                assert(PyFunction_Check(func));
                function_version = func->func_version;
                globals = func->func_globals;
                builtins = func->func_builtins;
                break;
            }
            case _CHECK_FUNCTION_EXACT_ARGS:
                prechecked_function_version = (uint32_t)buffer[pc].operand;
                break;
            default:
                if (op_is_end(opcode)) {
                    return 1;
                }
                break;
        }
    }
    return 0;
}



#define STACK_LEVEL()     ((int)(stack_pointer - ctx->frame->stack))

#define GETLOCAL(idx)          ((ctx->frame->locals[idx]))

#define REPLACE_OP(INST, OP, ARG, OPERAND)    \
    INST->opcode = OP;            \
    INST->oparg = ARG;            \
    INST->operand = OPERAND;

#define OUT_OF_SPACE_IF_NULL(EXPR)     \
    do {                               \
        if ((EXPR) == NULL) {          \
            goto out_of_space;         \
        }                              \
    } while (0);

#define _LOAD_ATTR_NOT_NULL \
    do {                    \
    OUT_OF_SPACE_IF_NULL(attr = _Py_uop_sym_new_not_null(ctx)); \
    OUT_OF_SPACE_IF_NULL(null = _Py_uop_sym_new_null(ctx)); \
    } while (0);


/* Shortened forms for convenience, used in optimizer_bytecodes.c */
#define sym_is_not_null _Py_uop_sym_is_not_null
#define sym_is_const _Py_uop_sym_is_const
#define sym_get_const _Py_uop_sym_get_const
#define sym_new_unknown _Py_uop_sym_new_unknown
#define sym_new_not_null _Py_uop_sym_new_not_null
#define sym_new_type _Py_uop_sym_new_type
#define sym_is_null _Py_uop_sym_is_null
#define sym_new_const _Py_uop_sym_new_const
#define sym_new_null _Py_uop_sym_new_null
#define sym_has_type _Py_uop_sym_has_type
#define sym_matches_type _Py_uop_sym_matches_type
#define sym_set_null _Py_uop_sym_set_null
#define sym_set_non_null _Py_uop_sym_set_non_null
#define sym_set_type _Py_uop_sym_set_type
#define sym_set_const _Py_uop_sym_set_const
#define sym_is_bottom _Py_uop_sym_is_bottom
#define sym_truthiness _Py_uop_sym_truthiness
#define frame_new _Py_uop_frame_new
#define frame_pop _Py_uop_frame_pop

static int
optimize_to_bool(
    _PyUOpInstruction *this_instr,
    _Py_UOpsContext *ctx,
    _Py_UopsSymbol *value,
    _Py_UopsSymbol **result_ptr)
{
    if (sym_matches_type(value, &PyBool_Type)) {
        REPLACE_OP(this_instr, _NOP, 0, 0);
        *result_ptr = value;
        return 1;
    }
    int truthiness = sym_truthiness(value);
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

    _Py_UOpsContext context;
    _Py_UOpsContext *ctx = &context;

    if (_Py_uop_abstractcontext_init(ctx) < 0) {
        goto out_of_space;
    }
    _Py_UOpsAbstractFrame *frame = _Py_uop_frame_new(ctx, co, ctx->n_consumed, 0, curr_stacklen);
    if (frame == NULL) {
        return -1;
    }
    ctx->curr_frame_depth++;
    ctx->frame = frame;

    for (_PyUOpInstruction *this_instr = trace;
         this_instr < trace + trace_len && !op_is_end(this_instr->opcode);
         this_instr++) {

        int oparg = this_instr->oparg;
        uint32_t opcode = this_instr->opcode;

        _Py_UopsSymbol **stack_pointer = ctx->frame->stack_pointer;

        DPRINTF(3, "Abstract interpreting %s:%d ",
                _PyUOpName(opcode),
                oparg);
        switch (opcode) {
#include "optimizer_cases.c.h"

            default:
                DPRINTF(1, "Unknown opcode in abstract interpreter\n");
                Py_UNREACHABLE();
        }
        assert(ctx->frame != NULL);
        DPRINTF(3, " stack_level %d\n", STACK_LEVEL());
        ctx->frame->stack_pointer = stack_pointer;
        assert(STACK_LEVEL() >= 0);
    }

    _Py_uop_abstractcontext_fini(ctx);
    return 1;

out_of_space:
    DPRINTF(1, "Out of space in abstract interpreter\n");
    _Py_uop_abstractcontext_fini(ctx);
    return 0;

error:
    DPRINTF(1, "Encountered error in abstract interpreter\n");
    _Py_uop_abstractcontext_fini(ctx);
    return 0;

hit_bottom:
    // Attempted to push a "bottom" (contradition) symbol onto the stack.
    // This means that the abstract interpreter has hit unreachable code.
    // We *could* generate an _EXIT_TRACE or _FATAL_ERROR here, but it's
    // simpler to just admit failure and not create the executor.
    DPRINTF(1, "Hit bottom in abstract interpreter\n");
    _Py_uop_abstractcontext_fini(ctx);
    return 0;
}


static void
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
            case _SET_IP:
                buffer[pc].opcode = NOP;
                last_set_ip = pc;
                break;
            case _CHECK_VALIDITY:
                if (may_have_escaped) {
                    may_have_escaped = false;
                }
                else {
                    buffer[pc].opcode = NOP;
                }
                break;
            case _CHECK_VALIDITY_AND_SET_IP:
                if (may_have_escaped) {
                    may_have_escaped = false;
                    buffer[pc].opcode = _CHECK_VALIDITY;
                }
                else {
                    buffer[pc].opcode = NOP;
                }
                last_set_ip = pc;
                break;
            case _POP_TOP:
            {
                _PyUOpInstruction *last = &buffer[pc-1];
                while (last->opcode == _NOP) {
                    last--;
                }
                if (last->opcode == _LOAD_CONST_INLINE  ||
                    last->opcode == _LOAD_CONST_INLINE_BORROW ||
                    last->opcode == _LOAD_FAST ||
                    last->opcode == _COPY
                ) {
                    last->opcode = _NOP;
                    buffer[pc].opcode = NOP;
                }
                if (last->opcode == _REPLACE_WITH_TRUE) {
                    last->opcode = _NOP;
                }
                break;
            }
            case _JUMP_TO_TOP:
            case _EXIT_TRACE:
                return;
            default:
            {
                bool needs_ip = false;
                if (_PyUop_Flags[opcode] & HAS_ESCAPES_FLAG) {
                    needs_ip = true;
                    may_have_escaped = true;
                }
                if (_PyUop_Flags[opcode] & HAS_ERROR_FLAG) {
                    needs_ip = true;
                }
                if (needs_ip && last_set_ip >= 0) {
                    if (buffer[last_set_ip].opcode == _CHECK_VALIDITY) {
                        buffer[last_set_ip].opcode = _CHECK_VALIDITY_AND_SET_IP;
                    }
                    else {
                        assert(buffer[last_set_ip].opcode == _NOP);
                        buffer[last_set_ip].opcode = _SET_IP;
                    }
                    last_set_ip = -1;
                }
            }
        }
    }
}

static void
peephole_opt(_PyInterpreterFrame *frame, _PyUOpInstruction *buffer, int buffer_size)
{
    PyCodeObject *co = (PyCodeObject *)frame->f_executable;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        switch(opcode) {
            case _LOAD_CONST: {
                assert(co != NULL);
                PyObject *val = PyTuple_GET_ITEM(co->co_consts, buffer[pc].oparg);
                buffer[pc].opcode = _Py_IsImmortal(val) ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
                buffer[pc].operand = (uintptr_t)val;
                break;
            }
            case _CHECK_PEP_523:
            {
                /* Setting the eval frame function invalidates
                 * all executors, so no need to check dynamically */
                if (_PyInterpreterState_GET()->eval_frame == NULL) {
                    buffer[pc].opcode = _NOP;
                }
                break;
            }
            case _PUSH_FRAME:
            case _POP_FRAME:
            {
                PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
                if (func == NULL) {
                    co = NULL;
                }
                else {
                    assert(PyFunction_Check(func));
                    co = (PyCodeObject *)func->func_code;
                }
                break;
            }
            case _JUMP_TO_TOP:
            case _EXIT_TRACE:
                return;
        }
    }
}

//  0 - failure, no error raised, just fall back to Tier 1
// -1 - failure, and raise error
//  1 - optimizer success
int
_Py_uop_analyze_and_optimize(
    _PyInterpreterFrame *frame,
    _PyUOpInstruction *buffer,
    int buffer_size,
    int curr_stacklen,
    _PyBloomFilter *dependencies
)
{
    OPT_STAT_INC(optimizer_attempts);

    int err = remove_globals(frame, buffer, buffer_size, dependencies);
    if (err == 0) {
        goto not_ready;
    }
    if (err < 0) {
        goto error;
    }

    peephole_opt(frame, buffer, buffer_size);

    err = optimize_uops(
        (PyCodeObject *)frame->f_executable, buffer,
        buffer_size, curr_stacklen, dependencies);

    if (err == 0) {
        goto not_ready;
    }
    assert(err == 1);

    remove_unneeded_uops(buffer, buffer_size);

    OPT_STAT_INC(optimizer_successes);
    return 1;
not_ready:
    return 0;
error:
    return -1;
}
