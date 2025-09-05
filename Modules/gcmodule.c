/*
 * Python interface to the garbage collector.
 *
 * See Python/gc.c for the implementation of the garbage collector.
 */

#include "Python.h"
#include "pycore_gc.h"
#include "pycore_object.h"      // _PyObject_IS_GC()
#include "pycore_pystate.h"     // _PyInterpreterState_GET()
#include "pycore_tuple.h"       // _PyTuple_FromArray()

typedef struct _gc_runtime_state GCState;

static GCState *
get_gc_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->gc;
}

/*[clinic input]
module gc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b5c9690ecc842d79]*/
#include "clinic/gcmodule.c.h"

/*[clinic input]
gc.enable

Enable automatic garbage collection.
[clinic start generated code]*/

static PyObject *
gc_enable_impl(PyObject *module)
/*[clinic end generated code: output=45a427e9dce9155c input=81ac4940ca579707]*/
{
    PyGC_Enable();
    Py_RETURN_NONE;
}

/*[clinic input]
gc.disable

Disable automatic garbage collection.
[clinic start generated code]*/

static PyObject *
gc_disable_impl(PyObject *module)
/*[clinic end generated code: output=97d1030f7aa9d279 input=8c2e5a14e800d83b]*/
{
    PyGC_Disable();
    Py_RETURN_NONE;
}

/*[clinic input]
gc.isenabled -> bool

Returns true if automatic garbage collection is enabled.
[clinic start generated code]*/

static int
gc_isenabled_impl(PyObject *module)
/*[clinic end generated code: output=1874298331c49130 input=30005e0422373b31]*/
{
    return PyGC_IsEnabled();
}

/*[clinic input]
gc.collect -> Py_ssize_t

    generation: int(c_default="NUM_GENERATIONS - 1") = 2

Run the garbage collector.

With no arguments, run a full collection.  The optional argument
may be an integer specifying which generation to collect.  A ValueError
is raised if the generation number is invalid.

The number of unreachable objects is returned.
[clinic start generated code]*/

static Py_ssize_t
gc_collect_impl(PyObject *module, int generation)
/*[clinic end generated code: output=b697e633043233c7 input=40720128b682d879]*/
{
    PyThreadState *tstate = _PyThreadState_GET();

    if (generation < 0 || generation >= NUM_GENERATIONS) {
        _PyErr_SetString(tstate, PyExc_ValueError, "invalid generation");
        return -1;
    }

    return _PyGC_Collect(tstate, generation, _Py_GC_REASON_MANUAL);
}

/*[clinic input]
gc.set_debug

    flags: int
        An integer that can have the following bits turned on:
          DEBUG_STATS - Print statistics during collection.
          DEBUG_COLLECTABLE - Print collectable objects found.
          DEBUG_UNCOLLECTABLE - Print unreachable but uncollectable objects
            found.
          DEBUG_SAVEALL - Save objects to gc.garbage rather than freeing them.
          DEBUG_LEAK - Debug leaking programs (everything but STATS).
    /

Set the garbage collection debugging flags.

Debugging information is written to sys.stderr.
[clinic start generated code]*/

static PyObject *
gc_set_debug_impl(PyObject *module, int flags)
/*[clinic end generated code: output=7c8366575486b228 input=5e5ce15e84fbed15]*/
{
    GCState *gcstate = get_gc_state();
    gcstate->debug = flags;
    Py_RETURN_NONE;
}

/*[clinic input]
gc.get_debug -> int

Get the garbage collection debugging flags.
[clinic start generated code]*/

static int
gc_get_debug_impl(PyObject *module)
/*[clinic end generated code: output=91242f3506cd1e50 input=91a101e1c3b98366]*/
{
    GCState *gcstate = get_gc_state();
    return gcstate->debug;
}

/*[clinic input]
gc.set_threshold

    threshold0: int
    [
    threshold1: int
    [
    threshold2: int
    ]
    ]
    /

Set the collection thresholds (the collection frequency).

Setting 'threshold0' to zero disables collection.
[clinic start generated code]*/

static PyObject *
gc_set_threshold_impl(PyObject *module, int threshold0, int group_right_1,
                      int threshold1, int group_right_2, int threshold2)
/*[clinic end generated code: output=2e3c7c7dd59060f3 input=0d9612db50984eec]*/
{
    GCState *gcstate = get_gc_state();

    gcstate->young.threshold = threshold0;
    if (group_right_1) {
        gcstate->old[0].threshold = threshold1;
    }
    if (group_right_2) {
        gcstate->old[1].threshold = threshold2;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
gc.get_threshold

Return the current collection thresholds.
[clinic start generated code]*/

static PyObject *
gc_get_threshold_impl(PyObject *module)
/*[clinic end generated code: output=7902bc9f41ecbbd8 input=286d79918034d6e6]*/
{
    GCState *gcstate = get_gc_state();
    return Py_BuildValue("(iii)",
                         gcstate->young.threshold,
                         gcstate->old[0].threshold,
                         0);
}

/*[clinic input]
gc.get_count

Return a three-tuple of the current collection counts.
[clinic start generated code]*/

static PyObject *
gc_get_count_impl(PyObject *module)
/*[clinic end generated code: output=354012e67b16398f input=a392794a08251751]*/
{
    GCState *gcstate = get_gc_state();

#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    struct _gc_thread_state *gc = &tstate->gc;

    // Flush the local allocation count to the global count
    _Py_atomic_add_int(&gcstate->young.count, (int)gc->alloc_count);
    gc->alloc_count = 0;
#endif

    return Py_BuildValue("(iii)",
                         gcstate->young.count,
                         gcstate->old[gcstate->visited_space].count,
                         gcstate->old[gcstate->visited_space^1].count);
}

/*[clinic input]
gc.get_referrers

    *objs: tuple

Return the list of objects that directly refer to any of 'objs'.
[clinic start generated code]*/

static PyObject *
gc_get_referrers_impl(PyObject *module, PyObject *objs)
/*[clinic end generated code: output=929d6dff26f609b9 input=9102be7ebee69ee3]*/
{
    if (PySys_Audit("gc.get_referrers", "(O)", objs) < 0) {
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    return _PyGC_GetReferrers(interp, objs);
}

/* Append obj to list; return true if error (out of memory), false if OK. */
static int
referentsvisit(PyObject *obj, void *arg)
{
    PyObject *list = arg;
    return PyList_Append(list, obj) < 0;
}

static int
append_referrents(PyObject *result, PyObject *args)
{
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(args); i++) {
        PyObject *obj = PyTuple_GET_ITEM(args, i);
        if (!_PyObject_IS_GC(obj)) {
            continue;
        }

        traverseproc traverse = Py_TYPE(obj)->tp_traverse;
        if (!traverse) {
            continue;
        }
        if (traverse(obj, referentsvisit, result)) {
            return -1;
        }
    }
    return 0;
}

/*[clinic input]
gc.get_referents

    *objs: tuple

Return the list of objects that are directly referred to by 'objs'.
[clinic start generated code]*/

static PyObject *
gc_get_referents_impl(PyObject *module, PyObject *objs)
/*[clinic end generated code: output=6dfde40cd1588e1d input=55c078a6d0248fe0]*/
{
    if (PySys_Audit("gc.get_referents", "(O)", objs) < 0) {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *result = PyList_New(0);

    if (result == NULL) {
        return NULL;
    }

    // NOTE: stop the world is a no-op in default build
    _PyEval_StopTheWorld(interp);
    int err = append_referrents(result, objs);
    _PyEval_StartTheWorld(interp);

    if (err < 0) {
        Py_CLEAR(result);
    }

    return result;
}

/*[clinic input]
@permit_long_summary
gc.get_objects
    generation: Py_ssize_t(accept={int, NoneType}, c_default="-1") = None
        Generation to extract the objects from.

Return a list of objects tracked by the collector (excluding the list returned).

If generation is not None, return only the objects tracked by the collector
that are in that generation.
[clinic start generated code]*/

static PyObject *
gc_get_objects_impl(PyObject *module, Py_ssize_t generation)
/*[clinic end generated code: output=48b35fea4ba6cb0e input=a887f1d9924be7cf]*/
{
    if (PySys_Audit("gc.get_objects", "n", generation) < 0) {
        return NULL;
    }

    if (generation >= NUM_GENERATIONS) {
        return PyErr_Format(PyExc_ValueError,
                            "generation parameter must be less than the number of "
                            "available generations (%i)",
                            NUM_GENERATIONS);
    }

    if (generation < -1) {
        PyErr_SetString(PyExc_ValueError,
                        "generation parameter cannot be negative");
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    return _PyGC_GetObjects(interp, (int)generation);
}

/*[clinic input]
gc.get_stats

Return a list of dictionaries containing per-generation statistics.
[clinic start generated code]*/

static PyObject *
gc_get_stats_impl(PyObject *module)
/*[clinic end generated code: output=a8ab1d8a5d26f3ab input=1ef4ed9d17b1a470]*/
{
    int i;
    struct gc_generation_stats stats[NUM_GENERATIONS], *st;

    /* To get consistent values despite allocations while constructing
       the result list, we use a snapshot of the running stats. */
    GCState *gcstate = get_gc_state();
    for (i = 0; i < NUM_GENERATIONS; i++) {
        stats[i] = gcstate->generation_stats[i];
    }

    PyObject *result = PyList_New(0);
    if (result == NULL)
        return NULL;

    for (i = 0; i < NUM_GENERATIONS; i++) {
        PyObject *dict;
        st = &stats[i];
        dict = Py_BuildValue("{snsnsn}",
                             "collections", st->collections,
                             "collected", st->collected,
                             "uncollectable", st->uncollectable
                            );
        if (dict == NULL)
            goto error;
        if (PyList_Append(result, dict)) {
            Py_DECREF(dict);
            goto error;
        }
        Py_DECREF(dict);
    }
    return result;

error:
    Py_XDECREF(result);
    return NULL;
}


/*[clinic input]
gc.is_tracked -> bool

    obj: object
    /

Returns true if the object is tracked by the garbage collector.

Simple atomic objects will return false.
[clinic start generated code]*/

static int
gc_is_tracked_impl(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=91c8d086b7f47a33 input=423b98ec680c3126]*/
{
    return PyObject_GC_IsTracked(obj);
}

/*[clinic input]
gc.is_finalized -> bool

    obj: object
    /

Returns true if the object has been already finalized by the GC.
[clinic start generated code]*/

static int
gc_is_finalized_impl(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=401ff5d6fc660429 input=ca4d111c8f8c4e3a]*/
{
    return PyObject_GC_IsFinalized(obj);
}

/*[clinic input]
@permit_long_docstring_body
gc.freeze

Freeze all current tracked objects and ignore them for future collections.

This can be used before a POSIX fork() call to make the gc copy-on-write friendly.
Note: collection before a POSIX fork() call may free pages for future allocation
which can cause copy-on-write.
[clinic start generated code]*/

static PyObject *
gc_freeze_impl(PyObject *module)
/*[clinic end generated code: output=502159d9cdc4c139 input=11fb59b0a75dcf3d]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyGC_Freeze(interp);
    Py_RETURN_NONE;
}

/*[clinic input]
gc.unfreeze

Unfreeze all objects in the permanent generation.

Put all objects in the permanent generation back into oldest generation.
[clinic start generated code]*/

static PyObject *
gc_unfreeze_impl(PyObject *module)
/*[clinic end generated code: output=1c15f2043b25e169 input=2dd52b170f4cef6c]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyGC_Unfreeze(interp);
    Py_RETURN_NONE;
}

/*[clinic input]
gc.get_freeze_count -> Py_ssize_t

Return the number of objects in the permanent generation.
[clinic start generated code]*/

static Py_ssize_t
gc_get_freeze_count_impl(PyObject *module)
/*[clinic end generated code: output=61cbd9f43aa032e1 input=45ffbc65cfe2a6ed]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return _PyGC_GetFreezeCount(interp);
}


PyDoc_STRVAR(gc__doc__,
"This module provides access to the garbage collector for reference cycles.\n"
"\n"
"enable() -- Enable automatic garbage collection.\n"
"disable() -- Disable automatic garbage collection.\n"
"isenabled() -- Returns true if automatic collection is enabled.\n"
"collect() -- Do a full collection right now.\n"
"get_count() -- Return the current collection counts.\n"
"get_stats() -- Return list of dictionaries containing per-generation stats.\n"
"set_debug() -- Set debugging flags.\n"
"get_debug() -- Get debugging flags.\n"
"set_threshold() -- Set the collection thresholds.\n"
"get_threshold() -- Return the current the collection thresholds.\n"
"get_objects() -- Return a list of all objects tracked by the collector.\n"
"is_tracked() -- Returns true if a given object is tracked.\n"
"is_finalized() -- Returns true if a given object has been already finalized.\n"
"get_referrers() -- Return the list of objects that refer to an object.\n"
"get_referents() -- Return the list of objects that an object refers to.\n"
"freeze() -- Freeze all tracked objects and ignore them for future collections.\n"
"unfreeze() -- Unfreeze all objects in the permanent generation.\n"
"get_freeze_count() -- Return the number of objects in the permanent generation.\n");

static PyMethodDef GcMethods[] = {
    GC_ENABLE_METHODDEF
    GC_DISABLE_METHODDEF
    GC_ISENABLED_METHODDEF
    GC_SET_DEBUG_METHODDEF
    GC_GET_DEBUG_METHODDEF
    GC_GET_COUNT_METHODDEF
    GC_SET_THRESHOLD_METHODDEF
    GC_GET_THRESHOLD_METHODDEF
    GC_COLLECT_METHODDEF
    GC_GET_OBJECTS_METHODDEF
    GC_GET_STATS_METHODDEF
    GC_IS_TRACKED_METHODDEF
    GC_IS_FINALIZED_METHODDEF
    GC_GET_REFERRERS_METHODDEF
    GC_GET_REFERENTS_METHODDEF
    GC_FREEZE_METHODDEF
    GC_UNFREEZE_METHODDEF
    GC_GET_FREEZE_COUNT_METHODDEF
    {NULL,      NULL}           /* Sentinel */
};

static int
gcmodule_exec(PyObject *module)
{
    GCState *gcstate = get_gc_state();

    /* garbage and callbacks are initialized by _PyGC_Init() early in
     * interpreter lifecycle. */
    assert(gcstate->garbage != NULL);
    if (PyModule_AddObjectRef(module, "garbage", gcstate->garbage) < 0) {
        return -1;
    }
    assert(gcstate->callbacks != NULL);
    if (PyModule_AddObjectRef(module, "callbacks", gcstate->callbacks) < 0) {
        return -1;
    }

#define ADD_INT(NAME) if (PyModule_AddIntConstant(module, #NAME, _PyGC_ ## NAME) < 0) { return -1; }
    ADD_INT(DEBUG_STATS);
    ADD_INT(DEBUG_COLLECTABLE);
    ADD_INT(DEBUG_UNCOLLECTABLE);
    ADD_INT(DEBUG_SAVEALL);
    ADD_INT(DEBUG_LEAK);
#undef ADD_INT
    return 0;
}

static PyModuleDef_Slot gcmodule_slots[] = {
    {Py_mod_exec, gcmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef gcmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "gc",
    .m_doc = gc__doc__,
    .m_size = 0,  // per interpreter state, see: get_gc_state()
    .m_methods = GcMethods,
    .m_slots = gcmodule_slots
};

PyMODINIT_FUNC
PyInit_gc(void)
{
    return PyModuleDef_Init(&gcmodule);
}
