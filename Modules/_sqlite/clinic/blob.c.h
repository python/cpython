/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(blob_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close blob.");

#define BLOB_CLOSE_METHODDEF    \
    {"close", (PyCFunction)blob_close, METH_NOARGS, blob_close__doc__},

static PyObject *
blob_close_impl(pysqlite_Blob *self);

static PyObject *
blob_close(pysqlite_Blob *self, PyObject *Py_UNUSED(ignored))
{
    return blob_close_impl(self);
}

PyDoc_STRVAR(blob_read__doc__,
"read($self, read_length=-1, /)\n"
"--\n"
"\n"
"Read data from blob.");

#define BLOB_READ_METHODDEF    \
    {"read", (PyCFunction)(void(*)(void))blob_read, METH_FASTCALL, blob_read__doc__},

static PyObject *
blob_read_impl(pysqlite_Blob *self, int read_length);

static PyObject *
blob_read(pysqlite_Blob *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int read_length = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    read_length = _PyLong_AsInt(args[0]);
    if (read_length == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = blob_read_impl(self, read_length);

exit:
    return return_value;
}

PyDoc_STRVAR(blob_write__doc__,
"write($self, data_buffer, /)\n"
"--\n"
"\n"
"Write data to blob.");

#define BLOB_WRITE_METHODDEF    \
    {"write", (PyCFunction)blob_write, METH_O, blob_write__doc__},

static PyObject *
blob_write_impl(pysqlite_Blob *self, Py_buffer *data_buffer);

static PyObject *
blob_write(pysqlite_Blob *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data_buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data_buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data_buffer, 'C')) {
        _PyArg_BadArgument("write", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = blob_write_impl(self, &data_buffer);

exit:
    /* Cleanup for data_buffer */
    if (data_buffer.obj) {
       PyBuffer_Release(&data_buffer);
    }

    return return_value;
}

PyDoc_STRVAR(blob_seek__doc__,
"seek($self, offset, from_what=0, /)\n"
"--\n"
"\n"
"Change the access position for a blob.");

#define BLOB_SEEK_METHODDEF    \
    {"seek", (PyCFunction)(void(*)(void))blob_seek, METH_FASTCALL, blob_seek__doc__},

static PyObject *
blob_seek_impl(pysqlite_Blob *self, int offset, int from_what);

static PyObject *
blob_seek(pysqlite_Blob *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int offset;
    int from_what = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    offset = _PyLong_AsInt(args[0]);
    if (offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    from_what = _PyLong_AsInt(args[1]);
    if (from_what == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = blob_seek_impl(self, offset, from_what);

exit:
    return return_value;
}

PyDoc_STRVAR(blob_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Return current access position for a blob.");

#define BLOB_TELL_METHODDEF    \
    {"tell", (PyCFunction)blob_tell, METH_NOARGS, blob_tell__doc__},

static PyObject *
blob_tell_impl(pysqlite_Blob *self);

static PyObject *
blob_tell(pysqlite_Blob *self, PyObject *Py_UNUSED(ignored))
{
    return blob_tell_impl(self);
}

PyDoc_STRVAR(blob_enter__doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Blob context manager enter.");

#define BLOB_ENTER_METHODDEF    \
    {"__enter__", (PyCFunction)blob_enter, METH_NOARGS, blob_enter__doc__},

static PyObject *
blob_enter_impl(pysqlite_Blob *self);

static PyObject *
blob_enter(pysqlite_Blob *self, PyObject *Py_UNUSED(ignored))
{
    return blob_enter_impl(self);
}

PyDoc_STRVAR(blob_exit__doc__,
"__exit__($self, type, val, tb, /)\n"
"--\n"
"\n"
"Blob context manager exit.");

#define BLOB_EXIT_METHODDEF    \
    {"__exit__", (PyCFunction)(void(*)(void))blob_exit, METH_FASTCALL, blob_exit__doc__},

static PyObject *
blob_exit_impl(pysqlite_Blob *self, PyObject *type, PyObject *val,
               PyObject *tb);

static PyObject *
blob_exit(pysqlite_Blob *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *type;
    PyObject *val;
    PyObject *tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    type = args[0];
    val = args[1];
    tb = args[2];
    return_value = blob_exit_impl(self, type, val, tb);

exit:
    return return_value;
}
/*[clinic end generated code: output=5d378130443aa9ce input=a9049054013a1b77]*/
