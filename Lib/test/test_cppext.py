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
    # the test uses venv+pip: skip if it's not available
    @support.requires_venv_with_pip()
    def test_build(self):
        # Build in a temporary directory
        with os_helper.temp_cwd(), os_helper.EnvironmentVarGuard() as env:
            # Recent setuptools versions have a bug that prevents being
            # used to compile from a source build virtual environment.
            # [BUG](https://github.com/pypa/setuptools/issues/3325)
            # The stdlib distutils does not have this problem.
            env['SETUPTOOLS_USE_DISTUTILS'] = 'stdlib'
            self._test_build()

    def _test_build(self):
        venv_dir = 'env'

        # Create virtual environment to get setuptools
        cmd = [sys.executable, '-X', 'dev', '-m', 'venv', venv_dir]
        if support.verbose:
            print()
            print('Run:', ' '.join(cmd))
        subprocess.run(cmd, check=True)

        # Get the Python executable of the venv
        python_exe = os.path.basename(sys.executable)
        if MS_WINDOWS:
            python = os.path.join(venv_dir, 'Scripts', python_exe)
        else:
            python = os.path.join(venv_dir, 'bin', python_exe)

        # Build the C++ extension
        cmd = [python, '-X', 'dev', SETUP_TESTCPPEXT, 'build_ext', '--verbose']
        if support.verbose:
            print('Run:', ' '.join(cmd))
        proc = subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              text=True)
        if proc.returncode:
            print(proc.stdout, end='')
            self.fail(f"Build failed with exit code {proc.returncode}")
        elif support.verbose:
            print(proc.stdout, end='')

if __name__ == "__main__":
    unittest.main()
