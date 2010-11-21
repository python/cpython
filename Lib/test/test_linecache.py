""" Tests for the linecache module """

import linecache
import unittest
import os.path
from test import support


FILENAME = linecache.__file__
INVALID_NAME = '!@$)(!@#_1'
EMPTY = ''
TESTS = 'cjkencodings_test inspect_fodder inspect_fodder2 mapping_tests'
TESTS = TESTS.split()
TEST_PATH = os.path.dirname(support.__file__)
MODULES = "linecache unittest".split()
MODULE_PATH = os.path.dirname(FILENAME)

SOURCE_1 = '''
" Docstring "

def function():
    return result

'''

SOURCE_2 = '''
def f():
    return 1 + 1

a = f()

'''

SOURCE_3 = '''
def f():
    return 3''' # No ending newline


class LineCacheTests(unittest.TestCase):

    def test_getline(self):
        getline = linecache.getline

        # Bad values for line number should return an empty string
        self.assertEqual(getline(FILENAME, 2**15), EMPTY)
        self.assertEqual(getline(FILENAME, -1), EMPTY)

        # Float values currently raise TypeError, should it?
        self.assertRaises(TypeError, getline, FILENAME, 1.1)

        # Bad filenames should return an empty string
        self.assertEqual(getline(EMPTY, 1), EMPTY)
        self.assertEqual(getline(INVALID_NAME, 1), EMPTY)

        # Check whether lines correspond to those from file iteration
        for entry in TESTS:
            filename = os.path.join(TEST_PATH, entry) + '.py'
            for index, line in enumerate(open(filename)):
                self.assertEqual(line, getline(filename, index + 1))

        # Check module loading
        for entry in MODULES:
            filename = os.path.join(MODULE_PATH, entry) + '.py'
            for index, line in enumerate(open(filename)):
                self.assertEqual(line, getline(filename, index + 1))

        # Check that bogus data isn't returned (issue #1309567)
        empty = linecache.getlines('a/b/c/__init__.py')
        self.assertEqual(empty, [])

    def test_no_ending_newline(self):
        self.addCleanup(support.unlink, support.TESTFN)
        with open(support.TESTFN, "w") as fp:
            fp.write(SOURCE_3)
        lines = linecache.getlines(support.TESTFN)
        self.assertEqual(lines, ["\n", "def f():\n", "    return 3\n"])

    def test_clearcache(self):
        cached = []
        for entry in TESTS:
            filename = os.path.join(TEST_PATH, entry) + '.py'
            cached.append(filename)
            linecache.getline(filename, 1)

        # Are all files cached?
        cached_empty = [fn for fn in cached if fn not in linecache.cache]
        self.assertEqual(cached_empty, [])

        # Can we clear the cache?
        linecache.clearcache()
        cached_empty = [fn for fn in cached if fn in linecache.cache]
        self.assertEqual(cached_empty, [])

    def test_checkcache(self):
        getline = linecache.getline
        # Create a source file and cache its contents
        source_name = support.TESTFN + '.py'
        self.addCleanup(support.unlink, source_name)
        with open(source_name, 'w') as source:
            source.write(SOURCE_1)
        getline(source_name, 1)
        # Keep a copy of the old contents
        source_list = []
        with open(source_name) as source:
            for index, line in enumerate(source):
                self.assertEqual(line, getline(source_name, index + 1))
                source_list.append(line)

        with open(source_name, 'w') as source:
            source.write(SOURCE_2)

        # Try to update a bogus cache entry
        linecache.checkcache('dummy')

        # Check that the cache matches the old contents
        for index, line in enumerate(source_list):
            self.assertEqual(line, getline(source_name, index + 1))

        # Update the cache and check whether it matches the new source file
        linecache.checkcache(source_name)
        with open(source_name) as source:
            for index, line in enumerate(source):
                self.assertEqual(line, getline(source_name, index + 1))
                source_list.append(line)


def test_main():
    support.run_unittest(LineCacheTests)

if __name__ == "__main__":
    test_main()
