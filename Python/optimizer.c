#include "Python.h"

#ifdef _Py_TIER2

#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_backoff.h"
#include "pycore_bitutils.h"        // _Py_popcount32()
#include "pycore_ceval.h"       // _Py_set_eval_breaker_bit
#include "pycore_code.h"            // _Py_GetBaseCodeUnit
#include "pycore_function.h"        // _PyFunction_LookupByVersion()
#include "pycore_interpframe.h"
#include "pycore_object.h"          // _PyObject_GC_UNTRACK()
#include "pycore_opcode_metadata.h" // _PyOpcode_OpName[]
#include "pycore_opcode_utils.h"  // MAX_REAL_OPCODE
#include "pycore_optimizer.h"     // _Py_uop_analyze_and_optimize()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_tuple.h"         // _PyTuple_FromArraySteal
#include "pycore_unicodeobject.h" // _PyUnicode_FromASCII
#include "pycore_uop_ids.h"
#include "pycore_jit.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define NEED_OPCODE_METADATA
#include "pycore_uop_metadata.h" // Uop tables
#undef NEED_OPCODE_METADATA

#define MAX_EXECUTORS_SIZE 256

// Trace too short, no progress:
// _START_EXECUTOR
// _MAKE_WARM
// _CHECK_VALIDITY
// _SET_IP
// is 4-5 instructions.
#define CODE_SIZE_NO_PROGRESS 5
// We start with _START_EXECUTOR, _MAKE_WARM
#define CODE_SIZE_EMPTY 2

#define _PyExecutorObject_CAST(op)  ((_PyExecutorObject *)(op))

static bool
has_space_for_executor(PyCodeObject *code, _Py_CODEUNIT *instr)
{
    if (code == (PyCodeObject *)&_Py_InitCleanup) {
        return false;
    }
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
        _Py_ExecutorDetach(code->co_executors->executors[index]);
    }
    else {
        assert(code->co_executors->size == index);
        assert(code->co_executors->capacity > index);
        code->co_executors->size++;
    }
    executor->vm_data.opcode = instr->op.code;
    executor->vm_data.oparg = instr->op.arg;
    executor->vm_data.code = code;
    executor->vm_data.index = (int)(instr - _PyCode_CODE(code));
    code->co_executors->executors[index] = executor;
    assert(index < MAX_EXECUTORS_SIZE);
    instr->op.code = ENTER_EXECUTOR;
    instr->op.arg = index;
}

static _PyExecutorObject *
make_executor_from_uops(_PyUOpInstruction *buffer, int length, const _PyBloomFilter *dependencies, int chain_depth);

static int
uop_optimize(_PyInterpreterFrame *frame, PyThreadState *tstate,
             _PyExecutorObject **exec_ptr,
             bool progress_needed);

/* Returns 1 if optimized, 0 if not optimized, and -1 for an error.
 * If optimized, *executor_ptr contains a new reference to the executor
 */
// gh-137573: inlining this function causes stack overflows
Py_NO_INLINE int
_PyOptimizer_Optimize(
    _PyInterpreterFrame *frame, PyThreadState *tstate)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    int chain_depth = _tstate->jit_tracer_state.initial_state.chain_depth;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!interp->jit) {
        // gh-140936: It is possible that interp->jit will become false during
        // interpreter finalization. However, the specialized JUMP_BACKWARD_JIT
        // instruction may still be present. In this case, we should
        // return immediately without optimization.
        return 0;
    }
    assert(!interp->compiling);
    assert(_tstate->jit_tracer_state.initial_state.stack_depth >= 0);
#ifndef Py_GIL_DISABLED
    assert(_tstate->jit_tracer_state.initial_state.func != NULL);
    interp->compiling = true;
    // The first executor in a chain and the MAX_CHAIN_DEPTH'th executor *must*
    // make progress in order to avoid infinite loops or excessively-long
    // side-exit chains. We can only insert the executor into the bytecode if
    // this is true, since a deopt won't infinitely re-enter the executor:
    chain_depth %= MAX_CHAIN_DEPTH;
    bool progress_needed = chain_depth == 0;
    PyCodeObject *code = (PyCodeObject *)_tstate->jit_tracer_state.initial_state.code;
    _Py_CODEUNIT *start = _tstate->jit_tracer_state.initial_state.start_instr;
    if (progress_needed && !has_space_for_executor(code, start)) {
        interp->compiling = false;
        return 0;
    }
    // One of our dependencies while tracing was invalidated. Not worth compiling.
    if (!_tstate->jit_tracer_state.prev_state.dependencies_still_valid) {
        interp->compiling = false;
        return 0;
    }
    _PyExecutorObject *executor;
    int err = uop_optimize(frame, tstate, &executor, progress_needed);
    if (err <= 0) {
        interp->compiling = false;
        return err;
    }
    assert(executor != NULL);
    if (progress_needed) {
        int index = get_index_for_executor(code, start);
        if (index < 0) {
            /* Out of memory. Don't raise and assume that the
             * error will show up elsewhere.
             *
             * If an optimizer has already produced an executor,
             * it might get confused by the executor disappearing,
             * but there is not much we can do about that here. */
            Py_DECREF(executor);
            interp->compiling = false;
            return 0;
        }
        insert_executor(code, start, index, executor);
    }
    else {
        executor->vm_data.code = NULL;
    }
    _PyExitData *exit = _tstate->jit_tracer_state.initial_state.exit;
    if (exit != NULL) {
        exit->executor = executor;
    }
    executor->vm_data.chain_depth = chain_depth;
    assert(executor->vm_data.valid);
    interp->compiling = false;
    return 1;
#else
    return 0;
#endif
}

static _PyExecutorObject *
get_executor_lock_held(PyCodeObject *code, int offset)
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

_PyExecutorObject *
_Py_GetExecutor(PyCodeObject *code, int offset)
{
    _PyExecutorObject *executor;
    Py_BEGIN_CRITICAL_SECTION(code);
    executor = get_executor_lock_held(code, offset);
    Py_END_CRITICAL_SECTION();
    return executor;
}

static PyObject *
is_valid(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyBool_FromLong(((_PyExecutorObject *)self)->vm_data.valid);
}

static PyObject *
get_opcode(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromUnsignedLong(((_PyExecutorObject *)self)->vm_data.opcode);
}

static PyObject *
get_oparg(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromUnsignedLong(((_PyExecutorObject *)self)->vm_data.oparg);
}

///////////////////// Experimental UOp Optimizer /////////////////////

static int executor_clear(PyObject *executor);
static void unlink_executor(_PyExecutorObject *executor);


void
_PyExecutor_Free(_PyExecutorObject *self)
{
#ifdef _Py_JIT
    _PyJIT_Free(self);
#endif
    PyObject_GC_Del(self);
}

void
_Py_ClearExecutorDeletionList(PyInterpreterState *interp)
{
    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    PyThreadState* ts = PyInterpreterState_ThreadHead(interp);
    HEAD_UNLOCK(runtime);
    while (ts) {
        _PyExecutorObject *current = (_PyExecutorObject *)ts->current_executor;
        if (current != NULL) {
            /* Anything in this list will be unlinked, so we can reuse the
             * linked field as a reachability marker. */
            current->vm_data.linked = 1;
        }
        HEAD_LOCK(runtime);
        ts = PyThreadState_Next(ts);
        HEAD_UNLOCK(runtime);
    }
    _PyExecutorObject **prev_to_next_ptr = &interp->executor_deletion_list_head;
    _PyExecutorObject *exec = *prev_to_next_ptr;
    while (exec != NULL) {
        if (exec->vm_data.linked) {
            // This executor is currently executing
            exec->vm_data.linked = 0;
            prev_to_next_ptr = &exec->vm_data.links.next;
        }
        else {
            *prev_to_next_ptr = exec->vm_data.links.next;
            _PyExecutor_Free(exec);
        }
        exec = *prev_to_next_ptr;
    }
    interp->executor_deletion_list_remaining_capacity = EXECUTOR_DELETE_LIST_MAX;
}

static void
add_to_pending_deletion_list(_PyExecutorObject *self)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    self->vm_data.links.next = interp->executor_deletion_list_head;
    interp->executor_deletion_list_head = self;
    if (interp->executor_deletion_list_remaining_capacity > 0) {
        interp->executor_deletion_list_remaining_capacity--;
    }
    else {
        _Py_ClearExecutorDeletionList(interp);
    }
}

static void
uop_dealloc(PyObject *op) {
    _PyExecutorObject *self = _PyExecutorObject_CAST(op);
    _PyObject_GC_UNTRACK(self);
    assert(self->vm_data.code == NULL);
    unlink_executor(self);
    // Once unlinked it becomes impossible to invalidate an executor, so do it here.
    self->vm_data.valid = 0;
    add_to_pending_deletion_list(self);
}

const char *
_PyUOpName(int index)
{
    if (index < 0 || index > MAX_UOP_ID) {
        return NULL;
    }
    return _PyOpcode_uop_name[index];
}

#ifdef Py_DEBUG
void
_PyUOpPrint(const _PyUOpInstruction *uop)
{
    const char *name = _PyUOpName(uop->opcode);
    if (name == NULL) {
        printf("<uop %d>", uop->opcode);
    }
    else {
        printf("%s", name);
    }
    switch(uop->format) {
        case UOP_FORMAT_TARGET:
            printf(" (%d, target=%d, operand0=%#" PRIx64 ", operand1=%#" PRIx64,
                uop->oparg,
                uop->target,
                (uint64_t)uop->operand0,
                (uint64_t)uop->operand1);
            break;
        case UOP_FORMAT_JUMP:
            printf(" (%d, jump_target=%d, operand0=%#" PRIx64 ", operand1=%#" PRIx64,
                uop->oparg,
                uop->jump_target,
                (uint64_t)uop->operand0,
                (uint64_t)uop->operand1);
            break;
        default:
            printf(" (%d, Unknown format)", uop->oparg);
    }
    if (_PyUop_Flags[uop->opcode] & HAS_ERROR_FLAG) {
        printf(", error_target=%d", uop->error_target);
    }

    printf(")");
}
#endif

static Py_ssize_t
uop_len(PyObject *op)
{
    _PyExecutorObject *self = _PyExecutorObject_CAST(op);
    return self->code_size;
}

static PyObject *
uop_item(PyObject *op, Py_ssize_t index)
{
    _PyExecutorObject *self = _PyExecutorObject_CAST(op);
    Py_ssize_t len = uop_len(op);
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
    PyObject *target = PyLong_FromUnsignedLong(self->trace[index].target);
    if (target == NULL) {
        Py_DECREF(oparg);
        Py_DECREF(oname);
        return NULL;
    }
    PyObject *operand = PyLong_FromUnsignedLongLong(self->trace[index].operand0);
    if (operand == NULL) {
        Py_DECREF(target);
        Py_DECREF(oparg);
        Py_DECREF(oname);
        return NULL;
    }
    PyObject *args[4] = { oname, oparg, target, operand };
    return _PyTuple_FromArraySteal(args, 4);
}

PySequenceMethods uop_as_sequence = {
    .sq_length = uop_len,
    .sq_item = uop_item,
};

static int
executor_traverse(PyObject *o, visitproc visit, void *arg)
{
    _PyExecutorObject *executor = _PyExecutorObject_CAST(o);
    for (uint32_t i = 0; i < executor->exit_count; i++) {
        Py_VISIT(executor->exits[i].executor);
    }
    return 0;
}

static PyObject *
get_jit_code(PyObject *self, PyObject *Py_UNUSED(ignored))
{
#ifndef _Py_JIT
    PyErr_SetString(PyExc_RuntimeError, "JIT support not enabled.");
    return NULL;
#else
    _PyExecutorObject *executor = _PyExecutorObject_CAST(self);
    if (executor->jit_code == NULL || executor->jit_size == 0) {
        Py_RETURN_NONE;
    }
    return PyBytes_FromStringAndSize(executor->jit_code, executor->jit_size);
#endif
}

static PyMethodDef uop_executor_methods[] = {
    { "is_valid", is_valid, METH_NOARGS, NULL },
    { "get_jit_code", get_jit_code, METH_NOARGS, NULL},
    { "get_opcode", get_opcode, METH_NOARGS, NULL },
    { "get_oparg", get_oparg, METH_NOARGS, NULL },
    { NULL, NULL },
};

static int
executor_is_gc(PyObject *o)
{
    return !_Py_IsImmortal(o);
}

PyTypeObject _PyUOpExecutor_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "uop_executor",
    .tp_basicsize = offsetof(_PyExecutorObject, exits),
    .tp_itemsize = 1,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    .tp_dealloc = uop_dealloc,
    .tp_as_sequence = &uop_as_sequence,
    .tp_methods = uop_executor_methods,
    .tp_traverse = executor_traverse,
    .tp_clear = executor_clear,
    .tp_is_gc = executor_is_gc,
};

/* TO DO -- Generate these tables */
static const uint16_t
_PyUOp_Replacements[MAX_UOP_ID + 1] = {
    [_ITER_JUMP_RANGE] = _GUARD_NOT_EXHAUSTED_RANGE,
    [_ITER_JUMP_LIST] = _GUARD_NOT_EXHAUSTED_LIST,
    [_ITER_JUMP_TUPLE] = _GUARD_NOT_EXHAUSTED_TUPLE,
    [_FOR_ITER] = _FOR_ITER_TIER_TWO,
    [_ITER_NEXT_LIST] = _ITER_NEXT_LIST_TIER_TWO,
    [_CHECK_PERIODIC_AT_END] = _TIER2_RESUME_CHECK,
};

static const uint8_t
is_for_iter_test[MAX_UOP_ID + 1] = {
    [_GUARD_NOT_EXHAUSTED_RANGE] = 1,
    [_GUARD_NOT_EXHAUSTED_LIST] = 1,
    [_GUARD_NOT_EXHAUSTED_TUPLE] = 1,
    [_FOR_ITER_TIER_TWO] = 1,
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

static const uint16_t
guard_ip_uop[MAX_UOP_ID + 1] = {
    [_PUSH_FRAME] = _GUARD_IP__PUSH_FRAME,
    [_RETURN_GENERATOR] = _GUARD_IP_RETURN_GENERATOR,
    [_RETURN_VALUE] = _GUARD_IP_RETURN_VALUE,
    [_YIELD_VALUE] = _GUARD_IP_YIELD_VALUE,
};


#define CONFIDENCE_RANGE 1000
#define CONFIDENCE_CUTOFF 333

#ifdef Py_DEBUG
#define DPRINTF(level, ...) \
    if (lltrace >= (level)) { printf(__VA_ARGS__); }
#else
#define DPRINTF(level, ...)
#endif


static inline int
add_to_trace(
    _PyUOpInstruction *trace,
    int trace_length,
    uint16_t opcode,
    uint16_t oparg,
    uint64_t operand,
    uint32_t target)
{
    trace[trace_length].opcode = opcode;
    trace[trace_length].format = UOP_FORMAT_TARGET;
    trace[trace_length].target = target;
    trace[trace_length].oparg = oparg;
    trace[trace_length].operand0 = operand;
#ifdef Py_STATS
    trace[trace_length].execution_count = 0;
#endif
    return trace_length + 1;
}

#ifdef Py_DEBUG
#define ADD_TO_TRACE(OPCODE, OPARG, OPERAND, TARGET) \
    assert(trace_length < max_length); \
    trace_length = add_to_trace(trace, trace_length, (OPCODE), (OPARG), (OPERAND), (TARGET)); \
    if (lltrace >= 2) { \
        printf("%4d ADD_TO_TRACE: ", trace_length); \
        _PyUOpPrint(&trace[trace_length-1]); \
        printf("\n"); \
    }
#else
#define ADD_TO_TRACE(OPCODE, OPARG, OPERAND, TARGET) \
    assert(trace_length < max_length); \
    trace_length = add_to_trace(trace, trace_length, (OPCODE), (OPARG), (OPERAND), (TARGET));
#endif

#define INSTR_IP(INSTR, CODE) \
    ((uint32_t)((INSTR) - ((_Py_CODEUNIT *)(CODE)->co_code_adaptive)))

// Reserve space for n uops
#define RESERVE_RAW(n, opname) \
    if (trace_length + (n) > max_length) { \
        DPRINTF(2, "No room for %s (need %d, got %d)\n", \
                (opname), (n), max_length - trace_length); \
        OPT_STAT_INC(trace_too_long); \
        goto full; \
    }


/* Returns 1 on success (added to trace), 0 on trace end.
 */
int
_PyJit_translate_single_bytecode_to_trace(
    PyThreadState *tstate,
    _PyInterpreterFrame *frame,
    _Py_CODEUNIT *next_instr,
    int stop_tracing_opcode)
{

#ifdef Py_DEBUG
    char *python_lltrace = Py_GETENV("PYTHON_LLTRACE");
    int lltrace = 0;
    if (python_lltrace != NULL && *python_lltrace >= '0') {
        lltrace = *python_lltrace - '0';  // TODO: Parse an int and all that
    }
#endif
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    PyCodeObject *old_code = _tstate->jit_tracer_state.prev_state.instr_code;
    bool progress_needed = (_tstate->jit_tracer_state.initial_state.chain_depth % MAX_CHAIN_DEPTH) == 0;
    _PyBloomFilter *dependencies = &_tstate->jit_tracer_state.prev_state.dependencies;
    int trace_length = _tstate->jit_tracer_state.prev_state.code_curr_size;
    _PyUOpInstruction *trace = _tstate->jit_tracer_state.code_buffer;
    int max_length = _tstate->jit_tracer_state.prev_state.code_max_size;

    _Py_CODEUNIT *this_instr =  _tstate->jit_tracer_state.prev_state.instr;
    _Py_CODEUNIT *target_instr = this_instr;
    uint32_t target = 0;

    target = Py_IsNone((PyObject *)old_code)
        ? (int)(target_instr - _Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS_PTR)
        : INSTR_IP(target_instr, old_code);

    // Rewind EXTENDED_ARG so that we see the whole thing.
    // We must point to the first EXTENDED_ARG when deopting.
    int oparg = _tstate->jit_tracer_state.prev_state.instr_oparg;
    int opcode = this_instr->op.code;
    int rewind_oparg = oparg;
    while (rewind_oparg > 255) {
        rewind_oparg >>= 8;
        target--;
    }

    int old_stack_level = _tstate->jit_tracer_state.prev_state.instr_stacklevel;

    // Strange control-flow
    bool has_dynamic_jump_taken = OPCODE_HAS_UNPREDICTABLE_JUMP(opcode) &&
        (next_instr != this_instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]]);

    /* Special case the first instruction,
    * so that we can guarantee forward progress */
    if (progress_needed && _tstate->jit_tracer_state.prev_state.code_curr_size < CODE_SIZE_NO_PROGRESS) {
        if (OPCODE_HAS_EXIT(opcode) || OPCODE_HAS_DEOPT(opcode)) {
            opcode = _PyOpcode_Deopt[opcode];
        }
        assert(!OPCODE_HAS_EXIT(opcode));
        assert(!OPCODE_HAS_DEOPT(opcode));
    }

    bool needs_guard_ip = OPCODE_HAS_NEEDS_GUARD_IP(opcode);
    if (has_dynamic_jump_taken && !needs_guard_ip) {
        DPRINTF(2, "Unsupported: dynamic jump taken %s\n", _PyOpcode_OpName[opcode]);
        goto unsupported;
    }

    int is_sys_tracing = (tstate->c_tracefunc != NULL) || (tstate->c_profilefunc != NULL);
    if (is_sys_tracing) {
        goto full;
    }

    if (stop_tracing_opcode != 0) {
        ADD_TO_TRACE(stop_tracing_opcode, 0, 0, target);
        goto done;
    }

    DPRINTF(2, "%p %d: %s(%d) %d %d\n", old_code, target, _PyOpcode_OpName[opcode], oparg, needs_guard_ip, old_stack_level);

#ifdef Py_DEBUG
    if (oparg > 255) {
        assert(_Py_GetBaseCodeUnit(old_code, target).op.code == EXTENDED_ARG);
    }
#endif

    // Skip over super instructions.
    if (_tstate->jit_tracer_state.prev_state.instr_is_super) {
        _tstate->jit_tracer_state.prev_state.instr_is_super = false;
        return 1;
    }

    if (opcode == ENTER_EXECUTOR) {
        goto full;
    }

    if (!_tstate->jit_tracer_state.prev_state.dependencies_still_valid) {
        goto done;
    }

    // This happens when a recursive call happens that we can't trace. Such as Python -> C -> Python calls
    // If we haven't guarded the IP, then it's untraceable.
    if (frame != _tstate->jit_tracer_state.prev_state.instr_frame && !needs_guard_ip) {
        DPRINTF(2, "Unsupported: unguardable jump taken\n");
        goto unsupported;
    }

    if (oparg > 0xFFFF) {
        DPRINTF(2, "Unsupported: oparg too large\n");
        goto unsupported;
    }

    // TODO (gh-140277): The constituent use one extra stack slot. So we need to check for headroom.
    if (opcode == BINARY_OP_SUBSCR_GETITEM && old_stack_level + 1 > old_code->co_stacksize) {
        unsupported:
        {
            // Rewind to previous instruction and replace with _EXIT_TRACE.
            _PyUOpInstruction *curr = &trace[trace_length-1];
            while (curr->opcode != _SET_IP && trace_length > 2) {
                trace_length--;
                curr = &trace[trace_length-1];
            }
            assert(curr->opcode == _SET_IP || trace_length == 2);
            if (curr->opcode == _SET_IP) {
                int32_t old_target = (int32_t)uop_get_target(curr);
                curr++;
                trace_length++;
                curr->opcode = _EXIT_TRACE;
                curr->format = UOP_FORMAT_TARGET;
                curr->target = old_target;
            }
            goto done;
        }
    }

    if (opcode == NOP) {
        return 1;
    }

    if (opcode == JUMP_FORWARD) {
        return 1;
    }

    if (opcode == EXTENDED_ARG) {
        return 1;
    }

    // One for possible _DEOPT, one because _CHECK_VALIDITY itself might _DEOPT
    max_length -= 2;

    const struct opcode_macro_expansion *expansion = &_PyOpcode_macro_expansion[opcode];

    assert(opcode != ENTER_EXECUTOR && opcode != EXTENDED_ARG);
    assert(!_PyErr_Occurred(tstate));


    if (OPCODE_HAS_EXIT(opcode)) {
        // Make space for side exit and final _EXIT_TRACE:
        max_length--;
    }
    if (OPCODE_HAS_ERROR(opcode)) {
        // Make space for error stub and final _EXIT_TRACE:
        max_length--;
    }

    // _GUARD_IP leads to an exit.
    max_length -= needs_guard_ip;

    RESERVE_RAW(expansion->nuops + needs_guard_ip + 2 + (!OPCODE_HAS_NO_SAVE_IP(opcode)), "uop and various checks");

    ADD_TO_TRACE(_CHECK_VALIDITY, 0, 0, target);

    if (!OPCODE_HAS_NO_SAVE_IP(opcode)) {
        ADD_TO_TRACE(_SET_IP, 0, (uintptr_t)target_instr, target);
    }

    // Can be NULL for the entry frame.
    if (old_code != NULL) {
        _Py_BloomFilter_Add(dependencies, old_code);
    }

    switch (opcode) {
        case POP_JUMP_IF_NONE:
        case POP_JUMP_IF_NOT_NONE:
        case POP_JUMP_IF_FALSE:
        case POP_JUMP_IF_TRUE:
        {
            _Py_CODEUNIT *computed_next_instr_without_modifiers = target_instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
            _Py_CODEUNIT *computed_next_instr = computed_next_instr_without_modifiers + (computed_next_instr_without_modifiers->op.code == NOT_TAKEN);
            _Py_CODEUNIT *computed_jump_instr = computed_next_instr_without_modifiers + oparg;
            assert(next_instr == computed_next_instr || next_instr == computed_jump_instr);
            int jump_happened = computed_jump_instr == next_instr;
            assert(jump_happened == (target_instr[1].cache & 1));
            uint32_t uopcode = BRANCH_TO_GUARD[opcode - POP_JUMP_IF_FALSE][jump_happened];
            ADD_TO_TRACE(uopcode, 0, 0, INSTR_IP(jump_happened ? computed_next_instr : computed_jump_instr, old_code));
            break;
        }
        case JUMP_BACKWARD_JIT:
            // This is possible as the JIT might have re-activated after it was disabled
        case JUMP_BACKWARD_NO_JIT:
        case JUMP_BACKWARD:
            ADD_TO_TRACE(_CHECK_PERIODIC, 0, 0, target);
            _Py_FALLTHROUGH;
        case JUMP_BACKWARD_NO_INTERRUPT:
        {
            if ((next_instr != _tstate->jit_tracer_state.initial_state.close_loop_instr) &&
                (next_instr != _tstate->jit_tracer_state.initial_state.start_instr) &&
                _tstate->jit_tracer_state.prev_state.code_curr_size > CODE_SIZE_NO_PROGRESS &&
                // For side exits, we don't want to terminate them early.
                _tstate->jit_tracer_state.initial_state.exit == NULL &&
                // These are coroutines, and we want to unroll those usually.
                opcode != JUMP_BACKWARD_NO_INTERRUPT) {
                // We encountered a JUMP_BACKWARD but not to the top of our own loop.
                // We don't want to continue tracing as we might get stuck in the
                // inner loop. Instead, end the trace where the executor of the
                // inner loop might start and let the traces rejoin.
                OPT_STAT_INC(inner_loop);
                ADD_TO_TRACE(_EXIT_TRACE, 0, 0, target);
                trace[trace_length-1].operand1 = true; // is_control_flow
                DPRINTF(2, "JUMP_BACKWARD not to top ends trace %p %p %p\n", next_instr,
                    _tstate->jit_tracer_state.initial_state.close_loop_instr, _tstate->jit_tracer_state.initial_state.start_instr);
                goto done;
            }
            break;
        }

        case RESUME:
        case RESUME_CHECK:
            /* Use a special tier 2 version of RESUME_CHECK to allow traces to
             *  start with RESUME_CHECK */
            ADD_TO_TRACE(_TIER2_RESUME_CHECK, 0, 0, target);
            break;
        default:
        {
            const struct opcode_macro_expansion *expansion = &_PyOpcode_macro_expansion[opcode];
            // Reserve space for nuops (+ _SET_IP + _EXIT_TRACE)
            int nuops = expansion->nuops;
            if (nuops == 0) {
                DPRINTF(2, "Unsupported opcode %s\n", _PyOpcode_OpName[opcode]);
                goto unsupported;
            }
            assert(nuops > 0);
            uint32_t orig_oparg = oparg;  // For OPARG_TOP/BOTTOM
            uint32_t orig_target = target;
            for (int i = 0; i < nuops; i++) {
                oparg = orig_oparg;
                target = orig_target;
                uint32_t uop = expansion->uops[i].uop;
                uint64_t operand = 0;
                // Add one to account for the actual opcode/oparg pair:
                int offset = expansion->uops[i].offset + 1;
                switch (expansion->uops[i].size) {
                    case OPARG_SIMPLE:
                        assert(opcode != _JUMP_BACKWARD_NO_INTERRUPT && opcode != JUMP_BACKWARD);
                        break;
                    case OPARG_CACHE_1:
                        operand = read_u16(&this_instr[offset].cache);
                        break;
                    case OPARG_CACHE_2:
                        operand = read_u32(&this_instr[offset].cache);
                        break;
                    case OPARG_CACHE_4:
                        operand = read_u64(&this_instr[offset].cache);
                        break;
                    case OPARG_TOP:  // First half of super-instr
                        assert(orig_oparg <= 255);
                        oparg = orig_oparg >> 4;
                        break;
                    case OPARG_BOTTOM:  // Second half of super-instr
                        assert(orig_oparg <= 255);
                        oparg = orig_oparg & 0xF;
                        break;
                    case OPARG_SAVE_RETURN_OFFSET:  // op=_SAVE_RETURN_OFFSET; oparg=return_offset
                        oparg = offset;
                        assert(uop == _SAVE_RETURN_OFFSET);
                        break;
                    case OPARG_REPLACED:
                        uop = _PyUOp_Replacements[uop];
                        assert(uop != 0);

                        uint32_t next_inst = target + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
                        if (uop == _TIER2_RESUME_CHECK) {
                            target = next_inst;
                        }
                        else {
                            int extended_arg = orig_oparg > 255;
                            uint32_t jump_target = next_inst + orig_oparg + extended_arg;
                            assert(_Py_GetBaseCodeUnit(old_code, jump_target).op.code == END_FOR);
                            assert(_Py_GetBaseCodeUnit(old_code, jump_target+1).op.code == POP_ITER);
                            if (is_for_iter_test[uop]) {
                                target = jump_target + 1;
                            }
                        }
                        break;
                    case OPERAND1_1:
                        assert(trace[trace_length-1].opcode == uop);
                        operand = read_u16(&this_instr[offset].cache);
                        trace[trace_length-1].operand1 = operand;
                        continue;
                    case OPERAND1_2:
                        assert(trace[trace_length-1].opcode == uop);
                        operand = read_u32(&this_instr[offset].cache);
                        trace[trace_length-1].operand1 = operand;
                        continue;
                    case OPERAND1_4:
                        assert(trace[trace_length-1].opcode == uop);
                        operand = read_u64(&this_instr[offset].cache);
                        trace[trace_length-1].operand1 = operand;
                        continue;
                    default:
                        fprintf(stderr,
                                "opcode=%d, oparg=%d; nuops=%d, i=%d; size=%d, offset=%d\n",
                                opcode, oparg, nuops, i,
                                expansion->uops[i].size,
                                expansion->uops[i].offset);
                        Py_FatalError("garbled expansion");
                }
                if (uop == _PUSH_FRAME || uop == _RETURN_VALUE || uop == _RETURN_GENERATOR || uop == _YIELD_VALUE) {
                    PyCodeObject *new_code = (PyCodeObject *)PyStackRef_AsPyObjectBorrow(frame->f_executable);
                    PyFunctionObject *new_func = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(frame->f_funcobj);

                    operand = 0;
                    if (frame->owner < FRAME_OWNED_BY_INTERPRETER) {
                        // Don't add nested code objects to the dependency.
                        // It causes endless re-traces.
                        if (new_func != NULL && !Py_IsNone((PyObject*)new_func) && !(new_code->co_flags & CO_NESTED)) {
                            operand = (uintptr_t)new_func;
                            DPRINTF(2, "Adding %p func to op\n", (void *)operand);
                            _Py_BloomFilter_Add(dependencies, new_func);
                        }
                        else if (new_code != NULL && !Py_IsNone((PyObject*)new_code)) {
                            operand = (uintptr_t)new_code | 1;
                            DPRINTF(2, "Adding %p code to op\n", (void *)operand);
                            _Py_BloomFilter_Add(dependencies, new_code);
                        }
                    }
                    ADD_TO_TRACE(uop, oparg, operand, target);
                    trace[trace_length - 1].operand1 = PyStackRef_IsNone(frame->f_executable) ? 2 : ((int)(frame->stackpointer - _PyFrame_Stackbase(frame)));
                    break;
                }
                if (uop == _BINARY_OP_INPLACE_ADD_UNICODE) {
                    assert(i + 1 == nuops);
                    _Py_CODEUNIT *next = target_instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
                    assert(next->op.code == STORE_FAST);
                    operand = next->op.arg;
                }
                // All other instructions
                ADD_TO_TRACE(uop, oparg, operand, target);
            }
            break;
        }  // End default

    }  // End switch (opcode)

    if (needs_guard_ip) {
        uint16_t guard_ip = guard_ip_uop[trace[trace_length-1].opcode];
        if (guard_ip == 0) {
            DPRINTF(1, "Unknown uop needing guard ip %s\n", _PyOpcode_uop_name[trace[trace_length-1].opcode]);
            Py_UNREACHABLE();
        }
        ADD_TO_TRACE(guard_ip, 0, (uintptr_t)next_instr, 0);
    }
    // Loop back to the start
    int is_first_instr = _tstate->jit_tracer_state.initial_state.close_loop_instr == next_instr ||
        _tstate->jit_tracer_state.initial_state.start_instr == next_instr;
    if (is_first_instr && _tstate->jit_tracer_state.prev_state.code_curr_size > CODE_SIZE_NO_PROGRESS) {
        if (needs_guard_ip) {
            ADD_TO_TRACE(_SET_IP, 0, (uintptr_t)next_instr, 0);
        }
        ADD_TO_TRACE(_JUMP_TO_TOP, 0, 0, 0);
        goto done;
    }
    DPRINTF(2, "Trace continuing\n");
    _tstate->jit_tracer_state.prev_state.code_curr_size = trace_length;
    _tstate->jit_tracer_state.prev_state.code_max_size = max_length;
    return 1;
done:
    DPRINTF(2, "Trace done\n");
    _tstate->jit_tracer_state.prev_state.code_curr_size = trace_length;
    _tstate->jit_tracer_state.prev_state.code_max_size = max_length;
    return 0;
full:
    DPRINTF(2, "Trace full\n");
    if (!is_terminator(&_tstate->jit_tracer_state.code_buffer[trace_length-1])) {
        // Undo the last few instructions.
        trace_length = _tstate->jit_tracer_state.prev_state.code_curr_size;
        max_length = _tstate->jit_tracer_state.prev_state.code_max_size;
        // We previously reversed one.
        max_length += 1;
        ADD_TO_TRACE(_EXIT_TRACE, 0, 0, target);
        trace[trace_length-1].operand1 = true; // is_control_flow
    }
    _tstate->jit_tracer_state.prev_state.code_curr_size = trace_length;
    _tstate->jit_tracer_state.prev_state.code_max_size = max_length;
    return 0;
}

// Returns 0 for do not enter tracing, 1 on enter tracing.
int
_PyJit_TryInitializeTracing(
    PyThreadState *tstate, _PyInterpreterFrame *frame, _Py_CODEUNIT *curr_instr,
    _Py_CODEUNIT *start_instr, _Py_CODEUNIT *close_loop_instr, int curr_stackdepth, int chain_depth,
    _PyExitData *exit, int oparg)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    // A recursive trace.
    // Don't trace into the inner call because it will stomp on the previous trace, causing endless retraces.
    if (_tstate->jit_tracer_state.prev_state.code_curr_size > CODE_SIZE_EMPTY) {
        return 0;
    }
    if (oparg > 0xFFFF) {
        return 0;
    }
    if (_tstate->jit_tracer_state.code_buffer == NULL) {
        _tstate->jit_tracer_state.code_buffer = (_PyUOpInstruction *)_PyObject_VirtualAlloc(UOP_BUFFER_SIZE);
        if (_tstate->jit_tracer_state.code_buffer == NULL) {
            // Don't error, just go to next instruction.
            return 0;
        }
    }
    PyObject *func = PyStackRef_AsPyObjectBorrow(frame->f_funcobj);
    if (func == NULL) {
        return 0;
    }
    PyCodeObject *code = _PyFrame_GetCode(frame);
#ifdef Py_DEBUG
    char *python_lltrace = Py_GETENV("PYTHON_LLTRACE");
    int lltrace = 0;
    if (python_lltrace != NULL && *python_lltrace >= '0') {
        lltrace = *python_lltrace - '0';  // TODO: Parse an int and all that
    }
    DPRINTF(2,
        "Tracing %s (%s:%d) at byte offset %d at chain depth %d\n",
        PyUnicode_AsUTF8(code->co_qualname),
        PyUnicode_AsUTF8(code->co_filename),
        code->co_firstlineno,
        2 * INSTR_IP(close_loop_instr, code),
        chain_depth);
#endif

    add_to_trace(_tstate->jit_tracer_state.code_buffer, 0, _START_EXECUTOR, 0, (uintptr_t)start_instr, INSTR_IP(start_instr, code));
    add_to_trace(_tstate->jit_tracer_state.code_buffer, 1, _MAKE_WARM, 0, 0, 0);
    _tstate->jit_tracer_state.prev_state.code_curr_size = CODE_SIZE_EMPTY;

    _tstate->jit_tracer_state.prev_state.code_max_size = UOP_MAX_TRACE_LENGTH;
    _tstate->jit_tracer_state.initial_state.start_instr = start_instr;
    _tstate->jit_tracer_state.initial_state.close_loop_instr = close_loop_instr;
    _tstate->jit_tracer_state.initial_state.code = (PyCodeObject *)Py_NewRef(code);
    _tstate->jit_tracer_state.initial_state.func = (PyFunctionObject *)Py_NewRef(func);
    _tstate->jit_tracer_state.initial_state.exit = exit;
    _tstate->jit_tracer_state.initial_state.stack_depth = curr_stackdepth;
    _tstate->jit_tracer_state.initial_state.chain_depth = chain_depth;
    _tstate->jit_tracer_state.prev_state.instr_frame = frame;
    _tstate->jit_tracer_state.prev_state.dependencies_still_valid = true;
    _tstate->jit_tracer_state.prev_state.instr_code = (PyCodeObject *)Py_NewRef(_PyFrame_GetCode(frame));
    _tstate->jit_tracer_state.prev_state.instr = curr_instr;
    _tstate->jit_tracer_state.prev_state.instr_frame = frame;
    _tstate->jit_tracer_state.prev_state.instr_oparg = oparg;
    _tstate->jit_tracer_state.prev_state.instr_stacklevel = curr_stackdepth;
    _tstate->jit_tracer_state.prev_state.instr_is_super = false;
    assert(curr_instr->op.code == JUMP_BACKWARD_JIT || (exit != NULL));
    _tstate->jit_tracer_state.initial_state.jump_backward_instr = curr_instr;

    if (_PyOpcode_Caches[_PyOpcode_Deopt[close_loop_instr->op.code]]) {
        close_loop_instr[1].counter = trigger_backoff_counter();
    }
    _Py_BloomFilter_Init(&_tstate->jit_tracer_state.prev_state.dependencies);
    return 1;
}

void
_PyJit_FinalizeTracing(PyThreadState *tstate)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    Py_CLEAR(_tstate->jit_tracer_state.initial_state.code);
    Py_CLEAR(_tstate->jit_tracer_state.initial_state.func);
    Py_CLEAR(_tstate->jit_tracer_state.prev_state.instr_code);
    _tstate->jit_tracer_state.prev_state.code_curr_size = CODE_SIZE_EMPTY;
    _tstate->jit_tracer_state.prev_state.code_max_size = UOP_MAX_TRACE_LENGTH - 1;
}


#undef RESERVE
#undef RESERVE_RAW
#undef INSTR_IP
#undef ADD_TO_TRACE
#undef DPRINTF

#define UNSET_BIT(array, bit) (array[(bit)>>5] &= ~(1<<((bit)&31)))
#define SET_BIT(array, bit) (array[(bit)>>5] |= (1<<((bit)&31)))
#define BIT_IS_SET(array, bit) (array[(bit)>>5] & (1<<((bit)&31)))

/* Count the number of unused uops and exits
*/
static int
count_exits(_PyUOpInstruction *buffer, int length)
{
    int exit_count = 0;
    for (int i = 0; i < length; i++) {
        int opcode = buffer[i].opcode;
        if (opcode == _EXIT_TRACE || opcode == _DYNAMIC_EXIT) {
            exit_count++;
        }
    }
    return exit_count;
}

static void make_exit(_PyUOpInstruction *inst, int opcode, int target, bool is_control_flow)
{
    inst->opcode = opcode;
    inst->oparg = 0;
    inst->operand0 = 0;
    inst->format = UOP_FORMAT_TARGET;
    inst->target = target;
    inst->operand1 = is_control_flow;
#ifdef Py_STATS
    inst->execution_count = 0;
#endif
}

/* Convert implicit exits, errors and deopts
 * into explicit ones. */
static int
prepare_for_execution(_PyUOpInstruction *buffer, int length)
{
    int32_t current_jump = -1;
    int32_t current_jump_target = -1;
    int32_t current_error = -1;
    int32_t current_error_target = -1;
    int32_t current_popped = -1;
    int32_t current_exit_op = -1;
    /* Leaving in NOPs slows down the interpreter and messes up the stats */
    _PyUOpInstruction *copy_to = &buffer[0];
    for (int i = 0; i < length; i++) {
        _PyUOpInstruction *inst = &buffer[i];
        if (inst->opcode != _NOP) {
            if (copy_to != inst) {
                *copy_to = *inst;
            }
            copy_to++;
        }
    }
    length = (int)(copy_to - buffer);
    int next_spare = length;
    for (int i = 0; i < length; i++) {
        _PyUOpInstruction *inst = &buffer[i];
        int opcode = inst->opcode;
        int32_t target = (int32_t)uop_get_target(inst);
        uint16_t exit_flags = _PyUop_Flags[opcode] & (HAS_EXIT_FLAG | HAS_DEOPT_FLAG | HAS_PERIODIC_FLAG);
        if (exit_flags) {
            uint16_t exit_op = _EXIT_TRACE;
            if (exit_flags & HAS_DEOPT_FLAG) {
                exit_op = _DEOPT;
            }
            else if (exit_flags & HAS_PERIODIC_FLAG) {
                exit_op = _HANDLE_PENDING_AND_DEOPT;
            }
            int32_t jump_target = target;
            if (
                opcode == _GUARD_IP__PUSH_FRAME ||
                opcode == _GUARD_IP_RETURN_VALUE ||
                opcode == _GUARD_IP_YIELD_VALUE ||
                opcode == _GUARD_IP_RETURN_GENERATOR
            ) {
                exit_op = _DYNAMIC_EXIT;
            }
            bool is_control_flow = (opcode == _GUARD_IS_FALSE_POP || opcode == _GUARD_IS_TRUE_POP || is_for_iter_test[opcode]);
            if (jump_target != current_jump_target || current_exit_op != exit_op) {
                make_exit(&buffer[next_spare], exit_op, jump_target, is_control_flow);
                current_exit_op = exit_op;
                current_jump_target = jump_target;
                current_jump = next_spare;
                next_spare++;
            }
            buffer[i].jump_target = current_jump;
            buffer[i].format = UOP_FORMAT_JUMP;
        }
        if (_PyUop_Flags[opcode] & HAS_ERROR_FLAG) {
            int popped = (_PyUop_Flags[opcode] & HAS_ERROR_NO_POP_FLAG) ?
                0 : _PyUop_num_popped(opcode, inst->oparg);
            if (target != current_error_target || popped != current_popped) {
                current_popped = popped;
                current_error = next_spare;
                current_error_target = target;
                make_exit(&buffer[next_spare], _ERROR_POP_N, 0, false);
                buffer[next_spare].operand0 = target;
                next_spare++;
            }
            buffer[i].error_target = current_error;
            if (buffer[i].format == UOP_FORMAT_TARGET) {
                buffer[i].format = UOP_FORMAT_JUMP;
                buffer[i].jump_target = 0;
            }
        }
        if (opcode == _JUMP_TO_TOP) {
            assert(buffer[0].opcode == _START_EXECUTOR);
            buffer[i].format = UOP_FORMAT_JUMP;
            buffer[i].jump_target = 1;
        }
    }
    return next_spare;
}

/* Executor side exits */

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

#ifdef Py_DEBUG

#define CHECK(PRED) \
if (!(PRED)) { \
    printf(#PRED " at %d\n", i); \
    assert(0); \
}

static int
target_unused(int opcode)
{
    return (_PyUop_Flags[opcode] & (HAS_ERROR_FLAG | HAS_EXIT_FLAG | HAS_DEOPT_FLAG)) == 0;
}

static void
sanity_check(_PyExecutorObject *executor)
{
    for (uint32_t i = 0; i < executor->exit_count; i++) {
        _PyExitData *exit = &executor->exits[i];
        CHECK(exit->target < (1 << 25));
    }
    bool ended = false;
    uint32_t i = 0;
    CHECK(executor->trace[0].opcode == _START_EXECUTOR ||
        executor->trace[0].opcode == _COLD_EXIT ||
        executor->trace[0].opcode == _COLD_DYNAMIC_EXIT);
    for (; i < executor->code_size; i++) {
        const _PyUOpInstruction *inst = &executor->trace[i];
        uint16_t opcode = inst->opcode;
        CHECK(opcode <= MAX_UOP_ID);
        CHECK(_PyOpcode_uop_name[opcode] != NULL);
        switch(inst->format) {
            case UOP_FORMAT_TARGET:
                CHECK(target_unused(opcode));
                break;
            case UOP_FORMAT_JUMP:
                CHECK(inst->jump_target < executor->code_size);
                break;
        }
        if (_PyUop_Flags[opcode] & HAS_ERROR_FLAG) {
            CHECK(inst->format == UOP_FORMAT_JUMP);
            CHECK(inst->error_target < executor->code_size);
        }
        if (is_terminator(inst)) {
            ended = true;
            i++;
            break;
        }
    }
    CHECK(ended);
    for (; i < executor->code_size; i++) {
        const _PyUOpInstruction *inst = &executor->trace[i];
        uint16_t opcode = inst->opcode;
        CHECK(
            opcode == _DEOPT ||
            opcode == _HANDLE_PENDING_AND_DEOPT ||
            opcode == _EXIT_TRACE ||
            opcode == _ERROR_POP_N ||
            opcode == _DYNAMIC_EXIT);
    }
}

#undef CHECK
#endif

/* Makes an executor from a buffer of uops.
 * Account for the buffer having gaps and NOPs by computing a "used"
 * bit vector and only copying the used uops. Here "used" means reachable
 * and not a NOP.
 */
static _PyExecutorObject *
make_executor_from_uops(_PyUOpInstruction *buffer, int length, const _PyBloomFilter *dependencies, int chain_depth)
{
    int exit_count = count_exits(buffer, length);
    _PyExecutorObject *executor = allocate_executor(exit_count, length);
    if (executor == NULL) {
        return NULL;
    }

    /* Initialize exits */
    _PyExecutorObject *cold = _PyExecutor_GetColdExecutor();
    _PyExecutorObject *cold_dynamic = _PyExecutor_GetColdDynamicExecutor();
    cold->vm_data.chain_depth = chain_depth;
    for (int i = 0; i < exit_count; i++) {
        executor->exits[i].index = i;
        executor->exits[i].temperature = initial_temperature_backoff_counter();
    }
    int next_exit = exit_count-1;
    _PyUOpInstruction *dest = (_PyUOpInstruction *)&executor->trace[length];
    assert(buffer[0].opcode == _START_EXECUTOR);
    buffer[0].operand0 = (uint64_t)executor;
    for (int i = length-1; i >= 0; i--) {
        int opcode = buffer[i].opcode;
        dest--;
        *dest = buffer[i];
        if (opcode == _EXIT_TRACE || opcode == _DYNAMIC_EXIT) {
            _PyExitData *exit = &executor->exits[next_exit];
            exit->target = buffer[i].target;
            dest->operand0 = (uint64_t)exit;
            exit->executor = opcode == _EXIT_TRACE ? cold : cold_dynamic;
            exit->is_dynamic = (char)(opcode == _DYNAMIC_EXIT);
            exit->is_control_flow = (char)buffer[i].operand1;
            next_exit--;
        }
    }
    assert(next_exit == -1);
    assert(dest == executor->trace);
    assert(dest->opcode == _START_EXECUTOR);
    _Py_ExecutorInit(executor, dependencies);
#ifdef Py_DEBUG
    char *python_lltrace = Py_GETENV("PYTHON_LLTRACE");
    int lltrace = 0;
    if (python_lltrace != NULL && *python_lltrace >= '0') {
        lltrace = *python_lltrace - '0';  // TODO: Parse an int and all that
    }
    if (lltrace >= 2) {
        printf("Optimized trace (length %d):\n", length);
        for (int i = 0; i < length; i++) {
            printf("%4d OPTIMIZED: ", i);
            _PyUOpPrint(&executor->trace[i]);
            printf("\n");
        }
    }
    sanity_check(executor);
#endif
#ifdef _Py_JIT
    executor->jit_code = NULL;
    executor->jit_size = 0;
    // This is initialized to true so we can prevent the executor
    // from being immediately detected as cold and invalidated.
    executor->vm_data.warm = true;
    if (_PyJIT_Compile(executor, executor->trace, length)) {
        Py_DECREF(executor);
        return NULL;
    }
#endif
    _PyObject_GC_TRACK(executor);
    return executor;
}

#ifdef Py_STATS
/* Returns the effective trace length.
 * Ignores NOPs and trailing exit and error handling.*/
int effective_trace_length(_PyUOpInstruction *buffer, int length)
{
    int nop_count = 0;
    for (int i = 0; i < length; i++) {
        int opcode = buffer[i].opcode;
        if (opcode == _NOP) {
            nop_count++;
        }
        if (is_terminator(&buffer[i])) {
            return i+1-nop_count;
        }
    }
    Py_FatalError("No terminating instruction");
    Py_UNREACHABLE();
}
#endif

static int
uop_optimize(
    _PyInterpreterFrame *frame,
    PyThreadState *tstate,
    _PyExecutorObject **exec_ptr,
    bool progress_needed)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    _PyBloomFilter *dependencies = &_tstate->jit_tracer_state.prev_state.dependencies;
    _PyUOpInstruction *buffer = _tstate->jit_tracer_state.code_buffer;
    OPT_STAT_INC(attempts);
    char *env_var = Py_GETENV("PYTHON_UOPS_OPTIMIZE");
    bool is_noopt = true;
    if (env_var == NULL || *env_var == '\0' || *env_var > '0') {
        is_noopt = false;
    }
    int curr_stackentries = _tstate->jit_tracer_state.initial_state.stack_depth;
    int length = _tstate->jit_tracer_state.prev_state.code_curr_size;
    if (length <= CODE_SIZE_NO_PROGRESS) {
        return 0;
    }
    assert(length > 0);
    assert(length < UOP_MAX_TRACE_LENGTH);
    OPT_STAT_INC(traces_created);
    if (!is_noopt) {
        length = _Py_uop_analyze_and_optimize(
            _tstate->jit_tracer_state.initial_state.func,
            buffer,length,
            curr_stackentries, dependencies);
        if (length <= 0) {
            return length;
        }
    }
    assert(length < UOP_MAX_TRACE_LENGTH);
    assert(length >= 1);
    /* Fix up */
    for (int pc = 0; pc < length; pc++) {
        int opcode = buffer[pc].opcode;
        int oparg = buffer[pc].oparg;
        if (oparg < _PyUop_Replication[opcode].stop && oparg >= _PyUop_Replication[opcode].start) {
            buffer[pc].opcode = opcode + oparg + 1 - _PyUop_Replication[opcode].start;
            assert(strncmp(_PyOpcode_uop_name[buffer[pc].opcode], _PyOpcode_uop_name[opcode], strlen(_PyOpcode_uop_name[opcode])) == 0);
        }
        else if (is_terminator(&buffer[pc])) {
            break;
        }
        assert(_PyOpcode_uop_name[buffer[pc].opcode]);
    }
    OPT_HIST(effective_trace_length(buffer, length), optimized_trace_length_hist);
    length = prepare_for_execution(buffer, length);
    assert(length <= UOP_MAX_TRACE_LENGTH);
    _PyExecutorObject *executor = make_executor_from_uops(
        buffer, length, dependencies, _tstate->jit_tracer_state.initial_state.chain_depth);
    if (executor == NULL) {
        return -1;
    }
    assert(length <= UOP_MAX_TRACE_LENGTH);

    // Check executor coldness
    // It's okay if this ends up going negative.
    if (--tstate->interp->executor_creation_counter == 0) {
        _Py_set_eval_breaker_bit(tstate, _PY_EVAL_JIT_INVALIDATE_COLD_BIT);
    }

    *exec_ptr = executor;
    return 1;
}


/*****************************************
 *        Executor management
 ****************************************/

/* We use a bloomfilter with k = 6, m = 256
 * The choice of k and the following constants
 * could do with a more rigorous analysis,
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
        uhash *= (uint64_t)PyHASH_MULTIPLIER;
        addr >>= 8;
    }
    return uhash;
}

void
_Py_BloomFilter_Init(_PyBloomFilter *bloom)
{
    for (int i = 0; i < _Py_BLOOM_FILTER_WORDS; i++) {
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
    for (int i = 0; i < _Py_BLOOM_FILTER_WORDS; i++) {
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
        assert(head->vm_data.links.previous == NULL);
        links->previous = NULL;
        links->next = head;
        head->vm_data.links.previous = executor;
        interp->executor_list_head = executor;
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
    assert(executor->vm_data.valid);
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
_Py_ExecutorInit(_PyExecutorObject *executor, const _PyBloomFilter *dependency_set)
{
    executor->vm_data.valid = true;
    for (int i = 0; i < _Py_BLOOM_FILTER_WORDS; i++) {
        executor->vm_data.bloom.bits[i] = dependency_set->bits[i];
    }
    link_executor(executor);
}

_PyExecutorObject *
_PyExecutor_GetColdExecutor(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->cold_executor != NULL) {
        return interp->cold_executor;
    }
    _PyExecutorObject *cold = allocate_executor(0, 1);
    if (cold == NULL) {
        Py_FatalError("Cannot allocate core JIT code");
    }
    ((_PyUOpInstruction *)cold->trace)->opcode = _COLD_EXIT;
#ifdef _Py_JIT
    cold->jit_code = NULL;
    cold->jit_size = 0;
    // This is initialized to true so we can prevent the executor
    // from being immediately detected as cold and invalidated.
    cold->vm_data.warm = true;
    if (_PyJIT_Compile(cold, cold->trace, 1)) {
        Py_DECREF(cold);
        Py_FatalError("Cannot allocate core JIT code");
    }
#endif
    _Py_SetImmortal((PyObject *)cold);
    interp->cold_executor = cold;
    return cold;
}

_PyExecutorObject *
_PyExecutor_GetColdDynamicExecutor(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->cold_dynamic_executor != NULL) {
        assert(interp->cold_dynamic_executor->trace[0].opcode == _COLD_DYNAMIC_EXIT);
        return interp->cold_dynamic_executor;
    }
    _PyExecutorObject *cold = allocate_executor(0, 1);
    if (cold == NULL) {
        Py_FatalError("Cannot allocate core JIT code");
    }
    ((_PyUOpInstruction *)cold->trace)->opcode = _COLD_DYNAMIC_EXIT;
#ifdef _Py_JIT
    cold->jit_code = NULL;
    cold->jit_size = 0;
    // This is initialized to true so we can prevent the executor
    // from being immediately detected as cold and invalidated.
    cold->vm_data.warm = true;
    if (_PyJIT_Compile(cold, cold->trace, 1)) {
        Py_DECREF(cold);
        Py_FatalError("Cannot allocate core JIT code");
    }
#endif
    _Py_SetImmortal((PyObject *)cold);
    interp->cold_dynamic_executor = cold;
    return cold;
}

void
_PyExecutor_ClearExit(_PyExitData *exit)
{
    if (exit == NULL) {
        return;
    }
    _PyExecutorObject *old = exit->executor;
    if (exit->is_dynamic) {
        exit->executor = _PyExecutor_GetColdDynamicExecutor();
    }
    else {
        exit->executor = _PyExecutor_GetColdExecutor();
    }
    Py_DECREF(old);
}

/* Detaches the executor from the code object (if any) that
 * holds a reference to it */
void
_Py_ExecutorDetach(_PyExecutorObject *executor)
{
    PyCodeObject *code = executor->vm_data.code;
    if (code == NULL) {
        return;
    }
    _Py_CODEUNIT *instruction = &_PyCode_CODE(code)[executor->vm_data.index];
    assert(instruction->op.code == ENTER_EXECUTOR);
    int index = instruction->op.arg;
    assert(code->co_executors->executors[index] == executor);
    instruction->op.code = executor->vm_data.opcode;
    instruction->op.arg = executor->vm_data.oparg;
    executor->vm_data.code = NULL;
    code->co_executors->executors[index] = NULL;
    Py_DECREF(executor);
}

static int
executor_clear(PyObject *op)
{
    _PyExecutorObject *executor = _PyExecutorObject_CAST(op);
    if (!executor->vm_data.valid) {
        return 0;
    }
    assert(executor->vm_data.valid == 1);
    unlink_executor(executor);
    executor->vm_data.valid = 0;

    /* It is possible for an executor to form a reference
     * cycle with itself, so decref'ing a side exit could
     * free the executor unless we hold a strong reference to it
     */
    _PyExecutorObject *cold = _PyExecutor_GetColdExecutor();
    Py_INCREF(executor);
    for (uint32_t i = 0; i < executor->exit_count; i++) {
        executor->exits[i].temperature = initial_unreachable_backoff_counter();
        _PyExecutorObject *e = executor->exits[i].executor;
        executor->exits[i].executor = cold;
        Py_DECREF(e);
    }
    _Py_ExecutorDetach(executor);
    Py_DECREF(executor);
    return 0;
}

void
_Py_Executor_DependsOn(_PyExecutorObject *executor, void *obj)
{
    assert(executor->vm_data.valid);
    _Py_BloomFilter_Add(&executor->vm_data.bloom, obj);
}

/* Invalidate all executors that depend on `obj`
 * May cause other executors to be invalidated as well
 */
void
_Py_Executors_InvalidateDependency(PyInterpreterState *interp, void *obj, int is_invalidation)
{
    _PyBloomFilter obj_filter;
    _Py_BloomFilter_Init(&obj_filter);
    _Py_BloomFilter_Add(&obj_filter, obj);
    /* Walk the list of executors */
    /* TO DO -- Use a tree to avoid traversing as many objects */
    PyObject *invalidate = PyList_New(0);
    if (invalidate == NULL) {
        goto error;
    }
    /* Clearing an executor can deallocate others, so we need to make a list of
     * executors to invalidate first */
    for (_PyExecutorObject *exec = interp->executor_list_head; exec != NULL;) {
        assert(exec->vm_data.valid);
        _PyExecutorObject *next = exec->vm_data.links.next;
        if (bloom_filter_may_contain(&exec->vm_data.bloom, &obj_filter) &&
            PyList_Append(invalidate, (PyObject *)exec))
        {
            goto error;
        }
        exec = next;
    }
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(invalidate); i++) {
        PyObject *exec = PyList_GET_ITEM(invalidate, i);
        executor_clear(exec);
        if (is_invalidation) {
            OPT_STAT_INC(executors_invalidated);
        }
    }
    Py_DECREF(invalidate);
    return;
error:
    PyErr_Clear();
    Py_XDECREF(invalidate);
    // If we're truly out of memory, wiping out everything is a fine fallback:
    _Py_Executors_InvalidateAll(interp, is_invalidation);
}

void
_PyJit_Tracer_InvalidateDependency(PyThreadState *tstate, void *obj)
{
    _PyBloomFilter obj_filter;
    _Py_BloomFilter_Init(&obj_filter);
    _Py_BloomFilter_Add(&obj_filter, obj);
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    if (bloom_filter_may_contain(&_tstate->jit_tracer_state.prev_state.dependencies, &obj_filter))
    {
        _tstate->jit_tracer_state.prev_state.dependencies_still_valid = false;
    }
}
/* Invalidate all executors */
void
_Py_Executors_InvalidateAll(PyInterpreterState *interp, int is_invalidation)
{
    while (interp->executor_list_head) {
        _PyExecutorObject *executor = interp->executor_list_head;
        assert(executor->vm_data.valid == 1 && executor->vm_data.linked == 1);
        if (executor->vm_data.code) {
            // Clear the entire code object so its co_executors array be freed:
            _PyCode_Clear_Executors(executor->vm_data.code);
        }
        else {
            executor_clear((PyObject *)executor);
        }
        if (is_invalidation) {
            OPT_STAT_INC(executors_invalidated);
        }
    }
}

void
_Py_Executors_InvalidateCold(PyInterpreterState *interp)
{
    /* Walk the list of executors */
    /* TO DO -- Use a tree to avoid traversing as many objects */
    PyObject *invalidate = PyList_New(0);
    if (invalidate == NULL) {
        goto error;
    }

    /* Clearing an executor can deallocate others, so we need to make a list of
     * executors to invalidate first */
    for (_PyExecutorObject *exec = interp->executor_list_head; exec != NULL;) {
        assert(exec->vm_data.valid);
        _PyExecutorObject *next = exec->vm_data.links.next;

        if (!exec->vm_data.warm && PyList_Append(invalidate, (PyObject *)exec) < 0) {
            goto error;
        }
        else {
            exec->vm_data.warm = false;
        }

        exec = next;
    }
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(invalidate); i++) {
        PyObject *exec = PyList_GET_ITEM(invalidate, i);
        executor_clear(exec);
    }
    Py_DECREF(invalidate);
    return;
error:
    PyErr_Clear();
    Py_XDECREF(invalidate);
    // If we're truly out of memory, wiping out everything is a fine fallback
    _Py_Executors_InvalidateAll(interp, 0);
}

static void
write_str(PyObject *str, FILE *out)
{
    // Encode the Unicode object to the specified encoding
    PyObject *encoded_obj = PyUnicode_AsEncodedString(str, "utf8", "strict");
    if (encoded_obj == NULL) {
        PyErr_Clear();
        return;
    }
    const char *encoded_str = PyBytes_AsString(encoded_obj);
    Py_ssize_t encoded_size = PyBytes_Size(encoded_obj);
    fwrite(encoded_str, 1, encoded_size, out);
    Py_DECREF(encoded_obj);
}

static int
find_line_number(PyCodeObject *code, _PyExecutorObject *executor)
{
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = instr->op.code;
        if (opcode == ENTER_EXECUTOR) {
            _PyExecutorObject *exec = code->co_executors->executors[instr->op.arg];
            if (exec == executor) {
                return PyCode_Addr2Line(code, i*2);
            }
        }
        i += _PyOpcode_Caches[_Py_GetBaseCodeUnit(code, i).op.code];
    }
    return -1;
}

/* Writes the node and outgoing edges for a single tracelet in graphviz format.
 * Each tracelet is presented as a table of the uops it contains.
 * If Py_STATS is enabled, execution counts are included.
 *
 * https://graphviz.readthedocs.io/en/stable/manual.html
 * https://graphviz.org/gallery/
 */
static void
executor_to_gv(_PyExecutorObject *executor, FILE *out)
{
    PyCodeObject *code = executor->vm_data.code;
    fprintf(out, "executor_%p [\n", executor);
    fprintf(out, "    shape = none\n");

    /* Write the HTML table for the uops */
    fprintf(out, "    label = <<table border=\"0\" cellspacing=\"0\">\n");
    fprintf(out, "        <tr><td port=\"start\" border=\"1\" ><b>Executor</b></td></tr>\n");
    if (code == NULL) {
        fprintf(out, "        <tr><td border=\"1\" >No code object</td></tr>\n");
    }
    else {
        fprintf(out, "        <tr><td  border=\"1\" >");
        write_str(code->co_qualname, out);
        int line = find_line_number(code, executor);
        fprintf(out, ": %d</td></tr>\n", line);
    }
    for (uint32_t i = 0; i < executor->code_size; i++) {
        /* Write row for uop.
         * The `port` is a marker so that outgoing edges can
         * be placed correctly. If a row is marked `port=17`,
         * then the outgoing edge is `{EXEC_NAME}:17 -> {TARGET}`
         * https://graphviz.readthedocs.io/en/stable/manual.html#node-ports-compass
         */
        _PyUOpInstruction const *inst = &executor->trace[i];
        const char *opname = _PyOpcode_uop_name[inst->opcode];
#ifdef Py_STATS
        fprintf(out, "        <tr><td port=\"i%d\" border=\"1\" >%s -- %" PRIu64 "</td></tr>\n", i, opname, inst->execution_count);
#else
        fprintf(out, "        <tr><td port=\"i%d\" border=\"1\" >%s op0=%" PRIu64 "</td></tr>\n", i, opname, inst->operand0);
#endif
        if (inst->opcode == _EXIT_TRACE || inst->opcode == _JUMP_TO_TOP) {
            break;
        }
    }
    fprintf(out, "    </table>>\n");
    fprintf(out, "]\n\n");

    /* Write all the outgoing edges */
    _PyExecutorObject *cold = _PyExecutor_GetColdExecutor();
    _PyExecutorObject *cold_dynamic = _PyExecutor_GetColdDynamicExecutor();
    for (uint32_t i = 0; i < executor->code_size; i++) {
        _PyUOpInstruction const *inst = &executor->trace[i];
        uint16_t flags = _PyUop_Flags[inst->opcode];
        _PyExitData *exit = NULL;
        if (inst->opcode == _EXIT_TRACE) {
            exit = (_PyExitData *)inst->operand0;
        }
        else if (flags & HAS_EXIT_FLAG) {
            assert(inst->format == UOP_FORMAT_JUMP);
            _PyUOpInstruction const *exit_inst = &executor->trace[inst->jump_target];
            assert(exit_inst->opcode == _EXIT_TRACE || exit_inst->opcode == _DYNAMIC_EXIT);
            exit = (_PyExitData *)exit_inst->operand0;
        }
        if (exit != NULL && exit->executor != cold && exit->executor != cold_dynamic) {
            fprintf(out, "executor_%p:i%d -> executor_%p:start\n", executor, i, exit->executor);
        }
        if (inst->opcode == _EXIT_TRACE || inst->opcode == _JUMP_TO_TOP) {
            break;
        }
    }
}

/* Write the graph of all the live tracelets in graphviz format. */
int
_PyDumpExecutors(FILE *out)
{
    fprintf(out, "digraph ideal {\n\n");
    fprintf(out, "    rankdir = \"LR\"\n\n");
    PyInterpreterState *interp = PyInterpreterState_Get();
    for (_PyExecutorObject *exec = interp->executor_list_head; exec != NULL;) {
        executor_to_gv(exec, out);
        exec = exec->vm_data.links.next;
    }
    fprintf(out, "}\n\n");
    return 0;
}

#else

int
_PyDumpExecutors(FILE *out)
{
    PyErr_SetString(PyExc_NotImplementedError, "No JIT available");
    return -1;
}

void
_PyExecutor_Free(struct _PyExecutorObject *self)
{
    /* This should never be called */
    Py_UNREACHABLE();
}

#endif /* _Py_TIER2 */
