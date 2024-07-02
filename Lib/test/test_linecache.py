""" Tests for the linecache module """

import linecache
import unittest
import unittest.mock
import os.path
import tempfile
import tokenize
from importlib.machinery import ModuleSpec
from test import support
from test.support import os_helper


FILENAME = linecache.__file__
NONEXISTENT_FILENAME = FILENAME + '.missing'
INVALID_NAME = '!@$)(!@#_1'
EMPTY = ''
TEST_PATH = os.path.dirname(__file__)
MODULES = "linecache abc".split()
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


class TempFile:

    def setUp(self):
        super().setUp()
        with tempfile.NamedTemporaryFile(delete=False) as fp:
            self.file_name = fp.name
            fp.write(self.file_byte_string)
        self.addCleanup(os_helper.unlink, self.file_name)
        # ensure that the cache is empty between tests
        linecache.cache.clear()
        linecache.failures.clear()


class GetLineTestsGoodData(TempFile):
    # file_list   = ['list\n', 'of\n', 'good\n', 'strings\n']

    def setUp(self):
        self.file_byte_string = ''.join(self.file_list).encode('utf-8')
        super().setUp()

    def test_getline(self):
        with tokenize.open(self.file_name) as fp:
            for index, line in enumerate(fp):
                if not line.endswith('\n'):
                    line += '\n'

                cached_line = linecache.getline(self.file_name, index + 1)
                self.assertEqual(line, cached_line)

    def test_getlines(self):
        lines = linecache.getlines(self.file_name)
        self.assertEqual(lines, self.file_list)


class GetLineTestsBadData(TempFile):
    # file_byte_string = b'Bad data goes here'

    def test_getline(self):
        self.assertEqual(linecache.getline(self.file_name, 1), '')
        self.assertDictEqual(linecache.cache, {})

        size, mtime, fullname = linecache.failures[self.file_name]
        self.assertIsInstance(size, int)
        self.assertIsInstance(mtime, int)

    def test_getlines(self):
        self.assertEqual(linecache.getlines(self.file_name), [])
        self.assertDictEqual(linecache.cache, {})

        size, mtime, fullname = linecache.failures[self.file_name]
        self.assertIsInstance(size, int)
        self.assertIsInstance(mtime, int)

    def test_getline_called_once(self):
        with unittest.mock.patch('tokenize.open', wraps=tokenize.open) as tok:
            self.assertEqual(linecache.getline(self.file_name, 1), '')
            tok.assert_called_once_with(self.file_name)
            self.assertDictEqual(linecache.cache, {})

            size, mtime, fullname = linecache.failures[self.file_name]
            self.assertIsInstance(size, int)
            self.assertIsInstance(mtime, int)

        with unittest.mock.patch('tokenize.open', wraps=tokenize.open) as tok:
            self.assertEqual(linecache.getline(self.file_name, 1), '')
            tok.assert_not_called()
            self.assertEqual(linecache.getline(self.file_name, 1), '')
            tok.assert_not_called()
            self.assertDictEqual(linecache.cache, {})

            failure = linecache.failures[self.file_name]
            self.assertEqual(failure[0], size)
            self.assertAlmostEqual(failure[1], mtime)

    def test_getlines_called_once(self):
        with unittest.mock.patch('tokenize.open', wraps=tokenize.open) as tok:
            self.assertEqual(linecache.getlines(self.file_name), [])
            tok.assert_called_once_with(self.file_name)
            self.assertDictEqual(linecache.cache, {})

            size, mtime, fullname = linecache.failures[self.file_name]
            self.assertIsInstance(size, int)
            self.assertIsInstance(mtime, int)

        with unittest.mock.patch('tokenize.open', wraps=tokenize.open) as tok:
            self.assertEqual(linecache.getlines(self.file_name), [])
            tok.assert_not_called()
            self.assertEqual(linecache.getlines(self.file_name), [])
            tok.assert_not_called()
            self.assertDictEqual(linecache.cache, {})

            failure = linecache.failures[self.file_name]
            self.assertEqual(failure[0], size)
            self.assertAlmostEqual(failure[1], mtime)


class EmptyFile(GetLineTestsGoodData, unittest.TestCase):
    file_list = []

    def test_getlines(self):
        lines = linecache.getlines(self.file_name)
        self.assertEqual(lines, ['\n'])


class SingleEmptyLine(GetLineTestsGoodData, unittest.TestCase):
    file_list = ['\n']


class GoodUnicode(GetLineTestsGoodData, unittest.TestCase):
    file_list = ['á\n', 'b\n', 'abcdef\n', 'ááááá\n']

class BadUnicode_NoDeclaration(GetLineTestsBadData, unittest.TestCase):
    file_byte_string = b'\n\x80abc'

class BadUnicode_WithDeclaration(GetLineTestsBadData, unittest.TestCase):
    file_byte_string = b'# coding=utf-8\n\x80abc'


class FakeLoader:
    def get_source(self, fullname):
        return f'source for {fullname}'


class NoSourceLoader:
    def get_source(self, fullname):
        return None


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

        # Check module loading
        for entry in MODULES:
            filename = os.path.join(MODULE_PATH, entry) + '.py'
            with open(filename, encoding='utf-8') as file:
                for index, line in enumerate(file):
                    self.assertEqual(line, getline(filename, index + 1))

        # Check that bogus data isn't returned (issue #1309567)
        empty = linecache.getlines('a/b/c/__init__.py')
        self.assertEqual(empty, [])

    def test_no_ending_newline(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(os_helper.TESTFN, "w", encoding='utf-8') as fp:
            fp.write(SOURCE_3)
        lines = linecache.getlines(os_helper.TESTFN)
        self.assertEqual(lines, ["\n", "def f():\n", "    return 3\n"])

    def test_clearcache(self):
        cached = []
        for entry in MODULES:
            filename = os.path.join(MODULE_PATH, entry) + '.py'
            cached.append(filename)
            linecache.getline(filename, 1)

        # Are all files cached?
        self.assertNotEqual(cached, [])
        cached_empty = [fn for fn in cached if fn not in linecache.cache]
        self.assertEqual(cached_empty, [])

        # Can we clear the cache?
        linecache.clearcache()
        cached_empty = [fn for fn in cached if fn in linecache.cache]
        self.assertEqual(cached_empty, [])

    def test_clearcache_failures(self):
        # assert that the cache is cleared at the beginning of the test
        linecache.clearcache()
        self.assertListEqual(list(linecache.cache), [])
        self.assertListEqual(list(linecache.failures), [])

        # try to cache a failure
        linecache.getline(NONEXISTENT_FILENAME, 1)
        self.assertListEqual(list(linecache.cache), [])
        self.assertListEqual(list(linecache.failures), [NONEXISTENT_FILENAME])

        # check that the cache is cleared
        linecache.clearcache()
        self.assertListEqual(list(linecache.cache), [])
        self.assertListEqual(list(linecache.failures), [])

    def test_checkcache(self):
        getline = linecache.getline
        # Create a source file and cache its contents
        source_name = os_helper.TESTFN + '.py'
        self.addCleanup(os_helper.unlink, source_name)
        with open(source_name, 'w', encoding='utf-8') as source:
            source.write(SOURCE_1)
        getline(source_name, 1)

        # Keep a copy of the old contents
        source_list = []
        with open(source_name, encoding='utf-8') as source:
            for index, line in enumerate(source):
                self.assertEqual(line, getline(source_name, index + 1))
                source_list.append(line)

        with open(source_name, 'w', encoding='utf-8') as source:
            source.write(SOURCE_2)

        # Try to update a bogus cache entry
        linecache.checkcache('dummy')

        # Check that the cache matches the old contents
        for index, line in enumerate(source_list):
            self.assertEqual(line, getline(source_name, index + 1))

        # Update the cache and check whether it matches the new source file
        linecache.checkcache(source_name)
        with open(source_name, encoding='utf-8') as source:
            for index, line in enumerate(source):
                self.assertEqual(line, getline(source_name, index + 1))
                source_list.append(line)

    def test_checkcache_removed(self):
        linecache.clearcache()
        # case: check that a cached file is never a failure even when removed
        filename = os_helper.TESTFN + '.py'
        self.addCleanup(os_helper.unlink, filename)
        with open(filename, 'w', encoding='utf-8') as source:
            source.write(SOURCE_1)
        # get the cached data
        self.assertNotIn(filename, linecache.cache)
        self.assertNotIn(filename, linecache.failures)
        data = linecache.getlines(filename)
        self.assertIn(filename, linecache.cache)
        self.assertNotIn(filename, linecache.failures)
        # remove the file
        os_helper.unlink(filename)
        # check that the data is still cached
        data = linecache.getlines(filename)
        self.assertIn(filename, linecache.cache)
        self.assertNotIn(filename, linecache.failures)
        # invalidate previous failures
        linecache.checkcache(filename)
        # no more cached
        self.assertNotIn(filename, linecache.cache)
        # the file is not stat()'able!
        self.assertIn(filename, linecache.failures)
        self.assertIsNone(linecache.failures[filename][0])
        self.assertIsNone(linecache.failures[filename][1])
        with unittest.mock.patch('os.stat') as statfn:
            lines = linecache.getlines(filename)
            statfn.assert_not_called()
            self.assertListEqual(lines, [])
        self.assertNotIn(filename, linecache.cache)
        self.assertIn(filename, linecache.failures)
        self.assertIsNone(linecache.failures[filename][0])
        self.assertIsNone(linecache.failures[filename][1])

    def test_lazycache_no_globals(self):
        lines = linecache.getlines(FILENAME)
        linecache.clearcache()
        self.assertEqual(False, linecache.lazycache(FILENAME, None))
        self.assertEqual(lines, linecache.getlines(FILENAME))

    def test_lazycache_smoke(self):
        lines = linecache.getlines(NONEXISTENT_FILENAME, globals())
        linecache.clearcache()
        self.assertEqual(
            True, linecache.lazycache(NONEXISTENT_FILENAME, globals()))
        self.assertEqual(1, len(linecache.cache[NONEXISTENT_FILENAME]))
        # Note here that we're looking up a nonexistent filename with no
        # globals: this would error if the lazy value wasn't resolved.
        self.assertEqual(lines, linecache.getlines(NONEXISTENT_FILENAME))

    def test_lazycache_provide_after_failed_lookup(self):
        linecache.clearcache()
        lines = linecache.getlines(NONEXISTENT_FILENAME, globals())
        linecache.clearcache()
        linecache.getlines(NONEXISTENT_FILENAME)
        linecache.lazycache(NONEXISTENT_FILENAME, globals())
        self.assertEqual(lines, linecache.updatecache(NONEXISTENT_FILENAME))

    def test_lazycache_check(self):
        linecache.clearcache()
        linecache.lazycache(NONEXISTENT_FILENAME, globals())
        linecache.checkcache()

    def test_lazycache_bad_filename(self):
        linecache.clearcache()
        self.assertEqual(False, linecache.lazycache('', globals()))
        self.assertEqual(False, linecache.lazycache('<foo>', globals()))

    def test_lazycache_already_cached(self):
        linecache.clearcache()
        lines = linecache.getlines(NONEXISTENT_FILENAME, globals())
        self.assertEqual(
            False,
            linecache.lazycache(NONEXISTENT_FILENAME, globals()))
        self.assertEqual(4, len(linecache.cache[NONEXISTENT_FILENAME]))

    def test_memoryerror(self):
        lines = linecache.getlines(FILENAME)
        self.assertTrue(lines)
        def raise_memoryerror(*args, **kwargs):
            raise MemoryError
        with support.swap_attr(linecache, 'updatecache', raise_memoryerror):
            lines2 = linecache.getlines(FILENAME)
        self.assertEqual(lines2, lines)

        linecache.clearcache()
        with support.swap_attr(linecache, 'updatecache', raise_memoryerror):
            lines3 = linecache.getlines(FILENAME)
        self.assertEqual(lines3, [])
        self.assertEqual(linecache.getlines(FILENAME), lines)

    def test_loader(self):
        filename = 'scheme://path'

        for loader in (None, object(), NoSourceLoader()):
            linecache.clearcache()
            module_globals = {'__name__': 'a.b.c', '__loader__': loader}
            self.assertEqual(linecache.getlines(filename, module_globals), [])

        linecache.clearcache()
        module_globals = {'__name__': 'a.b.c', '__loader__': FakeLoader()}
        self.assertEqual(linecache.getlines(filename, module_globals),
                         ['source for a.b.c\n'])

        for spec in (None, object(), ModuleSpec('', FakeLoader())):
            linecache.clearcache()
            module_globals = {'__name__': 'a.b.c', '__loader__': FakeLoader(),
                              '__spec__': spec}
            self.assertEqual(linecache.getlines(filename, module_globals),
                             ['source for a.b.c\n'])

        linecache.clearcache()
        spec = ModuleSpec('x.y.z', FakeLoader())
        module_globals = {'__name__': 'a.b.c', '__loader__': spec.loader,
                          '__spec__': spec}
        self.assertEqual(linecache.getlines(filename, module_globals),
                         ['source for x.y.z\n'])


class LineCacheInvalidationTests(unittest.TestCase):
    def setUp(self):
        super().setUp()
        linecache.clearcache()
        self.deleted_file = os_helper.TESTFN + '.1'
        self.modified_file = os_helper.TESTFN + '.2'
        self.unchanged_file = os_helper.TESTFN + '.3'

        for fname in (self.deleted_file,
                      self.modified_file,
                      self.unchanged_file):
            self.addCleanup(os_helper.unlink, fname)
            with open(fname, 'w', encoding='utf-8') as source:
                source.write(f'print("I am {fname}")')

            self.assertNotIn(fname, linecache.cache)
            linecache.getlines(fname)
            self.assertIn(fname, linecache.cache)

        os.remove(self.deleted_file)
        with open(self.modified_file, 'w', encoding='utf-8') as source:
            source.write('print("was modified")')

    def test_checkcache_for_deleted_file(self):
        linecache.checkcache(self.deleted_file)
        self.assertNotIn(self.deleted_file, linecache.cache)
        self.assertIn(self.modified_file, linecache.cache)
        self.assertIn(self.unchanged_file, linecache.cache)

    def test_checkcache_for_modified_file(self):
        linecache.checkcache(self.modified_file)
        self.assertIn(self.deleted_file, linecache.cache)
        self.assertNotIn(self.modified_file, linecache.cache)
        self.assertIn(self.unchanged_file, linecache.cache)

    def test_checkcache_with_no_parameter(self):
        linecache.checkcache()
        self.assertNotIn(self.deleted_file, linecache.cache)
        self.assertNotIn(self.modified_file, linecache.cache)
        self.assertIn(self.unchanged_file, linecache.cache)


class LineCacheUpdateTests(unittest.TestCase):

    def setUp(self):
        linecache.clearcache()
        self.filename = os_helper.TESTFN + '.py'
        self.addCleanup(os_helper.unlink, self.filename)

    def write_good_data(self):
        with open(self.filename, 'w', encoding='utf-8') as source:
            source.write(SOURCE_1)
        with open(self.filename, encoding='utf-8') as source:
            return source.readlines()

    def write_bad_data(self):
        with open(self.filename, 'wb') as source:
            source.write(b'\n\x80')

    def test_updatecache_with_missing_file(self):
        # remove the filename
        os_helper.unlink(self.filename)
        with unittest.mock.patch('tokenize.open', wraps=tokenize.open) as tok:
            for _ in range(3):
                linecache.updatecache(self.filename)
                tok.assert_not_called()
                self.assertDictEqual(linecache.cache, {})
                self.assertIn(self.filename, linecache.failures)
                failure = linecache.failures[self.filename]
                self.assertIsNone(failure[0])
                self.assertIsNone(failure[1])

    def test_updatecache_after_fix_missing(self):
        # remove the filename
        os_helper.unlink(self.filename)
        linecache.updatecache(self.filename)
        self.assertDictEqual(linecache.cache, {})
        self.assertIn(self.filename, linecache.failures)
        failure = linecache.failures[self.filename]
        self.assertIsNone(failure[0])
        self.assertIsNone(failure[1])
        # patch the failure
        data = self.write_good_data()
        linecache.updatecache(self.filename)
        self.assertDictEqual(linecache.failures, {})
        self.assertIn(self.filename, linecache.cache)
        self.assertListEqual(linecache.getlines(self.filename), data)

    def test_updatecache_after_fix_decoding(self):
        self.write_bad_data()
        # mark it as a failure
        with unittest.mock.patch('tokenize.open', wraps=tokenize.open) as tok:
            linecache.updatecache(self.filename)
            tok.assert_called_once_with(self.filename)

        self.assertDictEqual(linecache.cache, {})
        self.assertIn(self.filename, linecache.failures)
        failure = linecache.failures[self.filename]
        # failure but still with known stats
        self.assertIsInstance(failure[0], int)
        self.assertIsInstance(failure[1], int)
        size, mtime, fullname = failure

        with unittest.mock.patch('tokenize.open', wraps=tokenize.open) as tok:
            for _ in range(3):
                linecache.updatecache(self.filename)
                tok.assert_not_called()
                self.assertDictEqual(linecache.cache, {})
                self.assertIn(self.filename, linecache.failures)
                failure = linecache.failures[self.filename]
                # failure but still with same stats
                self.assertEqual(failure[0], size)
                self.assertAlmostEqual(failure[1], mtime)

        # patch the unicode content
        data = self.write_good_data()
        linecache.updatecache(self.filename)
        self.assertDictEqual(linecache.failures, {})
        self.assertIn(self.filename, linecache.cache)
        self.assertListEqual(linecache.getlines(self.filename), data)


if __name__ == "__main__":
    unittest.main()
