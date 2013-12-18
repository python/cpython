"""Tests for distutils.ccompiler."""
import os
import unittest
from test.test_support import captured_stdout

from distutils.ccompiler import (gen_lib_options, CCompiler,
                                 get_default_compiler)
from distutils.sysconfig import customize_compiler
from distutils import debug
from distutils.tests import support

class FakeCompiler(object):
    def library_dir_option(self, dir):
        return "-L" + dir

    def runtime_library_dir_option(self, dir):
        return ["-cool", "-R" + dir]

    def find_library_file(self, dirs, lib, debug=0):
        return 'found'

    def library_option(self, lib):
        return "-l" + lib

class CCompilerTestCase(support.EnvironGuard, unittest.TestCase):

    def test_gen_lib_options(self):
        compiler = FakeCompiler()
        libdirs = ['lib1', 'lib2']
        runlibdirs = ['runlib1']
        libs = [os.path.join('dir', 'name'), 'name2']

        opts = gen_lib_options(compiler, libdirs, runlibdirs, libs)
        wanted = ['-Llib1', '-Llib2', '-cool', '-Rrunlib1', 'found',
                  '-lname2']
        self.assertEqual(opts, wanted)

    def test_debug_print(self):

        class MyCCompiler(CCompiler):
            executables = {}

        compiler = MyCCompiler()
        with captured_stdout() as stdout:
            compiler.debug_print('xxx')
        stdout.seek(0)
        self.assertEqual(stdout.read(), '')

        debug.DEBUG = True
        try:
            with captured_stdout() as stdout:
                compiler.debug_print('xxx')
            stdout.seek(0)
            self.assertEqual(stdout.read(), 'xxx\n')
        finally:
            debug.DEBUG = False

    @unittest.skipUnless(get_default_compiler() == 'unix',
                         'not testing if default compiler is not unix')
    def test_customize_compiler(self):
        os.environ['AR'] = 'my_ar'
        os.environ['ARFLAGS'] = '-arflags'

        # make sure AR gets caught
        class compiler:
            compiler_type = 'unix'

            def set_executables(self, **kw):
                self.exes = kw

        comp = compiler()
        customize_compiler(comp)
        self.assertEqual(comp.exes['archiver'], 'my_ar -arflags')

def test_suite():
    return unittest.makeSuite(CCompilerTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
