import unittest
from test.support import requires
from tkinter import Tk, Text, TclError

import idlelib.AutoComplete as ac
import idlelib.AutoCompleteWindow as acw
import idlelib.macosxSupport as mac
from idlelib.idle_test.mock_idle import Func
from idlelib.idle_test.mock_tk import Event

class AutoCompleteWindow:
    def complete():
        return

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
        mac.setupApp(cls.root, None)
        cls.text = Text(cls.root)
        cls.editor = DummyEditwin(cls.root, cls.text)

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.text
        del cls.editor
        del cls.root

    def setUp(self):
        self.editor.text.delete('1.0', 'end')
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
        # Test that force_open_completions_event calls _open_completions
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

        # _open_completions_later should not be called with no text in editor
        trycompletions('event')
        Equal(o_c_l.args, None)

        # _open_completions_later should be called with COMPLETE_ATTRIBUTES (1)
        self.text.insert('1.0', 're.')
        trycompletions('event')
        Equal(o_c_l.args, (False, False, False, 1))

        # _open_completions_later should be called with COMPLETE_FILES (2)
        self.text.delete('1.0', 'end')
        self.text.insert('1.0', '"./Lib/')
        trycompletions('event')
        Equal(o_c_l.args, (False, False, False, 2))

    def test_autocomplete_event(self):
        Equal = self.assertEqual
        autocomplete = self.autocomplete

        # Test that the autocomplete event is ignored if user is pressing a
        # modifier key in addition to the tab key
        ev = Event(mc_state=True)
        self.assertIsNone(autocomplete.autocomplete_event(ev))
        del ev.mc_state

        # If autocomplete window is open, complete() method is called
        testwin = self.autocomplete._make_autocomplete_window()
        self.text.insert('1.0', 're.')
        Equal(self.autocomplete.autocomplete_event(ev), 'break')

        # If autocomplete window is not active or does not exist,
        # open_completions is called. Return depends on its return.
        autocomplete._remove_autocomplete_window()
        o_cs = Func()  # .result = None
        autocomplete.open_completions = o_cs
        Equal(self.autocomplete.autocomplete_event(ev), None)
        Equal(o_cs.args, (False, True, True))
        o_cs.result = True
        Equal(self.autocomplete.autocomplete_event(ev), 'break')
        Equal(o_cs.args, (False, True, True))

    def test_open_completions_later(self):
        # Test that autocomplete._delayed_completion_id is set
        pass

    def test_delayed_open_completions(self):
        # Test that autocomplete._delayed_completion_id set to None and that
        # open_completions only called if insertion index is the same as
        # _delayed_completion_index
        pass

    def test_open_completions(self):
        # Test completions of files and attributes as well as non-completion
        # of errors
        pass

    def test_fetch_completions(self):
        # Test that fetch_completions returns 2 lists:
        # For attribute completion, a large list containing all variables, and
        # a small list containing non-private variables.
        # For file completion, a large list containing all files in the path,
        # and a small list containing files that do not start with '.'
        pass

    def test_get_entity(self):
        # Test that a name is in the namespace of sys.modules and
        # __main__.__dict__
        pass


if __name__ == '__main__':
    unittest.main(verbosity=2)
