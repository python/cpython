import os
import re
import shlex
import sys
import sysconfig
import unittest
from test import support
from test.support import os_helper

from .util import (
    DebuggerTests,
    HOOK,
    MARKER,
    OPTIMIZED,
    SAMPLE_SCRIPT,
    command_output,
    run_lldb,
    setUpModule as setUpModule,
)


SAMPLE_WITH_C_CALL = """\
from _testcapi import pyobject_vectorcall

def foo(a, b, c):
    bar(a, b, c)

def bar(a, b, c):
    pyobject_vectorcall(baz, (a, b, c), None)

def baz(*args):
    id(42)

foo(1, 2, 3)
"""


@unittest.skipIf(OPTIMIZED, 'Python has compiler optimizations')
class CommandTests(DebuggerTests):
    def test_backtrace(self):
        output = self.get_stack_trace(
            script=SAMPLE_SCRIPT,
            commands=['py-bt', 'py-bt-full', 'expression 0'],
        )
        # py-* commands must not use LLDB's expression history.
        self.assertIn('(int) $0 = 0\n', output)
        self.assertRegex(
            output,
            r'Traceback \(most recent call first\):\n'
            r'  <built-in method id of module object at remote 0x[0-9a-f]+>\n'
            r'  File ".*gdb_sample.py", line 10, in baz\n    id\(42\)\n'
            r'  File ".*gdb_sample.py", line 7, in bar\n    baz\(a, b, c\)\n'
            r'  File ".*gdb_sample.py", line 4, in foo\n'
            r'    bar\(a=a, b=b, c=c\)\n'
            r'  File ".*gdb_sample.py", line 12, in <module>\n'
            r'    foo\(1, 2, 3\)\n',
        )
        for name, line, args in (
            ('baz', 10, 'args=(1, 2, 3)'),
            ('bar', 7, 'a=1, b=2, c=3'),
            ('foo', 4, 'a=1, b=2, c=3'),
            ('<module>', 12, ''),
        ):
            self.assertRegex(
                output,
                rf'#[0-9]+ Frame 0x[0-9a-f]+, for file .*gdb_sample.py, '
                rf'line {line}, in {name} \({re.escape(args)}\)',
            )

    def test_list(self):
        output = self.get_stack_trace(
            script=SAMPLE_SCRIPT, commands=['py-list']
        )
        self.assertEqual(
            output,
            '   5    \n'
            '   6    def bar(a, b, c):\n'
            '   7        baz(a, b, c)\n'
            '   8    \n'
            '   9    def baz(*args):\n'
            ' >10        id(42)\n'
            '  11    \n'
            '  12    foo(1, 2, 3)\n',
        )

    def test_up_down(self):
        output = self.get_stack_trace(
            SAMPLE_WITH_C_CALL,
            commands=['py-down', 'py-up', 'py-up', 'py-down', *['py-up'] * 3],
        )
        frames = re.findall(
            r'^(?:#[0-9]+ (?:<built-in method (\w+)|Frame .*, in (\S+) \()'
            r'|Unable to find an? (\w+))',
            output,
            re.M,
        )
        self.assertEqual(
            [''.join(frame) for frame in frames],
            [
                'newer',
                'baz',
                'pyobject_vectorcall',
                'baz',
                'pyobject_vectorcall',
                'bar',
                'foo',
                '<module>',
                'older',
            ],
            output,
        )

    def test_print(self):
        output = self.get_stack_trace(
            SAMPLE_WITH_C_CALL,
            commands=[
                'py-up',
                'py-print args',
                'py-print __name__',
                'py-print len',
                'py-print absent_name',
                'py-up',
                'py-up',
                'py-print c',
                'py-print b',
                'py-print a',
            ],
        )
        self.assertRegex(
            output,
            r"local 'args' = \(1, 2, 3\)\n"
            r"global '__name__' = '__main__'\n"
            r"builtin 'len' = <built-in method len .*>\n"
            r"'absent_name' not found\n",
        )
        self.assertEndsWith(
            output, "local 'c' = 3\nlocal 'b' = 2\nlocal 'a' = 1\n"
        )


class LoadingTests(DebuggerTests):
    def test_before_start(self):
        result = run_lldb(
            [f'command script import {shlex.quote(HOOK)}', 'py-bt']
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('Unable to locate python frame', result.stdout)

    def test_after_exit(self):
        output = self.get_stack_trace(
            'id(42)', commands=['continue', 'py-bt', 'py-up']
        )
        self.assertIn('exited with status = 0', output)
        self.assertEqual(
            output.count('Unable to locate python frame'), 2, output
        )

    def test_source_helper_and_reload(self):
        helper = os.path.join(
            sysconfig.get_config_var('abs_srcdir'),
            'Tools',
            'lldb',
            'libpython.py',
        )
        output = self.get_stack_trace(
            'id(42)',
            hook=helper,
            commands=[
                'frame variable v',
                f'command script import -r {shlex.quote(HOOK)}',
                'frame variable v',
                'py-bt',
            ],
        )
        self.assertEqual(len(re.findall(r'v = 0x[0-9a-f]+ 42\n', output)), 2)
        self.assertIn('line 1, in <module>', output)

    def test_multiple_targets(self):
        output = self.get_stack_trace(
            'id(42)',
            commands=[
                f'target create {shlex.quote(sys.executable)}',
                'settings set target.disable-aslr false',
                'breakpoint set --name builtin_id',
                'settings set -- target.run-args -S -c "id([1, 2])"',
                'run',
                'script print("SECOND_TARGET")',
                'frame variable v',
                'target select 0',
                'script print("FIRST_TARGET")',
                'frame variable v',
            ],
        )
        self.assertRegex(
            output,
            r'SECOND_TARGET\n\(PyObject \*\) v = 0x[0-9a-f]+ \[1, 2\]\n',
        )
        self.assertRegex(
            output, r'FIRST_TARGET\n\(PyObject \*\) v = 0x[0-9a-f]+ 42\n'
        )

    def test_restart(self):
        output = self.get_stack_trace(
            'id(42)',
            commands=[
                'frame variable v',
                'process kill',
                'settings set -- target.run-args -S -c "id([1, 2])"',
                'run',
                'frame variable v',
            ],
        )
        summaries = re.findall(
            r'^\(PyObject \*\) v = 0x[0-9a-f]+ (.*)$', output, re.M
        )
        self.assertEqual(summaries, ['42', '[1, 2]'])

    def test_nested_exec(self):
        # Optimized native stacks can contain artificial tail-call frames.
        # LLDB can give these the same ID as an earlier frame. Traversal
        # must terminate in both directions, including full frame numbering.
        output = self.get_stack_trace(
            'exec("id(42)")',
            commands=['py-bt', 'py-bt-full', 'py-up', 'py-down'],
        )
        self.assertEqual(
            output.count('Traceback (most recent call first):'), 2
        )
        self.assertIn('File "<string>", line 1, in <module>', output)

    @support.requires_resource('cpu')
    def test_thread_selection(self):
        source = """\
import threading
ready = threading.Barrier(4)
stop = threading.Event()
def worker():
    ready.wait()
    stop.wait()
for _ in range(3):
    threading.Thread(target=worker, daemon=True).start()
ready.wait()
id(42)
"""
        commands = []
        for index in range(1, 5):
            commands.extend((f'thread select {index}', 'py-bt'))
        output = self.get_stack_trace(source, commands=commands)
        self.assertEqual(
            output.count('Traceback (most recent call first):'), 4
        )
        # Only the main thread is in this module. Optimization can remove
        # the worker frame, but must not make it use the main thread's stack.
        self.assertEqual(output.count('line 10, in <module>'), 1, output)
        if not OPTIMIZED:
            self.assertEqual(output.count('in worker\n'), 3, output)

    @support.requires_resource('cpu')
    @unittest.skipIf(OPTIMIZED, 'Python has compiler optimizations')
    def test_core_file(self):
        plugin = 'mach-o' if sys.platform == 'darwin' else 'minidump'
        help_result = run_lldb(['help process save-core'])
        if plugin not in help_result.stdout:
            self.skipTest(f'LLDB has no {plugin} core writer')
        with os_helper.temp_dir() as directory:
            core = os.path.join(directory, 'python.core')
            commands = ['frame variable v', 'py-bt-full', 'py-up', 'py-locals']
            live_output = self.get_stack_trace(
                script=SAMPLE_SCRIPT,
                commands=[
                    f'process save-core --plugin-name={plugin} '
                    f'--style full {shlex.quote(core)}',
                    *commands,
                ],
            )
            result = run_lldb(
                [
                    f'command script import {shlex.quote(HOOK)}',
                    MARKER,
                    *commands,
                ],
                '--core',
                core,
                '--',
                sys.executable,
            )
            self.assertEqual(
                result.returncode, 0, result.stdout + result.stderr
            )
            self.assertEqual(command_output(result.stdout), live_output)


if __name__ == '__main__':
    unittest.main()
