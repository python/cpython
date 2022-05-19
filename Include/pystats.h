

#ifndef Py_PYSTATS_H
#define Py_PYSTATS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef Py_STATS

#define SPECIALIZATION_FAILURE_KINDS 32

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
} ObjectStats;

typedef struct _stats {
    OpcodeStats opcode_stats[256];
    CallStats call_stats;
    ObjectStats object_stats;
} PyStats;

PyAPI_DATA(PyStats) _py_stats;

extern void _Py_PrintSpecializationStats(int to_file);

#ifdef _PY_INTERPRETER

#define _Py_INCREF_STAT_INC() _py_stats.object_stats.interpreter_increfs++
#define _Py_DECREF_STAT_INC()  _py_stats.object_stats.interpreter_decrefs++

#else

#define _Py_INCREF_STAT_INC() _py_stats.object_stats.increfs++
#define _Py_DECREF_STAT_INC()  _py_stats.object_stats.decrefs++

#endif

#else

#define _Py_INCREF_STAT_INC() ((void)0)
#define _Py_DECREF_STAT_INC() ((void)0)

#endif  // !Py_STATS

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATs_H */
