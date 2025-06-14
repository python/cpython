#include "Python.h"

#include "opcode.h"

#include "pycore_code.h"
#include "pycore_critical_section.h"
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_dict.h"          // DICT_KEYS_UNICODE
#include "pycore_function.h"      // _PyFunction_GetVersionForCurrentState()
#include "pycore_interpframe.h"   // FRAME_SPECIALS_SIZE
#include "pycore_list.h"          // _PyListIterObject
#include "pycore_long.h"          // _PyLong_IsNonNegativeCompact()
#include "pycore_moduleobject.h"
#include "pycore_object.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_uop_metadata.h"    // _PyOpcode_uop_name
#include "pycore_uop_ids.h"       // MAX_UOP_ID
#include "pycore_opcode_utils.h"  // RESUME_AT_FUNC_START
#include "pycore_pylifecycle.h"   // _PyOS_URandomNonblock()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_unicodeobject.h" // _PyUnicodeASCIIIter_Type

#include <stdlib.h> // rand()

extern const char *_PyUOpName(int index);

/* For guidance on adding or extending families of instructions see
 * InternalDocs/interpreter.md `Specialization` section.
 */

#ifdef Py_STATS
GCStats _py_gc_stats[NUM_GENERATIONS] = { 0 };
static PyStats _Py_stats_struct = { .gc_stats = _py_gc_stats };
PyStats *_Py_stats = NULL;

#if PYSTATS_MAX_UOP_ID < MAX_UOP_REGS_ID
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
    PyObject *res,
    int opcode,
    const char *name) {

    SpecializationStats *stats = &_Py_stats_struct.opcode_stats[opcode].specialization;
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
    PyObject *stats = PyDict_New();
    if (stats == NULL) {
        return NULL;
    }
    int err = 0;
    err += add_stat_dict(stats, CONTAINS_OP, "contains_op");
    err += add_stat_dict(stats, LOAD_SUPER_ATTR, "load_super_attr");
    err += add_stat_dict(stats, LOAD_ATTR, "load_attr");
    err += add_stat_dict(stats, LOAD_GLOBAL, "load_global");
    err += add_stat_dict(stats, STORE_SUBSCR, "store_subscr");
    err += add_stat_dict(stats, STORE_ATTR, "store_attr");
    err += add_stat_dict(stats, CALL, "call");
    err += add_stat_dict(stats, CALL_KW, "call_kw");
    err += add_stat_dict(stats, BINARY_OP, "binary_op");
    err += add_stat_dict(stats, COMPARE_OP, "compare_op");
    err += add_stat_dict(stats, UNPACK_SEQUENCE, "unpack_sequence");
    err += add_stat_dict(stats, FOR_ITER, "for_iter");
    err += add_stat_dict(stats, TO_BOOL, "to_bool");
    err += add_stat_dict(stats, SEND, "send");
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
    print_rare_event_stats(out, &stats->rare_event_stats);
}

void
_Py_StatsOn(void)
{
    _Py_stats = &_Py_stats_struct;
}

void
_Py_StatsOff(void)
{
    _Py_stats = NULL;
}

void
_Py_StatsClear(void)
{
    memset(&_py_gc_stats, 0, sizeof(_py_gc_stats));
    memset(&_Py_stats_struct, 0, sizeof(_Py_stats_struct));
    _Py_stats_struct.gc_stats = _py_gc_stats;
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
    PyStats *stats = &_Py_stats_struct;
#define MEM_IS_ZERO(DATA) mem_is_zero((unsigned char*)DATA, sizeof(*(DATA)))
    int is_zero = (
        MEM_IS_ZERO(stats->gc_stats)  // is a pointer
        && MEM_IS_ZERO(&stats->opcode_stats)
        && MEM_IS_ZERO(&stats->call_stats)
        && MEM_IS_ZERO(&stats->object_stats)
    );
#undef MEM_IS_ZERO
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
    print_stats(out, stats);
    if (out != stderr) {
        fclose(out);
    }
    return 1;
}

#define SPECIALIZATION_FAIL(opcode, kind) \
do { \
    if (_Py_stats) { \
        int _kind = (kind); \
        assert(_kind < SPECIALIZATION_FAILURE_KINDS); \
        _Py_stats->opcode_stats[opcode].specialization.failure_kinds[_kind]++; \
    } \
} while (0)

#endif  // Py_STATS


#ifndef SPECIALIZATION_FAIL
#  define SPECIALIZATION_FAIL(opcode, kind) ((void)0)
#endif

// Initialize warmup counters and optimize instructions. This cannot fail.
void
_PyCode_Quicken(_Py_CODEUNIT *instructions, Py_ssize_t size, int enable_counters)
{
    #if ENABLE_SPECIALIZATION_FT
    _Py_BackoffCounter jump_counter, adaptive_counter;
    if (enable_counters) {
        jump_counter = initial_jump_backoff_counter();
        adaptive_counter = adaptive_counter_warmup();
    }
    else {
        jump_counter = initial_unreachable_backoff_counter();
        adaptive_counter = initial_unreachable_backoff_counter();
    }
    int opcode = 0;
    int oparg = 0;
    /* The last code unit cannot have a cache, so we don't need to check it */
    for (Py_ssize_t i = 0; i < size-1; i++) {
        opcode = instructions[i].op.code;
        int caches = _PyOpcode_Caches[opcode];
        oparg = (oparg << 8) | instructions[i].op.arg;
        if (caches) {
            // The initial value depends on the opcode
            switch (opcode) {
                case JUMP_BACKWARD:
                    instructions[i + 1].counter = jump_counter;
                    break;
                case POP_JUMP_IF_FALSE:
                case POP_JUMP_IF_TRUE:
                case POP_JUMP_IF_NONE:
                case POP_JUMP_IF_NOT_NONE:
                    instructions[i + 1].cache = 0x5555;  // Alternating 0, 1 bits
                    break;
                default:
                    instructions[i + 1].counter = adaptive_counter;
                    break;
            }
            i += caches;
        }
        if (opcode != EXTENDED_ARG) {
            oparg = 0;
        }
    }
    #endif /* ENABLE_SPECIALIZATION_FT */
}

#define SIMPLE_FUNCTION 0

/* Common */

#define SPEC_FAIL_OTHER 0
#define SPEC_FAIL_NO_DICT 1
#define SPEC_FAIL_OVERRIDDEN 2
#define SPEC_FAIL_OUT_OF_VERSIONS 3
#define SPEC_FAIL_OUT_OF_RANGE 4
#define SPEC_FAIL_EXPECTED_ERROR 5
#define SPEC_FAIL_WRONG_NUMBER_ARGUMENTS 6
#define SPEC_FAIL_CODE_COMPLEX_PARAMETERS 7
#define SPEC_FAIL_CODE_NOT_OPTIMIZED 8


#define SPEC_FAIL_LOAD_GLOBAL_NON_DICT 17
#define SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT 18

/* Super */

#define SPEC_FAIL_SUPER_BAD_CLASS 9
#define SPEC_FAIL_SUPER_SHADOWED 10

/* Attributes */

#define SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR 9
#define SPEC_FAIL_ATTR_NON_OVERRIDING_DESCRIPTOR 10
#define SPEC_FAIL_ATTR_NOT_DESCRIPTOR 11
#define SPEC_FAIL_ATTR_METHOD 12
#define SPEC_FAIL_ATTR_MUTABLE_CLASS 13
#define SPEC_FAIL_ATTR_PROPERTY 14
#define SPEC_FAIL_ATTR_NON_OBJECT_SLOT 15
#define SPEC_FAIL_ATTR_READ_ONLY 16
#define SPEC_FAIL_ATTR_AUDITED_SLOT 17
#define SPEC_FAIL_ATTR_NOT_MANAGED_DICT 18
#define SPEC_FAIL_ATTR_NON_STRING 19
#define SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND 20
#define SPEC_FAIL_ATTR_SHADOWED 21
#define SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD 22
#define SPEC_FAIL_ATTR_CLASS_METHOD_OBJ 23
#define SPEC_FAIL_ATTR_OBJECT_SLOT 24

#define SPEC_FAIL_ATTR_INSTANCE_ATTRIBUTE 26
#define SPEC_FAIL_ATTR_METACLASS_ATTRIBUTE 27
#define SPEC_FAIL_ATTR_PROPERTY_NOT_PY_FUNCTION 28
#define SPEC_FAIL_ATTR_NOT_IN_KEYS 29
#define SPEC_FAIL_ATTR_NOT_IN_DICT 30
#define SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE 31
#define SPEC_FAIL_ATTR_CLASS_ATTR_DESCRIPTOR 32
#define SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD_OBJ 33
#define SPEC_FAIL_ATTR_METACLASS_OVERRIDDEN 34
#define SPEC_FAIL_ATTR_SPLIT_DICT 35
#define SPEC_FAIL_ATTR_DESCR_NOT_DEFERRED 36

/* Binary subscr and store subscr */

#define SPEC_FAIL_SUBSCR_ARRAY_INT 9
#define SPEC_FAIL_SUBSCR_ARRAY_SLICE 10
#define SPEC_FAIL_SUBSCR_LIST_SLICE 11
#define SPEC_FAIL_SUBSCR_BUFFER_INT 12
#define SPEC_FAIL_SUBSCR_BUFFER_SLICE 13

/* Store subscr */
#define SPEC_FAIL_SUBSCR_BYTEARRAY_INT 18
#define SPEC_FAIL_SUBSCR_BYTEARRAY_SLICE 19
#define SPEC_FAIL_SUBSCR_PY_SIMPLE 20
#define SPEC_FAIL_SUBSCR_PY_OTHER 21
#define SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE 22
#define SPEC_FAIL_SUBSCR_NOT_HEAP_TYPE 23

/* Binary op */

#define SPEC_FAIL_BINARY_OP_ADD_DIFFERENT_TYPES          9
#define SPEC_FAIL_BINARY_OP_ADD_OTHER                   10
#define SPEC_FAIL_BINARY_OP_AND_DIFFERENT_TYPES         11
#define SPEC_FAIL_BINARY_OP_AND_INT                     12
#define SPEC_FAIL_BINARY_OP_AND_OTHER                   13
#define SPEC_FAIL_BINARY_OP_FLOOR_DIVIDE                14
#define SPEC_FAIL_BINARY_OP_LSHIFT                      15
#define SPEC_FAIL_BINARY_OP_MATRIX_MULTIPLY             16
#define SPEC_FAIL_BINARY_OP_MULTIPLY_DIFFERENT_TYPES    17
#define SPEC_FAIL_BINARY_OP_MULTIPLY_OTHER              18
#define SPEC_FAIL_BINARY_OP_OR                          19
#define SPEC_FAIL_BINARY_OP_POWER                       20
#define SPEC_FAIL_BINARY_OP_REMAINDER                   21
#define SPEC_FAIL_BINARY_OP_RSHIFT                      22
#define SPEC_FAIL_BINARY_OP_SUBTRACT_DIFFERENT_TYPES    23
#define SPEC_FAIL_BINARY_OP_SUBTRACT_OTHER              24
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_DIFFERENT_TYPES 25
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_FLOAT           26
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_OTHER           27
#define SPEC_FAIL_BINARY_OP_XOR                         28
#define SPEC_FAIL_BINARY_OP_OR_INT                      29
#define SPEC_FAIL_BINARY_OP_OR_DIFFERENT_TYPES          30
#define SPEC_FAIL_BINARY_OP_XOR_INT                     31
#define SPEC_FAIL_BINARY_OP_XOR_DIFFERENT_TYPES         32
#define SPEC_FAIL_BINARY_OP_SUBSCR                      33
#define SPEC_FAIL_BINARY_OP_SUBSCR_LIST_SLICE           34
#define SPEC_FAIL_BINARY_OP_SUBSCR_TUPLE_SLICE          35
#define SPEC_FAIL_BINARY_OP_SUBSCR_STRING_SLICE         36
#define SPEC_FAIL_BINARY_OP_SUBSCR_NOT_HEAP_TYPE        37
#define SPEC_FAIL_BINARY_OP_SUBSCR_OTHER_SLICE          38
#define SPEC_FAIL_BINARY_OP_SUBSCR_MAPPINGPROXY         39
#define SPEC_FAIL_BINARY_OP_SUBSCR_RE_MATCH             40
#define SPEC_FAIL_BINARY_OP_SUBSCR_ARRAY                41
#define SPEC_FAIL_BINARY_OP_SUBSCR_DEQUE                42
#define SPEC_FAIL_BINARY_OP_SUBSCR_ENUMDICT             43
#define SPEC_FAIL_BINARY_OP_SUBSCR_STACKSUMMARY         44
#define SPEC_FAIL_BINARY_OP_SUBSCR_DEFAULTDICT          45
#define SPEC_FAIL_BINARY_OP_SUBSCR_COUNTER              46
#define SPEC_FAIL_BINARY_OP_SUBSCR_ORDEREDDICT          47
#define SPEC_FAIL_BINARY_OP_SUBSCR_BYTES                48
#define SPEC_FAIL_BINARY_OP_SUBSCR_STRUCTTIME           49
#define SPEC_FAIL_BINARY_OP_SUBSCR_RANGE                50

/* Calls */

#define SPEC_FAIL_CALL_INSTANCE_METHOD 11
#define SPEC_FAIL_CALL_CMETHOD 12
#define SPEC_FAIL_CALL_CFUNC_VARARGS 13
#define SPEC_FAIL_CALL_CFUNC_VARARGS_KEYWORDS 14
#define SPEC_FAIL_CALL_CFUNC_NOARGS 15
#define SPEC_FAIL_CALL_CFUNC_METHOD_FASTCALL_KEYWORDS 16
#define SPEC_FAIL_CALL_METH_DESCR_VARARGS 17
#define SPEC_FAIL_CALL_METH_DESCR_VARARGS_KEYWORDS 18
#define SPEC_FAIL_CALL_METH_DESCR_METHOD_FASTCALL_KEYWORDS 19
#define SPEC_FAIL_CALL_BAD_CALL_FLAGS 20
#define SPEC_FAIL_CALL_INIT_NOT_PYTHON 21
#define SPEC_FAIL_CALL_PEP_523 22
#define SPEC_FAIL_CALL_BOUND_METHOD 23
#define SPEC_FAIL_CALL_CLASS_MUTABLE 26
#define SPEC_FAIL_CALL_METHOD_WRAPPER 28
#define SPEC_FAIL_CALL_OPERATOR_WRAPPER 29
#define SPEC_FAIL_CALL_INIT_NOT_SIMPLE 30
#define SPEC_FAIL_CALL_METACLASS 31
#define SPEC_FAIL_CALL_INIT_NOT_INLINE_VALUES 32

/* COMPARE_OP */
#define SPEC_FAIL_COMPARE_OP_DIFFERENT_TYPES 12
#define SPEC_FAIL_COMPARE_OP_STRING 13
#define SPEC_FAIL_COMPARE_OP_BIG_INT 14
#define SPEC_FAIL_COMPARE_OP_BYTES 15
#define SPEC_FAIL_COMPARE_OP_TUPLE 16
#define SPEC_FAIL_COMPARE_OP_LIST 17
#define SPEC_FAIL_COMPARE_OP_SET 18
#define SPEC_FAIL_COMPARE_OP_BOOL 19
#define SPEC_FAIL_COMPARE_OP_BASEOBJECT 20
#define SPEC_FAIL_COMPARE_OP_FLOAT_LONG 21
#define SPEC_FAIL_COMPARE_OP_LONG_FLOAT 22

/* FOR_ITER and SEND */
#define SPEC_FAIL_ITER_GENERATOR 10
#define SPEC_FAIL_ITER_COROUTINE 11
#define SPEC_FAIL_ITER_ASYNC_GENERATOR 12
#define SPEC_FAIL_ITER_LIST 13
#define SPEC_FAIL_ITER_TUPLE 14
#define SPEC_FAIL_ITER_SET 15
#define SPEC_FAIL_ITER_STRING 16
#define SPEC_FAIL_ITER_BYTES 17
#define SPEC_FAIL_ITER_RANGE 18
#define SPEC_FAIL_ITER_ITERTOOLS 19
#define SPEC_FAIL_ITER_DICT_KEYS 20
#define SPEC_FAIL_ITER_DICT_ITEMS 21
#define SPEC_FAIL_ITER_DICT_VALUES 22
#define SPEC_FAIL_ITER_ENUMERATE 23
#define SPEC_FAIL_ITER_MAP 24
#define SPEC_FAIL_ITER_ZIP 25
#define SPEC_FAIL_ITER_SEQ_ITER 26
#define SPEC_FAIL_ITER_REVERSED_LIST 27
#define SPEC_FAIL_ITER_CALLABLE 28
#define SPEC_FAIL_ITER_ASCII_STRING 29
#define SPEC_FAIL_ITER_ASYNC_GENERATOR_SEND 30
#define SPEC_FAIL_ITER_SELF 31

// UNPACK_SEQUENCE

#define SPEC_FAIL_UNPACK_SEQUENCE_ITERATOR 9
#define SPEC_FAIL_UNPACK_SEQUENCE_SEQUENCE 10

// TO_BOOL
#define SPEC_FAIL_TO_BOOL_BYTEARRAY    9
#define SPEC_FAIL_TO_BOOL_BYTES       10
#define SPEC_FAIL_TO_BOOL_DICT        11
#define SPEC_FAIL_TO_BOOL_FLOAT       12
#define SPEC_FAIL_TO_BOOL_MAPPING     13
#define SPEC_FAIL_TO_BOOL_MEMORY_VIEW 14
#define SPEC_FAIL_TO_BOOL_NUMBER      15
#define SPEC_FAIL_TO_BOOL_SEQUENCE    16
#define SPEC_FAIL_TO_BOOL_SET         17
#define SPEC_FAIL_TO_BOOL_TUPLE       18

// CONTAINS_OP
#define SPEC_FAIL_CONTAINS_OP_STR        9
#define SPEC_FAIL_CONTAINS_OP_TUPLE      10
#define SPEC_FAIL_CONTAINS_OP_LIST       11
#define SPEC_FAIL_CONTAINS_OP_USER_CLASS 12

static inline int
set_opcode(_Py_CODEUNIT *instr, uint8_t opcode)
{
#ifdef Py_GIL_DISABLED
    uint8_t old_op = _Py_atomic_load_uint8_relaxed(&instr->op.code);
    if (old_op >= MIN_INSTRUMENTED_OPCODE) {
        /* Lost race with instrumentation */
        return 0;
    }
    if (!_Py_atomic_compare_exchange_uint8(&instr->op.code, &old_op, opcode)) {
        /* Lost race with instrumentation */
        assert(old_op >= MIN_INSTRUMENTED_OPCODE);
        return 0;
    }
    return 1;
#else
    instr->op.code = opcode;
    return 1;
#endif
}

static inline void
set_counter(_Py_BackoffCounter *counter, _Py_BackoffCounter value)
{
    FT_ATOMIC_STORE_UINT16_RELAXED(counter->value_and_backoff,
                                   value.value_and_backoff);
}

static inline _Py_BackoffCounter
load_counter(_Py_BackoffCounter *counter)
{
    _Py_BackoffCounter result = {
        .value_and_backoff =
            FT_ATOMIC_LOAD_UINT16_RELAXED(counter->value_and_backoff)};
    return result;
}

static inline void
specialize(_Py_CODEUNIT *instr, uint8_t specialized_opcode)
{
    assert(!PyErr_Occurred());
    if (!set_opcode(instr, specialized_opcode)) {
        STAT_INC(_PyOpcode_Deopt[specialized_opcode], failure);
        SPECIALIZATION_FAIL(_PyOpcode_Deopt[specialized_opcode],
                            SPEC_FAIL_OTHER);
        return;
    }
    STAT_INC(_PyOpcode_Deopt[specialized_opcode], success);
    set_counter((_Py_BackoffCounter *)instr + 1, adaptive_counter_cooldown());
}

static inline void
unspecialize(_Py_CODEUNIT *instr)
{
    assert(!PyErr_Occurred());
    uint8_t opcode = FT_ATOMIC_LOAD_UINT8_RELAXED(instr->op.code);
    uint8_t generic_opcode = _PyOpcode_Deopt[opcode];
    STAT_INC(generic_opcode, failure);
    if (!set_opcode(instr, generic_opcode)) {
        SPECIALIZATION_FAIL(generic_opcode, SPEC_FAIL_OTHER);
        return;
    }
    _Py_BackoffCounter *counter = (_Py_BackoffCounter *)instr + 1;
    _Py_BackoffCounter cur = load_counter(counter);
    set_counter(counter, adaptive_counter_backoff(cur));
}

static int function_kind(PyCodeObject *code);
static bool function_check_args(PyObject *o, int expected_argcount, int opcode);
static uint32_t function_get_version(PyObject *o, int opcode);

static int
specialize_module_load_attr_lock_held(PyDictObject *dict, _Py_CODEUNIT *instr, PyObject *name)
{
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    if (dict->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_STRING);
        return -1;
    }
    Py_ssize_t index = _PyDict_LookupIndex(dict, &_Py_ID(__getattr__));
    assert(index != DKIX_ERROR);
    if (index != DKIX_EMPTY) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND);
        return -1;
    }
    index = _PyDict_LookupIndex(dict, name);
    assert (index != DKIX_ERROR);
    if (index != (uint16_t)index) {
        SPECIALIZATION_FAIL(LOAD_ATTR,
                            index == DKIX_EMPTY ?
                            SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND :
                            SPEC_FAIL_OUT_OF_RANGE);
        return -1;
    }
    uint32_t keys_version = _PyDict_GetKeysVersionForCurrentState(
            _PyInterpreterState_GET(), dict);
    if (keys_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    write_u32(cache->version, keys_version);
    cache->index = (uint16_t)index;
    specialize(instr, LOAD_ATTR_MODULE);
    return 0;
}

static int
specialize_module_load_attr(
    PyObject *owner, _Py_CODEUNIT *instr, PyObject *name)
{
    PyModuleObject *m = (PyModuleObject *)owner;
    assert((Py_TYPE(owner)->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);
    PyDictObject *dict = (PyDictObject *)m->md_dict;
    if (dict == NULL) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_NO_DICT);
        return -1;
    }
    int result;
    Py_BEGIN_CRITICAL_SECTION(dict);
    result = specialize_module_load_attr_lock_held(dict, instr, name);
    Py_END_CRITICAL_SECTION();
    return result;
}

/* Attribute specialization */

Py_NO_INLINE void
_Py_Specialize_LoadSuperAttr(_PyStackRef global_super_st, _PyStackRef cls_st, _Py_CODEUNIT *instr, int load_method) {
    PyObject *global_super = PyStackRef_AsPyObjectBorrow(global_super_st);
    PyObject *cls = PyStackRef_AsPyObjectBorrow(cls_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[LOAD_SUPER_ATTR] == INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR);
    if (global_super != (PyObject *)&PySuper_Type) {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_SHADOWED);
        goto fail;
    }
    if (!PyType_Check(cls)) {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_BAD_CLASS);
        goto fail;
    }
    uint8_t load_code = load_method ? LOAD_SUPER_ATTR_METHOD : LOAD_SUPER_ATTR_ATTR;
    specialize(instr, load_code);
    return;
fail:
    unspecialize(instr);
}

typedef enum {
    OVERRIDING, /* Is an overriding descriptor, and will remain so. */
    METHOD, /* Attribute has Py_TPFLAGS_METHOD_DESCRIPTOR set */
    PROPERTY, /* Is a property */
    OBJECT_SLOT, /* Is an object slot descriptor */
    OTHER_SLOT, /* Is a slot descriptor of another type */
    NON_OVERRIDING, /* Is another non-overriding descriptor, and is an instance of an immutable class*/
    BUILTIN_CLASSMETHOD, /* Builtin methods with METH_CLASS */
    PYTHON_CLASSMETHOD, /* Python classmethod(func) object */
    NON_DESCRIPTOR, /* Is not a descriptor, and is an instance of an immutable class */
    MUTABLE,   /* Instance of a mutable class; might, or might not, be a descriptor */
    ABSENT, /* Attribute is not present on the class */
    DUNDER_CLASS, /* __class__ attribute */
    GETSET_OVERRIDDEN, /* __getattribute__ or __setattr__ has been overridden */
    GETATTRIBUTE_IS_PYTHON_FUNCTION  /* Descriptor requires calling a Python __getattribute__ */
} DescriptorClassification;


static DescriptorClassification
classify_descriptor(PyObject *descriptor, bool has_getattr)
{
    if (descriptor == NULL) {
        return ABSENT;
    }
    PyTypeObject *desc_cls = Py_TYPE(descriptor);
    if (!(desc_cls->tp_flags & Py_TPFLAGS_IMMUTABLETYPE)) {
        return MUTABLE;
    }
    if (desc_cls->tp_descr_set) {
        if (desc_cls == &PyMemberDescr_Type) {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descriptor;
            struct PyMemberDef *dmem = member->d_member;
            if (dmem->type == Py_T_OBJECT_EX || dmem->type == _Py_T_OBJECT) {
                return OBJECT_SLOT;
            }
            return OTHER_SLOT;
        }
        if (desc_cls == &PyProperty_Type) {
            /* We can't detect at runtime whether an attribute exists
               with property. So that means we may have to call
               __getattr__. */
            return has_getattr ? GETSET_OVERRIDDEN : PROPERTY;
        }
        return OVERRIDING;
    }
    if (desc_cls->tp_descr_get) {
        if (desc_cls->tp_flags & Py_TPFLAGS_METHOD_DESCRIPTOR) {
            return METHOD;
        }
        if (Py_IS_TYPE(descriptor, &PyClassMethodDescr_Type)) {
            return BUILTIN_CLASSMETHOD;
        }
        if (Py_IS_TYPE(descriptor, &PyClassMethod_Type)) {
            return PYTHON_CLASSMETHOD;
        }
        return NON_OVERRIDING;
    }
    return NON_DESCRIPTOR;
}

static bool
descriptor_is_class(PyObject *descriptor, PyObject *name)
{
    return ((PyUnicode_CompareWithASCIIString(name, "__class__") == 0) &&
            (descriptor == _PyType_Lookup(&PyBaseObject_Type, name)));
}

static DescriptorClassification
analyze_descriptor_load(PyTypeObject *type, PyObject *name, PyObject **descr, unsigned int *tp_version) {
    bool has_getattr = false;
    bool have_ga_version = false;
    unsigned int ga_version;
    getattrofunc getattro_slot = type->tp_getattro;
    if (getattro_slot == PyObject_GenericGetAttr) {
        /* Normal attribute lookup; */
        has_getattr = false;
    }
    else if (getattro_slot == _Py_slot_tp_getattr_hook ||
        getattro_slot == _Py_slot_tp_getattro) {
        /* One or both of __getattribute__ or __getattr__ may have been
         overridden See typeobject.c for why these functions are special. */
        PyObject *getattribute = _PyType_LookupRefAndVersion(type,
                &_Py_ID(__getattribute__), &ga_version);
        have_ga_version = true;
        PyInterpreterState *interp = _PyInterpreterState_GET();
        bool has_custom_getattribute = getattribute != NULL &&
            getattribute != interp->callable_cache.object__getattribute__;
        PyObject *getattr = _PyType_Lookup(type, &_Py_ID(__getattr__));
        has_getattr = getattr != NULL;
        if (has_custom_getattribute) {
            if (getattro_slot == _Py_slot_tp_getattro &&
                !has_getattr &&
                Py_IS_TYPE(getattribute, &PyFunction_Type)) {
                *descr = getattribute;
                *tp_version = ga_version;
                return GETATTRIBUTE_IS_PYTHON_FUNCTION;
            }
            /* Potentially both __getattr__ and __getattribute__ are set.
               Too complicated */
            Py_DECREF(getattribute);
            *descr = NULL;
            *tp_version = ga_version;
            return GETSET_OVERRIDDEN;
        }
        /* Potentially has __getattr__ but no custom __getattribute__.
           Fall through to usual descriptor analysis.
           Usual attribute lookup should only be allowed at runtime
           if we can guarantee that there is no way an exception can be
           raised. This means some specializations, e.g. specializing
           for property() isn't safe.
        */
        Py_XDECREF(getattribute);
    }
    else {
        *descr = NULL;
        *tp_version = FT_ATOMIC_LOAD_UINT_RELAXED(type->tp_version_tag);
        return GETSET_OVERRIDDEN;
    }
    unsigned int descr_version;
    PyObject *descriptor = _PyType_LookupRefAndVersion(type, name, &descr_version);
    *descr = descriptor;
    *tp_version = have_ga_version ? ga_version : descr_version;
    if (descriptor_is_class(descriptor, name)) {
        return DUNDER_CLASS;
    }
    return classify_descriptor(descriptor, has_getattr);
}

static DescriptorClassification
analyze_descriptor_store(PyTypeObject *type, PyObject *name, PyObject **descr, unsigned int *tp_version)
{
    if (type->tp_setattro != PyObject_GenericSetAttr) {
        *descr = NULL;
        return GETSET_OVERRIDDEN;
    }
    PyObject *descriptor = _PyType_LookupRefAndVersion(type, name, tp_version);
    *descr = descriptor;
    if (descriptor_is_class(descriptor, name)) {
        return DUNDER_CLASS;
    }
    return classify_descriptor(descriptor, false);
}

static int
specialize_dict_access_inline(
    PyObject *owner, _Py_CODEUNIT *instr, PyTypeObject *type,
    PyObject *name, unsigned int tp_version,
    int base_op, int values_op)
{
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
    assert(PyUnicode_CheckExact(name));
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(owner);
    Py_ssize_t index = _PyDictKeys_StringLookupSplit(keys, name);
    assert (index != DKIX_ERROR);
    if (index == DKIX_EMPTY) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NOT_IN_KEYS);
        return 0;
    }
    assert(index >= 0);
    assert(_PyObject_InlineValues(owner)->valid);
    char *value_addr = (char *)&_PyObject_InlineValues(owner)->values[index];
    Py_ssize_t offset = value_addr - (char *)owner;
    if (offset != (uint16_t)offset) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_OUT_OF_RANGE);
        return 0;
    }
    cache->index = (uint16_t)offset;
    write_u32(cache->version, tp_version);
    specialize(instr, values_op);
    return 1;
}

static int
specialize_dict_access_hint(
    PyDictObject *dict, _Py_CODEUNIT *instr, PyTypeObject *type,
    PyObject *name, unsigned int tp_version,
    int base_op, int hint_op)
{
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(dict);
#ifdef Py_GIL_DISABLED
    _PyDict_EnsureSharedOnRead(dict);
#endif

    // We found an instance with a __dict__.
    if (_PyDict_HasSplitTable(dict)) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_SPLIT_DICT);
        return 0;
    }
    Py_ssize_t index = _PyDict_LookupIndex(dict, name);
    if (index != (uint16_t)index) {
        SPECIALIZATION_FAIL(base_op,
                            index == DKIX_EMPTY ?
                            SPEC_FAIL_ATTR_NOT_IN_DICT :
                            SPEC_FAIL_OUT_OF_RANGE);
        return 0;
    }
    cache->index = (uint16_t)index;
    write_u32(cache->version, tp_version);
    specialize(instr, hint_op);
    return 1;
}


static int
specialize_dict_access(
    PyObject *owner, _Py_CODEUNIT *instr, PyTypeObject *type,
    DescriptorClassification kind, PyObject *name, unsigned int tp_version,
    int base_op, int values_op, int hint_op)
{
    assert(kind == NON_OVERRIDING || kind == NON_DESCRIPTOR || kind == ABSENT ||
        kind == BUILTIN_CLASSMETHOD || kind == PYTHON_CLASSMETHOD ||
        kind == METHOD);
    // No descriptor, or non overriding.
    if ((type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
        return 0;
    }
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES &&
        FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner)->valid) &&
        !(base_op == STORE_ATTR && _PyObject_GetManagedDict(owner) != NULL))
    {
        int res;
        Py_BEGIN_CRITICAL_SECTION(owner);
        PyDictObject *dict = _PyObject_GetManagedDict(owner);
        if (dict == NULL) {
            // managed dict, not materialized, inline values valid
            res = specialize_dict_access_inline(owner, instr, type, name,
                                                tp_version, base_op, values_op);
        }
        else {
            // lost race and dict was created, fail specialization
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OTHER);
            res = 0;
        }
        Py_END_CRITICAL_SECTION();
        return res;
    }
    else {
        PyDictObject *dict = _PyObject_GetManagedDict(owner);
        if (dict == NULL || !PyDict_CheckExact(dict)) {
            SPECIALIZATION_FAIL(base_op, SPEC_FAIL_NO_DICT);
            return 0;
        }
        int res;
        Py_BEGIN_CRITICAL_SECTION(dict);
        // materialized managed dict
        res = specialize_dict_access_hint(dict, instr, type, name,
                                          tp_version, base_op, hint_op);
        Py_END_CRITICAL_SECTION();
        return res;
    }
}

static int
specialize_attr_loadclassattr(PyObject *owner, _Py_CODEUNIT *instr,
                              PyObject *name, PyObject *descr,
                              unsigned int tp_version,
                              DescriptorClassification kind, bool is_method,
                              uint32_t shared_keys_version);
static int specialize_class_load_attr(PyObject* owner, _Py_CODEUNIT* instr, PyObject* name);

/* Returns true if instances of obj's class are
 * likely to have `name` in their __dict__.
 * For objects with inline values, we check in the shared keys.
 * For other objects, we check their actual dictionary.
 */
static bool
instance_has_key(PyObject *obj, PyObject *name, uint32_t *shared_keys_version)
{
    PyTypeObject *cls = Py_TYPE(obj);
    if ((cls->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return false;
    }
    if (cls->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictKeysObject *keys = ((PyHeapTypeObject *)cls)->ht_cached_keys;
        Py_ssize_t index =
            _PyDictKeys_StringLookupAndVersion(keys, name, shared_keys_version);
        return index >= 0;
    }
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL || !PyDict_CheckExact(dict)) {
        return false;
    }
    bool result;
    Py_BEGIN_CRITICAL_SECTION(dict);
    if (dict->ma_values) {
        result = false;
    }
    else {
        result = (_PyDict_LookupIndex(dict, name) >= 0);
    }
    Py_END_CRITICAL_SECTION();
    return result;
}

static int
do_specialize_instance_load_attr(PyObject* owner, _Py_CODEUNIT* instr, PyObject* name,
                                 bool shadow, uint32_t shared_keys_version,
                                 DescriptorClassification kind, PyObject *descr, unsigned int tp_version)
{
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyTypeObject *type = Py_TYPE(owner);
    if (tp_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    uint8_t oparg = FT_ATOMIC_LOAD_UINT8_RELAXED(instr->op.arg);
    switch(kind) {
        case OVERRIDING:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            return -1;
        case METHOD:
        {
            if (shadow) {
                goto try_instance;
            }
            if (oparg & 1) {
                if (specialize_attr_loadclassattr(owner, instr, name, descr,
                                                  tp_version, kind, true,
                                                  shared_keys_version)) {
                    return 0;
                }
                else {
                    return -1;
                }
            }
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
            return -1;
        }
        case PROPERTY:
        {
            _PyLoadMethodCache *lm_cache = (_PyLoadMethodCache *)(instr + 1);
            assert(Py_TYPE(descr) == &PyProperty_Type);
            PyObject *fget = ((_PyPropertyObject *)descr)->prop_get;
            if (fget == NULL) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                return -1;
            }
            if (!Py_IS_TYPE(fget, &PyFunction_Type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_PROPERTY_NOT_PY_FUNCTION);
                return -1;
            }
            if (!function_check_args(fget, 1, LOAD_ATTR)) {
                return -1;
            }
            if (oparg & 1) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
                return -1;
            }
            /* Don't specialize if PEP 523 is active */
            if (_PyInterpreterState_GET()->eval_frame) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
                return -1;
            }
            #ifdef Py_GIL_DISABLED
            if (!_PyObject_HasDeferredRefcount(fget)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_DESCR_NOT_DEFERRED);
                return -1;
            }
            #endif
            assert(tp_version != 0);
            write_u32(lm_cache->type_version, tp_version);
            /* borrowed */
            write_ptr(lm_cache->descr, fget);
            specialize(instr, LOAD_ATTR_PROPERTY);
            return 0;
        }
        case OBJECT_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (!PyObject_TypeCheck(owner, member->d_common.d_type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                return -1;
            }
            if (dmem->flags & Py_AUDIT_READ) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_AUDITED_SLOT);
                return -1;
            }
            if (offset != (uint16_t)offset) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                return -1;
            }
            assert(dmem->type == Py_T_OBJECT_EX || dmem->type == _Py_T_OBJECT);
            assert(offset > 0);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, tp_version);
            specialize(instr, LOAD_ATTR_SLOT);
            return 0;
        }
        case DUNDER_CLASS:
        {
            Py_ssize_t offset = offsetof(PyObject, ob_type);
            assert(offset == (uint16_t)offset);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, tp_version);
            specialize(instr, LOAD_ATTR_SLOT);
            return 0;
        }
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_OBJECT_SLOT);
            return -1;
        case MUTABLE:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            return -1;
        case GETSET_OVERRIDDEN:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
            return -1;
        case GETATTRIBUTE_IS_PYTHON_FUNCTION:
        {
            #ifndef Py_GIL_DISABLED
            // In free-threaded builds it's possible for tp_getattro to change
            // after the call to analyze_descriptor. That is fine: the version
            // guard will fail.
            assert(type->tp_getattro == _Py_slot_tp_getattro);
            #endif
            assert(Py_IS_TYPE(descr, &PyFunction_Type));
            _PyLoadMethodCache *lm_cache = (_PyLoadMethodCache *)(instr + 1);
            if (!function_check_args(descr, 2, LOAD_ATTR)) {
                return -1;
            }
            if (oparg & 1) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
                return -1;
            }
            uint32_t version = function_get_version(descr, LOAD_ATTR);
            if (version == 0) {
                return -1;
            }
            /* Don't specialize if PEP 523 is active */
            if (_PyInterpreterState_GET()->eval_frame) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
                return -1;
            }
            #ifdef Py_GIL_DISABLED
            if (!_PyObject_HasDeferredRefcount(descr)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_DESCR_NOT_DEFERRED);
                return -1;
            }
            #endif
            write_u32(lm_cache->keys_version, version);
            /* borrowed */
            write_ptr(lm_cache->descr, descr);
            write_u32(lm_cache->type_version, tp_version);
            specialize(instr, LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN);
            return 0;
        }
        case BUILTIN_CLASSMETHOD:
        case PYTHON_CLASSMETHOD:
        case NON_OVERRIDING:
            if (shadow) {
                goto try_instance;
            }
            return -1;
        case NON_DESCRIPTOR:
            if (shadow) {
                goto try_instance;
            }
            if ((oparg & 1) == 0) {
                if (specialize_attr_loadclassattr(owner, instr, name, descr,
                                                  tp_version, kind, false,
                                                  shared_keys_version)) {
                    return 0;
                }
            }
            return -1;
        case ABSENT:
            if (shadow) {
                goto try_instance;
            }
            set_counter((_Py_BackoffCounter*)instr + 1, adaptive_counter_cooldown());
            return 0;
    }
    Py_UNREACHABLE();
try_instance:
    if (specialize_dict_access(owner, instr, type, kind, name, tp_version,
                               LOAD_ATTR, LOAD_ATTR_INSTANCE_VALUE, LOAD_ATTR_WITH_HINT))
    {
        return 0;
    }
    return -1;
}

static int
specialize_instance_load_attr(PyObject* owner, _Py_CODEUNIT* instr, PyObject* name)
{
    // 0 is not a valid version
    uint32_t shared_keys_version = 0;
    bool shadow = instance_has_key(owner, name, &shared_keys_version);
    PyObject *descr = NULL;
    unsigned int tp_version = 0;
    PyTypeObject *type = Py_TYPE(owner);
    DescriptorClassification kind = analyze_descriptor_load(type, name, &descr, &tp_version);
    int result = do_specialize_instance_load_attr(owner, instr, name, shadow, shared_keys_version, kind, descr, tp_version);
    Py_XDECREF(descr);
    return result;
}

Py_NO_INLINE void
_Py_Specialize_LoadAttr(_PyStackRef owner_st, _Py_CODEUNIT *instr, PyObject *name)
{
    PyObject *owner = PyStackRef_AsPyObjectBorrow(owner_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[LOAD_ATTR] == INLINE_CACHE_ENTRIES_LOAD_ATTR);
    PyTypeObject *type = Py_TYPE(owner);
    bool fail;
    if (!_PyType_IsReady(type)) {
        // We *might* not really need this check, but we inherited it from
        // PyObject_GenericGetAttr and friends... and this way we still do the
        // right thing if someone forgets to call PyType_Ready(type):
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
        fail = true;
    }
    else if (Py_TYPE(owner)->tp_getattro == PyModule_Type.tp_getattro) {
        fail = specialize_module_load_attr(owner, instr, name);
    }
    else if (PyType_Check(owner)) {
        fail = specialize_class_load_attr(owner, instr, name);
    }
    else {
        fail = specialize_instance_load_attr(owner, instr, name);
    }

    if (fail) {
        unspecialize(instr);
    }
}

Py_NO_INLINE void
_Py_Specialize_StoreAttr(_PyStackRef owner_st, _Py_CODEUNIT *instr, PyObject *name)
{
    PyObject *owner = PyStackRef_AsPyObjectBorrow(owner_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[STORE_ATTR] == INLINE_CACHE_ENTRIES_STORE_ATTR);
    PyObject *descr = NULL;
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyTypeObject *type = Py_TYPE(owner);
    if (!_PyType_IsReady(type)) {
        // We *might* not really need this check, but we inherited it from
        // PyObject_GenericSetAttr and friends... and this way we still do the
        // right thing if someone forgets to call PyType_Ready(type):
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OTHER);
        goto fail;
    }
    if (PyModule_CheckExact(owner)) {
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OVERRIDDEN);
        goto fail;
    }
    unsigned int tp_version = 0;
    DescriptorClassification kind = analyze_descriptor_store(type, name, &descr, &tp_version);
    if (tp_version == 0) {
        goto fail;
    }
    assert(descr != NULL || kind == ABSENT || kind == GETSET_OVERRIDDEN);
    switch(kind) {
        case OVERRIDING:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            goto fail;
        case METHOD:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_METHOD);
            goto fail;
        case PROPERTY:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_PROPERTY);
            goto fail;
        case OBJECT_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (!PyObject_TypeCheck(owner, member->d_common.d_type)) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                goto fail;
            }
            if (dmem->flags & Py_READONLY) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_READ_ONLY);
                goto fail;
            }
            if (offset != (uint16_t)offset) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                goto fail;
            }
            assert(dmem->type == Py_T_OBJECT_EX || dmem->type == _Py_T_OBJECT);
            assert(offset > 0);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, tp_version);
            specialize(instr, STORE_ATTR_SLOT);
            goto success;
        }
        case DUNDER_CLASS:
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_NON_OBJECT_SLOT);
            goto fail;
        case MUTABLE:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            goto fail;
        case GETATTRIBUTE_IS_PYTHON_FUNCTION:
        case GETSET_OVERRIDDEN:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OVERRIDDEN);
            goto fail;
        case BUILTIN_CLASSMETHOD:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD_OBJ);
            goto fail;
        case PYTHON_CLASSMETHOD:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_CLASS_METHOD_OBJ);
            goto fail;
        case NON_OVERRIDING:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_CLASS_ATTR_DESCRIPTOR);
            goto fail;
        case NON_DESCRIPTOR:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE);
            goto fail;
        case ABSENT:
            if (specialize_dict_access(owner, instr, type, kind, name, tp_version,
                                       STORE_ATTR, STORE_ATTR_INSTANCE_VALUE,
                                       STORE_ATTR_WITH_HINT)) {
                goto success;
            }
    }
fail:
    Py_XDECREF(descr);
    unspecialize(instr);
    return;
success:
    Py_XDECREF(descr);
    return;
}

#ifdef Py_STATS
static int
load_attr_fail_kind(DescriptorClassification kind)
{
    switch (kind) {
        case OVERRIDING:
            return SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR;
        case METHOD:
            return SPEC_FAIL_ATTR_METHOD;
        case PROPERTY:
            return SPEC_FAIL_ATTR_PROPERTY;
        case OBJECT_SLOT:
            return SPEC_FAIL_ATTR_OBJECT_SLOT;
        case OTHER_SLOT:
            return SPEC_FAIL_ATTR_NON_OBJECT_SLOT;
        case DUNDER_CLASS:
            return SPEC_FAIL_OTHER;
        case MUTABLE:
            return SPEC_FAIL_ATTR_MUTABLE_CLASS;
        case GETSET_OVERRIDDEN:
        case GETATTRIBUTE_IS_PYTHON_FUNCTION:
            return SPEC_FAIL_OVERRIDDEN;
        case BUILTIN_CLASSMETHOD:
            return SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD;
        case PYTHON_CLASSMETHOD:
            return SPEC_FAIL_ATTR_CLASS_METHOD_OBJ;
        case NON_OVERRIDING:
            return SPEC_FAIL_ATTR_NON_OVERRIDING_DESCRIPTOR;
        case NON_DESCRIPTOR:
            return SPEC_FAIL_ATTR_NOT_DESCRIPTOR;
        case ABSENT:
            return SPEC_FAIL_ATTR_INSTANCE_ATTRIBUTE;
    }
    Py_UNREACHABLE();
}
#endif   // Py_STATS

static int
specialize_class_load_attr(PyObject *owner, _Py_CODEUNIT *instr,
                             PyObject *name)
{
    assert(PyType_Check(owner));
    PyTypeObject *cls = (PyTypeObject *)owner;
    _PyLoadMethodCache *cache = (_PyLoadMethodCache *)(instr + 1);
    if (Py_TYPE(cls)->tp_getattro != _Py_type_getattro) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METACLASS_OVERRIDDEN);
        return -1;
    }
    unsigned int meta_version = 0;
    PyObject *metadescriptor = _PyType_LookupRefAndVersion(Py_TYPE(cls), name, &meta_version);
    DescriptorClassification metakind = classify_descriptor(metadescriptor, false);
    Py_XDECREF(metadescriptor);
    switch (metakind) {
        case METHOD:
        case NON_DESCRIPTOR:
        case NON_OVERRIDING:
        case BUILTIN_CLASSMETHOD:
        case PYTHON_CLASSMETHOD:
        case ABSENT:
            break;
        default:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METACLASS_ATTRIBUTE);
            return -1;
    }
    PyObject *descr = NULL;
    DescriptorClassification kind = 0;
    unsigned int tp_version = 0;
    kind = analyze_descriptor_load(cls, name, &descr, &tp_version);
    if (tp_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        Py_XDECREF(descr);
        return -1;
    }
    bool metaclass_check = false;
    if ((Py_TYPE(cls)->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0) {
        metaclass_check = true;
        if (meta_version == 0) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
            Py_XDECREF(descr);
            return -1;
        }
    }
    switch (kind) {
        case METHOD:
        case NON_DESCRIPTOR:
            #ifdef Py_GIL_DISABLED
            if (!_PyObject_HasDeferredRefcount(descr)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_DESCR_NOT_DEFERRED);
                Py_XDECREF(descr);
                return -1;
            }
            #endif
            write_u32(cache->type_version, tp_version);
            write_ptr(cache->descr, descr);
            if (metaclass_check) {
                write_u32(cache->keys_version, meta_version);
                specialize(instr, LOAD_ATTR_CLASS_WITH_METACLASS_CHECK);
            }
            else {
                specialize(instr, LOAD_ATTR_CLASS);
            }
            Py_XDECREF(descr);
            return 0;
#ifdef Py_STATS
        case ABSENT:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
            Py_XDECREF(descr);
            return -1;
#endif
        default:
            SPECIALIZATION_FAIL(LOAD_ATTR, load_attr_fail_kind(kind));
            Py_XDECREF(descr);
            return -1;
    }
}

// Please collect stats carefully before and after modifying. A subtle change
// can cause a significant drop in cache hits. A possible test is
// python.exe -m test_typing test_re test_dis test_zlib.
static int
specialize_attr_loadclassattr(PyObject *owner, _Py_CODEUNIT *instr,
                              PyObject *name, PyObject *descr,
                              unsigned int tp_version,
                              DescriptorClassification kind, bool is_method,
                              uint32_t shared_keys_version)
{
    _PyLoadMethodCache *cache = (_PyLoadMethodCache *)(instr + 1);
    PyTypeObject *owner_cls = Py_TYPE(owner);

    assert(descr != NULL);
    assert((is_method && kind == METHOD) || (!is_method && kind == NON_DESCRIPTOR));

    #ifdef Py_GIL_DISABLED
    if (!_PyObject_HasDeferredRefcount(descr)) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_DESCR_NOT_DEFERRED);
        return 0;
    }
    #endif

    unsigned long tp_flags = PyType_GetFlags(owner_cls);
    if (tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        #ifndef Py_GIL_DISABLED
        assert(_PyDictKeys_StringLookup(
                   ((PyHeapTypeObject *)owner_cls)->ht_cached_keys, name) < 0);
        #endif
        if (shared_keys_version == 0) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
            return 0;
        }
        write_u32(cache->keys_version, shared_keys_version);
        specialize(instr, is_method ? LOAD_ATTR_METHOD_WITH_VALUES : LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES);
    }
    else {
        Py_ssize_t dictoffset;
        if (tp_flags & Py_TPFLAGS_MANAGED_DICT) {
            dictoffset = MANAGED_DICT_OFFSET;
        }
        else {
            dictoffset = owner_cls->tp_dictoffset;
            if (dictoffset < 0 || dictoffset > INT16_MAX + MANAGED_DICT_OFFSET) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                return 0;
            }
        }
        if (dictoffset == 0) {
            specialize(instr, is_method ? LOAD_ATTR_METHOD_NO_DICT : LOAD_ATTR_NONDESCRIPTOR_NO_DICT);
        }
        else if (is_method) {
            PyObject **addr = (PyObject **)((char *)owner + dictoffset);
            PyObject *dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*addr);
            if (dict) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
                return 0;
            }
            /* Cache entries must be unsigned values, so we offset the
             * dictoffset by MANAGED_DICT_OFFSET.
             * We do the reverse offset in LOAD_ATTR_METHOD_LAZY_DICT */
            dictoffset -= MANAGED_DICT_OFFSET;
            assert(((uint16_t)dictoffset) == dictoffset);
            cache->dict_offset = (uint16_t)dictoffset;
            specialize(instr, LOAD_ATTR_METHOD_LAZY_DICT);
        }
        else {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE);
            return 0;
        }
    }
    /* `descr` is borrowed. This is safe for methods (even inherited ones from
    *  super classes!) as long as tp_version_tag is validated for two main reasons:
    *
    *  1. The class will always hold a reference to the method so it will
    *  usually not be GC-ed. Should it be deleted in Python, e.g.
    *  `del obj.meth`, tp_version_tag will be invalidated, because of reason 2.
    *
    *  2. The pre-existing type method cache (MCACHE) uses the same principles
    *  of caching a borrowed descriptor. The MCACHE infrastructure does all the
    *  heavy lifting for us. E.g. it invalidates tp_version_tag on any MRO
    *  modification, on any type object change along said MRO, etc. (see
    *  PyType_Modified usages in typeobject.c). The MCACHE has been
    *  working since Python 2.6 and it's battle-tested.
    */
    write_u32(cache->type_version, tp_version);
    write_ptr(cache->descr, descr);
    return 1;
}


static void
specialize_load_global_lock_held(
    PyObject *globals, PyObject *builtins,
    _Py_CODEUNIT *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[LOAD_GLOBAL] == INLINE_CACHE_ENTRIES_LOAD_GLOBAL);
    /* Use inline cache */
    _PyLoadGlobalCache *cache = (_PyLoadGlobalCache *)(instr + 1);
    assert(PyUnicode_CheckExact(name));
    if (!PyDict_CheckExact(globals)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_DICT);
        goto fail;
    }
    PyDictKeysObject * globals_keys = ((PyDictObject *)globals)->ma_keys;
    if (!DK_IS_UNICODE(globals_keys)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    Py_ssize_t index = _PyDictKeys_StringLookup(globals_keys, name);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto fail;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (index != DKIX_EMPTY) {
        if (index != (uint16_t)index) {
            SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_RANGE);
            goto fail;
        }
        uint32_t keys_version = _PyDict_GetKeysVersionForCurrentState(
                interp, (PyDictObject*) globals);
        if (keys_version == 0) {
            SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
            goto fail;
        }
        if (keys_version != (uint16_t)keys_version) {
            SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_RANGE);
            goto fail;
        }
        cache->index = (uint16_t)index;
        cache->module_keys_version = (uint16_t)keys_version;
        specialize(instr, LOAD_GLOBAL_MODULE);
        return;
    }
    if (!PyDict_CheckExact(builtins)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_DICT);
        goto fail;
    }
    PyDictKeysObject * builtin_keys = ((PyDictObject *)builtins)->ma_keys;
    if (!DK_IS_UNICODE(builtin_keys)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    index = _PyDictKeys_StringLookup(builtin_keys, name);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto fail;
    }
    if (index != (uint16_t)index) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_RANGE);
        goto fail;
    }
    uint32_t globals_version = _PyDict_GetKeysVersionForCurrentState(
            interp, (PyDictObject*) globals);
    if (globals_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    if (globals_version != (uint16_t)globals_version) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_RANGE);
        goto fail;
    }
    uint32_t builtins_version = _PyDict_GetKeysVersionForCurrentState(
            interp, (PyDictObject*) builtins);
    if (builtins_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    if (builtins_version > UINT16_MAX) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_RANGE);
        goto fail;
    }
    cache->index = (uint16_t)index;
    cache->module_keys_version = (uint16_t)globals_version;
    cache->builtin_keys_version = (uint16_t)builtins_version;
    specialize(instr, LOAD_GLOBAL_BUILTIN);
    return;
fail:
    unspecialize(instr);
}

Py_NO_INLINE void
_Py_Specialize_LoadGlobal(
    PyObject *globals, PyObject *builtins,
    _Py_CODEUNIT *instr, PyObject *name)
{
    Py_BEGIN_CRITICAL_SECTION2(globals, builtins);
    specialize_load_global_lock_held(globals, builtins, instr, name);
    Py_END_CRITICAL_SECTION2();
}

static int
function_kind(PyCodeObject *code) {
    int flags = code->co_flags;
    if ((flags & (CO_VARKEYWORDS | CO_VARARGS)) || code->co_kwonlyargcount) {
        return SPEC_FAIL_CODE_COMPLEX_PARAMETERS;
    }
    if ((flags & CO_OPTIMIZED) == 0) {
        return SPEC_FAIL_CODE_NOT_OPTIMIZED;
    }
    return SIMPLE_FUNCTION;
}

/* Returning false indicates a failure. */
static bool
function_check_args(PyObject *o, int expected_argcount, int opcode)
{
    assert(Py_IS_TYPE(o, &PyFunction_Type));
    PyFunctionObject *func = (PyFunctionObject *)o;
    PyCodeObject *fcode = (PyCodeObject *)func->func_code;
    int kind = function_kind(fcode);
    if (kind != SIMPLE_FUNCTION) {
        SPECIALIZATION_FAIL(opcode, kind);
        return false;
    }
    if (fcode->co_argcount != expected_argcount) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
        return false;
    }
    return true;
}

/* Returning 0 indicates a failure. */
static uint32_t
function_get_version(PyObject *o, int opcode)
{
    assert(Py_IS_TYPE(o, &PyFunction_Type));
    PyFunctionObject *func = (PyFunctionObject *)o;
    uint32_t version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_OUT_OF_VERSIONS);
        return 0;
    }
    return version;
}

#ifdef Py_STATS
static int
store_subscr_fail_kind(PyObject *container, PyObject *sub)
{
    PyTypeObject *container_type = Py_TYPE(container);
    PyMappingMethods *as_mapping = container_type->tp_as_mapping;
    if (as_mapping && (as_mapping->mp_ass_subscript
                       == PyDict_Type.tp_as_mapping->mp_ass_subscript)) {
        return SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE;
    }
    if (PyObject_CheckBuffer(container)) {
        if (PyLong_CheckExact(sub) && (!_PyLong_IsNonNegativeCompact((PyLongObject *)sub))) {
            return SPEC_FAIL_OUT_OF_RANGE;
        }
        else if (strcmp(container_type->tp_name, "array.array") == 0) {
            if (PyLong_CheckExact(sub)) {
                return SPEC_FAIL_SUBSCR_ARRAY_INT;
            }
            else if (PySlice_Check(sub)) {
                return SPEC_FAIL_SUBSCR_ARRAY_SLICE;
            }
            else {
                return SPEC_FAIL_OTHER;
            }
        }
        else if (PyByteArray_CheckExact(container)) {
            if (PyLong_CheckExact(sub)) {
                return SPEC_FAIL_SUBSCR_BYTEARRAY_INT;
            }
            else if (PySlice_Check(sub)) {
                return SPEC_FAIL_SUBSCR_BYTEARRAY_SLICE;
            }
            else {
                return SPEC_FAIL_OTHER;
            }
        }
        else {
            if (PyLong_CheckExact(sub)) {
                return SPEC_FAIL_SUBSCR_BUFFER_INT;
            }
            else if (PySlice_Check(sub)) {
                return SPEC_FAIL_SUBSCR_BUFFER_SLICE;
            }
            else {
                return SPEC_FAIL_OTHER;
            }
        }
        return SPEC_FAIL_OTHER;
    }
    PyObject *descriptor = _PyType_Lookup(container_type, &_Py_ID(__setitem__));
    if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
        PyFunctionObject *func = (PyFunctionObject *)descriptor;
        PyCodeObject *code = (PyCodeObject *)func->func_code;
        int kind = function_kind(code);
        if (kind == SIMPLE_FUNCTION) {
            return SPEC_FAIL_SUBSCR_PY_SIMPLE;
        }
        else {
            return SPEC_FAIL_SUBSCR_PY_OTHER;
        }
    }
    return SPEC_FAIL_OTHER;
}
#endif

Py_NO_INLINE void
_Py_Specialize_StoreSubscr(_PyStackRef container_st, _PyStackRef sub_st, _Py_CODEUNIT *instr)
{
    PyObject *container = PyStackRef_AsPyObjectBorrow(container_st);
    PyObject *sub = PyStackRef_AsPyObjectBorrow(sub_st);

    assert(ENABLE_SPECIALIZATION_FT);
    PyTypeObject *container_type = Py_TYPE(container);
    if (container_type == &PyList_Type) {
        if (PyLong_CheckExact(sub)) {
            if (_PyLong_IsNonNegativeCompact((PyLongObject *)sub)
                && ((PyLongObject *)sub)->long_value.ob_digit[0] < (size_t)PyList_GET_SIZE(container))
            {
                specialize(instr, STORE_SUBSCR_LIST_INT);
                return;
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
                unspecialize(instr);
                return;
            }
        }
        else if (PySlice_Check(sub)) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_LIST_SLICE);
            unspecialize(instr);
            return;
        }
        else {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            unspecialize(instr);
            return;
        }
    }
    if (container_type == &PyDict_Type) {
        specialize(instr, STORE_SUBSCR_DICT);
        return;
    }
    SPECIALIZATION_FAIL(STORE_SUBSCR, store_subscr_fail_kind(container, sub));
    unspecialize(instr);
}

/* Returns a strong reference. */
static PyObject *
get_init_for_simple_managed_python_class(PyTypeObject *tp, unsigned int *tp_version)
{
    assert(tp->tp_new == PyBaseObject_Type.tp_new);
    if (tp->tp_alloc != PyType_GenericAlloc) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OVERRIDDEN);
        return NULL;
    }
    unsigned long tp_flags = PyType_GetFlags(tp);
    if (!(tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        /* Is this possible? */
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_EXPECTED_ERROR);
        return NULL;
    }
    PyObject *init = _PyType_LookupRefAndVersion(tp, &_Py_ID(__init__), tp_version);
    if (init == NULL || !PyFunction_Check(init)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_PYTHON);
        Py_XDECREF(init);
        return NULL;
    }
    int kind = function_kind((PyCodeObject *)PyFunction_GET_CODE(init));
    if (kind != SIMPLE_FUNCTION) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_SIMPLE);
        Py_DECREF(init);
        return NULL;
    }
    return init;
}

static int
specialize_class_call(PyObject *callable, _Py_CODEUNIT *instr, int nargs)
{
    assert(PyType_Check(callable));
    PyTypeObject *tp = _PyType_CAST(callable);
    if (tp->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) {
        int oparg = instr->op.arg;
        if (nargs == 1 && oparg == 1) {
            if (tp == &PyUnicode_Type) {
                specialize(instr, CALL_STR_1);
                return 0;
            }
            else if (tp == &PyType_Type) {
                specialize(instr, CALL_TYPE_1);
                return 0;
            }
            else if (tp == &PyTuple_Type) {
                specialize(instr, CALL_TUPLE_1);
                return 0;
            }
        }
        if (tp->tp_vectorcall != NULL) {
            specialize(instr, CALL_BUILTIN_CLASS);
            return 0;
        }
        goto generic;
    }
    if (Py_TYPE(tp) != &PyType_Type) {
        goto generic;
    }
    if (tp->tp_new == PyBaseObject_Type.tp_new) {
        unsigned int tp_version = 0;
        PyObject *init = get_init_for_simple_managed_python_class(tp, &tp_version);
        if (!tp_version) {
            SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
            Py_XDECREF(init);
            return -1;
        }
        if (init != NULL && _PyType_CacheInitForSpecialization(
                                (PyHeapTypeObject *)tp, init, tp_version)) {
            _PyCallCache *cache = (_PyCallCache *)(instr + 1);
            write_u32(cache->func_version, tp_version);
            specialize(instr, CALL_ALLOC_AND_ENTER_INIT);
            Py_DECREF(init);
            return 0;
        }
        Py_XDECREF(init);
    }
generic:
    specialize(instr, CALL_NON_PY_GENERAL);
    return 0;
}

static int
specialize_method_descriptor(PyMethodDescrObject *descr, _Py_CODEUNIT *instr,
                             int nargs)
{
    switch (descr->d_method->ml_flags &
        (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O |
        METH_KEYWORDS | METH_METHOD)) {
        case METH_NOARGS: {
            if (nargs != 1) {
                SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return -1;
            }
            specialize(instr, CALL_METHOD_DESCRIPTOR_NOARGS);
            return 0;
        }
        case METH_O: {
            if (nargs != 2) {
                SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return -1;
            }
            PyInterpreterState *interp = _PyInterpreterState_GET();
            PyObject *list_append = interp->callable_cache.list_append;
            _Py_CODEUNIT next = instr[INLINE_CACHE_ENTRIES_CALL + 1];
            bool pop = (next.op.code == POP_TOP);
            int oparg = instr->op.arg;
            if ((PyObject *)descr == list_append && oparg == 1 && pop) {
                specialize(instr, CALL_LIST_APPEND);
                return 0;
            }
            specialize(instr, CALL_METHOD_DESCRIPTOR_O);
            return 0;
        }
        case METH_FASTCALL: {
            specialize(instr, CALL_METHOD_DESCRIPTOR_FAST);
            return 0;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            specialize(instr, CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS);
            return 0;
        }
    }
    specialize(instr, CALL_NON_PY_GENERAL);
    return 0;
}

static int
specialize_py_call(PyFunctionObject *func, _Py_CODEUNIT *instr, int nargs,
                   bool bound_method)
{
    _PyCallCache *cache = (_PyCallCache *)(instr + 1);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int kind = function_kind(code);
    /* Don't specialize if PEP 523 is active */
    if (_PyInterpreterState_GET()->eval_frame) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_PEP_523);
        return -1;
    }
    int argcount = -1;
    if (kind == SPEC_FAIL_CODE_NOT_OPTIMIZED) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CODE_NOT_OPTIMIZED);
        return -1;
    }
    if (kind == SIMPLE_FUNCTION) {
        argcount = code->co_argcount;
    }
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    write_u32(cache->func_version, version);
    uint8_t opcode;
    if (argcount == nargs + bound_method) {
        opcode =
            bound_method ? CALL_BOUND_METHOD_EXACT_ARGS : CALL_PY_EXACT_ARGS;
    }
    else {
        opcode = bound_method ? CALL_BOUND_METHOD_GENERAL : CALL_PY_GENERAL;
    }
    specialize(instr, opcode);
    return 0;
}


static int
specialize_py_call_kw(PyFunctionObject *func, _Py_CODEUNIT *instr, int nargs,
                   bool bound_method)
{
    _PyCallCache *cache = (_PyCallCache *)(instr + 1);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int kind = function_kind(code);
    /* Don't specialize if PEP 523 is active */
    if (_PyInterpreterState_GET()->eval_frame) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_PEP_523);
        return -1;
    }
    if (kind == SPEC_FAIL_CODE_NOT_OPTIMIZED) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CODE_NOT_OPTIMIZED);
        return -1;
    }
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    write_u32(cache->func_version, version);
    specialize(instr, bound_method ? CALL_KW_BOUND_METHOD : CALL_KW_PY);
    return 0;
}

static int
specialize_c_call(PyObject *callable, _Py_CODEUNIT *instr, int nargs)
{
    if (PyCFunction_GET_FUNCTION(callable) == NULL) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OTHER);
        return 1;
    }
    switch (PyCFunction_GET_FLAGS(callable) &
        (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O |
        METH_KEYWORDS | METH_METHOD)) {
        case METH_O: {
            if (nargs != 1) {
                SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return 1;
            }
            /* len(o) */
            PyInterpreterState *interp = _PyInterpreterState_GET();
            if (callable == interp->callable_cache.len && instr->op.arg == 1) {
                specialize(instr, CALL_LEN);
                return 0;
            }
            specialize(instr, CALL_BUILTIN_O);
            return 0;
        }
        case METH_FASTCALL: {
            if (nargs == 2) {
                /* isinstance(o1, o2) */
                PyInterpreterState *interp = _PyInterpreterState_GET();
                if (callable == interp->callable_cache.isinstance && instr->op.arg == 2) {
                    specialize(instr, CALL_ISINSTANCE);
                    return 0;
                }
            }
            specialize(instr, CALL_BUILTIN_FAST);
            return 0;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            specialize(instr, CALL_BUILTIN_FAST_WITH_KEYWORDS);
            return 0;
        }
        default:
            specialize(instr, CALL_NON_PY_GENERAL);
            return 0;
    }
}

Py_NO_INLINE void
_Py_Specialize_Call(_PyStackRef callable_st, _Py_CODEUNIT *instr, int nargs)
{
    PyObject *callable = PyStackRef_AsPyObjectBorrow(callable_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[CALL] == INLINE_CACHE_ENTRIES_CALL);
    assert(_Py_OPCODE(*instr) != INSTRUMENTED_CALL);
    int fail;
    if (PyCFunction_CheckExact(callable)) {
        fail = specialize_c_call(callable, instr, nargs);
    }
    else if (PyFunction_Check(callable)) {
        fail = specialize_py_call((PyFunctionObject *)callable, instr, nargs, false);
    }
    else if (PyType_Check(callable)) {
        fail = specialize_class_call(callable, instr, nargs);
    }
    else if (Py_IS_TYPE(callable, &PyMethodDescr_Type)) {
        fail = specialize_method_descriptor((PyMethodDescrObject *)callable, instr, nargs);
    }
    else if (PyMethod_Check(callable)) {
        PyObject *func = ((PyMethodObject *)callable)->im_func;
        if (PyFunction_Check(func)) {
            fail = specialize_py_call((PyFunctionObject *)func, instr, nargs, true);
        }
        else {
            SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_BOUND_METHOD);
            fail = -1;
        }
    }
    else {
        specialize(instr, CALL_NON_PY_GENERAL);
        fail = 0;
    }
    if (fail) {
        unspecialize(instr);
    }
}

Py_NO_INLINE void
_Py_Specialize_CallKw(_PyStackRef callable_st, _Py_CODEUNIT *instr, int nargs)
{
    PyObject *callable = PyStackRef_AsPyObjectBorrow(callable_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[CALL_KW] == INLINE_CACHE_ENTRIES_CALL_KW);
    assert(_Py_OPCODE(*instr) != INSTRUMENTED_CALL_KW);
    int fail;
    if (PyFunction_Check(callable)) {
        fail = specialize_py_call_kw((PyFunctionObject *)callable, instr, nargs, false);
    }
    else if (PyMethod_Check(callable)) {
        PyObject *func = ((PyMethodObject *)callable)->im_func;
        if (PyFunction_Check(func)) {
            fail = specialize_py_call_kw((PyFunctionObject *)func, instr, nargs, true);
        }
        else {
            SPECIALIZATION_FAIL(CALL_KW, SPEC_FAIL_CALL_BOUND_METHOD);
            fail = -1;
        }
    }
    else {
        specialize(instr, CALL_KW_NON_PY);
        fail = 0;
    }
    if (fail) {
        unspecialize(instr);
    }
}

#ifdef Py_STATS
static int
binary_op_fail_kind(int oparg, PyObject *lhs, PyObject *rhs)
{
    switch (oparg) {
        case NB_ADD:
        case NB_INPLACE_ADD:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_ADD_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_ADD_OTHER;
        case NB_AND:
        case NB_INPLACE_AND:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_AND_DIFFERENT_TYPES;
            }
            if (PyLong_CheckExact(lhs)) {
                return SPEC_FAIL_BINARY_OP_AND_INT;
            }
            return SPEC_FAIL_BINARY_OP_AND_OTHER;
        case NB_FLOOR_DIVIDE:
        case NB_INPLACE_FLOOR_DIVIDE:
            return SPEC_FAIL_BINARY_OP_FLOOR_DIVIDE;
        case NB_LSHIFT:
        case NB_INPLACE_LSHIFT:
            return SPEC_FAIL_BINARY_OP_LSHIFT;
        case NB_MATRIX_MULTIPLY:
        case NB_INPLACE_MATRIX_MULTIPLY:
            return SPEC_FAIL_BINARY_OP_MATRIX_MULTIPLY;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_MULTIPLY_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_MULTIPLY_OTHER;
        case NB_OR:
        case NB_INPLACE_OR:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_OR_DIFFERENT_TYPES;
            }
            if (PyLong_CheckExact(lhs)) {
                return SPEC_FAIL_BINARY_OP_OR_INT;
            }
            return SPEC_FAIL_BINARY_OP_OR;
        case NB_POWER:
        case NB_INPLACE_POWER:
            return SPEC_FAIL_BINARY_OP_POWER;
        case NB_REMAINDER:
        case NB_INPLACE_REMAINDER:
            return SPEC_FAIL_BINARY_OP_REMAINDER;
        case NB_RSHIFT:
        case NB_INPLACE_RSHIFT:
            return SPEC_FAIL_BINARY_OP_RSHIFT;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_SUBTRACT_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_SUBTRACT_OTHER;
        case NB_TRUE_DIVIDE:
        case NB_INPLACE_TRUE_DIVIDE:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_DIFFERENT_TYPES;
            }
            if (PyFloat_CheckExact(lhs)) {
                return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_FLOAT;
            }
            return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_OTHER;
        case NB_XOR:
        case NB_INPLACE_XOR:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                return SPEC_FAIL_BINARY_OP_XOR_DIFFERENT_TYPES;
            }
            if (PyLong_CheckExact(lhs)) {
                return SPEC_FAIL_BINARY_OP_XOR_INT;
            }
            return SPEC_FAIL_BINARY_OP_XOR;
        case NB_SUBSCR:
            if (PyList_CheckExact(lhs)) {
                if (PyLong_CheckExact(rhs) && !_PyLong_IsNonNegativeCompact((PyLongObject *)rhs)) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (PySlice_Check(rhs)) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_LIST_SLICE;
                }
            }
            if (PyTuple_CheckExact(lhs)) {
                if (PyLong_CheckExact(rhs) && !_PyLong_IsNonNegativeCompact((PyLongObject *)rhs)) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (PySlice_Check(rhs)) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_TUPLE_SLICE;
                }
            }
            if (PyUnicode_CheckExact(lhs)) {
                if (PyLong_CheckExact(rhs) && !_PyLong_IsNonNegativeCompact((PyLongObject *)rhs)) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (PySlice_Check(rhs)) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_STRING_SLICE;
                }
            }
            unsigned int tp_version;
            PyTypeObject *container_type = Py_TYPE(lhs);
            PyObject *descriptor = _PyType_LookupRefAndVersion(container_type, &_Py_ID(__getitem__), &tp_version);
            if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
                if (!(container_type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
                    Py_DECREF(descriptor);
                    return SPEC_FAIL_BINARY_OP_SUBSCR_NOT_HEAP_TYPE;
                }
                PyFunctionObject *func = (PyFunctionObject *)descriptor;
                PyCodeObject *fcode = (PyCodeObject *)func->func_code;
                int kind = function_kind(fcode);
                if (kind != SIMPLE_FUNCTION) {
                    Py_DECREF(descriptor);
                    return kind;
                }
                if (fcode->co_argcount != 2) {
                    Py_DECREF(descriptor);
                    return SPEC_FAIL_WRONG_NUMBER_ARGUMENTS;
                }

                if (_PyInterpreterState_GET()->eval_frame) {
                    /* Don't specialize if PEP 523 is active */
                    Py_DECREF(descriptor);
                    return SPEC_FAIL_OTHER;
                }
            }
            Py_XDECREF(descriptor);

            if (PyObject_TypeCheck(lhs, &PyDictProxy_Type)) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_MAPPINGPROXY;
            }

            if (PyObject_TypeCheck(lhs, &PyBytes_Type)) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_BYTES;
            }

            if (PyObject_TypeCheck(lhs, &PyRange_Type)) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_RANGE;
            }

            if (strcmp(container_type->tp_name, "array.array") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_ARRAY;
            }

            if (strcmp(container_type->tp_name, "re.Match") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_RE_MATCH;
            }

            if (strcmp(container_type->tp_name, "collections.deque") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_DEQUE;
            }

            if (strcmp(_PyType_Name(container_type), "EnumDict") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_ENUMDICT;
            }

            if (strcmp(container_type->tp_name, "StackSummary") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_STACKSUMMARY;
            }

            if (strcmp(container_type->tp_name, "collections.defaultdict") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_DEFAULTDICT;
            }

            if (strcmp(container_type->tp_name, "Counter") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_COUNTER;
            }

            if (strcmp(container_type->tp_name, "collections.OrderedDict") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_ORDEREDDICT;
            }

            if (strcmp(container_type->tp_name, "time.struct_time") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_STRUCTTIME;
            }

            if (PySlice_Check(rhs)) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_OTHER_SLICE;
            }
            return SPEC_FAIL_BINARY_OP_SUBSCR;
    }
    Py_UNREACHABLE();
}
#endif

/** Binary Op Specialization Extensions */

/* long-long */

static inline int
is_compactlong(PyObject *v)
{
    return PyLong_CheckExact(v) &&
           _PyLong_IsCompact((PyLongObject *)v);
}

static int
compactlongs_guard(PyObject *lhs, PyObject *rhs)
{
    return (is_compactlong(lhs) && is_compactlong(rhs));
}

#define BITWISE_LONGS_ACTION(NAME, OP) \
    static PyObject * \
    (NAME)(PyObject *lhs, PyObject *rhs) \
    { \
        Py_ssize_t rhs_val = _PyLong_CompactValue((PyLongObject *)rhs); \
        Py_ssize_t lhs_val = _PyLong_CompactValue((PyLongObject *)lhs); \
        return PyLong_FromSsize_t(lhs_val OP rhs_val); \
    }
BITWISE_LONGS_ACTION(compactlongs_or, |)
BITWISE_LONGS_ACTION(compactlongs_and, &)
BITWISE_LONGS_ACTION(compactlongs_xor, ^)
#undef BITWISE_LONGS_ACTION

/* float-long */

static inline int
float_compactlong_guard(PyObject *lhs, PyObject *rhs)
{
    return (
        PyFloat_CheckExact(lhs) &&
        !isnan(PyFloat_AsDouble(lhs)) &&
        PyLong_CheckExact(rhs) &&
        _PyLong_IsCompact((PyLongObject *)rhs)
    );
}

static inline int
nonzero_float_compactlong_guard(PyObject *lhs, PyObject *rhs)
{
    return (
        float_compactlong_guard(lhs, rhs) && !PyLong_IsZero(rhs)
    );
}

#define FLOAT_LONG_ACTION(NAME, OP) \
    static PyObject * \
    (NAME)(PyObject *lhs, PyObject *rhs) \
    { \
        double lhs_val = PyFloat_AsDouble(lhs); \
        Py_ssize_t rhs_val = _PyLong_CompactValue((PyLongObject *)rhs); \
        return PyFloat_FromDouble(lhs_val OP rhs_val); \
    }
FLOAT_LONG_ACTION(float_compactlong_add, +)
FLOAT_LONG_ACTION(float_compactlong_subtract, -)
FLOAT_LONG_ACTION(float_compactlong_multiply, *)
FLOAT_LONG_ACTION(float_compactlong_true_div, /)
#undef FLOAT_LONG_ACTION

/*  long-float */

static inline int
compactlong_float_guard(PyObject *lhs, PyObject *rhs)
{
    return (
        PyLong_CheckExact(lhs) &&
        _PyLong_IsCompact((PyLongObject *)lhs) &&
        PyFloat_CheckExact(rhs) &&
        !isnan(PyFloat_AsDouble(rhs))
    );
}

static inline int
nonzero_compactlong_float_guard(PyObject *lhs, PyObject *rhs)
{
    return (
        compactlong_float_guard(lhs, rhs) && PyFloat_AsDouble(rhs) != 0.0
    );
}

#define LONG_FLOAT_ACTION(NAME, OP) \
    static PyObject * \
    (NAME)(PyObject *lhs, PyObject *rhs) \
    { \
        double rhs_val = PyFloat_AsDouble(rhs); \
        Py_ssize_t lhs_val = _PyLong_CompactValue((PyLongObject *)lhs); \
        return PyFloat_FromDouble(lhs_val OP rhs_val); \
    }
LONG_FLOAT_ACTION(compactlong_float_add, +)
LONG_FLOAT_ACTION(compactlong_float_subtract, -)
LONG_FLOAT_ACTION(compactlong_float_multiply, *)
LONG_FLOAT_ACTION(compactlong_float_true_div, /)
#undef LONG_FLOAT_ACTION

static _PyBinaryOpSpecializationDescr binaryop_extend_descrs[] = {
    /* long-long arithmetic */
    {NB_OR, compactlongs_guard, compactlongs_or},
    {NB_AND, compactlongs_guard, compactlongs_and},
    {NB_XOR, compactlongs_guard, compactlongs_xor},
    {NB_INPLACE_OR, compactlongs_guard, compactlongs_or},
    {NB_INPLACE_AND, compactlongs_guard, compactlongs_and},
    {NB_INPLACE_XOR, compactlongs_guard, compactlongs_xor},

    /* float-long arithemetic */
    {NB_ADD, float_compactlong_guard, float_compactlong_add},
    {NB_SUBTRACT, float_compactlong_guard, float_compactlong_subtract},
    {NB_TRUE_DIVIDE, nonzero_float_compactlong_guard, float_compactlong_true_div},
    {NB_MULTIPLY, float_compactlong_guard, float_compactlong_multiply},

    /* float-float arithmetic */
    {NB_ADD, compactlong_float_guard, compactlong_float_add},
    {NB_SUBTRACT, compactlong_float_guard, compactlong_float_subtract},
    {NB_TRUE_DIVIDE, nonzero_compactlong_float_guard, compactlong_float_true_div},
    {NB_MULTIPLY, compactlong_float_guard, compactlong_float_multiply},
};

static int
binary_op_extended_specialization(PyObject *lhs, PyObject *rhs, int oparg,
                                  _PyBinaryOpSpecializationDescr **descr)
{
    size_t n = sizeof(binaryop_extend_descrs)/sizeof(_PyBinaryOpSpecializationDescr);
    for (size_t i = 0; i < n; i++) {
        _PyBinaryOpSpecializationDescr *d = &binaryop_extend_descrs[i];
        if (d->oparg == oparg && d->guard(lhs, rhs)) {
            *descr = d;
            return 1;
        }
    }
    return 0;
}

Py_NO_INLINE void
_Py_Specialize_BinaryOp(_PyStackRef lhs_st, _PyStackRef rhs_st, _Py_CODEUNIT *instr,
                        int oparg, _PyStackRef *locals)
{
    PyObject *lhs = PyStackRef_AsPyObjectBorrow(lhs_st);
    PyObject *rhs = PyStackRef_AsPyObjectBorrow(rhs_st);
    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[BINARY_OP] == INLINE_CACHE_ENTRIES_BINARY_OP);

    _PyBinaryOpCache *cache = (_PyBinaryOpCache *)(instr + 1);
    if (instr->op.code == BINARY_OP_EXTEND) {
        write_ptr(cache->external_cache, NULL);
    }

    switch (oparg) {
        case NB_ADD:
        case NB_INPLACE_ADD:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyUnicode_CheckExact(lhs)) {
                _Py_CODEUNIT next = instr[INLINE_CACHE_ENTRIES_BINARY_OP + 1];
                bool to_store = (next.op.code == STORE_FAST);
                if (to_store && PyStackRef_AsPyObjectBorrow(locals[next.op.arg]) == lhs) {
                    specialize(instr, BINARY_OP_INPLACE_ADD_UNICODE);
                    return;
                }
                specialize(instr, BINARY_OP_ADD_UNICODE);
                return;
            }
            if (PyLong_CheckExact(lhs)) {
                specialize(instr, BINARY_OP_ADD_INT);
                return;
            }
            if (PyFloat_CheckExact(lhs)) {
                specialize(instr, BINARY_OP_ADD_FLOAT);
                return;
            }
            break;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyLong_CheckExact(lhs)) {
                specialize(instr, BINARY_OP_MULTIPLY_INT);
                return;
            }
            if (PyFloat_CheckExact(lhs)) {
                specialize(instr, BINARY_OP_MULTIPLY_FLOAT);
                return;
            }
            break;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyLong_CheckExact(lhs)) {
                specialize(instr, BINARY_OP_SUBTRACT_INT);
                return;
            }
            if (PyFloat_CheckExact(lhs)) {
                specialize(instr, BINARY_OP_SUBTRACT_FLOAT);
                return;
            }
            break;
        case NB_SUBSCR:
            if (PyLong_CheckExact(rhs) && _PyLong_IsNonNegativeCompact((PyLongObject *)rhs)) {
                if (PyList_CheckExact(lhs)) {
                    specialize(instr, BINARY_OP_SUBSCR_LIST_INT);
                    return;
                }
                if (PyTuple_CheckExact(lhs)) {
                    specialize(instr, BINARY_OP_SUBSCR_TUPLE_INT);
                    return;
                }
                if (PyUnicode_CheckExact(lhs)) {
                    specialize(instr, BINARY_OP_SUBSCR_STR_INT);
                    return;
                }
            }
            if (PyDict_CheckExact(lhs)) {
                specialize(instr, BINARY_OP_SUBSCR_DICT);
                return;
            }
            if (PyList_CheckExact(lhs) && PySlice_Check(rhs)) {
                specialize(instr, BINARY_OP_SUBSCR_LIST_SLICE);
                return;
            }
            unsigned int tp_version;
            PyTypeObject *container_type = Py_TYPE(lhs);
            PyObject *descriptor = _PyType_LookupRefAndVersion(container_type, &_Py_ID(__getitem__), &tp_version);
            if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type &&
                container_type->tp_flags & Py_TPFLAGS_HEAPTYPE)
            {
                PyFunctionObject *func = (PyFunctionObject *)descriptor;
                PyCodeObject *fcode = (PyCodeObject *)func->func_code;
                int kind = function_kind(fcode);
                PyHeapTypeObject *ht = (PyHeapTypeObject *)container_type;
                if (kind == SIMPLE_FUNCTION &&
                    fcode->co_argcount == 2 &&
                    !_PyInterpreterState_GET()->eval_frame && /* Don't specialize if PEP 523 is active */
                    _PyType_CacheGetItemForSpecialization(ht, descriptor, (uint32_t)tp_version))
                {
                    specialize(instr, BINARY_OP_SUBSCR_GETITEM);
                    Py_DECREF(descriptor);
                    return;
                }
            }
            Py_XDECREF(descriptor);
            break;
    }

    _PyBinaryOpSpecializationDescr *descr;
    if (binary_op_extended_specialization(lhs, rhs, oparg, &descr)) {
        specialize(instr, BINARY_OP_EXTEND);
        write_ptr(cache->external_cache, (void*)descr);
        return;
    }

    SPECIALIZATION_FAIL(BINARY_OP, binary_op_fail_kind(oparg, lhs, rhs));
    unspecialize(instr);
    return;
}


#ifdef Py_STATS
static int
compare_op_fail_kind(PyObject *lhs, PyObject *rhs)
{
    if (Py_TYPE(lhs) != Py_TYPE(rhs)) {
        if (PyFloat_CheckExact(lhs) && PyLong_CheckExact(rhs)) {
            return SPEC_FAIL_COMPARE_OP_FLOAT_LONG;
        }
        if (PyLong_CheckExact(lhs) && PyFloat_CheckExact(rhs)) {
            return SPEC_FAIL_COMPARE_OP_LONG_FLOAT;
        }
        return SPEC_FAIL_COMPARE_OP_DIFFERENT_TYPES;
    }
    if (PyBytes_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_BYTES;
    }
    if (PyTuple_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_TUPLE;
    }
    if (PyList_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_LIST;
    }
    if (PySet_CheckExact(lhs) || PyFrozenSet_CheckExact(lhs)) {
        return SPEC_FAIL_COMPARE_OP_SET;
    }
    if (PyBool_Check(lhs)) {
        return SPEC_FAIL_COMPARE_OP_BOOL;
    }
    if (Py_TYPE(lhs)->tp_richcompare == PyBaseObject_Type.tp_richcompare) {
        return SPEC_FAIL_COMPARE_OP_BASEOBJECT;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

Py_NO_INLINE void
_Py_Specialize_CompareOp(_PyStackRef lhs_st, _PyStackRef rhs_st, _Py_CODEUNIT *instr,
                         int oparg)
{
    PyObject *lhs = PyStackRef_AsPyObjectBorrow(lhs_st);
    PyObject *rhs = PyStackRef_AsPyObjectBorrow(rhs_st);
    uint8_t specialized_op;

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[COMPARE_OP] == INLINE_CACHE_ENTRIES_COMPARE_OP);
    // All of these specializations compute boolean values, so they're all valid
    // regardless of the fifth-lowest oparg bit.
    if (Py_TYPE(lhs) != Py_TYPE(rhs)) {
        SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs, rhs));
        goto failure;
    }
    if (PyFloat_CheckExact(lhs)) {
        specialized_op = COMPARE_OP_FLOAT;
        goto success;
    }
    if (PyLong_CheckExact(lhs)) {
        if (_PyLong_IsCompact((PyLongObject *)lhs) && _PyLong_IsCompact((PyLongObject *)rhs)) {
            specialized_op = COMPARE_OP_INT;
            goto success;
        }
        else {
            SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_BIG_INT);
            goto failure;
        }
    }
    if (PyUnicode_CheckExact(lhs)) {
        int cmp = oparg >> 5;
        if (cmp != Py_EQ && cmp != Py_NE) {
            SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_STRING);
            goto failure;
        }
        else {
            specialized_op = COMPARE_OP_STR;
            goto success;
        }
    }
    SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs, rhs));
failure:
    unspecialize(instr);
    return;
success:
    specialize(instr, specialized_op);
}

#ifdef Py_STATS
static int
unpack_sequence_fail_kind(PyObject *seq)
{
    if (PySequence_Check(seq)) {
        return SPEC_FAIL_UNPACK_SEQUENCE_SEQUENCE;
    }
    if (PyIter_Check(seq)) {
        return SPEC_FAIL_UNPACK_SEQUENCE_ITERATOR;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

Py_NO_INLINE void
_Py_Specialize_UnpackSequence(_PyStackRef seq_st, _Py_CODEUNIT *instr, int oparg)
{
    PyObject *seq = PyStackRef_AsPyObjectBorrow(seq_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[UNPACK_SEQUENCE] ==
           INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
    if (PyTuple_CheckExact(seq)) {
        if (PyTuple_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            unspecialize(instr);
            return;
        }
        if (PyTuple_GET_SIZE(seq) == 2) {
            specialize(instr, UNPACK_SEQUENCE_TWO_TUPLE);
            return;
        }
        specialize(instr, UNPACK_SEQUENCE_TUPLE);
        return;
    }
    if (PyList_CheckExact(seq)) {
        if (PyList_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            unspecialize(instr);
            return;
        }
        specialize(instr, UNPACK_SEQUENCE_LIST);
        return;
    }
    SPECIALIZATION_FAIL(UNPACK_SEQUENCE, unpack_sequence_fail_kind(seq));
    unspecialize(instr);
}

#ifdef Py_STATS
int
 _PySpecialization_ClassifyIterator(PyObject *iter)
{
    if (PyGen_CheckExact(iter)) {
        return SPEC_FAIL_ITER_GENERATOR;
    }
    if (PyCoro_CheckExact(iter)) {
        return SPEC_FAIL_ITER_COROUTINE;
    }
    if (PyAsyncGen_CheckExact(iter)) {
        return SPEC_FAIL_ITER_ASYNC_GENERATOR;
    }
    if (PyAsyncGenASend_CheckExact(iter)) {
        return SPEC_FAIL_ITER_ASYNC_GENERATOR_SEND;
    }
    PyTypeObject *t = Py_TYPE(iter);
    if (t == &PyListIter_Type) {
        return SPEC_FAIL_ITER_LIST;
    }
    if (t == &PyTupleIter_Type) {
        return SPEC_FAIL_ITER_TUPLE;
    }
    if (t == &PyDictIterKey_Type) {
        return SPEC_FAIL_ITER_DICT_KEYS;
    }
    if (t == &PyDictIterValue_Type) {
        return SPEC_FAIL_ITER_DICT_VALUES;
    }
    if (t == &PyDictIterItem_Type) {
        return SPEC_FAIL_ITER_DICT_ITEMS;
    }
    if (t == &PySetIter_Type) {
        return SPEC_FAIL_ITER_SET;
    }
    if (t == &PyUnicodeIter_Type) {
        return SPEC_FAIL_ITER_STRING;
    }
    if (t == &PyBytesIter_Type) {
        return SPEC_FAIL_ITER_BYTES;
    }
    if (t == &PyRangeIter_Type) {
        return SPEC_FAIL_ITER_RANGE;
    }
    if (t == &PyEnum_Type) {
        return SPEC_FAIL_ITER_ENUMERATE;
    }
    if (t == &PyMap_Type) {
        return SPEC_FAIL_ITER_MAP;
    }
    if (t == &PyZip_Type) {
        return SPEC_FAIL_ITER_ZIP;
    }
    if (t == &PySeqIter_Type) {
        return SPEC_FAIL_ITER_SEQ_ITER;
    }
    if (t == &PyListRevIter_Type) {
        return SPEC_FAIL_ITER_REVERSED_LIST;
    }
    if (t == &_PyUnicodeASCIIIter_Type) {
        return SPEC_FAIL_ITER_ASCII_STRING;
    }
    const char *name = t->tp_name;
    if (strncmp(name, "itertools", 9) == 0) {
        return SPEC_FAIL_ITER_ITERTOOLS;
    }
    if (strncmp(name, "callable_iterator", 17) == 0) {
        return SPEC_FAIL_ITER_CALLABLE;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

Py_NO_INLINE void
_Py_Specialize_ForIter(_PyStackRef iter, _PyStackRef null_or_index, _Py_CODEUNIT *instr, int oparg)
{
    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[FOR_ITER] == INLINE_CACHE_ENTRIES_FOR_ITER);
    PyObject *iter_o = PyStackRef_AsPyObjectBorrow(iter);
    PyTypeObject *tp = Py_TYPE(iter_o);

    if (PyStackRef_IsNull(null_or_index)) {
#ifdef Py_GIL_DISABLED
        // Only specialize for uniquely referenced iterators, so that we know
        // they're only referenced by this one thread. This is more limiting
        // than we need (even `it = iter(mylist); for item in it:` won't get
        // specialized) but we don't have a way to check whether we're the only
        // _thread_ who has access to the object.
        if (!_PyObject_IsUniquelyReferenced(iter_o)) {
            goto failure;
        }
#endif
        if (tp == &PyRangeIter_Type) {
            specialize(instr, FOR_ITER_RANGE);
            return;
        }
        else if (tp == &PyGen_Type && oparg <= SHRT_MAX) {
            // Generators are very much not thread-safe, so don't worry about
            // the specialization not being thread-safe.
            assert(instr[oparg + INLINE_CACHE_ENTRIES_FOR_ITER + 1].op.code == END_FOR  ||
                instr[oparg + INLINE_CACHE_ENTRIES_FOR_ITER + 1].op.code == INSTRUMENTED_END_FOR
            );
            /* Don't specialize if PEP 523 is active */
            if (_PyInterpreterState_GET()->eval_frame) {
                goto failure;
            }
            specialize(instr, FOR_ITER_GEN);
            return;
        }
    }
    else {
        if (tp == &PyList_Type) {
#ifdef Py_GIL_DISABLED
            // Only specialize for lists owned by this thread or shared
            if (!_Py_IsOwnedByCurrentThread(iter_o) && !_PyObject_GC_IS_SHARED(iter_o)) {
                goto failure;
            }
#endif
            specialize(instr, FOR_ITER_LIST);
            return;
        }
        else if (tp == &PyTuple_Type) {
            specialize(instr, FOR_ITER_TUPLE);
            return;
        }
    }
failure:
    SPECIALIZATION_FAIL(FOR_ITER,
                        _PySpecialization_ClassifyIterator(iter_o));
    unspecialize(instr);
}

Py_NO_INLINE void
_Py_Specialize_Send(_PyStackRef receiver_st, _Py_CODEUNIT *instr)
{
    PyObject *receiver = PyStackRef_AsPyObjectBorrow(receiver_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[SEND] == INLINE_CACHE_ENTRIES_SEND);
    PyTypeObject *tp = Py_TYPE(receiver);
    if (tp == &PyGen_Type || tp == &PyCoro_Type) {
        /* Don't specialize if PEP 523 is active */
        if (_PyInterpreterState_GET()->eval_frame) {
            SPECIALIZATION_FAIL(SEND, SPEC_FAIL_OTHER);
            goto failure;
        }
        specialize(instr, SEND_GEN);
        return;
    }
    SPECIALIZATION_FAIL(SEND,
                        _PySpecialization_ClassifyIterator(receiver));
failure:
    unspecialize(instr);
}

#ifdef Py_STATS
static int
to_bool_fail_kind(PyObject *value)
{
    if (PyByteArray_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_BYTEARRAY;
    }
    if (PyBytes_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_BYTES;
    }
    if (PyDict_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_DICT;
    }
    if (PyFloat_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_FLOAT;
    }
    if (PyMemoryView_Check(value)) {
        return SPEC_FAIL_TO_BOOL_MEMORY_VIEW;
    }
    if (PyAnySet_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_SET;
    }
    if (PyTuple_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_TUPLE;
    }
    return SPEC_FAIL_OTHER;
}
#endif  // Py_STATS

static int
check_type_always_true(PyTypeObject *ty)
{
    PyNumberMethods *nb = ty->tp_as_number;
    if (nb && nb->nb_bool) {
        return SPEC_FAIL_TO_BOOL_NUMBER;
    }
    PyMappingMethods *mp = ty->tp_as_mapping;
    if (mp && mp->mp_length) {
        return SPEC_FAIL_TO_BOOL_MAPPING;
    }
    PySequenceMethods *sq = ty->tp_as_sequence;
    if (sq && sq->sq_length) {
      return SPEC_FAIL_TO_BOOL_SEQUENCE;
    }
    return 0;
}

Py_NO_INLINE void
_Py_Specialize_ToBool(_PyStackRef value_o, _Py_CODEUNIT *instr)
{
    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[TO_BOOL] == INLINE_CACHE_ENTRIES_TO_BOOL);
    _PyToBoolCache *cache = (_PyToBoolCache *)(instr + 1);
    PyObject *value = PyStackRef_AsPyObjectBorrow(value_o);
    uint8_t specialized_op;
    if (PyBool_Check(value)) {
        specialized_op = TO_BOOL_BOOL;
        goto success;
    }
    if (PyLong_CheckExact(value)) {
        specialized_op = TO_BOOL_INT;
        goto success;
    }
    if (PyList_CheckExact(value)) {
        specialized_op = TO_BOOL_LIST;
        goto success;
    }
    if (Py_IsNone(value)) {
        specialized_op = TO_BOOL_NONE;
        goto success;
    }
    if (PyUnicode_CheckExact(value)) {
        specialized_op = TO_BOOL_STR;
        goto success;
    }
    if (PyType_HasFeature(Py_TYPE(value), Py_TPFLAGS_HEAPTYPE)) {
        unsigned int version = 0;
        int err = _PyType_Validate(Py_TYPE(value), check_type_always_true, &version);
        if (err < 0) {
            SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_OUT_OF_VERSIONS);
            goto failure;
        }
        else if (err > 0) {
            SPECIALIZATION_FAIL(TO_BOOL, err);
            goto failure;
        }

        assert(err == 0);
        assert(version);
        write_u32(cache->version, version);
        specialized_op = TO_BOOL_ALWAYS_TRUE;
        goto success;
    }

    SPECIALIZATION_FAIL(TO_BOOL, to_bool_fail_kind(value));
failure:
    unspecialize(instr);
    return;
success:
    specialize(instr, specialized_op);
}

#ifdef Py_STATS
static int
containsop_fail_kind(PyObject *value) {
    if (PyUnicode_CheckExact(value)) {
        return SPEC_FAIL_CONTAINS_OP_STR;
    }
    if (PyList_CheckExact(value)) {
        return SPEC_FAIL_CONTAINS_OP_LIST;
    }
    if (PyTuple_CheckExact(value)) {
        return SPEC_FAIL_CONTAINS_OP_TUPLE;
    }
    if (PyType_Check(value)) {
        return SPEC_FAIL_CONTAINS_OP_USER_CLASS;
    }
    return SPEC_FAIL_OTHER;
}
#endif

Py_NO_INLINE void
_Py_Specialize_ContainsOp(_PyStackRef value_st, _Py_CODEUNIT *instr)
{
    PyObject *value = PyStackRef_AsPyObjectBorrow(value_st);

    assert(ENABLE_SPECIALIZATION_FT);
    assert(_PyOpcode_Caches[CONTAINS_OP] == INLINE_CACHE_ENTRIES_COMPARE_OP);
    if (PyDict_CheckExact(value)) {
        specialize(instr, CONTAINS_OP_DICT);
        return;
    }
    if (PySet_CheckExact(value) || PyFrozenSet_CheckExact(value)) {
        specialize(instr, CONTAINS_OP_SET);
        return;
    }

    SPECIALIZATION_FAIL(CONTAINS_OP, containsop_fail_kind(value));
    unspecialize(instr);
    return;
}

#ifdef Py_STATS
void
_Py_GatherStats_GetIter(_PyStackRef iterable)
{
    PyTypeObject *tp = PyStackRef_TYPE(iterable);
    int kind = SPEC_FAIL_OTHER;
    if (tp == &PyTuple_Type) {
        kind = SPEC_FAIL_ITER_TUPLE;
    }
    else if (tp == &PyList_Type) {
        kind = SPEC_FAIL_ITER_LIST;
    }
    else if (tp == &PyDict_Type) {
        kind = SPEC_FAIL_ITER_DICT_KEYS;
    }
    else if (tp == &PySet_Type) {
        kind = SPEC_FAIL_ITER_SET;
    }
    else if (tp == &PyBytes_Type) {
        kind = SPEC_FAIL_ITER_BYTES;
    }
    else if (tp == &PyEnum_Type) {
        kind = SPEC_FAIL_ITER_ENUMERATE;
    }
    else if (tp == &PyUnicode_Type) {
        kind = SPEC_FAIL_ITER_STRING;
    }
    else if (tp == &PyGen_Type) {
        kind = SPEC_FAIL_ITER_GENERATOR;
    }
    else if (tp == &PyCoro_Type) {
        kind = SPEC_FAIL_ITER_COROUTINE;
    }
    else if (tp == &PyAsyncGen_Type) {
        kind = SPEC_FAIL_ITER_ASYNC_GENERATOR;
    }
    else if (tp == &_PyAsyncGenASend_Type) {
        kind = SPEC_FAIL_ITER_ASYNC_GENERATOR_SEND;
    }
    else if (tp->tp_iter == PyObject_SelfIter) {
        kind = SPEC_FAIL_ITER_SELF;
    }
    SPECIALIZATION_FAIL(GET_ITER, kind);
}
#endif


/* Code init cleanup.
 * CALL_ALLOC_AND_ENTER_INIT will set up
 * the frame to execute the EXIT_INIT_CHECK
 * instruction.
 * Ends with a RESUME so that it is not traced.
 * This is used as a plain code object, not a function,
 * so must not access globals or builtins.
 * There are a few other constraints imposed on the code
 * by the free-threaded build:
 *
 * 1. The RESUME instruction must not be executed. Otherwise we may attempt to
 *    free the statically allocated TLBC array.
 * 2. It must contain no specializable instructions. Specializing multiple
 *    copies of the same bytecode is not thread-safe in free-threaded builds.
 *
 * This should be dynamically allocated if either of those restrictions need to
 * be lifted.
 */

#define NO_LOC_4 (128 | (PY_CODE_LOCATION_INFO_NONE << 3) | 3)

static const PyBytesObject no_location = {
    PyVarObject_HEAD_INIT(&PyBytes_Type, 1)
    .ob_sval = { NO_LOC_4 }
};

#ifdef Py_GIL_DISABLED
static _PyCodeArray init_cleanup_tlbc = {
    .size = 1,
    .entries = {(char*) &_Py_InitCleanup.co_code_adaptive},
};
#endif

const struct _PyCode8 _Py_InitCleanup = {
    _PyVarObject_HEAD_INIT(&PyCode_Type, 3),
    .co_consts = (PyObject *)&_Py_SINGLETON(tuple_empty),
    .co_names = (PyObject *)&_Py_SINGLETON(tuple_empty),
    .co_exceptiontable = (PyObject *)&_Py_SINGLETON(bytes_empty),
    .co_flags = CO_OPTIMIZED | CO_NO_MONITORING_EVENTS,
    .co_localsplusnames = (PyObject *)&_Py_SINGLETON(tuple_empty),
    .co_localspluskinds = (PyObject *)&_Py_SINGLETON(bytes_empty),
    .co_filename = &_Py_ID(__init__),
    .co_name = &_Py_ID(__init__),
    .co_qualname = &_Py_ID(__init__),
    .co_linetable = (PyObject *)&no_location,
    ._co_firsttraceable = 4,
    .co_stacksize = 2,
    .co_framesize = 2 + FRAME_SPECIALS_SIZE,
#ifdef Py_GIL_DISABLED
    .co_tlbc = &init_cleanup_tlbc,
#endif
    .co_code_adaptive = {
        EXIT_INIT_CHECK, 0,
        RETURN_VALUE, 0,
        RESUME, RESUME_AT_FUNC_START,
    }
};
