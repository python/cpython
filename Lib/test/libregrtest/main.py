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
    findtests, runtest, get_abs_module, is_failed,
    STDTESTS, NOTTESTS, PROGRESS_MIN_TIME,
    Passed, Failed, EnvChanged, Skipped, ResourceDenied, Interrupted,
    ChildError, DidNotRun)
from test.libregrtest.setup import setup_tests
from test.libregrtest.pgo import setup_pgo_tests
from test.libregrtest.utils import removepy, count, format_duration, printlist
from test import support
from test.support import os_helper
from test.support import threading_helper


# bpo-38203: Maximum delay in seconds to exit Python (call Py_Finalize()).
# Used to protect against threading._shutdown() hang.
# Must be smaller than buildbot "1200 seconds without output" limit.
EXIT_TIMEOUT = 120.0

# gh-90681: When rerunning tests, we might need to rerun the whole
# class or module suite if some its life-cycle hooks fail.
# Test level hooks are not affected.
_TEST_LIFECYCLE_HOOKS = frozenset((
    'setUpClass', 'tearDownClass',
    'setUpModule', 'tearDownModule',
))

EXITCODE_BAD_TEST = 2
EXITCODE_INTERRUPTED = 130
EXITCODE_ENV_CHANGED = 3
EXITCODE_NO_TESTS_RAN = 4


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

        # test results
        self.good = []
        self.bad = []
        self.skipped = []
        self.resource_denieds = []
        self.environment_changed = []
        self.run_no_tests = []
        self.need_rerun = []
        self.rerun = []
        self.first_result = None
        self.interrupted = False

        # used by --slow
        self.test_times = []

        # used by --coverage, trace.Trace instance
        self.tracer = None

        # used to display the progress bar "[ 3/100]"
        self.start_time = time.monotonic()
        self.test_count = ''
        self.test_count_width = 1

        # used by --single
        self.next_single_test = None
        self.next_single_filename = None

        # used by --junit-xml
        self.testsuite_xml = None

        # misc
        self.win_load_tracker = None
        self.tmp_dir = None
        self.worker_test_name = None

    def get_executed(self):
        return (set(self.good) | set(self.bad) | set(self.skipped)
                | set(self.resource_denieds) | set(self.environment_changed)
                | set(self.run_no_tests))

    def accumulate_result(self, result, rerun=False):
        test_name = result.name

        if not isinstance(result, (ChildError, Interrupted)) and not rerun:
            self.test_times.append((result.duration_sec, test_name))

        if isinstance(result, Passed):
            self.good.append(test_name)
        elif isinstance(result, ResourceDenied):
            self.skipped.append(test_name)
            self.resource_denieds.append(test_name)
        elif isinstance(result, Skipped):
            self.skipped.append(test_name)
        elif isinstance(result, EnvChanged):
            self.environment_changed.append(test_name)
        elif isinstance(result, Failed):
            if not rerun:
                self.bad.append(test_name)
                self.need_rerun.append(result)
        elif isinstance(result, DidNotRun):
            self.run_no_tests.append(test_name)
        elif isinstance(result, Interrupted):
            self.interrupted = True
        else:
            raise ValueError("invalid test result: %r" % result)

        if rerun and not isinstance(result, (Failed, Interrupted)):
            self.bad.remove(test_name)

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
        test_time = time.monotonic() - self.start_time

        mins, secs = divmod(int(test_time), 60)
        hours, mins = divmod(mins, 60)
        test_time = "%d:%02d:%02d" % (hours, mins, secs)

        line = f"{test_time} {line}"
        if empty:
            line = line[:-1]

        print(line, flush=True)

    def display_progress(self, test_index, text):
        if self.ns.quiet:
            return

        # "[ 51/405/1] test_tcl passed"
        line = f"{test_index:{self.test_count_width}}{self.test_count}"
        fails = len(self.bad) + len(self.environment_changed)
        if fails and not self.ns.pgo:
            line = f"{line}/{fails}"
        self.log(f"[{line}] {text}")

    def parse_args(self, kwargs):
        ns = _parse_args(sys.argv[1:], **kwargs)

        if ns.xmlpath:
            support.junit_xml_list = self.testsuite_xml = []

        worker_args = ns.worker_args
        if worker_args is not None:
            from test.libregrtest.runtest_mp import parse_worker_args
            ns, test_name = parse_worker_args(ns.worker_args)
            ns.worker_args = worker_args
            self.worker_test_name = test_name

        # Strip .py extensions.
        removepy(ns.args)

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
        self.tests = tests

        if self.ns.single:
            self.next_single_filename = os.path.join(self.tmp_dir, 'pynexttest')
            try:
                with open(self.next_single_filename, 'r') as fp:
                    next_test = fp.read().strip()
                    self.tests = [next_test]
            except OSError:
                pass

        if self.ns.fromfile:
            self.tests = []
            # regex to match 'test_builtin' in line:
            # '0:00:00 [  4/400] test_builtin -- test_dict took 1 sec'
            regex = re.compile(r'\btest_[a-zA-Z0-9_]+\b')
            with open(os.path.join(os_helper.SAVEDCWD, self.ns.fromfile)) as fp:
                for line in fp:
                    line = line.split('#', 1)[0]
                    line = line.strip()
                    match = regex.search(line)
                    if match is not None:
                        self.tests.append(match.group())

        removepy(self.tests)

        if self.ns.pgo:
            # add default PGO tests if no tests are specified
            setup_pgo_tests(self.ns)

        stdtests = STDTESTS[:]
        nottests = NOTTESTS.copy()
        if self.ns.exclude:
            for arg in self.ns.args:
                if arg in stdtests:
                    stdtests.remove(arg)
                nottests.add(arg)
            self.ns.args = []

        # if testdir is set, then we are not running the python tests suite, so
        # don't add default tests to be executed or skipped (pass empty values)
        if self.ns.testdir:
            alltests = findtests(self.ns.testdir, list(), set())
        else:
            alltests = findtests(self.ns.testdir, stdtests, nottests)

        if not self.ns.fromfile:
            self.selected = self.tests or self.ns.args or alltests
        else:
            self.selected = self.tests
        if self.ns.single:
            self.selected = self.selected[:1]
            try:
                pos = alltests.index(self.selected[0])
                self.next_single_test = alltests[pos + 1]
            except IndexError:
                pass

        # Remove all the selected tests that precede start if it's set.
        if self.ns.start:
            try:
                del self.selected[:self.selected.index(self.ns.start)]
            except ValueError:
                print("Couldn't find starting test (%s), using all tests"
                      % self.ns.start, file=sys.stderr)

        if self.ns.randomize:
            if self.ns.random_seed is None:
                self.ns.random_seed = random.randrange(10000000)
            random.seed(self.ns.random_seed)
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
        support.verbose = False
        support.set_match_tests(self.ns.match_tests, self.ns.ignore_tests)

        for test_name in self.selected:
            abstest = get_abs_module(self.ns, test_name)
            try:
                suite = unittest.defaultTestLoader.loadTestsFromName(abstest)
                self._list_cases(suite)
            except unittest.SkipTest:
                self.skipped.append(test_name)

        if self.skipped:
            print(file=sys.stderr)
            print(count(len(self.skipped), "test"), "skipped:", file=sys.stderr)
            printlist(self.skipped, file=sys.stderr)

    def rerun_failed_tests(self):
        self.log()

        if self.ns.python:
            # Temp patch for https://github.com/python/cpython/issues/94052
            self.log(
                "Re-running failed tests is not supported with --python "
                "host runner option."
            )
            return

        self.ns.verbose = True
        self.ns.failfast = False
        self.ns.verbose3 = False

        self.first_result = self.get_tests_result()

        self.log("Re-running failed tests in verbose mode")
        rerun_list = list(self.need_rerun)
        self.need_rerun.clear()
        for result in rerun_list:
            test_name = result.name
            self.rerun.append(test_name)

            errors = result.errors or []
            failures = result.failures or []
            error_names = [
                self.normalize_test_name(test_full_name, is_error=True)
                for (test_full_name, *_) in errors]
            failure_names = [
                self.normalize_test_name(test_full_name)
                for (test_full_name, *_) in failures]
            self.ns.verbose = True
            orig_match_tests = self.ns.match_tests
            if errors or failures:
                if self.ns.match_tests is None:
                    self.ns.match_tests = []
                self.ns.match_tests.extend(error_names)
                self.ns.match_tests.extend(failure_names)
                matching = "matching: " + ", ".join(self.ns.match_tests)
                self.log(f"Re-running {test_name} in verbose mode ({matching})")
            else:
                self.log(f"Re-running {test_name} in verbose mode")
            result = runtest(self.ns, test_name)
            self.ns.match_tests = orig_match_tests

            self.accumulate_result(result, rerun=True)

            if isinstance(result, Interrupted):
                break

        if self.bad:
            print(count(len(self.bad), 'test'), "failed again:")
            printlist(self.bad)

        self.display_result()

    def normalize_test_name(self, test_full_name, *, is_error=False):
        short_name = test_full_name.split(" ")[0]
        if is_error and short_name in _TEST_LIFECYCLE_HOOKS:
            # This means that we have a failure in a life-cycle hook,
            # we need to rerun the whole module or class suite.
            # Basically the error looks like this:
            #    ERROR: setUpClass (test.test_reg_ex.RegTest)
            # or
            #    ERROR: setUpModule (test.test_reg_ex)
            # So, we need to parse the class / module name.
            lpar = test_full_name.index('(')
            rpar = test_full_name.index(')')
            return test_full_name[lpar + 1: rpar].split('.')[-1]
        return short_name

    def display_result(self):
        # If running the test suite for PGO then no one cares about results.
        if self.ns.pgo:
            return

        print()
        print("== Tests result: %s ==" % self.get_tests_result())

        if self.interrupted:
            print("Test suite interrupted by signal SIGINT.")

        omitted = set(self.selected) - self.get_executed()
        if omitted:
            print()
            print(count(len(omitted), "test"), "omitted:")
            printlist(omitted)

        if self.good and not self.ns.quiet:
            print()
            if (not self.bad
                and not self.skipped
                and not self.interrupted
                and len(self.good) > 1):
                print("All", end=' ')
            print(count(len(self.good), "test"), "OK.")

        if self.ns.print_slow:
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

        if self.skipped and not self.ns.quiet:
            print()
            print(count(len(self.skipped), "test"), "skipped:")
            printlist(self.skipped)

        if self.rerun:
            print()
            print("%s:" % count(len(self.rerun), "re-run test"))
            printlist(self.rerun)

        if self.run_no_tests:
            print()
            print(count(len(self.run_no_tests), "test"), "run no tests:")
            printlist(self.run_no_tests)

    def run_tests_sequential(self):
        if self.ns.trace:
            import trace
            self.tracer = trace.Trace(trace=False, count=True)

        save_modules = sys.modules.keys()

        msg = "Run tests sequentially"
        if self.ns.timeout:
            msg += " (timeout: %s)" % format_duration(self.ns.timeout)
        self.log(msg)

        previous_test = None
        for test_index, test_name in enumerate(self.tests, 1):
            start_time = time.monotonic()

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

            if isinstance(result, Interrupted):
                break

            previous_test = str(result)
            test_time = time.monotonic() - start_time
            if test_time >= PROGRESS_MIN_TIME:
                previous_test = "%s in %s" % (previous_test, format_duration(test_time))
            elif isinstance(result, Passed):
                # be quiet: say nothing if the test passed shortly
                previous_test = None

            # Unload the newly imported modules (best effort finalization)
            for module in sys.modules.keys():
                if module not in save_modules and module.startswith("test."):
                    support.unload(module)

            if self.ns.failfast and is_failed(result, self.ns):
                break

        if previous_test:
            print(previous_test)

    def _test_forever(self, tests):
        while True:
            for test_name in tests:
                yield test_name
                if self.bad:
                    return
                if self.ns.fail_env_changed and self.environment_changed:
                    return

    def display_header(self):
        # Print basic platform information
        print("==", platform.python_implementation(), *sys.version.split())
        print("==", platform.platform(aliased=True),
                      "%s-endian" % sys.byteorder)
        print("== cwd:", os.getcwd())
        cpu_count = os.cpu_count()
        if cpu_count:
            print("== CPU count:", cpu_count)
        print("== encodings: locale=%s, FS=%s"
              % (locale.getencoding(), sys.getfilesystemencoding()))

    def get_tests_result(self):
        result = []
        if self.bad:
            result.append("FAILURE")
        elif self.ns.fail_env_changed and self.environment_changed:
            result.append("ENV CHANGED")
        elif not any((self.good, self.bad, self.skipped, self.interrupted,
            self.environment_changed)):
            result.append("NO TEST RUN")

        if self.interrupted:
            result.append("INTERRUPTED")

        if not result:
            result.append("SUCCESS")

        result = ', '.join(result)
        if self.first_result:
            result = '%s then %s' % (self.first_result, result)
        return result

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

        if self.ns.forever:
            self.tests = self._test_forever(list(self.selected))
            self.test_count = ''
            self.test_count_width = 3
        else:
            self.tests = iter(self.selected)
            self.test_count = '/{}'.format(len(self.selected))
            self.test_count_width = len(self.test_count) - 1

        if self.ns.use_mp:
            from test.libregrtest.runtest_mp import run_tests_multiprocess
            # If we're on windows and this is the parent runner (not a worker),
            # track the load average.
            if sys.platform == 'win32' and self.worker_test_name is None:
                from test.libregrtest.win_utils import WindowsLoadTracker

                try:
                    self.win_load_tracker = WindowsLoadTracker()
                except PermissionError as error:
                    # Standard accounts may not have access to the performance
                    # counters.
                    print(f'Failed to create WindowsLoadTracker: {error}')

            try:
                run_tests_multiprocess(self)
            finally:
                if self.win_load_tracker is not None:
                    self.win_load_tracker.close()
                    self.win_load_tracker = None
        else:
            self.run_tests_sequential()

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

        print()
        duration = time.monotonic() - self.start_time
        print("Total duration: %s" % format_duration(duration))
        print("Tests result: %s" % self.get_tests_result())

        if self.ns.runleaks:
            os.system("leaks %d" % os.getpid())

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
                    # bpo-30284: On Windows, only srcdir is available. Using
                    # abs_builddir mostly matters on UNIX when building Python
                    # out of the source tree, especially when the source tree
                    # is read only.
                    self.tmp_dir = sysconfig.get_config_var('srcdir')
                self.tmp_dir = os.path.join(self.tmp_dir, 'build')
            else:
                self.tmp_dir = tempfile.gettempdir()

        self.tmp_dir = os.path.abspath(self.tmp_dir)

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
        if self.worker_test_name is not None:
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

    def _main(self, tests, kwargs):
        if self.worker_test_name is not None:
            from test.libregrtest.runtest_mp import run_tests_worker
            run_tests_worker(self.ns, self.worker_test_name)

        if self.ns.wait:
            input("Press any key to continue...")

        support.PGO = self.ns.pgo
        support.PGO_EXTENDED = self.ns.pgo_extended

        setup_tests(self.ns)

        self.find_tests(tests)

        if self.ns.list_tests:
            self.list_tests()
            sys.exit(0)

        if self.ns.list_cases:
            self.list_cases()
            sys.exit(0)

        self.run_tests()
        self.display_result()

        if self.ns.verbose2 and self.bad:
            self.rerun_failed_tests()

        self.finalize()

        self.save_xml_result()

        if self.bad:
            sys.exit(2)
        if self.interrupted:
            sys.exit(130)
        if self.ns.fail_env_changed and self.environment_changed:
            sys.exit(3)
        sys.exit(0)


def main(tests=None, **kwargs):
    """Run the Python suite."""
    Regrtest().main(tests=tests, **kwargs)
