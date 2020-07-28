/*
   _PyOutputBufferWriter is used to maintain an output buffer
   that has unpredictable size. Suitable for compress/decompress
   API that has stream->next_out and stream->avail_out.

   It maintains a list of bytes object, so there is no overhead
   of resizing the buffer.

   Usage:

   1, Define OBW_SIZE_TYPE as avail_out's type, and include this file:
        #define OBW_SIZE_TYPE uint32_t   // type of avail_out
        #include "output_buffer_writer.h"
        #undef  OBW_SIZE_TYPE

   2, Initialize the writer use one of these functions:
        _PyOutputBufferWriter_Init()
        _PyOutputBufferWriter_InitAndGrow()

   3, Grow the buffer:
        _PyOutputBufferWriter_Grow()

   4, Get the current outputted data size:
        _PyOutputBufferWriter_GetDataSize()

   5, Finish the writer, and return a bytes object:
        _PyOutputBufferWriter_Finish()

   6, Clean up the writer when an error occurred:
        _PyOutputBufferWriter_OnError()
*/


typedef struct {
    /* List of blocks */
    PyObject *list;
    /* Number of whole allocated size. */
    Py_ssize_t allocated;

    /* Max length of the buffer, negative number for unlimited length. */
    Py_ssize_t max_length;
} _PyOutputBufferWriter;


/* Block size sequence. Some compressor/decompressor can't process large
   buffer (>4GB), so the type is int. Below functions assume the type is int.
*/
#define KB (1024)
#define MB (1024*1024)
static const int BUFFER_BLOCK_SIZE[] =
    { 16*KB, 64*KB, 256*KB, 1*MB, 4*MB, 8*MB, 16*MB,
      32*MB, 32*MB, 64*MB, 64*MB, 128*MB, 128*MB, 256*MB };
#undef KB
#undef MB

/* According to the block sizes defined by BUFFER_BLOCK_SIZE, the whole
   allocated size growth step is:
    1   16 KB      +16 KB
    2   80 KB      +64 KB
    3   336 KB     +256 KB
    4   1.32 MB    +1 MB
    5   5.32 MB    +4 MB
    6   13.32 MB   +8 MB
    7   29.32 MB   +16 MB
    8   61.32 MB   +32 MB
    9   93.32 MB   +32 MB
    10  157.32 MB  +64 MB
    11  221.32 MB  +64 MB
    12  349.32 MB  +128 MB
    13  477.32 MB  +128 MB
    14  733.32 MB  +256 MB
    15  989.32 MB  +256 MB
    16  1245.32 MB +256 MB
    17  1501.32 MB +256 MB
    18  1757.32 MB +256 MB
    19  2013.32 MB +256 MB
    ...
*/


/* Initialize the writer.

   max_length: Max length of the buffer, -1 for unlimited length.

   Return 0 on success
   Return -1 on failure
*/
static inline int
_PyOutputBufferWriter_Init(_PyOutputBufferWriter *writer, Py_ssize_t max_length,
                           OBW_SIZE_TYPE *avail_out)
{
    writer->list = PyList_New(0);
    if (writer->list == NULL) {
        return -1;
    }
    writer->allocated = 0;
    writer->max_length = max_length;

    *avail_out = 0;
    return 0;
}


/* Initialize the writer, and grow the buffer.

   max_length: Max length of the buffer, -1 for unlimited length.

   Return 0 on success
   Return -1 on failure
*/
static int
_PyOutputBufferWriter_InitAndGrow(_PyOutputBufferWriter *writer, Py_ssize_t max_length,
                                  uint8_t **next_out, OBW_SIZE_TYPE *avail_out)
{
    PyObject *b;
    int block_size;

    // Set & check max_length
    writer->max_length = max_length;
    if (max_length >= 0 && BUFFER_BLOCK_SIZE[0] > max_length) {
        block_size = (int) max_length;
    } else {
        block_size = BUFFER_BLOCK_SIZE[0];
    }

    // The first block
    b = PyBytes_FromStringAndSize(NULL, block_size);
    if (b == NULL) {
        writer->list = NULL; // For _PyOutputBufferWriter_OnError()
        return -1;
    }

    // Create list
    writer->list = PyList_New(1);
    if (writer->list == NULL) {
        Py_DECREF(b);
        return -1;
    }
    PyList_SET_ITEM(writer->list, 0, b);

    // Set variables
    *next_out = PyBytes_AS_STRING(b);
    *avail_out = block_size;

    writer->allocated = block_size;
    return 0;
}


/* Grow the buffer. The avail_out must be 0, please check it before calling.

   Return 0 on success
   Return -1 on failure
*/
static int
_PyOutputBufferWriter_Grow(_PyOutputBufferWriter *writer,
                           uint8_t **next_out, OBW_SIZE_TYPE *avail_out)
{
    PyObject *b;
    const Py_ssize_t list_len = Py_SIZE(writer->list);
    int block_size;

    // Ensure no gaps in the data
    assert(*avail_out == 0);

    // Get block size
    if (list_len < Py_ARRAY_LENGTH(BUFFER_BLOCK_SIZE)) {
        block_size = BUFFER_BLOCK_SIZE[list_len];
    } else {
        block_size = BUFFER_BLOCK_SIZE[Py_ARRAY_LENGTH(BUFFER_BLOCK_SIZE) - 1];
    }

    // Check max_length
    if (writer->max_length >= 0) {
        // Prevent adding unlimited number of empty bytes to the list.
        if (writer->max_length == 0) {
            assert(*avail_out == 0);
            return 0;
        }
        // block_size of the last block
        if (writer->allocated + block_size > writer->max_length) {
            block_size = (int) (writer->max_length - writer->allocated);
        }
    }

    // Create the block
    b = PyBytes_FromStringAndSize(NULL, block_size);
    if (b == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate output buffer.");
        return -1;
    }
    if (PyList_Append(writer->list, b) < 0) {
        Py_DECREF(b);
        return -1;
    }
    Py_DECREF(b);

    // Set variables
    *next_out = PyBytes_AS_STRING(b);
    *avail_out = block_size;

    writer->allocated += block_size;
    return 0;
}


/* Return the current outputted data size. */
static inline Py_ssize_t
_PyOutputBufferWriter_GetDataSize(_PyOutputBufferWriter *writer, OBW_SIZE_TYPE avail_out)
{
    return writer->allocated - avail_out;
}


/* Finish the writer.

   Return a bytes object on success
   Return NULL on failure
*/
static PyObject *
_PyOutputBufferWriter_Finish(_PyOutputBufferWriter *writer, OBW_SIZE_TYPE avail_out)
{
    PyObject *result, *block;
    int8_t *offset;

    // Allocate bytes object
    result = PyBytes_FromStringAndSize(NULL, writer->allocated - avail_out);
    if (result == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate output buffer.");
        return NULL;
    }
    offset = PyBytes_AS_STRING(result);

    // Copy blocks except the last one
    int i = 0;
    for (; i < Py_SIZE(writer->list)-1; i++) {
        block = PyList_GET_ITEM(writer->list, i);
        memcpy(offset,  PyBytes_AS_STRING(block), Py_SIZE(block));
        offset += Py_SIZE(block);
    }
    // Copy the last block
    block = PyList_GET_ITEM(writer->list, i);
    memcpy(offset, PyBytes_AS_STRING(block), Py_SIZE(block) - avail_out);

    Py_DECREF(writer->list);
    return result;
}


/* clean up the writer. */
static inline void
_PyOutputBufferWriter_OnError(_PyOutputBufferWriter *writer)
{
    Py_XDECREF(writer->list);
}
