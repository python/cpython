import sys
from ctypes import Array, Structure, Union

_array_type = type(Array)

def _get_other_endian_attr():
    """Return the appropriate other endian attribute based on system byte order."""
    return "__ctype_be__" if sys.byteorder == "little" else "__ctype_le__"

def _other_endian(typ):
    """Return the type with the 'other' byte order. Simple types like
    c_int and so on already have __ctype_be__ and __ctype_le__
    attributes which contain the types; for more complicated types,
    arrays and structures are supported.
    """
    other_endian_attr = _get_other_endian_attr()

    # if typ is array
    if isinstance(typ, _array_type):
        return _other_endian(typ._type_) * typ._length_
    # if typ is structure or union
    if issubclass(typ, (Structure, Union)):
        return typ
    # check other endian attribute (present if typ is primitive type)
    if hasattr(typ, other_endian_attr):
        return getattr(typ, other_endian_attr)

    raise TypeError("This type does not support other endian: %s" % typ)

class _swapped_meta:
    def __setattr__(self, attrname, value):
        if attrname == "_fields_":
            # Use a generator expression to avoid creating a new list
            fields = [
                (desc[0], _other_endian(desc[1])) + desc[2:]
                for desc in value
            ]
            value = fields
        super().__setattr__(attrname, value)

class _swapped_struct_meta(_swapped_meta, type(Structure)): pass
class _swapped_union_meta(_swapped_meta, type(Union)): pass

################################################################

# Note: The Structure metaclass checks for the *presence* (not the
# value!) of a _swappedbytes_ attribute to determine the bit order in
# structures containing bit fields.

# Determine the byte order and define types accordingly
if sys.byteorder == "little":
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
    raise RuntimeError("Invalid byteorder")
