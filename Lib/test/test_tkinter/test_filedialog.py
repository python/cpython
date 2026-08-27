import os
import unittest
import tkinter
from tkinter import filedialog
from tkinter import ttk
from tkinter.commondialog import Dialog
from test.support import requires, swap_attr
from test.test_tkinter.support import setUpModule  # noqa: F401
from test.test_tkinter.support import AbstractTkTest

requires('gui')


class NativeDialogTest(AbstractTkTest, unittest.TestCase):
    # Open, SaveAs and Directory wrap modal native dialogs.  The _test_callback
    # seam is called by show() just before the dialog would open; replacing it
    # with a function that raises exercises show() without blocking on the
    # dialog.

    def check(self, dialog_class, command):
        self.assertEqual(dialog_class.command, command)
        master = None
        def test_callback(dialog, parent):
            nonlocal master
            master = parent
            raise ZeroDivisionError
        with swap_attr(Dialog, '_test_callback', test_callback):
            d = dialog_class(self.root, title='Test')
            self.assertRaises(ZeroDivisionError, d.show)
        self.assertIs(master, self.root)

    def test_open(self):
        self.check(filedialog.Open, 'tk_getOpenFile')

    def test_saveas(self):
        self.check(filedialog.SaveAs, 'tk_getSaveFile')

    def test_directory(self):
        self.check(filedialog.Directory, 'tk_chooseDirectory')


class CancelResultTest(AbstractTkTest, unittest.TestCase):
    # On cancellation Tcl may report the empty result as '', () or b''
    # (gh-103878).  _fixresult() normalizes it to the documented empty value:
    # '' for the filename dialogs and () for the multiple-selection dialog.

    def check(self, dialog, expected):
        for empty in ('', (), b''):
            with self.subTest(empty=empty):
                result = dialog._fixresult(self.root, empty)
                self.assertEqual(result, expected)
                self.assertIs(type(result), type(expected))

    def test_open(self):
        self.check(filedialog.Open(self.root), '')

    def test_saveas(self):
        self.check(filedialog.SaveAs(self.root), '')

    def test_directory(self):
        self.check(filedialog.Directory(self.root), '')

    def test_openfilenames(self):
        self.check(filedialog.Open(self.root, multiple=1), ())

    def test_results_preserved(self):
        # A real selection is returned unchanged.
        single = filedialog.Open(self.root)
        self.assertEqual(single._fixresult(self.root, '/a/spam'), '/a/spam')
        multiple = filedialog.Open(self.root, multiple=1)
        self.assertEqual(multiple._fixresult(self.root, ('/a', '/b')),
                         ('/a', '/b'))


class FileDialogTest(AbstractTkTest, unittest.TestCase):
    # The pure-Python FileDialog runs its own modal loop in go(); its logic is
    # exercised here without entering the loop.

    def test_selection(self):
        d = filedialog.FileDialog(self.root)
        d.directory = os.path.abspath(os.sep)
        d.set_selection('spam.txt')
        self.assertEqual(os.path.basename(d.get_selection()), 'spam.txt')

    def test_filter(self):
        d = filedialog.FileDialog(self.root)
        d.set_filter(os.getcwd(), '*.py')
        self.assertEqual(d.get_filter(), (os.getcwd(), '*.py'))

    def test_ok_cancel(self):
        d = filedialog.FileDialog(self.root)
        d.directory = os.getcwd()
        d.set_selection('spam.txt')
        d.ok_command()  # Accepts the current selection.
        self.assertEqual(os.path.basename(d.how), 'spam.txt')
        d.cancel_command()  # Returns no selection.
        self.assertIsNone(d.how)

    def test_subclasses(self):
        for cls in filedialog.LoadFileDialog, filedialog.SaveFileDialog:
            with self.subTest(cls=cls.__name__):
                d = cls(self.root)
                self.assertIsInstance(d, filedialog.FileDialog)
                self.assertEqual(d.top.title(), cls.title)

    # --- Themed widgets and keyboard (modernization) ---

    def open(self, **kw):
        d = filedialog.FileDialog(self.root, **kw)
        self.addCleanup(lambda: d.top.winfo_exists() and d.top.destroy())
        d.top.deiconify()  # __init__ leaves the dialog withdrawn until go()
        d.top.update()
        return d

    def test_use_ttk(self):
        # The dialog uses the themed (ttk) widgets by default.
        d = self.open()
        self.assertEqual(d.ok_button.winfo_class(), 'TButton')
        self.assertEqual(d.selection.winfo_class(), 'TEntry')

    def test_use_classic(self):
        # use_ttk=False uses the classic Tk widgets.
        d = self.open(use_ttk=False)
        self.assertEqual(d.ok_button.winfo_class(), 'Button')
        self.assertEqual(d.selection.winfo_class(), 'Entry')
        if d.top._windowingsystem == 'x11':
            self.assertEqual(d.botframe.cget('relief'), 'raised')

    def test_background(self):
        # The ttk dialog adopts the ttk background, even a customized one, while
        # the classic dialog keeps the default Toplevel background.
        style = ttk.Style(self.root)
        old = style.lookup('.', 'background')
        style.configure('.', background='#123456')
        self.addCleanup(style.configure, '.', background=old)
        d = self.open()
        self.assertEqual(str(d.top.cget('background')), '#123456')
        d = self.open(use_ttk=False)
        ref = tkinter.Toplevel(self.root)
        self.addCleanup(ref.destroy)
        self.assertEqual(str(d.top.cget('background')),
                         str(ref.cget('background')))

    def test_button_accelerator(self):
        # The buttons' "&" accelerators are parsed.
        d = self.open()
        self.assertEqual(str(d.ok_button.cget('text')), 'OK')
        self.assertEqual(d.ok_button.cget('underline'),
                         0 if self.wantobjects else '0')

    def test_default_ring(self):
        # The default ring follows the keyboard focus among the buttons.
        d = self.open()
        self.assertEqual(d.cancel_button.cget('default'), 'normal')
        d.cancel_button.focus_force()
        d.top.update()
        self.assertEqual(d.cancel_button.cget('default'), 'active')
        d.ok_button.focus_force()
        d.top.update()
        self.assertEqual(d.cancel_button.cget('default'), 'normal')

    def test_alt_key(self):
        # Alt + the underlined letter invokes the matching button.
        d = self.open()
        invoked = []
        d.cancel_button.configure(command=lambda: invoked.append(True))
        d.top.focus_force()
        d.top.update()
        d.top.event_generate('<Alt-c>')  # "&Cancel"
        d.top.update()
        self.assertTrue(invoked)

    def test_escape_cancels(self):
        # The Escape key cancels the dialog.
        d = self.open()
        d.how = 'spam'
        d.top.focus_force()
        d.top.update()
        d.top.event_generate('<Escape>')
        d.top.update()
        self.assertIsNone(d.how)

    def test_horizontal_scrollbars(self):
        # Each list has a horizontal scrollbar besides the vertical one.
        d = self.open()
        self.assertEqual(d.dirshbar.cget('orient'), 'horizontal')
        self.assertEqual(d.fileshbar.cget('orient'), 'horizontal')
        self.assertTrue(d.dirs.cget('xscrollcommand'))
        self.assertTrue(d.files.cget('xscrollcommand'))

    def test_type_ahead(self):
        # Typing characters over a list jumps to a matching entry.
        d = self.open()
        d.directory = os.getcwd()  # browsing the match fills the selection entry
        d.files.delete(0, 'end')
        for name in ('alpha', 'bravo', 'charlie'):
            d.files.insert('end', name)
        d.files.focus_force()
        d.top.update()
        d.files.event_generate('<Key>', keysym='c')
        d.top.update()
        sel = d.files.curselection()
        self.assertEqual([d.files.get(i) for i in sel], ['charlie'])


class DeprecationTest(unittest.TestCase):

    def test_askopenfiles_deprecated(self):
        with swap_attr(filedialog, 'askopenfilenames', lambda **kw: ()):
            with self.assertWarns(DeprecationWarning):
                filedialog.askopenfiles()


if __name__ == "__main__":
    unittest.main()
