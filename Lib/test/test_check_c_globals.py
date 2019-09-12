import unittest
import test.test_tools

test.test_tools.skip_if_missing('c-analyzer')
with test.test_tools.imports_under_tool('c-analyzer'):
    from c_globals.__main__ import main


class ActualChecks(unittest.TestCase):

    # XXX Also run the check in "make check".
    #@unittest.expectedFailure
    # Failing on one of the buildbots (see https://bugs.python.org/issue36876).
    @unittest.skip('activate this once all the globals have been resolved')
    def test_check_c_globals(self):
        try:
            main('check', {})
        except NotImplementedError:
            raise unittest.SkipTest('not supported on this host')


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
