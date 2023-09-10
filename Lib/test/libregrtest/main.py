import faulthandler
import locale
import os
import platform
import random
import re
import sys
import sysconfig
import tempfile
import time
import unittest
from test.libregrtest.cmdline import _parse_args, Namespace
from test.libregrtest.logger import Logger
from test.libregrtest.runtest import (
    findtests, split_test_packages, run_single_test, abs_module_name,
    PROGRESS_MIN_TIME, State, RunTests, HuntRefleak,
    FilterTuple, TestList, StrPath, StrJSON, TestName)
from test.libregrtest.setup import setup_tests, setup_test_dir
from test.libregrtest.pgo import setup_pgo_tests
from test.libregrtest.results import TestResults
from test.libregrtest.utils import (strip_py_suffix, count, format_duration,
                                    printlist, get_build_info)
from test import support
from test.support import os_helper
from test.support import threading_helper


# bpo-38203: Maximum delay in seconds to exit Python (call Py_Finalize()).
# Used to protect against threading._shutdown() hang.
# Must be smaller than buildbot "1200 seconds without output" limit.
EXIT_TIMEOUT = 120.0


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
    def __init__(self, ns: Namespace):
        self.logger = Logger()

        # Actions
        self.want_header: bool = ns.header
        self.want_list_tests: bool = ns.list_tests
        self.want_list_cases: bool = ns.list_cases
        self.want_wait: bool = ns.wait
        self.want_cleanup: bool = ns.cleanup
        self.want_rerun: bool = ns.rerun
        self.want_run_leaks: bool = ns.runleaks

        # Select tests
        if ns.match_tests:
            self.match_tests: FilterTuple = tuple(ns.match_tests)
        else:
            self.match_tests = None
        if ns.ignore_tests:
            self.ignore_tests: FilterTuple = tuple(ns.ignore_tests)
        else:
            self.ignore_tests = None
        self.exclude: bool = ns.exclude
        self.fromfile: StrPath | None = ns.fromfile
        self.starting_test: TestName | None = ns.start
        self.cmdline_args: TestList = ns.args

        # Workers
        if ns.use_mp is None:
            num_workers = 0  # run sequentially
        elif ns.use_mp <= 0:
            num_workers = -1  # use the number of CPUs
        else:
            num_workers = ns.use_mp
        self.num_workers: int = num_workers
        self.worker_json: StrJSON | None = ns.worker_json

        # Options to run tests
        self.fail_fast: bool = ns.failfast
        self.fail_env_changed: bool = ns.fail_env_changed
        self.fail_rerun: bool = ns.fail_rerun
        self.forever: bool = ns.forever
        self.randomize: bool = ns.randomize
        self.random_seed: int | None = ns.random_seed
        self.pgo: bool = ns.pgo
        self.pgo_extended: bool = ns.pgo_extended
        self.output_on_failure: bool = ns.verbose3
        self.timeout: float | None = ns.timeout
        self.verbose: bool = ns.verbose
        self.quiet: bool = ns.quiet
        if ns.huntrleaks:
            self.hunt_refleak: HuntRefleak = HuntRefleak(*ns.huntrleaks)
        else:
            self.hunt_refleak = None
        self.test_dir: StrPath | None = ns.testdir
        self.junit_filename: StrPath | None = ns.xmlpath
        self.memory_limit: str | None = ns.memlimit
        self.gc_threshold: int | None = ns.threshold
        self.use_resources: list[str] = ns.use_resources
        self.python_cmd: list[str] | None = ns.python
        self.coverage: bool = ns.trace
        self.coverage_dir: StrPath | None = ns.coverdir
        self.tmp_dir: StrPath | None = ns.tempdir

        # tests
        self.tests = []
        self.selected: TestList = []
        self.first_runtests: RunTests | None = None

        # test results
        self.results: TestResults = TestResults()

        self.first_state: str | None = None

        # used by --slowest
        self.print_slowest: bool = ns.print_slow

        # used to display the progress bar "[ 3/100]"
        self.start_time = time.perf_counter()
        self.test_count_text = ''
        self.test_count_width = 1

        # used by --single
        self.single_test_run: bool = ns.single
        self.next_single_test: TestName | None = None
        self.next_single_filename: StrPath | None = None

    def log(self, line=''):
        self.logger.log(line)

    def display_progress(self, test_index, text):
        if self.quiet:
            return

        # "[ 51/405/1] test_tcl passed"
        line = f"{test_index:{self.test_count_width}}{self.test_count_text}"
        fails = len(self.results.bad) + len(self.results.env_changed)
        if fails and not self.pgo:
            line = f"{line}/{fails}"
        self.log(f"[{line}] {text}")

    def find_tests(self):
        if self.single_test_run:
            self.next_single_filename = os.path.join(self.tmp_dir, 'pynexttest')
            try:
                with open(self.next_single_filename, 'r') as fp:
                    next_test = fp.read().strip()
                    self.tests = [next_test]
            except OSError:
                pass

        if self.fromfile:
            self.tests = []
            # regex to match 'test_builtin' in line:
            # '0:00:00 [  4/400] test_builtin -- test_dict took 1 sec'
            regex = re.compile(r'\btest_[a-zA-Z0-9_]+\b')
            with open(os.path.join(os_helper.SAVEDCWD, self.fromfile)) as fp:
                for line in fp:
                    line = line.split('#', 1)[0]
                    line = line.strip()
                    match = regex.search(line)
                    if match is not None:
                        self.tests.append(match.group())

        strip_py_suffix(self.tests)

        if self.pgo:
            # add default PGO tests if no tests are specified
            setup_pgo_tests(self.cmdline_args, self.pgo_extended)

        exclude_tests = set()
        if self.exclude:
            for arg in self.cmdline_args:
                exclude_tests.add(arg)
            self.cmdline_args = []

        alltests = findtests(testdir=self.test_dir,
                             exclude=exclude_tests)

        if not self.fromfile:
            self.selected = self.tests or self.cmdline_args
            if self.selected:
                self.selected = split_test_packages(self.selected)
            else:
                self.selected = alltests
        else:
            self.selected = self.tests

        if self.single_test_run:
            self.selected = self.selected[:1]
            try:
                pos = alltests.index(self.selected[0])
                self.next_single_test = alltests[pos + 1]
            except IndexError:
                pass

        # Remove all the selected tests that precede start if it's set.
        if self.starting_test:
            try:
                del self.selected[:self.selected.index(self.starting_test)]
            except ValueError:
                print(f"Cannot find starting test: {self.starting_test}")
                sys.exit(1)

        if self.randomize:
            if self.random_seed is None:
                self.random_seed = random.randrange(100_000_000)
            random.seed(self.random_seed)
            random.shuffle(self.selected)

    @staticmethod
    def list_tests(tests: TestList):
        for name in tests:
            print(name)

    def _list_cases(self, suite):
        for test in suite:
            if isinstance(test, unittest.loader._FailedTest):
                continue
            if isinstance(test, unittest.TestSuite):
                self._list_cases(test)
            elif isinstance(test, unittest.TestCase):
                if support.match_test(test):
                    print(test.id())

    def list_cases(self):
        support.verbose = False
        support.set_match_tests(self.match_tests, self.ignore_tests)

        skipped = []
        for test_name in self.selected:
            module_name = abs_module_name(test_name, self.test_dir)
            try:
                suite = unittest.defaultTestLoader.loadTestsFromName(module_name)
                self._list_cases(suite)
            except unittest.SkipTest:
                skipped.append(test_name)

        if skipped:
            sys.stdout.flush()
            stderr = sys.stderr
            print(file=stderr)
            print(count(len(skipped), "test"), "skipped:", file=stderr)
            printlist(skipped, file=stderr)

    def _rerun_failed_tests(self, runtests: RunTests):
        # Configure the runner to re-run tests
        if self.num_workers == 0:
            self.num_workers = 1

        tests, match_tests_dict = self.results.prepare_rerun()

        # Re-run failed tests
        self.log(f"Re-running {len(tests)} failed tests in verbose mode in subprocesses")
        runtests = runtests.copy(
            tests=tests,
            rerun=True,
            verbose=True,
            forever=False,
            fail_fast=False,
            match_tests_dict=match_tests_dict,
            output_on_failure=False)
        self.logger.set_tests(runtests)
        self._run_tests_mp(runtests, self.num_workers)
        return runtests

    def rerun_failed_tests(self, runtests: RunTests):
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
            print(count(len(self.results.bad), 'test'), "failed again:")
            printlist(self.results.bad)

        self.display_result(rerun_runtests)

    def display_result(self, runtests):
        # If running the test suite for PGO then no one cares about results.
        if runtests.pgo:
            return

        state = self.get_state()
        print()
        print(f"== Tests result: {state} ==")

        self.results.display_result(self.selected, self.quiet, self.print_slowest)

    def run_test(self, test_name: TestName, runtests: RunTests, tracer):
        if tracer is not None:
            # If we're tracing code coverage, then we don't exit with status
            # if on a false return value from main.
            cmd = ('result = run_single_test(test_name, runtests)')
            namespace = dict(locals())
            tracer.runctx(cmd, globals=globals(), locals=namespace)
            result = namespace['result']
        else:
            result = run_single_test(test_name, runtests)

        self.results.accumulate_result(result, runtests)

        return result

    def run_tests_sequentially(self, runtests):
        if self.coverage:
            import trace
            tracer = trace.Trace(trace=False, count=True)
        else:
            tracer = None

        save_modules = sys.modules.keys()

        msg = "Run tests sequentially"
        if runtests.timeout:
            msg += " (timeout: %s)" % format_duration(runtests.timeout)
        self.log(msg)

        previous_test = None
        tests_iter = runtests.iter_tests()
        for test_index, test_name in enumerate(tests_iter, 1):
            start_time = time.perf_counter()

            text = test_name
            if previous_test:
                text = '%s -- %s' % (text, previous_test)
            self.display_progress(test_index, text)

            result = self.run_test(test_name, runtests, tracer)

            # Unload the newly imported modules (best effort finalization)
            for module in sys.modules.keys():
                if module not in save_modules and module.startswith("test."):
                    support.unload(module)

            if result.must_stop(self.fail_fast, self.fail_env_changed):
                break

            previous_test = str(result)
            test_time = time.perf_counter() - start_time
            if test_time >= PROGRESS_MIN_TIME:
                previous_test = "%s in %s" % (previous_test, format_duration(test_time))
            elif result.state == State.PASSED:
                # be quiet: say nothing if the test passed shortly
                previous_test = None

        if previous_test:
            print(previous_test)

        return tracer

    @staticmethod
    def display_header():
        # Print basic platform information
        print("==", platform.python_implementation(), *sys.version.split())
        print("==", platform.platform(aliased=True),
                      "%s-endian" % sys.byteorder)
        print("== Python build:", ' '.join(get_build_info()))
        print("== cwd:", os.getcwd())
        cpu_count = os.cpu_count()
        if cpu_count:
            print("== CPU count:", cpu_count)
        print("== encodings: locale=%s, FS=%s"
              % (locale.getencoding(), sys.getfilesystemencoding()))

        # This makes it easier to remember what to set in your local
        # environment when trying to reproduce a sanitizer failure.
        asan = support.check_sanitizer(address=True)
        msan = support.check_sanitizer(memory=True)
        ubsan = support.check_sanitizer(ub=True)
        sanitizers = []
        if asan:
            sanitizers.append("address")
        if msan:
            sanitizers.append("memory")
        if ubsan:
            sanitizers.append("undefined behavior")
        if not sanitizers:
            return

        print(f"== sanitizers: {', '.join(sanitizers)}")
        for sanitizer, env_var in (
            (asan, "ASAN_OPTIONS"),
            (msan, "MSAN_OPTIONS"),
            (ubsan, "UBSAN_OPTIONS"),
        ):
            options= os.environ.get(env_var)
            if sanitizer and options is not None:
                print(f"== {env_var}={options!r}")

    def get_state(self):
        state = self.results.get_state(self.fail_env_changed)
        if self.first_state:
            state = f'{self.first_state} then {state}'
        return state

    def _run_tests_mp(self, runtests: RunTests, num_workers: int) -> None:
        from test.libregrtest.runtest_mp import RunWorkers
        RunWorkers(self, runtests, num_workers).run()

    def finalize_tests(self, tracer):
        if self.next_single_filename:
            if self.next_single_test:
                with open(self.next_single_filename, 'w') as fp:
                    fp.write(self.next_single_test + '\n')
            else:
                os.unlink(self.next_single_filename)

        if tracer is not None:
            results = tracer.results()
            results.write_results(show_missing=True, summary=True,
                                  coverdir=self.coverage_dir)

        if self.want_run_leaks:
            os.system("leaks %d" % os.getpid())

        if self.junit_filename:
            self.results.write_junit(self.junit_filename)

    def display_summary(self):
        duration = time.perf_counter() - self.logger.start_time
        filtered = bool(self.match_tests) or bool(self.ignore_tests)

        # Total duration
        print()
        print("Total duration: %s" % format_duration(duration))

        self.results.display_summary(self.first_runtests, filtered)

        # Result
        state = self.get_state()
        print(f"Result: {state}")

    @staticmethod
    def fix_umask():
        if support.is_emscripten:
            # Emscripten has default umask 0o777, which breaks some tests.
            # see https://github.com/emscripten-core/emscripten/issues/17269
            old_mask = os.umask(0)
            if old_mask == 0o777:
                os.umask(0o027)
            else:
                os.umask(old_mask)

    @staticmethod
    def select_temp_dir(tmp_dir):
        if tmp_dir:
            tmp_dir = os.path.expanduser(tmp_dir)
        else:
            # When tests are run from the Python build directory, it is best practice
            # to keep the test files in a subfolder.  This eases the cleanup of leftover
            # files using the "make distclean" command.
            if sysconfig.is_python_build():
                tmp_dir = sysconfig.get_config_var('abs_builddir')
                if tmp_dir is None:
                    # bpo-30284: On Windows, only srcdir is available. Using
                    # abs_builddir mostly matters on UNIX when building Python
                    # out of the source tree, especially when the source tree
                    # is read only.
                    tmp_dir = sysconfig.get_config_var('srcdir')
                tmp_dir = os.path.join(tmp_dir, 'build')
            else:
                tmp_dir = tempfile.gettempdir()

        return os.path.abspath(tmp_dir)

    def is_worker(self):
        return (self.worker_json is not None)

    @staticmethod
    def make_temp_dir(tmp_dir: StrPath, is_worker: bool):
        os.makedirs(tmp_dir, exist_ok=True)

        # Define a writable temp dir that will be used as cwd while running
        # the tests. The name of the dir includes the pid to allow parallel
        # testing (see the -j option).
        # Emscripten and WASI have stubbed getpid(), Emscripten has only
        # milisecond clock resolution. Use randint() instead.
        if sys.platform in {"emscripten", "wasi"}:
            nounce = random.randint(0, 1_000_000)
        else:
            nounce = os.getpid()

        if is_worker:
            work_dir = 'test_python_worker_{}'.format(nounce)
        else:
            work_dir = 'test_python_{}'.format(nounce)
        work_dir += os_helper.FS_NONASCII
        work_dir = os.path.join(tmp_dir, work_dir)
        return work_dir

    @staticmethod
    def cleanup_temp_dir(tmp_dir: StrPath):
        import glob

        path = os.path.join(glob.escape(tmp_dir), 'test_python_*')
        print("Cleanup %s directory" % tmp_dir)
        for name in glob.glob(path):
            if os.path.isdir(name):
                print("Remove directory: %s" % name)
                os_helper.rmtree(name)
            else:
                print("Remove file: %s" % name)
                os_helper.unlink(name)

    def main(self, tests: TestList | None = None):
        if self.junit_filename and not os.path.isabs(self.junit_filename):
            self.junit_filename = os.path.abspath(self.junit_filename)

        self.tests = tests

        strip_py_suffix(self.cmdline_args)

        self.tmp_dir = self.select_temp_dir(self.tmp_dir)

        self.fix_umask()

        if self.want_cleanup:
            self.cleanup_temp_dir(self.tmp_dir)
            sys.exit(0)

        work_dir = self.make_temp_dir(self.tmp_dir, self.is_worker())

        try:
            # Run the tests in a context manager that temporarily changes the
            # CWD to a temporary and writable directory. If it's not possible
            # to create or change the CWD, the original CWD will be used.
            # The original CWD is available from os_helper.SAVEDCWD.
            with os_helper.temp_cwd(work_dir, quiet=True):
                # When using multiprocessing, worker processes will use
                # work_dir as their parent temporary directory. So when the
                # main process exit, it removes also subdirectories of worker
                # processes.

                self._main()
        except SystemExit as exc:
            # bpo-38203: Python can hang at exit in Py_Finalize(), especially
            # on threading._shutdown() call: put a timeout
            if threading_helper.can_start_thread:
                faulthandler.dump_traceback_later(EXIT_TIMEOUT, exit=True)

            sys.exit(exc.code)

    def create_run_tests(self):
        return RunTests(
            tuple(self.selected),
            fail_fast=self.fail_fast,
            match_tests=self.match_tests,
            ignore_tests=self.ignore_tests,
            forever=self.forever,
            pgo=self.pgo,
            pgo_extended=self.pgo_extended,
            output_on_failure=self.output_on_failure,
            timeout=self.timeout,
            verbose=self.verbose,
            quiet=self.quiet,
            hunt_refleak=self.hunt_refleak,
            test_dir=self.test_dir,
            junit_filename=self.junit_filename,
            memory_limit=self.memory_limit,
            gc_threshold=self.gc_threshold,
            use_resources=self.use_resources,
            python_cmd=self.python_cmd,
        )

    def run_tests(self) -> int:
        if self.hunt_refleak and self.hunt_refleak.warmups < 3:
            msg = ("WARNING: Running tests with --huntrleaks/-R and "
                   "less than 3 warmup repetitions can give false positives!")
            print(msg, file=sys.stdout, flush=True)

        if self.num_workers < 0:
            # Use all CPUs + 2 extra worker processes for tests
            # that like to sleep
            self.num_workers = (os.cpu_count() or 1) + 2

        # For a partial run, we do not need to clutter the output.
        if (self.want_header
            or not(self.pgo or self.quiet or self.single_test_run
                   or self.tests or self.cmdline_args)):
            self.display_header()

        if self.randomize:
            print("Using random seed", self.random_seed)

        runtests = self.create_run_tests()
        self.first_runtests = runtests
        self.logger.set_tests(runtests)

        setup_tests(runtests)

        self.logger.start_load_tracker()
        try:
            if self.num_workers:
                self._run_tests_mp(runtests, self.num_workers)
                tracer = None
            else:
                tracer = self.run_tests_sequentially(runtests)

            self.display_result(runtests)

            if self.want_rerun and self.results.need_rerun():
                self.rerun_failed_tests(runtests)
        finally:
            self.logger.stop_load_tracker()

        self.display_summary()
        self.finalize_tests(tracer)

        return self.results.get_exitcode(self.fail_env_changed,
                                         self.fail_rerun)

    def _main(self):
        if self.is_worker():
            from test.libregrtest.runtest_mp import worker_process
            worker_process(self.worker_json)
            return

        if self.want_wait:
            input("Press any key to continue...")

        setup_test_dir(self.test_dir)
        self.find_tests()

        exitcode = 0
        if self.want_list_tests:
            self.list_tests(self.selected)
        elif self.want_list_cases:
            self.list_cases()
        else:
            exitcode = self.run_tests()

        sys.exit(exitcode)


def main(tests=None, **kwargs):
    """Run the Python suite."""
    ns = _parse_args(sys.argv[1:], **kwargs)
    Regrtest(ns).main(tests=tests)
