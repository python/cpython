#include "blob.h"
#include "util.h"

#define clinic_state() (pysqlite_get_state_by_type(Py_TYPE(self)))
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
        sqlite3_blob *blob = self->blob;
        self->blob = NULL;

        Py_BEGIN_ALLOW_THREADS
        sqlite3_blob_close(blob);
        Py_END_ALLOW_THREADS
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

// Return 1 if the blob object is usable, 0 if not.
static int
check_blob(pysqlite_Blob *self)
{
    if (!pysqlite_check_connection(self->connection) ||
        !pysqlite_check_thread(self->connection)) {
        return 0;
    }
    if (self->blob == NULL) {
        pysqlite_state *state = self->connection->state;
        PyErr_SetString(state->ProgrammingError,
                        "Cannot operate on a closed blob.");
        return 0;
    }
    return 1;
}


/*[clinic input]
_sqlite3.Blob.close as blob_close

Close the blob.
[clinic start generated code]*/

static PyObject *
blob_close_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=848accc20a138d1b input=7bc178a402a40bd8]*/
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

static void
blob_seterror(pysqlite_Blob *self, int rc)
{
    assert(self->connection != NULL);
#if SQLITE_VERSION_NUMBER < 3008008
    // SQLite pre 3.8.8 does not set this blob error on the connection
    if (rc == SQLITE_ABORT) {
        PyErr_SetString(self->connection->OperationalError,
                        "Cannot operate on an expired blob handle");
        return;
    }
#endif
    _pysqlite_seterror(self->connection->state, self->connection->db);
}

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
        Read length in bytes.
    /

Read data at the current offset position.

If the end of the blob is reached, the data up to end of file will be returned.
When length is not specified, or is negative, Blob.read() will read until the
end of the blob.
[clinic start generated code]*/

static PyObject *
blob_read_impl(pysqlite_Blob *self, int length)
/*[clinic end generated code: output=1fc99b2541360dde input=f2e4aa4378837250]*/
{
    if (!check_blob(self)) {
        return NULL;
    }

    /* Make sure we never read past "EOB". Also read the rest of the blob if a
     * negative length is specified. */
    int blob_len = sqlite3_blob_bytes(self->blob);
    int max_read_len = blob_len - self->offset;
    if (length < 0 || length > max_read_len) {
        length = max_read_len;
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
    int remaining_len = sqlite3_blob_bytes(self->blob) - self->offset;
    if (len > remaining_len) {
        PyErr_SetString(PyExc_ValueError, "data longer than blob length");
        return -1;
    }

    int rc;
    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_blob_write(self->blob, buf, (int)len, offset);
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

Write data at the current offset.

This function cannot change the blob length.  Writing beyond the end of the
blob will result in an exception being raised.
[clinic start generated code]*/

static PyObject *
blob_write_impl(pysqlite_Blob *self, Py_buffer *data)
/*[clinic end generated code: output=b34cf22601b570b2 input=a84712f24a028e6d]*/
{
    if (!check_blob(self)) {
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

Set the current access position to offset.

The origin argument defaults to os.SEEK_SET (absolute blob positioning).
Other values for origin are os.SEEK_CUR (seek relative to the current position)
and os.SEEK_END (seek relative to the blob's end).
[clinic start generated code]*/

static PyObject *
blob_seek_impl(pysqlite_Blob *self, int offset, int origin)
/*[clinic end generated code: output=854c5a0e208547a5 input=5da9a07e55fe6bb6]*/
{
    if (!check_blob(self)) {
        return NULL;
    }

    int blob_len = sqlite3_blob_bytes(self->blob);
    switch (origin) {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            if (offset > INT_MAX - self->offset) {
                goto overflow;
            }
            offset += self->offset;
            break;
        case SEEK_END:
            if (offset > INT_MAX - blob_len) {
                goto overflow;
            }
            offset += blob_len;
            break;
        default:
            PyErr_SetString(PyExc_ValueError,
                            "'origin' should be os.SEEK_SET, os.SEEK_CUR, or "
                            "os.SEEK_END");
            return NULL;
    }

    if (offset < 0 || offset > blob_len) {
        PyErr_SetString(PyExc_ValueError, "offset out of blob range");
        return NULL;
    }

    self->offset = offset;
    Py_RETURN_NONE;

overflow:
    PyErr_SetString(PyExc_OverflowError, "seek offset results in overflow");
    return NULL;
}


/*[clinic input]
_sqlite3.Blob.tell as blob_tell

Return the current access position for the blob.
[clinic start generated code]*/

static PyObject *
blob_tell_impl(pysqlite_Blob *self)
/*[clinic end generated code: output=3d3ba484a90b3a99 input=7e34057aa303612c]*/
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
    {NULL},
};

static PyType_Slot blob_slots[] = {
    {Py_tp_dealloc, blob_dealloc},
    {Py_tp_traverse, blob_traverse},
    {Py_tp_clear, blob_clear},
    {Py_tp_methods, blob_methods},
    {Py_tp_members, blob_members},
    {0, NULL},
};

static PyType_Spec blob_spec = {
    .name = MODULE_NAME ".Blob",
    .basicsize = sizeof(pysqlite_Blob),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION),
    .slots = blob_slots,
};

int
pysqlite_blob_setup_types(PyObject *mod)
{
    PyObject *type = PyType_FromModuleAndSpec(mod, &blob_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    pysqlite_state *state = pysqlite_get_state(mod);
    state->BlobType = (PyTypeObject *)type;
    return 0;
}
