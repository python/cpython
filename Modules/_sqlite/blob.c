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


static void
close_blob(pysqlite_Blob *self)
{
    if (self->blob) {
        Py_BEGIN_ALLOW_THREADS
        sqlite3_blob_close(self->blob);
        Py_END_ALLOW_THREADS
        self->blob = NULL;
    }
}

static int
blob_traverse(pysqlite_Blob *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->connection);
    return 0;
}

static int
blob_clear(pysqlite_Blob *self)
{
    Py_CLEAR(self->connection);
    return 0;
}

static void
blob_dealloc(pysqlite_Blob *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);

    close_blob(self);

    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }
    tp->tp_clear((PyObject *)self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static void
blob_seterror(pysqlite_Blob *self, int rc)
{
    assert(self->connection != NULL);
#if SQLITE_VERSION_NUMBER < 3008008
    // SQLite pre 3.8.8 does not set errors on the connection
    if (rc == SQLITE_ABORT) {
        PyErr_SetString(self->connection->OperationalError,
                        "Cannot operate on an expired blob handle");
        return;
    }
#endif
    _pysqlite_seterror(self->connection->state, self->connection->db);
}

/*
 * Checks if a blob object is usable (i. e. not closed).
 *
 * 0 => error; 1 => ok
 */
static int
check_blob(pysqlite_Blob *self)
{
    if (!pysqlite_check_connection(self->connection) ||
        !pysqlite_check_thread(self->connection)) {
        return 0;
    }
    if (self->blob == NULL) {
        pysqlite_state *state = pysqlite_get_state(NULL);
        PyErr_SetString(state->ProgrammingError,
                        "Cannot operate on a closed blob.");
        return 0;
    }
    return 1;
}


/*[clinic input]
_sqlite3.Blob.close as blob_close

Close blob.
[clinic start generated code]*/

static PyObject *
blob_close_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=848accc20a138d1b input=56c86df5cab22490]*/
{
    if (!pysqlite_check_connection(self->connection) ||
        !pysqlite_check_thread(self->connection))
    {
        return NULL;
    }
    close_blob(self);
    Py_RETURN_NONE;
};

void
pysqlite_close_all_blobs(pysqlite_Connection *self)
{
    for (int i = 0; i < PyList_GET_SIZE(self->blobs); i++) {
        PyObject *weakref = PyList_GET_ITEM(self->blobs, i);
        PyObject *blob = PyWeakref_GetObject(weakref);
        if (!Py_IsNone(blob)) {
            close_blob((pysqlite_Blob *)blob);
        }
    }
}

static Py_ssize_t
blob_length(pysqlite_Blob *self)
{
    if (!check_blob(self)) {
        return -1;
    }
    return self->length;
};

static PyObject *
inner_read(pysqlite_Blob *self, int length, int offset)
{
    PyObject *buffer = PyBytes_FromStringAndSize(NULL, length);
    if (buffer == NULL) {
        return NULL;
    }

    char *raw_buffer = PyBytes_AS_STRING(buffer);
    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_read(self->blob, raw_buffer, length, offset);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        Py_DECREF(buffer);
        blob_seterror(self, rc);
        return NULL;
    }
    return buffer;
}


/*[clinic input]
_sqlite3.Blob.read as blob_read

    length: int = -1
    /

Read data from blob.
[clinic start generated code]*/

static PyObject *
blob_read_impl(pysqlite_Blob *self, int length)
/*[clinic end generated code: output=1fc99b2541360dde input=b4b443e99af5548f]*/
{
    if (!check_blob(self)) {
        return NULL;
    }

    if (length < 0) {
        /* same as file read. */
        length = self->length;
    }

    /* making sure we don't read more then blob size */
    if (length > self->length - self->offset) {
        length = self->length - self->offset;
    }

    PyObject *buffer = inner_read(self, length, self->offset);
    if (buffer == NULL) {
        return NULL;
    }
    self->offset += length;
    return buffer;
};

static int
inner_write(pysqlite_Blob *self, const void *buf, Py_ssize_t len, int offset)
{
    int rc;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_write(self->blob, buf, len, offset);
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        blob_seterror(self, rc);
        return -1;
    }
    return 0;
}


/*[clinic input]
_sqlite3.Blob.write as blob_write

    data: Py_buffer
    /

Write data to blob.
[clinic start generated code]*/

static PyObject *
blob_write_impl(pysqlite_Blob *self, Py_buffer *data)
/*[clinic end generated code: output=b34cf22601b570b2 input=0dcf4018286f55d2]*/
{
    if (!check_blob(self)) {
        return NULL;
    }

    if (data->len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "data longer than INT_MAX bytes");
        return NULL;
    }

    if (data->len > self->length - self->offset) {
        PyErr_SetString(PyExc_ValueError, "data longer than blob length");
        return NULL;
    }

    int rc = inner_write(self, data->buf, data->len, self->offset);
    if (rc < 0) {
        return NULL;
    }
    self->offset += (int)data->len;
    Py_RETURN_NONE;
}


/*[clinic input]
_sqlite3.Blob.seek as blob_seek

    offset: int
    origin: int = 0
    /

Change the access position for a blob.
[clinic start generated code]*/

static PyObject *
blob_seek_impl(pysqlite_Blob *self, int offset, int origin)
/*[clinic end generated code: output=854c5a0e208547a5 input=cc33da6f28af0561]*/
{
    if (!check_blob(self)) {
        return NULL;
    }

    switch (origin) {
        case BLOB_SEEK_START:
            break;
        case BLOB_SEEK_CUR:
            if (offset > INT_MAX - self->offset) {
                goto overflow;
            }
            offset = self->offset + offset;
            break;
        case BLOB_SEEK_END:
            if (offset > INT_MAX - self->length) {
                goto overflow;
            }
            offset = self->length + offset;
            break;
        default:
            PyErr_SetString(PyExc_ValueError,
                            "'origin' should be 'BLOB_SEEK_START', "
                            "'BLOB_SEEK_CUR', or 'BLOB_SEEK_END'");
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
    if (!check_blob(self)) {
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
    if (!check_blob(self)) {
        return NULL;
    }
    return Py_NewRef(self);
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
    if (!check_blob(self)) {
        return NULL;
    }
    close_blob(self);
    Py_RETURN_FALSE;
}

static PyObject *
subscript_index(pysqlite_Blob *self, PyObject *item)
{
    Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
    if (i == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (i < 0) {
        i += self->length;
    }
    if (i < 0 || i >= self->length) {
        PyErr_SetString(PyExc_IndexError, "Blob index out of range");
        return NULL;
    }
    return inner_read(self, 1, i);
}

static PyObject *
subscript_slice(pysqlite_Blob *self, PyObject *item)
{
    Py_ssize_t start, stop, step, slicelen;
    if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
        return NULL;
    }
    slicelen = PySlice_AdjustIndices(self->length, &start, &stop, step);

    if (slicelen <= 0) {
        return PyBytes_FromStringAndSize("", 0);
    }
    else if (step == 1) {
        return inner_read(self, slicelen, start);
    }

    PyObject *blob = inner_read(self, stop - start, start);
    if (blob == NULL) {
        return NULL;
    }

    PyObject *result = PyBytes_FromStringAndSize(NULL, slicelen);
    if (result == NULL) {
        goto exit;
    }

    char *blob_buf = PyBytes_AS_STRING(blob);
    char *res_buf = PyBytes_AS_STRING(result);
    for (Py_ssize_t i = 0, j = 0; i < slicelen; i++, j += step) {
        res_buf[i] = blob_buf[j];
    }

exit:
    Py_DECREF(blob);
    return result;
}

static PyObject *
blob_subscript(pysqlite_Blob *self, PyObject *item)
{
    if (!check_blob(self)) {
        return NULL;
    }

    if (PyIndex_Check(item)) {
        return subscript_index(self, item);
    }
    if (PySlice_Check(item)) {
        return subscript_slice(self, item);
    }

    PyErr_SetString(PyExc_TypeError, "Blob indices must be integers");
    return NULL;
}

static int
ass_subscript_index(pysqlite_Blob *self, PyObject *item, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Blob doesn't support item deletion");
        return -1;
    }
    if (!PyBytes_Check(value) || PyBytes_Size(value) != 1) {
        PyErr_SetString(PyExc_ValueError,
                        "Blob assignment must be a single byte");
        return -1;
    }

    Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
    if (i == -1 && PyErr_Occurred()) {
        return -1;
    }
    if (i < 0) {
        i += self->length;
    }
    if (i < 0 || i >= self->length) {
        PyErr_SetString(PyExc_IndexError, "Blob index out of range");
        return -1;
    }

    const char *buf = PyBytes_AS_STRING(value);
    return inner_write(self, buf, 1, i);
}

static int
ass_subscript_slice(pysqlite_Blob *self, PyObject *item, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Blob doesn't support slice deletion");
        return -1;
    }

    Py_ssize_t start, stop, step, slicelen;
    if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
        return -1;
    }
    slicelen = PySlice_AdjustIndices(self->length, &start, &stop, step);

    Py_buffer vbuf;
    if (PyObject_GetBuffer(value, &vbuf, PyBUF_SIMPLE) < 0) {
        return -1;
    }
    if (vbuf.len != slicelen) {
        PyErr_SetString(PyExc_IndexError,
                        "Blob slice assignment is wrong size");
        PyBuffer_Release(&vbuf);
        return -1;
    }

    int rc;
    if (slicelen == 0) {
        rc = 0;
    }
    else if (step == 1) {
        rc = inner_write(self, vbuf.buf, slicelen, start);
    }
    else {
        PyObject *read_blob = inner_read(self, stop - start, start);
        if (read_blob == NULL) {
            rc = -1;
        }
        else {
            char *blob_buf = PyBytes_AS_STRING(read_blob);
            for (Py_ssize_t i = 0, j = 0; i < slicelen; i++, j += step) {
                blob_buf[j] = ((char *)vbuf.buf)[i];
            }
            rc = inner_write(self, blob_buf, stop - start, start);
            Py_DECREF(read_blob);
        }
    }
    PyBuffer_Release(&vbuf);
    return rc;
}

static int
blob_ass_subscript(pysqlite_Blob *self, PyObject *item, PyObject *value)
{
    if (!check_blob(self)) {
        return -1;
    }

    if (PyIndex_Check(item)) {
        return ass_subscript_index(self, item, value);
    }
    if (PySlice_Check(item)) {
        return ass_subscript_slice(self, item, value);
    }

    PyErr_SetString(PyExc_TypeError, "Blob indices must be integer");
    return -1;
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

static struct PyMemberDef blob_members[] = {
    {"__weaklistoffset__", T_PYSSIZET, offsetof(pysqlite_Blob, in_weakreflist), READONLY},
};

static PyType_Slot blob_slots[] = {
    {Py_tp_dealloc, blob_dealloc},
    {Py_tp_traverse, blob_traverse},
    {Py_tp_clear, blob_clear},
    {Py_tp_methods, blob_methods},
    {Py_tp_members, blob_members},

    // Mapping protocol
    {Py_mp_length, blob_length},
    {Py_mp_subscript, blob_subscript},
    {Py_mp_ass_subscript, blob_ass_subscript},
    {0, NULL},
};

static PyType_Spec blob_spec = {
    .name = MODULE_NAME ".Blob",
    .basicsize = sizeof(pysqlite_Blob),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = blob_slots,
};

int
pysqlite_blob_setup_types(PyObject *module)
{
    PyObject *type = PyType_FromModuleAndSpec(module, &blob_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    pysqlite_state *state = pysqlite_get_state(module);
    state->BlobType = (PyTypeObject *)type;
    return 0;
}
