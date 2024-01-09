#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_bitutils.h"        // _Py_popcount32()
#include "pycore_opcode_metadata.h" // _PyOpcode_OpName[]
#include "pycore_opcode_utils.h"  // MAX_REAL_OPCODE
#include "pycore_optimizer.h"     // _Py_uop_analyze_and_optimize()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uop_ids.h"
#include "pycore_uops.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define NEED_OPCODE_METADATA
#include "pycore_uop_metadata.h" // Uop tables
#undef NEED_OPCODE_METADATA

#define MAX_EXECUTORS_SIZE 256


static bool
has_space_for_executor(PyCodeObject *code, _Py_CODEUNIT *instr)
{
    if (instr->op.code == ENTER_EXECUTOR) {
        return true;
    }
    if (code->co_executors == NULL) {
        return true;
    }
    return code->co_executors->size < MAX_EXECUTORS_SIZE;
}

static int32_t
get_index_for_executor(PyCodeObject *code, _Py_CODEUNIT *instr)
{
    if (instr->op.code == ENTER_EXECUTOR) {
        return instr->op.arg;
    }
    _PyExecutorArray *old = code->co_executors;
    int size = 0;
    int capacity = 0;
    if (old != NULL) {
        size = old->size;
        capacity = old->capacity;
        assert(size < MAX_EXECUTORS_SIZE);
    }
    assert(size <= capacity);
    if (size == capacity) {
        /* Array is full. Grow array */
        int new_capacity = capacity ? capacity * 2 : 4;
        _PyExecutorArray *new = PyMem_Realloc(
            old,
            offsetof(_PyExecutorArray, executors) +
            new_capacity * sizeof(_PyExecutorObject *));
        if (new == NULL) {
            return -1;
        }
        new->capacity = new_capacity;
        new->size = size;
        code->co_executors = new;
    }
    assert(size < code->co_executors->capacity);
    return size;
}

static void
insert_executor(PyCodeObject *code, _Py_CODEUNIT *instr, int index, _PyExecutorObject *executor)
{
    Py_INCREF(executor);
    if (instr->op.code == ENTER_EXECUTOR) {
        assert(index == instr->op.arg);
        _PyExecutorObject *old = code->co_executors->executors[index];
        executor->vm_data.opcode = old->vm_data.opcode;
        executor->vm_data.oparg = old->vm_data.oparg;
        old->vm_data.opcode = 0;
        code->co_executors->executors[index] = executor;
        Py_DECREF(old);
    }
    else {
        assert(code->co_executors->size == index);
        assert(code->co_executors->capacity > index);
        executor->vm_data.opcode = instr->op.code;
        executor->vm_data.oparg = instr->op.arg;
        code->co_executors->executors[index] = executor;
        assert(index < MAX_EXECUTORS_SIZE);
        instr->op.code = ENTER_EXECUTOR;
        instr->op.arg = index;
        code->co_executors->size++;
    }
    return;
}

int
PyUnstable_Replace_Executor(PyCodeObject *code, _Py_CODEUNIT *instr, _PyExecutorObject *new)
{
    if (instr->op.code != ENTER_EXECUTOR) {
        PyErr_Format(PyExc_ValueError, "No executor to replace");
        return -1;
    }
    int index = instr->op.arg;
    assert(index >= 0);
    insert_executor(code, instr, index, new);
    return 0;
}

static int
error_optimize(
    _PyOptimizerObject* self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyExecutorObject **exec,
    int Py_UNUSED(stack_entries))
{
    assert(0);
    PyErr_Format(PyExc_SystemError, "Should never call error_optimize");
    return -1;
}

PyTypeObject _PyDefaultOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "noop_optimizer",
    .tp_basicsize = sizeof(_PyOptimizerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
};

_PyOptimizerObject _PyOptimizer_Default = {
    PyObject_HEAD_INIT(&_PyDefaultOptimizer_Type)
    .optimize = error_optimize,
    .resume_threshold = INT16_MAX,
    .backedge_threshold = INT16_MAX,
};

_PyOptimizerObject *
PyUnstable_GetOptimizer(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->optimizer == &_PyOptimizer_Default) {
        return NULL;
    }
    assert(interp->optimizer_backedge_threshold == interp->optimizer->backedge_threshold);
    assert(interp->optimizer_resume_threshold == interp->optimizer->resume_threshold);
    Py_INCREF(interp->optimizer);
    return interp->optimizer;
}

void
PyUnstable_SetOptimizer(_PyOptimizerObject *optimizer)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (optimizer == NULL) {
        optimizer = &_PyOptimizer_Default;
    }
    _PyOptimizerObject *old = interp->optimizer;
    Py_INCREF(optimizer);
    interp->optimizer = optimizer;
    interp->optimizer_backedge_threshold = optimizer->backedge_threshold;
    interp->optimizer_resume_threshold = optimizer->resume_threshold;
    Py_DECREF(old);
}

int
_PyOptimizer_BackEdge(_PyInterpreterFrame *frame, _Py_CODEUNIT *src, _Py_CODEUNIT *dest, PyObject **stack_pointer)
{
    assert(src->op.code == JUMP_BACKWARD);
    PyCodeObject *code = (PyCodeObject *)frame->f_executable;
    assert(PyCode_Check(code));
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!has_space_for_executor(code, src)) {
        return 0;
    }
    _PyOptimizerObject *opt = interp->optimizer;
    _PyExecutorObject *executor = NULL;
    /* Start optimizing at the destination to guarantee forward progress */
    int err = opt->optimize(opt, code, dest, &executor, (int)(stack_pointer - _PyFrame_Stackbase(frame)));
    if (err <= 0) {
        assert(executor == NULL);
        return err;
    }
    int index = get_index_for_executor(code, src);
    if (index < 0) {
        /* Out of memory. Don't raise and assume that the
         * error will show up elsewhere.
         *
         * If an optimizer has already produced an executor,
         * it might get confused by the executor disappearing,
         * but there is not much we can do about that here. */
        Py_DECREF(executor);
        return 0;
    }
    insert_executor(code, src, index, executor);
    Py_DECREF(executor);
    return 1;
}

_PyExecutorObject *
PyUnstable_GetExecutor(PyCodeObject *code, int offset)
{
    int code_len = (int)Py_SIZE(code);
    for (int i = 0 ; i < code_len;) {
        if (_PyCode_CODE(code)[i].op.code == ENTER_EXECUTOR && i*2 == offset) {
            int oparg = _PyCode_CODE(code)[i].op.arg;
            _PyExecutorObject *res = code->co_executors->executors[oparg];
            Py_INCREF(res);
            return res;
        }
        i += _PyInstruction_GetLength(code, i);
    }
    PyErr_SetString(PyExc_ValueError, "no executor at given byte offset");
    return NULL;
}

/** Test support **/


typedef struct {
    _PyOptimizerObject base;
    int64_t count;
} _PyCounterOptimizerObject;

typedef struct {
    _PyExecutorObject executor;
    _PyCounterOptimizerObject *optimizer;
    _Py_CODEUNIT *next_instr;
} _PyCounterExecutorObject;

static void
counter_dealloc(_PyCounterExecutorObject *self) {
    _Py_ExecutorClear((_PyExecutorObject *)self);
    Py_DECREF(self->optimizer);
    PyObject_Free(self);
}

static PyObject *
is_valid(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyBool_FromLong(((_PyExecutorObject *)self)->vm_data.valid);
}

static PyMethodDef executor_methods[] = {
    { "is_valid", is_valid, METH_NOARGS, NULL },
    { NULL, NULL },
};

PyTypeObject _PyCounterExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "counting_executor",
    .tp_basicsize = sizeof(_PyCounterExecutorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)counter_dealloc,
    .tp_methods = executor_methods,
};

static _Py_CODEUNIT *
counter_execute(_PyExecutorObject *self, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    ((_PyCounterExecutorObject *)self)->optimizer->count++;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    Py_DECREF(self);
    return ((_PyCounterExecutorObject *)self)->next_instr;
}

static int
counter_optimize(
    _PyOptimizerObject* self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyExecutorObject **exec_ptr,
    int Py_UNUSED(curr_stackentries)
)
{
    _PyCounterExecutorObject *executor = (_PyCounterExecutorObject *)_PyObject_New(&_PyCounterExecutor_Type);
    if (executor == NULL) {
        return -1;
    }
    executor->executor.execute = counter_execute;
    Py_INCREF(self);
    executor->optimizer = (_PyCounterOptimizerObject *)self;
    executor->next_instr = instr;
    *exec_ptr = (_PyExecutorObject *)executor;
    _PyBloomFilter empty;
    _Py_BloomFilter_Init(&empty);
    _Py_ExecutorInit((_PyExecutorObject *)executor, &empty);
    return 1;
}

static PyObject *
counter_get_counter(PyObject *self, PyObject *args)
{
    return PyLong_FromLongLong(((_PyCounterOptimizerObject *)self)->count);
}

static PyMethodDef counter_optimizer_methods[] = {
    { "get_count", counter_get_counter, METH_NOARGS, NULL },
    { NULL, NULL },
};

PyTypeObject _PyCounterOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "Counter optimizer",
    .tp_basicsize = sizeof(_PyCounterOptimizerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_methods = counter_optimizer_methods,
    .tp_dealloc = (destructor)PyObject_Del,
};

PyObject *
PyUnstable_Optimizer_NewCounter(void)
{
    _PyCounterOptimizerObject *opt = (_PyCounterOptimizerObject *)_PyObject_New(&_PyCounterOptimizer_Type);
    if (opt == NULL) {
        return NULL;
    }
    opt->base.optimize = counter_optimize;
    opt->base.resume_threshold = INT16_MAX;
    opt->base.backedge_threshold = 0;
    opt->count = 0;
    return (PyObject *)opt;
}

///////////////////// Experimental UOp Optimizer /////////////////////

static void
uop_dealloc(_PyUOpExecutorObject *self) {
    _Py_ExecutorClear((_PyExecutorObject *)self);
    PyObject_Free(self);
}

const char *
_PyUOpName(int index)
{
    return _PyOpcode_uop_name[index];
}

static Py_ssize_t
uop_len(_PyUOpExecutorObject *self)
{
    return Py_SIZE(self);
}

static PyObject *
uop_item(_PyUOpExecutorObject *self, Py_ssize_t index)
{
    Py_ssize_t len = uop_len(self);
    if (index < 0 || index >= len) {
        PyErr_SetNone(PyExc_IndexError);
        return NULL;
    }
    const char *name = _PyUOpName(self->trace[index].opcode);
    if (name == NULL) {
        name = "<nil>";
    }
    PyObject *oname = _PyUnicode_FromASCII(name, strlen(name));
    if (oname == NULL) {
        return NULL;
    }
    PyObject *oparg = PyLong_FromUnsignedLong(self->trace[index].oparg);
    if (oparg == NULL) {
        Py_DECREF(oname);
        return NULL;
    }
    PyObject *operand = PyLong_FromUnsignedLongLong(self->trace[index].operand);
    if (operand == NULL) {
        Py_DECREF(oparg);
        Py_DECREF(oname);
        return NULL;
    }
    PyObject *args[3] = { oname, oparg, operand };
    return _PyTuple_FromArraySteal(args, 3);
}

PySequenceMethods uop_as_sequence = {
    .sq_length = (lenfunc)uop_len,
    .sq_item = (ssizeargfunc)uop_item,
};

PyTypeObject _PyUOpExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_executor",
    .tp_basicsize = sizeof(_PyUOpExecutorObject) - sizeof(_PyUOpInstruction),
    .tp_itemsize = sizeof(_PyUOpInstruction),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)uop_dealloc,
    .tp_as_sequence = &uop_as_sequence,
    .tp_methods = executor_methods,
};

/* TO DO -- Generate these tables */
static const uint16_t
_PyUOp_Replacements[MAX_UOP_ID + 1] = {
    [_ITER_JUMP_RANGE] = _GUARD_NOT_EXHAUSTED_RANGE,
    [_ITER_JUMP_LIST] = _GUARD_NOT_EXHAUSTED_LIST,
    [_ITER_JUMP_TUPLE] = _GUARD_NOT_EXHAUSTED_TUPLE,
    [_FOR_ITER] = _FOR_ITER_TIER_TWO,
};

static const uint16_t
BRANCH_TO_GUARD[4][2] = {
    [POP_JUMP_IF_FALSE - POP_JUMP_IF_FALSE][0] = _GUARD_IS_TRUE_POP,
    [POP_JUMP_IF_FALSE - POP_JUMP_IF_FALSE][1] = _GUARD_IS_FALSE_POP,
    [POP_JUMP_IF_TRUE - POP_JUMP_IF_FALSE][0] = _GUARD_IS_FALSE_POP,
    [POP_JUMP_IF_TRUE - POP_JUMP_IF_FALSE][1] = _GUARD_IS_TRUE_POP,
    [POP_JUMP_IF_NONE - POP_JUMP_IF_FALSE][0] = _GUARD_IS_NOT_NONE_POP,
    [POP_JUMP_IF_NONE - POP_JUMP_IF_FALSE][1] = _GUARD_IS_NONE_POP,
    [POP_JUMP_IF_NOT_NONE - POP_JUMP_IF_FALSE][0] = _GUARD_IS_NONE_POP,
    [POP_JUMP_IF_NOT_NONE - POP_JUMP_IF_FALSE][1] = _GUARD_IS_NOT_NONE_POP,
};

#define TRACE_STACK_SIZE 5

#define CONFIDENCE_RANGE 1000
#define CONFIDENCE_CUTOFF 333

/* Returns 1 on success,
 * 0 if it failed to produce a worthwhile trace,
 * and -1 on an error.
 */
static int
translate_bytecode_to_trace(
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyUOpInstruction *trace,
    int buffer_size,
    _PyBloomFilter *dependencies)
{
    PyCodeObject *initial_code = code;
    _Py_BloomFilter_Add(dependencies, initial_code);
    _Py_CODEUNIT *initial_instr = instr;
    int trace_length = 0;
    int max_length = buffer_size;
    struct {
        PyCodeObject *code;
        _Py_CODEUNIT *instr;
    } trace_stack[TRACE_STACK_SIZE];
    int trace_stack_depth = 0;
    int confidence = CONFIDENCE_RANGE;  // Adjusted by branch instructions

#ifdef Py_DEBUG
    char *python_lltrace = Py_GETENV("PYTHON_LLTRACE");
    int lltrace = 0;
    if (python_lltrace != NULL && *python_lltrace >= '0') {
        lltrace = *python_lltrace - '0';  // TODO: Parse an int and all that
    }
#endif

#ifdef Py_DEBUG
#define DPRINTF(level, ...) \
    if (lltrace >= (level)) { printf(__VA_ARGS__); }
#else
#define DPRINTF(level, ...)
#endif


#define ADD_TO_TRACE(OPCODE, OPARG, OPERAND, TARGET) \
    DPRINTF(2, \
            "  ADD_TO_TRACE(%s, %d, %" PRIu64 ")\n", \
            _PyUOpName(OPCODE), \
            (OPARG), \
            (uint64_t)(OPERAND)); \
    assert(trace_length < max_length); \
    trace[trace_length].opcode = (OPCODE); \
    trace[trace_length].oparg = (OPARG); \
    trace[trace_length].operand = (OPERAND); \
    trace[trace_length].target = (TARGET); \
    trace_length++;

#define INSTR_IP(INSTR, CODE) \
    ((uint32_t)((INSTR) - ((_Py_CODEUNIT *)(CODE)->co_code_adaptive)))

// Reserve space for n uops
#define RESERVE_RAW(n, opname) \
    if (trace_length + (n) > max_length) { \
        DPRINTF(2, "No room for %s (need %d, got %d)\n", \
                (opname), (n), max_length - trace_length); \
        OPT_STAT_INC(trace_too_long); \
        goto done; \
    }

// Reserve space for N uops, plus 3 for _SET_IP, _CHECK_VALIDITY and _EXIT_TRACE
#define RESERVE(needed) RESERVE_RAW((needed) + 3, _PyUOpName(opcode))

// Trace stack operations (used by _PUSH_FRAME, _POP_FRAME)
#define TRACE_STACK_PUSH() \
    if (trace_stack_depth >= TRACE_STACK_SIZE) { \
        DPRINTF(2, "Trace stack overflow\n"); \
        OPT_STAT_INC(trace_stack_overflow); \
        ADD_TO_TRACE(_EXIT_TRACE, 0, 0, 0); \
        goto done; \
    } \
    trace_stack[trace_stack_depth].code = code; \
    trace_stack[trace_stack_depth].instr = instr; \
    trace_stack_depth++;
#define TRACE_STACK_POP() \
    if (trace_stack_depth <= 0) { \
        Py_FatalError("Trace stack underflow\n"); \
    } \
    trace_stack_depth--; \
    code = trace_stack[trace_stack_depth].code; \
    instr = trace_stack[trace_stack_depth].instr;

    DPRINTF(4,
            "Optimizing %s (%s:%d) at byte offset %d\n",
            PyUnicode_AsUTF8(code->co_qualname),
            PyUnicode_AsUTF8(code->co_filename),
            code->co_firstlineno,
            2 * INSTR_IP(initial_instr, code));
    uint32_t target = 0;
top:  // Jump here after _PUSH_FRAME or likely branches
    for (;;) {
        target = INSTR_IP(instr, code);
        RESERVE_RAW(3, "epilogue");  // Always need space for _SET_IP, _CHECK_VALIDITY and _EXIT_TRACE
        ADD_TO_TRACE(_SET_IP, target, 0, target);
        ADD_TO_TRACE(_CHECK_VALIDITY, 0, 0, target);

        uint32_t opcode = instr->op.code;
        uint32_t oparg = instr->op.arg;
        uint32_t extended = 0;

        if (opcode == EXTENDED_ARG) {
            instr++;
            extended = 1;
            opcode = instr->op.code;
            oparg = (oparg << 8) | instr->op.arg;
            if (opcode == EXTENDED_ARG) {
                instr--;
                goto done;
            }
        }

        if (opcode == ENTER_EXECUTOR) {
            _PyExecutorObject *executor =
                (_PyExecutorObject *)code->co_executors->executors[oparg&255];
            opcode = executor->vm_data.opcode;
            DPRINTF(2, "  * ENTER_EXECUTOR -> %s\n",  _PyOpcode_OpName[opcode]);
            oparg = (oparg & 0xffffff00) | executor->vm_data.oparg;
        }

        switch (opcode) {
            case POP_JUMP_IF_NONE:
            case POP_JUMP_IF_NOT_NONE:
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            {
                RESERVE(1);
                int counter = instr[1].cache;
                int bitcount = _Py_popcount32(counter);
                int jump_likely = bitcount > 8;
                if (jump_likely) {
                    confidence = confidence * bitcount / 16;
                }
                else {
                    confidence = confidence * (16 - bitcount) / 16;
                }
                if (confidence < CONFIDENCE_CUTOFF) {
                    DPRINTF(2, "Confidence too low (%d)\n", confidence);
                    OPT_STAT_INC(low_confidence);
                    goto done;
                }
                uint32_t uopcode = BRANCH_TO_GUARD[opcode - POP_JUMP_IF_FALSE][jump_likely];
                _Py_CODEUNIT *next_instr = instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
                DPRINTF(2, "%s(%d): counter=%x, bitcount=%d, likely=%d, confidence=%d, uopcode=%s\n",
                        _PyOpcode_OpName[opcode], oparg,
                        counter, bitcount, jump_likely, confidence, _PyUOpName(uopcode));
                ADD_TO_TRACE(uopcode, max_length, 0, target);
                if (jump_likely) {
                    _Py_CODEUNIT *target_instr = next_instr + oparg;
                    DPRINTF(2, "Jump likely (%x = %d bits), continue at byte offset %d\n",
                            instr[1].cache, bitcount, 2 * INSTR_IP(target_instr, code));
                    instr = target_instr;
                    goto top;
                }
                break;
            }

            case JUMP_BACKWARD:
            case JUMP_BACKWARD_NO_INTERRUPT:
            {
                if (instr + 2 - oparg == initial_instr && code == initial_code) {
                    RESERVE(1);
                    ADD_TO_TRACE(_JUMP_TO_TOP, 0, 0, 0);
                }
                else {
                    OPT_STAT_INC(inner_loop);
                    DPRINTF(2, "JUMP_BACKWARD not to top ends trace\n");
                }
                goto done;
            }

            case JUMP_FORWARD:
            {
                RESERVE(0);
                // This will emit two _SET_IP instructions; leave it to the optimizer
                instr += oparg;
                break;
            }

            default:
            {
                const struct opcode_macro_expansion *expansion = &_PyOpcode_macro_expansion[opcode];
                if (expansion->nuops > 0) {
                    // Reserve space for nuops (+ _SET_IP + _EXIT_TRACE)
                    int nuops = expansion->nuops;
                    RESERVE(nuops);
                    if (expansion->uops[nuops-1].uop == _POP_FRAME) {
                        // Check for trace stack underflow now:
                        // We can't bail e.g. in the middle of
                        // LOAD_CONST + _POP_FRAME.
                        if (trace_stack_depth == 0) {
                            DPRINTF(2, "Trace stack underflow\n");
                            OPT_STAT_INC(trace_stack_underflow);
                            goto done;
                        }
                    }
                    uint32_t orig_oparg = oparg;  // For OPARG_TOP/BOTTOM
                    for (int i = 0; i < nuops; i++) {
                        oparg = orig_oparg;
                        uint32_t uop = expansion->uops[i].uop;
                        uint64_t operand = 0;
                        // Add one to account for the actual opcode/oparg pair:
                        int offset = expansion->uops[i].offset + 1;
                        switch (expansion->uops[i].size) {
                            case OPARG_FULL:
                                assert(opcode != JUMP_BACKWARD_NO_INTERRUPT && opcode != JUMP_BACKWARD);
                                break;
                            case OPARG_CACHE_1:
                                operand = read_u16(&instr[offset].cache);
                                break;
                            case OPARG_CACHE_2:
                                operand = read_u32(&instr[offset].cache);
                                break;
                            case OPARG_CACHE_4:
                                operand = read_u64(&instr[offset].cache);
                                break;
                            case OPARG_TOP:  // First half of super-instr
                                oparg = orig_oparg >> 4;
                                break;
                            case OPARG_BOTTOM:  // Second half of super-instr
                                oparg = orig_oparg & 0xF;
                                break;
                            case OPARG_SAVE_RETURN_OFFSET:  // op=_SAVE_RETURN_OFFSET; oparg=return_offset
                                oparg = offset;
                                assert(uop == _SAVE_RETURN_OFFSET);
                                break;
                            case OPARG_REPLACED:
                                uop = _PyUOp_Replacements[uop];
                                assert(uop != 0);
                                if (uop == _FOR_ITER_TIER_TWO) {
                                    target += 1 + INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1 + extended;
                                    assert(_PyCode_CODE(code)[target-1].op.code == END_FOR ||
                                            _PyCode_CODE(code)[target-1].op.code == INSTRUMENTED_END_FOR);
                                }
                                break;
                            default:
                                fprintf(stderr,
                                        "opcode=%d, oparg=%d; nuops=%d, i=%d; size=%d, offset=%d\n",
                                        opcode, oparg, nuops, i,
                                        expansion->uops[i].size,
                                        expansion->uops[i].offset);
                                Py_FatalError("garbled expansion");
                        }
                        ADD_TO_TRACE(uop, oparg, operand, target);
                        if (uop == _POP_FRAME) {
                            TRACE_STACK_POP();
                            DPRINTF(2,
                                "Returning to %s (%s:%d) at byte offset %d\n",
                                PyUnicode_AsUTF8(code->co_qualname),
                                PyUnicode_AsUTF8(code->co_filename),
                                code->co_firstlineno,
                                2 * INSTR_IP(instr, code));
                            goto top;
                        }
                        if (uop == _PUSH_FRAME) {
                            assert(i + 1 == nuops);
                            int func_version_offset =
                                offsetof(_PyCallCache, func_version)/sizeof(_Py_CODEUNIT)
                                // Add one to account for the actual opcode/oparg pair:
                                + 1;
                            uint32_t func_version = read_u32(&instr[func_version_offset].cache);
                            PyFunctionObject *func = _PyFunction_LookupByVersion(func_version);
                            DPRINTF(3, "Function object: %p\n", func);
                            if (func != NULL) {
                                PyCodeObject *new_code = (PyCodeObject *)PyFunction_GET_CODE(func);
                                if (new_code == code) {
                                    // Recursive call, bail (we could be here forever).
                                    DPRINTF(2, "Bailing on recursive call to %s (%s:%d)\n",
                                            PyUnicode_AsUTF8(new_code->co_qualname),
                                            PyUnicode_AsUTF8(new_code->co_filename),
                                            new_code->co_firstlineno);
                                    OPT_STAT_INC(recursive_call);
                                    ADD_TO_TRACE(_EXIT_TRACE, 0, 0, 0);
                                    goto done;
                                }
                                if (new_code->co_version != func_version) {
                                    // func.__code__ was updated.
                                    // Perhaps it may happen again, so don't bother tracing.
                                    // TODO: Reason about this -- is it better to bail or not?
                                    DPRINTF(2, "Bailing because co_version != func_version\n");
                                    ADD_TO_TRACE(_EXIT_TRACE, 0, 0, 0);
                                    goto done;
                                }
                                // Increment IP to the return address
                                instr += _PyOpcode_Caches[_PyOpcode_Deopt[opcode]] + 1;
                                TRACE_STACK_PUSH();
                                _Py_BloomFilter_Add(dependencies, new_code);
                                code = new_code;
                                instr = _PyCode_CODE(code);
                                DPRINTF(2,
                                    "Continuing in %s (%s:%d) at byte offset %d\n",
                                    PyUnicode_AsUTF8(code->co_qualname),
                                    PyUnicode_AsUTF8(code->co_filename),
                                    code->co_firstlineno,
                                    2 * INSTR_IP(instr, code));
                                goto top;
                            }
                            ADD_TO_TRACE(_EXIT_TRACE, 0, 0, 0);
                            goto done;
                        }
                    }
                    break;
                }
                DPRINTF(2, "Unsupported opcode %s\n", _PyOpcode_OpName[opcode]);
                OPT_UNSUPPORTED_OPCODE(opcode);
                goto done;  // Break out of loop
            }  // End default

        }  // End switch (opcode)

        instr++;
        // Add cache size for opcode
        instr += _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
    }  // End for (;;)

done:
    while (trace_stack_depth > 0) {
        TRACE_STACK_POP();
    }
    assert(code == initial_code);
    // Skip short traces like _SET_IP, LOAD_FAST, _SET_IP, _EXIT_TRACE
    if (trace_length > 4) {
        ADD_TO_TRACE(_EXIT_TRACE, 0, 0, target);
        DPRINTF(1,
                "Created a trace for %s (%s:%d) at byte offset %d -- length %d\n",
                PyUnicode_AsUTF8(code->co_qualname),
                PyUnicode_AsUTF8(code->co_filename),
                code->co_firstlineno,
                2 * INSTR_IP(initial_instr, code),
                trace_length);
        OPT_HIST(trace_length + buffer_size - max_length, trace_length_hist);
        return 1;
    }
    else {
        OPT_STAT_INC(trace_too_short);
        DPRINTF(4,
                "No trace for %s (%s:%d) at byte offset %d\n",
                PyUnicode_AsUTF8(code->co_qualname),
                PyUnicode_AsUTF8(code->co_filename),
                code->co_firstlineno,
                2 * INSTR_IP(initial_instr, code));
    }
    return 0;

#undef RESERVE
#undef RESERVE_RAW
#undef INSTR_IP
#undef ADD_TO_TRACE
#undef DPRINTF
}

#define UNSET_BIT(array, bit) (array[(bit)>>5] &= ~(1<<((bit)&31)))
#define SET_BIT(array, bit) (array[(bit)>>5] |= (1<<((bit)&31)))
#define BIT_IS_SET(array, bit) (array[(bit)>>5] & (1<<((bit)&31)))

/* Count the number of used uops, and mark them in the bit vector `used`.
 * This can be done in a single pass using simple reachability analysis,
 * as there are no backward jumps.
 * NOPs are excluded from the count.
*/
static int
compute_used(_PyUOpInstruction *buffer, uint32_t *used)
{
    int count = 0;
    SET_BIT(used, 0);
    for (int i = 0; i < _Py_UOP_MAX_TRACE_LENGTH; i++) {
        if (!BIT_IS_SET(used, i)) {
            continue;
        }
        count++;
        int opcode = buffer[i].opcode;
        if (opcode == _JUMP_TO_TOP || opcode == _EXIT_TRACE) {
            continue;
        }
        /* All other micro-ops fall through, so i+1 is reachable */
        SET_BIT(used, i+1);
        assert(opcode <= MAX_UOP_ID);
        if (_PyUop_Flags[opcode] & HAS_JUMP_FLAG) {
            /* Mark target as reachable */
            SET_BIT(used, buffer[i].oparg);
        }
        if (opcode == NOP) {
            count--;
            UNSET_BIT(used, i);
        }
    }
    return count;
}

/* Makes an executor from a buffer of uops.
 * Account for the buffer having gaps and NOPs by computing a "used"
 * bit vector and only copying the used uops. Here "used" means reachable
 * and not a NOP.
 */
static _PyExecutorObject *
make_executor_from_uops(_PyUOpInstruction *buffer, _PyBloomFilter *dependencies)
{
    uint32_t used[(_Py_UOP_MAX_TRACE_LENGTH + 31)/32] = { 0 };
    int length = compute_used(buffer, used);
    _PyUOpExecutorObject *executor = PyObject_NewVar(_PyUOpExecutorObject, &_PyUOpExecutor_Type, length);
    if (executor == NULL) {
        return NULL;
    }
    int dest = length - 1;
    /* Scan backwards, so that we see the destinations of jumps before the jumps themselves. */
    for (int i = _Py_UOP_MAX_TRACE_LENGTH-1; i >= 0; i--) {
        if (!BIT_IS_SET(used, i)) {
            continue;
        }
        executor->trace[dest] = buffer[i];
        int opcode = buffer[i].opcode;
        if (opcode == _POP_JUMP_IF_FALSE ||
            opcode == _POP_JUMP_IF_TRUE)
        {
            /* The oparg of the target will already have been set to its new offset */
            int oparg = executor->trace[dest].oparg;
            executor->trace[dest].oparg = buffer[oparg].oparg;
        }
        /* Set the oparg to be the destination offset,
         * so that we can set the oparg of earlier jumps correctly. */
        buffer[i].oparg = dest;
        dest--;
    }
    assert(dest == -1);
    executor->base.execute = _PyUOpExecute;
    _Py_ExecutorInit((_PyExecutorObject *)executor, dependencies);
#ifdef Py_DEBUG
    char *python_lltrace = Py_GETENV("PYTHON_LLTRACE");
    int lltrace = 0;
    if (python_lltrace != NULL && *python_lltrace >= '0') {
        lltrace = *python_lltrace - '0';  // TODO: Parse an int and all that
    }
    if (lltrace >= 2) {
        printf("Optimized executor (length %d):\n", length);
        for (int i = 0; i < length; i++) {
            printf("%4d %s(%d, %d, %" PRIu64 ")\n",
                   i,
                   _PyUOpName(executor->trace[i].opcode),
                   executor->trace[i].oparg,
                   executor->trace[i].target,
                   executor->trace[i].operand);
        }
    }
#endif
    return (_PyExecutorObject *)executor;
}

static int
uop_optimize(
    _PyOptimizerObject *self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyExecutorObject **exec_ptr,
    int curr_stackentries)
{
    _PyBloomFilter dependencies;
    _Py_BloomFilter_Init(&dependencies);
    _PyUOpInstruction buffer[_Py_UOP_MAX_TRACE_LENGTH];
    int err = translate_bytecode_to_trace(code, instr, buffer, _Py_UOP_MAX_TRACE_LENGTH, &dependencies);
    if (err <= 0) {
        // Error or nothing translated
        return err;
    }
    OPT_STAT_INC(traces_created);
    char *uop_optimize = Py_GETENV("PYTHONUOPSOPTIMIZE");
    if (uop_optimize == NULL || *uop_optimize > '0') {
        err = _Py_uop_analyze_and_optimize(code, buffer, _Py_UOP_MAX_TRACE_LENGTH, curr_stackentries);
        if (err < 0) {
            return -1;
        }
    }
    _PyExecutorObject *executor = make_executor_from_uops(buffer, &dependencies);
    if (executor == NULL) {
        return -1;
    }
    OPT_HIST(Py_SIZE(executor), optimized_trace_length_hist);
    *exec_ptr = executor;
    return 1;
}

/* Dummy execute() function for UOp Executor.
 * The actual implementation is inlined in ceval.c,
 * in _PyEval_EvalFrameDefault(). */
_Py_CODEUNIT *
_PyUOpExecute(_PyExecutorObject *executor, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    Py_FatalError("Tier 2 is now inlined into Tier 1");
}

static void
uop_opt_dealloc(PyObject *self) {
    PyObject_Free(self);
}

PyTypeObject _PyUOpOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_optimizer",
    .tp_basicsize = sizeof(_PyOptimizerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = uop_opt_dealloc,
};

PyObject *
PyUnstable_Optimizer_NewUOpOptimizer(void)
{
    _PyOptimizerObject *opt = PyObject_New(_PyOptimizerObject, &_PyUOpOptimizer_Type);
    if (opt == NULL) {
        return NULL;
    }
    opt->optimize = uop_optimize;
    opt->resume_threshold = INT16_MAX;
    // Need at least 3 iterations to settle specializations.
    // A few lower bits of the counter are reserved for other flags.
    opt->backedge_threshold = 16 << OPTIMIZER_BITS_IN_COUNTER;
    return (PyObject *)opt;
}


/*****************************************
 *        Executor management
 ****************************************/

/* We use a bloomfilter with k = 6, m = 256
 * The choice of k and the following constants
 * could do with a more rigourous analysis,
 * but here is a simple analysis:
 *
 * We want to keep the false positive rate low.
 * For n = 5 (a trace depends on 5 objects),
 * we expect 30 bits set, giving a false positive
 * rate of (30/256)**6 == 2.5e-6 which is plenty
 * good enough.
 *
 * However with n = 10 we expect 60 bits set (worst case),
 * giving a false positive of (60/256)**6 == 0.0001
 *
 * We choose k = 6, rather than a higher number as
 * it means the false positive rate grows slower for high n.
 *
 * n = 5, k = 6 => fp = 2.6e-6
 * n = 5, k = 8 => fp = 3.5e-7
 * n = 10, k = 6 => fp = 1.6e-4
 * n = 10, k = 8 => fp = 0.9e-4
 * n = 15, k = 6 => fp = 0.18%
 * n = 15, k = 8 => fp = 0.23%
 * n = 20, k = 6 => fp = 1.1%
 * n = 20, k = 8 => fp = 2.3%
 *
 * The above analysis assumes perfect hash functions,
 * but those don't exist, so the real false positive
 * rates may be worse.
 */

#define K 6

#define SEED 20221211

/* TO DO -- Use more modern hash functions with better distribution of bits */
static uint64_t
address_to_hash(void *ptr) {
    assert(ptr != NULL);
    uint64_t uhash = SEED;
    uintptr_t addr = (uintptr_t)ptr;
    for (int i = 0; i < SIZEOF_VOID_P; i++) {
        uhash ^= addr & 255;
        uhash *= (uint64_t)_PyHASH_MULTIPLIER;
        addr >>= 8;
    }
    return uhash;
}

void
_Py_BloomFilter_Init(_PyBloomFilter *bloom)
{
    for (int i = 0; i < BLOOM_FILTER_WORDS; i++) {
        bloom->bits[i] = 0;
    }
}

/* We want K hash functions that each set 1 bit.
 * A hash function that sets 1 bit in M bits can be trivially
 * derived from a log2(M) bit hash function.
 * So we extract 8 (log2(256)) bits at a time from
 * the 64bit hash. */
void
_Py_BloomFilter_Add(_PyBloomFilter *bloom, void *ptr)
{
    uint64_t hash = address_to_hash(ptr);
    assert(K <= 8);
    for (int i = 0; i < K; i++) {
        uint8_t bits = hash & 255;
        bloom->bits[bits >> 5] |= (1 << (bits&31));
        hash >>= 8;
    }
}

static bool
bloom_filter_may_contain(_PyBloomFilter *bloom, _PyBloomFilter *hashes)
{
    for (int i = 0; i < BLOOM_FILTER_WORDS; i++) {
        if ((bloom->bits[i] & hashes->bits[i]) != hashes->bits[i]) {
            return false;
        }
    }
    return true;
}

static void
link_executor(_PyExecutorObject *executor)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyExecutorLinkListNode *links = &executor->vm_data.links;
    _PyExecutorObject *head = interp->executor_list_head;
    if (head == NULL) {
        interp->executor_list_head = executor;
        links->previous = NULL;
        links->next = NULL;
    }
    else {
        _PyExecutorObject *next = head->vm_data.links.next;
        links->previous = head;
        links->next = next;
        if (next != NULL) {
            next->vm_data.links.previous = executor;
        }
        head->vm_data.links.next = executor;
    }
    executor->vm_data.linked = true;
    /* executor_list_head must be first in list */
    assert(interp->executor_list_head->vm_data.links.previous == NULL);
}

static void
unlink_executor(_PyExecutorObject *executor)
{
    if (!executor->vm_data.linked) {
        return;
    }
    _PyExecutorLinkListNode *links = &executor->vm_data.links;
    _PyExecutorObject *next = links->next;
    _PyExecutorObject *prev = links->previous;
    if (next != NULL) {
        next->vm_data.links.previous = prev;
    }
    if (prev != NULL) {
        prev->vm_data.links.next = next;
    }
    else {
        // prev == NULL implies that executor is the list head
        PyInterpreterState *interp = PyInterpreterState_Get();
        assert(interp->executor_list_head == executor);
        interp->executor_list_head = next;
    }
    executor->vm_data.linked = false;
}

/* This must be called by optimizers before using the executor */
void
_Py_ExecutorInit(_PyExecutorObject *executor, _PyBloomFilter *dependency_set)
{
    executor->vm_data.valid = true;
    for (int i = 0; i < BLOOM_FILTER_WORDS; i++) {
        executor->vm_data.bloom.bits[i] = dependency_set->bits[i];
    }
    link_executor(executor);
}

/* This must be called by executors during dealloc */
void
_Py_ExecutorClear(_PyExecutorObject *executor)
{
    unlink_executor(executor);
}

void
_Py_Executor_DependsOn(_PyExecutorObject *executor, void *obj)
{
    assert(executor->vm_data.valid = true);
    _Py_BloomFilter_Add(&executor->vm_data.bloom, obj);
}

/* Invalidate all executors that depend on `obj`
 * May cause other executors to be invalidated as well
 */
void
_Py_Executors_InvalidateDependency(PyInterpreterState *interp, void *obj)
{
    _PyBloomFilter obj_filter;
    _Py_BloomFilter_Init(&obj_filter);
    _Py_BloomFilter_Add(&obj_filter, obj);
    /* Walk the list of executors */
    /* TO DO -- Use a tree to avoid traversing as many objects */
    for (_PyExecutorObject *exec = interp->executor_list_head; exec != NULL;) {
        assert(exec->vm_data.valid);
        _PyExecutorObject *next = exec->vm_data.links.next;
        if (bloom_filter_may_contain(&exec->vm_data.bloom, &obj_filter)) {
            exec->vm_data.valid = false;
            unlink_executor(exec);
        }
        exec = next;
    }
}

/* Invalidate all executors */
void
_Py_Executors_InvalidateAll(PyInterpreterState *interp)
{
    /* Walk the list of executors */
    for (_PyExecutorObject *exec = interp->executor_list_head; exec != NULL;) {
        assert(exec->vm_data.valid);
        _PyExecutorObject *next = exec->vm_data.links.next;
        exec->vm_data.links.next = NULL;
        exec->vm_data.links.previous = NULL;
        exec->vm_data.valid = false;
        exec->vm_data.linked = false;
        exec = next;
    }
    interp->executor_list_head = NULL;
}
