import re
import unittest
from test.support import python_is_optimized

from .util import run_gdb, setup_module, DebuggerTests, SAMPLE_SCRIPT


def setUpModule():
    setup_module()


def gdb_has_frame_select():
    # Does this build of gdb have gdb.Frame.select ?
    stdout, stderr = run_gdb("--eval-command=python print(dir(gdb.Frame))")
    m = re.match(r'.*\[(.*)\].*', stdout)
    if not m:
        raise unittest.SkipTest(
            f"Unable to parse output from gdb.Frame.select test\n"
            f"stdout={stdout!r}\n"
            f"stderr={stderr!r}\n")
    gdb_frame_dir = m.group(1).split(', ')
    return "'select'" in gdb_frame_dir

HAS_PYUP_PYDOWN = gdb_has_frame_select()


@unittest.skipIf(python_is_optimized(),
                 "Python was compiled with optimizations")
class PyListTests(DebuggerTests):
    def assertListing(self, expected, actual):
        self.assertEndsWith(actual, expected)

    def test_basic_command(self):
        'Verify that the "py-list" command works'
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-list'])

        self.assertListing('   5    \n'
                           '   6    def bar(a, b, c):\n'
                           '   7        baz(a, b, c)\n'
                           '   8    \n'
                           '   9    def baz(*args):\n'
                           ' >10        id(42)\n'
                           '  11    \n'
                           '  12    foo(1, 2, 3)\n',
                           bt)

    def test_one_abs_arg(self):
        'Verify the "py-list" command with one absolute argument'
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-list 9'])

        self.assertListing('   9    def baz(*args):\n'
                           ' >10        id(42)\n'
                           '  11    \n'
                           '  12    foo(1, 2, 3)\n',
                           bt)

    def test_two_abs_args(self):
        'Verify the "py-list" command with two absolute arguments'
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-list 1,3'])

        self.assertListing('   1    # Sample script for use by test_gdb\n'
                           '   2    \n'
                           '   3    def foo(a, b, c):\n',
                           bt)

SAMPLE_WITH_C_CALL = """

from _testcapi import pyobject_vectorcall

def foo(a, b, c):
    bar(a, b, c)

def bar(a, b, c):
    pyobject_vectorcall(baz, (a, b, c), None)

def baz(*args):
    id(42)

foo(1, 2, 3)

"""


class StackNavigationTests(DebuggerTests):
    @unittest.skipUnless(HAS_PYUP_PYDOWN, "test requires py-up/py-down commands")
    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_pyup_command(self):
        'Verify that the "py-up" command works'
        bt = self.get_stack_trace(source=SAMPLE_WITH_C_CALL,
                                  cmds_after_breakpoint=['py-up', 'py-up'])
        self.assertMultilineMatches(bt,
                                    r'''^.*
#[0-9]+ Frame 0x-?[0-9a-f]+, for file <string>, line 12, in baz \(args=\(1, 2, 3\)\)
#[0-9]+ <built-in method pyobject_vectorcall of module object at remote 0x[0-9a-f]+>
$''')

    @unittest.skipUnless(HAS_PYUP_PYDOWN, "test requires py-up/py-down commands")
    def test_down_at_bottom(self):
        'Verify handling of "py-down" at the bottom of the stack'
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-down'])
        self.assertEndsWith(bt,
                            'Unable to find a newer python frame\n')

    @unittest.skipUnless(HAS_PYUP_PYDOWN, "test requires py-up/py-down commands")
    def test_up_at_top(self):
        'Verify handling of "py-up" at the top of the stack'
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-up'] * 5)
        self.assertEndsWith(bt,
                            'Unable to find an older python frame\n')

    @unittest.skipUnless(HAS_PYUP_PYDOWN, "test requires py-up/py-down commands")
    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_up_then_down(self):
        'Verify "py-up" followed by "py-down"'
        bt = self.get_stack_trace(source=SAMPLE_WITH_C_CALL,
                                  cmds_after_breakpoint=['py-up', 'py-up', 'py-down'])
        self.assertMultilineMatches(bt,
                                    r'''^.*
#[0-9]+ Frame 0x-?[0-9a-f]+, for file <string>, line 12, in baz \(args=\(1, 2, 3\)\)
#[0-9]+ <built-in method pyobject_vectorcall of module object at remote 0x[0-9a-f]+>
#[0-9]+ Frame 0x-?[0-9a-f]+, for file <string>, line 12, in baz \(args=\(1, 2, 3\)\)
$''')

class PyPrintTests(DebuggerTests):
    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_basic_command(self):
        'Verify that the "py-print" command works'
        bt = self.get_stack_trace(source=SAMPLE_WITH_C_CALL,
                                  cmds_after_breakpoint=['py-up', 'py-print args'])
        self.assertMultilineMatches(bt,
                                    r".*\nlocal 'args' = \(1, 2, 3\)\n.*")

    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    @unittest.skipUnless(HAS_PYUP_PYDOWN, "test requires py-up/py-down commands")
    def test_print_after_up(self):
        bt = self.get_stack_trace(source=SAMPLE_WITH_C_CALL,
                                  cmds_after_breakpoint=['py-up', 'py-up', 'py-print c', 'py-print b', 'py-print a'])
        self.assertMultilineMatches(bt,
                                    r".*\nlocal 'c' = 3\nlocal 'b' = 2\nlocal 'a' = 1\n.*")

    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_printing_global(self):
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-up', 'py-print __name__'])
        self.assertMultilineMatches(bt,
                                    r".*\nglobal '__name__' = '__main__'\n.*")

    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_printing_builtin(self):
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-up', 'py-print len'])
        self.assertMultilineMatches(bt,
                                    r".*\nbuiltin 'len' = <built-in method len of module object at remote 0x-?[0-9a-f]+>\n.*")

class PyLocalsTests(DebuggerTests):
    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_basic_command(self):
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-up', 'py-locals'])
        self.assertMultilineMatches(bt,
                                    r".*\nargs = \(1, 2, 3\)\n.*")

    @unittest.skipUnless(HAS_PYUP_PYDOWN, "test requires py-up/py-down commands")
    @unittest.skipIf(python_is_optimized(),
                     "Python was compiled with optimizations")
    def test_locals_after_up(self):
        bt = self.get_stack_trace(script=SAMPLE_SCRIPT,
                                  cmds_after_breakpoint=['py-up', 'py-up', 'py-locals'])
        self.assertMultilineMatches(bt,
                                    r'''^.*
Locals for foo
a = 1
b = 2
c = 3
Locals for <module>
.*$''')
