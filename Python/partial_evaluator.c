#ifdef _Py_TIER2

/*
 * This file contains the support code for CPython's partial evaluator.
 * It performs an abstract interpretation[1] over the trace of uops.
 * Using the information gained, it chooses to emit, or skip certain instructions
 * if possible.
 *
 * [1] For information on abstract interpertation and partial evaluation, please see
 * https://en.wikipedia.org/wiki/Abstract_interpretation
 * https://en.wikipedia.org/wiki/Partial_evaluation
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

#define STACK_LEVEL()     ((int)(stack_pointer - ctx->frame->stack))
#define STACK_SIZE()      ((int)(ctx->frame->stack_len))

#define WITHIN_STACK_BOUNDS() \
    (STACK_LEVEL() >= 0 && STACK_LEVEL() <= STACK_SIZE())


#define GETLOCAL(idx)          ((ctx->frame->locals[idx]))


/* _PUSH_FRAME/_RETURN_VALUE's operand can be 0, a PyFunctionObject *, or a
 * PyCodeObject *. Retrieve the code object if possible.
 */
static PyCodeObject *
get_code(_PyUOpInstruction *op)
{
    assert(op->opcode == _PUSH_FRAME || op->opcode == _RETURN_VALUE || op->opcode == _RETURN_GENERATOR);
    PyCodeObject *co = NULL;
    uint64_t operand = op->operand;
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
    uint64_t push_operand = op->operand;
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

#define MATERIALIZE_INST() (this_instr->is_virtual = false)
#define sym_set_origin_inst_override _Py_uop_sym_set_origin_inst_override
#define sym_is_virtual _Py_uop_sym_is_virtual
#define sym_get_origin _Py_uop_sym_get_origin

static void
materialize(_Py_UopsLocalsPlusSlot *slot)
{
    assert(slot != NULL);
    if (slot->origin_inst) {
        slot->origin_inst->is_virtual = false;
    }
}

static void
materialize_stack(_Py_UopsLocalsPlusSlot *stack_start, _Py_UopsLocalsPlusSlot *stack_end)
{
    while (stack_start < stack_end) {
        materialize(stack_start);
        stack_start++;
    }
}

static void
materialize_frame(_Py_UOpsAbstractFrame *frame)
{
    materialize_stack(frame->stack, frame->stack_pointer);
}

static void
materialize_ctx(_Py_UOpsContext *ctx)
{
    for (int i = 0; i < ctx->curr_frame_depth; i++) {
        materialize_frame(&ctx->frames[i]);
    }
}

/* 1 for success, 0 for not ready, cannot error at the moment. */
static int
partial_evaluate_uops(
    PyCodeObject *co,
    _PyUOpInstruction *trace,
    int trace_len,
    int curr_stacklen,
    _PyBloomFilter *dependencies
)
{
    _PyUOpInstruction trace_dest[UOP_MAX_TRACE_LENGTH];
    _Py_UOpsContext context;
    context.trace_dest = trace_dest;
    context.n_trace_dest = trace_len;
    _Py_UOpsContext *ctx = &context;
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

   for (int i = 0; i < trace_len; i++) {
       // The key part of PE --- we assume everything starts off virtual.
       trace_dest[i] = trace[i];
       trace_dest[i].is_virtual = true;
   }

    _PyUOpInstruction *this_instr = NULL;
    int i = 0;
    for (; !ctx->done; i++) {
        assert(i < trace_len);
        this_instr = &trace_dest[i];

        int oparg = this_instr->oparg;
        opcode = this_instr->opcode;
        _Py_UopsLocalsPlusSlot *stack_pointer = ctx->frame->stack_pointer;

#ifdef Py_DEBUG
        if (get_lltrace() >= 3) {
            printf("%4d pe: ", (int)(this_instr - trace_dest));
            _PyUOpPrint(this_instr);
            printf(" ");
        }
#endif

        int is_static = (_PyUop_Flags[opcode] & HAS_STATIC_FLAG);
        if (!is_static) {
            MATERIALIZE_INST();
        }
        if (!is_static &&
            // During these two opcodes, there's an abstract frame on the stack.
            // Which is not a valid symbol.
            (opcode != _PUSH_FRAME && opcode != _SAVE_RETURN_OFFSET)) {
            // An escaping opcode means we need to materialize _everything_.
            if (_PyUop_Flags[opcode] & HAS_ESCAPES_FLAG) {
                materialize_ctx(ctx);
            }
            else {

                materialize_frame(ctx->frame);
            }
        }

        switch (opcode) {

#include "partial_evaluator_cases.c.h"

            default:
                DPRINTF(1, "\nUnknown opcode in pe's abstract interpreter\n");
                Py_UNREACHABLE();
        }
        assert(ctx->frame != NULL);
        DPRINTF(3, " stack_level %d\n", STACK_LEVEL());
        ctx->frame->stack_pointer = stack_pointer;
        assert(STACK_LEVEL() >= 0);
        if (ctx->done) {
            break;
        }
    }
    if (ctx->out_of_space) {
        DPRINTF(3, "\n");
        DPRINTF(1, "Out of space in pe's abstract interpreter\n");
    }
    if (ctx->contradiction) {
        // Attempted to push a "bottom" (contradiction) symbol onto the stack.
        // This means that the abstract interpreter has hit unreachable code.
        // We *could* generate an _EXIT_TRACE or _FATAL_ERROR here, but hitting
        // bottom indicates type instability, so we are probably better off
        // retrying later.
        DPRINTF(3, "\n");
        DPRINTF(1, "Hit bottom in pe's abstract interpreter\n");
        _Py_uop_abstractcontext_fini(ctx);
        return 0;
    }

    if (ctx->out_of_space || !is_terminator(this_instr)) {
        _Py_uop_abstractcontext_fini(ctx);
        return trace_len;
    }
    else {
        // We MUST not have bailed early here.
        // That's the only time the PE's residual is valid.
        assert(ctx->n_trace_dest < UOP_MAX_TRACE_LENGTH);
        assert(is_terminator(this_instr));
        assert(ctx->n_trace_dest <= trace_len);

        // Copy trace_dest into trace.
        int trace_dest_len = ctx->n_trace_dest;
        // Only valid before we start inserting side exits.
        assert(trace_dest_len == trace_len);
        for (int x = 0; x < trace_dest_len; x++) {
            // Skip all virtual instructions.
            if (trace_dest[x].is_virtual) {
                trace[x].opcode = _NOP;
            }
            else {
                trace[x] = trace_dest[x];
            }
        }
        _Py_uop_abstractcontext_fini(ctx);
        return trace_dest_len;
    }

error:
    DPRINTF(3, "\n");
    DPRINTF(1, "Encountered error in pe's abstract interpreter\n");
    if (opcode <= MAX_UOP_ID) {
        OPT_ERROR_IN_OPCODE(opcode);
    }
    _Py_uop_abstractcontext_fini(ctx);
    return -1;

}


//  0 - failure, no error raised, just fall back to Tier 1
// -1 - failure, and raise error
//  > 0 - length of optimized trace
int
_Py_uop_partial_evaluate(
    _PyInterpreterFrame *frame,
    _PyUOpInstruction *buffer,
    int length,
    int curr_stacklen,
    _PyBloomFilter *dependencies
)
{

    length = partial_evaluate_uops(
        _PyFrame_GetCode(frame), buffer,
        length, curr_stacklen, dependencies);

    if (length <= 0) {
        return length;
    }

    return length;
}

#endif /* _Py_TIER2 */
