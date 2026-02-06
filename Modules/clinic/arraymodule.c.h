/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_SINGLETON()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(array_array_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the array.");

#define ARRAY_ARRAY_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)array_array_clear, METH_NOARGS, array_array_clear__doc__},

static PyObject *
array_array_clear_impl(arrayobject *self);

static PyObject *
array_array_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_clear_impl((arrayobject *)self);
}

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
array_array___copy__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array___copy___impl((arrayobject *)self);
}

PyDoc_STRVAR(array_array___deepcopy____doc__,
"__deepcopy__($self, unused, /)\n"
"--\n"
"\n"
"Return a copy of the array.");

#define ARRAY_ARRAY___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)array_array___deepcopy__, METH_O, array_array___deepcopy____doc__},

static PyObject *
array_array___deepcopy___impl(arrayobject *self, PyObject *unused);

static PyObject *
array_array___deepcopy__(PyObject *self, PyObject *unused)
{
    PyObject *return_value = NULL;

    return_value = array_array___deepcopy___impl((arrayobject *)self, unused);

    return return_value;
}

PyDoc_STRVAR(array_array_count__doc__,
"count($self, v, /)\n"
"--\n"
"\n"
"Return number of occurrences of v in the array.");

#define ARRAY_ARRAY_COUNT_METHODDEF    \
    {"count", (PyCFunction)array_array_count, METH_O, array_array_count__doc__},

static PyObject *
array_array_count_impl(arrayobject *self, PyObject *v);

static PyObject *
array_array_count(PyObject *self, PyObject *v)
{
    PyObject *return_value = NULL;

    return_value = array_array_count_impl((arrayobject *)self, v);

    return return_value;
}

PyDoc_STRVAR(array_array_index__doc__,
"index($self, v, start=0, stop=sys.maxsize, /)\n"
"--\n"
"\n"
"Return index of first occurrence of v in the array.\n"
"\n"
"Raise ValueError if the value is not present.");

#define ARRAY_ARRAY_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(array_array_index), METH_FASTCALL, array_array_index__doc__},

static PyObject *
array_array_index_impl(arrayobject *self, PyObject *v, Py_ssize_t start,
                       Py_ssize_t stop);

static PyObject *
array_array_index(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *v;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;

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
    return_value = array_array_index_impl((arrayobject *)self, v, start, stop);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_remove__doc__,
"remove($self, v, /)\n"
"--\n"
"\n"
"Remove the first occurrence of v in the array.");

#define ARRAY_ARRAY_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)array_array_remove, METH_O, array_array_remove__doc__},

static PyObject *
array_array_remove_impl(arrayobject *self, PyObject *v);

static PyObject *
array_array_remove(PyObject *self, PyObject *v)
{
    PyObject *return_value = NULL;

    return_value = array_array_remove_impl((arrayobject *)self, v);

    return return_value;
}

PyDoc_STRVAR(array_array_pop__doc__,
"pop($self, i=-1, /)\n"
"--\n"
"\n"
"Return the i-th element and delete it from the array.\n"
"\n"
"i defaults to -1.");

#define ARRAY_ARRAY_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(array_array_pop), METH_FASTCALL, array_array_pop__doc__},

static PyObject *
array_array_pop_impl(arrayobject *self, Py_ssize_t i);

static PyObject *
array_array_pop(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t i = -1;

    if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
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
        i = ival;
    }
skip_optional:
    return_value = array_array_pop_impl((arrayobject *)self, i);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_extend__doc__,
"extend($self, bb, /)\n"
"--\n"
"\n"
"Append items to the end of the array.");

#define ARRAY_ARRAY_EXTEND_METHODDEF    \
    {"extend", _PyCFunction_CAST(array_array_extend), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, array_array_extend__doc__},

static PyObject *
array_array_extend_impl(arrayobject *self, PyTypeObject *cls, PyObject *bb);

static PyObject *
array_array_extend(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "extend",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *bb;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    bb = args[0];
    return_value = array_array_extend_impl((arrayobject *)self, cls, bb);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_insert__doc__,
"insert($self, i, v, /)\n"
"--\n"
"\n"
"Insert a new item v into the array before position i.");

#define ARRAY_ARRAY_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(array_array_insert), METH_FASTCALL, array_array_insert__doc__},

static PyObject *
array_array_insert_impl(arrayobject *self, Py_ssize_t i, PyObject *v);

static PyObject *
array_array_insert(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t i;
    PyObject *v;

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
        i = ival;
    }
    v = args[1];
    return_value = array_array_insert_impl((arrayobject *)self, i, v);

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
array_array_buffer_info(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_buffer_info_impl((arrayobject *)self);
}

PyDoc_STRVAR(array_array_append__doc__,
"append($self, v, /)\n"
"--\n"
"\n"
"Append new value v to the end of the array.");

#define ARRAY_ARRAY_APPEND_METHODDEF    \
    {"append", (PyCFunction)array_array_append, METH_O, array_array_append__doc__},

static PyObject *
array_array_append_impl(arrayobject *self, PyObject *v);

static PyObject *
array_array_append(PyObject *self, PyObject *v)
{
    PyObject *return_value = NULL;

    return_value = array_array_append_impl((arrayobject *)self, v);

    return return_value;
}

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
array_array_byteswap(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_byteswap_impl((arrayobject *)self);
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
array_array_reverse(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_reverse_impl((arrayobject *)self);
}

PyDoc_STRVAR(array_array_fromfile__doc__,
"fromfile($self, f, n, /)\n"
"--\n"
"\n"
"Read n objects from the file object f and append them to the end of the array.");

#define ARRAY_ARRAY_FROMFILE_METHODDEF    \
    {"fromfile", _PyCFunction_CAST(array_array_fromfile), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, array_array_fromfile__doc__},

static PyObject *
array_array_fromfile_impl(arrayobject *self, PyTypeObject *cls, PyObject *f,
                          Py_ssize_t n);

static PyObject *
array_array_fromfile(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fromfile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject *f;
    Py_ssize_t n;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    f = args[0];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
        if (n < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "n cannot be negative");
            goto exit;
        }
    }
    return_value = array_array_fromfile_impl((arrayobject *)self, cls, f, n);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_tofile__doc__,
"tofile($self, f, /)\n"
"--\n"
"\n"
"Write all items (as machine values) to the file object f.");

#define ARRAY_ARRAY_TOFILE_METHODDEF    \
    {"tofile", _PyCFunction_CAST(array_array_tofile), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, array_array_tofile__doc__},

static PyObject *
array_array_tofile_impl(arrayobject *self, PyTypeObject *cls, PyObject *f);

static PyObject *
array_array_tofile(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "tofile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *f;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    f = args[0];
    return_value = array_array_tofile_impl((arrayobject *)self, cls, f);

exit:
    return return_value;
}

PyDoc_STRVAR(array_array_fromlist__doc__,
"fromlist($self, list, /)\n"
"--\n"
"\n"
"Append items to array from list.");

#define ARRAY_ARRAY_FROMLIST_METHODDEF    \
    {"fromlist", (PyCFunction)array_array_fromlist, METH_O, array_array_fromlist__doc__},

static PyObject *
array_array_fromlist_impl(arrayobject *self, PyObject *list);

static PyObject *
array_array_fromlist(PyObject *self, PyObject *list)
{
    PyObject *return_value = NULL;

    return_value = array_array_fromlist_impl((arrayobject *)self, list);

    return return_value;
}

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
array_array_tolist(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_tolist_impl((arrayobject *)self);
}

PyDoc_STRVAR(array_array_frombytes__doc__,
"frombytes($self, buffer, /)\n"
"--\n"
"\n"
"Appends items from the string, interpreting it as an array of machine values, as if it had been read from a file using the fromfile() method.");

#define ARRAY_ARRAY_FROMBYTES_METHODDEF    \
    {"frombytes", (PyCFunction)array_array_frombytes, METH_O, array_array_frombytes__doc__},

static PyObject *
array_array_frombytes_impl(arrayobject *self, Py_buffer *buffer);

static PyObject *
array_array_frombytes(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = array_array_frombytes_impl((arrayobject *)self, &buffer);

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
array_array_tobytes(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_tobytes_impl((arrayobject *)self);
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
array_array_fromunicode_impl(arrayobject *self, PyObject *ustr);

static PyObject *
array_array_fromunicode(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *ustr;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("fromunicode", "argument", "str", arg);
        goto exit;
    }
    ustr = arg;
    return_value = array_array_fromunicode_impl((arrayobject *)self, ustr);

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
array_array_tounicode(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array_tounicode_impl((arrayobject *)self);
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
array_array___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return array_array___sizeof___impl((arrayobject *)self);
}

PyDoc_STRVAR(array__array_reconstructor__doc__,
"_array_reconstructor($module, arraytype, typecode, mformat_code, items,\n"
"                     /)\n"
"--\n"
"\n"
"Internal. Used for pickling support.");

#define ARRAY__ARRAY_RECONSTRUCTOR_METHODDEF    \
    {"_array_reconstructor", _PyCFunction_CAST(array__array_reconstructor), METH_FASTCALL, array__array_reconstructor__doc__},

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
    if (PyUnicode_GET_LENGTH(args[1]) != 1) {
        PyErr_Format(PyExc_TypeError,
            "_array_reconstructor(): argument 2 must be a unicode character, "
            "not a string of length %zd",
            PyUnicode_GET_LENGTH(args[1]));
        goto exit;
    }
    typecode = PyUnicode_READ_CHAR(args[1], 0);
    mformat_code = PyLong_AsInt(args[2]);
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
    {"__reduce_ex__", _PyCFunction_CAST(array_array___reduce_ex__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, array_array___reduce_ex____doc__},

static PyObject *
array_array___reduce_ex___impl(arrayobject *self, PyTypeObject *cls,
                               PyObject *value);

static PyObject *
array_array___reduce_ex__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__reduce_ex__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    value = args[0];
    return_value = array_array___reduce_ex___impl((arrayobject *)self, cls, value);

exit:
    return return_value;
}

PyDoc_STRVAR(array_arrayiterator___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define ARRAY_ARRAYITERATOR___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(array_arrayiterator___reduce__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, array_arrayiterator___reduce____doc__},

static PyObject *
array_arrayiterator___reduce___impl(arrayiterobject *self, PyTypeObject *cls);

static PyObject *
array_arrayiterator___reduce__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__reduce__() takes no arguments");
        return NULL;
    }
    return array_arrayiterator___reduce___impl((arrayiterobject *)self, cls);
}

PyDoc_STRVAR(array_arrayiterator___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define ARRAY_ARRAYITERATOR___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)array_arrayiterator___setstate__, METH_O, array_arrayiterator___setstate____doc__},

static PyObject *
array_arrayiterator___setstate___impl(arrayiterobject *self, PyObject *state);

static PyObject *
array_arrayiterator___setstate__(PyObject *self, PyObject *state)
{
    PyObject *return_value = NULL;

    return_value = array_arrayiterator___setstate___impl((arrayiterobject *)self, state);

    return return_value;
}
/*[clinic end generated code: output=c993c3598085840e input=a9049054013a1b77]*/
