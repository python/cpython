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
#define DEF_WBITS MAX_WBITS

/* The output buffer will be increased in chunks of DEFAULTALLOC bytes. */
#define DEFAULTALLOC (16*1024)

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
module zlib
class zlib.Compress
class zlib.Decompress
[clinic start generated code]*/
/*[clinic end generated code: checksum=da39a3ee5e6b4b0d3255bfef95601890afd80709]*/

PyDoc_STRVAR(compressobj__doc__,
"compressobj(level=-1, method=DEFLATED, wbits=15, memlevel=8,\n"
"            strategy=Z_DEFAULT_STRATEGY[, zdict])\n"
" -- Return a compressor object.\n"
"\n"
"level is the compression level (an integer in the range 0-9; default is 6).\n"
"Higher compression levels are slower, but produce smaller results.\n"
"\n"
"method is the compression algorithm. If given, this must be DEFLATED.\n"
"\n"
"wbits is the base two logarithm of the window size (range: 8..15).\n"
"\n"
"memlevel controls the amount of memory used for internal compression state.\n"
"Valid values range from 1 to 9. Higher values result in higher memory usage,\n"
"faster compression, and smaller output.\n"
"\n"
"strategy is used to tune the compression algorithm. Possible values are\n"
"Z_DEFAULT_STRATEGY, Z_FILTERED, and Z_HUFFMAN_ONLY.\n"
"\n"
"zdict is the predefined compression dictionary - a sequence of bytes\n"
"containing subsequences that are likely to occur in the input data.");

PyDoc_STRVAR(decompressobj__doc__,
"decompressobj([wbits[, zdict]]) -- Return a decompressor object.\n"
"\n"
"Optional arg wbits is the window buffer size.\n"
"\n"
"Optional arg zdict is the predefined compression dictionary. This must be\n"
"the same dictionary as used by the compressor that produced the input data.");

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
    [
    level: int
        Compression level, in 0-9.
    ]
    /

Returns compressed string.

[clinic start generated code]*/

PyDoc_STRVAR(zlib_compress__doc__,
"compress(module, bytes, [level])\n"
"Returns compressed string.\n"
"\n"
"  bytes\n"
"    Binary data to be compressed.\n"
"  level\n"
"    Compression level, in 0-9.");

#define ZLIB_COMPRESS_METHODDEF    \
    {"compress", (PyCFunction)zlib_compress, METH_VARARGS, zlib_compress__doc__},

static PyObject *
zlib_compress_impl(PyModuleDef *module, Py_buffer *bytes, int group_right_1, int level);

static PyObject *
zlib_compress(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer bytes = {NULL, NULL};
    int group_right_1 = 0;
    int level = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "y*:compress", &bytes))
                goto exit;
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "y*i:compress", &bytes, &level))
                goto exit;
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "zlib.compress requires 1 to 2 arguments");
            goto exit;
    }
    return_value = zlib_compress_impl(module, &bytes, group_right_1, level);

exit:
    /* Cleanup for bytes */
    if (bytes.obj)
       PyBuffer_Release(&bytes);

    return return_value;
}

static PyObject *
zlib_compress_impl(PyModuleDef *module, Py_buffer *bytes, int group_right_1, int level)
/*[clinic end generated code: checksum=ce8d4c0a17ecd79c3ffcc032dcdf8ac6830ded1e]*/
{
    PyObject *ReturnVal = NULL;
    Byte *input, *output = NULL;
    unsigned int length;
    int err;
    z_stream zst;

    if (!group_right_1)
        level = Z_DEFAULT_COMPRESSION;

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

[python start generated code]*/
/*[python end generated code: checksum=da39a3ee5e6b4b0d3255bfef95601890afd80709]*/

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

PyDoc_STRVAR(decompress__doc__,
"decompress(string[, wbits[, bufsize]]) -- Return decompressed string.\n"
"\n"
"Optional arg wbits is the window buffer size.  Optional arg bufsize is\n"
"the initial output buffer size.");

static PyObject *
PyZlib_decompress(PyObject *self, PyObject *args)
{
    PyObject *result_str = NULL;
    Py_buffer pinput;
    Byte *input;
    unsigned int length;
    int err;
    int wsize=DEF_WBITS;
    unsigned int bufsize = DEFAULTALLOC, new_bufsize;
    z_stream zst;

    if (!PyArg_ParseTuple(args, "y*|iO&:decompress",
                          &pinput, &wsize, uint_converter, &bufsize))
        return NULL;

    if ((size_t)pinput.len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Size does not fit in an unsigned int");
        goto error;
    }
    input = pinput.buf;
    length = (unsigned int)pinput.len;

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
    err = inflateInit2(&zst, wsize);

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

    PyBuffer_Release(&pinput);
    return result_str;

 error:
    PyBuffer_Release(&pinput);
    Py_XDECREF(result_str);
    return NULL;
}

static PyObject *
PyZlib_compressobj(PyObject *selfptr, PyObject *args, PyObject *kwargs)
{
    compobject *self = NULL;
    int level=Z_DEFAULT_COMPRESSION, method=DEFLATED;
    int wbits=MAX_WBITS, memLevel=DEF_MEM_LEVEL, strategy=0, err;
    Py_buffer zdict;
    static char *kwlist[] = {"level", "method", "wbits",
                             "memLevel", "strategy", "zdict", NULL};

    zdict.buf = NULL; /* Sentinel, so we can tell whether zdict was supplied. */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iiiiiy*:compressobj",
                                     kwlist, &level, &method, &wbits,
                                     &memLevel, &strategy, &zdict))
        return NULL;

    if (zdict.buf != NULL && (size_t)zdict.len > UINT_MAX) {
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
        if (zdict.buf == NULL) {
            goto success;
        } else {
            err = deflateSetDictionary(&self->zst,
                                       zdict.buf, (unsigned int)zdict.len);
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
    Py_XDECREF(self);
    self = NULL;
 success:
    if (zdict.buf != NULL)
        PyBuffer_Release(&zdict);
    return (PyObject*)self;
}

static PyObject *
PyZlib_decompressobj(PyObject *selfptr, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"wbits", "zdict", NULL};
    int wbits=DEF_WBITS, err;
    compobject *self;
    PyObject *zdict=NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|iO:decompressobj",
                                     kwlist, &wbits, &zdict))
        return NULL;
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

PyDoc_STRVAR(comp_compress__doc__,
"compress(data) -- Return a string containing data compressed.\n"
"\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.");


static PyObject *
PyZlib_objcompress(compobject *self, PyObject *args)
{
    int err;
    unsigned int inplen;
    unsigned int length = DEFAULTALLOC, new_length;
    PyObject *RetVal = NULL;
    Py_buffer pinput;
    Byte *input;
    unsigned long start_total_out;

    if (!PyArg_ParseTuple(args, "y*:compress", &pinput))
        return NULL;
    if ((size_t)pinput.len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Size does not fit in an unsigned int");
        goto error_outer;
    }
    input = pinput.buf;
    inplen = (unsigned int)pinput.len;

    if (!(RetVal = PyBytes_FromStringAndSize(NULL, length)))
        goto error_outer;

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
            goto error;
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
        Py_DECREF(RetVal);
        RetVal = NULL;
        goto error;
    }
    if (_PyBytes_Resize(&RetVal, self->zst.total_out - start_total_out) < 0) {
        Py_CLEAR(RetVal);
    }

 error:
    LEAVE_ZLIB(self);
 error_outer:
    PyBuffer_Release(&pinput);
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

    self: self(type="compobject *")

    data: Py_buffer
        The binary data to decompress.
    max_length: uint = 0
        The maximum allowable length of the decompressed data.
        Unconsumed input data will be stored in
        the unconsumed_tail attribute.
    /

Return a string containing the decompressed version of the data.

After calling this function, some of the input data may still be stored in
internal buffers for later processing.
Call the flush() method to clear these buffers.
[clinic start generated code]*/

PyDoc_STRVAR(zlib_Decompress_decompress__doc__,
"decompress(self, data, max_length=0)\n"
"Return a string containing the decompressed version of the data.\n"
"\n"
"  data\n"
"    The binary data to decompress.\n"
"  max_length\n"
"    The maximum allowable length of the decompressed data.\n"
"    Unconsumed input data will be stored in\n"
"    the unconsumed_tail attribute.\n"
"\n"
"After calling this function, some of the input data may still be stored in\n"
"internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.");

#define ZLIB_DECOMPRESS_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)zlib_Decompress_decompress, METH_VARARGS, zlib_Decompress_decompress__doc__},

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, Py_buffer *data, unsigned int max_length);

static PyObject *
zlib_Decompress_decompress(compobject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int max_length = 0;

    if (!PyArg_ParseTuple(args,
        "y*|O&:decompress",
        &data, uint_converter, &max_length))
        goto exit;
    return_value = zlib_Decompress_decompress_impl(self, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, Py_buffer *data, unsigned int max_length)
/*[clinic end generated code: checksum=b7fd2e3b23430f57f5a84817189575bc46464901]*/
{
    int err;
    unsigned int old_length, length = DEFAULTALLOC;
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

PyDoc_STRVAR(comp_flush__doc__,
"flush( [mode] ) -- Return a string containing any remaining compressed data.\n"
"\n"
"mode can be one of the constants Z_SYNC_FLUSH, Z_FULL_FLUSH, Z_FINISH; the\n"
"default value used when mode is not specified is Z_FINISH.\n"
"If mode == Z_FINISH, the compressor object can no longer be used after\n"
"calling the flush() method.  Otherwise, more data can still be compressed.");

static PyObject *
PyZlib_flush(compobject *self, PyObject *args)
{
    int err;
    unsigned int length = DEFAULTALLOC, new_length;
    PyObject *RetVal;
    int flushmode = Z_FINISH;
    unsigned long start_total_out;

    if (!PyArg_ParseTuple(args, "|i:flush", &flushmode))
        return NULL;

    /* Flushing with Z_NO_FLUSH is a no-op, so there's no point in
       doing any work at all; just return an empty string. */
    if (flushmode == Z_NO_FLUSH) {
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
    err = deflate(&(self->zst), flushmode);
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
        err = deflate(&(self->zst), flushmode);
        Py_END_ALLOW_THREADS
    }

    /* If flushmode is Z_FINISH, we also have to call deflateEnd() to free
       various data structures. Note we should only get Z_STREAM_END when
       flushmode is Z_FINISH, but checking both for safety*/
    if (err == Z_STREAM_END && flushmode == Z_FINISH) {
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

    self: self(type="compobject *")

Return a copy of the compression object.
[clinic start generated code]*/

PyDoc_STRVAR(zlib_Compress_copy__doc__,
"copy(self)\n"
"Return a copy of the compression object.");

#define ZLIB_COMPRESS_COPY_METHODDEF    \
    {"copy", (PyCFunction)zlib_Compress_copy, METH_NOARGS, zlib_Compress_copy__doc__},

static PyObject *
zlib_Compress_copy_impl(compobject *self);

static PyObject *
zlib_Compress_copy(compobject *self, PyObject *Py_UNUSED(ignored))
{
    return zlib_Compress_copy_impl(self);
}

static PyObject *
zlib_Compress_copy_impl(compobject *self)
/*[clinic end generated code: checksum=7aa841ad51297eb83250f511a76872e88fdc737e]*/
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

PyDoc_STRVAR(decomp_copy__doc__,
"copy() -- Return a copy of the decompression object.");

static PyObject *
PyZlib_uncopy(compobject *self)
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

PyDoc_STRVAR(decomp_flush__doc__,
"flush( [length] ) -- Return a string containing any remaining\n"
"decompressed data. length, if given, is the initial size of the\n"
"output buffer.\n"
"\n"
"The decompressor object can no longer be used after this call.");

static PyObject *
PyZlib_unflush(compobject *self, PyObject *args)
{
    int err;
    unsigned int length = DEFAULTALLOC, new_length;
    PyObject * retval = NULL;
    unsigned long start_total_out;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "|O&:flush", uint_converter, &length))
        return NULL;
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

static PyMethodDef comp_methods[] =
{
    {"compress", (binaryfunc)PyZlib_objcompress, METH_VARARGS,
                 comp_compress__doc__},
    {"flush", (binaryfunc)PyZlib_flush, METH_VARARGS,
              comp_flush__doc__},
#ifdef HAVE_ZLIB_COPY
    ZLIB_COMPRESS_COPY_METHODDEF
#endif
    {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
    ZLIB_DECOMPRESS_DECOMPRESS_METHODDEF
    {"flush", (binaryfunc)PyZlib_unflush, METH_VARARGS,
              decomp_flush__doc__},
#ifdef HAVE_ZLIB_COPY
    {"copy",  (PyCFunction)PyZlib_uncopy, METH_NOARGS,
              decomp_copy__doc__},
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

PyDoc_STRVAR(adler32__doc__,
"adler32(string[, start]) -- Compute an Adler-32 checksum of string.\n"
"\n"
"An optional starting value can be specified.  The returned checksum is\n"
"an integer.");

static PyObject *
PyZlib_adler32(PyObject *self, PyObject *args)
{
    unsigned int adler32val = 1;  /* adler32(0L, Z_NULL, 0) */
    Py_buffer pbuf;

    if (!PyArg_ParseTuple(args, "y*|I:adler32", &pbuf, &adler32val))
        return NULL;
    /* Releasing the GIL for very small buffers is inefficient
       and may lower performance */
    if (pbuf.len > 1024*5) {
        unsigned char *buf = pbuf.buf;
        Py_ssize_t len = pbuf.len;

        Py_BEGIN_ALLOW_THREADS
        /* Avoid truncation of length for very large buffers. adler32() takes
           length as an unsigned int, which may be narrower than Py_ssize_t. */
        while ((size_t)len > UINT_MAX) {
            adler32val = adler32(adler32val, buf, UINT_MAX);
            buf += (size_t) UINT_MAX;
            len -= (size_t) UINT_MAX;
        }
        adler32val = adler32(adler32val, buf, (unsigned int)len);
        Py_END_ALLOW_THREADS
    } else {
        adler32val = adler32(adler32val, pbuf.buf, (unsigned int)pbuf.len);
    }
    PyBuffer_Release(&pbuf);
    return PyLong_FromUnsignedLong(adler32val & 0xffffffffU);
}

PyDoc_STRVAR(crc32__doc__,
"crc32(string[, start]) -- Compute a CRC-32 checksum of string.\n"
"\n"
"An optional starting value can be specified.  The returned checksum is\n"
"an integer.");

static PyObject *
PyZlib_crc32(PyObject *self, PyObject *args)
{
    unsigned int crc32val = 0;  /* crc32(0L, Z_NULL, 0) */
    Py_buffer pbuf;
    int signed_val;

    if (!PyArg_ParseTuple(args, "y*|I:crc32", &pbuf, &crc32val))
        return NULL;
    /* Releasing the GIL for very small buffers is inefficient
       and may lower performance */
    if (pbuf.len > 1024*5) {
        unsigned char *buf = pbuf.buf;
        Py_ssize_t len = pbuf.len;

        Py_BEGIN_ALLOW_THREADS
        /* Avoid truncation of length for very large buffers. crc32() takes
           length as an unsigned int, which may be narrower than Py_ssize_t. */
        while ((size_t)len > UINT_MAX) {
            crc32val = crc32(crc32val, buf, UINT_MAX);
            buf += (size_t) UINT_MAX;
            len -= (size_t) UINT_MAX;
        }
        signed_val = crc32(crc32val, buf, (unsigned int)len);
        Py_END_ALLOW_THREADS
    } else {
        signed_val = crc32(crc32val, pbuf.buf, (unsigned int)pbuf.len);
    }
    PyBuffer_Release(&pbuf);
    return PyLong_FromUnsignedLong(signed_val & 0xffffffffU);
}


static PyMethodDef zlib_methods[] =
{
    {"adler32", (PyCFunction)PyZlib_adler32, METH_VARARGS,
                adler32__doc__},
    ZLIB_COMPRESS_METHODDEF
    {"compressobj", (PyCFunction)PyZlib_compressobj, METH_VARARGS|METH_KEYWORDS,
                    compressobj__doc__},
    {"crc32", (PyCFunction)PyZlib_crc32, METH_VARARGS,
              crc32__doc__},
    {"decompress", (PyCFunction)PyZlib_decompress, METH_VARARGS,
                   decompress__doc__},
    {"decompressobj", (PyCFunction)PyZlib_decompressobj, METH_VARARGS|METH_KEYWORDS,
                   decompressobj__doc__},
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
