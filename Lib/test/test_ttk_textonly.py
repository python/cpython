import os
import sys
from test import test_support

# Skip this test if _tkinter does not exist.
test_support.import_module('_tkinter')

this_dir = os.path.dirname(os.path.abspath(__file__))
lib_tk_test = os.path.abspath(os.path.join(this_dir, '..', 'lib-tk', 'test'))
if lib_tk_test not in sys.path:
    sys.path.append(lib_tk_test)

import runtktests

def test_main():
    test_support.run_unittest(
            *runtktests.get_tests(gui=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main()
