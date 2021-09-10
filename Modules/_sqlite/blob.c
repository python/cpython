#include "blob.h"
#include "util.h"


int pysqlite_blob_init(pysqlite_Blob *self, pysqlite_Connection* connection,
                       sqlite3_blob *blob)
{
    Py_INCREF(connection);
    self->connection = connection;
    self->offset = 0;
    self->blob = blob;
    self->in_weakreflist = NULL;

    Py_BEGIN_ALLOW_THREADS
    self->length = sqlite3_blob_bytes(self->blob);
    Py_END_ALLOW_THREADS

    if (!pysqlite_check_thread(self->connection)) {
        return -1;
    }
    return 0;
}

static void remove_blob_from_connection_blob_list(pysqlite_Blob *self)
{
    Py_ssize_t i;
    PyObject *item;

    for (i = 0; i < PyList_GET_SIZE(self->connection->blobs); i++) {
        item = PyList_GET_ITEM(self->connection->blobs, i);
        if (PyWeakref_GetObject(item) == (PyObject *)self) {
            PyList_SetSlice(self->connection->blobs, i, i+1, NULL);
            break;
        }
    }
}

static void _close_blob_inner(pysqlite_Blob* self)
{
    sqlite3_blob *blob;

    /* close the blob */
    blob = self->blob;
    self->blob = NULL;
    if (blob) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_blob_close(blob);
        Py_END_ALLOW_THREADS
    }

    /* remove from connection weaklist */
    remove_blob_from_connection_blob_list(self);
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }   
}

static void pysqlite_blob_dealloc(pysqlite_Blob* self)
{
    _close_blob_inner(self);
    Py_XDECREF(self->connection);
    Py_TYPE(self)->tp_free((PyObject*)self);
}


/*
 * Checks if a blob object is usable (i. e. not closed).
 *
 * 0 => error; 1 => ok
 */
int pysqlite_check_blob(pysqlite_Blob *blob)
{

    if (!blob->blob) {
        pysqlite_state *state = pysqlite_get_state(NULL);
        PyErr_SetString(state->ProgrammingError,
                        "Cannot operate on a closed blob.");
        return 0;
    } else if (!pysqlite_check_connection(blob->connection) ||
               !pysqlite_check_thread(blob->connection)) {
        return 0;
    } else {
        return 1;
    }
}


PyObject* pysqlite_blob_close(pysqlite_Blob *self)
{

    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    _close_blob_inner(self);
    Py_RETURN_NONE;
};


static Py_ssize_t pysqlite_blob_length(pysqlite_Blob *self)
{
    if (!pysqlite_check_blob(self)) {
        return -1;
    }

    return self->length;
};

static PyObject* inner_read(pysqlite_Blob *self, int read_length, int offset)
{
    PyObject *buffer;
    char *raw_buffer;
    int rc;

    buffer = PyBytes_FromStringAndSize(NULL, read_length);
    if (!buffer) {
        return NULL;
    }
    raw_buffer = PyBytes_AS_STRING(buffer);

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_read(self->blob, raw_buffer, read_length, self->offset);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK){
        Py_DECREF(buffer);
        /* For some reason after modifying blob the
           error is not set on the connection db. */
        if (rc == SQLITE_ABORT) {
            PyErr_SetString(self->connection->OperationalError,
                            "Cannot operate on modified blob");
        } else {
            _pysqlite_seterror(self->connection->state, self->connection->db);
        }
        return NULL;
    }
    return buffer;
}


PyObject* pysqlite_blob_read(pysqlite_Blob *self, PyObject *args)
{
    int read_length = -1;
    PyObject *buffer;

    if (!PyArg_ParseTuple(args, "|i", &read_length)) {
        return NULL;
    }

    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    if (read_length < 0) {
        /* same as file read. */
        read_length = self->length;
    }

    /* making sure we don't read more then blob size */
    if (read_length > self->length - self->offset) {
        read_length = self->length - self->offset;
    }

    buffer = inner_read(self, read_length, self->offset);

    if (buffer != NULL) {
        /* update offset on sucess. */
        self->offset += read_length;
    }

    return buffer;
};

static int write_inner(pysqlite_Blob *self, const void *buf, Py_ssize_t len, int offset)
{
    int rc;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_write(self->blob, buf, len, offset);
    Py_END_ALLOW_THREADS
    if (rc != SQLITE_OK) {
        /* For some reason after modifying blob the
        error is not set on the connection db. */
        if (rc == SQLITE_ABORT) {
            PyErr_SetString(self->connection->OperationalError,
                            "Cannot operate on modified blob");
        } else {
            _pysqlite_seterror(self->connection->state, self->connection->db);
        }
        return -1;
    }
    return 0;
}


PyObject* pysqlite_blob_write(pysqlite_Blob *self, PyObject *data)
{
    Py_buffer data_buffer;
    int rc;

    if (PyObject_GetBuffer(data, &data_buffer, PyBUF_SIMPLE) < 0) {
        return NULL;
    }

    if (data_buffer.len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "data longer than INT_MAX bytes");
        PyBuffer_Release(&data_buffer);
        return NULL;
    }

    if (data_buffer.len > self->length - self->offset) {
        PyErr_SetString(PyExc_ValueError,
                        "data longer than blob length");
        PyBuffer_Release(&data_buffer);
        return NULL;
    }

    if (!pysqlite_check_blob(self)) {
        PyBuffer_Release(&data_buffer);
        return NULL;
    }

    rc = write_inner(self, data_buffer.buf, data_buffer.len, self->offset);

    if (rc == 0) {
        self->offset += (int)data_buffer.len;
        PyBuffer_Release(&data_buffer);
        Py_RETURN_NONE;
    } else {
        PyBuffer_Release(&data_buffer);
        return NULL;
    }
}


PyObject* pysqlite_blob_seek(pysqlite_Blob *self, PyObject *args)
{
    int offset, from_what = 0;

    if (!PyArg_ParseTuple(args, "i|i", &offset, &from_what)) {
        return NULL;
    }


    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    switch (from_what) {
        case 0:  // relative to blob begin
            break;
        case 1:  // relative to current position
            if (offset > INT_MAX - self->offset) {
                goto overflow;
            }
            offset = self->offset + offset;
            break;
        case 2:  // relative to blob end
            if (offset > INT_MAX - self->length) {
                goto overflow;
            }
            offset = self->length + offset;
            break;
        default:
            PyErr_SetString(PyExc_ValueError,
                                "from_what should be 0, 1 or 2");
            return NULL;
    }

    if (offset < 0 || offset > self->length) {
        PyErr_SetString(PyExc_ValueError, "offset out of blob range");
        return NULL;
    }

    self->offset = offset;
    Py_RETURN_NONE;

overflow:
    PyErr_SetString(PyExc_OverflowError, "seek offset result in overflow");
    return NULL;
}


PyObject* pysqlite_blob_tell(pysqlite_Blob *self)
{
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    return PyLong_FromLong(self->offset);
}


PyObject* pysqlite_blob_enter(pysqlite_Blob *self)
{
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    Py_INCREF(self);
    return (PyObject *)self;
}


PyObject* pysqlite_blob_exit(pysqlite_Blob *self, PyObject *args)
{
    PyObject *res;
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    res = pysqlite_blob_close(self);
    if (!res) {
        return NULL;
    }
    Py_XDECREF(res);

    Py_RETURN_FALSE;
}

static PyObject* pysqlite_blob_concat(pysqlite_Blob *self, PyObject *args)
{
    if (pysqlite_check_blob(self)) {
        PyErr_SetString(PyExc_SystemError,
                        "Blob don't support concatenation");
    }
    return NULL;
}

static PyObject* pysqlite_blob_repeat(pysqlite_Blob *self, PyObject *args)
{
    if (pysqlite_check_blob(self)) {
        PyErr_SetString(PyExc_SystemError,
                        "Blob don't support repeat operation");
    }
    return NULL;
}

static int pysqlite_blob_contains(pysqlite_Blob *self, PyObject *args)
{
    if (pysqlite_check_blob(self)) {
        PyErr_SetString(PyExc_SystemError,
                        "Blob don't support contains operation");
    }
    return -1;
}

static PyObject* pysqlite_blob_item(pysqlite_Blob *self, Py_ssize_t i)
{
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    if (i < 0 || i >= self->length) {
        PyErr_SetString(PyExc_IndexError, "Blob index out of range");
        return NULL;
    }

    return inner_read(self, 1, i);
}

static int pysqlite_blob_ass_item(pysqlite_Blob *self, Py_ssize_t i, PyObject *v)
{
    const char *buf;

    if (!pysqlite_check_blob(self)) {
        return -1;
    }

    if (i < 0 || i >= self->length) {
        PyErr_SetString(PyExc_IndexError, "Blob index out of range");
        return -1;
    }
    if (v == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Blob object doesn't support item deletion");
        return -1;
    }
    if (! (PyBytes_Check(v) && PyBytes_Size(v)==1) ) {
        PyErr_SetString(PyExc_IndexError,
                        "Blob assignment must be length-1 bytes()");
        return -1;
    }

    buf = PyBytes_AsString(v);
    return write_inner(self, buf, 1, i);
}


static PyObject * pysqlite_blob_subscript(pysqlite_Blob *self, PyObject *item)
{
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += self->length;
        if (i < 0 || i >= self->length) {
            PyErr_SetString(PyExc_IndexError,
                "Blob index out of range");
            return NULL;
        }
        // TODO: I am not sure...
        return inner_read(self, 1, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen;

        if (PySlice_GetIndicesEx(item, self->length,
                         &start, &stop, &step, &slicelen) < 0) {
            return NULL;
        }

        if (slicelen <= 0) {
            return PyBytes_FromStringAndSize("", 0);
        } else if (step == 1) {
            return inner_read(self, slicelen, start);
        } else {
            char *result_buf = (char *)PyMem_Malloc(slicelen);
            char *data_buff = NULL;
            Py_ssize_t cur, i;
            PyObject *result;
            int rc;

            if (result_buf == NULL)
                return PyErr_NoMemory();

            data_buff = (char *)PyMem_Malloc(stop - start);
            if (data_buff == NULL) {
                PyMem_Free(result_buf);
                return PyErr_NoMemory();
            }

            Py_BEGIN_ALLOW_THREADS
            rc = sqlite3_blob_read(self->blob, data_buff, stop - start, start);
            Py_END_ALLOW_THREADS

            if (rc != SQLITE_OK){
                /* For some reason after modifying blob the
                   error is not set on the connection db. */
                if (rc == SQLITE_ABORT) {
                    PyErr_SetString(self->connection->OperationalError,
                                    "Cannot operate on modified blob");
                } else {
                    _pysqlite_seterror(self->connection->state, self->connection->db);
                }
                PyMem_Free(result_buf);
                PyMem_Free(data_buff);
                return NULL;
            }

            for (cur = 0, i = 0; i < slicelen;
                 cur += step, i++) {
                result_buf[i] = data_buff[cur];
            }
            result = PyBytes_FromStringAndSize(result_buf,
                                                slicelen);
            PyMem_Free(result_buf);
            PyMem_Free(data_buff);
            return result;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Blob indices must be integers");
        return NULL;
    }
}


static int pysqlite_blob_ass_subscript(pysqlite_Blob *self, PyObject *item, PyObject *value)
{
    int rc;

    if (!pysqlite_check_blob(self)) {
        return -1;
    }

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        const char *buf;

        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += self->length;
        if (i < 0 || i >= self->length) {
            PyErr_SetString(PyExc_IndexError,
                            "Blob index out of range");
            return -1;
        }
        if (value == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "Blob doesn't support item deletion");
            return -1;
        }
        if (! (PyBytes_Check(value) && PyBytes_Size(value)==1) ) {
            PyErr_SetString(PyExc_IndexError,
                            "Blob assignment must be length-1 bytes()");
            return -1;
        }

        buf = PyBytes_AsString(value);
        return write_inner(self, buf, 1, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen;
        Py_buffer vbuf;

        if (PySlice_GetIndicesEx(item,
                                 self->length, &start, &stop,
                                 &step, &slicelen) < 0) {
            return -1;
        }
        if (value == NULL) {
            PyErr_SetString(PyExc_TypeError,
                "Blob object doesn't support slice deletion");
            return -1;
        }
        if (PyObject_GetBuffer(value, &vbuf, PyBUF_SIMPLE) < 0)
            return -1;
        if (vbuf.len != slicelen) {
            PyErr_SetString(PyExc_IndexError,
                "Blob slice assignment is wrong size");
            PyBuffer_Release(&vbuf);
            return -1;
        }

        if (slicelen == 0) {
        }
        else if (step == 1) {
            rc = write_inner(self, vbuf.buf, slicelen, start);
        }
        else {
            Py_ssize_t cur, i;
            char *data_buff;


            data_buff = (char *)PyMem_Malloc(stop - start);
            if (data_buff == NULL) {
                PyErr_NoMemory();
                return -1;
            }

            Py_BEGIN_ALLOW_THREADS
            rc = sqlite3_blob_read(self->blob, data_buff, stop - start, start);
            Py_END_ALLOW_THREADS

            if (rc != SQLITE_OK){
                /* For some reason after modifying blob the
                   error is not set on the connection db. */
                if (rc == SQLITE_ABORT) {
                    PyErr_SetString(self->connection->OperationalError,
                                    "Cannot operate on modified blob");
                } else {
                    _pysqlite_seterror(self->connection->state, self->connection->db);
                }
                PyMem_Free(data_buff);
                rc = -1;
            }

            for (cur = 0, i = 0;
                 i < slicelen;
                 cur += step, i++)
            {
                data_buff[cur] = ((char *)vbuf.buf)[i];
            }

            Py_BEGIN_ALLOW_THREADS
            rc = sqlite3_blob_write(self->blob, data_buff, stop - start, start);
            Py_END_ALLOW_THREADS

            if (rc != SQLITE_OK){
                /* For some reason after modifying blob the
                   error is not set on the connection db. */
                if (rc == SQLITE_ABORT) {
                    PyErr_SetString(self->connection->OperationalError,
                                    "Cannot operate on modified blob");
                } else {
                    _pysqlite_seterror(self->connection->state, self->connection->db);
                }
                PyMem_Free(data_buff);
                rc = -1;
            }
            rc = 0;

        }
        PyBuffer_Release(&vbuf);
        return rc;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "Blob indices must be integer");
        return -1;
    }
}


static PyMethodDef blob_methods[] = {
    {"read", (PyCFunction)pysqlite_blob_read, METH_VARARGS,
        PyDoc_STR("read data from blob")},
    {"write", (PyCFunction)pysqlite_blob_write, METH_O,
        PyDoc_STR("write data to blob")},
    {"close", (PyCFunction)pysqlite_blob_close, METH_NOARGS,
        PyDoc_STR("close blob")},
    {"seek", (PyCFunction)pysqlite_blob_seek, METH_VARARGS,
        PyDoc_STR("change blob current offset")},
    {"tell", (PyCFunction)pysqlite_blob_tell, METH_NOARGS,
        PyDoc_STR("return blob current offset")},
    {"__enter__", (PyCFunction)pysqlite_blob_enter, METH_NOARGS,
        PyDoc_STR("blob context manager enter")},
    {"__exit__", (PyCFunction)pysqlite_blob_exit, METH_VARARGS,
        PyDoc_STR("blob context manager exit")},
    {NULL, NULL}
};

static PySequenceMethods blob_sequence_methods = {
    .sq_length = (lenfunc)pysqlite_blob_length,
    .sq_concat = (binaryfunc)pysqlite_blob_concat,
    .sq_repeat = (ssizeargfunc)pysqlite_blob_repeat,
    .sq_item = (ssizeargfunc)pysqlite_blob_item,
    .sq_ass_item = (ssizeobjargproc)pysqlite_blob_ass_item,
    .sq_contains = (objobjproc)pysqlite_blob_contains,
};

static PyMappingMethods blob_mapping_methods = {
    (lenfunc)pysqlite_blob_length,
    (binaryfunc)pysqlite_blob_subscript,
    (objobjargproc)pysqlite_blob_ass_subscript,
};

PyTypeObject pysqlite_BlobType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        MODULE_NAME ".Blob",
        .tp_basicsize = sizeof(pysqlite_Blob),
        .tp_dealloc = (destructor)pysqlite_blob_dealloc,
        .tp_as_sequence = &blob_sequence_methods,
        .tp_as_mapping = &blob_mapping_methods,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_weaklistoffset = offsetof(pysqlite_Blob, in_weakreflist),
        .tp_methods = blob_methods,
};

extern int pysqlite_blob_setup_types(void)
{
    pysqlite_BlobType.tp_new = PyType_GenericNew;
    return PyType_Ready(&pysqlite_BlobType);
}
