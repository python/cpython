# packTab

Pack static integer tables into compact multi-level lookup tables
to save space.  Generates C or Rust code.

## Installation

```
pip install packtab
```

## Usage

### Command line

```bash
# Generate C lookup code
python -m packTab 1 2 3 4

# Generate Rust lookup code
python -m packTab --rust 1 2 3 4

# Generate Rust with unsafe array access
python -m packTab --rust --unsafe 1 2 3 4

# Analyze compression without generating code
python -m packTab --analyze 1 2 3 4

# Read data from stdin
seq 0 255 | python -m packTab --rust

# Tune compression (1..9 = heuristic; 10 = absolute minimum bytes)
echo "1 2 3 4" | python -m packTab --compression 5

# Force flat / unsplit encoding
echo "1 2 3 4" | python -m packTab --compression 0

# Force absolute minimum table bytes
echo "1 2 3 4" | python -m packTab --compression 10
```

### As a library

```python
from packTab import pack_table, Code, languages

data = [0, 1, 2, 3, 0, 1, 2, 3]
solution = pack_table(data, default=0, compression=1)

code = Code("mytable")
solution.genCode(code, "lookup", language="c", private=False)
code.print_code(language="c")
```

The `pack_table` function accepts:
- A list of integers, or a dict mapping integer keys to values
- `default`: value for missing keys (default `0`)
- `compression`: tuning knob with sentinel endpoints: `0` prefers flat
  encoding, `1..9` use the size/speed heuristic, and `10` minimizes raw
  table bytes (default `1`)
- `mapping`: optional mapping between string values and integers

### Rust with unsafe access

```python
from packTab import pack_table, Code, languageClasses

data = list(range(256)) * 4
solution = pack_table(data, default=0)

lang = languageClasses["rust"](unsafe_array_access=True)
code = Code("mytable")
solution.genCode(code, "lookup", language=lang, private=False)
code.print_code(language=lang)
```

## Examples

### Simple linear data

For sequential data, the packer still generates compact lookup code:

```bash
$ python -m packTab --analyze $(seq 0 255)
Original data: 256 values, range [0..255]
Original storage: 8 bits/value, 256 bytes total

Found Pareto-optimal solutions with compact packed storage
```

### Sparse data

For sparse lookup tables with many repeated values:

```python
from packTab import pack_table, Code

# Sparse Unicode-like table: mostly 0, some special values
data = [0] * 100
data[10] = 5
data[20] = 10
data[50] = 15
data[80] = 20

solution = pack_table(data, default=0)
code = Code("sparse")
solution.genCode(code, "lookup", language="c")
code.print_code(language="c")
```

The packer will use multi-level tables and sub-byte packing to minimize storage.
If all live values sit inside a power-of-two-aligned suffix, it also rebases the
stored span to skip the all-default prefix.
When the exact live span is small enough to inline as a constant, it also tries
an exact rebase to the first non-default index.

### Generated code structure

For small datasets, values are inlined as bit-packed constants:

```c
// Input: [1, 2, 3, 4]
extern inline uint8_t data_get (unsigned u)
{
  return u<4 ? ((228u>>((u)<<1))&3) : 0;
}
```

For larger datasets, generates lookup tables:

```rust
// Input: 256 values with pattern
static data_u8: [u8; 256] = [ ... ];

#[inline]
pub(crate) fn data_get (u: usize) -> u8
{
  if u<256 { data_u8[u] as u8 } else { 0 }
}
```

## How it works

The algorithm builds multi-level lookup tables using dynamic programming
to find optimal split points.  Values that fit in fewer bits get packed
into sub-byte storage (1, 2, or 4 bits per item).  An outer layer applies
arithmetic reductions (GCD factoring, bias subtraction) before splitting.

The solver produces a set of Pareto-optimal solutions trading off table
size against lookup speed, and `pick_solution` selects the best one based
on the `compression` parameter.

## Testing

```bash
pytest
```

## History

I first wrote something like this back in 2001 when I needed it in FriBidi:

  https://github.com/fribidi/fribidi/blob/master/gen.tab/packtab.c

In 2019 I wanted to use that to produce more compact Unicode data tables
for HarfBuzz, but for convenience I wanted to use it from Python.  While
I considered wrapping the C code in a module, it occurred to me that I
can rewrite it in pure Python in a much cleaner way.  That code remains
a stain on my resume in terms of readability (or lack thereof!). :D

This Python version builds on the same ideas, but is different from the
C version in two major ways:

1. Whereas the C version uses backtracking to find best split opportunities,
   I found that the same can be achieved using dynamic-programming.  So the
   Python version implements the DP approach, which is much faster.

2. The C version does not try packing multiple items into a single byte.
   The Python version does.  Ie. if items fit, they might get packed into
   1, 2, or 4 bits per item.

There's also a bunch of other optimizations, which make (eventually, when
complete) the Python version more generic and usable for a wider variety
of data tables.
