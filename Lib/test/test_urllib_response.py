"""Unit tests for code in urllib.response."""

import test.support
import urllib.response
import unittest

class TestFile(object):

    def __init__(self):
        self.closed = False

    def read(self, bytes):
        pass

    def readline(self):
        pass

    def close(self):
        self.closed = True

class Testaddbase(unittest.TestCase):

    # TODO(jhylton): Write tests for other functionality of addbase()

    def setUp(self):
        self.fp = TestFile()
        self.addbase = urllib.response.addbase(self.fp)

    def test_with(self):
        def f():
            with self.addbase as spam:
                pass
        self.assertFalse(self.fp.closed)
        f()
        self.assertTrue(self.fp.closed)
        self.assertRaises(ValueError, f)

def test_main():
    test.support.run_unittest(Testaddbase)

if __name__ == '__main__':
    test_main()
