#pragma once
/*
Low level interface to Meta's zstd library for use in the compression.zstd
Python module.
*/

/* Declarations shared between different parts of the _zstd module*/

#include "Python.h"

#include "zstd.h"
#include "zdict.h"


/* Forward declaration of module state */
typedef struct _zstd_state _zstd_state;

/* Forward reference of module def */
extern PyModuleDef _zstdmodule;

/* For clinic type calculations */
static inline _zstd_state *
get_zstd_state_from_type(PyTypeObject *type) {
    PyObject *module = PyType_GetModuleByDef(type, &_zstdmodule);
    if (module == NULL) {
        return NULL;
    }
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_zstd_state *)state;
}

extern PyType_Spec zstddict_type_spec;
extern PyType_Spec zstdcompressor_type_spec;
extern PyType_Spec ZstdDecompressor_type_spec;

struct _zstd_state {
    PyObject *empty_bytes;
    PyObject *empty_readonly_memoryview;
    PyObject *str_read;
    PyObject *str_readinto;
    PyObject *str_write;
    PyObject *str_flush;

    PyTypeObject *ZstdDict_type;
    PyTypeObject *ZstdCompressor_type;
    PyTypeObject *ZstdDecompressor_type;
    PyObject *ZstdError;

    PyTypeObject *CParameter_type;
    PyTypeObject *DParameter_type;
};

typedef struct {
    PyObject_HEAD

    /* Reusable compress/decompress dictionary, they are created once and
       can be shared by multiple threads concurrently, since its usage is
       read-only.
       c_dicts is a dict, int(compressionLevel):PyCapsule(ZSTD_CDict*) */
    ZSTD_DDict *d_dict;
    PyObject *c_dicts;

    /* Content of the dictionary, bytes object. */
    PyObject *dict_content;
    /* Dictionary id */
    uint32_t dict_id;

    /* __init__ has been called, 0 or 1. */
    int inited;
} ZstdDict;

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

    /* __init__ has been called, 0 or 1. */
    int inited;
} ZstdCompressor;

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
    char needs_input;

    /* For decompress(), 0 or 1.
       1 when both input and output streams are at a frame edge, means a
       frame is completely decoded and fully flushed, or the decompressor
       just be initialized. */
    char at_frame_edge;

    /* For ZstdDecompressor, 0 or 1.
       1 means the end of the first frame has been reached. */
    char eof;

    /* Used for fast reset above three variables */
    char _unused_char_for_align;

    /* __init__ has been called, 0 or 1. */
    int inited;
} ZstdDecompressor;

typedef enum {
    TYPE_DECOMPRESSOR,          // <D>, ZstdDecompressor class
    TYPE_ENDLESS_DECOMPRESSOR,  // <E>, decompress() function
} decompress_type;

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
    ERR_FINALIZE_DICT
} error_type;

typedef enum {
    DICT_TYPE_DIGESTED = 0,
    DICT_TYPE_UNDIGESTED = 1,
    DICT_TYPE_PREFIX = 2
} dictionary_type;

static inline int
mt_continue_should_break(ZSTD_inBuffer *in, ZSTD_outBuffer *out) {
    return in->size == in->pos && out->size != out->pos;
}

/* Format error message and set ZstdError. */
extern void
set_zstd_error(const _zstd_state* const state,
               const error_type type, size_t zstd_ret);

extern void
set_parameter_error(const _zstd_state* const state, int is_compress,
                    int key_v, int value_v);

static const char init_twice_msg[] = "__init__ method is called twice.";

extern PyObject *
decompress_impl(ZstdDecompressor *self, ZSTD_inBuffer *in,
                Py_ssize_t max_length,
                Py_ssize_t initial_size,
                decompress_type type);

extern PyObject *
compress_impl(ZstdCompressor *self, Py_buffer *data,
              ZSTD_EndDirective end_directive);
