"""
Tests of regrtest.py.
"""

import argparse
import getopt
import unittest
from test import regrtest, support

def old_parse_args(args):
    """Parse arguments as regrtest did strictly prior to 3.4.

    Raises getopt.GetoptError on bad arguments.
    """
    return getopt.getopt(args, 'hvqxsoS:rf:lu:t:TD:NLR:FdwWM:nj:Gm:',
        ['help', 'verbose', 'verbose2', 'verbose3', 'quiet',
         'exclude', 'single', 'slow', 'randomize', 'fromfile=', 'findleaks',
         'use=', 'threshold=', 'coverdir=', 'nocoverdir',
         'runleaks', 'huntrleaks=', 'memlimit=', 'randseed=',
         'multiprocess=', 'coverage', 'slaveargs=', 'forever', 'debug',
         'start=', 'nowindows', 'header', 'testdir=', 'timeout=', 'wait',
         'failfast', 'match='])

class ParseArgsTestCase(unittest.TestCase):

    """Test that regrtest's parsing code matches the prior getopt behavior."""

    def _parse_args(self, args):
        # This is the same logic as that used in regrtest.main()
        parser = regrtest._create_parser()
        ns = parser.parse_args(args=args)
        opts = regrtest._convert_namespace_to_getopt(ns)
        return opts, ns.args

    def _check_args(self, args, expected=None):
        """
        The expected parameter is for cases when the behavior of the new
        parse_args differs from the old (but deliberately so).
        """
        if expected is None:
            try:
                expected = old_parse_args(args)
            except getopt.GetoptError:
                # Suppress usage string output when an argparse.ArgumentError
                # error is raised.
                with support.captured_stderr():
                    self.assertRaises(SystemExit, self._parse_args, args)
                return
        # The new parse_args() sorts by long option string.
        expected[0].sort()
        actual = self._parse_args(args)
        self.assertEqual(actual, expected)

    def test_unrecognized_argument(self):
        self._check_args(['--xxx'])

    def test_value_not_provided(self):
        self._check_args(['--start'])

    def test_short_option(self):
        # getopt returns the short option whereas argparse returns the long.
        expected = ([('--quiet', '')], [])
        self._check_args(['-q'], expected=expected)

    def test_long_option(self):
        self._check_args(['--quiet'])

    def test_long_option__partial(self):
        self._check_args(['--qui'])

    def test_two_options(self):
        self._check_args(['--quiet', '--exclude'])

    def test_option_with_value(self):
        self._check_args(['--start', 'foo'])

    def test_option_with_empty_string_value(self):
        self._check_args(['--start', ''])

    def test_arg(self):
        self._check_args(['foo'])

    def test_option_and_arg(self):
        self._check_args(['--quiet', 'foo'])

    def test_fromfile(self):
        self._check_args(['--fromfile', 'file'])

    def test_match(self):
        self._check_args(['--match', 'pattern'])

    def test_randomize(self):
        self._check_args(['--randomize'])


def test_main():
    support.run_unittest(__name__)

if __name__ == '__main__':
    test_main()
