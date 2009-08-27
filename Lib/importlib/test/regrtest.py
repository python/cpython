"""Run Python's standard test suite using importlib.__import__.

XXX FAILING
    test___all__  # tuple being given for fromlist (looks like interpreter is
                    doing it)
    test_builtin  # Wanting a TypeError for an integer name
    test_import  # execution bit, exception name differing, file name differing
                    between code and module (?)
    test_importhooks  # package not set in _gcd_import() but level > 0
    test_pep3120  # Difference in exception
    test_runpy  # Importing sys.imp.eric raises AttributeError instead of
                    ImportError (as does any attempt to import a sub-module
                    from a non-package, e.g. tokenize.menotreal)

"""
import importlib
import sys
from test import regrtest

if __name__ == '__main__':
    __builtins__.__import__ = importlib.__import__

    exclude = ['--exclude',
                'test_frozen', # Does not expect __loader__ attribute
                'test_pkg',  # Does not expect __loader__ attribute
                'test_pydoc', # Does not expect __loader__ attribute
              ]
    # No programmatic way to specify tests to exclude
    sys.argv.extend(exclude)

    # verbose=True, quiet=False for all failure info
    # tests=[...] for specific tests to run
    regrtest.main(quiet=True)
