# Code objects

A `CodeObject` is a builtin Python type that represents a compiled executable,
such as a compiled function or class.
It contains a sequence of bytecode instructions along with its associated
metadata: data which is necessary to execute the bytecode instructions (such
as the values of the constants they access) or context information such as
the source code location, which is useful for debuggers and other tools.

Since 3.11, the final field of the `PyCodeObject` C struct is an array
of indeterminate length containing the bytecode, `code->co_code_adaptive`.
(In older versions the code object was a
[`bytes`](https://docs.python.org/dev/library/stdtypes.html#bytes)
object, `code->co_code`; this was changed to save an allocation and to
allow it to be mutated.)

Code objects are typically produced by the bytecode [compiler](compiler.md),
although they are often written to disk by one process and read back in by another.
The disk version of a code object is serialized using the
[marshal](https://docs.python.org/dev/library/marshal.html) protocol.
When a `CodeObject` is created, the function `_PyCode_Quicken()` from
[`Python/specialize.c`](../Python/specialize.c) is called to initialize
the caches of all adaptive instructions. This is required because the
on-disk format is a sequence of bytes, and some of the caches need to be
initialized with 16-bit values.

Code objects are nominally immutable.
Some fields (including `co_code_adaptive` and fields for runtime
information such as `_co_monitoring`) are mutable, but mutable fields are
not included when code objects are hashed or compared.

## Source code locations

Whenever an exception occurs, the interpreter adds a traceback entry to
the exception for the current frame, as well as each frame on the stack that
it unwinds.
The `tb_lineno` field of a traceback entry is (lazily) set to the line
number of the instruction that was executing in the frame at the time of
the exception.
This field is computed from the locations table, `co_linetable`, by the function
[`PyCode_Addr2Line`](https://docs.python.org/dev/c-api/code.html#c.PyCode_Addr2Line).
Despite its name, `co_linetable` includes more than line numbers; it represents
a 4-number source location for every instruction, indicating the precise line
and column at which it begins and ends. This is a significant amount of data,
so a compact format is very important.

Note that traceback objects don't store all this information -- they store the start line
number, for backward compatibility, and the "last instruction" value.
The rest can be computed from the last instruction (`tb_lasti`) with the help of the
locations table. For Python code, there is a convenience method
(`codeobject.co_positions`)[https://docs.python.org/dev/reference/datamodel.html#codeobject.co_positions]
which returns an iterator of `({line}, {endline}, {column}, {endcolumn})` tuples,
one per instruction.
There is also `co_lines()` which returns an iterator of `({start}, {end}, {line})` tuples,
where `{start}` and `{end}` are bytecode offsets.
The latter is described by [`PEP 626`](https://peps.python.org/pep-0626/); it is more
compact, but doesn't return end line numbers or column offsets.
From C code, you need to call
[`PyCode_Addr2Location`](https://docs.python.org/dev/c-api/code.html#c.PyCode_Addr2Location).

As the locations table is only consulted when displaying a traceback and when
tracing (to pass the line number to the tracing function), lookup is not
performance critical.
In order to reduce the overhead during tracing, the mapping from instruction offset to
line number is cached in the ``_co_linearray`` field.

### Format of the locations table

The `co_linetable` bytes object of code objects contains a compact
representation of the source code positions of instructions, which are
returned by the `co_positions()` iterator.

> [!NOTE]
> `co_linetable` is not to be confused with `co_lnotab`.
> For backwards compatibility, `co_lnotab` exposes the format
> as it existed in Python 3.10 and lower: this older format
> stores only the start line for each instruction.
> It is lazily created from `co_linetable` when accessed.
> See [`Objects/lnotab_notes.txt`](../Objects/lnotab_notes.txt) for more details.

`co_linetable` consists of a sequence of location entries.
Each entry starts with a byte with the most significant bit set, followed by
zero or more bytes with the most significant bit unset.

Each entry contains the following information:

* The number of code units covered by this entry (length)
* The start line
* The end line
* The start column
* The end column

The first byte has the following format:

| Bit 7 | Bits 3-6 | Bits 0-2                   |
|-------|----------|----------------------------|
| 1     | Code     | Length (in code units) - 1 |

The codes are enumerated in the `_PyCodeLocationInfoKind` enum.

### Variable-length integer encodings

Integers are often encoded using a variable length integer encoding

#### Unsigned integers (`varint`)

Unsigned integers are encoded in 6-bit chunks, least significant first.
Each chunk but the last has bit 6 set.
For example:

* 63 is encoded as `0x3f`
* 200 is encoded as `0x48`, `0x03` since ``200 = (0x03 << 6) | 0x48``.

The following helper can be used to convert an integer into a `varint`:

```py
def encode_varint(s):
    ret = []
    while s >= 64:
        ret.append(((s & 0x3F) | 0x40) & 0x3F)
        s >>= 6
    ret.append(s & 0x3F)
    return bytes(ret)
```

To convert a `varint` into an unsigned integer:

```py
def decode_varint(chunks):
    ret = 0
    for chunk in reversed(chunks):
        ret = (ret << 6) | chunk
    return ret
```

#### Signed integers (`svarint`)

Signed integers are encoded by converting them to unsigned integers, using the following function:

```py
def svarint_to_varint(s):
    if s < 0:
        return ((-s) << 1) | 1
    else:
        return s << 1
```

To convert a `varint` into a signed integer:

```py
def varint_to_svarint(uval):
    return -(uval >> 1) if uval & 1 else (uval >> 1)
```

### Location entries

The meaning of the codes and the following bytes are as follows:

| Code  | Meaning        | Start line    | End line | Start column  | End column    |
|-------|----------------|---------------|----------|---------------|---------------|
| 0-9   | Short form     | Δ 0           | Δ 0      | See below     | See below     |
| 10-12 | One line form  | Δ (code - 10) | Δ 0      | unsigned byte | unsigned byte |
| 13    | No column info | Δ svarint     | Δ 0      | None          | None          |
| 14    | Long form      | Δ svarint     | Δ varint | varint        | varint        |
| 15    | No location    | None          | None     | None          | None          |

The Δ means the value is encoded as a delta from another value:

* Start line: Delta from the previous start line, or `co_firstlineno` for the first entry.
* End line: Delta from the start line.

### The short forms

Codes 0-9 are the short forms. The short form consists of two bytes,
the second byte holding additional column information. The code is the
start column divided by 8 (and rounded down).

* Start column: `(code*8) + ((second_byte>>4)&7)`
* End column: `start_column + (second_byte&15)`
