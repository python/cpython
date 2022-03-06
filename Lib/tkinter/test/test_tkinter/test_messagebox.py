import unittest
import tkinter
from test.support import requires, run_unittest, swap_attr
from tkinter.test.support import AbstractDefaultRootTest
from tkinter.commondialog import Dialog
from tkinter.messagebox import showinfo

requires('gui')


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_showinfo(self):
        def test_callback(dialog, master):
            nonlocal ismapped
            master.update()
            ismapped = master.winfo_ismapped()
            raise ZeroDivisionError

        with swap_attr(Dialog, '_test_callback', test_callback):
            ismapped = None
            self.assertRaises(ZeroDivisionError, showinfo, "Spam", "Egg Information")
            self.assertEqual(ismapped, False)

            root = tkinter.Tk()
            ismapped = None
            self.assertRaises(ZeroDivisionError, showinfo, "Spam", "Egg Information")
            self.assertEqual(ismapped, True)
            root.destroy()

            tkinter.NoDefaultRoot()
            self.assertRaises(RuntimeError, showinfo, "Spam", "Egg Information")


tests_gui = (DefaultRootTest,)

if __name__ == "__main__":
    run_unittest(*tests_gui)
