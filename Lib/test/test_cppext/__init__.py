# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
import os.path
import shlex
import shutil
import subprocess
import sys
import unittest
from test import support


SOURCE = os.path.join(os.path.dirname(__file__), 'extension.cpp')
SETUP = os.path.join(os.path.dirname(__file__), 'setup.py')


# With MSVC on a debug build, the linker fails with: cannot open file
# 'python311.lib', it should look 'python311_d.lib'.
@unittest.skipIf(support.MS_WINDOWS and support.Py_DEBUG,
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

    def check_build(self, extension_name, std=None, limited=False):
        venv_dir = 'env'
        with support.setup_venv_with_pip_setuptools(venv_dir) as python_exe:
            self._check_build(extension_name, python_exe,
                              std=std, limited=limited)

    def _check_build(self, extension_name, python_exe, std, limited):
        pkg_dir = 'pkg'
        os.mkdir(pkg_dir)
        shutil.copy(SETUP, os.path.join(pkg_dir, os.path.basename(SETUP)))
        shutil.copy(SOURCE, os.path.join(pkg_dir, os.path.basename(SOURCE)))

        def run_cmd(operation, cmd):
            env = os.environ.copy()
            if std:
                env['CPYTHON_TEST_CPP_STD'] = std
            if limited:
                env['CPYTHON_TEST_LIMITED'] = '1'
            env['CPYTHON_TEST_EXT_NAME'] = extension_name
            env['TEST_INTERNAL_C_API'] = str(int(self.TEST_INTERNAL_C_API))
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

        # Build and install the C++ extension
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

        # Import the C++ extension
        cmd = [python_exe,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', f"import {extension_name}"]
        run_cmd('Import', cmd)


class TestPublicCAPI(BaseTests, unittest.TestCase):
    def test_build(self):
        self.check_build('_testcppext')

    @support.requires_gil_enabled('incompatible with Free Threading')
    def test_build_limited_cpp03(self):
        self.check_build('_test_limited_cpp03ext', std='c++03', limited=True)

    @support.requires_gil_enabled('incompatible with Free Threading')
    def test_build_limited(self):
        self.check_build('_testcppext_limited', limited=True)

    def test_build_cpp03(self):
        # In public docs, we say C API is compatible with C++11. However,
        # in practice we do maintain C++03 compatibility in public headers.
        # Please ask the C API WG before adding a new C++11-only feature.
        self.check_build('_testcpp03ext', std='c++03')

    @unittest.skipIf(support.MS_WINDOWS, "MSVC doesn't support /std:c++11")
    def test_build_cpp11(self):
        self.check_build('_testcpp11ext', std='c++11')

    # Only test C++14 on MSVC.
    # On s390x RHEL7, GCC 4.8.5 doesn't support C++14.
    @unittest.skipIf(not support.MS_WINDOWS, "need Windows")
    def test_build_cpp14(self):
        self.check_build('_testcpp14ext', std='c++14')


class TestInteralCAPI(BaseTests, unittest.TestCase):
    TEST_INTERNAL_C_API = True

    def test_build(self):
        kwargs = {}
        if sys.platform == 'darwin':
            # Old Apple clang++ default C++ std is gnu++98
            kwargs['std'] = 'c++11'
        self.check_build('_testcppext_internal', **kwargs)


if __name__ == "__main__":
    unittest.main()
