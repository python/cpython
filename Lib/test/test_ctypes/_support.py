# Some classes and types are not export to _ctypes module directly.

import ctypes
from _ctypes import Structure, Union, _Pointer, Array, _SimpleCData, CFuncPtr


_CData = Structure.__base__
assert _CData.__name__ == "_CData"

# metaclasses
PyCStructType = type(Structure)
UnionType = type(Union)
PyCPointerType = type(_Pointer)
PyCArrayType = type(Array)
PyCSimpleType = type(_SimpleCData)
PyCFuncPtrType = type(CFuncPtr)

# type flags
Py_TPFLAGS_DISALLOW_INSTANTIATION = 1 << 7
Py_TPFLAGS_IMMUTABLETYPE = 1 << 8


class StructCheckMixin:
    def check_struct(self, structure):
        """Assert that a structure is well-formed"""
        self._check_struct_or_union(structure, is_struct=True)

    def check_union(self, union):
        """Assert that a union is well-formed"""
        self._check_struct_or_union(union, is_struct=False)

    def check_struct_or_union(self, cls):
        if issubclass(cls, Structure):
            self._check_struct_or_union(cls, is_struct=True)
        elif issubclass(cls, Union):
            self._check_struct_or_union(cls, is_struct=False)
        else:
            raise TypeError(cls)

    def _check_struct_or_union(self, cls, is_struct):

        # Check that fields are not overlapping (for structs),
        # and that their metadata is consistent.

        # offset of the last checked bit, from start of struct
        # stays 0 for unions
        next_bit = 0

        anon_names = getattr(cls, '_anonymous_', ())
        cls_size = ctypes.sizeof(cls)
        for name, requested_type, *rest_of_tuple in cls._fields_:
            field = getattr(cls, name)
            try:
                is_bitfield = len(rest_of_tuple) > 0

                # name
                self.assertEqual(field.name, name)

                # type
                self.assertEqual(field.type, requested_type)

                # offset === byte_offset
                self.assertEqual(field.byte_offset, field.offset)
                if not is_struct:
                    self.assertEqual(field.byte_offset, 0)

                # byte_size
                self.assertEqual(field.byte_size, ctypes.sizeof(field.type))
                self.assertGreaterEqual(field.byte_size, 0)
                self.assertLessEqual(field.byte_offset + field.byte_size,
                                     cls_size)

                # size
                self.assertGreaterEqual(field.size, 0)
                if is_bitfield:
                    # size has backwards-compatible bit-packed info
                    if hasattr(cls, '_swappedbytes_'):
                        offset_for_size = (8 * field.byte_size
                                           - field.bit_offset
                                           - field.bit_size)
                    else:
                        offset_for_size = field.bit_offset
                    expected_size = (field.bit_size << 16) + offset_for_size
                    self.assertEqual(field.size, expected_size)
                else:
                    # size == byte_size
                    self.assertEqual(field.size, field.byte_size)

                # is_bitfield (bool)
                self.assertIs(field.is_bitfield, is_bitfield)

                # bit_offset
                if is_bitfield:
                    self.assertGreaterEqual(field.bit_offset, 0)
                    self.assertLessEqual(field.bit_offset + field.bit_size,
                                         field.byte_size * 8)
                else:
                    self.assertEqual(field.bit_offset, 0)
                if not is_struct:
                    self.assertEqual(field.bit_offset, 0)

                # bit_size
                if is_bitfield:
                    self.assertGreaterEqual(field.bit_size, 0)
                    self.assertLessEqual(field.bit_size, field.byte_size * 8)
                    [requested_bit_size] = rest_of_tuple
                    self.assertEqual(field.bit_size, requested_bit_size)
                else:
                    self.assertEqual(field.bit_size, field.byte_size * 8)

                # is_anonymous (bool)
                self.assertIs(field.is_anonymous, name in anon_names)

                # field is not overlapping earlier members in a struct.
                # (this assumes fields are laid out in order)
                self.assertGreaterEqual(
                    field.byte_offset * 8 + field.bit_offset,
                    next_bit)
                next_bit = (field.byte_offset * 8
                            + field.bit_offset
                            + field.bit_size)
                # field is inside cls
                self.assertLessEqual(next_bit, cls_size * 8)

                if not is_struct:
                    # union fields may overlap
                    next_bit = 0
            except Exception as e:
                # Mention the failing field in the traceback
                # (`self.subTest` would be nicer, but it doesn't nest well)
                e.add_note(f'while checking field {name!r}: {field}')
                raise
