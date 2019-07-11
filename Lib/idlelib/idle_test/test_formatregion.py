"Test formatregion, coverage 100%."

from idlelib import formatregion
import unittest
from unittest import mock
from test.support import requires
from tkinter import Tk, Text
from idlelib.editor import EditorWindow


code_sample = """\

class C1():
    # Class comment.
    def __init__(self, a, b):
        self.a = a
        self.b = b

    def compare(self):
        if a > b:
            return a
        elif a < b:
            return b
        else:
            return None
"""


class DummyEditwin:
    def __init__(self, root, text):
        self.root = root
        self.text = text
        self.indentwidth = 4
        self.tabwidth = 4
        self.usetabs = False
        self.context_use_ps1 = True

    _make_blanks = EditorWindow._make_blanks
    get_selection_indices = EditorWindow.get_selection_indices


class FormatRegionTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.text = Text(cls.root)
        cls.text.undo_block_start = mock.Mock()
        cls.text.undo_block_stop = mock.Mock()
        cls.editor = DummyEditwin(cls.root, cls.text)
        cls.formatter = formatregion.FormatRegion(cls.editor)

    @classmethod
    def tearDownClass(cls):
        del cls.text, cls.formatter, cls.editor
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.text.insert('1.0', code_sample)

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def test_get_region(self):
        get = self.formatter.get_region
        text = self.text
        eq = self.assertEqual

        # Add selection.
        text.tag_add('sel', '7.0', '10.0')
        eq(get(), ('7.0', '10.0',
                   '\n    def compare(self):\n        if a > b:\n',
                   ['', '    def compare(self):', '        if a > b:', '']))

        # Remove selection.
        text.tag_remove('sel', '1.0', 'end')
        eq(get(), ('15.0', '16.0', '\n', ['', '']))

    def test_set_region(self):
        set_ = self.formatter.set_region
        text = self.text
        eq = self.assertEqual

        save_bell = text.bell
        text.bell = mock.Mock()
        line6 = code_sample.splitlines()[5]
        line10 = code_sample.splitlines()[9]

        text.tag_add('sel', '6.0', '11.0')
        head, tail, chars, lines = self.formatter.get_region()

        # No changes.
        set_(head, tail, chars, lines)
        text.bell.assert_called_once()
        eq(text.get('6.0', '11.0'), chars)
        eq(text.get('sel.first', 'sel.last'), chars)
        text.tag_remove('sel', '1.0', 'end')

        # Alter selected lines by changing lines and adding a newline.
        newstring = 'added line 1\n\n\n\n'
        newlines = newstring.split('\n')
        set_('7.0', '10.0', chars, newlines)
        # Selection changed.
        eq(text.get('sel.first', 'sel.last'), newstring)
        # Additional line added, so last index is changed.
        eq(text.get('7.0', '11.0'), newstring)
        # Before and after lines unchanged.
        eq(text.get('6.0', '7.0-1c'), line6)
        eq(text.get('11.0', '12.0-1c'), line10)
        text.tag_remove('sel', '1.0', 'end')

        text.bell = save_bell

    def test_indent_region_event(self):
        indent = self.formatter.indent_region_event
        text = self.text
        eq = self.assertEqual

        text.tag_add('sel', '7.0', '10.0')
        indent()
        # Blank lines aren't affected by indent.
        eq(text.get('7.0', '10.0'), ('\n        def compare(self):\n            if a > b:\n'))

    def test_dedent_region_event(self):
        dedent = self.formatter.dedent_region_event
        text = self.text
        eq = self.assertEqual

        text.tag_add('sel', '7.0', '10.0')
        dedent()
        # Blank lines aren't affected by dedent.
        eq(text.get('7.0', '10.0'), ('\ndef compare(self):\n    if a > b:\n'))

    def test_comment_region_event(self):
        comment = self.formatter.comment_region_event
        text = self.text
        eq = self.assertEqual

        text.tag_add('sel', '7.0', '10.0')
        comment()
        eq(text.get('7.0', '10.0'), ('##\n##    def compare(self):\n##        if a > b:\n'))

    def test_uncomment_region_event(self):
        comment = self.formatter.comment_region_event
        uncomment = self.formatter.uncomment_region_event
        text = self.text
        eq = self.assertEqual

        text.tag_add('sel', '7.0', '10.0')
        comment()
        uncomment()
        eq(text.get('7.0', '10.0'), ('\n    def compare(self):\n        if a > b:\n'))

        # Only remove comments at the beginning of a line.
        text.tag_remove('sel', '1.0', 'end')
        text.tag_add('sel', '3.0', '4.0')
        uncomment()
        eq(text.get('3.0', '3.end'), ('    # Class comment.'))

        self.formatter.set_region('3.0', '4.0', '', ['# Class comment.', ''])
        uncomment()
        eq(text.get('3.0', '3.end'), (' Class comment.'))

    @mock.patch.object(formatregion.FormatRegion, "_asktabwidth")
    def test_tabify_region_event(self, _asktabwidth):
        tabify = self.formatter.tabify_region_event
        text = self.text
        eq = self.assertEqual

        text.tag_add('sel', '7.0', '10.0')
        # No tabwidth selected.
        _asktabwidth.return_value = None
        self.assertIsNone(tabify())

        _asktabwidth.return_value = 3
        self.assertIsNotNone(tabify())
        eq(text.get('7.0', '10.0'), ('\n\t def compare(self):\n\t\t  if a > b:\n'))

    @mock.patch.object(formatregion.FormatRegion, "_asktabwidth")
    def test_untabify_region_event(self, _asktabwidth):
        untabify = self.formatter.untabify_region_event
        text = self.text
        eq = self.assertEqual

        text.tag_add('sel', '7.0', '10.0')
        # No tabwidth selected.
        _asktabwidth.return_value = None
        self.assertIsNone(untabify())

        _asktabwidth.return_value = 2
        self.formatter.tabify_region_event()
        _asktabwidth.return_value = 3
        self.assertIsNotNone(untabify())
        eq(text.get('7.0', '10.0'), ('\n      def compare(self):\n            if a > b:\n'))

    @mock.patch.object(formatregion, "askinteger")
    def test_ask_tabwidth(self, askinteger):
        ask = self.formatter._asktabwidth
        askinteger.return_value = 10
        self.assertEqual(ask(), 10)


if __name__ == '__main__':
    unittest.main(verbosity=2)
