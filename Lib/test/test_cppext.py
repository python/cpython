# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
import os.path
try:
    import ssl
except ImportError:
    ssl = None
import sys
import unittest
import subprocess
import sysconfig
from test import support
from test.support import os_helper


MS_WINDOWS = (sys.platform == 'win32')


PKG_CPPEXTDATA = os.path.abspath(os.path.dirname(
    support.findfile('setup.py', subdir="cppextdata")
))


@support.requires_subprocess()
class TestCPPExt(unittest.TestCase):
    def test_build_cpp11(self):
        self.check_build(False, '_testcpp11ext')

    def test_build_cpp03(self):
        self.check_build(True, '_testcpp03ext')

    # With MSVC, the linker fails with: cannot open file 'python311.lib'
    # https://github.com/python/cpython/pull/32175#issuecomment-1111175897
    @unittest.skipIf(MS_WINDOWS, 'test fails on Windows')
    # Building and running an extension in clang sanitizing mode is not
    # straightforward
    @unittest.skipIf(
        '-fsanitize' in (sysconfig.get_config_var('PY_CFLAGS') or ''),
        'test does not work with analyzing builds')
    # the test uses pip which needs a TLS connection to PyPI
    @unittest.skipIf(ssl is None, 'No ssl module')
    # the test uses venv+pip: skip if it's not available
    @support.requires_venv_with_pip()
    def check_build(self, std_cpp03, extension_name):
        # Build in a temporary directory
        with os_helper.temp_cwd():
            self._check_build(std_cpp03, extension_name)

    def _check_build(self, std_cpp03, extension_name):
        venv_dir = 'env'
        verbose = support.verbose

        # Create virtual environment to get setuptools
        cmd = [sys.executable, '-X', 'dev', '-m', 'venv', venv_dir]
        if verbose:
            print()
            print('Run:', ' '.join(cmd))
        subprocess.run(cmd, check=True)

        # Get the Python executable of the venv
        python_exe = 'python'
        if sys.executable.endswith('.exe'):
            python_exe += '.exe'
        if MS_WINDOWS:
            python = os.path.join(venv_dir, 'Scripts', python_exe)
        else:
            python = os.path.join(venv_dir, 'bin', python_exe)

        def run_cmd(operation, cmd):
            env = os.environ.copy()
            env['CPYTHON_TEST_CPP_STD'] = 'c++03' if std_cpp03 else 'c++11'
            env['CPYTHON_TEST_EXT_NAME'] = extension_name
            if verbose:
                print('Run:', ' '.join(cmd))
                subprocess.run(cmd, check=True, env=env)
            else:
                proc = subprocess.run(cmd,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT,
                                      text=True,
                                      env=env)
                if proc.returncode:
                    print(proc.stdout, end='')
                    self.fail(
                        f"{operation} failed with exit code {proc.returncode}")

        # Build and install the C++ extension
        cmd = [python, '-X', 'dev',
               '-m', 'pip', 'install', PKG_CPPEXTDATA]
        run_cmd('Install', cmd)

        # Do a reference run. Until we test that running python
        # doesn't leak references (gh-94755), run it so one can manually check
        # -X showrefcount results against this baseline.
        cmd = [python,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', 'pass']
        run_cmd('Reference run', cmd)

        # Import the C++ extension
        cmd = [python,
               '-X', 'dev',
               '-X', 'showrefcount',
               '-c', f"import {extension_name}"]
        run_cmd('Import', cmd)


if __name__ == "__main__":
    unittest.main()
