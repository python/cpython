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
from test.libregrtest.cmdline import _parse_args
from test.libregrtest.runtest import (
    findtests, split_test_packages, runtest, abs_module_name,
    PROGRESS_MIN_TIME, State, MatchTestsDict, RunTests)
from test.libregrtest.setup import setup_tests
from test.libregrtest.pgo import setup_pgo_tests
from test.libregrtest.utils import (strip_py_suffix, count, format_duration,
                                    printlist, get_build_info)
from test import support
from test.support import TestStats
from test.support import os_helper
from test.support import threading_helper


# bpo-38203: Maximum delay in seconds to exit Python (call Py_Finalize()).
# Used to protect against threading._shutdown() hang.
# Must be smaller than buildbot "1200 seconds without output" limit.
EXIT_TIMEOUT = 120.0

EXITCODE_BAD_TEST = 2
EXITCODE_ENV_CHANGED = 3
EXITCODE_NO_TESTS_RAN = 4
EXITCODE_RERUN_FAIL = 5
EXITCODE_INTERRUPTED = 130


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
    def __init__(self):
        # Namespace of command line options
        self.ns = None

        # tests
        self.tests = []
        self.selected = []
        self.all_runtests: list[RunTests] = []

        # test results
        self.good: list[str] = []
        self.bad: list[str] = []
        self.rerun_bad: list[str] = []
        self.skipped: list[str] = []
        self.resource_denied: list[str] = []
        self.environment_changed: list[str] = []
        self.run_no_tests: list[str] = []
        self.rerun: list[str] = []

        self.need_rerun: list[TestResult] = []
        self.first_state: str | None = None
        self.interrupted = False
        self.total_stats = TestStats()

        # used by --slow
        self.test_times = []

        # used by --coverage, trace.Trace instance
        self.tracer = None

        # used to display the progress bar "[ 3/100]"
        self.start_time = time.perf_counter()
        self.test_count_text = ''
        self.test_count_width = 1

        # used by --single
        self.next_single_test = None
        self.next_single_filename = None

        # used by --junit-xml
        self.testsuite_xml = None

        # misc
        self.win_load_tracker = None
        self.tmp_dir = None

    def get_executed(self):
        return (set(self.good) | set(self.bad) | set(self.skipped)
                | set(self.resource_denied) | set(self.environment_changed)
                | set(self.run_no_tests))

    def accumulate_result(self, result, rerun=False):
        fail_env_changed = self.ns.fail_env_changed
        test_name = result.test_name

        match result.state:
            case State.PASSED:
                self.good.append(test_name)
            case State.ENV_CHANGED:
                self.environment_changed.append(test_name)
            case State.SKIPPED:
                self.skipped.append(test_name)
            case State.RESOURCE_DENIED:
                self.resource_denied.append(test_name)
            case State.INTERRUPTED:
                self.interrupted = True
            case State.DID_NOT_RUN:
                self.run_no_tests.append(test_name)
            case _:
                if result.is_failed(fail_env_changed):
                    self.bad.append(test_name)
                    self.need_rerun.append(result)
                else:
                    raise ValueError(f"invalid test state: {result.state!r}")

        if result.has_meaningful_duration() and not rerun:
            self.test_times.append((result.duration, test_name))
        if result.stats is not None:
            self.total_stats.accumulate(result.stats)
        if rerun:
            self.rerun.append(test_name)

        xml_data = result.xml_data
        if xml_data:
            import xml.etree.ElementTree as ET
            for e in xml_data:
                try:
                    self.testsuite_xml.append(ET.fromstring(e))
                except ET.ParseError:
                    print(xml_data, file=sys.__stderr__)
                    raise

    def log(self, line=''):
        empty = not line

        # add the system load prefix: "load avg: 1.80 "
        load_avg = self.getloadavg()
        if load_avg is not None:
            line = f"load avg: {load_avg:.2f} {line}"

        # add the timestamp prefix:  "0:01:05 "
        test_time = time.perf_counter() - self.start_time

        mins, secs = divmod(int(test_time), 60)
        hours, mins = divmod(mins, 60)
        test_time = "%d:%02d:%02d" % (hours, mins, secs)

        line = f"{test_time} {line}"
        if empty:
            line = line[:-1]

        print(line, flush=True)

    def display_progress(self, test_index, text):
        quiet = self.ns.quiet
        pgo = self.ns.pgo
        if quiet:
            return

        # "[ 51/405/1] test_tcl passed"
        line = f"{test_index:{self.test_count_width}}{self.test_count_text}"
        fails = len(self.bad) + len(self.environment_changed)
        if fails and not pgo:
            line = f"{line}/{fails}"
        self.log(f"[{line}] {text}")

    def parse_args(self, kwargs):
        ns = _parse_args(sys.argv[1:], **kwargs)

        if ns.xmlpath:
            support.junit_xml_list = self.testsuite_xml = []

        strip_py_suffix(ns.args)

        if ns.huntrleaks:
            warmup, repetitions, _ = ns.huntrleaks
            if warmup < 1 or repetitions < 1:
                msg = ("Invalid values for the --huntrleaks/-R parameters. The "
                       "number of warmups and repetitions must be at least 1 "
                       "each (1:1).")
                print(msg, file=sys.stderr, flush=True)
                sys.exit(2)

        if ns.tempdir:
            ns.tempdir = os.path.expanduser(ns.tempdir)

        self.ns = ns

    def find_tests(self, tests):
        ns = self.ns
        single = ns.single
        fromfile = ns.fromfile
        pgo = ns.pgo
        exclude = ns.exclude
        test_dir = ns.testdir
        starting_test = ns.start
        randomize = ns.randomize

        self.tests = tests

        if single:
            self.next_single_filename = os.path.join(self.tmp_dir, 'pynexttest')
            try:
                with open(self.next_single_filename, 'r') as fp:
                    next_test = fp.read().strip()
                    self.tests = [next_test]
            except OSError:
                pass

        if fromfile:
            self.tests = []
            # regex to match 'test_builtin' in line:
            # '0:00:00 [  4/400] test_builtin -- test_dict took 1 sec'
            regex = re.compile(r'\btest_[a-zA-Z0-9_]+\b')
            with open(os.path.join(os_helper.SAVEDCWD, fromfile)) as fp:
                for line in fp:
                    line = line.split('#', 1)[0]
                    line = line.strip()
                    match = regex.search(line)
                    if match is not None:
                        self.tests.append(match.group())

        strip_py_suffix(self.tests)

        if pgo:
            # add default PGO tests if no tests are specified
            setup_pgo_tests(ns)

        exclude_tests = set()
        if exclude:
            for arg in ns.args:
                exclude_tests.add(arg)
            ns.args = []

        alltests = findtests(testdir=test_dir, exclude=exclude_tests)

        if not fromfile:
            self.selected = self.tests or ns.args
            if self.selected:
                self.selected = split_test_packages(self.selected)
            else:
                self.selected = alltests
        else:
            self.selected = self.tests

        if single:
            self.selected = self.selected[:1]
            try:
                pos = alltests.index(self.selected[0])
                self.next_single_test = alltests[pos + 1]
            except IndexError:
                pass

        # Remove all the selected tests that precede start if it's set.
        if starting_test:
            try:
                del self.selected[:self.selected.index(starting_test)]
            except ValueError:
                print(f"Cannot find starting test: {starting_test}")
                sys.exit(1)

        if randomize:
            if ns.random_seed is None:
                ns.random_seed = random.randrange(10000000)
            random.seed(ns.random_seed)
            random.shuffle(self.selected)

    def list_tests(self):
        for name in self.selected:
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
        ns = self.ns
        test_dir = ns.testdir
        support.verbose = False
        support.set_match_tests(ns.match_tests, ns.ignore_tests)

        skipped = []
        for test_name in self.selected:
            module_name = abs_module_name(test_name, test_dir)
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

    def get_rerun_match(self, rerun_list) -> MatchTestsDict:
        rerun_match_tests = {}
        for result in rerun_list:
            match_tests = result.get_rerun_match_tests()
            # ignore empty match list
            if match_tests:
                rerun_match_tests[result.test_name] = match_tests
        return rerun_match_tests

    def _rerun_failed_tests(self, need_rerun):
        # Configure the runner to re-run tests
        ns = self.ns
        ns.verbose = True
        ns.failfast = False
        ns.verbose3 = False
        ns.forever = False
        if ns.use_mp is None:
            ns.use_mp = 1

        # Get tests to re-run
        tests = [result.test_name for result in need_rerun]
        match_tests = self.get_rerun_match(need_rerun)
        self.set_tests(tests)

        # Clear previously failed tests
        self.rerun_bad.extend(self.bad)
        self.bad.clear()
        self.need_rerun.clear()

        # Re-run failed tests
        self.log(f"Re-running {len(tests)} failed tests in verbose mode in subprocesses")
        runtests = RunTests(tests, match_tests=match_tests, rerun=True)
        self.all_runtests.append(runtests)
        self._run_tests_mp(runtests)

    def rerun_failed_tests(self, need_rerun):
        if self.ns.python:
            # Temp patch for https://github.com/python/cpython/issues/94052
            self.log(
                "Re-running failed tests is not supported with --python "
                "host runner option."
            )
            return

        self.first_state = self.get_tests_state()

        print()
        self._rerun_failed_tests(need_rerun)

        if self.bad:
            print(count(len(self.bad), 'test'), "failed again:")
            printlist(self.bad)

        self.display_result()

    def display_result(self):
        pgo = self.ns.pgo
        quiet = self.ns.quiet
        print_slow = self.ns.print_slow

        # If running the test suite for PGO then no one cares about results.
        if pgo:
            return

        print()
        print("== Tests result: %s ==" % self.get_tests_state())

        if self.interrupted:
            print("Test suite interrupted by signal SIGINT.")

        omitted = set(self.selected) - self.get_executed()
        if omitted:
            print()
            print(count(len(omitted), "test"), "omitted:")
            printlist(omitted)

        if self.good and not quiet:
            print()
            if (not self.bad
                and not self.skipped
                and not self.interrupted
                and len(self.good) > 1):
                print("All", end=' ')
            print(count(len(self.good), "test"), "OK.")

        if print_slow:
            self.test_times.sort(reverse=True)
            print()
            print("10 slowest tests:")
            for test_time, test in self.test_times[:10]:
                print("- %s: %s" % (test, format_duration(test_time)))

        if self.bad:
            print()
            print(count(len(self.bad), "test"), "failed:")
            printlist(self.bad)

        if self.environment_changed:
            print()
            print("{} altered the execution environment:".format(
                     count(len(self.environment_changed), "test")))
            printlist(self.environment_changed)

        if self.skipped and not quiet:
            print()
            print(count(len(self.skipped), "test"), "skipped:")
            printlist(self.skipped)

        if self.resource_denied and not quiet:
            print()
            print(count(len(self.resource_denied), "test"), "skipped (resource denied):")
            printlist(self.resource_denied)

        if self.rerun:
            print()
            print("%s:" % count(len(self.rerun), "re-run test"))
            printlist(self.rerun)

        if self.run_no_tests:
            print()
            print(count(len(self.run_no_tests), "test"), "run no tests:")
            printlist(self.run_no_tests)

    def run_test(self, test_index, test_name, previous_test, save_modules):
        text = test_name
        if previous_test:
            text = '%s -- %s' % (text, previous_test)
        self.display_progress(test_index, text)

        if self.tracer:
            # If we're tracing code coverage, then we don't exit with status
            # if on a false return value from main.
            cmd = ('result = runtest(self.ns, test_name); '
                   'self.accumulate_result(result)')
            ns = dict(locals())
            self.tracer.runctx(cmd, globals=globals(), locals=ns)
            result = ns['result']
        else:
            result = runtest(self.ns, test_name)
            self.accumulate_result(result)

        # Unload the newly imported modules (best effort finalization)
        for module in sys.modules.keys():
            if module not in save_modules and module.startswith("test."):
                support.unload(module)

        return result

    def run_tests_sequentially(self, runtests):
        ns = self.ns
        coverage = ns.trace
        fail_fast = ns.failfast
        fail_env_changed = ns.fail_env_changed
        timeout = ns.timeout

        if coverage:
            import trace
            self.tracer = trace.Trace(trace=False, count=True)

        save_modules = sys.modules.keys()

        msg = "Run tests sequentially"
        if timeout:
            msg += " (timeout: %s)" % format_duration(timeout)
        self.log(msg)

        previous_test = None
        tests_iter = runtests.iter_tests()
        for test_index, test_name in enumerate(tests_iter, 1):
            start_time = time.perf_counter()

            result = self.run_test(test_index, test_name,
                                   previous_test, save_modules)

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

    def display_header(self):
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
        self.display_sanitizers()

    def display_sanitizers(self):
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

    def no_tests_run(self):
        return not any((self.good, self.bad, self.skipped, self.interrupted,
                        self.environment_changed))

    def get_tests_state(self):
        fail_env_changed = self.ns.fail_env_changed

        result = []
        if self.bad:
            result.append("FAILURE")
        elif fail_env_changed and self.environment_changed:
            result.append("ENV CHANGED")
        elif self.no_tests_run():
            result.append("NO TESTS RAN")

        if self.interrupted:
            result.append("INTERRUPTED")

        if not result:
            result.append("SUCCESS")

        result = ', '.join(result)
        if self.first_state:
            result = '%s then %s' % (self.first_state, result)
        return result

    def _run_tests_mp(self, runtests: RunTests) -> None:
        from test.libregrtest.runtest_mp import run_tests_multiprocess
        # If we're on windows and this is the parent runner (not a worker),
        # track the load average.
        if sys.platform == 'win32':
            from test.libregrtest.win_utils import WindowsLoadTracker

            try:
                self.win_load_tracker = WindowsLoadTracker()
            except PermissionError as error:
                # Standard accounts may not have access to the performance
                # counters.
                print(f'Failed to create WindowsLoadTracker: {error}')

        try:
            run_tests_multiprocess(self, runtests)
        finally:
            if self.win_load_tracker is not None:
                self.win_load_tracker.close()
                self.win_load_tracker = None

    def set_tests(self, tests):
        self.tests = tests
        if self.ns.forever:
            self.test_count_text = ''
            self.test_count_width = 3
        else:
            self.test_count_text = '/{}'.format(len(self.tests))
            self.test_count_width = len(self.test_count_text) - 1

    def run_tests(self):
        # For a partial run, we do not need to clutter the output.
        if (self.ns.header
            or not(self.ns.pgo or self.ns.quiet or self.ns.single
                   or self.tests or self.ns.args)):
            self.display_header()

        if self.ns.huntrleaks:
            warmup, repetitions, _ = self.ns.huntrleaks
            if warmup < 3:
                msg = ("WARNING: Running tests with --huntrleaks/-R and less than "
                        "3 warmup repetitions can give false positives!")
                print(msg, file=sys.stdout, flush=True)

        if self.ns.randomize:
            print("Using random seed", self.ns.random_seed)

        tests = self.selected
        self.set_tests(tests)
        runtests = RunTests(tests, forever=self.ns.forever)
        self.all_runtests.append(runtests)
        if self.ns.use_mp:
            self._run_tests_mp(runtests)
        else:
            self.run_tests_sequentially(runtests)

    def finalize(self):
        if self.next_single_filename:
            if self.next_single_test:
                with open(self.next_single_filename, 'w') as fp:
                    fp.write(self.next_single_test + '\n')
            else:
                os.unlink(self.next_single_filename)

        if self.tracer:
            r = self.tracer.results()
            r.write_results(show_missing=True, summary=True,
                            coverdir=self.ns.coverdir)

        if self.ns.runleaks:
            os.system("leaks %d" % os.getpid())

        self.save_xml_result()

    def display_summary(self):
        duration = time.perf_counter() - self.start_time
        first_runtests = self.all_runtests[0]
        # the second runtests (re-run failed tests) disables forever,
        # use the first runtests
        forever = first_runtests.forever
        filtered = bool(self.ns.match_tests) or bool(self.ns.ignore_tests)

        # Total duration
        print()
        print("Total duration: %s" % format_duration(duration))

        # Total tests
        total = self.total_stats
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
        all_tests = [self.good, self.bad, self.rerun,
                     self.skipped,
                     self.environment_changed, self.run_no_tests]
        run = sum(map(len, all_tests))
        text = f'run={run}'
        if not forever:
            ntest = len(first_runtests.tests)
            text = f"{text}/{ntest}"
        if filtered:
            text = f"{text} (filtered)"
        report = [text]
        for name, tests in (
            ('failed', self.bad),
            ('env_changed', self.environment_changed),
            ('skipped', self.skipped),
            ('resource_denied', self.resource_denied),
            ('rerun', self.rerun),
            ('run_no_tests', self.run_no_tests),
        ):
            if tests:
                report.append(f'{name}={len(tests)}')
        print(f"Total test files: {' '.join(report)}")

        # Result
        result = self.get_tests_state()
        print(f"Result: {result}")

    def save_xml_result(self):
        if not self.ns.xmlpath and not self.testsuite_xml:
            return

        import xml.etree.ElementTree as ET
        root = ET.Element("testsuites")

        # Manually count the totals for the overall summary
        totals = {'tests': 0, 'errors': 0, 'failures': 0}
        for suite in self.testsuite_xml:
            root.append(suite)
            for k in totals:
                try:
                    totals[k] += int(suite.get(k, 0))
                except ValueError:
                    pass

        for k, v in totals.items():
            root.set(k, str(v))

        xmlpath = os.path.join(os_helper.SAVEDCWD, self.ns.xmlpath)
        with open(xmlpath, 'wb') as f:
            for s in ET.tostringlist(root):
                f.write(s)

    def fix_umask(self):
        if support.is_emscripten:
            # Emscripten has default umask 0o777, which breaks some tests.
            # see https://github.com/emscripten-core/emscripten/issues/17269
            old_mask = os.umask(0)
            if old_mask == 0o777:
                os.umask(0o027)
            else:
                os.umask(old_mask)

    def set_temp_dir(self):
        if self.ns.tempdir:
            self.tmp_dir = self.ns.tempdir

        if not self.tmp_dir:
            # When tests are run from the Python build directory, it is best practice
            # to keep the test files in a subfolder.  This eases the cleanup of leftover
            # files using the "make distclean" command.
            if sysconfig.is_python_build():
                self.tmp_dir = sysconfig.get_config_var('abs_builddir')
                if self.tmp_dir is None:
                    self.tmp_dir = sysconfig.get_config_var('abs_srcdir')
                    if not self.tmp_dir:
                        # gh-74470: On Windows, only srcdir is available. Using
                        # abs_builddir mostly matters on UNIX when building
                        # Python out of the source tree, especially when the
                        # source tree is read only.
                        self.tmp_dir = sysconfig.get_config_var('srcdir')
                self.tmp_dir = os.path.join(self.tmp_dir, 'build')
            else:
                self.tmp_dir = tempfile.gettempdir()

        self.tmp_dir = os.path.abspath(self.tmp_dir)

    def is_worker(self):
        return (self.ns.worker_args is not None)

    def create_temp_dir(self):
        os.makedirs(self.tmp_dir, exist_ok=True)

        # Define a writable temp dir that will be used as cwd while running
        # the tests. The name of the dir includes the pid to allow parallel
        # testing (see the -j option).
        # Emscripten and WASI have stubbed getpid(), Emscripten has only
        # milisecond clock resolution. Use randint() instead.
        if sys.platform in {"emscripten", "wasi"}:
            nounce = random.randint(0, 1_000_000)
        else:
            nounce = os.getpid()

        if self.is_worker():
            test_cwd = 'test_python_worker_{}'.format(nounce)
        else:
            test_cwd = 'test_python_{}'.format(nounce)
        test_cwd += os_helper.FS_NONASCII
        test_cwd = os.path.join(self.tmp_dir, test_cwd)
        return test_cwd

    def cleanup(self):
        import glob

        path = os.path.join(glob.escape(self.tmp_dir), 'test_python_*')
        print("Cleanup %s directory" % self.tmp_dir)
        for name in glob.glob(path):
            if os.path.isdir(name):
                print("Remove directory: %s" % name)
                os_helper.rmtree(name)
            else:
                print("Remove file: %s" % name)
                os_helper.unlink(name)

    def main(self, tests=None, **kwargs):
        self.parse_args(kwargs)

        self.set_temp_dir()

        self.fix_umask()

        if self.ns.cleanup:
            self.cleanup()
            sys.exit(0)

        test_cwd = self.create_temp_dir()

        try:
            # Run the tests in a context manager that temporarily changes the CWD
            # to a temporary and writable directory. If it's not possible to
            # create or change the CWD, the original CWD will be used.
            # The original CWD is available from os_helper.SAVEDCWD.
            with os_helper.temp_cwd(test_cwd, quiet=True):
                # When using multiprocessing, worker processes will use test_cwd
                # as their parent temporary directory. So when the main process
                # exit, it removes also subdirectories of worker processes.
                self.ns.tempdir = test_cwd

                self._main(tests, kwargs)
        except SystemExit as exc:
            # bpo-38203: Python can hang at exit in Py_Finalize(), especially
            # on threading._shutdown() call: put a timeout
            if threading_helper.can_start_thread:
                faulthandler.dump_traceback_later(EXIT_TIMEOUT, exit=True)

            sys.exit(exc.code)

    def getloadavg(self):
        if self.win_load_tracker is not None:
            return self.win_load_tracker.getloadavg()

        if hasattr(os, 'getloadavg'):
            return os.getloadavg()[0]

        return None

    def get_exitcode(self):
        exitcode = 0
        if self.bad:
            exitcode = EXITCODE_BAD_TEST
        elif self.interrupted:
            exitcode = EXITCODE_INTERRUPTED
        elif self.ns.fail_env_changed and self.environment_changed:
            exitcode = EXITCODE_ENV_CHANGED
        elif self.no_tests_run():
            exitcode = EXITCODE_NO_TESTS_RAN
        elif self.rerun and self.ns.fail_rerun:
            exitcode = EXITCODE_RERUN_FAIL
        return exitcode

    def action_run_tests(self):
        self.run_tests()
        self.display_result()

        need_rerun = self.need_rerun
        if self.ns.rerun and need_rerun:
            self.rerun_failed_tests(need_rerun)

        self.display_summary()
        self.finalize()

    def _main(self, tests, kwargs):
        if self.is_worker():
            from test.libregrtest.runtest_mp import run_tests_worker
            run_tests_worker(self.ns.worker_args)
            return

        if self.ns.wait:
            input("Press any key to continue...")

        setup_tests(self.ns)
        self.find_tests(tests)

        exitcode = 0
        if self.ns.list_tests:
            self.list_tests()
        elif self.ns.list_cases:
            self.list_cases()
        else:
            self.action_run_tests()
            exitcode = self.get_exitcode()

        sys.exit(exitcode)


def main(tests=None, **kwargs):
    """Run the Python suite."""
    Regrtest().main(tests=tests, **kwargs)
