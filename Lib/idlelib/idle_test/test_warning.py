'''Test warnings replacement in pyshell.py and run.py.

This file could be expanded to include traceback overrides
(in same two modules). If so, change name.
Revise if output destination changes (http://bugs.python.org/issue18318).
Make sure warnings module is left unaltered (http://bugs.python.org/issue18081).
'''
import io
import sys
import unittest
import warnings
from contextlib import redirect_stdout
from test.support import captured_stderr
from types import ModuleType

from idlelib import pyshell as shell
from idlelib import run


# Try to capture default showwarning before Idle modules are imported.
showwarning = warnings.showwarning
# But if we run this file within idle, we are in the middle of the run.main loop
# and default showwarnings has already been replaced.
running_in_idle = 'idle' in showwarning.__name__

# The following was generated from pyshell.idle_formatwarning
# and checked as matching expectation.
idlemsg = '''
Warning (from warnings module):
  File "test_warning.py", line 99
    Line of code
UserWarning: Test
'''
shellmsg = idlemsg + ">>> "


class RunWarnTest(unittest.TestCase):

    @unittest.skipIf(running_in_idle, "Does not work when run within Idle.")
    def test_showwarnings(self):
        self.assertIs(warnings.showwarning, showwarning)
        run.capture_warnings(True)
        self.assertIs(warnings.showwarning, run.idle_showwarning_subproc)
        run.capture_warnings(False)
        self.assertIs(warnings.showwarning, showwarning)

    def test_run_show(self):
        with captured_stderr() as f:
            run.idle_showwarning_subproc(
                    'Test', UserWarning, 'test_warning.py', 99, f, 'Line of code')
            # The following uses .splitlines to erase line-ending differences
            self.assertEqual(idlemsg.splitlines(), f.getvalue().splitlines())


class ShellWarnTest(unittest.TestCase):

    @unittest.skipIf(running_in_idle, "Does not work when run within Idle.")
    def test_showwarnings(self):
        self.assertIs(warnings.showwarning, showwarning)
        shell.capture_warnings(True)
        self.assertIs(warnings.showwarning, shell.idle_showwarning)
        shell.capture_warnings(False)
        self.assertIs(warnings.showwarning, showwarning)

    def test_idle_formatter(self):
        # Will fail if format changed without regenerating idlemsg
        s = shell.idle_formatwarning(
                'Test', UserWarning, 'test_warning.py', 99, 'Line of code')
        self.assertEqual(idlemsg, s)

    def test_shell_show(self):
        with captured_stderr() as f:
            shell.idle_showwarning(
                    'Test', UserWarning, 'test_warning.py', 99, f, 'Line of code')
            self.assertEqual(shellmsg.splitlines(), f.getvalue().splitlines())


class TestDeprecatedHelp(unittest.TestCase):
    CODE_SIMPLE = r"""
from warnings import deprecated
@deprecated("Test")
class A:
    pass
a = A()
"""
    CODE_SUBCLASS = r"""
from warnings import deprecated
@deprecated("Test")
class A:
    pass

class B(A):
    pass
b = B()
"""
    def setUp(self):
        self.module = ModuleType("testmodule")

    def tearDown(self):
        if "testmodule" in sys.modules:
            del sys.modules["testmodule"]

    def _get_help_output(self, code):
        with self.assertWarns(DeprecationWarning) as cm:
            exec(code, self.module.__dict__)
            sys.modules["testmodule"] = self.module

            f = io.StringIO()
            with redirect_stdout(f):
                help(self.module)

        self.assertEqual(str(cm.warning), "Test")
        return f.getvalue()

    def test_help_output(self):
        for code in (self.CODE_SIMPLE, self.CODE_SUBCLASS):
            with self.subTest(code=code):
                help_output = self._get_help_output(code)
                self.assertIn("Help on module testmodule:", help_output)


if __name__ == '__main__':
    unittest.main(verbosity=2)
