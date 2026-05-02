/******************************************************************************
 * Remote Debugging Module - GC Stats Functions
 *
 * This file contains declarations for reading GC stats from interpreter state.
 ******************************************************************************/

#ifndef Py_REMOTE_DEBUGGING_GC_STATS_H
#define Py_REMOTE_DEBUGGING_GC_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "_remote_debugging.h"

PyObject *
get_gc_stats(RuntimeOffsets *offsets, bool all_interpreters,
             PyTypeObject *gc_stats_info_type);

#ifdef __cplusplus
}
#endif

#endif  /* Py_REMOTE_DEBUGGING_GC_STATS_H */
