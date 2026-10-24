/******************************************************************************
 * Python Remote Debugging Module - Binary Writer Implementation
 *
 * High-performance binary file writer for profiling data with optional zstd
 * compression.
 ******************************************************************************/

#ifndef Py_BUILD_CORE_MODULE
#  define Py_BUILD_CORE_MODULE
#endif

#include "binary_io.h"
#include "_remote_debugging.h"
#include "pycore_bitutils.h"      // _Py_bit_length(), _Py_popcount32()
#include "pycore_opcode_utils.h"  // MAX_REAL_OPCODE
#include <math.h>
#include <string.h>

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#ifdef _Py_MEMORY_SANITIZER
#  include <sanitizer/msan_interface.h>
#endif

/* ============================================================================
 * CONSTANTS FOR BINARY FORMAT SIZES
 * ============================================================================ */

#define MAX_VARINT_SIZE 10               /* Maximum bytes for a varint64 */
#define MAX_VARINT_SIZE_U32 5            /* Maximum bytes for a varint32 */

#define MAX_BLOCK_HEADER_SIZE \
    (BLOCK_MAGIC_SIZE + MAX_VARINT_SIZE_U32 + MAX_VARINT_SIZE + \
     3 * sizeof(uint8_t) + \
     COLUMN_COUNT * (2 * MAX_VARINT_SIZE_U32 + sizeof(uint8_t)))

#define ZSTD_MIN_SIZE 32
#define STACK_REUSE_MIN_FRAMES 3

#define MAX_SAMPLE_SIZE \
    (STREAM_ENTRY_SIZE + 5 * MAX_VARINT_SIZE + sizeof(uint8_t) + \
     MAX_STACK_DEPTH * sizeof(uint32_t) + sizeof(Repeat))
#define MAX_BITMAP_PADDING (CHAR_BIT - 1)
#define BLOCK_FLUSH_SIZE \
    (MAX_BLOCK_SIZE - MAX_SAMPLE_SIZE - MAX_BITMAP_PADDING)

static int
buffer_append(ByteBuffer *buffer, const void *data, size_t size)
{
    if (buffer->size > MAX_BLOCK_SIZE ||
        size > MAX_BLOCK_SIZE - buffer->size) {
        PyErr_SetString(PyExc_RuntimeError,
                        "sample block buffer exceeds the writer safety limit");
        return -1;
    }
    if (buffer_reserve(buffer, buffer->size + size) < 0) {
        return -1;
    }
    if (size != 0) {
        memcpy(buffer->data + buffer->size, data, size);
    }
    buffer->size += size;
    return 0;
}

static int
buffer_append_varint(ByteBuffer *buffer, uint64_t value)
{
    uint8_t bytes[MAX_VARINT_SIZE];
    size_t size = encode_varint_u64(bytes, value);
    return buffer_append(buffer, bytes, size);
}

static int
column_write(SampleBlock *block, unsigned column, const void *data,
             size_t size)
{
    if (buffer_append(&block->columns[column], data, size) < 0) {
        return -1;
    }
    block->size += size;
    return 0;
}

static int
column_write_varint(SampleBlock *block, unsigned column, uint64_t value)
{
    uint8_t bytes[MAX_VARINT_SIZE];
    size_t size = encode_varint_u64(bytes, value);
    return column_write(block, column, bytes, size);
}

/* Helper macro: convert PyLong to int32, using default_val if conversion fails */
#define PYLONG_TO_INT32_OR_DEFAULT(obj, var, default_val) \
    do { \
        (var) = (int32_t)PyLong_AsLong(obj); \
        if (UNLIKELY(PyErr_Occurred() != NULL)) { \
            PyErr_Clear(); \
            (var) = (default_val); \
        } \
    } while (0)

/* ============================================================================
 * WRITER-SPECIFIC UTILITY HELPERS
 * ============================================================================ */

/* Grow two parallel arrays together (e.g., strings and string_lengths).
 * Returns 0 on success, -1 on error (sets PyErr).
 * On error, original arrays are preserved (truly atomic update). */
static inline int
grow_parallel_arrays(void **array1, void **array2, size_t *capacity,
                     size_t elem_size1, size_t elem_size2)
{
    size_t old_cap = *capacity;

    if (old_cap > SIZE_MAX / 2) {
        PyErr_SetString(PyExc_OverflowError, "Array capacity overflow");
        return -1;
    }
    size_t new_cap = old_cap * 2;

    if (new_cap > SIZE_MAX / elem_size1 || new_cap > SIZE_MAX / elem_size2) {
        PyErr_SetString(PyExc_OverflowError, "Array allocation size overflow");
        return -1;
    }

    size_t new_size1 = new_cap * elem_size1;
    size_t new_size2 = new_cap * elem_size2;
    size_t old_size1 = old_cap * elem_size1;
    size_t old_size2 = old_cap * elem_size2;

    /* Allocate fresh memory blocks (not realloc) to ensure atomicity.
     * If either allocation fails, original arrays are completely unchanged. */
    void *new_array1 = PyMem_Malloc(new_size1);
    if (!new_array1) {
        PyErr_NoMemory();
        return -1;
    }

    void *new_array2 = PyMem_Malloc(new_size2);
    if (!new_array2) {
        /* Second allocation failed - free first and return with no state change */
        PyMem_Free(new_array1);
        PyErr_NoMemory();
        return -1;
    }

    /* Both allocations succeeded - copy data and update pointers atomically */
    memcpy(new_array1, *array1, old_size1);
    memcpy(new_array2, *array2, old_size2);

    PyMem_Free(*array1);
    PyMem_Free(*array2);

    *array1 = new_array1;
    *array2 = new_array2;
    *capacity = new_cap;
    return 0;
}

/* Checked fwrite with GIL release - returns 0 on success, -1 on error (sets PyErr).
 * This version releases the GIL during the write operation to allow other Python
 * threads to run during potentially blocking I/O. */
static inline int
fwrite_checked_allow_threads(const void *data, size_t size, FILE *fp)
{
    size_t written;
    Py_BEGIN_ALLOW_THREADS
    written = fwrite(data, 1, size, fp);
    Py_END_ALLOW_THREADS
    if (written != size) {
        int err = errno;
        if (ferror(fp) && err != 0) {
            errno = err;
            PyErr_SetFromErrno(PyExc_IOError);
        }
        else {
            PyErr_Format(PyExc_IOError,
                "short write: wrote %zu of %zu bytes", written, size);
        }
        return -1;
    }
    return 0;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

int
binary_io_zstd_available(void)
{
#ifdef HAVE_ZSTD
    return 1;
#else
    return 0;
#endif
}

int
binary_io_get_best_compression(void)
{
#ifdef HAVE_ZSTD
    return COMPRESSION_ZSTD;
#else
    return COMPRESSION_NONE;
#endif
}

/* ============================================================================
 * BINARY WRITER IMPLEMENTATION
 * ============================================================================ */

static int
writer_init_zstd(BinaryWriter *writer)
{
#ifdef HAVE_ZSTD
    writer->zstd.cctx = ZSTD_createCCtx();
    if (!writer->zstd.cctx) {
        PyErr_SetString(PyExc_MemoryError, "Failed to create zstd compression context");
        return -1;
    }

    /* Compression level 3: good ratio without slowing down each block flush */
    size_t result = ZSTD_CCtx_setParameter(writer->zstd.cctx,
                                           ZSTD_c_compressionLevel, 3);
    if (ZSTD_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to set zstd compression level: %s",
                     ZSTD_getErrorName(result));
        ZSTD_freeCCtx(writer->zstd.cctx);
        writer->zstd.cctx = NULL;
        return -1;
    }

    return 0;
#else
    PyErr_SetString(PyExc_RuntimeError,
        "zstd compression requested but not available (HAVE_ZSTD not defined)");
    return -1;
#endif
}

/* ============================================================================
 * HASH TABLE SUPPORT FUNCTIONS (using _Py_hashtable)
 * ============================================================================ */

/* Hash function for Python strings - uses Python's cached hash */
static Py_uhash_t
string_hash_func(const void *key)
{
    PyObject *str = (PyObject *)key;
    Py_hash_t hash = PyObject_Hash(str);
    if (hash == -1) {
        PyErr_Clear();
        return 0;
    }
    return (Py_uhash_t)hash;
}

static int
string_compare_func(const void *key1, const void *key2)
{
    PyObject *str1 = (PyObject *)key1;
    PyObject *str2 = (PyObject *)key2;
    if (str1 == str2) {
        return 1;
    }
    int result = PyObject_RichCompareBool(str1, str2, Py_EQ);
    if (result == -1) {
        PyErr_Clear();
        return 0;
    }
    return result;
}

static void
string_key_destroy(void *key)
{
    Py_XDECREF((PyObject *)key);
}

static Py_uhash_t
frame_key_hash_func(const void *key)
{
    const FrameKey *fk = (const FrameKey *)key;
    /* FNV-1a style hash combining all fields */
    Py_uhash_t hash = 2166136261u;
    hash ^= fk->filename_idx;
    hash *= 16777619u;
    hash ^= fk->funcname_idx;
    hash *= 16777619u;
    hash ^= (uint32_t)fk->lineno;
    hash *= 16777619u;
    hash ^= (uint32_t)fk->end_lineno;
    hash *= 16777619u;
    hash ^= (uint32_t)fk->column;
    hash *= 16777619u;
    hash ^= (uint32_t)fk->end_column;
    hash *= 16777619u;
    hash ^= fk->opcode;
    hash *= 16777619u;
    return hash;
}

static int
frame_key_compare_func(const void *key1, const void *key2)
{
    const FrameKey *fk1 = (const FrameKey *)key1;
    const FrameKey *fk2 = (const FrameKey *)key2;
    return (fk1->filename_idx == fk2->filename_idx &&
            fk1->funcname_idx == fk2->funcname_idx &&
            fk1->lineno == fk2->lineno &&
            fk1->end_lineno == fk2->end_lineno &&
            fk1->column == fk2->column &&
            fk1->end_column == fk2->end_column &&
            fk1->opcode == fk2->opcode);
}

static void
frame_key_destroy(void *key)
{
    PyMem_Free(key);
}

static inline int
writer_intern_string(BinaryWriter *writer, PyObject *string, uint32_t *index)
{
    void *existing = _Py_hashtable_get(writer->string_hash, string);
    if (existing != NULL) {
        *index = (uint32_t)(uintptr_t)existing - 1;  /* index+1 stored to distinguish from NULL */
        return 0;
    }

    if (writer->string_count >= UINT32_MAX) {
        writer->state = BINARY_WRITER_LIMIT_REACHED;
        PyErr_SetString(PyExc_OverflowError,
            "too many strings for binary format");
        return -1;
    }
    if (writer->string_count >= writer->string_capacity) {
        if (grow_parallel_arrays((void **)&writer->strings,
                                  (void **)&writer->string_lengths,
                                  &writer->string_capacity,
                                  sizeof(char *), sizeof(size_t)) < 0) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                writer->state = BINARY_WRITER_LIMIT_REACHED;
            }
            return -1;
        }
    }

    Py_ssize_t str_len;
    const char *str_data = PyUnicode_AsUTF8AndSize(string, &str_len);
    if (!str_data) {
        return -1;
    }
    if ((uintmax_t)str_len > UINT32_MAX) {
        writer->state = BINARY_WRITER_LIMIT_REACHED;
        PyErr_Format(PyExc_OverflowError,
            "string length %zd exceeds binary format maximum %u",
            str_len, UINT32_MAX);
        return -1;
    }

    char *str_copy = PyMem_Malloc(str_len + 1);
    if (!str_copy) {
        PyErr_NoMemory();
        return -1;
    }
    memcpy(str_copy, str_data, str_len + 1);

    *index = (uint32_t)writer->string_count;

    /* Add to hash table FIRST to ensure atomic rollback on failure */
    Py_INCREF(string);
    if (_Py_hashtable_set(writer->string_hash, string, (void *)(uintptr_t)(*index + 1)) < 0) {
        Py_DECREF(string);
        PyMem_Free(str_copy);
        PyErr_NoMemory();
        return -1;
    }

    writer->strings[writer->string_count] = str_copy;
    writer->string_lengths[writer->string_count] = str_len;
    writer->string_count++;

    return 0;
}

static inline int
writer_intern_frame(BinaryWriter *writer, const FrameEntry *entry, uint32_t *index)
{
    FrameKey lookup_key = {
        entry->filename_idx, entry->funcname_idx,
        entry->lineno, entry->end_lineno,
        entry->column, entry->end_column,
        entry->opcode
    };

    void *existing = _Py_hashtable_get(writer->frame_hash, &lookup_key);
    if (existing != NULL) {
        *index = (uint32_t)(uintptr_t)existing - 1;  /* index+1 stored to distinguish from NULL */
        return 0;
    }

    if (writer->frame_count >= UINT32_MAX) {
        writer->state = BINARY_WRITER_LIMIT_REACHED;
        PyErr_SetString(PyExc_OverflowError,
            "too many frames for binary format");
        return -1;
    }
    if (GROW_ARRAY(writer->frame_entries, writer->frame_count,
                   writer->frame_capacity, FrameEntry) < 0) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
            writer->state = BINARY_WRITER_LIMIT_REACHED;
        }
        return -1;
    }

    FrameKey *key = PyMem_Malloc(sizeof(FrameKey));
    if (!key) {
        PyErr_NoMemory();
        return -1;
    }
    *key = lookup_key;

    *index = (uint32_t)writer->frame_count;
    writer->frame_entries[writer->frame_count] = *entry;

    if (_Py_hashtable_set(writer->frame_hash, key, (void *)(uintptr_t)(*index + 1)) < 0) {
        PyMem_Free(key);
        PyErr_NoMemory();
        return -1;
    }

    writer->frame_count++;
    return 0;
}

/* Get or create a thread entry for the given thread_id.
 * Returns pointer to ThreadEntry, or NULL on allocation failure.
 * If is_new is non-NULL, sets it to 1 if this is a new thread, 0 otherwise. */
static ThreadEntry *
writer_get_or_create_thread_entry(BinaryWriter *writer, uint64_t thread_id,
                                   uint32_t interpreter_id, int *is_new)
{
    /* Linear search is OK for small number of threads.
     * Key is (thread_id, interpreter_id) since same thread_id can exist in different interpreters. */
    for (size_t i = 0; i < writer->thread_count; i++) {
        if (writer->thread_entries[i].thread_id == thread_id &&
            writer->thread_entries[i].interpreter_id == interpreter_id) {
            if (is_new) {
                *is_new = 0;
            }
            return &writer->thread_entries[i];
        }
    }

    if (writer->thread_count >= UINT32_MAX) {
        writer->state = BINARY_WRITER_LIMIT_REACHED;
        PyErr_SetString(PyExc_OverflowError,
            "too many threads for binary format");
        return NULL;
    }
    if (writer->thread_count >= writer->thread_capacity) {
        ThreadEntry *new_entries = grow_array(writer->thread_entries,
                                              &writer->thread_capacity,
                                              sizeof(ThreadEntry));
        if (!new_entries) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                writer->state = BINARY_WRITER_LIMIT_REACHED;
            }
            return NULL;
        }
        writer->thread_entries = new_entries;
    }

    ThreadEntry *entry = &writer->thread_entries[writer->thread_count];
    memset(entry, 0, sizeof(ThreadEntry));
    entry->thread_id = thread_id;
    entry->interpreter_id = interpreter_id;
    entry->block_stream_index = UINT32_MAX;
    entry->current_repeat = UINT32_MAX;
    entry->prev_stack_capacity = MAX_STACK_DEPTH;

    entry->prev_stack = PyMem_Calloc(entry->prev_stack_capacity, sizeof(uint32_t));
    if (!entry->prev_stack) {
        PyErr_NoMemory();
        return NULL;
    }

    writer->thread_count++;
    if (is_new) {
        *is_new = 1;
    }
    return entry;
}

static int
encode_column(BinaryWriter *writer, ByteBuffer *decoded,
              ColumnDescriptor *descriptor)
{
    SampleBlock *block = writer->block;
    const uint8_t *data = decoded->data;
    size_t size = decoded->size;
    uint8_t encoding = COMPRESSION_NONE;
    if (size > 1 && memcmp(data, data + 1, size - 1) == 0) {
        encoding = COLUMN_CONSTANT;
        size = 1;
    }
#ifdef HAVE_ZSTD
    if (encoding == COMPRESSION_NONE && size >= ZSTD_MIN_SIZE &&
        writer->compression_type == COMPRESSION_ZSTD) {
        size_t compress_bound = ZSTD_compressBound(size);
        if (buffer_reserve(&block->compressed, compress_bound) < 0) {
            return -1;
        }
        size_t compressed_size = ZSTD_compress2(
            writer->zstd.cctx, block->compressed.data, compress_bound,
            data, size);
        if (ZSTD_isError(compressed_size)) {
            PyErr_Format(PyExc_IOError, "zstd compression error: %s",
                         ZSTD_getErrorName(compressed_size));
            return -1;
        }
        _Py_MSAN_UNPOISON(block->compressed.data, compressed_size);
        uint8_t stored_size[MAX_VARINT_SIZE];
        if (compressed_size + encode_varint_u64(stored_size, compressed_size)
                < size) {
            data = block->compressed.data;
            size = compressed_size;
            encoding = COMPRESSION_ZSTD;
        }
    }
#endif
    descriptor->decoded_size = (uint32_t)decoded->size;
    descriptor->stored_size = (uint32_t)size;
    descriptor->encoding = encoding;
    if (buffer_reserve(&block->output, block->output.size + size) < 0) {
        return -1;
    }
    if (size != 0) {
        memcpy(block->output.data + block->output.size, data, size);
    }
    block->output.size += size;
    return 0;
}

static int
transform_statuses(SampleBlock *block, uint8_t bitmap_flags,
                   ByteBuffer **decoded)
{
    size_t plane_size = status_bitmap_size(block->sample_count);
    unsigned plane_count = (unsigned)_Py_popcount32(bitmap_flags);
    if (buffer_reserve(&block->transformed,
                       plane_size * plane_count) < 0) {
        return -1;
    }
    const uint8_t *statuses =
        block->columns[COLUMN_STATUS_BITMAPS].data;
    block->transformed.size = 0;
    for (unsigned bit = 0; bit < CHAR_BIT; bit++) {
        if (!(bitmap_flags & (1U << bit))) {
            continue;
        }
        uint8_t *plane = block->transformed.data + block->transformed.size;
        memset(plane, 0, plane_size);
        for (uint32_t sample = 0; sample < block->sample_count; sample++) {
            plane[sample / CHAR_BIT] |= ((statuses[sample] >> bit) & 1U)
                                        << (sample % CHAR_BIT);
        }
        block->transformed.size += plane_size;
    }
    *decoded = &block->transformed;
    return 0;
}

static int
transform_frame_ids(SampleBlock *block, uint32_t frame_count,
                    unsigned width, ByteBuffer **decoded)
{
    ByteBuffer *frames = &block->columns[COLUMN_FRAME_IDS];
    if (buffer_reserve(&block->transformed,
                       (size_t)frame_count * width) < 0) {
        return -1;
    }
    const uint32_t *frame_ids = (const uint32_t *)frames->data;
    for (unsigned byte = 0; frame_count && byte < width; byte++) {
        uint8_t *plane =
            block->transformed.data + (size_t)byte * frame_count;
        for (uint32_t frame = 0; frame < frame_count; frame++) {
            plane[frame] =
                (uint8_t)(frame_ids[frame] >> (byte * CHAR_BIT));
        }
    }
    block->transformed.size = (size_t)frame_count * width;
    *decoded = &block->transformed;
    return 0;
}

static int
transform_stack_instructions(SampleBlock *block, ByteBuffer **decoded)
{
    size_t repeat_count = block->repeats.size / sizeof(Repeat);
    if (repeat_count == 0) {
        return 0;
    }
    ByteBuffer *instructions =
        &block->columns[COLUMN_STACK_INSTRUCTIONS];
    const Repeat *repeats = (const Repeat *)block->repeats.data;
    size_t copied_offset = 0;
    block->transformed.size = 0;
    for (size_t i = 0; i < repeat_count; i++) {
        if (buffer_append(&block->transformed,
                          instructions->data + copied_offset,
                          repeats[i].offset - copied_offset) < 0 ||
            buffer_append_varint(
                &block->transformed, STACK_REPEAT) < 0 ||
            buffer_append_varint(
                &block->transformed, repeats[i].count) < 0) {
            return -1;
        }
        copied_offset = repeats[i].offset;
    }
    if (buffer_append(&block->transformed,
                      instructions->data + copied_offset,
                      instructions->size - copied_offset) < 0) {
        return -1;
    }
    *decoded = &block->transformed;
    return 0;
}

static void
block_reset(BinaryWriter *writer)
{
    SampleBlock *block = writer->block;
    for (unsigned column = 0; column < COLUMN_COUNT; column++) {
        block->columns[column].size = 0;
    }
    block->sample_count = 0;
    block->stream_count = 0;
    block->repeats.size = 0;
    block->max_frame = 0;
    block->size = 0;
    memset(block->cache, 0, sizeof(block->cache));
    for (size_t thread = 0; thread < writer->thread_count; thread++) {
        writer->thread_entries[thread].block_stream_index = UINT32_MAX;
        writer->thread_entries[thread].current_repeat = UINT32_MAX;
    }
}

static unsigned
block_frame_width(const SampleBlock *block)
{
    if (block->max_frame == 0) {
        return 1;
    }
    return (unsigned)(_Py_bit_length(block->max_frame) + CHAR_BIT - 1)
           / CHAR_BIT;
}

static void
block_status_flags(const SampleBlock *block, uint8_t *common_flags,
                   uint8_t *bitmap_flags)
{
    uint8_t common_status = UINT8_MAX;
    uint8_t any_status = 0;
    const uint8_t *statuses = block->columns[COLUMN_STATUS_BITMAPS].data;
    for (uint32_t sample = 0; sample < block->sample_count; sample++) {
        common_status &= statuses[sample];
        any_status |= statuses[sample];
    }
    *common_flags = common_status;
    *bitmap_flags = any_status ^ common_status;
}

static size_t
write_block_header(const BlockHeader *header, uint8_t *bytes)
{
    /* Magic "TBLK", sample count and base timestamp (varints), then frame
     * ID width and the two status flag masks (1 byte each) */
    size_t size = BLOCK_MAGIC_SIZE;
    memcpy(bytes, BLOCK_MAGIC, BLOCK_MAGIC_SIZE);
    size += encode_varint_u32(bytes + size, header->sample_count);
    size += encode_varint_u64(bytes + size, header->base_timestamp);
    bytes[size++] = header->frame_width;
    bytes[size++] = header->common_status_flags;
    bytes[size++] = header->status_bitmap_flags;

    /* Each column: decoded size (varint), encoding (1 byte), and the stored
     * size (varint) for zstd only */
    for (unsigned column = 0; column < COLUMN_COUNT; column++) {
        const ColumnDescriptor *descriptor = &header->columns[column];
        size += encode_varint_u32(bytes + size, descriptor->decoded_size);
        bytes[size++] = descriptor->encoding;
        if (descriptor->encoding == COMPRESSION_ZSTD) {
            size += encode_varint_u32(bytes + size, descriptor->stored_size);
        }
    }
    return size;
}

static int
block_flush(BinaryWriter *writer)
{
    SampleBlock *block = writer->block;
    if (block->sample_count == 0) {
        return 0;
    }
    uint32_t frame_count = (uint32_t)(
        block->columns[COLUMN_FRAME_IDS].size / sizeof(uint32_t));
    BlockHeader header = {0};
    header.sample_count = block->sample_count;
    header.base_timestamp = block->base_timestamp;
    header.frame_width = (uint8_t)block_frame_width(block);
    block_status_flags(block, &header.common_status_flags,
                       &header.status_bitmap_flags);

    /* Convert each column to its file form, then encode it */
    /* Leave room for the header, which is written once its size is known */
    if (buffer_reserve(&block->output, MAX_BLOCK_HEADER_SIZE) < 0) {
        return -1;
    }
    block->output.size = MAX_BLOCK_HEADER_SIZE;
    size_t decoded_payload_size = 0;
    for (unsigned column = 0; column < COLUMN_COUNT; column++) {
        ByteBuffer *decoded = &block->columns[column];
        int result = 0;
        switch (column) {
        case COLUMN_STATUS_BITMAPS:
            result = transform_statuses(block, header.status_bitmap_flags,
                                        &decoded);
            break;
        case COLUMN_STACK_INSTRUCTIONS:
            result = transform_stack_instructions(block, &decoded);
            break;
        case COLUMN_FRAME_IDS:
            result = transform_frame_ids(
                block, frame_count, header.frame_width, &decoded);
            break;
        }
        if (result < 0) {
            return -1;
        }
        if (decoded->size >
                MAX_BLOCK_SIZE - decoded_payload_size) {
            PyErr_SetString(PyExc_RuntimeError,
                            "sample block exceeds the decoded size limit");
            return -1;
        }
        decoded_payload_size += decoded->size;
        if (encode_column(writer, decoded, &header.columns[column]) < 0) {
            return -1;
        }
    }
    /* Put the header right before the columns and write the block */
    uint8_t header_bytes[MAX_BLOCK_HEADER_SIZE];
    size_t header_size = write_block_header(&header, header_bytes);
    size_t unused = MAX_BLOCK_HEADER_SIZE - header_size;
    uint8_t *header_start = block->output.data + unused;
    memcpy(header_start, header_bytes, header_size);
    if (fwrite_checked_allow_threads(header_start,
                                     block->output.size - unused,
                                     writer->fp) < 0) {
        return -1;
    }
    writer->stats.bytes_written += decoded_payload_size;
    block_reset(writer);
    return 0;
}

static Py_uhash_t
stack_hash(const uint32_t *stack, size_t depth)
{
    return (Py_uhash_t)Py_HashBuffer(
        stack, (Py_ssize_t)(depth * sizeof(*stack)));
}

static size_t
common_outer_frames(const ThreadEntry *entry, const uint32_t *stack,
                    size_t depth)
{
    size_t shared = 0;
    while (shared < depth && shared < entry->prev_stack_depth &&
           stack[depth - 1 - shared] ==
               entry->prev_stack[entry->prev_stack_depth - 1 - shared]) {
        shared++;
    }
    return shared;
}

static int
cached_stack_matches(const StackCacheEntry *cached,
                     const ByteBuffer *frames, Py_uhash_t hash,
                     const uint32_t *stack, size_t depth)
{
    if (cached->hash != hash || cached->depth != depth) {
        return 0;
    }
    const uint32_t *frame_ids = (const uint32_t *)frames->data;
    for (size_t frame = 0; frame < depth; frame++) {
        if (stack[frame] !=
                frame_ids[cached->offset + depth - 1 - frame]) {
            return 0;
        }
    }
    return 1;
}

static StackCacheEntry *
stack_cache_lookup(StackCacheEntry *bucket, const ByteBuffer *frames,
                   Py_uhash_t hash, const uint32_t *stack, size_t depth)
{
    for (size_t i = 0; i < STACK_CACHE_BUCKET_SIZE; i++) {
        if (cached_stack_matches(&bucket[i], frames, hash, stack, depth)) {
            return &bucket[i];
        }
    }
    return NULL;
}

static void
stack_cache_insert(StackCacheEntry *bucket, StackCacheEntry entry)
{
    memmove(&bucket[1], &bucket[0],
            (STACK_CACHE_BUCKET_SIZE - 1) * sizeof(*bucket));
    bucket[0] = entry;
}

static int
write_stack_repeat(BinaryWriter *writer, ThreadEntry *entry, size_t depth)
{
    SampleBlock *block = writer->block;
    if (entry->current_repeat == UINT32_MAX) {
        Repeat repeat = {
            (uint32_t)block->columns[COLUMN_STACK_INSTRUCTIONS].size, 1};
        if (buffer_append(&block->repeats,
                          &repeat, sizeof(repeat)) < 0) {
            return -1;
        }
        block->size += sizeof(repeat);
        entry->current_repeat =
            (uint32_t)(block->repeats.size / sizeof(repeat) - 1);
        writer->stats.repeat_records++;
    }
    else {
        Repeat *repeats = (Repeat *)block->repeats.data;
        repeats[entry->current_repeat].count++;
    }
    writer->stats.repeat_samples++;
    writer->stats.frames_saved += depth;
    return 0;
}

static int
write_stack_reuse(BinaryWriter *writer,
                  const StackCacheEntry *cached, size_t depth)
{
    SampleBlock *block = writer->block;
    if (column_write_varint(block, COLUMN_STACK_INSTRUCTIONS,
                            STACK_REUSE) < 0 ||
        column_write_varint(block, COLUMN_STACK_INSTRUCTIONS,
                            cached->offset) < 0 ||
        column_write_varint(block, COLUMN_STACK_INSTRUCTIONS, depth) < 0) {
        return -1;
    }
    writer->stats.reuse_records++;
    writer->stats.frames_saved += depth;
    return 0;
}

static int
write_stack_replace_suffix(BinaryWriter *writer, StackCacheEntry *bucket,
                           Py_uhash_t hash, const uint32_t *stack,
                           size_t depth, size_t shared)
{
    SampleBlock *block = writer->block;
    ByteBuffer *frames = &block->columns[COLUMN_FRAME_IDS];
    size_t new_frames = depth - shared;
    if (column_write_varint(block, COLUMN_STACK_INSTRUCTIONS,
                            STACK_REPLACE_SUFFIX) < 0 ||
        column_write_varint(block, COLUMN_STACK_INSTRUCTIONS, shared) < 0 ||
        column_write_varint(block, COLUMN_STACK_INSTRUCTIONS,
                            new_frames) < 0) {
        return -1;
    }

    /* Append the new inner frames, outermost first */
    size_t stack_offset = frames->size / sizeof(uint32_t);
    if (new_frames != 0) {
        if (buffer_reserve(
                frames, frames->size + new_frames * sizeof(uint32_t)) < 0) {
            return -1;
        }
        uint32_t *new_frame_ids = (uint32_t *)(frames->data + frames->size);
        for (size_t i = 0; i < new_frames; i++) {
            uint32_t frame = stack[new_frames - 1 - i];
            new_frame_ids[i] = frame;
            if (frame > block->max_frame) {
                block->max_frame = frame;
            }
        }
        frames->size += new_frames * sizeof(uint32_t);
        block->size += new_frames * sizeof(uint32_t);
    }
    /* Complete stacks are cached so later samples can REUSE them */
    if (shared == 0) {
        if (bucket != NULL) {
            stack_cache_insert(bucket, (StackCacheEntry){
                hash, (uint32_t)stack_offset, (uint32_t)depth});
        }
        writer->stats.full_records++;
    }
    else {
        writer->stats.suffix_records++;
    }
    writer->stats.total_frames_written += new_frames;
    writer->stats.frames_saved += shared;
    return 0;
}

static int
choose_stack_encoding(SampleBlock *block, const ThreadEntry *entry,
                      int new_stream, const uint32_t *stack, size_t depth,
                      size_t *shared, Py_uhash_t *hash,
                      StackCacheEntry **bucket, StackCacheEntry **cached)
{
    *shared = common_outer_frames(entry, stack, depth);
    *hash = 0;
    *bucket = NULL;
    *cached = NULL;
    if (!new_stream && *shared == depth &&
        *shared == entry->prev_stack_depth) {
        return STACK_REPEAT;
    }
    if (depth - *shared < STACK_REUSE_MIN_FRAMES) {
        return STACK_REPLACE_SUFFIX;
    }
    *hash = stack_hash(stack, depth);
    *bucket = block->cache[*hash % STACK_CACHE_BUCKET_COUNT];
    *cached = stack_cache_lookup(*bucket, &block->columns[COLUMN_FRAME_IDS],
                                 *hash, stack, depth);
    if (*cached != NULL) {
        return STACK_REUSE;
    }
    return STACK_REPLACE_SUFFIX;
}

static int
write_stack(BinaryWriter *writer, ThreadEntry *entry, int new_stream,
            const uint32_t *stack, size_t depth)
{
    size_t shared;
    Py_uhash_t hash;
    StackCacheEntry *bucket;
    StackCacheEntry *cached;
    int encoding = choose_stack_encoding(writer->block, entry, new_stream,
                                         stack, depth, &shared, &hash,
                                         &bucket, &cached);

    int result;
    switch (encoding) {
    case STACK_REPEAT:
        return write_stack_repeat(writer, entry, depth);
    case STACK_REUSE:
        result = write_stack_reuse(writer, cached, depth);
        break;
    case STACK_REPLACE_SUFFIX:
        result = write_stack_replace_suffix(writer, bucket, hash, stack,
                                            depth, shared);
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "Invalid stack encoding type");
        return -1;
    }
    if (result < 0) {
        return -1;
    }
    entry->current_repeat = UINT32_MAX;
    memcpy(entry->prev_stack, stack, depth * sizeof(uint32_t));
    entry->prev_stack_depth = depth;
    return 0;
}

size_t
binary_writer_buffered_size(BinaryWriter *writer)
{
    SampleBlock *block = writer->block;
    if (block == NULL || block->sample_count == 0) {
        return 0;
    }
    size_t size = block->columns[COLUMN_STREAM_TABLE].size
                  + block->columns[COLUMN_SAMPLE_STREAM_IDS].size
                  + block->columns[COLUMN_TIMESTAMP_RESIDUALS].size
                  + block->columns[COLUMN_STACK_INSTRUCTIONS].size;
    const Repeat *repeats = (const Repeat *)block->repeats.data;
    size_t repeat_count = block->repeats.size / sizeof(Repeat);
    uint8_t bytes[MAX_VARINT_SIZE];
    for (size_t i = 0; i < repeat_count; i++) {
        size += encode_varint_u64(bytes, STACK_REPEAT)
                + encode_varint_u64(bytes, repeats[i].count);
    }
    uint8_t common_flags, bitmap_flags;
    block_status_flags(block, &common_flags, &bitmap_flags);
    size += (size_t)_Py_popcount32(bitmap_flags)
            * status_bitmap_size(block->sample_count);
    size += block->columns[COLUMN_FRAME_IDS].size / sizeof(uint32_t)
            * block_frame_width(block);
    return size;
}

static int
block_add_stream(BinaryWriter *writer, ThreadEntry *entry)
{
    SampleBlock *block = writer->block;
    /* Stream table entry: thread_id (8 bytes) + interpreter_id (4 bytes) */
    uint8_t stream_entry[STREAM_ENTRY_SIZE];
    memcpy(stream_entry + STM_OFF_THREAD_ID,
           &entry->thread_id, sizeof(entry->thread_id));
    memcpy(stream_entry + STM_OFF_INTERPRETER_ID,
           &entry->interpreter_id, sizeof(entry->interpreter_id));
    if (column_write(block, COLUMN_STREAM_TABLE,
                     stream_entry, sizeof(stream_entry)) < 0) {
        return -1;
    }
    entry->block_stream_index = block->stream_count++;
    entry->prev_timestamp = block->base_timestamp - writer->sample_interval_us;
    entry->prev_stack_depth = 0;
    return 0;
}

static int
block_add_sample(BinaryWriter *writer, ThreadEntry *entry,
                 uint64_t timestamp, uint8_t status,
                 const uint32_t *stack, size_t depth)
{
    static_assert(BLOCK_FLUSH_SIZE + MAX_SAMPLE_SIZE + MAX_BITMAP_PADDING ==
                      MAX_BLOCK_SIZE,
                  "block flush size needs headroom for one complete sample");

    SampleBlock *block = writer->block;
    /* Start a new block if this sample might not fit */
    if (block->size >= BLOCK_FLUSH_SIZE ||
        (entry->block_stream_index == UINT32_MAX &&
         block->stream_count == MAX_BLOCK_STREAMS)) {
        if (block_flush(writer) < 0) {
            return -1;
        }
    }
    if (block->sample_count == 0) {
        block->base_timestamp = timestamp;
    }
    int new_stream = entry->block_stream_index == UINT32_MAX;
    if (new_stream && block_add_stream(writer, entry) < 0) {
        return -1;
    }
    /* Store the difference from the expected timestamp */
    uint64_t residual =
        timestamp - entry->prev_timestamp - writer->sample_interval_us;
    uint8_t residual_bytes[MAX_VARINT_SIZE];
    size_t residual_size =
        encode_varint_i64(residual_bytes, (int64_t)residual);
    if (column_write_varint(block, COLUMN_SAMPLE_STREAM_IDS,
                            entry->block_stream_index) < 0 ||
        column_write(block, COLUMN_TIMESTAMP_RESIDUALS,
                     residual_bytes, residual_size) < 0 ||
        column_write(block, COLUMN_STATUS_BITMAPS,
                     &status, sizeof(status)) < 0) {
        return -1;
    }
    if (write_stack(writer, entry, new_stream, stack, depth) < 0) {
        return -1;
    }
    entry->prev_timestamp = timestamp;
    block->sample_count++;
    writer->total_samples++;
    return 0;
}

static void
block_destroy(SampleBlock *block)
{
    if (block == NULL) {
        return;
    }
    for (unsigned i = 0; i < COLUMN_COUNT; i++) {
        buffer_clear(&block->columns[i]);
    }
    buffer_clear(&block->repeats);
    buffer_clear(&block->transformed);
    buffer_clear(&block->compressed);
    buffer_clear(&block->output);
    PyMem_Free(block);
}

BinaryWriter *
binary_writer_create(PyObject *path, uint64_t sample_interval_us, int compression_type,
                     uint64_t start_time_us, int profiling_mode,
                     int capture_features)
{
    if (compression_type != COMPRESSION_NONE &&
        compression_type != COMPRESSION_ZSTD) {
        PyErr_SetString(PyExc_ValueError, "invalid compression type");
        return NULL;
    }
    BinaryWriter *writer = PyMem_Calloc(1, sizeof(BinaryWriter));
    if (!writer) {
        PyErr_NoMemory();
        return NULL;
    }

    writer->start_time_us = start_time_us;
    writer->sample_interval_us = sample_interval_us;
    writer->compression_type = compression_type;
    writer->profiling_mode = profiling_mode;
    writer->capture_features = capture_features;

    writer->block = PyMem_Calloc(1, sizeof(SampleBlock));
    if (writer->block == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    writer->string_hash = _Py_hashtable_new_full(
        string_hash_func,
        string_compare_func,
        string_key_destroy,  /* Key destroy: decref the Python string */
        NULL,                /* Value destroy: values are just indices, not pointers */
        NULL                 /* Use default allocator */
    );
    if (!writer->string_hash) {
        PyErr_NoMemory();
        goto error;
    }
    writer->strings = PyMem_Malloc(INITIAL_STRING_CAPACITY * sizeof(char *));
    if (!writer->strings) {
        PyErr_NoMemory();
        goto error;
    }
    writer->string_lengths = PyMem_Malloc(INITIAL_STRING_CAPACITY * sizeof(size_t));
    if (!writer->string_lengths) {
        PyErr_NoMemory();
        goto error;
    }
    writer->string_capacity = INITIAL_STRING_CAPACITY;

    writer->frame_hash = _Py_hashtable_new_full(
        frame_key_hash_func,
        frame_key_compare_func,
        frame_key_destroy,   /* Key destroy: free the FrameKey */
        NULL,                /* Value destroy: values are just indices, not pointers */
        NULL                 /* Use default allocator */
    );
    if (!writer->frame_hash) {
        PyErr_NoMemory();
        goto error;
    }
    writer->frame_entries = PyMem_Malloc(INITIAL_FRAME_CAPACITY * sizeof(FrameEntry));
    if (!writer->frame_entries) {
        PyErr_NoMemory();
        goto error;
    }
    writer->frame_capacity = INITIAL_FRAME_CAPACITY;

    writer->thread_entries = PyMem_Malloc(INITIAL_THREAD_CAPACITY * sizeof(ThreadEntry));
    if (!writer->thread_entries) {
        PyErr_NoMemory();
        goto error;
    }
    writer->thread_capacity = INITIAL_THREAD_CAPACITY;

    if (compression_type == COMPRESSION_ZSTD) {
        if (writer_init_zstd(writer) < 0) {
            goto error;
        }
    }

    writer->fp = Py_fopen(path, "wb");
    if (!writer->fp) {
        goto error;
    }

    /* Hint sequential write pattern to kernel for better I/O scheduling */
#if defined(__linux__) && defined(POSIX_FADV_SEQUENTIAL)
    {
        int fd = fileno(writer->fp);
        if (fd >= 0) {
            (void)posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
        }
    }
#endif

    uint8_t header[FILE_HEADER_PLACEHOLDER_SIZE] = {0};
    if (fwrite_checked_allow_threads(header, FILE_HEADER_PLACEHOLDER_SIZE, writer->fp) < 0) {
        goto error;
    }

    return writer;

error:
    binary_writer_destroy(writer);
    return NULL;
}

/* Build a frame stack from Python frame list by interning all strings and frames.
 * Returns 0 on success, -1 on error. */
static int
build_frame_stack(BinaryWriter *writer, PyObject *frame_list,
                  uint32_t *curr_stack, size_t *curr_depth)
{
    Py_ssize_t stack_depth = PyList_Size(frame_list);
    *curr_depth = (stack_depth < MAX_STACK_DEPTH) ? stack_depth : MAX_STACK_DEPTH;

    for (Py_ssize_t k = 0; k < (Py_ssize_t)*curr_depth; k++) {
        /* Use unchecked accessors since we control the data structures */
        PyObject *frame_info = PyList_GET_ITEM(frame_list, k);

        /* Get filename, location, funcname, opcode from FrameInfo using unchecked access */
        PyObject *filename = PyStructSequence_GET_ITEM(frame_info, 0);
        PyObject *location = PyStructSequence_GET_ITEM(frame_info, 1);
        PyObject *funcname = PyStructSequence_GET_ITEM(frame_info, 2);
        PyObject *opcode_obj = PyStructSequence_GET_ITEM(frame_info, 3);

        /* Extract location fields (can be None for synthetic frames) */
        int32_t lineno = LOCATION_NOT_AVAILABLE;
        int32_t end_lineno = LOCATION_NOT_AVAILABLE;
        int32_t column = LOCATION_NOT_AVAILABLE;
        int32_t end_column = LOCATION_NOT_AVAILABLE;

        if (location != Py_None) {
            /* LocationInfo is a struct sequence or tuple with:
             * (lineno, end_lineno, column, end_column) */
            PyObject *lineno_obj = PyTuple_Check(location) ?
                PyTuple_GET_ITEM(location, 0) :
                PyStructSequence_GET_ITEM(location, 0);
            PyObject *end_lineno_obj = PyTuple_Check(location) ?
                PyTuple_GET_ITEM(location, 1) :
                PyStructSequence_GET_ITEM(location, 1);
            PyObject *column_obj = PyTuple_Check(location) ?
                PyTuple_GET_ITEM(location, 2) :
                PyStructSequence_GET_ITEM(location, 2);
            PyObject *end_column_obj = PyTuple_Check(location) ?
                PyTuple_GET_ITEM(location, 3) :
                PyStructSequence_GET_ITEM(location, 3);

            PYLONG_TO_INT32_OR_DEFAULT(lineno_obj, lineno, LOCATION_NOT_AVAILABLE);
            PYLONG_TO_INT32_OR_DEFAULT(end_lineno_obj, end_lineno, LOCATION_NOT_AVAILABLE);
            PYLONG_TO_INT32_OR_DEFAULT(column_obj, column, LOCATION_NOT_AVAILABLE);
            PYLONG_TO_INT32_OR_DEFAULT(end_column_obj, end_column, LOCATION_NOT_AVAILABLE);
        }

        /* Extract opcode (can be None) */
        uint8_t opcode = OPCODE_NONE;
        if (opcode_obj != Py_None) {
            long opcode_long = PyLong_AsLong(opcode_obj);
            if (UNLIKELY(PyErr_Occurred() != NULL)) {
                PyErr_Clear();
                opcode = OPCODE_NONE;
            } else if (opcode_long >= 0 && opcode_long <= MAX_REAL_OPCODE) {
                opcode = (uint8_t)opcode_long;
            }
        }

        /* Intern filename */
        uint32_t filename_idx;
        if (writer_intern_string(writer, filename, &filename_idx) < 0) {
            return -1;
        }

        /* Intern funcname */
        uint32_t funcname_idx;
        if (writer_intern_string(writer, funcname, &funcname_idx) < 0) {
            return -1;
        }

        /* Intern frame with full location info */
        FrameEntry frame_entry = {
            .filename_idx = filename_idx,
            .funcname_idx = funcname_idx,
            .lineno = lineno,
            .end_lineno = end_lineno,
            .column = column,
            .end_column = end_column,
            .opcode = opcode
        };
        uint32_t frame_idx;
        if (writer_intern_frame(writer, &frame_entry, &frame_idx) < 0) {
            return -1;
        }

        curr_stack[k] = frame_idx;
    }
    return 0;
}

/* Process a single thread's sample.
 * Returns 0 on success, -1 on error. */
static int
process_thread_sample(BinaryWriter *writer, PyObject *thread_info,
                      uint32_t interpreter_id, uint64_t timestamp_us)
{
    if (writer->total_samples == UINT64_MAX) {
        writer->state = BINARY_WRITER_LIMIT_REACHED;
        PyErr_SetString(PyExc_OverflowError, "too many samples for binary format");
        return -1;
    }

    PyObject *thread_id_obj = PyStructSequence_GET_ITEM(thread_info, 0);
    PyObject *status_obj = PyStructSequence_GET_ITEM(thread_info, 1);
    PyObject *frame_list = PyStructSequence_GET_ITEM(thread_info, 2);

    uint64_t thread_id = PyLong_AsUnsignedLongLong(thread_id_obj);
    if (thread_id == (uint64_t)-1 && PyErr_Occurred()) {
        return -1;
    }
    long status_long = PyLong_AsLong(status_obj);
    if (status_long == -1 && PyErr_Occurred()) {
        return -1;
    }
    if (status_long < 0 || status_long > UINT8_MAX) {
        writer->state = BINARY_WRITER_LIMIT_REACHED;
        PyErr_Format(PyExc_OverflowError,
                     "thread status %ld exceeds the binary format range",
                     status_long);
        return -1;
    }
    uint8_t status = (uint8_t)status_long;

    ThreadEntry *entry = writer_get_or_create_thread_entry(
        writer, thread_id, interpreter_id, NULL);
    if (!entry) {
        return -1;
    }

    /* Process frames and build current stack */
    uint32_t curr_stack[MAX_STACK_DEPTH];
    size_t curr_depth;
    if (build_frame_stack(writer, frame_list, curr_stack, &curr_depth) < 0) {
        return -1;
    }
    return block_add_sample(
        writer, entry, timestamp_us, status, curr_stack, curr_depth);
}

int
binary_writer_write_sample(BinaryWriter *writer, PyObject *stack_frames, uint64_t timestamp_us)
{
    if (!PyList_Check(stack_frames)) {
        PyErr_SetString(PyExc_TypeError, "stack_frames must be a list");
        return -1;
    }

    Py_ssize_t num_interpreters = PyList_GET_SIZE(stack_frames);
    for (Py_ssize_t i = 0; i < num_interpreters; i++) {
        PyObject *interp_info = PyList_GET_ITEM(stack_frames, i);

        PyObject *interp_id_obj = PyStructSequence_GET_ITEM(interp_info, 0);
        PyObject *threads = PyStructSequence_GET_ITEM(interp_info, 1);

        unsigned long long interp_id_long = PyLong_AsUnsignedLongLong(interp_id_obj);
        if (interp_id_long == (unsigned long long)-1 && PyErr_Occurred()) {
            return -1;
        }
        /* Bounds check: interpreter_id is stored as uint32_t in binary format */
        if (interp_id_long > UINT32_MAX) {
            writer->state = BINARY_WRITER_LIMIT_REACHED;
            PyErr_Format(PyExc_OverflowError,
                "interpreter_id %llu exceeds maximum value %u",
                interp_id_long, UINT32_MAX);
            return -1;
        }
        uint32_t interpreter_id = (uint32_t)interp_id_long;

        Py_ssize_t num_threads = PyList_GET_SIZE(threads);
        for (Py_ssize_t j = 0; j < num_threads; j++) {
            PyObject *thread_info = PyList_GET_ITEM(threads, j);
            if (process_thread_sample(writer, thread_info, interpreter_id, timestamp_us) < 0) {
                return -1;
            }
        }
    }

    return 0;
}

int
binary_writer_finalize(BinaryWriter *writer)
{
    if (block_flush(writer) < 0) {
        return -1;
    }

    /* Use 64-bit file position for >2GB files */
    file_offset_t string_table_offset = FTELL64(writer->fp);
    if (string_table_offset < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    /* Release GIL during potentially large writes */
    for (size_t i = 0; i < writer->string_count; i++) {
        uint8_t len_buf[10];
        size_t len_size = encode_varint_u32(len_buf, (uint32_t)writer->string_lengths[i]);
        if (fwrite_checked_allow_threads(len_buf, len_size, writer->fp) < 0 ||
            fwrite_checked_allow_threads(writer->strings[i], writer->string_lengths[i], writer->fp) < 0) {
            return -1;
        }
    }

    file_offset_t frame_table_offset = FTELL64(writer->fp);
    if (frame_table_offset < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    for (size_t i = 0; i < writer->frame_count; i++) {
        FrameEntry *entry = &writer->frame_entries[i];
        uint8_t buf[64];  /* Increased buffer for additional fields */
        size_t pos = encode_varint_u32(buf, entry->filename_idx);
        pos += encode_varint_u32(buf + pos, entry->funcname_idx);
        pos += encode_varint_i32(buf + pos, entry->lineno);

        /* Delta encode end_lineno: store (end_lineno - lineno) as zigzag.
         * When lineno is -1, store delta as 0 (result will be -1). */
        int32_t end_lineno_delta = 0;
        if (entry->lineno != LOCATION_NOT_AVAILABLE &&
            entry->end_lineno != LOCATION_NOT_AVAILABLE) {
            end_lineno_delta = entry->end_lineno - entry->lineno;
        }
        pos += encode_varint_i32(buf + pos, end_lineno_delta);

        pos += encode_varint_i32(buf + pos, entry->column);

        /* Delta encode end_column: store (end_column - column) as zigzag.
         * When column is -1, store delta as 0 (result will be -1). */
        int32_t end_column_delta = 0;
        if (entry->column != LOCATION_NOT_AVAILABLE &&
            entry->end_column != LOCATION_NOT_AVAILABLE) {
            end_column_delta = entry->end_column - entry->column;
        }
        pos += encode_varint_i32(buf + pos, end_column_delta);

        buf[pos++] = entry->opcode;

        if (fwrite_checked_allow_threads(buf, pos, writer->fp) < 0) {
            return -1;
        }
    }

    if (writer->has_profile_stats) {
        uint8_t profile_stats[PROFILE_STATS_SIZE] = {0};
        uint32_t version = PROFILE_STATS_VERSION;
        uint32_t size = PROFILE_STATS_SIZE;
        memcpy(profile_stats + PST_OFF_DURATION,
               &writer->duration_sec, PST_SIZE_DURATION);
        memcpy(profile_stats + PST_OFF_SAMPLE_RATE,
               &writer->sample_rate, PST_SIZE_SAMPLE_RATE);
        memcpy(profile_stats + PST_OFF_ERROR_RATE,
               &writer->error_rate, PST_SIZE_ERROR_RATE);
        memcpy(profile_stats + PST_OFF_MISSED_SAMPLES,
               &writer->missed_samples, PST_SIZE_MISSED_SAMPLES);
        memcpy(profile_stats + PST_OFF_PRESENT,
               &writer->profile_stats_present, PST_SIZE_PRESENT);
        memcpy(profile_stats + PST_OFF_MAGIC,
               PROFILE_STATS_MAGIC, PROFILE_STATS_MAGIC_SIZE);
        memcpy(profile_stats + PST_OFF_VERSION, &version, PST_SIZE_VERSION);
        memcpy(profile_stats + PST_OFF_SIZE, &size, PST_SIZE_SIZE);
        if (fwrite_checked_allow_threads(
                profile_stats, PROFILE_STATS_SIZE, writer->fp) < 0) {
            return -1;
        }
    }

    /* Footer: string_count(4) + frame_count(4) + file_size(8) + checksum(16) */
    file_offset_t footer_offset = FTELL64(writer->fp);
    if (footer_offset < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    uint64_t file_size = (uint64_t)footer_offset + FILE_FOOTER_SIZE;
    uint8_t footer[FILE_FOOTER_SIZE] = {0};
    /* Cast size_t to uint32_t before memcpy to ensure correct bytes are copied
     * on both little-endian and big-endian systems (size_t is 8 bytes on 64-bit) */
    uint32_t string_count_u32 = (uint32_t)writer->string_count;
    uint32_t frame_count_u32 = (uint32_t)writer->frame_count;
    memcpy(footer + FTR_OFF_STRINGS, &string_count_u32, FTR_SIZE_STRINGS);
    memcpy(footer + FTR_OFF_FRAMES, &frame_count_u32, FTR_SIZE_FRAMES);
    memcpy(footer + FTR_OFF_FILE_SIZE, &file_size, FTR_SIZE_FILE_SIZE);
    /* checksum (FTR_OFF_CHECKSUM..FILE_FOOTER_SIZE-1): placeholder zeros */
    if (fwrite_checked_allow_threads(footer, FILE_FOOTER_SIZE, writer->fp) < 0) {
        return -1;
    }

    if (FSEEK64(writer->fp, 0, SEEK_SET) < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    /* Convert file offsets and counts to fixed-width types for portable header format.
     * This ensures correct behavior on both little-endian and big-endian systems. */
    uint64_t string_table_offset_u64 = (uint64_t)string_table_offset;
    uint64_t frame_table_offset_u64 = (uint64_t)frame_table_offset;
    uint32_t thread_count_u32 = (uint32_t)writer->thread_count;
    uint32_t compression_type_u32 = (uint32_t)writer->compression_type;
    uint32_t profiling_config_u32 = (uint32_t)(writer->profiling_mode + 1);
    if (writer->capture_features >= 0) {
        profiling_config_u32 |= PROFILING_CONFIG_FEATURES_KNOWN;
        profiling_config_u32 |= (uint32_t)writer->capture_features
            << PROFILING_CONFIG_FEATURES_SHIFT;
    }

    uint8_t header[FILE_HEADER_SIZE] = {0};
    uint32_t magic = BINARY_FORMAT_MAGIC;
    uint32_t version = BINARY_FORMAT_VERSION;
    memcpy(header + HDR_OFF_MAGIC, &magic, HDR_SIZE_MAGIC);
    memcpy(header + HDR_OFF_VERSION, &version, HDR_SIZE_VERSION);
    header[HDR_OFF_PY_MAJOR] = PY_MAJOR_VERSION;
    header[HDR_OFF_PY_MINOR] = PY_MINOR_VERSION;
    header[HDR_OFF_PY_MICRO] = PY_MICRO_VERSION;
    memcpy(header + HDR_OFF_START_TIME, &writer->start_time_us, HDR_SIZE_START_TIME);
    memcpy(header + HDR_OFF_INTERVAL, &writer->sample_interval_us, HDR_SIZE_INTERVAL);
    memcpy(header + HDR_OFF_SAMPLES, &writer->total_samples, HDR_SIZE_SAMPLES);
    memcpy(header + HDR_OFF_THREADS, &thread_count_u32, HDR_SIZE_THREADS);
    memcpy(header + HDR_OFF_STR_TABLE, &string_table_offset_u64, HDR_SIZE_STR_TABLE);
    memcpy(header + HDR_OFF_FRAME_TABLE, &frame_table_offset_u64, HDR_SIZE_FRAME_TABLE);
    memcpy(header + HDR_OFF_COMPRESSION, &compression_type_u32, HDR_SIZE_COMPRESSION);
    memcpy(header + HDR_OFF_CONFIG, &profiling_config_u32, HDR_SIZE_CONFIG);
    if (fwrite_checked_allow_threads(header, FILE_HEADER_SIZE, writer->fp) < 0) {
        return -1;
    }

    if (Py_fclose(writer->fp) != 0) {
        writer->fp = NULL;
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    writer->fp = NULL;

    return 0;
}

int
binary_writer_set_stats(BinaryWriter *writer, double duration_sec,
                        double sample_rate, double error_rate,
                        double missed_samples, uint32_t present)
{
    if (!isfinite(duration_sec) || duration_sec < 0.0) {
        PyErr_SetString(PyExc_ValueError,
                        "duration must be a finite non-negative value");
        return -1;
    }
    if (!isfinite(sample_rate) || sample_rate < 0.0) {
        PyErr_SetString(PyExc_ValueError,
                        "sample rate must be a finite non-negative value");
        return -1;
    }
    if ((present & PROFILE_STATS_ERROR_RATE) &&
        (!isfinite(error_rate) || error_rate < 0.0)) {
        PyErr_SetString(PyExc_ValueError,
                        "error rate must be a finite non-negative value");
        return -1;
    }
    if ((present & PROFILE_STATS_MISSED) && !isfinite(missed_samples)) {
        PyErr_SetString(PyExc_ValueError,
                        "missed samples must be a finite value");
        return -1;
    }
    writer->duration_sec = duration_sec;
    writer->sample_rate = sample_rate;
    writer->error_rate = error_rate;
    writer->missed_samples = missed_samples;
    writer->profile_stats_present = present;
    writer->has_profile_stats = 1;
    return 0;
}

void
binary_writer_destroy(BinaryWriter *writer)
{
    if (!writer) {
        return;
    }

    if (writer->fp) {
        Py_fclose(writer->fp);
    }

    block_destroy(writer->block);

#ifdef HAVE_ZSTD
    if (writer->zstd.cctx) {
        ZSTD_freeCCtx(writer->zstd.cctx);
    }
#endif

    if (writer->string_hash) {
        _Py_hashtable_destroy(writer->string_hash);
    }
    if (writer->strings) {
        for (size_t i = 0; i < writer->string_count; i++) {
            PyMem_Free(writer->strings[i]);
        }
        PyMem_Free(writer->strings);
    }
    PyMem_Free(writer->string_lengths);

    if (writer->frame_hash) {
        _Py_hashtable_destroy(writer->frame_hash);
    }
    PyMem_Free(writer->frame_entries);

    if (writer->thread_entries) {
        for (size_t i = 0; i < writer->thread_count; i++) {
            PyMem_Free(writer->thread_entries[i].prev_stack);
        }
        PyMem_Free(writer->thread_entries);
    }

    PyMem_Free(writer);
}

#undef PYLONG_TO_INT32_OR_DEFAULT
