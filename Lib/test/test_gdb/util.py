import os
import re
import shlex
import shutil
import subprocess
import sys
import sysconfig
import unittest
from test import support


GDB_PROGRAM = shutil.which('gdb') or 'gdb'

# Location of custom hooks file in a repository checkout.
CHECKOUT_HOOK_PATH = os.path.join(os.path.dirname(sys.executable),
                                  'python-gdb.py')

SAMPLE_SCRIPT = os.path.join(os.path.dirname(__file__), 'gdb_sample.py')
BREAKPOINT_FN = 'builtin_id'

PYTHONHASHSEED = '123'


def clean_environment():
    # Remove PYTHON* environment variables such as PYTHONHOME
    return {name: value for name, value in os.environ.items()
            if not name.startswith('PYTHON')}


# Temporary value until it's initialized by get_gdb_version() below
GDB_VERSION = (0, 0)

def run_gdb(*args, exitcode=0, check=True, **env_vars):
    """Runs gdb in --batch mode with the additional arguments given by *args.

    Returns its (stdout, stderr) decoded from utf-8 using the replace handler.
    """
    env = clean_environment()
    if env_vars:
        env.update(env_vars)

    cmd = [GDB_PROGRAM,
           # Batch mode: Exit after processing all the command files
           # specified with -x/--command
           '--batch',
            # -nx: Do not execute commands from any .gdbinit initialization
            # files (gh-66384)
           '-nx']
    if GDB_VERSION >= (7, 4):
        cmd.extend(('--init-eval-command',
                    f'add-auto-load-safe-path {CHECKOUT_HOOK_PATH}'))
    cmd.extend(args)

    proc = subprocess.run(
        cmd,
        # Redirect stdin to prevent gdb from messing with the terminal settings
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf8", errors="backslashreplace",
        env=env)

    stdout = proc.stdout
    stderr = proc.stderr
    if check and proc.returncode != exitcode:
        cmd_text = shlex.join(cmd)
        raise Exception(f"{cmd_text} failed with exit code {proc.returncode}, "
                        f"expected exit code {exitcode}:\n"
                        f"stdout={stdout!r}\n"
                        f"stderr={stderr!r}")

    return (stdout, stderr)


def get_gdb_version():
    try:
        stdout, stderr = run_gdb('--version')
    except OSError as exc:
        # This is what "no gdb" looks like.  There may, however, be other
        # errors that manifest this way too.
        raise unittest.SkipTest(f"Couldn't find gdb program on the path: {exc}")

    # Regex to parse:
    # 'GNU gdb (GDB; SUSE Linux Enterprise 12) 7.7\n' -> 7.7
    # 'GNU gdb (GDB) Fedora 7.9.1-17.fc22\n' -> 7.9
    # 'GNU gdb 6.1.1 [FreeBSD]\n' -> 6.1
    # 'GNU gdb (GDB) Fedora (7.5.1-37.fc18)\n' -> 7.5
    # 'HP gdb 6.7 for HP Itanium (32 or 64 bit) and target HP-UX 11iv2 and 11iv3.\n' -> 6.7
    match = re.search(r"^(?:GNU|HP) gdb.*?\b(\d+)\.(\d+)", stdout)
    if match is None:
        raise Exception("unable to parse gdb version: %r" % stdout)
    version_text = stdout
    major = int(match.group(1))
    minor = int(match.group(2))
    version = (major, minor)
    return (version_text, version)

GDB_VERSION_TEXT, GDB_VERSION = get_gdb_version()
if GDB_VERSION < (7, 0):
    raise unittest.SkipTest(
        f"gdb versions before 7.0 didn't support python embedding. "
        f"Saw gdb version {GDB_VERSION[0]}.{GDB_VERSION[1]}:\n"
        f"{GDB_VERSION_TEXT}")


def check_usable_gdb():
    # Verify that "gdb" was built with the embedded Python support enabled and
    # verify that "gdb" can load our custom hooks, as OS security settings may
    # disallow this without a customized .gdbinit.
    stdout, stderr = run_gdb(
        '--eval-command=python import sys; print(sys.version_info)',
        '--args', sys.executable,
        check=False)

    if "auto-loading has been declined" in stderr:
        raise unittest.SkipTest(
            f"gdb security settings prevent use of custom hooks; "
            f"stderr: {stderr!r}")

    if not stdout:
        raise unittest.SkipTest(
            f"gdb not built with embedded python support; "
            f"stderr: {stderr!r}")

    if "major=2" in stdout:
        raise unittest.SkipTest("gdb built with Python 2")

check_usable_gdb()


# Control-flow enforcement technology
def cet_protection():
    cflags = sysconfig.get_config_var('CFLAGS')
    if not cflags:
        return False
    flags = cflags.split()
    # True if "-mcet -fcf-protection" options are found, but false
    # if "-fcf-protection=none" or "-fcf-protection=return" is found.
    return (('-mcet' in flags)
            and any((flag.startswith('-fcf-protection')
                     and not flag.endswith(("=none", "=return")))
                    for flag in flags))
CET_PROTECTION = cet_protection()


def setup_module():
    if support.verbose:
        print(f"gdb version {GDB_VERSION[0]}.{GDB_VERSION[1]}:")
        for line in GDB_VERSION_TEXT.splitlines():
            print(" " * 4 + line)
        print(f"    path: {GDB_PROGRAM}")
        print()


class DebuggerTests(unittest.TestCase):

    """Test that the debugger can debug Python."""

    def get_stack_trace(self, source=None, script=None,
                        breakpoint=BREAKPOINT_FN,
                        cmds_after_breakpoint=None,
                        import_site=False,
                        ignore_stderr=False):
        '''
        Run 'python -c SOURCE' under gdb with a breakpoint.

        Support injecting commands after the breakpoint is reached

        Returns the stdout from gdb

        cmds_after_breakpoint: if provided, a list of strings: gdb commands
        '''
        # We use "set breakpoint pending yes" to avoid blocking with a:
        #   Function "foo" not defined.
        #   Make breakpoint pending on future shared library load? (y or [n])
        # error, which typically happens python is dynamically linked (the
        # breakpoints of interest are to be found in the shared library)
        # When this happens, we still get:
        #   Function "textiowrapper_write" not defined.
        # emitted to stderr each time, alas.

        # Initially I had "--eval-command=continue" here, but removed it to
        # avoid repeated print breakpoints when traversing hierarchical data
        # structures

        # Generate a list of commands in gdb's language:
        commands = [
            'set breakpoint pending yes',
            'break %s' % breakpoint,

            # The tests assume that the first frame of printed
            #  backtrace will not contain program counter,
            #  that is however not guaranteed by gdb
            #  therefore we need to use 'set print address off' to
            #  make sure the counter is not there. For example:
            # #0 in PyObject_Print ...
            #  is assumed, but sometimes this can be e.g.
            # #0 0x00003fffb7dd1798 in PyObject_Print ...
            'set print address off',

            'run',
        ]

        # GDB as of 7.4 onwards can distinguish between the
        # value of a variable at entry vs current value:
        #   http://sourceware.org/gdb/onlinedocs/gdb/Variables.html
        # which leads to the selftests failing with errors like this:
        #   AssertionError: 'v@entry=()' != '()'
        # Disable this:
        if GDB_VERSION >= (7, 4):
            commands += ['set print entry-values no']

        if cmds_after_breakpoint:
            if CET_PROTECTION:
                # bpo-32962: When Python is compiled with -mcet
                # -fcf-protection, function arguments are unusable before
                # running the first instruction of the function entry point.
                # The 'next' command makes the required first step.
                commands += ['next']
            commands += cmds_after_breakpoint
        else:
            commands += ['backtrace']

        # print commands

        # Use "commands" to generate the arguments with which to invoke "gdb":
        args = ['--eval-command=%s' % cmd for cmd in commands]
        args += ["--args",
                 sys.executable]
        args.extend(subprocess._args_from_interpreter_flags())

        if not import_site:
            # -S suppresses the default 'import site'
            args += ["-S"]

        if source:
            args += ["-c", source]
        elif script:
            args += [script]

        # Use "args" to invoke gdb, capturing stdout, stderr:
        out, err = run_gdb(*args, PYTHONHASHSEED=PYTHONHASHSEED)

        if not ignore_stderr:
            for line in err.splitlines():
                print(line, file=sys.stderr)

        # bpo-34007: Sometimes some versions of the shared libraries that
        # are part of the traceback are compiled in optimised mode and the
        # Program Counter (PC) is not present, not allowing gdb to walk the
        # frames back. When this happens, the Python bindings of gdb raise
        # an exception, making the test impossible to succeed.
        if "PC not saved" in err:
            raise unittest.SkipTest("gdb cannot walk the frame object"
                                    " because the Program Counter is"
                                    " not present")

        # bpo-40019: Skip the test if gdb failed to read debug information
        # because the Python binary is optimized.
        for pattern in (
            '(frame information optimized out)',
            'Unable to read information on python frame',

            # gh-91960: On Python built with "clang -Og", gdb gets
            # "frame=<optimized out>" for _PyEval_EvalFrameDefault() parameter
            '(unable to read python frame information)',

            # gh-104736: On Python built with "clang -Og" on ppc64le,
            # "py-bt" displays a truncated or not traceback, but "where"
            # logs this error message:
            'Backtrace stopped: frame did not save the PC',

            # gh-104736: When "bt" command displays something like:
            # "#1  0x0000000000000000 in ?? ()", the traceback is likely
            # truncated or wrong.
            ' ?? ()',
        ):
            if pattern in out:
                raise unittest.SkipTest(f"{pattern!r} found in gdb output")

        return out

    def assertMultilineMatches(self, actual, pattern):
        m = re.match(pattern, actual, re.DOTALL)
        if not m:
            self.fail(msg='%r did not match %r' % (actual, pattern))
