"""Unittest for idlelib.LineNumber"""
from itertools import chain
import unittest
from test.support import requires
import tkinter as tk

from idlelib.delegator import Delegator
from idlelib.percolator import Percolator
from idlelib.sidebar import LineNumbers


class Dummy_editwin:
    def __init__(self, text):
        self.text = text
        self.text_frame = self.text.master
        self.per = Percolator(text)
        self.undo = Delegator()
        self.per.insertfilter(self.undo)

    def setvar(self, name, value):
        pass

    def getlineno(self, index):
        return int(float(self.text.index(index)))


class LineNumbersTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()

        cls.text_frame = tk.Frame(cls.root)
        cls.text_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        cls.text_frame.rowconfigure(1, weight=1)
        cls.text_frame.columnconfigure(1, weight=1)

        cls.text = tk.Text(cls.text_frame, width=80, height=24, wrap=tk.NONE)
        cls.text.grid(row=1, column=1, sticky=tk.NSEW)

        cls.editwin = Dummy_editwin(cls.text)
        cls.editwin.vbar = tk.Scrollbar(cls.text_frame)

    @classmethod
    def tearDownClass(cls):
        cls.editwin.per.close()
        cls.root.update()
        cls.root.destroy()
        del cls.text, cls.text_frame, cls.editwin, cls.root

    def setUp(self):
        self.linenumber = LineNumbers(self.editwin)

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def get_selection(self):
        return tuple(map(str, self.text.tag_ranges('sel')))

    def get_line_screen_position(self, line):
        bbox = self.linenumber.sidebar_text.bbox(f'{line}.end -1c')
        x = bbox[0] + bbox[2] // 2
        y = bbox[1] + bbox[3] // 2
        return x, y

    def assert_state_disabled(self):
        state = self.linenumber.sidebar_text.config()['state']
        self.assertEqual(state[-1], tk.DISABLED)

    def get_sidebar_text_contents(self):
        return self.linenumber.sidebar_text.get('1.0', tk.END)

    def assert_sidebar_n_lines(self, n_lines):
        expected = '\n'.join(chain(map(str, range(1, n_lines + 1)), ['']))
        self.assertEqual(self.get_sidebar_text_contents(), expected)

    def assert_text_equals(self, expected):
        return self.assertEqual(self.text.get('1.0', 'end'), expected)

    def test_init_empty(self):
        self.assert_sidebar_n_lines(1)

    def test_init_not_empty(self):
        self.text.insert('insert', 'foo bar\n'*3)
        self.assert_text_equals('foo bar\n'*3 + '\n')
        self.assert_sidebar_n_lines(4)

    def test_toggle_linenumbering(self):
        self.assertEqual(self.linenumber.is_shown, False)
        self.linenumber.show_sidebar()
        self.assertEqual(self.linenumber.is_shown, True)
        self.linenumber.hide_sidebar()
        self.assertEqual(self.linenumber.is_shown, False)
        self.linenumber.hide_sidebar()
        self.assertEqual(self.linenumber.is_shown, False)
        self.linenumber.show_sidebar()
        self.assertEqual(self.linenumber.is_shown, True)
        self.linenumber.show_sidebar()
        self.assertEqual(self.linenumber.is_shown, True)

    def test_insert(self):
        self.text.insert('insert', 'foobar')
        self.assert_text_equals('foobar\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

        self.text.insert('insert', '\nfoo')
        self.assert_text_equals('foobar\nfoo\n')
        self.assert_sidebar_n_lines(2)
        self.assert_state_disabled()

        self.text.insert('insert', 'hello\n'*2)
        self.assert_text_equals('foobar\nfoohello\nhello\n\n')
        self.assert_sidebar_n_lines(4)
        self.assert_state_disabled()

        self.text.insert('insert', '\nworld')
        self.assert_text_equals('foobar\nfoohello\nhello\n\nworld\n')
        self.assert_sidebar_n_lines(5)
        self.assert_state_disabled()

    def test_delete(self):
        self.text.insert('insert', 'foobar')
        self.assert_text_equals('foobar\n')
        self.text.delete('1.1', '1.3')
        self.assert_text_equals('fbar\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

        self.text.insert('insert', 'foo\n'*2)
        self.assert_text_equals('fbarfoo\nfoo\n\n')
        self.assert_sidebar_n_lines(3)
        self.assert_state_disabled()

        # Note: deleting up to "2.end" doesn't delete the final newline.
        self.text.delete('2.0', '2.end')
        self.assert_text_equals('fbarfoo\n\n\n')
        self.assert_sidebar_n_lines(3)
        self.assert_state_disabled()

        self.text.delete('1.3', 'end')
        self.assert_text_equals('fba\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

        # Note: Text widgets always keep a single '\n' character at the end.
        self.text.delete('1.0', 'end')
        self.assert_text_equals('\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

    def test_sidebar_text_width(self):
        """
        Test that linenumber text widget is always at the minimum
        width
        """
        def get_width():
            return self.linenumber.sidebar_text.config()['width'][-1]

        self.assert_sidebar_n_lines(1)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo')
        self.assert_sidebar_n_lines(1)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo\n'*8)
        self.assert_sidebar_n_lines(9)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(10)
        self.assertEqual(get_width(), 2)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(11)
        self.assertEqual(get_width(), 2)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(10)
        self.assertEqual(get_width(), 2)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(9)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo\n'*90)
        self.assert_sidebar_n_lines(99)
        self.assertEqual(get_width(), 2)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(100)
        self.assertEqual(get_width(), 3)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(101)
        self.assertEqual(get_width(), 3)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(100)
        self.assertEqual(get_width(), 3)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(99)
        self.assertEqual(get_width(), 2)

        self.text.delete('50.0 -1c', 'end -1c')
        self.assert_sidebar_n_lines(49)
        self.assertEqual(get_width(), 2)

        self.text.delete('5.0 -1c', 'end -1c')
        self.assert_sidebar_n_lines(4)
        self.assertEqual(get_width(), 1)

        # Note: Text widgets always keep a single '\n' character at the end.
        self.text.delete('1.0', 'end -1c')
        self.assert_sidebar_n_lines(1)
        self.assertEqual(get_width(), 1)

    def test_click_selection(self):
        self.linenumber.show_sidebar()
        self.text.insert('1.0', 'one\ntwo\nthree\nfour\n')
        self.root.update()

        # Click on the second line.
        x, y = self.get_line_screen_position(2)
        self.linenumber.sidebar_text.event_generate('<Button-1>', x=x, y=y)
        self.linenumber.sidebar_text.update()
        self.root.update()

        self.assertEqual(self.get_selection(), ('2.0', '3.0'))

    def test_drag_selection(self):
        self.linenumber.show_sidebar()
        self.text.insert('1.0', 'one\ntwo\nthree\nfour\n')
        self.root.update()

        # Drag from the first line to the third line.
        start_x, start_y = self.get_line_screen_position(1)
        end_x, end_y = self.get_line_screen_position(3)
        self.linenumber.sidebar_text.event_generate('<Button-1>',
                                                    x=start_x, y=start_y)
        self.linenumber.sidebar_text.event_generate('<B1-Motion>',
                                                    x=start_x, y=start_y)
        self.linenumber.sidebar_text.event_generate('<B1-Motion>',
                                                    x=end_x, y=end_y)
        self.linenumber.sidebar_text.event_generate('<ButtonRelease-1>',
                                                    x=end_x, y=end_y)
        self.root.update()

        self.assertEqual(self.get_selection(), ('1.0', '4.0'))

    def test_scroll(self):
        self.linenumber.show_sidebar()
        self.text.insert('1.0', 'line\n' * 100)
        self.root.update()

        # Scroll down 10 lines.
        self.text.yview_scroll(10, 'unit')
        self.root.update()
        self.assertEqual(self.text.index('@0,0'), '11.0')
        self.assertEqual(self.linenumber.sidebar_text.index('@0,0'), '11.0')

        # Generate a mouse-wheel event and make sure it scrolled up or down.
        # The meaning of the "delta" is OS-dependant, so this just checks for
        # any change.
        self.linenumber.sidebar_text.event_generate('<MouseWheel>',
                                                    x=0, y=0,
                                                    delta=10)
        self.root.update()
        self.assertNotEqual(self.text.index('@0,0'), '11.0')
        self.assertNotEqual(self.linenumber.sidebar_text.index('@0,0'), '11.0')


if __name__ == '__main__':
    unittest.main(verbosity=2)
