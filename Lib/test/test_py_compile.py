import imp
import os
import py_compile
import shutil
import tempfile
import unittest

from test import test_support

class PyCompileTests(unittest.TestCase):

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        self.pyc_path = self.source_path + 'c'
        with open(self.source_path, 'w') as file:
            file.write('x = 123\n')

    def tearDown(self):
        shutil.rmtree(self.directory)

    def test_absolute_path(self):
        py_compile.compile(self.source_path, self.pyc_path)
        self.assertTrue(os.path.exists(self.pyc_path))

    def test_cwd(self):
        cwd = os.getcwd()
        os.chdir(self.directory)
        py_compile.compile(os.path.basename(self.source_path),
                           os.path.basename(self.pyc_path))
        os.chdir(cwd)
        self.assertTrue(os.path.exists(self.pyc_path))

    def test_relative_path(self):
        py_compile.compile(os.path.relpath(self.source_path),
                           os.path.relpath(self.pyc_path))
        self.assertTrue(os.path.exists(self.pyc_path))

def test_main():
    test_support.run_unittest(PyCompileTests)

if __name__ == "__main__":
    test_main()
