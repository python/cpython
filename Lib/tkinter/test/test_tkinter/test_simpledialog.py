import unittest
import tkinter
from test.support import requires, swap_attr
from tkinter.test.support import AbstractDefaultRootTest
from tkinter.simpledialog import Dialog, askinteger

requires('gui')


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_askinteger(self):
        @staticmethod
        def mock_wait_window(w):
            nonlocal ismapped
            ismapped = w.master.winfo_ismapped()
            w.destroy()

        with swap_attr(Dialog, 'wait_window', mock_wait_window):
            ismapped = None
            askinteger("Go To Line", "Line number")
            self.assertEqual(ismapped, False)

            root = tkinter.Tk()
            ismapped = None
            askinteger("Go To Line", "Line number")
            self.assertEqual(ismapped, True)
            root.destroy()

            tkinter.NoDefaultRoot()
            self.assertRaises(RuntimeError, askinteger, "Go To Line", "Line number")


if __name__ == "__main__":
    unittest.main()
