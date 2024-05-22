#include "Python.h"

#include "opcode.h"

#include "pycore_code.h"
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_dict.h"          // DICT_KEYS_UNICODE
#include "pycore_function.h"      // _PyFunction_GetVersionForCurrentState()
#include "pycore_long.h"          // _PyLong_IsNonNegativeCompact()
#include "pycore_moduleobject.h"
#include "pycore_object.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_uop_metadata.h"    // _PyOpcode_uop_name
#include "pycore_opcode_utils.h"  // RESUME_AT_FUNC_START
#include "pycore_pylifecycle.h"   // _PyOS_URandomNonblock()
#include "pycore_runtime.h"       // _Py_ID()

#include <stdlib.h> // rand()


/* For guidance on adding or extending families of instructions see
 * ./adaptive.md
 */

#ifdef Py_STATS
GCStats _py_gc_stats[NUM_GENERATIONS] = { 0 };
static PyStats _Py_stats_struct = { .gc_stats = _py_gc_stats };
PyStats *_Py_stats = NULL;

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
    err += add_stat_dict(stats, LOAD_SUPER_ATTR, "load_super_attr");
    err += add_stat_dict(stats, LOAD_ATTR, "load_attr");
    err += add_stat_dict(stats, LOAD_GLOBAL, "load_global");
    err += add_stat_dict(stats, BINARY_SUBSCR, "binary_subscr");
    err += add_stat_dict(stats, STORE_SUBSCR, "store_subscr");
    err += add_stat_dict(stats, STORE_ATTR, "store_attr");
    err += add_stat_dict(stats, CALL, "call");
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
    for (int i = 0; i < 256; i++) {
        if (_PyOpcode_Caches[i] && i != JUMP_BACKWARD) {
            fprintf(out, "opcode[%s].specializable : 1\n", _PyOpcode_OpName[i]);
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
    fprintf(out, "Object new values: %" PRIu64 "\n", stats->new_values);
    fprintf(out, "Object interpreter increfs: %" PRIu64 "\n", stats->interpreter_increfs);
    fprintf(out, "Object interpreter decrefs: %" PRIu64 "\n", stats->interpreter_decrefs);
    fprintf(out, "Object increfs: %" PRIu64 "\n", stats->increfs);
    fprintf(out, "Object decrefs: %" PRIu64 "\n", stats->decrefs);
    fprintf(out, "Object materialize dict (on request): %" PRIu64 "\n", stats->dict_materialized_on_request);
    fprintf(out, "Object materialize dict (new key): %" PRIu64 "\n", stats->dict_materialized_new_key);
    fprintf(out, "Object materialize dict (too big): %" PRIu64 "\n", stats->dict_materialized_too_big);
    fprintf(out, "Object materialize dict (str subclass): %" PRIu64 "\n", stats->dict_materialized_str_subclass);
    fprintf(out, "Object dematerialize dict: %" PRIu64 "\n", stats->dict_dematerialized);
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
    }
}

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

    print_histogram(out, "Trace length", stats->trace_length_hist);
    print_histogram(out, "Trace run length", stats->trace_run_length_hist);
    print_histogram(out, "Optimized trace length", stats->optimized_trace_length_hist);

    const char* const* names;
    for (int i = 0; i < 512; i++) {
        if (i < 256) {
            names = _PyOpcode_OpName;
        } else {
            names = _PyOpcode_uop_name;
        }
        if (stats->opcode[i].execution_count) {
            fprintf(out, "uops[%s].execution_count : %" PRIu64 "\n", names[i], stats->opcode[i].execution_count);
        }
        if (stats->opcode[i].miss) {
            fprintf(out, "uops[%s].specialization.miss : %" PRIu64 "\n", names[i], stats->opcode[i].miss);
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
}

static void
print_stats(FILE *out, PyStats *stats)
{
    print_spec_stats(out, stats->opcode_stats);
    print_call_stats(out, &stats->call_stats);
    print_object_stats(out, &stats->object_stats);
    print_gc_stats(out, stats->gc_stats);
    print_optimization_stats(out, &stats->optimization_stats);
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
        _Py_stats->opcode_stats[opcode].specialization.failure_kinds[kind]++; \
    } \
} while (0)

#endif  // Py_STATS


#ifndef SPECIALIZATION_FAIL
#  define SPECIALIZATION_FAIL(opcode, kind) ((void)0)
#endif

// Initialize warmup counters and insert superinstructions. This cannot fail.
void
_PyCode_Quicken(PyCodeObject *code)
{
    #if ENABLE_SPECIALIZATION
    int opcode = 0;
    _Py_CODEUNIT *instructions = _PyCode_CODE(code);
    for (int i = 0; i < Py_SIZE(code); i++) {
        opcode = _Py_GetBaseOpcode(code, i);
        assert(opcode < MIN_INSTRUMENTED_OPCODE);
        int caches = _PyOpcode_Caches[opcode];
        if (caches) {
            // The initial value depends on the opcode
            int initial_value;
            switch (opcode) {
                case JUMP_BACKWARD:
                    initial_value = 0;
                    break;
                case POP_JUMP_IF_FALSE:
                case POP_JUMP_IF_TRUE:
                case POP_JUMP_IF_NONE:
                case POP_JUMP_IF_NOT_NONE:
                    initial_value = 0x5555;  // Alternating 0, 1 bits
                    break;
                default:
                    initial_value = adaptive_counter_warmup();
                    break;
            }
            instructions[i + 1].cache = initial_value;
            i += caches;
        }
    }
    #endif /* ENABLE_SPECIALIZATION */
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
#define SPEC_FAIL_ATTR_NON_STRING_OR_SPLIT 19
#define SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND 20

#define SPEC_FAIL_ATTR_SHADOWED 21
#define SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD 22
#define SPEC_FAIL_ATTR_CLASS_METHOD_OBJ 23
#define SPEC_FAIL_ATTR_OBJECT_SLOT 24
#define SPEC_FAIL_ATTR_HAS_MANAGED_DICT 25
#define SPEC_FAIL_ATTR_INSTANCE_ATTRIBUTE 26
#define SPEC_FAIL_ATTR_METACLASS_ATTRIBUTE 27
#define SPEC_FAIL_ATTR_PROPERTY_NOT_PY_FUNCTION 28
#define SPEC_FAIL_ATTR_NOT_IN_KEYS 29
#define SPEC_FAIL_ATTR_NOT_IN_DICT 30
#define SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE 31
#define SPEC_FAIL_ATTR_CLASS_ATTR_DESCRIPTOR 32
#define SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD_OBJ 33

/* Binary subscr and store subscr */

#define SPEC_FAIL_SUBSCR_ARRAY_INT 9
#define SPEC_FAIL_SUBSCR_ARRAY_SLICE 10
#define SPEC_FAIL_SUBSCR_LIST_SLICE 11
#define SPEC_FAIL_SUBSCR_TUPLE_SLICE 12
#define SPEC_FAIL_SUBSCR_STRING_SLICE 14
#define SPEC_FAIL_SUBSCR_BUFFER_INT 15
#define SPEC_FAIL_SUBSCR_BUFFER_SLICE 16
#define SPEC_FAIL_SUBSCR_SEQUENCE_INT 17

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
#define SPEC_FAIL_CALL_STR 24
#define SPEC_FAIL_CALL_CLASS_NO_VECTORCALL 25
#define SPEC_FAIL_CALL_CLASS_MUTABLE 26
#define SPEC_FAIL_CALL_METHOD_WRAPPER 28
#define SPEC_FAIL_CALL_OPERATOR_WRAPPER 29
#define SPEC_FAIL_CALL_INIT_NOT_SIMPLE 30

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

static int function_kind(PyCodeObject *code);
static bool function_check_args(PyObject *o, int expected_argcount, int opcode);
static uint32_t function_get_version(PyObject *o, int opcode);

static int
specialize_module_load_attr(
    PyObject *owner, _Py_CODEUNIT *instr, PyObject *name
) {
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyModuleObject *m = (PyModuleObject *)owner;
    assert((owner->ob_type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);
    PyDictObject *dict = (PyDictObject *)m->md_dict;
    if (dict == NULL) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_NO_DICT);
        return -1;
    }
    if (dict->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_STRING_OR_SPLIT);
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
    uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(
            _PyInterpreterState_GET(), dict->ma_keys);
    if (keys_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    write_u32(cache->version, keys_version);
    cache->index = (uint16_t)index;
    instr->op.code = LOAD_ATTR_MODULE;
    return 0;
}



/* Attribute specialization */

void
_Py_Specialize_LoadSuperAttr(PyObject *global_super, PyObject *cls, _Py_CODEUNIT *instr, int load_method) {
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[LOAD_SUPER_ATTR] == INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR);
    _PySuperAttrCache *cache = (_PySuperAttrCache *)(instr + 1);
    if (global_super != (PyObject *)&PySuper_Type) {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_SHADOWED);
        goto fail;
    }
    if (!PyType_Check(cls)) {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_BAD_CLASS);
        goto fail;
    }
    instr->op.code = load_method ? LOAD_SUPER_ATTR_METHOD : LOAD_SUPER_ATTR_ATTR;
    goto success;

fail:
    STAT_INC(LOAD_SUPER_ATTR, failure);
    assert(!PyErr_Occurred());
    instr->op.code = LOAD_SUPER_ATTR;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(LOAD_SUPER_ATTR, success);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_cooldown();
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
analyze_descriptor(PyTypeObject *type, PyObject *name, PyObject **descr, int store)
{
    bool has_getattr = false;
    if (store) {
        if (type->tp_setattro != PyObject_GenericSetAttr) {
            *descr = NULL;
            return GETSET_OVERRIDDEN;
        }
    }
    else {
        getattrofunc getattro_slot = type->tp_getattro;
        if (getattro_slot == PyObject_GenericGetAttr) {
            /* Normal attribute lookup; */
            has_getattr = false;
        }
        else if (getattro_slot == _Py_slot_tp_getattr_hook ||
            getattro_slot == _Py_slot_tp_getattro) {
            /* One or both of __getattribute__ or __getattr__ may have been
             overridden See typeobject.c for why these functions are special. */
            PyObject *getattribute = _PyType_Lookup(type,
                &_Py_ID(__getattribute__));
            PyInterpreterState *interp = _PyInterpreterState_GET();
            bool has_custom_getattribute = getattribute != NULL &&
                getattribute != interp->callable_cache.object__getattribute__;
            has_getattr = _PyType_Lookup(type, &_Py_ID(__getattr__)) != NULL;
            if (has_custom_getattribute) {
                if (getattro_slot == _Py_slot_tp_getattro &&
                    !has_getattr &&
                    Py_IS_TYPE(getattribute, &PyFunction_Type)) {
                    *descr = getattribute;
                    return GETATTRIBUTE_IS_PYTHON_FUNCTION;
                }
                /* Potentially both __getattr__ and __getattribute__ are set.
                   Too complicated */
                *descr = NULL;
                return GETSET_OVERRIDDEN;
            }
            /* Potentially has __getattr__ but no custom __getattribute__.
               Fall through to usual descriptor analysis.
               Usual attribute lookup should only be allowed at runtime
               if we can guarantee that there is no way an exception can be
               raised. This means some specializations, e.g. specializing
               for property() isn't safe.
            */
        }
        else {
            *descr = NULL;
            return GETSET_OVERRIDDEN;
        }
    }
    PyObject *descriptor = _PyType_Lookup(type, name);
    *descr = descriptor;
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
        if (PyUnicode_CompareWithASCIIString(name, "__class__") == 0) {
            if (descriptor == _PyType_Lookup(&PyBaseObject_Type, name)) {
                return DUNDER_CLASS;
            }
        }
        if (store) {
            return OVERRIDING;
        }
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

static int
specialize_dict_access(
    PyObject *owner, _Py_CODEUNIT *instr, PyTypeObject *type,
    DescriptorClassification kind, PyObject *name,
    int base_op, int values_op, int hint_op)
{
    assert(kind == NON_OVERRIDING || kind == NON_DESCRIPTOR || kind == ABSENT ||
        kind == BUILTIN_CLASSMETHOD || kind == PYTHON_CLASSMETHOD);
    // No descriptor, or non overriding.
    if ((type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
        return 0;
    }
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyDictOrValues *dorv = _PyObject_DictOrValuesPointer(owner);
    if (_PyDictOrValues_IsValues(*dorv) ||
        _PyObject_MakeInstanceAttributesFromDict(owner, dorv))
    {
        // Virtual dictionary
        PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
        assert(PyUnicode_CheckExact(name));
        Py_ssize_t index = _PyDictKeys_StringLookup(keys, name);
        assert (index != DKIX_ERROR);
        if (index != (uint16_t)index) {
            SPECIALIZATION_FAIL(base_op,
                                index == DKIX_EMPTY ?
                                SPEC_FAIL_ATTR_NOT_IN_KEYS :
                                SPEC_FAIL_OUT_OF_RANGE);
            return 0;
        }
        write_u32(cache->version, type->tp_version_tag);
        cache->index = (uint16_t)index;
        instr->op.code = values_op;
    }
    else {
        PyDictObject *dict = (PyDictObject *)_PyDictOrValues_GetDict(*dorv);
        if (dict == NULL || !PyDict_CheckExact(dict)) {
            SPECIALIZATION_FAIL(base_op, SPEC_FAIL_NO_DICT);
            return 0;
        }
        // We found an instance with a __dict__.
        if (dict->ma_values) {
            SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NON_STRING_OR_SPLIT);
            return 0;
        }
        Py_ssize_t index =
            _PyDict_LookupIndex(dict, name);
        if (index != (uint16_t)index) {
            SPECIALIZATION_FAIL(base_op,
                                index == DKIX_EMPTY ?
                                SPEC_FAIL_ATTR_NOT_IN_DICT :
                                SPEC_FAIL_OUT_OF_RANGE);
            return 0;
        }
        cache->index = (uint16_t)index;
        write_u32(cache->version, type->tp_version_tag);
        instr->op.code = hint_op;
    }
    return 1;
}

static int specialize_attr_loadclassattr(PyObject* owner, _Py_CODEUNIT* instr, PyObject* name,
    PyObject* descr, DescriptorClassification kind, bool is_method);
static int specialize_class_load_attr(PyObject* owner, _Py_CODEUNIT* instr, PyObject* name);

void
_Py_Specialize_LoadAttr(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[LOAD_ATTR] == INLINE_CACHE_ENTRIES_LOAD_ATTR);
    _PyAttrCache *cache = (_PyAttrCache *)(instr + 1);
    PyTypeObject *type = Py_TYPE(owner);
    if (!_PyType_IsReady(type)) {
        // We *might* not really need this check, but we inherited it from
        // PyObject_GenericGetAttr and friends... and this way we still do the
        // right thing if someone forgets to call PyType_Ready(type):
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
        goto fail;
    }
    if (PyModule_CheckExact(owner)) {
        if (specialize_module_load_attr(owner, instr, name))
        {
            goto fail;
        }
        goto success;
    }
    if (PyType_Check(owner)) {
        if (specialize_class_load_attr(owner, instr, name)) {
            goto fail;
        }
        goto success;
    }
    PyObject *descr = NULL;
    DescriptorClassification kind = analyze_descriptor(type, name, &descr, 0);
    assert(descr != NULL || kind == ABSENT || kind == GETSET_OVERRIDDEN);
    switch(kind) {
        case OVERRIDING:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            goto fail;
        case METHOD:
        {
            int oparg = instr->op.arg;
            if (oparg & 1) {
                if (specialize_attr_loadclassattr(owner, instr, name, descr, kind, true)) {
                    goto success;
                }
            }
            else {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
            }
            goto fail;
        }
        case PROPERTY:
        {
            _PyLoadMethodCache *lm_cache = (_PyLoadMethodCache *)(instr + 1);
            assert(Py_TYPE(descr) == &PyProperty_Type);
            PyObject *fget = ((_PyPropertyObject *)descr)->prop_get;
            if (fget == NULL) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                goto fail;
            }
            if (!Py_IS_TYPE(fget, &PyFunction_Type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_PROPERTY_NOT_PY_FUNCTION);
                goto fail;
            }
            if (!function_check_args(fget, 1, LOAD_ATTR)) {
                goto fail;
            }
            if (instr->op.arg & 1) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
                goto fail;
            }
            uint32_t version = function_get_version(fget, LOAD_ATTR);
            if (version == 0) {
                goto fail;
            }
            if (_PyInterpreterState_GET()->eval_frame) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
                goto fail;
            }
            write_u32(lm_cache->keys_version, version);
            assert(type->tp_version_tag != 0);
            write_u32(lm_cache->type_version, type->tp_version_tag);
            /* borrowed */
            write_obj(lm_cache->descr, fget);
            instr->op.code = LOAD_ATTR_PROPERTY;
            goto success;
        }
        case OBJECT_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (!PyObject_TypeCheck(owner, member->d_common.d_type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                goto fail;
            }
            if (dmem->flags & Py_AUDIT_READ) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_AUDITED_SLOT);
                goto fail;
            }
            if (offset != (uint16_t)offset) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                goto fail;
            }
            assert(dmem->type == Py_T_OBJECT_EX || dmem->type == _Py_T_OBJECT);
            assert(offset > 0);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, type->tp_version_tag);
            instr->op.code = LOAD_ATTR_SLOT;
            goto success;
        }
        case DUNDER_CLASS:
        {
            Py_ssize_t offset = offsetof(PyObject, ob_type);
            assert(offset == (uint16_t)offset);
            cache->index = (uint16_t)offset;
            write_u32(cache->version, type->tp_version_tag);
            instr->op.code = LOAD_ATTR_SLOT;
            goto success;
        }
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_OBJECT_SLOT);
            goto fail;
        case MUTABLE:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            goto fail;
        case GETSET_OVERRIDDEN:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
            goto fail;
        case GETATTRIBUTE_IS_PYTHON_FUNCTION:
        {
            assert(type->tp_getattro == _Py_slot_tp_getattro);
            assert(Py_IS_TYPE(descr, &PyFunction_Type));
            _PyLoadMethodCache *lm_cache = (_PyLoadMethodCache *)(instr + 1);
            if (!function_check_args(descr, 2, LOAD_ATTR)) {
                goto fail;
            }
            if (instr->op.arg & 1) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
                goto fail;
            }
            uint32_t version = function_get_version(descr, LOAD_ATTR);
            if (version == 0) {
                goto fail;
            }
            if (_PyInterpreterState_GET()->eval_frame) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
                goto fail;
            }
            write_u32(lm_cache->keys_version, version);
            /* borrowed */
            write_obj(lm_cache->descr, descr);
            write_u32(lm_cache->type_version, type->tp_version_tag);
            instr->op.code = LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN;
            goto success;
        }
        case BUILTIN_CLASSMETHOD:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD_OBJ);
            goto fail;
        case PYTHON_CLASSMETHOD:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_CLASS_METHOD_OBJ);
            goto fail;
        case NON_OVERRIDING:
            SPECIALIZATION_FAIL(LOAD_ATTR,
                                (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) ?
                                SPEC_FAIL_ATTR_CLASS_ATTR_DESCRIPTOR :
                                SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
            goto fail;
        case NON_DESCRIPTOR:
            if ((instr->op.arg & 1) == 0) {
                if (specialize_attr_loadclassattr(owner, instr, name, descr, kind, false)) {
                    goto success;
                }
            }
            else {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE);
            }
            goto fail;
        case ABSENT:
            if (specialize_dict_access(owner, instr, type, kind, name, LOAD_ATTR,
                                    LOAD_ATTR_INSTANCE_VALUE, LOAD_ATTR_WITH_HINT))
            {
                goto success;
            }
    }
fail:
    STAT_INC(LOAD_ATTR, failure);
    assert(!PyErr_Occurred());
    instr->op.code = LOAD_ATTR;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(LOAD_ATTR, success);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_cooldown();
}

void
_Py_Specialize_StoreAttr(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[STORE_ATTR] == INLINE_CACHE_ENTRIES_STORE_ATTR);
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
    PyObject *descr;
    DescriptorClassification kind = analyze_descriptor(type, name, &descr, 1);
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
            write_u32(cache->version, type->tp_version_tag);
            instr->op.code = STORE_ATTR_SLOT;
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
            if (specialize_dict_access(owner, instr, type, kind, name, STORE_ATTR,
                                    STORE_ATTR_INSTANCE_VALUE, STORE_ATTR_WITH_HINT))
            {
                goto success;
            }
    }
fail:
    STAT_INC(STORE_ATTR, failure);
    assert(!PyErr_Occurred());
    instr->op.code = STORE_ATTR;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(STORE_ATTR, success);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_cooldown();
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
    _PyLoadMethodCache *cache = (_PyLoadMethodCache *)(instr + 1);
    if (!PyType_CheckExact(owner) || _PyType_Lookup(Py_TYPE(owner), name)) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METACLASS_ATTRIBUTE);
        return -1;
    }
    PyObject *descr = NULL;
    DescriptorClassification kind = 0;
    kind = analyze_descriptor((PyTypeObject *)owner, name, &descr, 0);
    switch (kind) {
        case METHOD:
        case NON_DESCRIPTOR:
            write_u32(cache->type_version, ((PyTypeObject *)owner)->tp_version_tag);
            write_obj(cache->descr, descr);
            instr->op.code = LOAD_ATTR_CLASS;
            return 0;
#ifdef Py_STATS
        case ABSENT:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
            return -1;
#endif
        default:
            SPECIALIZATION_FAIL(LOAD_ATTR, load_attr_fail_kind(kind));
            return -1;
    }
}

// Please collect stats carefully before and after modifying. A subtle change
// can cause a significant drop in cache hits. A possible test is
// python.exe -m test_typing test_re test_dis test_zlib.
static int
specialize_attr_loadclassattr(PyObject *owner, _Py_CODEUNIT *instr, PyObject *name,
PyObject *descr, DescriptorClassification kind, bool is_method)
{
    _PyLoadMethodCache *cache = (_PyLoadMethodCache *)(instr + 1);
    PyTypeObject *owner_cls = Py_TYPE(owner);

    assert(descr != NULL);
    assert((is_method && kind == METHOD) || (!is_method && kind == NON_DESCRIPTOR));
    if (owner_cls->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        PyDictOrValues *dorv = _PyObject_DictOrValuesPointer(owner);
        PyDictKeysObject *keys = ((PyHeapTypeObject *)owner_cls)->ht_cached_keys;
        if (!_PyDictOrValues_IsValues(*dorv) &&
            !_PyObject_MakeInstanceAttributesFromDict(owner, dorv))
        {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_HAS_MANAGED_DICT);
            return 0;
        }
        Py_ssize_t index = _PyDictKeys_StringLookup(keys, name);
        if (index != DKIX_EMPTY) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_SHADOWED);
            return 0;
        }
        uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(
                _PyInterpreterState_GET(), keys);
        if (keys_version == 0) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
            return 0;
        }
        write_u32(cache->keys_version, keys_version);
        instr->op.code = is_method ? LOAD_ATTR_METHOD_WITH_VALUES : LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES;
    }
    else {
        Py_ssize_t dictoffset = owner_cls->tp_dictoffset;
        if (dictoffset < 0 || dictoffset > INT16_MAX) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_RANGE);
            return 0;
        }
        if (dictoffset == 0) {
            instr->op.code = is_method ? LOAD_ATTR_METHOD_NO_DICT : LOAD_ATTR_NONDESCRIPTOR_NO_DICT;
        }
        else if (is_method) {
            PyObject *dict = *(PyObject **) ((char *)owner + dictoffset);
            if (dict) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
                return 0;
            }
            assert(owner_cls->tp_dictoffset > 0);
            assert(owner_cls->tp_dictoffset <= INT16_MAX);
            instr->op.code = LOAD_ATTR_METHOD_LAZY_DICT;
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
    write_u32(cache->type_version, owner_cls->tp_version_tag);
    write_obj(cache->descr, descr);
    return 1;
}

void
_Py_Specialize_LoadGlobal(
    PyObject *globals, PyObject *builtins,
    _Py_CODEUNIT *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION);
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
        uint32_t keys_version = _PyDictKeys_GetVersionForCurrentState(
                interp, globals_keys);
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
        instr->op.code = LOAD_GLOBAL_MODULE;
        goto success;
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
    uint32_t globals_version = _PyDictKeys_GetVersionForCurrentState(
            interp, globals_keys);
    if (globals_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    if (globals_version != (uint16_t)globals_version) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_RANGE);
        goto fail;
    }
    uint32_t builtins_version = _PyDictKeys_GetVersionForCurrentState(
            interp, builtin_keys);
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
    instr->op.code = LOAD_GLOBAL_BUILTIN;
    goto success;
fail:
    STAT_INC(LOAD_GLOBAL, failure);
    assert(!PyErr_Occurred());
    instr->op.code = LOAD_GLOBAL;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(LOAD_GLOBAL, success);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_cooldown();
}

#ifdef Py_STATS
static int
binary_subscr_fail_kind(PyTypeObject *container_type, PyObject *sub)
{
    if (strcmp(container_type->tp_name, "array.array") == 0) {
        if (PyLong_CheckExact(sub)) {
            return SPEC_FAIL_SUBSCR_ARRAY_INT;
        }
        if (PySlice_Check(sub)) {
            return SPEC_FAIL_SUBSCR_ARRAY_SLICE;
        }
        return SPEC_FAIL_OTHER;
    }
    else if (container_type->tp_as_buffer) {
        if (PyLong_CheckExact(sub)) {
            return SPEC_FAIL_SUBSCR_BUFFER_INT;
        }
        if (PySlice_Check(sub)) {
            return SPEC_FAIL_SUBSCR_BUFFER_SLICE;
        }
        return SPEC_FAIL_OTHER;
    }
    else if (container_type->tp_as_sequence) {
        if (PyLong_CheckExact(sub) && container_type->tp_as_sequence->sq_item) {
            return SPEC_FAIL_SUBSCR_SEQUENCE_INT;
        }
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

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
    if (version == 0) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_OUT_OF_VERSIONS);
        return 0;
    }
    return version;
}

void
_Py_Specialize_BinarySubscr(
     PyObject *container, PyObject *sub, _Py_CODEUNIT *instr)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[BINARY_SUBSCR] ==
           INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
    _PyBinarySubscrCache *cache = (_PyBinarySubscrCache *)(instr + 1);
    PyTypeObject *container_type = Py_TYPE(container);
    if (container_type == &PyList_Type) {
        if (PyLong_CheckExact(sub)) {
            if (_PyLong_IsNonNegativeCompact((PyLongObject *)sub)) {
                instr->op.code = BINARY_SUBSCR_LIST_INT;
                goto success;
            }
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
            goto fail;
        }
        SPECIALIZATION_FAIL(BINARY_SUBSCR,
            PySlice_Check(sub) ? SPEC_FAIL_SUBSCR_LIST_SLICE : SPEC_FAIL_OTHER);
        goto fail;
    }
    if (container_type == &PyTuple_Type) {
        if (PyLong_CheckExact(sub)) {
            if (_PyLong_IsNonNegativeCompact((PyLongObject *)sub)) {
                instr->op.code = BINARY_SUBSCR_TUPLE_INT;
                goto success;
            }
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
            goto fail;
        }
        SPECIALIZATION_FAIL(BINARY_SUBSCR,
            PySlice_Check(sub) ? SPEC_FAIL_SUBSCR_TUPLE_SLICE : SPEC_FAIL_OTHER);
        goto fail;
    }
    if (container_type == &PyUnicode_Type) {
        if (PyLong_CheckExact(sub)) {
            if (_PyLong_IsNonNegativeCompact((PyLongObject *)sub)) {
                instr->op.code = BINARY_SUBSCR_STR_INT;
                goto success;
            }
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
            goto fail;
        }
        SPECIALIZATION_FAIL(BINARY_SUBSCR,
            PySlice_Check(sub) ? SPEC_FAIL_SUBSCR_STRING_SLICE : SPEC_FAIL_OTHER);
        goto fail;
    }
    if (container_type == &PyDict_Type) {
        instr->op.code = BINARY_SUBSCR_DICT;
        goto success;
    }
    PyTypeObject *cls = Py_TYPE(container);
    PyObject *descriptor = _PyType_Lookup(cls, &_Py_ID(__getitem__));
    if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
        if (!(container_type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_SUBSCR_NOT_HEAP_TYPE);
            goto fail;
        }
        PyFunctionObject *func = (PyFunctionObject *)descriptor;
        PyCodeObject *fcode = (PyCodeObject *)func->func_code;
        int kind = function_kind(fcode);
        if (kind != SIMPLE_FUNCTION) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, kind);
            goto fail;
        }
        if (fcode->co_argcount != 2) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
            goto fail;
        }
        uint32_t version = _PyFunction_GetVersionForCurrentState(func);
        if (version == 0) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_OUT_OF_VERSIONS);
            goto fail;
        }
        if (_PyInterpreterState_GET()->eval_frame) {
            SPECIALIZATION_FAIL(BINARY_SUBSCR, SPEC_FAIL_OTHER);
            goto fail;
        }
        PyHeapTypeObject *ht = (PyHeapTypeObject *)container_type;
        // This pointer is invalidated by PyType_Modified (see the comment on
        // struct _specialization_cache):
        ht->_spec_cache.getitem = descriptor;
        ht->_spec_cache.getitem_version = version;
        instr->op.code = BINARY_SUBSCR_GETITEM;
        goto success;
    }
    SPECIALIZATION_FAIL(BINARY_SUBSCR,
                        binary_subscr_fail_kind(container_type, sub));
fail:
    STAT_INC(BINARY_SUBSCR, failure);
    assert(!PyErr_Occurred());
    instr->op.code = BINARY_SUBSCR;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(BINARY_SUBSCR, success);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_cooldown();
}

void
_Py_Specialize_StoreSubscr(PyObject *container, PyObject *sub, _Py_CODEUNIT *instr)
{
    assert(ENABLE_SPECIALIZATION);
    _PyStoreSubscrCache *cache = (_PyStoreSubscrCache *)(instr + 1);
    PyTypeObject *container_type = Py_TYPE(container);
    if (container_type == &PyList_Type) {
        if (PyLong_CheckExact(sub)) {
            if (_PyLong_IsNonNegativeCompact((PyLongObject *)sub)
                && ((PyLongObject *)sub)->long_value.ob_digit[0] < (size_t)PyList_GET_SIZE(container))
            {
                instr->op.code = STORE_SUBSCR_LIST_INT;
                goto success;
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
                goto fail;
            }
        }
        else if (PySlice_Check(sub)) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_LIST_SLICE);
            goto fail;
        }
        else {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            goto fail;
        }
    }
    if (container_type == &PyDict_Type) {
        instr->op.code = STORE_SUBSCR_DICT;
        goto success;
    }
#ifdef Py_STATS
    PyMappingMethods *as_mapping = container_type->tp_as_mapping;
    if (as_mapping && (as_mapping->mp_ass_subscript
                       == PyDict_Type.tp_as_mapping->mp_ass_subscript)) {
        SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE);
        goto fail;
    }
    if (PyObject_CheckBuffer(container)) {
        if (PyLong_CheckExact(sub) && (!_PyLong_IsNonNegativeCompact((PyLongObject *)sub))) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
        }
        else if (strcmp(container_type->tp_name, "array.array") == 0) {
            if (PyLong_CheckExact(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_ARRAY_INT);
            }
            else if (PySlice_Check(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_ARRAY_SLICE);
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            }
        }
        else if (PyByteArray_CheckExact(container)) {
            if (PyLong_CheckExact(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BYTEARRAY_INT);
            }
            else if (PySlice_Check(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BYTEARRAY_SLICE);
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            }
        }
        else {
            if (PyLong_CheckExact(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BUFFER_INT);
            }
            else if (PySlice_Check(sub)) {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_BUFFER_SLICE);
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            }
        }
        goto fail;
    }
    PyObject *descriptor = _PyType_Lookup(container_type, &_Py_ID(__setitem__));
    if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
        PyFunctionObject *func = (PyFunctionObject *)descriptor;
        PyCodeObject *code = (PyCodeObject *)func->func_code;
        int kind = function_kind(code);
        if (kind == SIMPLE_FUNCTION) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_PY_SIMPLE);
        }
        else {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_PY_OTHER);
        }
        goto fail;
    }
#endif   // Py_STATS
    SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
fail:
    STAT_INC(STORE_SUBSCR, failure);
    assert(!PyErr_Occurred());
    instr->op.code = STORE_SUBSCR;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(STORE_SUBSCR, success);
    assert(!PyErr_Occurred());
    cache->counter = adaptive_counter_cooldown();
}

/* Returns a borrowed reference.
 * The reference is only valid if guarded by a type version check.
 */
static PyFunctionObject *
get_init_for_simple_managed_python_class(PyTypeObject *tp)
{
    assert(tp->tp_new == PyBaseObject_Type.tp_new);
    if (tp->tp_alloc != PyType_GenericAlloc) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OVERRIDDEN);
        return NULL;
    }
    if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_NO_DICT);
        return NULL;
    }
    if (!(tp->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        /* Is this possible? */
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_EXPECTED_ERROR);
        return NULL;
    }
    PyObject *init = _PyType_Lookup(tp, &_Py_ID(__init__));
    if (init == NULL || !PyFunction_Check(init)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_PYTHON);
        return NULL;
    }
    int kind = function_kind((PyCodeObject *)PyFunction_GET_CODE(init));
    if (kind != SIMPLE_FUNCTION) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_SIMPLE);
        return NULL;
    }
    ((PyHeapTypeObject *)tp)->_spec_cache.init = init;
    return (PyFunctionObject *)init;
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
                instr->op.code = CALL_STR_1;
                return 0;
            }
            else if (tp == &PyType_Type) {
                instr->op.code = CALL_TYPE_1;
                return 0;
            }
            else if (tp == &PyTuple_Type) {
                instr->op.code = CALL_TUPLE_1;
                return 0;
            }
        }
        if (tp->tp_vectorcall != NULL) {
            instr->op.code = CALL_BUILTIN_CLASS;
            return 0;
        }
        SPECIALIZATION_FAIL(CALL, tp == &PyUnicode_Type ?
            SPEC_FAIL_CALL_STR : SPEC_FAIL_CALL_CLASS_NO_VECTORCALL);
        return -1;
    }
    if (tp->tp_new == PyBaseObject_Type.tp_new) {
        PyFunctionObject *init = get_init_for_simple_managed_python_class(tp);
        if (init != NULL) {
            if (((PyCodeObject *)init->func_code)->co_argcount != nargs+1) {
                SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return -1;
            }
            _PyCallCache *cache = (_PyCallCache *)(instr + 1);
            write_u32(cache->func_version, tp->tp_version_tag);
            _Py_SET_OPCODE(*instr, CALL_ALLOC_AND_ENTER_INIT);
            return 0;
        }
        return -1;
    }
    SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_CLASS_MUTABLE);
    return -1;
}

#ifdef Py_STATS
static int
builtin_call_fail_kind(int ml_flags)
{
    switch (ml_flags & (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O |
        METH_KEYWORDS | METH_METHOD)) {
        case METH_VARARGS:
            return SPEC_FAIL_CALL_CFUNC_VARARGS;
        case METH_VARARGS | METH_KEYWORDS:
            return SPEC_FAIL_CALL_CFUNC_VARARGS_KEYWORDS;
        case METH_NOARGS:
            return SPEC_FAIL_CALL_CFUNC_NOARGS;
        case METH_METHOD | METH_FASTCALL | METH_KEYWORDS:
            return SPEC_FAIL_CALL_CFUNC_METHOD_FASTCALL_KEYWORDS;
        /* These cases should be optimized, but return "other" just in case */
        case METH_O:
        case METH_FASTCALL:
        case METH_FASTCALL | METH_KEYWORDS:
            return SPEC_FAIL_OTHER;
        default:
            return SPEC_FAIL_CALL_BAD_CALL_FLAGS;
    }
}

static int
meth_descr_call_fail_kind(int ml_flags)
{
    switch (ml_flags & (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O |
                        METH_KEYWORDS | METH_METHOD)) {
        case METH_VARARGS:
            return SPEC_FAIL_CALL_METH_DESCR_VARARGS;
        case METH_VARARGS | METH_KEYWORDS:
            return SPEC_FAIL_CALL_METH_DESCR_VARARGS_KEYWORDS;
        case METH_METHOD | METH_FASTCALL | METH_KEYWORDS:
            return SPEC_FAIL_CALL_METH_DESCR_METHOD_FASTCALL_KEYWORDS;
            /* These cases should be optimized, but return "other" just in case */
        case METH_NOARGS:
        case METH_O:
        case METH_FASTCALL:
        case METH_FASTCALL | METH_KEYWORDS:
            return SPEC_FAIL_OTHER;
        default:
            return SPEC_FAIL_CALL_BAD_CALL_FLAGS;
    }
}
#endif   // Py_STATS

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
            instr->op.code = CALL_METHOD_DESCRIPTOR_NOARGS;
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
                instr->op.code = CALL_LIST_APPEND;
                return 0;
            }
            instr->op.code = CALL_METHOD_DESCRIPTOR_O;
            return 0;
        }
        case METH_FASTCALL: {
            instr->op.code = CALL_METHOD_DESCRIPTOR_FAST;
            return 0;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            instr->op.code = CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS;
            return 0;
        }
    }
    SPECIALIZATION_FAIL(CALL, meth_descr_call_fail_kind(descr->d_method->ml_flags));
    return -1;
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
    if (kind != SIMPLE_FUNCTION) {
        SPECIALIZATION_FAIL(CALL, kind);
        return -1;
    }
    int argcount = code->co_argcount;
    int defcount = func->func_defaults == NULL ? 0 : (int)PyTuple_GET_SIZE(func->func_defaults);
    int min_args = argcount-defcount;
    // GH-105840: min_args is negative when somebody sets too many __defaults__!
    if (min_args < 0 || nargs > argcount || nargs < min_args) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
        return -1;
    }
    assert(nargs <= argcount && nargs >= min_args);
    assert(min_args >= 0 && defcount >= 0);
    assert(defcount == 0 || func->func_defaults != NULL);
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (version == 0) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    write_u32(cache->func_version, version);
    if (argcount == nargs) {
        instr->op.code = bound_method ? CALL_BOUND_METHOD_EXACT_ARGS : CALL_PY_EXACT_ARGS;
    }
    else if (bound_method) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_BOUND_METHOD);
        return -1;
    }
    else {
        instr->op.code = CALL_PY_WITH_DEFAULTS;
    }
    return 0;
}

static int
specialize_c_call(PyObject *callable, _Py_CODEUNIT *instr, int nargs)
{
    if (PyCFunction_GET_FUNCTION(callable) == NULL) {
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
            if (callable == interp->callable_cache.len) {
                instr->op.code = CALL_LEN;
                return 0;
            }
            instr->op.code = CALL_BUILTIN_O;
            return 0;
        }
        case METH_FASTCALL: {
            if (nargs == 2) {
                /* isinstance(o1, o2) */
                PyInterpreterState *interp = _PyInterpreterState_GET();
                if (callable == interp->callable_cache.isinstance) {
                    instr->op.code = CALL_ISINSTANCE;
                    return 0;
                }
            }
            instr->op.code = CALL_BUILTIN_FAST;
            return 0;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            instr->op.code = CALL_BUILTIN_FAST_WITH_KEYWORDS;
            return 0;
        }
        default:
            SPECIALIZATION_FAIL(CALL,
                builtin_call_fail_kind(PyCFunction_GET_FLAGS(callable)));
            return 1;
    }
}

#ifdef Py_STATS
static int
call_fail_kind(PyObject *callable)
{
    assert(!PyCFunction_CheckExact(callable));
    assert(!PyFunction_Check(callable));
    assert(!PyType_Check(callable));
    assert(!Py_IS_TYPE(callable, &PyMethodDescr_Type));
    assert(!PyMethod_Check(callable));
    if (PyInstanceMethod_Check(callable)) {
        return SPEC_FAIL_CALL_INSTANCE_METHOD;
    }
    // builtin method
    else if (PyCMethod_Check(callable)) {
        return SPEC_FAIL_CALL_CMETHOD;
    }
    else if (Py_TYPE(callable) == &PyWrapperDescr_Type) {
        return SPEC_FAIL_CALL_OPERATOR_WRAPPER;
    }
    else if (Py_TYPE(callable) == &_PyMethodWrapper_Type) {
        return SPEC_FAIL_CALL_METHOD_WRAPPER;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS


void
_Py_Specialize_Call(PyObject *callable, _Py_CODEUNIT *instr, int nargs)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[CALL] == INLINE_CACHE_ENTRIES_CALL);
    assert(_Py_OPCODE(*instr) != INSTRUMENTED_CALL);
    _PyCallCache *cache = (_PyCallCache *)(instr + 1);
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
            fail = specialize_py_call((PyFunctionObject *)func, instr, nargs+1, true);
        }
        else {
            SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_BOUND_METHOD);
            fail = -1;
        }
    }
    else {
        SPECIALIZATION_FAIL(CALL, call_fail_kind(callable));
        fail = -1;
    }
    if (fail) {
        STAT_INC(CALL, failure);
        assert(!PyErr_Occurred());
        instr->op.code = CALL;
        cache->counter = adaptive_counter_backoff(cache->counter);
    }
    else {
        STAT_INC(CALL, success);
        assert(!PyErr_Occurred());
        cache->counter = adaptive_counter_cooldown();
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
            return SPEC_FAIL_BINARY_OP_XOR;
    }
    Py_UNREACHABLE();
}
#endif   // Py_STATS

void
_Py_Specialize_BinaryOp(PyObject *lhs, PyObject *rhs, _Py_CODEUNIT *instr,
                        int oparg, PyObject **locals)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[BINARY_OP] == INLINE_CACHE_ENTRIES_BINARY_OP);
    _PyBinaryOpCache *cache = (_PyBinaryOpCache *)(instr + 1);
    switch (oparg) {
        case NB_ADD:
        case NB_INPLACE_ADD:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyUnicode_CheckExact(lhs)) {
                _Py_CODEUNIT next = instr[INLINE_CACHE_ENTRIES_BINARY_OP + 1];
                bool to_store = (next.op.code == STORE_FAST);
                if (to_store && locals[next.op.arg] == lhs) {
                    instr->op.code = BINARY_OP_INPLACE_ADD_UNICODE;
                    goto success;
                }
                instr->op.code = BINARY_OP_ADD_UNICODE;
                goto success;
            }
            if (PyLong_CheckExact(lhs)) {
                instr->op.code = BINARY_OP_ADD_INT;
                goto success;
            }
            if (PyFloat_CheckExact(lhs)) {
                instr->op.code = BINARY_OP_ADD_FLOAT;
                goto success;
            }
            break;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyLong_CheckExact(lhs)) {
                instr->op.code = BINARY_OP_MULTIPLY_INT;
                goto success;
            }
            if (PyFloat_CheckExact(lhs)) {
                instr->op.code = BINARY_OP_MULTIPLY_FLOAT;
                goto success;
            }
            break;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            if (!Py_IS_TYPE(lhs, Py_TYPE(rhs))) {
                break;
            }
            if (PyLong_CheckExact(lhs)) {
                instr->op.code = BINARY_OP_SUBTRACT_INT;
                goto success;
            }
            if (PyFloat_CheckExact(lhs)) {
                instr->op.code = BINARY_OP_SUBTRACT_FLOAT;
                goto success;
            }
            break;
    }
    SPECIALIZATION_FAIL(BINARY_OP, binary_op_fail_kind(oparg, lhs, rhs));
    STAT_INC(BINARY_OP, failure);
    instr->op.code = BINARY_OP;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(BINARY_OP, success);
    cache->counter = adaptive_counter_cooldown();
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

void
_Py_Specialize_CompareOp(PyObject *lhs, PyObject *rhs, _Py_CODEUNIT *instr,
                         int oparg)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[COMPARE_OP] == INLINE_CACHE_ENTRIES_COMPARE_OP);
    // All of these specializations compute boolean values, so they're all valid
    // regardless of the fifth-lowest oparg bit.
    _PyCompareOpCache *cache = (_PyCompareOpCache *)(instr + 1);
    if (Py_TYPE(lhs) != Py_TYPE(rhs)) {
        SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs, rhs));
        goto failure;
    }
    if (PyFloat_CheckExact(lhs)) {
        instr->op.code = COMPARE_OP_FLOAT;
        goto success;
    }
    if (PyLong_CheckExact(lhs)) {
        if (_PyLong_IsCompact((PyLongObject *)lhs) && _PyLong_IsCompact((PyLongObject *)rhs)) {
            instr->op.code = COMPARE_OP_INT;
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
            instr->op.code = COMPARE_OP_STR;
            goto success;
        }
    }
    SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs, rhs));
failure:
    STAT_INC(COMPARE_OP, failure);
    instr->op.code = COMPARE_OP;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(COMPARE_OP, success);
    cache->counter = adaptive_counter_cooldown();
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

void
_Py_Specialize_UnpackSequence(PyObject *seq, _Py_CODEUNIT *instr, int oparg)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[UNPACK_SEQUENCE] ==
           INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
    _PyUnpackSequenceCache *cache = (_PyUnpackSequenceCache *)(instr + 1);
    if (PyTuple_CheckExact(seq)) {
        if (PyTuple_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            goto failure;
        }
        if (PyTuple_GET_SIZE(seq) == 2) {
            instr->op.code = UNPACK_SEQUENCE_TWO_TUPLE;
            goto success;
        }
        instr->op.code = UNPACK_SEQUENCE_TUPLE;
        goto success;
    }
    if (PyList_CheckExact(seq)) {
        if (PyList_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            goto failure;
        }
        instr->op.code = UNPACK_SEQUENCE_LIST;
        goto success;
    }
    SPECIALIZATION_FAIL(UNPACK_SEQUENCE, unpack_sequence_fail_kind(seq));
failure:
    STAT_INC(UNPACK_SEQUENCE, failure);
    instr->op.code = UNPACK_SEQUENCE;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(UNPACK_SEQUENCE, success);
    cache->counter = adaptive_counter_cooldown();
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

void
_Py_Specialize_ForIter(PyObject *iter, _Py_CODEUNIT *instr, int oparg)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[FOR_ITER] == INLINE_CACHE_ENTRIES_FOR_ITER);
    _PyForIterCache *cache = (_PyForIterCache *)(instr + 1);
    PyTypeObject *tp = Py_TYPE(iter);
    if (tp == &PyListIter_Type) {
        instr->op.code = FOR_ITER_LIST;
        goto success;
    }
    else if (tp == &PyTupleIter_Type) {
        instr->op.code = FOR_ITER_TUPLE;
        goto success;
    }
    else if (tp == &PyRangeIter_Type) {
        instr->op.code = FOR_ITER_RANGE;
        goto success;
    }
    else if (tp == &PyGen_Type && oparg <= SHRT_MAX) {
        assert(instr[oparg + INLINE_CACHE_ENTRIES_FOR_ITER + 1].op.code == END_FOR  ||
            instr[oparg + INLINE_CACHE_ENTRIES_FOR_ITER + 1].op.code == INSTRUMENTED_END_FOR
        );
        if (_PyInterpreterState_GET()->eval_frame) {
            SPECIALIZATION_FAIL(FOR_ITER, SPEC_FAIL_OTHER);
            goto failure;
        }
        instr->op.code = FOR_ITER_GEN;
        goto success;
    }
    SPECIALIZATION_FAIL(FOR_ITER,
                        _PySpecialization_ClassifyIterator(iter));
failure:
    STAT_INC(FOR_ITER, failure);
    instr->op.code = FOR_ITER;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(FOR_ITER, success);
    cache->counter = adaptive_counter_cooldown();
}

void
_Py_Specialize_Send(PyObject *receiver, _Py_CODEUNIT *instr)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[SEND] == INLINE_CACHE_ENTRIES_SEND);
    _PySendCache *cache = (_PySendCache *)(instr + 1);
    PyTypeObject *tp = Py_TYPE(receiver);
    if (tp == &PyGen_Type || tp == &PyCoro_Type) {
        if (_PyInterpreterState_GET()->eval_frame) {
            SPECIALIZATION_FAIL(SEND, SPEC_FAIL_OTHER);
            goto failure;
        }
        instr->op.code = SEND_GEN;
        goto success;
    }
    SPECIALIZATION_FAIL(SEND,
                        _PySpecialization_ClassifyIterator(receiver));
failure:
    STAT_INC(SEND, failure);
    instr->op.code = SEND;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(SEND, success);
    cache->counter = adaptive_counter_cooldown();
}

void
_Py_Specialize_ToBool(PyObject *value, _Py_CODEUNIT *instr)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[TO_BOOL] == INLINE_CACHE_ENTRIES_TO_BOOL);
    _PyToBoolCache *cache = (_PyToBoolCache *)(instr + 1);
    if (PyBool_Check(value)) {
        instr->op.code = TO_BOOL_BOOL;
        goto success;
    }
    if (PyLong_CheckExact(value)) {
        instr->op.code = TO_BOOL_INT;
        goto success;
    }
    if (PyList_CheckExact(value)) {
        instr->op.code = TO_BOOL_LIST;
        goto success;
    }
    if (Py_IsNone(value)) {
        instr->op.code = TO_BOOL_NONE;
        goto success;
    }
    if (PyUnicode_CheckExact(value)) {
        instr->op.code = TO_BOOL_STR;
        goto success;
    }
    if (PyType_HasFeature(Py_TYPE(value), Py_TPFLAGS_HEAPTYPE)) {
        PyNumberMethods *nb = Py_TYPE(value)->tp_as_number;
        if (nb && nb->nb_bool) {
            SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_NUMBER);
            goto failure;
        }
        PyMappingMethods *mp = Py_TYPE(value)->tp_as_mapping;
        if (mp && mp->mp_length) {
            SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_MAPPING);
            goto failure;
        }
        PySequenceMethods *sq = Py_TYPE(value)->tp_as_sequence;
        if (sq && sq->sq_length) {
            SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_SEQUENCE);
            goto failure;
        }
        if (!PyUnstable_Type_AssignVersionTag(Py_TYPE(value))) {
            SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_OUT_OF_VERSIONS);
            goto failure;
        }
        uint32_t version = Py_TYPE(value)->tp_version_tag;
        instr->op.code = TO_BOOL_ALWAYS_TRUE;
        write_u32(cache->version, version);
        assert(version);
        goto success;
    }
#ifdef Py_STATS
    if (PyByteArray_CheckExact(value)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_BYTEARRAY);
        goto failure;
    }
    if (PyBytes_CheckExact(value)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_BYTES);
        goto failure;
    }
    if (PyDict_CheckExact(value)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_DICT);
        goto failure;
    }
    if (PyFloat_CheckExact(value)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_FLOAT);
        goto failure;
    }
    if (PyMemoryView_Check(value)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_MEMORY_VIEW);
        goto failure;
    }
    if (PyAnySet_CheckExact(value)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_SET);
        goto failure;
    }
    if (PyTuple_CheckExact(value)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_TUPLE);
        goto failure;
    }
    SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_OTHER);
#endif   // Py_STATS
failure:
    STAT_INC(TO_BOOL, failure);
    instr->op.code = TO_BOOL;
    cache->counter = adaptive_counter_backoff(cache->counter);
    return;
success:
    STAT_INC(TO_BOOL, success);
    cache->counter = adaptive_counter_cooldown();
}

/* Code init cleanup.
 * CALL_ALLOC_AND_ENTER_INIT will set up
 * the frame to execute the EXIT_INIT_CHECK
 * instruction.
 * Ends with a RESUME so that it is not traced.
 * This is used as a plain code object, not a function,
 * so must not access globals or builtins.
 */

#define NO_LOC_4 (128 | (PY_CODE_LOCATION_INFO_NONE << 3) | 3)

static const PyBytesObject no_location = {
    PyVarObject_HEAD_INIT(&PyBytes_Type, 1)
    .ob_sval = { NO_LOC_4 }
};

const struct _PyCode_DEF(8) _Py_InitCleanup = {
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
    .co_code_adaptive = {
        EXIT_INIT_CHECK, 0,
        RETURN_VALUE, 0,
        RESUME, RESUME_AT_FUNC_START,
    }
};
