"""Tests for distutils.spawn."""
import unittest
from distutils.spawn import _nt_quote_args

class SpawnTestCase(unittest.TestCase):

    def test_nt_quote_args(self):

        for (args, wanted) in ((['with space', 'nospace'],
                                ['"with space"', 'nospace']),
                               (['nochange', 'nospace'],
                                ['nochange', 'nospace'])):
            res = _nt_quote_args(args)
            self.assertEquals(res, wanted)

def test_suite():
    return unittest.makeSuite(SpawnTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
