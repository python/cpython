import builtins
import contextlib
import copy
import enum
import io
import os
import pickle
import random
import sys
import unittest
import weakref
from itertools import product
from unittest import mock

from test import support
from test.support import force_not_colorized_test_class, import_helper, warnings_helper
from test.support.script_helper import assert_python_ok

py_uuid = import_helper.import_fresh_module('uuid', blocked=['_uuid'])
c_uuid = import_helper.import_fresh_module('uuid', fresh=['_uuid'])

def importable(name):
    try:
        __import__(name)
        return True
    except ModuleNotFoundError:
        return False


def mock_get_command_stdout(data):
    def get_command_stdout(command, args):
        return io.BytesIO(data.encode())
    return get_command_stdout


class BaseTestUUID:
    uuid = None

    def test_nil_uuid(self):
        nil_uuid = self.uuid.NIL

        s = '00000000-0000-0000-0000-000000000000'
        i = 0
        self.assertEqual(nil_uuid, self.uuid.UUID(s))
        self.assertEqual(nil_uuid, self.uuid.UUID(int=i))
        self.assertEqual(nil_uuid.int, i)
        self.assertEqual(str(nil_uuid), s)
        # The Nil UUID falls within the range of the Apollo NCS variant as per
        # RFC 9562.
        # See https://www.rfc-editor.org/rfc/rfc9562.html#section-5.9-4
        self.assertEqual(nil_uuid.variant, self.uuid.RESERVED_NCS)
        # A version field of all zeros is "Unused" in RFC 9562, but the version
        # field also only applies to the 10xx variant, i.e. the variant
        # specified in RFC 9562. As such, because the Nil UUID falls under a
        # different variant, its version is considered undefined.
        # See https://www.rfc-editor.org/rfc/rfc9562.html#table2
        self.assertIsNone(nil_uuid.version)

    def test_max_uuid(self):
        max_uuid = self.uuid.MAX

        s = 'ffffffff-ffff-ffff-ffff-ffffffffffff'
        i = (1 << 128) - 1
        self.assertEqual(max_uuid, self.uuid.UUID(s))
        self.assertEqual(max_uuid, self.uuid.UUID(int=i))
        self.assertEqual(max_uuid.int, i)
        self.assertEqual(str(max_uuid), s)
        # The Max UUID falls within the range of the "yet-to-be defined" future
        # UUID variant as per RFC 9562.
        # See https://www.rfc-editor.org/rfc/rfc9562.html#section-5.10-4
        self.assertEqual(max_uuid.variant, self.uuid.RESERVED_FUTURE)
        # A version field of all ones is "Reserved for future definition" in
        # RFC 9562, but the version field also only applies to the 10xx
        # variant, i.e. the variant specified in RFC 9562. As such, because the
        # Max UUID falls under a different variant, its version is considered
        # undefined.
        # See https://www.rfc-editor.org/rfc/rfc9562.html#table2
        self.assertIsNone(max_uuid.version)

    def test_safe_uuid_enum(self):
        class CheckedSafeUUID(enum.Enum):
            safe = 0
            unsafe = -1
            unknown = None
        enum._test_simple_enum(CheckedSafeUUID, py_uuid.SafeUUID)

    def test_UUID(self):
        equal = self.assertEqual
        ascending = []
        for (string, curly, hex, bytes, bytes_le, fields, integer, urn,
             time, clock_seq, variant, version) in [
            ('00000000-0000-0000-0000-000000000000',
             '{00000000-0000-0000-0000-000000000000}',
             '00000000000000000000000000000000',
             b'\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
             b'\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
             (0, 0, 0, 0, 0, 0),
             0,
             'urn:uuid:00000000-0000-0000-0000-000000000000',
             0, 0, self.uuid.RESERVED_NCS, None),
            ('00010203-0405-0607-0809-0a0b0c0d0e0f',
             '{00010203-0405-0607-0809-0a0b0c0d0e0f}',
             '000102030405060708090a0b0c0d0e0f',
             b'\0\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\x0d\x0e\x0f',
             b'\x03\x02\x01\0\x05\x04\x07\x06\x08\t\n\x0b\x0c\x0d\x0e\x0f',
             (0x00010203, 0x0405, 0x0607, 8, 9, 0x0a0b0c0d0e0f),
             0x000102030405060708090a0b0c0d0e0f,
             'urn:uuid:00010203-0405-0607-0809-0a0b0c0d0e0f',
             0x607040500010203, 0x809, self.uuid.RESERVED_NCS, None),
            ('02d9e6d5-9467-382e-8f9b-9300a64ac3cd',
             '{02d9e6d5-9467-382e-8f9b-9300a64ac3cd}',
             '02d9e6d59467382e8f9b9300a64ac3cd',
             b'\x02\xd9\xe6\xd5\x94\x67\x38\x2e\x8f\x9b\x93\x00\xa6\x4a\xc3\xcd',
             b'\xd5\xe6\xd9\x02\x67\x94\x2e\x38\x8f\x9b\x93\x00\xa6\x4a\xc3\xcd',
             (0x02d9e6d5, 0x9467, 0x382e, 0x8f, 0x9b, 0x9300a64ac3cd),
             0x02d9e6d59467382e8f9b9300a64ac3cd,
             'urn:uuid:02d9e6d5-9467-382e-8f9b-9300a64ac3cd',
             0x82e946702d9e6d5, 0xf9b, self.uuid.RFC_4122, 3),
            ('12345678-1234-5678-1234-567812345678',
             '{12345678-1234-5678-1234-567812345678}',
             '12345678123456781234567812345678',
             b'\x12\x34\x56\x78'*4,
             b'\x78\x56\x34\x12\x34\x12\x78\x56\x12\x34\x56\x78\x12\x34\x56\x78',
             (0x12345678, 0x1234, 0x5678, 0x12, 0x34, 0x567812345678),
             0x12345678123456781234567812345678,
             'urn:uuid:12345678-1234-5678-1234-567812345678',
             0x678123412345678, 0x1234, self.uuid.RESERVED_NCS, None),
            ('6ba7b810-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b810-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8109dad11d180b400c04fd430c8',
             b'\x6b\xa7\xb8\x10\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             b'\x10\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b810, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8),
             0x6ba7b8109dad11d180b400c04fd430c8,
             'urn:uuid:6ba7b810-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b810, 0xb4, self.uuid.RFC_4122, 1),
            ('6ba7b811-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b811-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8119dad11d180b400c04fd430c8',
             b'\x6b\xa7\xb8\x11\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             b'\x11\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b811, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8),
             0x6ba7b8119dad11d180b400c04fd430c8,
             'urn:uuid:6ba7b811-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b811, 0xb4, self.uuid.RFC_4122, 1),
            ('6ba7b812-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b812-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8129dad11d180b400c04fd430c8',
             b'\x6b\xa7\xb8\x12\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             b'\x12\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b812, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8),
             0x6ba7b8129dad11d180b400c04fd430c8,
             'urn:uuid:6ba7b812-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b812, 0xb4, self.uuid.RFC_4122, 1),
            ('6ba7b814-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b814-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8149dad11d180b400c04fd430c8',
             b'\x6b\xa7\xb8\x14\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             b'\x14\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b814, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8),
             0x6ba7b8149dad11d180b400c04fd430c8,
             'urn:uuid:6ba7b814-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b814, 0xb4, self.uuid.RFC_4122, 1),
            ('7d444840-9dc0-11d1-b245-5ffdce74fad2',
             '{7d444840-9dc0-11d1-b245-5ffdce74fad2}',
             '7d4448409dc011d1b2455ffdce74fad2',
             b'\x7d\x44\x48\x40\x9d\xc0\x11\xd1\xb2\x45\x5f\xfd\xce\x74\xfa\xd2',
             b'\x40\x48\x44\x7d\xc0\x9d\xd1\x11\xb2\x45\x5f\xfd\xce\x74\xfa\xd2',
             (0x7d444840, 0x9dc0, 0x11d1, 0xb2, 0x45, 0x5ffdce74fad2),
             0x7d4448409dc011d1b2455ffdce74fad2,
             'urn:uuid:7d444840-9dc0-11d1-b245-5ffdce74fad2',
             0x1d19dc07d444840, 0x3245, self.uuid.RFC_4122, 1),
            ('e902893a-9d22-3c7e-a7b8-d6e313b71d9f',
             '{e902893a-9d22-3c7e-a7b8-d6e313b71d9f}',
             'e902893a9d223c7ea7b8d6e313b71d9f',
             b'\xe9\x02\x89\x3a\x9d\x22\x3c\x7e\xa7\xb8\xd6\xe3\x13\xb7\x1d\x9f',
             b'\x3a\x89\x02\xe9\x22\x9d\x7e\x3c\xa7\xb8\xd6\xe3\x13\xb7\x1d\x9f',
             (0xe902893a, 0x9d22, 0x3c7e, 0xa7, 0xb8, 0xd6e313b71d9f),
             0xe902893a9d223c7ea7b8d6e313b71d9f,
             'urn:uuid:e902893a-9d22-3c7e-a7b8-d6e313b71d9f',
             0xc7e9d22e902893a, 0x27b8, self.uuid.RFC_4122, 3),
            ('eb424026-6f54-4ef8-a4d0-bb658a1fc6cf',
             '{eb424026-6f54-4ef8-a4d0-bb658a1fc6cf}',
             'eb4240266f544ef8a4d0bb658a1fc6cf',
             b'\xeb\x42\x40\x26\x6f\x54\x4e\xf8\xa4\xd0\xbb\x65\x8a\x1f\xc6\xcf',
             b'\x26\x40\x42\xeb\x54\x6f\xf8\x4e\xa4\xd0\xbb\x65\x8a\x1f\xc6\xcf',
             (0xeb424026, 0x6f54, 0x4ef8, 0xa4, 0xd0, 0xbb658a1fc6cf),
             0xeb4240266f544ef8a4d0bb658a1fc6cf,
             'urn:uuid:eb424026-6f54-4ef8-a4d0-bb658a1fc6cf',
             0xef86f54eb424026, 0x24d0, self.uuid.RFC_4122, 4),
            ('f81d4fae-7dec-11d0-a765-00a0c91e6bf6',
             '{f81d4fae-7dec-11d0-a765-00a0c91e6bf6}',
             'f81d4fae7dec11d0a76500a0c91e6bf6',
             b'\xf8\x1d\x4f\xae\x7d\xec\x11\xd0\xa7\x65\x00\xa0\xc9\x1e\x6b\xf6',
             b'\xae\x4f\x1d\xf8\xec\x7d\xd0\x11\xa7\x65\x00\xa0\xc9\x1e\x6b\xf6',
             (0xf81d4fae, 0x7dec, 0x11d0, 0xa7, 0x65, 0x00a0c91e6bf6),
             0xf81d4fae7dec11d0a76500a0c91e6bf6,
             'urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6',
             0x1d07decf81d4fae, 0x2765, self.uuid.RFC_4122, 1),
            ('fffefdfc-fffe-fffe-fffe-fffefdfcfbfa',
             '{fffefdfc-fffe-fffe-fffe-fffefdfcfbfa}',
             'fffefdfcfffefffefffefffefdfcfbfa',
             b'\xff\xfe\xfd\xfc\xff\xfe\xff\xfe\xff\xfe\xff\xfe\xfd\xfc\xfb\xfa',
             b'\xfc\xfd\xfe\xff\xfe\xff\xfe\xff\xff\xfe\xff\xfe\xfd\xfc\xfb\xfa',
             (0xfffefdfc, 0xfffe, 0xfffe, 0xff, 0xfe, 0xfffefdfcfbfa),
             0xfffefdfcfffefffefffefffefdfcfbfa,
             'urn:uuid:fffefdfc-fffe-fffe-fffe-fffefdfcfbfa',
             0xffefffefffefdfc, 0x3ffe, self.uuid.RESERVED_FUTURE, None),
            ('ffffffff-ffff-ffff-ffff-ffffffffffff',
             '{ffffffff-ffff-ffff-ffff-ffffffffffff}',
             'ffffffffffffffffffffffffffffffff',
             b'\xff'*16,
             b'\xff'*16,
             (0xffffffff, 0xffff, 0xffff, 0xff, 0xff, 0xffffffffffff),
             0xffffffffffffffffffffffffffffffff,
             'urn:uuid:ffffffff-ffff-ffff-ffff-ffffffffffff',
             0xfffffffffffffff, 0x3fff, self.uuid.RESERVED_FUTURE, None),
            ]:
            equivalents = []
            # Construct each UUID in several different ways.
            for u in [self.uuid.UUID(string), self.uuid.UUID(curly), self.uuid.UUID(hex),
                      self.uuid.UUID(bytes=bytes), self.uuid.UUID(bytes_le=bytes_le),
                      self.uuid.UUID(fields=fields), self.uuid.UUID(int=integer),
                      self.uuid.UUID(urn)]:
                # Test all conversions and properties of the UUID object.
                equal(str(u), string)
                equal(int(u), integer)
                equal(u.bytes, bytes)
                equal(u.bytes_le, bytes_le)
                equal(u.fields, fields)
                equal(u.time_low, fields[0])
                equal(u.time_mid, fields[1])
                equal(u.time_hi_version, fields[2])
                equal(u.clock_seq_hi_variant, fields[3])
                equal(u.clock_seq_low, fields[4])
                equal(u.node, fields[5])
                equal(u.hex, hex)
                equal(u.int, integer)
                equal(u.urn, urn)
                equal(u.time, time)
                equal(u.clock_seq, clock_seq)
                equal(u.variant, variant)
                equal(u.version, version)
                equivalents.append(u)

            # Different construction methods should give the same UUID.
            for u in equivalents:
                for v in equivalents:
                    equal(u, v)

            # Bug 7380: "bytes" and "bytes_le" should give the same type.
            equal(type(u.bytes), builtins.bytes)
            equal(type(u.bytes_le), builtins.bytes)

            ascending.append(u)

        # Test comparison of UUIDs.
        for i in range(len(ascending)):
            for j in range(len(ascending)):
                equal(i < j, ascending[i] < ascending[j])
                equal(i <= j, ascending[i] <= ascending[j])
                equal(i == j, ascending[i] == ascending[j])
                equal(i > j, ascending[i] > ascending[j])
                equal(i >= j, ascending[i] >= ascending[j])
                equal(i != j, ascending[i] != ascending[j])

        # Test sorting of UUIDs (above list is in ascending order).
        resorted = ascending[:]
        resorted.reverse()
        resorted.sort()
        equal(ascending, resorted)

    def test_exceptions(self):
        badvalue = lambda f: self.assertRaises(ValueError, f)
        badtype = lambda f: self.assertRaises(TypeError, f)

        # Badly formed hex strings.
        badvalue(lambda: self.uuid.UUID(''))
        badvalue(lambda: self.uuid.UUID('abc'))
        badvalue(lambda: self.uuid.UUID('1234567812345678123456781234567'))
        badvalue(lambda: self.uuid.UUID('123456781234567812345678123456789'))
        badvalue(lambda: self.uuid.UUID('123456781234567812345678z2345678'))

        # Badly formed bytes.
        badvalue(lambda: self.uuid.UUID(bytes='abc'))
        badvalue(lambda: self.uuid.UUID(bytes='\0'*15))
        badvalue(lambda: self.uuid.UUID(bytes='\0'*17))

        # Badly formed bytes_le.
        badvalue(lambda: self.uuid.UUID(bytes_le='abc'))
        badvalue(lambda: self.uuid.UUID(bytes_le='\0'*15))
        badvalue(lambda: self.uuid.UUID(bytes_le='\0'*17))

        # Badly formed fields.
        badvalue(lambda: self.uuid.UUID(fields=(1,)))
        badvalue(lambda: self.uuid.UUID(fields=(1, 2, 3, 4, 5)))
        badvalue(lambda: self.uuid.UUID(fields=(1, 2, 3, 4, 5, 6, 7)))

        # Field values out of range.
        badvalue(lambda: self.uuid.UUID(fields=(-1, 0, 0, 0, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0x100000000, 0, 0, 0, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, -1, 0, 0, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0x10000, 0, 0, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, -1, 0, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, 0x10000, 0, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, 0, -1, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, 0, 0x100, 0, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, 0, 0, -1, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, 0, 0, 0x100, 0)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, 0, 0, 0, -1)))
        badvalue(lambda: self.uuid.UUID(fields=(0, 0, 0, 0, 0, 0x1000000000000)))

        # Version number out of range.
        badvalue(lambda: self.uuid.UUID('00'*16, version=0))
        badvalue(lambda: self.uuid.UUID('00'*16, version=42))

        # Integer value out of range.
        badvalue(lambda: self.uuid.UUID(int=-1))
        badvalue(lambda: self.uuid.UUID(int=1<<128))

        # Must supply exactly one of hex, bytes, fields, int.
        h, b, f, i = '00'*16, b'\0'*16, (0, 0, 0, 0, 0, 0), 0
        self.uuid.UUID(h)
        self.uuid.UUID(hex=h)
        self.uuid.UUID(bytes=b)
        self.uuid.UUID(bytes_le=b)
        self.uuid.UUID(fields=f)
        self.uuid.UUID(int=i)

        # Wrong number of arguments (positional).
        badtype(lambda: self.uuid.UUID())
        badtype(lambda: self.uuid.UUID(h, b))
        badtype(lambda: self.uuid.UUID(h, b, b))
        badtype(lambda: self.uuid.UUID(h, b, b, f))
        badtype(lambda: self.uuid.UUID(h, b, b, f, i))

        # Duplicate arguments.
        for hh in [[], [('hex', h)]]:
            for bb in [[], [('bytes', b)]]:
                for bble in [[], [('bytes_le', b)]]:
                    for ii in [[], [('int', i)]]:
                        for ff in [[], [('fields', f)]]:
                            args = dict(hh + bb + bble + ii + ff)
                            if len(args) != 0:
                                badtype(lambda: self.uuid.UUID(h, **args))
                            if len(args) != 1:
                                badtype(lambda: self.uuid.UUID(**args))

        # Immutability.
        u = self.uuid.UUID(h)
        badtype(lambda: setattr(u, 'hex', h))
        badtype(lambda: setattr(u, 'bytes', b))
        badtype(lambda: setattr(u, 'bytes_le', b))
        badtype(lambda: setattr(u, 'fields', f))
        badtype(lambda: setattr(u, 'int', i))
        badtype(lambda: setattr(u, 'time_low', 0))
        badtype(lambda: setattr(u, 'time_mid', 0))
        badtype(lambda: setattr(u, 'time_hi_version', 0))
        badtype(lambda: setattr(u, 'time_hi_version', 0))
        badtype(lambda: setattr(u, 'clock_seq_hi_variant', 0))
        badtype(lambda: setattr(u, 'clock_seq_low', 0))
        badtype(lambda: setattr(u, 'node', 0))

        # Comparison with a non-UUID object
        badtype(lambda: u < object())
        badtype(lambda: u > object())

    def test_getnode(self):
        node1 = self.uuid.getnode()
        self.assertTrue(0 < node1 < (1 << 48), '%012x' % node1)

        # Test it again to ensure consistency.
        node2 = self.uuid.getnode()
        self.assertEqual(node1, node2, '%012x != %012x' % (node1, node2))

    def test_pickle_roundtrip(self):
        def check(actual, expected):
            self.assertEqual(actual, expected)
            self.assertEqual(actual.is_safe, expected.is_safe)

        with support.swap_item(sys.modules, 'uuid', self.uuid):
            for is_safe in self.uuid.SafeUUID:
                u = self.uuid.UUID('d82579ce6642a0de7ddf490a7aec7aa5',
                                   is_safe=is_safe)
                check(copy.copy(u), u)
                check(copy.deepcopy(u), u)
                for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                    with self.subTest(protocol=proto):
                        check(pickle.loads(pickle.dumps(u, proto)), u)

    def test_unpickle_previous_python_versions(self):
        def check(actual, expected):
            self.assertEqual(actual, expected)
            self.assertEqual(actual.is_safe, expected.is_safe)

        pickled_uuids = [
            # Python 2.7, protocol 0
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR(dS\'int\'\nL287307832597519156748809049798316161701L\nsb.',
            # Python 2.7, protocol 1
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR}U\x03intL287307832597519156748809049798316161701L\nsb.',
            # Python 2.7, protocol 2
            b'\x80\x02cuuid\nUUID\n)\x81}U\x03int\x8a\x11\xa5z\xecz\nI\xdf}'
            b'\xde\xa0Bf\xcey%\xd8\x00sb.',
            # Python 3.6, protocol 0
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR(dVint\nL287307832597519156748809049798316161701L\nsb.',
            # Python 3.6, protocol 1
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR}X\x03\x00\x00\x00intL287307832597519156748809049798316161701L'
            b'\nsb.',
            # Python 3.6, protocol 2
            b'\x80\x02cuuid\nUUID\n)\x81}X\x03\x00\x00\x00int\x8a\x11\xa5z\xec'
            b'z\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00sb.',
            # Python 3.6, protocol 3
            b'\x80\x03cuuid\nUUID\n)\x81}X\x03\x00\x00\x00int\x8a\x11\xa5z\xec'
            b'z\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00sb.',
            # Python 3.6, protocol 4
            b'\x80\x04\x95+\x00\x00\x00\x00\x00\x00\x00\x8c\x04uuid\x8c\x04UUI'
            b'D\x93)\x81}\x8c\x03int\x8a\x11\xa5z\xecz\nI\xdf}\xde\xa0Bf\xcey%'
            b'\xd8\x00sb.',
            # Python 3.7, protocol 0
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR(dVint\nL287307832597519156748809049798316161701L\nsVis_safe\n'
            b'cuuid\nSafeUUID\n(NtRsb.',
            # Python 3.7, protocol 1
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR}(X\x03\x00\x00\x00intL287307832597519156748809049798316161701'
            b'L\nX\x07\x00\x00\x00is_safecuuid\nSafeUUID\n(NtRub.',
            # Python 3.7, protocol 2
            b'\x80\x02cuuid\nUUID\n)\x81}(X\x03\x00\x00\x00int\x8a\x11\xa5z'
            b'\xecz\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00X\x07\x00\x00\x00is_safecuu'
            b'id\nSafeUUID\nN\x85Rub.',
            # Python 3.7, protocol 3
            b'\x80\x03cuuid\nUUID\n)\x81}(X\x03\x00\x00\x00int\x8a\x11\xa5z'
            b'\xecz\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00X\x07\x00\x00\x00is_safecuu'
            b'id\nSafeUUID\nN\x85Rub.',
            # Python 3.7, protocol 4
            b'\x80\x04\x95F\x00\x00\x00\x00\x00\x00\x00\x8c\x04uuid\x94\x8c'
            b'\x04UUID\x93)\x81}(\x8c\x03int\x8a\x11\xa5z\xecz\nI\xdf}\xde\xa0'
            b'Bf\xcey%\xd8\x00\x8c\x07is_safeh\x00\x8c\x08SafeUUID\x93N\x85Rub'
            b'.',
        ]
        pickled_uuids_safe = [
            # Python 3.7, protocol 0
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR(dVint\nL287307832597519156748809049798316161701L\nsVis_safe\n'
            b'cuuid\nSafeUUID\n(I0\ntRsb.',
            # Python 3.7, protocol 1
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR}(X\x03\x00\x00\x00intL287307832597519156748809049798316161701'
            b'L\nX\x07\x00\x00\x00is_safecuuid\nSafeUUID\n(K\x00tRub.',
            # Python 3.7, protocol 2
            b'\x80\x02cuuid\nUUID\n)\x81}(X\x03\x00\x00\x00int\x8a\x11\xa5z'
            b'\xecz\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00X\x07\x00\x00\x00is_safecuu'
            b'id\nSafeUUID\nK\x00\x85Rub.',
            # Python 3.7, protocol 3
            b'\x80\x03cuuid\nUUID\n)\x81}(X\x03\x00\x00\x00int\x8a\x11\xa5z'
            b'\xecz\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00X\x07\x00\x00\x00is_safecuu'
            b'id\nSafeUUID\nK\x00\x85Rub.',
            # Python 3.7, protocol 4
            b'\x80\x04\x95G\x00\x00\x00\x00\x00\x00\x00\x8c\x04uuid\x94\x8c'
            b'\x04UUID\x93)\x81}(\x8c\x03int\x8a\x11\xa5z\xecz\nI\xdf}\xde\xa0'
            b'Bf\xcey%\xd8\x00\x8c\x07is_safeh\x00\x8c\x08SafeUUID\x93K\x00'
            b'\x85Rub.',
        ]
        pickled_uuids_unsafe = [
            # Python 3.7, protocol 0
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR(dVint\nL287307832597519156748809049798316161701L\nsVis_safe\n'
            b'cuuid\nSafeUUID\n(I-1\ntRsb.',
            # Python 3.7, protocol 1
            b'ccopy_reg\n_reconstructor\n(cuuid\nUUID\nc__builtin__\nobject\nN'
            b'tR}(X\x03\x00\x00\x00intL287307832597519156748809049798316161701'
            b'L\nX\x07\x00\x00\x00is_safecuuid\nSafeUUID\n(J\xff\xff\xff\xfftR'
            b'ub.',
            # Python 3.7, protocol 2
            b'\x80\x02cuuid\nUUID\n)\x81}(X\x03\x00\x00\x00int\x8a\x11\xa5z'
            b'\xecz\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00X\x07\x00\x00\x00is_safecuu'
            b'id\nSafeUUID\nJ\xff\xff\xff\xff\x85Rub.',
            # Python 3.7, protocol 3
            b'\x80\x03cuuid\nUUID\n)\x81}(X\x03\x00\x00\x00int\x8a\x11\xa5z'
            b'\xecz\nI\xdf}\xde\xa0Bf\xcey%\xd8\x00X\x07\x00\x00\x00is_safecuu'
            b'id\nSafeUUID\nJ\xff\xff\xff\xff\x85Rub.',
            # Python 3.7, protocol 4
            b'\x80\x04\x95J\x00\x00\x00\x00\x00\x00\x00\x8c\x04uuid\x94\x8c'
            b'\x04UUID\x93)\x81}(\x8c\x03int\x8a\x11\xa5z\xecz\nI\xdf}\xde\xa0'
            b'Bf\xcey%\xd8\x00\x8c\x07is_safeh\x00\x8c\x08SafeUUID\x93J\xff'
            b'\xff\xff\xff\x85Rub.',
        ]

        u = self.uuid.UUID('d82579ce6642a0de7ddf490a7aec7aa5')
        u_safe = self.uuid.UUID('d82579ce6642a0de7ddf490a7aec7aa5',
                                is_safe=self.uuid.SafeUUID.safe)
        u_unsafe = self.uuid.UUID('d82579ce6642a0de7ddf490a7aec7aa5',
                                  is_safe=self.uuid.SafeUUID.unsafe)

        with support.swap_item(sys.modules, 'uuid', self.uuid):
            for pickled in pickled_uuids:
                # is_safe was added in 3.7.  When unpickling values from older
                # versions, is_safe will be missing, so it should be set to
                # SafeUUID.unknown.
                check(pickle.loads(pickled), u)
            for pickled in pickled_uuids_safe:
                check(pickle.loads(pickled), u_safe)
            for pickled in pickled_uuids_unsafe:
                check(pickle.loads(pickled), u_unsafe)

    # bpo-32502: UUID1 requires a 48-bit identifier, but hardware identifiers
    # need not necessarily be 48 bits (e.g., EUI-64).
    def test_uuid1_eui64(self):
        # Confirm that uuid.getnode ignores hardware addresses larger than 48
        # bits. Mock out each platform's *_getnode helper functions to return
        # something just larger than 48 bits to test. This will cause
        # uuid.getnode to fall back on uuid._random_getnode, which will
        # generate a valid value.
        too_large_getter = lambda: 1 << 48
        with mock.patch.multiple(
            self.uuid,
            _node=None,  # Ignore any cached node value.
            _GETTERS=[too_large_getter],
        ):
            node = self.uuid.getnode()
        self.assertTrue(0 < node < (1 << 48), '%012x' % node)

        # Confirm that uuid1 can use the generated node, i.e., the that
        # uuid.getnode fell back on uuid._random_getnode() rather than using
        # the value from too_large_getter above.
        try:
            self.uuid.uuid1(node=node)
        except ValueError:
            self.fail('uuid1 was given an invalid node ID')

    def test_uuid1(self):
        equal = self.assertEqual

        # Make sure uuid1() generates UUIDs that are actually version 1.
        for u in [self.uuid.uuid1() for i in range(10)]:
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 1)
            self.assertIn(u.is_safe, {self.uuid.SafeUUID.safe,
                                      self.uuid.SafeUUID.unsafe,
                                      self.uuid.SafeUUID.unknown})

        # Make sure the generated UUIDs are actually unique.
        uuids = {}
        for u in [self.uuid.uuid1() for i in range(1000)]:
            uuids[u] = 1
        equal(len(uuids.keys()), 1000)

        # Make sure the supplied node ID appears in the UUID.
        u = self.uuid.uuid1(0)
        equal(u.node, 0)
        u = self.uuid.uuid1(0x123456789abc)
        equal(u.node, 0x123456789abc)
        u = self.uuid.uuid1(0xffffffffffff)
        equal(u.node, 0xffffffffffff)

        # Make sure the supplied clock sequence appears in the UUID.
        u = self.uuid.uuid1(0x123456789abc, 0)
        equal(u.node, 0x123456789abc)
        equal(((u.clock_seq_hi_variant & 0x3f) << 8) | u.clock_seq_low, 0)
        u = self.uuid.uuid1(0x123456789abc, 0x1234)
        equal(u.node, 0x123456789abc)
        equal(((u.clock_seq_hi_variant & 0x3f) << 8) |
                         u.clock_seq_low, 0x1234)
        u = self.uuid.uuid1(0x123456789abc, 0x3fff)
        equal(u.node, 0x123456789abc)
        equal(((u.clock_seq_hi_variant & 0x3f) << 8) |
                         u.clock_seq_low, 0x3fff)

    # bpo-29925: On Mac OS X Tiger, self.uuid.uuid1().is_safe returns
    # self.uuid.SafeUUID.unknown
    @support.requires_mac_ver(10, 5)
    @unittest.skipUnless(os.name == 'posix', 'POSIX-only test')
    def test_uuid1_safe(self):
        try:
            import _uuid
        except ImportError:
            has_uuid_generate_time_safe = False
        else:
            has_uuid_generate_time_safe = _uuid.has_uuid_generate_time_safe

        if not has_uuid_generate_time_safe or not self.uuid._generate_time_safe:
            self.skipTest('requires uuid_generate_time_safe(3)')

        u = self.uuid.uuid1()
        # uuid_generate_time_safe() may return 0 or -1 but what it returns is
        # dependent on the underlying platform support.  At least it cannot be
        # unknown (unless I suppose the platform is buggy).
        self.assertNotEqual(u.is_safe, self.uuid.SafeUUID.unknown)
        self.assertEqual(u.version, 1)

    @contextlib.contextmanager
    def mock_generate_time_safe(self, safe_value):
        """
        Mock uuid._generate_time_safe() to return a given *safe_value*.
        """
        if os.name != 'posix':
            self.skipTest('POSIX-only test')
        f = self.uuid._generate_time_safe
        if f is None:
            self.skipTest('need uuid._generate_time_safe')
        with mock.patch.object(self.uuid, '_generate_time_safe',
                               lambda: (f()[0], safe_value)):
            yield

    @unittest.skipUnless(os.name == 'posix', 'POSIX-only test')
    def test_uuid1_unknown(self):
        # Even if the platform has uuid_generate_time_safe(), let's mock it to
        # be uuid_generate_time() and ensure the safety is unknown.
        with self.mock_generate_time_safe(None):
            u = self.uuid.uuid1()
            self.assertEqual(u.is_safe, self.uuid.SafeUUID.unknown)
            self.assertEqual(u.version, 1)

    @unittest.skipUnless(os.name == 'posix', 'POSIX-only test')
    def test_uuid1_is_safe(self):
        with self.mock_generate_time_safe(0):
            u = self.uuid.uuid1()
            self.assertEqual(u.is_safe, self.uuid.SafeUUID.safe)
            self.assertEqual(u.version, 1)

    @unittest.skipUnless(os.name == 'posix', 'POSIX-only test')
    def test_uuid1_is_unsafe(self):
        with self.mock_generate_time_safe(-1):
            u = self.uuid.uuid1()
            self.assertEqual(u.is_safe, self.uuid.SafeUUID.unsafe)
            self.assertEqual(u.version, 1)

    @unittest.skipUnless(os.name == 'posix', 'POSIX-only test')
    def test_uuid1_bogus_return_value(self):
        with self.mock_generate_time_safe(3):
            u = self.uuid.uuid1()
            self.assertEqual(u.is_safe, self.uuid.SafeUUID.unknown)
            self.assertEqual(u.version, 1)

    def test_uuid1_time(self):
        with mock.patch.object(self.uuid, '_generate_time_safe', None), \
             mock.patch.object(self.uuid, '_last_timestamp', None), \
             mock.patch.object(self.uuid, 'getnode', return_value=93328246233727), \
             mock.patch('time.time_ns', return_value=1545052026752910643), \
             mock.patch('random.getrandbits', return_value=5317): # guaranteed to be random
            u = self.uuid.uuid1()
            self.assertEqual(u, self.uuid.UUID('a7a55b92-01fc-11e9-94c5-54e1acf6da7f'))

        with mock.patch.object(self.uuid, '_generate_time_safe', None), \
             mock.patch.object(self.uuid, '_last_timestamp', None), \
             mock.patch('time.time_ns', return_value=1545052026752910643):
            u = self.uuid.uuid1(node=93328246233727, clock_seq=5317)
            self.assertEqual(u, self.uuid.UUID('a7a55b92-01fc-11e9-94c5-54e1acf6da7f'))

    def test_uuid3(self):
        equal = self.assertEqual

        # Test some known version-3 UUIDs with name passed as a byte object
        for u, v in [(self.uuid.uuid3(self.uuid.NAMESPACE_DNS, b'python.org'),
                      '6fa459ea-ee8a-3ca4-894e-db77e160355e'),
                     (self.uuid.uuid3(self.uuid.NAMESPACE_URL, b'http://python.org/'),
                      '9fe8e8c4-aaa8-32a9-a55c-4535a88b748d'),
                     (self.uuid.uuid3(self.uuid.NAMESPACE_OID, b'1.3.6.1'),
                      'dd1a1cef-13d5-368a-ad82-eca71acd4cd1'),
                     (self.uuid.uuid3(self.uuid.NAMESPACE_X500, b'c=ca'),
                      '658d3002-db6b-3040-a1d1-8ddd7d189a4d'),
                    ]:
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 3)
            equal(u, self.uuid.UUID(v))
            equal(str(u), v)

        # Test some known version-3 UUIDs with name passed as a string
        for u, v in [(self.uuid.uuid3(self.uuid.NAMESPACE_DNS, 'python.org'),
                      '6fa459ea-ee8a-3ca4-894e-db77e160355e'),
                     (self.uuid.uuid3(self.uuid.NAMESPACE_URL, 'http://python.org/'),
                      '9fe8e8c4-aaa8-32a9-a55c-4535a88b748d'),
                     (self.uuid.uuid3(self.uuid.NAMESPACE_OID, '1.3.6.1'),
                      'dd1a1cef-13d5-368a-ad82-eca71acd4cd1'),
                     (self.uuid.uuid3(self.uuid.NAMESPACE_X500, 'c=ca'),
                      '658d3002-db6b-3040-a1d1-8ddd7d189a4d'),
                    ]:
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 3)
            equal(u, self.uuid.UUID(v))
            equal(str(u), v)

    def test_uuid4(self):
        equal = self.assertEqual

        # Make sure uuid4() generates UUIDs that are actually version 4.
        for u in [self.uuid.uuid4() for i in range(10)]:
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 4)

        # Make sure the generated UUIDs are actually unique.
        uuids = {}
        for u in [self.uuid.uuid4() for i in range(1000)]:
            uuids[u] = 1
        equal(len(uuids.keys()), 1000)

    def test_uuid5(self):
        equal = self.assertEqual

        # Test some known version-5 UUIDs with names given as byte objects
        for u, v in [(self.uuid.uuid5(self.uuid.NAMESPACE_DNS, b'python.org'),
                      '886313e1-3b8a-5372-9b90-0c9aee199e5d'),
                     (self.uuid.uuid5(self.uuid.NAMESPACE_URL, b'http://python.org/'),
                      '4c565f0d-3f5a-5890-b41b-20cf47701c5e'),
                     (self.uuid.uuid5(self.uuid.NAMESPACE_OID, b'1.3.6.1'),
                      '1447fa61-5277-5fef-a9b3-fbc6e44f4af3'),
                     (self.uuid.uuid5(self.uuid.NAMESPACE_X500, b'c=ca'),
                      'cc957dd1-a972-5349-98cd-874190002798'),
                    ]:
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 5)
            equal(u, self.uuid.UUID(v))
            equal(str(u), v)

        # Test some known version-5 UUIDs with names given as strings
        for u, v in [(self.uuid.uuid5(self.uuid.NAMESPACE_DNS, 'python.org'),
                      '886313e1-3b8a-5372-9b90-0c9aee199e5d'),
                     (self.uuid.uuid5(self.uuid.NAMESPACE_URL, 'http://python.org/'),
                      '4c565f0d-3f5a-5890-b41b-20cf47701c5e'),
                     (self.uuid.uuid5(self.uuid.NAMESPACE_OID, '1.3.6.1'),
                      '1447fa61-5277-5fef-a9b3-fbc6e44f4af3'),
                     (self.uuid.uuid5(self.uuid.NAMESPACE_X500, 'c=ca'),
                      'cc957dd1-a972-5349-98cd-874190002798'),
                    ]:
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 5)
            equal(u, self.uuid.UUID(v))
            equal(str(u), v)

    def test_uuid6(self):
        equal = self.assertEqual
        u = self.uuid.uuid6()
        equal(u.variant, self.uuid.RFC_4122)
        equal(u.version, 6)

        fake_nanoseconds = 0x1571_20a1_de1a_c533
        fake_node_value = 0x54e1_acf6_da7f
        fake_clock_seq = 0x14c5
        with (
            mock.patch.object(self.uuid, '_last_timestamp_v6', None),
            mock.patch.object(self.uuid, 'getnode', return_value=fake_node_value),
            mock.patch('time.time_ns', return_value=fake_nanoseconds),
            mock.patch('random.getrandbits', return_value=fake_clock_seq)
        ):
            u = self.uuid.uuid6()
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 6)

            # 32 (top) | 16 (mid) | 12 (low) == 60 (timestamp)
            equal(u.time, 0x1e901fca_7a55_b92)
            equal(u.fields[0], 0x1e901fca)  # 32 top bits of time
            equal(u.fields[1], 0x7a55)  # 16 mid bits of time
            # 4 bits of version + 12 low bits of time
            equal((u.fields[2] >> 12) & 0xf, 6)
            equal((u.fields[2] & 0xfff), 0xb92)
            # 2 bits of variant + 6 high bits of clock_seq
            equal((u.fields[3] >> 6) & 0xf, 2)
            equal(u.fields[3] & 0x3f, fake_clock_seq >> 8)
            # 8 low bits of clock_seq
            equal(u.fields[4], fake_clock_seq & 0xff)
            equal(u.fields[5], fake_node_value)

    def test_uuid6_uniqueness(self):
        # Test that UUIDv6-generated values are unique.

        # Unlike UUIDv8, only 62 bits can be randomized for UUIDv6.
        # In practice, however, it remains unlikely to generate two
        # identical UUIDs for the same 60-bit timestamp if neither
        # the node ID nor the clock sequence is specified.
        uuids = {self.uuid.uuid6() for _ in range(1000)}
        self.assertEqual(len(uuids), 1000)
        versions = {u.version for u in uuids}
        self.assertSetEqual(versions, {6})

        timestamp = 0x1ec9414c_232a_b00
        fake_nanoseconds = (timestamp - 0x1b21dd21_3814_000) * 100

        with mock.patch('time.time_ns', return_value=fake_nanoseconds):
            def gen():
                with mock.patch.object(self.uuid, '_last_timestamp_v6', None):
                    return self.uuid.uuid6(node=0, clock_seq=None)

            # By the birthday paradox, sampling N = 1024 UUIDs with identical
            # node IDs and timestamps results in duplicates with probability
            # close to 1 (not having a duplicate happens with probability of
            # order 1E-15) since only the 14-bit clock sequence is randomized.
            N = 1024
            uuids = {gen() for _ in range(N)}
            self.assertSetEqual({u.node for u in uuids}, {0})
            self.assertSetEqual({u.time for u in uuids}, {timestamp})
            self.assertLess(len(uuids), N, 'collision property does not hold')

    def test_uuid6_node(self):
        # Make sure the given node ID appears in the UUID.
        #
        # Note: when no node ID is specified, the same logic as for UUIDv1
        # is applied to UUIDv6. In particular, there is no need to test that
        # getnode() correctly returns positive integers of exactly 48 bits
        # since this is done in test_uuid1_eui64().
        self.assertLessEqual(self.uuid.uuid6().node.bit_length(), 48)

        self.assertEqual(self.uuid.uuid6(0).node, 0)

        # tests with explicit values
        max_node = 0xffff_ffff_ffff
        self.assertEqual(self.uuid.uuid6(max_node).node, max_node)
        big_node = 0xE_1234_5678_ABCD  # 52-bit node
        res_node = 0x0_1234_5678_ABCD  # truncated to 48 bits
        self.assertEqual(self.uuid.uuid6(big_node).node, res_node)

        # randomized tests
        for _ in range(10):
            # node with > 48 bits is truncated
            for b in [24, 48, 72]:
                node = (1 << (b - 1)) | random.getrandbits(b)
                with self.subTest(node=node, bitlen=b):
                    self.assertEqual(node.bit_length(), b)
                    u = self.uuid.uuid6(node=node)
                    self.assertEqual(u.node, node & 0xffff_ffff_ffff)

    def test_uuid6_clock_seq(self):
        # Make sure the supplied clock sequence appears in the UUID.
        #
        # For UUIDv6, clock sequence bits are stored from bit 48 to bit 62,
        # with the convention that the least significant bit is bit 0 and
        # the most significant bit is bit 127.
        get_clock_seq = lambda u: (u.int >> 48) & 0x3fff

        u = self.uuid.uuid6()
        self.assertLessEqual(get_clock_seq(u).bit_length(), 14)

        # tests with explicit values
        big_clock_seq = 0xffff  # 16-bit clock sequence
        res_clock_seq = 0x3fff  # truncated to 14 bits
        u = self.uuid.uuid6(clock_seq=big_clock_seq)
        self.assertEqual(get_clock_seq(u), res_clock_seq)

        # some randomized tests
        for _ in range(10):
            # clock_seq with > 14 bits is truncated
            for b in [7, 14, 28]:
                node = random.getrandbits(48)
                clock_seq = (1 << (b - 1)) | random.getrandbits(b)
                with self.subTest(node=node, clock_seq=clock_seq, bitlen=b):
                    self.assertEqual(clock_seq.bit_length(), b)
                    u = self.uuid.uuid6(node=node, clock_seq=clock_seq)
                    self.assertEqual(get_clock_seq(u), clock_seq & 0x3fff)

    def test_uuid6_test_vectors(self):
        equal = self.assertEqual
        # https://www.rfc-editor.org/rfc/rfc9562#name-test-vectors
        # (separators are put at the 12th and 28th bits)
        timestamp = 0x1ec9414c_232a_b00
        fake_nanoseconds = (timestamp - 0x1b21dd21_3814_000) * 100
        # https://www.rfc-editor.org/rfc/rfc9562#name-example-of-a-uuidv6-value
        node = 0x9f6bdeced846
        clock_seq = (3 << 12) | 0x3c8

        with (
            mock.patch.object(self.uuid, '_last_timestamp_v6', None),
            mock.patch('time.time_ns', return_value=fake_nanoseconds)
        ):
            u = self.uuid.uuid6(node=node, clock_seq=clock_seq)
            equal(str(u).upper(), '1EC9414C-232A-6B00-B3C8-9F6BDECED846')
            #   32          16      4      12       2      14         48
            # time_hi | time_mid | ver | time_lo | var | clock_seq | node
            equal(u.time, timestamp)
            equal(u.int & 0xffff_ffff_ffff, node)
            equal((u.int >> 48) & 0x3fff, clock_seq)
            equal((u.int >> 62) & 0x3, 0b10)
            equal((u.int >> 64) & 0xfff, 0xb00)
            equal((u.int >> 76) & 0xf, 0x6)
            equal((u.int >> 80) & 0xffff, 0x232a)
            equal((u.int >> 96) & 0xffff_ffff, 0x1ec9_414c)

    def test_uuid7(self):
        equal = self.assertEqual
        u = self.uuid.uuid7()
        equal(u.variant, self.uuid.RFC_4122)
        equal(u.version, 7)

        # 1 Jan 2023 12:34:56.123_456_789
        timestamp_ns = 1672533296_123_456_789  # ns precision
        timestamp_ms, _ = divmod(timestamp_ns, 1_000_000)

        for _ in range(100):
            counter_hi = random.getrandbits(11)
            counter_lo = random.getrandbits(30)
            counter = (counter_hi << 30) | counter_lo

            tail = random.getrandbits(32)
            # effective number of bits is 32 + 30 + 11 = 73
            random_bits = counter << 32 | tail

            # set all remaining MSB of fake random bits to 1 to ensure that
            # the implementation correctly removes them
            random_bits = (((1 << 7) - 1) << 73) | random_bits
            random_data = random_bits.to_bytes(10)

            with (
                mock.patch.multiple(
                    self.uuid,
                    _last_timestamp_v7=None,
                    _last_counter_v7=0,
                ),
                mock.patch('time.time_ns', return_value=timestamp_ns),
                mock.patch('os.urandom', return_value=random_data) as urand
            ):
                u = self.uuid.uuid7()
                urand.assert_called_once_with(10)
                equal(u.variant, self.uuid.RFC_4122)
                equal(u.version, 7)

                equal(self.uuid._last_timestamp_v7, timestamp_ms)
                equal(self.uuid._last_counter_v7, counter)

                unix_ts_ms = timestamp_ms & 0xffff_ffff_ffff
                equal(u.time, unix_ts_ms)
                equal((u.int >> 80) & 0xffff_ffff_ffff, unix_ts_ms)

                equal((u.int >> 75) & 1, 0)  # check that the MSB is 0
                equal((u.int >> 64) & 0xfff, counter_hi)
                equal((u.int >> 32) & 0x3fff_ffff, counter_lo)
                equal(u.int & 0xffff_ffff, tail)

    def test_uuid7_uniqueness(self):
        # Test that UUIDv7-generated values are unique.
        #
        # While UUIDv8 has an entropy of 122 bits, those 122 bits may not
        # necessarily be sampled from a PRNG. On the other hand, UUIDv7
        # uses os.urandom() as a PRNG which features better randomness.
        N = 1000
        uuids = {self.uuid.uuid7() for _ in range(N)}
        self.assertEqual(len(uuids), N)

        versions = {u.version for u in uuids}
        self.assertSetEqual(versions, {7})

    def test_uuid7_monotonicity(self):
        equal = self.assertEqual

        us = [self.uuid.uuid7() for _ in range(10_000)]
        equal(us, sorted(us))

        with mock.patch.multiple(
            self.uuid,
            _last_timestamp_v7=0,
            _last_counter_v7=0,
        ):
            # 1 Jan 2023 12:34:56.123_456_789
            timestamp_ns = 1672533296_123_456_789  # ns precision
            timestamp_ms, _ = divmod(timestamp_ns, 1_000_000)

            # counter_{hi,lo} are chosen so that "counter + 1" does not overflow
            counter_hi = random.getrandbits(11)
            counter_lo = random.getrandbits(29)
            counter = (counter_hi << 30) | counter_lo
            self.assertLess(counter + 1, 0x3ff_ffff_ffff)

            tail = random.getrandbits(32)
            random_bits = counter << 32 | tail
            random_data = random_bits.to_bytes(10)

            with (
                mock.patch('time.time_ns', return_value=timestamp_ns),
                mock.patch('os.urandom', return_value=random_data) as urand
            ):
                u1 = self.uuid.uuid7()
                urand.assert_called_once_with(10)
                equal(self.uuid._last_timestamp_v7, timestamp_ms)
                equal(self.uuid._last_counter_v7, counter)
                equal(u1.time, timestamp_ms)
                equal((u1.int >> 64) & 0xfff, counter_hi)
                equal((u1.int >> 32) & 0x3fff_ffff, counter_lo)
                equal(u1.int & 0xffff_ffff, tail)

            # 1 Jan 2023 12:34:56.123_457_032 (same millisecond but not same ns)
            next_timestamp_ns = 1672533296_123_457_032
            next_timestamp_ms, _ = divmod(timestamp_ns, 1_000_000)
            equal(timestamp_ms, next_timestamp_ms)

            next_tail_bytes = os.urandom(4)
            next_fail = int.from_bytes(next_tail_bytes)

            with (
                mock.patch('time.time_ns', return_value=next_timestamp_ns),
                mock.patch('os.urandom', return_value=next_tail_bytes) as urand
            ):
                u2 = self.uuid.uuid7()
                urand.assert_called_once_with(4)
                # same milli-second
                equal(self.uuid._last_timestamp_v7, timestamp_ms)
                # 42-bit counter advanced by 1
                equal(self.uuid._last_counter_v7, counter + 1)
                equal(u2.time, timestamp_ms)
                equal((u2.int >> 64) & 0xfff, counter_hi)
                equal((u2.int >> 32) & 0x3fff_ffff, counter_lo + 1)
                equal(u2.int & 0xffff_ffff, next_fail)

            self.assertLess(u1, u2)

    def test_uuid7_timestamp_backwards(self):
        equal = self.assertEqual
        # 1 Jan 2023 12:34:56.123_456_789
        timestamp_ns = 1672533296_123_456_789  # ns precision
        timestamp_ms, _ = divmod(timestamp_ns, 1_000_000)
        fake_last_timestamp_v7 = timestamp_ms + 1

        # counter_{hi,lo} are chosen so that "counter + 1" does not overflow
        counter_hi = random.getrandbits(11)
        counter_lo = random.getrandbits(29)
        counter = (counter_hi << 30) | counter_lo
        self.assertLess(counter + 1, 0x3ff_ffff_ffff)

        tail_bytes = os.urandom(4)
        tail = int.from_bytes(tail_bytes)

        with (
            mock.patch.multiple(
                self.uuid,
                _last_timestamp_v7=fake_last_timestamp_v7,
                _last_counter_v7=counter,
            ),
            mock.patch('time.time_ns', return_value=timestamp_ns),
            mock.patch('os.urandom', return_value=tail_bytes) as urand
        ):
            u = self.uuid.uuid7()
            urand.assert_called_once_with(4)
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 7)
            equal(self.uuid._last_timestamp_v7, fake_last_timestamp_v7 + 1)
            unix_ts_ms = (fake_last_timestamp_v7 + 1) & 0xffff_ffff_ffff
            equal(u.time, unix_ts_ms)
            equal((u.int >> 80) & 0xffff_ffff_ffff, unix_ts_ms)
            # 42-bit counter advanced by 1
            equal(self.uuid._last_counter_v7, counter + 1)
            equal((u.int >> 64) & 0xfff, counter_hi)
            # 42-bit counter advanced by 1 (counter_hi is untouched)
            equal((u.int >> 32) & 0x3fff_ffff, counter_lo + 1)
            equal(u.int & 0xffff_ffff, tail)

    def test_uuid7_overflow_counter(self):
        equal = self.assertEqual
        # 1 Jan 2023 12:34:56.123_456_789
        timestamp_ns = 1672533296_123_456_789  # ns precision
        timestamp_ms, _ = divmod(timestamp_ns, 1_000_000)

        new_counter_hi = random.getrandbits(11)
        new_counter_lo = random.getrandbits(30)
        new_counter = (new_counter_hi << 30) | new_counter_lo

        tail = random.getrandbits(32)
        random_bits = (new_counter << 32) | tail
        random_data = random_bits.to_bytes(10)

        with (
            mock.patch.multiple(
                self.uuid,
                _last_timestamp_v7=timestamp_ms,
                # same timestamp, but force an overflow on the counter
                _last_counter_v7=0x3ff_ffff_ffff,
            ),
            mock.patch('time.time_ns', return_value=timestamp_ns),
            mock.patch('os.urandom', return_value=random_data) as urand
        ):
            u = self.uuid.uuid7()
            urand.assert_called_with(10)
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 7)
            # timestamp advanced due to overflow
            equal(self.uuid._last_timestamp_v7, timestamp_ms + 1)
            unix_ts_ms = (timestamp_ms + 1) & 0xffff_ffff_ffff
            equal(u.time, unix_ts_ms)
            equal((u.int >> 80) & 0xffff_ffff_ffff, unix_ts_ms)
            # counter overflowed, so we picked a new one
            equal(self.uuid._last_counter_v7, new_counter)
            equal((u.int >> 64) & 0xfff, new_counter_hi)
            equal((u.int >> 32) & 0x3fff_ffff, new_counter_lo)
            equal(u.int & 0xffff_ffff, tail)

    def test_uuid8(self):
        equal = self.assertEqual
        u = self.uuid.uuid8()

        equal(u.variant, self.uuid.RFC_4122)
        equal(u.version, 8)

        for (_, hi, mid, lo) in product(
            range(10),  # repeat 10 times
            [None, 0, random.getrandbits(48)],
            [None, 0, random.getrandbits(12)],
            [None, 0, random.getrandbits(62)],
        ):
            u = self.uuid.uuid8(hi, mid, lo)
            equal(u.variant, self.uuid.RFC_4122)
            equal(u.version, 8)
            if hi is not None:
                equal((u.int >> 80) & 0xffffffffffff, hi)
            if mid is not None:
                equal((u.int >> 64) & 0xfff, mid)
            if lo is not None:
                equal(u.int & 0x3fffffffffffffff, lo)

    def test_uuid8_uniqueness(self):
        # Test that UUIDv8-generated values are unique (up to a negligible
        # probability of failure). There are 122 bits of entropy and assuming
        # that the underlying mt-19937-based random generator is sufficiently
        # good, it is unlikely to have a collision of two UUIDs.
        N = 1000
        uuids = {self.uuid.uuid8() for _ in range(N)}
        self.assertEqual(len(uuids), N)

        versions = {u.version for u in uuids}
        self.assertSetEqual(versions, {8})

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @support.requires_fork()
    def testIssue8621(self):
        # On at least some versions of OSX self.uuid.uuid4 generates
        # the same sequence of UUIDs in the parent and any
        # children started using fork.
        fds = os.pipe()
        pid = os.fork()
        if pid == 0:
            os.close(fds[0])
            value = self.uuid.uuid4()
            os.write(fds[1], value.hex.encode('latin-1'))
            os._exit(0)

        else:
            os.close(fds[1])
            self.addCleanup(os.close, fds[0])
            parent_value = self.uuid.uuid4().hex
            support.wait_process(pid, exitcode=0)
            child_value = os.read(fds[0], 100).decode('latin-1')

            self.assertNotEqual(parent_value, child_value)

    def test_uuid_weakref(self):
        # bpo-35701: check that weak referencing to a UUID object can be created
        strong = self.uuid.uuid4()
        weak = weakref.ref(strong)
        self.assertIs(strong, weak())


class CommandLineTestCases:
    uuid = None  # to be defined in subclasses

    def do_test_standalone_uuid(self, version):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            self.uuid.main()
        output = stdout.getvalue().strip()
        u = self.uuid.UUID(output)
        self.assertEqual(output, str(u))
        self.assertEqual(u.version, version)

    @mock.patch.object(sys, "argv", ["", "-u", "uuid1"])
    def test_cli_uuid1(self):
        self.do_test_standalone_uuid(1)

    @mock.patch.object(sys, "argv", ["", "-u", "uuid3", "-n", "@dns"])
    @mock.patch('sys.stderr', new_callable=io.StringIO)
    def test_cli_namespace_required_for_uuid3(self, mock_err):
        with self.assertRaises(SystemExit) as cm:
            self.uuid.main()

        # Check that exception code is the same as argparse.ArgumentParser.error
        self.assertEqual(cm.exception.code, 2)
        self.assertIn("error: Incorrect number of arguments", mock_err.getvalue())

    @mock.patch.object(sys, "argv", ["", "-u", "uuid3", "-N", "python.org"])
    @mock.patch('sys.stderr', new_callable=io.StringIO)
    def test_cli_name_required_for_uuid3(self, mock_err):
        with self.assertRaises(SystemExit) as cm:
            self.uuid.main()
        # Check that exception code is the same as argparse.ArgumentParser.error
        self.assertEqual(cm.exception.code, 2)
        self.assertIn("error: Incorrect number of arguments", mock_err.getvalue())

    @mock.patch.object(sys, "argv", [""])
    def test_cli_uuid4_outputted_with_no_args(self):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            self.uuid.main()

        output = stdout.getvalue().strip()
        uuid_output = self.uuid.UUID(output)

        # Output uuid should be in the format of uuid4
        self.assertEqual(output, str(uuid_output))
        self.assertEqual(uuid_output.version, 4)

    @mock.patch.object(sys, "argv", ["", "-C", "3"])
    def test_cli_uuid4_outputted_with_count(self):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            self.uuid.main()

        output = stdout.getvalue().strip().splitlines()

        # Check that 3 UUIDs in the format of uuid4 have been generated
        self.assertEqual(len(output), 3)
        for o in output:
            uuid_output = self.uuid.UUID(o)
            self.assertEqual(uuid_output.version, 4)

    @mock.patch.object(sys, "argv",
                       ["", "-u", "uuid3", "-n", "@dns", "-N", "python.org"])
    def test_cli_uuid3_ouputted_with_valid_namespace_and_name(self):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            self.uuid.main()

        output = stdout.getvalue().strip()
        uuid_output = self.uuid.UUID(output)

        # Output should be in the form of uuid5
        self.assertEqual(output, str(uuid_output))
        self.assertEqual(uuid_output.version, 3)

    @mock.patch.object(sys, "argv",
                       ["", "-u", "uuid5", "-n", "@dns", "-N", "python.org"])
    def test_cli_uuid5_ouputted_with_valid_namespace_and_name(self):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            self.uuid.main()

        output = stdout.getvalue().strip()
        uuid_output = self.uuid.UUID(output)

        # Output should be in the form of uuid5
        self.assertEqual(output, str(uuid_output))
        self.assertEqual(uuid_output.version, 5)

    @mock.patch.object(sys, "argv", ["", "-u", "uuid6"])
    def test_cli_uuid6(self):
        self.do_test_standalone_uuid(6)

    @mock.patch.object(sys, "argv", ["", "-u", "uuid7"])
    def test_cli_uuid7(self):
        self.do_test_standalone_uuid(7)

    @mock.patch.object(sys, "argv", ["", "-u", "uuid8"])
    def test_cli_uuid8(self):
        self.do_test_standalone_uuid(8)


@force_not_colorized_test_class
class TestUUIDWithoutExtModule(CommandLineTestCases, BaseTestUUID, unittest.TestCase):
    uuid = py_uuid


@force_not_colorized_test_class
@unittest.skipUnless(c_uuid, 'requires the C _uuid module')
class TestUUIDWithExtModule(CommandLineTestCases, BaseTestUUID, unittest.TestCase):
    uuid = c_uuid

    def check_has_stable_libuuid_extractable_node(self):
        if not self.uuid._has_stable_extractable_node:
            self.skipTest("libuuid cannot deduce MAC address")

    @unittest.skipUnless(os.name == 'posix', 'POSIX only')
    def test_unix_getnode_from_libuuid(self):
        self.check_has_stable_libuuid_extractable_node()
        script = 'import uuid; print(uuid._unix_getnode())'
        _, n_a, _ = assert_python_ok('-c', script)
        _, n_b, _ = assert_python_ok('-c', script)
        n_a, n_b = n_a.decode().strip(), n_b.decode().strip()
        self.assertTrue(n_a.isdigit())
        self.assertTrue(n_b.isdigit())
        self.assertEqual(n_a, n_b)

    @unittest.skipUnless(os.name == 'nt', 'Windows only')
    def test_windows_getnode_from_libuuid(self):
        self.check_has_stable_libuuid_extractable_node()
        script = 'import uuid; print(uuid._windll_getnode())'
        _, n_a, _ = assert_python_ok('-c', script)
        _, n_b, _ = assert_python_ok('-c', script)
        n_a, n_b = n_a.decode().strip(), n_b.decode().strip()
        self.assertTrue(n_a.isdigit())
        self.assertTrue(n_b.isdigit())
        self.assertEqual(n_a, n_b)


class BaseTestInternals:
    _uuid = py_uuid

    def check_parse_mac(self, aix):
        if not aix:
            patch = mock.patch.multiple(self.uuid,
                                        _MAC_DELIM=b':',
                                        _MAC_OMITS_LEADING_ZEROES=False)
        else:
            patch = mock.patch.multiple(self.uuid,
                                        _MAC_DELIM=b'.',
                                        _MAC_OMITS_LEADING_ZEROES=True)

        with patch:
            # Valid MAC addresses
            if not aix:
                tests = (
                    (b'52:54:00:9d:0e:67', 0x5254009d0e67),
                    (b'12:34:56:78:90:ab', 0x1234567890ab),
                )
            else:
                # AIX format
                tests = (
                    (b'fe.ad.c.1.23.4', 0xfead0c012304),
                )
            for mac, expected in tests:
                self.assertEqual(self.uuid._parse_mac(mac), expected)

            # Invalid MAC addresses
            for mac in (
                b'',
                # IPv6 addresses with same length than valid MAC address
                # (17 characters)
                b'fe80::5054:ff:fe9',
                b'123:2:3:4:5:6:7:8',
                # empty 5rd field
                b'52:54:00:9d::67',
                # only 5 fields instead of 6
                b'52:54:00:9d:0e'
                # invalid character 'x'
                b'52:54:00:9d:0e:6x'
                # dash separator
                b'52-54-00-9d-0e-67',
            ):
                if aix:
                    mac = mac.replace(b':', b'.')
                with self.subTest(mac=mac):
                    self.assertIsNone(self.uuid._parse_mac(mac))

    def test_parse_mac(self):
        self.check_parse_mac(False)

    def test_parse_mac_aix(self):
        self.check_parse_mac(True)

    def test_find_under_heading(self):
        data = '''\
Name  Mtu   Network     Address           Ipkts Ierrs    Opkts Oerrs  Coll
en0   1500  link#2      fe.ad.c.1.23.4   1714807956     0 711348489     0     0
                        01:00:5e:00:00:01
en0   1500  192.168.129 x071             1714807956     0 711348489     0     0
                        224.0.0.1
en0   1500  192.168.90  x071             1714807956     0 711348489     0     0
                        224.0.0.1
'''

        # The above data is from AIX - with '.' as _MAC_DELIM and strings
        # shorter than 17 bytes (no leading 0). (_MAC_OMITS_LEADING_ZEROES=True)
        with mock.patch.multiple(self.uuid,
                                 _MAC_DELIM=b'.',
                                 _MAC_OMITS_LEADING_ZEROES=True,
                                 _get_command_stdout=mock_get_command_stdout(data)):
            mac = self.uuid._find_mac_under_heading(
                command='netstat',
                args='-ian',
                heading=b'Address',
            )

        self.assertEqual(mac, 0xfead0c012304)

    def test_find_under_heading_ipv6(self):
        # bpo-39991: IPv6 address "fe80::5054:ff:fe9" looks like a MAC address
        # (same string length) but must be skipped
        data = '''\
Name    Mtu Network       Address              Ipkts Ierrs Idrop    Opkts Oerrs  Coll
vtnet  1500 <Link#1>      52:54:00:9d:0e:67    10017     0     0     8174     0     0
vtnet     - fe80::%vtnet0 fe80::5054:ff:fe9        0     -     -        4     -     -
vtnet     - 192.168.122.0 192.168.122.45        8844     -     -     8171     -     -
lo0   16384 <Link#2>      lo0                 260148     0     0   260148     0     0
lo0       - ::1/128       ::1                    193     -     -      193     -     -
                          ff01::1%lo0
                          ff02::2:2eb7:74fa
                          ff02::2:ff2e:b774
                          ff02::1%lo0
                          ff02::1:ff00:1%lo
lo0       - fe80::%lo0/64 fe80::1%lo0              0     -     -        0     -     -
                          ff01::1%lo0
                          ff02::2:2eb7:74fa
                          ff02::2:ff2e:b774
                          ff02::1%lo0
                          ff02::1:ff00:1%lo
lo0       - 127.0.0.0/8   127.0.0.1           259955     -     -   259955     -     -
                          224.0.0.1
'''

        with mock.patch.multiple(self.uuid,
                                 _MAC_DELIM=b':',
                                 _MAC_OMITS_LEADING_ZEROES=False,
                                 _get_command_stdout=mock_get_command_stdout(data)):
            mac = self.uuid._find_mac_under_heading(
                command='netstat',
                args='-ian',
                heading=b'Address',
            )

        self.assertEqual(mac, 0x5254009d0e67)

    def test_find_mac_near_keyword(self):
        # key and value are on the same line
        data = '''
fake      Link encap:UNSPEC  hwaddr 00-00
cscotun0  Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00
eth0      Link encap:Ethernet  HWaddr 12:34:56:78:90:ab
'''

        # The above data will only be parsed properly on non-AIX unixes.
        with mock.patch.multiple(self.uuid,
                                 _MAC_DELIM=b':',
                                 _MAC_OMITS_LEADING_ZEROES=False,
                                 _get_command_stdout=mock_get_command_stdout(data)):
            mac = self.uuid._find_mac_near_keyword(
                command='ifconfig',
                args='',
                keywords=[b'hwaddr'],
                get_word_index=lambda x: x + 1,
            )

        self.assertEqual(mac, 0x1234567890ab)

    def check_node(self, node, requires=None):
        if requires and node is None:
            self.skipTest('requires ' + requires)
        hex = '%012x' % node
        if support.verbose >= 2:
            print(hex, end=' ')
        self.assertTrue(0 < node < (1 << 48),
                        "%s is not an RFC 4122 node ID" % hex)

    @unittest.skipUnless(_uuid._ifconfig_getnode in _uuid._GETTERS,
        "ifconfig is not used for introspection on this platform")
    def test_ifconfig_getnode(self):
        node = self.uuid._ifconfig_getnode()
        self.check_node(node, 'ifconfig')

    @unittest.skipUnless(_uuid._ip_getnode in _uuid._GETTERS,
        "ip is not used for introspection on this platform")
    def test_ip_getnode(self):
        node = self.uuid._ip_getnode()
        self.check_node(node, 'ip')

    @unittest.skipUnless(_uuid._arp_getnode in _uuid._GETTERS,
        "arp is not used for introspection on this platform")
    def test_arp_getnode(self):
        node = self.uuid._arp_getnode()
        self.check_node(node, 'arp')

    @unittest.skipUnless(_uuid._lanscan_getnode in _uuid._GETTERS,
        "lanscan is not used for introspection on this platform")
    def test_lanscan_getnode(self):
        node = self.uuid._lanscan_getnode()
        self.check_node(node, 'lanscan')

    @unittest.skipUnless(_uuid._netstat_getnode in _uuid._GETTERS,
        "netstat is not used for introspection on this platform")
    def test_netstat_getnode(self):
        node = self.uuid._netstat_getnode()
        self.check_node(node, 'netstat')

    def test_random_getnode(self):
        node = self.uuid._random_getnode()
        # The multicast bit, i.e. the least significant bit of first octet,
        # must be set for randomly generated MAC addresses.  See RFC 4122,
        # $4.1.6.
        self.assertTrue(node & (1 << 40), '%012x' % node)
        self.check_node(node)

        node2 = self.uuid._random_getnode()
        self.assertNotEqual(node2, node, '%012x' % node)

class TestInternalsWithoutExtModule(BaseTestInternals, unittest.TestCase):
    uuid = py_uuid

@unittest.skipUnless(c_uuid, 'requires the C _uuid module')
class TestInternalsWithExtModule(BaseTestInternals, unittest.TestCase):
    uuid = c_uuid

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def test_unix_getnode(self):
        if not importable('_uuid') and not importable('ctypes'):
            self.skipTest("neither _uuid extension nor ctypes available")
        try: # Issues 1481, 3581: _uuid_generate_time() might be None.
            node = self.uuid._unix_getnode()
        except TypeError:
            self.skipTest('requires uuid_generate_time')
        self.check_node(node, 'unix')

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_windll_getnode(self):
        node = self.uuid._windll_getnode()
        self.check_node(node)


if __name__ == '__main__':
    unittest.main()
