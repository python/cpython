"""Test the interactive interpreter."""

import sys
import os
import unittest
import subprocess
from textwrap import dedent
from test.support import cpython_only, has_subprocess_support, SuppressCrashReport
from test.support.script_helper import kill_python


if not has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")


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
    return subprocess.Popen(cmd_line,
                            executable=sys.executable,
                            text=True,
                            stdin=subprocess.PIPE,
                            stdout=stdout, stderr=stderr,
                            **kw)

def run_on_interactive_mode(source):
    """Spawn a new Python interpreter, pass the given
    input source code from the stdin and return the
    result back. If the interpreter exits non-zero, it
    raises a ValueError."""

    process = spawn_repl()
    process.stdin.write(source)
    output = kill_python(process)

    if process.returncode != 0:
        raise ValueError("Process didn't exit properly.")
    return output


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
        p = spawn_repl()
        with SuppressCrashReport():
            p.stdin.write(user_input)
        output = kill_python(p)
        self.assertIn('After the exception.', output)
        # Exit code 120: Py_FinalizeEx() failed to flush stdout and stderr.
        self.assertIn(p.returncode, (1, 120))

    @cpython_only
    def test_multiline_string_parsing(self):
        # bpo-39209: Multiline string tokens need to be handled in the tokenizer
        # in two places: the interactive path and the non-interactive path.
        user_input = '''\
        x = """<?xml version="1.0" encoding="iso-8859-1"?>
        <test>
            <Users>
                <fun25>
                    <limits>
                        <total>0KiB</total>
                        <kbps>0</kbps>
                        <rps>1.3</rps>
                        <connections>0</connections>
                    </limits>
                    <usages>
                        <total>16738211KiB</total>
                        <kbps>237.15</kbps>
                        <rps>1.3</rps>
                        <connections>0</connections>
                    </usages>
                    <time_to_refresh>never</time_to_refresh>
                    <limit_exceeded_URL>none</limit_exceeded_URL>
                </fun25>
            </Users>
        </test>"""
        '''
        user_input = dedent(user_input)
        p = spawn_repl()
        p.stdin.write(user_input)
        output = kill_python(p)
        self.assertEqual(p.returncode, 0)

    def test_close_stdin(self):
        user_input = dedent('''
            import os
            print("before close")
            os.close(0)
        ''')
        prepare_repl = dedent('''
            from test.support import suppress_msvcrt_asserts
            suppress_msvcrt_asserts()
        ''')
        process = spawn_repl('-c', prepare_repl)
        output = process.communicate(user_input)[0]
        self.assertEqual(process.returncode, 0)
        self.assertIn('before close', output)


class TestInteractiveModeSyntaxErrors(unittest.TestCase):

    def test_interactive_syntax_error_correct_line(self):
        output = run_on_interactive_mode(dedent("""\
        def f():
            print(0)
            return yield 42
        """))

        traceback_lines = output.splitlines()[-4:-1]
        expected_lines = [
            '    return yield 42',
            '           ^^^^^',
            'SyntaxError: invalid syntax'
        ]
        self.assertEqual(traceback_lines, expected_lines)


if __name__ == "__main__":
    unittest.main()
