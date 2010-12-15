import difflib
from test.support import run_unittest, findfile
import unittest
import doctest
import sys


class TestWithAscii(unittest.TestCase):
    def test_one_insert(self):
        sm = difflib.SequenceMatcher(None, 'b' * 100, 'a' + 'b' * 100)
        self.assertAlmostEqual(sm.ratio(), 0.995, places=3)
        self.assertEqual(list(sm.get_opcodes()),
            [   ('insert', 0, 0, 0, 1),
                ('equal', 0, 100, 1, 101)])
        self.assertEqual(sm.bpopular, set())
        sm = difflib.SequenceMatcher(None, 'b' * 100, 'b' * 50 + 'a' + 'b' * 50)
        self.assertAlmostEqual(sm.ratio(), 0.995, places=3)
        self.assertEqual(list(sm.get_opcodes()),
            [   ('equal', 0, 50, 0, 50),
                ('insert', 50, 50, 50, 51),
                ('equal', 50, 100, 51, 101)])
        self.assertEqual(sm.bpopular, set())

    def test_one_delete(self):
        sm = difflib.SequenceMatcher(None, 'a' * 40 + 'c' + 'b' * 40, 'a' * 40 + 'b' * 40)
        self.assertAlmostEqual(sm.ratio(), 0.994, places=3)
        self.assertEqual(list(sm.get_opcodes()),
            [   ('equal', 0, 40, 0, 40),
                ('delete', 40, 41, 40, 40),
                ('equal', 41, 81, 40, 80)])

    def test_bjunk(self):
        sm = difflib.SequenceMatcher(isjunk=lambda x: x == ' ',
                a='a' * 40 + 'b' * 40, b='a' * 44 + 'b' * 40)
        self.assertEqual(sm.bjunk, set())

        sm = difflib.SequenceMatcher(isjunk=lambda x: x == ' ',
                a='a' * 40 + 'b' * 40, b='a' * 44 + 'b' * 40 + ' ' * 20)
        self.assertEqual(sm.bjunk, {' '})

        sm = difflib.SequenceMatcher(isjunk=lambda x: x in [' ', 'b'],
                a='a' * 40 + 'b' * 40, b='a' * 44 + 'b' * 40 + ' ' * 20)
        self.assertEqual(sm.bjunk, {' ', 'b'})


class TestAutojunk(unittest.TestCase):
    """Tests for the autojunk parameter added in 2.7"""
    def test_one_insert_homogenous_sequence(self):
        # By default autojunk=True and the heuristic kicks in for a sequence
        # of length 200+
        seq1 = 'b' * 200
        seq2 = 'a' + 'b' * 200

        sm = difflib.SequenceMatcher(None, seq1, seq2)
        self.assertAlmostEqual(sm.ratio(), 0, places=3)
        self.assertEqual(sm.bpopular, {'b'})

        # Now turn the heuristic off
        sm = difflib.SequenceMatcher(None, seq1, seq2, autojunk=False)
        self.assertAlmostEqual(sm.ratio(), 0.9975, places=3)
        self.assertEqual(sm.bpopular, set())


class TestSFbugs(unittest.TestCase):
    def test_ratio_for_null_seqn(self):
        # Check clearing of SF bug 763023
        s = difflib.SequenceMatcher(None, [], [])
        self.assertEqual(s.ratio(), 1)
        self.assertEqual(s.quick_ratio(), 1)
        self.assertEqual(s.real_quick_ratio(), 1)

    def test_comparing_empty_lists(self):
        # Check fix for bug #979794
        group_gen = difflib.SequenceMatcher(None, [], []).get_grouped_opcodes()
        self.assertRaises(StopIteration, next, group_gen)
        diff_gen = difflib.unified_diff([], [])
        self.assertRaises(StopIteration, next, diff_gen)

    def test_added_tab_hint(self):
        # Check fix for bug #1488943
        diff = list(difflib.Differ().compare(["\tI am a buggy"],["\t\tI am a bug"]))
        self.assertEqual("- \tI am a buggy", diff[0])
        self.assertEqual("?            --\n", diff[1])
        self.assertEqual("+ \t\tI am a bug", diff[2])
        self.assertEqual("? +\n", diff[3])

patch914575_from1 = """
   1. Beautiful is beTTer than ugly.
   2. Explicit is better than implicit.
   3. Simple is better than complex.
   4. Complex is better than complicated.
"""

patch914575_to1 = """
   1. Beautiful is better than ugly.
   3.   Simple is better than complex.
   4. Complicated is better than complex.
   5. Flat is better than nested.
"""

patch914575_from2 = """
\t\tLine 1: preceeded by from:[tt] to:[ssss]
  \t\tLine 2: preceeded by from:[sstt] to:[sssst]
  \t \tLine 3: preceeded by from:[sstst] to:[ssssss]
Line 4:  \thas from:[sst] to:[sss] after :
Line 5: has from:[t] to:[ss] at end\t
"""

patch914575_to2 = """
    Line 1: preceeded by from:[tt] to:[ssss]
    \tLine 2: preceeded by from:[sstt] to:[sssst]
      Line 3: preceeded by from:[sstst] to:[ssssss]
Line 4:   has from:[sst] to:[sss] after :
Line 5: has from:[t] to:[ss] at end
"""

patch914575_from3 = """line 0
1234567890123456789012345689012345
line 1
line 2
line 3
line 4   changed
line 5   changed
line 6   changed
line 7
line 8  subtracted
line 9
1234567890123456789012345689012345
short line
just fits in!!
just fits in two lines yup!!
the end"""

patch914575_to3 = """line 0
1234567890123456789012345689012345
line 1
line 2    added
line 3
line 4   chanGEd
line 5a  chanGed
line 6a  changEd
line 7
line 8
line 9
1234567890
another long line that needs to be wrapped
just fitS in!!
just fits in two lineS yup!!
the end"""

class TestSFpatches(unittest.TestCase):

    def test_html_diff(self):
        # Check SF patch 914575 for generating HTML differences
        f1a = ((patch914575_from1 + '123\n'*10)*3)
        t1a = (patch914575_to1 + '123\n'*10)*3
        f1b = '456\n'*10 + f1a
        t1b = '456\n'*10 + t1a
        f1a = f1a.splitlines()
        t1a = t1a.splitlines()
        f1b = f1b.splitlines()
        t1b = t1b.splitlines()
        f2 = patch914575_from2.splitlines()
        t2 = patch914575_to2.splitlines()
        f3 = patch914575_from3
        t3 = patch914575_to3
        i = difflib.HtmlDiff()
        j = difflib.HtmlDiff(tabsize=2)
        k = difflib.HtmlDiff(wrapcolumn=14)

        full = i.make_file(f1a,t1a,'from','to',context=False,numlines=5)
        tables = '\n'.join(
            [
             '<h2>Context (first diff within numlines=5(default))</h2>',
             i.make_table(f1a,t1a,'from','to',context=True),
             '<h2>Context (first diff after numlines=5(default))</h2>',
             i.make_table(f1b,t1b,'from','to',context=True),
             '<h2>Context (numlines=6)</h2>',
             i.make_table(f1a,t1a,'from','to',context=True,numlines=6),
             '<h2>Context (numlines=0)</h2>',
             i.make_table(f1a,t1a,'from','to',context=True,numlines=0),
             '<h2>Same Context</h2>',
             i.make_table(f1a,f1a,'from','to',context=True),
             '<h2>Same Full</h2>',
             i.make_table(f1a,f1a,'from','to',context=False),
             '<h2>Empty Context</h2>',
             i.make_table([],[],'from','to',context=True),
             '<h2>Empty Full</h2>',
             i.make_table([],[],'from','to',context=False),
             '<h2>tabsize=2</h2>',
             j.make_table(f2,t2),
             '<h2>tabsize=default</h2>',
             i.make_table(f2,t2),
             '<h2>Context (wrapcolumn=14,numlines=0)</h2>',
             k.make_table(f3.splitlines(),t3.splitlines(),context=True,numlines=0),
             '<h2>wrapcolumn=14,splitlines()</h2>',
             k.make_table(f3.splitlines(),t3.splitlines()),
             '<h2>wrapcolumn=14,splitlines(True)</h2>',
             k.make_table(f3.splitlines(True),t3.splitlines(True)),
             ])
        actual = full.replace('</body>','\n%s\n</body>' % tables)

        # temporarily uncomment next two lines to baseline this test
        #with open('test_difflib_expect.html','w') as fp:
        #    fp.write(actual)

        with open(findfile('test_difflib_expect.html')) as fp:
            self.assertEqual(actual, fp.read())

    def test_recursion_limit(self):
        # Check if the problem described in patch #1413711 exists.
        limit = sys.getrecursionlimit()
        old = [(i%2 and "K:%d" or "V:A:%d") % i for i in range(limit*2)]
        new = [(i%2 and "K:%d" or "V:B:%d") % i for i in range(limit*2)]
        difflib.SequenceMatcher(None, old, new).get_opcodes()


class TestOutputFormat(unittest.TestCase):
    def test_tab_delimiter(self):
        args = ['one', 'two', 'Original', 'Current',
            '2005-01-26 23:30:50', '2010-04-02 10:20:52']
        ud = difflib.unified_diff(*args, lineterm='')
        self.assertEqual(list(ud)[0:2], [
                           "--- Original\t2005-01-26 23:30:50",
                           "+++ Current\t2010-04-02 10:20:52"])
        cd = difflib.context_diff(*args, lineterm='')
        self.assertEqual(list(cd)[0:2], [
                           "*** Original\t2005-01-26 23:30:50",
                           "--- Current\t2010-04-02 10:20:52"])

    def test_no_trailing_tab_on_empty_filedate(self):
        args = ['one', 'two', 'Original', 'Current']
        ud = difflib.unified_diff(*args, lineterm='')
        self.assertEqual(list(ud)[0:2], ["--- Original", "+++ Current"])

        cd = difflib.context_diff(*args, lineterm='')
        self.assertEqual(list(cd)[0:2], ["*** Original", "--- Current"])


def test_main():
    difflib.HtmlDiff._default_prefix = 0
    Doctests = doctest.DocTestSuite(difflib)
    run_unittest(
        TestWithAscii, TestAutojunk, TestSFpatches, TestSFbugs,
        TestOutputFormat, Doctests)

if __name__ == '__main__':
    test_main()
