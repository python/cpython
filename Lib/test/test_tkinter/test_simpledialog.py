import unittest
import tkinter
from tkinter import messagebox, ttk
from test.support import requires, swap_attr
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractDefaultRootTest, AbstractTkTest
from tkinter.simpledialog import (Dialog, SimpleDialog,
                                  askinteger, askfloat, askstring,
                                  _QueryInteger, _QueryFloat, _QueryString,
                                  _underline_ampersand, _find_alt_key_target)

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

    # --- Widget set and appearance ---

    def test_use_ttk(self):
        # By default SimpleDialog uses the themed (ttk) widgets (tk::MessageBox).
        d = self.create(buttons=['OK'], bitmap='warning')
        self.assertEqual(d._buttons[0].winfo_class(), 'TButton')
        self.assertEqual(d.message.winfo_class(), 'TLabel')
        self.assertEqual(str(d.message.cget('anchor')), 'nw')  # cf. MessageBox
        # The standard icons are drawn with themed images (cf. MessageBox).
        self.assertEqual(d.bitmap.winfo_class(), 'TLabel')
        self.assertIn('::tk::icons::warning', str(d.bitmap.cget('image')))
        # tk::MessageBox makes the buttons equal width.
        self.assertEqual(
            str(d.root.children['bot'].grid_columnconfigure(0)['uniform']),
            'buttons')
        # The dialog uses the themed background colour (cf. MessageBox).
        self.assertEqual(str(d.root.cget('background')),
                         ttk.Style(d.root).lookup('.', 'background'))
        # The bindings work with the themed buttons too.
        self.require_mapped(d.root)
        d._buttons[0].focus_force()
        d.root.update()
        d.root.event_generate('<Return>')
        d.root.update()
        self.assertEqual(d.num, 0)

    def test_use_classic(self):
        # use_ttk=False uses the classic Tk widgets, modelled on tk_dialog.
        d = self.create(buttons=['OK'], bitmap='warning', use_ttk=False)
        self.assertEqual(d._buttons[0].winfo_class(), 'Button')
        self.assertEqual(d.message.winfo_class(), 'Label')
        if d.root._windowingsystem == 'x11':
            self.assertEqual(str(d.frame.cget('relief')), 'raised')
        # tk_dialog does not make the buttons equal width.
        self.assertIsNone(d.root.children['bot'].grid_columnconfigure(0)['uniform'])
        # The bitmap is a classic monochrome label.
        self.assertEqual(str(d.bitmap.cget('bitmap')), 'warning')

    def test_class_name(self):
        # class_ sets the Tk class of the dialog window (default 'Dialog').
        self.assertEqual(self.create().root.winfo_class(), 'Dialog')
        d = self.create(class_='MyDialog')
        self.assertEqual(d.root.winfo_class(), 'MyDialog')

    # --- Message, detail and bitmap content ---

    def test_no_detail(self):
        # Without a detail message the message label expands.
        d = self.create()
        self.assertIsNone(d.detail)
        self.assertEqual(int(d.message.pack_info()['expand']), 1)

    def test_detail(self):
        # The detail message is shown below the main message.
        d = self.create(detail='More information.')
        self.assertEqual(d.detail.winfo_class(), 'TLabel')
        self.assertEqual(str(d.detail.cget('text')), 'More information.')
        self.assertEqual(str(d.detail.cget('anchor')), 'nw')  # cf. MessageBox
        # With a detail message it expands and the main message does not.
        self.assertEqual(int(d.message.pack_info()['expand']), 0)
        self.assertEqual(int(d.detail.pack_info()['expand']), 1)

    def test_bitmap_fallback(self):
        # A non-standard bitmap has no themed image, so even the ttk version
        # falls back to a classic bitmap label.
        d = self.create(buttons=['OK'], bitmap='questhead')
        self.assertEqual(d.bitmap.winfo_class(), 'Label')
        self.assertEqual(str(d.bitmap.cget('bitmap')), 'questhead')

    def test_bitmap_detail_layout(self):
        # The bitmap is packed first to claim the whole left side; the message
        # and detail stack on its right, as in tk::MessageBox.
        d = self.create(buttons=['OK'], bitmap='warning', detail='More.')
        top = d.root.children['top']
        layout = [(w.winfo_name(), str(w.pack_info()['side']))
                  for w in top.pack_slaves()]
        self.assertEqual(layout,
                         [('bitmap', 'left'), ('msg', 'top'), ('dtl', 'top')])

    # --- Buttons and keyboard ---

    def test_button_options(self):
        # A button entry can be a mapping of options, not just a label (like
        # the "[name ?-option value ...?]" button specs in tk::MessageBox).
        d = self.create(buttons=['Yes',
                                 {'text': 'No', 'underline': 0, 'width': 12}])
        yes, no = d._buttons
        self.assertEqual(str(yes.cget('text')), 'Yes')
        self.assertEqual(str(no.cget('text')), 'No')
        self.assertEqual(int(no.cget('underline')), 0)
        self.assertEqual(str(no.cget('width')), '12')
        # The dialog still controls the default ring (default=0) ...
        self.assertEqual(str(yes.cget('default')), 'active')
        self.assertEqual(str(no.cget('default')), 'normal')
        # ... and the command, which records the button index.
        no.invoke()
        self.assertEqual(d.num, 1)

    def test_default_ring(self):
        # The default ring follows the keyboard focus among the buttons
        # (cf. tk::MessageBox).
        d = self.create()  # buttons ['Yes', 'No'], default 0
        b0, b1 = d._buttons
        self.assertEqual(str(b1.cget('default')), 'normal')
        b1.focus_force()
        d.root.update()
        self.assertEqual(str(b1.cget('default')), 'active')   # focused -> ring
        b0.focus_force()
        d.root.update()
        self.assertEqual(str(b1.cget('default')), 'normal')   # unfocused -> none

    def test_alt_key(self):
        # Alt + an underlined character (the "underline" button option) invokes
        # the matching button (cf. tk::AmpWidget in tk::MessageBox).
        d = self.create(buttons=['Yes', {'text': 'No', 'underline': 0}])
        self.require_mapped(d.root)
        d._buttons[0].focus_force()
        d.root.update()
        d.root.event_generate('<Alt-n>')  # "No" -> underline 0 -> "N"
        d.root.update()
        self.assertEqual(d.num, 1)

    def test_return_invokes_focused_button(self):
        # <Return> invokes the button with the focus, even if it is not the
        # default and the focus was not moved by keyboard traversal.
        d = self.create(buttons=['Yes', 'No'])  # default 0
        self.require_mapped(d.root)
        d._buttons[1].focus_force()
        d.root.update()
        d.root.event_generate('<Return>')
        d.root.update()
        self.assertEqual(d.num, 1)

    def test_focus_next_then_return(self):
        # <Tab> moves the focus to the next button; <Return> invokes it.
        d = self.create(buttons=['Yes', 'No'])
        self.require_mapped(d.root)
        d._buttons[0].focus_force()
        d.root.update()
        d._buttons[0].event_generate('<Tab>')
        d.root.update()
        d.root.event_generate('<Return>')
        d.root.update()
        self.assertEqual(d.num, 1)

    def test_focus_prev_then_return(self):
        # <Shift-Tab> moves the focus to the previous button.
        d = self.create(buttons=['Yes', 'No'])
        self.require_mapped(d.root)
        d._buttons[1].focus_force()
        d.root.update()
        d._buttons[1].event_generate('<Shift-Tab>')
        d.root.update()
        d.root.event_generate('<Return>')
        d.root.update()
        self.assertEqual(d.num, 0)

    def test_return_activates_default(self):
        # <Return> with the focus off the buttons invokes the default button.
        d = self.create()  # default 0
        self.require_mapped(d.root)
        d.root.focus_force()  # the dialog, not a button, has the focus
        d.root.update()
        d.root.event_generate('<Return>')
        d.root.update()
        self.assertEqual(d.num, 0)

    def test_return_no_default(self):
        # With no default button, <Return> off the buttons rings the bell and
        # leaves the dialog open instead of activating a button.
        d = self.create(default=None)
        self.require_mapped(d.root)
        d.root.focus_force()  # the dialog, not a button, has the focus
        d.root.update()
        bells = []
        with swap_attr(d.root, 'bell', lambda *a, **k: bells.append(True)):
            d.root.event_generate('<Return>')
            d.root.update()
        self.assertTrue(bells)  # rang the bell
        self.assertIsNone(d.num)
        self.assertTrue(d.root.winfo_exists())

    # --- Modal lifecycle ---

    def test_destroy_cancels(self):
        # Destroying the window records the cancel index.
        d = self.create()
        d.root.update()
        d.root.destroy()
        self.assertEqual(d.num, 1)

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
        d.root.after(1, lambda: d._buttons[0].invoke())
        self.assertEqual(d.go(), 0)


class DialogTest(AbstractTkTest, unittest.TestCase):
    # Dialog's button box is modelled on tk::MessageBox.

    def open(self, **kw):
        with swap_attr(Dialog, 'wait_window', staticmethod(lambda w: None)):
            d = _QueryInteger('Title', 'Prompt', parent=self.root, **kw)
        self.addCleanup(lambda: d.winfo_exists() and d.destroy())
        return d

    # --- Widget set and appearance ---

    def test_use_ttk(self):
        # The query dialogs use the themed (ttk) widgets by default.
        d = self.open()
        self.assertEqual(d.children['ok'].winfo_class(), 'TButton')
        self.assertEqual(d.entry.winfo_class(), 'TEntry')
        # tk::MessageBox makes the buttons equal width.
        self.assertEqual(
            str(d.children['bot'].grid_columnconfigure(0)['uniform']), 'buttons')

    def test_use_classic(self):
        # use_ttk=False uses the classic Tk widgets, modelled on tk_dialog.
        d = self.open(use_ttk=False)
        self.assertEqual(d.children['ok'].winfo_class(), 'Button')
        self.assertEqual(d.entry.winfo_class(), 'Entry')
        if d._windowingsystem == 'x11':
            self.assertEqual(str(d.children['bot'].cget('relief')), 'raised')
        # tk_dialog does not make the buttons equal width.
        self.assertIsNone(d.children['bot'].grid_columnconfigure(0)['uniform'])
        # The bindings work with the classic buttons too.
        invoked = []
        cancel = d.children['cancel']
        cancel.configure(command=lambda: invoked.append(True))
        cancel.focus_force()
        d.update()
        d.event_generate('<Return>')
        d.update()
        self.assertTrue(invoked)

    def test_background(self):
        # The ttk dialog adopts the ttk background, even a customized one,
        # while the classic dialog keeps the default Toplevel background.
        style = ttk.Style(self.root)
        old = style.lookup('.', 'background')
        style.configure('.', background='#123456')
        self.addCleanup(style.configure, '.', background=old)
        d = self.open()
        self.assertEqual(str(d.cget('background')), '#123456')
        d = self.open(use_ttk=False)
        ref = tkinter.Toplevel(self.root)
        self.addCleanup(ref.destroy)
        self.assertEqual(str(d.cget('background')), str(ref.cget('background')))

    def test_base_classic_by_default(self):
        # The Dialog base defaults to classic widgets so that subclasses adding
        # classic widgets keep their look; only the query dialogs opt into ttk.
        class MyDialog(Dialog):
            def body(self, master):
                pass
        with swap_attr(Dialog, 'wait_window', staticmethod(lambda w: None)):
            d = MyDialog(self.root, 'Title')
        self.addCleanup(lambda: d.winfo_exists() and d.destroy())
        self.assertEqual(d.children['ok'].winfo_class(), 'Button')

    # --- Buttons and keyboard ---

    def test_button_default(self):
        d = self.open()
        self.assertEqual(str(d.children['ok'].cget('default')), 'active')
        self.assertEqual(str(d.children['cancel'].cget('default')), 'normal')

    def test_underline_ampersand(self):
        self.assertEqual(_underline_ampersand('Yes'), ('Yes', -1))
        self.assertEqual(_underline_ampersand('&Yes'), ('Yes', 0))
        self.assertEqual(_underline_ampersand('Save &As'), ('Save As', 5))
        self.assertEqual(_underline_ampersand('A&&B'), ('A&B', -1))
        self.assertEqual(_underline_ampersand('&a&b'), ('ab', 0))

    def test_button_accelerator(self):
        # The buttons' "&" accelerators are parsed (cf. tk::AmpWidget).
        d = self.open()
        ok = d.children['ok']  # "&OK" -> underline 0 -> "O"
        self.assertEqual(str(ok.cget('text')), 'OK')
        self.assertEqual(int(ok.cget('underline')), 0)

    def test_default_ring(self):
        # The default ring follows the keyboard focus among the buttons.
        d = self.open()
        cancel = d.children['cancel']
        self.assertEqual(str(cancel.cget('default')), 'normal')
        cancel.focus_force()
        d.update()
        self.assertEqual(str(cancel.cget('default')), 'active')
        d.children['ok'].focus_force()
        d.update()
        self.assertEqual(str(cancel.cget('default')), 'normal')

    def test_find_alt_key_target(self):
        d = self.open()
        ok = d.children['ok']          # "&OK" -> "O"
        cancel = d.children['cancel']  # "&Cancel" -> "C"
        self.assertIs(_find_alt_key_target(d, 'o'), ok)
        self.assertIs(_find_alt_key_target(d, 'O'), ok)  # case-insensitive
        self.assertIs(_find_alt_key_target(d, 'c'), cancel)
        self.assertIsNone(_find_alt_key_target(d, 'q'))

    def test_alt_key(self):
        # The accelerator key (Alt + the underlined letter) invokes the button:
        # <Alt-Key> -> _alt_key -> the button's <<AltUnderlined>> -> invoke.
        d = self.open()
        invoked = []
        cancel = d.children['cancel']  # "&Cancel"
        cancel.configure(command=lambda: invoked.append(True))
        d.focus_force()
        d.update()
        d.event_generate('<Alt-c>')
        d.update()
        self.assertTrue(invoked)

    def test_return_invokes_focused_button(self):
        # <Return> invokes the focused button.
        d = self.open()
        invoked = []
        cancel = d.children['cancel']
        cancel.configure(command=lambda: invoked.append(True))
        cancel.focus_force()
        d.update()
        d.event_generate('<Return>')
        d.update()
        self.assertEqual(invoked, [True])

    def test_focus_next_then_return(self):
        # <Tab> moves the focus to the next button; <Return> invokes it.
        d = self.open()
        invoked = []
        for name in ('ok', 'cancel'):
            d.children[name].configure(command=lambda name=name: invoked.append(name))
        ok = d.children['ok']
        ok.focus_force()
        d.update()
        ok.event_generate('<Tab>')  # OK -> Cancel
        d.update()
        d.event_generate('<Return>')
        d.update()
        self.assertEqual(invoked, ['cancel'])

    def test_focus_prev_then_return(self):
        # <Shift-Tab> moves the focus to the previous button.
        d = self.open()
        invoked = []
        for name in ('ok', 'cancel'):
            d.children[name].configure(command=lambda name=name: invoked.append(name))
        cancel = d.children['cancel']
        cancel.focus_force()
        d.update()
        cancel.event_generate('<Shift-Tab>')  # Cancel -> OK
        d.update()
        d.event_generate('<Return>')
        d.update()
        self.assertEqual(invoked, ['ok'])


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

    # --- Prompt and entry ---

    def test_prompt_wraplength(self):
        # A long prompt wraps instead of widening the dialog (cf. MessageBox).
        d = self.open(_QueryInteger)
        body = d.children['top']
        label = [w for w in body.winfo_children() if w is not d.entry][0]
        self.assertEqual(str(label.cget('wraplength')), '3i')

    def test_prompt_column_expands(self):
        # The prompt and entry column expands to the full width, like the
        # weighted message column in tk::MessageBox.
        d = self.open(_QueryInteger)
        body = d.children['top']
        self.assertEqual(body.grid_columnconfigure(0)['weight'], 1)

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

    # --- Accept and cancel ---

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

    # --- Validation ---

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

    # --- Convenience ask* functions ---

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
