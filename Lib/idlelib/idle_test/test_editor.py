"Test editor, coverage 35%."

from idlelib import editor
import unittest
from unittest import mock
from test.support import requires
import tkinter as tk
from functools import partial

Editor = editor.EditorWindow


class EditorWindowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()
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


class MenubarTest(unittest.TestCase):
    """Test functions involved with creating the menubar."""

    @classmethod
    def setUpClass(cls):
        requires('gui')
        root = cls.root = tk.Tk()
        cls.root.withdraw()
        # Test the functions called during the __init__ for
        # EditorWindow that create the menubar and submenus.
        # The class is mocked in order to prevent the functions
        # from being called automatically.
        w = cls.mock_editwin = mock.Mock(editor.EditorWindow)
        w.menubar = tk.Menu(root, tearoff=False)
        w.text = tk.Text(root)
        w.tkinter_vars = {}

    @classmethod
    def tearDownClass(cls):
        w = cls.mock_editwin
        w.text.destroy()
        w.menubar.destroy()
        del w.menubar, w.text, w
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)
        cls.root.destroy()
        del cls.root

    def test_fill_menus(self):
        eq = self.assertEqual
        ed = editor.EditorWindow
        w = self.mock_editwin
        # Call real functions instead of mock.
        fm = partial(editor.EditorWindow.fill_menus, w)
        w.get_var_obj = ed.get_var_obj.__get__(w)

        # Initialize top level menubar.
        w.menudict = {}
        edit = w.menudict['edit'] = tk.Menu(w.menubar, name='edit', tearoff=False)
        win = w.menudict['windows'] = tk.Menu(w.menubar, name='windows', tearoff=False)
        form = w.menudict['format'] = tk.Menu(w.menubar, name='format', tearoff=False)

        # Submenus.
        dict_menudefs = {'edit': [('_New', '<<open-new>>'),
                                  None,
                                  ('!Deb_ug', '<<debug>>')],
                         'shell': [('_View', '<<view-restart>>'), ],
                         'windows': [('Zoom Height', '<<zoom-height>>')],
                        }
        list_menudefs = [('edit', [('_New', '<<open-new>>'),
                                  None,
                                  ('!Deb_ug', '<<debug>>')]),
                         ('shell', [('_View', '<<view-restart>>'), ]),
                         ('windows', [('Zoom Height', '<<zoom-height>>')]),
                        ]
        keydefs = {'<<zoom-height>>': ['<Alt-Key-9>']}
        for menudefs in (dict_menudefs, list_menudefs):
            with self.subTest(menudefs=menudefs):
                fm(menudefs, keydefs)

                eq(edit.index('end'), 2)
                eq(edit.type(0), tk.COMMAND)
                eq(edit.entrycget(0, 'label'), 'New')
                eq(edit.entrycget(0, 'underline'), 0)
                self.assertIsNotNone(edit.entrycget(0, 'command'))
                with self.assertRaises(tk.TclError):
                    self.assertIsNone(edit.entrycget(0, 'var'))

                eq(edit.type(1), tk.SEPARATOR)
                with self.assertRaises(tk.TclError):
                    self.assertIsNone(edit.entrycget(1, 'label'))

                eq(edit.type(2), tk.CHECKBUTTON)
                # Strip !.
                eq(edit.entrycget(2, 'label'), 'Debug')
                # Check that underline ignores !.
                eq(edit.entrycget(2, 'underline'), 3)
                self.assertIsNotNone(edit.entrycget(2, 'var'))
                self.assertIn('<<debug>>', w.tkinter_vars)

                eq(win.index('end'), 0)
                eq(win.entrycget(0, 'underline'), -1)
                eq(win.entrycget(0, 'accelerator'), 'Alt+9')

                eq(form.index('end'), None)
                self.assertNotIn('shell', w.menudict)

                # Cleanup menus by deleting all menu items.
                edit.delete(0, 2)
                win.delete(0)
                form.delete(0)

        # Test defaults.
        w.mainmenu.menudefs = ed.mainmenu.menudefs
        w.mainmenu.default_keydefs = ed.mainmenu.default_keydefs
        fm()
        eq(form.index('end'), 9)  # Default Format menu has 10 items.
        self.assertNotIn('run', w.menudict)


if __name__ == '__main__':
    unittest.main(verbosity=2)
