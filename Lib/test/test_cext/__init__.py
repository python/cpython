# gh-116869: Build a basic C test extension to check that the Python C API
# does not emit C compiler warnings.

import os.path
import shutil
import subprocess
import unittest
from test import support


SOURCE = os.path.join(os.path.dirname(__file__), 'extension.c')
SETUP = os.path.join(os.path.dirname(__file__), 'setup.py')


# With MSVC, the linker fails with: cannot open file 'python311.lib'
# https://github.com/python/cpython/pull/32175#issuecomment-1111175897
@unittest.skipIf(support.MS_WINDOWS, 'test fails on Windows')
# Building and running an extension in clang sanitizing mode is not
# straightforward
@support.skip_if_sanitizer('test does not work with analyzing builds',
                           address=True, memory=True, ub=True, thread=True)
# the test uses venv+pip: skip if it's not available
@support.requires_venv_with_pip()
@support.requires_subprocess()
@support.requires_resource('cpu')
class TestExt(unittest.TestCase):
    def test_build_c99(self):
        self.check_build('c99', '_test_c99_ext')

    def test_build_c11(self):
        self.check_build('c11', '_test_c11_ext')

    def check_build(self, clang_std, extension_name):
        venv_dir = 'env'
        with support.setup_venv_with_pip_setuptools_wheel(venv_dir) as python_exe:
            self._check_build(clang_std, extension_name, python_exe)

    def _check_build(self, clang_std, extension_name, python_exe):
        pkg_dir = 'pkg'
        os.mkdir(pkg_dir)
        shutil.copy(SETUP, os.path.join(pkg_dir, os.path.basename(SETUP)))
        shutil.copy(SOURCE, os.path.join(pkg_dir, os.path.basename(SOURCE)))

        def run_cmd(operation, cmd):
            env = os.environ.copy()
            env['CPYTHON_TEST_STD'] = clang_std
            env['CPYTHON_TEST_EXT_NAME'] = extension_name
            if support.verbose:
                print('Run:', ' '.join(cmd))
                subprocess.run(cmd, check=True, env=env)
            else:
                proc = subprocess.run(cmd,
                                      env=env,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT,
                                      text=True)
                if proc.returncode:
                    print(proc.stdout, end='')
                    self.fail(
                        f"{operation} failed with exit code {proc.returncode}")

        # Build and install the C extension
        cmd = [python_exe, '-X', 'dev',
               '-m', 'pip', 'install', '--no-build-isolation',
               os.path.abspath(pkg_dir)]
        run_cmd('Install', cmd)

        # Do a reference run. Until we test that running python
        # doesn't leak references (gh-94755), run it so one can manually check
        # -X showrefcount results against this baseline.
        cmd = [python_exe,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', 'pass']
        run_cmd('Reference run', cmd)

        # Import the C extension
        cmd = [python_exe,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', f"import {extension_name}"]
        run_cmd('Import', cmd)


if __name__ == "__main__":
    unittest.main()
