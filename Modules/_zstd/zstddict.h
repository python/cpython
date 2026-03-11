/* Low level interface to the Zstandard algorithm & the zstd library. */

#ifndef ZSTD_DICT_H
#define ZSTD_DICT_H

#include <zstd.h>                 // ZSTD_DDict

typedef struct {
    PyObject_HEAD

    /* Reusable compress/decompress dictionary, they are created once and
       can be shared by multiple threads concurrently, since its usage is
       read-only.
       c_dicts is a dict, int(compressionLevel):PyCapsule(ZSTD_CDict*) */
    ZSTD_DDict *d_dict;
    PyObject *c_dicts;

    /* Dictionary content. */
    char *dict_buffer;
    Py_ssize_t dict_len;

    /* Dictionary id */
    uint32_t dict_id;

    /* Lock to protect the digested dictionaries */
    PyMutex lock;
} ZstdDict;

#endif  // !ZSTD_DICT_H
