# Copyright 2019 Facebook Inc. All Rights Reserved.
# Copyright 2026 Behdad Esfahbod
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Pack a static table of integers into compact lookup tables to save space.

Overview
--------

Given a flat array of integer values (e.g. Unicode character properties
indexed by codepoint), this module compresses it into a set of smaller
arrays plus a lookup function that reconstructs any original value.

The core idea is a multi-level table decomposition.  A single flat
lookup ``table[index]`` is replaced by a chain of smaller lookups:

    level2[level1[level0[index >> K] + (index & mask)]]

Each level splits the index domain by a power of two, so the split
point is always a bit-shift / bit-mask operation (no division).

The algorithm has two layers:

**OuterLayer** — applies arithmetic reductions *before* splitting:
  - **Bias subtraction**: if all values >= B, subtract B and add it
    back in the generated code (``B + lookup(u)``).
  - **GCD factoring**: if all values share a common factor M, divide
    them out and multiply back (``M * lookup(u)``).
  - **Combined bias + GCD**: subtract bias first, then divide by GCD.
  - **Mult bake-in**: if after dividing by M the values still fit in
    the same C integer type, store the undivided values instead and
    skip the runtime multiplication.

  These reductions can shrink the value range enough to fit values in
  fewer bits, enabling tighter sub-byte packing downstream.

**InnerLayer** — the recursive binary splitting engine:
  - Takes the (possibly reduced) data array.
  - Considers every possible split: use the data as-is (1 lookup), or
    split the index in half (shift by 1 bit), grouping adjacent pairs
    into a second-level table.  Pairs of values become entries in a
    mapping; the second level stores mapping indices.
  - Splitting recurses: the second-level table can itself be split,
    producing 3-level, 4-level, etc. solutions.
  - This is done via dynamic programming: each InnerLayer builds all
    its solutions from the solutions of its child layer.

**Sub-byte packing**: when values fit in 1, 2, or 4 bits, multiple
values are packed into each byte of the storage array.  A helper
function extracts elements with shift-and-mask.

**Inlining**: when the packed data fits in <= 64 bits (8 bytes), the
array is replaced by a single integer constant with inline bit
extraction: ``(CONSTANT >> (index << shift)) & mask``.

**Cost model**: each solution tracks ``nLookups`` (number of table
indirections), ``nExtraOps`` (bias/mult/sub-byte ops), and ``cost``
(total bytes of table storage).  ``fullCost`` combines them into a
single metric.  Dominated solutions (worse on both axes) are pruned,
keeping only the Pareto frontier.  The ``compression`` parameter
controls the size-vs-speed tradeoff when picking a final solution.

Code generation
---------------

The ``Code`` class accumulates arrays and functions as they are
generated.  ``genCode()`` on a solution recursively generates code
for all levels, registering arrays and helper functions in the Code
object.  Finally, ``Code.print_code()`` emits the accumulated
declarations in the target language (C or Rust).

The ``Language`` class hierarchy abstracts syntax differences between
C and Rust: type names, array declarations, function signatures,
linkage keywords, index expressions, and so on.
"""

import sys
import collections
from math import ceil, log2
from functools import partial
from typing import Union, List, Dict, Optional, Any, Tuple, TextIO


__all__ = [
    "Code",
    "pack_table",
    "pick_solution",
    "languages",
    "languageClasses",
    "binaryBitsFor",
]

__version__ = "1.5.0"


class AutoMapping(collections.defaultdict):
    """Bidirectional mapping that auto-assigns integer IDs to new keys.

    Used during InnerLayer splitting: pairs of values like (3, 7) are
    mapped to compact integer IDs (0, 1, 2, ...) for the next level.
    The mapping is bidirectional — ``m[(3,7)]`` returns the ID, and
    ``m[id]`` returns ``(3, 7)`` — so that code generation can
    reconstruct the expansion tables.
    """

    _next = 0

    def __missing__(self, key):
        assert not isinstance(key, int)
        v = self._next
        self._next = self._next + 1
        self[key] = v
        self[v] = key
        return v


def binaryBitsFor(minV, maxV):
    """Returns the smallest power-of-two bit width that can store values
    in [minV, maxV].

    The returned widths — 0, 1, 2, 4, 8, 16, 32, 64 — correspond to
    sub-byte packing granularities (0/1/2/4 bits per item packed into
    bytes) and standard C/Rust integer types (u8, u16, u32, u64).
    Only power-of-two widths are used because they allow bit-shift
    indexing without division.

    >>> binaryBitsFor(0, 0)
    0
    >>> binaryBitsFor(0, 1)
    1
    >>> binaryBitsFor(0, 6)
    4
    >>> binaryBitsFor(0, 14)
    4
    >>> binaryBitsFor(0, 15)
    4
    >>> binaryBitsFor(0, 16)
    8
    >>> binaryBitsFor(0, 100)
    8
    """

    if not isinstance(minV, int) or not isinstance(maxV, int):
        return 8

    if minV > maxV:
        raise ValueError("minV (%d) > maxV (%d)" % (minV, maxV))

    if 0 <= minV and maxV <= 0:
        return 0
    if 0 <= minV and maxV <= 1:
        return 1
    if 0 <= minV and maxV <= 3:
        return 2
    if 0 <= minV and maxV <= 15:
        return 4

    if 0 <= minV and maxV <= 255:
        return 8
    if -128 <= minV and maxV <= 127:
        return 8

    if 0 <= minV and maxV <= 65535:
        return 16
    if -32768 <= minV and maxV <= 32767:
        return 16

    if 0 <= minV and maxV <= 4294967295:
        return 32
    if -2147483648 <= minV and maxV <= 2147483647:
        return 32

    if 0 <= minV and maxV <= 18446744073709551615:
        return 64
    if -9223372036854775808 <= minV and maxV <= 9223372036854775807:
        return 64

    raise ValueError("values out of range: [%d, %d]" % (minV, maxV))


class Language:
    """Base class for target-language code generation backends.

    Subclasses (LanguageC, LanguageRust) override syntax-specific methods:
    type names, declarations, index expressions, linkage keywords, etc.

    Instances may be configured (e.g. ``unsafe_array_access=True`` for
    Rust's ``get_unchecked``).  Default instances live in the ``languages``
    dict; ``languageClasses`` holds the classes for custom instantiation.
    """

    def __init__(self, *, unsafe_array_access=False):
        self.unsafe_array_access = unsafe_array_access

    def print_array(self, name, array, *, print=print, private=True):
        linkage = self.private_array_linkage if private else self.public_array_linkage
        decl = self.declare_array(linkage, array.typ, name, len(array.values))
        print(decl, "=")
        print(self.array_start)
        w = max((len(str(v)) for v in array.values), default=1)
        n = 1 << int(round(log2(78 / (w + 1))))
        if (w + 2) * n <= 78:
            w += 1
        for i in range(0, len(array.values), n):
            line = array.values[i : i + n]
            print("  " + "".join("%*s," % (w, v) for v in line))
        print(self.array_end)

    def print_function(self, name, function, *, print=print):
        linkage = (
            self.private_function_linkage
            if function.private
            else self.public_function_linkage
        )
        decl = self.declare_function(linkage, function.retType, name, function.args)
        print(decl)
        print(self.function_start)
        if function.comment:
            print("  /* %s */" % function.comment)
        print("  %s" % self.return_stmt(function.body))
        print(self.function_end)

    def array_index(self, name, index):
        return "%s[%s]" % (name, index)

    def usize_literal(self, value):
        if value == "":
            return ""
        return "%s%s" % (value, self.usize_suffix)

    def wrapping_add(self, a, b):
        return "%s+%s" % (a, b)

    def wrapping_sub(self, a, b):
        return "%s-%s" % (a, b)

    def uint_literal(self, value, typ):
        return str(value)


class LanguageC(Language):
    name = "c"
    private_array_linkage = "static const"
    public_array_linkage = "extern const"
    private_function_linkage = "static inline"
    public_function_linkage = "extern inline"
    array_start = "{"
    array_end = "};"
    function_start = "{"
    function_end = "}"
    u8 = "uint8_t"
    usize = "unsigned"
    usize_suffix = "u"

    def print_preamble(self, *, print=print):
        print("#include <stdint.h>")
        print()

    def cast(self, typ, expr):
        return "(%s)(%s)" % (typ, expr)

    def borrow(self, name):
        return "%s" % name

    def slice(self, name, start):
        return "%s+%s" % (name, start)

    def tertiary(self, cond, trueExpr, falseExpr):
        return "%s ? %s : %s" % (cond, trueExpr, falseExpr)

    def declare_array(self, linkage, typ, name, size):
        if linkage:
            linkage += " "
        return "%s%s %s[%d]" % (linkage, typ, name, size)

    def declare_function(self, linkage, retType, name, args):
        if linkage:
            linkage += " "
        args = [(t if t[-1] != "*" else "const %s" % t, n) for t, n in args]
        args = ", ".join(" ".join(p) for p in args)
        return "%s%s %s (%s)" % (linkage, retType, name, args)

    def type_name(self, typ):
        assert typ[0] in "iu"
        signed = "" if typ[0] == "i" else "u"
        size = typeWidth(typ)
        return "%sint%s_t" % (signed, size)

    def type_for(self, minV, maxV):
        if not isinstance(minV, int) or not isinstance(maxV, int):
            return "uint8_t"

        if 0 <= minV and maxV <= 255:
            return "uint8_t"
        if -128 <= minV and maxV <= 127:
            return "int8_t"

        if 0 <= minV and maxV <= 65535:
            return "uint16_t"
        if -32768 <= minV and maxV <= 32767:
            return "int16_t"

        if 0 <= minV and maxV <= 4294967295:
            return "uint32_t"
        if -2147483648 <= minV and maxV <= 2147483647:
            return "int32_t"

        if 0 <= minV and maxV <= 18446744073709551615:
            return "uint64_t"
        if -9223372036854775808 <= minV and maxV <= 9223372036854775807:
            return "int64_t"

        raise ValueError("values out of range: [%d, %d]" % (minV, maxV))

    def uint_literal(self, value, typ):
        if "64" in typ:
            return "%dULL" % value
        return "%du" % value

    def as_usize(self, expr):
        return expr

    def return_stmt(self, expr):
        return "return %s;" % expr


class LanguageRust(Language):
    name = "rust"
    private_array_linkage = "static"
    public_array_linkage = "pub(crate) static"
    private_function_linkage = ""
    public_function_linkage = "pub(crate)"
    array_start = "["
    array_end = "];"
    function_start = "{"
    function_end = "}"
    u8 = "u8"
    usize = "usize"
    usize_suffix = "usize"

    def print_preamble(self, *, print=print):
        pass

    def cast(self, typ, expr):
        return "(%s) as %s" % (expr, typ)

    def borrow(self, name):
        return "&%s" % name

    def slice(self, name, start):
        return "&%s[%s..]" % (name, start)

    def tertiary(self, cond, trueExpr, falseExpr):
        return "if %s { %s } else { %s }" % (cond, trueExpr, falseExpr)

    def declare_array(self, linkage, typ, name, size):
        if linkage:
            linkage += " "
        typ = "%s" % typ
        return "%s%s: [%s; %d]" % (linkage, name, typ, size)

    def declare_function(self, linkage, retType, name, args):
        if linkage:
            linkage += " "
        retType = "%s" % retType
        args = [(t if t[-1] != "*" else "&[%s]" % t[:-1], n) for t, n in args]
        args = ", ".join("%s: %s" % (n, t) for t, n in args)
        return "%sfn %s (%s) -> %s" % (linkage, name, args, retType)

    def print_function(self, name, function, *, print=print):
        # Add #[inline] attribute for better optimization hints
        if function.inline_always:
            print("#[inline(always)]")
        else:
            print("#[inline]")
        # Call parent implementation
        super().print_function(name, function, print=print)

    def type_name(self, typ):
        assert typ[0] in "iu"
        signed = typ[0]
        size = typeWidth(typ)
        return "%s%s" % (signed, size)

    def type_for(self, minV, maxV):
        if not isinstance(minV, int) or not isinstance(maxV, int):
            return "u8"

        if 0 <= minV and maxV <= 255:
            return "u8"
        if -128 <= minV and maxV <= 127:
            return "i8"

        if 0 <= minV and maxV <= 65535:
            return "u16"
        if -32768 <= minV and maxV <= 32767:
            return "i16"

        if 0 <= minV and maxV <= 4294967295:
            return "u32"
        if -2147483648 <= minV and maxV <= 2147483647:
            return "i32"

        if 0 <= minV and maxV <= 18446744073709551615:
            return "u64"
        if -9223372036854775808 <= minV and maxV <= 9223372036854775807:
            return "i64"

        raise ValueError("values out of range: [%d, %d]" % (minV, maxV))

    def as_usize(self, expr):
        if not expr:
            return ""
        try:
            int(expr)
            return "%susize" % expr
        except ValueError:
            # Assume expr is a variable or expression that evaluates to an integer.
            # Rust requires explicit casting to usize.
            if expr.startswith("(") and expr.endswith(")"):
                return "%s as usize" % expr
            else:
                return "(%s) as usize" % expr

    def array_index(self, name, index):
        if self.unsafe_array_access:
            return "unsafe { *(%s.get_unchecked(%s)) }" % (name, index)
        return "%s[%s]" % (name, index)

    def uint_literal(self, value, typ):
        return "%d%s" % (value, typ)

    def wrapping_add(self, a, b):
        return "(%s).wrapping_add(%s)" % (a, b)

    def wrapping_sub(self, a, b):
        return "(%s).wrapping_sub(%s)" % (a, b)

    def return_stmt(self, expr):
        return expr


languageClasses = {
    "c": LanguageC,
    "rust": LanguageRust,
}

languages = {k: v() for k, v in languageClasses.items()}


class Array:
    """A named typed array accumulating values for code generation."""

    def __init__(self, typ):
        self.typ = typ
        self.values = []

    def extend(self, values):
        """Append values to array, with overlap optimization.

        If the last element of the existing array equals the first element
        of the new values, we find the maximum overlap where all overlapping
        elements have that same value, reducing total array size.
        """
        if not values:
            return len(self.values)

        start = len(self.values)

        # Overlap optimization: if last element of existing array equals
        # first element of new values, find maximum overlap of that value
        if self.values and self.values[-1] == values[0]:
            overlap_value = self.values[-1]

            # Count trailing run of overlap_value in existing array
            trailing_count = 0
            for i in range(len(self.values) - 1, -1, -1):
                if self.values[i] == overlap_value:
                    trailing_count += 1
                else:
                    break

            # Count leading run of overlap_value in new values
            leading_count = 0
            for val in values:
                if val == overlap_value:
                    leading_count += 1
                else:
                    break

            # Overlap by the minimum of the two runs
            overlap = min(trailing_count, leading_count)
            if overlap > 0:
                start -= overlap
                values = values[overlap:]

        self.values.extend(values)
        return start


class Function:
    """A generated inline function with a single-expression body."""

    def __init__(
        self,
        retType,
        args,
        body,
        *,
        private=True,
        inline_always=False,
        comment=None,
    ):
        self.retType = retType
        self.args = args
        self.body = body
        self.private = private
        self.inline_always = inline_always
        self.comment = comment


class Code:
    """Accumulator for generated arrays and functions.

    During ``genCode()``, each solution level registers its data array
    (via ``addArray``) and helper functions (via ``addFunction``) here.
    Multiple solutions sharing the same sub-byte accessor or the same
    array name will deduplicate automatically.

    Arrays with the same name accumulate values contiguously; the
    returned ``start`` offset lets each level index into its portion.

    Call ``print_code()`` to emit all accumulated declarations in the
    target language.
    """

    def __init__(self, namespace: str = "") -> None:
        self.namespace = namespace
        self.functions = collections.OrderedDict()
        self.arrays = collections.OrderedDict()

    def nameFor(self, name: str) -> str:
        return "%s_%s" % (self.namespace, name)

    def addFunction(
        self,
        retType: str,
        name: str,
        args: Tuple[Tuple[str, str], ...],
        body: str,
        *,
        private: bool = True,
        inline_always: bool = False,
        comment: Optional[str] = None,
    ) -> str:
        name = self.nameFor(name)
        if name in self.functions:
            assert self.functions[name].retType == retType
            assert self.functions[name].args == args
            assert self.functions[name].body == body
            assert self.functions[name].private == private
            assert self.functions[name].inline_always == inline_always
            assert self.functions[name].comment == comment
        else:
            self.functions[name] = Function(
                retType,
                args,
                body,
                private=private,
                inline_always=inline_always,
                comment=comment,
            )
        return name

    def addArray(self, typ: str, name: str, values: List[Any]) -> Tuple[str, int]:
        name = self.nameFor(name)
        array = self.arrays.get(name)
        if array is None:
            array = self.arrays[name] = Array(typ)
        start = array.extend(values)
        return name, start

    def print_code(
        self,
        *,
        file: TextIO = sys.stdout,
        private: bool = True,
        indent: Union[int, str] = 0,
        language: Union[str, "Language"] = "c",
    ) -> None:
        if isinstance(indent, int):
            indent *= " "
        printn = partial(print, file=file, sep="")
        println = partial(printn, indent)

        if isinstance(language, str):
            language = languages[language]

        language.print_preamble(print=println)

        for name, array in self.arrays.items():
            language.print_array(name, array, print=println, private=private)

        if self.arrays and self.functions:
            printn()

        for name, function in self.functions.items():
            language.print_function(name, function, print=println)

    def print_c(self, file=sys.stdout, indent=0):
        self.print_code(file=file, indent=indent, language="c")

    def print_h(self, file=sys.stdout, linkage="", indent=0):
        if linkage:
            linkage += " "
        if isinstance(indent, int):
            indent *= " "
        printn = partial(print, file=file, sep="")
        println = partial(printn, indent)

        for name, function in self.functions.items():
            link = (linkage if function.linkage is None else function.linkage) + " "
            args = ", ".join(" ".join(p) for p in function.args)
            println("%s%s %s (%s);" % (link, function.retType, name, args))


# Cost model constants.  These tune the tradeoff between table size
# (bytes of storage) and lookup speed (number of operations).
#
# bytesPerOp:        how many bytes of storage one extra op is "worth".
#                    Higher values bias toward fewer ops (faster lookup).
# lookupOps:         estimated ops per table indirection (array index,
#                    bounds-check overhead, cache miss potential).
# subByteAccessOps:  extra ops for sub-byte extraction (shift + mask
#                    to unpack 1/2/4-bit values from a byte).

bytesPerOp = 4
lookupOps = 4
subByteAccessOps = 4


class Solution:
    """A single point on the Pareto frontier of size vs. speed.

    Each solution records:
      - ``layer``:     the Layer that produced this solution.
      - ``next``:      the child solution for the next level (or None).
      - ``nLookups``:  total table indirections (determines speed).
      - ``nExtraOps``: additional operations (bias add, mult, sub-byte).
      - ``cost``:      total bytes of array storage.

    ``fullCost`` combines storage bytes with an operation penalty,
    giving a single comparable metric.  During pruning, a solution
    that is worse on *both* nLookups and fullCost is discarded.
    """

    def __init__(self, layer, next, nLookups, nExtraOps, cost):
        self.layer = layer
        self.next = next
        self.nLookups = nLookups
        self.nExtraOps = nExtraOps
        self.cost = cost

    @property
    def fullCost(self):
        return self.cost + (self.nLookups * lookupOps + self.nExtraOps) * bytesPerOp

    def __repr__(self):
        return "%s%s" % (
            self.__class__.__name__,
            (self.nLookups, self.nExtraOps, self.cost),
        )


def typeWidth(typ):
    """
    >>> typeWidth('int8_t')
    8
    >>> typeWidth('uint32_t')
    32
    >>> typeWidth('i8')
    8
    >>> typeWidth('u32')
    32
    """
    return int("".join([c for c in typ if c.isdigit()]))


def typeAbbr(typ):
    """
    >>> typeAbbr('int8_t')
    'i8'
    >>> typeAbbr('uint32_t')
    'u32'
    """
    return typ[0] + str(typeWidth(typ))


def fastType(typ):
    """
    >>> fastType('int8_t')
    'int8_t'
    >>> fastType('uint32_t')
    'uint32_t'
    >>> fastType('i8')
    'i8'
    >>> fastType('u32')
    'u32'
    """
    return typ
    # return typ.replace("int", "int_fast")


class InnerSolution(Solution):
    """Solution produced by InnerLayer — one specific split depth.

    ``bits`` is the number of index bits consumed at *this* level.
    If bits == 0, this is a leaf (single flat array).  If bits == K,
    the index is split: low K bits select within a group, high bits
    are delegated to ``self.next``.
    """

    def __init__(self, layer, next, nLookups, nExtraOps, cost, bits=0):
        Solution.__init__(self, layer, next, nLookups, nExtraOps, cost)
        self.bits = bits

    def shape_terms(self):
        terms = []
        if self.next:
            terms.extend(self.next.shape_terms())
        if self.bits:
            terms.append("2^%d" % self.bits)
        else:
            terms.append("2^%d" % self.layer.unitBits)
        return terms

    def genCode(self, code, name=None, var="u", language="c", data_multiplier=1):
        """Generate lookup code for this solution level.

        Recursively generates code for child levels (``self.next``),
        then emits the data array and index expression for this level.

        The generated expression composes as:

            outer_level(  inner_level(  leaf_array[index]  )  )

        Each level either:
          - Inlines the data as an integer constant (if <= 64 bits),
          - Emits a direct array index (if unitBits >= 8), or
          - Emits a sub-byte accessor function call (if unitBits < 8).

        Returns ``(retType, expr)`` where ``expr`` is the C/Rust
        expression that computes the lookup for this level.
        """
        inputVar = var
        if name:
            var = "u"
        expr = var

        if isinstance(language, str):
            language = languages[language]

        typ = language.type_for(self.layer.minV, self.layer.maxV)
        retType = fastType(typ)
        unitBits = self.layer.unitBits
        if not unitBits:
            # All values are identical (maxV == 0); return the constant.
            expr = str(self.layer.data[0])
            return (retType, expr)

        shift = self.bits
        mask = (1 << shift) - 1

        # Check if we can bake in the shift: pre-multiply the child's
        # stored values by (1 << shift) so the runtime shift is eliminated.
        # This works when the pre-shifted max still fits in the same
        # sub-byte bucket (e.g., maxV=31 shifted by 3 → 248, still 8-bit).
        bake_shift = False
        if self.next and shift > 0:
            child_maxV = self.next.layer.maxV
            child_unitBits = self.next.layer.unitBits
            if child_maxV * (1 << shift) < (1 << child_unitBits):
                bake_shift = True

        # Recurse into the child level (higher bits of the index).
        if self.next:
            child_multiplier = (1 << shift) if bake_shift else 1
            (_, expr) = self.next.genCode(
                code,
                None,
                "((%s)>>%d)" % (var, shift),
                language=language,
                data_multiplier=child_multiplier,
            )

        # Reconstruct the flat data for this level by expanding the
        # chain of split mappings back into a single array.
        #
        # If bits == 0 (leaf), the data is just the layer's own data.
        # If bits > 0, we walk the split chain and expand each mapping
        # entry back into its constituent pair, recursively, producing
        # the full 2^bits-wide expansion table.

        layers = []
        layer = self.layer
        bits = self.bits
        while bits:
            layers.append(layer)
            layer = layer.next
            bits -= 1

        data = []
        if not layers:
            data.extend(layer.data)
        else:
            assert layer.minV == 0
            for d in range(layer.maxV + 1):
                _expand(d, layers, len(layers) - 1, data)

        # Apply data_multiplier from parent: pre-scale values so the
        # parent can skip its shift operation.
        if data_multiplier > 1:
            data = [v * data_multiplier for v in data]

        # Pack sub-byte values: multiple 1/2/4-bit items per byte.
        data = _combine(data, self.layer.unitBits)

        # If the packed data fits in a single integer (<= 64 bits),
        # inline it as a constant instead of emitting an array.
        # Non-integer data (string identifiers) and negative values cannot
        # be inlined (negative values would produce invalid unsigned literals).
        can_inline = len(data) * 8 <= 64 and all(
            isinstance(v, int) and v >= 0 for v in data
        )

        if not can_inline:
            arrName, start = code.addArray(typ, typeAbbr(typ), data)

        # Build the index expression.  For a multi-level solution:
        #   index = (child_expr << shift) | (var & mask)
        # If shift was baked into child's data:
        #   index = child_expr + (var & mask)
        # For a single-level solution (shift == 0): index = var.

        if expr == "0":
            index0 = ""
        elif shift == 0 or bake_shift:
            index0 = language.as_usize(expr)
        else:
            index0 = "((%s)<<%d)" % (language.as_usize(expr), shift)
        index1 = "((%s)&%s)" % (var, mask) if mask else ""
        index = (
            language.as_usize(index0)
            + ("+" if index0 and index1 else "")
            + language.as_usize(index1)
        )

        # Emit the lookup expression, choosing between three strategies:
        if can_inline:
            # Strategy 1: inline constant with bit extraction.
            # Pack all bytes into one integer and extract with shift+mask.
            #   (CONSTANT >> (index << log2(unitBits))) & ((1 << unitBits) - 1)
            packed = 0
            for i, b in enumerate(data):
                packed |= b << (i * 8)
            const_typ = language.type_for(0, (1 << (len(data) * 8)) - 1)
            lit = language.uint_literal(packed, const_typ)
            elementMask = (1 << unitBits) - 1
            shift2 = int(round(log2(unitBits))) if unitBits > 1 else 0
            idx = language.as_usize(index)
            if shift2:
                expr = "((%s>>((%s)<<%d))&%d)" % (lit, idx, shift2, elementMask)
            else:
                expr = "((%s>>(%s))&%d)" % (lit, idx, elementMask)
        elif unitBits >= 8:
            # Strategy 2: direct array indexing (one value per element).
            start = language.usize_literal(start) if start else None
            if start:
                index = "%s+%s" % (start, language.as_usize(index))
            expr = language.array_index(arrName, index)
        else:
            # Strategy 3: sub-byte accessor function.
            # Values are packed N-per-byte; a helper function extracts
            # element i from byte array a:
            #   (a[i >> log2(8/unitBits)] >> ((i & mask) << log2(unitBits))) & valueMask
            start = language.usize_literal(start) if start else None
            shift1 = int(round(log2(8 // unitBits)))
            mask1 = (8 // unitBits) - 1
            shift2 = int(round(log2(unitBits)))
            mask2 = (1 << unitBits) - 1
            funcBody = "(%s>>((i&%s)<<%d))&%s" % (
                language.array_index("a", "i>>%s" % shift1),
                mask1,
                shift2,
                mask2,
            )
            funcName = code.addFunction(
                language.u8,
                "b%s" % unitBits,
                ((language.u8 + "*", "a"), (language.usize, "i")),
                funcBody,
                inline_always=True,
            )
            if start:
                sliced_array = language.slice(arrName, start)
            else:
                sliced_array = language.borrow(arrName)
            expr = "%s(%s,%s)" % (funcName, sliced_array, index)

        # If this is the top-level named function, wrap the expression.
        if name:
            funcName = code.addFunction(retType, name, ((language.usize, "u"),), expr)
            expr = "%s(%s)" % (funcName, inputVar)

        return (retType, expr)


def _expand(v, stack, i, out):
    """Recursively expand a mapping index back into flat data values.

    During splitting, pairs of values are mapped to single indices via
    AutoMapping.  This function reverses that: given an index ``v`` at
    level ``i`` of the ``stack``, it looks up the pair, and recursively
    expands each half, appending leaf values to ``out``.
    """
    if i < 0:
        out.append(v)
        return
    v = stack[i].mapping[v]
    i -= 1
    _expand(v[0], stack, i, out)
    _expand(v[1], stack, i, out)


def _combine(data, bits):
    """Pack sub-byte values into bytes.

    If ``bits`` (the per-element width) is 1, 2, or 4, multiple values
    are combined into each byte.  For example, with 2-bit values,
    four values [a, b, c, d] become one byte: d<<6 | c<<4 | b<<2 | a.

    The cascading ``if`` statements handle each packing stage:
    1-bit → 2-bit → 4-bit → 8-bit (byte).
    """
    if bits <= 1:
        data = _combine2(data, lambda a, b: (b << 1) | a)
    if bits <= 2:
        data = _combine2(data, lambda a, b: (b << 2) | a)
    if bits <= 4:
        data = _combine2(data, lambda a, b: (b << 4) | a)
    return data


def _combine2(data, f):
    """Pairwise reduce: combine adjacent elements using function ``f``."""
    data2 = []
    it = iter(data)
    for first in it:
        data2.append(f(first, next(it, 0)))
    return data2


class Layer:
    """Base for InnerLayer and OuterLayer."""

    def __init__(self, data):
        self.data = data
        self.next = None
        self.solutions = []


def _aligned_base_for_live_range(data, default):
    """Return the aligned base for the smallest dyadic interval covering live data.

    The live range is the span from the first non-default value to the last
    non-default value. The returned base is the start of the smallest power-of-two
    aligned interval containing that span. If all values equal the default, the
    base is zero.
    """
    first = next((i for i, v in enumerate(data) if v != default), None)
    if first is None:
        return 0

    last = len(data) - 1 - next(i for i, v in enumerate(reversed(data)) if v != default)
    differing_bits = first ^ last
    if differing_bits == 0:
        return first

    mask = (1 << differing_bits.bit_length()) - 1
    return first & ~mask


def _first_non_default_index(data, default):
    return next((i for i, v in enumerate(data) if v != default), None)


def _prune_pareto_solutions(solutions):
    """Remove solutions dominated on (nLookups, fullCost)."""
    solutions = sorted(solutions, key=lambda s: (s.nLookups, s.fullCost))
    kept = []
    best_cost = float("inf")
    for s in solutions:
        if s.fullCost < best_cost:
            kept.append(s)
            best_cost = s.fullCost
    return kept


class InnerLayer(Layer):
    """Recursive binary-split engine producing multi-level lookup solutions.

    Given a data array, InnerLayer considers:
      1. A flat solution (one lookup, cost = data size in bytes).
      2. Splitting: pair adjacent elements, map each unique pair to
         an integer ID (via AutoMapping), and recurse on the half-sized
         array of IDs.  Each split adds one level of indirection.

    The recursion builds a chain of InnerLayers linked by ``.next``.
    At each depth, solutions from the child are combined with the
    expansion cost at this level to form new candidate solutions.

    All non-dominated solutions are kept (Pareto frontier on nLookups
    vs. fullCost), so the caller can choose the desired tradeoff.
    """

    def __init__(self, data):
        Layer.__init__(self, data)

        self.maxV = max(data)
        self.minV = min(data)
        # Note: minV need not be zero even when unitBits < 8 (sub-byte packing).
        # OuterLayer ensures data is non-negative before passing to InnerLayer,
        # and binaryBitsFor() correctly handles ranges like [1..4] → 4 bits.
        self.unitBits = binaryBitsFor(self.minV, self.maxV)
        self.extraOps = subByteAccessOps if self.unitBits < 8 else 0
        self.bytes = ceil(self.unitBits * len(self.data) / 8)

        if self.maxV == 0:
            solution = InnerSolution(self, None, 0, 0, 0)
            self.solutions.append(solution)
            return

        self.split()

        solution = InnerSolution(self, None, 1, self.extraOps, self.bytes)
        self.solutions.append(solution)

        bits = 1
        layer = self.next
        while layer is not None:
            extraCost = ceil((layer.maxV + 1) * (1 << bits) * self.unitBits / 8)

            for s in layer.solutions:
                nLookups = s.nLookups + 1
                nExtraOps = s.nExtraOps + self.extraOps
                cost = s.cost + extraCost
                solution = InnerSolution(self, s, nLookups, nExtraOps, cost, bits)
                self.solutions.append(solution)

            layer = layer.next
            bits += 1

        self.prune_solutions()

    def split(self):
        """Split data by pairing adjacent elements.

        Pad to even length, then map each pair (a, b) to a unique
        integer ID.  The resulting half-length array of IDs becomes
        the data for the next (child) InnerLayer.

        Pairs are assigned IDs based on frequency (most common pairs
        get lower IDs) for better cache locality. Position is used as
        a tiebreaker for pairs with equal frequency.

        When padding is needed, the padded element is never accessed
        (guaranteed unreachable), so we choose a padding value that
        creates the most common pair, maximizing compression.
        """
        if len(self.data) & 1:
            # Smart padding: choose value that creates most common pair.
            # The padded position is never accessed, so this is safe.
            last_val = self.data[-1]
            padding = self._choose_optimal_padding(last_val)
            self.data.append(padding)

        # Collect pairs with frequencies and first occurrence positions
        from collections import Counter
        pairs = [(self.data[i], self.data[i + 1]) for i in range(0, len(self.data), 2)]
        pair_freq = Counter(pairs)
        first_occurrence = {}
        for i, pair in enumerate(pairs):
            if pair not in first_occurrence:
                first_occurrence[pair] = i

        # Sort unique pairs by frequency (descending), then position (ascending)
        unique_pairs = sorted(
            pair_freq.keys(), key=lambda p: (-pair_freq[p], first_occurrence[p])
        )

        # Create mapping with IDs assigned in sorted order
        mapping = self.mapping = AutoMapping()
        for pair in unique_pairs:
            mapping[pair]  # Assigns next sequential ID

        # Apply mapping to create child layer data
        data2 = _combine2(self.data, lambda a, b: mapping[(a, b)])

        self.next = InnerLayer(data2)

    def _choose_optimal_padding(self, last_val):
        """Choose padding value that maximizes compression.

        Returns a value V such that the pair (last_val, V) is the most
        common existing pair starting with last_val. If no such pair
        exists, returns the most frequent value overall.
        """
        from collections import Counter

        # Count existing pairs to find common patterns
        pairs = [(self.data[i], self.data[i + 1])
                 for i in range(0, len(self.data) - 1, 2)]
        pair_freq = Counter(pairs)

        # Find which value V makes (last_val, V) most frequent
        candidates = {b: freq for (a, b), freq in pair_freq.items() if a == last_val}

        if candidates:
            # Return V that makes (last_val, V) most common
            return max(candidates, key=candidates.get)

        # No pairs with last_val - use most frequent value overall
        # to maximize future duplicate detection opportunities
        value_freq = Counter(self.data)
        return value_freq.most_common(1)[0][0]

    def prune_solutions(self):
        """Remove dominated solutions (Pareto pruning).

        A solution B is dominated by A if A has <= lookups AND
        <= fullCost.  Only non-dominated solutions survive.

        O(N log N): sort by (nLookups, fullCost) ascending, then scan
        once keeping only solutions whose fullCost is strictly less
        than all previous (which have <= nLookups)."""

        self.solutions = _prune_pareto_solutions(self.solutions)


class OuterSolution(Solution):
    """Solution wrapping an InnerSolution with OuterLayer's arithmetic."""

    def __init__(self, layer, next, nLookups, nExtraOps, cost):
        Solution.__init__(self, layer, next, nLookups, nExtraOps, cost)
        self.palette = False

    def shape_comment(self):
        terms = []
        if self.layer.base:
            terms.append("base=%d" % self.layer.base)
        terms.append("[" + ",".join(self.next.shape_terms()) + "]")
        if self.layer.mult != 1:
            terms.append("*%d" % self.layer.mult)
        if self.layer.bias > 0:
            terms.append("+%d" % self.layer.bias)
        elif self.layer.bias < 0:
            terms.append("-%d" % (-self.layer.bias))
        return "packtab: " + " ".join(terms)

    def genCode(self, code, name=None, var="u", language="c", private=True):
        inputVar = var
        if name:
            var = "u"

        if isinstance(language, str):
            language = languages[language]

        typ = language.type_for(self.layer.minV, self.layer.maxV)
        retType = fastType(typ)
        lookup_var = var
        if self.layer.base:
            lookup_var = language.wrapping_sub(
                var, language.usize_literal(self.layer.base)
            )
        expr = lookup_var
        if self.next:
            (_, expr) = self.next.genCode(code, None, lookup_var, language=language)
            # Cast inner result to return type so that mult/bias arithmetic
            # happens at the correct width.  Rust requires this because it
            # has no implicit integer widening or narrowing.
            expr = language.cast(retType, expr)

        if self.layer.mult != 1:
            expr = "%d*%s" % (self.layer.mult, expr)
        if self.layer.bias > 0:
            expr = "%d+%s" % (self.layer.bias, expr)
        elif self.layer.bias < 0:
            expr = "%s-%d" % (expr, -self.layer.bias)

        span = language.usize_literal(len(self.layer.data))
        expr = language.tertiary("%s<%s" % (lookup_var, span), expr, self.layer.default)

        if name:
            funcName = code.addFunction(
                retType,
                name,
                ((language.usize, "u"),),
                expr,
                private=private,
                comment=self.shape_comment(),
            )
            expr = "%s(%s)" % (funcName, inputVar)

        return (retType, expr)


class PaletteOuterSolution(Solution):
    """Solution using palette encoding: indices -> palette of unique values.

    Like indexed color in graphics, stores an index array plus a palette
    table of unique values. The indices array is passed through InnerLayer
    and can still be split for further compression. This helps when there
    are few unique values (especially with outliers that would otherwise
    force wide storage for the entire array).
    """

    def __init__(self, layer, next, nLookups, nExtraOps, cost, palette, value_to_index):
        Solution.__init__(self, layer, next, nLookups, nExtraOps, cost)
        self.palette = palette  # List of unique values
        self.value_to_index = value_to_index  # Dict mapping value -> palette index

    def shape_comment(self):
        terms = []
        if self.layer.base:
            terms.append("base=%d" % self.layer.base)
        terms.append("[" + ",".join(self.next.shape_terms()) + "]")
        terms.append("palette[%d]" % len(self.palette))
        if self.layer.mult != 1:
            terms.append("*%d" % self.layer.mult)
        if self.layer.bias > 0:
            terms.append("+%d" % self.layer.bias)
        elif self.layer.bias < 0:
            terms.append("-%d" % (-self.layer.bias))
        return "packtab: " + " ".join(terms)

    def genCode(self, code, name=None, var="u", language="c", private=True):
        inputVar = var
        if name:
            var = "u"

        if isinstance(language, str):
            language = languages[language]

        # Determine return type based on original value range
        typ = language.type_for(self.layer.minV, self.layer.maxV)
        retType = fastType(typ)

        # Generate palette array containing unique values
        palette_typ = language.type_for(min(self.palette), max(self.palette))
        palette_name, _ = code.addArray(palette_typ, "palette", self.palette)
        lookup_var = var
        if self.layer.base:
            lookup_var = language.wrapping_sub(
                var, language.usize_literal(self.layer.base)
            )

        # Get index lookup expression from InnerLayer (self.next)
        # This returns an expression that evaluates to a palette index
        (_, index_expr) = self.next.genCode(code, None, lookup_var, language=language)

        # Look up value in palette: palette[index]
        # Cast index to usize for Rust array indexing
        index_expr_usize = language.as_usize(index_expr)
        expr = language.array_index(palette_name, index_expr_usize)
        expr = language.cast(retType, expr)

        # Apply OuterLayer's inverse arithmetic operations
        if self.layer.mult != 1:
            expr = "%d*%s" % (self.layer.mult, expr)
        if self.layer.bias > 0:
            expr = "%d+%s" % (self.layer.bias, expr)
        elif self.layer.bias < 0:
            expr = "%s-%d" % (expr, -self.layer.bias)

        # Bounds check
        span = language.usize_literal(len(self.layer.data))
        expr = language.tertiary("%s<%s" % (lookup_var, span), expr, self.layer.default)

        if name:
            funcName = code.addFunction(
                retType,
                name,
                ((language.usize, "u"),),
                expr,
                private=private,
                comment=self.shape_comment(),
            )
            expr = "%s(%s)" % (funcName, inputVar)

        return (retType, expr)


def gcd(lst):
    """
    >>> gcd([])
    1
    >>> gcd([48])
    48
    >>> gcd([-48])
    48
    >>> gcd([48, 60])
    12
    >>> gcd([48, 60, 6])
    6
    >>> gcd([48, 61, 6])
    1
    """
    it = iter(lst)
    x = abs(next(it, 1))
    for y in it:
        y = abs(y)
        while y:
            x, y = y, x % y
        if x == 1:
            break
    return x


def _best_reduction(values, minV, maxV):
    """Find the (unitBits, bias, mult) triple that minimizes unitBits.

    Tries three strategies on the values:
      1. Bias only: subtract minV.
      2. GCD only: divide by the common factor.
      3. Bias + GCD: subtract minV, then divide by GCD of remainders.
    """
    bias = 0
    mult = 1
    unitBits = binaryBitsFor(minV, maxV)

    b = minV
    candidateBits = binaryBitsFor(0, maxV - b)
    if unitBits > candidateBits:
        unitBits = candidateBits
        bias = b

    m = gcd(values)
    if m > 1:
        candidateBits = binaryBitsFor(minV // m, maxV // m)
        if unitBits > candidateBits:
            unitBits = candidateBits
            bias = 0
            mult = m

    if b:
        m = gcd(d - b for d in values)
        if m == 0:
            m = 1
        candidateBits = binaryBitsFor(0, (maxV - b) // m)
        if unitBits > candidateBits:
            unitBits = candidateBits
            bias = b
            mult = m

    return unitBits, bias, mult


class OuterLayer(Layer):
    """Arithmetic preprocessing layer that wraps InnerLayer.

    Before handing data to InnerLayer for binary splitting, OuterLayer
    tries to shrink the value range using arithmetic reductions:

    1. **Bias only**: subtract min value.  E.g. [100..115] → [0..15]
       reduces from 8-bit to 4-bit.  Generated code: ``bias + inner(u)``.

    2. **GCD only**: divide all values by their GCD.  E.g. [0,6,12,18]
       → [0,1,2,3] (GCD=6).  Generated code: ``mult * inner(u)``.

    3. **Bias + GCD**: subtract bias, then divide by GCD of remainders.
       Generated code: ``bias + mult * inner(u)``.

    The best reduction (fewest bits) wins.

    **Mult bake-in**: after choosing a GCD factor, if multiplying the
    stored values back up doesn't change the C integer type (e.g. both
    fit in uint8_t), store the un-divided values and skip the runtime
    multiplication.  This trades slightly larger sub-byte storage for
    one fewer runtime operation.

    The stored span is rebased to the smallest power-of-two-aligned
    interval containing all non-default values. Trailing default values
    in that rebased span are then stripped, since they can be handled by
    the generated bounds check. The reduced data is then passed to
    InnerLayer, and each InnerLayer solution is wrapped in an
    OuterSolution that accounts for the extra bias/mult ops.
    """

    def __init__(self, data, default, base=None):
        data = list(data)
        if base is None:
            base = _aligned_base_for_live_range(data, default)
        self.base = base
        data = data[self.base :]

        # Strip trailing default values — the generated bounds check
        # (``u < len``) handles out-of-range indices.
        while len(data) > 1 and data[-1] == default:
            data.pop()
        Layer.__init__(self, data)
        self.default = default

        self.minV, self.maxV = min(data), max(data)

        bias = 0
        mult = 1
        unitBits = binaryBitsFor(self.minV, self.maxV)
        if isinstance(self.minV, int) and isinstance(self.maxV, int):
            unitBits, bias, mult = _best_reduction(data, self.minV, self.maxV)

            # Compute the stored values after all reductions.
            base = list(self.data)
            data = [(d - bias) // mult for d in base]

            # Bake in width multiplier if doing so doesn't enlarge the
            # C integer type of the stored values.
            if mult > 1:
                undivided = [(d - bias) for d in base]
                divided_type_width = max(8, binaryBitsFor(min(data), max(data)))
                undivided_type_width = max(
                    8, binaryBitsFor(min(undivided), max(undivided))
                )
                if undivided_type_width <= divided_type_width:
                    data = undivided
                    unitBits = binaryBitsFor(min(undivided), max(undivided))
                    mult = 1

            # Bake in bias if doing so doesn't enlarge the C integer type
            # and doesn't introduce negative values (which would change
            # signedness and break sub-byte packing / inlining).
            # Don't bake if data is all zeros (constant), as InnerLayer
            # optimizes that case to cost=0.
            if bias != 0 and mult == 1 and min(base) >= 0 and max(data) != 0:
                current_type_width = max(8, binaryBitsFor(min(data), max(data)))
                base_type_width = max(8, binaryBitsFor(min(base), max(base)))
                if base_type_width <= current_type_width:
                    data = base
                    unitBits = binaryBitsFor(min(base), max(base))
                    bias = 0

        self.unitBits = unitBits
        self.extraOps = subByteAccessOps if self.unitBits < 8 else 0
        self.bias = bias
        if bias:
            self.extraOps += 1
        self.mult = mult
        if mult != 1:
            self.extraOps += 1

        self.bytes = ceil(self.unitBits * len(self.data) / 8)
        self.next = InnerLayer(data)

        extraCost = 0

        # Wrap each InnerLayer solution with our arithmetic overhead.
        layer = self.next
        for s in layer.solutions:
            nLookups = s.nLookups
            nExtraOps = s.nExtraOps + self.extraOps
            cost = s.cost + extraCost
            solution = OuterSolution(self, s, nLookups, nExtraOps, cost)
            self.solutions.append(solution)

        # Try palette encoding: indices -> unique values table
        # Only for integer data (strings already use mapping)
        if isinstance(self.minV, int) and isinstance(self.maxV, int):
            palette_solutions = self._try_palette_encoding(data, extraCost)
            self.solutions.extend(palette_solutions)

    def _try_palette_encoding(self, data, extraCost):
        """Try palette encoding: store indices + unique values table.

        Returns list of PaletteOuterSolution instances if beneficial,
        otherwise returns empty list.
        """
        # Extract unique values and build palette
        palette = sorted(set(data))

        # Check if index bits are smaller than value bits
        index_bits = binaryBitsFor(0, len(palette) - 1)
        value_bits = binaryBitsFor(min(palette), max(palette))

        # Only generate palette solution if it saves encoding bits
        # The Pareto frontier will decide if it's worth the extra lookup
        if index_bits >= value_bits:
            return []

        # Build index mapping and indices array
        value_to_index = {v: i for i, v in enumerate(palette)}
        indices = [value_to_index[v] for v in data]

        # Create InnerLayer for indices (can be split further!)
        indices_layer = InnerLayer(indices)

        # Calculate palette storage cost
        palette_bits = binaryBitsFor(min(palette), max(palette))
        palette_cost = ceil(palette_bits * len(palette) / 8)

        # Wrap each indices solution as a PaletteOuterSolution
        solutions = []
        for s in indices_layer.solutions:
            nLookups = s.nLookups + 1  # +1 for palette lookup
            nExtraOps = s.nExtraOps + self.extraOps
            cost = s.cost + palette_cost + extraCost

            solution = PaletteOuterSolution(
                self, s, nLookups, nExtraOps, cost, palette, value_to_index
            )
            solutions.append(solution)

        return solutions


# Public API


def pack_table(
    data: Union[List[Union[int, str]], Dict[int, Union[int, str]]],
    default: Union[int, str] = 0,
    compression: Optional[float] = 1,
    mapping: Optional[Dict[Any, Any]] = None,
) -> Union["OuterSolution", List["OuterSolution"]]:
    """Pack a static table of integers into compact lookup tables.

    Args:
        data: Either a dictionary mapping integer keys to values, or an
            iterable containing values for keys starting at zero. Values must
            all be integers, or all strings.
        default: Value to be used for keys not specified in data. Defaults
            to zero.
        compression: Tunes the size-vs-speed tradeoff. Higher values prefer
            smaller tables. If None, returns all Pareto-optimal solutions.
            Defaults to 1.
        mapping: Optional bidirectional mapping from integer keys to string
            values or vice versa. Used to convert non-integer values to integers.

    Returns:
        If compression is set: the best OuterSolution.
        If compression is None: list of all Pareto-optimal OuterSolutions.

    Raises:
        ValueError: If data is empty or dict keys are negative.
        TypeError: If dict keys are not integers or values are mixed types.
    """

    if not data:
        raise ValueError("data must not be empty")

    # Set up mapping.  See docstring.
    if mapping is not None:
        # Validate and normalize mapping to be bidirectional.
        # Accept either unidirectional (all int→str or all str→int) or
        # already-bidirectional mappings.
        if not mapping:
            raise ValueError("mapping must not be empty")

        # Check if it's a valid unidirectional mapping
        int_to_nonint = all(isinstance(k, int) and not isinstance(v, int) for k, v in mapping.items())
        nonint_to_int = all(not isinstance(k, int) and isinstance(v, int) for k, v in mapping.items())

        # Check if it's already bidirectional: every int key has a corresponding
        # non-int value that maps back, and vice versa
        if not (int_to_nonint or nonint_to_int):
            # Not unidirectional, check if it's a valid bidirectional mapping
            int_keys = {k for k in mapping.keys() if isinstance(k, int)}
            nonint_keys = {k for k in mapping.keys() if not isinstance(k, int)}

            # Verify bidirectional consistency: for all int→nonint pairs,
            # nonint→int must exist and match
            is_bidirectional = (
                int_keys and nonint_keys and
                all(isinstance(mapping.get(k), int) for k in nonint_keys) and
                all(not isinstance(mapping.get(k), int) for k in int_keys) and
                all(mapping.get(mapping.get(k)) == k for k in int_keys) and
                all(mapping.get(mapping.get(k)) == k for k in nonint_keys)
            )

            if not is_bidirectional:
                raise TypeError("mapping must be consistently int→str or str→int, or a valid bidirectional mapping")

        # Make bidirectional if not already
        mapping2 = mapping.copy()
        for k, v in mapping.items():
            if v not in mapping2:
                mapping2[v] = k
        mapping = mapping2
        del mapping2

    # Set up data as a list.
    if isinstance(data, dict):
        if not all(isinstance(k, int) for k in data.keys()):
            raise TypeError("dict keys must be integers")
        minK = min(data.keys())
        maxK = max(data.keys())
        if minK < 0:
            raise ValueError("dict keys must be non-negative")
        data2 = [default] * (maxK + 1)
        for k, v in data.items():
            data2[k] = v
        data = data2
        del data2

    # Convert all to integers
    if not (
        all(isinstance(v, int) for v in data)
        or all(not isinstance(v, int) for v in data)
    ):
        raise TypeError("data values must be all integers or all non-integers")
    if not isinstance(data[0], int) and mapping is not None:
        data = [mapping[v] for v in data]
    if not isinstance(default, int) and mapping is not None:
        default = mapping[default]

    aligned_layer = OuterLayer(data, default)
    solutions = list(aligned_layer.solutions)

    first_non_default = _first_non_default_index(data, default)
    if (
        first_non_default is not None
        and first_non_default != aligned_layer.base
    ):
        exact_layer = OuterLayer(data, default, base=first_non_default)
        inline_exact = any(
            not isinstance(s, PaletteOuterSolution)
            and getattr(getattr(s, "next", None), "bits", None) == 0
            and s.next.layer.bytes <= 8
            and isinstance(s.next.layer.minV, int)
            and s.next.layer.minV >= 0
            for s in exact_layer.solutions
        )
        if inline_exact:
            solutions = _prune_pareto_solutions(solutions + exact_layer.solutions)

    if compression is None:
        solutions.sort(key=lambda s: -s.fullCost)
        return solutions

    return pick_solution(solutions, compression)


def pick_solution(
    solutions: List["OuterSolution"], compression: float = 1
) -> "OuterSolution":
    """Select the best solution from the Pareto frontier.

    Args:
        solutions: List of OuterSolution objects (Pareto frontier).
        compression: Controls size-vs-speed tradeoff. Values 1..9 use the
            historical heuristic. Values <= 0 pick a flat / unsplit solution
            when available. Values >= 10 minimize raw table bytes. Defaults
            to 1.

    Returns:
        The OuterSolution with the best score.

    For 1..9, the scoring function
    ``nLookups + compression * log2(fullCost)`` balances lookup count
    against log-scaled storage cost.
    """
    if compression <= 0:
        flat = [
            s for s in solutions
            if not isinstance(s, PaletteOuterSolution)
            and getattr(getattr(s, "next", None), "bits", None) == 0
        ]
        if flat:
            return min(flat, key=lambda s: (s.nExtraOps, s.cost))
        return min(solutions, key=lambda s: (s.nLookups, s.nExtraOps, s.cost))

    if compression >= 10:
        return min(solutions, key=lambda s: (s.cost, s.nLookups, s.nExtraOps))

    return min(solutions, key=lambda s: s.nLookups + compression * log2(s.fullCost))


if __name__ == "__main__":
    import doctest

    sys.exit(doctest.testmod().failed)
