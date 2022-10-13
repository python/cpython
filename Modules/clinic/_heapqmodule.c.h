/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_heapq_heappush__doc__,
"heappush($module, heap, item, /)\n"
"--\n"
"\n"
"Push item onto heap, maintaining the heap invariant.");

#define _HEAPQ_HEAPPUSH_METHODDEF    \
    {"heappush", _PyCFunction_CAST(_heapq_heappush), METH_FASTCALL, _heapq_heappush__doc__},

static PyObject *
_heapq_heappush_impl(PyObject *module, PyObject *heap, PyObject *item);

static PyObject *
_heapq_heappush(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *heap;
    PyObject *item;

    if (!_PyArg_CheckPositional("heappush", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyList_Check(args[0])) {
        _PyArg_BadArgument("heappush", "argument 1", "list", args[0]);
        goto exit;
    }
    heap = args[0];
    item = args[1];
    return_value = _heapq_heappush_impl(module, heap, item);

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappop__doc__,
"heappop($module, heap, /)\n"
"--\n"
"\n"
"Pop the smallest item off the heap, maintaining the heap invariant.");

#define _HEAPQ_HEAPPOP_METHODDEF    \
    {"heappop", (PyCFunction)_heapq_heappop, METH_O, _heapq_heappop__doc__},

static PyObject *
_heapq_heappop_impl(PyObject *module, PyObject *heap);

static PyObject *
_heapq_heappop(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *heap;

    if (!PyList_Check(arg)) {
        _PyArg_BadArgument("heappop", "argument", "list", arg);
        goto exit;
    }
    heap = arg;
    return_value = _heapq_heappop_impl(module, heap);

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapreplace__doc__,
"heapreplace($module, heap, item, /)\n"
"--\n"
"\n"
"Pop and return the current smallest value, and add the new item.\n"
"\n"
"This is more efficient than heappop() followed by heappush(), and can be\n"
"more appropriate when using a fixed-size heap.  Note that the value\n"
"returned may be larger than item!  That constrains reasonable uses of\n"
"this routine unless written as part of a conditional replacement:\n"
"\n"
"    if item > heap[0]:\n"
"        item = heapreplace(heap, item)");

#define _HEAPQ_HEAPREPLACE_METHODDEF    \
    {"heapreplace", _PyCFunction_CAST(_heapq_heapreplace), METH_FASTCALL, _heapq_heapreplace__doc__},

static PyObject *
_heapq_heapreplace_impl(PyObject *module, PyObject *heap, PyObject *item);

static PyObject *
_heapq_heapreplace(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *heap;
    PyObject *item;

    if (!_PyArg_CheckPositional("heapreplace", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyList_Check(args[0])) {
        _PyArg_BadArgument("heapreplace", "argument 1", "list", args[0]);
        goto exit;
    }
    heap = args[0];
    item = args[1];
    return_value = _heapq_heapreplace_impl(module, heap, item);

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappushpop__doc__,
"heappushpop($module, heap, item, /)\n"
"--\n"
"\n"
"Push item on the heap, then pop and return the smallest item from the heap.\n"
"\n"
"The combined action runs more efficiently than heappush() followed by\n"
"a separate call to heappop().");

#define _HEAPQ_HEAPPUSHPOP_METHODDEF    \
    {"heappushpop", _PyCFunction_CAST(_heapq_heappushpop), METH_FASTCALL, _heapq_heappushpop__doc__},

static PyObject *
_heapq_heappushpop_impl(PyObject *module, PyObject *heap, PyObject *item);

static PyObject *
_heapq_heappushpop(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *heap;
    PyObject *item;

    if (!_PyArg_CheckPositional("heappushpop", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyList_Check(args[0])) {
        _PyArg_BadArgument("heappushpop", "argument 1", "list", args[0]);
        goto exit;
    }
    heap = args[0];
    item = args[1];
    return_value = _heapq_heappushpop_impl(module, heap, item);

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapify__doc__,
"heapify($module, heap, /)\n"
"--\n"
"\n"
"Transform list into a heap, in-place, in O(len(heap)) time.");

#define _HEAPQ_HEAPIFY_METHODDEF    \
    {"heapify", (PyCFunction)_heapq_heapify, METH_O, _heapq_heapify__doc__},

static PyObject *
_heapq_heapify_impl(PyObject *module, PyObject *heap);

static PyObject *
_heapq_heapify(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *heap;

    if (!PyList_Check(arg)) {
        _PyArg_BadArgument("heapify", "argument", "list", arg);
        goto exit;
    }
    heap = arg;
    return_value = _heapq_heapify_impl(module, heap);

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq__heappop_max__doc__,
"_heappop_max($module, heap, /)\n"
"--\n"
"\n"
"Maxheap variant of heappop.");

#define _HEAPQ__HEAPPOP_MAX_METHODDEF    \
    {"_heappop_max", (PyCFunction)_heapq__heappop_max, METH_O, _heapq__heappop_max__doc__},

static PyObject *
_heapq__heappop_max_impl(PyObject *module, PyObject *heap);

static PyObject *
_heapq__heappop_max(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *heap;

    if (!PyList_Check(arg)) {
        _PyArg_BadArgument("_heappop_max", "argument", "list", arg);
        goto exit;
    }
    heap = arg;
    return_value = _heapq__heappop_max_impl(module, heap);

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq__heapreplace_max__doc__,
"_heapreplace_max($module, heap, item, /)\n"
"--\n"
"\n"
"Maxheap variant of heapreplace.");

#define _HEAPQ__HEAPREPLACE_MAX_METHODDEF    \
    {"_heapreplace_max", _PyCFunction_CAST(_heapq__heapreplace_max), METH_FASTCALL, _heapq__heapreplace_max__doc__},

static PyObject *
_heapq__heapreplace_max_impl(PyObject *module, PyObject *heap,
                             PyObject *item);

static PyObject *
_heapq__heapreplace_max(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *heap;
    PyObject *item;

    if (!_PyArg_CheckPositional("_heapreplace_max", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyList_Check(args[0])) {
        _PyArg_BadArgument("_heapreplace_max", "argument 1", "list", args[0]);
        goto exit;
    }
    heap = args[0];
    item = args[1];
    return_value = _heapq__heapreplace_max_impl(module, heap, item);

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq__heapify_max__doc__,
"_heapify_max($module, heap, /)\n"
"--\n"
"\n"
"Maxheap variant of heapify.");

#define _HEAPQ__HEAPIFY_MAX_METHODDEF    \
    {"_heapify_max", (PyCFunction)_heapq__heapify_max, METH_O, _heapq__heapify_max__doc__},

static PyObject *
_heapq__heapify_max_impl(PyObject *module, PyObject *heap);

static PyObject *
_heapq__heapify_max(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *heap;

    if (!PyList_Check(arg)) {
        _PyArg_BadArgument("_heapify_max", "argument", "list", arg);
        goto exit;
    }
    heap = arg;
    return_value = _heapq__heapify_max_impl(module, heap);

exit:
    return return_value;
}
/*[clinic end generated code: output=29e99a48c57f82bb input=a9049054013a1b77]*/
