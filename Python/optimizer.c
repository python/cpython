#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_bitutils.h"        // _Py_popcount32()
#include "pycore_object.h"          // _PyObject_GC_UNTRACK()
#include "pycore_opcode_metadata.h" // _PyOpcode_OpName[]
#include "pycore_opcode_utils.h"  // MAX_REAL_OPCODE
#include "pycore_optimizer.h"     // _Py_uop_analyze_and_optimize()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uop_ids.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define NEED_OPCODE_METADATA
#include "pycore_uop_metadata.h" // Uop tables
#undef NEED_OPCODE_METADATA

#define UOP_MAX_TRACE_LENGTH 512

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
never_optimize(
    _PyOptimizerObject* self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyExecutorObject **exec,
    int Py_UNUSED(stack_entries))
{
    return 0;
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
    .optimize = never_optimize,
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

/* Returns 1 if optimized, 0 if not optimized, and -1 for an error.
 * If optimized, *executor_ptr contains a new reference to the executor
 */
int
_PyOptimizer_Optimize(
    _PyInterpreterFrame *frame, _Py_CODEUNIT *start,
    PyObject **stack_pointer, _PyExecutorObject **executor_ptr)
{
    PyCodeObject *code = (PyCodeObject *)frame->f_executable;
    assert(PyCode_Check(code));
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!has_space_for_executor(code, start)) {
        return 0;
    }
    _PyOptimizerObject *opt = interp->optimizer;
    int err = opt->optimize(opt, code, start, executor_ptr, (int)(stack_pointer - _PyFrame_Stackbase(frame)));
    if (err <= 0) {
        return err;
    }
    assert(*executor_ptr != NULL);
    int index = get_index_for_executor(code, start);
    if (index < 0) {
        /* Out of memory. Don't raise and assume that the
         * error will show up elsewhere.
         *
         * If an optimizer has already produced an executor,
         * it might get confused by the executor disappearing,
         * but there is not much we can do about that here. */
        Py_DECREF(*executor_ptr);
        return 0;
    }
    insert_executor(code, start, index, *executor_ptr);
    assert((*executor_ptr)->vm_data.valid);
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

static PyObject *
is_valid(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyBool_FromLong(((_PyExecutorObject *)self)->vm_data.valid);
}

static PyMethodDef executor_methods[] = {
    { "is_valid", is_valid, METH_NOARGS, NULL },
    { NULL, NULL },
};

///////////////////// Experimental UOp Optimizer /////////////////////

static void
uop_dealloc(_PyExecutorObject *self) {
    _PyObject_GC_UNTRACK(self);
    _Py_ExecutorClear(self);
    PyObject_GC_Del(self);
}

const char *
_PyUOpName(int index)
{
    return _PyOpcode_uop_name[index];
}

static Py_ssize_t
uop_len(_PyExecutorObject *self)
{
    return self->code_size;
}

static PyObject *
uop_item(_PyExecutorObject *self, Py_ssize_t index)
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

/* This must be called by executors during dealloc */
void
_Py_ExecutorClear(_PyExecutorObject *executor)
{
    executor->vm_data.valid = 0;
    unlink_executor(executor);
    for (uint32_t i = 0; i < executor->exit_count; i++) {
        Py_CLEAR(executor->exits[i].executor);
    }
}

static int
executor_clear(PyObject *o)
{
    _Py_ExecutorClear((_PyExecutorObject *)o);
    return 0;
}

static int
executor_traverse(PyObject *o, visitproc visit, void *arg)
{
    _PyExecutorObject *executor = (_PyExecutorObject *)o;
    for (uint32_t i = 0; i < executor->exit_count; i++) {
        Py_VISIT(executor->exits[i].executor);
    }
    return 0;
}

PyTypeObject _PyUOpExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_executor",
    .tp_basicsize = offsetof(_PyExecutorObject, exits),
    .tp_itemsize = 1,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    .tp_dealloc = (destructor)uop_dealloc,
    .tp_as_sequence = &uop_as_sequence,
    .tp_methods = executor_methods,
    .tp_traverse = executor_traverse,
    .tp_clear = executor_clear,
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
    bool progress_needed = true;
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

        if (opcode == ENTER_EXECUTOR) {
            assert(oparg < 256);
            _PyExecutorObject *executor = code->co_executors->executors[oparg];
            opcode = executor->vm_data.opcode;
            DPRINTF(2, "  * ENTER_EXECUTOR -> %s\n",  _PyOpcode_OpName[opcode]);
            oparg = executor->vm_data.oparg;
        }

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
        assert(opcode != ENTER_EXECUTOR && opcode != EXTENDED_ARG);

        /* Special case the first instruction,
         * so that we can guarantee forward progress */
        if (progress_needed) {
            progress_needed = false;
            if (opcode == JUMP_BACKWARD || opcode == JUMP_BACKWARD_NO_INTERRUPT) {
                instr += 1 + _PyOpcode_Caches[opcode] - (int32_t)oparg;
                initial_instr = instr;
                continue;
            }
            else {
                if (OPCODE_HAS_DEOPT(opcode)) {
                    opcode = _PyOpcode_Deopt[opcode];
                }
                assert(!OPCODE_HAS_DEOPT(opcode));
            }
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
                DPRINTF(2, "%s(%d): counter=%x, bitcount=%d, likely=%d, confidence=%d, uopcode=%s\n",
                        _PyOpcode_OpName[opcode], oparg,
                        counter, bitcount, jump_likely, confidence, _PyUOpName(uopcode));
                _Py_CODEUNIT *next_instr = instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
                _Py_CODEUNIT *target_instr = next_instr + oparg;
                if (jump_likely) {
                    DPRINTF(2, "Jump likely (%x = %d bits), continue at byte offset %d\n",
                            instr[1].cache, bitcount, 2 * INSTR_IP(target_instr, code));
                    instr = target_instr;
                    ADD_TO_TRACE(uopcode, max_length, 0, INSTR_IP(next_instr, code));
                    goto top;
                }
                ADD_TO_TRACE(uopcode, max_length, 0, INSTR_IP(target_instr, code));
                break;
            }

            case JUMP_BACKWARD:
            case JUMP_BACKWARD_NO_INTERRUPT:
            {
                _Py_CODEUNIT *target = instr + 1 + _PyOpcode_Caches[opcode] - (int)oparg;
                if (target == initial_instr) {
                    /* We have looped round to the start */
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
    if (progress_needed || trace_length < 5) {
        OPT_STAT_INC(trace_too_short);
        DPRINTF(4,
                "No trace for %s (%s:%d) at byte offset %d\n",
                PyUnicode_AsUTF8(code->co_qualname),
                PyUnicode_AsUTF8(code->co_filename),
                code->co_firstlineno,
                2 * INSTR_IP(initial_instr, code));
        return 0;
    }
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

#undef RESERVE
#undef RESERVE_RAW
#undef INSTR_IP
#undef ADD_TO_TRACE
#undef DPRINTF

#define UNSET_BIT(array, bit) (array[(bit)>>5] &= ~(1<<((bit)&31)))
#define SET_BIT(array, bit) (array[(bit)>>5] |= (1<<((bit)&31)))
#define BIT_IS_SET(array, bit) (array[(bit)>>5] & (1<<((bit)&31)))

/* Count the number of used uops, and mark them in the bit vector `used`.
 * This can be done in a single pass using simple reachability analysis,
 * as there are no backward jumps.
 * NOPs are excluded from the count.
*/
static int
compute_used(_PyUOpInstruction *buffer, uint32_t *used, int *exit_count_ptr)
{
    int count = 0;
    int exit_count = 0;
    SET_BIT(used, 0);
    for (int i = 0; i < UOP_MAX_TRACE_LENGTH; i++) {
        if (!BIT_IS_SET(used, i)) {
            continue;
        }
        count++;
        int opcode = buffer[i].opcode;
        if (_PyUop_Flags[opcode] & HAS_EXIT_FLAG) {
            exit_count++;
        }
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
    *exit_count_ptr = exit_count;
    return count;
}

/* Executor side exits */

static void
cold_dealloc(_PyExecutorObject *self) {
    _Py_ExecutorClear(self);
    PyObject_Free(self);
}

PyTypeObject _ColdExit_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "cold_exit",
    .tp_basicsize = offsetof(_PyExecutorObject, exits),
    .tp_itemsize = 1,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)cold_dealloc,
    .tp_as_sequence = &uop_as_sequence,
    .tp_methods = executor_methods,
};

static _PyColdExitObject COLD_EXITS[UOP_MAX_TRACE_LENGTH];

static _PyExecutorObject *
allocate_executor(int exit_count, int length)
{
    int size = exit_count*sizeof(_PyExitData) + length*sizeof(_PyUOpInstruction);
    _PyExecutorObject *res = PyObject_GC_NewVar(_PyExecutorObject, &_PyUOpExecutor_Type, size);
    if (res == NULL) {
        return NULL;
    }
    res->trace = (_PyUOpInstruction *)(res->exits + exit_count);
    res->code_size = length;
    res->exit_count = exit_count;
    return res;
}

_PyColdExitObject Py_FatalErrorExecutor = {
    .base = {
        PyVarObject_HEAD_INIT(&_ColdExit_Type, 0)
        .vm_data = { 0 },
        .trace = &Py_FatalErrorExecutor.uop
    },
    .uop.opcode = _FATAL_ERROR,
};

/* Makes an executor from a buffer of uops.
 * Account for the buffer having gaps and NOPs by computing a "used"
 * bit vector and only copying the used uops. Here "used" means reachable
 * and not a NOP.
 */
static _PyExecutorObject *
make_executor_from_uops(_PyUOpInstruction *buffer, _PyBloomFilter *dependencies)
{
    uint32_t used[(UOP_MAX_TRACE_LENGTH + 31)/32] = { 0 };
    int exit_count;
    int length = compute_used(buffer, used, &exit_count);
    _PyExecutorObject *executor = allocate_executor(exit_count, length+1);
    if (executor == NULL) {
        return NULL;
    }
    /* Initialize exits */
    for (int i = 0; i < exit_count; i++) {
        executor->exits[i].executor = (_PyExecutorObject *)&COLD_EXITS[i];
        executor->exits[i].hotness = -67;
    }
    int next_exit = exit_count-1;
    int dest = length;
    /* Scan backwards, so that we see the destinations of jumps before the jumps themselves. */
    for (int i = UOP_MAX_TRACE_LENGTH-1; i >= 0; i--) {
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
        if (_PyUop_Flags[opcode] & HAS_EXIT_FLAG) {
            executor->exits[next_exit].target = buffer[i].target;
            executor->trace[dest].target = next_exit;
            next_exit--;
        }
        /* Set the oparg to be the destination offset,
         * so that we can set the oparg of earlier jumps correctly. */
        buffer[i].oparg = dest;
        dest--;
    }
    assert(next_exit == -1);
    assert(dest == 0);
    executor->trace[0].opcode = _START_EXECUTOR;
    executor->trace[0].operand = (uintptr_t)executor;
    _Py_ExecutorInit(executor, dependencies);
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
    _PyObject_GC_TRACK(executor);
    return executor;
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
    _PyUOpInstruction buffer[UOP_MAX_TRACE_LENGTH];
    int err = translate_bytecode_to_trace(code, instr, buffer, UOP_MAX_TRACE_LENGTH, &dependencies);
    if (err <= 0) {
        // Error or nothing translated
        return err;
    }
    OPT_STAT_INC(traces_created);
    char *uop_optimize = Py_GETENV("PYTHONUOPSOPTIMIZE");
    if (uop_optimize == NULL || *uop_optimize > '0') {
        err = _Py_uop_analyze_and_optimize(code, buffer, UOP_MAX_TRACE_LENGTH, curr_stackentries);
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

static void
counter_dealloc(_PyExecutorObject *self) {
    /* The optimizer is the operand of the first uop. */
    PyObject *opt = (PyObject *)self->trace[1].operand;
    Py_DECREF(opt);
    uop_dealloc(self);
}

PyTypeObject _PyCounterExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "counting_executor",
    .tp_basicsize = offsetof(_PyExecutorObject, exits),
    .tp_itemsize = 1,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    .tp_dealloc = (destructor)counter_dealloc,
    .tp_methods = executor_methods,
    .tp_traverse = executor_traverse,
    .tp_clear = executor_clear,
};

static int
counter_optimize(
    _PyOptimizerObject* self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyExecutorObject **exec_ptr,
    int Py_UNUSED(curr_stackentries)
)
{
    int oparg = instr->op.arg;
    while (instr->op.code == EXTENDED_ARG) {
        instr++;
        oparg = (oparg << 8) | instr->op.arg;
    }
    if (instr->op.code != JUMP_BACKWARD) {
        /* Counter optimizer can only handle backward edges */
        return 0;
    }
    _Py_CODEUNIT *target = instr + 1 + _PyOpcode_Caches[JUMP_BACKWARD] - oparg;
    _PyUOpInstruction buffer[3] = {
        { .opcode = _LOAD_CONST_INLINE_BORROW, .operand = (uintptr_t)self },
        { .opcode = _INTERNAL_INCREMENT_OPT_COUNTER },
        { .opcode = _EXIT_TRACE, .target = (uint32_t)(target - _PyCode_CODE(code)) }
    };
    _PyBloomFilter empty;
    _Py_BloomFilter_Init(&empty);
    _PyExecutorObject *executor = make_executor_from_uops(buffer, &empty);
    if (executor == NULL) {
        return -1;
    }
    Py_INCREF(self);
    Py_SET_TYPE(executor, &_PyCounterExecutor_Type);
    *exec_ptr = executor;
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

/* The following table is generated from this script:
print(f'static_assert(UOP_MAX_TRACE_LENGTH == {UOP_MAX_TRACE_LENGTH}, "COLD_EXITS must be regenerated");\n')
print("static _PyColdExitObject COLD_EXITS[] = {")
for i in range(UOP_MAX_TRACE_LENGTH):
    print(f"    [{i}] = {{ .base = {{ PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = {{ 0 }},"
          f" .trace = &COLD_EXITS[{i}].uop }}, .uop.opcode = _COLD_EXIT, .uop.oparg = {i} }},")
print("};")
*/

static_assert(UOP_MAX_TRACE_LENGTH == 512, "COLD_EXITS must be regenerated");

static _PyColdExitObject COLD_EXITS[] = {
    [0] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[0].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 0 },
    [1] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[1].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 1 },
    [2] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[2].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 2 },
    [3] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[3].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 3 },
    [4] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[4].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 4 },
    [5] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[5].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 5 },
    [6] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[6].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 6 },
    [7] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[7].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 7 },
    [8] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[8].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 8 },
    [9] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[9].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 9 },
    [10] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[10].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 10 },
    [11] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[11].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 11 },
    [12] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[12].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 12 },
    [13] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[13].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 13 },
    [14] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[14].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 14 },
    [15] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[15].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 15 },
    [16] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[16].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 16 },
    [17] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[17].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 17 },
    [18] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[18].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 18 },
    [19] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[19].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 19 },
    [20] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[20].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 20 },
    [21] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[21].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 21 },
    [22] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[22].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 22 },
    [23] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[23].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 23 },
    [24] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[24].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 24 },
    [25] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[25].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 25 },
    [26] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[26].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 26 },
    [27] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[27].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 27 },
    [28] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[28].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 28 },
    [29] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[29].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 29 },
    [30] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[30].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 30 },
    [31] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[31].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 31 },
    [32] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[32].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 32 },
    [33] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[33].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 33 },
    [34] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[34].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 34 },
    [35] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[35].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 35 },
    [36] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[36].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 36 },
    [37] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[37].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 37 },
    [38] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[38].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 38 },
    [39] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[39].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 39 },
    [40] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[40].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 40 },
    [41] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[41].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 41 },
    [42] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[42].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 42 },
    [43] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[43].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 43 },
    [44] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[44].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 44 },
    [45] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[45].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 45 },
    [46] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[46].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 46 },
    [47] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[47].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 47 },
    [48] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[48].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 48 },
    [49] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[49].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 49 },
    [50] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[50].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 50 },
    [51] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[51].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 51 },
    [52] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[52].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 52 },
    [53] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[53].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 53 },
    [54] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[54].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 54 },
    [55] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[55].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 55 },
    [56] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[56].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 56 },
    [57] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[57].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 57 },
    [58] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[58].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 58 },
    [59] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[59].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 59 },
    [60] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[60].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 60 },
    [61] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[61].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 61 },
    [62] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[62].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 62 },
    [63] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[63].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 63 },
    [64] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[64].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 64 },
    [65] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[65].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 65 },
    [66] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[66].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 66 },
    [67] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[67].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 67 },
    [68] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[68].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 68 },
    [69] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[69].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 69 },
    [70] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[70].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 70 },
    [71] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[71].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 71 },
    [72] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[72].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 72 },
    [73] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[73].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 73 },
    [74] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[74].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 74 },
    [75] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[75].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 75 },
    [76] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[76].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 76 },
    [77] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[77].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 77 },
    [78] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[78].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 78 },
    [79] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[79].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 79 },
    [80] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[80].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 80 },
    [81] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[81].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 81 },
    [82] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[82].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 82 },
    [83] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[83].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 83 },
    [84] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[84].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 84 },
    [85] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[85].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 85 },
    [86] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[86].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 86 },
    [87] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[87].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 87 },
    [88] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[88].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 88 },
    [89] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[89].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 89 },
    [90] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[90].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 90 },
    [91] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[91].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 91 },
    [92] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[92].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 92 },
    [93] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[93].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 93 },
    [94] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[94].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 94 },
    [95] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[95].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 95 },
    [96] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[96].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 96 },
    [97] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[97].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 97 },
    [98] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[98].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 98 },
    [99] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[99].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 99 },
    [100] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[100].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 100 },
    [101] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[101].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 101 },
    [102] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[102].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 102 },
    [103] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[103].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 103 },
    [104] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[104].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 104 },
    [105] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[105].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 105 },
    [106] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[106].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 106 },
    [107] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[107].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 107 },
    [108] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[108].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 108 },
    [109] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[109].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 109 },
    [110] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[110].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 110 },
    [111] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[111].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 111 },
    [112] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[112].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 112 },
    [113] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[113].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 113 },
    [114] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[114].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 114 },
    [115] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[115].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 115 },
    [116] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[116].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 116 },
    [117] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[117].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 117 },
    [118] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[118].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 118 },
    [119] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[119].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 119 },
    [120] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[120].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 120 },
    [121] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[121].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 121 },
    [122] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[122].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 122 },
    [123] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[123].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 123 },
    [124] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[124].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 124 },
    [125] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[125].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 125 },
    [126] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[126].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 126 },
    [127] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[127].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 127 },
    [128] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[128].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 128 },
    [129] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[129].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 129 },
    [130] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[130].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 130 },
    [131] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[131].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 131 },
    [132] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[132].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 132 },
    [133] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[133].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 133 },
    [134] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[134].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 134 },
    [135] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[135].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 135 },
    [136] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[136].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 136 },
    [137] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[137].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 137 },
    [138] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[138].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 138 },
    [139] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[139].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 139 },
    [140] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[140].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 140 },
    [141] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[141].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 141 },
    [142] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[142].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 142 },
    [143] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[143].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 143 },
    [144] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[144].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 144 },
    [145] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[145].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 145 },
    [146] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[146].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 146 },
    [147] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[147].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 147 },
    [148] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[148].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 148 },
    [149] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[149].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 149 },
    [150] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[150].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 150 },
    [151] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[151].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 151 },
    [152] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[152].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 152 },
    [153] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[153].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 153 },
    [154] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[154].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 154 },
    [155] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[155].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 155 },
    [156] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[156].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 156 },
    [157] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[157].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 157 },
    [158] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[158].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 158 },
    [159] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[159].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 159 },
    [160] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[160].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 160 },
    [161] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[161].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 161 },
    [162] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[162].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 162 },
    [163] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[163].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 163 },
    [164] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[164].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 164 },
    [165] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[165].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 165 },
    [166] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[166].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 166 },
    [167] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[167].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 167 },
    [168] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[168].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 168 },
    [169] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[169].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 169 },
    [170] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[170].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 170 },
    [171] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[171].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 171 },
    [172] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[172].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 172 },
    [173] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[173].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 173 },
    [174] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[174].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 174 },
    [175] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[175].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 175 },
    [176] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[176].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 176 },
    [177] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[177].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 177 },
    [178] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[178].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 178 },
    [179] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[179].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 179 },
    [180] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[180].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 180 },
    [181] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[181].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 181 },
    [182] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[182].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 182 },
    [183] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[183].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 183 },
    [184] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[184].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 184 },
    [185] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[185].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 185 },
    [186] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[186].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 186 },
    [187] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[187].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 187 },
    [188] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[188].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 188 },
    [189] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[189].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 189 },
    [190] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[190].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 190 },
    [191] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[191].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 191 },
    [192] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[192].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 192 },
    [193] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[193].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 193 },
    [194] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[194].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 194 },
    [195] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[195].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 195 },
    [196] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[196].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 196 },
    [197] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[197].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 197 },
    [198] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[198].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 198 },
    [199] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[199].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 199 },
    [200] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[200].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 200 },
    [201] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[201].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 201 },
    [202] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[202].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 202 },
    [203] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[203].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 203 },
    [204] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[204].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 204 },
    [205] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[205].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 205 },
    [206] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[206].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 206 },
    [207] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[207].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 207 },
    [208] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[208].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 208 },
    [209] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[209].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 209 },
    [210] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[210].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 210 },
    [211] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[211].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 211 },
    [212] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[212].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 212 },
    [213] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[213].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 213 },
    [214] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[214].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 214 },
    [215] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[215].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 215 },
    [216] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[216].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 216 },
    [217] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[217].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 217 },
    [218] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[218].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 218 },
    [219] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[219].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 219 },
    [220] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[220].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 220 },
    [221] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[221].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 221 },
    [222] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[222].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 222 },
    [223] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[223].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 223 },
    [224] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[224].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 224 },
    [225] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[225].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 225 },
    [226] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[226].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 226 },
    [227] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[227].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 227 },
    [228] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[228].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 228 },
    [229] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[229].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 229 },
    [230] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[230].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 230 },
    [231] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[231].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 231 },
    [232] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[232].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 232 },
    [233] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[233].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 233 },
    [234] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[234].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 234 },
    [235] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[235].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 235 },
    [236] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[236].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 236 },
    [237] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[237].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 237 },
    [238] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[238].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 238 },
    [239] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[239].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 239 },
    [240] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[240].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 240 },
    [241] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[241].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 241 },
    [242] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[242].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 242 },
    [243] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[243].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 243 },
    [244] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[244].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 244 },
    [245] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[245].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 245 },
    [246] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[246].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 246 },
    [247] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[247].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 247 },
    [248] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[248].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 248 },
    [249] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[249].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 249 },
    [250] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[250].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 250 },
    [251] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[251].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 251 },
    [252] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[252].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 252 },
    [253] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[253].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 253 },
    [254] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[254].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 254 },
    [255] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[255].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 255 },
    [256] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[256].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 256 },
    [257] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[257].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 257 },
    [258] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[258].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 258 },
    [259] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[259].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 259 },
    [260] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[260].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 260 },
    [261] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[261].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 261 },
    [262] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[262].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 262 },
    [263] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[263].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 263 },
    [264] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[264].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 264 },
    [265] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[265].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 265 },
    [266] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[266].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 266 },
    [267] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[267].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 267 },
    [268] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[268].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 268 },
    [269] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[269].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 269 },
    [270] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[270].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 270 },
    [271] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[271].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 271 },
    [272] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[272].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 272 },
    [273] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[273].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 273 },
    [274] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[274].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 274 },
    [275] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[275].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 275 },
    [276] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[276].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 276 },
    [277] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[277].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 277 },
    [278] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[278].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 278 },
    [279] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[279].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 279 },
    [280] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[280].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 280 },
    [281] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[281].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 281 },
    [282] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[282].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 282 },
    [283] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[283].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 283 },
    [284] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[284].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 284 },
    [285] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[285].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 285 },
    [286] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[286].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 286 },
    [287] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[287].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 287 },
    [288] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[288].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 288 },
    [289] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[289].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 289 },
    [290] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[290].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 290 },
    [291] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[291].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 291 },
    [292] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[292].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 292 },
    [293] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[293].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 293 },
    [294] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[294].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 294 },
    [295] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[295].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 295 },
    [296] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[296].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 296 },
    [297] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[297].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 297 },
    [298] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[298].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 298 },
    [299] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[299].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 299 },
    [300] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[300].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 300 },
    [301] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[301].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 301 },
    [302] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[302].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 302 },
    [303] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[303].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 303 },
    [304] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[304].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 304 },
    [305] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[305].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 305 },
    [306] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[306].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 306 },
    [307] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[307].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 307 },
    [308] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[308].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 308 },
    [309] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[309].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 309 },
    [310] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[310].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 310 },
    [311] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[311].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 311 },
    [312] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[312].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 312 },
    [313] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[313].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 313 },
    [314] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[314].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 314 },
    [315] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[315].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 315 },
    [316] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[316].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 316 },
    [317] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[317].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 317 },
    [318] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[318].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 318 },
    [319] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[319].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 319 },
    [320] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[320].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 320 },
    [321] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[321].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 321 },
    [322] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[322].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 322 },
    [323] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[323].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 323 },
    [324] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[324].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 324 },
    [325] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[325].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 325 },
    [326] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[326].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 326 },
    [327] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[327].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 327 },
    [328] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[328].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 328 },
    [329] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[329].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 329 },
    [330] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[330].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 330 },
    [331] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[331].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 331 },
    [332] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[332].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 332 },
    [333] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[333].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 333 },
    [334] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[334].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 334 },
    [335] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[335].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 335 },
    [336] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[336].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 336 },
    [337] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[337].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 337 },
    [338] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[338].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 338 },
    [339] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[339].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 339 },
    [340] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[340].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 340 },
    [341] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[341].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 341 },
    [342] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[342].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 342 },
    [343] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[343].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 343 },
    [344] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[344].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 344 },
    [345] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[345].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 345 },
    [346] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[346].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 346 },
    [347] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[347].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 347 },
    [348] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[348].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 348 },
    [349] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[349].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 349 },
    [350] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[350].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 350 },
    [351] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[351].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 351 },
    [352] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[352].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 352 },
    [353] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[353].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 353 },
    [354] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[354].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 354 },
    [355] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[355].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 355 },
    [356] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[356].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 356 },
    [357] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[357].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 357 },
    [358] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[358].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 358 },
    [359] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[359].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 359 },
    [360] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[360].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 360 },
    [361] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[361].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 361 },
    [362] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[362].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 362 },
    [363] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[363].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 363 },
    [364] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[364].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 364 },
    [365] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[365].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 365 },
    [366] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[366].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 366 },
    [367] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[367].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 367 },
    [368] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[368].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 368 },
    [369] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[369].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 369 },
    [370] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[370].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 370 },
    [371] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[371].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 371 },
    [372] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[372].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 372 },
    [373] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[373].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 373 },
    [374] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[374].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 374 },
    [375] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[375].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 375 },
    [376] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[376].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 376 },
    [377] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[377].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 377 },
    [378] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[378].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 378 },
    [379] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[379].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 379 },
    [380] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[380].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 380 },
    [381] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[381].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 381 },
    [382] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[382].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 382 },
    [383] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[383].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 383 },
    [384] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[384].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 384 },
    [385] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[385].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 385 },
    [386] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[386].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 386 },
    [387] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[387].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 387 },
    [388] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[388].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 388 },
    [389] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[389].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 389 },
    [390] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[390].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 390 },
    [391] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[391].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 391 },
    [392] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[392].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 392 },
    [393] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[393].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 393 },
    [394] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[394].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 394 },
    [395] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[395].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 395 },
    [396] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[396].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 396 },
    [397] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[397].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 397 },
    [398] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[398].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 398 },
    [399] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[399].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 399 },
    [400] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[400].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 400 },
    [401] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[401].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 401 },
    [402] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[402].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 402 },
    [403] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[403].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 403 },
    [404] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[404].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 404 },
    [405] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[405].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 405 },
    [406] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[406].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 406 },
    [407] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[407].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 407 },
    [408] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[408].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 408 },
    [409] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[409].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 409 },
    [410] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[410].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 410 },
    [411] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[411].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 411 },
    [412] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[412].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 412 },
    [413] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[413].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 413 },
    [414] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[414].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 414 },
    [415] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[415].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 415 },
    [416] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[416].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 416 },
    [417] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[417].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 417 },
    [418] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[418].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 418 },
    [419] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[419].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 419 },
    [420] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[420].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 420 },
    [421] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[421].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 421 },
    [422] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[422].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 422 },
    [423] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[423].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 423 },
    [424] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[424].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 424 },
    [425] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[425].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 425 },
    [426] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[426].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 426 },
    [427] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[427].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 427 },
    [428] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[428].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 428 },
    [429] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[429].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 429 },
    [430] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[430].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 430 },
    [431] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[431].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 431 },
    [432] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[432].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 432 },
    [433] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[433].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 433 },
    [434] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[434].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 434 },
    [435] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[435].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 435 },
    [436] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[436].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 436 },
    [437] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[437].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 437 },
    [438] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[438].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 438 },
    [439] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[439].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 439 },
    [440] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[440].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 440 },
    [441] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[441].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 441 },
    [442] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[442].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 442 },
    [443] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[443].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 443 },
    [444] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[444].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 444 },
    [445] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[445].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 445 },
    [446] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[446].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 446 },
    [447] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[447].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 447 },
    [448] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[448].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 448 },
    [449] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[449].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 449 },
    [450] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[450].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 450 },
    [451] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[451].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 451 },
    [452] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[452].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 452 },
    [453] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[453].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 453 },
    [454] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[454].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 454 },
    [455] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[455].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 455 },
    [456] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[456].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 456 },
    [457] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[457].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 457 },
    [458] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[458].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 458 },
    [459] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[459].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 459 },
    [460] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[460].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 460 },
    [461] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[461].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 461 },
    [462] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[462].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 462 },
    [463] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[463].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 463 },
    [464] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[464].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 464 },
    [465] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[465].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 465 },
    [466] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[466].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 466 },
    [467] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[467].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 467 },
    [468] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[468].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 468 },
    [469] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[469].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 469 },
    [470] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[470].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 470 },
    [471] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[471].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 471 },
    [472] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[472].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 472 },
    [473] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[473].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 473 },
    [474] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[474].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 474 },
    [475] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[475].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 475 },
    [476] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[476].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 476 },
    [477] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[477].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 477 },
    [478] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[478].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 478 },
    [479] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[479].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 479 },
    [480] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[480].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 480 },
    [481] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[481].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 481 },
    [482] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[482].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 482 },
    [483] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[483].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 483 },
    [484] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[484].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 484 },
    [485] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[485].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 485 },
    [486] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[486].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 486 },
    [487] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[487].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 487 },
    [488] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[488].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 488 },
    [489] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[489].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 489 },
    [490] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[490].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 490 },
    [491] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[491].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 491 },
    [492] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[492].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 492 },
    [493] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[493].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 493 },
    [494] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[494].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 494 },
    [495] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[495].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 495 },
    [496] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[496].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 496 },
    [497] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[497].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 497 },
    [498] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[498].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 498 },
    [499] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[499].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 499 },
    [500] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[500].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 500 },
    [501] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[501].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 501 },
    [502] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[502].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 502 },
    [503] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[503].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 503 },
    [504] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[504].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 504 },
    [505] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[505].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 505 },
    [506] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[506].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 506 },
    [507] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[507].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 507 },
    [508] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[508].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 508 },
    [509] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[509].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 509 },
    [510] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[510].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 510 },
    [511] = { .base = { PyVarObject_HEAD_INIT(&_ColdExit_Type, 0) .vm_data = { 0 }, .trace = &COLD_EXITS[511].uop }, .uop.opcode = _COLD_EXIT, .uop.oparg = 511 },
};

