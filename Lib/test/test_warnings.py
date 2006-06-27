import warnings
import os
import unittest
from test import test_support

# The warnings module isn't easily tested, because it relies on module
# globals to store configuration information.  setUp() and tearDown()
# preserve the current settings to avoid bashing them while running tests.

# To capture the warning messages, a replacement for showwarning() is
# used to save warning information in a global variable.

class WarningMessage:
    "Holds results of latest showwarning() call"
    pass

def showwarning(message, category, filename, lineno, file=None):
    msg.message = str(message)
    msg.category = category.__name__
    msg.filename = os.path.basename(filename)
    msg.lineno = lineno

class TestModule(unittest.TestCase):

    def setUp(self):
        global msg
        msg = WarningMessage()
        self._filters = warnings.filters[:]
        self._showwarning = warnings.showwarning
        warnings.showwarning = showwarning
        self.ignored = [w[2].__name__ for w in self._filters
            if w[0]=='ignore' and w[1] is None and w[3] is None]

    def tearDown(self):
        warnings.filters = self._filters[:]
        warnings.showwarning = self._showwarning

    def test_warn_default_category(self):
        for i in range(4):
            text = 'multi %d' %i    # Different text on each call
            warnings.warn(text)
            self.assertEqual(msg.message, text)
            self.assertEqual(msg.category, 'UserWarning')

    def test_warn_specific_category(self):
        text = 'None'
        for category in [DeprecationWarning, FutureWarning,
                    PendingDeprecationWarning, RuntimeWarning,
                    SyntaxWarning, UserWarning, Warning]:
            if category.__name__ in self.ignored:
                text = 'filtered out' + category.__name__
                warnings.warn(text, category)
                self.assertNotEqual(msg.message, text)
            else:
                text = 'unfiltered %s' % category.__name__
                warnings.warn(text, category)
                self.assertEqual(msg.message, text)
                self.assertEqual(msg.category, category.__name__)

    def test_filtering(self):

        warnings.filterwarnings("error", "", Warning, "", 0)
        self.assertRaises(UserWarning, warnings.warn, 'convert to error')

        warnings.resetwarnings()
        text = 'handle normally'
        warnings.warn(text)
        self.assertEqual(msg.message, text)
        self.assertEqual(msg.category, 'UserWarning')

        warnings.filterwarnings("ignore", "", Warning, "", 0)
        text = 'filtered out'
        warnings.warn(text)
        self.assertNotEqual(msg.message, text)

        warnings.resetwarnings()
        warnings.filterwarnings("error", "hex*", Warning, "", 0)
        self.assertRaises(UserWarning, warnings.warn, 'hex/oct')
        text = 'nonmatching text'
        warnings.warn(text)
        self.assertEqual(msg.message, text)
        self.assertEqual(msg.category, 'UserWarning')

    def test_options(self):
        # Uses the private _setoption() function to test the parsing
        # of command-line warning arguments
        self.assertRaises(warnings._OptionError,
                          warnings._setoption, '1:2:3:4:5:6')
        self.assertRaises(warnings._OptionError,
                          warnings._setoption, 'bogus::Warning')
        self.assertRaises(warnings._OptionError,
                          warnings._setoption, 'ignore:2::4:-5')
        warnings._setoption('error::Warning::0')
        self.assertRaises(UserWarning, warnings.warn, 'convert to error')


def test_main(verbose=None):
    # Obscure hack so that this test passes after reloads or repeated calls
    # to test_main (regrtest -R).
    if '__warningregistry__' in globals():
        del globals()['__warningregistry__']
    test_support.run_unittest(TestModule)

if __name__ == "__main__":
    test_main(verbose=True)
