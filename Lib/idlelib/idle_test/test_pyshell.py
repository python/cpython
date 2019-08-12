"Test pyshell, coverage 12%."
# Plus coverage of test_warning.  Was 20% with test_openshell.

from idlelib import pyshell
import unittest
from test.support import requires
from tkinter import Tk


class PyShellFileListTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        #cls.root.update_idletasks()
##        for id in cls.root.tk.call('after', 'info'):
##            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        psfl = pyshell.PyShellFileList(self.root)
        self.assertEqual(psfl.EditorWindow, pyshell.PyShellEditorWindow)
        self.assertIsNone(psfl.pyshell)

# The following sometimes causes 'invalid command name "109734456recolorize"'.
# Uncommenting after_cancel above prevents this, but results in
# TclError: bad window path name ".!listedtoplevel.!frame.text"
# which is normally prevented by after_cancel.
##    def test_openshell(self):
##        pyshell.use_subprocess = False
##        ps = pyshell.PyShellFileList(self.root).open_shell()
##        self.assertIsInstance(ps, pyshell.PyShell)


class TestProcessControlChars(unittest.TestCase):
    def call(self, existing, string, cursor):
        return pyshell.PyShell._process_control_chars(existing, string, cursor)

    def check(self, existing, string, cursor, expected):
        self.assertEqual(self.call(existing, string, cursor), expected)

    def check_leniently(self, existing, string, cursor, expected):
        result = self.call(existing, string, cursor)
        if existing and not expected[0]:
            options = [
                expected,
                (True, existing[:cursor] + expected[1], expected[2]),
            ]
            self.assertIn(result, options)
        else:
            self.assertEqual(result, expected)

    def test_empty_written(self):
        self.check('', '', 0, (False, '', 0))
        self.check('a', '', 0, (False, '', 0))
        self.check('a', '', 1, (False, '', 0))
        for cursor in range(4):
            with self.subTest(cursor=cursor):
                self.check('abc', '', cursor, (False, '', 0))

    def test_empty_existing(self):
        self.check('', 'a', 0, (False, 'a', 0))
        self.check('', 'ab', 0, (False, 'ab', 0))
        self.check('', 'abc', 0, (False, 'abc', 0))

    def test_simple_cursor(self):
        self.check('abc', 'def', 0, (False, 'def', 0))
        self.check('abc', 'def', 1, (False, 'def', 0))
        self.check('abc', 'def', 2, (False, 'def', 0))

        self.check('abc', 'def', 3, (False, 'def', 0))

    def test_carriage_return(self):
        self.check('', 'a\rb', 0, (False, 'b', 0))
        self.check('', 'abc\rd', 0, (False, 'dbc', 2))

    def test_carriage_return_doesnt_delete(self):
        # \r should only move the cursor to the beginning of the current line
        self.check('', 'a\r', 0, (False, 'a', 1))
        self.check('', 'abc\r', 0, (False, 'abc', 3))

    def test_backspace(self):
        self.check('', '\ba', 0, (False, 'a', 0))
        self.check('', 'a\bb', 0, (False, 'b', 0))
        self.check('', 'ab\bc', 0, (False, 'ac', 0))
        self.check('', 'ab\bc\bd', 0, (False, 'ad', 0))

        self.check('abc', '\b', 3, (False, '', 1))
        self.check('abc', '\b', 2, (False, 'c', 2))
        self.check('abc', '\b', 1, (False, 'bc', 3))
        self.check('abc', '\b', 0, (False, 'abc', 3))

    def test_backspace_doesnt_delete(self):
        # \b should only move the cursor one place earlier
        self.check('', 'a\b', 0, (False, 'a', 1))
        self.check('', 'a\b\b', 0, (False, 'a', 1))
        self.check('', 'ab\b\b', 0, (False, 'ab', 2))
        self.check('', 'ab\b\bc', 0, (False, 'cb', 1))
        self.check('', 'abc\b\bd', 0, (False, 'adc', 1))
        self.check('', 'ab\bc\b', 0, (False, 'ac', 1))

    def test_newline(self):
        self.check('', '\n', 0, (False, '\n', 0))
        self.check('abc', '\n', 3, (False, '\n', 0))
        self.check('abc', 'def\n', 3, (False, 'def\n', 0))
        self.check('abc', '\ndef', 3, (False, '\ndef', 0))

    def test_newline_and_carriage_return(self):
        self.check('abc', '\n\rdef', 3, (False, '\ndef', 0))
        self.check('abc', 'd\n\ref', 3, (False, 'd\nef', 0))
        self.check('abc', 'de\n\rf', 3, (False, 'de\nf', 0))
        self.check('abc', 'def\n\r', 3, (False, 'def\n', 0))

        self.check_leniently('abc', '\r\n', 3, (False, '\n', 0))
        self.check_leniently('abc', 'def\r\n', 3, (False, 'def\n', 0))
        self.check_leniently('abc', '\r\ndef', 3, (False, '\ndef', 0))
        self.check_leniently('abc', '\rdef\n', 3, (True, 'def\n', 0))
        self.check_leniently('abc', '\rd\nef', 3, (True, 'dbc\nef', 0))


if __name__ == '__main__':
    unittest.main(verbosity=2)
