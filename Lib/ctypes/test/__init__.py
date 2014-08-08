import os
import unittest
from test import support

# skip tests if _ctypes was not built
ctypes = support.import_module('ctypes')
ctypes_symbols = dir(ctypes)

def need_symbol(name):
    return unittest.skipUnless(name in ctypes_symbols,
                               '{!r} is required'.format(name))

def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
