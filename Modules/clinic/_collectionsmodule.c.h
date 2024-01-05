/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_collections_deque_pop__doc__,
"pop($self, /)\n"
"--\n"
"\n"
"Remove and return the rightmost element.");

#define _COLLECTIONS_DEQUE_POP_METHODDEF    \
    {"pop", (PyCFunction)_collections_deque_pop, METH_NOARGS, _collections_deque_pop__doc__},

static PyObject *
_collections_deque_pop_impl(dequeobject *deque);

static PyObject *
_collections_deque_pop(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_pop_impl(deque);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_collections_deque_popleft__doc__,
"popleft($self, /)\n"
"--\n"
"\n"
"Remove and return the rightmost element.");

#define _COLLECTIONS_DEQUE_POPLEFT_METHODDEF    \
    {"popleft", (PyCFunction)_collections_deque_popleft, METH_NOARGS, _collections_deque_popleft__doc__},

static PyObject *
_collections_deque_popleft_impl(dequeobject *deque);

static PyObject *
_collections_deque_popleft(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_popleft_impl(deque);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_collections_deque_append__doc__,
"append($self, item, /)\n"
"--\n"
"\n"
"Add an element to the right side of the deque.");

#define _COLLECTIONS_DEQUE_APPEND_METHODDEF    \
    {"append", (PyCFunction)_collections_deque_append, METH_O, _collections_deque_append__doc__},

static PyObject *
_collections_deque_append_impl(dequeobject *deque, PyObject *item);

static PyObject *
_collections_deque_append(dequeobject *deque, PyObject *item)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_append_impl(deque, item);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_collections_deque_appendleft__doc__,
"appendleft($self, item, /)\n"
"--\n"
"\n"
"Add an element to the left side of the deque.");

#define _COLLECTIONS_DEQUE_APPENDLEFT_METHODDEF    \
    {"appendleft", (PyCFunction)_collections_deque_appendleft, METH_O, _collections_deque_appendleft__doc__},

static PyObject *
_collections_deque_appendleft_impl(dequeobject *deque, PyObject *item);

static PyObject *
_collections_deque_appendleft(dequeobject *deque, PyObject *item)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_appendleft_impl(deque, item);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_collections_deque_extend__doc__,
"extend($self, iterable, /)\n"
"--\n"
"\n"
"Extend the right side of the deque with elements from the iterable");

#define _COLLECTIONS_DEQUE_EXTEND_METHODDEF    \
    {"extend", (PyCFunction)_collections_deque_extend, METH_O, _collections_deque_extend__doc__},

PyDoc_STRVAR(_collections_deque_extendleft__doc__,
"extendleft($self, iterable, /)\n"
"--\n"
"\n"
"Extend the left side of the deque with elements from the iterable");

#define _COLLECTIONS_DEQUE_EXTENDLEFT_METHODDEF    \
    {"extendleft", (PyCFunction)_collections_deque_extendleft, METH_O, _collections_deque_extendleft__doc__},

PyDoc_STRVAR(_collections_deque_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of a deque.");

#define _COLLECTIONS_DEQUE_COPY_METHODDEF    \
    {"copy", (PyCFunction)_collections_deque_copy, METH_NOARGS, _collections_deque_copy__doc__},

static PyObject *
_collections_deque_copy_impl(dequeobject *deque);

static PyObject *
_collections_deque_copy(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_copy_impl(deque);
}

PyDoc_STRVAR(_collections_deque___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of a deque.");

#define _COLLECTIONS_DEQUE___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_collections_deque___copy__, METH_NOARGS, _collections_deque___copy____doc__},

static PyObject *
_collections_deque___copy___impl(dequeobject *deque);

static PyObject *
_collections_deque___copy__(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___copy___impl(deque);
}

PyDoc_STRVAR(_collections_deque_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all elements from the deque.");

#define _COLLECTIONS_DEQUE_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)_collections_deque_clear, METH_NOARGS, _collections_deque_clear__doc__},

static PyObject *
_collections_deque_clear_impl(dequeobject *deque);

static PyObject *
_collections_deque_clear(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_clear_impl(deque);
}

PyDoc_STRVAR(_collections_deque_rotate__doc__,
"rotate($self, n=1, /)\n"
"--\n"
"\n"
"Rotate the deque n steps to the right.  If n is negative, rotates left.");

#define _COLLECTIONS_DEQUE_ROTATE_METHODDEF    \
    {"rotate", _PyCFunction_CAST(_collections_deque_rotate), METH_FASTCALL, _collections_deque_rotate__doc__},

static PyObject *
_collections_deque_rotate_impl(dequeobject *deque, Py_ssize_t n);

static PyObject *
_collections_deque_rotate(dequeobject *deque, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t n = 1;

    if (!_PyArg_CheckPositional("rotate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_rotate_impl(deque, n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_collections_deque_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse *IN PLACE*");

#define _COLLECTIONS_DEQUE_REVERSE_METHODDEF    \
    {"reverse", (PyCFunction)_collections_deque_reverse, METH_NOARGS, _collections_deque_reverse__doc__},

static PyObject *
_collections_deque_reverse_impl(dequeobject *deque);

static PyObject *
_collections_deque_reverse(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_reverse_impl(deque);
}

PyDoc_STRVAR(_collections_deque_count__doc__,
"count($self, v, /)\n"
"--\n"
"\n"
"Return number of occurrences of v");

#define _COLLECTIONS_DEQUE_COUNT_METHODDEF    \
    {"count", (PyCFunction)_collections_deque_count, METH_O, _collections_deque_count__doc__},

static PyObject *
_collections_deque_count_impl(dequeobject *deque, PyObject *v);

static PyObject *
_collections_deque_count(dequeobject *deque, PyObject *v)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_count_impl(deque, v);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_collections_deque_index__doc__,
"index($self, value, [start, [stop]])\n"
"--\n"
"\n"
"Return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define _COLLECTIONS_DEQUE_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(_collections_deque_index), METH_FASTCALL, _collections_deque_index__doc__},

static PyObject *
_collections_deque_index_impl(dequeobject *deque, PyObject *v,
                              Py_ssize_t start, Py_ssize_t stop);

static PyObject *
_collections_deque_index(dequeobject *deque, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *v;
    Py_ssize_t start = 0;
    Py_ssize_t stop = Py_SIZE(deque);

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    v = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndexNotNone(args[1], &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyEval_SliceIndexNotNone(args[2], &stop)) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_index_impl(deque, v, start, stop);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_collections_deque_insert__doc__,
"insert($self, index, value, /)\n"
"--\n"
"\n"
"Insert value before index");

#define _COLLECTIONS_DEQUE_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(_collections_deque_insert), METH_FASTCALL, _collections_deque_insert__doc__},

static PyObject *
_collections_deque_insert_impl(dequeobject *deque, Py_ssize_t index,
                               PyObject *value);

static PyObject *
_collections_deque_insert(dequeobject *deque, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    PyObject *value;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    value = args[1];
    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_insert_impl(deque, index, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_collections_deque_remove__doc__,
"remove($self, value, /)\n"
"--\n"
"\n"
"Remove first occurrence of value.");

#define _COLLECTIONS_DEQUE_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)_collections_deque_remove, METH_O, _collections_deque_remove__doc__},

static PyObject *
_collections_deque_remove_impl(dequeobject *deque, PyObject *value);

static PyObject *
_collections_deque_remove(dequeobject *deque, PyObject *value)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = _collections_deque_remove_impl(deque, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_collections_deque___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define _COLLECTIONS_DEQUE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)_collections_deque___reduce__, METH_NOARGS, _collections_deque___reduce____doc__},

static PyObject *
_collections_deque___reduce___impl(dequeobject *deque);

static PyObject *
_collections_deque___reduce__(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___reduce___impl(deque);
}

PyDoc_STRVAR(_collections_deque___init____doc__,
"deque($self, iterable=None, maxlen=None)\n"
"--\n"
"\n"
"A list-like sequence optimized for data accesses near its endpoints.");

static int
_collections_deque___init___impl(dequeobject *deque, PyObject *iterable,
                                 PyObject *maxlen);

static int
_collections_deque___init__(PyObject *deque, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(iterable), &_Py_ID(maxlen), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "maxlen", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "deque",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *iterable = NULL;
    PyObject *maxlen = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        iterable = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    maxlen = fastargs[1];
skip_optional_pos:
    return_value = _collections_deque___init___impl((dequeobject *)deque, iterable, maxlen);

exit:
    return return_value;
}

PyDoc_STRVAR(_collections_deque___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the deque in memory, in bytes");

#define _COLLECTIONS_DEQUE___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_collections_deque___sizeof__, METH_NOARGS, _collections_deque___sizeof____doc__},

static PyObject *
_collections_deque___sizeof___impl(dequeobject *deque);

static PyObject *
_collections_deque___sizeof__(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___sizeof___impl(deque);
}

PyDoc_STRVAR(_collections_deque___reversed____doc__,
"__reversed__($self, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the deque.");

#define _COLLECTIONS_DEQUE___REVERSED___METHODDEF    \
    {"__reversed__", (PyCFunction)_collections_deque___reversed__, METH_NOARGS, _collections_deque___reversed____doc__},

static PyObject *
_collections_deque___reversed___impl(dequeobject *deque);

static PyObject *
_collections_deque___reversed__(dequeobject *deque, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___reversed___impl(deque);
}

PyDoc_STRVAR(_collections__count_elements__doc__,
"_count_elements($module, mapping, iterable, /)\n"
"--\n"
"\n"
"Count elements in the iterable, updating the mapping");

#define _COLLECTIONS__COUNT_ELEMENTS_METHODDEF    \
    {"_count_elements", _PyCFunction_CAST(_collections__count_elements), METH_FASTCALL, _collections__count_elements__doc__},

static PyObject *
_collections__count_elements_impl(PyObject *module, PyObject *mapping,
                                  PyObject *iterable);

static PyObject *
_collections__count_elements(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *mapping;
    PyObject *iterable;

    if (!_PyArg_CheckPositional("_count_elements", nargs, 2, 2)) {
        goto exit;
    }
    mapping = args[0];
    iterable = args[1];
    return_value = _collections__count_elements_impl(module, mapping, iterable);

exit:
    return return_value;
}

static PyObject *
tuplegetter_new_impl(PyTypeObject *type, Py_ssize_t index, PyObject *doc);

static PyObject *
tuplegetter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->tuplegetter_type;
    Py_ssize_t index;
    PyObject *doc;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("_tuplegetter", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_tuplegetter", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(PyTuple_GET_ITEM(args, 0));
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    doc = PyTuple_GET_ITEM(args, 1);
    return_value = tuplegetter_new_impl(type, index, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=048f7e8a64c412ea input=a9049054013a1b77]*/
