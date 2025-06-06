# Locations table

For versions up to 3.10 see ./lnotab_notes.txt

In version 3.11 the `co_linetable` bytes object of code objects contains a compact representation of the positions returned by the `co_positions()` iterator.

The `co_linetable` consists of a sequence of location entries.
Each entry starts with a byte with the most significant bit set, followed by zero or more bytes with most significant bit unset.

Each entry contains the following information:
* The number of code units covered by this entry (length)
* The start line
* The end line
* The start column
* The end column

The first byte has the following format:

Bit 7 | Bits 3-6 | Bits 0-2
 ---- | ---- | ----
 1 | Code | Length (in code units) - 1

The codes are enumerated in the `_PyCodeLocationInfoKind` enum.

## Variable length integer encodings

Integers are often encoded using a variable length integer encoding

### Unsigned integers (varint)

Unsigned integers are encoded in 6 bit chunks, least significant first.
Each chunk but the last has bit 6 set.
For example:

* 63 is encoded as `0x3f`
* 200 is encoded as `0x48`, `0x03`

### Signed integers (svarint)

Signed integers are encoded by converting them to unsigned integers, using the following function:
```Python
def convert(s):
    if s < 0:
        return ((-s)<<1) | 1
    else:
        return (s<<1)
```

## Location entries

The meaning of the codes and the following bytes are as follows:

Code | Meaning | Start line | End line | Start column | End column
 ---- | ---- | ---- | ---- | ---- | ----
 0-9 | Short form | Δ 0 | Δ 0 | See below | See below
 10-12 | One line form | Δ (code - 10) | Δ 0 | unsigned byte | unsigned byte
 13 | No column info | Δ svarint | Δ 0 | None | None
 14   | Long form | Δ svarint | Δ varint | varint | varint
 15   | No location |  None | None | None | None

The Δ means the value is encoded as a delta from another value:
* Start line: Delta from the previous start line, or `co_firstlineno` for the first entry.
* End line: Delta from the start line

### The short forms

Codes 0-9 are the short forms. The short form consists of two bytes, the second byte holding additional column information. The code is the start column divided by 8 (and rounded down).
* Start column: `(code*8) + ((second_byte>>4)&7)`
* End column: `start_column + (second_byte&15)`
