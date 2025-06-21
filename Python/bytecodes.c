// This file contains instruction definitions.
// It is read by generators stored in Tools/cases_generator/
// to generate Python/generated_cases.c.h and others.
// Note that there is some dummy C code at the top and bottom of the file
// to fool text editors like VS Code into believing this is valid C code.
// The actual instruction definitions start at // BEGIN BYTECODES //.
// See Tools/cases_generator/README.md for more information.

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_audit.h"         // _PySys_Audit()
#include "pycore_backoff.h"
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_ceval.h"
#include "pycore_code.h"
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS
#include "pycore_function.h"
#include "pycore_instruments.h"
#include "pycore_interpolation.h" // _PyInterpolation_Build()
#include "pycore_intrinsics.h"
#include "pycore_long.h"          // _PyLong_ExactDealloc(), _PyLong_GetZero()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_opcode_metadata.h"  // uop names
#include "pycore_opcode_utils.h"  // MAKE_FUNCTION_*
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_*
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include "pycore_sliceobject.h"   // _PyBuildSlice_ConsumeRefs
#include "pycore_stackref.h"
#include "pycore_template.h"      // _PyTemplate_Build()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_typeobject.h"    // _PySuper_Lookup()

#include "pycore_dict.h"
#include "dictobject.h"
#include "pycore_frame.h"
#include "opcode.h"
#include "optimizer.h"
#include "pydtrace.h"
#include "setobject.h"


#define USE_COMPUTED_GOTOS 0
#include "ceval_macros.h"

/* Flow control macros */

#define inst(name, ...) case name:
#define op(name, ...) /* NAME is ignored */
#define macro(name) static int MACRO_##name
#define super(name) static int SUPER_##name
#define family(name, ...) static int family_##name
#define pseudo(name) static int pseudo_##name
#define label(name) name:

/* Annotations */
#define guard
#define override
#define specializing
#define replicate(TIMES)
#define tier1
#define no_save_ip

// Dummy variables for stack effects.
static PyObject *value, *value1, *value2, *left, *right, *res, *sum, *prod, *sub;
static PyObject *container, *start, *stop, *v, *lhs, *rhs, *res2;
static PyObject *list, *tuple, *dict, *owner, *set, *str, *tup, *map, *keys;
static PyObject *exit_func, *lasti, *val, *retval, *obj, *iter, *exhausted;
static PyObject *aiter, *awaitable, *iterable, *w, *exc_value, *bc, *locals;
static PyObject *orig, *excs, *update, *b, *fromlist, *level, *from;
static PyObject **pieces, **values;
static size_t jump;
// Dummy variables for cache effects
static uint16_t invert, counter, index, hint;
#define unused 0  // Used in a macro def, can't be static
static uint32_t type_version;
static _PyExecutorObject *current_executor;

static PyObject *
dummy_func(
    PyThreadState *tstate,
    _PyInterpreterFrame *frame,
    unsigned char opcode,
    unsigned int oparg,
    _Py_CODEUNIT *next_instr,
    PyObject **stack_pointer,
    int throwflag,
    PyObject *args[]
)
{
    // Dummy labels.
    pop_1_error:
    // Dummy locals.
    PyObject *dummy;
    _Py_CODEUNIT *this_instr;
    PyObject *attr;
    PyObject *attrs;
    PyObject *bottom;
    PyObject *callable;
    PyObject *callargs;
    PyObject *codeobj;
    PyObject *cond;
    PyObject *descr;
    PyObject *exc;
    PyObject *exit;
    PyObject *fget;
    PyObject *fmt_spec;
    PyObject *func;
    uint32_t func_version;
    PyObject *getattribute;
    PyObject *kwargs;
    PyObject *kwdefaults;
    PyObject *len_o;
    PyObject *match;
    PyObject *match_type;
    PyObject *method;
    PyObject *mgr;
    Py_ssize_t min_args;
    PyObject *names;
    PyObject *new_exc;
    PyObject *next;
    PyObject *none;
    PyObject *null;
    PyObject *prev_exc;
    PyObject *receiver;
    PyObject *rest;
    int result;
    PyObject *self;
    PyObject *seq;
    PyObject *slice;
    PyObject *step;
    PyObject *subject;
    PyObject *top;
    PyObject *type;
    PyObject *typevars;
    PyObject *val0;
    PyObject *val1;
    int values_or_none;

    switch (opcode) {

// BEGIN BYTECODES //
        pure inst(NOP, (--)) {
        }

        family(RESUME, 0) = {
            RESUME_CHECK,
        };

        macro(NOT_TAKEN) = NOP;

        op(_CHECK_PERIODIC, (--)) {
            _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
            QSBR_QUIESCENT_STATE(tstate);
            if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
                int err = _Py_HandlePending(tstate);
                ERROR_IF(err != 0);
            }
        }

        op(_CHECK_PERIODIC_IF_NOT_YIELD_FROM, (--)) {
            if ((oparg & RESUME_OPARG_LOCATION_MASK) < RESUME_AFTER_YIELD_FROM) {
                _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
                QSBR_QUIESCENT_STATE(tstate);
                if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
                    int err = _Py_HandlePending(tstate);
                    ERROR_IF(err != 0);
                }
            }
        }

        op(_QUICKEN_RESUME, (--)) {
            #if ENABLE_SPECIALIZATION_FT
            if (tstate->tracing == 0 && this_instr->op.code == RESUME) {
                FT_ATOMIC_STORE_UINT8_RELAXED(this_instr->op.code, RESUME_CHECK);
            }
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        tier1 op(_MAYBE_INSTRUMENT, (--)) {
            if (tstate->tracing == 0) {
                uintptr_t global_version = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & ~_PY_EVAL_EVENTS_MASK;
                uintptr_t code_version = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(_PyFrame_GetCode(frame)->_co_instrumentation_version);
                if (code_version != global_version) {
                    int err = _Py_Instrument(_PyFrame_GetCode(frame), tstate->interp);
                    if (err) {
                        ERROR_NO_POP();
                    }
                    next_instr = this_instr;
                    DISPATCH();
                }
            }
        }

        op(_LOAD_BYTECODE, (--)) {
            #ifdef Py_GIL_DISABLED
            if (frame->tlbc_index !=
                ((_PyThreadStateImpl *)tstate)->tlbc_index) {
                _Py_CODEUNIT *bytecode =
                    _PyEval_GetExecutableCode(tstate, _PyFrame_GetCode(frame));
                ERROR_IF(bytecode == NULL);
                ptrdiff_t off = this_instr - _PyFrame_GetBytecode(frame);
                frame->tlbc_index = ((_PyThreadStateImpl *)tstate)->tlbc_index;
                frame->instr_ptr = bytecode + off;
                // Make sure this_instr gets reset correctly for any uops that
                // follow
                next_instr = frame->instr_ptr;
                DISPATCH();
            }
            #endif
        }

        macro(RESUME) =
            _LOAD_BYTECODE +
            _MAYBE_INSTRUMENT +
            _QUICKEN_RESUME +
            _CHECK_PERIODIC_IF_NOT_YIELD_FROM;

        inst(RESUME_CHECK, (--)) {
#if defined(__EMSCRIPTEN__)
            DEOPT_IF(_Py_emscripten_signal_clock == 0);
            _Py_emscripten_signal_clock -= Py_EMSCRIPTEN_SIGNAL_HANDLING;
#endif
            uintptr_t eval_breaker = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker);
            uintptr_t version = FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(_PyFrame_GetCode(frame)->_co_instrumentation_version);
            assert((version & _PY_EVAL_EVENTS_MASK) == 0);
            DEOPT_IF(eval_breaker != version);
            #ifdef Py_GIL_DISABLED
            DEOPT_IF(frame->tlbc_index !=
                     ((_PyThreadStateImpl *)tstate)->tlbc_index);
            #endif
        }

        op(_MONITOR_RESUME, (--)) {
            int err = _Py_call_instrumentation(
                    tstate, oparg > 0, frame, this_instr);
            ERROR_IF(err);
            if (frame->instr_ptr != this_instr) {
                /* Instrumentation has jumped */
                next_instr = frame->instr_ptr;
            }
        }

        macro(INSTRUMENTED_RESUME) =
            _LOAD_BYTECODE +
            _MAYBE_INSTRUMENT +
            _CHECK_PERIODIC_IF_NOT_YIELD_FROM +
            _MONITOR_RESUME;

        pseudo(LOAD_CLOSURE, (-- unused)) = {
            LOAD_FAST,
        };

        inst(LOAD_FAST_CHECK, (-- value)) {
            _PyStackRef value_s = GETLOCAL(oparg);
            if (PyStackRef_IsNull(value_s)) {
                _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
                    UNBOUNDLOCAL_ERROR_MSG,
                    PyTuple_GetItem(_PyFrame_GetCode(frame)->co_localsplusnames, oparg)
                );
                ERROR_IF(true);
            }
            value = PyStackRef_DUP(value_s);
        }

        replicate(8) pure inst(LOAD_FAST, (-- value)) {
            assert(!PyStackRef_IsNull(GETLOCAL(oparg)));
            value = PyStackRef_DUP(GETLOCAL(oparg));
        }

        replicate(8) pure inst (LOAD_FAST_BORROW, (-- value)) {
            assert(!PyStackRef_IsNull(GETLOCAL(oparg)));
            value = PyStackRef_Borrow(GETLOCAL(oparg));
        }

        inst(LOAD_FAST_AND_CLEAR, (-- value)) {
            value = GETLOCAL(oparg);
            GETLOCAL(oparg) = PyStackRef_NULL;
        }

        inst(LOAD_FAST_LOAD_FAST, ( -- value1, value2)) {
            uint32_t oparg1 = oparg >> 4;
            uint32_t oparg2 = oparg & 15;
            value1 = PyStackRef_DUP(GETLOCAL(oparg1));
            value2 = PyStackRef_DUP(GETLOCAL(oparg2));
        }

        inst(LOAD_FAST_BORROW_LOAD_FAST_BORROW, ( -- value1, value2)) {
            uint32_t oparg1 = oparg >> 4;
            uint32_t oparg2 = oparg & 15;
            value1 = PyStackRef_Borrow(GETLOCAL(oparg1));
            value2 = PyStackRef_Borrow(GETLOCAL(oparg2));
        }

        inst(LOAD_CONST, (-- value)) {
            PyObject *obj = GETITEM(FRAME_CO_CONSTS, oparg);
            value = PyStackRef_FromPyObjectBorrow(obj);
        }

        replicate(4) inst(LOAD_SMALL_INT, (-- value)) {
            assert(oparg < _PY_NSMALLPOSINTS);
            PyObject *obj = (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS + oparg];
            value = PyStackRef_FromPyObjectBorrow(obj);
        }

        replicate(8) inst(STORE_FAST, (value --)) {
            _PyStackRef tmp = GETLOCAL(oparg);
            GETLOCAL(oparg) = value;
            DEAD(value);
            PyStackRef_XCLOSE(tmp);
        }

        pseudo(STORE_FAST_MAYBE_NULL, (unused --)) = {
            STORE_FAST,
        };

        inst(STORE_FAST_LOAD_FAST, (value1 -- value2)) {
            uint32_t oparg1 = oparg >> 4;
            uint32_t oparg2 = oparg & 15;
            _PyStackRef tmp = GETLOCAL(oparg1);
            GETLOCAL(oparg1) = value1;
            DEAD(value1);
            value2 = PyStackRef_DUP(GETLOCAL(oparg2));
            PyStackRef_XCLOSE(tmp);
        }

        inst(STORE_FAST_STORE_FAST, (value2, value1 --)) {
            uint32_t oparg1 = oparg >> 4;
            uint32_t oparg2 = oparg & 15;
            _PyStackRef tmp = GETLOCAL(oparg1);
            GETLOCAL(oparg1) = value1;
            DEAD(value1);
            PyStackRef_XCLOSE(tmp);
            tmp = GETLOCAL(oparg2);
            GETLOCAL(oparg2) = value2;
            DEAD(value2);
            PyStackRef_XCLOSE(tmp);
        }

        pure inst(POP_TOP, (value --)) {
            PyStackRef_XCLOSE(value);
        }

        tier2 op(_POP_TWO, (nos, tos --)) {
            PyStackRef_CLOSE(tos);
            PyStackRef_CLOSE(nos);
        }

        pure inst(PUSH_NULL, (-- res)) {
            res = PyStackRef_NULL;
        }

        no_save_ip inst(END_FOR, (value -- )) {
            /* Don't update instr_ptr, so that POP_ITER sees
             * the FOR_ITER as the previous instruction.
             * This has the benign side effect that if value is
             * finalized it will see the location as the FOR_ITER's.
             */
            PyStackRef_CLOSE(value);
        }


        inst(POP_ITER, (iter, index_or_null -- )) {
            (void)index_or_null;
            DEAD(index_or_null);
            PyStackRef_CLOSE(iter);
        }

        no_save_ip tier1 inst(INSTRUMENTED_END_FOR, (receiver, index_or_null, value -- receiver, index_or_null)) {
            /* Need to create a fake StopIteration error here,
             * to conform to PEP 380 */
            if (PyStackRef_GenCheck(receiver)) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, PyStackRef_AsPyObjectBorrow(value));
                if (err) {
                    ERROR_NO_POP();
                }
            }
            PyStackRef_CLOSE(value);
        }

        tier1 inst(INSTRUMENTED_POP_ITER, (iter, index_or_null -- )) {
            (void)index_or_null;
            DEAD(index_or_null);
            INSTRUMENTED_JUMP(prev_instr, this_instr+1, PY_MONITORING_EVENT_BRANCH_RIGHT);
            PyStackRef_CLOSE(iter);
        }

        pure inst(END_SEND, (receiver, value -- val)) {
            val = value;
            DEAD(value);
            PyStackRef_CLOSE(receiver);
        }

        tier1 inst(INSTRUMENTED_END_SEND, (receiver, value -- val)) {
            PyObject *receiver_o = PyStackRef_AsPyObjectBorrow(receiver);
            if (PyGen_Check(receiver_o) || PyCoro_CheckExact(receiver_o)) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, PyStackRef_AsPyObjectBorrow(value));
                if (err) {
                    ERROR_NO_POP();
                }
            }
            val = value;
            DEAD(value);
            PyStackRef_CLOSE(receiver);
        }

        inst(UNARY_NEGATIVE, (value -- res)) {
            PyObject *res_o = PyNumber_Negative(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure inst(UNARY_NOT, (value -- res)) {
            assert(PyStackRef_BoolCheck(value));
            res = PyStackRef_IsFalse(value)
                ? PyStackRef_True : PyStackRef_False;
            DEAD(value);
        }

        family(TO_BOOL, INLINE_CACHE_ENTRIES_TO_BOOL) = {
            TO_BOOL_ALWAYS_TRUE,
            TO_BOOL_BOOL,
            TO_BOOL_INT,
            TO_BOOL_LIST,
            TO_BOOL_NONE,
            TO_BOOL_STR,
        };

        specializing op(_SPECIALIZE_TO_BOOL, (counter/1, value -- value)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_ToBool(value, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(TO_BOOL);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        op(_TO_BOOL, (value -- res)) {
            int err = PyObject_IsTrue(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
            ERROR_IF(err < 0);
            res = err ? PyStackRef_True : PyStackRef_False;
        }

        macro(TO_BOOL) = _SPECIALIZE_TO_BOOL + unused/2 + _TO_BOOL;

        inst(TO_BOOL_BOOL, (unused/1, unused/2, value -- value)) {
            EXIT_IF(!PyStackRef_BoolCheck(value));
            STAT_INC(TO_BOOL, hit);
        }

        inst(TO_BOOL_INT, (unused/1, unused/2, value -- res)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            EXIT_IF(!PyLong_CheckExact(value_o));
            STAT_INC(TO_BOOL, hit);
            if (_PyLong_IsZero((PyLongObject *)value_o)) {
                assert(_Py_IsImmortal(value_o));
                DEAD(value);
                res = PyStackRef_False;
            }
            else {
                PyStackRef_CLOSE(value);
                res = PyStackRef_True;
            }
        }

        op(_GUARD_NOS_LIST, (nos, unused -- nos, unused)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(nos);
            EXIT_IF(!PyList_CheckExact(o));
        }

        op(_GUARD_TOS_LIST, (tos -- tos)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(tos);
            EXIT_IF(!PyList_CheckExact(o));
        }

        op(_GUARD_TOS_SLICE, (tos -- tos)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(tos);
            EXIT_IF(!PySlice_Check(o));
        }

        macro(TO_BOOL_LIST) = _GUARD_TOS_LIST + unused/1 + unused/2 + _TO_BOOL_LIST;

        op(_TO_BOOL_LIST, (value -- res)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            assert(PyList_CheckExact(value_o));
            STAT_INC(TO_BOOL, hit);
            res = PyList_GET_SIZE(value_o) ? PyStackRef_True : PyStackRef_False;
            DECREF_INPUTS();
        }

        inst(TO_BOOL_NONE, (unused/1, unused/2, value -- res)) {
            // This one is a bit weird, because we expect *some* failures:
            EXIT_IF(!PyStackRef_IsNone(value));
            DEAD(value);
            STAT_INC(TO_BOOL, hit);
            res = PyStackRef_False;
        }

        op(_GUARD_NOS_UNICODE, (nos, unused -- nos, unused)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(nos);
            EXIT_IF(!PyUnicode_CheckExact(o));
        }

        op(_GUARD_TOS_UNICODE, (value -- value)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            EXIT_IF(!PyUnicode_CheckExact(value_o));
        }

        op(_TO_BOOL_STR, (value -- res)) {
            STAT_INC(TO_BOOL, hit);
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            if (value_o == &_Py_STR(empty)) {
                assert(_Py_IsImmortal(value_o));
                DEAD(value);
                res = PyStackRef_False;
            }
            else {
                assert(Py_SIZE(value_o));
                PyStackRef_CLOSE(value);
                res = PyStackRef_True;
            }
        }

        macro(TO_BOOL_STR) =
            _GUARD_TOS_UNICODE + unused/1 + unused/2 + _TO_BOOL_STR;

        op(_REPLACE_WITH_TRUE, (value -- res)) {
            PyStackRef_CLOSE(value);
            res = PyStackRef_True;
        }

        macro(TO_BOOL_ALWAYS_TRUE) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _REPLACE_WITH_TRUE;

        inst(UNARY_INVERT, (value -- res)) {
            PyObject *res_o = PyNumber_Invert(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        family(BINARY_OP, INLINE_CACHE_ENTRIES_BINARY_OP) = {
            BINARY_OP_MULTIPLY_INT,
            BINARY_OP_ADD_INT,
            BINARY_OP_SUBTRACT_INT,
            BINARY_OP_MULTIPLY_FLOAT,
            BINARY_OP_ADD_FLOAT,
            BINARY_OP_SUBTRACT_FLOAT,
            BINARY_OP_ADD_UNICODE,
            BINARY_OP_SUBSCR_LIST_INT,
            BINARY_OP_SUBSCR_LIST_SLICE,
            BINARY_OP_SUBSCR_TUPLE_INT,
            BINARY_OP_SUBSCR_STR_INT,
            BINARY_OP_SUBSCR_DICT,
            BINARY_OP_SUBSCR_GETITEM,
            // BINARY_OP_INPLACE_ADD_UNICODE,  // See comments at that opcode.
            BINARY_OP_EXTEND,
        };

        op(_GUARD_NOS_INT, (left, unused -- left, unused)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            EXIT_IF(!_PyLong_CheckExactAndCompact(left_o));
        }

        op(_GUARD_TOS_INT, (value -- value)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            EXIT_IF(!_PyLong_CheckExactAndCompact(value_o));
        }

        op(_GUARD_NOS_OVERFLOWED, (left, unused -- left, unused)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            assert(Py_TYPE(left_o) == &PyLong_Type);
            EXIT_IF(!_PyLong_IsCompact((PyLongObject *)left_o));
        }

        op(_GUARD_TOS_OVERFLOWED, (value -- value)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            assert(Py_TYPE(value_o) == &PyLong_Type);
            EXIT_IF(!_PyLong_IsCompact((PyLongObject *)value_o));
        }

        pure op(_BINARY_OP_MULTIPLY_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyLong_CheckExact(left_o));
            assert(PyLong_CheckExact(right_o));
            assert(_PyLong_BothAreCompact((PyLongObject *)left_o, (PyLongObject *)right_o));

            STAT_INC(BINARY_OP, hit);
            res = _PyCompactLong_Multiply((PyLongObject *)left_o, (PyLongObject *)right_o);
            EXIT_IF(PyStackRef_IsNull(res));
            PyStackRef_CLOSE_SPECIALIZED(right, _PyLong_ExactDealloc);
            PyStackRef_CLOSE_SPECIALIZED(left, _PyLong_ExactDealloc);
            INPUTS_DEAD();
        }

        pure op(_BINARY_OP_ADD_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyLong_CheckExact(left_o));
            assert(PyLong_CheckExact(right_o));
            assert(_PyLong_BothAreCompact((PyLongObject *)left_o, (PyLongObject *)right_o));

            STAT_INC(BINARY_OP, hit);
            res = _PyCompactLong_Add((PyLongObject *)left_o, (PyLongObject *)right_o);
            EXIT_IF(PyStackRef_IsNull(res));
            PyStackRef_CLOSE_SPECIALIZED(right, _PyLong_ExactDealloc);
            PyStackRef_CLOSE_SPECIALIZED(left, _PyLong_ExactDealloc);
            INPUTS_DEAD();
        }

        pure op(_BINARY_OP_SUBTRACT_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyLong_CheckExact(left_o));
            assert(PyLong_CheckExact(right_o));
            assert(_PyLong_BothAreCompact((PyLongObject *)left_o, (PyLongObject *)right_o));

            STAT_INC(BINARY_OP, hit);
            res = _PyCompactLong_Subtract((PyLongObject *)left_o, (PyLongObject *)right_o);
            EXIT_IF(PyStackRef_IsNull(res));
            PyStackRef_CLOSE_SPECIALIZED(right, _PyLong_ExactDealloc);
            PyStackRef_CLOSE_SPECIALIZED(left, _PyLong_ExactDealloc);
            INPUTS_DEAD();
        }

        macro(BINARY_OP_MULTIPLY_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_MULTIPLY_INT;

        macro(BINARY_OP_ADD_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_ADD_INT;

        macro(BINARY_OP_SUBTRACT_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/5 + _BINARY_OP_SUBTRACT_INT;

        op(_GUARD_NOS_FLOAT, (left, unused -- left, unused)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            EXIT_IF(!PyFloat_CheckExact(left_o));
        }

        op(_GUARD_TOS_FLOAT, (value -- value)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            EXIT_IF(!PyFloat_CheckExact(value_o));
        }

        pure op(_BINARY_OP_MULTIPLY_FLOAT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyFloat_CheckExact(left_o));
            assert(PyFloat_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval *
                ((PyFloatObject *)right_o)->ob_fval;
            res = _PyFloat_FromDouble_ConsumeInputs(left, right, dres);
            INPUTS_DEAD();
            ERROR_IF(PyStackRef_IsNull(res));
        }

        pure op(_BINARY_OP_ADD_FLOAT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyFloat_CheckExact(left_o));
            assert(PyFloat_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval +
                ((PyFloatObject *)right_o)->ob_fval;
            res = _PyFloat_FromDouble_ConsumeInputs(left, right, dres);
            INPUTS_DEAD();
            ERROR_IF(PyStackRef_IsNull(res));
        }

        pure op(_BINARY_OP_SUBTRACT_FLOAT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyFloat_CheckExact(left_o));
            assert(PyFloat_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval -
                ((PyFloatObject *)right_o)->ob_fval;
            res = _PyFloat_FromDouble_ConsumeInputs(left, right, dres);
            INPUTS_DEAD();
            ERROR_IF(PyStackRef_IsNull(res));
        }


        pure op(_BINARY_OP_MULTIPLY_FLOAT__NO_DECREF_INPUTS, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyFloat_CheckExact(left_o));
            assert(PyFloat_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval *
                ((PyFloatObject *)right_o)->ob_fval;
            res = PyStackRef_FromPyObjectSteal(PyFloat_FromDouble(dres));
            INPUTS_DEAD();
            ERROR_IF(PyStackRef_IsNull(res));
        }

        pure op(_BINARY_OP_ADD_FLOAT__NO_DECREF_INPUTS, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyFloat_CheckExact(left_o));
            assert(PyFloat_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval +
                ((PyFloatObject *)right_o)->ob_fval;
            res = PyStackRef_FromPyObjectSteal(PyFloat_FromDouble(dres));
            INPUTS_DEAD();
            ERROR_IF(PyStackRef_IsNull(res));
        }

        pure op(_BINARY_OP_SUBTRACT_FLOAT__NO_DECREF_INPUTS, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyFloat_CheckExact(left_o));
            assert(PyFloat_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval -
                ((PyFloatObject *)right_o)->ob_fval;
            res = PyStackRef_FromPyObjectSteal(PyFloat_FromDouble(dres));
            INPUTS_DEAD();
            ERROR_IF(PyStackRef_IsNull(res));
        }

        macro(BINARY_OP_MULTIPLY_FLOAT) =
            _GUARD_TOS_FLOAT + _GUARD_NOS_FLOAT + unused/5 + _BINARY_OP_MULTIPLY_FLOAT;
        macro(BINARY_OP_ADD_FLOAT) =
            _GUARD_TOS_FLOAT + _GUARD_NOS_FLOAT + unused/5 + _BINARY_OP_ADD_FLOAT;
        macro(BINARY_OP_SUBTRACT_FLOAT) =
            _GUARD_TOS_FLOAT + _GUARD_NOS_FLOAT + unused/5 + _BINARY_OP_SUBTRACT_FLOAT;

        pure op(_BINARY_OP_ADD_UNICODE, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(PyUnicode_CheckExact(left_o));
            assert(PyUnicode_CheckExact(right_o));

            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = PyUnicode_Concat(left_o, right_o);
            PyStackRef_CLOSE_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            PyStackRef_CLOSE_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            INPUTS_DEAD();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_OP_ADD_UNICODE) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_ADD_UNICODE;

        // This is a subtle one. It's a super-instruction for
        // BINARY_OP_ADD_UNICODE followed by STORE_FAST
        // where the store goes into the left argument.
        // So the inputs are the same as for all BINARY_OP
        // specializations, but there is no output.
        // At the end we just skip over the STORE_FAST.
        op(_BINARY_OP_INPLACE_ADD_UNICODE, (left, right --)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            assert(PyUnicode_CheckExact(left_o));
            assert(PyUnicode_CheckExact(PyStackRef_AsPyObjectBorrow(right)));

            int next_oparg;
        #if TIER_ONE
            assert(next_instr->op.code == STORE_FAST);
            next_oparg = next_instr->op.arg;
        #else
            next_oparg = CURRENT_OPERAND0();
        #endif
            _PyStackRef *target_local = &GETLOCAL(next_oparg);
            assert(PyUnicode_CheckExact(left_o));
            DEOPT_IF(PyStackRef_AsPyObjectBorrow(*target_local) != left_o);
            STAT_INC(BINARY_OP, hit);
            /* Handle `left = left + right` or `left += right` for str.
             *
             * When possible, extend `left` in place rather than
             * allocating a new PyUnicodeObject. This attempts to avoid
             * quadratic behavior when one neglects to use str.join().
             *
             * If `left` has only two references remaining (one from
             * the stack, one in the locals), DECREFing `left` leaves
             * only the locals reference, so PyUnicode_Append knows
             * that the string is safe to mutate.
             */
            assert(Py_REFCNT(left_o) >= 2 || !PyStackRef_IsHeapSafe(left));
            PyStackRef_CLOSE_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            DEAD(left);
            PyObject *temp = PyStackRef_AsPyObjectSteal(*target_local);
            PyObject *right_o = PyStackRef_AsPyObjectSteal(right);
            PyUnicode_Append(&temp, right_o);
            *target_local = PyStackRef_FromPyObjectSteal(temp);
            Py_DECREF(right_o);
            ERROR_IF(PyStackRef_IsNull(*target_local));
        #if TIER_ONE
            // The STORE_FAST is already done. This is done here in tier one,
            // and during trace projection in tier two:
            assert(next_instr->op.code == STORE_FAST);
            SKIP_OVER(1);
        #endif
        }

       op(_GUARD_BINARY_OP_EXTEND, (descr/4, left, right -- left, right)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            _PyBinaryOpSpecializationDescr *d = (_PyBinaryOpSpecializationDescr*)descr;
            assert(INLINE_CACHE_ENTRIES_BINARY_OP == 5);
            assert(d && d->guard);
            int res = d->guard(left_o, right_o);
            DEOPT_IF(!res);
        }

        pure op(_BINARY_OP_EXTEND, (descr/4, left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            assert(INLINE_CACHE_ENTRIES_BINARY_OP == 5);
            _PyBinaryOpSpecializationDescr *d = (_PyBinaryOpSpecializationDescr*)descr;

            STAT_INC(BINARY_OP, hit);

            PyObject *res_o = d->action(left_o, right_o);
            DECREF_INPUTS();
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_OP_EXTEND) =
            unused/1 + _GUARD_BINARY_OP_EXTEND + rewind/-4 + _BINARY_OP_EXTEND;

        macro(BINARY_OP_INPLACE_ADD_UNICODE) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_INPLACE_ADD_UNICODE;

        specializing op(_SPECIALIZE_BINARY_SLICE, (container, start, stop -- container, start, stop)) {
            // Placeholder until we implement BINARY_SLICE specialization
            #if ENABLE_SPECIALIZATION
            OPCODE_DEFERRED_INC(BINARY_SLICE);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_BINARY_SLICE, (container, start, stop -- res)) {
            PyObject *slice = _PyBuildSlice_ConsumeRefs(PyStackRef_AsPyObjectSteal(start),
                                                        PyStackRef_AsPyObjectSteal(stop));
            PyObject *res_o;
            // Can't use ERROR_IF() here, because we haven't
            // DECREF'ed container yet, and we still own slice.
            if (slice == NULL) {
                res_o = NULL;
            }
            else {
                res_o = PyObject_GetItem(PyStackRef_AsPyObjectBorrow(container), slice);
                Py_DECREF(slice);
            }
            PyStackRef_CLOSE(container);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_SLICE) = _SPECIALIZE_BINARY_SLICE + _BINARY_SLICE;

        specializing op(_SPECIALIZE_STORE_SLICE, (v, container, start, stop -- v, container, start, stop)) {
            // Placeholder until we implement STORE_SLICE specialization
            #if ENABLE_SPECIALIZATION
            OPCODE_DEFERRED_INC(STORE_SLICE);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_STORE_SLICE, (v, container, start, stop -- )) {
            PyObject *slice = _PyBuildSlice_ConsumeRefs(PyStackRef_AsPyObjectSteal(start),
                                                        PyStackRef_AsPyObjectSteal(stop));
            int err;
            if (slice == NULL) {
                err = 1;
            }
            else {
                err = PyObject_SetItem(PyStackRef_AsPyObjectBorrow(container), slice, PyStackRef_AsPyObjectBorrow(v));
                Py_DECREF(slice);
            }
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        macro(STORE_SLICE) = _SPECIALIZE_STORE_SLICE + _STORE_SLICE;

        macro(BINARY_OP_SUBSCR_LIST_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_LIST + unused/5 + _BINARY_OP_SUBSCR_LIST_INT;

        op(_BINARY_OP_SUBSCR_LIST_INT, (list_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *list = PyStackRef_AsPyObjectBorrow(list_st);

            assert(PyLong_CheckExact(sub));
            assert(PyList_CheckExact(list));

            // Deopt unless 0 <= sub < PyList_Size(list)
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
#ifdef Py_GIL_DISABLED
            PyObject *res_o = _PyList_GetItemRef((PyListObject*)list, index);
            DEOPT_IF(res_o == NULL);
            STAT_INC(BINARY_OP, hit);
            res = PyStackRef_FromPyObjectSteal(res_o);
#else
            DEOPT_IF(index >= PyList_GET_SIZE(list));
            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = PyList_GET_ITEM(list, index);
            assert(res_o != NULL);
            res = PyStackRef_FromPyObjectNew(res_o);
#endif
            STAT_INC(BINARY_OP, hit);
            DECREF_INPUTS();
        }

        macro(BINARY_OP_SUBSCR_LIST_SLICE) =
            _GUARD_TOS_SLICE + _GUARD_NOS_LIST + unused/5 + _BINARY_OP_SUBSCR_LIST_SLICE;

        op(_BINARY_OP_SUBSCR_LIST_SLICE, (list_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *list = PyStackRef_AsPyObjectBorrow(list_st);

            assert(PySlice_Check(sub));
            assert(PyList_CheckExact(list));

            PyObject *res_o = _PyList_SliceSubscript(list, sub);
            STAT_INC(BINARY_OP, hit);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_OP_SUBSCR_STR_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_UNICODE + unused/5 + _BINARY_OP_SUBSCR_STR_INT;

        op(_BINARY_OP_SUBSCR_STR_INT, (str_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *str = PyStackRef_AsPyObjectBorrow(str_st);

            assert(PyLong_CheckExact(sub));
            assert(PyUnicode_CheckExact(str));
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(PyUnicode_GET_LENGTH(str) <= index);
            // Specialize for reading an ASCII character from any string:
            Py_UCS4 c = PyUnicode_READ_CHAR(str, index);
            DEOPT_IF(Py_ARRAY_LENGTH(_Py_SINGLETON(strings).ascii) <= c);
            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = (PyObject*)&_Py_SINGLETON(strings).ascii[c];
            PyStackRef_CLOSE_SPECIALIZED(sub_st, _PyLong_ExactDealloc);
            DEAD(sub_st);
            PyStackRef_CLOSE(str_st);
            res = PyStackRef_FromPyObjectBorrow(res_o);
        }

        op(_GUARD_NOS_TUPLE, (nos, unused -- nos, unused)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(nos);
            EXIT_IF(!PyTuple_CheckExact(o));
        }

        op(_GUARD_TOS_TUPLE, (tos -- tos)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(tos);
            EXIT_IF(!PyTuple_CheckExact(o));
        }

        macro(BINARY_OP_SUBSCR_TUPLE_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_TUPLE + unused/5 + _BINARY_OP_SUBSCR_TUPLE_INT;

        op(_BINARY_OP_SUBSCR_TUPLE_INT, (tuple_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *tuple = PyStackRef_AsPyObjectBorrow(tuple_st);

            assert(PyLong_CheckExact(sub));
            assert(PyTuple_CheckExact(tuple));

            // Deopt unless 0 <= sub < PyTuple_Size(list)
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(index >= PyTuple_GET_SIZE(tuple));
            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = PyTuple_GET_ITEM(tuple, index);
            assert(res_o != NULL);
            PyStackRef_CLOSE_SPECIALIZED(sub_st, _PyLong_ExactDealloc);
            res = PyStackRef_FromPyObjectNew(res_o);
            DECREF_INPUTS();
        }

        op(_GUARD_NOS_DICT, (nos, unused -- nos, unused)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(nos);
            EXIT_IF(!PyDict_CheckExact(o));
        }

        op(_GUARD_TOS_DICT, (tos -- tos)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(tos);
            EXIT_IF(!PyDict_CheckExact(o));
        }

        macro(BINARY_OP_SUBSCR_DICT) =
            _GUARD_NOS_DICT + unused/5 + _BINARY_OP_SUBSCR_DICT;

        op(_BINARY_OP_SUBSCR_DICT, (dict_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *dict = PyStackRef_AsPyObjectBorrow(dict_st);

            assert(PyDict_CheckExact(dict));
            STAT_INC(BINARY_OP, hit);
            PyObject *res_o;
            int rc = PyDict_GetItemRef(dict, sub, &res_o);
            if (rc == 0) {
                _PyErr_SetKeyError(sub);
            }
            DECREF_INPUTS();
            ERROR_IF(rc <= 0); // not found or error
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_BINARY_OP_SUBSCR_CHECK_FUNC, (container, unused -- container, unused, getitem)) {
            PyTypeObject *tp = Py_TYPE(PyStackRef_AsPyObjectBorrow(container));
            DEOPT_IF(!PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE));
            PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
            PyObject *getitem_o = FT_ATOMIC_LOAD_PTR_ACQUIRE(ht->_spec_cache.getitem);
            DEOPT_IF(getitem_o == NULL);
            assert(PyFunction_Check(getitem_o));
            uint32_t cached_version = FT_ATOMIC_LOAD_UINT32_RELAXED(ht->_spec_cache.getitem_version);
            DEOPT_IF(((PyFunctionObject *)getitem_o)->func_version != cached_version);
            PyCodeObject *code = (PyCodeObject *)PyFunction_GET_CODE(getitem_o);
            assert(code->co_argcount == 2);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            getitem = PyStackRef_FromPyObjectNew(getitem_o);
            STAT_INC(BINARY_OP, hit);
        }

        op(_BINARY_OP_SUBSCR_INIT_CALL, (container, sub, getitem -- new_frame)) {
            _PyInterpreterFrame* pushed_frame = _PyFrame_PushUnchecked(tstate, getitem, 2, frame);
            pushed_frame->localsplus[0] = container;
            pushed_frame->localsplus[1] = sub;
            INPUTS_DEAD();
            frame->return_offset = INSTRUCTION_SIZE;
            new_frame = PyStackRef_Wrap(pushed_frame);
        }

        macro(BINARY_OP_SUBSCR_GETITEM) =
            unused/5 + // Skip over the counter and cache
            _CHECK_PEP_523 +
            _BINARY_OP_SUBSCR_CHECK_FUNC +
            _BINARY_OP_SUBSCR_INIT_CALL +
            _PUSH_FRAME;

        inst(LIST_APPEND, (list, unused[oparg-1], v -- list, unused[oparg-1])) {
            int err = _PyList_AppendTakeRef((PyListObject *)PyStackRef_AsPyObjectBorrow(list),
                                           PyStackRef_AsPyObjectSteal(v));
            ERROR_IF(err < 0);
        }

        inst(SET_ADD, (set, unused[oparg-1], v -- set, unused[oparg-1])) {
            int err = _PySet_AddTakeRef((PySetObject *)PyStackRef_AsPyObjectBorrow(set),
                                        PyStackRef_AsPyObjectSteal(v));
            ERROR_IF(err);
        }

        family(STORE_SUBSCR, INLINE_CACHE_ENTRIES_STORE_SUBSCR) = {
            STORE_SUBSCR_DICT,
            STORE_SUBSCR_LIST_INT,
        };

        specializing op(_SPECIALIZE_STORE_SUBSCR, (counter/1, container, sub -- container, sub)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_StoreSubscr(container, sub, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(STORE_SUBSCR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        op(_STORE_SUBSCR, (v, container, sub -- )) {
            /* container[sub] = v */
            int err = PyObject_SetItem(PyStackRef_AsPyObjectBorrow(container), PyStackRef_AsPyObjectBorrow(sub), PyStackRef_AsPyObjectBorrow(v));
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        macro(STORE_SUBSCR) = _SPECIALIZE_STORE_SUBSCR + _STORE_SUBSCR;

        macro(STORE_SUBSCR_LIST_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_LIST + unused/1 + _STORE_SUBSCR_LIST_INT;

        op(_STORE_SUBSCR_LIST_INT, (value, list_st, sub_st -- )) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *list = PyStackRef_AsPyObjectBorrow(list_st);

            assert(PyLong_CheckExact(sub));
            assert(PyList_CheckExact(list));

            // Ensure nonnegative, zero-or-one-digit ints.
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(!LOCK_OBJECT(list));
            // Ensure index < len(list)
            if (index >= PyList_GET_SIZE(list)) {
                UNLOCK_OBJECT(list);
                DEOPT_IF(true);
            }
            STAT_INC(STORE_SUBSCR, hit);

            PyObject *old_value = PyList_GET_ITEM(list, index);
            FT_ATOMIC_STORE_PTR_RELEASE(_PyList_ITEMS(list)[index],
                                        PyStackRef_AsPyObjectSteal(value));
            assert(old_value != NULL);
            UNLOCK_OBJECT(list);  // unlock before decrefs!
            PyStackRef_CLOSE_SPECIALIZED(sub_st, _PyLong_ExactDealloc);
            DEAD(sub_st);
            PyStackRef_CLOSE(list_st);
            Py_DECREF(old_value);
        }

        macro(STORE_SUBSCR_DICT) =
            _GUARD_NOS_DICT + unused/1 + _STORE_SUBSCR_DICT;

        op(_STORE_SUBSCR_DICT, (value, dict_st, sub -- )) {
            PyObject *dict = PyStackRef_AsPyObjectBorrow(dict_st);

            assert(PyDict_CheckExact(dict));
            STAT_INC(STORE_SUBSCR, hit);
            int err = _PyDict_SetItem_Take2((PyDictObject *)dict,
                                            PyStackRef_AsPyObjectSteal(sub),
                                            PyStackRef_AsPyObjectSteal(value));
            PyStackRef_CLOSE(dict_st);
            ERROR_IF(err);
        }

        inst(DELETE_SUBSCR, (container, sub --)) {
            /* del container[sub] */
            int err = PyObject_DelItem(PyStackRef_AsPyObjectBorrow(container),
                                       PyStackRef_AsPyObjectBorrow(sub));
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        inst(CALL_INTRINSIC_1, (value -- res)) {
            assert(oparg <= MAX_INTRINSIC_1);
            PyObject *res_o = _PyIntrinsics_UnaryFunctions[oparg].func(tstate, PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(CALL_INTRINSIC_2, (value2_st, value1_st -- res)) {
            assert(oparg <= MAX_INTRINSIC_2);
            PyObject *value1 = PyStackRef_AsPyObjectBorrow(value1_st);
            PyObject *value2 = PyStackRef_AsPyObjectBorrow(value2_st);

            PyObject *res_o = _PyIntrinsics_BinaryFunctions[oparg].func(tstate, value2, value1);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        tier1 inst(RAISE_VARARGS, (args[oparg] -- )) {
            assert(oparg < 3);
            PyObject *cause = oparg == 2 ? PyStackRef_AsPyObjectSteal(args[1]) : NULL;
            PyObject *exc = oparg > 0 ? PyStackRef_AsPyObjectSteal(args[0]) : NULL;
            int err = do_raise(tstate, exc, cause);
            if (err) {
                assert(oparg == 0);
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
            ERROR_IF(true);
        }

        tier1 inst(INTERPRETER_EXIT, (retval --)) {
            assert(frame->owner == FRAME_OWNED_BY_INTERPRETER);
            assert(_PyFrame_IsIncomplete(frame));
            /* Restore previous frame and return. */
            tstate->current_frame = frame->previous;
            assert(!_PyErr_Occurred(tstate));
            PyObject *result = PyStackRef_AsPyObjectSteal(retval);
#if !Py_TAIL_CALL_INTERP
            assert(frame == &entry.frame);
#endif
#ifdef _Py_TIER2
            _PyStackRef executor = frame->localsplus[0];
            assert(tstate->current_executor == NULL);
            if (!PyStackRef_IsNull(executor)) {
                tstate->current_executor = PyStackRef_AsPyObjectBorrow(executor);
                PyStackRef_CLOSE(executor);
            }
#endif
            LLTRACE_RESUME_FRAME();
            return result;
        }

        // The stack effect here is a bit misleading.
        // retval is popped from the stack, but res
        // is pushed to a different frame, the callers' frame.
        inst(RETURN_VALUE, (retval -- res)) {
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            _PyStackRef temp = PyStackRef_MakeHeapSafe(retval);
            DEAD(retval);
            SAVE_STACK();
            assert(STACK_LEVEL() == 0);
            _Py_LeaveRecursiveCallPy(tstate);
            // GH-99729: We need to unlink the frame *before* clearing it:
            _PyInterpreterFrame *dying = frame;
            frame = tstate->current_frame = dying->previous;
            _PyEval_FrameClearAndPop(tstate, dying);
            RELOAD_STACK();
            LOAD_IP(frame->return_offset);
            res = temp;
            LLTRACE_RESUME_FRAME();
        }

        tier1 op(_RETURN_VALUE_EVENT, (val -- val)) {
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_RETURN,
                    frame, this_instr, PyStackRef_AsPyObjectBorrow(val));
            ERROR_IF(err);
        }

        macro(INSTRUMENTED_RETURN_VALUE) =
            _RETURN_VALUE_EVENT +
            RETURN_VALUE;

        inst(GET_AITER, (obj -- iter)) {
            unaryfunc getter = NULL;
            PyObject *obj_o = PyStackRef_AsPyObjectBorrow(obj);
            PyObject *iter_o;
            PyTypeObject *type = Py_TYPE(obj_o);

            if (type->tp_as_async != NULL) {
                getter = type->tp_as_async->am_aiter;
            }

            if (getter == NULL) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' requires an object with "
                              "__aiter__ method, got %.100s",
                              type->tp_name);
                PyStackRef_CLOSE(obj);
                ERROR_IF(true);
            }

            iter_o = (*getter)(obj_o);
            PyStackRef_CLOSE(obj);
            ERROR_IF(iter_o == NULL);

            if (Py_TYPE(iter_o)->tp_as_async == NULL ||
                    Py_TYPE(iter_o)->tp_as_async->am_anext == NULL) {

                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' received an object from __aiter__ "
                              "that does not implement __anext__: %.100s",
                              Py_TYPE(iter_o)->tp_name);
                Py_DECREF(iter_o);
                ERROR_IF(true);
            }
            iter = PyStackRef_FromPyObjectSteal(iter_o);
        }

        inst(GET_ANEXT, (aiter -- aiter, awaitable)) {
            PyObject *awaitable_o = _PyEval_GetANext(PyStackRef_AsPyObjectBorrow(aiter));
            if (awaitable_o == NULL) {
                ERROR_NO_POP();
            }
            awaitable = PyStackRef_FromPyObjectSteal(awaitable_o);
        }

        inst(GET_AWAITABLE, (iterable -- iter)) {
            PyObject *iter_o = _PyEval_GetAwaitable(PyStackRef_AsPyObjectBorrow(iterable), oparg);
            PyStackRef_CLOSE(iterable);
            ERROR_IF(iter_o == NULL);
            iter = PyStackRef_FromPyObjectSteal(iter_o);
        }

        family(SEND, INLINE_CACHE_ENTRIES_SEND) = {
            SEND_GEN,
        };

        specializing op(_SPECIALIZE_SEND, (counter/1, receiver, unused -- receiver, unused)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_Send(receiver, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(SEND);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        op(_SEND, (receiver, v -- receiver, retval)) {
            PyObject *receiver_o = PyStackRef_AsPyObjectBorrow(receiver);
            PyObject *retval_o;
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            if ((tstate->interp->eval_frame == NULL) &&
                (Py_TYPE(receiver_o) == &PyGen_Type || Py_TYPE(receiver_o) == &PyCoro_Type) &&
                ((PyGenObject *)receiver_o)->gi_frame_state < FRAME_EXECUTING)
            {
                PyGenObject *gen = (PyGenObject *)receiver_o;
                _PyInterpreterFrame *gen_frame = &gen->gi_iframe;
                _PyFrame_StackPush(gen_frame, PyStackRef_MakeHeapSafe(v));
                DEAD(v);
                SYNC_SP();
                gen->gi_frame_state = FRAME_EXECUTING;
                gen->gi_exc_state.previous_item = tstate->exc_info;
                tstate->exc_info = &gen->gi_exc_state;
                assert(INSTRUCTION_SIZE + oparg <= UINT16_MAX);
                frame->return_offset = (uint16_t)(INSTRUCTION_SIZE + oparg);
                assert(gen_frame->previous == NULL);
                gen_frame->previous = frame;
                DISPATCH_INLINED(gen_frame);
            }
            if (PyStackRef_IsNone(v) && PyIter_Check(receiver_o)) {
                retval_o = Py_TYPE(receiver_o)->tp_iternext(receiver_o);
            }
            else {
                retval_o = PyObject_CallMethodOneArg(receiver_o,
                                                     &_Py_ID(send),
                                                     PyStackRef_AsPyObjectBorrow(v));
            }
            if (retval_o == NULL) {
                int matches = _PyErr_ExceptionMatches(tstate, PyExc_StopIteration);
                if (matches) {
                    _PyEval_MonitorRaise(tstate, frame, this_instr);
                }
                int err = _PyGen_FetchStopIterationValue(&retval_o);
                if (err == 0) {
                    assert(retval_o != NULL);
                    JUMPBY(oparg);
                }
                else {
                    PyStackRef_CLOSE(v);
                    ERROR_IF(true);
                }
            }
            PyStackRef_CLOSE(v);
            retval = PyStackRef_FromPyObjectSteal(retval_o);
        }

        macro(SEND) = _SPECIALIZE_SEND + _SEND;

        op(_SEND_GEN_FRAME, (receiver, v -- receiver, gen_frame)) {
            PyGenObject *gen = (PyGenObject *)PyStackRef_AsPyObjectBorrow(receiver);
            DEOPT_IF(Py_TYPE(gen) != &PyGen_Type && Py_TYPE(gen) != &PyCoro_Type);
            DEOPT_IF(gen->gi_frame_state >= FRAME_EXECUTING);
            STAT_INC(SEND, hit);
            _PyInterpreterFrame *pushed_frame = &gen->gi_iframe;
            _PyFrame_StackPush(pushed_frame, PyStackRef_MakeHeapSafe(v));
            DEAD(v);
            gen->gi_frame_state = FRAME_EXECUTING;
            gen->gi_exc_state.previous_item = tstate->exc_info;
            tstate->exc_info = &gen->gi_exc_state;
            assert(INSTRUCTION_SIZE + oparg <= UINT16_MAX);
            frame->return_offset = (uint16_t)(INSTRUCTION_SIZE + oparg);
            pushed_frame->previous = frame;
            gen_frame = PyStackRef_Wrap(pushed_frame);
        }

        macro(SEND_GEN) =
            unused/1 +
            _CHECK_PEP_523 +
            _SEND_GEN_FRAME +
            _PUSH_FRAME;

        inst(YIELD_VALUE, (retval -- value)) {
            // NOTE: It's important that YIELD_VALUE never raises an exception!
            // The compiler treats any exception raised here as a failed close()
            // or throw() call.
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            frame->instr_ptr++;
            PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
            assert(FRAME_SUSPENDED_YIELD_FROM == FRAME_SUSPENDED + 1);
            assert(oparg == 0 || oparg == 1);
            gen->gi_frame_state = FRAME_SUSPENDED + oparg;
            _PyStackRef temp = retval;
            DEAD(retval);
            SAVE_STACK();
            tstate->exc_info = gen->gi_exc_state.previous_item;
            gen->gi_exc_state.previous_item = NULL;
            _Py_LeaveRecursiveCallPy(tstate);
            _PyInterpreterFrame *gen_frame = frame;
            frame = tstate->current_frame = frame->previous;
            gen_frame->previous = NULL;
            /* We don't know which of these is relevant here, so keep them equal */
            assert(INLINE_CACHE_ENTRIES_SEND == INLINE_CACHE_ENTRIES_FOR_ITER);
            #if TIER_ONE
            assert(frame->instr_ptr->op.code == INSTRUMENTED_LINE ||
                   frame->instr_ptr->op.code == INSTRUMENTED_INSTRUCTION ||
                   _PyOpcode_Deopt[frame->instr_ptr->op.code] == SEND ||
                   _PyOpcode_Deopt[frame->instr_ptr->op.code] == FOR_ITER ||
                   _PyOpcode_Deopt[frame->instr_ptr->op.code] == INTERPRETER_EXIT ||
                   _PyOpcode_Deopt[frame->instr_ptr->op.code] == ENTER_EXECUTOR);
            #endif
            RELOAD_STACK();
            LOAD_IP(1 + INLINE_CACHE_ENTRIES_SEND);
            value = PyStackRef_MakeHeapSafe(temp);
            LLTRACE_RESUME_FRAME();
        }

        tier1 op(_YIELD_VALUE_EVENT, (val -- val)) {
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_YIELD,
                    frame, this_instr, PyStackRef_AsPyObjectBorrow(val));
            if (err) {
                ERROR_NO_POP();
            }
            if (frame->instr_ptr != this_instr) {
                next_instr = frame->instr_ptr;
                DISPATCH();
            }
        }

        macro(INSTRUMENTED_YIELD_VALUE) =
            _YIELD_VALUE_EVENT +
            YIELD_VALUE;

        inst(POP_EXCEPT, (exc_value -- )) {
            _PyErr_StackItem *exc_info = tstate->exc_info;
            Py_XSETREF(exc_info->exc_value,
                   PyStackRef_IsNone(exc_value)
                    ? NULL : PyStackRef_AsPyObjectSteal(exc_value));
        }

        tier1 inst(RERAISE, (values[oparg], exc_st -- values[oparg])) {
            PyObject *exc = PyStackRef_AsPyObjectSteal(exc_st);

            assert(oparg >= 0 && oparg <= 2);
            if (oparg) {
                frame->instr_ptr = _PyFrame_GetBytecode(frame) + PyStackRef_UntagInt(values[0]);
            }
            assert(exc && PyExceptionInstance_Check(exc));
            _PyErr_SetRaisedException(tstate, exc);
            monitor_reraise(tstate, frame, this_instr);
            goto exception_unwind;
        }

        tier1 op(_END_ASYNC_FOR, (awaitable_st, exc_st -- )) {
            JUMPBY(0); // Pretend jump as we need source offset for monitoring
            (void)oparg;
            PyObject *exc = PyStackRef_AsPyObjectBorrow(exc_st);

            assert(exc && PyExceptionInstance_Check(exc));
            int matches = PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration);
            if (matches) {
                DECREF_INPUTS();
            }
            else {
                Py_INCREF(exc);
                _PyErr_SetRaisedException(tstate, exc);
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
        }

        tier1 op(_MONITOR_END_ASYNC_FOR, ( -- )) {
            assert((next_instr-oparg)->op.code == END_SEND || (next_instr-oparg)->op.code >= MIN_INSTRUMENTED_OPCODE);
            INSTRUMENTED_JUMP(next_instr-oparg, this_instr+1, PY_MONITORING_EVENT_BRANCH_RIGHT);
        }

        macro(INSTRUMENTED_END_ASYNC_FOR) =
            _MONITOR_END_ASYNC_FOR +
            _END_ASYNC_FOR;

        macro(END_ASYNC_FOR) = _END_ASYNC_FOR;

        tier1 inst(CLEANUP_THROW, (sub_iter, last_sent_val, exc_value_st -- none, value)) {
            PyObject *exc_value = PyStackRef_AsPyObjectBorrow(exc_value_st);
            #if !Py_TAIL_CALL_INTERP
            assert(throwflag);
            #endif
            assert(exc_value && PyExceptionInstance_Check(exc_value));

            int matches = PyErr_GivenExceptionMatches(exc_value, PyExc_StopIteration);
            if (matches) {
                value = PyStackRef_FromPyObjectNew(((PyStopIterationObject *)exc_value)->value);
                DECREF_INPUTS();
                none = PyStackRef_None;
            }
            else {
                _PyErr_SetRaisedException(tstate, Py_NewRef(exc_value));
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
        }

        inst(LOAD_COMMON_CONSTANT, ( -- value)) {
            // Keep in sync with _common_constants in opcode.py
            assert(oparg < NUM_COMMON_CONSTANTS);
            value = PyStackRef_FromPyObjectNew(tstate->interp->common_consts[oparg]);
        }

        inst(LOAD_BUILD_CLASS, ( -- bc)) {
            PyObject *bc_o;
            int err = PyMapping_GetOptionalItem(BUILTINS(), &_Py_ID(__build_class__), &bc_o);
            ERROR_IF(err < 0);
            if (bc_o == NULL) {
                _PyErr_SetString(tstate, PyExc_NameError,
                                 "__build_class__ not found");
                ERROR_IF(true);
            }
            bc = PyStackRef_FromPyObjectSteal(bc_o);
        }

        inst(STORE_NAME, (v -- )) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when storing %R", name);
                PyStackRef_CLOSE(v);
                ERROR_IF(true);
            }
            if (PyDict_CheckExact(ns)) {
                err = PyDict_SetItem(ns, name, PyStackRef_AsPyObjectBorrow(v));
            }
            else {
                err = PyObject_SetItem(ns, name, PyStackRef_AsPyObjectBorrow(v));
            }
            PyStackRef_CLOSE(v);
            ERROR_IF(err);
        }

        inst(DELETE_NAME, (--)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when deleting %R", name);
                ERROR_NO_POP();
            }
            err = PyObject_DelItem(ns, name);
            // Can't use ERROR_IF here.
            if (err != 0) {
                _PyEval_FormatExcCheckArg(tstate, PyExc_NameError,
                                          NAME_ERROR_MSG,
                                          name);
                ERROR_NO_POP();
            }
        }

        family(UNPACK_SEQUENCE, INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE) = {
            UNPACK_SEQUENCE_TWO_TUPLE,
            UNPACK_SEQUENCE_TUPLE,
            UNPACK_SEQUENCE_LIST,
        };

        specializing op(_SPECIALIZE_UNPACK_SEQUENCE, (counter/1, seq -- seq)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_UnpackSequence(seq, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(UNPACK_SEQUENCE);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
            (void)seq;
            (void)counter;
        }

        op(_UNPACK_SEQUENCE, (seq -- unused[oparg], top[0])) {
            PyObject *seq_o = PyStackRef_AsPyObjectSteal(seq);
            int res = _PyEval_UnpackIterableStackRef(tstate, seq_o, oparg, -1, top);
            Py_DECREF(seq_o);
            ERROR_IF(res == 0);
        }

        macro(UNPACK_SEQUENCE) = _SPECIALIZE_UNPACK_SEQUENCE + _UNPACK_SEQUENCE;

        macro(UNPACK_SEQUENCE_TWO_TUPLE) =
            _GUARD_TOS_TUPLE + unused/1 + _UNPACK_SEQUENCE_TWO_TUPLE;

        op(_UNPACK_SEQUENCE_TWO_TUPLE, (seq -- val1, val0)) {
            assert(oparg == 2);
            PyObject *seq_o = PyStackRef_AsPyObjectBorrow(seq);
            assert(PyTuple_CheckExact(seq_o));
            DEOPT_IF(PyTuple_GET_SIZE(seq_o) != 2);
            STAT_INC(UNPACK_SEQUENCE, hit);
            val0 = PyStackRef_FromPyObjectNew(PyTuple_GET_ITEM(seq_o, 0));
            val1 = PyStackRef_FromPyObjectNew(PyTuple_GET_ITEM(seq_o, 1));
            PyStackRef_CLOSE(seq);
        }

        macro(UNPACK_SEQUENCE_TUPLE) =
            _GUARD_TOS_TUPLE + unused/1 + _UNPACK_SEQUENCE_TUPLE;

        op(_UNPACK_SEQUENCE_TUPLE, (seq -- values[oparg])) {
            PyObject *seq_o = PyStackRef_AsPyObjectBorrow(seq);
            assert(PyTuple_CheckExact(seq_o));
            DEOPT_IF(PyTuple_GET_SIZE(seq_o) != oparg);
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyObject **items = _PyTuple_ITEMS(seq_o);
            for (int i = oparg; --i >= 0; ) {
                *values++ = PyStackRef_FromPyObjectNew(items[i]);
            }
            DECREF_INPUTS();
        }

        macro(UNPACK_SEQUENCE_LIST) =
            _GUARD_TOS_LIST + unused/1 + _UNPACK_SEQUENCE_LIST;

        op(_UNPACK_SEQUENCE_LIST, (seq -- values[oparg])) {
            PyObject *seq_o = PyStackRef_AsPyObjectBorrow(seq);
            assert(PyList_CheckExact(seq_o));
            DEOPT_IF(!LOCK_OBJECT(seq_o));
            if (PyList_GET_SIZE(seq_o) != oparg) {
                UNLOCK_OBJECT(seq_o);
                DEOPT_IF(true);
            }
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyObject **items = _PyList_ITEMS(seq_o);
            for (int i = oparg; --i >= 0; ) {
                *values++ = PyStackRef_FromPyObjectNew(items[i]);
            }
            UNLOCK_OBJECT(seq_o);
            DECREF_INPUTS();
        }

        inst(UNPACK_EX, (seq -- unused[oparg & 0xFF], unused, unused[oparg >> 8], top[0])) {
            PyObject *seq_o = PyStackRef_AsPyObjectSteal(seq);
            int res = _PyEval_UnpackIterableStackRef(tstate, seq_o, oparg & 0xFF, oparg >> 8, top);
            Py_DECREF(seq_o);
            ERROR_IF(res == 0);
        }

        family(STORE_ATTR, INLINE_CACHE_ENTRIES_STORE_ATTR) = {
            STORE_ATTR_INSTANCE_VALUE,
            STORE_ATTR_SLOT,
            STORE_ATTR_WITH_HINT,
        };

        specializing op(_SPECIALIZE_STORE_ATTR, (counter/1, owner -- owner)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
                next_instr = this_instr;
                _Py_Specialize_StoreAttr(owner, next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(STORE_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        op(_STORE_ATTR, (v, owner --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = PyObject_SetAttr(PyStackRef_AsPyObjectBorrow(owner),
                                       name, PyStackRef_AsPyObjectBorrow(v));
            DECREF_INPUTS();
            ERROR_IF(err);
        }

        macro(STORE_ATTR) = _SPECIALIZE_STORE_ATTR + unused/3 + _STORE_ATTR;

        inst(DELETE_ATTR, (owner --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = PyObject_DelAttr(PyStackRef_AsPyObjectBorrow(owner), name);
            PyStackRef_CLOSE(owner);
            ERROR_IF(err);
        }

        inst(STORE_GLOBAL, (v --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = PyDict_SetItem(GLOBALS(), name, PyStackRef_AsPyObjectBorrow(v));
            PyStackRef_CLOSE(v);
            ERROR_IF(err);
        }

        inst(DELETE_GLOBAL, (--)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = PyDict_Pop(GLOBALS(), name, NULL);
            // Can't use ERROR_IF here.
            if (err < 0) {
                ERROR_NO_POP();
            }
            if (err == 0) {
                _PyEval_FormatExcCheckArg(tstate, PyExc_NameError,
                                          NAME_ERROR_MSG, name);
                ERROR_NO_POP();
            }
        }

        inst(LOAD_LOCALS, ( -- locals)) {
            PyObject *l = LOCALS();
            if (l == NULL) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "no locals found");
                ERROR_IF(true);
            }
            locals = PyStackRef_FromPyObjectNew(l);
        }

        inst(LOAD_FROM_DICT_OR_GLOBALS, (mod_or_class_dict -- v)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *v_o;
            int err = PyMapping_GetOptionalItem(PyStackRef_AsPyObjectBorrow(mod_or_class_dict), name, &v_o);
            PyStackRef_CLOSE(mod_or_class_dict);
            ERROR_IF(err < 0);
            if (v_o == NULL) {
                if (PyDict_CheckExact(GLOBALS())
                    && PyDict_CheckExact(BUILTINS()))
                {
                    v_o = _PyDict_LoadGlobal((PyDictObject *)GLOBALS(),
                                             (PyDictObject *)BUILTINS(),
                                             name);
                    if (v_o == NULL) {
                        if (!_PyErr_Occurred(tstate)) {
                            /* _PyDict_LoadGlobal() returns NULL without raising
                            * an exception if the key doesn't exist */
                            _PyEval_FormatExcCheckArg(tstate, PyExc_NameError,
                                                    NAME_ERROR_MSG, name);
                        }
                        ERROR_NO_POP();
                    }
                }
                else {
                    /* Slow-path if globals or builtins is not a dict */
                    /* namespace 1: globals */
                    int err = PyMapping_GetOptionalItem(GLOBALS(), name, &v_o);
                    ERROR_IF(err < 0);
                    if (v_o == NULL) {
                        /* namespace 2: builtins */
                        int err = PyMapping_GetOptionalItem(BUILTINS(), name, &v_o);
                        ERROR_IF(err < 0);
                        if (v_o == NULL) {
                            _PyEval_FormatExcCheckArg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                            ERROR_IF(true);
                        }
                    }
                }
            }
            v = PyStackRef_FromPyObjectSteal(v_o);
        }

        inst(LOAD_NAME, (-- v)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *v_o = _PyEval_LoadName(tstate, frame, name);
            ERROR_IF(v_o == NULL);
            v = PyStackRef_FromPyObjectSteal(v_o);
        }

        family(LOAD_GLOBAL, INLINE_CACHE_ENTRIES_LOAD_GLOBAL) = {
            LOAD_GLOBAL_MODULE,
            LOAD_GLOBAL_BUILTIN,
        };

        specializing op(_SPECIALIZE_LOAD_GLOBAL, (counter/1 -- )) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
                next_instr = this_instr;
                _Py_Specialize_LoadGlobal(GLOBALS(), BUILTINS(), next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_GLOBAL);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        // res[1] because we need a pointer to res to pass it to _PyEval_LoadGlobalStackRef
        op(_LOAD_GLOBAL, ( -- res[1])) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
            _PyEval_LoadGlobalStackRef(GLOBALS(), BUILTINS(), name, res);
            ERROR_IF(PyStackRef_IsNull(*res));
        }

        op(_PUSH_NULL_CONDITIONAL, ( -- null[oparg & 1])) {
            if (oparg & 1) {
                null[0] = PyStackRef_NULL;
            }
        }

        macro(LOAD_GLOBAL) =
            _SPECIALIZE_LOAD_GLOBAL +
            counter/1 +
            globals_version/1 +
            builtins_version/1 +
            _LOAD_GLOBAL +
            _PUSH_NULL_CONDITIONAL;

        op(_GUARD_GLOBALS_VERSION, (version/1 --)) {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != version);
            assert(DK_IS_UNICODE(keys));
        }

        op(_LOAD_GLOBAL_MODULE, (version/1, unused/1, index/1 -- res))
        {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != version);
            assert(DK_IS_UNICODE(keys));
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            assert(index < DK_SIZE(keys));
            PyObject *res_o = FT_ATOMIC_LOAD_PTR_RELAXED(entries[index].me_value);
            DEOPT_IF(res_o == NULL);
            #if Py_GIL_DISABLED
            int increfed = _Py_TryIncrefCompareStackRef(&entries[index].me_value, res_o, &res);
            DEOPT_IF(!increfed);
            #else
            res = PyStackRef_FromPyObjectNew(res_o);
            #endif
            STAT_INC(LOAD_GLOBAL, hit);
        }

        op(_LOAD_GLOBAL_BUILTINS, (version/1, index/1 -- res))
        {
            PyDictObject *dict = (PyDictObject *)BUILTINS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != version);
            assert(DK_IS_UNICODE(keys));
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            PyObject *res_o = FT_ATOMIC_LOAD_PTR_RELAXED(entries[index].me_value);
            DEOPT_IF(res_o == NULL);
            #if Py_GIL_DISABLED
            int increfed = _Py_TryIncrefCompareStackRef(&entries[index].me_value, res_o, &res);
            DEOPT_IF(!increfed);
            #else
            res = PyStackRef_FromPyObjectNew(res_o);
            #endif
            STAT_INC(LOAD_GLOBAL, hit);
        }

        macro(LOAD_GLOBAL_MODULE) =
            unused/1 + // Skip over the counter
            NOP + // For guard insertion in the JIT optimizer
            _LOAD_GLOBAL_MODULE +
            _PUSH_NULL_CONDITIONAL;

        macro(LOAD_GLOBAL_BUILTIN) =
            unused/1 + // Skip over the counter
            _GUARD_GLOBALS_VERSION +
            _LOAD_GLOBAL_BUILTINS +
            _PUSH_NULL_CONDITIONAL;

        inst(DELETE_FAST, (--)) {
            _PyStackRef v = GETLOCAL(oparg);
            if (PyStackRef_IsNull(v)) {
                _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
                    UNBOUNDLOCAL_ERROR_MSG,
                    PyTuple_GetItem(_PyFrame_GetCode(frame)->co_localsplusnames, oparg)
                );
                ERROR_IF(true);
            }
            _PyStackRef tmp = GETLOCAL(oparg);
            GETLOCAL(oparg) = PyStackRef_NULL;
            PyStackRef_XCLOSE(tmp);
        }

        inst(MAKE_CELL, (--)) {
            // "initial" is probably NULL but not if it's an arg (or set
            // via the f_locals proxy before MAKE_CELL has run).
            PyObject *initial = PyStackRef_AsPyObjectBorrow(GETLOCAL(oparg));
            PyObject *cell = PyCell_New(initial);
            if (cell == NULL) {
                ERROR_NO_POP();
            }
            _PyStackRef tmp = GETLOCAL(oparg);
            GETLOCAL(oparg) = PyStackRef_FromPyObjectSteal(cell);
            PyStackRef_XCLOSE(tmp);
        }

        inst(DELETE_DEREF, (--)) {
            PyObject *cell = PyStackRef_AsPyObjectBorrow(GETLOCAL(oparg));
            // Can't use ERROR_IF here.
            // Fortunately we don't need its superpower.
            PyObject *oldobj = PyCell_SwapTakeRef((PyCellObject *)cell, NULL);
            if (oldobj == NULL) {
                _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                ERROR_NO_POP();
            }
            Py_DECREF(oldobj);
        }

        inst(LOAD_FROM_DICT_OR_DEREF, (class_dict_st -- value)) {
            PyObject *value_o;
            PyObject *name;
            PyObject *class_dict = PyStackRef_AsPyObjectBorrow(class_dict_st);

            assert(class_dict);
            assert(oparg >= 0 && oparg < _PyFrame_GetCode(frame)->co_nlocalsplus);
            name = PyTuple_GET_ITEM(_PyFrame_GetCode(frame)->co_localsplusnames, oparg);
            int err = PyMapping_GetOptionalItem(class_dict, name, &value_o);
            if (err < 0) {
                ERROR_NO_POP();
            }
            if (!value_o) {
                PyCellObject *cell = (PyCellObject *)PyStackRef_AsPyObjectBorrow(GETLOCAL(oparg));
                value_o = PyCell_GetRef(cell);
                if (value_o == NULL) {
                    _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                    ERROR_NO_POP();
                }
            }
            PyStackRef_CLOSE(class_dict_st);
            value = PyStackRef_FromPyObjectSteal(value_o);
        }

        inst(LOAD_DEREF, ( -- value)) {
            PyCellObject *cell = (PyCellObject *)PyStackRef_AsPyObjectBorrow(GETLOCAL(oparg));
            value = _PyCell_GetStackRef(cell);
            if (PyStackRef_IsNull(value)) {
                _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                ERROR_IF(true);
            }
        }

        inst(STORE_DEREF, (v --)) {
            PyCellObject *cell = (PyCellObject *)PyStackRef_AsPyObjectBorrow(GETLOCAL(oparg));
            PyCell_SetTakeRef(cell, PyStackRef_AsPyObjectSteal(v));
        }

        inst(COPY_FREE_VARS, (--)) {
            /* Copy closure variables to free variables */
            PyCodeObject *co = _PyFrame_GetCode(frame);
            assert(PyStackRef_FunctionCheck(frame->f_funcobj));
            PyFunctionObject *func = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(frame->f_funcobj);
            PyObject *closure = func->func_closure;
            assert(oparg == co->co_nfreevars);
            int offset = co->co_nlocalsplus - oparg;
            for (int i = 0; i < oparg; ++i) {
                PyObject *o = PyTuple_GET_ITEM(closure, i);
                frame->localsplus[offset + i] = PyStackRef_FromPyObjectNew(o);
            }
        }

        inst(BUILD_STRING, (pieces[oparg] -- str)) {
            STACKREFS_TO_PYOBJECTS(pieces, oparg, pieces_o);
            if (CONVERSION_FAILED(pieces_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *str_o = _PyUnicode_JoinArray(&_Py_STR(empty), pieces_o, oparg);
            STACKREFS_TO_PYOBJECTS_CLEANUP(pieces_o);
            DECREF_INPUTS();
            ERROR_IF(str_o == NULL);
            str = PyStackRef_FromPyObjectSteal(str_o);
        }

        inst(BUILD_INTERPOLATION, (value, str, format[oparg & 1] -- interpolation)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            PyObject *str_o = PyStackRef_AsPyObjectBorrow(str);
            int conversion = oparg >> 2;
            PyObject *format_o;
            if (oparg & 1) {
                format_o = PyStackRef_AsPyObjectBorrow(format[0]);
            }
            else {
                format_o = &_Py_STR(empty);
            }
            PyObject *interpolation_o = _PyInterpolation_Build(value_o, str_o, conversion, format_o);
            if (oparg & 1) {
                PyStackRef_CLOSE(format[0]);
            }
            else {
                DEAD(format);
            }
            PyStackRef_CLOSE(str);
            PyStackRef_CLOSE(value);
            ERROR_IF(interpolation_o == NULL);
            interpolation = PyStackRef_FromPyObjectSteal(interpolation_o);
        }

        inst(BUILD_TEMPLATE, (strings, interpolations -- template)) {
            PyObject *strings_o = PyStackRef_AsPyObjectBorrow(strings);
            PyObject *interpolations_o = PyStackRef_AsPyObjectBorrow(interpolations);
            PyObject *template_o = _PyTemplate_Build(strings_o, interpolations_o);
            PyStackRef_CLOSE(interpolations);
            PyStackRef_CLOSE(strings);
            ERROR_IF(template_o == NULL);
            template = PyStackRef_FromPyObjectSteal(template_o);
        }

        inst(BUILD_TUPLE, (values[oparg] -- tup)) {
            PyObject *tup_o = _PyTuple_FromStackRefStealOnSuccess(values, oparg);
            if (tup_o == NULL) {
                ERROR_NO_POP();
            }
            INPUTS_DEAD();
            tup = PyStackRef_FromPyObjectStealMortal(tup_o);
        }

        inst(BUILD_LIST, (values[oparg] -- list)) {
            PyObject *list_o = _PyList_FromStackRefStealOnSuccess(values, oparg);
            if (list_o == NULL) {
                ERROR_NO_POP();
            }
            INPUTS_DEAD();
            list = PyStackRef_FromPyObjectStealMortal(list_o);
        }

        inst(LIST_EXTEND, (list_st, unused[oparg-1], iterable_st -- list_st, unused[oparg-1])) {
            PyObject *list = PyStackRef_AsPyObjectBorrow(list_st);
            PyObject *iterable = PyStackRef_AsPyObjectBorrow(iterable_st);

            PyObject *none_val = _PyList_Extend((PyListObject *)list, iterable);
            if (none_val == NULL) {
                int matches = _PyErr_ExceptionMatches(tstate, PyExc_TypeError);
                if (matches &&
                   (Py_TYPE(iterable)->tp_iter == NULL && !PySequence_Check(iterable)))
                {
                    _PyErr_Clear(tstate);
                    _PyErr_Format(tstate, PyExc_TypeError,
                          "Value after * must be an iterable, not %.200s",
                          Py_TYPE(iterable)->tp_name);
                }
                PyStackRef_CLOSE(iterable_st);
                ERROR_IF(true);
            }
            assert(Py_IsNone(none_val));
            PyStackRef_CLOSE(iterable_st);
        }

        inst(SET_UPDATE, (set, unused[oparg-1], iterable -- set, unused[oparg-1])) {
            int err = _PySet_Update(PyStackRef_AsPyObjectBorrow(set),
                                    PyStackRef_AsPyObjectBorrow(iterable));
            PyStackRef_CLOSE(iterable);
            ERROR_IF(err < 0);
        }

        inst(BUILD_SET, (values[oparg] -- set)) {
            PyObject *set_o = PySet_New(NULL);
            if (set_o == NULL) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }

            int err = 0;
            for (Py_ssize_t i = 0; i < oparg; i++) {
                _PyStackRef value = values[i];
                values[i] = PyStackRef_NULL;
                if (err == 0) {
                    err = _PySet_AddTakeRef((PySetObject *)set_o, PyStackRef_AsPyObjectSteal(value));
                }
                else {
                    PyStackRef_CLOSE(value);
                }
            }
            DEAD(values);
            if (err) {
                Py_DECREF(set_o);
                ERROR_IF(true);
            }

            INPUTS_DEAD();
            set = PyStackRef_FromPyObjectStealMortal(set_o);
        }

        inst(BUILD_MAP, (values[oparg*2] -- map)) {
            STACKREFS_TO_PYOBJECTS(values, oparg*2, values_o);
            if (CONVERSION_FAILED(values_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *map_o = _PyDict_FromItems(
                    values_o, 2,
                    values_o+1, 2,
                    oparg);
            STACKREFS_TO_PYOBJECTS_CLEANUP(values_o);
            DECREF_INPUTS();
            ERROR_IF(map_o == NULL);
            map = PyStackRef_FromPyObjectStealMortal(map_o);
        }

        inst(SETUP_ANNOTATIONS, (--)) {
            PyObject *ann_dict;
            if (LOCALS() == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when setting up annotations");
                ERROR_IF(true);
            }
            /* check if __annotations__ in locals()... */
            int err = PyMapping_GetOptionalItem(LOCALS(), &_Py_ID(__annotations__), &ann_dict);
            ERROR_IF(err < 0);
            if (ann_dict == NULL) {
                ann_dict = PyDict_New();
                ERROR_IF(ann_dict == NULL);
                err = PyObject_SetItem(LOCALS(), &_Py_ID(__annotations__),
                                       ann_dict);
                Py_DECREF(ann_dict);
                ERROR_IF(err);
            }
            else {
                Py_DECREF(ann_dict);
            }
        }

        pseudo(ANNOTATIONS_PLACEHOLDER, (--)) = {
            NOP,
        };

        inst(DICT_UPDATE, (dict, unused[oparg - 1], update -- dict, unused[oparg - 1])) {
            PyObject *dict_o = PyStackRef_AsPyObjectBorrow(dict);
            PyObject *update_o = PyStackRef_AsPyObjectBorrow(update);

            int err = PyDict_Update(dict_o, update_o);
            if (err < 0) {
                int matches = _PyErr_ExceptionMatches(tstate, PyExc_AttributeError);
                if (matches) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                    "'%.200s' object is not a mapping",
                                    Py_TYPE(update_o)->tp_name);
                }
                PyStackRef_CLOSE(update);
                ERROR_IF(true);
            }
            PyStackRef_CLOSE(update);
        }

        inst(DICT_MERGE, (callable, unused, unused, dict, unused[oparg - 1], update -- callable, unused, unused, dict, unused[oparg - 1])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *dict_o = PyStackRef_AsPyObjectBorrow(dict);
            PyObject *update_o = PyStackRef_AsPyObjectBorrow(update);

            int err = _PyDict_MergeEx(dict_o, update_o, 2);
            if (err < 0) {
                _PyEval_FormatKwargsError(tstate, callable_o, update_o);
                PyStackRef_CLOSE(update);
                ERROR_IF(true);
            }
            PyStackRef_CLOSE(update);
        }

        inst(MAP_ADD, (dict_st, unused[oparg - 1], key, value -- dict_st, unused[oparg - 1])) {
            PyObject *dict = PyStackRef_AsPyObjectBorrow(dict_st);
            assert(PyDict_CheckExact(dict));
            /* dict[key] = value */
            // Do not DECREF INPUTS because the function steals the references
            int err = _PyDict_SetItem_Take2(
                (PyDictObject *)dict,
                PyStackRef_AsPyObjectSteal(key),
                PyStackRef_AsPyObjectSteal(value)
            );
            ERROR_IF(err != 0);
        }

        macro(INSTRUMENTED_LOAD_SUPER_ATTR) =
            counter/1 +
            _LOAD_SUPER_ATTR +
            _PUSH_NULL_CONDITIONAL;

        family(LOAD_SUPER_ATTR, INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR) = {
            LOAD_SUPER_ATTR_ATTR,
            LOAD_SUPER_ATTR_METHOD,
        };

        specializing op(_SPECIALIZE_LOAD_SUPER_ATTR, (counter/1, global_super_st, class_st, unused -- global_super_st, class_st, unused)) {
            #if ENABLE_SPECIALIZATION_FT
            int load_method = oparg & 1;
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_LoadSuperAttr(global_super_st, class_st, next_instr, load_method);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_SUPER_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        tier1 op(_LOAD_SUPER_ATTR, (global_super_st, class_st, self_st -- attr)) {
            PyObject *global_super = PyStackRef_AsPyObjectBorrow(global_super_st);
            PyObject *class = PyStackRef_AsPyObjectBorrow(class_st);
            PyObject *self = PyStackRef_AsPyObjectBorrow(self_st);

            if (opcode == INSTRUMENTED_LOAD_SUPER_ATTR) {
                PyObject *arg = oparg & 2 ? class : &_PyInstrumentation_MISSING;
                int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_CALL,
                        frame, this_instr, global_super, arg);
                if (err) {
                    DECREF_INPUTS();
                    ERROR_IF(true);
                }
            }
            // we make no attempt to optimize here; specializations should
            // handle any case whose performance we care about
            PyObject *stack[] = {class, self};
            PyObject *super = PyObject_Vectorcall(global_super, stack, oparg & 2, NULL);
            if (opcode == INSTRUMENTED_LOAD_SUPER_ATTR) {
                PyObject *arg = oparg & 2 ? class : &_PyInstrumentation_MISSING;
                if (super == NULL) {
                    _Py_call_instrumentation_exc2(
                        tstate, PY_MONITORING_EVENT_C_RAISE,
                        frame, this_instr, global_super, arg);
                }
                else {
                    int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_C_RETURN,
                        frame, this_instr, global_super, arg);
                    if (err < 0) {
                        Py_CLEAR(super);
                    }
                }
            }
            DECREF_INPUTS();
            ERROR_IF(super == NULL);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            PyObject *attr_o = PyObject_GetAttr(super, name);
            Py_DECREF(super);
            ERROR_IF(attr_o == NULL);
            attr = PyStackRef_FromPyObjectSteal(attr_o);
        }

        macro(LOAD_SUPER_ATTR) =
            _SPECIALIZE_LOAD_SUPER_ATTR +
            _LOAD_SUPER_ATTR +
            _PUSH_NULL_CONDITIONAL;

        inst(LOAD_SUPER_ATTR_ATTR, (unused/1, global_super_st, class_st, self_st -- attr_st)) {
            PyObject *global_super = PyStackRef_AsPyObjectBorrow(global_super_st);
            PyObject *class = PyStackRef_AsPyObjectBorrow(class_st);
            PyObject *self = PyStackRef_AsPyObjectBorrow(self_st);

            assert(!(oparg & 1));
            DEOPT_IF(global_super != (PyObject *)&PySuper_Type);
            DEOPT_IF(!PyType_Check(class));
            STAT_INC(LOAD_SUPER_ATTR, hit);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            PyObject *attr = _PySuper_Lookup((PyTypeObject *)class, self, name, NULL);
            DECREF_INPUTS();
            ERROR_IF(attr == NULL);
            attr_st = PyStackRef_FromPyObjectSteal(attr);
        }

        inst(LOAD_SUPER_ATTR_METHOD, (unused/1, global_super_st, class_st, self_st -- attr, self_or_null)) {
            PyObject *global_super = PyStackRef_AsPyObjectBorrow(global_super_st);
            PyObject *class = PyStackRef_AsPyObjectBorrow(class_st);
            PyObject *self = PyStackRef_AsPyObjectBorrow(self_st);

            assert(oparg & 1);
            DEOPT_IF(global_super != (PyObject *)&PySuper_Type);
            DEOPT_IF(!PyType_Check(class));
            STAT_INC(LOAD_SUPER_ATTR, hit);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            PyTypeObject *cls = (PyTypeObject *)class;
            int method_found = 0;
            PyObject *attr_o = _PySuper_Lookup(cls, self, name,
                                   Py_TYPE(self)->tp_getattro == PyObject_GenericGetAttr ? &method_found : NULL);
            if (attr_o == NULL) {
                ERROR_NO_POP();
            }
            if (method_found) {
                self_or_null = self_st; // transfer ownership
                DEAD(self_st);
            } else {
                PyStackRef_CLOSE(self_st);
                self_or_null = PyStackRef_NULL;
            }
            DECREF_INPUTS();

            attr = PyStackRef_FromPyObjectSteal(attr_o);
        }

        family(LOAD_ATTR, INLINE_CACHE_ENTRIES_LOAD_ATTR) = {
            LOAD_ATTR_INSTANCE_VALUE,
            LOAD_ATTR_MODULE,
            LOAD_ATTR_WITH_HINT,
            LOAD_ATTR_SLOT,
            LOAD_ATTR_CLASS,
            LOAD_ATTR_CLASS_WITH_METACLASS_CHECK,
            LOAD_ATTR_PROPERTY,
            LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN,
            LOAD_ATTR_METHOD_WITH_VALUES,
            LOAD_ATTR_METHOD_NO_DICT,
            LOAD_ATTR_METHOD_LAZY_DICT,
            LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES,
            LOAD_ATTR_NONDESCRIPTOR_NO_DICT,
        };

        specializing op(_SPECIALIZE_LOAD_ATTR, (counter/1, owner -- owner)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
                next_instr = this_instr;
                _Py_Specialize_LoadAttr(owner, next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        op(_LOAD_ATTR, (owner -- attr, self_or_null[oparg&1])) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 1);
            PyObject *attr_o;
            if (oparg & 1) {
                /* Designed to work in tandem with CALL, pushes two values. */
                attr_o = NULL;
                int is_meth = _PyObject_GetMethod(PyStackRef_AsPyObjectBorrow(owner), name, &attr_o);
                if (is_meth) {
                    /* We can bypass temporary bound method object.
                       meth is unbound method and obj is self.
                       meth | self | arg1 | ... | argN
                     */
                    assert(attr_o != NULL);  // No errors on this branch
                    self_or_null[0] = owner;  // Transfer ownership
                    DEAD(owner);
                }
                else {
                    /* meth is not an unbound method (but a regular attr, or
                       something was returned by a descriptor protocol).  Set
                       the second element of the stack to NULL, to signal
                       CALL that it's not a method call.
                       meth | NULL | arg1 | ... | argN
                    */
                    PyStackRef_CLOSE(owner);
                    ERROR_IF(attr_o == NULL);
                    self_or_null[0] = PyStackRef_NULL;
                }
            }
            else {
                /* Classic, pushes one value. */
                attr_o = PyObject_GetAttr(PyStackRef_AsPyObjectBorrow(owner), name);
                PyStackRef_CLOSE(owner);
                ERROR_IF(attr_o == NULL);
            }
            attr = PyStackRef_FromPyObjectSteal(attr_o);
        }

        macro(LOAD_ATTR) =
            _SPECIALIZE_LOAD_ATTR +
            unused/8 +
            _LOAD_ATTR;

        op(_GUARD_TYPE_VERSION, (type_version/2, owner -- owner)) {
            PyTypeObject *tp = Py_TYPE(PyStackRef_AsPyObjectBorrow(owner));
            assert(type_version != 0);
            EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version);
        }

        op(_GUARD_TYPE_VERSION_AND_LOCK, (type_version/2, owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(type_version != 0);
            EXIT_IF(!LOCK_OBJECT(owner_o));
            PyTypeObject *tp = Py_TYPE(owner_o);
            if (FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) != type_version) {
                UNLOCK_OBJECT(owner_o);
                EXIT_IF(true);
            }
        }

        op(_CHECK_MANAGED_OBJECT_HAS_VALUES, (owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            DEOPT_IF(!FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid));
        }

        op(_LOAD_ATTR_INSTANCE_VALUE, (offset/1, owner -- attr)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            PyObject **value_ptr = (PyObject**)(((char *)owner_o) + offset);
            PyObject *attr_o = FT_ATOMIC_LOAD_PTR_ACQUIRE(*value_ptr);
            DEOPT_IF(attr_o == NULL);
            #ifdef Py_GIL_DISABLED
            int increfed = _Py_TryIncrefCompareStackRef(value_ptr, attr_o, &attr);
            if (!increfed) {
                DEOPT_IF(true);
            }
            #else
            attr = PyStackRef_FromPyObjectNew(attr_o);
            #endif
            STAT_INC(LOAD_ATTR, hit);
            PyStackRef_CLOSE(owner);
        }

        macro(LOAD_ATTR_INSTANCE_VALUE) =
            unused/1 + // Skip over the counter
            _GUARD_TYPE_VERSION +
            _CHECK_MANAGED_OBJECT_HAS_VALUES +
            _LOAD_ATTR_INSTANCE_VALUE +
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        op(_LOAD_ATTR_MODULE, (dict_version/2, index/1, owner -- attr)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            DEOPT_IF(Py_TYPE(owner_o)->tp_getattro != PyModule_Type.tp_getattro);
            PyDictObject *dict = (PyDictObject *)((PyModuleObject *)owner_o)->md_dict;
            assert(dict != NULL);
            PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != dict_version);
            assert(keys->dk_kind == DICT_KEYS_UNICODE);
            assert(index < FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries));
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(keys) + index;
            PyObject *attr_o = FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_value);
            DEOPT_IF(attr_o == NULL);
            #ifdef Py_GIL_DISABLED
            int increfed = _Py_TryIncrefCompareStackRef(&ep->me_value, attr_o, &attr);
            if (!increfed) {
                DEOPT_IF(true);
            }
            #else
            attr = PyStackRef_FromPyObjectNew(attr_o);
            #endif
            STAT_INC(LOAD_ATTR, hit);
            PyStackRef_CLOSE(owner);
        }

        macro(LOAD_ATTR_MODULE) =
            unused/1 +
            _LOAD_ATTR_MODULE +
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        op(_LOAD_ATTR_WITH_HINT, (hint/1, owner -- attr)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            DEOPT_IF(dict == NULL);
            PyDictKeysObject *dk = FT_ATOMIC_LOAD_PTR(dict->ma_keys);
            assert(PyDict_CheckExact((PyObject *)dict));
#ifdef Py_GIL_DISABLED
            DEOPT_IF(!_Py_IsOwnedByCurrentThread((PyObject *)dict) && !_PyObject_GC_IS_SHARED(dict));
#endif
            PyObject *attr_o;
            if (hint >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_nentries)) {
                DEOPT_IF(true);
            }

            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
            if (dk->dk_kind != DICT_KEYS_UNICODE) {
                DEOPT_IF(true);
            }
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dk) + hint;
            if (FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_key) != name) {
                DEOPT_IF(true);
            }
            attr_o = FT_ATOMIC_LOAD_PTR(ep->me_value);
            if (attr_o == NULL) {
                DEOPT_IF(true);
            }
            STAT_INC(LOAD_ATTR, hit);
#ifdef Py_GIL_DISABLED
            int increfed = _Py_TryIncrefCompareStackRef(&ep->me_value, attr_o, &attr);
            if (!increfed) {
                DEOPT_IF(true);
            }
#else
            attr = PyStackRef_FromPyObjectNew(attr_o);
#endif
            PyStackRef_CLOSE(owner);
        }

        macro(LOAD_ATTR_WITH_HINT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _LOAD_ATTR_WITH_HINT +
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        op(_LOAD_ATTR_SLOT, (index/1, owner -- attr)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            PyObject **addr = (PyObject **)((char *)owner_o + index);
            PyObject *attr_o = FT_ATOMIC_LOAD_PTR(*addr);
            DEOPT_IF(attr_o == NULL);
            #ifdef Py_GIL_DISABLED
            int increfed = _Py_TryIncrefCompareStackRef(addr, attr_o, &attr);
            DEOPT_IF(!increfed);
            #else
            attr = PyStackRef_FromPyObjectNew(attr_o);
            #endif
            STAT_INC(LOAD_ATTR, hit);
            DECREF_INPUTS();
        }

        macro(LOAD_ATTR_SLOT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _LOAD_ATTR_SLOT +  // NOTE: This action may also deopt
            unused/5 +
            _PUSH_NULL_CONDITIONAL;

        op(_CHECK_ATTR_CLASS, (type_version/2, owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            EXIT_IF(!PyType_Check(owner_o));
            assert(type_version != 0);
            EXIT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(((PyTypeObject *)owner_o)->tp_version_tag) != type_version);
        }

        op(_LOAD_ATTR_CLASS, (descr/4, owner -- attr)) {
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            attr = PyStackRef_FromPyObjectNew(descr);
            DECREF_INPUTS();
        }

        macro(LOAD_ATTR_CLASS) =
            unused/1 +
            _CHECK_ATTR_CLASS +
            unused/2 +
            _LOAD_ATTR_CLASS +
            _PUSH_NULL_CONDITIONAL;

        macro(LOAD_ATTR_CLASS_WITH_METACLASS_CHECK) =
            unused/1 +
            _CHECK_ATTR_CLASS +
            _GUARD_TYPE_VERSION +
            _LOAD_ATTR_CLASS +
            _PUSH_NULL_CONDITIONAL;

        op(_LOAD_ATTR_PROPERTY_FRAME, (fget/4, owner -- new_frame)) {
            assert((oparg & 1) == 0);
            assert(Py_IS_TYPE(fget, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)fget;
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            DEOPT_IF((code->co_flags & (CO_VARKEYWORDS | CO_VARARGS | CO_OPTIMIZED)) != CO_OPTIMIZED);
            DEOPT_IF(code->co_kwonlyargcount);
            DEOPT_IF(code->co_argcount != 1);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            STAT_INC(LOAD_ATTR, hit);
            _PyInterpreterFrame *pushed_frame = _PyFrame_PushUnchecked(tstate, PyStackRef_FromPyObjectNew(fget), 1, frame);
            pushed_frame->localsplus[0] = owner;
            DEAD(owner);
            new_frame = PyStackRef_Wrap(pushed_frame);
        }

        macro(LOAD_ATTR_PROPERTY) =
            unused/1 +
            _CHECK_PEP_523 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_PROPERTY_FRAME +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        inst(LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN, (unused/1, type_version/2, func_version/2, getattribute/4, owner -- unused)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            assert((oparg & 1) == 0);
            DEOPT_IF(tstate->interp->eval_frame);
            PyTypeObject *cls = Py_TYPE(owner_o);
            assert(type_version != 0);
            DEOPT_IF(FT_ATOMIC_LOAD_UINT_RELAXED(cls->tp_version_tag) != type_version);
            assert(Py_IS_TYPE(getattribute, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)getattribute;
            assert(func_version != 0);
            DEOPT_IF(f->func_version != func_version);
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            assert(code->co_argcount == 2);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            STAT_INC(LOAD_ATTR, hit);

            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 1);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(
                tstate, PyStackRef_FromPyObjectNew(f), 2, frame);
            new_frame->localsplus[0] = owner;
            DEAD(owner);
            // Manipulate stack directly because we exit with DISPATCH_INLINED().
            SYNC_SP();
            new_frame->localsplus[1] = PyStackRef_FromPyObjectNew(name);
            frame->return_offset = INSTRUCTION_SIZE;
            DISPATCH_INLINED(new_frame);
        }

        op(_GUARD_DORV_NO_DICT, (owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            if (_PyObject_GetManagedDict(owner_o) ||
                    !FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner_o)->valid)) {
                UNLOCK_OBJECT(owner_o);
                EXIT_IF(true);
            }
        }

        op(_STORE_ATTR_INSTANCE_VALUE, (offset/1, value, owner --)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            STAT_INC(STORE_ATTR, hit);
            assert(_PyObject_GetManagedDict(owner_o) == NULL);
            PyObject **value_ptr = (PyObject**)(((char *)owner_o) + offset);
            PyObject *old_value = *value_ptr;
            FT_ATOMIC_STORE_PTR_RELEASE(*value_ptr, PyStackRef_AsPyObjectSteal(value));
            if (old_value == NULL) {
                PyDictValues *values = _PyObject_InlineValues(owner_o);
                Py_ssize_t index = value_ptr - values->values;
                _PyDictValues_AddToInsertionOrder(values, index);
            }
            UNLOCK_OBJECT(owner_o);
            PyStackRef_CLOSE(owner);
            Py_XDECREF(old_value);
        }

        macro(STORE_ATTR_INSTANCE_VALUE) =
            unused/1 +
            _GUARD_TYPE_VERSION_AND_LOCK +
            _GUARD_DORV_NO_DICT +
            _STORE_ATTR_INSTANCE_VALUE;

        op(_STORE_ATTR_WITH_HINT, (hint/1, value, owner --)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            DEOPT_IF(dict == NULL);
            DEOPT_IF(!LOCK_OBJECT(dict));
            #ifdef Py_GIL_DISABLED
            if (dict != _PyObject_GetManagedDict(owner_o)) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true);
            }
            #endif
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            if (hint >= (size_t)dict->ma_keys->dk_nentries ||
                    !DK_IS_UNICODE(dict->ma_keys)) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true);
            }
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + hint;
            if (ep->me_key != name) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true);
            }
            PyObject *old_value = ep->me_value;
            if (old_value == NULL) {
                UNLOCK_OBJECT(dict);
                DEOPT_IF(true);
            }
            _PyDict_NotifyEvent(tstate->interp, PyDict_EVENT_MODIFIED, dict, name, PyStackRef_AsPyObjectBorrow(value));
            FT_ATOMIC_STORE_PTR_RELEASE(ep->me_value, PyStackRef_AsPyObjectSteal(value));
            UNLOCK_OBJECT(dict);

            // old_value should be DECREFed after GC track checking is done, if not, it could raise a segmentation fault,
            // when dict only holds the strong reference to value in ep->me_value.
            STAT_INC(STORE_ATTR, hit);
            PyStackRef_CLOSE(owner);
            Py_XDECREF(old_value);
        }

        macro(STORE_ATTR_WITH_HINT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _STORE_ATTR_WITH_HINT;

        op(_STORE_ATTR_SLOT, (index/1, value, owner --)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            DEOPT_IF(!LOCK_OBJECT(owner_o));
            char *addr = (char *)owner_o + index;
            STAT_INC(STORE_ATTR, hit);
            PyObject *old_value = *(PyObject **)addr;
            FT_ATOMIC_STORE_PTR_RELEASE(*(PyObject **)addr, PyStackRef_AsPyObjectSteal(value));
            UNLOCK_OBJECT(owner_o);
            PyStackRef_CLOSE(owner);
            Py_XDECREF(old_value);
        }

        macro(STORE_ATTR_SLOT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _STORE_ATTR_SLOT;

        family(COMPARE_OP, INLINE_CACHE_ENTRIES_COMPARE_OP) = {
            COMPARE_OP_FLOAT,
            COMPARE_OP_INT,
            COMPARE_OP_STR,
        };

        specializing op(_SPECIALIZE_COMPARE_OP, (counter/1, left, right -- left, right)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_CompareOp(left, right, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(COMPARE_OP);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        op(_COMPARE_OP, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            assert((oparg >> 5) <= Py_GE);
            PyObject *res_o = PyObject_RichCompare(left_o, right_o, oparg >> 5);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            if (oparg & 16) {
                int res_bool = PyObject_IsTrue(res_o);
                Py_DECREF(res_o);
                ERROR_IF(res_bool < 0);
                res = res_bool ? PyStackRef_True : PyStackRef_False;
            }
            else {
                res = PyStackRef_FromPyObjectSteal(res_o);
            }
        }

        macro(COMPARE_OP) = _SPECIALIZE_COMPARE_OP + _COMPARE_OP;

        macro(COMPARE_OP_FLOAT) =
            _GUARD_TOS_FLOAT + _GUARD_NOS_FLOAT + unused/1 + _COMPARE_OP_FLOAT;

        macro(COMPARE_OP_INT) =
            _GUARD_TOS_INT + _GUARD_NOS_INT + unused/1 + _COMPARE_OP_INT;

        macro(COMPARE_OP_STR) =
            _GUARD_TOS_UNICODE + _GUARD_NOS_UNICODE + unused/1 + _COMPARE_OP_STR;

        op(_COMPARE_OP_FLOAT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(COMPARE_OP, hit);
            double dleft = PyFloat_AS_DOUBLE(left_o);
            double dright = PyFloat_AS_DOUBLE(right_o);
            // 1 if NaN, 2 if <, 4 if >, 8 if ==; this matches low four bits of the oparg
            int sign_ish = COMPARISON_BIT(dleft, dright);
            PyStackRef_CLOSE_SPECIALIZED(left, _PyFloat_ExactDealloc);
            DEAD(left);
            PyStackRef_CLOSE_SPECIALIZED(right, _PyFloat_ExactDealloc);
            DEAD(right);
            res = (sign_ish & oparg) ? PyStackRef_True : PyStackRef_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        // Similar to COMPARE_OP_FLOAT
        op(_COMPARE_OP_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            assert(_PyLong_IsCompact((PyLongObject *)left_o));
            assert(_PyLong_IsCompact((PyLongObject *)right_o));
            STAT_INC(COMPARE_OP, hit);
            assert(_PyLong_DigitCount((PyLongObject *)left_o) <= 1 &&
                   _PyLong_DigitCount((PyLongObject *)right_o) <= 1);
            Py_ssize_t ileft = _PyLong_CompactValue((PyLongObject *)left_o);
            Py_ssize_t iright = _PyLong_CompactValue((PyLongObject *)right_o);
            // 2 if <, 4 if >, 8 if ==; this matches the low 4 bits of the oparg
            int sign_ish = COMPARISON_BIT(ileft, iright);
            PyStackRef_CLOSE_SPECIALIZED(left, _PyLong_ExactDealloc);
            DEAD(left);
            PyStackRef_CLOSE_SPECIALIZED(right, _PyLong_ExactDealloc);
            DEAD(right);
            res =  (sign_ish & oparg) ? PyStackRef_True : PyStackRef_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        // Similar to COMPARE_OP_FLOAT, but for ==, != only
        op(_COMPARE_OP_STR, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(COMPARE_OP, hit);
            int eq = _PyUnicode_Equal(left_o, right_o);
            assert((oparg >> 5) == Py_EQ || (oparg >> 5) == Py_NE);
            PyStackRef_CLOSE_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            DEAD(left);
            PyStackRef_CLOSE_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            DEAD(right);
            assert(eq == 0 || eq == 1);
            assert((oparg & 0xf) == COMPARISON_NOT_EQUALS || (oparg & 0xf) == COMPARISON_EQUALS);
            assert(COMPARISON_NOT_EQUALS + 1 == COMPARISON_EQUALS);
            res = ((COMPARISON_NOT_EQUALS + eq) & oparg) ? PyStackRef_True : PyStackRef_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        inst(IS_OP, (left, right -- b)) {
            int res = Py_Is(PyStackRef_AsPyObjectBorrow(left), PyStackRef_AsPyObjectBorrow(right)) ^ oparg;
            DECREF_INPUTS();
            b = res ? PyStackRef_True : PyStackRef_False;
        }

        family(CONTAINS_OP, INLINE_CACHE_ENTRIES_CONTAINS_OP) = {
            CONTAINS_OP_SET,
            CONTAINS_OP_DICT,
        };

        op(_CONTAINS_OP, (left, right -- b)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            int res = PySequence_Contains(right_o, left_o);
            DECREF_INPUTS();
            ERROR_IF(res < 0);
            b = (res ^ oparg) ? PyStackRef_True : PyStackRef_False;
        }

        specializing op(_SPECIALIZE_CONTAINS_OP, (counter/1, left, right -- left, right)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_ContainsOp(right, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CONTAINS_OP);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        macro(CONTAINS_OP) = _SPECIALIZE_CONTAINS_OP + _CONTAINS_OP;

        op(_GUARD_TOS_ANY_SET, (tos -- tos)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(tos);
            DEOPT_IF(!PyAnySet_CheckExact(o));
        }

        macro(CONTAINS_OP_SET) = _GUARD_TOS_ANY_SET + unused/1 + _CONTAINS_OP_SET;

        op(_CONTAINS_OP_SET, (left, right -- b)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            assert(PyAnySet_CheckExact(right_o));
            STAT_INC(CONTAINS_OP, hit);
            // Note: both set and frozenset use the same seq_contains method!
            int res = _PySet_Contains((PySetObject *)right_o, left_o);
            DECREF_INPUTS();
            ERROR_IF(res < 0);
            b = (res ^ oparg) ? PyStackRef_True : PyStackRef_False;
        }

        macro(CONTAINS_OP_DICT) = _GUARD_TOS_DICT + unused/1 + _CONTAINS_OP_DICT;

        op(_CONTAINS_OP_DICT, (left, right -- b)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            assert(PyDict_CheckExact(right_o));
            STAT_INC(CONTAINS_OP, hit);
            int res = PyDict_Contains(right_o, left_o);
            DECREF_INPUTS();
            ERROR_IF(res < 0);
            b = (res ^ oparg) ? PyStackRef_True : PyStackRef_False;
        }

        inst(CHECK_EG_MATCH, (exc_value_st, match_type_st -- rest, match)) {
            PyObject *exc_value = PyStackRef_AsPyObjectBorrow(exc_value_st);
            PyObject *match_type = PyStackRef_AsPyObjectBorrow(match_type_st);
            int err = _PyEval_CheckExceptStarTypeValid(tstate, match_type);
            if (err < 0) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }

            PyObject *match_o = NULL;
            PyObject *rest_o = NULL;
            int res = _PyEval_ExceptionGroupMatch(frame, exc_value, match_type,
                                                  &match_o, &rest_o);
            DECREF_INPUTS();
            ERROR_IF(res < 0);

            assert((match_o == NULL) == (rest_o == NULL));
            ERROR_IF(match_o == NULL);

            if (!Py_IsNone(match_o)) {
                PyErr_SetHandledException(match_o);
            }
            rest = PyStackRef_FromPyObjectSteal(rest_o);
            match = PyStackRef_FromPyObjectSteal(match_o);
        }

        inst(CHECK_EXC_MATCH, (left, right -- left, b)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            assert(PyExceptionInstance_Check(left_o));
            int err = _PyEval_CheckExceptTypeValid(tstate, right_o);
            if (err < 0) {
                ERROR_NO_POP();
            }

            int res = PyErr_GivenExceptionMatches(left_o, right_o);
            PyStackRef_CLOSE(right);
            b = res ? PyStackRef_True : PyStackRef_False;
        }

         inst(IMPORT_NAME, (level, fromlist -- res)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *res_o = _PyEval_ImportName(tstate, frame, name,
                              PyStackRef_AsPyObjectBorrow(fromlist),
                              PyStackRef_AsPyObjectBorrow(level));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(IMPORT_FROM, (from -- from, res)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *res_o = _PyEval_ImportFrom(tstate, PyStackRef_AsPyObjectBorrow(from), name);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        tier1 inst(JUMP_FORWARD, (--)) {
            JUMPBY(oparg);
        }

        family(JUMP_BACKWARD, 1) = {
            JUMP_BACKWARD_NO_JIT,
            JUMP_BACKWARD_JIT,
        };

        tier1 op(_SPECIALIZE_JUMP_BACKWARD, (--)) {
        #if ENABLE_SPECIALIZATION
            if (this_instr->op.code == JUMP_BACKWARD) {
                this_instr->op.code = tstate->interp->jit ? JUMP_BACKWARD_JIT : JUMP_BACKWARD_NO_JIT;
                // Need to re-dispatch so the warmup counter isn't off by one:
                next_instr = this_instr;
                DISPATCH_SAME_OPARG();
            }
        #endif
        }

        tier1 op(_JIT, (--)) {
        #ifdef _Py_TIER2
            _Py_BackoffCounter counter = this_instr[1].counter;
            if (backoff_counter_triggers(counter) && this_instr->op.code == JUMP_BACKWARD_JIT) {
                _Py_CODEUNIT *start = this_instr;
                /* Back up over EXTENDED_ARGs so optimizer sees the whole instruction */
                while (oparg > 255) {
                    oparg >>= 8;
                    start--;
                }
                _PyExecutorObject *executor;
                int optimized = _PyOptimizer_Optimize(frame, start, &executor, 0);
                if (optimized <= 0) {
                    this_instr[1].counter = restart_backoff_counter(counter);
                    ERROR_IF(optimized < 0);
                }
                else {
                    this_instr[1].counter = initial_jump_backoff_counter();
                    assert(tstate->current_executor == NULL);
                    GOTO_TIER_TWO(executor);
                }
            }
            else {
                ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            }
        #endif
        }

        macro(JUMP_BACKWARD) =
            unused/1 +
            _SPECIALIZE_JUMP_BACKWARD +
            _CHECK_PERIODIC +
            JUMP_BACKWARD_NO_INTERRUPT;

        macro(JUMP_BACKWARD_NO_JIT) =
            unused/1 +
            _CHECK_PERIODIC +
            JUMP_BACKWARD_NO_INTERRUPT;

        macro(JUMP_BACKWARD_JIT) =
            unused/1 +
            _CHECK_PERIODIC +
            JUMP_BACKWARD_NO_INTERRUPT +
            _JIT;

        pseudo(JUMP, (--)) = {
            JUMP_FORWARD,
            JUMP_BACKWARD,
        };

        pseudo(JUMP_NO_INTERRUPT, (--)) = {
            JUMP_FORWARD,
            JUMP_BACKWARD_NO_INTERRUPT,
        };

        pseudo(JUMP_IF_FALSE, (cond -- cond)) = [
            COPY, TO_BOOL, POP_JUMP_IF_FALSE,
        ];

        pseudo(JUMP_IF_TRUE, (cond -- cond)) = [
            COPY, TO_BOOL, POP_JUMP_IF_TRUE,
        ];

        tier1 inst(ENTER_EXECUTOR, (--)) {
            #ifdef _Py_TIER2
            PyCodeObject *code = _PyFrame_GetCode(frame);
            _PyExecutorObject *executor = code->co_executors->executors[oparg & 255];
            assert(executor->vm_data.index == INSTR_OFFSET() - 1);
            assert(executor->vm_data.code == code);
            assert(executor->vm_data.valid);
            assert(tstate->current_executor == NULL);
            /* If the eval breaker is set then stay in tier 1.
             * This avoids any potentially infinite loops
             * involving _RESUME_CHECK */
            if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
                opcode = executor->vm_data.opcode;
                oparg = (oparg & ~255) | executor->vm_data.oparg;
                next_instr = this_instr;
                if (_PyOpcode_Caches[_PyOpcode_Deopt[opcode]]) {
                    PAUSE_ADAPTIVE_COUNTER(this_instr[1].counter);
                }
                DISPATCH_GOTO();
            }
            GOTO_TIER_TWO(executor);
            #else
            Py_FatalError("ENTER_EXECUTOR is not supported in this build");
            #endif /* _Py_TIER2 */
        }

        replaced op(_POP_JUMP_IF_FALSE, (cond -- )) {
            assert(PyStackRef_BoolCheck(cond));
            int flag = PyStackRef_IsFalse(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(this_instr[1].cache, flag);
            JUMPBY(flag ? oparg : next_instr->op.code == NOT_TAKEN);
        }

        replaced op(_POP_JUMP_IF_TRUE, (cond -- )) {
            assert(PyStackRef_BoolCheck(cond));
            int flag = PyStackRef_IsTrue(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(this_instr[1].cache, flag);
            JUMPBY(flag ? oparg : next_instr->op.code == NOT_TAKEN);
        }

        op(_IS_NONE, (value -- b)) {
            if (PyStackRef_IsNone(value)) {
                b = PyStackRef_True;
                DEAD(value);
            }
            else {
                b = PyStackRef_False;
                DECREF_INPUTS();
            }
        }

        macro(POP_JUMP_IF_TRUE) = unused/1 + _POP_JUMP_IF_TRUE;

        macro(POP_JUMP_IF_FALSE) = unused/1 + _POP_JUMP_IF_FALSE;

        macro(POP_JUMP_IF_NONE) = unused/1 + _IS_NONE + _POP_JUMP_IF_TRUE;

        macro(POP_JUMP_IF_NOT_NONE) = unused/1 + _IS_NONE + _POP_JUMP_IF_FALSE;

        tier1 inst(JUMP_BACKWARD_NO_INTERRUPT, (--)) {
            /* This bytecode is used in the `yield from` or `await` loop.
             * If there is an interrupt, we want it handled in the innermost
             * generator or coroutine, so we deliberately do not check it here.
             * (see bpo-30039).
             */
            assert(oparg <= INSTR_OFFSET());
            JUMPBY(-oparg);
        }

        inst(GET_LEN, (obj -- obj, len)) {
            // PUSH(len(TOS))
            Py_ssize_t len_i = PyObject_Length(PyStackRef_AsPyObjectBorrow(obj));
            ERROR_IF(len_i < 0);
            PyObject *len_o = PyLong_FromSsize_t(len_i);
            ERROR_IF(len_o == NULL);
            len = PyStackRef_FromPyObjectSteal(len_o);
        }

        inst(MATCH_CLASS, (subject, type, names -- attrs)) {
            // Pop TOS and TOS1. Set TOS to a tuple of attributes on success, or
            // None on failure.
            assert(PyTuple_CheckExact(PyStackRef_AsPyObjectBorrow(names)));
            PyObject *attrs_o = _PyEval_MatchClass(tstate,
                PyStackRef_AsPyObjectBorrow(subject),
                PyStackRef_AsPyObjectBorrow(type), oparg,
                PyStackRef_AsPyObjectBorrow(names));
            DECREF_INPUTS();
            if (attrs_o) {
                assert(PyTuple_CheckExact(attrs_o));  // Success!
                attrs = PyStackRef_FromPyObjectSteal(attrs_o);
            }
            else {
                ERROR_IF(_PyErr_Occurred(tstate));  // Error!
                attrs = PyStackRef_None;  // Failure!
            }
        }

        inst(MATCH_MAPPING, (subject -- subject, res)) {
            int match = PyStackRef_TYPE(subject)->tp_flags & Py_TPFLAGS_MAPPING;
            res = match ? PyStackRef_True : PyStackRef_False;
        }

        inst(MATCH_SEQUENCE, (subject -- subject, res)) {
            int match = PyStackRef_TYPE(subject)->tp_flags & Py_TPFLAGS_SEQUENCE;
            res = match ? PyStackRef_True : PyStackRef_False;
        }

        inst(MATCH_KEYS, (subject, keys -- subject, keys, values_or_none)) {
            // On successful match, PUSH(values). Otherwise, PUSH(None).
            PyObject *values_or_none_o = _PyEval_MatchKeys(tstate,
                PyStackRef_AsPyObjectBorrow(subject), PyStackRef_AsPyObjectBorrow(keys));
            ERROR_IF(values_or_none_o == NULL);
            values_or_none = PyStackRef_FromPyObjectSteal(values_or_none_o);
        }

        inst(GET_ITER, (iterable -- iter, index_or_null)) {
            #ifdef Py_STATS
            _Py_GatherStats_GetIter(iterable);
            #endif
            /* before: [obj]; after [getiter(obj)] */
            PyTypeObject *tp = PyStackRef_TYPE(iterable);
            if (tp == &PyTuple_Type || tp == &PyList_Type) {
                iter = iterable;
                DEAD(iterable);
                index_or_null = PyStackRef_TagInt(0);
            }
            else {
                PyObject *iter_o = PyObject_GetIter(PyStackRef_AsPyObjectBorrow(iterable));
                PyStackRef_CLOSE(iterable);
                ERROR_IF(iter_o == NULL);
                iter = PyStackRef_FromPyObjectSteal(iter_o);
                index_or_null = PyStackRef_NULL;
            }
        }

        inst(GET_YIELD_FROM_ITER, (iterable -- iter)) {
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable_o = PyStackRef_AsPyObjectBorrow(iterable);
            if (PyCoro_CheckExact(iterable_o)) {
                /* `iterable` is a coroutine */
                if (!(_PyFrame_GetCode(frame)->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))) {
                    /* and it is used in a 'yield from' expression of a
                       regular generator. */
                    _PyErr_SetString(tstate, PyExc_TypeError,
                                     "cannot 'yield from' a coroutine object "
                                     "in a non-coroutine generator");
                    ERROR_NO_POP();
                }
                iter = iterable;
                DEAD(iterable);
            }
            else if (PyGen_CheckExact(iterable_o)) {
                iter = iterable;
                DEAD(iterable);
            }
            else {
                /* `iterable` is not a generator. */
                PyObject *iter_o = PyObject_GetIter(iterable_o);
                if (iter_o == NULL) {
                    ERROR_NO_POP();
                }
                iter = PyStackRef_FromPyObjectSteal(iter_o);
                DECREF_INPUTS();
            }
        }

        // Most members of this family are "secretly" super-instructions.
        // When the loop is exhausted, they jump, and the jump target is
        // always END_FOR, which pops two values off the stack.
        // This is optimized by skipping that instruction and combining
        // its effect (popping 'iter' instead of pushing 'next'.)

        family(FOR_ITER, INLINE_CACHE_ENTRIES_FOR_ITER) = {
            FOR_ITER_LIST,
            FOR_ITER_TUPLE,
            FOR_ITER_RANGE,
            FOR_ITER_GEN,
        };

        specializing op(_SPECIALIZE_FOR_ITER, (counter/1, iter, null_or_index -- iter, null_or_index)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_ForIter(iter, null_or_index, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(FOR_ITER);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        replaced op(_FOR_ITER, (iter, null_or_index -- iter, null_or_index, next)) {
            _PyStackRef item = _PyForIter_VirtualIteratorNext(tstate, frame, iter, &null_or_index);
            if (!PyStackRef_IsValid(item)) {
                if (PyStackRef_IsError(item)) {
                    ERROR_NO_POP();
                }
                // Jump forward by oparg and skip the following END_FOR
                JUMPBY(oparg + 1);
                DISPATCH();
            }
            next = item;
        }

        op(_FOR_ITER_TIER_TWO, (iter, null_or_index -- iter, null_or_index, next)) {
            _PyStackRef item = _PyForIter_VirtualIteratorNext(tstate, frame, iter, &null_or_index);
            if (!PyStackRef_IsValid(item)) {
                if (PyStackRef_IsError(item)) {
                    ERROR_NO_POP();
                }
                /* iterator ended normally */
                /* The translator sets the deopt target just past the matching END_FOR */
                EXIT_IF(true);
            }
            next = item;
        }


        macro(FOR_ITER) = _SPECIALIZE_FOR_ITER + _FOR_ITER;


        inst(INSTRUMENTED_FOR_ITER, (unused/1, iter, null_or_index -- iter, null_or_index, next)) {
            _PyStackRef item = _PyForIter_VirtualIteratorNext(tstate, frame, iter, &null_or_index);
            if (!PyStackRef_IsValid(item)) {
                if (PyStackRef_IsError(item)) {
                    ERROR_NO_POP();
                }
                // Jump forward by oparg and skip the following END_FOR
                JUMPBY(oparg + 1);
                DISPATCH();
            }
            next = item;
            INSTRUMENTED_JUMP(this_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT);
        }

        op(_ITER_CHECK_LIST, (iter, null_or_index -- iter, null_or_index)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            EXIT_IF(Py_TYPE(iter_o) != &PyList_Type);
            assert(PyStackRef_IsTaggedInt(null_or_index));
#ifdef Py_GIL_DISABLED
            EXIT_IF(!_Py_IsOwnedByCurrentThread(iter_o) && !_PyObject_GC_IS_SHARED(iter_o));
#endif
        }

        replaced op(_ITER_JUMP_LIST, (iter, null_or_index -- iter, null_or_index)) {
#ifdef Py_GIL_DISABLED
            // For free-threaded Python, the loop exit can happen at any point during
            // item retrieval, so it doesn't make much sense to check and jump
            // separately before item retrieval. Any length check we do here can be
            // invalid by the time we actually try to fetch the item.
#else
            PyObject *list_o = PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(list_o) == &PyList_Type);
            STAT_INC(FOR_ITER, hit);
            if ((size_t)PyStackRef_UntagInt(null_or_index) >= (size_t)PyList_GET_SIZE(list_o)) {
                null_or_index = PyStackRef_TagInt(-1);
                /* Jump forward oparg, then skip following END_FOR instruction */
                JUMPBY(oparg + 1);
                DISPATCH();
            }
#endif
        }

        // Only used by Tier 2
        op(_GUARD_NOT_EXHAUSTED_LIST, (iter, null_or_index -- iter, null_or_index)) {
#ifndef Py_GIL_DISABLED
            PyObject *list_o = PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(list_o) == &PyList_Type);
            EXIT_IF((size_t)PyStackRef_UntagInt(null_or_index) >= (size_t)PyList_GET_SIZE(list_o));
#endif
        }

        replaced op(_ITER_NEXT_LIST, (iter, null_or_index -- iter, null_or_index, next)) {
            PyObject *list_o = PyStackRef_AsPyObjectBorrow(iter);
            assert(PyList_CheckExact(list_o));
#ifdef Py_GIL_DISABLED
            assert(_Py_IsOwnedByCurrentThread(list_o) ||
                   _PyObject_GC_IS_SHARED(list_o));
            STAT_INC(FOR_ITER, hit);
            int result = _PyList_GetItemRefNoLock((PyListObject *)list_o, PyStackRef_UntagInt(null_or_index), &next);
            // A negative result means we lost a race with another thread
            // and we need to take the slow path.
            DEOPT_IF(result < 0);
            if (result == 0) {
                null_or_index = PyStackRef_TagInt(-1);
                /* Jump forward oparg, then skip following END_FOR instruction */
                JUMPBY(oparg + 1);
                DISPATCH();
            }
#else
            next = PyStackRef_FromPyObjectNew(PyList_GET_ITEM(list_o, PyStackRef_UntagInt(null_or_index)));
#endif
            null_or_index = PyStackRef_IncrementTaggedIntNoOverflow(null_or_index);
        }

        // Only used by Tier 2
        op(_ITER_NEXT_LIST_TIER_TWO, (iter, null_or_index -- iter, null_or_index, next)) {
            PyObject *list_o = PyStackRef_AsPyObjectBorrow(iter);
            assert(PyList_CheckExact(list_o));
#ifdef Py_GIL_DISABLED
            assert(_Py_IsOwnedByCurrentThread((PyObject *)list_o) ||
                   _PyObject_GC_IS_SHARED(list_o));
            STAT_INC(FOR_ITER, hit);
            int result = _PyList_GetItemRefNoLock((PyListObject *)list_o, PyStackRef_UntagInt(null_or_index), &next);
            // A negative result means we lost a race with another thread
            // and we need to take the slow path.
            DEOPT_IF(result <= 0);
#else
            assert(PyStackRef_UntagInt(null_or_index) < PyList_GET_SIZE(list_o));
            next = PyStackRef_FromPyObjectNew(PyList_GET_ITEM(list_o, PyStackRef_UntagInt(null_or_index)));
#endif
            null_or_index = PyStackRef_IncrementTaggedIntNoOverflow(null_or_index);
        }

        macro(FOR_ITER_LIST) =
            unused/1 +  // Skip over the counter
            _ITER_CHECK_LIST +
            _ITER_JUMP_LIST +
            _ITER_NEXT_LIST;

        op(_ITER_CHECK_TUPLE, (iter, null_or_index -- iter, null_or_index)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            EXIT_IF(Py_TYPE(iter_o) != &PyTuple_Type);
            assert(PyStackRef_IsTaggedInt(null_or_index));
        }

        replaced op(_ITER_JUMP_TUPLE, (iter, null_or_index -- iter, null_or_index)) {
            PyObject *tuple_o = PyStackRef_AsPyObjectBorrow(iter);
            (void)tuple_o;
            assert(Py_TYPE(tuple_o) == &PyTuple_Type);
            STAT_INC(FOR_ITER, hit);
            if ((size_t)PyStackRef_UntagInt(null_or_index) >= (size_t)PyTuple_GET_SIZE(tuple_o)) {
                null_or_index = PyStackRef_TagInt(-1);
                /* Jump forward oparg, then skip following END_FOR instruction */
                JUMPBY(oparg + 1);
                DISPATCH();
            }
        }

        // Only used by Tier 2
        op(_GUARD_NOT_EXHAUSTED_TUPLE, (iter, null_or_index -- iter, null_or_index)) {
            PyObject *tuple_o = PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(tuple_o) == &PyTuple_Type);
            EXIT_IF((size_t)PyStackRef_UntagInt(null_or_index) >= (size_t)PyTuple_GET_SIZE(tuple_o));
        }

        op(_ITER_NEXT_TUPLE, (iter, null_or_index -- iter, null_or_index, next)) {
            PyObject *tuple_o = PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(tuple_o) == &PyTuple_Type);
            uintptr_t i = PyStackRef_UntagInt(null_or_index);
            assert((size_t)i < (size_t)PyTuple_GET_SIZE(tuple_o));
            next = PyStackRef_FromPyObjectNew(PyTuple_GET_ITEM(tuple_o, i));
            null_or_index = PyStackRef_IncrementTaggedIntNoOverflow(null_or_index);
        }

        macro(FOR_ITER_TUPLE) =
            unused/1 +  // Skip over the counter
            _ITER_CHECK_TUPLE +
            _ITER_JUMP_TUPLE +
            _ITER_NEXT_TUPLE;

        op(_ITER_CHECK_RANGE, (iter, null_or_index -- iter, null_or_index)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            EXIT_IF(Py_TYPE(r) != &PyRangeIter_Type);
#ifdef Py_GIL_DISABLED
            EXIT_IF(!_PyObject_IsUniquelyReferenced((PyObject *)r));
#endif
        }

        replaced op(_ITER_JUMP_RANGE, (iter, null_or_index -- iter, null_or_index)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
#ifdef Py_GIL_DISABLED
            assert(_PyObject_IsUniquelyReferenced((PyObject *)r));
#endif
            STAT_INC(FOR_ITER, hit);
            if (r->len <= 0) {
                // Jump over END_FOR instruction.
                JUMPBY(oparg + 1);
                DISPATCH();
            }
        }

        // Only used by Tier 2
        op(_GUARD_NOT_EXHAUSTED_RANGE, (iter, null_or_index -- iter, null_or_index)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
            EXIT_IF(r->len <= 0);
        }

        op(_ITER_NEXT_RANGE, (iter, null_or_index -- iter, null_or_index, next)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
#ifdef Py_GIL_DISABLED
            assert(_PyObject_IsUniquelyReferenced((PyObject *)r));
#endif
            assert(r->len > 0);
            long value = r->start;
            r->start = value + r->step;
            r->len--;
            PyObject *res = PyLong_FromLong(value);
            ERROR_IF(res == NULL);
            next = PyStackRef_FromPyObjectSteal(res);
        }

        macro(FOR_ITER_RANGE) =
            unused/1 +  // Skip over the counter
            _ITER_CHECK_RANGE +
            _ITER_JUMP_RANGE +
            _ITER_NEXT_RANGE;

        op(_FOR_ITER_GEN_FRAME, (iter, null -- iter, null, gen_frame)) {
            PyGenObject *gen = (PyGenObject *)PyStackRef_AsPyObjectBorrow(iter);
            DEOPT_IF(Py_TYPE(gen) != &PyGen_Type);
#ifdef Py_GIL_DISABLED
            // Since generators can't be used by multiple threads anyway we
            // don't need to deopt here, but this lets us work on making
            // generators thread-safe without necessarily having to
            // specialize them thread-safely as well.
            DEOPT_IF(!_PyObject_IsUniquelyReferenced((PyObject *)gen));
#endif
            DEOPT_IF(gen->gi_frame_state >= FRAME_EXECUTING);
            STAT_INC(FOR_ITER, hit);
            _PyInterpreterFrame *pushed_frame = &gen->gi_iframe;
            _PyFrame_StackPush(pushed_frame, PyStackRef_None);
            gen->gi_frame_state = FRAME_EXECUTING;
            gen->gi_exc_state.previous_item = tstate->exc_info;
            tstate->exc_info = &gen->gi_exc_state;
            pushed_frame->previous = frame;
            // oparg is the return offset from the next instruction.
            frame->return_offset = (uint16_t)(INSTRUCTION_SIZE + oparg);
            gen_frame = PyStackRef_Wrap(pushed_frame);
        }

        macro(FOR_ITER_GEN) =
            unused/1 +
            _CHECK_PEP_523 +
            _FOR_ITER_GEN_FRAME +
            _PUSH_FRAME;

        op(_INSERT_NULL, (self -- method_and_self[2])) {
            method_and_self[1] = self;
            method_and_self[0] = PyStackRef_NULL;
            DEAD(self);
        }

        op(_LOAD_SPECIAL, (method_and_self[2] -- method_and_self[2])) {
            PyObject *name = _Py_SpecialMethods[oparg].name;
            int err = _PyObject_LookupSpecialMethod(name, method_and_self);
            if (err <= 0) {
                if (err == 0) {
                    PyObject *owner = PyStackRef_AsPyObjectBorrow(method_and_self[1]);
                    const char *errfmt = _PyEval_SpecialMethodCanSuggest(owner, oparg)
                        ? _Py_SpecialMethods[oparg].error_suggestion
                        : _Py_SpecialMethods[oparg].error;
                    assert(!_PyErr_Occurred(tstate));
                    assert(errfmt != NULL);
                    _PyErr_Format(tstate, PyExc_TypeError, errfmt, owner);
                }
                ERROR_NO_POP();
            }
        }

        macro(LOAD_SPECIAL) =
            _INSERT_NULL +
            _LOAD_SPECIAL;

        inst(WITH_EXCEPT_START, (exit_func, exit_self, lasti, unused, val -- exit_func, exit_self, lasti, unused, val, res)) {
            /* At the top of the stack are 4 values:
               - val: TOP = exc_info()
               - unused: SECOND = previous exception
               - lasti: THIRD = lasti of exception in exc_info()
               - exit_self: FOURTH = the context or NULL
               - exit_func: FIFTH = the context.__exit__ function or context.__exit__ bound method
               We call FOURTH(type(TOP), TOP, GetTraceback(TOP)).
               Then we push the __exit__ return value.
            */
            PyObject *exc, *tb;

            PyObject *val_o = PyStackRef_AsPyObjectBorrow(val);
            PyObject *exit_func_o = PyStackRef_AsPyObjectBorrow(exit_func);

            assert(val_o && PyExceptionInstance_Check(val_o));
            exc = PyExceptionInstance_Class(val_o);
            PyObject *original_tb = tb = PyException_GetTraceback(val_o);
            if (tb == NULL) {
                tb = Py_None;
            }
            assert(PyStackRef_IsTaggedInt(lasti));
            (void)lasti; // Shut up compiler warning if asserts are off
            PyObject *stack[5] = {NULL, PyStackRef_AsPyObjectBorrow(exit_self), exc, val_o, tb};
            int has_self = !PyStackRef_IsNull(exit_self);
            PyObject *res_o = PyObject_Vectorcall(exit_func_o, stack + 2 - has_self,
                    (3 + has_self) | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
            Py_XDECREF(original_tb);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pseudo(SETUP_FINALLY, (-- unused), (HAS_ARG)) = {
            /* If an exception is raised, restore the stack position
             * and push one value before jumping to the handler.
             */
            NOP,
        };

        pseudo(SETUP_CLEANUP, (-- unused, unused), (HAS_ARG)) = {
            /* As SETUP_FINALLY, but push lasti as well */
            NOP,
        };

        pseudo(SETUP_WITH, (-- unused), (HAS_ARG)) = {
            /* If an exception is raised, restore the stack position to the
             * position before the result of __(a)enter__ and push 2 values
             * before jumping to the handler.
             */
            NOP,
        };

        pseudo(POP_BLOCK, (--)) = {
            NOP,
        };

        inst(PUSH_EXC_INFO, (exc -- prev_exc, new_exc)) {

            _PyErr_StackItem *exc_info = tstate->exc_info;
            if (exc_info->exc_value != NULL) {
                prev_exc = PyStackRef_FromPyObjectSteal(exc_info->exc_value);
            }
            else {
                prev_exc = PyStackRef_None;
            }
            assert(PyStackRef_ExceptionInstanceCheck(exc));
            exc_info->exc_value = PyStackRef_AsPyObjectNew(exc);
            new_exc = exc;
            DEAD(exc);
        }

        op(_GUARD_DORV_VALUES_INST_ATTR_FROM_DICT, (owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            PyDictValues *ivs = _PyObject_InlineValues(owner_o);
            DEOPT_IF(!FT_ATOMIC_LOAD_UINT8(ivs->valid));
        }

        op(_GUARD_KEYS_VERSION, (keys_version/2, owner -- owner)) {
            PyTypeObject *owner_cls = Py_TYPE(PyStackRef_AsPyObjectBorrow(owner));
            PyHeapTypeObject *owner_heap_type = (PyHeapTypeObject *)owner_cls;
            PyDictKeysObject *keys = owner_heap_type->ht_cached_keys;
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != keys_version);
        }

        op(_LOAD_ATTR_METHOD_WITH_VALUES, (descr/4, owner -- attr, self)) {
            assert(oparg & 1);
            /* Cached method object */
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyStackRef_FromPyObjectNew(descr);
            self = owner;
            DEAD(owner);
        }

        macro(LOAD_ATTR_METHOD_WITH_VALUES) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_DORV_VALUES_INST_ATTR_FROM_DICT +
            _GUARD_KEYS_VERSION +
            _LOAD_ATTR_METHOD_WITH_VALUES;

        op(_LOAD_ATTR_METHOD_NO_DICT, (descr/4, owner -- attr, self)) {
            assert(oparg & 1);
            assert(Py_TYPE(PyStackRef_AsPyObjectBorrow(owner))->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyStackRef_FromPyObjectNew(descr);
            self = owner;
            DEAD(owner);
        }

        macro(LOAD_ATTR_METHOD_NO_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_METHOD_NO_DICT;

        op(_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES, (descr/4, owner -- attr)) {
            assert((oparg & 1) == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            PyStackRef_CLOSE(owner);
            attr = PyStackRef_FromPyObjectNew(descr);
        }

        macro(LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_DORV_VALUES_INST_ATTR_FROM_DICT +
            _GUARD_KEYS_VERSION +
            _LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES;

        op(_LOAD_ATTR_NONDESCRIPTOR_NO_DICT, (descr/4, owner -- attr)) {
            assert((oparg & 1) == 0);
            assert(Py_TYPE(PyStackRef_AsPyObjectBorrow(owner))->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            PyStackRef_CLOSE(owner);
            attr = PyStackRef_FromPyObjectNew(descr);
        }

        macro(LOAD_ATTR_NONDESCRIPTOR_NO_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_NONDESCRIPTOR_NO_DICT;

        op(_CHECK_ATTR_METHOD_LAZY_DICT, (dictoffset/1, owner -- owner)) {
            char *ptr = ((char *)PyStackRef_AsPyObjectBorrow(owner)) + MANAGED_DICT_OFFSET + dictoffset;
            PyObject *dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*(PyObject **)ptr);
            /* This object has a __dict__, just not yet created */
            DEOPT_IF(dict != NULL);
        }

        op(_LOAD_ATTR_METHOD_LAZY_DICT, (descr/4, owner -- attr, self)) {
            assert(oparg & 1);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyStackRef_FromPyObjectNew(descr);
            self = owner;
            DEAD(owner);
        }

        macro(LOAD_ATTR_METHOD_LAZY_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_ATTR_METHOD_LAZY_DICT +
            unused/1 +
            _LOAD_ATTR_METHOD_LAZY_DICT;

        // Cache layout: counter/1, func_version/2
        // CALL_INTRINSIC_1/2, CALL_KW, and CALL_FUNCTION_EX aren't members!
        family(CALL, INLINE_CACHE_ENTRIES_CALL) = {
            CALL_BOUND_METHOD_EXACT_ARGS,
            CALL_PY_EXACT_ARGS,
            CALL_TYPE_1,
            CALL_STR_1,
            CALL_TUPLE_1,
            CALL_BUILTIN_CLASS,
            CALL_BUILTIN_O,
            CALL_BUILTIN_FAST,
            CALL_BUILTIN_FAST_WITH_KEYWORDS,
            CALL_LEN,
            CALL_ISINSTANCE,
            CALL_LIST_APPEND,
            CALL_METHOD_DESCRIPTOR_O,
            CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS,
            CALL_METHOD_DESCRIPTOR_NOARGS,
            CALL_METHOD_DESCRIPTOR_FAST,
            CALL_ALLOC_AND_ENTER_INIT,
            CALL_PY_GENERAL,
            CALL_BOUND_METHOD_GENERAL,
            CALL_NON_PY_GENERAL,
        };

        specializing op(_SPECIALIZE_CALL, (counter/1, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_Call(callable, next_instr, oparg + !PyStackRef_IsNull(self_or_null));
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CALL);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        op(_MAYBE_EXPAND_METHOD, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            if (PyStackRef_TYPE(callable) == &PyMethod_Type && PyStackRef_IsNull(self_or_null)) {
                PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
                PyObject *self = ((PyMethodObject *)callable_o)->im_self;
                self_or_null = PyStackRef_FromPyObjectNew(self);
                PyObject *method = ((PyMethodObject *)callable_o)->im_func;
                _PyStackRef temp = callable;
                callable = PyStackRef_FromPyObjectNew(method);
                PyStackRef_CLOSE(temp);
            }
        }

        // When calling Python, inline the call using DISPATCH_INLINED().
        op(_DO_CALL, (callable, self_or_null, args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            // Check if the call can be inlined or not
            if (Py_TYPE(callable_o) == &PyFunction_Type &&
                tstate->interp->eval_frame == NULL &&
                ((PyFunctionObject *)callable_o)->vectorcall == _PyFunction_Vectorcall)
            {
                int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(callable_o))->co_flags;
                PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(callable_o));
                _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit(
                    tstate, callable, locals,
                    arguments, total_args, NULL, frame
                );
                DEAD(args);
                DEAD(self_or_null);
                DEAD(callable);
                // Manipulate stack directly since we leave using DISPATCH_INLINED().
                SYNC_SP();
                // The frame has stolen all the arguments from the stack,
                // so there is no need to clean them up.
                if (new_frame == NULL) {
                    ERROR_NO_POP();
                }
                frame->return_offset = INSTRUCTION_SIZE;
                DISPATCH_INLINED(new_frame);
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *res_o = PyObject_Vectorcall(
                callable_o, args_o,
                total_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            if (opcode == INSTRUMENTED_CALL) {
                PyObject *arg = total_args == 0 ?
                    &_PyInstrumentation_MISSING : PyStackRef_AsPyObjectBorrow(arguments[0]);
                if (res_o == NULL) {
                    _Py_call_instrumentation_exc2(
                        tstate, PY_MONITORING_EVENT_C_RAISE,
                        frame, this_instr, callable_o, arg);
                }
                else {
                    int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_C_RETURN,
                        frame, this_instr, callable_o, arg);
                    if (err < 0) {
                        Py_CLEAR(res_o);
                    }
                }
            }
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_MONITOR_CALL, (func, maybe_self, args[oparg] -- func, maybe_self, args[oparg])) {
            int is_meth = !PyStackRef_IsNull(maybe_self);
            PyObject *function = PyStackRef_AsPyObjectBorrow(func);
            PyObject *arg0;
            if (is_meth) {
                arg0 = PyStackRef_AsPyObjectBorrow(maybe_self);
            }
            else if (oparg) {
                arg0 = PyStackRef_AsPyObjectBorrow(args[0]);
            }
            else {
                arg0 = &_PyInstrumentation_MISSING;
            }
            SYNC_SP();
            int err = _Py_call_instrumentation_2args(
                tstate, PY_MONITORING_EVENT_CALL,
                frame, this_instr, function, arg0
            );
            ERROR_IF(err);
        }

        macro(CALL) = _SPECIALIZE_CALL + unused/2 + _MAYBE_EXPAND_METHOD + _DO_CALL + _CHECK_PERIODIC;
        macro(INSTRUMENTED_CALL) = unused/3 + _MAYBE_EXPAND_METHOD + _MONITOR_CALL + _DO_CALL + _CHECK_PERIODIC;

        op(_PY_FRAME_GENERAL, (callable, self_or_null, args[oparg] -- new_frame)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null)) {
                args--;
                total_args++;
            }
            assert(Py_TYPE(callable_o) == &PyFunction_Type);
            int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(callable_o))->co_flags;
            PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(callable_o));
            _PyInterpreterFrame *temp = _PyEvalFramePushAndInit(
                tstate, callable, locals,
                args, total_args, NULL, frame
            );
            // The frame has stolen all the arguments from the stack.
            INPUTS_DEAD();
            SYNC_SP();
            if (temp == NULL) {
                ERROR_NO_POP();
            }
            new_frame = PyStackRef_Wrap(temp);
        }

        op(_CHECK_FUNCTION_VERSION, (func_version/2, callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            EXIT_IF(!PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            EXIT_IF(func->func_version != func_version);
        }

        tier2 op(_CHECK_FUNCTION_VERSION_INLINE, (func_version/2, callable_o/4 --)) {
            assert(PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            EXIT_IF(func->func_version != func_version);
        }

        macro(CALL_PY_GENERAL) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION +
            _CHECK_RECURSION_REMAINING +
            _PY_FRAME_GENERAL +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        op(_CHECK_METHOD_VERSION, (func_version/2, callable, null, unused[oparg] -- callable, null, unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
            PyObject *func = ((PyMethodObject *)callable_o)->im_func;
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
            EXIT_IF(!PyStackRef_IsNull(null));
        }

        op(_EXPAND_METHOD, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            assert(PyStackRef_IsNull(self_or_null));
            assert(Py_TYPE(callable_o) == &PyMethod_Type);
            self_or_null = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_self);
            _PyStackRef temp = callable;
            callable = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_func);
            assert(PyStackRef_FunctionCheck(callable));
            PyStackRef_CLOSE(temp);
        }

        macro(CALL_BOUND_METHOD_GENERAL) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_METHOD_VERSION +
            _EXPAND_METHOD +
            flush + // so that self is in the argument array
            _CHECK_RECURSION_REMAINING +
            _PY_FRAME_GENERAL +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        op(_CHECK_IS_NOT_PY_CALLABLE, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            EXIT_IF(PyFunction_Check(callable_o));
            EXIT_IF(Py_TYPE(callable_o) == &PyMethod_Type);
        }

        op(_CALL_NON_PY_GENERAL, (callable, self_or_null, args[oparg] -- res)) {
#if TIER_ONE
            assert(opcode != INSTRUMENTED_CALL);
#endif
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *res_o = PyObject_Vectorcall(
                callable_o, args_o,
                total_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_NON_PY_GENERAL) =
            unused/1 + // Skip over the counter
            unused/2 +
            _CHECK_IS_NOT_PY_CALLABLE +
            _CALL_NON_PY_GENERAL +
            _CHECK_PERIODIC;

        op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null, unused[oparg] -- callable, null, unused[oparg])) {
            EXIT_IF(!PyStackRef_IsNull(null));
            EXIT_IF(Py_TYPE(PyStackRef_AsPyObjectBorrow(callable)) != &PyMethod_Type);
        }

        op(_INIT_CALL_BOUND_METHOD_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            assert(PyStackRef_IsNull(self_or_null));
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            STAT_INC(CALL, hit);
            self_or_null = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_self);
            _PyStackRef temp = callable;
            callable = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_func);
            PyStackRef_CLOSE(temp);
        }

        op(_CHECK_PEP_523, (--)) {
            DEOPT_IF(tstate->interp->eval_frame);
        }

        op(_CHECK_FUNCTION_EXACT_ARGS, (callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            assert(PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            EXIT_IF(code->co_argcount != oparg + (!PyStackRef_IsNull(self_or_null)));
        }

        op(_CHECK_STACK_SPACE, (callable, unused, unused[oparg] -- callable, unused, unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
        }

        op(_CHECK_RECURSION_REMAINING, (--)) {
            DEOPT_IF(tstate->py_recursion_remaining <= 1);
        }

        replicate(5) pure op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null, args[oparg] -- new_frame)) {
            int has_self = !PyStackRef_IsNull(self_or_null);
            STAT_INC(CALL, hit);
            _PyInterpreterFrame *pushed_frame = _PyFrame_PushUnchecked(tstate, callable, oparg + has_self, frame);
            _PyStackRef *first_non_self_local = pushed_frame->localsplus + has_self;
            pushed_frame->localsplus[0] = self_or_null;
            for (int i = 0; i < oparg; i++) {
                first_non_self_local[i] = args[i];
            }
            INPUTS_DEAD();
            new_frame = PyStackRef_Wrap(pushed_frame);
        }

        op(_PUSH_FRAME, (new_frame -- )) {
            assert(tstate->interp->eval_frame == NULL);
            _PyInterpreterFrame *temp = PyStackRef_Unwrap(new_frame);
            DEAD(new_frame);
            SYNC_SP();
            _PyFrame_SetStackPointer(frame, stack_pointer);
            assert(temp->previous == frame || temp->previous->previous == frame);
            CALL_STAT_INC(inlined_py_calls);
            frame = tstate->current_frame = temp;
            tstate->py_recursion_remaining--;
            LOAD_SP();
            LOAD_IP(0);
            LLTRACE_RESUME_FRAME();
        }

        macro(CALL_BOUND_METHOD_EXACT_ARGS) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_CALL_BOUND_METHOD_EXACT_ARGS +
            _INIT_CALL_BOUND_METHOD_EXACT_ARGS +
            flush + // In case the following deopt
            _CHECK_FUNCTION_VERSION +
            _CHECK_FUNCTION_EXACT_ARGS +
            _CHECK_STACK_SPACE +
            _CHECK_RECURSION_REMAINING +
            _INIT_CALL_PY_EXACT_ARGS +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        macro(CALL_PY_EXACT_ARGS) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION +
            _CHECK_FUNCTION_EXACT_ARGS +
            _CHECK_STACK_SPACE +
            _CHECK_RECURSION_REMAINING +
            _INIT_CALL_PY_EXACT_ARGS +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        op(_GUARD_NOS_NULL, (null, unused -- null, unused)) {
            DEOPT_IF(!PyStackRef_IsNull(null));
        }

        op(_GUARD_NOS_NOT_NULL, (nos, unused -- nos, unused)) {
            PyObject *o = PyStackRef_AsPyObjectBorrow(nos);
            EXIT_IF(o == NULL);
        }

        op(_GUARD_THIRD_NULL, (null, unused, unused -- null, unused, unused)) {
            DEOPT_IF(!PyStackRef_IsNull(null));
        }

        op(_GUARD_CALLABLE_TYPE_1, (callable, unused, unused -- callable, unused, unused)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            DEOPT_IF(callable_o != (PyObject *)&PyType_Type);
        }

        op(_CALL_TYPE_1, (callable, null, arg -- res)) {
            PyObject *arg_o = PyStackRef_AsPyObjectBorrow(arg);

            assert(oparg == 1);
            DEAD(null);
            DEAD(callable);
            (void)callable; // Silence compiler warnings about unused variables
            (void)null;
            STAT_INC(CALL, hit);
            res = PyStackRef_FromPyObjectNew(Py_TYPE(arg_o));
            PyStackRef_CLOSE(arg);
        }

        macro(CALL_TYPE_1) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_TYPE_1 +
            _CALL_TYPE_1;

        op(_GUARD_CALLABLE_STR_1, (callable, unused, unused -- callable, unused, unused)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            DEOPT_IF(callable_o != (PyObject *)&PyUnicode_Type);
        }

        op(_CALL_STR_1, (callable, null, arg -- res)) {
            PyObject *arg_o = PyStackRef_AsPyObjectBorrow(arg);

            assert(oparg == 1);
            STAT_INC(CALL, hit);
            PyObject *res_o = PyObject_Str(arg_o);
            DEAD(null);
            DEAD(callable);
            (void)callable; // Silence compiler warnings about unused variables
            (void)null;
            PyStackRef_CLOSE(arg);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_STR_1) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_STR_1 +
            _CALL_STR_1 +
            _CHECK_PERIODIC;

        op(_GUARD_CALLABLE_TUPLE_1, (callable, unused, unused -- callable, unused, unused)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            DEOPT_IF(callable_o != (PyObject *)&PyTuple_Type);
        }

        op(_CALL_TUPLE_1, (callable, null, arg -- res)) {
            PyObject *arg_o = PyStackRef_AsPyObjectBorrow(arg);

            assert(oparg == 1);
            STAT_INC(CALL, hit);
            PyObject *res_o = PySequence_Tuple(arg_o);
            DEAD(null);
            DEAD(callable);
            (void)callable; // Silence compiler warnings about unused variables
            (void)null;
            PyStackRef_CLOSE(arg);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_TUPLE_1) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_TUPLE_1 +
            _CALL_TUPLE_1 +
            _CHECK_PERIODIC;

        op(_CHECK_AND_ALLOCATE_OBJECT, (type_version/2, callable, self_or_null, unused[oparg] -- callable, self_or_null, unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            DEOPT_IF(!PyStackRef_IsNull(self_or_null));
            DEOPT_IF(!PyType_Check(callable_o));
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            DEOPT_IF(FT_ATOMIC_LOAD_UINT32_RELAXED(tp->tp_version_tag) != type_version);
            assert(tp->tp_new == PyBaseObject_Type.tp_new);
            assert(tp->tp_flags & Py_TPFLAGS_HEAPTYPE);
            assert(tp->tp_alloc == PyType_GenericAlloc);
            PyHeapTypeObject *cls = (PyHeapTypeObject *)callable_o;
            PyFunctionObject *init_func = (PyFunctionObject *)FT_ATOMIC_LOAD_PTR_ACQUIRE(cls->_spec_cache.init);
            PyCodeObject *code = (PyCodeObject *)init_func->func_code;
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize + _Py_InitCleanup.co_framesize));
            STAT_INC(CALL, hit);
            PyObject *self_o = PyType_GenericAlloc(tp, 0);
            if (self_o == NULL) {
                ERROR_NO_POP();
            }
            self_or_null = PyStackRef_FromPyObjectSteal(self_o);
            _PyStackRef temp = callable;
            callable = PyStackRef_FromPyObjectNew(init_func);
            PyStackRef_CLOSE(temp);
        }

        op(_CREATE_INIT_FRAME, (init, self, args[oparg] -- init_frame)) {
            _PyInterpreterFrame *shim = _PyFrame_PushTrampolineUnchecked(
                tstate, (PyCodeObject *)&_Py_InitCleanup, 1, frame);
            assert(_PyFrame_GetBytecode(shim)[0].op.code == EXIT_INIT_CHECK);
            assert(_PyFrame_GetBytecode(shim)[1].op.code == RETURN_VALUE);
            /* Push self onto stack of shim */
            shim->localsplus[0] = PyStackRef_DUP(self);
            _PyInterpreterFrame *temp = _PyEvalFramePushAndInit(
                tstate, init, NULL, args-1, oparg+1, NULL, shim);
            DEAD(init);
            DEAD(self);
            DEAD(args);
            SYNC_SP();
            if (temp == NULL) {
                _PyEval_FrameClearAndPop(tstate, shim);
                ERROR_NO_POP();
            }
            frame->return_offset = 1 + INLINE_CACHE_ENTRIES_CALL;
            /* Account for pushing the extra frame.
             * We don't check recursion depth here,
             * as it will be checked after start_frame */
            tstate->py_recursion_remaining--;
            init_frame = PyStackRef_Wrap(temp);
        }

        macro(CALL_ALLOC_AND_ENTER_INIT) =
            unused/1 +
            _CHECK_PEP_523 +
            _CHECK_AND_ALLOCATE_OBJECT +
            _CREATE_INIT_FRAME +
            _PUSH_FRAME;

        inst(EXIT_INIT_CHECK, (should_be_none -- )) {
            if (!PyStackRef_IsNone(should_be_none)) {
                PyErr_Format(PyExc_TypeError,
                    "__init__() should return None, not '%.200s'",
                    Py_TYPE(PyStackRef_AsPyObjectBorrow(should_be_none))->tp_name);
                ERROR_NO_POP();
            }
            DEAD(should_be_none);
        }

        op(_CALL_BUILTIN_CLASS, (callable, self_or_null, args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            DEOPT_IF(!PyType_Check(callable_o));
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            DEOPT_IF(tp->tp_vectorcall == NULL);
            STAT_INC(CALL, hit);
            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *res_o = tp->tp_vectorcall((PyObject *)tp, args_o, total_args, NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_CLASS) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_CLASS +
            _CHECK_PERIODIC;

        op(_CALL_BUILTIN_O, (callable, self_or_null, args[oparg] -- res)) {
            /* Builtin METH_O functions */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null)) {
                args--;
                total_args++;
            }
            EXIT_IF(total_args != 1);
            EXIT_IF(!PyCFunction_CheckExact(callable_o));
            EXIT_IF(PyCFunction_GET_FLAGS(callable_o) != METH_O);
            // CPython promises to check all non-vectorcall function calls.
            EXIT_IF(_Py_ReachedRecursionLimit(tstate));
            STAT_INC(CALL, hit);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable_o);
            _PyStackRef arg = args[0];
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, PyCFunction_GET_SELF(callable_o), PyStackRef_AsPyObjectBorrow(arg));
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            PyStackRef_CLOSE(arg);
            DEAD(args);
            DEAD(self_or_null);
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_O) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_O +
            _CHECK_PERIODIC;

        op(_CALL_BUILTIN_FAST, (callable, self_or_null, args[oparg] -- res)) {
            /* Builtin METH_FASTCALL functions, without keywords */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            DEOPT_IF(!PyCFunction_CheckExact(callable_o));
            DEOPT_IF(PyCFunction_GET_FLAGS(callable_o) != METH_FASTCALL);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable_o);
            /* res = func(self, args, nargs) */
            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *res_o = _PyCFunctionFast_CAST(cfunc)(
                PyCFunction_GET_SELF(callable_o),
                args_o,
                total_args);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_FAST) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_FAST +
            _CHECK_PERIODIC;

        op(_CALL_BUILTIN_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- res)) {
            /* Builtin METH_FASTCALL | METH_KEYWORDS functions */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            DEOPT_IF(!PyCFunction_CheckExact(callable_o));
            DEOPT_IF(PyCFunction_GET_FLAGS(callable_o) != (METH_FASTCALL | METH_KEYWORDS));
            STAT_INC(CALL, hit);
            /* res = func(self, arguments, nargs, kwnames) */
            PyCFunctionFastWithKeywords cfunc =
                _PyCFunctionFastWithKeywords_CAST(PyCFunction_GET_FUNCTION(callable_o));

            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *res_o = cfunc(PyCFunction_GET_SELF(callable_o), args_o, total_args, NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_FAST_WITH_KEYWORDS) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_FAST_WITH_KEYWORDS +
            _CHECK_PERIODIC;

        macro(CALL_LEN) =
            unused/1 +
            unused/2 +
            _GUARD_NOS_NULL +
            _GUARD_CALLABLE_LEN +
            _CALL_LEN;

        op(_GUARD_CALLABLE_LEN, (callable, unused, unused -- callable, unused, unused)){
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyInterpreterState *interp = tstate->interp;
            DEOPT_IF(callable_o != interp->callable_cache.len);
        }

        op(_CALL_LEN, (callable, null, arg -- res)) {
            /* len(o) */
            (void)null;
            STAT_INC(CALL, hit);
            PyObject *arg_o = PyStackRef_AsPyObjectBorrow(arg);
            Py_ssize_t len_i = PyObject_Length(arg_o);
            if (len_i < 0) {
                ERROR_NO_POP();
            }
            PyObject *res_o = PyLong_FromSsize_t(len_i);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            PyStackRef_CLOSE(arg);
            DEAD(null);
            PyStackRef_CLOSE(callable);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_GUARD_CALLABLE_ISINSTANCE, (callable, unused, unused, unused -- callable, unused, unused, unused)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyInterpreterState *interp = tstate->interp;
            DEOPT_IF(callable_o != interp->callable_cache.isinstance);
        }

        op(_CALL_ISINSTANCE, (callable, null, instance, cls -- res)) {
            /* isinstance(o, o2) */
            STAT_INC(CALL, hit);
            PyObject *inst_o = PyStackRef_AsPyObjectBorrow(instance);
            PyObject *cls_o = PyStackRef_AsPyObjectBorrow(cls);
            int retval = PyObject_IsInstance(inst_o, cls_o);
            if (retval < 0) {
                ERROR_NO_POP();
            }
            (void)null; // Silence compiler warnings about unused variables
            PyStackRef_CLOSE(cls);
            PyStackRef_CLOSE(instance);
            DEAD(null);
            PyStackRef_CLOSE(callable);
            res = retval ? PyStackRef_True : PyStackRef_False;
            assert((!PyStackRef_IsNull(res)) ^ (_PyErr_Occurred(tstate) != NULL));
        }

        macro(CALL_ISINSTANCE) =
            unused/1 +
            unused/2 +
            _GUARD_THIRD_NULL +
            _GUARD_CALLABLE_ISINSTANCE +
            _CALL_ISINSTANCE;

        macro(CALL_LIST_APPEND) =
            unused/1 +
            unused/2 +
            _GUARD_CALLABLE_LIST_APPEND +
            _GUARD_NOS_NOT_NULL +
            _GUARD_NOS_LIST +
            _CALL_LIST_APPEND;

        op(_GUARD_CALLABLE_LIST_APPEND, (callable, unused, unused -- callable, unused, unused)){
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyInterpreterState *interp = tstate->interp;
            DEOPT_IF(callable_o != interp->callable_cache.list_append);
        }

        // This is secretly a super-instruction
        op(_CALL_LIST_APPEND, (callable, self, arg -- )) {
            assert(oparg == 1);
            PyObject *self_o = PyStackRef_AsPyObjectBorrow(self);

            DEOPT_IF(!PyList_CheckExact(self_o));
            DEOPT_IF(!LOCK_OBJECT(self_o));
            STAT_INC(CALL, hit);
            int err = _PyList_AppendTakeRef((PyListObject *)self_o, PyStackRef_AsPyObjectSteal(arg));
            UNLOCK_OBJECT(self_o);
            PyStackRef_CLOSE(self);
            PyStackRef_CLOSE(callable);
            ERROR_IF(err);
        #if TIER_ONE
            // Skip the following POP_TOP. This is done here in tier one, and
            // during trace projection in tier two:
            assert(next_instr->op.code == POP_TOP);
            SKIP_OVER(1);
        #endif
        }

         op(_CALL_METHOD_DESCRIPTOR_O, (callable, self_or_null, args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }

            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(total_args != 2);
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            PyMethodDef *meth = method->d_method;
            EXIT_IF(meth->ml_flags != METH_O);
            // CPython promises to check all non-vectorcall function calls.
            EXIT_IF(_Py_ReachedRecursionLimit(tstate));
            _PyStackRef arg_stackref = arguments[1];
            _PyStackRef self_stackref = arguments[0];
            EXIT_IF(!Py_IS_TYPE(PyStackRef_AsPyObjectBorrow(self_stackref),
                                 method->d_common.d_type));
            STAT_INC(CALL, hit);
            PyCFunction cfunc = meth->ml_meth;
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc,
                                  PyStackRef_AsPyObjectBorrow(self_stackref),
                                  PyStackRef_AsPyObjectBorrow(arg_stackref));
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_O) =
            unused/1 +
            unused/2 +
            _CALL_METHOD_DESCRIPTOR_O +
            _CHECK_PERIODIC;

        op(_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, (callable, self_or_null, args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            EXIT_IF(total_args == 0);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            PyMethodDef *meth = method->d_method;
            EXIT_IF(meth->ml_flags != (METH_FASTCALL|METH_KEYWORDS));
            PyTypeObject *d_type = method->d_common.d_type;
            PyObject *self = PyStackRef_AsPyObjectBorrow(arguments[0]);
            assert(self != NULL);
            EXIT_IF(!Py_IS_TYPE(self, d_type));
            STAT_INC(CALL, hit);
            int nargs = total_args - 1;

            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyCFunctionFastWithKeywords cfunc =
                _PyCFunctionFastWithKeywords_CAST(meth->ml_meth);
            PyObject *res_o = cfunc(self, (args_o + 1), nargs, NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS) =
            unused/1 +
            unused/2 +
            _CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS +
            _CHECK_PERIODIC;

        op(_CALL_METHOD_DESCRIPTOR_NOARGS, (callable, self_or_null, args[oparg] -- res)) {
            assert(oparg == 0 || oparg == 1);
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null)) {
                args--;
                total_args++;
            }
            EXIT_IF(total_args != 1);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            PyMethodDef *meth = method->d_method;
            _PyStackRef self_stackref = args[0];
            PyObject *self = PyStackRef_AsPyObjectBorrow(self_stackref);
            EXIT_IF(!Py_IS_TYPE(self, method->d_common.d_type));
            EXIT_IF(meth->ml_flags != METH_NOARGS);
            // CPython promises to check all non-vectorcall function calls.
            EXIT_IF(_Py_ReachedRecursionLimit(tstate));
            STAT_INC(CALL, hit);
            PyCFunction cfunc = meth->ml_meth;
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, self, NULL);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            PyStackRef_CLOSE(self_stackref);
            DEAD(args);
            DEAD(self_or_null);
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_NOARGS) =
            unused/1 +
            unused/2 +
            _CALL_METHOD_DESCRIPTOR_NOARGS +
            _CHECK_PERIODIC;

        op(_CALL_METHOD_DESCRIPTOR_FAST, (callable, self_or_null, args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            EXIT_IF(total_args == 0);
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            /* Builtin METH_FASTCALL methods, without keywords */
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            PyMethodDef *meth = method->d_method;
            EXIT_IF(meth->ml_flags != METH_FASTCALL);
            PyObject *self = PyStackRef_AsPyObjectBorrow(arguments[0]);
            assert(self != NULL);
            EXIT_IF(!Py_IS_TYPE(self, method->d_common.d_type));
            STAT_INC(CALL, hit);
            int nargs = total_args - 1;

            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyCFunctionFast cfunc = _PyCFunctionFast_CAST(meth->ml_meth);
            PyObject *res_o = cfunc(self, (args_o + 1), nargs);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_FAST) =
            unused/1 +
            unused/2 +
            _CALL_METHOD_DESCRIPTOR_FAST +
            _CHECK_PERIODIC;

        // Cache layout: counter/1, func_version/2
        family(CALL_KW, INLINE_CACHE_ENTRIES_CALL_KW) = {
            CALL_KW_BOUND_METHOD,
            CALL_KW_PY,
            CALL_KW_NON_PY,
        };

        op(_MONITOR_CALL_KW, (callable, self_or_null, args[oparg], unused -- callable, self_or_null, args[oparg], unused)) {
            int is_meth = !PyStackRef_IsNull(self_or_null);
            PyObject *arg;
            if (is_meth) {
                arg = PyStackRef_AsPyObjectBorrow(self_or_null);
            }
            else if (args) {
                arg = PyStackRef_AsPyObjectBorrow(args[0]);
            }
            else {
                arg = &_PyInstrumentation_MISSING;
            }
            PyObject *function = PyStackRef_AsPyObjectBorrow(callable);
            int err = _Py_call_instrumentation_2args(
                    tstate, PY_MONITORING_EVENT_CALL,
                    frame, this_instr, function, arg);
            ERROR_IF(err);
        }

        op(_MAYBE_EXPAND_METHOD_KW, (callable, self_or_null, unused[oparg], unused -- callable, self_or_null, unused[oparg], unused)) {
            if (PyStackRef_TYPE(callable) == &PyMethod_Type && PyStackRef_IsNull(self_or_null)) {
                PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
                PyObject *self = ((PyMethodObject *)callable_o)->im_self;
                self_or_null = PyStackRef_FromPyObjectNew(self);
                PyObject *method = ((PyMethodObject *)callable_o)->im_func;
                _PyStackRef temp = callable;
                callable = PyStackRef_FromPyObjectNew(method);
                PyStackRef_CLOSE(temp);
            }
        }

        op(_DO_CALL_KW, (callable, self_or_null, args[oparg], kwnames -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *kwnames_o = PyStackRef_AsPyObjectBorrow(kwnames);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            int positional_args = total_args - (int)PyTuple_GET_SIZE(kwnames_o);
            // Check if the call can be inlined or not
            if (Py_TYPE(callable_o) == &PyFunction_Type &&
                tstate->interp->eval_frame == NULL &&
                ((PyFunctionObject *)callable_o)->vectorcall == _PyFunction_Vectorcall)
            {
                int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(callable_o))->co_flags;
                PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(callable_o));
                _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit(
                    tstate, callable, locals,
                    arguments, positional_args, kwnames_o, frame
                );
                DEAD(args);
                DEAD(self_or_null);
                DEAD(callable);
                PyStackRef_CLOSE(kwnames);
                // Sync stack explicitly since we leave using DISPATCH_INLINED().
                SYNC_SP();
                // The frame has stolen all the arguments from the stack,
                // so there is no need to clean them up.
                if (new_frame == NULL) {
                    ERROR_NO_POP();
                }
                assert(INSTRUCTION_SIZE == 1 + INLINE_CACHE_ENTRIES_CALL_KW);
                frame->return_offset = INSTRUCTION_SIZE;
                DISPATCH_INLINED(new_frame);
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *res_o = PyObject_Vectorcall(
                callable_o, args_o,
                positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                kwnames_o);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            if (opcode == INSTRUMENTED_CALL_KW) {
                PyObject *arg = total_args == 0 ?
                    &_PyInstrumentation_MISSING : PyStackRef_AsPyObjectBorrow(arguments[0]);
                if (res_o == NULL) {
                    _Py_call_instrumentation_exc2(
                        tstate, PY_MONITORING_EVENT_C_RAISE,
                        frame, this_instr, callable_o, arg);
                }
                else {
                    int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_C_RETURN,
                        frame, this_instr, callable_o, arg);
                    if (err < 0) {
                        Py_CLEAR(res_o);
                    }
                }
            }
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_PY_FRAME_KW, (callable, self_or_null, args[oparg], kwnames -- new_frame)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            PyObject *kwnames_o = PyStackRef_AsPyObjectBorrow(kwnames);
            int positional_args = total_args - (int)PyTuple_GET_SIZE(kwnames_o);
            assert(Py_TYPE(callable_o) == &PyFunction_Type);
            int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(callable_o))->co_flags;
            PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(callable_o));
            _PyInterpreterFrame *temp = _PyEvalFramePushAndInit(
                tstate, callable, locals,
                arguments, positional_args, kwnames_o, frame
            );
            PyStackRef_CLOSE(kwnames);
            // The frame has stolen all the arguments from the stack,
            // so there is no need to clean them up.
            DEAD(args);
            DEAD(self_or_null);
            DEAD(callable);
            SYNC_SP();
            ERROR_IF(temp == NULL);
            new_frame = PyStackRef_Wrap(temp);
        }

        op(_CHECK_FUNCTION_VERSION_KW, (func_version/2, callable, unused, unused[oparg], unused -- callable, unused, unused[oparg], unused)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            EXIT_IF(!PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            EXIT_IF(func->func_version != func_version);
        }

        macro(CALL_KW_PY) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION_KW +
            _PY_FRAME_KW +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        op(_CHECK_METHOD_VERSION_KW, (func_version/2, callable, null, unused[oparg], unused -- callable, null, unused[oparg], unused)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
            PyObject *func = ((PyMethodObject *)callable_o)->im_func;
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
            EXIT_IF(!PyStackRef_IsNull(null));
        }

        op(_EXPAND_METHOD_KW, (callable, self_or_null, unused[oparg], unused -- callable, self_or_null, unused[oparg], unused)) {
            assert(PyStackRef_IsNull(self_or_null));
            _PyStackRef callable_s = callable;
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            assert(Py_TYPE(callable_o) == &PyMethod_Type);
            self_or_null = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_self);
            callable = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_func);
            assert(PyStackRef_FunctionCheck(callable));
            PyStackRef_CLOSE(callable_s);
        }

        macro(CALL_KW_BOUND_METHOD) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_METHOD_VERSION_KW +
            _EXPAND_METHOD_KW +
            flush + // so that self is in the argument array
            _PY_FRAME_KW +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        specializing op(_SPECIALIZE_CALL_KW, (counter/1, callable, self_or_null, unused[oparg], unused -- callable, self_or_null, unused[oparg], unused)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_CallKw(callable, next_instr, oparg + !PyStackRef_IsNull(self_or_null));
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CALL_KW);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
        }

        macro(CALL_KW) =
            _SPECIALIZE_CALL_KW +
            unused/2 +
            _MAYBE_EXPAND_METHOD_KW +
            _DO_CALL_KW;

        macro(INSTRUMENTED_CALL_KW) =
            counter/1 +
            unused/2 +
            _MAYBE_EXPAND_METHOD_KW +
            _MONITOR_CALL_KW +
            _DO_CALL_KW;

        op(_CHECK_IS_NOT_PY_CALLABLE_KW, (callable, unused, unused[oparg], unused -- callable, unused, unused[oparg], unused)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            EXIT_IF(PyFunction_Check(callable_o));
            EXIT_IF(Py_TYPE(callable_o) == &PyMethod_Type);
        }


        op(_CALL_KW_NON_PY, (callable, self_or_null, args[oparg], kwnames -- res)) {
#if TIER_ONE
            assert(opcode != INSTRUMENTED_CALL);
#endif
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            _PyStackRef *arguments = args;
            if (!PyStackRef_IsNull(self_or_null)) {
                arguments--;
                total_args++;
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(arguments, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true);
            }
            PyObject *kwnames_o = PyStackRef_AsPyObjectBorrow(kwnames);
            int positional_args = total_args - (int)PyTuple_GET_SIZE(kwnames_o);
            PyObject *res_o = PyObject_Vectorcall(
                callable_o, args_o,
                positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                kwnames_o);
            PyStackRef_CLOSE(kwnames);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_KW_NON_PY) =
            unused/1 + // Skip over the counter
            unused/2 +
            _CHECK_IS_NOT_PY_CALLABLE_KW +
            _CALL_KW_NON_PY +
            _CHECK_PERIODIC;

        op(_MAKE_CALLARGS_A_TUPLE, (func, unused, callargs, kwargs -- func, unused, callargs, kwargs)) {
            PyObject *callargs_o = PyStackRef_AsPyObjectBorrow(callargs);
            if (!PyTuple_CheckExact(callargs_o)) {
                int err = _Py_Check_ArgsIterable(tstate, PyStackRef_AsPyObjectBorrow(func), callargs_o);
                if (err < 0) {
                    ERROR_NO_POP();
                }
                PyObject *tuple_o = PySequence_Tuple(callargs_o);
                if (tuple_o == NULL) {
                    ERROR_NO_POP();
                }
                _PyStackRef temp = callargs;
                callargs = PyStackRef_FromPyObjectSteal(tuple_o);
                PyStackRef_CLOSE(temp);
            }
        }

        op(_DO_CALL_FUNCTION_EX, (func_st, null, callargs_st, kwargs_st -- result)) {
            (void)null;
            PyObject *func = PyStackRef_AsPyObjectBorrow(func_st);

            // DICT_MERGE is called before this opcode if there are kwargs.
            // It converts all dict subtypes in kwargs into regular dicts.
            EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_FUNCTION_EX, func);
            PyObject *result_o;
            assert(!_PyErr_Occurred(tstate));
            if (opcode == INSTRUMENTED_CALL_FUNCTION_EX) {
                PyObject *callargs = PyStackRef_AsPyObjectBorrow(callargs_st);
                PyObject *kwargs = PyStackRef_AsPyObjectBorrow(kwargs_st);
                assert(kwargs == NULL || PyDict_CheckExact(kwargs));
                assert(PyTuple_CheckExact(callargs));
                PyObject *arg = PyTuple_GET_SIZE(callargs) > 0 ?
                    PyTuple_GET_ITEM(callargs, 0) : &_PyInstrumentation_MISSING;
                int err = _Py_call_instrumentation_2args(
                    tstate, PY_MONITORING_EVENT_CALL,
                    frame, this_instr, func, arg);
                if (err) {
                    ERROR_NO_POP();
                }
                result_o = PyObject_Call(func, callargs, kwargs);

                if (!PyFunction_Check(func) && !PyMethod_Check(func)) {
                    if (result_o == NULL) {
                        _Py_call_instrumentation_exc2(
                            tstate, PY_MONITORING_EVENT_C_RAISE,
                            frame, this_instr, func, arg);
                    }
                    else {
                        int err = _Py_call_instrumentation_2args(
                            tstate, PY_MONITORING_EVENT_C_RETURN,
                            frame, this_instr, func, arg);
                        if (err < 0) {
                            Py_CLEAR(result_o);
                        }
                    }
                }
            }
            else {
                if (Py_TYPE(func) == &PyFunction_Type &&
                    tstate->interp->eval_frame == NULL &&
                    ((PyFunctionObject *)func)->vectorcall == _PyFunction_Vectorcall) {
                    PyObject *callargs = PyStackRef_AsPyObjectSteal(callargs_st);
                    assert(PyTuple_CheckExact(callargs));
                    PyObject *kwargs = PyStackRef_IsNull(kwargs_st) ? NULL : PyStackRef_AsPyObjectSteal(kwargs_st);
                    assert(kwargs == NULL || PyDict_CheckExact(kwargs));
                    Py_ssize_t nargs = PyTuple_GET_SIZE(callargs);
                    int code_flags = ((PyCodeObject *)PyFunction_GET_CODE(func))->co_flags;
                    PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(func));

                    _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit_Ex(
                        tstate, func_st, locals,
                        nargs, callargs, kwargs, frame);
                    // Need to sync the stack since we exit with DISPATCH_INLINED.
                    INPUTS_DEAD();
                    SYNC_SP();
                    if (new_frame == NULL) {
                        ERROR_NO_POP();
                    }
                    assert(INSTRUCTION_SIZE == 1);
                    frame->return_offset = 1;
                    DISPATCH_INLINED(new_frame);
                }
                PyObject *callargs = PyStackRef_AsPyObjectBorrow(callargs_st);
                assert(PyTuple_CheckExact(callargs));
                PyObject *kwargs = PyStackRef_AsPyObjectBorrow(kwargs_st);
                assert(kwargs == NULL || PyDict_CheckExact(kwargs));
                result_o = PyObject_Call(func, callargs, kwargs);
            }
            PyStackRef_XCLOSE(kwargs_st);
            PyStackRef_CLOSE(callargs_st);
            DEAD(null);
            PyStackRef_CLOSE(func_st);
            ERROR_IF(result_o == NULL);
            result = PyStackRef_FromPyObjectSteal(result_o);
        }

        macro(CALL_FUNCTION_EX) =
            _MAKE_CALLARGS_A_TUPLE +
            _DO_CALL_FUNCTION_EX +
            _CHECK_PERIODIC;

        macro(INSTRUMENTED_CALL_FUNCTION_EX) =
            _MAKE_CALLARGS_A_TUPLE +
            _DO_CALL_FUNCTION_EX +
            _CHECK_PERIODIC;

        inst(MAKE_FUNCTION, (codeobj_st -- func)) {
            PyObject *codeobj = PyStackRef_AsPyObjectBorrow(codeobj_st);

            PyFunctionObject *func_obj = (PyFunctionObject *)
                PyFunction_New(codeobj, GLOBALS());

            PyStackRef_CLOSE(codeobj_st);
            ERROR_IF(func_obj == NULL);

            _PyFunction_SetVersion(
                func_obj, ((PyCodeObject *)codeobj)->co_version);
            func = PyStackRef_FromPyObjectSteal((PyObject *)func_obj);
        }

        inst(SET_FUNCTION_ATTRIBUTE, (attr_st, func_in -- func_out)) {
            PyObject *func = PyStackRef_AsPyObjectBorrow(func_in);
            PyObject *attr = PyStackRef_AsPyObjectSteal(attr_st);
            func_out = func_in;
            DEAD(func_in);
            assert(PyFunction_Check(func));
            size_t offset = _Py_FunctionAttributeOffsets[oparg];
            assert(offset != 0);
            PyObject **ptr = (PyObject **)(((char *)func) + offset);
            assert(*ptr == NULL);
            *ptr = attr;
        }

        inst(RETURN_GENERATOR, (-- res)) {
            assert(PyStackRef_FunctionCheck(frame->f_funcobj));
            PyFunctionObject *func = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(frame->f_funcobj);
            PyGenObject *gen = (PyGenObject *)_Py_MakeCoro(func);
            ERROR_IF(gen == NULL);
            assert(STACK_LEVEL() == 0);
            SAVE_STACK();
            _PyInterpreterFrame *gen_frame = &gen->gi_iframe;
            frame->instr_ptr++;
            _PyFrame_Copy(frame, gen_frame);
            assert(frame->frame_obj == NULL);
            gen->gi_frame_state = FRAME_CREATED;
            gen_frame->owner = FRAME_OWNED_BY_GENERATOR;
            _Py_LeaveRecursiveCallPy(tstate);
            _PyInterpreterFrame *prev = frame->previous;
            _PyThreadState_PopFrame(tstate, frame);
            frame = tstate->current_frame = prev;
            LOAD_IP(frame->return_offset);
            RELOAD_STACK();
            res = PyStackRef_FromPyObjectStealMortal((PyObject *)gen);
            LLTRACE_RESUME_FRAME();
        }

        inst(BUILD_SLICE, (args[oparg] -- slice)) {
            PyObject *start_o = PyStackRef_AsPyObjectBorrow(args[0]);
            PyObject *stop_o = PyStackRef_AsPyObjectBorrow(args[1]);
            PyObject *step_o = oparg == 3 ? PyStackRef_AsPyObjectBorrow(args[2]) : NULL;
            PyObject *slice_o = PySlice_New(start_o, stop_o, step_o);
            DECREF_INPUTS();
            ERROR_IF(slice_o == NULL);
            slice = PyStackRef_FromPyObjectStealMortal(slice_o);
        }

        inst(CONVERT_VALUE, (value -- result)) {
            conversion_func conv_fn;
            assert(oparg >= FVC_STR && oparg <= FVC_ASCII);
            conv_fn = _PyEval_ConversionFuncs[oparg];
            PyObject *result_o = conv_fn(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
            ERROR_IF(result_o == NULL);
            result = PyStackRef_FromPyObjectSteal(result_o);
        }

        inst(FORMAT_SIMPLE, (value -- res)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            /* If value is a unicode object, then we know the result
             * of format(value) is value itself. */
            if (!PyUnicode_CheckExact(value_o)) {
                PyObject *res_o = PyObject_Format(value_o, NULL);
                PyStackRef_CLOSE(value);
                ERROR_IF(res_o == NULL);
                res = PyStackRef_FromPyObjectSteal(res_o);
            }
            else {
                res = value;
                DEAD(value);
            }
        }

        inst(FORMAT_WITH_SPEC, (value, fmt_spec -- res)) {
            PyObject *res_o = PyObject_Format(PyStackRef_AsPyObjectBorrow(value), PyStackRef_AsPyObjectBorrow(fmt_spec));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure replicate(1:4) inst(COPY, (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
            top = PyStackRef_DUP(bottom);
        }

        specializing op(_SPECIALIZE_BINARY_OP, (counter/1, lhs, rhs -- lhs, rhs)) {
            #if ENABLE_SPECIALIZATION_FT
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_BinaryOp(lhs, rhs, next_instr, oparg, LOCALS_ARRAY);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(BINARY_OP);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION_FT */
            assert(NB_ADD <= oparg);
            assert(oparg <= NB_OPARG_LAST);
        }

        op(_BINARY_OP, (lhs, rhs -- res)) {
            PyObject *lhs_o = PyStackRef_AsPyObjectBorrow(lhs);
            PyObject *rhs_o = PyStackRef_AsPyObjectBorrow(rhs);

            assert(_PyEval_BinaryOps[oparg]);
            PyObject *res_o = _PyEval_BinaryOps[oparg](lhs_o, rhs_o);
            if (res_o == NULL) {
                ERROR_NO_POP();
            }
            res = PyStackRef_FromPyObjectSteal(res_o);
            DECREF_INPUTS();
        }

        macro(BINARY_OP) = _SPECIALIZE_BINARY_OP + unused/4 + _BINARY_OP;

        pure replicate(2:4) inst(SWAP, (bottom, unused[oparg-2], top --
                    bottom, unused[oparg-2], top)) {
            _PyStackRef temp = bottom;
            bottom = top;
            top = temp;
        }

        inst(INSTRUMENTED_LINE, ( -- )) {
            int original_opcode = 0;
            if (tstate->tracing) {
                PyCodeObject *code = _PyFrame_GetCode(frame);
                int index = (int)(this_instr - _PyFrame_GetBytecode(frame));
                original_opcode = code->_co_monitoring->lines->data[index*code->_co_monitoring->lines->bytes_per_entry];
                next_instr = this_instr;
            } else {
                original_opcode = _Py_call_instrumentation_line(
                        tstate, frame, this_instr, prev_instr);
                if (original_opcode < 0) {
                    next_instr = this_instr+1;
                    goto error;
                }
                next_instr = frame->instr_ptr;
                if (next_instr != this_instr) {
                    SYNC_SP();
                    DISPATCH();
                }
            }
            if (_PyOpcode_Caches[original_opcode]) {
                _PyBinaryOpCache *cache = (_PyBinaryOpCache *)(next_instr+1);
                /* Prevent the underlying instruction from specializing
                * and overwriting the instrumentation. */
                PAUSE_ADAPTIVE_COUNTER(cache->counter);
            }
            opcode = original_opcode;
            DISPATCH_GOTO();
        }

        inst(INSTRUMENTED_INSTRUCTION, ( -- )) {
            int next_opcode = _Py_call_instrumentation_instruction(
                tstate, frame, this_instr);
            ERROR_IF(next_opcode < 0);
            next_instr = this_instr;
            if (_PyOpcode_Caches[next_opcode]) {
                PAUSE_ADAPTIVE_COUNTER(next_instr[1].counter);
            }
            assert(next_opcode > 0 && next_opcode < 256);
            opcode = next_opcode;
            DISPATCH_GOTO();
        }

        inst(INSTRUMENTED_JUMP_FORWARD, ( -- )) {
            INSTRUMENTED_JUMP(this_instr, next_instr + oparg, PY_MONITORING_EVENT_JUMP);
        }

        op(_MONITOR_JUMP_BACKWARD, (-- )) {
            INSTRUMENTED_JUMP(this_instr, next_instr - oparg, PY_MONITORING_EVENT_JUMP);
        }

        inst(INSTRUMENTED_NOT_TAKEN, ( -- )) {
            (void)this_instr; // INSTRUMENTED_JUMP requires this_instr
            INSTRUMENTED_JUMP(prev_instr, next_instr, PY_MONITORING_EVENT_BRANCH_LEFT);
        }

        macro(INSTRUMENTED_JUMP_BACKWARD) =
            unused/1 +
            _CHECK_PERIODIC +
            _MONITOR_JUMP_BACKWARD;

        inst(INSTRUMENTED_POP_JUMP_IF_TRUE, (unused/1, cond -- )) {
            assert(PyStackRef_BoolCheck(cond));
            int jump = PyStackRef_IsTrue(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(this_instr[1].cache, jump);
            if (jump) {
                INSTRUMENTED_JUMP(this_instr, next_instr + oparg, PY_MONITORING_EVENT_BRANCH_RIGHT);
            }
        }

        inst(INSTRUMENTED_POP_JUMP_IF_FALSE, (unused/1, cond -- )) {
            assert(PyStackRef_BoolCheck(cond));
            int jump = PyStackRef_IsFalse(cond);
            DEAD(cond);
            RECORD_BRANCH_TAKEN(this_instr[1].cache, jump);
            if (jump) {
                INSTRUMENTED_JUMP(this_instr, next_instr + oparg, PY_MONITORING_EVENT_BRANCH_RIGHT);
            }
        }

        inst(INSTRUMENTED_POP_JUMP_IF_NONE, (unused/1, value -- )) {
            int jump = PyStackRef_IsNone(value);
            RECORD_BRANCH_TAKEN(this_instr[1].cache, jump);
            if (jump) {
                DEAD(value);
                INSTRUMENTED_JUMP(this_instr, next_instr + oparg, PY_MONITORING_EVENT_BRANCH_RIGHT);
            }
            else {
                PyStackRef_CLOSE(value);
            }
        }

        inst(INSTRUMENTED_POP_JUMP_IF_NOT_NONE, (unused/1, value -- )) {
            int jump = !PyStackRef_IsNone(value);
            RECORD_BRANCH_TAKEN(this_instr[1].cache, jump);
            if (jump) {
                PyStackRef_CLOSE(value);
                INSTRUMENTED_JUMP(this_instr, next_instr + oparg, PY_MONITORING_EVENT_BRANCH_RIGHT);
            }
            else {
                DEAD(value);
            }
        }

        tier1 inst(EXTENDED_ARG, ( -- )) {
            assert(oparg);
            opcode = next_instr->op.code;
            oparg = oparg << 8 | next_instr->op.arg;
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }

        tier1 inst(CACHE, (--)) {
            assert(0 && "Executing a cache.");
            Py_FatalError("Executing a cache.");
        }

        tier1 inst(RESERVED, (--)) {
            assert(0 && "Executing RESERVED instruction.");
            Py_FatalError("Executing RESERVED instruction.");
        }

        ///////// Tier-2 only opcodes /////////

        op (_GUARD_IS_TRUE_POP, (flag -- )) {
            int is_true = PyStackRef_IsTrue(flag);
            DEAD(flag);
            SYNC_SP();
            EXIT_IF(!is_true);
        }

        op (_GUARD_IS_FALSE_POP, (flag -- )) {
            int is_false = PyStackRef_IsFalse(flag);
            DEAD(flag);
            SYNC_SP();
            EXIT_IF(!is_false);
        }

        op (_GUARD_IS_NONE_POP, (val -- )) {
            int is_none = PyStackRef_IsNone(val);
            if (!is_none) {
                PyStackRef_CLOSE(val);
                SYNC_SP();
                EXIT_IF(1);
            }
            DEAD(val);
        }

        op (_GUARD_IS_NOT_NONE_POP, (val -- )) {
            int is_none = PyStackRef_IsNone(val);
            PyStackRef_CLOSE(val);
            SYNC_SP();
            EXIT_IF(is_none);
        }

        op(_JUMP_TO_TOP, (--)) {
            JUMP_TO_JUMP_TARGET();
        }

        tier2 op(_SET_IP, (instr_ptr/4 --)) {
            frame->instr_ptr = (_Py_CODEUNIT *)instr_ptr;
        }

        tier2 op(_CHECK_STACK_SPACE_OPERAND, (framesize/2 --)) {
            assert(framesize <= INT_MAX);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, framesize));
            DEOPT_IF(tstate->py_recursion_remaining <= 1);
        }

        op(_SAVE_RETURN_OFFSET, (--)) {
            #if TIER_ONE
            frame->return_offset = (uint16_t)(next_instr - this_instr);
            #endif
            #if TIER_TWO
            frame->return_offset = oparg;
            #endif
        }

        tier2 op(_EXIT_TRACE, (exit_p/4 --)) {
            _PyExitData *exit = (_PyExitData *)exit_p;
            PyCodeObject *code = _PyFrame_GetCode(frame);
            _Py_CODEUNIT *target = _PyFrame_GetBytecode(frame) + exit->target;
        #if defined(Py_DEBUG) && !defined(_Py_JIT)
            OPT_HIST(trace_uop_execution_counter, trace_run_length_hist);
            if (frame->lltrace >= 2) {
                printf("SIDE EXIT: [UOp ");
                _PyUOpPrint(&next_uop[-1]);
                printf(", exit %lu, temp %d, target %d -> %s]\n",
                    exit - current_executor->exits, exit->temperature.value_and_backoff,
                    (int)(target - _PyFrame_GetBytecode(frame)),
                    _PyOpcode_OpName[target->op.code]);
            }
        #endif
            if (exit->executor && !exit->executor->vm_data.valid) {
                exit->temperature = initial_temperature_backoff_counter();
                Py_CLEAR(exit->executor);
            }
            if (exit->executor == NULL) {
                _Py_BackoffCounter temperature = exit->temperature;
                if (!backoff_counter_triggers(temperature)) {
                    exit->temperature = advance_backoff_counter(temperature);
                    SYNC_SP();
                    GOTO_TIER_ONE(target);
                    Py_UNREACHABLE();
                }
                _PyExecutorObject *executor;
                if (target->op.code == ENTER_EXECUTOR) {
                    executor = code->co_executors->executors[target->op.arg];
                    Py_INCREF(executor);
                }
                else {
                    int chain_depth = current_executor->vm_data.chain_depth + 1;
                    int optimized = _PyOptimizer_Optimize(frame, target, &executor, chain_depth);
                    if (optimized <= 0) {
                        exit->temperature = restart_backoff_counter(temperature);
                        SYNC_SP();
                        GOTO_TIER_ONE(optimized < 0 ? NULL : target);
                        Py_UNREACHABLE();
                    }
                    exit->temperature = initial_temperature_backoff_counter();
                }
                exit->executor = executor;
            }
            /* In future we might want to avoid spilling
            * on side exits, so we might not sync the stack
            * here and start the side trace with N cached registers */
            SYNC_SP();
            GOTO_TIER_TWO(exit->executor);
        }

        tier2 op(_CHECK_VALIDITY, (--)) {
            DEOPT_IF(!current_executor->vm_data.valid);
        }

        tier2 pure op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
            value = PyStackRef_FromPyObjectNew(ptr);
        }

        tier2 pure op (_POP_TOP_LOAD_CONST_INLINE, (ptr/4, pop -- value)) {
            PyStackRef_CLOSE(pop);
            value = PyStackRef_FromPyObjectNew(ptr);
        }

        tier2 pure op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
            value = PyStackRef_FromPyObjectBorrow(ptr);
        }

        tier2 op(_POP_CALL, (callable, null --)) {
            (void)null; // Silence compiler warnings about unused variables
            DEAD(null);
            PyStackRef_CLOSE(callable);
        }

        tier2 op(_POP_CALL_ONE, (callable, null, pop --)) {
            PyStackRef_CLOSE(pop);
            (void)null; // Silence compiler warnings about unused variables
            DEAD(null);
            PyStackRef_CLOSE(callable);
        }

        tier2 op(_POP_CALL_TWO, (callable, null, pop1, pop2 --)) {
            PyStackRef_CLOSE(pop2);
            PyStackRef_CLOSE(pop1);
            (void)null; // Silence compiler warnings about unused variables
            DEAD(null);
            PyStackRef_CLOSE(callable);
        }

        tier2 op(_POP_TOP_LOAD_CONST_INLINE_BORROW, (ptr/4, pop -- value)) {
            PyStackRef_CLOSE(pop);
            value = PyStackRef_FromPyObjectBorrow(ptr);
        }

        tier2 op(_POP_TWO_LOAD_CONST_INLINE_BORROW, (ptr/4, pop1, pop2 -- value)) {
            PyStackRef_CLOSE(pop2);
            PyStackRef_CLOSE(pop1);
            value = PyStackRef_FromPyObjectBorrow(ptr);
        }

        tier2 op(_POP_CALL_LOAD_CONST_INLINE_BORROW, (ptr/4, callable, null -- value)) {
            (void)null; // Silence compiler warnings about unused variables
            DEAD(null);
            PyStackRef_CLOSE(callable);
            value = PyStackRef_FromPyObjectBorrow(ptr);
        }

        tier2 op(_POP_CALL_ONE_LOAD_CONST_INLINE_BORROW, (ptr/4, callable, null, pop -- value)) {
            PyStackRef_CLOSE(pop);
            (void)null; // Silence compiler warnings about unused variables
            DEAD(null);
            PyStackRef_CLOSE(callable);
            value = PyStackRef_FromPyObjectBorrow(ptr);
        }

        tier2 op(_POP_CALL_TWO_LOAD_CONST_INLINE_BORROW, (ptr/4, callable, null, pop1, pop2 -- value)) {
            PyStackRef_CLOSE(pop2);
            PyStackRef_CLOSE(pop1);
            (void)null; // Silence compiler warnings about unused variables
            DEAD(null);
            PyStackRef_CLOSE(callable);
            value = PyStackRef_FromPyObjectBorrow(ptr);
        }

        tier2 op(_LOAD_CONST_UNDER_INLINE, (ptr/4, old -- value, new)) {
            new = old;
            DEAD(old);
            value = PyStackRef_FromPyObjectNew(ptr);
        }

        tier2 op(_LOAD_CONST_UNDER_INLINE_BORROW, (ptr/4, old -- value, new)) {
            new = old;
            DEAD(old);
            value = PyStackRef_FromPyObjectBorrow(ptr);
        }

        tier2 op(_CHECK_FUNCTION, (func_version/2 -- )) {
            assert(PyStackRef_FunctionCheck(frame->f_funcobj));
            PyFunctionObject *func = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(frame->f_funcobj);
            DEOPT_IF(func->func_version != func_version);
        }

        tier2 op(_START_EXECUTOR, (executor/4 --)) {
#ifndef _Py_JIT
            current_executor = (_PyExecutorObject*)executor;
#endif
            assert(((_PyExecutorObject *)executor)->vm_data.valid);
        }

        tier2 op(_MAKE_WARM, (--)) {
            current_executor->vm_data.warm = true;
            // It's okay if this ends up going negative.
            if (--tstate->interp->trace_run_counter == 0) {
                _Py_set_eval_breaker_bit(tstate, _PY_EVAL_JIT_INVALIDATE_COLD_BIT);
            }
        }

        tier2 op(_FATAL_ERROR, (--)) {
            assert(0);
            Py_FatalError("Fatal error uop executed.");
        }

        tier2 op(_DEOPT, (--)) {
            SYNC_SP();
            GOTO_TIER_ONE(_PyFrame_GetBytecode(frame) + CURRENT_TARGET());
        }

        tier2 op(_ERROR_POP_N, (target/2 --)) {
            assert(oparg == 0);
            frame->instr_ptr = _PyFrame_GetBytecode(frame) + target;
            SYNC_SP();
            GOTO_TIER_ONE(NULL);
        }

        /* Progress is guaranteed if we DEOPT on the eval breaker, because
         * ENTER_EXECUTOR will not re-enter tier 2 with the eval breaker set. */
        tier2 op(_TIER2_RESUME_CHECK, (--)) {
#if defined(__EMSCRIPTEN__)
            DEOPT_IF(_Py_emscripten_signal_clock == 0);
            _Py_emscripten_signal_clock -= Py_EMSCRIPTEN_SIGNAL_HANDLING;
#endif
            uintptr_t eval_breaker = _Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker);
            DEOPT_IF(eval_breaker & _PY_EVAL_EVENTS_MASK);
            assert(tstate->tracing || eval_breaker == FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(_PyFrame_GetCode(frame)->_co_instrumentation_version));
        }

        tier2 op(_SPILL_OR_RELOAD, (--)) {
        }

        label(pop_2_error) {
            stack_pointer -= 2;
            assert(WITHIN_STACK_BOUNDS());
            goto error;
        }

        label(pop_1_error) {
            stack_pointer -= 1;
            assert(WITHIN_STACK_BOUNDS());
            goto error;
        }

        label(error) {
            /* Double-check exception status. */
#ifdef NDEBUG
            if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_SystemError,
                             "error return without exception set");
        }
#else
            assert(_PyErr_Occurred(tstate));
#endif

            /* Log traceback info. */
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            if (!_PyFrame_IsIncomplete(frame)) {
                PyFrameObject *f = _PyFrame_GetFrameObject(frame);
                if (f != NULL) {
                    PyTraceBack_Here(f);
                }
            }
            _PyEval_MonitorRaise(tstate, frame, next_instr-1);
            goto exception_unwind;
        }

        spilled label(exception_unwind) {
            /* We can't use frame->instr_ptr here, as RERAISE may have set it */
            int offset = INSTR_OFFSET()-1;
            int level, handler, lasti;
            int handled = get_exception_handler(_PyFrame_GetCode(frame), offset, &level, &handler, &lasti);
            if (handled == 0) {
                // No handlers, so exit.
                assert(_PyErr_Occurred(tstate));
                /* Pop remaining stack entries. */
                _PyStackRef *stackbase = _PyFrame_Stackbase(frame);
                while (frame->stackpointer > stackbase) {
                    _PyStackRef ref = _PyFrame_StackPop(frame);
                    PyStackRef_XCLOSE(ref);
                }
                monitor_unwind(tstate, frame, next_instr-1);
                goto exit_unwind;
            }
            assert(STACK_LEVEL() >= level);
            _PyStackRef *new_top = _PyFrame_Stackbase(frame) + level;
            assert(frame->stackpointer >= new_top);
            while (frame->stackpointer > new_top) {
                _PyStackRef ref = _PyFrame_StackPop(frame);
                PyStackRef_XCLOSE(ref);
            }
            if (lasti) {
                int frame_lasti = _PyInterpreterFrame_LASTI(frame);
                _PyStackRef lasti = PyStackRef_TagInt(frame_lasti);
                _PyFrame_StackPush(frame, lasti);
            }

            /* Make the raw exception data
                available to the handler,
                so a program can emulate the
                Python main loop. */
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            _PyFrame_StackPush(frame, PyStackRef_FromPyObjectSteal(exc));
            next_instr = _PyFrame_GetBytecode(frame) + handler;

            int err = monitor_handled(tstate, frame, next_instr, exc);
            if (err < 0) {
                goto exception_unwind;
            }
            /* Resume normal execution */
#ifdef Py_DEBUG
            if (frame->lltrace >= 5) {
                lltrace_resume_frame(frame);
            }
#endif
            RELOAD_STACK();
#if Py_TAIL_CALL_INTERP
            int opcode;
#endif
            DISPATCH();
        }

        spilled label(exit_unwind) {
            assert(_PyErr_Occurred(tstate));
            _Py_LeaveRecursiveCallPy(tstate);
            assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
            // GH-99729: We need to unlink the frame *before* clearing it:
            _PyInterpreterFrame *dying = frame;
            frame = tstate->current_frame = dying->previous;
            _PyEval_FrameClearAndPop(tstate, dying);
            frame->return_offset = 0;
            if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
                /* Restore previous frame and exit */
                tstate->current_frame = frame->previous;
#if !Py_TAIL_CALL_INTERP
                assert(frame == &entry.frame);
#endif
#ifdef _Py_TIER2
                _PyStackRef executor = frame->localsplus[0];
                assert(tstate->current_executor == NULL);
                if (!PyStackRef_IsNull(executor)) {
                    tstate->current_executor = PyStackRef_AsPyObjectBorrow(executor);
                    PyStackRef_CLOSE(executor);
                }
#endif
                return NULL;
            }
            next_instr = frame->instr_ptr;
            RELOAD_STACK();
            goto error;
        }

        spilled label(start_frame) {
            int too_deep = _Py_EnterRecursivePy(tstate);
            if (too_deep) {
                goto exit_unwind;
            }
            next_instr = frame->instr_ptr;
        #ifdef Py_DEBUG
            int lltrace = maybe_lltrace_resume_frame(frame, GLOBALS());
            if (lltrace < 0) {
                JUMP_TO_LABEL(exit_unwind);
            }
            frame->lltrace = lltrace;
            /* _PyEval_EvalFrameDefault() must not be called with an exception set,
            because it can clear it (directly or indirectly) and so the
            caller loses its exception */
            assert(!_PyErr_Occurred(tstate));
        #endif
            RELOAD_STACK();
#if Py_TAIL_CALL_INTERP
            int opcode;
#endif
            DISPATCH();
        }



// END BYTECODES //

    }
 dispatch_opcode:
 error:
 exception_unwind:
 exit_unwind:
 handle_eval_breaker:
 resume_frame:
 start_frame:
 unbound_local_error:
    ;
}

// Future families go below this point //
