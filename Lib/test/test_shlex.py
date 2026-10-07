import io
import itertools
import os
import shlex
import string
import tempfile
import unittest
from unittest.mock import patch
from test.support import cpython_only
from test.support import import_helper


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
        self.assertEqual(shlex.quote(None), "''")
        self.assertEqual(shlex.quote(safeunquoted), safeunquoted)
        self.assertEqual(shlex.quote('test file name'), "'test file name'")
        for u in unsafe:
            self.assertEqual(shlex.quote('test%sname' % u),
                             "'test%sname'" % u)
        for u in unsafe:
            self.assertEqual(shlex.quote("test%s'name'" % u),
                             "'test%s'\"'\"'name'\"'\"''" % u)
        self.assertRaises(TypeError, shlex.quote, 42)
        self.assertRaises(TypeError, shlex.quote, b"abc")

    def testForceQuote(self):
        self.assertEqual(shlex.quote("spam"), "spam")
        self.assertEqual(shlex.quote("spam", force=False), "spam")
        self.assertEqual(shlex.quote("spam", force=True), "'spam'")
        self.assertEqual(shlex.quote("spam eggs", force=False), "'spam eggs'")
        self.assertEqual(shlex.quote("spam eggs", force=True), "'spam eggs'")
        self.assertEqual(shlex.quote("two's-complement", force=False), "'two'\"'\"'s-complement'")

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

    def testLinenoAfterNewLine(self):
        s = shlex.shlex("line 1\nline 2")
        self.assertEqual(s.lineno, 1)  # before consumption
        list(s)
        self.assertEqual(s.lineno, 2)

    def testLinenoAfterComment(self):
        """Comment handler increments lineno even without a trailing newline."""
        s = shlex.shlex("line 1 # line 2")
        list(s)
        self.assertEqual(s.lineno, 2)

    def testPushToken(self):
        s = shlex.shlex("b c")
        s.push_token("a")
        self.assertListEqual(list(s), ["a", "b", "c"])

    def testPushTokenLifo(self):
        s = shlex.shlex("")
        s.push_token("first")
        s.push_token("last")
        self.assertListEqual(list(s), ["last", "first"])

    def testPushTokenDebug(self):
        s = shlex.shlex("")
        s.debug = 1
        tok = "a"
        with patch("builtins.print") as mock_print:
            s.push_token(tok)
        mock_print.assert_called_once_with(f"shlex: pushing token {tok!r}")

    def testPushSourceString(self):
        s = shlex.shlex("world")
        s.push_source("hello")
        self.assertListEqual(list(s), ["hello", "world"])

    def testPushSourceStream(self):
        s = shlex.shlex("world")
        s.push_source(io.StringIO("hello"))
        self.assertListEqual(list(s), ["hello", "world"])

    def testPushSourceStreamDebug(self):
        s = shlex.shlex("")
        stream = io.StringIO("hello")
        s.debug = 1
        with patch("builtins.print") as mock_print:
            s.push_source(stream)
        mock_print.assert_called_once_with(f"shlex: pushing to stream {stream}")

    def testPushSourceNewfile(self):
        """shlex.push_source sets infile to newfile; pop_source restores the original on exhaustion."""
        original_file = "original.sh"
        new_file = "new.sh"
        s = shlex.shlex("b", infile=original_file)
        s.debug = 1
        with patch("builtins.print") as mock_print:
            s.push_source("a", newfile=new_file)
        mock_print.assert_called_once_with(f"shlex: pushing to file {new_file}")
        self.assertEqual(s.infile, new_file)
        s.debug = 0
        list(s)
        self.assertEqual(s.infile, original_file)

    def testPopSourceDebug(self):
        """pop_source emits debug output when debug is set."""
        s = shlex.shlex("b")
        original_stream = s.instream
        s.push_source("a")
        s.debug = 1
        with patch("builtins.print") as mock_print:
            list(s)  # exhausts pushed source and triggers pop_source internally
        mock_print.assert_any_call(f"shlex: popping to {original_stream}, line 1")

    def testErrorLeaderTracksPosition(self):
        infile_label = "test.sh"
        s = shlex.shlex("line 1\nline 2", infile=infile_label)
        list(s)
        result = s.error_leader()
        self.assertEqual(result, f'"{infile_label}", line 2: ')

    def testErrorLeaderOverrides(self):
        s = shlex.shlex("foo", infile="original.sh")
        infile_label_override = "override.sh"
        lineno_override = 42
        result = s.error_leader(infile=infile_label_override, lineno=lineno_override)
        self.assertEqual(result, f'"{infile_label_override}", line {lineno_override}: ')

    def testNoClosingQuotation(self):
        s = shlex.shlex('"foo')
        with self.assertRaisesRegex(ValueError, "No closing quotation"):
            list(s)

    def testNoEscapedCharacter(self):
        s = shlex.shlex("\\", posix=True)
        with self.assertRaisesRegex(ValueError, "No escaped character"):
            list(s)

    def testSourcehookStripsQuotes(self):
        with tempfile.NamedTemporaryFile(mode="w", suffix=".sh", delete_on_close=False) as f:
            f.write("hello")
            f.close()
            s = shlex.shlex("")
            newfile, stream = s.sourcehook(f'"{f.name}"')
            stream.close()
        self.assertEqual(newfile, f.name)

    def testSourcehookAbsolutePath(self):
        with tempfile.NamedTemporaryFile(mode="w", delete_on_close=False) as f:
            f.close()
            s = shlex.shlex("", infile="/some/dir/main.sh")
            newfile, stream = s.sourcehook(f.name)
            stream.close()
        self.assertEqual(newfile, f.name)

    def testSourcehookRelativePath(self):
        with tempfile.TemporaryDirectory() as d:
            fpath = os.path.join(d, "included.sh")
            with open(fpath, "w"):
                pass
            s = shlex.shlex("", infile=os.path.join(d, "main.sh"))
            newfile, stream = s.sourcehook("included.sh")
            stream.close()
            self.assertEqual(newfile, fpath)

    def testSourceInclusion(self):
        """shlex.source sets a trigger keyword: when the lexer reads a token equal
        to it, the next token is consumed as a filename and passed to
        sourcehook, which returns a stream to push onto the input stack.
        Tokens flow from that stream first, then resume from the original.
        """
        s = shlex.shlex("trigger filename remaining")
        s.source = "trigger"
        s.sourcehook = lambda f: (f, io.StringIO("included"))
        self.assertEqual(list(s), ["included", "remaining"])

    def testGetTokenPopsPushbackDebug(self):
        s = shlex.shlex("")
        s.push_token("hello")
        s.debug = 1  # set after push_token to isolate the pop-token branch
        with patch("builtins.print") as mock_print:
            tok = s.get_token()
        self.assertEqual(tok, "hello")
        mock_print.assert_called_once_with("shlex: popping token 'hello'")

    def testDebugWhitespaceInWhitespaceState(self):
        s = shlex.shlex(" a")
        s.debug = 2
        with patch("builtins.print") as mock_print:
            list(s)
        mock_print.assert_any_call("shlex: I see whitespace in whitespace state")

    def testDebugWhitespaceInWordState(self):
        s = shlex.shlex("a b")
        s.debug = 2
        with patch("builtins.print") as mock_print:
            list(s)
        mock_print.assert_any_call("shlex: I see whitespace in word state")

    def testDebugPunctuationInWordState(self):
        s = shlex.shlex("a(")
        s.debug = 2
        with patch("builtins.print") as mock_print:
            list(s)
        mock_print.assert_any_call("shlex: I see punctuation in word state")

    def testDebugRawToken(self):
        s = shlex.shlex("hello")
        s.debug = 2
        with patch("builtins.print") as mock_print:
            list(s)
        mock_print.assert_any_call("shlex: raw token='hello'")

    def testDebugEOFInQuote(self):
        s = shlex.shlex('"oops', posix=True)
        s.debug = 2
        with patch('builtins.print') as mock_print:
            with self.assertRaises(ValueError):
                list(s)
        msgs = [call.args[0] for call in mock_print.call_args_list]
        self.assertTrue(any("EOF in quotes" in m for m in msgs))

    def testDebugEOFInEscape(self):
        s = shlex.shlex("oops\\", posix=True)
        s.debug = 2
        with patch("builtins.print") as mock_print:
            with self.assertRaises(ValueError):
                list(s)
        msgs = [call.args[0] for call in mock_print.call_args_list]
        self.assertTrue(any("EOF in escape" in m for m in msgs))

    def testDebugStateTrace(self):
        s = shlex.shlex("a")
        s.debug = 3
        with patch("builtins.print") as mock_print:
            list(s)
        mock_print.assert_any_call("shlex: in state ' ' I see character: 'a'")

    @cpython_only
    def test_lazy_imports(self):
        import_helper.ensure_lazy_imports('shlex', {'collections', 're', 'os'})


# Allow this test to be used with old shlex.py
if not getattr(shlex, "split", None):
    for methname in dir(ShlexTest):
        if methname.startswith("test") and methname != "testCompat":
            delattr(ShlexTest, methname)

if __name__ == "__main__":
    unittest.main()
