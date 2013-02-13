# Tests StringIO and cStringIO

import unittest
import StringIO
import cStringIO
import types
import array
import sys
from test import test_support


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
        self.assertRaises(TypeError, self._fp.seek)
        eq(self._fp.read(10), self._line[:10])
        eq(self._fp.read(0), '')
        eq(self._fp.readline(0), '')
        eq(self._fp.readline(), self._line[10:] + '\n')
        eq(len(self._fp.readlines(60)), 2)
        self._fp.seek(0)
        eq(self._fp.readline(-1), self._line + '\n')

    def test_writes(self):
        f = self.MODULE.StringIO()
        self.assertRaises(TypeError, f.seek)
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

    def test_writelines_error(self):
        def errorGen():
            yield 'a'
            raise KeyboardInterrupt()
        f = self.MODULE.StringIO()
        self.assertRaises(KeyboardInterrupt, f.writelines, errorGen())

    def test_truncate(self):
        eq = self.assertEqual
        f = self.MODULE.StringIO()
        f.write(self._lines)
        f.seek(10)
        f.truncate()
        eq(f.getvalue(), 'abcdefghij')
        f.truncate(5)
        eq(f.getvalue(), 'abcde')
        f.write('xyz')
        eq(f.getvalue(), 'abcdexyz')
        self.assertRaises(IOError, f.truncate, -1)
        f.close()
        self.assertRaises(ValueError, f.write, 'frobnitz')

    def test_closed_flag(self):
        f = self.MODULE.StringIO()
        self.assertEqual(f.closed, False)
        f.close()
        self.assertEqual(f.closed, True)
        f = self.MODULE.StringIO("abc")
        self.assertEqual(f.closed, False)
        f.close()
        self.assertEqual(f.closed, True)

    def test_isatty(self):
        f = self.MODULE.StringIO()
        self.assertRaises(TypeError, f.isatty, None)
        self.assertEqual(f.isatty(), False)
        f.close()
        self.assertRaises(ValueError, f.isatty)

    def test_iterator(self):
        eq = self.assertEqual
        unless = self.assertTrue
        eq(iter(self._fp), self._fp)
        # Does this object support the iteration protocol?
        unless(hasattr(self._fp, '__iter__'))
        unless(hasattr(self._fp, 'next'))
        i = 0
        for line in self._fp:
            eq(line, self._line + '\n')
            i += 1
        eq(i, 5)
        self._fp.close()
        self.assertRaises(ValueError, self._fp.next)

    def test_getvalue(self):
        self._fp.close()
        self.assertRaises(ValueError, self._fp.getvalue)

    @test_support.bigmemtest(test_support._2G + 2**26, memuse=2.001)
    def test_reads_from_large_stream(self, size):
        linesize = 2**26 # 64 MiB
        lines = ['x' * (linesize - 1) + '\n'] * (size // linesize) + \
                ['y' * (size % linesize)]
        f = self.MODULE.StringIO(''.join(lines))
        for i, expected in enumerate(lines):
            line = f.read(len(expected))
            self.assertEqual(len(line), len(expected))
            self.assertEqual(line, expected)
        self.assertEqual(f.read(), '')
        f.seek(0)
        for i, expected in enumerate(lines):
            line = f.readline()
            self.assertEqual(len(line), len(expected))
            self.assertEqual(line, expected)
        self.assertEqual(f.readline(), '')
        f.seek(0)
        self.assertEqual(f.readlines(), lines)
        self.assertEqual(f.readlines(), [])
        f.seek(0)
        self.assertEqual(f.readlines(size), lines)
        self.assertEqual(f.readlines(), [])

    # In worst case cStringIO requires 2 + 1 + 1/2 + 1/2**2 + ... = 4
    # bytes per input character.
    @test_support.bigmemtest(test_support._2G, memuse=4)
    def test_writes_to_large_stream(self, size):
        s = 'x' * 2**26 # 64 MiB
        f = self.MODULE.StringIO()
        n = size
        while n > len(s):
            f.write(s)
            n -= len(s)
        s = None
        f.write('x' * n)
        self.assertEqual(len(f.getvalue()), size)


class TestStringIO(TestGenericStringIO):
    MODULE = StringIO

    def test_unicode(self):

        if not test_support.have_unicode: return

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

    def test_array_support(self):
        # Issue #1730114: cStringIO should accept array objects
        a = array.array('B', [0,1,2])
        f = self.MODULE.StringIO(a)
        self.assertEqual(f.getvalue(), '\x00\x01\x02')

    def test_unicode(self):

        if not test_support.have_unicode: return

        # The cStringIO module converts Unicode strings to character
        # strings when writing them to cStringIO objects.
        # Check that this works.

        f = self.MODULE.StringIO()
        f.write(u'abcde')
        s = f.getvalue()
        self.assertEqual(s, 'abcde')
        self.assertEqual(type(s), str)

        f = self.MODULE.StringIO(u'abcde')
        s = f.getvalue()
        self.assertEqual(s, 'abcde')
        self.assertEqual(type(s), str)

        self.assertRaises(UnicodeEncodeError, self.MODULE.StringIO, u'\xf4')


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
    test_support.run_unittest(TestStringIO, TestcStringIO)
    with test_support.check_py3k_warnings(("buffer.. not supported",
                                             DeprecationWarning)):
        test_support.run_unittest(TestBufferStringIO, TestBuffercStringIO)

if __name__ == '__main__':
    test_main()
