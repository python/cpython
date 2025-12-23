/* Low level interface to the Zstandard algorithm & the zstd library. */

#ifndef ZSTD_BUFFER_H
#define ZSTD_BUFFER_H

#include "pycore_blocks_output_buffer.h"

#include <zstd.h>                 // ZSTD_outBuffer

/* Blocks output buffer wrapper code */

/* Initialize the buffer, and grow the buffer.
   Return 0 on success
   Return -1 on failure */
static inline int
_OutputBuffer_InitAndGrow(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob,
                        Py_ssize_t max_length)
{
    /* Ensure .writer was set to NULL */
    assert(buffer->writer == NULL);

    Py_ssize_t res = _BlocksOutputBuffer_InitAndGrow(buffer, max_length,
                                                     &ob->dst);
    if (res < 0) {
        return -1;
    }
    ob->size = (size_t) res;
    ob->pos = 0;
    return 0;
}

/* Initialize the buffer, with an initial size.
    init_size: the initial size.
    Return 0 on success
    Return -1 on failure */
static inline int
_OutputBuffer_InitWithSize(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob,
                            Py_ssize_t max_length, Py_ssize_t init_size)
{
    Py_ssize_t block_size;

    /* Ensure .writer was set to NULL */
    assert(buffer->writer == NULL);

    /* Get block size */
    if (0 <= max_length && max_length < init_size) {
        block_size = max_length;
    }
    else {
        block_size = init_size;
    }

    Py_ssize_t res = _BlocksOutputBuffer_InitWithSize(buffer, block_size,
                                                      &ob->dst);
    if (res < 0) {
        return -1;
    }
    // Set max_length, InitWithSize doesn't do this
    buffer->max_length = max_length;
    ob->size = (size_t) res;
    ob->pos = 0;
    return 0;
}

/* Grow the buffer.
    Return 0 on success
    Return -1 on failure */
static inline int
_OutputBuffer_Grow(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob)
{
    assert(ob->pos == ob->size);
    Py_ssize_t res = _BlocksOutputBuffer_Grow(buffer, &ob->dst, 0);
    if (res < 0) {
        return -1;
    }
    ob->size = (size_t) res;
    ob->pos = 0;
    return 0;
}

/* Finish the buffer.
    Return a bytes object on success
    Return NULL on failure */
static inline PyObject *
_OutputBuffer_Finish(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob)
{
    return _BlocksOutputBuffer_Finish(buffer, ob->size - ob->pos);
}

/* Clean up the buffer */
static inline void
_OutputBuffer_OnError(_BlocksOutputBuffer *buffer)
{
    _BlocksOutputBuffer_OnError(buffer);
}

/* Whether the output data has reached max_length.
The avail_out must be 0, please check it before calling. */
static inline int
_OutputBuffer_ReachedMaxLength(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob)
{
    /* Ensure (data size == allocated size) */
    assert(ob->pos == ob->size);

    return buffer->allocated == buffer->max_length;
}

#endif  // !ZSTD_BUFFER_H
