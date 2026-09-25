# gh-116869: Build a C/C++ test extension to check that the Python C API does
# not emit compiler warnings.
#
# The Python C API must be compatible with building
# with the -Werror=declaration-after-statement compiler flag.

import os.path
import platform
import shlex
import shutil
import subprocess
import sys
import sysconfig
import unittest
from test import support
from test.support import os_helper


if not support.has_subprocess_support:
    raise unittest.SkipTest("requires subprocess support")


SOURCE_DIR = os.path.dirname(__file__)
SOURCES = [
    os.path.join(SOURCE_DIR, 'extension.c'),
    os.path.join(SOURCE_DIR, 'extension.cpp'),
    os.path.join(SOURCE_DIR, 'setup.py'),
]
MSVC = support.MS_WINDOWS


# With MSVC on a debug build, the linker fails with: cannot open file
# 'python311.lib', it should look 'python311_d.lib'.
@unittest.skipIf(MSVC and support.Py_DEBUG,
                 'test fails on Windows debug build')
# Building and running an extension in clang sanitizing mode is not
# straightforward
@support.skip_if_sanitizer('test does not work with analyzing builds',
                           address=True, memory=True, ub=True, thread=True)
# the test uses venv+pip: skip if it's not available
@support.requires_venv_with_pip()
@support.requires_subprocess()
@support.requires_resource('cpu')
class BaseTests:
    TEST_INTERNAL_C_API = False
    LANGUAGE = None

    def check_build(self, extension_name, std=None, limited=False,
                    abi3t=False, extra_cflags=None):
        if self.LANGUAGE == 'C++' and not std and sys.platform == 'darwin':
            # Old Apple clang++ default C++ std is gnu++98, use C++11 instead
            std = 'c++11'

        pkg_dir = 'pkg'
        os.mkdir(pkg_dir)
        self.addCleanup(os_helper.rmtree, pkg_dir)

        for source in SOURCES:
            dest = os.path.join(pkg_dir, os.path.basename(source))
            shutil.copy(source, dest)

        def run_cmd(operation, cmd):
            env = os.environ.copy()
            env['CPYTHON_TEST_EXT_NAME'] = extension_name
            env['CPYTHON_TEST_LANG'] = self.LANGUAGE
            if std:
                env['CPYTHON_TEST_STD'] = std
            if limited:
                env['CPYTHON_TEST_LIMITED'] = '1'
            if abi3t:
                env['CPYTHON_TEST_ABI3T'] = '1'
            if MSVC and sysconfig.is_python_build():
                env['CPYTHON_TEST_EXTRA_INCDIRS'] = os.path.split(sysconfig.get_config_h_filename())[0]
                env['CPYTHON_TEST_EXTRA_LIBDIRS'] = os.path.split(sys.executable)[0]
            env['CPYTHON_TEST_INTERNAL_C_API'] = str(int(self.TEST_INTERNAL_C_API))
            if extra_cflags:
                env['CPYTHON_TEST_EXTRA_CFLAGS'] = extra_cflags
            if support.verbose:
                print('Run:', ' '.join(map(shlex.quote, cmd)))
                subprocess.run(cmd, check=True, env=env)
            else:
                proc = subprocess.run(cmd,
                                      env=env,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT,
                                      text=True)
                if proc.returncode:
                    print('Run:', ' '.join(map(shlex.quote, cmd)))
                    print(proc.stdout, end='')
                    self.fail(
                        f"{operation} failed with exit code {proc.returncode}")

        # Build and install the C extension
        python_exe = PYTHON_EXE
        cmd = [python_exe, '-X', 'dev',
               '-m', 'pip', 'install', '--no-build-isolation',
               os.path.abspath(pkg_dir)]
        if support.verbose:
            cmd.append('-v')
        run_cmd('Install', cmd)

        # Do a reference run. Until we test that running python
        # doesn't leak references (gh-94755), run it so one can manually check
        # -X showrefcount results against this baseline.
        cmd = [python_exe,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', 'pass']
        run_cmd('Reference run', cmd)

        # Import the C/C++ extension
        cmd = [python_exe,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', f"import {extension_name}"]
        run_cmd('Import', cmd)


class TestPublicC(BaseTests, unittest.TestCase):
    LANGUAGE = 'C'

    # Default build with no options
    def test_build(self):
        self.check_build('_test_cext')

    @unittest.skipIf(MSVC, "MSVC doesn't support /std:c99")
    def test_build_c99(self):
        # In public docs, we say C API is compatible with C11. However,
        # in practice we do maintain C99 compatibility in public headers.
        # Please ask the C API WG before adding a new C11-only feature.
        self.check_build('_test_cext_c99', std='c99')

    def test_build_c11(self):
        self.check_build('_test_cext_c11', std='c11')

    def test_build_limited(self):
        self.check_build('_test_cext_limited', limited=True)

    def test_build_limited_c11(self):
        self.check_build('_test_cext_limited_c11', limited=True, std='c11')

    def test_build_abi3t(self):
        # Test with Py_TARGET_ABI3T
        self.check_build('_test_cext_abi3t', abi3t=True)


class TestPublicCpp(BaseTests, unittest.TestCase):
    LANGUAGE = 'C++'

    def test_build(self):
        self.check_build('_test_cppext')

    def test_build_cpp03(self):
        # In public docs, we say C API is compatible with C++11. However,
        # in practice we do maintain C++03 compatibility in public headers.
        # Please ask the C API WG before adding a new C++11-only feature.
        self.check_build('_test_cppext_cpp03', std='c++03')

    @unittest.skipIf(MSVC, "MSVC doesn't support /std:c++11")
    def test_build_cpp11(self):
        self.check_build('_test_cppext_cpp11', std='c++11')

    # Only test C++14 on MSVC.
    # On s390x RHEL7, GCC 4.8.5 doesn't support C++14.
    @unittest.skipIf(not MSVC, "need MSVC")
    def test_build_cpp14(self):
        self.check_build('_test_cppext_cpp14', std='c++14')

    # Test that headers compile with Intel asm syntax, which may conflict
    # with inline assembly in free-threading headers that use AT&T syntax.
    @unittest.skipIf(MSVC, "MSVC doesn't support -masm=intel")
    @unittest.skipUnless(platform.machine() in ('x86_64', 'i686', 'AMD64'),
                         "x86-specific flag")
    def test_build_intel_asm(self):
        self.check_build('_test_cppext_intel_asm', extra_cflags='-masm=intel')

    def test_build_limited(self):
        self.check_build('_test_cppext_limited', limited=True)

    def test_build_limited_cpp03(self):
        self.check_build('_test_cppext_limited_cpp03', std='c++03', limited=True)

    def test_build_abi3t(self):
        # Test with Py_TARGET_ABI3T
        self.check_build('_test_cppext_abi3t', abi3t=True)


class TestInteralC(BaseTests, unittest.TestCase):
    LANGUAGE = 'C'
    TEST_INTERNAL_C_API = True

    # Default build with no options
    def test_build(self):
        self.check_build('_test_cext_internal')


class TestInteralCpp(BaseTests, unittest.TestCase):
    LANGUAGE = 'C++'
    TEST_INTERNAL_C_API = True

    def test_build(self):
        self.check_build('_test_cppext_internal')


def setUpModule():
    global VENV_CONTEXT, PYTHON_EXE
    VENV_CONTEXT = support.setup_venv_with_pip_setuptools('env')
    PYTHON_EXE = VENV_CONTEXT.__enter__()


def tearDownModule():
    VENV_CONTEXT.__exit__(None, None, None)


if __name__ == "__main__":
    unittest.main()
