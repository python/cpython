"""Tests for distutils.fancy_getopt."""
import unittest
from test.support import run_unittest
from distutils.fancy_getopt import FancyGetopt


class FancyGetoptTestCase(unittest.TestCase):

    def _get_fancy_getopt_instance(self, option_table):
        return FancyGetopt(option_table)

    def test_generate_help_with_none_help_text(self):
        instance = self._get_fancy_getopt_instance([('long', 'l', None)])
        help_text = instance.generate_help()
        self.assertEqual(help_text, ['Option summary:', '  --long (-l)'])

    def test_generate_help_with_empty_help_text(self):
        instance = self._get_fancy_getopt_instance([('long', 'l', '')])
        help_text = instance.generate_help()
        self.assertEqual(help_text, ['Option summary:', '  --long (-l)'])


def test_suite():
    return unittest.makeSuite(FancyGetoptTestCase)


if __name__ == "__main__":
    run_unittest(test_suite())
