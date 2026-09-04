""" !Changing this line will break Test_findfile.test_found!
Non-gui unit tests for grep.GrepDialog methods.
default_command calls grep_it calls findfiles.
An exception raised in one method will fail callers.
Otherwise, tests are mostly independent.
Currently test default_command and grep_it, coverage 51%.
"""
from idlelib import grep
import unittest
from unittest import mock
from test.support import captured_stdout
from idlelib.idle_test.mock_tk import Var
import io
import os
import re
import sys


class Dummy_searchengine:
    '''GrepDialog.__init__ calls parent SearchDiabolBase which attaches the
    passed in SearchEngine instance as attribute 'engine'. Only a few of the
    many possible self.engine.x attributes are needed here.
    '''
    def getpat(self):
        return self._pat

    def getprog(self):
        return self._prog

searchengine = Dummy_searchengine()


class Dummy_grep:
    # Methods tested
    default_command = grep.GrepDialog.default_command
    grep_it = grep.GrepDialog.grep_it
    # Other stuff needed
    recvar = Var(False)
    engine = searchengine
    def close(self):  # gui method
        pass

_grep = Dummy_grep()


class DefaultGlobTest(unittest.TestCase):

    def test_no_path(self):
        # gh-80504: an unsaved editor or the Shell has no path, so the
        # pattern uses the current directory, not just '*.py'.
        self.assertEqual(grep.default_glob(''),
                         os.path.join(os.getcwd(), '*.py'))

    def test_full_path(self):
        path = os.path.join(os.path.abspath(os.sep), 'ab', 'foo.py')
        self.assertEqual(grep.default_glob(path),
                         os.path.join(os.path.dirname(path), '*.py'))

    def test_other_extension(self):
        path = os.path.join(os.path.abspath(os.sep), 'ab', 'foo.txt')
        self.assertEqual(grep.default_glob(path),
                         os.path.join(os.path.dirname(path), '*.txt'))


class FindfilesTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.realpath = os.path.realpath(__file__)
        cls.path = os.path.dirname(cls.realpath)

    @classmethod
    def tearDownClass(cls):
        del cls.realpath, cls.path

    def test_invaliddir(self):
        with captured_stdout() as s:
            filelist = list(grep.findfiles('invaliddir', '*.*', False))
        self.assertEqual(filelist, [])
        self.assertIn('invalid', s.getvalue())

    def test_curdir(self):
        # Test os.curdir.
        ff = grep.findfiles
        save_cwd = os.getcwd()
        os.chdir(self.path)
        filename = 'test_grep.py'
        filelist = list(ff(os.curdir, filename, False))
        self.assertIn(os.path.join(os.curdir, filename), filelist)
        os.chdir(save_cwd)

    def test_base(self):
        ff = grep.findfiles
        readme = os.path.join(self.path, 'README.txt')

        # Check for Python files in path where this file lives.
        filelist = list(ff(self.path, '*.py', False))
        # This directory has many Python files.
        self.assertGreater(len(filelist), 10)
        self.assertIn(self.realpath, filelist)
        self.assertNotIn(readme, filelist)

        # Look for .txt files in path where this file lives.
        filelist = list(ff(self.path, '*.txt', False))
        self.assertNotEqual(len(filelist), 0)
        self.assertNotIn(self.realpath, filelist)
        self.assertIn(readme, filelist)

        # Look for non-matching pattern.
        filelist = list(ff(self.path, 'grep.*', False))
        self.assertEqual(len(filelist), 0)
        self.assertNotIn(self.realpath, filelist)

    def test_recurse(self):
        ff = grep.findfiles
        parent = os.path.dirname(self.path)
        grepfile = os.path.join(parent, 'grep.py')
        pat = '*.py'

        # Get Python files only in parent directory.
        filelist = list(ff(parent, pat, False))
        parent_size = len(filelist)
        # Lots of Python files in idlelib.
        self.assertGreater(parent_size, 20)
        self.assertIn(grepfile, filelist)
        # Without subdirectories, this file isn't returned.
        self.assertNotIn(self.realpath, filelist)

        # Include subdirectories.
        filelist = list(ff(parent, pat, True))
        # More files found now.
        self.assertGreater(len(filelist), parent_size)
        self.assertIn(grepfile, filelist)
        # This file exists in list now.
        self.assertIn(self.realpath, filelist)

        # Check another level up the tree.
        parent = os.path.dirname(parent)
        filelist = list(ff(parent, '*.py', True))
        self.assertIn(self.realpath, filelist)


class Grep_itTest(unittest.TestCase):
    # Test captured reports with 0 and some hits.
    # Should test file names, but Windows reports have mixed / and \ separators
    # from incomplete replacement, so 'later'.

    def report(self, pat):
        _grep.engine._pat = pat
        with captured_stdout() as s:
            _grep.grep_it(re.compile(pat), __file__)
        lines = s.getvalue().split('\n')
        lines.pop()  # remove bogus '' after last \n
        return lines

    def test_unfound(self):
        pat = 'xyz*'*7
        lines = self.report(pat)
        self.assertEqual(len(lines), 2)
        self.assertIn(pat, lines[0])
        self.assertEqual(lines[1], 'No hits.')

    def test_found(self):

        pat = '""" !Changing this line will break Test_findfile.test_found!'
        lines = self.report(pat)
        self.assertEqual(len(lines), 5)
        self.assertIn(pat, lines[0])
        self.assertIn('py: 1:', lines[1])  # line number 1
        self.assertIn('2', lines[3])  # hits found 2
        self.assertStartsWith(lines[4], '(Hint:')


class Default_commandTest(unittest.TestCase):
    # default_command searches with grep_it, tested above, writing to an
    # OutputWindow, which is replaced here by a text buffer.

    def setUp(self):
        self.dialog = Dummy_grep()
        self.dialog.top = mock.Mock()  # For top.bell().
        self.dialog.flist = 'flist'
        self.dialog.globvar = Var(value=__file__)
        searchengine._pat = pat = 'xyz*' * 7  # Not in this file.
        searchengine._prog = re.compile(pat)

    def default_command(self):
        "Return the mock OutputWindow class and what was written to it."
        save = sys.stdout
        with mock.patch('idlelib.outwin.OutputWindow') as OutputWindow:
            OutputWindow.return_value = out = io.StringIO()
            self.dialog.default_command()
        self.assertIs(sys.stdout, save)  # Restored even when not replaced.
        return OutputWindow, out.getvalue()

    def test_no_pattern(self):
        # An invalid pattern is reported by getprog, not here.
        searchengine._prog = None
        OutputWindow, output = self.default_command()
        OutputWindow.assert_not_called()
        self.assertEqual(output, '')
        self.dialog.top.bell.assert_not_called()

    def test_no_path(self):
        self.dialog.globvar = Var(value='')
        OutputWindow, output = self.default_command()
        self.dialog.top.bell.assert_called_once()
        OutputWindow.assert_not_called()
        self.assertEqual(output, '')

    def test_search(self):
        OutputWindow, output = self.default_command()
        OutputWindow.assert_called_once_with('flist')  # The flist argument.
        lines = output.split('\n')
        self.assertIn(searchengine._pat, lines[0])
        self.assertEqual(lines[1], 'No hits.')
        self.dialog.top.bell.assert_not_called()


if __name__ == '__main__':
    unittest.main(verbosity=2)
