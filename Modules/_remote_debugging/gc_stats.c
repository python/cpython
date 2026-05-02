/******************************************************************************
 * Remote Debugging Module - GC Stats Functions
 *
 * This file contains functions for reading GC stats from interpreter state.
 ******************************************************************************/

#include "gc_stats.h"

typedef struct {
    PyObject *result;
    bool all_interpreters;
} GetGCStatsContext;

static int
read_gc_stats(struct gc_stats *stats, int64_t iid, PyObject *result)
{
#define ADD_LOCAL_ULONG(name) do { \
    PyObject *value = PyLong_FromUnsignedLong(name); \
    if (value == NULL) { \
        goto error; \
    } \
    int rc = PyDict_SetItemString(item, #name, value); \
    Py_DECREF(value); \
    if (rc < 0) { \
        goto error; \
    } \
} while (0)

#define ADD_LOCAL_INT64(name) do { \
    PyObject *value = PyLong_FromInt64(name); \
    if (value == NULL) { \
        goto error; \
    } \
    int rc = PyDict_SetItemString(item, #name, value); \
    Py_DECREF(value); \
    if (rc < 0) { \
        goto error; \
    } \
} while (0)

#define ADD_STATS_SSIZE(name) do { \
    PyObject *value = PyLong_FromSsize_t(stats_item->name); \
    if (value == NULL) { \
        goto error; \
    } \
    int rc = PyDict_SetItemString(item, #name, value); \
    Py_DECREF(value); \
    if (rc < 0) { \
        goto error; \
    } \
} while (0)

#define ADD_STATS_INT64(name) do { \
    PyObject *value = PyLong_FromInt64(stats_item->name); \
    if (value == NULL) { \
        goto error; \
    } \
    int rc = PyDict_SetItemString(item, #name, value); \
    Py_DECREF(value); \
    if (rc < 0) { \
        goto error; \
    } \
} while (0)

#define ADD_STATS_DOUBLE(name) do { \
    PyObject *value = PyFloat_FromDouble(stats_item->name); \
    if (value == NULL) { \
        goto error; \
    } \
    int rc = PyDict_SetItemString(item, #name, value); \
    Py_DECREF(value); \
    if (rc < 0) { \
        goto error; \
    } \
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
            struct gc_generation_stats *stats_item = items;
            item = PyDict_New();
            if (item == NULL) {
                goto error;
            }

            ADD_LOCAL_ULONG(gen);
            ADD_LOCAL_INT64(iid);

            ADD_STATS_INT64(ts_start);
            ADD_STATS_INT64(ts_stop);
            ADD_STATS_SSIZE(collections);
            ADD_STATS_SSIZE(collected);
            ADD_STATS_SSIZE(uncollectable);
            ADD_STATS_SSIZE(candidates);

            ADD_STATS_DOUBLE(duration);

            int rc = PyList_Append(result, item);
            Py_CLEAR(item);
            if (rc < 0) {
                goto error;
            }
        }
    }

#undef ADD_LOCAL_ULONG
#undef ADD_LOCAL_INT64
#undef ADD_STATS_SSIZE
#undef ADD_STATS_INT64
#undef ADD_STATS_DOUBLE

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
        PyErr_SetString(PyExc_RuntimeError, "Failed to read GC state address");
        return -1;
    }
    if (gc_stats_addr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "GC state address is NULL");
        return -1;
    }

    struct gc_stats stats;
    uint64_t gc_stats_size = offsets->debug_offsets.gc.generation_stats_size;
    if (gc_stats_size != sizeof(stats)) {
        PyErr_Format(PyExc_RuntimeError,
                     "Remote gc_stats size (%llu) does not match "
                     "local size (%zu)",
                     (unsigned long long)gc_stats_size, sizeof(stats));
        return -1;
    }
    if (_Py_RemoteDebug_ReadRemoteMemory(&offsets->handle,
                                         gc_stats_addr,
                                         gc_stats_size,
                                         &stats) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to read GC state");
        return -1;
    }

    if (read_gc_stats(&stats, iid, ctx->result) < 0) {
        return -1;
    }

    return 0;
}

PyObject *
get_gc_stats(RuntimeOffsets *offsets, bool all_interpreters)
{
    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }
    GetGCStatsContext ctx = {
        .result = result,
        .all_interpreters = all_interpreters,
    };
    if (iterate_interpreters(offsets, get_gc_stats_from_interpreter_state,
                             &ctx) < 0) {
        Py_CLEAR(result);
        return NULL;
    }

    return result;
}
