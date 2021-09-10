#include "blob.h"
#include "util.h"

#define clinic_state() (pysqlite_get_state(NULL))
#include "clinic/blob.c.h"
#undef clinic_state

/*[clinic input]
module _sqlite3
class _sqlite3.Blob "pysqlite_Blob *" "clinic_state()->BlobType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=908d3e16a45f8da7]*/

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


/*[clinic input]
_sqlite3.Blob.close as blob_close

Close blob.
[clinic start generated code]*/

static PyObject *
blob_close_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=848accc20a138d1b input=56c86df5cab22490]*/
{

    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    _close_blob_inner(self);
    Py_RETURN_NONE;
};

void
pysqlite_close_all_blobs(pysqlite_Connection *self)
{
    int i;
    PyObject *weakref;
    PyObject *blob;

    for (i = 0; i < PyList_GET_SIZE(self->blobs); i++) {
        weakref = PyList_GET_ITEM(self->blobs, i);
        blob = PyWeakref_GetObject(weakref);
        if (blob != Py_None) {
            blob_close_impl((pysqlite_Blob*)blob);
        }
    }
}

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


/*[clinic input]
_sqlite3.Blob.read as blob_read

    read_length: int = -1
    /

Read data from blob.
[clinic start generated code]*/

static PyObject *
blob_read_impl(pysqlite_Blob *self, int read_length)
/*[clinic end generated code: output=9c4881a77860b216 input=753a766082129348]*/
{
    PyObject *buffer;

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


/*[clinic input]
_sqlite3.Blob.write as blob_write

    data_buffer: Py_buffer
    /

Write data to blob.
[clinic start generated code]*/

static PyObject *
blob_write_impl(pysqlite_Blob *self, Py_buffer *data_buffer)
/*[clinic end generated code: output=dc8da6900b969799 input=8597402caf368add]*/
{
    int rc;

    if (data_buffer->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "data longer than INT_MAX bytes");
        return NULL;
    }

    if (data_buffer->len > self->length - self->offset) {
        PyErr_SetString(PyExc_ValueError,
                        "data longer than blob length");
        return NULL;
    }

    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    rc = write_inner(self, data_buffer->buf, data_buffer->len, self->offset);

    if (rc == 0) {
        self->offset += (int)data_buffer->len;
        Py_RETURN_NONE;
    } else {
        return NULL;
    }
}


/*[clinic input]
_sqlite3.Blob.seek as blob_seek

    offset: int
    from_what: int = 0
    /

Change the access position for a blob.
[clinic start generated code]*/

static PyObject *
blob_seek_impl(pysqlite_Blob *self, int offset, int from_what)
/*[clinic end generated code: output=c7f07cd9e1d90bf7 input=f8dae77055129be3]*/
{
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


/*[clinic input]
_sqlite3.Blob.tell as blob_tell

Return current access position for a blob.
[clinic start generated code]*/

static PyObject *
blob_tell_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=3d3ba484a90b3a99 input=aa1660f9aee18be4]*/
{
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    return PyLong_FromLong(self->offset);
}


/*[clinic input]
_sqlite3.Blob.__enter__ as blob_enter

Blob context manager enter.
[clinic start generated code]*/

static PyObject *
blob_enter_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=4fd32484b071a6cd input=fe4842c3c582d5a7]*/
{
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    Py_INCREF(self);
    return (PyObject *)self;
}


/*[clinic input]
_sqlite3.Blob.__exit__ as blob_exit

    type: object
    val: object
    tb: object
    /

Blob context manager exit.
[clinic start generated code]*/

static PyObject *
blob_exit_impl(pysqlite_Blob *self, PyObject *type, PyObject *val,
               PyObject *tb)
/*[clinic end generated code: output=fc86ceeb2b68c7b2 input=575d9ecea205f35f]*/
{
    PyObject *res;
    if (!pysqlite_check_blob(self)) {
        return NULL;
    }

    res = blob_close_impl(self);
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
    BLOB_CLOSE_METHODDEF
    BLOB_ENTER_METHODDEF
    BLOB_EXIT_METHODDEF
    BLOB_READ_METHODDEF
    BLOB_SEEK_METHODDEF
    BLOB_TELL_METHODDEF
    BLOB_WRITE_METHODDEF
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
