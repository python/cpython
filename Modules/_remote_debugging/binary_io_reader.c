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
#include <string.h>

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

/* ============================================================================
 * CONSTANTS FOR BINARY FORMAT SIZES
 * ============================================================================ */

/* File structure sizes */
#define FILE_FOOTER_SIZE 32
#define MIN_DECOMPRESS_BUFFER_SIZE (64 * 1024)  /* Minimum decompression buffer */

/* Progress callback frequency */
#define PROGRESS_CALLBACK_INTERVAL 1000

/* Maximum decompression size limit (1GB) */
#define MAX_DECOMPRESS_SIZE (1ULL << 30)

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
    memcpy(&magic, &data[0], sizeof(magic));
    memcpy(&version, &data[4], sizeof(version));

    if (magic != BINARY_FORMAT_MAGIC) {
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
    memcpy(&reader->start_time_us, &data[HDR_OFF_START_TIME], HDR_SIZE_START_TIME);
    memcpy(&reader->sample_interval_us, &data[HDR_OFF_INTERVAL], HDR_SIZE_INTERVAL);
    memcpy(&reader->sample_count, &data[HDR_OFF_SAMPLES], HDR_SIZE_SAMPLES);
    memcpy(&reader->thread_count, &data[HDR_OFF_THREADS], HDR_SIZE_THREADS);
    memcpy(&reader->string_table_offset, &data[HDR_OFF_STR_TABLE], HDR_SIZE_STR_TABLE);
    memcpy(&reader->frame_table_offset, &data[HDR_OFF_FRAME_TABLE], HDR_SIZE_FRAME_TABLE);
    memcpy(&reader->compression_type, &data[HDR_OFF_COMPRESSION], HDR_SIZE_COMPRESSION);

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
    memcpy(&reader->strings_count, &footer[0], sizeof(reader->strings_count));
    memcpy(&reader->frames_count, &footer[4], sizeof(reader->frames_count));

    return 0;
}

#ifdef HAVE_ZSTD
/* Maximum decompression buffer size to prevent memory exhaustion (1GB) */
#define MAX_DECOMPRESS_SIZE (1ULL << 30)

static inline int
reader_decompress_samples(BinaryReader *reader, const uint8_t *data)
{
    size_t compressed_size = reader->string_table_offset - FILE_HEADER_PLACEHOLDER_SIZE;
    const uint8_t *compressed_data = data + FILE_HEADER_PLACEHOLDER_SIZE;

    /* Validate compressed data region */
    if (reader->string_table_offset < FILE_HEADER_PLACEHOLDER_SIZE) {
        PyErr_SetString(PyExc_ValueError, "Invalid string table offset");
        return -1;
    }

    ZSTD_DCtx *dctx = ZSTD_createDCtx();
    if (!dctx) {
        PyErr_SetString(PyExc_MemoryError, "Failed to create zstd decompression context");
        return -1;
    }

    /* Try to get exact decompressed size from frame header for optimal allocation */
    unsigned long long frame_content_size = ZSTD_getFrameContentSize(compressed_data, compressed_size);
    size_t alloc_size;

    if (frame_content_size == ZSTD_CONTENTSIZE_ERROR) {
        /* Corrupted frame header - fail early */
        ZSTD_freeDCtx(dctx);
        PyErr_SetString(PyExc_ValueError, "Corrupted zstd frame header");
        return -1;
    } else if (frame_content_size != ZSTD_CONTENTSIZE_UNKNOWN &&
               frame_content_size <= SIZE_MAX &&
               frame_content_size <= MAX_DECOMPRESS_SIZE) {
        alloc_size = (size_t)frame_content_size;
    } else {
        alloc_size = ZSTD_DStreamOutSize() * 4;
        if (alloc_size < MIN_DECOMPRESS_BUFFER_SIZE) {
            alloc_size = MIN_DECOMPRESS_BUFFER_SIZE;
        }
    }

    reader->decompressed_data = PyMem_Malloc(alloc_size);
    if (!reader->decompressed_data) {
        ZSTD_freeDCtx(dctx);
        PyErr_NoMemory();
        return -1;
    }

    ZSTD_inBuffer input = { compressed_data, compressed_size, 0 };
    size_t total_output = 0;
    size_t last_result = 0;

    while (input.pos < input.size) {
        if (total_output >= alloc_size) {
            /* Check for overflow before doubling */
            if (alloc_size > SIZE_MAX / 2 || alloc_size * 2 > MAX_DECOMPRESS_SIZE) {
                PyMem_Free(reader->decompressed_data);
                reader->decompressed_data = NULL;
                ZSTD_freeDCtx(dctx);
                PyErr_SetString(PyExc_MemoryError, "Decompressed data exceeds maximum size");
                return -1;
            }
            size_t new_size = alloc_size * 2;
            uint8_t *new_buf = PyMem_Realloc(reader->decompressed_data, new_size);
            if (!new_buf) {
                PyMem_Free(reader->decompressed_data);
                reader->decompressed_data = NULL;
                ZSTD_freeDCtx(dctx);
                PyErr_NoMemory();
                return -1;
            }
            reader->decompressed_data = new_buf;
            alloc_size = new_size;
        }

        ZSTD_outBuffer output = {
            reader->decompressed_data + total_output,
            alloc_size - total_output,
            0
        };

        last_result = ZSTD_decompressStream(dctx, &output, &input);
        if (ZSTD_isError(last_result)) {
            PyMem_Free(reader->decompressed_data);
            reader->decompressed_data = NULL;
            ZSTD_freeDCtx(dctx);
            PyErr_Format(PyExc_ValueError, "zstd decompression error: %s",
                         ZSTD_getErrorName(last_result));
            return -1;
        }

        total_output += output.pos;
    }

    /* Verify decompression is complete (last_result == 0 means frame is complete) */
    if (last_result != 0) {
        PyMem_Free(reader->decompressed_data);
        reader->decompressed_data = NULL;
        ZSTD_freeDCtx(dctx);
        PyErr_SetString(PyExc_ValueError, "Incomplete zstd frame: data may be truncated");
        return -1;
    }

    ZSTD_freeDCtx(dctx);
    reader->decompressed_size = total_output;
    reader->sample_data = reader->decompressed_data;
    reader->sample_data_size = reader->decompressed_size;

    return 0;
}
#endif

static inline int
reader_parse_string_table(BinaryReader *reader, const uint8_t *data, size_t file_size)
{
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
        if (offset + str_len > file_size) {
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
    /* Check for integer overflow in allocation size calculation.
       Only needed on 32-bit where SIZE_MAX can be exceeded by uint32_t * 12. */
#if SIZEOF_SIZE_T < 8
    if (reader->frames_count > SIZE_MAX / (3 * sizeof(uint32_t))) {
        PyErr_SetString(PyExc_OverflowError, "Frame count too large for allocation");
        return -1;
    }
#endif

    size_t alloc_size = (size_t)reader->frames_count * 3 * sizeof(uint32_t);
    reader->frame_data = PyMem_Malloc(alloc_size);
    if (!reader->frame_data && reader->frames_count > 0) {
        PyErr_NoMemory();
        return -1;
    }

    size_t offset = reader->frame_table_offset;
    for (uint32_t i = 0; i < reader->frames_count; i++) {
        size_t base = (size_t)i * 3;
        size_t prev_offset;

        prev_offset = offset;
        reader->frame_data[base] = decode_varint_u32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (filename)");
            return -1;
        }

        prev_offset = offset;
        reader->frame_data[base + 1] = decode_varint_u32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (funcname)");
            return -1;
        }

        prev_offset = offset;
        reader->frame_data[base + 2] = (uint32_t)decode_varint_i32(data, &offset, file_size);
        if (offset == prev_offset) {
            PyErr_SetString(PyExc_ValueError, "Malformed varint in frame table (lineno)");
            return -1;
        }
    }

    return 0;
}

BinaryReader *
binary_reader_open(const char *filename)
{
    BinaryReader *reader = PyMem_Calloc(1, sizeof(BinaryReader));
    if (!reader) {
        PyErr_NoMemory();
        return NULL;
    }

#if USE_MMAP
    reader->fd = -1;  /* Explicit initialization for cleanup safety */
#endif

    reader->filename = PyMem_Malloc(strlen(filename) + 1);
    if (!reader->filename) {
        PyMem_Free(reader);
        PyErr_NoMemory();
        return NULL;
    }
    strcpy(reader->filename, filename);

#if USE_MMAP
    /* Open with mmap on Unix */
    reader->fd = open(filename, O_RDONLY);
    if (reader->fd < 0) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
        goto error;
    }

    struct stat st;
    if (fstat(reader->fd, &st) < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }
    reader->mapped_size = st.st_size;

    /* Map the file into memory.
     * MAP_POPULATE (Linux-only) pre-faults all pages at mmap time, which:
     * - Catches issues (e.g., file truncation) immediately rather than as SIGBUS during reads
     * - Eliminates page faults during subsequent reads for better performance
     */
#ifdef __linux__
    reader->mapped_data = mmap(NULL, reader->mapped_size, PROT_READ,
                               MAP_PRIVATE | MAP_POPULATE, reader->fd, 0);
#else
    reader->mapped_data = mmap(NULL, reader->mapped_size, PROT_READ,
                               MAP_PRIVATE, reader->fd, 0);
#endif
    if (reader->mapped_data == MAP_FAILED) {
        reader->mapped_data = NULL;
        PyErr_SetFromErrno(PyExc_IOError);
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
    (void)posix_fadvise(reader->fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if (reader->mapped_size > (64 * 1024 * 1024)) {
        (void)posix_fadvise(reader->fd, 0, 0, POSIX_FADV_WILLNEED);
    }
#endif

    uint8_t *data = reader->mapped_data;
    size_t file_size = reader->mapped_size;
#else
    /* Use stdio on Windows */
    reader->fp = fopen(filename, "rb");
    if (!reader->fp) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
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
    reader->file_size = (size_t)file_size_off;
    if (FSEEK64(reader->fp, 0, SEEK_SET) != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }

    reader->file_data = PyMem_Malloc(reader->file_size);
    if (!reader->file_data) {
        PyErr_NoMemory();
        goto error;
    }

    if (fread(reader->file_data, 1, reader->file_size, reader->fp) != reader->file_size) {
        PyErr_SetFromErrno(PyExc_IOError);
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

    /* Handle compressed data */
    if (reader->compression_type == COMPRESSION_ZSTD) {
#ifdef HAVE_ZSTD
        if (reader_decompress_samples(reader, data) < 0) {
            goto error;
        }
#else
        PyErr_SetString(PyExc_RuntimeError,
            "File uses zstd compression but zstd support not compiled in");
        goto error;
#endif
    } else {
        reader->sample_data = data + FILE_HEADER_PLACEHOLDER_SIZE;
        reader->sample_data_size = reader->string_table_offset - FILE_HEADER_PLACEHOLDER_SIZE;
    }

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

    if (!reader->thread_states) {
        reader->thread_state_capacity = 16;
        reader->thread_states = PyMem_Calloc(reader->thread_state_capacity, sizeof(ReaderThreadState));
        if (!reader->thread_states) {
            PyErr_NoMemory();
            return NULL;
        }
    } else if (reader->thread_state_count >= reader->thread_state_capacity) {
        reader->thread_states = grow_array(reader->thread_states,
                                           &reader->thread_state_capacity,
                                           sizeof(ReaderThreadState));
        if (!reader->thread_states) {
            return NULL;
        }
    }

    ReaderThreadState *ts = &reader->thread_states[reader->thread_state_count++];
    memset(ts, 0, sizeof(ReaderThreadState));
    ts->thread_id = thread_id;
    ts->interpreter_id = interpreter_id;
    ts->prev_timestamp = reader->start_time_us;
    ts->current_stack_capacity = MAX_STACK_DEPTH;
    ts->current_stack = PyMem_Malloc(ts->current_stack_capacity * sizeof(uint32_t));
    if (!ts->current_stack) {
        PyErr_NoMemory();
        return NULL;
    }
    return ts;
}

/* ============================================================================
 * STACK DECODING HELPERS
 * ============================================================================ */

/* Decode a full stack from sample data.
 * Updates ts->current_stack and ts->current_stack_depth.
 * Returns 0 on success, -1 on error (bounds violation). */
static inline int
decode_stack_full(ReaderThreadState *ts, const uint8_t *data,
                  size_t *offset, size_t max_size)
{
    uint32_t depth = decode_varint_u32(data, offset, max_size);

    /* Validate depth against capacity to prevent buffer overflow */
    if (depth > ts->current_stack_capacity) {
        PyErr_Format(PyExc_ValueError,
            "Stack depth %u exceeds capacity %zu", depth, ts->current_stack_capacity);
        return -1;
    }

    ts->current_stack_depth = depth;
    for (uint32_t i = 0; i < depth; i++) {
        ts->current_stack[i] = decode_varint_u32(data, offset, max_size);
    }
    return 0;
}

/* Decode a suffix-encoded stack from sample data.
 * The suffix encoding shares frames from the bottom of the previous stack.
 * Returns 0 on success, -1 on error (bounds violation). */
static inline int
decode_stack_suffix(ReaderThreadState *ts, const uint8_t *data,
                    size_t *offset, size_t max_size)
{
    uint32_t shared = decode_varint_u32(data, offset, max_size);
    uint32_t new_count = decode_varint_u32(data, offset, max_size);

    /* Validate shared doesn't exceed current stack depth */
    if (shared > ts->current_stack_depth) {
        PyErr_Format(PyExc_ValueError,
            "Shared count %u exceeds current stack depth %zu",
            shared, ts->current_stack_depth);
        return -1;
    }

    /* Validate final depth doesn't exceed capacity */
    size_t final_depth = (size_t)shared + new_count;
    if (final_depth > ts->current_stack_capacity) {
        PyErr_Format(PyExc_ValueError,
            "Final stack depth %zu exceeds capacity %zu",
            final_depth, ts->current_stack_capacity);
        return -1;
    }

    /* Move shared frames (from bottom of stack) to make room for new frames at the top */
    if (new_count > 0 && shared > 0) {
        size_t prev_shared_start = ts->current_stack_depth - shared;
        memmove(&ts->current_stack[new_count],
                &ts->current_stack[prev_shared_start],
                shared * sizeof(uint32_t));
    }

    for (uint32_t i = 0; i < new_count; i++) {
        ts->current_stack[i] = decode_varint_u32(data, offset, max_size);
    }
    ts->current_stack_depth = final_depth;
    return 0;
}

/* Decode a pop-push encoded stack from sample data.
 * Pops frames from the top and pushes new frames.
 * Returns 0 on success, -1 on error (bounds violation). */
static inline int
decode_stack_pop_push(ReaderThreadState *ts, const uint8_t *data,
                      size_t *offset, size_t max_size)
{
    uint32_t pop = decode_varint_u32(data, offset, max_size);
    uint32_t push = decode_varint_u32(data, offset, max_size);
    size_t keep = (ts->current_stack_depth > pop) ? ts->current_stack_depth - pop : 0;

    /* Validate final depth doesn't exceed capacity */
    size_t final_depth = keep + push;
    if (final_depth > ts->current_stack_capacity) {
        PyErr_Format(PyExc_ValueError,
            "Final stack depth %zu exceeds capacity %zu",
            final_depth, ts->current_stack_capacity);
        return -1;
    }

    /* Move kept frames (from bottom of stack) to make room for new frames at the top.
     * Even when push == 0, we need to move kept frames to index 0 if pop > 0. */
    if (keep > 0) {
        memmove(&ts->current_stack[push],
                &ts->current_stack[pop],
                keep * sizeof(uint32_t));
    }

    for (uint32_t i = 0; i < push; i++) {
        ts->current_stack[i] = decode_varint_u32(data, offset, max_size);
    }
    ts->current_stack_depth = final_depth;
    return 0;
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
        uint32_t frame_idx = frame_indices[k];
        if (frame_idx >= reader->frames_count) {
            PyErr_Format(PyExc_ValueError, "Invalid frame index: %u", frame_idx);
            goto error;
        }

        size_t base = frame_idx * 3;
        uint32_t filename_idx = reader->frame_data[base];
        uint32_t funcname_idx = reader->frame_data[base + 1];
        int32_t lineno = (int32_t)reader->frame_data[base + 2];

        if (filename_idx >= reader->strings_count ||
            funcname_idx >= reader->strings_count) {
            PyErr_SetString(PyExc_ValueError, "Invalid string index in frame");
            goto error;
        }

        PyObject *frame_info = PyStructSequence_New(state->FrameInfo_Type);
        if (!frame_info) {
            goto error;
        }

        PyObject *location;
        if (lineno > 0) {
            location = Py_BuildValue("(iiii)", lineno, lineno, 0, 0);
            if (!location) {
                Py_DECREF(frame_info);
                goto error;
            }
        }
        else {
            location = Py_NewRef(Py_None);
        }

        PyStructSequence_SetItem(frame_info, 0, Py_NewRef(reader->strings[filename_idx]));
        PyStructSequence_SetItem(frame_info, 1, location);
        PyStructSequence_SetItem(frame_info, 2, Py_NewRef(reader->strings[funcname_idx]));
        PyStructSequence_SetItem(frame_info, 3, Py_NewRef(Py_None));
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

/* Helper to trim timestamp list and emit batch. Returns 0 on success, -1 on error. */
static int
emit_batch(RemoteDebuggingState *state, PyObject *collector,
           uint64_t thread_id, uint32_t interpreter_id, uint8_t status,
           const uint32_t *frame_indices, size_t stack_depth,
           BinaryReader *reader, PyObject *timestamps_list, Py_ssize_t actual_size)
{
    /* Trim list to actual size */
    if (PyList_SetSlice(timestamps_list, actual_size, PyList_GET_SIZE(timestamps_list), NULL) < 0) {
        return -1;
    }
    return emit_sample(state, collector, thread_id, interpreter_id, status,
                       frame_indices, stack_depth, reader, timestamps_list);
}

/* Helper to invoke progress callback, clearing any errors */
static inline void
invoke_progress_callback(PyObject *callback, Py_ssize_t current, uint32_t total)
{
    if (callback && callback != Py_None) {
        PyObject *result = PyObject_CallFunction(callback, "nI", current, total);
        if (result) {
            Py_DECREF(result);
        } else {
            PyErr_Clear();
        }
    }
}

Py_ssize_t
binary_reader_replay(BinaryReader *reader, PyObject *collector, PyObject *progress_callback)
{
    if (!PyObject_HasAttrString(collector, "collect")) {
        PyErr_SetString(PyExc_TypeError, "Collector must have a collect() method");
        return -1;
    }

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

    size_t offset = 0;
    Py_ssize_t replayed = 0;

    /* Initial progress callback at 0% */
    invoke_progress_callback(progress_callback, 0, reader->sample_count);

    while (offset < reader->sample_data_size) {
        /* Read thread_id (8 bytes) + interpreter_id (4 bytes) */
        if (offset + 13 > reader->sample_data_size) {
            break;  /* End of data */
        }

        /* Use memcpy to avoid strict aliasing violations */
        uint64_t thread_id;
        uint32_t interpreter_id;
        memcpy(&thread_id, &reader->sample_data[offset], sizeof(thread_id));
        offset += 8;

        memcpy(&interpreter_id, &reader->sample_data[offset], sizeof(interpreter_id));
        offset += 4;

        /* Get or create thread state for reconstruction */
        ReaderThreadState *ts = reader_get_or_create_thread_state(reader, thread_id, interpreter_id);
        if (!ts) {
            return -1;
        }

        /* Read encoding byte */
        uint8_t encoding = reader->sample_data[offset++];

        switch (encoding) {
        case STACK_REPEAT: {
            /* RLE repeat: [count: varint] [delta: varint, status: 1]... */
            size_t prev_offset = offset;
            uint32_t count = decode_varint_u32(reader->sample_data, &offset, reader->sample_data_size);
            /* Detect varint decode failure */
            if (offset == prev_offset) {
                PyErr_SetString(PyExc_ValueError, "Malformed varint for RLE count");
                return -1;
            }

            /* Validate RLE count to prevent DoS from malicious files.
             * Each RLE sample needs at least 2 bytes (1 byte min varint + 1 status byte).
             * Also reject absurdly large counts that would exhaust memory. */
            size_t remaining_data = reader->sample_data_size - offset;
            size_t max_possible_samples = remaining_data / 2;
            if (count > max_possible_samples) {
                PyErr_Format(PyExc_ValueError,
                    "Invalid RLE count %u exceeds maximum possible %zu for remaining data",
                    count, max_possible_samples);
                return -1;
            }

            reader->stats.repeat_records++;
            reader->stats.repeat_samples += count;

            /* Process RLE samples, batching by status */
            PyObject *timestamps_list = NULL;
            uint8_t batch_status = 0;
            Py_ssize_t batch_idx = 0;

            for (uint32_t i = 0; i < count; i++) {
                size_t delta_prev_offset = offset;
                uint64_t delta = decode_varint_u64(reader->sample_data, &offset, reader->sample_data_size);
                if (offset == delta_prev_offset) {
                    Py_XDECREF(timestamps_list);
                    PyErr_SetString(PyExc_ValueError, "Malformed varint in RLE sample data");
                    return -1;
                }
                if (offset >= reader->sample_data_size) {
                    Py_XDECREF(timestamps_list);
                    PyErr_SetString(PyExc_ValueError, "Unexpected end of sample data in RLE");
                    return -1;
                }
                uint8_t status = reader->sample_data[offset++];
                ts->prev_timestamp += delta;

                /* Start new batch on first sample or status change */
                if (i == 0 || status != batch_status) {
                    if (timestamps_list) {
                        int rc = emit_batch(state, collector, thread_id, interpreter_id,
                                            batch_status, ts->current_stack, ts->current_stack_depth,
                                            reader, timestamps_list, batch_idx);
                        Py_DECREF(timestamps_list);
                        if (rc < 0) {
                            return -1;
                        }
                    }
                    timestamps_list = PyList_New(count - i);
                    if (!timestamps_list) {
                        return -1;
                    }
                    batch_status = status;
                    batch_idx = 0;
                }

                PyObject *ts_obj = PyLong_FromUnsignedLongLong(ts->prev_timestamp);
                if (!ts_obj) {
                    Py_DECREF(timestamps_list);
                    return -1;
                }
                PyList_SET_ITEM(timestamps_list, batch_idx++, ts_obj);
            }

            /* Emit final batch */
            if (timestamps_list) {
                int rc = emit_batch(state, collector, thread_id, interpreter_id,
                                    batch_status, ts->current_stack, ts->current_stack_depth,
                                    reader, timestamps_list, batch_idx);
                Py_DECREF(timestamps_list);
                if (rc < 0) {
                    return -1;
                }
            }

            replayed += count;
            reader->stats.total_samples += count;

            /* Progress callback after batch */
            if (replayed % PROGRESS_CALLBACK_INTERVAL < count) {
                invoke_progress_callback(progress_callback, replayed, reader->sample_count);
            }
            break;
        }

        case STACK_FULL:
        case STACK_SUFFIX:
        case STACK_POP_PUSH: {
            /* All three encodings share: [delta: varint] [status: 1] ... */
            size_t prev_offset = offset;
            uint64_t delta = decode_varint_u64(reader->sample_data, &offset, reader->sample_data_size);
            /* Detect varint decode failure: offset unchanged means error */
            if (offset == prev_offset) {
                PyErr_SetString(PyExc_ValueError, "Malformed varint in sample data");
                return -1;
            }
            if (offset >= reader->sample_data_size) {
                PyErr_SetString(PyExc_ValueError, "Unexpected end of sample data");
                return -1;
            }
            uint8_t status = reader->sample_data[offset++];
            ts->prev_timestamp += delta;

            if (encoding == STACK_FULL) {
                if (decode_stack_full(ts, reader->sample_data, &offset, reader->sample_data_size) < 0) {
                    return -1;
                }
                reader->stats.full_records++;
            } else if (encoding == STACK_SUFFIX) {
                if (decode_stack_suffix(ts, reader->sample_data, &offset, reader->sample_data_size) < 0) {
                    return -1;
                }
                reader->stats.suffix_records++;
            } else { /* STACK_POP_PUSH */
                if (decode_stack_pop_push(ts, reader->sample_data, &offset, reader->sample_data_size) < 0) {
                    return -1;
                }
                reader->stats.pop_push_records++;
            }
            reader->stats.stack_reconstructions++;

            /* Build single-element timestamp list */
            PyObject *ts_obj = PyLong_FromUnsignedLongLong(ts->prev_timestamp);
            if (!ts_obj) {
                return -1;
            }
            PyObject *timestamps_list = PyList_New(1);
            if (!timestamps_list) {
                Py_DECREF(ts_obj);
                return -1;
            }
            PyList_SET_ITEM(timestamps_list, 0, ts_obj);

            if (emit_sample(state, collector, thread_id, interpreter_id, status,
                           ts->current_stack, ts->current_stack_depth, reader,
                           timestamps_list) < 0) {
                Py_DECREF(timestamps_list);
                return -1;
            }
            Py_DECREF(timestamps_list);
            replayed++;
            reader->stats.total_samples++;
            break;
        }

        default:
            PyErr_Format(PyExc_ValueError, "Unknown stack encoding: %u", encoding);
            return -1;
        }

        /* Progress callback */
        if (replayed % PROGRESS_CALLBACK_INTERVAL == 0) {
            invoke_progress_callback(progress_callback, replayed, reader->sample_count);
        }
    }

    /* Final progress callback at 100% */
    invoke_progress_callback(progress_callback, replayed, reader->sample_count);

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
    return Py_BuildValue(
        "{s:I, s:N, s:K, s:K, s:I, s:I, s:I, s:I, s:i}",
        "version", BINARY_FORMAT_VERSION,
        "python_version", py_version,
        "start_time_us", reader->start_time_us,
        "sample_interval_us", reader->sample_interval_us,
        "sample_count", reader->sample_count,
        "thread_count", reader->thread_count,
        "string_count", reader->strings_count,
        "frame_count", reader->frames_count,
        "compression_type", reader->compression_type
    );
}

PyObject *
binary_writer_get_stats(BinaryWriter *writer)
{
    BinaryWriterStats *s = &writer->stats;

    /* Calculate derived stats */
    uint64_t total_records = s->repeat_records + s->full_records +
                             s->suffix_records + s->pop_push_records;
    uint64_t total_samples = writer->total_samples;
    uint64_t potential_frames = s->total_frames_written + s->frames_saved;
    double compression_ratio = (potential_frames > 0) ?
        (double)s->frames_saved / potential_frames * 100.0 : 0.0;

    return Py_BuildValue(
        "{s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:d}",
        "repeat_records", s->repeat_records,
        "repeat_samples", s->repeat_samples,
        "full_records", s->full_records,
        "suffix_records", s->suffix_records,
        "pop_push_records", s->pop_push_records,
        "total_records", total_records,
        "total_samples", total_samples,
        "total_frames_written", s->total_frames_written,
        "frames_saved", s->frames_saved,
        "bytes_written", s->bytes_written,
        "frame_compression_pct", compression_ratio
    );
}

PyObject *
binary_reader_get_stats(BinaryReader *reader)
{
    BinaryReaderStats *s = &reader->stats;

    uint64_t total_records = s->repeat_records + s->full_records +
                             s->suffix_records + s->pop_push_records;

    return Py_BuildValue(
        "{s:K, s:K, s:K, s:K, s:K, s:K, s:K, s:K}",
        "repeat_records", s->repeat_records,
        "repeat_samples", s->repeat_samples,
        "full_records", s->full_records,
        "suffix_records", s->suffix_records,
        "pop_push_records", s->pop_push_records,
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

    PyMem_Free(reader->filename);

#if USE_MMAP
    if (reader->mapped_data) {
        munmap(reader->mapped_data, reader->mapped_size);
        reader->mapped_data = NULL;  /* Prevent use-after-free */
        reader->mapped_size = 0;
    }
    if (reader->fd >= 0) {
        close(reader->fd);
        reader->fd = -1;  /* Mark as closed */
    }
#else
    if (reader->fp) {
        fclose(reader->fp);
        reader->fp = NULL;
    }
    if (reader->file_data) {
        PyMem_Free(reader->file_data);
        reader->file_data = NULL;
        reader->file_size = 0;
    }
#endif

    PyMem_Free(reader->decompressed_data);

    if (reader->strings) {
        for (uint32_t i = 0; i < reader->strings_count; i++) {
            Py_XDECREF(reader->strings[i]);
        }
        PyMem_Free(reader->strings);
    }

    PyMem_Free(reader->frame_data);

    if (reader->thread_states) {
        for (size_t i = 0; i < reader->thread_state_count; i++) {
            PyMem_Free(reader->thread_states[i].current_stack);
        }
        PyMem_Free(reader->thread_states);
    }

    PyMem_Free(reader);
}
