import difflib
from test.support import findfile, force_colorized
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

    def test_matching_blocks_cache(self):
        # Issue #21635
        s = difflib.SequenceMatcher(None, "abxcd", "abcd")
        first = s.get_matching_blocks()
        second = s.get_matching_blocks()
        self.assertEqual(second[0].size, 2)
        self.assertEqual(second[1].size, 2)
        self.assertEqual(second[2].size, 0)

    def test_added_tab_hint(self):
        # Check fix for bug #1488943
        diff = list(difflib.Differ().compare(["\tI am a buggy"],["\t\tI am a bug"]))
        self.assertEqual("- \tI am a buggy", diff[0])
        self.assertEqual("? \t          --\n", diff[1])
        self.assertEqual("+ \t\tI am a bug", diff[2])
        self.assertEqual("? +\n", diff[3])

    def test_hint_indented_properly_with_tabs(self):
        diff = list(difflib.Differ().compare(["\t \t \t^"], ["\t \t \t^\n"]))
        self.assertEqual("- \t \t \t^", diff[0])
        self.assertEqual("+ \t \t \t^\n", diff[1])
        self.assertEqual("? \t \t \t +\n", diff[2])

    def test_mdiff_catch_stop_iteration(self):
        # Issue #33224
        self.assertEqual(
            list(difflib._mdiff(["2"], ["3"], 1)),
            [((1, '\x00-2\x01'), (1, '\x00+3\x01'), True)],
        )


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

patch914575_nonascii_from1 = """
   1. Beautiful is beTTer than ugly.
   2. Explicit is better than ımplıcıt.
   3. Simple is better than complex.
   4. Complex is better than complicated.
"""

patch914575_nonascii_to1 = """
   1. Beautiful is better than ügly.
   3.   Sımple is better than complex.
   4. Complicated is better than cömplex.
   5. Flat is better than nested.
"""

patch914575_from2 = """
\t\tLine 1: preceded by from:[tt] to:[ssss]
  \t\tLine 2: preceded by from:[sstt] to:[sssst]
  \t \tLine 3: preceded by from:[sstst] to:[ssssss]
Line 4:  \thas from:[sst] to:[sss] after :
Line 5: has from:[t] to:[ss] at end\t
"""

patch914575_to2 = """
    Line 1: preceded by from:[tt] to:[ssss]
    \tLine 2: preceded by from:[sstt] to:[sssst]
      Line 3: preceded by from:[sstst] to:[ssssss]
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

        with open(findfile('test_difflib_expect.html'), encoding="utf-8") as fp:
            self.assertEqual(actual, fp.read())

    def test_recursion_limit(self):
        # Check if the problem described in patch #1413711 exists.
        limit = sys.getrecursionlimit()
        old = [(i%2 and "K:%d" or "V:A:%d") % i for i in range(limit*2)]
        new = [(i%2 and "K:%d" or "V:B:%d") % i for i in range(limit*2)]
        difflib.SequenceMatcher(None, old, new).get_opcodes()

    def test_make_file_default_charset(self):
        html_diff = difflib.HtmlDiff()
        output = html_diff.make_file(patch914575_from1.splitlines(),
                                     patch914575_to1.splitlines())
        self.assertIn('charset="utf-8"', output)

    def test_make_file_iso88591_charset(self):
        html_diff = difflib.HtmlDiff()
        output = html_diff.make_file(patch914575_from1.splitlines(),
                                     patch914575_to1.splitlines(),
                                     charset='iso-8859-1')
        self.assertIn('charset="iso-8859-1"', output)

    def test_make_file_usascii_charset_with_nonascii_input(self):
        html_diff = difflib.HtmlDiff()
        output = html_diff.make_file(patch914575_nonascii_from1.splitlines(),
                                     patch914575_nonascii_to1.splitlines(),
                                     charset='us-ascii')
        self.assertIn('charset="us-ascii"', output)
        self.assertIn('&#305;mpl&#305;c&#305;t', output)

class TestDiffer(unittest.TestCase):
    def test_close_matches_aligned(self):
        # Of the 4 closely matching pairs, we want 1 to match with 3,
        # and 2 with 4, to align with a "top to bottom" mental model.
        a = ["cat\n", "dog\n", "close match 1\n", "close match 2\n"]
        b = ["close match 3\n", "close match 4\n", "kitten\n", "puppy\n"]
        m = difflib.Differ().compare(a, b)
        self.assertEqual(list(m),
                           ['- cat\n',
                            '- dog\n',
                            '- close match 1\n',
                            '?             ^\n',
                            '+ close match 3\n',
                            '?             ^\n',
                            '- close match 2\n',
                            '?             ^\n',
                            '+ close match 4\n',
                            '?             ^\n',
                            '+ kitten\n',
                            '+ puppy\n'])

class TestOutputFormat(unittest.TestCase):
    def test_tab_delimiter(self):
        args = [['one'], ['two'], 'Original', 'Current',
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
        args = [['one'], ['two'], 'Original', 'Current']
        ud = difflib.unified_diff(*args, lineterm='')
        self.assertEqual(list(ud)[0:2], ["--- Original", "+++ Current"])

        cd = difflib.context_diff(*args, lineterm='')
        self.assertEqual(list(cd)[0:2], ["*** Original", "--- Current"])

    def test_range_format_unified(self):
        # Per the diff spec at http://www.unix.org/single_unix_specification/
        spec = '''\
           Each <range> field shall be of the form:
             %1d", <beginning line number>  if the range contains exactly one line,
           and:
            "%1d,%1d", <beginning line number>, <number of lines> otherwise.
           If a range is empty, its beginning line number shall be the number of
           the line just before the range, or 0 if the empty range starts the file.
        '''
        fmt = difflib._format_range_unified
        self.assertEqual(fmt(3,3), '3,0')
        self.assertEqual(fmt(3,4), '4')
        self.assertEqual(fmt(3,5), '4,2')
        self.assertEqual(fmt(3,6), '4,3')
        self.assertEqual(fmt(0,0), '0,0')

    def test_range_format_context(self):
        # Per the diff spec at http://www.unix.org/single_unix_specification/
        spec = '''\
           The range of lines in file1 shall be written in the following format
           if the range contains two or more lines:
               "*** %d,%d ****\n", <beginning line number>, <ending line number>
           and the following format otherwise:
               "*** %d ****\n", <ending line number>
           The ending line number of an empty range shall be the number of the preceding line,
           or 0 if the range is at the start of the file.

           Next, the range of lines in file2 shall be written in the following format
           if the range contains two or more lines:
               "--- %d,%d ----\n", <beginning line number>, <ending line number>
           and the following format otherwise:
               "--- %d ----\n", <ending line number>
        '''
        fmt = difflib._format_range_context
        self.assertEqual(fmt(3,3), '3')
        self.assertEqual(fmt(3,4), '4')
        self.assertEqual(fmt(3,5), '4,5')
        self.assertEqual(fmt(3,6), '4,6')
        self.assertEqual(fmt(0,0), '0')

    @force_colorized
    def test_unified_diff_colored_output(self):
        args = [['one', 'three'], ['two', 'three'], 'Original', 'Current',
            '2005-01-26 23:30:50', '2010-04-02 10:20:52']
        actual = list(difflib.unified_diff(*args, lineterm='', color=True))

        expect = [
            "\033[1m--- Original\t2005-01-26 23:30:50\033[0m",
            "\033[1m+++ Current\t2010-04-02 10:20:52\033[0m",
             "\033[36m@@ -1,2 +1,2 @@\033[0m",
             "\033[31m-one\033[0m",
             "\033[32m+two\033[0m",
             "\033[0m three\033[0m",
        ]
        self.assertEqual(expect, actual)


class TestBytes(unittest.TestCase):
    # don't really care about the content of the output, just the fact
    # that it's bytes and we don't crash
    def check(self, diff):
        diff = list(diff)   # trigger exceptions first
        for line in diff:
            self.assertIsInstance(
                line, bytes,
                "all lines of diff should be bytes, but got: %r" % line)

    def test_byte_content(self):
        # if we receive byte strings, we return byte strings
        a = [b'hello', b'andr\xe9']     # iso-8859-1 bytes
        b = [b'hello', b'andr\xc3\xa9'] # utf-8 bytes

        unified = difflib.unified_diff
        context = difflib.context_diff

        check = self.check
        check(difflib.diff_bytes(unified, a, a))
        check(difflib.diff_bytes(unified, a, b))

        # now with filenames (content and filenames are all bytes!)
        check(difflib.diff_bytes(unified, a, a, b'a', b'a'))
        check(difflib.diff_bytes(unified, a, b, b'a', b'b'))

        # and with filenames and dates
        check(difflib.diff_bytes(unified, a, a, b'a', b'a', b'2005', b'2013'))
        check(difflib.diff_bytes(unified, a, b, b'a', b'b', b'2005', b'2013'))

        # same all over again, with context diff
        check(difflib.diff_bytes(context, a, a))
        check(difflib.diff_bytes(context, a, b))
        check(difflib.diff_bytes(context, a, a, b'a', b'a'))
        check(difflib.diff_bytes(context, a, b, b'a', b'b'))
        check(difflib.diff_bytes(context, a, a, b'a', b'a', b'2005', b'2013'))
        check(difflib.diff_bytes(context, a, b, b'a', b'b', b'2005', b'2013'))

    def test_byte_filenames(self):
        # somebody renamed a file from ISO-8859-2 to UTF-8
        fna = b'\xb3odz.txt'    # "łodz.txt"
        fnb = b'\xc5\x82odz.txt'

        # they transcoded the content at the same time
        a = [b'\xa3odz is a city in Poland.']
        b = [b'\xc5\x81odz is a city in Poland.']

        check = self.check
        unified = difflib.unified_diff
        context = difflib.context_diff
        check(difflib.diff_bytes(unified, a, b, fna, fnb))
        check(difflib.diff_bytes(context, a, b, fna, fnb))

        def assertDiff(expect, actual):
            # do not compare expect and equal as lists, because unittest
            # uses difflib to report difference between lists
            actual = list(actual)
            self.assertEqual(len(expect), len(actual))
            for e, a in zip(expect, actual):
                self.assertEqual(e, a)

        expect = [
            b'--- \xb3odz.txt',
            b'+++ \xc5\x82odz.txt',
            b'@@ -1 +1 @@',
            b'-\xa3odz is a city in Poland.',
            b'+\xc5\x81odz is a city in Poland.',
        ]
        actual = difflib.diff_bytes(unified, a, b, fna, fnb, lineterm=b'')
        assertDiff(expect, actual)

        # with dates (plain ASCII)
        datea = b'2005-03-18'
        dateb = b'2005-03-19'
        check(difflib.diff_bytes(unified, a, b, fna, fnb, datea, dateb))
        check(difflib.diff_bytes(context, a, b, fna, fnb, datea, dateb))

        expect = [
            # note the mixed encodings here: this is deeply wrong by every
            # tenet of Unicode, but it doesn't crash, it's parseable by
            # patch, and it's how UNIX(tm) diff behaves
            b'--- \xb3odz.txt\t2005-03-18',
            b'+++ \xc5\x82odz.txt\t2005-03-19',
            b'@@ -1 +1 @@',
            b'-\xa3odz is a city in Poland.',
            b'+\xc5\x81odz is a city in Poland.',
        ]
        actual = difflib.diff_bytes(unified, a, b, fna, fnb, datea, dateb,
                                    lineterm=b'')
        assertDiff(expect, actual)


class TestInputTypes(unittest.TestCase):
    def _assert_type_error(self, msg, generator, *args):
        with self.assertRaises(TypeError) as ctx:
            list(generator(*args))
        self.assertEqual(msg, str(ctx.exception))

    def test_input_type_checks(self):
        unified = difflib.unified_diff
        context = difflib.context_diff

        expect = "input must be a sequence of strings, not str"
        self._assert_type_error(expect, unified, 'a', ['b'])
        self._assert_type_error(expect, context, 'a', ['b'])

        self._assert_type_error(expect, unified, ['a'], 'b')
        self._assert_type_error(expect, context, ['a'], 'b')

        expect = "lines to compare must be str, not NoneType (None)"
        self._assert_type_error(expect, unified, ['a'], [None])
        self._assert_type_error(expect, context, ['a'], [None])

    def test_mixed_types_content(self):
        # type of input content must be consistent: all str or all bytes
        a = [b'hello']
        b = ['hello']

        unified = difflib.unified_diff
        context = difflib.context_diff

        expect = "lines to compare must be str, not bytes (b'hello')"
        self._assert_type_error(expect, unified, a, b)
        self._assert_type_error(expect, unified, b, a)
        self._assert_type_error(expect, context, a, b)
        self._assert_type_error(expect, context, b, a)

        expect = "all arguments must be bytes, not str ('hello')"
        self._assert_type_error(expect, difflib.diff_bytes, unified, a, b)
        self._assert_type_error(expect, difflib.diff_bytes, unified, b, a)
        self._assert_type_error(expect, difflib.diff_bytes, context, a, b)
        self._assert_type_error(expect, difflib.diff_bytes, context, b, a)

    def test_mixed_types_filenames(self):
        # cannot pass filenames as bytes if content is str (this may not be
        # the right behaviour, but at least the test demonstrates how
        # things work)
        a = ['hello\n']
        b = ['ohell\n']
        fna = b'ol\xe9.txt'     # filename transcoded from ISO-8859-1
        fnb = b'ol\xc3a9.txt'   # to UTF-8
        self._assert_type_error(
            "all arguments must be str, not: b'ol\\xe9.txt'",
            difflib.unified_diff, a, b, fna, fnb)

    def test_mixed_types_dates(self):
        # type of dates must be consistent with type of contents
        a = [b'foo\n']
        b = [b'bar\n']
        datea = '1 fév'
        dateb = '3 fév'
        self._assert_type_error(
            "all arguments must be bytes, not str ('1 fév')",
            difflib.diff_bytes, difflib.unified_diff,
            a, b, b'a', b'b', datea, dateb)

        # if input is str, non-ASCII dates are fine
        a = ['foo\n']
        b = ['bar\n']
        list(difflib.unified_diff(a, b, 'a', 'b', datea, dateb))


class TestJunkAPIs(unittest.TestCase):
    def test_is_line_junk_true(self):
        for line in ['#', '  ', ' #', '# ', ' # ', '']:
            self.assertTrue(difflib.IS_LINE_JUNK(line), repr(line))

    def test_is_line_junk_false(self):
        for line in ['##', ' ##', '## ', 'abc ', 'abc #', 'Mr. Moose is up!']:
            self.assertFalse(difflib.IS_LINE_JUNK(line), repr(line))

    def test_is_line_junk_REDOS(self):
        evil_input = ('\t' * 1000000) + '##'
        self.assertFalse(difflib.IS_LINE_JUNK(evil_input))

    def test_is_character_junk_true(self):
        for char in [' ', '\t']:
            self.assertTrue(difflib.IS_CHARACTER_JUNK(char), repr(char))

    def test_is_character_junk_false(self):
        for char in ['a', '#', '\n', '\f', '\r', '\v']:
            self.assertFalse(difflib.IS_CHARACTER_JUNK(char), repr(char))

class TestFindLongest(unittest.TestCase):
    def longer_match_exists(self, a, b, n):
        return any(b_part in a for b_part in
                   [b[i:i + n + 1] for i in range(0, len(b) - n - 1)])

    def test_default_args(self):
        a = 'foo bar'
        b = 'foo baz bar'
        sm = difflib.SequenceMatcher(a=a, b=b)
        match = sm.find_longest_match()
        self.assertEqual(match.a, 0)
        self.assertEqual(match.b, 0)
        self.assertEqual(match.size, 6)
        self.assertEqual(a[match.a: match.a + match.size],
                         b[match.b: match.b + match.size])
        self.assertFalse(self.longer_match_exists(a, b, match.size))

        match = sm.find_longest_match(alo=2, blo=4)
        self.assertEqual(match.a, 3)
        self.assertEqual(match.b, 7)
        self.assertEqual(match.size, 4)
        self.assertEqual(a[match.a: match.a + match.size],
                         b[match.b: match.b + match.size])
        self.assertFalse(self.longer_match_exists(a[2:], b[4:], match.size))

        match = sm.find_longest_match(bhi=5, blo=1)
        self.assertEqual(match.a, 1)
        self.assertEqual(match.b, 1)
        self.assertEqual(match.size, 4)
        self.assertEqual(a[match.a: match.a + match.size],
                         b[match.b: match.b + match.size])
        self.assertFalse(self.longer_match_exists(a, b[1:5], match.size))

    def test_longest_match_with_popular_chars(self):
        a = 'dabcd'
        b = 'd'*100 + 'abc' + 'd'*100  # length over 200 so popular used
        sm = difflib.SequenceMatcher(a=a, b=b)
        match = sm.find_longest_match(0, len(a), 0, len(b))
        self.assertEqual(match.a, 0)
        self.assertEqual(match.b, 99)
        self.assertEqual(match.size, 5)
        self.assertEqual(a[match.a: match.a + match.size],
                         b[match.b: match.b + match.size])
        self.assertFalse(self.longer_match_exists(a, b, match.size))


def setUpModule():
    difflib.HtmlDiff._default_prefix = 0


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite(difflib))
    return tests


if __name__ == '__main__':
    unittest.main()
