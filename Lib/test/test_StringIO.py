# Tests StringIO and cStringIO

import unittest
import StringIO
import cStringIO
import types
import test_support


class TestGenericStringIO(unittest.TestCase):
    # use a class variable MODULE to define which module is being tested

    # Line of data to test as string
    _line = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!'

    # Constructor to use for the test data (._line is passed to this
    # constructor)
    constructor = str

    def setUp(self):
        self._line = self.constructor(self._line)
        self._lines = self.constructor((self._line + '\n') * 5)
        self._fp = self.MODULE.StringIO(self._lines)

    def test_reads(self):
        eq = self.assertEqual
        eq(self._fp.read(10), self._line[:10])
        eq(self._fp.readline(), self._line[10:] + '\n')
        eq(len(self._fp.readlines(60)), 2)

    def test_writes(self):
        f = self.MODULE.StringIO()
        f.write(self._line[:6])
        f.seek(3)
        f.write(self._line[20:26])
        f.write(self._line[52])
        self.assertEqual(f.getvalue(), 'abcuvwxyz!')

    def test_writelines(self):
        f = self.MODULE.StringIO()
        f.writelines([self._line[0], self._line[1], self._line[2]])
        f.seek(0)
        self.assertEqual(f.getvalue(), 'abc')

    def test_truncate(self):
        eq = self.assertEqual
        f = self.MODULE.StringIO()
        f.write(self._lines)
        f.seek(10)
        f.truncate()
        eq(f.getvalue(), 'abcdefghij')
        f.seek(0)
        f.truncate(5)
        eq(f.getvalue(), 'abcde')
        f.close()
        self.assertRaises(ValueError, f.write, 'frobnitz')

    def test_iterator(self):
        eq = self.assertEqual
        unless = self.failUnless
        it = iter(self._fp)
        # Does this object support the iteration protocol?
        unless(hasattr(it, '__iter__'))
        unless(hasattr(it, 'next'))
        i = 0
        for line in self._fp:
            eq(line, self._line + '\n')
            i += 1
        eq(i, 5)

class TestStringIO(TestGenericStringIO):
    MODULE = StringIO

    if test_support.have_unicode:
        def test_unicode(self):

            # The StringIO module also supports concatenating Unicode
            # snippets to larger Unicode strings. This is tested by this
            # method. Note that cStringIO does not support this extension.

            f = self.MODULE.StringIO()
            f.write(self._line[:6])
            f.seek(3)
            f.write(unicode(self._line[20:26]))
            f.write(unicode(self._line[52]))
            s = f.getvalue()
            self.assertEqual(s, unicode('abcuvwxyz!'))
            self.assertEqual(type(s), types.UnicodeType)

class TestcStringIO(TestGenericStringIO):
    MODULE = cStringIO

import sys
if sys.platform.startswith('java'):
    # Jython doesn't have a buffer object, so we just do a useless
    # fake of the buffer tests.
    buffer = str

class TestBufferStringIO(TestStringIO):
    constructor = buffer

class TestBuffercStringIO(TestcStringIO):
    constructor = buffer


def test_main():
    test_support.run_unittest(TestStringIO)
    test_support.run_unittest(TestcStringIO)
    test_support.run_unittest(TestBufferStringIO)
    test_support.run_unittest(TestBuffercStringIO)

if __name__ == '__main__':
    test_main()
