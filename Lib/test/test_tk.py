import tkinter
from tkinter.test import runtktests
from test import support
import unittest

try:
    tkinter.Button()
except tkinter.TclError as msg:
    # assuming tk is not available
    raise unittest.SkipTest("tk not available: %s" % msg)

def test_main(enable_gui=False):
    if enable_gui:
        if support.use_resources is None:
            support.use_resources = ['gui']
        elif 'gui' not in support.use_resources:
            support.use_resources.append('gui')

    support.run_unittest(
            *runtktests.get_tests(text=False, packages=['test_tkinter']))

if __name__ == '__main__':
    test_main(enable_gui=True)
