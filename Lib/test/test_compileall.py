import compileall
import imp
import os
import py_compile
import shutil
import struct
import tempfile
from test import test_support
import unittest


class CompileallTests(unittest.TestCase):

    def setUp(self):
        self.directory = tempfile.mkdtemp()
        self.source_path = os.path.join(self.directory, '_test.py')
        self.bc_path = self.source_path + ('c' if __debug__ else 'o')
        with open(self.source_path, 'w') as file:
            file.write('x = 123\n')
        self.source_path2 = os.path.join(self.directory, '_test2.py')
        self.bc_path2 = self.source_path2 + ('c' if __debug__ else 'o')
        shutil.copyfile(self.source_path, self.source_path2)

    def tearDown(self):
        shutil.rmtree(self.directory)

    def data(self):
        with open(self.bc_path, 'rb') as file:
            data = file.read(8)
        mtime = int(os.stat(self.source_path).st_mtime)
        compare = struct.pack('<4sl', imp.get_magic(), mtime)
        return data, compare

    @unittest.skipUnless(hasattr(os, 'stat'), 'test needs os.stat()')
    def recreation_check(self, metadata):
        """Check that compileall recreates bytecode when the new metadata is
        used."""
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

    def test_compile_files(self):
        # Test compiling a single file, and complete directory
        for fn in (self.bc_path, self.bc_path2):
            try:
                os.unlink(fn)
            except:
                pass
        compileall.compile_file(self.source_path, force=False, quiet=True)
        self.assertTrue(os.path.isfile(self.bc_path) \
                        and not os.path.isfile(self.bc_path2))
        os.unlink(self.bc_path)
        compileall.compile_dir(self.directory, force=False, quiet=True)
        self.assertTrue(os.path.isfile(self.bc_path) \
                        and os.path.isfile(self.bc_path2))
        os.unlink(self.bc_path)
        os.unlink(self.bc_path2)

def test_main():
    test_support.run_unittest(CompileallTests)


if __name__ == "__main__":
    test_main()
