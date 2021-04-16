/*
   _BlocksOutputBuffer is used to maintain an output buffer
   that has unpredictable size. Suitable for compression/decompression
   API (bz2/lzma/zlib) that has stream->next_out and stream->avail_out:

        stream->next_out:  point to the next output position.
        stream->avail_out: the number of available bytes left in the buffer.

   It maintains a list of bytes object, so there is no overhead
   of resizing the buffer.

   Usage:

   1, Define BOB_BUFFER_TYPE and BOB_SIZE_TYPE, and include this file:
        #define BOB_BUFFER_TYPE uint8_t  // type of next_out pointer
        #define BOB_SIZE_TYPE   size_t   // type of avail_out
        #include "blocks_output_buffer.h"

   2, Initialize the buffer use one of these functions:
        _BlocksOutputBuffer_Init()
        _BlocksOutputBuffer_InitAndGrow()
        _BlocksOutputBuffer_InitWithSize()

   3, If (avail_out == 0), grow the buffer:
        _BlocksOutputBuffer_Grow()

   4, Get the current outputted data size:
        _BlocksOutputBuffer_GetDataSize()

   5, Finish the buffer, and return a bytes object:
        _BlocksOutputBuffer_Finish()

   6, Clean up the buffer when an error occurred:
        _BlocksOutputBuffer_OnError()
*/

typedef struct {
    // List of bytes objects
    PyObject *list;
    // Number of whole allocated size
    Py_ssize_t allocated;
    // Max length of the buffer, negative number means unlimited length.
    Py_ssize_t max_length;
} _BlocksOutputBuffer;

#if defined(__GNUC__)
#  define UNUSED_ATTR __attribute__((unused))
#else
#  define UNUSED_ATTR
#endif

#if BOB_SIZE_TYPE == uint32_t
    #define BOB_SIZE_MAX UINT32_MAX
#elif BOB_SIZE_TYPE == size_t
    #define BOB_SIZE_MAX UINTPTR_MAX
#else
    #error "Please define BOB_SIZE_MAX to BOB_SIZE_TYPE's max value"
#endif

const char unable_allocate_msg[] = "Unable to allocate output buffer.";

/* Block size sequence.
   In 32-bit build Py_ssize_t is int, so the type is int. Below functions
   assume the type is int.
*/
#define KB (1024)
#define MB (1024*1024)
const int BUFFER_BLOCK_SIZE[] =
    { 32*KB, 64*KB, 256*KB, 1*MB, 4*MB, 8*MB, 16*MB, 16*MB,
      32*MB, 32*MB, 32*MB, 32*MB, 64*MB, 64*MB, 128*MB, 128*MB,
      256*MB };
#undef KB
#undef MB

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

/* Initialize the buffer.

   max_length: Max length of the buffer, -1 for unlimited length.

   Return 0 on success
   Return -1 on failure
*/
static UNUSED_ATTR int
_BlocksOutputBuffer_Init(_BlocksOutputBuffer *buffer, Py_ssize_t max_length,
                         BOB_BUFFER_TYPE **next_out, BOB_SIZE_TYPE *avail_out)
{
    buffer->list = PyList_New(0);
    if (buffer->list == NULL) {
        return -1;
    }
    buffer->allocated = 0;
    buffer->max_length = max_length;

    *next_out = (BOB_BUFFER_TYPE*) 1; // some libs don't accept NULL
    *avail_out = 0;
    return 0;
}

/* Initialize the buffer, and grow the buffer.

   max_length: Max length of the buffer, -1 for unlimited length.

   Return 0 on success
   Return -1 on failure
*/
static UNUSED_ATTR int
_BlocksOutputBuffer_InitAndGrow(_BlocksOutputBuffer *buffer, Py_ssize_t max_length,
                                BOB_BUFFER_TYPE **next_out, BOB_SIZE_TYPE *avail_out)
{
    PyObject *b;
    int block_size;

    // set & check max_length
    buffer->max_length = max_length;
    if (0 <= max_length && max_length < BUFFER_BLOCK_SIZE[0]) {
        block_size = (int) max_length;
    } else {
        block_size = BUFFER_BLOCK_SIZE[0];
    }

    // the first block
    b = PyBytes_FromStringAndSize(NULL, block_size);
    if (b == NULL) {
        buffer->list = NULL; // for _BlocksOutputBuffer_OnError()
        return -1;
    }

    // create the list
    buffer->list = PyList_New(1);
    if (buffer->list == NULL) {
        Py_DECREF(b);
        return -1;
    }
    PyList_SET_ITEM(buffer->list, 0, b);

    // set variables
    buffer->allocated = block_size;

    *next_out = (BOB_BUFFER_TYPE*) PyBytes_AS_STRING(b);
    *avail_out = block_size;
    return 0;
}

/* Initialize the buffer, with an initial size.

   Return 0 on success
   Return -1 on failure
*/
static UNUSED_ATTR int
_BlocksOutputBuffer_InitWithSize(_BlocksOutputBuffer *buffer, Py_ssize_t init_size,
                                 BOB_BUFFER_TYPE **next_out, BOB_SIZE_TYPE *avail_out)
{
    PyObject *b;

    // check block size limit
    if (init_size > BOB_SIZE_MAX) {
        buffer->list = NULL; // for _BlocksOutputBuffer_OnError()

        PyErr_SetString(PyExc_ValueError, "Initial buffer size is too large.");
        return -1;
    }

    // the first block
    b = PyBytes_FromStringAndSize(NULL, init_size);
    if (b == NULL) {
        buffer->list = NULL; // for OutputBuffer_OnError()

        PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
        return -1;
    }

    // create the list
    buffer->list = PyList_New(1);
    if (buffer->list == NULL) {
        Py_DECREF(b);
        return -1;
    }
    PyList_SET_ITEM(buffer->list, 0, b);

    // set variables
    buffer->allocated = init_size;
    buffer->max_length = -1;

    *next_out = (BOB_BUFFER_TYPE*) PyBytes_AS_STRING(b);
    *avail_out = (BOB_SIZE_TYPE) init_size;
    return 0;
}

/* Grow the buffer. The avail_out must be 0, please check it before calling.

   Return 0 on success
   Return -1 on failure
*/
static int
_BlocksOutputBuffer_Grow(_BlocksOutputBuffer *buffer,
                         BOB_BUFFER_TYPE **next_out, BOB_SIZE_TYPE *avail_out)
{
    PyObject *b;
    const Py_ssize_t list_len = Py_SIZE(buffer->list);
    int block_size;

    // ensure no gaps in the data
    if (*avail_out != 0) {
        PyErr_SetString(PyExc_SystemError,
                        "*avail_out is non-zero in _BlocksOutputBuffer_Grow().");
        return -1;
    }

    // get block size
    if (list_len < (Py_ssize_t) Py_ARRAY_LENGTH(BUFFER_BLOCK_SIZE)) {
        block_size = BUFFER_BLOCK_SIZE[list_len];
    } else {
        block_size = BUFFER_BLOCK_SIZE[Py_ARRAY_LENGTH(BUFFER_BLOCK_SIZE) - 1];
    }

    // check max_length
    if (buffer->max_length >= 0) {
        // if (rest == 0), should not grow the buffer.
        Py_ssize_t rest = buffer->max_length - buffer->allocated;
        assert(rest > 0);

        // block_size of the last block
        if (block_size > rest) {
            block_size = (int) rest;
        }
    }

    // check buffer->allocated overflow
    if (block_size > PY_SSIZE_T_MAX - buffer->allocated) {
        PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
        return -1;
    }

    // create the block
    b = PyBytes_FromStringAndSize(NULL, block_size);
    if (b == NULL) {
        PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
        return -1;
    }
    if (PyList_Append(buffer->list, b) < 0) {
        Py_DECREF(b);
        return -1;
    }
    Py_DECREF(b);

    // set variables
    buffer->allocated += block_size;

    *next_out = (BOB_BUFFER_TYPE*) PyBytes_AS_STRING(b);
    *avail_out = block_size;
    return 0;
}

/* Return the current outputted data size. */
static inline Py_ssize_t
_BlocksOutputBuffer_GetDataSize(_BlocksOutputBuffer *buffer, BOB_SIZE_TYPE avail_out)
{
    return buffer->allocated - avail_out;
}

/* Finish the buffer.

   Return a bytes object on success
   Return NULL on failure
*/
static PyObject *
_BlocksOutputBuffer_Finish(_BlocksOutputBuffer *buffer, BOB_SIZE_TYPE avail_out)
{
    PyObject *result, *block;
    const Py_ssize_t list_len = Py_SIZE(buffer->list);

    // fast path for single block
    if ( (list_len == 1 && avail_out == 0) ||
         (list_len == 2 && Py_SIZE(PyList_GET_ITEM(buffer->list, 1)) == (Py_ssize_t) avail_out))
    {
        block = PyList_GET_ITEM(buffer->list, 0);
        Py_INCREF(block);

        Py_CLEAR(buffer->list);
        return block;
    }

    // final bytes object
    result = PyBytes_FromStringAndSize(NULL, buffer->allocated - avail_out);
    if (result == NULL) {
        PyErr_SetString(PyExc_MemoryError, unable_allocate_msg);
        return NULL;
    }

    // memory copy
    if (list_len > 0) {
        char *posi = PyBytes_AS_STRING(result);

        // blocks except the last one
        Py_ssize_t i = 0;
        for (; i < list_len-1; i++) {
            block = PyList_GET_ITEM(buffer->list, i);
            memcpy(posi, PyBytes_AS_STRING(block), Py_SIZE(block));
            posi += Py_SIZE(block);
        }
        // the last block
        block = PyList_GET_ITEM(buffer->list, i);
        memcpy(posi, PyBytes_AS_STRING(block), Py_SIZE(block) - avail_out);
    } else {
        assert(Py_SIZE(result) == 0);
    }

    Py_CLEAR(buffer->list);
    return result;
}

/* Clean up the buffer when an error occurred. */
static inline void
_BlocksOutputBuffer_OnError(_BlocksOutputBuffer *buffer)
{
    Py_CLEAR(buffer->list);
}

#undef BOB_BUFFER_TYPE
#undef BOB_SIZE_TYPE
#undef BOB_SIZE_MAX
