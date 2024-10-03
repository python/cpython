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

    op(_GUARD_BOTH_INT, (left, right -- left, right)) {
        if (sym_matches_type(left, &PyLong_Type)) {
            if (sym_matches_type(right, &PyLong_Type)) {
                REPLACE_OP(this_instr, _NOP, 0, 0);
            }
            else {
                REPLACE_OP(this_instr, _GUARD_TOS_INT, 0, 0);
            }
        }
        else {
            if (sym_matches_type(right, &PyLong_Type)) {
                REPLACE_OP(this_instr, _GUARD_NOS_INT, 0, 0);
            }
        }
        sym_set_type(left, &PyLong_Type);
        sym_set_type(right, &PyLong_Type);
    }

    op(_GUARD_TYPE_VERSION, (type_version/2, owner -- owner)) {
        assert(type_version);
        if (sym_matches_type_version(owner, type_version)) {
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
                if (sym_set_type_version(owner, type_version)) {
                    PyType_Watch(TYPE_WATCHER_ID, (PyObject *)type);
                    _Py_BloomFilter_Add(dependencies, type);
                }
            }

        }
    }

    op(_GUARD_BOTH_FLOAT, (left, right -- left, right)) {
        if (sym_matches_type(left, &PyFloat_Type)) {
            if (sym_matches_type(right, &PyFloat_Type)) {
                REPLACE_OP(this_instr, _NOP, 0, 0);
            }
            else {
                REPLACE_OP(this_instr, _GUARD_TOS_FLOAT, 0, 0);
            }
        }
        else {
            if (sym_matches_type(right, &PyFloat_Type)) {
                REPLACE_OP(this_instr, _GUARD_NOS_FLOAT, 0, 0);
            }
        }

        sym_set_type(left, &PyFloat_Type);
        sym_set_type(right, &PyFloat_Type);
    }

    op(_GUARD_BOTH_UNICODE, (left, right -- left, right)) {
        if (sym_matches_type(left, &PyUnicode_Type) &&
            sym_matches_type(right, &PyUnicode_Type)) {
            REPLACE_OP(this_instr, _NOP, 0 ,0);
        }
        sym_set_type(left, &PyUnicode_Type);
        sym_set_type(left, &PyUnicode_Type);
    }

    op(_BINARY_OP, (left, right -- res)) {
        PyTypeObject *ltype = sym_get_type(left);
        PyTypeObject *rtype = sym_get_type(right);
        if (ltype != NULL && (ltype == &PyLong_Type || ltype == &PyFloat_Type) &&
            rtype != NULL && (rtype == &PyLong_Type || rtype == &PyFloat_Type))
        {
            if (oparg != NB_TRUE_DIVIDE && oparg != NB_INPLACE_TRUE_DIVIDE &&
                ltype == &PyLong_Type && rtype == &PyLong_Type) {
                /* If both inputs are ints and the op is not division the result is an int */
                res = sym_new_type(ctx, &PyLong_Type);
            }
            else {
                /* For any other op combining ints/floats the result is a float */
                res = sym_new_type(ctx, &PyFloat_Type);
            }
        }
        res = sym_new_unknown(ctx);
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
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            res = sym_new_type(ctx, &PyLong_Type);
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
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            res = sym_new_type(ctx, &PyLong_Type);
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
            // TODO gh-115506:
            // replace opcode with constant propagated one and add tests!
        }
        else {
            res = sym_new_type(ctx, &PyLong_Type);
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
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            res = sym_new_type(ctx, &PyFloat_Type);
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
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            res = sym_new_type(ctx, &PyFloat_Type);
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
            // TODO gh-115506:
            // replace opcode with constant propagated one and update tests!
        }
        else {
            res = sym_new_type(ctx, &PyFloat_Type);
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
        }
        else {
            res = sym_new_type(ctx, &PyUnicode_Type);
        }
    }

    op(_BINARY_SUBSCR_INIT_CALL, (container, sub -- new_frame: _Py_UOpsAbstractFrame *)) {
        (void)container;
        (void)sub;
        new_frame = NULL;
        ctx->done = true;
    }

    op(_TO_BOOL, (value -- res)) {
        if (!optimize_to_bool(this_instr, ctx, value, &res)) {
            res = sym_new_type(ctx, &PyBool_Type);
        }
    }

    op(_TO_BOOL_BOOL, (value -- res)) {
        if (!optimize_to_bool(this_instr, ctx, value, &res)) {
            sym_set_type(value, &PyBool_Type);
            res = value;
        }
    }

    op(_TO_BOOL_INT, (value -- res)) {
        if (!optimize_to_bool(this_instr, ctx, value, &res)) {
            sym_set_type(value, &PyLong_Type);
            res = sym_new_type(ctx, &PyBool_Type);
        }
    }

    op(_TO_BOOL_LIST, (value -- res)) {
        if (!optimize_to_bool(this_instr, ctx, value, &res)) {
            sym_set_type(value, &PyList_Type);
            res = sym_new_type(ctx, &PyBool_Type);
        }
    }

    op(_TO_BOOL_NONE, (value -- res)) {
        if (!optimize_to_bool(this_instr, ctx, value, &res)) {
            sym_set_const(value, Py_None);
            res = sym_new_const(ctx, Py_False);
        }
    }

    op(_TO_BOOL_STR, (value -- res)) {
        if (!optimize_to_bool(this_instr, ctx, value, &res)) {
            res = sym_new_type(ctx, &PyBool_Type);
            sym_set_type(value, &PyUnicode_Type);
        }
    }

    op(_COMPARE_OP, (left, right -- res)) {
        (void)left;
        (void)right;
        if (oparg & 16) {
            res = sym_new_type(ctx, &PyBool_Type);
        }
        else {
            res = _Py_uop_sym_new_not_null(ctx);
        }
    }

    op(_COMPARE_OP_INT, (left, right -- res)) {
        (void)left;
        (void)right;
        res = sym_new_type(ctx, &PyBool_Type);
    }

    op(_COMPARE_OP_FLOAT, (left, right -- res)) {
        (void)left;
        (void)right;
        res = sym_new_type(ctx, &PyBool_Type);
    }

    op(_COMPARE_OP_STR, (left, right -- res)) {
        (void)left;
        (void)right;
        res = sym_new_type(ctx, &PyBool_Type);
    }

    op(_IS_OP, (left, right -- res)) {
        (void)left;
        (void)right;
        res = sym_new_type(ctx, &PyBool_Type);
    }

    op(_CONTAINS_OP, (left, right -- res)) {
        (void)left;
        (void)right;
        res = sym_new_type(ctx, &PyBool_Type);
    }

    op(_LOAD_CONST, (-- value)) {
        PyObject *val = PyTuple_GET_ITEM(co->co_consts, this_instr->oparg);
        int opcode = _Py_IsImmortal(val) ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
        REPLACE_OP(this_instr, opcode, 0, (uintptr_t)val);
        value = sym_new_const(ctx, val);
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

    op(_LOAD_ATTR_INSTANCE_VALUE, (offset/1, owner -- attr, null if (oparg & 1))) {
        attr = sym_new_not_null(ctx);
        null = sym_new_null(ctx);
        (void)offset;
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

    op(_LOAD_ATTR, (owner -- attr, self_or_null if (oparg & 1))) {
        (void)owner;
        attr = sym_new_not_null(ctx);
        if (oparg & 1) {
            self_or_null = sym_new_unknown(ctx);
        }
    }

    op(_LOAD_ATTR_MODULE, (index/1, owner -- attr, null if (oparg & 1))) {
        (void)index;
        null = sym_new_null(ctx);
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
                attr = sym_new_const(ctx, res);
            }
        }
        if (attr == NULL) {
            /* No conversion made. We don't know what `attr` is. */
            attr = sym_new_not_null(ctx);
        }
    }

    op(_LOAD_ATTR_WITH_HINT, (hint/1, owner -- attr, null if (oparg & 1))) {
        attr = sym_new_not_null(ctx);
        null = sym_new_null(ctx);
        (void)hint;
        (void)owner;
    }

    op(_LOAD_ATTR_SLOT, (index/1, owner -- attr, null if (oparg & 1))) {
        attr = sym_new_not_null(ctx);
        null = sym_new_null(ctx);
        (void)index;
        (void)owner;
    }

    op(_LOAD_ATTR_CLASS, (descr/4, owner -- attr, null if (oparg & 1))) {
        attr = sym_new_not_null(ctx);
        null = sym_new_null(ctx);
        (void)descr;
        (void)owner;
    }

    op(_LOAD_ATTR_METHOD_WITH_VALUES, (descr/4, owner -- attr, self if (1))) {
        (void)descr;
        attr = sym_new_not_null(ctx);
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_NO_DICT, (descr/4, owner -- attr, self if (1))) {
        (void)descr;
        attr = sym_new_not_null(ctx);
        self = owner;
    }

    op(_LOAD_ATTR_METHOD_LAZY_DICT, (descr/4, owner -- attr, self if (1))) {
        (void)descr;
        attr = sym_new_not_null(ctx);
        self = owner;
    }

    op(_LOAD_ATTR_PROPERTY_FRAME, (fget/4, owner -- new_frame: _Py_UOpsAbstractFrame *)) {
        (void)fget;
        (void)owner;
        new_frame = NULL;
        ctx->done = true;
    }

    op(_INIT_CALL_BOUND_METHOD_EXACT_ARGS, (callable, unused, unused[oparg] -- func, self, unused[oparg])) {
        (void)callable;
        func = sym_new_not_null(ctx);
        self = sym_new_not_null(ctx);
    }

    op(_CHECK_FUNCTION_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
        sym_set_type(callable, &PyFunction_Type);
        (void)self_or_null;
    }

    op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null, unused[oparg] -- callable, null, unused[oparg])) {
        sym_set_null(null);
        sym_set_type(callable, &PyMethod_Type);
    }

    op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null, args[oparg] -- new_frame: _Py_UOpsAbstractFrame *)) {
        int argcount = oparg;

        (void)callable;

        PyCodeObject *co = NULL;
        assert((this_instr + 2)->opcode == _PUSH_FRAME);
        co = get_code_with_logging((this_instr + 2));
        if (co == NULL) {
            ctx->done = true;
            break;
        }


        assert(self_or_null != NULL);
        assert(args != NULL);
        if (sym_is_not_null(self_or_null)) {
            // Bound method fiddling, same as _INIT_CALL_PY_EXACT_ARGS in VM
            args--;
            argcount++;
        }

        if (sym_is_null(self_or_null) || sym_is_not_null(self_or_null)) {
            new_frame = frame_new(ctx, co, 0, args, argcount);
        } else {
            new_frame = frame_new(ctx, co, 0, NULL, 0);

        }
    }

    op(_MAYBE_EXPAND_METHOD, (callable, self_or_null, args[oparg] -- func, maybe_self, args[oparg])) {
        (void)callable;
        (void)self_or_null;
        (void)args;
        func = sym_new_not_null(ctx);
        maybe_self = sym_new_not_null(ctx);
    }

    op(_PY_FRAME_GENERAL, (callable, self_or_null, args[oparg] -- new_frame: _Py_UOpsAbstractFrame *)) {
        (void)(self_or_null);
        (void)(callable);
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
        (void)callable;
        (void)self_or_null;
        (void)args;
        (void)kwnames;
        new_frame = NULL;
        ctx->done = true;
    }

    op(_CHECK_AND_ALLOCATE_OBJECT, (type_version/2, callable, null, args[oparg] -- self, init, args[oparg])) {
        (void)type_version;
        (void)callable;
        (void)null;
        (void)args;
        self = sym_new_not_null(ctx);
        init = sym_new_not_null(ctx);
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

    op(_CHECK_STACK_SPACE, ( --)) {
        assert(corresponding_check_stack == NULL);
        corresponding_check_stack = this_instr;
    }

    op (_CHECK_STACK_SPACE_OPERAND, ( -- )) {
        (void)framesize;
        /* We should never see _CHECK_STACK_SPACE_OPERANDs.
        * They are only created at the end of this pass. */
        Py_UNREACHABLE();
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

    op(_ITER_NEXT_RANGE, (iter -- iter, next)) {
       next = sym_new_type(ctx, &PyLong_Type);
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

    op(_CHECK_PEP_523, (--)) {
        /* Setting the eval frame function invalidates
        * all executors, so no need to check dynamically */
        if (_PyInterpreterState_GET()->eval_frame == NULL) {
            REPLACE_OP(this_instr, _NOP, 0 ,0);
        }
    }

    op(_LOAD_SPECIAL, (owner -- attr, self_or_null)) {
        (void)owner;
        attr = sym_new_not_null(ctx);
        self_or_null = sym_new_unknown(ctx);
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
