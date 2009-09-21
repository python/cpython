"""Tests for distutils.ccompiler."""
import os
import unittest
from test.support import captured_stdout

from distutils.ccompiler import gen_lib_options, CCompiler
from distutils import debug

class FakeCompiler(object):
    def library_dir_option(self, dir):
        return "-L" + dir

    def runtime_library_dir_option(self, dir):
        return ["-cool", "-R" + dir]

    def find_library_file(self, dirs, lib, debug=0):
        return 'found'

    def library_option(self, lib):
        return "-l" + lib

class CCompilerTestCase(unittest.TestCase):

    def test_gen_lib_options(self):
        compiler = FakeCompiler()
        libdirs = ['lib1', 'lib2']
        runlibdirs = ['runlib1']
        libs = [os.path.join('dir', 'name'), 'name2']

        opts = gen_lib_options(compiler, libdirs, runlibdirs, libs)
        wanted = ['-Llib1', '-Llib2', '-cool', '-Rrunlib1', 'found',
                  '-lname2']
        self.assertEquals(opts, wanted)

    def test_debug_print(self):

        class MyCCompiler(CCompiler):
            executables = {}

        compiler = MyCCompiler()
        with captured_stdout() as stdout:
            compiler.debug_print('xxx')
        stdout.seek(0)
        self.assertEquals(stdout.read(), '')

        debug.DEBUG = True
        try:
            with captured_stdout() as stdout:
                compiler.debug_print('xxx')
            stdout.seek(0)
            self.assertEquals(stdout.read(), 'xxx\n')
        finally:
            debug.DEBUG = False

def test_suite():
    return unittest.makeSuite(CCompilerTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
