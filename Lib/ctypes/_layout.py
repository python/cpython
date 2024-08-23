import sys
import warnings
import struct

from _ctypes import CField, buffer_info
import ctypes

def round_down(n, multiple):
    assert(n >= 0);
    assert(multiple >= 0);
    if (multiple == 0):
        return n;
    return int(n / multiple) * multiple;

def round_up(n, multiple):
    assert(n >= 0);
    assert(multiple >= 0);
    if (multiple == 0):
        return n;
    return int((n + multiple - 1) / multiple) * multiple;

def LOW_BIT(offset):
    return offset & 0xFFFF;

def NUM_BITS(bitsize):
    return bitsize >> 16;

def BUILD_SIZE(bitsize, offset):
    assert(0 <= offset);
    assert(offset <= 0xFFFF);
    ## We don't support zero length bitfields.
    ## And GET_BITFIELD uses NUM_BITS(size)==0,
    ## to figure out whether we are handling a bitfield.
    assert(0 < bitsize);
    result = (bitsize << 16) + offset;
    assert(bitsize == NUM_BITS(result));
    assert(offset == LOW_BIT(result));
    return result;

def build_size(bit_size, effective_bitsof, big_endian, type_size):
    if big_endian:
        return BUILD_SIZE(bit_size, 8 * type_size - effective_bitsof - bit_size)
    return BUILD_SIZE(bit_size, effective_bitsof)

_INT_MAX = (1 << (ctypes.sizeof(ctypes.c_int) * 8) - 1) - 1

class _structunion_layout:
    """Compute the layout of a struct or union

    This is a callable that returns an object with attributes:
    - fields: sequence of CField objects
    - size: total size of the aggregate
    - align: total alignment requirement of the aggregate
    - format_spec: buffer format specification (as a string, UTF-8 but
      best kept ASCII-only)

    Technically this is a class, that might change.
    """
    def __init__(self, cls, fields, is_struct, base, _ms):
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

        _pack_ = getattr(cls, '_pack_', None)
        if _pack_ is not None:
            try:
                _pack_ = int(_pack_)
            except (TypeError, ValueError):
                raise ValueError("_pack_ must be an integer")
            if _pack_ < 0:
                raise ValueError("_pack_ must be a non-negative integer")
            if _pack_ > _INT_MAX:
                raise ValueError("_pack_ too big")
            if not _ms:
                raise ValueError('_pack_ is not compatible with gcc-sysv layout')

        self.fields = []

        if is_struct:
            format_spec = "T{"
        else:
            format_spec = "B"

        state_field_size = 0
        # `8 * offset + bitofs` points to where the  next field would start.
        state_bitofs = 0
        state_offset = 0
        state_size = 0
        if base:
            state_size = state_offset = ctypes.sizeof(base)

        union_size = 0
        last_size = state_size
        last_field = None
        for i, field in enumerate(fields):
            if not is_struct:
                # Unions start fresh each time
                state_field_size = 0
                state_bitofs = 0
                state_offset = 0
                state_size = 0

            field = tuple(field)
            try:
                name, ftype = field
                is_bitfield = False
                bit_size = ctypes.sizeof(ftype) * 8
            except ValueError:
                name, ftype, bit_size = field
                is_bitfield = True
                if bit_size <= 0:
                    raise ValueError(f'number of bits invalid for bit field {name!r}')

            type_size = ctypes.sizeof(ftype)
            type_align = ctypes.alignment(ftype)

            if not _ms:
                # We don't use packstate->offset here, so clear it, if it has been set.
                state_bitofs += state_offset * 8;
                state_offset = 0

                ## We don't use packstate->offset here, so clear it, if it has been set.
                state_bitofs += state_offset * 8;
                state_offset = 0;

                assert(_pack_ in (0, None)); ## TODO: This shouldn't be a C assertion

                ## Determine whether the bit field, if placed at the next free bit,
                ## fits within a single object of its specified type.
                ## That is: determine a "slot", sized & aligned for the specified type,
                ## which contains the bitfield's beginning:
                slot_start_bit = round_down(state_bitofs, 8 * type_align);
                slot_end_bit = slot_start_bit + 8 * type_size;
                ## And see if it also contains the bitfield's last bit:
                field_end_bit = state_bitofs + bit_size;
                if field_end_bit > slot_end_bit:
                    ## It doesn't: add padding (bump up to the next alignment boundary)
                    state_bitofs = round_up(state_bitofs, 8 * type_align);

                assert(state_offset == 0);

                offset = int(round_down(state_bitofs, 8 * type_align) / 8);
                if is_bitfield:
                    effective_bitsof = state_bitofs - 8 * offset;
                    size = build_size(bit_size, effective_bitsof,
                                      big_endian, type_size)
                    assert(effective_bitsof <= type_size * 8);
                else:
                    size = type_size;

                state_bitofs += bit_size;
                state_size = int(round_up(state_bitofs, 8) / 8);
            else:
                if _pack_:
                    type_align = min(_pack_, type_align);

                ## packstate->offset points to end of current bitfield.
                ## packstate->bitofs is generally non-positive,
                ## and 8 * packstate->offset + packstate->bitofs points just behind
                ## the end of the last field we placed.
                if ((0 < state_bitofs + bit_size) or (8 * type_size != state_field_size)):
                    ## Close the previous bitfield (if any).
                    ## and start a new bitfield:
                    state_offset = round_up(state_offset, type_align);

                    state_offset += type_size;

                    state_field_size = type_size * 8;
                    ## Reminder: 8 * (packstate->offset) + bitofs points to where we would start a
                    ## new field.  Ie just behind where we placed the last field plus an
                    ## allowance for alignment.
                    state_bitofs = - state_field_size;

                assert(8 * type_size == state_field_size);

                offset = state_offset - int((state_field_size) // 8);
                if is_bitfield:
                    assert(0 <= (state_field_size + state_bitofs));
                    assert((state_field_size + state_bitofs) < type_size * 8);
                    size = build_size(bit_size, state_field_size + state_bitofs,
                                      big_endian, type_size)
                else:
                    size = type_size;
                assert(state_field_size + state_bitofs <= type_size * 8);

                state_bitofs += bit_size;
                state_size = state_offset;


            assert((not is_bitfield) or (LOW_BIT(size) <= size * 8));

            padding = offset - last_size
            if is_struct:
                format_spec += padding_spec(padding)

                fieldfmt, bf_ndim, bf_shape = buffer_info(ftype)
                if fieldfmt is None:
                    fieldfmt = "B"
                buf = f"{fieldfmt}:{name}:"
                if bf_shape:
                    shape_numbers = ",".join(str(n) for n in bf_shape)
                    buf = f"({shape_numbers}){buf}"
                format_spec += buf

            last_field = CField(
                name=name,
                type=ftype,
                size=size,
                offset=offset,
                bit_size=bit_size if is_bitfield else None,
                index=i,
            )
            self.fields.append(last_field)
            align = max(align, type_align)
            last_size = state_size
            union_size = max(state_size, union_size);

        if is_struct:
            total_size = state_size
        else:
            total_size = union_size

        # Adjust the size according to the alignment requirements
        aligned_size = int((total_size + align - 1) / align) * align

        if is_struct:
            padding = aligned_size - total_size
            format_spec += f"{padding_spec(padding)}}}"

        self.size = aligned_size
        self.align = align
        self.format_spec = format_spec

def padding_spec(padding):
    if padding <= 0:
        return ""
    if padding == 1:
        return "x"
    return f"{padding}x"


def default_layout(cls, *args, **kwargs):
    layout = getattr(cls, '_layout_', None)
    if layout is None:
        if sys.platform == 'win32' or getattr(cls, '_pack_', None):
            return _structunion_layout(cls, *args, **kwargs, _ms=True)
        return _structunion_layout(cls, *args, **kwargs, _ms=False)
    elif layout == 'ms':
        return _structunion_layout(cls, *args, **kwargs, _ms=True)
    elif layout == 'gcc-sysv':
        return _structunion_layout(cls, *args, **kwargs, _ms=False)
    else:
        raise ValueError(f'unknown _layout_: {layout!r}')
