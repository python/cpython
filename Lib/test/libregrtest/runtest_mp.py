import faulthandler
import json
import os
import queue
import sys
import threading
import time
import traceback
import types
from test import support

from test.libregrtest.runtest import (
    runtest, INTERRUPTED, CHILD_ERROR, PROGRESS_MIN_TIME,
    format_test_result)
from test.libregrtest.setup import setup_tests
from test.libregrtest.utils import format_duration


# Display the running tests if nothing happened last N seconds
PROGRESS_UPDATE = 30.0   # seconds

# If interrupted, display the wait progress every N seconds
WAIT_PROGRESS = 2.0   # seconds


def run_test_in_subprocess(testname, ns):
    """Run the given test in a subprocess with --slaveargs.

    ns is the option Namespace parsed from command-line arguments. regrtest
    is invoked in a subprocess with the --slaveargs argument; when the
    subprocess exits, its return code, stdout and stderr are returned as a
    3-tuple.
    """
    from subprocess import Popen, PIPE

    ns_dict = vars(ns)
    slaveargs = (ns_dict, testname)
    slaveargs = json.dumps(slaveargs)

    cmd = [sys.executable, *support.args_from_interpreter_flags(),
           '-u',    # Unbuffered stdout and stderr
           '-m', 'test.regrtest',
           '--slaveargs', slaveargs]
    if ns.pgo:
        cmd += ['--pgo']

    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    popen = Popen(cmd,
                  stdout=PIPE, stderr=PIPE,
                  universal_newlines=True,
                  close_fds=(os.name != 'nt'),
                  cwd=support.SAVEDCWD)
    with popen:
        stdout, stderr = popen.communicate()
        retcode = popen.wait()
    return retcode, stdout, stderr


def run_tests_slave(slaveargs):
    ns_dict, testname = json.loads(slaveargs)
    ns = types.SimpleNamespace(**ns_dict)

    setup_tests(ns)

    try:
        result = runtest(ns, testname)
    except KeyboardInterrupt:
        result = INTERRUPTED, '', None
    except BaseException as e:
        traceback.print_exc()
        result = CHILD_ERROR, str(e)

    print()   # Force a newline (just in case)
    print(json.dumps(result), flush=True)
    sys.exit(0)


# We do not use a generator so multiple threads can call next().
class MultiprocessIterator:

    """A thread-safe iterator over tests for multiprocess mode."""

    def __init__(self, tests):
        self.interrupted = False
        self.lock = threading.Lock()
        self.tests = tests

    def __iter__(self):
        return self

    def __next__(self):
        with self.lock:
            if self.interrupted:
                raise StopIteration('tests interrupted')
            return next(self.tests)


class MultiprocessThread(threading.Thread):
    def __init__(self, pending, output, ns):
        super().__init__()
        self.pending = pending
        self.output = output
        self.ns = ns
        self.current_test = None
        self.start_time = None

    def _runtest(self):
        try:
            test = next(self.pending)
        except StopIteration:
            self.output.put((None, None, None, None))
            return True

        try:
            self.start_time = time.monotonic()
            self.current_test = test

            retcode, stdout, stderr = run_test_in_subprocess(test, self.ns)
        finally:
            self.current_test = None

        if retcode != 0:
            result = (CHILD_ERROR, "Exit code %s" % retcode, None)
            self.output.put((test, stdout.rstrip(), stderr.rstrip(),
                             result))
            return False

        stdout, _, result = stdout.strip().rpartition("\n")
        if not result:
            self.output.put((None, None, None, None))
            return True

        result = json.loads(result)
        assert len(result) == 3, f"Invalid result tuple: {result!r}"
        self.output.put((test, stdout.rstrip(), stderr.rstrip(),
                         result))
        return False

    def run(self):
        try:
            stop = False
            while not stop:
                stop = self._runtest()
        except BaseException:
            self.output.put((None, None, None, None))
            raise


def run_tests_multiprocess(regrtest):
    output = queue.Queue()
    pending = MultiprocessIterator(regrtest.tests)
    test_timeout = regrtest.ns.timeout
    use_timeout = (test_timeout is not None)

    workers = [MultiprocessThread(pending, output, regrtest.ns)
               for i in range(regrtest.ns.use_mp)]
    print("Run tests in parallel using %s child processes"
          % len(workers))
    for worker in workers:
        worker.start()

    def get_running(workers):
        running = []
        for worker in workers:
            current_test = worker.current_test
            if not current_test:
                continue
            dt = time.monotonic() - worker.start_time
            if dt >= PROGRESS_MIN_TIME:
                text = '%s (%s)' % (current_test, format_duration(dt))
                running.append(text)
        return running

    finished = 0
    test_index = 1
    get_timeout = max(PROGRESS_UPDATE, PROGRESS_MIN_TIME)
    try:
        while finished < regrtest.ns.use_mp:
            if use_timeout:
                faulthandler.dump_traceback_later(test_timeout, exit=True)

            try:
                item = output.get(timeout=get_timeout)
            except queue.Empty:
                running = get_running(workers)
                if running and not regrtest.ns.pgo:
                    print('running: %s' % ', '.join(running), flush=True)
                continue

            test, stdout, stderr, result = item
            if test is None:
                finished += 1
                continue
            regrtest.accumulate_result(test, result)

            # Display progress
            ok, test_time, xml_data = result
            text = format_test_result(test, ok)
            if (ok not in (CHILD_ERROR, INTERRUPTED)
                and test_time >= PROGRESS_MIN_TIME
                and not regrtest.ns.pgo):
                text += ' (%s)' % format_duration(test_time)
            elif ok == CHILD_ERROR:
                text = '%s (%s)' % (text, test_time)
            running = get_running(workers)
            if running and not regrtest.ns.pgo:
                text += ' -- running: %s' % ', '.join(running)
            regrtest.display_progress(test_index, text)

            # Copy stdout and stderr from the child process
            if stdout:
                print(stdout, flush=True)
            if stderr and not regrtest.ns.pgo:
                print(stderr, file=sys.stderr, flush=True)

            if result[0] == INTERRUPTED:
                raise KeyboardInterrupt
            test_index += 1
    except KeyboardInterrupt:
        regrtest.interrupted = True
        pending.interrupted = True
        print()
    finally:
        if use_timeout:
            faulthandler.cancel_dump_traceback_later()

    # If tests are interrupted, wait until tests complete
    wait_start = time.monotonic()
    while True:
        running = [worker.current_test for worker in workers]
        running = list(filter(bool, running))
        if not running:
            break

        dt = time.monotonic() - wait_start
        line = "Waiting for %s (%s tests)" % (', '.join(running), len(running))
        if dt >= WAIT_PROGRESS:
            line = "%s since %.0f sec" % (line, dt)
        print(line, flush=True)
        for worker in workers:
            worker.join(WAIT_PROGRESS)
