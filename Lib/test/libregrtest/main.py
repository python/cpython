import os
import random
import re
import shlex
import sys
import sysconfig
import time
import trace
from _colorize import get_colors  # type: ignore[import-not-found]
from typing import NoReturn

from test.support import os_helper, MS_WINDOWS, flush_std_streams

from .cmdline import _parse_args, Namespace
from .findtests import findtests, split_test_packages, list_cases
from .logger import Logger
from .pgo import setup_pgo_tests
from .result import TestResult
from .results import TestResults, EXITCODE_INTERRUPTED
from .runtests import RunTests, HuntRefleak
from .setup import setup_process, setup_test_dir
from .single import run_single_test, PROGRESS_MIN_TIME
from .tsan import setup_tsan_tests, setup_tsan_parallel_tests
from .utils import (
    StrPath, StrJSON, TestName, TestList, TestTuple, TestFilter,
    strip_py_suffix, count, format_duration,
    printlist, get_temp_dir, get_work_dir, exit_timeout,
    display_header, cleanup_temp_dir, print_warning,
    is_cross_compiled, get_host_runner,
    EXIT_TIMEOUT)


class Regrtest:
    """Execute a test suite.

    This also parses command-line options and modifies its behavior
    accordingly.

    tests -- a list of strings containing test names (optional)
    testdir -- the directory in which to look for tests (optional)

    Users other than the Python test suite will certainly want to
    specify testdir; if it's omitted, the directory containing the
    Python test suite is searched for.

    If the tests argument is omitted, the tests listed on the
    command-line will be used.  If that's empty, too, then all *.py
    files beginning with test_ will be used.

    The other default arguments (verbose, quiet, exclude,
    single, randomize, use_resources, trace, coverdir,
    print_slow, and random_seed) allow programmers calling main()
    directly to set the values that would normally be set by flags
    on the command line.
    """
    def __init__(self, ns: Namespace, _add_python_opts: bool = False):
        # Log verbosity
        self.verbose: int = int(ns.verbose)
        self.quiet: bool = ns.quiet
        self.pgo: bool = ns.pgo
        self.pgo_extended: bool = ns.pgo_extended
        self.tsan: bool = ns.tsan
        self.tsan_parallel: bool = ns.tsan_parallel

        # Test results
        self.results: TestResults = TestResults()
        self.first_state: str | None = None

        # Logger
        self.logger = Logger(self.results, self.quiet, self.pgo)

        # Actions
        self.want_header: bool = ns.header
        self.want_list_tests: bool = ns.list_tests
        self.want_list_cases: bool = ns.list_cases
        self.want_wait: bool = ns.wait
        self.want_cleanup: bool = ns.cleanup
        self.want_rerun: bool = ns.rerun
        self.want_run_leaks: bool = ns.runleaks
        self.want_bisect: bool = ns.bisect

        self.ci_mode: bool = (ns.fast_ci or ns.slow_ci)
        self.want_add_python_opts: bool = (_add_python_opts
                                           and ns._add_python_opts)

        # Select tests
        self.match_tests: TestFilter = ns.match_tests
        self.exclude: bool = ns.exclude
        self.fromfile: StrPath | None = ns.fromfile
        self.starting_test: TestName | None = ns.start
        self.cmdline_args: TestList = ns.args

        # Workers
        self.single_process: bool = ns.single_process
        if self.single_process or ns.use_mp is None:
            num_workers = 0   # run sequentially in a single process
        elif ns.use_mp <= 0:
            num_workers = -1  # run in parallel, use the number of CPUs
        else:
            num_workers = ns.use_mp  # run in parallel
        self.num_workers: int = num_workers
        self.worker_json: StrJSON | None = ns.worker_json

        # Options to run tests
        self.fail_fast: bool = ns.failfast
        self.fail_env_changed: bool = ns.fail_env_changed
        self.fail_rerun: bool = ns.fail_rerun
        self.forever: bool = ns.forever
        self.output_on_failure: bool = ns.verbose3
        self.timeout: float | None = ns.timeout
        if ns.huntrleaks:
            warmups, runs, filename = ns.huntrleaks
            filename = os.path.abspath(filename)
            self.hunt_refleak: HuntRefleak | None = HuntRefleak(warmups, runs, filename)
        else:
            self.hunt_refleak = None
        self.test_dir: StrPath | None = ns.testdir
        self.junit_filename: StrPath | None = ns.xmlpath
        self.memory_limit: str | None = ns.memlimit
        self.gc_threshold: int | None = ns.threshold
        self.use_resources: tuple[str, ...] = tuple(ns.use_resources)
        if ns.python:
            self.python_cmd: tuple[str, ...] | None = tuple(ns.python)
        else:
            self.python_cmd = None
        self.coverage: bool = ns.trace
        self.coverage_dir: StrPath | None = ns.coverdir
        self._tmp_dir: StrPath | None = ns.tempdir

        # Randomize
        self.randomize: bool = ns.randomize
        if ('SOURCE_DATE_EPOCH' in os.environ
            # don't use the variable if empty
            and os.environ['SOURCE_DATE_EPOCH']
        ):
            self.randomize = False
            # SOURCE_DATE_EPOCH should be an integer, but use a string to not
            # fail if it's not integer. random.seed() accepts a string.
            # https://reproducible-builds.org/docs/source-date-epoch/
            self.random_seed: int | str = os.environ['SOURCE_DATE_EPOCH']
        elif ns.random_seed is None:
            self.random_seed = random.getrandbits(32)
        else:
            self.random_seed = ns.random_seed
        self.prioritize_tests: tuple[str, ...] = tuple(ns.prioritize)

        self.parallel_threads = ns.parallel_threads

        # tests
        self.first_runtests: RunTests | None = None

        # used by --slowest
        self.print_slowest: bool = ns.print_slow

        # used to display the progress bar "[ 3/100]"
        self.start_time = time.perf_counter()

        # used by --single
        self.single_test_run: bool = ns.single
        self.next_single_test: TestName | None = None
        self.next_single_filename: StrPath | None = None

    def log(self, line: str = '') -> None:
        self.logger.log(line)

    def find_tests(self, tests: TestList | None = None) -> tuple[TestTuple, TestList | None]:
        if tests is None:
            tests = []
        if self.single_test_run:
            self.next_single_filename = os.path.join(self.tmp_dir, 'pynexttest')
            try:
                with open(self.next_single_filename, 'r') as fp:
                    next_test = fp.read().strip()
                    tests = [next_test]
            except OSError:
                pass

        if self.fromfile:
            tests = []
            # regex to match 'test_builtin' in line:
            # '0:00:00 [  4/400] test_builtin -- test_dict took 1 sec'
            regex = re.compile(r'\btest_[a-zA-Z0-9_]+\b')
            with open(os.path.join(os_helper.SAVEDCWD, self.fromfile)) as fp:
                for line in fp:
                    line = line.split('#', 1)[0]
                    line = line.strip()
                    match = regex.search(line)
                    if match is not None:
                        tests.append(match.group())

        strip_py_suffix(tests)

        if self.pgo:
            # add default PGO tests if no tests are specified
            setup_pgo_tests(self.cmdline_args, self.pgo_extended)

        if self.tsan:
            setup_tsan_tests(self.cmdline_args)

        if self.tsan_parallel:
            setup_tsan_parallel_tests(self.cmdline_args)

        exclude_tests = set()
        if self.exclude:
            for arg in self.cmdline_args:
                exclude_tests.add(arg)
            self.cmdline_args = []

        alltests = findtests(testdir=self.test_dir,
                             exclude=exclude_tests)

        if not self.fromfile:
            selected = tests or self.cmdline_args
            if selected:
                selected = split_test_packages(selected)
            else:
                selected = alltests
        else:
            selected = tests

        if self.single_test_run:
            selected = selected[:1]
            try:
                pos = alltests.index(selected[0])
                self.next_single_test = alltests[pos + 1]
            except IndexError:
                pass

        # Remove all the selected tests that precede start if it's set.
        if self.starting_test:
            try:
                del selected[:selected.index(self.starting_test)]
            except ValueError:
                print(f"Cannot find starting test: {self.starting_test}")
                sys.exit(1)

        random.seed(self.random_seed)
        if self.randomize:
            random.shuffle(selected)

        for priority_test in reversed(self.prioritize_tests):
            try:
                selected.remove(priority_test)
            except ValueError:
                print(f"warning: --prioritize={priority_test} used"
                        f" but test not actually selected")
                continue
            else:
                selected.insert(0, priority_test)

        return (tuple(selected), tests)

    @staticmethod
    def list_tests(tests: TestTuple) -> None:
        for name in tests:
            print(name)

    def _rerun_failed_tests(self, runtests: RunTests) -> RunTests:
        # Configure the runner to re-run tests
        if self.num_workers == 0 and not self.single_process:
            # Always run tests in fresh processes to have more deterministic
            # initial state. Don't re-run tests in parallel but limit to a
            # single worker process to have side effects (on the system load
            # and timings) between tests.
            self.num_workers = 1

        tests, match_tests_dict = self.results.prepare_rerun()

        # Re-run failed tests
        runtests = runtests.copy(
            tests=tests,
            rerun=True,
            verbose=True,
            forever=False,
            fail_fast=False,
            match_tests_dict=match_tests_dict,
            output_on_failure=False)
        self.logger.set_tests(runtests)

        msg = f"Re-running {len(tests)} failed tests in verbose mode"
        if not self.single_process:
            msg = f"{msg} in subprocesses"
            self.log(msg)
            self._run_tests_mp(runtests, self.num_workers)
        else:
            self.log(msg)
            self.run_tests_sequentially(runtests)
        return runtests

    def rerun_failed_tests(self, runtests: RunTests) -> None:
        ansi = get_colors()
        red, reset = ansi.BOLD_RED, ansi.RESET

        if self.python_cmd:
            # Temp patch for https://github.com/python/cpython/issues/94052
            self.log(
                "Re-running failed tests is not supported with --python "
                "host runner option."
            )
            return

        self.first_state = self.get_state()

        print()
        rerun_runtests = self._rerun_failed_tests(runtests)

        if self.results.bad:
            print(
                f"{red}{count(len(self.results.bad), 'test')} "
                f"failed again:{reset}"
            )
            printlist(self.results.bad)

        self.display_result(rerun_runtests)

    def _run_bisect(self, runtests: RunTests, test: str, progress: str) -> bool:
        print()
        title = f"Bisect {test}"
        if progress:
            title = f"{title} ({progress})"
        print(title)
        print("#" * len(title))
        print()

        cmd = runtests.create_python_cmd()
        cmd.extend([
            "-u", "-m", "test.bisect_cmd",
            # Limit to 25 iterations (instead of 100) to not abuse CI resources
            "--max-iter", "25",
            "-v",
            # runtests.match_tests is not used (yet) for bisect_cmd -i arg
        ])
        cmd.extend(runtests.bisect_cmd_args())
        cmd.append(test)
        print("+", shlex.join(cmd), flush=True)

        flush_std_streams()

        import subprocess
        proc = subprocess.run(cmd, timeout=runtests.timeout)
        exitcode = proc.returncode

        title = f"{title}: exit code {exitcode}"
        print(title)
        print("#" * len(title))
        print(flush=True)

        if exitcode:
            print(f"Bisect failed with exit code {exitcode}")
            return False

        return True

    def run_bisect(self, runtests: RunTests) -> None:
        tests, _ = self.results.prepare_rerun(clear=False)

        for index, name in enumerate(tests, 1):
            if len(tests) > 1:
                progress = f"{index}/{len(tests)}"
            else:
                progress = ""
            if not self._run_bisect(runtests, name, progress):
                return

    def display_result(self, runtests: RunTests) -> None:
        # If running the test suite for PGO then no one cares about results.
        if runtests.pgo:
            return

        state = self.get_state()
        print()
        print(f"== Tests result: {state} ==")

        self.results.display_result(runtests.tests,
                                    self.quiet, self.print_slowest)

    def run_test(
        self, test_name: TestName, runtests: RunTests, tracer: trace.Trace | None
    ) -> TestResult:
        if tracer is not None:
            # If we're tracing code coverage, then we don't exit with status
            # if on a false return value from main.
            cmd = ('result = run_single_test(test_name, runtests)')
            namespace = dict(locals())
            tracer.runctx(cmd, globals=globals(), locals=namespace)
            result = namespace['result']
            result.covered_lines = list(tracer.counts)
        else:
            result = run_single_test(test_name, runtests)

        self.results.accumulate_result(result, runtests)

        return result

    def run_tests_sequentially(self, runtests: RunTests) -> None:
        if self.coverage:
            tracer = trace.Trace(trace=False, count=True)
        else:
            tracer = None

        save_modules = set(sys.modules)

        jobs = runtests.get_jobs()
        if jobs is not None:
            tests = count(jobs, 'test')
        else:
            tests = 'tests'
        msg = f"Run {tests} sequentially in a single process"
        if runtests.timeout:
            msg += " (timeout: %s)" % format_duration(runtests.timeout)
        self.log(msg)

        tests_iter = runtests.iter_tests()
        for test_index, test_name in enumerate(tests_iter, 1):
            start_time = time.perf_counter()

            self.logger.display_progress(test_index, test_name)

            result = self.run_test(test_name, runtests, tracer)

            # Unload the newly imported test modules (best effort finalization)
            new_modules = [module for module in sys.modules
                           if module not in save_modules and
                                module.startswith(("test.", "test_"))]
            for module in new_modules:
                sys.modules.pop(module, None)
                # Remove the attribute of the parent module.
                parent, _, name = module.rpartition('.')
                try:
                    delattr(sys.modules[parent], name)
                except (KeyError, AttributeError):
                    pass

            text = str(result)
            test_time = time.perf_counter() - start_time
            if test_time >= PROGRESS_MIN_TIME:
                text = f"{text} in {format_duration(test_time)}"
            self.logger.display_progress(test_index, text)

            if result.must_stop(self.fail_fast, self.fail_env_changed):
                break

    def get_state(self) -> str:
        state = self.results.get_state(self.fail_env_changed)
        if self.first_state:
            state = f'{self.first_state} then {state}'
        return state

    def _run_tests_mp(self, runtests: RunTests, num_workers: int) -> None:
        from .run_workers import RunWorkers
        RunWorkers(num_workers, runtests, self.logger, self.results).run()

    def finalize_tests(self, coverage: trace.CoverageResults | None) -> None:
        if self.next_single_filename:
            if self.next_single_test:
                with open(self.next_single_filename, 'w') as fp:
                    fp.write(self.next_single_test + '\n')
            else:
                os.unlink(self.next_single_filename)

        if coverage is not None:
            # uses a new-in-Python 3.13 keyword argument that mypy doesn't know about yet:
            coverage.write_results(show_missing=True, summary=True,  # type: ignore[call-arg]
                                   coverdir=self.coverage_dir,
                                   ignore_missing_files=True)

        if self.want_run_leaks:
            os.system("leaks %d" % os.getpid())

        if self.junit_filename:
            self.results.write_junit(self.junit_filename)

    def display_summary(self) -> None:
        if self.first_runtests is None:
            raise ValueError(
                "Should never call `display_summary()` before calling `_run_test()`"
            )

        duration = time.perf_counter() - self.logger.start_time
        filtered = bool(self.match_tests)

        # Total duration
        print()
        print("Total duration: %s" % format_duration(duration))

        self.results.display_summary(self.first_runtests, filtered)

        # Result
        state = self.get_state()
        print(f"Result: {state}")

    def create_run_tests(self, tests: TestTuple) -> RunTests:
        return RunTests(
            tests,
            fail_fast=self.fail_fast,
            fail_env_changed=self.fail_env_changed,
            match_tests=self.match_tests,
            match_tests_dict=None,
            rerun=False,
            forever=self.forever,
            pgo=self.pgo,
            pgo_extended=self.pgo_extended,
            output_on_failure=self.output_on_failure,
            timeout=self.timeout,
            verbose=self.verbose,
            quiet=self.quiet,
            hunt_refleak=self.hunt_refleak,
            test_dir=self.test_dir,
            use_junit=(self.junit_filename is not None),
            coverage=self.coverage,
            memory_limit=self.memory_limit,
            gc_threshold=self.gc_threshold,
            use_resources=self.use_resources,
            python_cmd=self.python_cmd,
            randomize=self.randomize,
            random_seed=self.random_seed,
            parallel_threads=self.parallel_threads,
        )

    def _run_tests(self, selected: TestTuple, tests: TestList | None) -> int:
        if self.hunt_refleak and self.hunt_refleak.warmups < 3:
            msg = ("WARNING: Running tests with --huntrleaks/-R and "
                   "less than 3 warmup repetitions can give false positives!")
            print(msg, file=sys.stdout, flush=True)

        if self.num_workers < 0:
            # Use all CPUs + 2 extra worker processes for tests
            # that like to sleep
            #
            # os.process.cpu_count() is new in Python 3.13;
            # mypy doesn't know about it yet
            self.num_workers = (os.process_cpu_count() or 1) + 2  # type: ignore[attr-defined]

        # For a partial run, we do not need to clutter the output.
        if (self.want_header
            or not(self.pgo or self.quiet or self.single_test_run
                   or tests or self.cmdline_args)):
            display_header(self.use_resources, self.python_cmd)

        print("Using random seed:", self.random_seed)

        runtests = self.create_run_tests(selected)
        self.first_runtests = runtests
        self.logger.set_tests(runtests)

        if (runtests.hunt_refleak is not None) and (not self.num_workers):
            # gh-109739: WindowsLoadTracker thread interferes with refleak check
            use_load_tracker = False
        else:
            # WindowsLoadTracker is only needed on Windows
            use_load_tracker = MS_WINDOWS

        if use_load_tracker:
            self.logger.start_load_tracker()
        try:
            if self.num_workers:
                self._run_tests_mp(runtests, self.num_workers)
            else:
                self.run_tests_sequentially(runtests)

            coverage = self.results.get_coverage_results()
            self.display_result(runtests)

            if self.want_rerun and self.results.need_rerun():
                self.rerun_failed_tests(runtests)

            if self.want_bisect and self.results.need_rerun():
                self.run_bisect(runtests)
        finally:
            if use_load_tracker:
                self.logger.stop_load_tracker()

        self.display_summary()
        self.finalize_tests(coverage)

        return self.results.get_exitcode(self.fail_env_changed,
                                         self.fail_rerun)

    def run_tests(self, selected: TestTuple, tests: TestList | None) -> int:
        os.makedirs(self.tmp_dir, exist_ok=True)
        work_dir = get_work_dir(self.tmp_dir)

        # Put a timeout on Python exit
        with exit_timeout():
            # Run the tests in a context manager that temporarily changes the
            # CWD to a temporary and writable directory. If it's not possible
            # to create or change the CWD, the original CWD will be used.
            # The original CWD is available from os_helper.SAVEDCWD.
            with os_helper.temp_cwd(work_dir, quiet=True):
                # When using multiprocessing, worker processes will use
                # work_dir as their parent temporary directory. So when the
                # main process exit, it removes also subdirectories of worker
                # processes.
                return self._run_tests(selected, tests)

    def _add_cross_compile_opts(self, regrtest_opts):
        # WASM/WASI buildbot builders pass multiple PYTHON environment
        # variables such as PYTHONPATH and _PYTHON_HOSTRUNNER.
        keep_environ = bool(self.python_cmd)
        environ = None

        # Are we using cross-compilation?
        cross_compile = is_cross_compiled()

        # Get HOSTRUNNER
        hostrunner = get_host_runner()

        if cross_compile:
            # emulate -E, but keep PYTHONPATH + cross compile env vars,
            # so test executable can load correct sysconfigdata file.
            keep = {
                '_PYTHON_PROJECT_BASE',
                '_PYTHON_HOST_PLATFORM',
                '_PYTHON_SYSCONFIGDATA_NAME',
                "_PYTHON_SYSCONFIGDATA_PATH",
                'PYTHONPATH'
            }
            old_environ = os.environ
            new_environ = {
                name: value for name, value in os.environ.items()
                if not name.startswith(('PYTHON', '_PYTHON')) or name in keep
            }
            # Only set environ if at least one variable was removed
            if new_environ != old_environ:
                environ = new_environ
            keep_environ = True

        if cross_compile and hostrunner:
            if self.num_workers == 0 and not self.single_process:
                # For now use only two cores for cross-compiled builds;
                # hostrunner can be expensive.
                regrtest_opts.extend(['-j', '2'])

            # If HOSTRUNNER is set and -p/--python option is not given, then
            # use hostrunner to execute python binary for tests.
            if not self.python_cmd:
                buildpython = sysconfig.get_config_var("BUILDPYTHON")
                python_cmd = f"{hostrunner} {buildpython}"
                regrtest_opts.extend(["--python", python_cmd])
                keep_environ = True

        return (environ, keep_environ)

    def _add_ci_python_opts(self, python_opts, keep_environ):
        # --fast-ci and --slow-ci add options to Python:
        # "-u -W default -bb -E"

        # Unbuffered stdout and stderr
        if not sys.stdout.write_through:
            python_opts.append('-u')

        # Add warnings filter 'error'
        if 'default' not in sys.warnoptions:
            python_opts.extend(('-W', 'error'))

        # Error on bytes/str comparison
        if sys.flags.bytes_warning < 2:
            python_opts.append('-bb')

        if not keep_environ:
            # Ignore PYTHON* environment variables
            if not sys.flags.ignore_environment:
                python_opts.append('-E')

    def _execute_python(self, cmd, environ):
        # Make sure that messages before execv() are logged
        sys.stdout.flush()
        sys.stderr.flush()

        cmd_text = shlex.join(cmd)
        try:
            print(f"+ {cmd_text}", flush=True)

            if hasattr(os, 'execv') and not MS_WINDOWS:
                os.execv(cmd[0], cmd)
                # On success, execv() do no return.
                # On error, it raises an OSError.
            else:
                import subprocess
                with subprocess.Popen(cmd, env=environ) as proc:
                    try:
                        proc.wait()
                    except KeyboardInterrupt:
                        # There is no need to call proc.terminate(): on CTRL+C,
                        # SIGTERM is also sent to the child process.
                        try:
                            proc.wait(timeout=EXIT_TIMEOUT)
                        except subprocess.TimeoutExpired:
                            proc.kill()
                            proc.wait()
                            sys.exit(EXITCODE_INTERRUPTED)

                sys.exit(proc.returncode)
        except Exception as exc:
            print_warning(f"Failed to change Python options: {exc!r}\n"
                          f"Command: {cmd_text}")
            # continue executing main()

    def _add_python_opts(self) -> None:
        python_opts: list[str] = []
        regrtest_opts: list[str] = []

        environ, keep_environ = self._add_cross_compile_opts(regrtest_opts)
        if self.ci_mode:
            self._add_ci_python_opts(python_opts, keep_environ)

        if (not python_opts) and (not regrtest_opts) and (environ is None):
            # Nothing changed: nothing to do
            return

        # Create new command line
        cmd = list(sys.orig_argv)
        if python_opts:
            cmd[1:1] = python_opts
        if regrtest_opts:
            cmd.extend(regrtest_opts)
        cmd.append("--dont-add-python-opts")

        self._execute_python(cmd, environ)

    def _init(self):
        setup_process()

        if self.junit_filename and not os.path.isabs(self.junit_filename):
            self.junit_filename = os.path.abspath(self.junit_filename)

        strip_py_suffix(self.cmdline_args)

        self._tmp_dir = get_temp_dir(self._tmp_dir)

    @property
    def tmp_dir(self) -> StrPath:
        if self._tmp_dir is None:
            raise ValueError(
                "Should never use `.tmp_dir` before calling `.main()`"
            )
        return self._tmp_dir

    def main(self, tests: TestList | None = None) -> NoReturn:
        if self.want_add_python_opts:
            self._add_python_opts()

        self._init()

        if self.want_cleanup:
            cleanup_temp_dir(self.tmp_dir)
            sys.exit(0)

        if self.want_wait:
            input("Press any key to continue...")

        setup_test_dir(self.test_dir)
        selected, tests = self.find_tests(tests)

        exitcode = 0
        if self.want_list_tests:
            self.list_tests(selected)
        elif self.want_list_cases:
            list_cases(selected,
                       match_tests=self.match_tests,
                       test_dir=self.test_dir)
        else:
            exitcode = self.run_tests(selected, tests)

        sys.exit(exitcode)


def main(tests=None, _add_python_opts=False, **kwargs) -> NoReturn:
    """Run the Python suite."""
    ns = _parse_args(sys.argv[1:], **kwargs)
    Regrtest(ns, _add_python_opts=_add_python_opts).main(tests=tests)
