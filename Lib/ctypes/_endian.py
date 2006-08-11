######################################################################
#  This file should be kept compatible with Python 2.3, see PEP 291. #
######################################################################
import sys
from ctypes import *

_array_type = type(c_int * 3)

def _other_endian(typ):
    """Return the type with the 'other' byte order.  Simple types like
    c_int and so on already have __ctype_be__ and __ctype_le__
    attributes which contain the types, for more complicated types
    only arrays are supported.
    """
    try:
        return getattr(typ, _OTHER_ENDIAN)
    except AttributeError:
        if type(typ) == _array_type:
            return _other_endian(typ._type_) * typ._length_
        raise TypeError("This type does not support other endian: %s" % typ)

class _swapped_meta(type(Structure)):
    def __setattr__(self, attrname, value):
        if attrname == "_fields_":
            fields = []
            for desc in value:
                name = desc[0]
                typ = desc[1]
                rest = desc[2:]
                fields.append((name, _other_endian(typ)) + rest)
            value = fields
        super(_swapped_meta, self).__setattr__(attrname, value)

################################################################

# Note: The Structure metaclass checks for the *presence* (not the
# value!) of a _swapped_bytes_ attribute to determine the bit order in
# structures containing bit fields.

if sys.byteorder == "little":
    _OTHER_ENDIAN = "__ctype_be__"

    LittleEndianStructure = Structure

    class BigEndianStructure(Structure):
        """Structure with big endian byte order"""
        __metaclass__ = _swapped_meta
        _swappedbytes_ = None

elif sys.byteorder == "big":
    _OTHER_ENDIAN = "__ctype_le__"

    BigEndianStructure = Structure
    class LittleEndianStructure(Structure):
        """Structure with little endian byte order"""
        __metaclass__ = _swapped_meta
        _swappedbytes_ = None

else:
    raise RuntimeError("Invalid byteorder")
