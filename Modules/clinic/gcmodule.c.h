/*[clinic input]
preserve
[clinic start generated code]*/

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
    {"collect", (PyCFunction)(void(*)(void))gc_collect, METH_FASTCALL|METH_KEYWORDS, gc_collect__doc__},

static Py_ssize_t
gc_collect_impl(PyObject *module, int generation);

static PyObject *
gc_collect(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"generation", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "collect", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int generation = NUM_GENERATIONS - 1;
    Py_ssize_t _return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    generation = _PyLong_AsInt(args[0]);
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

    flags = _PyLong_AsInt(arg);
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
    {"get_objects", (PyCFunction)(void(*)(void))gc_get_objects, METH_FASTCALL|METH_KEYWORDS, gc_get_objects__doc__},

static PyObject *
gc_get_objects_impl(PyObject *module, Py_ssize_t generation);

static PyObject *
gc_get_objects(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"generation", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "get_objects", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    Py_ssize_t generation = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
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

PyDoc_STRVAR(gc_is_finalized__doc__,
"is_finalized($module, obj, /)\n"
"--\n"
"\n"
"Returns true if the object has been already finalized by the GC.");

#define GC_IS_FINALIZED_METHODDEF    \
    {"is_finalized", (PyCFunction)gc_is_finalized, METH_O, gc_is_finalized__doc__},

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
/*[clinic end generated code: output=61e15f3a549f3ab5 input=a9049054013a1b77]*/
