import argparse
import os
import sys
from test import support


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

-r randomizes test execution order. You can use --randseed=int to provide an
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
which is a string in the form of '2.5Gb', determines how much memory the
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
                consume >2 GiB of disk space temporarily.

    network -   It is okay to run tests that use external network
                resource, e.g. testing SSL support for sockets.

    decimal -   Test the decimal module against a large suite that
                verifies compliance with standards.

    cpu -       Used for certain CPU-heavy tests.

    subprocess  Run all tests for the subprocess module.

    urlfetch -  It is okay to download files required on testing.

    gui -       Run tests that require a running GUI.

    tzdata -    Run tests that require timezone data.

To enable all resources except one, use '-uall,-<resource>'.  For
example, to run all the tests except for the gui tests, give the
option '-uall,-gui'.

--matchfile filters tests using a text file, one pattern per line.
Pattern examples:

- test method: test_stat_attributes
- test class: FileTests
- test identifier: test_os.FileTests.test_stat_attributes
"""


ALL_RESOURCES = ('audio', 'curses', 'largefile', 'network',
                 'decimal', 'cpu', 'subprocess', 'urlfetch', 'gui')

# Other resources excluded from --use=all:
#
# - extralagefile (ex: test_zipfile64): really too slow to be enabled
#   "by default"
# - tzdata: while needed to validate fully test_datetime, it makes
#   test_datetime too slow (15-20 min on some buildbots) and so is disabled by
#   default (see bpo-30822).
RESOURCE_NAMES = ALL_RESOURCES + ('extralargefile', 'tzdata')

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
    group.add_argument('--worker-args', metavar='ARGS')
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
    group.add_argument('-o', '--slowest', action='store_true', dest='print_slow',
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
                       dest='match_tests', action='append',
                       help='match test cases and methods with glob pattern PAT')
    group.add_argument('--matchfile', metavar='FILENAME',
                       dest='match_filename',
                       help='similar to --match but get patterns from a '
                            'text file, one pattern per line')
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
    group.add_argument('-l', '--findleaks', action='store_const', const=2,
                       default=1,
                       help='deprecated alias to --fail-env-changed')
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
                            'error happens; imply --failfast')
    group.add_argument('--list-tests', action='store_true',
                       help="only write the name of tests that will be run, "
                            "don't execute them")
    group.add_argument('--list-cases', action='store_true',
                       help='only write the name of test cases that will be run'
                            ' , don\'t execute them')
    group.add_argument('-P', '--pgo', dest='pgo', action='store_true',
                       help='enable Profile Guided Optimization training')
    group.add_argument('--fail-env-changed', action='store_true',
                       help='if a test file alters the environment, mark '
                            'the test as failed')

    group.add_argument('--junit-xml', dest='xmlpath', metavar='FILENAME',
                       help='writes JUnit-style XML results to the specified '
                            'file')
    group.add_argument('--tempdir', dest='tempdir', metavar='PATH',
                       help='override the working directory for the test run')
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
         findleaks=1, use_resources=None, trace=False, coverdir='coverage',
         runleaks=False, huntrleaks=False, verbose2=False, print_slow=False,
         random_seed=None, use_mp=None, verbose3=False, forever=False,
         header=False, failfast=False, match_tests=None, pgo=False)
    for k, v in kwargs.items():
        if not hasattr(ns, k):
            raise TypeError('%r is an invalid keyword argument '
                            'for this function' % k)
        setattr(ns, k, v)
    if ns.use_resources is None:
        ns.use_resources = []

    parser = _create_parser()
    # Issue #14191: argparse doesn't support "intermixed" positional and
    # optional arguments. Use parse_known_args() as workaround.
    ns.args = parser.parse_known_args(args=args, namespace=ns)[1]
    for arg in ns.args:
        if arg.startswith('-'):
            parser.error("unrecognized arguments: %s" % arg)
            sys.exit(1)

    if ns.findleaks > 1:
        # --findleaks implies --fail-env-changed
        ns.fail_env_changed = True
    if ns.single and ns.fromfile:
        parser.error("-s and -f don't go together!")
    if ns.use_mp is not None and ns.trace:
        parser.error("-T and -j don't go together!")
    if ns.failfast and not (ns.verbose or ns.verbose3):
        parser.error("-G/--failfast needs either -v or -W")
    if ns.pgo and (ns.verbose or ns.verbose2 or ns.verbose3):
        parser.error("--pgo/-v don't go together!")

    if ns.nowindows:
        print("Warning: the --nowindows (-n) option is deprecated. "
              "Use -vv to display assertions in stderr.", file=sys.stderr)

    if ns.quiet:
        ns.verbose = 0
    if ns.timeout is not None:
        if ns.timeout <= 0:
            ns.timeout = None
    if ns.use_mp is not None:
        if ns.use_mp <= 0:
            # Use all cores + extras for tests that like to sleep
            ns.use_mp = 2 + (os.cpu_count() or 1)
    if ns.use:
        for a in ns.use:
            for r in a:
                if r == 'all':
                    ns.use_resources[:] = ALL_RESOURCES
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
    if ns.verbose:
        ns.header = True
    if ns.huntrleaks and ns.verbose3:
        ns.verbose3 = False
        print("WARNING: Disable --verbose3 because it's incompatible with "
              "--huntrleaks: see http://bugs.python.org/issue27103",
              file=sys.stderr)
    if ns.match_filename:
        if ns.match_tests is None:
            ns.match_tests = []
        with open(ns.match_filename) as fp:
            for line in fp:
                ns.match_tests.append(line.strip())
    if ns.forever:
        # --forever implies --failfast
        ns.failfast = True

    return ns
