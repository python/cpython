import io
import itertools
import shlex
import string
import unittest
from unittest import mock


# The original test data set was from shellwords, by Hartmut Goebel.

data = r"""x|x|
foo bar|foo|bar|
 foo bar|foo|bar|
 foo bar |foo|bar|
foo   bar    bla     fasel|foo|bar|bla|fasel|
x y  z              xxxx|x|y|z|xxxx|
\x bar|\|x|bar|
\ x bar|\|x|bar|
\ bar|\|bar|
foo \x bar|foo|\|x|bar|
foo \ x bar|foo|\|x|bar|
foo \ bar|foo|\|bar|
foo "bar" bla|foo|"bar"|bla|
"foo" "bar" "bla"|"foo"|"bar"|"bla"|
"foo" bar "bla"|"foo"|bar|"bla"|
"foo" bar bla|"foo"|bar|bla|
foo 'bar' bla|foo|'bar'|bla|
'foo' 'bar' 'bla'|'foo'|'bar'|'bla'|
'foo' bar 'bla'|'foo'|bar|'bla'|
'foo' bar bla|'foo'|bar|bla|
blurb foo"bar"bar"fasel" baz|blurb|foo"bar"bar"fasel"|baz|
blurb foo'bar'bar'fasel' baz|blurb|foo'bar'bar'fasel'|baz|
""|""|
''|''|
foo "" bar|foo|""|bar|
foo '' bar|foo|''|bar|
foo "" "" "" bar|foo|""|""|""|bar|
foo '' '' '' bar|foo|''|''|''|bar|
\""|\|""|
"\"|"\"|
"foo\ bar"|"foo\ bar"|
"foo\\ bar"|"foo\\ bar"|
"foo\\ bar\"|"foo\\ bar\"|
"foo\\" bar\""|"foo\\"|bar|\|""|
"foo\\ bar\" dfadf"|"foo\\ bar\"|dfadf"|
"foo\\\ bar\" dfadf"|"foo\\\ bar\"|dfadf"|
"foo\\\x bar\" dfadf"|"foo\\\x bar\"|dfadf"|
"foo\x bar\" dfadf"|"foo\x bar\"|dfadf"|
\''|\|''|
'foo\ bar'|'foo\ bar'|
'foo\\ bar'|'foo\\ bar'|
"foo\\\x bar\" df'a\ 'df'|"foo\\\x bar\"|df'a|\|'df'|
\"foo"|\|"foo"|
\"foo"\x|\|"foo"|\|x|
"foo\x"|"foo\x"|
"foo\ "|"foo\ "|
foo\ xx|foo|\|xx|
foo\ x\x|foo|\|x|\|x|
foo\ x\x\""|foo|\|x|\|x|\|""|
"foo\ x\x"|"foo\ x\x"|
"foo\ x\x\\"|"foo\ x\x\\"|
"foo\ x\x\\""foobar"|"foo\ x\x\\"|"foobar"|
"foo\ x\x\\"\''"foobar"|"foo\ x\x\\"|\|''|"foobar"|
"foo\ x\x\\"\'"fo'obar"|"foo\ x\x\\"|\|'"fo'|obar"|
"foo\ x\x\\"\'"fo'obar" 'don'\''t'|"foo\ x\x\\"|\|'"fo'|obar"|'don'|\|''|t'|
'foo\ bar'|'foo\ bar'|
'foo\\ bar'|'foo\\ bar'|
foo\ bar|foo|\|bar|
foo#bar\nbaz|foobaz|
:-) ;-)|:|-|)|;|-|)|
áéíóú|á|é|í|ó|ú|
"""

posix_data = r"""x|x|
foo bar|foo|bar|
 foo bar|foo|bar|
 foo bar |foo|bar|
foo   bar    bla     fasel|foo|bar|bla|fasel|
x y  z              xxxx|x|y|z|xxxx|
\x bar|x|bar|
\ x bar| x|bar|
\ bar| bar|
foo \x bar|foo|x|bar|
foo \ x bar|foo| x|bar|
foo \ bar|foo| bar|
foo "bar" bla|foo|bar|bla|
"foo" "bar" "bla"|foo|bar|bla|
"foo" bar "bla"|foo|bar|bla|
"foo" bar bla|foo|bar|bla|
foo 'bar' bla|foo|bar|bla|
'foo' 'bar' 'bla'|foo|bar|bla|
'foo' bar 'bla'|foo|bar|bla|
'foo' bar bla|foo|bar|bla|
blurb foo"bar"bar"fasel" baz|blurb|foobarbarfasel|baz|
blurb foo'bar'bar'fasel' baz|blurb|foobarbarfasel|baz|
""||
''||
foo "" bar|foo||bar|
foo '' bar|foo||bar|
foo "" "" "" bar|foo||||bar|
foo '' '' '' bar|foo||||bar|
\"|"|
"\""|"|
"foo\ bar"|foo\ bar|
"foo\\ bar"|foo\ bar|
"foo\\ bar\""|foo\ bar"|
"foo\\" bar\"|foo\|bar"|
"foo\\ bar\" dfadf"|foo\ bar" dfadf|
"foo\\\ bar\" dfadf"|foo\\ bar" dfadf|
"foo\\\x bar\" dfadf"|foo\\x bar" dfadf|
"foo\x bar\" dfadf"|foo\x bar" dfadf|
\'|'|
'foo\ bar'|foo\ bar|
'foo\\ bar'|foo\\ bar|
"foo\\\x bar\" df'a\ 'df"|foo\\x bar" df'a\ 'df|
\"foo|"foo|
\"foo\x|"foox|
"foo\x"|foo\x|
"foo\ "|foo\ |
foo\ xx|foo xx|
foo\ x\x|foo xx|
foo\ x\x\"|foo xx"|
"foo\ x\x"|foo\ x\x|
"foo\ x\x\\"|foo\ x\x\|
"foo\ x\x\\""foobar"|foo\ x\x\foobar|
"foo\ x\x\\"\'"foobar"|foo\ x\x\'foobar|
"foo\ x\x\\"\'"fo'obar"|foo\ x\x\'fo'obar|
"foo\ x\x\\"\'"fo'obar" 'don'\''t'|foo\ x\x\'fo'obar|don't|
"foo\ x\x\\"\'"fo'obar" 'don'\''t' \\|foo\ x\x\'fo'obar|don't|\|
'foo\ bar'|foo\ bar|
'foo\\ bar'|foo\\ bar|
foo\ bar|foo bar|
foo#bar\nbaz|foo|baz|
:-) ;-)|:-)|;-)|
áéíóú|áéíóú|
"""

class ShlexTest(unittest.TestCase):
    def setUp(self):
        self.data = [x.split("|")[:-1]
                     for x in data.splitlines()]
        self.posix_data = [x.split("|")[:-1]
                           for x in posix_data.splitlines()]
        for item in self.data:
            item[0] = item[0].replace(r"\n", "\n")
        for item in self.posix_data:
            item[0] = item[0].replace(r"\n", "\n")

    def splitTest(self, data, comments):
        for i in range(len(data)):
            l = shlex.split(data[i][0], comments=comments)
            self.assertEqual(l, data[i][1:],
                             "%s: %s != %s" %
                             (data[i][0], l, data[i][1:]))

    def oldSplit(self, s):
        ret = []
        lex = shlex.shlex(io.StringIO(s))
        tok = lex.get_token()
        while tok:
            ret.append(tok)
            tok = lex.get_token()
        return ret

    def testSplitNone(self):
        with self.assertRaises(ValueError):
            shlex.split(None)

    def testSplitPosix(self):
        """Test data splitting with posix parser"""
        self.splitTest(self.posix_data, comments=True)

    def testCompat(self):
        """Test compatibility interface"""
        for i in range(len(self.data)):
            l = self.oldSplit(self.data[i][0])
            self.assertEqual(l, self.data[i][1:],
                             "%s: %s != %s" %
                             (self.data[i][0], l, self.data[i][1:]))

    def testSyntaxSplitAmpersandAndPipe(self):
        """Test handling of syntax splitting of &, |"""
        # Could take these forms: &&, &, |&, ;&, ;;&
        # of course, the same applies to | and ||
        # these should all parse to the same output
        for delimiter in ('&&', '&', '|&', ';&', ';;&',
                          '||', '|', '&|', ';|', ';;|'):
            src = ['echo hi %s echo bye' % delimiter,
                   'echo hi%secho bye' % delimiter]
            ref = ['echo', 'hi', delimiter, 'echo', 'bye']
            for ss, ws in itertools.product(src, (False, True)):
                s = shlex.shlex(ss, punctuation_chars=True)
                s.whitespace_split = ws
                result = list(s)
                self.assertEqual(ref, result,
                                 "While splitting '%s' [ws=%s]" % (ss, ws))

    def testSyntaxSplitSemicolon(self):
        """Test handling of syntax splitting of ;"""
        # Could take these forms: ;, ;;, ;&, ;;&
        # these should all parse to the same output
        for delimiter in (';', ';;', ';&', ';;&'):
            src = ['echo hi %s echo bye' % delimiter,
                   'echo hi%s echo bye' % delimiter,
                   'echo hi%secho bye' % delimiter]
            ref = ['echo', 'hi', delimiter, 'echo', 'bye']
            for ss, ws in itertools.product(src, (False, True)):
                s = shlex.shlex(ss, punctuation_chars=True)
                s.whitespace_split = ws
                result = list(s)
                self.assertEqual(ref, result,
                                 "While splitting '%s' [ws=%s]" % (ss, ws))

    def testSyntaxSplitRedirect(self):
        """Test handling of syntax splitting of >"""
        # of course, the same applies to <, |
        # these should all parse to the same output
        for delimiter in ('<', '|'):
            src = ['echo hi %s out' % delimiter,
                   'echo hi%s out' % delimiter,
                   'echo hi%sout' % delimiter]
            ref = ['echo', 'hi', delimiter, 'out']
            for ss, ws in itertools.product(src, (False, True)):
                s = shlex.shlex(ss, punctuation_chars=True)
                result = list(s)
                self.assertEqual(ref, result,
                                 "While splitting '%s' [ws=%s]" % (ss, ws))

    def testSyntaxSplitParen(self):
        """Test handling of syntax splitting of ()"""
        # these should all parse to the same output
        src = ['( echo hi )',
               '(echo hi)']
        ref = ['(', 'echo', 'hi', ')']
        for ss, ws in itertools.product(src, (False, True)):
            s = shlex.shlex(ss, punctuation_chars=True)
            s.whitespace_split = ws
            result = list(s)
            self.assertEqual(ref, result,
                             "While splitting '%s' [ws=%s]" % (ss, ws))

    def testSyntaxSplitCustom(self):
        """Test handling of syntax splitting with custom chars"""
        ss = "~/a&&b-c --color=auto||d *.py?"
        ref = ['~/a', '&', '&', 'b-c', '--color=auto', '||', 'd', '*.py?']
        s = shlex.shlex(ss, punctuation_chars="|")
        result = list(s)
        self.assertEqual(ref, result, "While splitting '%s' [ws=False]" % ss)
        ref = ['~/a&&b-c', '--color=auto', '||', 'd', '*.py?']
        s = shlex.shlex(ss, punctuation_chars="|")
        s.whitespace_split = True
        result = list(s)
        self.assertEqual(ref, result, "While splitting '%s' [ws=True]" % ss)

    def testTokenTypes(self):
        """Test that tokens are split with types as expected."""
        for source, expected in (
                                ('a && b || c',
                                 [('a', 'a'), ('&&', 'c'), ('b', 'a'),
                                  ('||', 'c'), ('c', 'a')]),
                              ):
            s = shlex.shlex(source, punctuation_chars=True)
            observed = []
            while True:
                t = s.get_token()
                if t == s.eof:
                    break
                if t[0] in s.punctuation_chars:
                    tt = 'c'
                else:
                    tt = 'a'
                observed.append((t, tt))
            self.assertEqual(observed, expected)

    def testPunctuationInWordChars(self):
        """Test that any punctuation chars are removed from wordchars"""
        s = shlex.shlex('a_b__c', punctuation_chars='_')
        self.assertNotIn('_', s.wordchars)
        self.assertEqual(list(s), ['a', '_', 'b', '__', 'c'])

    def testPunctuationWithWhitespaceSplit(self):
        """Test that with whitespace_split, behaviour is as expected"""
        s = shlex.shlex('a  && b  ||  c', punctuation_chars='&')
        # whitespace_split is False, so splitting will be based on
        # punctuation_chars
        self.assertEqual(list(s), ['a', '&&', 'b', '|', '|', 'c'])
        s = shlex.shlex('a  && b  ||  c', punctuation_chars='&')
        s.whitespace_split = True
        # whitespace_split is True, so splitting will be based on
        # white space
        self.assertEqual(list(s), ['a', '&&', 'b', '||', 'c'])

    def testPunctuationWithPosix(self):
        """Test that punctuation_chars and posix behave correctly together."""
        # see Issue #29132
        s = shlex.shlex('f >"abc"', posix=True, punctuation_chars=True)
        self.assertEqual(list(s), ['f', '>', 'abc'])
        s = shlex.shlex('f >\\"abc\\"', posix=True, punctuation_chars=True)
        self.assertEqual(list(s), ['f', '>', '"abc"'])

    def testEmptyStringHandling(self):
        """Test that parsing of empty strings is correctly handled."""
        # see Issue #21999
        expected = ['', ')', 'abc']
        for punct in (False, True):
            s = shlex.shlex("'')abc", posix=True, punctuation_chars=punct)
            slist = list(s)
            self.assertEqual(slist, expected)
        expected = ["''", ')', 'abc']
        s = shlex.shlex("'')abc", punctuation_chars=True)
        self.assertEqual(list(s), expected)

    def testUnicodeHandling(self):
        """Test punctuation_chars and whitespace_split handle unicode."""
        ss = "\u2119\u01b4\u2602\u210c\u00f8\u1f24"
        # Should be parsed as one complete token (whitespace_split=True).
        ref = ['\u2119\u01b4\u2602\u210c\u00f8\u1f24']
        s = shlex.shlex(ss, punctuation_chars=True)
        s.whitespace_split = True
        self.assertEqual(list(s), ref)
        # Without whitespace_split, uses wordchars and splits on all.
        ref = ['\u2119', '\u01b4', '\u2602', '\u210c', '\u00f8', '\u1f24']
        s = shlex.shlex(ss, punctuation_chars=True)
        self.assertEqual(list(s), ref)

    def testQuote(self):
        safeunquoted = string.ascii_letters + string.digits + '@%_-+=:,./'
        unicode_sample = '\xe9\xe0\xdf'  # e + acute accent, a + grave, sharp s
        unsafe = '"`$\\!' + unicode_sample

        self.assertEqual(shlex.quote(''), "''")
        self.assertEqual(shlex.quote(safeunquoted), safeunquoted)
        self.assertEqual(shlex.quote('test file name'), "'test file name'")
        for u in unsafe:
            self.assertEqual(shlex.quote('test%sname' % u),
                             "'test%sname'" % u)
        for u in unsafe:
            self.assertEqual(shlex.quote("test%s'name'" % u),
                             "'test%s'\"'\"'name'\"'\"''" % u)

    def testJoin(self):
        for split_command, command in [
            (['a ', 'b'], "'a ' b"),
            (['a', ' b'], "a ' b'"),
            (['a', ' ', 'b'], "a ' ' b"),
            (['"a', 'b"'], '\'"a\' \'b"\''),
        ]:
            with self.subTest(command=command):
                joined = shlex.join(split_command)
                self.assertEqual(joined, command)

    def testJoinRoundtrip(self):
        all_data = self.data + self.posix_data
        for command, *split_command in all_data:
            with self.subTest(command=command):
                joined = shlex.join(split_command)
                resplit = shlex.split(joined)
                self.assertEqual(split_command, resplit)

    def testPunctuationCharsReadOnly(self):
        punctuation_chars = "/|$%^"
        shlex_instance = shlex.shlex(punctuation_chars=punctuation_chars)
        self.assertEqual(shlex_instance.punctuation_chars, punctuation_chars)
        with self.assertRaises(AttributeError):
            shlex_instance.punctuation_chars = False


# Allow this test to be used with old shlex.py
if not getattr(shlex, "split", None):
    for methname in dir(ShlexTest):
        if methname.startswith("test") and methname != "testCompat":
            delattr(ShlexTest, methname)

if __name__ == "__main__":
    unittest.main()
