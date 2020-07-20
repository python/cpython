/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_collections_deque_pop__doc__,
"pop($self, /)\n"
"--\n"
"\n"
"Remove and return the rightmost element.");

#define _COLLECTIONS_DEQUE_POP_METHODDEF    \
    {"pop", (PyCFunction)_collections_deque_pop, METH_NOARGS, _collections_deque_pop__doc__},

static PyObject *
_collections_deque_pop_impl(dequeobject *self);

static PyObject *
_collections_deque_pop(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_pop_impl(self);
}

PyDoc_STRVAR(_collections_deque_popleft__doc__,
"popleft($self, /)\n"
"--\n"
"\n"
"Remove and return the leftmost element.\"");

#define _COLLECTIONS_DEQUE_POPLEFT_METHODDEF    \
    {"popleft", (PyCFunction)_collections_deque_popleft, METH_NOARGS, _collections_deque_popleft__doc__},

static PyObject *
_collections_deque_popleft_impl(dequeobject *self);

static PyObject *
_collections_deque_popleft(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_popleft_impl(self);
}

PyDoc_STRVAR(_collections_deque_append__doc__,
"append($self, object, /)\n"
"--\n"
"\n"
"Add an element to the right side of the deque.");

#define _COLLECTIONS_DEQUE_APPEND_METHODDEF    \
    {"append", (PyCFunction)_collections_deque_append, METH_O, _collections_deque_append__doc__},

PyDoc_STRVAR(_collections_deque_appendleft__doc__,
"appendleft($self, object, /)\n"
"--\n"
"\n"
"Add an element to the left side of the deque.");

#define _COLLECTIONS_DEQUE_APPENDLEFT_METHODDEF    \
    {"appendleft", (PyCFunction)_collections_deque_appendleft, METH_O, _collections_deque_appendleft__doc__},

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
_collections_deque_copy_impl(dequeobject *self);

static PyObject *
_collections_deque_copy(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_copy_impl(self);
}

PyDoc_STRVAR(_collections_deque___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of a deque.");

#define _COLLECTIONS_DEQUE___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_collections_deque___copy__, METH_NOARGS, _collections_deque___copy____doc__},

static PyObject *
_collections_deque___copy___impl(dequeobject *self);

static PyObject *
_collections_deque___copy__(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___copy___impl(self);
}

PyDoc_STRVAR(_collections_deque_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all elements from the deque.");

#define _COLLECTIONS_DEQUE_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)_collections_deque_clear, METH_NOARGS, _collections_deque_clear__doc__},

static PyObject *
_collections_deque_clear_impl(dequeobject *self);

static PyObject *
_collections_deque_clear(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_clear_impl(self);
}

PyDoc_STRVAR(_collections_deque_rotate__doc__,
"rotate($self, start=1, /)\n"
"--\n"
"\n"
"Rotate the deque n steps to the right (default n=1).  If n is negative, rotates left.");

#define _COLLECTIONS_DEQUE_ROTATE_METHODDEF    \
    {"rotate", (PyCFunction)(void(*)(void))_collections_deque_rotate, METH_FASTCALL, _collections_deque_rotate__doc__},

static PyObject *
_collections_deque_rotate_impl(dequeobject *self, Py_ssize_t start);

static PyObject *
_collections_deque_rotate(dequeobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t start = 1;

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
        start = ival;
    }
skip_optional:
    return_value = _collections_deque_rotate_impl(self, start);

exit:
    return return_value;
}

PyDoc_STRVAR(_collections_deque_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse *IN PLACE*.");

#define _COLLECTIONS_DEQUE_REVERSE_METHODDEF    \
    {"reverse", (PyCFunction)_collections_deque_reverse, METH_NOARGS, _collections_deque_reverse__doc__},

static PyObject *
_collections_deque_reverse_impl(dequeobject *self);

static PyObject *
_collections_deque_reverse(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque_reverse_impl(self);
}

PyDoc_STRVAR(_collections_deque_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"D.count(value) -> integer -- return number of occurrences of value");

#define _COLLECTIONS_DEQUE_COUNT_METHODDEF    \
    {"count", (PyCFunction)_collections_deque_count, METH_O, _collections_deque_count__doc__},

PyDoc_STRVAR(_collections_deque_index__doc__,
"index($self, value, start=0, stop=sys.maxsize, /)\n"
"--\n"
"\n"
"D.index(value, [start, [stop]]) -> integer -- return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define _COLLECTIONS_DEQUE_INDEX_METHODDEF    \
    {"index", (PyCFunction)(void(*)(void))_collections_deque_index, METH_FASTCALL, _collections_deque_index__doc__},

static PyObject *
_collections_deque_index_impl(dequeobject *self, PyObject *value,
                              Py_ssize_t start, Py_ssize_t stop);

static PyObject *
_collections_deque_index(dequeobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    value = args[0];
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
    return_value = _collections_deque_index_impl(self, value, start, stop);

exit:
    return return_value;
}

PyDoc_STRVAR(_collections_deque_insert__doc__,
"insert($self, index, object, /)\n"
"--\n"
"\n"
"D.insert(index, object) -- insert object before index");

#define _COLLECTIONS_DEQUE_INSERT_METHODDEF    \
    {"insert", (PyCFunction)(void(*)(void))_collections_deque_insert, METH_FASTCALL, _collections_deque_insert__doc__},

static PyObject *
_collections_deque_insert_impl(dequeobject *self, Py_ssize_t index,
                               PyObject *object);

static PyObject *
_collections_deque_insert(dequeobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t index;
    PyObject *object;

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
    object = args[1];
    return_value = _collections_deque_insert_impl(self, index, object);

exit:
    return return_value;
}

PyDoc_STRVAR(_collections_deque_remove__doc__,
"remove($self, value, /)\n"
"--\n"
"\n"
"D.remove(value) -- remove first occurrence of value.");

#define _COLLECTIONS_DEQUE_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)_collections_deque_remove, METH_O, _collections_deque_remove__doc__},

PyDoc_STRVAR(_collections_deque___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define _COLLECTIONS_DEQUE___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)_collections_deque___reduce__, METH_NOARGS, _collections_deque___reduce____doc__},

static PyObject *
_collections_deque___reduce___impl(dequeobject *self);

static PyObject *
_collections_deque___reduce__(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___reduce___impl(self);
}

PyDoc_STRVAR(_collections_deque___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"D.__sizeof__() -- size of D in memory, in bytes");

#define _COLLECTIONS_DEQUE___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_collections_deque___sizeof__, METH_NOARGS, _collections_deque___sizeof____doc__},

static PyObject *
_collections_deque___sizeof___impl(dequeobject *self);

static PyObject *
_collections_deque___sizeof__(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___sizeof___impl(self);
}

PyDoc_STRVAR(_collections_deque___reversed____doc__,
"__reversed__($self, /)\n"
"--\n"
"\n"
"D.__reversed__() -- return a reverse iterator over the deque\"");

#define _COLLECTIONS_DEQUE___REVERSED___METHODDEF    \
    {"__reversed__", (PyCFunction)_collections_deque___reversed__, METH_NOARGS, _collections_deque___reversed____doc__},

static PyObject *
_collections_deque___reversed___impl(dequeobject *self);

static PyObject *
_collections_deque___reversed__(dequeobject *self, PyObject *Py_UNUSED(ignored))
{
    return _collections_deque___reversed___impl(self);
}

PyDoc_STRVAR(_collections__count_elements__doc__,
"_count_elements($module, mapping, iterable, /)\n"
"--\n"
"\n"
"Count elements in the iterable, updating the mapping");

#define _COLLECTIONS__COUNT_ELEMENTS_METHODDEF    \
    {"_count_elements", (PyCFunction)(void(*)(void))_collections__count_elements, METH_FASTCALL, _collections__count_elements__doc__},

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
    Py_ssize_t index;
    PyObject *doc;

    if ((type == &tuplegetter_type) &&
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
/*[clinic end generated code: output=f70acf24fbcfde07 input=a9049054013a1b77]*/
