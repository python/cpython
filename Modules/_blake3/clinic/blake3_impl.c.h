/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(py_blake3_new__doc__,
"blake3(data=b\'\', /, *, key=b\'\', derive_key_context=None,\n"
"       max_threads=-1, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new BLAKE3 hash object.");

static PyObject *
py_blake3_new_impl(PyTypeObject *type, PyObject *data, Py_buffer *key,
                   PyObject *derive_key_context, Py_ssize_t max_threads,
                   int usedforsecurity);

static PyObject *
py_blake3_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "key", "derive_key_context", "max_threads", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "blake3", 0};
    PyObject *argsbuf[5];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *data = NULL;
    Py_buffer key = {NULL, NULL};
    PyObject *derive_key_context = NULL;
    Py_ssize_t max_threads = -1;
    int usedforsecurity = 1;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    noptargs--;
    data = fastargs[0];
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        if (PyObject_GetBuffer(fastargs[1], &key, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!PyBuffer_IsContiguous(&key, 'C')) {
            _PyArg_BadArgument("blake3", "argument 'key'", "contiguous buffer", fastargs[1]);
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        if (!PyUnicode_Check(fastargs[2])) {
            _PyArg_BadArgument("blake3", "argument 'derive_key_context'", "str", fastargs[2]);
            goto exit;
        }
        if (PyUnicode_READY(fastargs[2]) == -1) {
            goto exit;
        }
        derive_key_context = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(fastargs[3]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            max_threads = ival;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    usedforsecurity = PyObject_IsTrue(fastargs[4]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = py_blake3_new_impl(type, data, &key, derive_key_context, max_threads, usedforsecurity);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }

    return return_value;
}

PyDoc_STRVAR(_blake3_blake3_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n"
"Reset this hash object to its initial state.");

#define _BLAKE3_BLAKE3_RESET_METHODDEF    \
    {"reset", (PyCFunction)_blake3_blake3_reset, METH_NOARGS, _blake3_blake3_reset__doc__},

static PyObject *
_blake3_blake3_reset_impl(BLAKE3Object *self);

static PyObject *
_blake3_blake3_reset(BLAKE3Object *self, PyObject *Py_UNUSED(ignored))
{
    return _blake3_blake3_reset_impl(self);
}

PyDoc_STRVAR(_blake3_blake3_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _BLAKE3_BLAKE3_COPY_METHODDEF    \
    {"copy", (PyCFunction)_blake3_blake3_copy, METH_NOARGS, _blake3_blake3_copy__doc__},

static PyObject *
_blake3_blake3_copy_impl(BLAKE3Object *self);

static PyObject *
_blake3_blake3_copy(BLAKE3Object *self, PyObject *Py_UNUSED(ignored))
{
    return _blake3_blake3_copy_impl(self);
}

PyDoc_STRVAR(_blake3_blake3_update__doc__,
"update($self, /, data)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided bytes-like object.");

#define _BLAKE3_BLAKE3_UPDATE_METHODDEF    \
    {"update", (PyCFunction)(void(*)(void))_blake3_blake3_update, METH_FASTCALL|METH_KEYWORDS, _blake3_blake3_update__doc__},

static PyObject *
_blake3_blake3_update_impl(BLAKE3Object *self, PyObject *data);

static PyObject *
_blake3_blake3_update(BLAKE3Object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "update", 0};
    PyObject *argsbuf[1];
    PyObject *data;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    data = args[0];
    return_value = _blake3_blake3_update_impl(self, data);

exit:
    return return_value;
}

PyDoc_STRVAR(_blake3_blake3_digest__doc__,
"digest($self, /, length=_blake3.OUT_LEN, seek=0)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _BLAKE3_BLAKE3_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)(void(*)(void))_blake3_blake3_digest, METH_FASTCALL|METH_KEYWORDS, _blake3_blake3_digest__doc__},

static PyObject *
_blake3_blake3_digest_impl(BLAKE3Object *self, size_t length, size_t seek);

static PyObject *
_blake3_blake3_digest(BLAKE3Object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"length", "seek", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "digest", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    size_t length = BLAKE3_OUT_LEN;
    size_t seek = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        if (!_PyLong_Size_t_Converter(args[0], &length)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!_PyLong_Size_t_Converter(args[1], &seek)) {
        goto exit;
    }
skip_optional_pos:
    return_value = _blake3_blake3_digest_impl(self, length, seek);

exit:
    return return_value;
}

PyDoc_STRVAR(_blake3_blake3_hexdigest__doc__,
"hexdigest($self, /, length=_blake3.OUT_LEN, seek=0)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _BLAKE3_BLAKE3_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)(void(*)(void))_blake3_blake3_hexdigest, METH_FASTCALL|METH_KEYWORDS, _blake3_blake3_hexdigest__doc__},

static PyObject *
_blake3_blake3_hexdigest_impl(BLAKE3Object *self, size_t length, size_t seek);

static PyObject *
_blake3_blake3_hexdigest(BLAKE3Object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"length", "seek", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "hexdigest", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    size_t length = BLAKE3_OUT_LEN;
    size_t seek = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        if (!_PyLong_Size_t_Converter(args[0], &length)) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!_PyLong_Size_t_Converter(args[1], &seek)) {
        goto exit;
    }
skip_optional_pos:
    return_value = _blake3_blake3_hexdigest_impl(self, length, seek);

exit:
    return return_value;
}
/*[clinic end generated code: output=d8fd6ecf96180c65 input=a9049054013a1b77]*/
