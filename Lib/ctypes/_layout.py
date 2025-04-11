"""Python implementation of computing the layout of a struct/union

This code is internal and tightly coupled to the C part. The interface
may change at any time.
"""

import sys

from _ctypes import CField, buffer_info
import ctypes

def round_down(n, multiple):
    assert n >= 0
    assert multiple > 0
    return (n // multiple) * multiple

def round_up(n, multiple):
    assert n >= 0
    assert multiple > 0
    return ((n + multiple - 1) // multiple) * multiple

_INT_MAX = (1 << (ctypes.sizeof(ctypes.c_int) * 8) - 1) - 1


class StructUnionLayout:
    def __init__(self, fields, size, align, format_spec):
        # sequence of CField objects
        self.fields = fields

        # total size of the aggregate (rounded up to alignment)
        self.size = size

        # total alignment requirement of the aggregate
        self.align = align

        # buffer format specification (as a string, UTF-8 but bes
        # kept ASCII-only)
        self.format_spec = format_spec


def get_layout(cls, input_fields, is_struct, base):
    """Return a StructUnionLayout for the given class.

    Called by PyCStructUnionType_update_stginfo when _fields_ is assigned
    to a class.
    """
    # Currently there are two modes, selectable using the '_layout_' attribute:
    #
    # 'gcc-sysv' mode places fields one after another, bit by bit.
    #   But "each bit field must fit within a single object of its specified
    #   type" (GCC manual, section 15.8 "Bit Field Packing"). When it doesn't,
    #   we insert a few bits of padding to avoid that.
    #
    # 'ms' mode works similar except for bitfield packing.  Adjacent
    #   bit-fields are packed into the same 1-, 2-, or 4-byte allocation unit
    #   if the integral types are the same size and if the next bit-field fits
    #   into the current allocation unit without crossing the boundary imposed
    #   by the common alignment requirements of the bit-fields.
    #
    #   See https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html#index-mms-bitfields
    #   for details.

    # We do not support zero length bitfields (we use bitsize != 0
    # elsewhere to indicate a bitfield). Here, non-bitfields have bit_size
    # set to size*8.

    # For clarity, variables that count bits have `bit` in their names.

    layout = getattr(cls, '_layout_', None)
    if layout is None:
        if sys.platform == 'win32' or getattr(cls, '_pack_', None):
            gcc_layout = False
        else:
            gcc_layout = True
    elif layout == 'ms':
        gcc_layout = False
    elif layout == 'gcc-sysv':
        gcc_layout = True
    else:
        raise ValueError(f'unknown _layout_: {layout!r}')

    align = getattr(cls, '_align_', 1)
    if align < 0:
        raise ValueError('_align_ must be a non-negative integer')
    elif align == 0:
        # Setting `_align_ = 0` amounts to using the default alignment
        align == 1

    if base:
        align = max(ctypes.alignment(base), align)

    swapped_bytes = hasattr(cls, '_swappedbytes_')
    if swapped_bytes:
        big_endian = sys.byteorder == 'little'
    else:
        big_endian = sys.byteorder == 'big'

    pack = getattr(cls, '_pack_', None)
    if pack is not None:
        try:
            pack = int(pack)
        except (TypeError, ValueError):
            raise ValueError("_pack_ must be an integer")
        if pack < 0:
            raise ValueError("_pack_ must be a non-negative integer")
        if pack > _INT_MAX:
            raise ValueError("_pack_ too big")
        if gcc_layout:
            raise ValueError('_pack_ is not compatible with gcc-sysv layout')

    result_fields = []

    if is_struct:
        format_spec_parts = ["T{"]
    else:
        format_spec_parts = ["B"]

    last_field_bit_size = 0  # used in MS layout only

    # `8 * next_byte_offset + next_bit_offset` points to where the
    # next field would start.
    next_bit_offset = 0
    next_byte_offset = 0

    # size if this was a struct (sum of field sizes, plus padding)
    struct_size = 0
    # max of field sizes; only meaningful for unions
    union_size = 0

    if base:
        struct_size = ctypes.sizeof(base)
        if gcc_layout:
            next_bit_offset = struct_size * 8
        else:
            next_byte_offset = struct_size

    last_size = struct_size
    for i, field in enumerate(input_fields):
        if not is_struct:
            # Unions start fresh each time
            last_field_bit_size = 0
            next_bit_offset = 0
            next_byte_offset = 0

        # Unpack the field
        field = tuple(field)
        try:
            name, ctype = field
        except (ValueError, TypeError):
            try:
                name, ctype, bit_size = field
            except (ValueError, TypeError) as exc:
                raise ValueError(
                    '_fields_ must be a sequence of (name, C type) pairs '
                    + 'or (name, C type, bit size) triples') from exc
            is_bitfield = True
            if bit_size <= 0:
                raise ValueError(
                    f'number of bits invalid for bit field {name!r}')
            type_size = ctypes.sizeof(ctype)
            if bit_size > type_size * 8:
                raise ValueError(
                    f'number of bits invalid for bit field {name!r}')
        else:
            is_bitfield = False
            type_size = ctypes.sizeof(ctype)
            bit_size = type_size * 8

        type_bit_size = type_size * 8
        type_align = ctypes.alignment(ctype) or 1
        type_bit_align = type_align * 8

        if gcc_layout:
            # We don't use next_byte_offset here
            assert pack is None
            assert next_byte_offset == 0

            # Determine whether the bit field, if placed at the next
            # free bit, fits within a single object of its specified type.
            # That is: determine a "slot", sized & aligned for the
            # specified type, which contains the bitfield's beginning:
            slot_start_bit = round_down(next_bit_offset, type_bit_align)
            slot_end_bit = slot_start_bit + type_bit_size
            # And see if it also contains the bitfield's last bit:
            field_end_bit = next_bit_offset + bit_size
            if field_end_bit > slot_end_bit:
                # It doesn't: add padding (bump up to the next
                # alignment boundary)
                next_bit_offset = round_up(next_bit_offset, type_bit_align)

            offset = round_down(next_bit_offset, type_bit_align) // 8
            if is_bitfield:
                bit_offset = next_bit_offset - 8 * offset
                assert bit_offset <= type_bit_size
            else:
                assert offset == next_bit_offset / 8

            next_bit_offset += bit_size
            struct_size = round_up(next_bit_offset, 8) // 8
        else:
            if pack:
                type_align = min(pack, type_align)

            # next_byte_offset points to end of current bitfield.
            # next_bit_offset is generally non-positive,
            # and 8 * next_byte_offset + next_bit_offset points just behind
            # the end of the last field we placed.
            if (
                (0 < next_bit_offset + bit_size)
                or (type_bit_size != last_field_bit_size)
            ):
                # Close the previous bitfield (if any)
                # and start a new bitfield
                next_byte_offset = round_up(next_byte_offset, type_align)

                next_byte_offset += type_size

                last_field_bit_size = type_bit_size
                # Reminder: 8 * (next_byte_offset) + next_bit_offset
                # points to where we would start a new field, namely
                # just behind where we placed the last field plus an
                # allowance for alignment.
                next_bit_offset = -last_field_bit_size

            assert type_bit_size == last_field_bit_size

            offset = next_byte_offset - last_field_bit_size // 8
            if is_bitfield:
                assert 0 <= (last_field_bit_size + next_bit_offset)
                bit_offset = last_field_bit_size + next_bit_offset
            if type_bit_size:
                assert (last_field_bit_size + next_bit_offset) < type_bit_size

            next_bit_offset += bit_size
            struct_size = next_byte_offset

        if is_bitfield and big_endian:
            # On big-endian architectures, bit fields are also laid out
            # starting with the big end.
            bit_offset = type_bit_size - bit_size - bit_offset

        # Add the format spec parts
        if is_struct:
            padding = offset - last_size
            format_spec_parts.append(padding_spec(padding))

            fieldfmt, bf_ndim, bf_shape = buffer_info(ctype)

            if bf_shape:
                format_spec_parts.extend((
                    "(",
                    ','.join(str(n) for n in bf_shape),
                    ")",
                ))

            if fieldfmt is None:
                fieldfmt = "B"
            if isinstance(name, bytes):
                # a bytes name would be rejected later, but we check early
                # to avoid a BytesWarning with `python -bb`
                raise TypeError(
                    f"field {name!r}: name must be a string, not bytes")
            format_spec_parts.append(f"{fieldfmt}:{name}:")

        result_fields.append(CField(
            name=name,
            type=ctype,
            byte_size=type_size,
            byte_offset=offset,
            bit_size=bit_size if is_bitfield else None,
            bit_offset=bit_offset if is_bitfield else None,
            index=i,

            # Do not use CField outside ctypes, yet.
            # The constructor is internal API and may change without warning.
            _internal_use=True,
        ))
        if is_bitfield and not gcc_layout:
            assert type_bit_size > 0

        align = max(align, type_align)
        last_size = struct_size
        if not is_struct:
            union_size = max(struct_size, union_size)

    if is_struct:
        total_size = struct_size
    else:
        total_size = union_size

    # Adjust the size according to the alignment requirements
    aligned_size = round_up(total_size, align)

    # Finish up the format spec
    if is_struct:
        padding = aligned_size - total_size
        format_spec_parts.append(padding_spec(padding))
        format_spec_parts.append("}")

    return StructUnionLayout(
        fields=result_fields,
        size=aligned_size,
        align=align,
        format_spec="".join(format_spec_parts),
    )


def padding_spec(padding):
    if padding <= 0:
        return ""
    if padding == 1:
        return "x"
    return f"{padding}x"
