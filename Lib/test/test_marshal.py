#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-

from test import test_support
import marshal
import sys
import unittest
import os

class IntTestCase(unittest.TestCase):
    def test_ints(self):
        # Test the full range of Python ints.
        n = sys.maxint
        while n:
            for expected in (-n, n):
                s = marshal.dumps(expected)
                got = marshal.loads(s)
                self.assertEqual(expected, got)
                marshal.dump(expected, file(test_support.TESTFN, "wb"))
                got = marshal.load(file(test_support.TESTFN, "rb"))
                self.assertEqual(expected, got)
            n = n >> 1
        os.unlink(test_support.TESTFN)

    def test_int64(self):
        # Simulate int marshaling on a 64-bit box.  This is most interesting if
        # we're running the test on a 32-bit box, of course.

        def to_little_endian_string(value, nbytes):
            bytes = []
            for i in range(nbytes):
                bytes.append(chr(value & 0xff))
                value >>= 8
            return ''.join(bytes)

        maxint64 = (1L << 63) - 1
        minint64 = -maxint64-1

        for base in maxint64, minint64, -maxint64, -(minint64 >> 1):
            while base:
                s = 'I' + to_little_endian_string(base, 8)
                got = marshal.loads(s)
                self.assertEqual(base, got)
                if base == -1:  # a fixed-point for shifting right 1
                    base = 0
                else:
                    base >>= 1

    def test_bool(self):
        for b in (True, False):
            new = marshal.loads(marshal.dumps(b))
            self.assertEqual(b, new)
            self.assertEqual(type(b), type(new))
            marshal.dump(b, file(test_support.TESTFN, "wb"))
            new = marshal.load(file(test_support.TESTFN, "rb"))
            self.assertEqual(b, new)
            self.assertEqual(type(b), type(new))

class FloatTestCase(unittest.TestCase):
    def test_floats(self):
        # Test a few floats
        small = 1e-25
        n = sys.maxint * 3.7e250
        while n > small:
            for expected in (-n, n):
                f = float(expected)
                s = marshal.dumps(f)
                got = marshal.loads(s)
                self.assertEqual(f, got)
                marshal.dump(f, file(test_support.TESTFN, "wb"))
                got = marshal.load(file(test_support.TESTFN, "rb"))
                self.assertEqual(f, got)
            n /= 123.4567

        f = 0.0
        s = marshal.dumps(f, 2)
        got = marshal.loads(s)
        self.assertEqual(f, got)
        # and with version <= 1 (floats marshalled differently then)
        s = marshal.dumps(f, 1)
        got = marshal.loads(s)
        self.assertEqual(f, got)

        n = sys.maxint * 3.7e-250
        while n < small:
            for expected in (-n, n):
                f = float(expected)

                s = marshal.dumps(f)
                got = marshal.loads(s)
                self.assertEqual(f, got)

                s = marshal.dumps(f, 1)
                got = marshal.loads(s)
                self.assertEqual(f, got)

                marshal.dump(f, file(test_support.TESTFN, "wb"))
                got = marshal.load(file(test_support.TESTFN, "rb"))
                self.assertEqual(f, got)

                marshal.dump(f, file(test_support.TESTFN, "wb"), 1)
                got = marshal.load(file(test_support.TESTFN, "rb"))
                self.assertEqual(f, got)
            n *= 123.4567
        os.unlink(test_support.TESTFN)

class StringTestCase(unittest.TestCase):
    def test_unicode(self):
        for s in [u"", u"Andrè Previn", u"abc", u" "*10000]:
            new = marshal.loads(marshal.dumps(s))
            self.assertEqual(s, new)
            self.assertEqual(type(s), type(new))
            marshal.dump(s, file(test_support.TESTFN, "wb"))
            new = marshal.load(file(test_support.TESTFN, "rb"))
            self.assertEqual(s, new)
            self.assertEqual(type(s), type(new))
        os.unlink(test_support.TESTFN)

    def test_string(self):
        for s in ["", "Andrè Previn", "abc", " "*10000]:
            new = marshal.loads(marshal.dumps(s))
            self.assertEqual(s, new)
            self.assertEqual(type(s), type(new))
            marshal.dump(s, file(test_support.TESTFN, "wb"))
            new = marshal.load(file(test_support.TESTFN, "rb"))
            self.assertEqual(s, new)
            self.assertEqual(type(s), type(new))
        os.unlink(test_support.TESTFN)

    def test_buffer(self):
        for s in ["", "Andrè Previn", "abc", " "*10000]:
            b = buffer(s)
            new = marshal.loads(marshal.dumps(b))
            self.assertEqual(s, new)
            marshal.dump(b, file(test_support.TESTFN, "wb"))
            new = marshal.load(file(test_support.TESTFN, "rb"))
            self.assertEqual(s, new)
        os.unlink(test_support.TESTFN)

class ExceptionTestCase(unittest.TestCase):
    def test_exceptions(self):
        new = marshal.loads(marshal.dumps(StopIteration))
        self.assertEqual(StopIteration, new)

class CodeTestCase(unittest.TestCase):
    def test_code(self):
        co = ExceptionTestCase.test_exceptions.func_code
        new = marshal.loads(marshal.dumps(co))
        self.assertEqual(co, new)

class ContainerTestCase(unittest.TestCase):
    d = {'astring': 'foo@bar.baz.spam',
         'afloat': 7283.43,
         'anint': 2**20,
         'ashortlong': 2L,
         'alist': ['.zyx.41'],
         'atuple': ('.zyx.41',)*10,
         'aboolean': False,
         'aunicode': u"Andrè Previn"
         }
    def test_dict(self):
        new = marshal.loads(marshal.dumps(self.d))
        self.assertEqual(self.d, new)
        marshal.dump(self.d, file(test_support.TESTFN, "wb"))
        new = marshal.load(file(test_support.TESTFN, "rb"))
        self.assertEqual(self.d, new)
        os.unlink(test_support.TESTFN)

    def test_list(self):
        lst = self.d.items()
        new = marshal.loads(marshal.dumps(lst))
        self.assertEqual(lst, new)
        marshal.dump(lst, file(test_support.TESTFN, "wb"))
        new = marshal.load(file(test_support.TESTFN, "rb"))
        self.assertEqual(lst, new)
        os.unlink(test_support.TESTFN)

    def test_tuple(self):
        t = tuple(self.d.keys())
        new = marshal.loads(marshal.dumps(t))
        self.assertEqual(t, new)
        marshal.dump(t, file(test_support.TESTFN, "wb"))
        new = marshal.load(file(test_support.TESTFN, "rb"))
        self.assertEqual(t, new)
        os.unlink(test_support.TESTFN)

    def test_sets(self):
        for constructor in (set, frozenset):
            t = constructor(self.d.keys())
            new = marshal.loads(marshal.dumps(t))
            self.assertEqual(t, new)
            self.assert_(isinstance(new, constructor))
            self.assertNotEqual(id(t), id(new))
            marshal.dump(t, file(test_support.TESTFN, "wb"))
            new = marshal.load(file(test_support.TESTFN, "rb"))
            self.assertEqual(t, new)
            os.unlink(test_support.TESTFN)

class BugsTestCase(unittest.TestCase):
    def test_bug_5888452(self):
        # Simple-minded check for SF 588452: Debug build crashes
        marshal.dumps([128] * 1000)

    def test_patch_873224(self):
        self.assertRaises(Exception, marshal.loads, '0')
        self.assertRaises(Exception, marshal.loads, 'f')
        self.assertRaises(Exception, marshal.loads, marshal.dumps(5L)[:-1])

    def test_version_argument(self):
        # Python 2.4.0 crashes for any call to marshal.dumps(x, y)
        self.assertEquals(marshal.loads(marshal.dumps(5, 0)), 5)
        self.assertEquals(marshal.loads(marshal.dumps(5, 1)), 5)

    def test_fuzz(self):
        # simple test that it's at least not *totally* trivial to
        # crash from bad marshal data
        for c in [chr(i) for i in range(256)]:
            try:
                marshal.loads(c)
            except Exception:
                pass

    def test_loads_recursion(self):
        s = 'c' + ('X' * 4*4) + '{' * 2**20
        self.assertRaises(ValueError, marshal.loads, s)

    def test_recursion_limit(self):
        # Create a deeply nested structure.
        head = last = []
        # The max stack depth should match the value in Python/marshal.c.
        MAX_MARSHAL_STACK_DEPTH = 2000
        for i in range(MAX_MARSHAL_STACK_DEPTH - 2):
            last.append([0])
            last = last[-1]

        # Verify we don't blow out the stack with dumps/load.
        data = marshal.dumps(head)
        new_head = marshal.loads(data)
        # Don't use == to compare objects, it can exceed the recursion limit.
        self.assertEqual(len(new_head), len(head))
        self.assertEqual(len(new_head[0]), len(head[0]))
        self.assertEqual(len(new_head[-1]), len(head[-1]))

        last.append([0])
        self.assertRaises(ValueError, marshal.dumps, head)

    def test_exact_type_match(self):
        # Former bug:
        #   >>> class Int(int): pass
        #   >>> type(loads(dumps(Int())))
        #   <type 'int'>
        for typ in (int, long, float, complex, tuple, list, dict, set, frozenset):
            # Note: str and unicode sublclasses are not tested because they get handled
            # by marshal's routines for objects supporting the buffer API.
            subtyp = type('subtyp', (typ,), {})
            self.assertRaises(ValueError, marshal.dumps, subtyp())

    # Issue #1792 introduced a change in how marshal increases the size of its
    # internal buffer; this test ensures that the new code is exercised.
    def test_large_marshal(self):
        size = int(1e6)
        testString = 'abc' * size
        marshal.dumps(testString)

    def test_invalid_longs(self):
        # Issue #7019: marshal.loads shouldn't produce unnormalized PyLongs
        invalid_string = 'l\x02\x00\x00\x00\x00\x00\x00\x00'
        self.assertRaises(ValueError, marshal.loads, invalid_string)


def test_main():
    test_support.run_unittest(IntTestCase,
                              FloatTestCase,
                              StringTestCase,
                              CodeTestCase,
                              ContainerTestCase,
                              ExceptionTestCase,
                              BugsTestCase)

if __name__ == "__main__":
    test_main()
