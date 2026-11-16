import time
import unittest
import tkinter
from tkinter import font
from tkinter.fontchooser import FontChooser
from test import support
from test.support import requires
from test.test_tkinter.support import (AbstractTkTest,
                                       AbstractDefaultRootTest,
                                       setUpModule)  # noqa: F401

requires('gui')


class FontChooserTest(AbstractTkTest, unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.fc = FontChooser(self.root)
        self.addCleanup(self._reset)

    def _reset(self):
        # The font dialog is global for the interpreter, so restore
        # a clean state for the next test.
        try:
            self.fc.configure(parent=self.root, title='', command=None)
            self.fc.hide()
        except tkinter.TclError:
            pass

    def _visible(self):
        return self.root.tk.getboolean(self.fc.cget('visible'))

    def _wait_visibility(self, expected, timeout=None):
        # Bounded wait until the dialog visibility matches expected.
        if timeout is None:
            timeout = support.LOOPBACK_TIMEOUT
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            self.root.update()
            if self._visible() == expected:
                return True
            time.sleep(0.01)
        return False

    def test_configure_query(self):
        options = self.fc.configure()
        self.assertIsInstance(options, dict)
        self.assertLessEqual({'parent', 'title', 'font', 'command', 'visible'},
                             options.keys())

    def test_configure(self):
        self.fc.configure(title='Pick a font')
        if self.root._windowingsystem == 'x11':
            self.assertEqual(self.fc.cget('title'), 'Pick a font')
        self.fc.configure(font='Courier 10')
        self.assertTrue(self.fc.cget('font'))
        if self.root._windowingsystem == 'x11':
            self.assertEqual(str(self.fc.cget('font')), 'Courier 10')

    def test_parent(self):
        top = tkinter.Toplevel(self.root)
        self.addCleanup(top.destroy)
        # The constructor does not force -parent to the master: it stays
        # the main window even when the master is another widget.
        FontChooser(top)
        self.assertEqual(self.fc.cget('parent'), str(self.root))
        # -parent can be set explicitly.
        self.fc.configure(parent=top)
        self.assertEqual(self.fc.cget('parent'), str(top))

    def test_configure_font_instance(self):
        # A Font instance can be passed as the font, both to the
        # constructor and to configure().  The dialog may store it as
        # the font name or as a resolved description depending on the
        # platform, so compare the actual attributes.
        def actual(spec):
            return font.Font(self.root, spec, exists=True).actual()
        f = font.Font(self.root, family='Courier', size=14, weight='bold')
        fc = FontChooser(self.root, font=f)
        self.assertEqual(actual(fc.cget('font')), f.actual())
        f2 = font.Font(self.root, family='Times', size=11)
        fc.configure(font=f2)
        self.assertEqual(actual(fc.cget('font')), f2.actual())

    def test_configure_visible_readonly(self):
        with self.assertRaises(tkinter.TclError):
            self.fc.configure(visible=True)

    def test_cget_visible(self):
        self.assertFalse(self._visible())

    def test_command(self):
        result = []
        self.fc.configure(command=result.append)
        name = self.fc._command_name
        self.assertTrue(name)
        # The Tcl command is named after the wrapped callback.
        self.assertTrue(name.endswith('append'), name)
        # The callback receives a Font wrapping the selected font.
        self.root.tk.call(name, 'Courier 10')
        self.assertEqual(len(result), 1)
        selected = result[0]
        self.assertIsInstance(selected, font.Font)
        # The description is wrapped without creating a named font.
        self.assertEqual(str(selected), 'Courier 10')
        self.assertFalse(selected.delete_font)
        self.assertEqual(int(selected.actual('size')), 10)
        # Replacing the callback deletes the old Tcl command.
        self.fc.configure(command=lambda font: None)
        self.assertNotEqual(self.fc._command_name, name)
        self.assertFalse(self.root.tk.call('info', 'commands', name))
        # Removing the callback deletes the Tcl command.
        name = self.fc._command_name
        self.fc.configure(command=None)
        self.assertIsNone(self.fc._command_name)
        self.assertFalse(self.root.tk.call('info', 'commands', name))
        self.assertEqual(self.fc.cget('command'), '')

    def test_show_hide(self):
        if self.root._windowingsystem != 'x11':
            self.skipTest('cannot safely drive the native font dialog')
        events = []
        self.root.bind('<<TkFontchooserVisibility>>', events.append)
        self.fc.show()
        if not self._wait_visibility(True):
            self.skipTest('the font dialog was not mapped')
        self.fc.hide()
        self.assertTrue(self._wait_visibility(False))
        self.assertTrue(events)


class DefaultRootTest(AbstractDefaultRootTest, unittest.TestCase):

    def test_fontchooser(self):
        root = tkinter.Tk()
        fc = FontChooser()
        self.assertIs(fc.master, root)
        root.destroy()
        tkinter.NoDefaultRoot()
        self.assertRaises(RuntimeError, FontChooser)


if __name__ == "__main__":
    unittest.main()
