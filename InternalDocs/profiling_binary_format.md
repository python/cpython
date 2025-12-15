# Profiling Binary Format

The profiling module includes a binary file format for storing sampling
profiler data. This document describes the format's structure and the
design decisions behind it.

The implementation is in
[`Modules/_remote_debugging/binary_io.c`](../Modules/_remote_debugging/binary_io.c)
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

The file consists of five sections:

```
+------------------+  Offset 0
|     Header       |  64 bytes (fixed)
+------------------+  Offset 64
|                  |
|   Sample Data    |  Variable size (optionally compressed)
|                  |
+------------------+  string_table_offset
|   String Table   |  Variable size
+------------------+  frame_table_offset
|   Frame Table    |  Variable size
+------------------+  file_size - 32
|     Footer       |  32 bytes (fixed)
+------------------+  file_size
```

The layout is designed for streaming writes during profiling. The profiler
cannot know in advance how many unique strings or frames will be encountered,
so these tables must be built incrementally and written at the end.

The header comes first so readers can quickly validate the file and locate
the metadata tables. The sample data follows immediately, allowing the writer
to stream samples directly to disk (or through a compression stream) without
buffering the entire dataset in memory.

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
|    4   |  4   | uint32  | Format version (currently 2)           |
|    8   |  8   | uint64  | Start timestamp (microseconds)         |
|   16   |  8   | uint64  | Sample interval (microseconds)         |
|   24   |  4   | uint32  | Total sample count                     |
|   28   |  4   | uint32  | Thread count                           |
|   32   |  8   | uint64  | String table offset                    |
|   40   |  8   | uint64  | Frame table offset                     |
|   48   |  4   | uint32  | Compression type (0=none, 1=zstd)      |
|   52   | 12   | bytes   | Reserved (zero-filled)                 |
+--------+------+---------+----------------------------------------+
```

The header is written as zeros initially, then overwritten with actual values
during finalization. This requires the output stream to be seekable, which
is acceptable since the format targets regular files rather than pipes or
network streams.

## Sample Data

Sample data begins at offset 64 and extends to `string_table_offset`. Samples
use delta compression to minimize redundancy when consecutive samples from the
same thread have identical or similar call stacks.

### Stack Encoding Types

Each sample record begins with thread identification, then an encoding byte:

| Code | Name | Description |
|------|------|-------------|
| 0x00 | REPEAT | RLE: identical stack repeated N times |
| 0x01 | FULL | Complete stack (first sample or no match) |
| 0x02 | SUFFIX | Shares N frames from bottom of previous stack |
| 0x03 | POP_PUSH | Remove M frames from top, add N new frames |

### Record Formats

**REPEAT (0x00) - Run-Length Encoded Identical Stacks:**
```
+-----------------+-----------+----------------------------------------+
| thread_id       | 8 bytes   | Thread identifier (uint64, fixed)      |
| interpreter_id  | 4 bytes   | Interpreter ID (uint32, fixed)         |
| encoding        | 1 byte    | 0x00 (REPEAT)                          |
| count           | varint    | Number of samples in this RLE group    |
| samples         | varies    | Interleaved: [delta: varint, status: 1]|
|                 |           | repeated count times                   |
+-----------------+-----------+----------------------------------------+
```
The stack is inherited from this thread's previous sample. Each sample in the
group gets its own timestamp delta and status byte, stored as interleaved pairs
(delta1, status1, delta2, status2, ...) rather than separate arrays.

**FULL (0x01) - Complete Stack:**
```
+-----------------+-----------+----------------------------------------+
| thread_id       | 8 bytes   | Thread identifier (uint64, fixed)      |
| interpreter_id  | 4 bytes   | Interpreter ID (uint32, fixed)         |
| encoding        | 1 byte    | 0x01 (FULL)                            |
| timestamp_delta | varint    | Microseconds since thread's last sample|
| status          | 1 byte    | Thread state flags                     |
| stack_depth     | varint    | Number of frames in call stack         |
| frame_indices   | varint[]  | Array of frame table indices           |
+-----------------+-----------+----------------------------------------+
```
Used for the first sample from a thread, or when delta encoding would not
provide savings.

**SUFFIX (0x02) - Shared Suffix Match:**
```
+-----------------+-----------+----------------------------------------+
| thread_id       | 8 bytes   | Thread identifier (uint64, fixed)      |
| interpreter_id  | 4 bytes   | Interpreter ID (uint32, fixed)         |
| encoding        | 1 byte    | 0x02 (SUFFIX)                          |
| timestamp_delta | varint    | Microseconds since thread's last sample|
| status          | 1 byte    | Thread state flags                     |
| shared_count    | varint    | Frames shared from bottom of prev stack|
| new_count       | varint    | New frames at top of stack             |
| new_frames      | varint[]  | Array of new_count frame indices       |
+-----------------+-----------+----------------------------------------+
```
Used when a function call added frames to the top of the stack. The shared
frames from the previous stack are kept, and new frames are prepended.

**POP_PUSH (0x03) - Pop and Push:**
```
+-----------------+-----------+----------------------------------------+
| thread_id       | 8 bytes   | Thread identifier (uint64, fixed)      |
| interpreter_id  | 4 bytes   | Interpreter ID (uint32, fixed)         |
| encoding        | 1 byte    | 0x03 (POP_PUSH)                        |
| timestamp_delta | varint    | Microseconds since thread's last sample|
| status          | 1 byte    | Thread state flags                     |
| pop_count       | varint    | Frames to remove from top of prev stack|
| push_count      | varint    | New frames to add at top               |
| new_frames      | varint[]  | Array of push_count frame indices      |
+-----------------+-----------+----------------------------------------+
```
Used when the code path changed: some frames were popped (function returns)
and new frames were pushed (different function calls).

### Compression Benefits

This delta encoding provides massive savings for typical profiling workloads:

- **CPU-bound code**: Hot loops produce many identical samples. RLE encoding
  compresses 100 identical samples to just 2-3 bytes of overhead plus the
  timestamp/status data.

- **I/O-bound code**: Alternating between wait and work produces similar
  stacks with small variations. SUFFIX encoding captures this efficiently.

- **Call-heavy code**: Functions calling other functions share common stack
  prefixes. POP_PUSH encoding only stores the changed frames.

### Thread and Interpreter Identification

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

Multiple flags can be set simultaneously (e.g., a thread can hold the GIL
while also running on CPU). Analysis tools use these to filter samples or
visualize thread states over time.

### Timestamp Delta Encoding

Timestamps use delta encoding rather than absolute values. Absolute
timestamps in microseconds require 8 bytes each, but consecutive samples
from the same thread are typically separated by the sampling interval
(e.g., 1000 microseconds), so the delta between them is small and fits
in 1-2 varint bytes. The writer tracks the previous timestamp for each
thread separately. The first sample from a thread encodes its delta from
the profiling start time; subsequent samples encode the delta from that
thread's previous sample. This per-thread tracking is necessary because
samples are interleaved across threads in arrival order, not grouped by
thread.

For REPEAT (RLE) records, timestamp deltas and status bytes are stored as
interleaved pairs (delta, status, delta, status, ...) - one pair per
repeated sample - allowing efficient batching while preserving the exact
timing and state of each sample.

### Frame Indexing

Each frame in a call stack is represented by an index into the frame table
rather than inline data. This provides massive space savings because call
stacks are highly repetitive: the same function appears in many samples
(hot functions), call stacks often share common prefixes (main -> app ->
handler -> ...), and recursive functions create repeated frame sequences.
A frame index is typically 1-2 varint bytes. Inline frame data would be
20-200+ bytes (two strings plus a line number). For a profile with 100,000
samples averaging 30 frames each, this reduces frame data from potentially
gigabytes to tens of megabytes.

Frame indices are written innermost-first (the currently executing frame
has index 0 in the array). This ordering works well with delta compression:
function calls typically add frames at the top (index 0), while shared
frames remain at the bottom.

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

The frame table stores deduplicated frame entries:

```
+----------------------+
| filename_idx: varint |
| funcname_idx: varint |
| lineno: svarint      |
+----------------------+  (repeated for each frame)
```

Each unique (filename, funcname, lineno) combination gets one entry. Two
calls to the same function at different line numbers produce different
frame entries; two calls at the same line number share one entry.

Strings and frames are deduplicated separately because they have different
cardinalities and reference patterns. A codebase might have hundreds of
unique source files but thousands of unique functions. Many functions share
the same filename, so storing the filename index in each frame entry (rather
than the full string) provides an additional layer of deduplication. A frame
entry is just three varints (typically 3-6 bytes) rather than two full
strings plus a line number.

Line numbers use signed varint (zigzag encoding) rather than unsigned to
handle edge cases. Synthetic frames—generated frames that don't correspond
directly to Python source code, such as C extension boundaries or internal
interpreter frames—use line number 0 or -1 to indicate the absence of a
source location. Zigzag encoding ensures these small negative values encode
efficiently (−1 becomes 1, which is one byte) rather than requiring the
maximum varint length.

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
in few bytes.

## Compression

When compression is enabled, the sample data region contains a zstd stream.
The string table, frame table, and footer remain uncompressed so readers can
access metadata without decompressing the entire file. A tool that only needs
to report "this file contains 50,000 samples of 3 threads" can read the header
and footer without touching the compressed sample data. This also simplifies
the format: the header's offset fields point directly to the tables rather
than to positions within a decompressed stream.

Zstd provides an excellent balance of compression ratio and speed. Profiling
data compresses very well (often 5-10x) due to repetitive patterns: the same
small set of frame indices appears repeatedly, and delta-encoded timestamps
cluster around the sampling interval. Zstd's streaming API allows compression
without buffering the entire dataset. The writer feeds sample data through
the compressor incrementally, flushing compressed chunks to disk as they
become available.

Level 5 compression is used as a default. Lower levels (1-3) are faster but
compress less; higher levels (6+) compress more but slow down writing. Level
5 provides good compression with minimal impact on profiling overhead.

## Reading and Writing

### Writing

1. Open the output file and write 64 zero bytes as a placeholder header
2. Initialize empty string and frame dictionaries for deduplication
3. For each sample:
   - Intern any new strings, assigning sequential indices
   - Intern any new frames, assigning sequential indices
   - Encode the sample record and write to the buffer
   - Flush the buffer through compression (if enabled) when full
4. Flush remaining buffered data and finalize compression
5. Write the string table (length-prefixed strings in index order)
6. Write the frame table (varint-encoded entries in index order)
7. Write the footer with final counts
8. Seek to offset 0 and write the header with actual values

The writer maintains two dictionaries: one mapping strings to indices, one
mapping (filename_idx, funcname_idx, lineno) tuples to frame indices. These
enable O(1) lookup during interning.

### Reading

1. Read the header and validate magic/version
2. Seek to end − 32 and read the footer
3. Allocate string array of `string_count` elements
4. Parse the string table, populating the array
5. Allocate frame array of `frame_count * 3` uint32 elements
6. Parse the frame table, populating the array
7. If compressed, decompress the sample data region
8. Iterate through samples, resolving indices to strings/frames

The reader builds lookup arrays rather than dictionaries since it only needs
index-to-value mapping, not value-to-index.

## Platform Considerations

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

The writer uses a 512 KB buffer to batch small writes. Each sample record
is typically tens of bytes; writing these individually would incur excessive
syscall overhead. The buffer accumulates data until full, then flushes in
one write() call (or feeds through the compression stream).

## Future Considerations

The format reserves space for future extensions. The 12 reserved bytes in
the header could hold additional metadata. The 16-byte checksum field in
the footer is currently unused. The version field allows incompatible
changes with graceful rejection. New compression types could be added
(compression_type > 1).

Any changes that alter the meaning of existing fields or the parsing logic
should increment the version number to prevent older readers from
misinterpreting new files.
