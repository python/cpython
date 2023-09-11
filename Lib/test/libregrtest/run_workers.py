import dataclasses
import faulthandler
import os.path
import queue
import signal
import subprocess
import sys
import tempfile
import threading
import time
import traceback
from typing import Literal, TextIO

from test import support
from test.support import os_helper

from .logger import Logger
from .result import TestResult, State
from .results import TestResults
from .runtests import RunTests
from .single import PROGRESS_MIN_TIME
from .utils import (
    StrPath, TestName,
    format_duration, print_warning)
from .worker import create_worker_process, USE_PROCESS_GROUP

if sys.platform == 'win32':
    import locale


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


@dataclasses.dataclass(slots=True, frozen=True)
class MultiprocessResult:
    result: TestResult
    # bpo-45410: stderr is written into stdout to keep messages order
    worker_stdout: str | None = None
    err_msg: str | None = None


ExcStr = str
QueueOutput = tuple[Literal[False], MultiprocessResult] | tuple[Literal[True], ExcStr]


class ExitThread(Exception):
    pass


class WorkerThread(threading.Thread):
    def __init__(self, worker_id: int, runner: "RunWorkers") -> None:
        super().__init__()
        self.worker_id = worker_id
        self.runtests = runner.runtests
        self.pending = runner.pending
        self.output = runner.output
        self.timeout = runner.worker_timeout
        self.log = runner.log
        self.current_test_name = None
        self.start_time = None
        self._popen = None
        self._killed = False
        self._stopped = False

    def __repr__(self) -> str:
        info = [f'WorkerThread #{self.worker_id}']
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
            # popen.kill(): the process completed, the WorkerThread thread
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
        stdout: str | None = None,
        err_msg=None
    ) -> MultiprocessResult:
        return MultiprocessResult(test_result, stdout, err_msg)

    def _run_process(self, runtests: RunTests, output_file: TextIO,
                     tmp_dir: StrPath | None = None) -> int:
        try:
            popen = create_worker_process(runtests, output_file, tmp_dir)

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
                # gh-94026: stdout+stderr are written to tempfile
                retcode = popen.wait(timeout=self.timeout)
                assert retcode is not None
                return retcode
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
            except OSError:
                if self._stopped:
                    # kill() has been called: communicate() fails
                    # on reading closed stdout
                    raise ExitThread
                raise
        except:
            self._kill()
            raise
        finally:
            self._wait_completed()
            self._popen = None
            self.current_test_name = None

    def _runtest(self, test_name: TestName) -> MultiprocessResult:
        self.current_test_name = test_name

        if sys.platform == 'win32':
            # gh-95027: When stdout is not a TTY, Python uses the ANSI code
            # page for the sys.stdout encoding. If the main process runs in a
            # terminal, sys.stdout uses WindowsConsoleIO with UTF-8 encoding.
            encoding = locale.getencoding()
        else:
            encoding = sys.stdout.encoding

        tests = (test_name,)
        if self.runtests.rerun:
            match_tests = self.runtests.get_match_tests(test_name)
        else:
            match_tests = None
        kwargs = {}
        if match_tests:
            kwargs['match_tests'] = match_tests
        worker_runtests = self.runtests.copy(tests=tests, **kwargs)

        # gh-94026: Write stdout+stderr to a tempfile as workaround for
        # non-blocking pipes on Emscripten with NodeJS.
        with tempfile.TemporaryFile('w+', encoding=encoding) as stdout_file:
            # gh-93353: Check for leaked temporary files in the parent process,
            # since the deletion of temporary files can happen late during
            # Python finalization: too late for libregrtest.
            if not support.is_wasi:
                # Don't check for leaked temporary files and directories if Python is
                # run on WASI. WASI don't pass environment variables like TMPDIR to
                # worker processes.
                tmp_dir = tempfile.mkdtemp(prefix="test_python_")
                tmp_dir = os.path.abspath(tmp_dir)
                try:
                    retcode = self._run_process(worker_runtests, stdout_file, tmp_dir)
                finally:
                    tmp_files = os.listdir(tmp_dir)
                    os_helper.rmtree(tmp_dir)
            else:
                retcode = self._run_process(worker_runtests, stdout_file)
                tmp_files = ()
            stdout_file.seek(0)

            try:
                stdout = stdout_file.read().strip()
            except Exception as exc:
                # gh-101634: Catch UnicodeDecodeError if stdout cannot be
                # decoded from encoding
                err_msg = f"Cannot read process stdout: {exc}"
                result = TestResult(test_name, state=State.MULTIPROCESSING_ERROR)
                return self.mp_result_error(result, err_msg=err_msg)

        if retcode is None:
            result = TestResult(test_name, state=State.TIMEOUT)
            return self.mp_result_error(result, stdout)

        err_msg = None
        if retcode != 0:
            err_msg = "Exit code %s" % retcode
        else:
            stdout, _, worker_json = stdout.rpartition("\n")
            stdout = stdout.rstrip()
            if not worker_json:
                err_msg = "Failed to parse worker stdout"
            else:
                try:
                    result = TestResult.from_json(worker_json)
                except Exception as exc:
                    err_msg = "Failed to parse worker JSON: %s" % exc

        if err_msg:
            result = TestResult(test_name, state=State.MULTIPROCESSING_ERROR)
            return self.mp_result_error(result, stdout, err_msg)

        if tmp_files:
            msg = (f'\n\n'
                   f'Warning -- {test_name} leaked temporary files '
                   f'({len(tmp_files)}): {", ".join(sorted(tmp_files))}')
            stdout += msg
            result.set_env_changed()

        return MultiprocessResult(result, stdout)

    def run(self) -> None:
        fail_fast = self.runtests.fail_fast
        fail_env_changed = self.runtests.fail_env_changed
        while not self._stopped:
            try:
                try:
                    test_name = next(self.pending)
                except StopIteration:
                    break

                self.start_time = time.monotonic()
                mp_result = self._runtest(test_name)
                mp_result.result.duration = time.monotonic() - self.start_time
                self.output.put((False, mp_result))

                if mp_result.result.must_stop(fail_fast, fail_env_changed):
                    break
            except ExitThread:
                break
            except BaseException:
                self.output.put((True, traceback.format_exc()))
                break

    def _wait_completed(self) -> None:
        popen = self._popen

        try:
            popen.wait(JOIN_TIMEOUT)
        except (subprocess.TimeoutExpired, OSError) as exc:
            print_warning(f"Failed to wait for {self} completion "
                          f"(timeout={format_duration(JOIN_TIMEOUT)}): "
                          f"{exc!r}")

    def wait_stopped(self, start_time: float) -> None:
        # bpo-38207: RunWorkers.stop_workers() called self.stop()
        # which killed the process. Sometimes, killing the process from the
        # main thread does not interrupt popen.communicate() in
        # WorkerThread thread. This loop with a timeout is a workaround
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
            self.log(f"Waiting for {self} thread for {format_duration(dt)}")
            if dt > JOIN_TIMEOUT:
                print_warning(f"Failed to join {self} in {format_duration(dt)}")
                break


def get_running(workers: list[WorkerThread]) -> list[str]:
    running = []
    for worker in workers:
        current_test_name = worker.current_test_name
        if not current_test_name:
            continue
        dt = time.monotonic() - worker.start_time
        if dt >= PROGRESS_MIN_TIME:
            text = '%s (%s)' % (current_test_name, format_duration(dt))
            running.append(text)
    if not running:
        return None
    return f"running ({len(running)}): {', '.join(running)}"


class RunWorkers:
    def __init__(self, num_workers: int, runtests: RunTests,
                 logger: Logger, results: TestResult) -> None:
        self.num_workers = num_workers
        self.runtests = runtests
        self.log = logger.log
        self.display_progress = logger.display_progress
        self.results: TestResults = results

        self.output: queue.Queue[QueueOutput] = queue.Queue()
        tests_iter = runtests.iter_tests()
        self.pending = MultiprocessIterator(tests_iter)
        self.timeout = runtests.timeout
        if self.timeout is not None:
            # Rely on faulthandler to kill a worker process. This timouet is
            # when faulthandler fails to kill a worker process. Give a maximum
            # of 5 minutes to faulthandler to kill the worker.
            self.worker_timeout = min(self.timeout * 1.5, self.timeout + 5 * 60)
        else:
            self.worker_timeout = None
        self.workers = None

    def start_workers(self) -> None:
        self.workers = [WorkerThread(index, self)
                        for index in range(1, self.num_workers + 1)]
        msg = f"Run tests in parallel using {len(self.workers)} child processes"
        if self.timeout:
            msg += (" (timeout: %s, worker timeout: %s)"
                    % (format_duration(self.timeout),
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
        pgo = self.runtests.pgo
        use_faulthandler = (self.timeout is not None)

        # bpo-46205: check the status of workers every iteration to avoid
        # waiting forever on an empty queue.
        while any(worker.is_alive() for worker in self.workers):
            if use_faulthandler:
                faulthandler.dump_traceback_later(MAIN_PROCESS_TIMEOUT,
                                                  exit=True)

            # wait for a thread
            try:
                return self.output.get(timeout=PROGRESS_UPDATE)
            except queue.Empty:
                pass

            if not pgo:
                # display progress
                running = get_running(self.workers)
                if running:
                    self.log(running)

        # all worker threads are done: consume pending results
        try:
            return self.output.get(timeout=0)
        except queue.Empty:
            return None

    def display_result(self, mp_result: MultiprocessResult) -> None:
        result = mp_result.result
        pgo = self.runtests.pgo

        text = str(result)
        if mp_result.err_msg:
            # MULTIPROCESSING_ERROR
            text += ' (%s)' % mp_result.err_msg
        elif (result.duration >= PROGRESS_MIN_TIME and not pgo):
            text += ' (%s)' % format_duration(result.duration)
        if not pgo:
            running = get_running(self.workers)
            if running:
                text += f' -- {running}'
        self.display_progress(self.test_index, text)

    def _process_result(self, item: QueueOutput) -> bool:
        """Returns True if test runner must stop."""
        if item[0]:
            # Thread got an exception
            format_exc = item[1]
            print_warning(f"regrtest worker thread failed: {format_exc}")
            result = TestResult("<regrtest worker>", state=State.MULTIPROCESSING_ERROR)
            self.results.accumulate_result(result, self.runtests)
            return result

        self.test_index += 1
        mp_result = item[1]
        result = mp_result.result
        self.results.accumulate_result(result, self.runtests)
        self.display_result(mp_result)

        if mp_result.worker_stdout:
            print(mp_result.worker_stdout, flush=True)

        return result

    def run(self) -> None:
        fail_fast = self.runtests.fail_fast
        fail_env_changed = self.runtests.fail_env_changed

        self.start_workers()

        self.test_index = 0
        try:
            while True:
                item = self._get_result()
                if item is None:
                    break

                result = self._process_result(item)
                if result.must_stop(fail_fast, fail_env_changed):
                    break
        except KeyboardInterrupt:
            print()
            self.results.interrupted = True
        finally:
            if self.timeout is not None:
                faulthandler.cancel_dump_traceback_later()

            # Always ensure that all worker processes are no longer
            # worker when we exit this function
            self.pending.stop()
            self.stop_workers()
