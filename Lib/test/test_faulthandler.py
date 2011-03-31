from contextlib import contextmanager
import faulthandler
import re
import signal
import subprocess
import sys
from test import support, script_helper
import tempfile
import unittest

try:
    from resource import setrlimit, RLIMIT_CORE, error as resource_error
except ImportError:
    prepare_subprocess = None
else:
    def prepare_subprocess():
        # don't create core file
        try:
            setrlimit(RLIMIT_CORE, (0, 0))
        except (ValueError, resource_error):
            pass

def expected_traceback(lineno1, lineno2, header, count=1):
    regex = header
    regex += r'  File "\<string\>", line %s in func\n' % lineno1
    regex += r'  File "\<string\>", line %s in \<module\>' % lineno2
    if count != 1:
        regex = (regex + '\n') * (count - 1) + regex
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
        options = {}
        if prepare_subprocess:
            options['preexec_fn'] = prepare_subprocess
        process = script_helper.spawn_python('-c', code, **options)
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
                          filename=None, all_threads=False, other_regex=None):
        """
        Check that the fault handler for fatal errors is enabled and check the
        traceback from the child process output.

        Raise an error if the output doesn't match the expected format.
        """
        if all_threads:
            header = 'Current thread XXX'
        else:
            header = 'Traceback (most recent call first)'
        regex = """
^Fatal Python error: {name}

{header}:
  File "<string>", line {lineno} in <module>$
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

    def test_read_null(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._read_null()
""".strip(),
            3,
            '(?:Segmentation fault|Bus error)')

    def test_sigsegv(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable()
faulthandler._sigsegv()
""".strip(),
            3,
            'Segmentation fault')

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
            '(?:Segmentation fault|Bus error)')

    def test_enable_file(self):
        with temporary_filename() as filename:
            self.check_fatal_error("""
import faulthandler
output = open({filename}, 'wb')
faulthandler.enable(output)
faulthandler._read_null(True)
""".strip().format(filename=repr(filename)),
                4,
                '(?:Segmentation fault|Bus error)',
                filename=filename)

    def test_enable_threads(self):
        self.check_fatal_error("""
import faulthandler
faulthandler.enable(all_threads=True)
faulthandler._read_null(True)
""".strip(),
            3,
            '(?:Segmentation fault|Bus error)',
            all_threads=True)

    def test_disable(self):
        code = """
import faulthandler
faulthandler.enable()
faulthandler.disable()
faulthandler._read_null()
""".strip()
        not_expected = 'Fatal Python error'
        stderr, exitcode = self.get_output(code)
        stder = '\n'.join(stderr)
        self.assertTrue(not_expected not in stderr,
                     "%r is present in %r" % (not_expected, stderr))
        self.assertNotEqual(exitcode, 0)

    def test_is_enabled(self):
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
            faulthandler.dump_traceback(fp)
    else:
        faulthandler.dump_traceback()

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
            'Traceback (most recent call first):',
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
^Thread 0x[0-9a-f]+:
(?:  File ".*threading.py", line [0-9]+ in wait
)?  File ".*threading.py", line [0-9]+ in wait
  File "<string>", line 23 in run
  File ".*threading.py", line [0-9]+ in _bootstrap_inner
  File ".*threading.py", line [0-9]+ in _bootstrap

Current thread XXX:
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

    def _check_dump_tracebacks_later(self, repeat, cancel, filename):
        """
        Check how many times the traceback is written in timeout x 2.5 seconds,
        or timeout x 3.5 seconds if cancel is True: 1, 2 or 3 times depending
        on repeat and cancel options.

        Raise an error if the output doesn't match the expect format.
        """
        code = """
import faulthandler
import time

def func(repeat, cancel, timeout):
    pause = timeout * 2.5
    a = time.time()
    time.sleep(pause)
    faulthandler.cancel_dump_tracebacks_later()
    b = time.time()
    # Check that sleep() was not interrupted
    assert (b -a) >= pause

    if cancel:
        pause = timeout * 1.5
        a = time.time()
        time.sleep(pause)
        b = time.time()
        # Check that sleep() was not interrupted
        assert (b -a) >= pause

timeout = 0.5
repeat = {repeat}
cancel = {cancel}
if {has_filename}:
    file = open({filename}, "wb")
else:
    file = None
faulthandler.dump_tracebacks_later(timeout,
    repeat=repeat, file=file)
func(repeat, cancel, timeout)
if file is not None:
    file.close()
""".strip()
        code = code.format(
            filename=repr(filename),
            has_filename=bool(filename),
            repeat=repeat,
            cancel=cancel,
        )
        trace, exitcode = self.get_output(code, filename)
        trace = '\n'.join(trace)

        if repeat:
            count = 2
        else:
            count = 1
        header = 'Thread 0x[0-9a-f]+:\n'
        regex = expected_traceback(7, 30, header, count=count)
        self.assertRegex(trace, '^%s$' % regex)
        self.assertEqual(exitcode, 0)

    @unittest.skipIf(not hasattr(faulthandler, 'dump_tracebacks_later'),
                     'need faulthandler.dump_tracebacks_later()')
    def check_dump_tracebacks_later(self, repeat=False, cancel=False,
                                  file=False):
        if file:
            with temporary_filename() as filename:
                self._check_dump_tracebacks_later(repeat, cancel, filename)
        else:
            self._check_dump_tracebacks_later(repeat, cancel, None)

    def test_dump_tracebacks_later(self):
        self.check_dump_tracebacks_later()

    def test_dump_tracebacks_later_repeat(self):
        self.check_dump_tracebacks_later(repeat=True)

    def test_dump_tracebacks_later_repeat_cancel(self):
        self.check_dump_tracebacks_later(repeat=True, cancel=True)

    def test_dump_tracebacks_later_file(self):
        self.check_dump_tracebacks_later(file=True)

    @unittest.skipIf(not hasattr(faulthandler, "register"),
                     "need faulthandler.register")
    def check_register(self, filename=False, all_threads=False):
        """
        Register a handler displaying the traceback on a user signal. Raise the
        signal and check the written traceback.

        Raise an error if the output doesn't match the expected format.
        """
        code = """
import faulthandler
import os
import signal

def func(signum):
    os.kill(os.getpid(), signum)

signum = signal.SIGUSR1
if {has_filename}:
    file = open({filename}, "wb")
else:
    file = None
faulthandler.register(signum, file=file, all_threads={all_threads})
func(signum)
if file is not None:
    file.close()
""".strip()
        code = code.format(
            filename=repr(filename),
            has_filename=bool(filename),
            all_threads=all_threads,
        )
        trace, exitcode = self.get_output(code, filename)
        trace = '\n'.join(trace)
        if all_threads:
            regex = 'Current thread XXX:\n'
        else:
            regex = 'Traceback \(most recent call first\):\n'
        regex = expected_traceback(6, 14, regex)
        self.assertRegex(trace, '^%s$' % regex)
        self.assertEqual(exitcode, 0)

    def test_register(self):
        self.check_register()

    def test_register_file(self):
        with temporary_filename() as filename:
            self.check_register(filename=filename)

    def test_register_threads(self):
        self.check_register(all_threads=True)


def test_main():
    support.run_unittest(FaultHandlerTests)

if __name__ == "__main__":
    test_main()
