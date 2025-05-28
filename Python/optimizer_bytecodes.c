#include "Python.h"
#include "pycore_optimizer.h"
#include "pycore_uops.h"
#include "pycore_uop_ids.h"
#include "internal/pycore_moduleobject.h"

#define op(name, ...) /* NAME is ignored */

typedef struct _Py_UOpsAbstractFrame _Py_UOpsAbstractFrame;

/* Shortened forms for convenience */
#define ref_is_not_null _Py_uop_ref_is_not_null
#define ref_is_const _Py_uop_ref_is_const
#define ref_get_const _Py_uop_ref_get_const
#define ref_new_unknown _Py_uop_ref_new_unknown
#define ref_new_not_null _Py_uop_ref_new_not_null
#define ref_new_type _Py_uop_ref_new_type
#define ref_is_null _Py_uop_ref_is_null
#define ref_new_const _Py_uop_ref_new_const
#define ref_new_null _Py_uop_ref_new_null
#define ref_matches_type _Py_uop_ref_matches_type
#define ref_matches_type_version _Py_uop_ref_matches_type_version
#define ref_get_type _Py_uop_ref_get_type
#define ref_has_type _Py_uop_ref_has_type
#define ref_set_null(SYM) _Py_uop_ref_set_null(ctx, SYM)
#define ref_set_non_null(SYM) _Py_uop_ref_set_non_null(ctx, SYM)
#define ref_set_type(SYM, TYPE) _Py_uop_ref_set_type(ctx, SYM, TYPE)
#define ref_set_type_version(SYM, VERSION) _Py_uop_ref_set_type_version(ctx, SYM, VERSION)
#define ref_set_const(SYM, CNST) _Py_uop_ref_set_const(ctx, SYM, CNST)
#define ref_is_bottom _Py_uop_ref_is_bottom
#define frame_new _Py_uop_frame_new
#define frame_pop _Py_uop_frame_pop
#define ref_new_tuple _Py_uop_ref_new_tuple
#define ref_tuple_getitem _Py_uop_ref_tuple_getitem
#define ref_tuple_length _Py_uop_ref_tuple_length
#define ref_is_immortal _Py_uop_ref_is_immortal
#define ref_new_truthiness _Py_uop_ref_new_truthiness

extern int
optimize_to_bool(
    _PyUOpInstruction *this_instr,
    JitOptContext *ctx,
    JitOptSymbol *value,
    JitOptSymbol **result_ptr);

extern void
eliminate_pop_guard(_PyUOpInstruction *this_instr, bool exit);

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

    op(_LOAD_FAST_CHECK, (-- value)) {
        value = GETLOCAL(oparg);
        // We guarantee this will error - just bail and don't optimize it.
        if (ref_is_null(value)) {
            ctx->done = true;
        }
    }

    op(_LOAD_FAST, (-- value)) {
        value = GETLOCAL(oparg);
    }

    op(_LOAD_FAST_BORROW, (-- value)) {
        value = PyJitRef_Borrow(GETLOCAL(oparg));
    }

    op(_LOAD_FAST_AND_CLEAR, (-- value)) {
        value = GETLOCAL(oparg);
        JitOptRef temp = ref_new_null(ctx);
        GETLOCAL(oparg) = temp;
    }

    op(_STORE_FAST, (value --)) {
        GETLOCAL(oparg) = value;
    }

    op(_PUSH_NULL, (-- res)) {
        res = ref_new_null(ctx);
    }

    op(_GUARD_TOS_INT, (value -- value)) {
        if (ref_matches_type(value, &PyLong_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(value, &PyLong_Type);
    }

    op(_GUARD_NOS_INT, (left, unused -- left, unused)) {
        if (ref_matches_type(left, &PyLong_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(left, &PyLong_Type);
    }

    op(_CHECK_ATTR_CLASS, (type_version/2, owner -- owner)) {
        PyObject *type = (PyObject *)_PyType_LookupByVersion(type_version);
        if (type) {
            if (type == ref_get_const(ctx, owner)) {
                REPLACE_OP(this_instr, _NOP, 0, 0);
            }
            else {
                ref_set_const(owner, type);
            }
        }
    }

    op(_GUARD_TYPE_VERSION, (type_version/2, owner -- owner)) {
        assert(type_version);
        if (ref_matches_type_version(owner, type_version)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        } else {
            // add watcher so that whenever the type changes we invalidate this
            PyTypeObject *type = _PyType_LookupByVersion(type_version);
            // if the type is null, it was not found in the cache (there was a conflict)
            // with the key, in which case we can't trust the version
            if (type) {
                // if the type version was set properly, then add a watcher
                // if it wasn't this means that the type version was previously set to something else
                // and we set the owner to bottom, so we don't need to add a watcher because we must have
                // already added one earlier.
                if (ref_set_type_version(owner, type_version)) {
                    PyType_Watch(TYPE_WATCHER_ID, (PyObject *)type);
                    _Py_BloomFilter_Add(dependencies, type);
                }
            }

        }
    }

    op(_GUARD_TOS_FLOAT, (value -- value)) {
        if (ref_matches_type(value, &PyFloat_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(value, &PyFloat_Type);
    }

    op(_GUARD_NOS_FLOAT, (left, unused -- left, unused)) {
        if (ref_matches_type(left, &PyFloat_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(left, &PyFloat_Type);
    }

    op(_BINARY_OP, (lhs, rhs -- res)) {
        bool lhs_int = ref_matches_type(lhs, &PyLong_Type);
        bool rhs_int = ref_matches_type(rhs, &PyLong_Type);
        bool lhs_float = ref_matches_type(lhs, &PyFloat_Type);
        bool rhs_float = ref_matches_type(rhs, &PyFloat_Type);
        if (!((lhs_int || lhs_float) && (rhs_int || rhs_float))) {
            // There's something other than an int or float involved:
            res = ref_new_unknown(ctx);
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
                res = ref_new_unknown(ctx);
            }
            else if (lhs_float) {
                // Case C:
                res = ref_new_type(ctx, &PyFloat_Type);
            }
            else if (!ref_is_const(ctx, rhs)) {
                // Case A or B... can't know without the sign of the RHS:
                res = ref_new_unknown(ctx);
            }
            else if (_PyLong_IsNegative((PyLongObject *)ref_get_const(ctx, rhs))) {
                // Case B:
                res = ref_new_type(ctx, &PyFloat_Type);
            }
            else {
                // Case A:
                res = ref_new_type(ctx, &PyLong_Type);
            }
        }
        else if (oparg == NB_TRUE_DIVIDE || oparg == NB_INPLACE_TRUE_DIVIDE) {
            res = ref_new_type(ctx, &PyFloat_Type);
        }
        else if (lhs_int && rhs_int) {
            res = ref_new_type(ctx, &PyLong_Type);
        }
        else {
            res = ref_new_type(ctx, &PyFloat_Type);
        }
    }

    op(_BINARY_OP_ADD_INT, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyLong_CheckExact(ref_get_const(ctx, left)));
            assert(PyLong_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = _PyLong_Add((PyLongObject *)ref_get_const(ctx, left),
                                         (PyLongObject *)ref_get_const(ctx, right));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            res = ref_new_type(ctx, &PyLong_Type);
        }
    }

    op(_BINARY_OP_SUBTRACT_INT, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyLong_CheckExact(ref_get_const(ctx, left)));
            assert(PyLong_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = _PyLong_Subtract((PyLongObject *)ref_get_const(ctx, left),
                                              (PyLongObject *)ref_get_const(ctx, right));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            res = ref_new_type(ctx, &PyLong_Type);
        }
    }

    op(_BINARY_OP_MULTIPLY_INT, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyLong_CheckExact(ref_get_const(ctx, left)));
            assert(PyLong_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = _PyLong_Multiply((PyLongObject *)ref_get_const(ctx, left),
                                              (PyLongObject *)ref_get_const(ctx, right));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            res = ref_new_type(ctx, &PyLong_Type);
        }
    }

    op(_BINARY_OP_ADD_FLOAT, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyFloat_CheckExact(ref_get_const(ctx, left)));
            assert(PyFloat_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = PyFloat_FromDouble(
                PyFloat_AS_DOUBLE(ref_get_const(ctx, left)) +
                PyFloat_AS_DOUBLE(ref_get_const(ctx, right)));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            res = ref_new_type(ctx, &PyFloat_Type);
        }
        // TODO (gh-134584): Move this to the optimizer generator.
        if (PyJitRef_IsBorrowed(left) && PyJitRef_IsBorrowed(right)) {
            REPLACE_OP(this_instr, op_without_decref_inputs[opcode], oparg, 0);
        }
    }

    op(_BINARY_OP_SUBTRACT_FLOAT, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyFloat_CheckExact(ref_get_const(ctx, left)));
            assert(PyFloat_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = PyFloat_FromDouble(
                PyFloat_AS_DOUBLE(ref_get_const(ctx, left)) -
                PyFloat_AS_DOUBLE(ref_get_const(ctx, right)));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            res = ref_new_type(ctx, &PyFloat_Type);
        }
        // TODO (gh-134584): Move this to the optimizer generator.
        if (PyJitRef_IsBorrowed(left) && PyJitRef_IsBorrowed(right)) {
            REPLACE_OP(this_instr, op_without_decref_inputs[opcode], oparg, 0);
        }
    }

    op(_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyFloat_CheckExact(ref_get_const(ctx, left)));
            assert(PyFloat_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = PyFloat_FromDouble(
                PyFloat_AS_DOUBLE(ref_get_const(ctx, left)) *
                PyFloat_AS_DOUBLE(ref_get_const(ctx, right)));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            res = ref_new_type(ctx, &PyFloat_Type);
        }
        // TODO (gh-134584): Move this to the optimizer generator.
        if (PyJitRef_IsBorrowed(left) && PyJitRef_IsBorrowed(right)) {
            REPLACE_OP(this_instr, op_without_decref_inputs[opcode], oparg, 0);
        }
    }

    op(_BINARY_OP_ADD_UNICODE, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyUnicode_CheckExact(ref_get_const(ctx, left)));
            assert(PyUnicode_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = PyUnicode_Concat(ref_get_const(ctx, left), ref_get_const(ctx, right));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
        }
        else {
            res = ref_new_type(ctx, &PyUnicode_Type);
        }
    }

    op(_BINARY_OP_INPLACE_ADD_UNICODE, (left, right -- )) {
        JitOptRef res;
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyUnicode_CheckExact(ref_get_const(ctx, left)));
            assert(PyUnicode_CheckExact(ref_get_const(ctx, right)));
            PyObject *temp = PyUnicode_Concat(ref_get_const(ctx, left), ref_get_const(ctx, right));
            if (temp == NULL) {
                goto error;
            }
            res = ref_new_const(ctx, temp);
            Py_DECREF(temp);
        }
        else {
            res = ref_new_type(ctx, &PyUnicode_Type);
        }
        // _STORE_FAST:
        GETLOCAL(this_instr->operand0) = res;
    }

    op(_BINARY_OP_SUBSCR_INIT_CALL, (container, sub, getitem  -- new_frame: _Py_UOpsAbstractFrame *)) {
        new_frame = NULL;
        ctx->done = true;
    }

    op(_BINARY_OP_SUBSCR_STR_INT, (str_st, sub_st -- res)) {
        res = ref_new_type(ctx, &PyUnicode_Type);
    }

    op(_BINARY_OP_SUBSCR_TUPLE_INT, (tuple_st, sub_st -- res)) {
        assert(ref_matches_type(tuple_st, &PyTuple_Type));
        if (ref_is_const(ctx, sub_st)) {
            assert(PyLong_CheckExact(ref_get_const(ctx, sub_st)));
            long index = PyLong_AsLong(ref_get_const(ctx, sub_st));
            assert(index >= 0);
            int tuple_length = ref_tuple_length(tuple_st);
            if (tuple_length == -1) {
                // Unknown length
                res = ref_new_not_null(ctx);
            }
            else {
                assert(index < tuple_length);
                res = ref_tuple_getitem(ctx, tuple_st, index);
            }
        }
        else {
            res = ref_new_not_null(ctx);
        }
    }

    op(_TO_BOOL, (value -- res)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res);
        if (!already_bool) {
            res = ref_new_truthiness(ctx, value, true);
        }
    }

    op(_TO_BOOL_BOOL, (value -- value)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &value);
        if (!already_bool) {
            ref_set_type(value, &PyBool_Type);
            value = ref_new_truthiness(ctx, value, true);
        }
    }

    op(_TO_BOOL_INT, (value -- res)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res);
        if (!already_bool) {
            ref_set_type(value, &PyLong_Type);
            res = ref_new_truthiness(ctx, value, true);
        }
    }

    op(_TO_BOOL_LIST, (value -- res)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res);
        if (!already_bool) {
            res = ref_new_type(ctx, &PyBool_Type);
        }
    }

    op(_TO_BOOL_NONE, (value -- res)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res);
        if (!already_bool) {
            ref_set_const(value, Py_None);
            res = ref_new_const(ctx, Py_False);
        }
    }

    op(_GUARD_NOS_UNICODE, (nos, unused -- nos, unused)) {
        if (ref_matches_type(nos, &PyUnicode_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(nos, &PyUnicode_Type);
    }

    op(_GUARD_TOS_UNICODE, (value -- value)) {
        if (ref_matches_type(value, &PyUnicode_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(value, &PyUnicode_Type);
    }

    op(_TO_BOOL_STR, (value -- res)) {
        int already_bool = optimize_to_bool(this_instr, ctx, value, &res);
        if (!already_bool) {
            res = ref_new_truthiness(ctx, value, true);
        }
    }

    op(_UNARY_NOT, (value -- res)) {
        ref_set_type(value, &PyBool_Type);
        res = ref_new_truthiness(ctx, value, false);
    }

    op(_COMPARE_OP, (left, right -- res)) {
        if (oparg & 16) {
            res = ref_new_type(ctx, &PyBool_Type);
        }
        else {
            res = _Py_uop_ref_new_not_null(ctx);
        }
    }

    op(_COMPARE_OP_INT, (left, right -- res)) {
        if (ref_is_const(ctx, left) && ref_is_const(ctx, right)) {
            assert(PyLong_CheckExact(ref_get_const(ctx, left)));
            assert(PyLong_CheckExact(ref_get_const(ctx, right)));
            PyObject *tmp = PyObject_RichCompare(ref_get_const(ctx, left),
                                                 ref_get_const(ctx, right),
                                                 oparg >> 5);
            if (tmp == NULL) {
                goto error;
            }
            assert(PyBool_Check(tmp));
            assert(_Py_IsImmortal(tmp));
            REPLACE_OP(this_instr, _POP_TWO_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)tmp);
            res = ref_new_const(ctx, tmp);
            Py_DECREF(tmp);
        }
        else {
            res = ref_new_type(ctx, &PyBool_Type);
        }
    }

    op(_COMPARE_OP_FLOAT, (left, right -- res)) {
        res = ref_new_type(ctx, &PyBool_Type);
    }

    op(_COMPARE_OP_STR, (left, right -- res)) {
        res = ref_new_type(ctx, &PyBool_Type);
    }

    op(_IS_OP, (left, right -- b)) {
        b = ref_new_type(ctx, &PyBool_Type);
    }

    op(_CONTAINS_OP, (left, right -- b)) {
        b = ref_new_type(ctx, &PyBool_Type);
    }

    op(_CONTAINS_OP_SET, (left, right -- b)) {
        b = ref_new_type(ctx, &PyBool_Type);
    }

    op(_CONTAINS_OP_DICT, (left, right -- b)) {
        b = ref_new_type(ctx, &PyBool_Type);
    }

    op(_LOAD_CONST, (-- value)) {
        PyObject *val = PyTuple_GET_ITEM(co->co_consts, oparg);
        REPLACE_OP(this_instr, _LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)val);
        value = PyJitRef_Borrow(ref_new_const(ctx, val));
    }

    op(_LOAD_SMALL_INT, (-- value)) {
        PyObject *val = PyLong_FromLong(oparg);
        assert(val);
        assert(_Py_IsImmortal(val));
        REPLACE_OP(this_instr, _LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)val);
        value = PyJitRef_Borrow(ref_new_const(ctx, val));
    }

    op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
        value = PyJitRef_Borrow(ref_new_const(ctx, ptr));
    }

    op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
        value = PyJitRef_Borrow(ref_new_const(ctx, ptr));
    }

    op(_POP_TOP_LOAD_CONST_INLINE, (ptr/4, pop -- value)) {
        value = PyJitRef_Borrow(ref_new_const(ctx, ptr));
    }

    op(_POP_TOP_LOAD_CONST_INLINE_BORROW, (ptr/4, pop -- value)) {
        value = PyJitRef_Borrow(ref_new_const(ctx, ptr));
    }

    op(_POP_CALL_LOAD_CONST_INLINE_BORROW, (ptr/4, unused, unused -- value)) {
        value = PyJitRef_Borrow(ref_new_const(ctx, ptr));
    }

    op(_POP_CALL_ONE_LOAD_CONST_INLINE_BORROW, (ptr/4, unused, unused, unused -- value)) {
        value = PyJitRef_Borrow(ref_new_const(ctx, ptr));
    }

    op(_POP_CALL_TWO_LOAD_CONST_INLINE_BORROW, (ptr/4, unused, unused, unused, unused -- value)) {
        value = PyJitRef_Borrow(ref_new_const(ctx, ptr));
    }

    op(_COPY, (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
        assert(oparg > 0);
        top = bottom;
    }

    op(_SWAP, (bottom, unused[oparg-2], top -- bottom, unused[oparg-2], top)) {
        JitOptRef temp = bottom;
        bottom = top;
        top = temp;
        assert(oparg >= 2);
    }

    op(_LOAD_ATTR_INSTANCE_VALUE, (offset/1, owner -- attr)) {
        attr = ref_new_not_null(ctx);
        (void)offset;
    }

    op(_LOAD_ATTR_MODULE, (dict_version/2, index/1, owner -- attr)) {
        (void)dict_version;
        (void)index;
        attr = PyJitRef_NULL;
        if (ref_is_const(ctx, owner)) {
            PyModuleObject *mod = (PyModuleObject *)ref_get_const(ctx, owner);
            if (PyModule_CheckExact(mod)) {
                PyObject *dict = mod->md_dict;
                uint64_t watched_mutations = get_mutations(dict);
                if (watched_mutations < _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, dict);
                    _Py_BloomFilter_Add(dependencies, dict);
                    PyObject *res = convert_global_to_const(this_instr, dict, true);
                    attr = ref_new_const(ctx, res);
                }
            }
        }
        if (PyJitRef_IsNull(attr)) {
            /* No conversion made. We don't know what `attr` is. */
            attr = ref_new_not_null(ctx);
        }
    }

    op (_PUSH_NULL_CONDITIONAL, ( -- null[oparg & 1])) {
        if (oparg & 1) {
            REPLACE_OP(this_instr, _PUSH_NULL, 0, 0);
            null[0] = ref_new_null(ctx);
        }
        else {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
    }

    op(_LOAD_ATTR, (owner -- attr, self_or_null[oparg&1])) {
        (void)owner;
        attr = ref_new_not_null(ctx);
        if (oparg & 1) {
            self_or_null[0] = ref_new_unknown(ctx);
        }
    }

    op(_LOAD_ATTR_WITH_HINT, (hint/1, owner -- attr)) {
        attr = ref_new_not_null(ctx);
        (void)hint;
    }

    op(_LOAD_ATTR_SLOT, (index/1, owner -- attr)) {
        attr = ref_new_not_null(ctx);
        (void)index;
    }

    op(_LOAD_ATTR_CLASS, (descr/4, owner -- attr)) {
        (void)descr;
        PyTypeObject *type = (PyTypeObject *)ref_get_const(ctx, owner);
        PyObject *name = PyTuple_GET_ITEM(co->co_names, oparg >> 1);
        attr = lookup_attr(ctx, this_instr, type, name,
                           _POP_TOP_LOAD_CONST_INLINE_BORROW,
                           _POP_TOP_LOAD_CONST_INLINE);
    }

    op(_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES, (descr/4, owner -- attr)) {
        (void)descr;
        PyTypeObject *type = ref_get_type(owner);
        PyObject *name = PyTuple_GET_ITEM(co->co_names, oparg >> 1);
        attr = lookup_attr(ctx, this_instr, type, name,
                           _POP_TOP_LOAD_CONST_INLINE_BORROW,
                           _POP_TOP_LOAD_CONST_INLINE);
    }

    op(_LOAD_ATTR_NONDESCRIPTOR_NO_DICT, (descr/4, owner -- attr)) {
        (void)descr;
        PyTypeObject *type = ref_get_type(owner);
        PyObject *name = PyTuple_GET_ITEM(co->co_names, oparg >> 1);
        attr = lookup_attr(ctx, this_instr, type, name,
                           _POP_TOP_LOAD_CONST_INLINE_BORROW,
                           _POP_TOP_LOAD_CONST_INLINE);
    }

    op(_LOAD_ATTR_METHOD_WITH_VALUES, (descr/4, owner -- attr, self)) {
        (void)descr;
        PyTypeObject *type = ref_get_type(owner);
        PyObject *name = PyTuple_GET_ITEM(co->co_names, oparg >> 1);
        attr = lookup_attr(ctx, this_instr, type, name,
                           _LOAD_CONST_UNDER_INLINE_BORROW,
                           _LOAD_CONST_UNDER_INLINE);
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_NO_DICT, (descr/4, owner -- attr, self)) {
        (void)descr;
        PyTypeObject *type = ref_get_type(owner);
        PyObject *name = PyTuple_GET_ITEM(co->co_names, oparg >> 1);
        attr = lookup_attr(ctx, this_instr, type, name,
                           _LOAD_CONST_UNDER_INLINE_BORROW,
                           _LOAD_CONST_UNDER_INLINE);
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_LAZY_DICT, (descr/4, owner -- attr, self)) {
        (void)descr;
        PyTypeObject *type = ref_get_type(owner);
        PyObject *name = PyTuple_GET_ITEM(co->co_names, oparg >> 1);
        attr = lookup_attr(ctx, this_instr, type, name,
                           _LOAD_CONST_UNDER_INLINE_BORROW,
                           _LOAD_CONST_UNDER_INLINE);
        self = owner;
    }

    op(_LOAD_ATTR_PROPERTY_FRAME, (fget/4, owner -- new_frame: _Py_UOpsAbstractFrame *)) {
        (void)fget;
        new_frame = NULL;
        ctx->done = true;
    }

    op(_INIT_CALL_BOUND_METHOD_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        callable = ref_new_not_null(ctx);
        self_or_null = ref_new_not_null(ctx);
    }

    op(_CHECK_FUNCTION_VERSION, (func_version/2, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        if (ref_is_const(ctx, callable) && ref_matches_type(callable, &PyFunction_Type)) {
            assert(PyFunction_Check(ref_get_const(ctx, callable)));
            REPLACE_OP(this_instr, _CHECK_FUNCTION_VERSION_INLINE, 0, func_version);
            this_instr->operand1 = (uintptr_t)ref_get_const(ctx, callable);
        }
        ref_set_type(callable, &PyFunction_Type);
    }

    op(_CHECK_FUNCTION_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        assert(ref_matches_type(callable, &PyFunction_Type));
        if (ref_is_const(ctx, callable)) {
            if (ref_is_null(self_or_null) || ref_is_not_null(self_or_null)) {
                PyFunctionObject *func = (PyFunctionObject *)ref_get_const(ctx, callable);
                PyCodeObject *co = (PyCodeObject *)func->func_code;
                if (co->co_argcount == oparg + !ref_is_null(self_or_null)) {
                    REPLACE_OP(this_instr, _NOP, 0 ,0);
                }
            }
        }
    }

    op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null, unused[oparg] -- callable, null, unused[oparg])) {
        ref_set_null(null);
        ref_set_type(callable, &PyMethod_Type);
    }

    op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null, args[oparg] -- new_frame: _Py_UOpsAbstractFrame *)) {
        int argcount = oparg;

        PyCodeObject *co = NULL;
        assert((this_instr + 2)->opcode == _PUSH_FRAME);
        co = get_code_with_logging((this_instr + 2));
        if (co == NULL) {
            ctx->done = true;
            break;
        }


        assert(!PyJitRef_IsNull(self_or_null));
        assert(args != NULL);
        if (ref_is_not_null(self_or_null)) {
            // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS in VM
            args--;
            argcount++;
        }

        if (ref_is_null(self_or_null) || ref_is_not_null(self_or_null)) {
            new_frame = frame_new(ctx, co, 0, args, argcount);
        } else {
            new_frame = frame_new(ctx, co, 0, NULL, 0);

        }
    }

    op(_MAYBE_EXPAND_METHOD, (callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        (void)args;
        callable = ref_new_not_null(ctx);
        self_or_null = ref_new_not_null(ctx);
    }

    op(_PY_FRAME_GENERAL, (callable, self_or_null, args[oparg] -- new_frame: _Py_UOpsAbstractFrame *)) {
        PyCodeObject *co = NULL;
        assert((this_instr + 2)->opcode == _PUSH_FRAME);
        co = get_code_with_logging((this_instr + 2));
        if (co == NULL) {
            ctx->done = true;
            break;
        }

        new_frame = frame_new(ctx, co, 0, NULL, 0);
    }

    op(_PY_FRAME_KW, (callable, self_or_null, args[oparg], kwnames -- new_frame: _Py_UOpsAbstractFrame *)) {
        new_frame = NULL;
        ctx->done = true;
    }

    op(_CHECK_AND_ALLOCATE_OBJECT, (type_version/2, callable, self_or_null, args[oparg] -- callable, self_or_null, args[oparg])) {
        (void)type_version;
        (void)args;
        callable = ref_new_not_null(ctx);
        self_or_null = ref_new_not_null(ctx);
    }

    op(_CREATE_INIT_FRAME, (init, self, args[oparg] -- init_frame: _Py_UOpsAbstractFrame *)) {
        init_frame = NULL;
        ctx->done = true;
    }

    op(_RETURN_VALUE, (retval -- res)) {
        JitOptRef temp = retval;
        DEAD(retval);
        SAVE_STACK();
        ctx->frame->stack_pointer = stack_pointer;
        frame_pop(ctx);
        stack_pointer = ctx->frame->stack_pointer;

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
        RELOAD_STACK();
        res = temp;
    }

    op(_RETURN_GENERATOR, ( -- res)) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        frame_pop(ctx);
        stack_pointer = ctx->frame->stack_pointer;
        res = ref_new_unknown(ctx);

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

    op(_YIELD_VALUE, (unused -- value)) {
        value = ref_new_unknown(ctx);
    }

    op(_FOR_ITER_GEN_FRAME, (unused, unused -- unused, unused, gen_frame: _Py_UOpsAbstractFrame*)) {
        gen_frame = NULL;
        /* We are about to hit the end of the trace */
        ctx->done = true;
    }

    op(_SEND_GEN_FRAME, (unused, unused -- unused, gen_frame: _Py_UOpsAbstractFrame *)) {
        gen_frame = NULL;
        // We are about to hit the end of the trace:
        ctx->done = true;
    }

    op(_CHECK_STACK_SPACE, (unused, unused, unused[oparg] -- unused, unused, unused[oparg])) {
        assert(corresponding_check_stack == NULL);
        corresponding_check_stack = this_instr;
    }

    op (_CHECK_STACK_SPACE_OPERAND, (framesize/2 -- )) {
        (void)framesize;
        /* We should never see _CHECK_STACK_SPACE_OPERANDs.
        * They are only created at the end of this pass. */
        Py_UNREACHABLE();
    }

    op(_PUSH_FRAME, (new_frame: _Py_UOpsAbstractFrame * -- )) {
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

        /* Stack space handling */
        int framesize = co->co_framesize;
        assert(framesize > 0);
        curr_space += framesize;
        if (curr_space < 0 || curr_space > INT32_MAX) {
            // won't fit in signed 32-bit int
            ctx->done = true;
            break;
        }
        max_space = curr_space > max_space ? curr_space : max_space;
        if (first_valid_check_stack == NULL) {
            first_valid_check_stack = corresponding_check_stack;
        }
        else if (corresponding_check_stack) {
            // delete all but the first valid _CHECK_STACK_SPACE
            corresponding_check_stack->opcode = _NOP;
        }
        corresponding_check_stack = NULL;
    }

    op(_UNPACK_SEQUENCE, (seq -- values[oparg], top[0])) {
        (void)top;
        /* This has to be done manually */
        for (int i = 0; i < oparg; i++) {
            values[i] = ref_new_unknown(ctx);
        }
    }

    op(_UNPACK_EX, (seq -- values[oparg & 0xFF], unused, unused[oparg >> 8], top[0])) {
        (void)top;
        /* This has to be done manually */
        int totalargs = (oparg & 0xFF) + (oparg >> 8) + 1;
        for (int i = 0; i < totalargs; i++) {
            values[i] = ref_new_unknown(ctx);
        }
    }

    op(_ITER_CHECK_TUPLE, (iter, null_or_index -- iter, null_or_index)) {
        if (sym_matches_type(iter, &PyTuple_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        sym_set_type(iter, &PyTuple_Type);
    }

    op(_ITER_NEXT_RANGE, (iter, null_or_index -- iter, null_or_index, next)) {
       next = ref_new_type(ctx, &PyLong_Type);
    }

    op(_CALL_TYPE_1, (unused, unused, arg -- res)) {
        if (ref_has_type(arg)) {
            res = ref_new_const(ctx, (PyObject *)ref_get_type(arg));
        }
        else {
            res = ref_new_not_null(ctx);
        }
    }

    op(_CALL_STR_1, (unused, unused, arg -- res)) {
        if (ref_matches_type(arg, &PyUnicode_Type)) {
            // e.g. str('foo') or str(foo) where foo is known to be a string
            res = arg;
        }
        else {
            res = ref_new_type(ctx, &PyUnicode_Type);
        }
    }

    op(_CALL_ISINSTANCE, (unused, unused, instance, cls -- res)) {
        // the result is always a bool, but sometimes we can
        // narrow it down to True or False
        res = ref_new_type(ctx, &PyBool_Type);
        PyTypeObject *inst_type = ref_get_type(instance);
        PyTypeObject *cls_o = (PyTypeObject *)ref_get_const(ctx, cls);
        if (inst_type && cls_o && ref_matches_type(cls, &PyType_Type)) {
            // isinstance(inst, cls) where both inst and cls have
            // known types, meaning we can deduce either True or False

            // The below check is equivalent to PyObject_TypeCheck(inst, cls)
            PyObject *out = Py_False;
            if (inst_type == cls_o || PyType_IsSubtype(inst_type, cls_o)) {
                out = Py_True;
            }
            ref_set_const(res, out);
            REPLACE_OP(this_instr, _POP_CALL_TWO_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)out);
        }
    }

    op(_GUARD_IS_TRUE_POP, (flag -- )) {
        if (ref_is_const(ctx, flag)) {
            PyObject *value = ref_get_const(ctx, flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, value != Py_True);
        }
        ref_set_const(flag, Py_True);
    }

    op(_GUARD_IS_FALSE_POP, (flag -- )) {
        if (ref_is_const(ctx, flag)) {
            PyObject *value = ref_get_const(ctx, flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, value != Py_False);
        }
        ref_set_const(flag, Py_False);
    }

    op(_GUARD_IS_NONE_POP, (val -- )) {
        if (ref_is_const(ctx, val)) {
            PyObject *value = ref_get_const(ctx, val);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, !Py_IsNone(value));
        }
        else if (ref_has_type(val)) {
            assert(!ref_matches_type(val, &_PyNone_Type));
            eliminate_pop_guard(this_instr, true);
        }
        ref_set_const(val, Py_None);
    }

    op(_GUARD_IS_NOT_NONE_POP, (val -- )) {
        if (ref_is_const(ctx, val)) {
            PyObject *value = ref_get_const(ctx, val);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, Py_IsNone(value));
        }
        else if (ref_has_type(val)) {
            assert(!ref_matches_type(val, &_PyNone_Type));
            eliminate_pop_guard(this_instr, false);
        }
    }

    op(_CHECK_PEP_523, (--)) {
        /* Setting the eval frame function invalidates
        * all executors, so no need to check dynamically */
        if (_PyInterpreterState_GET()->eval_frame == NULL) {
            REPLACE_OP(this_instr, _NOP, 0 ,0);
        }
    }

    op(_INSERT_NULL, (self -- method_and_self[2])) {
        method_and_self[0] = ref_new_null(ctx);
        method_and_self[1] = self;
    }

    op(_LOAD_SPECIAL, (method_and_self[2] -- method_and_self[2])) {
        method_and_self[0] = ref_new_not_null(ctx);
        method_and_self[1] = ref_new_unknown(ctx);
    }

    op(_JUMP_TO_TOP, (--)) {
        ctx->done = true;
    }

    op(_EXIT_TRACE, (exit_p/4 --)) {
        (void)exit_p;
        ctx->done = true;
    }

    op(_REPLACE_WITH_TRUE, (value -- res)) {
        REPLACE_OP(this_instr, _POP_TOP_LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)Py_True);
        res = ref_new_const(ctx, Py_True);
    }

    op(_BUILD_TUPLE, (values[oparg] -- tup)) {
        tup = ref_new_tuple(ctx, oparg, values);
    }

    op(_BUILD_LIST, (values[oparg] -- list)) {
        list = ref_new_type(ctx, &PyList_Type);
    }

    op(_BUILD_SLICE, (args[oparg] -- slice)) {
        slice = ref_new_type(ctx, &PySlice_Type);
    }

    op(_BUILD_MAP, (values[oparg*2] -- map)) {
        map = ref_new_type(ctx, &PyDict_Type);
    }

    op(_BUILD_STRING, (pieces[oparg] -- str)) {
        str = ref_new_type(ctx, &PyUnicode_Type);
    }

    op(_BUILD_SET, (values[oparg] -- set)) {
        set = ref_new_type(ctx, &PySet_Type);
    }

    op(_UNPACK_SEQUENCE_TWO_TUPLE, (seq -- val1, val0)) {
        val0 = ref_tuple_getitem(ctx, seq, 0);
        val1 = ref_tuple_getitem(ctx, seq, 1);
    }

    op(_UNPACK_SEQUENCE_TUPLE, (seq -- values[oparg])) {
        for (int i = 0; i < oparg; i++) {
            values[i] = ref_tuple_getitem(ctx, seq, oparg - i - 1);
        }
    }

    op(_CALL_TUPLE_1, (callable, null, arg -- res)) {
        if (ref_matches_type(arg, &PyTuple_Type)) {
            // e.g. tuple((1, 2)) or tuple(foo) where foo is known to be a tuple
            res = arg;
        }
        else {
            res = ref_new_type(ctx, &PyTuple_Type);
        }
    }

    op(_GUARD_TOS_LIST, (tos -- tos)) {
        if (ref_matches_type(tos, &PyList_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(tos, &PyList_Type);
    }

    op(_GUARD_NOS_LIST, (nos, unused -- nos, unused)) {
        if (ref_matches_type(nos, &PyList_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(nos, &PyList_Type);
    }

    op(_GUARD_TOS_TUPLE, (tos -- tos)) {
        if (ref_matches_type(tos, &PyTuple_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(tos, &PyTuple_Type);
    }

    op(_GUARD_NOS_TUPLE, (nos, unused -- nos, unused)) {
        if (ref_matches_type(nos, &PyTuple_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(nos, &PyTuple_Type);
    }

    op(_GUARD_TOS_DICT, (tos -- tos)) {
        if (ref_matches_type(tos, &PyDict_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(tos, &PyDict_Type);
    }

    op(_GUARD_NOS_DICT, (nos, unused -- nos, unused)) {
        if (ref_matches_type(nos, &PyDict_Type)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_type(nos, &PyDict_Type);
    }

    op(_GUARD_TOS_ANY_SET, (tos -- tos)) {
        if (ref_matches_type(tos, &PySet_Type) ||
            ref_matches_type(tos, &PyFrozenSet_Type))
        {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
    }

    op(_GUARD_NOS_NULL, (null, unused -- null, unused)) {
        if (ref_is_null(null)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_null(null);
    }

    op(_GUARD_NOS_NOT_NULL, (nos, unused -- nos, unused)) {
        if (ref_is_not_null(nos)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_non_null(nos);
    }

    op(_GUARD_THIRD_NULL, (null, unused, unused -- null, unused, unused)) {
        if (ref_is_null(null)) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_null(null);
    }

    op(_GUARD_CALLABLE_TYPE_1, (callable, unused, unused -- callable, unused, unused)) {
        if (ref_get_const(ctx, callable) == (PyObject *)&PyType_Type) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_const(callable, (PyObject *)&PyType_Type);
    }

    op(_GUARD_CALLABLE_TUPLE_1, (callable, unused, unused -- callable, unused, unused)) {
        if (ref_get_const(ctx, callable) == (PyObject *)&PyTuple_Type) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_const(callable, (PyObject *)&PyTuple_Type);
    }

    op(_GUARD_CALLABLE_STR_1, (callable, unused, unused -- callable, unused, unused)) {
        if (ref_get_const(ctx, callable) == (PyObject *)&PyUnicode_Type) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_const(callable, (PyObject *)&PyUnicode_Type);
    }

    op(_CALL_LEN, (unused, unused, unused -- res)) {
        res = ref_new_type(ctx, &PyLong_Type);
    }

    op(_GET_LEN, (obj -- obj, len)) {
        int tuple_length = ref_tuple_length(obj);
        if (tuple_length == -1) {
            len = ref_new_type(ctx, &PyLong_Type);
        }
        else {
            assert(tuple_length >= 0);
            PyObject *temp = PyLong_FromLong(tuple_length);
            if (temp == NULL) {
                goto error;
            }
            if (_Py_IsImmortal(temp)) {
                REPLACE_OP(this_instr, _LOAD_CONST_INLINE_BORROW, 0, (uintptr_t)temp);
            }
            len = ref_new_const(ctx, temp);
            Py_DECREF(temp);
        }
    }

    op(_GUARD_CALLABLE_LEN, (callable, unused, unused -- callable, unused, unused)) {
        PyObject *len = _PyInterpreterState_GET()->callable_cache.len;
        if (ref_get_const(ctx, callable) == len) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_const(callable, len);
    }

    op(_GUARD_CALLABLE_ISINSTANCE, (callable, unused, unused, unused -- callable, unused, unused, unused)) {
        PyObject *isinstance = _PyInterpreterState_GET()->callable_cache.isinstance;
        if (ref_get_const(ctx, callable) == isinstance) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_const(callable, isinstance);
    }

    op(_GUARD_CALLABLE_LIST_APPEND, (callable, unused, unused -- callable, unused, unused)) {
        PyObject *list_append = _PyInterpreterState_GET()->callable_cache.list_append;
        if (ref_get_const(ctx, callable) == list_append) {
            REPLACE_OP(this_instr, _NOP, 0, 0);
        }
        ref_set_const(callable, list_append);
    }

// END BYTECODES //

}
