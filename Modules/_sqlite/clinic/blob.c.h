/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(blob_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the blob.");

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
"read($self, length=-1, /)\n"
"--\n"
"\n"
"Read data at the current offset position.\n"
"\n"
"  length\n"
"    Read length in bytes.\n"
"\n"
"If the end of the blob is reached, the data up to end of file will be returned.\n"
"When length is not specified, or is negative, Blob.read() will read until the\n"
"end of the blob.");

#define BLOB_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(blob_read), METH_FASTCALL, blob_read__doc__},

static PyObject *
blob_read_impl(pysqlite_Blob *self, int length);

static PyObject *
blob_read(pysqlite_Blob *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int length = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    length = PyLong_AsInt(args[0]);
    if (length == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = blob_read_impl(self, length);

exit:
    return return_value;
}

PyDoc_STRVAR(blob_write__doc__,
"write($self, data, /)\n"
"--\n"
"\n"
"Write data at the current offset.\n"
"\n"
"This function cannot change the blob length.  Writing beyond the end of the\n"
"blob will result in an exception being raised.");

#define BLOB_WRITE_METHODDEF    \
    {"write", (PyCFunction)blob_write, METH_O, blob_write__doc__},

static PyObject *
blob_write_impl(pysqlite_Blob *self, Py_buffer *data);

static PyObject *
blob_write(pysqlite_Blob *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = blob_write_impl(self, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(blob_seek__doc__,
"seek($self, offset, origin=0, /)\n"
"--\n"
"\n"
"Set the current access position to offset.\n"
"\n"
"The origin argument defaults to os.SEEK_SET (absolute blob positioning).\n"
"Other values for origin are os.SEEK_CUR (seek relative to the current position)\n"
"and os.SEEK_END (seek relative to the blob\'s end).");

#define BLOB_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(blob_seek), METH_FASTCALL, blob_seek__doc__},

static PyObject *
blob_seek_impl(pysqlite_Blob *self, int offset, int origin);

static PyObject *
blob_seek(pysqlite_Blob *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int offset;
    int origin = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    offset = PyLong_AsInt(args[0]);
    if (offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    origin = PyLong_AsInt(args[1]);
    if (origin == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = blob_seek_impl(self, offset, origin);

exit:
    return return_value;
}

PyDoc_STRVAR(blob_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Return the current access position for the blob.");

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
    {"__exit__", _PyCFunction_CAST(blob_exit), METH_FASTCALL, blob_exit__doc__},

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
/*[clinic end generated code: output=31abd55660e0c5af input=a9049054013a1b77]*/
