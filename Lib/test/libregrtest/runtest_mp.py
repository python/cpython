import collections
import faulthandler
import json
import os
import queue
import signal
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
from test.libregrtest.utils import format_duration, print_warning


# Display the running tests if nothing happened last N seconds
PROGRESS_UPDATE = 30.0   # seconds
assert PROGRESS_UPDATE >= PROGRESS_MIN_TIME

# Kill the main process after 5 minutes. It is supposed to write an update
# every PROGRESS_UPDATE seconds. Tolerate 5 minutes for Python slowest
# buildbot workers.
MAIN_PROCESS_TIMEOUT = 5 * 60.0
assert MAIN_PROCESS_TIMEOUT >= PROGRESS_UPDATE

# Time to wait until a worker completes: should be immediate
JOIN_TIMEOUT = 30.0   # seconds

USE_PROCESS_GROUP = (hasattr(os, "setsid") and hasattr(os, "killpg"))


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
    kw = {}
    if USE_PROCESS_GROUP:
        kw['start_new_session'] = True
    return subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True,
                            close_fds=(os.name != 'nt'),
                            cwd=support.SAVEDCWD,
                            **kw)


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


class TestWorkerProcess(threading.Thread):
    def __init__(self, worker_id, runner):
        super().__init__()
        self.worker_id = worker_id
        self.pending = runner.pending
        self.output = runner.output
        self.ns = runner.ns
        self.timeout = runner.worker_timeout
        self.regrtest = runner.regrtest
        self.current_test_name = None
        self.start_time = None
        self._popen = None
        self._killed = False
        self._stopped = False

    def __repr__(self):
        info = [f'TestWorkerProcess #{self.worker_id}']
        if self.is_alive():
            info.append("running")
        else:
            info.append('stopped')
        test = self.current_test_name
        if test:
            info.append(f'test={test}')
        popen = self._popen
        if popen is not None:
            dt = time.monotonic() - self.start_time
            info.extend((f'pid={self._popen.pid}',
                         f'time={format_duration(dt)}'))
        return '<%s>' % ' '.join(info)

    def _kill(self):
        popen = self._popen
        if popen is None:
            return

        if self._killed:
            return
        self._killed = True

        if USE_PROCESS_GROUP:
            what = f"{self} process group"
        else:
            what = f"{self}"

        print(f"Kill {what}", file=sys.stderr, flush=True)
        try:
            if USE_PROCESS_GROUP:
                os.killpg(popen.pid, signal.SIGKILL)
            else:
                popen.kill()
        except ProcessLookupError:
            # popen.kill(): the process completed, the TestWorkerProcess thread
            # read its exit status, but Popen.send_signal() read the returncode
            # just before Popen.wait() set returncode.
            pass
        except OSError as exc:
            print_warning(f"Failed to kill {what}: {exc!r}")

    def stop(self):
        # Method called from a different thread to stop this thread
        self._stopped = True
        self._kill()

    def mp_result_error(self, test_name, error_type, stdout='', stderr='',
                        err_msg=None):
        test_time = time.monotonic() - self.start_time
        result = TestResult(test_name, error_type, test_time, None)
        return MultiprocessResult(result, stdout, stderr, err_msg)

    def _run_process(self, test_name):
        self.start_time = time.monotonic()

        self.current_test_name = test_name
        try:
            popen = run_test_in_subprocess(test_name, self.ns)

            self._killed = False
            self._popen = popen
        except:
            self.current_test_name = None
            raise

        try:
            if self._stopped:
                # If kill() has been called before self._popen is set,
                # self._popen is still running. Call again kill()
                # to ensure that the process is killed.
                self._kill()
                raise ExitThread

            try:
                stdout, stderr = popen.communicate(timeout=self.timeout)
                retcode = popen.returncode
                assert retcode is not None
            except subprocess.TimeoutExpired:
                if self._stopped:
                    # kill() has been called: communicate() fails
                    # on reading closed stdout/stderr
                    raise ExitThread

                # On timeout, kill the process
                self._kill()

                # None means TIMEOUT for the caller
                retcode = None
                # bpo-38207: Don't attempt to call communicate() again: on it
                # can hang until all child processes using stdout and stderr
                # pipes completes.
                stdout = stderr = ''
            except OSError:
                if self._stopped:
                    # kill() has been called: communicate() fails
                    # on reading closed stdout/stderr
                    raise ExitThread
                raise
            else:
                stdout = stdout.strip()
                stderr = stderr.rstrip()

            return (retcode, stdout, stderr)
        except:
            self._kill()
            raise
        finally:
            self._wait_completed()
            self._popen = None
            self.current_test_name = None

    def _runtest(self, test_name):
        retcode, stdout, stderr = self._run_process(test_name)

        if retcode is None:
            return self.mp_result_error(test_name, TIMEOUT, stdout, stderr)

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
            return self.mp_result_error(test_name, CHILD_ERROR,
                                        stdout, stderr, err_msg)

        return MultiprocessResult(result, stdout, stderr, err_msg)

    def run(self):
        while not self._stopped:
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

    def _wait_completed(self):
        popen = self._popen

        # stdout and stderr must be closed to ensure that communicate()
        # does not hang
        popen.stdout.close()
        popen.stderr.close()

        try:
            popen.wait(JOIN_TIMEOUT)
        except (subprocess.TimeoutExpired, OSError) as exc:
            print_warning(f"Failed to wait for {self} completion "
                          f"(timeout={format_duration(JOIN_TIMEOUT)}): "
                          f"{exc!r}")

    def wait_stopped(self, start_time):
        # bpo-38207: MultiprocessTestRunner.stop_workers() called self.stop()
        # which killed the process. Sometimes, killing the process from the
        # main thread does not interrupt popen.communicate() in
        # TestWorkerProcess thread. This loop with a timeout is a workaround
        # for that.
        #
        # Moreover, if this method fails to join the thread, it is likely
        # that Python will hang at exit while calling threading._shutdown()
        # which tries again to join the blocked thread. Regrtest.main()
        # uses EXIT_TIMEOUT to workaround this second bug.
        while True:
            # Write a message every second
            self.join(1.0)
            if not self.is_alive():
                break
            dt = time.monotonic() - start_time
            self.regrtest.log(f"Waiting for {self} thread "
                              f"for {format_duration(dt)}")
            if dt > JOIN_TIMEOUT:
                print_warning(f"Failed to join {self} in {format_duration(dt)}")
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


class MultiprocessTestRunner:
    def __init__(self, regrtest):
        self.regrtest = regrtest
        self.log = self.regrtest.log
        self.ns = regrtest.ns
        self.output = queue.Queue()
        self.pending = MultiprocessIterator(self.regrtest.tests)
        if self.ns.timeout is not None:
            # Rely on faulthandler to kill a worker process. This timouet is
            # when faulthandler fails to kill a worker process. Give a maximum
            # of 5 minutes to faulthandler to kill the worker.
            self.worker_timeout = min(self.ns.timeout * 1.5,
                                      self.ns.timeout + 5 * 60)
        else:
            self.worker_timeout = None
        self.workers = None

    def start_workers(self):
        self.workers = [TestWorkerProcess(index, self)
                        for index in range(1, self.ns.use_mp + 1)]
        msg = f"Run tests in parallel using {len(self.workers)} child processes"
        if self.ns.timeout:
            msg += (" (timeout: %s, worker timeout: %s)"
                    % (format_duration(self.ns.timeout),
                       format_duration(self.worker_timeout)))
        self.log(msg)
        for worker in self.workers:
            worker.start()

    def stop_workers(self):
        start_time = time.monotonic()
        for worker in self.workers:
            worker.stop()
        for worker in self.workers:
            worker.wait_stopped(start_time)

    def _get_result(self):
        if not any(worker.is_alive() for worker in self.workers):
            # all worker threads are done: consume pending results
            try:
                return self.output.get(timeout=0)
            except queue.Empty:
                return None

        use_faulthandler = (self.ns.timeout is not None)
        timeout = PROGRESS_UPDATE
        while True:
            if use_faulthandler:
                faulthandler.dump_traceback_later(MAIN_PROCESS_TIMEOUT,
                                                  exit=True)

            # wait for a thread
            try:
                return self.output.get(timeout=timeout)
            except queue.Empty:
                pass

            # display progress
            running = get_running(self.workers)
            if running and not self.ns.pgo:
                self.log('running: %s' % ', '.join(running))

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
            print_warning(f"regrtest worker thread failed: {format_exc}")
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
            if self.ns.timeout is not None:
                faulthandler.cancel_dump_traceback_later()

            # Always ensure that all worker processes are no longer
            # worker when we exit this function
            self.pending.stop()
            self.stop_workers()


def run_tests_multiprocess(regrtest):
    MultiprocessTestRunner(regrtest).run_tests()
