"Test pyshell, coverage 12%."
# Plus coverage of test_warning.  Was 20% with test_openshell.

from idlelib import pyshell
import os
import unittest
from test.support import requires
from tkinter import Tk


class FunctionTest(unittest.TestCase):
    # Test stand-alone module level non-gui functions.

    def test_restart_line_wide(self):
        eq = self.assertEqual
        for file, mul, extra in (('', 22, ''), ('finame', 21, '=')):
            width = 60
            bar = mul * '='
            with self.subTest(file=file, bar=bar):
                file = file or 'Shell'
                line = pyshell.restart_line(width, file)
                eq(len(line), width)
                eq(line, f"{bar+extra} RESTART: {file} {bar}")

    def test_restart_line_narrow(self):
        expect, taglen = "= RESTART: Shell", 16
        for width in (taglen-1, taglen, taglen+1):
            with self.subTest(width=width):
                self.assertEqual(pyshell.restart_line(width, ''), expect)
        self.assertEqual(pyshell.restart_line(taglen+2, ''), expect+' =')

    def test_fix_user_path(self):
        # gh-134300: the idlelib directory is removed, other entries kept.
        eq = self.assertEqual
        idlelib_dir = os.path.dirname(os.path.abspath(pyshell.__file__))
        eq(pyshell.fix_user_path(['', '/a', idlelib_dir, '/b']), ['', '/a', '/b'])
        eq(pyshell.fix_user_path(['/a', '/b']), ['/a', '/b'])
        eq(pyshell.fix_user_path([idlelib_dir]), [])


class PyShellFileListTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        #cls.root.update_idletasks()
##        for id in cls.root.after_info():
##            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        psfl = pyshell.PyShellFileList(self.root)
        self.assertEqual(psfl.EditorWindow, pyshell.PyShellEditorWindow)
        self.assertIsNone(psfl.pyshell)

# The following sometimes causes 'invalid command name "109734456recolorize"'.
# Uncommenting after_cancel above prevents this, but results in
# TclError: bad window path name ".!listedtoplevel.!frame.text"
# which is normally prevented by after_cancel.
##    def test_openshell(self):
##        pyshell.use_subprocess = False
##        ps = pyshell.PyShellFileList(self.root).open_shell()
##        self.assertIsInstance(ps, pyshell.PyShell)


class PyShellTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.shell = pyshell.PyShell(pyshell.PyShellFileList(cls.root))

    @classmethod
    def tearDownClass(cls):
        cls.shell.close()
        del cls.shell
        cls.root.destroy()
        del cls.root

    def setUp(self):
        text = self.shell.text
        self.shell.per.bottom.delete('1.0', 'end')
        text.mark_set('iomark', '1.0')  # As after begin().
        text.mark_gravity('iomark', 'left')
        self.shell.undo.reset_undo()

    def test_output_at_prompt(self):
        # gh-75512: output from a thread while a prompt is shown goes
        # before the prompt, which stays with the input in progress.
        shell = self.shell
        text = shell.text
        shell.write('x\n', 'stdout')
        shell.resetoutput()
        text.tag_add('console', 'iomark-1c')  # As showprompt() does.
        text.insert('end-1c', 'a = (')
        shell.write('hel', 'stdout')
        shell.write('lo\n', 'stdout')
        self.assertEqual(text.get('iomark-6c', 'end-1c'), 'hello\na = (')
        self.assertIn('console', text.tag_names('iomark-1c'))
        self.assertNotIn('console', text.tag_names('iomark-7c'))
        self.assertEqual(shell.shell_sidebar.line_prompts, {3: '>>>'})


class InputStatementlTest(unittest.TestCase):
    # Test handling of response to input statements in user code.

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.shell = pyshell.PyShell(pyshell.PyShellFileList(cls.root))
        # As after begin().
        cls.shell.text.mark_set('iomark', 'insert')
        cls.shell.text.mark_gravity('iomark', 'left')

    @classmethod
    def tearDownClass(cls):
        cls.shell.close()
        del cls.shell
        cls.root.destroy()
        del cls.root

    def test_input_not_colorized(self):
        # gh-64007: input for input() is not colorized, unlike code.
        shell = self.shell
        text = shell.text
        color = shell.color
        shell.resetoutput()
        color.reading = True
        text.insert('end-1c', 'for x in y')
        color.recolorize_main()
        self.assertEqual(text.tag_ranges('KEYWORD'), ())
        color.reading = False
        color.notify_range('iomark', 'end')
        color.recolorize_main()
        self.assertEqual(len(text.tag_ranges('KEYWORD')), 4)


class PyShellRemoveLastNewlineAndSurroundingWhitespaceTest(unittest.TestCase):
    regexp = pyshell.PyShell._last_newline_re

    def all_removed(self, text):
        self.assertEqual('', self.regexp.sub('', text))

    def none_removed(self, text):
        self.assertEqual(text, self.regexp.sub('', text))

    def check_result(self, text, expected):
        self.assertEqual(expected, self.regexp.sub('', text))

    def test_empty(self):
        self.all_removed('')

    def test_newline(self):
        self.all_removed('\n')

    def test_whitespace_no_newline(self):
        self.all_removed(' ')
        self.all_removed('  ')
        self.all_removed('   ')
        self.all_removed(' ' * 20)
        self.all_removed('\t')
        self.all_removed('\t\t')
        self.all_removed('\t\t\t')
        self.all_removed('\t' * 20)
        self.all_removed('\t ')
        self.all_removed(' \t')
        self.all_removed(' \t \t ')
        self.all_removed('\t \t \t')

    def test_newline_with_whitespace(self):
        self.all_removed(' \n')
        self.all_removed('\t\n')
        self.all_removed(' \t\n')
        self.all_removed('\t \n')
        self.all_removed('\n ')
        self.all_removed('\n\t')
        self.all_removed('\n \t')
        self.all_removed('\n\t ')
        self.all_removed(' \n ')
        self.all_removed('\t\n ')
        self.all_removed(' \n\t')
        self.all_removed('\t\n\t')
        self.all_removed('\t \t \t\n')
        self.all_removed(' \t \t \n')
        self.all_removed('\n\t \t \t')
        self.all_removed('\n \t \t ')

    def test_multiple_newlines(self):
        self.check_result('\n\n', '\n')
        self.check_result('\n' * 5, '\n' * 4)
        self.check_result('\n' * 5 + '\t', '\n' * 4)
        self.check_result('\n' * 20, '\n' * 19)
        self.check_result('\n' * 20 + ' ', '\n' * 19)
        self.check_result(' \n \n ', ' \n')
        self.check_result(' \n\n ', ' \n')
        self.check_result(' \n\n', ' \n')
        self.check_result('\t\n\n', '\t\n')
        self.check_result('\n\n ', '\n')
        self.check_result('\n\n\t', '\n')
        self.check_result(' \n \n ', ' \n')
        self.check_result('\t\n\t\n\t', '\t\n')

    def test_non_whitespace(self):
        self.none_removed('a')
        self.check_result('a\n', 'a')
        self.check_result('a\n ', 'a')
        self.check_result('a \n ', 'a')
        self.check_result('a \n\t', 'a')
        self.none_removed('-')
        self.check_result('-\n', '-')
        self.none_removed('.')
        self.check_result('.\n', '.')

    def test_unsupported_whitespace(self):
        self.none_removed('\v')
        self.none_removed('\n\v')
        self.check_result('\v\n', '\v')
        self.none_removed(' \n\v')
        self.check_result('\v\n ', '\v')


if __name__ == '__main__':
    unittest.main(verbosity=2)
