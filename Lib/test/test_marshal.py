#!/usr/bin/env python

from test import test_support
import marshal
import sys
import unittest
import os

class HelperMixin:
    def helper(self, sample, *extra):
        new = marshal.loads(marshal.dumps(sample, *extra))
        self.assertEqual(sample, new)
        try:
            f = open(test_support.TESTFN, "wb")
            try:
                marshal.dump(sample, f, *extra)
            finally:
                f.close()
            f = open(test_support.TESTFN, "rb")
            try:
                new = marshal.load(f)
            finally:
                f.close()
            self.assertEqual(sample, new)
        finally:
            test_support.unlink(test_support.TESTFN)

class IntTestCase(unittest.TestCase, HelperMixin):
    def test_ints(self):
        # Test the full range of Python ints.
        n = sys.maxint
        while n:
            for expected in (-n, n):
                self.helper(expected)
            n = n >> 1

    def test_int64(self):
        # Simulate int marshaling on a 64-bit box.  This is most interesting if
        # we're running the test on a 32-bit box, of course.

        def to_little_endian_string(value, nbytes):
            b = bytes()
            for i in range(nbytes):
                b.append(value & 0xff)
                value >>= 8
            return b

        maxint64 = (1 << 63) - 1
        minint64 = -maxint64-1

        for base in maxint64, minint64, -maxint64, -(minint64 >> 1):
            while base:
                s = b'I' + to_little_endian_string(base, 8)
                got = marshal.loads(s)
                self.assertEqual(base, got)
                if base == -1:  # a fixed-point for shifting right 1
                    base = 0
                else:
                    base >>= 1

    def test_bool(self):
        for b in (True, False):
            self.helper(b)

class FloatTestCase(unittest.TestCase, HelperMixin):
    def test_floats(self):
        # Test a few floats
        small = 1e-25
        n = sys.maxint * 3.7e250
        while n > small:
            for expected in (-n, n):
                self.helper(float(expected))
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
                self.helper(f)
                self.helper(f, 1)
            n *= 123.4567

class StringTestCase(unittest.TestCase, HelperMixin):
    def test_unicode(self):
        for s in ["", "Andr\xe8 Previn", "abc", " "*10000]:
            self.helper(marshal.loads(marshal.dumps(s)))

    def test_string(self):
        for s in ["", "Andr\xe8 Previn", "abc", " "*10000]:
            self.helper(s)

    def test_buffer(self):
        for s in [b"", b"Andr\xe8 Previn", b"abc", b" "*10000]:
            self.helper(buffer(s))

class ExceptionTestCase(unittest.TestCase):
    def test_exceptions(self):
        new = marshal.loads(marshal.dumps(StopIteration))
        self.assertEqual(StopIteration, new)

class CodeTestCase(unittest.TestCase):
    def test_code(self):
        co = ExceptionTestCase.test_exceptions.__code__
        new = marshal.loads(marshal.dumps(co))
        self.assertEqual(co, new)

class ContainerTestCase(unittest.TestCase, HelperMixin):
    d = {'astring': 'foo@bar.baz.spam',
         'afloat': 7283.43,
         'anint': 2**20,
         'ashortlong': 2,
         'alist': ['.zyx.41'],
         'atuple': ('.zyx.41',)*10,
         'aboolean': False,
         'aunicode': "Andr\xe8 Previn"
         }

    def test_dict(self):
        self.helper(self.d)

    def test_list(self):
        self.helper(list(self.d.items()))

    def test_tuple(self):
        self.helper(tuple(self.d.keys()))

    def test_sets(self):
        for constructor in (set, frozenset):
            self.helper(constructor(self.d.keys()))

class BugsTestCase(unittest.TestCase):
    def test_bug_5888452(self):
        # Simple-minded check for SF 588452: Debug build crashes
        marshal.dumps([128] * 1000)

    def test_patch_873224(self):
        self.assertRaises(Exception, marshal.loads, '0')
        self.assertRaises(Exception, marshal.loads, 'f')
        self.assertRaises(Exception, marshal.loads, marshal.dumps(2**65)[:-1])

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
        if os.name == 'nt' and hasattr(sys, 'gettotalrefcount'):
            MAX_MARSHAL_STACK_DEPTH = 1500
        else:
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
