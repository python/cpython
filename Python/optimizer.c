
#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_opcode.h"
#include "pycore_pystate.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

static int32_t
get_next_free_in_executor_array(PyCodeObject *code)
{
    _PyExecutorArray *old = code->co_executors;
    int size = 0;
    int capacity = 0;
    if (old != NULL) {
        size = old->size;
        capacity = old->capacity;
        if (capacity >= 256) {
            PyErr_Format(PyExc_SystemError,
                        "too many executors for code object");
            return -1;
        }
    }
    for (uintptr_t i = 0; i < size; i++) {
        if (old->executors[i] == NULL) {
            return i;
        }
    }
    if (size >= capacity) {
        int new_capacity = capacity ? capacity * 2 : 4;
        _PyExecutorArray *new = PyMem_Malloc(offsetof(_PyExecutorArray, executors) +
            new_capacity * sizeof(_PyExecutorObject *));
        if (new == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        code->co_executors = new;
        new->capacity = new_capacity;
        for (uintptr_t i = 0; i < size; i++) {
            new->executors[i] = old->executors[i];
        }
        if (old != NULL) {
            PyMem_Free(old);
        }
    }
    assert(size < code->co_executors->capacity);
    code->co_executors->size = size + 1;
    return size;
}

static int
insert_executor(PyCodeObject *code, int offset, _PyExecutorObject *executor)
{
    _Py_CODEUNIT original = _PyCode_CODE(code)[offset];
    if (original.op.code == ENTER_EXECUTOR) {
        _PyExecutorObject *old = code->co_executors->executors[original.op.arg];
        executor->vm_data.opcode = old->vm_data.opcode;
        executor->vm_data.oparg = old->vm_data.oparg;
        old->vm_data.opcode = 0;
        code->co_executors->executors[original.op.arg] = executor;
        Py_DECREF(old);
    }
    else {
        int32_t index = get_next_free_in_executor_array(code);
        if (index < 0) {
            return -1;
        }
        Py_INCREF(executor);
        executor->vm_data.opcode = original.op.code;
        executor->vm_data.oparg = original.op.arg;
        code->co_executors->executors[index] = executor;
        assert(index < 256);
        _PyCode_CODE(code)[offset].op.code = ENTER_EXECUTOR;
        _PyCode_CODE(code)[offset].op.arg = index;
    }
    return 0;
}

int PyUnstable_Replace_Executor(PyCodeObject *code, int offset, _PyExecutorObject *new)
{

    _Py_CODEUNIT original = _PyCode_CODE(code)[offset];
    if (original.op.code != ENTER_EXECUTOR) {
        PyErr_Format(PyExc_ValueError, "No executor to replace");
        return -1;
    }
    return insert_executor(code, offset, new);
}

static _PyExecutorObject *
error_optimize(
    _PyOptimizerObject* self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr)
{
    PyErr_Format(PyExc_SystemError, "Should never call error_optimize");
    return NULL;
}

static PyTypeObject DefaultOptimizer_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "No-op optimizer",
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

PyObject*
PyUnstable_GetOptimizer(_PyOptimizerObject* optimizer)
{
    /* Ignore costs for now */
    PyInterpreterState *interp = PyInterpreterState_Get();
    if (interp->optimizer == &_PyOptimizer_Default) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(interp->optimizer);
}

void
PyUnstable_SetOptimizer(_PyOptimizerObject *optimizer)
{
    /* Ignore costs for now */
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
    PyInterpreterState *interp = PyInterpreterState_Get();
    _PyOptimizerObject *opt = interp->optimizer;
    _PyExecutorObject *executor = opt->optimize(opt, frame->f_code, dest);
    if (executor == NULL) {
        return NULL;
    }
    if (insert_executor(frame->f_code, src - _PyCode_CODE(frame->f_code), executor)) {
        return NULL;
    }
    return executor->execute(executor, frame, stack_pointer);
}


/** Test support **/


typedef struct _PyCounterOptimizerObject {
    _PyOptimizerObject base;
    int64_t count;
} _PyCounterOptimizerObject;

typedef struct _PyCounterExecutorObject {
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
    .tp_name = "Test executor",
    .tp_basicsize = sizeof(_PyCounterExecutorObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .tp_dealloc = (destructor)counter_dealloc,
};

static _PyInterpreterFrame *counter_execute(_PyExecutorObject *self, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    ((_PyCounterExecutorObject *)self)->optimizer->count++;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    frame->prev_instr = ((_PyCounterExecutorObject *)self)->next_instr - 1;
    Py_DECREF(self);
    return frame;
}

static _PyExecutorObject *
counter_optimize(
    _PyOptimizerObject* self,
    PyCodeObject *code,
    _Py_CODEUNIT *instr)
{
    _PyCounterExecutorObject *executor = (_PyCounterExecutorObject *)_PyObject_New(&CounterExecutor_Type);
    if (executor == NULL) {
        return NULL;
    }
    executor->executor.execute = counter_execute;
    Py_INCREF(self);
    executor->optimizer = (_PyCounterOptimizerObject *)self;
    executor->next_instr = instr;
    return (_PyExecutorObject *)executor;
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

PyObject *PyUnstable_Optimizer_NewCounter(void)
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

