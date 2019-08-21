import collections
import faulthandler
import json
import os
import queue
import subprocess
import sys
import threading
import time
import traceback
import types
from test import support

from test.libregrtest.runtest import (
    runtest, INTERRUPTED, CHILD_ERROR, PROGRESS_MIN_TIME,
    format_test_result, TestResult, is_failed, TIMEOUT)
from test.libregrtest.setup import setup_tests
from test.libregrtest.utils import format_duration


# Display the running tests if nothing happened last N seconds
PROGRESS_UPDATE = 30.0   # seconds

# Time to wait until a worker completes: should be immediate
JOIN_TIMEOUT = 30.0   # seconds


def must_stop(result, ns):
    if result.result == INTERRUPTED:
        return True
    if ns.failfast and is_failed(result, ns):
        return True
    return False


def parse_worker_args(worker_args):
    ns_dict, test_name = json.loads(worker_args)
    ns = types.SimpleNamespace(**ns_dict)
    return (ns, test_name)


def run_test_in_subprocess(testname, ns):
    ns_dict = vars(ns)
    worker_args = (ns_dict, testname)
    worker_args = json.dumps(worker_args)

    cmd = [sys.executable, *support.args_from_interpreter_flags(),
           '-u',    # Unbuffered stdout and stderr
           '-m', 'test.regrtest',
           '--worker-args', worker_args]

    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    return subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True,
                            close_fds=(os.name != 'nt'),
                            cwd=support.SAVEDCWD)


def run_tests_worker(ns, test_name):
    setup_tests(ns)

    result = runtest(ns, test_name)

    print()   # Force a newline (just in case)

    # Serialize TestResult as list in JSON
    print(json.dumps(list(result)), flush=True)
    sys.exit(0)


# We do not use a generator so multiple threads can call next().
class MultiprocessIterator:

    """A thread-safe iterator over tests for multiprocess mode."""

    def __init__(self, tests_iter):
        self.lock = threading.Lock()
        self.tests_iter = tests_iter

    def __iter__(self):
        return self

    def __next__(self):
        with self.lock:
            if self.tests_iter is None:
                raise StopIteration
            return next(self.tests_iter)

    def stop(self):
        with self.lock:
            self.tests_iter = None


MultiprocessResult = collections.namedtuple('MultiprocessResult',
    'result stdout stderr error_msg')

class ExitThread(Exception):
    pass


class MultiprocessThread(threading.Thread):
    def __init__(self, pending, output, ns, timeout):
        super().__init__()
        self.pending = pending
        self.output = output
        self.ns = ns
        self.timeout = timeout
        self.current_test_name = None
        self.start_time = None
        self._popen = None
        self._killed = False

    def __repr__(self):
        info = ['MultiprocessThread']
        test = self.current_test_name
        if self.is_alive():
            info.append('alive')
        if test:
            info.append(f'test={test}')
        popen = self._popen
        if popen:
            info.append(f'pid={popen.pid}')
        return '<%s>' % ' '.join(info)

    def _kill(self):
        dt = time.monotonic() - self.start_time

        popen = self._popen
        pid = popen.pid
        print("Kill worker process %s running for %.1f sec" % (pid, dt),
              file=sys.stderr, flush=True)

        try:
            popen.kill()
            return True
        except OSError as exc:
            print("WARNING: Failed to kill worker process %s: %r" % (pid, exc),
                  file=sys.stderr, flush=True)
            return False

    def _close_wait(self):
        popen = self._popen

        # stdout and stderr must be closed to ensure that communicate()
        # does not hang
        popen.stdout.close()
        popen.stderr.close()

        try:
            popen.wait(JOIN_TIMEOUT)
        except (subprocess.TimeoutExpired, OSError) as exc:
            print("WARNING: Failed to wait for worker process %s "
                  "completion (timeout=%.1f sec): %r"
                  % (popen.pid, JOIN_TIMEOUT, exc),
                  file=sys.stderr, flush=True)

    def kill(self):
        """
        Kill the current process (if any).

        This method can be called by the thread running the process,
        or by another thread.
        """
        self._killed = True

        if self._popen is None:
            return

        if not self._kill():
            return

        self._close_wait()

    def mp_result_error(self, test_name, error_type, stdout='', stderr='',
                        err_msg=None):
        test_time = time.monotonic() - self.start_time
        result = TestResult(test_name, error_type, test_time, None)
        return MultiprocessResult(result, stdout, stderr, err_msg)

    def _timedout(self, test_name):
        self._kill()

        stdout = sterr = ''
        popen = self._popen
        try:
            stdout, stderr = popen.communicate(timeout=JOIN_TIMEOUT)
        except (subprocess.TimeoutExpired, OSError) as exc:
            print("WARNING: Failed to read worker process %s output "
                  "(timeout=%.1f sec): %r"
                  % (popen.pid, exc, timeout),
                  file=sys.stderr, flush=True)

        self._close_wait()

        return self.mp_result_error(test_name, TIMEOUT, stdout, stderr)

    def _runtest(self, test_name):
        try:
            self.start_time = time.monotonic()
            self.current_test_name = test_name

            self._popen = run_test_in_subprocess(test_name, self.ns)
            popen = self._popen
            try:
                try:
                    if self._killed:
                        # If kill() has been called before self._popen is set,
                        # self._popen is still running. Call again kill()
                        # to ensure that the process is killed.
                        self.kill()
                        raise ExitThread

                    try:
                        stdout, stderr = popen.communicate(timeout=self.timeout)
                    except subprocess.TimeoutExpired:
                        if self._killed:
                            # kill() has been called: communicate() fails
                            # on reading closed stdout/stderr
                            raise ExitThread

                        return self._timedout(test_name)
                    except OSError:
                        if self._killed:
                            # kill() has been called: communicate() fails
                            # on reading closed stdout/stderr
                            raise ExitThread
                        raise
                except:
                    self.kill()
                    raise
            finally:
                self._close_wait()

            retcode = popen.returncode
        finally:
            self.current_test_name = None
            self._popen = None

        stdout = stdout.strip()
        stderr = stderr.rstrip()

        err_msg = None
        if retcode != 0:
            err_msg = "Exit code %s" % retcode
        else:
            stdout, _, result = stdout.rpartition("\n")
            stdout = stdout.rstrip()
            if not result:
                err_msg = "Failed to parse worker stdout"
            else:
                try:
                    # deserialize run_tests_worker() output
                    result = json.loads(result)
                    result = TestResult(*result)
                except Exception as exc:
                    err_msg = "Failed to parse worker JSON: %s" % exc

        if err_msg is not None:
            return self.mp_result_error(test_name, CHILD_ERROR, stdout, stderr, err_msg)

        return MultiprocessResult(result, stdout, stderr, err_msg)

    def run(self):
        while not self._killed:
            try:
                try:
                    test_name = next(self.pending)
                except StopIteration:
                    break

                mp_result = self._runtest(test_name)
                self.output.put((False, mp_result))

                if must_stop(mp_result.result, self.ns):
                    break
            except ExitThread:
                break
            except BaseException:
                self.output.put((True, traceback.format_exc()))
                break


def get_running(workers):
    running = []
    for worker in workers:
        current_test_name = worker.current_test_name
        if not current_test_name:
            continue
        dt = time.monotonic() - worker.start_time
        if dt >= PROGRESS_MIN_TIME:
            text = '%s (%s)' % (current_test_name, format_duration(dt))
            running.append(text)
    return running


class MultiprocessRunner:
    def __init__(self, regrtest):
        self.regrtest = regrtest
        self.ns = regrtest.ns
        self.output = queue.Queue()
        self.pending = MultiprocessIterator(self.regrtest.tests)
        if self.ns.timeout is not None:
            self.worker_timeout = self.ns.timeout * 1.5
            self.main_timeout = self.ns.timeout * 2.0
        else:
            self.worker_timeout = None
            self.main_timeout = None
        self.workers = None

    def start_workers(self):
        self.workers = [MultiprocessThread(self.pending, self.output,
                                           self.ns, self.worker_timeout)
                        for _ in range(self.ns.use_mp)]
        print("Run tests in parallel using %s child processes"
              % len(self.workers))
        for worker in self.workers:
            worker.start()

    def wait_workers(self):
        start_time = time.monotonic()
        for worker in self.workers:
            worker.kill()
        for worker in self.workers:
            while True:
                worker.join(1.0)
                if not worker.is_alive():
                    break
                dt = time.monotonic() - start_time
                print("Wait for regrtest worker %r for %.1f sec" % (worker, dt),
                      flush=True)
                if dt > JOIN_TIMEOUT:
                    print("Warning -- failed to join a regrtest worker %s"
                          % worker, flush=True)
                    break

    def _get_result(self):
        if not any(worker.is_alive() for worker in self.workers):
            # all worker threads are done: consume pending results
            try:
                return self.output.get(timeout=0)
            except queue.Empty:
                return None

        while True:
            if self.main_timeout is not None:
                faulthandler.dump_traceback_later(self.main_timeout, exit=True)

            # wait for a thread
            timeout = max(PROGRESS_UPDATE, PROGRESS_MIN_TIME)
            try:
                return self.output.get(timeout=timeout)
            except queue.Empty:
                pass

            # display progress
            running = get_running(self.workers)
            if running and not self.ns.pgo:
                print('running: %s' % ', '.join(running), flush=True)

    def display_result(self, mp_result):
        result = mp_result.result

        text = format_test_result(result)
        if mp_result.error_msg is not None:
            # CHILD_ERROR
            text += ' (%s)' % mp_result.error_msg
        elif (result.test_time >= PROGRESS_MIN_TIME and not self.ns.pgo):
            text += ' (%s)' % format_duration(result.test_time)
        running = get_running(self.workers)
        if running and not self.ns.pgo:
            text += ' -- running: %s' % ', '.join(running)
        self.regrtest.display_progress(self.test_index, text)

    def _process_result(self, item):
        if item[0]:
            # Thread got an exception
            format_exc = item[1]
            print(f"regrtest worker thread failed: {format_exc}",
                  file=sys.stderr, flush=True)
            return True

        self.test_index += 1
        mp_result = item[1]
        self.regrtest.accumulate_result(mp_result.result)
        self.display_result(mp_result)

        if mp_result.stdout:
            print(mp_result.stdout, flush=True)
        if mp_result.stderr and not self.ns.pgo:
            print(mp_result.stderr, file=sys.stderr, flush=True)

        if must_stop(mp_result.result, self.ns):
            return True

        return False

    def run_tests(self):
        self.start_workers()

        self.test_index = 0
        try:
            while True:
                item = self._get_result()
                if item is None:
                    break

                stop = self._process_result(item)
                if stop:
                    break
        except KeyboardInterrupt:
            print()
            self.regrtest.interrupted = True
        finally:
            if self.main_timeout is not None:
                faulthandler.cancel_dump_traceback_later()

        # a test failed (and --failfast is set) or all tests completed
        self.pending.stop()
        self.wait_workers()


def run_tests_multiprocess(regrtest):
    MultiprocessRunner(regrtest).run_tests()
