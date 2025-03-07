import re
import sys
import unittest
from ctypes import (CFUNCTYPE, POINTER, sizeof, Union,
                    Structure, LittleEndianStructure, BigEndianStructure,
                    c_char, c_byte, c_ubyte,
                    c_short, c_ushort, c_int, c_uint,
                    c_long, c_ulong, c_longlong, c_ulonglong, c_uint64,
                    c_bool, c_float, c_double, c_longdouble, py_object)


if sys.byteorder == "little":
    THIS_ENDIAN = "<"
    OTHER_ENDIAN = ">"
else:
    THIS_ENDIAN = ">"
    OTHER_ENDIAN = "<"


def normalize(format):
    # Remove current endian specifier and white space from a format
    # string
    if format is None:
        return ""
    format = format.replace(OTHER_ENDIAN, THIS_ENDIAN)
    return re.sub(r"\s", "", format)


class Test(unittest.TestCase):
    def test_native_types(self):
        for tp, fmt, shape, itemtp in native_types:
            ob = tp()
            v = memoryview(ob)
            self.assertEqual(normalize(v.format), normalize(fmt))
            if shape:
                self.assertEqual(len(v), shape[0])
            else:
                self.assertRaises(TypeError, len, v)
            self.assertEqual(v.itemsize, sizeof(itemtp))
            self.assertEqual(v.shape, shape)
            # XXX Issue #12851: PyCData_NewGetBuffer() must provide strides
            #     if requested. memoryview currently reconstructs missing
            #     stride information, so this assert will fail.
            # self.assertEqual(v.strides, ())

            # they are always read/write
            self.assertFalse(v.readonly)

            n = 1
            for dim in v.shape:
                n = n * dim
            self.assertEqual(n * v.itemsize, len(v.tobytes()))

    def test_endian_types(self):
        for tp, fmt, shape, itemtp in endian_types:
            ob = tp()
            v = memoryview(ob)
            self.assertEqual(v.format, fmt)
            if shape:
                self.assertEqual(len(v), shape[0])
            else:
                self.assertRaises(TypeError, len, v)
            self.assertEqual(v.itemsize, sizeof(itemtp))
            self.assertEqual(v.shape, shape)
            # XXX Issue #12851
            # self.assertEqual(v.strides, ())

            # they are always read/write
            self.assertFalse(v.readonly)

            n = 1
            for dim in v.shape:
                n = n * dim
            self.assertEqual(n * v.itemsize, len(v.tobytes()))


# define some structure classes

class Point(Structure):
    _fields_ = [("x", c_long), ("y", c_long)]

class PackedPoint(Structure):
    _pack_ = 2
    _fields_ = [("x", c_long), ("y", c_long)]

class PointMidPad(Structure):
    _fields_ = [("x", c_byte), ("y", c_uint)]

class PackedPointMidPad(Structure):
    _pack_ = 2
    _fields_ = [("x", c_byte), ("y", c_uint64)]

class PointEndPad(Structure):
    _fields_ = [("x", c_uint), ("y", c_byte)]

class PackedPointEndPad(Structure):
    _pack_ = 2
    _fields_ = [("x", c_uint64), ("y", c_byte)]

class Point2(Structure):
    pass
Point2._fields_ = [("x", c_long), ("y", c_long)]

class EmptyStruct(Structure):
    _fields_ = []

class aUnion(Union):
    _fields_ = [("a", c_int)]

class StructWithArrays(Structure):
    _fields_ = [("x", c_long * 3 * 2), ("y", Point * 4)]

class Incomplete(Structure):
    pass

class Complete(Structure):
    pass
PComplete = POINTER(Complete)
Complete._fields_ = [("a", c_long)]


################################################################
#
# This table contains format strings as they look on little endian
# machines.  The test replaces '<' with '>' on big endian machines.
#

# Platform-specific type codes
s_bool = {1: '?', 2: 'H', 4: 'L', 8: 'Q'}[sizeof(c_bool)]
s_short = {2: 'h', 4: 'l', 8: 'q'}[sizeof(c_short)]
s_ushort = {2: 'H', 4: 'L', 8: 'Q'}[sizeof(c_ushort)]
s_int = {2: 'h', 4: 'i', 8: 'q'}[sizeof(c_int)]
s_uint = {2: 'H', 4: 'I', 8: 'Q'}[sizeof(c_uint)]
s_long = {4: 'l', 8: 'q'}[sizeof(c_long)]
s_ulong = {4: 'L', 8: 'Q'}[sizeof(c_ulong)]
s_longlong = "q"
s_ulonglong = "Q"
s_float = "f"
s_double = "d"
s_longdouble = "g"

# Alias definitions in ctypes/__init__.py
if c_int is c_long:
    s_int = s_long
if c_uint is c_ulong:
    s_uint = s_ulong
if c_longlong is c_long:
    s_longlong = s_long
if c_ulonglong is c_ulong:
    s_ulonglong = s_ulong
if c_longdouble is c_double:
    s_longdouble = s_double


native_types = [
    # type                      format                  shape           calc itemsize

    ## simple types

    (c_char,                    "<c",                   (),           c_char),
    (c_byte,                    "<b",                   (),           c_byte),
    (c_ubyte,                   "<B",                   (),           c_ubyte),
    (c_short,                   "<" + s_short,          (),           c_short),
    (c_ushort,                  "<" + s_ushort,         (),           c_ushort),

    (c_int,                     "<" + s_int,            (),           c_int),
    (c_uint,                    "<" + s_uint,           (),           c_uint),

    (c_long,                    "<" + s_long,           (),           c_long),
    (c_ulong,                   "<" + s_ulong,          (),           c_ulong),

    (c_longlong,                "<" + s_longlong,       (),           c_longlong),
    (c_ulonglong,               "<" + s_ulonglong,      (),           c_ulonglong),

    (c_float,                   "<f",                   (),           c_float),
    (c_double,                  "<d",                   (),           c_double),

    (c_longdouble,              "<" + s_longdouble,     (),           c_longdouble),

    (c_bool,                    "<" + s_bool,           (),           c_bool),
    (py_object,                 "<O",                   (),           py_object),

    ## pointers

    (POINTER(c_byte),           "&<b",                  (),           POINTER(c_byte)),
    (POINTER(POINTER(c_long)),  "&&<" + s_long,         (),           POINTER(POINTER(c_long))),

    ## arrays and pointers

    (c_double * 4,              "<d",                   (4,),           c_double),
    (c_double * 0,              "<d",                   (0,),           c_double),
    (c_float * 4 * 3 * 2,       "<f",                   (2,3,4),        c_float),
    (c_float * 4 * 0 * 2,       "<f",                   (2,0,4),        c_float),
    (POINTER(c_short) * 2,      "&<" + s_short,         (2,),           POINTER(c_short)),
    (POINTER(c_short) * 2 * 3,  "&<" + s_short,         (3,2,),         POINTER(c_short)),
    (POINTER(c_short * 2),      "&(2)<" + s_short,      (),             POINTER(c_short)),

    ## structures and unions

    (Point2,                    "T{<l:x:<l:y:}".replace('l', s_long),   (),  Point2),
    (Point,                     "T{<l:x:<l:y:}".replace('l', s_long),   (),  Point),
    (PackedPoint,               "T{<l:x:<l:y:}".replace('l', s_long),   (),  PackedPoint),
    (PointMidPad,               "T{<b:x:3x<I:y:}".replace('I', s_uint), (),  PointMidPad),
    (PackedPointMidPad,         "T{<b:x:x<Q:y:}",                       (),  PackedPointMidPad),
    (PointEndPad,               "T{<I:x:<b:y:3x}".replace('I', s_uint), (),  PointEndPad),
    (PackedPointEndPad,         "T{<Q:x:<b:y:x}",                       (),  PackedPointEndPad),
    (EmptyStruct,               "T{}",                                  (),  EmptyStruct),
    # the pep doesn't support unions
    (aUnion,                    "B",                                   (),  aUnion),
    # structure with sub-arrays
    (StructWithArrays, "T{(2,3)<l:x:(4)T{<l:x:<l:y:}:y:}".replace('l', s_long), (), StructWithArrays),
    (StructWithArrays * 3, "T{(2,3)<l:x:(4)T{<l:x:<l:y:}:y:}".replace('l', s_long), (3,), StructWithArrays),

    ## pointer to incomplete structure
    (Incomplete,                "B",                    (),           Incomplete),
    (POINTER(Incomplete),       "&B",                   (),           POINTER(Incomplete)),

    # 'Complete' is a structure that starts incomplete, but is completed after the
    # pointer type to it has been created.
    (Complete,                  "T{<l:a:}".replace('l', s_long), (), Complete),
    # Unfortunately the pointer format string is not fixed...
    (POINTER(Complete),         "&B",                   (),           POINTER(Complete)),

    ## other

    # function signatures are not implemented
    (CFUNCTYPE(None),           "X{}",                  (),           CFUNCTYPE(None)),

    ]


class BEPoint(BigEndianStructure):
    _fields_ = [("x", c_long), ("y", c_long)]

class LEPoint(LittleEndianStructure):
    _fields_ = [("x", c_long), ("y", c_long)]


# This table contains format strings as they really look, on both big
# and little endian machines.
endian_types = [
    (BEPoint, "T{>l:x:>l:y:}".replace('l', s_long), (), BEPoint),
    (LEPoint * 1, "T{<l:x:<l:y:}".replace('l', s_long), (1,), LEPoint),
    (POINTER(BEPoint), "&T{>l:x:>l:y:}".replace('l', s_long), (), POINTER(BEPoint)),
    (POINTER(LEPoint), "&T{<l:x:<l:y:}".replace('l', s_long), (), POINTER(LEPoint)),
    ]


if __name__ == "__main__":
    unittest.main()
