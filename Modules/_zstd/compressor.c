/* Low level interface to the Zstandard algorthm & the zstd library. */

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
#include "zstddict.h"

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
} ZstdCompressor;

#define ZstdCompressor_CAST(op) ((ZstdCompressor *)op)

#include "clinic/compressor.c.h"

static int
_zstd_set_c_parameters(ZstdCompressor *self, PyObject *level_or_options,
                       const char *arg_name, const char* arg_type)
{
    size_t zstd_ret;
    _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
    if (mod_state == NULL) {
        return -1;
    }

    /* Integer compression level */
    if (PyLong_Check(level_or_options)) {
        int level = PyLong_AsInt(level_or_options);
        if (level == -1 && PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError,
                            "Compression level should be an int value between %d and %d.",
                            ZSTD_minCLevel(), ZSTD_maxCLevel());
            return -1;
        }

        /* Save for generating ZSTD_CDICT */
        self->compression_level = level;

        /* Set compressionLevel to compression context */
        zstd_ret = ZSTD_CCtx_setParameter(self->cctx,
                                          ZSTD_c_compressionLevel,
                                          level);

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            set_zstd_error(mod_state, ERR_SET_C_LEVEL, zstd_ret);
            return -1;
        }
        return 0;
    }

    /* Options dict */
    if (PyDict_Check(level_or_options)) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(level_or_options, &pos, &key, &value)) {
            /* Check key type */
            if (Py_TYPE(key) == mod_state->DParameter_type) {
                PyErr_SetString(PyExc_TypeError,
                                "Key of compression option dict should "
                                "NOT be DecompressionParameter.");
                return -1;
            }

            int key_v = PyLong_AsInt(key);
            if (key_v == -1 && PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                                "Key of options dict should be a CompressionParameter attribute.");
                return -1;
            }

            // TODO(emmatyping): check bounds when there is a value error here for better
            // error message?
            int value_v = PyLong_AsInt(value);
            if (value_v == -1 && PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                                "Value of option dict should be an int.");
                return -1;
            }

            if (key_v == ZSTD_c_compressionLevel) {
                /* Save for generating ZSTD_CDICT */
                self->compression_level = value_v;
            }
            else if (key_v == ZSTD_c_nbWorkers) {
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
            zstd_ret = ZSTD_CCtx_setParameter(self->cctx, key_v, value_v);
            if (ZSTD_isError(zstd_ret)) {
                set_parameter_error(mod_state, 1, key_v, value_v);
                return -1;
            }
        }
        return 0;
    }
    PyErr_Format(PyExc_TypeError, "Invalid type for %s. Expected %s", arg_name, arg_type);
    return -1;
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
    PyObject *level = NULL;
    PyObject *capsule;
    ZSTD_CDict *cdict;

    // TODO(emmatyping): refactor critical section code into a lock_held function
    Py_BEGIN_CRITICAL_SECTION(self);

    /* int level object */
    level = PyLong_FromLong(compressionLevel);
    if (level == NULL) {
        goto error;
    }

    /* Get PyCapsule object from self->c_dicts */
    capsule = PyDict_GetItemWithError(self->c_dicts, level);
    if (capsule == NULL) {
        if (PyErr_Occurred()) {
            goto error;
        }

        /* Create ZSTD_CDict instance */
        char *dict_buffer = PyBytes_AS_STRING(self->dict_content);
        Py_ssize_t dict_len = Py_SIZE(self->dict_content);
        Py_BEGIN_ALLOW_THREADS
        cdict = ZSTD_createCDict(dict_buffer,
                                 dict_len,
                                 compressionLevel);
        Py_END_ALLOW_THREADS

        if (cdict == NULL) {
            _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
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
        if (PyDict_SetItem(self->c_dicts, level, capsule) < 0) {
            Py_DECREF(capsule);
            goto error;
        }
        Py_DECREF(capsule);
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
    Py_END_CRITICAL_SECTION();
    return cdict;
}

static int
_zstd_load_c_dict(ZstdCompressor *self, PyObject *dict)
{

    size_t zstd_ret;
    _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
    if (mod_state == NULL) {
        return -1;
    }
    ZstdDict *zd;
    int type, ret;

    /* Check ZstdDict */
    ret = PyObject_IsInstance(dict, (PyObject*)mod_state->ZstdDict_type);
    if (ret < 0) {
        return -1;
    }
    else if (ret > 0) {
        /* When compressing, use undigested dictionary by default. */
        zd = (ZstdDict*)dict;
        type = DICT_TYPE_UNDIGESTED;
        goto load;
    }

    /* Check (ZstdDict, type) */
    if (PyTuple_CheckExact(dict) && PyTuple_GET_SIZE(dict) == 2) {
        /* Check ZstdDict */
        ret = PyObject_IsInstance(PyTuple_GET_ITEM(dict, 0),
                                  (PyObject*)mod_state->ZstdDict_type);
        if (ret < 0) {
            return -1;
        }
        else if (ret > 0) {
            /* type == -1 may indicate an error. */
            type = PyLong_AsInt(PyTuple_GET_ITEM(dict, 1));
            if (type == DICT_TYPE_DIGESTED ||
                type == DICT_TYPE_UNDIGESTED ||
                type == DICT_TYPE_PREFIX)
            {
                assert(type >= 0);
                zd = (ZstdDict*)PyTuple_GET_ITEM(dict, 0);
                goto load;
            }
        }
    }

    /* Wrong type */
    PyErr_SetString(PyExc_TypeError,
                    "zstd_dict argument should be ZstdDict object.");
    return -1;

load:
    if (type == DICT_TYPE_DIGESTED) {
        /* Get ZSTD_CDict */
        ZSTD_CDict *c_dict = _get_CDict(zd, self->compression_level);
        if (c_dict == NULL) {
            return -1;
        }
        /* Reference a prepared dictionary.
           It overrides some compression context's parameters. */
        Py_BEGIN_CRITICAL_SECTION(self);
        zstd_ret = ZSTD_CCtx_refCDict(self->cctx, c_dict);
        Py_END_CRITICAL_SECTION();
    }
    else if (type == DICT_TYPE_UNDIGESTED) {
        /* Load a dictionary.
           It doesn't override compression context's parameters. */
        Py_BEGIN_CRITICAL_SECTION2(self, zd);
        zstd_ret = ZSTD_CCtx_loadDictionary(
                            self->cctx,
                            PyBytes_AS_STRING(zd->dict_content),
                            Py_SIZE(zd->dict_content));
        Py_END_CRITICAL_SECTION2();
    }
    else if (type == DICT_TYPE_PREFIX) {
        /* Load a prefix */
        Py_BEGIN_CRITICAL_SECTION2(self, zd);
        zstd_ret = ZSTD_CCtx_refPrefix(
                            self->cctx,
                            PyBytes_AS_STRING(zd->dict_content),
                            Py_SIZE(zd->dict_content));
        Py_END_CRITICAL_SECTION2();
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

    /* Compression context */
    self->cctx = ZSTD_createCCtx();
    if (self->cctx == NULL) {
        _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
        if (mod_state != NULL) {
            PyErr_SetString(mod_state->ZstdError,
                            "Unable to create ZSTD_CCtx instance.");
        }
        goto error;
    }

    /* Last mode */
    self->last_mode = ZSTD_e_end;

    if (level != Py_None && options != Py_None) {
        PyErr_SetString(PyExc_RuntimeError, "Only one of level or options should be used.");
        goto error;
    }

    /* Set compressLevel/options to compression context */
    if (level != Py_None) {
        if (_zstd_set_c_parameters(self, level, "level", "int") < 0) {
            goto error;
        }
    }

    if (options != Py_None) {
        if (_zstd_set_c_parameters(self, options, "options", "dict") < 0) {
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

    /* Py_XDECREF the dict after free the compression context */
    Py_CLEAR(self->dict);

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_Del(ob);
    Py_DECREF(tp);
}

static PyObject *
compress_impl(ZstdCompressor *self, Py_buffer *data,
              ZSTD_EndDirective end_directive)
{
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
            _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
            if (mod_state != NULL) {
                set_zstd_error(mod_state, ERR_COMPRESS, zstd_ret);
            }
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

#ifdef Py_DEBUG
static inline int
mt_continue_should_break(ZSTD_inBuffer *in, ZSTD_outBuffer *out)
{
    return in->size == in->pos && out->size != out->pos;
}
#endif

static PyObject *
compress_mt_continue_impl(ZstdCompressor *self, Py_buffer *data)
{
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
            zstd_ret = ZSTD_compressStream2(self->cctx, &out, &in, ZSTD_e_continue);
        } while (out.pos != out.size && in.pos != in.size && !ZSTD_isError(zstd_ret));
        Py_END_ALLOW_THREADS

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
            if (mod_state != NULL) {
                set_zstd_error(mod_state, ERR_COMPRESS, zstd_ret);
            }
            goto error;
        }

        /* Like compress_impl(), output as much as possible. */
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
    Py_BEGIN_CRITICAL_SECTION(self);

    /* Compress */
    if (self->use_multithread && mode == ZSTD_e_continue) {
        ret = compress_mt_continue_impl(self, data);
    }
    else {
        ret = compress_impl(self, data, mode);
    }

    if (ret) {
        self->last_mode = mode;
    }
    else {
        self->last_mode = ZSTD_e_end;

        /* Resetting cctx's session never fail */
        ZSTD_CCtx_reset(self->cctx, ZSTD_reset_session_only);
    }
    Py_END_CRITICAL_SECTION();

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
    Py_BEGIN_CRITICAL_SECTION(self);
    ret = compress_impl(self, NULL, mode);

    if (ret) {
        self->last_mode = mode;
    }
    else {
        self->last_mode = ZSTD_e_end;

        /* Resetting cctx's session never fail */
        ZSTD_CCtx_reset(self->cctx, ZSTD_reset_session_only);
    }
    Py_END_CRITICAL_SECTION();

    return ret;
}

static PyMethodDef ZstdCompressor_methods[] = {
    _ZSTD_ZSTDCOMPRESSOR_COMPRESS_METHODDEF
    _ZSTD_ZSTDCOMPRESSOR_FLUSH_METHODDEF
    {NULL, NULL}
};

PyDoc_STRVAR(ZstdCompressor_last_mode_doc,
"The last mode used to this compressor object, its value can be .CONTINUE,\n"
".FLUSH_BLOCK, .FLUSH_FRAME. Initialized to .FLUSH_FRAME.\n\n"
"It can be used to get the current state of a compressor, such as, data flushed,\n"
"a frame ended.");

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
