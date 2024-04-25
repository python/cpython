# gh-91321: Build a basic C++ test extension to check that the Python C API is
# compatible with C++ and does not emit C++ compiler warnings.
import os.path
import sys
import unittest
import subprocess
from test import support
from test.support import os_helper


MS_WINDOWS = (sys.platform == 'win32')


SETUP_TESTCPPEXT = support.findfile('setup_testcppext.py')


@support.requires_subprocess()
class TestCPPExt(unittest.TestCase):
    def test_build_cpp11(self):
        self.check_build(False)

    def test_build_cpp03(self):
        self.check_build(True)

    # With MSVC, the linker fails with: cannot open file 'python311.lib'
    # https://github.com/python/cpython/pull/32175#issuecomment-1111175897
    @unittest.skipIf(MS_WINDOWS, 'test fails on Windows')
    # the test uses venv+pip: skip if it's not available
    @support.requires_venv_with_pip()
    def check_build(self, std_cpp03):
        # Build in a temporary directory
        with os_helper.temp_cwd():
            self._check_build(std_cpp03)

    def _check_build(self, std_cpp03):
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

        # Build the C++ extension
        cmd = [python, '-X', 'dev',
               SETUP_TESTCPPEXT, 'build_ext', '--verbose']
        if std_cpp03:
            cmd.append('-std=c++03')
        if verbose:
            print('Run:', ' '.join(cmd))
            subprocess.run(cmd, check=True)
        else:
            proc = subprocess.run(cmd,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT,
                                  text=True)
            if proc.returncode:
                print(proc.stdout, end='')
                self.fail(f"Build failed with exit code {proc.returncode}")


if __name__ == "__main__":
    unittest.main()
