from test import support
from test.support import os_helper
from tokenize import (tokenize, untokenize, NUMBER, NAME, OP,
                     STRING, ENDMARKER, ENCODING, tok_name, detect_encoding,
                     open as tokenize_open, Untokenizer, generate_tokens,
                     NEWLINE, _generate_tokens_from_c_tokenizer, DEDENT, TokenInfo,
                     TokenError)
from io import BytesIO, StringIO
import unittest
from textwrap import dedent
from unittest import TestCase, mock
from test.test_grammar import (VALID_UNDERSCORE_LITERALS,
                               INVALID_UNDERSCORE_LITERALS)
from test.support import os_helper
from test.support.script_helper import run_test_script, make_script, run_python_until_end
import os
import token

# Converts a source string into a list of textual representation
# of the tokens such as:
# `    NAME       'if'          (1, 0) (1, 2)`
# to make writing tests easier.
def stringify_tokens_from_source(token_generator, source_string):
    result = []
    num_lines = len(source_string.splitlines())
    missing_trailing_nl = source_string[-1] not in '\r\n'

    for type, token, start, end, line in token_generator:
        if type == ENDMARKER:
            break
        # Ignore the new line on the last line if the input lacks one
        if missing_trailing_nl and type == NEWLINE and end[0] == num_lines:
            continue
        type = tok_name[type]
        result.append(f"    {type:10} {token!r:13} {start} {end}")

    return result

class TokenizeTest(TestCase):
    # Tests for the tokenize module.

    # The tests can be really simple. Given a small fragment of source
    # code, print out a table with tokens. The ENDMARKER, ENCODING and
    # final NEWLINE are omitted for brevity.

    def check_tokenize(self, s, expected):
        # Format the tokens in s in a table format.
        # The ENDMARKER and final NEWLINE are omitted.
        f = BytesIO(s.encode('utf-8'))
        result = stringify_tokens_from_source(tokenize(f.readline), s)
        self.assertEqual(result,
                         ["    ENCODING   'utf-8'       (0, 0) (0, 0)"] +
                         expected.rstrip().splitlines())

    def test_invalid_readline(self):
        def gen():
            yield "sdfosdg"
            yield "sdfosdg"
        with self.assertRaises(TypeError):
            list(tokenize(gen().__next__))

        def gen():
            yield b"sdfosdg"
            yield b"sdfosdg"
        with self.assertRaises(TypeError):
            list(generate_tokens(gen().__next__))

        def gen():
            yield "sdfosdg"
            1/0
        with self.assertRaises(ZeroDivisionError):
            list(generate_tokens(gen().__next__))

    def test_implicit_newline(self):
        # Make sure that the tokenizer puts in an implicit NEWLINE
        # when the input lacks a trailing new line.
        f = BytesIO("x".encode('utf-8'))
        tokens = list(tokenize(f.readline))
        self.assertEqual(tokens[-2].type, NEWLINE)
        self.assertEqual(tokens[-1].type, ENDMARKER)

    def test_basic(self):
        self.check_tokenize("1 + 1", """\
    NUMBER     '1'           (1, 0) (1, 1)
    OP         '+'           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    """)
        self.check_tokenize("if False:\n"
                            "    # NL\n"
                            "    \n"
                            "    True = False # NEWLINE\n", """\
    NAME       'if'          (1, 0) (1, 2)
    NAME       'False'       (1, 3) (1, 8)
    OP         ':'           (1, 8) (1, 9)
    NEWLINE    '\\n'          (1, 9) (1, 10)
    COMMENT    '# NL'        (2, 4) (2, 8)
    NL         '\\n'          (2, 8) (2, 9)
    NL         '\\n'          (3, 4) (3, 5)
    INDENT     '    '        (4, 0) (4, 4)
    NAME       'True'        (4, 4) (4, 8)
    OP         '='           (4, 9) (4, 10)
    NAME       'False'       (4, 11) (4, 16)
    COMMENT    '# NEWLINE'   (4, 17) (4, 26)
    NEWLINE    '\\n'          (4, 26) (4, 27)
    DEDENT     ''            (5, 0) (5, 0)
    """)

        self.check_tokenize("if True:\r\n    # NL\r\n    foo='bar'\r\n\r\n", """\
    NAME       'if'          (1, 0) (1, 2)
    NAME       'True'        (1, 3) (1, 7)
    OP         ':'           (1, 7) (1, 8)
    NEWLINE    '\\r\\n'        (1, 8) (1, 10)
    COMMENT    '# NL'        (2, 4) (2, 8)
    NL         '\\r\\n'        (2, 8) (2, 10)
    INDENT     '    '        (3, 0) (3, 4)
    NAME       'foo'         (3, 4) (3, 7)
    OP         '='           (3, 7) (3, 8)
    STRING     "\'bar\'"       (3, 8) (3, 13)
    NEWLINE    '\\r\\n'        (3, 13) (3, 15)
    NL         '\\r\\n'        (4, 0) (4, 2)
    DEDENT     ''            (5, 0) (5, 0)
            """)

        self.check_tokenize("x = 1 + \\\r\n1\r\n", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    OP         '+'           (1, 6) (1, 7)
    NUMBER     '1'           (2, 0) (2, 1)
    NEWLINE    '\\r\\n'        (2, 1) (2, 3)
            """)

        indent_error_file = b"""\
def k(x):
    x += 2
  x += 5
"""
        readline = BytesIO(indent_error_file).readline
        with self.assertRaisesRegex(IndentationError,
                                    "unindent does not match any "
                                    "outer indentation level") as e:
            for tok in tokenize(readline):
                pass
        self.assertEqual(e.exception.lineno, 3)
        self.assertEqual(e.exception.filename, '<string>')
        self.assertEqual(e.exception.end_lineno, None)
        self.assertEqual(e.exception.end_offset, None)
        self.assertEqual(
            e.exception.msg,
            'unindent does not match any outer indentation level')
        self.assertEqual(e.exception.offset, 9)
        self.assertEqual(e.exception.text, '  x += 5')

    def test_int(self):
        # Ordinary integers and binary operators
        self.check_tokenize("0xff <= 255", """\
    NUMBER     '0xff'        (1, 0) (1, 4)
    OP         '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    """)
        self.check_tokenize("0b10 <= 255", """\
    NUMBER     '0b10'        (1, 0) (1, 4)
    OP         '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    """)
        self.check_tokenize("0o123 <= 0O123", """\
    NUMBER     '0o123'       (1, 0) (1, 5)
    OP         '<='          (1, 6) (1, 8)
    NUMBER     '0O123'       (1, 9) (1, 14)
    """)
        self.check_tokenize("1234567 > ~0x15", """\
    NUMBER     '1234567'     (1, 0) (1, 7)
    OP         '>'           (1, 8) (1, 9)
    OP         '~'           (1, 10) (1, 11)
    NUMBER     '0x15'        (1, 11) (1, 15)
    """)
        self.check_tokenize("2134568 != 1231515", """\
    NUMBER     '2134568'     (1, 0) (1, 7)
    OP         '!='          (1, 8) (1, 10)
    NUMBER     '1231515'     (1, 11) (1, 18)
    """)
        self.check_tokenize("(-124561-1) & 200000000", """\
    OP         '('           (1, 0) (1, 1)
    OP         '-'           (1, 1) (1, 2)
    NUMBER     '124561'      (1, 2) (1, 8)
    OP         '-'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         ')'           (1, 10) (1, 11)
    OP         '&'           (1, 12) (1, 13)
    NUMBER     '200000000'   (1, 14) (1, 23)
    """)
        self.check_tokenize("0xdeadbeef != -1", """\
    NUMBER     '0xdeadbeef'  (1, 0) (1, 10)
    OP         '!='          (1, 11) (1, 13)
    OP         '-'           (1, 14) (1, 15)
    NUMBER     '1'           (1, 15) (1, 16)
    """)
        self.check_tokenize("0xdeadc0de & 12345", """\
    NUMBER     '0xdeadc0de'  (1, 0) (1, 10)
    OP         '&'           (1, 11) (1, 12)
    NUMBER     '12345'       (1, 13) (1, 18)
    """)
        self.check_tokenize("0xFF & 0x15 | 1234", """\
    NUMBER     '0xFF'        (1, 0) (1, 4)
    OP         '&'           (1, 5) (1, 6)
    NUMBER     '0x15'        (1, 7) (1, 11)
    OP         '|'           (1, 12) (1, 13)
    NUMBER     '1234'        (1, 14) (1, 18)
    """)

    def test_long(self):
        # Long integers
        self.check_tokenize("x = 0", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '0'           (1, 4) (1, 5)
    """)
        self.check_tokenize("x = 0xfffffffffff", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '0xfffffffffff' (1, 4) (1, 17)
    """)
        self.check_tokenize("x = 123141242151251616110", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '123141242151251616110' (1, 4) (1, 25)
    """)
        self.check_tokenize("x = -15921590215012591", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    OP         '-'           (1, 4) (1, 5)
    NUMBER     '15921590215012591' (1, 5) (1, 22)
    """)

    def test_float(self):
        # Floating point numbers
        self.check_tokenize("x = 3.14159", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3.14159'     (1, 4) (1, 11)
    """)
        self.check_tokenize("x = 314159.", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '314159.'     (1, 4) (1, 11)
    """)
        self.check_tokenize("x = .314159", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '.314159'     (1, 4) (1, 11)
    """)
        self.check_tokenize("x = 3e14159", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3e14159'     (1, 4) (1, 11)
    """)
        self.check_tokenize("x = 3E123", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3E123'       (1, 4) (1, 9)
    """)
        self.check_tokenize("x+y = 3e-1230", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '+'           (1, 1) (1, 2)
    NAME       'y'           (1, 2) (1, 3)
    OP         '='           (1, 4) (1, 5)
    NUMBER     '3e-1230'     (1, 6) (1, 13)
    """)
        self.check_tokenize("x = 3.14e159", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3.14e159'    (1, 4) (1, 12)
    """)

    def test_underscore_literals(self):
        def number_token(s):
            f = BytesIO(s.encode('utf-8'))
            for toktype, token, start, end, line in tokenize(f.readline):
                if toktype == NUMBER:
                    return token
            return 'invalid token'
        for lit in VALID_UNDERSCORE_LITERALS:
            if '(' in lit:
                # this won't work with compound complex inputs
                continue
            self.assertEqual(number_token(lit), lit)
        # Valid cases with extra underscores in the tokenize module
        # See gh-105549 for context
        extra_valid_cases = {"0_7", "09_99"}
        for lit in INVALID_UNDERSCORE_LITERALS:
            if lit in extra_valid_cases:
                continue
            try:
                number_token(lit)
            except TokenError:
                continue
            self.assertNotEqual(number_token(lit), lit)

    def test_string(self):
        # String literals
        self.check_tokenize("x = ''; y = \"\"", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "''"          (1, 4) (1, 6)
    OP         ';'           (1, 6) (1, 7)
    NAME       'y'           (1, 8) (1, 9)
    OP         '='           (1, 10) (1, 11)
    STRING     '""'          (1, 12) (1, 14)
    """)
        self.check_tokenize("x = '\"'; y = \"'\"", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '\\'"\\''       (1, 4) (1, 7)
    OP         ';'           (1, 7) (1, 8)
    NAME       'y'           (1, 9) (1, 10)
    OP         '='           (1, 11) (1, 12)
    STRING     '"\\'"'        (1, 13) (1, 16)
    """)
        self.check_tokenize("x = \"doesn't \"shrink\", does it\"", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '"doesn\\'t "' (1, 4) (1, 14)
    NAME       'shrink'      (1, 14) (1, 20)
    STRING     '", does it"' (1, 20) (1, 31)
    """)
        self.check_tokenize("x = 'abc' + 'ABC'", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "'abc'"       (1, 4) (1, 9)
    OP         '+'           (1, 10) (1, 11)
    STRING     "'ABC'"       (1, 12) (1, 17)
    """)
        self.check_tokenize('y = "ABC" + "ABC"', """\
    NAME       'y'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '"ABC"'       (1, 4) (1, 9)
    OP         '+'           (1, 10) (1, 11)
    STRING     '"ABC"'       (1, 12) (1, 17)
    """)
        self.check_tokenize("x = r'abc' + r'ABC' + R'ABC' + R'ABC'", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "r'abc'"      (1, 4) (1, 10)
    OP         '+'           (1, 11) (1, 12)
    STRING     "r'ABC'"      (1, 13) (1, 19)
    OP         '+'           (1, 20) (1, 21)
    STRING     "R'ABC'"      (1, 22) (1, 28)
    OP         '+'           (1, 29) (1, 30)
    STRING     "R'ABC'"      (1, 31) (1, 37)
    """)
        self.check_tokenize('y = r"abc" + r"ABC" + R"ABC" + R"ABC"', """\
    NAME       'y'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     'r"abc"'      (1, 4) (1, 10)
    OP         '+'           (1, 11) (1, 12)
    STRING     'r"ABC"'      (1, 13) (1, 19)
    OP         '+'           (1, 20) (1, 21)
    STRING     'R"ABC"'      (1, 22) (1, 28)
    OP         '+'           (1, 29) (1, 30)
    STRING     'R"ABC"'      (1, 31) (1, 37)
    """)

        self.check_tokenize("u'abc' + U'abc'", """\
    STRING     "u'abc'"      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     "U'abc'"      (1, 9) (1, 15)
    """)
        self.check_tokenize('u"abc" + U"abc"', """\
    STRING     'u"abc"'      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     'U"abc"'      (1, 9) (1, 15)
    """)

        self.check_tokenize("b'abc' + B'abc'", """\
    STRING     "b'abc'"      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     "B'abc'"      (1, 9) (1, 15)
    """)
        self.check_tokenize('b"abc" + B"abc"', """\
    STRING     'b"abc"'      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     'B"abc"'      (1, 9) (1, 15)
    """)
        self.check_tokenize("br'abc' + bR'abc' + Br'abc' + BR'abc'", """\
    STRING     "br'abc'"     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     "bR'abc'"     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     "Br'abc'"     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     "BR'abc'"     (1, 30) (1, 37)
    """)
        self.check_tokenize('br"abc" + bR"abc" + Br"abc" + BR"abc"', """\
    STRING     'br"abc"'     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     'bR"abc"'     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     'Br"abc"'     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     'BR"abc"'     (1, 30) (1, 37)
    """)
        self.check_tokenize("rb'abc' + rB'abc' + Rb'abc' + RB'abc'", """\
    STRING     "rb'abc'"     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     "rB'abc'"     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     "Rb'abc'"     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     "RB'abc'"     (1, 30) (1, 37)
    """)
        self.check_tokenize('rb"abc" + rB"abc" + Rb"abc" + RB"abc"', """\
    STRING     'rb"abc"'     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     'rB"abc"'     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     'Rb"abc"'     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     'RB"abc"'     (1, 30) (1, 37)
    """)
        # Check 0, 1, and 2 character string prefixes.
        self.check_tokenize(r'"a\
de\
fg"', """\
    STRING     '"a\\\\\\nde\\\\\\nfg"\' (1, 0) (3, 3)
    """)
        self.check_tokenize(r'u"a\
de"', """\
    STRING     'u"a\\\\\\nde"\'  (1, 0) (2, 3)
    """)
        self.check_tokenize(r'rb"a\
d"', """\
    STRING     'rb"a\\\\\\nd"\'  (1, 0) (2, 2)
    """)
        self.check_tokenize(r'"""a\
b"""', """\
    STRING     '\"\""a\\\\\\nb\"\""' (1, 0) (2, 4)
    """)
        self.check_tokenize(r'u"""a\
b"""', """\
    STRING     'u\"\""a\\\\\\nb\"\""' (1, 0) (2, 4)
    """)
        self.check_tokenize(r'rb"""a\
b\
c"""', """\
    STRING     'rb"\""a\\\\\\nb\\\\\\nc"\""' (1, 0) (3, 4)
    """)
        self.check_tokenize('f"abc"', """\
    FSTRING_START 'f"'          (1, 0) (1, 2)
    FSTRING_MIDDLE 'abc'         (1, 2) (1, 5)
    FSTRING_END '"'           (1, 5) (1, 6)
    """)
        self.check_tokenize('fR"a{b}c"', """\
    FSTRING_START 'fR"'         (1, 0) (1, 3)
    FSTRING_MIDDLE 'a'           (1, 3) (1, 4)
    OP         '{'           (1, 4) (1, 5)
    NAME       'b'           (1, 5) (1, 6)
    OP         '}'           (1, 6) (1, 7)
    FSTRING_MIDDLE 'c'           (1, 7) (1, 8)
    FSTRING_END '"'           (1, 8) (1, 9)
    """)
        self.check_tokenize('fR"a{{{b!r}}}c"', """\
    FSTRING_START 'fR"'         (1, 0) (1, 3)
    FSTRING_MIDDLE 'a{'          (1, 3) (1, 5)
    OP         '{'           (1, 6) (1, 7)
    NAME       'b'           (1, 7) (1, 8)
    OP         '!'           (1, 8) (1, 9)
    NAME       'r'           (1, 9) (1, 10)
    OP         '}'           (1, 10) (1, 11)
    FSTRING_MIDDLE '}'           (1, 11) (1, 12)
    FSTRING_MIDDLE 'c'           (1, 13) (1, 14)
    FSTRING_END '"'           (1, 14) (1, 15)
    """)
        self.check_tokenize('f"{{{1+1}}}"', """\
    FSTRING_START 'f"'          (1, 0) (1, 2)
    FSTRING_MIDDLE '{'           (1, 2) (1, 3)
    OP         '{'           (1, 4) (1, 5)
    NUMBER     '1'           (1, 5) (1, 6)
    OP         '+'           (1, 6) (1, 7)
    NUMBER     '1'           (1, 7) (1, 8)
    OP         '}'           (1, 8) (1, 9)
    FSTRING_MIDDLE '}'           (1, 9) (1, 10)
    FSTRING_END '"'           (1, 11) (1, 12)
    """)
        self.check_tokenize('f"""{f\'\'\'{f\'{f"{1+1}"}\'}\'\'\'}"""', """\
    FSTRING_START 'f\"""'        (1, 0) (1, 4)
    OP         '{'           (1, 4) (1, 5)
    FSTRING_START "f'''"        (1, 5) (1, 9)
    OP         '{'           (1, 9) (1, 10)
    FSTRING_START "f'"          (1, 10) (1, 12)
    OP         '{'           (1, 12) (1, 13)
    FSTRING_START 'f"'          (1, 13) (1, 15)
    OP         '{'           (1, 15) (1, 16)
    NUMBER     '1'           (1, 16) (1, 17)
    OP         '+'           (1, 17) (1, 18)
    NUMBER     '1'           (1, 18) (1, 19)
    OP         '}'           (1, 19) (1, 20)
    FSTRING_END '"'           (1, 20) (1, 21)
    OP         '}'           (1, 21) (1, 22)
    FSTRING_END "'"           (1, 22) (1, 23)
    OP         '}'           (1, 23) (1, 24)
    FSTRING_END "'''"         (1, 24) (1, 27)
    OP         '}'           (1, 27) (1, 28)
    FSTRING_END '\"""'         (1, 28) (1, 31)
    """)
        self.check_tokenize('f"""     x\nstr(data, encoding={invalid!r})\n"""', """\
    FSTRING_START 'f\"""'        (1, 0) (1, 4)
    FSTRING_MIDDLE '     x\\nstr(data, encoding=' (1, 4) (2, 19)
    OP         '{'           (2, 19) (2, 20)
    NAME       'invalid'     (2, 20) (2, 27)
    OP         '!'           (2, 27) (2, 28)
    NAME       'r'           (2, 28) (2, 29)
    OP         '}'           (2, 29) (2, 30)
    FSTRING_MIDDLE ')\\n'         (2, 30) (3, 0)
    FSTRING_END '\"""'         (3, 0) (3, 3)
    """)
        self.check_tokenize('f"""123456789\nsomething{None}bad"""', """\
    FSTRING_START 'f\"""'        (1, 0) (1, 4)
    FSTRING_MIDDLE '123456789\\nsomething' (1, 4) (2, 9)
    OP         '{'           (2, 9) (2, 10)
    NAME       'None'        (2, 10) (2, 14)
    OP         '}'           (2, 14) (2, 15)
    FSTRING_MIDDLE 'bad'         (2, 15) (2, 18)
    FSTRING_END '\"""'         (2, 18) (2, 21)
    """)
        self.check_tokenize('f"""abc"""', """\
    FSTRING_START 'f\"""'        (1, 0) (1, 4)
    FSTRING_MIDDLE 'abc'         (1, 4) (1, 7)
    FSTRING_END '\"""'         (1, 7) (1, 10)
    """)
        self.check_tokenize(r'f"abc\
def"', """\
    FSTRING_START 'f"'          (1, 0) (1, 2)
    FSTRING_MIDDLE 'abc\\\\\\ndef'  (1, 2) (2, 3)
    FSTRING_END '"'           (2, 3) (2, 4)
    """)
        self.check_tokenize(r'Rf"abc\
def"', """\
    FSTRING_START 'Rf"'         (1, 0) (1, 3)
    FSTRING_MIDDLE 'abc\\\\\\ndef'  (1, 3) (2, 3)
    FSTRING_END '"'           (2, 3) (2, 4)
    """)
        self.check_tokenize("f'some words {a+b:.3f} more words {c+d=} final words'", """\
    FSTRING_START "f'"          (1, 0) (1, 2)
    FSTRING_MIDDLE 'some words ' (1, 2) (1, 13)
    OP         '{'           (1, 13) (1, 14)
    NAME       'a'           (1, 14) (1, 15)
    OP         '+'           (1, 15) (1, 16)
    NAME       'b'           (1, 16) (1, 17)
    OP         ':'           (1, 17) (1, 18)
    FSTRING_MIDDLE '.3f'         (1, 18) (1, 21)
    OP         '}'           (1, 21) (1, 22)
    FSTRING_MIDDLE ' more words ' (1, 22) (1, 34)
    OP         '{'           (1, 34) (1, 35)
    NAME       'c'           (1, 35) (1, 36)
    OP         '+'           (1, 36) (1, 37)
    NAME       'd'           (1, 37) (1, 38)
    OP         '='           (1, 38) (1, 39)
    OP         '}'           (1, 39) (1, 40)
    FSTRING_MIDDLE ' final words' (1, 40) (1, 52)
    FSTRING_END "'"           (1, 52) (1, 53)
    """)
        self.check_tokenize("""\
f'''{
3
=}'''""", """\
    FSTRING_START "f'''"        (1, 0) (1, 4)
    OP         '{'           (1, 4) (1, 5)
    NL         '\\n'          (1, 5) (1, 6)
    NUMBER     '3'           (2, 0) (2, 1)
    NL         '\\n'          (2, 1) (2, 2)
    OP         '='           (3, 0) (3, 1)
    OP         '}'           (3, 1) (3, 2)
    FSTRING_END "'''"         (3, 2) (3, 5)
    """)
        self.check_tokenize("""\
f'''__{
    x:a
}__'''""", """\
    FSTRING_START "f'''"        (1, 0) (1, 4)
    FSTRING_MIDDLE '__'          (1, 4) (1, 6)
    OP         '{'           (1, 6) (1, 7)
    NL         '\\n'          (1, 7) (1, 8)
    NAME       'x'           (2, 4) (2, 5)
    OP         ':'           (2, 5) (2, 6)
    FSTRING_MIDDLE 'a\\n'         (2, 6) (3, 0)
    OP         '}'           (3, 0) (3, 1)
    FSTRING_MIDDLE '__'          (3, 1) (3, 3)
    FSTRING_END "'''"         (3, 3) (3, 6)
    """)
        self.check_tokenize("""\
f'''__{
    x:a
    b
     c
      d
}__'''""", """\
    FSTRING_START "f'''"        (1, 0) (1, 4)
    FSTRING_MIDDLE '__'          (1, 4) (1, 6)
    OP         '{'           (1, 6) (1, 7)
    NL         '\\n'          (1, 7) (1, 8)
    NAME       'x'           (2, 4) (2, 5)
    OP         ':'           (2, 5) (2, 6)
    FSTRING_MIDDLE 'a\\n    b\\n     c\\n      d\\n' (2, 6) (6, 0)
    OP         '}'           (6, 0) (6, 1)
    FSTRING_MIDDLE '__'          (6, 1) (6, 3)
    FSTRING_END "'''"         (6, 3) (6, 6)
    """)
        self.check_tokenize("""\
f'__{
    x:d
}__'""", """\
    FSTRING_START "f'"          (1, 0) (1, 2)
    FSTRING_MIDDLE '__'          (1, 2) (1, 4)
    OP         '{'           (1, 4) (1, 5)
    NL         '\\n'          (1, 5) (1, 6)
    NAME       'x'           (2, 4) (2, 5)
    OP         ':'           (2, 5) (2, 6)
    FSTRING_MIDDLE 'd'           (2, 6) (2, 7)
    NL         '\\n'          (2, 7) (2, 8)
    OP         '}'           (3, 0) (3, 1)
    FSTRING_MIDDLE '__'          (3, 1) (3, 3)
    FSTRING_END "'"           (3, 3) (3, 4)
    """)

        self.check_tokenize("""\
    '''Autorzy, którzy tą jednostkę mają wpisani jako AKTUALNA -- czyli
    aktualni pracownicy, obecni pracownicy'''
""", """\
    INDENT     '    '        (1, 0) (1, 4)
    STRING     "'''Autorzy, którzy tą jednostkę mają wpisani jako AKTUALNA -- czyli\\n    aktualni pracownicy, obecni pracownicy'''" (1, 4) (2, 45)
    NEWLINE    '\\n'          (2, 45) (2, 46)
    DEDENT     ''            (3, 0) (3, 0)
    """)

    def test_function(self):
        self.check_tokenize("def d22(a, b, c=2, d=2, *k): pass", """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'd22'         (1, 4) (1, 7)
    OP         '('           (1, 7) (1, 8)
    NAME       'a'           (1, 8) (1, 9)
    OP         ','           (1, 9) (1, 10)
    NAME       'b'           (1, 11) (1, 12)
    OP         ','           (1, 12) (1, 13)
    NAME       'c'           (1, 14) (1, 15)
    OP         '='           (1, 15) (1, 16)
    NUMBER     '2'           (1, 16) (1, 17)
    OP         ','           (1, 17) (1, 18)
    NAME       'd'           (1, 19) (1, 20)
    OP         '='           (1, 20) (1, 21)
    NUMBER     '2'           (1, 21) (1, 22)
    OP         ','           (1, 22) (1, 23)
    OP         '*'           (1, 24) (1, 25)
    NAME       'k'           (1, 25) (1, 26)
    OP         ')'           (1, 26) (1, 27)
    OP         ':'           (1, 27) (1, 28)
    NAME       'pass'        (1, 29) (1, 33)
    """)
        self.check_tokenize("def d01v_(a=1, *k, **w): pass", """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'd01v_'       (1, 4) (1, 9)
    OP         '('           (1, 9) (1, 10)
    NAME       'a'           (1, 10) (1, 11)
    OP         '='           (1, 11) (1, 12)
    NUMBER     '1'           (1, 12) (1, 13)
    OP         ','           (1, 13) (1, 14)
    OP         '*'           (1, 15) (1, 16)
    NAME       'k'           (1, 16) (1, 17)
    OP         ','           (1, 17) (1, 18)
    OP         '**'          (1, 19) (1, 21)
    NAME       'w'           (1, 21) (1, 22)
    OP         ')'           (1, 22) (1, 23)
    OP         ':'           (1, 23) (1, 24)
    NAME       'pass'        (1, 25) (1, 29)
    """)
        self.check_tokenize("def d23(a: str, b: int=3) -> int: pass", """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'd23'         (1, 4) (1, 7)
    OP         '('           (1, 7) (1, 8)
    NAME       'a'           (1, 8) (1, 9)
    OP         ':'           (1, 9) (1, 10)
    NAME       'str'         (1, 11) (1, 14)
    OP         ','           (1, 14) (1, 15)
    NAME       'b'           (1, 16) (1, 17)
    OP         ':'           (1, 17) (1, 18)
    NAME       'int'         (1, 19) (1, 22)
    OP         '='           (1, 22) (1, 23)
    NUMBER     '3'           (1, 23) (1, 24)
    OP         ')'           (1, 24) (1, 25)
    OP         '->'          (1, 26) (1, 28)
    NAME       'int'         (1, 29) (1, 32)
    OP         ':'           (1, 32) (1, 33)
    NAME       'pass'        (1, 34) (1, 38)
    """)

    def test_comparison(self):
        # Comparison
        self.check_tokenize("if 1 < 1 > 1 == 1 >= 5 <= 0x15 <= 0x12 != "
                            "1 and 5 in 1 not in 1 is 1 or 5 is not 1: pass", """\
    NAME       'if'          (1, 0) (1, 2)
    NUMBER     '1'           (1, 3) (1, 4)
    OP         '<'           (1, 5) (1, 6)
    NUMBER     '1'           (1, 7) (1, 8)
    OP         '>'           (1, 9) (1, 10)
    NUMBER     '1'           (1, 11) (1, 12)
    OP         '=='          (1, 13) (1, 15)
    NUMBER     '1'           (1, 16) (1, 17)
    OP         '>='          (1, 18) (1, 20)
    NUMBER     '5'           (1, 21) (1, 22)
    OP         '<='          (1, 23) (1, 25)
    NUMBER     '0x15'        (1, 26) (1, 30)
    OP         '<='          (1, 31) (1, 33)
    NUMBER     '0x12'        (1, 34) (1, 38)
    OP         '!='          (1, 39) (1, 41)
    NUMBER     '1'           (1, 42) (1, 43)
    NAME       'and'         (1, 44) (1, 47)
    NUMBER     '5'           (1, 48) (1, 49)
    NAME       'in'          (1, 50) (1, 52)
    NUMBER     '1'           (1, 53) (1, 54)
    NAME       'not'         (1, 55) (1, 58)
    NAME       'in'          (1, 59) (1, 61)
    NUMBER     '1'           (1, 62) (1, 63)
    NAME       'is'          (1, 64) (1, 66)
    NUMBER     '1'           (1, 67) (1, 68)
    NAME       'or'          (1, 69) (1, 71)
    NUMBER     '5'           (1, 72) (1, 73)
    NAME       'is'          (1, 74) (1, 76)
    NAME       'not'         (1, 77) (1, 80)
    NUMBER     '1'           (1, 81) (1, 82)
    OP         ':'           (1, 82) (1, 83)
    NAME       'pass'        (1, 84) (1, 88)
    """)

    def test_shift(self):
        # Shift
        self.check_tokenize("x = 1 << 1 >> 5", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    OP         '<<'          (1, 6) (1, 8)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         '>>'          (1, 11) (1, 13)
    NUMBER     '5'           (1, 14) (1, 15)
    """)

    def test_additive(self):
        # Additive
        self.check_tokenize("x = 1 - y + 15 - 1 + 0x124 + z + a[5]", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    OP         '-'           (1, 6) (1, 7)
    NAME       'y'           (1, 8) (1, 9)
    OP         '+'           (1, 10) (1, 11)
    NUMBER     '15'          (1, 12) (1, 14)
    OP         '-'           (1, 15) (1, 16)
    NUMBER     '1'           (1, 17) (1, 18)
    OP         '+'           (1, 19) (1, 20)
    NUMBER     '0x124'       (1, 21) (1, 26)
    OP         '+'           (1, 27) (1, 28)
    NAME       'z'           (1, 29) (1, 30)
    OP         '+'           (1, 31) (1, 32)
    NAME       'a'           (1, 33) (1, 34)
    OP         '['           (1, 34) (1, 35)
    NUMBER     '5'           (1, 35) (1, 36)
    OP         ']'           (1, 36) (1, 37)
    """)

    def test_multiplicative(self):
        # Multiplicative
        self.check_tokenize("x = 1//1*1/5*12%0x12@42", """\
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    OP         '//'          (1, 5) (1, 7)
    NUMBER     '1'           (1, 7) (1, 8)
    OP         '*'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         '/'           (1, 10) (1, 11)
    NUMBER     '5'           (1, 11) (1, 12)
    OP         '*'           (1, 12) (1, 13)
    NUMBER     '12'          (1, 13) (1, 15)
    OP         '%'           (1, 15) (1, 16)
    NUMBER     '0x12'        (1, 16) (1, 20)
    OP         '@'           (1, 20) (1, 21)
    NUMBER     '42'          (1, 21) (1, 23)
    """)

    def test_unary(self):
        # Unary
        self.check_tokenize("~1 ^ 1 & 1 |1 ^ -1", """\
    OP         '~'           (1, 0) (1, 1)
    NUMBER     '1'           (1, 1) (1, 2)
    OP         '^'           (1, 3) (1, 4)
    NUMBER     '1'           (1, 5) (1, 6)
    OP         '&'           (1, 7) (1, 8)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         '|'           (1, 11) (1, 12)
    NUMBER     '1'           (1, 12) (1, 13)
    OP         '^'           (1, 14) (1, 15)
    OP         '-'           (1, 16) (1, 17)
    NUMBER     '1'           (1, 17) (1, 18)
    """)
        self.check_tokenize("-1*1/1+1*1//1 - ---1**1", """\
    OP         '-'           (1, 0) (1, 1)
    NUMBER     '1'           (1, 1) (1, 2)
    OP         '*'           (1, 2) (1, 3)
    NUMBER     '1'           (1, 3) (1, 4)
    OP         '/'           (1, 4) (1, 5)
    NUMBER     '1'           (1, 5) (1, 6)
    OP         '+'           (1, 6) (1, 7)
    NUMBER     '1'           (1, 7) (1, 8)
    OP         '*'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         '//'          (1, 10) (1, 12)
    NUMBER     '1'           (1, 12) (1, 13)
    OP         '-'           (1, 14) (1, 15)
    OP         '-'           (1, 16) (1, 17)
    OP         '-'           (1, 17) (1, 18)
    OP         '-'           (1, 18) (1, 19)
    NUMBER     '1'           (1, 19) (1, 20)
    OP         '**'          (1, 20) (1, 22)
    NUMBER     '1'           (1, 22) (1, 23)
    """)

    def test_selector(self):
        # Selector
        self.check_tokenize("import sys, time\nx = sys.modules['time'].time()", """\
    NAME       'import'      (1, 0) (1, 6)
    NAME       'sys'         (1, 7) (1, 10)
    OP         ','           (1, 10) (1, 11)
    NAME       'time'        (1, 12) (1, 16)
    NEWLINE    '\\n'          (1, 16) (1, 17)
    NAME       'x'           (2, 0) (2, 1)
    OP         '='           (2, 2) (2, 3)
    NAME       'sys'         (2, 4) (2, 7)
    OP         '.'           (2, 7) (2, 8)
    NAME       'modules'     (2, 8) (2, 15)
    OP         '['           (2, 15) (2, 16)
    STRING     "'time'"      (2, 16) (2, 22)
    OP         ']'           (2, 22) (2, 23)
    OP         '.'           (2, 23) (2, 24)
    NAME       'time'        (2, 24) (2, 28)
    OP         '('           (2, 28) (2, 29)
    OP         ')'           (2, 29) (2, 30)
    """)

    def test_method(self):
        # Methods
        self.check_tokenize("@staticmethod\ndef foo(x,y): pass", """\
    OP         '@'           (1, 0) (1, 1)
    NAME       'staticmethod' (1, 1) (1, 13)
    NEWLINE    '\\n'          (1, 13) (1, 14)
    NAME       'def'         (2, 0) (2, 3)
    NAME       'foo'         (2, 4) (2, 7)
    OP         '('           (2, 7) (2, 8)
    NAME       'x'           (2, 8) (2, 9)
    OP         ','           (2, 9) (2, 10)
    NAME       'y'           (2, 10) (2, 11)
    OP         ')'           (2, 11) (2, 12)
    OP         ':'           (2, 12) (2, 13)
    NAME       'pass'        (2, 14) (2, 18)
    """)

    def test_tabs(self):
        # Evil tabs
        self.check_tokenize("def f():\n"
                            "\tif x\n"
                            "        \tpass", """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'f'           (1, 4) (1, 5)
    OP         '('           (1, 5) (1, 6)
    OP         ')'           (1, 6) (1, 7)
    OP         ':'           (1, 7) (1, 8)
    NEWLINE    '\\n'          (1, 8) (1, 9)
    INDENT     '\\t'          (2, 0) (2, 1)
    NAME       'if'          (2, 1) (2, 3)
    NAME       'x'           (2, 4) (2, 5)
    NEWLINE    '\\n'          (2, 5) (2, 6)
    INDENT     '        \\t'  (3, 0) (3, 9)
    NAME       'pass'        (3, 9) (3, 13)
    DEDENT     ''            (4, 0) (4, 0)
    DEDENT     ''            (4, 0) (4, 0)
    """)

    def test_non_ascii_identifiers(self):
        # Non-ascii identifiers
        self.check_tokenize("Örter = 'places'\ngrün = 'green'", """\
    NAME       'Örter'       (1, 0) (1, 5)
    OP         '='           (1, 6) (1, 7)
    STRING     "'places'"    (1, 8) (1, 16)
    NEWLINE    '\\n'          (1, 16) (1, 17)
    NAME       'grün'        (2, 0) (2, 4)
    OP         '='           (2, 5) (2, 6)
    STRING     "'green'"     (2, 7) (2, 14)
    """)

    def test_unicode(self):
        # Legacy unicode literals:
        self.check_tokenize("Örter = u'places'\ngrün = U'green'", """\
    NAME       'Örter'       (1, 0) (1, 5)
    OP         '='           (1, 6) (1, 7)
    STRING     "u'places'"   (1, 8) (1, 17)
    NEWLINE    '\\n'          (1, 17) (1, 18)
    NAME       'grün'        (2, 0) (2, 4)
    OP         '='           (2, 5) (2, 6)
    STRING     "U'green'"    (2, 7) (2, 15)
    """)

    def test_async(self):
        # Async/await extension:
        self.check_tokenize("async = 1", """\
    NAME       'async'       (1, 0) (1, 5)
    OP         '='           (1, 6) (1, 7)
    NUMBER     '1'           (1, 8) (1, 9)
    """)

        self.check_tokenize("a = (async = 1)", """\
    NAME       'a'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    OP         '('           (1, 4) (1, 5)
    NAME       'async'       (1, 5) (1, 10)
    OP         '='           (1, 11) (1, 12)
    NUMBER     '1'           (1, 13) (1, 14)
    OP         ')'           (1, 14) (1, 15)
    """)

        self.check_tokenize("async()", """\
    NAME       'async'       (1, 0) (1, 5)
    OP         '('           (1, 5) (1, 6)
    OP         ')'           (1, 6) (1, 7)
    """)

        self.check_tokenize("class async(Bar):pass", """\
    NAME       'class'       (1, 0) (1, 5)
    NAME       'async'       (1, 6) (1, 11)
    OP         '('           (1, 11) (1, 12)
    NAME       'Bar'         (1, 12) (1, 15)
    OP         ')'           (1, 15) (1, 16)
    OP         ':'           (1, 16) (1, 17)
    NAME       'pass'        (1, 17) (1, 21)
    """)

        self.check_tokenize("class async:pass", """\
    NAME       'class'       (1, 0) (1, 5)
    NAME       'async'       (1, 6) (1, 11)
    OP         ':'           (1, 11) (1, 12)
    NAME       'pass'        (1, 12) (1, 16)
    """)

        self.check_tokenize("await = 1", """\
    NAME       'await'       (1, 0) (1, 5)
    OP         '='           (1, 6) (1, 7)
    NUMBER     '1'           (1, 8) (1, 9)
    """)

        self.check_tokenize("foo.async", """\
    NAME       'foo'         (1, 0) (1, 3)
    OP         '.'           (1, 3) (1, 4)
    NAME       'async'       (1, 4) (1, 9)
    """)

        self.check_tokenize("async for a in b: pass", """\
    NAME       'async'       (1, 0) (1, 5)
    NAME       'for'         (1, 6) (1, 9)
    NAME       'a'           (1, 10) (1, 11)
    NAME       'in'          (1, 12) (1, 14)
    NAME       'b'           (1, 15) (1, 16)
    OP         ':'           (1, 16) (1, 17)
    NAME       'pass'        (1, 18) (1, 22)
    """)

        self.check_tokenize("async with a as b: pass", """\
    NAME       'async'       (1, 0) (1, 5)
    NAME       'with'        (1, 6) (1, 10)
    NAME       'a'           (1, 11) (1, 12)
    NAME       'as'          (1, 13) (1, 15)
    NAME       'b'           (1, 16) (1, 17)
    OP         ':'           (1, 17) (1, 18)
    NAME       'pass'        (1, 19) (1, 23)
    """)

        self.check_tokenize("async.foo", """\
    NAME       'async'       (1, 0) (1, 5)
    OP         '.'           (1, 5) (1, 6)
    NAME       'foo'         (1, 6) (1, 9)
    """)

        self.check_tokenize("async", """\
    NAME       'async'       (1, 0) (1, 5)
    """)

        self.check_tokenize("async\n#comment\nawait", """\
    NAME       'async'       (1, 0) (1, 5)
    NEWLINE    '\\n'          (1, 5) (1, 6)
    COMMENT    '#comment'    (2, 0) (2, 8)
    NL         '\\n'          (2, 8) (2, 9)
    NAME       'await'       (3, 0) (3, 5)
    """)

        self.check_tokenize("async\n...\nawait", """\
    NAME       'async'       (1, 0) (1, 5)
    NEWLINE    '\\n'          (1, 5) (1, 6)
    OP         '...'         (2, 0) (2, 3)
    NEWLINE    '\\n'          (2, 3) (2, 4)
    NAME       'await'       (3, 0) (3, 5)
    """)

        self.check_tokenize("async\nawait", """\
    NAME       'async'       (1, 0) (1, 5)
    NEWLINE    '\\n'          (1, 5) (1, 6)
    NAME       'await'       (2, 0) (2, 5)
    """)

        self.check_tokenize("foo.async + 1", """\
    NAME       'foo'         (1, 0) (1, 3)
    OP         '.'           (1, 3) (1, 4)
    NAME       'async'       (1, 4) (1, 9)
    OP         '+'           (1, 10) (1, 11)
    NUMBER     '1'           (1, 12) (1, 13)
    """)

        self.check_tokenize("async def foo(): pass", """\
    NAME       'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    OP         '('           (1, 13) (1, 14)
    OP         ')'           (1, 14) (1, 15)
    OP         ':'           (1, 15) (1, 16)
    NAME       'pass'        (1, 17) (1, 21)
    """)

        self.check_tokenize('''\
async def foo():
  def foo(await):
    await = 1
  if 1:
    await
async += 1
''', """\
    NAME       'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    OP         '('           (1, 13) (1, 14)
    OP         ')'           (1, 14) (1, 15)
    OP         ':'           (1, 15) (1, 16)
    NEWLINE    '\\n'          (1, 16) (1, 17)
    INDENT     '  '          (2, 0) (2, 2)
    NAME       'def'         (2, 2) (2, 5)
    NAME       'foo'         (2, 6) (2, 9)
    OP         '('           (2, 9) (2, 10)
    NAME       'await'       (2, 10) (2, 15)
    OP         ')'           (2, 15) (2, 16)
    OP         ':'           (2, 16) (2, 17)
    NEWLINE    '\\n'          (2, 17) (2, 18)
    INDENT     '    '        (3, 0) (3, 4)
    NAME       'await'       (3, 4) (3, 9)
    OP         '='           (3, 10) (3, 11)
    NUMBER     '1'           (3, 12) (3, 13)
    NEWLINE    '\\n'          (3, 13) (3, 14)
    DEDENT     ''            (4, 2) (4, 2)
    NAME       'if'          (4, 2) (4, 4)
    NUMBER     '1'           (4, 5) (4, 6)
    OP         ':'           (4, 6) (4, 7)
    NEWLINE    '\\n'          (4, 7) (4, 8)
    INDENT     '    '        (5, 0) (5, 4)
    NAME       'await'       (5, 4) (5, 9)
    NEWLINE    '\\n'          (5, 9) (5, 10)
    DEDENT     ''            (6, 0) (6, 0)
    DEDENT     ''            (6, 0) (6, 0)
    NAME       'async'       (6, 0) (6, 5)
    OP         '+='          (6, 6) (6, 8)
    NUMBER     '1'           (6, 9) (6, 10)
    NEWLINE    '\\n'          (6, 10) (6, 11)
    """)

        self.check_tokenize('''\
async def foo():
  async for i in 1: pass''', """\
    NAME       'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    OP         '('           (1, 13) (1, 14)
    OP         ')'           (1, 14) (1, 15)
    OP         ':'           (1, 15) (1, 16)
    NEWLINE    '\\n'          (1, 16) (1, 17)
    INDENT     '  '          (2, 0) (2, 2)
    NAME       'async'       (2, 2) (2, 7)
    NAME       'for'         (2, 8) (2, 11)
    NAME       'i'           (2, 12) (2, 13)
    NAME       'in'          (2, 14) (2, 16)
    NUMBER     '1'           (2, 17) (2, 18)
    OP         ':'           (2, 18) (2, 19)
    NAME       'pass'        (2, 20) (2, 24)
    DEDENT     ''            (3, 0) (3, 0)
    """)

        self.check_tokenize('''async def foo(async): await''', """\
    NAME       'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    OP         '('           (1, 13) (1, 14)
    NAME       'async'       (1, 14) (1, 19)
    OP         ')'           (1, 19) (1, 20)
    OP         ':'           (1, 20) (1, 21)
    NAME       'await'       (1, 22) (1, 27)
    """)

        self.check_tokenize('''\
def f():

  def baz(): pass
  async def bar(): pass

  await = 2''', """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'f'           (1, 4) (1, 5)
    OP         '('           (1, 5) (1, 6)
    OP         ')'           (1, 6) (1, 7)
    OP         ':'           (1, 7) (1, 8)
    NEWLINE    '\\n'          (1, 8) (1, 9)
    NL         '\\n'          (2, 0) (2, 1)
    INDENT     '  '          (3, 0) (3, 2)
    NAME       'def'         (3, 2) (3, 5)
    NAME       'baz'         (3, 6) (3, 9)
    OP         '('           (3, 9) (3, 10)
    OP         ')'           (3, 10) (3, 11)
    OP         ':'           (3, 11) (3, 12)
    NAME       'pass'        (3, 13) (3, 17)
    NEWLINE    '\\n'          (3, 17) (3, 18)
    NAME       'async'       (4, 2) (4, 7)
    NAME       'def'         (4, 8) (4, 11)
    NAME       'bar'         (4, 12) (4, 15)
    OP         '('           (4, 15) (4, 16)
    OP         ')'           (4, 16) (4, 17)
    OP         ':'           (4, 17) (4, 18)
    NAME       'pass'        (4, 19) (4, 23)
    NEWLINE    '\\n'          (4, 23) (4, 24)
    NL         '\\n'          (5, 0) (5, 1)
    NAME       'await'       (6, 2) (6, 7)
    OP         '='           (6, 8) (6, 9)
    NUMBER     '2'           (6, 10) (6, 11)
    DEDENT     ''            (7, 0) (7, 0)
    """)

        self.check_tokenize('''\
async def f():

  def baz(): pass
  async def bar(): pass

  await = 2''', """\
    NAME       'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'f'           (1, 10) (1, 11)
    OP         '('           (1, 11) (1, 12)
    OP         ')'           (1, 12) (1, 13)
    OP         ':'           (1, 13) (1, 14)
    NEWLINE    '\\n'          (1, 14) (1, 15)
    NL         '\\n'          (2, 0) (2, 1)
    INDENT     '  '          (3, 0) (3, 2)
    NAME       'def'         (3, 2) (3, 5)
    NAME       'baz'         (3, 6) (3, 9)
    OP         '('           (3, 9) (3, 10)
    OP         ')'           (3, 10) (3, 11)
    OP         ':'           (3, 11) (3, 12)
    NAME       'pass'        (3, 13) (3, 17)
    NEWLINE    '\\n'          (3, 17) (3, 18)
    NAME       'async'       (4, 2) (4, 7)
    NAME       'def'         (4, 8) (4, 11)
    NAME       'bar'         (4, 12) (4, 15)
    OP         '('           (4, 15) (4, 16)
    OP         ')'           (4, 16) (4, 17)
    OP         ':'           (4, 17) (4, 18)
    NAME       'pass'        (4, 19) (4, 23)
    NEWLINE    '\\n'          (4, 23) (4, 24)
    NL         '\\n'          (5, 0) (5, 1)
    NAME       'await'       (6, 2) (6, 7)
    OP         '='           (6, 8) (6, 9)
    NUMBER     '2'           (6, 10) (6, 11)
    DEDENT     ''            (7, 0) (7, 0)
    """)

    def test_newline_after_parenthesized_block_with_comment(self):
        self.check_tokenize('''\
[
    # A comment here
    1
]
''', """\
    OP         '['           (1, 0) (1, 1)
    NL         '\\n'          (1, 1) (1, 2)
    COMMENT    '# A comment here' (2, 4) (2, 20)
    NL         '\\n'          (2, 20) (2, 21)
    NUMBER     '1'           (3, 4) (3, 5)
    NL         '\\n'          (3, 5) (3, 6)
    OP         ']'           (4, 0) (4, 1)
    NEWLINE    '\\n'          (4, 1) (4, 2)
    """)

    def test_closing_parenthesis_from_different_line(self):
        self.check_tokenize("); x", """\
    OP         ')'           (1, 0) (1, 1)
    OP         ';'           (1, 1) (1, 2)
    NAME       'x'           (1, 3) (1, 4)
    """)

class GenerateTokensTest(TokenizeTest):
    def check_tokenize(self, s, expected):
        # Format the tokens in s in a table format.
        # The ENDMARKER and final NEWLINE are omitted.
        f = StringIO(s)
        result = stringify_tokens_from_source(generate_tokens(f.readline), s)
        self.assertEqual(result, expected.rstrip().splitlines())


def decistmt(s):
    result = []
    g = tokenize(BytesIO(s.encode('utf-8')).readline)   # tokenize the string
    for toknum, tokval, _, _, _  in g:
        if toknum == NUMBER and '.' in tokval:  # replace NUMBER tokens
            result.extend([
                (NAME, 'Decimal'),
                (OP, '('),
                (STRING, repr(tokval)),
                (OP, ')')
            ])
        else:
            result.append((toknum, tokval))
    return untokenize(result).decode('utf-8').strip()

class TestMisc(TestCase):

    def test_decistmt(self):
        # Substitute Decimals for floats in a string of statements.
        # This is an example from the docs.

        from decimal import Decimal
        s = '+21.3e-5*-.1234/81.7'
        self.assertEqual(decistmt(s),
                         "+Decimal ('21.3e-5')*-Decimal ('.1234')/Decimal ('81.7')")

        # The format of the exponent is inherited from the platform C library.
        # Known cases are "e-007" (Windows) and "e-07" (not Windows).  Since
        # we're only showing 11 digits, and the 12th isn't close to 5, the
        # rest of the output should be platform-independent.
        self.assertRegex(repr(eval(s)), '-3.2171603427[0-9]*e-0+7')

        # Output from calculations with Decimal should be identical across all
        # platforms.
        self.assertEqual(eval(decistmt(s)),
                         Decimal('-3.217160342717258261933904529E-7'))


class TestTokenizerAdheresToPep0263(TestCase):
    """
    Test that tokenizer adheres to the coding behaviour stipulated in PEP 0263.
    """

    def _testFile(self, filename):
        path = os.path.join(os.path.dirname(__file__), 'tokenizedata', filename)
        with open(path, 'rb') as f:
            TestRoundtrip.check_roundtrip(self, f)

    def test_utf8_coding_cookie_and_no_utf8_bom(self):
        f = 'tokenize_tests-utf8-coding-cookie-and-no-utf8-bom-sig.txt'
        self._testFile(f)

    def test_latin1_coding_cookie_and_utf8_bom(self):
        """
        As per PEP 0263, if a file starts with a utf-8 BOM signature, the only
        allowed encoding for the comment is 'utf-8'.  The text file used in
        this test starts with a BOM signature, but specifies latin1 as the
        coding, so verify that a SyntaxError is raised, which matches the
        behaviour of the interpreter when it encounters a similar condition.
        """
        f = 'tokenize_tests-latin1-coding-cookie-and-utf8-bom-sig.txt'
        self.assertRaises(SyntaxError, self._testFile, f)

    def test_no_coding_cookie_and_utf8_bom(self):
        f = 'tokenize_tests-no-coding-cookie-and-utf8-bom-sig-only.txt'
        self._testFile(f)

    def test_utf8_coding_cookie_and_utf8_bom(self):
        f = 'tokenize_tests-utf8-coding-cookie-and-utf8-bom-sig.txt'
        self._testFile(f)

    def test_bad_coding_cookie(self):
        self.assertRaises(SyntaxError, self._testFile, 'bad_coding.py')
        self.assertRaises(SyntaxError, self._testFile, 'bad_coding2.py')


class Test_Tokenize(TestCase):

    def test__tokenize_decodes_with_specified_encoding(self):
        literal = '"ЉЊЈЁЂ"'
        line = literal.encode('utf-8')
        first = False
        def readline():
            nonlocal first
            if not first:
                first = True
                yield line
            else:
                yield b''

        # skip the initial encoding token and the end tokens
        tokens = list(_generate_tokens_from_c_tokenizer(readline().__next__, encoding='utf-8',
                      extra_tokens=True))[:-2]
        expected_tokens = [TokenInfo(3, '"ЉЊЈЁЂ"', (1, 0), (1, 7), '"ЉЊЈЁЂ"')]
        self.assertEqual(tokens, expected_tokens,
                         "bytes not decoded with encoding")


class TestDetectEncoding(TestCase):

    def get_readline(self, lines):
        index = 0
        def readline():
            nonlocal index
            if index == len(lines):
                raise StopIteration
            line = lines[index]
            index += 1
            return line
        return readline

    def test_no_bom_no_encoding_cookie(self):
        lines = (
            b'# something\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'utf-8')
        self.assertEqual(consumed_lines, list(lines[:2]))

    def test_bom_no_cookie(self):
        lines = (
            b'\xef\xbb\xbf# something\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'utf-8-sig')
        self.assertEqual(consumed_lines,
                         [b'# something\n', b'print(something)\n'])

    def test_cookie_first_line_no_bom(self):
        lines = (
            b'# -*- coding: latin-1 -*-\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'iso-8859-1')
        self.assertEqual(consumed_lines, [b'# -*- coding: latin-1 -*-\n'])

    def test_matched_bom_and_cookie_first_line(self):
        lines = (
            b'\xef\xbb\xbf# coding=utf-8\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'utf-8-sig')
        self.assertEqual(consumed_lines, [b'# coding=utf-8\n'])

    def test_mismatched_bom_and_cookie_first_line_raises_syntaxerror(self):
        lines = (
            b'\xef\xbb\xbf# vim: set fileencoding=ascii :\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        readline = self.get_readline(lines)
        self.assertRaises(SyntaxError, detect_encoding, readline)

    def test_cookie_second_line_no_bom(self):
        lines = (
            b'#! something\n',
            b'# vim: set fileencoding=ascii :\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'ascii')
        expected = [b'#! something\n', b'# vim: set fileencoding=ascii :\n']
        self.assertEqual(consumed_lines, expected)

    def test_matched_bom_and_cookie_second_line(self):
        lines = (
            b'\xef\xbb\xbf#! something\n',
            b'f# coding=utf-8\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'utf-8-sig')
        self.assertEqual(consumed_lines,
                         [b'#! something\n', b'f# coding=utf-8\n'])

    def test_mismatched_bom_and_cookie_second_line_raises_syntaxerror(self):
        lines = (
            b'\xef\xbb\xbf#! something\n',
            b'# vim: set fileencoding=ascii :\n',
            b'print(something)\n',
            b'do_something(else)\n'
        )
        readline = self.get_readline(lines)
        self.assertRaises(SyntaxError, detect_encoding, readline)

    def test_cookie_second_line_noncommented_first_line(self):
        lines = (
            b"print('\xc2\xa3')\n",
            b'# vim: set fileencoding=iso8859-15 :\n',
            b"print('\xe2\x82\xac')\n"
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'utf-8')
        expected = [b"print('\xc2\xa3')\n"]
        self.assertEqual(consumed_lines, expected)

    def test_cookie_second_line_commented_first_line(self):
        lines = (
            b"#print('\xc2\xa3')\n",
            b'# vim: set fileencoding=iso8859-15 :\n',
            b"print('\xe2\x82\xac')\n"
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'iso8859-15')
        expected = [b"#print('\xc2\xa3')\n", b'# vim: set fileencoding=iso8859-15 :\n']
        self.assertEqual(consumed_lines, expected)

    def test_cookie_second_line_empty_first_line(self):
        lines = (
            b'\n',
            b'# vim: set fileencoding=iso8859-15 :\n',
            b"print('\xe2\x82\xac')\n"
        )
        encoding, consumed_lines = detect_encoding(self.get_readline(lines))
        self.assertEqual(encoding, 'iso8859-15')
        expected = [b'\n', b'# vim: set fileencoding=iso8859-15 :\n']
        self.assertEqual(consumed_lines, expected)

    def test_latin1_normalization(self):
        # See get_normal_name() in tokenizer.c.
        encodings = ("latin-1", "iso-8859-1", "iso-latin-1", "latin-1-unix",
                     "iso-8859-1-unix", "iso-latin-1-mac")
        for encoding in encodings:
            for rep in ("-", "_"):
                enc = encoding.replace("-", rep)
                lines = (b"#!/usr/bin/python\n",
                         b"# coding: " + enc.encode("ascii") + b"\n",
                         b"print(things)\n",
                         b"do_something += 4\n")
                rl = self.get_readline(lines)
                found, consumed_lines = detect_encoding(rl)
                self.assertEqual(found, "iso-8859-1")

    def test_syntaxerror_latin1(self):
        # Issue 14629: need to raise TokenError if the first
        # line(s) have non-UTF-8 characters
        lines = (
            b'print("\xdf")', # Latin-1: LATIN SMALL LETTER SHARP S
            )
        readline = self.get_readline(lines)
        self.assertRaises(SyntaxError, detect_encoding, readline)


    def test_utf8_normalization(self):
        # See get_normal_name() in tokenizer.c.
        encodings = ("utf-8", "utf-8-mac", "utf-8-unix")
        for encoding in encodings:
            for rep in ("-", "_"):
                enc = encoding.replace("-", rep)
                lines = (b"#!/usr/bin/python\n",
                         b"# coding: " + enc.encode("ascii") + b"\n",
                         b"1 + 3\n")
                rl = self.get_readline(lines)
                found, consumed_lines = detect_encoding(rl)
                self.assertEqual(found, "utf-8")

    def test_short_files(self):
        readline = self.get_readline((b'print(something)\n',))
        encoding, consumed_lines = detect_encoding(readline)
        self.assertEqual(encoding, 'utf-8')
        self.assertEqual(consumed_lines, [b'print(something)\n'])

        encoding, consumed_lines = detect_encoding(self.get_readline(()))
        self.assertEqual(encoding, 'utf-8')
        self.assertEqual(consumed_lines, [])

        readline = self.get_readline((b'\xef\xbb\xbfprint(something)\n',))
        encoding, consumed_lines = detect_encoding(readline)
        self.assertEqual(encoding, 'utf-8-sig')
        self.assertEqual(consumed_lines, [b'print(something)\n'])

        readline = self.get_readline((b'\xef\xbb\xbf',))
        encoding, consumed_lines = detect_encoding(readline)
        self.assertEqual(encoding, 'utf-8-sig')
        self.assertEqual(consumed_lines, [])

        readline = self.get_readline((b'# coding: bad\n',))
        self.assertRaises(SyntaxError, detect_encoding, readline)

    def test_false_encoding(self):
        # Issue 18873: "Encoding" detected in non-comment lines
        readline = self.get_readline((b'print("#coding=fake")',))
        encoding, consumed_lines = detect_encoding(readline)
        self.assertEqual(encoding, 'utf-8')
        self.assertEqual(consumed_lines, [b'print("#coding=fake")'])

    def test_open(self):
        filename = os_helper.TESTFN + '.py'
        self.addCleanup(os_helper.unlink, filename)

        # test coding cookie
        for encoding in ('iso-8859-15', 'utf-8'):
            with open(filename, 'w', encoding=encoding) as fp:
                print("# coding: %s" % encoding, file=fp)
                print("print('euro:\u20ac')", file=fp)
            with tokenize_open(filename) as fp:
                self.assertEqual(fp.encoding, encoding)
                self.assertEqual(fp.mode, 'r')

        # test BOM (no coding cookie)
        with open(filename, 'w', encoding='utf-8-sig') as fp:
            print("print('euro:\u20ac')", file=fp)
        with tokenize_open(filename) as fp:
            self.assertEqual(fp.encoding, 'utf-8-sig')
            self.assertEqual(fp.mode, 'r')

    def test_filename_in_exception(self):
        # When possible, include the file name in the exception.
        path = 'some_file_path'
        lines = (
            b'print("\xdf")', # Latin-1: LATIN SMALL LETTER SHARP S
            )
        class Bunk:
            def __init__(self, lines, path):
                self.name = path
                self._lines = lines
                self._index = 0

            def readline(self):
                if self._index == len(lines):
                    raise StopIteration
                line = lines[self._index]
                self._index += 1
                return line

        with self.assertRaises(SyntaxError):
            ins = Bunk(lines, path)
            # Make sure lacking a name isn't an issue.
            del ins.name
            detect_encoding(ins.readline)
        with self.assertRaisesRegex(SyntaxError, '.*{}'.format(path)):
            ins = Bunk(lines, path)
            detect_encoding(ins.readline)

    def test_open_error(self):
        # Issue #23840: open() must close the binary file on error
        m = BytesIO(b'#coding:xxx')
        with mock.patch('tokenize._builtin_open', return_value=m):
            self.assertRaises(SyntaxError, tokenize_open, 'foobar')
        self.assertTrue(m.closed)


class TestTokenize(TestCase):

    def test_tokenize(self):
        import tokenize as tokenize_module
        encoding = "utf-8"
        encoding_used = None
        def mock_detect_encoding(readline):
            return encoding, [b'first', b'second']

        def mock__tokenize(readline, encoding, **kwargs):
            nonlocal encoding_used
            encoding_used = encoding
            out = []
            while True:
                try:
                    next_line = readline()
                except StopIteration:
                    return out
                if next_line:
                    out.append(next_line)
                    continue
                return out

        counter = 0
        def mock_readline():
            nonlocal counter
            counter += 1
            if counter == 5:
                return b''
            return str(counter).encode()

        orig_detect_encoding = tokenize_module.detect_encoding
        orig_c_token = tokenize_module._generate_tokens_from_c_tokenizer
        tokenize_module.detect_encoding = mock_detect_encoding
        tokenize_module._generate_tokens_from_c_tokenizer = mock__tokenize
        try:
            results = tokenize(mock_readline)
            self.assertEqual(list(results)[1:],
                             [b'first', b'second', b'1', b'2', b'3', b'4'])
        finally:
            tokenize_module.detect_encoding = orig_detect_encoding
            tokenize_module._generate_tokens_from_c_tokenizer = orig_c_token

        self.assertEqual(encoding_used, encoding)

    def test_oneline_defs(self):
        buf = []
        for i in range(500):
            buf.append('def i{i}(): return {i}'.format(i=i))
        buf.append('OK')
        buf = '\n'.join(buf)

        # Test that 500 consequent, one-line defs is OK
        toks = list(tokenize(BytesIO(buf.encode('utf-8')).readline))
        self.assertEqual(toks[-3].string, 'OK') # [-1] is always ENDMARKER
                                                # [-2] is always NEWLINE

    def assertExactTypeEqual(self, opstr, *optypes):
        tokens = list(tokenize(BytesIO(opstr.encode('utf-8')).readline))
        num_optypes = len(optypes)
        self.assertEqual(len(tokens), 3 + num_optypes)
        self.assertEqual(tok_name[tokens[0].exact_type],
                         tok_name[ENCODING])
        for i in range(num_optypes):
            self.assertEqual(tok_name[tokens[i + 1].exact_type],
                             tok_name[optypes[i]])
        self.assertEqual(tok_name[tokens[1 + num_optypes].exact_type],
                         tok_name[token.NEWLINE])
        self.assertEqual(tok_name[tokens[2 + num_optypes].exact_type],
                         tok_name[token.ENDMARKER])

    def test_exact_type(self):
        self.assertExactTypeEqual('()', token.LPAR, token.RPAR)
        self.assertExactTypeEqual('[]', token.LSQB, token.RSQB)
        self.assertExactTypeEqual(':', token.COLON)
        self.assertExactTypeEqual(',', token.COMMA)
        self.assertExactTypeEqual(';', token.SEMI)
        self.assertExactTypeEqual('+', token.PLUS)
        self.assertExactTypeEqual('-', token.MINUS)
        self.assertExactTypeEqual('*', token.STAR)
        self.assertExactTypeEqual('/', token.SLASH)
        self.assertExactTypeEqual('|', token.VBAR)
        self.assertExactTypeEqual('&', token.AMPER)
        self.assertExactTypeEqual('<', token.LESS)
        self.assertExactTypeEqual('>', token.GREATER)
        self.assertExactTypeEqual('=', token.EQUAL)
        self.assertExactTypeEqual('.', token.DOT)
        self.assertExactTypeEqual('%', token.PERCENT)
        self.assertExactTypeEqual('{}', token.LBRACE, token.RBRACE)
        self.assertExactTypeEqual('==', token.EQEQUAL)
        self.assertExactTypeEqual('!=', token.NOTEQUAL)
        self.assertExactTypeEqual('<=', token.LESSEQUAL)
        self.assertExactTypeEqual('>=', token.GREATEREQUAL)
        self.assertExactTypeEqual('~', token.TILDE)
        self.assertExactTypeEqual('^', token.CIRCUMFLEX)
        self.assertExactTypeEqual('<<', token.LEFTSHIFT)
        self.assertExactTypeEqual('>>', token.RIGHTSHIFT)
        self.assertExactTypeEqual('**', token.DOUBLESTAR)
        self.assertExactTypeEqual('+=', token.PLUSEQUAL)
        self.assertExactTypeEqual('-=', token.MINEQUAL)
        self.assertExactTypeEqual('*=', token.STAREQUAL)
        self.assertExactTypeEqual('/=', token.SLASHEQUAL)
        self.assertExactTypeEqual('%=', token.PERCENTEQUAL)
        self.assertExactTypeEqual('&=', token.AMPEREQUAL)
        self.assertExactTypeEqual('|=', token.VBAREQUAL)
        self.assertExactTypeEqual('^=', token.CIRCUMFLEXEQUAL)
        self.assertExactTypeEqual('^=', token.CIRCUMFLEXEQUAL)
        self.assertExactTypeEqual('<<=', token.LEFTSHIFTEQUAL)
        self.assertExactTypeEqual('>>=', token.RIGHTSHIFTEQUAL)
        self.assertExactTypeEqual('**=', token.DOUBLESTAREQUAL)
        self.assertExactTypeEqual('//', token.DOUBLESLASH)
        self.assertExactTypeEqual('//=', token.DOUBLESLASHEQUAL)
        self.assertExactTypeEqual(':=', token.COLONEQUAL)
        self.assertExactTypeEqual('...', token.ELLIPSIS)
        self.assertExactTypeEqual('->', token.RARROW)
        self.assertExactTypeEqual('@', token.AT)
        self.assertExactTypeEqual('@=', token.ATEQUAL)

        self.assertExactTypeEqual('a**2+b**2==c**2',
                                  NAME, token.DOUBLESTAR, NUMBER,
                                  token.PLUS,
                                  NAME, token.DOUBLESTAR, NUMBER,
                                  token.EQEQUAL,
                                  NAME, token.DOUBLESTAR, NUMBER)
        self.assertExactTypeEqual('{1, 2, 3}',
                                  token.LBRACE,
                                  token.NUMBER, token.COMMA,
                                  token.NUMBER, token.COMMA,
                                  token.NUMBER,
                                  token.RBRACE)
        self.assertExactTypeEqual('^(x & 0x1)',
                                  token.CIRCUMFLEX,
                                  token.LPAR,
                                  token.NAME, token.AMPER, token.NUMBER,
                                  token.RPAR)

    def test_pathological_trailing_whitespace(self):
        # See http://bugs.python.org/issue16152
        self.assertExactTypeEqual('@          ', token.AT)

    def test_comment_at_the_end_of_the_source_without_newline(self):
        # See http://bugs.python.org/issue44667
        source = 'b = 1\n\n#test'
        expected_tokens = [
            TokenInfo(type=token.ENCODING, string='utf-8', start=(0, 0), end=(0, 0), line=''),
            TokenInfo(type=token.NAME, string='b', start=(1, 0), end=(1, 1), line='b = 1\n'),
            TokenInfo(type=token.OP, string='=', start=(1, 2), end=(1, 3), line='b = 1\n'),
            TokenInfo(type=token.NUMBER, string='1', start=(1, 4), end=(1, 5), line='b = 1\n'),
            TokenInfo(type=token.NEWLINE, string='\n', start=(1, 5), end=(1, 6), line='b = 1\n'),
            TokenInfo(type=token.NL, string='\n', start=(2, 0), end=(2, 1), line='\n'),
            TokenInfo(type=token.COMMENT, string='#test', start=(3, 0), end=(3, 5), line='#test'),
            TokenInfo(type=token.NL, string='', start=(3, 5), end=(3, 6), line='#test'),
            TokenInfo(type=token.ENDMARKER, string='', start=(4, 0), end=(4, 0), line='')
        ]

        tokens = list(tokenize(BytesIO(source.encode('utf-8')).readline))
        self.assertEqual(tokens, expected_tokens)

    def test_newline_and_space_at_the_end_of_the_source_without_newline(self):
        # See https://github.com/python/cpython/issues/105435
        source = 'a\n '
        expected_tokens = [
            TokenInfo(token.ENCODING, string='utf-8', start=(0, 0), end=(0, 0), line=''),
            TokenInfo(token.NAME, string='a', start=(1, 0), end=(1, 1), line='a\n'),
            TokenInfo(token.NEWLINE, string='\n', start=(1, 1), end=(1, 2), line='a\n'),
            TokenInfo(token.NL, string='', start=(2, 1), end=(2, 2), line=' '),
            TokenInfo(token.ENDMARKER, string='', start=(3, 0), end=(3, 0), line='')
        ]

        tokens = list(tokenize(BytesIO(source.encode('utf-8')).readline))
        self.assertEqual(tokens, expected_tokens)

    def test_invalid_character_in_fstring_middle(self):
        # See gh-103824
        script = b'''F"""
        \xe5"""'''

        with os_helper.temp_dir() as temp_dir:
            filename = os.path.join(temp_dir, "script.py")
            with open(filename, 'wb') as file:
                file.write(script)
            rs, _ = run_python_until_end(filename)
            self.assertIn(b"SyntaxError", rs.err)


class UntokenizeTest(TestCase):

    def test_bad_input_order(self):
        # raise if previous row
        u = Untokenizer()
        u.prev_row = 2
        u.prev_col = 2
        with self.assertRaises(ValueError) as cm:
            u.add_whitespace((1,3))
        self.assertEqual(cm.exception.args[0],
                'start (1,3) precedes previous end (2,2)')
        # raise if previous column in row
        self.assertRaises(ValueError, u.add_whitespace, (2,1))

    def test_backslash_continuation(self):
        # The problem is that <whitespace>\<newline> leaves no token
        u = Untokenizer()
        u.prev_row = 1
        u.prev_col =  1
        u.tokens = []
        u.add_whitespace((2, 0))
        self.assertEqual(u.tokens, ['\\\n'])
        u.prev_row = 2
        u.add_whitespace((4, 4))
        self.assertEqual(u.tokens, ['\\\n', '\\\n\\\n', '    '])
        TestRoundtrip.check_roundtrip(self, 'a\n  b\n    c\n  \\\n  c\n')

    def test_iter_compat(self):
        u = Untokenizer()
        token = (NAME, 'Hello')
        tokens = [(ENCODING, 'utf-8'), token]
        u.compat(token, iter([]))
        self.assertEqual(u.tokens, ["Hello "])
        u = Untokenizer()
        self.assertEqual(u.untokenize(iter([token])), 'Hello ')
        u = Untokenizer()
        self.assertEqual(u.untokenize(iter(tokens)), 'Hello ')
        self.assertEqual(u.encoding, 'utf-8')
        self.assertEqual(untokenize(iter(tokens)), b'Hello ')


class TestRoundtrip(TestCase):

    def check_roundtrip(self, f):
        """
        Test roundtrip for `untokenize`. `f` is an open file or a string.
        The source code in f is tokenized to both 5- and 2-tuples.
        Both sequences are converted back to source code via
        tokenize.untokenize(), and the latter tokenized again to 2-tuples.
        The test fails if the 3 pair tokenizations do not match.

        When untokenize bugs are fixed, untokenize with 5-tuples should
        reproduce code that does not contain a backslash continuation
        following spaces.  A proper test should test this.
        """
        # Get source code and original tokenizations
        if isinstance(f, str):
            code = f.encode('utf-8')
        else:
            code = f.read()
        readline = iter(code.splitlines(keepends=True)).__next__
        tokens5 = list(tokenize(readline))
        tokens2 = [tok[:2] for tok in tokens5]
        # Reproduce tokens2 from pairs
        bytes_from2 = untokenize(tokens2)
        readline2 = iter(bytes_from2.splitlines(keepends=True)).__next__
        tokens2_from2 = [tok[:2] for tok in tokenize(readline2)]
        self.assertEqual(tokens2_from2, tokens2)
        # Reproduce tokens2 from 5-tuples
        bytes_from5 = untokenize(tokens5)
        readline5 = iter(bytes_from5.splitlines(keepends=True)).__next__
        tokens2_from5 = [tok[:2] for tok in tokenize(readline5)]
        self.assertEqual(tokens2_from5, tokens2)

    def check_line_extraction(self, f):
        if isinstance(f, str):
            code = f.encode('utf-8')
        else:
            code = f.read()
        readline = iter(code.splitlines(keepends=True)).__next__
        for tok in tokenize(readline):
            if tok.type in  {ENCODING, ENDMARKER}:
                continue
            self.assertEqual(tok.string, tok.line[tok.start[1]: tok.end[1]])

    def test_roundtrip(self):
        # There are some standard formatting practices that are easy to get right.

        self.check_roundtrip("if x == 1:\n"
                             "    print(x)\n")
        self.check_roundtrip("# This is a comment\n"
                             "# This also\n")

        # Some people use different formatting conventions, which makes
        # untokenize a little trickier. Note that this test involves trailing
        # whitespace after the colon. Note that we use hex escapes to make the
        # two trailing blanks apparent in the expected output.

        self.check_roundtrip("if x == 1 : \n"
                             "  print(x)\n")
        fn = support.findfile("tokenize_tests.txt", subdir="tokenizedata")
        with open(fn, 'rb') as f:
            self.check_roundtrip(f)
        self.check_roundtrip("if x == 1:\n"
                             "    # A comment by itself.\n"
                             "    print(x) # Comment here, too.\n"
                             "    # Another comment.\n"
                             "after_if = True\n")
        self.check_roundtrip("if (x # The comments need to go in the right place\n"
                             "    == 1):\n"
                             "    print('x==1')\n")
        self.check_roundtrip("class Test: # A comment here\n"
                             "  # A comment with weird indent\n"
                             "  after_com = 5\n"
                             "  def x(m): return m*5 # a one liner\n"
                             "  def y(m): # A whitespace after the colon\n"
                             "     return y*4 # 3-space indent\n")

        # Some error-handling code
        self.check_roundtrip("try: import somemodule\n"
                             "except ImportError: # comment\n"
                             "    print('Can not import' # comment2\n)"
                             "else:   print('Loaded')\n")

        self.check_roundtrip("f'\\N{EXCLAMATION MARK}'")
        self.check_roundtrip(r"f'\\N{SNAKE}'")
        self.check_roundtrip(r"f'\\N{{SNAKE}}'")
        self.check_roundtrip(r"f'\N{SNAKE}'")
        self.check_roundtrip(r"f'\\\N{SNAKE}'")
        self.check_roundtrip(r"f'\\\\\N{SNAKE}'")
        self.check_roundtrip(r"f'\\\\\\\N{SNAKE}'")

        self.check_roundtrip(r"f'\\N{1}'")
        self.check_roundtrip(r"f'\\\\N{2}'")
        self.check_roundtrip(r"f'\\\\\\N{3}'")
        self.check_roundtrip(r"f'\\\\\\\\N{4}'")

        self.check_roundtrip(r"f'\\N{{'")
        self.check_roundtrip(r"f'\\\\N{{'")
        self.check_roundtrip(r"f'\\\\\\N{{'")
        self.check_roundtrip(r"f'\\\\\\\\N{{'")
        cases = [
    """
if 1:
    "foo"
"bar"
""",
    """
if 1:
    ("foo"
     "bar")
""",
    """
if 1:
    "foo"
    "bar"
""" ]
        for case in cases:
            self.check_roundtrip(case)


    def test_continuation(self):
        # Balancing continuation
        self.check_roundtrip("a = (3,4, \n"
                             "5,6)\n"
                             "y = [3, 4,\n"
                             "5]\n"
                             "z = {'a': 5,\n"
                             "'b':15, 'c':True}\n"
                             "x = len(y) + 5 - a[\n"
                             "3] - a[2]\n"
                             "+ len(z) - z[\n"
                             "'b']\n")

    def test_backslash_continuation(self):
        # Backslash means line continuation, except for comments
        self.check_roundtrip("x=1+\\\n"
                             "1\n"
                             "# This is a comment\\\n"
                             "# This also\n")
        self.check_roundtrip("# Comment \\\n"
                             "x = 0")

    def test_string_concatenation(self):
        # Two string literals on the same line
        self.check_roundtrip("'' ''")

    def test_random_files(self):
        # Test roundtrip on random python modules.
        # pass the '-ucpu' option to process the full directory.

        import glob, random
        tempdir = os.path.dirname(__file__) or os.curdir
        testfiles = glob.glob(os.path.join(glob.escape(tempdir), "test*.py"))

        if not support.is_resource_enabled("cpu"):
            testfiles = random.sample(testfiles, 10)

        for testfile in testfiles:
            if support.verbose >= 2:
                print('tokenize', testfile)
            with open(testfile, 'rb') as f:
                with self.subTest(file=testfile):
                    self.check_roundtrip(f)
                    self.check_line_extraction(f)


    def roundtrip(self, code):
        if isinstance(code, str):
            code = code.encode('utf-8')
        return untokenize(tokenize(BytesIO(code).readline)).decode('utf-8')

    def test_indentation_semantics_retained(self):
        """
        Ensure that although whitespace might be mutated in a roundtrip,
        the semantic meaning of the indentation remains consistent.
        """
        code = "if False:\n\tx=3\n\tx=3\n"
        codelines = self.roundtrip(code).split('\n')
        self.assertEqual(codelines[1], codelines[2])
        self.check_roundtrip(code)


class InvalidPythonTests(TestCase):
    def test_number_followed_by_name(self):
        # See issue #gh-105549
        source = "2sin(x)"
        expected_tokens = [
            TokenInfo(type=token.NUMBER, string='2', start=(1, 0), end=(1, 1), line='2sin(x)'),
            TokenInfo(type=token.NAME, string='sin', start=(1, 1), end=(1, 4), line='2sin(x)'),
            TokenInfo(type=token.OP, string='(', start=(1, 4), end=(1, 5), line='2sin(x)'),
            TokenInfo(type=token.NAME, string='x', start=(1, 5), end=(1, 6), line='2sin(x)'),
            TokenInfo(type=token.OP, string=')', start=(1, 6), end=(1, 7), line='2sin(x)'),
            TokenInfo(type=token.NEWLINE, string='', start=(1, 7), end=(1, 8), line='2sin(x)'),
            TokenInfo(type=token.ENDMARKER, string='', start=(2, 0), end=(2, 0), line='')
        ]

        tokens = list(generate_tokens(StringIO(source).readline))
        self.assertEqual(tokens, expected_tokens)

    def test_number_starting_with_zero(self):
        source = "01234"
        expected_tokens = [
            TokenInfo(type=token.NUMBER, string='01234', start=(1, 0), end=(1, 5), line='01234'),
            TokenInfo(type=token.NEWLINE, string='', start=(1, 5), end=(1, 6), line='01234'),
            TokenInfo(type=token.ENDMARKER, string='', start=(2, 0), end=(2, 0), line='')
        ]

        tokens = list(generate_tokens(StringIO(source).readline))
        self.assertEqual(tokens, expected_tokens)

class CTokenizeTest(TestCase):
    def check_tokenize(self, s, expected):
        # Format the tokens in s in a table format.
        # The ENDMARKER and final NEWLINE are omitted.
        f = StringIO(s)
        with self.subTest(source=s):
            result = stringify_tokens_from_source(
                _generate_tokens_from_c_tokenizer(f.readline), s
            )
            self.assertEqual(result, expected.rstrip().splitlines())

    def test_encoding(self):
        def readline(encoding):
            yield "1+1".encode(encoding)

        expected = [
            TokenInfo(type=NUMBER, string='1', start=(1, 0), end=(1, 1), line='1+1'),
            TokenInfo(type=OP, string='+', start=(1, 1), end=(1, 2), line='1+1'),
            TokenInfo(type=NUMBER, string='1', start=(1, 2), end=(1, 3), line='1+1'),
            TokenInfo(type=NEWLINE, string='', start=(1, 3), end=(1, 4), line='1+1'),
            TokenInfo(type=ENDMARKER, string='', start=(2, 0), end=(2, 0), line='')
        ]
        for encoding in ["utf-8", "latin-1", "utf-16"]:
            with self.subTest(encoding=encoding):
                tokens = list(_generate_tokens_from_c_tokenizer(
                    readline(encoding).__next__,
                    extra_tokens=True,
                    encoding=encoding,
                ))
                self.assertEqual(tokens, expected)

    def test_int(self):

        self.check_tokenize('0xff <= 255', """\
    NUMBER     '0xff'        (1, 0) (1, 4)
    LESSEQUAL  '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    """)

        self.check_tokenize('0b10 <= 255', """\
    NUMBER     '0b10'        (1, 0) (1, 4)
    LESSEQUAL  '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    """)

        self.check_tokenize('0o123 <= 0O123', """\
    NUMBER     '0o123'       (1, 0) (1, 5)
    LESSEQUAL  '<='          (1, 6) (1, 8)
    NUMBER     '0O123'       (1, 9) (1, 14)
    """)

        self.check_tokenize('1234567 > ~0x15', """\
    NUMBER     '1234567'     (1, 0) (1, 7)
    GREATER    '>'           (1, 8) (1, 9)
    TILDE      '~'           (1, 10) (1, 11)
    NUMBER     '0x15'        (1, 11) (1, 15)
    """)

        self.check_tokenize('2134568 != 1231515', """\
    NUMBER     '2134568'     (1, 0) (1, 7)
    NOTEQUAL   '!='          (1, 8) (1, 10)
    NUMBER     '1231515'     (1, 11) (1, 18)
    """)

        self.check_tokenize('(-124561-1) & 200000000', """\
    LPAR       '('           (1, 0) (1, 1)
    MINUS      '-'           (1, 1) (1, 2)
    NUMBER     '124561'      (1, 2) (1, 8)
    MINUS      '-'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    RPAR       ')'           (1, 10) (1, 11)
    AMPER      '&'           (1, 12) (1, 13)
    NUMBER     '200000000'   (1, 14) (1, 23)
    """)

        self.check_tokenize('0xdeadbeef != -1', """\
    NUMBER     '0xdeadbeef'  (1, 0) (1, 10)
    NOTEQUAL   '!='          (1, 11) (1, 13)
    MINUS      '-'           (1, 14) (1, 15)
    NUMBER     '1'           (1, 15) (1, 16)
    """)

        self.check_tokenize('0xdeadc0de & 12345', """\
    NUMBER     '0xdeadc0de'  (1, 0) (1, 10)
    AMPER      '&'           (1, 11) (1, 12)
    NUMBER     '12345'       (1, 13) (1, 18)
    """)

        self.check_tokenize('0xFF & 0x15 | 1234', """\
    NUMBER     '0xFF'        (1, 0) (1, 4)
    AMPER      '&'           (1, 5) (1, 6)
    NUMBER     '0x15'        (1, 7) (1, 11)
    VBAR       '|'           (1, 12) (1, 13)
    NUMBER     '1234'        (1, 14) (1, 18)
    """)

    def test_float(self):

        self.check_tokenize('x = 3.14159', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '3.14159'     (1, 4) (1, 11)
    """)

        self.check_tokenize('x = 314159.', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '314159.'     (1, 4) (1, 11)
    """)

        self.check_tokenize('x = .314159', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '.314159'     (1, 4) (1, 11)
    """)

        self.check_tokenize('x = 3e14159', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '3e14159'     (1, 4) (1, 11)
    """)

        self.check_tokenize('x = 3E123', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '3E123'       (1, 4) (1, 9)
    """)

        self.check_tokenize('x+y = 3e-1230', """\
    NAME       'x'           (1, 0) (1, 1)
    PLUS       '+'           (1, 1) (1, 2)
    NAME       'y'           (1, 2) (1, 3)
    EQUAL      '='           (1, 4) (1, 5)
    NUMBER     '3e-1230'     (1, 6) (1, 13)
    """)

        self.check_tokenize('x = 3.14e159', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '3.14e159'    (1, 4) (1, 12)
    """)

    def test_string(self):

        self.check_tokenize('x = \'\'; y = ""', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    STRING     "''"          (1, 4) (1, 6)
    SEMI       ';'           (1, 6) (1, 7)
    NAME       'y'           (1, 8) (1, 9)
    EQUAL      '='           (1, 10) (1, 11)
    STRING     '""'          (1, 12) (1, 14)
    """)

        self.check_tokenize('x = \'"\'; y = "\'"', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    STRING     '\\'"\\''       (1, 4) (1, 7)
    SEMI       ';'           (1, 7) (1, 8)
    NAME       'y'           (1, 9) (1, 10)
    EQUAL      '='           (1, 11) (1, 12)
    STRING     '"\\'"'        (1, 13) (1, 16)
    """)

        self.check_tokenize('x = "doesn\'t "shrink", does it"', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    STRING     '"doesn\\'t "' (1, 4) (1, 14)
    NAME       'shrink'      (1, 14) (1, 20)
    STRING     '", does it"' (1, 20) (1, 31)
    """)

        self.check_tokenize("x = 'abc' + 'ABC'", """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    STRING     "'abc'"       (1, 4) (1, 9)
    PLUS       '+'           (1, 10) (1, 11)
    STRING     "'ABC'"       (1, 12) (1, 17)
    """)

        self.check_tokenize('y = "ABC" + "ABC"', """\
    NAME       'y'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    STRING     '"ABC"'       (1, 4) (1, 9)
    PLUS       '+'           (1, 10) (1, 11)
    STRING     '"ABC"'       (1, 12) (1, 17)
    """)

        self.check_tokenize("x = r'abc' + r'ABC' + R'ABC' + R'ABC'", """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    STRING     "r'abc'"      (1, 4) (1, 10)
    PLUS       '+'           (1, 11) (1, 12)
    STRING     "r'ABC'"      (1, 13) (1, 19)
    PLUS       '+'           (1, 20) (1, 21)
    STRING     "R'ABC'"      (1, 22) (1, 28)
    PLUS       '+'           (1, 29) (1, 30)
    STRING     "R'ABC'"      (1, 31) (1, 37)
    """)

        self.check_tokenize('y = r"abc" + r"ABC" + R"ABC" + R"ABC"', """\
    NAME       'y'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    STRING     'r"abc"'      (1, 4) (1, 10)
    PLUS       '+'           (1, 11) (1, 12)
    STRING     'r"ABC"'      (1, 13) (1, 19)
    PLUS       '+'           (1, 20) (1, 21)
    STRING     'R"ABC"'      (1, 22) (1, 28)
    PLUS       '+'           (1, 29) (1, 30)
    STRING     'R"ABC"'      (1, 31) (1, 37)
    """)

        self.check_tokenize("u'abc' + U'abc'", """\
    STRING     "u'abc'"      (1, 0) (1, 6)
    PLUS       '+'           (1, 7) (1, 8)
    STRING     "U'abc'"      (1, 9) (1, 15)
    """)

        self.check_tokenize('u"abc" + U"abc"', """\
    STRING     'u"abc"'      (1, 0) (1, 6)
    PLUS       '+'           (1, 7) (1, 8)
    STRING     'U"abc"'      (1, 9) (1, 15)
    """)

        self.check_tokenize("b'abc' + B'abc'", """\
    STRING     "b'abc'"      (1, 0) (1, 6)
    PLUS       '+'           (1, 7) (1, 8)
    STRING     "B'abc'"      (1, 9) (1, 15)
    """)

        self.check_tokenize('b"abc" + B"abc"', """\
    STRING     'b"abc"'      (1, 0) (1, 6)
    PLUS       '+'           (1, 7) (1, 8)
    STRING     'B"abc"'      (1, 9) (1, 15)
    """)

        self.check_tokenize("br'abc' + bR'abc' + Br'abc' + BR'abc'", """\
    STRING     "br'abc'"     (1, 0) (1, 7)
    PLUS       '+'           (1, 8) (1, 9)
    STRING     "bR'abc'"     (1, 10) (1, 17)
    PLUS       '+'           (1, 18) (1, 19)
    STRING     "Br'abc'"     (1, 20) (1, 27)
    PLUS       '+'           (1, 28) (1, 29)
    STRING     "BR'abc'"     (1, 30) (1, 37)
    """)

        self.check_tokenize('br"abc" + bR"abc" + Br"abc" + BR"abc"', """\
    STRING     'br"abc"'     (1, 0) (1, 7)
    PLUS       '+'           (1, 8) (1, 9)
    STRING     'bR"abc"'     (1, 10) (1, 17)
    PLUS       '+'           (1, 18) (1, 19)
    STRING     'Br"abc"'     (1, 20) (1, 27)
    PLUS       '+'           (1, 28) (1, 29)
    STRING     'BR"abc"'     (1, 30) (1, 37)
    """)

        self.check_tokenize("rb'abc' + rB'abc' + Rb'abc' + RB'abc'", """\
    STRING     "rb'abc'"     (1, 0) (1, 7)
    PLUS       '+'           (1, 8) (1, 9)
    STRING     "rB'abc'"     (1, 10) (1, 17)
    PLUS       '+'           (1, 18) (1, 19)
    STRING     "Rb'abc'"     (1, 20) (1, 27)
    PLUS       '+'           (1, 28) (1, 29)
    STRING     "RB'abc'"     (1, 30) (1, 37)
    """)

        self.check_tokenize('rb"abc" + rB"abc" + Rb"abc" + RB"abc"', """\
    STRING     'rb"abc"'     (1, 0) (1, 7)
    PLUS       '+'           (1, 8) (1, 9)
    STRING     'rB"abc"'     (1, 10) (1, 17)
    PLUS       '+'           (1, 18) (1, 19)
    STRING     'Rb"abc"'     (1, 20) (1, 27)
    PLUS       '+'           (1, 28) (1, 29)
    STRING     'RB"abc"'     (1, 30) (1, 37)
    """)

        self.check_tokenize('"a\\\nde\\\nfg"', """\
    STRING     '"a\\\\\\nde\\\\\\nfg"\' (1, 0) (3, 3)
    """)

        self.check_tokenize('u"a\\\nde"', """\
    STRING     'u"a\\\\\\nde"\'  (1, 0) (2, 3)
    """)

        self.check_tokenize('rb"a\\\nd"', """\
    STRING     'rb"a\\\\\\nd"\'  (1, 0) (2, 2)
    """)

        self.check_tokenize(r'"""a\
b"""', """\
    STRING     '\"\""a\\\\\\nb\"\""' (1, 0) (2, 4)
    """)
        self.check_tokenize(r'u"""a\
b"""', """\
    STRING     'u\"\""a\\\\\\nb\"\""' (1, 0) (2, 4)
    """)
        self.check_tokenize(r'rb"""a\
b\
c"""', """\
    STRING     'rb"\""a\\\\\\nb\\\\\\nc"\""' (1, 0) (3, 4)
    """)

        self.check_tokenize(r'"hola\\\r\ndfgf"', """\
    STRING     \'"hola\\\\\\\\\\\\r\\\\ndfgf"\' (1, 0) (1, 16)
    """)

        self.check_tokenize('f"abc"', """\
    FSTRING_START 'f"'          (1, 0) (1, 2)
    FSTRING_MIDDLE 'abc'         (1, 2) (1, 5)
    FSTRING_END '"'           (1, 5) (1, 6)
    """)

        self.check_tokenize('fR"a{b}c"', """\
    FSTRING_START 'fR"'         (1, 0) (1, 3)
    FSTRING_MIDDLE 'a'           (1, 3) (1, 4)
    LBRACE     '{'           (1, 4) (1, 5)
    NAME       'b'           (1, 5) (1, 6)
    RBRACE     '}'           (1, 6) (1, 7)
    FSTRING_MIDDLE 'c'           (1, 7) (1, 8)
    FSTRING_END '"'           (1, 8) (1, 9)
    """)

        self.check_tokenize('f"""abc"""', """\
    FSTRING_START 'f\"""'        (1, 0) (1, 4)
    FSTRING_MIDDLE 'abc'         (1, 4) (1, 7)
    FSTRING_END '\"""'         (1, 7) (1, 10)
    """)

        self.check_tokenize(r'f"abc\
def"', """\
    FSTRING_START \'f"\'          (1, 0) (1, 2)
    FSTRING_MIDDLE 'abc\\\\\\ndef'  (1, 2) (2, 3)
    FSTRING_END '"'           (2, 3) (2, 4)
    """)

        self.check_tokenize('''\
f"{
a}"''', """\
    FSTRING_START 'f"'          (1, 0) (1, 2)
    LBRACE     '{'           (1, 2) (1, 3)
    NAME       'a'           (2, 0) (2, 1)
    RBRACE     '}'           (2, 1) (2, 2)
    FSTRING_END '"'           (2, 2) (2, 3)
    """)

        self.check_tokenize(r'Rf"abc\
def"', """\
    FSTRING_START 'Rf"'         (1, 0) (1, 3)
    FSTRING_MIDDLE 'abc\\\\\\ndef'  (1, 3) (2, 3)
    FSTRING_END '"'           (2, 3) (2, 4)
    """)

        self.check_tokenize(r'f"hola\\\r\ndfgf"', """\
    FSTRING_START \'f"\'          (1, 0) (1, 2)
    FSTRING_MIDDLE 'hola\\\\\\\\\\\\r\\\\ndfgf' (1, 2) (1, 16)
    FSTRING_END \'"\'           (1, 16) (1, 17)
    """)

        self.check_tokenize("""\
f'''__{
    x:a
}__'''""", """\
    FSTRING_START "f'''"        (1, 0) (1, 4)
    FSTRING_MIDDLE '__'          (1, 4) (1, 6)
    LBRACE     '{'           (1, 6) (1, 7)
    NAME       'x'           (2, 4) (2, 5)
    COLON      ':'           (2, 5) (2, 6)
    FSTRING_MIDDLE 'a\\n'         (2, 6) (3, 0)
    RBRACE     '}'           (3, 0) (3, 1)
    FSTRING_MIDDLE '__'          (3, 1) (3, 3)
    FSTRING_END "'''"         (3, 3) (3, 6)
    """)

        self.check_tokenize("""\
f'''__{
    x:a
    b
     c
      d
}__'''""", """\
    FSTRING_START "f'''"        (1, 0) (1, 4)
    FSTRING_MIDDLE '__'          (1, 4) (1, 6)
    LBRACE     '{'           (1, 6) (1, 7)
    NAME       'x'           (2, 4) (2, 5)
    COLON      ':'           (2, 5) (2, 6)
    FSTRING_MIDDLE 'a\\n    b\\n     c\\n      d\\n' (2, 6) (6, 0)
    RBRACE     '}'           (6, 0) (6, 1)
    FSTRING_MIDDLE '__'          (6, 1) (6, 3)
    FSTRING_END "'''"         (6, 3) (6, 6)
    """)

        self.check_tokenize("""\
f'__{
    x:d
}__'""", """\
    FSTRING_START "f'"          (1, 0) (1, 2)
    FSTRING_MIDDLE '__'          (1, 2) (1, 4)
    LBRACE     '{'           (1, 4) (1, 5)
    NAME       'x'           (2, 4) (2, 5)
    COLON      ':'           (2, 5) (2, 6)
    FSTRING_MIDDLE 'd'           (2, 6) (2, 7)
    RBRACE     '}'           (3, 0) (3, 1)
    FSTRING_MIDDLE '__'          (3, 1) (3, 3)
    FSTRING_END "'"           (3, 3) (3, 4)
    """)

    def test_function(self):

        self.check_tokenize('def d22(a, b, c=2, d=2, *k): pass', """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'd22'         (1, 4) (1, 7)
    LPAR       '('           (1, 7) (1, 8)
    NAME       'a'           (1, 8) (1, 9)
    COMMA      ','           (1, 9) (1, 10)
    NAME       'b'           (1, 11) (1, 12)
    COMMA      ','           (1, 12) (1, 13)
    NAME       'c'           (1, 14) (1, 15)
    EQUAL      '='           (1, 15) (1, 16)
    NUMBER     '2'           (1, 16) (1, 17)
    COMMA      ','           (1, 17) (1, 18)
    NAME       'd'           (1, 19) (1, 20)
    EQUAL      '='           (1, 20) (1, 21)
    NUMBER     '2'           (1, 21) (1, 22)
    COMMA      ','           (1, 22) (1, 23)
    STAR       '*'           (1, 24) (1, 25)
    NAME       'k'           (1, 25) (1, 26)
    RPAR       ')'           (1, 26) (1, 27)
    COLON      ':'           (1, 27) (1, 28)
    NAME       'pass'        (1, 29) (1, 33)
    """)

        self.check_tokenize('def d01v_(a=1, *k, **w): pass', """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'd01v_'       (1, 4) (1, 9)
    LPAR       '('           (1, 9) (1, 10)
    NAME       'a'           (1, 10) (1, 11)
    EQUAL      '='           (1, 11) (1, 12)
    NUMBER     '1'           (1, 12) (1, 13)
    COMMA      ','           (1, 13) (1, 14)
    STAR       '*'           (1, 15) (1, 16)
    NAME       'k'           (1, 16) (1, 17)
    COMMA      ','           (1, 17) (1, 18)
    DOUBLESTAR '**'          (1, 19) (1, 21)
    NAME       'w'           (1, 21) (1, 22)
    RPAR       ')'           (1, 22) (1, 23)
    COLON      ':'           (1, 23) (1, 24)
    NAME       'pass'        (1, 25) (1, 29)
    """)

        self.check_tokenize('def d23(a: str, b: int=3) -> int: pass', """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'd23'         (1, 4) (1, 7)
    LPAR       '('           (1, 7) (1, 8)
    NAME       'a'           (1, 8) (1, 9)
    COLON      ':'           (1, 9) (1, 10)
    NAME       'str'         (1, 11) (1, 14)
    COMMA      ','           (1, 14) (1, 15)
    NAME       'b'           (1, 16) (1, 17)
    COLON      ':'           (1, 17) (1, 18)
    NAME       'int'         (1, 19) (1, 22)
    EQUAL      '='           (1, 22) (1, 23)
    NUMBER     '3'           (1, 23) (1, 24)
    RPAR       ')'           (1, 24) (1, 25)
    RARROW     '->'          (1, 26) (1, 28)
    NAME       'int'         (1, 29) (1, 32)
    COLON      ':'           (1, 32) (1, 33)
    NAME       'pass'        (1, 34) (1, 38)
    """)

    def test_comparison(self):

        self.check_tokenize("if 1 < 1 > 1 == 1 >= 5 <= 0x15 <= 0x12 != "
                            "1 and 5 in 1 not in 1 is 1 or 5 is not 1: pass", """\
    NAME       'if'          (1, 0) (1, 2)
    NUMBER     '1'           (1, 3) (1, 4)
    LESS       '<'           (1, 5) (1, 6)
    NUMBER     '1'           (1, 7) (1, 8)
    GREATER    '>'           (1, 9) (1, 10)
    NUMBER     '1'           (1, 11) (1, 12)
    EQEQUAL    '=='          (1, 13) (1, 15)
    NUMBER     '1'           (1, 16) (1, 17)
    GREATEREQUAL '>='          (1, 18) (1, 20)
    NUMBER     '5'           (1, 21) (1, 22)
    LESSEQUAL  '<='          (1, 23) (1, 25)
    NUMBER     '0x15'        (1, 26) (1, 30)
    LESSEQUAL  '<='          (1, 31) (1, 33)
    NUMBER     '0x12'        (1, 34) (1, 38)
    NOTEQUAL   '!='          (1, 39) (1, 41)
    NUMBER     '1'           (1, 42) (1, 43)
    NAME       'and'         (1, 44) (1, 47)
    NUMBER     '5'           (1, 48) (1, 49)
    NAME       'in'          (1, 50) (1, 52)
    NUMBER     '1'           (1, 53) (1, 54)
    NAME       'not'         (1, 55) (1, 58)
    NAME       'in'          (1, 59) (1, 61)
    NUMBER     '1'           (1, 62) (1, 63)
    NAME       'is'          (1, 64) (1, 66)
    NUMBER     '1'           (1, 67) (1, 68)
    NAME       'or'          (1, 69) (1, 71)
    NUMBER     '5'           (1, 72) (1, 73)
    NAME       'is'          (1, 74) (1, 76)
    NAME       'not'         (1, 77) (1, 80)
    NUMBER     '1'           (1, 81) (1, 82)
    COLON      ':'           (1, 82) (1, 83)
    NAME       'pass'        (1, 84) (1, 88)
    """)

    def test_additive(self):

        self.check_tokenize('x = 1 - y + 15 - 1 + 0x124 + z + a[5]', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    MINUS      '-'           (1, 6) (1, 7)
    NAME       'y'           (1, 8) (1, 9)
    PLUS       '+'           (1, 10) (1, 11)
    NUMBER     '15'          (1, 12) (1, 14)
    MINUS      '-'           (1, 15) (1, 16)
    NUMBER     '1'           (1, 17) (1, 18)
    PLUS       '+'           (1, 19) (1, 20)
    NUMBER     '0x124'       (1, 21) (1, 26)
    PLUS       '+'           (1, 27) (1, 28)
    NAME       'z'           (1, 29) (1, 30)
    PLUS       '+'           (1, 31) (1, 32)
    NAME       'a'           (1, 33) (1, 34)
    LSQB       '['           (1, 34) (1, 35)
    NUMBER     '5'           (1, 35) (1, 36)
    RSQB       ']'           (1, 36) (1, 37)
    """)

    def test_multiplicative(self):

        self.check_tokenize('x = 1//1*1/5*12%0x12@42', """\
    NAME       'x'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    DOUBLESLASH '//'          (1, 5) (1, 7)
    NUMBER     '1'           (1, 7) (1, 8)
    STAR       '*'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    SLASH      '/'           (1, 10) (1, 11)
    NUMBER     '5'           (1, 11) (1, 12)
    STAR       '*'           (1, 12) (1, 13)
    NUMBER     '12'          (1, 13) (1, 15)
    PERCENT    '%'           (1, 15) (1, 16)
    NUMBER     '0x12'        (1, 16) (1, 20)
    AT         '@'           (1, 20) (1, 21)
    NUMBER     '42'          (1, 21) (1, 23)
    """)

    def test_unary(self):

        self.check_tokenize('~1 ^ 1 & 1 |1 ^ -1', """\
    TILDE      '~'           (1, 0) (1, 1)
    NUMBER     '1'           (1, 1) (1, 2)
    CIRCUMFLEX '^'           (1, 3) (1, 4)
    NUMBER     '1'           (1, 5) (1, 6)
    AMPER      '&'           (1, 7) (1, 8)
    NUMBER     '1'           (1, 9) (1, 10)
    VBAR       '|'           (1, 11) (1, 12)
    NUMBER     '1'           (1, 12) (1, 13)
    CIRCUMFLEX '^'           (1, 14) (1, 15)
    MINUS      '-'           (1, 16) (1, 17)
    NUMBER     '1'           (1, 17) (1, 18)
    """)

        self.check_tokenize('-1*1/1+1*1//1 - ---1**1', """\
    MINUS      '-'           (1, 0) (1, 1)
    NUMBER     '1'           (1, 1) (1, 2)
    STAR       '*'           (1, 2) (1, 3)
    NUMBER     '1'           (1, 3) (1, 4)
    SLASH      '/'           (1, 4) (1, 5)
    NUMBER     '1'           (1, 5) (1, 6)
    PLUS       '+'           (1, 6) (1, 7)
    NUMBER     '1'           (1, 7) (1, 8)
    STAR       '*'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    DOUBLESLASH '//'          (1, 10) (1, 12)
    NUMBER     '1'           (1, 12) (1, 13)
    MINUS      '-'           (1, 14) (1, 15)
    MINUS      '-'           (1, 16) (1, 17)
    MINUS      '-'           (1, 17) (1, 18)
    MINUS      '-'           (1, 18) (1, 19)
    NUMBER     '1'           (1, 19) (1, 20)
    DOUBLESTAR '**'          (1, 20) (1, 22)
    NUMBER     '1'           (1, 22) (1, 23)
    """)

    def test_selector(self):

        self.check_tokenize("import sys, time\nx = sys.modules['time'].time()", """\
    NAME       'import'      (1, 0) (1, 6)
    NAME       'sys'         (1, 7) (1, 10)
    COMMA      ','           (1, 10) (1, 11)
    NAME       'time'        (1, 12) (1, 16)
    NEWLINE    ''            (1, 16) (1, 16)
    NAME       'x'           (2, 0) (2, 1)
    EQUAL      '='           (2, 2) (2, 3)
    NAME       'sys'         (2, 4) (2, 7)
    DOT        '.'           (2, 7) (2, 8)
    NAME       'modules'     (2, 8) (2, 15)
    LSQB       '['           (2, 15) (2, 16)
    STRING     "'time'"      (2, 16) (2, 22)
    RSQB       ']'           (2, 22) (2, 23)
    DOT        '.'           (2, 23) (2, 24)
    NAME       'time'        (2, 24) (2, 28)
    LPAR       '('           (2, 28) (2, 29)
    RPAR       ')'           (2, 29) (2, 30)
    """)

    def test_method(self):

        self.check_tokenize('@staticmethod\ndef foo(x,y): pass', """\
    AT         '@'           (1, 0) (1, 1)
    NAME       'staticmethod' (1, 1) (1, 13)
    NEWLINE    ''            (1, 13) (1, 13)
    NAME       'def'         (2, 0) (2, 3)
    NAME       'foo'         (2, 4) (2, 7)
    LPAR       '('           (2, 7) (2, 8)
    NAME       'x'           (2, 8) (2, 9)
    COMMA      ','           (2, 9) (2, 10)
    NAME       'y'           (2, 10) (2, 11)
    RPAR       ')'           (2, 11) (2, 12)
    COLON      ':'           (2, 12) (2, 13)
    NAME       'pass'        (2, 14) (2, 18)
    """)

    def test_tabs(self):

        self.check_tokenize('@staticmethod\ndef foo(x,y): pass', """\
    AT         '@'           (1, 0) (1, 1)
    NAME       'staticmethod' (1, 1) (1, 13)
    NEWLINE    ''            (1, 13) (1, 13)
    NAME       'def'         (2, 0) (2, 3)
    NAME       'foo'         (2, 4) (2, 7)
    LPAR       '('           (2, 7) (2, 8)
    NAME       'x'           (2, 8) (2, 9)
    COMMA      ','           (2, 9) (2, 10)
    NAME       'y'           (2, 10) (2, 11)
    RPAR       ')'           (2, 11) (2, 12)
    COLON      ':'           (2, 12) (2, 13)
    NAME       'pass'        (2, 14) (2, 18)
    """)

    def test_async(self):

        self.check_tokenize('async = 1', """\
    ASYNC      'async'       (1, 0) (1, 5)
    EQUAL      '='           (1, 6) (1, 7)
    NUMBER     '1'           (1, 8) (1, 9)
    """)

        self.check_tokenize('a = (async = 1)', """\
    NAME       'a'           (1, 0) (1, 1)
    EQUAL      '='           (1, 2) (1, 3)
    LPAR       '('           (1, 4) (1, 5)
    ASYNC      'async'       (1, 5) (1, 10)
    EQUAL      '='           (1, 11) (1, 12)
    NUMBER     '1'           (1, 13) (1, 14)
    RPAR       ')'           (1, 14) (1, 15)
    """)

        self.check_tokenize('async()', """\
    ASYNC      'async'       (1, 0) (1, 5)
    LPAR       '('           (1, 5) (1, 6)
    RPAR       ')'           (1, 6) (1, 7)
    """)

        self.check_tokenize('class async(Bar):pass', """\
    NAME       'class'       (1, 0) (1, 5)
    ASYNC      'async'       (1, 6) (1, 11)
    LPAR       '('           (1, 11) (1, 12)
    NAME       'Bar'         (1, 12) (1, 15)
    RPAR       ')'           (1, 15) (1, 16)
    COLON      ':'           (1, 16) (1, 17)
    NAME       'pass'        (1, 17) (1, 21)
    """)

        self.check_tokenize('class async:pass', """\
    NAME       'class'       (1, 0) (1, 5)
    ASYNC      'async'       (1, 6) (1, 11)
    COLON      ':'           (1, 11) (1, 12)
    NAME       'pass'        (1, 12) (1, 16)
    """)

        self.check_tokenize('await = 1', """\
    AWAIT      'await'       (1, 0) (1, 5)
    EQUAL      '='           (1, 6) (1, 7)
    NUMBER     '1'           (1, 8) (1, 9)
    """)

        self.check_tokenize('foo.async', """\
    NAME       'foo'         (1, 0) (1, 3)
    DOT        '.'           (1, 3) (1, 4)
    ASYNC      'async'       (1, 4) (1, 9)
    """)

        self.check_tokenize('async for a in b: pass', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NAME       'for'         (1, 6) (1, 9)
    NAME       'a'           (1, 10) (1, 11)
    NAME       'in'          (1, 12) (1, 14)
    NAME       'b'           (1, 15) (1, 16)
    COLON      ':'           (1, 16) (1, 17)
    NAME       'pass'        (1, 18) (1, 22)
    """)

        self.check_tokenize('async with a as b: pass', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NAME       'with'        (1, 6) (1, 10)
    NAME       'a'           (1, 11) (1, 12)
    NAME       'as'          (1, 13) (1, 15)
    NAME       'b'           (1, 16) (1, 17)
    COLON      ':'           (1, 17) (1, 18)
    NAME       'pass'        (1, 19) (1, 23)
    """)

        self.check_tokenize('async.foo', """\
    ASYNC      'async'       (1, 0) (1, 5)
    DOT        '.'           (1, 5) (1, 6)
    NAME       'foo'         (1, 6) (1, 9)
    """)

        self.check_tokenize('async', """\
    ASYNC      'async'       (1, 0) (1, 5)
    """)

        self.check_tokenize('async\n#comment\nawait', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NEWLINE    ''            (1, 5) (1, 5)
    AWAIT      'await'       (3, 0) (3, 5)
    """)

        self.check_tokenize('async\n...\nawait', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NEWLINE    ''            (1, 5) (1, 5)
    ELLIPSIS   '...'         (2, 0) (2, 3)
    NEWLINE    ''            (2, 3) (2, 3)
    AWAIT      'await'       (3, 0) (3, 5)
    """)

        self.check_tokenize('async\nawait', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NEWLINE    ''            (1, 5) (1, 5)
    AWAIT      'await'       (2, 0) (2, 5)
    """)

        self.check_tokenize('foo.async + 1', """\
    NAME       'foo'         (1, 0) (1, 3)
    DOT        '.'           (1, 3) (1, 4)
    ASYNC      'async'       (1, 4) (1, 9)
    PLUS       '+'           (1, 10) (1, 11)
    NUMBER     '1'           (1, 12) (1, 13)
    """)

        self.check_tokenize('async def foo(): pass', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    LPAR       '('           (1, 13) (1, 14)
    RPAR       ')'           (1, 14) (1, 15)
    COLON      ':'           (1, 15) (1, 16)
    NAME       'pass'        (1, 17) (1, 21)
    """)

        self.check_tokenize('''\
async def foo():
  def foo(await):
    await = 1
  if 1:
    await
async += 1
''', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    LPAR       '('           (1, 13) (1, 14)
    RPAR       ')'           (1, 14) (1, 15)
    COLON      ':'           (1, 15) (1, 16)
    NEWLINE    ''            (1, 16) (1, 16)
    INDENT     ''            (2, -1) (2, -1)
    NAME       'def'         (2, 2) (2, 5)
    NAME       'foo'         (2, 6) (2, 9)
    LPAR       '('           (2, 9) (2, 10)
    AWAIT      'await'       (2, 10) (2, 15)
    RPAR       ')'           (2, 15) (2, 16)
    COLON      ':'           (2, 16) (2, 17)
    NEWLINE    ''            (2, 17) (2, 17)
    INDENT     ''            (3, -1) (3, -1)
    AWAIT      'await'       (3, 4) (3, 9)
    EQUAL      '='           (3, 10) (3, 11)
    NUMBER     '1'           (3, 12) (3, 13)
    NEWLINE    ''            (3, 13) (3, 13)
    DEDENT     ''            (4, -1) (4, -1)
    NAME       'if'          (4, 2) (4, 4)
    NUMBER     '1'           (4, 5) (4, 6)
    COLON      ':'           (4, 6) (4, 7)
    NEWLINE    ''            (4, 7) (4, 7)
    INDENT     ''            (5, -1) (5, -1)
    AWAIT      'await'       (5, 4) (5, 9)
    NEWLINE    ''            (5, 9) (5, 9)
    DEDENT     ''            (6, -1) (6, -1)
    DEDENT     ''            (6, -1) (6, -1)
    ASYNC      'async'       (6, 0) (6, 5)
    PLUSEQUAL  '+='          (6, 6) (6, 8)
    NUMBER     '1'           (6, 9) (6, 10)
    NEWLINE    ''            (6, 10) (6, 10)
    """)

        self.check_tokenize('async def foo():\n  async for i in 1: pass', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    LPAR       '('           (1, 13) (1, 14)
    RPAR       ')'           (1, 14) (1, 15)
    COLON      ':'           (1, 15) (1, 16)
    NEWLINE    ''            (1, 16) (1, 16)
    INDENT     ''            (2, -1) (2, -1)
    ASYNC      'async'       (2, 2) (2, 7)
    NAME       'for'         (2, 8) (2, 11)
    NAME       'i'           (2, 12) (2, 13)
    NAME       'in'          (2, 14) (2, 16)
    NUMBER     '1'           (2, 17) (2, 18)
    COLON      ':'           (2, 18) (2, 19)
    NAME       'pass'        (2, 20) (2, 24)
    DEDENT     ''            (2, -1) (2, -1)
    """)

        self.check_tokenize('async def foo(async): await', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'foo'         (1, 10) (1, 13)
    LPAR       '('           (1, 13) (1, 14)
    ASYNC      'async'       (1, 14) (1, 19)
    RPAR       ')'           (1, 19) (1, 20)
    COLON      ':'           (1, 20) (1, 21)
    AWAIT      'await'       (1, 22) (1, 27)
    """)

        self.check_tokenize('''\
def f():

  def baz(): pass
  async def bar(): pass

  await = 2''', """\
    NAME       'def'         (1, 0) (1, 3)
    NAME       'f'           (1, 4) (1, 5)
    LPAR       '('           (1, 5) (1, 6)
    RPAR       ')'           (1, 6) (1, 7)
    COLON      ':'           (1, 7) (1, 8)
    NEWLINE    ''            (1, 8) (1, 8)
    INDENT     ''            (3, -1) (3, -1)
    NAME       'def'         (3, 2) (3, 5)
    NAME       'baz'         (3, 6) (3, 9)
    LPAR       '('           (3, 9) (3, 10)
    RPAR       ')'           (3, 10) (3, 11)
    COLON      ':'           (3, 11) (3, 12)
    NAME       'pass'        (3, 13) (3, 17)
    NEWLINE    ''            (3, 17) (3, 17)
    ASYNC      'async'       (4, 2) (4, 7)
    NAME       'def'         (4, 8) (4, 11)
    NAME       'bar'         (4, 12) (4, 15)
    LPAR       '('           (4, 15) (4, 16)
    RPAR       ')'           (4, 16) (4, 17)
    COLON      ':'           (4, 17) (4, 18)
    NAME       'pass'        (4, 19) (4, 23)
    NEWLINE    ''            (4, 23) (4, 23)
    AWAIT      'await'       (6, 2) (6, 7)
    EQUAL      '='           (6, 8) (6, 9)
    NUMBER     '2'           (6, 10) (6, 11)
    DEDENT     ''            (6, -1) (6, -1)
    """)

        self.check_tokenize('''\
async def f():

  def baz(): pass
  async def bar(): pass

  await = 2''', """\
    ASYNC      'async'       (1, 0) (1, 5)
    NAME       'def'         (1, 6) (1, 9)
    NAME       'f'           (1, 10) (1, 11)
    LPAR       '('           (1, 11) (1, 12)
    RPAR       ')'           (1, 12) (1, 13)
    COLON      ':'           (1, 13) (1, 14)
    NEWLINE    ''            (1, 14) (1, 14)
    INDENT     ''            (3, -1) (3, -1)
    NAME       'def'         (3, 2) (3, 5)
    NAME       'baz'         (3, 6) (3, 9)
    LPAR       '('           (3, 9) (3, 10)
    RPAR       ')'           (3, 10) (3, 11)
    COLON      ':'           (3, 11) (3, 12)
    NAME       'pass'        (3, 13) (3, 17)
    NEWLINE    ''            (3, 17) (3, 17)
    ASYNC      'async'       (4, 2) (4, 7)
    NAME       'def'         (4, 8) (4, 11)
    NAME       'bar'         (4, 12) (4, 15)
    LPAR       '('           (4, 15) (4, 16)
    RPAR       ')'           (4, 16) (4, 17)
    COLON      ':'           (4, 17) (4, 18)
    NAME       'pass'        (4, 19) (4, 23)
    NEWLINE    ''            (4, 23) (4, 23)
    AWAIT      'await'       (6, 2) (6, 7)
    EQUAL      '='           (6, 8) (6, 9)
    NUMBER     '2'           (6, 10) (6, 11)
    DEDENT     ''            (6, -1) (6, -1)
    """)

    def test_unicode(self):

        self.check_tokenize("Örter = u'places'\ngrün = U'green'", """\
    NAME       'Örter'       (1, 0) (1, 5)
    EQUAL      '='           (1, 6) (1, 7)
    STRING     "u'places'"   (1, 8) (1, 17)
    NEWLINE    ''            (1, 17) (1, 17)
    NAME       'grün'        (2, 0) (2, 4)
    EQUAL      '='           (2, 5) (2, 6)
    STRING     "U'green'"    (2, 7) (2, 15)
    """)

    def test_invalid_syntax(self):
        def get_tokens(string):
            the_string = StringIO(string)
            return list(_generate_tokens_from_c_tokenizer(the_string.readline))

        for case in [
            "(1+2]",
            "(1+2}",
            "{1+2]",
            "1_",
            "1.2_",
            "1e2_",
            "1e+",

            "\xa0",
            "€",
            "0b12",
            "0b1_2",
            "0b2",
            "0b1_",
            "0b",
            "0o18",
            "0o1_8",
            "0o8",
            "0o1_",
            "0o",
            "0x1_",
            "0x",
            "1_",
            "012",
            "1.2_",
            "1e2_",
            "1e+",
            "'sdfsdf",
            "'''sdfsdf''",
            "("*1000+"a"+")"*1000,
            "]",
        ]:
            with self.subTest(case=case):
                self.assertRaises(TokenError, get_tokens, case)

    def test_max_indent(self):
        MAXINDENT = 100

        def generate_source(indents):
            source = ''.join(('  ' * x) + 'if True:\n' for x in range(indents))
            source += '  ' * indents + 'pass\n'
            return source

        valid = generate_source(MAXINDENT - 1)
        the_input = StringIO(valid)
        tokens = list(_generate_tokens_from_c_tokenizer(the_input.readline))
        self.assertEqual(tokens[-2].type, DEDENT)
        self.assertEqual(tokens[-1].type, ENDMARKER)
        compile(valid, "<string>", "exec")

        invalid = generate_source(MAXINDENT)
        the_input = StringIO(invalid)
        self.assertRaises(IndentationError, lambda: list(_generate_tokens_from_c_tokenizer(the_input.readline)))
        self.assertRaises(
            IndentationError, compile, invalid, "<string>", "exec"
        )

    def test_continuation_lines_indentation(self):
        def get_tokens(string):
            the_string = StringIO(string)
            return [(kind, string) for (kind, string, *_)
                    in _generate_tokens_from_c_tokenizer(the_string.readline)]

        code = dedent("""
            def fib(n):
                \\
            '''Print a Fibonacci series up to n.'''
                \\
            a, b = 0, 1
        """)

        self.check_tokenize(code, """\
    NAME       'def'         (2, 0) (2, 3)
    NAME       'fib'         (2, 4) (2, 7)
    LPAR       '('           (2, 7) (2, 8)
    NAME       'n'           (2, 8) (2, 9)
    RPAR       ')'           (2, 9) (2, 10)
    COLON      ':'           (2, 10) (2, 11)
    NEWLINE    ''            (2, 11) (2, 11)
    INDENT     ''            (4, -1) (4, -1)
    STRING     "'''Print a Fibonacci series up to n.'''" (4, 0) (4, 39)
    NEWLINE    ''            (4, 39) (4, 39)
    NAME       'a'           (6, 0) (6, 1)
    COMMA      ','           (6, 1) (6, 2)
    NAME       'b'           (6, 3) (6, 4)
    EQUAL      '='           (6, 5) (6, 6)
    NUMBER     '0'           (6, 7) (6, 8)
    COMMA      ','           (6, 8) (6, 9)
    NUMBER     '1'           (6, 10) (6, 11)
    NEWLINE    ''            (6, 11) (6, 11)
    DEDENT     ''            (6, -1) (6, -1)
        """)

        code_no_cont = dedent("""
            def fib(n):
                '''Print a Fibonacci series up to n.'''
                a, b = 0, 1
        """)

        self.assertEqual(get_tokens(code), get_tokens(code_no_cont))

        code = dedent("""
            pass
                \\

            pass
        """)

        self.check_tokenize(code, """\
    NAME       'pass'        (2, 0) (2, 4)
    NEWLINE    ''            (2, 4) (2, 4)
    NAME       'pass'        (5, 0) (5, 4)
    NEWLINE    ''            (5, 4) (5, 4)
        """)

        code_no_cont = dedent("""
            pass
            pass
        """)

        self.assertEqual(get_tokens(code), get_tokens(code_no_cont))

        code = dedent("""
            if x:
                y = 1
                \\
                        \\
                    \\
                \\
                foo = 1
        """)

        self.check_tokenize(code, """\
    NAME       'if'          (2, 0) (2, 2)
    NAME       'x'           (2, 3) (2, 4)
    COLON      ':'           (2, 4) (2, 5)
    NEWLINE    ''            (2, 5) (2, 5)
    INDENT     ''            (3, -1) (3, -1)
    NAME       'y'           (3, 4) (3, 5)
    EQUAL      '='           (3, 6) (3, 7)
    NUMBER     '1'           (3, 8) (3, 9)
    NEWLINE    ''            (3, 9) (3, 9)
    NAME       'foo'         (8, 4) (8, 7)
    EQUAL      '='           (8, 8) (8, 9)
    NUMBER     '1'           (8, 10) (8, 11)
    NEWLINE    ''            (8, 11) (8, 11)
    DEDENT     ''            (8, -1) (8, -1)
        """)

        code_no_cont = dedent("""
            if x:
                y = 1
                foo = 1
        """)

        self.assertEqual(get_tokens(code), get_tokens(code_no_cont))


class CTokenizerBufferTests(unittest.TestCase):
    def test_newline_at_the_end_of_buffer(self):
        # See issue 99581: Make sure that if we need to add a new line at the
        # end of the buffer, we have enough space in the buffer, specially when
        # the current line is as long as the buffer space available.
        test_script = f"""\
        #coding: latin-1
        #{"a"*10000}
        #{"a"*10002}"""
        with os_helper.temp_dir() as temp_dir:
            file_name = make_script(temp_dir, 'foo', test_script)
            run_test_script(file_name)


if __name__ == "__main__":
    unittest.main()
