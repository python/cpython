import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import sysconfig
import unittest
from test import support
from test.support import os_helper


LLDB = shutil.which('lldb')
if LLDB is None:
    raise unittest.SkipTest('lldb is not available')

BUILDDIR = sysconfig.get_config_var('abs_builddir')
HOOK = os.path.join(BUILDDIR, 'python_lldb.py')
SAMPLE_SCRIPT = os.path.join(
    os.path.dirname(__file__), '..', 'test_gdb', 'gdb_sample.py'
)
MARKER = 'script print("CPYTHON_LLDB_OUTPUT")'

# Clang's -Og can drop the eval loop's frame variable. On macOS, CC is
# "gcc", so support.python_is_optimized() treats -Og there as unoptimized.
OPTIMIZED = support.python_is_optimized() or (
    'clang' in platform.python_compiler().lower()
    and '-O0' not in sysconfig.get_config_var('PY_CFLAGS').split()
)


def run_lldb(commands, *args):
    env = {
        name: value
        for name, value in os.environ.items()
        if not name.startswith('PYTHON')
    }
    env['PYTHONHASHSEED'] = '123'
    cmd = [LLDB, '--batch', '--no-lldbinit']
    for command in commands:
        cmd.extend(('-o', command))
    cmd.extend(args)
    # LLDB on macOS leaves an "lldb" directory in TMPDIR.
    with os_helper.temp_dir() as env['TMPDIR']:
        return subprocess.run(
            cmd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding='utf-8',
            errors='backslashreplace',
            env=env,
            timeout=support.LONG_TIMEOUT,
        )


def command_output(stdout):
    """Return the output of the commands after MARKER."""
    output = stdout.split('\nCPYTHON_LLDB_OUTPUT\n', 1)[1]
    return ''.join(
        line
        for line in output.splitlines(keepends=True)
        if not line.startswith('(lldb) ')
    )


def setUpModule():
    result = run_lldb(['script print("lldb-python-ok")'])
    if '\nlldb-python-ok\n' not in result.stdout:
        raise unittest.SkipTest(
            f'LLDB Python support is unavailable: {result.stderr}'
        )
    if support.verbose:
        result = run_lldb(['version'])
        print(result.stdout)


class DebuggerTests(unittest.TestCase):
    def get_stack_trace(
        self,
        source=None,
        *,
        commands=(),
        script=None,
        breakpoint='builtin_id',
        hook=HOOK,
    ):
        setup = [
            # ASLR is unrelated to these tests. Some containers forbid the
            # personality syscall that LLDB uses to disable it on Linux.
            'settings set target.disable-aslr false',
            'settings set interpreter.stop-command-source-on-error true',
            f'command script import {shlex.quote(hook)}',
            f'breakpoint set --name {breakpoint}',
            'run',
            MARKER,
        ]
        if script is None:
            # LLDB can shell-expand launch arguments, so pass source as a file.
            script = os.path.join(
                self.enterContext(os_helper.temp_dir()), 'source.py'
            )
            with open(script, 'w', encoding='utf-8') as file:
                file.write(source)
        result = run_lldb(
            [*setup, *commands], '--', sys.executable, '-S', script
        )
        diagnostic = result.stdout + result.stderr
        # A missing debugger entitlement or ptrace permission is a platform
        # limitation. Helper failures and missing breakpoints are failures.
        if (
            'Operation not permitted' in diagnostic
            or 'Not allowed to attach to process' in diagnostic
        ):
            self.skipTest(diagnostic)
        self.assertEqual(result.returncode, 0, diagnostic)
        self.assertIn('stop reason = breakpoint', result.stdout, diagnostic)
        self.assertNotIn('Traceback (most recent call last):', diagnostic)
        # LLDB can report nonfatal DWARF diagnostics for system libraries on
        # stderr. Command failures set a nonzero exit status, checked above.
        if result.stderr and support.verbose:
            print(result.stderr, file=sys.stderr)
        return command_output(result.stdout)

    def get_reprs(self, source, count=1, *, commands=()):
        """Return the summaries of v at the first count calls of id()."""
        stops = ['frame variable v'] + ['continue', 'frame variable v'] * (
            count - 1
        )
        output = self.get_stack_trace(source, commands=[*commands, *stops])
        # LLDB keeps the native type and address before the Python summary.
        reprs = re.findall(
            r'^\(PyObject \*\) v = (?:0x[0-9a-f]+ )?(.*)$', output, re.M
        )
        self.assertEqual(len(reprs), count, output)
        return reprs

    def get_repr(self, source, *, commands=()):
        return self.get_reprs(source, commands=commands)[0]
