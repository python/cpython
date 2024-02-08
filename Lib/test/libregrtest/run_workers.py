import contextlib
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
from typing import Any, Literal, TextIO

from test import support
from test.support import os_helper, MS_WINDOWS

from .logger import Logger
from .result import TestResult, State
from .results import TestResults
from .runtests import RunTests, WorkerRunTests, JsonFile, JsonFileType
from .single import PROGRESS_MIN_TIME
from .utils import (
    StrPath, TestName,
    format_duration, print_warning, count, plural, get_signal_name)
from .worker import create_worker_process, USE_PROCESS_GROUP

if MS_WINDOWS:
    import locale
    import msvcrt



# Display the running tests if nothing happened last N seconds
PROGRESS_UPDATE = 30.0   # seconds
assert PROGRESS_UPDATE >= PROGRESS_MIN_TIME

# Kill the main process after 5 minutes. It is supposed to write an update
# every PROGRESS_UPDATE seconds. Tolerate 5 minutes for Python slowest
# buildbot workers.
MAIN_PROCESS_TIMEOUT = 5 * 60.0
assert MAIN_PROCESS_TIMEOUT >= PROGRESS_UPDATE

# Time to wait until a worker completes: should be immediate
WAIT_COMPLETED_TIMEOUT = 30.0   # seconds

# Time to wait a killed process (in seconds)
WAIT_KILLED_TIMEOUT = 60.0


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


class WorkerError(Exception):
    def __init__(self,
                 test_name: TestName,
                 err_msg: str | None,
                 stdout: str | None,
                 state: str):
        result = TestResult(test_name, state=state)
        self.mp_result = MultiprocessResult(result, stdout, err_msg)
        super().__init__()


class WorkerThread(threading.Thread):
    def __init__(self, worker_id: int, runner: "RunWorkers") -> None:
        super().__init__()
        self.worker_id = worker_id
        self.runtests = runner.runtests
        self.pending = runner.pending
        self.output = runner.output
        self.timeout = runner.worker_timeout
        self.log = runner.log
        self.test_name: TestName | None = None
        self.start_time: float | None = None
        self._popen: subprocess.Popen[str] | None = None
        self._killed = False
        self._stopped = False

    def __repr__(self) -> str:
        info = [f'WorkerThread #{self.worker_id}']
        if self.is_alive():
            info.append("running")
        else:
            info.append('stopped')
        test = self.test_name
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
            what = f"{self} process"

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

    def _run_process(self, runtests: WorkerRunTests, output_fd: int,
                     tmp_dir: StrPath | None = None) -> int | None:
        popen = create_worker_process(runtests, output_fd, tmp_dir)
        self._popen = popen
        self._killed = False

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

    def create_stdout(self, stack: contextlib.ExitStack) -> TextIO:
        """Create stdout temporay file (file descriptor)."""

        if MS_WINDOWS:
            # gh-95027: When stdout is not a TTY, Python uses the ANSI code
            # page for the sys.stdout encoding. If the main process runs in a
            # terminal, sys.stdout uses WindowsConsoleIO with UTF-8 encoding.
            encoding = locale.getencoding()
        else:
            encoding = sys.stdout.encoding

        # gh-94026: Write stdout+stderr to a tempfile as workaround for
        # non-blocking pipes on Emscripten with NodeJS.
        # gh-109425: Use "backslashreplace" error handler: log corrupted
        # stdout+stderr, instead of failing with a UnicodeDecodeError and not
        # logging stdout+stderr at all.
        stdout_file = tempfile.TemporaryFile('w+',
                                             encoding=encoding,
                                             errors='backslashreplace')
        stack.enter_context(stdout_file)
        return stdout_file

    def create_json_file(self, stack: contextlib.ExitStack) -> tuple[JsonFile, TextIO | None]:
        """Create JSON file."""

        json_file_use_stdout = self.runtests.json_file_use_stdout()
        if json_file_use_stdout:
            json_file = JsonFile(None, JsonFileType.STDOUT)
            json_tmpfile = None
        else:
            json_tmpfile = tempfile.TemporaryFile('w+', encoding='utf8')
            stack.enter_context(json_tmpfile)

            json_fd = json_tmpfile.fileno()
            if MS_WINDOWS:
                # The msvcrt module is only available on Windows;
                # we run mypy with `--platform=linux` in CI
                json_handle: int = msvcrt.get_osfhandle(json_fd)  # type: ignore[attr-defined]
                json_file = JsonFile(json_handle,
                                     JsonFileType.WINDOWS_HANDLE)
            else:
                json_file = JsonFile(json_fd, JsonFileType.UNIX_FD)
        return (json_file, json_tmpfile)

    def create_worker_runtests(self, test_name: TestName, json_file: JsonFile) -> WorkerRunTests:
        tests = (test_name,)
        if self.runtests.rerun:
            match_tests = self.runtests.get_match_tests(test_name)
        else:
            match_tests = None

        kwargs: dict[str, Any] = {}
        if match_tests:
            kwargs['match_tests'] = [(test, True) for test in match_tests]
        if self.runtests.output_on_failure:
            kwargs['verbose'] = True
            kwargs['output_on_failure'] = False
        return self.runtests.create_worker_runtests(
            tests=tests,
            json_file=json_file,
            **kwargs)

    def run_tmp_files(self, worker_runtests: WorkerRunTests,
                      stdout_fd: int) -> tuple[int | None, list[StrPath]]:
        # gh-93353: Check for leaked temporary files in the parent process,
        # since the deletion of temporary files can happen late during
        # Python finalization: too late for libregrtest.
        if not support.is_wasi:
            # Don't check for leaked temporary files and directories if Python is
            # run on WASI. WASI doesn't pass environment variables like TMPDIR to
            # worker processes.
            tmp_dir = tempfile.mkdtemp(prefix="test_python_")
            tmp_dir = os.path.abspath(tmp_dir)
            try:
                retcode = self._run_process(worker_runtests,
                                            stdout_fd, tmp_dir)
            finally:
                tmp_files = os.listdir(tmp_dir)
                os_helper.rmtree(tmp_dir)
        else:
            retcode = self._run_process(worker_runtests, stdout_fd)
            tmp_files = []

        return (retcode, tmp_files)

    def read_stdout(self, stdout_file: TextIO) -> str:
        stdout_file.seek(0)
        try:
            return stdout_file.read().strip()
        except Exception as exc:
            # gh-101634: Catch UnicodeDecodeError if stdout cannot be
            # decoded from encoding
            raise WorkerError(self.test_name,
                              f"Cannot read process stdout: {exc}",
                              stdout=None,
                              state=State.WORKER_BUG)

    def read_json(self, json_file: JsonFile, json_tmpfile: TextIO | None,
                  stdout: str) -> tuple[TestResult, str]:
        try:
            if json_tmpfile is not None:
                json_tmpfile.seek(0)
                worker_json = json_tmpfile.read()
            elif json_file.file_type == JsonFileType.STDOUT:
                stdout, _, worker_json = stdout.rpartition("\n")
                stdout = stdout.rstrip()
            else:
                with json_file.open(encoding='utf8') as json_fp:
                    worker_json = json_fp.read()
        except Exception as exc:
            # gh-101634: Catch UnicodeDecodeError if stdout cannot be
            # decoded from encoding
            err_msg = f"Failed to read worker process JSON: {exc}"
            raise WorkerError(self.test_name, err_msg, stdout,
                              state=State.WORKER_BUG)

        if not worker_json:
            raise WorkerError(self.test_name, "empty JSON", stdout,
                              state=State.WORKER_BUG)

        try:
            result = TestResult.from_json(worker_json)
        except Exception as exc:
            # gh-101634: Catch UnicodeDecodeError if stdout cannot be
            # decoded from encoding
            err_msg = f"Failed to parse worker process JSON: {exc}"
            raise WorkerError(self.test_name, err_msg, stdout,
                              state=State.WORKER_BUG)

        return (result, stdout)

    def _runtest(self, test_name: TestName) -> MultiprocessResult:
        with contextlib.ExitStack() as stack:
            stdout_file = self.create_stdout(stack)
            json_file, json_tmpfile = self.create_json_file(stack)
            worker_runtests = self.create_worker_runtests(test_name, json_file)

            retcode: str | int | None
            retcode, tmp_files = self.run_tmp_files(worker_runtests,
                                                    stdout_file.fileno())

            stdout = self.read_stdout(stdout_file)

            if retcode is None:
                raise WorkerError(self.test_name, stdout=stdout,
                                  err_msg=None,
                                  state=State.TIMEOUT)
            if retcode != 0:
                name = get_signal_name(retcode)
                if name:
                    retcode = f"{retcode} ({name})"
                raise WorkerError(self.test_name, f"Exit code {retcode}", stdout,
                                  state=State.WORKER_FAILED)

            result, stdout = self.read_json(json_file, json_tmpfile, stdout)

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
                self.test_name = test_name
                try:
                    mp_result = self._runtest(test_name)
                except WorkerError as exc:
                    mp_result = exc.mp_result
                finally:
                    self.test_name = None
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
            popen.wait(WAIT_COMPLETED_TIMEOUT)
        except (subprocess.TimeoutExpired, OSError) as exc:
            print_warning(f"Failed to wait for {self} completion "
                          f"(timeout={format_duration(WAIT_COMPLETED_TIMEOUT)}): "
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
            if dt > WAIT_KILLED_TIMEOUT:
                print_warning(f"Failed to join {self} in {format_duration(dt)}")
                break


def get_running(workers: list[WorkerThread]) -> str | None:
    running: list[str] = []
    for worker in workers:
        test_name = worker.test_name
        if not test_name:
            continue
        dt = time.monotonic() - worker.start_time
        if dt >= PROGRESS_MIN_TIME:
            text = f'{test_name} ({format_duration(dt)})'
            running.append(text)
    if not running:
        return None
    return f"running ({len(running)}): {', '.join(running)}"


class RunWorkers:
    def __init__(self, num_workers: int, runtests: RunTests,
                 logger: Logger, results: TestResults) -> None:
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
            self.worker_timeout: float | None = min(self.timeout * 1.5, self.timeout + 5 * 60)
        else:
            self.worker_timeout = None
        self.workers: list[WorkerThread] | None = None

        jobs = self.runtests.get_jobs()
        if jobs is not None:
            # Don't spawn more threads than the number of jobs:
            # these worker threads would never get anything to do.
            self.num_workers = min(self.num_workers, jobs)

    def start_workers(self) -> None:
        self.workers = [WorkerThread(index, self)
                        for index in range(1, self.num_workers + 1)]
        jobs = self.runtests.get_jobs()
        if jobs is not None:
            tests = count(jobs, 'test')
        else:
            tests = 'tests'
        nworkers = len(self.workers)
        processes = plural(nworkers, "process", "processes")
        msg = (f"Run {tests} in parallel using "
               f"{nworkers} worker {processes}")
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
            # WORKER_BUG
            text += ' (%s)' % mp_result.err_msg
        elif (result.duration >= PROGRESS_MIN_TIME and not pgo):
            text += ' (%s)' % format_duration(result.duration)
        if not pgo:
            running = get_running(self.workers)
            if running:
                text += f' -- {running}'
        self.display_progress(self.test_index, text)

    def _process_result(self, item: QueueOutput) -> TestResult:
        """Returns True if test runner must stop."""
        if item[0]:
            # Thread got an exception
            format_exc = item[1]
            print_warning(f"regrtest worker thread failed: {format_exc}")
            result = TestResult("<regrtest worker>", state=State.WORKER_BUG)
            self.results.accumulate_result(result, self.runtests)
            return result

        self.test_index += 1
        mp_result = item[1]
        result = mp_result.result
        self.results.accumulate_result(result, self.runtests)
        self.display_result(mp_result)

        # Display worker stdout
        if not self.runtests.output_on_failure:
            show_stdout = True
        else:
            # --verbose3 ignores stdout on success
            show_stdout = (result.state != State.PASSED)
        if show_stdout:
            stdout = mp_result.worker_stdout
            if stdout:
                print(stdout, flush=True)

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
