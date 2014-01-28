/* _bz2 - Low-level Python interface to libbzip2. */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h"

#ifdef WITH_THREAD
#include "pythread.h"
#endif

#include <bzlib.h>
#include <stdio.h>


#ifndef BZ_CONFIG_ERROR
#define BZ2_bzCompress bzCompress
#define BZ2_bzCompressInit bzCompressInit
#define BZ2_bzCompressEnd bzCompressEnd
#define BZ2_bzDecompress bzDecompress
#define BZ2_bzDecompressInit bzDecompressInit
#define BZ2_bzDecompressEnd bzDecompressEnd
#endif  /* ! BZ_CONFIG_ERROR */


#ifdef WITH_THREAD
#define ACQUIRE_LOCK(obj) do { \
    if (!PyThread_acquire_lock((obj)->lock, 0)) { \
        Py_BEGIN_ALLOW_THREADS \
        PyThread_acquire_lock((obj)->lock, 1); \
        Py_END_ALLOW_THREADS \
    } } while (0)
#define RELEASE_LOCK(obj) PyThread_release_lock((obj)->lock)
#else
#define ACQUIRE_LOCK(obj)
#define RELEASE_LOCK(obj)
#endif


typedef struct {
    PyObject_HEAD
    bz_stream bzs;
    int flushed;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} BZ2Compressor;

typedef struct {
    PyObject_HEAD
    bz_stream bzs;
    char eof;           /* T_BOOL expects a char */
    PyObject *unused_data;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} BZ2Decompressor;

static PyTypeObject BZ2Compressor_Type;
static PyTypeObject BZ2Decompressor_Type;

/* Helper functions. */

static int
catch_bz2_error(int bzerror)
{
    switch(bzerror) {
        case BZ_OK:
        case BZ_RUN_OK:
        case BZ_FLUSH_OK:
        case BZ_FINISH_OK:
        case BZ_STREAM_END:
            return 0;

#ifdef BZ_CONFIG_ERROR
        case BZ_CONFIG_ERROR:
            PyErr_SetString(PyExc_SystemError,
                            "libbzip2 was not compiled correctly");
            return 1;
#endif
        case BZ_PARAM_ERROR:
            PyErr_SetString(PyExc_ValueError,
                            "Internal error - "
                            "invalid parameters passed to libbzip2");
            return 1;
        case BZ_MEM_ERROR:
            PyErr_NoMemory();
            return 1;
        case BZ_DATA_ERROR:
        case BZ_DATA_ERROR_MAGIC:
            PyErr_SetString(PyExc_IOError, "Invalid data stream");
            return 1;
        case BZ_IO_ERROR:
            PyErr_SetString(PyExc_IOError, "Unknown I/O error");
            return 1;
        case BZ_UNEXPECTED_EOF:
            PyErr_SetString(PyExc_EOFError,
                            "Compressed file ended before the logical "
                            "end-of-stream was detected");
            return 1;
        case BZ_SEQUENCE_ERROR:
            PyErr_SetString(PyExc_RuntimeError,
                            "Internal error - "
                            "Invalid sequence of commands sent to libbzip2");
            return 1;
        default:
            PyErr_Format(PyExc_IOError,
                         "Unrecognized error from libbzip2: %d", bzerror);
            return 1;
    }
}

#if BUFSIZ < 8192
#define SMALLCHUNK 8192
#else
#define SMALLCHUNK BUFSIZ
#endif

static int
grow_buffer(PyObject **buf)
{
    /* Expand the buffer by an amount proportional to the current size,
       giving us amortized linear-time behavior. Use a less-than-double
       growth factor to avoid excessive allocation. */
    size_t size = PyBytes_GET_SIZE(*buf);
    size_t new_size = size + (size >> 3) + 6;
    if (new_size > size) {
        return _PyBytes_Resize(buf, new_size);
    } else {  /* overflow */
        PyErr_SetString(PyExc_OverflowError,
                        "Unable to allocate buffer - output too large");
        return -1;
    }
}


/* BZ2Compressor class. */

static PyObject *
compress(BZ2Compressor *c, char *data, size_t len, int action)
{
    size_t data_size = 0;
    PyObject *result;

    result = PyBytes_FromStringAndSize(NULL, SMALLCHUNK);
    if (result == NULL)
        return NULL;

    c->bzs.next_in = data;
    c->bzs.avail_in = 0;
    c->bzs.next_out = PyBytes_AS_STRING(result);
    c->bzs.avail_out = SMALLCHUNK;
    for (;;) {
        char *this_out;
        int bzerror;

        /* On a 64-bit system, len might not fit in avail_in (an unsigned int).
           Do compression in chunks of no more than UINT_MAX bytes each. */
        if (c->bzs.avail_in == 0 && len > 0) {
            c->bzs.avail_in = (unsigned int)Py_MIN(len, UINT_MAX);
            len -= c->bzs.avail_in;
        }

        /* In regular compression mode, stop when input data is exhausted. */
        if (action == BZ_RUN && c->bzs.avail_in == 0)
            break;

        if (c->bzs.avail_out == 0) {
            size_t buffer_left = PyBytes_GET_SIZE(result) - data_size;
            if (buffer_left == 0) {
                if (grow_buffer(&result) < 0)
                    goto error;
                c->bzs.next_out = PyBytes_AS_STRING(result) + data_size;
                buffer_left = PyBytes_GET_SIZE(result) - data_size;
            }
            c->bzs.avail_out = (unsigned int)Py_MIN(buffer_left, UINT_MAX);
        }

        Py_BEGIN_ALLOW_THREADS
        this_out = c->bzs.next_out;
        bzerror = BZ2_bzCompress(&c->bzs, action);
        data_size += c->bzs.next_out - this_out;
        Py_END_ALLOW_THREADS
        if (catch_bz2_error(bzerror))
            goto error;

        /* In flushing mode, stop when all buffered data has been flushed. */
        if (action == BZ_FINISH && bzerror == BZ_STREAM_END)
            break;
    }
    if (data_size != PyBytes_GET_SIZE(result))
        if (_PyBytes_Resize(&result, data_size) < 0)
            goto error;
    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
output preset file
module _bz2
class _bz2.BZ2Compressor "BZ2Compressor *" "&BZ2Compressor_Type"
class _bz2.BZ2Decompressor "BZ2Decompressor *" "&BZ2Decompressor_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e3b139924f5e18cc]*/

#include "clinic/_bz2module.c.h"

/*[clinic input]
_bz2.BZ2Compressor.compress

    data: Py_buffer
    /

Provide data to the compressor object.

Returns a chunk of compressed data if possible, or b'' otherwise.

When you have finished providing data to the compressor, call the
flush() method to finish the compression process.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Compressor_compress_impl(BZ2Compressor *self, Py_buffer *data)
/*[clinic end generated code: output=59365426e941fbcc input=85c963218070fc4c]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed)
        PyErr_SetString(PyExc_ValueError, "Compressor has been flushed");
    else
        result = compress(self, data->buf, data->len, BZ_RUN);
    RELEASE_LOCK(self);
    return result;
}

/*[clinic input]
_bz2.BZ2Compressor.flush

Finish the compression process.

Returns the compressed data left in internal buffers.

The compressor object may not be used after this method is called.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Compressor_flush_impl(BZ2Compressor *self)
/*[clinic end generated code: output=3ef03fc1b092a701 input=d64405d3c6f76691]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed)
        PyErr_SetString(PyExc_ValueError, "Repeated call to flush()");
    else {
        self->flushed = 1;
        result = compress(self, NULL, 0, BZ_FINISH);
    }
    RELEASE_LOCK(self);
    return result;
}

static PyObject *
BZ2Compressor_getstate(BZ2Compressor *self, PyObject *noargs)
{
    PyErr_Format(PyExc_TypeError, "cannot serialize '%s' object",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

static void*
BZ2_Malloc(void* ctx, int items, int size)
{
    if (items < 0 || size < 0)
        return NULL;
    if ((size_t)items > (size_t)PY_SSIZE_T_MAX / (size_t)size)
        return NULL;
    /* PyMem_Malloc() cannot be used: compress() and decompress()
       release the GIL */
    return PyMem_RawMalloc(items * size);
}

static void
BZ2_Free(void* ctx, void *ptr)
{
    PyMem_RawFree(ptr);
}

/*[clinic input]
_bz2.BZ2Compressor.__init__

    compresslevel: int = 9
        Compression level, as a number between 1 and 9.
    /

Create a compressor object for compressing data incrementally.

For one-shot compression, use the compress() function instead.
[clinic start generated code]*/

static int
_bz2_BZ2Compressor___init___impl(BZ2Compressor *self, int compresslevel)
/*[clinic end generated code: output=c4e6adfd02963827 input=4e1ff7b8394b6e9a]*/
{
    int bzerror;

    if (!(1 <= compresslevel && compresslevel <= 9)) {
        PyErr_SetString(PyExc_ValueError,
                        "compresslevel must be between 1 and 9");
        return -1;
    }

#ifdef WITH_THREAD
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return -1;
    }
#endif

    self->bzs.opaque = NULL;
    self->bzs.bzalloc = BZ2_Malloc;
    self->bzs.bzfree = BZ2_Free;
    bzerror = BZ2_bzCompressInit(&self->bzs, compresslevel, 0, 0);
    if (catch_bz2_error(bzerror))
        goto error;

    return 0;

error:
#ifdef WITH_THREAD
    PyThread_free_lock(self->lock);
    self->lock = NULL;
#endif
    return -1;
}

static void
BZ2Compressor_dealloc(BZ2Compressor *self)
{
    BZ2_bzCompressEnd(&self->bzs);
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef BZ2Compressor_methods[] = {
    _BZ2_BZ2COMPRESSOR_COMPRESS_METHODDEF
    _BZ2_BZ2COMPRESSOR_FLUSH_METHODDEF
    {"__getstate__", (PyCFunction)BZ2Compressor_getstate, METH_NOARGS},
    {NULL}
};


static PyTypeObject BZ2Compressor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_bz2.BZ2Compressor",               /* tp_name */
    sizeof(BZ2Compressor),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)BZ2Compressor_dealloc,  /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    _bz2_BZ2Compressor___init____doc__,  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    BZ2Compressor_methods,              /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    _bz2_BZ2Compressor___init__,        /* tp_init */
    0,                                  /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};


/* BZ2Decompressor class. */

static PyObject *
decompress(BZ2Decompressor *d, char *data, size_t len)
{
    size_t data_size = 0;
    PyObject *result;

    result = PyBytes_FromStringAndSize(NULL, SMALLCHUNK);
    if (result == NULL)
        return result;
    d->bzs.next_in = data;
    /* On a 64-bit system, len might not fit in avail_in (an unsigned int).
       Do decompression in chunks of no more than UINT_MAX bytes each. */
    d->bzs.avail_in = (unsigned int)Py_MIN(len, UINT_MAX);
    len -= d->bzs.avail_in;
    d->bzs.next_out = PyBytes_AS_STRING(result);
    d->bzs.avail_out = SMALLCHUNK;
    for (;;) {
        char *this_out;
        int bzerror;

        Py_BEGIN_ALLOW_THREADS
        this_out = d->bzs.next_out;
        bzerror = BZ2_bzDecompress(&d->bzs);
        data_size += d->bzs.next_out - this_out;
        Py_END_ALLOW_THREADS
        if (catch_bz2_error(bzerror))
            goto error;
        if (bzerror == BZ_STREAM_END) {
            d->eof = 1;
            len += d->bzs.avail_in;
            if (len > 0) { /* Save leftover input to unused_data */
                Py_CLEAR(d->unused_data);
                d->unused_data = PyBytes_FromStringAndSize(d->bzs.next_in, len);
                if (d->unused_data == NULL)
                    goto error;
            }
            break;
        }
        if (d->bzs.avail_in == 0) {
            if (len == 0)
                break;
            d->bzs.avail_in = (unsigned int)Py_MIN(len, UINT_MAX);
            len -= d->bzs.avail_in;
        }
        if (d->bzs.avail_out == 0) {
            size_t buffer_left = PyBytes_GET_SIZE(result) - data_size;
            if (buffer_left == 0) {
                if (grow_buffer(&result) < 0)
                    goto error;
                d->bzs.next_out = PyBytes_AS_STRING(result) + data_size;
                buffer_left = PyBytes_GET_SIZE(result) - data_size;
            }
            d->bzs.avail_out = (unsigned int)Py_MIN(buffer_left, UINT_MAX);
        }
    }
    if (data_size != PyBytes_GET_SIZE(result))
        if (_PyBytes_Resize(&result, data_size) < 0)
            goto error;
    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
_bz2.BZ2Decompressor.decompress

    data: Py_buffer
    /

Provide data to the decompressor object.

Returns a chunk of decompressed data if possible, or b'' otherwise.

Attempting to decompress data after the end of stream is reached
raises an EOFError.  Any data found after the end of the stream
is ignored and saved in the unused_data attribute.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Decompressor_decompress_impl(BZ2Decompressor *self, Py_buffer *data)
/*[clinic end generated code: output=086e4b99e60cb3f6 input=616c2a6db5269961]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->eof)
        PyErr_SetString(PyExc_EOFError, "End of stream already reached");
    else
        result = decompress(self, data->buf, data->len);
    RELEASE_LOCK(self);
    return result;
}

static PyObject *
BZ2Decompressor_getstate(BZ2Decompressor *self, PyObject *noargs)
{
    PyErr_Format(PyExc_TypeError, "cannot serialize '%s' object",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

/*[clinic input]
_bz2.BZ2Decompressor.__init__

Create a decompressor object for decompressing data incrementally.

For one-shot decompression, use the decompress() function instead.
[clinic start generated code]*/

static int
_bz2_BZ2Decompressor___init___impl(BZ2Decompressor *self)
/*[clinic end generated code: output=e4d2b9bb866ab8f1 input=95f6500dcda60088]*/
{
    int bzerror;

#ifdef WITH_THREAD
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return -1;
    }
#endif

    self->unused_data = PyBytes_FromStringAndSize("", 0);
    if (self->unused_data == NULL)
        goto error;

    bzerror = BZ2_bzDecompressInit(&self->bzs, 0, 0);
    if (catch_bz2_error(bzerror))
        goto error;

    return 0;

error:
    Py_CLEAR(self->unused_data);
#ifdef WITH_THREAD
    PyThread_free_lock(self->lock);
    self->lock = NULL;
#endif
    return -1;
}

static void
BZ2Decompressor_dealloc(BZ2Decompressor *self)
{
    BZ2_bzDecompressEnd(&self->bzs);
    Py_CLEAR(self->unused_data);
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef BZ2Decompressor_methods[] = {
    _BZ2_BZ2DECOMPRESSOR_DECOMPRESS_METHODDEF
    {"__getstate__", (PyCFunction)BZ2Decompressor_getstate, METH_NOARGS},
    {NULL}
};

PyDoc_STRVAR(BZ2Decompressor_eof__doc__,
"True if the end-of-stream marker has been reached.");

PyDoc_STRVAR(BZ2Decompressor_unused_data__doc__,
"Data found after the end of the compressed stream.");

static PyMemberDef BZ2Decompressor_members[] = {
    {"eof", T_BOOL, offsetof(BZ2Decompressor, eof),
     READONLY, BZ2Decompressor_eof__doc__},
    {"unused_data", T_OBJECT_EX, offsetof(BZ2Decompressor, unused_data),
     READONLY, BZ2Decompressor_unused_data__doc__},
    {NULL}
};

static PyTypeObject BZ2Decompressor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_bz2.BZ2Decompressor",             /* tp_name */
    sizeof(BZ2Decompressor),            /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)BZ2Decompressor_dealloc,/* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    _bz2_BZ2Decompressor___init____doc__,  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    BZ2Decompressor_methods,            /* tp_methods */
    BZ2Decompressor_members,            /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    _bz2_BZ2Decompressor___init__,      /* tp_init */
    0,                                  /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};


/* Module initialization. */

static struct PyModuleDef _bz2module = {
    PyModuleDef_HEAD_INIT,
    "_bz2",
    NULL,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__bz2(void)
{
    PyObject *m;

    if (PyType_Ready(&BZ2Compressor_Type) < 0)
        return NULL;
    if (PyType_Ready(&BZ2Decompressor_Type) < 0)
        return NULL;

    m = PyModule_Create(&_bz2module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&BZ2Compressor_Type);
    PyModule_AddObject(m, "BZ2Compressor", (PyObject *)&BZ2Compressor_Type);

    Py_INCREF(&BZ2Decompressor_Type);
    PyModule_AddObject(m, "BZ2Decompressor",
                       (PyObject *)&BZ2Decompressor_Type);

    return m;
}
