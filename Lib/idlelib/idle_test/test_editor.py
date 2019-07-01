"Test editor, coverage 35%."

from idlelib import editor
import unittest
from test.support import requires
from tkinter import Tk

Editor = editor.EditorWindow


class EditorWindowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)
        cls.root.destroy()
        del cls.root

    def test_init(self):
        e = Editor(root=self.root)
        self.assertEqual(e.root, self.root)
        e._close()


class EditorFunctionTest(unittest.TestCase):

    def test_filename_to_unicode(self):
        func = Editor._filename_to_unicode
        class dummy():
            filesystemencoding = 'utf-8'
        pairs = (('abc', 'abc'), ('a\U00011111c', 'a\ufffdc'),
                 (b'abc', 'abc'), (b'a\xf0\x91\x84\x91c', 'a\ufffdc'))
        for inp, out in pairs:
            self.assertEqual(func(dummy, inp), out)


class TestClassifyWS(unittest.TestCase):
    def test_empty_lines(self):
        for tabwidth in [1, 2, 4, 6, 8]:
            for s in ['', '\n']:
                with self.subTest(string=s, tabwidth=tabwidth):
                    self.assertEqual(
                        editor.classifyws(s, tabwidth=tabwidth),
                        (0, 0),
                    )

    def test_tabwidth_4(self):
        #        (line, (raw, effective))
        tests = (('no spaces', (0, 0)),
                 # Internal space isn't counted.
                 ('    space test', (4, 4)),
                 ('\ttab test', (1, 4)),
                 ('\t\tdouble tabs test', (2, 8)),
                 # Different results when mixing tabs and spaces.
                 ('    \tmixed test', (5, 8)),
                 ('  \t  mixed test', (5, 6)),
                 ('\t    mixed test', (5, 8)),
                 # Spaces not divisible by tabwidth.
                 ('  \tmixed test', (3, 4)),
                 (' \t mixed test', (3, 5)),
                 ('\t  mixed test', (3, 6)),
                 # Only checks spaces and tabs.
                 ('\nnewline test', (0, 0)))

        for string, expected in tests:
            with self.subTest(string=string):
                self.assertEqual(
                    editor.classifyws(string, tabwidth=4),
                    expected,
                )

    def test_tabwidth_8(self):
        #        (line, (raw, effective))
        tests = (('no spaces', (0, 0)),
                 # Internal space isn't counted.
                 ('        space test', (8, 8)),
                 ('\ttab test', (1, 8)),
                 ('\t\tdouble tabs test', (2, 16)),
                 # Different results when mixing tabs and spaces.
                 ('        \tmixed test', (9, 16)),
                 ('      \t  mixed test', (9, 10)),
                 ('\t        mixed test', (9, 16)),
                 # Spaces not divisible by tabwidth.
                 ('  \tmixed test', (3, 8)),
                 (' \t mixed test', (3, 9)),
                 ('\t  mixed test', (3, 10)),
                 # Only checks spaces and tabs.
                 ('\nnewline test', (0, 0)))

        for string, expected in tests:
            with self.subTest(string=string):
                self.assertEqual(
                    editor.classifyws(string, tabwidth=8),
                    expected,
                )


if __name__ == '__main__':
    unittest.main(verbosity=2)
