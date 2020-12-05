"""Do a minimal test of all the modules that aren't otherwise tested."""
import importlib
import platform
import sys
from test import support
from test.support import import_helper
from test.support import warnings_helper
import unittest

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

            import distutils.bcppcompiler
            import distutils.ccompiler
            import distutils.cygwinccompiler
            import distutils.filelist
            import distutils.text_file
            import distutils.unixccompiler

            import distutils.command.bdist_dumb
            if sys.platform.startswith('win') and not platform.win32_is_iot():
                import distutils.command.bdist_msi
            import distutils.command.bdist
            import distutils.command.bdist_rpm
            import distutils.command.bdist_wininst
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

            import html.entities

            try:
                import tty  # Not available on Windows
            except ImportError:
                if support.verbose:
                    print("skipping tty")


if __name__ == "__main__":
    unittest.main()
