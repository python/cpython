"""Mach-O section lookup with synthetic executable images."""

import struct
import sys
import unittest

from test.support.import_helper import import_module

_testinternalcapi = import_module('_testinternalcapi')


@unittest.skipUnless(sys.platform == 'darwin', 'requires Mach-O headers')
class MachOSectionTests(unittest.TestCase):
    BASE = 0x20000000
    OFFSET = 0x2345

    def lookup(self, image, *, base=BASE, name='PyRuntime'):
        return _testinternalcapi.get_macho_section_address(bytes(image), base, name)

    def image(self, *, wide=True, endian='<', data_first=False, name=b'PyRuntime'):
        # The preferred VM address differs from the load address.
        address = 0x100000000 if wide else 0x1000
        segment_format = endian + ('II16sQQQQiiII' if wide else 'II16sIIIIiiII')
        section_format = endian + ('16s16sQQIIIIIIII' if wide else '16s16sIIIIIIIII')
        segment_size = struct.calcsize(segment_format)
        section_size = struct.calcsize(section_format)
        kind = 0x19 if wide else 1  # LC_SEGMENT_64 / LC_SEGMENT
        text = struct.pack(segment_format, kind, segment_size, b'__TEXT',
                           address, 0x1000, 0, 0x1000, 7, 5, 0, 0)
        data = struct.pack(segment_format, kind, segment_size + section_size,
                           b'__DATA', address + 0x2000, 0x1000,
                           0x1000, 0x1000, 7, 3, 1, 0)
        fields = [name, b'__DATA', address + self.OFFSET, 0x100,
                  0, 0, 0, 0, 0, 0, 0]
        if wide:
            fields.append(0)
        data += struct.pack(section_format, *fields)
        uuid = struct.pack(endian + 'II16s', 0x1b, 24, bytes(16))
        commands = data + uuid + text if data_first else text + uuid + data
        header = [0xfeedfacf if wide else 0xfeedface,
                  0x1000007 if wide else 7, 3, 2, 3, len(commands), 0]
        if wide:
            header.append(0)
        return struct.pack(endian + 'I' * len(header), *header) + commands

    def test_thin_images(self):
        for wide in (False, True):
            for endian in ('<', '>'):
                for data_first in (False, True):
                    with self.subTest(wide=wide, endian=endian, data_first=data_first):
                        image = self.image(wide=wide, endian=endian, data_first=data_first)
                        self.assertEqual(self.lookup(image), self.BASE + self.OFFSET)

    def test_missing_section(self):
        self.assertEqual(self.lookup(self.image(), name='AsyncioDebug'), 0)

    def test_fixed_width_section_name(self):
        name = b'0123456789abcdef'
        self.assertEqual(self.lookup(self.image(name=name), name=name.decode()),
                         self.BASE + self.OFFSET)
        self.assertEqual(self.lookup(self.image(name=name), name=name.decode() + 'x'), 0)

    def test_truncated_images(self):
        for image in (self.image(), self.image(wide=False)):
            for size in range(len(image)):
                with self.subTest(length=len(image), size=size):
                    with self.assertRaises(RuntimeError):
                        self.lookup(image[:size])

    def test_invalid_commands(self):
        # The first 64-bit segment command follows the 32-byte header.
        for field, value in ((16, 1000),       # ncmds
                             (20, 0xffffffff),  # sizeofcmds
                             (36, 0),        # cmdsize
                             (36, 8),        # shorter than segment header
                             (36, 0xffffffff),
                             (96, 0xffffffff)):  # nsects
            with self.subTest(field=field, value=value):
                image = bytearray(self.image())
                struct.pack_into('<I', image, field, value)
                with self.assertRaises(RuntimeError):
                    self.lookup(image)

    def test_address_overflow(self):
        with self.assertRaises(RuntimeError):
            self.lookup(self.image(), base=(1 << (8 * struct.calcsize('P'))) - 1)

    def test_missing_header_segment(self):
        image = bytearray(self.image())
        struct.pack_into('<Q', image, 80, 0)  # __TEXT.filesize
        with self.assertRaises(RuntimeError):
            self.lookup(image)


@unittest.skipUnless(sys.platform == 'darwin', 'requires dyld')
class DyldImageTests(unittest.TestCase):
    def test_memory_region_fallback(self):
        try:
            _testinternalcapi.check_macho_runtime_address()
        except PermissionError as exc:
            self.skipTest(f"self-inspection is unavailable: {exc}")

    def test_loaded_executable(self):
        ctypes = import_module('ctypes')
        get_header = ctypes.CDLL(None)._dyld_get_image_header
        get_header.argtypes = [ctypes.c_uint32]
        get_header.restype = ctypes.c_void_p
        expected = get_header(0)
        self.assertIsNotNone(expected)
        try:
            images = _testinternalcapi.get_dyld_image_addresses()
        except PermissionError as exc:
            self.skipTest(f"self-inspection is unavailable: {exc}")
        self.assertIn(expected, images)


if __name__ == '__main__':
    unittest.main()
