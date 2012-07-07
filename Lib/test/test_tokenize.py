doctests = """
Tests for the tokenize module.

The tests can be really simple. Given a small fragment of source
code, print out a table with tokens. The ENDMARK is omitted for
brevity.

    >>> dump_tokens("1 + 1")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '1'           (1, 0) (1, 1)
    OP         '+'           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)

    >>> dump_tokens("if False:\\n"
    ...             "    # NL\\n"
    ...             "    True = False # NEWLINE\\n")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'if'          (1, 0) (1, 2)
    NAME       'False'       (1, 3) (1, 8)
    OP         ':'           (1, 8) (1, 9)
    NEWLINE    '\\n'          (1, 9) (1, 10)
    COMMENT    '# NL'        (2, 4) (2, 8)
    NL         '\\n'          (2, 8) (2, 9)
    INDENT     '    '        (3, 0) (3, 4)
    NAME       'True'        (3, 4) (3, 8)
    OP         '='           (3, 9) (3, 10)
    NAME       'False'       (3, 11) (3, 16)
    COMMENT    '# NEWLINE'   (3, 17) (3, 26)
    NEWLINE    '\\n'          (3, 26) (3, 27)
    DEDENT     ''            (4, 0) (4, 0)

    >>> indent_error_file = \"""
    ... def k(x):
    ...     x += 2
    ...   x += 5
    ... \"""
    >>> readline = BytesIO(indent_error_file.encode('utf-8')).readline
    >>> for tok in tokenize(readline): pass
    Traceback (most recent call last):
        ...
    IndentationError: unindent does not match any outer indentation level

There are some standard formatting practices that are easy to get right.

    >>> roundtrip("if x == 1:\\n"
    ...           "    print(x)\\n")
    True

    >>> roundtrip("# This is a comment\\n# This also")
    True

Some people use different formatting conventions, which makes
untokenize a little trickier. Note that this test involves trailing
whitespace after the colon. Note that we use hex escapes to make the
two trailing blanks apparent in the expected output.

    >>> roundtrip("if x == 1 : \\n"
    ...           "  print(x)\\n")
    True

    >>> f = support.findfile("tokenize_tests.txt")
    >>> roundtrip(open(f, 'rb'))
    True

    >>> roundtrip("if x == 1:\\n"
    ...           "    # A comment by itself.\\n"
    ...           "    print(x) # Comment here, too.\\n"
    ...           "    # Another comment.\\n"
    ...           "after_if = True\\n")
    True

    >>> roundtrip("if (x # The comments need to go in the right place\\n"
    ...           "    == 1):\\n"
    ...           "    print('x==1')\\n")
    True

    >>> roundtrip("class Test: # A comment here\\n"
    ...           "  # A comment with weird indent\\n"
    ...           "  after_com = 5\\n"
    ...           "  def x(m): return m*5 # a one liner\\n"
    ...           "  def y(m): # A whitespace after the colon\\n"
    ...           "     return y*4 # 3-space indent\\n")
    True

Some error-handling code

    >>> roundtrip("try: import somemodule\\n"
    ...           "except ImportError: # comment\\n"
    ...           "    print('Can not import' # comment2\\n)"
    ...           "else:   print('Loaded')\\n")
    True

Balancing continuation

    >>> roundtrip("a = (3,4, \\n"
    ...           "5,6)\\n"
    ...           "y = [3, 4,\\n"
    ...           "5]\\n"
    ...           "z = {'a': 5,\\n"
    ...           "'b':15, 'c':True}\\n"
    ...           "x = len(y) + 5 - a[\\n"
    ...           "3] - a[2]\\n"
    ...           "+ len(z) - z[\\n"
    ...           "'b']\\n")
    True

Ordinary integers and binary operators

    >>> dump_tokens("0xff <= 255")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '0xff'        (1, 0) (1, 4)
    OP         '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    >>> dump_tokens("0b10 <= 255")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '0b10'        (1, 0) (1, 4)
    OP         '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    >>> dump_tokens("0o123 <= 0O123")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '0o123'       (1, 0) (1, 5)
    OP         '<='          (1, 6) (1, 8)
    NUMBER     '0O123'       (1, 9) (1, 14)
    >>> dump_tokens("1234567 > ~0x15")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '1234567'     (1, 0) (1, 7)
    OP         '>'           (1, 8) (1, 9)
    OP         '~'           (1, 10) (1, 11)
    NUMBER     '0x15'        (1, 11) (1, 15)
    >>> dump_tokens("2134568 != 1231515")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '2134568'     (1, 0) (1, 7)
    OP         '!='          (1, 8) (1, 10)
    NUMBER     '1231515'     (1, 11) (1, 18)
    >>> dump_tokens("(-124561-1) & 200000000")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    OP         '('           (1, 0) (1, 1)
    OP         '-'           (1, 1) (1, 2)
    NUMBER     '124561'      (1, 2) (1, 8)
    OP         '-'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         ')'           (1, 10) (1, 11)
    OP         '&'           (1, 12) (1, 13)
    NUMBER     '200000000'   (1, 14) (1, 23)
    >>> dump_tokens("0xdeadbeef != -1")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '0xdeadbeef'  (1, 0) (1, 10)
    OP         '!='          (1, 11) (1, 13)
    OP         '-'           (1, 14) (1, 15)
    NUMBER     '1'           (1, 15) (1, 16)
    >>> dump_tokens("0xdeadc0de & 12345")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '0xdeadc0de'  (1, 0) (1, 10)
    OP         '&'           (1, 11) (1, 12)
    NUMBER     '12345'       (1, 13) (1, 18)
    >>> dump_tokens("0xFF & 0x15 | 1234")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NUMBER     '0xFF'        (1, 0) (1, 4)
    OP         '&'           (1, 5) (1, 6)
    NUMBER     '0x15'        (1, 7) (1, 11)
    OP         '|'           (1, 12) (1, 13)
    NUMBER     '1234'        (1, 14) (1, 18)

Long integers

    >>> dump_tokens("x = 0")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '0'           (1, 4) (1, 5)
    >>> dump_tokens("x = 0xfffffffffff")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '0xffffffffff (1, 4) (1, 17)
    >>> dump_tokens("x = 123141242151251616110")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '123141242151 (1, 4) (1, 25)
    >>> dump_tokens("x = -15921590215012591")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    OP         '-'           (1, 4) (1, 5)
    NUMBER     '159215902150 (1, 5) (1, 22)

Floating point numbers

    >>> dump_tokens("x = 3.14159")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3.14159'     (1, 4) (1, 11)
    >>> dump_tokens("x = 314159.")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '314159.'     (1, 4) (1, 11)
    >>> dump_tokens("x = .314159")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '.314159'     (1, 4) (1, 11)
    >>> dump_tokens("x = 3e14159")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3e14159'     (1, 4) (1, 11)
    >>> dump_tokens("x = 3E123")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3E123'       (1, 4) (1, 9)
    >>> dump_tokens("x+y = 3e-1230")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '+'           (1, 1) (1, 2)
    NAME       'y'           (1, 2) (1, 3)
    OP         '='           (1, 4) (1, 5)
    NUMBER     '3e-1230'     (1, 6) (1, 13)
    >>> dump_tokens("x = 3.14e159")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3.14e159'    (1, 4) (1, 12)

String literals

    >>> dump_tokens("x = ''; y = \\\"\\\"")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "''"          (1, 4) (1, 6)
    OP         ';'           (1, 6) (1, 7)
    NAME       'y'           (1, 8) (1, 9)
    OP         '='           (1, 10) (1, 11)
    STRING     '""'          (1, 12) (1, 14)
    >>> dump_tokens("x = '\\\"'; y = \\\"'\\\"")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '\\'"\\''       (1, 4) (1, 7)
    OP         ';'           (1, 7) (1, 8)
    NAME       'y'           (1, 9) (1, 10)
    OP         '='           (1, 11) (1, 12)
    STRING     '"\\'"'        (1, 13) (1, 16)
    >>> dump_tokens("x = \\\"doesn't \\\"shrink\\\", does it\\\"")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '"doesn\\'t "' (1, 4) (1, 14)
    NAME       'shrink'      (1, 14) (1, 20)
    STRING     '", does it"' (1, 20) (1, 31)
    >>> dump_tokens("x = 'abc' + 'ABC'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "'abc'"       (1, 4) (1, 9)
    OP         '+'           (1, 10) (1, 11)
    STRING     "'ABC'"       (1, 12) (1, 17)
    >>> dump_tokens('y = "ABC" + "ABC"')
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'y'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '"ABC"'       (1, 4) (1, 9)
    OP         '+'           (1, 10) (1, 11)
    STRING     '"ABC"'       (1, 12) (1, 17)
    >>> dump_tokens("x = r'abc' + r'ABC' + R'ABC' + R'ABC'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "r'abc'"      (1, 4) (1, 10)
    OP         '+'           (1, 11) (1, 12)
    STRING     "r'ABC'"      (1, 13) (1, 19)
    OP         '+'           (1, 20) (1, 21)
    STRING     "R'ABC'"      (1, 22) (1, 28)
    OP         '+'           (1, 29) (1, 30)
    STRING     "R'ABC'"      (1, 31) (1, 37)
    >>> dump_tokens('y = r"abc" + r"ABC" + R"ABC" + R"ABC"')
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'y'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     'r"abc"'      (1, 4) (1, 10)
    OP         '+'           (1, 11) (1, 12)
    STRING     'r"ABC"'      (1, 13) (1, 19)
    OP         '+'           (1, 20) (1, 21)
    STRING     'R"ABC"'      (1, 22) (1, 28)
    OP         '+'           (1, 29) (1, 30)
    STRING     'R"ABC"'      (1, 31) (1, 37)

    >>> dump_tokens("u'abc' + U'abc'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     "u'abc'"      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     "U'abc'"      (1, 9) (1, 15)
    >>> dump_tokens('u"abc" + U"abc"')
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     'u"abc"'      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     'U"abc"'      (1, 9) (1, 15)

    >>> dump_tokens("b'abc' + B'abc'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     "b'abc'"      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     "B'abc'"      (1, 9) (1, 15)
    >>> dump_tokens('b"abc" + B"abc"')
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     'b"abc"'      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     'B"abc"'      (1, 9) (1, 15)
    >>> dump_tokens("br'abc' + bR'abc' + Br'abc' + BR'abc'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     "br'abc'"     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     "bR'abc'"     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     "Br'abc'"     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     "BR'abc'"     (1, 30) (1, 37)
    >>> dump_tokens('br"abc" + bR"abc" + Br"abc" + BR"abc"')
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     'br"abc"'     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     'bR"abc"'     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     'Br"abc"'     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     'BR"abc"'     (1, 30) (1, 37)
    >>> dump_tokens("rb'abc' + rB'abc' + Rb'abc' + RB'abc'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     "rb'abc'"     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     "rB'abc'"     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     "Rb'abc'"     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     "RB'abc'"     (1, 30) (1, 37)
    >>> dump_tokens('rb"abc" + rB"abc" + Rb"abc" + RB"abc"')
    ENCODING   'utf-8'       (0, 0) (0, 0)
    STRING     'rb"abc"'     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     'rB"abc"'     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     'Rb"abc"'     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     'RB"abc"'     (1, 30) (1, 37)

Operators

    >>> dump_tokens("def d22(a, b, c=2, d=2, *k): pass")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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
    >>> dump_tokens("def d01v_(a=1, *k, **w): pass")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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

Comparison

    >>> dump_tokens("if 1 < 1 > 1 == 1 >= 5 <= 0x15 <= 0x12 != " +
    ...             "1 and 5 in 1 not in 1 is 1 or 5 is not 1: pass")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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

Shift

    >>> dump_tokens("x = 1 << 1 >> 5")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    OP         '<<'          (1, 6) (1, 8)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         '>>'          (1, 11) (1, 13)
    NUMBER     '5'           (1, 14) (1, 15)

Additive

    >>> dump_tokens("x = 1 - y + 15 - 1 + 0x124 + z + a[5]")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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

Multiplicative

    >>> dump_tokens("x = 1//1*1/5*12%0x12")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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

Unary

    >>> dump_tokens("~1 ^ 1 & 1 |1 ^ -1")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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
    >>> dump_tokens("-1*1/1+1*1//1 - ---1**1")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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

Selector

    >>> dump_tokens("import sys, time\\nx = sys.modules['time'].time()")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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

Methods

    >>> dump_tokens("@staticmethod\\ndef foo(x,y): pass")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    OP         '@'           (1, 0) (1, 1)
    NAME       'staticmethod (1, 1) (1, 13)
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

Backslash means line continuation, except for comments

    >>> roundtrip("x=1+\\\\n"
    ...           "1\\n"
    ...           "# This is a comment\\\\n"
    ...           "# This also\\n")
    True
    >>> roundtrip("# Comment \\\\nx = 0")
    True

Two string literals on the same line

    >>> roundtrip("'' ''")
    True

Test roundtrip on random python modules.
pass the '-ucpu' option to process the full directory.

    >>> import random
    >>> tempdir = os.path.dirname(f) or os.curdir
    >>> testfiles = glob.glob(os.path.join(tempdir, "test*.py"))

tokenize is broken on test_pep3131.py because regular expressions are broken on
the obscure unicode identifiers in it. *sigh*
    >>> testfiles.remove(os.path.join(tempdir, "test_pep3131.py"))
    >>> if not support.is_resource_enabled("cpu"):
    ...     testfiles = random.sample(testfiles, 10)
    ...
    >>> for testfile in testfiles:
    ...     if not roundtrip(open(testfile, 'rb')):
    ...         print("Roundtrip failed for file %s" % testfile)
    ...         break
    ... else: True
    True

Evil tabs

    >>> dump_tokens("def f():\\n\\tif x\\n        \\tpass")
    ENCODING   'utf-8'       (0, 0) (0, 0)
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

Non-ascii identifiers

    >>> dump_tokens("Örter = 'places'\\ngrün = 'green'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'Örter'       (1, 0) (1, 5)
    OP         '='           (1, 6) (1, 7)
    STRING     "'places'"    (1, 8) (1, 16)
    NEWLINE    '\\n'          (1, 16) (1, 17)
    NAME       'grün'        (2, 0) (2, 4)
    OP         '='           (2, 5) (2, 6)
    STRING     "'green'"     (2, 7) (2, 14)

Legacy unicode literals:

    >>> dump_tokens("Örter = u'places'\\ngrün = U'green'")
    ENCODING   'utf-8'       (0, 0) (0, 0)
    NAME       'Örter'       (1, 0) (1, 5)
    OP         '='           (1, 6) (1, 7)
    STRING     "u'places'"   (1, 8) (1, 17)
    NEWLINE    '\\n'          (1, 17) (1, 18)
    NAME       'grün'        (2, 0) (2, 4)
    OP         '='           (2, 5) (2, 6)
    STRING     "U'green'"    (2, 7) (2, 15)
"""

from test import support
from tokenize import (tokenize, _tokenize, untokenize, NUMBER, NAME, OP,
                     STRING, ENDMARKER, ENCODING, tok_name, detect_encoding,
                     open as tokenize_open)
from io import BytesIO
from unittest import TestCase
import os, sys, glob
import token

def dump_tokens(s):
    """Print out the tokens in s in a table format.

    The ENDMARKER is omitted.
    """
    f = BytesIO(s.encode('utf-8'))
    for type, token, start, end, line in tokenize(f.readline):
        if type == ENDMARKER:
            break
        type = tok_name[type]
        print("%(type)-10.10s %(token)-13.13r %(start)s %(end)s" % locals())

def roundtrip(f):
    """
    Test roundtrip for `untokenize`. `f` is an open file or a string.
    The source code in f is tokenized, converted back to source code via
    tokenize.untokenize(), and tokenized again from the latter. The test
    fails if the second tokenization doesn't match the first.
    """
    if isinstance(f, str):
        f = BytesIO(f.encode('utf-8'))
    try:
        token_list = list(tokenize(f.readline))
    finally:
        f.close()
    tokens1 = [tok[:2] for tok in token_list]
    new_bytes = untokenize(tokens1)
    readline = (line for line in new_bytes.splitlines(keepends=True)).__next__
    tokens2 = [tok[:2] for tok in tokenize(readline)]
    return tokens1 == tokens2

# This is an example from the docs, set up as a doctest.
def decistmt(s):
    """Substitute Decimals for floats in a string of statements.

    >>> from decimal import Decimal
    >>> s = 'print(+21.3e-5*-.1234/81.7)'
    >>> decistmt(s)
    "print (+Decimal ('21.3e-5')*-Decimal ('.1234')/Decimal ('81.7'))"

    The format of the exponent is inherited from the platform C library.
    Known cases are "e-007" (Windows) and "e-07" (not Windows).  Since
    we're only showing 11 digits, and the 12th isn't close to 5, the
    rest of the output should be platform-independent.

    >>> exec(s) #doctest: +ELLIPSIS
    -3.2171603427...e-0...7

    Output from calculations with Decimal should be identical across all
    platforms.

    >>> exec(decistmt(s))
    -3.217160342717258261933904529E-7
    """
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
    return untokenize(result).decode('utf-8')


class TestTokenizerAdheresToPep0263(TestCase):
    """
    Test that tokenizer adheres to the coding behaviour stipulated in PEP 0263.
    """

    def _testFile(self, filename):
        path = os.path.join(os.path.dirname(__file__), filename)
        return roundtrip(open(path, 'rb'))

    def test_utf8_coding_cookie_and_no_utf8_bom(self):
        f = 'tokenize_tests-utf8-coding-cookie-and-no-utf8-bom-sig.txt'
        self.assertTrue(self._testFile(f))

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
        self.assertTrue(self._testFile(f))

    def test_utf8_coding_cookie_and_utf8_bom(self):
        f = 'tokenize_tests-utf8-coding-cookie-and-utf8-bom-sig.txt'
        self.assertTrue(self._testFile(f))

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
                return line
            else:
                return b''

        # skip the initial encoding token and the end token
        tokens = list(_tokenize(readline, encoding='utf-8'))[1:-1]
        expected_tokens = [(3, '"ЉЊЈЁЂ"', (1, 0), (1, 7), '"ЉЊЈЁЂ"')]
        self.assertEqual(tokens, expected_tokens,
                         "bytes not decoded with encoding")

    def test__tokenize_does_not_decode_with_encoding_none(self):
        literal = '"ЉЊЈЁЂ"'
        first = False
        def readline():
            nonlocal first
            if not first:
                first = True
                return literal
            else:
                return b''

        # skip the end token
        tokens = list(_tokenize(readline, encoding=None))[:-1]
        expected_tokens = [(3, '"ЉЊЈЁЂ"', (1, 0), (1, 7), '"ЉЊЈЁЂ"')]
        self.assertEqual(tokens, expected_tokens,
                         "string not tokenized when encoding is None")


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
        # Issue 14629: need to raise SyntaxError if the first
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

    def test_open(self):
        filename = support.TESTFN + '.py'
        self.addCleanup(support.unlink, filename)

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


class TestTokenize(TestCase):

    def test_tokenize(self):
        import tokenize as tokenize_module
        encoding = object()
        encoding_used = None
        def mock_detect_encoding(readline):
            return encoding, ['first', 'second']

        def mock__tokenize(readline, encoding):
            nonlocal encoding_used
            encoding_used = encoding
            out = []
            while True:
                next_line = readline()
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
            return counter

        orig_detect_encoding = tokenize_module.detect_encoding
        orig__tokenize = tokenize_module._tokenize
        tokenize_module.detect_encoding = mock_detect_encoding
        tokenize_module._tokenize = mock__tokenize
        try:
            results = tokenize(mock_readline)
            self.assertEqual(list(results), ['first', 'second', 1, 2, 3, 4])
        finally:
            tokenize_module.detect_encoding = orig_detect_encoding
            tokenize_module._tokenize = orig__tokenize

        self.assertTrue(encoding_used, encoding)

    def assertExactTypeEqual(self, opstr, *optypes):
        tokens = list(tokenize(BytesIO(opstr.encode('utf-8')).readline))
        num_optypes = len(optypes)
        self.assertEqual(len(tokens), 2 + num_optypes)
        self.assertEqual(token.tok_name[tokens[0].exact_type],
                         token.tok_name[ENCODING])
        for i in range(num_optypes):
            self.assertEqual(token.tok_name[tokens[i + 1].exact_type],
                             token.tok_name[optypes[i]])
        self.assertEqual(token.tok_name[tokens[1 + num_optypes].exact_type],
                         token.tok_name[token.ENDMARKER])

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
        self.assertExactTypeEqual('@', token.AT)

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

__test__ = {"doctests" : doctests, 'decistmt': decistmt}

def test_main():
    from test import test_tokenize
    support.run_doctest(test_tokenize, True)
    support.run_unittest(TestTokenizerAdheresToPep0263)
    support.run_unittest(Test_Tokenize)
    support.run_unittest(TestDetectEncoding)
    support.run_unittest(TestTokenize)

if __name__ == "__main__":
    test_main()
