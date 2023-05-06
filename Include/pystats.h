

#ifndef Py_PYSTATS_H
#define Py_PYSTATS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef Py_STATS

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
    uint64_t new_values;
    uint64_t dict_materialized_on_request;
    uint64_t dict_materialized_new_key;
    uint64_t dict_materialized_too_big;
    uint64_t dict_materialized_str_subclass;
    uint64_t type_cache_hits;
    uint64_t type_cache_misses;
    uint64_t type_cache_dunder_hits;
    uint64_t type_cache_dunder_misses;
    uint64_t type_cache_collisions;
} ObjectStats;

typedef struct _stats {
    OpcodeStats opcode_stats[256];
    CallStats call_stats;
    ObjectStats object_stats;
} PyStats;


PyAPI_DATA(PyStats) _py_stats_struct;
PyAPI_DATA(PyStats *) _py_stats;

extern void _Py_StatsClear(void);
extern void _Py_PrintSpecializationStats(int to_file);

#ifdef _PY_INTERPRETER

#define _Py_INCREF_STAT_INC() do { if (_py_stats) _py_stats->object_stats.interpreter_increfs++; } while (0)
#define _Py_DECREF_STAT_INC() do { if (_py_stats) _py_stats->object_stats.interpreter_decrefs++; } while (0)

#else

#define _Py_INCREF_STAT_INC() do { if (_py_stats) _py_stats->object_stats.increfs++; } while (0)
#define _Py_DECREF_STAT_INC() do { if (_py_stats) _py_stats->object_stats.decrefs++; } while (0)

#endif

#else

#define _Py_INCREF_STAT_INC() ((void)0)
#define _Py_DECREF_STAT_INC() ((void)0)

#endif  // !Py_STATS

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATs_H */
