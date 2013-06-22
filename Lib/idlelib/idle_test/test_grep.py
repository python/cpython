""" !Changing this line will break Test_findfile.test_found!
Non-gui unit tests for idlelib.GrepDialog methods.
dummy_command calls grep_it calls findfiles.
An exception raised in one method will fail callers.
Otherwise, tests are mostly independent.
*** Currently only test grep_it.
"""
import unittest
from test.support import captured_stdout
from idlelib.idle_test.mock_tk import Var
from idlelib.GrepDialog import GrepDialog
import re

class Dummy_searchengine:
    '''GrepDialog.__init__ calls parent SearchDiabolBase which attaches the
    passed in SearchEngine instance as attribute 'engine'. Only a few of the
    many possible self.engine.x attributes are needed here.
    '''
    def getpat(self):
        return self._pat

searchengine = Dummy_searchengine()

class Dummy_grep:
    # Methods tested
    #default_command = GrepDialog.default_command
    grep_it = GrepDialog.grep_it
    findfiles = GrepDialog.findfiles
    # Other stuff needed
    recvar = Var(False)
    engine = searchengine
    def close(self):  # gui method
        pass

grep = Dummy_grep()

class FindfilesTest(unittest.TestCase):
    # findfiles is really a function, not a method, could be iterator
    # test that filename return filename
    # test that idlelib has many .py files
    # test that recursive flag adds idle_test .py files
    pass

class Grep_itTest(unittest.TestCase):
    # Test captured reports with 0 and some hits.
    # Should test file names, but Windows reports have mixed / and \ separators
    # from incomplete replacement, so 'later'.

    def report(self, pat):
        grep.engine._pat = pat
        with captured_stdout() as s:
            grep.grep_it(re.compile(pat), __file__)
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
        self.assertTrue(lines[4].startswith('(Hint:'))

class Default_commandTest(unittest.TestCase):
    # To write this, mode OutputWindow import to top of GrepDialog
    # so it can be replaced by captured_stdout in class setup/teardown.
    pass

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
