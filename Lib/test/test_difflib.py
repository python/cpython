import difflib
from test.test_support import run_unittest, findfile
import unittest
import doctest

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
        self.assertRaises(StopIteration, group_gen.next)
        diff_gen = difflib.unified_diff([], [])
        self.assertRaises(StopIteration, diff_gen.next)

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
        # temporarily uncomment next three lines to baseline this test
        #f = open('test_difflib_expect.html','w')
        #f.write(actual)
        #f.close()
        expect = open(findfile('test_difflib_expect.html')).read()


        self.assertEqual(actual,expect)

Doctests = doctest.DocTestSuite(difflib)

run_unittest(TestSFpatches, TestSFbugs, Doctests)
