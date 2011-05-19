"""Tests for distutils.compiler."""
import os

from packaging.compiler import (get_default_compiler, customize_compiler,
                                gen_lib_options)
from packaging.tests import unittest, support


class FakeCompiler:

    name = 'fake'
    description = 'Fake'

    def library_dir_option(self, dir):
        return "-L" + dir

    def runtime_library_dir_option(self, dir):
        return ["-cool", "-R" + dir]

    def find_library_file(self, dirs, lib, debug=False):
        return 'found'

    def library_option(self, lib):
        return "-l" + lib


class CompilerTestCase(support.EnvironRestorer, unittest.TestCase):

    restore_environ = ['AR', 'ARFLAGS']

    @unittest.skipUnless(get_default_compiler() == 'unix',
                        'irrelevant if default compiler is not unix')
    def test_customize_compiler(self):

        os.environ['AR'] = 'my_ar'
        os.environ['ARFLAGS'] = '-arflags'

        # make sure AR gets caught
        class compiler:
            name = 'unix'

            def set_executables(self, **kw):
                self.exes = kw

        comp = compiler()
        customize_compiler(comp)
        self.assertEqual(comp.exes['archiver'], 'my_ar -arflags')

    def test_gen_lib_options(self):
        compiler = FakeCompiler()
        libdirs = ['lib1', 'lib2']
        runlibdirs = ['runlib1']
        libs = [os.path.join('dir', 'name'), 'name2']

        opts = gen_lib_options(compiler, libdirs, runlibdirs, libs)
        wanted = ['-Llib1', '-Llib2', '-cool', '-Rrunlib1', 'found',
                  '-lname2']
        self.assertEqual(opts, wanted)


def test_suite():
    return unittest.makeSuite(CompilerTestCase)


if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
