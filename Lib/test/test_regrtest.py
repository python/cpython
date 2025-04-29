"""
Tests of regrtest.py.

Note: test_regrtest cannot be run twice in parallel.
"""

import _colorize
import contextlib
import dataclasses
import glob
import io
import locale
import os.path
import platform
import random
import re
import shlex
import signal
import subprocess
import sys
import sysconfig
import tempfile
import textwrap
import unittest
import unittest.mock
from xml.etree import ElementTree

from test import support
from test.support import import_helper
from test.support import os_helper
from test.libregrtest import cmdline
from test.libregrtest import main
from test.libregrtest import setup
from test.libregrtest import utils
from test.libregrtest.filter import get_match_tests, set_match_tests, match_test
from test.libregrtest.result import TestStats
from test.libregrtest.utils import normalize_test_name

if not support.has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")

ROOT_DIR = os.path.join(os.path.dirname(__file__), '..', '..')
ROOT_DIR = os.path.abspath(os.path.normpath(ROOT_DIR))
LOG_PREFIX = r'[0-9]+:[0-9]+:[0-9]+ (?:load avg: [0-9]+\.[0-9]{2} )?'
RESULT_REGEX = (
    'passed',
    'failed',
    'skipped',
    'interrupted',
    'env changed',
    'timed out',
    'ran no tests',
    'worker non-zero exit code',
)
RESULT_REGEX = fr'(?:{"|".join(RESULT_REGEX)})'

EXITCODE_BAD_TEST = 2
EXITCODE_ENV_CHANGED = 3
EXITCODE_NO_TESTS_RAN = 4
EXITCODE_RERUN_FAIL = 5
EXITCODE_INTERRUPTED = 130

TEST_INTERRUPTED = textwrap.dedent("""
    from signal import SIGINT, raise_signal
    try:
        raise_signal(SIGINT)
    except ImportError:
        import os
        os.kill(os.getpid(), SIGINT)
    """)


class ParseArgsTestCase(unittest.TestCase):
    """
    Test regrtest's argument parsing, function _parse_args().
    """

    @staticmethod
    def parse_args(args):
        return cmdline._parse_args(args)

    def checkError(self, args, msg):
        with support.captured_stderr() as err, self.assertRaises(SystemExit):
            self.parse_args(args)
        self.assertIn(msg, err.getvalue())

    def test_help(self):
        for opt in '-h', '--help':
            with self.subTest(opt=opt):
                with support.captured_stdout() as out, \
                     self.assertRaises(SystemExit):
                    self.parse_args([opt])
                self.assertIn('Run Python regression tests.', out.getvalue())

    def test_timeout(self):
        ns = self.parse_args(['--timeout', '4.2'])
        self.assertEqual(ns.timeout, 4.2)

        # negative, zero and empty string are treated as "no timeout"
        for value in ('-1', '0', ''):
            with self.subTest(value=value):
                ns = self.parse_args([f'--timeout={value}'])
                self.assertEqual(ns.timeout, None)

        self.checkError(['--timeout'], 'expected one argument')
        self.checkError(['--timeout', 'foo'], 'invalid timeout value:')

    def test_wait(self):
        ns = self.parse_args(['--wait'])
        self.assertTrue(ns.wait)

    def test_start(self):
        for opt in '-S', '--start':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, 'foo'])
                self.assertEqual(ns.start, 'foo')
                self.checkError([opt], 'expected one argument')

    def test_verbose(self):
        ns = self.parse_args(['-v'])
        self.assertEqual(ns.verbose, 1)
        ns = self.parse_args(['-vvv'])
        self.assertEqual(ns.verbose, 3)
        ns = self.parse_args(['--verbose'])
        self.assertEqual(ns.verbose, 1)
        ns = self.parse_args(['--verbose'] * 3)
        self.assertEqual(ns.verbose, 3)
        ns = self.parse_args([])
        self.assertEqual(ns.verbose, 0)

    def test_rerun(self):
        for opt in '-w', '--rerun', '--verbose2':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.rerun)

    def test_verbose3(self):
        for opt in '-W', '--verbose3':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.verbose3)

    def test_quiet(self):
        for opt in '-q', '--quiet':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.quiet)
                self.assertEqual(ns.verbose, 0)

    def test_slowest(self):
        for opt in '-o', '--slowest':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.print_slow)

    def test_header(self):
        ns = self.parse_args(['--header'])
        self.assertTrue(ns.header)

        ns = self.parse_args(['--verbose'])
        self.assertTrue(ns.header)

    def test_randomize(self):
        for opt in ('-r', '--randomize'):
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.randomize)

        with os_helper.EnvironmentVarGuard() as env:
            # with SOURCE_DATE_EPOCH
            env['SOURCE_DATE_EPOCH'] = '1697839080'
            ns = self.parse_args(['--randomize'])
            regrtest = main.Regrtest(ns)
            self.assertFalse(regrtest.randomize)
            self.assertIsInstance(regrtest.random_seed, str)
            self.assertEqual(regrtest.random_seed, '1697839080')

            # without SOURCE_DATE_EPOCH
            del env['SOURCE_DATE_EPOCH']
            ns = self.parse_args(['--randomize'])
            regrtest = main.Regrtest(ns)
            self.assertTrue(regrtest.randomize)
            self.assertIsInstance(regrtest.random_seed, int)

    def test_randseed(self):
        ns = self.parse_args(['--randseed', '12345'])
        self.assertEqual(ns.random_seed, 12345)
        self.assertTrue(ns.randomize)
        self.checkError(['--randseed'], 'expected one argument')
        self.checkError(['--randseed', 'foo'], 'invalid int value')

    def test_fromfile(self):
        for opt in '-f', '--fromfile':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, 'foo'])
                self.assertEqual(ns.fromfile, 'foo')
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo', '-s'], "don't go together")

    def test_exclude(self):
        for opt in '-x', '--exclude':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.exclude)

    def test_single(self):
        for opt in '-s', '--single':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.single)
                self.checkError([opt, '-f', 'foo'], "don't go together")

    def test_match(self):
        for opt in '-m', '--match':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, 'pattern'])
                self.assertEqual(ns.match_tests, [('pattern', True)])
                self.checkError([opt], 'expected one argument')

        for opt in '-i', '--ignore':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, 'pattern'])
                self.assertEqual(ns.match_tests, [('pattern', False)])
                self.checkError([opt], 'expected one argument')

        ns = self.parse_args(['-m', 'pattern1', '-m', 'pattern2'])
        self.assertEqual(ns.match_tests, [('pattern1', True), ('pattern2', True)])

        ns = self.parse_args(['-m', 'pattern1', '-i', 'pattern2'])
        self.assertEqual(ns.match_tests, [('pattern1', True), ('pattern2', False)])

        ns = self.parse_args(['-i', 'pattern1', '-m', 'pattern2'])
        self.assertEqual(ns.match_tests, [('pattern1', False), ('pattern2', True)])

        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(os_helper.TESTFN, "w") as fp:
            print('matchfile1', file=fp)
            print('matchfile2', file=fp)

        filename = os.path.abspath(os_helper.TESTFN)
        ns = self.parse_args(['-m', 'match', '--matchfile', filename])
        self.assertEqual(ns.match_tests,
                         [('match', True), ('matchfile1', True), ('matchfile2', True)])

        ns = self.parse_args(['-i', 'match', '--ignorefile', filename])
        self.assertEqual(ns.match_tests,
                         [('match', False), ('matchfile1', False), ('matchfile2', False)])

    def test_failfast(self):
        for opt in '-G', '--failfast':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, '-v'])
                self.assertTrue(ns.failfast)
                ns = self.parse_args([opt, '-W'])
                self.assertTrue(ns.failfast)
                self.checkError([opt], '-G/--failfast needs either -v or -W')

    def test_use(self):
        for opt in '-u', '--use':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, 'gui,network'])
                self.assertEqual(ns.use_resources, ['gui', 'network'])

                ns = self.parse_args([opt, 'gui,none,network'])
                self.assertEqual(ns.use_resources, ['network'])

                expected = list(cmdline.ALL_RESOURCES)
                expected.remove('gui')
                ns = self.parse_args([opt, 'all,-gui'])
                self.assertEqual(ns.use_resources, expected)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid resource')

                # all + a resource not part of "all"
                ns = self.parse_args([opt, 'all,tzdata'])
                self.assertEqual(ns.use_resources,
                                 list(cmdline.ALL_RESOURCES) + ['tzdata'])

                # test another resource which is not part of "all"
                ns = self.parse_args([opt, 'extralargefile'])
                self.assertEqual(ns.use_resources, ['extralargefile'])

    def test_memlimit(self):
        for opt in '-M', '--memlimit':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, '4G'])
                self.assertEqual(ns.memlimit, '4G')
                self.checkError([opt], 'expected one argument')

    def test_testdir(self):
        ns = self.parse_args(['--testdir', 'foo'])
        self.assertEqual(ns.testdir, os.path.join(os_helper.SAVEDCWD, 'foo'))
        self.checkError(['--testdir'], 'expected one argument')

    def test_runleaks(self):
        for opt in '-L', '--runleaks':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.runleaks)

    def test_huntrleaks(self):
        for opt in '-R', '--huntrleaks':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, ':'])
                self.assertEqual(ns.huntrleaks, (5, 4, 'reflog.txt'))
                ns = self.parse_args([opt, '6:'])
                self.assertEqual(ns.huntrleaks, (6, 4, 'reflog.txt'))
                ns = self.parse_args([opt, ':3'])
                self.assertEqual(ns.huntrleaks, (5, 3, 'reflog.txt'))
                ns = self.parse_args([opt, '6:3:leaks.log'])
                self.assertEqual(ns.huntrleaks, (6, 3, 'leaks.log'))
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, '6'],
                                'needs 2 or 3 colon-separated arguments')
                self.checkError([opt, 'foo:'], 'invalid huntrleaks value')
                self.checkError([opt, '6:foo'], 'invalid huntrleaks value')

    def test_multiprocess(self):
        for opt in '-j', '--multiprocess':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, '2'])
                self.assertEqual(ns.use_mp, 2)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid int value')

    def test_coverage_sequential(self):
        for opt in '-T', '--coverage':
            with self.subTest(opt=opt):
                with support.captured_stderr() as stderr:
                    ns = self.parse_args([opt])
                self.assertTrue(ns.trace)
                self.assertIn(
                    "collecting coverage without -j is imprecise",
                    stderr.getvalue(),
                )

    @unittest.skipUnless(support.Py_DEBUG, 'need a debug build')
    def test_coverage_mp(self):
        for opt in '-T', '--coverage':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, '-j1'])
                self.assertTrue(ns.trace)

    def test_coverdir(self):
        for opt in '-D', '--coverdir':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, 'foo'])
                self.assertEqual(ns.coverdir,
                                 os.path.join(os_helper.SAVEDCWD, 'foo'))
                self.checkError([opt], 'expected one argument')

    def test_nocoverdir(self):
        for opt in '-N', '--nocoverdir':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertIsNone(ns.coverdir)

    def test_threshold(self):
        for opt in '-t', '--threshold':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt, '1000'])
                self.assertEqual(ns.threshold, 1000)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid int value')

    def test_nowindows(self):
        for opt in '-n', '--nowindows':
            with self.subTest(opt=opt):
                with contextlib.redirect_stderr(io.StringIO()) as stderr:
                    ns = self.parse_args([opt])
                self.assertTrue(ns.nowindows)
                err = stderr.getvalue()
                self.assertIn('the --nowindows (-n) option is deprecated', err)

    def test_forever(self):
        for opt in '-F', '--forever':
            with self.subTest(opt=opt):
                ns = self.parse_args([opt])
                self.assertTrue(ns.forever)

    def test_unrecognized_argument(self):
        self.checkError(['--xxx'], 'usage:')

    def test_long_option__partial(self):
        ns = self.parse_args(['--qui'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)

    def test_two_options(self):
        ns = self.parse_args(['--quiet', '--exclude'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)
        self.assertTrue(ns.exclude)

    def test_option_with_empty_string_value(self):
        ns = self.parse_args(['--start', ''])
        self.assertEqual(ns.start, '')

    def test_arg(self):
        ns = self.parse_args(['foo'])
        self.assertEqual(ns.args, ['foo'])

    def test_option_and_arg(self):
        ns = self.parse_args(['--quiet', 'foo'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)
        self.assertEqual(ns.args, ['foo'])

    def test_arg_option_arg(self):
        ns = self.parse_args(['test_unaryop', '-v', 'test_binop'])
        self.assertEqual(ns.verbose, 1)
        self.assertEqual(ns.args, ['test_unaryop', 'test_binop'])

    def test_unknown_option(self):
        self.checkError(['--unknown-option'],
                        'unrecognized arguments: --unknown-option')

    def create_regrtest(self, args):
        ns = cmdline._parse_args(args)

        # Check Regrtest attributes which are more reliable than Namespace
        # which has an unclear API
        with os_helper.EnvironmentVarGuard() as env:
            # Ignore SOURCE_DATE_EPOCH env var if it's set
            del env['SOURCE_DATE_EPOCH']

            regrtest = main.Regrtest(ns)

        return regrtest

    def check_ci_mode(self, args, use_resources, rerun=True):
        regrtest = self.create_regrtest(args)
        self.assertEqual(regrtest.num_workers, -1)
        self.assertEqual(regrtest.want_rerun, rerun)
        self.assertTrue(regrtest.randomize)
        self.assertIsInstance(regrtest.random_seed, int)
        self.assertTrue(regrtest.fail_env_changed)
        self.assertTrue(regrtest.print_slowest)
        self.assertTrue(regrtest.output_on_failure)
        self.assertEqual(sorted(regrtest.use_resources), sorted(use_resources))
        return regrtest

    def test_fast_ci(self):
        args = ['--fast-ci']
        use_resources = sorted(cmdline.ALL_RESOURCES)
        use_resources.remove('cpu')
        regrtest = self.check_ci_mode(args, use_resources)
        self.assertEqual(regrtest.timeout, 10 * 60)

    def test_fast_ci_python_cmd(self):
        args = ['--fast-ci', '--python', 'python -X dev']
        use_resources = sorted(cmdline.ALL_RESOURCES)
        use_resources.remove('cpu')
        regrtest = self.check_ci_mode(args, use_resources, rerun=False)
        self.assertEqual(regrtest.timeout, 10 * 60)
        self.assertEqual(regrtest.python_cmd, ('python', '-X', 'dev'))

    def test_fast_ci_resource(self):
        # it should be possible to override resources individually
        args = ['--fast-ci', '-u-network']
        use_resources = sorted(cmdline.ALL_RESOURCES)
        use_resources.remove('cpu')
        use_resources.remove('network')
        self.check_ci_mode(args, use_resources)

    def test_slow_ci(self):
        args = ['--slow-ci']
        use_resources = sorted(cmdline.ALL_RESOURCES)
        regrtest = self.check_ci_mode(args, use_resources)
        self.assertEqual(regrtest.timeout, 20 * 60)

    def test_dont_add_python_opts(self):
        args = ['--dont-add-python-opts']
        ns = cmdline._parse_args(args)
        self.assertFalse(ns._add_python_opts)

    def test_bisect(self):
        args = ['--bisect']
        regrtest = self.create_regrtest(args)
        self.assertTrue(regrtest.want_bisect)

    def test_verbose3_huntrleaks(self):
        args = ['-R', '3:10', '--verbose3']
        with support.captured_stderr():
            regrtest = self.create_regrtest(args)
        self.assertIsNotNone(regrtest.hunt_refleak)
        self.assertEqual(regrtest.hunt_refleak.warmups, 3)
        self.assertEqual(regrtest.hunt_refleak.runs, 10)
        self.assertFalse(regrtest.output_on_failure)

    def test_single_process(self):
        args = ['-j2', '--single-process']
        with support.captured_stderr():
            regrtest = self.create_regrtest(args)
        self.assertEqual(regrtest.num_workers, 0)
        self.assertTrue(regrtest.single_process)

        args = ['--fast-ci', '--single-process']
        with support.captured_stderr():
            regrtest = self.create_regrtest(args)
        self.assertEqual(regrtest.num_workers, 0)
        self.assertTrue(regrtest.single_process)


@dataclasses.dataclass(slots=True)
class Rerun:
    name: str
    match: str | None
    success: bool


class BaseTestCase(unittest.TestCase):
    TEST_UNIQUE_ID = 1
    TESTNAME_PREFIX = 'test_regrtest_'
    TESTNAME_REGEX = r'test_[a-zA-Z0-9_]+'

    def setUp(self):
        self.testdir = os.path.realpath(os.path.dirname(__file__))

        self.tmptestdir = tempfile.mkdtemp()
        self.addCleanup(os_helper.rmtree, self.tmptestdir)

    def create_test(self, name=None, code=None):
        if not name:
            name = 'noop%s' % BaseTestCase.TEST_UNIQUE_ID
            BaseTestCase.TEST_UNIQUE_ID += 1

        if code is None:
            code = textwrap.dedent("""
                    import unittest

                    class Tests(unittest.TestCase):
                        def test_empty_test(self):
                            pass
                """)

        # test_regrtest cannot be run twice in parallel because
        # of setUp() and create_test()
        name = self.TESTNAME_PREFIX + name
        path = os.path.join(self.tmptestdir, name + '.py')

        self.addCleanup(os_helper.unlink, path)
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

    def check_line(self, output, pattern, full=False, regex=True):
        if not regex:
            pattern = re.escape(pattern)
        if full:
            pattern += '\n'
        regex = re.compile(r'^' + pattern, re.MULTILINE)
        self.assertRegex(output, regex)

    def parse_executed_tests(self, output):
        regex = (fr'^{LOG_PREFIX}\[ *[0-9]+(?:/ *[0-9]+)*\] '
                 fr'({self.TESTNAME_REGEX}) {RESULT_REGEX}')
        parser = re.finditer(regex, output, re.MULTILINE)
        return list(match.group(1) for match in parser)

    def check_executed_tests(self, output, tests, *, stats,
                             skipped=(), failed=(),
                             env_changed=(), omitted=(),
                             rerun=None, run_no_tests=(),
                             resource_denied=(),
                             randomize=False, parallel=False, interrupted=False,
                             fail_env_changed=False,
                             forever=False, filtered=False):
        if isinstance(tests, str):
            tests = [tests]
        if isinstance(skipped, str):
            skipped = [skipped]
        if isinstance(resource_denied, str):
            resource_denied = [resource_denied]
        if isinstance(failed, str):
            failed = [failed]
        if isinstance(env_changed, str):
            env_changed = [env_changed]
        if isinstance(omitted, str):
            omitted = [omitted]
        if isinstance(run_no_tests, str):
            run_no_tests = [run_no_tests]
        if isinstance(stats, int):
            stats = TestStats(stats)
        if parallel:
            randomize = True

        rerun_failed = []
        if rerun is not None and not env_changed:
            failed = [rerun.name]
            if not rerun.success:
                rerun_failed.append(rerun.name)

        executed = self.parse_executed_tests(output)
        total_tests = list(tests)
        if rerun is not None:
            total_tests.append(rerun.name)
        if randomize:
            self.assertEqual(set(executed), set(total_tests), output)
        else:
            self.assertEqual(executed, total_tests, output)

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

        if resource_denied:
            regex = list_regex(r'%s test%s skipped \(resource denied\)', resource_denied)
            self.check_line(output, regex)

        if failed:
            regex = list_regex('%s test%s failed', failed)
            self.check_line(output, regex)

        if env_changed:
            regex = list_regex(r'%s test%s altered the execution environment '
                               r'\(env changed\)',
                               env_changed)
            self.check_line(output, regex)

        if omitted:
            regex = list_regex('%s test%s omitted', omitted)
            self.check_line(output, regex)

        if rerun is not None:
            regex = list_regex('%s re-run test%s', [rerun.name])
            self.check_line(output, regex)
            regex = LOG_PREFIX + r"Re-running 1 failed tests in verbose mode"
            self.check_line(output, regex)
            regex = fr"Re-running {rerun.name} in verbose mode"
            if rerun.match:
                regex = fr"{regex} \(matching: {rerun.match}\)"
            self.check_line(output, regex)

        if run_no_tests:
            regex = list_regex('%s test%s run no tests', run_no_tests)
            self.check_line(output, regex)

        good = (len(tests) - len(skipped) - len(resource_denied) - len(failed)
                - len(omitted) - len(env_changed) - len(run_no_tests))
        if good:
            regex = r'%s test%s OK\.' % (good, plural(good))
            if not skipped and not failed and (rerun is None or rerun.success) and good > 1:
                regex = 'All %s' % regex
            self.check_line(output, regex, full=True)

        if interrupted:
            self.check_line(output, 'Test suite interrupted by signal SIGINT.')

        # Total tests
        text = f'run={stats.tests_run:,}'
        if filtered:
            text = fr'{text} \(filtered\)'
        parts = [text]
        if stats.failures:
            parts.append(f'failures={stats.failures:,}')
        if stats.skipped:
            parts.append(f'skipped={stats.skipped:,}')
        line = fr'Total tests: {" ".join(parts)}'
        self.check_line(output, line, full=True)

        # Total test files
        run = len(total_tests) - len(resource_denied)
        if rerun is not None:
            total_failed = len(rerun_failed)
            total_rerun = 1
        else:
            total_failed = len(failed)
            total_rerun = 0
        if interrupted:
            run = 0
        text = f'run={run}'
        if not forever:
            text = f'{text}/{len(tests)}'
        if filtered:
            text = fr'{text} \(filtered\)'
        report = [text]
        for name, ntest in (
            ('failed', total_failed),
            ('env_changed', len(env_changed)),
            ('skipped', len(skipped)),
            ('resource_denied', len(resource_denied)),
            ('rerun', total_rerun),
            ('run_no_tests', len(run_no_tests)),
        ):
            if ntest:
                report.append(f'{name}={ntest}')
        line = fr'Total test files: {" ".join(report)}'
        self.check_line(output, line, full=True)

        # Result
        state = []
        if failed:
            state.append('FAILURE')
        elif fail_env_changed and env_changed:
            state.append('ENV CHANGED')
        if interrupted:
            state.append('INTERRUPTED')
        if not any((good, failed, interrupted, skipped,
                    env_changed, fail_env_changed)):
            state.append("NO TESTS RAN")
        elif not state:
            state.append('SUCCESS')
        state = ', '.join(state)
        if rerun is not None:
            new_state = 'SUCCESS' if rerun.success else 'FAILURE'
            state = f'{state} then {new_state}'
        self.check_line(output, f'Result: {state}', full=True)

    def parse_random_seed(self, output: str) -> str:
        match = self.regex_search(r'Using random seed: (.*)', output)
        return match.group(1)

    def run_command(self, args, input=None, exitcode=0, **kw):
        if not input:
            input = ''
        if 'stderr' not in kw:
            kw['stderr'] = subprocess.STDOUT

        env = kw.pop('env', None)
        if env is None:
            env = dict(os.environ)
            env.pop('SOURCE_DATE_EPOCH', None)

        proc = subprocess.run(args,
                              text=True,
                              input=input,
                              stdout=subprocess.PIPE,
                              env=env,
                              **kw)
        if proc.returncode != exitcode:
            msg = ("Command %s failed with exit code %s, but exit code %s expected!\n"
                   "\n"
                   "stdout:\n"
                   "---\n"
                   "%s\n"
                   "---\n"
                   % (str(args), proc.returncode, exitcode, proc.stdout))
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
        extraargs = []
        if 'uops' in sys._xoptions:
            # Pass -X uops along
            extraargs.extend(['-X', 'uops'])
        args = [sys.executable, *extraargs, '-X', 'faulthandler', '-I', *args]
        proc = self.run_command(args, **kw)
        return proc.stdout


class CheckActualTests(BaseTestCase):
    def test_finds_expected_number_of_tests(self):
        """
        Check that regrtest appears to find the expected set of tests.
        """
        args = ['-Wd', '-E', '-bb', '-m', 'test.regrtest', '--list-tests']
        output = self.run_python(args)
        rough_number_of_tests_found = len(output.splitlines())
        actual_testsuite_glob = os.path.join(glob.escape(os.path.dirname(__file__)),
                                             'test*.py')
        rough_counted_test_py_files = len(glob.glob(actual_testsuite_glob))
        # We're not trying to duplicate test finding logic in here,
        # just give a rough estimate of how many there should be and
        # be near that.  This is a regression test to prevent mishaps
        # such as https://bugs.python.org/issue37667 in the future.
        # If you need to change the values in here during some
        # mythical future test suite reorganization, don't go
        # overboard with logic and keep that goal in mind.
        self.assertGreater(rough_number_of_tests_found,
                           rough_counted_test_py_files*9//10,
                           msg='Unexpectedly low number of tests found in:\n'
                           f'{", ".join(output.splitlines())}')


@support.force_not_colorized_test_class
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
        self.regrtest_args.extend(('--timeout', '3600', '-j4'))
        if sys.platform == 'win32':
            self.regrtest_args.append('-n')

    def check_output(self, output):
        randseed = self.parse_random_seed(output)
        self.assertTrue(randseed.isdigit(), randseed)

        self.check_executed_tests(output, self.tests,
                                  randomize=True, stats=len(self.tests))

    def run_tests(self, args, env=None):
        output = self.run_python(args, env=env)
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
        if platform.machine() == 'ARM64':
            test_args.append('-arm64') # ARM 64-bit build
        elif platform.machine() == 'ARM':
            test_args.append('-arm32')   # 32-bit ARM build
        elif platform.architecture()[0] == '64bit':
            test_args.append('-x64')   # 64-bit build
        if not support.Py_DEBUG:
            test_args.append('+d')     # Release build, use python.exe
        if sysconfig.get_config_var("Py_GIL_DISABLED"):
            test_args.append('--disable-gil')
        self.run_batch(script, *test_args, *self.tests)

    @unittest.skipUnless(sys.platform == 'win32', 'Windows only')
    def test_pcbuild_rt(self):
        # PCbuild\rt.bat
        script = os.path.join(ROOT_DIR, r'PCbuild\rt.bat')
        if not os.path.isfile(script):
            self.skipTest(f'File "{script}" does not exist')
        rt_args = ["-q"]             # Quick, don't run tests twice
        if platform.machine() == 'ARM64':
            rt_args.append('-arm64') # ARM 64-bit build
        elif platform.machine() == 'ARM':
            rt_args.append('-arm32')   # 32-bit ARM build
        elif platform.architecture()[0] == '64bit':
            rt_args.append('-x64')   # 64-bit build
        if support.Py_DEBUG:
            rt_args.append('-d')     # Debug build, use python_d.exe
        if sysconfig.get_config_var("Py_GIL_DISABLED"):
            rt_args.append('--disable-gil')
        self.run_batch(script, *rt_args, *self.regrtest_args, *self.tests)


@support.force_not_colorized_test_class
class ArgsTestCase(BaseTestCase):
    """
    Test arguments of the Python test suite.
    """

    def run_tests(self, *testargs, **kw):
        cmdargs = ['-m', 'test', '--testdir=%s' % self.tmptestdir, *testargs]
        return self.run_python(cmdargs, **kw)

    def test_success(self):
        code = textwrap.dedent("""
            import unittest

            class PassingTests(unittest.TestCase):
                def test_test1(self):
                    pass

                def test_test2(self):
                    pass

                def test_test3(self):
                    pass
        """)
        tests = [self.create_test(f'ok{i}', code=code) for i in range(1, 6)]

        output = self.run_tests(*tests)
        self.check_executed_tests(output, tests,
                                  stats=3 * len(tests))

    def test_skip(self):
        code = textwrap.dedent("""
            import unittest
            raise unittest.SkipTest("nope")
        """)
        test_ok = self.create_test('ok')
        test_skip = self.create_test('skip', code=code)
        tests = [test_ok, test_skip]

        output = self.run_tests(*tests)
        self.check_executed_tests(output, tests,
                                  skipped=[test_skip],
                                  stats=1)

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

        output = self.run_tests(*tests, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, tests, failed=test_failing,
                                  stats=TestStats(2, 1))

    def test_resources(self):
        # test -u command line option
        tests = {}
        for resource in ('audio', 'network'):
            code = textwrap.dedent("""
                        from test import support; support.requires(%r)
                        import unittest
                        class PassingTest(unittest.TestCase):
                            def test_pass(self):
                                pass
                    """ % resource)

            tests[resource] = self.create_test(resource, code)
        test_names = sorted(tests.values())

        # -u all: 2 resources enabled
        output = self.run_tests('-u', 'all', *test_names)
        self.check_executed_tests(output, test_names, stats=2)

        # -u audio: 1 resource enabled
        output = self.run_tests('-uaudio', *test_names)
        self.check_executed_tests(output, test_names,
                                  resource_denied=tests['network'],
                                  stats=1)

        # no option: 0 resources enabled
        output = self.run_tests(*test_names, exitcode=EXITCODE_NO_TESTS_RAN)
        self.check_executed_tests(output, test_names,
                                  resource_denied=test_names,
                                  stats=0)

    def test_random(self):
        # test -r and --randseed command line option
        code = textwrap.dedent("""
            import random
            print("TESTRANDOM: %s" % random.randint(1, 1000))
        """)
        test = self.create_test('random', code)

        # first run to get the output with the random seed
        output = self.run_tests('-r', test, exitcode=EXITCODE_NO_TESTS_RAN)
        randseed = self.parse_random_seed(output)
        match = self.regex_search(r'TESTRANDOM: ([0-9]+)', output)
        test_random = int(match.group(1))

        # try to reproduce with the random seed
        output = self.run_tests('-r', f'--randseed={randseed}', test,
                                exitcode=EXITCODE_NO_TESTS_RAN)
        randseed2 = self.parse_random_seed(output)
        self.assertEqual(randseed2, randseed)

        match = self.regex_search(r'TESTRANDOM: ([0-9]+)', output)
        test_random2 = int(match.group(1))
        self.assertEqual(test_random2, test_random)

        # check that random.seed is used by default
        output = self.run_tests(test, exitcode=EXITCODE_NO_TESTS_RAN)
        randseed = self.parse_random_seed(output)
        self.assertTrue(randseed.isdigit(), randseed)

        # check SOURCE_DATE_EPOCH (integer)
        timestamp = '1697839080'
        env = dict(os.environ, SOURCE_DATE_EPOCH=timestamp)
        output = self.run_tests('-r', test, exitcode=EXITCODE_NO_TESTS_RAN,
                                env=env)
        randseed = self.parse_random_seed(output)
        self.assertEqual(randseed, timestamp)
        self.check_line(output, 'TESTRANDOM: 520')

        # check SOURCE_DATE_EPOCH (string)
        env = dict(os.environ, SOURCE_DATE_EPOCH='XYZ')
        output = self.run_tests('-r', test, exitcode=EXITCODE_NO_TESTS_RAN,
                                env=env)
        randseed = self.parse_random_seed(output)
        self.assertEqual(randseed, 'XYZ')
        self.check_line(output, 'TESTRANDOM: 22')

        # check SOURCE_DATE_EPOCH (empty string): ignore the env var
        env = dict(os.environ, SOURCE_DATE_EPOCH='')
        output = self.run_tests('-r', test, exitcode=EXITCODE_NO_TESTS_RAN,
                                env=env)
        randseed = self.parse_random_seed(output)
        self.assertTrue(randseed.isdigit(), randseed)

    def test_fromfile(self):
        # test --fromfile
        tests = [self.create_test() for index in range(5)]

        # Write the list of files using a format similar to regrtest output:
        # [1/2] test_1
        # [2/2] test_2
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

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
        stats = len(tests)
        self.check_executed_tests(output, tests, stats=stats)

        # test format '[2/7] test_opcodes'
        with open(filename, "w") as fp:
            for index, name in enumerate(tests, 1):
                print("[%s/%s] %s" % (index, len(tests), name), file=fp)

        output = self.run_tests('--fromfile', filename)
        self.check_executed_tests(output, tests, stats=stats)

        # test format 'test_opcodes'
        with open(filename, "w") as fp:
            for name in tests:
                print(name, file=fp)

        output = self.run_tests('--fromfile', filename)
        self.check_executed_tests(output, tests, stats=stats)

        # test format 'Lib/test/test_opcodes.py'
        with open(filename, "w") as fp:
            for name in tests:
                print('Lib/test/%s.py' % name, file=fp)

        output = self.run_tests('--fromfile', filename)
        self.check_executed_tests(output, tests, stats=stats)

    def test_interrupted(self):
        code = TEST_INTERRUPTED
        test = self.create_test('sigint', code=code)
        output = self.run_tests(test, exitcode=EXITCODE_INTERRUPTED)
        self.check_executed_tests(output, test, omitted=test,
                                  interrupted=True, stats=0)

    def test_slowest(self):
        # test --slowest
        tests = [self.create_test() for index in range(3)]
        output = self.run_tests("--slowest", *tests)
        self.check_executed_tests(output, tests, stats=len(tests))
        regex = ('10 slowest tests:\n'
                 '(?:- %s: .*\n){%s}'
                 % (self.TESTNAME_REGEX, len(tests)))
        self.check_line(output, regex)

    def test_slowest_interrupted(self):
        # Issue #25373: test --slowest with an interrupted test
        code = TEST_INTERRUPTED
        test = self.create_test("sigint", code=code)

        for multiprocessing in (False, True):
            with self.subTest(multiprocessing=multiprocessing):
                if multiprocessing:
                    args = ("--slowest", "-j2", test)
                else:
                    args = ("--slowest", test)
                output = self.run_tests(*args, exitcode=EXITCODE_INTERRUPTED)
                self.check_executed_tests(output, test,
                                          omitted=test, interrupted=True,
                                          stats=0)

                regex = ('10 slowest tests:\n')
                self.check_line(output, regex)

    def test_coverage(self):
        # test --coverage
        test = self.create_test('coverage')
        output = self.run_tests("--coverage", test)
        self.check_executed_tests(output, [test], stats=1)
        regex = (r'lines +cov% +module +\(path\)\n'
                 r'(?: *[0-9]+ *[0-9]{1,2}\.[0-9]% *[^ ]+ +\([^)]+\)+)+')
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

        # --forever
        output = self.run_tests('--forever', test, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, [test]*3, failed=test,
                                  stats=TestStats(3, 1),
                                  forever=True)

        # --forever --rerun
        output = self.run_tests('--forever', '--rerun', test, exitcode=0)
        self.check_executed_tests(output, [test]*3,
                                  rerun=Rerun(test,
                                              match='test_run',
                                              success=True),
                                  stats=TestStats(4, 1),
                                  forever=True)

    @support.requires_jit_disabled
    def check_leak(self, code, what, *, run_workers=False):
        test = self.create_test('huntrleaks', code=code)

        filename = 'reflog.txt'
        self.addCleanup(os_helper.unlink, filename)
        cmd = ['--huntrleaks', '3:3:']
        if run_workers:
            cmd.append('-j1')
        cmd.append(test)
        output = self.run_tests(*cmd,
                                exitcode=EXITCODE_BAD_TEST,
                                stderr=subprocess.STDOUT)
        self.check_executed_tests(output, [test], failed=test, stats=1)

        line = r'beginning 6 repetitions. .*\n123:456\n[.0-9X]{3} 111\n'
        self.check_line(output, line)

        line2 = '%s leaked [1, 1, 1] %s, sum=3\n' % (test, what)
        self.assertIn(line2, output)

        with open(filename) as fp:
            reflog = fp.read()
            self.assertIn(line2, reflog)

    @unittest.skipUnless(support.Py_DEBUG, 'need a debug build')
    def check_huntrleaks(self, *, run_workers: bool):
        # test --huntrleaks
        code = textwrap.dedent("""
            import unittest

            GLOBAL_LIST = []

            class RefLeakTest(unittest.TestCase):
                def test_leak(self):
                    GLOBAL_LIST.append(object())
        """)
        self.check_leak(code, 'references', run_workers=run_workers)

    def test_huntrleaks(self):
        self.check_huntrleaks(run_workers=False)

    def test_huntrleaks_mp(self):
        self.check_huntrleaks(run_workers=True)

    @unittest.skipUnless(support.Py_DEBUG, 'need a debug build')
    def test_huntrleaks_bisect(self):
        # test --huntrleaks --bisect
        code = textwrap.dedent("""
            import unittest

            GLOBAL_LIST = []

            class RefLeakTest(unittest.TestCase):
                def test1(self):
                    pass

                def test2(self):
                    pass

                def test3(self):
                    GLOBAL_LIST.append(object())

                def test4(self):
                    pass
        """)

        test = self.create_test('huntrleaks', code=code)

        filename = 'reflog.txt'
        self.addCleanup(os_helper.unlink, filename)
        cmd = ['--huntrleaks', '3:3:', '--bisect', test]
        output = self.run_tests(*cmd,
                                exitcode=EXITCODE_BAD_TEST,
                                stderr=subprocess.STDOUT)

        self.assertIn(f"Bisect {test}", output)
        self.assertIn(f"Bisect {test}: exit code 0", output)

        # test3 is the one which leaks
        self.assertIn("Bisection completed in", output)
        self.assertIn(
            "Tests (1):\n"
            f"* {test}.RefLeakTest.test3\n",
            output)

    @unittest.skipUnless(support.Py_DEBUG, 'need a debug build')
    def test_huntrleaks_fd_leak(self):
        # test --huntrleaks for file descriptor leak
        code = textwrap.dedent("""
            import os
            import unittest

            class FDLeakTest(unittest.TestCase):
                def test_leak(self):
                    fd = os.open(__file__, os.O_RDONLY)
                    # bug: never close the file descriptor
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

    @support.cpython_only
    def test_crashed(self):
        # Any code which causes a crash
        code = 'import faulthandler; faulthandler._sigsegv()'
        crash_test = self.create_test(name="crash", code=code)

        tests = [crash_test]
        output = self.run_tests("-j2", *tests, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, tests, failed=crash_test,
                                  parallel=True, stats=0)

    def parse_methods(self, output):
        regex = re.compile("^(test[^ ]+).*ok$", flags=re.MULTILINE)
        return [match.group(1) for match in regex.finditer(output)]

    def test_ignorefile(self):
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
        testname = self.create_test(code=code)

        # only run a subset
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        subset = [
            # only ignore the method name
            'test_method1',
            # ignore the full identifier
            '%s.Tests.test_method3' % testname]
        with open(filename, "w") as fp:
            for name in subset:
                print(name, file=fp)

        output = self.run_tests("-v", "--ignorefile", filename, testname)
        methods = self.parse_methods(output)
        subset = ['test_method2', 'test_method4']
        self.assertEqual(methods, subset)

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
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

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
        self.check_executed_tests(output, [testname],
                                  env_changed=testname, stats=1)

        # fail with --fail-env-changed
        output = self.run_tests("--fail-env-changed", testname,
                                exitcode=EXITCODE_ENV_CHANGED)
        self.check_executed_tests(output, [testname], env_changed=testname,
                                  fail_env_changed=True, stats=1)

        # rerun
        output = self.run_tests("--rerun", testname)
        self.check_executed_tests(output, [testname],
                                  env_changed=testname,
                                  rerun=Rerun(testname,
                                              match=None,
                                              success=True),
                                  stats=2)

    def test_rerun_fail(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_succeed(self):
                    return

                def test_fail_always(self):
                    # test that always fails
                    self.fail("bug")
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, [testname],
                                  rerun=Rerun(testname,
                                              "test_fail_always",
                                              success=False),
                                  stats=TestStats(3, 2))

    def test_rerun_success(self):
        # FAILURE then SUCCESS
        marker_filename = os.path.abspath("regrtest_marker_filename")
        self.addCleanup(os_helper.unlink, marker_filename)
        self.assertFalse(os.path.exists(marker_filename))

        code = textwrap.dedent(f"""
            import os.path
            import unittest

            marker_filename = {marker_filename!r}

            class Tests(unittest.TestCase):
                def test_succeed(self):
                    return

                def test_fail_once(self):
                    if not os.path.exists(marker_filename):
                        open(marker_filename, "w").close()
                        self.fail("bug")
        """)
        testname = self.create_test(code=code)

        # FAILURE then SUCCESS => exit code 0
        output = self.run_tests("--rerun", testname, exitcode=0)
        self.check_executed_tests(output, [testname],
                                  rerun=Rerun(testname,
                                              match="test_fail_once",
                                              success=True),
                                  stats=TestStats(3, 1))
        os_helper.unlink(marker_filename)

        # with --fail-rerun, exit code EXITCODE_RERUN_FAIL
        # on "FAILURE then SUCCESS" state.
        output = self.run_tests("--rerun", "--fail-rerun", testname,
                                exitcode=EXITCODE_RERUN_FAIL)
        self.check_executed_tests(output, [testname],
                                  rerun=Rerun(testname,
                                              match="test_fail_once",
                                              success=True),
                                  stats=TestStats(3, 1))
        os_helper.unlink(marker_filename)

    def test_rerun_setup_class_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            class ExampleTests(unittest.TestCase):
                @classmethod
                def setUpClass(self):
                    raise RuntimeError('Fail')

                def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  failed=[testname],
                                  rerun=Rerun(testname,
                                              match="ExampleTests",
                                              success=False),
                                  stats=0)

    def test_rerun_teardown_class_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            class ExampleTests(unittest.TestCase):
                @classmethod
                def tearDownClass(self):
                    raise RuntimeError('Fail')

                def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  failed=[testname],
                                  rerun=Rerun(testname,
                                              match="ExampleTests",
                                              success=False),
                                  stats=2)

    def test_rerun_setup_module_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            def setUpModule():
                raise RuntimeError('Fail')

            class ExampleTests(unittest.TestCase):
                def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  failed=[testname],
                                  rerun=Rerun(testname,
                                              match=None,
                                              success=False),
                                  stats=0)

    def test_rerun_teardown_module_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            def tearDownModule():
                raise RuntimeError('Fail')

            class ExampleTests(unittest.TestCase):
                def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, [testname],
                                  failed=[testname],
                                  rerun=Rerun(testname,
                                              match=None,
                                              success=False),
                                  stats=2)

    def test_rerun_setup_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            class ExampleTests(unittest.TestCase):
                def setUp(self):
                    raise RuntimeError('Fail')

                def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  failed=[testname],
                                  rerun=Rerun(testname,
                                              match="test_success",
                                              success=False),
                                  stats=2)

    def test_rerun_teardown_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            class ExampleTests(unittest.TestCase):
                def tearDown(self):
                    raise RuntimeError('Fail')

                def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  failed=[testname],
                                  rerun=Rerun(testname,
                                              match="test_success",
                                              success=False),
                                  stats=2)

    def test_rerun_async_setup_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            class ExampleTests(unittest.IsolatedAsyncioTestCase):
                async def asyncSetUp(self):
                    raise RuntimeError('Fail')

                async def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  rerun=Rerun(testname,
                                              match="test_success",
                                              success=False),
                                  stats=2)

    def test_rerun_async_teardown_hook_failure(self):
        # FAILURE then FAILURE
        code = textwrap.dedent("""
            import unittest

            class ExampleTests(unittest.IsolatedAsyncioTestCase):
                async def asyncTearDown(self):
                    raise RuntimeError('Fail')

                async def test_success(self):
                    return
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--rerun", testname, exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  failed=[testname],
                                  rerun=Rerun(testname,
                                              match="test_success",
                                              success=False),
                                  stats=2)

    def test_no_tests_ran(self):
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_bug(self):
                    pass
        """)
        testname = self.create_test(code=code)

        output = self.run_tests(testname, "-m", "nosuchtest",
                                exitcode=EXITCODE_NO_TESTS_RAN)
        self.check_executed_tests(output, [testname],
                                  run_no_tests=testname,
                                  stats=0, filtered=True)

    def test_no_tests_ran_skip(self):
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_skipped(self):
                    self.skipTest("because")
        """)
        testname = self.create_test(code=code)

        output = self.run_tests(testname)
        self.check_executed_tests(output, [testname],
                                  stats=TestStats(1, skipped=1))

    def test_no_tests_ran_multiple_tests_nonexistent(self):
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_bug(self):
                    pass
        """)
        testname = self.create_test(code=code)
        testname2 = self.create_test(code=code)

        output = self.run_tests(testname, testname2, "-m", "nosuchtest",
                                exitcode=EXITCODE_NO_TESTS_RAN)
        self.check_executed_tests(output, [testname, testname2],
                                  run_no_tests=[testname, testname2],
                                  stats=0, filtered=True)

    def test_no_test_ran_some_test_exist_some_not(self):
        code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_bug(self):
                    pass
        """)
        testname = self.create_test(code=code)
        other_code = textwrap.dedent("""
            import unittest

            class Tests(unittest.TestCase):
                def test_other_bug(self):
                    pass
        """)
        testname2 = self.create_test(code=other_code)

        output = self.run_tests(testname, testname2, "-m", "nosuchtest",
                                "-m", "test_other_bug", exitcode=0)
        self.check_executed_tests(output, [testname, testname2],
                                  run_no_tests=[testname],
                                  stats=1, filtered=True)

    @support.cpython_only
    def test_uncollectable(self):
        # Skip test if _testcapi is missing
        import_helper.import_module('_testcapi')

        code = textwrap.dedent(r"""
            import _testcapi
            import gc
            import unittest

            @_testcapi.with_tp_del
            class Garbage:
                def __tp_del__(self):
                    pass

            class Tests(unittest.TestCase):
                def test_garbage(self):
                    # create an uncollectable object
                    obj = Garbage()
                    obj.ref_cycle = obj
                    obj = None
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--fail-env-changed", testname,
                                exitcode=EXITCODE_ENV_CHANGED)
        self.check_executed_tests(output, [testname],
                                  env_changed=[testname],
                                  fail_env_changed=True,
                                  stats=1)

    def test_multiprocessing_timeout(self):
        code = textwrap.dedent(r"""
            import time
            import unittest
            try:
                import faulthandler
            except ImportError:
                faulthandler = None

            class Tests(unittest.TestCase):
                # test hangs and so should be stopped by the timeout
                def test_sleep(self):
                    # we want to test regrtest multiprocessing timeout,
                    # not faulthandler timeout
                    if faulthandler is not None:
                        faulthandler.cancel_dump_traceback_later()

                    time.sleep(60 * 5)
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("-j2", "--timeout=1.0", testname,
                                exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, [testname],
                                  failed=testname, stats=0)
        self.assertRegex(output,
                         re.compile('%s timed out' % testname, re.MULTILINE))

    def test_unraisable_exc(self):
        # --fail-env-changed must catch unraisable exception.
        # The exception must be displayed even if sys.stderr is redirected.
        code = textwrap.dedent(r"""
            import unittest
            import weakref
            from test.support import captured_stderr

            class MyObject:
                pass

            def weakref_callback(obj):
                raise Exception("weakref callback bug")

            class Tests(unittest.TestCase):
                def test_unraisable_exc(self):
                    obj = MyObject()
                    ref = weakref.ref(obj, weakref_callback)
                    with captured_stderr() as stderr:
                        # call weakref_callback() which logs
                        # an unraisable exception
                        obj = None
                    self.assertEqual(stderr.getvalue(), '')
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--fail-env-changed", "-v", testname,
                                exitcode=EXITCODE_ENV_CHANGED)
        self.check_executed_tests(output, [testname],
                                  env_changed=[testname],
                                  fail_env_changed=True,
                                  stats=1)
        self.assertIn("Warning -- Unraisable exception", output)
        self.assertIn("Exception: weakref callback bug", output)

    def test_threading_excepthook(self):
        # --fail-env-changed must catch uncaught thread exception.
        # The exception must be displayed even if sys.stderr is redirected.
        code = textwrap.dedent(r"""
            import threading
            import unittest
            from test.support import captured_stderr

            class MyObject:
                pass

            def func_bug():
                raise Exception("bug in thread")

            class Tests(unittest.TestCase):
                def test_threading_excepthook(self):
                    with captured_stderr() as stderr:
                        thread = threading.Thread(target=func_bug)
                        thread.start()
                        thread.join()
                    self.assertEqual(stderr.getvalue(), '')
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--fail-env-changed", "-v", testname,
                                exitcode=EXITCODE_ENV_CHANGED)
        self.check_executed_tests(output, [testname],
                                  env_changed=[testname],
                                  fail_env_changed=True,
                                  stats=1)
        self.assertIn("Warning -- Uncaught thread exception", output)
        self.assertIn("Exception: bug in thread", output)

    def test_print_warning(self):
        # bpo-45410: The order of messages must be preserved when -W and
        # support.print_warning() are used.
        code = textwrap.dedent(r"""
            import sys
            import unittest
            from test import support

            class MyObject:
                pass

            def func_bug():
                raise Exception("bug in thread")

            class Tests(unittest.TestCase):
                def test_print_warning(self):
                    print("msg1: stdout")
                    support.print_warning("msg2: print_warning")
                    # Fail with ENV CHANGED to see print_warning() log
                    support.environment_altered = True
        """)
        testname = self.create_test(code=code)

        # Expect an output like:
        #
        #   test_threading_excepthook (test.test_x.Tests) ... msg1: stdout
        #   Warning -- msg2: print_warning
        #   ok
        regex = (r"test_print_warning.*msg1: stdout\n"
                 r"Warning -- msg2: print_warning\n"
                 r"ok\n")
        for option in ("-v", "-W"):
            with self.subTest(option=option):
                cmd = ["--fail-env-changed", option, testname]
                output = self.run_tests(*cmd, exitcode=EXITCODE_ENV_CHANGED)
                self.check_executed_tests(output, [testname],
                                          env_changed=[testname],
                                          fail_env_changed=True,
                                          stats=1)
                self.assertRegex(output, regex)

    def test_unicode_guard_env(self):
        guard = os.environ.get(setup.UNICODE_GUARD_ENV)
        self.assertIsNotNone(guard, f"{setup.UNICODE_GUARD_ENV} not set")
        if guard.isascii():
            # Skip to signify that the env var value was changed by the user;
            # possibly to something ASCII to work around Unicode issues.
            self.skipTest("Modified guard")

    def test_cleanup(self):
        dirname = os.path.join(self.tmptestdir, "test_python_123")
        os.mkdir(dirname)
        filename = os.path.join(self.tmptestdir, "test_python_456")
        open(filename, "wb").close()
        names = [dirname, filename]

        cmdargs = ['-m', 'test',
                   '--tempdir=%s' % self.tmptestdir,
                   '--cleanup']
        self.run_python(cmdargs)

        for name in names:
            self.assertFalse(os.path.exists(name), name)

    @unittest.skipIf(support.is_wasi,
                     'checking temp files is not implemented on WASI')
    def test_leak_tmp_file(self):
        code = textwrap.dedent(r"""
            import os.path
            import tempfile
            import unittest

            class FileTests(unittest.TestCase):
                def test_leak_tmp_file(self):
                    filename = os.path.join(tempfile.gettempdir(), 'mytmpfile')
                    with open(filename, "wb") as fp:
                        fp.write(b'content')
        """)
        testnames = [self.create_test(code=code) for _ in range(3)]

        output = self.run_tests("--fail-env-changed", "-v", "-j2", *testnames,
                                exitcode=EXITCODE_ENV_CHANGED)
        self.check_executed_tests(output, testnames,
                                  env_changed=testnames,
                                  fail_env_changed=True,
                                  parallel=True,
                                  stats=len(testnames))
        for testname in testnames:
            self.assertIn(f"Warning -- {testname} leaked temporary "
                          f"files (1): mytmpfile",
                          output)

    def test_worker_decode_error(self):
        # gh-109425: Use "backslashreplace" error handler to decode stdout.
        if sys.platform == 'win32':
            encoding = locale.getencoding()
        else:
            encoding = sys.stdout.encoding
            if encoding is None:
                encoding = sys.__stdout__.encoding
                if encoding is None:
                    self.skipTest("cannot get regrtest worker encoding")

        nonascii = bytes(ch for ch in range(128, 256))
        corrupted_output = b"nonascii:%s\n" % (nonascii,)
        # gh-108989: On Windows, assertion errors are written in UTF-16: when
        # decoded each letter is follow by a NUL character.
        assertion_failed = 'Assertion failed: tstate_is_alive(tstate)\n'
        corrupted_output += assertion_failed.encode('utf-16-le')
        try:
            corrupted_output.decode(encoding)
        except UnicodeDecodeError:
            pass
        else:
            self.skipTest(f"{encoding} can decode non-ASCII bytes")

        expected_line = corrupted_output.decode(encoding, 'backslashreplace')

        code = textwrap.dedent(fr"""
            import sys
            import unittest

            class Tests(unittest.TestCase):
                def test_pass(self):
                    pass

            # bytes which cannot be decoded from UTF-8
            corrupted_output = {corrupted_output!a}
            sys.stdout.buffer.write(corrupted_output)
            sys.stdout.buffer.flush()
        """)
        testname = self.create_test(code=code)

        output = self.run_tests("--fail-env-changed", "-v", "-j1", testname)
        self.check_executed_tests(output, [testname],
                                  parallel=True,
                                  stats=1)
        self.check_line(output, expected_line, regex=False)

    def test_doctest(self):
        code = textwrap.dedent(r'''
            import doctest
            import sys
            from test import support

            def my_function():
                """
                Pass:

                >>> 1 + 1
                2

                Failure:

                >>> 2 + 3
                23
                >>> 1 + 1
                11

                Skipped test (ignored):

                >>> id(1.0)  # doctest: +SKIP
                7948648
                """

            def load_tests(loader, tests, pattern):
                tests.addTest(doctest.DocTestSuite())
                return tests
        ''')
        testname = self.create_test(code=code)

        output = self.run_tests("--fail-env-changed", "-v", "-j1", testname,
                                exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, [testname],
                                  failed=[testname],
                                  parallel=True,
                                  stats=TestStats(1, 1, 0))

    def _check_random_seed(self, run_workers: bool):
        # gh-109276: When -r/--randomize is used, random.seed() is called
        # with the same random seed before running each test file.
        code = textwrap.dedent(r'''
            import random
            import unittest

            class RandomSeedTest(unittest.TestCase):
                def test_randint(self):
                    numbers = [random.randint(0, 1000) for _ in range(10)]
                    print(f"Random numbers: {numbers}")
        ''')
        tests = [self.create_test(name=f'test_random{i}', code=code)
                 for i in range(1, 3+1)]

        random_seed = 856_656_202
        cmd = ["--randomize", f"--randseed={random_seed}"]
        if run_workers:
            # run as many worker processes than the number of tests
            cmd.append(f'-j{len(tests)}')
        cmd.extend(tests)
        output = self.run_tests(*cmd)

        random.seed(random_seed)
        # Make the assumption that nothing consume entropy between libregrest
        # setup_tests() which calls random.seed() and RandomSeedTest calling
        # random.randint().
        numbers = [random.randint(0, 1000) for _ in range(10)]
        expected = f"Random numbers: {numbers}"

        regex = r'^Random numbers: .*$'
        matches = re.findall(regex, output, flags=re.MULTILINE)
        self.assertEqual(matches, [expected] * len(tests))

    def test_random_seed(self):
        self._check_random_seed(run_workers=False)

    def test_random_seed_workers(self):
        self._check_random_seed(run_workers=True)

    def test_python_command(self):
        code = textwrap.dedent(r"""
            import sys
            import unittest

            class WorkerTests(unittest.TestCase):
                def test_dev_mode(self):
                    self.assertTrue(sys.flags.dev_mode)
        """)
        tests = [self.create_test(code=code) for _ in range(3)]

        # Custom Python command: "python -X dev"
        python_cmd = [sys.executable, '-X', 'dev']
        # test.libregrtest.cmdline uses shlex.split() to parse the Python
        # command line string
        python_cmd = shlex.join(python_cmd)

        output = self.run_tests("--python", python_cmd, "-j0", *tests)
        self.check_executed_tests(output, tests,
                                  stats=len(tests), parallel=True)

    def test_unload_tests(self):
        # Test that unloading test modules does not break tests
        # that import from other tests.
        # The test execution order matters for this test.
        # Both test_regrtest_a and test_regrtest_c which are executed before
        # and after test_regrtest_b import a submodule from the test_regrtest_b
        # package and use it in testing. test_regrtest_b itself does not import
        # that submodule.
        # Previously test_regrtest_c failed because test_regrtest_b.util in
        # sys.modules was left after test_regrtest_a (making the import
        # statement no-op), but new test_regrtest_b without the util attribute
        # was imported for test_regrtest_b.
        testdir = os.path.join(os.path.dirname(__file__),
                               'regrtestdata', 'import_from_tests')
        tests = [f'test_regrtest_{name}' for name in ('a', 'b', 'c')]
        args = ['-Wd', '-E', '-bb', '-m', 'test', '--testdir=%s' % testdir, *tests]
        output = self.run_python(args)
        self.check_executed_tests(output, tests, stats=3)

    def check_add_python_opts(self, option):
        # --fast-ci and --slow-ci add "-u -W default -bb -E" options to Python

        # Skip test if _testinternalcapi is missing
        import_helper.import_module('_testinternalcapi')

        code = textwrap.dedent(r"""
            import sys
            import unittest
            from test import support
            try:
                from _testcapi import config_get
            except ImportError:
                config_get = None

            # WASI/WASM buildbots don't use -E option
            use_environment = (support.is_emscripten or support.is_wasi)

            class WorkerTests(unittest.TestCase):
                @unittest.skipUnless(config_get is None, 'need config_get()')
                def test_config(self):
                    config = config_get()
                    # -u option
                    self.assertEqual(config_get('buffered_stdio'), 0)
                    # -W default option
                    self.assertTrue(config_get('warnoptions'), ['default'])
                    # -bb option
                    self.assertTrue(config_get('bytes_warning'), 2)
                    # -E option
                    self.assertTrue(config_get('use_environment'), use_environment)

                def test_python_opts(self):
                    # -u option
                    self.assertTrue(sys.__stdout__.write_through)
                    self.assertTrue(sys.__stderr__.write_through)

                    # -W default option
                    self.assertTrue(sys.warnoptions, ['default'])

                    # -bb option
                    self.assertEqual(sys.flags.bytes_warning, 2)

                    # -E option
                    self.assertEqual(not sys.flags.ignore_environment,
                                     use_environment)
        """)
        testname = self.create_test(code=code)

        # Use directly subprocess to control the exact command line
        cmd = [sys.executable,
               "-m", "test", option,
               f'--testdir={self.tmptestdir}',
               testname]
        proc = subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              text=True)
        self.assertEqual(proc.returncode, 0, proc)

    def test_add_python_opts(self):
        for opt in ("--fast-ci", "--slow-ci"):
            with self.subTest(opt=opt):
                self.check_add_python_opts(opt)

    # gh-76319: Raising SIGSEGV on Android may not cause a crash.
    @unittest.skipIf(support.is_android,
                     'raising SIGSEGV on Android is unreliable')
    def test_worker_output_on_failure(self):
        # Skip test if faulthandler is missing
        import_helper.import_module('faulthandler')

        code = textwrap.dedent(r"""
            import faulthandler
            import unittest
            from test import support

            class CrashTests(unittest.TestCase):
                def test_crash(self):
                    print("just before crash!", flush=True)

                    with support.SuppressCrashReport():
                        faulthandler._sigsegv(True)
        """)
        testname = self.create_test(code=code)

        # Sanitizers must not handle SIGSEGV (ex: for test_enable_fd())
        env = dict(os.environ)
        option = 'handle_segv=0'
        support.set_sanitizer_env_var(env, option)

        output = self.run_tests("-j1", testname,
                                exitcode=EXITCODE_BAD_TEST,
                                env=env)
        self.check_executed_tests(output, testname,
                                  failed=[testname],
                                  stats=0, parallel=True)
        if not support.MS_WINDOWS:
            exitcode = -int(signal.SIGSEGV)
            self.assertIn(f"Exit code {exitcode} (SIGSEGV)", output)
        self.check_line(output, "just before crash!", full=True, regex=False)

    def test_verbose3(self):
        code = textwrap.dedent(r"""
            import unittest
            from test import support

            class VerboseTests(unittest.TestCase):
                def test_pass(self):
                    print("SPAM SPAM SPAM")
        """)
        testname = self.create_test(code=code)

        # Run sequentially
        output = self.run_tests("--verbose3", testname)
        self.check_executed_tests(output, testname, stats=1)
        self.assertNotIn('SPAM SPAM SPAM', output)

        # -R option needs a debug build
        if support.Py_DEBUG:
            # Check for reference leaks, run in parallel
            output = self.run_tests("-R", "3:3", "-j1", "--verbose3", testname)
            self.check_executed_tests(output, testname, stats=1, parallel=True)
            self.assertNotIn('SPAM SPAM SPAM', output)

    def test_xml(self):
        code = textwrap.dedent(r"""
            import unittest
            from test import support

            class VerboseTests(unittest.TestCase):
                def test_failed(self):
                    print("abc \x1b def")
                    self.fail()
        """)
        testname = self.create_test(code=code)

        # Run sequentially
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        output = self.run_tests(testname, "--junit-xml", filename,
                                exitcode=EXITCODE_BAD_TEST)
        self.check_executed_tests(output, testname,
                                  failed=testname,
                                  stats=TestStats(1, 1, 0))

        # Test generated XML
        with open(filename, encoding="utf8") as fp:
            content = fp.read()

        testsuite = ElementTree.fromstring(content)
        self.assertEqual(int(testsuite.get('tests')), 1)
        self.assertEqual(int(testsuite.get('errors')), 0)
        self.assertEqual(int(testsuite.get('failures')), 1)

        testcase = testsuite[0][0]
        self.assertEqual(testcase.get('status'), 'run')
        self.assertEqual(testcase.get('result'), 'completed')
        self.assertGreater(float(testcase.get('time')), 0)
        for out in testcase.iter('system-out'):
            self.assertEqual(out.text, r"abc \x1b def")


class TestUtils(unittest.TestCase):
    def test_format_duration(self):
        self.assertEqual(utils.format_duration(0),
                         '0 ms')
        self.assertEqual(utils.format_duration(1e-9),
                         '1 ms')
        self.assertEqual(utils.format_duration(10e-3),
                         '10 ms')
        self.assertEqual(utils.format_duration(1.5),
                         '1.5 sec')
        self.assertEqual(utils.format_duration(1),
                         '1.0 sec')
        self.assertEqual(utils.format_duration(2 * 60),
                         '2 min')
        self.assertEqual(utils.format_duration(2 * 60 + 1),
                         '2 min 1 sec')
        self.assertEqual(utils.format_duration(3 * 3600),
                         '3 hour')
        self.assertEqual(utils.format_duration(3 * 3600  + 2 * 60 + 1),
                         '3 hour 2 min')
        self.assertEqual(utils.format_duration(3 * 3600 + 1),
                         '3 hour 1 sec')

    def test_normalize_test_name(self):
        normalize = normalize_test_name
        self.assertEqual(normalize('test_access (test.test_os.FileTests.test_access)'),
                         'test_access')
        self.assertEqual(normalize('setUpClass (test.test_os.ChownFileTests)', is_error=True),
                         'ChownFileTests')
        self.assertEqual(normalize('test_success (test.test_bug.ExampleTests.test_success)', is_error=True),
                         'test_success')
        self.assertIsNone(normalize('setUpModule (test.test_x)', is_error=True))
        self.assertIsNone(normalize('tearDownModule (test.test_module)', is_error=True))

    def test_format_resources(self):
        format_resources = utils.format_resources
        ALL_RESOURCES = utils.ALL_RESOURCES
        self.assertEqual(
            format_resources(("network",)),
            'resources (1): network')
        self.assertEqual(
            format_resources(("audio", "decimal", "network")),
            'resources (3): audio,decimal,network')
        self.assertEqual(
            format_resources(ALL_RESOURCES),
            'resources: all')
        self.assertEqual(
            format_resources(tuple(name for name in ALL_RESOURCES
                                   if name != "cpu")),
            'resources: all,-cpu')
        self.assertEqual(
            format_resources((*ALL_RESOURCES, "tzdata")),
            'resources: all,tzdata')

    def test_match_test(self):
        class Test:
            def __init__(self, test_id):
                self.test_id = test_id

            def id(self):
                return self.test_id

        # Restore patterns once the test completes
        patterns = get_match_tests()
        self.addCleanup(set_match_tests, patterns)

        test_access = Test('test.test_os.FileTests.test_access')
        test_chdir = Test('test.test_os.Win32ErrorTests.test_chdir')
        test_copy = Test('test.test_shutil.TestCopy.test_copy')

        # Test acceptance
        with support.swap_attr(support, '_test_matchers', ()):
            # match all
            set_match_tests([])
            self.assertTrue(match_test(test_access))
            self.assertTrue(match_test(test_chdir))

            # match all using None
            set_match_tests(None)
            self.assertTrue(match_test(test_access))
            self.assertTrue(match_test(test_chdir))

            # match the full test identifier
            set_match_tests([(test_access.id(), True)])
            self.assertTrue(match_test(test_access))
            self.assertFalse(match_test(test_chdir))

            # match the module name
            set_match_tests([('test_os', True)])
            self.assertTrue(match_test(test_access))
            self.assertTrue(match_test(test_chdir))
            self.assertFalse(match_test(test_copy))

            # Test '*' pattern
            set_match_tests([('test_*', True)])
            self.assertTrue(match_test(test_access))
            self.assertTrue(match_test(test_chdir))

            # Test case sensitivity
            set_match_tests([('filetests', True)])
            self.assertFalse(match_test(test_access))
            set_match_tests([('FileTests', True)])
            self.assertTrue(match_test(test_access))

            # Test pattern containing '.' and a '*' metacharacter
            set_match_tests([('*test_os.*.test_*', True)])
            self.assertTrue(match_test(test_access))
            self.assertTrue(match_test(test_chdir))
            self.assertFalse(match_test(test_copy))

            # Multiple patterns
            set_match_tests([(test_access.id(), True), (test_chdir.id(), True)])
            self.assertTrue(match_test(test_access))
            self.assertTrue(match_test(test_chdir))
            self.assertFalse(match_test(test_copy))

            set_match_tests([('test_access', True), ('DONTMATCH', True)])
            self.assertTrue(match_test(test_access))
            self.assertFalse(match_test(test_chdir))

        # Test rejection
        with support.swap_attr(support, '_test_matchers', ()):
            # match the full test identifier
            set_match_tests([(test_access.id(), False)])
            self.assertFalse(match_test(test_access))
            self.assertTrue(match_test(test_chdir))

            # match the module name
            set_match_tests([('test_os', False)])
            self.assertFalse(match_test(test_access))
            self.assertFalse(match_test(test_chdir))
            self.assertTrue(match_test(test_copy))

            # Test '*' pattern
            set_match_tests([('test_*', False)])
            self.assertFalse(match_test(test_access))
            self.assertFalse(match_test(test_chdir))

            # Test case sensitivity
            set_match_tests([('filetests', False)])
            self.assertTrue(match_test(test_access))
            set_match_tests([('FileTests', False)])
            self.assertFalse(match_test(test_access))

            # Test pattern containing '.' and a '*' metacharacter
            set_match_tests([('*test_os.*.test_*', False)])
            self.assertFalse(match_test(test_access))
            self.assertFalse(match_test(test_chdir))
            self.assertTrue(match_test(test_copy))

            # Multiple patterns
            set_match_tests([(test_access.id(), False), (test_chdir.id(), False)])
            self.assertFalse(match_test(test_access))
            self.assertFalse(match_test(test_chdir))
            self.assertTrue(match_test(test_copy))

            set_match_tests([('test_access', False), ('DONTMATCH', False)])
            self.assertFalse(match_test(test_access))
            self.assertTrue(match_test(test_chdir))

        # Test mixed filters
        with support.swap_attr(support, '_test_matchers', ()):
            set_match_tests([('*test_os', False), ('test_access', True)])
            self.assertTrue(match_test(test_access))
            self.assertFalse(match_test(test_chdir))
            self.assertTrue(match_test(test_copy))

            set_match_tests([('*test_os', True), ('test_access', False)])
            self.assertFalse(match_test(test_access))
            self.assertTrue(match_test(test_chdir))
            self.assertFalse(match_test(test_copy))

    def test_sanitize_xml(self):
        sanitize_xml = utils.sanitize_xml

        # escape invalid XML characters
        self.assertEqual(sanitize_xml('abc \x1b\x1f def'),
                         r'abc \x1b\x1f def')
        self.assertEqual(sanitize_xml('nul:\x00, bell:\x07'),
                         r'nul:\x00, bell:\x07')
        self.assertEqual(sanitize_xml('surrogate:\uDC80'),
                         r'surrogate:\udc80')
        self.assertEqual(sanitize_xml('illegal \uFFFE and \uFFFF'),
                         r'illegal \ufffe and \uffff')

        # no escape for valid XML characters
        self.assertEqual(sanitize_xml('a\n\tb'),
                         'a\n\tb')
        self.assertEqual(sanitize_xml('valid t\xe9xt \u20ac'),
                         'valid t\xe9xt \u20ac')


from test.libregrtest.results import TestResults


class TestColorized(unittest.TestCase):
    def test_test_result_get_state(self):
        # Arrange
        green = _colorize.ANSIColors.GREEN
        red = _colorize.ANSIColors.BOLD_RED
        reset = _colorize.ANSIColors.RESET
        yellow = _colorize.ANSIColors.YELLOW

        good_results = TestResults()
        good_results.good = ["good1", "good2"]
        bad_results = TestResults()
        bad_results.bad = ["bad1", "bad2"]
        no_results = TestResults()
        no_results.bad = []
        interrupted_results = TestResults()
        interrupted_results.interrupted = True
        interrupted_worker_bug = TestResults()
        interrupted_worker_bug.interrupted = True
        interrupted_worker_bug.worker_bug = True

        for results, expected in (
            (good_results, f"{green}SUCCESS{reset}"),
            (bad_results, f"{red}FAILURE{reset}"),
            (no_results, f"{yellow}NO TESTS RAN{reset}"),
            (interrupted_results, f"{yellow}INTERRUPTED{reset}"),
            (
                interrupted_worker_bug,
                f"{yellow}INTERRUPTED{reset}, {red}WORKER BUG{reset}",
            ),
        ):
            with self.subTest(results=results, expected=expected):
                # Act
                with unittest.mock.patch(
                    "_colorize.can_colorize", return_value=True
                ):
                    result = results.get_state(fail_env_changed=False)

                # Assert
                self.assertEqual(result, expected)


if __name__ == '__main__':
    setup.setup_process()
    unittest.main()
