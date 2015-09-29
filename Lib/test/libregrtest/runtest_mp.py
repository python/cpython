import json
import os
import re
import sys
import traceback
import unittest
from queue import Queue
from test import support
try:
    import threading
except ImportError:
    print("Multiprocess option requires thread support")
    sys.exit(2)

from test.libregrtest.runtest import runtest, INTERRUPTED, CHILD_ERROR


debug_output_pat = re.compile(r"\[\d+ refs, \d+ blocks\]$")


def run_tests_in_subprocess(testname, ns):
    """Run the given test in a subprocess with --slaveargs.

    ns is the option Namespace parsed from command-line arguments. regrtest
    is invoked in a subprocess with the --slaveargs argument; when the
    subprocess exits, its return code, stdout and stderr are returned as a
    3-tuple.
    """
    from subprocess import Popen, PIPE
    base_cmd = ([sys.executable] + support.args_from_interpreter_flags() +
                ['-X', 'faulthandler', '-m', 'test.regrtest'])

    slaveargs = (
            (testname, ns.verbose, ns.quiet),
            dict(huntrleaks=ns.huntrleaks,
                 use_resources=ns.use_resources,
                 output_on_failure=ns.verbose3,
                 timeout=ns.timeout, failfast=ns.failfast,
                 match_tests=ns.match_tests))
    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    popen = Popen(base_cmd + ['--slaveargs', json.dumps(slaveargs)],
                  stdout=PIPE, stderr=PIPE,
                  universal_newlines=True,
                  close_fds=(os.name != 'nt'),
                  cwd=support.SAVEDCWD)
    stdout, stderr = popen.communicate()
    retcode = popen.wait()
    return retcode, stdout, stderr


def run_tests_slave(slaveargs):
    args, kwargs = json.loads(slaveargs)
    if kwargs.get('huntrleaks'):
        unittest.BaseTestSuite._cleanup = False
    try:
        result = runtest(*args, **kwargs)
    except KeyboardInterrupt:
        result = INTERRUPTED, ''
    except BaseException as e:
        traceback.print_exc()
        result = CHILD_ERROR, str(e)
    sys.stdout.flush()
    print()   # Force a newline (just in case)
    print(json.dumps(result))
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

    def run(self):
        # A worker thread.
        try:
            while True:
                try:
                    test = next(self.pending)
                except StopIteration:
                    self.output.put((None, None, None, None))
                    return
                retcode, stdout, stderr = run_tests_in_subprocess(test,
                                                                  self.ns)
                # Strip last refcount output line if it exists, since it
                # comes from the shutdown of the interpreter in the subcommand.
                stderr = debug_output_pat.sub("", stderr)
                stdout, _, result = stdout.strip().rpartition("\n")
                if retcode != 0:
                    result = (CHILD_ERROR, "Exit code %s" % retcode)
                    self.output.put((test, stdout.rstrip(), stderr.rstrip(),
                                     result))
                    return
                if not result:
                    self.output.put((None, None, None, None))
                    return
                result = json.loads(result)
                self.output.put((test, stdout.rstrip(), stderr.rstrip(),
                                 result))
        except BaseException:
            self.output.put((None, None, None, None))
            raise


def run_tests_multiprocess(regrtest):
    output = Queue()
    pending = MultiprocessIterator(regrtest.tests)

    workers = [MultiprocessThread(pending, output, regrtest.ns)
               for i in range(regrtest.ns.use_mp)]
    for worker in workers:
        worker.start()
    finished = 0
    test_index = 1
    try:
        while finished < regrtest.ns.use_mp:
            test, stdout, stderr, result = output.get()
            if test is None:
                finished += 1
                continue
            regrtest.accumulate_result(test, result)
            regrtest.display_progress(test_index, test)
            if stdout:
                print(stdout)
            if stderr:
                print(stderr, file=sys.stderr)
            sys.stdout.flush()
            sys.stderr.flush()
            if result[0] == INTERRUPTED:
                raise KeyboardInterrupt
            if result[0] == CHILD_ERROR:
                msg = "Child error on {}: {}".format(test, result[1])
                raise Exception(msg)
            test_index += 1
    except KeyboardInterrupt:
        regrtest.interrupted = True
        pending.interrupted = True
    for worker in workers:
        worker.join()
