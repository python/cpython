#include "Python.h"

#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pylifecycle.h"     // _PyOS_URandomNonblock()
#include "pycore_tstate.h"
#include "pycore_initconfig.h"      // _PyStatus_OK()
#include "pycore_uop_metadata.h"    // _PyOpcode_uop_name
#include "pycore_uop_ids.h"         // MAX_UOP_ID
#include "pycore_pystate.h"         // _PyThreadState_GET()
#include "pycore_runtime.h"         // NUM_GENERATIONS

#include <stdlib.h> // rand()

#ifdef Py_STATS

PyStats *
_PyStats_GetLocal(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate) {
        return tstate->pystats;
    }
    return NULL;
}

#ifdef Py_GIL_DISABLED
#define STATS_LOCK(interp) PyMutex_Lock(&interp->pystats_mutex)
#define STATS_UNLOCK(interp) PyMutex_Unlock(&interp->pystats_mutex)
#else
#define STATS_LOCK(interp)
#define STATS_UNLOCK(interp)
#endif


#if PYSTATS_MAX_UOP_ID < MAX_UOP_ID
#error "Not enough space allocated for pystats. Increase PYSTATS_MAX_UOP_ID to at least MAX_UOP_ID"
#endif

#define ADD_STAT_TO_DICT(res, field) \
    do { \
        PyObject *val = PyLong_FromUnsignedLongLong(stats->field); \
        if (val == NULL) { \
            Py_DECREF(res); \
            return NULL; \
        } \
        if (PyDict_SetItemString(res, #field, val) == -1) { \
            Py_DECREF(res); \
            Py_DECREF(val); \
            return NULL; \
        } \
        Py_DECREF(val); \
    } while(0);

static PyObject*
stats_to_dict(SpecializationStats *stats)
{
    PyObject *res = PyDict_New();
    if (res == NULL) {
        return NULL;
    }
    ADD_STAT_TO_DICT(res, success);
    ADD_STAT_TO_DICT(res, failure);
    ADD_STAT_TO_DICT(res, hit);
    ADD_STAT_TO_DICT(res, deferred);
    ADD_STAT_TO_DICT(res, miss);
    ADD_STAT_TO_DICT(res, deopt);
    PyObject *failure_kinds = PyTuple_New(SPECIALIZATION_FAILURE_KINDS);
    if (failure_kinds == NULL) {
        Py_DECREF(res);
        return NULL;
    }
    for (int i = 0; i < SPECIALIZATION_FAILURE_KINDS; i++) {
        PyObject *stat = PyLong_FromUnsignedLongLong(stats->failure_kinds[i]);
        if (stat == NULL) {
            Py_DECREF(res);
            Py_DECREF(failure_kinds);
            return NULL;
        }
        PyTuple_SET_ITEM(failure_kinds, i, stat);
    }
    if (PyDict_SetItemString(res, "failure_kinds", failure_kinds)) {
        Py_DECREF(res);
        Py_DECREF(failure_kinds);
        return NULL;
    }
    Py_DECREF(failure_kinds);
    return res;
}
#undef ADD_STAT_TO_DICT

static int
add_stat_dict(
    PyStats *src,
    PyObject *res,
    int opcode,
    const char *name) {

    SpecializationStats *stats = &src->opcode_stats[opcode].specialization;
    PyObject *d = stats_to_dict(stats);
    if (d == NULL) {
        return -1;
    }
    int err = PyDict_SetItemString(res, name, d);
    Py_DECREF(d);
    return err;
}

PyObject*
_Py_GetSpecializationStats(void) {
    PyThreadState *tstate = _PyThreadState_GET();
    PyStats *src = FT_ATOMIC_LOAD_PTR_RELAXED(tstate->interp->pystats_struct);
    if (src == NULL) {
        Py_RETURN_NONE;
    }
    PyObject *stats = PyDict_New();
    if (stats == NULL) {
        return NULL;
    }
    int err = 0;
    err += add_stat_dict(src, stats, CONTAINS_OP, "contains_op");
    err += add_stat_dict(src, stats, LOAD_SUPER_ATTR, "load_super_attr");
    err += add_stat_dict(src, stats, LOAD_ATTR, "load_attr");
    err += add_stat_dict(src, stats, LOAD_GLOBAL, "load_global");
    err += add_stat_dict(src, stats, STORE_SUBSCR, "store_subscr");
    err += add_stat_dict(src, stats, STORE_ATTR, "store_attr");
    err += add_stat_dict(src, stats, JUMP_BACKWARD, "jump_backward");
    err += add_stat_dict(src, stats, CALL, "call");
    err += add_stat_dict(src, stats, CALL_KW, "call_kw");
    err += add_stat_dict(src, stats, BINARY_OP, "binary_op");
    err += add_stat_dict(src, stats, COMPARE_OP, "compare_op");
    err += add_stat_dict(src, stats, UNPACK_SEQUENCE, "unpack_sequence");
    err += add_stat_dict(src, stats, FOR_ITER, "for_iter");
    err += add_stat_dict(src, stats, TO_BOOL, "to_bool");
    err += add_stat_dict(src, stats, SEND, "send");
    if (err < 0) {
        Py_DECREF(stats);
        return NULL;
    }
    return stats;
}


#define PRINT_STAT(i, field) \
    if (stats[i].field) { \
        fprintf(out, "    opcode[%s]." #field " : %" PRIu64 "\n", _PyOpcode_OpName[i], stats[i].field); \
    }

static void
print_spec_stats(FILE *out, OpcodeStats *stats)
{
    /* Mark some opcodes as specializable for stats,
     * even though we don't specialize them yet. */
    fprintf(out, "opcode[BINARY_SLICE].specializable : 1\n");
    fprintf(out, "opcode[STORE_SLICE].specializable : 1\n");
    fprintf(out, "opcode[GET_ITER].specializable : 1\n");
    for (int i = 0; i < 256; i++) {
        if (_PyOpcode_Caches[i]) {
            /* Ignore jumps as they cannot be specialized */
            switch (i) {
                case POP_JUMP_IF_FALSE:
                case POP_JUMP_IF_TRUE:
                case POP_JUMP_IF_NONE:
                case POP_JUMP_IF_NOT_NONE:
                case JUMP_BACKWARD:
                    break;
                default:
                    fprintf(out, "opcode[%s].specializable : 1\n", _PyOpcode_OpName[i]);
            }
        }
        PRINT_STAT(i, specialization.success);
        PRINT_STAT(i, specialization.failure);
        PRINT_STAT(i, specialization.hit);
        PRINT_STAT(i, specialization.deferred);
        PRINT_STAT(i, specialization.miss);
        PRINT_STAT(i, specialization.deopt);
        PRINT_STAT(i, execution_count);
        for (int j = 0; j < SPECIALIZATION_FAILURE_KINDS; j++) {
            uint64_t val = stats[i].specialization.failure_kinds[j];
            if (val) {
                fprintf(out, "    opcode[%s].specialization.failure_kinds[%d] : %"
                    PRIu64 "\n", _PyOpcode_OpName[i], j, val);
            }
        }
        for (int j = 0; j < 256; j++) {
            if (stats[i].pair_count[j]) {
                fprintf(out, "opcode[%s].pair_count[%s] : %" PRIu64 "\n",
                        _PyOpcode_OpName[i], _PyOpcode_OpName[j], stats[i].pair_count[j]);
            }
        }
    }
}
#undef PRINT_STAT


static void
print_call_stats(FILE *out, CallStats *stats)
{
    fprintf(out, "Calls to PyEval_EvalDefault: %" PRIu64 "\n", stats->pyeval_calls);
    fprintf(out, "Calls to Python functions inlined: %" PRIu64 "\n", stats->inlined_py_calls);
    fprintf(out, "Frames pushed: %" PRIu64 "\n", stats->frames_pushed);
    fprintf(out, "Frame objects created: %" PRIu64 "\n", stats->frame_objects_created);
    for (int i = 0; i < EVAL_CALL_KINDS; i++) {
        fprintf(out, "Calls via PyEval_EvalFrame[%d] : %" PRIu64 "\n", i, stats->eval_calls[i]);
    }
}

static void
print_object_stats(FILE *out, ObjectStats *stats)
{
    fprintf(out, "Object allocations from freelist: %" PRIu64 "\n", stats->from_freelist);
    fprintf(out, "Object frees to freelist: %" PRIu64 "\n", stats->to_freelist);
    fprintf(out, "Object allocations: %" PRIu64 "\n", stats->allocations);
    fprintf(out, "Object allocations to 512 bytes: %" PRIu64 "\n", stats->allocations512);
    fprintf(out, "Object allocations to 4 kbytes: %" PRIu64 "\n", stats->allocations4k);
    fprintf(out, "Object allocations over 4 kbytes: %" PRIu64 "\n", stats->allocations_big);
    fprintf(out, "Object frees: %" PRIu64 "\n", stats->frees);
    fprintf(out, "Object inline values: %" PRIu64 "\n", stats->inline_values);
    fprintf(out, "Object interpreter mortal increfs: %" PRIu64 "\n", stats->interpreter_increfs);
    fprintf(out, "Object interpreter mortal decrefs: %" PRIu64 "\n", stats->interpreter_decrefs);
    fprintf(out, "Object mortal increfs: %" PRIu64 "\n", stats->increfs);
    fprintf(out, "Object mortal decrefs: %" PRIu64 "\n", stats->decrefs);
    fprintf(out, "Object interpreter immortal increfs: %" PRIu64 "\n", stats->interpreter_immortal_increfs);
    fprintf(out, "Object interpreter immortal decrefs: %" PRIu64 "\n", stats->interpreter_immortal_decrefs);
    fprintf(out, "Object immortal increfs: %" PRIu64 "\n", stats->immortal_increfs);
    fprintf(out, "Object immortal decrefs: %" PRIu64 "\n", stats->immortal_decrefs);
    fprintf(out, "Object materialize dict (on request): %" PRIu64 "\n", stats->dict_materialized_on_request);
    fprintf(out, "Object materialize dict (new key): %" PRIu64 "\n", stats->dict_materialized_new_key);
    fprintf(out, "Object materialize dict (too big): %" PRIu64 "\n", stats->dict_materialized_too_big);
    fprintf(out, "Object materialize dict (str subclass): %" PRIu64 "\n", stats->dict_materialized_str_subclass);
    fprintf(out, "Object method cache hits: %" PRIu64 "\n", stats->type_cache_hits);
    fprintf(out, "Object method cache misses: %" PRIu64 "\n", stats->type_cache_misses);
    fprintf(out, "Object method cache collisions: %" PRIu64 "\n", stats->type_cache_collisions);
    fprintf(out, "Object method cache dunder hits: %" PRIu64 "\n", stats->type_cache_dunder_hits);
    fprintf(out, "Object method cache dunder misses: %" PRIu64 "\n", stats->type_cache_dunder_misses);
}

static void
print_gc_stats(FILE *out, GCStats *stats)
{
    for (int i = 0; i < NUM_GENERATIONS; i++) {
        fprintf(out, "GC[%d] collections: %" PRIu64 "\n", i, stats[i].collections);
        fprintf(out, "GC[%d] object visits: %" PRIu64 "\n", i, stats[i].object_visits);
        fprintf(out, "GC[%d] objects collected: %" PRIu64 "\n", i, stats[i].objects_collected);
        fprintf(out, "GC[%d] objects reachable from roots: %" PRIu64 "\n", i, stats[i].objects_transitively_reachable);
        fprintf(out, "GC[%d] objects not reachable from roots: %" PRIu64 "\n", i, stats[i].objects_not_transitively_reachable);
    }
}

#ifdef _Py_TIER2
static void
print_histogram(FILE *out, const char *name, uint64_t hist[_Py_UOP_HIST_SIZE])
{
    for (int i = 0; i < _Py_UOP_HIST_SIZE; i++) {
        fprintf(out, "%s[%" PRIu64"]: %" PRIu64 "\n", name, (uint64_t)1 << i, hist[i]);
    }
}

extern const char *_PyUOpName(int index);

static void
print_optimization_stats(FILE *out, OptimizationStats *stats)
{
    fprintf(out, "Optimization attempts: %" PRIu64 "\n", stats->attempts);
    fprintf(out, "Optimization traces created: %" PRIu64 "\n", stats->traces_created);
    fprintf(out, "Optimization traces executed: %" PRIu64 "\n", stats->traces_executed);
    fprintf(out, "Optimization uops executed: %" PRIu64 "\n", stats->uops_executed);
    fprintf(out, "Optimization trace stack overflow: %" PRIu64 "\n", stats->trace_stack_overflow);
    fprintf(out, "Optimization trace stack underflow: %" PRIu64 "\n", stats->trace_stack_underflow);
    fprintf(out, "Optimization trace too long: %" PRIu64 "\n", stats->trace_too_long);
    fprintf(out, "Optimization trace too short: %" PRIu64 "\n", stats->trace_too_short);
    fprintf(out, "Optimization inner loop: %" PRIu64 "\n", stats->inner_loop);
    fprintf(out, "Optimization recursive call: %" PRIu64 "\n", stats->recursive_call);
    fprintf(out, "Optimization low confidence: %" PRIu64 "\n", stats->low_confidence);
    fprintf(out, "Optimization unknown callee: %" PRIu64 "\n", stats->unknown_callee);
    fprintf(out, "Executors invalidated: %" PRIu64 "\n", stats->executors_invalidated);

    print_histogram(out, "Trace length", stats->trace_length_hist);
    print_histogram(out, "Trace run length", stats->trace_run_length_hist);
    print_histogram(out, "Optimized trace length", stats->optimized_trace_length_hist);

    fprintf(out, "Optimization optimizer attempts: %" PRIu64 "\n", stats->optimizer_attempts);
    fprintf(out, "Optimization optimizer successes: %" PRIu64 "\n", stats->optimizer_successes);
    fprintf(out, "Optimization optimizer failure no memory: %" PRIu64 "\n",
            stats->optimizer_failure_reason_no_memory);
    fprintf(out, "Optimizer remove globals builtins changed: %" PRIu64 "\n", stats->remove_globals_builtins_changed);
    fprintf(out, "Optimizer remove globals incorrect keys: %" PRIu64 "\n", stats->remove_globals_incorrect_keys);
    for (int i = 0; i <= MAX_UOP_ID; i++) {
        if (stats->opcode[i].execution_count) {
            fprintf(out, "uops[%s].execution_count : %" PRIu64 "\n", _PyUOpName(i), stats->opcode[i].execution_count);
        }
        if (stats->opcode[i].miss) {
            fprintf(out, "uops[%s].specialization.miss : %" PRIu64 "\n", _PyUOpName(i), stats->opcode[i].miss);
        }
    }
    for (int i = 0; i < 256; i++) {
        if (stats->unsupported_opcode[i]) {
            fprintf(
                out,
                "unsupported_opcode[%s].count : %" PRIu64 "\n",
                _PyOpcode_OpName[i],
                stats->unsupported_opcode[i]
            );
        }
    }

    for (int i = 1; i <= MAX_UOP_ID; i++){
        for (int j = 1; j <= MAX_UOP_ID; j++) {
            if (stats->opcode[i].pair_count[j]) {
                fprintf(out, "uop[%s].pair_count[%s] : %" PRIu64 "\n",
                        _PyOpcode_uop_name[i], _PyOpcode_uop_name[j], stats->opcode[i].pair_count[j]);
            }
        }
    }
    for (int i = 0; i < MAX_UOP_ID; i++) {
        if (stats->error_in_opcode[i]) {
            fprintf(
                out,
                "error_in_opcode[%s].count : %" PRIu64 "\n",
                _PyUOpName(i),
                stats->error_in_opcode[i]
            );
        }
    }
    fprintf(out, "JIT total memory size: %" PRIu64 "\n", stats->jit_total_memory_size);
    fprintf(out, "JIT code size: %" PRIu64 "\n", stats->jit_code_size);
    fprintf(out, "JIT trampoline size: %" PRIu64 "\n", stats->jit_trampoline_size);
    fprintf(out, "JIT data size: %" PRIu64 "\n", stats->jit_data_size);
    fprintf(out, "JIT padding size: %" PRIu64 "\n", stats->jit_padding_size);
    fprintf(out, "JIT freed memory size: %" PRIu64 "\n", stats->jit_freed_memory_size);

    print_histogram(out, "Trace total memory size", stats->trace_total_memory_hist);
}
#endif

#ifdef Py_GIL_DISABLED
static void
print_ft_stats(FILE *out, FTStats *stats)
{
    fprintf(out, "Mutex sleeps (mutex_sleeps): %" PRIu64 "\n", stats->mutex_sleeps);
    fprintf(out, "QSBR polls (qsbr_polls): %" PRIu64 "\n", stats->qsbr_polls);
    fprintf(out, "World stops (world_stops): %" PRIu64 "\n", stats->world_stops);
}
#endif

static void
print_rare_event_stats(FILE *out, RareEventStats *stats)
{
    fprintf(out, "Rare event (set_class): %" PRIu64 "\n", stats->set_class);
    fprintf(out, "Rare event (set_bases): %" PRIu64 "\n", stats->set_bases);
    fprintf(out, "Rare event (set_eval_frame_func): %" PRIu64 "\n", stats->set_eval_frame_func);
    fprintf(out, "Rare event (builtin_dict): %" PRIu64 "\n", stats->builtin_dict);
    fprintf(out, "Rare event (func_modification): %" PRIu64 "\n", stats->func_modification);
    fprintf(out, "Rare event (watched_dict_modification): %" PRIu64 "\n", stats->watched_dict_modification);
    fprintf(out, "Rare event (watched_globals_modification): %" PRIu64 "\n", stats->watched_globals_modification);
}

static void
print_stats(FILE *out, PyStats *stats)
{
    print_spec_stats(out, stats->opcode_stats);
    print_call_stats(out, &stats->call_stats);
    print_object_stats(out, &stats->object_stats);
    print_gc_stats(out, stats->gc_stats);
#ifdef _Py_TIER2
    print_optimization_stats(out, &stats->optimization_stats);
#endif
#ifdef Py_GIL_DISABLED
    print_ft_stats(out, &stats->ft_stats);
#endif
    print_rare_event_stats(out, &stats->rare_event_stats);
}

#ifdef Py_GIL_DISABLED

static void
merge_specialization_stats(SpecializationStats *dest, const SpecializationStats *src)
{
    dest->success += src->success;
    dest->failure += src->failure;
    dest->hit += src->hit;
    dest->deferred += src->deferred;
    dest->miss += src->miss;
    dest->deopt += src->deopt;
    for (int i = 0; i < SPECIALIZATION_FAILURE_KINDS; i++) {
        dest->failure_kinds[i] += src->failure_kinds[i];
    }
}

static void
merge_opcode_stats_array(OpcodeStats *dest, const OpcodeStats *src)
{
    for (int i = 0; i < 256; i++) {
        merge_specialization_stats(&dest[i].specialization, &src[i].specialization);
        dest[i].execution_count += src[i].execution_count;
        for (int j = 0; j < 256; j++) {
            dest[i].pair_count[j] += src[i].pair_count[j];
        }
    }
}

static void
merge_call_stats(CallStats *dest, const CallStats *src)
{
    dest->inlined_py_calls += src->inlined_py_calls;
    dest->pyeval_calls += src->pyeval_calls;
    dest->frames_pushed += src->frames_pushed;
    dest->frame_objects_created += src->frame_objects_created;
    for (int i = 0; i < EVAL_CALL_KINDS; i++) {
        dest->eval_calls[i] += src->eval_calls[i];
    }
}

static void
merge_object_stats(ObjectStats *dest, const ObjectStats *src)
{
    dest->increfs += src->increfs;
    dest->decrefs += src->decrefs;
    dest->interpreter_increfs += src->interpreter_increfs;
    dest->interpreter_decrefs += src->interpreter_decrefs;
    dest->immortal_increfs += src->immortal_increfs;
    dest->immortal_decrefs += src->immortal_decrefs;
    dest->interpreter_immortal_increfs += src->interpreter_immortal_increfs;
    dest->interpreter_immortal_decrefs += src->interpreter_immortal_decrefs;
    dest->allocations += src->allocations;
    dest->allocations512 += src->allocations512;
    dest->allocations4k += src->allocations4k;
    dest->allocations_big += src->allocations_big;
    dest->frees += src->frees;
    dest->to_freelist += src->to_freelist;
    dest->from_freelist += src->from_freelist;
    dest->inline_values += src->inline_values;
    dest->dict_materialized_on_request += src->dict_materialized_on_request;
    dest->dict_materialized_new_key += src->dict_materialized_new_key;
    dest->dict_materialized_too_big += src->dict_materialized_too_big;
    dest->dict_materialized_str_subclass += src->dict_materialized_str_subclass;
    dest->type_cache_hits += src->type_cache_hits;
    dest->type_cache_misses += src->type_cache_misses;
    dest->type_cache_dunder_hits += src->type_cache_dunder_hits;
    dest->type_cache_dunder_misses += src->type_cache_dunder_misses;
    dest->type_cache_collisions += src->type_cache_collisions;
    dest->object_visits += src->object_visits;
}

static void
merge_uop_stats_array(UOpStats *dest, const UOpStats *src)
{
    for (int i = 0; i <= PYSTATS_MAX_UOP_ID; i++) {
        dest[i].execution_count += src[i].execution_count;
        dest[i].miss += src[i].miss;
        for (int j = 0; j <= PYSTATS_MAX_UOP_ID; j++) {
            dest[i].pair_count[j] += src[i].pair_count[j];
        }
    }
}

static void
merge_optimization_stats(OptimizationStats *dest, const OptimizationStats *src)
{
    dest->attempts += src->attempts;
    dest->traces_created += src->traces_created;
    dest->traces_executed += src->traces_executed;
    dest->uops_executed += src->uops_executed;
    dest->trace_stack_overflow += src->trace_stack_overflow;
    dest->trace_stack_underflow += src->trace_stack_underflow;
    dest->trace_too_long += src->trace_too_long;
    dest->trace_too_short += src->trace_too_short;
    dest->inner_loop += src->inner_loop;
    dest->recursive_call += src->recursive_call;
    dest->low_confidence += src->low_confidence;
    dest->unknown_callee += src->unknown_callee;
    dest->executors_invalidated += src->executors_invalidated;
    dest->optimizer_attempts += src->optimizer_attempts;
    dest->optimizer_successes += src->optimizer_successes;
    dest->optimizer_failure_reason_no_memory += src->optimizer_failure_reason_no_memory;
    dest->remove_globals_builtins_changed += src->remove_globals_builtins_changed;
    dest->remove_globals_incorrect_keys += src->remove_globals_incorrect_keys;
    dest->jit_total_memory_size += src->jit_total_memory_size;
    dest->jit_code_size += src->jit_code_size;
    dest->jit_trampoline_size += src->jit_trampoline_size;
    dest->jit_data_size += src->jit_data_size;
    dest->jit_padding_size += src->jit_padding_size;
    dest->jit_freed_memory_size += src->jit_freed_memory_size;

    merge_uop_stats_array(dest->opcode, src->opcode);

    for (int i = 0; i < 256; i++) {
        dest->unsupported_opcode[i] += src->unsupported_opcode[i];
    }
    for (int i = 0; i < _Py_UOP_HIST_SIZE; i++) {
        dest->trace_length_hist[i] += src->trace_length_hist[i];
        dest->trace_run_length_hist[i] += src->trace_run_length_hist[i];
        dest->optimized_trace_length_hist[i] += src->optimized_trace_length_hist[i];
        dest->trace_total_memory_hist[i] += src->trace_total_memory_hist[i];
    }
    for (int i = 0; i <= PYSTATS_MAX_UOP_ID; i++) {
        dest->error_in_opcode[i] += src->error_in_opcode[i];
    }
}

static void
merge_ft_stats(FTStats *dest, const FTStats *src)
{
    dest->mutex_sleeps = src->mutex_sleeps;
    dest->qsbr_polls = src->qsbr_polls;
    dest->world_stops = src->world_stops;
}

static void
merge_rare_event_stats(RareEventStats *dest, const RareEventStats *src)
{
    dest->set_class += src->set_class;
    dest->set_bases += src->set_bases;
    dest->set_eval_frame_func += src->set_eval_frame_func;
    dest->builtin_dict += src->builtin_dict;
    dest->func_modification += src->func_modification;
    dest->watched_dict_modification += src->watched_dict_modification;
    dest->watched_globals_modification += src->watched_globals_modification;
}

static void
merge_gc_stats_array(GCStats *dest, const GCStats *src)
{
    for (int i = 0; i < NUM_GENERATIONS; i++) {
        dest[i].collections += src[i].collections;
        dest[i].object_visits += src[i].object_visits;
        dest[i].objects_collected += src[i].objects_collected;
        dest[i].objects_transitively_reachable += src[i].objects_transitively_reachable;
        dest[i].objects_not_transitively_reachable += src[i].objects_not_transitively_reachable;
    }
}

void
stats_zero_thread(_PyThreadStateImpl *tstate)
{
    // Zero the thread local stat counters
    if (tstate->pystats_struct) {
        memset(tstate->pystats_struct, 0, sizeof(PyStats));
    }
}

// merge stats for a single thread into the global structure
void
stats_merge_thread(_PyThreadStateImpl *tstate)
{
    PyStats *src = tstate->pystats_struct;
    PyStats *dest = ((PyThreadState *)tstate)->interp->pystats_struct;

    if (src == NULL || dest == NULL) {
        return;
    }

    // Merge each category of stats using the helper functions.
    merge_opcode_stats_array(dest->opcode_stats, src->opcode_stats);
    merge_call_stats(&dest->call_stats, &src->call_stats);
    merge_object_stats(&dest->object_stats, &src->object_stats);
    merge_optimization_stats(&dest->optimization_stats, &src->optimization_stats);
    merge_ft_stats(&dest->ft_stats, &src->ft_stats);
    merge_rare_event_stats(&dest->rare_event_stats, &src->rare_event_stats);
    merge_gc_stats_array(dest->gc_stats, src->gc_stats);
}
#endif // Py_GIL_DISABLED

// toggle stats collection on or off for all threads
static int
stats_toggle_on_off(PyThreadState *tstate, int on)
{
    bool changed = false;
    PyInterpreterState *interp = tstate->interp;
    STATS_LOCK(interp);
    if (on && interp->pystats_struct == NULL) {
        PyStats *s = PyMem_RawCalloc(1, sizeof(PyStats));
        if (s == NULL) {
            STATS_UNLOCK(interp);
            return -1;
        }
        FT_ATOMIC_STORE_PTR_RELAXED(interp->pystats_struct, s);
    }
    if (tstate->interp->pystats_enabled != on) {
        FT_ATOMIC_STORE_INT_RELAXED(interp->pystats_enabled, on);
        changed = true;
    }
    STATS_UNLOCK(interp);
    if (!changed) {
        return 0;
    }
    _PyEval_StopTheWorld(interp);
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, ts) {
        PyStats *s = NULL;
        if (interp->pystats_enabled) {
#ifdef Py_GIL_DISABLED
            _PyThreadStateImpl *ts_impl = (_PyThreadStateImpl *)ts;
            if (ts_impl->pystats_struct == NULL) {
                // first activation for this thread, allocate structure
                ts_impl->pystats_struct = PyMem_RawCalloc(1, sizeof(PyStats));
            }
            s = ts_impl->pystats_struct;
#else
            s = ts->interp->pystats_struct;
#endif
        }
        ts->pystats = s;
    }
    _PyEval_StartTheWorld(interp);
    return 0;
}

// zero stats for all threads and for the interpreter
static void
stats_zero_all(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        return;
    }
    if (FT_ATOMIC_LOAD_PTR_RELAXED(tstate->interp->pystats_struct) == NULL) {
        return;
    }
    PyInterpreterState *interp = tstate->interp;
    _PyEval_StopTheWorld(interp);
#ifdef Py_GIL_DISABLED
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, ts) {
        stats_zero_thread((_PyThreadStateImpl *)ts);
    }
#endif
    if (interp->pystats_struct) {
        memset(interp->pystats_struct, 0, sizeof(PyStats));
    }
    _PyEval_StartTheWorld(interp);
}

// merge stats for all threads into the per-interpreter structure
static void
stats_merge_all(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        return;
    }
    if (FT_ATOMIC_LOAD_PTR_RELAXED(tstate->interp->pystats_struct) == NULL) {
        return;
    }
    PyInterpreterState *interp = tstate->interp;
    _PyEval_StopTheWorld(interp);
#ifdef Py_GIL_DISABLED
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, ts) {
        stats_merge_thread((_PyThreadStateImpl *)ts);
        stats_zero_thread((_PyThreadStateImpl *)ts);
    }
#endif
    _PyEval_StartTheWorld(interp);
}

int
_Py_StatsOn(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return stats_toggle_on_off(tstate, 1);
}

void
_Py_StatsOff(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    stats_toggle_on_off(tstate, 0);
}

void
_Py_StatsClear(void)
{
    stats_zero_all();
}

static int
mem_is_zero(unsigned char *ptr, size_t size)
{
    for (size_t i=0; i < size; i++) {
        if (*ptr != 0) {
            return 0;
        }
        ptr++;
    }
    return 1;
}

int
_Py_PrintSpecializationStats(int to_file)
{
    assert(to_file);
    stats_merge_all();
    PyThreadState *tstate = _PyThreadState_GET();
    STATS_LOCK(tstate->interp);
    PyStats *stats = tstate->interp->pystats_struct;
    if (stats == NULL) {
        STATS_UNLOCK(tstate->interp);
        return 0;
    }
#define MEM_IS_ZERO(DATA) mem_is_zero((unsigned char*)DATA, sizeof(*(DATA)))
    int is_zero = (
        MEM_IS_ZERO(stats->gc_stats)  // is a pointer
        && MEM_IS_ZERO(&stats->opcode_stats)
        && MEM_IS_ZERO(&stats->call_stats)
        && MEM_IS_ZERO(&stats->object_stats)
    );
#undef MEM_IS_ZERO
    STATS_UNLOCK(tstate->interp);
    if (is_zero) {
        // gh-108753: -X pystats command line was used, but then _stats_off()
        // and _stats_clear() have been called: in this case, avoid printing
        // useless "all zeros" statistics.
        return 0;
    }

    FILE *out = stderr;
    if (to_file) {
        /* Write to a file instead of stderr. */
# ifdef MS_WINDOWS
        const char *dirname = "c:\\temp\\py_stats\\";
# else
        const char *dirname = "/tmp/py_stats/";
# endif
        /* Use random 160 bit number as file name,
        * to avoid both accidental collisions and
        * symlink attacks. */
        unsigned char rand[20];
        char hex_name[41];
        _PyOS_URandomNonblock(rand, 20);
        for (int i = 0; i < 20; i++) {
            hex_name[2*i] = Py_hexdigits[rand[i]&15];
            hex_name[2*i+1] = Py_hexdigits[(rand[i]>>4)&15];
        }
        hex_name[40] = '\0';
        char buf[64];
        assert(strlen(dirname) + 40 + strlen(".txt") < 64);
        sprintf(buf, "%s%s.txt", dirname, hex_name);
        FILE *fout = fopen(buf, "w");
        if (fout) {
            out = fout;
        }
    }
    else {
        fprintf(out, "Specialization stats:\n");
    }
    STATS_LOCK(tstate->interp);
    print_stats(out, stats);
    STATS_UNLOCK(tstate->interp);
    if (out != stderr) {
        fclose(out);
    }
    return 1;
}

PyStatus
_PyStats_InterpInit(PyInterpreterState *interp)
{
    if (interp->config._pystats) {
        // start with pystats enabled, can be disabled via sys._stats_off()
        // this needs to be set before the first tstate is created
        interp->pystats_enabled = 1;
        interp->pystats_struct = PyMem_RawCalloc(1, sizeof(PyStats));
        if (interp->pystats_struct == NULL) {
            return _PyStatus_ERR("out-of-memory while initializing interpreter");
        }
    }
    return _PyStatus_OK();
}

bool
_PyStats_ThreadInit(PyInterpreterState *interp, _PyThreadStateImpl *tstate)
{
#ifdef Py_GIL_DISABLED
    if (FT_ATOMIC_LOAD_INT_RELAXED(interp->pystats_enabled)) {
        assert(interp->pystats_struct != NULL);
        tstate->pystats_struct = PyMem_RawCalloc(1, sizeof(PyStats));
        if (tstate->pystats_struct == NULL) {
            return false;
        }
    }
#endif
    return true;
}

void
_PyStats_ThreadFini(_PyThreadStateImpl *tstate)
{
#ifdef Py_GIL_DISABLED
    STATS_LOCK(((PyThreadState *)tstate)->interp);
    stats_merge_thread(tstate);
    STATS_UNLOCK(((PyThreadState *)tstate)->interp);
    PyMem_RawFree(tstate->pystats_struct);
#endif
}

void
_PyStats_Attach(_PyThreadStateImpl *tstate_impl)
{
    PyStats *s;
    PyThreadState *tstate = (PyThreadState *)tstate_impl;
    PyInterpreterState *interp = tstate->interp;
    if (FT_ATOMIC_LOAD_INT_RELAXED(interp->pystats_enabled)) {
#ifdef Py_GIL_DISABLED
        s = ((_PyThreadStateImpl *)tstate)->pystats_struct;
#else
        s = tstate->interp->pystats_struct;
#endif
    }
    else {
        s = NULL;
    }
    tstate->pystats = s;
}

void
_PyStats_Detach(_PyThreadStateImpl *tstate_impl)
{
    ((PyThreadState *)tstate_impl)->pystats = NULL;
}

#endif // Py_STATS
