/*
   _BlocksOutputBuffer is used to maintain an output buffer
   that has unpredictable size. Suitable for compression/decompression
   API (bz2/lzma/zlib) that has stream->next_out and stream->avail_out:

        stream->next_out:  point to the next output position.
        stream->avail_out: the number of available bytes left in the buffer.

   The first blocks are written into a PyBytesWriter, which is returned as is
   when the output fits in them. Further blocks are separate bytes objects,
   joined once by _BlocksOutputBuffer_Finish(), except where the writer is
   known to grow in place (glibc). See OUTPUT_BUFFER_HEAD_SIZE.

   Usage:

   1, Initialize the struct instance like this:
        _BlocksOutputBuffer buffer = {.writer = NULL};
      This zeroes the other members too. _BlocksOutputBuffer_OnError()
      relies on .writer and .list being NULL.

   2, Initialize the buffer use one of these functions:
        _BlocksOutputBuffer_InitAndGrow()
        _BlocksOutputBuffer_InitWithSize()

   3, If (avail_out == 0), grow the buffer:
        _BlocksOutputBuffer_Grow()

   4, Get the current outputted data size:
        _BlocksOutputBuffer_GetDataSize()

   5, Finish the buffer, and return a bytes object:
        _BlocksOutputBuffer_Finish()

   6, Clean up the buffer when an error occurred, including when
      _BlocksOutputBuffer_Finish() returned NULL:
        _BlocksOutputBuffer_OnError()
*/

#ifndef Py_INTERNAL_BLOCKS_OUTPUT_BUFFER_H
#define Py_INTERNAL_BLOCKS_OUTPUT_BUFFER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    // Bytes writer managing output buffer
    PyBytesWriter *writer;
    // Number of whole allocated size
    Py_ssize_t allocated;
    // Max length of the buffer, negative number means unlimited length
    Py_ssize_t max_length;
    // Number of blocks of bytes. Used to calculate next allocation size
    size_t num_blocks;
    // Blocks after the first OUTPUT_BUFFER_HEAD_SIZE bytes, as a list of
    // bytes objects. NULL while all the output is in the writer.
    PyObject *list;
    // Size of the writer's data once blocks are added to the list
    Py_ssize_t head_size;
} _BlocksOutputBuffer;

static const char unable_allocate_msg[] = "Unable to allocate output buffer.";

/* In 32-bit build, the max block size should <= INT32_MAX. */
#define OUTPUT_BUFFER_MAX_BLOCK_SIZE (256*1024*1024)

/* Block size sequence. The first four blocks are the writer's head, see
   OUTPUT_BUFFER_HEAD_SIZE; the tests in test_zlib, test_gzip, test_bz2,
   test_lzma and test_zstd probe the boundaries of these sizes. */
#define OUTPUT_BUFFER_BLOCK_1 (32*1024)
#define OUTPUT_BUFFER_BLOCK_2 (64*1024)
#define OUTPUT_BUFFER_BLOCK_3 (256*1024)
#define OUTPUT_BUFFER_BLOCK_4 (1024*1024)
#define MB (1024*1024)
static const Py_ssize_t BUFFER_BLOCK_SIZE[] =
    { OUTPUT_BUFFER_BLOCK_1, OUTPUT_BUFFER_BLOCK_2, OUTPUT_BUFFER_BLOCK_3,
      OUTPUT_BUFFER_BLOCK_4, 4*MB, 8*MB, 16*MB, 16*MB,
      32*MB, 32*MB, 32*MB, 32*MB, 64*MB, 64*MB, 128*MB, 128*MB,
      OUTPUT_BUFFER_MAX_BLOCK_SIZE };
#undef MB

/* The writer grows to at most this many bytes: the first four blocks of the
   sequence, 1.34 MiB. Up to there, returning the writer without a join
   beats separate blocks; from the 4 MiB block on, the blocks win (measured
   on macOS and Windows, see gh-161478). Later blocks go to the list, and so
   does every block after a larger initial size.

   On macOS and Windows, a large block grown with realloc() lands in memory
   the process has never touched (on Windows, realloc() also copies it), so
   a writer that keeps growing faults in new pages on every call, which
   costs more than writing blocks once and joining them at the end, as the
   code before gh-139877 did. With glibc, realloc() grows a large block in
   place (mremap()), and there growing the writer is faster than joining
   blocks. Everything else gets the blocks: platforms that were not
   measured, and allocators that copy on realloc(): Android's, musl's, and
   mimalloc, the free-threaded build's object allocator (with
   PYTHONMALLOC=mimalloc on macOS, the growing writer regressed just like
   with the system allocator). */
#if defined(__GLIBC__) && !defined(Py_GIL_DISABLED)
#  define OUTPUT_BUFFER_HEAD_SIZE PY_SSIZE_T_MAX
#else
#  define OUTPUT_BUFFER_HEAD_SIZE \
     (OUTPUT_BUFFER_BLOCK_1 + OUTPUT_BUFFER_BLOCK_2 \
      + OUTPUT_BUFFER_BLOCK_3 + OUTPUT_BUFFER_BLOCK_4)
#endif

/* According to the block sizes defined by BUFFER_BLOCK_SIZE, the whole
   allocated size growth step is:
    1   32 KB       +32 KB
    2   96 KB       +64 KB
    3   352 KB      +256 KB
    4   1.34 MB     +1 MB
    5   5.34 MB     +4 MB
    6   13.34 MB    +8 MB
    7   29.34 MB    +16 MB
    8   45.34 MB    +16 MB
    9   77.34 MB    +32 MB
    10  109.34 MB   +32 MB
    11  141.34 MB   +32 MB
    12  173.34 MB   +32 MB
    13  237.34 MB   +64 MB
    14  301.34 MB   +64 MB
    15  429.34 MB   +128 MB
    16  557.34 MB   +128 MB
    17  813.34 MB   +256 MB
    18  1069.34 MB  +256 MB
    19  1325.34 MB  +256 MB
    20  1581.34 MB  +256 MB
    21  1837.34 MB  +256 MB
    22  2093.34 MB  +256 MB
    ...
*/

/* Initialize the buffer, and grow the buffer.

   max_length: Max length of the buffer, -1 for unlimited length.

   On success, return allocated size (>=0)
   On failure, return -1
*/
static inline Py_ssize_t
_BlocksOutputBuffer_InitAndGrow(_BlocksOutputBuffer *buffer,
                                const Py_ssize_t max_length,
                                void **next_out)
{
    Py_ssize_t block_size;

    // ensure .writer was set to NULL
    assert(buffer->writer == NULL);

    // get block size
    if (0 <= max_length && max_length < BUFFER_BLOCK_SIZE[0]) {
        block_size = max_length;
    } else {
        block_size = BUFFER_BLOCK_SIZE[0];
    }

    buffer->writer = PyBytesWriter_Create(block_size);
    if (buffer->writer == NULL) {
        return -1;
    }

    // set variables
    buffer->allocated = block_size;
    buffer->max_length = max_length;
    buffer->num_blocks = 1;
    buffer->list = NULL;
    buffer->head_size = 0;

    *next_out = PyBytesWriter_GetData(buffer->writer);
    return block_size;
}

/* Initialize the buffer, with an initial size.

   Check block size limit in the outer wrapper function. For example, some libs
   accept UINT32_MAX as the maximum block size, then init_size should <= it.

   On success, return allocated size (>=0)
   On failure, return -1
*/
static inline Py_ssize_t
_BlocksOutputBuffer_InitWithSize(_BlocksOutputBuffer *buffer,
                                 const Py_ssize_t init_size,
                                 void **next_out)
{

    // ensure .writer was set to NULL
    assert(buffer->writer == NULL);

    buffer->writer = PyBytesWriter_Create(init_size);
    if (buffer->writer == NULL) {
        return -1;
    }

    // set variables
    buffer->allocated = init_size;
    buffer->max_length = -1;
    buffer->num_blocks = 1;
    buffer->list = NULL;
    buffer->head_size = 0;

    *next_out = PyBytesWriter_GetData(buffer->writer);
    return init_size;
}

/* Grow the buffer. The avail_out must be 0, please check it before calling.

   On success, return allocated size (>=0)
   On failure, return -1
*/
static inline Py_ssize_t
_BlocksOutputBuffer_Grow(_BlocksOutputBuffer *buffer,
                         void **next_out,
                         const Py_ssize_t avail_out)
{
    Py_ssize_t block_size;

    // ensure no gaps in the data
    if (avail_out != 0) {
        PyErr_SetString(PyExc_SystemError,
                        "avail_out is non-zero in _BlocksOutputBuffer_Grow().");
        return -1;
    }

    // get block size
    size_t maxblock = Py_ARRAY_LENGTH(BUFFER_BLOCK_SIZE);
    assert(maxblock >= 1);
    size_t block_index = Py_MIN(buffer->num_blocks, maxblock - 1);
    block_size = BUFFER_BLOCK_SIZE[block_index];

    // check max_length
    if (buffer->max_length >= 0) {
        // if (rest == 0), should not grow the buffer.
        Py_ssize_t rest = buffer->max_length - buffer->allocated;
        assert(rest > 0);

        // block_size of the last block
        if (block_size > rest) {
            block_size = rest;
        }
    }

    // check buffer->allocated overflow
    if (block_size > PY_SSIZE_T_MAX - buffer->allocated) {
        PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
        return -1;
    }

    if (buffer->list == NULL
        && buffer->allocated <= OUTPUT_BUFFER_HEAD_SIZE - block_size)
    {
        // grow the writer
        if (PyBytesWriter_Grow(buffer->writer, block_size)) {
            PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
            return -1;
        }
        char *data = PyBytesWriter_GetData(buffer->writer);
        *next_out = data + buffer->allocated;
    }
    else {
        // add a block to the list
        if (buffer->list == NULL) {
            buffer->list = PyList_New(0);
            if (buffer->list == NULL) {
                PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
                return -1;
            }
            buffer->head_size = buffer->allocated;
        }
        PyObject *block = PyBytes_FromStringAndSize(NULL, block_size);
        if (block == NULL) {
            PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
            return -1;
        }
        if (PyList_Append(buffer->list, block) < 0) {
            Py_DECREF(block);
            PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
            return -1;
        }
        Py_DECREF(block);  // the list holds it
        *next_out = PyBytes_AS_STRING(block);
    }

    // set variables
    buffer->allocated += block_size;
    buffer->num_blocks += 1;
    return block_size;
}

/* Return the current outputted data size. */
static inline Py_ssize_t
_BlocksOutputBuffer_GetDataSize(_BlocksOutputBuffer *buffer,
                                const Py_ssize_t avail_out)
{
    return buffer->allocated - avail_out;
}

/* Finish the buffer.

   Return a bytes object on success
   Return NULL on failure
*/
static inline PyObject *
_BlocksOutputBuffer_Finish(_BlocksOutputBuffer *buffer,
                           const Py_ssize_t avail_out)
{
    PyObject *obj;
    assert(buffer->writer != NULL);
    if (buffer->list == NULL) {
        obj = PyBytesWriter_FinishWithSize(buffer->writer,
                                           buffer->allocated - avail_out);
        buffer->writer = NULL;
        return obj;
    }

    Py_ssize_t num = PyList_GET_SIZE(buffer->list);
    if (num == 1
        && avail_out == PyBytes_GET_SIZE(PyList_GET_ITEM(buffer->list, 0)))
    {
        // the one block was never written to: the writer holds all the data
        obj = PyBytesWriter_FinishWithSize(buffer->writer, buffer->head_size);
        buffer->writer = NULL;
        Py_CLEAR(buffer->list);
        return obj;
    }

    // join the writer's data and the blocks into one bytes object
    Py_ssize_t size = buffer->allocated - avail_out;
    obj = PyBytes_FromStringAndSize(NULL, size);
    if (obj == NULL) {
        PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
        return NULL;  // the caller calls _BlocksOutputBuffer_OnError()
    }
    char *dst = PyBytes_AS_STRING(obj);
    memcpy(dst, PyBytesWriter_GetData(buffer->writer), buffer->head_size);
    dst += buffer->head_size;

    for (Py_ssize_t i = 0; i < num; i++) {
        PyObject *block = PyList_GET_ITEM(buffer->list, i);
        Py_ssize_t block_size = PyBytes_GET_SIZE(block);
        if (i == num - 1) {
            // the last block holds avail_out unused bytes
            block_size -= avail_out;
        }
        memcpy(dst, PyBytes_AS_STRING(block), block_size);
        dst += block_size;
    }
    assert(dst == PyBytes_AS_STRING(obj) + size);

    PyBytesWriter_Discard(buffer->writer);
    buffer->writer = NULL;
    Py_CLEAR(buffer->list);
    return obj;
}

/* Clean up the buffer when an error occurred. */
static inline void
_BlocksOutputBuffer_OnError(_BlocksOutputBuffer *buffer)
{
    PyBytesWriter_Discard(buffer->writer);
    buffer->writer = NULL;
    Py_CLEAR(buffer->list);
}

#ifdef __cplusplus
}
#endif
#endif /* Py_INTERNAL_BLOCKS_OUTPUT_BUFFER_H */
