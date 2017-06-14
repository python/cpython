""" Test idlelib.outwin.
"""

import unittest
import unittest.mock as mock
from tkinter import Tk, Text
from idlelib.idle_test.mock_tk import Mbox_func

from idlelib import filelist
from idlelib import outwin
from test.support import requires


class OutputWindowTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.window = outwin.OutputWindow(None, None, None, cls.root)
        cls.text = Text(cls.root)
        cls.window.text = cls.text

    @classmethod
    def tearDownClass(cls):
        del cls.text, cls.window
        cls.root.destroy()
        del cls.root

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def test_window_title(self):
        self.assertEqual(self.window.top.title(), 'Output')

    def test_ispythonsource(self):
        "OutputWindow overrides ispythonsource to always return False."
        self.assertFalse(self.window.ispythonsource('test.txt'))
        self.assertFalse(self.window.ispythonsource(__file__))

    def test_maybesave(self):
        self.window.get_saved = mock.Mock(return_value=False)
        self.assertEqual(self.window.maybesave(), 'no')
        self.window.get_saved.assert_called_once()
        del self.window.get_saved

    def test_write(self):
        text = self.text
        write = self.window.write
        test_text = 'test text\nLine 2'
        self.assertEqual(write(test_text), len(test_text))
        self.assertEqual(text.get('1.0', '1.end'), 'test text')
        self.assertEqual(text.get('insert linestart', 'insert lineend'),
                         'Line 2')

    def test_writelines(self):
        text = self.text
        writelines = self.window.writelines
        writelines(['Line 1\n', 'Line 2\n', 'Line 3\n'])
        self.assertEqual(text.get('1.0', '1.end'), 'Line 1')
        self.assertEqual(text.get('3.0', '3.end'), 'Line 3')


class GotoFileLineTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.orig_error = outwin.messagebox.showerror
        outwin.messagebox.showerror = Mbox_func()
        cls.flist = filelist.FileList(cls.root)
        cls.window = outwin.OutputWindow(cls.flist, None, None, None)
        cls.text = Text(cls.root)
        cls.window.text = cls.text

    @classmethod
    def tearDownClass(cls):
        outwin.messagebox.showerror = cls.orig_error
        del cls.text, cls.window, cls.flist, cls.orig_error
        cls.root.destroy()
        del cls.root

    def setUp(self):
        self.window.flist.gotofileline = mock.Mock()

    def tearDown(self):
        self.window.text.delete('1.0', 'end')
        del self.window.flist.gotofileline

    def test_curr_line(self):
        text = self.text
        goto_file_line = self.window.goto_file_line

        text.insert('insert', '{}: 42: spam\n'.format(str(__file__)))
        text.insert('insert', '{}: 21: spam'.format(str(__file__)))
        self.assertIsNone(goto_file_line())
        self.window.flist.gotofileline.assert_called_with(str(__file__), 21)

    def test_no_line(self):
        text = self.text
        goto_file_line = self.window.goto_file_line

        text.insert('insert', 'Not a file line')
        self.assertIsNone(goto_file_line())
        self.window.flist.gotofileline.assert_not_called()
        self.assertEqual(outwin.messagebox.showerror.title, 'No special line')

    def test_prev_line(self):
        text = self.text
        goto_file_line = self.window.goto_file_line

        text.insert('insert', '{}: 42: spam\n'.format(str(__file__)))
        text.insert('insert', 'Not a file line')
        self.assertIsNone(goto_file_line())
        self.window.flist.gotofileline.assert_called_with(str(__file__), 42)


class ModuleFunctionTest(unittest.TestCase):

    def test_compile_progs(self):
        outwin.compile_progs()
        for pat, regex in zip(outwin.file_line_pats, outwin.file_line_progs):
            self.assertEqual(regex.pattern, pat)

    @mock.patch('builtins.open')
    def test_file_line_helper(self, mock_open):
        file_line_helper = outwin.file_line_helper
        test_lines = (
            (r'foo file "testfile1", line 42, bar', ('testfile1', 42)),
            (r'foo testfile2(21) bar', ('testfile2', 21)),
            (r'  testfile3  : 42: foo bar\n', ('  testfile3  ', 42)),
            (r'foo testfile4.py :1: ', ('foo testfile4.py ', 1)),
            ('testfile5: \u19D4\u19D2: ', ('testfile5', 42)),
            (r'testfile6: 42', None),       # only one :
            (r'testfile7 42 text', None)    # no separators
            )
        for line, expected_output in test_lines:
            self.assertEqual(file_line_helper(line), expected_output)
            if expected_output:
                mock_open.assert_called_with(expected_output[0], 'r')


if __name__ == '__main__':
    unittest.main(verbosity=2)
