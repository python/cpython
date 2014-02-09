/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.gzip.org/zlib/ */

/* Windows users:  read Python's PCbuild\readme.txt */


#include "Python.h"
#include "structmember.h"
#include "zlib.h"


#ifdef WITH_THREAD
    #include "pythread.h"
    #define ENTER_ZLIB(obj) \
        Py_BEGIN_ALLOW_THREADS; \
        PyThread_acquire_lock((obj)->lock, 1); \
        Py_END_ALLOW_THREADS;
    #define LEAVE_ZLIB(obj) PyThread_release_lock((obj)->lock);
#else
    #define ENTER_ZLIB(obj)
    #define LEAVE_ZLIB(obj)
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
    #ifdef WITH_THREAD
        PyThread_type_lock lock;
    #endif
} compobject;

static void
zlib_error(z_stream zst, int err, char *msg)
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
output preset file
module zlib
class zlib.Compress "compobject *" "&Comptype"
class zlib.Decompress "compobject *" "&Decomptype"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=bfd4c340573ba91d]*/

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
#ifdef WITH_THREAD
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }
#endif
    return self;
}

static void*
PyZlib_Malloc(voidpf ctx, uInt items, uInt size)
{
    if (items > (size_t)PY_SSIZE_T_MAX / size)
        return NULL;
    /* PyMem_Malloc() cannot be used: the GIL is not held when
       inflate() and deflate() are called */
    return PyMem_RawMalloc(items * size);
}

static void
PyZlib_Free(voidpf ctx, void *ptr)
{
    PyMem_RawFree(ptr);
}

/*[clinic input]
zlib.compress

    bytes: Py_buffer
        Binary data to be compressed.
    level: int(c_default="Z_DEFAULT_COMPRESSION") = Z_DEFAULT_COMPRESSION
        Compression level, in 0-9.
    /

Returns a bytes object containing compressed data.
[clinic start generated code]*/

static PyObject *
zlib_compress_impl(PyModuleDef *module, Py_buffer *bytes, int level)
/*[clinic end generated code: output=5d7dd4588788efd3 input=be3abe9934bda4b3]*/
{
    PyObject *ReturnVal = NULL;
    Byte *input, *output = NULL;
    unsigned int length;
    int err;
    z_stream zst;

    if ((size_t)bytes->len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Size does not fit in an unsigned int");
        goto error;
    }
    input = bytes->buf;
    length = (unsigned int)bytes->len;

    zst.avail_out = length + length/1000 + 12 + 1;

    output = (Byte*)PyMem_Malloc(zst.avail_out);
    if (output == NULL) {
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory to compress data");
        goto error;
    }

    /* Past the point of no return.  From here on out, we need to make sure
       we clean up mallocs & INCREFs. */

    zst.opaque = NULL;
    zst.zalloc = PyZlib_Malloc;
    zst.zfree = PyZlib_Free;
    zst.next_out = (Byte *)output;
    zst.next_in = (Byte *)input;
    zst.avail_in = length;
    err = deflateInit(&zst, level);

    switch(err) {
    case(Z_OK):
        break;
    case(Z_MEM_ERROR):
        PyErr_SetString(PyExc_MemoryError,
                        "Out of memory while compressing data");
        goto error;
    case(Z_STREAM_ERROR):
        PyErr_SetString(ZlibError,
                        "Bad compression level");
        goto error;
    default:
        deflateEnd(&zst);
        zlib_error(zst, err, "while compressing data");
        goto error;
    }

    Py_BEGIN_ALLOW_THREADS;
    err = deflate(&zst, Z_FINISH);
    Py_END_ALLOW_THREADS;

    if (err != Z_STREAM_END) {
        zlib_error(zst, err, "while compressing data");
        deflateEnd(&zst);
        goto error;
    }

    err=deflateEnd(&zst);
    if (err == Z_OK)
        ReturnVal = PyBytes_FromStringAndSize((char *)output,
                                              zst.total_out);
    else
        zlib_error(zst, err, "while finishing compression");

 error:
    PyMem_Free(output);

    return ReturnVal;
}

/*[python input]

class uint_converter(CConverter):
    type = 'unsigned int'
    converter = 'uint_converter'
    c_ignored_default = "0"

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=22263855f7a3ebfd]*/

static int
uint_converter(PyObject *obj, void *ptr)
{
    long val;
    unsigned long uval;

    val = PyLong_AsLong(obj);
    if (val == -1 && PyErr_Occurred()) {
        uval = PyLong_AsUnsignedLong(obj);
        if (uval == (unsigned long)-1 && PyErr_Occurred())
            return 0;
    }
    else {
        if (val < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "value must be positive");
            return 0;
        }
        uval = (unsigned long)val;
    }

    if (uval > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned int");
        return 0;
    }

    *(unsigned int *)ptr = Py_SAFE_DOWNCAST(uval, unsigned long, unsigned int);
    return 1;
}

/*[clinic input]
zlib.decompress

    data: Py_buffer
        Compressed data.
    wbits: int(c_default="MAX_WBITS") = MAX_WBITS
        The window buffer size.
    bufsize: uint(c_default="DEF_BUF_SIZE") = DEF_BUF_SIZE
        The initial output buffer size.
    /

Returns a bytes object containing the uncompressed data.
[clinic start generated code]*/

static PyObject *
zlib_decompress_impl(PyModuleDef *module, Py_buffer *data, int wbits, unsigned int bufsize)
/*[clinic end generated code: output=9e5464e72df9cb5f input=0f4b9abb7103f50e]*/
{
    PyObject *result_str = NULL;
    Byte *input;
    unsigned int length;
    int err;
    unsigned int new_bufsize;
    z_stream zst;

    if ((size_t)data->len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Size does not fit in an unsigned int");
        goto error;
    }
    input = data->buf;
    length = (unsigned int)data->len;

    if (bufsize == 0)
        bufsize = 1;

    zst.avail_in = length;
    zst.avail_out = bufsize;

    if (!(result_str = PyBytes_FromStringAndSize(NULL, bufsize)))
        goto error;

    zst.opaque = NULL;
    zst.zalloc = PyZlib_Malloc;
    zst.zfree = PyZlib_Free;
    zst.next_out = (Byte *)PyBytes_AS_STRING(result_str);
    zst.next_in = (Byte *)input;
    err = inflateInit2(&zst, wbits);

    switch(err) {
    case(Z_OK):
        break;
    case(Z_MEM_ERROR):
        PyErr_SetString(PyExc_MemoryError,
                        "Out of memory while decompressing data");
        goto error;
    default:
        inflateEnd(&zst);
        zlib_error(zst, err, "while preparing to decompress data");
        goto error;
    }

    do {
        Py_BEGIN_ALLOW_THREADS
        err=inflate(&zst, Z_FINISH);
        Py_END_ALLOW_THREADS

        switch(err) {
        case(Z_STREAM_END):
            break;
        case(Z_BUF_ERROR):
            /*
             * If there is at least 1 byte of room according to zst.avail_out
             * and we get this error, assume that it means zlib cannot
             * process the inflate call() due to an error in the data.
             */
            if (zst.avail_out > 0) {
                zlib_error(zst, err, "while decompressing data");
                inflateEnd(&zst);
                goto error;
            }
            /* fall through */
        case(Z_OK):
            /* need more memory */
            if (bufsize <= (UINT_MAX >> 1))
                new_bufsize = bufsize << 1;
            else
                new_bufsize = UINT_MAX;
            if (_PyBytes_Resize(&result_str, new_bufsize) < 0) {
                inflateEnd(&zst);
                goto error;
            }
            zst.next_out =
                (unsigned char *)PyBytes_AS_STRING(result_str) + bufsize;
            zst.avail_out = bufsize;
            bufsize = new_bufsize;
            break;
        default:
            inflateEnd(&zst);
            zlib_error(zst, err, "while decompressing data");
            goto error;
        }
    } while (err != Z_STREAM_END);

    err = inflateEnd(&zst);
    if (err != Z_OK) {
        zlib_error(zst, err, "while finishing decompression");
        goto error;
    }

    if (_PyBytes_Resize(&result_str, zst.total_out) < 0)
        goto error;

    return result_str;

 error:
    Py_XDECREF(result_str);
    return NULL;
}

/*[clinic input]
zlib.compressobj

    level: int(c_default="Z_DEFAULT_COMPRESSION") = Z_DEFAULT_COMPRESSION
        The compression level (an integer in the range 0-9; default is 6).
        Higher compression levels are slower, but produce smaller results.
    method: int(c_default="DEFLATED") = DEFLATED
        The compression algorithm.  If given, this must be DEFLATED.
    wbits: int(c_default="MAX_WBITS") = MAX_WBITS
        The base two logarithm of the window size (range: 8..15).
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
zlib_compressobj_impl(PyModuleDef *module, int level, int method, int wbits, int memLevel, int strategy, Py_buffer *zdict)
/*[clinic end generated code: output=89e5a6c1449caa9e input=b034847f8821f6af]*/
{
    compobject *self = NULL;
    int err;

    if (zdict->buf != NULL && (size_t)zdict->len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "zdict length does not fit in an unsigned int");
        goto error;
    }

    self = newcompobject(&Comptype);
    if (self==NULL)
        goto error;
    self->zst.opaque = NULL;
    self->zst.zalloc = PyZlib_Malloc;
    self->zst.zfree = PyZlib_Free;
    self->zst.next_in = NULL;
    self->zst.avail_in = 0;
    err = deflateInit2(&self->zst, level, method, wbits, memLevel, strategy);
    switch(err) {
    case (Z_OK):
        self->is_initialised = 1;
        if (zdict->buf == NULL) {
            goto success;
        } else {
            err = deflateSetDictionary(&self->zst,
                                       zdict->buf, (unsigned int)zdict->len);
            switch (err) {
            case (Z_OK):
                goto success;
            case (Z_STREAM_ERROR):
                PyErr_SetString(PyExc_ValueError, "Invalid dictionary");
                goto error;
            default:
                PyErr_SetString(PyExc_ValueError, "deflateSetDictionary()");
                goto error;
            }
       }
    case (Z_MEM_ERROR):
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for compression object");
        goto error;
    case(Z_STREAM_ERROR):
        PyErr_SetString(PyExc_ValueError, "Invalid initialization option");
        goto error;
    default:
        zlib_error(self->zst, err, "while creating compression object");
        goto error;
    }

 error:
    Py_CLEAR(self);
 success:
    return (PyObject*)self;
}

/*[clinic input]
zlib.decompressobj

    wbits: int(c_default="MAX_WBITS") = MAX_WBITS
        The window buffer size.
    zdict: object(c_default="NULL") = b''
        The predefined compression dictionary.  This must be the same
        dictionary as used by the compressor that produced the input data.

Return a decompressor object.
[clinic start generated code]*/

static PyObject *
zlib_decompressobj_impl(PyModuleDef *module, int wbits, PyObject *zdict)
/*[clinic end generated code: output=8ccd583fbd631798 input=67f05145a6920127]*/
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
        return(NULL);
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
    switch(err) {
    case (Z_OK):
        self->is_initialised = 1;
        return (PyObject*)self;
    case(Z_STREAM_ERROR):
        Py_DECREF(self);
        PyErr_SetString(PyExc_ValueError, "Invalid initialization option");
        return NULL;
    case (Z_MEM_ERROR):
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
#ifdef WITH_THREAD
    PyThread_free_lock(self->lock);
#endif
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
    int err;
    unsigned int inplen;
    unsigned int length = DEF_BUF_SIZE, new_length;
    PyObject *RetVal;
    Byte *input;
    unsigned long start_total_out;

    if ((size_t)data->len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Size does not fit in an unsigned int");
        return NULL;
    }
    input = data->buf;
    inplen = (unsigned int)data->len;

    if (!(RetVal = PyBytes_FromStringAndSize(NULL, length)))
        return NULL;

    ENTER_ZLIB(self);

    start_total_out = self->zst.total_out;
    self->zst.avail_in = inplen;
    self->zst.next_in = input;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyBytes_AS_STRING(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = deflate(&(self->zst), Z_NO_FLUSH);
    Py_END_ALLOW_THREADS

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while (err == Z_OK && self->zst.avail_out == 0) {
        if (length <= (UINT_MAX >> 1))
            new_length = length << 1;
        else
            new_length = UINT_MAX;
        if (_PyBytes_Resize(&RetVal, new_length) < 0) {
            Py_CLEAR(RetVal);
            goto done;
        }
        self->zst.next_out =
            (unsigned char *)PyBytes_AS_STRING(RetVal) + length;
        self->zst.avail_out = length;
        length = new_length;

        Py_BEGIN_ALLOW_THREADS
        err = deflate(&(self->zst), Z_NO_FLUSH);
        Py_END_ALLOW_THREADS
    }
    /* We will only get Z_BUF_ERROR if the output buffer was full but
       there wasn't more output when we tried again, so it is not an error
       condition.
    */

    if (err != Z_OK && err != Z_BUF_ERROR) {
        zlib_error(self->zst, err, "while compressing data");
        Py_CLEAR(RetVal);
        goto done;
    }
    if (_PyBytes_Resize(&RetVal, self->zst.total_out - start_total_out) < 0) {
        Py_CLEAR(RetVal);
    }

 done:
    LEAVE_ZLIB(self);
    return RetVal;
}

/* Helper for objdecompress() and unflush(). Saves any unconsumed input data in
   self->unused_data or self->unconsumed_tail, as appropriate. */
static int
save_unconsumed_input(compobject *self, int err)
{
    if (err == Z_STREAM_END) {
        /* The end of the compressed data has been reached. Store the leftover
           input data in self->unused_data. */
        if (self->zst.avail_in > 0) {
            Py_ssize_t old_size = PyBytes_GET_SIZE(self->unused_data);
            Py_ssize_t new_size;
            PyObject *new_data;
            if ((size_t)self->zst.avail_in > (size_t)UINT_MAX - (size_t)old_size) {
                PyErr_NoMemory();
                return -1;
            }
            new_size = old_size + self->zst.avail_in;
            new_data = PyBytes_FromStringAndSize(NULL, new_size);
            if (new_data == NULL)
                return -1;
            Py_MEMCPY(PyBytes_AS_STRING(new_data),
                      PyBytes_AS_STRING(self->unused_data), old_size);
            Py_MEMCPY(PyBytes_AS_STRING(new_data) + old_size,
                      self->zst.next_in, self->zst.avail_in);
            Py_DECREF(self->unused_data);
            self->unused_data = new_data;
            self->zst.avail_in = 0;
        }
    }
    if (self->zst.avail_in > 0 || PyBytes_GET_SIZE(self->unconsumed_tail)) {
        /* This code handles two distinct cases:
           1. Output limit was reached. Save leftover input in unconsumed_tail.
           2. All input data was consumed. Clear unconsumed_tail. */
        PyObject *new_data = PyBytes_FromStringAndSize(
                (char *)self->zst.next_in, self->zst.avail_in);
        if (new_data == NULL)
            return -1;
        Py_DECREF(self->unconsumed_tail);
        self->unconsumed_tail = new_data;
    }
    return 0;
}

/*[clinic input]
zlib.Decompress.decompress

    data: Py_buffer
        The binary data to decompress.
    max_length: uint = 0
        The maximum allowable length of the decompressed data.
        Unconsumed input data will be stored in
        the unconsumed_tail attribute.
    /

Return a bytes object containing the decompressed version of the data.

After calling this function, some of the input data may still be stored in
internal buffers for later processing.
Call the flush() method to clear these buffers.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, Py_buffer *data, unsigned int max_length)
/*[clinic end generated code: output=755cccc9087bfe55 input=02cfc047377cec86]*/
{
    int err;
    unsigned int old_length, length = DEF_BUF_SIZE;
    PyObject *RetVal = NULL;
    unsigned long start_total_out;

    if ((size_t)data->len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Size does not fit in an unsigned int");
        return NULL;
    }

    /* limit amount of data allocated to max_length */
    if (max_length && length > max_length)
        length = max_length;
    if (!(RetVal = PyBytes_FromStringAndSize(NULL, length)))
        return NULL;

    ENTER_ZLIB(self);

    start_total_out = self->zst.total_out;
    self->zst.avail_in = (unsigned int)data->len;
    self->zst.next_in = data->buf;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyBytes_AS_STRING(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = inflate(&(self->zst), Z_SYNC_FLUSH);
    Py_END_ALLOW_THREADS

    if (err == Z_NEED_DICT && self->zdict != NULL) {
        Py_buffer zdict_buf;
        if (PyObject_GetBuffer(self->zdict, &zdict_buf, PyBUF_SIMPLE) == -1) {
            Py_DECREF(RetVal);
            RetVal = NULL;
            goto error;
        }

        if ((size_t)zdict_buf.len > UINT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                    "zdict length does not fit in an unsigned int");
            PyBuffer_Release(&zdict_buf);
            Py_CLEAR(RetVal);
            goto error;
        }

        err = inflateSetDictionary(&(self->zst),
                                   zdict_buf.buf, (unsigned int)zdict_buf.len);
        PyBuffer_Release(&zdict_buf);
        if (err != Z_OK) {
            zlib_error(self->zst, err, "while decompressing data");
            Py_CLEAR(RetVal);
            goto error;
        }
        /* Repeat the call to inflate. */
        Py_BEGIN_ALLOW_THREADS
        err = inflate(&(self->zst), Z_SYNC_FLUSH);
        Py_END_ALLOW_THREADS
    }

    /* While Z_OK and the output buffer is full, there might be more output.
       So extend the output buffer and try again.
    */
    while (err == Z_OK && self->zst.avail_out == 0) {
        /* If max_length set, don't continue decompressing if we've already
           reached the limit.
        */
        if (max_length && length >= max_length)
            break;

        /* otherwise, ... */
        old_length = length;
        length = length << 1;
        if (max_length && length > max_length)
            length = max_length;

        if (_PyBytes_Resize(&RetVal, length) < 0) {
            Py_CLEAR(RetVal);
            goto error;
        }
        self->zst.next_out =
            (unsigned char *)PyBytes_AS_STRING(RetVal) + old_length;
        self->zst.avail_out = length - old_length;

        Py_BEGIN_ALLOW_THREADS
        err = inflate(&(self->zst), Z_SYNC_FLUSH);
        Py_END_ALLOW_THREADS
    }

    if (save_unconsumed_input(self, err) < 0) {
        Py_DECREF(RetVal);
        RetVal = NULL;
        goto error;
    }

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
        Py_DECREF(RetVal);
        RetVal = NULL;
        goto error;
    }

    if (_PyBytes_Resize(&RetVal, self->zst.total_out - start_total_out) < 0) {
        Py_CLEAR(RetVal);
    }

 error:
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
    unsigned int length = DEF_BUF_SIZE, new_length;
    PyObject *RetVal;
    unsigned long start_total_out;

    /* Flushing with Z_NO_FLUSH is a no-op, so there's no point in
       doing any work at all; just return an empty string. */
    if (mode == Z_NO_FLUSH) {
        return PyBytes_FromStringAndSize(NULL, 0);
    }

    if (!(RetVal = PyBytes_FromStringAndSize(NULL, length)))
        return NULL;

    ENTER_ZLIB(self);

    start_total_out = self->zst.total_out;
    self->zst.avail_in = 0;
    self->zst.avail_out = length;
    self->zst.next_out = (unsigned char *)PyBytes_AS_STRING(RetVal);

    Py_BEGIN_ALLOW_THREADS
    err = deflate(&(self->zst), mode);
    Py_END_ALLOW_THREADS

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while (err == Z_OK && self->zst.avail_out == 0) {
        if (length <= (UINT_MAX >> 1))
            new_length = length << 1;
        else
            new_length = UINT_MAX;
        if (_PyBytes_Resize(&RetVal, new_length) < 0) {
            Py_CLEAR(RetVal);
            goto error;
        }
        self->zst.next_out =
            (unsigned char *)PyBytes_AS_STRING(RetVal) + length;
        self->zst.avail_out = length;
        length = new_length;

        Py_BEGIN_ALLOW_THREADS
        err = deflate(&(self->zst), mode);
        Py_END_ALLOW_THREADS
    }

    /* If mode is Z_FINISH, we also have to call deflateEnd() to free
       various data structures. Note we should only get Z_STREAM_END when
       mode is Z_FINISH, but checking both for safety*/
    if (err == Z_STREAM_END && mode == Z_FINISH) {
        err = deflateEnd(&(self->zst));
        if (err != Z_OK) {
            zlib_error(self->zst, err, "while finishing compression");
            Py_DECREF(RetVal);
            RetVal = NULL;
            goto error;
        }
        else
            self->is_initialised = 0;

        /* We will only get Z_BUF_ERROR if the output buffer was full
           but there wasn't more output when we tried again, so it is
           not an error condition.
        */
    } else if (err!=Z_OK && err!=Z_BUF_ERROR) {
        zlib_error(self->zst, err, "while flushing");
        Py_DECREF(RetVal);
        RetVal = NULL;
        goto error;
    }

    if (_PyBytes_Resize(&RetVal, self->zst.total_out - start_total_out) < 0) {
        Py_CLEAR(RetVal);
    }

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
    switch(err) {
    case(Z_OK):
        break;
    case(Z_STREAM_ERROR):
        PyErr_SetString(PyExc_ValueError, "Inconsistent stream state");
        goto error;
    case(Z_MEM_ERROR):
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for compression object");
        goto error;
    default:
        zlib_error(self->zst, err, "while copying compression object");
        goto error;
    }
    Py_INCREF(self->unused_data);
    Py_INCREF(self->unconsumed_tail);
    Py_XINCREF(self->zdict);
    Py_XDECREF(retval->unused_data);
    Py_XDECREF(retval->unconsumed_tail);
    Py_XDECREF(retval->zdict);
    retval->unused_data = self->unused_data;
    retval->unconsumed_tail = self->unconsumed_tail;
    retval->zdict = self->zdict;
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
    switch(err) {
    case(Z_OK):
        break;
    case(Z_STREAM_ERROR):
        PyErr_SetString(PyExc_ValueError, "Inconsistent stream state");
        goto error;
    case(Z_MEM_ERROR):
        PyErr_SetString(PyExc_MemoryError,
                        "Can't allocate memory for decompression object");
        goto error;
    default:
        zlib_error(self->zst, err, "while copying decompression object");
        goto error;
    }

    Py_INCREF(self->unused_data);
    Py_INCREF(self->unconsumed_tail);
    Py_XINCREF(self->zdict);
    Py_XDECREF(retval->unused_data);
    Py_XDECREF(retval->unconsumed_tail);
    Py_XDECREF(retval->zdict);
    retval->unused_data = self->unused_data;
    retval->unconsumed_tail = self->unconsumed_tail;
    retval->zdict = self->zdict;
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

    length: uint(c_default="DEF_BUF_SIZE") = zlib.DEF_BUF_SIZE
        the initial size of the output buffer.
    /

Return a bytes object containing any remaining decompressed data.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_flush_impl(compobject *self, unsigned int length)
/*[clinic end generated code: output=db6fb753ab698e22 input=1580956505978993]*/
{
    int err;
    unsigned int new_length;
    PyObject * retval = NULL;
    unsigned long start_total_out;
    Py_ssize_t size;

    if (length == 0) {
        PyErr_SetString(PyExc_ValueError, "length must be greater than zero");
        return NULL;
    }

    if (!(retval = PyBytes_FromStringAndSize(NULL, length)))
        return NULL;


    ENTER_ZLIB(self);

    size = PyBytes_GET_SIZE(self->unconsumed_tail);

    start_total_out = self->zst.total_out;
    /* save_unconsumed_input() ensures that unconsumed_tail length is lesser
       or equal than UINT_MAX */
    self->zst.avail_in = Py_SAFE_DOWNCAST(size, Py_ssize_t, unsigned int);
    self->zst.next_in = (Byte *)PyBytes_AS_STRING(self->unconsumed_tail);
    self->zst.avail_out = length;
    self->zst.next_out = (Byte *)PyBytes_AS_STRING(retval);

    Py_BEGIN_ALLOW_THREADS
    err = inflate(&(self->zst), Z_FINISH);
    Py_END_ALLOW_THREADS

    /* while Z_OK and the output buffer is full, there might be more output,
       so extend the output buffer and try again */
    while ((err == Z_OK || err == Z_BUF_ERROR) && self->zst.avail_out == 0) {
        if (length <= (UINT_MAX >> 1))
            new_length = length << 1;
        else
            new_length = UINT_MAX;
        if (_PyBytes_Resize(&retval, new_length) < 0) {
            Py_CLEAR(retval);
            goto error;
        }
        self->zst.next_out = (Byte *)PyBytes_AS_STRING(retval) + length;
        self->zst.avail_out = length;
        length = new_length;

        Py_BEGIN_ALLOW_THREADS
        err = inflate(&(self->zst), Z_FINISH);
        Py_END_ALLOW_THREADS
    }

    if (save_unconsumed_input(self, err) < 0) {
        Py_DECREF(retval);
        retval = NULL;
        goto error;
    }

    /* If at end of stream, clean up any memory allocated by zlib. */
    if (err == Z_STREAM_END) {
        self->eof = 1;
        self->is_initialised = 0;
        err = inflateEnd(&(self->zst));
        if (err != Z_OK) {
            zlib_error(self->zst, err, "while finishing decompression");
            Py_DECREF(retval);
            retval = NULL;
            goto error;
        }
    }

    if (_PyBytes_Resize(&retval, self->zst.total_out - start_total_out) < 0) {
        Py_CLEAR(retval);
    }

error:

    LEAVE_ZLIB(self);

    return retval;
}

#include "clinic/zlibmodule.c.h"

static PyMethodDef comp_methods[] =
{
    ZLIB_COMPRESS_COMPRESS_METHODDEF
    ZLIB_COMPRESS_FLUSH_METHODDEF
#ifdef HAVE_ZLIB_COPY
    ZLIB_COMPRESS_COPY_METHODDEF
#endif
    {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
    ZLIB_DECOMPRESS_DECOMPRESS_METHODDEF
    ZLIB_DECOMPRESS_FLUSH_METHODDEF
#ifdef HAVE_ZLIB_COPY
    ZLIB_DECOMPRESS_COPY_METHODDEF
#endif
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
zlib_adler32_impl(PyModuleDef *module, Py_buffer *data, unsigned int value)
/*[clinic end generated code: output=51d6d75ee655c78a input=6ff4557872160e88]*/
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
zlib_crc32_impl(PyModuleDef *module, Py_buffer *data, unsigned int value)
/*[clinic end generated code: output=c1e986e74fe7b623 input=26c3ed430fa00b4c]*/
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
"compress(string[, level]) -- Compress string, with compression level in 0-9.\n"
"compressobj([level[, ...]]) -- Return a compressor object.\n"
"crc32(string[, start]) -- Compute a CRC-32 checksum.\n"
"decompress(string,[wbits],[bufsize]) -- Decompresses a compressed string.\n"
"decompressobj([wbits[, zdict]]]) -- Return a decompressor object.\n"
"\n"
"'wbits' is window buffer size.\n"
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
    PyModule_AddIntMacro(m, Z_BEST_SPEED);
    PyModule_AddIntMacro(m, Z_BEST_COMPRESSION);
    PyModule_AddIntMacro(m, Z_DEFAULT_COMPRESSION);
    PyModule_AddIntMacro(m, Z_FILTERED);
    PyModule_AddIntMacro(m, Z_HUFFMAN_ONLY);
    PyModule_AddIntMacro(m, Z_DEFAULT_STRATEGY);

    PyModule_AddIntMacro(m, Z_FINISH);
    PyModule_AddIntMacro(m, Z_NO_FLUSH);
    PyModule_AddIntMacro(m, Z_SYNC_FLUSH);
    PyModule_AddIntMacro(m, Z_FULL_FLUSH);

    ver = PyUnicode_FromString(ZLIB_VERSION);
    if (ver != NULL)
        PyModule_AddObject(m, "ZLIB_VERSION", ver);

    ver = PyUnicode_FromString(zlibVersion());
    if (ver != NULL)
        PyModule_AddObject(m, "ZLIB_RUNTIME_VERSION", ver);

    PyModule_AddStringConstant(m, "__version__", "1.0");

    return m;
}
