# gh-116869: Build a basic C test extension to check that the Python C API
# does not emit C compiler warnings.
#
# The Python C API must be compatible with building
# with the -Werror=declaration-after-statement compiler flag.

import os.path
import shlex
import shutil
import subprocess
import unittest
from test import support


SOURCE = os.path.join(os.path.dirname(__file__), 'extension.c')
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
class TestExt(unittest.TestCase):
    # Default build with no options
    def test_build(self):
        self.check_build('_test_cext')

    def test_build_c11(self):
        self.check_build('_test_c11_cext', std='c11')

    @unittest.skipIf(support.MS_WINDOWS, "MSVC doesn't support /std:c99")
    def test_build_c99(self):
        # In public docs, we say C API is compatible with C11. However,
        # in practice we do maintain C99 compatibility in public headers.
        # Please ask the C API WG before adding a new C11-only feature.
        self.check_build('_test_c99_cext', std='c99')

    @support.requires_gil_enabled('incompatible with Free Threading')
    def test_build_limited(self):
        self.check_build('_test_limited_cext', limited=True)

    @support.requires_gil_enabled('broken for now with Free Threading')
    def test_build_limited_c11(self):
        self.check_build('_test_limited_c11_cext', limited=True, std='c11')

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
                env['CPYTHON_TEST_STD'] = std
            if limited:
                env['CPYTHON_TEST_LIMITED'] = '1'
            env['CPYTHON_TEST_EXT_NAME'] = extension_name
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

        # Import the C extension
        cmd = [python_exe,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', f"import {extension_name}"]
        run_cmd('Import', cmd)


if __name__ == "__main__":
    unittest.main()
