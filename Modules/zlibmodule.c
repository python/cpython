/* zlibmodule.c -- gzip-compatible data compression */
/* See http://zlib.net/ */

/* Windows users:  read Python's PCbuild\readme.txt */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h"         // PyMemberDef
#include "zlib.h"

// Blocks output buffer wrappers
#include "pycore_blocks_output_buffer.h"

#if OUTPUT_BUFFER_MAX_BLOCK_SIZE > UINT32_MAX
    #error "The maximum block size accepted by zlib is UINT32_MAX."
#endif

/* On success, return value >= 0
   On failure, return -1 */
static inline Py_ssize_t
OutputBuffer_InitAndGrow(_BlocksOutputBuffer *buffer, Py_ssize_t max_length,
                         Bytef **next_out, uint32_t *avail_out)
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
                  Bytef **next_out, uint32_t *avail_out)
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

/* The max buffer size accepted by zlib is UINT32_MAX, the initial buffer size
   `init_size` may > it in 64-bit build. These wrapper functions maintain an
   UINT32_MAX sliding window for the first block:
    1. OutputBuffer_WindowInitWithSize()
    2. OutputBuffer_WindowGrow()
    3. OutputBuffer_WindowFinish()
    4. OutputBuffer_WindowOnError()

   ==== is the sliding window:
    1. ====------
           ^ next_posi, left_bytes is 6
    2. ----====--
               ^ next_posi, left_bytes is 2
    3. --------==
                 ^ next_posi, left_bytes is 0  */
typedef struct {
    Py_ssize_t left_bytes;
    Bytef *next_posi;
} _Uint32Window;

/* Initialize the buffer with an inital buffer size.

   On success, return value >= 0
   On failure, return value < 0 */
static inline Py_ssize_t
OutputBuffer_WindowInitWithSize(_BlocksOutputBuffer *buffer, _Uint32Window *window,
                                Py_ssize_t init_size,
                                Bytef **next_out, uint32_t *avail_out)
{
    Py_ssize_t allocated = _BlocksOutputBuffer_InitWithSize(
                               buffer, init_size, (void**) next_out);

    if (allocated >= 0) {
        // the UINT32_MAX sliding window
        Py_ssize_t window_size = Py_MIN((size_t)allocated, UINT32_MAX);
        *avail_out = (uint32_t) window_size;

        window->left_bytes = allocated - window_size;
        window->next_posi = *next_out + window_size;
    }
    return allocated;
}

/* Grow the buffer.

   On success, return value >= 0
   On failure, return value < 0 */
static inline Py_ssize_t
OutputBuffer_WindowGrow(_BlocksOutputBuffer *buffer, _Uint32Window *window,
                        Bytef **next_out, uint32_t *avail_out)
{
    Py_ssize_t allocated;

    /* ensure no gaps in the data.
       if inlined, this check could be optimized away.*/
    if (*avail_out != 0) {
        PyErr_SetString(PyExc_SystemError,
                        "*avail_out != 0 in OutputBuffer_WindowGrow().");
        return -1;
    }

    // slide the UINT32_MAX sliding window
    if (window->left_bytes > 0) {
        Py_ssize_t window_size = Py_MIN((size_t)window->left_bytes, UINT32_MAX);

        *next_out = window->next_posi;
        *avail_out = (uint32_t) window_size;

        window->left_bytes -= window_size;
        window->next_posi += window_size;

        return window_size;
    }
    assert(window->left_bytes == 0);

    // only the first block may > UINT32_MAX
    allocated = _BlocksOutputBuffer_Grow(
                    buffer, (void**) next_out, (Py_ssize_t) *avail_out);
    *avail_out = (uint32_t) allocated;
    return allocated;
}

/* Finish the buffer.

   On success, return a bytes object
   On failure, return NULL */
static inline PyObject *
OutputBuffer_WindowFinish(_BlocksOutputBuffer *buffer, _Uint32Window *window,
                          uint32_t avail_out)
{
    Py_ssize_t real_avail_out = (Py_ssize_t) avail_out + window->left_bytes;
    return _BlocksOutputBuffer_Finish(buffer, real_avail_out);
}

static inline void
OutputBuffer_WindowOnError(_BlocksOutputBuffer *buffer, _Uint32Window *window)
{
    _BlocksOutputBuffer_OnError(buffer);
}


#define ENTER_ZLIB(obj) do {                      \
    if (!PyThread_acquire_lock((obj)->lock, 0)) { \
        Py_BEGIN_ALLOW_THREADS                    \
        PyThread_acquire_lock((obj)->lock, 1);    \
        Py_END_ALLOW_THREADS                      \
    } } while (0)
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

static PyModuleDef zlibmodule;

typedef struct {
    PyTypeObject *Comptype;
    PyTypeObject *Decomptype;
    PyObject *ZlibError;
} zlibstate;

static inline zlibstate*
get_zlib_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (zlibstate *)state;
}

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
zlib_error(zlibstate *state, z_stream zst, int err, const char *msg)
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
        PyErr_Format(state->ZlibError, "Error %d %s", err, msg);
    else
        PyErr_Format(state->ZlibError, "Error %d %s: %.200s", err, msg, zmsg);
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
    PyObject *RetVal;
    int flush;
    z_stream zst;
    _BlocksOutputBuffer buffer = {.list = NULL};

    zlibstate *state = get_zlib_state(module);

    Byte *ibuf = data->buf;
    Py_ssize_t ibuflen = data->len;

    if (OutputBuffer_InitAndGrow(&buffer, -1, &zst.next_out, &zst.avail_out) < 0) {
        goto error;
    }

    zst.opaque = NULL;
    zst.zalloc = PyZlib_Malloc;
    zst.zfree = PyZlib_Free;
    zst.next_in = ibuf;
    int err = deflateInit(&zst, level);

    switch (err) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        PyErr_SetString(PyExc_MemoryError,
                        "Out of memory while compressing data");
        goto error;
    case Z_STREAM_ERROR:
        PyErr_SetString(state->ZlibError, "Bad compression level");
        goto error;
    default:
        deflateEnd(&zst);
        zlib_error(state, zst, err, "while compressing data");
        goto error;
    }

    do {
        arrange_input_buffer(&zst, &ibuflen);
        flush = ibuflen == 0 ? Z_FINISH : Z_NO_FLUSH;

        do {
            if (zst.avail_out == 0) {
                if (OutputBuffer_Grow(&buffer, &zst.next_out, &zst.avail_out) < 0) {
                    deflateEnd(&zst);
                    goto error;
                }
            }

            Py_BEGIN_ALLOW_THREADS
            err = deflate(&zst, flush);
            Py_END_ALLOW_THREADS

            if (err == Z_STREAM_ERROR) {
                deflateEnd(&zst);
                zlib_error(state, zst, err, "while compressing data");
                goto error;
            }

        } while (zst.avail_out == 0);
        assert(zst.avail_in == 0);

    } while (flush != Z_FINISH);
    assert(err == Z_STREAM_END);

    err = deflateEnd(&zst);
    if (err == Z_OK) {
        RetVal = OutputBuffer_Finish(&buffer, zst.avail_out);
        if (RetVal == NULL) {
            goto error;
        }
        return RetVal;
    }
    else
        zlib_error(state, zst, err, "while finishing compression");
 error:
    OutputBuffer_OnError(&buffer);
    return NULL;
}

/*[clinic input]
zlib.decompress

    data: Py_buffer
        Compressed data.
    /
    wbits: int(c_default="MAX_WBITS") = MAX_WBITS
        The window buffer size and container format.
    bufsize: Py_ssize_t(c_default="DEF_BUF_SIZE") = DEF_BUF_SIZE
        The initial output buffer size.

Returns a bytes object containing the uncompressed data.
[clinic start generated code]*/

static PyObject *
zlib_decompress_impl(PyObject *module, Py_buffer *data, int wbits,
                     Py_ssize_t bufsize)
/*[clinic end generated code: output=77c7e35111dc8c42 input=a9ac17beff1f893f]*/
{
    PyObject *RetVal;
    Byte *ibuf;
    Py_ssize_t ibuflen;
    int err, flush;
    z_stream zst;
    _BlocksOutputBuffer buffer = {.list = NULL};
    _Uint32Window window;  // output buffer's UINT32_MAX sliding window

    zlibstate *state = get_zlib_state(module);

    if (bufsize < 0) {
        PyErr_SetString(PyExc_ValueError, "bufsize must be non-negative");
        return NULL;
    } else if (bufsize == 0) {
        bufsize = 1;
    }

    if (OutputBuffer_WindowInitWithSize(&buffer, &window, bufsize,
                                        &zst.next_out, &zst.avail_out) < 0) {
        goto error;
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
        zlib_error(state, zst, err, "while preparing to decompress data");
        goto error;
    }

    do {
        arrange_input_buffer(&zst, &ibuflen);
        flush = ibuflen == 0 ? Z_FINISH : Z_NO_FLUSH;

        do {
            if (zst.avail_out == 0) {
                if (OutputBuffer_WindowGrow(&buffer, &window,
                                            &zst.next_out, &zst.avail_out) < 0) {
                    inflateEnd(&zst);
                    goto error;
                }
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
                zlib_error(state, zst, err, "while decompressing data");
                goto error;
            }

        } while (zst.avail_out == 0);

    } while (err != Z_STREAM_END && ibuflen != 0);


    if (err != Z_STREAM_END) {
        inflateEnd(&zst);
        zlib_error(state, zst, err, "while decompressing data");
        goto error;
    }

    err = inflateEnd(&zst);
    if (err != Z_OK) {
        zlib_error(state, zst, err, "while finishing decompression");
        goto error;
    }

    RetVal = OutputBuffer_WindowFinish(&buffer, &window, zst.avail_out);
    if (RetVal != NULL) {
        return RetVal;
    }

 error:
    OutputBuffer_WindowOnError(&buffer, &window);
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
    zlibstate *state = get_zlib_state(module);
    if (zdict->buf != NULL && (size_t)zdict->len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "zdict length does not fit in an unsigned int");
        return NULL;
    }

    compobject *self = newcompobject(state->Comptype);
    if (self == NULL)
        goto error;
    self->zst.opaque = NULL;
    self->zst.zalloc = PyZlib_Malloc;
    self->zst.zfree = PyZlib_Free;
    self->zst.next_in = NULL;
    self->zst.avail_in = 0;
    int err = deflateInit2(&self->zst, level, method, wbits, memLevel, strategy);
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
        zlib_error(state, self->zst, err, "while creating compression object");
        goto error;
    }

 error:
    Py_CLEAR(self);
 success:
    return (PyObject *)self;
}

static int
set_inflate_zdict(zlibstate *state, compobject *self)
{
    Py_buffer zdict_buf;
    if (PyObject_GetBuffer(self->zdict, &zdict_buf, PyBUF_SIMPLE) == -1) {
        return -1;
    }
    if ((size_t)zdict_buf.len > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "zdict length does not fit in an unsigned int");
        PyBuffer_Release(&zdict_buf);
        return -1;
    }
    int err;
    err = inflateSetDictionary(&self->zst,
                               zdict_buf.buf, (unsigned int)zdict_buf.len);
    PyBuffer_Release(&zdict_buf);
    if (err != Z_OK) {
        zlib_error(state, self->zst, err, "while setting zdict");
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
    zlibstate *state = get_zlib_state(module);

    if (zdict != NULL && !PyObject_CheckBuffer(zdict)) {
        PyErr_SetString(PyExc_TypeError,
                        "zdict argument must support the buffer protocol");
        return NULL;
    }

    compobject *self = newcompobject(state->Decomptype);
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
    int err = inflateInit2(&self->zst, wbits);
    switch (err) {
    case Z_OK:
        self->is_initialised = 1;
        if (self->zdict != NULL && wbits < 0) {
#ifdef AT_LEAST_ZLIB_1_2_2_1
            if (set_inflate_zdict(state, self) < 0) {
                Py_DECREF(self);
                return NULL;
            }
#else
            PyErr_Format(state->ZlibError,
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
        zlib_error(state, self->zst, err, "while creating decompression object");
        Py_DECREF(self);
        return NULL;
    }
}

static void
Dealloc(compobject *self)
{
    PyObject *type = (PyObject *)Py_TYPE(self);
    PyThread_free_lock(self->lock);
    Py_XDECREF(self->unused_data);
    Py_XDECREF(self->unconsumed_tail);
    Py_XDECREF(self->zdict);
    PyObject_Free(self);
    Py_DECREF(type);
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

    cls: defining_class
    data: Py_buffer
        Binary data to be compressed.
    /

Returns a bytes object containing compressed data.

After calling this function, some of the input data may still
be stored in internal buffers for later processing.
Call the flush() method to clear these buffers.
[clinic start generated code]*/

static PyObject *
zlib_Compress_compress_impl(compobject *self, PyTypeObject *cls,
                            Py_buffer *data)
/*[clinic end generated code: output=6731b3f0ff357ca6 input=04d00f65ab01d260]*/
{
    PyObject *RetVal;
    int err;
    _BlocksOutputBuffer buffer = {.list = NULL};
    zlibstate *state = PyType_GetModuleState(cls);

    ENTER_ZLIB(self);

    self->zst.next_in = data->buf;
    Py_ssize_t ibuflen = data->len;

    if (OutputBuffer_InitAndGrow(&buffer, -1, &self->zst.next_out, &self->zst.avail_out) < 0) {
        goto error;
    }

    do {
        arrange_input_buffer(&self->zst, &ibuflen);

        do {
            if (self->zst.avail_out == 0) {
                if (OutputBuffer_Grow(&buffer, &self->zst.next_out, &self->zst.avail_out) < 0) {
                    goto error;
                }
            }

            Py_BEGIN_ALLOW_THREADS
            err = deflate(&self->zst, Z_NO_FLUSH);
            Py_END_ALLOW_THREADS

            if (err == Z_STREAM_ERROR) {
                zlib_error(state, self->zst, err, "while compressing data");
                goto error;
            }

        } while (self->zst.avail_out == 0);
        assert(self->zst.avail_in == 0);

    } while (ibuflen != 0);

    RetVal = OutputBuffer_Finish(&buffer, self->zst.avail_out);
    if (RetVal != NULL) {
        goto success;
    }

 error:
    OutputBuffer_OnError(&buffer);
    RetVal = NULL;
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

    cls: defining_class
    data: Py_buffer
        The binary data to decompress.
    /
    max_length: Py_ssize_t = 0
        The maximum allowable length of the decompressed data.
        Unconsumed input data will be stored in
        the unconsumed_tail attribute.

Return a bytes object containing the decompressed version of the data.

After calling this function, some of the input data may still be stored in
internal buffers for later processing.
Call the flush() method to clear these buffers.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, PyTypeObject *cls,
                                Py_buffer *data, Py_ssize_t max_length)
/*[clinic end generated code: output=b024a93c2c922d57 input=bfb37b3864cfb606]*/
{
    int err = Z_OK;
    Py_ssize_t ibuflen;
    PyObject *RetVal;
    _BlocksOutputBuffer buffer = {.list = NULL};

    PyObject *module = PyType_GetModule(cls);
    if (module == NULL)
        return NULL;

    zlibstate *state = get_zlib_state(module);
    if (max_length < 0) {
        PyErr_SetString(PyExc_ValueError, "max_length must be non-negative");
        return NULL;
    } else if (max_length == 0) {
        max_length = -1;
    }

    ENTER_ZLIB(self);

    self->zst.next_in = data->buf;
    ibuflen = data->len;

    if (OutputBuffer_InitAndGrow(&buffer, max_length, &self->zst.next_out, &self->zst.avail_out) < 0) {
        goto abort;
    }

    do {
        arrange_input_buffer(&self->zst, &ibuflen);

        do {
            if (self->zst.avail_out == 0) {
                if (OutputBuffer_GetDataSize(&buffer, self->zst.avail_out) == max_length) {
                    goto save;
                }
                if (OutputBuffer_Grow(&buffer, &self->zst.next_out, &self->zst.avail_out) < 0) {
                    goto abort;
                }
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
                    if (set_inflate_zdict(state, self) < 0) {
                        goto abort;
                    }
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
        zlib_error(state, self->zst, err, "while decompressing data");
        goto abort;
    }

    RetVal = OutputBuffer_Finish(&buffer, self->zst.avail_out);
    if (RetVal != NULL) {
        goto success;
    }

 abort:
    OutputBuffer_OnError(&buffer);
    RetVal = NULL;
 success:
    LEAVE_ZLIB(self);
    return RetVal;
}

/*[clinic input]
zlib.Compress.flush

    cls: defining_class
    mode: int(c_default="Z_FINISH") = zlib.Z_FINISH
        One of the constants Z_SYNC_FLUSH, Z_FULL_FLUSH, Z_FINISH.
        If mode == Z_FINISH, the compressor object can no longer be
        used after calling the flush() method.  Otherwise, more data
        can still be compressed.
    /

Return a bytes object containing any remaining compressed data.
[clinic start generated code]*/

static PyObject *
zlib_Compress_flush_impl(compobject *self, PyTypeObject *cls, int mode)
/*[clinic end generated code: output=c7efd13efd62add2 input=286146e29442eb6c]*/
{
    int err;
    PyObject *RetVal;
    _BlocksOutputBuffer buffer = {.list = NULL};

    zlibstate *state = PyType_GetModuleState(cls);
    /* Flushing with Z_NO_FLUSH is a no-op, so there's no point in
       doing any work at all; just return an empty string. */
    if (mode == Z_NO_FLUSH) {
        return PyBytes_FromStringAndSize(NULL, 0);
    }

    ENTER_ZLIB(self);

    self->zst.avail_in = 0;

    if (OutputBuffer_InitAndGrow(&buffer, -1, &self->zst.next_out, &self->zst.avail_out) < 0) {
        goto error;
    }

    do {
        if (self->zst.avail_out == 0) {
            if (OutputBuffer_Grow(&buffer, &self->zst.next_out, &self->zst.avail_out) < 0) {
                goto error;
            }
        }

        Py_BEGIN_ALLOW_THREADS
        err = deflate(&self->zst, mode);
        Py_END_ALLOW_THREADS

        if (err == Z_STREAM_ERROR) {
            zlib_error(state, self->zst, err, "while flushing");
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
            zlib_error(state, self->zst, err, "while finishing compression");
            goto error;
        }
        else
            self->is_initialised = 0;

        /* We will only get Z_BUF_ERROR if the output buffer was full
           but there wasn't more output when we tried again, so it is
           not an error condition.
        */
    } else if (err != Z_OK && err != Z_BUF_ERROR) {
        zlib_error(state, self->zst, err, "while flushing");
        goto error;
    }

    RetVal = OutputBuffer_Finish(&buffer, self->zst.avail_out);
    if (RetVal != NULL) {
        goto success;
    }

error:
    OutputBuffer_OnError(&buffer);
    RetVal = NULL;
success:
    LEAVE_ZLIB(self);
    return RetVal;
}

#ifdef HAVE_ZLIB_COPY

/*[clinic input]
zlib.Compress.copy

    cls: defining_class

Return a copy of the compression object.
[clinic start generated code]*/

static PyObject *
zlib_Compress_copy_impl(compobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=c4d2cfb4b0d7350b input=235497e482d40986]*/
{
    zlibstate *state = PyType_GetModuleState(cls);

    compobject *retval = newcompobject(state->Comptype);
    if (!retval) return NULL;

    /* Copy the zstream state
     * We use ENTER_ZLIB / LEAVE_ZLIB to make this thread-safe
     */
    ENTER_ZLIB(self);
    int err = deflateCopy(&retval->zst, &self->zst);
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
        zlib_error(state, self->zst, err, "while copying compression object");
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
zlib.Compress.__copy__

    cls: defining_class

[clinic start generated code]*/

static PyObject *
zlib_Compress___copy___impl(compobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=074613db332cb668 input=5c0188367ab0fe64]*/
{
    return zlib_Compress_copy_impl(self, cls);
}

/*[clinic input]
zlib.Compress.__deepcopy__

    cls: defining_class
    memo: object
    /

[clinic start generated code]*/

static PyObject *
zlib_Compress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                PyObject *memo)
/*[clinic end generated code: output=24b3aed785f54033 input=c90347319a514430]*/
{
    return zlib_Compress_copy_impl(self, cls);
}

/*[clinic input]
zlib.Decompress.copy

    cls: defining_class

Return a copy of the decompression object.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_copy_impl(compobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=a7ddc016e1d0a781 input=20ef3aa208282ff2]*/
{
    zlibstate *state = PyType_GetModuleState(cls);

    compobject *retval = newcompobject(state->Decomptype);
    if (!retval) return NULL;

    /* Copy the zstream state
     * We use ENTER_ZLIB / LEAVE_ZLIB to make this thread-safe
     */
    ENTER_ZLIB(self);
    int err = inflateCopy(&retval->zst, &self->zst);
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
        zlib_error(state, self->zst, err, "while copying decompression object");
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
zlib.Decompress.__copy__

    cls: defining_class

[clinic start generated code]*/

static PyObject *
zlib_Decompress___copy___impl(compobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=cf1e6473744f53fa input=cc3143067b622bdf]*/
{
    return zlib_Decompress_copy_impl(self, cls);
}

/*[clinic input]
zlib.Decompress.__deepcopy__

    cls: defining_class
    memo: object
    /

[clinic start generated code]*/

static PyObject *
zlib_Decompress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                  PyObject *memo)
/*[clinic end generated code: output=34f7b719a0c0d51b input=fc13b9c58622544e]*/
{
    return zlib_Decompress_copy_impl(self, cls);
}

#endif

/*[clinic input]
zlib.Decompress.flush

    cls: defining_class
    length: Py_ssize_t(c_default="DEF_BUF_SIZE") = zlib.DEF_BUF_SIZE
        the initial size of the output buffer.
    /

Return a bytes object containing any remaining decompressed data.
[clinic start generated code]*/

static PyObject *
zlib_Decompress_flush_impl(compobject *self, PyTypeObject *cls,
                           Py_ssize_t length)
/*[clinic end generated code: output=4532fc280bd0f8f2 input=42f1f4b75230e2cd]*/
{
    int err, flush;
    Py_buffer data;
    PyObject *RetVal;
    Py_ssize_t ibuflen;
    _BlocksOutputBuffer buffer = {.list = NULL};
    _Uint32Window window;  // output buffer's UINT32_MAX sliding window

    PyObject *module = PyType_GetModule(cls);
    if (module == NULL) {
        return NULL;
    }

    zlibstate *state = get_zlib_state(module);

    if (length <= 0) {
        PyErr_SetString(PyExc_ValueError, "length must be greater than zero");
        return NULL;
    }

    if (PyObject_GetBuffer(self->unconsumed_tail, &data, PyBUF_SIMPLE) == -1) {
        return NULL;
    }

    ENTER_ZLIB(self);

    self->zst.next_in = data.buf;
    ibuflen = data.len;

    if (OutputBuffer_WindowInitWithSize(&buffer, &window, length,
                                        &self->zst.next_out, &self->zst.avail_out) < 0) {
        goto abort;
    }

    do {
        arrange_input_buffer(&self->zst, &ibuflen);
        flush = ibuflen == 0 ? Z_FINISH : Z_NO_FLUSH;

        do {
            if (self->zst.avail_out == 0) {
                if (OutputBuffer_WindowGrow(&buffer, &window,
                                            &self->zst.next_out, &self->zst.avail_out) < 0) {
                    goto abort;
                }
            }

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
                    if (set_inflate_zdict(state, self) < 0) {
                        goto abort;
                    }
                    else
                        break;
                }
                goto save;
            }

        } while (self->zst.avail_out == 0 || err == Z_NEED_DICT);

    } while (err != Z_STREAM_END && ibuflen != 0);

 save:
    if (save_unconsumed_input(self, &data, err) < 0) {
        goto abort;
    }

    /* If at end of stream, clean up any memory allocated by zlib. */
    if (err == Z_STREAM_END) {
        self->eof = 1;
        self->is_initialised = 0;
        err = inflateEnd(&self->zst);
        if (err != Z_OK) {
            zlib_error(state, self->zst, err, "while finishing decompression");
            goto abort;
        }
    }

    RetVal = OutputBuffer_WindowFinish(&buffer, &window, self->zst.avail_out);
    if (RetVal != NULL) {
        goto success;
    }

 abort:
    OutputBuffer_WindowOnError(&buffer, &window);
    RetVal = NULL;
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
    ZLIB_COMPRESS___COPY___METHODDEF
    ZLIB_COMPRESS___DEEPCOPY___METHODDEF
    {NULL, NULL}
};

static PyMethodDef Decomp_methods[] =
{
    ZLIB_DECOMPRESS_DECOMPRESS_METHODDEF
    ZLIB_DECOMPRESS_FLUSH_METHODDEF
    ZLIB_DECOMPRESS_COPY_METHODDEF
    ZLIB_DECOMPRESS___COPY___METHODDEF
    ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF
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

static PyType_Slot Comptype_slots[] = {
    {Py_tp_dealloc, Comp_dealloc},
    {Py_tp_methods, comp_methods},
    {0, 0},
};

static PyType_Spec Comptype_spec = {
    .name = "zlib.Compress",
    .basicsize = sizeof(compobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .slots= Comptype_slots,
};

static PyType_Slot Decomptype_slots[] = {
    {Py_tp_dealloc, Decomp_dealloc},
    {Py_tp_methods, Decomp_methods},
    {Py_tp_members, Decomp_members},
    {0, 0},
};

static PyType_Spec Decomptype_spec = {
    .name = "zlib.Decompress",
    .basicsize = sizeof(compobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .slots = Decomptype_slots,
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
"decompressobj([wbits[, zdict]]) -- Return a decompressor object.\n"
"\n"
"'wbits' is window buffer size and container format.\n"
"Compressor objects support compress() and flush() methods; decompressor\n"
"objects support decompress() and flush().");

static int
zlib_clear(PyObject *mod)
{
    zlibstate *state = get_zlib_state(mod);
    Py_CLEAR(state->Comptype);
    Py_CLEAR(state->Decomptype);
    Py_CLEAR(state->ZlibError);
    return 0;
}

static int
zlib_traverse(PyObject *mod, visitproc visit, void *arg)
{
    zlibstate *state = get_zlib_state(mod);
    Py_VISIT(state->Comptype);
    Py_VISIT(state->Decomptype);
    Py_VISIT(state->ZlibError);
    return 0;
}

static void
zlib_free(void *mod)
{
    zlib_clear((PyObject *)mod);
}

static int
zlib_exec(PyObject *mod)
{
    zlibstate *state = get_zlib_state(mod);

    state->Comptype = (PyTypeObject *)PyType_FromModuleAndSpec(
        mod, &Comptype_spec, NULL);
    if (state->Comptype == NULL) {
        return -1;
    }

    state->Decomptype = (PyTypeObject *)PyType_FromModuleAndSpec(
        mod, &Decomptype_spec, NULL);
    if (state->Decomptype == NULL) {
        return -1;
    }

    state->ZlibError = PyErr_NewException("zlib.error", NULL, NULL);
    if (state->ZlibError == NULL) {
        return -1;
    }

    Py_INCREF(state->ZlibError);
    if (PyModule_AddObject(mod, "error", state->ZlibError) < 0) {
        Py_DECREF(state->ZlibError);
        return -1;
    }

#define ZLIB_ADD_INT_MACRO(c)                           \
    do {                                                \
        if ((PyModule_AddIntConstant(mod, #c, c)) < 0) {  \
            return -1;                                  \
        }                                               \
    } while(0)

    ZLIB_ADD_INT_MACRO(MAX_WBITS);
    ZLIB_ADD_INT_MACRO(DEFLATED);
    ZLIB_ADD_INT_MACRO(DEF_MEM_LEVEL);
    ZLIB_ADD_INT_MACRO(DEF_BUF_SIZE);
    // compression levels
    ZLIB_ADD_INT_MACRO(Z_NO_COMPRESSION);
    ZLIB_ADD_INT_MACRO(Z_BEST_SPEED);
    ZLIB_ADD_INT_MACRO(Z_BEST_COMPRESSION);
    ZLIB_ADD_INT_MACRO(Z_DEFAULT_COMPRESSION);
    // compression strategies
    ZLIB_ADD_INT_MACRO(Z_FILTERED);
    ZLIB_ADD_INT_MACRO(Z_HUFFMAN_ONLY);
#ifdef Z_RLE // 1.2.0.1
    ZLIB_ADD_INT_MACRO(Z_RLE);
#endif
#ifdef Z_FIXED // 1.2.2.2
    ZLIB_ADD_INT_MACRO(Z_FIXED);
#endif
    ZLIB_ADD_INT_MACRO(Z_DEFAULT_STRATEGY);
    // allowed flush values
    ZLIB_ADD_INT_MACRO(Z_NO_FLUSH);
    ZLIB_ADD_INT_MACRO(Z_PARTIAL_FLUSH);
    ZLIB_ADD_INT_MACRO(Z_SYNC_FLUSH);
    ZLIB_ADD_INT_MACRO(Z_FULL_FLUSH);
    ZLIB_ADD_INT_MACRO(Z_FINISH);
#ifdef Z_BLOCK // 1.2.0.5 for inflate, 1.2.3.4 for deflate
    ZLIB_ADD_INT_MACRO(Z_BLOCK);
#endif
#ifdef Z_TREES // 1.2.3.4, only for inflate
    ZLIB_ADD_INT_MACRO(Z_TREES);
#endif
    PyObject *ver = PyUnicode_FromString(ZLIB_VERSION);
    if (ver == NULL) {
        return -1;
    }

    if (PyModule_AddObject(mod, "ZLIB_VERSION", ver) < 0) {
        Py_DECREF(ver);
        return -1;
    }

    ver = PyUnicode_FromString(zlibVersion());
    if (ver == NULL) {
        return -1;
    }

    if (PyModule_AddObject(mod, "ZLIB_RUNTIME_VERSION", ver) < 0) {
        Py_DECREF(ver);
        return -1;
    }

    if (PyModule_AddStringConstant(mod, "__version__", "1.0") < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot zlib_slots[] = {
    {Py_mod_exec, zlib_exec},
    {0, NULL}
};

static struct PyModuleDef zlibmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "zlib",
    .m_doc = zlib_module_documentation,
    .m_size = sizeof(zlibstate),
    .m_methods = zlib_methods,
    .m_slots = zlib_slots,
    .m_traverse = zlib_traverse,
    .m_clear = zlib_clear,
    .m_free = zlib_free,
};

PyMODINIT_FUNC
PyInit_zlib(void)
{
    return PyModuleDef_Init(&zlibmodule);
}
