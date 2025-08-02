/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _Py_convert_optional_to_ssize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_FromArray()

PyDoc_STRVAR(gc_enable__doc__,
"enable($module, /)\n"
"--\n"
"\n"
"Enable automatic garbage collection.");

#define GC_ENABLE_METHODDEF    \
    {"enable", (PyCFunction)gc_enable, METH_NOARGS, gc_enable__doc__},

static PyObject *
gc_enable_impl(PyObject *module);

static PyObject *
gc_enable(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return gc_enable_impl(module);
}

PyDoc_STRVAR(gc_disable__doc__,
"disable($module, /)\n"
"--\n"
"\n"
"Disable automatic garbage collection.");

#define GC_DISABLE_METHODDEF    \
    {"disable", (PyCFunction)gc_disable, METH_NOARGS, gc_disable__doc__},

static PyObject *
gc_disable_impl(PyObject *module);

static PyObject *
gc_disable(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return gc_disable_impl(module);
}

PyDoc_STRVAR(gc_isenabled__doc__,
"isenabled($module, /)\n"
"--\n"
"\n"
"Returns true if automatic garbage collection is enabled.");

#define GC_ISENABLED_METHODDEF    \
    {"isenabled", (PyCFunction)gc_isenabled, METH_NOARGS, gc_isenabled__doc__},

static int
gc_isenabled_impl(PyObject *module);

static PyObject *
gc_isenabled(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = gc_isenabled_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_collect__doc__,
"collect($module, /, generation=2)\n"
"--\n"
"\n"
"Run the garbage collector.\n"
"\n"
"With no arguments, run a full collection.  The optional argument\n"
"may be an integer specifying which generation to collect.  A ValueError\n"
"is raised if the generation number is invalid.\n"
"\n"
"The number of unreachable objects is returned.");

#define GC_COLLECT_METHODDEF    \
    {"collect", _PyCFunction_CAST(gc_collect), METH_FASTCALL|METH_KEYWORDS, gc_collect__doc__},

static Py_ssize_t
gc_collect_impl(PyObject *module, int generation);

static PyObject *
gc_collect(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(generation), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"generation", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "collect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int generation = NUM_GENERATIONS - 1;
    Py_ssize_t _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    generation = PyLong_AsInt(args[0]);
    if (generation == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = gc_collect_impl(module, generation);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_set_debug__doc__,
"set_debug($module, flags, /)\n"
"--\n"
"\n"
"Set the garbage collection debugging flags.\n"
"\n"
"  flags\n"
"    An integer that can have the following bits turned on:\n"
"      DEBUG_STATS - Print statistics during collection.\n"
"      DEBUG_COLLECTABLE - Print collectable objects found.\n"
"      DEBUG_UNCOLLECTABLE - Print unreachable but uncollectable objects\n"
"        found.\n"
"      DEBUG_SAVEALL - Save objects to gc.garbage rather than freeing them.\n"
"      DEBUG_LEAK - Debug leaking programs (everything but STATS).\n"
"\n"
"Debugging information is written to sys.stderr.");

#define GC_SET_DEBUG_METHODDEF    \
    {"set_debug", (PyCFunction)gc_set_debug, METH_O, gc_set_debug__doc__},

static PyObject *
gc_set_debug_impl(PyObject *module, int flags);

static PyObject *
gc_set_debug(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int flags;

    flags = PyLong_AsInt(arg);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = gc_set_debug_impl(module, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_get_debug__doc__,
"get_debug($module, /)\n"
"--\n"
"\n"
"Get the garbage collection debugging flags.");

#define GC_GET_DEBUG_METHODDEF    \
    {"get_debug", (PyCFunction)gc_get_debug, METH_NOARGS, gc_get_debug__doc__},

static int
gc_get_debug_impl(PyObject *module);

static PyObject *
gc_get_debug(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = gc_get_debug_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_set_threshold__doc__,
"set_threshold(threshold0, [threshold1, [threshold2]])\n"
"Set the collection thresholds (the collection frequency).\n"
"\n"
"Setting \'threshold0\' to zero disables collection.");

#define GC_SET_THRESHOLD_METHODDEF    \
    {"set_threshold", (PyCFunction)gc_set_threshold, METH_VARARGS, gc_set_threshold__doc__},

static PyObject *
gc_set_threshold_impl(PyObject *module, int threshold0, int group_right_1,
                      int threshold1, int group_right_2, int threshold2);

static PyObject *
gc_set_threshold(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int threshold0;
    int group_right_1 = 0;
    int threshold1 = 0;
    int group_right_2 = 0;
    int threshold2 = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "i:set_threshold", &threshold0)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "ii:set_threshold", &threshold0, &threshold1)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "iii:set_threshold", &threshold0, &threshold1, &threshold2)) {
                goto exit;
            }
            group_right_1 = 1;
            group_right_2 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "gc.set_threshold requires 1 to 3 arguments");
            goto exit;
    }
    return_value = gc_set_threshold_impl(module, threshold0, group_right_1, threshold1, group_right_2, threshold2);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_get_threshold__doc__,
"get_threshold($module, /)\n"
"--\n"
"\n"
"Return the current collection thresholds.");

#define GC_GET_THRESHOLD_METHODDEF    \
    {"get_threshold", (PyCFunction)gc_get_threshold, METH_NOARGS, gc_get_threshold__doc__},

static PyObject *
gc_get_threshold_impl(PyObject *module);

static PyObject *
gc_get_threshold(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return gc_get_threshold_impl(module);
}

PyDoc_STRVAR(gc_get_count__doc__,
"get_count($module, /)\n"
"--\n"
"\n"
"Return a three-tuple of the current collection counts.");

#define GC_GET_COUNT_METHODDEF    \
    {"get_count", (PyCFunction)gc_get_count, METH_NOARGS, gc_get_count__doc__},

static PyObject *
gc_get_count_impl(PyObject *module);

static PyObject *
gc_get_count(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return gc_get_count_impl(module);
}

PyDoc_STRVAR(gc_get_referrers__doc__,
"get_referrers($module, /, *objs)\n"
"--\n"
"\n"
"Return the list of objects that directly refer to any of \'objs\'.");

#define GC_GET_REFERRERS_METHODDEF    \
    {"get_referrers", _PyCFunction_CAST(gc_get_referrers), METH_FASTCALL, gc_get_referrers__doc__},

static PyObject *
gc_get_referrers_impl(PyObject *module, PyObject *objs);

static PyObject *
gc_get_referrers(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *objs = NULL;

    objs = _PyTuple_FromArray(args, nargs);
    if (objs == NULL) {
        goto exit;
    }
    return_value = gc_get_referrers_impl(module, objs);

exit:
    /* Cleanup for objs */
    Py_XDECREF(objs);

    return return_value;
}

PyDoc_STRVAR(gc_get_referents__doc__,
"get_referents($module, /, *objs)\n"
"--\n"
"\n"
"Return the list of objects that are directly referred to by \'objs\'.");

#define GC_GET_REFERENTS_METHODDEF    \
    {"get_referents", _PyCFunction_CAST(gc_get_referents), METH_FASTCALL, gc_get_referents__doc__},

static PyObject *
gc_get_referents_impl(PyObject *module, PyObject *objs);

static PyObject *
gc_get_referents(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *objs = NULL;

    objs = _PyTuple_FromArray(args, nargs);
    if (objs == NULL) {
        goto exit;
    }
    return_value = gc_get_referents_impl(module, objs);

exit:
    /* Cleanup for objs */
    Py_XDECREF(objs);

    return return_value;
}

PyDoc_STRVAR(gc_get_objects__doc__,
"get_objects($module, /, generation=None)\n"
"--\n"
"\n"
"Return a list of objects tracked by the collector (excluding the list returned).\n"
"\n"
"  generation\n"
"    Generation to extract the objects from.\n"
"\n"
"If generation is not None, return only the objects tracked by the collector\n"
"that are in that generation.");

#define GC_GET_OBJECTS_METHODDEF    \
    {"get_objects", _PyCFunction_CAST(gc_get_objects), METH_FASTCALL|METH_KEYWORDS, gc_get_objects__doc__},

static PyObject *
gc_get_objects_impl(PyObject *module, Py_ssize_t generation);

static PyObject *
gc_get_objects(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(generation), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"generation", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_objects",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    Py_ssize_t generation = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &generation)) {
        goto exit;
    }
skip_optional_pos:
    return_value = gc_get_objects_impl(module, generation);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_get_stats__doc__,
"get_stats($module, /)\n"
"--\n"
"\n"
"Return a list of dictionaries containing per-generation statistics.");

#define GC_GET_STATS_METHODDEF    \
    {"get_stats", (PyCFunction)gc_get_stats, METH_NOARGS, gc_get_stats__doc__},

static PyObject *
gc_get_stats_impl(PyObject *module);

static PyObject *
gc_get_stats(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return gc_get_stats_impl(module);
}

PyDoc_STRVAR(gc_is_tracked__doc__,
"is_tracked($module, obj, /)\n"
"--\n"
"\n"
"Returns true if the object is tracked by the garbage collector.\n"
"\n"
"Simple atomic objects will return false.");

#define GC_IS_TRACKED_METHODDEF    \
    {"is_tracked", (PyCFunction)gc_is_tracked, METH_O, gc_is_tracked__doc__},

static int
gc_is_tracked_impl(PyObject *module, PyObject *obj);

static PyObject *
gc_is_tracked(PyObject *module, PyObject *obj)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = gc_is_tracked_impl(module, obj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_is_finalized__doc__,
"is_finalized($module, obj, /)\n"
"--\n"
"\n"
"Returns true if the object has been already finalized by the GC.");

#define GC_IS_FINALIZED_METHODDEF    \
    {"is_finalized", (PyCFunction)gc_is_finalized, METH_O, gc_is_finalized__doc__},

static int
gc_is_finalized_impl(PyObject *module, PyObject *obj);

static PyObject *
gc_is_finalized(PyObject *module, PyObject *obj)
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = gc_is_finalized_impl(module, obj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_freeze__doc__,
"freeze($module, /)\n"
"--\n"
"\n"
"Freeze all current tracked objects and ignore them for future collections.\n"
"\n"
"This can be used before a POSIX fork() call to make the gc copy-on-write friendly.\n"
"Note: collection before a POSIX fork() call may free pages for future allocation\n"
"which can cause copy-on-write.");

#define GC_FREEZE_METHODDEF    \
    {"freeze", (PyCFunction)gc_freeze, METH_NOARGS, gc_freeze__doc__},

static PyObject *
gc_freeze_impl(PyObject *module);

static PyObject *
gc_freeze(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return gc_freeze_impl(module);
}

PyDoc_STRVAR(gc_unfreeze__doc__,
"unfreeze($module, /)\n"
"--\n"
"\n"
"Unfreeze all objects in the permanent generation.\n"
"\n"
"Put all objects in the permanent generation back into oldest generation.");

#define GC_UNFREEZE_METHODDEF    \
    {"unfreeze", (PyCFunction)gc_unfreeze, METH_NOARGS, gc_unfreeze__doc__},

static PyObject *
gc_unfreeze_impl(PyObject *module);

static PyObject *
gc_unfreeze(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return gc_unfreeze_impl(module);
}

PyDoc_STRVAR(gc_get_freeze_count__doc__,
"get_freeze_count($module, /)\n"
"--\n"
"\n"
"Return the number of objects in the permanent generation.");

#define GC_GET_FREEZE_COUNT_METHODDEF    \
    {"get_freeze_count", (PyCFunction)gc_get_freeze_count, METH_NOARGS, gc_get_freeze_count__doc__},

static Py_ssize_t
gc_get_freeze_count_impl(PyObject *module);

static PyObject *
gc_get_freeze_count(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = gc_get_freeze_count_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=96d057eac558e6ca input=a9049054013a1b77]*/
