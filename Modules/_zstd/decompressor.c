/* Low level interface to the Zstandard algorithm & the zstd library. */

/* ZstdDecompressor class definition */

/*[clinic input]
module _zstd
class _zstd.ZstdDecompressor "ZstdDecompressor *" "&zstd_decompressor_type_spec"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e2969ddf48a203e0]*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include "_zstdmodule.h"
#include "buffer.h"
#include "internal/pycore_lock.h" // PyMutex_IsLocked

#include <stdbool.h>              // bool
#include <stddef.h>               // offsetof()
#include <zstd.h>                 // ZSTD_*()

typedef struct {
    PyObject_HEAD

    /* Decompression context */
    ZSTD_DCtx *dctx;

    /* ZstdDict object in use */
    PyObject *dict;

    /* Unconsumed input data */
    char *input_buffer;
    size_t input_buffer_size;
    size_t in_begin, in_end;

    /* Unused data */
    PyObject *unused_data;

    /* 0 if decompressor has (or may has) unconsumed input data, 0 or 1. */
    bool needs_input;

    /* For ZstdDecompressor, 0 or 1.
       1 means the end of the first frame has been reached. */
    bool eof;

    /* Lock to protect the decompression context */
    PyMutex lock;
} ZstdDecompressor;

#define ZstdDecompressor_CAST(op) ((ZstdDecompressor *)op)

#include "clinic/decompressor.c.h"

static inline ZSTD_DDict *
_get_DDict(ZstdDict *self)
{
    assert(PyMutex_IsLocked(&self->lock));
    ZSTD_DDict *ret;

    if (self->d_dict == NULL) {
        /* Create ZSTD_DDict instance from dictionary content */
        Py_BEGIN_ALLOW_THREADS
        ret = ZSTD_createDDict(self->dict_buffer, self->dict_len);
        Py_END_ALLOW_THREADS
        self->d_dict = ret;

        if (self->d_dict == NULL) {
            _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
            if (mod_state != NULL) {
                PyErr_SetString(mod_state->ZstdError,
                                "Failed to create a ZSTD_DDict instance from "
                                "Zstandard dictionary content.");
            }
        }
    }

    return self->d_dict;
}

static int
_zstd_set_d_parameters(ZstdDecompressor *self, PyObject *options)
{
    _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
    if (mod_state == NULL) {
        return -1;
    }

    if (!PyDict_Check(options)) {
        PyErr_Format(PyExc_TypeError,
             "ZstdDecompressor() argument 'options' must be dict, not %T",
             options);
        return -1;
    }

    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(options, &pos, &key, &value)) {
        /* Check key type */
        if (Py_TYPE(key) == mod_state->CParameter_type) {
            PyErr_SetString(PyExc_TypeError,
                "compression options dictionary key must not be a "
                "CompressionParameter attribute");
            return -1;
        }

        Py_INCREF(key);
        Py_INCREF(value);
        int key_v = PyLong_AsInt(key);
        Py_DECREF(key);
        if (key_v == -1 && PyErr_Occurred()) {
            return -1;
        }

        int value_v = PyLong_AsInt(value);
        Py_DECREF(value);
        if (value_v == -1 && PyErr_Occurred()) {
            return -1;
        }

        /* Set parameter to compression context */
        size_t zstd_ret = ZSTD_DCtx_setParameter(self->dctx, key_v, value_v);

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            set_parameter_error(0, key_v, value_v);
            return -1;
        }
    }
    return 0;
}

static int
_zstd_load_impl(ZstdDecompressor *self, ZstdDict *zd,
                _zstd_state *mod_state, int type)
{
    size_t zstd_ret;
    if (type == DICT_TYPE_DIGESTED) {
        /* Get ZSTD_DDict */
        ZSTD_DDict *d_dict = _get_DDict(zd);
        if (d_dict == NULL) {
            return -1;
        }
        /* Reference a prepared dictionary */
        zstd_ret = ZSTD_DCtx_refDDict(self->dctx, d_dict);
    }
    else if (type == DICT_TYPE_UNDIGESTED) {
        /* Load a dictionary */
        zstd_ret = ZSTD_DCtx_loadDictionary(self->dctx, zd->dict_buffer,
                                            zd->dict_len);
    }
    else if (type == DICT_TYPE_PREFIX) {
        /* Load a prefix */
        zstd_ret = ZSTD_DCtx_refPrefix(self->dctx, zd->dict_buffer,
                                       zd->dict_len);
    }
    else {
        Py_UNREACHABLE();
    }

    /* Check error */
    if (ZSTD_isError(zstd_ret)) {
        set_zstd_error(mod_state, ERR_LOAD_D_DICT, zstd_ret);
        return -1;
    }
    return 0;
}

/* Load dictionary or prefix to decompression context */
static int
_zstd_load_d_dict(ZstdDecompressor *self, PyObject *dict)
{
    _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
    /* When decompressing, use digested dictionary by default. */
    int type = DICT_TYPE_DIGESTED;
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

/*
    Decompress implementation in pseudo code:

        initialize_output_buffer
        while True:
            decompress_data
            set_object_flag   # .eof

            if output_buffer_exhausted:
                if output_buffer_reached_max_length:
                    finish
                grow_output_buffer
            elif input_buffer_exhausted:
                finish

    ZSTD_decompressStream()'s size_t return value:
      - 0 when a frame is completely decoded and fully flushed,
        zstd's internal buffer has no data.
      - An error code, which can be tested using ZSTD_isError().
      - Or any other value > 0, which means there is still some decoding or
        flushing to do to complete current frame.

      Note, decompressing "an empty input" in any case will make it > 0.
*/
static PyObject *
decompress_lock_held(ZstdDecompressor *self, ZSTD_inBuffer *in,
                     Py_ssize_t max_length)
{
    size_t zstd_ret;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer = {.writer = NULL};
    PyObject *ret;

    /* Initialize the output buffer */
    if (_OutputBuffer_InitAndGrow(&buffer, &out, max_length) < 0) {
        goto error;
    }
    assert(out.pos == 0);

    while (1) {
        /* Decompress */
        Py_BEGIN_ALLOW_THREADS
        zstd_ret = ZSTD_decompressStream(self->dctx, &out, in);
        Py_END_ALLOW_THREADS

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
            set_zstd_error(mod_state, ERR_DECOMPRESS, zstd_ret);
            goto error;
        }

        /* Set .eof flag */
        if (zstd_ret == 0) {
            /* Stop when a frame is decompressed */
            self->eof = 1;
            break;
        }

        /* Need to check out before in. Maybe zstd's internal buffer still has
           a few bytes that can be output, grow the buffer and continue. */
        if (out.pos == out.size) {
            /* Output buffer exhausted */

            /* Output buffer reached max_length */
            if (_OutputBuffer_ReachedMaxLength(&buffer, &out)) {
                break;
            }

            /* Grow output buffer */
            if (_OutputBuffer_Grow(&buffer, &out) < 0) {
                goto error;
            }
            assert(out.pos == 0);

        }
        else if (in->pos == in->size) {
            /* Finished */
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

static void
decompressor_reset_session_lock_held(ZstdDecompressor *self)
{
    assert(PyMutex_IsLocked(&self->lock));

    /* Reset variables */
    self->in_begin = 0;
    self->in_end = 0;

    Py_CLEAR(self->unused_data);

    /* Reset variables in one operation */
    self->needs_input = 1;
    self->eof = 0;

    /* Resetting session is guaranteed to never fail */
    ZSTD_DCtx_reset(self->dctx, ZSTD_reset_session_only);
}

static PyObject *
stream_decompress_lock_held(ZstdDecompressor *self, Py_buffer *data,
                            Py_ssize_t max_length)
{
    assert(PyMutex_IsLocked(&self->lock));
    ZSTD_inBuffer in;
    PyObject *ret = NULL;
    int use_input_buffer;

    /* Check .eof flag */
    if (self->eof) {
        PyErr_SetString(PyExc_EOFError,
                        "Already at the end of a Zstandard frame.");
        assert(ret == NULL);
        return NULL;
    }

    /* Prepare input buffer w/wo unconsumed data */
    if (self->in_begin == self->in_end) {
        /* No unconsumed data */
        use_input_buffer = 0;

        in.src = data->buf;
        in.size = data->len;
        in.pos = 0;
    }
    else if (data->len == 0) {
        /* Has unconsumed data, fast path for b'' */
        assert(self->in_begin < self->in_end);

        use_input_buffer = 1;

        in.src = self->input_buffer + self->in_begin;
        in.size = self->in_end - self->in_begin;
        in.pos = 0;
    }
    else {
        /* Has unconsumed data */
        use_input_buffer = 1;

        /* Unconsumed data size in input_buffer */
        size_t used_now = self->in_end - self->in_begin;
        assert(self->in_end > self->in_begin);

        /* Number of bytes we can append to input buffer */
        size_t avail_now = self->input_buffer_size - self->in_end;
        assert(self->input_buffer_size >= self->in_end);

        /* Number of bytes we can append if we move existing contents to
           beginning of buffer */
        size_t avail_total = self->input_buffer_size - used_now;
        assert(self->input_buffer_size >= used_now);

        if (avail_total < (size_t) data->len) {
            char *tmp;
            size_t new_size = used_now + data->len;

            /* Allocate with new size */
            tmp = PyMem_Malloc(new_size);
            if (tmp == NULL) {
                PyErr_NoMemory();
                goto error;
            }

            /* Copy unconsumed data to the beginning of new buffer */
            memcpy(tmp,
                   self->input_buffer + self->in_begin,
                   used_now);

            /* Switch to new buffer */
            PyMem_Free(self->input_buffer);
            self->input_buffer = tmp;
            self->input_buffer_size = new_size;

            /* Set begin & end position */
            self->in_begin = 0;
            self->in_end = used_now;
        }
        else if (avail_now < (size_t) data->len) {
            /* Move unconsumed data to the beginning.
               Overlap is possible, so use memmove(). */
            memmove(self->input_buffer,
                    self->input_buffer + self->in_begin,
                    used_now);

            /* Set begin & end position */
            self->in_begin = 0;
            self->in_end = used_now;
        }

        /* Copy data to input buffer */
        memcpy(self->input_buffer + self->in_end, data->buf, data->len);
        self->in_end += data->len;

        in.src = self->input_buffer + self->in_begin;
        in.size = used_now + data->len;
        in.pos = 0;
    }
    assert(in.pos == 0);

    /* Decompress */
    ret = decompress_lock_held(self, &in, max_length);
    if (ret == NULL) {
        goto error;
    }

    /* Unconsumed input data */
    if (in.pos == in.size) {
        if (Py_SIZE(ret) == max_length || self->eof) {
            self->needs_input = 0;
        }
        else {
            self->needs_input = 1;
        }

        if (use_input_buffer) {
            /* Clear input_buffer */
            self->in_begin = 0;
            self->in_end = 0;
        }
    }
    else {
        size_t data_size = in.size - in.pos;

        self->needs_input = 0;

        if (!use_input_buffer) {
            /* Discard buffer if it's too small
               (resizing it may needlessly copy the current contents) */
            if (self->input_buffer != NULL
                && self->input_buffer_size < data_size)
            {
                PyMem_Free(self->input_buffer);
                self->input_buffer = NULL;
                self->input_buffer_size = 0;
            }

            /* Allocate if necessary */
            if (self->input_buffer == NULL) {
                self->input_buffer = PyMem_Malloc(data_size);
                if (self->input_buffer == NULL) {
                    PyErr_NoMemory();
                    goto error;
                }
                self->input_buffer_size = data_size;
            }

            /* Copy unconsumed data */
            memcpy(self->input_buffer, (char*)in.src + in.pos, data_size);
            self->in_begin = 0;
            self->in_end = data_size;
        }
        else {
            /* Use input buffer */
            self->in_begin += in.pos;
        }
    }

    return ret;

error:
    /* Reset decompressor's states/session */
    decompressor_reset_session_lock_held(self);

    Py_CLEAR(ret);
    return NULL;
}


/*[clinic input]
@permit_long_docstring_body
@classmethod
_zstd.ZstdDecompressor.__new__ as _zstd_ZstdDecompressor_new
    zstd_dict: object = None
        A ZstdDict object, a pre-trained Zstandard dictionary.
    options: object = None
        A dict object that contains advanced decompression parameters.

Create a decompressor object for decompressing data incrementally.

Thread-safe at method level. For one-shot decompression, use the decompress()
function instead.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDecompressor_new_impl(PyTypeObject *type, PyObject *zstd_dict,
                                PyObject *options)
/*[clinic end generated code: output=590ca65c1102ff4a input=ed8891edfd14cdaa]*/
{
    ZstdDecompressor* self = PyObject_GC_New(ZstdDecompressor, type);
    if (self == NULL) {
        goto error;
    }

    self->input_buffer = NULL;
    self->input_buffer_size = 0;
    self->in_begin = -1;
    self->in_end = -1;
    self->unused_data = NULL;
    self->eof = 0;
    self->dict = NULL;
    self->lock = (PyMutex){0};

    /* needs_input flag */
    self->needs_input = 1;

    /* Decompression context */
    self->dctx = ZSTD_createDCtx();
    if (self->dctx == NULL) {
        _zstd_state* mod_state = PyType_GetModuleState(Py_TYPE(self));
        if (mod_state != NULL) {
            PyErr_SetString(mod_state->ZstdError,
                            "Unable to create ZSTD_DCtx instance.");
        }
        goto error;
    }

    /* Load Zstandard dictionary to decompression context */
    if (zstd_dict != Py_None) {
        if (_zstd_load_d_dict(self, zstd_dict) < 0) {
            goto error;
        }
        Py_INCREF(zstd_dict);
        self->dict = zstd_dict;
    }

    /* Set options dictionary */
    if (options != Py_None) {
        if (_zstd_set_d_parameters(self, options) < 0) {
            goto error;
        }
    }

    // We can only start GC tracking once self->dict is set.
    PyObject_GC_Track(self);

    return (PyObject*)self;

error:
    Py_XDECREF(self);
    return NULL;
}

static void
ZstdDecompressor_dealloc(PyObject *ob)
{
    ZstdDecompressor *self = ZstdDecompressor_CAST(ob);

    PyObject_GC_UnTrack(self);

    /* Free decompression context */
    if (self->dctx) {
        ZSTD_freeDCtx(self->dctx);
    }

    assert(!PyMutex_IsLocked(&self->lock));

    /* Py_CLEAR the dict after free decompression context */
    Py_CLEAR(self->dict);

    /* Free unconsumed input data buffer */
    PyMem_Free(self->input_buffer);

    /* Free unused data */
    Py_CLEAR(self->unused_data);

    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_Del(ob);
    Py_DECREF(tp);
}

/*[clinic input]
@permit_long_docstring_body
@getter
_zstd.ZstdDecompressor.unused_data

A bytes object of un-consumed input data.

When ZstdDecompressor object stops after a frame is
decompressed, unused input data after the frame. Otherwise this will be b''.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDecompressor_unused_data_get_impl(ZstdDecompressor *self)
/*[clinic end generated code: output=f3a20940f11b6b09 input=37c2c531ab56f914]*/
{
    PyObject *ret;

    PyMutex_Lock(&self->lock);

    if (!self->eof) {
        PyMutex_Unlock(&self->lock);
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    else {
        if (self->unused_data == NULL) {
            self->unused_data = PyBytes_FromStringAndSize(
                                    self->input_buffer + self->in_begin,
                                    self->in_end - self->in_begin);
            ret = self->unused_data;
            Py_XINCREF(ret);
        }
        else {
            ret = self->unused_data;
            Py_INCREF(ret);
        }
    }

    PyMutex_Unlock(&self->lock);
    return ret;
}

/*[clinic input]
@permit_long_summary
@permit_long_docstring_body
_zstd.ZstdDecompressor.decompress

    data: Py_buffer
        A bytes-like object, Zstandard data to be decompressed.
    max_length: Py_ssize_t = -1
        Maximum size of returned data. When it is negative, the size of
        output buffer is unlimited. When it is nonnegative, returns at
        most max_length bytes of decompressed data.

Decompress *data*, returning uncompressed bytes if possible, or b'' otherwise.

If *max_length* is nonnegative, returns at most *max_length* bytes of
decompressed data. If this limit is reached and further output can be
produced, *self.needs_input* will be set to ``False``. In this case, the next
call to *decompress()* may provide *data* as b'' to obtain more of the output.

If all of the input data was decompressed and returned (either because this
was less than *max_length* bytes, or because *max_length* was negative),
*self.needs_input* will be set to True.

Attempting to decompress data after the end of a frame is reached raises an
EOFError. Any data found after the end of the frame is ignored and saved in
the self.unused_data attribute.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDecompressor_decompress_impl(ZstdDecompressor *self,
                                       Py_buffer *data,
                                       Py_ssize_t max_length)
/*[clinic end generated code: output=a4302b3c940dbec6 input=e5c905a774df1553]*/
{
    PyObject *ret;
    /* Thread-safe code */
    PyMutex_Lock(&self->lock);
    ret = stream_decompress_lock_held(self, data, max_length);
    PyMutex_Unlock(&self->lock);
    return ret;
}

static PyMethodDef ZstdDecompressor_methods[] = {
    _ZSTD_ZSTDDECOMPRESSOR_DECOMPRESS_METHODDEF
    {NULL, NULL}
};

PyDoc_STRVAR(ZstdDecompressor_eof_doc,
"True means the end of the first frame has been reached. If decompress data\n"
"after that, an EOFError exception will be raised.");

PyDoc_STRVAR(ZstdDecompressor_needs_input_doc,
"If the max_length output limit in .decompress() method has been reached,\n"
"and the decompressor has (or may has) unconsumed input data, it will be set\n"
"to False. In this case, passing b'' to the .decompress() method may output\n"
"further data.");

static PyMemberDef ZstdDecompressor_members[] = {
    {"eof", Py_T_BOOL, offsetof(ZstdDecompressor, eof),
    Py_READONLY, ZstdDecompressor_eof_doc},
    {"needs_input", Py_T_BOOL, offsetof(ZstdDecompressor, needs_input),
    Py_READONLY, ZstdDecompressor_needs_input_doc},
    {NULL}
};

static PyGetSetDef ZstdDecompressor_getset[] = {
    _ZSTD_ZSTDDECOMPRESSOR_UNUSED_DATA_GETSETDEF
    {NULL}
};

static int
ZstdDecompressor_traverse(PyObject *ob, visitproc visit, void *arg)
{
    ZstdDecompressor *self = ZstdDecompressor_CAST(ob);
    Py_VISIT(self->dict);
    return 0;
}

static int
ZstdDecompressor_clear(PyObject *ob)
{
    ZstdDecompressor *self = ZstdDecompressor_CAST(ob);
    Py_CLEAR(self->dict);
    Py_CLEAR(self->unused_data);
    return 0;
}

static PyType_Slot ZstdDecompressor_slots[] = {
    {Py_tp_new, _zstd_ZstdDecompressor_new},
    {Py_tp_dealloc, ZstdDecompressor_dealloc},
    {Py_tp_methods, ZstdDecompressor_methods},
    {Py_tp_members, ZstdDecompressor_members},
    {Py_tp_getset, ZstdDecompressor_getset},
    {Py_tp_doc, (void *)_zstd_ZstdDecompressor_new__doc__},
    {Py_tp_traverse, ZstdDecompressor_traverse},
    {Py_tp_clear, ZstdDecompressor_clear},
    {0, 0}
};

PyType_Spec zstd_decompressor_type_spec = {
    .name = "compression.zstd.ZstdDecompressor",
    .basicsize = sizeof(ZstdDecompressor),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE
             | Py_TPFLAGS_HAVE_GC,
    .slots = ZstdDecompressor_slots,
};
