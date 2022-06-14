import faulthandler
import json
import os.path
import queue
import shlex
import signal
import subprocess
import sys
import threading
import time
import traceback
from typing import NamedTuple, NoReturn, Literal, Any

from test import support
from test.support import os_helper

from test.libregrtest.cmdline import Namespace
from test.libregrtest.main import Regrtest
from test.libregrtest.runtest import (
    runtest, is_failed, TestResult, Interrupted, Timeout, ChildError,
    PROGRESS_MIN_TIME, Passed, EnvChanged)
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


def must_stop(result: TestResult, ns: Namespace) -> bool:
    if isinstance(result, Interrupted):
        return True
    if ns.failfast and is_failed(result, ns):
        return True
    return False


def parse_worker_args(worker_args) -> tuple[Namespace, str]:
    ns_dict, test_name = json.loads(worker_args)
    ns = Namespace(**ns_dict)
    return (ns, test_name)


def run_test_in_subprocess(testname: str, ns: Namespace, tmp_dir: str) -> subprocess.Popen:
    ns_dict = vars(ns)
    worker_args = (ns_dict, testname)
    worker_args = json.dumps(worker_args)
    if ns.python is not None:
        # The "executable" may be two or more parts, e.g. "node python.js"
        executable = shlex.split(ns.python)
    else:
        executable = [sys.executable]
    cmd = [*executable, *support.args_from_interpreter_flags(),
           '-u',    # Unbuffered stdout and stderr
           '-m', 'test.regrtest',
           '--worker-args', worker_args]

    env = dict(os.environ)
    env['TMPDIR'] = tmp_dir
    env['TEMPDIR'] = tmp_dir

    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    kw = {'env': env}
    if USE_PROCESS_GROUP:
        kw['start_new_session'] = True
    return subprocess.Popen(cmd,
                            stdout=subprocess.PIPE,
                            # bpo-45410: Write stderr into stdout to keep
                            # messages order
                            stderr=subprocess.STDOUT,
                            universal_newlines=True,
                            close_fds=(os.name != 'nt'),
                            cwd=os_helper.SAVEDCWD,
                            **kw)


def run_tests_worker(ns: Namespace, test_name: str) -> NoReturn:
    setup_tests(ns)

    result = runtest(ns, test_name)

    print()   # Force a newline (just in case)

    # Serialize TestResult as dict in JSON
    print(json.dumps(result, cls=EncodeTestResult), flush=True)
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


class MultiprocessResult(NamedTuple):
    result: TestResult
    # bpo-45410: stderr is written into stdout to keep messages order
    stdout: str
    error_msg: str


ExcStr = str
QueueOutput = tuple[Literal[False], MultiprocessResult] | tuple[Literal[True], ExcStr]


class ExitThread(Exception):
    pass


class TestWorkerProcess(threading.Thread):
    def __init__(self, worker_id: int, runner: "MultiprocessTestRunner") -> None:
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

    def __repr__(self) -> str:
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

    def _kill(self) -> None:
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

    def stop(self) -> None:
        # Method called from a different thread to stop this thread
        self._stopped = True
        self._kill()

    def mp_result_error(
        self,
        test_result: TestResult,
        stdout: str = '',
        err_msg=None
    ) -> MultiprocessResult:
        test_result.duration_sec = time.monotonic() - self.start_time
        return MultiprocessResult(test_result, stdout, err_msg)

    def _run_process(self, test_name: str, tmp_dir: str) -> tuple[int, str, str]:
        self.start_time = time.monotonic()

        self.current_test_name = test_name
        try:
            popen = run_test_in_subprocess(test_name, self.ns, tmp_dir)

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
                # bpo-45410: stderr is written into stdout
                stdout, _ = popen.communicate(timeout=self.timeout)
                retcode = popen.returncode
                assert retcode is not None
            except subprocess.TimeoutExpired:
                if self._stopped:
                    # kill() has been called: communicate() fails on reading
                    # closed stdout
                    raise ExitThread

                # On timeout, kill the process
                self._kill()

                # None means TIMEOUT for the caller
                retcode = None
                # bpo-38207: Don't attempt to call communicate() again: on it
                # can hang until all child processes using stdout
                # pipes completes.
                stdout = ''
            except OSError:
                if self._stopped:
                    # kill() has been called: communicate() fails
                    # on reading closed stdout
                    raise ExitThread
                raise
            else:
                stdout = stdout.strip()

            return (retcode, stdout)
        except:
            self._kill()
            raise
        finally:
            self._wait_completed()
            self._popen = None
            self.current_test_name = None

    def _runtest(self, test_name: str) -> MultiprocessResult:
        # gh-93353: Check for leaked temporary files in the parent process,
        # since the deletion of temporary files can happen late during
        # Python finalization: too late for libregrtest.
        tmp_dir = os.getcwd() + '_tmpdir'
        tmp_dir = os.path.abspath(tmp_dir)
        try:
            os.mkdir(tmp_dir)
            retcode, stdout = self._run_process(test_name, tmp_dir)
        finally:
            tmp_files = os.listdir(tmp_dir)
            os_helper.rmtree(tmp_dir)

        if retcode is None:
            return self.mp_result_error(Timeout(test_name), stdout)

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
                    result = json.loads(result, object_hook=decode_test_result)
                except Exception as exc:
                    err_msg = "Failed to parse worker JSON: %s" % exc

        if err_msg is not None:
            return self.mp_result_error(ChildError(test_name), stdout, err_msg)

        if tmp_files:
            msg = (f'\n\n'
                   f'Warning -- Test leaked temporary files ({len(tmp_files)}): '
                   f'{", ".join(sorted(tmp_files))}')
            stdout += msg
            if isinstance(result, Passed):
                result = EnvChanged.from_passed(result)

        return MultiprocessResult(result, stdout, err_msg)

    def run(self) -> None:
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

    def _wait_completed(self) -> None:
        popen = self._popen

        # stdout must be closed to ensure that communicate() does not hang
        popen.stdout.close()

        try:
            popen.wait(JOIN_TIMEOUT)
        except (subprocess.TimeoutExpired, OSError) as exc:
            print_warning(f"Failed to wait for {self} completion "
                          f"(timeout={format_duration(JOIN_TIMEOUT)}): "
                          f"{exc!r}")

    def wait_stopped(self, start_time: float) -> None:
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


def get_running(workers: list[TestWorkerProcess]) -> list[TestWorkerProcess]:
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
    def __init__(self, regrtest: Regrtest) -> None:
        self.regrtest = regrtest
        self.log = self.regrtest.log
        self.ns = regrtest.ns
        self.output: queue.Queue[QueueOutput] = queue.Queue()
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

    def start_workers(self) -> None:
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

    def stop_workers(self) -> None:
        start_time = time.monotonic()
        for worker in self.workers:
            worker.stop()
        for worker in self.workers:
            worker.wait_stopped(start_time)

    def _get_result(self) -> QueueOutput | None:
        use_faulthandler = (self.ns.timeout is not None)
        timeout = PROGRESS_UPDATE

        # bpo-46205: check the status of workers every iteration to avoid
        # waiting forever on an empty queue.
        while any(worker.is_alive() for worker in self.workers):
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

        # all worker threads are done: consume pending results
        try:
            return self.output.get(timeout=0)
        except queue.Empty:
            return None

    def display_result(self, mp_result: MultiprocessResult) -> None:
        result = mp_result.result

        text = str(result)
        if mp_result.error_msg is not None:
            # CHILD_ERROR
            text += ' (%s)' % mp_result.error_msg
        elif (result.duration_sec >= PROGRESS_MIN_TIME and not self.ns.pgo):
            text += ' (%s)' % format_duration(result.duration_sec)
        running = get_running(self.workers)
        if running and not self.ns.pgo:
            text += ' -- running: %s' % ', '.join(running)
        self.regrtest.display_progress(self.test_index, text)

    def _process_result(self, item: QueueOutput) -> bool:
        """Returns True if test runner must stop."""
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

        if must_stop(mp_result.result, self.ns):
            return True

        return False

    def run_tests(self) -> None:
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


def run_tests_multiprocess(regrtest: Regrtest) -> None:
    MultiprocessTestRunner(regrtest).run_tests()


class EncodeTestResult(json.JSONEncoder):
    """Encode a TestResult (sub)class object into a JSON dict."""

    def default(self, o: Any) -> dict[str, Any]:
        if isinstance(o, TestResult):
            result = vars(o)
            result["__test_result__"] = o.__class__.__name__
            return result

        return super().default(o)


def decode_test_result(d: dict[str, Any]) -> TestResult | dict[str, Any]:
    """Decode a TestResult (sub)class object from a JSON dict."""

    if "__test_result__" not in d:
        return d

    cls_name = d.pop("__test_result__")
    for cls in get_all_test_result_classes():
        if cls.__name__ == cls_name:
            return cls(**d)


def get_all_test_result_classes() -> set[type[TestResult]]:
    prev_count = 0
    classes = {TestResult}
    while len(classes) > prev_count:
        prev_count = len(classes)
        to_add = []
        for cls in classes:
            to_add.extend(cls.__subclasses__())
        classes.update(to_add)
    return classes
