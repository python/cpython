#include "Python.h"
#include "pycore_uops.h"
#include "pycore_uop_ids.h"

#define op(name, ...) /* NAME is ignored */

typedef struct _Py_UOpsSymType _Py_UOpsSymType;
typedef struct _Py_UOpsAbstractInterpContext _Py_UOpsAbstractInterpContext;
typedef struct _Py_UOpsAbstractFrame _Py_UOpsAbstractFrame;

extern _Py_UOpsSymType *get_local(_Py_UOpsAbstractFrame *frame, int index);
extern void set_local(_Py_UOpsAbstractFrame *frame, int index, _Py_UOpsSymType *value);

extern _Py_UOpsSymType *new_constant(PyObject *k, _Py_UOpsAbstractInterpContext *ctx);
extern _Py_UOpsSymType *new_null(_Py_UOpsAbstractInterpContext *ctx);
extern PyObject *get_constant(_Py_UOpsSymType *o);
extern PyTypeObject *get_type(_Py_UOpsSymType *o);

extern int is_constant(_Py_UOpsSymType *o);
extern int is_int(_Py_UOpsSymType *o);
extern int is_bool(_Py_UOpsSymType *o);
extern int is_float(_Py_UOpsSymType *o);

extern int is_not_none(_Py_UOpsSymType *o);
extern int is_none(_Py_UOpsSymType *o);

extern _Py_UOpsSymType *new_unknown(_Py_UOpsAbstractInterpContext *ctx);

extern void promote_to_const(_Py_UOpsSymType *o, PyObject *k);
extern void promote_to_type(_Py_UOpsSymType *o, PyTypeObject *t);

extern SpecializerSpace *initialize_space(void *memory, int size);


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
    SpecializerSpace *space;
    _PyUOpInstruction *this_instr;
    _PyBloomFilter *dependencies;
    int modified;

// BEGIN BYTECODES //

    op(_LOAD_FAST_CHECK, (-- value)) {
        value = GETLOCAL(oparg);
        // We guarantee this will error - just bail and don't optimize it.
        if (sym_is_type(value, NULL_TYPE)) {
            goto error;
        }
    }

    op(_LOAD_FAST, (-- value)) {
        value = GETLOCAL(oparg);
    }

    op(_LOAD_FAST_AND_CLEAR, (-- value)) {
        value = GETLOCAL(oparg);
        _Py_UOpsSymType *temp = sym_init_null(ctx);
        if (temp == NULL) {
            goto error;
        }
        GETLOCAL(oparg) = temp;
    }

    op(_STORE_FAST, (value --)) {
        GETLOCAL(oparg) = value;
    }

    op(_STORE_FAST_MAYBE_NULL, (value --)) {
        GETLOCAL(oparg) = value;
    }

    op(_PUSH_NULL, (-- res)) {
        res = sym_init_null(ctx);
        if (res == NULL) {
            goto error;
        };
    }

    op(_GUARD_BOTH_INT, (left, right -- left:  &PYLONG_TYPE, right:  &PYLONG_TYPE)) {
        if (sym_matches_type(left, PYLONG_TYPE, 0) &&
            sym_matches_type(right, PYLONG_TYPE, 0)) {
            REPLACE_OP(_NOP, 0, 0);
        }
    }

    op(_GUARD_BOTH_FLOAT, (left, right -- left: &PYFLOAT_TYPE, right: &PYFLOAT_TYPE)) {
        if (sym_matches_type(left, PYFLOAT_TYPE, 0) &&
            sym_matches_type(right, PYFLOAT_TYPE, 0)) {
            REPLACE_OP(_NOP, 0 ,0);
        }
    }


    op(_BINARY_OP_ADD_INT, (left, right -- res: &PYLONG_TYPE)) {
        // TODO constant propagation
        (void)left;
        (void)right;
        res = sym_init_unknown(ctx);
        if (res == NULL) {
            goto error;
        }
    }

    op(_LOAD_CONST, (-- value)) {
        value = GETITEM(ctx, oparg);
    }

    op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
        _Py_UOpsSymType *sym_const = sym_init_const(ctx, (PyObject *)inst->operand);
        if (sym_const == NULL) {
            goto error;
        }
        value = sym_const;
        assert(is_const(value));
    }

    op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
        _Py_UOpsSymType *sym_const = sym_init_const(ctx, (PyObject *)inst->operand);
        if (sym_const == NULL) {
            goto error;
        }
        value = sym_const;
        assert(is_const(value));
    }

    op(_LOAD_CONST_INLINE_WITH_NULL, (ptr/4 -- value, null)) {
        _Py_UOpsSymType *sym_const = sym_init_const(ctx, (PyObject *)inst->operand);
        if (sym_const == NULL) {
            goto error;
        }
        value = sym_const;
        assert(is_const(value));
        _Py_UOpsSymType *null_sym =  sym_init_null(ctx);
        if (null_sym == NULL) {
            goto error;
        }
        null = null_sym;
    }

    op(_LOAD_CONST_INLINE_BORROW_WITH_NULL, (ptr/4 -- value, null)) {
        _Py_UOpsSymType *sym_const = sym_init_const(ctx, (PyObject *)inst->operand);
        if (sym_const == NULL) {
            goto error;
        }
        value = sym_const;
        assert(is_const(value));
        _Py_UOpsSymType *null_sym =  sym_init_null(ctx);
        if (null_sym == NULL) {
            goto error;
        }
        null = null_sym;
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

        PyFunctionObject *func = extract_func_from_sym(callable);
        if (func == NULL) {
            goto error;
        }
        PyCodeObject *co = (PyCodeObject *)func->func_code;

        assert(self_or_null != NULL);
        assert(args != NULL);
        if (!sym_is_type(self_or_null, NULL_TYPE) &&
            !sym_is_type(self_or_null, SELF_OR_NULL)) {
            // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS in
            // VM
            args--;
            argcount++;
        }

        _Py_UOpsSymType **localsplus_start = ctx->n_consumed;
        int n_locals_already_filled = 0;
        // Can determine statically, so we interleave the new locals
        // and make the current stack the new locals.
        // This also sets up for true call inlining.
        if (!sym_is_type(self_or_null, SELF_OR_NULL)) {
            localsplus_start = args;
            n_locals_already_filled = argcount;
        }
        new_frame = ctx_frame_new(ctx, co, localsplus_start, n_locals_already_filled, 0);
        if (new_frame == NULL){
            goto error;
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

    op(_UNPACK_EX, (seq -- values[oparg & 0xFF], unused, unused[oparg >> 8])) {
        /* This has to be done manually */
        int totalargs = (oparg & 0xFF) + (oparg >> 8) + 1;
        for (int i = 0; i < totalargs; i++) {
            values[i] = sym_init_unknown(ctx);
            if (values[i] == NULL) {
                goto error;
            }
        }
    }

    op(_CHECK_PEP_523, (--)) {
        /* Setting the eval frame function invalidates
         * all executors, so no need to check dynamically */
        if (_PyInterpreterState_GET()->eval_frame == NULL) {
            REPLACE_OP(_NOP, 0, 0);
        }
    }




// END BYTECODES //

}