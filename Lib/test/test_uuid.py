import unittest
from test import test_support
import io
import os
import uuid

def importable(name):
    try:
        __import__(name)
        return True
    except:
        return False

class TestUUID(unittest.TestCase):
    def test_UUID(self):
        equal = self.assertEqual
        ascending = []
        for (string, curly, hex, bytes, bytes_le, fields, integer, urn,
             time, clock_seq, variant, version) in [
            ('00000000-0000-0000-0000-000000000000',
             '{00000000-0000-0000-0000-000000000000}',
             '00000000000000000000000000000000',
             '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
             '\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0',
             (0, 0, 0, 0, 0, 0),
             0,
             'urn:uuid:00000000-0000-0000-0000-000000000000',
             0, 0, uuid.RESERVED_NCS, None),
            ('00010203-0405-0607-0809-0a0b0c0d0e0f',
             '{00010203-0405-0607-0809-0a0b0c0d0e0f}',
             '000102030405060708090a0b0c0d0e0f',
             '\0\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\x0d\x0e\x0f',
             '\x03\x02\x01\0\x05\x04\x07\x06\x08\t\n\x0b\x0c\x0d\x0e\x0f',
             (0x00010203L, 0x0405, 0x0607, 8, 9, 0x0a0b0c0d0e0fL),
             0x000102030405060708090a0b0c0d0e0fL,
             'urn:uuid:00010203-0405-0607-0809-0a0b0c0d0e0f',
             0x607040500010203L, 0x809, uuid.RESERVED_NCS, None),
            ('02d9e6d5-9467-382e-8f9b-9300a64ac3cd',
             '{02d9e6d5-9467-382e-8f9b-9300a64ac3cd}',
             '02d9e6d59467382e8f9b9300a64ac3cd',
             '\x02\xd9\xe6\xd5\x94\x67\x38\x2e\x8f\x9b\x93\x00\xa6\x4a\xc3\xcd',
             '\xd5\xe6\xd9\x02\x67\x94\x2e\x38\x8f\x9b\x93\x00\xa6\x4a\xc3\xcd',
             (0x02d9e6d5L, 0x9467, 0x382e, 0x8f, 0x9b, 0x9300a64ac3cdL),
             0x02d9e6d59467382e8f9b9300a64ac3cdL,
             'urn:uuid:02d9e6d5-9467-382e-8f9b-9300a64ac3cd',
             0x82e946702d9e6d5L, 0xf9b, uuid.RFC_4122, 3),
            ('12345678-1234-5678-1234-567812345678',
             '{12345678-1234-5678-1234-567812345678}',
             '12345678123456781234567812345678',
             '\x12\x34\x56\x78'*4,
             '\x78\x56\x34\x12\x34\x12\x78\x56\x12\x34\x56\x78\x12\x34\x56\x78',
             (0x12345678, 0x1234, 0x5678, 0x12, 0x34, 0x567812345678),
             0x12345678123456781234567812345678,
             'urn:uuid:12345678-1234-5678-1234-567812345678',
             0x678123412345678L, 0x1234, uuid.RESERVED_NCS, None),
            ('6ba7b810-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b810-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8109dad11d180b400c04fd430c8',
             '\x6b\xa7\xb8\x10\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             '\x10\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b810L, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8L),
             0x6ba7b8109dad11d180b400c04fd430c8L,
             'urn:uuid:6ba7b810-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b810L, 0xb4, uuid.RFC_4122, 1),
            ('6ba7b811-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b811-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8119dad11d180b400c04fd430c8',
             '\x6b\xa7\xb8\x11\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             '\x11\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b811L, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8L),
             0x6ba7b8119dad11d180b400c04fd430c8L,
             'urn:uuid:6ba7b811-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b811L, 0xb4, uuid.RFC_4122, 1),
            ('6ba7b812-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b812-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8129dad11d180b400c04fd430c8',
             '\x6b\xa7\xb8\x12\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             '\x12\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b812L, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8L),
             0x6ba7b8129dad11d180b400c04fd430c8L,
             'urn:uuid:6ba7b812-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b812L, 0xb4, uuid.RFC_4122, 1),
            ('6ba7b814-9dad-11d1-80b4-00c04fd430c8',
             '{6ba7b814-9dad-11d1-80b4-00c04fd430c8}',
             '6ba7b8149dad11d180b400c04fd430c8',
             '\x6b\xa7\xb8\x14\x9d\xad\x11\xd1\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             '\x14\xb8\xa7\x6b\xad\x9d\xd1\x11\x80\xb4\x00\xc0\x4f\xd4\x30\xc8',
             (0x6ba7b814L, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00c04fd430c8L),
             0x6ba7b8149dad11d180b400c04fd430c8L,
             'urn:uuid:6ba7b814-9dad-11d1-80b4-00c04fd430c8',
             0x1d19dad6ba7b814L, 0xb4, uuid.RFC_4122, 1),
            ('7d444840-9dc0-11d1-b245-5ffdce74fad2',
             '{7d444840-9dc0-11d1-b245-5ffdce74fad2}',
             '7d4448409dc011d1b2455ffdce74fad2',
             '\x7d\x44\x48\x40\x9d\xc0\x11\xd1\xb2\x45\x5f\xfd\xce\x74\xfa\xd2',
             '\x40\x48\x44\x7d\xc0\x9d\xd1\x11\xb2\x45\x5f\xfd\xce\x74\xfa\xd2',
             (0x7d444840L, 0x9dc0, 0x11d1, 0xb2, 0x45, 0x5ffdce74fad2L),
             0x7d4448409dc011d1b2455ffdce74fad2L,
             'urn:uuid:7d444840-9dc0-11d1-b245-5ffdce74fad2',
             0x1d19dc07d444840L, 0x3245, uuid.RFC_4122, 1),
            ('e902893a-9d22-3c7e-a7b8-d6e313b71d9f',
             '{e902893a-9d22-3c7e-a7b8-d6e313b71d9f}',
             'e902893a9d223c7ea7b8d6e313b71d9f',
             '\xe9\x02\x89\x3a\x9d\x22\x3c\x7e\xa7\xb8\xd6\xe3\x13\xb7\x1d\x9f',
             '\x3a\x89\x02\xe9\x22\x9d\x7e\x3c\xa7\xb8\xd6\xe3\x13\xb7\x1d\x9f',
             (0xe902893aL, 0x9d22, 0x3c7e, 0xa7, 0xb8, 0xd6e313b71d9fL),
             0xe902893a9d223c7ea7b8d6e313b71d9fL,
             'urn:uuid:e902893a-9d22-3c7e-a7b8-d6e313b71d9f',
             0xc7e9d22e902893aL, 0x27b8, uuid.RFC_4122, 3),
            ('eb424026-6f54-4ef8-a4d0-bb658a1fc6cf',
             '{eb424026-6f54-4ef8-a4d0-bb658a1fc6cf}',
             'eb4240266f544ef8a4d0bb658a1fc6cf',
             '\xeb\x42\x40\x26\x6f\x54\x4e\xf8\xa4\xd0\xbb\x65\x8a\x1f\xc6\xcf',
             '\x26\x40\x42\xeb\x54\x6f\xf8\x4e\xa4\xd0\xbb\x65\x8a\x1f\xc6\xcf',
             (0xeb424026L, 0x6f54, 0x4ef8, 0xa4, 0xd0, 0xbb658a1fc6cfL),
             0xeb4240266f544ef8a4d0bb658a1fc6cfL,
             'urn:uuid:eb424026-6f54-4ef8-a4d0-bb658a1fc6cf',
             0xef86f54eb424026L, 0x24d0, uuid.RFC_4122, 4),
            ('f81d4fae-7dec-11d0-a765-00a0c91e6bf6',
             '{f81d4fae-7dec-11d0-a765-00a0c91e6bf6}',
             'f81d4fae7dec11d0a76500a0c91e6bf6',
             '\xf8\x1d\x4f\xae\x7d\xec\x11\xd0\xa7\x65\x00\xa0\xc9\x1e\x6b\xf6',
             '\xae\x4f\x1d\xf8\xec\x7d\xd0\x11\xa7\x65\x00\xa0\xc9\x1e\x6b\xf6',
             (0xf81d4faeL, 0x7dec, 0x11d0, 0xa7, 0x65, 0x00a0c91e6bf6L),
             0xf81d4fae7dec11d0a76500a0c91e6bf6L,
             'urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6',
             0x1d07decf81d4faeL, 0x2765, uuid.RFC_4122, 1),
            ('fffefdfc-fffe-fffe-fffe-fffefdfcfbfa',
             '{fffefdfc-fffe-fffe-fffe-fffefdfcfbfa}',
             'fffefdfcfffefffefffefffefdfcfbfa',
             '\xff\xfe\xfd\xfc\xff\xfe\xff\xfe\xff\xfe\xff\xfe\xfd\xfc\xfb\xfa',
             '\xfc\xfd\xfe\xff\xfe\xff\xfe\xff\xff\xfe\xff\xfe\xfd\xfc\xfb\xfa',
             (0xfffefdfcL, 0xfffe, 0xfffe, 0xff, 0xfe, 0xfffefdfcfbfaL),
             0xfffefdfcfffefffefffefffefdfcfbfaL,
             'urn:uuid:fffefdfc-fffe-fffe-fffe-fffefdfcfbfa',
             0xffefffefffefdfcL, 0x3ffe, uuid.RESERVED_FUTURE, None),
            ('ffffffff-ffff-ffff-ffff-ffffffffffff',
             '{ffffffff-ffff-ffff-ffff-ffffffffffff}',
             'ffffffffffffffffffffffffffffffff',
             '\xff'*16,
             '\xff'*16,
             (0xffffffffL, 0xffffL, 0xffffL, 0xff, 0xff, 0xffffffffffffL),
             0xffffffffffffffffffffffffffffffffL,
             'urn:uuid:ffffffff-ffff-ffff-ffff-ffffffffffff',
             0xfffffffffffffffL, 0x3fff, uuid.RESERVED_FUTURE, None),
            ]:
            equivalents = []
            # Construct each UUID in several different ways.
            for u in [uuid.UUID(string), uuid.UUID(curly), uuid.UUID(hex),
                      uuid.UUID(bytes=bytes), uuid.UUID(bytes_le=bytes_le),
                      uuid.UUID(fields=fields), uuid.UUID(int=integer),
                      uuid.UUID(urn)]:
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
            ascending.append(u)

        # Test comparison of UUIDs.
        for i in range(len(ascending)):
            for j in range(len(ascending)):
                equal(cmp(i, j), cmp(ascending[i], ascending[j]))

        # Test sorting of UUIDs (above list is in ascending order).
        resorted = ascending[:]
        resorted.reverse()
        resorted.sort()
        equal(ascending, resorted)

    def test_exceptions(self):
        badvalue = lambda f: self.assertRaises(ValueError, f)
        badtype = lambda f: self.assertRaises(TypeError, f)

        # Badly formed hex strings.
        badvalue(lambda: uuid.UUID(''))
        badvalue(lambda: uuid.UUID('abc'))
        badvalue(lambda: uuid.UUID('1234567812345678123456781234567'))
        badvalue(lambda: uuid.UUID('123456781234567812345678123456789'))
        badvalue(lambda: uuid.UUID('123456781234567812345678z2345678'))

        # Badly formed bytes.
        badvalue(lambda: uuid.UUID(bytes='abc'))
        badvalue(lambda: uuid.UUID(bytes='\0'*15))
        badvalue(lambda: uuid.UUID(bytes='\0'*17))

        # Badly formed bytes_le.
        badvalue(lambda: uuid.UUID(bytes_le='abc'))
        badvalue(lambda: uuid.UUID(bytes_le='\0'*15))
        badvalue(lambda: uuid.UUID(bytes_le='\0'*17))

        # Badly formed fields.
        badvalue(lambda: uuid.UUID(fields=(1,)))
        badvalue(lambda: uuid.UUID(fields=(1, 2, 3, 4, 5)))
        badvalue(lambda: uuid.UUID(fields=(1, 2, 3, 4, 5, 6, 7)))

        # Field values out of range.
        badvalue(lambda: uuid.UUID(fields=(-1, 0, 0, 0, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0x100000000L, 0, 0, 0, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, -1, 0, 0, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0x10000L, 0, 0, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, -1, 0, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, 0x10000L, 0, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, 0, -1, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, 0, 0x100L, 0, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, 0, 0, -1, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, 0, 0, 0x100L, 0)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, 0, 0, 0, -1)))
        badvalue(lambda: uuid.UUID(fields=(0, 0, 0, 0, 0, 0x1000000000000L)))

        # Version number out of range.
        badvalue(lambda: uuid.UUID('00'*16, version=0))
        badvalue(lambda: uuid.UUID('00'*16, version=6))

        # Integer value out of range.
        badvalue(lambda: uuid.UUID(int=-1))
        badvalue(lambda: uuid.UUID(int=1<<128L))

        # Must supply exactly one of hex, bytes, fields, int.
        h, b, f, i = '00'*16, '\0'*16, (0, 0, 0, 0, 0, 0), 0
        uuid.UUID(h)
        uuid.UUID(hex=h)
        uuid.UUID(bytes=b)
        uuid.UUID(bytes_le=b)
        uuid.UUID(fields=f)
        uuid.UUID(int=i)

        # Wrong number of arguments (positional).
        badtype(lambda: uuid.UUID())
        badtype(lambda: uuid.UUID(h, b))
        badtype(lambda: uuid.UUID(h, b, b))
        badtype(lambda: uuid.UUID(h, b, b, f))
        badtype(lambda: uuid.UUID(h, b, b, f, i))

        # Duplicate arguments.
        for hh in [[], [('hex', h)]]:
            for bb in [[], [('bytes', b)]]:
                for bble in [[], [('bytes_le', b)]]:
                    for ii in [[], [('int', i)]]:
                        for ff in [[], [('fields', f)]]:
                            args = dict(hh + bb + bble + ii + ff)
                            if len(args) != 0:
                                badtype(lambda: uuid.UUID(h, **args))
                            if len(args) != 1:
                                badtype(lambda: uuid.UUID(**args))

        # Immutability.
        u = uuid.UUID(h)
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

    def test_getnode(self):
        node1 = uuid.getnode()
        self.assertTrue(0 < node1 < (1 << 48), '%012x' % node1)

        # Test it again to ensure consistency.
        node2 = uuid.getnode()
        self.assertEqual(node1, node2, '%012x != %012x' % (node1, node2))

    # bpo-32502: UUID1 requires a 48-bit identifier, but hardware identifiers
    # need not necessarily be 48 bits (e.g., EUI-64).
    def test_uuid1_eui64(self):
        # Confirm that uuid.getnode ignores hardware addresses larger than 48
        # bits. Mock out each platform's *_getnode helper functions to return
        # something just larger than 48 bits to test. This will cause
        # uuid.getnode to fall back on uuid._random_getnode, which will
        # generate a valid value.
        too_large_getter = lambda: 1 << 48

        uuid_real__node = uuid._node
        uuid_real__NODE_GETTERS_WIN32 = uuid._NODE_GETTERS_WIN32
        uuid_real__NODE_GETTERS_UNIX = uuid._NODE_GETTERS_UNIX
        uuid._node = None
        uuid._NODE_GETTERS_WIN32 = [too_large_getter]
        uuid._NODE_GETTERS_UNIX = [too_large_getter]
        try:
            node = uuid.getnode()
        finally:
            uuid._node = uuid_real__node
            uuid._NODE_GETTERS_WIN32 = uuid_real__NODE_GETTERS_WIN32
            uuid._NODE_GETTERS_UNIX = uuid_real__NODE_GETTERS_UNIX

        self.assertTrue(0 < node < (1 << 48), '%012x' % node)

        # Confirm that uuid1 can use the generated node, i.e., the that
        # uuid.getnode fell back on uuid._random_getnode() rather than using
        # the value from too_large_getter above.
        try:
            uuid.uuid1(node=node)
        except ValueError as e:
            self.fail('uuid1 was given an invalid node ID')

    @unittest.skipUnless(importable('ctypes'), 'requires ctypes')
    def test_uuid1(self):
        equal = self.assertEqual

        # Make sure uuid1() generates UUIDs that are actually version 1.
        for u in [uuid.uuid1() for i in range(10)]:
            equal(u.variant, uuid.RFC_4122)
            equal(u.version, 1)

        # Make sure the generated UUIDs are actually unique.
        uuids = {}
        for u in [uuid.uuid1() for i in range(1000)]:
            uuids[u] = 1
        equal(len(uuids.keys()), 1000)

        # Make sure the supplied node ID appears in the UUID.
        u = uuid.uuid1(0)
        equal(u.node, 0)
        u = uuid.uuid1(0x123456789abc)
        equal(u.node, 0x123456789abc)
        u = uuid.uuid1(0xffffffffffff)
        equal(u.node, 0xffffffffffff)

        # Make sure the supplied clock sequence appears in the UUID.
        u = uuid.uuid1(0x123456789abc, 0)
        equal(u.node, 0x123456789abc)
        equal(((u.clock_seq_hi_variant & 0x3f) << 8) | u.clock_seq_low, 0)
        u = uuid.uuid1(0x123456789abc, 0x1234)
        equal(u.node, 0x123456789abc)
        equal(((u.clock_seq_hi_variant & 0x3f) << 8) |
                         u.clock_seq_low, 0x1234)
        u = uuid.uuid1(0x123456789abc, 0x3fff)
        equal(u.node, 0x123456789abc)
        equal(((u.clock_seq_hi_variant & 0x3f) << 8) |
                         u.clock_seq_low, 0x3fff)

    def test_uuid3(self):
        equal = self.assertEqual

        # Test some known version-3 UUIDs.
        for u, v in [(uuid.uuid3(uuid.NAMESPACE_DNS, 'python.org'),
                      '6fa459ea-ee8a-3ca4-894e-db77e160355e'),
                     (uuid.uuid3(uuid.NAMESPACE_URL, 'http://python.org/'),
                      '9fe8e8c4-aaa8-32a9-a55c-4535a88b748d'),
                     (uuid.uuid3(uuid.NAMESPACE_OID, '1.3.6.1'),
                      'dd1a1cef-13d5-368a-ad82-eca71acd4cd1'),
                     (uuid.uuid3(uuid.NAMESPACE_X500, 'c=ca'),
                      '658d3002-db6b-3040-a1d1-8ddd7d189a4d'),
                    ]:
            equal(u.variant, uuid.RFC_4122)
            equal(u.version, 3)
            equal(u, uuid.UUID(v))
            equal(str(u), v)

    @unittest.skipUnless(importable('ctypes'), 'requires ctypes')
    def test_uuid4(self):
        equal = self.assertEqual

        # Make sure uuid4() generates UUIDs that are actually version 4.
        for u in [uuid.uuid4() for i in range(10)]:
            equal(u.variant, uuid.RFC_4122)
            equal(u.version, 4)

        # Make sure the generated UUIDs are actually unique.
        uuids = {}
        for u in [uuid.uuid4() for i in range(1000)]:
            uuids[u] = 1
        equal(len(uuids.keys()), 1000)

    def test_uuid5(self):
        equal = self.assertEqual

        # Test some known version-5 UUIDs.
        for u, v in [(uuid.uuid5(uuid.NAMESPACE_DNS, 'python.org'),
                      '886313e1-3b8a-5372-9b90-0c9aee199e5d'),
                     (uuid.uuid5(uuid.NAMESPACE_URL, 'http://python.org/'),
                      '4c565f0d-3f5a-5890-b41b-20cf47701c5e'),
                     (uuid.uuid5(uuid.NAMESPACE_OID, '1.3.6.1'),
                      '1447fa61-5277-5fef-a9b3-fbc6e44f4af3'),
                     (uuid.uuid5(uuid.NAMESPACE_X500, 'c=ca'),
                      'cc957dd1-a972-5349-98cd-874190002798'),
                    ]:
            equal(u.variant, uuid.RFC_4122)
            equal(u.version, 5)
            equal(u, uuid.UUID(v))
            equal(str(u), v)

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def testIssue8621(self):
        # On at least some versions of OSX uuid.uuid4 generates
        # the same sequence of UUIDs in the parent and any
        # children started using fork.
        fds = os.pipe()
        pid = os.fork()
        if pid == 0:
            os.close(fds[0])
            value = uuid.uuid4()
            os.write(fds[1], value.hex)
            os._exit(0)

        else:
            os.close(fds[1])
            self.addCleanup(os.close, fds[0])
            parent_value = uuid.uuid4().hex
            os.waitpid(pid, 0)
            child_value = os.read(fds[0], 100)

            self.assertNotEqual(parent_value, child_value)


class TestInternals(unittest.TestCase):
    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def test_find_mac(self):
        data = '''\

fake hwaddr
cscotun0  Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00
eth0      Link encap:Ethernet  HWaddr 12:34:56:78:90:ab
'''
        def mock_popen(cmd):
            return io.BytesIO(data)

        path = os.environ.get("PATH", os.defpath).split(os.pathsep)
        path.extend(('/sbin', '/usr/sbin'))
        for dir in path:
            executable = os.path.join(dir, 'ifconfig')
            if (os.path.exists(executable) and
                os.access(executable, os.F_OK | os.X_OK) and
                not os.path.isdir(executable)):
                break
        else:
            self.skipTest('requires ifconfig')

        with test_support.swap_attr(os, 'popen', mock_popen):
            mac = uuid._find_mac(
                command='ifconfig',
                args='',
                hw_identifiers=['hwaddr'],
                get_index=lambda x: x + 1,
            )
            self.assertEqual(mac, 0x1234567890ab)

    def check_node(self, node, requires=None):
        if requires and node is None:
            self.skipTest('requires ' + requires)
        hex = '%012x' % node
        if test_support.verbose >= 2:
            print hex + ' ',
        self.assertTrue(0 < node < (1L << 48),
                        "%s is not an RFC 4122 node ID" % hex)

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def test_ifconfig_getnode(self):
        node = uuid._ifconfig_getnode()
        self.check_node(node, 'ifconfig')

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def test_arp_getnode(self):
        node = uuid._arp_getnode()
        self.check_node(node, 'arp')

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def test_lanscan_getnode(self):
        node = uuid._lanscan_getnode()
        self.check_node(node, 'lanscan')

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    def test_netstat_getnode(self):
        node = uuid._netstat_getnode()
        self.check_node(node, 'netstat')

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_ipconfig_getnode(self):
        node = uuid._ipconfig_getnode()
        self.check_node(node, 'ipconfig')

    @unittest.skipUnless(importable('win32wnet'), 'requires win32wnet')
    @unittest.skipUnless(importable('netbios'), 'requires netbios')
    def test_netbios_getnode(self):
        node = uuid._netbios_getnode()
        self.check_node(node)

    def test_random_getnode(self):
        node = uuid._random_getnode()
        # The multicast bit, i.e. the least significant bit of first octet,
        # must be set for randomly generated MAC addresses.  See RFC 4122,
        # $4.1.6.
        self.assertTrue(node & (1 << 40), '%012x' % node)
        self.check_node(node)

    @unittest.skipUnless(os.name == 'posix', 'requires Posix')
    @unittest.skipUnless(importable('ctypes'), 'requires ctypes')
    def test_unixdll_getnode(self):
        try: # Issues 1481, 3581: _uuid_generate_time() might be None.
            node = uuid._unixdll_getnode()
        except TypeError:
            self.skipTest('requires uuid_generate_time')
        self.check_node(node)

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    @unittest.skipUnless(importable('ctypes'), 'requires ctypes')
    def test_windll_getnode(self):
        node = uuid._windll_getnode()
        self.check_node(node)


def test_main():
    test_support.run_unittest(TestUUID, TestInternals)

if __name__ == '__main__':
    test_main()
