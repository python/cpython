# Locations table

The `co_linetable` bytes object of code objects contains a compact
representation of the source code positions of instructions, which are
returned by the `co_positions()` iterator.

`co_linetable` consists of a sequence of location entries.
Each entry starts with a byte with the most significant bit set, followed by
zero or more bytes with most significant bit unset.

Each entry contains the following information:

* The number of code units covered by this entry (length).
* The start line
* The end line
* The start column
* The end column

The first byte has the following format:

| Bit 7 | Bits 3-6 | Bits 0-2                   |
|-------|----------|----------------------------|
| 1     | Code     | Length (in code units) - 1 |

The codes are enumerated in the `_PyCodeLocationInfoKind` enum.

## Variable length integer encodings

Integers are often encoded using a variable length integer encoding

### Unsigned integers (varint)

Unsigned integers are encoded in 6 bit chunks, least significant first.
Each chunk but the last has bit 6 set.
For example:

* 63 is encoded as `0x3f`
* 200 is encoded as `0x48`, `0x03` since ``200 = (0x03 << 6) | 0x48``.

The following helper can be used to convert an integer into a `varint`:

```py
def write_varint(s):
    ret = []
    while s >= 64:
        ret.append(((s & 0x3F) | 0x40) & 0x3F)
        s >>= 6
    ret.append(s & 0x3F)
    return bytes(ret)
```

To convert a `varint` into an unsigned integer:

```py
def read_varint(chunks):
    ret = 0
    for chunk in reversed(chunks):
        ret = (ret << 6) | chunk
    return ret
```

### Signed integers (svarint)

Signed integers are encoded by converting them to unsigned integers, using the following function:

```py
def write_svarint(s):
    if s < 0:
        uval = ((-s) << 1) | 1
    else:
        uval = s << 1
    return write_varint(uval)
```

To convert a `svarint` into a signed integer:

```py
def read_svarint(s):
    uval = read_varint(s)
    return -(uval >> 1) if uval & 1 else (uval >> 1)
```

## Location entries

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
* End line: Delta from the start line

### The short forms

Codes 0-9 are the short forms. The short form consists of two bytes,
the second byte holding additional column information. The code is the
start column divided by 8 (and rounded down).

* Start column: `(code*8) + ((second_byte>>4)&7)`
* End column: `start_column + (second_byte&15)`

## Artificial constructions

When constructing artificial `co_linetable` values, only non-None values should
be specified. For instance:

```py
def foo():
    pass
    
co_firstlineno = 42
foo.__code__ = foo.__code__.replace(
    co_firstlineno=co_firstlineno, 
    co_linetable=bytes([
        # RESUME
        (1 << 7)    | (13 << 3)             | (1 - 1),
        # sentinel  # no column info        # number of units - 1
        *write_svarint(2),  # start line delta
        # RETURN_CONST (None)
        (1 << 7)    | (14 << 3)             | (1 - 1),
        # sentinel  # has column info       # number of units - 1
        *write_svarint(5),  # relative start line delta
        *write_varint(12),  # end line delta
        *write_varint(3),   # start column  (starts from 1)
        *write_varint(8),   # end column    (starts from 1)
    ])
)

instructions = list(dis.get_instructions(foo))
assert len(instructions) == 2

assert instructions[0].opname == 'RESUME'
assert instructions[1].opname == 'RETURN_CONST'

ip0, ip1 = instructions[0].positions, instructions[1].positions
assert ip0 == (co_firstlineno + 2, co_firstlineno + 2, None, None)
assert ip1 == (ip0.lineno + 5, ip1.lineno + 12, (3 - 1), (8 - 1))
```

Note that the indexation of the start and end column values are assumed to
start from 1 and are absolute but that `dis.Positions` is using 0-based values
for the column start and end offsets, when available.
