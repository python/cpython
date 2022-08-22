import os.path
import sys
import unittest
import subprocess
import textwrap
from test import support
from test.support import os_helper


MS_WINDOWS = (sys.platform == 'win32')
SETUP = support.findfile('setup.py', subdir='test_clinic_functionality')
MOD_NAME = 'test_clinic_functionality'


def _run_cmd(operation, cmd, verbose=False):
    if verbose:
        print(f'{operation}:', ' '.join(cmd))
        subprocess.run(cmd, check=True)
    else:
        proc = subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              text=True)
        if proc.returncode:
            print(proc.stdout, end='')
            self.fail(f'{operation} failed with exit code {proc.returncode}')


class EnvInitializer:
    """Build the module before test and clear the files after test."""
    python = ''
    verbose = support.verbose

    def run_cmd(self, operation, cmd):
        _run_cmd(operation, cmd, self.verbose)

    def _setup_env(self):
        venv_dir = 'env'

        # Create virtual environment to get setuptools
        cmd = [sys.executable, '-X', 'dev', '-m', 'venv', venv_dir]
        self.run_cmd('Setup venv', cmd)

        # Get the Python executable of the venv
        python_exe = 'python'
        if sys.executable.endswith('.exe'):
            python_exe += '.exe'
        if MS_WINDOWS:
            self.python = os.path.join(venv_dir, 'Scripts', python_exe)
        else:
            self.python = os.path.join(venv_dir, 'bin', python_exe)

        # Build module
        cmd = [self.python, '-X', 'dev', SETUP, 'build_ext', '--verbose']
        self.run_cmd('Build', cmd)

        # Install module
        cmd = [self.python, '-X', 'dev', SETUP, 'install']
        self.run_cmd('Install', cmd)

    def setup_env(self):
        # Build in a temporary directory
        with os_helper.temp_cwd():
            self._setup_env()
            yield

    def __init__(self):
        if self.verbose:
            print()
            print('Setup test environment')
        self.env = self.setup_env()
        next(self.env)

    def __del__(self):
        if self.verbose:
            print('Clear test environment')
        try:
            next(self.env)
        except StopIteration:
            pass
        except Exception as e:
            raise e


env = None


def setUpModule():
    global env
    env = EnvInitializer()


def tearDownModule():
    global env
    del env


@support.requires_subprocess()
@support.requires_venv_with_pip()
class TestClinicFunctional(unittest.TestCase):
    verbose = support.verbose

    def run_cmd(self, operation, cmd):
        _run_cmd(operation, cmd, self.verbose)

    def exec_script(self, script):
        script = textwrap.dedent(script)
        global env
        python = env.python
        cmd = [python, '-c', script]
        self.run_cmd('Test', cmd)

    def assert_func_result(self, func_str, expected_result):
        script = f'''\
            import {MOD_NAME} as mod
            assert(mod.{func_str} == {expected_result})
        '''
        self.exec_script(script)

    def test_gh_32092_oob(self):
        self.assert_func_result('gh_32092_oob(1, 2, 3, 4, kw1=5, kw2=6)', '(1, 2, (3, 4), 5, 6))')

    def test_gh_32092_kw_pass(self):
        self.assert_func_result('gh_32092_kw_pass(1, 2, 3)', '(1, (2, 3))')


if __name__ == "__main__":
    unittest.main()
