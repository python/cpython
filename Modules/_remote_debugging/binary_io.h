/******************************************************************************
 * Python Remote Debugging Module - Binary I/O Header
 *
 * This header provides declarations for high-performance binary file I/O
 * for profiling data with optional zstd streaming compression.
 ******************************************************************************/

#ifndef Py_BINARY_IO_H
#define Py_BINARY_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"
#include "pycore_hashtable.h"
#include <stdint.h>
#include <stdio.h>

/* ============================================================================
 * BINARY FORMAT CONSTANTS
 * ============================================================================ */

#define BINARY_FORMAT_MAGIC     0x54414348  /* "TACH" (Tachyon) */
#define BINARY_FORMAT_VERSION   2

/* Buffer sizes: 512KB balances syscall amortization against memory use,
 * and aligns well with filesystem block sizes and zstd dictionary windows */
#define WRITE_BUFFER_SIZE       (512 * 1024)
#define COMPRESSED_BUFFER_SIZE  (512 * 1024)

/* Compression types */
#define COMPRESSION_NONE        0
#define COMPRESSION_ZSTD        1

/* Stack encoding types for delta compression */
#define STACK_REPEAT            0x00  /* RLE: identical to previous, with count */
#define STACK_FULL              0x01  /* Full stack (first sample or no match) */
#define STACK_SUFFIX            0x02  /* Shares N frames from bottom */
#define STACK_POP_PUSH          0x03  /* Remove M frames, add N frames */

/* Maximum stack depth we'll buffer for delta encoding */
#define MAX_STACK_DEPTH         256

/* Initial capacity for RLE pending buffer */
#define INITIAL_RLE_CAPACITY    64

/* Initial capacities for dynamic arrays - sized to reduce reallocations */
#define INITIAL_STRING_CAPACITY 4096
#define INITIAL_FRAME_CAPACITY  4096
#define INITIAL_THREAD_CAPACITY 256

/* ============================================================================
 * STATISTICS STRUCTURES
 * ============================================================================ */

/* Writer statistics - tracks encoding efficiency */
typedef struct {
    uint64_t repeat_records;      /* Number of RLE repeat records written */
    uint64_t repeat_samples;      /* Total samples encoded via RLE */
    uint64_t full_records;        /* Number of full stack records */
    uint64_t suffix_records;      /* Number of suffix match records */
    uint64_t pop_push_records;    /* Number of pop-push records */
    uint64_t total_frames_written;/* Total frame indices written */
    uint64_t frames_saved;        /* Frames avoided due to delta encoding */
    uint64_t bytes_written;       /* Total bytes written (before compression) */
} BinaryWriterStats;

/* Reader statistics - tracks reconstruction performance */
typedef struct {
    uint64_t repeat_records;      /* RLE records decoded */
    uint64_t repeat_samples;      /* Samples decoded from RLE */
    uint64_t full_records;        /* Full stack records decoded */
    uint64_t suffix_records;      /* Suffix match records decoded */
    uint64_t pop_push_records;    /* Pop-push records decoded */
    uint64_t total_samples;       /* Total samples reconstructed */
    uint64_t stack_reconstructions; /* Number of stack array reconstructions */
} BinaryReaderStats;

/* ============================================================================
 * PLATFORM ABSTRACTION
 * ============================================================================ */

#if defined(__linux__) || defined(__APPLE__)
    #include <sys/mman.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #define USE_MMAP 1
#else
    #define USE_MMAP 0
#endif

/* 64-bit file position support for files larger than 2GB.
 * On POSIX: use ftello/fseeko with off_t (already 64-bit on 64-bit systems)
 * On Windows: use _ftelli64/_fseeki64 with __int64 */
#if defined(_WIN32) || defined(_WIN64)
    #include <io.h>
    typedef __int64 file_offset_t;
    #define FTELL64(fp) _ftelli64(fp)
    #define FSEEK64(fp, offset, whence) _fseeki64(fp, offset, whence)
#else
    /* POSIX - off_t is 64-bit on 64-bit systems, ftello/fseeko handle large files */
    typedef off_t file_offset_t;
    #define FTELL64(fp) ftello(fp)
    #define FSEEK64(fp, offset, whence) fseeko(fp, offset, whence)
#endif

/* Forward declare zstd types if available */
#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

/* Branch prediction hints - same as Objects/obmalloc.c */
#if (defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 2))) && defined(__OPTIMIZE__)
#  define UNLIKELY(value) __builtin_expect((value), 0)
#  define LIKELY(value) __builtin_expect((value), 1)
#else
#  define UNLIKELY(value) (value)
#  define LIKELY(value) (value)
#endif

/* ============================================================================
 * BINARY WRITER STRUCTURES
 * ============================================================================ */

/* zstd compression state (only used if HAVE_ZSTD defined) */
typedef struct {
#ifdef HAVE_ZSTD
    ZSTD_CCtx *cctx;  /* Modern API: CCtx and CStream are the same since v1.3.0 */
#else
    void *cctx;  /* Placeholder */
#endif
    uint8_t *compressed_buffer;
    size_t compressed_buffer_size;
} ZstdCompressor;

/* Frame entry - combines all frame data for better cache locality */
typedef struct {
    uint32_t filename_idx;
    uint32_t funcname_idx;
    int32_t lineno;
} FrameEntry;

/* Frame key for hash table lookup */
typedef struct {
    uint32_t filename_idx;
    uint32_t funcname_idx;
    int32_t lineno;
} FrameKey;

/* Pending RLE sample - buffered for run-length encoding */
typedef struct {
    uint64_t timestamp_delta;
    uint8_t status;
} PendingRLESample;

/* Thread entry - tracks per-thread state for delta encoding */
typedef struct {
    uint64_t thread_id;
    uint64_t prev_timestamp;
    uint32_t interpreter_id;

    /* Previous stack for delta encoding (frame indices, innermost first) */
    uint32_t *prev_stack;
    size_t prev_stack_depth;
    size_t prev_stack_capacity;

    /* RLE pending buffer - samples waiting to be written as a repeat group */
    PendingRLESample *pending_rle;
    size_t pending_rle_count;
    size_t pending_rle_capacity;
    int has_pending_rle;  /* Flag: do we have buffered repeats? */
} ThreadEntry;

/* Main binary writer structure */
typedef struct {
    FILE *fp;
    char *filename;

    /* Write buffer for batched I/O */
    uint8_t *write_buffer;
    size_t buffer_pos;
    size_t buffer_size;

    /* Compression */
    int compression_type;
    ZstdCompressor zstd;

    /* Metadata */
    uint64_t start_time_us;
    uint64_t sample_interval_us;
    uint32_t total_samples;

    /* String hash table: PyObject* -> uint32_t index */
    _Py_hashtable_t *string_hash;
    /* String storage: array of UTF-8 encoded strings */
    char **strings;
    size_t *string_lengths;
    size_t string_count;
    size_t string_capacity;

    /* Frame hash table: FrameKey* -> uint32_t index */
    _Py_hashtable_t *frame_hash;
    /* Frame storage: combined struct for better cache locality */
    FrameEntry *frame_entries;
    size_t frame_count;
    size_t frame_capacity;

    /* Thread timestamp tracking for delta encoding - combined for cache locality */
    ThreadEntry *thread_entries;
    size_t thread_count;
    size_t thread_capacity;

    /* Statistics */
    BinaryWriterStats stats;
} BinaryWriter;

/* ============================================================================
 * BINARY READER STRUCTURES
 * ============================================================================ */

/* Per-thread state for stack reconstruction during replay */
typedef struct {
    uint64_t thread_id;
    uint32_t interpreter_id;
    uint64_t prev_timestamp;

    /* Reconstructed stack buffer (frame indices, innermost first) */
    uint32_t *current_stack;
    size_t current_stack_depth;
    size_t current_stack_capacity;
} ReaderThreadState;

/* Main binary reader structure */
typedef struct {
    char *filename;

#if USE_MMAP
    int fd;
    uint8_t *mapped_data;
    size_t mapped_size;
#else
    FILE *fp;
    uint8_t *file_data;
    size_t file_size;
#endif

    /* Decompression state */
    int compression_type;
    /* Note: ZSTD_DCtx is not stored - created/freed during decompression */
    uint8_t *decompressed_data;
    size_t decompressed_size;

    /* Header metadata */
    uint64_t start_time_us;
    uint64_t sample_interval_us;
    uint32_t sample_count;
    uint32_t thread_count;
    uint64_t string_table_offset;
    uint64_t frame_table_offset;

    /* Parsed string table: array of Python string objects */
    PyObject **strings;
    uint32_t strings_count;

    /* Parsed frame table: packed as [filename_idx, funcname_idx, lineno] */
    uint32_t *frame_data;
    uint32_t frames_count;

    /* Sample data region */
    uint8_t *sample_data;
    size_t sample_data_size;

    /* Per-thread state for stack reconstruction (used during replay) */
    ReaderThreadState *thread_states;
    size_t thread_state_count;
    size_t thread_state_capacity;

    /* Statistics */
    BinaryReaderStats stats;
} BinaryReader;

/* ============================================================================
 * VARINT ENCODING/DECODING (INLINE FOR PERFORMANCE)
 * ============================================================================ */

/* Encode unsigned 64-bit varint (LEB128). Returns bytes written. */
static inline size_t
encode_varint_u64(uint8_t *buf, uint64_t value)
{
    /* Fast path for single-byte values (0-127) - very common case */
    if (value < 0x80) {
        buf[0] = (uint8_t)value;
        return 1;
    }

    size_t i = 0;
    while (value >= 0x80) {
        buf[i++] = (uint8_t)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    buf[i++] = (uint8_t)(value & 0x7F);
    return i;
}

/* Encode unsigned 32-bit varint. Returns bytes written. */
static inline size_t
encode_varint_u32(uint8_t *buf, uint32_t value)
{
    return encode_varint_u64(buf, value);
}

/* Encode signed 32-bit varint (zigzag encoding). Returns bytes written. */
static inline size_t
encode_varint_i32(uint8_t *buf, int32_t value)
{
    /* Zigzag encode: map signed to unsigned */
    uint32_t zigzag = ((uint32_t)value << 1) ^ (uint32_t)(value >> 31);
    return encode_varint_u32(buf, zigzag);
}

/* Decode unsigned 64-bit varint. Updates offset only on success. Returns value.
 * On error (overflow or incomplete), offset is NOT updated, allowing callers
 * to detect errors via (offset == prev_offset) check.
 * On success, sets *error to 0 if error is non-NULL.
 * On error, sets *error to 1 if error is non-NULL. */
static inline uint64_t
decode_varint_u64_ex(const uint8_t *data, size_t *offset, size_t max_size, int *error)
{
    size_t pos = *offset;
    uint64_t result = 0;
    int shift = 0;

    /* Fast path for single-byte varints (0-127) - most common case */
    if (LIKELY(pos < max_size && (data[pos] & 0x80) == 0)) {
        *offset = pos + 1;
        if (error) *error = 0;
        return data[pos];
    }

    while (pos < max_size) {
        uint8_t byte = data[pos++];
        result |= (uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            *offset = pos;
            if (error) *error = 0;
            return result;
        }
        shift += 7;
        if (UNLIKELY(shift >= 64)) {
            /* Overflow - do NOT update offset so caller can detect error */
            if (error) *error = 1;
            return 0;
        }
    }

    /* Incomplete varint - do NOT update offset so caller can detect error */
    if (error) *error = 1;
    return 0;
}

/* Backward-compatible wrapper that sets PyErr on error.
 * Callers should check PyErr_Occurred() after batch operations. */
static inline uint64_t
decode_varint_u64(const uint8_t *data, size_t *offset, size_t max_size)
{
    int error = 0;
    uint64_t result = decode_varint_u64_ex(data, offset, max_size, &error);
    if (UNLIKELY(error)) {
        PyErr_SetString(PyExc_ValueError, "Invalid or incomplete varint in binary data");
    }
    return result;
}

/* Decode unsigned 32-bit varint with explicit error handling.
 * If value exceeds UINT32_MAX, treats as error: offset is NOT updated,
 * *error is set to 1, allowing callers to detect via (offset == prev_offset). */
static inline uint32_t
decode_varint_u32_ex(const uint8_t *data, size_t *offset, size_t max_size, int *error)
{
    size_t saved_offset = *offset;
    uint64_t value = decode_varint_u64_ex(data, offset, max_size, error);
    if (error && *error) {
        /* decode_varint_u64_ex already handled the error, offset unchanged */
        return 0;
    }
    if (UNLIKELY(value > UINT32_MAX)) {
        /* Value overflow - restore offset so caller can detect error */
        *offset = saved_offset;
        if (error) *error = 1;
        return 0;
    }
    return (uint32_t)value;
}

/* Backward-compatible wrapper that sets PyErr on error. */
static inline uint32_t
decode_varint_u32(const uint8_t *data, size_t *offset, size_t max_size)
{
    int error = 0;
    uint32_t result = decode_varint_u32_ex(data, offset, max_size, &error);
    if (UNLIKELY(error)) {
        PyErr_SetString(PyExc_ValueError, "Invalid or incomplete varint in binary data");
    }
    return result;
}

/* Decode signed 32-bit varint (zigzag) with explicit error handling. */
static inline int32_t
decode_varint_i32_ex(const uint8_t *data, size_t *offset, size_t max_size, int *error)
{
    uint32_t zigzag = decode_varint_u32_ex(data, offset, max_size, error);
    /* Zigzag decode */
    return (int32_t)((zigzag >> 1) ^ -(int32_t)(zigzag & 1));
}

/* Backward-compatible wrapper that sets PyErr on error. */
static inline int32_t
decode_varint_i32(const uint8_t *data, size_t *offset, size_t max_size)
{
    int error = 0;
    int32_t result = decode_varint_i32_ex(data, offset, max_size, &error);
    if (UNLIKELY(error)) {
        PyErr_SetString(PyExc_ValueError, "Invalid or incomplete varint in binary data");
    }
    return result;
}

/* ============================================================================
 * SHARED UTILITY FUNCTIONS
 * ============================================================================ */

/* Generic array growth - returns new pointer or NULL (sets PyErr_NoMemory)
 * Includes overflow checking for capacity doubling and allocation size. */
static inline void *
grow_array(void *ptr, size_t *capacity, size_t elem_size)
{
    size_t old_cap = *capacity;

    /* Check for overflow when doubling capacity */
    if (old_cap > SIZE_MAX / 2) {
        PyErr_SetString(PyExc_OverflowError, "Array capacity overflow");
        return NULL;
    }
    size_t new_cap = old_cap * 2;

    /* Check for overflow when calculating allocation size */
    if (new_cap > SIZE_MAX / elem_size) {
        PyErr_SetString(PyExc_OverflowError, "Array allocation size overflow");
        return NULL;
    }

    void *new_ptr = PyMem_Realloc(ptr, new_cap * elem_size);
    if (new_ptr) {
        *capacity = new_cap;
    } else {
        PyErr_NoMemory();
    }
    return new_ptr;
}

/* Macro wrapper for type safety with grow_array */
#define GROW_ARRAY(ptr, count, capacity, type) \
    ((count) < (capacity) ? 0 : \
     ((ptr) = grow_array((ptr), &(capacity), sizeof(type))) ? 0 : -1)

/* ============================================================================
 * BINARY WRITER API
 * ============================================================================ */

/*
 * Create a new binary writer.
 *
 * Arguments:
 *   filename: Path to output file
 *   sample_interval_us: Sampling interval in microseconds
 *   compression_type: COMPRESSION_NONE or COMPRESSION_ZSTD
 *   start_time_us: Start timestamp in microseconds (from time.monotonic() * 1e6)
 *
 * Returns:
 *   New BinaryWriter* on success, NULL on failure (PyErr set)
 */
BinaryWriter *binary_writer_create(
    const char *filename,
    uint64_t sample_interval_us,
    int compression_type,
    uint64_t start_time_us
);

/*
 * Write a sample to the binary file.
 *
 * Arguments:
 *   writer: Writer from binary_writer_create
 *   stack_frames: List of InterpreterInfo struct sequences
 *   timestamp_us: Current timestamp in microseconds (from time.monotonic() * 1e6)
 *
 * Returns:
 *   0 on success, -1 on failure (PyErr set)
 */
int binary_writer_write_sample(
    BinaryWriter *writer,
    PyObject *stack_frames,
    uint64_t timestamp_us
);

/*
 * Finalize and close the binary file.
 * Writes string/frame tables, footer, and updates header.
 *
 * Arguments:
 *   writer: Writer to finalize
 *
 * Returns:
 *   0 on success, -1 on failure (PyErr set)
 */
int binary_writer_finalize(BinaryWriter *writer);

/*
 * Destroy a binary writer and free all resources.
 * Safe to call even if writer is partially initialized.
 *
 * Arguments:
 *   writer: Writer to destroy (may be NULL)
 */
void binary_writer_destroy(BinaryWriter *writer);

/* ============================================================================
 * BINARY READER API
 * ============================================================================ */

/*
 * Open a binary file for reading.
 *
 * Arguments:
 *   filename: Path to input file
 *
 * Returns:
 *   New BinaryReader* on success, NULL on failure (PyErr set)
 */
BinaryReader *binary_reader_open(const char *filename);

/*
 * Replay samples from binary file through a collector.
 *
 * Arguments:
 *   reader: Reader from binary_reader_open
 *   collector: Python collector with collect() method
 *   progress_callback: Optional callable(current, total) or NULL
 *
 * Returns:
 *   Number of samples replayed on success, -1 on failure (PyErr set)
 */
Py_ssize_t binary_reader_replay(
    BinaryReader *reader,
    PyObject *collector,
    PyObject *progress_callback
);

/*
 * Get metadata about the binary file.
 *
 * Arguments:
 *   reader: Reader from binary_reader_open
 *
 * Returns:
 *   Dict with file metadata on success, NULL on failure (PyErr set)
 */
PyObject *binary_reader_get_info(BinaryReader *reader);

/*
 * Close a binary reader and free all resources.
 *
 * Arguments:
 *   reader: Reader to close (may be NULL)
 */
void binary_reader_close(BinaryReader *reader);

/* ============================================================================
 * STATISTICS FUNCTIONS
 * ============================================================================ */

/*
 * Get writer statistics as a Python dict.
 *
 * Arguments:
 *   writer: Writer to get stats from
 *
 * Returns:
 *   Dict with statistics on success, NULL on failure (PyErr set)
 */
PyObject *binary_writer_get_stats(BinaryWriter *writer);

/*
 * Get reader statistics as a Python dict.
 *
 * Arguments:
 *   reader: Reader to get stats from
 *
 * Returns:
 *   Dict with statistics on success, NULL on failure (PyErr set)
 */
PyObject *binary_reader_get_stats(BinaryReader *reader);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/*
 * Check if zstd compression is available.
 *
 * Returns:
 *   1 if zstd available, 0 otherwise
 */
int binary_io_zstd_available(void);

/*
 * Get the best available compression type.
 *
 * Returns:
 *   COMPRESSION_ZSTD if available, COMPRESSION_NONE otherwise
 */
int binary_io_get_best_compression(void);

#ifdef __cplusplus
}
#endif

#endif /* Py_BINARY_IO_H */
