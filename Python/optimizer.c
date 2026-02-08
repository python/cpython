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
make_executor_from_uops(_PyThreadStateImpl *tstate, _PyUOpInstruction *buffer, int length, const _PyBloomFilter *dependencies);

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
    int chain_depth = _tstate->jit_tracer_state->initial_state.chain_depth;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!interp->jit) {
        // gh-140936: It is possible that interp->jit will become false during
        // interpreter finalization. However, the specialized JUMP_BACKWARD_JIT
        // instruction may still be present. In this case, we should
        // return immediately without optimization.
        return 0;
    }
    _PyExecutorObject *prev_executor = _tstate->jit_tracer_state->initial_state.executor;
    if (prev_executor != NULL && !prev_executor->vm_data.valid) {
        // gh-143604: If we are a side exit executor and the original executor is no
        // longer valid, don't compile to prevent a reference leak.
        return 0;
    }
    assert(!interp->compiling);
    assert(_tstate->jit_tracer_state->initial_state.stack_depth >= 0);
#ifndef Py_GIL_DISABLED
    assert(_tstate->jit_tracer_state->initial_state.func != NULL);
    interp->compiling = true;
    // The first executor in a chain and the MAX_CHAIN_DEPTH'th executor *must*
    // make progress in order to avoid infinite loops or excessively-long
    // side-exit chains. We can only insert the executor into the bytecode if
    // this is true, since a deopt won't infinitely re-enter the executor:
    chain_depth %= MAX_CHAIN_DEPTH;
    bool progress_needed = chain_depth == 0;
    PyCodeObject *code = (PyCodeObject *)_tstate->jit_tracer_state->initial_state.code;
    _Py_CODEUNIT *start = _tstate->jit_tracer_state->initial_state.start_instr;
    if (progress_needed && !has_space_for_executor(code, start)) {
        interp->compiling = false;
        return 0;
    }
    // One of our dependencies while tracing was invalidated. Not worth compiling.
    if (!_tstate->jit_tracer_state->prev_state.dependencies_still_valid) {
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
    executor->vm_data.chain_depth = chain_depth;
    assert(executor->vm_data.valid);
    _PyExitData *exit = _tstate->jit_tracer_state->initial_state.exit;
    if (exit != NULL && !progress_needed) {
        exit->executor = executor;
    }
    else {
        // An executor inserted into the code object now has a strong reference
        // to it from the code object. Thus, we don't need this reference anymore.
        Py_DECREF(executor);
    }
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

void
_PyExecutor_Free(_PyExecutorObject *self)
{
#ifdef _Py_JIT
    _PyJIT_Free(self);
#endif
    PyObject_GC_Del(self);
}

static void executor_invalidate(PyObject *op);

static void
executor_clear_exits(_PyExecutorObject *executor)
{
    _PyExecutorObject *cold = _PyExecutor_GetColdExecutor();
    _PyExecutorObject *cold_dynamic = _PyExecutor_GetColdDynamicExecutor();
    for (uint32_t i = 0; i < executor->exit_count; i++) {
        _PyExitData *exit = &executor->exits[i];
        exit->temperature = initial_unreachable_backoff_counter();
        _PyExecutorObject *old = executor->exits[i].executor;
        exit->executor = exit->is_dynamic ? cold_dynamic : cold;
        Py_DECREF(old);
    }
}


void
_Py_ClearExecutorDeletionList(PyInterpreterState *interp)
{
    if (interp->executor_deletion_list_head == NULL) {
        return;
    }
    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    PyThreadState* ts = PyInterpreterState_ThreadHead(interp);
    while (ts) {
        _PyExecutorObject *current = (_PyExecutorObject *)ts->current_executor;
        Py_XINCREF(current);
        ts = ts->next;
    }
    HEAD_UNLOCK(runtime);
    _PyExecutorObject *keep_list = NULL;
    do {
        _PyExecutorObject *exec = interp->executor_deletion_list_head;
        interp->executor_deletion_list_head = exec->vm_data.links.next;
        if (Py_REFCNT(exec) == 0) {
            _PyExecutor_Free(exec);
        } else {
            exec->vm_data.links.next = keep_list;
            keep_list = exec;
        }
    } while (interp->executor_deletion_list_head != NULL);
    interp->executor_deletion_list_head = keep_list;
    HEAD_LOCK(runtime);
    ts = PyInterpreterState_ThreadHead(interp);
    while (ts) {
        _PyExecutorObject *current = (_PyExecutorObject *)ts->current_executor;
        if (current != NULL) {
            Py_DECREF((PyObject *)current);
        }
        ts = ts->next;
    }
    HEAD_UNLOCK(runtime);
}

static void
add_to_pending_deletion_list(_PyExecutorObject *self)
{
    if (self->vm_data.pending_deletion) {
        return;
    }
    self->vm_data.pending_deletion = 1;
    PyInterpreterState *interp = PyInterpreterState_Get();
    self->vm_data.links.previous = NULL;
    self->vm_data.links.next = interp->executor_deletion_list_head;
    interp->executor_deletion_list_head = self;
}

static void
uop_dealloc(PyObject *op) {
    _PyExecutorObject *self = _PyExecutorObject_CAST(op);
    executor_invalidate(op);
    assert(self->vm_data.code == NULL);
    add_to_pending_deletion_list(self);
}

const char *
_PyUOpName(int index)
{
    if (index < 0 || index > MAX_UOP_REGS_ID) {
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
    if (_PyUop_Flags[_PyUop_Uncached[uop->opcode]] & HAS_ERROR_FLAG) {
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
    int opcode = self->trace[index].opcode;
    int base_opcode = _PyUop_Uncached[opcode];
    const char *name = _PyUOpName(base_opcode);
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


static inline void
add_to_trace(
    _PyJitUopBuffer *trace,
    uint16_t opcode,
    uint16_t oparg,
    uint64_t operand,
    uint32_t target)
{
    _PyUOpInstruction *inst = trace->next;
    inst->opcode = opcode;
    inst->format = UOP_FORMAT_TARGET;
    inst->target = target;
    inst->oparg = oparg;
    inst->operand0 = operand;
#ifdef Py_STATS
    inst->execution_count = 0;
#endif
    trace->next++;
}


#ifdef Py_DEBUG
#define ADD_TO_TRACE(OPCODE, OPARG, OPERAND, TARGET) \
    add_to_trace(trace, (OPCODE), (OPARG), (OPERAND), (TARGET)); \
    if (lltrace >= 2) { \
        printf("%4d ADD_TO_TRACE: ", uop_buffer_length(trace)); \
        _PyUOpPrint(uop_buffer_last(trace)); \
        printf("\n"); \
    }
#else
#define ADD_TO_TRACE(OPCODE, OPARG, OPERAND, TARGET) \
    add_to_trace(trace, (OPCODE), (OPARG), (OPERAND), (TARGET))
#endif

#define INSTR_IP(INSTR, CODE) \
    ((uint32_t)((INSTR) - ((_Py_CODEUNIT *)(CODE)->co_code_adaptive)))


static int
is_terminator(const _PyUOpInstruction *uop)
{
    int opcode = _PyUop_Uncached[uop->opcode];
    return (
        opcode == _EXIT_TRACE ||
        opcode == _DEOPT ||
        opcode == _JUMP_TO_TOP ||
        opcode == _DYNAMIC_EXIT
    );
}

/* Returns 1 on success (added to trace), 0 on trace end.
 */
// gh-142543: inlining this function causes stack overflows
Py_NO_INLINE int
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
    _PyJitTracerState *tracer = _tstate->jit_tracer_state;
    PyCodeObject *old_code = tracer->prev_state.instr_code;
    bool progress_needed = (tracer->initial_state.chain_depth % MAX_CHAIN_DEPTH) == 0;
    _PyBloomFilter *dependencies = &tracer->prev_state.dependencies;
    _PyJitUopBuffer *trace = &tracer->code_buffer;

    _Py_CODEUNIT *this_instr =  tracer->prev_state.instr;
    _Py_CODEUNIT *target_instr = this_instr;
    uint32_t target = 0;

    target = Py_IsNone((PyObject *)old_code)
        ? (uint32_t)(target_instr - _Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS_PTR)
        : INSTR_IP(target_instr, old_code);

    // Rewind EXTENDED_ARG so that we see the whole thing.
    // We must point to the first EXTENDED_ARG when deopting.
    int oparg = tracer->prev_state.instr_oparg;
    int opcode = this_instr->op.code;
    int rewind_oparg = oparg;
    while (rewind_oparg > 255) {
        rewind_oparg >>= 8;
        target--;
    }

    if (_PyOpcode_Caches[_PyOpcode_Deopt[opcode]] > 0) {
        uint16_t backoff = (this_instr + 1)->counter.value_and_backoff;
        // adaptive_counter_cooldown is a fresh specialization.
        // trigger_backoff_counter is what we set during tracing.
        // All tracing backoffs should be freshly specialized or untouched.
        // If not, that indicates a deopt during tracing, and
        // thus the "actual" instruction executed is not the one that is
        // in the instruction stream, but rather the deopt.
        // It's important we check for this, as some specializations might make
        // no progress (they can immediately deopt after specializing).
        // We do this to improve performance, as otherwise a compiled trace
        // will just deopt immediately.
        if (backoff != adaptive_counter_cooldown().value_and_backoff &&
            backoff != trigger_backoff_counter().value_and_backoff) {
            OPT_STAT_INC(trace_immediately_deopts);
            opcode = _PyOpcode_Deopt[opcode];
        }
    }

    // Strange control-flow
    bool has_dynamic_jump_taken = OPCODE_HAS_UNPREDICTABLE_JUMP(opcode) &&
        (next_instr != this_instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]]);

    /* Special case the first instruction,
    * so that we can guarantee forward progress */
    if (progress_needed && uop_buffer_length(&tracer->code_buffer) < CODE_SIZE_NO_PROGRESS) {
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
        goto done;
    }

    if (stop_tracing_opcode == _DEOPT) {
        // gh-143183: It's important we rewind to the last known proper target.
        // The current target might be garbage as stop tracing usually indicates
        // we are in something that we can't trace.
        DPRINTF(2, "Told to stop tracing\n");
        goto unsupported;
    }
    else if (stop_tracing_opcode != 0) {
        assert(stop_tracing_opcode == _EXIT_TRACE);
        ADD_TO_TRACE(stop_tracing_opcode, 0, 0, target);
        goto done;
    }

    DPRINTF(2, "%p %d: %s(%d) %d\n", old_code, target, _PyOpcode_OpName[opcode], oparg, needs_guard_ip);

#ifdef Py_DEBUG
    if (oparg > 255) {
        assert(_Py_GetBaseCodeUnit(old_code, target).op.code == EXTENDED_ARG);
    }
#endif

    if (!tracer->prev_state.dependencies_still_valid) {
        goto done;
    }

    // This happens when a recursive call happens that we can't trace. Such as Python -> C -> Python calls
    // If we haven't guarded the IP, then it's untraceable.
    if (frame != tracer->prev_state.instr_frame && !needs_guard_ip) {
        DPRINTF(2, "Unsupported: unguardable jump taken\n");
        goto unsupported;
    }

    if (oparg > 0xFFFF) {
        DPRINTF(2, "Unsupported: oparg too large\n");
        unsupported:
        {
            // Rewind to previous instruction and replace with _EXIT_TRACE.
            _PyUOpInstruction *curr = uop_buffer_last(trace);
            while (curr->opcode != _SET_IP && uop_buffer_length(trace) > 2) {
                trace->next--;
                curr = uop_buffer_last(trace);
            }
            assert(curr->opcode == _SET_IP || uop_buffer_length(trace) == 2);
            if (curr->opcode == _SET_IP) {
                int32_t old_target = (int32_t)uop_get_target(curr);
                curr->opcode = _DEOPT;
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
    trace->end -= 2;

    const struct opcode_macro_expansion *expansion = &_PyOpcode_macro_expansion[opcode];

    assert(opcode != ENTER_EXECUTOR && opcode != EXTENDED_ARG);
    assert(!_PyErr_Occurred(tstate));


    if (OPCODE_HAS_EXIT(opcode)) {
        // Make space for side exit
        trace->end--;
    }
    if (OPCODE_HAS_ERROR(opcode)) {
        // Make space for error stub
        trace->end--;
    }
    if (OPCODE_HAS_DEOPT(opcode)) {
        // Make space for side exit
        trace->end--;
    }

    // _GUARD_IP leads to an exit.
    trace->end -= needs_guard_ip;

    int space_needed = expansion->nuops + needs_guard_ip + 2 + (!OPCODE_HAS_NO_SAVE_IP(opcode));
    if (uop_buffer_remaining_space(trace) < space_needed) {
        DPRINTF(2, "No room for expansions and guards (need %d, got %d)\n",
                space_needed, uop_buffer_remaining_space(trace));
        OPT_STAT_INC(trace_too_long);
        goto done;
    }

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
            if ((next_instr != tracer->initial_state.close_loop_instr) &&
                (next_instr != tracer->initial_state.start_instr) &&
                uop_buffer_length(&tracer->code_buffer) > CODE_SIZE_NO_PROGRESS &&
                // For side exits, we don't want to terminate them early.
                tracer->initial_state.exit == NULL &&
                // These are coroutines, and we want to unroll those usually.
                opcode != JUMP_BACKWARD_NO_INTERRUPT) {
                // We encountered a JUMP_BACKWARD but not to the top of our own loop.
                // We don't want to continue tracing as we might get stuck in the
                // inner loop. Instead, end the trace where the executor of the
                // inner loop might start and let the traces rejoin.
                OPT_STAT_INC(inner_loop);
                ADD_TO_TRACE(_EXIT_TRACE, 0, 0, target);
                uop_buffer_last(trace)->operand1 = true; // is_control_flow
                DPRINTF(2, "JUMP_BACKWARD not to top ends trace %p %p %p\n", next_instr,
                    tracer->initial_state.close_loop_instr, tracer->initial_state.start_instr);
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
                        assert(uop_buffer_last(trace)->opcode == uop);
                        operand = read_u16(&this_instr[offset].cache);
                        uop_buffer_last(trace)->operand1 = operand;
                        continue;
                    case OPERAND1_2:
                        assert(uop_buffer_last(trace)->opcode == uop);
                        operand = read_u32(&this_instr[offset].cache);
                        uop_buffer_last(trace)->operand1 = operand;
                        continue;
                    case OPERAND1_4:
                        assert(uop_buffer_last(trace)->opcode == uop);
                        operand = read_u64(&this_instr[offset].cache);
                        uop_buffer_last(trace)->operand1 = operand;
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
                    if (new_code != NULL && !Py_IsNone((PyObject*)new_code)) {
                        _Py_BloomFilter_Add(dependencies, new_code);
                    }
                    ADD_TO_TRACE(uop, oparg, operand, target);
                    uop_buffer_last(trace)->operand1 = PyStackRef_IsNone(frame->f_executable) ? 2 : ((int)(frame->stackpointer - _PyFrame_Stackbase(frame)));
                    break;
                }
                if (uop == _BINARY_OP_INPLACE_ADD_UNICODE) {
                    assert(i + 1 == nuops);
                    _Py_CODEUNIT *next = target_instr + 1 + _PyOpcode_Caches[_PyOpcode_Deopt[opcode]];
                    assert(next->op.code == STORE_FAST);
                    operand = next->op.arg;
                }
                else if (_PyUop_Flags[uop] & HAS_RECORDS_VALUE_FLAG) {
                    PyObject *recorded_value = tracer->prev_state.recorded_value;
                    tracer->prev_state.recorded_value = NULL;
                    operand = (uintptr_t)recorded_value;
                }
                // All other instructions
                ADD_TO_TRACE(uop, oparg, operand, target);
            }
            break;
        }  // End default

    }  // End switch (opcode)

    if (needs_guard_ip) {
        uint16_t guard_ip = guard_ip_uop[uop_buffer_last(trace)->opcode];
        if (guard_ip == 0) {
            DPRINTF(1, "Unknown uop needing guard ip %s\n", _PyOpcode_uop_name[uop_buffer_last(trace)->opcode]);
            Py_UNREACHABLE();
        }
        PyObject *code = PyStackRef_AsPyObjectBorrow(frame->f_executable);
        Py_INCREF(code);
        ADD_TO_TRACE(_RECORD_CODE, 0, (uintptr_t)code, 0);
        ADD_TO_TRACE(guard_ip, 0, (uintptr_t)next_instr, 0);
        if (PyCode_Check(code)) {
            ADD_TO_TRACE(_GUARD_CODE, 0, ((PyCodeObject *)code)->co_version, 0);
        }
    }
    // Loop back to the start
    int is_first_instr = tracer->initial_state.close_loop_instr == next_instr ||
        tracer->initial_state.start_instr == next_instr;
    if (is_first_instr && uop_buffer_length(trace) > CODE_SIZE_NO_PROGRESS) {
        if (needs_guard_ip) {
            ADD_TO_TRACE(_SET_IP, 0, (uintptr_t)next_instr, 0);
        }
        ADD_TO_TRACE(_JUMP_TO_TOP, 0, 0, 0);
        goto done;
    }
    DPRINTF(2, "Trace continuing\n");
    return 1;
done:
    DPRINTF(2, "Trace done\n");
    if (!is_terminator(uop_buffer_last(trace))) {
        ADD_TO_TRACE(_EXIT_TRACE, 0, 0, target);
        uop_buffer_last(trace)->operand1 = true; // is_control_flow
    }
    return 0;
}

// Returns 0 for do not enter tracing, 1 on enter tracing.
// gh-142543: inlining this function causes stack overflows
Py_NO_INLINE int
_PyJit_TryInitializeTracing(
    PyThreadState *tstate, _PyInterpreterFrame *frame, _Py_CODEUNIT *curr_instr,
    _Py_CODEUNIT *start_instr, _Py_CODEUNIT *close_loop_instr, _PyStackRef *stack_pointer, int chain_depth,
    _PyExitData *exit, int oparg, _PyExecutorObject *current_executor)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    if (_tstate->jit_tracer_state == NULL) {
        _tstate->jit_tracer_state = (_PyJitTracerState *)_PyObject_VirtualAlloc(sizeof(_PyJitTracerState));
        if (_tstate->jit_tracer_state == NULL) {
            // Don't error, just go to next instruction.
            return 0;
        }
        _tstate->jit_tracer_state->is_tracing = false;
    }
    _PyJitTracerState *tracer = _tstate->jit_tracer_state;
    // A recursive trace.
    if (tracer->is_tracing) {
        return 0;
    }
    if (oparg > 0xFFFF) {
        return 0;
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
    /* Set up tracing buffer*/
    _PyJitUopBuffer *trace = &tracer->code_buffer;
    uop_buffer_init(trace, &tracer->uop_array[0], UOP_MAX_TRACE_LENGTH);
    ADD_TO_TRACE(_START_EXECUTOR, 0, (uintptr_t)start_instr, INSTR_IP(start_instr, code));
    ADD_TO_TRACE(_MAKE_WARM, 0, 0, 0);

    tracer->initial_state.start_instr = start_instr;
    tracer->initial_state.close_loop_instr = close_loop_instr;
    tracer->initial_state.code = (PyCodeObject *)Py_NewRef(code);
    tracer->initial_state.func = (PyFunctionObject *)Py_NewRef(func);
    tracer->initial_state.executor = (_PyExecutorObject *)Py_XNewRef(current_executor);
    tracer->initial_state.exit = exit;
    tracer->initial_state.stack_depth = stack_pointer - _PyFrame_Stackbase(frame);
    tracer->initial_state.chain_depth = chain_depth;
    tracer->prev_state.dependencies_still_valid = true;
    tracer->prev_state.instr_code = (PyCodeObject *)Py_NewRef(_PyFrame_GetCode(frame));
    tracer->prev_state.instr = curr_instr;
    tracer->prev_state.instr_frame = frame;
    tracer->prev_state.instr_oparg = oparg;
    tracer->prev_state.instr_stacklevel = tracer->initial_state.stack_depth;
    tracer->prev_state.recorded_value = NULL;
    uint8_t record_func_index = _PyOpcode_RecordFunctionIndices[curr_instr->op.code];
    if (record_func_index) {
        _Py_RecordFuncPtr record_func = _PyOpcode_RecordFunctions[record_func_index];
        record_func(frame, stack_pointer, oparg, &tracer->prev_state.recorded_value);
    }
    assert(curr_instr->op.code == JUMP_BACKWARD_JIT || (exit != NULL));
    tracer->initial_state.jump_backward_instr = curr_instr;

    if (_PyOpcode_Caches[_PyOpcode_Deopt[close_loop_instr->op.code]]) {
        close_loop_instr[1].counter = trigger_backoff_counter();
    }
    _Py_BloomFilter_Init(&tracer->prev_state.dependencies);
    tracer->is_tracing = true;
    return 1;
}

Py_NO_INLINE void
_PyJit_FinalizeTracing(PyThreadState *tstate, int err)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    _PyJitTracerState *tracer = _tstate->jit_tracer_state;
    // Deal with backoffs
    assert(tracer != NULL);
    _PyExitData *exit = tracer->initial_state.exit;
    if (exit == NULL) {
        // We hold a strong reference to the code object, so the instruction won't be freed.
        if (err <= 0) {
            _Py_BackoffCounter counter = tracer->initial_state.jump_backward_instr[1].counter;
            tracer->initial_state.jump_backward_instr[1].counter = restart_backoff_counter(counter);
        }
        else {
            tracer->initial_state.jump_backward_instr[1].counter = initial_jump_backoff_counter(&tstate->interp->opt_config);
        }
    }
    else if (tracer->initial_state.executor->vm_data.valid) {
        // Likewise, we hold a strong reference to the executor containing this exit, so the exit is guaranteed
        // to be valid to access.
        if (err <= 0) {
            exit->temperature = restart_backoff_counter(exit->temperature);
        }
        else {
            exit->temperature = initial_temperature_backoff_counter(&tstate->interp->opt_config);
        }
    }
    Py_CLEAR(tracer->initial_state.code);
    Py_CLEAR(tracer->initial_state.func);
    Py_CLEAR(tracer->initial_state.executor);
    Py_CLEAR(tracer->prev_state.instr_code);
    Py_CLEAR(tracer->prev_state.recorded_value);
    uop_buffer_init(&tracer->code_buffer, &tracer->uop_array[0], UOP_MAX_TRACE_LENGTH);
    tracer->is_tracing = false;
}

void
_PyJit_TracerFree(_PyThreadStateImpl *_tstate)
{
    if (_tstate->jit_tracer_state != NULL) {
        _PyObject_VirtualFree(_tstate->jit_tracer_state, sizeof(_PyJitTracerState));
        _tstate->jit_tracer_state = NULL;
    }
}

#undef RESERVE
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
        uint16_t base_opcode = _PyUop_Uncached[buffer[i].opcode];
        if (base_opcode == _EXIT_TRACE || base_opcode == _DYNAMIC_EXIT) {
            exit_count++;
        }
    }
    return exit_count;
}

/* The number of cached registers at any exit (`EXIT_IF` or `DEOPT_IF`)
 * This is the number of cached at entries at start, unless the uop is
 * marked as `exit_depth_is_output` in which case it is the number of
 * cached entries at the end */
static int
get_cached_entries_for_side_exit(_PyUOpInstruction *inst)
{
    // Maybe add another generated table for this?
    int base_opcode = _PyUop_Uncached[inst->opcode];
    assert(base_opcode != 0);
    for (int i = 0; i <= MAX_CACHED_REGISTER; i++) {
        const _PyUopTOSentry *entry = &_PyUop_Caching[base_opcode].entries[i];
        if (entry->opcode == inst->opcode) {
            return entry->exit;
        }
    }
    Py_UNREACHABLE();
}

static void make_exit(_PyUOpInstruction *inst, int opcode, int target, bool is_control_flow)
{
    assert(opcode > MAX_UOP_ID && opcode <= MAX_UOP_REGS_ID);
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
        int base_opcode = _PyUop_Uncached[inst->opcode];
        assert(inst->opcode != _NOP);
        int32_t target = (int32_t)uop_get_target(inst);
        uint16_t exit_flags = _PyUop_Flags[base_opcode] & (HAS_EXIT_FLAG | HAS_DEOPT_FLAG | HAS_PERIODIC_FLAG);
        if (exit_flags) {
            uint16_t base_exit_op = _EXIT_TRACE;
            if (exit_flags & HAS_DEOPT_FLAG) {
                base_exit_op = _DEOPT;
            }
            else if (exit_flags & HAS_PERIODIC_FLAG) {
                base_exit_op = _HANDLE_PENDING_AND_DEOPT;
            }
            int32_t jump_target = target;
            if (
                base_opcode == _GUARD_IP__PUSH_FRAME ||
                base_opcode == _GUARD_IP_RETURN_VALUE ||
                base_opcode == _GUARD_IP_YIELD_VALUE ||
                base_opcode == _GUARD_IP_RETURN_GENERATOR ||
                base_opcode == _GUARD_CODE
            ) {
                base_exit_op = _DYNAMIC_EXIT;
            }
            int exit_depth = get_cached_entries_for_side_exit(inst);
            assert(_PyUop_Caching[base_exit_op].entries[exit_depth].opcode > 0);
            int16_t exit_op = _PyUop_Caching[base_exit_op].entries[exit_depth].opcode;
            bool is_control_flow = (base_opcode == _GUARD_IS_FALSE_POP || base_opcode == _GUARD_IS_TRUE_POP || is_for_iter_test[base_opcode]);
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
        if (_PyUop_Flags[base_opcode] & HAS_ERROR_FLAG) {
            int popped = (_PyUop_Flags[base_opcode] & HAS_ERROR_NO_POP_FLAG) ?
                0 : _PyUop_num_popped(base_opcode, inst->oparg);
            if (target != current_error_target || popped != current_popped) {
                current_popped = popped;
                current_error = next_spare;
                current_error_target = target;
                make_exit(&buffer[next_spare], _ERROR_POP_N_r00, 0, false);
                buffer[next_spare].operand0 = target;
                next_spare++;
            }
            buffer[i].error_target = current_error;
            if (buffer[i].format == UOP_FORMAT_TARGET) {
                buffer[i].format = UOP_FORMAT_JUMP;
                buffer[i].jump_target = 0;
            }
        }
        if (base_opcode == _JUMP_TO_TOP) {
            assert(_PyUop_Uncached[buffer[0].opcode] == _START_EXECUTOR);
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
    CHECK(_PyUop_Uncached[executor->trace[0].opcode] == _START_EXECUTOR ||
        _PyUop_Uncached[executor->trace[0].opcode] == _COLD_EXIT ||
        _PyUop_Uncached[executor->trace[0].opcode] == _COLD_DYNAMIC_EXIT);
    for (; i < executor->code_size; i++) {
        const _PyUOpInstruction *inst = &executor->trace[i];
        uint16_t opcode = inst->opcode;
        uint16_t base_opcode = _PyUop_Uncached[opcode];
        CHECK(opcode > MAX_UOP_ID);
        CHECK(opcode <= MAX_UOP_REGS_ID);
        CHECK(base_opcode <= MAX_UOP_ID);
        CHECK(base_opcode != 0);
        switch(inst->format) {
            case UOP_FORMAT_TARGET:
                CHECK(target_unused(base_opcode));
                break;
            case UOP_FORMAT_JUMP:
                CHECK(inst->jump_target < executor->code_size);
                break;
        }
        if (_PyUop_Flags[base_opcode] & HAS_ERROR_FLAG) {
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
        uint16_t base_opcode = _PyUop_Uncached[inst->opcode];
        CHECK(
            base_opcode == _DEOPT ||
            base_opcode == _HANDLE_PENDING_AND_DEOPT ||
            base_opcode == _EXIT_TRACE ||
            base_opcode == _ERROR_POP_N ||
            base_opcode == _DYNAMIC_EXIT);
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
make_executor_from_uops(_PyThreadStateImpl *tstate, _PyUOpInstruction *buffer, int length, const _PyBloomFilter *dependencies)
{
    int exit_count = count_exits(buffer, length);
    _PyExecutorObject *executor = allocate_executor(exit_count, length);
    if (executor == NULL) {
        return NULL;
    }

    /* Initialize exits */
    int chain_depth = tstate->jit_tracer_state->initial_state.chain_depth;
    _PyExecutorObject *cold = _PyExecutor_GetColdExecutor();
    _PyExecutorObject *cold_dynamic = _PyExecutor_GetColdDynamicExecutor();
    cold->vm_data.chain_depth = chain_depth;
    PyInterpreterState *interp = tstate->base.interp;
    for (int i = 0; i < exit_count; i++) {
        executor->exits[i].index = i;
        executor->exits[i].temperature = initial_temperature_backoff_counter(&interp->opt_config);
    }
    int next_exit = exit_count-1;
    _PyUOpInstruction *dest = (_PyUOpInstruction *)&executor->trace[length];
    assert(_PyUop_Uncached[buffer[0].opcode] == _START_EXECUTOR);
    buffer[0].operand0 = (uint64_t)executor;
    for (int i = length-1; i >= 0; i--) {
        uint16_t base_opcode = _PyUop_Uncached[buffer[i].opcode];
        dest--;
        *dest = buffer[i];
        if (base_opcode == _EXIT_TRACE || base_opcode == _DYNAMIC_EXIT) {
            _PyExitData *exit = &executor->exits[next_exit];
            exit->target = buffer[i].target;
            dest->operand0 = (uint64_t)exit;
            exit->executor = base_opcode == _EXIT_TRACE ? cold : cold_dynamic;
            exit->is_dynamic = (char)(base_opcode == _DYNAMIC_EXIT);
            exit->is_control_flow = (char)buffer[i].operand1;
            next_exit--;
        }
    }
    assert(next_exit == -1);
    assert(dest == executor->trace);
    assert(_PyUop_Uncached[dest->opcode] == _START_EXECUTOR);
    // Note: we MUST track it here before any Py_DECREF(executor) or
    // linking of executor. Otherwise, the GC tries to untrack a
    // still untracked object during dealloc.
    _PyObject_GC_TRACK(executor);
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
    // This is initialized to false so we can prevent the executor
    // from being immediately detected as cold and invalidated.
    executor->vm_data.cold = false;
    if (_PyJIT_Compile(executor, executor->trace, length)) {
        Py_DECREF(executor);
        return NULL;
    }
#endif
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
stack_allocate(_PyUOpInstruction *buffer, _PyUOpInstruction *output, int length)
{
    assert(buffer[0].opcode == _START_EXECUTOR);
    /* The input buffer and output buffers will overlap.
       Make sure that we can move instructions to the output
       without overwriting the input. */
    if (buffer == output) {
        // This can only happen if optimizer has not been run
        for (int i = 0; i < length; i++) {
            buffer[i + UOP_MAX_TRACE_LENGTH] = buffer[i];
        }
        buffer += UOP_MAX_TRACE_LENGTH;
    }
    else {
        assert(output + UOP_MAX_TRACE_LENGTH == buffer);
    }
    int depth = 0;
    _PyUOpInstruction *write = output;
    for (int i = 0; i < length; i++) {
        int uop = buffer[i].opcode;
        if (uop == _NOP) {
            continue;
        }
        int new_depth = _PyUop_Caching[uop].best[depth];
        if (new_depth != depth) {
            write->opcode = _PyUop_SpillsAndReloads[depth][new_depth];
            assert(write->opcode != 0);
            write->format = UOP_FORMAT_TARGET;
            write->oparg = 0;
            write->target = 0;
            write++;
            depth = new_depth;
        }
        *write = buffer[i];
        uint16_t new_opcode = _PyUop_Caching[uop].entries[depth].opcode;
        assert(new_opcode != 0);
        write->opcode = new_opcode;
        write++;
        depth = _PyUop_Caching[uop].entries[depth].output;
    }
    return write - output;
}

static int
uop_optimize(
    _PyInterpreterFrame *frame,
    PyThreadState *tstate,
    _PyExecutorObject **exec_ptr,
    bool progress_needed)
{
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    assert(_tstate->jit_tracer_state != NULL);
    _PyBloomFilter *dependencies = &_tstate->jit_tracer_state->prev_state.dependencies;
    _PyUOpInstruction *buffer = _tstate->jit_tracer_state->code_buffer.start;
    OPT_STAT_INC(attempts);
    bool is_noopt = !tstate->interp->opt_config.uops_optimize_enabled;
    int curr_stackentries = _tstate->jit_tracer_state->initial_state.stack_depth;
    int length = uop_buffer_length(&_tstate->jit_tracer_state->code_buffer);
    if (length <= CODE_SIZE_NO_PROGRESS) {
        return 0;
    }
    assert(length > 0);
    assert(length < UOP_MAX_TRACE_LENGTH);
    OPT_STAT_INC(traces_created);
    if (!is_noopt) {
        _PyUOpInstruction *output = &_tstate->jit_tracer_state->uop_array[UOP_MAX_TRACE_LENGTH];
        length = _Py_uop_analyze_and_optimize(
            _tstate, buffer, length, curr_stackentries,
            output, dependencies);
        if (length <= 0) {
            return length;
        }
        buffer = output;
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
        else if (_PyUop_Flags[opcode] & HAS_RECORDS_VALUE_FLAG) {
            Py_XDECREF((PyObject *)(uintptr_t)buffer[pc].operand0);
            buffer[pc].opcode = _NOP;
        }
        else if (is_terminator(&buffer[pc])) {
            break;
        }
        assert(_PyOpcode_uop_name[buffer[pc].opcode]);
    }
    OPT_HIST(effective_trace_length(buffer, length), optimized_trace_length_hist);
    _PyUOpInstruction *output = &_tstate->jit_tracer_state->uop_array[0];
    length = stack_allocate(buffer, output, length);
    buffer = output;
    length = prepare_for_execution(buffer, length);
    assert(length <= UOP_MAX_TRACE_LENGTH);
    _PyExecutorObject *executor = make_executor_from_uops(
        _tstate, buffer, length, dependencies);
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
    /* executor_list_head must be first in list */
    assert(interp->executor_list_head->vm_data.links.previous == NULL);
}

static void
unlink_executor(_PyExecutorObject *executor)
{
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
}

/* This must be called by optimizers before using the executor */
void
_Py_ExecutorInit(_PyExecutorObject *executor, const _PyBloomFilter *dependency_set)
{
    executor->vm_data.valid = true;
    executor->vm_data.pending_deletion = 0;
    executor->vm_data.code = NULL;
    for (int i = 0; i < _Py_BLOOM_FILTER_WORDS; i++) {
        executor->vm_data.bloom.bits[i] = dependency_set->bits[i];
    }
    link_executor(executor);
}

static _PyExecutorObject *
make_cold_executor(uint16_t opcode)
{
    _PyExecutorObject *cold = allocate_executor(0, 1);
    if (cold == NULL) {
        Py_FatalError("Cannot allocate core JIT code");
    }
    ((_PyUOpInstruction *)cold->trace)->opcode = opcode;
    // This is initialized to false so we can prevent the executor
    // from being immediately detected as cold and invalidated.
    cold->vm_data.cold = false;
#ifdef _Py_JIT
    cold->jit_code = NULL;
    cold->jit_size = 0;
    if (_PyJIT_Compile(cold, cold->trace, 1)) {
        Py_DECREF(cold);
        Py_FatalError("Cannot allocate core JIT code");
    }
#endif
    _Py_SetImmortal((PyObject *)cold);
    return cold;
}

_PyExecutorObject *
_PyExecutor_GetColdExecutor(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->cold_executor == NULL) {
        return interp->cold_executor = make_cold_executor(_COLD_EXIT_r00);;
    }
    return interp->cold_executor;
}

_PyExecutorObject *
_PyExecutor_GetColdDynamicExecutor(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->cold_dynamic_executor == NULL) {
        interp->cold_dynamic_executor = make_cold_executor(_COLD_DYNAMIC_EXIT_r00);
    }
    return interp->cold_dynamic_executor;
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

/* Executors can be invalidated at any time,
   even with a stop-the-world lock held.
   Consequently it must not run arbitrary code,
   including Py_DECREF with a non-executor. */
static void
executor_invalidate(PyObject *op)
{
    _PyExecutorObject *executor = _PyExecutorObject_CAST(op);
    if (!executor->vm_data.valid) {
        return;
    }
    executor->vm_data.valid = 0;
    unlink_executor(executor);
    executor_clear_exits(executor);
    _Py_ExecutorDetach(executor);
    _PyObject_GC_UNTRACK(op);
}

static int
executor_clear(PyObject *op)
{
    executor_invalidate(op);
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
    /* Clearing an executor can clear others, so we need to make a list of
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
        executor_invalidate(exec);
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
    if (_tstate->jit_tracer_state == NULL) {
        return;
    }
    if (bloom_filter_may_contain(&_tstate->jit_tracer_state->prev_state.dependencies, &obj_filter))
    {
        _tstate->jit_tracer_state->prev_state.dependencies_still_valid = false;
    }
}
/* Invalidate all executors */
void
_Py_Executors_InvalidateAll(PyInterpreterState *interp, int is_invalidation)
{
    while (interp->executor_list_head) {
        _PyExecutorObject *executor = interp->executor_list_head;
        assert(executor->vm_data.valid);
        if (executor->vm_data.code) {
            // Clear the entire code object so its co_executors array be freed:
            _PyCode_Clear_Executors(executor->vm_data.code);
        }
        else {
            executor_invalidate((PyObject *)executor);
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

        if (exec->vm_data.cold && PyList_Append(invalidate, (PyObject *)exec) < 0) {
            goto error;
        }
        else {
            exec->vm_data.cold = true;
        }

        exec = next;
    }
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(invalidate); i++) {
        PyObject *exec = PyList_GET_ITEM(invalidate, i);
        executor_invalidate(exec);
    }
    Py_DECREF(invalidate);
    return;
error:
    PyErr_Clear();
    Py_XDECREF(invalidate);
    // If we're truly out of memory, wiping out everything is a fine fallback
    _Py_Executors_InvalidateAll(interp, 0);
}

#include "record_functions.c.h"

static int
escape_angles(const char *input, Py_ssize_t size, char *buffer) {
    int written = 0;
    for (Py_ssize_t i = 0; i < size; i++) {
        char c = input[i];
        if (c == '<' || c == '>') {
            buffer[written++] = '&';
            buffer[written++] = c == '>' ? 'g' : 'l';
            buffer[written++] = 't';
            buffer[written++] = ';';
        }
        else {
            buffer[written++] = c;
        }
    }
    return written;
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
    char buffer[120];
    bool truncated = false;
    if (encoded_size > 24) {
        encoded_size = 24;
        truncated = true;
    }
    int size = escape_angles(encoded_str, encoded_size, buffer);
    fwrite(buffer, 1, size, out);
    if (truncated) {
        fwrite("...", 1, 3, out);
    }
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

#define RED "#ff0000"
#define WHITE "#ffffff"
#define BLUE "#0000ff"
#define BLACK "#000000"
#define LOOP "#00c000"

#ifdef Py_STATS

static const char *COLORS[10] = {
    "9",
    "8",
    "7",
    "6",
    "5",
    "4",
    "3",
    "2",
    "1",
    WHITE,
};
const char *
get_background_color(_PyUOpInstruction const *inst, uint64_t max_hotness)
{
    uint64_t hotness = inst->execution_count;
    int index = (hotness * 10)/max_hotness;
    if (index > 9) {
        index = 9;
    }
    if (index < 0) {
        index = 0;
    }
    return COLORS[index];
}

const char *
get_foreground_color(_PyUOpInstruction const *inst, uint64_t max_hotness)
{
    if(_PyUop_Uncached[inst->opcode] == _DEOPT) {
        return RED;
    }
    uint64_t hotness = inst->execution_count;
    int index = (hotness * 10)/max_hotness;
    if (index > 3) {
        return BLACK;
    }
    return WHITE;
}
#endif

static void
write_row_for_uop(_PyExecutorObject *executor, uint32_t i, FILE *out)
{
    /* Write row for uop.
        * The `port` is a marker so that outgoing edges can
        * be placed correctly. If a row is marked `port=17`,
        * then the outgoing edge is `{EXEC_NAME}:17 -> {TARGET}`
        * https://graphviz.readthedocs.io/en/stable/manual.html#node-ports-compass
        */
    _PyUOpInstruction const *inst = &executor->trace[i];
    const char *opname = _PyOpcode_uop_name[inst->opcode];
#ifdef Py_STATS
    const char *bg_color = get_background_color(inst, executor->trace[0].execution_count);
    const char *color = get_foreground_color(inst, executor->trace[0].execution_count);
    fprintf(out, "        <tr><td port=\"i%d\" border=\"1\" color=\"%s\" bgcolor=\"%s\" ><font color=\"%s\"> %s &nbsp;--&nbsp; %" PRIu64 "</font></td></tr>\n",
        i, color, bg_color, color, opname, inst->execution_count);
#else
    const char *color = (_PyUop_Uncached[inst->opcode] == _DEOPT) ? RED : BLACK;
    fprintf(out, "        <tr><td port=\"i%d\" border=\"1\" color=\"%s\" >%s op0=%" PRIu64 "</td></tr>\n", i, color, opname, inst->operand0);
#endif
}

static bool
is_stop(_PyUOpInstruction const *inst)
{
    uint16_t base_opcode = _PyUop_Uncached[inst->opcode];
    return (base_opcode == _EXIT_TRACE || base_opcode == _DEOPT || base_opcode == _JUMP_TO_TOP);
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
        write_row_for_uop(executor, i, out);
        if (is_stop(&executor->trace[i])) {
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
        uint16_t base_opcode = _PyUop_Uncached[inst->opcode];
        uint16_t flags = _PyUop_Flags[base_opcode];
        _PyExitData *exit = NULL;
        if (base_opcode == _JUMP_TO_TOP) {
            fprintf(out, "executor_%p:i%d -> executor_%p:i%d [color = \"" LOOP "\"]\n", executor, i, executor, inst->jump_target);
            break;
        }
        if (base_opcode == _EXIT_TRACE) {
            exit = (_PyExitData *)inst->operand0;
        }
        else if (flags & HAS_EXIT_FLAG) {
            assert(inst->format == UOP_FORMAT_JUMP);
            _PyUOpInstruction const *exit_inst = &executor->trace[inst->jump_target];
            uint16_t base_exit_opcode = _PyUop_Uncached[exit_inst->opcode];
            assert(base_exit_opcode == _EXIT_TRACE || base_exit_opcode == _DYNAMIC_EXIT);
            exit = (_PyExitData *)exit_inst->operand0;
        }
        if (exit != NULL) {
            if (exit->executor == cold || exit->executor == cold_dynamic) {
#ifdef Py_STATS
                /* Only mark as have cold exit if it has actually exited */
                uint64_t diff = inst->execution_count - executor->trace[i+1].execution_count;
                if (diff) {
                    fprintf(out, "cold_%p%d [ label = \"%"  PRIu64  "\" shape = ellipse color=\"" BLUE "\" ]\n", executor, i, diff);
                    fprintf(out, "executor_%p:i%d -> cold_%p%d\n", executor, i, executor, i);
                }
#endif
            }
            else {
                fprintf(out, "executor_%p:i%d -> executor_%p:start\n", executor, i, exit->executor);
            }
        }
        if (is_stop(inst)) {
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
    fprintf(out, "    node [colorscheme=greys9]\n");
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
