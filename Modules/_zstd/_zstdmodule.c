/* Low level interface to the Zstandard algorithm & the zstd library. */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include "_zstdmodule.h"

#include <zstd.h>                 // ZSTD_*()
#include <zdict.h>                // ZDICT_*()

/*[clinic input]
module _zstd

[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4b5f5587aac15c14]*/
#include "clinic/_zstdmodule.c.h"


ZstdDict *
_Py_parse_zstd_dict(const _zstd_state *state, PyObject *dict, int *ptype)
{
    if (state == NULL) {
        return NULL;
    }

    /* Check ZstdDict */
    if (PyObject_TypeCheck(dict, state->ZstdDict_type)) {
        return (ZstdDict*)dict;
    }

    /* Check (ZstdDict, type) */
    if (PyTuple_CheckExact(dict) && PyTuple_GET_SIZE(dict) == 2
        && PyObject_TypeCheck(PyTuple_GET_ITEM(dict, 0), state->ZstdDict_type)
        && PyLong_Check(PyTuple_GET_ITEM(dict, 1)))
    {
        int type = PyLong_AsInt(PyTuple_GET_ITEM(dict, 1));
        if (type == -1 && PyErr_Occurred()) {
            return NULL;
        }
        if (type == DICT_TYPE_DIGESTED
            || type == DICT_TYPE_UNDIGESTED
            || type == DICT_TYPE_PREFIX)
        {
            *ptype = type;
            return (ZstdDict*)PyTuple_GET_ITEM(dict, 0);
        }
    }

    /* Wrong type */
    PyErr_SetString(PyExc_TypeError,
                    "zstd_dict argument should be a ZstdDict object.");
    return NULL;
}

/* Format error message and set ZstdError. */
void
set_zstd_error(const _zstd_state *state, error_type type, size_t zstd_ret)
{
    const char *msg;
    assert(ZSTD_isError(zstd_ret));

    if (state == NULL) {
        return;
    }
    switch (type) {
        case ERR_DECOMPRESS:
            msg = "Unable to decompress Zstandard data: %s";
            break;
        case ERR_COMPRESS:
            msg = "Unable to compress Zstandard data: %s";
            break;
        case ERR_SET_PLEDGED_INPUT_SIZE:
            msg = "Unable to set pledged uncompressed content size: %s";
            break;

        case ERR_LOAD_D_DICT:
            msg = "Unable to load Zstandard dictionary or prefix for "
                  "decompression: %s";
            break;
        case ERR_LOAD_C_DICT:
            msg = "Unable to load Zstandard dictionary or prefix for "
                  "compression: %s";
            break;

        case ERR_GET_C_BOUNDS:
            msg = "Unable to get zstd compression parameter bounds: %s";
            break;
        case ERR_GET_D_BOUNDS:
            msg = "Unable to get zstd decompression parameter bounds: %s";
            break;
        case ERR_SET_C_LEVEL:
            msg = "Unable to set zstd compression level: %s";
            break;

        case ERR_TRAIN_DICT:
            msg = "Unable to train the Zstandard dictionary: %s";
            break;
        case ERR_FINALIZE_DICT:
            msg = "Unable to finalize the Zstandard dictionary: %s";
            break;

        default:
            Py_UNREACHABLE();
    }
    PyErr_Format(state->ZstdError, msg, ZSTD_getErrorName(zstd_ret));
}

typedef struct {
    int parameter;
    char parameter_name[32];
} ParameterInfo;

static const ParameterInfo cp_list[] = {
    {ZSTD_c_compressionLevel, "compression_level"},
    {ZSTD_c_windowLog,        "window_log"},
    {ZSTD_c_hashLog,          "hash_log"},
    {ZSTD_c_chainLog,         "chain_log"},
    {ZSTD_c_searchLog,        "search_log"},
    {ZSTD_c_minMatch,         "min_match"},
    {ZSTD_c_targetLength,     "target_length"},
    {ZSTD_c_strategy,         "strategy"},

    {ZSTD_c_enableLongDistanceMatching, "enable_long_distance_matching"},
    {ZSTD_c_ldmHashLog,       "ldm_hash_log"},
    {ZSTD_c_ldmMinMatch,      "ldm_min_match"},
    {ZSTD_c_ldmBucketSizeLog, "ldm_bucket_size_log"},
    {ZSTD_c_ldmHashRateLog,   "ldm_hash_rate_log"},

    {ZSTD_c_contentSizeFlag,  "content_size_flag"},
    {ZSTD_c_checksumFlag,     "checksum_flag"},
    {ZSTD_c_dictIDFlag,       "dict_id_flag"},

    {ZSTD_c_nbWorkers,        "nb_workers"},
    {ZSTD_c_jobSize,          "job_size"},
    {ZSTD_c_overlapLog,       "overlap_log"}
};

static const ParameterInfo dp_list[] = {
    {ZSTD_d_windowLogMax, "window_log_max"}
};

void
set_parameter_error(int is_compress, int key_v, int value_v)
{
    ParameterInfo const *list;
    int list_size;
    char *type;
    ZSTD_bounds bounds;
    char pos_msg[64];

    if (is_compress) {
        list = cp_list;
        list_size = Py_ARRAY_LENGTH(cp_list);
        type = "compression";
    }
    else {
        list = dp_list;
        list_size = Py_ARRAY_LENGTH(dp_list);
        type = "decompression";
    }

    /* Find parameter's name */
    char const *name = NULL;
    for (int i = 0; i < list_size; i++) {
        if (key_v == (list+i)->parameter) {
            name = (list+i)->parameter_name;
            break;
        }
    }

    /* Unknown parameter */
    if (name == NULL) {
        PyOS_snprintf(pos_msg, sizeof(pos_msg),
                      "unknown parameter (key %d)", key_v);
        name = pos_msg;
    }

    /* Get parameter bounds */
    if (is_compress) {
        bounds = ZSTD_cParam_getBounds(key_v);
    }
    else {
        bounds = ZSTD_dParam_getBounds(key_v);
    }
    if (ZSTD_isError(bounds.error)) {
        PyErr_Format(PyExc_ValueError, "invalid %s parameter '%s'",
                     type, name);
        return;
    }

    /* Error message */
    PyErr_Format(PyExc_ValueError,
        "%s parameter '%s' received an illegal value %d; "
        "the valid range is [%d, %d]",
        type, name, value_v, bounds.lowerBound, bounds.upperBound);
}

static inline _zstd_state*
get_zstd_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_zstd_state *)state;
}

static Py_ssize_t
calculate_samples_stats(PyBytesObject *samples_bytes, PyObject *samples_sizes,
                    size_t **chunk_sizes)
{
    Py_ssize_t chunks_number;
    Py_ssize_t sizes_sum;
    Py_ssize_t i;

    chunks_number = PyTuple_GET_SIZE(samples_sizes);
    if ((size_t) chunks_number > UINT32_MAX) {
        PyErr_Format(PyExc_ValueError,
                     "The number of samples should be <= %u.", UINT32_MAX);
        return -1;
    }

    /* Prepare chunk_sizes */
    *chunk_sizes = PyMem_New(size_t, chunks_number);
    if (*chunk_sizes == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    sizes_sum = PyBytes_GET_SIZE(samples_bytes);
    for (i = 0; i < chunks_number; i++) {
        size_t size = PyLong_AsSize_t(PyTuple_GET_ITEM(samples_sizes, i));
        (*chunk_sizes)[i] = size;
        if (size == (size_t)-1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                goto sum_error;
            }
            return -1;
        }
        if ((size_t)sizes_sum < size) {
            goto sum_error;
        }
        sizes_sum -= size;
    }

    if (sizes_sum != 0) {
sum_error:
        PyErr_SetString(PyExc_ValueError,
                        "The samples size tuple doesn't match the "
                        "concatenation's size.");
        return -1;
    }
    return chunks_number;
}


/*[clinic input]
_zstd.train_dict

    samples_bytes: PyBytesObject
        Concatenation of samples.
    samples_sizes: object(subclass_of='&PyTuple_Type')
        Tuple of samples' sizes.
    dict_size: Py_ssize_t
        The size of the dictionary.
    /

Train a Zstandard dictionary on sample data.
[clinic start generated code]*/

static PyObject *
_zstd_train_dict_impl(PyObject *module, PyBytesObject *samples_bytes,
                      PyObject *samples_sizes, Py_ssize_t dict_size)
/*[clinic end generated code: output=8e87fe43935e8f77 input=d20dedb21c72cb62]*/
{
    PyObject *dst_dict_bytes = NULL;
    size_t *chunk_sizes = NULL;
    Py_ssize_t chunks_number;
    size_t zstd_ret;

    /* Check arguments */
    if (dict_size <= 0) {
        PyErr_SetString(PyExc_ValueError,
                        "dict_size argument should be positive number.");
        return NULL;
    }

    /* Check that the samples are valid and get their sizes */
    chunks_number = calculate_samples_stats(samples_bytes, samples_sizes,
                                            &chunk_sizes);
    if (chunks_number < 0) {
        goto error;
    }

    /* Allocate dict buffer */
    dst_dict_bytes = PyBytes_FromStringAndSize(NULL, dict_size);
    if (dst_dict_bytes == NULL) {
        goto error;
    }

    /* Train the dictionary */
    char *dst_dict_buffer = PyBytes_AS_STRING(dst_dict_bytes);
    const char *samples_buffer = PyBytes_AS_STRING(samples_bytes);
    Py_BEGIN_ALLOW_THREADS
    zstd_ret = ZDICT_trainFromBuffer(dst_dict_buffer, dict_size,
                                     samples_buffer,
                                     chunk_sizes, (uint32_t)chunks_number);
    Py_END_ALLOW_THREADS

    /* Check Zstandard dict error */
    if (ZDICT_isError(zstd_ret)) {
        _zstd_state* mod_state = get_zstd_state(module);
        set_zstd_error(mod_state, ERR_TRAIN_DICT, zstd_ret);
        goto error;
    }

    /* Resize dict_buffer */
    if (_PyBytes_Resize(&dst_dict_bytes, zstd_ret) < 0) {
        goto error;
    }

    goto success;

error:
    Py_CLEAR(dst_dict_bytes);

success:
    PyMem_Free(chunk_sizes);
    return dst_dict_bytes;
}

/*[clinic input]
_zstd.finalize_dict

    custom_dict_bytes: PyBytesObject
        Custom dictionary content.
    samples_bytes: PyBytesObject
        Concatenation of samples.
    samples_sizes: object(subclass_of='&PyTuple_Type')
        Tuple of samples' sizes.
    dict_size: Py_ssize_t
        The size of the dictionary.
    compression_level: int
        Optimize for a specific Zstandard compression level, 0 means default.
    /

Finalize a Zstandard dictionary.
[clinic start generated code]*/

static PyObject *
_zstd_finalize_dict_impl(PyObject *module, PyBytesObject *custom_dict_bytes,
                         PyBytesObject *samples_bytes,
                         PyObject *samples_sizes, Py_ssize_t dict_size,
                         int compression_level)
/*[clinic end generated code: output=f91821ba5ae85bda input=3c7e2480aa08fb56]*/
{
    Py_ssize_t chunks_number;
    size_t *chunk_sizes = NULL;
    PyObject *dst_dict_bytes = NULL;
    size_t zstd_ret;
    ZDICT_params_t params;

    /* Check arguments */
    if (dict_size <= 0) {
        PyErr_SetString(PyExc_ValueError,
                        "dict_size argument should be positive number.");
        return NULL;
    }

    /* Check that the samples are valid and get their sizes */
    chunks_number = calculate_samples_stats(samples_bytes, samples_sizes,
                                            &chunk_sizes);
    if (chunks_number < 0) {
        goto error;
    }

    /* Allocate dict buffer */
    dst_dict_bytes = PyBytes_FromStringAndSize(NULL, dict_size);
    if (dst_dict_bytes == NULL) {
        goto error;
    }

    /* Parameters */

    /* Optimize for a specific Zstandard compression level, 0 means default. */
    params.compressionLevel = compression_level;
    /* Write log to stderr, 0 = none. */
    params.notificationLevel = 0;
    /* Force dictID value, 0 means auto mode (32-bits random value). */
    params.dictID = 0;

    /* Finalize the dictionary */
    Py_BEGIN_ALLOW_THREADS
    zstd_ret = ZDICT_finalizeDictionary(
                        PyBytes_AS_STRING(dst_dict_bytes), dict_size,
                        PyBytes_AS_STRING(custom_dict_bytes),
                        Py_SIZE(custom_dict_bytes),
                        PyBytes_AS_STRING(samples_bytes), chunk_sizes,
                        (uint32_t)chunks_number, params);
    Py_END_ALLOW_THREADS

    /* Check Zstandard dict error */
    if (ZDICT_isError(zstd_ret)) {
        _zstd_state* mod_state = get_zstd_state(module);
        set_zstd_error(mod_state, ERR_FINALIZE_DICT, zstd_ret);
        goto error;
    }

    /* Resize dict_buffer */
    if (_PyBytes_Resize(&dst_dict_bytes, zstd_ret) < 0) {
        goto error;
    }

    goto success;

error:
    Py_CLEAR(dst_dict_bytes);

success:
    PyMem_Free(chunk_sizes);
    return dst_dict_bytes;
}


/*[clinic input]
_zstd.get_param_bounds

    parameter: int
        The parameter to get bounds.
    is_compress: bool
        True for CompressionParameter, False for DecompressionParameter.

Get CompressionParameter/DecompressionParameter bounds.
[clinic start generated code]*/

static PyObject *
_zstd_get_param_bounds_impl(PyObject *module, int parameter, int is_compress)
/*[clinic end generated code: output=4acf5a876f0620ca input=45742ef0a3531b65]*/
{
    ZSTD_bounds bound;
    if (is_compress) {
        bound = ZSTD_cParam_getBounds(parameter);
        if (ZSTD_isError(bound.error)) {
            _zstd_state* mod_state = get_zstd_state(module);
            set_zstd_error(mod_state, ERR_GET_C_BOUNDS, bound.error);
            return NULL;
        }
    }
    else {
        bound = ZSTD_dParam_getBounds(parameter);
        if (ZSTD_isError(bound.error)) {
            _zstd_state* mod_state = get_zstd_state(module);
            set_zstd_error(mod_state, ERR_GET_D_BOUNDS, bound.error);
            return NULL;
        }
    }

    return Py_BuildValue("ii", bound.lowerBound, bound.upperBound);
}

/*[clinic input]
@permit_long_summary
_zstd.get_frame_size

    frame_buffer: Py_buffer
        A bytes-like object, it should start from the beginning of a frame,
        and contains at least one complete frame.

Get the size of a Zstandard frame, including the header and optional checksum.
[clinic start generated code]*/

static PyObject *
_zstd_get_frame_size_impl(PyObject *module, Py_buffer *frame_buffer)
/*[clinic end generated code: output=a7384c2f8780f442 input=aac83b33045b5f43]*/
{
    size_t frame_size;

    frame_size = ZSTD_findFrameCompressedSize(frame_buffer->buf,
                                              frame_buffer->len);
    if (ZSTD_isError(frame_size)) {
        _zstd_state* mod_state = get_zstd_state(module);
        PyErr_Format(mod_state->ZstdError,
            "Error when finding the compressed size of a Zstandard frame. "
            "Ensure the frame_buffer argument starts from the "
            "beginning of a frame, and its length is not less than this "
            "complete frame. Zstd error message: %s.",
            ZSTD_getErrorName(frame_size));
        return NULL;
    }

    return PyLong_FromSize_t(frame_size);
}

/*[clinic input]
_zstd.get_frame_info

    frame_buffer: Py_buffer
        A bytes-like object, containing the header of a Zstandard frame.

Get Zstandard frame infomation from a frame header.
[clinic start generated code]*/

static PyObject *
_zstd_get_frame_info_impl(PyObject *module, Py_buffer *frame_buffer)
/*[clinic end generated code: output=56e033cf48001929 input=94b240583ae22ca5]*/
{
    uint64_t decompressed_size;
    uint32_t dict_id;

    /* ZSTD_getFrameContentSize */
    decompressed_size = ZSTD_getFrameContentSize(frame_buffer->buf,
                                                 frame_buffer->len);

    /* #define ZSTD_CONTENTSIZE_UNKNOWN (0ULL - 1)
       #define ZSTD_CONTENTSIZE_ERROR   (0ULL - 2) */
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        _zstd_state* mod_state = get_zstd_state(module);
        PyErr_SetString(mod_state->ZstdError,
            "Error when getting information from the header of "
            "a Zstandard frame. Ensure the frame_buffer argument "
            "starts from the beginning of a frame, and its length "
            "is not less than the frame header (6~18 bytes).");
        return NULL;
    }

    /* ZSTD_getDictID_fromFrame */
    dict_id = ZSTD_getDictID_fromFrame(frame_buffer->buf, frame_buffer->len);

    /* Build tuple */
    if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        return Py_BuildValue("OI", Py_None, dict_id);
    }
    return Py_BuildValue("KI", decompressed_size, dict_id);
}

/*[clinic input]
@permit_long_summary
_zstd.set_parameter_types

    c_parameter_type: object(subclass_of='&PyType_Type')
        CompressionParameter IntEnum type object
    d_parameter_type: object(subclass_of='&PyType_Type')
        DecompressionParameter IntEnum type object

Set CompressionParameter and DecompressionParameter types for validity check.
[clinic start generated code]*/

static PyObject *
_zstd_set_parameter_types_impl(PyObject *module, PyObject *c_parameter_type,
                               PyObject *d_parameter_type)
/*[clinic end generated code: output=f3313b1294f19502 input=0529e918dfe54863]*/
{
    _zstd_state* mod_state = get_zstd_state(module);

    Py_INCREF(c_parameter_type);
    Py_XSETREF(mod_state->CParameter_type, (PyTypeObject*)c_parameter_type);
    Py_INCREF(d_parameter_type);
    Py_XSETREF(mod_state->DParameter_type, (PyTypeObject*)d_parameter_type);

    Py_RETURN_NONE;
}

static PyMethodDef _zstd_methods[] = {
    _ZSTD_TRAIN_DICT_METHODDEF
    _ZSTD_FINALIZE_DICT_METHODDEF
    _ZSTD_GET_PARAM_BOUNDS_METHODDEF
    _ZSTD_GET_FRAME_SIZE_METHODDEF
    _ZSTD_GET_FRAME_INFO_METHODDEF
    _ZSTD_SET_PARAMETER_TYPES_METHODDEF
    {NULL, NULL}
};

static int
_zstd_exec(PyObject *m)
{
#define ADD_TYPE(TYPE, SPEC)                                                 \
do {                                                                         \
    TYPE = (PyTypeObject *)PyType_FromModuleAndSpec(m, &(SPEC), NULL);       \
    if (TYPE == NULL) {                                                      \
        return -1;                                                           \
    }                                                                        \
    if (PyModule_AddType(m, TYPE) < 0) {                                     \
        return -1;                                                           \
    }                                                                        \
} while (0)

#define ADD_INT_MACRO(MACRO)                                                 \
    if (PyModule_AddIntConstant((m), #MACRO, (MACRO)) < 0) {                 \
        return -1;                                                           \
    }

#define ADD_INT_CONST_TO_TYPE(TYPE, NAME, VALUE)                             \
do {                                                                         \
    PyObject *v = PyLong_FromLong((VALUE));                                  \
    if (v == NULL || PyObject_SetAttrString((PyObject *)(TYPE),              \
                                            (NAME), v) < 0) {                \
        Py_XDECREF(v);                                                       \
        return -1;                                                           \
    }                                                                        \
    Py_DECREF(v);                                                            \
} while (0)

    _zstd_state* mod_state = get_zstd_state(m);

    /* Reusable objects & variables */
    mod_state->CParameter_type = NULL;
    mod_state->DParameter_type = NULL;

    /* Create and add heap types */
    ADD_TYPE(mod_state->ZstdDict_type, zstd_dict_type_spec);
    ADD_TYPE(mod_state->ZstdCompressor_type, zstd_compressor_type_spec);
    ADD_TYPE(mod_state->ZstdDecompressor_type, zstd_decompressor_type_spec);
    mod_state->ZstdError = PyErr_NewExceptionWithDoc(
        "compression.zstd.ZstdError",
        "An error occurred in the zstd library.",
        NULL, NULL);
    if (mod_state->ZstdError == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)mod_state->ZstdError) < 0) {
        return -1;
    }

    /* Add constants */
    if (PyModule_AddIntConstant(m, "zstd_version_number",
                                ZSTD_versionNumber()) < 0) {
        return -1;
    }

    if (PyModule_AddStringConstant(m, "zstd_version",
                                   ZSTD_versionString()) < 0) {
        return -1;
    }

#if ZSTD_VERSION_NUMBER >= 10500
    if (PyModule_AddIntConstant(m, "ZSTD_CLEVEL_DEFAULT",
                                ZSTD_defaultCLevel()) < 0) {
        return -1;
    }
#else
    ADD_INT_MACRO(ZSTD_CLEVEL_DEFAULT);
#endif

    if (PyModule_Add(m, "ZSTD_DStreamOutSize",
                     PyLong_FromSize_t(ZSTD_DStreamOutSize())) < 0) {
        return -1;
    }

    /* Add zstd compression parameters. All should also be in cp_list. */
    ADD_INT_MACRO(ZSTD_c_compressionLevel);
    ADD_INT_MACRO(ZSTD_c_windowLog);
    ADD_INT_MACRO(ZSTD_c_hashLog);
    ADD_INT_MACRO(ZSTD_c_chainLog);
    ADD_INT_MACRO(ZSTD_c_searchLog);
    ADD_INT_MACRO(ZSTD_c_minMatch);
    ADD_INT_MACRO(ZSTD_c_targetLength);
    ADD_INT_MACRO(ZSTD_c_strategy);

    ADD_INT_MACRO(ZSTD_c_enableLongDistanceMatching);
    ADD_INT_MACRO(ZSTD_c_ldmHashLog);
    ADD_INT_MACRO(ZSTD_c_ldmMinMatch);
    ADD_INT_MACRO(ZSTD_c_ldmBucketSizeLog);
    ADD_INT_MACRO(ZSTD_c_ldmHashRateLog);

    ADD_INT_MACRO(ZSTD_c_contentSizeFlag);
    ADD_INT_MACRO(ZSTD_c_checksumFlag);
    ADD_INT_MACRO(ZSTD_c_dictIDFlag);

    ADD_INT_MACRO(ZSTD_c_nbWorkers);
    ADD_INT_MACRO(ZSTD_c_jobSize);
    ADD_INT_MACRO(ZSTD_c_overlapLog);

    /* Add zstd decompression parameters. All should also be in dp_list. */
    ADD_INT_MACRO(ZSTD_d_windowLogMax);

    /* Add ZSTD_strategy enum members */
    ADD_INT_MACRO(ZSTD_fast);
    ADD_INT_MACRO(ZSTD_dfast);
    ADD_INT_MACRO(ZSTD_greedy);
    ADD_INT_MACRO(ZSTD_lazy);
    ADD_INT_MACRO(ZSTD_lazy2);
    ADD_INT_MACRO(ZSTD_btlazy2);
    ADD_INT_MACRO(ZSTD_btopt);
    ADD_INT_MACRO(ZSTD_btultra);
    ADD_INT_MACRO(ZSTD_btultra2);

    /* Add ZSTD_EndDirective enum members to ZstdCompressor */
    ADD_INT_CONST_TO_TYPE(mod_state->ZstdCompressor_type,
                          "CONTINUE", ZSTD_e_continue);
    ADD_INT_CONST_TO_TYPE(mod_state->ZstdCompressor_type,
                          "FLUSH_BLOCK", ZSTD_e_flush);
    ADD_INT_CONST_TO_TYPE(mod_state->ZstdCompressor_type,
                          "FLUSH_FRAME", ZSTD_e_end);

    /* Make ZstdCompressor immutable (set Py_TPFLAGS_IMMUTABLETYPE) */
    PyType_Freeze(mod_state->ZstdCompressor_type);

#undef ADD_TYPE
#undef ADD_INT_MACRO
#undef ADD_ZSTD_COMPRESSOR_INT_CONST

    return 0;
}

static int
_zstd_traverse(PyObject *module, visitproc visit, void *arg)
{
    _zstd_state* mod_state = get_zstd_state(module);

    Py_VISIT(mod_state->ZstdDict_type);
    Py_VISIT(mod_state->ZstdCompressor_type);

    Py_VISIT(mod_state->ZstdDecompressor_type);

    Py_VISIT(mod_state->ZstdError);

    Py_VISIT(mod_state->CParameter_type);
    Py_VISIT(mod_state->DParameter_type);
    return 0;
}

static int
_zstd_clear(PyObject *module)
{
    _zstd_state* mod_state = get_zstd_state(module);

    Py_CLEAR(mod_state->ZstdDict_type);
    Py_CLEAR(mod_state->ZstdCompressor_type);

    Py_CLEAR(mod_state->ZstdDecompressor_type);

    Py_CLEAR(mod_state->ZstdError);

    Py_CLEAR(mod_state->CParameter_type);
    Py_CLEAR(mod_state->DParameter_type);
    return 0;
}

static void
_zstd_free(void *module)
{
    (void)_zstd_clear((PyObject *)module);
}

static struct PyModuleDef_Slot _zstd_slots[] = {
    {Py_mod_exec, _zstd_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef _zstdmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_zstd",
    .m_doc = "Implementation module for Zstandard compression.",
    .m_size = sizeof(_zstd_state),
    .m_slots = _zstd_slots,
    .m_methods = _zstd_methods,
    .m_traverse = _zstd_traverse,
    .m_clear = _zstd_clear,
    .m_free = _zstd_free,
};

PyMODINIT_FUNC
PyInit__zstd(void)
{
    return PyModuleDef_Init(&_zstdmodule);
}
