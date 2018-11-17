/* zlibmodule.c -- gzip-compatible data compression */
/* See http://zlib.net/ */

/* Windows users:  read Python's PCbuild\readme.txt */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h"
#include "zlib.h"


#include "pythread.h"
#define ENTER_ZLIB(obj) \
    Py_BEGIN_ALLOW_THREADS; \
    PyThread_acquire_lock((obj)->lock, 1); \
    Py_END_ALLOW_THREADS;
#define LEAVE_ZLIB(obj) PyThread_release_lock((obj)->lock);

#if defined(ZLIB_VERNUM) && ZLIB_VERNUM >= 0x1221
#  define AT_LEAST_ZLIB_1_2_2_1
#endif

/* The following parameters are copied from zutil.h, version 0.95 */
#define DEFLATED   8
#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

/* Initial buffer size. */
#define DEF_BUF_SIZE (16*1024)

static PyTypeObject Comptype;
static PyTypeObject Decomptype;

static PyObject *ZlibError;

typedef struct
{
    PyObject_HEAD
    z_stream zst;
    PyObject *unused_data;
    PyObject *unconsumed_tail;
    char eof;
    int is_initialised;
    PyObject *zdict;
    PyThread_type_lock lock;
} compobject;

static void
zlib_error(z_stream zst, int err, const char *msg)
{
    const char *zmsg = Z_NULL;
    /* In case of a version mismatch, zst.msg won't be initialized.
       Check for this case first, before looking at zst.msg. */
    if (err == Z_VERSION_ERROR)
        zmsg = "library version mismatch";
    if (zmsg == Z_NULL)
        zmsg = zst.msg;
    if (zmsg == Z_NULL) {
        switch (err) {
        case Z_BUF_ERROR:
            zmsg = "incomplete or truncated stream";
            break;
        case Z_STREAM_ERROR:
            zmsg = "inconsistent stream state";
            break;
        case Z_DATA_ERROR:
            zmsg = "invalid input data";
            break;
        }
    }
    if (zmsg == Z_NULL)
        PyErr_Format(ZlibError, "Error %d %s", err, msg);
    else
        PyErr_Format(ZlibError, "Error %d %s: %.200s", err, msg, zmsg);
}

/*[clinic input]
module zlib
class zlib.Compress "compobject *" "&Comptype"
class zlib.Decompress "compobject *" "&Decomptype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=093935115c3e3158]*/

static compobject *
newcompobject(PyTypeObject *type)
{
    compobject *self;
    self = PyObject_New(compobject, type);
    if (self == NULL)
        return NULL;
    self->eof = 0;
    self->is_initialised = 0;
    self->zdict = NULL;
    self->unused_data = PyBytes_FromStringAndSize("", 0);
    if (self->unused_data == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    self->unconsumed_tail = PyBytes_FromStringAndSize("", 0);
    if (self->unconsumed_tail == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        Py_DECREF(self);
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }
    return self;
}

static void*
PyZlib_Malloc(voidpf ctx, uInt items, uInt size)
{
    if (size != 0 && items > (size_t)PY_SSIZE_T_MAX / size)
        return NULL;
    /* PyMem_Malloc() cannot be used: the GIL is not held when
       inflate() and deflate() are called */
    return PyMem_RawMalloc((size_t)items * (size_t)size);
}

static void
PyZlib_Free(voidpf ctx, void *ptr)
{
    PyMem_RawFree(ptr);
}

static void
arrange_input_buffer(z_stream *zst, Py_ssize_t *remains)
{
    zst->avail_in = (uInt)Py_MIN((size_t)*remains, UINT_MAX);
    *remains -= zst->avail_in;
}

static Py_ssize_t
arrange_output_buffer_with_maximum(z_stream *zst, PyObject **buffer,
                                   Py_ssize_t length,
                                   Py_ssize_t max_length)
{
    Py_ssize_t occupied;

    if (*buffer == NULL) {
        if (!(*buffer = PyBytes_FromStringAndSize(NULL, length)))
            return -1;
        occupied = 0;
    }
    else {
        occupied = zst->next_out - (Byte *)PyBytes_AS_STRING(*buffer);

        if (length == occupied) {
            Py_ssize_t new_length;
            assert(length <= max_length);
            /* can not scale the buffer over max_length */
            if (length == max_length)
                return -2;
            if (length <= (max_length >> 1))
                new_length = length << 1;
            else
                new_length = max_length;
            if (_PyBytes_Resize(buffer, new_length) < 0)
                return -1;
            length = new_length;
        }
    }

    zst->avail_out = (uInt)Py_MIN((size_t)(length - occupied), UINT_MAX);
    zst->next_out = (Byte *)PyBytes_AS_STRING(*buffer) + occupied;

    return length;
}

static Py_ssize_t
arrange_output_buffer(z_stream *zst, PyObject **buffer, Py_ssize_t length)
{
    Py_ssize_t ret;

    ret = arrange_output_buffer_with_maximum(zst, buffer, length,
                                             PY_SSIZE_T_MAX);
    if (ret == -2)
        PyErr_NoMemory();

    return ret;
}

/*[clinic input]
zlib.compress

    data: Py_buffer
        Binary data to be compressed.
    /
    level: int(c_default="Z_DEFAULT_COMPRESSION") = Z_DEFAULT_COMPRESSION
        Compression level, in 0-9 or -1.

Returns a bytes object containing compressed data.
[clinic start generated code]*/

static PyObject *
zlib_compress_impl(PyObject *module, Py_buffer *data, int level)
/*[clinic end generated code: output=d80906d73f6294c8 input=638d54b6315dbed3]*/
{
    PyObject *RetVal = NULL;
    Byte *ibuf;
    Py_ssize_t ibuflen, obuflen = DEF_BUF_SIZE;
    int err, flush;
    z_stream zst;

    ibuf = data->buf;
    ibuflen = data->len;

    zst.opaque = NULL;
    zst.zalloc = PyZlib_Malloc;
    zst.zfree = PyZlib_Free;
    zst.next_in = ibuf;
    err = deflateInit(&zst, level);

    switch (err) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        PyErr_SetString(PyExc_MemoryError,
                        "Out of memory while compressing data");
        goto error;
    case Z_STREAM_ERROR:
        PyErr_SetString(ZlibError, "Bad compression level");
        goto error;
    default:
        deflateEnd(&zst);
        zlib_error(zst, err, "while compressing data");
        goto error;
    }

    do {
        arrange_input_buffer(&zst, &ibuflen);
        flush = ibuflen == 0 ? Z_FINISH : Z_NO_FLUSH;

        do {
            obuflen = arrange_output_buffer(&zst, &RetVal, obuflen);
            if (obuflen < 0) {
                deflateEnd(&zst);
                goto error;
            }

            Py_BEGIN_ALLOW_THREADS
            err = deflate(&zst, flush);
            Py_END_ALLOW_THREADS

            if (err == Z_STREAM_ERROR) {
                deflateEnd(&zst);
                zlib_error(zst, err, "while compressing data");
                goto error;
            }

        } while (zst.avail_out == 0);
        assert(zst.avail_in == 0);

    } while (flush != Z_FINISH);
    assert(err == Z_STREAM_END);

    err = deflateEnd(&zst);
    if (err == Z_OK) {
        if (_PyBytes_Resize(&RetVal, zst.next_out -
                            (Byte *)PyBytes_AS_STRING(RetVal)) < 0)
            goto error;
        return RetVal;
    }
    else
        zlib_error(zst, err, "while finishing compression");
 error:
    Py_XDECREF(RetVal);
    return NULL;
}

/*[python input]

class ssize_t_converter(CConverter):
    type = 'Py_ssize_t'
    converter = 'ssize_t_converter'
    c_ignored_default = "0"

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=5f34ba1b394cb8e7]*/

static int
ssize_t_converter(PyObject *obj, void *ptr)
{
    PyObject *long_obj;
    Py_ssize_t val;

    long_obj = (PyObject *)_PyLong_FromNbInt(obj);
    if (long_obj == NULL) {
        return 0;
    }
    val = PyLong_AsSsize_t(long_obj);
    Py_DECREF(long_obj);
    if (val == -1 && PyErr_Occurred()) {
        return 0;
    }
    *(Py_ssize_t *)ptr = val;
    return 1;
}

/*[clinic input]
zlib.decompress

    data: Py_buffer
        Compressed data.
    /
    wbits: int(c_default="MAX_WBITS") = MAX_WBITS
        The window buffer size and container format.
    bufsize: ssize_t(c_default="DEF_BUF_SIZE") = DEF_BUF_SIZE
        The initial output buffer size.

Returns a bytes object containing the uncompressed data.
[clinic start generated code]*/

static PyObject *
zlib_decompress_impl(PyObject *module, Py_buffer *data, int wbits,
                     Py_ssize_t bufsize)
/*[clinic end generated code: output=77c7e35111dc8c42 input=21960936208e9a5b]*/
{
    PyObject *RetVal = NULL;
    Byte *ibuf;
    Py_ssize_t ibuflen;
    int err, flush;
    z_stream zst;

    if (bufsize < 0) {
        PyErr_SetString(PyExc_ValueError, "bufsize must be non-negative");
        return NULL;
    } else if (bufsize == 0) {
        bufsize = 1;
    }

    ibuf = data->buf;
    ibuflen = data->len;

    zst.opaque = NULL;
    zst.zalloc = PyZlib_Malloc;
    zst.zfree = PyZlib_Free;
    zst.avail_in = 0;
    zst.next_in = ibuf;
    err = inflateInit2(&zst, wbits);

    switch (err) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        PyErr_SetString(PyExc_MemoryError,
                        "Out of memory while decompressing data");
        goto error;
    default:
        inflateEnd(&zst);
        zlib_error(zst, err, "while preparing to decompress data");
        goto error;
    }

    do {
        arrange_input_buffer(&zst, &ibuflen);
        flush = ibuflen == 0 ? Z_FINISH : Z_NO_FLUSH;

        do {
            bufsize = arrange_output_buffer(&zst, &RetVal, bufsize);
            if (bufsize < 0) {
                inflateEnd(&zst);
                goto error;
            }

            Py_BEGIN_ALLOW_THREADS
            err = inflate(&zst, flush);
            Py_END_ALLOW_THREADS

            switch (err) {
            case Z_OK:            /* fall through */
            case Z_BUF_ERROR:     /* fall through */
            case Z_STREAM_END:
                break;
            case Z_MEM_ERROR:
                inflateEnd(&zst);
                PyErr_SetString(PyExc_MemoryError,
                                "Out of memory while decompressing data");
                goto error;
            default:
                inflateEnd(&zst);
                zlib_error(zst, err, "while decompressing data");
                goto error;
            }

        } while (zst.avail_out == 0);

    } while (err != Z_STREAM_END && ibuflen != 0);


    if (err != Z_STREAM_END) {
        inflateEnd(&zst);
        zlib_error(zst, err, "while decompressing data");
        goto error;
    }

    err = inflateEnd(&zst);
    if (err != Z_OK) {
        zlib_error(zst, err, "while finishing decompression");
        goto error;
    }

    if (_PyBytes_Resize(&RetVal, zst.next_out -
                        (Byte *)PyBytes_AS_STRING(RetVal)) < 0)
        goto error;

    return RetVal;

 error:
    Py_XDECREF(RetVal);
    return NULL;
}

/*[clinic input]
zlib.compressobj

    level: int(c_default="Z_DEFAULT_COMPRESSION") = Z_DEFAULT_COMPRESSION
        The compression level (an integer in the range 0-9 or -1; default is
        currently equivalent to 6).  Higher compression levels are slower,
        but produce smaller results.
    method: int(c_default="DEFLATED") = DEFLATED
        The compression algorithm.  If given, this must be DEFLATED.
    wbits: int(c_default="MAX_WBITS") = MAX_WBITS
        +9 to +15: The base-two logarithm of the window size.  Include a zlib
            container.
        -9 to -15: Generate a raw stream.
        +25 to +31: Include a gzip container.
    memLevel: int(c_default="DEF_MEM_LEVEL") = DEF_MEM_LEVEL
        Controls the amount of memory used for internal compression state.
        Valid values range from 1 to 9.  Higher values result in higher memory
        usage, faster compression, and smaller output.
    strategy: int(c_default="Z_DEFAULT_STRATEGY") = Z_DEFAULT_STRATEGY
        Used to tune the compression algorithm.  Possible values are
        Z_DEFAULT_STRATEGY, Z_FILTERED, and Z_HUFFMAN_ONLY.
    zdict: Py_buffer = None
        The predefined compression dictionary - a sequence of bytes
        containing subsequences that are likely to occur in the input data.

Return a compressor object.
[clinic start generated code]*/

static PyObject *
zlib_compressobj_impl(PyObject *module, int level, int method, int wbits,
                      int memLevel, int strategy, Py_buffer *zdict)
/*[clinic end generated code: output=8b5bed9c8fc3814d input=2fa3d026f90ab8d5]*/
{
    compobject *self = NULL;
    int err;

    if (zdict->buf != NULL && (size_t)zdict->len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "zdict length does not fit in an unsigned int");
        goto error;
    }

    self = newcompobject(&Comptype);
    if (self == NULL)
        goto error;
    self->zst.opaque = NULL;
    self->zst.zalloc = PyZlib_Malloc;
    self->zst.zfree = PyZlib_Free;
    self->zst.next_in = NULL;
    self->zst.avail_in = 0;
    err = deflateInit2(&self->zst, level, method, wbits, memLevel, strategy);
    switch (err) {
    case Z_OK:
        self->is_initialised = 1;
        if (zdict->buf == NULL) {
            goto success;
        } else {
            err = deflateSetDictionary(&self->zst,
                                       zdict->buf, (unsigned int)zdict->len);
            switch (err) {
            case Z_OK:
                goto success;
            case Z_STREAM_ERROR:
                PyErr_SetString(PyExc_ValueError, "Invalid dictionary");
                goto error;
            default:
                PyErr_SetString(PyExc_ValueError, "deflateSetDictionary()");
                goto error;
            }
       }
    case Z_MEM_ERROR:
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for compression object");
        goto error;
    case Z_STREAM_ERROR:
        PyErr_SetString(PyExc_ValueError, "Invalid initialization option");
        goto error;
    default:
        zlib_error(self->zst, err, "while creating compression object");
        goto error;
    }

 error:
    Py_CLEAR(self);
 success:
    return (PyObject *)self;
}

static int
set_inflate_zdict(compobject *self)
{
    Py_buffer zdict_buf;
    int err;

    if (PyObject_GetBuffer(self->zdict, &zdict_buf, PyBUF_SIMPLE) == -1) {
        return -1;
    }
    if ((size_t)zdict_buf.len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "zdict length does not fit in an unsigned int");
        PyBuffer_Release(&zdict_buf);
        return -1;
    }
    err = inflateSetDictionary(&self->zst,
                               zdict_buf.buf, (unsigned int)zdict_buf.len);
    PyBuffer_Release(&zdict_buf);
    if (err != Z_OK) {
        zlib_error(self->zst, err, "while setting zdict");
        return -1;
    }
    return 0;
}

/*[clinic input]
zlib.decompressobj

    wbits: int(c_default="MAX_WBITS") = MAX_WBITS
        The window buffer size and container format.
    zdict: object(c_default="NULL") = b''
        The predefined compression dictionary.  This must be the same
        dictionary as used by the compressor that produced the input data.

Return a decompressor object.
[clinic start generated code]*/

static PyObject *
zlib_decompressobj_impl(PyObject *module, int wbits, PyObject *zdict)
/*[clinic end generated code: output=3069b99994f36906 input=d3832b8511fc977b]*/
{
    int err;
    compobject *self;

    if (zdict != NULL && !PyObject_CheckBuffer(zdict)) {
        PyErr_SetString(PyExc_TypeError,
                        "zdict argument must support the buffer protocol");
        return NULL;
    }

    self = newcompobject(&Decomptype);
    if (self == NULL)
        return NULL;
    self->zst.opaque = NULL;
    self->zst.zalloc = PyZlib_Malloc;
    self->zst.zfree = PyZlib_Free;
    self->zst.next_in = NULL;
    self->zst.avail_in = 0;
    if (zdict != NULL) {
        Py_INCREF(zdict);
        self->zdict = zdict;
    }
    err = inflateInit2(&self->zst, wbits);
    switch (err) {
    case Z_OK:
        self->is_initialised = 1;
        if (self->zdict != NULL && wbits < 0) {
#ifdef AT_LEAST_ZLIB_1_2_2_1
            if (set_inflate_zdict(self) < 0) {
                Py_DECREF(self);
                return NULL;
            }
#else
            PyErr_Format(ZlibError,
                         "zlib version %s does not allow raw inflate with dictionary",
                         ZLIB_VERSION);
            Py_DECREF(self);
            return NULL;
#endif
        }
        return (PyObject *)self;
    case Z_STREAM_ERROR:
        Py_DECREF(self);
        PyErr_SetString(PyExc_ValueError, "Invalid initialization option");
        return NULL;
    case Z_MEM_ERROR:
        Py_DECREF(self);
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for decompression object");
        return NULL;
    default:
        zlib_error(self->zst, err, "while creating decompression object");
        Py_DECREF(self);
        return NULL;
    }
}

static void
Dealloc(compobject *self)
{
    PyThread_free_lock(self->lock);
    Py_XDECREF(self->unused_data);
    Py_XDECREF(self->unconsumed_tail);
    Py_XDECREF(self->zdict);
    PyObject_Del(self);
}

static void
Comp_dealloc(compobject *self)
{
    if (self->is_initialised)
        deflateEnd(&self->zst);
    Dealloc(self);
}

static void
Decomp_dealloc(compobject *self)
{
    if (self->is_initialised)
        inflateEnd(&self->zst);
    Dealloc(self);
}

/*[clinic input]
zlib.Compress.compress

    data: Py_buffer
        Binary data to be compressed.
    /

Returns a bytes object containing compressed data.

After calling this function, some of the input data may still
be stored in internal buffers for later processing.
Call the flush() method to clear these buffers.
[clinic start generated code]*/

static PyObject *
zlib_Compress_compress_impl(compobject *self, Py_buffer *data)
/*[clinic end generated code: output=5d5cd791cbc6a7f4 input=0d95908d6e64fab8]*/
{
    PyObject *RetVal = NULL;
    Py_ssize_t ibuflen, obuflen = DEF_BUF_SIZE;
    int err;

    self->zst.next_in = data->buf;
    ibuflen = data->len;

    ENTER_ZLIB(self);

    do {
        arrange_input_buffer(&self->zst, &ibuflen);

        do {
            obuflen = arrange_output_buffer(&self->zst, &RetVal, obuflen);
            if (obuflen < 0)
                goto error;

            Py_BEGIN_ALLOW_THREADS
            err = deflate(&self->zst, Z_NO_FLUSH);
            Py_END_ALLOW_THREADS

            if (err == Z_STREAM_ERROR) {
                zlib_error(self->zst, err, "while compressing data");
                goto error;
            }

        } while (self->zst.avail_out == 0);
        assert(self->zst.avail_in == 0);

    } while (ibuflen != 0);

    if (_PyBytes_Resize(&RetVal, self->zst.next_out -
                        (Byte *)PyBytes_AS_STRING(RetVal)) == 0)
        goto success;

 error:
    Py_CLEAR(RetVal);
 success:
    LEAVE_ZLIB(self);
    return RetVal;
}

/* Helper for objdecompress() and flush(). Saves any unconsumed input data in
   self->unused_data or self->unconsumed_tail, as appropriate. */
static int
save_unconsumed_input(compobject *self, Py_buffer *data, int err)
{
    if (err == Z_STREAM_END) {
        /* The end of the compressed data has been reached. Store the leftover
           input data in self->unused_data. */
        if (self->zst.avail_in > 0) {
            Py_ssize_t old_size = PyBytes_GET_SIZE(self->unused_data);
            Py_ssize_t new_size, left_size;
            PyObject *new_data;
            left_size = (Byte *)data->buf + data->len - self->zst.next_in;
            if (left_size > (PY_SSIZE_T_MAX - old_size)) {
                PyErr_NoMemory();
                return -1;
            }
            new_size = old_size + left_size;
            new_data = PyBytes_FromStringAndSize(NULL, new_size);
            if (new_data == NULL)
                return -1;
            memcpy(PyBytes_AS_STRING(new_data),
                      PyBytes_AS_STRING(self->unused_data), old_size);
            memcpy(PyBytes_AS_STRING(new_data) + old_size,
                      self->zst.next_in, left_size);
            Py_SETREF(self->unused_data, new_data);
            self->zst.avail_in = 0;
        }
    }

    if (self->zst.avail_in > 0 || PyBytes_GET_SIZE(self->unconsumed_tail)) {
        /* This code handles two distinct cases:
           1. Output limit was reached. Save leftover input in unconsumed_tail.
           2. All input data was consumed. Clear unconsumed_tail. */
        Py_ssize_t left_size = (Byte *)data->buf + data->len - self->zst.next_in;
        PyObject *new_data = PyBytes_FromStringAndSize(
                (char *)self->zst.next_in, left_size);
        if (new_data == NULL)
            return -1;
        Py_SETREF(self->unconsumed_tail, new_data);
    }

    return 0;
}

/*[clinic input]
zlib.Decompress.decompress

    data: Py_buffer
        The binary data to decompress.
    /
    max_length: ssize_t = 0
        The maximum allowable length of the decompressed data.
        Unconsumed input data will be stored in
        the unconsumed_tail attribute.

Return a bytes object containing the decompressed version of the data.

After calling this function, some of the input data may still be stored in
internal buffers for later processing.
Call the flush() method to clear these buffers.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, Py_buffer *data,
                                Py_ssize_t max_length)
/*[clinic end generated code: output=6e5173c74e710352 input=b85a212a012b770a]*/
{
    int err = Z_OK;
    Py_ssize_t ibuflen, obuflen = DEF_BUF_SIZE, hard_limit;
    PyObject *RetVal = NULL;

    if (max_length < 0) {
        PyErr_SetString(PyExc_ValueError, "max_length must be non-negative");
        return NULL;
    } else if (max_length == 0)
        hard_limit = PY_SSIZE_T_MAX;
    else
        hard_limit = max_length;

    self->zst.next_in = data->buf;
    ibuflen = data->len;

    /* limit amount of data allocated to max_length */
    if (max_length && obuflen > max_length)
        obuflen = max_length;

    ENTER_ZLIB(self);

    do {
        arrange_input_buffer(&self->zst, &ibuflen);

        do {
            obuflen = arrange_output_buffer_with_maximum(&self->zst, &RetVal,
                                                         obuflen, hard_limit);
            if (obuflen == -2) {
                if (max_length > 0) {
                    goto save;
                }
                PyErr_NoMemory();
            }
            if (obuflen < 0) {
                goto abort;
            }

            Py_BEGIN_ALLOW_THREADS
            err = inflate(&self->zst, Z_SYNC_FLUSH);
            Py_END_ALLOW_THREADS

            switch (err) {
            case Z_OK:            /* fall through */
            case Z_BUF_ERROR:     /* fall through */
            case Z_STREAM_END:
                break;
            default:
                if (err == Z_NEED_DICT && self->zdict != NULL) {
                    if (set_inflate_zdict(self) < 0)
                        goto abort;
                    else
                        break;
                }
                goto save;
            }

        } while (self->zst.avail_out == 0 || err == Z_NEED_DICT);

    } while (err != Z_STREAM_END && ibuflen != 0);

 save:
    if (save_unconsumed_input(self, data, err) < 0)
        goto abort;

    if (err == Z_STREAM_END) {
        /* This is the logical place to call inflateEnd, but the old behaviour
           of only calling it on flush() is preserved. */
        self->eof = 1;
    } else if (err != Z_OK && err != Z_BUF_ERROR) {
        /* We will only get Z_BUF_ERROR if the output buffer was full
           but there wasn't more output when we tried again, so it is
           not an error condition.
        */
        zlib_error(self->zst, err, "while decompressing data");
        goto abort;
    }

    if (_PyBytes_Resize(&RetVal, self->zst.next_out -
                        (Byte *)PyBytes_AS_STRING(RetVal)) == 0)
        goto success;

 abort:
    Py_CLEAR(RetVal);
 success:
    LEAVE_ZLIB(self);
    return RetVal;
}

/*[clinic input]
zlib.Compress.flush

    mode: int(c_default="Z_FINISH") = zlib.Z_FINISH
        One of the constants Z_SYNC_FLUSH, Z_FULL_FLUSH, Z_FINISH.
        If mode == Z_FINISH, the compressor object can no longer be
        used after calling the flush() method.  Otherwise, more data
        can still be compressed.
    /

Return a bytes object containing any remaining compressed data.
[clinic start generated code]*/

static PyObject *
zlib_Compress_flush_impl(compobject *self, int mode)
/*[clinic end generated code: output=a203f4cefc9de727 input=73ed066794bd15bc]*/
{
    int err;
    Py_ssize_t length = DEF_BUF_SIZE;
    PyObject *RetVal = NULL;

    /* Flushing with Z_NO_FLUSH is a no-op, so there's no point in
       doing any work at all; just return an empty string. */
    if (mode == Z_NO_FLUSH) {
        return PyBytes_FromStringAndSize(NULL, 0);
    }

    ENTER_ZLIB(self);

    self->zst.avail_in = 0;

    do {
        length = arrange_output_buffer(&self->zst, &RetVal, length);
        if (length < 0) {
            Py_CLEAR(RetVal);
            goto error;
        }

        Py_BEGIN_ALLOW_THREADS
        err = deflate(&self->zst, mode);
        Py_END_ALLOW_THREADS

        if (err == Z_STREAM_ERROR) {
            zlib_error(self->zst, err, "while flushing");
            Py_CLEAR(RetVal);
            goto error;
        }
    } while (self->zst.avail_out == 0);
    assert(self->zst.avail_in == 0);

    /* If mode is Z_FINISH, we also have to call deflateEnd() to free
       various data structures. Note we should only get Z_STREAM_END when
       mode is Z_FINISH, but checking both for safety*/
    if (err == Z_STREAM_END && mode == Z_FINISH) {
        err = deflateEnd(&self->zst);
        if (err != Z_OK) {
            zlib_error(self->zst, err, "while finishing compression");
            Py_CLEAR(RetVal);
            goto error;
        }
        else
            self->is_initialised = 0;

        /* We will only get Z_BUF_ERROR if the output buffer was full
           but there wasn't more output when we tried again, so it is
           not an error condition.
        */
    } else if (err != Z_OK && err != Z_BUF_ERROR) {
        zlib_error(self->zst, err, "while flushing");
        Py_CLEAR(RetVal);
        goto error;
    }

    if (_PyBytes_Resize(&RetVal, self->zst.next_out -
                        (Byte *)PyBytes_AS_STRING(RetVal)) < 0)
        Py_CLEAR(RetVal);

 error:
    LEAVE_ZLIB(self);
    return RetVal;
}

#ifdef HAVE_ZLIB_COPY

/*[clinic input]
zlib.Compress.copy

Return a copy of the compression object.
[clinic start generated code]*/

static PyObject *
zlib_Compress_copy_impl(compobject *self)
/*[clinic end generated code: output=5144aa153c21e805 input=c656351f94b82718]*/
{
    compobject *retval = NULL;
    int err;

    retval = newcompobject(&Comptype);
    if (!retval) return NULL;

    /* Copy the zstream state
     * We use ENTER_ZLIB / LEAVE_ZLIB to make this thread-safe
     */
    ENTER_ZLIB(self);
    err = deflateCopy(&retval->zst, &self->zst);
    switch (err) {
    case Z_OK:
        break;
    case Z_STREAM_ERROR:
        PyErr_SetString(PyExc_ValueError, "Inconsistent stream state");
        goto error;
    case Z_MEM_ERROR:
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for compression object");
        goto error;
    default:
        zlib_error(self->zst, err, "while copying compression object");
        goto error;
    }
    Py_INCREF(self->unused_data);
    Py_XSETREF(retval->unused_data, self->unused_data);
    Py_INCREF(self->unconsumed_tail);
    Py_XSETREF(retval->unconsumed_tail, self->unconsumed_tail);
    Py_XINCREF(self->zdict);
    Py_XSETREF(retval->zdict, self->zdict);
    retval->eof = self->eof;

    /* Mark it as being initialized */
    retval->is_initialised = 1;

    LEAVE_ZLIB(self);
    return (PyObject *)retval;

error:
    LEAVE_ZLIB(self);
    Py_XDECREF(retval);
    return NULL;
}

/*[clinic input]
zlib.Decompress.copy

Return a copy of the decompression object.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_copy_impl(compobject *self)
/*[clinic end generated code: output=02a883a2a510c8cc input=ba6c3e96712a596b]*/
{
    compobject *retval = NULL;
    int err;

    retval = newcompobject(&Decomptype);
    if (!retval) return NULL;

    /* Copy the zstream state
     * We use ENTER_ZLIB / LEAVE_ZLIB to make this thread-safe
     */
    ENTER_ZLIB(self);
    err = inflateCopy(&retval->zst, &self->zst);
    switch (err) {
    case Z_OK:
        break;
    case Z_STREAM_ERROR:
        PyErr_SetString(PyExc_ValueError, "Inconsistent stream state");
        goto error;
    case Z_MEM_ERROR:
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for decompression object");
        goto error;
    default:
        zlib_error(self->zst, err, "while copying decompression object");
        goto error;
    }

    Py_INCREF(self->unused_data);
    Py_XSETREF(retval->unused_data, self->unused_data);
    Py_INCREF(self->unconsumed_tail);
    Py_XSETREF(retval->unconsumed_tail, self->unconsumed_tail);
    Py_XINCREF(self->zdict);
    Py_XSETREF(retval->zdict, self->zdict);
    retval->eof = self->eof;

    /* Mark it as being initialized */
    retval->is_initialised = 1;

    LEAVE_ZLIB(self);
    return (PyObject *)retval;

error:
    LEAVE_ZLIB(self);
    Py_XDECREF(retval);
    return NULL;
}
#endif

/*[clinic input]
zlib.Decompress.flush

    length: ssize_t(c_default="DEF_BUF_SIZE") = zlib.DEF_BUF_SIZE
        the initial size of the output buffer.
    /

Return a bytes object containing any remaining decompressed data.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_flush_impl(compobject *self, Py_ssize_t length)
/*[clinic end generated code: output=68c75ea127cbe654 input=aa4ec37f3aef4da0]*/
{
    int err, flush;
    Py_buffer data;
    PyObject *RetVal = NULL;
    Py_ssize_t ibuflen;

    if (length <= 0) {
        PyErr_SetString(PyExc_ValueError, "length must be greater than zero");
        return NULL;
    }

    if (PyObject_GetBuffer(self->unconsumed_tail, &data, PyBUF_SIMPLE) == -1)
        return NULL;

    ENTER_ZLIB(self);

    self->zst.next_in = data.buf;
    ibuflen = data.len;

    do {
        arrange_input_buffer(&self->zst, &ibuflen);
        flush = ibuflen == 0 ? Z_FINISH : Z_NO_FLUSH;

        do {
            length = arrange_output_buffer(&self->zst, &RetVal, length);
            if (length < 0)
                goto abort;

            Py_BEGIN_ALLOW_THREADS
            err = inflate(&self->zst, flush);
            Py_END_ALLOW_THREADS

            switch (err) {
            case Z_OK:            /* fall through */
            case Z_BUF_ERROR:     /* fall through */
            case Z_STREAM_END:
                break;
            default:
                if (err == Z_NEED_DICT && self->zdict != NULL) {
                    if (set_inflate_zdict(self) < 0)
                        goto abort;
                    else
                        break;
                }
                goto save;
            }

        } while (self->zst.avail_out == 0 || err == Z_NEED_DICT);

    } while (err != Z_STREAM_END && ibuflen != 0);

 save:
    if (save_unconsumed_input(self, &data, err) < 0)
        goto abort;

    /* If at end of stream, clean up any memory allocated by zlib. */
    if (err == Z_STREAM_END) {
        self->eof = 1;
        self->is_initialised = 0;
        err = inflateEnd(&self->zst);
        if (err != Z_OK) {
            zlib_error(self->zst, err, "while finishing decompression");
            goto abort;
        }
    }

    if (_PyBytes_Resize(&RetVal, self->zst.next_out -
                        (Byte *)PyBytes_AS_STRING(RetVal)) == 0)
        goto success;

 abort:
    Py_CLEAR(RetVal);
 success:
    PyBuffer_Release(&data);
    LEAVE_ZLIB(self);
    return RetVal;
}

#include "clinic/zlibmodule.c.h"

static PyMethodDef comp_methods[] =
{
    ZLIB_COMPRESS_COMPRESS_METHODDEF
    ZLIB_COMPRESS_FLUSH_METHODDEF
    ZLIB_COMPRESS_COPY_METHODDEF
    {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
    ZLIB_DECOMPRESS_DECOMPRESS_METHODDEF
    ZLIB_DECOMPRESS_FLUSH_METHODDEF
    ZLIB_DECOMPRESS_COPY_METHODDEF
    {NULL, NULL}
};

#define COMP_OFF(x) offsetof(compobject, x)
static PyMemberDef Decomp_members[] = {
    {"unused_data",     T_OBJECT, COMP_OFF(unused_data), READONLY},
    {"unconsumed_tail", T_OBJECT, COMP_OFF(unconsumed_tail), READONLY},
    {"eof",             T_BOOL,   COMP_OFF(eof), READONLY},
    {NULL},
};

/*[clinic input]
zlib.adler32

    data: Py_buffer
    value: unsigned_int(bitwise=True) = 1
        Starting value of the checksum.
    /

Compute an Adler-32 checksum of data.

The returned checksum is an integer.
[clinic start generated code]*/

static PyObject *
zlib_adler32_impl(PyObject *module, Py_buffer *data, unsigned int value)
/*[clinic end generated code: output=422106f5ca8c92c0 input=6ff4557872160e88]*/
{
    /* Releasing the GIL for very small buffers is inefficient
       and may lower performance */
    if (data->len > 1024*5) {
        unsigned char *buf = data->buf;
        Py_ssize_t len = data->len;

        Py_BEGIN_ALLOW_THREADS
        /* Avoid truncation of length for very large buffers. adler32() takes
           length as an unsigned int, which may be narrower than Py_ssize_t. */
        while ((size_t)len > UINT_MAX) {
            value = adler32(value, buf, UINT_MAX);
            buf += (size_t) UINT_MAX;
            len -= (size_t) UINT_MAX;
        }
        value = adler32(value, buf, (unsigned int)len);
        Py_END_ALLOW_THREADS
    } else {
        value = adler32(value, data->buf, (unsigned int)data->len);
    }
    return PyLong_FromUnsignedLong(value & 0xffffffffU);
}

/*[clinic input]
zlib.crc32

    data: Py_buffer
    value: unsigned_int(bitwise=True) = 0
        Starting value of the checksum.
    /

Compute a CRC-32 checksum of data.

The returned checksum is an integer.
[clinic start generated code]*/

static PyObject *
zlib_crc32_impl(PyObject *module, Py_buffer *data, unsigned int value)
/*[clinic end generated code: output=63499fa20af7ea25 input=26c3ed430fa00b4c]*/
{
    int signed_val;

    /* Releasing the GIL for very small buffers is inefficient
       and may lower performance */
    if (data->len > 1024*5) {
        unsigned char *buf = data->buf;
        Py_ssize_t len = data->len;

        Py_BEGIN_ALLOW_THREADS
        /* Avoid truncation of length for very large buffers. crc32() takes
           length as an unsigned int, which may be narrower than Py_ssize_t. */
        while ((size_t)len > UINT_MAX) {
            value = crc32(value, buf, UINT_MAX);
            buf += (size_t) UINT_MAX;
            len -= (size_t) UINT_MAX;
        }
        signed_val = crc32(value, buf, (unsigned int)len);
        Py_END_ALLOW_THREADS
    } else {
        signed_val = crc32(value, data->buf, (unsigned int)data->len);
    }
    return PyLong_FromUnsignedLong(signed_val & 0xffffffffU);
}


static PyMethodDef zlib_methods[] =
{
    ZLIB_ADLER32_METHODDEF
    ZLIB_COMPRESS_METHODDEF
    ZLIB_COMPRESSOBJ_METHODDEF
    ZLIB_CRC32_METHODDEF
    ZLIB_DECOMPRESS_METHODDEF
    ZLIB_DECOMPRESSOBJ_METHODDEF
    {NULL, NULL}
};

static PyTypeObject Comptype = {
    PyVarObject_HEAD_INIT(0, 0)
    "zlib.Compress",
    sizeof(compobject),
    0,
    (destructor)Comp_dealloc,       /*tp_dealloc*/
    0,                              /*tp_print*/
    0,                              /*tp_getattr*/
    0,                              /*tp_setattr*/
    0,                              /*tp_reserved*/
    0,                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    0,                              /*tp_hash*/
    0,                              /*tp_call*/
    0,                              /*tp_str*/
    0,                              /*tp_getattro*/
    0,                              /*tp_setattro*/
    0,                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,             /*tp_flags*/
    0,                              /*tp_doc*/
    0,                              /*tp_traverse*/
    0,                              /*tp_clear*/
    0,                              /*tp_richcompare*/
    0,                              /*tp_weaklistoffset*/
    0,                              /*tp_iter*/
    0,                              /*tp_iternext*/
    comp_methods,                   /*tp_methods*/
};

static PyTypeObject Decomptype = {
    PyVarObject_HEAD_INIT(0, 0)
    "zlib.Decompress",
    sizeof(compobject),
    0,
    (destructor)Decomp_dealloc,     /*tp_dealloc*/
    0,                              /*tp_print*/
    0,                              /*tp_getattr*/
    0,                              /*tp_setattr*/
    0,                              /*tp_reserved*/
    0,                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                              /*tp_as_sequence*/
    0,                              /*tp_as_mapping*/
    0,                              /*tp_hash*/
    0,                              /*tp_call*/
    0,                              /*tp_str*/
    0,                              /*tp_getattro*/
    0,                              /*tp_setattro*/
    0,                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,             /*tp_flags*/
    0,                              /*tp_doc*/
    0,                              /*tp_traverse*/
    0,                              /*tp_clear*/
    0,                              /*tp_richcompare*/
    0,                              /*tp_weaklistoffset*/
    0,                              /*tp_iter*/
    0,                              /*tp_iternext*/
    Decomp_methods,                 /*tp_methods*/
    Decomp_members,                 /*tp_members*/
};

PyDoc_STRVAR(zlib_module_documentation,
"The functions in this module allow compression and decompression using the\n"
"zlib library, which is based on GNU zip.\n"
"\n"
"adler32(string[, start]) -- Compute an Adler-32 checksum.\n"
"compress(data[, level]) -- Compress data, with compression level 0-9 or -1.\n"
"compressobj([level[, ...]]) -- Return a compressor object.\n"
"crc32(string[, start]) -- Compute a CRC-32 checksum.\n"
"decompress(string,[wbits],[bufsize]) -- Decompresses a compressed string.\n"
"decompressobj([wbits[, zdict]]]) -- Return a decompressor object.\n"
"\n"
"'wbits' is window buffer size and container format.\n"
"Compressor objects support compress() and flush() methods; decompressor\n"
"objects support decompress() and flush().");

static struct PyModuleDef zlibmodule = {
        PyModuleDef_HEAD_INIT,
        "zlib",
        zlib_module_documentation,
        -1,
        zlib_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit_zlib(void)
{
    PyObject *m, *ver;
    if (PyType_Ready(&Comptype) < 0)
            return NULL;
    if (PyType_Ready(&Decomptype) < 0)
            return NULL;
    m = PyModule_Create(&zlibmodule);
    if (m == NULL)
        return NULL;

    ZlibError = PyErr_NewException("zlib.error", NULL, NULL);
    if (ZlibError != NULL) {
        Py_INCREF(ZlibError);
        PyModule_AddObject(m, "error", ZlibError);
    }
    PyModule_AddIntMacro(m, MAX_WBITS);
    PyModule_AddIntMacro(m, DEFLATED);
    PyModule_AddIntMacro(m, DEF_MEM_LEVEL);
    PyModule_AddIntMacro(m, DEF_BUF_SIZE);
    // compression levels
    PyModule_AddIntMacro(m, Z_NO_COMPRESSION);
    PyModule_AddIntMacro(m, Z_BEST_SPEED);
    PyModule_AddIntMacro(m, Z_BEST_COMPRESSION);
    PyModule_AddIntMacro(m, Z_DEFAULT_COMPRESSION);
    // compression strategies
    PyModule_AddIntMacro(m, Z_FILTERED);
    PyModule_AddIntMacro(m, Z_HUFFMAN_ONLY);
#ifdef Z_RLE // 1.2.0.1
    PyModule_AddIntMacro(m, Z_RLE);
#endif
#ifdef Z_FIXED // 1.2.2.2
    PyModule_AddIntMacro(m, Z_FIXED);
#endif
    PyModule_AddIntMacro(m, Z_DEFAULT_STRATEGY);
    // allowed flush values
    PyModule_AddIntMacro(m, Z_NO_FLUSH);
    PyModule_AddIntMacro(m, Z_PARTIAL_FLUSH);
    PyModule_AddIntMacro(m, Z_SYNC_FLUSH);
    PyModule_AddIntMacro(m, Z_FULL_FLUSH);
    PyModule_AddIntMacro(m, Z_FINISH);
#ifdef Z_BLOCK // 1.2.0.5 for inflate, 1.2.3.4 for deflate
    PyModule_AddIntMacro(m, Z_BLOCK);
#endif
#ifdef Z_TREES // 1.2.3.4, only for inflate
    PyModule_AddIntMacro(m, Z_TREES);
#endif
    ver = PyUnicode_FromString(ZLIB_VERSION);
    if (ver != NULL)
        PyModule_AddObject(m, "ZLIB_VERSION", ver);

    ver = PyUnicode_FromString(zlibVersion());
    if (ver != NULL)
        PyModule_AddObject(m, "ZLIB_RUNTIME_VERSION", ver);

    PyModule_AddStringConstant(m, "__version__", "1.0");

    return m;
}
