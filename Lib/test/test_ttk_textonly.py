import os
from test import support

# Skip this test if _tkinter does not exist.
support.import_module('_tkinter')

from tkinter.test import runtktests

def test_main():
    support.run_unittest(
            *runtktests.get_tests(gui=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main()
