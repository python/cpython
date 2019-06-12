"""Unittest for idlelib.LineNumber"""
import unittest
from test.support import requires
import tkinter as tk

from idlelib.delegator import Delegator
from idlelib.percolator import Percolator
from idlelib.linenumbers import LineNumbers


class Dummy_editwin:
    def __init__(self, text):
        self.text = text
        self.per = Percolator(text)
        self.undo = Delegator()
        self.per.insertfilter(self.undo)

    def setvar(self, name, value):
        pass


class LineNumberTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()
        cls.text = tk.Text(cls.root)
        cls.editwin = Dummy_editwin(cls.text)
        cls.editwin.vbar = tk.Scrollbar(cls.root)

    @classmethod
    def tearDownClass(cls):
        cls.editwin.per.close()
        cls.text.destroy()
        cls.root.destroy()
        del cls.text, cls.root

    def setUp(self):
        self.linenumber = LineNumbers(self.editwin)

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def test_init_empty(self):
        get = self.linenumber.sidebar_text.get
        self.assertEqual(get('1.0', 'end'), '1\n')

    def test_init_not_empty(self):
        self.text.insert('insert', 'foo bar\n'*3)
        linenumber = LineNumbers(self.editwin)
        get = linenumber.sidebar_text.get
        self.assertEqual(get('1.0', 'end'), '1\n2\n3\n4\n')

    def test_toggle_linenumbering(self):
        self.linenumber.side = tk.LEFT
        self.linenumber.toggle_line_numbers_event('')
        self.assertEqual(self.linenumber.side, None)

        self.linenumber.side = None
        self.linenumber.toggle_line_numbers_event('')
        self.assertEqual(self.linenumber.side, tk.LEFT)

    def test_insert(self):
        text = self.editwin.text
        get = self.linenumber.sidebar_text.get
        config = self.linenumber.sidebar_text.config

        text.insert('insert', 'foobar')
        self.assertEqual(get('1.0', 'end'), '1\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

        text.insert('insert', '\nfoo')
        self.assertEqual(get('1.0', 'end'), '1\n2\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

        text.insert('insert', 'hello\n'*2)
        self.assertEqual(get('1.0', 'end'), '1\n2\n3\n4\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

        text.insert('insert', '\nworld')
        self.assertEqual(get('1.0', 'end'), '1\n2\n3\n4\n5\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

    def test_delete(self):
        text = self.editwin.text
        get = self.linenumber.sidebar_text.get
        config = self.linenumber.sidebar_text.config

        text.insert('insert', 'foobar')
        text.delete('1.1', '1.3')
        self.assertEqual(get('1.0', 'end'), '1\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

        text.insert('insert', 'foo\n'*2)
        text.delete('3.1')
        text.delete('2.0', '2.end')
        self.assertEqual(get('1.0', 'end'), '1\n2\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

        text.delete('1.3', 'end')
        self.assertEqual(get('1.0', 'end'), '1\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

        text.delete('1.0', 'end')
        self.assertEqual(get('1.0', 'end'), '1\n')
        self.assertEqual(config()['state'][-1], tk.DISABLED)

    def test_sidebar_text_width(self):
        """
        Test that linenumber text widget is always at the minimum
        width
        """
        def get_width():
            return self.linenumber.sidebar_text.config()['width'][-1]
        text = self.text

        self.assertEqual(get_width(), 1)

        text.insert('insert', 'foo')
        self.assertEqual(get_width(), 1)

        text.insert('insert', 'foo\n'*10)
        self.assertEqual(get_width(), 2)

        text.insert('insert', 'foo\n'*100)
        self.assertEqual(get_width(), 3)

        text.delete('50.0', 'end')
        self.assertEqual(get_width(), 2)

        text.delete('5.0', 'end')
        self.assertEqual(get_width(), 1)

        text.delete('1.0', 'end')
        self.assertEqual(get_width(), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
