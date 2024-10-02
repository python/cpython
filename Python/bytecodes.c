// This file contains instruction definitions.
// It is read by generators stored in Tools/cases_generator/
// to generate Python/generated_cases.c.h and others.
// Note that there is some dummy C code at the top and bottom of the file
// to fool text editors like VS Code into believing this is valid C code.
// The actual instruction definitions start at // BEGIN BYTECODES //.
// See Tools/cases_generator/README.md for more information.

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_backoff.h"
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_ceval.h"
#include "pycore_code.h"
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS
#include "pycore_function.h"
#include "pycore_instruments.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"          // _PyLong_GetZero()
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
#include "pycore_sysmodule.h"     // _PySys_Audit()
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
#define GO_TO_INSTRUCTION(instname) ((void)0)

#define inst(name, ...) case name:
#define op(name, ...) /* NAME is ignored */
#define macro(name) static int MACRO_##name
#define super(name) static int SUPER_##name
#define family(name, ...) static int family_##name
#define pseudo(name) static int pseudo_##name

/* Annotations */
#define guard
#define override
#define specializing
#define split
#define replicate(TIMES)

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
    _PyInterpreterFrame  entry_frame;
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

        op(_CHECK_PERIODIC, (--)) {
            _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
            QSBR_QUIESCENT_STATE(tstate); \
            if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
                int err = _Py_HandlePending(tstate);
                ERROR_IF(err != 0, error);
            }
        }

        op(_CHECK_PERIODIC_IF_NOT_YIELD_FROM, (--)) {
            if ((oparg & RESUME_OPARG_LOCATION_MASK) < RESUME_AFTER_YIELD_FROM) {
                _Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY();
                QSBR_QUIESCENT_STATE(tstate); \
                if (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) & _PY_EVAL_EVENTS_MASK) {
                    int err = _Py_HandlePending(tstate);
                    ERROR_IF(err != 0, error);
                }
            }
        }

        op(_QUICKEN_RESUME, (--)) {
            #if ENABLE_SPECIALIZATION
            if (tstate->tracing == 0 && this_instr->op.code == RESUME) {
                FT_ATOMIC_STORE_UINT8_RELAXED(this_instr->op.code, RESUME_CHECK);
            }
            #endif  /* ENABLE_SPECIALIZATION */
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

        macro(RESUME) =
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
        }

        op(_MONITOR_RESUME, (--)) {
            _PyFrame_SetStackPointer(frame, stack_pointer);
            int err = _Py_call_instrumentation(
                    tstate, oparg > 0, frame, this_instr);
            stack_pointer = _PyFrame_GetStackPointer(frame);
            ERROR_IF(err, error);
            if (frame->instr_ptr != this_instr) {
                /* Instrumentation has jumped */
                next_instr = frame->instr_ptr;
            }
        }

        macro(INSTRUMENTED_RESUME) =
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
                ERROR_IF(1, error);
            }
            value = PyStackRef_DUP(value_s);
        }

        replicate(8) pure inst(LOAD_FAST, (-- value)) {
            assert(!PyStackRef_IsNull(GETLOCAL(oparg)));
            value = PyStackRef_DUP(GETLOCAL(oparg));
        }

        inst(LOAD_FAST_AND_CLEAR, (-- value)) {
            value = GETLOCAL(oparg);
            // do not use SETLOCAL here, it decrefs the old value
            GETLOCAL(oparg) = PyStackRef_NULL;
        }

        inst(LOAD_FAST_LOAD_FAST, ( -- value1, value2)) {
            uint32_t oparg1 = oparg >> 4;
            uint32_t oparg2 = oparg & 15;
            value1 = PyStackRef_DUP(GETLOCAL(oparg1));
            value2 = PyStackRef_DUP(GETLOCAL(oparg2));
        }

        pure inst(LOAD_CONST, (-- value)) {
            value = PyStackRef_FromPyObjectNew(GETITEM(FRAME_CO_CONSTS, oparg));
        }

        replicate(8) inst(STORE_FAST, (value --)) {
            SETLOCAL(oparg, value);
        }

        pseudo(STORE_FAST_MAYBE_NULL, (unused --)) = {
            STORE_FAST,
        };

        inst(STORE_FAST_LOAD_FAST, (value1 -- value2)) {
            uint32_t oparg1 = oparg >> 4;
            uint32_t oparg2 = oparg & 15;
            SETLOCAL(oparg1, value1);
            value2 = PyStackRef_DUP(GETLOCAL(oparg2));
        }

        inst(STORE_FAST_STORE_FAST, (value2, value1 --)) {
            uint32_t oparg1 = oparg >> 4;
            uint32_t oparg2 = oparg & 15;
            SETLOCAL(oparg1, value1);
            SETLOCAL(oparg2, value2);
        }

        pure inst(POP_TOP, (value --)) {
            DECREF_INPUTS();
        }

        pure inst(PUSH_NULL, (-- res)) {
            res = PyStackRef_NULL;
        }

        macro(END_FOR) = POP_TOP;

        tier1 inst(INSTRUMENTED_END_FOR, (receiver, value -- receiver)) {
            /* Need to create a fake StopIteration error here,
             * to conform to PEP 380 */
            if (PyStackRef_GenCheck(receiver)) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, PyStackRef_AsPyObjectBorrow(value));
                if (err) {
                    ERROR_NO_POP();
                }
            }
            DECREF_INPUTS();
        }

        pure inst(END_SEND, (receiver, value -- value)) {
            (void)receiver;
            PyStackRef_CLOSE(receiver);
        }

        tier1 inst(INSTRUMENTED_END_SEND, (receiver, value -- value)) {
            PyObject *receiver_o = PyStackRef_AsPyObjectBorrow(receiver);
            if (PyGen_Check(receiver_o) || PyCoro_CheckExact(receiver_o)) {
                int err = monitor_stop_iteration(tstate, frame, this_instr, PyStackRef_AsPyObjectBorrow(value));
                if (err) {
                    ERROR_NO_POP();
                }
            }
            PyStackRef_CLOSE(receiver);
        }

        inst(UNARY_NEGATIVE, (value -- res)) {
            PyObject *res_o = PyNumber_Negative(PyStackRef_AsPyObjectBorrow(value));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure inst(UNARY_NOT, (value -- res)) {
            assert(PyStackRef_BoolCheck(value));
            res = PyStackRef_Is(value, PyStackRef_False)
                ? PyStackRef_True : PyStackRef_False;
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
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_ToBool(value, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(TO_BOOL);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_TO_BOOL, (value -- res)) {
            int err = PyObject_IsTrue(PyStackRef_AsPyObjectBorrow(value));
            DECREF_INPUTS();
            ERROR_IF(err < 0, error);
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
                assert(_Py_IsImmortalLoose(value_o));
                res = PyStackRef_False;
            }
            else {
                DECREF_INPUTS();
                res = PyStackRef_True;
            }
        }

        inst(TO_BOOL_LIST, (unused/1, unused/2, value -- res)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            EXIT_IF(!PyList_CheckExact(value_o));
            STAT_INC(TO_BOOL, hit);
            res = Py_SIZE(value_o) ? PyStackRef_True : PyStackRef_False;
            DECREF_INPUTS();
        }

        inst(TO_BOOL_NONE, (unused/1, unused/2, value -- res)) {
            // This one is a bit weird, because we expect *some* failures:
            EXIT_IF(!PyStackRef_Is(value, PyStackRef_None));
            STAT_INC(TO_BOOL, hit);
            res = PyStackRef_False;
        }

        inst(TO_BOOL_STR, (unused/1, unused/2, value -- res)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            EXIT_IF(!PyUnicode_CheckExact(value_o));
            STAT_INC(TO_BOOL, hit);
            if (value_o == &_Py_STR(empty)) {
                assert(_Py_IsImmortalLoose(value_o));
                res = PyStackRef_False;
            }
            else {
                assert(Py_SIZE(value_o));
                DECREF_INPUTS();
                res = PyStackRef_True;
            }
        }

        op(_REPLACE_WITH_TRUE, (value -- res)) {
            DECREF_INPUTS();
            res = PyStackRef_True;
        }

        macro(TO_BOOL_ALWAYS_TRUE) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _REPLACE_WITH_TRUE;

        inst(UNARY_INVERT, (value -- res)) {
            PyObject *res_o = PyNumber_Invert(PyStackRef_AsPyObjectBorrow(value));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
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
            // BINARY_OP_INPLACE_ADD_UNICODE,  // See comments at that opcode.
        };

        op(_GUARD_BOTH_INT, (left, right -- left, right)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            EXIT_IF(!PyLong_CheckExact(left_o));
            EXIT_IF(!PyLong_CheckExact(right_o));
        }

        op(_GUARD_NOS_INT, (left, unused -- left, unused)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            EXIT_IF(!PyLong_CheckExact(left_o));
        }

        op(_GUARD_TOS_INT, (value -- value)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            EXIT_IF(!PyLong_CheckExact(value_o));
        }

        pure op(_BINARY_OP_MULTIPLY_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = _PyLong_Multiply((PyLongObject *)left_o, (PyLongObject *)right_o);
            _Py_DECREF_SPECIALIZED(right_o, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left_o, (destructor)PyObject_Free);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure op(_BINARY_OP_ADD_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = _PyLong_Add((PyLongObject *)left_o, (PyLongObject *)right_o);
            _Py_DECREF_SPECIALIZED(right_o, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left_o, (destructor)PyObject_Free);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure op(_BINARY_OP_SUBTRACT_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = _PyLong_Subtract((PyLongObject *)left_o, (PyLongObject *)right_o);
            _Py_DECREF_SPECIALIZED(right_o, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left_o, (destructor)PyObject_Free);;
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_OP_MULTIPLY_INT) =
            _GUARD_BOTH_INT + unused/1 + _BINARY_OP_MULTIPLY_INT;
        macro(BINARY_OP_ADD_INT) =
            _GUARD_BOTH_INT + unused/1 + _BINARY_OP_ADD_INT;
        macro(BINARY_OP_SUBTRACT_INT) =
            _GUARD_BOTH_INT + unused/1 + _BINARY_OP_SUBTRACT_INT;

        op(_GUARD_BOTH_FLOAT, (left, right -- left, right)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);
            EXIT_IF(!PyFloat_CheckExact(left_o));
            EXIT_IF(!PyFloat_CheckExact(right_o));
        }

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

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval *
                ((PyFloatObject *)right_o)->ob_fval;
            PyObject *res_o;
            DECREF_INPUTS_AND_REUSE_FLOAT(left_o, right_o, dres, res_o);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure op(_BINARY_OP_ADD_FLOAT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval +
                ((PyFloatObject *)right_o)->ob_fval;
            PyObject *res_o;
            DECREF_INPUTS_AND_REUSE_FLOAT(left_o, right_o, dres, res_o);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure op(_BINARY_OP_SUBTRACT_FLOAT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(BINARY_OP, hit);
            double dres =
                ((PyFloatObject *)left_o)->ob_fval -
                ((PyFloatObject *)right_o)->ob_fval;
            PyObject *res_o;
            DECREF_INPUTS_AND_REUSE_FLOAT(left_o, right_o, dres, res_o);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_OP_MULTIPLY_FLOAT) =
            _GUARD_BOTH_FLOAT + unused/1 + _BINARY_OP_MULTIPLY_FLOAT;
        macro(BINARY_OP_ADD_FLOAT) =
            _GUARD_BOTH_FLOAT + unused/1 + _BINARY_OP_ADD_FLOAT;
        macro(BINARY_OP_SUBTRACT_FLOAT) =
            _GUARD_BOTH_FLOAT + unused/1 + _BINARY_OP_SUBTRACT_FLOAT;

        op(_GUARD_BOTH_UNICODE, (left, right -- left, right)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            EXIT_IF(!PyUnicode_CheckExact(left_o));
            EXIT_IF(!PyUnicode_CheckExact(right_o));
        }

        pure op(_BINARY_OP_ADD_UNICODE, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(BINARY_OP, hit);
            PyObject *res_o = PyUnicode_Concat(left_o, right_o);
            _Py_DECREF_SPECIALIZED(left_o, _PyUnicode_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right_o, _PyUnicode_ExactDealloc);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_OP_ADD_UNICODE) =
            _GUARD_BOTH_UNICODE + unused/1 + _BINARY_OP_ADD_UNICODE;

        // This is a subtle one. It's a super-instruction for
        // BINARY_OP_ADD_UNICODE followed by STORE_FAST
        // where the store goes into the left argument.
        // So the inputs are the same as for all BINARY_OP
        // specializations, but there is no output.
        // At the end we just skip over the STORE_FAST.
        op(_BINARY_OP_INPLACE_ADD_UNICODE, (left, right --)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            int next_oparg;
        #if TIER_ONE
            assert(next_instr->op.code == STORE_FAST);
            next_oparg = next_instr->op.arg;
        #else
            next_oparg = CURRENT_OPERAND();
        #endif
            _PyStackRef *target_local = &GETLOCAL(next_oparg);
            DEOPT_IF(!PyStackRef_Is(*target_local, left));
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
            assert(Py_REFCNT(left_o) >= 2);
            _Py_DECREF_NO_DEALLOC(left_o);
            PyObject *temp = PyStackRef_AsPyObjectBorrow(*target_local);
            PyUnicode_Append(&temp, right_o);
            *target_local = PyStackRef_FromPyObjectSteal(temp);
            _Py_DECREF_SPECIALIZED(right_o, _PyUnicode_ExactDealloc);
            ERROR_IF(PyStackRef_IsNull(*target_local), error);
        #if TIER_ONE
            // The STORE_FAST is already done. This is done here in tier one,
            // and during trace projection in tier two:
            assert(next_instr->op.code == STORE_FAST);
            SKIP_OVER(1);
        #endif
        }

        macro(BINARY_OP_INPLACE_ADD_UNICODE) =
            _GUARD_BOTH_UNICODE + unused/1 + _BINARY_OP_INPLACE_ADD_UNICODE;

        family(BINARY_SUBSCR, INLINE_CACHE_ENTRIES_BINARY_SUBSCR) = {
            BINARY_SUBSCR_DICT,
            BINARY_SUBSCR_GETITEM,
            BINARY_SUBSCR_LIST_INT,
            BINARY_SUBSCR_STR_INT,
            BINARY_SUBSCR_TUPLE_INT,
        };

        specializing op(_SPECIALIZE_BINARY_SUBSCR, (counter/1, container, sub -- container, sub)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_BinarySubscr(container, sub, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(BINARY_SUBSCR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_BINARY_SUBSCR, (container, sub -- res)) {
            PyObject *container_o = PyStackRef_AsPyObjectBorrow(container);
            PyObject *sub_o = PyStackRef_AsPyObjectBorrow(sub);

            PyObject *res_o = PyObject_GetItem(container_o, sub_o);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_SUBSCR) = _SPECIALIZE_BINARY_SUBSCR + _BINARY_SUBSCR;

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
            ERROR_IF(res_o == NULL, error);
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
            PyStackRef_CLOSE(v);
            PyStackRef_CLOSE(container);
            ERROR_IF(err, error);
        }

        macro(STORE_SLICE) = _SPECIALIZE_STORE_SLICE + _STORE_SLICE;

        inst(BINARY_SUBSCR_LIST_INT, (unused/1, list_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *list = PyStackRef_AsPyObjectBorrow(list_st);

            DEOPT_IF(!PyLong_CheckExact(sub));
            DEOPT_IF(!PyList_CheckExact(list));

            // Deopt unless 0 <= sub < PyList_Size(list)
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(index >= PyList_GET_SIZE(list));
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res_o = PyList_GET_ITEM(list, index);
            assert(res_o != NULL);
            Py_INCREF(res_o);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            PyStackRef_CLOSE(list_st);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(BINARY_SUBSCR_STR_INT, (unused/1, str_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *str = PyStackRef_AsPyObjectBorrow(str_st);

            DEOPT_IF(!PyLong_CheckExact(sub));
            DEOPT_IF(!PyUnicode_CheckExact(str));
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(PyUnicode_GET_LENGTH(str) <= index);
            // Specialize for reading an ASCII character from any string:
            Py_UCS4 c = PyUnicode_READ_CHAR(str, index);
            DEOPT_IF(Py_ARRAY_LENGTH(_Py_SINGLETON(strings).ascii) <= c);
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res_o = (PyObject*)&_Py_SINGLETON(strings).ascii[c];
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            PyStackRef_CLOSE(str_st);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(BINARY_SUBSCR_TUPLE_INT, (unused/1, tuple_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *tuple = PyStackRef_AsPyObjectBorrow(tuple_st);

            DEOPT_IF(!PyLong_CheckExact(sub));
            DEOPT_IF(!PyTuple_CheckExact(tuple));

            // Deopt unless 0 <= sub < PyTuple_Size(list)
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(index >= PyTuple_GET_SIZE(tuple));
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res_o = PyTuple_GET_ITEM(tuple, index);
            assert(res_o != NULL);
            Py_INCREF(res_o);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            PyStackRef_CLOSE(tuple_st);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(BINARY_SUBSCR_DICT, (unused/1, dict_st, sub_st -- res)) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *dict = PyStackRef_AsPyObjectBorrow(dict_st);

            DEOPT_IF(!PyDict_CheckExact(dict));
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res_o;
            int rc = PyDict_GetItemRef(dict, sub, &res_o);
            if (rc == 0) {
                _PyErr_SetKeyError(sub);
            }
            DECREF_INPUTS();
            ERROR_IF(rc <= 0, error); // not found or error
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_BINARY_SUBSCR_CHECK_FUNC, (container, unused -- container, unused)) {
            PyTypeObject *tp = Py_TYPE(PyStackRef_AsPyObjectBorrow(container));
            DEOPT_IF(!PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE));
            PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
            PyObject *getitem = ht->_spec_cache.getitem;
            DEOPT_IF(getitem == NULL);
            assert(PyFunction_Check(getitem));
            uint32_t cached_version = ht->_spec_cache.getitem_version;
            DEOPT_IF(((PyFunctionObject *)getitem)->func_version != cached_version);
            PyCodeObject *code = (PyCodeObject *)PyFunction_GET_CODE(getitem);
            assert(code->co_argcount == 2);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            STAT_INC(BINARY_SUBSCR, hit);
        }

        op(_BINARY_SUBSCR_INIT_CALL, (container, sub -- new_frame: _PyInterpreterFrame* )) {
            PyTypeObject *tp = Py_TYPE(PyStackRef_AsPyObjectBorrow(container));
            PyHeapTypeObject *ht = (PyHeapTypeObject *)tp;
            PyObject *getitem = ht->_spec_cache.getitem;
            new_frame = _PyFrame_PushUnchecked(tstate, PyStackRef_FromPyObjectNew(getitem), 2, frame);
            SYNC_SP();
            new_frame->localsplus[0] = container;
            new_frame->localsplus[1] = sub;
            frame->return_offset = (uint16_t)(1 + INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
        }

        macro(BINARY_SUBSCR_GETITEM) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _BINARY_SUBSCR_CHECK_FUNC +
            _BINARY_SUBSCR_INIT_CALL +
            _PUSH_FRAME;

        inst(LIST_APPEND, (list, unused[oparg-1], v -- list, unused[oparg-1])) {
            ERROR_IF(_PyList_AppendTakeRef((PyListObject *)PyStackRef_AsPyObjectBorrow(list),
                                           PyStackRef_AsPyObjectSteal(v)) < 0, error);
        }

        inst(SET_ADD, (set, unused[oparg-1], v -- set, unused[oparg-1])) {
            int err = PySet_Add(PyStackRef_AsPyObjectBorrow(set),
                                PyStackRef_AsPyObjectBorrow(v));
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        family(STORE_SUBSCR, INLINE_CACHE_ENTRIES_STORE_SUBSCR) = {
            STORE_SUBSCR_DICT,
            STORE_SUBSCR_LIST_INT,
        };

        specializing op(_SPECIALIZE_STORE_SUBSCR, (counter/1, container, sub -- container, sub)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_StoreSubscr(container, sub, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(STORE_SUBSCR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_STORE_SUBSCR, (v, container, sub -- )) {
            /* container[sub] = v */
            int err = PyObject_SetItem(PyStackRef_AsPyObjectBorrow(container), PyStackRef_AsPyObjectBorrow(sub), PyStackRef_AsPyObjectBorrow(v));
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        macro(STORE_SUBSCR) = _SPECIALIZE_STORE_SUBSCR + _STORE_SUBSCR;

        inst(STORE_SUBSCR_LIST_INT, (unused/1, value, list_st, sub_st -- )) {
            PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);
            PyObject *list = PyStackRef_AsPyObjectBorrow(list_st);

            DEOPT_IF(!PyLong_CheckExact(sub));
            DEOPT_IF(!PyList_CheckExact(list));

            // Ensure nonnegative, zero-or-one-digit ints.
            DEOPT_IF(!_PyLong_IsNonNegativeCompact((PyLongObject *)sub));
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            // Ensure index < len(list)
            DEOPT_IF(index >= PyList_GET_SIZE(list));
            STAT_INC(STORE_SUBSCR, hit);

            PyObject *old_value = PyList_GET_ITEM(list, index);
            PyList_SET_ITEM(list, index, PyStackRef_AsPyObjectSteal(value));
            assert(old_value != NULL);
            Py_DECREF(old_value);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            PyStackRef_CLOSE(list_st);
        }

        inst(STORE_SUBSCR_DICT, (unused/1, value, dict_st, sub -- )) {
            PyObject *dict = PyStackRef_AsPyObjectBorrow(dict_st);

            DEOPT_IF(!PyDict_CheckExact(dict));
            STAT_INC(STORE_SUBSCR, hit);
            int err = _PyDict_SetItem_Take2((PyDictObject *)dict,
                                            PyStackRef_AsPyObjectSteal(sub),
                                            PyStackRef_AsPyObjectSteal(value));
            PyStackRef_CLOSE(dict_st);
            ERROR_IF(err, error);
        }

        inst(DELETE_SUBSCR, (container, sub --)) {
            /* del container[sub] */
            int err = PyObject_DelItem(PyStackRef_AsPyObjectBorrow(container),
                                       PyStackRef_AsPyObjectBorrow(sub));
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        inst(CALL_INTRINSIC_1, (value -- res)) {
            assert(oparg <= MAX_INTRINSIC_1);
            PyObject *res_o = _PyIntrinsics_UnaryFunctions[oparg].func(tstate, PyStackRef_AsPyObjectBorrow(value));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(CALL_INTRINSIC_2, (value2_st, value1_st -- res)) {
            assert(oparg <= MAX_INTRINSIC_2);
            PyObject *value1 = PyStackRef_AsPyObjectBorrow(value1_st);
            PyObject *value2 = PyStackRef_AsPyObjectBorrow(value2_st);

            PyObject *res_o = _PyIntrinsics_BinaryFunctions[oparg].func(tstate, value2, value1);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        tier1 inst(RAISE_VARARGS, (args[oparg] -- )) {
            PyObject *cause = NULL, *exc = NULL;
            switch (oparg) {
            case 2:
                cause = PyStackRef_AsPyObjectSteal(args[1]);
                _Py_FALLTHROUGH;
            case 1:
                exc = PyStackRef_AsPyObjectSteal(args[0]);
                _Py_FALLTHROUGH;
            case 0:
                if (do_raise(tstate, exc, cause)) {
                    assert(oparg == 0);
                    monitor_reraise(tstate, frame, this_instr);
                    goto exception_unwind;
                }
                break;
            default:
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "bad RAISE_VARARGS oparg");
                break;
            }
            ERROR_IF(true, error);
        }

        tier1 inst(INTERPRETER_EXIT, (retval --)) {
            assert(frame == &entry_frame);
            assert(_PyFrame_IsIncomplete(frame));
            /* Restore previous frame and return. */
            tstate->current_frame = frame->previous;
            assert(!_PyErr_Occurred(tstate));
            tstate->c_recursion_remaining += PY_EVAL_C_STACK_UNITS;
            return PyStackRef_AsPyObjectSteal(retval);
        }

        // The stack effect here is ambiguous.
        // We definitely pop the return value off the stack on entry.
        // We also push it onto the stack on exit, but that's a
        // different frame, and it's accounted for by _PUSH_FRAME.
        inst(RETURN_VALUE, (retval -- res)) {
            #if TIER_ONE
            assert(frame != &entry_frame);
            #endif
            SYNC_SP();
            _PyFrame_SetStackPointer(frame, stack_pointer);
            assert(EMPTY());
            _Py_LeaveRecursiveCallPy(tstate);
            // GH-99729: We need to unlink the frame *before* clearing it:
            _PyInterpreterFrame *dying = frame;
            frame = tstate->current_frame = dying->previous;
            _PyEval_FrameClearAndPop(tstate, dying);
            LOAD_SP();
            LOAD_IP(frame->return_offset);
            res = retval;
            LLTRACE_RESUME_FRAME();
        }

        tier1 op(_RETURN_VALUE_EVENT, (val -- val)) {
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_RETURN,
                    frame, this_instr, PyStackRef_AsPyObjectBorrow(val));
            if (err) ERROR_NO_POP();
        }

        macro(INSTRUMENTED_RETURN_VALUE) =
            _RETURN_VALUE_EVENT +
            RETURN_VALUE;

        macro(RETURN_CONST) =
            LOAD_CONST +
            RETURN_VALUE;

        macro(INSTRUMENTED_RETURN_CONST) =
            LOAD_CONST +
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
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }

            iter_o = (*getter)(obj_o);
            DECREF_INPUTS();
            ERROR_IF(iter_o == NULL, error);

            if (Py_TYPE(iter_o)->tp_as_async == NULL ||
                    Py_TYPE(iter_o)->tp_as_async->am_anext == NULL) {

                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' received an object from __aiter__ "
                              "that does not implement __anext__: %.100s",
                              Py_TYPE(iter_o)->tp_name);
                Py_DECREF(iter_o);
                ERROR_IF(true, error);
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
            DECREF_INPUTS();
            ERROR_IF(iter_o == NULL, error);
            iter = PyStackRef_FromPyObjectSteal(iter_o);
        }

        family(SEND, INLINE_CACHE_ENTRIES_SEND) = {
            SEND_GEN,
        };

        specializing op(_SPECIALIZE_SEND, (counter/1, receiver, unused -- receiver, unused)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_Send(receiver, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(SEND);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_SEND, (receiver, v -- receiver, retval)) {
            PyObject *receiver_o = PyStackRef_AsPyObjectBorrow(receiver);

            PyObject *retval_o;
            assert(frame != &entry_frame);
            if ((tstate->interp->eval_frame == NULL) &&
                (Py_TYPE(receiver_o) == &PyGen_Type || Py_TYPE(receiver_o) == &PyCoro_Type) &&
                ((PyGenObject *)receiver_o)->gi_frame_state < FRAME_EXECUTING)
            {
                PyGenObject *gen = (PyGenObject *)receiver_o;
                _PyInterpreterFrame *gen_frame = &gen->gi_iframe;
                STACK_SHRINK(1);
                _PyFrame_StackPush(gen_frame, v);
                gen->gi_frame_state = FRAME_EXECUTING;
                gen->gi_exc_state.previous_item = tstate->exc_info;
                tstate->exc_info = &gen->gi_exc_state;
                assert(next_instr - this_instr + oparg <= UINT16_MAX);
                frame->return_offset = (uint16_t)(next_instr - this_instr + oparg);
                assert(gen_frame->previous == NULL);
                gen_frame->previous = frame;
                DISPATCH_INLINED(gen_frame);
            }
            if (PyStackRef_Is(v, PyStackRef_None) && PyIter_Check(receiver_o)) {
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
                    ERROR_NO_POP();
                }
            }
            PyStackRef_CLOSE(v);
            retval = PyStackRef_FromPyObjectSteal(retval_o);
        }

        macro(SEND) = _SPECIALIZE_SEND + _SEND;

        op(_SEND_GEN_FRAME, (receiver, v -- receiver, gen_frame: _PyInterpreterFrame *)) {
            PyGenObject *gen = (PyGenObject *)PyStackRef_AsPyObjectBorrow(receiver);
            DEOPT_IF(Py_TYPE(gen) != &PyGen_Type && Py_TYPE(gen) != &PyCoro_Type);
            DEOPT_IF(gen->gi_frame_state >= FRAME_EXECUTING);
            STAT_INC(SEND, hit);
            gen_frame = &gen->gi_iframe;
            _PyFrame_StackPush(gen_frame, v);
            gen->gi_frame_state = FRAME_EXECUTING;
            gen->gi_exc_state.previous_item = tstate->exc_info;
            tstate->exc_info = &gen->gi_exc_state;
            assert(1 + INLINE_CACHE_ENTRIES_SEND + oparg <= UINT16_MAX);
            frame->return_offset = (uint16_t)(1 + INLINE_CACHE_ENTRIES_SEND + oparg);
            gen_frame->previous = frame;
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
            #if TIER_ONE
            assert(frame != &entry_frame);
            #endif
            frame->instr_ptr++;
            PyGenObject *gen = _PyGen_GetGeneratorFromFrame(frame);
            assert(FRAME_SUSPENDED_YIELD_FROM == FRAME_SUSPENDED + 1);
            assert(oparg == 0 || oparg == 1);
            gen->gi_frame_state = FRAME_SUSPENDED + oparg;
            SYNC_SP();
            _PyFrame_SetStackPointer(frame, stack_pointer);
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
            LOAD_IP(1 + INLINE_CACHE_ENTRIES_SEND);
            LOAD_SP();
            value = retval;
            LLTRACE_RESUME_FRAME();
        }

        tier1 op(_YIELD_VALUE_EVENT, (val -- val)) {
            SAVE_SP();
            int err = _Py_call_instrumentation_arg(
                    tstate, PY_MONITORING_EVENT_PY_YIELD,
                    frame, this_instr, PyStackRef_AsPyObjectBorrow(val));
            LOAD_SP();
            if (err) ERROR_NO_POP();
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
                   PyStackRef_Is(exc_value, PyStackRef_None)
                    ? NULL : PyStackRef_AsPyObjectSteal(exc_value));
        }

        tier1 inst(RERAISE, (values[oparg], exc_st -- values[oparg])) {
            PyObject *exc = PyStackRef_AsPyObjectBorrow(exc_st);

            assert(oparg >= 0 && oparg <= 2);
            if (oparg) {
                PyObject *lasti = PyStackRef_AsPyObjectBorrow(values[0]);
                if (PyLong_Check(lasti)) {
                    frame->instr_ptr = _PyCode_CODE(_PyFrame_GetCode(frame)) + PyLong_AsLong(lasti);
                    assert(!_PyErr_Occurred(tstate));
                }
                else {
                    _PyErr_SetString(tstate, PyExc_SystemError, "lasti is not an int");
                    ERROR_NO_POP();
                }
            }
            assert(exc && PyExceptionInstance_Check(exc));
            Py_INCREF(exc);
            _PyErr_SetRaisedException(tstate, exc);
            monitor_reraise(tstate, frame, this_instr);
            goto exception_unwind;
        }

        tier1 inst(END_ASYNC_FOR, (awaitable_st, exc_st -- )) {
            PyObject *exc = PyStackRef_AsPyObjectBorrow(exc_st);

            assert(exc && PyExceptionInstance_Check(exc));
            if (PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration)) {
                DECREF_INPUTS();
            }
            else {
                Py_INCREF(exc);
                _PyErr_SetRaisedException(tstate, exc);
                monitor_reraise(tstate, frame, this_instr);
                goto exception_unwind;
            }
        }

        tier1 inst(CLEANUP_THROW, (sub_iter_st, last_sent_val_st, exc_value_st -- none, value)) {
            PyObject *exc_value = PyStackRef_AsPyObjectBorrow(exc_value_st);
            assert(throwflag);
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
            switch(oparg) {
            case CONSTANT_ASSERTIONERROR:
                value = PyStackRef_FromPyObjectImmortal(PyExc_AssertionError);
                break;
            case CONSTANT_NOTIMPLEMENTEDERROR:
                value = PyStackRef_FromPyObjectImmortal(PyExc_NotImplementedError);
                break;
            default:
                Py_FatalError("bad LOAD_COMMON_CONSTANT oparg");
            }
        }

        inst(LOAD_BUILD_CLASS, ( -- bc)) {
            PyObject *bc_o;
            ERROR_IF(PyMapping_GetOptionalItem(BUILTINS(), &_Py_ID(__build_class__), &bc_o) < 0, error);
            if (bc_o == NULL) {
                _PyErr_SetString(tstate, PyExc_NameError,
                                 "__build_class__ not found");
                ERROR_IF(true, error);
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
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            if (PyDict_CheckExact(ns))
                err = PyDict_SetItem(ns, name, PyStackRef_AsPyObjectBorrow(v));
            else
                err = PyObject_SetItem(ns, name, PyStackRef_AsPyObjectBorrow(v));
            DECREF_INPUTS();
            ERROR_IF(err, error);
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
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_UnpackSequence(seq, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(UNPACK_SEQUENCE);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
            (void)seq;
            (void)counter;
        }

        op(_UNPACK_SEQUENCE, (seq -- output[oparg])) {
            _PyStackRef *top = output + oparg;
            int res = _PyEval_UnpackIterableStackRef(tstate, seq, oparg, -1, top);
            DECREF_INPUTS();
            ERROR_IF(res == 0, error);
        }

        macro(UNPACK_SEQUENCE) = _SPECIALIZE_UNPACK_SEQUENCE + _UNPACK_SEQUENCE;

        inst(UNPACK_SEQUENCE_TWO_TUPLE, (unused/1, seq -- val1, val0)) {
            assert(oparg == 2);
            PyObject *seq_o = PyStackRef_AsPyObjectBorrow(seq);
            DEOPT_IF(!PyTuple_CheckExact(seq_o));
            DEOPT_IF(PyTuple_GET_SIZE(seq_o) != 2);
            STAT_INC(UNPACK_SEQUENCE, hit);
            val0 = PyStackRef_FromPyObjectNew(PyTuple_GET_ITEM(seq_o, 0));
            val1 = PyStackRef_FromPyObjectNew(PyTuple_GET_ITEM(seq_o, 1));
            DECREF_INPUTS();
        }

        inst(UNPACK_SEQUENCE_TUPLE, (unused/1, seq -- values[oparg])) {
            PyObject *seq_o = PyStackRef_AsPyObjectBorrow(seq);
            DEOPT_IF(!PyTuple_CheckExact(seq_o));
            DEOPT_IF(PyTuple_GET_SIZE(seq_o) != oparg);
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyObject **items = _PyTuple_ITEMS(seq_o);
            for (int i = oparg; --i >= 0; ) {
                *values++ = PyStackRef_FromPyObjectNew(items[i]);
            }
            DECREF_INPUTS();
        }

        inst(UNPACK_SEQUENCE_LIST, (unused/1, seq -- values[oparg])) {
            PyObject *seq_o = PyStackRef_AsPyObjectBorrow(seq);
            DEOPT_IF(!PyList_CheckExact(seq_o));
            DEOPT_IF(PyList_GET_SIZE(seq_o) != oparg);
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyObject **items = _PyList_ITEMS(seq_o);
            for (int i = oparg; --i >= 0; ) {
                *values++ = PyStackRef_FromPyObjectNew(items[i]);
            }
            DECREF_INPUTS();
        }

        inst(UNPACK_EX, (seq -- left[oparg & 0xFF], unused, right[oparg >> 8])) {
            _PyStackRef *top = right + (oparg >> 8);
            int res = _PyEval_UnpackIterableStackRef(tstate, seq, oparg & 0xFF, oparg >> 8, top);
            DECREF_INPUTS();
            ERROR_IF(res == 0, error);
        }

        family(STORE_ATTR, INLINE_CACHE_ENTRIES_STORE_ATTR) = {
            STORE_ATTR_INSTANCE_VALUE,
            STORE_ATTR_SLOT,
            STORE_ATTR_WITH_HINT,
        };

        specializing op(_SPECIALIZE_STORE_ATTR, (counter/1, owner -- owner)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
                next_instr = this_instr;
                _Py_Specialize_StoreAttr(owner, next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(STORE_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_STORE_ATTR, (v, owner --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = PyObject_SetAttr(PyStackRef_AsPyObjectBorrow(owner),
                                       name, PyStackRef_AsPyObjectBorrow(v));
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        macro(STORE_ATTR) = _SPECIALIZE_STORE_ATTR + unused/3 + _STORE_ATTR;

        inst(DELETE_ATTR, (owner --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = PyObject_DelAttr(PyStackRef_AsPyObjectBorrow(owner), name);
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        inst(STORE_GLOBAL, (v --)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            int err = PyDict_SetItem(GLOBALS(), name, PyStackRef_AsPyObjectBorrow(v));
            DECREF_INPUTS();
            ERROR_IF(err, error);
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
                ERROR_IF(true, error);
            }
            locals = PyStackRef_FromPyObjectNew(l);
        }

        inst(LOAD_FROM_DICT_OR_GLOBALS, (mod_or_class_dict -- v)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *v_o;
            int err = PyMapping_GetOptionalItem(PyStackRef_AsPyObjectBorrow(mod_or_class_dict), name, &v_o);
            if (err < 0) {
                ERROR_NO_POP();
            }
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
                    ERROR_IF(PyMapping_GetOptionalItem(GLOBALS(), name, &v_o) < 0, error);
                    if (v_o == NULL) {
                        /* namespace 2: builtins */
                        ERROR_IF(PyMapping_GetOptionalItem(BUILTINS(), name, &v_o) < 0, error);
                        if (v_o == NULL) {
                            _PyEval_FormatExcCheckArg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                            ERROR_IF(true, error);
                        }
                    }
                }
            }
            DECREF_INPUTS();
            v = PyStackRef_FromPyObjectSteal(v_o);
        }

        inst(LOAD_NAME, (-- v)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *v_o = _PyEval_LoadName(tstate, frame, name);
            ERROR_IF(v_o == NULL, error);
            v = PyStackRef_FromPyObjectSteal(v_o);
        }

        family(LOAD_GLOBAL, INLINE_CACHE_ENTRIES_LOAD_GLOBAL) = {
            LOAD_GLOBAL_MODULE,
            LOAD_GLOBAL_BUILTIN,
        };

        specializing op(_SPECIALIZE_LOAD_GLOBAL, (counter/1 -- )) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
                next_instr = this_instr;
                _Py_Specialize_LoadGlobal(GLOBALS(), BUILTINS(), next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_GLOBAL);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        // res[1] because we need a pointer to res to pass it to _PyEval_LoadGlobalStackRef
        op(_LOAD_GLOBAL, ( -- res[1], null if (oparg & 1))) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
            _PyEval_LoadGlobalStackRef(GLOBALS(), BUILTINS(), name, res);
            ERROR_IF(PyStackRef_IsNull(*res), error);
            null = PyStackRef_NULL;
        }

        macro(LOAD_GLOBAL) =
            _SPECIALIZE_LOAD_GLOBAL +
            counter/1 +
            globals_version/1 +
            builtins_version/1 +
            _LOAD_GLOBAL;

        op(_GUARD_GLOBALS_VERSION, (version/1 --)) {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            DEOPT_IF(dict->ma_keys->dk_version != version);
            assert(DK_IS_UNICODE(dict->ma_keys));
        }

        op(_GUARD_BUILTINS_VERSION, (version/1 --)) {
            PyDictObject *dict = (PyDictObject *)BUILTINS();
            DEOPT_IF(!PyDict_CheckExact(dict));
            DEOPT_IF(dict->ma_keys->dk_version != version);
            assert(DK_IS_UNICODE(dict->ma_keys));
        }

        op(_LOAD_GLOBAL_MODULE, (index/1 -- res, null if (oparg & 1))) {
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
            PyObject *res_o = entries[index].me_value;
            DEOPT_IF(res_o == NULL);
            Py_INCREF(res_o);
            STAT_INC(LOAD_GLOBAL, hit);
            null = PyStackRef_NULL;
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_LOAD_GLOBAL_BUILTINS, (index/1 -- res, null if (oparg & 1))) {
            PyDictObject *bdict = (PyDictObject *)BUILTINS();
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(bdict->ma_keys);
            PyObject *res_o = entries[index].me_value;
            DEOPT_IF(res_o == NULL);
            Py_INCREF(res_o);
            STAT_INC(LOAD_GLOBAL, hit);
            null = PyStackRef_NULL;
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(LOAD_GLOBAL_MODULE) =
            unused/1 + // Skip over the counter
            _GUARD_GLOBALS_VERSION +
            unused/1 + // Skip over the builtins version
            _LOAD_GLOBAL_MODULE;

        macro(LOAD_GLOBAL_BUILTIN) =
            unused/1 + // Skip over the counter
            _GUARD_GLOBALS_VERSION +
            _GUARD_BUILTINS_VERSION +
            _LOAD_GLOBAL_BUILTINS;

        inst(DELETE_FAST, (--)) {
            _PyStackRef v = GETLOCAL(oparg);
            if (PyStackRef_IsNull(v)) {
                _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
                    UNBOUNDLOCAL_ERROR_MSG,
                    PyTuple_GetItem(_PyFrame_GetCode(frame)->co_localsplusnames, oparg)
                );
                ERROR_IF(1, error);
            }
            SETLOCAL(oparg, PyStackRef_NULL);
        }

        inst(MAKE_CELL, (--)) {
            // "initial" is probably NULL but not if it's an arg (or set
            // via the f_locals proxy before MAKE_CELL has run).
            PyObject *initial = PyStackRef_AsPyObjectBorrow(GETLOCAL(oparg));
            PyObject *cell = PyCell_New(initial);
            if (cell == NULL) {
                ERROR_NO_POP();
            }
            SETLOCAL(oparg, PyStackRef_FromPyObjectSteal(cell));
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
            PyObject *value_o = PyCell_GetRef(cell);
            if (value_o == NULL) {
                _PyEval_FormatExcUnbound(tstate, _PyFrame_GetCode(frame), oparg);
                ERROR_IF(true, error);
            }
            value = PyStackRef_FromPyObjectSteal(value_o);
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
                ERROR_IF(true, error);
            }
            PyObject *str_o = _PyUnicode_JoinArray(&_Py_STR(empty), pieces_o, oparg);
            STACKREFS_TO_PYOBJECTS_CLEANUP(pieces_o);
            DECREF_INPUTS();
            ERROR_IF(str_o == NULL, error);
            str = PyStackRef_FromPyObjectSteal(str_o);
        }

        inst(BUILD_TUPLE, (values[oparg] -- tup)) {
            PyObject *tup_o = _PyTuple_FromStackRefSteal(values, oparg);
            ERROR_IF(tup_o == NULL, error);
            tup = PyStackRef_FromPyObjectSteal(tup_o);
        }

        inst(BUILD_LIST, (values[oparg] -- list)) {
            PyObject *list_o = _PyList_FromStackRefSteal(values, oparg);
            ERROR_IF(list_o == NULL, error);
            list = PyStackRef_FromPyObjectSteal(list_o);
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
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            assert(Py_IsNone(none_val));
            DECREF_INPUTS();
        }

        inst(SET_UPDATE, (set, unused[oparg-1], iterable -- set, unused[oparg-1])) {
            int err = _PySet_Update(PyStackRef_AsPyObjectBorrow(set),
                                    PyStackRef_AsPyObjectBorrow(iterable));
            DECREF_INPUTS();
            ERROR_IF(err < 0, error);
        }

        inst(BUILD_SET, (values[oparg] -- set)) {
            PyObject *set_o = PySet_New(NULL);
            if (set_o == NULL) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            int err = 0;
            for (int i = 0; i < oparg; i++) {
                if (err == 0) {
                    err = PySet_Add(set_o, PyStackRef_AsPyObjectBorrow(values[i]));
                }
                PyStackRef_CLOSE(values[i]);
            }
            if (err != 0) {
                Py_DECREF(set_o);
                ERROR_IF(true, error);
            }
            set = PyStackRef_FromPyObjectSteal(set_o);
        }

        inst(BUILD_MAP, (values[oparg*2] -- map)) {
            STACKREFS_TO_PYOBJECTS(values, oparg*2, values_o);
            if (CONVERSION_FAILED(values_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *map_o = _PyDict_FromItems(
                    values_o, 2,
                    values_o+1, 2,
                    oparg);
            STACKREFS_TO_PYOBJECTS_CLEANUP(values_o);
            DECREF_INPUTS();
            ERROR_IF(map_o == NULL, error);
            map = PyStackRef_FromPyObjectSteal(map_o);
        }

        inst(SETUP_ANNOTATIONS, (--)) {
            int err;
            PyObject *ann_dict;
            if (LOCALS() == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when setting up annotations");
                ERROR_IF(true, error);
            }
            /* check if __annotations__ in locals()... */
            ERROR_IF(PyMapping_GetOptionalItem(LOCALS(), &_Py_ID(__annotations__), &ann_dict) < 0, error);
            if (ann_dict == NULL) {
                ann_dict = PyDict_New();
                ERROR_IF(ann_dict == NULL, error);
                err = PyObject_SetItem(LOCALS(), &_Py_ID(__annotations__),
                                       ann_dict);
                Py_DECREF(ann_dict);
                ERROR_IF(err, error);
            }
            else {
                Py_DECREF(ann_dict);
            }
        }

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
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            DECREF_INPUTS();
        }

        inst(DICT_MERGE, (callable, unused, unused, dict, unused[oparg - 1], update -- callable, unused, unused, dict, unused[oparg - 1])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *dict_o = PyStackRef_AsPyObjectBorrow(dict);
            PyObject *update_o = PyStackRef_AsPyObjectBorrow(update);

            int err = _PyDict_MergeEx(dict_o, update_o, 2);
            if (err < 0) {
                _PyEval_FormatKwargsError(tstate, callable_o, update_o);
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            DECREF_INPUTS();
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
            ERROR_IF(err != 0, error);
        }

        inst(INSTRUMENTED_LOAD_SUPER_ATTR, (unused/1, unused, unused, unused -- unused, unused if (oparg & 1))) {
            // cancel out the decrement that will happen in LOAD_SUPER_ATTR; we
            // don't want to specialize instrumented instructions
            PAUSE_ADAPTIVE_COUNTER(this_instr[1].counter);
            GO_TO_INSTRUCTION(LOAD_SUPER_ATTR);
        }

        family(LOAD_SUPER_ATTR, INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR) = {
            LOAD_SUPER_ATTR_ATTR,
            LOAD_SUPER_ATTR_METHOD,
        };

        specializing op(_SPECIALIZE_LOAD_SUPER_ATTR, (counter/1, global_super_st, class_st, unused -- global_super_st, class_st, unused)) {
            #if ENABLE_SPECIALIZATION
            int load_method = oparg & 1;
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_LoadSuperAttr(global_super_st, class_st, next_instr, load_method);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_SUPER_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        tier1 op(_LOAD_SUPER_ATTR, (global_super_st, class_st, self_st -- attr, null if (oparg & 1))) {
            PyObject *global_super = PyStackRef_AsPyObjectBorrow(global_super_st);
            PyObject *class = PyStackRef_AsPyObjectBorrow(class_st);
            PyObject *self = PyStackRef_AsPyObjectBorrow(self_st);

            if (opcode == INSTRUMENTED_LOAD_SUPER_ATTR) {
                PyObject *arg = oparg & 2 ? class : &_PyInstrumentation_MISSING;
                int err = _Py_call_instrumentation_2args(
                        tstate, PY_MONITORING_EVENT_CALL,
                        frame, this_instr, global_super, arg);
                ERROR_IF(err, error);
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
            ERROR_IF(super == NULL, error);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg >> 2);
            attr = PyStackRef_FromPyObjectSteal(PyObject_GetAttr(super, name));
            Py_DECREF(super);
            ERROR_IF(PyStackRef_IsNull(attr), error);
            null = PyStackRef_NULL;
        }

        macro(LOAD_SUPER_ATTR) = _SPECIALIZE_LOAD_SUPER_ATTR + _LOAD_SUPER_ATTR;

        inst(LOAD_SUPER_ATTR_ATTR, (unused/1, global_super_st, class_st, self_st -- attr_st, unused if (0))) {
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
            ERROR_IF(attr == NULL, error);
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
            PyStackRef_CLOSE(global_super_st);
            PyStackRef_CLOSE(class_st);
            if (attr_o == NULL) {
                PyStackRef_CLOSE(self_st);
                ERROR_IF(true, error);
            }
            if (method_found) {
                self_or_null = self_st; // transfer ownership
            } else {
                PyStackRef_CLOSE(self_st);
                self_or_null = PyStackRef_NULL;
            }

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
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
                next_instr = this_instr;
                _Py_Specialize_LoadAttr(owner, next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(LOAD_ATTR);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_LOAD_ATTR, (owner -- attr, self_or_null if (oparg & 1))) {
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
                    self_or_null = owner;  // Transfer ownership
                }
                else {
                    /* meth is not an unbound method (but a regular attr, or
                       something was returned by a descriptor protocol).  Set
                       the second element of the stack to NULL, to signal
                       CALL that it's not a method call.
                       meth | NULL | arg1 | ... | argN
                    */
                    DECREF_INPUTS();
                    ERROR_IF(attr_o == NULL, error);
                    self_or_null = PyStackRef_NULL;
                }
            }
            else {
                /* Classic, pushes one value. */
                attr_o = PyObject_GetAttr(PyStackRef_AsPyObjectBorrow(owner), name);
                DECREF_INPUTS();
                ERROR_IF(attr_o == NULL, error);
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
            EXIT_IF(tp->tp_version_tag != type_version);
        }

        op(_CHECK_MANAGED_OBJECT_HAS_VALUES, (owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            DEOPT_IF(!_PyObject_InlineValues(owner_o)->valid);
        }

        split op(_LOAD_ATTR_INSTANCE_VALUE, (offset/1, owner -- attr, null if (oparg & 1))) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            PyObject **value_ptr = (PyObject**)(((char *)owner_o) + offset);
            PyObject *attr_o = *value_ptr;
            DEOPT_IF(attr_o == NULL);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(attr_o);
            null = PyStackRef_NULL;
            attr = PyStackRef_FromPyObjectSteal(attr_o);
            DECREF_INPUTS();
        }

        macro(LOAD_ATTR_INSTANCE_VALUE) =
            unused/1 + // Skip over the counter
            _GUARD_TYPE_VERSION +
            _CHECK_MANAGED_OBJECT_HAS_VALUES +
            _LOAD_ATTR_INSTANCE_VALUE +
            unused/5;  // Skip over rest of cache

        op(_CHECK_ATTR_MODULE, (dict_version/2, owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            DEOPT_IF(!PyModule_CheckExact(owner_o));
            PyDictObject *dict = (PyDictObject *)((PyModuleObject *)owner_o)->md_dict;
            assert(dict != NULL);
            DEOPT_IF(dict->ma_keys->dk_version != dict_version);
        }

        op(_LOAD_ATTR_MODULE, (index/1, owner -- attr, null if (oparg & 1))) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            PyDictObject *dict = (PyDictObject *)((PyModuleObject *)owner_o)->md_dict;
            assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
            assert(index < dict->ma_keys->dk_nentries);
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + index;
            PyObject *attr_o = ep->me_value;
            DEOPT_IF(attr_o == NULL);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(attr_o);
            attr = PyStackRef_FromPyObjectSteal(attr_o);
            null = PyStackRef_NULL;
            DECREF_INPUTS();
        }

        macro(LOAD_ATTR_MODULE) =
            unused/1 +
            _CHECK_ATTR_MODULE +
            _LOAD_ATTR_MODULE +
            unused/5;

        op(_CHECK_ATTR_WITH_HINT, (owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            EXIT_IF(dict == NULL);
            assert(PyDict_CheckExact((PyObject *)dict));
        }

        op(_LOAD_ATTR_WITH_HINT, (hint/1, owner -- attr, null if (oparg & 1))) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            PyObject *attr_o;

            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries);
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg>>1);
            DEOPT_IF(!DK_IS_UNICODE(dict->ma_keys));
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + hint;
            DEOPT_IF(ep->me_key != name);
            attr_o = ep->me_value;
            DEOPT_IF(attr_o == NULL);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(attr_o);
            attr = PyStackRef_FromPyObjectSteal(attr_o);
            null = PyStackRef_NULL;
            DECREF_INPUTS();
        }

        macro(LOAD_ATTR_WITH_HINT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _CHECK_ATTR_WITH_HINT +
            _LOAD_ATTR_WITH_HINT +
            unused/5;

        split op(_LOAD_ATTR_SLOT, (index/1, owner -- attr, null if (oparg & 1))) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            char *addr = (char *)owner_o + index;
            PyObject *attr_o = *(PyObject **)addr;
            DEOPT_IF(attr_o == NULL);
            STAT_INC(LOAD_ATTR, hit);
            null = PyStackRef_NULL;
            attr = PyStackRef_FromPyObjectNew(attr_o);
            DECREF_INPUTS();
        }

        macro(LOAD_ATTR_SLOT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _LOAD_ATTR_SLOT +  // NOTE: This action may also deopt
            unused/5;

        op(_CHECK_ATTR_CLASS, (type_version/2, owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            EXIT_IF(!PyType_Check(owner_o));
            assert(type_version != 0);
            EXIT_IF(((PyTypeObject *)owner_o)->tp_version_tag != type_version);
        }

        split op(_LOAD_ATTR_CLASS, (descr/4, owner -- attr, null if (oparg & 1))) {
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            attr = PyStackRef_FromPyObjectNew(descr);
            null = PyStackRef_NULL;
            DECREF_INPUTS();
        }

        macro(LOAD_ATTR_CLASS) =
            unused/1 +
            _CHECK_ATTR_CLASS +
            unused/2 +
            _LOAD_ATTR_CLASS;

        macro(LOAD_ATTR_CLASS_WITH_METACLASS_CHECK) =
            unused/1 +
            _CHECK_ATTR_CLASS +
            _GUARD_TYPE_VERSION +
            _LOAD_ATTR_CLASS;

        op(_LOAD_ATTR_PROPERTY_FRAME, (fget/4, owner -- new_frame: _PyInterpreterFrame *)) {
            assert((oparg & 1) == 0);
            assert(Py_IS_TYPE(fget, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)fget;
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            DEOPT_IF((code->co_flags & (CO_VARKEYWORDS | CO_VARARGS | CO_OPTIMIZED)) != CO_OPTIMIZED);
            DEOPT_IF(code->co_kwonlyargcount);
            DEOPT_IF(code->co_argcount != 1);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            STAT_INC(LOAD_ATTR, hit);
            new_frame = _PyFrame_PushUnchecked(tstate, PyStackRef_FromPyObjectNew(fget), 1, frame);
            new_frame->localsplus[0] = owner;
        }

        macro(LOAD_ATTR_PROPERTY) =
            unused/1 +
            _CHECK_PEP_523 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_PROPERTY_FRAME +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        inst(LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN, (unused/1, type_version/2, func_version/2, getattribute/4, owner -- unused, unused if (0))) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            assert((oparg & 1) == 0);
            DEOPT_IF(tstate->interp->eval_frame);
            PyTypeObject *cls = Py_TYPE(owner_o);
            assert(type_version != 0);
            DEOPT_IF(cls->tp_version_tag != type_version);
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
            // Manipulate stack directly because we exit with DISPATCH_INLINED().
            STACK_SHRINK(1);
            new_frame->localsplus[0] = owner;
            new_frame->localsplus[1] = PyStackRef_FromPyObjectNew(name);
            frame->return_offset = (uint16_t)(next_instr - this_instr);
            DISPATCH_INLINED(new_frame);
        }

        op(_GUARD_DORV_NO_DICT, (owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            assert(Py_TYPE(owner_o)->tp_dictoffset < 0);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            EXIT_IF(_PyObject_GetManagedDict(owner_o));
            EXIT_IF(_PyObject_InlineValues(owner_o)->valid == 0);
        }

        op(_STORE_ATTR_INSTANCE_VALUE, (offset/1, value, owner --)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            STAT_INC(STORE_ATTR, hit);
            assert(_PyObject_GetManagedDict(owner_o) == NULL);
            PyObject **value_ptr = (PyObject**)(((char *)owner_o) + offset);
            PyObject *old_value = *value_ptr;
            *value_ptr = PyStackRef_AsPyObjectSteal(value);
            if (old_value == NULL) {
                PyDictValues *values = _PyObject_InlineValues(owner_o);
                Py_ssize_t index = value_ptr - values->values;
                _PyDictValues_AddToInsertionOrder(values, index);
            }
            else {
                Py_DECREF(old_value);
            }
            PyStackRef_CLOSE(owner);
        }

        macro(STORE_ATTR_INSTANCE_VALUE) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_DORV_NO_DICT +
            _STORE_ATTR_INSTANCE_VALUE;

        op(_STORE_ATTR_WITH_HINT, (hint/1, value, owner --)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictObject *dict = _PyObject_GetManagedDict(owner_o);
            DEOPT_IF(dict == NULL);
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries);
            PyObject *old_value;
            DEOPT_IF(!DK_IS_UNICODE(dict->ma_keys));
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + hint;
            DEOPT_IF(ep->me_key != name);
            /* Ensure dict is GC tracked if it needs to be */
            if (!_PyObject_GC_IS_TRACKED(dict) && _PyObject_GC_MAY_BE_TRACKED(PyStackRef_AsPyObjectBorrow(value))) {
                _PyObject_GC_TRACK(dict);
            }
            old_value = ep->me_value;
            PyDict_WatchEvent event = old_value == NULL ? PyDict_EVENT_ADDED : PyDict_EVENT_MODIFIED;
            _PyDict_NotifyEvent(tstate->interp, event, dict, name, PyStackRef_AsPyObjectBorrow(value));
            ep->me_value = PyStackRef_AsPyObjectSteal(value);
            // old_value should be DECREFed after GC track checking is done, if not, it could raise a segmentation fault,
            // when dict only holds the strong reference to value in ep->me_value.
            Py_XDECREF(old_value);
            STAT_INC(STORE_ATTR, hit);
            PyStackRef_CLOSE(owner);
        }

        macro(STORE_ATTR_WITH_HINT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _STORE_ATTR_WITH_HINT;

        op(_STORE_ATTR_SLOT, (index/1, value, owner --)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);

            char *addr = (char *)owner_o + index;
            STAT_INC(STORE_ATTR, hit);
            PyObject *old_value = *(PyObject **)addr;
            *(PyObject **)addr = PyStackRef_AsPyObjectSteal(value);
            Py_XDECREF(old_value);
            PyStackRef_CLOSE(owner);
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
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_CompareOp(left, right, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(COMPARE_OP);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_COMPARE_OP, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            assert((oparg >> 5) <= Py_GE);
            PyObject *res_o = PyObject_RichCompare(left_o, right_o, oparg >> 5);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
            if (oparg & 16) {
                int res_bool = PyObject_IsTrue(res_o);
                Py_DECREF(res_o);
                ERROR_IF(res_bool < 0, error);
                res = res_bool ? PyStackRef_True : PyStackRef_False;
            }
            else {
                res = PyStackRef_FromPyObjectSteal(res_o);
            }
        }

        macro(COMPARE_OP) = _SPECIALIZE_COMPARE_OP + _COMPARE_OP;

        macro(COMPARE_OP_FLOAT) =
            _GUARD_BOTH_FLOAT + unused/1 + _COMPARE_OP_FLOAT;

        macro(COMPARE_OP_INT) =
            _GUARD_BOTH_INT + unused/1 + _COMPARE_OP_INT;

        macro(COMPARE_OP_STR) =
            _GUARD_BOTH_UNICODE + unused/1 + _COMPARE_OP_STR;

        op(_COMPARE_OP_FLOAT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            STAT_INC(COMPARE_OP, hit);
            double dleft = PyFloat_AS_DOUBLE(left_o);
            double dright = PyFloat_AS_DOUBLE(right_o);
            // 1 if NaN, 2 if <, 4 if >, 8 if ==; this matches low four bits of the oparg
            int sign_ish = COMPARISON_BIT(dleft, dright);
            _Py_DECREF_SPECIALIZED(left_o, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right_o, _PyFloat_ExactDealloc);
            res = (sign_ish & oparg) ? PyStackRef_True : PyStackRef_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        // Similar to COMPARE_OP_FLOAT
        op(_COMPARE_OP_INT, (left, right -- res)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            DEOPT_IF(!_PyLong_IsCompact((PyLongObject *)left_o));
            DEOPT_IF(!_PyLong_IsCompact((PyLongObject *)right_o));
            STAT_INC(COMPARE_OP, hit);
            assert(_PyLong_DigitCount((PyLongObject *)left_o) <= 1 &&
                   _PyLong_DigitCount((PyLongObject *)right_o) <= 1);
            Py_ssize_t ileft = _PyLong_CompactValue((PyLongObject *)left_o);
            Py_ssize_t iright = _PyLong_CompactValue((PyLongObject *)right_o);
            // 2 if <, 4 if >, 8 if ==; this matches the low 4 bits of the oparg
            int sign_ish = COMPARISON_BIT(ileft, iright);
            _Py_DECREF_SPECIALIZED(left_o, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(right_o, (destructor)PyObject_Free);
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
            _Py_DECREF_SPECIALIZED(left_o, _PyUnicode_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right_o, _PyUnicode_ExactDealloc);
            assert(eq == 0 || eq == 1);
            assert((oparg & 0xf) == COMPARISON_NOT_EQUALS || (oparg & 0xf) == COMPARISON_EQUALS);
            assert(COMPARISON_NOT_EQUALS + 1 == COMPARISON_EQUALS);
            res = ((COMPARISON_NOT_EQUALS + eq) & oparg) ? PyStackRef_True : PyStackRef_False;
            // It's always a bool, so we don't care about oparg & 16.
        }

        inst(IS_OP, (left, right -- b)) {
#ifdef Py_GIL_DISABLED
            // On free-threaded builds, objects are conditionally immortalized.
            // So their bits don't always compare equally.
            int res = Py_Is(PyStackRef_AsPyObjectBorrow(left), PyStackRef_AsPyObjectBorrow(right)) ^ oparg;
#else
            int res = PyStackRef_Is(left, right) ^ oparg;
#endif
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
            ERROR_IF(res < 0, error);
            b = (res ^ oparg) ? PyStackRef_True : PyStackRef_False;
        }

        specializing op(_SPECIALIZE_CONTAINS_OP, (counter/1, left, right -- left, right)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_ContainsOp(right, next_instr);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CONTAINS_OP);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(CONTAINS_OP) = _SPECIALIZE_CONTAINS_OP + _CONTAINS_OP;

        inst(CONTAINS_OP_SET, (unused/1, left, right -- b)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            DEOPT_IF(!(PySet_CheckExact(right_o) || PyFrozenSet_CheckExact(right_o)));
            STAT_INC(CONTAINS_OP, hit);
            // Note: both set and frozenset use the same seq_contains method!
            int res = _PySet_Contains((PySetObject *)right_o, left_o);
            DECREF_INPUTS();
            ERROR_IF(res < 0, error);
            b = (res ^ oparg) ? PyStackRef_True : PyStackRef_False;
        }

        inst(CONTAINS_OP_DICT, (unused/1, left, right -- b)) {
            PyObject *left_o = PyStackRef_AsPyObjectBorrow(left);
            PyObject *right_o = PyStackRef_AsPyObjectBorrow(right);

            DEOPT_IF(!PyDict_CheckExact(right_o));
            STAT_INC(CONTAINS_OP, hit);
            int res = PyDict_Contains(right_o, left_o);
            DECREF_INPUTS();
            ERROR_IF(res < 0, error);
            b = (res ^ oparg) ? PyStackRef_True : PyStackRef_False;
        }

        inst(CHECK_EG_MATCH, (exc_value_st, match_type_st -- rest, match)) {
            PyObject *exc_value = PyStackRef_AsPyObjectBorrow(exc_value_st);
            PyObject *match_type = PyStackRef_AsPyObjectBorrow(match_type_st);
            int err = _PyEval_CheckExceptStarTypeValid(tstate, match_type);
            if (err < 0) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }

            PyObject *match_o = NULL;
            PyObject *rest_o = NULL;
            int res = _PyEval_ExceptionGroupMatch(exc_value, match_type,
                                                  &match_o, &rest_o);
            DECREF_INPUTS();
            ERROR_IF(res < 0, error);

            assert((match_o == NULL) == (rest_o == NULL));
            ERROR_IF(match_o == NULL, error);

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
                 DECREF_INPUTS();
                 ERROR_IF(true, error);
            }

            int res = PyErr_GivenExceptionMatches(left_o, right_o);
            DECREF_INPUTS();
            b = res ? PyStackRef_True : PyStackRef_False;
        }

         inst(IMPORT_NAME, (level, fromlist -- res)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *res_o = _PyEval_ImportName(tstate, frame, name,
                              PyStackRef_AsPyObjectBorrow(fromlist),
                              PyStackRef_AsPyObjectBorrow(level));
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(IMPORT_FROM, (from -- from, res)) {
            PyObject *name = GETITEM(FRAME_CO_NAMES, oparg);
            PyObject *res_o = _PyEval_ImportFrom(tstate, PyStackRef_AsPyObjectBorrow(from), name);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        tier1 inst(JUMP_FORWARD, (--)) {
            JUMPBY(oparg);
        }

        tier1 op(_JUMP_BACKWARD, (the_counter/1 --)) {
            assert(oparg <= INSTR_OFFSET());
            JUMPBY(-oparg);
            #ifdef _Py_TIER2
            #if ENABLE_SPECIALIZATION
            _Py_BackoffCounter counter = this_instr[1].counter;
            if (backoff_counter_triggers(counter) && this_instr->op.code == JUMP_BACKWARD) {
                _Py_CODEUNIT *start = this_instr;
                /* Back up over EXTENDED_ARGs so optimizer sees the whole instruction */
                while (oparg > 255) {
                    oparg >>= 8;
                    start--;
                }
                _PyExecutorObject *executor;
                int optimized = _PyOptimizer_Optimize(frame, start, stack_pointer, &executor, 0);
                ERROR_IF(optimized < 0, error);
                if (optimized) {
                    assert(tstate->previous_executor == NULL);
                    tstate->previous_executor = Py_None;
                    GOTO_TIER_TWO(executor);
                }
                else {
                    this_instr[1].counter = restart_backoff_counter(counter);
                }
            }
            else {
                ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            }
            #endif  /* ENABLE_SPECIALIZATION */
            #endif /* _Py_TIER2 */
        }

        macro(JUMP_BACKWARD) =
            _CHECK_PERIODIC +
            _JUMP_BACKWARD;

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
            assert(tstate->previous_executor == NULL);
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
            tstate->previous_executor = Py_None;
            Py_INCREF(executor);
            GOTO_TIER_TWO(executor);
            #else
            Py_FatalError("ENTER_EXECUTOR is not supported in this build");
            #endif /* _Py_TIER2 */
        }

        replaced op(_POP_JUMP_IF_FALSE, (cond -- )) {
            assert(PyStackRef_BoolCheck(cond));
            int flag = PyStackRef_Is(cond, PyStackRef_False);
            #if ENABLE_SPECIALIZATION
            this_instr[1].cache = (this_instr[1].cache << 1) | flag;
            #endif
            JUMPBY(oparg * flag);
        }

        replaced op(_POP_JUMP_IF_TRUE, (cond -- )) {
            assert(PyStackRef_BoolCheck(cond));
            int flag = PyStackRef_Is(cond, PyStackRef_True);
            #if ENABLE_SPECIALIZATION
            this_instr[1].cache = (this_instr[1].cache << 1) | flag;
            #endif
            JUMPBY(oparg * flag);
        }

        op(_IS_NONE, (value -- b)) {
            if (PyStackRef_Is(value, PyStackRef_None)) {
                b = PyStackRef_True;
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
            JUMPBY(-oparg);
        }

        inst(GET_LEN, (obj -- obj, len)) {
            // PUSH(len(TOS))
            Py_ssize_t len_i = PyObject_Length(PyStackRef_AsPyObjectBorrow(obj));
            ERROR_IF(len_i < 0, error);
            PyObject *len_o = PyLong_FromSsize_t(len_i);
            ERROR_IF(len_o == NULL, error);
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
                ERROR_IF(_PyErr_Occurred(tstate), error);  // Error!
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
            ERROR_IF(values_or_none_o == NULL, error);
            values_or_none = PyStackRef_FromPyObjectSteal(values_or_none_o);
        }

        inst(GET_ITER, (iterable -- iter)) {
            /* before: [obj]; after [getiter(obj)] */
            iter = PyStackRef_FromPyObjectSteal(PyObject_GetIter(PyStackRef_AsPyObjectBorrow(iterable)));
            DECREF_INPUTS();
            ERROR_IF(PyStackRef_IsNull(iter), error);
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
            }
            else if (PyGen_CheckExact(iterable_o)) {
                iter = iterable;
            }
            else {
                /* `iterable` is not a generator. */
                iter = PyStackRef_FromPyObjectSteal(PyObject_GetIter(iterable_o));
                if (PyStackRef_IsNull(iter)) {
                    ERROR_NO_POP();
                }
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

        specializing op(_SPECIALIZE_FOR_ITER, (counter/1, iter -- iter)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_ForIter(iter, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(FOR_ITER);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        replaced op(_FOR_ITER, (iter -- iter, next)) {
            /* before: [iter]; after: [iter, iter()] *or* [] (and jump over END_FOR.) */
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            PyObject *next_o = (*Py_TYPE(iter_o)->tp_iternext)(iter_o);
            if (next_o == NULL) {
                next = PyStackRef_NULL;
                if (_PyErr_Occurred(tstate)) {
                    int matches = _PyErr_ExceptionMatches(tstate, PyExc_StopIteration);
                    if (!matches) {
                        ERROR_NO_POP();
                    }
                    _PyEval_MonitorRaise(tstate, frame, this_instr);
                    _PyErr_Clear(tstate);
                }
                /* iterator ended normally */
                assert(next_instr[oparg].op.code == END_FOR ||
                       next_instr[oparg].op.code == INSTRUMENTED_END_FOR);
                PyStackRef_CLOSE(iter);
                STACK_SHRINK(1);
                /* Jump forward oparg, then skip following END_FOR and POP_TOP instruction */
                JUMPBY(oparg + 2);
                DISPATCH();
            }
            next = PyStackRef_FromPyObjectSteal(next_o);
            // Common case: no jump, leave it to the code generator
        }

        op(_FOR_ITER_TIER_TWO, (iter -- iter, next)) {
            /* before: [iter]; after: [iter, iter()] *or* [] (and jump over END_FOR.) */
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            PyObject *next_o = (*Py_TYPE(iter_o)->tp_iternext)(iter_o);
            if (next_o == NULL) {
                if (_PyErr_Occurred(tstate)) {
                    int matches = _PyErr_ExceptionMatches(tstate, PyExc_StopIteration);
                    if (!matches) {
                        ERROR_NO_POP();
                    }
                    _PyEval_MonitorRaise(tstate, frame, frame->instr_ptr);
                    _PyErr_Clear(tstate);
                }
                /* iterator ended normally */
                /* The translator sets the deopt target just past the matching END_FOR */
                EXIT_IF(true);
            }
            next = PyStackRef_FromPyObjectSteal(next_o);
            // Common case: no jump, leave it to the code generator
        }

        macro(FOR_ITER) = _SPECIALIZE_FOR_ITER + _FOR_ITER;

        inst(INSTRUMENTED_FOR_ITER, (unused/1 -- )) {
            _Py_CODEUNIT *target;
            _PyStackRef iter_stackref = TOP();
            PyObject *iter = PyStackRef_AsPyObjectBorrow(iter_stackref);
            PyObject *next = (*Py_TYPE(iter)->tp_iternext)(iter);
            if (next != NULL) {
                PUSH(PyStackRef_FromPyObjectSteal(next));
                target = next_instr;
            }
            else {
                if (_PyErr_Occurred(tstate)) {
                    int matches = _PyErr_ExceptionMatches(tstate, PyExc_StopIteration);
                    if (!matches) {
                        ERROR_NO_POP();
                    }
                    _PyEval_MonitorRaise(tstate, frame, this_instr);
                    _PyErr_Clear(tstate);
                }
                /* iterator ended normally */
                assert(next_instr[oparg].op.code == END_FOR ||
                       next_instr[oparg].op.code == INSTRUMENTED_END_FOR);
                STACK_SHRINK(1);
                PyStackRef_CLOSE(iter_stackref);
                /* Skip END_FOR and POP_TOP */
                target = next_instr + oparg + 2;
            }
            INSTRUMENTED_JUMP(this_instr, target, PY_MONITORING_EVENT_BRANCH);
        }

        op(_ITER_CHECK_LIST, (iter -- iter)) {
            EXIT_IF(Py_TYPE(PyStackRef_AsPyObjectBorrow(iter)) != &PyListIter_Type);
        }

        replaced op(_ITER_JUMP_LIST, (iter -- iter)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            _PyListIterObject *it = (_PyListIterObject *)iter_o;
            assert(Py_TYPE(iter_o) == &PyListIter_Type);
            STAT_INC(FOR_ITER, hit);
            PyListObject *seq = it->it_seq;
            if (seq == NULL || (size_t)it->it_index >= (size_t)PyList_GET_SIZE(seq)) {
                it->it_index = -1;
                #ifndef Py_GIL_DISABLED
                if (seq != NULL) {
                    it->it_seq = NULL;
                    Py_DECREF(seq);
                }
                #endif
                PyStackRef_CLOSE(iter);
                STACK_SHRINK(1);
                /* Jump forward oparg, then skip following END_FOR and POP_TOP instructions */
                JUMPBY(oparg + 2);
                DISPATCH();
            }
        }

        // Only used by Tier 2
        op(_GUARD_NOT_EXHAUSTED_LIST, (iter -- iter)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            _PyListIterObject *it = (_PyListIterObject *)iter_o;
            assert(Py_TYPE(iter_o) == &PyListIter_Type);
            PyListObject *seq = it->it_seq;
            EXIT_IF(seq == NULL);
            if ((size_t)it->it_index >= (size_t)PyList_GET_SIZE(seq)) {
                it->it_index = -1;
                EXIT_IF(1);
            }
        }

        op(_ITER_NEXT_LIST, (iter -- iter, next)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            _PyListIterObject *it = (_PyListIterObject *)iter_o;
            assert(Py_TYPE(iter_o) == &PyListIter_Type);
            PyListObject *seq = it->it_seq;
            assert(seq);
            assert(it->it_index < PyList_GET_SIZE(seq));
            next = PyStackRef_FromPyObjectNew(PyList_GET_ITEM(seq, it->it_index++));
        }

        macro(FOR_ITER_LIST) =
            unused/1 +  // Skip over the counter
            _ITER_CHECK_LIST +
            _ITER_JUMP_LIST +
            _ITER_NEXT_LIST;

        op(_ITER_CHECK_TUPLE, (iter -- iter)) {
            EXIT_IF(Py_TYPE(PyStackRef_AsPyObjectBorrow(iter)) != &PyTupleIter_Type);
        }

        replaced op(_ITER_JUMP_TUPLE, (iter -- iter)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            _PyTupleIterObject *it = (_PyTupleIterObject *)iter_o;
            assert(Py_TYPE(iter_o) == &PyTupleIter_Type);
            STAT_INC(FOR_ITER, hit);
            PyTupleObject *seq = it->it_seq;
            if (seq == NULL || it->it_index >= PyTuple_GET_SIZE(seq)) {
                if (seq != NULL) {
                    it->it_seq = NULL;
                    Py_DECREF(seq);
                }
                PyStackRef_CLOSE(iter);
                STACK_SHRINK(1);
                /* Jump forward oparg, then skip following END_FOR and POP_TOP instructions */
                JUMPBY(oparg + 2);
                DISPATCH();
            }
        }

        // Only used by Tier 2
        op(_GUARD_NOT_EXHAUSTED_TUPLE, (iter -- iter)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            _PyTupleIterObject *it = (_PyTupleIterObject *)iter_o;
            assert(Py_TYPE(iter_o) == &PyTupleIter_Type);
            PyTupleObject *seq = it->it_seq;
            EXIT_IF(seq == NULL);
            EXIT_IF(it->it_index >= PyTuple_GET_SIZE(seq));
        }

        op(_ITER_NEXT_TUPLE, (iter -- iter, next)) {
            PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
            _PyTupleIterObject *it = (_PyTupleIterObject *)iter_o;
            assert(Py_TYPE(iter_o) == &PyTupleIter_Type);
            PyTupleObject *seq = it->it_seq;
            assert(seq);
            assert(it->it_index < PyTuple_GET_SIZE(seq));
            next = PyStackRef_FromPyObjectNew(PyTuple_GET_ITEM(seq, it->it_index++));
        }

        macro(FOR_ITER_TUPLE) =
            unused/1 +  // Skip over the counter
            _ITER_CHECK_TUPLE +
            _ITER_JUMP_TUPLE +
            _ITER_NEXT_TUPLE;

        op(_ITER_CHECK_RANGE, (iter -- iter)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            EXIT_IF(Py_TYPE(r) != &PyRangeIter_Type);
        }

        replaced op(_ITER_JUMP_RANGE, (iter -- iter)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
            STAT_INC(FOR_ITER, hit);
            if (r->len <= 0) {
                STACK_SHRINK(1);
                PyStackRef_CLOSE(iter);
                // Jump over END_FOR and POP_TOP instructions.
                JUMPBY(oparg + 2);
                DISPATCH();
            }
        }

        // Only used by Tier 2
        op(_GUARD_NOT_EXHAUSTED_RANGE, (iter -- iter)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
            EXIT_IF(r->len <= 0);
        }

        op(_ITER_NEXT_RANGE, (iter -- iter, next)) {
            _PyRangeIterObject *r = (_PyRangeIterObject *)PyStackRef_AsPyObjectBorrow(iter);
            assert(Py_TYPE(r) == &PyRangeIter_Type);
            assert(r->len > 0);
            long value = r->start;
            r->start = value + r->step;
            r->len--;
            PyObject *res = PyLong_FromLong(value);
            ERROR_IF(res == NULL, error);
            next = PyStackRef_FromPyObjectSteal(res);
        }

        macro(FOR_ITER_RANGE) =
            unused/1 +  // Skip over the counter
            _ITER_CHECK_RANGE +
            _ITER_JUMP_RANGE +
            _ITER_NEXT_RANGE;

        op(_FOR_ITER_GEN_FRAME, (iter -- iter, gen_frame: _PyInterpreterFrame*)) {
            PyGenObject *gen = (PyGenObject *)PyStackRef_AsPyObjectBorrow(iter);
            DEOPT_IF(Py_TYPE(gen) != &PyGen_Type);
            DEOPT_IF(gen->gi_frame_state >= FRAME_EXECUTING);
            STAT_INC(FOR_ITER, hit);
            gen_frame = &gen->gi_iframe;
            _PyFrame_StackPush(gen_frame, PyStackRef_None);
            gen->gi_frame_state = FRAME_EXECUTING;
            gen->gi_exc_state.previous_item = tstate->exc_info;
            tstate->exc_info = &gen->gi_exc_state;
            gen_frame->previous = frame;
            // oparg is the return offset from the next instruction.
            frame->return_offset = (uint16_t)(1 + INLINE_CACHE_ENTRIES_FOR_ITER + oparg);
        }

        macro(FOR_ITER_GEN) =
            unused/1 +
            _CHECK_PEP_523 +
            _FOR_ITER_GEN_FRAME +
            _PUSH_FRAME;

        inst(LOAD_SPECIAL, (owner -- attr, self_or_null)) {
            assert(oparg <= SPECIAL_MAX);
            PyObject *owner_o = PyStackRef_AsPyObjectSteal(owner);
            PyObject *name = _Py_SpecialMethods[oparg].name;
            PyObject *self_or_null_o;
            attr = PyStackRef_FromPyObjectSteal(_PyObject_LookupSpecialMethod(owner_o, name, &self_or_null_o));
            if (PyStackRef_IsNull(attr)) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  _Py_SpecialMethods[oparg].error,
                                  Py_TYPE(owner_o)->tp_name);
                }
            }
            ERROR_IF(PyStackRef_IsNull(attr), error);
            self_or_null = PyStackRef_FromPyObjectSteal(self_or_null_o);
        }

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
            tb = PyException_GetTraceback(val_o);
            if (tb == NULL) {
                tb = Py_None;
            }
            else {
                Py_DECREF(tb);
            }
            assert(PyStackRef_LongCheck(lasti));
            (void)lasti; // Shut up compiler warning if asserts are off
            PyObject *stack[5] = {NULL, PyStackRef_AsPyObjectBorrow(exit_self), exc, val_o, tb};
            int has_self = !PyStackRef_IsNull(exit_self);
            res = PyStackRef_FromPyObjectSteal(PyObject_Vectorcall(exit_func_o, stack + 2 - has_self,
                    (3 + has_self) | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL));
            ERROR_IF(PyStackRef_IsNull(res), error);
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

        inst(PUSH_EXC_INFO, (new_exc -- prev_exc, new_exc)) {

            _PyErr_StackItem *exc_info = tstate->exc_info;
            if (exc_info->exc_value != NULL) {
                prev_exc = PyStackRef_FromPyObjectSteal(exc_info->exc_value);
            }
            else {
                prev_exc = PyStackRef_None;
            }
            assert(PyStackRef_ExceptionInstanceCheck(new_exc));
            exc_info->exc_value = PyStackRef_AsPyObjectNew(new_exc);
        }

        op(_GUARD_DORV_VALUES_INST_ATTR_FROM_DICT, (owner -- owner)) {
            PyObject *owner_o = PyStackRef_AsPyObjectBorrow(owner);
            assert(Py_TYPE(owner_o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            DEOPT_IF(!_PyObject_InlineValues(owner_o)->valid);
        }

        op(_GUARD_KEYS_VERSION, (keys_version/2, owner -- owner)) {
            PyTypeObject *owner_cls = Py_TYPE(PyStackRef_AsPyObjectBorrow(owner));
            PyHeapTypeObject *owner_heap_type = (PyHeapTypeObject *)owner_cls;
            DEOPT_IF(owner_heap_type->ht_cached_keys->dk_version != keys_version);
        }

        split op(_LOAD_ATTR_METHOD_WITH_VALUES, (descr/4, owner -- attr, self if (1))) {
            assert(oparg & 1);
            /* Cached method object */
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyStackRef_FromPyObjectNew(descr);
            self = owner;
        }

        macro(LOAD_ATTR_METHOD_WITH_VALUES) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_DORV_VALUES_INST_ATTR_FROM_DICT +
            _GUARD_KEYS_VERSION +
            _LOAD_ATTR_METHOD_WITH_VALUES;

        op(_LOAD_ATTR_METHOD_NO_DICT, (descr/4, owner -- attr, self if (1))) {
            assert(oparg & 1);
            assert(Py_TYPE(PyStackRef_AsPyObjectBorrow(owner))->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyStackRef_FromPyObjectNew(descr);
            self = owner;
        }

        macro(LOAD_ATTR_METHOD_NO_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_METHOD_NO_DICT;

        op(_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES, (descr/4, owner -- attr, unused if (0))) {
            assert((oparg & 1) == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            DECREF_INPUTS();
            attr = PyStackRef_FromPyObjectNew(descr);
        }

        macro(LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            _GUARD_DORV_VALUES_INST_ATTR_FROM_DICT +
            _GUARD_KEYS_VERSION +
            _LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES;

        op(_LOAD_ATTR_NONDESCRIPTOR_NO_DICT, (descr/4, owner -- attr, unused if (0))) {
            assert((oparg & 1) == 0);
            assert(Py_TYPE(PyStackRef_AsPyObjectBorrow(owner))->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            DECREF_INPUTS();
            attr = PyStackRef_FromPyObjectNew(descr);
        }

        macro(LOAD_ATTR_NONDESCRIPTOR_NO_DICT) =
            unused/1 +
            _GUARD_TYPE_VERSION +
            unused/2 +
            _LOAD_ATTR_NONDESCRIPTOR_NO_DICT;

        op(_CHECK_ATTR_METHOD_LAZY_DICT, (dictoffset/1, owner -- owner)) {
            char *ptr = ((char *)PyStackRef_AsPyObjectBorrow(owner)) + MANAGED_DICT_OFFSET + dictoffset;
            PyObject *dict = *(PyObject **)ptr;
            /* This object has a __dict__, just not yet created */
            DEOPT_IF(dict != NULL);
        }

        op(_LOAD_ATTR_METHOD_LAZY_DICT, (descr/4, owner -- attr, self if (1))) {
            assert(oparg & 1);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            attr = PyStackRef_FromPyObjectNew(descr);
            self = owner;
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

        specializing op(_SPECIALIZE_CALL, (counter/1, callable, self_or_null[1], args[oparg] -- callable, self_or_null[1], args[oparg])) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_Call(callable, next_instr, oparg + !PyStackRef_IsNull(self_or_null[0]));
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CALL);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        op(_MAYBE_EXPAND_METHOD, (callable, self_or_null[1], args[oparg] -- func, maybe_self[1], args[oparg])) {
            if (PyStackRef_TYPE(callable) == &PyMethod_Type && PyStackRef_IsNull(self_or_null[0])) {
                PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
                PyObject *self = ((PyMethodObject *)callable_o)->im_self;
                maybe_self[0] = PyStackRef_FromPyObjectNew(self);
                PyObject *method = ((PyMethodObject *)callable_o)->im_func;
                func = PyStackRef_FromPyObjectNew(method);
                PyStackRef_CLOSE(callable);
            }
            else {
                func = callable;
            }
        }

        // When calling Python, inline the call using DISPATCH_INLINED().
        op(_DO_CALL, (callable, self_or_null[1], args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
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
                    args, total_args, NULL, frame
                );
                // Manipulate stack directly since we leave using DISPATCH_INLINED().
                STACK_SHRINK(oparg + 2);
                // The frame has stolen all the arguments from the stack,
                // so there is no need to clean them up.
                if (new_frame == NULL) {
                    ERROR_NO_POP();
                }
                frame->return_offset = (uint16_t)(next_instr - this_instr);
                DISPATCH_INLINED(new_frame);
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                PyStackRef_CLOSE(callable);
                for (int i = 0; i < total_args; i++) {
                    PyStackRef_CLOSE(args[i]);
                }
                ERROR_IF(true, error);
            }
            PyObject *res_o = PyObject_Vectorcall(
                callable_o, args_o,
                total_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            if (opcode == INSTRUMENTED_CALL) {
                PyObject *arg = total_args == 0 ?
                    &_PyInstrumentation_MISSING : PyStackRef_AsPyObjectBorrow(args[0]);
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
            PyStackRef_CLOSE(callable);
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_MONITOR_CALL, (func, maybe_self[1], args[oparg] -- func, maybe_self[1], args[oparg])) {
            int is_meth = !PyStackRef_IsNull(maybe_self[0]);
            PyObject *function = PyStackRef_AsPyObjectBorrow(func);
            PyObject *arg0;
            if (is_meth) {
                arg0 = PyStackRef_AsPyObjectBorrow(maybe_self[0]);
            }
            else if (oparg) {
                arg0 = PyStackRef_AsPyObjectBorrow(args[0]);
            }
            else {
                arg0 = &_PyInstrumentation_MISSING;
            }
            int err = _Py_call_instrumentation_2args(
                tstate, PY_MONITORING_EVENT_CALL,
                frame, this_instr, function, arg0
            );
            ERROR_IF(err, error);
        }

        macro(CALL) = _SPECIALIZE_CALL + unused/2 + _MAYBE_EXPAND_METHOD + _DO_CALL + _CHECK_PERIODIC;
        macro(INSTRUMENTED_CALL) = unused/3 + _MAYBE_EXPAND_METHOD + _MONITOR_CALL + _DO_CALL + _CHECK_PERIODIC;

        op(_PY_FRAME_GENERAL, (callable, self_or_null[1], args[oparg] -- new_frame: _PyInterpreterFrame*)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            assert(Py_TYPE(callable_o) == &PyFunction_Type);
            int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(callable_o))->co_flags;
            PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(callable_o));
            new_frame = _PyEvalFramePushAndInit(
                tstate, callable, locals,
                args, total_args, NULL, frame
            );
            // The frame has stolen all the arguments from the stack,
            // so there is no need to clean them up.
            SYNC_SP();
            if (new_frame == NULL) {
                ERROR_NO_POP();
            }
        }

        op(_CHECK_FUNCTION_VERSION, (func_version/2, callable, self_or_null[1], unused[oparg] -- callable, self_or_null[1], unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            EXIT_IF(!PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            EXIT_IF(func->func_version != func_version);
        }

        macro(CALL_PY_GENERAL) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION +
            _PY_FRAME_GENERAL +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        op(_CHECK_METHOD_VERSION, (func_version/2, callable, null[1], unused[oparg] -- callable, null[1], unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
            PyObject *func = ((PyMethodObject *)callable_o)->im_func;
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
            EXIT_IF(!PyStackRef_IsNull(null[0]));
        }

        op(_EXPAND_METHOD, (callable, null[1], unused[oparg] -- method, self[1], unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            assert(PyStackRef_IsNull(null[0]));
            assert(Py_TYPE(callable_o) == &PyMethod_Type);
            self[0] = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_self);
            method = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_func);
            assert(PyStackRef_FunctionCheck(method));
            PyStackRef_CLOSE(callable);
        }

        macro(CALL_BOUND_METHOD_GENERAL) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_METHOD_VERSION +
            _EXPAND_METHOD +
            flush + // so that self is in the argument array
            _PY_FRAME_GENERAL +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        op(_CHECK_IS_NOT_PY_CALLABLE, (callable, unused[1], unused[oparg] -- callable, unused[1], unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            EXIT_IF(PyFunction_Check(callable_o));
            EXIT_IF(Py_TYPE(callable_o) == &PyMethod_Type);
        }

        op(_CALL_NON_PY_GENERAL, (callable, self_or_null[1], args[oparg] -- res)) {
#if TIER_ONE
            assert(opcode != INSTRUMENTED_CALL);
#endif
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *res_o = PyObject_Vectorcall(
                callable_o, args_o,
                total_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            PyStackRef_CLOSE(callable);
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_NON_PY_GENERAL) =
            unused/1 + // Skip over the counter
            unused/2 +
            _CHECK_IS_NOT_PY_CALLABLE +
            _CALL_NON_PY_GENERAL +
            _CHECK_PERIODIC;

        op(_CHECK_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null[1], unused[oparg] -- callable, null[1], unused[oparg])) {
            EXIT_IF(!PyStackRef_IsNull(null[0]));
            EXIT_IF(Py_TYPE(PyStackRef_AsPyObjectBorrow(callable)) != &PyMethod_Type);
        }

        op(_INIT_CALL_BOUND_METHOD_EXACT_ARGS, (callable, null[1], unused[oparg] -- func, self[1], unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            STAT_INC(CALL, hit);
            self[0] = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_self);
            func = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_func);
            PyStackRef_CLOSE(callable);
        }

        op(_CHECK_PEP_523, (--)) {
            DEOPT_IF(tstate->interp->eval_frame);
        }

        op(_CHECK_FUNCTION_EXACT_ARGS, (callable, self_or_null[1], unused[oparg] -- callable, self_or_null[1], unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            assert(PyFunction_Check(callable_o));
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            EXIT_IF(code->co_argcount != oparg + (!PyStackRef_IsNull(self_or_null[0])));
        }

        op(_CHECK_STACK_SPACE, (callable, self_or_null[1], unused[oparg] -- callable, self_or_null[1], unused[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyFunctionObject *func = (PyFunctionObject *)callable_o;
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize));
            DEOPT_IF(tstate->py_recursion_remaining <= 1);
        }

        replicate(5) pure op(_INIT_CALL_PY_EXACT_ARGS, (callable, self_or_null[1], args[oparg] -- new_frame: _PyInterpreterFrame*)) {
            int has_self = !PyStackRef_IsNull(self_or_null[0]);
            STAT_INC(CALL, hit);
            new_frame = _PyFrame_PushUnchecked(tstate, callable, oparg + has_self, frame);
            _PyStackRef *first_non_self_local = new_frame->localsplus + has_self;
            new_frame->localsplus[0] = self_or_null[0];
            for (int i = 0; i < oparg; i++) {
                first_non_self_local[i] = args[i];
            }
        }

        op(_PUSH_FRAME, (new_frame: _PyInterpreterFrame* -- )) {
            // Write it out explicitly because it's subtly different.
            // Eventually this should be the only occurrence of this code.
            assert(tstate->interp->eval_frame == NULL);
            SYNC_SP();
            _PyFrame_SetStackPointer(frame, stack_pointer);
            assert(new_frame->previous == frame || new_frame->previous->previous == frame);
            CALL_STAT_INC(inlined_py_calls);
            frame = tstate->current_frame = new_frame;
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
            _INIT_CALL_PY_EXACT_ARGS +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        macro(CALL_PY_EXACT_ARGS) =
            unused/1 + // Skip over the counter
            _CHECK_PEP_523 +
            _CHECK_FUNCTION_VERSION +
            _CHECK_FUNCTION_EXACT_ARGS +
            _CHECK_STACK_SPACE +
            _INIT_CALL_PY_EXACT_ARGS +
            _SAVE_RETURN_OFFSET +
            _PUSH_FRAME;

        inst(CALL_TYPE_1, (unused/1, unused/2, callable, null, arg -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *arg_o = PyStackRef_AsPyObjectBorrow(arg);

            assert(oparg == 1);
            DEOPT_IF(!PyStackRef_IsNull(null));
            DEOPT_IF(callable_o != (PyObject *)&PyType_Type);
            STAT_INC(CALL, hit);
            res = PyStackRef_FromPyObjectSteal(Py_NewRef(Py_TYPE(arg_o)));
            PyStackRef_CLOSE(arg);
        }

        op(_CALL_STR_1, (callable, null, arg -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *arg_o = PyStackRef_AsPyObjectBorrow(arg);

            assert(oparg == 1);
            DEOPT_IF(!PyStackRef_IsNull(null));
            DEOPT_IF(callable_o != (PyObject *)&PyUnicode_Type);
            STAT_INC(CALL, hit);
            res = PyStackRef_FromPyObjectSteal(PyObject_Str(arg_o));
            PyStackRef_CLOSE(arg);
            ERROR_IF(PyStackRef_IsNull(res), error);
        }

        macro(CALL_STR_1) =
            unused/1 +
            unused/2 +
            _CALL_STR_1 +
            _CHECK_PERIODIC;

        op(_CALL_TUPLE_1, (callable, null, arg -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *arg_o = PyStackRef_AsPyObjectBorrow(arg);

            assert(oparg == 1);
            DEOPT_IF(!PyStackRef_IsNull(null));
            DEOPT_IF(callable_o != (PyObject *)&PyTuple_Type);
            STAT_INC(CALL, hit);
            res = PyStackRef_FromPyObjectSteal(PySequence_Tuple(arg_o));
            PyStackRef_CLOSE(arg);
            ERROR_IF(PyStackRef_IsNull(res), error);
        }

        macro(CALL_TUPLE_1) =
            unused/1 +
            unused/2 +
            _CALL_TUPLE_1 +
            _CHECK_PERIODIC;

        op(_CHECK_AND_ALLOCATE_OBJECT, (type_version/2, callable, null, args[oparg] -- self, init, args[oparg])) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            DEOPT_IF(!PyStackRef_IsNull(null));
            DEOPT_IF(!PyType_Check(callable_o));
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            DEOPT_IF(tp->tp_version_tag != type_version);
            assert(tp->tp_flags & Py_TPFLAGS_INLINE_VALUES);
            PyHeapTypeObject *cls = (PyHeapTypeObject *)callable_o;
            PyFunctionObject *init_func = (PyFunctionObject *)cls->_spec_cache.init;
            PyCodeObject *code = (PyCodeObject *)init_func->func_code;
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize + _Py_InitCleanup.co_framesize));
            STAT_INC(CALL, hit);
            self = PyStackRef_FromPyObjectSteal(_PyType_NewManagedObject(tp));
            if (PyStackRef_IsNull(self)) {
                ERROR_NO_POP();
            }
            PyStackRef_CLOSE(callable);
            init = PyStackRef_FromPyObjectNew(init_func);
        }

        op(_CREATE_INIT_FRAME, (self, init, args[oparg] -- init_frame: _PyInterpreterFrame *)) {
            _PyInterpreterFrame *shim = _PyFrame_PushTrampolineUnchecked(
                tstate, (PyCodeObject *)&_Py_InitCleanup, 1, frame);
            assert(_PyCode_CODE(_PyFrame_GetCode(shim))[0].op.code == EXIT_INIT_CHECK);
            /* Push self onto stack of shim */
            shim->localsplus[0] = PyStackRef_DUP(self);
            args[-1] = self;
            init_frame = _PyEvalFramePushAndInit(
                tstate, init, NULL, args-1, oparg+1, NULL, shim);
            SYNC_SP();
            if (init_frame == NULL) {
                _PyEval_FrameClearAndPop(tstate, shim);
                ERROR_NO_POP();
            }
            frame->return_offset = 1 + INLINE_CACHE_ENTRIES_CALL;
            /* Account for pushing the extra frame.
             * We don't check recursion depth here,
             * as it will be checked after start_frame */
            tstate->py_recursion_remaining--;
        }

        macro(CALL_ALLOC_AND_ENTER_INIT) =
            unused/1 +
            _CHECK_PEP_523 +
            _CHECK_AND_ALLOCATE_OBJECT +
            _CREATE_INIT_FRAME +
            _PUSH_FRAME;

        inst(EXIT_INIT_CHECK, (should_be_none -- )) {
            assert(STACK_LEVEL() == 2);
            if (!PyStackRef_Is(should_be_none, PyStackRef_None)) {
                PyErr_Format(PyExc_TypeError,
                    "__init__() should return None, not '%.200s'",
                    Py_TYPE(PyStackRef_AsPyObjectBorrow(should_be_none))->tp_name);
                ERROR_NO_POP();
            }
        }

        op(_CALL_BUILTIN_CLASS, (callable, self_or_null[1], args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            DEOPT_IF(!PyType_Check(callable_o));
            PyTypeObject *tp = (PyTypeObject *)callable_o;
            DEOPT_IF(tp->tp_vectorcall == NULL);
            STAT_INC(CALL, hit);
            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *res_o = tp->tp_vectorcall((PyObject *)tp, args_o, total_args, NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_CLASS) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_CLASS +
            _CHECK_PERIODIC;

        op(_CALL_BUILTIN_O, (callable, self_or_null[1], args[oparg] -- res)) {
            /* Builtin METH_O functions */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            EXIT_IF(total_args != 1);
            EXIT_IF(!PyCFunction_CheckExact(callable_o));
            EXIT_IF(PyCFunction_GET_FLAGS(callable_o) != METH_O);
            // CPython promises to check all non-vectorcall function calls.
            EXIT_IF(tstate->c_recursion_remaining <= 0);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable_o);
            _PyStackRef arg = args[0];
            _Py_EnterRecursiveCallTstateUnchecked(tstate);
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, PyCFunction_GET_SELF(callable_o), PyStackRef_AsPyObjectBorrow(arg));
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            PyStackRef_CLOSE(arg);
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_O) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_O +
            _CHECK_PERIODIC;

        op(_CALL_BUILTIN_FAST, (callable, self_or_null[1], args[oparg] -- res)) {
            /* Builtin METH_FASTCALL functions, without keywords */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            DEOPT_IF(!PyCFunction_CheckExact(callable_o));
            DEOPT_IF(PyCFunction_GET_FLAGS(callable_o) != METH_FASTCALL);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable_o);
            /* res = func(self, args, nargs) */
            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *res_o = ((PyCFunctionFast)(void(*)(void))cfunc)(
                PyCFunction_GET_SELF(callable_o),
                args_o,
                total_args);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_FAST) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_FAST +
            _CHECK_PERIODIC;

        op(_CALL_BUILTIN_FAST_WITH_KEYWORDS, (callable, self_or_null[1], args[oparg] -- res)) {
            /* Builtin METH_FASTCALL | METH_KEYWORDS functions */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            DEOPT_IF(!PyCFunction_CheckExact(callable_o));
            DEOPT_IF(PyCFunction_GET_FLAGS(callable_o) != (METH_FASTCALL | METH_KEYWORDS));
            STAT_INC(CALL, hit);
            /* res = func(self, args, nargs, kwnames) */
            PyCFunctionFastWithKeywords cfunc =
                (PyCFunctionFastWithKeywords)(void(*)(void))
                PyCFunction_GET_FUNCTION(callable_o);

            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *res_o = cfunc(PyCFunction_GET_SELF(callable_o), args_o, total_args, NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);

            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_BUILTIN_FAST_WITH_KEYWORDS) =
            unused/1 +
            unused/2 +
            _CALL_BUILTIN_FAST_WITH_KEYWORDS +
            _CHECK_PERIODIC;

        inst(CALL_LEN, (unused/1, unused/2, callable, self_or_null[1], args[oparg] -- res)) {
            /* len(o) */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            DEOPT_IF(total_args != 1);
            PyInterpreterState *interp = tstate->interp;
            DEOPT_IF(callable_o != interp->callable_cache.len);
            STAT_INC(CALL, hit);
            _PyStackRef arg_stackref = args[0];
            PyObject *arg = PyStackRef_AsPyObjectBorrow(arg_stackref);
            Py_ssize_t len_i = PyObject_Length(arg);
            if (len_i < 0) {
                ERROR_NO_POP();
            }
            PyObject *res_o = PyLong_FromSsize_t(len_i);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            if (res_o == NULL) {
                GOTO_ERROR(error);
            }
            PyStackRef_CLOSE(callable);
            PyStackRef_CLOSE(arg_stackref);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        inst(CALL_ISINSTANCE, (unused/1, unused/2, callable, self_or_null[1], args[oparg] -- res)) {
            /* isinstance(o, o2) */
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            DEOPT_IF(total_args != 2);
            PyInterpreterState *interp = tstate->interp;
            DEOPT_IF(callable_o != interp->callable_cache.isinstance);
            STAT_INC(CALL, hit);
            _PyStackRef cls_stackref = args[1];
            _PyStackRef inst_stackref = args[0];
            int retval = PyObject_IsInstance(PyStackRef_AsPyObjectBorrow(inst_stackref), PyStackRef_AsPyObjectBorrow(cls_stackref));
            if (retval < 0) {
                ERROR_NO_POP();
            }
            res = retval ? PyStackRef_True : PyStackRef_False;
            assert((!PyStackRef_IsNull(res)) ^ (_PyErr_Occurred(tstate) != NULL));
            PyStackRef_CLOSE(inst_stackref);
            PyStackRef_CLOSE(cls_stackref);
            PyStackRef_CLOSE(callable);
        }

        // This is secretly a super-instruction
        inst(CALL_LIST_APPEND, (unused/1, unused/2, callable, self, arg -- )) {
            assert(oparg == 1);
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *self_o = PyStackRef_AsPyObjectBorrow(self);

            PyInterpreterState *interp = tstate->interp;
            DEOPT_IF(callable_o != interp->callable_cache.list_append);
            assert(self_o != NULL);
            DEOPT_IF(!PyList_Check(self_o));
            STAT_INC(CALL, hit);
            int err = _PyList_AppendTakeRef((PyListObject *)self_o, PyStackRef_AsPyObjectSteal(arg));
            PyStackRef_CLOSE(self);
            PyStackRef_CLOSE(callable);
            ERROR_IF(err, error);
        #if TIER_ONE
            // Skip the following POP_TOP. This is done here in tier one, and
            // during trace projection in tier two:
            assert(next_instr->op.code == POP_TOP);
            SKIP_OVER(1);
        #endif
        }

         op(_CALL_METHOD_DESCRIPTOR_O, (callable, self_or_null[1], args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }

            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(total_args != 2);
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            PyMethodDef *meth = method->d_method;
            EXIT_IF(meth->ml_flags != METH_O);
            // CPython promises to check all non-vectorcall function calls.
            EXIT_IF(tstate->c_recursion_remaining <= 0);
            _PyStackRef arg_stackref = args[1];
            _PyStackRef self_stackref = args[0];
            EXIT_IF(!Py_IS_TYPE(PyStackRef_AsPyObjectBorrow(self_stackref),
                                 method->d_common.d_type));
            STAT_INC(CALL, hit);
            PyCFunction cfunc = meth->ml_meth;
            _Py_EnterRecursiveCallTstateUnchecked(tstate);
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc,
                                  PyStackRef_AsPyObjectBorrow(self_stackref),
                                  PyStackRef_AsPyObjectBorrow(arg_stackref));
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            PyStackRef_CLOSE(self_stackref);
            PyStackRef_CLOSE(arg_stackref);
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_O) =
            unused/1 +
            unused/2 +
            _CALL_METHOD_DESCRIPTOR_O +
            _CHECK_PERIODIC;

        op(_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, (callable, self_or_null[1], args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            PyMethodDef *meth = method->d_method;
            EXIT_IF(meth->ml_flags != (METH_FASTCALL|METH_KEYWORDS));
            PyTypeObject *d_type = method->d_common.d_type;
            PyObject *self = PyStackRef_AsPyObjectBorrow(args[0]);
            EXIT_IF(!Py_IS_TYPE(self, d_type));
            STAT_INC(CALL, hit);
            int nargs = total_args - 1;
            PyCFunctionFastWithKeywords cfunc =
                (PyCFunctionFastWithKeywords)(void(*)(void))meth->ml_meth;

            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *res_o = cfunc(self, (args_o + 1), nargs, NULL);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS) =
            unused/1 +
            unused/2 +
            _CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS +
            _CHECK_PERIODIC;

        op(_CALL_METHOD_DESCRIPTOR_NOARGS, (callable, self_or_null[1], args[oparg] -- res)) {
            assert(oparg == 0 || oparg == 1);
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
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
            EXIT_IF(tstate->c_recursion_remaining <= 0);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = meth->ml_meth;
            _Py_EnterRecursiveCallTstateUnchecked(tstate);
            PyObject *res_o = _PyCFunction_TrampolineCall(cfunc, self, NULL);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            PyStackRef_CLOSE(self_stackref);
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_METHOD_DESCRIPTOR_NOARGS) =
            unused/1 +
            unused/2 +
            _CALL_METHOD_DESCRIPTOR_NOARGS +
            _CHECK_PERIODIC;

        op(_CALL_METHOD_DESCRIPTOR_FAST, (callable, self_or_null[1], args[oparg] -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            PyMethodDescrObject *method = (PyMethodDescrObject *)callable_o;
            /* Builtin METH_FASTCALL methods, without keywords */
            EXIT_IF(!Py_IS_TYPE(method, &PyMethodDescr_Type));
            PyMethodDef *meth = method->d_method;
            EXIT_IF(meth->ml_flags != METH_FASTCALL);
            PyObject *self = PyStackRef_AsPyObjectBorrow(args[0]);
            EXIT_IF(!Py_IS_TYPE(self, method->d_common.d_type));
            STAT_INC(CALL, hit);
            PyCFunctionFast cfunc =
                (PyCFunctionFast)(void(*)(void))meth->ml_meth;
            int nargs = total_args - 1;

            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *res_o = cfunc(self, (args_o + 1), nargs);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Clear the stack of the arguments. */
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            PyStackRef_CLOSE(callable);
            ERROR_IF(res_o == NULL, error);
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

        inst(INSTRUMENTED_CALL_KW, (counter/1, version/2 -- )) {
            int is_meth = !PyStackRef_IsNull(PEEK(oparg + 2));
            int total_args = oparg + is_meth;
            PyObject *function = PyStackRef_AsPyObjectBorrow(PEEK(oparg + 3));
            PyObject *arg = total_args == 0 ? &_PyInstrumentation_MISSING
                                            : PyStackRef_AsPyObjectBorrow(PEEK(total_args + 1));
            int err = _Py_call_instrumentation_2args(
                    tstate, PY_MONITORING_EVENT_CALL,
                    frame, this_instr, function, arg);
            ERROR_IF(err, error);
            PAUSE_ADAPTIVE_COUNTER(this_instr[1].counter);
            GO_TO_INSTRUCTION(CALL_KW);
        }

        op(_DO_CALL_KW, (callable, self_or_null[1], args[oparg], kwnames -- res)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            PyObject *kwnames_o = PyStackRef_AsPyObjectBorrow(kwnames);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            else if (Py_TYPE(callable_o) == &PyMethod_Type) {
                args--;
                total_args++;
                PyObject *self = ((PyMethodObject *)callable_o)->im_self;
                args[0] = PyStackRef_FromPyObjectNew(self);
                PyObject *method = ((PyMethodObject *)callable_o)->im_func;
                args[-1] = PyStackRef_FromPyObjectNew(method);
                PyStackRef_CLOSE(callable);
                callable_o = method;
                callable = args[-1];
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
                    args, positional_args, kwnames_o, frame
                );
                PyStackRef_CLOSE(kwnames);
                // Manipulate stack directly since we leave using DISPATCH_INLINED().
                STACK_SHRINK(oparg + 3);
                // The frame has stolen all the arguments from the stack,
                // so there is no need to clean them up.
                if (new_frame == NULL) {
                    ERROR_NO_POP();
                }
                assert(next_instr - this_instr == 1 + INLINE_CACHE_ENTRIES_CALL_KW);
                frame->return_offset = 1 + INLINE_CACHE_ENTRIES_CALL_KW;
                DISPATCH_INLINED(new_frame);
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            PyObject *res_o = PyObject_Vectorcall(
                callable_o, args_o,
                positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                kwnames_o);
            STACKREFS_TO_PYOBJECTS_CLEANUP(args_o);
            if (opcode == INSTRUMENTED_CALL_KW) {
                PyObject *arg = total_args == 0 ?
                    &_PyInstrumentation_MISSING : PyStackRef_AsPyObjectBorrow(args[0]);
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
            PyStackRef_CLOSE(kwnames);
            assert((res_o != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            PyStackRef_CLOSE(callable);
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        op(_PY_FRAME_KW, (callable, self_or_null[1], args[oparg], kwnames -- new_frame: _PyInterpreterFrame*)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            // oparg counts all of the args, but *not* self:
            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            PyObject *kwnames_o = PyStackRef_AsPyObjectBorrow(kwnames);
            int positional_args = total_args - (int)PyTuple_GET_SIZE(kwnames_o);
            assert(Py_TYPE(callable_o) == &PyFunction_Type);
            int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(callable_o))->co_flags;
            PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(callable_o));
            new_frame = _PyEvalFramePushAndInit(
                tstate, callable, locals,
                args, positional_args, kwnames_o, frame
            );
            PyStackRef_CLOSE(kwnames);
            // The frame has stolen all the arguments from the stack,
            // so there is no need to clean them up.
            SYNC_SP();
            if (new_frame == NULL) {
                ERROR_NO_POP();
            }
        }

        op(_CHECK_FUNCTION_VERSION_KW, (func_version/2, callable, self_or_null[1], unused[oparg], kwnames -- callable, self_or_null[1], unused[oparg], kwnames)) {
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

        op(_CHECK_METHOD_VERSION_KW, (func_version/2, callable, null[1], unused[oparg], kwnames -- callable, null[1], unused[oparg], kwnames)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            EXIT_IF(Py_TYPE(callable_o) != &PyMethod_Type);
            PyObject *func = ((PyMethodObject *)callable_o)->im_func;
            EXIT_IF(!PyFunction_Check(func));
            EXIT_IF(((PyFunctionObject *)func)->func_version != func_version);
            EXIT_IF(!PyStackRef_IsNull(null[0]));
        }

        op(_EXPAND_METHOD_KW, (callable, null[1], unused[oparg], kwnames -- method, self[1], unused[oparg], kwnames)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            assert(PyStackRef_IsNull(null[0]));
            assert(Py_TYPE(callable_o) == &PyMethod_Type);
            self[0] = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_self);
            method = PyStackRef_FromPyObjectNew(((PyMethodObject *)callable_o)->im_func);
            assert(PyStackRef_FunctionCheck(method));
            PyStackRef_CLOSE(callable);
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

        specializing op(_SPECIALIZE_CALL_KW, (counter/1, callable, self_or_null[1], args[oparg], kwnames -- callable, self_or_null[1], args[oparg], kwnames)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_CallKw(callable, next_instr, oparg + !PyStackRef_IsNull(self_or_null[0]));
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(CALL_KW);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
        }

        macro(CALL_KW) =
            _SPECIALIZE_CALL_KW +
            unused/2 +
            _DO_CALL_KW;

        op(_CHECK_IS_NOT_PY_CALLABLE_KW, (callable, unused[1], unused[oparg], kwnames -- callable, unused[1], unused[oparg], kwnames)) {
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);
            EXIT_IF(PyFunction_Check(callable_o));
            EXIT_IF(Py_TYPE(callable_o) == &PyMethod_Type);
        }


        op(_CALL_KW_NON_PY, (callable, self_or_null[1], args[oparg], kwnames -- res)) {
#if TIER_ONE
            assert(opcode != INSTRUMENTED_CALL);
#endif
            PyObject *callable_o = PyStackRef_AsPyObjectBorrow(callable);

            int total_args = oparg;
            if (!PyStackRef_IsNull(self_or_null[0])) {
                args--;
                total_args++;
            }
            /* Callable is not a normal Python function */
            STACKREFS_TO_PYOBJECTS(args, total_args, args_o);
            if (CONVERSION_FAILED(args_o)) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
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
            PyStackRef_CLOSE(callable);
            for (int i = 0; i < total_args; i++) {
                PyStackRef_CLOSE(args[i]);
            }
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(CALL_KW_NON_PY) =
            unused/1 + // Skip over the counter
            unused/2 +
            _CHECK_IS_NOT_PY_CALLABLE_KW +
            _CALL_KW_NON_PY +
            _CHECK_PERIODIC;

        inst(INSTRUMENTED_CALL_FUNCTION_EX, ( -- )) {
            GO_TO_INSTRUCTION(CALL_FUNCTION_EX);
        }

        inst(_DO_CALL_FUNCTION_EX, (func_st, unused, callargs_st, kwargs_st if (oparg & 1) -- result)) {
            PyObject *func = PyStackRef_AsPyObjectBorrow(func_st);
            PyObject *callargs = PyStackRef_AsPyObjectBorrow(callargs_st);
            PyObject *kwargs = PyStackRef_AsPyObjectBorrow(kwargs_st);

            // DICT_MERGE is called before this opcode if there are kwargs.
            // It converts all dict subtypes in kwargs into regular dicts.
            assert(kwargs == NULL || PyDict_CheckExact(kwargs));
            if (!PyTuple_CheckExact(callargs)) {
                int err = check_args_iterable(tstate, func, callargs);
                if (err < 0) {
                    ERROR_NO_POP();
                }
                PyObject *tuple = PySequence_Tuple(callargs);
                if (tuple == NULL) {
                    ERROR_NO_POP();
                }
                PyStackRef_CLOSE(callargs_st);
                callargs_st = PyStackRef_FromPyObjectSteal(tuple);
                callargs = tuple;
            }
            assert(PyTuple_CheckExact(callargs));
            EVAL_CALL_STAT_INC_IF_FUNCTION(EVAL_CALL_FUNCTION_EX, func);
            if (opcode == INSTRUMENTED_CALL_FUNCTION_EX) {
                PyObject *arg = PyTuple_GET_SIZE(callargs) > 0 ?
                    PyTuple_GET_ITEM(callargs, 0) : &_PyInstrumentation_MISSING;
                int err = _Py_call_instrumentation_2args(
                    tstate, PY_MONITORING_EVENT_CALL,
                    frame, this_instr, func, arg);
                if (err) ERROR_NO_POP();
                result = PyStackRef_FromPyObjectSteal(PyObject_Call(func, callargs, kwargs));

                if (!PyFunction_Check(func) && !PyMethod_Check(func)) {
                    if (PyStackRef_IsNull(result)) {
                        _Py_call_instrumentation_exc2(
                            tstate, PY_MONITORING_EVENT_C_RAISE,
                            frame, this_instr, func, arg);
                    }
                    else {
                        int err = _Py_call_instrumentation_2args(
                            tstate, PY_MONITORING_EVENT_C_RETURN,
                            frame, this_instr, func, arg);
                        if (err < 0) {
                            PyStackRef_CLEAR(result);
                        }
                    }
                }
            }
            else {
                if (Py_TYPE(func) == &PyFunction_Type &&
                    tstate->interp->eval_frame == NULL &&
                    ((PyFunctionObject *)func)->vectorcall == _PyFunction_Vectorcall) {
                    assert(PyTuple_CheckExact(callargs));
                    Py_ssize_t nargs = PyTuple_GET_SIZE(callargs);
                    int code_flags = ((PyCodeObject *)PyFunction_GET_CODE(func))->co_flags;
                    PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(func));

                    _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit_Ex(
                        tstate, func_st, locals,
                        nargs, callargs, kwargs, frame);
                    // Need to manually shrink the stack since we exit with DISPATCH_INLINED.
                    STACK_SHRINK(oparg + 3);
                    if (new_frame == NULL) {
                        ERROR_NO_POP();
                    }
                    assert(next_instr - this_instr == 1);
                    frame->return_offset = 1;
                    DISPATCH_INLINED(new_frame);
                }
                result = PyStackRef_FromPyObjectSteal(PyObject_Call(func, callargs, kwargs));
            }
            DECREF_INPUTS();
            assert(PyStackRef_AsPyObjectBorrow(PEEK(2 + (oparg & 1))) == NULL);
            ERROR_IF(PyStackRef_IsNull(result), error);
        }

        macro(CALL_FUNCTION_EX) =
            _DO_CALL_FUNCTION_EX +
            _CHECK_PERIODIC;


        inst(MAKE_FUNCTION, (codeobj_st -- func)) {
            PyObject *codeobj = PyStackRef_AsPyObjectBorrow(codeobj_st);

            PyFunctionObject *func_obj = (PyFunctionObject *)
                PyFunction_New(codeobj, GLOBALS());

            PyStackRef_CLOSE(codeobj_st);
            if (func_obj == NULL) {
                ERROR_NO_POP();
            }

            _PyFunction_SetVersion(
                func_obj, ((PyCodeObject *)codeobj)->co_version);
            func = PyStackRef_FromPyObjectSteal((PyObject *)func_obj);
        }

        inst(SET_FUNCTION_ATTRIBUTE, (attr_st, func_st -- func_st)) {
            PyObject *func = PyStackRef_AsPyObjectBorrow(func_st);
            PyObject *attr = PyStackRef_AsPyObjectBorrow(attr_st);

            assert(PyFunction_Check(func));
            PyFunctionObject *func_obj = (PyFunctionObject *)func;
            switch(oparg) {
                case MAKE_FUNCTION_CLOSURE:
                    assert(func_obj->func_closure == NULL);
                    func_obj->func_closure = attr;
                    break;
                case MAKE_FUNCTION_ANNOTATIONS:
                    assert(func_obj->func_annotations == NULL);
                    func_obj->func_annotations = attr;
                    break;
                case MAKE_FUNCTION_KWDEFAULTS:
                    assert(PyDict_CheckExact(attr));
                    assert(func_obj->func_kwdefaults == NULL);
                    func_obj->func_kwdefaults = attr;
                    break;
                case MAKE_FUNCTION_DEFAULTS:
                    assert(PyTuple_CheckExact(attr));
                    assert(func_obj->func_defaults == NULL);
                    func_obj->func_defaults = attr;
                    break;
                case MAKE_FUNCTION_ANNOTATE:
                    assert(PyCallable_Check(attr));
                    assert(func_obj->func_annotate == NULL);
                    func_obj->func_annotate = attr;
                    break;
                default:
                    Py_UNREACHABLE();
            }
        }

        inst(RETURN_GENERATOR, (-- res)) {
            assert(PyStackRef_FunctionCheck(frame->f_funcobj));
            PyFunctionObject *func = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(frame->f_funcobj);
            PyGenObject *gen = (PyGenObject *)_Py_MakeCoro(func);
            if (gen == NULL) {
                ERROR_NO_POP();
            }
            assert(EMPTY());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyInterpreterFrame *gen_frame = &gen->gi_iframe;
            frame->instr_ptr++;
            _PyFrame_Copy(frame, gen_frame);
            assert(frame->frame_obj == NULL);
            gen->gi_frame_state = FRAME_CREATED;
            gen_frame->owner = FRAME_OWNED_BY_GENERATOR;
            _Py_LeaveRecursiveCallPy(tstate);
            res = PyStackRef_FromPyObjectSteal((PyObject *)gen);
            _PyInterpreterFrame *prev = frame->previous;
            _PyThreadState_PopFrame(tstate, frame);
            frame = tstate->current_frame = prev;
            LOAD_IP(frame->return_offset);
            LOAD_SP();
            LLTRACE_RESUME_FRAME();
        }

        inst(BUILD_SLICE, (start, stop, step if (oparg == 3) -- slice)) {
            PyObject *start_o = PyStackRef_AsPyObjectBorrow(start);
            PyObject *stop_o = PyStackRef_AsPyObjectBorrow(stop);
            PyObject *step_o = PyStackRef_AsPyObjectBorrow(step);

            PyObject *slice_o = PySlice_New(start_o, stop_o, step_o);
            DECREF_INPUTS();
            ERROR_IF(slice_o == NULL, error);
            slice = PyStackRef_FromPyObjectSteal(slice_o);
        }

        inst(CONVERT_VALUE, (value -- result)) {
            conversion_func conv_fn;
            assert(oparg >= FVC_STR && oparg <= FVC_ASCII);
            conv_fn = _PyEval_ConversionFuncs[oparg];
            PyObject *result_o = conv_fn(PyStackRef_AsPyObjectBorrow(value));
            PyStackRef_CLOSE(value);
            ERROR_IF(result_o == NULL, error);
            result = PyStackRef_FromPyObjectSteal(result_o);
        }

        inst(FORMAT_SIMPLE, (value -- res)) {
            PyObject *value_o = PyStackRef_AsPyObjectBorrow(value);
            /* If value is a unicode object, then we know the result
             * of format(value) is value itself. */
            if (!PyUnicode_CheckExact(value_o)) {
                res = PyStackRef_FromPyObjectSteal(PyObject_Format(value_o, NULL));
                PyStackRef_CLOSE(value);
                ERROR_IF(PyStackRef_IsNull(res), error);
            }
            else {
                res = value;
            }
        }

        inst(FORMAT_WITH_SPEC, (value, fmt_spec -- res)) {
            PyObject *res_o = PyObject_Format(PyStackRef_AsPyObjectBorrow(value), PyStackRef_AsPyObjectBorrow(fmt_spec));
            PyStackRef_CLOSE(value);
            PyStackRef_CLOSE(fmt_spec);
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        pure inst(COPY, (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
            assert(oparg > 0);
            top = PyStackRef_DUP(bottom);
        }

        specializing op(_SPECIALIZE_BINARY_OP, (counter/1, lhs, rhs -- lhs, rhs)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_TRIGGERS(counter)) {
                next_instr = this_instr;
                _Py_Specialize_BinaryOp(lhs, rhs, next_instr, oparg, LOCALS_ARRAY);
                DISPATCH_SAME_OPARG();
            }
            OPCODE_DEFERRED_INC(BINARY_OP);
            ADVANCE_ADAPTIVE_COUNTER(this_instr[1].counter);
            #endif  /* ENABLE_SPECIALIZATION */
            assert(NB_ADD <= oparg);
            assert(oparg <= NB_INPLACE_XOR);
        }

        op(_BINARY_OP, (lhs, rhs -- res)) {
            PyObject *lhs_o = PyStackRef_AsPyObjectBorrow(lhs);
            PyObject *rhs_o = PyStackRef_AsPyObjectBorrow(rhs);

            assert(_PyEval_BinaryOps[oparg]);
            PyObject *res_o = _PyEval_BinaryOps[oparg](lhs_o, rhs_o);
            DECREF_INPUTS();
            ERROR_IF(res_o == NULL, error);
            res = PyStackRef_FromPyObjectSteal(res_o);
        }

        macro(BINARY_OP) = _SPECIALIZE_BINARY_OP + _BINARY_OP;

        pure inst(SWAP, (bottom, unused[oparg-2], top --
                    top, unused[oparg-2], bottom)) {
            assert(oparg >= 2);
        }

        inst(INSTRUMENTED_LINE, ( -- )) {
            int original_opcode = 0;
            if (tstate->tracing) {
                PyCodeObject *code = _PyFrame_GetCode(frame);
                original_opcode = code->_co_monitoring->lines[(int)(this_instr - _PyCode_CODE(code))].original_opcode;
                next_instr = this_instr;
            } else {
                _PyFrame_SetStackPointer(frame, stack_pointer);
                original_opcode = _Py_call_instrumentation_line(
                        tstate, frame, this_instr, prev_instr);
                stack_pointer = _PyFrame_GetStackPointer(frame);
                if (original_opcode < 0) {
                    next_instr = this_instr+1;
                    goto error;
                }
                next_instr = frame->instr_ptr;
                if (next_instr != this_instr) {
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
            ERROR_IF(next_opcode < 0, error);
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

        macro(INSTRUMENTED_JUMP_BACKWARD) =
            unused/1 +
            _CHECK_PERIODIC +
            _MONITOR_JUMP_BACKWARD;

        inst(INSTRUMENTED_POP_JUMP_IF_TRUE, (unused/1 -- )) {
            _PyStackRef cond = POP();
            assert(PyStackRef_BoolCheck(cond));
            int flag = PyStackRef_Is(cond, PyStackRef_True);
            int offset = flag * oparg;
            #if ENABLE_SPECIALIZATION
            this_instr[1].cache = (this_instr[1].cache << 1) | flag;
            #endif
            INSTRUMENTED_JUMP(this_instr, next_instr + offset, PY_MONITORING_EVENT_BRANCH);
        }

        inst(INSTRUMENTED_POP_JUMP_IF_FALSE, (unused/1 -- )) {
            _PyStackRef cond = POP();
            assert(PyStackRef_BoolCheck(cond));
            int flag = PyStackRef_Is(cond, PyStackRef_False);
            int offset = flag * oparg;
            #if ENABLE_SPECIALIZATION
            this_instr[1].cache = (this_instr[1].cache << 1) | flag;
            #endif
            INSTRUMENTED_JUMP(this_instr, next_instr + offset, PY_MONITORING_EVENT_BRANCH);
        }

        inst(INSTRUMENTED_POP_JUMP_IF_NONE, (unused/1 -- )) {
            _PyStackRef value_stackref = POP();
            int flag = PyStackRef_Is(value_stackref, PyStackRef_None);
            int offset;
            if (flag) {
                offset = oparg;
            }
            else {
                PyStackRef_CLOSE(value_stackref);
                offset = 0;
            }
            #if ENABLE_SPECIALIZATION
            this_instr[1].cache = (this_instr[1].cache << 1) | flag;
            #endif
            INSTRUMENTED_JUMP(this_instr, next_instr + offset, PY_MONITORING_EVENT_BRANCH);
        }

        inst(INSTRUMENTED_POP_JUMP_IF_NOT_NONE, (unused/1 -- )) {
            _PyStackRef value_stackref = POP();
            int offset;
            int nflag = PyStackRef_Is(value_stackref, PyStackRef_None);
            if (nflag) {
                offset = 0;
            }
            else {
                PyStackRef_CLOSE(value_stackref);
                offset = oparg;
            }
            #if ENABLE_SPECIALIZATION
            this_instr[1].cache = (this_instr[1].cache << 1) | !nflag;
            #endif
            INSTRUMENTED_JUMP(this_instr, next_instr + offset, PY_MONITORING_EVENT_BRANCH);
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
            SYNC_SP();
            EXIT_IF(!PyStackRef_Is(flag, PyStackRef_True));
            assert(PyStackRef_Is(flag, PyStackRef_True));
        }

        op (_GUARD_IS_FALSE_POP, (flag -- )) {
            SYNC_SP();
            EXIT_IF(!PyStackRef_Is(flag, PyStackRef_False));
            assert(PyStackRef_Is(flag, PyStackRef_False));
        }

        op (_GUARD_IS_NONE_POP, (val -- )) {
            SYNC_SP();
            if (!PyStackRef_Is(val, PyStackRef_None)) {
                PyStackRef_CLOSE(val);
                EXIT_IF(1);
            }
        }

        op (_GUARD_IS_NOT_NONE_POP, (val -- )) {
            SYNC_SP();
            EXIT_IF(PyStackRef_Is(val, PyStackRef_None));
            PyStackRef_CLOSE(val);
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
            _Py_CODEUNIT *target = _PyCode_CODE(code) + exit->target;
        #if defined(Py_DEBUG) && !defined(_Py_JIT)
            OPT_HIST(trace_uop_execution_counter, trace_run_length_hist);
            if (lltrace >= 2) {
                printf("SIDE EXIT: [UOp ");
                _PyUOpPrint(&next_uop[-1]);
                printf(", exit %u, temp %d, target %d -> %s]\n",
                    exit - current_executor->exits, exit->temperature.as_counter,
                    (int)(target - _PyCode_CODE(code)),
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
                    tstate->previous_executor = (PyObject *)current_executor;
                    GOTO_TIER_ONE(target);
                }
                _PyExecutorObject *executor;
                if (target->op.code == ENTER_EXECUTOR) {
                    executor = code->co_executors->executors[target->op.arg];
                    Py_INCREF(executor);
                }
                else {
                    int chain_depth = current_executor->vm_data.chain_depth + 1;
                    int optimized = _PyOptimizer_Optimize(frame, target, stack_pointer, &executor, chain_depth);
                    if (optimized <= 0) {
                        exit->temperature = restart_backoff_counter(temperature);
                        if (optimized < 0) {
                            GOTO_UNWIND();
                        }
                        tstate->previous_executor = (PyObject *)current_executor;
                        GOTO_TIER_ONE(target);
                    }
                }
                exit->executor = executor;
            }
            Py_INCREF(exit->executor);
            tstate->previous_executor = (PyObject *)current_executor;
            GOTO_TIER_TWO(exit->executor);
        }

        tier2 op(_CHECK_VALIDITY, (--)) {
            DEOPT_IF(!current_executor->vm_data.valid);
        }

        tier2 pure op(_LOAD_CONST_INLINE, (ptr/4 -- value)) {
            value = PyStackRef_FromPyObjectNew(ptr);
        }

        tier2 pure op(_LOAD_CONST_INLINE_BORROW, (ptr/4 -- value)) {
            value = PyStackRef_FromPyObjectImmortal(ptr);
        }

        tier2 pure op (_POP_TOP_LOAD_CONST_INLINE_BORROW, (ptr/4, pop -- value)) {
            PyStackRef_CLOSE(pop);
            value = PyStackRef_FromPyObjectImmortal(ptr);
        }

        tier2 pure op(_LOAD_CONST_INLINE_WITH_NULL, (ptr/4 -- value, null)) {
            value = PyStackRef_FromPyObjectNew(ptr);
            null = PyStackRef_NULL;
        }

        tier2 pure op(_LOAD_CONST_INLINE_BORROW_WITH_NULL, (ptr/4 -- value, null)) {
            value = PyStackRef_FromPyObjectImmortal(ptr);
            null = PyStackRef_NULL;
        }

        tier2 op(_CHECK_FUNCTION, (func_version/2 -- )) {
            assert(PyStackRef_FunctionCheck(frame->f_funcobj));
            PyFunctionObject *func = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(frame->f_funcobj);
            DEOPT_IF(func->func_version != func_version);
        }

        /* Internal -- for testing executors */
        op(_INTERNAL_INCREMENT_OPT_COUNTER, (opt --)) {
            _PyCounterOptimizerObject *exe = (_PyCounterOptimizerObject *)PyStackRef_AsPyObjectBorrow(opt);
            exe->count++;
        }

        tier2 op(_DYNAMIC_EXIT, (exit_p/4 --)) {
            tstate->previous_executor = (PyObject *)current_executor;
            _PyExitData *exit = (_PyExitData *)exit_p;
            _Py_CODEUNIT *target = frame->instr_ptr;
        #if defined(Py_DEBUG) && !defined(_Py_JIT)
            OPT_HIST(trace_uop_execution_counter, trace_run_length_hist);
            if (lltrace >= 2) {
                printf("DYNAMIC EXIT: [UOp ");
                _PyUOpPrint(&next_uop[-1]);
                printf(", exit %u, temp %d, target %d -> %s]\n",
                    exit - current_executor->exits, exit->temperature.as_counter,
                    (int)(target - _PyCode_CODE(_PyFrame_GetCode(frame))),
                    _PyOpcode_OpName[target->op.code]);
            }
        #endif
            _PyExecutorObject *executor;
            if (target->op.code == ENTER_EXECUTOR) {
                PyCodeObject *code = _PyFrame_GetCode(frame);
                executor = code->co_executors->executors[target->op.arg];
                Py_INCREF(executor);
            }
            else {
                if (!backoff_counter_triggers(exit->temperature)) {
                    exit->temperature = advance_backoff_counter(exit->temperature);
                    GOTO_TIER_ONE(target);
                }
                int optimized = _PyOptimizer_Optimize(frame, target, stack_pointer, &executor, 0);
                if (optimized <= 0) {
                    exit->temperature = restart_backoff_counter(exit->temperature);
                    if (optimized < 0) {
                        GOTO_UNWIND();
                    }
                    GOTO_TIER_ONE(target);
                }
                else {
                    exit->temperature = initial_temperature_backoff_counter();
                }
            }
            GOTO_TIER_TWO(executor);
        }

        tier2 op(_START_EXECUTOR, (executor/4 --)) {
            Py_DECREF(tstate->previous_executor);
            tstate->previous_executor = NULL;
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

        tier2 op(_CHECK_VALIDITY_AND_SET_IP, (instr_ptr/4 --)) {
            DEOPT_IF(!current_executor->vm_data.valid);
            frame->instr_ptr = (_Py_CODEUNIT *)instr_ptr;
        }

        tier2 op(_DEOPT, (--)) {
            EXIT_TO_TIER1();
        }

        tier2 op(_ERROR_POP_N, (target/2, unused[oparg] --)) {
            frame->instr_ptr = ((_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive) + target;
            SYNC_SP();
            GOTO_UNWIND();
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

// END BYTECODES //

    }
 dispatch_opcode:
 error:
 exception_unwind:
 exit_unwind:
 handle_eval_breaker:
 resume_frame:
 resume_with_error:
 start_frame:
 unbound_local_error:
    ;
}

// Future families go below this point //
