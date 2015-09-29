import faulthandler
import json
import os
import re
import sys
import tempfile
import sysconfig
import signal
import random
import platform
import traceback
import unittest
from test.libregrtest.runtest import (
    findtests, runtest, run_test_in_subprocess,
    STDTESTS, NOTTESTS,
    PASSED, FAILED, ENV_CHANGED, SKIPPED,
    RESOURCE_DENIED, INTERRUPTED, CHILD_ERROR)
from test.libregrtest.refleak import warm_caches
from test.libregrtest.cmdline import _parse_args
from test import support
try:
    import threading
except ImportError:
    threading = None


# Some times __path__ and __file__ are not absolute (e.g. while running from
# Lib/) and, if we change the CWD to run the tests in a temporary dir, some
# imports might fail.  This affects only the modules imported before os.chdir().
# These modules are searched first in sys.path[0] (so '' -- the CWD) and if
# they are found in the CWD their __file__ and __path__ will be relative (this
# happens before the chdir).  All the modules imported after the chdir, are
# not found in the CWD, and since the other paths in sys.path[1:] are absolute
# (site.py absolutize them), the __file__ and __path__ will be absolute too.
# Therefore it is necessary to absolutize manually the __file__ and __path__ of
# the packages to prevent later imports to fail when the CWD is different.
for module in sys.modules.values():
    if hasattr(module, '__path__'):
        module.__path__ = [os.path.abspath(path) for path in module.__path__]
    if hasattr(module, '__file__'):
        module.__file__ = os.path.abspath(module.__file__)


# MacOSX (a.k.a. Darwin) has a default stack size that is too small
# for deeply recursive regular expressions.  We see this as crashes in
# the Python test suite when running test_re.py and test_sre.py.  The
# fix is to set the stack limit to 2048.
# This approach may also be useful for other Unixy platforms that
# suffer from small default stack limits.
if sys.platform == 'darwin':
    try:
        import resource
    except ImportError:
        pass
    else:
        soft, hard = resource.getrlimit(resource.RLIMIT_STACK)
        newsoft = min(hard, max(soft, 1024*2048))
        resource.setrlimit(resource.RLIMIT_STACK, (newsoft, hard))


# When tests are run from the Python build directory, it is best practice
# to keep the test files in a subfolder.  This eases the cleanup of leftover
# files using the "make distclean" command.
if sysconfig.is_python_build():
    TEMPDIR = os.path.join(sysconfig.get_config_var('srcdir'), 'build')
else:
    TEMPDIR = tempfile.gettempdir()
TEMPDIR = os.path.abspath(TEMPDIR)


def main(tests=None, **kwargs):
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
    # Display the Python traceback on fatal errors (e.g. segfault)
    faulthandler.enable(all_threads=True)

    # Display the Python traceback on SIGALRM or SIGUSR1 signal
    signals = []
    if hasattr(signal, 'SIGALRM'):
        signals.append(signal.SIGALRM)
    if hasattr(signal, 'SIGUSR1'):
        signals.append(signal.SIGUSR1)
    for signum in signals:
        faulthandler.register(signum, chain=True)

    replace_stdout()

    support.record_original_stdout(sys.stdout)

    ns = _parse_args(sys.argv[1:], **kwargs)

    if ns.huntrleaks:
        # Avoid false positives due to various caches
        # filling slowly with random data:
        warm_caches()
    if ns.memlimit is not None:
        support.set_memlimit(ns.memlimit)
    if ns.threshold is not None:
        import gc
        gc.set_threshold(ns.threshold)
    if ns.nowindows:
        import msvcrt
        msvcrt.SetErrorMode(msvcrt.SEM_FAILCRITICALERRORS|
                            msvcrt.SEM_NOALIGNMENTFAULTEXCEPT|
                            msvcrt.SEM_NOGPFAULTERRORBOX|
                            msvcrt.SEM_NOOPENFILEERRORBOX)
        try:
            msvcrt.CrtSetReportMode
        except AttributeError:
            # release build
            pass
        else:
            for m in [msvcrt.CRT_WARN, msvcrt.CRT_ERROR, msvcrt.CRT_ASSERT]:
                msvcrt.CrtSetReportMode(m, msvcrt.CRTDBG_MODE_FILE)
                msvcrt.CrtSetReportFile(m, msvcrt.CRTDBG_FILE_STDERR)
    if ns.wait:
        input("Press any key to continue...")

    if ns.slaveargs is not None:
        args, kwargs = json.loads(ns.slaveargs)
        if kwargs.get('huntrleaks'):
            unittest.BaseTestSuite._cleanup = False
        try:
            result = runtest(*args, **kwargs)
        except KeyboardInterrupt:
            result = INTERRUPTED, ''
        except BaseException as e:
            traceback.print_exc()
            result = CHILD_ERROR, str(e)
        sys.stdout.flush()
        print()   # Force a newline (just in case)
        print(json.dumps(result))
        sys.exit(0)

    good = []
    bad = []
    skipped = []
    resource_denieds = []
    environment_changed = []
    interrupted = False

    if ns.findleaks:
        try:
            import gc
        except ImportError:
            print('No GC available, disabling findleaks.')
            ns.findleaks = False
        else:
            # Uncomment the line below to report garbage that is not
            # freeable by reference counting alone.  By default only
            # garbage that is not collectable by the GC is reported.
            #gc.set_debug(gc.DEBUG_SAVEALL)
            found_garbage = []

    if ns.huntrleaks:
        unittest.BaseTestSuite._cleanup = False

    if ns.single:
        filename = os.path.join(TEMPDIR, 'pynexttest')
        try:
            with open(filename, 'r') as fp:
                next_test = fp.read().strip()
                tests = [next_test]
        except OSError:
            pass

    if ns.fromfile:
        tests = []
        with open(os.path.join(support.SAVEDCWD, ns.fromfile)) as fp:
            count_pat = re.compile(r'\[\s*\d+/\s*\d+\]')
            for line in fp:
                line = count_pat.sub('', line)
                guts = line.split() # assuming no test has whitespace in its name
                if guts and not guts[0].startswith('#'):
                    tests.extend(guts)

    # Strip .py extensions.
    removepy(ns.args)
    removepy(tests)

    stdtests = STDTESTS[:]
    nottests = NOTTESTS.copy()
    if ns.exclude:
        for arg in ns.args:
            if arg in stdtests:
                stdtests.remove(arg)
            nottests.add(arg)
        ns.args = []

    # For a partial run, we do not need to clutter the output.
    if ns.verbose or ns.header or not (ns.quiet or ns.single or tests or ns.args):
        # Print basic platform information
        print("==", platform.python_implementation(), *sys.version.split())
        print("==  ", platform.platform(aliased=True),
                      "%s-endian" % sys.byteorder)
        print("==  ", "hash algorithm:", sys.hash_info.algorithm,
              "64bit" if sys.maxsize > 2**32 else "32bit")
        print("==  ", os.getcwd())
        print("Testing with flags:", sys.flags)

    # if testdir is set, then we are not running the python tests suite, so
    # don't add default tests to be executed or skipped (pass empty values)
    if ns.testdir:
        alltests = findtests(ns.testdir, list(), set())
    else:
        alltests = findtests(ns.testdir, stdtests, nottests)

    selected = tests or ns.args or alltests
    if ns.single:
        selected = selected[:1]
        try:
            next_single_test = alltests[alltests.index(selected[0])+1]
        except IndexError:
            next_single_test = None
    # Remove all the selected tests that precede start if it's set.
    if ns.start:
        try:
            del selected[:selected.index(ns.start)]
        except ValueError:
            print("Couldn't find starting test (%s), using all tests" % ns.start)
    if ns.randomize:
        if ns.random_seed is None:
            ns.random_seed = random.randrange(10000000)
        random.seed(ns.random_seed)
        print("Using random seed", ns.random_seed)
        random.shuffle(selected)
    if ns.trace:
        import trace, tempfile
        tracer = trace.Trace(ignoredirs=[sys.base_prefix, sys.base_exec_prefix,
                                         tempfile.gettempdir()],
                             trace=False, count=True)

    test_times = []
    support.verbose = ns.verbose      # Tell tests to be moderately quiet
    support.use_resources = ns.use_resources
    save_modules = sys.modules.keys()

    def accumulate_result(test, result):
        ok, test_time = result
        test_times.append((test_time, test))
        if ok == PASSED:
            good.append(test)
        elif ok == FAILED:
            bad.append(test)
        elif ok == ENV_CHANGED:
            environment_changed.append(test)
        elif ok == SKIPPED:
            skipped.append(test)
        elif ok == RESOURCE_DENIED:
            skipped.append(test)
            resource_denieds.append(test)

    if ns.forever:
        def test_forever(tests=list(selected)):
            while True:
                for test in tests:
                    yield test
                    if bad:
                        return
        tests = test_forever()
        test_count = ''
        test_count_width = 3
    else:
        tests = iter(selected)
        test_count = '/{}'.format(len(selected))
        test_count_width = len(test_count) - 1

    if ns.use_mp:
        try:
            from threading import Thread
        except ImportError:
            print("Multiprocess option requires thread support")
            sys.exit(2)
        from queue import Queue
        debug_output_pat = re.compile(r"\[\d+ refs, \d+ blocks\]$")
        output = Queue()
        pending = MultiprocessTests(tests)
        def work():
            # A worker thread.
            try:
                while True:
                    try:
                        test = next(pending)
                    except StopIteration:
                        output.put((None, None, None, None))
                        return
                    retcode, stdout, stderr = run_test_in_subprocess(test, ns)
                    # Strip last refcount output line if it exists, since it
                    # comes from the shutdown of the interpreter in the subcommand.
                    stderr = debug_output_pat.sub("", stderr)
                    stdout, _, result = stdout.strip().rpartition("\n")
                    if retcode != 0:
                        result = (CHILD_ERROR, "Exit code %s" % retcode)
                        output.put((test, stdout.rstrip(), stderr.rstrip(), result))
                        return
                    if not result:
                        output.put((None, None, None, None))
                        return
                    result = json.loads(result)
                    output.put((test, stdout.rstrip(), stderr.rstrip(), result))
            except BaseException:
                output.put((None, None, None, None))
                raise
        workers = [Thread(target=work) for i in range(ns.use_mp)]
        for worker in workers:
            worker.start()
        finished = 0
        test_index = 1
        try:
            while finished < ns.use_mp:
                test, stdout, stderr, result = output.get()
                if test is None:
                    finished += 1
                    continue
                accumulate_result(test, result)
                if not ns.quiet:
                    fmt = "[{1:{0}}{2}/{3}] {4}" if bad else "[{1:{0}}{2}] {4}"
                    print(fmt.format(
                        test_count_width, test_index, test_count,
                        len(bad), test))
                if stdout:
                    print(stdout)
                if stderr:
                    print(stderr, file=sys.stderr)
                sys.stdout.flush()
                sys.stderr.flush()
                if result[0] == INTERRUPTED:
                    raise KeyboardInterrupt
                if result[0] == CHILD_ERROR:
                    raise Exception("Child error on {}: {}".format(test, result[1]))
                test_index += 1
        except KeyboardInterrupt:
            interrupted = True
            pending.interrupted = True
        for worker in workers:
            worker.join()
    else:
        for test_index, test in enumerate(tests, 1):
            if not ns.quiet:
                fmt = "[{1:{0}}{2}/{3}] {4}" if bad else "[{1:{0}}{2}] {4}"
                print(fmt.format(
                    test_count_width, test_index, test_count, len(bad), test))
                sys.stdout.flush()
            if ns.trace:
                # If we're tracing code coverage, then we don't exit with status
                # if on a false return value from main.
                tracer.runctx('runtest(test, ns.verbose, ns.quiet, timeout=ns.timeout)',
                              globals=globals(), locals=vars())
            else:
                try:
                    result = runtest(test, ns.verbose, ns.quiet,
                                     ns.huntrleaks,
                                     output_on_failure=ns.verbose3,
                                     timeout=ns.timeout, failfast=ns.failfast,
                                     match_tests=ns.match_tests)
                    accumulate_result(test, result)
                except KeyboardInterrupt:
                    interrupted = True
                    break
            if ns.findleaks:
                gc.collect()
                if gc.garbage:
                    print("Warning: test created", len(gc.garbage), end=' ')
                    print("uncollectable object(s).")
                    # move the uncollectable objects somewhere so we don't see
                    # them again
                    found_garbage.extend(gc.garbage)
                    del gc.garbage[:]
            # Unload the newly imported modules (best effort finalization)
            for module in sys.modules.keys():
                if module not in save_modules and module.startswith("test."):
                    support.unload(module)

    if interrupted:
        # print a newline after ^C
        print()
        print("Test suite interrupted by signal SIGINT.")
        omitted = set(selected) - set(good) - set(bad) - set(skipped)
        print(count(len(omitted), "test"), "omitted:")
        printlist(omitted)
    if good and not ns.quiet:
        if not bad and not skipped and not interrupted and len(good) > 1:
            print("All", end=' ')
        print(count(len(good), "test"), "OK.")
    if ns.print_slow:
        test_times.sort(reverse=True)
        print("10 slowest tests:")
        for time, test in test_times[:10]:
            print("%s: %.1fs" % (test, time))
    if bad:
        print(count(len(bad), "test"), "failed:")
        printlist(bad)
    if environment_changed:
        print("{} altered the execution environment:".format(
                 count(len(environment_changed), "test")))
        printlist(environment_changed)
    if skipped and not ns.quiet:
        print(count(len(skipped), "test"), "skipped:")
        printlist(skipped)

    if ns.verbose2 and bad:
        print("Re-running failed tests in verbose mode")
        for test in bad[:]:
            print("Re-running test %r in verbose mode" % test)
            sys.stdout.flush()
            try:
                ns.verbose = True
                ok = runtest(test, True, ns.quiet, ns.huntrleaks,
                             timeout=ns.timeout)
            except KeyboardInterrupt:
                # print a newline separate from the ^C
                print()
                break
            else:
                if ok[0] in {PASSED, ENV_CHANGED, SKIPPED, RESOURCE_DENIED}:
                    bad.remove(test)
        else:
            if bad:
                print(count(len(bad), 'test'), "failed again:")
                printlist(bad)

    if ns.single:
        if next_single_test:
            with open(filename, 'w') as fp:
                fp.write(next_single_test + '\n')
        else:
            os.unlink(filename)

    if ns.trace:
        r = tracer.results()
        r.write_results(show_missing=True, summary=True, coverdir=ns.coverdir)

    if ns.runleaks:
        os.system("leaks %d" % os.getpid())

    sys.exit(len(bad) > 0 or interrupted)


# We do not use a generator so multiple threads can call next().
class MultiprocessTests(object):

    """A thread-safe iterator over tests for multiprocess mode."""

    def __init__(self, tests):
        self.interrupted = False
        self.lock = threading.Lock()
        self.tests = tests

    def __iter__(self):
        return self

    def __next__(self):
        with self.lock:
            if self.interrupted:
                raise StopIteration('tests interrupted')
            return next(self.tests)


def replace_stdout():
    """Set stdout encoder error handler to backslashreplace (as stderr error
    handler) to avoid UnicodeEncodeError when printing a traceback"""
    import atexit

    stdout = sys.stdout
    sys.stdout = open(stdout.fileno(), 'w',
        encoding=stdout.encoding,
        errors="backslashreplace",
        closefd=False,
        newline='\n')

    def restore_stdout():
        sys.stdout.close()
        sys.stdout = stdout
    atexit.register(restore_stdout)


def removepy(names):
    if not names:
        return
    for idx, name in enumerate(names):
        basename, ext = os.path.splitext(name)
        if ext == '.py':
            names[idx] = basename


def count(n, word):
    if n == 1:
        return "%d %s" % (n, word)
    else:
        return "%d %ss" % (n, word)


def printlist(x, width=70, indent=4):
    """Print the elements of iterable x to stdout.

    Optional arg width (default 70) is the maximum line length.
    Optional arg indent (default 4) is the number of blanks with which to
    begin each line.
    """

    from textwrap import fill
    blanks = ' ' * indent
    # Print the sorted list: 'x' may be a '--random' list or a set()
    print(fill(' '.join(str(elt) for elt in sorted(x)), width,
               initial_indent=blanks, subsequent_indent=blanks))


def main_in_temp_cwd():
    """Run main() in a temporary working directory."""
    if sysconfig.is_python_build():
        try:
            os.mkdir(TEMPDIR)
        except FileExistsError:
            pass

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
        main()
