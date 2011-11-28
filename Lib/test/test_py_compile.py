import imp
import os
import py_compile
import shutil
import tempfile
import unittest

from test import support, script_helper

class PyCompileTests(unittest.TestCase):

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        self.pyc_path = self.source_path + 'c'
        self.cache_path = imp.cache_from_source(self.source_path)
        self.cwd_drive = os.path.splitdrive(os.getcwd())[0]
        # In these tests we compute relative paths.  When using Windows, the
        # current working directory path and the 'self.source_path' might be
        # on different drives.  Therefore we need to switch to the drive where
        # the temporary source file lives.
        drive = os.path.splitdrive(self.source_path)[0]
        if drive:
            os.chdir(drive)
        with open(self.source_path, 'w') as file:
            file.write('x = 123\n')

    def tearDown(self):
        shutil.rmtree(self.directory)
        if self.cwd_drive:
            os.chdir(self.cwd_drive)

    def test_absolute_path(self):
        py_compile.compile(self.source_path, self.pyc_path)
        self.assertTrue(os.path.exists(self.pyc_path))
        self.assertFalse(os.path.exists(self.cache_path))

    def test_cache_path(self):
        py_compile.compile(self.source_path)
        self.assertTrue(os.path.exists(self.cache_path))

    def test_cwd(self):
        cwd = os.getcwd()
        os.chdir(self.directory)
        py_compile.compile(os.path.basename(self.source_path),
                           os.path.basename(self.pyc_path))
        os.chdir(cwd)
        self.assertTrue(os.path.exists(self.pyc_path))
        self.assertFalse(os.path.exists(self.cache_path))

    def test_relative_path(self):
        py_compile.compile(os.path.relpath(self.source_path),
                           os.path.relpath(self.pyc_path))
        self.assertTrue(os.path.exists(self.pyc_path))
        self.assertFalse(os.path.exists(self.cache_path))

def test_main():
    support.run_unittest(PyCompileTests)

if __name__ == "__main__":
    test_main()
