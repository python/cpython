/*
Low level interface to Meta's zstd library for use in the compression.zstd
Python module.
*/

/* Declarations shared between different parts of the _zstd module*/

#ifndef ZSTD_MODULE_H
#define ZSTD_MODULE_H
#include "Python.h"

/* Forward declaration of module state */
typedef struct _zstd_state _zstd_state;

/* Forward reference of module def */
extern PyModuleDef _zstdmodule;

/* For clinic type calculations */
static inline _zstd_state *
get_zstd_state_from_type(PyTypeObject *type)
{
    PyObject *module = PyType_GetModuleByDef(type, &_zstdmodule);
    if (module == NULL) {
        return NULL;
    }
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_zstd_state *)state;
}

extern PyType_Spec zstd_dict_type_spec;
extern PyType_Spec zstd_compressor_type_spec;
extern PyType_Spec zstd_decompressor_type_spec;

struct _zstd_state {
    PyTypeObject *ZstdDict_type;
    PyTypeObject *ZstdCompressor_type;
    PyTypeObject *ZstdDecompressor_type;
    PyObject *ZstdError;

    PyTypeObject *CParameter_type;
    PyTypeObject *DParameter_type;
};

typedef enum {
    ERR_DECOMPRESS,
    ERR_COMPRESS,

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

/* Format error message and set ZstdError. */
extern void
set_zstd_error(const _zstd_state* const state,
               const error_type type, size_t zstd_ret);

extern void
set_parameter_error(const _zstd_state* const state, int is_compress,
                    int key_v, int value_v);

#endif
