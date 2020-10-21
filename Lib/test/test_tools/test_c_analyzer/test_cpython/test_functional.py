import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    pass


class SelfCheckTests(unittest.TestCase):

    @unittest.expectedFailure
    def test_known(self):
        # Make sure known macros & vartypes aren't hiding unknown local types.
        # XXX finish!
        raise NotImplementedError

    @unittest.expectedFailure
    def test_compare_nm_results(self):
        # Make sure the "show" results match the statics found by "nm" command.
        # XXX Skip if "nm" is not available.
        # XXX finish!
        raise NotImplementedError


class DummySourceTests(unittest.TestCase):

    @unittest.expectedFailure
    def test_check(self):
        # XXX finish!
        raise NotImplementedError

    @unittest.expectedFailure
    def test_show(self):
        # XXX finish!
        raise NotImplementedError
