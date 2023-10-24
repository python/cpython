#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_bitutils.h"        // _Py_popcount32()
#include "pycore_opcode_metadata.h" // _PyOpcode_OpName()
#include "pycore_opcode_utils.h"  // MAX_REAL_OPCODE
#include "pycore_optimizer.h"     // _Py_uop_analyze_and_optimize()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uops.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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
    PyErr_Format(PyExc_SystemError, "Should never call error_optimize");
    return -1;
}

static PyTypeObject DefaultOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "noop_optimizer",
    .tp_basicsize = sizeof(_PyOptimizerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
};

_PyOptimizerObject _PyOptimizer_Default = {
    PyObject_HEAD_INIT(&DefaultOptimizer_Type)
    .optimize = error_optimize,
    .resume_threshold = UINT16_MAX,
    .backedge_threshold = UINT16_MAX,
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

static PyMemberDef counter_members[] = {
    { "valid", Py_T_UBYTE, offsetof(_PyCounterExecutorObject, executor.vm_data.valid), Py_READONLY, "is valid?" },
    { NULL }
};

static PyTypeObject CounterExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "counting_executor",
    .tp_basicsize = sizeof(_PyCounterExecutorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)counter_dealloc,
    .tp_members = counter_members,
};

static _PyInterpreterFrame *
counter_execute(_PyExecutorObject *self, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    ((_PyCounterExecutorObject *)self)->optimizer->count++;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    frame->prev_instr = ((_PyCounterExecutorObject *)self)->next_instr - 1;
    Py_DECREF(self);
    return frame;
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
    _PyCounterExecutorObject *executor = (_PyCounterExecutorObject *)_PyObject_New(&CounterExecutor_Type);
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

static PyMethodDef counter_methods[] = {
    { "get_count", counter_get_counter, METH_NOARGS, NULL },
    { NULL, NULL },
};

static PyTypeObject CounterOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "Counter optimizer",
    .tp_basicsize = sizeof(_PyCounterOptimizerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_methods = counter_methods,
    .tp_dealloc = (destructor)PyObject_Del,
};

PyObject *
PyUnstable_Optimizer_NewCounter(void)
{
    PyType_Ready(&CounterExecutor_Type);
    PyType_Ready(&CounterOptimizer_Type);
    _PyCounterOptimizerObject *opt = (_PyCounterOptimizerObject *)_PyObject_New(&CounterOptimizer_Type);
    if (opt == NULL) {
        return NULL;
    }
    opt->base.optimize = counter_optimize;
    opt->base.resume_threshold = UINT16_MAX;
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

static const char *
uop_name(int index) {
    if (index <= MAX_REAL_OPCODE) {
        return _PyOpcode_OpName[index];
    }
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
    const char *name = uop_name(self->trace[index].opcode);
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


static PyMemberDef uop_members[] = {
    { "valid", Py_T_UBYTE, offsetof(_PyUOpExecutorObject, base.vm_data.valid), Py_READONLY, "is valid?" },
    { NULL }
};

static PyTypeObject UOpExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_executor",
    .tp_basicsize = sizeof(_PyUOpExecutorObject) - sizeof(_PyUOpInstruction),
    .tp_itemsize = sizeof(_PyUOpInstruction),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)uop_dealloc,
    .tp_as_sequence = &uop_as_sequence,
    .tp_members = uop_members,
};

static int
move_stubs(
    _PyUOpInstruction *trace,
    int trace_length,
    int stubs_start,
    int stubs_end
)
{
    memmove(trace + trace_length,
            trace + stubs_start,
            (stubs_end - stubs_start) * sizeof(_PyUOpInstruction));
    // Patch up the jump targets
    for (int i = 0; i < trace_length; i++) {
        if (trace[i].opcode == _POP_JUMP_IF_FALSE ||
            trace[i].opcode == _POP_JUMP_IF_TRUE)
        {
            int target = trace[i].oparg;
            if (target >= stubs_start) {
                target += trace_length - stubs_start;
                trace[i].oparg = target;
            }
        }
    }
    return trace_length + stubs_end - stubs_start;
}

#define TRACE_STACK_SIZE 5

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
    int reserved = 0;
    struct {
        PyCodeObject *code;
        _Py_CODEUNIT *instr;
    } trace_stack[TRACE_STACK_SIZE];
    int trace_stack_depth = 0;

#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV("PYTHONUOPSDEBUG");
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
#endif

#ifdef Py_DEBUG
#define DPRINTF(level, ...) \
    if (lltrace >= (level)) { printf(__VA_ARGS__); }
#else
#define DPRINTF(level, ...)
#endif

#define ADD_TO_TRACE(OPCODE, OPARG, OPERAND) \
    DPRINTF(2, \
            "  ADD_TO_TRACE(%s, %d, %" PRIu64 ")\n", \
            uop_name(OPCODE), \
            (OPARG), \
            (uint64_t)(OPERAND)); \
    assert(trace_length < max_length); \
    assert(reserved > 0); \
    reserved--; \
    trace[trace_length].opcode = (OPCODE); \
    trace[trace_length].oparg = (OPARG); \
    trace[trace_length].operand = (OPERAND); \
    trace_length++;

#define INSTR_IP(INSTR, CODE) \
    ((uint32_t)((INSTR) - ((_Py_CODEUNIT *)(CODE)->co_code_adaptive)))

#define ADD_TO_STUB(INDEX, OPCODE, OPARG, OPERAND) \
    DPRINTF(2, "    ADD_TO_STUB(%d, %s, %d, %" PRIu64 ")\n", \
            (INDEX), \
            uop_name(OPCODE), \
            (OPARG), \
            (uint64_t)(OPERAND)); \
    assert(reserved > 0); \
    reserved--; \
    trace[(INDEX)].opcode = (OPCODE); \
    trace[(INDEX)].oparg = (OPARG); \
    trace[(INDEX)].operand = (OPERAND);

// Reserve space for n uops
#define RESERVE_RAW(n, opname) \
    if (trace_length + (n) > max_length) { \
        DPRINTF(2, "No room for %s (need %d, got %d)\n", \
                (opname), (n), max_length - trace_length); \
        OPT_STAT_INC(trace_too_long); \
        goto done; \
    } \
    reserved = (n);  // Keep ADD_TO_TRACE / ADD_TO_STUB honest

// Reserve space for main+stub uops, plus 2 for _SET_IP and _EXIT_TRACE
#define RESERVE(main, stub) RESERVE_RAW((main) + (stub) + 2, uop_name(opcode))

// Trace stack operations (used by _PUSH_FRAME, _POP_FRAME)
#define TRACE_STACK_PUSH() \
    if (trace_stack_depth >= TRACE_STACK_SIZE) { \
        DPRINTF(2, "Trace stack overflow\n"); \
        OPT_STAT_INC(trace_stack_overflow); \
        ADD_TO_TRACE(_SET_IP, 0, 0); \
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

top:  // Jump here after _PUSH_FRAME or likely branches
    for (;;) {
        RESERVE_RAW(2, "epilogue");  // Always need space for _SET_IP and _EXIT_TRACE
        ADD_TO_TRACE(_SET_IP, INSTR_IP(instr, code), 0);

        uint32_t opcode = instr->op.code;
        uint32_t oparg = instr->op.arg;
        uint32_t extras = 0;

        while (opcode == EXTENDED_ARG) {
            instr++;
            extras += 1;
            opcode = instr->op.code;
            oparg = (oparg << 8) | instr->op.arg;
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
            {
                RESERVE(2, 2);
                ADD_TO_TRACE(_IS_NONE, 0, 0);
                opcode = POP_JUMP_IF_TRUE;
                goto pop_jump_if_bool;
            }

            case POP_JUMP_IF_NOT_NONE:
            {
                RESERVE(2, 2);
                ADD_TO_TRACE(_IS_NONE, 0, 0);
                opcode = POP_JUMP_IF_FALSE;
                goto pop_jump_if_bool;
            }

            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            {
pop_jump_if_bool:
                RESERVE(1, 2);
                max_length -= 2;  // Really the start of the stubs
                int counter = instr[1].cache;
                int bitcount = _Py_popcount32(counter);
                bool jump_likely = bitcount > 8;
                bool jump_sense = opcode == POP_JUMP_IF_TRUE;
                uint32_t uopcode = jump_sense ^ jump_likely ?
                    _POP_JUMP_IF_TRUE : _POP_JUMP_IF_FALSE;
                _Py_CODEUNIT *next_instr = instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
                _Py_CODEUNIT *target_instr = next_instr + oparg;
                _Py_CODEUNIT *stub_target = jump_likely ? next_instr : target_instr;
                DPRINTF(4, "%s(%d): counter=%x, bitcount=%d, likely=%d, sense=%d, uopcode=%s\n",
                        uop_name(opcode), oparg,
                        counter, bitcount, jump_likely, jump_sense, uop_name(uopcode));
                ADD_TO_TRACE(uopcode, max_length, 0);
                ADD_TO_STUB(max_length, _SET_IP, INSTR_IP(stub_target, code), 0);
                ADD_TO_STUB(max_length + 1, _EXIT_TRACE, 0, 0);
                if (jump_likely) {
                    DPRINTF(2, "Jump likely (%x = %d bits), continue at byte offset %d\n",
                            instr[1].cache, bitcount, 2 * INSTR_IP(target_instr, code));
                    instr = target_instr;
                    goto top;
                }
                break;
            }

            case JUMP_BACKWARD:
            {
                if (instr + 2 - oparg == initial_instr && code == initial_code) {
                    RESERVE(1, 0);
                    ADD_TO_TRACE(_JUMP_TO_TOP, 0, 0);
                }
                else {
                    OPT_STAT_INC(inner_loop);
                    DPRINTF(2, "JUMP_BACKWARD not to top ends trace\n");
                }
                goto done;
            }

            case JUMP_FORWARD:
            {
                RESERVE(0, 0);
                // This will emit two _SET_IP instructions; leave it to the optimizer
                instr += oparg;
                break;
            }

            case FOR_ITER_LIST:
            case FOR_ITER_TUPLE:
            case FOR_ITER_RANGE:
            {
                RESERVE(4, 3);
                int check_op, exhausted_op, next_op;
                switch (opcode) {
                    case FOR_ITER_LIST:
                        check_op = _ITER_CHECK_LIST;
                        exhausted_op = _IS_ITER_EXHAUSTED_LIST;
                        next_op = _ITER_NEXT_LIST;
                        break;
                    case FOR_ITER_TUPLE:
                        check_op = _ITER_CHECK_TUPLE;
                        exhausted_op = _IS_ITER_EXHAUSTED_TUPLE;
                        next_op = _ITER_NEXT_TUPLE;
                        break;
                    case FOR_ITER_RANGE:
                        check_op = _ITER_CHECK_RANGE;
                        exhausted_op = _IS_ITER_EXHAUSTED_RANGE;
                        next_op = _ITER_NEXT_RANGE;
                        break;
                    default:
                        Py_UNREACHABLE();
                }
                // Assume jump unlikely (can a for-loop exit be likely?)
                _Py_CODEUNIT *target_instr =  // +1 at the end skips over END_FOR
                    instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]] + oparg + 1;
                max_length -= 3;  // Really the start of the stubs
                ADD_TO_TRACE(check_op, 0, 0);
                ADD_TO_TRACE(exhausted_op, 0, 0);
                ADD_TO_TRACE(_POP_JUMP_IF_TRUE, max_length, 0);
                ADD_TO_TRACE(next_op, 0, 0);

                ADD_TO_STUB(max_length + 0, POP_TOP, 0, 0);
                ADD_TO_STUB(max_length + 1, _SET_IP, INSTR_IP(target_instr, code), 0);
                ADD_TO_STUB(max_length + 2, _EXIT_TRACE, 0, 0);
                break;
            }

            default:
            {
                const struct opcode_macro_expansion *expansion = &_PyOpcode_macro_expansion[opcode];
                if (expansion->nuops > 0) {
                    // Reserve space for nuops (+ _SET_IP + _EXIT_TRACE)
                    int nuops = expansion->nuops;
                    RESERVE(nuops, 0);
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
                                if (extras && OPCODE_HAS_JUMP(opcode)) {
                                    if (opcode == JUMP_BACKWARD_NO_INTERRUPT) {
                                        oparg -= extras;
                                    }
                                    else {
                                        assert(opcode != JUMP_BACKWARD);
                                        oparg += extras;
                                    }
                                }
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
                            case OPARG_SET_IP:  // uop=_SET_IP; oparg=next_instr-1
                                // The number of caches is smuggled in via offset:
                                assert(offset == _PyOpcode_Caches[_PyOpcode_Deopt[opcode]]);
                                oparg = INSTR_IP(instr + offset, code);
                                uop = _SET_IP;
                                break;

                            default:
                                fprintf(stderr,
                                        "opcode=%d, oparg=%d; nuops=%d, i=%d; size=%d, offset=%d\n",
                                        opcode, oparg, nuops, i,
                                        expansion->uops[i].size,
                                        expansion->uops[i].offset);
                                Py_FatalError("garbled expansion");
                        }
                        ADD_TO_TRACE(uop, oparg, operand);
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
                                    ADD_TO_TRACE(_SET_IP, 0, 0);
                                    goto done;
                                }
                                if (new_code->co_version != func_version) {
                                    // func.__code__ was updated.
                                    // Perhaps it may happen again, so don't bother tracing.
                                    // TODO: Reason about this -- is it better to bail or not?
                                    DPRINTF(2, "Bailing because co_version != func_version\n");
                                    ADD_TO_TRACE(_SET_IP, 0, 0);
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
                            ADD_TO_TRACE(_SET_IP, 0, 0);
                            goto done;
                        }
                    }
                    break;
                }
                DPRINTF(2, "Unsupported opcode %s\n", uop_name(opcode));
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
    if (trace_length > 3) {
        ADD_TO_TRACE(_EXIT_TRACE, 0, 0);
        DPRINTF(1,
                "Created a trace for %s (%s:%d) at byte offset %d -- length %d+%d\n",
                PyUnicode_AsUTF8(code->co_qualname),
                PyUnicode_AsUTF8(code->co_filename),
                code->co_firstlineno,
                2 * INSTR_IP(initial_instr, code),
                trace_length,
                buffer_size - max_length);
        if (max_length < buffer_size) {
            // There are stubs
            if (trace_length < max_length) {
                // There's a gap before the stubs
                // Move the stubs back to be immediately after the main trace
                // (which ends at trace_length)
                DPRINTF(2,
                        "Moving %d stub uops back by %d\n",
                        buffer_size - max_length,
                        max_length - trace_length);
                trace_length = move_stubs(trace, trace_length, max_length, buffer_size);
            }
            else {
                assert(trace_length == max_length);
                // There's no gap
                trace_length = buffer_size;
            }
        }
        return trace_length;
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

static int
remove_unneeded_uops(_PyUOpInstruction *trace, int trace_length)
{
    // Stage 1: Replace unneeded _SET_IP uops with NOP.
    // Note that we don't enter stubs, those SET_IPs are needed.
    int last_set_ip = -1;
    int last_instr = 0;
    bool need_ip = true;
    for (int pc = 0; pc < trace_length; pc++) {
        int opcode = trace[pc].opcode;
        if (opcode == _SET_IP) {
            if (!need_ip && last_set_ip >= 0) {
                trace[last_set_ip].opcode = NOP;
            }
            need_ip = false;
            last_set_ip = pc;
        }
        else if (opcode == _JUMP_TO_TOP || opcode == _EXIT_TRACE) {
            last_instr = pc + 1;
            break;
        }
        else {
            // If opcode has ERROR or DEOPT, set need_ip to true
            if (_PyOpcode_opcode_metadata[opcode].flags & (HAS_ERROR_FLAG | HAS_DEOPT_FLAG) || opcode == _PUSH_FRAME) {
                need_ip = true;
            }
        }
    }
    // Stage 2: Squash NOP opcodes (pre-existing or set above).
    int dest = 0;
    for (int pc = 0; pc < last_instr; pc++) {
        int opcode = trace[pc].opcode;
        if (opcode != NOP) {
            if (pc != dest) {
                trace[dest] = trace[pc];
            }
            dest++;
        }
    }
    // Stage 3: Move the stubs back.
    if (dest < last_instr) {
        int new_trace_length = move_stubs(trace, dest, last_instr, trace_length);
#ifdef Py_DEBUG
        char *uop_debug = Py_GETENV("PYTHONUOPSDEBUG");
        int lltrace = 0;
        if (uop_debug != NULL && *uop_debug >= '0') {
            lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
        }
        if (lltrace >= 2) {
            printf("Optimized trace (length %d+%d = %d, saved %d):\n",
                dest, trace_length - last_instr, new_trace_length,
                trace_length - new_trace_length);
            for (int pc = 0; pc < new_trace_length; pc++) {
                printf("%4d: (%s, %d, %" PRIu64 ")\n",
                    pc,
                    uop_name(trace[pc].opcode),
                    (trace[pc].oparg),
                    (uint64_t)(trace[pc].operand));
            }
        }
#endif
        trace_length = new_trace_length;
    }
    return trace_length;
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
    _PyUOpInstruction trace[_Py_UOP_MAX_TRACE_LENGTH];
    int trace_length = translate_bytecode_to_trace(code, instr, trace, _Py_UOP_MAX_TRACE_LENGTH, &dependencies);
    if (trace_length <= 0) {
        // Error or nothing translated
        return trace_length;
    }
    OPT_HIST(trace_length, trace_length_hist);
    OPT_STAT_INC(traces_created);
    char *uop_optimize = Py_GETENV("PYTHONUOPSOPTIMIZE");
    if (uop_optimize != NULL && *uop_optimize > '0') {
        trace_length = _Py_uop_analyze_and_optimize(code, trace, trace_length, curr_stackentries);
    }
    trace_length = remove_unneeded_uops(trace, trace_length);
    _PyUOpExecutorObject *executor = PyObject_NewVar(_PyUOpExecutorObject, &UOpExecutor_Type, trace_length);
    if (executor == NULL) {
        return -1;
    }
    OPT_HIST(trace_length, optimized_trace_length_hist);
    executor->base.execute = _PyUopExecute;
    memcpy(executor->trace, trace, trace_length * sizeof(_PyUOpInstruction));
    _Py_ExecutorInit((_PyExecutorObject *)executor, &dependencies);
    *exec_ptr = (_PyExecutorObject *)executor;
    return 1;
}

static void
uop_opt_dealloc(PyObject *self) {
    PyObject_Free(self);
}

static PyTypeObject UOpOptimizer_Type = {
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
    PyType_Ready(&UOpExecutor_Type);
    PyType_Ready(&UOpOptimizer_Type);
    _PyOptimizerObject *opt = PyObject_New(_PyOptimizerObject, &UOpOptimizer_Type);
    if (opt == NULL) {
        return NULL;
    }
    opt->optimize = uop_optimize;
    opt->resume_threshold = UINT16_MAX;
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
