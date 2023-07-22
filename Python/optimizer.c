#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_opcode.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
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
    _PyExecutorObject **exec)
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

_PyInterpreterFrame *
_PyOptimizer_BackEdge(_PyInterpreterFrame *frame, _Py_CODEUNIT *src, _Py_CODEUNIT *dest, PyObject **stack_pointer)
{
    assert(src->op.code == JUMP_BACKWARD);
    PyCodeObject *code = (PyCodeObject *)frame->f_executable;
    assert(PyCode_Check(code));
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!has_space_for_executor(code, src)) {
        goto jump_to_destination;
    }
    _PyOptimizerObject *opt = interp->optimizer;
    _PyExecutorObject *executor = NULL;
    int err = opt->optimize(opt, code, dest, &executor);
    if (err <= 0) {
        assert(executor == NULL);
        if (err < 0) {
            return NULL;
        }
        goto jump_to_destination;
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
        goto jump_to_destination;
    }
    insert_executor(code, src, index, executor);
    assert(frame->prev_instr == src);
    frame->prev_instr = dest - 1;
    return executor->execute(executor, frame, stack_pointer);
jump_to_destination:
    frame->prev_instr = dest - 1;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return frame;
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
    Py_DECREF(self->optimizer);
    PyObject_Free(self);
}

static PyTypeObject CounterExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "counting_executor",
    .tp_basicsize = sizeof(_PyCounterExecutorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)counter_dealloc,
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
    _PyExecutorObject **exec_ptr)
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
};

PyObject *
PyUnstable_Optimizer_NewCounter(void)
{
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
    int count = 0;
    for (; count < _Py_UOP_MAX_TRACE_LENGTH; count++) {
        if (self->trace[count].opcode == 0) {
            break;
        }
    }
    return count;
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

static PyTypeObject UOpExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_executor",
    .tp_basicsize = sizeof(_PyUOpExecutorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)uop_dealloc,
    .tp_as_sequence = &uop_as_sequence,
};

static int
translate_bytecode_to_trace(
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyUOpInstruction *trace,
    int buffer_size)
{
    _Py_CODEUNIT *initial_instr = instr;
    int trace_length = 0;
    int max_length = buffer_size;
    int reserved = 0;

#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV("PYTHONUOPSDEBUG");
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
#endif

#ifdef Py_DEBUG
#define DPRINTF(level, ...) \
    if (lltrace >= (level)) { fprintf(stderr, __VA_ARGS__); }
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
        goto done; \
    } \
    reserved = (n);  // Keep ADD_TO_TRACE / ADD_TO_STUB honest

// Reserve space for main+stub uops, plus 2 for SAVE_IP and EXIT_TRACE
#define RESERVE(main, stub) RESERVE_RAW((main) + (stub) + 2, uop_name(opcode))

    DPRINTF(4,
            "Optimizing %s (%s:%d) at byte offset %d\n",
            PyUnicode_AsUTF8(code->co_qualname),
            PyUnicode_AsUTF8(code->co_filename),
            code->co_firstlineno,
            2 * INSTR_IP(initial_instr, code));

    for (;;) {
        RESERVE_RAW(2, "epilogue");  // Always need space for SAVE_IP and EXIT_TRACE
        ADD_TO_TRACE(SAVE_IP, INSTR_IP(instr, code), 0);

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
                ADD_TO_TRACE(IS_NONE, 0, 0);
                opcode = POP_JUMP_IF_TRUE;
                goto pop_jump_if_bool;
            }

            case POP_JUMP_IF_NOT_NONE:
            {
                RESERVE(2, 2);
                ADD_TO_TRACE(IS_NONE, 0, 0);
                opcode = POP_JUMP_IF_FALSE;
                goto pop_jump_if_bool;
            }

            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            {
pop_jump_if_bool:
                // Assume jump unlikely (TODO: handle jump likely case)
                RESERVE(1, 2);
                _Py_CODEUNIT *target_instr =
                    instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]] + oparg;
                max_length -= 2;  // Really the start of the stubs
                uint32_t uopcode = opcode == POP_JUMP_IF_TRUE ?
                    _POP_JUMP_IF_TRUE : _POP_JUMP_IF_FALSE;
                ADD_TO_TRACE(uopcode, max_length, 0);
                ADD_TO_STUB(max_length, SAVE_IP, INSTR_IP(target_instr, code), 0);
                ADD_TO_STUB(max_length + 1, EXIT_TRACE, 0, 0);
                break;
            }

            case JUMP_BACKWARD:
            {
                if (instr + 2 - oparg == initial_instr) {
                    RESERVE(1, 0);
                    ADD_TO_TRACE(JUMP_TO_TOP, 0, 0);
                }
                else {
                    DPRINTF(2, "JUMP_BACKWARD not to top ends trace\n");
                }
                goto done;
            }

            case JUMP_FORWARD:
            {
                RESERVE(0, 0);
                // This will emit two SAVE_IP instructions; leave it to the optimizer
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
                ADD_TO_STUB(max_length + 1, SAVE_IP, INSTR_IP(target_instr, code), 0);
                ADD_TO_STUB(max_length + 2, EXIT_TRACE, 0, 0);
                break;
            }

            default:
            {
                const struct opcode_macro_expansion *expansion = &_PyOpcode_macro_expansion[opcode];
                if (expansion->nuops > 0) {
                    // Reserve space for nuops (+ SAVE_IP + EXIT_TRACE)
                    int nuops = expansion->nuops;
                    RESERVE(nuops, 0);
                    uint32_t orig_oparg = oparg;  // For OPARG_TOP/BOTTOM
                    for (int i = 0; i < nuops; i++) {
                        oparg = orig_oparg;
                        uint64_t operand = 0;
                        int offset = expansion->uops[i].offset;
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
                            default:
                                fprintf(stderr,
                                        "opcode=%d, oparg=%d; nuops=%d, i=%d; size=%d, offset=%d\n",
                                        opcode, oparg, nuops, i,
                                        expansion->uops[i].size,
                                        expansion->uops[i].offset);
                                Py_FatalError("garbled expansion");
                        }
                        ADD_TO_TRACE(expansion->uops[i].uop, oparg, operand);
                    }
                    break;
                }
                DPRINTF(2, "Unsupported opcode %s\n", uop_name(opcode));
                goto done;  // Break out of loop
            }  // End default

        }  // End switch (opcode)

        instr++;
        // Add cache size for opcode
        instr += _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
    }  // End for (;;)

done:
    // Skip short traces like SAVE_IP, LOAD_FAST, SAVE_IP, EXIT_TRACE
    if (trace_length > 3) {
        ADD_TO_TRACE(EXIT_TRACE, 0, 0);
        DPRINTF(1,
                "Created a trace for %s (%s:%d) at byte offset %d -- length %d\n",
                PyUnicode_AsUTF8(code->co_qualname),
                PyUnicode_AsUTF8(code->co_filename),
                code->co_firstlineno,
                2 * INSTR_IP(initial_instr, code),
                trace_length);
        if (max_length < buffer_size && trace_length < max_length) {
            // Move the stubs back to be immediately after the main trace
            // (which ends at trace_length)
            DPRINTF(2,
                    "Moving %d stub uops back by %d\n",
                    buffer_size - max_length,
                    max_length - trace_length);
            memmove(trace + trace_length,
                    trace + max_length,
                    (buffer_size - max_length) * sizeof(_PyUOpInstruction));
            // Patch up the jump targets
            for (int i = 0; i < trace_length; i++) {
                if (trace[i].opcode == _POP_JUMP_IF_FALSE ||
                    trace[i].opcode == _POP_JUMP_IF_TRUE)
                {
                    int target = trace[i].oparg;
                    if (target >= max_length) {
                        target += trace_length - max_length;
                        trace[i].oparg = target;
                    }
                }
            }
        }
        trace_length += buffer_size - max_length;
        return trace_length;
    }
    else {
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
uop_optimize(
    _PyOptimizerObject *self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyExecutorObject **exec_ptr)
{
    _PyUOpInstruction trace[_Py_UOP_MAX_TRACE_LENGTH];
    int trace_length = translate_bytecode_to_trace(code, instr, trace, _Py_UOP_MAX_TRACE_LENGTH);
    if (trace_length <= 0) {
        // Error or nothing translated
        return trace_length;
    }
    OBJECT_STAT_INC(optimization_traces_created);
    _PyUOpExecutorObject *executor = PyObject_New(_PyUOpExecutorObject, &UOpExecutor_Type);
    if (executor == NULL) {
        return -1;
    }
    executor->base.execute = _PyUopExecute;
    memcpy(executor->trace, trace, trace_length * sizeof(_PyUOpInstruction));
        if (trace_length < _Py_UOP_MAX_TRACE_LENGTH) {
            executor->trace[trace_length].opcode = 0;  // Sentinel
        }
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
    _PyOptimizerObject *opt = PyObject_New(_PyOptimizerObject, &UOpOptimizer_Type);
    if (opt == NULL) {
        return NULL;
    }
    opt->optimize = uop_optimize;
    opt->resume_threshold = UINT16_MAX;
    // Need at least 3 iterations to settle specializations.
    // A few lower bits of the counter are reserved for other flags.
    opt->backedge_threshold = 3 << OPTIMIZER_BITS_IN_COUNTER;
    return (PyObject *)opt;
}
