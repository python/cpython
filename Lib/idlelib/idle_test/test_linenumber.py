"""Unittest for idlelib.LineNumber"""
import unittest
from test.support import requires
import tkinter as tk
from idlelib.Percolator import Percolator
from idlelib.LineNumber import LineNumber


class Dummy_editwin:
    def __init__(self, text):
        self.text = text
        self.per = Percolator(text)


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
        self.linenumber = LineNumber(self.editwin)

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def test_init_empty(self):
        get = self.linenumber.sidebar_text.get
        self.assertEqual(get('1.0', 'end'), '1\n')

    def test_init_not_empty(self):
        self.text.insert('insert', 'foo bar\n'*3)
        linenumber = LineNumber(self.editwin)
        get = linenumber.sidebar_text.get
        self.assertEqual(get('1.0', 'end'), '1\n2\n3\n4\n')

    def test_toggle_linenumbering(self):
        ENABLED = True
        DISABLED = False

        self.linenumber.state = ENABLED
        self.linenumber.toggle_linenumbering_event('')
        self.assertEqual(self.linenumber.state, DISABLED)

        self.linenumber.state = DISABLED
        self.linenumber.toggle_linenumbering_event('')
        self.assertEqual(self.linenumber.state, ENABLED)

    def test_insert(self):
        text = self.editwin.text
        get = self.linenumber.sidebar_text.get
        config = self.linenumber.sidebar_text.config
        equal = self.assertEqual

        text.insert('insert', 'foobar')
        equal(get('1.0', 'end'), '1\n')
        equal(config()['state'][-1], tk.DISABLED)

        text.insert('insert', '\nfoo')
        equal(get('1.0', 'end'), '1\n2\n')
        equal(config()['state'][-1], tk.DISABLED)

        text.insert('insert', 'hello\n'*2)
        equal(get('1.0', 'end'), '1\n2\n3\n4\n')
        equal(config()['state'][-1], tk.DISABLED)

        text.insert('insert', '\nworld')
        equal(get('1.0', 'end'), '1\n2\n3\n4\n5\n')
        equal(config()['state'][-1], tk.DISABLED)

    def test_delete(self):
        text = self.editwin.text
        get = self.linenumber.sidebar_text.get
        config = self.linenumber.sidebar_text.config
        equal = self.assertEqual

        text.insert('insert', 'foobar')
        text.delete('1.1', '1.3')
        equal(get('1.0', 'end'), '1\n')
        equal(config()['state'][-1], tk.DISABLED)

        text.insert('insert', 'foo\n'*2)
        text.delete('3.1')
        text.delete('2.0', '2.end')
        equal(get('1.0', 'end'), '1\n2\n')
        equal(config()['state'][-1], tk.DISABLED)

        text.delete('1.3', 'end')
        equal(get('1.0', 'end'), '1\n')
        equal(config()['state'][-1], tk.DISABLED)

        text.delete('1.0', 'end')
        equal(get('1.0', 'end'), '1\n')
        equal(config()['state'][-1], tk.DISABLED)

    def test_sidebar_text_width(self):
        """
        Test that linenumber text widget is always at the minimum
        width
        """
        def get_width():
            return self.linenumber.sidebar_text.config()['width'][-1]
        text = self.text
        equal = self.assertEqual

        equal(get_width(), 1)

        text.insert('insert', 'foo')
        equal(get_width(), 1)

        text.insert('insert', 'foo\n'*10)
        equal(get_width(), 2)

        text.insert('insert', 'foo\n'*100)
        equal(get_width(), 3)

        text.delete('50.0', 'end')
        equal(get_width(), 2)

        text.delete('5.0', 'end')
        equal(get_width(), 1)

        text.delete('1.0', 'end')
        equal(get_width(), 1)

if __name__ == '__main__':
    unittest.main(verbosity=2)
