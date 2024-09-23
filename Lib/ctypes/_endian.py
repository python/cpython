import sys
from ctypes import Array, Structure, Union

_array_type = type(Array)

def _other_endian(typ, other_endian_attr):
    """Return the type with the 'other' byte order."""
    # Check for _OTHER_ENDIAN attribute (present if typ is primitive type)
    if hasattr(typ, other_endian_attr):
        return getattr(typ, other_endian_attr)
    # If typ is an array
    if isinstance(typ, _array_type):
        return _other_endian(typ._type_, other_endian_attr) * typ._length_
    # If typ is structure or union
    if issubclass(typ, (Structure, Union)):
        return typ
    raise TypeError(f"This type does not support other endian: {typ}")

class _swapped_meta:
    def __setattr__(self, attrname, value):
        if attrname == "_fields_":
            # Use a generator to avoid unnecessary list creation
            fields = [
                (desc[0], _other_endian(desc[1], self._other_endian_attr)) + desc[2:]
                for desc in value
            ]
            value = fields
        super().__setattr__(attrname, value)

class _swapped_struct_meta(_swapped_meta, type(Structure)):
    _other_endian_attr = None  # Placeholder for other endian attribute

class _swapped_union_meta(_swapped_meta, type(Union)):
    _other_endian_attr = None  # Placeholder for other endian attribute

# Determine the byte order and define types accordingly
if sys.byteorder == "little":
    other_endian_attr = "__ctype_be__"
    LittleEndianStructure = Structure

    class BigEndianStructure(Structure, metaclass=_swapped_struct_meta):
        """Structure with big endian byte order"""
        __slots__ = ()
        _swappedbytes_ = None

    LittleEndianUnion = Union

    class BigEndianUnion(Union, metaclass=_swapped_union_meta):
        """Union with big endian byte order"""
        __slots__ = ()
        _swappedbytes_ = None

elif sys.byteorder == "big":
    other_endian_attr = "__ctype_le__"
    BigEndianStructure = Structure

    class LittleEndianStructure(Structure, metaclass=_swapped_struct_meta):
        """Structure with little endian byte order"""
        __slots__ = ()
        _swappedbytes_ = None

    BigEndianUnion = Union

    class LittleEndianUnion(Union, metaclass=_swapped_union_meta):
        """Union with little endian byte order"""
        __slots__ = ()
        _swappedbytes_ = None

else:
    raise RuntimeError(f"Invalid byteorder: {sys.byteorder}")

# Set the other endian attribute for swapped metas
_swapped_struct_meta._other_endian_attr = other_endian_attr
_swapped_union_meta._other_endian_attr = other_endian_attr
