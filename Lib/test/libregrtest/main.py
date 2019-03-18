import datetime
import faulthandler
import json
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
    findtests, runtest, get_abs_module,
    STDTESTS, NOTTESTS, PASSED, FAILED, ENV_CHANGED, SKIPPED, RESOURCE_DENIED,
    INTERRUPTED, CHILD_ERROR, TEST_DID_NOT_RUN,
    PROGRESS_MIN_TIME, format_test_result)
from test.libregrtest.setup import setup_tests
from test.libregrtest.utils import removepy, count, format_duration, printlist
from test import support
try:
    import gc
except ImportError:
    gc = None


# When tests are run from the Python build directory, it is best practice
# to keep the test files in a subfolder.  This eases the cleanup of leftover
# files using the "make distclean" command.
if sysconfig.is_python_build():
    TEMPDIR = sysconfig.get_config_var('abs_builddir')
    if TEMPDIR is None:
        # bpo-30284: On Windows, only srcdir is available. Using abs_builddir
        # mostly matters on UNIX when building Python out of the source tree,
        # especially when the source tree is read only.
        TEMPDIR = sysconfig.get_config_var('srcdir')
    TEMPDIR = os.path.join(TEMPDIR, 'build')
else:
    TEMPDIR = tempfile.gettempdir()
TEMPDIR = os.path.abspath(TEMPDIR)


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
    single, randomize, findleaks, use_resources, trace, coverdir,
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
        self.rerun = []
        self.run_no_tests = []
        self.first_result = None
        self.interrupted = False

        # used by --slow
        self.test_times = []

        # used by --coverage, trace.Trace instance
        self.tracer = None

        # used by --findleaks, store for gc.garbage
        self.found_garbage = []

        # used to display the progress bar "[ 3/100]"
        self.start_time = time.monotonic()
        self.test_count = ''
        self.test_count_width = 1

        # used by --single
        self.next_single_test = None
        self.next_single_filename = None

        # used by --junit-xml
        self.testsuite_xml = None

    def accumulate_result(self, test, result):
        ok, test_time, xml_data = result
        if ok not in (CHILD_ERROR, INTERRUPTED):
            self.test_times.append((test_time, test))
        if ok == PASSED:
            self.good.append(test)
        elif ok in (FAILED, CHILD_ERROR):
            self.bad.append(test)
        elif ok == ENV_CHANGED:
            self.environment_changed.append(test)
        elif ok == SKIPPED:
            self.skipped.append(test)
        elif ok == RESOURCE_DENIED:
            self.skipped.append(test)
            self.resource_denieds.append(test)
        elif ok == TEST_DID_NOT_RUN:
            self.run_no_tests.append(test)
        elif ok != INTERRUPTED:
            raise ValueError("invalid test result: %r" % ok)

        if xml_data:
            import xml.etree.ElementTree as ET
            for e in xml_data:
                try:
                    self.testsuite_xml.append(ET.fromstring(e))
                except ET.ParseError:
                    print(xml_data, file=sys.__stderr__)
                    raise

    def display_progress(self, test_index, test):
        if self.ns.quiet:
            return

        # "[ 51/405/1] test_tcl passed"
        line = f"{test_index:{self.test_count_width}}{self.test_count}"
        fails = len(self.bad) + len(self.environment_changed)
        if fails and not self.ns.pgo:
            line = f"{line}/{fails}"
        line = f"[{line}] {test}"

        # add the system load prefix: "load avg: 1.80 "
        if hasattr(os, 'getloadavg'):
            load_avg_1min = os.getloadavg()[0]
            line = f"load avg: {load_avg_1min:.2f} {line}"

        # add the timestamp prefix:  "0:01:05 "
        test_time = time.monotonic() - self.start_time
        test_time = datetime.timedelta(seconds=int(test_time))
        line = f"{test_time} {line}"
        print(line, flush=True)

    def parse_args(self, kwargs):
        ns = _parse_args(sys.argv[1:], **kwargs)

        if ns.timeout and not hasattr(faulthandler, 'dump_traceback_later'):
            print("Warning: The timeout option requires "
                  "faulthandler.dump_traceback_later", file=sys.stderr)
            ns.timeout = None

        if ns.threshold is not None and gc is None:
            print('No GC available, ignore --threshold.', file=sys.stderr)
            ns.threshold = None

        if ns.findleaks:
            if gc is not None:
                # Uncomment the line below to report garbage that is not
                # freeable by reference counting alone.  By default only
                # garbage that is not collectable by the GC is reported.
                pass
                #gc.set_debug(gc.DEBUG_SAVEALL)
            else:
                print('No GC available, disabling --findleaks',
                      file=sys.stderr)
                ns.findleaks = False

        if ns.xmlpath:
            support.junit_xml_list = self.testsuite_xml = []

        # Strip .py extensions.
        removepy(ns.args)

        return ns

    def find_tests(self, tests):
        self.tests = tests

        if self.ns.single:
            self.next_single_filename = os.path.join(TEMPDIR, 'pynexttest')
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
            with open(os.path.join(support.SAVEDCWD, self.ns.fromfile)) as fp:
                for line in fp:
                    line = line.split('#', 1)[0]
                    line = line.strip()
                    match = regex.search(line)
                    if match is not None:
                        self.tests.append(match.group())

        removepy(self.tests)

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
        support.set_match_tests(self.ns.match_tests)

        for test in self.selected:
            abstest = get_abs_module(self.ns, test)
            try:
                suite = unittest.defaultTestLoader.loadTestsFromName(abstest)
                self._list_cases(suite)
            except unittest.SkipTest:
                self.skipped.append(test)

        if self.skipped:
            print(file=sys.stderr)
            print(count(len(self.skipped), "test"), "skipped:", file=sys.stderr)
            printlist(self.skipped, file=sys.stderr)

    def rerun_failed_tests(self):
        self.ns.verbose = True
        self.ns.failfast = False
        self.ns.verbose3 = False

        self.first_result = self.get_tests_result()

        print()
        print("Re-running failed tests in verbose mode")
        self.rerun = self.bad[:]
        for test in self.rerun:
            print("Re-running test %r in verbose mode" % test, flush=True)
            try:
                self.ns.verbose = True
                ok = runtest(self.ns, test)
            except KeyboardInterrupt:
                self.interrupted = True
                # print a newline separate from the ^C
                print()
                break
            else:
                if ok[0] in {PASSED, ENV_CHANGED, SKIPPED, RESOURCE_DENIED}:
                    self.bad.remove(test)
        else:
            if self.bad:
                print(count(len(self.bad), 'test'), "failed again:")
                printlist(self.bad)

        self.display_result()

    def display_result(self):
        # If running the test suite for PGO then no one cares about results.
        if self.ns.pgo:
            return

        print()
        print("== Tests result: %s ==" % self.get_tests_result())

        if self.interrupted:
            print()
            # print a newline after ^C
            print("Test suite interrupted by signal SIGINT.")
            executed = set(self.good) | set(self.bad) | set(self.skipped)
            omitted = set(self.selected) - executed
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
            for time, test in self.test_times[:10]:
                print("- %s: %s" % (test, format_duration(time)))

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

        print("Run tests sequentially")

        previous_test = None
        for test_index, test in enumerate(self.tests, 1):
            start_time = time.monotonic()

            text = test
            if previous_test:
                text = '%s -- %s' % (text, previous_test)
            self.display_progress(test_index, text)

            if self.tracer:
                # If we're tracing code coverage, then we don't exit with status
                # if on a false return value from main.
                cmd = ('result = runtest(self.ns, test); '
                       'self.accumulate_result(test, result)')
                ns = dict(locals())
                self.tracer.runctx(cmd, globals=globals(), locals=ns)
                result = ns['result']
            else:
                try:
                    result = runtest(self.ns, test)
                except KeyboardInterrupt:
                    self.interrupted = True
                    self.accumulate_result(test, (INTERRUPTED, None, None))
                    break
                else:
                    self.accumulate_result(test, result)

            previous_test = format_test_result(test, result[0])
            test_time = time.monotonic() - start_time
            if test_time >= PROGRESS_MIN_TIME:
                previous_test = "%s in %s" % (previous_test, format_duration(test_time))
            elif result[0] == PASSED:
                # be quiet: say nothing if the test passed shortly
                previous_test = None

            if self.ns.findleaks:
                gc.collect()
                if gc.garbage:
                    print("Warning: test created", len(gc.garbage), end=' ')
                    print("uncollectable object(s).")
                    # move the uncollectable objects somewhere so we don't see
                    # them again
                    self.found_garbage.extend(gc.garbage)
                    del gc.garbage[:]

            # Unload the newly imported modules (best effort finalization)
            for module in sys.modules.keys():
                if module not in save_modules and module.startswith("test."):
                    support.unload(module)

        if previous_test:
            print(previous_test)

    def _test_forever(self, tests):
        while True:
            for test in tests:
                yield test
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
              % (locale.getpreferredencoding(False),
                 sys.getfilesystemencoding()))

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
            run_tests_multiprocess(self)
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

        xmlpath = os.path.join(support.SAVEDCWD, self.ns.xmlpath)
        with open(xmlpath, 'wb') as f:
            for s in ET.tostringlist(root):
                f.write(s)

    def main(self, tests=None, **kwargs):
        global TEMPDIR
        self.ns = self.parse_args(kwargs)

        if self.ns.tempdir:
            TEMPDIR = self.ns.tempdir
        elif self.ns.worker_args:
            ns_dict, _ = json.loads(self.ns.worker_args)
            TEMPDIR = ns_dict.get("tempdir") or TEMPDIR

        os.makedirs(TEMPDIR, exist_ok=True)

        # Define a writable temp dir that will be used as cwd while running
        # the tests. The name of the dir includes the pid to allow parallel
        # testing (see the -j option).
        test_cwd = 'test_python_{}'.format(os.getpid())
        test_cwd = os.path.join(TEMPDIR, test_cwd)

        # Run the tests in a context manager that temporarily changes the CWD to a
        # temporary and writable directory.  If it's not possible to create or
        # change the CWD, the original CWD will be used.  The original CWD is
        # available from support.SAVEDCWD.
        with support.temp_cwd(test_cwd, quiet=True):
            self._main(tests, kwargs)

    def _main(self, tests, kwargs):
        if self.ns.huntrleaks:
            warmup, repetitions, _ = self.ns.huntrleaks
            if warmup < 1 or repetitions < 1:
                msg = ("Invalid values for the --huntrleaks/-R parameters. The "
                       "number of warmups and repetitions must be at least 1 "
                       "each (1:1).")
                print(msg, file=sys.stderr, flush=True)
                sys.exit(2)

        if self.ns.worker_args is not None:
            from test.libregrtest.runtest_mp import run_tests_worker
            run_tests_worker(self.ns.worker_args)

        if self.ns.wait:
            input("Press any key to continue...")

        support.PGO = self.ns.pgo

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
