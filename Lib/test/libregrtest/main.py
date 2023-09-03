import faulthandler
import os
import random
import re
import sys
import time

from test import support
from test.support import os_helper
from test.support import threading_helper

from . import (
    TestsTuple, FilterTuple, State, RunTests, TestsList, Results)
from .cmdline import _parse_args
from .findtests import (
    findtests, split_test_packages, list_cases)
from .logger import Logger
from .single import run_single_test, PROGRESS_MIN_TIME
from .setup import setup_tests, setup_test_dir
from .pgo import PGO_TESTS
from .utils import (
    strip_py_suffix, format_duration, count, printlist,
    fix_umask, display_header,
    select_temp_dir, make_temp_dir, cleanup_directory)


# bpo-38203: maximum delay in seconds to exit python (call py_finalize()).
# used to protect against threading._shutdown() hang.
# must be smaller than buildbot "1200 seconds without output" limit.
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
    def __init__(self, ns):
        # Actions
        self.want_header = ns.header
        self.want_list_tests = ns.list_tests
        self.want_list_cases = ns.list_cases
        self.want_wait = ns.wait
        self.want_cleanup = ns.cleanup
        self.want_rerun = ns.rerun

        # Select tests
        if ns.match_tests:
            self.match_tests: FilterTuple = tuple(ns.match_tests)
        else:
            self.match_tests = None
        if ns.ignore_tests:
            self.ignore_tests: FilterTuple = tuple(ns.ignore_tests)
        else:
            self.ignore_tests = None
        self.cmdline_args = ns.args
        self.exclude = ns.exclude
        self.fromfile = ns.fromfile
        self.use_resources = ns.use_resources
        self.starting_test = ns.start

        # Run tests
        self.num_processes = ns.use_mp
        self.worker_json = ns.worker_json
        self.coverage = ns.coverage
        self.coverage_dir = ns.coverdir

        # Options to run tests
        self.fail_fast = ns.failfast
        self.forever = ns.forever
        self.quiet = ns.quiet
        self.runleaks = ns.runleaks
        self.print_slowest = ns.print_slow
        self.pgo = ns.pgo
        self.pgo_extended = ns.pgo_extended
        self.randomize = ns.randomize
        self.random_seed = ns.random_seed
        self.gc_threshold = ns.threshold
        self.output_on_failure = ns.verbose3
        self.verbose = ns.verbose
        self.python_executable = ns.python
        self.test_dir = ns.testdir
        self.timeout = ns.timeout
        self.orig_tempdir = ns.tempdir
        self.huntrleaks = ns.huntrleaks
        self.memlimit = ns.memlimit

        # Configure the exit code
        self.fail_env_changed = ns.fail_env_changed
        self.fail_rerun = ns.fail_rerun

        # Used by --single
        self.single_test_run = ns.single
        self.next_single_test = None
        self.next_single_filename = None

        # Tests
        self.results = Results(xmlpath=ns.xmlpath)
        self.first_runtests: RunTests | None = None
        self.first_state: str | None = None

        # Misc
        self.tmp_dir = None
        self.logger = Logger(self.pgo)
        self.log = self.logger.log

    def find_tests(self, tests: TestsList | None,
                   test_dir: str) -> tuple[TestsTuple, TestsTuple | None]:
        if tests is not None:
            tests = list(tests)
        selected = []

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
            if not self.cmdline_args and not self.pgo_extended:
                # run default set of tests for PGO training
                self.cmdline_args[:] = PGO_TESTS

        exclude_tests = set()
        if self.exclude:
            for arg in self.cmdline_args:
                exclude_tests.add(arg)
            self.cmdline_args = []

        alltests = findtests(testdir=self.test_dir, exclude=exclude_tests)

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

        selected = tuple(selected)
        if tests is not None:
            tests = tuple(tests)
        return (selected, tests)

    def _rerun_failed_tests(self, runtests: RunTests) -> None:
        # Get tests to re-run
        tests, match_tests_dict = self.results.prepare_rerun()

        # Re-run failed tests
        runtests = runtests.copy(
            tests=tests,
            rerun=True,
            verbose=True,
            match_tests_dict=match_tests_dict,
            output_on_failure=False,
            forever=False,
            fail_fast=False)
        self.logger.set_tests(runtests)

        if not self.num_processes:
            # Always run tests in subprocesses, at least with 1 worker
            # process
            self.num_processes = 1

        self.log(f"Re-running {len(runtests.tests)} failed tests "
                 f"in verbose mode with {self.num_processes} "
                 f"worker processes")
        self._run_tests_mp(runtests)

    def rerun_failed_tests(self, runtests: RunTests):
        if self.python_executable:
            # Temp patch for https://github.com/python/cpython/issues/94052
            self.log(
                "Re-running failed tests is not supported with --python "
                "host runner option."
            )
            return

        self.first_state = self.get_tests_state()
        print()
        self._rerun_failed_tests(runtests)

        bad = self.results.bad
        if bad:
            print(count(len(bad), 'test'), "failed again:")
            printlist(bad)

        self.display_result(runtests.tests)

    def display_result(self, tests):
        # If running the test suite for PGO then no one cares about results.
        if self.pgo:
            return

        self.results.display_result(
            tests,
            fail_env_changed=self.fail_env_changed,
            first_state=self.first_state,
            quiet=self.quiet,
            print_slowest=self.print_slowest)

    @staticmethod
    def run_test(test_name, results, runtests, tracer):
        if tracer is not None:
            # If we're tracing code coverage, then we don't exit with status
            # if on a false return value from main.
            cmd = ('result = run_single_test(test_name, runtests); '
                   'results.accumulate_result(result, runtests)')
            local_namespace = dict(locals())
            tracer.runctx(cmd, globals=globals(), locals=local_namespace)
            result = local_namespace['result']
        else:
            result = run_single_test(test_name, runtests)
            results.accumulate_result(result, runtests)
        return result

    def run_tests_sequentially(self, runtests: RunTests):
        fail_fast = runtests.fail_fast
        fail_env_changed = runtests.fail_env_changed

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
            self.logger.display_progress(test_index, text, self.results, runtests)

            result = self.run_test(test_name, self.results, runtests, tracer)

            # Unload the newly imported modules (best effort finalization)
            for module in sys.modules.keys():
                if module not in save_modules and module.startswith("test."):
                    support.unload(module)

            if result.must_stop(fail_fast, fail_env_changed):
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

    def get_tests_state(self):
        return self.results.get_state(
            fail_env_changed=self.fail_env_changed,
            first_state=self.first_state)

    def _run_tests_mp(self, runtests: RunTests) -> None:
        from .mp_runner import MultiprocessTestRunner

        runner = MultiprocessTestRunner(self.results, runtests,
                                        self.logger, self.num_processes)
        runner.run_tests()

    def run_tests(self, runtests: RunTests):
        self.logger.set_tests(runtests)

        setup_tests(runtests)

        if self.num_processes:
            self._run_tests_mp(runtests)
            tracer = None
        else:
            tracer = self.run_tests_sequentially(runtests)
        return tracer

    def finalize_tests(self, tracer) -> None:
        self.display_summary()

        if self.next_single_filename:
            if self.next_single_test:
                with open(self.next_single_filename, 'w') as fp:
                    fp.write(self.next_single_test + '\n')
            else:
                os.unlink(self.next_single_filename)

        if self.runleaks:
            os.system("leaks %d" % os.getpid())

        self.results.write_junit()

        if tracer:
            results = tracer.results()
            results.write_results(show_missing=True, summary=True,
                                  coverdir=self.coverage_dir)

    def display_summary(self):
        results = self.results
        duration = self.logger.get_time()
        filtered = (bool(self.match_tests) or bool(self.ignore_tests))

        # Total duration
        print()
        print("Total duration: %s" % format_duration(duration))

        # Total tests
        total = results.total_stats
        text = f'run={total.tests_run:,}'
        if filtered:
            text = f"{text} (filtered)"
        stats = [text]
        if total.failures:
            stats.append(f'failures={total.failures:,}')
        if total.skipped:
            stats.append(f'skipped={total.skipped:,}')
        print(f"Total tests: {' '.join(stats)}")

        # Total test files
        all_tests = [results.good, results.bad, results.rerun,
                     results.skipped,
                     results.environment_changed, results.run_no_tests]
        run = sum(map(len, all_tests))
        text = f'run={run}'
        if not self.forever:
            ntest = len(self.first_runtests.tests)
            text = f"{text}/{ntest}"
        if filtered:
            text = f"{text} (filtered)"
        report = [text]
        for name, tests in (
            ('failed', results.bad),
            ('env_changed', results.environment_changed),
            ('skipped', results.skipped),
            ('resource_denied', results.resource_denied),
            ('rerun', results.rerun),
            ('run_no_tests', results.run_no_tests),
        ):
            if tests:
                report.append(f'{name}={len(tests)}')
        print(f"Total test files: {' '.join(report)}")

        # Result
        state = self.get_tests_state()
        print(f"Result: {state}")

    def main(self, tests: TestsList | None = None):
        strip_py_suffix(self.cmdline_args)

        self.tmp_dir = select_temp_dir(self.orig_tempdir)
        fix_umask()

        if self.want_cleanup:
            cleanup_directory(self.tmp_dir)
            sys.exit(0)

        try:
            # Run the tests in a context manager that temporarily changes the
            # CWD to a temporary and writable directory. If it's not possible
            # to create or change the CWD, the original CWD will be used. The
            # original CWD is available from os_helper.SAVEDCWD.
            #
            # When using multiprocessing, worker processes will use
            # test_cwd as their parent temporary directory. So when the
            # main process exit, it removes also subdirectories of worker
            # processes.
            is_worker = (self.worker_json is not None)
            test_cwd = make_temp_dir(self.tmp_dir, is_worker)
            with os_helper.temp_cwd(test_cwd, quiet=True):
                self._main(tests)
        finally:
            # bpo-38203: Python can hang at exit in Py_Finalize(), especially
            # on threading._shutdown() call: put a timeout
            if threading_helper.can_start_thread:
                faulthandler.dump_traceback_later(EXIT_TIMEOUT, exit=True)

    def get_exitcode(self):
        return self.results.get_exitcode(
            fail_env_changed=self.fail_env_changed,
            fail_rerun=self.fail_rerun)

    def create_run_tests(self, tests):
        return RunTests(
            tuple(tests),
            timeout=self.timeout,
            quiet=self.quiet,
            fail_env_changed=self.fail_env_changed,
            match_tests=self.match_tests,
            ignore_tests=self.ignore_tests,
            pgo=self.pgo,
            pgo_extended=self.pgo_extended,
            use_junit=self.results.use_junit(),
            test_dir=self.test_dir,
            huntrleaks=self.huntrleaks,
            memlimit=self.memlimit,
            gc_threshold=self.gc_threshold,
            use_resources=self.use_resources,
            python_executable=self.python_executable,
            verbose=self.verbose,
            fail_fast=self.fail_fast,
            output_on_failure=self.output_on_failure,
            forever=self.forever)

    def action_run_tests(self, tests, self_tests):
        if self.huntrleaks:
            warmup, repetitions, _ = self.huntrleaks
            if warmup < 3:
                msg = ("WARNING: Running tests with --huntrleaks/-R and less than "
                        "3 warmup repetitions can give false positives!")
                print(msg, file=sys.stdout, flush=True)
                print()

        # For a partial run, we do not need to clutter the output.
        if (self.want_header
             or not(self.pgo or self.quiet or self.single_test_run
                    or self_tests or self.cmdline_args)):
            display_header()
        if self.randomize:
            print(f"Using random seed: {self.random_seed}")

        if self.num_processes is not None:
            if self.pgo:
                print("PGO: disable multiprocessing")
                self.num_processes = 0
            elif self.num_processes <= 0:
                # Use all cores + extras for tests that like to sleep
                self.num_processes = (os.cpu_count() or 1) + 2
        else:
            self.num_processes = 0

        # run tests
        runtests = self.create_run_tests(tests)
        tracer = self.run_tests(runtests)
        self.first_runtests = runtests
        self.display_result(runtests.tests)

        # if tests failed, re-run them in verbose mode
        if self.want_rerun and self.results.rerun_needed():
            self.rerun_failed_tests(runtests)

        self.finalize_tests(tracer)

    def _main(self, tests: TestsList | None):
        worker_json = self.worker_json
        if worker_json is not None:
            from .worker import worker_process
            worker_process(worker_json)
            return

        if self.want_wait:
            input("Press any key to continue...")

        if self.randomize:
            if self.random_seed is None:
                self.random_seed = random.randrange(100_000_000)
            random.seed(self.random_seed)

        setup_test_dir(self.test_dir)
        selected, tests = self.find_tests(tests, self.test_dir)

        if self.randomize:
            selected = list(selected)
            random.shuffle(selected)
            selected = tuple(selected)

        exitcode = 0
        if self.want_list_tests:
            for name in selected:
                print(name)
        elif self.want_list_cases:
            list_cases(
                selected,
                test_dir=self.test_dir,
                match_tests=self.match_tests,
                ignore_tests=self.ignore_tests)
        else:
            self.logger.start_load_tracker()
            try:
                self.action_run_tests(selected, tests)
            finally:
                self.logger.stop_load_tracker()
                exitcode = self.get_exitcode()

        sys.exit(exitcode)


def main(tests: TestsList | None = None, **kwargs):
    """Run the Python suite."""
    ns = _parse_args(sys.argv[1:], **kwargs)
    Regrtest(ns).main(tests)
