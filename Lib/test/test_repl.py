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
            _testcapi.set_nomemory(0)
            sys.exit(0)
        """
        user_input = dedent(user_input)
        user_input = b'\r'.join(x.encode() for x in user_input.split('\n'))
        user_input += b'\r'
        with SuppressCrashReport():
            output = run_pty(None, input=user_input)
        self.assertIn(b"Fatal Python error: Cannot recover from MemoryErrors",
                      output)

if __name__ == "__main__":
    unittest.main()
