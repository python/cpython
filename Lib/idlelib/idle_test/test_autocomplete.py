"Test autocomplete, coverage 93%."

import unittest
from unittest.mock import Mock, patch
from test.support import requires
from tkinter import Tk, Text
import os
import __main__

import idlelib.autocomplete as ac
import idlelib.autocomplete_w as acw
from idlelib.idle_test.mock_idle import Func
import idlelib.idle_test.mock_tk as mock_tk


class DummyEditwin:
    def __init__(self, root, text):
        self.root = root
        self.text = text
        self.indentwidth = 8
        self.tabwidth = 8
        self.prompt_last_line = '>>>'  # Currently not used by autocomplete.


class AutoCompleteTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.text = Text(cls.root)
        cls.editor = DummyEditwin(cls.root, cls.text)

    @classmethod
    def tearDownClass(cls):
        del cls.editor, cls.text
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.text.delete('1.0', 'end')
        self.autocomplete = ac.AutoComplete(self.editor)

    def test_init(self):
        self.assertEqual(self.autocomplete.editwin, self.editor)
        self.assertEqual(self.autocomplete.text, self.text)

    def test_make_autocomplete_window(self):
        testwin = self.autocomplete._make_autocomplete_window()
        self.assertIsInstance(testwin, acw.AutoCompleteWindow)

    def test_remove_autocomplete_window(self):
        acp = self.autocomplete
        acp.autocompletewindow = m = Mock()
        acp._remove_autocomplete_window()
        m.hide_window.assert_called_once()
        self.assertIsNone(acp.autocompletewindow)

    def test_force_open_completions_event(self):
        # Call _open_completions and break.
        acp = self.autocomplete
        open_c = Func()
        acp.open_completions = open_c
        self.assertEqual(acp.force_open_completions_event('event'), 'break')
        self.assertEqual(open_c.args[0], ac.FORCE)

    def test_autocomplete_event(self):
        Equal = self.assertEqual
        acp = self.autocomplete

        # Result of autocomplete event: If modified tab, None.
        ev = mock_tk.Event(mc_state=True)
        self.assertIsNone(acp.autocomplete_event(ev))
        del ev.mc_state

        # If tab after whitespace, None.
        self.text.insert('1.0', '        """Docstring.\n    ')
        self.assertIsNone(acp.autocomplete_event(ev))
        self.text.delete('1.0', 'end')

        # If active autocomplete window, complete() and 'break'.
        self.text.insert('1.0', 're.')
        acp.autocompletewindow = mock = Mock()
        mock.is_active = Mock(return_value=True)
        Equal(acp.autocomplete_event(ev), 'break')
        mock.complete.assert_called_once()
        acp.autocompletewindow = None

        # If no active autocomplete window, open_completions(), None/break.
        open_c = Func(result=False)
        acp.open_completions = open_c
        Equal(acp.autocomplete_event(ev), None)
        Equal(open_c.args[0], ac.TAB)
        open_c.result = True
        Equal(acp.autocomplete_event(ev), 'break')
        Equal(open_c.args[0], ac.TAB)

    def test_try_open_completions_event(self):
        Equal = self.assertEqual
        text = self.text
        acp = self.autocomplete
        trycompletions = acp.try_open_completions_event
        after = Func(result='after1')
        acp.text.after = after

        # If no text or trigger, after not called.
        trycompletions()
        Equal(after.called, 0)
        text.insert('1.0', 're')
        trycompletions()
        Equal(after.called, 0)

        # Attribute needed, no existing callback.
        text.insert('insert', ' re.')
        acp._delayed_completion_id = None
        trycompletions()
        Equal(acp._delayed_completion_index, text.index('insert'))
        Equal(after.args,
              (acp.popupwait, acp._delayed_open_completions, ac.TRY_A))
        cb1 = acp._delayed_completion_id
        Equal(cb1, 'after1')

        # File needed, existing callback cancelled.
        text.insert('insert', ' "./Lib/')
        after.result = 'after2'
        cancel = Func()
        acp.text.after_cancel = cancel
        trycompletions()
        Equal(acp._delayed_completion_index, text.index('insert'))
        Equal(cancel.args, (cb1,))
        Equal(after.args,
              (acp.popupwait, acp._delayed_open_completions, ac.TRY_F))
        Equal(acp._delayed_completion_id, 'after2')

    def test_delayed_open_completions(self):
        Equal = self.assertEqual
        acp = self.autocomplete
        open_c = Func()
        acp.open_completions = open_c
        self.text.insert('1.0', '"dict.')

        # Set autocomplete._delayed_completion_id to None.
        # Text index changed, don't call open_completions.
        acp._delayed_completion_id = 'after'
        acp._delayed_completion_index = self.text.index('insert+1c')
        acp._delayed_open_completions('dummy')
        self.assertIsNone(acp._delayed_completion_id)
        Equal(open_c.called, 0)

        # Text index unchanged, call open_completions.
        acp._delayed_completion_index = self.text.index('insert')
        acp._delayed_open_completions((1, 2, 3, ac.FILES))
        self.assertEqual(open_c.args[0], (1, 2, 3, ac.FILES))

    def test_oc_cancel_comment(self):
        none = self.assertIsNone
        acp = self.autocomplete

        # Comment is in neither code or string.
        acp._delayed_completion_id = 'after'
        after = Func(result='after')
        acp.text.after_cancel = after
        self.text.insert('1.0', '# comment')
        none(acp.open_completions(ac.TAB))  # From 'else' after 'elif'.
        none(acp._delayed_completion_id)

    def test_oc_no_list(self):
        acp = self.autocomplete
        fetch = Func(result=([],[]))
        acp.fetch_completions = fetch
        self.text.insert('1.0', 'object')
        self.assertIsNone(acp.open_completions(ac.TAB))
        self.text.insert('insert', '.')
        self.assertIsNone(acp.open_completions(ac.TAB))
        self.assertEqual(fetch.called, 2)


    def test_open_completions_none(self):
        # Test other two None returns.
        none = self.assertIsNone
        acp = self.autocomplete

        # No object for attributes or need call not allowed.
        self.text.insert('1.0', '.')
        none(acp.open_completions(ac.TAB))
        self.text.insert('insert', ' int().')
        none(acp.open_completions(ac.TAB))

        # Blank or quote trigger 'if complete ...'.
        self.text.delete('1.0', 'end')
        self.assertFalse(acp.open_completions(ac.TAB))
        self.text.insert('1.0', '"')
        self.assertFalse(acp.open_completions(ac.TAB))
        self.text.delete('1.0', 'end')


class FetchCompletionsTest(unittest.TestCase):
    def setUp(self):
        self.root = Mock()
        self.text = mock_tk.Text()
        self.editor = DummyEditwin(self.root, self.text)
        self.autocomplete = ac.AutoComplete(self.editor)

    def test_attrs(self):
        # Test that fetch_completions returns 2 lists:
        # 1. a small list containing all non-private variables
        # 2. a big list containing all variables
        acp = self.autocomplete

        # Test attributes
        s, b = acp.fetch_completions('', ac.ATTRS)
        self.assertLess(set(s), set(b))
        self.assertTrue(all([not x.startswith('_') for x in s]))

        if __main__.__file__ != ac.__file__:
            self.assertNotIn('AutoComplete', s)  # See issue 36405.

        # Test smalll should respect __all__.
        with patch.dict('__main__.__dict__', {'__all__': ['a', 'b']}):
            s, b = acp.fetch_completions('', ac.ATTRS)
            self.assertEqual(s, ['a', 'b'])
            self.assertIn('__name__', b)    # From __main__.__dict__
            self.assertIn('sum', b)         # From __main__.__builtins__.__dict__

        # Test attributes with name entity.
        mock = Mock()
        mock._private = Mock()
        with patch.dict('__main__.__dict__', {'foo': mock}):
            s, b = acp.fetch_completions('foo', ac.ATTRS)
            self.assertNotIn('_private', s)
            self.assertIn('_private', b)
            self.assertEqual(s, [i for i in sorted(dir(mock)) if i[0] != '_'])
            self.assertEqual(b, sorted(dir(mock)))

    def test_files(self):
        # Test that fetch_completions returns 2 lists:
        # 1. a small list containing files that do not start with '.'.
        # 2. a big list containing all files in the path
        acp = self.autocomplete

        def _listdir(path):
            # This will be patch and used in fetch_completions.
            if path == '.':
                return ['foo', 'bar', '.hidden']
            return ['monty', 'python', '.hidden']

        with patch.object(os, 'listdir', _listdir):
            s, b = acp.fetch_completions('', ac.FILES)
            self.assertEqual(s, ['bar', 'foo'])
            self.assertEqual(b, ['.hidden', 'bar', 'foo'])

            s, b = acp.fetch_completions('~', ac.FILES)
            self.assertEqual(s, ['monty', 'python'])
            self.assertEqual(b, ['.hidden', 'monty', 'python'])

    def test_get_entity(self):
        # Test that a name is in the namespace of sys.modules and
        # __main__.__dict__.
        acp = self.autocomplete

        self.assertEqual(acp.get_entity('int'), int)

        # Test name from sys.modules.
        mock = Mock()
        with patch.dict('sys.modules', {'tempfile': mock}):
            self.assertEqual(acp.get_entity('tempfile'), mock)

        # Test name from __main__.__dict__.
        di = {'foo': 10, 'bar': 20}
        with patch.dict('__main__.__dict__', {'d': di}):
            self.assertEqual(acp.get_entity('d'), di)

        # Test name not in namespace.
        with patch.dict('__main__.__dict__', {}):
            with self.assertRaises(NameError):
                acp.get_entity('doesnt_exist')

    def test_open_completions(self):
        # Test completions of files and attributes.
        acp = self.autocomplete

        mock_acw = None
        def make_acw():
            nonlocal mock_acw
            mock_acw = Mock()
            mock_acw.show_window = Mock(return_value=False,
                                        spec=acw.AutoCompleteWindow.show_window)
            return mock_acw

        # Test attributes
        self.text.insert('1.0', 'int.')
        with patch.object(acp, '_make_autocomplete_window', make_acw):
            acp.open_completions(ac.TAB)
        self.assertIs(acp.autocompletewindow, mock_acw)
        mock_acw.show_window.assert_called_once()
        comp_lists, index, complete, mode, userWantsWin = \
            mock_acw.show_window.call_args[0]
        self.assertEqual(mode, ac.ATTRS)
        self.assertIn('bit_length', comp_lists[0])
        self.assertIn('bit_length', comp_lists[1])
        self.assertNotIn('__index__', comp_lists[0])
        self.assertIn('__index__', comp_lists[1])
        self.text.delete('1.0', 'end')

        # Test files.
        self.text.insert('1.0', '"t')
        def _listdir(path):
            return ['.hidden', 'monty', 'python']
        with patch.object(acp, '_make_autocomplete_window', make_acw):
            with patch('os.listdir', _listdir):
                acp.open_completions(ac.FORCE)
        mock_acw.show_window.assert_called_once()
        comp_lists, index, complete, mode, userWantsWin = \
            mock_acw.show_window.call_args[0]
        self.assertEqual(mode, ac.FILES)
        self.assertEqual(comp_lists[0], ['monty', 'python'])
        self.assertEqual(comp_lists[1], ['.hidden', 'monty', 'python'])
        self.text.delete('1.0', 'end')


if __name__ == '__main__':
    unittest.main(verbosity=2)
