from test import support
# Skip test if _tkinter wasn't built.
support.import_module('_tkinter')

# Skip test if tk cannot be initialized.
support.requires('gui')

def load_tests(loader, tests, pattern):
    return loader.discover('tkinter.test.test_tkinter')


if __name__ == '__main__':
    unittest.main()
