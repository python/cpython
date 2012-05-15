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
    lzma_stream lzs;
    int flushed;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} Compressor;

typedef struct {
    PyObject_HEAD
    lzma_stream lzs;
    int check;
    char eof;
    PyObject *unused_data;
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

#if BUFSIZ < 8192
#define INITIAL_BUFFER_SIZE 8192
#else
#define INITIAL_BUFFER_SIZE BUFSIZ
#endif

static int
grow_buffer(PyObject **buf)
{
    size_t size = PyBytes_GET_SIZE(*buf);
    return _PyBytes_Resize(buf, size + (size >> 3) + 6);
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

static void *
parse_filter_spec(lzma_filter *f, PyObject *spec)
{
    PyObject *id_obj;

    if (!PyMapping_Check(spec)) {
        PyErr_SetString(PyExc_TypeError,
                        "Filter specifier must be a dict or dict-like object");
        return NULL;
    }
    id_obj = PyMapping_GetItemString(spec, "id");
    if (id_obj == NULL) {
        if (PyErr_ExceptionMatches(PyExc_KeyError))
            PyErr_SetString(PyExc_ValueError,
                            "Filter specifier must have an \"id\" entry");
        return NULL;
    }
    f->id = PyLong_AsUnsignedLongLong(id_obj);
    Py_DECREF(id_obj);
    if (PyErr_Occurred())
        return NULL;

    switch (f->id) {
        case LZMA_FILTER_LZMA1:
        case LZMA_FILTER_LZMA2:
            f->options = parse_filter_spec_lzma(spec);
            return f->options;
        case LZMA_FILTER_DELTA:
            f->options = parse_filter_spec_delta(spec);
            return f->options;
        case LZMA_FILTER_X86:
        case LZMA_FILTER_POWERPC:
        case LZMA_FILTER_IA64:
        case LZMA_FILTER_ARM:
        case LZMA_FILTER_ARMTHUMB:
        case LZMA_FILTER_SPARC:
            f->options = parse_filter_spec_bcj(spec);
            return f->options;
        default:
            PyErr_Format(PyExc_ValueError, "Invalid filter ID: %llu", f->id);
            return NULL;
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
        if (spec == NULL || parse_filter_spec(&filters[i], spec) == NULL)
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


/* LZMACompressor class. */

static PyObject *
compress(Compressor *c, uint8_t *data, size_t len, lzma_action action)
{
    size_t data_size = 0;
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
            if (grow_buffer(&result) == -1)
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

PyDoc_STRVAR(Compressor_compress_doc,
"compress(data) -> bytes\n"
"\n"
"Provide data to the compressor object. Returns a chunk of\n"
"compressed data if possible, or b\"\" otherwise.\n"
"\n"
"When you have finished providing data to the compressor, call the\n"
"flush() method to finish the conversion process.\n");

static PyObject *
Compressor_compress(Compressor *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "y*:compress", &buffer))
        return NULL;

    ACQUIRE_LOCK(self);
    if (self->flushed)
        PyErr_SetString(PyExc_ValueError, "Compressor has been flushed");
    else
        result = compress(self, buffer.buf, buffer.len, LZMA_RUN);
    RELEASE_LOCK(self);
    PyBuffer_Release(&buffer);
    return result;
}

PyDoc_STRVAR(Compressor_flush_doc,
"flush() -> bytes\n"
"\n"
"Finish the compression process. Returns the compressed data left\n"
"in internal buffers.\n"
"\n"
"The compressor object cannot be used after this method is called.\n");

static PyObject *
Compressor_flush(Compressor *self, PyObject *noargs)
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
    {"compress", (PyCFunction)Compressor_compress, METH_VARARGS,
     Compressor_compress_doc},
    {"flush", (PyCFunction)Compressor_flush, METH_NOARGS,
     Compressor_flush_doc},
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

static PyObject *
decompress(Decompressor *d, uint8_t *data, size_t len)
{
    size_t data_size = 0;
    PyObject *result;

    result = PyBytes_FromStringAndSize(NULL, INITIAL_BUFFER_SIZE);
    if (result == NULL)
        return NULL;
    d->lzs.next_in = data;
    d->lzs.avail_in = len;
    d->lzs.next_out = (uint8_t *)PyBytes_AS_STRING(result);
    d->lzs.avail_out = PyBytes_GET_SIZE(result);
    for (;;) {
        lzma_ret lzret;

        Py_BEGIN_ALLOW_THREADS
        lzret = lzma_code(&d->lzs, LZMA_RUN);
        data_size = (char *)d->lzs.next_out - PyBytes_AS_STRING(result);
        Py_END_ALLOW_THREADS
        if (catch_lzma_error(lzret))
            goto error;
        if (lzret == LZMA_GET_CHECK || lzret == LZMA_NO_CHECK)
            d->check = lzma_get_check(&d->lzs);
        if (lzret == LZMA_STREAM_END) {
            d->eof = 1;
            if (d->lzs.avail_in > 0) {
                Py_CLEAR(d->unused_data);
                d->unused_data = PyBytes_FromStringAndSize(
                        (char *)d->lzs.next_in, d->lzs.avail_in);
                if (d->unused_data == NULL)
                    goto error;
            }
            break;
        } else if (d->lzs.avail_in == 0) {
            break;
        } else if (d->lzs.avail_out == 0) {
            if (grow_buffer(&result) == -1)
                goto error;
            d->lzs.next_out = (uint8_t *)PyBytes_AS_STRING(result) + data_size;
            d->lzs.avail_out = PyBytes_GET_SIZE(result) - data_size;
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

PyDoc_STRVAR(Decompressor_decompress_doc,
"decompress(data) -> bytes\n"
"\n"
"Provide data to the decompressor object. Returns a chunk of\n"
"decompressed data if possible, or b\"\" otherwise.\n"
"\n"
"Attempting to decompress data after the end of the stream is\n"
"reached raises an EOFError. Any data found after the end of the\n"
"stream is ignored, and saved in the unused_data attribute.\n");

static PyObject *
Decompressor_decompress(Decompressor *self, PyObject *args)
{
    Py_buffer buffer;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "y*:decompress", &buffer))
        return NULL;

    ACQUIRE_LOCK(self);
    if (self->eof)
        PyErr_SetString(PyExc_EOFError, "Already at end of stream");
    else
        result = decompress(self, buffer.buf, buffer.len);
    RELEASE_LOCK(self);
    PyBuffer_Release(&buffer);
    return result;
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

static int
Decompressor_init(Decompressor *self, PyObject *args, PyObject *kwargs)
{
    static char *arg_names[] = {"format", "memlimit", "filters", NULL};
    const uint32_t decoder_flags = LZMA_TELL_ANY_CHECK | LZMA_TELL_NO_CHECK;
    int format = FORMAT_AUTO;
    uint64_t memlimit = UINT64_MAX;
    PyObject *memlimit_obj = Py_None;
    PyObject *filterspecs = Py_None;
    lzma_ret lzret;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "|iOO:LZMADecompressor", arg_names,
                                     &format, &memlimit_obj, &filterspecs))
        return -1;

    if (memlimit_obj != Py_None) {
        if (format == FORMAT_RAW) {
            PyErr_SetString(PyExc_ValueError,
                            "Cannot specify memory limit with FORMAT_RAW");
            return -1;
        }
        memlimit = PyLong_AsUnsignedLongLong(memlimit_obj);
        if (PyErr_Occurred())
            return -1;
    }

    if (format == FORMAT_RAW && filterspecs == Py_None) {
        PyErr_SetString(PyExc_ValueError,
                        "Must specify filters for FORMAT_RAW");
        return -1;
    } else if (format != FORMAT_RAW && filterspecs != Py_None) {
        PyErr_SetString(PyExc_ValueError,
                        "Cannot specify filters except with FORMAT_RAW");
        return -1;
    }

#ifdef WITH_THREAD
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return -1;
    }
#endif

    self->check = LZMA_CHECK_UNKNOWN;
    self->unused_data = PyBytes_FromStringAndSize(NULL, 0);
    if (self->unused_data == NULL)
        goto error;

    switch (format) {
        case FORMAT_AUTO:
            lzret = lzma_auto_decoder(&self->lzs, memlimit, decoder_flags);
            if (catch_lzma_error(lzret))
                break;
            return 0;

        case FORMAT_XZ:
            lzret = lzma_stream_decoder(&self->lzs, memlimit, decoder_flags);
            if (catch_lzma_error(lzret))
                break;
            return 0;

        case FORMAT_ALONE:
            self->check = LZMA_CHECK_NONE;
            lzret = lzma_alone_decoder(&self->lzs, memlimit);
            if (catch_lzma_error(lzret))
                break;
            return 0;

        case FORMAT_RAW:
            self->check = LZMA_CHECK_NONE;
            if (Decompressor_init_raw(&self->lzs, filterspecs) == -1)
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
    lzma_end(&self->lzs);
    Py_CLEAR(self->unused_data);
#ifdef WITH_THREAD
    if (self->lock != NULL)
        PyThread_free_lock(self->lock);
#endif
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMethodDef Decompressor_methods[] = {
    {"decompress", (PyCFunction)Decompressor_decompress, METH_VARARGS,
     Decompressor_decompress_doc},
    {NULL}
};

PyDoc_STRVAR(Decompressor_check_doc,
"ID of the integrity check used by the input stream.");

PyDoc_STRVAR(Decompressor_eof_doc,
"True if the end-of-stream marker has been reached.");

PyDoc_STRVAR(Decompressor_unused_data_doc,
"Data found after the end of the compressed stream.");

static PyMemberDef Decompressor_members[] = {
    {"check", T_INT, offsetof(Decompressor, check), READONLY,
     Decompressor_check_doc},
    {"eof", T_BOOL, offsetof(Decompressor, eof), READONLY,
     Decompressor_eof_doc},
    {"unused_data", T_OBJECT_EX, offsetof(Decompressor, unused_data), READONLY,
     Decompressor_unused_data_doc},
    {NULL}
};

PyDoc_STRVAR(Decompressor_doc,
"LZMADecompressor(format=FORMAT_AUTO, memlimit=None, filters=None)\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"format specifies the container format of the input stream. If this is\n"
"FORMAT_AUTO (the default), the decompressor will automatically detect\n"
"whether the input is FORMAT_XZ or FORMAT_ALONE. Streams created with\n"
"FORMAT_RAW cannot be autodetected.\n"
"\n"
"memlimit can be specified to limit the amount of memory used by the\n"
"decompressor. This will cause decompression to fail if the input\n"
"cannot be decompressed within the given limit.\n"
"\n"
"filters specifies a custom filter chain. This argument is required for\n"
"FORMAT_RAW, and not accepted with any other format. When provided,\n"
"this should be a sequence of dicts, each indicating the ID and options\n"
"for a single filter.\n"
"\n"
"For one-shot decompression, use the decompress() function instead.\n");

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
    Decompressor_doc,                   /* tp_doc */
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
    (initproc)Decompressor_init,        /* tp_init */
    0,                                  /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};


/* Module-level functions. */

PyDoc_STRVAR(is_check_supported_doc,
"is_check_supported(check_id) -> bool\n"
"\n"
"Test whether the given integrity check is supported.\n"
"\n"
"Always returns True for CHECK_NONE and CHECK_CRC32.\n");

static PyObject *
is_check_supported(PyObject *self, PyObject *args)
{
    int check_id;

    if (!PyArg_ParseTuple(args, "i:is_check_supported", &check_id))
        return NULL;

    return PyBool_FromLong(lzma_check_is_supported(check_id));
}


PyDoc_STRVAR(encode_filter_properties_doc,
"encode_filter_properties(filter) -> bytes\n"
"\n"
"Return a bytes object encoding the options (properties) of the filter\n"
"specified by *filter* (a dict).\n"
"\n"
"The result does not include the filter ID itself, only the options.\n"
"\n"
"This function is primarily of interest to users implementing custom\n"
"file formats.\n");

static PyObject *
encode_filter_properties(PyObject *self, PyObject *args)
{
    PyObject *filterspec;
    lzma_filter filter;
    lzma_ret lzret;
    uint32_t encoded_size;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "O:encode_filter_properties", &filterspec))
        return NULL;

    if (parse_filter_spec(&filter, filterspec) == NULL)
        return NULL;

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

    PyMem_Free(filter.options);
    return result;

error:
    Py_XDECREF(result);
    PyMem_Free(filter.options);
    return NULL;
}


PyDoc_STRVAR(decode_filter_properties_doc,
"decode_filter_properties(filter_id, encoded_props) -> dict\n"
"\n"
"Return a dict describing a filter with ID *filter_id*, and options\n"
"(properties) decoded from the bytes object *encoded_props*.\n"
"\n"
"This function is primarily of interest to users implementing custom\n"
"file formats.\n");

static PyObject *
decode_filter_properties(PyObject *self, PyObject *args)
{
    Py_buffer encoded_props;
    lzma_filter filter;
    lzma_ret lzret;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "O&y*:decode_filter_properties",
                          lzma_vli_converter, &filter.id, &encoded_props))
        return NULL;

    lzret = lzma_properties_decode(
            &filter, NULL, encoded_props.buf, encoded_props.len);
    PyBuffer_Release(&encoded_props);
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
    {"is_check_supported", (PyCFunction)is_check_supported,
     METH_VARARGS, is_check_supported_doc},
    {"encode_filter_properties", (PyCFunction)encode_filter_properties,
     METH_VARARGS, encode_filter_properties_doc},
    {"decode_filter_properties", (PyCFunction)decode_filter_properties,
     METH_VARARGS, decode_filter_properties_doc},
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
