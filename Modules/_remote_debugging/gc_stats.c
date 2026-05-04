/******************************************************************************
 * Remote Debugging Module - GC Stats Functions
 *
 * This file contains functions for reading GC stats from interpreter state.
 ******************************************************************************/

#include "gc_stats.h"

typedef struct {
    PyObject *result;
    PyTypeObject *gc_stats_info_type;
    bool all_interpreters;
} GetGCStatsContext;

static int
read_gc_stats(struct gc_stats *stats, int64_t iid, PyObject *result,
              PyTypeObject *gc_stats_info_type)
{
#define SET_FIELD(converter, expr) do { \
    PyObject *value = converter(expr); \
    if (value == NULL) { \
        goto error; \
    } \
    PyStructSequence_SetItem(item, field++, value); \
} while (0)

    PyObject *item = NULL;

    for (unsigned long gen = 0; gen < NUM_GENERATIONS; gen++) {
        struct gc_generation_stats *items;
        int size;
        if (gen == 0) {
            items = (struct gc_generation_stats *)stats->young.items;
            size = GC_YOUNG_STATS_SIZE;
        }
        else {
            items = (struct gc_generation_stats *)stats->old[gen-1].items;
            size = GC_OLD_STATS_SIZE;
        }
        for (int i = 0; i < size; i++, items++) {
            item = PyStructSequence_New(gc_stats_info_type);
            if (item == NULL) {
                goto error;
            }
            Py_ssize_t field = 0;

            SET_FIELD(PyLong_FromUnsignedLong, gen);
            SET_FIELD(PyLong_FromInt64, iid);

            SET_FIELD(PyLong_FromInt64, items->ts_start);
            SET_FIELD(PyLong_FromInt64, items->ts_stop);
            SET_FIELD(PyLong_FromSsize_t, items->collections);
            SET_FIELD(PyLong_FromSsize_t, items->collected);
            SET_FIELD(PyLong_FromSsize_t, items->uncollectable);
            SET_FIELD(PyLong_FromSsize_t, items->candidates);

            SET_FIELD(PyFloat_FromDouble, items->duration);

            int rc = PyList_Append(result, item);
            Py_CLEAR(item);
            if (rc < 0) {
                goto error;
            }
        }
    }

#undef SET_FIELD

    return 0;

error:
    Py_XDECREF(item);

    return -1;
}

static int
get_gc_stats_from_interpreter_state(RuntimeOffsets *offsets,
                                    uintptr_t interpreter_state_addr,
                                    int64_t iid,
                                    void *context)
{
    GetGCStatsContext *ctx = (GetGCStatsContext *)context;
    if (!ctx->all_interpreters && iid > 0) {
        return 0;
    }

    uintptr_t gc_stats_addr = 0;
    uintptr_t gc_stats_pointer_address = interpreter_state_addr
        + offsets->debug_offsets.interpreter_state.gc
        + offsets->debug_offsets.gc.generation_stats;
    if (_Py_RemoteDebug_ReadRemoteMemory(&offsets->handle,
                                         gc_stats_pointer_address,
                                         sizeof(gc_stats_addr),
                                         &gc_stats_addr) < 0) {
        set_exception_cause(offsets, PyExc_RuntimeError, "Failed to read GC state address");
        return -1;
    }
    if (gc_stats_addr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "GC state address is NULL");
        return -1;
    }

    struct gc_stats stats;
    if (_Py_RemoteDebug_ReadRemoteMemory(&offsets->handle,
                                         gc_stats_addr,
                                         sizeof(stats),
                                         &stats) < 0) {
        set_exception_cause(offsets, PyExc_RuntimeError, "Failed to read GC state");
        return -1;
    }

    if (read_gc_stats(&stats, iid, ctx->result,
                      ctx->gc_stats_info_type) < 0) {
        set_exception_cause(offsets, PyExc_RuntimeError, "Failed to populate GC stats result");
        return -1;
    }

    return 0;
}

PyObject *
get_gc_stats(RuntimeOffsets *offsets, bool all_interpreters,
             PyTypeObject *gc_stats_info_type)
{
    uint64_t gc_stats_size = offsets->debug_offsets.gc.generation_stats_size;
    if (gc_stats_size != sizeof(struct gc_stats)) {
        PyErr_Format(PyExc_RuntimeError,
                     "Remote gc_stats size (%llu) does not match "
                     "local size (%zu)",
                     (unsigned long long)gc_stats_size,
                     sizeof(struct gc_stats));
        set_exception_cause(offsets, PyExc_RuntimeError, "Remote gc_stats size mismatch");
        return NULL;
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }
    GetGCStatsContext ctx = {
        .result = result,
        .gc_stats_info_type = gc_stats_info_type,
        .all_interpreters = all_interpreters,
    };
    if (iterate_interpreters(offsets, get_gc_stats_from_interpreter_state,
                             &ctx) < 0) {
        Py_CLEAR(result);
        return NULL;
    }

    return result;
}
