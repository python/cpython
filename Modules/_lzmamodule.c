/* _lzma - Low-level Python interface to liblzma.

   Initial implementation by Per Øyvind Karlsen.
   Rewritten by Nadeem Vawda.

*/

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h"
#ifdef WITH_THREAD
#include "pythread.h"
#endif

#include <stdarg.h>
#include <string.h>

#include <lzma.h>


#ifndef PY_LONG_LONG
#error "This module requires PY_LONG_LONG to be defined"
#endif


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


/* Container formats: */
enum {
    FORMAT_AUTO,
    FORMAT_XZ,
    FORMAT_ALONE,
    FORMAT_RAW,
};

#define LZMA_CHECK_UNKNOWN (LZMA_CHECK_ID_MAX + 1)


typedef struct {
    PyObject_HEAD
    lzma_allocator alloc;
    lzma_stream lzs;
    int flushed;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} Compressor;

typedef struct {
    PyObject_HEAD
    lzma_allocator alloc;
    lzma_stream lzs;
    int check;
    char eof;
    PyObject *unused_data;
    char needs_input;
    uint8_t *input_buffer;
    size_t input_buffer_size;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} Decompressor;

/* LZMAError class object. */
static PyObject *Error;

/* An empty tuple, used by the filter specifier parsing code. */
static PyObject *empty_tuple;


/* Helper functions. */

static int
catch_lzma_error(lzma_ret lzret)
{
    switch (lzret) {
        case LZMA_OK:
        case LZMA_GET_CHECK:
        case LZMA_NO_CHECK:
        case LZMA_STREAM_END:
            return 0;
        case LZMA_UNSUPPORTED_CHECK:
            PyErr_SetString(Error, "Unsupported integrity check");
            return 1;
        case LZMA_MEM_ERROR:
            PyErr_NoMemory();
            return 1;
        case LZMA_MEMLIMIT_ERROR:
            PyErr_SetString(Error, "Memory usage limit exceeded");
            return 1;
        case LZMA_FORMAT_ERROR:
            PyErr_SetString(Error, "Input format not supported by decoder");
            return 1;
        case LZMA_OPTIONS_ERROR:
            PyErr_SetString(Error, "Invalid or unsupported options");
            return 1;
        case LZMA_DATA_ERROR:
            PyErr_SetString(Error, "Corrupt input data");
            return 1;
        case LZMA_BUF_ERROR:
            PyErr_SetString(Error, "Insufficient buffer space");
            return 1;
        case LZMA_PROG_ERROR:
            PyErr_SetString(Error, "Internal error");
            return 1;
        default:
            PyErr_Format(Error, "Unrecognized error from liblzma: %d", lzret);
            return 1;
    }
}

static void*
PyLzma_Malloc(void *opaque, size_t items, size_t size)
{
    if (items > (size_t)PY_SSIZE_T_MAX / size)
        return NULL;
    /* PyMem_Malloc() cannot be used:
       the GIL is not held when lzma_code() is called */
    return PyMem_RawMalloc(items * size);
}

static void
PyLzma_Free(void *opaque, void *ptr)
{
    PyMem_RawFree(ptr);
}

#if BUFSIZ < 8192
#define INITIAL_BUFFER_SIZE 8192
#else
#define INITIAL_BUFFER_SIZE BUFSIZ
#endif

static int
grow_buffer(PyObject **buf, Py_ssize_t max_length)
{
    Py_ssize_t size = PyBytes_GET_SIZE(*buf);
    Py_ssize_t newsize = size + (size >> 3) + 6;

    if (max_length > 0 && newsize > max_length)
        newsize = max_length;

    return _PyBytes_Resize(buf, newsize);
}


/* Some custom type conversions for PyArg_ParseTupleAndKeywords(),
   since the predefined conversion specifiers do not suit our needs:

      uint32_t - the "I" (unsigned int) specifier is the right size, but
      silently ignores overflows on conversion.

      lzma_vli - the "K" (unsigned PY_LONG_LONG) specifier is the right
      size, but like "I" it silently ignores overflows on conversion.

      lzma_mode and lzma_match_finder - these are enumeration types, and
      so the size of each is implementation-defined. Worse, different
      enum types can be of different sizes within the same program, so
      to be strictly correct, we need to define two separate converters.
 */

#define INT_TYPE_CONVERTER_FUNC(TYPE, FUNCNAME) \
    static int \
    FUNCNAME(PyObject *obj, void *ptr) \
    { \
        unsigned PY_LONG_LONG val; \
        \
        val = PyLong_AsUnsignedLongLong(obj); \
        if (PyErr_Occurred()) \
            return 0; \
        if ((unsigned PY_LONG_LONG)(TYPE)val != val) { \
            PyErr_SetString(PyExc_OverflowError, \
                            "Value too large for " #TYPE " type"); \
            return 0; \
        } \
        *(TYPE *)ptr = (TYPE)val; \
        return 1; \
    }

INT_TYPE_CONVERTER_FUNC(uint32_t, uint32_converter)
INT_TYPE_CONVERTER_FUNC(lzma_vli, lzma_vli_converter)
INT_TYPE_CONVERTER_FUNC(lzma_mode, lzma_mode_converter)
INT_TYPE_CONVERTER_FUNC(lzma_match_finder, lzma_mf_converter)

#undef INT_TYPE_CONVERTER_FUNC


/* Filter specifier parsing.

   This code handles converting filter specifiers (Python dicts) into
   the C lzma_filter structs expected by liblzma. */

static void *
parse_filter_spec_lzma(PyObject *spec)
{
    static char *optnames[] = {"id", "preset", "dict_size", "lc", "lp",
                               "pb", "mode", "nice_len", "mf", "depth", NULL};
    PyObject *id;
    PyObject *preset_obj;
    uint32_t preset = LZMA_PRESET_DEFAULT;
    lzma_options_lzma *options;

    /* First, fill in default values for all the options using a preset.
       Then, override the defaults with any values given by the caller. */

    preset_obj = PyMapping_GetItemString(spec, "preset");
    if (preset_obj == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyError))
            PyErr_Clear();
        else
            return NULL;
    } else {
        int ok = uint32_converter(preset_obj, &preset);
        Py_DECREF(preset_obj);
        if (!ok)
            return NULL;
    }

    options = (lzma_options_lzma *)PyMem_Malloc(sizeof *options);
    if (options == NULL)
        return PyErr_NoMemory();
    memset(options, 0, sizeof *options);

    if (lzma_lzma_preset(options, preset)) {
        PyMem_Free(options);
        PyErr_Format(Error, "Invalid compression preset: %d", preset);
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(empty_tuple, spec,
                                     "|OOO&O&O&O&O&O&O&O&", optnames,
                                     &id, &preset_obj,
                                     uint32_converter, &options->dict_size,
                                     uint32_converter, &options->lc,
                                     uint32_converter, &options->lp,
                                     uint32_converter, &options->pb,
                                     lzma_mode_converter, &options->mode,
                                     uint32_converter, &options->nice_len,
                                     lzma_mf_converter, &options->mf,
                                     uint32_converter, &options->depth)) {
        PyErr_SetString(PyExc_ValueError,
                        "Invalid filter specifier for LZMA filter");
        PyMem_Free(options);
        options = NULL;
    }
    return options;
}

static void *
parse_filter_spec_delta(PyObject *spec)
{
    static char *optnames[] = {"id", "dist", NULL};
    PyObject *id;
    uint32_t dist = 1;
    lzma_options_delta *options;

    if (!PyArg_ParseTupleAndKeywords(empty_tuple, spec, "|OO&", optnames,
                                     &id, uint32_converter, &dist)) {
        PyErr_SetString(PyExc_ValueError,
                        "Invalid filter specifier for delta filter");
        return NULL;
    }

    options = (lzma_options_delta *)PyMem_Malloc(sizeof *options);
    if (options == NULL)
        return PyErr_NoMemory();
    memset(options, 0, sizeof *options);
    options->type = LZMA_DELTA_TYPE_BYTE;
    options->dist = dist;
    return options;
}

static void *
parse_filter_spec_bcj(PyObject *spec)
{
    static char *optnames[] = {"id", "start_offset", NULL};
    PyObject *id;
    uint32_t start_offset = 0;
    lzma_options_bcj *options;

    if (!PyArg_ParseTupleAndKeywords(empty_tuple, spec, "|OO&", optnames,
                                     &id, uint32_converter, &start_offset)) {
        PyErr_SetString(PyExc_ValueError,
                        "Invalid filter specifier for BCJ filter");
        return NULL;
    }

    options = (lzma_options_bcj *)PyMem_Malloc(sizeof *options);
    if (options == NULL)
        return PyErr_NoMemory();
    memset(options, 0, sizeof *options);
    options->start_offset = start_offset;
    return options;
}

static int
lzma_filter_converter(PyObject *spec, void *ptr)
{
    lzma_filter *f = (lzma_filter *)ptr;
    PyObject *id_obj;

    if (!PyMapping_Check(spec)) {
        PyErr_SetString(PyExc_TypeError,
                        "Filter specifier must be a dict or dict-like object");
        return 0;
    }
    id_obj = PyMapping_GetItemString(spec, "id");
    if (id_obj == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyError))
            PyErr_SetString(PyExc_ValueError,
                            "Filter specifier must have an \"id\" entry");
        return 0;
    }
    f->id = PyLong_AsUnsignedLongLong(id_obj);
    Py_DECREF(id_obj);
    if (PyErr_Occurred())
        return 0;

    switch (f->id) {
        case LZMA_FILTER_LZMA1:
        case LZMA_FILTER_LZMA2:
            f->options = parse_filter_spec_lzma(spec);
            return f->options != NULL;
        case LZMA_FILTER_DELTA:
            f->options = parse_filter_spec_delta(spec);
            return f->options != NULL;
        case LZMA_FILTER_X86:
        case LZMA_FILTER_POWERPC:
        case LZMA_FILTER_IA64:
        case LZMA_FILTER_ARM:
        case LZMA_FILTER_ARMTHUMB:
        case LZMA_FILTER_SPARC:
            f->options = parse_filter_spec_bcj(spec);
            return f->options != NULL;
        default:
            PyErr_Format(PyExc_ValueError, "Invalid filter ID: %llu", f->id);
            return 0;
    }
}

static void
free_filter_chain(lzma_filter filters[])
{
    int i;

    for (i = 0; filters[i].id != LZMA_VLI_UNKNOWN; i++)
        PyMem_Free(filters[i].options);
}

static int
parse_filter_chain_spec(lzma_filter filters[], PyObject *filterspecs)
{
    Py_ssize_t i, num_filters;

    num_filters = PySequence_Length(filterspecs);
    if (num_filters == -1)
        return -1;
    if (num_filters > LZMA_FILTERS_MAX) {
        PyErr_Format(PyExc_ValueError,
                     "Too many filters - liblzma supports a maximum of %d",
                     LZMA_FILTERS_MAX);
        return -1;
    }

    for (i = 0; i < num_filters; i++) {
        int ok = 1;
        PyObject *spec = PySequence_GetItem(filterspecs, i);
        if (spec == NULL || !lzma_filter_converter(spec, &filters[i]))
            ok = 0;
        Py_XDECREF(spec);
        if (!ok) {
            filters[i].id = LZMA_VLI_UNKNOWN;
            free_filter_chain(filters);
            return -1;
        }
    }
    filters[num_filters].id = LZMA_VLI_UNKNOWN;
    return 0;
}


/* Filter specifier construction.

   This code handles converting C lzma_filter structs into
   Python-level filter specifiers (represented as dicts). */

static int
spec_add_field(PyObject *spec, _Py_Identifier *key, unsigned PY_LONG_LONG value)
{
    int status;
    PyObject *value_object;

    value_object = PyLong_FromUnsignedLongLong(value);
    if (value_object == NULL)
        return -1;

    status = _PyDict_SetItemId(spec, key, value_object);
    Py_DECREF(value_object);
    return status;
}

static PyObject *
build_filter_spec(const lzma_filter *f)
{
    PyObject *spec;

    spec = PyDict_New();
    if (spec == NULL)
        return NULL;

#define ADD_FIELD(SOURCE, FIELD) \
    do { \
        _Py_IDENTIFIER(FIELD); \
        if (spec_add_field(spec, &PyId_##FIELD, SOURCE->FIELD) == -1) \
            goto error;\
    } while (0)

    ADD_FIELD(f, id);

    switch (f->id) {
        /* For LZMA1 filters, lzma_properties_{encode,decode}() only look at the
           lc, lp, pb, and dict_size fields. For LZMA2 filters, only the
           dict_size field is used. */
        case LZMA_FILTER_LZMA1: {
            lzma_options_lzma *options = f->options;
            ADD_FIELD(options, lc);
            ADD_FIELD(options, lp);
            ADD_FIELD(options, pb);
            ADD_FIELD(options, dict_size);
            break;
        }
        case LZMA_FILTER_LZMA2: {
            lzma_options_lzma *options = f->options;
            ADD_FIELD(options, dict_size);
            break;
        }
        case LZMA_FILTER_DELTA: {
            lzma_options_delta *options = f->options;
            ADD_FIELD(options, dist);
            break;
        }
        case LZMA_FILTER_X86:
        case LZMA_FILTER_POWERPC:
        case LZMA_FILTER_IA64:
        case LZMA_FILTER_ARM:
        case LZMA_FILTER_ARMTHUMB:
        case LZMA_FILTER_SPARC: {
            lzma_options_bcj *options = f->options;
            ADD_FIELD(options, start_offset);
            break;
        }
        default:
            PyErr_Format(PyExc_ValueError, "Invalid filter ID: %llu", f->id);
            goto error;
    }

#undef ADD_FIELD

    return spec;

error:
    Py_DECREF(spec);
    return NULL;
}


/*[clinic input]
module _lzma
class _lzma.LZMACompressor "Compressor *" "&Compressor_type"
class _lzma.LZMADecompressor "Decompressor *" "&Decompressor_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2c14bbe05ff0c147]*/

#include "clinic/_lzmamodule.c.h"

/*[python input]

class lzma_vli_converter(CConverter):
    type = 'lzma_vli'
    converter = 'lzma_vli_converter'

class lzma_filter_converter(CConverter):
    type = 'lzma_filter'
    converter = 'lzma_filter_converter'
    c_default = c_ignored_default = "{LZMA_VLI_UNKNOWN, NULL}"

    def cleanup(self):
        name = ensure_legal_c_identifier(self.name)
        return ('if (%(name)s.id != LZMA_VLI_UNKNOWN)\n'
                '   PyMem_Free(%(name)s.options);\n') % {'name': name}

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=74fe7631ce377a94]*/


/* LZMACompressor class. */

static PyObject *
compress(Compressor *c, uint8_t *data, size_t len, lzma_action action)
{
    Py_ssize_t data_size = 0;
    PyObject *result;

    result = PyBytes_FromStringAndSize(NULL, INITIAL_BUFFER_SIZE);
    if (result == NULL)
        return NULL;
    c->lzs.next_in = data;
    c->lzs.avail_in = len;
    c->lzs.next_out = (uint8_t *)PyBytes_AS_STRING(result);
    c->lzs.avail_out = PyBytes_GET_SIZE(result);
    for (;;) {
        lzma_ret lzret;

        Py_BEGIN_ALLOW_THREADS
        lzret = lzma_code(&c->lzs, action);
        data_size = (char *)c->lzs.next_out - PyBytes_AS_STRING(result);
        Py_END_ALLOW_THREADS
        if (catch_lzma_error(lzret))
            goto error;
        if ((action == LZMA_RUN && c->lzs.avail_in == 0) ||
            (action == LZMA_FINISH && lzret == LZMA_STREAM_END)) {
            break;
        } else if (c->lzs.avail_out == 0) {
            if (grow_buffer(&result, -1) == -1)
                goto error;
            c->lzs.next_out = (uint8_t *)PyBytes_AS_STRING(result) + data_size;
            c->lzs.avail_out = PyBytes_GET_SIZE(result) - data_size;
        }
    }
    if (data_size != PyBytes_GET_SIZE(result))
        if (_PyBytes_Resize(&result, data_size) == -1)
            goto error;
    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
_lzma.LZMACompressor.compress

    self: self(type="Compressor *")
    data: Py_buffer
    /

Provide data to the compressor object.

Returns a chunk of compressed data if possible, or b'' otherwise.

When you have finished providing data to the compressor, call the
flush() method to finish the compression process.
[clinic start generated code]*/

static PyObject *
_lzma_LZMACompressor_compress_impl(Compressor *self, Py_buffer *data)
/*[clinic end generated code: output=31f615136963e00f input=8b60cb13e0ce6420]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed)
        PyErr_SetString(PyExc_ValueError, "Compressor has been flushed");
    else
        result = compress(self, data->buf, data->len, LZMA_RUN);
    RELEASE_LOCK(self);
    return result;
}

/*[clinic input]
_lzma.LZMACompressor.flush

    self: self(type="Compressor *")

Finish the compression process.

Returns the compressed data left in internal buffers.

The compressor object may not be used after this method is called.
[clinic start generated code]*/

static PyObject *
_lzma_LZMACompressor_flush_impl(Compressor *self)
/*[clinic end generated code: output=fec21f3e22504f50 input=3060fb26f9b4042c]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed) {
        PyErr_SetString(PyExc_ValueError, "Repeated call to flush()");
    } else {
        self->flushed = 1;
        result = compress(self, NULL, 0, LZMA_FINISH);
    }
    RELEASE_LOCK(self);
    return result;
}

static PyObject *
Compressor_getstate(Compressor *self, PyObject *noargs)
{
    PyErr_Format(PyExc_TypeError, "cannot serialize '%s' object",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

static int
Compressor_init_xz(lzma_stream *lzs, int check, uint32_t preset,
                   PyObject *filterspecs)
{
    lzma_ret lzret;

    if (filterspecs == Py_None) {
        lzret = lzma_easy_encoder(lzs, preset, check);
    } else {
        lzma_filter filters[LZMA_FILTERS_MAX + 1];

        if (parse_filter_chain_spec(filters, filterspecs) == -1)
            return -1;
        lzret = lzma_stream_encoder(lzs, filters, check);
        free_filter_chain(filters);
    }
    if (catch_lzma_error(lzret))
        return -1;
    else
        return 0;
}

static int
Compressor_init_alone(lzma_stream *lzs, uint32_t preset, PyObject *filterspecs)
{
    lzma_ret lzret;

    if (filterspecs == Py_None) {
        lzma_options_lzma options;

        if (lzma_lzma_preset(&options, preset)) {
            PyErr_Format(Error, "Invalid compression preset: %d", preset);
            return -1;
        }
        lzret = lzma_alone_encoder(lzs, &options);
    } else {
        lzma_filter filters[LZMA_FILTERS_MAX + 1];

        if (parse_filter_chain_spec(filters, filterspecs) == -1)
            return -1;
        if (filters[0].id == LZMA_FILTER_LZMA1 &&
            filters[1].id == LZMA_VLI_UNKNOWN) {
            lzret = lzma_alone_encoder(lzs, filters[0].options);
        } else {
            PyErr_SetString(PyExc_ValueError,
                            "Invalid filter chain for FORMAT_ALONE - "
                            "must be a single LZMA1 filter");
            lzret = LZMA_PROG_ERROR;
        }
        free_filter_chain(filters);
    }
    if (PyErr_Occurred() || catch_lzma_error(lzret))
        return -1;
    else
        return 0;
}

static int
Compressor_init_raw(lzma_stream *lzs, PyObject *filterspecs)
{
    lzma_filter filters[LZMA_FILTERS_MAX + 1];
    lzma_ret lzret;

    if (filterspecs == Py_None) {
        PyErr_SetString(PyExc_ValueError,
                        "Must specify filters for FORMAT_RAW");
        return -1;
    }
    if (parse_filter_chain_spec(filters, filterspecs) == -1)
        return -1;
    lzret = lzma_raw_encoder(lzs, filters);
    free_filter_chain(filters);
    if (catch_lzma_error(lzret))
        return -1;
    else
        return 0;
}

/*[-clinic input]
_lzma.LZMACompressor.__init__

    self: self(type="Compressor *")
    format: int(c_default="FORMAT_XZ") = FORMAT_XZ
        The container format to use for the output.  This can
        be FORMAT_XZ (default), FORMAT_ALONE, or FORMAT_RAW.

    check: int(c_default="-1") = unspecified
        The integrity check to use.  For FORMAT_XZ, the default
        is CHECK_CRC64.  FORMAT_ALONE and FORMAT_RAW do not suport integrity
        checks; for these formats, check must be omitted, or be CHECK_NONE.

    preset: object = None
        If provided should be an integer in the range 0-9, optionally
        OR-ed with the constant PRESET_EXTREME.

    filters: object = None
        If provided should be a sequence of dicts.  Each dict should
        have an entry for "id" indicating the ID of the filter, plus
        additional entries for options to the filter.

Create a compressor object for compressing data incrementally.

The settings used by the compressor can be specified either as a
preset compression level (with the 'preset' argument), or in detail
as a custom filter chain (with the 'filters' argument).  For FORMAT_XZ
and FORMAT_ALONE, the default is to use the PRESET_DEFAULT preset
level.  For FORMAT_RAW, the caller must always specify a filter chain;
the raw compressor does not support preset compression levels.

For one-shot compression, use the compress() function instead.
[-clinic start generated code]*/
static int
Compressor_init(Compressor *self, PyObject *args, PyObject *kwargs)
{
    static char *arg_names[] = {"format", "check", "preset", "filters", NULL};
    int format = FORMAT_XZ;
    int check = -1;
    uint32_t preset = LZMA_PRESET_DEFAULT;
    PyObject *preset_obj = Py_None;
    PyObject *filterspecs = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "|iiOO:LZMACompressor", arg_names,
                                     &format, &check, &preset_obj,
                                     &filterspecs))
        return -1;

    if (format != FORMAT_XZ && check != -1 && check != LZMA_CHECK_NONE) {
        PyErr_SetString(PyExc_ValueError,
                        "Integrity checks are only supported by FORMAT_XZ");
        return -1;
    }

    if (preset_obj != Py_None && filterspecs != Py_None) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot specify both preset and filter chain");
        return -1;
    }

    if (preset_obj != Py_None)
        if (!uint32_converter(preset_obj, &preset))
            return -1;

    self->alloc.opaque = NULL;
    self->alloc.alloc = PyLzma_Malloc;
    self->alloc.free = PyLzma_Free;
    self->lzs.allocator = &self->alloc;

#ifdef WITH_THREAD
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return -1;
    }
#endif

    self->flushed = 0;
    switch (format) {
        case FORMAT_XZ:
            if (check == -1)
                check = LZMA_CHECK_CRC64;
            if (Compressor_init_xz(&self->lzs, check, preset, filterspecs) != 0)
                break;
            return 0;

        case FORMAT_ALONE:
            if (Compressor_init_alone(&self->lzs, preset, filterspecs) != 0)
                break;
            return 0;

        case FORMAT_RAW:
            if (Compressor_init_raw(&self->lzs, filterspecs) != 0)
                break;
            return 0;

        default:
            PyErr_Format(PyExc_ValueError,
                         "Invalid container format: %d", format);
            break;
    }

#ifdef WITH_THREAD
    PyThread_free_lock(self->lock);
    self->lock = NULL;
#endif
    return -1;
}

static void
Compressor_dealloc(Compressor *self)
{
    lzma_end(&self->lzs);
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef Compressor_methods[] = {
    _LZMA_LZMACOMPRESSOR_COMPRESS_METHODDEF
    _LZMA_LZMACOMPRESSOR_FLUSH_METHODDEF
    {"__getstate__", (PyCFunction)Compressor_getstate, METH_NOARGS},
    {NULL}
};

PyDoc_STRVAR(Compressor_doc,
"LZMACompressor(format=FORMAT_XZ, check=-1, preset=None, filters=None)\n"
"\n"
"Create a compressor object for compressing data incrementally.\n"
"\n"
"format specifies the container format to use for the output. This can\n"
"be FORMAT_XZ (default), FORMAT_ALONE, or FORMAT_RAW.\n"
"\n"
"check specifies the integrity check to use. For FORMAT_XZ, the default\n"
"is CHECK_CRC64. FORMAT_ALONE and FORMAT_RAW do not suport integrity\n"
"checks; for these formats, check must be omitted, or be CHECK_NONE.\n"
"\n"
"The settings used by the compressor can be specified either as a\n"
"preset compression level (with the 'preset' argument), or in detail\n"
"as a custom filter chain (with the 'filters' argument). For FORMAT_XZ\n"
"and FORMAT_ALONE, the default is to use the PRESET_DEFAULT preset\n"
"level. For FORMAT_RAW, the caller must always specify a filter chain;\n"
"the raw compressor does not support preset compression levels.\n"
"\n"
"preset (if provided) should be an integer in the range 0-9, optionally\n"
"OR-ed with the constant PRESET_EXTREME.\n"
"\n"
"filters (if provided) should be a sequence of dicts. Each dict should\n"
"have an entry for \"id\" indicating the ID of the filter, plus\n"
"additional entries for options to the filter.\n"
"\n"
"For one-shot compression, use the compress() function instead.\n");

static PyTypeObject Compressor_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_lzma.LZMACompressor",             /* tp_name */
    sizeof(Compressor),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)Compressor_dealloc,     /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    Compressor_doc,                     /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    Compressor_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)Compressor_init,          /* tp_init */
    0,                                  /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};


/* LZMADecompressor class. */

/* Decompress data of length d->lzs.avail_in in d->lzs.next_in.  The output
   buffer is allocated dynamically and returned.  At most max_length bytes are
   returned, so some of the input may not be consumed. d->lzs.next_in and
   d->lzs.avail_in are updated to reflect the consumed input. */
static PyObject*
decompress_buf(Decompressor *d, Py_ssize_t max_length)
{
    Py_ssize_t data_size = 0;
    PyObject *result;
    lzma_stream *lzs = &d->lzs;

    if (max_length < 0 || max_length >= INITIAL_BUFFER_SIZE)
        result = PyBytes_FromStringAndSize(NULL, INITIAL_BUFFER_SIZE);
    else
        result = PyBytes_FromStringAndSize(NULL, max_length);
    if (result == NULL)
        return NULL;

    lzs->next_out = (uint8_t *)PyBytes_AS_STRING(result);
    lzs->avail_out = PyBytes_GET_SIZE(result);

    for (;;) {
        lzma_ret lzret;

        Py_BEGIN_ALLOW_THREADS
        lzret = lzma_code(lzs, LZMA_RUN);
        data_size = (char *)lzs->next_out - PyBytes_AS_STRING(result);
        Py_END_ALLOW_THREADS
        if (catch_lzma_error(lzret))
            goto error;
        if (lzret == LZMA_GET_CHECK || lzret == LZMA_NO_CHECK)
            d->check = lzma_get_check(&d->lzs);
        if (lzret == LZMA_STREAM_END) {
            d->eof = 1;
            break;
        } else if (lzs->avail_in == 0) {
            break;
        } else if (lzs->avail_out == 0) {
            if (data_size == max_length)
                break;
            if (grow_buffer(&result, max_length) == -1)
                goto error;
            lzs->next_out = (uint8_t *)PyBytes_AS_STRING(result) + data_size;
            lzs->avail_out = PyBytes_GET_SIZE(result) - data_size;
        }
    }
    if (data_size != PyBytes_GET_SIZE(result))
        if (_PyBytes_Resize(&result, data_size) == -1)
            goto error;

    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

static PyObject *
decompress(Decompressor *d, uint8_t *data, size_t len, Py_ssize_t max_length)
{
    char input_buffer_in_use;
    PyObject *result;
    lzma_stream *lzs = &d->lzs;

    /* Prepend unconsumed input if necessary */
    if (lzs->next_in != NULL) {
        size_t avail_now, avail_total;

        /* Number of bytes we can append to input buffer */
        avail_now = (d->input_buffer + d->input_buffer_size)
            - (lzs->next_in + lzs->avail_in);

        /* Number of bytes we can append if we move existing
           contents to beginning of buffer (overwriting
           consumed input) */
        avail_total = d->input_buffer_size - lzs->avail_in;

        if (avail_total < len) {
            size_t offset = lzs->next_in - d->input_buffer;
            uint8_t *tmp;
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

            lzs->next_in = d->input_buffer + offset;
        }
        else if (avail_now < len) {
            memmove(d->input_buffer, lzs->next_in,
                    lzs->avail_in);
            lzs->next_in = d->input_buffer;
        }
        memcpy((void*)(lzs->next_in + lzs->avail_in), data, len);
        lzs->avail_in += len;
        input_buffer_in_use = 1;
    }
    else {
        lzs->next_in = data;
        lzs->avail_in = len;
        input_buffer_in_use = 0;
    }

    result = decompress_buf(d, max_length);
    if(result == NULL)
        return NULL;

    if (d->eof) {
        d->needs_input = 0;
        if (lzs->avail_in > 0) {
            Py_CLEAR(d->unused_data);
            d->unused_data = PyBytes_FromStringAndSize(
                (char *)lzs->next_in, lzs->avail_in);
            if (d->unused_data == NULL)
                goto error;
        }
    }
    else if (lzs->avail_in == 0) {
        lzs->next_in = NULL;
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
                d->input_buffer_size < lzs->avail_in) {
                PyMem_Free(d->input_buffer);
                d->input_buffer = NULL;
            }

            /* Allocate if necessary */
            if (d->input_buffer == NULL) {
                d->input_buffer = PyMem_Malloc(lzs->avail_in);
                if (d->input_buffer == NULL) {
                    PyErr_SetNone(PyExc_MemoryError);
                    goto error;
                }
                d->input_buffer_size = lzs->avail_in;
            }

            /* Copy tail */
            memcpy(d->input_buffer, lzs->next_in, lzs->avail_in);
            lzs->next_in = d->input_buffer;
        }
    }

    return result;

error:
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
_lzma.LZMADecompressor.decompress

    self: self(type="Decompressor *")
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
_lzma_LZMADecompressor_decompress_impl(Decompressor *self, Py_buffer *data,
                                       Py_ssize_t max_length)
/*[clinic end generated code: output=ef4e20ec7122241d input=f2bb902cc1caf203]*/
{
    PyObject *result = NULL;

    ACQUIRE_LOCK(self);
    if (self->eof)
        PyErr_SetString(PyExc_EOFError, "Already at end of stream");
    else
        result = decompress(self, data->buf, data->len, max_length);
    RELEASE_LOCK(self);
    return result;
}

static PyObject *
Decompressor_getstate(Decompressor *self, PyObject *noargs)
{
    PyErr_Format(PyExc_TypeError, "cannot serialize '%s' object",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

static int
Decompressor_init_raw(lzma_stream *lzs, PyObject *filterspecs)
{
    lzma_filter filters[LZMA_FILTERS_MAX + 1];
    lzma_ret lzret;

    if (parse_filter_chain_spec(filters, filterspecs) == -1)
        return -1;
    lzret = lzma_raw_decoder(lzs, filters);
    free_filter_chain(filters);
    if (catch_lzma_error(lzret))
        return -1;
    else
        return 0;
}

/*[clinic input]
_lzma.LZMADecompressor.__init__

    self: self(type="Decompressor *")
    format: int(c_default="FORMAT_AUTO") = FORMAT_AUTO
        Specifies the container format of the input stream.  If this is
        FORMAT_AUTO (the default), the decompressor will automatically detect
        whether the input is FORMAT_XZ or FORMAT_ALONE.  Streams created with
        FORMAT_RAW cannot be autodetected.

    memlimit: object = None
        Limit the amount of memory used by the decompressor.  This will cause
        decompression to fail if the input cannot be decompressed within the
        given limit.

    filters: object = None
        A custom filter chain.  This argument is required for FORMAT_RAW, and
        not accepted with any other format.  When provided, this should be a
        sequence of dicts, each indicating the ID and options for a single
        filter.

Create a decompressor object for decompressing data incrementally.

For one-shot decompression, use the decompress() function instead.
[clinic start generated code]*/

static int
_lzma_LZMADecompressor___init___impl(Decompressor *self, int format,
                                     PyObject *memlimit, PyObject *filters)
/*[clinic end generated code: output=3e1821f8aa36564c input=458ca6132ef29801]*/
{
    const uint32_t decoder_flags = LZMA_TELL_ANY_CHECK | LZMA_TELL_NO_CHECK;
    uint64_t memlimit_ = UINT64_MAX;
    lzma_ret lzret;

    if (memlimit != Py_None) {
        if (format == FORMAT_RAW) {
            PyErr_SetString(PyExc_ValueError,
                            "Cannot specify memory limit with FORMAT_RAW");
            return -1;
        }
        memlimit_ = PyLong_AsUnsignedLongLong(memlimit);
        if (PyErr_Occurred())
            return -1;
    }

    if (format == FORMAT_RAW && filters == Py_None) {
        PyErr_SetString(PyExc_ValueError,
                        "Must specify filters for FORMAT_RAW");
        return -1;
    } else if (format != FORMAT_RAW && filters != Py_None) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot specify filters except with FORMAT_RAW");
        return -1;
    }

    self->alloc.opaque = NULL;
    self->alloc.alloc = PyLzma_Malloc;
    self->alloc.free = PyLzma_Free;
    self->lzs.allocator = &self->alloc;
    self->lzs.next_in = NULL;

#ifdef WITH_THREAD
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return -1;
    }
#endif

    self->check = LZMA_CHECK_UNKNOWN;
    self->needs_input = 1;
    self->input_buffer = NULL;
    self->input_buffer_size = 0;
    self->unused_data = PyBytes_FromStringAndSize(NULL, 0);
    if (self->unused_data == NULL)
        goto error;

    switch (format) {
        case FORMAT_AUTO:
            lzret = lzma_auto_decoder(&self->lzs, memlimit_, decoder_flags);
            if (catch_lzma_error(lzret))
                break;
            return 0;

        case FORMAT_XZ:
            lzret = lzma_stream_decoder(&self->lzs, memlimit_, decoder_flags);
            if (catch_lzma_error(lzret))
                break;
            return 0;

        case FORMAT_ALONE:
            self->check = LZMA_CHECK_NONE;
            lzret = lzma_alone_decoder(&self->lzs, memlimit_);
            if (catch_lzma_error(lzret))
                break;
            return 0;

        case FORMAT_RAW:
            self->check = LZMA_CHECK_NONE;
            if (Decompressor_init_raw(&self->lzs, filters) == -1)
                break;
            return 0;

        default:
            PyErr_Format(PyExc_ValueError,
                         "Invalid container format: %d", format);
            break;
    }

error:
    Py_CLEAR(self->unused_data);
#ifdef WITH_THREAD
    PyThread_free_lock(self->lock);
    self->lock = NULL;
#endif
    return -1;
}

static void
Decompressor_dealloc(Decompressor *self)
{
    if(self->input_buffer != NULL)
        PyMem_Free(self->input_buffer);

    lzma_end(&self->lzs);
    Py_CLEAR(self->unused_data);
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef Decompressor_methods[] = {
    _LZMA_LZMADECOMPRESSOR_DECOMPRESS_METHODDEF
    {"__getstate__", (PyCFunction)Decompressor_getstate, METH_NOARGS},
    {NULL}
};

PyDoc_STRVAR(Decompressor_check_doc,
"ID of the integrity check used by the input stream.");

PyDoc_STRVAR(Decompressor_eof_doc,
"True if the end-of-stream marker has been reached.");

PyDoc_STRVAR(Decompressor_needs_input_doc,
"True if more input is needed before more decompressed data can be produced.");

PyDoc_STRVAR(Decompressor_unused_data_doc,
"Data found after the end of the compressed stream.");

static PyMemberDef Decompressor_members[] = {
    {"check", T_INT, offsetof(Decompressor, check), READONLY,
     Decompressor_check_doc},
    {"eof", T_BOOL, offsetof(Decompressor, eof), READONLY,
     Decompressor_eof_doc},
    {"needs_input", T_BOOL, offsetof(Decompressor, needs_input), READONLY,
     Decompressor_needs_input_doc},
    {"unused_data", T_OBJECT_EX, offsetof(Decompressor, unused_data), READONLY,
     Decompressor_unused_data_doc},
    {NULL}
};

static PyTypeObject Decompressor_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_lzma.LZMADecompressor",           /* tp_name */
    sizeof(Decompressor),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)Decompressor_dealloc,   /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    _lzma_LZMADecompressor___init____doc__,  /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    Decompressor_methods,               /* tp_methods */
    Decompressor_members,               /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    _lzma_LZMADecompressor___init__,    /* tp_init */
    0,                                  /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};


/* Module-level functions. */

/*[clinic input]
_lzma.is_check_supported
    check_id: int
    /

Test whether the given integrity check is supported.

Always returns True for CHECK_NONE and CHECK_CRC32.
[clinic start generated code]*/

static PyObject *
_lzma_is_check_supported_impl(PyModuleDef *module, int check_id)
/*[clinic end generated code: output=bb828e90e00ad96e input=5518297b97b2318f]*/
{
    return PyBool_FromLong(lzma_check_is_supported(check_id));
}


/*[clinic input]
_lzma._encode_filter_properties
    filter: lzma_filter(c_default="{LZMA_VLI_UNKNOWN, NULL}")
    /

Return a bytes object encoding the options (properties) of the filter specified by *filter* (a dict).

The result does not include the filter ID itself, only the options.
[clinic start generated code]*/

static PyObject *
_lzma__encode_filter_properties_impl(PyModuleDef *module, lzma_filter filter)
/*[clinic end generated code: output=b5fe690acd6b61d1 input=d4c64f1b557c77d4]*/
{
    lzma_ret lzret;
    uint32_t encoded_size;
    PyObject *result = NULL;

    lzret = lzma_properties_size(&encoded_size, &filter);
    if (catch_lzma_error(lzret))
        goto error;

    result = PyBytes_FromStringAndSize(NULL, encoded_size);
    if (result == NULL)
        goto error;

    lzret = lzma_properties_encode(
            &filter, (uint8_t *)PyBytes_AS_STRING(result));
    if (catch_lzma_error(lzret))
        goto error;

    return result;

error:
    Py_XDECREF(result);
    return NULL;
}


/*[clinic input]
_lzma._decode_filter_properties
    filter_id: lzma_vli
    encoded_props: Py_buffer
    /

Return a bytes object encoding the options (properties) of the filter specified by *filter* (a dict).

The result does not include the filter ID itself, only the options.
[clinic start generated code]*/

static PyObject *
_lzma__decode_filter_properties_impl(PyModuleDef *module, lzma_vli filter_id,
                                     Py_buffer *encoded_props)
/*[clinic end generated code: output=af248f570746668b input=246410800782160c]*/
{
    lzma_filter filter;
    lzma_ret lzret;
    PyObject *result = NULL;
    filter.id = filter_id;

    lzret = lzma_properties_decode(
            &filter, NULL, encoded_props->buf, encoded_props->len);
    if (catch_lzma_error(lzret))
        return NULL;

    result = build_filter_spec(&filter);

    /* We use vanilla free() here instead of PyMem_Free() - filter.options was
       allocated by lzma_properties_decode() using the default allocator. */
    free(filter.options);
    return result;
}


/* Module initialization. */

static PyMethodDef module_methods[] = {
    _LZMA_IS_CHECK_SUPPORTED_METHODDEF
    _LZMA__ENCODE_FILTER_PROPERTIES_METHODDEF
    _LZMA__DECODE_FILTER_PROPERTIES_METHODDEF
    {NULL}
};

static PyModuleDef _lzmamodule = {
    PyModuleDef_HEAD_INIT,
    "_lzma",
    NULL,
    -1,
    module_methods,
    NULL,
    NULL,
    NULL,
    NULL,
};

/* Some of our constants are more than 32 bits wide, so PyModule_AddIntConstant
   would not work correctly on platforms with 32-bit longs. */
static int
module_add_int_constant(PyObject *m, const char *name, PY_LONG_LONG value)
{
    PyObject *o = PyLong_FromLongLong(value);
    if (o == NULL)
        return -1;
    if (PyModule_AddObject(m, name, o) == 0)
        return 0;
    Py_DECREF(o);
    return -1;
}

#define ADD_INT_PREFIX_MACRO(m, macro) \
    module_add_int_constant(m, #macro, LZMA_ ## macro)

PyMODINIT_FUNC
PyInit__lzma(void)
{
    PyObject *m;

    empty_tuple = PyTuple_New(0);
    if (empty_tuple == NULL)
        return NULL;

    m = PyModule_Create(&_lzmamodule);
    if (m == NULL)
        return NULL;

    if (PyModule_AddIntMacro(m, FORMAT_AUTO) == -1 ||
        PyModule_AddIntMacro(m, FORMAT_XZ) == -1 ||
        PyModule_AddIntMacro(m, FORMAT_ALONE) == -1 ||
        PyModule_AddIntMacro(m, FORMAT_RAW) == -1 ||
        ADD_INT_PREFIX_MACRO(m, CHECK_NONE) == -1 ||
        ADD_INT_PREFIX_MACRO(m, CHECK_CRC32) == -1 ||
        ADD_INT_PREFIX_MACRO(m, CHECK_CRC64) == -1 ||
        ADD_INT_PREFIX_MACRO(m, CHECK_SHA256) == -1 ||
        ADD_INT_PREFIX_MACRO(m, CHECK_ID_MAX) == -1 ||
        ADD_INT_PREFIX_MACRO(m, CHECK_UNKNOWN) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_LZMA1) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_LZMA2) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_DELTA) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_X86) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_IA64) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_ARM) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_ARMTHUMB) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_SPARC) == -1 ||
        ADD_INT_PREFIX_MACRO(m, FILTER_POWERPC) == -1 ||
        ADD_INT_PREFIX_MACRO(m, MF_HC3) == -1 ||
        ADD_INT_PREFIX_MACRO(m, MF_HC4) == -1 ||
        ADD_INT_PREFIX_MACRO(m, MF_BT2) == -1 ||
        ADD_INT_PREFIX_MACRO(m, MF_BT3) == -1 ||
        ADD_INT_PREFIX_MACRO(m, MF_BT4) == -1 ||
        ADD_INT_PREFIX_MACRO(m, MODE_FAST) == -1 ||
        ADD_INT_PREFIX_MACRO(m, MODE_NORMAL) == -1 ||
        ADD_INT_PREFIX_MACRO(m, PRESET_DEFAULT) == -1 ||
        ADD_INT_PREFIX_MACRO(m, PRESET_EXTREME) == -1)
        return NULL;

    Error = PyErr_NewExceptionWithDoc(
            "_lzma.LZMAError", "Call to liblzma failed.", NULL, NULL);
    if (Error == NULL)
        return NULL;
    Py_INCREF(Error);
    if (PyModule_AddObject(m, "LZMAError", Error) == -1)
        return NULL;

    if (PyType_Ready(&Compressor_type) == -1)
        return NULL;
    Py_INCREF(&Compressor_type);
    if (PyModule_AddObject(m, "LZMACompressor",
                           (PyObject *)&Compressor_type) == -1)
        return NULL;

    if (PyType_Ready(&Decompressor_type) == -1)
        return NULL;
    Py_INCREF(&Decompressor_type);
    if (PyModule_AddObject(m, "LZMADecompressor",
                           (PyObject *)&Decompressor_type) == -1)
        return NULL;

    return m;
}
