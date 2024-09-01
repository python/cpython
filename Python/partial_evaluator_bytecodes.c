#include "Python.h"
#include "pycore_optimizer.h"
#include "pycore_uops.h"
#include "pycore_uop_ids.h"
#include "internal/pycore_moduleobject.h"

#define op(name, ...) /* NAME is ignored */

typedef struct _Py_UopsSymbol _Py_UopsSymbol;
typedef struct _Py_UOpsContext _Py_UOpsContext;
typedef struct _Py_UOpsAbstractFrame _Py_UOpsAbstractFrame;

/* Shortened forms for convenience */
#define sym_is_not_null _Py_uop_sym_is_not_null
#define sym_is_const _Py_uop_sym_is_const
#define sym_get_const _Py_uop_sym_get_const
#define sym_new_unknown _Py_uop_sym_new_unknown
#define sym_new_not_null _Py_uop_sym_new_not_null
#define sym_new_type _Py_uop_sym_new_type
#define sym_is_null _Py_uop_sym_is_null
#define sym_new_const _Py_uop_sym_new_const
#define sym_new_null _Py_uop_sym_new_null
#define sym_matches_type _Py_uop_sym_matches_type
#define sym_matches_type_version _Py_uop_sym_matches_type_version
#define sym_get_type _Py_uop_sym_get_type
#define sym_has_type _Py_uop_sym_has_type
#define sym_set_null(SYM) _Py_uop_sym_set_null(ctx, SYM)
#define sym_set_non_null(SYM) _Py_uop_sym_set_non_null(ctx, SYM)
#define sym_set_type(SYM, TYPE) _Py_uop_sym_set_type(ctx, SYM, TYPE)
#define sym_set_type_version(SYM, VERSION) _Py_uop_sym_set_type_version(ctx, SYM, VERSION)
#define sym_set_const(SYM, CNST) _Py_uop_sym_set_const(ctx, SYM, CNST)
#define sym_is_bottom _Py_uop_sym_is_bottom
#define frame_new _Py_uop_frame_new
#define frame_pop _Py_uop_frame_pop

extern int
optimize_to_bool(
    _PyUOpInstruction *this_instr,
    _Py_UOpsContext *ctx,
    _Py_UopsSymbol *value,
    _Py_UopsSymbol **result_ptr);

extern void
eliminate_pop_guard(_PyUOpInstruction *this_instr, bool exit);

extern PyCodeObject *get_code(_PyUOpInstruction *op);

static int
dummy_func(void) {

    PyCodeObject *co;
    int oparg;
    _Py_UopsSymbol *flag;
    _Py_UopsSymbol *left;
    _Py_UopsSymbol *right;
    _Py_UopsSymbol *value;
    _Py_UopsSymbol *res;
    _Py_UopsSymbol *iter;
    _Py_UopsSymbol *top;
    _Py_UopsSymbol *bottom;
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractFrame *new_frame;
    _Py_UOpsContext *ctx;
    _PyUOpInstruction *this_instr;

// BEGIN BYTECODES //

    op(_LOAD_FAST_CHECK, (-- value)) {
        value = GETLOCAL(oparg);
        // We guarantee this will error - just bail and don't optimize it.
        if (sym_is_null(value)) {
            ctx->done = true;
        }
    }

    op(_LOAD_FAST, (-- value)) {
        value = GETLOCAL(oparg);
    }

    op(_LOAD_FAST_AND_CLEAR, (-- value)) {
        value = GETLOCAL(oparg);
        _Py_UopsSymbol *temp = sym_new_null(ctx);
        GETLOCAL(oparg) = temp;
    }

    op(_STORE_FAST, (value --)) {
        GETLOCAL(oparg) = value;
    }

    op(_PUSH_NULL, (-- res)) {
        res = sym_new_null(ctx);
    }

    op(_LOAD_CONST, (-- value)) {
        // Should never happen. This should be run after the specializer pass.
        Py_UNREACHABLE();
    }

    op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
        value = sym_new_const(ctx, ptr);
    }

    op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
        value = sym_new_const(ctx, ptr);
    }

    op(_LOAD_CONST_INLINE_WITH_NULL, (ptr/4 -- value, null)) {
        value = sym_new_const(ctx, ptr);
        null = sym_new_null(ctx);
    }

    op(_LOAD_CONST_INLINE_BORROW_WITH_NULL, (ptr/4 -- value, null)) {
        value = sym_new_const(ctx, ptr);
        null = sym_new_null(ctx);
    }

    op(_COPY, (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
        assert(oparg > 0);
        top = bottom;
    }

    op(_SWAP, (bottom, unused[oparg-2], top --
        top, unused[oparg-2], bottom)) {
    }

    op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null, args[oparg] -- new_frame: _Py_UOpsAbstractFrame *)) {
        int argcount = oparg;

        (void)callable;

        PyCodeObject *co = NULL;
        assert((this_instr + 2)->opcode == _PUSH_FRAME);
        uint64_t push_operand = (this_instr + 2)->operand;
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
                ctx->done = true;
                break;
            }
            co = (PyCodeObject *)func->func_code;
            DPRINTF(3, "code=%p ", co);
        }

        assert(self_or_null != NULL);
        assert(args != NULL);
        new_frame = frame_new(ctx, co, 0, NULL, 0);
    }

    op(_PY_FRAME_GENERAL, (callable, self_or_null, args[oparg] -- new_frame: _Py_UOpsAbstractFrame *)) {
        /* The _Py_UOpsAbstractFrame design assumes that we can copy arguments across directly */
        (void)callable;
        (void)self_or_null;
        (void)args;
        new_frame = NULL;
        ctx->done = true;
    }

    op(_PY_FRAME_KW, (callable, self_or_null, args[oparg], kwnames -- new_frame: _Py_UOpsAbstractFrame *)) {
        (void)callable;
        (void)self_or_null;
        (void)args;
        (void)kwnames;
        new_frame = NULL;
        ctx->done = true;
    }

    op(_CREATE_INIT_FRAME, (self, init, args[oparg] -- init_frame: _Py_UOpsAbstractFrame *)) {
        (void)self;
        (void)init;
        (void)args;
        init_frame = NULL;
        ctx->done = true;
    }

    op(_RETURN_VALUE, (retval -- res)) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        frame_pop(ctx);
        stack_pointer = ctx->frame->stack_pointer;
        res = retval;

        /* Stack space handling */
        assert(corresponding_check_stack == NULL);
        assert(co != NULL);
        int framesize = co->co_framesize;
        assert(framesize > 0);
        assert(framesize <= curr_space);
        curr_space -= framesize;

        co = get_code(this_instr);
        if (co == NULL) {
            // might be impossible, but bailing is still safe
            ctx->done = true;
        }
    }

    op(_RETURN_GENERATOR, ( -- res)) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        frame_pop(ctx);
        stack_pointer = ctx->frame->stack_pointer;
        res = sym_new_unknown(ctx);

        /* Stack space handling */
        assert(corresponding_check_stack == NULL);
        assert(co != NULL);
        int framesize = co->co_framesize;
        assert(framesize > 0);
        assert(framesize <= curr_space);
        curr_space -= framesize;

        co = get_code(this_instr);
        if (co == NULL) {
            // might be impossible, but bailing is still safe
            ctx->done = true;
        }
    }

    op(_YIELD_VALUE, (unused -- res)) {
        res = sym_new_unknown(ctx);
    }

    op(_FOR_ITER_GEN_FRAME, ( -- )) {
        /* We are about to hit the end of the trace */
        ctx->done = true;
    }

    op(_SEND_GEN_FRAME, ( -- )) {
        // We are about to hit the end of the trace:
        ctx->done = true;
    }

    op(_PUSH_FRAME, (new_frame: _Py_UOpsAbstractFrame * -- unused if (0))) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        ctx->frame = new_frame;
        ctx->curr_frame_depth++;
        stack_pointer = new_frame->stack_pointer;
        co = get_code(this_instr);
        if (co == NULL) {
            // should be about to _EXIT_TRACE anyway
            ctx->done = true;
            break;
        }
    }

    op(_UNPACK_SEQUENCE, (seq -- values[oparg])) {
        /* This has to be done manually */
        (void)seq;
        for (int i = 0; i < oparg; i++) {
            values[i] = sym_new_unknown(ctx);
        }
    }

    op(_UNPACK_EX, (seq -- values[oparg & 0xFF], unused, unused[oparg >> 8])) {
        /* This has to be done manually */
        (void)seq;
        int totalargs = (oparg & 0xFF) + (oparg >> 8) + 1;
        for (int i = 0; i < totalargs; i++) {
            values[i] = sym_new_unknown(ctx);
        }
    }

    op(_JUMP_TO_TOP, (--)) {
        ctx->done = true;
    }

    op(_EXIT_TRACE, (exit_p/4 --)) {
        (void)exit_p;
        ctx->done = true;
    }

// END BYTECODES //

}
