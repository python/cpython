#! /usr/bin/env python

"""Regression test.

This will find all modules whose name is "test_*" in the test
directory, and run them.  Various command line options provide
additional facilities.

Command line options:

-v: verbose   -- run tests in verbose mode with output to stdout
-q: quiet     -- don't print anything except if a test fails
-g: generate  -- write the output file for a test instead of comparing it
-x: exclude   -- arguments are tests to *exclude*
-s: single    -- run only a single test (see below)
-r: random    -- randomize test execution order
-l: findleaks -- if GC is available detect tests that leak memory
-u: use       -- specify which special resource intensive tests to run
-h: help      -- print this text and exit

If non-option arguments are present, they are names for tests to run,
unless -x is given, in which case they are names for tests not to run.
If no test names are given, all tests are run.

-v is incompatible with -g and does not compare test output files.

-s means to run only a single test and exit.  This is useful when doing memory
analysis on the Python interpreter (which tend to consume to many resources to
run the full regression test non-stop).  The file /tmp/pynexttest is read to
find the next test to run.  If this file is missing, the first test_*.py file
in testdir or on the command line is used.  (actually tempfile.gettempdir() is
used instead of /tmp).

-u is used to specify which special resource intensive tests to run, such as
those requiring large file support or network connectivity.  The argument is a
comma-separated list of words indicating the resources to test.  Currently
only the following are defined:

    largefile - It is okay to run some test that may create huge files.  These
                tests can take a long time and may consume >2GB of disk space
                temporarily.

    network -   It is okay to run tests that use external network resource,
                e.g. testing SSL support for sockets.
"""

import sys
import os
import getopt
import traceback
import random
import StringIO

import test_support

def usage(code, msg=''):
    print __doc__
    if msg: print msg
    sys.exit(code)


def main(tests=None, testdir=None, verbose=0, quiet=0, generate=0,
         exclude=0, single=0, randomize=0, findleaks=0,
         use_resources=None):
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
    randomize, findleaks, and use_resources) allow programmers calling main()
    directly to set the values that would normally be set by flags on the
    command line.

    """

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hvgqxsrlu:',
                                   ['help', 'verbose', 'quiet', 'generate',
                                    'exclude', 'single', 'random',
                                    'findleaks', 'use='])
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
            quiet = 1;
            verbose = 0
        elif o in ('-g', '--generate'):
            generate = 1
        elif o in ('-x', '--exclude'):
            exclude = 1
        elif o in ('-s', '--single'):
            single = 1
        elif o in ('-r', '--randomize'):
            randomize = 1
        elif o in ('-l', '--findleaks'):
            findleaks = 1
        elif o in ('-u', '--use'):
            u = [x.lower() for x in a.split(',')]
            for r in u:
                if r not in ('largefile', 'network'):
                    usage(1, 'Invalid -u/--use option: %s' % a)
            use_resources.extend(u)
    if generate and verbose:
        usage(2, "-g and -v don't go together!")

    good = []
    bad = []
    skipped = []

    if findleaks:
        try:
            import gc
        except ImportError:
            print 'No GC available, disabling findleaks.'
            findleaks = 0
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
    for i in range(len(args)):
        # Strip trailing ".py" from arguments
        if args[i][-3:] == '.py':
            args[i] = args[i][:-3]
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
    test_support.verbose = verbose      # Tell tests to be moderately quiet
    test_support.use_resources = use_resources
    save_modules = sys.modules.keys()
    for test in tests:
        if not quiet:
            print test
        ok = runtest(test, generate, verbose, quiet, testdir)
        if ok > 0:
            good.append(test)
        elif ok == 0:
            bad.append(test)
        else:
            skipped.append(test)
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
    if good and not quiet:
        if not bad and not skipped and len(good) > 1:
            print "All",
        print count(len(good), "test"), "OK."
        if verbose:
            print "CAUTION:  stdout isn't compared in verbose mode:  a test"
            print "that passes in verbose mode may fail without it."
    if bad:
        print count(len(bad), "test"), "failed:"
        printlist(bad)
    if skipped and not quiet:
        print count(len(skipped), "test"), "skipped:"
        printlist(skipped)

        e = _ExpectedSkips()
        plat = sys.platform
        if e.isvalid():
            surprise = _Set(skipped) - e.getexpected()
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
    'test_b1',
    'test_b2',
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
        if name[:5] == "test_" and name[-3:] == ".py":
            modname = name[:-3]
            if modname not in stdtests and modname not in nottests:
                tests.append(modname)
    tests.sort()
    return stdtests + tests

def runtest(test, generate, verbose, quiet, testdir = None):
    """Run a single test.
    test -- the name of the test
    generate -- if true, generate output, instead of running the test
    and comparing it to a previously created output file
    verbose -- if true, print more messages
    quiet -- if true, don't print 'skipped' messages (probably redundant)
    testdir -- test directory
    """
    test_support.unload(test)
    if not testdir: testdir = findtestdir()
    outputdir = os.path.join(testdir, "output")
    outputfile = os.path.join(outputdir, test)
    if verbose or generate:
        cfp = None
    else:
        cfp =  StringIO.StringIO()
    try:
        save_stdout = sys.stdout
        try:
            if cfp:
                sys.stdout = cfp
                print test              # Output file starts with test name
            the_module = __import__(test, globals(), locals(), [])
            # Most tests run to completion simply as a side-effect of
            # being imported.  For the benefit of tests that can't run
            # that way (like test_threaded_import), explicitly invoke
            # their test_main() function (if it exists).
            indirect_test = getattr(the_module, "test_main", None)
            if indirect_test is not None:
                indirect_test()
        finally:
            sys.stdout = save_stdout
            if cfp and test_support.output_comparison_denied():
                output = cfp.getvalue()
                cfp = None
                s = test + "\n"
                if output.startswith(s):
                    output = output[len(s):]
                sys.stdout.write(output)
    except (ImportError, test_support.TestSkipped), msg:
        if not quiet:
            print "test", test, "skipped --", msg
        return -1
    except KeyboardInterrupt:
        raise
    except test_support.TestFailed, msg:
        print "test", test, "failed --", msg
        return 0
    except:
        type, value = sys.exc_info()[:2]
        print "test", test, "crashed --", str(type) + ":", value
        if verbose:
            traceback.print_exc(file=sys.stdout)
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
        if output == expected:
            return 1
        print "test", test, "produced unexpected output:"
        reportdiff(expected, output)
        return 0

def reportdiff(expected, output):
    print "*" * 70
    import difflib
    a = expected.splitlines(1)
    b = output.splitlines(1)
    diff = difflib.ndiff(a, b)
    print ''.join(diff),
    print "*" * 70

def findtestdir():
    if __name__ == '__main__':
        file = sys.argv[0]
    else:
        file = __file__
    testdir = os.path.dirname(file) or os.curdir
    return testdir

def count(n, word):
    if n == 1:
        return "%d %s" % (n, word)
    else:
        return "%d %ss" % (n, word)

def printlist(x, width=70, indent=4):
    """Print the elements of a sequence to stdout.

    Optional arg width (default 70) is the maximum line length.
    Optional arg indent (default 4) is the number of blanks with which to
    begin each line.
    """

    line = ' ' * indent
    for one in map(str, x):
        w = len(line) + len(one)
        if line[-1:] == ' ':
            pad = ''
        else:
            pad = ' '
            w += 1
        if w > width:
            print line
            line = ' ' * indent + one
        else:
            line += pad + one
    if len(line) > indent:
        print line

class _Set:
    def __init__(self, seq=[]):
        data = self.data = {}
        for x in seq:
            data[x] = 1

    def __len__(self):
        return len(self.data)

    def __sub__(self, other):
        "Return set of all elements in self not in other."
        result = _Set()
        data = result.data = self.data.copy()
        for x in other.data:
            if x in data:
                del data[x]
        return result

    def __iter__(self):
        return iter(self.data)

    def tolist(self, sorted=1):
        "Return _Set elements as a list."
        data = self.data.keys()
        if sorted:
            data.sort()
        return data

_expectations = {
    'win32':
        """
        test_al
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
        test_mhlib
        test_nis
        test_openpty
        test_poll
        test_pty
        test_pwd
        test_signal
        test_socket_ssl
        test_socketserver
        test_sunaudiodev
        test_timing
        """,
    'linux2':
        """
        test_al
        test_cd
        test_cl
        test_dl
        test_gl
        test_imgfile
        test_largefile
        test_nis
        test_ntpath
        test_socket_ssl
        test_socketserver
        test_sunaudiodev
        test_unicode_file
        test_winreg
        test_winsound
        """,
   'mac':
        """
        test_al
        test_bsddb
        test_cd
        test_cl
        test_commands
        test_crypt
        test_dbm
        test_dl
        test_fcntl
        test_fork1
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
        test_signal
        test_socket_ssl
        test_socketserver
        test_sunaudiodev
        test_sundry
        test_timing
        test_unicode_file
        test_winreg
        test_winsound
        """,
    'unixware5':
        """
        test_al
        test_bsddb
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
        test_socketserver
        test_sunaudiodev
        test_sundry
        test_unicode_file
        test_winreg
        test_winsound
        """,
}

class _ExpectedSkips:
    def __init__(self):
        self.valid = 0
        if _expectations.has_key(sys.platform):
            s = _expectations[sys.platform]
            self.expected = _Set(s.split())
            self.valid = 1

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
    main()
