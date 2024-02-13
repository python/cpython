#include "Python.h"
#include "pycore_uops.h"
#include "pycore_uop_ids.h"

#define op(name, ...) /* NAME is ignored */

typedef struct _Py_UOpsSymType _Py_UOpsSymType;
typedef struct _Py_UOpsAbstractInterpContext _Py_UOpsAbstractInterpContext;
typedef struct _Py_UOpsAbstractFrame _Py_UOpsAbstractFrame;

static int
dummy_func(void) {

    PyCodeObject *code;
    int oparg;
    _Py_UOpsSymType *flag;
    _Py_UOpsSymType *left;
    _Py_UOpsSymType *right;
    _Py_UOpsSymType *value;
    _Py_UOpsSymType *res;
    _Py_UOpsSymType *iter;
    _Py_UOpsSymType *top;
    _Py_UOpsSymType *bottom;
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractInterpContext *ctx;
    _PyUOpInstruction *this_instr;
    _PyBloomFilter *dependencies;
    int modified;

// BEGIN BYTECODES //

    op(_LOAD_FAST_CHECK, (-- value)) {
        value = GETLOCAL(oparg);
        // We guarantee this will error - just bail and don't optimize it.
        if (sym_is_null(value)) {
            goto out_of_space;
        }
    }

    op(_LOAD_FAST, (-- value)) {
        value = GETLOCAL(oparg);
    }

    op(_LOAD_FAST_AND_CLEAR, (-- value)) {
        value = GETLOCAL(oparg);
        _Py_UOpsSymType *temp = sym_new_null(ctx);
        if (temp == NULL) {
            goto out_of_space;
        }
        GETLOCAL(oparg) = temp;
    }

    op(_STORE_FAST, (value --)) {
        GETLOCAL(oparg) = value;
    }

    op(_PUSH_NULL, (-- res)) {
        res = sym_new_null(ctx);
        if (res == NULL) {
            goto out_of_space;
        };
    }

    op(_GUARD_BOTH_INT, (left, right -- left, right)) {
        if (sym_matches_type(left, &PyLong_Type) &&
            sym_matches_type(right, &PyLong_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        sym_set_type(left, &PyLong_Type);
        sym_set_type(right, &PyLong_Type);
    }

    op(_GUARD_BOTH_FLOAT, (left, right -- left, right)) {
        if (sym_matches_type(left, &PyFloat_Type) &&
            sym_matches_type(right, &PyFloat_Type)) {
            REPLACE_OP(this_instr, _NOP, 0 ,0);
        }
        sym_set_type(left, &PyFloat_Type);
        sym_set_type(right, &PyFloat_Type);
    }


    op(_BINARY_OP_ADD_INT, (left, right -- res)) {
        // TODO constant propagation
        (void)left;
        (void)right;
        res = sym_new_known_type(ctx, &PyLong_Type);
        if (res == NULL) {
            goto out_of_space;
        }
    }

    op(_LOAD_CONST, (-- value)) {
        // There should be no LOAD_CONST. It should be all
        // replaced by peephole_opt.
        Py_UNREACHABLE();
    }

    op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
        value = sym_new_const(ctx, ptr);
        if (value == NULL) {
            goto out_of_space;
        }
    }

    op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
        value = sym_new_const(ctx, ptr);
        if (value == NULL) {
            goto out_of_space;
        }
    }

    op(_LOAD_CONST_INLINE_WITH_NULL, (ptr/4 -- value, null)) {
        value = sym_new_const(ctx, ptr);
        if (value == NULL) {
            goto out_of_space;
        }
        null = sym_new_null(ctx);
        if (null == NULL) {
            goto out_of_space;
        }
    }

    op(_LOAD_CONST_INLINE_BORROW_WITH_NULL, (ptr/4 -- value, null)) {
        value = sym_new_const(ctx, ptr);
        if (value == NULL) {
            goto out_of_space;
        }
        null = sym_new_null(ctx);
        if (null == NULL) {
            goto out_of_space;
        }
    }


    op(_COPY, (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
        assert(oparg > 0);
        top = bottom;
    }

    op(_SWAP, (bottom, unused[oparg-2], top --
        top, unused[oparg-2], bottom)) {
    }

    op(_LOAD_ATTR_INSTANCE_VALUE, (index/1, owner -- attr, null if (oparg & 1))) {
        _LOAD_ATTR_NOT_NULL
        (void)index;
        (void)owner;
    }

    op(_LOAD_ATTR_MODULE, (index/1, owner -- attr, null if (oparg & 1))) {
        _LOAD_ATTR_NOT_NULL
        (void)index;
        (void)owner;
    }

    op(_LOAD_ATTR_WITH_HINT, (hint/1, owner -- attr, null if (oparg & 1))) {
        _LOAD_ATTR_NOT_NULL
        (void)hint;
        (void)owner;
    }

    op(_LOAD_ATTR_SLOT, (index/1, owner -- attr, null if (oparg & 1))) {
        _LOAD_ATTR_NOT_NULL
        (void)index;
        (void)owner;
    }

    op(_LOAD_ATTR_CLASS, (descr/4, owner -- attr, null if (oparg & 1))) {
        _LOAD_ATTR_NOT_NULL
        (void)descr;
        (void)owner;
    }

    op(_CHECK_FUNCTION_EXACT_ARGS, (func_version/2, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        sym_set_type(callable, &PyFunction_Type);
        (void)self_or_null;
        (void)func_version;
    }

    op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null, unused[oparg] -- callable, null, unused[oparg])) {
        sym_set_null(null);
        sym_set_type(callable, &PyMethod_Type);
    }

    op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null, args[oparg] -- new_frame: _Py_UOpsAbstractFrame *)) {
        int argcount = oparg;

        (void)callable;

        PyFunctionObject *func = (PyFunctionObject *)(this_instr + 2)->operand;
        if (func == NULL) {
            goto error;
        }
        PyCodeObject *co = (PyCodeObject *)func->func_code;

        assert(self_or_null != NULL);
        assert(args != NULL);
        if (sym_is_not_null(self_or_null)) {
            // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS in VM
            args--;
            argcount++;
        }

        _Py_UOpsSymType **localsplus_start = ctx->n_consumed;
        int n_locals_already_filled = 0;
        // Can determine statically, so we interleave the new locals
        // and make the current stack the new locals.
        // This also sets up for true call inlining.
        if (sym_is_known(self_or_null)) {
            localsplus_start = args;
            n_locals_already_filled = argcount;
        }
        new_frame = ctx_frame_new(ctx, co, localsplus_start, n_locals_already_filled, 0);
        if (new_frame == NULL){
            goto out_of_space;
        }
    }

    op(_POP_FRAME, (retval -- res)) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        ctx_frame_pop(ctx);
        stack_pointer = ctx->frame->stack_pointer;
        res = retval;
    }

    op(_PUSH_FRAME, (new_frame: _Py_UOpsAbstractFrame * -- unused if (0))) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        ctx->frame = new_frame;
        ctx->curr_frame_depth++;
        stack_pointer = new_frame->stack_pointer;
    }

    op(_UNPACK_SEQUENCE, (seq -- values[oparg])) {
        /* This has to be done manually */
        (void)seq;
        for (int i = 0; i < oparg; i++) {
            values[i] = sym_new_unknown(ctx);
            if (values[i] == NULL) {
                goto out_of_space;
            }
        }
    }

    op(_UNPACK_EX, (seq -- values[oparg & 0xFF], unused, unused[oparg >> 8])) {
        /* This has to be done manually */
        (void)seq;
        int totalargs = (oparg & 0xFF) + (oparg >> 8) + 1;
        for (int i = 0; i < totalargs; i++) {
            values[i] = sym_new_unknown(ctx);
            if (values[i] == NULL) {
                goto out_of_space;
            }
        }
    }

    op(_ITER_NEXT_RANGE, (iter -- iter, next)) {
       next = sym_new_known_type(ctx, &PyLong_Type);
       if (next == NULL) {
           goto out_of_space;
       }
       (void)iter;
    }




// END BYTECODES //

}