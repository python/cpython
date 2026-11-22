# Profiling Binary Format

The profiling module includes a binary file format for storing sampling
profiler data. This document describes the format's structure and the
design decisions behind it.

The implementation is in
[`Modules/_remote_debugging/binary_io_writer.c`](../Modules/_remote_debugging/binary_io_writer.c)
and [`Modules/_remote_debugging/binary_io_reader.c`](../Modules/_remote_debugging/binary_io_reader.c),
with declarations in
[`Modules/_remote_debugging/binary_io.h`](../Modules/_remote_debugging/binary_io.h).

## Overview

The sampling profiler can generate enormous amounts of data. A typical
profiling session sampling at 1000 Hz for 60 seconds produces 60,000 samples.
Each sample contains a full call stack, often 20-50 frames deep, and each
frame includes a filename, function name, and line number. In a text-based
format like collapsed stacks, this would mean repeating the same long file
paths and function names thousands of times.

The binary format addresses this through two key strategies:

1. **Deduplication**: Strings and frames are stored once in lookup tables,
   then referenced by small integer indices. A 100-character file path that
   appears in 50,000 samples is stored once, not 50,000 times.

2. **Compact encoding**: Variable-length integers (varints) encode small
   values in fewer bytes. Since most indices are small (under 128), they
   typically need only one byte instead of four.

Together with optional zstd compression, these techniques reduce file sizes
by 10-50x compared to text formats while also enabling faster I/O.

## File Layout

The file consists of five required sections and one optional extension:

```
+------------------+  Offset 0
|     Header       |  64 bytes (fixed)
+------------------+  Offset 64
|                  |
|   Sample Data    |  Blocks of columns (optionally compressed)
|                  |
+------------------+  string_table_offset
|   String Table   |  Variable size
+------------------+  frame_table_offset
|   Frame Table    |  Variable size
+------------------+  file_size - 88 (when stats are present)
| Profile Stats    |  56 bytes (optional)
+------------------+  file_size - 32
|     Footer       |  32 bytes (fixed)
+------------------+  file_size
```

The layout is designed for streaming writes during profiling. The profiler
cannot know in advance how many unique strings or frames will be encountered,
so these tables must be built incrementally and written at the end.

The header comes first so readers can quickly validate the file and locate
the metadata tables. The sample data follows immediately, allowing the writer
to write each block of samples as soon as it is full without buffering the
entire dataset in memory.

The string and frame tables are placed after sample data because they grow
as new unique entries are discovered during profiling. By deferring their
output until finalization, the writer avoids the complexity of reserving
space or rewriting portions of the file.

The footer at the end contains counts needed to allocate arrays before
parsing the tables. Placing it at a fixed offset from the end (rather than
at a variable offset recorded in the header) means readers can locate it
with a single seek to `file_size - 32`, without first reading the header.

## Header

```
 Offset   Size   Type      Description
+--------+------+---------+----------------------------------------+
|    0   |  4   | uint32  | Magic number (0x54414348 = "TACH")     |
|    4   |  4   | uint32  | Format version                         |
|    8   |  4   | bytes   | Python version (major, minor, micro,   |
|        |      |         | reserved)                              |
|   12   |  8   | uint64  | Start timestamp (microseconds)         |
|   20   |  8   | uint64  | Sample interval (microseconds)         |
|   28   |  8   | uint64  | Total sample count                     |
|   36   |  4   | uint32  | Thread count                           |
|   40   |  8   | uint64  | String table offset                    |
|   48   |  8   | uint64  | Frame table offset                     |
|   56   |  4   | uint32  | Compression type (0=none, 1=zstd)      |
|   60   |  4   | uint32  | Profiling configuration bit field      |
+--------+------+---------+----------------------------------------+
```

The low three configuration bits contain the
`_remote_debugging.PROFILING_MODE_*` value plus one. Zero means that the mode
was not recorded, which is also the value in binaries written before this
field was defined. Bit 3 indicates that capture features are known; when set,
bits 4 through 8 respectively record `--all-threads`, `--native`, GC frames,
`--opcodes`, and `--blocking`. Remaining bits are reserved for future capture
features.

The magic number `0x54414348` ("TACH" for Tachyon) identifies the file format
and also serves as an **endianness marker**. When read on a system with
different byte order than the writer, it appears as `0x48434154`. The reader
uses this to detect cross-endian files and automatically byte-swap all
multi-byte integer fields.

The Python version field records the major, minor, and micro version numbers
of the Python interpreter that generated the file. This allows analysis tools
to detect version mismatches when replaying data collected on a different
Python version, which may have different internal structures or behaviors.

The header is written as zeros initially, then overwritten with actual values
during finalization. This requires the output stream to be seekable, which
is acceptable since the format targets regular files rather than pipes or
network streams.

## Sample Data

Sample data begins at offset 64 and extends to `string_table_offset`. Samples
are grouped into blocks of at most 1 MiB of decoded data. Within a block,
samples use delta compression to minimize redundancy when consecutive samples
from the same thread have identical or similar call stacks. Each block can be
decoded on its own once the string and frame tables are loaded, so a reader
only needs to decode one block at a time.

### Block Format

A block stores values of the same kind together, in six columns:

```
+------------------------+
| Block header           |  21 to 88 bytes
+------------------------+
| STREAM_TABLE           |  (thread_id, interpreter_id) pairs
+------------------------+
| SAMPLE_STREAM_IDS      |  One stream index per sample
+------------------------+
| TIMESTAMP_RESIDUALS    |  One timestamp residual per sample
+------------------------+
| STATUS_BITMAPS         |  One bitmap per varying status flag
+------------------------+
| STACK_INSTRUCTIONS     |  Stack encodings
+------------------------+
| FRAME_IDS              |  Frame table indices
+------------------------+
```

**Block header:**
```
+---------------------+-----------+----------------------------------------+
| magic               | 4 bytes   | "TBLK"                                 |
| sample_count        | varint    | Number of samples in this block        |
| base_timestamp      | varint    | Timestamp of the first sample          |
| frame_id_width      | 1 byte    | Bytes per frame index (1 to 4)         |
| common_status_flags | 1 byte    | Status bits set in every sample        |
| status_bitmap_flags | 1 byte    | Status bits stored as bitmaps          |
| columns             | varies    | One column descriptor per column       |
+---------------------+-----------+----------------------------------------+
```

**Column descriptor:**
```
+---------------------+-----------+----------------------------------------+
| decoded_size        | varint    | Size of the decoded column             |
| encoding            | 1 byte    | 0=raw, 1=zstd, 2=constant              |
| stored_size         | varint    | Compressed size (zstd only)            |
+---------------------+-----------+----------------------------------------+
```

A raw column is stored as is, and a constant column is a single byte repeated
`decoded_size` times, so neither needs a stored size. The number of streams
and frame indices in a block follows from the decoded sizes of
`STREAM_TABLE` and `FRAME_IDS`.

A block holds at most 4,096 streams. There is no separate limit on the number
of samples: the stream index and timestamp columns need at least one byte per
sample, so the 1 MiB limit already bounds it.

### Stack Encoding Types

Each thread's stack is reconstructed from the previous stack of the same
thread in the block. The first sample of a thread in a block starts from an
empty stack. `STACK_INSTRUCTIONS` holds one of these encodings for every
sample not covered by a `REPEAT`:

| Code | Name | Description |
|------|------|-------------|
| 0x00 | REPEAT | RLE: identical stack repeated N times |
| 0x01 | REPLACE_SUFFIX | Keeps N outer frames of the previous stack, adds M new frames |
| 0x02 | REUSE | Complete stack stored earlier in the block |

Stacks hold at most 256 frames. The writer keeps the innermost 256 frames of
deeper stacks.

### Stack Encoding Formats

All fields are varints. Stacks are written outermost frame first.

**REPEAT (0x00) - Run-Length Encoded Identical Stacks:**
```
+-----------------+-----------+----------------------------------------+
| encoding        | varint    | 0x00 (REPEAT)                          |
| count           | varint    | Number of samples in this RLE group    |
+-----------------+-----------+----------------------------------------+
```
The stack is inherited from this thread's previous sample, for this sample
and the thread's next `count - 1` samples. Samples from other threads can
appear in between: the reader keeps a remaining count for each thread and
reads no encoding for that thread until the count reaches zero. Each sample
in the group keeps its own timestamp and status in the other columns.

**REPLACE_SUFFIX (0x01) - Shared Outer Frames:**
```
+-----------------+-----------+----------------------------------------+
| encoding        | varint    | 0x01 (REPLACE_SUFFIX)                  |
| keep_count      | varint    | Outer frames kept from previous stack  |
| new_count       | varint    | Inner frames read from FRAME_IDS       |
+-----------------+-----------+----------------------------------------+
```
Used when a call or return changed the inner part of the stack: the outer
frames are kept, and `new_count` new frames are read from `FRAME_IDS`. A
complete stack, such as the first sample of a thread, uses `keep_count` 0.

**REUSE (0x02) - Repeated Complete Stack:**
```
+-----------------+-----------+----------------------------------------+
| encoding        | varint    | 0x02 (REUSE)                           |
| offset          | varint    | Index of the first frame in FRAME_IDS  |
| count           | varint    | Number of frames in the stack          |
+-----------------+-----------+----------------------------------------+
```
Used when a complete stack was already written earlier in the same block,
for example by another thread running the same code. The frames are copied
from `FRAME_IDS` instead of being stored again.

For example, two threads produce four samples:

| Sample | Thread | Stack          | Encoding             |
|--------|--------|----------------|----------------------|
| 0      | A      | `[4, 21, 37]`  | `REPLACE_SUFFIX 0 3` |
| 1      | B      | `[4, 21, 37]`  | `REUSE 0 3`          |
| 2      | A      | `[4, 21, 52]`  | `REPLACE_SUFFIX 2 1` |
| 3      | A      | `[4, 21, 52]`  | `REPEAT 1`           |

`FRAME_IDS` holds only `[4, 21, 37, 52]`.

### Thread and Interpreter Identification

Each `(thread_id, interpreter_id)` pair used in a block is stored once in
`STREAM_TABLE`, as an 8-byte thread ID followed by a 4-byte interpreter ID.
`SAMPLE_STREAM_IDS` then holds one varint per sample: the index of its pair
in the table. Samples are kept in arrival order, so samples from different
threads are interleaved.

Thread IDs are 64-bit values that can be large (memory addresses on some
platforms) and vary unpredictably. Using a fixed 8-byte encoding avoids
the overhead of varint encoding for large values and simplifies parsing
since the reader knows exactly where each field begins.

The interpreter ID identifies which Python sub-interpreter the thread
belongs to, allowing analysis tools to separate activity across interpreters
in processes using multiple sub-interpreters.

### Status Byte

The status byte is a bitfield encoding thread state at sample time:

| Bit | Flag                  | Meaning                                    |
|-----|-----------------------|--------------------------------------------|
|  0  | THREAD_STATUS_HAS_GIL | Thread holds the GIL (Global Interpreter Lock) |
|  1  | THREAD_STATUS_ON_CPU  | Thread is actively running on a CPU core   |
|  2  | THREAD_STATUS_UNKNOWN | Thread state could not be determined       |
|  3  | THREAD_STATUS_GIL_REQUESTED | Thread is waiting to acquire the GIL  |
|  4  | THREAD_STATUS_HAS_EXCEPTION | Thread has a pending exception         |
|  5  | THREAD_STATUS_MAIN_THREAD | Thread is the interpreter's main thread |

Multiple flags can be set simultaneously (e.g., a thread can hold the GIL
while also running on CPU). Analysis tools use these to filter samples or
visualize thread states over time.

Most flags rarely change within a block, so the status bytes are stored as
bit planes. Bits set in every sample are recorded once in
`common_status_flags`. Each bit in `status_bitmap_flags` has a bitmap of
`ceil(sample_count / 8)` bytes in `STATUS_BITMAPS`, in increasing bit order,
with sample `i` in bit `i % 8` of byte `i / 8`. Bits that never occur take no
space. The reader rebuilds each status byte from `common_status_flags` and
the sample's bit in each bitmap.

### Timestamp Delta Encoding

Timestamps use delta encoding rather than absolute values. Absolute
timestamps in microseconds require 8 bytes each, but consecutive samples
from the same thread are typically separated by the sampling interval
(e.g., 1000 microseconds), so each timestamp is expected to be the thread's
previous timestamp plus the interval, and only the difference from that
expected timestamp is stored. The writer tracks the previous timestamp for
each thread separately. The expected timestamp for a thread's first sample
in a block is the block's `base_timestamp`. This per-thread tracking is
necessary because samples are interleaved across threads in arrival order,
not grouped by thread.

Each residual is a signed 64-bit zigzag varint. A sample taken exactly on
schedule has a residual of 0 and takes one byte, while timestamps are still
preserved exactly.

### Frame Indexing

Each frame in a call stack is represented by an index into the frame table
rather than inline data. This provides massive space savings because call
stacks are highly repetitive: the same function appears in many samples
(hot functions), call stacks often share common prefixes (main -> app ->
handler -> ...), and recursive functions create repeated frame sequences.
A frame index is typically 1-2 bytes. Inline frame data would be 20-200+
bytes (two strings plus a line number). For a profile with 100,000 samples
averaging 30 frames each, this reduces frame data from potentially
gigabytes to tens of megabytes.

`FRAME_IDS` stores the indices read by `REPLACE_SUFFIX`, using the smallest
byte width that fits the largest index in the block. The bytes are split into
planes, low bytes first, so similar bytes sit together for compression:

```
indices:            0x1234  0x5678  0x9abc
low-byte plane:       0x34    0x78    0xbc
high-byte plane:      0x12    0x56    0x9a
```

Frame indices are written outermost-first (the outermost frame has index 0
in the array). This ordering works well with delta compression: function
calls typically change frames at the end of the array, while shared frames
remain at the start.

## String Table

The string table stores deduplicated UTF-8 strings (filenames and function
names). It begins at `string_table_offset` and contains entries in order of
their assignment during writing:

```
+----------------+
| length: varint |
| data: bytes    |
+----------------+  (repeated for each string)
```

Strings are stored in the order they were first encountered during writing.
The first unique filename gets index 0, the second gets index 1, and so on.
Length-prefixing (rather than null-termination) allows strings containing
null bytes and enables readers to allocate exact-sized buffers. The varint
length encoding means short strings (under 128 bytes) need only one length
byte.

## Frame Table

The frame table stores deduplicated frame entries with full source position
information and bytecode opcode:

```
+----------------------------+
| filename_idx: varint       |
| funcname_idx: varint       |
| lineno: svarint            |
| end_lineno_delta: svarint  |
| column: svarint            |
| end_column_delta: svarint  |
| opcode: u8                 |
+----------------------------+  (repeated for each frame)
```

### Field Definitions

| Field            | Type          | Description                                              |
|------------------|---------------|----------------------------------------------------------|
| filename_idx     | varint        | Index into string table for file name                    |
| funcname_idx     | varint        | Index into string table for function name                |
| lineno           | zigzag varint | Start line number (-1 for synthetic frames)              |
| end_lineno_delta | zigzag varint | Delta from lineno (end_lineno = lineno + delta)          |
| column           | zigzag varint | Start column offset in UTF-8 bytes (-1 if not available) |
| end_column_delta | zigzag varint | Delta from column (end_column = column + delta)          |
| opcode           | u8            | Python bytecode opcode (0-254) or 255 for None           |

### Delta Encoding

Position end values use delta encoding for efficiency:

- `end_lineno = lineno + end_lineno_delta`
- `end_column = column + end_column_delta`

Typical values:
- `end_lineno_delta`: Usually 0 (single-line expressions) → encodes to 1 byte
- `end_column_delta`: Usually 5-20 (expression width) → encodes to 1 byte

This saves ~1-2 bytes per frame compared to absolute encoding. When the base
value (lineno or column) is -1 (not available), the delta is stored as 0 and
the reconstructed value is -1.

### Sentinel Values

- `opcode = 255`: No opcode captured
- `lineno = -1`: Synthetic frame (no source location)
- `column = -1`: Column offset not available

### Deduplication

Each unique (filename, funcname, lineno, end_lineno, column, end_column,
opcode) combination gets one entry. This enables instruction-level profiling
where multiple bytecode instructions on the same line can be distinguished.

Strings and frames are deduplicated separately because they have different
cardinalities and reference patterns. A codebase might have hundreds of
unique source files but thousands of unique functions. Many functions share
the same filename, so storing the filename index in each frame entry (rather
than the full string) provides an additional layer of deduplication. A frame
entry is typically 7-9 bytes rather than two full strings plus location data.

### Size Analysis

Typical frame size with delta encoding:
- file_idx: 1-2 bytes
- func_idx: 1-2 bytes
- lineno: 1-2 bytes
- end_lineno_delta: 1 byte (usually 0)
- column: 1 byte (usually < 64)
- end_column_delta: 1 byte (usually < 64)
- opcode: 1 byte

**Total: ~7-9 bytes per frame**

Line numbers and columns use signed varint (zigzag encoding) to handle
sentinel values efficiently. Synthetic frames—generated frames that don't
correspond directly to Python source code, such as C extension boundaries or
internal interpreter frames—use -1 to indicate the absence of a source
location. Zigzag encoding ensures these small negative values encode
efficiently (−1 becomes 1, which is one byte) rather than requiring the
maximum varint length.

## Profile Statistics

New files can store measured duration, sampling rate, error rate, and missed
sample percentage in an optional 56-byte extension immediately before the
footer. Older readers ignore these
bytes after parsing the declared number of frame-table entries, and newer
readers treat a missing extension as unavailable statistics.

```
 Offset   Size   Type      Description
+--------+------+---------+----------------------------------------+
|    0   |  8   | double  | Measured duration (seconds)            |
|    8   |  8   | double  | Measured sample rate (samples/second)  |
|   16   |  8   | double  | Failed sample percentage               |
|   24   |  8   | double  | Missed sample percentage               |
|   32   |  4   | uint32  | Optional field presence flags          |
|   36   |  4   | uint32  | Reserved                               |
|   40   |  8   | bytes   | Signature ("TACHSTAT")                 |
|   48   |  4   | uint32  | Extension version (1)                  |
|   52   |  4   | uint32  | Extension size (56)                    |
+--------+------+---------+----------------------------------------+
```

Putting the signature, version, and size at the end lets readers discover
the extension from its fixed position relative to the footer while allowing
future versions to add fields before that trailer. Multi-byte values use the
same native byte order as the rest of the file and are byte-swapped by
cross-endian readers.

Readers also accept the original 32-byte extension, which only contains the
duration and sampling rate.

## Footer

```
 Offset   Size   Type      Description
+--------+------+---------+----------------------------------------+
|    0   |  4   | uint32  | String count                           |
|    4   |  4   | uint32  | Frame count                            |
|    8   |  8   | uint64  | Total file size                        |
|   16   | 16   | bytes   | Checksum (reserved, currently zeros)   |
+--------+------+---------+----------------------------------------+
```

The string and frame counts allow readers to pre-allocate arrays of the
correct size before parsing the tables. Without these counts, readers would
need to either scan the tables twice (once to count, once to parse) or use
dynamically-growing arrays.

The file size field provides a consistency check: if the actual file size
does not match, the file may be truncated or corrupted.

The checksum field is reserved for future use. A checksum would allow
detection of corruption but adds complexity and computation cost. The
current implementation leaves this as zeros.

## Variable-Length Integer Encoding

The format uses LEB128 (Little Endian Base 128) for unsigned integers and
zigzag + LEB128 for signed integers. These encodings are widely used
(Protocol Buffers, DWARF debug info, WebAssembly) and well-understood.

### Unsigned Varint (LEB128)

Each byte stores 7 bits of data. The high bit indicates whether more bytes
follow:

```
Value        Encoded bytes
0-127        [0xxxxxxx]                    (1 byte)
128-16383    [1xxxxxxx] [0xxxxxxx]         (2 bytes)
16384+       [1xxxxxxx] [1xxxxxxx] ...     (3+ bytes)
```

Most indices in profiling data are small. A profile with 1000 unique frames
needs at most 2 bytes per frame index. The common case (indices under 128)
needs only 1 byte.

### Signed Varint (Zigzag)

Standard LEB128 encodes −1 as a very large unsigned value, requiring many
bytes. Zigzag encoding interleaves positive and negative values:

```
 0 -> 0    -1 -> 1     1 -> 2    -2 -> 3     2 -> 4
```

This ensures small-magnitude values (whether positive or negative) encode
in few bytes. Frame table deltas use 32-bit zigzag values and timestamp
residuals use 64-bit zigzag values.

## Compression

The header's compression type controls whether zstd may be used. Each column
of each block is encoded separately, using whichever representation is
smallest:

- **constant**, when every byte of the column is the same, such as the stream
  IDs of a single-threaded block or residuals of on-time samples;
- **zstd**, when compression is enabled and the compressed column plus its
  stored-size varint is smaller than the raw column;
- **raw** otherwise.

The string table, frame table, and footer remain uncompressed so readers can
access metadata without decompressing the entire file. A tool that only needs
to report "this file contains 50,000 samples of 3 threads" can read the header
and footer without touching the compressed sample data.

Zstd provides an excellent balance of compression ratio and speed. Profiling
data compresses very well (often 5-10x) due to repetitive patterns: the same
small set of frame indices appears repeatedly, and timestamp residuals
cluster around zero.

Level 3 compression is used as a default. It gives good compression without
slowing down each block flush. Compressing columns separately keeps blocks
independent, at the cost of restarting compression history at every block.

## Reading and Writing

### Writing

1. Open the output file and write 64 zero bytes as a placeholder header
2. Initialize empty string and frame dictionaries for deduplication
3. For each sample:
   - Intern any new strings, assigning sequential indices
   - Intern any new frames, assigning sequential indices
   - Append the sample to the current block's columns
   - Flush the block when it reaches its size or stream limit
4. Flush the last block
5. Write the string table (length-prefixed strings in index order)
6. Write the frame table (varint-encoded entries in index order)
7. Write measured profile statistics, when available
8. Write the footer with final counts
9. Seek to offset 0 and write the header with actual values

The writer maintains two dictionaries: one mapping strings to indices, one
mapping (filename_idx, funcname_idx, lineno) tuples to frame indices. These
enable O(1) lookup during interning. To find `REUSE` candidates, it also keeps
a small per-block cache of complete stacks, confirmed by a full comparison.

When a block is flushed, the status and frame ID columns are transposed into
bit and byte planes, the `REPEAT` counts are inserted into the instruction
column, and each column is encoded as described in
[Compression](#compression).

### Reading

1. Read the header magic number to detect endianness (set `needs_swap` flag
   if the magic appears byte-swapped)
2. Validate version and read remaining header fields (byte-swapping if needed)
3. Seek to end − 32 and read the footer (byte-swapping counts if needed)
4. Read measured profile statistics when the optional extension is present
5. Allocate string array of `string_count` elements
6. Parse the string table, populating the array
7. Allocate the frame array
8. Parse the frame table, populating the array
9. For each block, decode its columns into a reusable buffer and replay its
   samples in order, resolving indices to strings/frames (byte-swapping the
   stream table if needed)

The reader builds lookup arrays rather than dictionaries since it only needs
index-to-value mapping, not value-to-index.

Consecutive samples of one stream with the same stack and status are passed
to the collector in a single call, with up to 8,192 timestamps.

The block header and columns are validated before any sample of the block is
replayed. Stack instructions are checked as they are replayed, so a block
with a bad instruction may already have passed earlier samples to the
collector before the error is raised. The reader rejects a block when:

- a size, encoding, or stream or frame reference is out of range, or a
  column has trailing data;
- the stream table lists a thread twice or has a stream with no samples;
- a stream's first instruction in the block is `REPEAT`, or a `REPEAT`
  count is zero or runs past the block;
- a stack would be deeper than 256 frames, or `REUSE` refers to frame IDs
  that have not been read yet;
- not every frame ID is used;
- status bitmap padding bits are not zero;
- it uses zstd in an uncompressed file.

## Platform Considerations

### Byte Ordering and Cross-Platform Portability

The binary format uses **native byte order** for all multi-byte integer
fields when writing. However, the reader supports **cross-endian reading**:
files written on a little-endian system (x86, ARM) can be read on a
big-endian system (s390x, PowerPC), and vice versa.

The magic number doubles as an endianness marker. When read on a system with
different byte order, it appears byte-swapped (`0x48434154` instead of
`0x54414348`). The reader detects this and automatically byte-swaps all
fixed-width integer fields during parsing.

Writers must use `memcpy()` from properly-sized integer types when writing
fixed-width integer fields. When the source variable's type differs from the
field width (e.g., `size_t` written as 4 bytes), explicit casting to the
correct type (e.g., `uint32_t`) is required before `memcpy()`. On big-endian
systems, copying from an oversized type would copy the wrong bytes—high-order
zeros instead of the actual value.

The reader tracks whether byte-swapping is needed via a `needs_swap` flag set
during header parsing. All fixed-width fields in the header, footer, and
stream tables are conditionally byte-swapped using Python's internal byte-swap
functions (`_Py_bswap32`, `_Py_bswap64` from `pycore_bitutils.h`).

Variable-length integers (varints) are byte-order independent since they
encode values one byte at a time using the LEB128 scheme, so they require
no special handling for cross-endian reading.

### Memory-Mapped I/O

On Unix systems (Linux, macOS), the reader uses `mmap()` to map the file
into the process address space. The kernel handles paging data in and out
as needed, no explicit read() calls or buffer management are required,
multiple readers can share the same physical pages, and sequential access
patterns benefit from kernel read-ahead.

The implementation uses `madvise()` to hint the access pattern to the kernel:
`MADV_SEQUENTIAL` indicates the file will be read linearly, enabling
aggressive read-ahead. `MADV_WILLNEED` requests pre-faulting of pages.
On Linux, `MAP_POPULATE` pre-faults all pages at mmap time rather than on
first access, moving page fault overhead from the parsing loop to the
initial mapping for more predictable performance. For large files (over
32 MB), `MADV_HUGEPAGE` requests transparent huge pages (2 MB instead of
4 KB) to reduce TLB pressure when accessing large amounts of data.

On Windows, the implementation falls back to standard file I/O with full
file buffering. Profiling data files are typically small enough (tens to
hundreds of megabytes) that this is acceptable.

The writer buffers samples in the current block and writes each completed
block with a single call, so individual samples never cause a write.

## Future Considerations

The optional profile-statistics block provides an extensible metadata area.
The Python-version field retains one reserved byte. The 16-byte checksum
field in the footer is currently unused. The version field allows
incompatible changes with graceful rejection. New compression types could
be added (compression_type > 1).

Any changes that alter the meaning of existing fields or the parsing logic
should increment the version number to prevent older readers from
misinterpreting new files.
