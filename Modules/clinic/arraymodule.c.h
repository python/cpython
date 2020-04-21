/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(array_array___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n"
"Return a copy of the array.");

#define ARRAY_ARRAY___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)array_array___copy__, METH_NOARGS, array_array___copy____doc__},

static PyObject *
array_array___copy___impl(arrayobject *self);

static PyObject *
array_array___copy__(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array___copy___impl(self);
}

PyDoc_STRVAR(array_array___deepcopy____doc__,
"__deepcopy__($self, unused, /)\n"
"--\n"
"\n"
"Return a copy of the array.");

#define ARRAY_ARRAY___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)array_array___deepcopy__, METH_O, array_array___deepcopy____doc__},

PyDoc_STRVAR(array_array_count__doc__,
"count($self, v, /)\n"
"--\n"
"\n"
"Return number of occurrences of v in the array.");

#define ARRAY_ARRAY_COUNT_METHODDEF    \
    {"count", (PyCFunction)array_array_count, METH_O, array_array_count__doc__},

PyDoc_STRVAR(array_array_index__doc__,
"index($self, v, /)\n"
"--\n"
"\n"
"Return index of first occurrence of v in the array.");

#define ARRAY_ARRAY_INDEX_METHODDEF    \
    {"index", (PyCFunction)array_array_index, METH_O, array_array_index__doc__},

PyDoc_STRVAR(array_array_remove__doc__,
"remove($self, v, /)\n"
"--\n"
"\n"
"Remove the first occurrence of v in the array.");

#define ARRAY_ARRAY_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)array_array_remove, METH_O, array_array_remove__doc__},

PyDoc_STRVAR(array_array_pop__doc__,
"pop($self, i=-1, /)\n"
"--\n"
"\n"
"Return the i-th element and delete it from the array.\n"
"\n"
"i defaults to -1.");

#define ARRAY_ARRAY_POP_METHODDEF    \
    {"pop", (PyCFunction)(void(*)(void))array_array_pop, METH_FASTCALL, array_array_pop__doc__},

static PyObject *
array_array_pop_impl(arrayobject *self, Py_ssize_t i);

static PyObject *
array_array_pop(arrayobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t i = -1;

    if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        i = ival;
    }
skip_optional:
    return_value = array_array_pop_impl(self, i);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_extend__doc__,
"extend($self, bb, /)\n"
"--\n"
"\n"
"Append items to the end of the array.");

#define ARRAY_ARRAY_EXTEND_METHODDEF    \
    {"extend", (PyCFunction)array_array_extend, METH_O, array_array_extend__doc__},

PyDoc_STRVAR(array_array_insert__doc__,
"insert($self, i, v, /)\n"
"--\n"
"\n"
"Insert a new item v into the array before position i.");

#define ARRAY_ARRAY_INSERT_METHODDEF    \
    {"insert", (PyCFunction)(void(*)(void))array_array_insert, METH_FASTCALL, array_array_insert__doc__},

static PyObject *
array_array_insert_impl(arrayobject *self, Py_ssize_t i, PyObject *v);

static PyObject *
array_array_insert(arrayobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t i;
    PyObject *v;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        i = ival;
    }
    v = args[1];
    return_value = array_array_insert_impl(self, i, v);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_buffer_info__doc__,
"buffer_info($self, /)\n"
"--\n"
"\n"
"Return a tuple (address, length) giving the current memory address and the length in items of the buffer used to hold array\'s contents.\n"
"\n"
"The length should be multiplied by the itemsize attribute to calculate\n"
"the buffer length in bytes.");

#define ARRAY_ARRAY_BUFFER_INFO_METHODDEF    \
    {"buffer_info", (PyCFunction)array_array_buffer_info, METH_NOARGS, array_array_buffer_info__doc__},

static PyObject *
array_array_buffer_info_impl(arrayobject *self);

static PyObject *
array_array_buffer_info(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_buffer_info_impl(self);
}

PyDoc_STRVAR(array_array_append__doc__,
"append($self, v, /)\n"
"--\n"
"\n"
"Append new value v to the end of the array.");

#define ARRAY_ARRAY_APPEND_METHODDEF    \
    {"append", (PyCFunction)array_array_append, METH_O, array_array_append__doc__},

PyDoc_STRVAR(array_array_byteswap__doc__,
"byteswap($self, /)\n"
"--\n"
"\n"
"Byteswap all items of the array.\n"
"\n"
"If the items in the array are not 1, 2, 4, or 8 bytes in size, RuntimeError is\n"
"raised.");

#define ARRAY_ARRAY_BYTESWAP_METHODDEF    \
    {"byteswap", (PyCFunction)array_array_byteswap, METH_NOARGS, array_array_byteswap__doc__},

static PyObject *
array_array_byteswap_impl(arrayobject *self);

static PyObject *
array_array_byteswap(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_byteswap_impl(self);
}

PyDoc_STRVAR(array_array_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse the order of the items in the array.");

#define ARRAY_ARRAY_REVERSE_METHODDEF    \
    {"reverse", (PyCFunction)array_array_reverse, METH_NOARGS, array_array_reverse__doc__},

static PyObject *
array_array_reverse_impl(arrayobject *self);

static PyObject *
array_array_reverse(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_reverse_impl(self);
}

PyDoc_STRVAR(array_array_fromfile__doc__,
"fromfile($self, f, n, /)\n"
"--\n"
"\n"
"Read n objects from the file object f and append them to the end of the array.");

#define ARRAY_ARRAY_FROMFILE_METHODDEF    \
    {"fromfile", (PyCFunction)(void(*)(void))array_array_fromfile, METH_FASTCALL, array_array_fromfile__doc__},

static PyObject *
array_array_fromfile_impl(arrayobject *self, PyObject *f, Py_ssize_t n);

static PyObject *
array_array_fromfile(arrayobject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *f;
    Py_ssize_t n;

    if (!_PyArg_CheckPositional("fromfile", nargs, 2, 2)) {
        goto exit;
    }
    f = args[0];
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
    }
    return_value = array_array_fromfile_impl(self, f, n);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_tofile__doc__,
"tofile($self, f, /)\n"
"--\n"
"\n"
"Write all items (as machine values) to the file object f.");

#define ARRAY_ARRAY_TOFILE_METHODDEF    \
    {"tofile", (PyCFunction)array_array_tofile, METH_O, array_array_tofile__doc__},

PyDoc_STRVAR(array_array_fromlist__doc__,
"fromlist($self, list, /)\n"
"--\n"
"\n"
"Append items to array from list.");

#define ARRAY_ARRAY_FROMLIST_METHODDEF    \
    {"fromlist", (PyCFunction)array_array_fromlist, METH_O, array_array_fromlist__doc__},

PyDoc_STRVAR(array_array_tolist__doc__,
"tolist($self, /)\n"
"--\n"
"\n"
"Convert array to an ordinary list with the same items.");

#define ARRAY_ARRAY_TOLIST_METHODDEF    \
    {"tolist", (PyCFunction)array_array_tolist, METH_NOARGS, array_array_tolist__doc__},

static PyObject *
array_array_tolist_impl(arrayobject *self);

static PyObject *
array_array_tolist(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_tolist_impl(self);
}

PyDoc_STRVAR(array_array_frombytes__doc__,
"frombytes($self, buffer, /)\n"
"--\n"
"\n"
"Appends items from the string, interpreting it as an array of machine values, as if it had been read from a file using the fromfile() method).");

#define ARRAY_ARRAY_FROMBYTES_METHODDEF    \
    {"frombytes", (PyCFunction)array_array_frombytes, METH_O, array_array_frombytes__doc__},

static PyObject *
array_array_frombytes_impl(arrayobject *self, Py_buffer *buffer);

static PyObject *
array_array_frombytes(arrayobject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("frombytes", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = array_array_frombytes_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(array_array_tobytes__doc__,
"tobytes($self, /)\n"
"--\n"
"\n"
"Convert the array to an array of machine values and return the bytes representation.");

#define ARRAY_ARRAY_TOBYTES_METHODDEF    \
    {"tobytes", (PyCFunction)array_array_tobytes, METH_NOARGS, array_array_tobytes__doc__},

static PyObject *
array_array_tobytes_impl(arrayobject *self);

static PyObject *
array_array_tobytes(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_tobytes_impl(self);
}

PyDoc_STRVAR(array_array_fromunicode__doc__,
"fromunicode($self, ustr, /)\n"
"--\n"
"\n"
"Extends this array with data from the unicode string ustr.\n"
"\n"
"The array must be a unicode type array; otherwise a ValueError is raised.\n"
"Use array.frombytes(ustr.encode(...)) to append Unicode data to an array of\n"
"some other type.");

#define ARRAY_ARRAY_FROMUNICODE_METHODDEF    \
    {"fromunicode", (PyCFunction)array_array_fromunicode, METH_O, array_array_fromunicode__doc__},

static PyObject *
array_array_fromunicode_impl(arrayobject *self, const Py_UNICODE *ustr,
                             Py_ssize_clean_t ustr_length);

static PyObject *
array_array_fromunicode(arrayobject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    const Py_UNICODE *ustr;
    Py_ssize_clean_t ustr_length;

    if (!PyArg_Parse(arg, "u#:fromunicode", &ustr, &ustr_length)) {
        goto exit;
    }
    return_value = array_array_fromunicode_impl(self, ustr, ustr_length);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_tounicode__doc__,
"tounicode($self, /)\n"
"--\n"
"\n"
"Extends this array with data from the unicode string ustr.\n"
"\n"
"Convert the array to a unicode string.  The array must be a unicode type array;\n"
"otherwise a ValueError is raised.  Use array.tobytes().decode() to obtain a\n"
"unicode string from an array of some other type.");

#define ARRAY_ARRAY_TOUNICODE_METHODDEF    \
    {"tounicode", (PyCFunction)array_array_tounicode, METH_NOARGS, array_array_tounicode__doc__},

static PyObject *
array_array_tounicode_impl(arrayobject *self);

static PyObject *
array_array_tounicode(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_tounicode_impl(self);
}

PyDoc_STRVAR(array_array___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Size of the array in memory, in bytes.");

#define ARRAY_ARRAY___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)array_array___sizeof__, METH_NOARGS, array_array___sizeof____doc__},

static PyObject *
array_array___sizeof___impl(arrayobject *self);

static PyObject *
array_array___sizeof__(arrayobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array___sizeof___impl(self);
}

PyDoc_STRVAR(array__array_reconstructor__doc__,
"_array_reconstructor($module, arraytype, typecode, mformat_code, items,\n"
"                     /)\n"
"--\n"
"\n"
"Internal. Used for pickling support.");

#define ARRAY__ARRAY_RECONSTRUCTOR_METHODDEF    \
    {"_array_reconstructor", (PyCFunction)(void(*)(void))array__array_reconstructor, METH_FASTCALL, array__array_reconstructor__doc__},

static PyObject *
array__array_reconstructor_impl(PyObject *module, PyTypeObject *arraytype,
                                int typecode,
                                enum machine_format_code mformat_code,
                                PyObject *items);

static PyObject *
array__array_reconstructor(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *arraytype;
    int typecode;
    enum machine_format_code mformat_code;
    PyObject *items;

    if (!_PyArg_CheckPositional("_array_reconstructor", nargs, 4, 4)) {
        goto exit;
    }
    arraytype = (PyTypeObject *)args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("_array_reconstructor", "argument 2", "a unicode character", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1])) {
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(args[1]) != 1) {
        _PyArg_BadArgument("_array_reconstructor", "argument 2", "a unicode character", args[1]);
        goto exit;
    }
    typecode = PyUnicode_READ_CHAR(args[1], 0);
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    mformat_code = _PyLong_AsInt(args[2]);
    if (mformat_code == -1 && PyErr_Occurred()) {
        goto exit;
    }
    items = args[3];
    return_value = array__array_reconstructor_impl(module, arraytype, typecode, mformat_code, items);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array___reduce_ex____doc__,
"__reduce_ex__($self, value, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ARRAY_ARRAY___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", (PyCFunction)array_array___reduce_ex__, METH_O, array_array___reduce_ex____doc__},

PyDoc_STRVAR(array_arrayiterator___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ARRAY_ARRAYITERATOR___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)array_arrayiterator___reduce__, METH_NOARGS, array_arrayiterator___reduce____doc__},

static PyObject *
array_arrayiterator___reduce___impl(arrayiterobject *self);

static PyObject *
array_arrayiterator___reduce__(arrayiterobject *self, PyObject *Py_UNUSED(ignored))
{
    return array_arrayiterator___reduce___impl(self);
}

PyDoc_STRVAR(array_arrayiterator___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ARRAY_ARRAYITERATOR___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)array_arrayiterator___setstate__, METH_O, array_arrayiterator___setstate____doc__},
/*[clinic end generated code: output=f649fc0bc9f6b13a input=a9049054013a1b77]*/
