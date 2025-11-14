#ifndef Py_INTERNAL_STATS_H
#define Py_INTERNAL_STATS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_structs.h"     //


#ifdef Py_STATS

#include "pycore_bitutils.h"  // _Py_bit_length

#define STAT_INC(opname, name) _Py_STATS_EXPR(opcode_stats[opname].specialization.name++)
#define STAT_DEC(opname, name) _Py_STATS_EXPR(opcode_stats[opname].specialization.name--)
#define OPCODE_EXE_INC(opname) _Py_STATS_EXPR(opcode_stats[opname].execution_count++)
#define CALL_STAT_INC(name) _Py_STATS_EXPR(call_stats.name++)
#define OBJECT_STAT_INC(name) _Py_STATS_EXPR(object_stats.name++)
#define OBJECT_STAT_INC_COND(name, cond) _Py_STATS_COND_EXPR(cond, object_stats.name++)
#define EVAL_CALL_STAT_INC(name) _Py_STATS_EXPR(call_stats.eval_calls[name]++)
#define EVAL_CALL_STAT_INC_IF_FUNCTION(name, callable) _Py_STATS_COND_EXPR(PyFunction_Check(callable), call_stats.eval_calls[name]++)
#define GC_STAT_ADD(gen, name, n) _Py_STATS_EXPR(gc_stats[(gen)].name += (n))
#define OPT_STAT_INC(name) _Py_STATS_EXPR(optimization_stats.name++)
#define OPT_STAT_ADD(name, n) _Py_STATS_EXPR(optimization_stats.name += (n))
#define UOP_STAT_INC(opname, name) \
    do { \
        PyStats *s = _PyStats_GET(); \
        if (s) { \
            assert(opname < 512); \
            s->optimization_stats.opcode[opname].name++; \
        } \
    } while (0)
#define UOP_PAIR_INC(uopcode, lastuop) \
    do { \
        PyStats *s = _PyStats_GET(); \
        if (lastuop && s) { \
            s->optimization_stats.opcode[lastuop].pair_count[uopcode]++; \
        } \
        lastuop = uopcode; \
    } while (0)
#define OPT_UNSUPPORTED_OPCODE(opname) _Py_STATS_EXPR(optimization_stats.unsupported_opcode[opname]++)
#define OPT_ERROR_IN_OPCODE(opname) _Py_STATS_EXPR(optimization_stats.error_in_opcode[opname]++)
#define OPT_HIST(length, name) \
    do { \
        PyStats *s = _PyStats_GET(); \
        if (s) { \
            int bucket = _Py_bit_length(length >= 1 ? length - 1 : 0); \
            bucket = (bucket >= _Py_UOP_HIST_SIZE) ? _Py_UOP_HIST_SIZE - 1 : bucket; \
            s->optimization_stats.name[bucket]++; \
        } \
    } while (0)
#define RARE_EVENT_STAT_INC(name) _Py_STATS_EXPR(rare_event_stats.name++)
#define OPCODE_DEFERRED_INC(opname) _Py_STATS_COND_EXPR(opcode==opname, opcode_stats[opname].specialization.deferred++)

#ifdef Py_GIL_DISABLED
#define FT_STAT_MUTEX_SLEEP_INC() _Py_STATS_EXPR(ft_stats.mutex_sleeps++)
#define FT_STAT_QSBR_POLL_INC() _Py_STATS_EXPR(ft_stats.qsbr_polls++)
#define FT_STAT_WORLD_STOP_INC() _Py_STATS_EXPR(ft_stats.world_stops++)
#else
#define FT_STAT_MUTEX_SLEEP_INC()
#define FT_STAT_QSBR_POLL_INC()
#define FT_STAT_WORLD_STOP_INC()
#endif

// Export for '_opcode' shared extension
PyAPI_FUNC(PyObject*) _Py_GetSpecializationStats(void);

#else
#define STAT_INC(opname, name) ((void)0)
#define STAT_DEC(opname, name) ((void)0)
#define OPCODE_EXE_INC(opname) ((void)0)
#define CALL_STAT_INC(name) ((void)0)
#define OBJECT_STAT_INC(name) ((void)0)
#define OBJECT_STAT_INC_COND(name, cond) ((void)0)
#define EVAL_CALL_STAT_INC(name) ((void)0)
#define EVAL_CALL_STAT_INC_IF_FUNCTION(name, callable) ((void)0)
#define GC_STAT_ADD(gen, name, n) ((void)0)
#define OPT_STAT_INC(name) ((void)0)
#define OPT_STAT_ADD(name, n) ((void)0)
#define UOP_STAT_INC(opname, name) ((void)0)
#define UOP_PAIR_INC(uopcode, lastuop) ((void)0)
#define OPT_UNSUPPORTED_OPCODE(opname) ((void)0)
#define OPT_ERROR_IN_OPCODE(opname) ((void)0)
#define OPT_HIST(length, name) ((void)0)
#define RARE_EVENT_STAT_INC(name) ((void)0)
#define OPCODE_DEFERRED_INC(opname) ((void)0)
#define FT_STAT_MUTEX_SLEEP_INC()
#define FT_STAT_QSBR_POLL_INC()
#define FT_STAT_WORLD_STOP_INC()
#endif  // !Py_STATS


#define RARE_EVENT_INTERP_INC(interp, name) \
    do { \
        /* saturating add */ \
        uint8_t val = FT_ATOMIC_LOAD_UINT8_RELAXED(interp->rare_events.name); \
        if (val < UINT8_MAX) { \
            FT_ATOMIC_STORE_UINT8(interp->rare_events.name, val + 1); \
        } \
        RARE_EVENT_STAT_INC(name); \
    } while (0); \

#define RARE_EVENT_INC(name) \
    do { \
        PyInterpreterState *interp = PyInterpreterState_Get(); \
        RARE_EVENT_INTERP_INC(interp, name); \
    } while (0); \

PyStatus _PyStats_InterpInit(PyInterpreterState *);
bool _PyStats_ThreadInit(PyInterpreterState *, _PyThreadStateImpl *);
void _PyStats_ThreadFini(_PyThreadStateImpl *);
void _PyStats_Attach(_PyThreadStateImpl *);
void _PyStats_Detach(_PyThreadStateImpl *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STATS_H */
