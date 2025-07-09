import argparse
import os.path
import shlex
import sys
from test.support import os_helper, Py_DEBUG
from .utils import ALL_RESOURCES, RESOURCE_NAMES, TestFilter


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
int seed value for the randomizer. The randseed value will be used
to set seeds for all random usages in tests
(including randomizing the tests order if -r is set).
By default we always set random seed, but do not randomize test order.

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

-S is used to resume running tests after an interrupted run.  It will
maintain the order a standard run (i.e. it assumes -r is not used).
This is useful after the tests have prematurely stopped for some external
reason and you want to resume the run from where you left off rather
than starting from the beginning. Note: this is different from --prioritize.

--prioritize is used to influence the order of selected tests, such that
the tests listed as an argument are executed first. This is especially
useful when combined with -j and -r to pin the longest-running tests
to start at the beginning of a test run. Pass --prioritize=test_a,test_b
to make test_a run first, followed by test_b, and then the other tests.
If test_a wasn't selected for execution by regular means, --prioritize will
not make it execute.

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

    all -            Enable all special resources.

    none -           Disable all special resources (this is the default).

    audio -          Tests that use the audio device.  (There are known
                     cases of broken audio drivers that can crash Python or
                     even the Linux kernel.)

    curses -         Tests that use curses and will modify the terminal's
                     state and output modes.

    largefile -      It is okay to run some test that may create huge
                     files.  These tests can take a long time and may
                     consume >2 GiB of disk space temporarily.

    extralargefile - Like 'largefile', but even larger (and slower).

    network -        It is okay to run tests that use external network
                     resource, e.g. testing SSL support for sockets.

    decimal -        Test the decimal module against a large suite that
                     verifies compliance with standards.

    cpu -            Used for certain CPU-heavy tests.

    walltime -       Long running but not CPU-bound tests.

    subprocess       Run all tests for the subprocess module.

    urlfetch -       It is okay to download files required on testing.

    gui -            Run tests that require a running GUI.

    tzdata -         Run tests that require timezone data.

To enable all resources except one, use '-uall,-<resource>'.  For
example, to run all the tests except for the gui tests, give the
option '-uall,-gui'.

--matchfile filters tests using a text file, one pattern per line.
Pattern examples:

- test method: test_stat_attributes
- test class: FileTests
- test identifier: test_os.FileTests.test_stat_attributes
"""


class Namespace(argparse.Namespace):
    def __init__(self, **kwargs) -> None:
        self.ci = False
        self.testdir = None
        self.verbose = 0
        self.quiet = False
        self.exclude = False
        self.cleanup = False
        self.wait = False
        self.list_cases = False
        self.list_tests = False
        self.single = False
        self.randomize = False
        self.fromfile = None
        self.fail_env_changed = False
        self.use_resources: list[str] = []
        self.trace = False
        self.coverdir = 'coverage'
        self.runleaks = False
        self.huntrleaks: tuple[int, int, str] | None = None
        self.rerun = False
        self.verbose3 = False
        self.print_slow = False
        self.random_seed = None
        self.use_mp = None
        self.parallel_threads = None
        self.forever = False
        self.header = False
        self.failfast = False
        self.match_tests: TestFilter = []
        self.pgo = False
        self.pgo_extended = False
        self.tsan = False
        self.tsan_parallel = False
        self.worker_json = None
        self.start = None
        self.timeout = None
        self.memlimit = None
        self.threshold = None
        self.fail_rerun = False
        self.tempdir = None
        self._add_python_opts = True
        self.xmlpath = None
        self.single_process = False

        super().__init__(**kwargs)


class _ArgParser(argparse.ArgumentParser):

    def error(self, message):
        super().error(message + "\nPass -h or --help for complete help.")


class FilterAction(argparse.Action):
    def __call__(self, parser, namespace, value, option_string=None):
        items = getattr(namespace, self.dest)
        items.append((value, self.const))


class FromFileFilterAction(argparse.Action):
    def __call__(self, parser, namespace, value, option_string=None):
        items = getattr(namespace, self.dest)
        with open(value, encoding='utf-8') as fp:
            for line in fp:
                items.append((line.strip(), self.const))


def _create_parser():
    # Set prog to prevent the uninformative "__main__.py" from displaying in
    # error messages when using "python -m test ...".
    parser = _ArgParser(prog='regrtest.py',
                        usage=USAGE,
                        description=DESCRIPTION,
                        epilog=EPILOG,
                        add_help=False,
                        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.set_defaults(match_tests=[])

    # Arguments with this clause added to its help are described further in
    # the epilog's "Additional option details" section.
    more_details = '  See the section at bottom for more details.'

    group = parser.add_argument_group('General options')
    # We add help explicitly to control what argument group it renders under.
    group.add_argument('-h', '--help', action='help',
                       help='show this help message and exit')
    group.add_argument('--fast-ci', action='store_true',
                       help='Fast Continuous Integration (CI) mode used by '
                            'GitHub Actions')
    group.add_argument('--slow-ci', action='store_true',
                       help='Slow Continuous Integration (CI) mode used by '
                            'buildbot workers')
    group.add_argument('--timeout', metavar='TIMEOUT',
                        help='dump the traceback and exit if a test takes '
                             'more than TIMEOUT seconds; disabled if TIMEOUT '
                             'is negative or equals to zero')
    group.add_argument('--wait', action='store_true',
                       help='wait for user input, e.g., allow a debugger '
                            'to be attached')
    group.add_argument('-S', '--start', metavar='START',
                       help='resume an interrupted run at the following test.' +
                            more_details)
    group.add_argument('-p', '--python', metavar='PYTHON',
                       help='Command to run Python test subprocesses with.')
    group.add_argument('--randseed', metavar='SEED',
                       dest='random_seed', type=int,
                       help='pass a global random seed')

    group = parser.add_argument_group('Verbosity')
    group.add_argument('-v', '--verbose', action='count',
                       help='run tests in verbose mode with output to stdout')
    group.add_argument('-w', '--rerun', action='store_true',
                       help='re-run failed tests in verbose mode')
    group.add_argument('--verbose2', action='store_true', dest='rerun',
                       help='deprecated alias to --rerun')
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
    group.add_argument('--prioritize', metavar='TEST1,TEST2,...',
                       action='append', type=priority_list,
                       help='select these tests first, even if the order is'
                            ' randomized.' + more_details)
    group.add_argument('-f', '--fromfile', metavar='FILE',
                       help='read names of tests to run from a file.' +
                            more_details)
    group.add_argument('-x', '--exclude', action='store_true',
                       help='arguments are tests to *exclude*')
    group.add_argument('-s', '--single', action='store_true',
                       help='single step through a set of tests.' +
                            more_details)
    group.add_argument('-m', '--match', metavar='PAT',
                       dest='match_tests', action=FilterAction, const=True,
                       help='match test cases and methods with glob pattern PAT')
    group.add_argument('-i', '--ignore', metavar='PAT',
                       dest='match_tests', action=FilterAction, const=False,
                       help='ignore test cases and methods with glob pattern PAT')
    group.add_argument('--matchfile', metavar='FILENAME',
                       dest='match_tests',
                       action=FromFileFilterAction, const=True,
                       help='similar to --match but get patterns from a '
                            'text file, one pattern per line')
    group.add_argument('--ignorefile', metavar='FILENAME',
                       dest='match_tests',
                       action=FromFileFilterAction, const=False,
                       help='similar to --matchfile but it receives patterns '
                            'from text file to ignore')
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
    group.add_argument('--single-process', action='store_true',
                       dest='single_process',
                       help='always run all tests sequentially in '
                            'a single process, ignore -jN option, '
                            'and failed tests are also rerun sequentially '
                            'in the same process')
    group.add_argument('--parallel-threads', metavar='PARALLEL_THREADS',
                       type=int,
                       help='run copies of each test in PARALLEL_THREADS at '
                            'once')
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
                       help='enable Profile Guided Optimization (PGO) training')
    group.add_argument('--pgo-extended', action='store_true',
                       help='enable extended PGO training (slower training)')
    group.add_argument('--tsan', dest='tsan', action='store_true',
                       help='run a subset of test cases that are proper for the TSAN test')
    group.add_argument('--tsan-parallel', action='store_true',
                       help='run a subset of test cases that are appropriate '
                            'for TSAN with `--parallel-threads=N`')
    group.add_argument('--fail-env-changed', action='store_true',
                       help='if a test file alters the environment, mark '
                            'the test as failed')
    group.add_argument('--fail-rerun', action='store_true',
                       help='if a test failed and then passed when re-run, '
                            'mark the tests as failed')

    group.add_argument('--junit-xml', dest='xmlpath', metavar='FILENAME',
                       help='writes JUnit-style XML results to the specified '
                            'file')
    group.add_argument('--tempdir', metavar='PATH',
                       help='override the working directory for the test run')
    group.add_argument('--cleanup', action='store_true',
                       help='remove old test_python_* directories')
    group.add_argument('--bisect', action='store_true',
                       help='if some tests fail, run test.bisect_cmd on them')
    group.add_argument('--dont-add-python-opts', dest='_add_python_opts',
                       action='store_false',
                       help="internal option, don't use it")
    return parser


def relative_filename(string):
    # CWD is replaced with a temporary dir before calling main(), so we
    # join it with the saved CWD so it ends up where the user expects.
    return os.path.join(os_helper.SAVEDCWD, string)


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


def priority_list(string):
    return string.split(",")


def _parse_args(args, **kwargs):
    # Defaults
    ns = Namespace()
    for k, v in kwargs.items():
        if not hasattr(ns, k):
            raise TypeError('%r is an invalid keyword argument '
                            'for this function' % k)
        setattr(ns, k, v)

    parser = _create_parser()
    # Issue #14191: argparse doesn't support "intermixed" positional and
    # optional arguments. Use parse_known_args() as workaround.
    ns.args = parser.parse_known_args(args=args, namespace=ns)[1]
    for arg in ns.args:
        if arg.startswith('-'):
            parser.error("unrecognized arguments: %s" % arg)

    if ns.timeout is not None:
        # Support "--timeout=" (no value) so Makefile.pre.pre TESTTIMEOUT
        # can be used by "make buildbottest" and "make test".
        if ns.timeout != "":
            try:
                ns.timeout = float(ns.timeout)
            except ValueError:
                parser.error(f"invalid timeout value: {ns.timeout!r}")
        else:
            ns.timeout = None

    # Continuous Integration (CI): common options for fast/slow CI modes
    if ns.slow_ci or ns.fast_ci:
        # Similar to options:
        #   -j0 --randomize --fail-env-changed --rerun --slowest --verbose3
        if ns.use_mp is None:
            ns.use_mp = 0
        ns.randomize = True
        ns.fail_env_changed = True
        if ns.python is None:
            ns.rerun = True
        ns.print_slow = True
        ns.verbose3 = True
    else:
        ns._add_python_opts = False

    # --singleprocess overrides -jN option
    if ns.single_process:
        ns.use_mp = None

    # When both --slow-ci and --fast-ci options are present,
    # --slow-ci has the priority
    if ns.slow_ci:
        # Similar to: -u "all" --timeout=1200
        if ns.use is None:
            ns.use = []
        ns.use.insert(0, ['all'])
        if ns.timeout is None:
            ns.timeout = 1200  # 20 minutes
    elif ns.fast_ci:
        # Similar to: -u "all,-cpu" --timeout=600
        if ns.use is None:
            ns.use = []
        ns.use.insert(0, ['all', '-cpu'])
        if ns.timeout is None:
            ns.timeout = 600  # 10 minutes

    if ns.single and ns.fromfile:
        parser.error("-s and -f don't go together!")
    if ns.trace:
        if ns.use_mp is not None:
            if not Py_DEBUG:
                parser.error("need --with-pydebug to use -T and -j together")
        else:
            print(
                "Warning: collecting coverage without -j is imprecise. Configure"
                " --with-pydebug and run -m test -T -j for best results.",
                file=sys.stderr
            )
    if ns.python is not None:
        if ns.use_mp is None:
            parser.error("-p requires -j!")
        # The "executable" may be two or more parts, e.g. "node python.js"
        ns.python = shlex.split(ns.python)
    if ns.failfast and not (ns.verbose or ns.verbose3):
        parser.error("-G/--failfast needs either -v or -W")
    if ns.pgo and (ns.verbose or ns.rerun or ns.verbose3):
        parser.error("--pgo/-v don't go together!")
    if ns.pgo_extended:
        ns.pgo = True  # pgo_extended implies pgo

    if ns.nowindows:
        print("Warning: the --nowindows (-n) option is deprecated. "
              "Use -vv to display assertions in stderr.", file=sys.stderr)

    if ns.quiet:
        ns.verbose = 0
    if ns.timeout is not None:
        if ns.timeout <= 0:
            ns.timeout = None
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

    # When -jN option is used, a worker process does not use --verbose3
    # and so -R 3:3 -jN --verbose3 just works as expected: there is no false
    # alarm about memory leak.
    if ns.huntrleaks and ns.verbose3 and ns.use_mp is None:
        # run_single_test() replaces sys.stdout with io.StringIO if verbose3
        # is true. In this case, huntrleaks sees an write into StringIO as
        # a memory leak, whereas it is not (gh-71290).
        ns.verbose3 = False
        print("WARNING: Disable --verbose3 because it's incompatible with "
              "--huntrleaks without -jN option",
              file=sys.stderr)

    if ns.forever:
        # --forever implies --failfast
        ns.failfast = True

    if ns.huntrleaks:
        warmup, repetitions, _ = ns.huntrleaks
        if warmup < 1 or repetitions < 1:
            msg = ("Invalid values for the --huntrleaks/-R parameters. The "
                   "number of warmups and repetitions must be at least 1 "
                   "each (1:1).")
            print(msg, file=sys.stderr, flush=True)
            sys.exit(2)

    ns.prioritize = [
        test
        for test_list in (ns.prioritize or ())
        for test in test_list
    ]

    return ns
