from contextlib import contextmanager
import datetime
import faulthandler
import os
import re
import signal
import subprocess
import sys
from test import support, script_helper
from test.script_helper import assert_python_ok
import tempfile
import unittest

try:
    import threading
    HAVE_THREADS = True
except ImportError:
    HAVE_THREADS = False

TIMEOUT = 0.5

def expected_traceback(lineno1, lineno2, header, min_count=1):
    regex = header
    regex += '  File "<string>", line %s in func\n' % lineno1
    regex += '  File "<string>", line %s in <module>' % lineno2
    if 1 < min_count:
        return '^' + (regex + '\n') * (min_count - 1) + regex
    else:
        return '^' + regex + '$'

@contextmanager
def temporary_filename():
    filename = tempfile.mktemp()
    try:
        yield filename
    finally:
        support.unlink(filename)

class FaultHandlerTests(unittest.TestCase):
    def get_output(self, code, filename=None):
        """
        Run the specified code in Python (in a new child process) and read the
        output from the standard error or from a file (if filename is set).
        Return the output lines as a list.

        Strip the reference count from the standard error for Python debug
        build, and replace "Current thread 0x00007f8d8fbd9700" by "Current
        thread XXX".
        """
        with support.SuppressCrashReport():
            process = script_helper.spawn_python('-c', code)
        stdout, stderr = process.communicate()
        exitcode = process.wait()
        output = support.strip_python_stderr(stdout)
        output = output.decode('ascii', 'backslashreplace')
        if filename:
            self.assertEqual(output, '')
            with open(filename, "rb") as fp:
                output = fp.read()
            output = output.decode('ascii', 'backslashreplace')
        output = re.sub('Current thread 0x[0-9a-f]+',
                        'Current thread XXX',
                        output)
        return output.splitlines(), exitcode

    def check_fatal_error(self, code, line_number, name_regex,
                          filename=None, all_threads=True, other_regex=None):
        """
        Check that the fault handler for fatal errors is enabled and check the
        traceback from the child process output.

        Raise an error if the output doesn't match the expected format.
        """
        if all_threads:
            header = 'Current thread XXX (most recent call first)'
        else:
            header = 'Stack (most recent call first)'
        regex = """
^Fatal Python error: {name}

{header}:
  File "<string>", line {lineno} in <module>
""".strip()
        regex = regex.format(
            lineno=line_number,
            name=name_regex,
            header=re.escape(header))
        if other_regex:
            regex += '|' + other_regex
        output, exitcode = self.get_output(code, filename)
        output = '\n'.join(output)
        self.assertRegex(output, regex)
        self.assertNotEqual(exitcode, 0)

    @unittest.skipIf(sys.platform.startswith('aix'),
                     "the first page of memory is a mapped read-only on AIX")
    def test_read_null(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._read_null()
""".strip(),
            3,
            # Issue #12700: Read NULL raises SIGILL on Mac OS X Lion
            '(?:Segmentation fault|Bus error|Illegal instruction)')

    def test_sigsegv(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._sigsegv()
""".strip(),
            3,
            'Segmentation fault')

    def test_sigabrt(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._sigabrt()
""".strip(),
            3,
            'Aborted')

    @unittest.skipIf(sys.platform == 'win32',
                     "SIGFPE cannot be caught on Windows")
    def test_sigfpe(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._sigfpe()
""".strip(),
            3,
            'Floating point exception')

    @unittest.skipIf(not hasattr(faulthandler, '_sigbus'),
                     "need faulthandler._sigbus()")
    def test_sigbus(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._sigbus()
""".strip(),
            3,
            'Bus error')

    @unittest.skipIf(not hasattr(faulthandler, '_sigill'),
                     "need faulthandler._sigill()")
    def test_sigill(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._sigill()
""".strip(),
            3,
            'Illegal instruction')

    def test_fatal_error(self):
        self.check_fatal_error("""
import faulthandler
faulthandler._fatal_error(b'xyz')
""".strip(),
            2,
            'xyz')

    @unittest.skipIf(sys.platform.startswith('openbsd') and HAVE_THREADS,
                     "Issue #12868: sigaltstack() doesn't work on "
                     "OpenBSD if Python is compiled with pthread")
    @unittest.skipIf(not hasattr(faulthandler, '_stack_overflow'),
                     'need faulthandler._stack_overflow()')
    def test_stack_overflow(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._stack_overflow()
""".strip(),
            3,
            '(?:Segmentation fault|Bus error)',
            other_regex='unable to raise a stack overflow')

    def test_gil_released(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._read_null(True)
""".strip(),
            3,
            '(?:Segmentation fault|Bus error|Illegal instruction)')

    def test_enable_file(self):
        with temporary_filename() as filename:
            self.check_fatal_error("""
import faulthandler
output = open({filename}, 'wb')
faulthandler.enable(output)
faulthandler._sigsegv()
""".strip().format(filename=repr(filename)),
                4,
                'Segmentation fault',
                filename=filename)

    def test_enable_single_thread(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable(all_threads=False)
faulthandler._sigsegv()
""".strip(),
            3,
            'Segmentation fault',
            all_threads=False)

    def test_disable(self):
        code = """
import faulthandler
faulthandler.enable()
faulthandler.disable()
faulthandler._sigsegv()
""".strip()
        not_expected = 'Fatal Python error'
        stderr, exitcode = self.get_output(code)
        stder = '\n'.join(stderr)
        self.assertTrue(not_expected not in stderr,
                     "%r is present in %r" % (not_expected, stderr))
        self.assertNotEqual(exitcode, 0)

    def test_is_enabled(self):
        orig_stderr = sys.stderr
        try:
            # regrtest may replace sys.stderr by io.StringIO object, but
            # faulthandler.enable() requires that sys.stderr has a fileno()
            # method
            sys.stderr = sys.__stderr__

            was_enabled = faulthandler.is_enabled()
            try:
                faulthandler.enable()
                self.assertTrue(faulthandler.is_enabled())
                faulthandler.disable()
                self.assertFalse(faulthandler.is_enabled())
            finally:
                if was_enabled:
                    faulthandler.enable()
                else:
                    faulthandler.disable()
        finally:
            sys.stderr = orig_stderr

    def test_disabled_by_default(self):
        # By default, the module should be disabled
        code = "import faulthandler; print(faulthandler.is_enabled())"
        args = (sys.executable, '-E', '-c', code)
        # don't use assert_python_ok() because it always enable faulthandler
        output = subprocess.check_output(args)
        self.assertEqual(output.rstrip(), b"False")

    def test_sys_xoptions(self):
        # Test python -X faulthandler
        code = "import faulthandler; print(faulthandler.is_enabled())"
        args = (sys.executable, "-E", "-X", "faulthandler", "-c", code)
        # don't use assert_python_ok() because it always enable faulthandler
        output = subprocess.check_output(args)
        self.assertEqual(output.rstrip(), b"True")

    def test_env_var(self):
        # empty env var
        code = "import faulthandler; print(faulthandler.is_enabled())"
        args = (sys.executable, "-c", code)
        env = os.environ.copy()
        env['PYTHONFAULTHANDLER'] = ''
        # don't use assert_python_ok() because it always enable faulthandler
        output = subprocess.check_output(args, env=env)
        self.assertEqual(output.rstrip(), b"False")

        # non-empty env var
        env = os.environ.copy()
        env['PYTHONFAULTHANDLER'] = '1'
        output = subprocess.check_output(args, env=env)
        self.assertEqual(output.rstrip(), b"True")

    def check_dump_traceback(self, filename):
        """
        Explicitly call dump_traceback() function and check its output.
        Raise an error if the output doesn't match the expected format.
        """
        code = """
import faulthandler

def funcB():
    if {has_filename}:
        with open({filename}, "wb") as fp:
            faulthandler.dump_traceback(fp, all_threads=False)
    else:
        faulthandler.dump_traceback(all_threads=False)

def funcA():
    funcB()

funcA()
""".strip()
        code = code.format(
            filename=repr(filename),
            has_filename=bool(filename),
        )
        if filename:
            lineno = 6
        else:
            lineno = 8
        expected = [
            'Stack (most recent call first):',
            '  File "<string>", line %s in funcB' % lineno,
            '  File "<string>", line 11 in funcA',
            '  File "<string>", line 13 in <module>'
        ]
        trace, exitcode = self.get_output(code, filename)
        self.assertEqual(trace, expected)
        self.assertEqual(exitcode, 0)

    def test_dump_traceback(self):
        self.check_dump_traceback(None)

    def test_dump_traceback_file(self):
        with temporary_filename() as filename:
            self.check_dump_traceback(filename)

    def test_truncate(self):
        maxlen = 500
        func_name = 'x' * (maxlen + 50)
        truncated = 'x' * maxlen + '...'
        code = """
import faulthandler

def {func_name}():
    faulthandler.dump_traceback(all_threads=False)

{func_name}()
""".strip()
        code = code.format(
            func_name=func_name,
        )
        expected = [
            'Stack (most recent call first):',
            '  File "<string>", line 4 in %s' % truncated,
            '  File "<string>", line 6 in <module>'
        ]
        trace, exitcode = self.get_output(code)
        self.assertEqual(trace, expected)
        self.assertEqual(exitcode, 0)

    @unittest.skipIf(not HAVE_THREADS, 'need threads')
    def check_dump_traceback_threads(self, filename):
        """
        Call explicitly dump_traceback(all_threads=True) and check the output.
        Raise an error if the output doesn't match the expected format.
        """
        code = """
import faulthandler
from threading import Thread, Event
import time

def dump():
    if {filename}:
        with open({filename}, "wb") as fp:
            faulthandler.dump_traceback(fp, all_threads=True)
    else:
        faulthandler.dump_traceback(all_threads=True)

class Waiter(Thread):
    # avoid blocking if the main thread raises an exception.
    daemon = True

    def __init__(self):
        Thread.__init__(self)
        self.running = Event()
        self.stop = Event()

    def run(self):
        self.running.set()
        self.stop.wait()

waiter = Waiter()
waiter.start()
waiter.running.wait()
dump()
waiter.stop.set()
waiter.join()
""".strip()
        code = code.format(filename=repr(filename))
        output, exitcode = self.get_output(code, filename)
        output = '\n'.join(output)
        if filename:
            lineno = 8
        else:
            lineno = 10
        regex = """
^Thread 0x[0-9a-f]+ \(most recent call first\):
(?:  File ".*threading.py", line [0-9]+ in [_a-z]+
){{1,3}}  File "<string>", line 23 in run
  File ".*threading.py", line [0-9]+ in _bootstrap_inner
  File ".*threading.py", line [0-9]+ in _bootstrap

Current thread XXX \(most recent call first\):
  File "<string>", line {lineno} in dump
  File "<string>", line 28 in <module>$
""".strip()
        regex = regex.format(lineno=lineno)
        self.assertRegex(output, regex)
        self.assertEqual(exitcode, 0)

    def test_dump_traceback_threads(self):
        self.check_dump_traceback_threads(None)

    def test_dump_traceback_threads_file(self):
        with temporary_filename() as filename:
            self.check_dump_traceback_threads(filename)

    def _check_dump_traceback_later(self, repeat, cancel, filename, loops):
        """
        Check how many times the traceback is written in timeout x 2.5 seconds,
        or timeout x 3.5 seconds if cancel is True: 1, 2 or 3 times depending
        on repeat and cancel options.

        Raise an error if the output doesn't match the expect format.
        """
        timeout_str = str(datetime.timedelta(seconds=TIMEOUT))
        code = """
import faulthandler
import time

def func(timeout, repeat, cancel, file, loops):
    for loop in range(loops):
        faulthandler.dump_traceback_later(timeout, repeat=repeat, file=file)
        if cancel:
            faulthandler.cancel_dump_traceback_later()
        time.sleep(timeout * 5)
        faulthandler.cancel_dump_traceback_later()

timeout = {timeout}
repeat = {repeat}
cancel = {cancel}
loops = {loops}
if {has_filename}:
    file = open({filename}, "wb")
else:
    file = None
func(timeout, repeat, cancel, file, loops)
if file is not None:
    file.close()
""".strip()
        code = code.format(
            timeout=TIMEOUT,
            repeat=repeat,
            cancel=cancel,
            loops=loops,
            has_filename=bool(filename),
            filename=repr(filename),
        )
        trace, exitcode = self.get_output(code, filename)
        trace = '\n'.join(trace)

        if not cancel:
            count = loops
            if repeat:
                count *= 2
            header = r'Timeout \(%s\)!\nThread 0x[0-9a-f]+ \(most recent call first\):\n' % timeout_str
            regex = expected_traceback(9, 20, header, min_count=count)
            self.assertRegex(trace, regex)
        else:
            self.assertEqual(trace, '')
        self.assertEqual(exitcode, 0)

    @unittest.skipIf(not hasattr(faulthandler, 'dump_traceback_later'),
                     'need faulthandler.dump_traceback_later()')
    def check_dump_traceback_later(self, repeat=False, cancel=False,
                                    file=False, twice=False):
        if twice:
            loops = 2
        else:
            loops = 1
        if file:
            with temporary_filename() as filename:
                self._check_dump_traceback_later(repeat, cancel,
                                                  filename, loops)
        else:
            self._check_dump_traceback_later(repeat, cancel, None, loops)

    def test_dump_traceback_later(self):
        self.check_dump_traceback_later()

    def test_dump_traceback_later_repeat(self):
        self.check_dump_traceback_later(repeat=True)

    def test_dump_traceback_later_cancel(self):
        self.check_dump_traceback_later(cancel=True)

    def test_dump_traceback_later_file(self):
        self.check_dump_traceback_later(file=True)

    def test_dump_traceback_later_twice(self):
        self.check_dump_traceback_later(twice=True)

    @unittest.skipIf(not hasattr(faulthandler, "register"),
                     "need faulthandler.register")
    def check_register(self, filename=False, all_threads=False,
                       unregister=False, chain=False):
        """
        Register a handler displaying the traceback on a user signal. Raise the
        signal and check the written traceback.

        If chain is True, check that the previous signal handler is called.

        Raise an error if the output doesn't match the expected format.
        """
        signum = signal.SIGUSR1
        code = """
import faulthandler
import os
import signal
import sys

def func(signum):
    os.kill(os.getpid(), signum)

def handler(signum, frame):
    handler.called = True
handler.called = False

exitcode = 0
signum = {signum}
unregister = {unregister}
chain = {chain}

if {has_filename}:
    file = open({filename}, "wb")
else:
    file = None
if chain:
    signal.signal(signum, handler)
faulthandler.register(signum, file=file,
                      all_threads={all_threads}, chain={chain})
if unregister:
    faulthandler.unregister(signum)
func(signum)
if chain and not handler.called:
    if file is not None:
        output = file
    else:
        output = sys.stderr
    print("Error: signal handler not called!", file=output)
    exitcode = 1
if file is not None:
    file.close()
sys.exit(exitcode)
""".strip()
        code = code.format(
            filename=repr(filename),
            has_filename=bool(filename),
            all_threads=all_threads,
            signum=signum,
            unregister=unregister,
            chain=chain,
        )
        trace, exitcode = self.get_output(code, filename)
        trace = '\n'.join(trace)
        if not unregister:
            if all_threads:
                regex = 'Current thread XXX \(most recent call first\):\n'
            else:
                regex = 'Stack \(most recent call first\):\n'
            regex = expected_traceback(7, 28, regex)
            self.assertRegex(trace, regex)
        else:
            self.assertEqual(trace, '')
        if unregister:
            self.assertNotEqual(exitcode, 0)
        else:
            self.assertEqual(exitcode, 0)

    def test_register(self):
        self.check_register()

    def test_unregister(self):
        self.check_register(unregister=True)

    def test_register_file(self):
        with temporary_filename() as filename:
            self.check_register(filename=filename)

    def test_register_threads(self):
        self.check_register(all_threads=True)

    def test_register_chain(self):
        self.check_register(chain=True)


if __name__ == "__main__":
    unittest.main()
