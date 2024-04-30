from textwrap import dedent
from operator import attrgetter
from dataclasses import dataclass
from functools import cached_property

import ctypes
import _ctypes

print((ctypes.c_bool).mro())
print(dir(ctypes.POINTER(ctypes.c_int8)))
print(ctypes.POINTER(ctypes.c_int8)._type_)

def dump_ctype(tp, name='%'):
    length = getattr(tp, '_length_', None)
    if length is not None:
        return f'{dump_ctype(tp._type_, name)}[{length}]'
    if tp == ctypes.c_char_p:
        return f'char *{name}'
    if tp == ctypes.c_wchar_p:
        return f'wchar *{name}'
    if tp == ctypes.c_wchar:
        return f'wchar {name}'
    if tp == ctypes.c_void_p:
        return f'void *{name}'
    if tp == ctypes.c_bool:
        _space = ' ' if name else ''
        return f'bool{_space}{name}'
    if tp == ctypes.py_object:
        return f'PyObject *{name}'
    if issubclass(tp, _ctypes._Pointer):
        return f'{dump_ctype(tp._type_, '*' + name)}'
    if issubclass(tp, (ctypes.Structure, ctypes.Union)):
        if issubclass(tp, ctypes.Structure):
            lines = ['struct {']
        else:
            lines = ['union {']
        for fielddesc in tp._fields_:
            f_name, f_tp, f_bits = unpack_field_desc(fielddesc)
            if f_name in getattr(tp, '_anonymous_', ()):
                f_name = ''
            for line in dump_ctype(f_tp, f_name).splitlines():
                lines.append('    ' + line)
            if f_bits is None:
                lines[-1] += ';'
            else:
                lines[-1] += f' :{f_bits};'
        _space = ' ' if name else ''
        lines.append(f'}}{_space}{name}')
        return '\n'.join(lines)
    if issubclass(tp, ctypes._SimpleCData):
        if tp(-1).value == -1:
            sign = ''
        else:
            sign = 'u'
        size = ctypes.sizeof(tp)
        _space = ' ' if name else ''
        return f'{sign}int{size * 8}_t{_space}{name}'
    restype = getattr(tp, '_restype_', None)
    if restype is not None:
        return f'{dump_ctype(restype, "")} (*{name})({", ".join(dump_ctype(t, "") for t in tp._argtypes_)})'

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
    def field_descriptor(self):
        return getattr(self.parent_type, self.name)

    @cached_property
    def offset(self):
        offset = self.field_descriptor.offset
        if self.parent:
            offset += self.parent.offset
        return offset

    @cached_property
    def field_path(self):
        if self.parent:
            return (*self.parent.field_path, self.name)
        return (self.name,)

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

    @cached_property
    def is_in_union(self):
        if self.parent and self.parent.is_in_union:
            return True
        return issubclass(self.parent_type, ctypes.Union)


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


class Uni(ctypes.Union):
    _fields_ = [
        ('u', ctypes.c_int8),
        ('v', ctypes.c_int16, 9),
        ('w', ctypes.c_int16, 3),
    ]

class Sub(ctypes.Structure):
    _fields_ = [
        ('x', ctypes.c_int8),
        ('y', ctypes.c_int8, 3),
        ('z', ctypes.c_int16, 9),
    ]

class S(ctypes.Structure):
    _fields_ = [
        ('a', ctypes.c_int8),
        ('b', ctypes.c_int8, 3),
        ('c', Sub),
        ('anon', Sub),
        ('d', Uni),
    ]
    _anonymous_ = ['anon']

TESTCASES = (
    (ctypes.c_int8, 'int8_t %'),
    (ctypes.c_uint8, 'uint8_t %'),
    (ctypes.c_int16, 'int16_t %'),
    (ctypes.c_uint16, 'uint16_t %'),
    (ctypes.c_int32, 'int32_t %'),
    (ctypes.c_uint32, 'uint32_t %'),
    (ctypes.c_int64, 'int64_t %'),
    (ctypes.c_uint64, 'uint64_t %'),
    (ctypes.c_int8 * 8, 'int8_t %[8]'),
    (ctypes.CFUNCTYPE(ctypes.c_int8, ctypes.c_uint16, ctypes.c_uint32), 'int8_t (*%)(uint16_t, uint32_t)'),
    (ctypes.c_char_p, 'char *%'),
    (ctypes.POINTER(ctypes.c_int8), 'int8_t *%'),
    (ctypes.POINTER(ctypes.POINTER(ctypes.c_int8)), 'int8_t **%'),
    (ctypes.c_void_p, 'void *%'),
    (ctypes.c_wchar_p, 'wchar *%'),
    (ctypes.c_wchar, 'wchar %'),
    (ctypes.c_bool, 'bool %'),
    (ctypes.py_object, 'PyObject *%'),
    (S, dedent("""
        struct {
            int8_t a;
            int8_t b :3;
            struct {
                int8_t x;
                int8_t y :3;
                int16_t z :9;
            } c;
            struct {
                int8_t x;
                int8_t y :3;
                int16_t z :9;
            };
            union {
                int8_t u;
                int16_t v :9;
                int16_t w :3;
            } d;
        } %
    """).strip()),
)

for ctype, expected_name in TESTCASES:
    print(ctype, expected_name, '...')
    got_name = dump_ctype(ctype)
    print(ctype, got_name, '!!!')
    if expected_name != got_name:
        raise AssertionError()

def getfieldtype(tp, attrnames):
    match attrnames:
        case [name]:
            return getattr(tp, name)
        case [name, *rest]:
            return getfieldtype(getattr(tp, name).type, rest)
        case _:
            raise AttributeError(attrnames)

print(S.x)
print(dir(S.x))
s = S()
expected_names = ['a', 'b', 'c.x', 'c.y', 'c.z', 'x', 'y', 'z', 'd.u', 'd.v', 'd.w']
for got_info, expected_name in zip(iterfields(S), expected_names, strict=True):
    print(got_info, expected_name, got_info.offset)
    assert attrgetter(expected_name)(s) == 0
    print(got_info.full_name)
    print(got_info.attr_path)
    print(got_info.field_path)
    print(got_info.field_descriptor)
    print(got_info.offset)
    print(got_info.is_in_union)
    assert got_info.full_name == expected_name

print(dir(ctypes))
print(type(ctypes.c_int))
print(dir(type(ctypes.c_int)))
print(ctypes.sizeof(ctypes.c_int))
print(dir(ctypes.c_int))
print(dir(ctypes.c_int*8))
print((ctypes.c_int*8)._length_)
print((ctypes.c_int*8)._type_)
print(dir(ctypes.CFUNCTYPE(ctypes.c_int8, ctypes.c_uint16, ctypes.c_uint32)))
print(dir(ctypes.PYFUNCTYPE(ctypes.c_int8, ctypes.c_uint16, ctypes.c_uint32)))
print(dir(ctypes.c_char_p))
print(ctypes.c_char_p)
print(ctypes.POINTER(ctypes.c_int8))
print(dir(ctypes.POINTER(ctypes.c_int8)))
print(ctypes.POINTER(ctypes.c_int8)._type_)
