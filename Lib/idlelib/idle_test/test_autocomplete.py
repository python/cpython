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
        self.context_use_ps1 = True


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
        acac = ac.AutoComplete()
        self.assertEqual(acac.editwin, None)

    def test_remove_autocomplete_window(self):
        acp = self.autocomplete
        acp.autocompletewindow = m = Mock(spec=acw.AutoCompleteWindow)
        acp._remove_autocomplete_window()
        m.hide_window.assert_called_once()
        self.assertIsNone(acp.autocompletewindow)

    def test_force_open_completions_event(self):
        # Call _open_completions and break.
        acp = self.autocomplete
        o_cs = Func()
        acp.open_completions = o_cs
        self.assertEqual(acp.force_open_completions_event('event'), 'break')
        self.assertEqual(o_cs.args, (True, False, True))

    def test_autocomplete_event(self):
        Equal = self.assertEqual
        acp = self.autocomplete

        # Result of autocomplete event: If modified tab, None.
        ev = Event(mc_state=True)
        self.assertIsNone(acp.autocomplete_event(ev))
        del ev.mc_state

        # If tab after whitespace, None.
        self.text.insert('1.0', '        """Docstring.\n    ')
        self.assertIsNone(acp.autocomplete_event(ev))
        self.text.delete('1.0', 'end')

        # If active autocomplete window, complete() and 'break'.
        self.text.insert('1.0', 're.')
        acp.autocompletewindow = m = Mock(spec=acw.AutoCompleteWindow)
        m.is_active = Mock(return_value=True)
        Equal(acp.autocomplete_event(ev), 'break')
        m.complete.assert_called_once()
        acp.autocompletewindow = None

        # If no active autocomplete window, open_completions(), None/break.
        o_cs = Func(result=False)
        acp.open_completions = o_cs
        Equal(acp.autocomplete_event(ev), None)
        Equal(o_cs.args, (False, True, True))
        o_cs.result = True
        Equal(acp.autocomplete_event(ev), 'break')
        Equal(o_cs.args, (False, True, True))

    def test_try_open_completions_event(self):
        Equal = self.assertEqual
        acp = self.autocomplete
        trycompletions = acp.try_open_completions_event
        o_c_l = Func()
        acp._open_completions_later = o_c_l

        # If no text or trigger, _open_completions_later not called.
        trycompletions()
        Equal(o_c_l.called, 0)
        self.text.insert('1.0', 're')
        trycompletions()
        Equal(o_c_l.called, 0)

        # _open_completions_later called with COMPLETE_ATTRIBUTES (1).
        self.text.insert('insert', 're.')
        trycompletions()
        Equal(o_c_l.args, ((False, False, False, 1),))

        # _open_completions_later called with COMPLETE_FILES (2).
        self.text.delete('1.0', 'end')
        self.text.insert('1.0', '"./Lib/')
        trycompletions()
        Equal(o_c_l.args, ((False, False, False, 2),))

    def test_open_completions_later(self):
        Equal = self.assertEqual

        # Test after call and autocomplete._delayed_completion_id.
        acp = self.autocomplete
        after = Func(result='after1')
        acp.text.after = after
        acp._delayed_completion_id = None
        acp._open_completions_later('dummy1')
        Equal(after.args,
              (acp.popupwait, acp._delayed_open_completions, 'dummy1'))
        cb1 = acp._delayed_completion_id
        Equal(cb1, 'after1')

        # Test that cb1 is cancelled and cb2 is new.
        after.result = 'after2'
        acp.text.after_cancel = Func()
        acp._open_completions_later('dummy2')
        Equal(after.args,
              (acp.popupwait, acp._delayed_open_completions, 'dummy2'))
        Equal(self.text.after_cancel.args, (cb1,))
        cb2 = acp._delayed_completion_id
        Equal(cb2, 'after2')

    def test_delayed_open_completions(self):
        Equal = self.assertEqual
        acp = self.autocomplete
        o_c = Func()
        acp.open_completions = o_c
        self.text.delete('1.0', 'end')
        self.text.insert('1.0', '"dict.')


        # Set autocomplete._delayed_completion_id to None.
        # Text index changed, don't call open_completions.
        acp._delayed_completion_id = 'after'
        acp._delayed_completion_index = self.text.index('insert+1c')
        acp._delayed_open_completions('dummy')
        self.assertIsNone(acp._delayed_completion_id)
        Equal(acp.open_completions.called, 0)

        # Text index unchanged, call open_completions.
        acp._delayed_completion_index = self.text.index('insert')
        acp._delayed_open_completions((1, 2, 3, ac.COMPLETE_FILES))
        self.assertEqual(acp.open_completions.args, (1, 2, 3, 2))

    def test_open_completions(self):
        # Test completions of files and attributes as well as non-completion
        # of errors.
        # add 're.', others to complete coverage
        acp = self.autocomplete
        self.text.insert('1.0', 'pr')
        self.assertIsNone(acp.autocompletewindow)
        self.assertTrue(acp.open_completions(False, True, True))
        self.assertIsInstance(acp.autocompletewindow, acw.AutoCompleteWindow)
        self.text.delete('1.0', 'end')

        # Test files.
        self.text.insert('1.0', '"t')
        # When run under regrtest, not comp_lists[0] (small)
        #self.assertTrue(self.autocomplete.open_completions(False, True, True))
        self.text.delete('1.0', 'end')

        # Test with blank will fail.
        self.assertFalse(acp.open_completions(False, True, True))

        # Test with only string quote will fail.
        self.text.insert('1.0', '"')
        self.assertFalse(acp.open_completions(False, True, True))
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
