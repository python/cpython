import doctest
import os
import pdbx
import subprocess
import sys
import textwrap
import unittest

from test import support
from test.support import os_helper
from test.test_doctest import _FakeInput

class PdbxTestInput(object):
    """Context manager that makes testing Pdb in doctests easier."""

    def __init__(self, input):
        self.input = input

    def __enter__(self):
        self.real_stdin = sys.stdin
        sys.stdin = _FakeInput(self.input)

    def __exit__(self, *exc):
        sys.stdin = self.real_stdin


def test_pdbx_basic_commands():
    """Test the basic commands of pdb.

    >>> def f(x):
    ...     x = x + 1
    ...     return x

    >>> def test_function():
    ...     import pdbx; pdbx.break_here()
    ...     for i in range(5):
    ...         n = f(i)
    ...         pass

    >>> with PdbxTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'return',
    ...     'next',
    ...     'n',
    ...     'continue',
    ... ]):
    ...     test_function()
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:3
    -> for i in range(5):
    (Pdbx) step
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:4
    -> n = f(i)
    (Pdbx) step
    > f() @ <doctest test.test_pdbx.test_pdbx_basic_commands[0]>:2
    -> x = x + 1
    (Pdbx) step
    > f() @ <doctest test.test_pdbx.test_pdbx_basic_commands[0]>:3
    -> return x
    (Pdbx) step
    ----return----
    > f() @ <doctest test.test_pdbx.test_pdbx_basic_commands[0]>:3
    -> return x
    (Pdbx) step
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:5
    -> pass
    (Pdbx) step
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:3
    -> for i in range(5):
    (Pdbx) step
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:4
    -> n = f(i)
    (Pdbx) step
    > f() @ <doctest test.test_pdbx.test_pdbx_basic_commands[0]>:2
    -> x = x + 1
    (Pdbx) return
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:5
    -> pass
    (Pdbx) next
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:3
    -> for i in range(5):
    (Pdbx) n
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_commands[1]>:4
    -> n = f(i)
    (Pdbx) continue
    """

def test_pdbx_basic_breakpoint():
    """Test the breakpoints of pdbx.

    >>> def f(x):
    ...     x = x + 1
    ...     return x

    >>> def test_function():
    ...     import pdbx; pdbx.break_here()
    ...     for i in range(5):
    ...         n = f(i)
    ...         pass
    ...     a = 3

    >>> with PdbxTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'break invalid',
    ...     'break f',
    ...     'continue',
    ...     'clear',
    ...     'return',
    ...     'break 6',
    ...     'break 100',
    ...     'continue',
    ...     'continue',
    ... ]):
    ...     test_function()
        > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_breakpoint[1]>:3
    -> for i in range(5):
    (Pdbx) break invalid
    Invalid breakpoint argument: invalid
    (Pdbx) break f
    (Pdbx) continue
    ----call----
    > f() @ <doctest test.test_pdbx.test_pdbx_basic_breakpoint[0]>:1
    -> def f(x):
    (Pdbx) clear
    (Pdbx) return
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_breakpoint[1]>:5
    -> pass
    (Pdbx) break 6
    (Pdbx) break 100
    Can't find line 100 in file <doctest test.test_pdbx.test_pdbx_basic_breakpoint[1]>
    (Pdbx) continue
    > test_function() @ <doctest test.test_pdbx.test_pdbx_basic_breakpoint[1]>:6
    -> a = 3
    (Pdbx) continue
    """

def test_pdbx_where_command():
    """Test where command

    >>> def g():
    ...     import pdbx; pdbx.break_here()
    ...     pass

    >>> def f():
    ...     g();

    >>> def test_function():
    ...     f()

    >>> with PdbxTestInput([  # doctest: +ELLIPSIS
    ...     'w',
    ...     'where',
    ...     'u',
    ...     'w',
    ...     'd',
    ...     'w',
    ...     'continue',
    ... ]):
    ...     test_function()
    > g() @ <doctest test.test_pdbx.test_pdbx_where_command[0]>:3
    -> pass
    (Pdbx) w
    ...
    #18 <module>() @ <doctest test.test_pdbx.test_pdbx_where_command[3]>:10
        -> test_function()
    #19 test_function() @ <doctest test.test_pdbx.test_pdbx_where_command[2]>:2
        -> f()
    #20 f() @ <doctest test.test_pdbx.test_pdbx_where_command[1]>:2
        -> g();
    >21 g() @ <doctest test.test_pdbx.test_pdbx_where_command[0]>:3
        -> pass
    (Pdbx) where
    ...
    #18 <module>() @ <doctest test.test_pdbx.test_pdbx_where_command[3]>:10
        -> test_function()
    #19 test_function() @ <doctest test.test_pdbx.test_pdbx_where_command[2]>:2
        -> f()
    #20 f() @ <doctest test.test_pdbx.test_pdbx_where_command[1]>:2
        -> g();
    >21 g() @ <doctest test.test_pdbx.test_pdbx_where_command[0]>:3
        -> pass
    (Pdbx) u
    > f() @ <doctest test.test_pdbx.test_pdbx_where_command[1]>:2
    -> g();
    (Pdbx) w
    ...
    #19 test_function() @ <doctest test.test_pdbx.test_pdbx_where_command[2]>:2
        -> f()
    >20 f() @ <doctest test.test_pdbx.test_pdbx_where_command[1]>:2
        -> g();
    #21 g() @ <doctest test.test_pdbx.test_pdbx_where_command[0]>:3
        -> pass
    (Pdbx) d
    > g() @ <doctest test.test_pdbx.test_pdbx_where_command[0]>:3
    -> pass
    (Pdbx) w
    ...
    #18 <module>() @ <doctest test.test_pdbx.test_pdbx_where_command[3]>:10
        -> test_function()
    #19 test_function() @ <doctest test.test_pdbx.test_pdbx_where_command[2]>:2
        -> f()
    #20 f() @ <doctest test.test_pdbx.test_pdbx_where_command[1]>:2
        -> g();
    >21 g() @ <doctest test.test_pdbx.test_pdbx_where_command[0]>:3
        -> pass
    (Pdbx) continue
    """

def load_tests(loader, tests, pattern):
    from test import test_pdbx
    tests.addTest(doctest.DocTestSuite(test_pdbx))
    return tests


@support.requires_subprocess()
class PdbxTestCase(unittest.TestCase):
    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)

    @unittest.skipIf(sys.flags.safe_path,
                     'PYTHONSAFEPATH changes default sys.path')
    def _run_pdbx(self, pdbx_args, commands, expected_returncode=0):
        self.addCleanup(os_helper.rmtree, '__pycache__')
        cmd = [sys.executable, '-m', 'pdbx'] + pdbx_args
        with subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stdin=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                env = {**os.environ, 'PYTHONIOENCODING': 'utf-8'}
        ) as proc:
            stdout, stderr = proc.communicate(str.encode(commands))
        stdout = stdout and bytes.decode(stdout)
        stderr = stderr and bytes.decode(stderr)
        self.assertEqual(
            proc.returncode,
            expected_returncode,
            f"Unexpected return code\nstdout: {stdout}\nstderr: {stderr}"
        )
        return stdout, stderr

    def run_pdbx_script(self, script, commands, expected_returncode=0):
        """Run 'script' lines with pdbx and the pdbx 'commands'."""
        filename = 'main.py'
        with open(filename, 'w') as f:
            f.write(textwrap.dedent(script))
        self.addCleanup(os_helper.unlink, filename)
        return self._run_pdbx([filename], commands, expected_returncode)

    def run_pdbx_module(self, script, commands):
        """Runs the script code as part of a module"""
        self.module_name = 't_main'
        os_helper.rmtree(self.module_name)
        main_file = self.module_name + '/__main__.py'
        init_file = self.module_name + '/__init__.py'
        os.mkdir(self.module_name)
        with open(init_file, 'w') as f:
            pass
        with open(main_file, 'w') as f:
            f.write(textwrap.dedent(script))
        self.addCleanup(os_helper.rmtree, self.module_name)
        return self._run_pdbx(['-m', self.module_name], commands)

    def test_run_module(self):
        script = """print("SUCCESS")"""
        commands = """
            continue
            quit
        """
        stdout, stderr = self.run_pdbx_module(script, commands)
        self.assertTrue(any("SUCCESS" in l for l in stdout.splitlines()), stdout)

    def test_module_is_run_as_main(self):
        script = """
            if __name__ == '__main__':
                print("SUCCESS")
        """
        commands = """
            continue
            quit
        """
        stdout, stderr = self.run_pdbx_module(script, commands)
        self.assertTrue(any("SUCCESS" in l for l in stdout.splitlines()), stdout)

    def test_run_pdbx_with_pdbx(self):
        commands = """
            c
            quit
        """
        stdout, stderr = self._run_pdbx(["-m", "pdbx"], commands)
        self.assertIn(
            pdbx._usage,
            stdout.replace('\r', '')  # remove \r for windows
        )

    def test_module_without_a_main(self):
        module_name = 't_main'
        os_helper.rmtree(module_name)
        init_file = module_name + '/__init__.py'
        os.mkdir(module_name)
        with open(init_file, 'w'):
            pass
        self.addCleanup(os_helper.rmtree, module_name)
        stdout, stderr = self._run_pdbx(
            ['-m', module_name], "", expected_returncode=1
        )
        self.assertIn("ImportError: No module named t_main.__main__",
                      stdout.splitlines())

    def test_package_without_a_main(self):
        pkg_name = 't_pkg'
        module_name = 't_main'
        os_helper.rmtree(pkg_name)
        modpath = pkg_name + '/' + module_name
        os.makedirs(modpath)
        with open(modpath + '/__init__.py', 'w'):
            pass
        self.addCleanup(os_helper.rmtree, pkg_name)
        stdout, stderr = self._run_pdbx(
            ['-m', modpath.replace('/', '.')], "", expected_returncode=1
        )
        self.assertIn(
            "'t_pkg.t_main' is a package and cannot be directly executed",
            stdout)

    def test_blocks_at_first_code_line(self):
        script = """
                #This is a comment, on line 2

                print("SUCCESS")
        """
        commands = """
            quit
        """
        stdout, stderr = self.run_pdbx_module(script, commands)
        self.assertTrue(any("__main__.py:4"
                            in l for l in stdout.splitlines()), stdout)
        self.assertTrue(any("<module>"
                            in l for l in stdout.splitlines()), stdout)

    def test_relative_imports(self):
        self.module_name = 't_main'
        os_helper.rmtree(self.module_name)
        main_file = self.module_name + '/__main__.py'
        init_file = self.module_name + '/__init__.py'
        module_file = self.module_name + '/module.py'
        self.addCleanup(os_helper.rmtree, self.module_name)
        os.mkdir(self.module_name)
        with open(init_file, 'w') as f:
            f.write(textwrap.dedent("""
                top_var = "VAR from top"
            """))
        with open(main_file, 'w') as f:
            f.write(textwrap.dedent("""
                from . import top_var
                from .module import var
                from . import module
                pass # We'll stop here and print the vars
            """))
        with open(module_file, 'w') as f:
            f.write(textwrap.dedent("""
                var = "VAR from module"
                var2 = "second var"
            """))
        commands = """
            b 5
            c
            p top_var
            p var
            p module.var2
            quit
        """
        stdout, _ = self._run_pdbx(['-m', self.module_name], commands)
        self.assertTrue(any("VAR from module" in l for l in stdout.splitlines()), stdout)
        self.assertTrue(any("VAR from top" in l for l in stdout.splitlines()))
        self.assertTrue(any("second var" in l for l in stdout.splitlines()))
