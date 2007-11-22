"""Unit tests for the bytes and bytearray types.

XXX This is a mess.  Common tests should be moved to buffer_tests.py,
which itself ought to be unified with string_tests.py (and the latter
should be modernized).
"""

import os
import re
import sys
import copy
import pickle
import tempfile
import unittest
import warnings
import test.test_support
import test.string_tests
import test.buffer_tests


class BytesTest(unittest.TestCase):

    def setUp(self):
        self.warning_filters = warnings.filters[:]

    def tearDown(self):
        warnings.filters = self.warning_filters

    def test_basics(self):
        b = bytearray()
        self.assertEqual(type(b), bytearray)
        self.assertEqual(b.__class__, bytearray)

    def test_empty_sequence(self):
        b = bytearray()
        self.assertEqual(len(b), 0)
        self.assertRaises(IndexError, lambda: b[0])
        self.assertRaises(IndexError, lambda: b[1])
        self.assertRaises(IndexError, lambda: b[sys.maxint])
        self.assertRaises(IndexError, lambda: b[sys.maxint+1])
        self.assertRaises(IndexError, lambda: b[10**100])
        self.assertRaises(IndexError, lambda: b[-1])
        self.assertRaises(IndexError, lambda: b[-2])
        self.assertRaises(IndexError, lambda: b[-sys.maxint])
        self.assertRaises(IndexError, lambda: b[-sys.maxint-1])
        self.assertRaises(IndexError, lambda: b[-sys.maxint-2])
        self.assertRaises(IndexError, lambda: b[-10**100])

    def test_from_list(self):
        ints = list(range(256))
        b = bytearray(i for i in ints)
        self.assertEqual(len(b), 256)
        self.assertEqual(list(b), ints)

    def test_from_index(self):
        class C:
            def __init__(self, i=0):
                self.i = i
            def __index__(self):
                return self.i
        b = bytearray([C(), C(1), C(254), C(255)])
        self.assertEqual(list(b), [0, 1, 254, 255])
        self.assertRaises(ValueError, bytearray, [C(-1)])
        self.assertRaises(ValueError, bytearray, [C(256)])

    def test_from_ssize(self):
        self.assertEqual(bytearray(0), b'')
        self.assertEqual(bytearray(1), b'\x00')
        self.assertEqual(bytearray(5), b'\x00\x00\x00\x00\x00')
        self.assertRaises(ValueError, bytearray, -1)

        self.assertEqual(bytearray('0', 'ascii'), b'0')
        self.assertEqual(bytearray(b'0'), b'0')

    def test_constructor_type_errors(self):
        self.assertRaises(TypeError, bytearray, 0.0)
        class C:
            pass
        self.assertRaises(TypeError, bytearray, ["0"])
        self.assertRaises(TypeError, bytearray, [0.0])
        self.assertRaises(TypeError, bytearray, [None])
        self.assertRaises(TypeError, bytearray, [C()])

    def test_constructor_value_errors(self):
        self.assertRaises(ValueError, bytearray, [-1])
        self.assertRaises(ValueError, bytearray, [-sys.maxint])
        self.assertRaises(ValueError, bytearray, [-sys.maxint-1])
        self.assertRaises(ValueError, bytearray, [-sys.maxint-2])
        self.assertRaises(ValueError, bytearray, [-10**100])
        self.assertRaises(ValueError, bytearray, [256])
        self.assertRaises(ValueError, bytearray, [257])
        self.assertRaises(ValueError, bytearray, [sys.maxint])
        self.assertRaises(ValueError, bytearray, [sys.maxint+1])
        self.assertRaises(ValueError, bytearray, [10**100])

    def test_repr_str(self):
        warnings.simplefilter('ignore', BytesWarning)
        for f in str, repr:
            self.assertEqual(f(bytearray()), "bytearray(b'')")
            self.assertEqual(f(bytearray([0])), "bytearray(b'\\x00')")
            self.assertEqual(f(bytearray([0, 1, 254, 255])),
                             "bytearray(b'\\x00\\x01\\xfe\\xff')")
            self.assertEqual(f(b"abc"), "b'abc'")
            self.assertEqual(f(b"'"), '''b"'"''')
            self.assertEqual(f(b"'\""), r"""b'\'"'""")


    def test_compare(self):
        b1 = bytearray([1, 2, 3])
        b2 = bytearray([1, 2, 3])
        b3 = bytearray([1, 3])

        self.assertEqual(b1, b2)
        self.failUnless(b2 != b3)
        self.failUnless(b1 <= b2)
        self.failUnless(b1 <= b3)
        self.failUnless(b1 <  b3)
        self.failUnless(b1 >= b2)
        self.failUnless(b3 >= b2)
        self.failUnless(b3 >  b2)

        self.failIf(b1 != b2)
        self.failIf(b2 == b3)
        self.failIf(b1 >  b2)
        self.failIf(b1 >  b3)
        self.failIf(b1 >= b3)
        self.failIf(b1 <  b2)
        self.failIf(b3 <  b2)
        self.failIf(b3 <= b2)

    def test_compare_bytes_to_bytearray(self):
        self.assertEqual(b"abc" == bytes(b"abc"), True)
        self.assertEqual(b"ab" != bytes(b"abc"), True)
        self.assertEqual(b"ab" <= bytes(b"abc"), True)
        self.assertEqual(b"ab" < bytes(b"abc"), True)
        self.assertEqual(b"abc" >= bytes(b"ab"), True)
        self.assertEqual(b"abc" > bytes(b"ab"), True)

        self.assertEqual(b"abc" != bytes(b"abc"), False)
        self.assertEqual(b"ab" == bytes(b"abc"), False)
        self.assertEqual(b"ab" > bytes(b"abc"), False)
        self.assertEqual(b"ab" >= bytes(b"abc"), False)
        self.assertEqual(b"abc" < bytes(b"ab"), False)
        self.assertEqual(b"abc" <= bytes(b"ab"), False)

        self.assertEqual(bytes(b"abc") == b"abc", True)
        self.assertEqual(bytes(b"ab") != b"abc", True)
        self.assertEqual(bytes(b"ab") <= b"abc", True)
        self.assertEqual(bytes(b"ab") < b"abc", True)
        self.assertEqual(bytes(b"abc") >= b"ab", True)
        self.assertEqual(bytes(b"abc") > b"ab", True)

        self.assertEqual(bytes(b"abc") != b"abc", False)
        self.assertEqual(bytes(b"ab") == b"abc", False)
        self.assertEqual(bytes(b"ab") > b"abc", False)
        self.assertEqual(bytes(b"ab") >= b"abc", False)
        self.assertEqual(bytes(b"abc") < b"ab", False)
        self.assertEqual(bytes(b"abc") <= b"ab", False)

    def test_compare_to_str(self):
        warnings.simplefilter('ignore', BytesWarning)
        # Byte comparisons with unicode should always fail!
        # Test this for all expected byte orders and Unicode character sizes
        self.assertEqual(b"\0a\0b\0c" == "abc", False)
        self.assertEqual(b"\0\0\0a\0\0\0b\0\0\0c" == "abc", False)
        self.assertEqual(b"a\0b\0c\0" == "abc", False)
        self.assertEqual(b"a\0\0\0b\0\0\0c\0\0\0" == "abc", False)
        self.assertEqual(bytearray() == str(), False)
        self.assertEqual(bytearray() != str(), True)

    def test_nohash(self):
        self.assertRaises(TypeError, hash, bytearray())

    def test_doc(self):
        self.failUnless(bytearray.__doc__ != None)
        self.failUnless(bytearray.__doc__.startswith("bytearray("), bytearray.__doc__)
        self.failUnless(bytes.__doc__ != None)
        self.failUnless(bytes.__doc__.startswith("bytes("), bytes.__doc__)

    def test_bytearray_api(self):
        short_sample = b"Hello world\n"
        sample = short_sample + b"\0"*(20 - len(short_sample))
        tfn = tempfile.mktemp()
        try:
            # Prepare
            with open(tfn, "wb") as f:
                f.write(short_sample)
            # Test readinto
            with open(tfn, "rb") as f:
                b = bytearray(20)
                n = f.readinto(b)
            self.assertEqual(n, len(short_sample))
            self.assertEqual(list(b), list(sample))
            # Test writing in binary mode
            with open(tfn, "wb") as f:
                f.write(b)
            with open(tfn, "rb") as f:
                self.assertEqual(f.read(), sample)
            # Text mode is ambiguous; don't test
        finally:
            try:
                os.remove(tfn)
            except os.error:
                pass

    def test_reversed(self):
        input = list(map(ord, "Hello"))
        b = bytearray(input)
        output = list(reversed(b))
        input.reverse()
        self.assertEqual(output, input)

    def test_reverse(self):
        b = bytearray(b'hello')
        self.assertEqual(b.reverse(), None)
        self.assertEqual(b, b'olleh')
        b = bytearray(b'hello1') # test even number of items
        b.reverse()
        self.assertEqual(b, b'1olleh')
        b = bytearray()
        b.reverse()
        self.assertFalse(b)

    def test_getslice(self):
        def by(s):
            return bytearray(map(ord, s))
        b = by("Hello, world")

        self.assertEqual(b[:5], by("Hello"))
        self.assertEqual(b[1:5], by("ello"))
        self.assertEqual(b[5:7], by(", "))
        self.assertEqual(b[7:], by("world"))
        self.assertEqual(b[7:12], by("world"))
        self.assertEqual(b[7:100], by("world"))

        self.assertEqual(b[:-7], by("Hello"))
        self.assertEqual(b[-11:-7], by("ello"))
        self.assertEqual(b[-7:-5], by(", "))
        self.assertEqual(b[-5:], by("world"))
        self.assertEqual(b[-5:12], by("world"))
        self.assertEqual(b[-5:100], by("world"))
        self.assertEqual(b[-100:5], by("Hello"))

    def test_extended_getslice(self):
        # Test extended slicing by comparing with list slicing.
        L = list(range(255))
        b = bytearray(L)
        indices = (0, None, 1, 3, 19, 100, -1, -2, -31, -100)
        for start in indices:
            for stop in indices:
                # Skip step 0 (invalid)
                for step in indices[1:]:
                    self.assertEqual(b[start:stop:step], bytearray(L[start:stop:step]))

    def test_regexps(self):
        def by(s):
            return bytearray(map(ord, s))
        b = by("Hello, world")
        self.assertEqual(re.findall(r"\w+", b), [by("Hello"), by("world")])

    def test_setitem(self):
        b = bytearray([1, 2, 3])
        b[1] = 100
        self.assertEqual(b, bytearray([1, 100, 3]))
        b[-1] = 200
        self.assertEqual(b, bytearray([1, 100, 200]))
        class C:
            def __init__(self, i=0):
                self.i = i
            def __index__(self):
                return self.i
        b[0] = C(10)
        self.assertEqual(b, bytearray([10, 100, 200]))
        try:
            b[3] = 0
            self.fail("Didn't raise IndexError")
        except IndexError:
            pass
        try:
            b[-10] = 0
            self.fail("Didn't raise IndexError")
        except IndexError:
            pass
        try:
            b[0] = 256
            self.fail("Didn't raise ValueError")
        except ValueError:
            pass
        try:
            b[0] = C(-1)
            self.fail("Didn't raise ValueError")
        except ValueError:
            pass
        try:
            b[0] = None
            self.fail("Didn't raise TypeError")
        except TypeError:
            pass

    def test_delitem(self):
        b = bytearray(range(10))
        del b[0]
        self.assertEqual(b, bytearray(range(1, 10)))
        del b[-1]
        self.assertEqual(b, bytearray(range(1, 9)))
        del b[4]
        self.assertEqual(b, bytearray([1, 2, 3, 4, 6, 7, 8]))

    def test_setslice(self):
        b = bytearray(range(10))
        self.assertEqual(list(b), list(range(10)))

        b[0:5] = bytearray([1, 1, 1, 1, 1])
        self.assertEqual(b, bytearray([1, 1, 1, 1, 1, 5, 6, 7, 8, 9]))

        del b[0:-5]
        self.assertEqual(b, bytearray([5, 6, 7, 8, 9]))

        b[0:0] = bytearray([0, 1, 2, 3, 4])
        self.assertEqual(b, bytearray(range(10)))

        b[-7:-3] = bytearray([100, 101])
        self.assertEqual(b, bytearray([0, 1, 2, 100, 101, 7, 8, 9]))

        b[3:5] = [3, 4, 5, 6]
        self.assertEqual(b, bytearray(range(10)))

        b[3:0] = [42, 42, 42]
        self.assertEqual(b, bytearray([0, 1, 2, 42, 42, 42, 3, 4, 5, 6, 7, 8, 9]))

    def test_extended_set_del_slice(self):
        indices = (0, None, 1, 3, 19, 300, -1, -2, -31, -300)
        for start in indices:
            for stop in indices:
                # Skip invalid step 0
                for step in indices[1:]:
                    L = list(range(255))
                    b = bytearray(L)
                    # Make sure we have a slice of exactly the right length,
                    # but with different data.
                    data = L[start:stop:step]
                    data.reverse()
                    L[start:stop:step] = data
                    b[start:stop:step] = data
                    self.assertEquals(b, bytearray(L))

                    del L[start:stop:step]
                    del b[start:stop:step]
                    self.assertEquals(b, bytearray(L))

    def test_setslice_trap(self):
        # This test verifies that we correctly handle assigning self
        # to a slice of self (the old Lambert Meertens trap).
        b = bytearray(range(256))
        b[8:] = b
        self.assertEqual(b, bytearray(list(range(8)) + list(range(256))))

    def test_encoding(self):
        sample = "Hello world\n\u1234\u5678\u9abc\udef0"
        for enc in ("utf8", "utf16"):
            b = bytearray(sample, enc)
            self.assertEqual(b, bytearray(sample.encode(enc)))
        self.assertRaises(UnicodeEncodeError, bytearray, sample, "latin1")
        b = bytearray(sample, "latin1", "ignore")
        self.assertEqual(b, bytearray(sample[:-4], "utf-8"))

    def test_decode(self):
        sample = "Hello world\n\u1234\u5678\u9abc\def0\def0"
        for enc in ("utf8", "utf16"):
            b = bytearray(sample, enc)
            self.assertEqual(b.decode(enc), sample)
        sample = "Hello world\n\x80\x81\xfe\xff"
        b = bytearray(sample, "latin1")
        self.assertRaises(UnicodeDecodeError, b.decode, "utf8")
        self.assertEqual(b.decode("utf8", "ignore"), "Hello world\n")

    def test_from_bytearray(self):
        sample = bytes(b"Hello world\n\x80\x81\xfe\xff")
        buf = memoryview(sample)
        b = bytearray(buf)
        self.assertEqual(b, bytearray(sample))

    def test_to_str(self):
        warnings.simplefilter('ignore', BytesWarning)
        self.assertEqual(str(b''), "b''")
        self.assertEqual(str(b'x'), "b'x'")
        self.assertEqual(str(b'\x80'), "b'\\x80'")

    def test_from_int(self):
        b = bytearray(0)
        self.assertEqual(b, bytearray())
        b = bytearray(10)
        self.assertEqual(b, bytearray([0]*10))
        b = bytearray(10000)
        self.assertEqual(b, bytearray([0]*10000))

    def test_concat(self):
        b1 = b"abc"
        b2 = b"def"
        self.assertEqual(b1 + b2, b"abcdef")
        self.assertEqual(b1 + bytes(b"def"), b"abcdef")
        self.assertEqual(bytes(b"def") + b1, b"defabc")
        self.assertRaises(TypeError, lambda: b1 + "def")
        self.assertRaises(TypeError, lambda: "abc" + b2)

    def test_repeat(self):
        for b in b"abc", bytearray(b"abc"):
            self.assertEqual(b * 3, b"abcabcabc")
            self.assertEqual(b * 0, b"")
            self.assertEqual(b * -1, b"")
            self.assertRaises(TypeError, lambda: b * 3.14)
            self.assertRaises(TypeError, lambda: 3.14 * b)
            # XXX Shouldn't bytes and bytearray agree on what to raise?
            self.assertRaises((OverflowError, MemoryError),
                              lambda: b * sys.maxint)

    def test_repeat_1char(self):
        self.assertEqual(b'x'*100, bytearray([ord('x')]*100))

    def test_iconcat(self):
        b = bytearray(b"abc")
        b1 = b
        b += b"def"
        self.assertEqual(b, b"abcdef")
        self.assertEqual(b, b1)
        self.failUnless(b is b1)
        b += b"xyz"
        self.assertEqual(b, b"abcdefxyz")
        try:
            b += ""
        except TypeError:
            pass
        else:
            self.fail("bytes += unicode didn't raise TypeError")

    def test_irepeat(self):
        b = bytearray(b"abc")
        b1 = b
        b *= 3
        self.assertEqual(b, b"abcabcabc")
        self.assertEqual(b, b1)
        self.failUnless(b is b1)

    def test_irepeat_1char(self):
        b = bytearray(b"x")
        b1 = b
        b *= 100
        self.assertEqual(b, b"x"*100)
        self.assertEqual(b, b1)
        self.failUnless(b is b1)

    def test_contains(self):
        for b in b"abc", bytearray(b"abc"):
            self.failUnless(ord('a') in b)
            self.failUnless(int(ord('a')) in b)
            self.failIf(200 in b)
            self.failIf(200 in b)
            self.assertRaises(ValueError, lambda: 300 in b)
            self.assertRaises(ValueError, lambda: -1 in b)
            self.assertRaises(TypeError, lambda: None in b)
            self.assertRaises(TypeError, lambda: float(ord('a')) in b)
            self.assertRaises(TypeError, lambda: "a" in b)
            for f in bytes, bytearray:
                self.failUnless(f(b"") in b)
                self.failUnless(f(b"a") in b)
                self.failUnless(f(b"b") in b)
                self.failUnless(f(b"c") in b)
                self.failUnless(f(b"ab") in b)
                self.failUnless(f(b"bc") in b)
                self.failUnless(f(b"abc") in b)
                self.failIf(f(b"ac") in b)
                self.failIf(f(b"d") in b)
                self.failIf(f(b"dab") in b)
                self.failIf(f(b"abd") in b)

    def test_alloc(self):
        b = bytearray()
        alloc = b.__alloc__()
        self.assert_(alloc >= 0)
        seq = [alloc]
        for i in range(100):
            b += b"x"
            alloc = b.__alloc__()
            self.assert_(alloc >= len(b))
            if alloc not in seq:
                seq.append(alloc)

    def test_fromhex(self):
        self.assertRaises(TypeError, bytearray.fromhex)
        self.assertRaises(TypeError, bytearray.fromhex, 1)
        self.assertEquals(bytearray.fromhex(''), bytearray())
        b = bytearray([0x1a, 0x2b, 0x30])
        self.assertEquals(bytearray.fromhex('1a2B30'), b)
        self.assertEquals(bytearray.fromhex('  1A 2B  30   '), b)
        self.assertEquals(bytearray.fromhex('0000'), b'\0\0')
        self.assertRaises(TypeError, bytearray.fromhex, b'1B')
        self.assertRaises(ValueError, bytearray.fromhex, 'a')
        self.assertRaises(ValueError, bytearray.fromhex, 'rt')
        self.assertRaises(ValueError, bytearray.fromhex, '1a b cd')
        self.assertRaises(ValueError, bytearray.fromhex, '\x00')
        self.assertRaises(ValueError, bytearray.fromhex, '12   \x00   34')

    def test_join(self):
        self.assertEqual(b"".join([]), b"")
        self.assertEqual(b"".join([b""]), b"")
        for lst in [[b"abc"], [b"a", b"bc"], [b"ab", b"c"], [b"a", b"b", b"c"]]:
            self.assertEqual(b"".join(lst), b"abc")
            self.assertEqual(b"".join(tuple(lst)), b"abc")
            self.assertEqual(b"".join(iter(lst)), b"abc")
        self.assertEqual(b".".join([b"ab", b"cd"]), b"ab.cd")
        # XXX more...

    def test_literal(self):
        tests =  [
            (b"Wonderful spam", "Wonderful spam"),
            (br"Wonderful spam too", "Wonderful spam too"),
            (b"\xaa\x00\000\200", "\xaa\x00\000\200"),
            (br"\xaa\x00\000\200", r"\xaa\x00\000\200"),
        ]
        for b, s in tests:
            self.assertEqual(b, bytearray(s, 'latin-1'))
        for c in range(128, 256):
            self.assertRaises(SyntaxError, eval,
                              'b"%s"' % chr(c))

    def test_extend(self):
        orig = b'hello'
        a = bytearray(orig)
        a.extend(a)
        self.assertEqual(a, orig + orig)
        self.assertEqual(a[5:], orig)

    def test_remove(self):
        b = bytearray(b'hello')
        b.remove(ord('l'))
        self.assertEqual(b, b'helo')
        b.remove(ord('l'))
        self.assertEqual(b, b'heo')
        self.assertRaises(ValueError, lambda: b.remove(ord('l')))
        self.assertRaises(ValueError, lambda: b.remove(400))
        self.assertRaises(TypeError, lambda: b.remove('e'))
        # remove first and last
        b.remove(ord('o'))
        b.remove(ord('h'))
        self.assertEqual(b, b'e')
        self.assertRaises(TypeError, lambda: b.remove(b'e'))

    def test_pop(self):
        b = bytearray(b'world')
        self.assertEqual(b.pop(), ord('d'))
        self.assertEqual(b.pop(0), ord('w'))
        self.assertEqual(b.pop(-2), ord('r'))
        self.assertRaises(IndexError, lambda: b.pop(10))
        self.assertRaises(OverflowError, lambda: bytearray().pop())

    def test_nosort(self):
        self.assertRaises(AttributeError, lambda: bytearray().sort())

    def test_index(self):
        b = b'parrot'
        self.assertEqual(b.index('p'), 0)
        self.assertEqual(b.index('rr'), 2)
        self.assertEqual(b.index('t'), 5)
        self.assertRaises(ValueError, lambda: b.index('w'))

    def test_count(self):
        b = b'mississippi'
        self.assertEqual(b.count(b'i'), 4)
        self.assertEqual(b.count(b'ss'), 2)
        self.assertEqual(b.count(b'w'), 0)

    def test_append(self):
        b = bytearray(b'hell')
        b.append(ord('o'))
        self.assertEqual(b, b'hello')
        self.assertEqual(b.append(100), None)
        b = bytearray()
        b.append(ord('A'))
        self.assertEqual(len(b), 1)
        self.assertRaises(TypeError, lambda: b.append(b'o'))

    def test_insert(self):
        b = bytearray(b'msssspp')
        b.insert(1, ord('i'))
        b.insert(4, ord('i'))
        b.insert(-2, ord('i'))
        b.insert(1000, ord('i'))
        self.assertEqual(b, b'mississippi')
        self.assertRaises(TypeError, lambda: b.insert(0, b'1'))

    def test_startswith(self):
        b = b'hello'
        self.assertFalse(bytearray().startswith(b"anything"))
        self.assertTrue(b.startswith(b"hello"))
        self.assertTrue(b.startswith(b"hel"))
        self.assertTrue(b.startswith(b"h"))
        self.assertFalse(b.startswith(b"hellow"))
        self.assertFalse(b.startswith(b"ha"))

    def test_endswith(self):
        b = b'hello'
        self.assertFalse(bytearray().endswith(b"anything"))
        self.assertTrue(b.endswith(b"hello"))
        self.assertTrue(b.endswith(b"llo"))
        self.assertTrue(b.endswith(b"o"))
        self.assertFalse(b.endswith(b"whello"))
        self.assertFalse(b.endswith(b"no"))

    def test_find(self):
        b = b'mississippi'
        self.assertEqual(b.find(b'ss'), 2)
        self.assertEqual(b.find(b'ss', 3), 5)
        self.assertEqual(b.find(b'ss', 1, 7), 2)
        self.assertEqual(b.find(b'ss', 1, 3), -1)
        self.assertEqual(b.find(b'w'), -1)
        self.assertEqual(b.find(b'mississippian'), -1)

    def test_rfind(self):
        b = b'mississippi'
        self.assertEqual(b.rfind(b'ss'), 5)
        self.assertEqual(b.rfind(b'ss', 3), 5)
        self.assertEqual(b.rfind(b'ss', 0, 6), 2)
        self.assertEqual(b.rfind(b'w'), -1)
        self.assertEqual(b.rfind(b'mississippian'), -1)

    def test_index(self):
        b = b'world'
        self.assertEqual(b.index(b'w'), 0)
        self.assertEqual(b.index(b'orl'), 1)
        self.assertRaises(ValueError, b.index, b'worm')
        self.assertRaises(ValueError, b.index, b'ldo')

    def test_rindex(self):
        # XXX could be more rigorous
        b = b'world'
        self.assertEqual(b.rindex(b'w'), 0)
        self.assertEqual(b.rindex(b'orl'), 1)
        self.assertRaises(ValueError, b.rindex, b'worm')
        self.assertRaises(ValueError, b.rindex, b'ldo')

    def test_replace(self):
        b = b'mississippi'
        self.assertEqual(b.replace(b'i', b'a'), b'massassappa')
        self.assertEqual(b.replace(b'ss', b'x'), b'mixixippi')

    def test_translate(self):
        b = b'hello'
        rosetta = bytearray(range(0, 256))
        rosetta[ord('o')] = ord('e')
        c = b.translate(rosetta, b'l')
        self.assertEqual(b, b'hello')
        self.assertEqual(c, b'hee')

    def test_split(self):
        b = b'mississippi'
        self.assertEqual(b.split(b'i'), [b'm', b'ss', b'ss', b'pp', b''])
        self.assertEqual(b.split(b'ss'), [b'mi', b'i', b'ippi'])
        self.assertEqual(b.split(b'w'), [b])

    def test_split_whitespace(self):
        for b in (b'  arf  barf  ', b'arf\tbarf', b'arf\nbarf', b'arf\rbarf',
                  b'arf\fbarf', b'arf\vbarf'):
            self.assertEqual(b.split(), [b'arf', b'barf'])
            self.assertEqual(b.split(None), [b'arf', b'barf'])
            self.assertEqual(b.split(None, 2), [b'arf', b'barf'])
        self.assertEqual(b'  a  bb  c  '.split(None, 0), [b'a  bb  c  '])
        self.assertEqual(b'  a  bb  c  '.split(None, 1), [b'a', b'bb  c  '])
        self.assertEqual(b'  a  bb  c  '.split(None, 2), [b'a', b'bb', b'c  '])
        self.assertEqual(b'  a  bb  c  '.split(None, 3), [b'a', b'bb', b'c'])

    def test_split_bytearray(self):
        self.assertEqual(b'a b'.split(memoryview(b' ')), [b'a', b'b'])

    def test_split_string_error(self):
        self.assertRaises(TypeError, b'a b'.split, ' ')

    def test_rsplit(self):
        b = b'mississippi'
        self.assertEqual(b.rsplit(b'i'), [b'm', b'ss', b'ss', b'pp', b''])
        self.assertEqual(b.rsplit(b'ss'), [b'mi', b'i', b'ippi'])
        self.assertEqual(b.rsplit(b'w'), [b])

    def test_rsplit_whitespace(self):
        for b in (b'  arf  barf  ', b'arf\tbarf', b'arf\nbarf', b'arf\rbarf',
                  b'arf\fbarf', b'arf\vbarf'):
            self.assertEqual(b.rsplit(), [b'arf', b'barf'])
            self.assertEqual(b.rsplit(None), [b'arf', b'barf'])
            self.assertEqual(b.rsplit(None, 2), [b'arf', b'barf'])
        self.assertEqual(b'  a  bb  c  '.rsplit(None, 0), [b'  a  bb  c'])
        self.assertEqual(b'  a  bb  c  '.rsplit(None, 1), [b'  a  bb', b'c'])
        self.assertEqual(b'  a  bb  c  '.rsplit(None,2), [b'  a', b'bb', b'c'])
        self.assertEqual(b'  a  bb  c  '.rsplit(None, 3), [b'a', b'bb', b'c'])

    def test_rsplit_bytearray(self):
        self.assertEqual(b'a b'.rsplit(memoryview(b' ')), [b'a', b'b'])

    def test_rsplit_string_error(self):
        self.assertRaises(TypeError, b'a b'.rsplit, ' ')

    def test_partition(self):
        b = b'mississippi'
        self.assertEqual(b.partition(b'ss'), (b'mi', b'ss', b'issippi'))
        self.assertEqual(b.rpartition(b'w'), (b'', b'', b'mississippi'))

    def test_rpartition(self):
        b = b'mississippi'
        self.assertEqual(b.rpartition(b'ss'), (b'missi', b'ss', b'ippi'))
        self.assertEqual(b.rpartition(b'i'), (b'mississipp', b'i', b''))

    def test_pickling(self):
        for proto in range(pickle.HIGHEST_PROTOCOL):
            for b in b"", b"a", b"abc", b"\xffab\x80", b"\0\0\377\0\0":
                ps = pickle.dumps(b, proto)
                q = pickle.loads(ps)
                self.assertEqual(b, q)

    def test_strip(self):
        b = b'mississippi'
        self.assertEqual(b.strip(b'i'), b'mississipp')
        self.assertEqual(b.strip(b'm'), b'ississippi')
        self.assertEqual(b.strip(b'pi'), b'mississ')
        self.assertEqual(b.strip(b'im'), b'ssissipp')
        self.assertEqual(b.strip(b'pim'), b'ssiss')
        self.assertEqual(b.strip(b), b'')

    def test_lstrip(self):
        b = b'mississippi'
        self.assertEqual(b.lstrip(b'i'), b'mississippi')
        self.assertEqual(b.lstrip(b'm'), b'ississippi')
        self.assertEqual(b.lstrip(b'pi'), b'mississippi')
        self.assertEqual(b.lstrip(b'im'), b'ssissippi')
        self.assertEqual(b.lstrip(b'pim'), b'ssissippi')

    def test_rstrip(self):
        b = b'mississippi'
        self.assertEqual(b.rstrip(b'i'), b'mississipp')
        self.assertEqual(b.rstrip(b'm'), b'mississippi')
        self.assertEqual(b.rstrip(b'pi'), b'mississ')
        self.assertEqual(b.rstrip(b'im'), b'mississipp')
        self.assertEqual(b.rstrip(b'pim'), b'mississ')

    def test_strip_whitespace(self):
        b = b' \t\n\r\f\vabc \t\n\r\f\v'
        self.assertEqual(b.strip(), b'abc')
        self.assertEqual(b.lstrip(), b'abc \t\n\r\f\v')
        self.assertEqual(b.rstrip(), b' \t\n\r\f\vabc')

    def test_strip_bytearray(self):
        self.assertEqual(b'abc'.strip(memoryview(b'ac')), b'b')
        self.assertEqual(b'abc'.lstrip(memoryview(b'ac')), b'bc')
        self.assertEqual(b'abc'.rstrip(memoryview(b'ac')), b'ab')

    def test_strip_string_error(self):
        self.assertRaises(TypeError, b'abc'.strip, 'b')
        self.assertRaises(TypeError, b'abc'.lstrip, 'b')
        self.assertRaises(TypeError, b'abc'.rstrip, 'b')

    def test_ord(self):
        b = b'\0A\x7f\x80\xff'
        self.assertEqual([ord(b[i:i+1]) for i in range(len(b))],
                         [0, 65, 127, 128, 255])

    def test_partition_bytearray_doesnt_share_nullstring(self):
        a, b, c = bytearray(b"x").partition(b"y")
        self.assertEqual(b, b"")
        self.assertEqual(c, b"")
        self.assert_(b is not c)
        b += b"!"
        self.assertEqual(c, b"")
        a, b, c = bytearray(b"x").partition(b"y")
        self.assertEqual(b, b"")
        self.assertEqual(c, b"")
        # Same for rpartition
        b, c, a = bytearray(b"x").rpartition(b"y")
        self.assertEqual(b, b"")
        self.assertEqual(c, b"")
        self.assert_(b is not c)
        b += b"!"
        self.assertEqual(c, b"")
        c, b, a = bytearray(b"x").rpartition(b"y")
        self.assertEqual(b, b"")
        self.assertEqual(c, b"")


    # Optimizations:
    # __iter__? (optimization)
    # __reversed__? (optimization)

    # XXX More string methods?  (Those that don't use character properties)

    # There are tests in string_tests.py that are more
    # comprehensive for things like split, partition, etc.
    # Unfortunately they are all bundled with tests that
    # are not appropriate for bytes

    # I've started porting some of those into bytearray_tests.py, we should port
    # the rest that make sense (the code can be cleaned up to use modern
    # unittest methods at the same time).

class BytearrayPEP3137Test(unittest.TestCase,
                       test.buffer_tests.MixinBytesBufferCommonTests):
    def marshal(self, x):
        return bytearray(x)

    def test_returns_new_copy(self):
        val = self.marshal(b'1234')
        # On immutable types these MAY return a reference to themselves
        # but on mutable types like bytearray they MUST return a new copy.
        for methname in ('zfill', 'rjust', 'ljust', 'center'):
            method = getattr(val, methname)
            newval = method(3)
            self.assertEqual(val, newval)
            self.assertTrue(val is not newval,
                            methname+' returned self on a mutable object')


class BytesAsStringTest(test.string_tests.BaseTest):
    type2test = bytearray

    def fixtype(self, obj):
        if isinstance(obj, str):
            return obj.encode("utf-8")
        return super().fixtype(obj)

    # Currently the bytes containment testing uses a single integer
    # value. This may not be the final design, but until then the
    # bytes section with in a bytes containment not valid
    def test_contains(self):
        pass
    def test_expandtabs(self):
        pass
    def test_upper(self):
        pass
    def test_lower(self):
        pass


class ByteArraySubclass(bytearray):
    pass

class ByteArraySubclassTest(unittest.TestCase):

    def test_basic(self):
        self.assert_(issubclass(ByteArraySubclass, bytearray))
        self.assert_(isinstance(ByteArraySubclass(), bytearray))

        a, b = b"abcd", b"efgh"
        _a, _b = ByteArraySubclass(a), ByteArraySubclass(b)

        # test comparison operators with subclass instances
        self.assert_(_a == _a)
        self.assert_(_a != _b)
        self.assert_(_a < _b)
        self.assert_(_a <= _b)
        self.assert_(_b >= _a)
        self.assert_(_b > _a)
        self.assert_(_a is not a)

        # test concat of subclass instances
        self.assertEqual(a + b, _a + _b)
        self.assertEqual(a + b, a + _b)
        self.assertEqual(a + b, _a + b)

        # test repeat
        self.assert_(a*5 == _a*5)

    def test_join(self):
        # Make sure join returns a NEW object for single item sequences
        # involving a subclass.
        # Make sure that it is of the appropriate type.
        s1 = ByteArraySubclass(b"abcd")
        s2 = bytearray().join([s1])
        self.assert_(s1 is not s2)
        self.assert_(type(s2) is bytearray, type(s2))

        # Test reverse, calling join on subclass
        s3 = s1.join([b"abcd"])
        self.assert_(type(s3) is bytearray)

    def test_pickle(self):
        a = ByteArraySubclass(b"abcd")
        a.x = 10
        a.y = ByteArraySubclass(b"efgh")
        for proto in range(pickle.HIGHEST_PROTOCOL):
            b = pickle.loads(pickle.dumps(a, proto))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)
            self.assertEqual(a.x, b.x)
            self.assertEqual(a.y, b.y)
            self.assertEqual(type(a), type(b))
            self.assertEqual(type(a.y), type(b.y))

    def test_copy(self):
        a = ByteArraySubclass(b"abcd")
        a.x = 10
        a.y = ByteArraySubclass(b"efgh")
        for copy_method in (copy.copy, copy.deepcopy):
            b = copy_method(a)
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)
            self.assertEqual(a.x, b.x)
            self.assertEqual(a.y, b.y)
            self.assertEqual(type(a), type(b))
            self.assertEqual(type(a.y), type(b.y))

    def test_init_override(self):
        class subclass(bytearray):
            def __init__(self, newarg=1, *args, **kwargs):
                bytearray.__init__(self, *args, **kwargs)
        x = subclass(4, source=b"abcd")
        self.assertEqual(x, b"abcd")
        x = subclass(newarg=4, source=b"abcd")
        self.assertEqual(x, b"abcd")


def test_main():
    test.test_support.run_unittest(BytesTest)
    test.test_support.run_unittest(BytesAsStringTest)
    test.test_support.run_unittest(ByteArraySubclassTest)
    test.test_support.run_unittest(BytearrayPEP3137Test)

if __name__ == "__main__":
    test_main()
