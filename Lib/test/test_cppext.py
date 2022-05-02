# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
import os
import sys
import unittest
import warnings
from test import support
from test.support import os_helper

with warnings.catch_warnings():
    warnings.simplefilter('ignore', DeprecationWarning)
    from distutils.core import setup, Extension
    import distutils.sysconfig


MS_WINDOWS = (sys.platform == 'win32')


SOURCE = support.findfile('_testcppext.cpp')
if not MS_WINDOWS:
    # C++ compiler flags for GCC and clang
    CPPFLAGS = [
        # Python currently targets C++11
        '-std=c++11',
        # gh-91321: The purpose of _testcppext extension is to check that building
        # a C++ extension using the Python C API does not emit C++ compiler
        # warnings
        '-Werror',
    ]
else:
    # Don't pass any compiler flag to MSVC
    CPPFLAGS = []


class TestCPPExt(unittest.TestCase):
    def build(self):
        cpp_ext = Extension(
            '_testcppext',
            sources=[SOURCE],
            language='c++',
            extra_compile_args=CPPFLAGS)

        try:
            try:
                with (support.captured_stdout() as stdout,
                      support.swap_attr(sys, 'argv', ['setup.py', 'build_ext'])):
                    setup(name="_testcppext", ext_modules=[cpp_ext])
                    return
            except:
                # Show output on error
                print()
                print(stdout.getvalue())
                raise
        except SystemExit:
            self.fail("Build failed")

    # With MSVC, the linker fails with: cannot open file 'python311.lib'
    # https://github.com/python/cpython/pull/32175#issuecomment-1111175897
    @unittest.skipIf(MS_WINDOWS, 'test fails on Windows')
    def test_build(self):
        # save/restore os.environ
        def restore_env(old_env):
            os.environ.clear()
            os.environ.update(old_env)
        self.addCleanup(restore_env, dict(os.environ))

        def restore_sysconfig_vars(old_config_vars):
            distutils.sysconfig._config_vars.clear()
            distutils.sysconfig._config_vars.update(old_config_vars)
        self.addCleanup(restore_sysconfig_vars,
                        dict(distutils.sysconfig._config_vars))

        # Build in a temporary directory
        with os_helper.temp_cwd():
            self.build()


if __name__ == "__main__":
    unittest.main()
