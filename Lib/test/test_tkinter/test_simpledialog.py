import unittest
import tkinter
from tkinter import messagebox
from test.support import requires, swap_attr
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractDefaultRootTest, AbstractTkTest
from tkinter.simpledialog import (Dialog, SimpleDialog,
                                  askinteger, askfloat, askstring,
                                  _QueryInteger, _QueryFloat, _QueryString)

requires('gui')


class SimpleDialogTest(AbstractTkTest, unittest.TestCase):
    # SimpleDialog's modal loop is in go(); its bindings are exercised here by
    # generating events on the constructed dialog, without entering the loop.

    def create(self, **kw):
        kw.setdefault('text', 'Question?')
        kw.setdefault('buttons', ['Yes', 'No'])
        kw.setdefault('default', 0)
        kw.setdefault('cancel', 1)
        d = SimpleDialog(self.root, **kw)
        self.addCleanup(lambda: d.root.winfo_exists() and d.root.destroy())
        return d

    def test_message(self):
        # The text is shown in a message widget.
        d = self.create(text='Hello?')
        self.assertEqual(d.message.winfo_class(), 'Message')
        self.assertEqual(str(d.message.cget('text')), 'Hello?')

    def test_class_name(self):
        # class_ sets the Tk class of the dialog window.
        d = self.create(class_='MyDialog')
        self.assertEqual(d.root.winfo_class(), 'MyDialog')

    def test_button(self):
        # Pressing a button records its index.
        d = self.create(buttons=['Yes', 'No'])
        d.frame.winfo_children()[1].invoke()  # "No"
        self.assertEqual(d.num, 1)

    def test_default_button(self):
        # The default button is drawn with a raised border.
        d = self.create(buttons=['Yes', 'No'], default=0)
        self.assertEqual(str(d.frame.winfo_children()[0].cget('relief')), 'ridge')

    def test_return_activates_default(self):
        # <Return> invokes the default button.
        d = self.create()  # default 0
        self.require_mapped(d.root)
        d.root.focus_force()
        d.root.update()
        d.root.event_generate('<Return>')
        d.root.update()
        self.assertEqual(d.num, 0)

    def test_return_no_default(self):
        # With no default button, <Return> rings the bell and leaves the dialog
        # open instead of activating a button.
        d = self.create(default=None)
        self.require_mapped(d.root)
        d.root.focus_force()
        d.root.update()
        bells = []
        with swap_attr(d.root, 'bell', lambda *a, **k: bells.append(True)):
            d.root.event_generate('<Return>')
            d.root.update()
        self.assertTrue(bells)  # rang the bell
        self.assertIsNone(d.num)
        self.assertTrue(d.root.winfo_exists())

    def test_wm_delete_cancels(self):
        # Closing the window through the window manager records the cancel index.
        d = self.create()  # cancel 1
        d.wm_delete_window()
        self.assertEqual(d.num, 1)

    def test_wm_delete_no_cancel(self):
        # With no cancel index, closing the window through the window manager
        # rings the bell and leaves the dialog open instead of recording an
        # index.
        d = self.create(default=None, cancel=None)
        d.root.update()
        bells = []
        with swap_attr(d.root, 'bell', lambda *a, **k: bells.append(True)):
            d.wm_delete_window()
            d.root.update()
        self.assertTrue(bells)  # rang the bell
        self.assertIsNone(d.num)
        self.assertTrue(d.root.winfo_exists())

    def test_go(self):
        # go() runs the modal loop and returns the chosen button's index.
        d = self.create()
        d.root.after(1, lambda: d.done(0))
        self.assertEqual(d.go(), 0)


class DialogTest(AbstractTkTest, unittest.TestCase):
    # Dialog is a base class for custom dialogs; exercise it via _QueryInteger.

    def open(self, **kw):
        with swap_attr(Dialog, 'wait_window', staticmethod(lambda w: None)):
            d = _QueryInteger('Title', 'Prompt', parent=self.root, **kw)
        self.addCleanup(lambda: d.winfo_exists() and d.destroy())
        return d

    def buttons(self, d):
        # Map the button box's buttons by their label.
        return {str(b.cget('text')): b
                for frame in d.winfo_children()
                for b in frame.winfo_children()
                if b.winfo_class() == 'Button'}

    def test_background(self):
        # The classic dialog keeps the default Toplevel background.
        d = self.open()
        ref = tkinter.Toplevel(self.root)
        self.addCleanup(ref.destroy)
        self.assertEqual(str(d.cget('background')), str(ref.cget('background')))

    def test_buttons(self):
        # The button box has OK (the default) and Cancel buttons.
        buttons = self.buttons(self.open())
        self.assertEqual(set(buttons), {'OK', 'Cancel'})
        self.assertEqual(str(buttons['OK'].cget('default')), 'active')

    def test_ok(self):
        # The OK button validates the entry and stores the result.
        d = self.open()
        d.entry.insert(0, '42')
        self.buttons(d)['OK'].invoke()
        self.assertEqual(d.result, 42)
        self.assertFalse(d.winfo_exists())  # The dialog closed.

    def test_cancel(self):
        # The Cancel button closes the dialog without a result.
        d = self.open()
        self.buttons(d)['Cancel'].invoke()
        self.assertIsNone(d.result)
        self.assertFalse(d.winfo_exists())


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

    def test_initialvalue(self):
        # The entry is pre-filled with the initial value, which is accepted.
        d = self.open(_QueryInteger, initialvalue=42)
        self.assertEqual(d.entry.get(), '42')
        d.event_generate('<Return>')
        d.update()
        self.assertEqual(d.result, 42)

    def test_show(self):
        # _QueryString hides the entered text when show is given.
        d = self.open(_QueryString, show='*')
        self.assertEqual(str(d.entry.cget('show')), '*')

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

    def test_boundary_values_accepted(self):
        # The min/max checks are inclusive: a value equal to a bound passes.
        d = self.open(_QueryInteger, minvalue=10, maxvalue=20)
        self.enter(d, '10')   # Exactly the minimum.
        self.assertEqual(d.result, 10)
        self.assertFalse(d.winfo_exists())

        d = self.open(_QueryInteger, minvalue=10, maxvalue=20)
        self.enter(d, '20')   # Exactly the maximum.
        self.assertEqual(d.result, 20)
        self.assertFalse(d.winfo_exists())

    def run_ask(self, ask, value, **kw):
        # Drive a modal ask* function: enter a value and accept it.
        def accept(d):
            d.entry.delete(0, 'end')
            d.entry.insert(0, value)
            d.ok()
        with swap_attr(Dialog, 'wait_window', staticmethod(accept)):
            return ask('Title', 'Prompt', parent=self.root, **kw)

    def test_ask_functions(self):
        self.assertEqual(self.run_ask(askinteger, '42'), 42)
        self.assertEqual(self.run_ask(askfloat, '1.5'), 1.5)
        self.assertEqual(self.run_ask(askstring, 'spam'), 'spam')

    def test_ask_cancelled(self):
        # A cancelled ask* returns None.
        with swap_attr(Dialog, 'wait_window', staticmethod(lambda d: d.cancel())):
            self.assertIsNone(askstring('Title', 'Prompt', parent=self.root))


if __name__ == "__main__":
    unittest.main()
