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
#define sym_has_type _Py_uop_sym_has_type
#define sym_set_null _Py_uop_sym_set_null
#define sym_set_non_null _Py_uop_sym_set_non_null
#define sym_set_type _Py_uop_sym_set_type
#define sym_set_const _Py_uop_sym_set_const
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
eliminate_pop_guard(_PyUOpInstruction *this_instr, bool exit)

static int
dummy_func(void) {

    PyCodeObject *code;
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
    _Py_UOpsContext *ctx;
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
        _Py_UopsSymbol *temp;
        OUT_OF_SPACE_IF_NULL(temp = sym_new_null(ctx));
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
        if (!sym_set_type(left, &PyLong_Type)) {
            goto hit_bottom;
        }
        if (!sym_set_type(right, &PyLong_Type)) {
            goto hit_bottom;
        }
    }

    op(_GUARD_BOTH_FLOAT, (left, right -- left, right)) {
        if (sym_matches_type(left, &PyFloat_Type) &&
            sym_matches_type(right, &PyFloat_Type)) {
            REPLACE_OP(this_instr, _NOP, 0 ,0);
        }
        if (!sym_set_type(left, &PyFloat_Type)) {
            goto hit_bottom;
        }
        if (!sym_set_type(right, &PyFloat_Type)) {
            goto hit_bottom;
        }
    }

    op(_GUARD_BOTH_UNICODE, (left, right -- left, right)) {
        if (sym_matches_type(left, &PyUnicode_Type) &&
            sym_matches_type(right, &PyUnicode_Type)) {
            REPLACE_OP(this_instr, _NOP, 0 ,0);
        }
        if (!sym_set_type(left, &PyUnicode_Type)) {
            goto hit_bottom;
        }
        if (!sym_set_type(right, &PyUnicode_Type)) {
            goto hit_bottom;
        }
    }

    op(_BINARY_OP_ADD_INT, (left, right -- res)) {
        if (sym_is_const(left) && sym_is_const(right) &&
            sym_matches_type(left, &PyLong_Type) && sym_matches_type(right, &PyLong_Type))
        {
            assert(PyLong_CheckExact(sym_get_const(left)));
            assert(PyLong_CheckExact(sym_get_const(right)));
            PyObject *temp = _PyLong_Add((PyLongObject *)sym_get_const(left),
                                         (PyLongObject *)sym_get_const(right));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
            OUT_OF_SPACE_IF_NULL(res);
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyLong_Type));
        }
    }

    op(_BINARY_OP_SUBTRACT_INT, (left, right -- res)) {
        if (sym_is_const(left) && sym_is_const(right) &&
            sym_matches_type(left, &PyLong_Type) && sym_matches_type(right, &PyLong_Type))
        {
            assert(PyLong_CheckExact(sym_get_const(left)));
            assert(PyLong_CheckExact(sym_get_const(right)));
            PyObject *temp = _PyLong_Subtract((PyLongObject *)sym_get_const(left),
                                              (PyLongObject *)sym_get_const(right));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
            OUT_OF_SPACE_IF_NULL(res);
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyLong_Type));
        }
    }

    op(_BINARY_OP_MULTIPLY_INT, (left, right -- res)) {
        if (sym_is_const(left) && sym_is_const(right) &&
            sym_matches_type(left, &PyLong_Type) && sym_matches_type(right, &PyLong_Type))
        {
            assert(PyLong_CheckExact(sym_get_const(left)));
            assert(PyLong_CheckExact(sym_get_const(right)));
            PyObject *temp = _PyLong_Multiply((PyLongObject *)sym_get_const(left),
                                              (PyLongObject *)sym_get_const(right));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
            OUT_OF_SPACE_IF_NULL(res);
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyLong_Type));
        }
    }

    op(_BINARY_OP_ADD_FLOAT, (left, right -- res)) {
        if (sym_is_const(left) && sym_is_const(right) &&
            sym_matches_type(left, &PyFloat_Type) && sym_matches_type(right, &PyFloat_Type))
        {
            assert(PyFloat_CheckExact(sym_get_const(left)));
            assert(PyFloat_CheckExact(sym_get_const(right)));
            PyObject *temp = PyFloat_FromDouble(
                PyFloat_AS_DOUBLE(sym_get_const(left)) +
                PyFloat_AS_DOUBLE(sym_get_const(right)));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
            OUT_OF_SPACE_IF_NULL(res);
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyFloat_Type));
        }
    }

    op(_BINARY_OP_SUBTRACT_FLOAT, (left, right -- res)) {
        if (sym_is_const(left) && sym_is_const(right) &&
            sym_matches_type(left, &PyFloat_Type) && sym_matches_type(right, &PyFloat_Type))
        {
            assert(PyFloat_CheckExact(sym_get_const(left)));
            assert(PyFloat_CheckExact(sym_get_const(right)));
            PyObject *temp = PyFloat_FromDouble(
                PyFloat_AS_DOUBLE(sym_get_const(left)) -
                PyFloat_AS_DOUBLE(sym_get_const(right)));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
            OUT_OF_SPACE_IF_NULL(res);
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyFloat_Type));
        }
    }

    op(_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res)) {
        if (sym_is_const(left) && sym_is_const(right) &&
            sym_matches_type(left, &PyFloat_Type) && sym_matches_type(right, &PyFloat_Type))
        {
            assert(PyFloat_CheckExact(sym_get_const(left)));
            assert(PyFloat_CheckExact(sym_get_const(right)));
            PyObject *temp = PyFloat_FromDouble(
                PyFloat_AS_DOUBLE(sym_get_const(left)) *
                PyFloat_AS_DOUBLE(sym_get_const(right)));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
            OUT_OF_SPACE_IF_NULL(res);
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyFloat_Type));
        }
    }

    op(_BINARY_OP_ADD_UNICODE, (left, right -- res)) {
        if (sym_is_const(left) && sym_is_const(right) &&
            sym_matches_type(left, &PyUnicode_Type) && sym_matches_type(right, &PyUnicode_Type)) {
            PyObject *temp = PyUnicode_Concat(sym_get_const(left), sym_get_const(right));
            if (temp == NULL) {
                goto error;
            }
            res = sym_new_const(ctx, temp);
            Py_DECREF(temp);
            OUT_OF_SPACE_IF_NULL(res);
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyUnicode_Type));
        }
    }

    op(_TO_BOOL, (value -- res)) {
        if (optimize_to_bool(this_instr, ctx, value, &res)) {
            OUT_OF_SPACE_IF_NULL(res);
        }
        else {
            res = sym_new_type(ctx, &PyBool_Type);
            OUT_OF_SPACE_IF_NULL(res);
        }
    }

    op(_TO_BOOL_BOOL, (value -- res)) {
        if (optimize_to_bool(this_instr, ctx, value, &res)) {
            OUT_OF_SPACE_IF_NULL(res);
        }
        else {
            if(!sym_set_type(value, &PyBool_Type)) {
                goto hit_bottom;
            }
            res = value;
        }
    }

    op(_TO_BOOL_INT, (value -- res)) {
        if (optimize_to_bool(this_instr, ctx, value, &res)) {
            OUT_OF_SPACE_IF_NULL(res);
        }
        else {
            if(!sym_set_type(value, &PyLong_Type)) {
                goto hit_bottom;
            }
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
        }
    }

    op(_TO_BOOL_LIST, (value -- res)) {
        if (optimize_to_bool(this_instr, ctx, value, &res)) {
            OUT_OF_SPACE_IF_NULL(res);
        }
        else {
            if(!sym_set_type(value, &PyList_Type)) {
                goto hit_bottom;
            }
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
        }
    }

    op(_TO_BOOL_NONE, (value -- res)) {
        if (optimize_to_bool(this_instr, ctx, value, &res)) {
            OUT_OF_SPACE_IF_NULL(res);
        }
        else {
            if (!sym_set_const(value, Py_None)) {
                goto hit_bottom;
            }
            OUT_OF_SPACE_IF_NULL(res = sym_new_const(ctx, Py_False));
        }
    }

    op(_TO_BOOL_STR, (value -- res)) {
        if (optimize_to_bool(this_instr, ctx, value, &res)) {
            OUT_OF_SPACE_IF_NULL(res);
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
            if(!sym_set_type(value, &PyUnicode_Type)) {
                goto hit_bottom;
            }
        }
    }

    op(_COMPARE_OP, (left, right -- res)) {
        (void)left;
        (void)right;
        if (oparg & 16) {
            OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
        }
        else {
            OUT_OF_SPACE_IF_NULL(res = _Py_uop_sym_new_not_null(ctx));
        }
    }

    op(_COMPARE_OP_INT, (left, right -- res)) {
        (void)left;
        (void)right;
        OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
    }

    op(_COMPARE_OP_FLOAT, (left, right -- res)) {
        (void)left;
        (void)right;
        OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
    }

    op(_COMPARE_OP_STR, (left, right -- res)) {
        (void)left;
        (void)right;
        OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
    }

    op(_IS_OP, (left, right -- res)) {
        (void)left;
        (void)right;
        OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
    }

    op(_CONTAINS_OP, (left, right -- res)) {
        (void)left;
        (void)right;
        OUT_OF_SPACE_IF_NULL(res = sym_new_type(ctx, &PyBool_Type));
    }

    op(_LOAD_CONST, (-- value)) {
        // There should be no LOAD_CONST. It should be all
        // replaced by peephole_opt.
        Py_UNREACHABLE();
    }

    op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
        OUT_OF_SPACE_IF_NULL(value = sym_new_const(ctx, ptr));
    }

    op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
        OUT_OF_SPACE_IF_NULL(value = sym_new_const(ctx, ptr));
    }

    op(_LOAD_CONST_INLINE_WITH_NULL, (ptr/4 -- value, null)) {
        OUT_OF_SPACE_IF_NULL(value = sym_new_const(ctx, ptr));
        OUT_OF_SPACE_IF_NULL(null = sym_new_null(ctx));
    }

    op(_LOAD_CONST_INLINE_BORROW_WITH_NULL, (ptr/4 -- value, null)) {
        OUT_OF_SPACE_IF_NULL(value = sym_new_const(ctx, ptr));
        OUT_OF_SPACE_IF_NULL(null = sym_new_null(ctx));
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

    op(_CHECK_ATTR_MODULE, (dict_version/2, owner -- owner)) {
        (void)dict_version;
        if (sym_is_const(owner)) {
            PyObject *cnst = sym_get_const(owner);
            if (PyModule_CheckExact(cnst)) {
                PyModuleObject *mod = (PyModuleObject *)cnst;
                PyObject *dict = mod->md_dict;
                uint64_t watched_mutations = get_mutations(dict);
                if (watched_mutations < _Py_MAX_ALLOWED_GLOBALS_MODIFICATIONS) {
                    PyDict_Watch(GLOBALS_WATCHER_ID, dict);
                    _Py_BloomFilter_Add(dependencies, dict);
                    this_instr->opcode = _NOP;
                }
            }
        }
    }

    op(_LOAD_ATTR_MODULE, (index/1, owner -- attr, null if (oparg & 1))) {
        (void)index;
        OUT_OF_SPACE_IF_NULL(null = sym_new_null(ctx));
        attr = NULL;
        if (this_instr[-1].opcode == _NOP) {
            // Preceding _CHECK_ATTR_MODULE was removed: mod is const and dict is watched.
            assert(sym_is_const(owner));
            PyModuleObject *mod = (PyModuleObject *)sym_get_const(owner);
            assert(PyModule_CheckExact(mod));
            PyObject *dict = mod->md_dict;
            PyObject *res = convert_global_to_const(this_instr, dict);
            if (res != NULL) {
                this_instr[-1].opcode = _POP_TOP;
                OUT_OF_SPACE_IF_NULL(attr = sym_new_const(ctx, res));
            }
        }
        if (attr == NULL) {
            /* No conversion made. We don't know what `attr` is. */
            OUT_OF_SPACE_IF_NULL(attr = sym_new_not_null(ctx));
        }
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

    op(_LOAD_ATTR_METHOD_WITH_VALUES, (descr/4, owner -- attr, self if (1))) {
        (void)descr;
        OUT_OF_SPACE_IF_NULL(attr = sym_new_not_null(ctx));
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_NO_DICT, (descr/4, owner -- attr, self if (1))) {
        (void)descr;
        OUT_OF_SPACE_IF_NULL(attr = sym_new_not_null(ctx));
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_LAZY_DICT, (descr/4, owner -- attr, self if (1))) {
        (void)descr;
        OUT_OF_SPACE_IF_NULL(attr = sym_new_not_null(ctx));
        self = owner;
    }

    op(_INIT_CALL_BOUND_METHOD_EXACT_ARGS, (callable, unused, unused[oparg] -- func, self, unused[oparg])) {
        (void)callable;
        OUT_OF_SPACE_IF_NULL(func = sym_new_not_null(ctx));
        OUT_OF_SPACE_IF_NULL(self = sym_new_not_null(ctx));
    }


    op(_CHECK_FUNCTION_EXACT_ARGS, (func_version/2, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        if (!sym_set_type(callable, &PyFunction_Type)) {
            goto hit_bottom;
        }
        (void)self_or_null;
        (void)func_version;
    }

    op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null, unused[oparg] -- callable, null, unused[oparg])) {
        if (!sym_set_null(null)) {
            goto hit_bottom;
        }
        if (!sym_set_type(callable, &PyMethod_Type)) {
            goto hit_bottom;
        }
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

        _Py_UopsSymbol **localsplus_start = ctx->n_consumed;
        int n_locals_already_filled = 0;
        // Can determine statically, so we interleave the new locals
        // and make the current stack the new locals.
        // This also sets up for true call inlining.
        if (sym_is_null(self_or_null) || sym_is_not_null(self_or_null)) {
            localsplus_start = args;
            n_locals_already_filled = argcount;
        }
        OUT_OF_SPACE_IF_NULL(new_frame =
                             frame_new(ctx, co, localsplus_start, n_locals_already_filled, 0));
    }

    op(_POP_FRAME, (retval -- res)) {
        SYNC_SP();
        ctx->frame->stack_pointer = stack_pointer;
        frame_pop(ctx);
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
            OUT_OF_SPACE_IF_NULL(values[i] = sym_new_unknown(ctx));
        }
    }

    op(_UNPACK_EX, (seq -- values[oparg & 0xFF], unused, unused[oparg >> 8])) {
        /* This has to be done manually */
        (void)seq;
        int totalargs = (oparg & 0xFF) + (oparg >> 8) + 1;
        for (int i = 0; i < totalargs; i++) {
            OUT_OF_SPACE_IF_NULL(values[i] = sym_new_unknown(ctx));
        }
    }

    op(_ITER_NEXT_RANGE, (iter -- iter, next)) {
       OUT_OF_SPACE_IF_NULL(next = sym_new_type(ctx, &PyLong_Type));
       (void)iter;
    }

    op(_GUARD_IS_TRUE_POP, (flag -- )) {
        if (sym_is_const(flag)) {
            PyObject *value = sym_get_const(flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, value != Py_True);
        }
    }

    op(_GUARD_IS_FALSE_POP, (flag -- )) {
        if (sym_is_const(flag)) {
            PyObject *value = sym_get_const(flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, value != Py_False);
        }
    }

    op(_GUARD_IS_NONE_POP, (flag -- )) {
        if (sym_is_const(flag)) {
            PyObject *value = sym_get_const(flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, !Py_IsNone(value));
        }
        else if (sym_has_type(flag)) {
            assert(!sym_matches_type(flag, &_PyNone_Type));
            eliminate_pop_guard(this_instr, true);
        }
    }

    op(_GUARD_IS_NOT_NONE_POP, (flag -- )) {
        if (sym_is_const(flag)) {
            PyObject *value = sym_get_const(flag);
            assert(value != NULL);
            eliminate_pop_guard(this_instr, Py_IsNone(value));
        }
        else if (sym_has_type(flag)) {
            assert(!sym_matches_type(flag, &_PyNone_Type));
            eliminate_pop_guard(this_instr, false);
        }
    }


// END BYTECODES //

}
