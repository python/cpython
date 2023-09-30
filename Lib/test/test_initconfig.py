import textwrap
import unittest
from test.support import import_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')


class InitConfigTests(unittest.TestCase):
    def check_parse(self, text, expected):
        text = textwrap.dedent(text)
        config = _testinternalcapi.parse_config_text(text)
        self.assertEqual(config, expected)

    def test_parse(self):
        self.check_parse('verbose=1', {'verbose': 1})

        # strip spaces
        self.check_parse(' \tverbose = 1 ', {'verbose': 1})

        # comments
        self.check_parse('''
            verbose=1
            # ignored comment
            verbose=2 # more #hashtag comment
        ''', dict(
            verbose = 2,
        ))
        self.check_parse('verbose=1', {'verbose': 1})

        # types
        self.check_parse('''
            # int
            int_max_str_digits = -123

            # uint
            isolated = 0

            # ulong
            hash_seed = 555

            # wstr
            filesystem_encoding = 'with spaces'
            filesystem_errors = "double quotes"
            program_name = "key=value"

            # wstr optional
            pycache_prefix =

            # == wstr list ==

            argv = ['python', '-c', 'pass']

            # accept spaces before/after strings and commas
            orig_argv = [ 'python' , '-c' , 'accept spaces, and commas' ]

            # empty list
            xoptions = []
        ''', dict(
            int_max_str_digits = -123,
            isolated = 0,
            hash_seed = 555,
            filesystem_encoding = "with spaces",
            filesystem_errors = "double quotes",
            program_name = "key=value",
            pycache_prefix = None,
            argv = ['python', '-c', 'pass'],
            orig_argv = ['python', '-c', 'accept spaces, and commas'],
            xoptions = [],
        ))

    def check_error(self, text):
        text = textwrap.dedent(text)
        with self.assertRaises(ValueError):
            _testinternalcapi.parse_config_text(text)

    def test_parse_invalid(self):
        # no value
        self.check_error('isolated=')
        # no key
        self.check_error('= 123')


if __name__ == "__main__":
    unittest.main()
