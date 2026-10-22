/* Low level interface to the Zstandard algorithm & the zstd library. */

/* Declarations shared between different parts of the _zstd module*/

#ifndef ZSTD_MODULE_H
#define ZSTD_MODULE_H

#include "zstddict.h"

/* Type specs */
extern PyType_Spec zstd_dict_type_spec;
extern PyType_Spec zstd_compressor_type_spec;
extern PyType_Spec zstd_decompressor_type_spec;

typedef struct {
    /* Module heap types. */
    PyTypeObject *ZstdDict_type;
    PyTypeObject *ZstdCompressor_type;
    PyTypeObject *ZstdDecompressor_type;
    PyObject *ZstdError;

    /* enum types set by set_parameter_types. */
    PyTypeObject *CParameter_type;
    PyTypeObject *DParameter_type;
} _zstd_state;

typedef enum {
    ERR_DECOMPRESS,
    ERR_COMPRESS,
    ERR_SET_PLEDGED_INPUT_SIZE,

    ERR_LOAD_D_DICT,
    ERR_LOAD_C_DICT,

    ERR_GET_C_BOUNDS,
    ERR_GET_D_BOUNDS,
    ERR_SET_C_LEVEL,

    ERR_TRAIN_DICT,
    ERR_FINALIZE_DICT,
} error_type;

typedef enum {
    DICT_TYPE_DIGESTED = 0,
    DICT_TYPE_UNDIGESTED = 1,
    DICT_TYPE_PREFIX = 2
} dictionary_type;

extern ZstdDict *
_Py_parse_zstd_dict(const _zstd_state *state,
                    PyObject *dict, int *type);

/* Format error message and set ZstdError. */
extern void
set_zstd_error(const _zstd_state *state,
               error_type type, size_t zstd_ret);

extern void
set_parameter_error(int is_compress, int key_v, int value_v);

#endif  // !ZSTD_MODULE_H
