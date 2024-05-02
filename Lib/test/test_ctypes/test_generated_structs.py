"""Test CTypes structs, unions, bitfields against C equivalents.

The types here are auto-converted to C source at
`Modules/_ctypes/_ctypes_test_generated.c.h`, which is compiled into
_ctypes_test.

The generated tests return a list of raw things to check,
or [None, skip_reason] to skip the test.

Run this module to regenerate the files:

./python Lib/test/test_ctypes/test_generated_structs.py > Modules/_ctypes/_ctypes_test_generated.c.h
"""

import unittest
from test.support import import_helper
import re
from dataclasses import dataclass
from functools import cached_property
import sys

import ctypes
from ctypes import Structure, Union, _SimpleCData
from ctypes import sizeof, alignment, pointer, string_at
_ctypes_test = import_helper.import_module("_ctypes_test")

KNOWN_COMPILERS = 'defined(MS_WIN32) || defined(__GNUC__) || defined(__clang__)'

# ctypes erases the difference between `c_int` and e.g.`c_int16`.
# To keep it, we'll use custom subclasses with the C name in `_c_name`.

for c_name, ctypes_name in {
    'signed char': 'c_byte',
    'short': 'c_short',
    'int': 'c_int',
    'long': 'c_long',
    'long long': 'c_longlong',
    'unsigned char': 'c_ubyte',
    'unsigned short': 'c_ushort',
    'unsigned int': 'c_uint',
    'unsigned long': 'c_ulong',
    'unsigned long long': 'c_ulonglong',
    **{f'{u}int{n}_t': f'c_{u}int{n}'
       for u in ('', 'u')
       for n in (8, 16, 32, 64)}
}.items():
    ctype = getattr(ctypes, ctypes_name)
    newtype = type(ctypes_name, (ctype,), {'_c_name': c_name})
    globals()[ctypes_name] = newtype

TESTCASES = {}
def register(name=None):
    def decorator(cls, name=name):
        if name is None:
            name = cls.__name__
        assert name.isascii()  # will be used in _PyUnicode_EqualToASCIIString
        assert name.isidentifier()  # will be used as a C identifier
        assert name not in TESTCASES
        TESTCASES[name] = cls
        return cls
    return decorator

@register()
class SingleInt(Structure):
    _fields_ = [('a', c_int)]

@register()
class SingleInt_Union(Union):
    _fields_ = [('a', c_int)]


@register()
class SingleU32(Structure):
    _fields_ = [('a', c_uint32)]


@register()
class SimpleStruct(Structure):
    _fields_ = [('x', c_int32), ('y', c_int8), ('z', c_uint16)]


@register()
class SimpleUnion(Union):
    _fields_ = [('x', c_int32), ('y', c_int8), ('z', c_uint16)]


@register()
class ManyTypes(Structure):
    _fields_ = [
        ('i8', c_int8), ('u8', c_uint8),
        ('i16', c_int16), ('u16', c_uint16),
        ('i32', c_int32), ('u32', c_uint32),
        ('i64', c_int64), ('u64', c_uint64),
    ]


@register()
class ManyTypesU(Union):
    _fields_ = [
        ('i8', c_int8), ('u8', c_uint8),
        ('i16', c_int16), ('u16', c_uint16),
        ('i32', c_int32), ('u32', c_uint32),
        ('i64', c_int64), ('u64', c_uint64),
    ]


@register()
class Nested(Structure):
    _fields_ = [
        ('a', SimpleStruct), ('b', SimpleUnion), ('anon', SimpleStruct),
    ]
    _anonymous_ = ['anon']


@register()
class Packed1(Structure):
    _fields_ = [('a', c_int8), ('b', c_int64)]
    _pack_ = 1


@register()
class Packed2(Structure):
    _fields_ = [('a', c_int8), ('b', c_int64)]
    _pack_ = 2


@register()
class Packed3(Structure):
    _fields_ = [('a', c_int8), ('b', c_int64)]
    _pack_ = 4


@register()
class Packed4(Structure):
    _fields_ = [('a', c_int8), ('b', c_int64)]
    _pack_ = 8

@register()
class IntBits(Structure):
    _fields_ = [("A", c_int, 1),
                ("B", c_int, 2),
                ("C", c_int, 3),
                ("D", c_int, 4),
                ("E", c_int, 5),
                ("F", c_int, 6),
                ("G", c_int, 7),
                ("H", c_int, 8),
                ("I", c_int, 9)]

@register()
class Bits(Structure):
    _fields_ = [*IntBits._fields_,

                ("M", c_short, 1),
                ("N", c_short, 2),
                ("O", c_short, 3),
                ("P", c_short, 4),
                ("Q", c_short, 5),
                ("R", c_short, 6),
                ("S", c_short, 7)]

@register()
class IntBits_MSVC(Structure):
    _layout_ = "ms"
    _fields_ = [("A", c_int, 1),
                ("B", c_int, 2),
                ("C", c_int, 3),
                ("D", c_int, 4),
                ("E", c_int, 5),
                ("F", c_int, 6),
                ("G", c_int, 7),
                ("H", c_int, 8),
                ("I", c_int, 9)]

@register()
class Bits_MSVC(Structure):
    _layout_ = "ms"
    _fields_ = [*IntBits_MSVC._fields_,

                ("M", c_short, 1),
                ("N", c_short, 2),
                ("O", c_short, 3),
                ("P", c_short, 4),
                ("Q", c_short, 5),
                ("R", c_short, 6),
                ("S", c_short, 7)]

# Skipped for now -- we don't always match the alignment
#@register()
class IntBits_Union(Union):
    _fields_ = [("A", c_int, 1),
                ("B", c_int, 2),
                ("C", c_int, 3),
                ("D", c_int, 4),
                ("E", c_int, 5),
                ("F", c_int, 6),
                ("G", c_int, 7),
                ("H", c_int, 8),
                ("I", c_int, 9)]

# Skipped for now -- we don't always match the alignment
#@register()
class BitsUnion(Union):
    _fields_ = [*IntBits_Union._fields_,

                ("M", c_short, 1),
                ("N", c_short, 2),
                ("O", c_short, 3),
                ("P", c_short, 4),
                ("Q", c_short, 5),
                ("R", c_short, 6),
                ("S", c_short, 7)]

@register()
class I64Bits(Structure):
    _fields_ = [("a", c_int64, 1),
                ("b", c_int64, 62),
                ("c", c_int64, 1)]

@register()
class U64Bits(Structure):
    _fields_ = [("a", c_uint64, 1),
                ("b", c_uint64, 62),
                ("c", c_uint64, 1)]

for n in 8, 16, 32, 64:
    for signedness in '', 'u':
        ctype = globals()[f'c_{signedness}int{n}']

        @register(f'Struct331_{signedness}{n}')
        class _cls(Structure):
            _fields_ = [("a", ctype, 3),
                        ("b", ctype, 3),
                        ("c", ctype, 1)]

        @register(f'Struct1x1_{signedness}{n}')
        class _cls(Structure):
            _fields_ = [("a", ctype, 1),
                        ("b", ctype, n-2),
                        ("c", ctype, 1)]

        @register(f'Struct1nx1_{signedness}{n}')
        class _cls(Structure):
            _fields_ = [("a", ctype, 1),
                        ("full", ctype),
                        ("b", ctype, n-2),
                        ("c", ctype, 1)]

        @register(f'Struct3xx_{signedness}{n}')
        class _cls(Structure):
            _fields_ = [("a", ctype, 3),
                        ("b", ctype, n-2),
                        ("c", ctype, n-2)]

@register()
class Mixed1(Structure):
    _fields_ = [("a", c_byte, 4),
                ("b", c_int, 4)]

@register()
class Mixed2(Structure):
    _fields_ = [("a", c_byte, 4),
                ("b", c_int32, 32)]

@register()
class Mixed3(Structure):
    _fields_ = [("a", c_byte, 4),
                ("b", c_ubyte, 4)]

@register()
class Mixed4(Structure):
    _fields_ = [("a", c_short, 4),
                ("b", c_short, 4),
                ("c", c_int, 24),
                ("d", c_short, 4),
                ("e", c_short, 4),
                ("f", c_int, 24)]

@register()
class Mixed5(Structure):
    _fields_ = [('A', c_uint, 1),
                ('B', c_ushort, 16)]

@register()
class Mixed6(Structure):
    _fields_ = [('A', c_ulonglong, 1),
                ('B', c_uint, 32)]

@register()
class Mixed7(Structure):
    _fields_ = [("A", c_uint32),
                ('B', c_uint32, 20),
                ('C', c_uint64, 24)]

@register()
class Mixed8_a(Structure):
    _fields_ = [("A", c_uint32),
                ("B", c_uint32, 32),
                ("C", c_ulonglong, 1)]

@register()
class Mixed8_b(Structure):
    _fields_ = [("A", c_uint32),
                ("B", c_uint32),
                ("C", c_ulonglong, 1)]

@register()
class Mixed9(Structure):
    _fields_ = [("A", c_uint8),
                ("B", c_uint32, 1)]

@register()
class Mixed10(Structure):
    _fields_ = [("A", c_uint32, 1),
                ("B", c_uint64, 1)]

@register()
class Example_gh_95496(Structure):
    _fields_ = [("A", c_uint32, 1),
                ("B", c_uint64, 1)]

@register()
class Example_gh_84039_bad(Structure):
    _pack_ = 1
    _fields_ = [("a0", c_uint8, 1),
                ("a1", c_uint8, 1),
                ("a2", c_uint8, 1),
                ("a3", c_uint8, 1),
                ("a4", c_uint8, 1),
                ("a5", c_uint8, 1),
                ("a6", c_uint8, 1),
                ("a7", c_uint8, 1),
                ("b0", c_uint16, 4),
                ("b1", c_uint16, 12)]

@register()
class Example_gh_84039_good_a(Structure):
    _pack_ = 1
    _fields_ = [("a0", c_uint8, 1),
                ("a1", c_uint8, 1),
                ("a2", c_uint8, 1),
                ("a3", c_uint8, 1),
                ("a4", c_uint8, 1),
                ("a5", c_uint8, 1),
                ("a6", c_uint8, 1),
                ("a7", c_uint8, 1)]

@register()
class Example_gh_84039_good(Structure):
    _pack_ = 1
    _fields_ = [("a", Example_gh_84039_good_a),
                ("b0", c_uint16, 4),
                ("b1", c_uint16, 12)]

@register()
class Example_gh_73939(Structure):
    _pack_ = 1
    _fields_ = [("P", c_uint16),
                ("L", c_uint16, 9),
                ("Pro", c_uint16, 1),
                ("G", c_uint16, 1),
                ("IB", c_uint16, 1),
                ("IR", c_uint16, 1),
                ("R", c_uint16, 3),
                ("T", c_uint32, 10),
                ("C", c_uint32, 20),
                ("R2", c_uint32, 2)]

@register()
class Example_gh_86098(Structure):
    _fields_ = [("a", c_uint8, 8),
                ("b", c_uint8, 8),
                ("c", c_uint32, 16)]

@register()
class Example_gh_86098_pack(Structure):
    _pack_ = 1
    _fields_ = [("a", c_uint8, 8),
                ("b", c_uint8, 8),
                ("c", c_uint32, 16)]

@register()
class AnonBitfields(Structure):
    class X(Structure):
        _fields_ = [("a", c_byte, 4),
                    ("b", c_ubyte, 4)]
    _anonymous_ = ["_"]
    _fields_ = [("_", X), ('y', c_byte)]


class GeneratedTest(unittest.TestCase):
    def test_generated_data(self):
        for name, cls in TESTCASES.items():
            with self.subTest(name=name):
                expected = iter(_ctypes_test.get_generated_test_data(name))
                expected_name = next(expected)
                if expected_name is None:
                    unittest.skipTest(next(expected))
                self.assertEqual(name, expected_name)
                self.assertEqual(sizeof(cls), next(expected))
                with self.subTest('alignment'):
                    self.assertEqual(alignment(cls), next(expected))
                obj = cls()
                ptr = pointer(obj)
                for field in iterfields(cls):
                    for value in -1, 1, 0:
                        with self.subTest(field=field.full_name, value=value):
                            field.set_to(obj, value)
                            mem = string_at(ptr, sizeof(obj))
                            self.assertEqual(mem, next(expected))


# The rest of this file is generating C code from a ctypes type.
# This is only tested with the known inputs here.

def c_str_repr(string):
    return '"' + re.sub('([\"\'\\\\\n])', r'\\\1', string) + '"'

def dump_simple_ctype(tp, variable_name='', semi=''):
    length = getattr(tp, '_length_', None)
    if length is not None:
        return f'{dump_simple_ctype(tp._type_, variable_name)}[{length}]{semi}'
    assert not issubclass(tp, (Structure, Union))
    return f'{tp._c_name}{maybe_space(variable_name)}{semi}'


def dump_ctype(tp, agg_name='', variable_name='', semi=''):
    requires = set()
    if issubclass(tp, (Structure, Union)):
        attributes = []
        pushes = []
        pops = []
        pack = getattr(tp, '_pack_', None)
        if pack is not None:
            requires.add(KNOWN_COMPILERS)
            pushes.append(f'#pragma pack(push, {pack})')
            pops.append(f'#pragma pack(pop)')
        layout = getattr(tp, '_layout_', None)
        if layout == 'ms' or pack:
            requires.add(KNOWN_COMPILERS)
            attributes.append('ms_struct')
        if attributes:
            a = f' GCC_ATTR({", ".join(attributes)})'
        else:
            a = ''
        lines = [f'{struct_or_union(tp)}{a}{maybe_space(agg_name)} ' +'{']
        for fielddesc in tp._fields_:
            f_name, f_tp, f_bits = unpack_field_desc(fielddesc)
            if f_name in getattr(tp, '_anonymous_', ()):
                f_name = ''
            if f_bits is None:
                subsemi = ';'
            else:
                if f_tp not in (c_int, c_uint):
                    # XLC can reportedly only handle int & unsigned int
                    # bitfields (the only types required by C spec)
                    requires.add('!defined(__xlc__)')
                subsemi = f' :{f_bits};'
            sub_lines, sub_requires = dump_ctype(
                f_tp, variable_name=f_name, semi=subsemi)
            requires.update(sub_requires)
            for line in sub_lines:
                lines.append('    ' + line)
        lines.append(f'}}{maybe_space(variable_name)}{semi}')
        return [*pushes, *lines, *reversed(pops)], requires
    else:
        return [dump_simple_ctype(tp, variable_name, semi)], requires

def struct_or_union(cls):
    if issubclass(cls, Structure):
         return 'struct'
    if issubclass(cls, Union):
        return 'union'
    raise TypeError(cls)

def maybe_space(string):
    if string:
        return ' ' + string
    return string

def unpack_field_desc(fielddesc):
    try:
        f_name, f_tp, f_bits = fielddesc
        return f_name, f_tp, f_bits
    except ValueError:
        f_name, f_tp = fielddesc
        return f_name, f_tp, None

@dataclass
class FieldInfo:
    name: str
    tp: type
    bits: int | None
    parent_type: type
    parent: 'FieldInfo' #| None

    @cached_property
    def attr_path(self):
        if self.name in getattr(self.parent_type, '_anonymous_', ()):
            selfpath = ()
        else:
            selfpath = (self.name,)
        if self.parent:
            return (*self.parent.attr_path, *selfpath)
        else:
            return selfpath

    @cached_property
    def full_name(self):
        return '.'.join(self.attr_path)

    def set_to(self, obj, new):
        for attr_name in self.attr_path[:-1]:
            obj = getattr(obj, attr_name)
        setattr(obj, self.attr_path[-1], new)


def iterfields(tp, parent=None):
    try:
        fields = tp._fields_
    except AttributeError:
        yield parent
    else:
        for fielddesc in fields:
            f_name, f_tp, f_bits = unpack_field_desc(fielddesc)
            sub = FieldInfo(f_name, f_tp, f_bits, tp, parent)
            yield from iterfields(f_tp, sub)


if __name__ == '__main__':
    print('/* Generated by Lib/test/test_ctypes/test_generated_structs.py */')
    print("""
        // Append VALUE to the result.
        #define APPEND(VAL) {                           \\
            if (!VAL) {                                 \\
                Py_DECREF(result);                      \\
                return NULL;                            \\
            }                                           \\
            int rv = PyList_Append(result, VAL);        \\
            Py_DECREF(VAL);                             \\
            if (rv < 0) {                               \\
                Py_DECREF(result);                      \\
                return NULL;                            \\
            }                                           \\
        }

        // Set TARGET, and append a snapshot of `value`'s
        // memory to the result.
        #define SET_AND_APPEND(TYPE, TARGET, VAL) {     \\
            TYPE v = VAL;                               \\
            TARGET = v;                                 \\
            APPEND(PyBytes_FromStringAndSize(           \\
                (char*)&value, sizeof(value)));         \\
        }

        // Set a field to -1, 1 and 0; append a snapshot of the memory
        // after each of the operations.
        #define TEST_FIELD(TYPE, TARGET) {              \\
            SET_AND_APPEND(TYPE, TARGET, -1)            \\
            SET_AND_APPEND(TYPE, TARGET, 1)             \\
            SET_AND_APPEND(TYPE, TARGET, 0)             \\
        }

        #if defined(__GNUC__) || defined(__clang__)
        #define GCC_ATTR(X) __attribute__((X))
        #else
        #define GCC_ATTR(X) /* */
        #endif

        static PyObject *
        get_generated_test_data(PyObject *self, PyObject *name)
        {
            if (!PyUnicode_Check(name)) {
                PyErr_SetString(PyExc_TypeError, "need a string");
                return NULL;
            }
            PyObject *result = PyList_New(0);
            if (!result) {
                return NULL;
            }
    """)
    for name, cls in TESTCASES.items():
        print("""
            if (_PyUnicode_EqualToASCIIString(name, %s)) {
            """ % c_str_repr(name))
        lines, requires = dump_ctype(cls, agg_name=name, semi=';')
        if requires:
            print(f"""
            #if {" && ".join(f'({r})' for r in sorted(requires))}
            """)
        for line in lines:
            print('                ', line, sep='')
        typename = f'{struct_or_union(cls)} {name}'
        print(f"""
                {typename} value = {{0}};
                APPEND(PyUnicode_FromString({c_str_repr(name)}));
                APPEND(PyLong_FromLong(sizeof({typename})));
                APPEND(PyLong_FromLong(_Alignof({typename})));
        """.rstrip())
        for field in iterfields(cls):
            f_tp = dump_simple_ctype(field.tp)
            print(f"""\
                TEST_FIELD({f_tp}, value.{field.full_name});
            """.rstrip())
        if requires:
            print(f"""
            #else
                APPEND(Py_NewRef(Py_None));
                APPEND(PyUnicode_FromString("skipped on this compiler"));
            #endif
            """)
        print("""
                return result;
            }
        """)

    print("""
            Py_DECREF(result);
            PyErr_Format(PyExc_ValueError, "unknown testcase %R", name);
            return NULL;
        }

        #undef GCC_ATTR
        #undef TEST_FIELD
        #undef SET_AND_APPEND
        #undef APPEND
    """)

