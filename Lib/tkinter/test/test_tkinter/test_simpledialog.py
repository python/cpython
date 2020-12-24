import unittest
import tkinter
from test.support import requires, run_unittest, swap_attr
from tkinter.test.support import AbstractDefaultRootTest
from tkinter.simpledialog import Dialog, askinteger

requires('gui')


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_askinteger(self):
        self.assertRaises(RuntimeError, askinteger, "Go To Line", "Line number")
        root = tkinter.Tk()
        with swap_attr(Dialog, 'wait_window', lambda self, w: w.destroy()):
            askinteger("Go To Line", "Line number")
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, askinteger, "Go To Line", "Line number")


tests_gui = (DefaultRootTest,)

if __name__ == "__main__":
    run_unittest(*tests_gui)
