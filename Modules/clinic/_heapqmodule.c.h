/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

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
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heappush_impl(module, heap, item);
    Py_END_CRITICAL_SECTION();

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
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heappop_impl(module, heap);
    Py_END_CRITICAL_SECTION();

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
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heapreplace_impl(module, heap, item);
    Py_END_CRITICAL_SECTION();

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
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heappushpop_impl(module, heap, item);
    Py_END_CRITICAL_SECTION();

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
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heapify_impl(module, heap);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappush_max__doc__,
"heappush_max($module, heap, item, /)\n"
"--\n"
"\n"
"Push item onto max heap, maintaining the heap invariant.");

#define _HEAPQ_HEAPPUSH_MAX_METHODDEF    \
    {"heappush_max", _PyCFunction_CAST(_heapq_heappush_max), METH_FASTCALL, _heapq_heappush_max__doc__},

static PyObject *
_heapq_heappush_max_impl(PyObject *module, PyObject *heap, PyObject *item);

static PyObject *
_heapq_heappush_max(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *heap;
    PyObject *item;

    if (!_PyArg_CheckPositional("heappush_max", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyList_Check(args[0])) {
        _PyArg_BadArgument("heappush_max", "argument 1", "list", args[0]);
        goto exit;
    }
    heap = args[0];
    item = args[1];
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heappush_max_impl(module, heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappop_max__doc__,
"heappop_max($module, heap, /)\n"
"--\n"
"\n"
"Maxheap variant of heappop.");

#define _HEAPQ_HEAPPOP_MAX_METHODDEF    \
    {"heappop_max", (PyCFunction)_heapq_heappop_max, METH_O, _heapq_heappop_max__doc__},

static PyObject *
_heapq_heappop_max_impl(PyObject *module, PyObject *heap);

static PyObject *
_heapq_heappop_max(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *heap;

    if (!PyList_Check(arg)) {
        _PyArg_BadArgument("heappop_max", "argument", "list", arg);
        goto exit;
    }
    heap = arg;
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heappop_max_impl(module, heap);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapreplace_max__doc__,
"heapreplace_max($module, heap, item, /)\n"
"--\n"
"\n"
"Maxheap variant of heapreplace.");

#define _HEAPQ_HEAPREPLACE_MAX_METHODDEF    \
    {"heapreplace_max", _PyCFunction_CAST(_heapq_heapreplace_max), METH_FASTCALL, _heapq_heapreplace_max__doc__},

static PyObject *
_heapq_heapreplace_max_impl(PyObject *module, PyObject *heap, PyObject *item);

static PyObject *
_heapq_heapreplace_max(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *heap;
    PyObject *item;

    if (!_PyArg_CheckPositional("heapreplace_max", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyList_Check(args[0])) {
        _PyArg_BadArgument("heapreplace_max", "argument 1", "list", args[0]);
        goto exit;
    }
    heap = args[0];
    item = args[1];
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heapreplace_max_impl(module, heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapify_max__doc__,
"heapify_max($module, heap, /)\n"
"--\n"
"\n"
"Maxheap variant of heapify.");

#define _HEAPQ_HEAPIFY_MAX_METHODDEF    \
    {"heapify_max", (PyCFunction)_heapq_heapify_max, METH_O, _heapq_heapify_max__doc__},

static PyObject *
_heapq_heapify_max_impl(PyObject *module, PyObject *heap);

static PyObject *
_heapq_heapify_max(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *heap;

    if (!PyList_Check(arg)) {
        _PyArg_BadArgument("heapify_max", "argument", "list", arg);
        goto exit;
    }
    heap = arg;
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heapify_max_impl(module, heap);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappushpop_max__doc__,
"heappushpop_max($module, heap, item, /)\n"
"--\n"
"\n"
"Maxheap variant of heappushpop.\n"
"\n"
"The combined action runs more efficiently than heappush_max() followed by\n"
"a separate call to heappop_max().");

#define _HEAPQ_HEAPPUSHPOP_MAX_METHODDEF    \
    {"heappushpop_max", _PyCFunction_CAST(_heapq_heappushpop_max), METH_FASTCALL, _heapq_heappushpop_max__doc__},

static PyObject *
_heapq_heappushpop_max_impl(PyObject *module, PyObject *heap, PyObject *item);

static PyObject *
_heapq_heappushpop_max(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *heap;
    PyObject *item;

    if (!_PyArg_CheckPositional("heappushpop_max", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyList_Check(args[0])) {
        _PyArg_BadArgument("heappushpop_max", "argument 1", "list", args[0]);
        goto exit;
    }
    heap = args[0];
    item = args[1];
    Py_BEGIN_CRITICAL_SECTION(heap);
    return_value = _heapq_heappushpop_max_impl(module, heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=e83d50002c29a96d input=a9049054013a1b77]*/
