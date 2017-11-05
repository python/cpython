"""Test the interactive interpreter."""

import unittest
from textwrap import dedent
from test.support import run_pty, SuppressCrashReport

class TestInteractiveInterpreter(unittest.TestCase):

    def test_no_memory(self):
        # Issue #30696: Fix the interactive interpreter looping endlessly when
        # no memory. Check also that the fix does not break the interactive
        # loop when an exception is raised.
        user_input = """
            import sys, _testcapi
            1/0
            print('After the exception.')
            _testcapi.set_nomemory(0)
            sys.exit(0)
        """
        user_input = dedent(user_input)
        user_input = b'\r'.join(x.encode() for x in user_input.split('\n'))
        user_input += b'\r'
        with SuppressCrashReport():
            rc, output = run_pty(None, input=user_input)
        self.assertIn(b'After the exception.\r\n>>>', output)
        self.assertIn(rc, (1, 120))

if __name__ == "__main__":
    unittest.main()
