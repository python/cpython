# Tests StringIO and cStringIO

import unittest
import StringIO
import cStringIO
import types
import test_support


class TestGenericStringIO(unittest.TestCase):
    # use a class variable MODULE to define which module is being tested

    def setUp(self):
        self._line = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
        self._lines = (self._line + '\n') * 5
        self._fp = self.MODULE.StringIO(self._lines)

    def test_reads(self):
        eq = self.assertEqual
        eq(self._fp.read(10), 'abcdefghij')
        eq(self._fp.readline(), 'klmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\n')
        eq(len(self._fp.readlines(60)), 2)

    def test_writes(self):
        f = self.MODULE.StringIO()
        f.write('abcdef')
        f.seek(3)
        f.write('uvwxyz')
        f.write('!')
        self.assertEqual(f.getvalue(), 'abcuvwxyz!')

    def test_writelines(self):
        f = self.MODULE.StringIO()
        f.writelines(['a', 'b', 'c'])
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
        it = iter(self._fp)
        self.failUnless(isinstance(it, types.FunctionIterType))
        i = 0
        for line in self._fp:
            eq(line, self._line + '\n')
            i += 1
        eq(i, 5)

class TestStringIO(TestGenericStringIO):
    MODULE = StringIO

class TestcStringIO(TestGenericStringIO):
    MODULE = cStringIO


def test_main():
    test_support.run_unittest(TestStringIO)
    test_support.run_unittest(TestcStringIO)

if __name__ == '__main__':
    test_main()
