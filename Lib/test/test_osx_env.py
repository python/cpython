"""
Test suite for OS X interpreter environment variables.
"""

from test.support import EnvironmentVarGuard, run_unittest
import subprocess
import sys
import unittest

class OSXEnvironmentVariableTestCase(unittest.TestCase):
    def _check_sys(self, ev, cond, sv, val = sys.executable + 'dummy'):
        with EnvironmentVarGuard() as evg:
            subpc = [str(sys.executable), '-c',
                'import sys; sys.exit(2 if "%s" %s %s else 3)' % (val, cond, sv)]
            # ensure environment variable does not exist
            evg.unset(ev)
            # test that test on sys.xxx normally fails
            rc = subprocess.call(subpc)
            self.assertEqual(rc, 3, "expected %s not %s %s" % (ev, cond, sv))
            # set environ variable
            evg.set(ev, val)
            # test that sys.xxx has been influenced by the environ value
            rc = subprocess.call(subpc)
            self.assertEqual(rc, 2, "expected %s %s %s" % (ev, cond, sv))

    def test_pythonexecutable_sets_sys_executable(self):
        self._check_sys('PYTHONEXECUTABLE', '==', 'sys.executable')

def test_main():
    from distutils import sysconfig

    if sys.platform == 'darwin' and sysconfig.get_config_var('WITH_NEXT_FRAMEWORK'):
        run_unittest(OSXEnvironmentVariableTestCase)

if __name__ == "__main__":
    test_main()
