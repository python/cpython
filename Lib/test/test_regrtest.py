"""
Tests of regrtest.py.
"""

import argparse
import faulthandler
import getopt
import os.path
import unittest
from test import regrtest, support

class ParseArgsTestCase(unittest.TestCase):

    """Test regrtest's argument parsing."""

    def checkError(self, args, msg):
        with support.captured_stderr() as err, self.assertRaises(SystemExit):
            regrtest._parse_args(args)
        self.assertIn(msg, err.getvalue())

    def test_help(self):
        for opt in '-h', '--help':
            with self.subTest(opt=opt):
                with support.captured_stdout() as out, \
                     self.assertRaises(SystemExit):
                    regrtest._parse_args([opt])
                self.assertIn('Run Python regression tests.', out.getvalue())

    @unittest.skipUnless(hasattr(faulthandler, 'dump_traceback_later'),
                         "faulthandler.dump_traceback_later() required")
    def test_timeout(self):
        ns = regrtest._parse_args(['--timeout', '4.2'])
        self.assertEqual(ns.timeout, 4.2)
        self.checkError(['--timeout'], 'expected one argument')
        self.checkError(['--timeout', 'foo'], 'invalid float value')

    def test_wait(self):
        ns = regrtest._parse_args(['--wait'])
        self.assertTrue(ns.wait)

    def test_slaveargs(self):
        ns = regrtest._parse_args(['--slaveargs', '[[], {}]'])
        self.assertEqual(ns.slaveargs, '[[], {}]')
        self.checkError(['--slaveargs'], 'expected one argument')

    def test_start(self):
        for opt in '-S', '--start':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, 'foo'])
                self.assertEqual(ns.start, 'foo')
                self.checkError([opt], 'expected one argument')

    def test_verbose(self):
        ns = regrtest._parse_args(['-v'])
        self.assertEqual(ns.verbose, 1)
        ns = regrtest._parse_args(['-vvv'])
        self.assertEqual(ns.verbose, 3)
        ns = regrtest._parse_args(['--verbose'])
        self.assertEqual(ns.verbose, 1)
        ns = regrtest._parse_args(['--verbose'] * 3)
        self.assertEqual(ns.verbose, 3)
        ns = regrtest._parse_args([])
        self.assertEqual(ns.verbose, 0)

    def test_verbose2(self):
        for opt in '-w', '--verbose2':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.verbose2)

    def test_verbose3(self):
        for opt in '-W', '--verbose3':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.verbose3)

    def test_quiet(self):
        for opt in '-q', '--quiet':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.quiet)
                self.assertEqual(ns.verbose, 0)

    def test_slow(self):
        for opt in '-o', '--slow':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.print_slow)

    def test_header(self):
        ns = regrtest._parse_args(['--header'])
        self.assertTrue(ns.header)

    def test_randomize(self):
        for opt in '-r', '--randomize':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.randomize)

    def test_randseed(self):
        ns = regrtest._parse_args(['--randseed', '12345'])
        self.assertEqual(ns.random_seed, 12345)
        self.assertTrue(ns.randomize)
        self.checkError(['--randseed'], 'expected one argument')
        self.checkError(['--randseed', 'foo'], 'invalid int value')

    def test_fromfile(self):
        for opt in '-f', '--fromfile':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, 'foo'])
                self.assertEqual(ns.fromfile, 'foo')
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo', '-s'], "don't go together")

    def test_exclude(self):
        for opt in '-x', '--exclude':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.exclude)

    def test_single(self):
        for opt in '-s', '--single':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.single)
                self.checkError([opt, '-f', 'foo'], "don't go together")

    def test_match(self):
        for opt in '-m', '--match':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, 'pattern'])
                self.assertEqual(ns.match_tests, 'pattern')
                self.checkError([opt], 'expected one argument')

    def test_failfast(self):
        for opt in '-G', '--failfast':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, '-v'])
                self.assertTrue(ns.failfast)
                ns = regrtest._parse_args([opt, '-W'])
                self.assertTrue(ns.failfast)
                self.checkError([opt], '-G/--failfast needs either -v or -W')

    def test_use(self):
        for opt in '-u', '--use':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, 'gui,network'])
                self.assertEqual(ns.use_resources, ['gui', 'network'])
                ns = regrtest._parse_args([opt, 'gui,none,network'])
                self.assertEqual(ns.use_resources, ['network'])
                expected = list(regrtest.RESOURCE_NAMES)
                expected.remove('gui')
                ns = regrtest._parse_args([opt, 'all,-gui'])
                self.assertEqual(ns.use_resources, expected)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid resource')

    def test_memlimit(self):
        for opt in '-M', '--memlimit':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, '4G'])
                self.assertEqual(ns.memlimit, '4G')
                self.checkError([opt], 'expected one argument')

    def test_testdir(self):
        ns = regrtest._parse_args(['--testdir', 'foo'])
        self.assertEqual(ns.testdir, os.path.join(support.SAVEDCWD, 'foo'))
        self.checkError(['--testdir'], 'expected one argument')

    def test_runleaks(self):
        for opt in '-L', '--runleaks':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.runleaks)

    def test_huntrleaks(self):
        for opt in '-R', '--huntrleaks':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, ':'])
                self.assertEqual(ns.huntrleaks, (5, 4, 'reflog.txt'))
                ns = regrtest._parse_args([opt, '6:'])
                self.assertEqual(ns.huntrleaks, (6, 4, 'reflog.txt'))
                ns = regrtest._parse_args([opt, ':3'])
                self.assertEqual(ns.huntrleaks, (5, 3, 'reflog.txt'))
                ns = regrtest._parse_args([opt, '6:3:leaks.log'])
                self.assertEqual(ns.huntrleaks, (6, 3, 'leaks.log'))
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, '6'],
                                'needs 2 or 3 colon-separated arguments')
                self.checkError([opt, 'foo:'], 'invalid huntrleaks value')
                self.checkError([opt, '6:foo'], 'invalid huntrleaks value')

    def test_multiprocess(self):
        for opt in '-j', '--multiprocess':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, '2'])
                self.assertEqual(ns.use_mp, 2)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid int value')
                self.checkError([opt, '2', '-T'], "don't go together")
                self.checkError([opt, '2', '-l'], "don't go together")
                self.checkError([opt, '2', '-M', '4G'], "don't go together")

    def test_coverage(self):
        for opt in '-T', '--coverage':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.trace)

    def test_coverdir(self):
        for opt in '-D', '--coverdir':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, 'foo'])
                self.assertEqual(ns.coverdir,
                                 os.path.join(support.SAVEDCWD, 'foo'))
                self.checkError([opt], 'expected one argument')

    def test_nocoverdir(self):
        for opt in '-N', '--nocoverdir':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertIsNone(ns.coverdir)

    def test_threshold(self):
        for opt in '-t', '--threshold':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt, '1000'])
                self.assertEqual(ns.threshold, 1000)
                self.checkError([opt], 'expected one argument')
                self.checkError([opt, 'foo'], 'invalid int value')

    def test_nowindows(self):
        for opt in '-n', '--nowindows':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.nowindows)

    def test_forever(self):
        for opt in '-F', '--forever':
            with self.subTest(opt=opt):
                ns = regrtest._parse_args([opt])
                self.assertTrue(ns.forever)


    def test_unrecognized_argument(self):
        self.checkError(['--xxx'], 'usage:')

    def test_long_option__partial(self):
        ns = regrtest._parse_args(['--qui'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)

    def test_two_options(self):
        ns = regrtest._parse_args(['--quiet', '--exclude'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)
        self.assertTrue(ns.exclude)

    def test_option_with_empty_string_value(self):
        ns = regrtest._parse_args(['--start', ''])
        self.assertEqual(ns.start, '')

    def test_arg(self):
        ns = regrtest._parse_args(['foo'])
        self.assertEqual(ns.args, ['foo'])

    def test_option_and_arg(self):
        ns = regrtest._parse_args(['--quiet', 'foo'])
        self.assertTrue(ns.quiet)
        self.assertEqual(ns.verbose, 0)
        self.assertEqual(ns.args, ['foo'])


if __name__ == '__main__':
    unittest.main()
