/*
Low level interface to Meta's zstd library for use in the compression.zstd
Python module.
*/

/* ZstdDecompressor class definition */

/*[clinic input]
module _zstd
class _zstd.ZstdDecompressor "ZstdDecompressor *" "clinic_state()->ZstdDecompressor_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4e6eae327c0c0c76]*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "_zstdmodule.h"

#include "buffer.h"

#include <stddef.h>               // offsetof()

#define ZstdDecompressor_CAST(op) ((ZstdDecompressor *)op)

static inline ZSTD_DDict *
_get_DDict(ZstdDict *self)
{
    ZSTD_DDict *ret;

    /* Already created */
    if (self->d_dict != NULL) {
        return self->d_dict;
    }

    Py_BEGIN_CRITICAL_SECTION(self);
    if (self->d_dict == NULL) {
        /* Create ZSTD_DDict instance from dictionary content */
        char *dict_buffer = PyBytes_AS_STRING(self->dict_content);
        Py_ssize_t dict_len = Py_SIZE(self->dict_content);
        Py_BEGIN_ALLOW_THREADS
        self->d_dict = ZSTD_createDDict(dict_buffer,
                                        dict_len);
        Py_END_ALLOW_THREADS

        if (self->d_dict == NULL) {
            _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
            if (mod_state != NULL) {
                PyErr_SetString(mod_state->ZstdError,
                                "Failed to create ZSTD_DDict instance from zstd "
                                "dictionary content. Maybe the content is corrupted.");
            }
        }
    }

    /* Don't lose any exception */
    ret = self->d_dict;
    Py_END_CRITICAL_SECTION();

    return ret;
}

/* Set decompression parameters to decompression context */
static int
_zstd_set_d_parameters(ZstdDecompressor *self, PyObject *options)
{
    size_t zstd_ret;
    PyObject *key, *value;
    Py_ssize_t pos;
    _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
    if (mod_state == NULL) {
        return -1;
    }

    if (!PyDict_Check(options)) {
        PyErr_SetString(PyExc_TypeError,
                        "options argument should be dict object.");
        return -1;
    }

    pos = 0;
    while (PyDict_Next(options, &pos, &key, &value)) {
        /* Check key type */
        if (Py_TYPE(key) == mod_state->CParameter_type) {
            PyErr_SetString(PyExc_TypeError,
                            "Key of decompression options dict should "
                            "NOT be CompressionParameter.");
            return -1;
        }

        /* Both key & value should be 32-bit signed int */
        int key_v = PyLong_AsInt(key);
        if (key_v == -1 && PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "Key of options dict should be a DecompressionParameter attribute.");
            return -1;
        }

        // TODO(emmatyping): check bounds when there is a value error here for better
        // error message?
        int value_v = PyLong_AsInt(value);
        if (value_v == -1 && PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "Value of options dict should be an int.");
            return -1;
        }

        /* Set parameter to compression context */
        Py_BEGIN_CRITICAL_SECTION(self);
        zstd_ret = ZSTD_DCtx_setParameter(self->dctx, key_v, value_v);
        Py_END_CRITICAL_SECTION();

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            set_parameter_error(mod_state, 0, key_v, value_v);
            return -1;
        }
    }
    return 0;
}

/* Load dictionary or prefix to decompression context */
static int
_zstd_load_d_dict(ZstdDecompressor *self, PyObject *dict)
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
        /* When decompressing, use digested dictionary by default. */
        zd = (ZstdDict*)dict;
        type = DICT_TYPE_DIGESTED;
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
        /* Get ZSTD_DDict */
        ZSTD_DDict *d_dict = _get_DDict(zd);
        if (d_dict == NULL) {
            return -1;
        }
        /* Reference a prepared dictionary */
        Py_BEGIN_CRITICAL_SECTION(self);
        zstd_ret = ZSTD_DCtx_refDDict(self->dctx, d_dict);
        Py_END_CRITICAL_SECTION();
    }
    else if (type == DICT_TYPE_UNDIGESTED) {
        /* Load a dictionary */
        Py_BEGIN_CRITICAL_SECTION2(self, zd);
        zstd_ret = ZSTD_DCtx_loadDictionary(
                            self->dctx,
                            PyBytes_AS_STRING(zd->dict_content),
                            Py_SIZE(zd->dict_content));
        Py_END_CRITICAL_SECTION2();
    }
    else if (type == DICT_TYPE_PREFIX) {
        /* Load a prefix */
        Py_BEGIN_CRITICAL_SECTION2(self, zd);
        zstd_ret = ZSTD_DCtx_refPrefix(
                            self->dctx,
                            PyBytes_AS_STRING(zd->dict_content),
                            Py_SIZE(zd->dict_content));
        Py_END_CRITICAL_SECTION2();
    }
    else {
        /* Impossible code path */
        PyErr_SetString(PyExc_SystemError,
                        "load_d_dict() impossible code path");
        return -1;
    }

    /* Check error */
    if (ZSTD_isError(zstd_ret)) {
        set_zstd_error(mod_state, ERR_LOAD_D_DICT, zstd_ret);
        return -1;
    }
    return 0;
}



/*
    Given the two types of decompressors (defined in _zstdmodule.h):

        typedef enum {
            TYPE_DECOMPRESSOR,          // <D>, ZstdDecompressor class
            TYPE_ENDLESS_DECOMPRESSOR,  // <E>, decompress() function
        } decompress_type;

    Decompress implementation for <D>, <E>, pseudo code:

        initialize_output_buffer
        while True:
            decompress_data
            set_object_flag   # .eof for <D>, .at_frame_edge for <E>.

            if output_buffer_exhausted:
                if output_buffer_reached_max_length:
                    finish
                grow_output_buffer
            elif input_buffer_exhausted:
                finish

    ZSTD_decompressStream()'s size_t return value:
      - 0 when a frame is completely decoded and fully flushed, zstd's internal
        buffer has no data.
      - An error code, which can be tested using ZSTD_isError().
      - Or any other value > 0, which means there is still some decoding or
        flushing to do to complete current frame.

      Note, decompressing "an empty input" in any case will make it > 0.

    <E> supports multiple frames, has an .at_frame_edge flag, it means both the
    input and output streams are at a frame edge. The flag can be set by this
    statement:

        .at_frame_edge = (zstd_ret == 0) ? 1 : 0

    But if decompressing "an empty input" at "a frame edge", zstd_ret will be
    non-zero, then .at_frame_edge will be wrongly set to false. To solve this
    problem, two AFE checks are needed to ensure that: when at "a frame edge",
    empty input will not be decompressed.

        // AFE check
        if (self->at_frame_edge && in->pos == in->size) {
            finish
        }

    In <E>, if .at_frame_edge is eventually set to true, but input stream has
    unconsumed data (in->pos < in->size), then the outer function
    stream_decompress() will set .at_frame_edge to false. In this case,
    although the output stream is at a frame edge, for the caller, the input
    stream is not at a frame edge, see below diagram. This behavior does not
    affect the next AFE check, since (in->pos < in->size).

    input stream:  --------------|---
                                    ^
    output stream: ====================|
                                       ^
*/
PyObject *
decompress_impl(ZstdDecompressor *self, ZSTD_inBuffer *in,
                Py_ssize_t max_length,
                Py_ssize_t initial_size,
                decompress_type type)
{
    size_t zstd_ret;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer = {.list = NULL};
    PyObject *ret;

    /* The first AFE check for setting .at_frame_edge flag */
    if (type == TYPE_ENDLESS_DECOMPRESSOR) {
        if (self->at_frame_edge && in->pos == in->size) {
            _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
            if (mod_state == NULL) {
                return NULL;
            }
            ret = mod_state->empty_bytes;
            Py_INCREF(ret);
            return ret;
        }
    }

    /* Initialize the output buffer */
    if (initial_size >= 0) {
        if (_OutputBuffer_InitWithSize(&buffer, &out, max_length, initial_size) < 0) {
            goto error;
        }
    }
    else {
        if (_OutputBuffer_InitAndGrow(&buffer, &out, max_length) < 0) {
            goto error;
        }
    }
    assert(out.pos == 0);

    while (1) {
        /* Decompress */
        Py_BEGIN_ALLOW_THREADS
        zstd_ret = ZSTD_decompressStream(self->dctx, &out, in);
        Py_END_ALLOW_THREADS

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
            if (mod_state != NULL) {
                set_zstd_error(mod_state, ERR_DECOMPRESS, zstd_ret);
            }
            goto error;
        }

        /* Set .eof/.af_frame_edge flag */
        if (type == TYPE_DECOMPRESSOR) {
            /* ZstdDecompressor class stops when a frame is decompressed */
            if (zstd_ret == 0) {
                self->eof = 1;
                break;
            }
        }
        else if (type == TYPE_ENDLESS_DECOMPRESSOR) {
            /* decompress() function supports multiple frames */
            self->at_frame_edge = (zstd_ret == 0) ? 1 : 0;

            /* The second AFE check for setting .at_frame_edge flag */
            if (self->at_frame_edge && in->pos == in->size) {
                break;
            }
        }

        /* Need to check out before in. Maybe zstd's internal buffer still has
           a few bytes can be output, grow the buffer and continue. */
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

void
decompressor_reset_session(ZstdDecompressor *self,
                           decompress_type type)
{
    // TODO(emmatyping): use _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED here
    // and ensure lock is always held

    /* Reset variables */
    self->in_begin = 0;
    self->in_end = 0;

    if (type == TYPE_DECOMPRESSOR) {
        Py_CLEAR(self->unused_data);
    }

    /* Reset variables in one operation */
    self->needs_input = 1;
    self->at_frame_edge = 1;
    self->eof = 0;
    self->_unused_char_for_align = 0;

    /* Resetting session never fail */
    ZSTD_DCtx_reset(self->dctx, ZSTD_reset_session_only);
}

PyObject *
stream_decompress(ZstdDecompressor *self, Py_buffer *data, Py_ssize_t max_length,
                  decompress_type type)
{
    Py_ssize_t initial_buffer_size = -1;
    ZSTD_inBuffer in;
    PyObject *ret = NULL;
    int use_input_buffer;

    if (type == TYPE_DECOMPRESSOR) {
        /* Check .eof flag */
        if (self->eof) {
            PyErr_SetString(PyExc_EOFError, "Already at the end of a zstd frame.");
            assert(ret == NULL);
            goto success;
        }
    }
    else if (type == TYPE_ENDLESS_DECOMPRESSOR) {
        /* Fast path for the first frame */
        if (self->at_frame_edge && self->in_begin == self->in_end) {
            /* Read decompressed size */
            uint64_t decompressed_size = ZSTD_getFrameContentSize(data->buf, data->len);

            /* These two zstd constants always > PY_SSIZE_T_MAX:
                  ZSTD_CONTENTSIZE_UNKNOWN is (0ULL - 1)
                  ZSTD_CONTENTSIZE_ERROR   is (0ULL - 2)

               Use ZSTD_findFrameCompressedSize() to check complete frame,
               prevent allocating too much memory for small input chunk. */

            if (decompressed_size <= (uint64_t) PY_SSIZE_T_MAX &&
                !ZSTD_isError(ZSTD_findFrameCompressedSize(data->buf, data->len)) )
            {
                initial_buffer_size = (Py_ssize_t) decompressed_size;
            }
        }
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
    ret = decompress_impl(self, &in,
                          max_length, initial_buffer_size,
                          type);
    if (ret == NULL) {
        goto error;
    }

    /* Unconsumed input data */
    if (in.pos == in.size) {
        if (type == TYPE_DECOMPRESSOR) {
            if (Py_SIZE(ret) == max_length || self->eof) {
                self->needs_input = 0;
            }
            else {
                self->needs_input = 1;
            }
        }
        else if (type == TYPE_ENDLESS_DECOMPRESSOR) {
            if (Py_SIZE(ret) == max_length && !self->at_frame_edge) {
                self->needs_input = 0;
            }
            else {
                self->needs_input = 1;
            }
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

        if (type == TYPE_ENDLESS_DECOMPRESSOR) {
            self->at_frame_edge = 0;
        }

        if (!use_input_buffer) {
            /* Discard buffer if it's too small
               (resizing it may needlessly copy the current contents) */
            if (self->input_buffer != NULL &&
                self->input_buffer_size < data_size)
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

    goto success;

error:
    /* Reset decompressor's states/session */
    decompressor_reset_session(self, type);

    Py_CLEAR(ret);
success:

    return ret;
}


static PyObject *
_zstd_ZstdDecompressor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ZstdDecompressor *self;
    self = PyObject_GC_New(ZstdDecompressor, type);
    if (self == NULL) {
        goto error;
    }

    self->inited = 0;
    self->dict = NULL;
    self->input_buffer = NULL;
    self->input_buffer_size = 0;
    self->in_begin = -1;
    self->in_end = -1;
    self->unused_data = NULL;
    self->eof = 0;

    /* needs_input flag */
    self->needs_input = 1;

    /* at_frame_edge flag */
    self->at_frame_edge = 1;

    /* Decompression context */
    self->dctx = ZSTD_createDCtx();
    if (self->dctx == NULL) {
        _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
        if (mod_state != NULL) {
            PyErr_SetString(mod_state->ZstdError,
                            "Unable to create ZSTD_DCtx instance.");
        }
        goto error;
    }

    return (PyObject*)self;

error:
    if (self != NULL) {
        PyObject_GC_Del(self);
    }
    return NULL;
}

static void
ZstdDecompressor_dealloc(PyObject *ob)
{
    ZstdDecompressor *self = ZstdDecompressor_CAST(ob);

    PyObject_GC_UnTrack(self);

    /* Free decompression context */
    ZSTD_freeDCtx(self->dctx);

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
_zstd.ZstdDecompressor.__init__

    zstd_dict: object = None
        A ZstdDict object, a pre-trained zstd dictionary.
    options: object = None
        A dict object that contains advanced decompression parameters.

Create a decompressor object for decompressing data incrementally.

Thread-safe at method level. For one-shot decompression, use the decompress()
function instead.
[clinic start generated code]*/

static int
_zstd_ZstdDecompressor___init___impl(ZstdDecompressor *self,
                                     PyObject *zstd_dict, PyObject *options)
/*[clinic end generated code: output=703af2f1ec226642 input=8fd72999acc1a146]*/
{
    /* Only called once */
    if (self->inited) {
        PyErr_SetString(PyExc_RuntimeError, init_twice_msg);
        return -1;
    }
    self->inited = 1;

    /* Load dictionary to decompression context */
    if (zstd_dict != Py_None) {
        if (_zstd_load_d_dict(self, zstd_dict) < 0) {
            return -1;
        }

        /* Py_INCREF the dict */
        Py_INCREF(zstd_dict);
        self->dict = zstd_dict;
    }

    /* Set option to decompression context */
    if (options != Py_None) {
        if (_zstd_set_d_parameters(self, options) < 0) {
            return -1;
        }
    }

    // We can only start tracking self with the GC once self->dict is set.
    PyObject_GC_Track(self);
    return 0;
}

/*[clinic input]
@critical_section
@getter
_zstd.ZstdDecompressor.unused_data

A bytes object of un-consumed input data.

When ZstdDecompressor object stops after a frame is
decompressed, unused input data after the frame. Otherwise this will be b''.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDecompressor_unused_data_get_impl(ZstdDecompressor *self)
/*[clinic end generated code: output=f3a20940f11b6b09 input=5233800bef00df04]*/
{
    PyObject *ret;

    /* Thread-safe code */
    Py_BEGIN_CRITICAL_SECTION(self);

    if (!self->eof) {
        _zstd_state* const mod_state = PyType_GetModuleState(Py_TYPE(self));
        if (mod_state == NULL) {
            return NULL;
        }
        ret = mod_state->empty_bytes;
        Py_INCREF(ret);
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

    Py_END_CRITICAL_SECTION();

    return ret;
}

/*[clinic input]
_zstd.ZstdDecompressor.decompress

    data: Py_buffer
        A bytes-like object, zstd data to be decompressed.
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
/*[clinic end generated code: output=a4302b3c940dbec6 input=830e455bc9a50b6e]*/
{
    PyObject *ret;
    /* Thread-safe code */
    Py_BEGIN_CRITICAL_SECTION(self);

    ret = stream_decompress(self, data, max_length, TYPE_DECOMPRESSOR);
    Py_END_CRITICAL_SECTION();
    return ret;
}

#define clinic_state() (get_zstd_state_from_type(type))
#include "clinic/decompressor.c.h"
#undef clinic_state

static PyMethodDef ZstdDecompressor_methods[] = {
    _ZSTD_ZSTDDECOMPRESSOR_DECOMPRESS_METHODDEF

    {0}
};

PyDoc_STRVAR(ZstdDecompressor_eof_doc,
"True means the end of the first frame has been reached. If decompress data\n"
"after that, an EOFError exception will be raised.");

PyDoc_STRVAR(ZstdDecompressor_needs_input_doc,
"If the max_length output limit in .decompress() method has been reached, and\n"
"the decompressor has (or may has) unconsumed input data, it will be set to\n"
"False. In this case, pass b'' to .decompress() method may output further data.");

static PyMemberDef ZstdDecompressor_members[] = {
    {"eof", Py_T_BOOL, offsetof(ZstdDecompressor, eof),
    Py_READONLY, ZstdDecompressor_eof_doc},

    {"needs_input", Py_T_BOOL, offsetof(ZstdDecompressor, needs_input),
    Py_READONLY, ZstdDecompressor_needs_input_doc},

    {0}
};

static PyGetSetDef ZstdDecompressor_getset[] = {
    _ZSTD_ZSTDDECOMPRESSOR_UNUSED_DATA_GETSETDEF

    {0}
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
    {Py_tp_init, _zstd_ZstdDecompressor___init__},
    {Py_tp_methods, ZstdDecompressor_methods},
    {Py_tp_members, ZstdDecompressor_members},
    {Py_tp_getset, ZstdDecompressor_getset},
    {Py_tp_doc, (char*)_zstd_ZstdDecompressor___init____doc__},
    {Py_tp_traverse, ZstdDecompressor_traverse},
    {Py_tp_clear, ZstdDecompressor_clear},
    {0}
};

PyType_Spec ZstdDecompressor_type_spec = {
    .name = "_zstd.ZstdDecompressor",
    .basicsize = sizeof(ZstdDecompressor),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = ZstdDecompressor_slots,
};
