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

#if SIZEOF_INT < 4
#define BIGCHUNK  (512 * 32)
#else
#define BIGCHUNK  (512 * 1024)
#endif

static int
grow_buffer(PyObject **buf)
{
    size_t size = PyBytes_GET_SIZE(*buf);
    if (size <= SMALLCHUNK)
        return _PyBytes_Resize(buf, size + SMALLCHUNK);
    else if (size <= BIGCHUNK)
        return _PyBytes_Resize(buf, size * 2);
    else
        return _PyBytes_Resize(buf, size + BIGCHUNK);
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
    /* FIXME This is not 64-bit clean - avail_in is an int. */
    c->bzs.avail_in = len;
    c->bzs.next_out = PyBytes_AS_STRING(result);
    c->bzs.avail_out = PyBytes_GET_SIZE(result);
    for (;;) {
        char *this_out;
        int bzerror;

        Py_BEGIN_ALLOW_THREADS
        this_out = c->bzs.next_out;
        bzerror = BZ2_bzCompress(&c->bzs, action);
        data_size += c->bzs.next_out - this_out;
        Py_END_ALLOW_THREADS
        if (catch_bz2_error(bzerror))
            goto error;

        /* In regular compression mode, stop when input data is exhausted.
           In flushing mode, stop when all buffered data has been flushed. */
        if ((action == BZ_RUN && c->bzs.avail_in == 0) ||
            (action == BZ_FINISH && bzerror == BZ_STREAM_END))
            break;

        if (c->bzs.avail_out == 0) {
            if (grow_buffer(&result) < 0)
                goto error;
            c->bzs.next_out = PyBytes_AS_STRING(result) + data_size;
            c->bzs.avail_out = PyBytes_GET_SIZE(result) - data_size;
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

PyDoc_STRVAR(BZ2Compressor_compress__doc__,
"compress(data) -> bytes\n"
"\n"
"Provide data to the compressor object. Returns a chunk of\n"
"compressed data if possible, or b'' otherwise.\n"
"\n"
"When you have finished providing data to the compressor, call the\n"
"flush() method to finish the compression process.\n");

static PyObject *
BZ2Compressor_compress(BZ2Compressor *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "y*:compress", &buffer))
        return NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed)
        PyErr_SetString(PyExc_ValueError, "Compressor has been flushed");
    else
        result = compress(self, buffer.buf, buffer.len, BZ_RUN);
    RELEASE_LOCK(self);
    PyBuffer_Release(&buffer);
    return result;
}

PyDoc_STRVAR(BZ2Compressor_flush__doc__,
"flush() -> bytes\n"
"\n"
"Finish the compression process. Returns the compressed data left\n"
"in internal buffers.\n"
"\n"
"The compressor object may not be used after this method is called.\n");

static PyObject *
BZ2Compressor_flush(BZ2Compressor *self, PyObject *noargs)
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

static int
BZ2Compressor_init(BZ2Compressor *self, PyObject *args, PyObject *kwargs)
{
    int compresslevel = 9;
    int bzerror;

    if (!PyArg_ParseTuple(args, "|i:BZ2Compressor", &compresslevel))
        return -1;
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
    {"compress", (PyCFunction)BZ2Compressor_compress, METH_VARARGS,
     BZ2Compressor_compress__doc__},
    {"flush",    (PyCFunction)BZ2Compressor_flush,    METH_NOARGS,
     BZ2Compressor_flush__doc__},
    {NULL}
};

PyDoc_STRVAR(BZ2Compressor__doc__,
"BZ2Compressor(compresslevel=9)\n"
"\n"
"Create a compressor object for compressing data incrementally.\n"
"\n"
"compresslevel, if given, must be a number between 1 and 9.\n"
"\n"
"For one-shot compression, use the compress() function instead.\n");

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
    BZ2Compressor__doc__,               /* tp_doc */
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
    (initproc)BZ2Compressor_init,       /* tp_init */
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
    /* FIXME This is not 64-bit clean - avail_in is an int. */
    d->bzs.avail_in = len;
    d->bzs.next_out = PyBytes_AS_STRING(result);
    d->bzs.avail_out = PyBytes_GET_SIZE(result);
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
            if (d->bzs.avail_in > 0) { /* Save leftover input to unused_data */
                Py_CLEAR(d->unused_data);
                d->unused_data = PyBytes_FromStringAndSize(d->bzs.next_in,
                                                           d->bzs.avail_in);
                if (d->unused_data == NULL)
                    goto error;
            }
            break;
        }
        if (d->bzs.avail_in == 0)
            break;
        if (d->bzs.avail_out == 0) {
            if (grow_buffer(&result) < 0)
                goto error;
            d->bzs.next_out = PyBytes_AS_STRING(result) + data_size;
            d->bzs.avail_out = PyBytes_GET_SIZE(result) - data_size;
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

PyDoc_STRVAR(BZ2Decompressor_decompress__doc__,
"decompress(data) -> bytes\n"
"\n"
"Provide data to the decompressor object. Returns a chunk of\n"
"decompressed data if possible, or b'' otherwise.\n"
"\n"
"Attempting to decompress data after the end of stream is reached\n"
"raises an EOFError. Any data found after the end of the stream\n"
"is ignored and saved in the unused_data attribute.\n");

static PyObject *
BZ2Decompressor_decompress(BZ2Decompressor *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "y*:decompress", &buffer))
        return NULL;

    ACQUIRE_LOCK(self);
    if (self->eof)
        PyErr_SetString(PyExc_EOFError, "End of stream already reached");
    else
        result = decompress(self, buffer.buf, buffer.len);
    RELEASE_LOCK(self);
    PyBuffer_Release(&buffer);
    return result;
}

static int
BZ2Decompressor_init(BZ2Decompressor *self, PyObject *args, PyObject *kwargs)
{
    int bzerror;

    if (!PyArg_ParseTuple(args, ":BZ2Decompressor"))
        return -1;

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
    {"decompress", (PyCFunction)BZ2Decompressor_decompress, METH_VARARGS,
     BZ2Decompressor_decompress__doc__},
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

PyDoc_STRVAR(BZ2Decompressor__doc__,
"BZ2Decompressor()\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"For one-shot decompression, use the decompress() function instead.\n");

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
    BZ2Decompressor__doc__,             /* tp_doc */
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
    (initproc)BZ2Decompressor_init,     /* tp_init */
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
