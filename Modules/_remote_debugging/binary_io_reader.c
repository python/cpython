/******************************************************************************
 * Python Remote Debugging Module - Binary Reader Implementation
 *
 * High-performance binary file reader for profiling data with optional zstd
 * decompression.
 ******************************************************************************/

#ifndef Py_BUILD_CORE_MODULE
#  define Py_BUILD_CORE_MODULE
#endif

#include "binary_io.h"
#include "_remote_debugging.h"
#include "pycore_bitutils.h"   /* _Py_bswap32, _Py_bswap64 for cross-endian reading */
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

/* Progress callback frequency */
#define PROGRESS_CALLBACK_INTERVAL 1000

/* Cap per-batch RLE samples to bound the timestamp list (gh-151378) */
#define MAX_RLE_BATCH_SAMPLES 8192

#define FRAME_INFO_CACHE_SIZE 1024

/* ============================================================================
 * BINARY READER IMPLEMENTATION
 * ============================================================================ */

static inline int
reader_parse_header(BinaryReader *reader, const uint8_t *data, size_t file_size)
{
    if (file_size < FILE_HEADER_PLACEHOLDER_SIZE) {
        PyErr_SetString(PyExc_ValueError, "File too small for header");
        return -1;
    }

    /* Use memcpy to avoid strict aliasing violations and unaligned access */
    uint32_t magic;
    uint32_t version;
    memcpy(&magic, &data[HDR_OFF_MAGIC], HDR_SIZE_MAGIC);
    memcpy(&version, &data[HDR_OFF_VERSION], HDR_SIZE_VERSION);

    /* Detect endianness from magic number */
    if (magic == BINARY_FORMAT_MAGIC) {
        reader->needs_swap = 0;
    } else if (magic == BINARY_FORMAT_MAGIC_SWAPPED) {
        reader->needs_swap = 1;
        version = _Py_bswap32(version);
    } else {
        PyErr_Format(PyExc_ValueError, "Invalid magic number: 0x%08x", magic);
        return -1;
    }

    if (version != BINARY_FORMAT_VERSION) {
        if (version > BINARY_FORMAT_VERSION && file_size >= HDR_OFF_PY_MICRO + 1) {
            /* Newer format - try to read Python version for better error */
            uint8_t py_major = data[HDR_OFF_PY_MAJOR];
            uint8_t py_minor = data[HDR_OFF_PY_MINOR];
            uint8_t py_micro = data[HDR_OFF_PY_MICRO];
            PyErr_Format(PyExc_ValueError,
                "Binary file was created with Python %u.%u.%u (format version %u), "
                "but this is Python %d.%d.%d (format version %d)",
                py_major, py_minor, py_micro, version,
                PY_MAJOR_VERSION, PY_MINOR_VERSION, PY_MICRO_VERSION,
                BINARY_FORMAT_VERSION);
        } else {
            PyErr_Format(PyExc_ValueError,
                "Unsupported format version %u (this reader supports version %d)",
                version, BINARY_FORMAT_VERSION);
        }
        return -1;
    }

    reader->py_major = data[HDR_OFF_PY_MAJOR];
    reader->py_minor = data[HDR_OFF_PY_MINOR];
    reader->py_micro = data[HDR_OFF_PY_MICRO];

    /* Read header fields with byte-swapping if needed */
    uint64_t start_time_us, sample_interval_us, string_table_offset, frame_table_offset;
    uint64_t sample_count;
    uint32_t thread_count, compression_type, profiling_config;

    memcpy(&start_time_us, &data[HDR_OFF_START_TIME], HDR_SIZE_START_TIME);
    memcpy(&sample_interval_us, &data[HDR_OFF_INTERVAL], HDR_SIZE_INTERVAL);
    memcpy(&sample_count, &data[HDR_OFF_SAMPLES], HDR_SIZE_SAMPLES);
    memcpy(&thread_count, &data[HDR_OFF_THREADS], HDR_SIZE_THREADS);
    memcpy(&string_table_offset, &data[HDR_OFF_STR_TABLE], HDR_SIZE_STR_TABLE);
    memcpy(&frame_table_offset, &data[HDR_OFF_FRAME_TABLE], HDR_SIZE_FRAME_TABLE);
    memcpy(&compression_type, &data[HDR_OFF_COMPRESSION], HDR_SIZE_COMPRESSION);
    memcpy(&profiling_config, &data[HDR_OFF_CONFIG], HDR_SIZE_CONFIG);

    reader->start_time_us = SWAP64_IF(reader->needs_swap, start_time_us);
    reader->sample_interval_us = SWAP64_IF(reader->needs_swap, sample_interval_us);
    reader->sample_count = SWAP64_IF(reader->needs_swap, sample_count);
    reader->thread_count = SWAP32_IF(reader->needs_swap, thread_count);
    reader->string_table_offset = SWAP64_IF(reader->needs_swap, string_table_offset);
    reader->frame_table_offset = SWAP64_IF(reader->needs_swap, frame_table_offset);
    reader->compression_type = (int)SWAP32_IF(reader->needs_swap, compression_type);
    profiling_config = SWAP32_IF(reader->needs_swap, profiling_config);
    uint32_t profiling_mode =
        profiling_config & PROFILING_CONFIG_MODE_MASK;
    if (profiling_mode > PROFILING_MODE_EXCEPTION + 1) {
        PyErr_Format(PyExc_ValueError,
                     "Invalid profiling mode in header: %u", profiling_mode);
        return -1;
    }
    reader->profiling_mode = (int)profiling_mode - 1;
    reader->capture_features =
        profiling_config & PROFILING_CONFIG_FEATURES_KNOWN
        ? (int)((profiling_config >> PROFILING_CONFIG_FEATURES_SHIFT) &
                PROFILING_FEATURE_MASK)
        : -1;

    return 0;
}

static inline int
reader_parse_footer(BinaryReader *reader, const uint8_t *data, size_t file_size)
{
    if (file_size < FILE_FOOTER_SIZE) {
        PyErr_SetString(PyExc_ValueError, "File too small for footer");
        return -1;
    }

    const uint8_t *footer = data + file_size - FILE_FOOTER_SIZE;
    /* Use memcpy to avoid strict aliasing violations */
    uint32_t strings_count, frames_count;
    memcpy(&strings_count, &footer[FTR_OFF_STRINGS], FTR_SIZE_STRINGS);
    memcpy(&frames_count, &footer[FTR_OFF_FRAMES], FTR_SIZE_FRAMES);

    reader->strings_count = SWAP32_IF(reader->needs_swap, strings_count);
    reader->frames_count = SWAP32_IF(reader->needs_swap, frames_count);

    return 0;
}

static inline int
reader_parse_profile_stats(BinaryReader *reader, const uint8_t *data,
                           size_t file_size)
{
    size_t minimum_size = FILE_FOOTER_SIZE + PROFILE_STATS_V1_SIZE;
    if (file_size < minimum_size) {
        return 0;
    }

    const uint8_t *stats_end = data + file_size - FILE_FOOTER_SIZE;
    const uint8_t *stats_tail = stats_end - PROFILE_STATS_MAGIC_SIZE -
                                PST_SIZE_VERSION - PST_SIZE_SIZE;
    if (memcmp(stats_tail,
               PROFILE_STATS_MAGIC, PROFILE_STATS_MAGIC_SIZE) != 0) {
        return 0;
    }

    uint32_t version, size;
    memcpy(&version, stats_tail + PROFILE_STATS_MAGIC_SIZE,
           PST_SIZE_VERSION);
    memcpy(&size, stats_tail + PROFILE_STATS_MAGIC_SIZE + PST_SIZE_VERSION,
           PST_SIZE_SIZE);
    version = SWAP32_IF(reader->needs_swap, version);
    size = SWAP32_IF(reader->needs_swap, size);

    if (size < PROFILE_STATS_V1_SIZE ||
        size > file_size - FILE_FOOTER_SIZE) {
        PyErr_SetString(PyExc_ValueError,
                        "Invalid profiling statistics size");
        return -1;
    }
    if (version != PROFILE_STATS_VERSION) {
        return 0;
    }

    const uint8_t *trailer = stats_end - size;
    uint64_t duration_bits, sample_rate_bits;
    memcpy(&duration_bits, trailer + PST_OFF_DURATION, PST_SIZE_DURATION);
    memcpy(&sample_rate_bits, trailer + PST_OFF_SAMPLE_RATE,
           PST_SIZE_SAMPLE_RATE);
    duration_bits = SWAP64_IF(reader->needs_swap, duration_bits);
    sample_rate_bits = SWAP64_IF(reader->needs_swap, sample_rate_bits);
    memcpy(&reader->duration_sec, &duration_bits, sizeof(duration_bits));
    memcpy(&reader->sample_rate, &sample_rate_bits, sizeof(sample_rate_bits));

    if (!isfinite(reader->duration_sec) || reader->duration_sec < 0.0 ||
        !isfinite(reader->sample_rate) || reader->sample_rate < 0.0) {
        PyErr_SetString(PyExc_ValueError,
                        "Invalid profiling statistics values");
        return -1;
    }
    reader->has_profile_stats = 1;

    if (size >= PROFILE_STATS_SIZE) {
        uint64_t error_rate_bits, missed_samples_bits;
        uint32_t present;
        memcpy(&error_rate_bits, trailer + PST_OFF_ERROR_RATE,
               PST_SIZE_ERROR_RATE);
        memcpy(&missed_samples_bits, trailer + PST_OFF_MISSED_SAMPLES,
               PST_SIZE_MISSED_SAMPLES);
        memcpy(&present, trailer + PST_OFF_PRESENT, PST_SIZE_PRESENT);
        error_rate_bits = SWAP64_IF(reader->needs_swap, error_rate_bits);
        missed_samples_bits = SWAP64_IF(reader->needs_swap,
                                       missed_samples_bits);
        present = SWAP32_IF(reader->needs_swap, present);
        memcpy(&reader->error_rate, &error_rate_bits,
               sizeof(error_rate_bits));
        memcpy(&reader->missed_samples, &missed_samples_bits,
               sizeof(missed_samples_bits));
        if (((present & PROFILE_STATS_ERROR_RATE) &&
             (!isfinite(reader->error_rate) || reader->error_rate < 0.0)) ||
            ((present & PROFILE_STATS_MISSED) &&
             !isfinite(reader->missed_samples))) {
            PyErr_SetString(PyExc_ValueError,
                            "Invalid profiling statistics values");
            return -1;
        }
        reader->profile_stats_present = present;
    }
    return 0;
}

static int
reader_decompress(BinaryReader *reader, const uint8_t *compressed_data,
                  size_t compressed_size, ByteBuffer *output,
                  size_t decoded_size)
{
#ifdef HAVE_ZSTD
    /* The data must be exactly one zstd frame */
    size_t frame_size = ZSTD_findFrameCompressedSize(compressed_data,
                                                     compressed_size);
    if (ZSTD_isError(frame_size) || frame_size != compressed_size) {
        PyErr_SetString(PyExc_ValueError, "Corrupted zstd frame header");
        return -1;
    }

    /* Check the decompressed size recorded in the frame header before
     * allocating for it */
    unsigned long long frame_content_size = ZSTD_getFrameContentSize(compressed_data, compressed_size);
    if (frame_content_size == ZSTD_CONTENTSIZE_ERROR) {
        /* Corrupted frame header - fail early */
        PyErr_SetString(PyExc_ValueError, "Corrupted zstd frame header");
        return -1;
    }
    if (frame_content_size != decoded_size) {
        PyErr_SetString(PyExc_ValueError,
                        "zstd frame size does not match the decoded size");
        return -1;
    }

    /* Append the decoded data to the output buffer */
    if (buffer_reserve(output, output->size + decoded_size) < 0) {
        return -1;
    }
    uint8_t *decoded = output->data + output->size;
    size_t result = ZSTD_decompressDCtx(reader->dctx, decoded, decoded_size,
                                        compressed_data, compressed_size);
    if (ZSTD_isError(result)) {
        PyErr_Format(PyExc_ValueError, "zstd decompression error: %s",
                     ZSTD_getErrorName(result));
        return -1;
    }

    /* Verify decompression is complete */
    if (result != decoded_size) {
        PyErr_SetString(PyExc_ValueError, "Incomplete zstd frame: data may be truncated");
        return -1;
    }

    _Py_MSAN_UNPOISON(decoded, decoded_size);
    output->size += decoded_size;
    return 0;
#else
    PyErr_SetString(PyExc_RuntimeError,
        "File uses zstd compression but zstd support not compiled in");
    return -1;
#endif
}

/* Reject a table/run count whose entries cannot fit in the bytes still
 * available; a malicious file could otherwise drive a huge allocation.
 * Each entry occupies at least min_entry_size bytes. */
static int
reader_validate_count(const char *what, uint32_t count,
                      size_t available_bytes, size_t min_entry_size)
{
    size_t max_possible = available_bytes / min_entry_size;
    if (count > max_possible) {
        PyErr_Format(PyExc_ValueError,
            "Invalid %s count %u exceeds maximum possible %zu",
            what, count, max_possible);
        return -1;
    }
    return 0;
}

static inline int
reader_parse_string_table(BinaryReader *reader, const uint8_t *data, size_t file_size)
{
    if (reader_validate_count("string", reader->strings_count,
                              file_size - reader->string_table_offset,
                              MIN_STRING_ENTRY_SIZE) < 0) {
        return -1;
    }

    reader->strings = PyMem_Calloc(reader->strings_count, sizeof(PyObject *));
    if (!reader->strings && reader->strings_count > 0) {
        PyErr_NoMemory();
        return -1;
    }

    size_t offset = reader->string_table_offset;
    for (uint32_t i = 0; i < reader->strings_count; i++) {
        size_t prev_offset = offset;
        uint32_t str_len = decode_varint_u32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in string table");
            return -1;
        }
        if (offset > file_size || str_len > file_size - offset) {
            PyErr_SetString(PyExc_ValueError, "String table overflow");
            return -1;
        }

        reader->strings[i] = PyUnicode_DecodeUTF8((char *)&data[offset], str_len, "replace");
        if (!reader->strings[i]) {
            return -1;
        }
        offset += str_len;
    }

    return 0;
}

static inline int
reader_parse_frame_table(BinaryReader *reader, const uint8_t *data, size_t file_size)
{
    /* Check for integer overflow in allocation size calculation. */
#if SIZEOF_SIZE_T < 8
    if (reader->frames_count > SIZE_MAX / sizeof(FrameEntry)) {
        PyErr_SetString(PyExc_OverflowError, "Frame count too large for allocation");
        return -1;
    }
#endif

    if (reader_validate_count("frame", reader->frames_count,
                              file_size - reader->frame_table_offset,
                              MIN_FRAME_ENTRY_SIZE) < 0) {
        return -1;
    }

    size_t alloc_size = (size_t)reader->frames_count * sizeof(FrameEntry);
    reader->frames = PyMem_Malloc(alloc_size);
    if (!reader->frames && reader->frames_count > 0) {
        PyErr_NoMemory();
        return -1;
    }

    size_t offset = reader->frame_table_offset;
    for (uint32_t i = 0; i < reader->frames_count; i++) {
        FrameEntry *frame = &reader->frames[i];
        size_t prev_offset;

        prev_offset = offset;
        frame->filename_idx = decode_varint_u32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (filename)");
            return -1;
        }

        prev_offset = offset;
        frame->funcname_idx = decode_varint_u32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (funcname)");
            return -1;
        }

        prev_offset = offset;
        frame->lineno = decode_varint_i32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (lineno)");
            return -1;
        }

        prev_offset = offset;
        int32_t end_lineno_delta = decode_varint_i32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (end_lineno_delta)");
            return -1;
        }
        /* Reconstruct end_lineno from delta. If lineno is -1, result is -1. */
        if (frame->lineno == LOCATION_NOT_AVAILABLE) {
            frame->end_lineno = LOCATION_NOT_AVAILABLE;
        } else {
            frame->end_lineno = frame->lineno + end_lineno_delta;
        }

        prev_offset = offset;
        frame->column = decode_varint_i32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (column)");
            return -1;
        }

        prev_offset = offset;
        int32_t end_column_delta = decode_varint_i32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (end_column_delta)");
            return -1;
        }
        /* Reconstruct end_column from delta. If column is -1, result is -1. */
        if (frame->column == LOCATION_NOT_AVAILABLE) {
            frame->end_column = LOCATION_NOT_AVAILABLE;
        } else {
            frame->end_column = frame->column + end_column_delta;
        }

        /* Read opcode byte */
        if (offset >= file_size) {
            PyErr_SetString(PyExc_ValueError, "Unexpected end of frame table (opcode)");
            return -1;
        }
        frame->opcode = data[offset++];
    }

    return 0;
}

BinaryReader *
binary_reader_open(PyObject *path)
{
    BinaryReader *reader = PyMem_Calloc(1, sizeof(BinaryReader));
    if (!reader) {
        PyErr_NoMemory();
        return NULL;
    }

#if USE_MMAP
    /* Open with mmap on Unix */
    FILE *fp = Py_fopen(path, "rb");
    if (!fp) {
        goto error;
    }
    int fd = fileno(fp);

    struct stat st;
    if (fstat(fd, &st) < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        Py_fclose(fp);
        goto error;
    }
    if (st.st_size < 0) {
        PyErr_SetString(PyExc_IOError, "Invalid negative file size");
        Py_fclose(fp);
        goto error;
    }
    if ((uintmax_t)st.st_size > SIZE_MAX) {
        PyErr_SetString(PyExc_OverflowError, "File is too large to map");
        Py_fclose(fp);
        goto error;
    }
    reader->mapped_size = st.st_size;
    if (reader->mapped_size == 0) {
        PyErr_SetString(PyExc_ValueError, "File too small for header");
        Py_fclose(fp);
        goto error;
    }

    /* Map the file into memory.
     * MAP_POPULATE (Linux-only) pre-faults all pages at mmap time, which:
     * - Catches issues (e.g., file truncation) immediately rather than as SIGBUS during reads
     * - Eliminates page faults during subsequent reads for better performance
     */
#ifdef __linux__
    reader->mapped_data = mmap(NULL, reader->mapped_size, PROT_READ,
                               MAP_PRIVATE | MAP_POPULATE, fd, 0);
#else
    reader->mapped_data = mmap(NULL, reader->mapped_size, PROT_READ,
                               MAP_PRIVATE, fd, 0);
#endif
    if (reader->mapped_data == MAP_FAILED) {
        reader->mapped_data = NULL;
        PyErr_SetFromErrno(PyExc_IOError);
        Py_fclose(fp);
        goto error;
    }

    /* Hint sequential access pattern - failures are non-fatal */
    (void)madvise(reader->mapped_data, reader->mapped_size, MADV_SEQUENTIAL);

    /* Pre-fetch pages into memory - failures are non-fatal.
     * Complements MAP_POPULATE on Linux, provides benefit on macOS. */
    (void)madvise(reader->mapped_data, reader->mapped_size, MADV_WILLNEED);

    /* Use transparent huge pages for large files to reduce TLB misses.
     * Only beneficial for files >= 32MB where TLB pressure matters. */
#ifdef MADV_HUGEPAGE
    if (reader->mapped_size >= (32 * 1024 * 1024)) {
        (void)madvise(reader->mapped_data, reader->mapped_size, MADV_HUGEPAGE);
    }
#endif

    /* Add file descriptor-level hints for better kernel I/O scheduling */
#if defined(__linux__) && defined(POSIX_FADV_SEQUENTIAL)
    (void)posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if (reader->mapped_size > (64 * 1024 * 1024)) {
        (void)posix_fadvise(fd, 0, 0, POSIX_FADV_WILLNEED);
    }
#endif

    if (Py_fclose(fp) != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }

    uint8_t *data = reader->mapped_data;
    size_t file_size = reader->mapped_size;
#else
    /* Use stdio on Windows */
    reader->fp = Py_fopen(path, "rb");
    if (!reader->fp) {
        goto error;
    }

    if (FSEEK64(reader->fp, 0, SEEK_END) != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }
    file_offset_t file_size_off = FTELL64(reader->fp);
    if (file_size_off < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }
    if ((uint64_t)file_size_off > SIZE_MAX) {
        PyErr_SetString(PyExc_OverflowError, "File is too large to read");
        goto error;
    }
    reader->file_size = (size_t)file_size_off;
    if (reader->file_size == 0) {
        PyErr_SetString(PyExc_ValueError, "File too small for header");
        goto error;
    }
    if (FSEEK64(reader->fp, 0, SEEK_SET) != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }

    reader->file_data = PyMem_Malloc(reader->file_size);
    if (!reader->file_data) {
        PyErr_NoMemory();
        goto error;
    }

    size_t nread = fread(reader->file_data, 1, reader->file_size, reader->fp);
    if (nread != reader->file_size) {
        int err = errno;
        if (ferror(reader->fp) && err != 0) {
            errno = err;
            PyErr_SetFromErrno(PyExc_IOError);
        }
        else {
            PyErr_Format(PyExc_ValueError,
                "Unexpected end of file: read %zu of %zu bytes",
                nread, reader->file_size);
        }
        goto error;
    }

    uint8_t *data = reader->file_data;
    size_t file_size = reader->file_size;
#endif

    /* Parse header and footer */
    if (reader_parse_header(reader, data, file_size) < 0) {
        goto error;
    }
    if (reader_parse_footer(reader, data, file_size) < 0) {
        goto error;
    }
    if (reader_parse_profile_stats(reader, data, file_size) < 0) {
        goto error;
    }

    /* Validate table offsets are within file bounds */
    if (reader->string_table_offset > file_size) {
        PyErr_Format(PyExc_ValueError,
            "Invalid string table offset: %llu exceeds file size %zu",
            (unsigned long long)reader->string_table_offset, file_size);
        goto error;
    }
    if (reader->frame_table_offset > file_size) {
        PyErr_Format(PyExc_ValueError,
            "Invalid frame table offset: %llu exceeds file size %zu",
            (unsigned long long)reader->frame_table_offset, file_size);
        goto error;
    }
    if (reader->string_table_offset < FILE_HEADER_PLACEHOLDER_SIZE) {
        PyErr_Format(PyExc_ValueError,
            "Invalid string table offset: %llu is before data section",
            (unsigned long long)reader->string_table_offset);
        goto error;
    }
    if (reader->frame_table_offset < FILE_HEADER_PLACEHOLDER_SIZE) {
        PyErr_Format(PyExc_ValueError,
            "Invalid frame table offset: %llu is before data section",
            (unsigned long long)reader->frame_table_offset);
        goto error;
    }
    if (reader->string_table_offset > reader->frame_table_offset) {
        PyErr_Format(PyExc_ValueError,
            "Invalid table offsets: string table (%llu) is after frame table (%llu)",
            (unsigned long long)reader->string_table_offset,
            (unsigned long long)reader->frame_table_offset);
        goto error;
    }

    if (reader->compression_type == COMPRESSION_ZSTD) {
#ifdef HAVE_ZSTD
        reader->dctx = ZSTD_createDCtx();
        if (!reader->dctx) {
            PyErr_SetString(PyExc_MemoryError, "Failed to create zstd decompression context");
            goto error;
        }
#else
        PyErr_SetString(PyExc_RuntimeError,
            "File uses zstd compression but zstd support not compiled in");
        goto error;
#endif
    }
    reader->sample_data = data + FILE_HEADER_PLACEHOLDER_SIZE;
    reader->sample_data_size =
        reader->string_table_offset - FILE_HEADER_PLACEHOLDER_SIZE;

    /* Parse string and frame tables */
    if (reader_parse_string_table(reader, data, file_size) < 0) {
        goto error;
    }
    if (reader_parse_frame_table(reader, data, file_size) < 0) {
        goto error;
    }

    return reader;

error:
    binary_reader_close(reader);
    return NULL;
}

/* Get or create reader thread state for stack reconstruction */
static ReaderThreadState *
reader_get_or_create_thread_state(BinaryReader *reader, uint64_t thread_id,
                                   uint32_t interpreter_id)
{
    /* Search existing threads (key is thread_id + interpreter_id) */
    for (size_t i = 0; i < reader->thread_state_count; i++) {
        if (reader->thread_states[i].thread_id == thread_id &&
            reader->thread_states[i].interpreter_id == interpreter_id) {
            return &reader->thread_states[i];
        }
    }

    if (reader->thread_state_count >= reader->thread_count) {
        PyErr_Format(PyExc_ValueError,
            "Invalid thread count: sample data contains more unique threads than declared in header "
            "(declared %u, found at least %zu)",
            reader->thread_count, reader->thread_state_count + 1);
        return NULL;
    }

    if (!reader->thread_states) {
        reader->thread_state_capacity = 16;
        reader->thread_states = PyMem_Calloc(reader->thread_state_capacity, sizeof(ReaderThreadState));
        if (!reader->thread_states) {
            PyErr_NoMemory();
            return NULL;
        }
    } else if (reader->thread_state_count >= reader->thread_state_capacity) {
        ReaderThreadState *new_states = grow_array(reader->thread_states,
                                                   &reader->thread_state_capacity,
                                                   sizeof(ReaderThreadState));
        if (!new_states) {
            return NULL;
        }
        reader->thread_states = new_states;
    }

    ReaderThreadState *ts = &reader->thread_states[reader->thread_state_count];
    memset(ts, 0, sizeof(ReaderThreadState));
    ts->thread_id = thread_id;
    ts->interpreter_id = interpreter_id;
    reader->thread_state_count++;
    return ts;
}

static PyObject *
reader_get_frame_info(RemoteDebuggingState *state, BinaryReader *reader,
                      uint32_t frame_index)
{
    if (frame_index >= reader->frames_count) {
        PyErr_Format(PyExc_ValueError, "Invalid frame index: %u", frame_index);
        return NULL;
    }
    if (reader->frame_info_cache == NULL) {
        size_t cache_size = Py_MIN(reader->frames_count,
                                   (uint32_t)FRAME_INFO_CACHE_SIZE);
        reader->frame_info_cache = PyMem_Calloc(
            cache_size, sizeof(*reader->frame_info_cache));
        if (reader->frame_info_cache == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
        reader->frame_info_cache_size = cache_size;
    }
    ReaderFrameCacheEntry *entry = &reader->frame_info_cache[
        frame_index % reader->frame_info_cache_size];
    if (entry->frame_info != NULL && entry->frame_index == frame_index) {
        return Py_NewRef(entry->frame_info);
    }

    FrameEntry *frame = &reader->frames[frame_index];
    if (frame->filename_idx >= reader->strings_count ||
        frame->funcname_idx >= reader->strings_count) {
        PyErr_SetString(PyExc_ValueError, "Invalid string index in frame");
        return NULL;
    }

    PyObject *frame_info = PyStructSequence_New(state->FrameInfo_Type);
    if (!frame_info) {
        return NULL;
    }

    /* Build location tuple with full position info */
    PyObject *location;
    if (frame->lineno != LOCATION_NOT_AVAILABLE) {
        location = Py_BuildValue("(iiii)",
            frame->lineno,
            frame->end_lineno,
            frame->column,
            frame->end_column);
        if (!location) {
            Py_DECREF(frame_info);
            return NULL;
        }
    }
    else {
        location = Py_NewRef(Py_None);
    }

    /* Build opcode object */
    PyObject *opcode_obj;
    if (frame->opcode != OPCODE_NONE) {
        opcode_obj = PyLong_FromLong(frame->opcode);
        if (!opcode_obj) {
            Py_DECREF(location);
            Py_DECREF(frame_info);
            return NULL;
        }
    }
    else {
        opcode_obj = Py_NewRef(Py_None);
    }

    PyStructSequence_SetItem(frame_info, 0, Py_NewRef(reader->strings[frame->filename_idx]));
    PyStructSequence_SetItem(frame_info, 1, location);
    PyStructSequence_SetItem(frame_info, 2, Py_NewRef(reader->strings[frame->funcname_idx]));
    PyStructSequence_SetItem(frame_info, 3, opcode_obj);
    Py_XSETREF(entry->frame_info, Py_NewRef(frame_info));
    entry->frame_index = frame_index;
    return frame_info;
}

/* Build a Python list of FrameInfo objects from frame indices */
static PyObject *
build_frame_list(RemoteDebuggingState *state, BinaryReader *reader,
                 const uint32_t *frame_indices, size_t stack_depth)
{
    PyObject *frame_list = PyList_New(stack_depth);
    if (!frame_list) {
        return NULL;
    }

    for (size_t k = 0; k < stack_depth; k++) {
        PyObject *frame_info = reader_get_frame_info(
            state, reader, frame_indices[k]);
        if (frame_info == NULL) {
            goto error;
        }
        PyList_SET_ITEM(frame_list, k, frame_info);
    }

    return frame_list;

error:
    Py_DECREF(frame_list);
    return NULL;
}

/* Helper to build sample_list from frame indices (shared by emit functions) */
static PyObject *
build_sample_list(RemoteDebuggingState *state, BinaryReader *reader,
                  uint64_t thread_id, uint32_t interpreter_id, uint8_t status,
                  const uint32_t *frame_indices, size_t stack_depth)
{
    PyObject *frame_list = NULL, *thread_info = NULL, *thread_list = NULL;
    PyObject *interp_info = NULL, *sample_list = NULL;

    frame_list = build_frame_list(state, reader, frame_indices, stack_depth);
    if (!frame_list) {
        goto error;
    }

    thread_info = PyStructSequence_New(state->ThreadInfo_Type);
    if (!thread_info) {
        goto error;
    }
    PyObject *tid = PyLong_FromUnsignedLongLong(thread_id);
    if (!tid) {
        goto error;
    }
    PyObject *st = PyLong_FromLong(status);
    if (!st) {
        Py_DECREF(tid);
        goto error;
    }
    PyStructSequence_SetItem(thread_info, 0, tid);
    PyStructSequence_SetItem(thread_info, 1, st);
    PyStructSequence_SetItem(thread_info, 2, frame_list);
    frame_list = NULL;  /* ownership transferred */

    thread_list = PyList_New(1);
    if (!thread_list) {
        goto error;
    }
    PyList_SET_ITEM(thread_list, 0, thread_info);
    thread_info = NULL;

    interp_info = PyStructSequence_New(state->InterpreterInfo_Type);
    if (!interp_info) {
        goto error;
    }
    PyObject *iid = PyLong_FromUnsignedLong(interpreter_id);
    if (!iid) {
        goto error;
    }
    PyStructSequence_SetItem(interp_info, 0, iid);
    PyStructSequence_SetItem(interp_info, 1, thread_list);
    thread_list = NULL;

    sample_list = PyList_New(1);
    if (!sample_list) {
        goto error;
    }
    PyList_SET_ITEM(sample_list, 0, interp_info);
    return sample_list;

error:
    Py_XDECREF(sample_list);
    Py_XDECREF(interp_info);
    Py_XDECREF(thread_list);
    Py_XDECREF(thread_info);
    Py_XDECREF(frame_list);
    return NULL;
}

/* Helper to emit a sample to the collector. timestamps_list is borrowed. */
static int
emit_sample(RemoteDebuggingState *state, PyObject *collector,
            uint64_t thread_id, uint32_t interpreter_id, uint8_t status,
            const uint32_t *frame_indices, size_t stack_depth,
            BinaryReader *reader, PyObject *timestamps_list)
{
    PyObject *sample_list = build_sample_list(state, reader, thread_id,
                                               interpreter_id, status,
                                               frame_indices, stack_depth);
    if (!sample_list) {
        return -1;
    }

    PyObject *result = PyObject_CallMethod(collector, "collect", "OO", sample_list, timestamps_list);
    Py_DECREF(sample_list);

    if (!result) {
        return -1;
    }
    Py_DECREF(result);
    return 0;
}

/* Helper to invoke progress callback, returns -1 on error */
static inline int
invoke_progress_callback(PyObject *callback, Py_ssize_t current, uint64_t total)
{
    if (callback && callback != Py_None) {
        PyObject *result = PyObject_CallFunction(callback, "nK", current, (unsigned long long)total);
        if (result) {
            Py_DECREF(result);
        } else {
            return -1;
        }
    }
    return 0;
}

/* ============================================================================
 * SAMPLE BLOCKS
 * ============================================================================ */

static int
block_error(const char *message)
{
    PyErr_Format(PyExc_ValueError, "Invalid sample block: %s", message);
    return -1;
}

static inline int
block_read_varint(const ByteBuffer *column, size_t *offset, uint64_t *value)
{
    size_t prev_offset = *offset;
    *value = decode_varint_u64(column->data, offset, column->size);
    return *offset == prev_offset ? -1 : 0;
}

static int
block_read_varint_u32(const ByteBuffer *column, size_t *offset,
                      uint32_t *value)
{
    size_t prev_offset = *offset;
    *value = decode_varint_u32(column->data, offset, column->size);
    return *offset == prev_offset ? -1 : 0;
}

static int
reader_parse_block_header(BinaryReader *reader, const ByteBuffer *section,
                          size_t *offset, BlockHeader *header)
{
    /* Block magic "TBLK" */
    if (section->size - *offset < BLOCK_MAGIC_SIZE) {
        return block_error("truncated header");
    }
    if (memcmp(section->data + *offset, BLOCK_MAGIC, BLOCK_MAGIC_SIZE) != 0) {
        return block_error("wrong magic");
    }
    *offset += BLOCK_MAGIC_SIZE;

    /* Sample count and base timestamp (varints) */
    if (block_read_varint_u32(section, offset, &header->sample_count) < 0 ||
        block_read_varint(section, offset, &header->base_timestamp) < 0) {
        return -1;
    }

    /* Frame ID width and the two status flag masks (1 byte each) */
    if (section->size - *offset < 3) {
        return block_error("truncated header");
    }
    header->frame_width = section->data[(*offset)++];
    header->common_status_flags = section->data[(*offset)++];
    header->status_bitmap_flags = section->data[(*offset)++];
    if (header->sample_count == 0) {
        return block_error("no samples");
    }
    if (header->frame_width < 1 || header->frame_width > sizeof(uint32_t)) {
        return block_error("frame ID width out of range");
    }
    if (header->common_status_flags & header->status_bitmap_flags) {
        return block_error("common and varying status flags overlap");
    }

    size_t decoded_total = 0;
    for (unsigned column = 0; column < COLUMN_COUNT; column++) {
        ColumnDescriptor *descriptor = &header->columns[column];
        /* Decoded size (varint) and encoding (1 byte) */
        if (block_read_varint_u32(section, offset,
                                  &descriptor->decoded_size) < 0) {
            return -1;
        }
        if (*offset >= section->size) {
            return block_error("truncated column descriptor");
        }
        descriptor->encoding = section->data[(*offset)++];

        /* Stored size: implied for raw and constant, a varint for zstd */
        switch (descriptor->encoding) {
        case COMPRESSION_NONE:
            descriptor->stored_size = descriptor->decoded_size;
            break;
        case COLUMN_CONSTANT:
            descriptor->stored_size = 1;
            break;
        case COMPRESSION_ZSTD:
            if (reader->compression_type != COMPRESSION_ZSTD) {
                return block_error("zstd column in an uncompressed file");
            }
            if (block_read_varint_u32(section, offset,
                                      &descriptor->stored_size) < 0) {
                return -1;
            }
            break;
        default:
            return block_error("unknown column encoding");
        }

        if (descriptor->encoding != COMPRESSION_NONE &&
            descriptor->decoded_size == 0) {
            return block_error("encoded column is empty");
        }
        if (descriptor->decoded_size > MAX_BLOCK_SIZE - decoded_total) {
            return block_error("decoded columns exceed the block limit");
        }
        decoded_total += descriptor->decoded_size;
    }

    /* Each sample needs at least one byte of stream index and residual */
    if (header->sample_count >
            header->columns[COLUMN_SAMPLE_STREAM_IDS].decoded_size ||
        header->sample_count >
            header->columns[COLUMN_TIMESTAMP_RESIDUALS].decoded_size) {
        return block_error("more samples than stored stream indices");
    }
    return 0;
}

static int
decode_block_columns(BinaryReader *reader, const ByteBuffer *section,
                     size_t *offset, DecodedBlock *block)
{
    const ColumnDescriptor *descriptors = block->header.columns;
    /* Constant and zstd columns share one buffer, reused across blocks.
     * It is reserved up front so decoded columns never move. */
    size_t buffered_total = 0;
    for (unsigned column = 0; column < COLUMN_COUNT; column++) {
        if (descriptors[column].encoding != COMPRESSION_NONE) {
            buffered_total += descriptors[column].decoded_size;
        }
    }
    ByteBuffer *buffer = &reader->block_data;
    if (buffer_reserve(buffer, buffered_total) < 0) {
        return -1;
    }

    buffer->size = 0;
    for (unsigned column = 0; column < COLUMN_COUNT; column++) {
        const ColumnDescriptor *descriptor = &descriptors[column];
        if (descriptor->stored_size > section->size - *offset) {
            return block_error("column extends past the sample section");
        }
        uint8_t *stored_data = section->data + *offset;
        ByteBuffer *decoded = &block->columns[column];
        decoded->size = descriptor->decoded_size;
        switch (descriptor->encoding) {
        case COMPRESSION_NONE:
            /* Raw columns point into the file data */
            decoded->data = stored_data;
            break;
        case COLUMN_CONSTANT:
            decoded->data = buffer->data + buffer->size;
            memset(decoded->data, stored_data[0], decoded->size);
            buffer->size += decoded->size;
            break;
        case COMPRESSION_ZSTD:
            decoded->data = buffer->data + buffer->size;
            if (reader_decompress(reader, stored_data,
                                  descriptor->stored_size, buffer,
                                  decoded->size) < 0) {
                return -1;
            }
            break;
        default:
            return block_error("unknown column encoding");
        }
        *offset += descriptor->stored_size;
    }
    return 0;
}

static int
validate_block_columns(DecodedBlock *block)
{
    size_t stream_table_size = block->columns[COLUMN_STREAM_TABLE].size;
    if (stream_table_size % STREAM_ENTRY_SIZE != 0) {
        return block_error("stream table has a partial entry");
    }
    size_t stream_count = stream_table_size / STREAM_ENTRY_SIZE;
    if (stream_count == 0 || stream_count > MAX_BLOCK_STREAMS) {
        return block_error("stream count out of range");
    }
    block->stream_count = (uint32_t)stream_count;

    size_t frame_ids_size = block->columns[COLUMN_FRAME_IDS].size;
    if (frame_ids_size % block->header.frame_width != 0) {
        return block_error("frame ID column has a partial value");
    }
    block->frame_count =
        (uint32_t)(frame_ids_size / block->header.frame_width);

    size_t plane_size = status_bitmap_size(block->header.sample_count);
    size_t plane_count =
        (size_t)_Py_popcount32(block->header.status_bitmap_flags);
    const ByteBuffer *statuses = &block->columns[COLUMN_STATUS_BITMAPS];
    if (statuses->size != plane_size * plane_count) {
        return block_error("status bitmaps have the wrong size");
    }
    unsigned used_bits = block->header.sample_count % CHAR_BIT;
    for (size_t plane = 0; used_bits && plane < plane_count; plane++) {
        if (statuses->data[(plane + 1) * plane_size - 1] >> used_bits) {
            return block_error("status bitmap padding is not zero");
        }
    }
    return 0;
}

static int
reserve_block_streams(BinaryReader *reader, uint32_t stream_count)
{
    if (stream_count <= reader->block_stream_capacity) {
        return 0;
    }
    ReaderBlockStream *streams = PyMem_Realloc(
        reader->block_streams, stream_count * sizeof(ReaderBlockStream));
    if (streams == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    reader->block_streams = streams;
    uint32_t *stacks = PyMem_Realloc(
        reader->block_stacks,
        stream_count * MAX_STACK_DEPTH * sizeof(uint32_t));
    if (stacks == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    reader->block_stacks = stacks;
    reader->block_stream_capacity = stream_count;
    return 0;
}

static int
reader_parse_stream_table(BinaryReader *reader, const DecodedBlock *block)
{
    const uint8_t *stream_table = block->columns[COLUMN_STREAM_TABLE].data;
    uint64_t initial_prev_timestamp =
        block->header.base_timestamp - reader->sample_interval_us;
    memset(reader->block_streams, 0,
           block->stream_count * sizeof(ReaderBlockStream));
    for (uint32_t i = 0; i < block->stream_count; i++) {
        /* Read thread_id (8 bytes) + interpreter_id (4 bytes) */
        const uint8_t *entry = stream_table + (size_t)i * STREAM_ENTRY_SIZE;
        uint64_t thread_id;
        uint32_t interpreter_id;
        memcpy(&thread_id, entry + STM_OFF_THREAD_ID, STM_SIZE_THREAD_ID);
        memcpy(&interpreter_id, entry + STM_OFF_INTERPRETER_ID,
               STM_SIZE_INTERPRETER_ID);
        ReaderThreadState *thread = reader_get_or_create_thread_state(
            reader, SWAP64_IF(reader->needs_swap, thread_id),
            SWAP32_IF(reader->needs_swap, interpreter_id));
        if (thread == NULL) {
            return -1;
        }
        /* Each thread may appear only once in a block. An index left over
         * from an earlier block points at a different thread here. */
        uint32_t previous_index = thread->block_stream_index;
        if (previous_index < i &&
            reader->block_streams[previous_index].thread_id ==
                thread->thread_id &&
            reader->block_streams[previous_index].interpreter_id ==
                thread->interpreter_id) {
            return block_error("stream table repeats a thread");
        }
        thread->block_stream_index = i;

        /* Each stream's first expected timestamp is the base timestamp */
        ReaderBlockStream *stream = &reader->block_streams[i];
        stream->thread_id = thread->thread_id;
        stream->interpreter_id = thread->interpreter_id;
        stream->prev_timestamp = initial_prev_timestamp;
        stream->current_stack =
            reader->block_stacks + (size_t)i * MAX_STACK_DEPTH;
    }
    return 0;
}

static int
decode_samples(BinaryReader *reader, DecodedBlock *block)
{
    size_t sample_count = block->header.sample_count;
    if (buffer_reserve(&reader->block_stream_ids,
                       sample_count * sizeof(uint32_t)) < 0 ||
        buffer_reserve(&reader->block_timestamps,
                       sample_count * sizeof(uint64_t)) < 0) {
        return -1;
    }
    uint32_t *stream_ids = (uint32_t *)reader->block_stream_ids.data;
    uint64_t *timestamps = (uint64_t *)reader->block_timestamps.data;
    const ByteBuffer *stream_id_column =
        &block->columns[COLUMN_SAMPLE_STREAM_IDS];
    const ByteBuffer *residual_column =
        &block->columns[COLUMN_TIMESTAMP_RESIDUALS];
    size_t stream_id_offset = 0;
    size_t residual_offset = 0;
    for (uint32_t sample = 0; sample < block->header.sample_count; sample++) {
        uint64_t stream_id;
        if (block_read_varint(stream_id_column, &stream_id_offset,
                              &stream_id) < 0) {
            return -1;
        }
        if (stream_id >= block->stream_count) {
            return block_error("sample refers to an unknown stream");
        }
        size_t prev_offset = residual_offset;
        int64_t residual = decode_varint_i64(residual_column->data,
                                             &residual_offset,
                                             residual_column->size);
        if (residual_offset == prev_offset) {
            return -1;
        }

        /* Timestamp is the expected time plus the stored residual */
        ReaderBlockStream *stream = &reader->block_streams[stream_id];
        stream->prev_timestamp +=
            reader->sample_interval_us + (uint64_t)residual;

        stream_ids[sample] = (uint32_t)stream_id;
        timestamps[sample] = stream->prev_timestamp;
    }
    if (stream_id_offset != stream_id_column->size) {
        return block_error("stream index column has trailing data");
    }
    if (residual_offset != residual_column->size) {
        return block_error("timestamp column has trailing data");
    }
    block->stream_ids = stream_ids;
    block->timestamps = timestamps;
    return 0;
}

static int
decode_statuses(BinaryReader *reader, DecodedBlock *block)
{
    if (buffer_reserve(&reader->block_statuses,
                       block->header.sample_count) < 0) {
        return -1;
    }
    /* Start from the common flags, then add each varying flag's bitmap */
    uint8_t *statuses = reader->block_statuses.data;
    memset(statuses, block->header.common_status_flags,
           block->header.sample_count);
    const uint8_t *plane = block->columns[COLUMN_STATUS_BITMAPS].data;
    size_t plane_size = status_bitmap_size(block->header.sample_count);
    for (unsigned bit = 0; bit < CHAR_BIT; bit++) {
        if (!(block->header.status_bitmap_flags & (1U << bit))) {
            continue;
        }
        for (uint32_t i = 0; i < block->header.sample_count; i++) {
            statuses[i] |= ((plane[i / CHAR_BIT] >> (i % CHAR_BIT)) & 1U)
                           << bit;
        }
        plane += plane_size;
    }
    block->statuses = statuses;
    return 0;
}

static int
decode_frame_ids(BinaryReader *reader, DecodedBlock *block)
{
    size_t frame_count = block->frame_count;
    size_t frames_size = frame_count * sizeof(uint32_t);
    if (buffer_reserve(&reader->block_frames, frames_size) < 0) {
        return -1;
    }
    uint32_t *frames = (uint32_t *)reader->block_frames.data;
    block->frames = frames;
    if (frame_count == 0) {
        return 0;
    }

    /* Combine byte b of every frame index from plane b, low byte first */
    memset(frames, 0, frames_size);
    const uint8_t *plane = block->columns[COLUMN_FRAME_IDS].data;
    for (unsigned byte = 0; byte < block->header.frame_width; byte++) {
        for (size_t i = 0; i < frame_count; i++) {
            frames[i] |= (uint32_t)plane[i] << (byte * CHAR_BIT);
        }
        plane += frame_count;
    }
    for (size_t i = 0; i < frame_count; i++) {
        if (frames[i] >= reader->frames_count) {
            return block_error("frame index out of range");
        }
    }
    return 0;
}

static int
decode_block(BinaryReader *reader, const ByteBuffer *section,
             size_t *offset, DecodedBlock *block)
{
    block->instruction_offset = 0;
    block->frame_offset = 0;
    if (reader_parse_block_header(reader, section, offset,
                                  &block->header) < 0 ||
        decode_block_columns(reader, section, offset, block) < 0 ||
        validate_block_columns(block) < 0 ||
        reserve_block_streams(reader, block->stream_count) < 0 ||
        reader_parse_stream_table(reader, block) < 0 ||
        decode_samples(reader, block) < 0 ||
        decode_statuses(reader, block) < 0 ||
        decode_frame_ids(reader, block) < 0) {
        return -1;
    }
    return 0;
}

static int
flush_stream_batch(BinaryReader *reader, RemoteDebuggingState *state,
                   PyObject *collector, ReaderBlockStream *stream)
{
    if (stream->batch == NULL) {
        return 0;
    }
    /* Stacks are kept outermost first; collectors expect innermost first */
    uint32_t stack[MAX_STACK_DEPTH];
    for (size_t i = 0; i < stream->current_stack_depth; i++) {
        stack[i] = stream->current_stack[stream->current_stack_depth - 1 - i];
    }
    int result = emit_sample(state, collector, stream->thread_id,
                             stream->interpreter_id, stream->batch_status,
                             stack, stream->current_stack_depth, reader,
                             stream->batch);
    Py_CLEAR(stream->batch);
    return result;
}

static int
decode_stack_repeat(BinaryReader *reader, DecodedBlock *block,
                    ReaderBlockStream *stream)
{
    const ByteBuffer *instructions =
        &block->columns[COLUMN_STACK_INSTRUCTIONS];
    size_t *offset = &block->instruction_offset;
    uint64_t count;
    if (block_read_varint(instructions, offset, &count) < 0) {
        return -1;
    }
    if (!stream->has_stack) {
        return block_error("first stack of a stream cannot repeat");
    }
    if (count == 0 || count > block->header.sample_count) {
        return block_error("repeat count out of range");
    }
    stream->pending_repeats = (uint32_t)(count - 1);
    reader->stats.repeat_records++;
    reader->stats.repeat_samples++;
    return 0;
}

static int
decode_stack_replace_suffix(BinaryReader *reader, DecodedBlock *block,
                            ReaderBlockStream *stream)
{
    const ByteBuffer *instructions =
        &block->columns[COLUMN_STACK_INSTRUCTIONS];
    size_t *offset = &block->instruction_offset;
    uint64_t keep, count;
    if (block_read_varint(instructions, offset, &keep) < 0 ||
        block_read_varint(instructions, offset, &count) < 0) {
        return -1;
    }
    if (keep > stream->current_stack_depth || count > MAX_STACK_DEPTH - keep) {
        return block_error("stack prefix out of range");
    }
    if (count > block->frame_count - block->frame_offset) {
        return block_error("stack uses more frame IDs than stored");
    }
    if (count > 0) {
        memcpy(&stream->current_stack[keep],
               &block->frames[block->frame_offset], count * sizeof(uint32_t));
    }
    block->frame_offset += (uint32_t)count;
    stream->current_stack_depth = (size_t)(keep + count);
    if (keep == 0) {
        reader->stats.full_records++;
    }
    else {
        reader->stats.suffix_records++;
    }
    return 0;
}

static int
decode_stack_reuse(BinaryReader *reader, DecodedBlock *block,
                   ReaderBlockStream *stream)
{
    const ByteBuffer *instructions =
        &block->columns[COLUMN_STACK_INSTRUCTIONS];
    size_t *offset = &block->instruction_offset;
    uint64_t start, count;
    if (block_read_varint(instructions, offset, &start) < 0 ||
        block_read_varint(instructions, offset, &count) < 0) {
        return -1;
    }
    if (count > MAX_STACK_DEPTH) {
        return block_error("stack is too deep");
    }
    if (start > block->frame_offset || count > block->frame_offset - start) {
        return block_error("reused stack is not stored earlier");
    }
    if (count > 0) {
        memcpy(stream->current_stack, &block->frames[start],
               count * sizeof(uint32_t));
    }
    stream->current_stack_depth = (size_t)count;
    reader->stats.reuse_records++;
    return 0;
}

static int
decode_stack(BinaryReader *reader, RemoteDebuggingState *state,
             PyObject *collector, DecodedBlock *block,
             ReaderBlockStream *stream)
{
    /* An earlier REPEAT still covers this sample */
    if (stream->pending_repeats > 0) {
        stream->pending_repeats--;
        reader->stats.repeat_samples++;
        return 0;
    }
    uint64_t encoding;
    if (block_read_varint(&block->columns[COLUMN_STACK_INSTRUCTIONS],
                          &block->instruction_offset, &encoding) < 0) {
        return -1;
    }
    if (encoding == STACK_REPEAT) {
        return decode_stack_repeat(reader, block, stream);
    }

    /* The pending batch was collected with the stack being replaced. */
    if (flush_stream_batch(reader, state, collector, stream) < 0) {
        return -1;
    }
    int result;
    switch (encoding) {
    case STACK_REPLACE_SUFFIX:
        result = decode_stack_replace_suffix(reader, block, stream);
        break;
    case STACK_REUSE:
        result = decode_stack_reuse(reader, block, stream);
        break;
    default:
        return block_error("unknown stack instruction");
    }
    if (result < 0) {
        return -1;
    }
    stream->has_stack = 1;
    reader->stats.stack_reconstructions++;
    return 0;
}

static int
replay_sample(BinaryReader *reader, RemoteDebuggingState *state,
              PyObject *collector, DecodedBlock *block, uint32_t sample)
{
    ReaderBlockStream *stream =
        &reader->block_streams[block->stream_ids[sample]];
    uint8_t status = block->statuses[sample];
    if (decode_stack(reader, state, collector, block, stream) < 0) {
        return -1;
    }

    /* Start a new batch when the status changes or the batch is full */
    if (stream->batch == NULL || status != stream->batch_status ||
        PyList_GET_SIZE(stream->batch) >= MAX_RLE_BATCH_SAMPLES) {
        if (flush_stream_batch(reader, state, collector, stream) < 0) {
            return -1;
        }
        stream->batch = PyList_New(0);
        if (stream->batch == NULL) {
            return -1;
        }
        stream->batch_status = status;
    }
    PyObject *timestamp =
        PyLong_FromUnsignedLongLong(block->timestamps[sample]);
    if (timestamp == NULL) {
        return -1;
    }
    int result = PyList_Append(stream->batch, timestamp);
    Py_DECREF(timestamp);
    return result;
}

static int
validate_block_consumed(BinaryReader *reader, const DecodedBlock *block)
{
    if (block->instruction_offset !=
            block->columns[COLUMN_STACK_INSTRUCTIONS].size) {
        return block_error("stack instruction column has trailing data");
    }
    if (block->frame_offset != block->frame_count) {
        return block_error("frame ID column has unused values");
    }
    for (uint32_t i = 0; i < block->stream_count; i++) {
        if (!reader->block_streams[i].has_stack) {
            return block_error("stream table has an unused entry");
        }
        if (reader->block_streams[i].pending_repeats != 0) {
            return block_error("repeat count extends past the block");
        }
    }
    return 0;
}

static int
replay_block(BinaryReader *reader, RemoteDebuggingState *state,
             PyObject *collector, PyObject *progress_callback,
             DecodedBlock *block, Py_ssize_t *replayed)
{
    if ((uint64_t)block->header.sample_count >
            reader->sample_count - (uint64_t)*replayed) {
        return block_error("more samples than the file header declares");
    }
    int result = 0;
    for (uint32_t sample = 0; sample < block->header.sample_count; sample++) {
        if (replay_sample(reader, state, collector, block, sample) < 0) {
            result = -1;
            break;
        }
        (*replayed)++;
        reader->stats.total_samples++;
        if (*replayed % PROGRESS_CALLBACK_INTERVAL == 0 &&
            invoke_progress_callback(progress_callback, *replayed,
                                     reader->sample_count) < 0) {
            result = -1;
            break;
        }
    }
    if (result == 0) {
        result = validate_block_consumed(reader, block);
    }

    /* Send the remaining batches, or drop them after an error. */
    for (uint32_t i = 0; i < block->stream_count; i++) {
        ReaderBlockStream *stream = &reader->block_streams[i];
        if (result == 0 &&
            flush_stream_batch(reader, state, collector, stream) < 0) {
            result = -1;
        }
        Py_CLEAR(stream->batch);
    }

    return result;
}

static int
replay_blocks(BinaryReader *reader, RemoteDebuggingState *state,
              PyObject *collector, PyObject *progress_callback,
              Py_ssize_t *replayed)
{
    ByteBuffer section = {reader->sample_data, reader->sample_data_size, 0};
    size_t offset = 0;
    while (offset < section.size) {
        DecodedBlock block;
        if (decode_block(reader, &section, &offset, &block) < 0 ||
            replay_block(reader, state, collector, progress_callback,
                         &block, replayed) < 0) {
            return -1;
        }
    }
    return 0;
}

Py_ssize_t
binary_reader_replay(BinaryReader *reader, PyObject *collector, PyObject *progress_callback)
{
    if (reader->replaying) {
        PyErr_SetString(PyExc_RuntimeError, "Reader is already replaying");
        return -1;
    }
    PyObject *collect_method;
    int has_collect = PyObject_GetOptionalAttrString(collector, "collect", &collect_method);
    if (has_collect < 0) {
        return -1;
    }
    if (has_collect == 0) {
        PyErr_SetString(PyExc_TypeError, "Collector must have a collect() method");
        return -1;
    }
    Py_DECREF(collect_method);

    /* Get module state for struct sequence types */
    PyObject *module = PyImport_ImportModule("_remote_debugging");
    if (!module) {
        return -1;
    }
    RemoteDebuggingState *state = RemoteDebugging_GetState(module);
    Py_DECREF(module);

    if (!state) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to get module state");
        return -1;
    }
    if (reader->sample_count > (uint64_t)PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "Sample count exceeds Py_ssize_t maximum");
        return -1;
    }

    /* Initial progress callback at 0% */
    if (invoke_progress_callback(progress_callback, 0, reader->sample_count) < 0) {
        return -1;
    }

    memset(&reader->stats, 0, sizeof(reader->stats));
    /* The collector and progress callback must not replay this reader again
     * while its block state is in use */
    Py_ssize_t replayed = 0;
    reader->replaying = 1;
    int result = replay_blocks(reader, state, collector, progress_callback,
                               &replayed);
    reader->replaying = 0;
    if (result < 0) {
        return -1;
    }

    if ((uint64_t)replayed != reader->sample_count) {
        PyErr_Format(PyExc_ValueError,
            "Sample count mismatch: header declares %llu samples but replay decoded %zd",
            (unsigned long long)reader->sample_count, replayed);
        return -1;
    }

    /* Final progress callback at 100% */
    if (invoke_progress_callback(progress_callback, replayed, reader->sample_count) < 0) {
        return -1;
    }

    return replayed;
}

PyObject *
binary_reader_get_info(BinaryReader *reader)
{
    PyObject *py_version = Py_BuildValue("(B,B,B)",
        reader->py_major, reader->py_minor, reader->py_micro);
    if (py_version == NULL) {
        return NULL;
    }
    PyObject *duration = reader->has_profile_stats
        ? PyFloat_FromDouble(reader->duration_sec) : Py_NewRef(Py_None);
    if (duration == NULL) {
        Py_DECREF(py_version);
        return NULL;
    }
    PyObject *sample_rate = reader->has_profile_stats
        ? PyFloat_FromDouble(reader->sample_rate) : Py_NewRef(Py_None);
    if (sample_rate == NULL) {
        Py_DECREF(py_version);
        Py_DECREF(duration);
        return NULL;
    }
    PyObject *error_rate =
        reader->profile_stats_present & PROFILE_STATS_ERROR_RATE
        ? PyFloat_FromDouble(reader->error_rate) : Py_NewRef(Py_None);
    if (error_rate == NULL) {
        Py_DECREF(py_version);
        Py_DECREF(duration);
        Py_DECREF(sample_rate);
        return NULL;
    }
    PyObject *missed_samples =
        reader->profile_stats_present & PROFILE_STATS_MISSED
        ? PyFloat_FromDouble(reader->missed_samples) : Py_NewRef(Py_None);
    if (missed_samples == NULL) {
        Py_DECREF(py_version);
        Py_DECREF(duration);
        Py_DECREF(sample_rate);
        Py_DECREF(error_rate);
        return NULL;
    }
    PyObject *profiling_mode = reader->profiling_mode < 0
        ? Py_NewRef(Py_None)
        : PyLong_FromLong(reader->profiling_mode);
    if (profiling_mode == NULL) {
        Py_DECREF(py_version);
        Py_DECREF(duration);
        Py_DECREF(sample_rate);
        Py_DECREF(error_rate);
        Py_DECREF(missed_samples);
        return NULL;
    }
    PyObject *capture_features = reader->capture_features < 0
        ? Py_NewRef(Py_None)
        : PyLong_FromLong(reader->capture_features);
    if (capture_features == NULL) {
        Py_DECREF(py_version);
        Py_DECREF(profiling_mode);
        Py_DECREF(duration);
        Py_DECREF(sample_rate);
        Py_DECREF(error_rate);
        Py_DECREF(missed_samples);
        return NULL;
    }
    return Py_BuildValue(
        "{s:I, s:N, s:K, s:K, s:K, s:I, s:I, s:I, s:i, s:N, s:N, s:N, s:N, s:N, s:N}",
        "version", BINARY_FORMAT_VERSION,
        "python_version", py_version,
        "start_time_us", reader->start_time_us,
        "sample_interval_us", reader->sample_interval_us,
        "sample_count", reader->sample_count,
        "thread_count", reader->thread_count,
        "string_count", reader->strings_count,
        "frame_count", reader->frames_count,
        "compression_type", reader->compression_type,
        "duration_sec", duration,
        "sample_rate", sample_rate,
        "error_rate", error_rate,
        "missed_samples", missed_samples,
        "mode", profiling_mode,
        "capture_features", capture_features
    );
}

PyObject *
binary_writer_get_stats(BinaryWriter *writer)
{
    BinaryWriterStats *s = &writer->stats;

    /* Calculate derived stats */
    uint64_t total_records = s->repeat_records + s->full_records +
                             s->suffix_records + s->reuse_records;
    uint64_t total_samples = writer->total_samples;
    uint64_t bytes_written = s->bytes_written + binary_writer_buffered_size(writer);
    uint64_t potential_frames = s->total_frames_written + s->frames_saved;
    double compression_ratio = (potential_frames > 0) ?
        (double)s->frames_saved / potential_frames * 100.0 : 0.0;

    return Py_BuildValue(
        "{s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:d}",
        "repeat_records", s->repeat_records,
        "repeat_samples", s->repeat_samples,
        "full_records", s->full_records,
        "suffix_records", s->suffix_records,
        "reuse_records", s->reuse_records,
        "total_records", total_records,
        "total_samples", total_samples,
        "total_frames_written", s->total_frames_written,
        "frames_saved", s->frames_saved,
        "bytes_written", bytes_written,
        "frame_compression_pct", compression_ratio
    );
}

PyObject *
binary_reader_get_stats(BinaryReader *reader)
{
    BinaryReaderStats *s = &reader->stats;

    uint64_t total_records = s->repeat_records + s->full_records +
                             s->suffix_records + s->reuse_records;

    return Py_BuildValue(
        "{s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K}",
        "repeat_records", s->repeat_records,
        "repeat_samples", s->repeat_samples,
        "full_records", s->full_records,
        "suffix_records", s->suffix_records,
        "reuse_records", s->reuse_records,
        "total_records", total_records,
        "total_samples", s->total_samples,
        "stack_reconstructions", s->stack_reconstructions
    );
}

void
binary_reader_close(BinaryReader *reader)
{
    if (!reader) {
        return;
    }

#if USE_MMAP
    if (reader->mapped_data) {
        munmap(reader->mapped_data, reader->mapped_size);
        reader->mapped_data = NULL;  /* Prevent use-after-free */
        reader->mapped_size = 0;
    }
    /* Clear sample_data which may point into the now-unmapped region */
    reader->sample_data = NULL;
    reader->sample_data_size = 0;
#else
    if (reader->fp) {
        Py_fclose(reader->fp);
        reader->fp = NULL;
    }
    if (reader->file_data) {
        PyMem_Free(reader->file_data);
        reader->file_data = NULL;
        reader->file_size = 0;
    }
#endif

#ifdef HAVE_ZSTD
    ZSTD_freeDCtx(reader->dctx);
#endif
    buffer_clear(&reader->block_data);
    buffer_clear(&reader->block_stream_ids);
    buffer_clear(&reader->block_timestamps);
    buffer_clear(&reader->block_statuses);
    buffer_clear(&reader->block_frames);
    PyMem_Free(reader->block_streams);
    PyMem_Free(reader->block_stacks);

    if (reader->frame_info_cache != NULL) {
        for (size_t i = 0; i < reader->frame_info_cache_size; i++) {
            Py_XDECREF(reader->frame_info_cache[i].frame_info);
        }
        PyMem_Free(reader->frame_info_cache);
    }

    if (reader->strings) {
        for (uint32_t i = 0; i < reader->strings_count; i++) {
            Py_XDECREF(reader->strings[i]);
        }
        PyMem_Free(reader->strings);
    }

    PyMem_Free(reader->frames);

    PyMem_Free(reader->thread_states);

    PyMem_Free(reader);
}
