from test_support import verbose
import doctest, difflib
doctest.testmod(difflib, verbose=verbose)
