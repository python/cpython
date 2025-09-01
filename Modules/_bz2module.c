/* _bz2 - Low-level Python interface to libbzip2. */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include <bzlib.h>
#include <stdio.h>
#include <stddef.h>               // offsetof()

// Blocks output buffer wrappers
#include "pycore_blocks_output_buffer.h"

#if OUTPUT_BUFFER_MAX_BLOCK_SIZE > UINT32_MAX
    #error "The maximum block size accepted by libbzip2 is UINT32_MAX."
#endif

typedef struct {
    PyTypeObject *bz2_compressor_type;
    PyTypeObject *bz2_decompressor_type;
} _bz2_state;

static inline _bz2_state *
get_module_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_bz2_state *)state;
}

static struct PyModuleDef _bz2module;

static inline _bz2_state *
find_module_state_by_def(PyTypeObject *type)
{
    PyObject *module = PyType_GetModuleByDef(type, &_bz2module);
    assert(module != NULL);
    return get_module_state(module);
}

/* On success, return value >= 0
   On failure, return -1 */
static inline Py_ssize_t
OutputBuffer_InitAndGrow(_BlocksOutputBuffer *buffer, Py_ssize_t max_length,
                         char **next_out, uint32_t *avail_out)
{
    Py_ssize_t allocated;

    allocated = _BlocksOutputBuffer_InitAndGrow(
                    buffer, max_length, (void**) next_out);
    *avail_out = (uint32_t) allocated;
    return allocated;
}

/* On success, return value >= 0
   On failure, return -1 */
static inline Py_ssize_t
OutputBuffer_Grow(_BlocksOutputBuffer *buffer,
                  char **next_out, uint32_t *avail_out)
{
    Py_ssize_t allocated;

    allocated = _BlocksOutputBuffer_Grow(
                    buffer, (void**) next_out, (Py_ssize_t) *avail_out);
    *avail_out = (uint32_t) allocated;
    return allocated;
}

static inline Py_ssize_t
OutputBuffer_GetDataSize(_BlocksOutputBuffer *buffer, uint32_t avail_out)
{
    return _BlocksOutputBuffer_GetDataSize(buffer, (Py_ssize_t) avail_out);
}

static inline PyObject *
OutputBuffer_Finish(_BlocksOutputBuffer *buffer, uint32_t avail_out)
{
    return _BlocksOutputBuffer_Finish(buffer, (Py_ssize_t) avail_out);
}

static inline void
OutputBuffer_OnError(_BlocksOutputBuffer *buffer)
{
    _BlocksOutputBuffer_OnError(buffer);
}


#ifndef BZ_CONFIG_ERROR
#define BZ2_bzCompress bzCompress
#define BZ2_bzCompressInit bzCompressInit
#define BZ2_bzCompressEnd bzCompressEnd
#define BZ2_bzDecompress bzDecompress
#define BZ2_bzDecompressInit bzDecompressInit
#define BZ2_bzDecompressEnd bzDecompressEnd
#endif  /* ! BZ_CONFIG_ERROR */


#define ACQUIRE_LOCK(obj) do { \
    if (!PyThread_acquire_lock((obj)->lock, 0)) { \
        Py_BEGIN_ALLOW_THREADS \
        PyThread_acquire_lock((obj)->lock, 1); \
        Py_END_ALLOW_THREADS \
    } } while (0)
#define RELEASE_LOCK(obj) PyThread_release_lock((obj)->lock)


typedef struct {
    PyObject_HEAD
    bz_stream bzs;
    int flushed;
    PyThread_type_lock lock;
} BZ2Compressor;

typedef struct {
    PyObject_HEAD
    bz_stream bzs;
    char eof;           /* Py_T_BOOL expects a char */
    PyObject *unused_data;
    char needs_input;
    char *input_buffer;
    size_t input_buffer_size;

    /* bzs->avail_in is only 32 bit, so we store the true length
       separately. Conversion and looping is encapsulated in
       decompress_buf() */
    size_t bzs_avail_in_real;
    PyThread_type_lock lock;
} BZ2Decompressor;

#define _BZ2Compressor_CAST(op)     ((BZ2Compressor *)(op))
#define _BZ2Decompressor_CAST(op)   ((BZ2Decompressor *)(op))

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
            PyErr_SetString(PyExc_OSError, "Invalid data stream");
            return 1;
        case BZ_IO_ERROR:
            PyErr_SetString(PyExc_OSError, "Unknown I/O error");
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
            PyErr_Format(PyExc_OSError,
                         "Unrecognized error from libbzip2: %d", bzerror);
            return 1;
    }
}


/* BZ2Compressor class. */

static PyObject *
compress(BZ2Compressor *c, char *data, size_t len, int action)
{
    PyObject *result;
    _BlocksOutputBuffer buffer = {.list = NULL};

    if (OutputBuffer_InitAndGrow(&buffer, -1, &c->bzs.next_out, &c->bzs.avail_out) < 0) {
        goto error;
    }
    c->bzs.next_in = data;
    c->bzs.avail_in = 0;

    for (;;) {
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
            if (OutputBuffer_Grow(&buffer, &c->bzs.next_out, &c->bzs.avail_out) < 0) {
                goto error;
            }
        }

        Py_BEGIN_ALLOW_THREADS
        bzerror = BZ2_bzCompress(&c->bzs, action);
        Py_END_ALLOW_THREADS

        if (catch_bz2_error(bzerror))
            goto error;

        /* In flushing mode, stop when all buffered data has been flushed. */
        if (action == BZ_FINISH && bzerror == BZ_STREAM_END)
            break;
    }

    result = OutputBuffer_Finish(&buffer, c->bzs.avail_out);
    if (result != NULL) {
        return result;
    }

error:
    OutputBuffer_OnError(&buffer);
    return NULL;
}

/*[clinic input]
module _bz2
class _bz2.BZ2Compressor "BZ2Compressor *" "clinic_state()->bz2_compressor_type"
class _bz2.BZ2Decompressor "BZ2Decompressor *" "clinic_state()->bz2_decompressor_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=92348121632b94c4]*/

#define clinic_state() (find_module_state_by_def(type))
#include "clinic/_bz2module.c.h"
#undef clinic_state

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

static void*
BZ2_Malloc(void* ctx, int items, int size)
{
    if (items < 0 || size < 0)
        return NULL;
    if (size != 0 && (size_t)items > (size_t)PY_SSIZE_T_MAX / (size_t)size)
        return NULL;
    /* PyMem_Malloc() cannot be used: compress() and decompress()
       release the GIL */
    return PyMem_RawMalloc((size_t)items * (size_t)size);
}

static void
BZ2_Free(void* ctx, void *ptr)
{
    PyMem_RawFree(ptr);
}

/*[clinic input]
@classmethod
_bz2.BZ2Compressor.__new__

    compresslevel: int = 9
        Compression level, as a number between 1 and 9.
    /

Create a compressor object for compressing data incrementally.

For one-shot compression, use the compress() function instead.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Compressor_impl(PyTypeObject *type, int compresslevel)
/*[clinic end generated code: output=83346c96beaacad7 input=d4500d2a52c8b263]*/
{
    int bzerror;
    BZ2Compressor *self;

    if (!(1 <= compresslevel && compresslevel <= 9)) {
        PyErr_SetString(PyExc_ValueError,
                        "compresslevel must be between 1 and 9");
        return NULL;
    }

    assert(type != NULL && type->tp_alloc != NULL);
    self = (BZ2Compressor *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        Py_DECREF(self);
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }

    self->bzs.opaque = NULL;
    self->bzs.bzalloc = BZ2_Malloc;
    self->bzs.bzfree = BZ2_Free;
    bzerror = BZ2_bzCompressInit(&self->bzs, compresslevel, 0, 0);
    if (catch_bz2_error(bzerror))
        goto error;

    return (PyObject *)self;

error:
    Py_DECREF(self);
    return NULL;
}

static void
BZ2Compressor_dealloc(PyObject *op)
{
    BZ2Compressor *self = _BZ2Compressor_CAST(op);
    BZ2_bzCompressEnd(&self->bzs);
    if (self->lock != NULL) {
        PyThread_free_lock(self->lock);
    }
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

static int
BZ2Compressor_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyMethodDef BZ2Compressor_methods[] = {
    _BZ2_BZ2COMPRESSOR_COMPRESS_METHODDEF
    _BZ2_BZ2COMPRESSOR_FLUSH_METHODDEF
    {NULL}
};

static PyType_Slot bz2_compressor_type_slots[] = {
    {Py_tp_dealloc, BZ2Compressor_dealloc},
    {Py_tp_methods, BZ2Compressor_methods},
    {Py_tp_new, _bz2_BZ2Compressor},
    {Py_tp_doc, (char *)_bz2_BZ2Compressor__doc__},
    {Py_tp_traverse, BZ2Compressor_traverse},
    {0, 0}
};

static PyType_Spec bz2_compressor_type_spec = {
    .name = "_bz2.BZ2Compressor",
    .basicsize = sizeof(BZ2Compressor),
    // Calling PyType_GetModuleState() on a subclass is not safe.
    // bz2_compressor_type_spec does not have Py_TPFLAGS_BASETYPE flag
    // which prevents to create a subclass.
    // So calling PyType_GetModuleState() in this file is always safe.
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = bz2_compressor_type_slots,
};

/* BZ2Decompressor class. */

/* Decompress data of length d->bzs_avail_in_real in d->bzs.next_in.  The output
   buffer is allocated dynamically and returned.  At most max_length bytes are
   returned, so some of the input may not be consumed. d->bzs.next_in and
   d->bzs_avail_in_real are updated to reflect the consumed input. */
static PyObject*
decompress_buf(BZ2Decompressor *d, Py_ssize_t max_length)
{
    /* data_size is strictly positive, but because we repeatedly have to
       compare against max_length and PyBytes_GET_SIZE we declare it as
       signed */
    PyObject *result;
    _BlocksOutputBuffer buffer = {.list = NULL};
    bz_stream *bzs = &d->bzs;

    if (OutputBuffer_InitAndGrow(&buffer, max_length, &bzs->next_out, &bzs->avail_out) < 0) {
        goto error;
    }

    for (;;) {
        int bzret;
        /* On a 64-bit system, buffer length might not fit in avail_out, so we
           do decompression in chunks of no more than UINT_MAX bytes
           each. Note that the expression for `avail` is guaranteed to be
           positive, so the cast is safe. */
        bzs->avail_in = (unsigned int)Py_MIN(d->bzs_avail_in_real, UINT_MAX);
        d->bzs_avail_in_real -= bzs->avail_in;

        Py_BEGIN_ALLOW_THREADS
        bzret = BZ2_bzDecompress(bzs);
        Py_END_ALLOW_THREADS

        d->bzs_avail_in_real += bzs->avail_in;

        if (catch_bz2_error(bzret))
            goto error;
        if (bzret == BZ_STREAM_END) {
            d->eof = 1;
            break;
        } else if (d->bzs_avail_in_real == 0) {
            break;
        } else if (bzs->avail_out == 0) {
            if (OutputBuffer_GetDataSize(&buffer, bzs->avail_out) == max_length) {
                break;
            }
            if (OutputBuffer_Grow(&buffer, &bzs->next_out, &bzs->avail_out) < 0) {
                goto error;
            }
        }
    }

    result = OutputBuffer_Finish(&buffer, bzs->avail_out);
    if (result != NULL) {
        return result;
    }

error:
    OutputBuffer_OnError(&buffer);
    return NULL;
}


static PyObject *
decompress(BZ2Decompressor *d, char *data, size_t len, Py_ssize_t max_length)
{
    char input_buffer_in_use;
    PyObject *result;
    bz_stream *bzs = &d->bzs;

    /* Prepend unconsumed input if necessary */
    if (bzs->next_in != NULL) {
        size_t avail_now, avail_total;

        /* Number of bytes we can append to input buffer */
        avail_now = (d->input_buffer + d->input_buffer_size)
            - (bzs->next_in + d->bzs_avail_in_real);

        /* Number of bytes we can append if we move existing
           contents to beginning of buffer (overwriting
           consumed input) */
        avail_total = d->input_buffer_size - d->bzs_avail_in_real;

        if (avail_total < len) {
            size_t offset = bzs->next_in - d->input_buffer;
            char *tmp;
            size_t new_size = d->input_buffer_size + len - avail_now;

            /* Assign to temporary variable first, so we don't
               lose address of allocated buffer if realloc fails */
            tmp = PyMem_Realloc(d->input_buffer, new_size);
            if (tmp == NULL) {
                PyErr_SetNone(PyExc_MemoryError);
                return NULL;
            }
            d->input_buffer = tmp;
            d->input_buffer_size = new_size;

            bzs->next_in = d->input_buffer + offset;
        }
        else if (avail_now < len) {
            memmove(d->input_buffer, bzs->next_in,
                    d->bzs_avail_in_real);
            bzs->next_in = d->input_buffer;
        }
        memcpy((void*)(bzs->next_in + d->bzs_avail_in_real), data, len);
        d->bzs_avail_in_real += len;
        input_buffer_in_use = 1;
    }
    else {
        bzs->next_in = data;
        d->bzs_avail_in_real = len;
        input_buffer_in_use = 0;
    }

    result = decompress_buf(d, max_length);
    if(result == NULL) {
        bzs->next_in = NULL;
        return NULL;
    }

    if (d->eof) {
        d->needs_input = 0;
        if (d->bzs_avail_in_real > 0) {
            Py_XSETREF(d->unused_data,
                      PyBytes_FromStringAndSize(bzs->next_in, d->bzs_avail_in_real));
            if (d->unused_data == NULL)
                goto error;
        }
    }
    else if (d->bzs_avail_in_real == 0) {
        bzs->next_in = NULL;
        d->needs_input = 1;
    }
    else {
        d->needs_input = 0;

        /* If we did not use the input buffer, we now have
           to copy the tail from the caller's buffer into the
           input buffer */
        if (!input_buffer_in_use) {

            /* Discard buffer if it's too small
               (resizing it may needlessly copy the current contents) */
            if (d->input_buffer != NULL &&
                d->input_buffer_size < d->bzs_avail_in_real) {
                PyMem_Free(d->input_buffer);
                d->input_buffer = NULL;
            }

            /* Allocate if necessary */
            if (d->input_buffer == NULL) {
                d->input_buffer = PyMem_Malloc(d->bzs_avail_in_real);
                if (d->input_buffer == NULL) {
                    PyErr_SetNone(PyExc_MemoryError);
                    goto error;
                }
                d->input_buffer_size = d->bzs_avail_in_real;
            }

            /* Copy tail */
            memcpy(d->input_buffer, bzs->next_in, d->bzs_avail_in_real);
            bzs->next_in = d->input_buffer;
        }
    }

    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
@permit_long_docstring_body
_bz2.BZ2Decompressor.decompress

    data: Py_buffer
    max_length: Py_ssize_t=-1

Decompress *data*, returning uncompressed data as bytes.

If *max_length* is nonnegative, returns at most *max_length* bytes of
decompressed data. If this limit is reached and further output can be
produced, *self.needs_input* will be set to ``False``. In this case, the next
call to *decompress()* may provide *data* as b'' to obtain more of the output.

If all of the input data was decompressed and returned (either because this
was less than *max_length* bytes, or because *max_length* was negative),
*self.needs_input* will be set to True.

Attempting to decompress data after the end of stream is reached raises an
EOFError.  Any data found after the end of the stream is ignored and saved in
the unused_data attribute.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Decompressor_decompress_impl(BZ2Decompressor *self, Py_buffer *data,
                                     Py_ssize_t max_length)
/*[clinic end generated code: output=23e41045deb240a3 input=3703e78f91757655]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->eof)
        PyErr_SetString(PyExc_EOFError, "End of stream already reached");
    else
        result = decompress(self, data->buf, data->len, max_length);
    RELEASE_LOCK(self);
    return result;
}

/*[clinic input]
@classmethod
_bz2.BZ2Decompressor.__new__

Create a decompressor object for decompressing data incrementally.

For one-shot decompression, use the decompress() function instead.
[clinic start generated code]*/

static PyObject *
_bz2_BZ2Decompressor_impl(PyTypeObject *type)
/*[clinic end generated code: output=5150d51ccaab220e input=b87413ce51853528]*/
{
    BZ2Decompressor *self;
    int bzerror;

    assert(type != NULL && type->tp_alloc != NULL);
    self = (BZ2Decompressor *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        Py_DECREF(self);
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }

    self->needs_input = 1;
    self->bzs_avail_in_real = 0;
    self->input_buffer = NULL;
    self->input_buffer_size = 0;
    self->unused_data = PyBytes_FromStringAndSize(NULL, 0);
    if (self->unused_data == NULL)
        goto error;

    bzerror = BZ2_bzDecompressInit(&self->bzs, 0, 0);
    if (catch_bz2_error(bzerror))
        goto error;

    return (PyObject *)self;

error:
    Py_DECREF(self);
    return NULL;
}

static void
BZ2Decompressor_dealloc(PyObject *op)
{
    BZ2Decompressor *self = _BZ2Decompressor_CAST(op);

    if(self->input_buffer != NULL) {
        PyMem_Free(self->input_buffer);
    }
    BZ2_bzDecompressEnd(&self->bzs);
    Py_CLEAR(self->unused_data);
    if (self->lock != NULL) {
        PyThread_free_lock(self->lock);
    }

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

static int
BZ2Decompressor_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyMethodDef BZ2Decompressor_methods[] = {
    _BZ2_BZ2DECOMPRESSOR_DECOMPRESS_METHODDEF
    {NULL}
};

PyDoc_STRVAR(BZ2Decompressor_eof__doc__,
"True if the end-of-stream marker has been reached.");

PyDoc_STRVAR(BZ2Decompressor_unused_data__doc__,
"Data found after the end of the compressed stream.");

PyDoc_STRVAR(BZ2Decompressor_needs_input_doc,
"True if more input is needed before more decompressed data can be produced.");

static PyMemberDef BZ2Decompressor_members[] = {
    {"eof", Py_T_BOOL, offsetof(BZ2Decompressor, eof),
     Py_READONLY, BZ2Decompressor_eof__doc__},
    {"unused_data", Py_T_OBJECT_EX, offsetof(BZ2Decompressor, unused_data),
     Py_READONLY, BZ2Decompressor_unused_data__doc__},
    {"needs_input", Py_T_BOOL, offsetof(BZ2Decompressor, needs_input), Py_READONLY,
     BZ2Decompressor_needs_input_doc},
    {NULL}
};

static PyType_Slot bz2_decompressor_type_slots[] = {
    {Py_tp_dealloc, BZ2Decompressor_dealloc},
    {Py_tp_methods, BZ2Decompressor_methods},
    {Py_tp_doc, (char *)_bz2_BZ2Decompressor__doc__},
    {Py_tp_members, BZ2Decompressor_members},
    {Py_tp_new, _bz2_BZ2Decompressor},
    {Py_tp_traverse, BZ2Decompressor_traverse},
    {0, 0}
};

static PyType_Spec bz2_decompressor_type_spec = {
    .name = "_bz2.BZ2Decompressor",
    .basicsize = sizeof(BZ2Decompressor),
    // Calling PyType_GetModuleState() on a subclass is not safe.
    // bz2_decompressor_type_spec does not have Py_TPFLAGS_BASETYPE flag
    // which prevents to create a subclass.
    // So calling PyType_GetModuleState() in this file is always safe.
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = bz2_decompressor_type_slots,
};

/* Module initialization. */

static int
_bz2_exec(PyObject *module)
{
    _bz2_state *state = get_module_state(module);
    state->bz2_compressor_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
                                                            &bz2_compressor_type_spec, NULL);
    if (state->bz2_compressor_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->bz2_compressor_type) < 0) {
        return -1;
    }

    state->bz2_decompressor_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
                                                         &bz2_decompressor_type_spec, NULL);
    if (state->bz2_decompressor_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->bz2_decompressor_type) < 0) {
        return -1;
    }

    return 0;
}

static int
_bz2_traverse(PyObject *module, visitproc visit, void *arg)
{
    _bz2_state *state = get_module_state(module);
    Py_VISIT(state->bz2_compressor_type);
    Py_VISIT(state->bz2_decompressor_type);
    return 0;
}

static int
_bz2_clear(PyObject *module)
{
    _bz2_state *state = get_module_state(module);
    Py_CLEAR(state->bz2_compressor_type);
    Py_CLEAR(state->bz2_decompressor_type);
    return 0;
}

static void
_bz2_free(void *module)
{
    (void)_bz2_clear((PyObject *)module);
}

static struct PyModuleDef_Slot _bz2_slots[] = {
    {Py_mod_exec, _bz2_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _bz2module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_bz2",
    .m_size = sizeof(_bz2_state),
    .m_traverse = _bz2_traverse,
    .m_clear = _bz2_clear,
    .m_free = _bz2_free,
    .m_slots = _bz2_slots,
};

PyMODINIT_FUNC
PyInit__bz2(void)
{
    return PyModuleDef_Init(&_bz2module);
}
