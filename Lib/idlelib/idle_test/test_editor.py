"Test editor, coverage 35%."

from idlelib import editor
from collections import namedtuple
from functools import partial
import sys
from test.support import requires

import unittest
from unittest import mock
import tkinter as tk
from idlelib.multicall import MultiCallCreator
from idlelib.idle_test.mock_idle import Func

Editor = editor.EditorWindow
root = None
editwin = None


def setUpModule():
    global root, editwin
    requires('gui')
    root = tk.Tk()
    root.withdraw()
    editwin = editor.EditorWindow(root=root)


def tearDownModule():
    global root, editwin
    editwin.close()
    del editwin
    root.update_idletasks()
    root.destroy()
    del root


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


class TestGetLineIndent(unittest.TestCase):
    def test_empty_lines(self):
        for tabwidth in [1, 2, 4, 6, 8]:
            for line in ['', '\n']:
                with self.subTest(line=line, tabwidth=tabwidth):
                    self.assertEqual(
                        editor.get_line_indent(line, tabwidth=tabwidth),
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

        for line, expected in tests:
            with self.subTest(line=line):
                self.assertEqual(
                    editor.get_line_indent(line, tabwidth=4),
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

        for line, expected in tests:
            with self.subTest(line=line):
                self.assertEqual(
                    editor.get_line_indent(line, tabwidth=8),
                    expected,
                )


def insert(text, string):
    text.delete('1.0', 'end')
    text.insert('end', string)
    text.update()  # Force update for colorizer to finish.


class IndentAndNewlineTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()
        cls.root.withdraw()
        cls.window = Editor(root=cls.root)
        cls.window.indentwidth = 2
        cls.window.tabwidth = 2

    @classmethod
    def tearDownClass(cls):
        cls.window._close()
        del cls.window
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)
        cls.root.destroy()
        del cls.root

    def test_indent_and_newline_event(self):
        eq = self.assertEqual
        w = self.window
        text = w.text
        get = text.get
        nl = w.newline_and_indent_event

        TestInfo = namedtuple('Tests', ['label', 'text', 'expected', 'mark'])

        tests = (TestInfo('Empty line inserts with no indent.',
                          '  \n  def __init__(self):',
                          '\n  \n  def __init__(self):\n',
                          '1.end'),
                 TestInfo('Inside bracket before space, deletes space.',
                          '  def f1(self, a, b):',
                          '  def f1(self,\n         a, b):\n',
                          '1.14'),
                 TestInfo('Inside bracket after space, deletes space.',
                          '  def f1(self, a, b):',
                          '  def f1(self,\n         a, b):\n',
                          '1.15'),
                 TestInfo('Inside string with one line - no indent.',
                          '  """Docstring."""',
                          '  """Docstring.\n"""\n',
                          '1.15'),
                 TestInfo('Inside string with more than one line.',
                          '  """Docstring.\n  Docstring Line 2"""',
                          '  """Docstring.\n  Docstring Line 2\n  """\n',
                          '2.18'),
                 TestInfo('Backslash with one line.',
                          'a =\\',
                          'a =\\\n  \n',
                          '1.end'),
                 TestInfo('Backslash with more than one line.',
                          'a =\\\n          multiline\\',
                          'a =\\\n          multiline\\\n          \n',
                          '2.end'),
                 TestInfo('Block opener - indents +1 level.',
                          '  def f1(self):\n    pass',
                          '  def f1(self):\n    \n    pass\n',
                          '1.end'),
                 TestInfo('Block closer - dedents -1 level.',
                          '  def f1(self):\n    pass',
                          '  def f1(self):\n    pass\n  \n',
                          '2.end'),
                 )

        for test in tests:
            with self.subTest(label=test.label):
                insert(text, test.text)
                text.mark_set('insert', test.mark)
                nl(event=None)
                eq(get('1.0', 'end'), test.expected)

        # Selected text.
        insert(text, '  def f1(self, a, b):\n    return a + b')
        text.tag_add('sel', '1.17', '1.end')
        nl(None)
        # Deletes selected text before adding new line.
        eq(get('1.0', 'end'),
           '  def f1(self, a,\n         \n    return a + b\n')


class RMenuTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()
        cls.root.withdraw()
        cls.window = Editor(root=cls.root)

    @classmethod
    def tearDownClass(cls):
        cls.window._close()
        del cls.window
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)
        cls.root.destroy()
        del cls.root

    class DummyRMenu:
        def tk_popup(x, y): pass

    def test_rclick(self):
        pass


class ModuleHelpersTest(unittest.TestCase):
    """Test functions defined at the module level."""

    def test_prepstr(self):
        ps = editor.prepstr
        eq = self.assertEqual
        eq(ps('_spam'), (0, 'spam'))
        eq(ps('spam'), (-1, 'spam'))
        eq(ps('spam_'), (4, 'spam'))

    @mock.patch.object(editor.macosx, 'isCocoaTk')
    def test_get_accelerator(self, mock_cocoa):
        ga = editor.get_accelerator
        eq = self.assertEqual
        keydefs = {'<<zoom-height>>': ['<Alt-Key-9>'],
                   '<<open-module>>': ['<Control-Shift-O>'],
                   '<<cancel>>': ['<Cancel>'],
                   '<<indent>>': ['<Alt-bracketleft>'],
                   '<<copy>>': ['<Control-j>', '<Control-c>']}

        mock_cocoa.return_value = False
        eq(ga(keydefs, '<<paste>>'), '')  # Not in keydefs.
        eq(ga(keydefs, '<<copy>>'), 'Ctrl+J')  # Control to Ctrl and first only.
        eq(ga(keydefs, '<<zoom-height>>'), 'Alt+9')  # Remove Key-.
        eq(ga(keydefs, '<<indent>>'), 'Alt+[')  # bracketleft to [.
        eq(ga(keydefs, '<<cancel>>'), 'Ctrl+Break')  # Cancel to Ctrl-Break.
        eq(ga(keydefs, '<<open-module>>'), 'Ctrl+Shift+O')  # Shift doesn't change.

        # Cocoa test: it skips open-module shortcut.
        mock_cocoa.return_value = True
        eq(ga(keydefs, '<<open-module>>'), '')


class MenubarTest(unittest.TestCase):
    """Test functions involved with creating the menubar."""

    @classmethod
    def setUpClass(cls):
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

    @mock.patch.object(editor.macosx, 'isCarbonTk')
    def test_createmenubar(self, mock_mac):
        eq = self.assertEqual
        ed = editor.EditorWindow
        w = self.mock_editwin
        # Call real function instead of mock.
        cmb = partial(editor.EditorWindow.createmenubar, w)

        # Load real editor menus.
        w.menu_specs = ed.menu_specs

        mock_mac.return_value = False
        cmb()
        eq(list(w.menudict.keys()),
           [name[0] for name in w.menu_specs])
        for index in range(w.menubar.index('end') + 1):
            eq(w.menubar.type(index), tk.CASCADE)
            eq(w.menubar.entrycget(index, 'label'),
               editor.prepstr(w.menu_specs[index][1])[1])
        # Recent Files added here and not fill_menus.
        eq(w.menudict['file'].entrycget(3, 'label'), 'Recent Files')
        # No items added to helpmenu, so the length has no value.
        eq(w.base_helpmenu_length, None)
        w.fill_menus.assert_called_with()
        w.reset_help_menu_entries.assert_called_with()

        # Carbon includes an application menu.
        mock_mac.return_value = True
        cmb()
        eq(list(w.menudict.keys()),
           [name[0] for name in w.menu_specs] + ['application'])

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
        menudefs = [('edit', [('_New', '<<open-new>>'),
                              None,
                              ('!Deb_ug', '<<debug>>')]),
                    ('shell', [('_View', '<<view-restart>>'), ]),
                    ('windows', [('Zoom Height', '<<zoom-height>>')]), ]
        keydefs = {'<<zoom-height>>': ['<Alt-Key-9>']}

        fm(menudefs, keydefs)
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
        eq(edit.entrycget(2, 'label'), 'Debug')  # Strip !.
        eq(edit.entrycget(2, 'underline'), 3)  # Check that underline ignores !.
        self.assertIsNotNone(edit.entrycget(2, 'var'))
        self.assertIn('<<debug>>', w.tkinter_vars)

        eq(win.entrycget(0, 'underline'), -1)
        eq(win.entrycget(0, 'accelerator'), 'Alt+9')

        self.assertNotIn('shell', w.menudict)

        # Test defaults.
        w.mainmenu.menudefs = ed.mainmenu.menudefs
        w.mainmenu.default_keydefs = ed.mainmenu.default_keydefs
        fm()
        eq(form.index('end'), 9)  # Default Format menu has 10 items.
        self.assertNotIn('run', w.menudict)

    @mock.patch.object(editor.idleConf, 'GetAllExtraHelpSourcesList')
    def test_reset_help_menu_entries(self, mock_extrahelp):
        w = self.mock_editwin
        mock_extrahelp.return_value = [('Python', 'https://python.org', '1')]
        mock_callback = w._extra_help_callback

        # Create help menu.
        help = w.menudict['help'] = tk.Menu(w.menubar, name='help',
                                            tearoff=False)
        cmd = mock_callback.return_value = lambda e: 'break'
        help.add_command(label='help1', command=cmd)
        w.base_helpmenu_length = help.index('end')

        # Add extra menu items that will be removed.
        help.add_command(label='extra1', command=cmd)
        help.add_command(label='extra2', command=cmd)
        help.add_command(label='extra3', command=cmd)
        help.add_command(label='extra4', command=cmd)

        # Assert that there are extra help items.
        self.assertTrue(help.index('end') - w.base_helpmenu_length >= 4)
        self.assertNotEqual(help.index('end'), w.base_helpmenu_length)
        editor.EditorWindow.reset_help_menu_entries(w)
        # Count is 2 because of separator.
        self.assertEqual(help.index('end') - w.base_helpmenu_length, 2)
        mock_callback.assert_called_with('https://python.org')

    def test_get_var_obj(self):
        w = self.mock_editwin
        gvo = partial(editor.EditorWindow.get_var_obj, w)
        w.tkinter_vars = {}

        # No vartype.
        self.assertIsNone(gvo('<<spam>>'))
        self.assertNotIn('<<spam>>', w.tkinter_vars)

        # Create BooleanVar.
        self.assertIsInstance(gvo('<<toggle-debugger>>', tk.BooleanVar),
                              tk.BooleanVar)
        self.assertIn('<<toggle-debugger>>', w.tkinter_vars)

        # No vartype - check cache.
        self.assertIsInstance(gvo('<<toggle-debugger>>'), tk.BooleanVar)

    @mock.patch.object(editor.webbrowser, 'open')
    @unittest.skipIf(sys.platform.startswith('win'), 'Unix only')
    def test__extra_help_callback_not_windows(self, mock_openfile):
        w = self.mock_editwin
        ehc = partial(w._EditorWindow__extra_help_callback, w)

        ehc('http://python.org')
        mock_openfile.called_with('http://python.org')
        ehc('www.python.org')
        mock_openfile.called_with('www.python.org')
        ehc('/foo/bar/baz/')
        mock_openfile.called_with('/foo/bar/baz')

    @mock.patch.object(editor.os, 'startfile')
    @unittest.skipIf(not sys.platform.startswith('win'), 'Windows only')
    def test_extra_help_callback_windows(self, mock_start):
        # os.startfile doesn't exist on other platforms.
        w = self.mock_editwin
        w.showerror = mock.Mock()
        def ehc(source):
            return Editor._extra_help_callback(w, source)
        ehc('http://python.org')
        # 'called_with' requires spec tjr
        #mock_start.called_with('http://python.org')
        # Filename that doesn't open.
        # But get '(X)' tk box 'Document Start Fa...' that needs OK click.
        #mock_start.side_effect = OSError('boom')
        ehc('/foo/bar/baz/')()
        self.assertTrue(w.showerror.callargs.kwargs)
 

class BindingsTest(unittest.TestCase):

    def test_apply_bindings(self):
        eq = self.assertEqual
        w = editwin
        # Save original text and recreate an empty version.  It is not
        # actually empty because Text widgets are created with default
        # events.
        orig_text = w.text
        # Multicall has its own versions of the event_* methods.
        text = w.text = MultiCallCreator(tk.Text)(root)

        keydefs = {'<<zoom-height>>': ['<Alt-Key-9>'],
                   '<<open-module>>': ['<Control-Shift-O>'],
                   '<<cancel>>': ['<Cancel>'],
                   '<<empty>>': [],
                   '<<indent>>': ['<Alt-bracketleft>'],
                   '<<copy>>': ['<Control-j>', '<Control-c>']}

        w.apply_bindings(keydefs)
        eq(text.keydefs, keydefs)
        # Multicall event_add() formats the key sequences.
        eq(text.event_info('<<zoom-height>>'), ('<Alt-KeyPress-9>',))
        eq(text.event_info('<<copy>>'), ('<Control-Key-j>', '<Control-Key-c>'))
        eq(text.event_info('<<cancel>>'), ('<Key-Cancel>',))
        # Although apply_bindings() skips events with no keys, Multicall
        # event_info() just returns an empty tuple for undefined events.
        eq(text.event_info('<<empty>>'), ())
        # Not in keydefs.
        eq(text.event_info('<<python-docs>>'), ())

        # Cleanup.
        for event, keylist in keydefs.items():
            text.event_delete(event, *keylist)

        # Use default.
        w.apply_bindings()
        eq(text.event_info('<<python-docs>>'), ('<KeyPress-F1>',))

        del w.text
        w.text = orig_text


class ReloadTests(unittest.TestCase):
    """Test functions called from configdialog for reloading attributes."""

    @classmethod
    def setUpClass(cls):
        cls.keydefs = {'<<copy>>': ['<Control-c>', '<Control-C>'],
                       '<<beginning-of-line>>': ['<Control-a>', '<Home>'],
                       '<<close-window>>': ['<Alt-F4>'],
                       '<<python-docs>>': ['<F15>'],
                       '<<python-context-help>>': ['<Shift-F1>'], }
        cls.extensions = {'<<zzdummy>>'}
        cls.ext_keydefs = {'<<zzdummy>>': ['<Alt-Control-Shift-z>']}

    @classmethod
    def tearDownClass(cls):
        del cls.keydefs, cls.extensions, cls.ext_keydefs

    def setUp(self):
        self.save_text = editwin.text
        editwin.text = MultiCallCreator(tk.Text)(root)

    def tearDown(self):
        del editwin.text
        editwin.text = self.save_text

    @mock.patch.object(editor.idleConf, 'GetExtensionBindings')
    @mock.patch.object(editor.idleConf, 'GetExtensions')
    @mock.patch.object(editor.idleConf, 'GetCurrentKeySet')
    def test_RemoveKeyBindings(self, mock_keyset, mock_ext, mock_ext_bindings):
        eq = self.assertEqual
        w = editwin
        tei = w.text.event_info
        keys = self.keydefs
        extkeys = self.ext_keydefs

        mock_keyset.return_value = keys
        mock_ext.return_value = self.extensions
        mock_ext_bindings.return_value = extkeys

        w.apply_bindings(keys)
        w.apply_bindings({'<<spam>>': ['<F18>']})
        w.apply_bindings(extkeys)

        # Bindings exist.
        for event in keys:
            self.assertNotEqual(tei(event), ())
        self.assertNotEqual(tei('<<spam>>'), ())
        # Extention bindings exist.
        for event in extkeys:
            self.assertNotEqual(tei(event), ())

        w.RemoveKeybindings()
        # Binding events have been deleted.
        for event in keys:
            eq(tei(event), ())
        # Extention bindings have been removed.
        for event in extkeys:
            eq(tei(event), ())
        # Extra keybindings are not removed - only removes those in idleConf.
        self.assertNotEqual(tei('<<spam>>'), ())
        # Remove it.
        w.text.event_delete('<<spam>>', ['<F18>'])

    @mock.patch.object(editor.idleConf, 'GetExtensionBindings')
    @mock.patch.object(editor.idleConf, 'GetExtensions')
    @mock.patch.object(editor.idleConf, 'GetCurrentKeySet')
    def test_ApplyKeyBindings(self, mock_keyset, mock_ext, mock_ext_bindings):
        eq = self.assertEqual
        w = editwin
        tei = w.text.event_info
        keys = self.keydefs
        extkeys = self.ext_keydefs

        mock_keyset.return_value = keys
        mock_ext.return_value = self.extensions
        mock_ext_bindings.return_value = extkeys

        # Bindings don't exist.
        for event in keys:
            eq(tei(event), ())
        # Extention bindings don't exist.
        for event in extkeys:
            eq(tei(event), ())

        w.ApplyKeybindings()
        eq(tei('<<python-docs>>'), ('<Key-F15>',))
        eq(tei('<<beginning-of-line>>'), ('<Control-Key-a>', '<Key-Home>'))
        eq(tei('<<zzdummy>>'), ('<Control-Shift-Alt-Key-z>',))
        # Check menu accelerator update.
        eq(w.menudict['help'].entrycget(3, 'accelerator'), 'F15')

        # Calling ApplyBindings is additive.
        mock_keyset.return_value = {'<<python-docs>>': ['<Shift-F1>']}
        w.ApplyKeybindings()
        eq(tei('<<python-docs>>'), ('<Key-F15>', '<Shift-Key-F1>'))
        w.text.event_delete('<<python-docs>>', ['<Shift-Key-F1>'])

        mock_keyset.return_value = keys
        w.RemoveKeybindings()

    def test_ResetColorizer(self):
        pass

    @mock.patch.object(editor.idleConf, 'GetFont')
    def test_ResetFont(self, mock_getfont):
        mock_getfont.return_value = ('spam', 16, 'bold')
        self.assertNotEqual(editwin.text['font'], 'spam 16 bold')
        editwin.ResetFont()
        self.assertEqual(editwin.text['font'], 'spam 16 bold')

    @mock.patch.object(editor.idleConf, 'GetOption')
    def test_set_notabs_indentwidth(self, mock_get_option):
        save_usetabs = editwin.usetabs
        save_indentwidth = editwin.indentwidth
        mock_get_option.return_value = 11

        editwin.usetabs = True
        editwin.set_notabs_indentwidth()
        self.assertNotEqual(editwin.indentwidth, 11)

        editwin.usetabs = False
        editwin.set_notabs_indentwidth()
        self.assertEqual(editwin.indentwidth, 11)

        editwin.usetabs = save_usetabs
        editwin.indentwidth = save_indentwidth


if __name__ == '__main__':
    unittest.main(verbosity=2)
