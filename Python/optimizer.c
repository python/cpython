
#include "Python.h"
#include "opcode.h"
#include "pycore_dict.h"
#include "pycore_interp.h"
#include "pycore_long.h"
#include "pycore_object.h"
#include "pycore_opcode.h"
#include "pycore_pystate.h"
#include "pycore_sliceobject.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "opcode_metadata.h"
#include "ceval_macros.h"

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
    PyInterpreterState *interp = PyInterpreterState_Get();
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
    PyInterpreterState *interp = PyInterpreterState_Get();
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
    PyInterpreterState *interp = PyInterpreterState_Get();
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

///////////////////// Experimental UOp Interpreter /////////////////////

typedef struct {
    _PyOptimizerObject base;
    int traces_executed;
    int instrs_executed;
} UOpOptimizerObject;

#define MAX_TRACE_LENGTH 16

// UOp opcodes are outside the range of bytecodes or pseudo ops
#define EXIT_TRACE 512
#define SET_IP 513
// TODOL Generate these in Tools/cases_generator
#define _BINARY_OP_MULTIPLY_FLOAT 514
#define _BINARY_OP_ADD_FLOAT 515
#define _BINARY_OP_SUBTRACT_FLOAT 516
#define _BINARY_OP_MULTIPLY_INT 517
#define _BINARY_OP_ADD_INT 518
#define _BINARY_OP_SUBTRACT_INT 519
#define _BINARY_OP_ADD_UNICODE 520

typedef struct {
    int opcode;
    int oparg;
} uop_instruction;

typedef struct {
    _PyExecutorObject executor;  // Base
    UOpOptimizerObject *optimizer;
    uop_instruction trace[MAX_TRACE_LENGTH];  // TODO: variable length
} UOpExecutorObject;

static void
uop_dealloc(UOpExecutorObject *self) {
    Py_DECREF(self->optimizer);
    PyObject_Free(self);
}

static PyTypeObject UOpExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_executor",
    .tp_basicsize = sizeof(UOpExecutorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)uop_dealloc,
};

static _PyInterpreterFrame *
uop_execute(_PyExecutorObject *executor, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    UOpExecutorObject *self = (UOpExecutorObject *)executor;
    self->optimizer->traces_executed++;
    _Py_CODEUNIT *ip_offset = (_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive - 1;
    int pc = 0;
    for (;;) {
        int opcode = self->trace[pc].opcode;
        int oparg = self->trace[pc].oparg;
        // fprintf(stderr, "uop %d, oparg %d\n", opcode, oparg);
        pc++;
        self->optimizer->instrs_executed++;
        switch (opcode) {

#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0
#include "executor_cases.c.h"

            case SET_IP:
            {
                // fprintf(stderr, "SET_IP %d\n", oparg);
                frame->prev_instr = ip_offset + oparg;
                break;
            }

            case EXIT_TRACE:
            {
                // fprintf(stderr, "EXIT_TRACE\n");
                _PyFrame_SetStackPointer(frame, stack_pointer);
                Py_DECREF(self);
                return frame;
            }

            default:
            {
                fprintf(stderr, "Unknown uop %d, oparg %d\n", opcode, oparg);
                Py_FatalError("Unknown uop");
                abort();  // Unreachable
                for (;;) {}
                // Really unreachable
            }

        }
        continue;
    pop_1_error:
    pop_2_error:
    pop_3_error:
    pop_4_error:
    error:
    unbound_local_error:
        fprintf(stderr, "[Opcode %d, oparg %d]\n", opcode, oparg);
        Py_FatalError("Errors not yet supported");
    }
}

static int
translate_bytecode_to_trace(
    PyCodeObject *code,
    _Py_CODEUNIT *instr,
    uop_instruction *trace,
    int max_length)
{
#define ADD_TO_TRACE(OPCODE, OPARG) \
        /* fprintf(stderr, "ADD_TO_TRACE(%d, %d)\n", (OPCODE), (OPARG)); */ \
        trace[trace_length].opcode = (OPCODE); \
        trace[trace_length].oparg = (OPARG); \
        trace_length++;

    int trace_length = 0;
    // ALways reserve space for one uop, plus SET_UP, plus EXIT_TRACE
    while (trace_length + 3 <= max_length) {
        int opcode = instr->op.code;
        int oparg = instr->op.arg;
        switch (opcode) {
            case LOAD_FAST_LOAD_FAST:
            {
                // Reserve space for two uops (+ SETUP + EXIT_TRACE)
                if (trace_length + 4 > max_length) {
                    goto done;
                }
                int oparg1 = oparg >> 4;
                int oparg2 = oparg & 15;
                ADD_TO_TRACE(LOAD_FAST, oparg1);
                ADD_TO_TRACE(LOAD_FAST, oparg2);
                break;
            }
            default:
            {
                if (OPCODE_IS_UOP(opcode)) {
                    ADD_TO_TRACE(opcode, oparg);
                    break;
                }
                goto done;  // Break out of while loop
            }
        }
        instr++;
        ADD_TO_TRACE(SET_IP, (int)(instr - (_Py_CODEUNIT *)code->co_code_adaptive));
    }
done:
    if (trace_length > 0) {
        ADD_TO_TRACE(EXIT_TRACE, 0);
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
    uop_instruction trace[MAX_TRACE_LENGTH];
    int trace_length = translate_bytecode_to_trace(code, instr, trace, MAX_TRACE_LENGTH);
    if (trace_length <= 0) {
        // Error or nothing translated
        return trace_length;
    }
    UOpExecutorObject *executor = (UOpExecutorObject *)_PyObject_New(&UOpExecutor_Type);
    if (executor == NULL) {
        return -1;
    }
    executor->executor.execute = uop_execute;
    Py_INCREF(self);
    executor->optimizer = (UOpOptimizerObject *)self;
    memcpy(executor->trace, trace, trace_length * sizeof(uop_instruction));
    *exec_ptr = (_PyExecutorObject *)executor;
    return 1;
}

static PyMemberDef uop_members[] = {
    {"traces_executed", Py_T_INT, offsetof(UOpOptimizerObject, traces_executed), 0,
     "Total number of traces executed since inception"},
    {"instrs_executed", Py_T_INT, offsetof(UOpOptimizerObject, instrs_executed), 0,
     "Total number of instructions executed since inception"},
    {NULL}
};

static PyTypeObject UOpOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "UOp Optimizer",
    .tp_basicsize = sizeof(UOpOptimizerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_members = uop_members,
};

PyObject *
PyUnstable_Optimizer_NewUOpOptimizer(void)
{
    if (PyType_Ready(&UOpOptimizer_Type) < 0) {
        return NULL;
    }
    UOpOptimizerObject *opt = (UOpOptimizerObject *)_PyObject_New(&UOpOptimizer_Type);
    if (opt == NULL) {
        return NULL;
    }
    opt->base.optimize = uop_optimize;
    opt->base.resume_threshold = UINT16_MAX;
    opt->base.backedge_threshold = 0;
    opt->traces_executed = 0;
    opt->instrs_executed = 0;
    return (PyObject *)opt;
}
