// Statistics on Python performance.
//
// API:
//
// - _Py_INCREF_STAT_INC() and _Py_DECREF_STAT_INC() used by Py_INCREF()
//   and Py_DECREF().
// - _Py_stats variable
//
// Functions of the sys module:
//
// - sys._stats_on()
// - sys._stats_off()
// - sys._stats_clear()
// - sys._stats_dump()
//
// Python must be built with ./configure --enable-pystats to define the
// Py_STATS macro.
//
// Define _PY_INTERPRETER macro to increment interpreter_increfs and
// interpreter_decrefs. Otherwise, increment increfs and decrefs.

#ifndef Py_CPYTHON_PYSTATS_H
#  error "this header file must not be included directly"
#endif

#define PYSTATS_MAX_UOP_ID 512

#define SPECIALIZATION_FAILURE_KINDS 36

/* Stats for determining who is calling PyEval_EvalFrame */
#define EVAL_CALL_TOTAL 0
#define EVAL_CALL_VECTOR 1
#define EVAL_CALL_GENERATOR 2
#define EVAL_CALL_LEGACY 3
#define EVAL_CALL_FUNCTION_VECTORCALL 4
#define EVAL_CALL_BUILD_CLASS 5
#define EVAL_CALL_SLOT 6
#define EVAL_CALL_FUNCTION_EX 7
#define EVAL_CALL_API 8
#define EVAL_CALL_METHOD 9

#define EVAL_CALL_KINDS 10

typedef struct _specialization_stats {
    uint64_t success;
    uint64_t failure;
    uint64_t hit;
    uint64_t deferred;
    uint64_t miss;
    uint64_t deopt;
    uint64_t failure_kinds[SPECIALIZATION_FAILURE_KINDS];
} SpecializationStats;

typedef struct _opcode_stats {
    SpecializationStats specialization;
    uint64_t execution_count;
    uint64_t pair_count[256];
} OpcodeStats;

typedef struct _call_stats {
    uint64_t inlined_py_calls;
    uint64_t pyeval_calls;
    uint64_t frames_pushed;
    uint64_t frame_objects_created;
    uint64_t eval_calls[EVAL_CALL_KINDS];
} CallStats;

typedef struct _object_stats {
    uint64_t increfs;
    uint64_t decrefs;
    uint64_t interpreter_increfs;
    uint64_t interpreter_decrefs;
    uint64_t allocations;
    uint64_t allocations512;
    uint64_t allocations4k;
    uint64_t allocations_big;
    uint64_t frees;
    uint64_t to_freelist;
    uint64_t from_freelist;
    uint64_t inline_values;
    uint64_t dict_materialized_on_request;
    uint64_t dict_materialized_new_key;
    uint64_t dict_materialized_too_big;
    uint64_t dict_materialized_str_subclass;
    uint64_t type_cache_hits;
    uint64_t type_cache_misses;
    uint64_t type_cache_dunder_hits;
    uint64_t type_cache_dunder_misses;
    uint64_t type_cache_collisions;
    /* Temporary value used during GC */
    uint64_t object_visits;
} ObjectStats;

typedef struct _gc_stats {
    uint64_t collections;
    uint64_t object_visits;
    uint64_t objects_collected;
} GCStats;

typedef struct _uop_stats {
    uint64_t execution_count;
    uint64_t miss;
    uint64_t pair_count[PYSTATS_MAX_UOP_ID + 1];
} UOpStats;

#define _Py_UOP_HIST_SIZE 32

typedef struct _optimization_stats {
    uint64_t attempts;
    uint64_t traces_created;
    uint64_t traces_executed;
    uint64_t uops_executed;
    uint64_t trace_stack_overflow;
    uint64_t trace_stack_underflow;
    uint64_t trace_too_long;
    uint64_t trace_too_short;
    uint64_t inner_loop;
    uint64_t recursive_call;
    uint64_t low_confidence;
    uint64_t executors_invalidated;
    UOpStats opcode[PYSTATS_MAX_UOP_ID + 1];
    uint64_t unsupported_opcode[256];
    uint64_t trace_length_hist[_Py_UOP_HIST_SIZE];
    uint64_t trace_run_length_hist[_Py_UOP_HIST_SIZE];
    uint64_t optimized_trace_length_hist[_Py_UOP_HIST_SIZE];
    uint64_t optimizer_attempts;
    uint64_t optimizer_successes;
    uint64_t optimizer_failure_reason_no_memory;
    uint64_t remove_globals_builtins_changed;
    uint64_t remove_globals_incorrect_keys;
    uint64_t error_in_opcode[PYSTATS_MAX_UOP_ID + 1];
} OptimizationStats;

typedef struct _rare_event_stats {
    /* Setting an object's class, obj.__class__ = ... */
    uint64_t set_class;
    /* Setting the bases of a class, cls.__bases__ = ... */
    uint64_t set_bases;
    /* Setting the PEP 523 frame eval function, _PyInterpreterState_SetFrameEvalFunc() */
    uint64_t set_eval_frame_func;
    /* Modifying the builtins,  __builtins__.__dict__[var] = ... */
    uint64_t builtin_dict;
    /* Modifying a function, e.g. func.__defaults__ = ..., etc. */
    uint64_t func_modification;
    /* Modifying a dict that is being watched */
    uint64_t watched_dict_modification;
    uint64_t watched_globals_modification;
} RareEventStats;

typedef struct _stats {
    OpcodeStats opcode_stats[256];
    CallStats call_stats;
    ObjectStats object_stats;
    OptimizationStats optimization_stats;
    RareEventStats rare_event_stats;
    GCStats *gc_stats;
} PyStats;


// Export for shared extensions like 'math'
PyAPI_DATA(PyStats*) _Py_stats;

#ifdef _PY_INTERPRETER
#  define _Py_INCREF_STAT_INC() do { if (_Py_stats) _Py_stats->object_stats.interpreter_increfs++; } while (0)
#  define _Py_DECREF_STAT_INC() do { if (_Py_stats) _Py_stats->object_stats.interpreter_decrefs++; } while (0)
#else
#  define _Py_INCREF_STAT_INC() do { if (_Py_stats) _Py_stats->object_stats.increfs++; } while (0)
#  define _Py_DECREF_STAT_INC() do { if (_Py_stats) _Py_stats->object_stats.decrefs++; } while (0)
#endif
