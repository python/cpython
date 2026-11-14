"Test replace, coverage 78%."

from idlelib.replace import ReplaceDialog
import unittest
from test.support import requires
requires('gui')
from tkinter import Tk, Text

from unittest.mock import Mock
from idlelib.idle_test.mock_tk import Mbox
import idlelib.searchengine as se

orig_mbox = se.messagebox
showerror = Mbox.showerror


class ReplaceDialogTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()
        se.messagebox = Mbox
        cls.engine = se.SearchEngine(cls.root)
        cls.dialog = ReplaceDialog(cls.root, cls.engine)
        cls.dialog.bell = lambda: None
        cls.dialog.ok = Mock()
        cls.text = Text(cls.root)
        cls.text.undo_block_start = Mock()
        cls.text.undo_block_stop = Mock()
        cls.dialog.text = cls.text

    @classmethod
    def tearDownClass(cls):
        se.messagebox = orig_mbox
        del cls.text, cls.dialog, cls.engine
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.text.insert('insert', 'This is a sample sTring')

    def tearDown(self):
        self.engine.patvar.set('')
        self.dialog.replvar.set('')
        self.engine.wordvar.set(False)
        self.engine.casevar.set(False)
        self.engine.revar.set(False)
        self.engine.wrapvar.set(True)
        self.engine.backvar.set(False)
        showerror.title = ''
        showerror.message = ''
        self.text.delete('1.0', 'end')

    def test_replace_simple(self):
        # Test replace function with all options at default setting.
        # Wrap around - True
        # Regular Expression - False
        # Match case - False
        # Match word - False
        # Direction - Forwards
        text = self.text
        equal = self.assertEqual
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace = self.dialog.replace_it

        # test accessor method
        self.engine.setpat('asdf')
        equal(self.engine.getpat(), pv.get())

        # text found and replaced
        pv.set('a')
        rv.set('asdf')
        replace()
        equal(text.get('1.8', '1.12'), 'asdf')

        # don't "match word" case
        text.mark_set('insert', '1.0')
        pv.set('is')
        rv.set('hello')
        replace()
        equal(text.get('1.2', '1.7'), 'hello')

        # don't "match case" case
        pv.set('string')
        rv.set('world')
        replace()
        equal(text.get('1.23', '1.28'), 'world')

        # without "regular expression" case
        text.mark_set('insert', 'end')
        text.insert('insert', '\nline42:')
        before_text = text.get('1.0', 'end')
        pv.set(r'[a-z][\d]+')
        replace()
        after_text = text.get('1.0', 'end')
        equal(before_text, after_text)

        # test with wrap around selected and complete a cycle
        text.mark_set('insert', '1.9')
        pv.set('i')
        rv.set('j')
        replace()
        equal(text.get('1.8'), 'i')
        equal(text.get('2.1'), 'j')
        replace()
        equal(text.get('2.1'), 'j')
        equal(text.get('1.8'), 'j')
        before_text = text.get('1.0', 'end')
        replace()
        after_text = text.get('1.0', 'end')
        equal(before_text, after_text)

        # text not found
        before_text = text.get('1.0', 'end')
        pv.set('foobar')
        replace()
        after_text = text.get('1.0', 'end')
        equal(before_text, after_text)

        # test access method
        self.dialog.find_it(0)

    def test_replace_wrap_around(self):
        text = self.text
        equal = self.assertEqual
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace = self.dialog.replace_it
        self.engine.wrapvar.set(False)

        # replace candidate found both after and before 'insert'
        text.mark_set('insert', '1.4')
        pv.set('i')
        rv.set('j')
        replace()
        equal(text.get('1.2'), 'i')
        equal(text.get('1.5'), 'j')
        replace()
        equal(text.get('1.2'), 'i')
        equal(text.get('1.20'), 'j')
        replace()
        equal(text.get('1.2'), 'i')

        # replace candidate found only before 'insert'
        text.mark_set('insert', '1.8')
        pv.set('is')
        before_text = text.get('1.0', 'end')
        replace()
        after_text = text.get('1.0', 'end')
        equal(before_text, after_text)

    def test_replace_whole_word(self):
        text = self.text
        equal = self.assertEqual
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace = self.dialog.replace_it
        self.engine.wordvar.set(True)

        pv.set('is')
        rv.set('hello')
        replace()
        equal(text.get('1.0', '1.4'), 'This')
        equal(text.get('1.5', '1.10'), 'hello')

    def test_replace_match_case(self):
        equal = self.assertEqual
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace = self.dialog.replace_it
        self.engine.casevar.set(True)

        before_text = self.text.get('1.0', 'end')
        pv.set('this')
        rv.set('that')
        replace()
        after_text = self.text.get('1.0', 'end')
        equal(before_text, after_text)

        pv.set('This')
        replace()
        equal(text.get('1.0', '1.4'), 'that')

    def test_replace_regex(self):
        equal = self.assertEqual
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace = self.dialog.replace_it
        self.engine.revar.set(True)

        before_text = text.get('1.0', 'end')
        pv.set(r'[a-z][\d]+')
        rv.set('hello')
        replace()
        after_text = text.get('1.0', 'end')
        equal(before_text, after_text)

        text.insert('insert', '\nline42')
        replace()
        equal(text.get('2.0', '2.8'), 'linhello')

        pv.set('')
        replace()
        self.assertIn('error', showerror.title)
        self.assertIn('Empty', showerror.message)

        pv.set(r'[\d')
        replace()
        self.assertIn('error', showerror.title)
        self.assertIn('Pattern', showerror.message)

        showerror.title = ''
        showerror.message = ''
        pv.set('[a]')
        rv.set('test\\')
        replace()
        self.assertIn('error', showerror.title)
        self.assertIn('Invalid Replace Expression', showerror.message)

        # test access method
        self.engine.setcookedpat("?")
        equal(pv.get(), "\\?")

    def test_replace_backwards(self):
        equal = self.assertEqual
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace = self.dialog.replace_it
        self.engine.backvar.set(True)

        text.insert('insert', '\nis as ')

        pv.set('is')
        rv.set('was')
        replace()
        equal(text.get('1.2', '1.4'), 'is')
        equal(text.get('2.0', '2.3'), 'was')
        replace()
        equal(text.get('1.5', '1.8'), 'was')
        replace()
        equal(text.get('1.2', '1.5'), 'was')

    def test_replace_all(self):
        # The default mode, forward with wrap around, replaces every
        # match, both below and above the current position.
        equal = self.assertEqual
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace_all = self.dialog.replace_all

        text.delete('1.0', 'end')
        text.insert('1.0', 'a\na\na\n')
        text.mark_set('insert', '2.1')
        pv.set('a')
        rv.set('b')
        replace_all()
        equal(text.get('1.0', '3.end'), 'b\nb\nb')  # Wrapped around.

        # An empty regular expression is reported as an error.
        self.engine.revar.set(True)
        pv.set('')
        replace_all()
        self.assertIn('error', showerror.title)
        self.assertIn('Empty', showerror.message)

        # An invalid replacement expression is reported as an error,
        # and nothing is replaced.
        text.delete('1.0', 'end')
        text.insert('1.0', 'asT')
        pv.set('[s][T]')
        rv.set('\\')
        replace_all()
        self.assertIn('error', showerror.title)
        self.assertIn('Invalid Replace Expression', showerror.message)
        equal(text.get('1.0', '1.end'), 'asT')

        # A pattern that is not present replaces nothing.
        self.engine.revar.set(False)
        text.delete('1.0', 'end')
        text.insert('1.0', 'unchanged')
        pv.set('text which is not present')
        rv.set('foobar')
        replace_all()
        equal(text.get('1.0', '1.end'), 'unchanged')

    def test_replace_all_backwards_no_wrap(self):
        # gh-71956: 'up' without wrap replaces all matches from the start
        # of the text down to the current position, not just one up.
        equal = self.assertEqual
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace_all = self.dialog.replace_all
        self.engine.backvar.set(True)
        self.engine.wrapvar.set(False)

        text.delete('1.0', 'end')
        text.insert('1.0', 'a\na\na\n')
        text.mark_set('insert', '2.1')
        pv.set('a')
        rv.set('b')
        replace_all()
        equal(text.get('1.0', '1.end'), 'b')  # Above the cursor.
        equal(text.get('2.0', '2.end'), 'b')  # At the cursor.
        equal(text.get('3.0', '3.end'), 'a')  # Below the cursor, untouched.

    def test_replace_all_forwards_no_wrap(self):
        # 'down' without wrap replaces all matches from the current
        # position to the end of the text, and none before it.
        equal = self.assertEqual
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace_all = self.dialog.replace_all
        self.engine.backvar.set(False)
        self.engine.wrapvar.set(False)

        text.delete('1.0', 'end')
        text.insert('1.0', 'a\na\na\n')
        text.mark_set('insert', '2.1')
        pv.set('a')
        rv.set('b')
        replace_all()
        equal(text.get('1.0', '1.end'), 'a')  # Before the cursor, untouched.
        equal(text.get('2.0', '2.end'), 'a')  # Before the cursor, untouched.
        equal(text.get('3.0', '3.end'), 'b')  # After the cursor.

    def test_replace_all_backwards_wrap(self):
        # With wrap around, an 'up' search also replaces every match.
        equal = self.assertEqual
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace_all = self.dialog.replace_all
        self.engine.backvar.set(True)
        self.engine.wrapvar.set(True)

        text.delete('1.0', 'end')
        text.insert('1.0', 'a\na\na\n')
        text.mark_set('insert', '2.1')
        pv.set('a')
        rv.set('b')
        replace_all()
        equal(text.get('1.0', '3.end'), 'b\nb\nb')

    def test_default_command(self):
        text = self.text
        pv = self.engine.patvar
        rv = self.dialog.replvar
        replace_find = self.dialog.default_command
        equal = self.assertEqual

        pv.set('This')
        rv.set('was')
        replace_find()
        equal(text.get('sel.first', 'sel.last'), 'was')

        self.engine.revar.set(True)
        pv.set('')
        replace_find()


if __name__ == '__main__':
    unittest.main(verbosity=2)
