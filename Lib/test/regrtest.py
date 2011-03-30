#! /usr/bin/env python3

"""
Usage:

python -m test [options] [test_name1 [test_name2 ...]]
python path/to/Lib/test/regrtest.py [options] [test_name1 [test_name2 ...]]


If no arguments or options are provided, finds all files matching
the pattern "test_*" in the Lib/test subdirectory and runs
them in alphabetical order (but see -M and -u, below, for exceptions).

For more rigorous testing, it is useful to use the following
command line:

python -E -Wd -m test [options] [test_name1 ...]


Options:

-h/--help       -- print this text and exit

Verbosity

-v/--verbose    -- run tests in verbose mode with output to stdout
-w/--verbose2   -- re-run failed tests in verbose mode
-W/--verbose3   -- re-run failed tests in verbose mode immediately
-d/--debug      -- print traceback for failed tests
-q/--quiet      -- no output unless one or more tests fail
-S/--slow       -- print the slowest 10 tests
   --header     -- print header with interpreter info

Selecting tests

-r/--random     -- randomize test execution order (see below)
   --randseed   -- pass a random seed to reproduce a previous random run
-f/--fromfile   -- read names of tests to run from a file (see below)
-x/--exclude    -- arguments are tests to *exclude*
-s/--single     -- single step through a set of tests (see below)
-u/--use RES1,RES2,...
                -- specify which special resource intensive tests to run
-M/--memlimit LIMIT
                -- run very large memory-consuming tests
   --testdir DIR
                -- execute test files in the specified directory (instead
                   of the Python stdlib test suite)

Special runs

-l/--findleaks  -- if GC is available detect tests that leak memory
-L/--runleaks   -- run the leaks(1) command just before exit
-R/--huntrleaks RUNCOUNTS
                -- search for reference leaks (needs debug build, v. slow)
-j/--multiprocess PROCESSES
                -- run PROCESSES processes at once
-T/--coverage   -- turn on code coverage tracing using the trace module
-D/--coverdir DIRECTORY
                -- Directory where coverage files are put
-N/--nocoverdir -- Put coverage files alongside modules
-t/--threshold THRESHOLD
                -- call gc.set_threshold(THRESHOLD)
-n/--nowindows  -- suppress error message boxes on Windows
-F/--forever    -- run the specified tests in a loop, until an error happens


Additional Option Details:

-r randomizes test execution order. You can use --randseed=int to provide a
int seed value for the randomizer; this is useful for reproducing troublesome
test orders.

-s On the first invocation of regrtest using -s, the first test file found
or the first test file given on the command line is run, and the name of
the next test is recorded in a file named pynexttest.  If run from the
Python build directory, pynexttest is located in the 'build' subdirectory,
otherwise it is located in tempfile.gettempdir().  On subsequent runs,
the test in pynexttest is run, and the next test is written to pynexttest.
When the last test has been run, pynexttest is deleted.  In this way it
is possible to single step through the test files.  This is useful when
doing memory analysis on the Python interpreter, which process tends to
consume too many resources to run the full regression test non-stop.

-S is used to continue running tests after an aborted run.  It will
maintain the order a standard run (ie, this assumes -r is not used).
This is useful after the tests have prematurely stopped for some external
reason and you want to start running from where you left off rather
than starting from the beginning.

-f reads the names of tests from the file given as f's argument, one
or more test names per line.  Whitespace is ignored.  Blank lines and
lines beginning with '#' are ignored.  This is especially useful for
whittling down failures involving interactions among tests.

-L causes the leaks(1) command to be run just before exit if it exists.
leaks(1) is available on Mac OS X and presumably on some other
FreeBSD-derived systems.

-R runs each test several times and examines sys.gettotalrefcount() to
see if the test appears to be leaking references.  The argument should
be of the form stab:run:fname where 'stab' is the number of times the
test is run to let gettotalrefcount settle down, 'run' is the number
of times further it is run and 'fname' is the name of the file the
reports are written to.  These parameters all have defaults (5, 4 and
"reflog.txt" respectively), and the minimal invocation is '-R :'.

-M runs tests that require an exorbitant amount of memory. These tests
typically try to ascertain containers keep working when containing more than
2 billion objects, which only works on 64-bit systems. There are also some
tests that try to exhaust the address space of the process, which only makes
sense on 32-bit systems with at least 2Gb of memory. The passed-in memlimit,
which is a string in the form of '2.5Gb', determines howmuch memory the
tests will limit themselves to (but they may go slightly over.) The number
shouldn't be more memory than the machine has (including swap memory). You
should also keep in mind that swap memory is generally much, much slower
than RAM, and setting memlimit to all available RAM or higher will heavily
tax the machine. On the other hand, it is no use running these tests with a
limit of less than 2.5Gb, and many require more than 20Gb. Tests that expect
to use more than memlimit memory will be skipped. The big-memory tests
generally run very, very long.

-u is used to specify which special resource intensive tests to run,
such as those requiring large file support or network connectivity.
The argument is a comma-separated list of words indicating the
resources to test.  Currently only the following are defined:

    all -       Enable all special resources.

    audio -     Tests that use the audio device.  (There are known
                cases of broken audio drivers that can crash Python or
                even the Linux kernel.)

    curses -    Tests that use curses and will modify the terminal's
                state and output modes.

    largefile - It is okay to run some test that may create huge
                files.  These tests can take a long time and may
                consume >2GB of disk space temporarily.

    network -   It is okay to run tests that use external network
                resource, e.g. testing SSL support for sockets.

    decimal -   Test the decimal module against a large suite that
                verifies compliance with standards.

    cpu -       Used for certain CPU-heavy tests.

    subprocess  Run all tests for the subprocess module.

    urlfetch -  It is okay to download files required on testing.

    gui -       Run tests that require a running GUI.

To enable all resources except one, use '-uall,-<resource>'.  For
example, to run all the tests except for the gui tests, give the
option '-uall,-gui'.
"""

import builtins
import faulthandler
import getopt
import json
import os
import random
import re
import io
import sys
import time
import traceback
import warnings
import unittest
from inspect import isabstract
import tempfile
import platform
import sysconfig
import logging


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

# Test result constants.
PASSED = 1
FAILED = 0
ENV_CHANGED = -1
SKIPPED = -2
RESOURCE_DENIED = -3
INTERRUPTED = -4

from test import support

RESOURCE_NAMES = ('audio', 'curses', 'largefile', 'network',
                  'decimal', 'cpu', 'subprocess', 'urlfetch', 'gui')

TEMPDIR = os.path.abspath(tempfile.gettempdir())

def usage(msg):
    print(msg, file=sys.stderr)
    print("Use --help for usage", file=sys.stderr)
    sys.exit(2)


def main(tests=None, testdir=None, verbose=0, quiet=False,
         exclude=False, single=False, randomize=False, fromfile=None,
         findleaks=False, use_resources=None, trace=False, coverdir='coverage',
         runleaks=False, huntrleaks=False, verbose2=False, print_slow=False,
         random_seed=None, use_mp=None, verbose3=False, forever=False,
         header=False):
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

    replace_stdout()

    support.record_original_stdout(sys.stdout)
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hvqxsSrf:lu:t:TD:NLR:FwWM:nj:',
            ['help', 'verbose', 'verbose2', 'verbose3', 'quiet',
             'exclude', 'single', 'slow', 'random', 'fromfile', 'findleaks',
             'use=', 'threshold=', 'trace', 'coverdir=', 'nocoverdir',
             'runleaks', 'huntrleaks=', 'memlimit=', 'randseed=',
             'multiprocess=', 'coverage', 'slaveargs=', 'forever', 'debug',
             'start=', 'nowindows', 'header', 'testdir='])
    except getopt.error as msg:
        usage(msg)

    # Defaults
    if random_seed is None:
        random_seed = random.randrange(10000000)
    if use_resources is None:
        use_resources = []
    debug = False
    start = None
    for o, a in opts:
        if o in ('-h', '--help'):
            print(__doc__)
            return
        elif o in ('-v', '--verbose'):
            verbose += 1
        elif o in ('-w', '--verbose2'):
            verbose2 = True
        elif o in ('-d', '--debug'):
            debug = True
        elif o in ('-W', '--verbose3'):
            verbose3 = True
        elif o in ('-q', '--quiet'):
            quiet = True;
            verbose = 0
        elif o in ('-x', '--exclude'):
            exclude = True
        elif o in ('-S', '--start'):
            start = a
        elif o in ('-s', '--single'):
            single = True
        elif o in ('-S', '--slow'):
            print_slow = True
        elif o in ('-r', '--randomize'):
            randomize = True
        elif o == '--randseed':
            random_seed = int(a)
        elif o in ('-f', '--fromfile'):
            fromfile = a
        elif o in ('-l', '--findleaks'):
            findleaks = True
        elif o in ('-L', '--runleaks'):
            runleaks = True
        elif o in ('-t', '--threshold'):
            import gc
            gc.set_threshold(int(a))
        elif o in ('-T', '--coverage'):
            trace = True
        elif o in ('-D', '--coverdir'):
            # CWD is replaced with a temporary dir before calling main(), so we
            # need  join it with the saved CWD so it goes where the user expects.
            coverdir = os.path.join(support.SAVEDCWD, a)
        elif o in ('-N', '--nocoverdir'):
            coverdir = None
        elif o in ('-R', '--huntrleaks'):
            huntrleaks = a.split(':')
            if len(huntrleaks) not in (2, 3):
                print(a, huntrleaks)
                usage('-R takes 2 or 3 colon-separated arguments')
            if not huntrleaks[0]:
                huntrleaks[0] = 5
            else:
                huntrleaks[0] = int(huntrleaks[0])
            if not huntrleaks[1]:
                huntrleaks[1] = 4
            else:
                huntrleaks[1] = int(huntrleaks[1])
            if len(huntrleaks) == 2 or not huntrleaks[2]:
                huntrleaks[2:] = ["reflog.txt"]
            # Avoid false positives due to the character cache in
            # stringobject.c filling slowly with random data
            warm_char_cache()
        elif o in ('-M', '--memlimit'):
            support.set_memlimit(a)
        elif o in ('-u', '--use'):
            u = [x.lower() for x in a.split(',')]
            for r in u:
                if r == 'all':
                    use_resources[:] = RESOURCE_NAMES
                    continue
                remove = False
                if r[0] == '-':
                    remove = True
                    r = r[1:]
                if r not in RESOURCE_NAMES:
                    usage('Invalid -u/--use option: ' + a)
                if remove:
                    if r in use_resources:
                        use_resources.remove(r)
                elif r not in use_resources:
                    use_resources.append(r)
        elif o in ('-n', '--nowindows'):
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
        elif o in ('-F', '--forever'):
            forever = True
        elif o in ('-j', '--multiprocess'):
            use_mp = int(a)
            if use_mp <= 0:
                try:
                    import multiprocessing
                    # Use all cores + extras for tests that like to sleep
                    use_mp = 2 + multiprocessing.cpu_count()
                except (ImportError, NotImplementedError):
                    use_mp = 3
        elif o == '--header':
            header = True
        elif o == '--slaveargs':
            args, kwargs = json.loads(a)
            try:
                result = runtest(*args, **kwargs)
            except BaseException as e:
                result = INTERRUPTED, e.__class__.__name__
            sys.stdout.flush()
            print()   # Force a newline (just in case)
            print(json.dumps(result))
            sys.exit(0)
        elif o == '--testdir':
            # CWD is replaced with a temporary dir before calling main(), so we
            # join it with the saved CWD so it ends up where the user expects.
            testdir = os.path.join(support.SAVEDCWD, a)
        else:
            print(("No handler for option {}.  Please report this as a bug "
                   "at http://bugs.python.org.").format(o), file=sys.stderr)
            sys.exit(1)
    if single and fromfile:
        usage("-s and -f don't go together!")
    if use_mp and trace:
        usage("-T and -j don't go together!")
    if use_mp and findleaks:
        usage("-l and -j don't go together!")

    good = []
    bad = []
    skipped = []
    resource_denieds = []
    environment_changed = []
    interrupted = False

    if findleaks:
        try:
            import gc
        except ImportError:
            print('No GC available, disabling findleaks.')
            findleaks = False
        else:
            # Uncomment the line below to report garbage that is not
            # freeable by reference counting alone.  By default only
            # garbage that is not collectable by the GC is reported.
            #gc.set_debug(gc.DEBUG_SAVEALL)
            found_garbage = []

    if single:
        filename = os.path.join(TEMPDIR, 'pynexttest')
        try:
            fp = open(filename, 'r')
            next_test = fp.read().strip()
            tests = [next_test]
            fp.close()
        except IOError:
            pass

    if fromfile:
        tests = []
        fp = open(os.path.join(support.SAVEDCWD, fromfile))
        count_pat = re.compile(r'\[\s*\d+/\s*\d+\]')
        for line in fp:
            line = count_pat.sub('', line)
            guts = line.split() # assuming no test has whitespace in its name
            if guts and not guts[0].startswith('#'):
                tests.extend(guts)
        fp.close()

    # Strip .py extensions.
    removepy(args)
    removepy(tests)

    stdtests = STDTESTS[:]
    nottests = NOTTESTS.copy()
    if exclude:
        for arg in args:
            if arg in stdtests:
                stdtests.remove(arg)
            nottests.add(arg)
        args = []

    # For a partial run, we do not need to clutter the output.
    if verbose or header or not (quiet or single or tests or args):
        # Print basic platform information
        print("==", platform.python_implementation(), *sys.version.split())
        print("==  ", platform.platform(aliased=True),
                      "%s-endian" % sys.byteorder)
        print("==  ", os.getcwd())
        print("Testing with flags:", sys.flags)

    # if testdir is set, then we are not running the python tests suite, so
    # don't add default tests to be executed or skipped (pass empty values)
    if testdir:
        alltests = findtests(testdir, list(), set())
    else:
        alltests = findtests(testdir, stdtests, nottests)

    selected = tests or args or alltests
    if single:
        selected = selected[:1]
        try:
            next_single_test = alltests[alltests.index(selected[0])+1]
        except IndexError:
            next_single_test = None
    # Remove all the tests that precede start if it's set.
    if start:
        try:
            del tests[:tests.index(start)]
        except ValueError:
            print("Couldn't find starting test (%s), using all tests" % start)
    if randomize:
        random.seed(random_seed)
        print("Using random seed", random_seed)
        random.shuffle(selected)
    if trace:
        import trace, tempfile
        tracer = trace.Trace(ignoredirs=[sys.prefix, sys.exec_prefix,
                                         tempfile.gettempdir()],
                             trace=False, count=True)

    test_times = []
    support.verbose = verbose      # Tell tests to be moderately quiet
    support.use_resources = use_resources
    save_modules = sys.modules.keys()

    def accumulate_result(test, result):
        ok, test_time = result
        test_times.append((test_time, test))
        if ok == PASSED:
            good.append(test)
        elif ok == FAILED:
            bad.append(test)
        elif ok == ENV_CHANGED:
            bad.append(test)
            environment_changed.append(test)
        elif ok == SKIPPED:
            skipped.append(test)
        elif ok == RESOURCE_DENIED:
            skipped.append(test)
            resource_denieds.append(test)

    if forever:
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

    if use_mp:
        try:
            from threading import Thread
        except ImportError:
            print("Multiprocess option requires thread support")
            sys.exit(2)
        from queue import Queue
        from subprocess import Popen, PIPE
        debug_output_pat = re.compile(r"\[\d+ refs\]$")
        output = Queue()
        def tests_and_args():
            for test in tests:
                args_tuple = (
                    (test, verbose, quiet),
                    dict(huntrleaks=huntrleaks, use_resources=use_resources,
                         debug=debug, rerun_failed=verbose3)
                )
                yield (test, args_tuple)
        pending = tests_and_args()
        opt_args = support.args_from_interpreter_flags()
        base_cmd = [sys.executable] + opt_args + ['-m', 'test.regrtest']
        def work():
            # A worker thread.
            try:
                while True:
                    try:
                        test, args_tuple = next(pending)
                    except StopIteration:
                        output.put((None, None, None, None))
                        return
                    # -E is needed by some tests, e.g. test_import
                    popen = Popen(base_cmd + ['--slaveargs', json.dumps(args_tuple)],
                                   stdout=PIPE, stderr=PIPE,
                                   universal_newlines=True,
                                   close_fds=(os.name != 'nt'))
                    stdout, stderr = popen.communicate()
                    # Strip last refcount output line if it exists, since it
                    # comes from the shutdown of the interpreter in the subcommand.
                    stderr = debug_output_pat.sub("", stderr)
                    stdout, _, result = stdout.strip().rpartition("\n")
                    if not result:
                        output.put((None, None, None, None))
                        return
                    result = json.loads(result)
                    output.put((test, stdout.rstrip(), stderr.rstrip(), result))
            except BaseException:
                output.put((None, None, None, None))
                raise
        workers = [Thread(target=work) for i in range(use_mp)]
        for worker in workers:
            worker.start()
        finished = 0
        test_index = 1
        try:
            while finished < use_mp:
                test, stdout, stderr, result = output.get()
                if test is None:
                    finished += 1
                    continue
                if not quiet:
                    print("[{1:{0}}{2}] {3}".format(
                        test_count_width, test_index, test_count, test))
                if stdout:
                    print(stdout)
                if stderr:
                    print(stderr, file=sys.stderr)
                if result[0] == INTERRUPTED:
                    assert result[1] == 'KeyboardInterrupt'
                    raise KeyboardInterrupt   # What else?
                accumulate_result(test, result)
                test_index += 1
        except KeyboardInterrupt:
            interrupted = True
            pending.close()
        for worker in workers:
            worker.join()
    else:
        for test_index, test in enumerate(tests, 1):
            if not quiet:
                print("[{1:{0}}{2}] {3}".format(
                    test_count_width, test_index, test_count, test))
                sys.stdout.flush()
            if trace:
                # If we're tracing code coverage, then we don't exit with status
                # if on a false return value from main.
                tracer.runctx('runtest(test, verbose, quiet)',
                              globals=globals(), locals=vars())
            else:
                try:
                    result = runtest(test, verbose, quiet, huntrleaks, debug,
                                     rerun_failed=verbose3)
                    accumulate_result(test, result)
                except KeyboardInterrupt:
                    interrupted = True
                    break
                except:
                    raise
            if findleaks:
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
    if good and not quiet:
        if not bad and not skipped and not interrupted and len(good) > 1:
            print("All", end=' ')
        print(count(len(good), "test"), "OK.")
    if print_slow:
        test_times.sort(reverse=True)
        print("10 slowest tests:")
        for time, test in test_times[:10]:
            print("%s: %.1fs" % (test, time))
    if bad:
        bad = sorted(set(bad) - set(environment_changed))
        if bad:
            print(count(len(bad), "test"), "failed:")
            printlist(bad)
        if environment_changed:
            print("{} altered the execution environment:".format(
                     count(len(environment_changed), "test")))
            printlist(environment_changed)
    if skipped and not quiet:
        print(count(len(skipped), "test"), "skipped:")
        printlist(skipped)

        e = _ExpectedSkips()
        plat = sys.platform
        if e.isvalid():
            surprise = set(skipped) - e.getexpected() - set(resource_denieds)
            if surprise:
                print(count(len(surprise), "skip"), \
                      "unexpected on", plat + ":")
                printlist(surprise)
            else:
                print("Those skips are all expected on", plat + ".")
        else:
            print("Ask someone to teach regrtest.py about which tests are")
            print("expected to get skipped on", plat + ".")

    if verbose2 and bad:
        print("Re-running failed tests in verbose mode")
        for test in bad:
            print("Re-running test %r in verbose mode" % test)
            sys.stdout.flush()
            try:
                verbose = True
                ok = runtest(test, True, quiet, huntrleaks, debug)
            except KeyboardInterrupt:
                # print a newline separate from the ^C
                print()
                break
            except:
                raise

    if single:
        if next_single_test:
            with open(filename, 'w') as fp:
                fp.write(next_single_test + '\n')
        else:
            os.unlink(filename)

    if trace:
        r = tracer.results()
        r.write_results(show_missing=True, summary=True, coverdir=coverdir)

    if runleaks:
        os.system("leaks %d" % os.getpid())

    sys.exit(len(bad) > 0 or interrupted)


# small set of tests to determine if we have a basically functioning interpreter
# (i.e. if any of these fail, then anything else is likely to follow)
STDTESTS = [
    'test_grammar',
    'test_opcodes',
    'test_dict',
    'test_builtin',
    'test_exceptions',
    'test_types',
    'test_unittest',
    'test_doctest',
    'test_doctest2',
]

# set of tests that we don't want to be executed when using regrtest
NOTTESTS = set()

def findtests(testdir=None, stdtests=STDTESTS, nottests=NOTTESTS):
    """Return a list of all applicable test modules."""
    testdir = findtestdir(testdir)
    names = os.listdir(testdir)
    tests = []
    others = set(stdtests) | nottests
    for name in names:
        mod, ext = os.path.splitext(name)
        if mod[:5] == "test_" and ext in (".py", "") and mod not in others:
            tests.append(mod)
    return stdtests + sorted(tests)

def replace_stdout():
    """Set stdout encoder error handler to backslashreplace (as stderr error
    handler) to avoid UnicodeEncodeError when printing a traceback"""
    if os.name == "nt":
        # Replace sys.stdout breaks the stdout newlines on Windows: issue #8533
        return

    import atexit

    stdout = sys.stdout
    sys.stdout = open(stdout.fileno(), 'w',
        encoding=stdout.encoding,
        errors="backslashreplace",
        closefd=False)

    def restore_stdout():
        sys.stdout.close()
        sys.stdout = stdout
    atexit.register(restore_stdout)

def runtest(test, verbose, quiet,
            huntrleaks=False, debug=False, use_resources=None,
            rerun_failed=False):
    """Run a single test.

    test -- the name of the test
    verbose -- if true, print more messages
    quiet -- if true, don't print 'skipped' messages (probably redundant)
    test_times -- a list of (time, test_name) pairs
    huntrleaks -- run multiple times to test for leaks; requires a debug
                  build; a triple corresponding to -R's three arguments
    rerun_failed -- if true, re-run in verbose mode when failed

    Returns one of the test result constants:
        INTERRUPTED      KeyboardInterrupt when run under -j
        RESOURCE_DENIED  test skipped because resource denied
        SKIPPED          test skipped for some other reason
        ENV_CHANGED      test failed because it changed the execution environment
        FAILED           test failed
        PASSED           test passed
    """

    support.verbose = verbose  # Tell tests to be moderately quiet
    if use_resources is not None:
        support.use_resources = use_resources
    try:
        result = runtest_inner(test, verbose, quiet, huntrleaks, debug)
        if result[0] == FAILED and rerun_failed:
            cleanup_test_droppings(test, verbose)
            sys.stdout.flush()
            sys.stderr.flush()
            print("Re-running test {} in verbose mode".format(test))
            runtest(test, True, quiet, huntrleaks, debug)
        return result
    finally:
        cleanup_test_droppings(test, verbose)

# Unit tests are supposed to leave the execution environment unchanged
# once they complete.  But sometimes tests have bugs, especially when
# tests fail, and the changes to environment go on to mess up other
# tests.  This can cause issues with buildbot stability, since tests
# are run in random order and so problems may appear to come and go.
# There are a few things we can save and restore to mitigate this, and
# the following context manager handles this task.

class saved_test_environment:
    """Save bits of the test environment and restore them at block exit.

        with saved_test_environment(testname, verbose, quiet):
            #stuff

    Unless quiet is True, a warning is printed to stderr if any of
    the saved items was changed by the test.  The attribute 'changed'
    is initially False, but is set to True if a change is detected.

    If verbose is more than 1, the before and after state of changed
    items is also printed.
    """

    changed = False

    def __init__(self, testname, verbose=0, quiet=False):
        self.testname = testname
        self.verbose = verbose
        self.quiet = quiet

    # To add things to save and restore, add a name XXX to the resources list
    # and add corresponding get_XXX/restore_XXX functions.  get_XXX should
    # return the value to be saved and compared against a second call to the
    # get function when test execution completes.  restore_XXX should accept
    # the saved value and restore the resource using it.  It will be called if
    # and only if a change in the value is detected.
    #
    # Note: XXX will have any '.' replaced with '_' characters when determining
    # the corresponding method names.

    resources = ('sys.argv', 'cwd', 'sys.stdin', 'sys.stdout', 'sys.stderr',
                 'os.environ', 'sys.path', 'sys.path_hooks', '__import__',
                 'warnings.filters', 'asyncore.socket_map',
                 'logging._handlers', 'logging._handlerList', 'sys.gettrace',
                 'sys.warnoptions')

    def get_sys_argv(self):
        return id(sys.argv), sys.argv, sys.argv[:]
    def restore_sys_argv(self, saved_argv):
        sys.argv = saved_argv[1]
        sys.argv[:] = saved_argv[2]

    def get_cwd(self):
        return os.getcwd()
    def restore_cwd(self, saved_cwd):
        os.chdir(saved_cwd)

    def get_sys_stdout(self):
        return sys.stdout
    def restore_sys_stdout(self, saved_stdout):
        sys.stdout = saved_stdout

    def get_sys_stderr(self):
        return sys.stderr
    def restore_sys_stderr(self, saved_stderr):
        sys.stderr = saved_stderr

    def get_sys_stdin(self):
        return sys.stdin
    def restore_sys_stdin(self, saved_stdin):
        sys.stdin = saved_stdin

    def get_os_environ(self):
        return id(os.environ), os.environ, dict(os.environ)
    def restore_os_environ(self, saved_environ):
        os.environ = saved_environ[1]
        os.environ.clear()
        os.environ.update(saved_environ[2])

    def get_sys_path(self):
        return id(sys.path), sys.path, sys.path[:]
    def restore_sys_path(self, saved_path):
        sys.path = saved_path[1]
        sys.path[:] = saved_path[2]

    def get_sys_path_hooks(self):
        return id(sys.path_hooks), sys.path_hooks, sys.path_hooks[:]
    def restore_sys_path_hooks(self, saved_hooks):
        sys.path_hooks = saved_hooks[1]
        sys.path_hooks[:] = saved_hooks[2]

    def get_sys_gettrace(self):
        return sys.gettrace()
    def restore_sys_gettrace(self, trace_fxn):
        sys.settrace(trace_fxn)

    def get___import__(self):
        return builtins.__import__
    def restore___import__(self, import_):
        builtins.__import__ = import_

    def get_warnings_filters(self):
        return id(warnings.filters), warnings.filters, warnings.filters[:]
    def restore_warnings_filters(self, saved_filters):
        warnings.filters = saved_filters[1]
        warnings.filters[:] = saved_filters[2]

    def get_asyncore_socket_map(self):
        asyncore = sys.modules.get('asyncore')
        # XXX Making a copy keeps objects alive until __exit__ gets called.
        return asyncore and asyncore.socket_map.copy() or {}
    def restore_asyncore_socket_map(self, saved_map):
        asyncore = sys.modules.get('asyncore')
        if asyncore is not None:
            asyncore.close_all(ignore_all=True)
            asyncore.socket_map.update(saved_map)

    def get_logging__handlers(self):
        # _handlers is a WeakValueDictionary
        return id(logging._handlers), logging._handlers, logging._handlers.copy()
    def restore_logging__handlers(self, saved_handlers):
        # Can't easily revert the logging state
        pass

    def get_logging__handlerList(self):
        # _handlerList is a list of weakrefs to handlers
        return id(logging._handlerList), logging._handlerList, logging._handlerList[:]
    def restore_logging__handlerList(self, saved_handlerList):
        # Can't easily revert the logging state
        pass

    def get_sys_warnoptions(self):
        return id(sys.warnoptions), sys.warnoptions, sys.warnoptions[:]
    def restore_sys_warnoptions(self, saved_options):
        sys.warnoptions = saved_options[1]
        sys.warnoptions[:] = saved_options[2]

    def resource_info(self):
        for name in self.resources:
            method_suffix = name.replace('.', '_')
            get_name = 'get_' + method_suffix
            restore_name = 'restore_' + method_suffix
            yield name, getattr(self, get_name), getattr(self, restore_name)

    def __enter__(self):
        self.saved_values = dict((name, get()) for name, get, restore
                                                   in self.resource_info())
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        saved_values = self.saved_values
        del self.saved_values
        for name, get, restore in self.resource_info():
            current = get()
            original = saved_values.pop(name)
            # Check for changes to the resource's value
            if current != original:
                self.changed = True
                restore(original)
                if not self.quiet:
                    print("Warning -- {} was modified by {}".format(
                                                 name, self.testname),
                                                 file=sys.stderr)
                    if self.verbose > 1:
                        print("  Before: {}\n  After:  {} ".format(
                                                  original, current),
                                                  file=sys.stderr)
        return False


def runtest_inner(test, verbose, quiet, huntrleaks=False, debug=False):
    support.unload(test)
    if verbose:
        capture_stdout = None
    else:
        capture_stdout = io.StringIO()

    test_time = 0.0
    refleak = False  # True if the test leaked references.
    try:
        if test.startswith('test.'):
            abstest = test
        else:
            # Always import it from the test package
            abstest = 'test.' + test
        with saved_test_environment(test, verbose, quiet) as environment:
            start_time = time.time()
            the_package = __import__(abstest, globals(), locals(), [])
            the_module = getattr(the_package, test)
            # Old tests run to completion simply as a side-effect of
            # being imported.  For tests based on unittest or doctest,
            # explicitly invoke their test_main() function (if it exists).
            indirect_test = getattr(the_module, "test_main", None)
            if indirect_test is not None:
                indirect_test()
            if huntrleaks:
                refleak = dash_R(the_module, test, indirect_test,
                    huntrleaks)
            test_time = time.time() - start_time
    except support.ResourceDenied as msg:
        if not quiet:
            print(test, "skipped --", msg)
            sys.stdout.flush()
        return RESOURCE_DENIED, test_time
    except unittest.SkipTest as msg:
        if not quiet:
            print(test, "skipped --", msg)
            sys.stdout.flush()
        return SKIPPED, test_time
    except KeyboardInterrupt:
        raise
    except support.TestFailed as msg:
        print("test", test, "failed --", msg, file=sys.stderr)
        sys.stderr.flush()
        return FAILED, test_time
    except:
        msg = traceback.format_exc()
        print("test", test, "crashed --", msg, file=sys.stderr)
        sys.stderr.flush()
        return FAILED, test_time
    else:
        if refleak:
            return FAILED, test_time
        if environment.changed:
            return ENV_CHANGED, test_time
        return PASSED, test_time

def cleanup_test_droppings(testname, verbose):
    import shutil
    import stat
    import gc

    # First kill any dangling references to open files etc.
    # This can also issue some ResourceWarnings which would otherwise get
    # triggered during the following test run, and possibly produce failures.
    gc.collect()

    # Try to clean up junk commonly left behind.  While tests shouldn't leave
    # any files or directories behind, when a test fails that can be tedious
    # for it to arrange.  The consequences can be especially nasty on Windows,
    # since if a test leaves a file open, it cannot be deleted by name (while
    # there's nothing we can do about that here either, we can display the
    # name of the offending test, which is a real help).
    for name in (support.TESTFN,
                 "db_home",
                ):
        if not os.path.exists(name):
            continue

        if os.path.isdir(name):
            kind, nuker = "directory", shutil.rmtree
        elif os.path.isfile(name):
            kind, nuker = "file", os.unlink
        else:
            raise SystemError("os.path says %r exists but is neither "
                              "directory nor file" % name)

        if verbose:
            print("%r left behind %s %r" % (testname, kind, name))
        try:
            # if we have chmod, fix possible permissions problems
            # that might prevent cleanup
            if (hasattr(os, 'chmod')):
                os.chmod(name, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
            nuker(name)
        except Exception as msg:
            print(("%r left behind %s %r and it couldn't be "
                "removed: %s" % (testname, kind, name, msg)), file=sys.stderr)

def dash_R(the_module, test, indirect_test, huntrleaks):
    """Run a test multiple times, looking for reference leaks.

    Returns:
        False if the test didn't leak references; True if we detected refleaks.
    """
    # This code is hackish and inelegant, but it seems to do the job.
    import copyreg
    import collections.abc

    if not hasattr(sys, 'gettotalrefcount'):
        raise Exception("Tracking reference leaks requires a debug build "
                        "of Python")

    # Save current values for dash_R_cleanup() to restore.
    fs = warnings.filters[:]
    ps = copyreg.dispatch_table.copy()
    pic = sys.path_importer_cache.copy()
    try:
        import zipimport
    except ImportError:
        zdc = None # Run unmodified on platforms without zipimport support
    else:
        zdc = zipimport._zip_directory_cache.copy()
    abcs = {}
    for abc in [getattr(collections.abc, a) for a in collections.abc.__all__]:
        if not isabstract(abc):
            continue
        for obj in abc.__subclasses__() + [abc]:
            abcs[obj] = obj._abc_registry.copy()

    if indirect_test:
        def run_the_test():
            indirect_test()
    else:
        def run_the_test():
            del sys.modules[the_module.__name__]
            exec('import ' + the_module.__name__)

    deltas = []
    nwarmup, ntracked, fname = huntrleaks
    fname = os.path.join(support.SAVEDCWD, fname)
    repcount = nwarmup + ntracked
    print("beginning", repcount, "repetitions", file=sys.stderr)
    print(("1234567890"*(repcount//10 + 1))[:repcount], file=sys.stderr)
    sys.stderr.flush()
    dash_R_cleanup(fs, ps, pic, zdc, abcs)
    for i in range(repcount):
        rc_before = sys.gettotalrefcount()
        run_the_test()
        sys.stderr.write('.')
        sys.stderr.flush()
        dash_R_cleanup(fs, ps, pic, zdc, abcs)
        rc_after = sys.gettotalrefcount()
        if i >= nwarmup:
            deltas.append(rc_after - rc_before)
    print(file=sys.stderr)
    if any(deltas):
        msg = '%s leaked %s references, sum=%s' % (test, deltas, sum(deltas))
        print(msg, file=sys.stderr)
        sys.stderr.flush()
        with open(fname, "a") as refrep:
            print(msg, file=refrep)
            refrep.flush()
        return True
    return False

def dash_R_cleanup(fs, ps, pic, zdc, abcs):
    import gc, copyreg
    import _strptime, linecache
    import urllib.parse, urllib.request, mimetypes, doctest
    import struct, filecmp, collections.abc
    from distutils.dir_util import _path_created
    from weakref import WeakSet

    # Clear the warnings registry, so they can be displayed again
    for mod in sys.modules.values():
        if hasattr(mod, '__warningregistry__'):
            del mod.__warningregistry__

    # Restore some original values.
    warnings.filters[:] = fs
    copyreg.dispatch_table.clear()
    copyreg.dispatch_table.update(ps)
    sys.path_importer_cache.clear()
    sys.path_importer_cache.update(pic)
    try:
        import zipimport
    except ImportError:
        pass # Run unmodified on platforms without zipimport support
    else:
        zipimport._zip_directory_cache.clear()
        zipimport._zip_directory_cache.update(zdc)

    # clear type cache
    sys._clear_type_cache()

    # Clear ABC registries, restoring previously saved ABC registries.
    for abc in [getattr(collections.abc, a) for a in collections.abc.__all__]:
        if not isabstract(abc):
            continue
        for obj in abc.__subclasses__() + [abc]:
            obj._abc_registry = abcs.get(obj, WeakSet()).copy()
            obj._abc_cache.clear()
            obj._abc_negative_cache.clear()

    # Flush standard output, so that buffered data is sent to the OS and
    # associated Python objects are reclaimed.
    for stream in (sys.stdout, sys.stderr, sys.__stdout__, sys.__stderr__):
        if stream is not None:
            stream.flush()

    # Clear assorted module caches.
    _path_created.clear()
    re.purge()
    _strptime._regex_cache.clear()
    urllib.parse.clear_cache()
    urllib.request.urlcleanup()
    linecache.clearcache()
    mimetypes._default_mime_types()
    filecmp._cache.clear()
    struct._clearcache()
    doctest.master = None

    # Collect cyclic trash.
    gc.collect()

def warm_char_cache():
    s = bytes(range(256))
    for i in range(256):
        s[i:i+1]

def findtestdir(path=None):
    return path or os.path.dirname(__file__) or os.curdir

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

# Map sys.platform to a string containing the basenames of tests
# expected to be skipped on that platform.
#
# Special cases:
#     test_pep277
#         The _ExpectedSkips constructor adds this to the set of expected
#         skips if not os.path.supports_unicode_filenames.
#     test_timeout
#         Controlled by test_timeout.skip_expected.  Requires the network
#         resource and a socket module.
#
# Tests that are expected to be skipped everywhere except on one platform
# are also handled separately.

_expectations = {
    'win32':
        """
        test__locale
        test_crypt
        test_curses
        test_dbm
        test_fcntl
        test_fork1
        test_epoll
        test_dbm_gnu
        test_dbm_ndbm
        test_grp
        test_ioctl
        test_largefile
        test_kqueue
        test_openpty
        test_ossaudiodev
        test_pipes
        test_poll
        test_posix
        test_pty
        test_pwd
        test_resource
        test_signal
        test_syslog
        test_threadsignals
        test_wait3
        test_wait4
        """,
    'linux2':
        """
        test_curses
        test_largefile
        test_kqueue
        test_ossaudiodev
        """,
    'unixware7':
        """
        test_epoll
        test_largefile
        test_kqueue
        test_minidom
        test_openpty
        test_pyexpat
        test_sax
        test_sundry
        """,
    'openunix8':
        """
        test_epoll
        test_largefile
        test_kqueue
        test_minidom
        test_openpty
        test_pyexpat
        test_sax
        test_sundry
        """,
    'sco_sv3':
        """
        test_asynchat
        test_fork1
        test_epoll
        test_gettext
        test_largefile
        test_locale
        test_kqueue
        test_minidom
        test_openpty
        test_pyexpat
        test_queue
        test_sax
        test_sundry
        test_thread
        test_threaded_import
        test_threadedtempfile
        test_threading
        """,
    'darwin':
        """
        test__locale
        test_curses
        test_epoll
        test_dbm_gnu
        test_gdb
        test_largefile
        test_locale
        test_minidom
        test_ossaudiodev
        test_poll
        """,
    'sunos5':
        """
        test_curses
        test_dbm
        test_epoll
        test_kqueue
        test_dbm_gnu
        test_gzip
        test_openpty
        test_zipfile
        test_zlib
        """,
    'hp-ux11':
        """
        test_curses
        test_epoll
        test_dbm_gnu
        test_gzip
        test_largefile
        test_locale
        test_kqueue
        test_minidom
        test_openpty
        test_pyexpat
        test_sax
        test_zipfile
        test_zlib
        """,
    'cygwin':
        """
        test_curses
        test_dbm
        test_epoll
        test_ioctl
        test_kqueue
        test_largefile
        test_locale
        test_ossaudiodev
        test_socketserver
        """,
    'os2emx':
        """
        test_audioop
        test_curses
        test_epoll
        test_kqueue
        test_largefile
        test_mmap
        test_openpty
        test_ossaudiodev
        test_pty
        test_resource
        test_signal
        """,
    'freebsd4':
        """
        test_epoll
        test_dbm_gnu
        test_locale
        test_ossaudiodev
        test_pep277
        test_pty
        test_socketserver
        test_tcl
        test_tk
        test_ttk_guionly
        test_ttk_textonly
        test_timeout
        test_urllibnet
        test_multiprocessing
        """,
    'aix5':
        """
        test_bz2
        test_epoll
        test_dbm_gnu
        test_gzip
        test_kqueue
        test_ossaudiodev
        test_tcl
        test_tk
        test_ttk_guionly
        test_ttk_textonly
        test_zipimport
        test_zlib
        """,
    'openbsd3':
        """
        test_ctypes
        test_epoll
        test_dbm_gnu
        test_locale
        test_normalization
        test_ossaudiodev
        test_pep277
        test_tcl
        test_tk
        test_ttk_guionly
        test_ttk_textonly
        test_multiprocessing
        """,
    'netbsd3':
        """
        test_ctypes
        test_curses
        test_epoll
        test_dbm_gnu
        test_locale
        test_ossaudiodev
        test_pep277
        test_tcl
        test_tk
        test_ttk_guionly
        test_ttk_textonly
        test_multiprocessing
        """,
}
_expectations['freebsd5'] = _expectations['freebsd4']
_expectations['freebsd6'] = _expectations['freebsd4']
_expectations['freebsd7'] = _expectations['freebsd4']
_expectations['freebsd8'] = _expectations['freebsd4']

class _ExpectedSkips:
    def __init__(self):
        import os.path
        from test import test_timeout

        self.valid = False
        if sys.platform in _expectations:
            s = _expectations[sys.platform]
            self.expected = set(s.split())

            # These are broken tests, for now skipped on every platform.
            # XXX Fix these!
            self.expected.add('test_nis')

            # expected to be skipped on every platform, even Linux
            if not os.path.supports_unicode_filenames:
                self.expected.add('test_pep277')

            # doctest, profile and cProfile tests fail when the codec for the
            # fs encoding isn't built in because PyUnicode_Decode() adds two
            # calls into Python.
            encs = ("utf-8", "latin-1", "ascii", "mbcs", "utf-16", "utf-32")
            if sys.getfilesystemencoding().lower() not in encs:
                self.expected.add('test_profile')
                self.expected.add('test_cProfile')
                self.expected.add('test_doctest')

            if test_timeout.skip_expected:
                self.expected.add('test_timeout')

            if sys.platform != "win32":
                # test_sqlite is only reliable on Windows where the library
                # is distributed with Python
                WIN_ONLY = {"test_unicode_file", "test_winreg",
                            "test_winsound", "test_startfile",
                            "test_sqlite"}
                self.expected |= WIN_ONLY

            if sys.platform != 'sunos5':
                self.expected.add('test_nis')

            if support.python_is_optimized():
                self.expected.add("test_gdb")

            self.valid = True

    def isvalid(self):
        "Return true iff _ExpectedSkips knows about the current platform."
        return self.valid

    def getexpected(self):
        """Return set of test names we expect to skip on current platform.

        self.isvalid() must be true.
        """

        assert self.isvalid()
        return self.expected

def _make_temp_dir_for_build(TEMPDIR):
    # When tests are run from the Python build directory, it is best practice
    # to keep the test files in a subfolder.  It eases the cleanup of leftover
    # files using command "make distclean".
    if sysconfig.is_python_build():
        TEMPDIR = os.path.join(sysconfig.get_config_var('srcdir'), 'build')
        TEMPDIR = os.path.abspath(TEMPDIR)
        if not os.path.exists(TEMPDIR):
            os.mkdir(TEMPDIR)

    # Define a writable temp dir that will be used as cwd while running
    # the tests. The name of the dir includes the pid to allow parallel
    # testing (see the -j option).
    TESTCWD = 'test_python_{}'.format(os.getpid())

    TESTCWD = os.path.join(TEMPDIR, TESTCWD)
    return TEMPDIR, TESTCWD

if __name__ == '__main__':
    # Display the Python traceback on segfault and division by zero
    faulthandler.enable()

    # Remove regrtest.py's own directory from the module search path. Despite
    # the elimination of implicit relative imports, this is still needed to
    # ensure that submodules of the test package do not inappropriately appear
    # as top-level modules even when people (or buildbots!) invoke regrtest.py
    # directly instead of using the -m switch
    mydir = os.path.abspath(os.path.normpath(os.path.dirname(sys.argv[0])))
    i = len(sys.path)
    while i >= 0:
        i -= 1
        if os.path.abspath(os.path.normpath(sys.path[i])) == mydir:
            del sys.path[i]

    # findtestdir() gets the dirname out of __file__, so we have to make it
    # absolute before changing the working directory.
    # For example __file__ may be relative when running trace or profile.
    # See issue #9323.
    __file__ = os.path.abspath(__file__)

    # sanity check
    assert __file__ == os.path.abspath(sys.argv[0])

    TEMPDIR, TESTCWD = _make_temp_dir_for_build(TEMPDIR)

    # Run the tests in a context manager that temporary changes the CWD to a
    # temporary and writable directory. If it's not possible to create or
    # change the CWD, the original CWD will be used. The original CWD is
    # available from support.SAVEDCWD.
    with support.temp_cwd(TESTCWD, quiet=True):
        main()
