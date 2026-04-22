#include "Python.h"
#include "pycore_optimizer.h"
#include "pycore_uops.h"
#include "pycore_uop_ids.h"
#include "internal/pycore_moduleobject.h"

#define op(name, ...) /* NAME is ignored */

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
#define sym_get_type_version _Py_uop_sym_get_type_version
#define sym_get_type _Py_uop_sym_get_type
#define sym_has_type _Py_uop_sym_has_type
#define sym_set_null(SYM) _Py_uop_sym_set_null(ctx, SYM)
#define sym_set_non_null(SYM) _Py_uop_sym_set_non_null(ctx, SYM)
#define sym_set_type(SYM, TYPE) _Py_uop_sym_set_type(ctx, SYM, TYPE)
#define sym_set_type_version(SYM, VERSION) _Py_uop_sym_set_type_version(ctx, SYM, VERSION)
#define sym_set_const(SYM, CNST) _Py_uop_sym_set_const(ctx, SYM, CNST)
#define sym_set_compact_int(SYM) _Py_uop_sym_set_compact_int(ctx, SYM)
#define sym_is_bottom _Py_uop_sym_is_bottom
#define frame_new _Py_uop_frame_new
#define frame_new_from_symbol _Py_uop_frame_new_from_symbol
#define frame_pop _Py_uop_frame_pop
#define sym_new_tuple _Py_uop_sym_new_tuple
#define sym_tuple_getitem _Py_uop_sym_tuple_getitem
#define sym_tuple_length _Py_uop_sym_tuple_length
#define sym_is_immortal _Py_uop_symbol_is_immortal
#define sym_new_compact_int _Py_uop_sym_new_compact_int
#define sym_is_compact_int _Py_uop_sym_is_compact_int
#define sym_new_truthiness _Py_uop_sym_new_truthiness
#define sym_new_predicate _Py_uop_sym_new_predicate
#define sym_apply_predicate_narrowing _Py_uop_sym_apply_predicate_narrowing
#define sym_set_recorded_type(SYM, TYPE) _Py_uop_sym_set_recorded_type(ctx, SYM, TYPE)
#define sym_set_recorded_value(SYM, VAL) _Py_uop_sym_set_recorded_value(ctx, SYM, VAL)
#define sym_set_recorded_gen_func(SYM, VAL) _Py_uop_sym_set_recorded_gen_func(ctx, SYM, VAL)
#define sym_get_probable_func_code _Py_uop_sym_get_probable_func_code
#define sym_get_probable_value _Py_uop_sym_get_probable_value
#define sym_get_probable_type _Py_uop_sym_get_probable_type
#define sym_set_stack_depth(DEPTH, SP) _Py_uop_sym_set_stack_depth(ctx, DEPTH, SP)

extern int
optimize_to_bool(
    _PyUOpInstruction *this_instr,
    JitOptContext *ctx,
    JitOptSymbol *value,
    JitOptSymbol **result_ptr,
    uint16_t prefix, uint16_t suffix);

extern void
eliminate_pop_guard(_PyUOpInstruction *this_instr, JitOptContext *ctx, bool exit);

extern PyCodeObject *get_code(_PyUOpInstruction *op);

static int
dummy_func(void) {

    PyCodeObject *co;
    int oparg;
    JitOptSymbol *flag;
    JitOptSymbol *left;
    JitOptSymbol *right;
    JitOptSymbol *value;
    JitOptSymbol *res;
    JitOptSymbol *iter;
    JitOptSymbol *top;
    JitOptSymbol *bottom;
    _Py_UOpsAbstractFrame *frame;
    _Py_UOpsAbstractFrame *new_frame;
    JitOptContext *ctx;
    _PyUOpInstruction *this_instr;
    _PyBloomFilter *dependencies;
    int modified;
    int curr_space;
    int max_space;
    _PyUOpInstruction *first_valid_check_stack;
    _PyUOpInstruction *corresponding_check_stack;

// BEGIN BYTECODES //

    op(_MAKE_HEAP_SAFE, (value -- value)) {
        // eliminate _MAKE_HEAP_SAFE when we *know* the value is immortal
        if (sym_is_immortal(PyJitRef_Unwrap(value))) {
            ADD_OP(_NOP, 0, 0);
        }
        value = PyJitRef_StripBorrowInfo(value);
    }

    op(_COPY_FREE_VARS, (--)) {
        PyCodeObject *co = get_current_code_object(ctx);
        if (co == NULL) {
            ctx->done = true;
            break;
        }
        int offset = co->co_nlocalsplus - oparg;
        for (int i = 0; i < oparg; ++i) {
            ctx->frame->locals[offset + i] = sym_new_not_null(ctx);
        }
    }

    op(_LOAD_FAST_CHECK, (-- value)) {
        value = GETLOCAL(oparg);
        // We guarantee this will error - just bail and don't optimize it.
        if (sym_is_null(value)) {
            ctx->done = true;
        }
        assert(!PyJitRef_IsUnique(value));
    }

    op(_LOAD_FAST, (-- value)) {
        value = GETLOCAL(oparg);
        assert(!PyJitRef_IsUnique(value));
    }

    op(_LOAD_FAST_BORROW, (-- value)) {
        value = PyJitRef_Borrow(GETLOCAL(oparg));
        assert(!PyJitRef_IsUnique(value));
    }

    op(_LOAD_FAST_AND_CLEAR, (-- value)) {
        value = GETLOCAL(oparg);
        JitOptRef temp = sym_new_null(ctx);
        GETLOCAL(oparg) = temp;
        assert(!PyJitRef_IsUnique(value));
    }

    op(_GUARD_TYPE_VERSION_LOCKED, (type_version/2, owner -- owner)) {
        assert(type_version);
        if (sym_matches_type_version(owner, type_version)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            PyTypeObject *probable_type = sym_get_probable_type(owner);
            if (probable_type != NULL &&
                probable_type->tp_version_tag == type_version) {
                // Promote the probable type version to a known one.
                sym_set_type(owner, probable_type);
                sym_set_type_version(owner, type_version);
                if ((probable_type->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0) {
                    PyType_Watch(TYPE_WATCHER_ID, (PyObject *)probable_type);
                    _Py_BloomFilter_Add(dependencies, probable_type);
                }
            }
            else {
                ctx->contradiction = true;
                ctx->done = true;
                break;
            }
        }
    }

    op(_STORE_ATTR_INSTANCE_VALUE, (offset/1, value, owner -- o)) {
        (void)offset;
        (void)value;
        o = owner;
    }

    op(_STORE_ATTR_WITH_HINT, (hint/1, value, owner -- o)) {
        (void)hint;
        (void)value;
        o = owner;
    }

    op(_SWAP_FAST, (value -- trash)) {
        JitOptRef tmp = GETLOCAL(oparg);
        GETLOCAL(oparg) = PyJitRef_RemoveUnique(value);
        trash = tmp;
    }

    op(_STORE_SUBSCR_LIST_INT, (value, list_st, sub_st -- ls, ss)) {
        (void)value;
        ls = list_st;
        ss = sub_st;
    }

    op(_STORE_ATTR_SLOT, (index/1, value, owner -- o)) {
        (void)index;
        (void)value;
        o = owner;
    }

    op(_STORE_SUBSCR_DICT, (value, dict_st, sub -- st)) {
        PyObject *sub_o = sym_get_const(ctx, sub);
        if (sub_o != NULL) {
            optimize_dict_known_hash(ctx, dependencies, this_instr,
                                     sub_o, _STORE_SUBSCR_DICT_KNOWN_HASH);
        }
        (void)value;
        st = dict_st;
    }

    op(_PUSH_NULL, (-- res)) {
        res = PyJitRef_Borrow(sym_new_null(ctx));
    }

    op(_GUARD_TOS_INT, (value -- value)) {
        if (sym_is_compact_int(value)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            if (sym_get_type(value) == &PyLong_Type) {
                ADD_OP(_GUARD_TOS_OVERFLOWED, 0, 0);
            }
            sym_set_compact_int(value);
        }
    }

    op(_GUARD_NOS_INT, (left, unused -- left, unused)) {
        if (sym_is_compact_int(left)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            if (sym_get_type(left) == &PyLong_Type) {
                ADD_OP(_GUARD_NOS_OVERFLOWED, 0, 0);
            }
            sym_set_compact_int(left);
        }
    }

    op(_CHECK_ATTR_CLASS, (type_version/2, owner -- owner)) {
        PyObject *type = (PyObject *)_PyType_LookupByVersion(type_version);
        if (type) {
            if (type == sym_get_const(ctx, owner)) {
                ADD_OP(_NOP, 0, 0);
            }
            else {
                sym_set_const(owner, type);
                if ((((PyTypeObject *)type)->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0) {
                    PyType_Watch(TYPE_WATCHER_ID, type);
                    _Py_BloomFilter_Add(dependencies, type);
                }
            }
        }
    }

    op(_GUARD_TYPE_VERSION, (type_version/2, owner -- owner)) {
        assert(type_version);
        assert(this_instr[-1].opcode == _RECORD_TOS_TYPE);
        if (sym_matches_type_version(owner, type_version)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            PyTypeObject *probable_type = sym_get_probable_type(owner);
            if (probable_type != NULL &&
                probable_type->tp_version_tag == type_version) {
                sym_set_type(owner, probable_type);
                sym_set_type_version(owner, type_version);
                PyType_Watch(TYPE_WATCHER_ID, (PyObject *)probable_type);
                _Py_BloomFilter_Add(dependencies, probable_type);
            }
            else {
                ctx->contradiction = true;
                ctx->done = true;
                break;
            }
        }
    }

    op(_GUARD_TOS_FLOAT, (value -- value)) {
        if (sym_matches_type(value, &PyFloat_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(value, &PyFloat_Type);
    }

    op(_GUARD_NOS_FLOAT, (left, unused -- left, unused)) {
        if (sym_matches_type(left, &PyFloat_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(left, &PyFloat_Type);
    }

    op(_BINARY_OP, (lhs, rhs -- res, l, r)) {
        l = lhs;
        r = rhs;
        REPLACE_OPCODE_IF_EVALUATES_PURE(lhs, rhs, res);
        bool lhs_int = sym_matches_type(lhs, &PyLong_Type);
        bool rhs_int = sym_matches_type(rhs, &PyLong_Type);
        bool lhs_float = sym_matches_type(lhs, &PyFloat_Type);
        bool rhs_float = sym_matches_type(rhs, &PyFloat_Type);
        bool is_truediv = (oparg == NB_TRUE_DIVIDE
                           || oparg == NB_INPLACE_TRUE_DIVIDE);
        bool is_remainder = (oparg == NB_REMAINDER
                             || oparg == NB_INPLACE_REMAINDER);
        // Promote probable-float operands to known floats via speculative
        // guards. _RECORD_TOS_TYPE / _RECORD_NOS_TYPE in the BINARY_OP macro
        // record the observed operand type during tracing, which
        // sym_get_probable_type reads here. Applied only to ops where
        // narrowing unlocks a meaningful downstream win:
        //   - NB_TRUE_DIVIDE: enables the specialized float path below.
        //   - NB_REMAINDER: lets the float result type propagate.
        // NB_POWER is excluded: speculative guards there regressed
        // test_power_type_depends_on_input_values (GH-127844).
        if (is_truediv || is_remainder) {
            if (!sym_has_type(rhs)
                    && sym_get_probable_type(rhs) == &PyFloat_Type) {
                ADD_OP(_GUARD_TOS_FLOAT, 0, 0);
                sym_set_type(rhs, &PyFloat_Type);
                rhs_float = true;
            }
            if (!sym_has_type(lhs)
                    && sym_get_probable_type(lhs) == &PyFloat_Type) {
                ADD_OP(_GUARD_NOS_FLOAT, 0, 0);
                sym_set_type(lhs, &PyFloat_Type);
                lhs_float = true;
            }
        }
        if (is_truediv && lhs_float && rhs_float) {
            if (PyJitRef_IsUnique(lhs)) {
                ADD_OP(_BINARY_OP_TRUEDIV_FLOAT_INPLACE, 0, 0);
                l = sym_new_null(ctx);
                r = rhs;
            }
            else if (PyJitRef_IsUnique(rhs)) {
                ADD_OP(_BINARY_OP_TRUEDIV_FLOAT_INPLACE_RIGHT, 0, 0);
                l = lhs;
                r = sym_new_null(ctx);
            }
            else {
                ADD_OP(_BINARY_OP_TRUEDIV_FLOAT, 0, 0);
                l = lhs;
                r = rhs;
            }
            res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
        }
        else if (is_truediv
                && (lhs_int || lhs_float) && (rhs_int || rhs_float)) {
            res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
        }
        else if (!((lhs_int || lhs_float) && (rhs_int || rhs_float))) {
            // There's something other than an int or float involved:
            res = sym_new_unknown(ctx);
        }
        else if (oparg == NB_POWER || oparg == NB_INPLACE_POWER) {
            // This one's fun... the *type* of the result depends on the
            // *values* being exponentiated. However, exponents with one
            // constant part are reasonably common, so it's probably worth
            // trying to infer some simple cases:
            // - A: 1 ** 1 -> 1 (int ** int -> int)
            // - B: 1 ** -1 -> 1.0 (int ** int -> float)
            // - C: 1.0 ** 1 -> 1.0 (float ** int -> float)
            // - D: 1 ** 1.0 -> 1.0 (int ** float -> float)
            // - E: -1 ** 0.5 ~> 1j (int ** float -> complex)
            // - F: 1.0 ** 1.0 -> 1.0 (float ** float -> float)
            // - G: -1.0 ** 0.5 ~> 1j (float ** float -> complex)
            if (rhs_float) {
                // Case D, E, F, or G... can't know without the sign of the LHS
                // or whether the RHS is whole, which isn't worth the effort:
                res = sym_new_unknown(ctx);
            }
            else if (lhs_float) {
                // Case C:
                res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
            }
            else if (!sym_is_const(ctx, rhs)) {
                // Case A or B... can't know without the sign of the RHS:
                res = sym_new_unknown(ctx);
            }
            else if (_PyLong_IsNegative((PyLongObject *)sym_get_const(ctx, rhs))) {
                // Case B:
                res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
            }
            else {
                // Case A:
                res = sym_new_type(ctx, &PyLong_Type);
            }
        }
        else if (lhs_int && rhs_int) {
            res = sym_new_type(ctx, &PyLong_Type);
        }
        else {
            res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
        }
    }

    op(_BINARY_OP_ADD_INT, (left, right -- res, l, r)) {
        if (PyJitRef_IsUnique(left)) {
            REPLACE_OP(this_instr, _BINARY_OP_ADD_INT_INPLACE, 0, 0);
        }
        else if (PyJitRef_IsUnique(right)) {
            REPLACE_OP(this_instr, _BINARY_OP_ADD_INT_INPLACE_RIGHT, 0, 0);
        }
        // Result may be a unique compact int or a cached small int
        // at runtime. Mark as unique; inplace ops verify at runtime.
        res = PyJitRef_MakeUnique(sym_new_compact_int(ctx));
        l = left;
        r = right;
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, res);
    }

    op(_BINARY_OP_SUBTRACT_INT, (left, right -- res, l, r)) {
        if (PyJitRef_IsUnique(left)) {
            REPLACE_OP(this_instr, _BINARY_OP_SUBTRACT_INT_INPLACE, 0, 0);
        }
        else if (PyJitRef_IsUnique(right)) {
            REPLACE_OP(this_instr, _BINARY_OP_SUBTRACT_INT_INPLACE_RIGHT, 0, 0);
        }
        res = PyJitRef_MakeUnique(sym_new_compact_int(ctx));
        l = left;
        r = right;
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, res);
    }

    op(_BINARY_OP_MULTIPLY_INT, (left, right -- res, l, r)) {
        if (PyJitRef_IsUnique(left)) {
            REPLACE_OP(this_instr, _BINARY_OP_MULTIPLY_INT_INPLACE, 0, 0);
        }
        else if (PyJitRef_IsUnique(right)) {
            REPLACE_OP(this_instr, _BINARY_OP_MULTIPLY_INT_INPLACE_RIGHT, 0, 0);
        }
        res = PyJitRef_MakeUnique(sym_new_compact_int(ctx));
        l = left;
        r = right;
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, res);
    }

    op(_BINARY_OP_ADD_FLOAT, (left, right -- res, l, r)) {
        if (PyJitRef_IsUnique(left)) {
            ADD_OP(_BINARY_OP_ADD_FLOAT_INPLACE, 0, 0);
            l = PyJitRef_Borrow(sym_new_null(ctx));
            r = right;
        }
        else if (PyJitRef_IsUnique(right)) {
            ADD_OP(_BINARY_OP_ADD_FLOAT_INPLACE_RIGHT, 0, 0);
            l = left;
            r = PyJitRef_Borrow(sym_new_null(ctx));
        }
        else {
            l = left;
            r = right;
        }
        res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
    }

    op(_BINARY_OP_SUBTRACT_FLOAT, (left, right -- res, l, r)) {
        if (PyJitRef_IsUnique(left)) {
            ADD_OP(_BINARY_OP_SUBTRACT_FLOAT_INPLACE, 0, 0);
            l = PyJitRef_Borrow(sym_new_null(ctx));
            r = right;
        }
        else if (PyJitRef_IsUnique(right)) {
            ADD_OP(_BINARY_OP_SUBTRACT_FLOAT_INPLACE_RIGHT, 0, 0);
            l = left;
            r = PyJitRef_Borrow(sym_new_null(ctx));
        }
        else {
            l = left;
            r = right;
        }
        res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
    }

    op(_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res, l, r)) {
        if (PyJitRef_IsUnique(left)) {
            ADD_OP(_BINARY_OP_MULTIPLY_FLOAT_INPLACE, 0, 0);
            l = PyJitRef_Borrow(sym_new_null(ctx));
            r = right;
        }
        else if (PyJitRef_IsUnique(right)) {
            ADD_OP(_BINARY_OP_MULTIPLY_FLOAT_INPLACE_RIGHT, 0, 0);
            l = left;
            r = PyJitRef_Borrow(sym_new_null(ctx));
        }
        else {
            l = left;
            r = right;
        }
        res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
    }

    op(_BINARY_OP_ADD_UNICODE, (left, right -- res, l, r)) {
        res = sym_new_type(ctx, &PyUnicode_Type);
        l = left;
        r = right;
    }

    op(_GUARD_BINARY_OP_EXTEND_LHS, (descr/4, left, right -- left, right)) {
        _PyBinaryOpSpecializationDescr *d = (_PyBinaryOpSpecializationDescr *)descr;
        assert(d != NULL && d->guard == NULL && d->lhs_type != NULL);
        if (sym_matches_type(left, d->lhs_type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(left, d->lhs_type);
    }

    op(_GUARD_BINARY_OP_EXTEND_RHS, (descr/4, left, right -- left, right)) {
        _PyBinaryOpSpecializationDescr *d = (_PyBinaryOpSpecializationDescr *)descr;
        assert(d != NULL && d->guard == NULL && d->rhs_type != NULL);
        if (sym_matches_type(right, d->rhs_type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(right, d->rhs_type);
    }

    op(_GUARD_BINARY_OP_EXTEND, (descr/4, left, right -- left, right)) {
        _PyBinaryOpSpecializationDescr *d = (_PyBinaryOpSpecializationDescr *)descr;
        if (d != NULL && d->guard == NULL) {
            /* guard == NULL means the check is purely a type test against
               lhs_type/rhs_type, so eliminate it when types are already known. */
            assert(d->lhs_type != NULL && d->rhs_type != NULL);
            bool lhs_known = sym_matches_type(left, d->lhs_type);
            bool rhs_known = sym_matches_type(right, d->rhs_type);
            if (lhs_known && rhs_known) {
                ADD_OP(_NOP, 0, 0);
            }
            else if (lhs_known) {
                ADD_OP(_GUARD_BINARY_OP_EXTEND_RHS, 0, (uintptr_t)d);
                sym_set_type(right, d->rhs_type);
            }
            else if (rhs_known) {
                ADD_OP(_GUARD_BINARY_OP_EXTEND_LHS, 0, (uintptr_t)d);
                sym_set_type(left, d->lhs_type);
            }
        }
    }

    op(_BINARY_OP_EXTEND, (descr/4, left, right -- res, l, r)) {
        _PyBinaryOpSpecializationDescr *d = (_PyBinaryOpSpecializationDescr *)descr;
        if (d != NULL && d->result_type != NULL) {
            res = sym_new_type(ctx, d->result_type);
            if (d->result_unique) {
                res = PyJitRef_MakeUnique(res);
            }
        }
        else {
            res = sym_new_not_null(ctx);
        }
        l = left;
        r = right;
    }

    op(_BINARY_OP_INPLACE_ADD_UNICODE, (left, right -- res)) {
        if (sym_is_const(ctx, left) && sym_is_const(ctx, right)) {
            assert(PyUnicode_CheckExact(sym_get_const(ctx, left)));
            assert(PyUnicode_CheckExact(sym_get_const(ctx, right)));
            PyObject *temp = PyUnicode_Concat(sym_get_const(ctx, left), sym_get_const(ctx, right));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
        }
        else {
            res = sym_new_type(ctx, &PyUnicode_Type);
        }
        GETLOCAL(this_instr->operand0) = sym_new_null(ctx);
    }

    op(_BINARY_OP_SUBSCR_CHECK_FUNC, (container, unused -- container, unused, getitem)) {
        getitem = sym_new_not_null(ctx);
        PyTypeObject *tp = sym_get_type(container);
        if (tp == NULL) {
            PyObject *c = sym_get_probable_value(container);
            if (c != NULL) {
                tp = Py_TYPE(c);
            }
        }
        if (tp != NULL) {
            PyObject *getitem_o = ((PyHeapTypeObject *)tp)->_spec_cache.getitem;
            sym_set_recorded_value(getitem, getitem_o);
        }
    }

    op(_BINARY_OP_SUBSCR_INIT_CALL, (container, sub, getitem -- new_frame)) {
        _Py_UOpsAbstractFrame *f = frame_new_from_symbol(ctx, getitem, NULL, 0);
        if (f == NULL) {
            break;
        }
        f->locals[0] = container;
        f->locals[1] = sub;
        new_frame = PyJitRef_WrapInvalid(f);
    }

    op(_BINARY_OP_SUBSCR_STR_INT, (str_st, sub_st -- res, s, i)) {
        res = sym_new_type(ctx, &PyUnicode_Type);
        s = str_st;
        i = sub_st;
    }

    op(_BINARY_OP_SUBSCR_USTR_INT, (str_st, sub_st -- res, s, i)) {
        res = sym_new_type(ctx, &PyUnicode_Type);
        s = str_st;
        i = sub_st;
    }

    op(_GUARD_BINARY_OP_SUBSCR_TUPLE_INT_BOUNDS, (tuple_st, sub_st -- tuple_st, sub_st)) {
        assert(sym_matches_type(tuple_st, &PyTuple_Type));
        if (sym_is_const(ctx, sub_st)) {
            assert(PyLong_CheckExact(sym_get_const(ctx, sub_st)));
            long index = PyLong_AsLong(sym_get_const(ctx, sub_st));
            assert(index >= 0);
            Py_ssize_t tuple_length = sym_tuple_length(tuple_st);
            if (tuple_length != -1 && index < tuple_length) {
                ADD_OP(_NOP, 0, 0);
            }
        }
    }

    op(_BINARY_OP_SUBSCR_TUPLE_INT, (tuple_st, sub_st -- res, ts, ss)) {
        assert(sym_matches_type(tuple_st, &PyTuple_Type));
        if (sym_is_const(ctx, sub_st)) {
            assert(PyLong_CheckExact(sym_get_const(ctx, sub_st)));
            long index = PyLong_AsLong(sym_get_const(ctx, sub_st));
            assert(index >= 0);
            Py_ssize_t tuple_length = sym_tuple_length(tuple_st);
            if (tuple_length == -1) {
                // Unknown length
                res = sym_new_not_null(ctx);
            }
            else {
                assert(index < tuple_length);
                res = sym_tuple_getitem(ctx, tuple_st, index);
            }
        }
        else {
            res = sym_new_not_null(ctx);
        }
        ts = tuple_st;
        ss = sub_st;
    }

    op(_BINARY_OP_SUBSCR_DICT, (dict_st, sub_st -- res, ds, ss)) {
        res = sym_new_not_null(ctx);
        ds = dict_st;
        ss = sub_st;
        PyObject *sub = sym_get_const(ctx, sub_st);
        if (sym_is_not_container(sub_st) &&
            sym_matches_type(dict_st, &PyFrozenDict_Type)) {
            REPLACE_OPCODE_IF_EVALUATES_PURE(dict_st, sub_st, res);
        }
        else if (sub != NULL) {
            optimize_dict_known_hash(ctx, dependencies, this_instr,
                                     sub, _BINARY_OP_SUBSCR_DICT_KNOWN_HASH);
        }
    }

    op(_BINARY_OP_SUBSCR_LIST_SLICE, (list_st, sub_st -- res, ls, ss)) {
        res = sym_new_type(ctx, &PyList_Type);
        ls = list_st;
        ss = sub_st;
    }

    op(_TO_BOOL, (value -- res)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res,
                                            _POP_TOP, _NOP);
        if (!already_bool) {
            res = sym_new_truthiness(ctx, value, true);
        }
    }

    op(_TO_BOOL_BOOL, (value -- value)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &value,
                                            _POP_TOP, _NOP);
        if (!already_bool) {
            sym_set_type(value, &PyBool_Type);
        }
    }

    op(_TO_BOOL_INT, (value -- res, v)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res,
                                            _NOP, _SWAP);
        if (!already_bool) {
            sym_set_type(value, &PyLong_Type);
            res = sym_new_truthiness(ctx, value, true);
        }
        v = value;
    }

    op(_TO_BOOL_LIST, (value -- res, v)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res,
                                            _NOP, _SWAP);
        if (!already_bool) {
            res = sym_new_type(ctx, &PyBool_Type);
        }
        v = value;
    }

    op(_TO_BOOL_NONE, (value -- res)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res,
                                            _POP_TOP, _NOP);
        if (!already_bool) {
            sym_set_const(value, Py_None);
            res = sym_new_const(ctx, Py_False);
        }
    }

    op(_GUARD_NOS_UNICODE, (nos, unused -- nos, unused)) {
        if (sym_matches_type(nos, &PyUnicode_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(nos, &PyUnicode_Type);
    }

    op(_GUARD_NOS_COMPACT_ASCII, (nos, unused -- nos, unused)) {
        sym_set_type(nos, &PyUnicode_Type);
    }

    op(_GUARD_TOS_UNICODE, (value -- value)) {
        if (sym_matches_type(value, &PyUnicode_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(value, &PyUnicode_Type);
    }

    op(_TO_BOOL_STR, (value -- res, v)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res,
                                            _NOP, _SWAP);
        v = value;
        if (!already_bool) {
            res = sym_new_truthiness(ctx, value, true);
        }
    }

    op(_UNARY_NOT, (value -- res)) {
        REPLACE_OPCODE_IF_EVALUATES_PURE(value, res);
        sym_set_type(value, &PyBool_Type);
        res = sym_new_truthiness(ctx, value, false);
    }

    op(_UNARY_NEGATIVE, (value -- res, v)) {
        v = value;
        REPLACE_OPCODE_IF_EVALUATES_PURE(value, res);
        if (sym_matches_type(value, &PyFloat_Type) && PyJitRef_IsUnique(value)) {
            ADD_OP(_UNARY_NEGATIVE_FLOAT_INPLACE, 0, 0);
            v = PyJitRef_Borrow(sym_new_null(ctx));
            res = PyJitRef_MakeUnique(sym_new_type(ctx, &PyFloat_Type));
        }
        else if (sym_is_compact_int(value)) {
            res = sym_new_compact_int(ctx);
        }
        else {
            PyTypeObject *type = sym_get_type(value);
            if (type == &PyLong_Type || type == &PyFloat_Type) {
                res = sym_new_type(ctx, type);
            }
            else {
                res = sym_new_not_null(ctx);
            }
        }
    }

    op(_UNARY_INVERT, (value -- res, v)) {
        v = value;
        // Required to avoid a warning due to the deprecation of bitwise inversion of bools
        if (!sym_matches_type(value, &PyBool_Type)) {
            REPLACE_OPCODE_IF_EVALUATES_PURE(value, res);
        }
        if (sym_matches_type(value, &PyLong_Type)) {
            res = sym_new_type(ctx, &PyLong_Type);
        }
        else {
            res = sym_new_not_null(ctx);
        }
    }

    op(_COMPARE_OP, (left, right -- res)) {
        // Comparison between bytes and str or int is not impacted by this optimization as bytes
        // is not a safe type (due to its ability to raise a warning during comparisons).
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, res);
        if (oparg & 16) {
            res = sym_new_type(ctx, &PyBool_Type);
        }
        else {
            res = _Py_uop_sym_new_not_null(ctx);
        }
    }

    op(_COMPARE_OP_INT, (left, right -- res, l, r)) {
        int cmp_mask = oparg & (COMPARE_LT_MASK | COMPARE_GT_MASK | COMPARE_EQ_MASK);

        if (cmp_mask == COMPARE_EQ_MASK) {
            res = sym_new_predicate(ctx, left, right, JIT_PRED_EQ);
        }
        else if (cmp_mask == (COMPARE_LT_MASK | COMPARE_GT_MASK)) {
            res = sym_new_predicate(ctx, left, right, JIT_PRED_NE);
        }
        else {
            res = sym_new_type(ctx, &PyBool_Type);
        }
        l = left;
        r = right;
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, res);
    }

    op(_COMPARE_OP_FLOAT, (left, right -- res, l, r)) {
        int cmp_mask = oparg & (COMPARE_LT_MASK | COMPARE_GT_MASK | COMPARE_EQ_MASK);

        if (cmp_mask == COMPARE_EQ_MASK) {
            res = sym_new_predicate(ctx, left, right, JIT_PRED_EQ);
        }
        else if (cmp_mask == (COMPARE_LT_MASK | COMPARE_GT_MASK)) {
            res = sym_new_predicate(ctx, left, right, JIT_PRED_NE);
        }
        else {
            res = sym_new_type(ctx, &PyBool_Type);
        }
        l = left;
        r = right;
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, res);
    }

    op(_COMPARE_OP_STR, (left, right -- res, l, r)) {
        /* Cannot use predicate optimization here, as `a == b`
         * does not imply that `a` is equivalent to `b`. `a` may be
         * mortal, while `b` is immortal */
        res = sym_new_type(ctx, &PyBool_Type);
        l = left;
        r = right;
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, res);
    }

    op(_IS_OP, (left, right -- b, l, r)) {
        b = sym_new_predicate(ctx, left, right, (oparg ? JIT_PRED_IS_NOT : JIT_PRED_IS));
        l = left;
        r = right;
    }

    op(_IS_NONE, (value -- b)) {
        if (sym_is_const(ctx, value)) {
            PyObject *value_o = sym_get_const(ctx, value);
            b = sym_new_const(ctx, Py_IsNone(value_o) ? Py_True : Py_False);
        }
        else {
            b = sym_new_type(ctx, &PyBool_Type);
        }
    }

    op(_CONTAINS_OP, (left, right -- b, l, r)) {
        b = sym_new_type(ctx, &PyBool_Type);
        l = left;
        r = right;
        REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, b);
    }

    op(_CONTAINS_OP_SET, (left, right -- b, l, r)) {
        b = sym_new_type(ctx, &PyBool_Type);
        l = left;
        r = right;
        if (sym_is_not_container(left) &&
            sym_matches_type(right, &PyFrozenSet_Type)) {
            REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, b);
        }
    }

    op(_CONTAINS_OP_DICT, (left, right -- b, l, r)) {
        b = sym_new_type(ctx, &PyBool_Type);
        l = left;
        r = right;
        if (sym_is_not_container(left) &&
            sym_matches_type(right, &PyFrozenDict_Type)) {
            REPLACE_OPCODE_IF_EVALUATES_PURE(left, right, b);
        }
    }

    op(_LOAD_CONST, (-- value)) {
        PyCodeObject *co = get_current_code_object(ctx);
        PyObject *val = PyTuple_GET_ITEM(co->co_consts, oparg);
        ADD_OP(_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)val);
        value = PyJitRef_Borrow(sym_new_const(ctx, val));
    }

    op(_LOAD_COMMON_CONSTANT, (-- value)) {
        assert(oparg < NUM_COMMON_CONSTANTS);
        PyObject *val = _PyInterpreterState_GET()->common_consts[oparg];
        ADD_OP(_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)val);
        value = PyJitRef_Borrow(sym_new_const(ctx, val));
    }

    op(_LOAD_SMALL_INT, (-- value)) {
        PyObject *val = PyLong_FromLong(oparg);
        assert(val);
        assert(_Py_IsImmortal(val));
        ADD_OP(_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)val);
        value = PyJitRef_Borrow(sym_new_const(ctx, val));
    }

    op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
        value = sym_new_const(ctx, ptr);
    }

    op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
        value = PyJitRef_Borrow(sym_new_const(ctx, ptr));
    }

    op(_POP_TOP_OPARG, (args[oparg] --)) {
        for (int i = oparg-1; i >= 0; i--) {
            optimize_pop_top(ctx, this_instr, args[i]);
        }
    }

    op(_POP_TOP, (value -- )) {
        optimize_pop_top(ctx, this_instr, value);
    }

    op(_POP_TOP_INT, (value --)) {
        if (PyJitRef_IsBorrowed(value)) {
            ADD_OP(_POP_TOP_NOP, 0, 0);
        }
    }

    op(_POP_TOP_FLOAT, (value --)) {
        if (PyJitRef_IsBorrowed(value)) {
            ADD_OP(_POP_TOP_NOP, 0, 0);
        }
    }

    op(_POP_TOP_UNICODE, (value --)) {
        if (PyJitRef_IsBorrowed(value)) {
            ADD_OP(_POP_TOP_NOP, 0, 0);
        }
    }

    op(_COPY, (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
        assert(oparg > 0);
        bottom = PyJitRef_RemoveUnique(bottom);
        top = bottom;
    }

    op(_SWAP, (bottom, unused[oparg-2], top -- bottom, unused[oparg-2], top)) {
        JitOptRef temp = bottom;
        bottom = top;
        top = temp;
        assert(oparg >= 2);
    }

    op(_LOAD_ATTR_INSTANCE_VALUE, (offset/1, owner -- attr, o)) {
        attr = sym_new_not_null(ctx);
        (void)offset;
        o = owner;
    }

    op(_LOAD_ATTR_MODULE, (dict_version/2, index/1, owner -- attr, o)) {
        (void)dict_version;
        (void)index;
        attr = PyJitRef_NULL;
        if (sym_is_const(ctx, owner)) {
            PyModuleObject *mod = (PyModuleObject *)sym_get_const(ctx, owner);
            if (PyModule_CheckExact(mod)) {
                PyObject *dict = mod->md_dict;
                uint64_t watched_mutations = get_mutations(dict);
                if (watched_mutations < _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, dict);
                    _Py_BloomFilter_Add(dependencies, dict);
                    PyObject *res = convert_global_to_const(this_instr, dict);
                    if (res == NULL) {
                        attr = sym_new_not_null(ctx);
                    }
                    else {
                        bool immortal = _Py_IsImmortal(res);
                        ADD_OP(immortal ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE,
                               0, (uintptr_t)res);
                        ADD_OP(_SWAP, 2, 0);
                        attr = sym_new_const(ctx, res);
                    }

                }
            }
        }
        if (PyJitRef_IsNull(attr)) {
            /* No conversion made. We don't know what `attr` is. */
            attr = sym_new_not_null(ctx);
        }
        o = owner;
    }

    op (_PUSH_NULL_CONDITIONAL, ( -- null[oparg & 1])) {
        if (oparg & 1) {
            ADD_OP(_PUSH_NULL, 0, 0);
            null[0] = sym_new_null(ctx);
        }
        else {
            ADD_OP(_NOP, 0, 0);
        }
    }

    op(_LOAD_ATTR, (owner -- attr, self_or_null[oparg&1])) {
        (void)owner;
        attr = sym_new_not_null(ctx);
        if (oparg & 1) {
            self_or_null[0] = sym_new_unknown(ctx);
        }
    }

    op(_LOAD_ATTR_WITH_HINT, (hint/1, owner -- attr, o)) {
        attr = sym_new_not_null(ctx);
        (void)hint;
        o = owner;
    }

    op(_LOAD_ATTR_SLOT, (index/1, owner -- attr, o)) {
        attr = sym_new_not_null(ctx);
        (void)index;
        o = owner;
    }

    op(_LOAD_ATTR_CLASS, (descr/4, owner -- attr)) {
        (void)descr;
        PyTypeObject *type = (PyTypeObject *)sym_get_const(ctx, owner);
        PyObject *name = get_co_name(ctx, oparg >> 1);
        attr = lookup_attr(ctx, dependencies, this_instr, type, name,
                           _POP_TOP, _NOP);
    }

    op(_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES, (descr/4, owner -- attr)) {
        (void)descr;
        PyTypeObject *type = sym_get_type(owner);
        PyObject *name = get_co_name(ctx, oparg >> 1);
        attr = lookup_attr(ctx, dependencies, this_instr, type, name,
                           _POP_TOP, _NOP);
    }

    op(_LOAD_ATTR_NONDESCRIPTOR_NO_DICT, (descr/4, owner -- attr)) {
        (void)descr;
        PyTypeObject *type = sym_get_type(owner);
        PyObject *name = get_co_name(ctx, oparg >> 1);
        attr = lookup_attr(ctx, dependencies, this_instr, type, name,
                           _POP_TOP, _NOP);
    }

    op(_LOAD_ATTR_METHOD_WITH_VALUES, (descr/4, owner -- attr, self)) {
        (void)descr;
        PyTypeObject *type = sym_get_type(owner);
        PyObject *name = get_co_name(ctx, oparg >> 1);
        attr = lookup_attr(ctx, dependencies, this_instr, type, name,
                           _NOP, _SWAP);
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_NO_DICT, (descr/4, owner -- attr, self)) {
        (void)descr;
        PyTypeObject *type = sym_get_type(owner);
        PyObject *name = get_co_name(ctx, oparg >> 1);
        attr = lookup_attr(ctx, dependencies, this_instr, type, name,
                           _NOP, _SWAP);
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_LAZY_DICT, (descr/4, owner -- attr, self)) {
        (void)descr;
        PyTypeObject *type = sym_get_type(owner);
        PyObject *name = get_co_name(ctx, oparg >> 1);
        attr = lookup_attr(ctx, dependencies, this_instr, type, name,
                           _NOP, _SWAP);
        self = owner;
    }

    op(_GUARD_LOAD_SUPER_ATTR_METHOD, (global_super_st, class_st, unused -- global_super_st, class_st, unused)) {
        if (sym_get_const(ctx, global_super_st) == (PyObject *)&PySuper_Type) {
            PyTypeObject *probable = (PyTypeObject *)sym_get_probable_value(class_st);
            PyTypeObject *known = (PyTypeObject *)sym_get_const(ctx, class_st);
            // not known, but has a probable type, promote the probable type
            if (known == NULL && probable != NULL && PyType_Check(probable)) {
                ADD_OP(_GUARD_NOS_TYPE_VERSION, 0, probable->tp_version_tag);
                known = probable;
            }
            sym_set_const(class_st, (PyObject *)known);
        }
        else {
            sym_set_const(global_super_st, (PyObject *)&PySuper_Type);
            sym_set_type(class_st, &PyType_Type);
        }
    }

    op(_LOAD_SUPER_ATTR_METHOD, (global_super_st, class_st, self_st -- attr, self_or_null)) {
        self_or_null = self_st;
        PyTypeObject *su_type = (PyTypeObject *)sym_get_const(ctx, class_st);
        PyTypeObject *obj_type = sym_get_type(self_st);
        PyObject *name = get_co_name(ctx, oparg >> 2);
        attr = lookup_super_attr(ctx, dependencies, this_instr,
                                 su_type, obj_type, name,
                                 _LOAD_CONST_INLINE_BORROW,
                                 _LOAD_CONST_INLINE, _SWAP);
    }

    op(_LOAD_ATTR_PROPERTY_FRAME, (func_version/2, fget/4, owner -- new_frame)) {
        PyFunctionObject *func = (PyFunctionObject *)fget;
        if (sym_get_type_version(owner) == 0 ||
            func->func_version != func_version) {
            ctx->contradiction = true;
            ctx->done = true;
            break;
        }
        _Py_BloomFilter_Add(dependencies, fget);
        PyCodeObject *co = (PyCodeObject *)func->func_code;
        _Py_UOpsAbstractFrame *f = frame_new(ctx, co, NULL, 0);
        if (f == NULL) {
            break;
        }
        f->locals[0] = owner;
        f->func = func;
        new_frame = PyJitRef_WrapInvalid(f);
    }

    op(_LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN_FRAME, (func_version/2, getattribute/4, owner -- new_frame)) {
        PyFunctionObject *func = (PyFunctionObject *)getattribute;
        if (sym_get_type_version(owner) == 0 ||
            func->func_version != func_version) {
            ctx->contradiction = true;
            ctx->done = true;
            break;
        }
        _Py_BloomFilter_Add(dependencies, func);
        PyCodeObject *co = (PyCodeObject *)func->func_code;
        _Py_UOpsAbstractFrame *f = frame_new(ctx, co, NULL, 0);
        if (f == NULL) {
            break;
        }
        PyObject *name = get_co_name(ctx, oparg >> 1);
        f->locals[0] = owner;
        f->locals[1] = sym_new_const(ctx, name);
        f->func = func;
        new_frame = PyJitRef_WrapInvalid(f);
    }

    op(_INIT_CALL_BOUND_METHOD_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        PyObject *bound_method = sym_get_probable_value(callable);
        if (bound_method != NULL && Py_TYPE(bound_method) == &PyMethod_Type) {
            PyMethodObject *method = (PyMethodObject *)bound_method;
            callable = sym_new_not_null(ctx);
            sym_set_recorded_value(callable, method->im_func);
            self_or_null = sym_new_not_null(ctx);
            sym_set_recorded_value(self_or_null, method->im_self);
        }
        else {
            callable = sym_new_not_null(ctx);
            self_or_null = sym_new_not_null(ctx);
        }
    }

    op(_CHECK_FUNCTION_VERSION, (func_version/2, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        PyObject *func = sym_get_probable_value(callable);
        if (func == NULL || !PyFunction_Check(func) || ((PyFunctionObject *)func)->func_version != func_version) {
            ctx->contradiction = true;
            ctx->done = true;
            break;
        }
        // Guarded on this, so it can be promoted.
        sym_set_const(callable, func);
        _Py_BloomFilter_Add(dependencies, func);
    }

    op(_CHECK_FUNCTION_VERSION_KW, (func_version/2, callable, unused, unused[oparg], unused -- callable, unused, unused[oparg], unused)) {
        PyObject *func = sym_get_probable_value(callable);
        if (func == NULL || !PyFunction_Check(func) || ((PyFunctionObject *)func)->func_version != func_version) {
            ctx->contradiction = true;
            ctx->done = true;
            break;
        }
        // Guarded on this, so it can be promoted.
        sym_set_const(callable, func);
        _Py_BloomFilter_Add(dependencies, func);
    }

    op(_CHECK_METHOD_VERSION, (func_version/2, callable, null, unused[oparg] -- callable, null, unused[oparg])) {
        if (sym_is_const(ctx, callable) && sym_matches_type(callable, &PyMethod_Type)) {
            PyMethodObject *method = (PyMethodObject *)sym_get_const(ctx, callable);
            assert(PyMethod_Check(method));
            ADD_OP(_CHECK_FUNCTION_VERSION_INLINE, 0, func_version);
            uop_buffer_last(&ctx->out_buffer)->operand1 = (uintptr_t)method->im_func;
        }
        else {
            // Guarding on the bound method, safe to promote.
            PyObject *bound_method = sym_get_probable_value(callable);
            if (bound_method != NULL && Py_TYPE(bound_method) == &PyMethod_Type) {
                PyMethodObject *method = (PyMethodObject *)bound_method;
                PyObject *func = method->im_func;
                if (PyFunction_Check(func) &&
                    ((PyFunctionObject *)func)->func_version == func_version) {
                    _Py_BloomFilter_Add(dependencies, func);
                    sym_set_const(callable, bound_method);
                }
            }
        }
        sym_set_type(callable, &PyMethod_Type);
    }

    op(_CHECK_METHOD_VERSION_KW, (func_version/2, callable, null, unused[oparg], unused -- callable, null, unused[oparg], unused)) {
        if (sym_is_const(ctx, callable) && sym_matches_type(callable, &PyMethod_Type)) {
            PyMethodObject *method = (PyMethodObject *)sym_get_const(ctx, callable);
            assert(PyMethod_Check(method));
            ADD_OP(_CHECK_FUNCTION_VERSION_INLINE, 0, func_version);
            uop_buffer_last(&ctx->out_buffer)->operand1 = (uintptr_t)method->im_func;
        }
        else {
            // Guarding on the bound method, safe to promote.
            PyObject *bound_method = sym_get_probable_value(callable);
            if (bound_method != NULL && Py_TYPE(bound_method) == &PyMethod_Type) {
                PyMethodObject *method = (PyMethodObject *)bound_method;
                PyObject *func = method->im_func;
                if (PyFunction_Check(func) &&
                    ((PyFunctionObject *)func)->func_version == func_version) {
                    _Py_BloomFilter_Add(dependencies, func);
                    sym_set_const(callable, bound_method);
                }
            }
        }
        sym_set_type(callable, &PyMethod_Type);
    }

    op(_CHECK_FUNCTION_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        assert(sym_matches_type(callable, &PyFunction_Type));
        if (sym_is_const(ctx, callable)) {
            if (sym_is_null(self_or_null) || sym_is_not_null(self_or_null)) {
                PyFunctionObject *func = (PyFunctionObject *)sym_get_const(ctx, callable);
                PyCodeObject *co = (PyCodeObject *)func->func_code;
                if (co->co_argcount == oparg + sym_is_not_null(self_or_null)) {
                    ADD_OP(_NOP, 0 ,0);
                }
            }
        }
    }

    op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null, unused[oparg] -- callable, null, unused[oparg])) {
        sym_set_null(null);
        sym_set_type(callable, &PyMethod_Type);
    }

    op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null, args[oparg] -- new_frame)) {
        int argcount = oparg;
        assert(!PyJitRef_IsNull(self_or_null));
        assert(args != NULL);
        if (sym_is_not_null(self_or_null)) {
            // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS in VM
            args--;
            argcount++;
        }

        if (sym_is_null(self_or_null) || sym_is_not_null(self_or_null)) {
            new_frame = PyJitRef_WrapInvalid(frame_new_from_symbol(ctx, callable, args, argcount));
        } else {
            new_frame = PyJitRef_WrapInvalid(frame_new_from_symbol(ctx, callable, NULL, 0));
        }
    }

    op(_EXPAND_METHOD, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        if (sym_is_const(ctx, callable) && sym_matches_type(callable, &PyMethod_Type)) {
            PyMethodObject *method = (PyMethodObject *)sym_get_const(ctx, callable);
            callable = sym_new_const(ctx, method->im_func);
            self_or_null = sym_new_const(ctx, method->im_self);
        }
        else {
            callable = sym_new_not_null(ctx);
            self_or_null = sym_new_not_null(ctx);
        }
    }

    op(_EXPAND_METHOD_KW, (callable, self_or_null, unused[oparg], unused -- callable, self_or_null, unused[oparg], unused)) {
        if (sym_is_const(ctx, callable) && sym_matches_type(callable, &PyMethod_Type)) {
            PyMethodObject *method = (PyMethodObject *)sym_get_const(ctx, callable);
            callable = sym_new_const(ctx, method->im_func);
            self_or_null = sym_new_const(ctx, method->im_self);
        }
        else {
            callable = sym_new_not_null(ctx);
            self_or_null = sym_new_not_null(ctx);
        }
    }

    op(_MAYBE_EXPAND_METHOD, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        (void)args;
        callable = sym_new_not_null(ctx);
        self_or_null = sym_new_not_null(ctx);
    }

    op(_PY_FRAME_GENERAL, (callable, self_or_null, args[oparg] -- new_frame)) {
        new_frame = PyJitRef_WrapInvalid(frame_new_from_symbol(ctx, callable, NULL, 0));
    }

    op(_PY_FRAME_KW, (callable, self_or_null, args[oparg], kwnames -- new_frame)) {
        new_frame = PyJitRef_WrapInvalid(frame_new_from_symbol(ctx, callable, NULL, 0));
    }

    op(_PY_FRAME_EX, (func_st, null, callargs_st, kwargs_st -- ex_frame)) {
        ex_frame = PyJitRef_WrapInvalid(frame_new_from_symbol(ctx, func_st, NULL, 0));
    }

    op(_CHECK_OBJECT, (type_version/2, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        PyObject *probable_callable = sym_get_probable_value(callable);
        assert(probable_callable != NULL);
        PyObject *const_callable = sym_get_const(ctx, callable);
        bool is_probable = const_callable == NULL && probable_callable != NULL;
        PyObject *callable_o = const_callable != NULL ? const_callable : probable_callable;
        if (sym_is_null(self_or_null) &&
            callable_o != NULL &&
            PyType_Check(callable_o) &&
            ((PyTypeObject *)callable_o)->tp_version_tag == type_version) {
            // Probable types need the guard.
            if (!is_probable) {
                ADD_OP(_NOP, 0, 0);
            }
            else {
                // Promote the probable type, as we have
                // guarded on it.
                sym_set_const(callable, callable_o);
            }
            PyHeapTypeObject *cls = (PyHeapTypeObject *)callable_o;
            PyObject *init = cls->_spec_cache.init;
            assert(init != NULL);
            assert(PyFunction_Check(init));
            callable = sym_new_const(ctx, init);
            PyType_Watch(TYPE_WATCHER_ID, callable_o);
            _Py_BloomFilter_Add(dependencies, callable_o);;
        }
        else {
            callable = sym_new_not_null(ctx);
        }
    }

    op(_ALLOCATE_OBJECT, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        self_or_null = sym_new_not_null(ctx);
    }

    op(_CREATE_INIT_FRAME, (init, self, args[oparg] -- init_frame)) {
        ctx->frame->stack_pointer = stack_pointer - oparg - 2;
        _Py_UOpsAbstractFrame *shim = frame_new(ctx, (PyCodeObject *)&_Py_InitCleanup, NULL, 0);
        if (shim == NULL) {
            break;
        }
        /* Push self onto stack of shim */
        shim->stack_pointer[0] = self;
        shim->stack_pointer++;
        assert((int)(shim->stack_pointer - shim->stack) == 1);
        ctx->frame = shim;
        ctx->curr_frame_depth++;
        assert((this_instr + 1)->opcode == _PUSH_FRAME);
        init_frame = PyJitRef_WrapInvalid(frame_new_from_symbol(ctx, init, args-1, oparg+1));
    }

    op(_RETURN_VALUE, (retval -- res)) {
        JitOptRef temp = retval;
        DEAD(retval);
        SAVE_STACK();
        ctx->frame->stack_pointer = stack_pointer;
        assert(this_instr[1].opcode == _RECORD_CODE);
        PyCodeObject *returning_code = (PyCodeObject *)this_instr[1].operand0;
        assert(PyCode_Check(returning_code));
        if (returning_code == NULL) {
            ctx->done = true;
            break;
        }
        if (frame_pop(ctx, returning_code)) {
            break;
        }
        stack_pointer = ctx->frame->stack_pointer;

        RELOAD_STACK();
        res = temp;
    }

    op(_RETURN_GENERATOR, ( -- res)) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        assert(this_instr[1].opcode == _RECORD_CODE);
        PyCodeObject *returning_code = (PyCodeObject *)this_instr[1].operand0;
        if (returning_code == NULL) {
            ctx->done = true;
            break;
        }
        assert(PyCode_Check(returning_code));
        if (frame_pop(ctx, returning_code)) {
            break;
        }
        stack_pointer = ctx->frame->stack_pointer;
        res = sym_new_unknown(ctx);
    }

    op(_YIELD_VALUE, (retval -- value)) {
        JitOptRef temp = retval;
        DEAD(retval);
        SAVE_STACK();
        ctx->frame->stack_pointer = stack_pointer;
        assert(this_instr[1].opcode == _RECORD_CODE);
        PyCodeObject *returning_code = (PyCodeObject *)this_instr[1].operand0;
        if (returning_code == NULL) {
            ctx->done = true;
            break;
        }
        assert(PyCode_Check(returning_code));
        if (frame_pop(ctx, returning_code)) {
            break;
        }
        stack_pointer = ctx->frame->stack_pointer;
        RELOAD_STACK();
        value = temp;
    }

    op(_GET_ITER, (iterable -- iter, index_or_null)) {
        bool is_coro = false;
        bool is_trad = false; // has `tp_iter` slot
        bool definite = true;
        PyTypeObject *tp = sym_get_type(iterable);
        if (tp == NULL) {
            definite = false;
            tp = sym_get_probable_type(iterable);
        }
        if (oparg == GET_ITER_YIELD_FROM_NO_CHECK) {
            if (tp == &PyCoro_Type) {
                if (!definite) {
                    ADD_OP(_GUARD_TYPE, 0, (uintptr_t)tp);
                    sym_set_type(iterable, tp);
                }
                ADD_OP(_PUSH_NULL, 0, 0);
                is_coro = true;
            }
        }
        if (tp != NULL &&
            tp->_tp_iteritem == NULL &&
            tp->tp_iter != NULL &&
            tp->tp_iter != PyObject_SelfIter &&
            tp->tp_flags & Py_TPFLAGS_IMMUTABLETYPE
        ) {
            assert(tp != &PyCoro_Type);
            is_trad = true;
            if (!definite) {
                ADD_OP(_GUARD_TYPE, 0, (uintptr_t)tp);
                sym_set_type(iterable, tp);
            }
            ADD_OP(_GET_ITER_TRAD, 0, 0);
        }
        if (is_coro) {
            assert(!is_trad);
            iter = iterable;
            index_or_null = sym_new_null(ctx);
        }
        else if (is_trad) {
            iter = sym_new_not_null(ctx);
            index_or_null = sym_new_null(ctx);
        }
        else {
            iter = sym_new_not_null(ctx);
            index_or_null = sym_new_unknown(ctx);
        }
    }

    op(_GUARD_ITERATOR, (iterable -- iterable)) {
        bool definite = true;
        PyTypeObject *tp = sym_get_type(iterable);
        if (tp == NULL) {
            definite = false;
            tp = sym_get_probable_type(iterable);
        }
        if (tp != NULL && tp->tp_iter == PyObject_SelfIter) {
            if (definite) {
                ADD_OP(_NOP, 0, 0);
            }
            else {
                ADD_OP(_GUARD_TYPE, 0, (uintptr_t)tp);
                sym_set_type(iterable, tp);
            }
        }
    }

    op(_GUARD_ITER_VIRTUAL, (iterable -- iterable)) {
        bool definite = true;
        PyTypeObject *tp = sym_get_type(iterable);
        if (tp == NULL) {
            definite = false;
            tp = sym_get_probable_type(iterable);
        }
        if (tp != NULL && tp->_tp_iteritem != NULL) {
            if (definite) {
                ADD_OP(_NOP, 0, 0);
            }
            else {
                ADD_OP(_GUARD_TYPE, 0, (uintptr_t)tp);
                sym_set_type(iterable, tp);
            }
        }
    }

    op(_FOR_ITER_GEN_FRAME, (iter, unused -- iter, unused, gen_frame)) {
        _Py_UOpsAbstractFrame *new_frame = frame_new_from_symbol(ctx, iter, NULL, 0);
        if (new_frame == NULL) {
            ctx->done = true;
            break;
        }
        new_frame->stack_pointer[0] = sym_new_const(ctx, Py_None);
        new_frame->stack_pointer++;
        gen_frame = PyJitRef_WrapInvalid(new_frame);
    }

    op(_SEND_GEN_FRAME, (receiver, null, v -- receiver, null, gen_frame)) {
        _Py_UOpsAbstractFrame *new_frame = frame_new_from_symbol(ctx, receiver, NULL, 0);
        if (new_frame == NULL) {
            ctx->done = true;
            break;
        }
        new_frame->stack_pointer[0] = PyJitRef_StripReferenceInfo(v);
        new_frame->stack_pointer++;
        gen_frame = PyJitRef_WrapInvalid(new_frame);
    }

    op(_CHECK_STACK_SPACE, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
        PyCodeObject *co = sym_get_probable_func_code(callable);
        if (co == NULL) {
            ctx->done = true;
            break;
        }
        ADD_OP(_CHECK_STACK_SPACE_OPERAND, 0, co->co_framesize);
    }

    op (_CHECK_STACK_SPACE_OPERAND, (framesize/2 -- )) {
        (void)framesize;
    }

    op(_CHECK_IS_NOT_PY_CALLABLE, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
        PyTypeObject *type = sym_get_type(callable);
        if (type && type != &PyFunction_Type && type != &PyMethod_Type) {
            ADD_OP(_NOP, 0, 0);
        }
    }

    op(_CHECK_IS_NOT_PY_CALLABLE_EX, (func_st, unused, unused, unused -- func_st, unused, unused, unused)) {
        PyTypeObject *type = sym_get_type(func_st);
        if (type && type != &PyFunction_Type) {
            ADD_OP(_NOP, 0, 0);
        }
    }

    op(_CHECK_IS_NOT_PY_CALLABLE_KW, (callable, unused, unused[oparg], unused -- callable, unused, unused[oparg], unused)) {
        PyTypeObject *type = sym_get_type(callable);
        if (type && type != &PyFunction_Type && type != &PyMethod_Type) {
            ADD_OP(_NOP, 0, 0);
        }
    }

    op(_PUSH_FRAME, (new_frame -- )) {
        SYNC_SP();
        if (!CURRENT_FRAME_IS_INIT_SHIM()) {
            ctx->frame->stack_pointer = stack_pointer;
        }
        ctx->frame->caller = true;
        ctx->frame = (_Py_UOpsAbstractFrame *)PyJitRef_Unwrap(new_frame);
        ctx->curr_frame_depth++;
        stack_pointer = ctx->frame->stack_pointer;
        assert(ctx->frame->locals != NULL);
    }

    op(_UNPACK_SEQUENCE, (seq -- values[oparg], top[0])) {
        (void)top;
        /* This has to be done manually */
        for (int i = 0; i < oparg; i++) {
            values[i] = sym_new_unknown(ctx);
        }
    }

    op(_UNPACK_EX, (seq -- values[oparg & 0xFF], unused, unused[oparg >> 8], top[0])) {
        (void)top;
        /* This has to be done manually */
        int totalargs = (oparg & 0xFF) + (oparg >> 8) + 1;
        for (int i = 0; i < totalargs; i++) {
            values[i] = sym_new_unknown(ctx);
        }
    }

    op(_ITER_CHECK_TUPLE, (iter, null_or_index -- iter, null_or_index)) {
        if (sym_matches_type(iter, &PyTuple_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(iter, &PyTuple_Type);
    }

    op(_ITER_CHECK_LIST, (iter, null_or_index -- iter, null_or_index)) {
        if (sym_matches_type(iter, &PyList_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(iter, &PyList_Type);
        }
    }

    op(_ITER_CHECK_RANGE, (iter, null_or_index -- iter, null_or_index)) {
        if (sym_matches_type(iter, &PyRangeIter_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(iter, &PyRangeIter_Type);
        }
    }

    op(_ITER_NEXT_RANGE, (iter, null_or_index -- iter, null_or_index, next)) {
       next = sym_new_type(ctx, &PyLong_Type);
    }

    op(_CALL_TYPE_1, (callable, null, arg -- res, a)) {
        PyObject* type = (PyObject *)sym_get_type(arg);
        if (type) {
            res = sym_new_const(ctx, type);
            ADD_OP(_SWAP, 3, 0);
            optimize_pop_top(ctx, this_instr, callable);
            optimize_pop_top(ctx, this_instr, null);
            ADD_OP(_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)type);
            ADD_OP(_SWAP, 2, 0);
        }
        else {
            res = sym_new_not_null(ctx);
        }
        a = arg;
    }

    op(_CALL_STR_1, (unused, unused, arg -- res, a)) {
        if (sym_matches_type(arg, &PyUnicode_Type)) {
            // e.g. str('foo') or str(foo) where foo is known to be a string
            // Note: we must strip the reference information because it goes
            // through str() which strips the reference information from it.
            res = PyJitRef_StripReferenceInfo(arg);
        }
        else {
            res = sym_new_type(ctx, &PyUnicode_Type);
        }
        a = arg;
    }

    op(_CALL_ISINSTANCE, (callable, null, instance, cls -- res)) {
        // the result is always a bool, but sometimes we can
        // narrow it down to True or False
        res = sym_new_type(ctx, &PyBool_Type);
        PyTypeObject *inst_type = sym_get_type(instance);
        PyTypeObject *cls_o = (PyTypeObject *)sym_get_const(ctx, cls);
        if (inst_type && cls_o && sym_matches_type(cls, &PyType_Type)) {
            // isinstance(inst, cls) where both inst and cls have
            // known types, meaning we can deduce either True or False

            // The below check is equivalent to PyObject_TypeCheck(inst, cls)
            PyObject *out = Py_False;
            if (inst_type == cls_o || PyType_IsSubtype(inst_type, cls_o)) {
                out = Py_True;
            }
            sym_set_const(res, out);
            optimize_pop_top(ctx, this_instr, cls);
            optimize_pop_top(ctx, this_instr, instance);
            optimize_pop_top(ctx, this_instr, null);
            optimize_pop_top(ctx, this_instr, callable);
            ADD_OP(_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)out);
        }
    }

    op(_CALL_LIST_APPEND, (callable, self, arg -- none, c, s)) {
        (void)(arg);
        c = callable;
        s = self;
        none = sym_new_const(ctx, Py_None);
    }

    op(_GUARD_CALLABLE_BUILTIN_CLASS, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyType_Type)) {
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            if (tp->tp_vectorcall != NULL) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyType_Type);
        }
    }

    op(_GUARD_CALLABLE_BUILTIN_O, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyCFunction_Type) &&
            (sym_is_not_null(self_or_null) || sym_is_null(self_or_null))) {
            int total_args = oparg;
            if (sym_is_not_null(self_or_null)) {
                total_args++;
            }
            if (total_args == 1 && PyCFunction_GET_FLAGS(callable_o) == METH_O) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyCFunction_Type);
        }
    }

    op(_GUARD_CALLABLE_BUILTIN_FAST, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyCFunction_Type)) {
            if (PyCFunction_GET_FLAGS(callable_o) == METH_FASTCALL) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyCFunction_Type);
        }
    }

    op(_GUARD_CALLABLE_BUILTIN_FAST_WITH_KEYWORDS, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyCFunction_Type)) {
            if (PyCFunction_GET_FLAGS(callable_o) == (METH_FASTCALL | METH_KEYWORDS)) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyCFunction_Type);
        }
    }

    op(_CALL_BUILTIN_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        callable = sym_new_not_null(ctx);
    }

    op(_CALL_BUILTIN_O, (callable, self_or_null, args[oparg] -- res, c, s)) {
        res = sym_new_not_null(ctx);
        c = callable;
        if (sym_is_not_null(self_or_null)) {
            args--;
            s = args[0];
        }
        else if (sym_is_null(self_or_null)) {
            s = args[0];
        }
        else {
            s = sym_new_unknown(ctx);
        }
    }

    op(_CALL_BUILTIN_FAST, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        callable = sym_new_not_null(ctx);
    }

    op(_CALL_BUILTIN_CLASS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        callable = sym_new_not_null(ctx);
    }

    op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_O, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyMethodDescr_Type) &&
            (sym_is_not_null(self_or_null) || sym_is_null(self_or_null))) {
            int total_args = oparg;
            if (sym_is_not_null(self_or_null)) {
                total_args++;
            }
            PyTypeObject *self_type = NULL;
            if (sym_is_not_null(self_or_null)) {
                self_type = sym_get_type(self_or_null);
            }
            else {
                self_type = sym_get_type(args[0]);
            }
            PyTypeObject *d_type = ((PyMethodDescrObject *)callable_o)->d_common.d_type;
            if (total_args == 2 &&
                ((PyMethodDescrObject *)callable_o)->d_method->ml_flags == METH_O &&
                self_type == d_type) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyMethodDescr_Type);
        }
    }

    op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyMethodDescr_Type) &&
            (sym_is_not_null(self_or_null) || sym_is_null(self_or_null))) {
            int total_args = oparg;
            if (sym_is_not_null(self_or_null)) {
                total_args++;
            }
            PyTypeObject *self_type = NULL;
            if (sym_is_not_null(self_or_null)) {
                self_type = sym_get_type(self_or_null);
            }
            else {
                self_type = sym_get_type(args[0]);
            }
            PyTypeObject *d_type = ((PyMethodDescrObject *)callable_o)->d_common.d_type;
            if (total_args != 0 &&
                ((PyMethodDescrObject *)callable_o)->d_method->ml_flags == (METH_FASTCALL|METH_KEYWORDS) &&
                self_type == d_type) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyMethodDescr_Type);
        }
    }

    op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_NOARGS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyMethodDescr_Type) &&
            (sym_is_not_null(self_or_null) || sym_is_null(self_or_null))) {
            int total_args = oparg;
            if (sym_is_not_null(self_or_null)) {
                total_args++;
            }
            PyTypeObject *self_type = NULL;
            if (sym_is_not_null(self_or_null)) {
                self_type = sym_get_type(self_or_null);
            }
            else {
                self_type = sym_get_type(args[0]);
            }
            PyTypeObject *d_type = ((PyMethodDescrObject *)callable_o)->d_common.d_type;
            if (total_args == 1 &&
                ((PyMethodDescrObject *)callable_o)->d_method->ml_flags == METH_NOARGS &&
                self_type == d_type) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyMethodDescr_Type);
        }
    }

    op(_CHECK_RECURSION_LIMIT, ( -- )) {
        if (ctx->frame->is_c_recursion_checked) {
            ADD_OP(_NOP, 0, 0);
        }
        ctx->frame->is_c_recursion_checked = true;
    }

    op(_CALL_METHOD_DESCRIPTOR_NOARGS, (callable, self_or_null, args[oparg] -- res, c, s)) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && Py_IS_TYPE(callable_o, &PyMethodDescr_Type)
            && sym_is_not_null(self_or_null)) {
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            PyCFunction cfunc = method->d_method->ml_meth;
            ADD_OP(_CALL_METHOD_DESCRIPTOR_NOARGS_INLINE, oparg + 1, (uintptr_t)cfunc);
        }
        res = sym_new_not_null(ctx);
        c = callable;
        if (sym_is_not_null(self_or_null)) {
            args--;
            s = args[0];
        }
        else if (sym_is_null(self_or_null)) {
            s = args[0];
        }
        else {
            s = sym_new_unknown(ctx);
        }
    }

    op(_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && Py_IS_TYPE(callable_o, &PyMethodDescr_Type)
            && sym_is_not_null(self_or_null)) {
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            PyCFunction cfunc = method->d_method->ml_meth;
            ADD_OP(_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS_INLINE, oparg, (uintptr_t)cfunc);
        }
        callable = sym_new_not_null(ctx);
    }

    op(_CALL_METHOD_DESCRIPTOR_FAST, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && Py_IS_TYPE(callable_o, &PyMethodDescr_Type)
            && sym_is_not_null(self_or_null)) {
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            PyCFunction cfunc = method->d_method->ml_meth;
            ADD_OP(_CALL_METHOD_DESCRIPTOR_FAST_INLINE, oparg, (uintptr_t)cfunc);
        }
        callable = sym_new_not_null(ctx);
    }

    op(_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && sym_matches_type(callable, &PyMethodDescr_Type) &&
            (sym_is_not_null(self_or_null) || sym_is_null(self_or_null))) {
            int total_args = oparg;
            if (sym_is_not_null(self_or_null)) {
                total_args++;
            }
            PyTypeObject *self_type = NULL;
            if (sym_is_not_null(self_or_null)) {
                self_type = sym_get_type(self_or_null);
            }
            else {
                self_type = sym_get_type(args[0]);
            }
            PyTypeObject *d_type = ((PyMethodDescrObject *)callable_o)->d_common.d_type;
            if (total_args != 0 &&
                ((PyMethodDescrObject *)callable_o)->d_method->ml_flags == METH_FASTCALL &&
                self_type == d_type) {
                ADD_OP(_NOP, 0, 0);
            }
        }
        else {
            sym_set_type(callable, &PyMethodDescr_Type);
        }
    }

    op(_CALL_METHOD_DESCRIPTOR_O, (callable, self_or_null, args[oparg] -- res, c, s, a)) {
        PyObject *callable_o = sym_get_const(ctx, callable);
        if (callable_o && Py_IS_TYPE(callable_o, &PyMethodDescr_Type)
            && sym_is_not_null(self_or_null)) {
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            PyCFunction cfunc = method->d_method->ml_meth;
            ADD_OP(_CALL_METHOD_DESCRIPTOR_O_INLINE, oparg + 1, (uintptr_t)cfunc);
        }
        res = sym_new_not_null(ctx);
        c = callable;
        if (sym_is_not_null(self_or_null)) {
            args--;
            s = args[0];
            a = args[1];
        }
        else {
            s = sym_new_unknown(ctx);
            a = sym_new_unknown(ctx);
        }
    }

    op(_CALL_INTRINSIC_1, (value -- res, v)) {
        res = sym_new_not_null(ctx);
        v = value;
    }

    op(_CALL_INTRINSIC_2, (value2_st, value1_st -- res, vs1, vs2)) {
        res = sym_new_not_null(ctx);
        vs1 = value1_st;
        vs2 = value2_st;
    }

    op(_GUARD_IS_TRUE_POP, (flag -- )) {
        sym_apply_predicate_narrowing(ctx, flag, true);

        if (sym_is_const(ctx, flag)) {
            PyObject *value = sym_get_const(ctx, flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, ctx, value != Py_True);
        }
        else {
            int bit = get_test_bit_for_bools();
            if (bit) {
                REPLACE_OP(this_instr,
                    test_bit_set_in_true(bit) ?
                    _GUARD_BIT_IS_SET_POP :
                    _GUARD_BIT_IS_UNSET_POP, bit, 0);
            }
        }
        sym_set_const(flag, Py_True);
    }

    op(_GUARD_IS_FALSE_POP, (flag -- )) {
        sym_apply_predicate_narrowing(ctx, flag, false);

        if (sym_is_const(ctx, flag)) {
            PyObject *value = sym_get_const(ctx, flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, ctx, value != Py_False);
        }
        else {
            int bit = get_test_bit_for_bools();
            if (bit) {
                REPLACE_OP(this_instr,
                    test_bit_set_in_true(bit) ?
                    _GUARD_BIT_IS_UNSET_POP :
                    _GUARD_BIT_IS_SET_POP, bit, 0);
            }
        }
        sym_set_const(flag, Py_False);
    }

    op(_GUARD_IS_NONE_POP, (val -- )) {
        if (sym_is_const(ctx, val)) {
            PyObject *value = sym_get_const(ctx, val);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, ctx, !Py_IsNone(value));
        }
        else if (sym_has_type(val)) {
            assert(!sym_matches_type(val, &_PyNone_Type));
            eliminate_pop_guard(this_instr, ctx, true);
        }
        sym_set_const(val, Py_None);
    }

    op(_GUARD_IS_NOT_NONE_POP, (val -- )) {
        if (sym_is_const(ctx, val)) {
            PyObject *value = sym_get_const(ctx, val);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, ctx, Py_IsNone(value));
        }
        else if (sym_has_type(val)) {
            assert(!sym_matches_type(val, &_PyNone_Type));
            eliminate_pop_guard(this_instr, ctx, false);
        }
    }

    op(_CHECK_PEP_523, (--)) {
        /* Setting the eval frame function invalidates
        * all executors, so no need to check dynamically */
        if (_PyInterpreterState_GET()->eval_frame == NULL) {
            ADD_OP(_NOP, 0 ,0);
        }
    }

    op(_INSERT_NULL, (self -- method_and_self[2])) {
        method_and_self[0] = sym_new_null(ctx);
        method_and_self[1] = self;
    }

    op(_LOAD_SPECIAL, (method_and_self[2] -- method_and_self[2])) {
        bool optimized = false;
        PyTypeObject *type = sym_get_probable_type(method_and_self[1]);
        if (type != NULL) {
            PyObject *name = _Py_SpecialMethods[oparg].name;
            PyObject *descr = _PyType_Lookup(type, name);
            if (descr != NULL && (Py_TYPE(descr)->tp_flags & Py_TPFLAGS_METHOD_DESCRIPTOR)) {
                ADD_OP(_GUARD_TYPE_VERSION, 0, type->tp_version_tag);
                bool immortal = _Py_IsImmortal(descr) || (type->tp_flags & Py_TPFLAGS_IMMUTABLETYPE);
                ADD_OP(immortal ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE,
                       0, (uintptr_t)descr);
                ADD_OP(_SWAP, 3, 0);
                optimize_pop_top(ctx, this_instr, method_and_self[0]);
                if ((type->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0) {
                    PyType_Watch(TYPE_WATCHER_ID, (PyObject *)type);
                    _Py_BloomFilter_Add(dependencies, type);
                }
                method_and_self[0] = sym_new_const(ctx, descr);
                optimized = true;
            }
        }
        if (!optimized) {
            method_and_self[0] = sym_new_not_null(ctx);
            method_and_self[1] = sym_new_unknown(ctx);
        }
    }

    op(_JUMP_TO_TOP, (--)) {
        ctx->done = true;
    }

    op(_EXIT_TRACE, (exit_p/4 --)) {
        (void)exit_p;
        ctx->done = true;
    }

    op(_DEOPT, (--)) {
        ctx->done = true;
    }

    op(_REPLACE_WITH_TRUE, (value -- res, v)) {
        ADD_OP(_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)Py_True);
        ADD_OP(_SWAP, 2, 0);
        res = sym_new_const(ctx, Py_True);
        v = value;
    }

    op(_BUILD_TUPLE, (values[oparg] -- tup)) {
        tup = sym_new_tuple(ctx, oparg, values);
        tup = PyJitRef_MakeUnique(tup);
    }

    op(_BUILD_LIST, (values[oparg] -- list)) {
        list = sym_new_type(ctx, &PyList_Type);
    }

    op(_BUILD_SLICE, (args[oparg] -- slice)) {
        slice = sym_new_type(ctx, &PySlice_Type);
    }

    op(_BUILD_MAP, (values[oparg*2] -- map)) {
        map = sym_new_type(ctx, &PyDict_Type);
    }

    op(_BUILD_STRING, (pieces[oparg] -- str)) {
        str = sym_new_type(ctx, &PyUnicode_Type);
    }

    op(_BUILD_SET, (values[oparg] -- set)) {
        set = sym_new_type(ctx, &PySet_Type);
    }

    op(_SET_UPDATE, (set, unused[oparg-1], iterable -- set, unused[oparg-1], i)) {
        (void)set;
        i = iterable;
    }

    op(_LIST_EXTEND, (list_st, unused[oparg-1], iterable_st -- list_st, unused[oparg-1], i)) {
        (void)list_st;
        i = iterable_st;
    }

    op(_DICT_MERGE, (callable, unused, unused, dict, unused[oparg - 1], update -- callable, unused, unused, dict, unused[oparg - 1], u)) {
        (void)callable;
        (void)dict;
        u = update;
    }

    op(_UNPACK_SEQUENCE_TWO_TUPLE, (seq -- val1, val0)) {
        if (PyJitRef_IsUnique(seq) && sym_tuple_length(seq) == 2) {
            ADD_OP(_UNPACK_SEQUENCE_UNIQUE_TWO_TUPLE, oparg, 0);
        }
        val0 = sym_tuple_getitem(ctx, seq, 0);
        val1 = sym_tuple_getitem(ctx, seq, 1);
    }

    op(_UNPACK_SEQUENCE_TUPLE, (seq -- values[oparg])) {
        if (PyJitRef_IsUnique(seq) && sym_tuple_length(seq) == 3) {
            ADD_OP(_UNPACK_SEQUENCE_UNIQUE_THREE_TUPLE, oparg, 0);
        }
        else if (PyJitRef_IsUnique(seq) && sym_tuple_length(seq) == oparg) {
            ADD_OP(_UNPACK_SEQUENCE_UNIQUE_TUPLE, oparg, 0);
        }
        for (int i = 0; i < oparg; i++) {
            values[i] = sym_tuple_getitem(ctx, seq, oparg - i - 1);
        }
    }

    op(_CALL_TUPLE_1, (callable, null, arg -- res, a)) {
        if (sym_matches_type(arg, &PyTuple_Type)) {
            // e.g. tuple((1, 2)) or tuple(foo) where foo is known to be a tuple
            // Note: we must strip the reference information because it goes
            // through tuple() which strips the reference information from it.
            res = PyJitRef_StripReferenceInfo(arg);
        }
        else {
            res = sym_new_type(ctx, &PyTuple_Type);
        }
        a = arg;
    }

    op(_GUARD_TOS_LIST, (tos -- tos)) {
        if (sym_matches_type(tos, &PyList_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(tos, &PyList_Type);
        }
    }

    op(_GUARD_NOS_LIST, (nos, unused -- nos, unused)) {
        if (sym_matches_type(nos, &PyList_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(nos, &PyList_Type);
        }
    }

    op(_GUARD_TOS_TUPLE, (tos -- tos)) {
        if (sym_matches_type(tos, &PyTuple_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(tos, &PyTuple_Type);
        }
    }

    op(_GUARD_NOS_TUPLE, (nos, unused -- nos, unused)) {
        if (sym_matches_type(nos, &PyTuple_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(nos, &PyTuple_Type);
        }
    }

    op(_GUARD_NOS_DICT, (nos, unused -- nos, unused)) {
        if (sym_matches_type(nos, &PyDict_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        sym_set_type(nos, &PyDict_Type);
    }

    op(_GUARD_NOS_ANY_DICT, (nos, unused -- nos, unused)) {
        PyTypeObject *tp = sym_get_type(nos);
        if (tp == &PyDict_Type || tp == &PyFrozenDict_Type) {
            ADD_OP(_NOP, 0, 0);
        }
    }

    op(_GUARD_TOS_ANY_DICT, (tos -- tos)) {
        PyTypeObject *tp = sym_get_type(tos);
        if (tp == &PyDict_Type || tp == &PyFrozenDict_Type) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            // Narrowing the guard based on the probable type.
            tp = sym_get_probable_type(tos);
            if (tp == &PyDict_Type) {
                ADD_OP(_GUARD_TOS_DICT, 0, 0);
            }
            else if (tp == &PyFrozenDict_Type) {
                ADD_OP(_GUARD_TOS_FROZENDICT, 0, 0);
            }
        }
    }

    op(_GUARD_TOS_DICT, (tos -- tos)) {
        if (sym_matches_type(tos, &PyDict_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(tos, &PyDict_Type);
        }
    }

    op(_GUARD_TOS_FROZENDICT, (tos -- tos)) {
        if (sym_matches_type(tos, &PyFrozenDict_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(tos, &PyFrozenDict_Type);
        }
    }

    op(_GUARD_TOS_ANY_SET, (tos -- tos)) {
        PyTypeObject *tp = sym_get_type(tos);
        if (tp == &PySet_Type || tp == &PyFrozenSet_Type) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            // Narrowing the guard based on the probable type.
            tp = sym_get_probable_type(tos);
            if (tp == &PySet_Type) {
                ADD_OP(_GUARD_TOS_SET, 0, 0);
            }
            else if (tp == &PyFrozenSet_Type) {
                ADD_OP(_GUARD_TOS_FROZENSET, 0, 0);
            }
        }
    }

    op(_GUARD_TOS_SET, (tos -- tos)) {
        if (sym_matches_type(tos, &PySet_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(tos, &PySet_Type);
        }
    }

    op(_GUARD_TOS_FROZENSET, (tos -- tos)) {
        if (sym_matches_type(tos, &PyFrozenSet_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(tos, &PyFrozenSet_Type);
        }
    }

    op(_GUARD_TOS_SLICE, (tos -- tos)) {
        if (sym_matches_type(tos, &PySlice_Type)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_type(tos, &PySlice_Type);
        }
    }

    op(_GUARD_NOS_NULL, (null, unused -- null, unused)) {
        if (sym_is_null(null)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_null(null);
        }
    }

    op(_GUARD_NOS_NOT_NULL, (nos, unused -- nos, unused)) {
        if (sym_is_not_null(nos)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_non_null(nos);
        }
    }

    op(_GUARD_THIRD_NULL, (null, unused, unused -- null, unused, unused)) {
        if (sym_is_null(null)) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_null(null);
        }
    }

    op(_GUARD_CALLABLE_TYPE_1, (callable, unused, unused -- callable, unused, unused)) {
        if (sym_get_const(ctx, callable) == (PyObject *)&PyType_Type) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_const(callable, (PyObject *)&PyType_Type);
        }
    }

    op(_GUARD_CALLABLE_TUPLE_1, (callable, unused, unused -- callable, unused, unused)) {
        if (sym_get_const(ctx, callable) == (PyObject *)&PyTuple_Type) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_const(callable, (PyObject *)&PyTuple_Type);
        }
    }

    op(_GUARD_CALLABLE_STR_1, (callable, unused, unused -- callable, unused, unused)) {
        if (sym_get_const(ctx, callable) == (PyObject *)&PyUnicode_Type) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_const(callable, (PyObject *)&PyUnicode_Type);
        }
    }

    op(_CALL_LEN, (callable, null, arg -- res, a, c)) {
        res = sym_new_type(ctx, &PyLong_Type);
        Py_ssize_t length = sym_tuple_length(arg);

        // Not a tuple, check if it's a const string
        if (length < 0 && sym_is_const(ctx, arg)) {
            PyObject *const_val = sym_get_const(ctx, arg);
            if (const_val != NULL) {
                if (PyUnicode_CheckExact(const_val)) {
                    length = PyUnicode_GET_LENGTH(const_val);
                }
                else if (PyBytes_CheckExact(const_val)) {
                    length = PyBytes_GET_SIZE(const_val);
                }
            }
        }

        if (length >= 0) {
            PyObject *temp = PyLong_FromSsize_t(length);
            if (temp == NULL) {
                goto error;
            }
            if (_Py_IsImmortal(temp)) {
                ADD_OP(_SHUFFLE_3_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)temp);
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
        }
        a = arg;
        c = callable;
    }

    op(_GET_LEN, (obj -- obj, len)) {
        Py_ssize_t tuple_length = sym_tuple_length(obj);
        if (tuple_length == -1) {
            len = sym_new_type(ctx, &PyLong_Type);
        }
        else {
            assert(tuple_length >= 0);
            PyObject *temp = PyLong_FromSsize_t(tuple_length);
            if (temp == NULL) {
                goto error;
            }
            if (_Py_IsImmortal(temp)) {
                ADD_OP(_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)temp);
            }
            len = sym_new_const(ctx, temp);
            Py_DECREF(temp);
        }
    }

    op(_GUARD_CALLABLE_LEN, (callable, unused, unused -- callable, unused, unused)) {
        PyObject *len = _PyInterpreterState_GET()->callable_cache.len;
        if (sym_get_const(ctx, callable) == len) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_const(callable, len);
        }
    }

    op(_GUARD_CALLABLE_ISINSTANCE, (callable, unused, unused, unused -- callable, unused, unused, unused)) {
        PyObject *isinstance = _PyInterpreterState_GET()->callable_cache.isinstance;
        if (sym_get_const(ctx, callable) == isinstance) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_const(callable, isinstance);
        }
    }

    op(_GUARD_CALLABLE_LIST_APPEND, (callable, unused, unused -- callable, unused, unused)) {
        PyObject *list_append = _PyInterpreterState_GET()->callable_cache.list_append;
        if (sym_get_const(ctx, callable) == list_append) {
            ADD_OP(_NOP, 0, 0);
        }
        else {
            sym_set_const(callable, list_append);
        }
    }

    op(_BINARY_SLICE, (container, start, stop -- res)) {
        // Slicing a string/list/tuple always returns the same type.
        PyTypeObject *type = sym_get_type(container);
        if (type == &PyUnicode_Type ||
            type == &PyList_Type ||
            type == &PyTuple_Type)
        {
            res = sym_new_type(ctx, type);
        }
        else {
            res = sym_new_not_null(ctx);
        }
    }

    op(_GUARD_GLOBALS_VERSION, (version/1 --)) {
        if (ctx->frame->func != NULL) {
            PyObject *globals = ctx->frame->func->func_globals;
            if (incorrect_keys(globals, version)) {
                OPT_STAT_INC(remove_globals_incorrect_keys);
                ctx->done = true;
            }
            else if (get_mutations(globals) >= _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                /* Do nothing */
            }
            else {
                if (!ctx->frame->globals_watched) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, globals);
                    _Py_BloomFilter_Add(dependencies, globals);
                    ctx->frame->globals_watched = true;
                }
                if (ctx->frame->globals_checked_version == version) {
                    ADD_OP(_NOP, 0, 0);
                }
            }
        }
        ctx->frame->globals_checked_version = version;
    }

    op(_LOAD_GLOBAL_BUILTINS, (version/1, index/1 -- res)) {
        (void)version;
        (void)index;
        PyObject *cnst = NULL;
        PyInterpreterState *interp = _PyInterpreterState_GET();
        PyObject *builtins = interp->builtins;
        if (incorrect_keys(builtins, version)) {
            OPT_STAT_INC(remove_globals_incorrect_keys);
            ctx->done = true;
        }
        else if (interp->rare_events.builtin_dict >= _Py_MAX_ALLOWED_BUILTINS_MODIFICATIONS) {
            /* Do nothing */
        }
        else {
            if (!ctx->builtins_watched) {
                PyDict_Watch(BUILTINS_WATCHER_ID, builtins);
                ctx->builtins_watched = true;
            }
            if (ctx->frame->globals_checked_version != 0 && ctx->frame->globals_watched) {
                cnst = convert_global_to_const(this_instr, builtins);
            }
        }
        if (cnst == NULL) {
            res = sym_new_not_null(ctx);
        }
        else {
            if (_Py_IsImmortal(cnst)) {
                res = PyJitRef_Borrow(sym_new_const(ctx, cnst));
            }
            else {
                res = sym_new_const(ctx, cnst);
            }
        }
    }

    op(_LOAD_GLOBAL_MODULE, (version/1, unused/1, index/1 -- res)) {
        (void)index;
        PyObject *cnst = NULL;
        if (ctx->frame->func != NULL) {
            PyObject *globals = ctx->frame->func->func_globals;
            if (incorrect_keys(globals, version)) {
                OPT_STAT_INC(remove_globals_incorrect_keys);
                ctx->done = true;
            }
            else if (get_mutations(globals) >= _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                /* Do nothing */
            }
            else {
                if (!ctx->frame->globals_watched) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, globals);
                    _Py_BloomFilter_Add(dependencies, globals);
                    ctx->frame->globals_watched = true;
                }
                if (ctx->frame->globals_checked_version != version && this_instr[-1].opcode == _NOP) {
                    REPLACE_OP(uop_buffer_last(&ctx->out_buffer), _GUARD_GLOBALS_VERSION, 0, version);
                    ctx->frame->globals_checked_version = version;
                }
                if (ctx->frame->globals_checked_version == version) {
                    cnst = convert_global_to_const(this_instr, globals);
                }
            }
        }
        if (cnst == NULL) {
            res = sym_new_not_null(ctx);
        }
        else {
            if (_Py_IsImmortal(cnst)) {
                res = PyJitRef_Borrow(sym_new_const(ctx, cnst));
            }
            else {
                res = sym_new_const(ctx, cnst);
            }
        }
    }

    op(_BINARY_OP_SUBSCR_LIST_INT, (list_st, sub_st -- res, ls, ss)) {
        res = sym_new_unknown(ctx);
        ls = list_st;
        ss = sub_st;
    }

    op(_MAKE_FUNCTION, (codeobj_st -- func, co)) {
        func = sym_new_type(ctx, &PyFunction_Type);
        co = codeobj_st;
    }

    op(_MATCH_CLASS, (subject, type, names -- attrs, s, tp, n)) {
        attrs = sym_new_not_null(ctx);
        s = subject;
        tp = type;
        n = names;
    }

    op(_DICT_UPDATE, (dict, unused[oparg - 1], update -- dict, unused[oparg - 1], upd)) {
        (void)dict;
        upd = update;
    }

    op(_RECORD_TOS, (tos -- tos)) {
        sym_set_recorded_value(tos, (PyObject *)this_instr->operand0);
    }

    op(_RECORD_TOS_TYPE, (tos -- tos)) {
        PyTypeObject *tp = (PyTypeObject *)this_instr->operand0;
        sym_set_recorded_type(tos, tp);
    }

    op(_RECORD_NOS, (nos, tos -- nos, tos)) {
        sym_set_recorded_value(nos, (PyObject *)this_instr->operand0);
    }

    op(_RECORD_NOS_TYPE, (nos, tos -- nos, tos)) {
        PyTypeObject *tp = (PyTypeObject *)this_instr->operand0;
        sym_set_recorded_type(nos, tp);
    }

    op(_RECORD_4OS, (value, _3os, nos, tos -- value, _3os, nos, tos)) {
        sym_set_recorded_value(value, (PyObject *)this_instr->operand0);
    }

    op(_RECORD_CALLABLE, (func, self, args[oparg] -- func, self, args[oparg])) {
        sym_set_recorded_value(func, (PyObject *)this_instr->operand0);
    }

    op(_RECORD_CALLABLE_KW, (func, self, args[oparg], kwnames -- func, self, args[oparg], kwnames)) {
        sym_set_recorded_value(func, (PyObject *)this_instr->operand0);
    }

    op(_RECORD_BOUND_METHOD, (callable, self, args[oparg] -- callable, self, args[oparg])) {
        sym_set_recorded_value(callable, (PyObject *)this_instr->operand0);
    }

    op(_RECORD_NOS_GEN_FUNC, (nos, tos -- nos, tos)) {
        PyFunctionObject *func = (PyFunctionObject *)this_instr->operand0;
        assert(func == NULL || PyFunction_Check(func));
        sym_set_recorded_gen_func(nos, func);
    }

    op(_RECORD_3OS_GEN_FUNC, (gen, nos, tos -- gen, nos, tos)) {
        PyFunctionObject *func = (PyFunctionObject *)this_instr->operand0;
        assert(func == NULL || PyFunction_Check(func));
        sym_set_recorded_gen_func(gen, func);
    }

    op(_GUARD_CODE_VERSION__PUSH_FRAME, (version/2 -- )) {
        PyCodeObject *co = get_current_code_object(ctx);
        if (co->co_version == version) {
            _Py_BloomFilter_Add(dependencies, co);
            // Functions derive their version from code objects.
            PyFunctionObject *func = (PyFunctionObject *)sym_get_const(ctx, ctx->frame->callable);
            if (func != NULL && func->func_version == version) {
                REPLACE_OP(this_instr, _NOP, 0, 0);
            }
        }
        else {
            ctx->done = true;
        }
    }

    op(_GUARD_CODE_VERSION_RETURN_VALUE, (version/2 -- )) {
        (void)version;
        if (ctx->frame->caller) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
    }

    op(_GUARD_CODE_VERSION_YIELD_VALUE, (version/2 -- )) {
        (void)version;
        if (ctx->frame->caller) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
    }

    op(_GUARD_CODE_VERSION_RETURN_GENERATOR, (version/2 -- )) {
        (void)version;
        if (ctx->frame->caller) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
    }

    op(_GUARD_IP__PUSH_FRAME, (ip/4 --)) {
        (void)ip;
        stack_pointer = sym_set_stack_depth((int)this_instr->operand1, stack_pointer);
        PyFunctionObject *func = (PyFunctionObject *)sym_get_const(ctx, ctx->frame->callable);
        if (func != NULL && func->func_version != 0 &&
            // We can remove this guard for simple function call targets.
            (((PyCodeObject *)ctx->frame->func->func_code)->co_flags &
                (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) == 0) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
    }

    op(_GUARD_IP_YIELD_VALUE, (ip/4 --)) {
        (void)ip;
        if (ctx->frame->caller) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        stack_pointer = sym_set_stack_depth((int)this_instr->operand1, stack_pointer);
    }

    op(_GUARD_IP_RETURN_VALUE, (ip/4 --)) {
        (void)ip;
        if (ctx->frame->caller) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        stack_pointer = sym_set_stack_depth((int)this_instr->operand1, stack_pointer);
    }

    op(_GUARD_IP_RETURN_GENERATOR, (ip/4 --)) {
        (void)ip;
        if (ctx->frame->caller) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        stack_pointer = sym_set_stack_depth((int)this_instr->operand1, stack_pointer);
    }



// END BYTECODES //

}
