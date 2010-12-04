#! /usr/bin/env python3
"""Test the arraymodule.
   Roger E. Masse
"""

import unittest
from test import support
import weakref
import pickle
import operator
import io
import math
import struct
import warnings

import array
from array import _array_reconstructor as array_reconstructor


class ArraySubclass(array.array):
    pass

class ArraySubclassWithKwargs(array.array):
    def __init__(self, typecode, newarg=None):
        array.array.__init__(typecode)

tests = [] # list to accumulate all tests
typecodes = "ubBhHiIlLfd"

class BadConstructorTest(unittest.TestCase):

    def test_constructor(self):
        self.assertRaises(TypeError, array.array)
        self.assertRaises(TypeError, array.array, spam=42)
        self.assertRaises(TypeError, array.array, 'xx')
        self.assertRaises(ValueError, array.array, 'x')

tests.append(BadConstructorTest)

# Machine format codes.
#
# Search for "enum machine_format_code" in Modules/arraymodule.c to get the
# authoritative values.
UNKNOWN_FORMAT = -1
UNSIGNED_INT8 = 0
SIGNED_INT8 = 1
UNSIGNED_INT16_LE = 2
UNSIGNED_INT16_BE = 3
SIGNED_INT16_LE = 4
SIGNED_INT16_BE = 5
UNSIGNED_INT32_LE = 6
UNSIGNED_INT32_BE = 7
SIGNED_INT32_LE = 8
SIGNED_INT32_BE = 9
UNSIGNED_INT64_LE = 10
UNSIGNED_INT64_BE = 11
SIGNED_INT64_LE = 12
SIGNED_INT64_BE = 13
IEEE_754_FLOAT_LE = 14
IEEE_754_FLOAT_BE = 15
IEEE_754_DOUBLE_LE = 16
IEEE_754_DOUBLE_BE = 17
UTF16_LE = 18
UTF16_BE = 19
UTF32_LE = 20
UTF32_BE = 21

class ArrayReconstructorTest(unittest.TestCase):

    def test_error(self):
        self.assertRaises(TypeError, array_reconstructor,
                          "", "b", 0, b"")
        self.assertRaises(TypeError, array_reconstructor,
                          str, "b", 0, b"")
        self.assertRaises(TypeError, array_reconstructor,
                          array.array, "b", '', b"")
        self.assertRaises(TypeError, array_reconstructor,
                          array.array, "b", 0, "")
        self.assertRaises(ValueError, array_reconstructor,
                          array.array, "?", 0, b"")
        self.assertRaises(ValueError, array_reconstructor,
                          array.array, "b", UNKNOWN_FORMAT, b"")
        self.assertRaises(ValueError, array_reconstructor,
                          array.array, "b", 22, b"")
        self.assertRaises(ValueError, array_reconstructor,
                          array.array, "d", 16, b"a")

    def test_numbers(self):
        testcases = (
            (['B', 'H', 'I', 'L'], UNSIGNED_INT8, '=BBBB',
             [0x80, 0x7f, 0, 0xff]),
            (['b', 'h', 'i', 'l'], SIGNED_INT8, '=bbb',
             [-0x80, 0x7f, 0]),
            (['H', 'I', 'L'], UNSIGNED_INT16_LE, '<HHHH',
             [0x8000, 0x7fff, 0, 0xffff]),
            (['H', 'I', 'L'], UNSIGNED_INT16_BE, '>HHHH',
             [0x8000, 0x7fff, 0, 0xffff]),
            (['h', 'i', 'l'], SIGNED_INT16_LE, '<hhh',
             [-0x8000, 0x7fff, 0]),
            (['h', 'i', 'l'], SIGNED_INT16_BE, '>hhh',
             [-0x8000, 0x7fff, 0]),
            (['I', 'L'], UNSIGNED_INT32_LE, '<IIII',
             [1<<31, (1<<31)-1, 0, (1<<32)-1]),
            (['I', 'L'], UNSIGNED_INT32_BE, '>IIII',
             [1<<31, (1<<31)-1, 0, (1<<32)-1]),
            (['i', 'l'], SIGNED_INT32_LE, '<iii',
             [-1<<31, (1<<31)-1, 0]),
            (['i', 'l'], SIGNED_INT32_BE, '>iii',
             [-1<<31, (1<<31)-1, 0]),
            (['L'], UNSIGNED_INT64_LE, '<QQQQ',
             [1<<31, (1<<31)-1, 0, (1<<32)-1]),
            (['L'], UNSIGNED_INT64_BE, '>QQQQ',
             [1<<31, (1<<31)-1, 0, (1<<32)-1]),
            (['l'], SIGNED_INT64_LE, '<qqq',
             [-1<<31, (1<<31)-1, 0]),
            (['l'], SIGNED_INT64_BE, '>qqq',
             [-1<<31, (1<<31)-1, 0]),
            # The following tests for INT64 will raise an OverflowError
            # when run on a 32-bit machine. The tests are simply skipped
            # in that case.
            (['L'], UNSIGNED_INT64_LE, '<QQQQ',
             [1<<63, (1<<63)-1, 0, (1<<64)-1]),
            (['L'], UNSIGNED_INT64_BE, '>QQQQ',
             [1<<63, (1<<63)-1, 0, (1<<64)-1]),
            (['l'], SIGNED_INT64_LE, '<qqq',
             [-1<<63, (1<<63)-1, 0]),
            (['l'], SIGNED_INT64_BE, '>qqq',
             [-1<<63, (1<<63)-1, 0]),
            (['f'], IEEE_754_FLOAT_LE, '<ffff',
             [16711938.0, float('inf'), float('-inf'), -0.0]),
            (['f'], IEEE_754_FLOAT_BE, '>ffff',
             [16711938.0, float('inf'), float('-inf'), -0.0]),
            (['d'], IEEE_754_DOUBLE_LE, '<dddd',
             [9006104071832581.0, float('inf'), float('-inf'), -0.0]),
            (['d'], IEEE_754_DOUBLE_BE, '>dddd',
             [9006104071832581.0, float('inf'), float('-inf'), -0.0])
        )
        for testcase in testcases:
            valid_typecodes, mformat_code, struct_fmt, values = testcase
            arraystr = struct.pack(struct_fmt, *values)
            for typecode in valid_typecodes:
                try:
                    a = array.array(typecode, values)
                except OverflowError:
                    continue  # Skip this test case.
                b = array_reconstructor(
                    array.array, typecode, mformat_code, arraystr)
                self.assertEqual(a, b,
                    msg="{0!r} != {1!r}; testcase={2!r}".format(a, b, testcase))

    def test_unicode(self):
        teststr = "Bonne Journ\xe9e \U0002030a\U00020347"
        testcases = (
            (UTF16_LE, "UTF-16-LE"),
            (UTF16_BE, "UTF-16-BE"),
            (UTF32_LE, "UTF-32-LE"),
            (UTF32_BE, "UTF-32-BE")
        )
        for testcase in testcases:
            mformat_code, encoding = testcase
            a = array.array('u', teststr)
            b = array_reconstructor(
                array.array, 'u', mformat_code, teststr.encode(encoding))
            self.assertEqual(a, b,
                msg="{0!r} != {1!r}; testcase={2!r}".format(a, b, testcase))


tests.append(ArrayReconstructorTest)


class BaseTest(unittest.TestCase):
    # Required class attributes (provided by subclasses
    # typecode: the typecode to test
    # example: an initializer usable in the constructor for this type
    # smallerexample: the same length as example, but smaller
    # biggerexample: the same length as example, but bigger
    # outside: An entry that is not in example
    # minitemsize: the minimum guaranteed itemsize

    def assertEntryEqual(self, entry1, entry2):
        self.assertEqual(entry1, entry2)

    def badtypecode(self):
        # Return a typecode that is different from our own
        return typecodes[(typecodes.index(self.typecode)+1) % len(typecodes)]

    def test_constructor(self):
        a = array.array(self.typecode)
        self.assertEqual(a.typecode, self.typecode)
        self.assertTrue(a.itemsize>=self.minitemsize)
        self.assertRaises(TypeError, array.array, self.typecode, None)

    def test_len(self):
        a = array.array(self.typecode)
        a.append(self.example[0])
        self.assertEqual(len(a), 1)

        a = array.array(self.typecode, self.example)
        self.assertEqual(len(a), len(self.example))

    def test_buffer_info(self):
        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.buffer_info, 42)
        bi = a.buffer_info()
        self.assertIsInstance(bi, tuple)
        self.assertEqual(len(bi), 2)
        self.assertIsInstance(bi[0], int)
        self.assertIsInstance(bi[1], int)
        self.assertEqual(bi[1], len(a))

    def test_byteswap(self):
        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.byteswap, 42)
        if a.itemsize in (1, 2, 4, 8):
            b = array.array(self.typecode, self.example)
            b.byteswap()
            if a.itemsize==1:
                self.assertEqual(a, b)
            else:
                self.assertNotEqual(a, b)
            b.byteswap()
            self.assertEqual(a, b)

    def test_copy(self):
        import copy
        a = array.array(self.typecode, self.example)
        b = copy.copy(a)
        self.assertNotEqual(id(a), id(b))
        self.assertEqual(a, b)

    def test_deepcopy(self):
        import copy
        a = array.array(self.typecode, self.example)
        b = copy.deepcopy(a)
        self.assertNotEqual(id(a), id(b))
        self.assertEqual(a, b)

    def test_reduce_ex(self):
        a = array.array(self.typecode, self.example)
        for protocol in range(3):
            self.assertIs(a.__reduce_ex__(protocol)[0], array.array)
        for protocol in range(3, pickle.HIGHEST_PROTOCOL):
            self.assertIs(a.__reduce_ex__(protocol)[0], array_reconstructor)

    def test_pickle(self):
        for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
            a = array.array(self.typecode, self.example)
            b = pickle.loads(pickle.dumps(a, protocol))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)

            a = ArraySubclass(self.typecode, self.example)
            a.x = 10
            b = pickle.loads(pickle.dumps(a, protocol))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)
            self.assertEqual(a.x, b.x)
            self.assertEqual(type(a), type(b))

    def test_pickle_for_empty_array(self):
        for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
            a = array.array(self.typecode)
            b = pickle.loads(pickle.dumps(a, protocol))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)

            a = ArraySubclass(self.typecode)
            a.x = 10
            b = pickle.loads(pickle.dumps(a, protocol))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)
            self.assertEqual(a.x, b.x)
            self.assertEqual(type(a), type(b))

    def test_insert(self):
        a = array.array(self.typecode, self.example)
        a.insert(0, self.example[0])
        self.assertEqual(len(a), 1+len(self.example))
        self.assertEqual(a[0], a[1])
        self.assertRaises(TypeError, a.insert)
        self.assertRaises(TypeError, a.insert, None)
        self.assertRaises(TypeError, a.insert, 0, None)

        a = array.array(self.typecode, self.example)
        a.insert(-1, self.example[0])
        self.assertEqual(
            a,
            array.array(
                self.typecode,
                self.example[:-1] + self.example[:1] + self.example[-1:]
            )
        )

        a = array.array(self.typecode, self.example)
        a.insert(-1000, self.example[0])
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[:1] + self.example)
        )

        a = array.array(self.typecode, self.example)
        a.insert(1000, self.example[0])
        self.assertEqual(
            a,
            array.array(self.typecode, self.example + self.example[:1])
        )

    def test_tofromfile(self):
        a = array.array(self.typecode, 2*self.example)
        self.assertRaises(TypeError, a.tofile)
        support.unlink(support.TESTFN)
        f = open(support.TESTFN, 'wb')
        try:
            a.tofile(f)
            f.close()
            b = array.array(self.typecode)
            f = open(support.TESTFN, 'rb')
            self.assertRaises(TypeError, b.fromfile)
            b.fromfile(f, len(self.example))
            self.assertEqual(b, array.array(self.typecode, self.example))
            self.assertNotEqual(a, b)
            self.assertRaises(EOFError, b.fromfile, f, len(self.example)+1)
            self.assertEqual(a, b)
            f.close()
        finally:
            if not f.closed:
                f.close()
            support.unlink(support.TESTFN)

    def test_fromfile_ioerror(self):
        # Issue #5395: Check if fromfile raises a proper IOError
        # instead of EOFError.
        a = array.array(self.typecode)
        f = open(support.TESTFN, 'wb')
        try:
            self.assertRaises(IOError, a.fromfile, f, len(self.example))
        finally:
            f.close()
            support.unlink(support.TESTFN)

    def test_filewrite(self):
        a = array.array(self.typecode, 2*self.example)
        f = open(support.TESTFN, 'wb')
        try:
            f.write(a)
            f.close()
            b = array.array(self.typecode)
            f = open(support.TESTFN, 'rb')
            b.fromfile(f, len(self.example))
            self.assertEqual(b, array.array(self.typecode, self.example))
            self.assertNotEqual(a, b)
            b.fromfile(f, len(self.example))
            self.assertEqual(a, b)
            f.close()
        finally:
            if not f.closed:
                f.close()
            support.unlink(support.TESTFN)

    def test_tofromlist(self):
        a = array.array(self.typecode, 2*self.example)
        b = array.array(self.typecode)
        self.assertRaises(TypeError, a.tolist, 42)
        self.assertRaises(TypeError, b.fromlist)
        self.assertRaises(TypeError, b.fromlist, 42)
        self.assertRaises(TypeError, b.fromlist, [None])
        b.fromlist(a.tolist())
        self.assertEqual(a, b)

    def test_tofromstring(self):
        nb_warnings = 4
        with warnings.catch_warnings(record=True) as r:
            warnings.filterwarnings("always",
                                    message=r"(to|from)string\(\) is deprecated",
                                    category=DeprecationWarning)
            a = array.array(self.typecode, 2*self.example)
            b = array.array(self.typecode)
            self.assertRaises(TypeError, a.tostring, 42)
            self.assertRaises(TypeError, b.fromstring)
            self.assertRaises(TypeError, b.fromstring, 42)
            b.fromstring(a.tostring())
            self.assertEqual(a, b)
            if a.itemsize>1:
                self.assertRaises(ValueError, b.fromstring, "x")
                nb_warnings += 1
        self.assertEqual(len(r), nb_warnings)

    def test_tofrombytes(self):
        a = array.array(self.typecode, 2*self.example)
        b = array.array(self.typecode)
        self.assertRaises(TypeError, a.tobytes, 42)
        self.assertRaises(TypeError, b.frombytes)
        self.assertRaises(TypeError, b.frombytes, 42)
        b.frombytes(a.tobytes())
        c = array.array(self.typecode, bytearray(a.tobytes()))
        self.assertEqual(a, b)
        self.assertEqual(a, c)
        if a.itemsize>1:
            self.assertRaises(ValueError, b.frombytes, b"x")

    def test_repr(self):
        a = array.array(self.typecode, 2*self.example)
        self.assertEqual(a, eval(repr(a), {"array": array.array}))

        a = array.array(self.typecode)
        self.assertEqual(repr(a), "array('%s')" % self.typecode)

    def test_str(self):
        a = array.array(self.typecode, 2*self.example)
        str(a)

    def test_cmp(self):
        a = array.array(self.typecode, self.example)
        self.assertTrue((a == 42) is False)
        self.assertTrue((a != 42) is True)

        self.assertTrue((a == a) is True)
        self.assertTrue((a != a) is False)
        self.assertTrue((a < a) is False)
        self.assertTrue((a <= a) is True)
        self.assertTrue((a > a) is False)
        self.assertTrue((a >= a) is True)

        al = array.array(self.typecode, self.smallerexample)
        ab = array.array(self.typecode, self.biggerexample)

        self.assertTrue((a == 2*a) is False)
        self.assertTrue((a != 2*a) is True)
        self.assertTrue((a < 2*a) is True)
        self.assertTrue((a <= 2*a) is True)
        self.assertTrue((a > 2*a) is False)
        self.assertTrue((a >= 2*a) is False)

        self.assertTrue((a == al) is False)
        self.assertTrue((a != al) is True)
        self.assertTrue((a < al) is False)
        self.assertTrue((a <= al) is False)
        self.assertTrue((a > al) is True)
        self.assertTrue((a >= al) is True)

        self.assertTrue((a == ab) is False)
        self.assertTrue((a != ab) is True)
        self.assertTrue((a < ab) is True)
        self.assertTrue((a <= ab) is True)
        self.assertTrue((a > ab) is False)
        self.assertTrue((a >= ab) is False)

    def test_add(self):
        a = array.array(self.typecode, self.example) \
            + array.array(self.typecode, self.example[::-1])
        self.assertEqual(
            a,
            array.array(self.typecode, self.example + self.example[::-1])
        )

        b = array.array(self.badtypecode())
        self.assertRaises(TypeError, a.__add__, b)

        self.assertRaises(TypeError, a.__add__, "bad")

    def test_iadd(self):
        a = array.array(self.typecode, self.example[::-1])
        b = a
        a += array.array(self.typecode, 2*self.example)
        self.assertTrue(a is b)
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[::-1]+2*self.example)
        )
        a = array.array(self.typecode, self.example)
        a += a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example + self.example)
        )

        b = array.array(self.badtypecode())
        self.assertRaises(TypeError, a.__add__, b)

        self.assertRaises(TypeError, a.__iadd__, "bad")

    def test_mul(self):
        a = 5*array.array(self.typecode, self.example)
        self.assertEqual(
            a,
            array.array(self.typecode, 5*self.example)
        )

        a = array.array(self.typecode, self.example)*5
        self.assertEqual(
            a,
            array.array(self.typecode, self.example*5)
        )

        a = 0*array.array(self.typecode, self.example)
        self.assertEqual(
            a,
            array.array(self.typecode)
        )

        a = (-1)*array.array(self.typecode, self.example)
        self.assertEqual(
            a,
            array.array(self.typecode)
        )

        a = 5 * array.array(self.typecode, self.example[:1])
        self.assertEqual(
            a,
            array.array(self.typecode, [a[0]] * 5)
        )

        self.assertRaises(TypeError, a.__mul__, "bad")

    def test_imul(self):
        a = array.array(self.typecode, self.example)
        b = a

        a *= 5
        self.assertTrue(a is b)
        self.assertEqual(
            a,
            array.array(self.typecode, 5*self.example)
        )

        a *= 0
        self.assertTrue(a is b)
        self.assertEqual(a, array.array(self.typecode))

        a *= 1000
        self.assertTrue(a is b)
        self.assertEqual(a, array.array(self.typecode))

        a *= -1
        self.assertTrue(a is b)
        self.assertEqual(a, array.array(self.typecode))

        a = array.array(self.typecode, self.example)
        a *= -1
        self.assertEqual(a, array.array(self.typecode))

        self.assertRaises(TypeError, a.__imul__, "bad")

    def test_getitem(self):
        a = array.array(self.typecode, self.example)
        self.assertEntryEqual(a[0], self.example[0])
        self.assertEntryEqual(a[0], self.example[0])
        self.assertEntryEqual(a[-1], self.example[-1])
        self.assertEntryEqual(a[-1], self.example[-1])
        self.assertEntryEqual(a[len(self.example)-1], self.example[-1])
        self.assertEntryEqual(a[-len(self.example)], self.example[0])
        self.assertRaises(TypeError, a.__getitem__)
        self.assertRaises(IndexError, a.__getitem__, len(self.example))
        self.assertRaises(IndexError, a.__getitem__, -len(self.example)-1)

    def test_setitem(self):
        a = array.array(self.typecode, self.example)
        a[0] = a[-1]
        self.assertEntryEqual(a[0], a[-1])

        a = array.array(self.typecode, self.example)
        a[0] = a[-1]
        self.assertEntryEqual(a[0], a[-1])

        a = array.array(self.typecode, self.example)
        a[-1] = a[0]
        self.assertEntryEqual(a[0], a[-1])

        a = array.array(self.typecode, self.example)
        a[-1] = a[0]
        self.assertEntryEqual(a[0], a[-1])

        a = array.array(self.typecode, self.example)
        a[len(self.example)-1] = a[0]
        self.assertEntryEqual(a[0], a[-1])

        a = array.array(self.typecode, self.example)
        a[-len(self.example)] = a[-1]
        self.assertEntryEqual(a[0], a[-1])

        self.assertRaises(TypeError, a.__setitem__)
        self.assertRaises(TypeError, a.__setitem__, None)
        self.assertRaises(TypeError, a.__setitem__, 0, None)
        self.assertRaises(
            IndexError,
            a.__setitem__,
            len(self.example), self.example[0]
        )
        self.assertRaises(
            IndexError,
            a.__setitem__,
            -len(self.example)-1, self.example[0]
        )

    def test_delitem(self):
        a = array.array(self.typecode, self.example)
        del a[0]
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[1:])
        )

        a = array.array(self.typecode, self.example)
        del a[-1]
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[:-1])
        )

        a = array.array(self.typecode, self.example)
        del a[len(self.example)-1]
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[:-1])
        )

        a = array.array(self.typecode, self.example)
        del a[-len(self.example)]
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[1:])
        )

        self.assertRaises(TypeError, a.__delitem__)
        self.assertRaises(TypeError, a.__delitem__, None)
        self.assertRaises(IndexError, a.__delitem__, len(self.example))
        self.assertRaises(IndexError, a.__delitem__, -len(self.example)-1)

    def test_getslice(self):
        a = array.array(self.typecode, self.example)
        self.assertEqual(a[:], a)

        self.assertEqual(
            a[1:],
            array.array(self.typecode, self.example[1:])
        )

        self.assertEqual(
            a[:1],
            array.array(self.typecode, self.example[:1])
        )

        self.assertEqual(
            a[:-1],
            array.array(self.typecode, self.example[:-1])
        )

        self.assertEqual(
            a[-1:],
            array.array(self.typecode, self.example[-1:])
        )

        self.assertEqual(
            a[-1:-1],
            array.array(self.typecode)
        )

        self.assertEqual(
            a[2:1],
            array.array(self.typecode)
        )

        self.assertEqual(
            a[1000:],
            array.array(self.typecode)
        )
        self.assertEqual(a[-1000:], a)
        self.assertEqual(a[:1000], a)
        self.assertEqual(
            a[:-1000],
            array.array(self.typecode)
        )
        self.assertEqual(a[-1000:1000], a)
        self.assertEqual(
            a[2000:1000],
            array.array(self.typecode)
        )

    def test_extended_getslice(self):
        # Test extended slicing by comparing with list slicing
        # (Assumes list conversion works correctly, too)
        a = array.array(self.typecode, self.example)
        indices = (0, None, 1, 3, 19, 100, -1, -2, -31, -100)
        for start in indices:
            for stop in indices:
                # Everything except the initial 0 (invalid step)
                for step in indices[1:]:
                    self.assertEqual(list(a[start:stop:step]),
                                     list(a)[start:stop:step])

    def test_setslice(self):
        a = array.array(self.typecode, self.example)
        a[:1] = a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example + self.example[1:])
        )

        a = array.array(self.typecode, self.example)
        a[:-1] = a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example + self.example[-1:])
        )

        a = array.array(self.typecode, self.example)
        a[-1:] = a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[:-1] + self.example)
        )

        a = array.array(self.typecode, self.example)
        a[1:] = a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[:1] + self.example)
        )

        a = array.array(self.typecode, self.example)
        a[1:-1] = a
        self.assertEqual(
            a,
            array.array(
                self.typecode,
                self.example[:1] + self.example + self.example[-1:]
            )
        )

        a = array.array(self.typecode, self.example)
        a[1000:] = a
        self.assertEqual(
            a,
            array.array(self.typecode, 2*self.example)
        )

        a = array.array(self.typecode, self.example)
        a[-1000:] = a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example)
        )

        a = array.array(self.typecode, self.example)
        a[:1000] = a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example)
        )

        a = array.array(self.typecode, self.example)
        a[:-1000] = a
        self.assertEqual(
            a,
            array.array(self.typecode, 2*self.example)
        )

        a = array.array(self.typecode, self.example)
        a[1:0] = a
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[:1] + self.example + self.example[1:])
        )

        a = array.array(self.typecode, self.example)
        a[2000:1000] = a
        self.assertEqual(
            a,
            array.array(self.typecode, 2*self.example)
        )

        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.__setitem__, slice(0, 0), None)
        self.assertRaises(TypeError, a.__setitem__, slice(0, 1), None)

        b = array.array(self.badtypecode())
        self.assertRaises(TypeError, a.__setitem__, slice(0, 0), b)
        self.assertRaises(TypeError, a.__setitem__, slice(0, 1), b)

    def test_extended_set_del_slice(self):
        indices = (0, None, 1, 3, 19, 100, -1, -2, -31, -100)
        for start in indices:
            for stop in indices:
                # Everything except the initial 0 (invalid step)
                for step in indices[1:]:
                    a = array.array(self.typecode, self.example)
                    L = list(a)
                    # Make sure we have a slice of exactly the right length,
                    # but with (hopefully) different data.
                    data = L[start:stop:step]
                    data.reverse()
                    L[start:stop:step] = data
                    a[start:stop:step] = array.array(self.typecode, data)
                    self.assertEqual(a, array.array(self.typecode, L))

                    del L[start:stop:step]
                    del a[start:stop:step]
                    self.assertEqual(a, array.array(self.typecode, L))

    def test_index(self):
        example = 2*self.example
        a = array.array(self.typecode, example)
        self.assertRaises(TypeError, a.index)
        for x in example:
            self.assertEqual(a.index(x), example.index(x))
        self.assertRaises(ValueError, a.index, None)
        self.assertRaises(ValueError, a.index, self.outside)

    def test_count(self):
        example = 2*self.example
        a = array.array(self.typecode, example)
        self.assertRaises(TypeError, a.count)
        for x in example:
            self.assertEqual(a.count(x), example.count(x))
        self.assertEqual(a.count(self.outside), 0)
        self.assertEqual(a.count(None), 0)

    def test_remove(self):
        for x in self.example:
            example = 2*self.example
            a = array.array(self.typecode, example)
            pos = example.index(x)
            example2 = example[:pos] + example[pos+1:]
            a.remove(x)
            self.assertEqual(a, array.array(self.typecode, example2))

        a = array.array(self.typecode, self.example)
        self.assertRaises(ValueError, a.remove, self.outside)

        self.assertRaises(ValueError, a.remove, None)

    def test_pop(self):
        a = array.array(self.typecode)
        self.assertRaises(IndexError, a.pop)

        a = array.array(self.typecode, 2*self.example)
        self.assertRaises(TypeError, a.pop, 42, 42)
        self.assertRaises(TypeError, a.pop, None)
        self.assertRaises(IndexError, a.pop, len(a))
        self.assertRaises(IndexError, a.pop, -len(a)-1)

        self.assertEntryEqual(a.pop(0), self.example[0])
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[1:]+self.example)
        )
        self.assertEntryEqual(a.pop(1), self.example[2])
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[1:2]+self.example[3:]+self.example)
        )
        self.assertEntryEqual(a.pop(0), self.example[1])
        self.assertEntryEqual(a.pop(), self.example[-1])
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[3:]+self.example[:-1])
        )

    def test_reverse(self):
        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.reverse, 42)
        a.reverse()
        self.assertEqual(
            a,
            array.array(self.typecode, self.example[::-1])
        )

    def test_extend(self):
        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.extend)
        a.extend(array.array(self.typecode, self.example[::-1]))
        self.assertEqual(
            a,
            array.array(self.typecode, self.example+self.example[::-1])
        )

        a = array.array(self.typecode, self.example)
        a.extend(a)
        self.assertEqual(
            a,
            array.array(self.typecode, self.example+self.example)
        )

        b = array.array(self.badtypecode())
        self.assertRaises(TypeError, a.extend, b)

        a = array.array(self.typecode, self.example)
        a.extend(self.example[::-1])
        self.assertEqual(
            a,
            array.array(self.typecode, self.example+self.example[::-1])
        )

    def test_constructor_with_iterable_argument(self):
        a = array.array(self.typecode, iter(self.example))
        b = array.array(self.typecode, self.example)
        self.assertEqual(a, b)

        # non-iterable argument
        self.assertRaises(TypeError, array.array, self.typecode, 10)

        # pass through errors raised in __iter__
        class A:
            def __iter__(self):
                raise UnicodeError
        self.assertRaises(UnicodeError, array.array, self.typecode, A())

        # pass through errors raised in next()
        def B():
            raise UnicodeError
            yield None
        self.assertRaises(UnicodeError, array.array, self.typecode, B())

    def test_coveritertraverse(self):
        try:
            import gc
        except ImportError:
            return
        a = array.array(self.typecode)
        l = [iter(a)]
        l.append(l)
        gc.collect()

    def test_buffer(self):
        a = array.array(self.typecode, self.example)
        m = memoryview(a)
        expected = m.tobytes()
        self.assertEqual(a.tobytes(), expected)
        self.assertEqual(a.tobytes()[0], expected[0])
        # Resizing is forbidden when there are buffer exports.
        # For issue 4509, we also check after each error that
        # the array was not modified.
        self.assertRaises(BufferError, a.append, a[0])
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, a.extend, a[0:1])
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, a.remove, a[0])
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, a.pop, 0)
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, a.fromlist, a.tolist())
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, a.frombytes, a.tobytes())
        self.assertEqual(m.tobytes(), expected)
        if self.typecode == 'u':
            self.assertRaises(BufferError, a.fromunicode, a.tounicode())
            self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, operator.imul, a, 2)
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, operator.imul, a, 0)
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, operator.setitem, a, slice(0, 0), a)
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, operator.delitem, a, 0)
        self.assertEqual(m.tobytes(), expected)
        self.assertRaises(BufferError, operator.delitem, a, slice(0, 1))
        self.assertEqual(m.tobytes(), expected)

    def test_weakref(self):
        s = array.array(self.typecode, self.example)
        p = weakref.proxy(s)
        self.assertEqual(p.tobytes(), s.tobytes())
        s = None
        self.assertRaises(ReferenceError, len, p)

    def test_bug_782369(self):
        import sys
        if hasattr(sys, "getrefcount"):
            for i in range(10):
                b = array.array('B', range(64))
            rc = sys.getrefcount(10)
            for i in range(10):
                b = array.array('B', range(64))
            self.assertEqual(rc, sys.getrefcount(10))

    def test_subclass_with_kwargs(self):
        # SF bug #1486663 -- this used to erroneously raise a TypeError
        ArraySubclassWithKwargs('b', newarg=1)

    def test_create_from_bytes(self):
        # XXX This test probably needs to be moved in a subclass or
        # generalized to use self.typecode.
        a = array.array('H', b"1234")
        self.assertEqual(len(a) * a.itemsize, 4)


class StringTest(BaseTest):

    def test_setitem(self):
        super().test_setitem()
        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.__setitem__, 0, self.example[:2])

class UnicodeTest(StringTest):
    typecode = 'u'
    example = '\x01\u263a\x00\ufeff'
    smallerexample = '\x01\u263a\x00\ufefe'
    biggerexample = '\x01\u263a\x01\ufeff'
    outside = str('\x33')
    minitemsize = 2

    def test_unicode(self):
        self.assertRaises(TypeError, array.array, 'b', 'foo')

        a = array.array('u', '\xa0\xc2\u1234')
        a.fromunicode(' ')
        a.fromunicode('')
        a.fromunicode('')
        a.fromunicode('\x11abc\xff\u1234')
        s = a.tounicode()
        self.assertEqual(s, '\xa0\xc2\u1234 \x11abc\xff\u1234')

        s = '\x00="\'a\\b\x80\xff\u0000\u0001\u1234'
        a = array.array('u', s)
        self.assertEqual(
            repr(a),
            "array('u', '\\x00=\"\\'a\\\\b\\x80\xff\\x00\\x01\u1234')")

        self.assertRaises(TypeError, a.fromunicode)

tests.append(UnicodeTest)

class NumberTest(BaseTest):

    def test_extslice(self):
        a = array.array(self.typecode, range(5))
        self.assertEqual(a[::], a)
        self.assertEqual(a[::2], array.array(self.typecode, [0,2,4]))
        self.assertEqual(a[1::2], array.array(self.typecode, [1,3]))
        self.assertEqual(a[::-1], array.array(self.typecode, [4,3,2,1,0]))
        self.assertEqual(a[::-2], array.array(self.typecode, [4,2,0]))
        self.assertEqual(a[3::-2], array.array(self.typecode, [3,1]))
        self.assertEqual(a[-100:100:], a)
        self.assertEqual(a[100:-100:-1], a[::-1])
        self.assertEqual(a[-100:100:2], array.array(self.typecode, [0,2,4]))
        self.assertEqual(a[1000:2000:2], array.array(self.typecode, []))
        self.assertEqual(a[-1000:-2000:-2], array.array(self.typecode, []))

    def test_delslice(self):
        a = array.array(self.typecode, range(5))
        del a[::2]
        self.assertEqual(a, array.array(self.typecode, [1,3]))
        a = array.array(self.typecode, range(5))
        del a[1::2]
        self.assertEqual(a, array.array(self.typecode, [0,2,4]))
        a = array.array(self.typecode, range(5))
        del a[1::-2]
        self.assertEqual(a, array.array(self.typecode, [0,2,3,4]))
        a = array.array(self.typecode, range(10))
        del a[::1000]
        self.assertEqual(a, array.array(self.typecode, [1,2,3,4,5,6,7,8,9]))
        # test issue7788
        a = array.array(self.typecode, range(10))
        del a[9::1<<333]

    def test_assignment(self):
        a = array.array(self.typecode, range(10))
        a[::2] = array.array(self.typecode, [42]*5)
        self.assertEqual(a, array.array(self.typecode, [42, 1, 42, 3, 42, 5, 42, 7, 42, 9]))
        a = array.array(self.typecode, range(10))
        a[::-4] = array.array(self.typecode, [10]*3)
        self.assertEqual(a, array.array(self.typecode, [0, 10, 2, 3, 4, 10, 6, 7, 8 ,10]))
        a = array.array(self.typecode, range(4))
        a[::-1] = a
        self.assertEqual(a, array.array(self.typecode, [3, 2, 1, 0]))
        a = array.array(self.typecode, range(10))
        b = a[:]
        c = a[:]
        ins = array.array(self.typecode, range(2))
        a[2:3] = ins
        b[slice(2,3)] = ins
        c[2:3:] = ins

    def test_iterationcontains(self):
        a = array.array(self.typecode, range(10))
        self.assertEqual(list(a), list(range(10)))
        b = array.array(self.typecode, [20])
        self.assertEqual(a[-1] in a, True)
        self.assertEqual(b[0] not in a, True)

    def check_overflow(self, lower, upper):
        # method to be used by subclasses

        # should not overflow assigning lower limit
        a = array.array(self.typecode, [lower])
        a[0] = lower
        # should overflow assigning less than lower limit
        self.assertRaises(OverflowError, array.array, self.typecode, [lower-1])
        self.assertRaises(OverflowError, a.__setitem__, 0, lower-1)
        # should not overflow assigning upper limit
        a = array.array(self.typecode, [upper])
        a[0] = upper
        # should overflow assigning more than upper limit
        self.assertRaises(OverflowError, array.array, self.typecode, [upper+1])
        self.assertRaises(OverflowError, a.__setitem__, 0, upper+1)

    def test_subclassing(self):
        typecode = self.typecode
        class ExaggeratingArray(array.array):
            __slots__ = ['offset']

            def __new__(cls, typecode, data, offset):
                return array.array.__new__(cls, typecode, data)

            def __init__(self, typecode, data, offset):
                self.offset = offset

            def __getitem__(self, i):
                return array.array.__getitem__(self, i) + self.offset

        a = ExaggeratingArray(self.typecode, [3, 6, 7, 11], 4)
        self.assertEntryEqual(a[0], 7)

        self.assertRaises(AttributeError, setattr, a, "color", "blue")

class SignedNumberTest(NumberTest):
    example = [-1, 0, 1, 42, 0x7f]
    smallerexample = [-1, 0, 1, 42, 0x7e]
    biggerexample = [-1, 0, 1, 43, 0x7f]
    outside = 23

    def test_overflow(self):
        a = array.array(self.typecode)
        lower = -1 * int(pow(2, a.itemsize * 8 - 1))
        upper = int(pow(2, a.itemsize * 8 - 1)) - 1
        self.check_overflow(lower, upper)

class UnsignedNumberTest(NumberTest):
    example = [0, 1, 17, 23, 42, 0xff]
    smallerexample = [0, 1, 17, 23, 42, 0xfe]
    biggerexample = [0, 1, 17, 23, 43, 0xff]
    outside = 0xaa

    def test_overflow(self):
        a = array.array(self.typecode)
        lower = 0
        upper = int(pow(2, a.itemsize * 8)) - 1
        self.check_overflow(lower, upper)

    def test_bytes_extend(self):
        s = bytes(self.example)

        a = array.array(self.typecode, self.example)
        a.extend(s)
        self.assertEqual(
            a,
            array.array(self.typecode, self.example+self.example)
        )

        a = array.array(self.typecode, self.example)
        a.extend(bytearray(reversed(s)))
        self.assertEqual(
            a,
            array.array(self.typecode, self.example+self.example[::-1])
        )


class ByteTest(SignedNumberTest):
    typecode = 'b'
    minitemsize = 1
tests.append(ByteTest)

class UnsignedByteTest(UnsignedNumberTest):
    typecode = 'B'
    minitemsize = 1
tests.append(UnsignedByteTest)

class ShortTest(SignedNumberTest):
    typecode = 'h'
    minitemsize = 2
tests.append(ShortTest)

class UnsignedShortTest(UnsignedNumberTest):
    typecode = 'H'
    minitemsize = 2
tests.append(UnsignedShortTest)

class IntTest(SignedNumberTest):
    typecode = 'i'
    minitemsize = 2
tests.append(IntTest)

class UnsignedIntTest(UnsignedNumberTest):
    typecode = 'I'
    minitemsize = 2
tests.append(UnsignedIntTest)

class LongTest(SignedNumberTest):
    typecode = 'l'
    minitemsize = 4
tests.append(LongTest)

class UnsignedLongTest(UnsignedNumberTest):
    typecode = 'L'
    minitemsize = 4
tests.append(UnsignedLongTest)

class FPTest(NumberTest):
    example = [-42.0, 0, 42, 1e5, -1e10]
    smallerexample = [-42.0, 0, 42, 1e5, -2e10]
    biggerexample = [-42.0, 0, 42, 1e5, 1e10]
    outside = 23

    def assertEntryEqual(self, entry1, entry2):
        self.assertAlmostEqual(entry1, entry2)

    def test_byteswap(self):
        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.byteswap, 42)
        if a.itemsize in (1, 2, 4, 8):
            b = array.array(self.typecode, self.example)
            b.byteswap()
            if a.itemsize==1:
                self.assertEqual(a, b)
            else:
                # On alphas treating the byte swapped bit patters as
                # floats/doubles results in floating point exceptions
                # => compare the 8bit string values instead
                self.assertNotEqual(a.tobytes(), b.tobytes())
            b.byteswap()
            self.assertEqual(a, b)

class FloatTest(FPTest):
    typecode = 'f'
    minitemsize = 4
tests.append(FloatTest)

class DoubleTest(FPTest):
    typecode = 'd'
    minitemsize = 8

    def test_alloc_overflow(self):
        from sys import maxsize
        a = array.array('d', [-1]*65536)
        try:
            a *= maxsize//65536 + 1
        except MemoryError:
            pass
        else:
            self.fail("Array of size > maxsize created - MemoryError expected")
        b = array.array('d', [ 2.71828183, 3.14159265, -1])
        try:
            b * (maxsize//3 + 1)
        except MemoryError:
            pass
        else:
            self.fail("Array of size > maxsize created - MemoryError expected")

tests.append(DoubleTest)

def test_main(verbose=None):
    import sys

    support.run_unittest(*tests)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            support.run_unittest(*tests)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)

if __name__ == "__main__":
    test_main(verbose=True)
