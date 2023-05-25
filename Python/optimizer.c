
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
    uintptr_t old_size = 0;
    if (old != NULL) {
        for (uintptr_t i = 0; i < old->size; i++) {
            if (old->executors[i] == NULL) {
                return i;
            }
        }
        if (old->size >= 256) {
            PyErr_Format(PyExc_SystemError,
                        "too many executors for code object");
            return -1;
        }
        old_size = old->size;
    }
    uintptr_t new_size = old_size ? old_size * 2 : 4;
    _PyExecutorArray *new = PyMem_Malloc(offsetof(_PyExecutorArray, executors) +
        new_size * sizeof(_PyExecutorObject *));
    if (new == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    new->size = new_size;
    for (uintptr_t i = 0; i < old_size; i++) {
        new->executors[i] = old->executors[i];
    }
    if (old) {
        PyMem_Free(old);
    }
    return old_size;
}

int
PyUnstable_Insert_Executor(PyCodeObject *code, int offset, _PyExecutorObject *executor)
{
    if (executor->vm_data.opcode) {
        PyErr_Format(PyExc_SystemError,
                        "Executor is already attached to code");
        return -1;
    }
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
        executor->vm_data.opcode = original.op.code;
        executor->vm_data.oparg = original.op.arg;
        code->co_executors->executors[index] = executor;
        assert(index < 256);
        _PyCode_CODE(code)[offset].op.code = ENTER_EXECUTOR;
        _PyCode_CODE(code)[offset].op.arg = index;
    }
    return 0;
}

static _PyInterpreterFrame *
noop_optimize(
    _PyOptimizerObject* self,
    _PyInterpreterFrame *frame,
    _Py_CODEUNIT *instr,
    PyObject **stack_pointer)
{
    frame->prev_instr = instr - 1;
    _PyFrame_SetStackPointer(frame, stack_pointer);
    return frame;
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
    .optimize = noop_optimize,
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
PyUnstable_SetOptimizer(_PyOptimizerObject* optimizer)
{
    /* Ignore costs for now */
    PyInterpreterState *interp = PyInterpreterState_Get();
    Py_CLEAR(interp->optimizer);
    if (optimizer == NULL) {
        interp->optimizer = &_PyOptimizer_Default;
        return;
    }
    Py_INCREF(optimizer);
    interp->optimizer = optimizer;
    interp->optimizer_backedge_threshold = optimizer->backedge_threshold;
    interp->optimizer_resume_threshold = optimizer->resume_threshold;
}
