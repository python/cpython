import unittest
import tkinter
from test.support import requires, swap_attr
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractDefaultRootTest
from tkinter.simpledialog import SimpleDialog, Dialog, askinteger, askfloat

requires('gui')


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_ask(self):
        @staticmethod
        def mock_wait_window(w):
            nonlocal ismapped
            ismapped = w.master.winfo_ismapped()
            w.destroy()

        with swap_attr(Dialog, 'wait_window', mock_wait_window):
            ismapped = None

            askinteger("Go To Line", "Line number")
            self.assertEqual(ismapped, False)
            askfloat("Float", "Enter a float")
            self.assertEqual(ismapped, False)

            root = tkinter.Tk()
            ismapped = None

            askinteger("Go To Line", "Line number")
            self.assertEqual(ismapped, True)
            askfloat("Float", "Enter a float")
            self.assertEqual(ismapped, True)

            root.destroy()
            tkinter.NoDefaultRoot()

            self.assertRaises(RuntimeError, askinteger, "Go To Line", "Line number")
            self.assertRaises(RuntimeError, askfloat, "Float", "Enter a float")

    def test_simpledialog(self):
        root = tkinter.Tk()

        dialog = SimpleDialog(
            root,
            title="title",
            text="text",
            buttons=["b1", "b2", "b3"]
        )

        root.update()
        root.destroy()

if __name__ == "__main__":
    unittest.main()
