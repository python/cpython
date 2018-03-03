""" Test idlelib.editor.
"""

from collections import namedtuple
import unittest
import tkinter as tk
from idlelib import editor
from test.support import requires

root = None


def setUpModule():
    global root
    requires('gui')
    root = tk.Tk()
    root.withdraw()


def tearDownModule():
    global root
    # Cleanup all scheduled tasks, not just idle tasks.
    tasks = root.tk.call('after', 'info')
    for task in tasks:
        root.after_cancel(task)
    root.destroy()
    del root


class Editor_func_test(unittest.TestCase):
    def test_filename_to_unicode(self):
        func = editor.EditorWindow._filename_to_unicode
        class dummy(): filesystemencoding = 'utf-8'
        pairs = (('abc', 'abc'), ('a\U00011111c', 'a\ufffdc'),
                 (b'abc', 'abc'), (b'a\xf0\x91\x84\x91c', 'a\ufffdc'))
        for inp, out in pairs:
            self.assertEqual(func(dummy, inp), out)


class IndentAndNewlineTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.window = editor.EditorWindow(root=root)
        cls.window.indentwidth = 2
        cls.window.tabwidth = 2

    @classmethod
    def tearDownClass(cls):
        cls.window._close()
        del cls.window

    def insert(self, text):
        t = self.window.text
        t.delete('1.0', 'end')
        t.insert('end', text)
        # Force update for colorizer to finish.
        t.update()

    def test_indent_and_newline_event(self):
        eq = self.assertEqual
        w = self.window
        text = w.text
        get = text.get
        nl = w.newline_and_indent_event

        TestInfo = namedtuple('Tests', ['label', 'text', 'expected', 'mark'])

        tests = (TestInfo('Empty line inserts with no indent.',
                          '  \n  def __init__(self):',
                          '\n  \n  def __init__(self):\n',
                          '1.end'),
                 TestInfo('Inside bracket before space, deletes space.',
                          '  def f1(self, a, b):',
                          '  def f1(self,\n         a, b):\n',
                          '1.14'),
                 TestInfo('Inside bracket after space, deletes space.',
                          '  def f1(self, a, b):',
                          '  def f1(self,\n         a, b):\n',
                          '1.15'),
                 TestInfo('Inside string with one line - no indent.',
                          '  """Docstring."""',
                          '  """Docstring.\n"""\n',
                          '1.15'),
                 TestInfo('Inside string with more than one line.',
                          '  """Docstring.\n  Docstring Line 2"""',
                          '  """Docstring.\n  Docstring Line 2\n  """\n',
                          '2.18'),
                 TestInfo('Backslash with one line.',
                          'a =\\',
                          'a =\\\n  \n',
                          '1.end'),
                 TestInfo('Backslash with more than one line.',
                          'a =\\\n          multiline\\',
                          'a =\\\n          multiline\\\n          \n',
                          '2.end'),
                 TestInfo('Block opener - indents +1 level.',
                          '  def f1(self):\n    pass',
                          '  def f1(self):\n    \n    pass\n',
                          '1.end'),
                 TestInfo('Block closer - dedents -1 level.',
                          '  def f1(self):\n    pass',
                          '  def f1(self):\n    pass\n  \n',
                          '2.end'),
                 )

        w.prompt_last_line = ''
        for context in (True, False):
            w.context_use_ps1 = context
            for test in tests:
                with self.subTest(label=test.label):
                    self.insert(test.text)
                    text.mark_set('insert', test.mark)
                    nl(None)
                    eq(get('1.0', 'end'), test.expected)

        # Selected text.
        self.insert('  def f1(self, a, b):\n    return a + b')
        text.tag_add('sel', '1.17', '1.end')
        nl(None)
        # Deletes selected text before adding new line.
        eq(get('1.0', 'end'), '  def f1(self, a,\n         \n    return a + b\n')

        # Prompt preserves the whitespace in the prompt.
        w.prompt_last_line = '>>> '
        self.insert('>>> \t\ta =')
        text.mark_set('insert', '1.5')
        nl(None)
        eq(get('1.0', 'end'), '>>> \na =\n')
        w.prompt_last_line = ''


if __name__ == '__main__':
    unittest.main(verbosity=2)
