import unittest
import tkinter
from tkinter import messagebox
from test.support import requires, swap_attr
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractDefaultRootTest, AbstractTkTest
from tkinter.simpledialog import (Dialog, askinteger,
                                  _QueryInteger, _QueryFloat, _QueryString)

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


class QueryDialogTest(AbstractTkTest, unittest.TestCase):
    # The query dialogs are modal: their __init__ blocks in wait_window().
    # Mock that out so the dialog stays alive and can be driven with generated
    # events, exercising the <Return>/<Escape> bindings and the validation.

    def open(self, query, **kw):
        with swap_attr(Dialog, 'wait_window', staticmethod(lambda w: None)):
            d = query("Title", "Prompt", parent=self.root, **kw)
        self.addCleanup(lambda: d.winfo_exists() and d.destroy())
        d.focus_force()
        d.update()
        return d

    def enter(self, d, value, key='<Return>'):
        d.entry.delete(0, 'end')
        d.entry.insert(0, value)
        d.event_generate(key)
        d.update()

    def test_return_accepts(self):
        for query, value, expected in [
            (_QueryInteger, '42', 42),
            (_QueryFloat, '1.5', 1.5),
            (_QueryString, 'spam', 'spam'),
        ]:
            with self.subTest(query=query.__name__):
                d = self.open(query)
                self.enter(d, value)
                self.assertEqual(d.result, expected)
                self.assertFalse(d.winfo_exists())  # The dialog closed.

    def test_escape_cancels(self):
        d = self.open(_QueryString)
        self.enter(d, 'spam', '<Escape>')
        self.assertIsNone(d.result)
        self.assertFalse(d.winfo_exists())

    def test_invalid_value(self):
        warnings = []
        d = self.open(_QueryInteger)
        with swap_attr(messagebox, 'showwarning',
                       lambda *a, **k: warnings.append(a)):
            self.enter(d, 'not a number')
        self.assertIsNone(d.result)
        self.assertTrue(d.winfo_exists())  # The dialog stays open.
        self.assertTrue(warnings)

    def test_out_of_range(self):
        warnings = []
        d = self.open(_QueryInteger, minvalue=10, maxvalue=20)
        with swap_attr(messagebox, 'showwarning',
                       lambda *a, **k: warnings.append(a)):
            self.enter(d, '5')   # Below the minimum.
            self.assertIsNone(d.result)
            self.enter(d, '25')  # Above the maximum.
            self.assertIsNone(d.result)
        self.assertTrue(d.winfo_exists())
        self.assertEqual(len(warnings), 2)


if __name__ == "__main__":
    unittest.main()
