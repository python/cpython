from test import support
from test.support import import_helper
# Skip test if _tkinter wasn't built.
import_helper.import_module('_tkinter')

# Skip test if tk cannot be initialized.
support.requires('gui')

def load_tests(loader, tests, pattern):
    return loader.discover('tkinter.test.test_tkinter')


if __name__ == '__main__':
    unittest.main()
