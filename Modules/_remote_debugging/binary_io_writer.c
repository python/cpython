/******************************************************************************
 * Python Remote Debugging Module - Binary Writer Implementation
 *
 * High-performance binary file writer for profiling data with optional zstd
 * streaming compression.
 ******************************************************************************/

#ifndef Py_BUILD_CORE_MODULE
#  define Py_BUILD_CORE_MODULE
#endif

#include "binary_io.h"
#include "_remote_debugging.h"
#include <string.h>

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

/* ============================================================================
 * CONSTANTS FOR BINARY FORMAT SIZES
 * ============================================================================ */

/* Sample header sizes */
#define SAMPLE_HEADER_FIXED_SIZE 13      /* thread_id(8) + interpreter_id(4) + encoding(1) */
#define SAMPLE_HEADER_MAX_SIZE 26        /* fixed + max_varint(10) + status(1) + margin */
#define MAX_VARINT_SIZE 10               /* Maximum bytes for a varint64 */
#define MAX_VARINT_SIZE_U32 5            /* Maximum bytes for a varint32 */
/* Frame buffer: depth varint (max 2 bytes for 256) + 256 frames * 5 bytes/varint + margin */
#define MAX_FRAME_BUFFER_SIZE ((MAX_STACK_DEPTH * MAX_VARINT_SIZE_U32) + MAX_VARINT_SIZE_U32 + 16)

/* File structure sizes */
#define FILE_HEADER_PLACEHOLDER_SIZE 64  /* Placeholder written at file start */
#define FILE_HEADER_SIZE 52              /* Actual header content size */
#define FILE_FOOTER_SIZE 32              /* Footer size */

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

    /* Check for overflow when doubling capacity */
    if (old_cap > SIZE_MAX / 2) {
        PyErr_SetString(PyExc_OverflowError, "Array capacity overflow");
        return -1;
    }
    size_t new_cap = old_cap * 2;

    /* Check for overflow when calculating allocation sizes */
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

    /* Free old arrays */
    PyMem_Free(*array1);
    PyMem_Free(*array2);

    /* Update all pointers */
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
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    return 0;
}

/* Forward declaration for writer_write_bytes */
static inline int writer_write_bytes(BinaryWriter *writer, const void *data, size_t size);

/* Encode and write a varint u32 - returns 0 on success, -1 on error */
static inline int
writer_write_varint_u32(BinaryWriter *writer, uint32_t value)
{
    uint8_t buf[MAX_VARINT_SIZE];
    size_t len = encode_varint_u32(buf, value);
    return writer_write_bytes(writer, buf, len);
}

/* Encode and write a varint u64 - returns 0 on success, -1 on error */
static inline int
writer_write_varint_u64(BinaryWriter *writer, uint64_t value)
{
    uint8_t buf[MAX_VARINT_SIZE];
    size_t len = encode_varint_u64(buf, value);
    return writer_write_bytes(writer, buf, len);
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

/* Initialize zstd compression */
static int
writer_init_zstd(BinaryWriter *writer)
{
#ifdef HAVE_ZSTD
    writer->zstd.cctx = ZSTD_createCCtx();
    if (!writer->zstd.cctx) {
        PyErr_SetString(PyExc_MemoryError, "Failed to create zstd compression context");
        return -1;
    }

    /* Compression level 5: better ratio for repetitive profiling data */
    size_t result = ZSTD_CCtx_setParameter(writer->zstd.cctx,
                                           ZSTD_c_compressionLevel, 5);
    if (ZSTD_isError(result)) {
        PyErr_Format(PyExc_RuntimeError, "Failed to set zstd compression level: %s",
                     ZSTD_getErrorName(result));
        ZSTD_freeCCtx(writer->zstd.cctx);
        writer->zstd.cctx = NULL;
        return -1;
    }

    /* Use large buffer (512KB) for fewer I/O syscalls */
    writer->zstd.compressed_buffer = PyMem_Malloc(COMPRESSED_BUFFER_SIZE);
    if (!writer->zstd.compressed_buffer) {
        ZSTD_freeCCtx(writer->zstd.cctx);
        writer->zstd.cctx = NULL;
        PyErr_NoMemory();
        return -1;
    }
    writer->zstd.compressed_buffer_size = COMPRESSED_BUFFER_SIZE;

    return 0;
#else
    PyErr_SetString(PyExc_RuntimeError,
        "zstd compression requested but not available (HAVE_ZSTD not defined)");
    return -1;
#endif
}

/* Flush write buffer to disk (with compression if enabled) */
static int
writer_flush_buffer(BinaryWriter *writer)
{
    if (writer->buffer_pos == 0) {
        return 0;
    }

#ifdef HAVE_ZSTD
    if (writer->compression_type == COMPRESSION_ZSTD) {
        ZSTD_inBuffer input = { writer->write_buffer, writer->buffer_pos, 0 };

        while (input.pos < input.size) {
            ZSTD_outBuffer output = {
                writer->zstd.compressed_buffer,
                writer->zstd.compressed_buffer_size,
                0
            };

            size_t result = ZSTD_compressStream2(
                writer->zstd.cctx, &output, &input, ZSTD_e_continue
            );

            if (ZSTD_isError(result)) {
                PyErr_Format(PyExc_IOError, "zstd compression error: %s",
                             ZSTD_getErrorName(result));
                return -1;
            }

            if (output.pos > 0) {
                if (fwrite_checked_allow_threads(writer->zstd.compressed_buffer, output.pos, writer->fp) < 0) {
                    return -1;
                }
            }
        }
    } else
#endif
    {
        /* Uncompressed write */
        if (fwrite_checked_allow_threads(writer->write_buffer, writer->buffer_pos, writer->fp) < 0) {
            return -1;
        }
    }

    writer->buffer_pos = 0;
    return 0;
}

/* Write bytes to buffer (flushing if needed) */
static inline int
writer_write_bytes(BinaryWriter *writer, const void *data, size_t size)
{
    const uint8_t *src = (const uint8_t *)data;
    size_t original_size = size;

    while (size > 0) {
        size_t space = writer->buffer_size - writer->buffer_pos;
        size_t to_copy = (size < space) ? size : space;

        memcpy(writer->write_buffer + writer->buffer_pos, src, to_copy);
        writer->buffer_pos += to_copy;
        src += to_copy;
        size -= to_copy;

        if (writer->buffer_pos == writer->buffer_size) {
            if (writer_flush_buffer(writer) < 0) {
                return -1;
            }
        }
    }

    writer->stats.bytes_written += original_size;
    return 0;
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

/* Compare function for Python strings */
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

/* Destroy function for string keys - decref the Python string */
static void
string_key_destroy(void *key)
{
    Py_XDECREF((PyObject *)key);
}

/* Hash function for frame keys */
static Py_uhash_t
frame_key_hash_func(const void *key)
{
    const FrameKey *fk = (const FrameKey *)key;
    /* FNV-1a style hash combining all three values */
    Py_uhash_t hash = 2166136261u;
    hash ^= fk->filename_idx;
    hash *= 16777619u;
    hash ^= fk->funcname_idx;
    hash *= 16777619u;
    hash ^= (uint32_t)fk->lineno;
    hash *= 16777619u;
    return hash;
}

/* Compare function for frame keys */
static int
frame_key_compare_func(const void *key1, const void *key2)
{
    const FrameKey *fk1 = (const FrameKey *)key1;
    const FrameKey *fk2 = (const FrameKey *)key2;
    return (fk1->filename_idx == fk2->filename_idx &&
            fk1->funcname_idx == fk2->funcname_idx &&
            fk1->lineno == fk2->lineno);
}

/* Destroy function for frame keys - free the allocated FrameKey */
static void
frame_key_destroy(void *key)
{
    PyMem_Free(key);
}

/* Intern a string and return its index */
static inline int
writer_intern_string(BinaryWriter *writer, PyObject *string, uint32_t *index)
{
    /* Check if string already exists in hash table */
    void *existing = _Py_hashtable_get(writer->string_hash, string);
    if (existing != NULL) {
        *index = (uint32_t)(uintptr_t)existing - 1;  /* Subtract 1 since we store index+1 */
        return 0;
    }

    /* New string - grow storage if needed */
    if (writer->string_count >= writer->string_capacity) {
        if (grow_parallel_arrays((void **)&writer->strings,
                                  (void **)&writer->string_lengths,
                                  &writer->string_capacity,
                                  sizeof(char *), sizeof(size_t)) < 0) {
            return -1;
        }
    }

    Py_ssize_t str_len;
    const char *str_data = PyUnicode_AsUTF8AndSize(string, &str_len);
    if (!str_data) {
        return -1;
    }

    /* Store copy of string data */
    char *str_copy = PyMem_Malloc(str_len + 1);
    if (!str_copy) {
        PyErr_NoMemory();
        return -1;
    }
    memcpy(str_copy, str_data, str_len + 1);

    /* The index we'll use (current count before incrementing) */
    *index = (uint32_t)writer->string_count;

    /* Add to hash table FIRST (before modifying arrays/count) to ensure atomicity.
     * If hash table insert fails, we can simply free str_copy without rolling back.
     * Store index+1 to distinguish from NULL (0 would be ambiguous). */
    Py_INCREF(string);
    if (_Py_hashtable_set(writer->string_hash, string, (void *)(uintptr_t)(*index + 1)) < 0) {
        Py_DECREF(string);
        PyMem_Free(str_copy);
        PyErr_NoMemory();
        return -1;
    }

    /* Hash table insert succeeded - now safely update arrays and count.
     * These operations cannot fail, so the data structures stay consistent. */
    writer->strings[writer->string_count] = str_copy;
    writer->string_lengths[writer->string_count] = str_len;
    writer->string_count++;

    return 0;
}

/* Intern a frame and return its index */
static inline int
writer_intern_frame(BinaryWriter *writer, uint32_t filename_idx, uint32_t funcname_idx,
                    int32_t lineno, uint32_t *index)
{
    /* Create a temporary key for lookup */
    FrameKey lookup_key = {filename_idx, funcname_idx, lineno};

    /* Check if frame already exists in hash table */
    void *existing = _Py_hashtable_get(writer->frame_hash, &lookup_key);
    if (existing != NULL) {
        *index = (uint32_t)(uintptr_t)existing - 1;  /* Subtract 1 since we store index+1 */
        return 0;
    }

    /* New frame - grow storage if needed */
    if (GROW_ARRAY(writer->frame_entries, writer->frame_count,
                   writer->frame_capacity, FrameEntry) < 0) {
        return -1;
    }

    /* Allocate key for hash table first (before modifying frame_count)
     * to ensure atomic rollback on failure */
    FrameKey *key = PyMem_Malloc(sizeof(FrameKey));
    if (!key) {
        PyErr_NoMemory();
        return -1;
    }
    *key = lookup_key;

    /* Now add the frame entry */
    *index = (uint32_t)writer->frame_count;
    FrameEntry *fe = &writer->frame_entries[writer->frame_count];
    fe->filename_idx = filename_idx;
    fe->funcname_idx = funcname_idx;
    fe->lineno = lineno;

    /* Add to hash table (store index+1 to distinguish from NULL) */
    if (_Py_hashtable_set(writer->frame_hash, key, (void *)(uintptr_t)(*index + 1)) < 0) {
        PyMem_Free(key);
        /* Don't increment frame_count - rollback the frame entry */
        PyErr_NoMemory();
        return -1;
    }

    /* Success - now increment frame_count */
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
    /* Linear search (OK for small number of threads) */
    /* Key is (thread_id, interpreter_id) since same thread_id can exist in different interpreters */
    for (size_t i = 0; i < writer->thread_count; i++) {
        if (writer->thread_entries[i].thread_id == thread_id &&
            writer->thread_entries[i].interpreter_id == interpreter_id) {
            if (is_new) {
                *is_new = 0;
            }
            return &writer->thread_entries[i];
        }
    }

    /* Add new thread - grow array if needed */
    if (writer->thread_count >= writer->thread_capacity) {
        writer->thread_entries = grow_array(writer->thread_entries,
                                            &writer->thread_capacity,
                                            sizeof(ThreadEntry));
        if (!writer->thread_entries) {
            return NULL;
        }
    }

    ThreadEntry *entry = &writer->thread_entries[writer->thread_count];
    memset(entry, 0, sizeof(ThreadEntry));
    entry->thread_id = thread_id;
    entry->interpreter_id = interpreter_id;
    entry->prev_timestamp = writer->start_time_us;
    entry->prev_stack_capacity = MAX_STACK_DEPTH;
    entry->pending_rle_capacity = INITIAL_RLE_CAPACITY;

    entry->prev_stack = PyMem_Malloc(entry->prev_stack_capacity * sizeof(uint32_t));
    if (!entry->prev_stack) {
        PyErr_NoMemory();
        return NULL;
    }

    entry->pending_rle = PyMem_Malloc(entry->pending_rle_capacity * sizeof(PendingRLESample));
    if (!entry->pending_rle) {
        PyMem_Free(entry->prev_stack);
        PyErr_NoMemory();
        return NULL;
    }

    writer->thread_count++;
    if (is_new) {
        *is_new = 1;
    }
    return entry;
}

/* Compare two stacks and return the encoding type and parameters.
 * Sets:
 *   - shared_count: number of frames matching from bottom of stack
 *   - pop_count: frames to remove from prev stack
 *   - push_count: new frames to add
 *
 * Returns the best encoding type to use. */
static int
compare_stacks(const uint32_t *prev_stack, size_t prev_depth,
               const uint32_t *curr_stack, size_t curr_depth,
               size_t *shared_count, size_t *pop_count, size_t *push_count)
{
    /* Check for identical stacks */
    if (prev_depth == curr_depth) {
        int identical = 1;
        for (size_t i = 0; i < prev_depth; i++) {
            if (prev_stack[i] != curr_stack[i]) {
                identical = 0;
                break;
            }
        }
        if (identical) {
            *shared_count = prev_depth;
            *pop_count = 0;
            *push_count = 0;
            return STACK_REPEAT;
        }
    }

    /* Find longest common suffix (frames at the bottom/outer part of stack).
     * Stacks are stored innermost-first, so suffix is at the end. */
    size_t suffix_len = 0;
    size_t min_depth = (prev_depth < curr_depth) ? prev_depth : curr_depth;

    for (size_t i = 0; i < min_depth; i++) {
        size_t prev_idx = prev_depth - 1 - i;
        size_t curr_idx = curr_depth - 1 - i;
        if (prev_stack[prev_idx] == curr_stack[curr_idx]) {
            suffix_len++;
        } else {
            break;
        }
    }

    *shared_count = suffix_len;
    *pop_count = prev_depth - suffix_len;
    *push_count = curr_depth - suffix_len;

    /* Choose best encoding based on byte cost */
    /* STACK_FULL: 1 (type) + 1-2 (depth) + sum(frame varints) */
    /* STACK_SUFFIX: 1 (type) + 1-2 (shared) + 1-2 (new_count) + sum(new frame varints) */
    /* STACK_POP_PUSH: 1 (type) + 1-2 (pop) + 1-2 (push) + sum(new frame varints) */

    /* If no common suffix, use full stack */
    if (suffix_len == 0) {
        return STACK_FULL;
    }

    /* If only adding frames (suffix == prev_depth), use SUFFIX */
    if (*pop_count == 0 && *push_count > 0) {
        return STACK_SUFFIX;
    }

    /* If popping and/or pushing, use POP_PUSH if it saves bytes */
    /* Heuristic: POP_PUSH is better when we're modifying top frames */
    if (*pop_count > 0 || *push_count > 0) {
        /* Use full stack if sharing less than half the frames */
        if (suffix_len < curr_depth / 2) {
            return STACK_FULL;
        }
        return STACK_POP_PUSH;
    }

    return STACK_FULL;
}

/* Write common sample header: thread_id(8) + interpreter_id(4) + encoding(1).
 * Returns 0 on success, -1 on failure. */
static inline int
write_sample_header(BinaryWriter *writer, ThreadEntry *entry, uint8_t encoding)
{
    uint8_t header[SAMPLE_HEADER_FIXED_SIZE];
    memcpy(header, &entry->thread_id, 8);
    memcpy(header + 8, &entry->interpreter_id, 4);
    header[12] = encoding;
    return writer_write_bytes(writer, header, SAMPLE_HEADER_FIXED_SIZE);
}

/* Flush pending RLE samples for a thread.
 * Writes the RLE record to the output buffer.
 * Returns 0 on success, -1 on failure. */
static int
flush_pending_rle(BinaryWriter *writer, ThreadEntry *entry)
{
    if (!entry->has_pending_rle || entry->pending_rle_count == 0) {
        return 0;
    }

    /* Write RLE record:
     * [thread_id: 8] [interpreter_id: 4] [STACK_REPEAT: 1] [count: varint]
     * [timestamp_delta_1: varint] [status_1: 1] ... [timestamp_delta_N: varint] [status_N: 1]
     */

    /* Write fixed header */
    if (write_sample_header(writer, entry, STACK_REPEAT) < 0) {
        return -1;
    }

    /* Write count */
    if (writer_write_varint_u32(writer, (uint32_t)entry->pending_rle_count) < 0) {
        return -1;
    }

    /* Write timestamp deltas and status bytes */
    for (size_t i = 0; i < entry->pending_rle_count; i++) {
        if (writer_write_varint_u64(writer, entry->pending_rle[i].timestamp_delta) < 0) {
            return -1;
        }
        if (writer_write_bytes(writer, &entry->pending_rle[i].status, 1) < 0) {
            return -1;
        }
        writer->total_samples++;
    }

    /* Update stats: RLE saved writing full stacks for each repeat sample */
    writer->stats.repeat_records++;
    writer->stats.repeat_samples += entry->pending_rle_count;
    /* Each RLE sample saves writing the entire stack (prev_stack_depth frames) */
    writer->stats.frames_saved += entry->pending_rle_count * entry->prev_stack_depth;

    /* Clear pending state */
    entry->pending_rle_count = 0;
    entry->has_pending_rle = 0;

    return 0;
}

/* Write a single sample with the specified encoding.
 * Returns 0 on success, -1 on failure. */
static int
write_sample_with_encoding(BinaryWriter *writer, ThreadEntry *entry,
                           uint64_t timestamp_delta, uint8_t status,
                           int encoding_type,
                           const uint32_t *frame_indices, size_t stack_depth,
                           size_t shared_count, size_t pop_count, size_t push_count)
{
    /* Write header: thread_id (8) + interpreter_id (4) + encoding (1) + delta (varint) + status (1) */
    uint8_t header_buf[SAMPLE_HEADER_MAX_SIZE];
    memcpy(header_buf, &entry->thread_id, 8);
    memcpy(header_buf + 8, &entry->interpreter_id, 4);
    header_buf[12] = (uint8_t)encoding_type;
    size_t varint_len = encode_varint_u64(header_buf + 13, timestamp_delta);
    header_buf[13 + varint_len] = status;

    if (writer_write_bytes(writer, header_buf, 14 + varint_len) < 0) {
        return -1;
    }

    /* Write encoding-specific data */
    uint8_t frame_buf[MAX_FRAME_BUFFER_SIZE];
    size_t frame_buf_pos = 0;
    size_t frames_written = 0;

    switch (encoding_type) {
    case STACK_FULL:
        /* [depth: varint] [frame_idx: varint]... */
        frame_buf_pos += encode_varint_u32(frame_buf, (uint32_t)stack_depth);
        for (size_t i = 0; i < stack_depth; i++) {
            frame_buf_pos += encode_varint_u32(frame_buf + frame_buf_pos, frame_indices[i]);
        }
        frames_written = stack_depth;
        writer->stats.full_records++;
        break;

    case STACK_SUFFIX:
        /* [shared_count: varint] [new_count: varint] [new_frame_idx: varint]... */
        frame_buf_pos += encode_varint_u32(frame_buf, (uint32_t)shared_count);
        frame_buf_pos += encode_varint_u32(frame_buf + frame_buf_pos, (uint32_t)push_count);
        /* New frames are at the top (beginning) of current stack */
        for (size_t i = 0; i < push_count; i++) {
            frame_buf_pos += encode_varint_u32(frame_buf + frame_buf_pos, frame_indices[i]);
        }
        frames_written = push_count;
        writer->stats.suffix_records++;
        /* Saved writing shared_count frames */
        writer->stats.frames_saved += shared_count;
        break;

    case STACK_POP_PUSH:
        /* [pop_count: varint] [push_count: varint] [new_frame_idx: varint]... */
        frame_buf_pos += encode_varint_u32(frame_buf, (uint32_t)pop_count);
        frame_buf_pos += encode_varint_u32(frame_buf + frame_buf_pos, (uint32_t)push_count);
        /* New frames are at the top (beginning) of current stack */
        for (size_t i = 0; i < push_count; i++) {
            frame_buf_pos += encode_varint_u32(frame_buf + frame_buf_pos, frame_indices[i]);
        }
        frames_written = push_count;
        writer->stats.pop_push_records++;
        /* Saved writing shared_count frames (stack_depth - push_count if we had written full) */
        writer->stats.frames_saved += shared_count;
        break;

    default:
        PyErr_SetString(PyExc_RuntimeError, "Invalid stack encoding type");
        return -1;
    }

    if (writer_write_bytes(writer, frame_buf, frame_buf_pos) < 0) {
        return -1;
    }

    writer->stats.total_frames_written += frames_written;
    writer->total_samples++;
    return 0;
}

BinaryWriter *
binary_writer_create(const char *filename, uint64_t sample_interval_us, int compression_type,
                     uint64_t start_time_us)
{
    BinaryWriter *writer = PyMem_Calloc(1, sizeof(BinaryWriter));
    if (!writer) {
        PyErr_NoMemory();
        return NULL;
    }

    writer->filename = PyMem_Malloc(strlen(filename) + 1);
    if (!writer->filename) {
        PyMem_Free(writer);
        PyErr_NoMemory();
        return NULL;
    }
    strcpy(writer->filename, filename);

    writer->start_time_us = start_time_us;
    writer->sample_interval_us = sample_interval_us;
    writer->compression_type = compression_type;

    writer->write_buffer = PyMem_Malloc(WRITE_BUFFER_SIZE);
    if (!writer->write_buffer) {
        goto error;
    }
    writer->buffer_size = WRITE_BUFFER_SIZE;

    writer->string_hash = _Py_hashtable_new_full(
        string_hash_func,
        string_compare_func,
        string_key_destroy,  /* Key destroy: decref the Python string */
        NULL,                /* Value destroy: values are just indices, not pointers */
        NULL                 /* Use default allocator */
    );
    if (!writer->string_hash) {
        goto error;
    }
    writer->strings = PyMem_Malloc(INITIAL_STRING_CAPACITY * sizeof(char *));
    if (!writer->strings) {
        goto error;
    }
    writer->string_lengths = PyMem_Malloc(INITIAL_STRING_CAPACITY * sizeof(size_t));
    if (!writer->string_lengths) {
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
        goto error;
    }
    writer->frame_entries = PyMem_Malloc(INITIAL_FRAME_CAPACITY * sizeof(FrameEntry));
    if (!writer->frame_entries) {
        goto error;
    }
    writer->frame_capacity = INITIAL_FRAME_CAPACITY;

    writer->thread_entries = PyMem_Malloc(INITIAL_THREAD_CAPACITY * sizeof(ThreadEntry));
    if (!writer->thread_entries) {
        goto error;
    }
    writer->thread_capacity = INITIAL_THREAD_CAPACITY;

    /* Initialize compression if requested */
    if (compression_type == COMPRESSION_ZSTD) {
        if (writer_init_zstd(writer) < 0) {
            goto error;
        }
    }

    /* Open file */
    writer->fp = fopen(filename, "wb");
    if (!writer->fp) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
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

    /* Write placeholder header - release GIL during I/O */
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

        /* Get filename, location, funcname from FrameInfo using unchecked access */
        PyObject *filename = PyStructSequence_GET_ITEM(frame_info, 0);
        PyObject *location = PyStructSequence_GET_ITEM(frame_info, 1);
        PyObject *funcname = PyStructSequence_GET_ITEM(frame_info, 2);

        /* Extract lineno from location (can be None for synthetic frames) */
        int32_t lineno = 0;
        if (location != Py_None) {
            /* Use unchecked access - first element is lineno */
            PyObject *lineno_obj = PyTuple_Check(location) ?
                PyTuple_GET_ITEM(location, 0) :
                PyStructSequence_GET_ITEM(location, 0);
            lineno = (int32_t)PyLong_AsLong(lineno_obj);
            if (UNLIKELY(PyErr_Occurred() != NULL)) {
                PyErr_Clear();
                lineno = 0;
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

        /* Intern frame */
        uint32_t frame_idx;
        if (writer_intern_frame(writer, filename_idx, funcname_idx, lineno, &frame_idx) < 0) {
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
    /* Get thread_id, status, frame_list from ThreadInfo using unchecked access */
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
    uint8_t status = (uint8_t)status_long;

    /* Get or create thread entry */
    int is_new_thread = 0;
    ThreadEntry *entry = writer_get_or_create_thread_entry(
        writer, thread_id, interpreter_id, &is_new_thread);
    if (!entry) {
        return -1;
    }

    /* Calculate timestamp delta */
    uint64_t delta = timestamp_us - entry->prev_timestamp;
    entry->prev_timestamp = timestamp_us;

    /* Process frames and build current stack */
    uint32_t curr_stack[MAX_STACK_DEPTH];
    size_t curr_depth;
    if (build_frame_stack(writer, frame_list, curr_stack, &curr_depth) < 0) {
        return -1;
    }

    /* Compare with previous stack to determine encoding */
    size_t shared_count, pop_count, push_count;
    int encoding = compare_stacks(
        entry->prev_stack, entry->prev_stack_depth,
        curr_stack, curr_depth,
        &shared_count, &pop_count, &push_count);

    if (encoding == STACK_REPEAT && !is_new_thread) {
        /* Buffer this sample for RLE */
        if (GROW_ARRAY(entry->pending_rle, entry->pending_rle_count,
                       entry->pending_rle_capacity, PendingRLESample) < 0) {
            return -1;
        }
        entry->pending_rle[entry->pending_rle_count].timestamp_delta = delta;
        entry->pending_rle[entry->pending_rle_count].status = status;
        entry->pending_rle_count++;
        entry->has_pending_rle = 1;
    } else {
        /* Stack changed - flush any pending RLE first */
        if (entry->has_pending_rle) {
            if (flush_pending_rle(writer, entry) < 0) {
                return -1;
            }
        }

        /* Write this sample with the appropriate encoding */
        if (write_sample_with_encoding(writer, entry, delta, status, encoding,
                                       curr_stack, curr_depth,
                                       shared_count, pop_count, push_count) < 0) {
            return -1;
        }

        /* Update previous stack */
        memcpy(entry->prev_stack, curr_stack, curr_depth * sizeof(uint32_t));
        entry->prev_stack_depth = curr_depth;
    }

    return 0;
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

        /* Get interpreter_id and threads from InterpreterInfo using unchecked access */
        PyObject *interp_id_obj = PyStructSequence_GET_ITEM(interp_info, 0);
        PyObject *threads = PyStructSequence_GET_ITEM(interp_info, 1);

        unsigned long interp_id_long = PyLong_AsUnsignedLong(interp_id_obj);
        if (interp_id_long == (unsigned long)-1 && PyErr_Occurred()) {
            return -1;
        }
        /* Bounds check: interpreter_id is stored as uint32_t in binary format */
        if (interp_id_long > UINT32_MAX) {
            PyErr_Format(PyExc_OverflowError,
                "interpreter_id %lu exceeds maximum value %lu",
                interp_id_long, (unsigned long)UINT32_MAX);
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
    /* Flush any pending RLE for all threads */
    for (size_t i = 0; i < writer->thread_count; i++) {
        if (writer->thread_entries[i].has_pending_rle) {
            if (flush_pending_rle(writer, &writer->thread_entries[i]) < 0) {
                return -1;
            }
        }
    }

    /* Flush remaining buffer */
    if (writer_flush_buffer(writer) < 0) {
        return -1;
    }

#ifdef HAVE_ZSTD
    /* Finalize compression stream */
    if (writer->compression_type == COMPRESSION_ZSTD && writer->zstd.cctx) {
        ZSTD_inBuffer input = { NULL, 0, 0 };
        size_t remaining;

        do {
            ZSTD_outBuffer output = {
                writer->zstd.compressed_buffer,
                writer->zstd.compressed_buffer_size,
                0
            };

            remaining = ZSTD_compressStream2(writer->zstd.cctx, &output, &input, ZSTD_e_end);

            if (ZSTD_isError(remaining)) {
                PyErr_Format(PyExc_IOError, "zstd finalization error: %s",
                             ZSTD_getErrorName(remaining));
                return -1;
            }

            if (output.pos > 0) {
                if (fwrite_checked_allow_threads(writer->zstd.compressed_buffer, output.pos, writer->fp) < 0) {
                    return -1;
                }
            }
        } while (remaining > 0);
    }
#endif

    /* Get offset for string table (use 64-bit file position for >2GB files) */
    file_offset_t string_table_offset = FTELL64(writer->fp);
    if (string_table_offset < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    /* Write string table - release GIL during potentially large writes */
    for (size_t i = 0; i < writer->string_count; i++) {
        uint8_t len_buf[10];
        size_t len_size = encode_varint_u32(len_buf, (uint32_t)writer->string_lengths[i]);
        if (fwrite_checked_allow_threads(len_buf, len_size, writer->fp) < 0 ||
            fwrite_checked_allow_threads(writer->strings[i], writer->string_lengths[i], writer->fp) < 0) {
            return -1;
        }
    }

    /* Get offset for frame table */
    file_offset_t frame_table_offset = FTELL64(writer->fp);
    if (frame_table_offset < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    /* Write frame table - release GIL during writes */
    for (size_t i = 0; i < writer->frame_count; i++) {
        FrameEntry *entry = &writer->frame_entries[i];
        uint8_t buf[30];
        size_t pos = encode_varint_u32(buf, entry->filename_idx);
        pos += encode_varint_u32(buf + pos, entry->funcname_idx);
        pos += encode_varint_i32(buf + pos, entry->lineno);
        if (fwrite_checked_allow_threads(buf, pos, writer->fp) < 0) {
            return -1;
        }
    }

    /* Write footer (32 bytes): string_count(4) + frame_count(4) + file_size(8) + checksum(16) */
    file_offset_t footer_offset = FTELL64(writer->fp);
    if (footer_offset < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    uint64_t file_size = (uint64_t)footer_offset + 32;
    uint8_t footer[32] = {0};
    memcpy(footer + 0, &writer->string_count, 4);
    memcpy(footer + 4, &writer->frame_count, 4);
    memcpy(footer + 8, &file_size, 8);
    /* bytes 16-31: checksum placeholder (zeros) */
    if (fwrite_checked_allow_threads(footer, 32, writer->fp) < 0) {
        return -1;
    }

    /* Write header at file start */
    if (FSEEK64(writer->fp, 0, SEEK_SET) < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }

    /* Convert file offsets to uint64_t for portable header format */
    uint64_t string_table_offset_u64 = (uint64_t)string_table_offset;
    uint64_t frame_table_offset_u64 = (uint64_t)frame_table_offset;

    uint8_t header[52] = {0};
    uint32_t magic = BINARY_FORMAT_MAGIC;
    uint32_t version = BINARY_FORMAT_VERSION;
    memcpy(header + 0, &magic, 4);
    memcpy(header + 4, &version, 4);
    memcpy(header + 8, &writer->start_time_us, 8);
    memcpy(header + 16, &writer->sample_interval_us, 8);
    memcpy(header + 24, &writer->total_samples, 4);
    memcpy(header + 28, &writer->thread_count, 4);
    memcpy(header + 32, &string_table_offset_u64, 8);
    memcpy(header + 40, &frame_table_offset_u64, 8);
    memcpy(header + 48, &writer->compression_type, 4);
    if (fwrite_checked_allow_threads(header, 52, writer->fp) < 0) {
        return -1;
    }

    /* Close file */
    if (fclose(writer->fp) != 0) {
        writer->fp = NULL;
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    writer->fp = NULL;

    return 0;
}

void
binary_writer_destroy(BinaryWriter *writer)
{
    if (!writer) {
        return;
    }

    if (writer->fp) {
        fclose(writer->fp);
    }

    PyMem_Free(writer->filename);
    PyMem_Free(writer->write_buffer);

#ifdef HAVE_ZSTD
    if (writer->zstd.cctx) {
        ZSTD_freeCCtx(writer->zstd.cctx);
    }
    PyMem_Free(writer->zstd.compressed_buffer);
#endif

    /* Free string hash table (destroys keys which decrefs Python strings) */
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

    /* Free frame hash table (destroys keys which frees FrameKey structs) */
    if (writer->frame_hash) {
        _Py_hashtable_destroy(writer->frame_hash);
    }
    PyMem_Free(writer->frame_entries);

    /* Free per-thread buffers */
    if (writer->thread_entries) {
        for (size_t i = 0; i < writer->thread_count; i++) {
            PyMem_Free(writer->thread_entries[i].prev_stack);
            PyMem_Free(writer->thread_entries[i].pending_rle);
        }
        PyMem_Free(writer->thread_entries);
    }

    PyMem_Free(writer);
}

