/* Low level interface to the Zstandard algorithm & the zstd library. */

/* ZstdCompressor class definitions */

/*[clinic input]
module _zstd
class _zstd.ZstdCompressor "ZstdCompressor *" "&zstd_compressor_type_spec"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7166021db1ef7df8]*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include "_zstdmodule.h"
#include "buffer.h"
#include "internal/pycore_lock.h" // PyMutex_IsLocked

#include <stddef.h>               // offsetof()
#include <zstd.h>                 // ZSTD_*()

typedef struct {
    PyObject_HEAD

    /* Compression context */
    ZSTD_CCtx *cctx;

    /* ZstdDict object in use */
    PyObject *dict;

    /* Last mode, initialized to ZSTD_e_end */
    int last_mode;

    /* (nbWorker >= 1) ? 1 : 0 */
    int use_multithread;

    /* Compression level */
    int compression_level;

    /* Lock to protect the compression context */
    PyMutex lock;
} ZstdCompressor;

#define ZstdCompressor_CAST(op) ((ZstdCompressor *)op)

/*[python input]

class zstd_contentsize_converter(CConverter):
    type = 'unsigned long long'
    converter = 'zstd_contentsize_converter'

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=0932c350d633c7de]*/


static int
zstd_contentsize_converter(PyObject *size, unsigned long long *p)
{
    // None means the user indicates the size is unknown.
    if (size == Py_None) {
        *p = ZSTD_CONTENTSIZE_UNKNOWN;
    }
    else {
        /* ZSTD_CONTENTSIZE_UNKNOWN is 0ULL - 1
           ZSTD_CONTENTSIZE_ERROR   is 0ULL - 2
           Users should only pass values < ZSTD_CONTENTSIZE_ERROR */
        unsigned long long pledged_size = PyLong_AsUnsignedLongLong(size);
        /* Here we check for (unsigned long long)-1 as a sign of an error in
           PyLong_AsUnsignedLongLong */
        if (pledged_size == (unsigned long long)-1 && PyErr_Occurred()) {
            *p = ZSTD_CONTENTSIZE_ERROR;
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                PyErr_Format(PyExc_ValueError,
                             "size argument should be a positive int less "
                             "than %ull", ZSTD_CONTENTSIZE_ERROR);
                return 0;
            }
            return 0;
        }
        if (pledged_size >= ZSTD_CONTENTSIZE_ERROR) {
            *p = ZSTD_CONTENTSIZE_ERROR;
            PyErr_Format(PyExc_ValueError,
                         "size argument should be a positive int less "
                         "than %ull", ZSTD_CONTENTSIZE_ERROR);
            return 0;
        }
        *p = pledged_size;
    }
    return 1;
}

#include "clinic/compressor.c.h"

static int
_zstd_set_c_level(ZstdCompressor *self, int level)
{
    /* Set integer compression level */
    int min_level = ZSTD_minCLevel();
    int max_level = ZSTD_maxCLevel();
    if (level < min_level || level > max_level) {
        PyErr_Format(PyExc_ValueError,
        "illegal compression level %d; the valid range is [%d, %d]",
            level, min_level, max_level);
        return -1;
    }

    /* Save for generating ZSTD_CDICT */
    self->compression_level = level;

    /* Set compressionLevel to compression context */
    size_t zstd_ret = ZSTD_CCtx_setParameter(
        self->cctx, ZSTD_c_compressionLevel, level);

    /* Check error */
    if (ZSTD_isError(zstd_ret)) {
        _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
        set_zstd_error(mod_state, ERR_SET_C_LEVEL, zstd_ret);
        return -1;
    }
    return 0;
}

static int
_zstd_set_c_parameters(ZstdCompressor *self, PyObject *options)
{
    _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
    if (mod_state == NULL) {
        return -1;
    }

    if (!PyDict_Check(options)) {
        PyErr_Format(PyExc_TypeError,
             "ZstdCompressor() argument 'options' must be dict, not %T",
             options);
        return -1;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(options, &pos, &key, &value)) {
        /* Check key type */
        if (Py_TYPE(key) == mod_state->DParameter_type) {
            PyErr_SetString(PyExc_TypeError,
                "compression options dictionary key must not be a "
                "DecompressionParameter attribute");
            return -1;
        }

        Py_INCREF(key);
        Py_INCREF(value);
        int key_v = PyLong_AsInt(key);
        Py_DECREF(key);
        if (key_v == -1 && PyErr_Occurred()) {
            Py_DECREF(value);
            return -1;
        }

        int value_v = PyLong_AsInt(value);
        Py_DECREF(value);
        if (value_v == -1 && PyErr_Occurred()) {
            return -1;
        }

        if (key_v == ZSTD_c_compressionLevel) {
            if (_zstd_set_c_level(self, value_v) < 0) {
                return -1;
            }
            continue;
        }
        if (key_v == ZSTD_c_nbWorkers) {
            /* From the zstd library docs:
               1. When nbWorkers >= 1, triggers asynchronous mode when
                  used with ZSTD_compressStream2().
               2, Default value is `0`, aka "single-threaded mode" : no
                  worker is spawned, compression is performed inside
                  caller's thread, all invocations are blocking. */
            if (value_v != 0) {
                self->use_multithread = 1;
            }
        }

        /* Set parameter to compression context */
        size_t zstd_ret = ZSTD_CCtx_setParameter(self->cctx, key_v, value_v);

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            set_parameter_error(1, key_v, value_v);
            return -1;
        }
    }
    return 0;
}

static void
capsule_free_cdict(PyObject *capsule)
{
    ZSTD_CDict *cdict = PyCapsule_GetPointer(capsule, NULL);
    ZSTD_freeCDict(cdict);
}

ZSTD_CDict *
_get_CDict(ZstdDict *self, int compressionLevel)
{
    assert(PyMutex_IsLocked(&self->lock));
    PyObject *level = NULL;
    PyObject *capsule = NULL;
    ZSTD_CDict *cdict;
    int ret;


    /* int level object */
    level = PyLong_FromLong(compressionLevel);
    if (level == NULL) {
        goto error;
    }

    /* Get PyCapsule object from self->c_dicts */
    ret = PyDict_GetItemRef(self->c_dicts, level, &capsule);
    if (ret < 0) {
        goto error;
    }
    if (capsule == NULL) {
        /* Create ZSTD_CDict instance */
        Py_BEGIN_ALLOW_THREADS
        cdict = ZSTD_createCDict(self->dict_buffer, self->dict_len,
                                 compressionLevel);
        Py_END_ALLOW_THREADS

        if (cdict == NULL) {
            _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
            if (mod_state != NULL) {
                PyErr_SetString(mod_state->ZstdError,
                    "Failed to create a ZSTD_CDict instance from "
                    "Zstandard dictionary content.");
            }
            goto error;
        }

        /* Put ZSTD_CDict instance into PyCapsule object */
        capsule = PyCapsule_New(cdict, NULL, capsule_free_cdict);
        if (capsule == NULL) {
            ZSTD_freeCDict(cdict);
            goto error;
        }

        /* Add PyCapsule object to self->c_dicts */
        ret = PyDict_SetItem(self->c_dicts, level, capsule);
        if (ret < 0) {
            goto error;
        }
    }
    else {
        /* ZSTD_CDict instance already exists */
        cdict = PyCapsule_GetPointer(capsule, NULL);
    }
    goto success;

error:
    cdict = NULL;
success:
    Py_XDECREF(level);
    Py_XDECREF(capsule);
    return cdict;
}

static int
_zstd_load_impl(ZstdCompressor *self, ZstdDict *zd,
                _zstd_state *mod_state, int type)
{
    size_t zstd_ret;
    if (type == DICT_TYPE_DIGESTED) {
        /* Get ZSTD_CDict */
        ZSTD_CDict *c_dict = _get_CDict(zd, self->compression_level);
        if (c_dict == NULL) {
            return -1;
        }
        /* Reference a prepared dictionary.
           It overrides some compression context's parameters. */
        zstd_ret = ZSTD_CCtx_refCDict(self->cctx, c_dict);
    }
    else if (type == DICT_TYPE_UNDIGESTED) {
        /* Load a dictionary.
           It doesn't override compression context's parameters. */
        zstd_ret = ZSTD_CCtx_loadDictionary(self->cctx, zd->dict_buffer,
                                            zd->dict_len);
    }
    else if (type == DICT_TYPE_PREFIX) {
        /* Load a prefix */
        zstd_ret = ZSTD_CCtx_refPrefix(self->cctx, zd->dict_buffer,
                                       zd->dict_len);
    }
    else {
        Py_UNREACHABLE();
    }

    /* Check error */
    if (ZSTD_isError(zstd_ret)) {
        set_zstd_error(mod_state, ERR_LOAD_C_DICT, zstd_ret);
        return -1;
    }
    return 0;
}

static int
_zstd_load_c_dict(ZstdCompressor *self, PyObject *dict)
{
    _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
    /* When compressing, use undigested dictionary by default. */
    int type = DICT_TYPE_UNDIGESTED;
    ZstdDict *zd = _Py_parse_zstd_dict(mod_state, dict, &type);
    if (zd == NULL) {
        return -1;
    }
    int ret;
    PyMutex_Lock(&zd->lock);
    ret = _zstd_load_impl(self, zd, mod_state, type);
    PyMutex_Unlock(&zd->lock);
    return ret;
}

/*[clinic input]
@classmethod
_zstd.ZstdCompressor.__new__ as _zstd_ZstdCompressor_new
    level: object = None
        The compression level to use. Defaults to COMPRESSION_LEVEL_DEFAULT.
    options: object = None
        A dict object that contains advanced compression parameters.
    zstd_dict: object = None
        A ZstdDict object, a pre-trained Zstandard dictionary.

Create a compressor object for compressing data incrementally.

Thread-safe at method level. For one-shot compression, use the compress()
function instead.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdCompressor_new_impl(PyTypeObject *type, PyObject *level,
                              PyObject *options, PyObject *zstd_dict)
/*[clinic end generated code: output=cdef61eafecac3d7 input=92de0211ae20ffdc]*/
{
    ZstdCompressor* self = PyObject_GC_New(ZstdCompressor, type);
    if (self == NULL) {
        goto error;
    }

    self->use_multithread = 0;
    self->dict = NULL;
    self->lock = (PyMutex){0};

    /* Compression context */
    self->cctx = ZSTD_createCCtx();
    if (self->cctx == NULL) {
        _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
        if (mod_state != NULL) {
            PyErr_SetString(mod_state->ZstdError,
                            "Unable to create ZSTD_CCtx instance.");
        }
        goto error;
    }

    /* Last mode */
    self->last_mode = ZSTD_e_end;

    if (level != Py_None && options != Py_None) {
        PyErr_SetString(PyExc_TypeError,
                        "Only one of level or options should be used.");
        goto error;
    }

    /* Set compression level */
    if (level != Py_None) {
        if (!PyLong_Check(level)) {
            PyErr_SetString(PyExc_TypeError,
                "invalid type for level, expected int");
            goto error;
        }
        int level_v = PyLong_AsInt(level);
        if (level_v == -1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                PyErr_Format(PyExc_ValueError,
                    "illegal compression level; the valid range is [%d, %d]",
                    ZSTD_minCLevel(), ZSTD_maxCLevel());
            }
            goto error;
        }
        if (_zstd_set_c_level(self, level_v) < 0) {
            goto error;
        }
    }

    /* Set options dictionary */
    if (options != Py_None) {
        if (_zstd_set_c_parameters(self, options) < 0) {
            goto error;
        }
    }

    /* Load Zstandard dictionary to compression context */
    if (zstd_dict != Py_None) {
        if (_zstd_load_c_dict(self, zstd_dict) < 0) {
            goto error;
        }
        Py_INCREF(zstd_dict);
        self->dict = zstd_dict;
    }

    // We can only start GC tracking once self->dict is set.
    PyObject_GC_Track(self);

    return (PyObject*)self;

error:
    Py_XDECREF(self);
    return NULL;
}

static void
ZstdCompressor_dealloc(PyObject *ob)
{
    ZstdCompressor *self = ZstdCompressor_CAST(ob);

    PyObject_GC_UnTrack(self);

    /* Free compression context */
    if (self->cctx) {
        ZSTD_freeCCtx(self->cctx);
    }

    assert(!PyMutex_IsLocked(&self->lock));

    /* Py_XDECREF the dict after free the compression context */
    Py_CLEAR(self->dict);

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_Del(ob);
    Py_DECREF(tp);
}

static PyObject *
compress_lock_held(ZstdCompressor *self, Py_buffer *data,
                   ZSTD_EndDirective end_directive)
{
    assert(PyMutex_IsLocked(&self->lock));
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer = {.list = NULL};
    size_t zstd_ret;
    PyObject *ret;

    /* Prepare input & output buffers */
    if (data != NULL) {
        in.src = data->buf;
        in.size = data->len;
        in.pos = 0;
    }
    else {
        in.src = &in;
        in.size = 0;
        in.pos = 0;
    }

    /* Calculate output buffer's size */
    size_t output_buffer_size = ZSTD_compressBound(in.size);
    if (output_buffer_size > (size_t) PY_SSIZE_T_MAX) {
        PyErr_NoMemory();
        goto error;
    }

    if (_OutputBuffer_InitWithSize(&buffer, &out, -1,
                                   (Py_ssize_t) output_buffer_size) < 0) {
        goto error;
    }


    /* Zstandard stream compress */
    while (1) {
        Py_BEGIN_ALLOW_THREADS
        zstd_ret = ZSTD_compressStream2(self->cctx, &out, &in, end_directive);
        Py_END_ALLOW_THREADS

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
            set_zstd_error(mod_state, ERR_COMPRESS, zstd_ret);
            goto error;
        }

        /* Finished */
        if (zstd_ret == 0) {
            break;
        }

        /* Output buffer should be exhausted, grow the buffer. */
        assert(out.pos == out.size);
        if (out.pos == out.size) {
            if (_OutputBuffer_Grow(&buffer, &out) < 0) {
                goto error;
            }
        }
    }

    /* Return a bytes object */
    ret = _OutputBuffer_Finish(&buffer, &out);
    if (ret != NULL) {
        return ret;
    }

error:
    _OutputBuffer_OnError(&buffer);
    return NULL;
}

#ifndef NDEBUG
static inline int
mt_continue_should_break(ZSTD_inBuffer *in, ZSTD_outBuffer *out)
{
    return in->size == in->pos && out->size != out->pos;
}
#endif

static PyObject *
compress_mt_continue_lock_held(ZstdCompressor *self, Py_buffer *data)
{
    assert(PyMutex_IsLocked(&self->lock));
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer = {.list = NULL};
    size_t zstd_ret;
    PyObject *ret;

    /* Prepare input & output buffers */
    in.src = data->buf;
    in.size = data->len;
    in.pos = 0;

    if (_OutputBuffer_InitAndGrow(&buffer, &out, -1) < 0) {
        goto error;
    }

    /* Zstandard stream compress */
    while (1) {
        Py_BEGIN_ALLOW_THREADS
        do {
            zstd_ret = ZSTD_compressStream2(self->cctx, &out, &in,
                                            ZSTD_e_continue);
        } while (out.pos != out.size
                 && in.pos != in.size
                 && !ZSTD_isError(zstd_ret));
        Py_END_ALLOW_THREADS

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
            set_zstd_error(mod_state, ERR_COMPRESS, zstd_ret);
            goto error;
        }

        /* Like compress_lock_held(), output as much as possible. */
        if (out.pos == out.size) {
            if (_OutputBuffer_Grow(&buffer, &out) < 0) {
                goto error;
            }
        }
        else if (in.pos == in.size) {
            /* Finished */
            assert(mt_continue_should_break(&in, &out));
            break;
        }
    }

    /* Return a bytes object */
    ret = _OutputBuffer_Finish(&buffer, &out);
    if (ret != NULL) {
        return ret;
    }

error:
    _OutputBuffer_OnError(&buffer);
    return NULL;
}

/*[clinic input]
_zstd.ZstdCompressor.compress

    data: Py_buffer
    mode: int(c_default="ZSTD_e_continue") = ZstdCompressor.CONTINUE
        Can be these 3 values ZstdCompressor.CONTINUE,
        ZstdCompressor.FLUSH_BLOCK, ZstdCompressor.FLUSH_FRAME

Provide data to the compressor object.

Return a chunk of compressed data if possible, or b'' otherwise. When you have
finished providing data to the compressor, call the flush() method to finish
the compression process.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdCompressor_compress_impl(ZstdCompressor *self, Py_buffer *data,
                                   int mode)
/*[clinic end generated code: output=ed7982d1cf7b4f98 input=ac2c21d180f579ea]*/
{
    PyObject *ret;

    /* Check mode value */
    if (mode != ZSTD_e_continue &&
        mode != ZSTD_e_flush &&
        mode != ZSTD_e_end)
    {
        PyErr_SetString(PyExc_ValueError,
                        "mode argument wrong value, it should be one of "
                        "ZstdCompressor.CONTINUE, ZstdCompressor.FLUSH_BLOCK, "
                        "ZstdCompressor.FLUSH_FRAME.");
        return NULL;
    }

    /* Thread-safe code */
    PyMutex_Lock(&self->lock);

    /* Compress */
    if (self->use_multithread && mode == ZSTD_e_continue) {
        ret = compress_mt_continue_lock_held(self, data);
    }
    else {
        ret = compress_lock_held(self, data, mode);
    }

    if (ret) {
        self->last_mode = mode;
    }
    else {
        self->last_mode = ZSTD_e_end;

        /* Resetting cctx's session never fail */
        ZSTD_CCtx_reset(self->cctx, ZSTD_reset_session_only);
    }
    PyMutex_Unlock(&self->lock);

    return ret;
}

/*[clinic input]
_zstd.ZstdCompressor.flush

    mode: int(c_default="ZSTD_e_end") = ZstdCompressor.FLUSH_FRAME
        Can be these 2 values ZstdCompressor.FLUSH_FRAME,
        ZstdCompressor.FLUSH_BLOCK

Finish the compression process.

Flush any remaining data left in internal buffers. Since Zstandard data
consists of one or more independent frames, the compressor object can still
be used after this method is called.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdCompressor_flush_impl(ZstdCompressor *self, int mode)
/*[clinic end generated code: output=b7cf2c8d64dcf2e3 input=0ab19627f323cdbc]*/
{
    PyObject *ret;

    /* Check mode value */
    if (mode != ZSTD_e_end && mode != ZSTD_e_flush) {
        PyErr_SetString(PyExc_ValueError,
                        "mode argument wrong value, it should be "
                        "ZstdCompressor.FLUSH_FRAME or "
                        "ZstdCompressor.FLUSH_BLOCK.");
        return NULL;
    }

    /* Thread-safe code */
    PyMutex_Lock(&self->lock);

    ret = compress_lock_held(self, NULL, mode);

    if (ret) {
        self->last_mode = mode;
    }
    else {
        self->last_mode = ZSTD_e_end;

        /* Resetting cctx's session never fail */
        ZSTD_CCtx_reset(self->cctx, ZSTD_reset_session_only);
    }
    PyMutex_Unlock(&self->lock);

    return ret;
}


/*[clinic input]
_zstd.ZstdCompressor.set_pledged_input_size

    size: zstd_contentsize
        The size of the uncompressed data to be provided to the compressor.
    /

Set the uncompressed content size to be written into the frame header.

This method can be used to ensure the header of the frame about to be written
includes the size of the data, unless the CompressionParameter.content_size_flag
is set to False. If last_mode != FLUSH_FRAME, then a RuntimeError is raised.

It is important to ensure that the pledged data size matches the actual data
size. If they do not match the compressed output data may be corrupted and the
final chunk written may be lost.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdCompressor_set_pledged_input_size_impl(ZstdCompressor *self,
                                                 unsigned long long size)
/*[clinic end generated code: output=3a09e55cc0e3b4f9 input=afd8a7d78cff2eb5]*/
{
    // Error occured while converting argument, should be unreachable
    assert(size != ZSTD_CONTENTSIZE_ERROR);

    /* Thread-safe code */
    PyMutex_Lock(&self->lock);

    /* Check the current mode */
    if (self->last_mode != ZSTD_e_end) {
        PyErr_SetString(PyExc_ValueError,
                        "set_pledged_input_size() method must be called "
                        "when last_mode == FLUSH_FRAME");
        PyMutex_Unlock(&self->lock);
        return NULL;
    }

    /* Set pledged content size */
    size_t zstd_ret = ZSTD_CCtx_setPledgedSrcSize(self->cctx, size);
    PyMutex_Unlock(&self->lock);
    if (ZSTD_isError(zstd_ret)) {
        _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
        set_zstd_error(mod_state, ERR_SET_PLEDGED_INPUT_SIZE, zstd_ret);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef ZstdCompressor_methods[] = {
    _ZSTD_ZSTDCOMPRESSOR_COMPRESS_METHODDEF
    _ZSTD_ZSTDCOMPRESSOR_FLUSH_METHODDEF
    _ZSTD_ZSTDCOMPRESSOR_SET_PLEDGED_INPUT_SIZE_METHODDEF
    {NULL, NULL}
};

PyDoc_STRVAR(ZstdCompressor_last_mode_doc,
"The last mode used to this compressor object, its value can be .CONTINUE,\n"
".FLUSH_BLOCK, .FLUSH_FRAME. Initialized to .FLUSH_FRAME.\n\n"
"It can be used to get the current state of a compressor, such as, data\n"
"flushed, or a frame ended.");

static PyMemberDef ZstdCompressor_members[] = {
    {"last_mode", Py_T_INT, offsetof(ZstdCompressor, last_mode),
     Py_READONLY, ZstdCompressor_last_mode_doc},
    {NULL}
};

static int
ZstdCompressor_traverse(PyObject *ob, visitproc visit, void *arg)
{
    ZstdCompressor *self = ZstdCompressor_CAST(ob);
    Py_VISIT(self->dict);
    return 0;
}

static int
ZstdCompressor_clear(PyObject *ob)
{
    ZstdCompressor *self = ZstdCompressor_CAST(ob);
    Py_CLEAR(self->dict);
    return 0;
}

static PyType_Slot zstdcompressor_slots[] = {
    {Py_tp_new, _zstd_ZstdCompressor_new},
    {Py_tp_dealloc, ZstdCompressor_dealloc},
    {Py_tp_methods, ZstdCompressor_methods},
    {Py_tp_members, ZstdCompressor_members},
    {Py_tp_doc, (void *)_zstd_ZstdCompressor_new__doc__},
    {Py_tp_traverse,  ZstdCompressor_traverse},
    {Py_tp_clear, ZstdCompressor_clear},
    {0, 0}
};

PyType_Spec zstd_compressor_type_spec = {
    .name = "compression.zstd.ZstdCompressor",
    .basicsize = sizeof(ZstdCompressor),
    // Py_TPFLAGS_IMMUTABLETYPE is not used here as several
    // associated constants need to be added to the type.
    // PyType_Freeze is called later to set the flag.
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = zstdcompressor_slots,
};
