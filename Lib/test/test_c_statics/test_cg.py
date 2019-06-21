import unittest

from .cg.__main__ import parse_args, main


class ParseArgsTests(unittest.TestCase):

    def test_no_args(self):
        cmd, cmdkwargs = parse_args('cg', [])

        self.assertIs(cmd, None)
        self.assertEqual(cmdkwargs, {})


class MainTests(unittest.TestCase):

    def test_no_command(self):
        main(None, {})
