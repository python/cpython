"""Test the interactive interpreter."""

import sys
import os
import unittest
import subprocess
from textwrap import dedent
from test.support import cpython_only, SuppressCrashReport
from test.support.script_helper import kill_python

def spawn_repl(*args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kw):
    """Run the Python REPL with the given arguments.

    kw is extra keyword args to pass to subprocess.Popen. Returns a Popen
    object.
    """

    # To run the REPL without using a terminal, spawn python with the command
    # line option '-i' and the process name set to '<stdin>'.
    # The directory of argv[0] must match the directory of the Python
    # executable for the Popen() call to python to succeed as the directory
    # path may be used by Py_GetPath() to build the default module search
    # path.
    stdin_fname = os.path.join(os.path.dirname(sys.executable), "<stdin>")
    cmd_line = [stdin_fname, '-E', '-i']
    cmd_line.extend(args)

    # Set TERM=vt100, for the rationale see the comments in spawn_python() of
    # test.support.script_helper.
    env = kw.setdefault('env', dict(os.environ))
    env['TERM'] = 'vt100'
    return subprocess.Popen(cmd_line, executable=sys.executable,
                            stdin=subprocess.PIPE,
                            stdout=stdout, stderr=stderr,
                            **kw)

class TestInteractiveInterpreter(unittest.TestCase):

    @cpython_only
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
        user_input = user_input.encode()
        p = spawn_repl()
        with SuppressCrashReport():
            p.stdin.write(user_input)
        output = kill_python(p)
        self.assertIn(b'After the exception.', output)
        # Exit code 120: Py_FinalizeEx() failed to flush stdout and stderr.
        self.assertIn(p.returncode, (1, 120))

if __name__ == "__main__":
    unittest.main()
