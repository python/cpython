/* _gilstats: GIL contention statistics (see Python/gil_stats.c). */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_gil_stats.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()

#include "clinic/_gilstatsmodule.c.h"

/*[clinic input]
module _gilstats
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a6f7945ecb77a84f]*/


static PyInterpreterState *
get_main_interp(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (!_Py_IsMainInterpreter(interp)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "GIL statistics are only available in the main "
                        "interpreter");
        return NULL;
    }
    return interp;
}


/*[clinic input]
_gilstats.enable

    *
    ring_size: unsigned_int(bitwise=False) = 65536
        Capacity of the event ring (a power of two).  Events that do not
        fit between two drains are dropped and counted in "dropped".
    record_threshold_us: unsigned_int(bitwise=False) = 100
        Contended waits shorter than this are only counted
        ("below_threshold"), not recorded individually.  0 records all.
    drain_interval_min: double = 0.01
    drain_interval_max: double = 2.0
        Bounds, in seconds, of the adaptive interval at which the
        aggregator thread drains the ring.

Start collecting GIL contention statistics.

If statistics are already enabled, they are restarted with the new
settings and the current window is discarded.
[clinic start generated code]*/

static PyObject *
_gilstats_enable_impl(PyObject *module, unsigned int ring_size,
                      unsigned int record_threshold_us,
                      double drain_interval_min, double drain_interval_max)
/*[clinic end generated code: output=3cc578fb248b2d1e input=ead64bea5cb790c4]*/
{
    PyInterpreterState *interp = get_main_interp();
    if (interp == NULL) {
        return NULL;
    }
    _PyGILStatsConfig cfg = {
        .ring_size = ring_size,
        .threshold_us = record_threshold_us,
        .drain_interval_min = drain_interval_min,
        .drain_interval_max = drain_interval_max,
    };
    if (_PyGILStats_Enable(interp, &cfg) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_gilstats.disable

Stop collecting GIL contention statistics.

Stops and joins the aggregator thread.  The callback and the last
published snapshot are kept.
[clinic start generated code]*/

static PyObject *
_gilstats_disable_impl(PyObject *module)
/*[clinic end generated code: output=6c9839b7b3904af1 input=ad3ab14cf8e778b9]*/
{
    PyInterpreterState *interp = get_main_interp();
    if (interp == NULL) {
        return NULL;
    }
    if (_PyGILStats_Disable(interp) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_gilstats.is_enabled -> bool

Return True if GIL contention statistics are being collected.
[clinic start generated code]*/

static int
_gilstats_is_enabled_impl(PyObject *module)
/*[clinic end generated code: output=163251e89705e537 input=eddb1ed15b162f03]*/
{
    PyInterpreterState *interp = get_main_interp();
    if (interp == NULL) {
        return -1;
    }
    return _PyGILStats_IsEnabled(interp);
}


/*[clinic input]
_gilstats.set_callback

    callback: object
        Called as callback(snapshot) from the aggregator thread, with the
        GIL held, each time a window is published.  None removes it.
    /
    period: double = 60.0
        Minimum window length in seconds.
    min_events: unsigned_long_long(bitwise=False) = 1000
        Minimum number of contended waits before a window is published.
    max_period: double = 600.0
        A window is published after this many seconds regardless of
        min_events.

Set the function called with each published window.

The window settings apply whether or not a callback is set; they also
control what snapshot() returns.
[clinic start generated code]*/

static PyObject *
_gilstats_set_callback_impl(PyObject *module, PyObject *callback,
                            double period, unsigned long long min_events,
                            double max_period)
/*[clinic end generated code: output=1ea90de4ce259cfd input=706caa7a64c69155]*/
{
    PyInterpreterState *interp = get_main_interp();
    if (interp == NULL) {
        return NULL;
    }
    if (_PyGILStats_SetCallback(interp, callback, period,
                                (uint64_t)min_events, max_period) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_gilstats.snapshot

Return the last published window as a dict, or None.
[clinic start generated code]*/

static PyObject *
_gilstats_snapshot_impl(PyObject *module)
/*[clinic end generated code: output=e4f3055a0b048e19 input=afeb3c94b4e093ba]*/
{
    PyInterpreterState *interp = get_main_interp();
    if (interp == NULL) {
        return NULL;
    }
    return _PyGILStats_Snapshot(interp);
}


/*[clinic input]
_gilstats._summarize

    waits: object
        Iterable of wait_us ints or (wait_us, waiters_at_entry) tuples.
    /
    below_threshold: unsigned_long_long(bitwise=False) = 0
    threshold_us: unsigned_int(bitwise=False) = 0

Summarize synthetic waits with the aggregator's code (for tests).
[clinic start generated code]*/

static PyObject *
_gilstats__summarize_impl(PyObject *module, PyObject *waits,
                          unsigned long long below_threshold,
                          unsigned int threshold_us)
/*[clinic end generated code: output=18f49a4dd81b2d27 input=1e7d2ad10a8cf0e1]*/
{
    return _PyGILStats_Summarize(waits, (uint64_t)below_threshold,
                                 (uint32_t)threshold_us);
}


static PyMethodDef gilstats_methods[] = {
    _GILSTATS_ENABLE_METHODDEF
    _GILSTATS_DISABLE_METHODDEF
    _GILSTATS_IS_ENABLED_METHODDEF
    _GILSTATS_SET_CALLBACK_METHODDEF
    _GILSTATS_SNAPSHOT_METHODDEF
    _GILSTATS__SUMMARIZE_METHODDEF
    {NULL, NULL}
};

static PyModuleDef_Slot gilstats_slots[] = {
    /* Functions other than _summarize() refuse to run outside the main
       interpreter, so no state is shared with other interpreters. */
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

PyDoc_STRVAR(gilstats_doc,
"GIL contention statistics.\n\
\n\
Collects how long threads wait for the GIL and how many wait at once,\n\
with a background thread aggregating the data into a log2 histogram.");

static struct PyModuleDef gilstats_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_gilstats",
    .m_doc = gilstats_doc,
    .m_size = 0,
    .m_methods = gilstats_methods,
    .m_slots = gilstats_slots,
};

PyMODINIT_FUNC
PyInit__gilstats(void)
{
    return PyModuleDef_Init(&gilstats_module);
}
