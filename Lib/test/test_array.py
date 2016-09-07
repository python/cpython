"""Test the arraymodule.
   Roger E. Masse
"""

import unittest
import warnings
from test import test_support
from weakref import proxy
import array, cStringIO
from cPickle import loads, dumps, HIGHEST_PROTOCOL
import sys

class ArraySubclass(array.array):
    pass

class ArraySubclassWithKwargs(array.array):
    def __init__(self, typecode, newarg=None):
        array.array.__init__(self, typecode)

tests = [] # list to accumulate all tests
typecodes = "cbBhHiIlLfd"
if test_support.have_unicode:
    typecodes += "u"

class MiscTest(unittest.TestCase):

    def test_bad_constructor(self):
        self.assertRaises(TypeError, array.array)
        self.assertRaises(TypeError, array.array, spam=42)
        self.assertRaises(TypeError, array.array, 'xx')
        self.assertRaises(TypeError, array.array, '')
        self.assertRaises(TypeError, array.array, 1)
        self.assertRaises(ValueError, array.array, 'x')
        self.assertRaises(ValueError, array.array, '\x80')

    @test_support.requires_unicode
    def test_unicode_constructor(self):
        self.assertRaises(TypeError, array.array, u'xx')
        self.assertRaises(TypeError, array.array, u'')
        self.assertRaises(ValueError, array.array, u'x')
        self.assertRaises(ValueError, array.array, u'\x80')

    def test_empty(self):
        # Exercise code for handling zero-length arrays
        a = array.array('B')
        a[:] = a
        self.assertEqual(len(a), 0)
        self.assertEqual(len(a + a), 0)
        self.assertEqual(len(a * 3), 0)
        a += a
        self.assertEqual(len(a), 0)

tests.append(MiscTest)

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
        self.assertGreaterEqual(a.itemsize, self.minitemsize)
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
        self.assertIsInstance(bi[0], (int, long))
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

    def test_pickle(self):
        for protocol in range(HIGHEST_PROTOCOL + 1):
            a = array.array(self.typecode, self.example)
            b = loads(dumps(a, protocol))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)

            a = ArraySubclass(self.typecode, self.example)
            a.x = 10
            b = loads(dumps(a, protocol))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)
            self.assertEqual(a.x, b.x)
            self.assertEqual(type(a), type(b))

    def test_pickle_for_empty_array(self):
        for protocol in range(HIGHEST_PROTOCOL + 1):
            a = array.array(self.typecode)
            b = loads(dumps(a, protocol))
            self.assertNotEqual(id(a), id(b))
            self.assertEqual(a, b)

            a = ArraySubclass(self.typecode)
            a.x = 10
            b = loads(dumps(a, protocol))
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
        self.assertRaises(TypeError, a.tofile, cStringIO.StringIO())
        test_support.unlink(test_support.TESTFN)
        f = open(test_support.TESTFN, 'wb')
        try:
            a.tofile(f)
            f.close()
            b = array.array(self.typecode)
            f = open(test_support.TESTFN, 'rb')
            self.assertRaises(TypeError, b.fromfile)
            self.assertRaises(
                TypeError,
                b.fromfile,
                cStringIO.StringIO(), len(self.example)
            )
            b.fromfile(f, len(self.example))
            self.assertEqual(b, array.array(self.typecode, self.example))
            self.assertNotEqual(a, b)
            b.fromfile(f, len(self.example))
            self.assertEqual(a, b)
            self.assertRaises(EOFError, b.fromfile, f, 1)
            f.close()
        finally:
            if not f.closed:
                f.close()
            test_support.unlink(test_support.TESTFN)

    def test_fromfile_ioerror(self):
        # Issue #5395: Check if fromfile raises a proper IOError
        # instead of EOFError.
        a = array.array(self.typecode)
        f = open(test_support.TESTFN, 'wb')
        try:
            self.assertRaises(IOError, a.fromfile, f, len(self.example))
        finally:
            f.close()
            test_support.unlink(test_support.TESTFN)

    def test_filewrite(self):
        a = array.array(self.typecode, 2*self.example)
        f = open(test_support.TESTFN, 'wb')
        try:
            f.write(a)
            f.close()
            b = array.array(self.typecode)
            f = open(test_support.TESTFN, 'rb')
            b.fromfile(f, len(self.example))
            self.assertEqual(b, array.array(self.typecode, self.example))
            self.assertNotEqual(a, b)
            b.fromfile(f, len(self.example))
            self.assertEqual(a, b)
            f.close()
        finally:
            if not f.closed:
                f.close()
            test_support.unlink(test_support.TESTFN)

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
        a = array.array(self.typecode, 2*self.example)
        b = array.array(self.typecode)
        self.assertRaises(TypeError, a.tostring, 42)
        self.assertRaises(TypeError, b.fromstring)
        self.assertRaises(TypeError, b.fromstring, 42)
        self.assertRaises(ValueError, a.fromstring, a)
        b.fromstring(a.tostring())
        self.assertEqual(a, b)
        if a.itemsize>1:
            self.assertRaises(ValueError, b.fromstring, "x")

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
        self.assertIs(a == 42, False)
        self.assertIs(a != 42, True)

        self.assertIs(a == a, True)
        self.assertIs(a != a, False)
        self.assertIs(a < a, False)
        self.assertIs(a <= a, True)
        self.assertIs(a > a, False)
        self.assertIs(a >= a, True)

        al = array.array(self.typecode, self.smallerexample)
        ab = array.array(self.typecode, self.biggerexample)

        self.assertIs(a == 2*a, False)
        self.assertIs(a != 2*a, True)
        self.assertIs(a < 2*a, True)
        self.assertIs(a <= 2*a, True)
        self.assertIs(a > 2*a, False)
        self.assertIs(a >= 2*a, False)

        self.assertIs(a == al, False)
        self.assertIs(a != al, True)
        self.assertIs(a < al, False)
        self.assertIs(a <= al, False)
        self.assertIs(a > al, True)
        self.assertIs(a >= al, True)

        self.assertIs(a == ab, False)
        self.assertIs(a != ab, True)
        self.assertIs(a < ab, True)
        self.assertIs(a <= ab, True)
        self.assertIs(a > ab, False)
        self.assertIs(a >= ab, False)

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
        self.assertIs(a, b)
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

        self.assertRaises(TypeError, a.__mul__, "bad")

    def test_imul(self):
        a = array.array(self.typecode, self.example)
        b = a

        a *= 5
        self.assertIs(a, b)
        self.assertEqual(
            a,
            array.array(self.typecode, 5*self.example)
        )

        a *= 0
        self.assertIs(a, b)
        self.assertEqual(a, array.array(self.typecode))

        a *= 1000
        self.assertIs(a, b)
        self.assertEqual(a, array.array(self.typecode))

        a *= -1
        self.assertIs(a, b)
        self.assertEqual(a, array.array(self.typecode))

        a = array.array(self.typecode, self.example)
        a *= -1
        self.assertEqual(a, array.array(self.typecode))

        self.assertRaises(TypeError, a.__imul__, "bad")

    def test_getitem(self):
        a = array.array(self.typecode, self.example)
        self.assertEntryEqual(a[0], self.example[0])
        self.assertEntryEqual(a[0L], self.example[0])
        self.assertEntryEqual(a[-1], self.example[-1])
        self.assertEntryEqual(a[-1L], self.example[-1])
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
        a[0L] = a[-1]
        self.assertEntryEqual(a[0], a[-1])

        a = array.array(self.typecode, self.example)
        a[-1] = a[0]
        self.assertEntryEqual(a[0], a[-1])

        a = array.array(self.typecode, self.example)
        a[-1L] = a[0]
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
        self.assertRaises(TypeError, a.__setslice__, 0, 0, None)
        self.assertRaises(TypeError, a.__setitem__, slice(0, 0), None)
        self.assertRaises(TypeError, a.__setitem__, slice(0, 1), None)

        b = array.array(self.badtypecode())
        self.assertRaises(TypeError, a.__setslice__, 0, 0, b)
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
            self.skipTest('gc module not available')
        a = array.array(self.typecode)
        l = [iter(a)]
        l.append(l)
        gc.collect()

    def test_buffer(self):
        a = array.array(self.typecode, self.example)
        with test_support.check_py3k_warnings():
            b = buffer(a)
        self.assertEqual(b[0], a.tostring()[0])

    def test_weakref(self):
        s = array.array(self.typecode, self.example)
        p = proxy(s)
        self.assertEqual(p.tostring(), s.tostring())
        s = None
        self.assertRaises(ReferenceError, len, p)

    @unittest.skipUnless(hasattr(sys, 'getrefcount'),
                         'test needs sys.getrefcount()')
    def test_bug_782369(self):
        for i in range(10):
            b = array.array('B', range(64))
        rc = sys.getrefcount(10)
        for i in range(10):
            b = array.array('B', range(64))
        self.assertEqual(rc, sys.getrefcount(10))

    def test_subclass_with_kwargs(self):
        # SF bug #1486663 -- this used to erroneously raise a TypeError
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", '', DeprecationWarning)
            ArraySubclassWithKwargs('b', newarg=1)


class StringTest(BaseTest):

    def test_setitem(self):
        super(StringTest, self).test_setitem()
        a = array.array(self.typecode, self.example)
        self.assertRaises(TypeError, a.__setitem__, 0, self.example[:2])

class CharacterTest(StringTest):
    typecode = 'c'
    example = '\x01azAZ\x00\xfe'
    smallerexample = '\x01azAY\x00\xfe'
    biggerexample = '\x01azAZ\x00\xff'
    outside = '\x33'
    minitemsize = 1

    def test_subbclassing(self):
        class EditableString(array.array):
            def __new__(cls, s, *args, **kwargs):
                return array.array.__new__(cls, 'c', s)

            def __init__(self, s, color='blue'):
                self.color = color

            def strip(self):
                self[:] = array.array('c', self.tostring().strip())

            def __repr__(self):
                return 'EditableString(%r)' % self.tostring()

        s = EditableString("\ttest\r\n")
        s.strip()
        self.assertEqual(s.tostring(), "test")

        self.assertEqual(s.color, "blue")
        s.color = "red"
        self.assertEqual(s.color, "red")
        self.assertEqual(s.__dict__.keys(), ["color"])

    @test_support.requires_unicode
    def test_nounicode(self):
        a = array.array(self.typecode, self.example)
        self.assertRaises(ValueError, a.fromunicode, unicode(''))
        self.assertRaises(ValueError, a.tounicode)

tests.append(CharacterTest)

if test_support.have_unicode:
    class UnicodeTest(StringTest):
        typecode = 'u'
        example = unicode(r'\x01\u263a\x00\ufeff', 'unicode-escape')
        smallerexample = unicode(r'\x01\u263a\x00\ufefe', 'unicode-escape')
        biggerexample = unicode(r'\x01\u263a\x01\ufeff', 'unicode-escape')
        outside = unicode('\x33')
        minitemsize = 2

        def test_unicode(self):
            self.assertRaises(TypeError, array.array, 'b', unicode('foo', 'ascii'))

            a = array.array('u', unicode(r'\xa0\xc2\u1234', 'unicode-escape'))
            a.fromunicode(unicode(' ', 'ascii'))
            a.fromunicode(unicode('', 'ascii'))
            a.fromunicode(unicode('', 'ascii'))
            a.fromunicode(unicode(r'\x11abc\xff\u1234', 'unicode-escape'))
            s = a.tounicode()
            self.assertEqual(
                s,
                unicode(r'\xa0\xc2\u1234 \x11abc\xff\u1234', 'unicode-escape')
            )

            s = unicode(r'\x00="\'a\\b\x80\xff\u0000\u0001\u1234', 'unicode-escape')
            a = array.array('u', s)
            self.assertEqual(
                repr(a),
                r"""array('u', u'\x00="\'a\\b\x80\xff\x00\x01\u1234')"""
            )

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
        self.assertEqual(a[-100L:100L:2L], array.array(self.typecode, [0,2,4]))
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
        self.assertEqual(list(a), range(10))
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
        lower = -1 * long(pow(2, a.itemsize * 8 - 1))
        upper = long(pow(2, a.itemsize * 8 - 1)) - 1L
        self.check_overflow(lower, upper)

class UnsignedNumberTest(NumberTest):
    example = [0, 1, 17, 23, 42, 0xff]
    smallerexample = [0, 1, 17, 23, 42, 0xfe]
    biggerexample = [0, 1, 17, 23, 43, 0xff]
    outside = 0xaa

    def test_overflow(self):
        a = array.array(self.typecode)
        lower = 0
        upper = long(pow(2, a.itemsize * 8)) - 1L
        self.check_overflow(lower, upper)

    @test_support.cpython_only
    def test_sizeof_with_buffer(self):
        a = array.array(self.typecode, self.example)
        basesize = test_support.calcvobjsize('4P')
        buffer_size = a.buffer_info()[1] * a.itemsize
        test_support.check_sizeof(self, a, basesize + buffer_size)

    @test_support.cpython_only
    def test_sizeof_without_buffer(self):
        a = array.array(self.typecode)
        basesize = test_support.calcvobjsize('4P')
        test_support.check_sizeof(self, a, basesize)


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


@test_support.requires_unicode
class UnicodeTypecodeTest(unittest.TestCase):
    def test_unicode_typecode(self):
        for typecode in typecodes:
            a = array.array(unicode(typecode))
            self.assertEqual(a.typecode, typecode)
            self.assertIs(type(a.typecode), str)
tests.append(UnicodeTypecodeTest)


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
                self.assertNotEqual(a.tostring(), b.tostring())
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

    test_support.run_unittest(*tests)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*tests)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
