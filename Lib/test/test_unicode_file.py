# Test some Unicode file name semantics
# We don't test many operations on files other than
# that their names can be used with Unicode characters.
import os, glob, time, shutil
import sys
import unicodedata

import unittest
from test.support.os_helper import (rmtree, change_cwd, TESTFN_UNICODE,
    TESTFN_UNENCODABLE, create_empty_file)


if not os.path.supports_unicode_filenames:
    try:
        TESTFN_UNICODE.encode(sys.getfilesystemencoding())
    except (UnicodeError, TypeError):
        # Either the file system encoding is None, or the file name
        # cannot be encoded in the file system encoding.
        raise unittest.SkipTest("No Unicode filesystem semantics on this platform.")

def remove_if_exists(filename):
    if os.path.exists(filename):
        os.unlink(filename)

class TestUnicodeFiles(unittest.TestCase):
    # The 'do_' functions are the actual tests.  They generally assume the
    # file already exists etc.

    # Do all the tests we can given only a single filename.  The file should
    # exist.
    def _do_single(self, filename):
        self.assertTrue(os.path.exists(filename))
        self.assertTrue(os.path.isfile(filename))
        self.assertTrue(os.access(filename, os.R_OK))
        self.assertTrue(os.path.exists(os.path.abspath(filename)))
        self.assertTrue(os.path.isfile(os.path.abspath(filename)))
        self.assertTrue(os.access(os.path.abspath(filename), os.R_OK))
        os.chmod(filename, 0o777)
        os.utime(filename, None)
        os.utime(filename, (time.time(), time.time()))
        # Copy/rename etc tests using the same filename
        self._do_copyish(filename, filename)
        # Filename should appear in glob output
        self.assertTrue(
            os.path.abspath(filename)==os.path.abspath(glob.glob(glob.escape(filename))[0]))
        # basename should appear in listdir.
        path, base = os.path.split(os.path.abspath(filename))
        file_list = os.listdir(path)
        # Normalize the unicode strings, as round-tripping the name via the OS
        # may return a different (but equivalent) value.
        base = unicodedata.normalize("NFD", base)
        file_list = [unicodedata.normalize("NFD", f) for f in file_list]

        self.assertIn(base, file_list)

    # Tests that copy, move, etc one file to another.
    def _do_copyish(self, filename1, filename2):
        # Should be able to rename the file using either name.
        self.assertTrue(os.path.isfile(filename1)) # must exist.
        os.rename(filename1, filename2 + ".new")
        self.assertFalse(os.path.isfile(filename2))
        self.assertTrue(os.path.isfile(filename1 + '.new'))
        os.rename(filename1 + ".new", filename2)
        self.assertFalse(os.path.isfile(filename1 + '.new'))
        self.assertTrue(os.path.isfile(filename2))

        shutil.copy(filename1, filename2 + ".new")
        os.unlink(filename1 + ".new") # remove using equiv name.
        # And a couple of moves, one using each name.
        shutil.move(filename1, filename2 + ".new")
        self.assertFalse(os.path.exists(filename2))
        self.assertTrue(os.path.exists(filename1 + '.new'))
        shutil.move(filename1 + ".new", filename2)
        self.assertFalse(os.path.exists(filename2 + '.new'))
        self.assertTrue(os.path.exists(filename1))
        # Note - due to the implementation of shutil.move,
        # it tries a rename first.  This only fails on Windows when on
        # different file systems - and this test can't ensure that.
        # So we test the shutil.copy2 function, which is the thing most
        # likely to fail.
        shutil.copy2(filename1, filename2 + ".new")
        self.assertTrue(os.path.isfile(filename1 + '.new'))
        os.unlink(filename1 + ".new")
        self.assertFalse(os.path.exists(filename2 + '.new'))

    def _do_directory(self, make_name, chdir_name):
        if os.path.isdir(make_name):
            rmtree(make_name)
        os.mkdir(make_name)
        try:
            with change_cwd(chdir_name):
                cwd_result = os.getcwd()
                name_result = make_name

                cwd_result = unicodedata.normalize("NFD", cwd_result)
                name_result = unicodedata.normalize("NFD", name_result)

                self.assertEqual(os.path.basename(cwd_result),name_result)
        finally:
            os.rmdir(make_name)

    # The '_test' functions 'entry points with params' - ie, what the
    # top-level 'test' functions would be if they could take params
    def _test_single(self, filename):
        remove_if_exists(filename)
        create_empty_file(filename)
        try:
            self._do_single(filename)
        finally:
            os.unlink(filename)
        self.assertTrue(not os.path.exists(filename))
        # and again with os.open.
        f = os.open(filename, os.O_CREAT | os.O_WRONLY)
        os.close(f)
        try:
            self._do_single(filename)
        finally:
            os.unlink(filename)

    # The 'test' functions are unittest entry points, and simply call our
    # _test functions with each of the filename combinations we wish to test
    def test_single_files(self):
        self._test_single(TESTFN_UNICODE)
        if TESTFN_UNENCODABLE is not None:
            self._test_single(TESTFN_UNENCODABLE)

    def test_directories(self):
        # For all 'equivalent' combinations:
        #  Make dir with encoded, chdir with unicode, checkdir with encoded
        #  (or unicode/encoded/unicode, etc
        ext = ".dir"
        self._do_directory(TESTFN_UNICODE+ext, TESTFN_UNICODE+ext)
        # Our directory name that can't use a non-unicode name.
        if TESTFN_UNENCODABLE is not None:
            self._do_directory(TESTFN_UNENCODABLE+ext,
                               TESTFN_UNENCODABLE+ext)


if __name__ == "__main__":
    unittest.main()
