#! /usr/bin/env python3

"""
Script to run Python regression tests.

Run this script with -h or --help for documentation.
"""

USAGE = """\
python -m test [options] [test_name1 [test_name2 ...]]
python path/to/Lib/test/regrtest.py [options] [test_name1 [test_name2 ...]]
"""

DESCRIPTION = """\
Run Python regression tests.

If no arguments or options are provided, finds all files matching
the pattern "test_*" in the Lib/test subdirectory and runs
them in alphabetical order (but see -M and -u, below, for exceptions).

For more rigorous testing, it is useful to use the following
command line:

python -E -Wd -m test [options] [test_name1 ...]
"""

EPILOG = """\
Additional option details:

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

    none -      Disable all special resources (this is the default).

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

# We import importlib *ASAP* in order to test #15386
import importlib

import argparse
import builtins
import faulthandler
import io
import json
import locale
import logging
import os
import platform
import random
import re
import shutil
import signal
import sys
import sysconfig
import tempfile
import time
import traceback
import unittest
import warnings
from inspect import isabstract

try:
    import threading
except ImportError:
    threading = None
try:
    import _multiprocessing, multiprocessing.process
except ImportError:
    multiprocessing = None


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
CHILD_ERROR = -5   # error in a child process

from test import support

RESOURCE_NAMES = ('audio', 'curses', 'largefile', 'network',
                  'decimal', 'cpu', 'subprocess', 'urlfetch', 'gui')

# When tests are run from the Python build directory, it is best practice
# to keep the test files in a subfolder.  This eases the cleanup of leftover
# files using the "make distclean" command.
if sysconfig.is_python_build():
    TEMPDIR = os.path.join(sysconfig.get_config_var('srcdir'), 'build')
else:
    TEMPDIR = tempfile.gettempdir()
TEMPDIR = os.path.abspath(TEMPDIR)

class _ArgParser(argparse.ArgumentParser):

    def error(self, message):
        super().error(message + "\nPass -h or --help for complete help.")

def _create_parser():
    # Set prog to prevent the uninformative "__main__.py" from displaying in
    # error messages when using "python -m test ...".
    parser = _ArgParser(prog='regrtest.py',
                        usage=USAGE,
                        description=DESCRIPTION,
                        epilog=EPILOG,
                        add_help=False,
                        formatter_class=argparse.RawDescriptionHelpFormatter)

    # Arguments with this clause added to its help are described further in
    # the epilog's "Additional option details" section.
    more_details = '  See the section at bottom for more details.'

    group = parser.add_argument_group('General options')
    # We add help explicitly to control what argument group it renders under.
    group.add_argument('-h', '--help', action='help',
                       help='show this help message and exit')
    group.add_argument('--timeout', metavar='TIMEOUT', type=float,
                        help='dump the traceback and exit if a test takes '
                             'more than TIMEOUT seconds; disabled if TIMEOUT '
                             'is negative or equals to zero')
    group.add_argument('--wait', action='store_true',
                       help='wait for user input, e.g., allow a debugger '
                            'to be attached')
    group.add_argument('--slaveargs', metavar='ARGS')
    group.add_argument('-S', '--start', metavar='START',
                       help='the name of the test at which to start.' +
                            more_details)

    group = parser.add_argument_group('Verbosity')
    group.add_argument('-v', '--verbose', action='count',
                       help='run tests in verbose mode with output to stdout')
    group.add_argument('-w', '--verbose2', action='store_true',
                       help='re-run failed tests in verbose mode')
    group.add_argument('-W', '--verbose3', action='store_true',
                       help='display test output on failure')
    group.add_argument('-q', '--quiet', action='store_true',
                       help='no output unless one or more tests fail')
    group.add_argument('-o', '--slow', action='store_true', dest='print_slow',
                       help='print the slowest 10 tests')
    group.add_argument('--header', action='store_true',
                       help='print header with interpreter info')

    group = parser.add_argument_group('Selecting tests')
    group.add_argument('-r', '--randomize', action='store_true',
                       help='randomize test execution order.' + more_details)
    group.add_argument('--randseed', metavar='SEED',
                       dest='random_seed', type=int,
                       help='pass a random seed to reproduce a previous '
                            'random run')
    group.add_argument('-f', '--fromfile', metavar='FILE',
                       help='read names of tests to run from a file.' +
                            more_details)
    group.add_argument('-x', '--exclude', action='store_true',
                       help='arguments are tests to *exclude*')
    group.add_argument('-s', '--single', action='store_true',
                       help='single step through a set of tests.' +
                            more_details)
    group.add_argument('-m', '--match', metavar='PAT',
                       dest='match_tests',
                       help='match test cases and methods with glob pattern PAT')
    group.add_argument('-G', '--failfast', action='store_true',
                       help='fail as soon as a test fails (only with -v or -W)')
    group.add_argument('-u', '--use', metavar='RES1,RES2,...',
                       action='append', type=resources_list,
                       help='specify which special resource intensive tests '
                            'to run.' + more_details)
    group.add_argument('-M', '--memlimit', metavar='LIMIT',
                       help='run very large memory-consuming tests.' +
                            more_details)
    group.add_argument('--testdir', metavar='DIR',
                       type=relative_filename,
                       help='execute test files in the specified directory '
                            '(instead of the Python stdlib test suite)')

    group = parser.add_argument_group('Special runs')
    group.add_argument('-l', '--findleaks', action='store_true',
                       help='if GC is available detect tests that leak memory')
    group.add_argument('-L', '--runleaks', action='store_true',
                       help='run the leaks(1) command just before exit.' +
                            more_details)
    group.add_argument('-R', '--huntrleaks', metavar='RUNCOUNTS',
                       type=huntrleaks,
                       help='search for reference leaks (needs debug build, '
                            'very slow).' + more_details)
    group.add_argument('-j', '--multiprocess', metavar='PROCESSES',
                       dest='use_mp', type=int,
                       help='run PROCESSES processes at once')
    group.add_argument('-T', '--coverage', action='store_true',
                       dest='trace',
                       help='turn on code coverage tracing using the trace '
                            'module')
    group.add_argument('-D', '--coverdir', metavar='DIR',
                       type=relative_filename,
                       help='directory where coverage files are put')
    group.add_argument('-N', '--nocoverdir',
                       action='store_const', const=None, dest='coverdir',
                       help='put coverage files alongside modules')
    group.add_argument('-t', '--threshold', metavar='THRESHOLD',
                       type=int,
                       help='call gc.set_threshold(THRESHOLD)')
    group.add_argument('-n', '--nowindows', action='store_true',
                       help='suppress error message boxes on Windows')
    group.add_argument('-F', '--forever', action='store_true',
                       help='run the specified tests in a loop, until an '
                            'error happens')

    parser.add_argument('args', nargs=argparse.REMAINDER,
                        help=argparse.SUPPRESS)

    return parser

def relative_filename(string):
    # CWD is replaced with a temporary dir before calling main(), so we
    # join it with the saved CWD so it ends up where the user expects.
    return os.path.join(support.SAVEDCWD, string)

def huntrleaks(string):
    args = string.split(':')
    if len(args) not in (2, 3):
        raise argparse.ArgumentTypeError(
            'needs 2 or 3 colon-separated arguments')
    nwarmup = int(args[0]) if args[0] else 5
    ntracked = int(args[1]) if args[1] else 4
    fname = args[2] if len(args) > 2 and args[2] else 'reflog.txt'
    return nwarmup, ntracked, fname

def resources_list(string):
    u = [x.lower() for x in string.split(',')]
    for r in u:
        if r == 'all' or r == 'none':
            continue
        if r[0] == '-':
            r = r[1:]
        if r not in RESOURCE_NAMES:
            raise argparse.ArgumentTypeError('invalid resource: ' + r)
    return u

def _parse_args(args, **kwargs):
    # Defaults
    ns = argparse.Namespace(testdir=None, verbose=0, quiet=False,
         exclude=False, single=False, randomize=False, fromfile=None,
         findleaks=False, use_resources=None, trace=False, coverdir='coverage',
         runleaks=False, huntrleaks=False, verbose2=False, print_slow=False,
         random_seed=None, use_mp=None, verbose3=False, forever=False,
         header=False, failfast=False, match_tests=None)
    for k, v in kwargs.items():
        if not hasattr(ns, k):
            raise TypeError('%r is an invalid keyword argument '
                            'for this function' % k)
        setattr(ns, k, v)
    if ns.use_resources is None:
        ns.use_resources = []

    parser = _create_parser()
    parser.parse_args(args=args, namespace=ns)

    if ns.single and ns.fromfile:
        parser.error("-s and -f don't go together!")
    if ns.use_mp and ns.trace:
        parser.error("-T and -j don't go together!")
    if ns.use_mp and ns.findleaks:
        parser.error("-l and -j don't go together!")
    if ns.use_mp and ns.memlimit:
        parser.error("-M and -j don't go together!")
    if ns.failfast and not (ns.verbose or ns.verbose3):
        parser.error("-G/--failfast needs either -v or -W")

    if ns.quiet:
        ns.verbose = 0
    if ns.timeout is not None:
        if hasattr(faulthandler, 'dump_traceback_later'):
            if ns.timeout <= 0:
                ns.timeout = None
        else:
            print("Warning: The timeout option requires "
                  "faulthandler.dump_traceback_later")
            ns.timeout = None
    if ns.use_mp is not None:
        if ns.use_mp <= 0:
            # Use all cores + extras for tests that like to sleep
            ns.use_mp = 2 + (os.cpu_count() or 1)
        if ns.use_mp == 1:
            ns.use_mp = None
    if ns.use:
        for a in ns.use:
            for r in a:
                if r == 'all':
                    ns.use_resources[:] = RESOURCE_NAMES
                    continue
                if r == 'none':
                    del ns.use_resources[:]
                    continue
                remove = False
                if r[0] == '-':
                    remove = True
                    r = r[1:]
                if remove:
                    if r in ns.use_resources:
                        ns.use_resources.remove(r)
                elif r not in ns.use_resources:
                    ns.use_resources.append(r)
    if ns.random_seed is not None:
        ns.randomize = True

    return ns


def run_test_in_subprocess(testname, ns):
    """Run the given test in a subprocess with --slaveargs.

    ns is the option Namespace parsed from command-line arguments. regrtest
    is invoked in a subprocess with the --slaveargs argument; when the
    subprocess exits, its return code, stdout and stderr are returned as a
    3-tuple.
    """
    from subprocess import Popen, PIPE
    base_cmd = ([sys.executable] + support.args_from_interpreter_flags() +
                ['-X', 'faulthandler', '-m', 'test.regrtest'])

    slaveargs = (
            (testname, ns.verbose, ns.quiet),
            dict(huntrleaks=ns.huntrleaks,
                 use_resources=ns.use_resources,
                 output_on_failure=ns.verbose3,
                 timeout=ns.timeout, failfast=ns.failfast,
                 match_tests=ns.match_tests))
    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    popen = Popen(base_cmd + ['--slaveargs', json.dumps(slaveargs)],
                  stdout=PIPE, stderr=PIPE,
                  universal_newlines=True,
                  close_fds=(os.name != 'nt'),
                  cwd=support.SAVEDCWD)
    stdout, stderr = popen.communicate()
    retcode = popen.wait()
    return retcode, stdout, stderr


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
                except:
                    raise
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
        bad = sorted(set(bad) - set(environment_changed))
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
        for test in bad:
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
            except:
                raise

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
    'test_support'
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

def runtest(test, verbose, quiet,
            huntrleaks=False, use_resources=None,
            output_on_failure=False, failfast=False, match_tests=None,
            timeout=None):
    """Run a single test.

    test -- the name of the test
    verbose -- if true, print more messages
    quiet -- if true, don't print 'skipped' messages (probably redundant)
    huntrleaks -- run multiple times to test for leaks; requires a debug
                  build; a triple corresponding to -R's three arguments
    use_resources -- list of extra resources to use
    output_on_failure -- if true, display test output on failure
    timeout -- dump the traceback and exit if a test takes more than
               timeout seconds
    failfast, match_tests -- See regrtest command-line flags for these.

    Returns the tuple result, test_time, where result is one of the constants:
        INTERRUPTED      KeyboardInterrupt when run under -j
        RESOURCE_DENIED  test skipped because resource denied
        SKIPPED          test skipped for some other reason
        ENV_CHANGED      test failed because it changed the execution environment
        FAILED           test failed
        PASSED           test passed
    """

    if use_resources is not None:
        support.use_resources = use_resources
    use_timeout = (timeout is not None)
    if use_timeout:
        faulthandler.dump_traceback_later(timeout, exit=True)
    try:
        support.match_tests = match_tests
        if failfast:
            support.failfast = True
        if output_on_failure:
            support.verbose = True

            # Reuse the same instance to all calls to runtest(). Some
            # tests keep a reference to sys.stdout or sys.stderr
            # (eg. test_argparse).
            if runtest.stringio is None:
                stream = io.StringIO()
                runtest.stringio = stream
            else:
                stream = runtest.stringio
                stream.seek(0)
                stream.truncate()

            orig_stdout = sys.stdout
            orig_stderr = sys.stderr
            try:
                sys.stdout = stream
                sys.stderr = stream
                result = runtest_inner(test, verbose, quiet, huntrleaks,
                                       display_failure=False)
                if result[0] == FAILED:
                    output = stream.getvalue()
                    orig_stderr.write(output)
                    orig_stderr.flush()
            finally:
                sys.stdout = orig_stdout
                sys.stderr = orig_stderr
        else:
            support.verbose = verbose  # Tell tests to be moderately quiet
            result = runtest_inner(test, verbose, quiet, huntrleaks,
                                   display_failure=not verbose)
        return result
    finally:
        if use_timeout:
            faulthandler.cancel_dump_traceback_later()
        cleanup_test_droppings(test, verbose)
runtest.stringio = None

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
                 'sys.warnoptions',
                 # multiprocessing.process._cleanup() may release ref
                 # to a thread, so check processes first.
                 'multiprocessing.process._dangling', 'threading._dangling',
                 'sysconfig._CONFIG_VARS', 'sysconfig._INSTALL_SCHEMES',
                 'support.TESTFN', 'locale', 'warnings.showwarning',
                )

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

    def get_shutil_archive_formats(self):
        # we could call get_archives_formats() but that only returns the
        # registry keys; we want to check the values too (the functions that
        # are registered)
        return shutil._ARCHIVE_FORMATS, shutil._ARCHIVE_FORMATS.copy()
    def restore_shutil_archive_formats(self, saved):
        shutil._ARCHIVE_FORMATS = saved[0]
        shutil._ARCHIVE_FORMATS.clear()
        shutil._ARCHIVE_FORMATS.update(saved[1])

    def get_shutil_unpack_formats(self):
        return shutil._UNPACK_FORMATS, shutil._UNPACK_FORMATS.copy()
    def restore_shutil_unpack_formats(self, saved):
        shutil._UNPACK_FORMATS = saved[0]
        shutil._UNPACK_FORMATS.clear()
        shutil._UNPACK_FORMATS.update(saved[1])

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

    # Controlling dangling references to Thread objects can make it easier
    # to track reference leaks.
    def get_threading__dangling(self):
        if not threading:
            return None
        # This copies the weakrefs without making any strong reference
        return threading._dangling.copy()
    def restore_threading__dangling(self, saved):
        if not threading:
            return
        threading._dangling.clear()
        threading._dangling.update(saved)

    # Same for Process objects
    def get_multiprocessing_process__dangling(self):
        if not multiprocessing:
            return None
        # Unjoined process objects can survive after process exits
        multiprocessing.process._cleanup()
        # This copies the weakrefs without making any strong reference
        return multiprocessing.process._dangling.copy()
    def restore_multiprocessing_process__dangling(self, saved):
        if not multiprocessing:
            return
        multiprocessing.process._dangling.clear()
        multiprocessing.process._dangling.update(saved)

    def get_sysconfig__CONFIG_VARS(self):
        # make sure the dict is initialized
        sysconfig.get_config_var('prefix')
        return (id(sysconfig._CONFIG_VARS), sysconfig._CONFIG_VARS,
                dict(sysconfig._CONFIG_VARS))
    def restore_sysconfig__CONFIG_VARS(self, saved):
        sysconfig._CONFIG_VARS = saved[1]
        sysconfig._CONFIG_VARS.clear()
        sysconfig._CONFIG_VARS.update(saved[2])

    def get_sysconfig__INSTALL_SCHEMES(self):
        return (id(sysconfig._INSTALL_SCHEMES), sysconfig._INSTALL_SCHEMES,
                sysconfig._INSTALL_SCHEMES.copy())
    def restore_sysconfig__INSTALL_SCHEMES(self, saved):
        sysconfig._INSTALL_SCHEMES = saved[1]
        sysconfig._INSTALL_SCHEMES.clear()
        sysconfig._INSTALL_SCHEMES.update(saved[2])

    def get_support_TESTFN(self):
        if os.path.isfile(support.TESTFN):
            result = 'f'
        elif os.path.isdir(support.TESTFN):
            result = 'd'
        else:
            result = None
        return result
    def restore_support_TESTFN(self, saved_value):
        if saved_value is None:
            if os.path.isfile(support.TESTFN):
                os.unlink(support.TESTFN)
            elif os.path.isdir(support.TESTFN):
                shutil.rmtree(support.TESTFN)

    _lc = [getattr(locale, lc) for lc in dir(locale)
           if lc.startswith('LC_')]
    def get_locale(self):
        pairings = []
        for lc in self._lc:
            try:
                pairings.append((lc, locale.setlocale(lc, None)))
            except (TypeError, ValueError):
                continue
        return pairings
    def restore_locale(self, saved):
        for lc, setting in saved:
            locale.setlocale(lc, setting)

    def get_warnings_showwarning(self):
        return warnings.showwarning
    def restore_warnings_showwarning(self, fxn):
        warnings.showwarning = fxn

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


def runtest_inner(test, verbose, quiet,
                  huntrleaks=False, display_failure=True):
    support.unload(test)

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
            the_module = importlib.import_module(abstest)
            # If the test has a test_main, that will run the appropriate
            # tests.  If not, use normal unittest test loading.
            test_runner = getattr(the_module, "test_main", None)
            if test_runner is None:
                tests = unittest.TestLoader().loadTestsFromModule(the_module)
                test_runner = lambda: support.run_unittest(tests)
            test_runner()
            if huntrleaks:
                refleak = dash_R(the_module, test, test_runner, huntrleaks)
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
        if display_failure:
            print("test", test, "failed --", msg, file=sys.stderr)
        else:
            print("test", test, "failed", file=sys.stderr)
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

    nwarmup, ntracked, fname = huntrleaks
    fname = os.path.join(support.SAVEDCWD, fname)
    repcount = nwarmup + ntracked
    rc_deltas = [0] * repcount
    alloc_deltas = [0] * repcount

    print("beginning", repcount, "repetitions", file=sys.stderr)
    print(("1234567890"*(repcount//10 + 1))[:repcount], file=sys.stderr)
    sys.stderr.flush()
    for i in range(repcount):
        indirect_test()
        alloc_after, rc_after = dash_R_cleanup(fs, ps, pic, zdc, abcs)
        sys.stderr.write('.')
        sys.stderr.flush()
        if i >= nwarmup:
            rc_deltas[i] = rc_after - rc_before
            alloc_deltas[i] = alloc_after - alloc_before
        alloc_before, rc_before = alloc_after, rc_after
    print(file=sys.stderr)
    # These checkers return False on success, True on failure
    def check_rc_deltas(deltas):
        return any(deltas)
    def check_alloc_deltas(deltas):
        # At least 1/3rd of 0s
        if 3 * deltas.count(0) < len(deltas):
            return True
        # Nothing else than 1s, 0s and -1s
        if not set(deltas) <= {1,0,-1}:
            return True
        return False
    failed = False
    for deltas, item_name, checker in [
        (rc_deltas, 'references', check_rc_deltas),
        (alloc_deltas, 'memory blocks', check_alloc_deltas)]:
        if checker(deltas):
            msg = '%s leaked %s %s, sum=%s' % (
                test, deltas[nwarmup:], item_name, sum(deltas))
            print(msg, file=sys.stderr)
            sys.stderr.flush()
            with open(fname, "a") as refrep:
                print(msg, file=refrep)
                refrep.flush()
            failed = True
    return failed

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
    try:
        import ctypes
    except ImportError:
        # Don't worry about resetting the cache if ctypes is not supported
        pass
    else:
        ctypes._reset_cache()

    # Collect cyclic trash and read memory statistics immediately after.
    func1 = sys.getallocatedblocks
    func2 = sys.gettotalrefcount
    gc.collect()
    return func1(), func2()

def warm_caches():
    # char cache
    s = bytes(range(256))
    for i in range(256):
        s[i:i+1]
    # unicode cache
    x = [chr(i) for i in range(256)]
    # int cache
    x = list(range(-5, 257))

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


if __name__ == '__main__':
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

    main_in_temp_cwd()
