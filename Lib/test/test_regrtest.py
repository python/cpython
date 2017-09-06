"""
Tests of regrtest.py.

Note: test_regrtest cannot be run twice in parallel.
"""

import contextlib
import faulthandler
import io
import os.path
import platform
import re
import subprocess
import sys
import sysconfig
import tempfile
import textwrap
import unittest
from test import libregrtest
from test import support


Py_DEBUG = hasattr(sys, 'getobjects')
ROOT_DIR = os.path.join(os.path.dirname(__file__), '..', '..')
ROOT_DIR = os.path.abspath(os.path.normpath(ROOT_DIR))

TEST_INTERRUPTED = textwrap.dedent("""
    from signal import SIGINT
    try:
        from _testcapi import raise_signal
        raise_signal(SIGINT)
    except ImportError:
        import os
        os.kill(os.getpid(), SIGINT)
    """)


class ParseArgsTestCase(unittest.TestCase):
    """
    Test regrtest's argument parsing, function _parse_args().
    """

    def checkError(self, args, msg):
        with support.captured_stderr() as err, self.assertRaises(SystemExit):
            libregrtest._parse_args(args)
        self.assertIn(msg, err.getvalue())

    def test_help(self):
        for opt in '-h', '--help':
            with self.subTest(opt=opt):
                with support.captured_stdout() as out, \
                     self.assertRaises(SystemExit):
                    libregrtest._parse_args([opt])
                self.assertIn('Run Python regression tests.', out.getvalue())

    @unittest.skipUnless(hasattr(faulthandler, 'dump_traceback_later'),
                         "faulthandler.dump_traceback_later() required")
    def test_timeout(self):
        ns = libregrtest._parse_args(['--timeout', '4.2'])
        self.assertEqual(ns.timeout, 4.2)
        self.checkError(['--timeout'], 'expected one argument')
        self.checkError(['--timeout', 'foo'], 'invalid float value')

    def test_wait(self):
        ns = libregrtest._parse_args(['--wait'])
        self.assertTrue(ns.wait)

    def test_slaveargs(self):
        ns = libregrtest._parse_args(['--slaveargs', '[[], {}]'])
        self.assertEqual(ns.slaveargs, '[[], {}]')
        self.checkError(['--slaveargs'], 'expected one argument')

    def test_start(self):
        for opt in '-S', '--start':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, 'foo'])
                self.assertEqual(ns.start, 'foo')
                self.checkError([opt], 'expected one argument')

    def test_verbose(self):
        ns = libregrtest._parse_args(['-v'])
        self.assertEqual(ns.verbose, 1)
        ns = libregrtest._parse_args(['-vvv'])
        self.assertEqual(ns.verbose, 3)
        ns = libregrtest._parse_args(['--verbose'])
        self.assertEqual(ns.verbose, 1)
        ns = libregrtest._parse_args(['--verbose'] * 3)
        self.assertEqual(ns.verbose, 3)
        ns = libregrtest._parse_args([])
        self.assertEqual(ns.verbose, 0)

    def test_verbose2(self):
        for opt in '-w', '--verbose2':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.verbose2)

    def test_verbose3(self):
        for opt in '-W', '--verbose3':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.verbose3)

    def test_quiet(self):
        for opt in '-q', '--quiet':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.quiet)
                self.assertEqual(ns.verbose, 0)

    def test_slow(self):
        for opt in '-o', '--slowest':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.print_slow)

    def test_header(self):
        ns = libregrtest._parse_args(['--header'])
        self.assertTrue(ns.header)

        ns = libregrtest._parse_args(['--verbose'])
        self.assertTrue(ns.header)

    def test_randomize(self):
        for opt in '-r', '--randomize':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.randomize)

    def test_randseed(self):
        ns = libregrtest._parse_args(['--randseed', '12345'])
        self.assertEqual(ns.random_seed, 12345)
        self.assertTrue(ns.randomize)
        self.checkError(['--randseed'], 'expected one argument')
        self.checkError(['--randseed', 'foo'], 'invalid int value')

    def test_fromfile(self):
        for opt in '-f', '--fromfile':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, 'foo'])
                self.assertEqual(ns.fromfile, 'foo')
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo', '-s'], "don't go together")

    def test_exclude(self):
        for opt in '-x', '--exclude':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.exclude)

    def test_single(self):
        for opt in '-s', '--single':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.single)
                self.checkError([opt, '-f', 'foo'], "don't go together")

    def test_match(self):
        for opt in '-m', '--match':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, 'pattern'])
                self.assertEqual(ns.match_tests, ['pattern'])
                self.checkError([opt], 'expected one argument')

        ns = libregrtest._parse_args(['-m', 'pattern1',
                                      '-m', 'pattern2'])
        self.assertEqual(ns.match_tests, ['pattern1', 'pattern2'])

        self.addCleanup(support.unlink, support.TESTFN)
        with open(support.TESTFN, "w") as fp:
            print('matchfile1', file=fp)
            print('matchfile2', file=fp)

        filename = os.path.abspath(support.TESTFN)
        ns = libregrtest._parse_args(['-m', 'match',
                                      '--matchfile', filename])
        self.assertEqual(ns.match_tests,
                         ['match', 'matchfile1', 'matchfile2'])

    def test_failfast(self):
        for opt in '-G', '--failfast':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, '-v'])
                self.assertTrue(ns.failfast)
                ns = libregrtest._parse_args([opt, '-W'])
                self.assertTrue(ns.failfast)
                self.checkError([opt], '-G/--failfast needs either -v or -W')

    def test_use(self):
        for opt in '-u', '--use':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, 'gui,network'])
                self.assertEqual(ns.use_resources, ['gui', 'network'])

                ns = libregrtest._parse_args([opt, 'gui,none,network'])
                self.assertEqual(ns.use_resources, ['network'])

                expected = list(libregrtest.ALL_RESOURCES)
                expected.remove('gui')
                ns = libregrtest._parse_args([opt, 'all,-gui'])
                self.assertEqual(ns.use_resources, expected)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid resource')

                # all + a resource not part of "all"
                ns = libregrtest._parse_args([opt, 'all,tzdata'])
                self.assertEqual(ns.use_resources,
                                 list(libregrtest.ALL_RESOURCES) + ['tzdata'])

                # test another resource which is not part of "all"
                ns = libregrtest._parse_args([opt, 'extralargefile'])
                self.assertEqual(ns.use_resources, ['extralargefile'])

    def test_memlimit(self):
        for opt in '-M', '--memlimit':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, '4G'])
                self.assertEqual(ns.memlimit, '4G')
                self.checkError([opt], 'expected one argument')

    def test_testdir(self):
        ns = libregrtest._parse_args(['--testdir', 'foo'])
        self.assertEqual(ns.testdir, os.path.join(support.SAVEDCWD, 'foo'))
        self.checkError(['--testdir'], 'expected one argument')

    def test_runleaks(self):
        for opt in '-L', '--runleaks':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.runleaks)

    def test_huntrleaks(self):
        for opt in '-R', '--huntrleaks':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, ':'])
                self.assertEqual(ns.huntrleaks, (5, 4, 'reflog.txt'))
                ns = libregrtest._parse_args([opt, '6:'])
                self.assertEqual(ns.huntrleaks, (6, 4, 'reflog.txt'))
                ns = libregrtest._parse_args([opt, ':3'])
                self.assertEqual(ns.huntrleaks, (5, 3, 'reflog.txt'))
                ns = libregrtest._parse_args([opt, '6:3:leaks.log'])
                self.assertEqual(ns.huntrleaks, (6, 3, 'leaks.log'))
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, '6'],
                                'needs 2 or 3 colon-separated arguments')
                self.checkError([opt, 'foo:'], 'invalid huntrleaks value')
                self.checkError([opt, '6:foo'], 'invalid huntrleaks value')

    def test_multiprocess(self):
        for opt in '-j', '--multiprocess':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, '2'])
                self.assertEqual(ns.use_mp, 2)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid int value')
                self.checkError([opt, '2', '-T'], "don't go together")
                self.checkError([opt, '2', '-l'], "don't go together")
                self.checkError([opt, '0', '-T'], "don't go together")
                self.checkError([opt, '0', '-l'], "don't go together")

    def test_coverage(self):
        for opt in '-T', '--coverage':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.trace)

    def test_coverdir(self):
        for opt in '-D', '--coverdir':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, 'foo'])
                self.assertEqual(ns.coverdir,
                                 os.path.join(support.SAVEDCWD, 'foo'))
                self.checkError([opt], 'expected one argument')

    def test_nocoverdir(self):
        for opt in '-N', '--nocoverdir':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertIsNone(ns.coverdir)

    def test_threshold(self):
        for opt in '-t', '--threshold':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt, '1000'])
                self.assertEqual(ns.threshold, 1000)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid int value')

    def test_nowindows(self):
        for opt in '-n', '--nowindows':
            with self.subTest(opt=opt):
                with contextlib.redirect_stderr(io.StringIO()) as stderr:
                    ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.nowindows)
                err = stderr.getvalue()
                self.assertIn('the --nowindows (-n) option is deprecated', err)

    def test_forever(self):
        for opt in '-F', '--forever':
            with self.subTest(opt=opt):
                ns = libregrtest._parse_args([opt])
                self.assertTrue(ns.forever)

    def test_unrecognized_argument(self):
        self.checkError(['--xxx'], 'usage:')

    def test_long_option__partial(self):
        ns = libregrtest._parse_args(['--qui'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)

    def test_two_options(self):
        ns = libregrtest._parse_args(['--quiet', '--exclude'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)
        self.assertTrue(ns.exclude)

    def test_option_with_empty_string_value(self):
        ns = libregrtest._parse_args(['--start', ''])
        self.assertEqual(ns.start, '')

    def test_arg(self):
        ns = libregrtest._parse_args(['foo'])
        self.assertEqual(ns.args, ['foo'])

    def test_option_and_arg(self):
        ns = libregrtest._parse_args(['--quiet', 'foo'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)
        self.assertEqual(ns.args, ['foo'])

    def test_arg_option_arg(self):
        ns = libregrtest._parse_args(['test_unaryop', '-v', 'test_binop'])
        self.assertEqual(ns.verbose, 1)
        self.assertEqual(ns.args, ['test_unaryop', 'test_binop'])

    def test_unknown_option(self):
        self.checkError(['--unknown-option'],
                        'unrecognized arguments: --unknown-option')


class BaseTestCase(unittest.TestCase):
    TEST_UNIQUE_ID = 1
    TESTNAME_PREFIX = 'test_regrtest_'
    TESTNAME_REGEX = r'test_[a-zA-Z0-9_]+'

    def setUp(self):
        self.testdir = os.path.realpath(os.path.dirname(__file__))

        self.tmptestdir = tempfile.mkdtemp()
        self.addCleanup(support.rmtree, self.tmptestdir)

    def create_test(self, name=None, code=''):
        if not name:
            name = 'noop%s' % BaseTestCase.TEST_UNIQUE_ID
            BaseTestCase.TEST_UNIQUE_ID += 1

        # test_regrtest cannot be run twice in parallel because
        # of setUp() and create_test()
        name = self.TESTNAME_PREFIX + name
        path = os.path.join(self.tmptestdir, name + '.py')

        self.addCleanup(support.unlink, path)
        # Use 'x' mode to ensure that we do not override existing tests
        try:
            with open(path, 'x', encoding='utf-8') as fp:
                fp.write(code)
        except PermissionError as exc:
            if not sysconfig.is_python_build():
                self.skipTest("cannot write %s: %s" % (path, exc))
            raise
        return name

    def regex_search(self, regex, output):
        match = re.search(regex, output, re.MULTILINE)
        if not match:
            self.fail("%r not found in %r" % (regex, output))
        return match

    def check_line(self, output, regex):
        regex = re.compile(r'^' + regex, re.MULTILINE)
        self.assertRegex(output, regex)

    def parse_executed_tests(self, output):
        regex = (r'^[0-9]+:[0-9]+:[0-9]+ (?:load avg: [0-9]+\.[0-9]{2} )?\[ *[0-9]+(?:/ *[0-9]+)*\] (%s)'
                 % self.TESTNAME_REGEX)
        parser = re.finditer(regex, output, re.MULTILINE)
        return list(match.group(1) for match in parser)

    def check_executed_tests(self, output, tests, skipped=(), failed=(),
                             env_changed=(), omitted=(),
                             randomize=False, interrupted=False,
                             fail_env_changed=False):
        if isinstance(tests, str):
            tests = [tests]
        if isinstance(skipped, str):
            skipped = [skipped]
        if isinstance(failed, str):
            failed = [failed]
        if isinstance(env_changed, str):
            env_changed = [env_changed]
        if isinstance(omitted, str):
            omitted = [omitted]

        executed = self.parse_executed_tests(output)
        if randomize:
            self.assertEqual(set(executed), set(tests), output)
        else:
            self.assertEqual(executed, tests, output)

        def plural(count):
            return 's' if count != 1 else ''

        def list_regex(line_format, tests):
            count = len(tests)
            names = ' '.join(sorted(tests))
            regex = line_format % (count, plural(count))
            regex = r'%s:\n    %s$' % (regex, names)
            return regex

        if skipped:
            regex = list_regex('%s test%s skipped', skipped)
            self.check_line(output, regex)

        if failed:
            regex = list_regex('%s test%s failed', failed)
            self.check_line(output, regex)

        if env_changed:
            regex = list_regex('%s test%s altered the execution environment',
                               env_changed)
            self.check_line(output, regex)

        if omitted:
            regex = list_regex('%s test%s omitted', omitted)
            self.check_line(output, regex)

        good = (len(tests) - len(skipped) - len(failed)
                - len(omitted) - len(env_changed))
        if good:
            regex = r'%s test%s OK\.$' % (good, plural(good))
            if not skipped and not failed and good > 1:
                regex = 'All %s' % regex
            self.check_line(output, regex)

        if interrupted:
            self.check_line(output, 'Test suite interrupted by signal SIGINT.')

        if failed:
            result = 'FAILURE'
        elif interrupted:
            result = 'INTERRUPTED'
        elif fail_env_changed and env_changed:
            result = 'ENV CHANGED'
        else:
            result = 'SUCCESS'
        self.check_line(output, 'Tests result: %s' % result)

    def parse_random_seed(self, output):
        match = self.regex_search(r'Using random seed ([0-9]+)', output)
        randseed = int(match.group(1))
        self.assertTrue(0 <= randseed <= 10000000, randseed)
        return randseed

    def run_command(self, args, input=None, exitcode=0, **kw):
        if not input:
            input = ''
        if 'stderr' not in kw:
            kw['stderr'] = subprocess.PIPE
        proc = subprocess.run(args,
                              universal_newlines=True,
                              input=input,
                              stdout=subprocess.PIPE,
                              **kw)
        if proc.returncode != exitcode:
            msg = ("Command %s failed with exit code %s\n"
                   "\n"
                   "stdout:\n"
                   "---\n"
                   "%s\n"
                   "---\n"
                   % (str(args), proc.returncode, proc.stdout))
            if proc.stderr:
                msg += ("\n"
                        "stderr:\n"
                        "---\n"
                        "%s"
                        "---\n"
                        % proc.stderr)
            self.fail(msg)
        return proc

    def run_python(self, args, **kw):
        args = [sys.executable, '-X', 'faulthandler', '-I', *args]
        proc = self.run_command(args, **kw)
        return proc.stdout


class ProgramsTestCase(BaseTestCase):
    """
    Test various ways to run the Python test suite. Use options close
    to options used on the buildbot.
    """

    NTEST = 4

    def setUp(self):
        super().setUp()

        # Create NTEST tests doing nothing
        self.tests = [self.create_test() for index in range(self.NTEST)]

        self.python_args = ['-Wd', '-E', '-bb']
        self.regrtest_args = ['-uall', '-rwW',
                              '--testdir=%s' % self.tmptestdir]
        if hasattr(faulthandler, 'dump_traceback_later'):
            self.regrtest_args.extend(('--timeout', '3600', '-j4'))
        if sys.platform == 'win32':
            self.regrtest_args.append('-n')

    def check_output(self, output):
        self.parse_random_seed(output)
        self.check_executed_tests(output, self.tests, randomize=True)

    def run_tests(self, args):
        output = self.run_python(args)
        self.check_output(output)

    def test_script_regrtest(self):
        # Lib/test/regrtest.py
        script = os.path.join(self.testdir, 'regrtest.py')

        args = [*self.python_args, script, *self.regrtest_args, *self.tests]
        self.run_tests(args)

    def test_module_test(self):
        # -m test
        args = [*self.python_args, '-m', 'test',
                *self.regrtest_args, *self.tests]
        self.run_tests(args)

    def test_module_regrtest(self):
        # -m test.regrtest
        args = [*self.python_args, '-m', 'test.regrtest',
                *self.regrtest_args, *self.tests]
        self.run_tests(args)

    def test_module_autotest(self):
        # -m test.autotest
        args = [*self.python_args, '-m', 'test.autotest',
                *self.regrtest_args, *self.tests]
        self.run_tests(args)

    def test_module_from_test_autotest(self):
        # from test import autotest
        code = 'from test import autotest'
        args = [*self.python_args, '-c', code,
                *self.regrtest_args, *self.tests]
        self.run_tests(args)

    def test_script_autotest(self):
        # Lib/test/autotest.py
        script = os.path.join(self.testdir, 'autotest.py')
        args = [*self.python_args, script, *self.regrtest_args, *self.tests]
        self.run_tests(args)

    @unittest.skipUnless(sysconfig.is_python_build(),
                         'run_tests.py script is not installed')
    def test_tools_script_run_tests(self):
        # Tools/scripts/run_tests.py
        script = os.path.join(ROOT_DIR, 'Tools', 'scripts', 'run_tests.py')
        args = [script, *self.regrtest_args, *self.tests]
        self.run_tests(args)

    def run_batch(self, *args):
        proc = self.run_command(args)
        self.check_output(proc.stdout)

    @unittest.skipUnless(sysconfig.is_python_build(),
                         'test.bat script is not installed')
    @unittest.skipUnless(sys.platform == 'win32', 'Windows only')
    def test_tools_buildbot_test(self):
        # Tools\buildbot\test.bat
        script = os.path.join(ROOT_DIR, 'Tools', 'buildbot', 'test.bat')
        test_args = ['--testdir=%s' % self.tmptestdir]
        if platform.architecture()[0] == '64bit':
            test_args.append('-x64')   # 64-bit build
        if not Py_DEBUG:
            test_args.append('+d')     # Release build, use python.exe
        self.run_batch(script, *test_args, *self.tests)

    @unittest.skipUnless(sys.platform == 'win32', 'Windows only')
    def test_pcbuild_rt(self):
        # PCbuild\rt.bat
        script = os.path.join(ROOT_DIR, r'PCbuild\rt.bat')
        rt_args = ["-q"]             # Quick, don't run tests twice
        if platform.architecture()[0] == '64bit':
            rt_args.append('-x64')   # 64-bit build
        if Py_DEBUG:
            rt_args.append('-d')     # Debug build, use python_d.exe
        self.run_batch(script, *rt_args, *self.regrtest_args, *self.tests)


class ArgsTestCase(BaseTestCase):
    """
    Test arguments of the Python test suite.
    """

    def run_tests(self, *testargs, **kw):
        cmdargs = ['-m', 'test', '--testdir=%s' % self.tmptestdir, *testargs]
        return self.run_python(cmdargs, **kw)

    def test_failing_test(self):
        # test a failing test
        code = textwrap.dedent("""
            import unittest

            class FailingTest(unittest.TestCase):
                def test_failing(self):
                    self.fail("bug")
        """)
        test_ok = self.create_test('ok')
        test_failing = self.create_test('failing', code=code)
        tests = [test_ok, test_failing]

        output = self.run_tests(*tests, exitcode=2)
        self.check_executed_tests(output, tests, failed=test_failing)

    def test_resources(self):
        # test -u command line option
        tests = {}
        for resource in ('audio', 'network'):
            code = 'from test import support\nsupport.requires(%r)' % resource
            tests[resource] = self.create_test(resource, code)
        test_names = sorted(tests.values())

        # -u all: 2 resources enabled
        output = self.run_tests('-u', 'all', *test_names)
        self.check_executed_tests(output, test_names)

        # -u audio: 1 resource enabled
        output = self.run_tests('-uaudio', *test_names)
        self.check_executed_tests(output, test_names,
                                  skipped=tests['network'])

        # no option: 0 resources enabled
        output = self.run_tests(*test_names)
        self.check_executed_tests(output, test_names,
                                  skipped=test_names)

    def test_random(self):
        # test -r and --randseed command line option
        code = textwrap.dedent("""
            import random
            print("TESTRANDOM: %s" % random.randint(1, 1000))
        """)
        test = self.create_test('random', code)

        # first run to get the output with the random seed
        output = self.run_tests('-r', test)
        randseed = self.parse_random_seed(output)
        match = self.regex_search(r'TESTRANDOM: ([0-9]+)', output)
        test_random = int(match.group(1))

        # try to reproduce with the random seed
        output = self.run_tests('-r', '--randseed=%s' % randseed, test)
        randseed2 = self.parse_random_seed(output)
        self.assertEqual(randseed2, randseed)

        match = self.regex_search(r'TESTRANDOM: ([0-9]+)', output)
        test_random2 = int(match.group(1))
        self.assertEqual(test_random2, test_random)

    def test_fromfile(self):
        # test --fromfile
        tests = [self.create_test() for index in range(5)]

        # Write the list of files using a format similar to regrtest output:
        # [1/2] test_1
        # [2/2] test_2
        filename = support.TESTFN
        self.addCleanup(support.unlink, filename)

        # test format '0:00:00 [2/7] test_opcodes -- test_grammar took 0 sec'
        with open(filename, "w") as fp:
            previous = None
            for index, name in enumerate(tests, 1):
                line = ("00:00:%02i [%s/%s] %s"
                        % (index, index, len(tests), name))
                if previous:
                    line += " -- %s took 0 sec" % previous
                print(line, file=fp)
                previous = name

        output = self.run_tests('--fromfile', filename)
        self.check_executed_tests(output, tests)

        # test format '[2/7] test_opcodes'
        with open(filename, "w") as fp:
            for index, name in enumerate(tests, 1):
                print("[%s/%s] %s" % (index, len(tests), name), file=fp)

        output = self.run_tests('--fromfile', filename)
        self.check_executed_tests(output, tests)

        # test format 'test_opcodes'
        with open(filename, "w") as fp:
            for name in tests:
                print(name, file=fp)

        output = self.run_tests('--fromfile', filename)
        self.check_executed_tests(output, tests)

        # test format 'Lib/test/test_opcodes.py'
        with open(filename, "w") as fp:
            for name in tests:
                print('Lib/test/%s.py' % name, file=fp)

        output = self.run_tests('--fromfile', filename)
        self.check_executed_tests(output, tests)

    def test_interrupted(self):
        code = TEST_INTERRUPTED
        test = self.create_test('sigint', code=code)
        output = self.run_tests(test, exitcode=130)
        self.check_executed_tests(output, test, omitted=test,
                                  interrupted=True)

    def test_slowest(self):
        # test --slowest
        tests = [self.create_test() for index in range(3)]
        output = self.run_tests("--slowest", *tests)
        self.check_executed_tests(output, tests)
        regex = ('10 slowest tests:\n'
                 '(?:- %s: .*\n){%s}'
                 % (self.TESTNAME_REGEX, len(tests)))
        self.check_line(output, regex)

    def test_slow_interrupted(self):
        # Issue #25373: test --slowest with an interrupted test
        code = TEST_INTERRUPTED
        test = self.create_test("sigint", code=code)

        try:
            import threading
            tests = (False, True)
        except ImportError:
            tests = (False,)
        for multiprocessing in tests:
            if multiprocessing:
                args = ("--slowest", "-j2", test)
            else:
                args = ("--slowest", test)
            output = self.run_tests(*args, exitcode=130)
            self.check_executed_tests(output, test,
                                      omitted=test, interrupted=True)

            regex = ('10 slowest tests:\n')
            self.check_line(output, regex)

    def test_coverage(self):
        # test --coverage
        test = self.create_test('coverage')
        output = self.run_tests("--coverage", test)
        self.check_executed_tests(output, [test])
        regex = (r'lines +cov% +module +\(path\)\n'
                 r'(?: *[0-9]+ *[0-9]{1,2}% *[^ ]+ +\([^)]+\)+)+')
        self.check_line(output, regex)

    def test_wait(self):
        # test --wait
        test = self.create_test('wait')
        output = self.run_tests("--wait", test, input='key')
        self.check_line(output, 'Press any key to continue')

    def test_forever(self):
        # test --forever
        code = textwrap.dedent("""
            import builtins
            import unittest

            class ForeverTester(unittest.TestCase):
                def test_run(self):
                    # Store the state in the builtins module, because the test
                    # module is reload at each run
                    if 'RUN' in builtins.__dict__:
                        builtins.__dict__['RUN'] += 1
                        if builtins.__dict__['RUN'] >= 3:
                            self.fail("fail at the 3rd runs")
                    else:
                        builtins.__dict__['RUN'] = 1
        """)
        test = self.create_test('forever', code=code)
        output = self.run_tests('--forever', test, exitcode=2)
        self.check_executed_tests(output, [test]*3, failed=test)

    def check_leak(self, code, what):
        test = self.create_test('huntrleaks', code=code)

        filename = 'reflog.txt'
        self.addCleanup(support.unlink, filename)
        output = self.run_tests('--huntrleaks', '3:3:', test,
                                exitcode=2,
                                stderr=subprocess.STDOUT)
        self.check_executed_tests(output, [test], failed=test)

        line = 'beginning 6 repetitions\n123456\n......\n'
        self.check_line(output, re.escape(line))

        line2 = '%s leaked [1, 1, 1] %s, sum=3\n' % (test, what)
        self.assertIn(line2, output)

        with open(filename) as fp:
            reflog = fp.read()
            self.assertIn(line2, reflog)

    @unittest.skipUnless(Py_DEBUG, 'need a debug build')
    def test_huntrleaks(self):
        # test --huntrleaks
        code = textwrap.dedent("""
            import unittest

            GLOBAL_LIST = []

            class RefLeakTest(unittest.TestCase):
                def test_leak(self):
                    GLOBAL_LIST.append(object())
        """)
        self.check_leak(code, 'references')

    @unittest.skipUnless(Py_DEBUG, 'need a debug build')
    def test_huntrleaks_fd_leak(self):
        # test --huntrleaks for file descriptor leak
        code = textwrap.dedent("""
            import os
            import unittest

            # Issue #25306: Disable popups and logs to stderr on assertion
            # failures in MSCRT
            try:
                import msvcrt
                msvcrt.CrtSetReportMode
            except (ImportError, AttributeError):
                # no Windows, o release build
                pass
            else:
                for m in [msvcrt.CRT_WARN, msvcrt.CRT_ERROR, msvcrt.CRT_ASSERT]:
                    msvcrt.CrtSetReportMode(m, 0)

            class FDLeakTest(unittest.TestCase):
                def test_leak(self):
                    fd = os.open(__file__, os.O_RDONLY)
                    # bug: never cloes the file descriptor
        """)
        self.check_leak(code, 'file descriptors')

    def test_list_tests(self):
        # test --list-tests
        tests = [self.create_test() for i in range(5)]
        output = self.run_tests('--list-tests', *tests)
        self.assertEqual(output.rstrip().splitlines(),
                         tests)

    def test_list_cases(self):
        # test --list-cases
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_method1(self):
                    pass
                def test_method2(self):
                    pass
        """)
        testname = self.create_test(code=code)

        # Test --list-cases
        all_methods = ['%s.Tests.test_method1' % testname,
                       '%s.Tests.test_method2' % testname]
        output = self.run_tests('--list-cases', testname)
        self.assertEqual(output.splitlines(), all_methods)

        # Test --list-cases with --match
        all_methods = ['%s.Tests.test_method1' % testname]
        output = self.run_tests('--list-cases',
                                '-m', 'test_method1',
                                testname)
        self.assertEqual(output.splitlines(), all_methods)

    def test_crashed(self):
        # Any code which causes a crash
        code = 'import faulthandler; faulthandler._sigsegv()'
        crash_test = self.create_test(name="crash", code=code)
        ok_test = self.create_test(name="ok")

        tests = [crash_test, ok_test]
        output = self.run_tests("-j2", *tests, exitcode=2)
        self.check_executed_tests(output, tests, failed=crash_test,
                                  randomize=True)

    def parse_methods(self, output):
        regex = re.compile("^(test[^ ]+).*ok$", flags=re.MULTILINE)
        return [match.group(1) for match in regex.finditer(output)]

    def test_matchfile(self):
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_method1(self):
                    pass
                def test_method2(self):
                    pass
                def test_method3(self):
                    pass
                def test_method4(self):
                    pass
        """)
        all_methods = ['test_method1', 'test_method2',
                       'test_method3', 'test_method4']
        testname = self.create_test(code=code)

        # by default, all methods should be run
        output = self.run_tests("-v", testname)
        methods = self.parse_methods(output)
        self.assertEqual(methods, all_methods)

        # only run a subset
        filename = support.TESTFN
        self.addCleanup(support.unlink, filename)

        subset = [
            # only match the method name
            'test_method1',
            # match the full identifier
            '%s.Tests.test_method3' % testname]
        with open(filename, "w") as fp:
            for name in subset:
                print(name, file=fp)

        output = self.run_tests("-v", "--matchfile", filename, testname)
        methods = self.parse_methods(output)
        subset = ['test_method1', 'test_method3']
        self.assertEqual(methods, subset)

    def test_env_changed(self):
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_env_changed(self):
                    open("env_changed", "w").close()
        """)
        testname = self.create_test(code=code)

        # don't fail by default
        output = self.run_tests(testname)
        self.check_executed_tests(output, [testname], env_changed=testname)

        # fail with --fail-env-changed
        output = self.run_tests("--fail-env-changed", testname, exitcode=3)
        self.check_executed_tests(output, [testname], env_changed=testname,
                                  fail_env_changed=True)


if __name__ == '__main__':
    unittest.main()
