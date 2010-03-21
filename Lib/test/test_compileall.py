import sys
import compileall
import imp
import os
import py_compile
import shutil
import struct
import sys
import tempfile
import time
from test import support
import unittest
import io


class CompileallTests(unittest.TestCase):

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        self.bc_path = self.source_path + ('c' if __debug__ else 'o')
        with open(self.source_path, 'w') as file:
            file.write('x = 123\n')

    def tearDown(self):
        shutil.rmtree(self.directory)

    def data(self):
        with open(self.bc_path, 'rb') as file:
            data = file.read(8)
        mtime = int(os.stat(self.source_path).st_mtime)
        compare = struct.pack('<4sl', imp.get_magic(), mtime)
        return data, compare

    def recreation_check(self, metadata):
        """Check that compileall recreates bytecode when the new metadata is
        used."""
        if not hasattr(os, 'stat'):
            return
        py_compile.compile(self.source_path)
        self.assertEqual(*self.data())
        with open(self.bc_path, 'rb') as file:
            bc = file.read()[len(metadata):]
        with open(self.bc_path, 'wb') as file:
            file.write(metadata)
            file.write(bc)
        self.assertNotEqual(*self.data())
        compileall.compile_dir(self.directory, force=False, quiet=True)
        self.assertTrue(*self.data())

    def test_mtime(self):
        # Test a change in mtime leads to a new .pyc.
        self.recreation_check(struct.pack('<4sl', imp.get_magic(), 1))

    def test_magic_number(self):
        # Test a change in mtime leads to a new .pyc.
        self.recreation_check(b'\0\0\0\0')


class EncodingTest(unittest.TestCase):
    'Issue 6716: compileall should escape source code when printing errors to stdout.'

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        with open(self.source_path, 'w', encoding='utf-8') as file:
            file.write('# -*- coding: utf-8 -*-\n')
            file.write('print u"\u20ac"\n')

    def tearDown(self):
        shutil.rmtree(self.directory)

    def test_error(self):
        try:
            orig_stdout = sys.stdout
            sys.stdout = io.TextIOWrapper(io.BytesIO(),encoding='ascii')
            compileall.compile_dir(self.directory)
        finally:
            sys.stdout = orig_stdout

def test_main():
    support.run_unittest(CompileallTests,
                         EncodingTest)


if __name__ == "__main__":
    test_main()
