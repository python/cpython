import unittest
import tkinter
from test.support import requires, run_unittest, swap_attr
from tkinter.test.support import AbstractDefaultRootTest
from tkinter.commondialog import Dialog
from tkinter.colorchooser import askcolor

requires('gui')


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_askcolor(self):
        def test_callback(dialog, master):
            nonlocal ismapped
            master.update()
            ismapped = master.winfo_ismapped()
            raise ZeroDivisionError

        with swap_attr(Dialog, '_test_callback', test_callback):
            ismapped = None
            self.assertRaises(ZeroDivisionError, askcolor)
            #askcolor()
            self.assertEqual(ismapped, False)

            root = tkinter.Tk()
            ismapped = None
            self.assertRaises(ZeroDivisionError, askcolor)
            self.assertEqual(ismapped, True)
            root.destroy()

            tkinter.NoDefaultRoot()
            self.assertRaises(RuntimeError, askcolor)


tests_gui = (DefaultRootTest,)

if __name__ == "__main__":
    run_unittest(*tests_gui)
