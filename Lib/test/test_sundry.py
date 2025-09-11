"""Do a minimal test of all the modules that aren't otherwise tested."""
import importlib
from test import support
from test.support import import_helper
from test.support import warnings_helper
import unittest
import sys

class TestUntestedModules(unittest.TestCase):
    def test_untested_modules_can_be_imported(self):
        untested = ('encodings',)
        with warnings_helper.check_warnings(quiet=True):
            for name in untested:
                try:
                    import_helper.import_module('test.test_{}'.format(name))
                except unittest.SkipTest:
                    importlib.import_module(name)
                else:
                    self.fail('{} has tests even though test_sundry claims '
                              'otherwise'.format(name))

            import html.entities

            try:
                import tty  # Not available on Windows
            except ImportError:
                if support.verbose:
                    print("skipping tty")

    def test_distutils_modules(self):
        with warnings_helper.check_warnings(quiet=True):

            path_copy = sys.path[:]
            import distutils
            if '_distutils_hack' in sys.modules:
                # gh-135374: when 'setuptools' is installed, it now replaces
                # 'distutils' with its own version.
                # This imports '_distutils_hack' and modifies sys.path.
                # The setuptols version of distutils also does not include some
                # of the modules tested here.

                # Undo the path modifications and skip the test.

                sys.path[:] = path_copy
                raise unittest.SkipTest(
                    'setuptools has replaced distutils with its own version')

            import distutils.bcppcompiler
            import distutils.ccompiler
            import distutils.cygwinccompiler
            import distutils.filelist
            import distutils.text_file
            import distutils.unixccompiler

            import distutils.command.bdist_dumb
            import distutils.command.bdist
            import distutils.command.bdist_rpm
            import distutils.command.build_clib
            import distutils.command.build_ext
            import distutils.command.build
            import distutils.command.clean
            import distutils.command.config
            import distutils.command.install_data
            import distutils.command.install_egg_info
            import distutils.command.install_headers
            import distutils.command.install_lib
            import distutils.command.register
            import distutils.command.sdist
            import distutils.command.upload



if __name__ == "__main__":
    unittest.main()
