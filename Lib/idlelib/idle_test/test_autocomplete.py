"Test autocomplete, coverage 87%."

import unittest
from unittest.mock import Mock, patch
from test.support import requires
from tkinter import Tk, Text
import os
import __main__

import idlelib.autocomplete as ac
import idlelib.autocomplete_w as acw
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Event


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

    def test_make_autocomplete_window(self):
        testwin = self.autocomplete._make_autocomplete_window()
        self.assertIsInstance(testwin, acw.AutoCompleteWindow)

    def test_remove_autocomplete_window(self):
        self.autocomplete.autocompletewindow = (
            self.autocomplete._make_autocomplete_window())
        self.autocomplete._remove_autocomplete_window()
        self.assertIsNone(self.autocomplete.autocompletewindow)

    def test_force_open_completions_event(self):
        # Test that force_open_completions_event calls _open_completions.
        o_cs = Func()
        self.autocomplete.open_completions = o_cs
        self.autocomplete.force_open_completions_event('event')
        self.assertEqual(o_cs.args, (True, False, True))

    def test_try_open_completions_event(self):
        Equal = self.assertEqual
        autocomplete = self.autocomplete
        trycompletions = self.autocomplete.try_open_completions_event
        o_c_l = Func()
        autocomplete._open_completions_later = o_c_l

        # _open_completions_later should not be called with no text in editor.
        trycompletions('event')
        Equal(o_c_l.args, None)

        # _open_completions_later should be called with COMPLETE_ATTRIBUTES (1).
        self.text.insert('1.0', 're.')
        trycompletions('event')
        Equal(o_c_l.args, (False, False, False, 1))

        # _open_completions_later should be called with COMPLETE_FILES (2).
        self.text.delete('1.0', 'end')
        self.text.insert('1.0', '"./Lib/')
        trycompletions('event')
        Equal(o_c_l.args, (False, False, False, 2))

    def test_autocomplete_event(self):
        Equal = self.assertEqual
        autocomplete = self.autocomplete

        # Test that the autocomplete event is ignored if user is pressing a
        # modifier key in addition to the tab key.
        ev = Event(mc_state=True)
        self.assertIsNone(autocomplete.autocomplete_event(ev))
        del ev.mc_state

        # Test that tab after whitespace is ignored.
        self.text.insert('1.0', '        """Docstring.\n    ')
        self.assertIsNone(autocomplete.autocomplete_event(ev))
        self.text.delete('1.0', 'end')

        # If autocomplete window is open, complete() method is called.
        self.text.insert('1.0', 're.')
        # This must call autocomplete._make_autocomplete_window().
        Equal(self.autocomplete.autocomplete_event(ev), 'break')

        # If autocomplete window is not active or does not exist,
        # open_completions is called. Return depends on its return.
        autocomplete._remove_autocomplete_window()
        o_cs = Func()  # .result = None.
        autocomplete.open_completions = o_cs
        Equal(self.autocomplete.autocomplete_event(ev), None)
        Equal(o_cs.args, (False, True, True))
        o_cs.result = True
        Equal(self.autocomplete.autocomplete_event(ev), 'break')
        Equal(o_cs.args, (False, True, True))

    def test_open_completions_later(self):
        # Test that autocomplete._delayed_completion_id is set.
        acp = self.autocomplete
        acp._delayed_completion_id = None
        acp._open_completions_later(False, False, False, ac.COMPLETE_ATTRIBUTES)
        cb1 = acp._delayed_completion_id
        self.assertTrue(cb1.startswith('after'))

        # Test that cb1 is cancelled and cb2 is new.
        acp._open_completions_later(False, False, False, ac.COMPLETE_FILES)
        self.assertNotIn(cb1, self.root.tk.call('after', 'info'))
        cb2 = acp._delayed_completion_id
        self.assertTrue(cb2.startswith('after') and cb2 != cb1)
        self.text.after_cancel(cb2)

    def test_delayed_open_completions(self):
        # Test that autocomplete._delayed_completion_id set to None
        # and that open_completions is not called if the index is not
        # equal to _delayed_completion_index.
        acp = self.autocomplete
        acp.open_completions = Func()
        acp._delayed_completion_id = 'after'
        acp._delayed_completion_index = self.text.index('insert+1c')
        acp._delayed_open_completions(1, 2, 3)
        self.assertIsNone(acp._delayed_completion_id)
        self.assertEqual(acp.open_completions.called, 0)

        # Test that open_completions is called if indexes match.
        acp._delayed_completion_index = self.text.index('insert')
        acp._delayed_open_completions(1, 2, 3, ac.COMPLETE_FILES)
        self.assertEqual(acp.open_completions.args, (1, 2, 3, 2))

    def test_open_completions(self):
        # Test completions of files and attributes as well as non-completion
        # of errors.
        self.text.insert('1.0', 'pr')
        self.assertTrue(self.autocomplete.open_completions(False, True, True))
        self.text.delete('1.0', 'end')

        # Test files.
        self.text.insert('1.0', '"t')
        #self.assertTrue(self.autocomplete.open_completions(False, True, True))
        self.text.delete('1.0', 'end')

        # Test with blank will fail.
        self.assertFalse(self.autocomplete.open_completions(False, True, True))

        # Test with only string quote will fail.
        self.text.insert('1.0', '"')
        self.assertFalse(self.autocomplete.open_completions(False, True, True))
        self.text.delete('1.0', 'end')

    def test_fetch_completions(self):
        # Test that fetch_completions returns 2 lists:
        # For attribute completion, a large list containing all variables, and
        # a small list containing non-private variables.
        # For file completion, a large list containing all files in the path,
        # and a small list containing files that do not start with '.'.
        autocomplete = self.autocomplete
        small, large = self.autocomplete.fetch_completions(
                '', ac.COMPLETE_ATTRIBUTES)
        if __main__.__file__ != ac.__file__:
            self.assertNotIn('AutoComplete', small)  # See issue 36405.

        # Test attributes
        s, b = autocomplete.fetch_completions('', ac.COMPLETE_ATTRIBUTES)
        self.assertLess(len(small), len(large))
        self.assertTrue(all(filter(lambda x: x.startswith('_'), s)))
        self.assertTrue(any(filter(lambda x: x.startswith('_'), b)))

        # Test smalll should respect to __all__.
        with patch.dict('__main__.__dict__', {'__all__': ['a', 'b']}):
            s, b = autocomplete.fetch_completions('', ac.COMPLETE_ATTRIBUTES)
            self.assertEqual(s, ['a', 'b'])
            self.assertIn('__name__', b)    # From __main__.__dict__
            self.assertIn('sum', b)         # From __main__.__builtins__.__dict__

        # Test attributes with name entity.
        mock = Mock()
        mock._private = Mock()
        with patch.dict('__main__.__dict__', {'foo': mock}):
            s, b = autocomplete.fetch_completions('foo', ac.COMPLETE_ATTRIBUTES)
            self.assertNotIn('_private', s)
            self.assertIn('_private', b)
            self.assertEqual(s, [i for i in sorted(dir(mock)) if i[:1] != '_'])
            self.assertEqual(b, sorted(dir(mock)))

        # Test files
        def _listdir(path):
            # This will be patch and used in fetch_completions.
            if path == '.':
                return ['foo', 'bar', '.hidden']
            return ['monty', 'python', '.hidden']

        with patch.object(os, 'listdir', _listdir):
            s, b = autocomplete.fetch_completions('', ac.COMPLETE_FILES)
            self.assertEqual(s, ['bar', 'foo'])
            self.assertEqual(b, ['.hidden', 'bar', 'foo'])

            s, b = autocomplete.fetch_completions('~', ac.COMPLETE_FILES)
            self.assertEqual(s, ['monty', 'python'])
            self.assertEqual(b, ['.hidden', 'monty', 'python'])

    def test_get_entity(self):
        # Test that a name is in the namespace of sys.modules and
        # __main__.__dict__.
        autocomplete = self.autocomplete
        Equal = self.assertEqual

        Equal(self.autocomplete.get_entity('int'), int)

        # Test name from sys.modules.
        mock = Mock()
        with patch.dict('sys.modules', {'tempfile': mock}):
            Equal(autocomplete.get_entity('tempfile'), mock)

        # Test name from __main__.__dict__.
        di = {'foo': 10, 'bar': 20}
        with patch.dict('__main__.__dict__', {'d': di}):
            Equal(autocomplete.get_entity('d'), di)

        # Test name not in namespace.
        with patch.dict('__main__.__dict__', {}):
            with self.assertRaises(NameError):
                autocomplete.get_entity('not_exist')


if __name__ == '__main__':
    unittest.main(verbosity=2)
