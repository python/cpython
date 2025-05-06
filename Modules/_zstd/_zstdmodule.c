/*
Low level interface to Meta's zstd library for use in the compression.zstd
Python module.
*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "_zstdmodule.h"

/*[clinic input]
module _zstd

[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4b5f5587aac15c14]*/
#include "clinic/_zstdmodule.c.h"


/* Format error message and set ZstdError. */
void
set_zstd_error(const _zstd_state* const state,
               error_type type, size_t zstd_ret)
{
    char *msg;
    assert(ZSTD_isError(zstd_ret));

    switch (type)
    {
    case ERR_DECOMPRESS:
        msg = "Unable to decompress zstd data: %s";
        break;
    case ERR_COMPRESS:
        msg = "Unable to compress zstd data: %s";
        break;
    case ERR_SET_PLEDGED_INPUT_SIZE:
        msg = "Unable to set pledged uncompressed content size: %s";
        break;

    case ERR_LOAD_D_DICT:
        msg = "Unable to load zstd dictionary or prefix for decompression: %s";
        break;
    case ERR_LOAD_C_DICT:
        msg = "Unable to load zstd dictionary or prefix for compression: %s";
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
        msg = "Unable to train zstd dictionary: %s";
        break;
    case ERR_FINALIZE_DICT:
        msg = "Unable to finalize zstd dictionary: %s";
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

static const ParameterInfo cp_list[] =
{
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

static const ParameterInfo dp_list[] =
{
    {ZSTD_d_windowLogMax, "window_log_max"}
};

void
set_parameter_error(const _zstd_state* const state, int is_compress,
                    int key_v, int value_v)
{
    ParameterInfo const *list;
    int list_size;
    char const *name;
    char *type;
    ZSTD_bounds bounds;
    int i;
    char pos_msg[128];

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
    name = NULL;
    for (i = 0; i < list_size; i++) {
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
        PyErr_Format(state->ZstdError,
                     "Zstd %s parameter \"%s\" is invalid. (zstd v%s)",
                     type, name, ZSTD_versionString());
        return;
    }

    /* Error message */
    PyErr_Format(state->ZstdError,
                 "Error when setting zstd %s parameter \"%s\", it "
                 "should %d <= value <= %d, provided value is %d. "
                 "(zstd v%s, %d-bit build)",
                 type, name,
                 bounds.lowerBound, bounds.upperBound, value_v,
                 ZSTD_versionString(), 8*(int)sizeof(Py_ssize_t));
}

static inline _zstd_state*
get_zstd_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_zstd_state *)state;
}


/*[clinic input]
_zstd._train_dict

    samples_bytes: PyBytesObject
        Concatenation of samples.
    samples_sizes: object(subclass_of='&PyTuple_Type')
        Tuple of samples' sizes.
    dict_size: Py_ssize_t
        The size of the dictionary.
    /

Internal function, train a zstd dictionary on sample data.
[clinic start generated code]*/

static PyObject *
_zstd__train_dict_impl(PyObject *module, PyBytesObject *samples_bytes,
                       PyObject *samples_sizes, Py_ssize_t dict_size)
/*[clinic end generated code: output=b5b4f36347c0addd input=2dce5b57d63923e2]*/
{
    // TODO(emmatyping): The preamble and suffix to this function and _finalize_dict
    // are pretty similar. We should see if we can refactor them to share that code.
    Py_ssize_t chunks_number;
    size_t *chunk_sizes = NULL;
    PyObject *dst_dict_bytes = NULL;
    size_t zstd_ret;
    Py_ssize_t sizes_sum;
    Py_ssize_t i;

    /* Check arguments */
    if (dict_size <= 0) {
        PyErr_SetString(PyExc_ValueError, "dict_size argument should be positive number.");
        return NULL;
    }

    chunks_number = Py_SIZE(samples_sizes);
    if ((size_t) chunks_number > UINT32_MAX) {
        PyErr_Format(PyExc_ValueError,
                        "The number of samples should be <= %u.", UINT32_MAX);
        return NULL;
    }

    /* Prepare chunk_sizes */
    chunk_sizes = PyMem_New(size_t, chunks_number);
    if (chunk_sizes == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    sizes_sum = 0;
    for (i = 0; i < chunks_number; i++) {
        PyObject *size = PyTuple_GetItem(samples_sizes, i);
        chunk_sizes[i] = PyLong_AsSize_t(size);
        if (chunk_sizes[i] == (size_t)-1 && PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError,
                            "Items in samples_sizes should be an int "
                            "object, with a value between 0 and %u.", SIZE_MAX);
            goto error;
        }
        sizes_sum += chunk_sizes[i];
    }

    if (sizes_sum != Py_SIZE(samples_bytes)) {
        PyErr_SetString(PyExc_ValueError,
                        "The samples size tuple doesn't match the concatenation's size.");
        goto error;
    }

    /* Allocate dict buffer */
    dst_dict_bytes = PyBytes_FromStringAndSize(NULL, dict_size);
    if (dst_dict_bytes == NULL) {
        goto error;
    }

    /* Train the dictionary */
    char *dst_dict_buffer = PyBytes_AS_STRING(dst_dict_bytes);
    char *samples_buffer = PyBytes_AS_STRING(samples_bytes);
    Py_BEGIN_ALLOW_THREADS
    zstd_ret = ZDICT_trainFromBuffer(dst_dict_buffer, dict_size,
                                     samples_buffer,
                                     chunk_sizes, (uint32_t)chunks_number);
    Py_END_ALLOW_THREADS

    /* Check zstd dict error */
    if (ZDICT_isError(zstd_ret)) {
        _zstd_state* const mod_state = get_zstd_state(module);
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
_zstd._finalize_dict

    custom_dict_bytes: PyBytesObject
        Custom dictionary content.
    samples_bytes: PyBytesObject
        Concatenation of samples.
    samples_sizes: object(subclass_of='&PyTuple_Type')
        Tuple of samples' sizes.
    dict_size: Py_ssize_t
        The size of the dictionary.
    compression_level: int
        Optimize for a specific zstd compression level, 0 means default.
    /

Internal function, finalize a zstd dictionary.
[clinic start generated code]*/

static PyObject *
_zstd__finalize_dict_impl(PyObject *module, PyBytesObject *custom_dict_bytes,
                          PyBytesObject *samples_bytes,
                          PyObject *samples_sizes, Py_ssize_t dict_size,
                          int compression_level)
/*[clinic end generated code: output=5dc5b520fddba37f input=8afd42a249078460]*/
{
    Py_ssize_t chunks_number;
    size_t *chunk_sizes = NULL;
    PyObject *dst_dict_bytes = NULL;
    size_t zstd_ret;
    ZDICT_params_t params;
    Py_ssize_t sizes_sum;
    Py_ssize_t i;

    /* Check arguments */
    if (dict_size <= 0) {
        PyErr_SetString(PyExc_ValueError, "dict_size argument should be positive number.");
        return NULL;
    }

    chunks_number = Py_SIZE(samples_sizes);
    if ((size_t) chunks_number > UINT32_MAX) {
        PyErr_Format(PyExc_ValueError,
                        "The number of samples should be <= %u.", UINT32_MAX);
        return NULL;
    }

    /* Prepare chunk_sizes */
    chunk_sizes = PyMem_New(size_t, chunks_number);
    if (chunk_sizes == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    sizes_sum = 0;
    for (i = 0; i < chunks_number; i++) {
        PyObject *size = PyTuple_GetItem(samples_sizes, i);
        chunk_sizes[i] = PyLong_AsSize_t(size);
        if (chunk_sizes[i] == (size_t)-1 && PyErr_Occurred()) {
            PyErr_Format(PyExc_ValueError,
                            "Items in samples_sizes should be an int "
                            "object, with a value between 0 and %u.", SIZE_MAX);
            goto error;
        }
        sizes_sum += chunk_sizes[i];
    }

    if (sizes_sum != Py_SIZE(samples_bytes)) {
        PyErr_SetString(PyExc_ValueError,
                        "The samples size tuple doesn't match the concatenation's size.");
        goto error;
    }

    /* Allocate dict buffer */
    dst_dict_bytes = PyBytes_FromStringAndSize(NULL, dict_size);
    if (dst_dict_bytes == NULL) {
        goto error;
    }

    /* Parameters */

    /* Optimize for a specific zstd compression level, 0 means default. */
    params.compressionLevel = compression_level;
    /* Write log to stderr, 0 = none. */
    params.notificationLevel = 0;
    /* Force dictID value, 0 means auto mode (32-bits random value). */
    params.dictID = 0;

    /* Finalize the dictionary */
    Py_BEGIN_ALLOW_THREADS
    zstd_ret = ZDICT_finalizeDictionary(
                        PyBytes_AS_STRING(dst_dict_bytes), dict_size,
                        PyBytes_AS_STRING(custom_dict_bytes), Py_SIZE(custom_dict_bytes),
                        PyBytes_AS_STRING(samples_bytes), chunk_sizes,
                        (uint32_t)chunks_number, params);
    Py_END_ALLOW_THREADS

    /* Check zstd dict error */
    if (ZDICT_isError(zstd_ret)) {
        _zstd_state* const mod_state = get_zstd_state(module);
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
_zstd._get_param_bounds

    parameter: int
        The parameter to get bounds.
    is_compress: bool
        True for CompressionParameter, False for DecompressionParameter.

Internal function, get CompressionParameter/DecompressionParameter bounds.
[clinic start generated code]*/

static PyObject *
_zstd__get_param_bounds_impl(PyObject *module, int parameter,
                             int is_compress)
/*[clinic end generated code: output=9892cd822f937e79 input=884cd1a01125267d]*/
{
    ZSTD_bounds bound;
    if (is_compress) {
        bound = ZSTD_cParam_getBounds(parameter);
        if (ZSTD_isError(bound.error)) {
            _zstd_state* const mod_state = get_zstd_state(module);
            set_zstd_error(mod_state, ERR_GET_C_BOUNDS, bound.error);
            return NULL;
        }
    }
    else {
        bound = ZSTD_dParam_getBounds(parameter);
        if (ZSTD_isError(bound.error)) {
            _zstd_state* const mod_state = get_zstd_state(module);
            set_zstd_error(mod_state, ERR_GET_D_BOUNDS, bound.error);
            return NULL;
        }
    }

    return Py_BuildValue("ii", bound.lowerBound, bound.upperBound);
}

/*[clinic input]
_zstd.get_frame_size

    frame_buffer: Py_buffer
        A bytes-like object, it should start from the beginning of a frame,
        and contains at least one complete frame.

Get the size of a zstd frame, including frame header and 4-byte checksum if it has one.

It will iterate all blocks' headers within a frame, to accumulate the frame size.
[clinic start generated code]*/

static PyObject *
_zstd_get_frame_size_impl(PyObject *module, Py_buffer *frame_buffer)
/*[clinic end generated code: output=a7384c2f8780f442 input=7d3ad24311893bf3]*/
{
    size_t frame_size;

    frame_size = ZSTD_findFrameCompressedSize(frame_buffer->buf, frame_buffer->len);
    if (ZSTD_isError(frame_size)) {
        _zstd_state* const mod_state = get_zstd_state(module);
        PyErr_Format(mod_state->ZstdError,
            "Error when finding the compressed size of a zstd frame. "
            "Make sure the frame_buffer argument starts from the "
            "beginning of a frame, and its length not less than this "
            "complete frame. Zstd error message: %s.",
            ZSTD_getErrorName(frame_size));
        return NULL;
    }

    return PyLong_FromSize_t(frame_size);
}

/*[clinic input]
_zstd._get_frame_info

    frame_buffer: Py_buffer
        A bytes-like object, containing the header of a zstd frame.

Internal function, get zstd frame infomation from a frame header.
[clinic start generated code]*/

static PyObject *
_zstd__get_frame_info_impl(PyObject *module, Py_buffer *frame_buffer)
/*[clinic end generated code: output=5462855464ecdf81 input=67f1f8e4b7b89c4d]*/
{
    uint64_t decompressed_size;
    uint32_t dict_id;

    /* ZSTD_getFrameContentSize */
    decompressed_size = ZSTD_getFrameContentSize(frame_buffer->buf,
                                                 frame_buffer->len);

    /* #define ZSTD_CONTENTSIZE_UNKNOWN (0ULL - 1)
       #define ZSTD_CONTENTSIZE_ERROR   (0ULL - 2) */
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        _zstd_state* const mod_state = get_zstd_state(module);
        PyErr_SetString(mod_state->ZstdError,
            "Error when getting information from the header of "
            "a zstd frame. Make sure the frame_buffer argument "
            "starts from the beginning of a frame, and its length "
            "not less than the frame header (6~18 bytes).");
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
_zstd._set_parameter_types

    c_parameter_type: object(subclass_of='&PyType_Type')
        CompressionParameter IntEnum type object
    d_parameter_type: object(subclass_of='&PyType_Type')
        DecompressionParameter IntEnum type object

Internal function, set CompressionParameter/DecompressionParameter types for validity check.
[clinic start generated code]*/

static PyObject *
_zstd__set_parameter_types_impl(PyObject *module, PyObject *c_parameter_type,
                                PyObject *d_parameter_type)
/*[clinic end generated code: output=a13d4890ccbd2873 input=4535545d903853d3]*/
{
    _zstd_state* const mod_state = get_zstd_state(module);

    if (!PyType_Check(c_parameter_type) || !PyType_Check(d_parameter_type)) {
        PyErr_SetString(PyExc_ValueError,
                        "The two arguments should be CompressionParameter and "
                        "DecompressionParameter types.");
        return NULL;
    }

    Py_XDECREF(mod_state->CParameter_type);
    Py_INCREF(c_parameter_type);
    mod_state->CParameter_type = (PyTypeObject*)c_parameter_type;

    Py_XDECREF(mod_state->DParameter_type);
    Py_INCREF(d_parameter_type);
    mod_state->DParameter_type = (PyTypeObject*)d_parameter_type;

    Py_RETURN_NONE;
}

static PyMethodDef _zstd_methods[] = {
    _ZSTD__TRAIN_DICT_METHODDEF
    _ZSTD__FINALIZE_DICT_METHODDEF
    _ZSTD__GET_PARAM_BOUNDS_METHODDEF
    _ZSTD_GET_FRAME_SIZE_METHODDEF
    _ZSTD__GET_FRAME_INFO_METHODDEF
    _ZSTD__SET_PARAMETER_TYPES_METHODDEF

    {0}
};


#define ADD_INT_PREFIX_MACRO(module, macro)                           \
    do {                                                              \
        if (PyModule_AddIntConstant(module, "_" #macro, macro) < 0) { \
            return -1;                                                \
        }                                                             \
    } while(0)

static int
add_parameters(PyObject *module)
{
    /* If add new parameters, please also add to cp_list/dp_list above. */

    /* Compression parameters */
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_compressionLevel);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_windowLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_hashLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_chainLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_searchLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_minMatch);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_targetLength);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_strategy);

    ADD_INT_PREFIX_MACRO(module, ZSTD_c_enableLongDistanceMatching);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmHashLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmMinMatch);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmBucketSizeLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmHashRateLog);

    ADD_INT_PREFIX_MACRO(module, ZSTD_c_contentSizeFlag);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_checksumFlag);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_dictIDFlag);

    ADD_INT_PREFIX_MACRO(module, ZSTD_c_nbWorkers);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_jobSize);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_overlapLog);

    /* Decompression parameters */
    ADD_INT_PREFIX_MACRO(module, ZSTD_d_windowLogMax);

    /* ZSTD_strategy enum */
    ADD_INT_PREFIX_MACRO(module, ZSTD_fast);
    ADD_INT_PREFIX_MACRO(module, ZSTD_dfast);
    ADD_INT_PREFIX_MACRO(module, ZSTD_greedy);
    ADD_INT_PREFIX_MACRO(module, ZSTD_lazy);
    ADD_INT_PREFIX_MACRO(module, ZSTD_lazy2);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btlazy2);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btopt);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btultra);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btultra2);

    return 0;
}

static inline PyObject *
get_zstd_version_info(void)
{
    uint32_t ver = ZSTD_versionNumber();
    uint32_t major, minor, release;

    major = ver / 10000;
    minor = (ver / 100) % 100;
    release = ver % 100;

    return Py_BuildValue("III", major, minor, release);
}

static inline int
add_vars_to_module(PyObject *module)
{
    PyObject *obj;

    /* zstd_version, a str. */
    if (PyModule_AddStringConstant(module, "zstd_version",
                                   ZSTD_versionString()) < 0) {
        return -1;
    }

    /* zstd_version_info, a tuple. */
    obj = get_zstd_version_info();
    if (PyModule_AddObjectRef(module, "zstd_version_info", obj) < 0) {
        Py_XDECREF(obj);
        return -1;
    }
    Py_DECREF(obj);

    /* Add zstd parameters */
    if (add_parameters(module) < 0) {
        return -1;
    }

    /* _compressionLevel_values: (default, min, max)
       ZSTD_defaultCLevel() was added in zstd v1.5.0 */
    obj = Py_BuildValue("iii",
#if ZSTD_VERSION_NUMBER < 10500
                        ZSTD_CLEVEL_DEFAULT,
#else
                        ZSTD_defaultCLevel(),
#endif
                        ZSTD_minCLevel(),
                        ZSTD_maxCLevel());
    if (PyModule_AddObjectRef(module,
                           "_compressionLevel_values",
                           obj) < 0) {
        Py_XDECREF(obj);
        return -1;
    }
    Py_DECREF(obj);

    /* _ZSTD_CStreamSizes */
    obj = Py_BuildValue("II",
                        (uint32_t)ZSTD_CStreamInSize(),
                        (uint32_t)ZSTD_CStreamOutSize());
    if (PyModule_AddObjectRef(module, "_ZSTD_CStreamSizes", obj) < 0) {
        Py_XDECREF(obj);
        return -1;
    }
    Py_DECREF(obj);

    /* _ZSTD_DStreamSizes */
    obj = Py_BuildValue("II",
                        (uint32_t)ZSTD_DStreamInSize(),
                        (uint32_t)ZSTD_DStreamOutSize());
    if (PyModule_AddObjectRef(module, "_ZSTD_DStreamSizes", obj) < 0) {
        Py_XDECREF(obj);
        return -1;
    }
    Py_DECREF(obj);

    /* _ZSTD_CONFIG */
    obj = Py_BuildValue("isOOO", 8*(int)sizeof(Py_ssize_t), "c",
                        Py_False,
                        Py_True,
/* User mremap output buffer */
#if defined(HAVE_MREMAP)
                        Py_True
#else
                        Py_False
#endif
                        );
    if (PyModule_AddObjectRef(module, "_ZSTD_CONFIG", obj) < 0) {
        Py_XDECREF(obj);
        return -1;
    }
    Py_DECREF(obj);

    return 0;
}

#define ADD_STR_TO_STATE_MACRO(STR)                        \
    do {                                                   \
        mod_state->str_##STR = PyUnicode_FromString(#STR); \
        if (mod_state->str_##STR == NULL) {                \
            return -1;                                     \
        }                                                  \
    } while(0)

static inline int
add_type_to_module(PyObject *module, const char *name,
                   PyType_Spec *type_spec, PyTypeObject **dest)
{
    PyObject *temp = PyType_FromModuleAndSpec(module, type_spec, NULL);

    if (PyModule_AddObjectRef(module, name, temp) < 0) {
        Py_XDECREF(temp);
        return -1;
    }

    *dest = (PyTypeObject*) temp;

    return 0;
}

static inline int
add_constant_to_type(PyTypeObject *type, const char *name, long value)
{
    PyObject *temp;

    temp = PyLong_FromLong(value);
    if (temp == NULL) {
        return -1;
    }

    int rc = PyObject_SetAttrString((PyObject*) type, name, temp);
    Py_DECREF(temp);
    return rc;
}

static int _zstd_exec(PyObject *module) {
    _zstd_state* const mod_state = get_zstd_state(module);

    /* Reusable objects & variables */
    mod_state->empty_bytes = PyBytes_FromStringAndSize(NULL, 0);
    if (mod_state->empty_bytes == NULL) {
        return -1;
    }

    mod_state->empty_readonly_memoryview =
                PyMemoryView_FromMemory((char*)mod_state, 0, PyBUF_READ);
    if (mod_state->empty_readonly_memoryview == NULL) {
        return -1;
    }

    /* Add str to module state */
    ADD_STR_TO_STATE_MACRO(read);
    ADD_STR_TO_STATE_MACRO(readinto);
    ADD_STR_TO_STATE_MACRO(write);
    ADD_STR_TO_STATE_MACRO(flush);

    mod_state->CParameter_type = NULL;
    mod_state->DParameter_type = NULL;

    /* Add variables to module */
    if (add_vars_to_module(module) < 0) {
        return -1;
    }

    /* ZstdError */
    mod_state->ZstdError = PyErr_NewExceptionWithDoc(
                                  "_zstd.ZstdError",
                                  "Call to the underlying zstd library failed.",
                                  NULL, NULL);
    if (mod_state->ZstdError == NULL) {
        return -1;
    }

    if (PyModule_AddObjectRef(module, "ZstdError", mod_state->ZstdError) < 0) {
        Py_DECREF(mod_state->ZstdError);
        return -1;
    }

    /* ZstdDict */
    if (add_type_to_module(module,
                           "ZstdDict",
                           &zstddict_type_spec,
                           &mod_state->ZstdDict_type) < 0) {
        return -1;
    }

    // ZstdCompressor
    if (add_type_to_module(module,
                           "ZstdCompressor",
                           &zstdcompressor_type_spec,
                           &mod_state->ZstdCompressor_type) < 0) {
        return -1;
    }

    // Add EndDirective enum to ZstdCompressor
    if (add_constant_to_type(mod_state->ZstdCompressor_type,
                             "CONTINUE",
                             ZSTD_e_continue) < 0) {
        return -1;
    }

    if (add_constant_to_type(mod_state->ZstdCompressor_type,
                             "FLUSH_BLOCK",
                             ZSTD_e_flush) < 0) {
        return -1;
    }

    if (add_constant_to_type(mod_state->ZstdCompressor_type,
                             "FLUSH_FRAME",
                             ZSTD_e_end) < 0) {
        return -1;
    }

    // ZstdDecompressor
    if (add_type_to_module(module,
                           "ZstdDecompressor",
                           &ZstdDecompressor_type_spec,
                           &mod_state->ZstdDecompressor_type) < 0) {
        return -1;
    }

    return 0;
}

static int
_zstd_traverse(PyObject *module, visitproc visit, void *arg)
{
    _zstd_state* const mod_state = get_zstd_state(module);

    Py_VISIT(mod_state->empty_bytes);
    Py_VISIT(mod_state->empty_readonly_memoryview);
    Py_VISIT(mod_state->str_read);
    Py_VISIT(mod_state->str_readinto);
    Py_VISIT(mod_state->str_write);
    Py_VISIT(mod_state->str_flush);

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
    _zstd_state* const mod_state = get_zstd_state(module);

    Py_CLEAR(mod_state->empty_bytes);
    Py_CLEAR(mod_state->empty_readonly_memoryview);
    Py_CLEAR(mod_state->str_read);
    Py_CLEAR(mod_state->str_readinto);
    Py_CLEAR(mod_state->str_write);
    Py_CLEAR(mod_state->str_flush);

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
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},

    {0}
};

struct PyModuleDef _zstdmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_zstd",
    .m_size = sizeof(_zstd_state),
    .m_slots = _zstd_slots,
    .m_methods = _zstd_methods,
    .m_traverse = _zstd_traverse,
    .m_clear = _zstd_clear,
    .m_free = _zstd_free
};

PyMODINIT_FUNC
PyInit__zstd(void)
{
    return PyModuleDef_Init(&_zstdmodule);
}
