import warnings
import os
import sys
import unittest
from test import test_support

import warning_tests

class TestModule(unittest.TestCase):
    def setUp(self):
        self.ignored = [w[2].__name__ for w in warnings.filters
            if w[0]=='ignore' and w[1] is None and w[3] is None]

    def test_warn_default_category(self):
        with test_support.catch_warning() as w:
            for i in range(4):
                text = 'multi %d' %i    # Different text on each call
                warnings.warn(text)
                self.assertEqual(str(w.message), text)
                self.assert_(w.category is UserWarning)

    def test_warn_specific_category(self):
        with test_support.catch_warning() as w:
            text = 'None'
            for category in [DeprecationWarning, FutureWarning,
                        PendingDeprecationWarning, RuntimeWarning,
                        SyntaxWarning, UserWarning, Warning]:
                if category.__name__ in self.ignored:
                    text = 'filtered out' + category.__name__
                    warnings.warn(text, category)
                    self.assertNotEqual(w.message, text)
                else:
                    text = 'unfiltered %s' % category.__name__
                    warnings.warn(text, category)
                    self.assertEqual(str(w.message), text)
                    self.assert_(w.category is category)

    def test_filtering(self):
        with test_support.catch_warning() as w:
            warnings.filterwarnings("error", "", Warning, "", 0)
            self.assertRaises(UserWarning, warnings.warn, 'convert to error')

            warnings.resetwarnings()
            text = 'handle normally'
            warnings.warn(text)
            self.assertEqual(str(w.message), text)
            self.assert_(w.category is UserWarning)

            warnings.filterwarnings("ignore", "", Warning, "", 0)
            text = 'filtered out'
            warnings.warn(text)
            self.assertNotEqual(str(w.message), text)

            warnings.resetwarnings()
            warnings.filterwarnings("error", "hex*", Warning, "", 0)
            self.assertRaises(UserWarning, warnings.warn, 'hex/oct')
            text = 'nonmatching text'
            warnings.warn(text)
            self.assertEqual(str(w.message), text)
            self.assert_(w.category is UserWarning)

    def test_options(self):
        # Uses the private _setoption() function to test the parsing
        # of command-line warning arguments
        with test_support.catch_warning():
            self.assertRaises(warnings._OptionError,
                              warnings._setoption, '1:2:3:4:5:6')
            self.assertRaises(warnings._OptionError,
                              warnings._setoption, 'bogus::Warning')
            self.assertRaises(warnings._OptionError,
                              warnings._setoption, 'ignore:2::4:-5')
            warnings._setoption('error::Warning::0')
            self.assertRaises(UserWarning, warnings.warn, 'convert to error')

    def test_filename(self):
        with test_support.catch_warning() as w:
            warning_tests.inner("spam1")
            self.assertEqual(os.path.basename(w.filename), "warning_tests.py")
            warning_tests.outer("spam2")
            self.assertEqual(os.path.basename(w.filename), "warning_tests.py")

    def test_stacklevel(self):
        # Test stacklevel argument
        # make sure all messages are different, so the warning won't be skipped
        with test_support.catch_warning() as w:
            warning_tests.inner("spam3", stacklevel=1)
            self.assertEqual(os.path.basename(w.filename), "warning_tests.py")
            warning_tests.outer("spam4", stacklevel=1)
            self.assertEqual(os.path.basename(w.filename), "warning_tests.py")

            warning_tests.inner("spam5", stacklevel=2)
            self.assertEqual(os.path.basename(w.filename), "test_warnings.py")
            warning_tests.outer("spam6", stacklevel=2)
            self.assertEqual(os.path.basename(w.filename), "warning_tests.py")

            warning_tests.inner("spam7", stacklevel=9999)
            self.assertEqual(os.path.basename(w.filename), "sys")


def test_main(verbose=None):
    # Obscure hack so that this test passes after reloads or repeated calls
    # to test_main (regrtest -R).
    if '__warningregistry__' in globals():
        del globals()['__warningregistry__']
    if hasattr(warning_tests, '__warningregistry__'):
        del warning_tests.__warningregistry__
    if hasattr(sys, '__warningregistry__'):
        del sys.__warningregistry__
    test_support.run_unittest(TestModule)

if __name__ == "__main__":
    test_main(verbose=True)
