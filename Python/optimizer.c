#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_opcode.h"
#include "opcode_metadata.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uops.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

static bool
has_space_for_executor(PyCodeObject *code, _Py_CODEUNIT *instr)
{
    if (instr->op.code == ENTER_EXECUTOR) {
        return true;
    }
    if (code->co_executors == NULL) {
        return true;
    }
    return code->co_executors->size < 256;
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
        assert(size < 256);
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
        assert(index < 256);
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
    return executor->execute(executor, frame, stack_pointer);
jump_to_destination:
    frame->prev_instr = dest - 1;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return frame;
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

#ifdef Py_DEBUG
   /* For debugging the interpreter: */
#  define LLTRACE  1      /* Low-level trace feature */
#endif

static void
uop_dealloc(_PyUOpExecutorObject *self) {
    PyObject_Free(self);
}

static PyTypeObject UOpExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_executor",
    .tp_basicsize = sizeof(_PyUOpExecutorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)uop_dealloc,
};

static int
translate_bytecode_to_trace(
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    _PyUOpInstruction *trace,
    int max_length)
{
#ifdef LLTRACE
    char *uop_debug = Py_GETENV("PYTHONUOPSDEBUG");
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
    if (lltrace >= 4) {
        fprintf(stderr,
                "Optimizing %s (%s:%d) at offset %ld\n",
                PyUnicode_AsUTF8(code->co_qualname),
                PyUnicode_AsUTF8(code->co_filename),
                code->co_firstlineno,
                (long)(instr - (_Py_CODEUNIT *)code->co_code_adaptive));
    }
#define ADD_TO_TRACE(OPCODE, OPERAND) \
        if (lltrace >= 2) { \
            const char *opname = (OPCODE) < 256 ? _PyOpcode_OpName[(OPCODE)] : _PyOpcode_uop_name[(OPCODE)]; \
            fprintf(stderr, "  ADD_TO_TRACE(%s, %" PRIu64 ")\n", opname, (uint64_t)(OPERAND)); \
        } \
        trace[trace_length].opcode = (OPCODE); \
        trace[trace_length].operand = (OPERAND); \
        trace_length++;
#else
#define ADD_TO_TRACE(OPCODE, OPERAND) \
        trace[trace_length].opcode = (OPCODE); \
        trace[trace_length].operand = (OPERAND); \
        trace_length++;
#endif

    int trace_length = 0;
    // Always reserve space for one uop, plus SET_UP, plus EXIT_TRACE
    while (trace_length + 3 <= max_length) {
        int opcode = instr->op.code;
        uint64_t operand = instr->op.arg;
        switch (opcode) {
            case LOAD_FAST_LOAD_FAST:
            {
                // Reserve space for two uops (+ SETUP + EXIT_TRACE)
                if (trace_length + 4 > max_length) {
                    goto done;
                }
                uint64_t oparg1 = operand >> 4;
                uint64_t oparg2 = operand & 15;
                ADD_TO_TRACE(LOAD_FAST, oparg1);
                ADD_TO_TRACE(LOAD_FAST, oparg2);
                break;
            }
            default:
            {
                const struct opcode_macro_expansion *expansion = &_PyOpcode_macro_expansion[opcode];
                if (expansion->nuops > 0) {
                    // Reserve space for nuops (+ SETUP + EXIT_TRACE)
                    int nuops = expansion->nuops;
                    if (trace_length + nuops + 2 > max_length) {
                        goto done;
                    }
                    for (int i = 0; i < nuops; i++) {
                        int offset = expansion->uops[i].offset;
                        switch (expansion->uops[i].size) {
                            case 0:
                                break;
                            case 1:
                                operand = read_u16(&instr[offset].cache);
                                break;
                            case 2:
                                operand = read_u32(&instr[offset].cache);
                                break;
                            case 4:
                                operand = read_u64(&instr[offset].cache);
                                break;
                            default:
                                fprintf(stderr,
                                        "opcode=%d, operand=%" PRIu64 "; nuops=%d, i=%d; size=%d, offset=%d\n",
                                        opcode, operand, nuops, i,
                                        expansion->uops[i].size,
                                        expansion->uops[i].offset);
                                Py_FatalError("garbled expansion");
                        }
                        ADD_TO_TRACE(expansion->uops[i].uop, operand);
                        assert(expansion->uops[0].size == 0);  // TODO
                    }
                    break;
                }
                // fprintf(stderr, "Unsupported opcode %d\n", opcode);
                goto done;  // Break out of while loop
            }
        }
        instr++;
        // Add cache size for opcode
        instr += _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
        ADD_TO_TRACE(SET_IP, (int)(instr - (_Py_CODEUNIT *)code->co_code_adaptive));
    }
done:
    if (trace_length > 0) {
        ADD_TO_TRACE(EXIT_TRACE, 0);
#ifdef LLTRACE
        if (lltrace >= 1) {
            fprintf(stderr,
                    "Created a trace for %s (%s:%d) at offset %ld -- length %d\n",
                    PyUnicode_AsUTF8(code->co_qualname),
                    PyUnicode_AsUTF8(code->co_filename),
                    code->co_firstlineno,
                    (long)(instr - (_Py_CODEUNIT *)code->co_code_adaptive),
                    trace_length);
        }
#endif
    }
    else {
#ifdef LLTRACE
        if (lltrace >= 4) {
            fprintf(stderr,
                    "No trace for %s (%s:%d) at offset %ld\n",
                    PyUnicode_AsUTF8(code->co_qualname),
                    PyUnicode_AsUTF8(code->co_filename),
                    code->co_firstlineno,
                    (long)(instr - (_Py_CODEUNIT *)code->co_code_adaptive));
        }
#endif
    }
    return trace_length;

#undef ADD_TO_TRACE
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
    _PyUOpExecutorObject *executor = (_PyUOpExecutorObject *)_PyObject_New(&UOpExecutor_Type);
    if (executor == NULL) {
        return -1;
    }
    executor->base.execute = _PyUopExecute;
    memcpy(executor->trace, trace, trace_length * sizeof(_PyUOpInstruction));
    *exec_ptr = (_PyExecutorObject *)executor;
    return 1;
}

static PyTypeObject UOpOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_optimizer",
    .tp_basicsize = sizeof(_PyOptimizerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
};

PyObject *
PyUnstable_Optimizer_NewUOpOptimizer(void)
{
    _PyOptimizerObject *opt = (_PyOptimizerObject *)_PyObject_New(&UOpOptimizer_Type);
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
