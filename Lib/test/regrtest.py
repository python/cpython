#! /usr/bin/env python

"""Regression test.

This will find all modules whose name is "test_*" in the test
directory, and run them.  Various command line options provide
additional facilities.

Command line options:

-v: verbose    -- run tests in verbose mode with output to stdout
-q: quiet      -- don't print anything except if a test fails
-g: generate   -- write the output file for a test instead of comparing it
-x: exclude    -- arguments are tests to *exclude*
-s: single     -- run only a single test (see below)
-r: random     -- randomize test execution order
-f: fromfile   -- read names of tests to run from a file (see below)
-l: findleaks  -- if GC is available detect tests that leak memory
-u: use        -- specify which special resource intensive tests to run
-h: help       -- print this text and exit
-t: threshold  -- call gc.set_threshold(N)
-T: coverage   -- turn on code coverage using the trace module
-D: coverdir   -- Directory where coverage files are put
-N: nocoverdir -- Put coverage files alongside modules
-L: runleaks   -- run the leaks(1) command just before exit
-R: huntrleaks -- search for reference leaks (needs debug build, v. slow)

If non-option arguments are present, they are names for tests to run,
unless -x is given, in which case they are names for tests not to run.
If no test names are given, all tests are run.

-v is incompatible with -g and does not compare test output files.

-T turns on code coverage tracing with the trace module.

-D specifies the directory where coverage files are put.

-N Put coverage files alongside modules.

-s means to run only a single test and exit.  This is useful when
doing memory analysis on the Python interpreter (which tend to consume
too many resources to run the full regression test non-stop).  The
file /tmp/pynexttest is read to find the next test to run.  If this
file is missing, the first test_*.py file in testdir or on the command
line is used.  (actually tempfile.gettempdir() is used instead of
/tmp).

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
"reflog.txt" respectively), so the minimal invocation is '-R ::'.

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

    bsddb -     It is okay to run the bsddb testsuite, which takes
                a long time to complete.

    decimal -   Test the decimal module against a large suite that
                verifies compliance with standards.

    compiler -  Test the compiler package by compiling all the source
                in the standard library and test suite.  This takes
                a long time.

    subprocess  Run all tests for the subprocess module. 

To enable all resources except one, use '-uall,-<resource>'.  For
example, to run all the tests except for the bsddb tests, give the
option '-uall,-bsddb'.
"""

import os
import sys
import getopt
import random
import warnings
import sre
import cStringIO
import traceback

# I see no other way to suppress these warnings;
# putting them in test_grammar.py has no effect:
warnings.filterwarnings("ignore", "hex/oct constants", FutureWarning,
                        ".*test.test_grammar$")
if sys.maxint > 0x7fffffff:
    # Also suppress them in <string>, because for 64-bit platforms,
    # that's where test_grammar.py hides them.
    warnings.filterwarnings("ignore", "hex/oct constants", FutureWarning,
                            "<string>")

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

from test import test_support

RESOURCE_NAMES = ('audio', 'curses', 'largefile', 'network', 'bsddb',
                  'decimal', 'compiler', 'subprocess')


def usage(code, msg=''):
    print __doc__
    if msg: print msg
    sys.exit(code)


def main(tests=None, testdir=None, verbose=0, quiet=False, generate=False,
         exclude=False, single=False, randomize=False, fromfile=None,
         findleaks=False, use_resources=None, trace=False, coverdir='coverage',
         runleaks=False, huntrleaks=False):
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

    The other default arguments (verbose, quiet, generate, exclude, single,
    randomize, findleaks, use_resources, trace and coverdir) allow programmers
    calling main() directly to set the values that would normally be set by
    flags on the command line.
    """

    test_support.record_original_stdout(sys.stdout)
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hvgqxsrf:lu:t:TD:NLR:',
                                   ['help', 'verbose', 'quiet', 'generate',
                                    'exclude', 'single', 'random', 'fromfile',
                                    'findleaks', 'use=', 'threshold=', 'trace',
                                    'coverdir=', 'nocoverdir', 'runleaks',
                                    'huntrleaks='
                                    ])
    except getopt.error, msg:
        usage(2, msg)

    # Defaults
    if use_resources is None:
        use_resources = []
    for o, a in opts:
        if o in ('-h', '--help'):
            usage(0)
        elif o in ('-v', '--verbose'):
            verbose += 1
        elif o in ('-q', '--quiet'):
            quiet = True;
            verbose = 0
        elif o in ('-g', '--generate'):
            generate = True
        elif o in ('-x', '--exclude'):
            exclude = True
        elif o in ('-s', '--single'):
            single = True
        elif o in ('-r', '--randomize'):
            randomize = True
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
            coverdir = os.path.join(os.getcwd(), a)
        elif o in ('-N', '--nocoverdir'):
            coverdir = None
        elif o in ('-R', '--huntrleaks'):
            huntrleaks = a.split(':')
            if len(huntrleaks) != 3:
                print a, huntrleaks
                usage(2, '-R takes three colon-separated arguments')
            if len(huntrleaks[0]) == 0:
                huntrleaks[0] = 5
            else:
                huntrleaks[0] = int(huntrleaks[0])
            if len(huntrleaks[1]) == 0:
                huntrleaks[1] = 4
            else:
                huntrleaks[1] = int(huntrleaks[1])
            if len(huntrleaks[2]) == 0:
                huntrleaks[2] = "reflog.txt"
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
                    usage(1, 'Invalid -u/--use option: ' + a)
                if remove:
                    if r in use_resources:
                        use_resources.remove(r)
                elif r not in use_resources:
                    use_resources.append(r)
    if generate and verbose:
        usage(2, "-g and -v don't go together!")
    if single and fromfile:
        usage(2, "-s and -f don't go together!")

    good = []
    bad = []
    skipped = []
    resource_denieds = []

    if findleaks:
        try:
            import gc
        except ImportError:
            print 'No GC available, disabling findleaks.'
            findleaks = False
        else:
            # Uncomment the line below to report garbage that is not
            # freeable by reference counting alone.  By default only
            # garbage that is not collectable by the GC is reported.
            #gc.set_debug(gc.DEBUG_SAVEALL)
            found_garbage = []

    if single:
        from tempfile import gettempdir
        filename = os.path.join(gettempdir(), 'pynexttest')
        try:
            fp = open(filename, 'r')
            next = fp.read().strip()
            tests = [next]
            fp.close()
        except IOError:
            pass

    if fromfile:
        tests = []
        fp = open(fromfile)
        for line in fp:
            guts = line.split() # assuming no test has whitespace in its name
            if guts and not guts[0].startswith('#'):
                tests.extend(guts)
        fp.close()

    # Strip .py extensions.
    if args:
        args = map(removepy, args)
    if tests:
        tests = map(removepy, tests)

    stdtests = STDTESTS[:]
    nottests = NOTTESTS[:]
    if exclude:
        for arg in args:
            if arg in stdtests:
                stdtests.remove(arg)
        nottests[:0] = args
        args = []
    tests = tests or args or findtests(testdir, stdtests, nottests)
    if single:
        tests = tests[:1]
    if randomize:
        random.shuffle(tests)
    if trace:
        import trace
        tracer = trace.Trace(ignoredirs=[sys.prefix, sys.exec_prefix],
                             trace=False, count=True)
    test_support.verbose = verbose      # Tell tests to be moderately quiet
    test_support.use_resources = use_resources
    save_modules = sys.modules.keys()
    for test in tests:
        if not quiet:
            print test
            sys.stdout.flush()
        if trace:
            # If we're tracing code coverage, then we don't exit with status
            # if on a false return value from main.
            tracer.runctx('runtest(test, generate, verbose, quiet, testdir)',
                          globals=globals(), locals=vars())
        else:
            ok = runtest(test, generate, verbose, quiet, testdir, huntrleaks)
            if ok > 0:
                good.append(test)
            elif ok == 0:
                bad.append(test)
            else:
                skipped.append(test)
                if ok == -2:
                    resource_denieds.append(test)
        if findleaks:
            gc.collect()
            if gc.garbage:
                print "Warning: test created", len(gc.garbage),
                print "uncollectable object(s)."
                # move the uncollectable objects somewhere so we don't see
                # them again
                found_garbage.extend(gc.garbage)
                del gc.garbage[:]
        # Unload the newly imported modules (best effort finalization)
        for module in sys.modules.keys():
            if module not in save_modules and module.startswith("test."):
                test_support.unload(module)

    # The lists won't be sorted if running with -r
    good.sort()
    bad.sort()
    skipped.sort()

    if good and not quiet:
        if not bad and not skipped and len(good) > 1:
            print "All",
        print count(len(good), "test"), "OK."
        if verbose:
            print "CAUTION:  stdout isn't compared in verbose mode:"
            print "a test that passes in verbose mode may fail without it."
    if bad:
        print count(len(bad), "test"), "failed:"
        printlist(bad)
    if skipped and not quiet:
        print count(len(skipped), "test"), "skipped:"
        printlist(skipped)

        e = _ExpectedSkips()
        plat = sys.platform
        if e.isvalid():
            surprise = set(skipped) - e.getexpected() - set(resource_denieds)
            if surprise:
                print count(len(surprise), "skip"), \
                      "unexpected on", plat + ":"
                printlist(surprise)
            else:
                print "Those skips are all expected on", plat + "."
        else:
            print "Ask someone to teach regrtest.py about which tests are"
            print "expected to get skipped on", plat + "."

    if single:
        alltests = findtests(testdir, stdtests, nottests)
        for i in range(len(alltests)):
            if tests[0] == alltests[i]:
                if i == len(alltests) - 1:
                    os.unlink(filename)
                else:
                    fp = open(filename, 'w')
                    fp.write(alltests[i+1] + '\n')
                    fp.close()
                break
        else:
            os.unlink(filename)

    if trace:
        r = tracer.results()
        r.write_results(show_missing=True, summary=True, coverdir=coverdir)

    if runleaks:
        os.system("leaks %d" % os.getpid())

    sys.exit(len(bad) > 0)


STDTESTS = [
    'test_grammar',
    'test_opcodes',
    'test_operations',
    'test_builtin',
    'test_exceptions',
    'test_types',
   ]

NOTTESTS = [
    'test_support',
    'test_future1',
    'test_future2',
    'test_future3',
    ]

def findtests(testdir=None, stdtests=STDTESTS, nottests=NOTTESTS):
    """Return a list of all applicable test modules."""
    if not testdir: testdir = findtestdir()
    names = os.listdir(testdir)
    tests = []
    for name in names:
        if name[:5] == "test_" and name[-3:] == os.extsep+"py":
            modname = name[:-3]
            if modname not in stdtests and modname not in nottests:
                tests.append(modname)
    tests.sort()
    return stdtests + tests

def runtest(test, generate, verbose, quiet, testdir=None, huntrleaks=False):
    """Run a single test.
    test -- the name of the test
    generate -- if true, generate output, instead of running the test
    and comparing it to a previously created output file
    verbose -- if true, print more messages
    quiet -- if true, don't print 'skipped' messages (probably redundant)
    testdir -- test directory
    """
    test_support.unload(test)
    if not testdir:
        testdir = findtestdir()
    outputdir = os.path.join(testdir, "output")
    outputfile = os.path.join(outputdir, test)
    if verbose:
        cfp = None
    else:
        cfp = cStringIO.StringIO()
    if huntrleaks:
        refrep = open(huntrleaks[2], "a")
    try:
        save_stdout = sys.stdout
        try:
            if cfp:
                sys.stdout = cfp
                print test              # Output file starts with test name
            if test.startswith('test.'):
                abstest = test
            else:
                # Always import it from the test package
                abstest = 'test.' + test
            the_package = __import__(abstest, globals(), locals(), [])
            the_module = getattr(the_package, test)
            # Most tests run to completion simply as a side-effect of
            # being imported.  For the benefit of tests that can't run
            # that way (like test_threaded_import), explicitly invoke
            # their test_main() function (if it exists).
            indirect_test = getattr(the_module, "test_main", None)
            if indirect_test is not None:
                indirect_test()
            if huntrleaks:
                # This code *is* hackish and inelegant, yes.
                # But it seems to do the job.
                import copy_reg
                fs = warnings.filters[:]
                ps = copy_reg.dispatch_table.copy()
                pic = sys.path_importer_cache.copy()
                import gc
                def cleanup():
                    import _strptime, urlparse, warnings, dircache
                    from distutils.dir_util import _path_created
                    _path_created.clear()
                    warnings.filters[:] = fs
                    gc.collect()
                    sre.purge()
                    _strptime._regex_cache.clear()
                    urlparse.clear_cache()
                    copy_reg.dispatch_table.clear()
                    copy_reg.dispatch_table.update(ps)
                    sys.path_importer_cache.clear()
                    sys.path_importer_cache.update(pic)
                    dircache.reset()
                if indirect_test:
                    def run_the_test():
                        indirect_test()
                else:
                    def run_the_test():
                        reload(the_module)
                deltas = []
                repcount = huntrleaks[0] + huntrleaks[1]
                print >> sys.stderr, "beginning", repcount, "repetitions"
                print >> sys.stderr, \
                      ("1234567890"*(repcount//10 + 1))[:repcount]
                for i in range(repcount):
                    rc = sys.gettotalrefcount()
                    run_the_test()
                    sys.stderr.write('.')
                    cleanup()
                    deltas.append(sys.gettotalrefcount() - rc - 2)
                print >>sys.stderr
                if max(map(abs, deltas[-huntrleaks[1]:])) > 0:
                    print >>sys.stderr, test, 'leaked', \
                          deltas[-huntrleaks[1]:], 'references'
                    print >>refrep, test, 'leaked', \
                          deltas[-huntrleaks[1]:], 'references'
                # The end of the huntrleaks hackishness.
        finally:
            sys.stdout = save_stdout
    except test_support.ResourceDenied, msg:
        if not quiet:
            print test, "skipped --", msg
            sys.stdout.flush()
        return -2
    except (ImportError, test_support.TestSkipped), msg:
        if not quiet:
            print test, "skipped --", msg
            sys.stdout.flush()
        return -1
    except KeyboardInterrupt:
        raise
    except test_support.TestFailed, msg:
        print "test", test, "failed --", msg
        sys.stdout.flush()
        return 0
    except:
        type, value = sys.exc_info()[:2]
        print "test", test, "crashed --", str(type) + ":", value
        sys.stdout.flush()
        if verbose:
            traceback.print_exc(file=sys.stdout)
            sys.stdout.flush()
        return 0
    else:
        if not cfp:
            return 1
        output = cfp.getvalue()
        if generate:
            if output == test + "\n":
                if os.path.exists(outputfile):
                    # Write it since it already exists (and the contents
                    # may have changed), but let the user know it isn't
                    # needed:
                    print "output file", outputfile, \
                          "is no longer needed; consider removing it"
                else:
                    # We don't need it, so don't create it.
                    return 1
            fp = open(outputfile, "w")
            fp.write(output)
            fp.close()
            return 1
        if os.path.exists(outputfile):
            fp = open(outputfile, "r")
            expected = fp.read()
            fp.close()
        else:
            expected = test + "\n"
        if output == expected or huntrleaks:
            return 1
        print "test", test, "produced unexpected output:"
        sys.stdout.flush()
        reportdiff(expected, output)
        sys.stdout.flush()
        return 0

def reportdiff(expected, output):
    import difflib
    print "*" * 70
    a = expected.splitlines(1)
    b = output.splitlines(1)
    sm = difflib.SequenceMatcher(a=a, b=b)
    tuples = sm.get_opcodes()

    def pair(x0, x1):
        # x0:x1 are 0-based slice indices; convert to 1-based line indices.
        x0 += 1
        if x0 >= x1:
            return "line " + str(x0)
        else:
            return "lines %d-%d" % (x0, x1)

    for op, a0, a1, b0, b1 in tuples:
        if op == 'equal':
            pass

        elif op == 'delete':
            print "***", pair(a0, a1), "of expected output missing:"
            for line in a[a0:a1]:
                print "-", line,

        elif op == 'replace':
            print "*** mismatch between", pair(a0, a1), "of expected", \
                  "output and", pair(b0, b1), "of actual output:"
            for line in difflib.ndiff(a[a0:a1], b[b0:b1]):
                print line,

        elif op == 'insert':
            print "***", pair(b0, b1), "of actual output doesn't appear", \
                  "in expected output after line", str(a1)+":"
            for line in b[b0:b1]:
                print "+", line,

        else:
            print "get_opcodes() returned bad tuple?!?!", (op, a0, a1, b0, b1)

    print "*" * 70

def findtestdir():
    if __name__ == '__main__':
        file = sys.argv[0]
    else:
        file = __file__
    testdir = os.path.dirname(file) or os.curdir
    return testdir

def removepy(name):
    if name.endswith(os.extsep + "py"):
        name = name[:-3]
    return name

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
    print fill(' '.join(map(str, x)), width,
               initial_indent=blanks, subsequent_indent=blanks)

# Map sys.platform to a string containing the basenames of tests
# expected to be skipped on that platform.
#
# Special cases:
#     test_pep277
#         The _ExpectedSkips constructor adds this to the set of expected
#         skips if not os.path.supports_unicode_filenames.
#     test_normalization
#         Whether a skip is expected here depends on whether a large test
#         input file has been downloaded.  test_normalization.skip_expected
#         controls that.
#     test_socket_ssl
#         Controlled by test_socket_ssl.skip_expected.  Requires the network
#         resource, and a socket module with ssl support.
#     test_timeout
#         Controlled by test_timeout.skip_expected.  Requires the network
#         resource and a socket module.
#     test_codecmaps_*
#         Whether a skip is expected here depends on whether a large test
#         input file has been downloaded.  test_codecmaps_*.skip_expected
#         controls that.

_expectations = {
    'win32':
        """
        test__locale
        test_applesingle
        test_al
        test_bsddb185
        test_bsddb3
        test_cd
        test_cl
        test_commands
        test_crypt
        test_curses
        test_dbm
        test_dl
        test_fcntl
        test_fork1
        test_gdbm
        test_gl
        test_grp
        test_imgfile
        test_ioctl
        test_largefile
        test_linuxaudiodev
        test_mhlib
        test_nis
        test_openpty
        test_ossaudiodev
        test_poll
        test_posix
        test_pty
        test_pwd
        test_resource
        test_signal
        test_sunaudiodev
        test_threadsignals
        test_timing
        """,
    'linux2':
        """
        test_al
        test_applesingle
        test_bsddb185
        test_cd
        test_cl
        test_curses
        test_dl
        test_gl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_nis
        test_ntpath
        test_ossaudiodev
        test_sunaudiodev
        """,
   'mac':
        """
        test_al
        test_atexit
        test_bsddb
        test_bsddb185
        test_bsddb3
        test_bz2
        test_cd
        test_cl
        test_commands
        test_crypt
        test_curses
        test_dbm
        test_dl
        test_fcntl
        test_fork1
        test_gl
        test_grp
        test_ioctl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_locale
        test_mmap
        test_nis
        test_ntpath
        test_openpty
        test_ossaudiodev
        test_poll
        test_popen
        test_popen2
        test_posix
        test_pty
        test_pwd
        test_resource
        test_signal
        test_sunaudiodev
        test_sundry
        test_tarfile
        test_timing
        """,
    'unixware7':
        """
        test_al
        test_applesingle
        test_bsddb
        test_bsddb185
        test_cd
        test_cl
        test_dl
        test_gl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_minidom
        test_nis
        test_ntpath
        test_openpty
        test_pyexpat
        test_sax
        test_sunaudiodev
        test_sundry
        """,
    'openunix8':
        """
        test_al
        test_applesingle
        test_bsddb
        test_bsddb185
        test_cd
        test_cl
        test_dl
        test_gl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_minidom
        test_nis
        test_ntpath
        test_openpty
        test_pyexpat
        test_sax
        test_sunaudiodev
        test_sundry
        """,
    'sco_sv3':
        """
        test_al
        test_applesingle
        test_asynchat
        test_bsddb
        test_bsddb185
        test_cd
        test_cl
        test_dl
        test_fork1
        test_gettext
        test_gl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_locale
        test_minidom
        test_nis
        test_ntpath
        test_openpty
        test_pyexpat
        test_queue
        test_sax
        test_sunaudiodev
        test_sundry
        test_thread
        test_threaded_import
        test_threadedtempfile
        test_threading
        """,
    'riscos':
        """
        test_al
        test_applesingle
        test_asynchat
        test_atexit
        test_bsddb
        test_bsddb185
        test_bsddb3
        test_cd
        test_cl
        test_commands
        test_crypt
        test_dbm
        test_dl
        test_fcntl
        test_fork1
        test_gdbm
        test_gl
        test_grp
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_locale
        test_mmap
        test_nis
        test_ntpath
        test_openpty
        test_poll
        test_popen2
        test_pty
        test_pwd
        test_strop
        test_sunaudiodev
        test_sundry
        test_thread
        test_threaded_import
        test_threadedtempfile
        test_threading
        test_timing
        """,
    'darwin':
        """
        test__locale
        test_al
        test_bsddb
        test_bsddb3
        test_cd
        test_cl
        test_curses
        test_dl
        test_gdbm
        test_gl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_locale
        test_minidom
        test_nis
        test_ntpath
        test_ossaudiodev
        test_poll
        test_sunaudiodev
        """,
    'sunos5':
        """
        test_al
        test_applesingle
        test_bsddb
        test_bsddb185
        test_cd
        test_cl
        test_curses
        test_dbm
        test_gdbm
        test_gl
        test_gzip
        test_imgfile
        test_linuxaudiodev
        test_openpty
        test_zipfile
        test_zlib
        """,
    'hp-ux11':
        """
        test_al
        test_applesingle
        test_bsddb
        test_bsddb185
        test_cd
        test_cl
        test_curses
        test_dl
        test_gdbm
        test_gl
        test_gzip
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_locale
        test_minidom
        test_nis
        test_ntpath
        test_openpty
        test_pyexpat
        test_sax
        test_sunaudiodev
        test_zipfile
        test_zlib
        """,
    'atheos':
        """
        test_al
        test_applesingle
        test_bsddb185
        test_cd
        test_cl
        test_curses
        test_dl
        test_gdbm
        test_gl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_locale
        test_mhlib
        test_mmap
        test_nis
        test_poll
        test_popen2
        test_resource
        test_sunaudiodev
        """,
    'cygwin':
        """
        test_al
        test_applesingle
        test_bsddb185
        test_bsddb3
        test_cd
        test_cl
        test_curses
        test_dbm
        test_gl
        test_imgfile
        test_ioctl
        test_largefile
        test_linuxaudiodev
        test_locale
        test_nis
        test_ossaudiodev
        test_socketserver
        test_sunaudiodev
        """,
    'os2emx':
        """
        test_al
        test_applesingle
        test_audioop
        test_bsddb185
        test_bsddb3
        test_cd
        test_cl
        test_commands
        test_curses
        test_dl
        test_gl
        test_imgfile
        test_largefile
        test_linuxaudiodev
        test_mhlib
        test_mmap
        test_nis
        test_openpty
        test_ossaudiodev
        test_pty
        test_resource
        test_signal
        test_sunaudiodev
        """,
    'freebsd4':
        """
        test_aepack
        test_al
        test_applesingle
        test_bsddb
        test_bsddb3
        test_cd
        test_cl
        test_gdbm
        test_gl
        test_imgfile
        test_linuxaudiodev
        test_locale
        test_macfs
        test_macostools
        test_nis
        test_normalization
        test_ossaudiodev
        test_pep277
        test_plistlib
        test_pty
        test_scriptpackages
        test_socket_ssl
        test_socketserver
        test_sunaudiodev
        test_tcl
        test_timeout
        test_unicode_file
        test_urllibnet
        test_winreg
        test_winsound
        """,
}
_expectations['freebsd5'] = _expectations['freebsd4']
_expectations['freebsd6'] = _expectations['freebsd4']

class _ExpectedSkips:
    def __init__(self):
        import os.path
        from test import test_normalization
        from test import test_socket_ssl
        from test import test_timeout
        from test import test_codecmaps_cn, test_codecmaps_jp
        from test import test_codecmaps_kr, test_codecmaps_tw
        from test import test_codecmaps_hk

        self.valid = False
        if sys.platform in _expectations:
            s = _expectations[sys.platform]
            self.expected = set(s.split())

            if not os.path.supports_unicode_filenames:
                self.expected.add('test_pep277')

            if test_normalization.skip_expected:
                self.expected.add('test_normalization')

            if test_socket_ssl.skip_expected:
                self.expected.add('test_socket_ssl')

            if test_timeout.skip_expected:
                self.expected.add('test_timeout')

            for cc in ('cn', 'jp', 'kr', 'tw', 'hk'):
                if eval('test_codecmaps_' + cc).skip_expected:
                    self.expected.add('test_codecmaps_' + cc)

            if sys.maxint == 9223372036854775807L:
                self.expected.add('test_rgbimg')
                self.expected.add('test_imageop')

            if not sys.platform in ("mac", "darwin"):
                MAC_ONLY = ["test_macostools", "test_macfs", "test_aepack",
                            "test_plistlib", "test_scriptpackages"]
                for skip in MAC_ONLY:
                    self.expected.add(skip)

            if sys.platform != "win32":
                WIN_ONLY = ["test_unicode_file", "test_winreg",
                            "test_winsound"]
                for skip in WIN_ONLY:
                    self.expected.add(skip)

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

if __name__ == '__main__':
    # Remove regrtest.py's own directory from the module search path.  This
    # prevents relative imports from working, and relative imports will screw
    # up the testing framework.  E.g. if both test.test_support and
    # test_support are imported, they will not contain the same globals, and
    # much of the testing framework relies on the globals in the
    # test.test_support module.
    mydir = os.path.abspath(os.path.normpath(os.path.dirname(sys.argv[0])))
    i = pathlen = len(sys.path)
    while i >= 0:
        i -= 1
        if os.path.abspath(os.path.normpath(sys.path[i])) == mydir:
            del sys.path[i]
    if len(sys.path) == pathlen:
        print 'Could not find %r in sys.path to remove it' % mydir
    main()
