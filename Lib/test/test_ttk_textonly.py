import os
import sys
from test import support
from tkinter.test import runtktests

def test_main():
    support.run_unittest(
            *runtktests.get_tests(gui=False, packages=['test_ttk']))

if __name__ == '__main__':
    test_main()
