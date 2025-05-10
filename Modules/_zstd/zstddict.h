/*
Low level interface to Meta's zstd library for use in the compression.zstd
Python module.
*/

#ifndef ZSTD_DICT_H
#define ZSTD_DICT_H

#include <stdbool.h>              // bool
#include <zstd.h>                 // ZSTD_DDict

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
    bool initialized;
} ZstdDict;

#endif  // !ZSTD_DICT_H
