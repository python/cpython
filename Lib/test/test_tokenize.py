doctests = """
Tests for the tokenize module.

    >>> import glob, random, sys

The tests can be really simple. Given a small fragment of source
code, print out a table with tokens. The ENDMARK is omitted for
brevity.

    >>> dump_tokens("1 + 1")
    NUMBER     '1'           (1, 0) (1, 1)
    OP         '+'           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)

    >>> dump_tokens("if False:\\n"
    ...             "    # NL\\n"
    ...             "    True = False # NEWLINE\\n")
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

    >>> for tok in generate_tokens(StringIO(indent_error_file).readline): pass
    Traceback (most recent call last):
        ...
    IndentationError: unindent does not match any outer indentation level

Test roundtrip for `untokenize`. `f` is an open file or a string. The source
code in f is tokenized, converted back to source code via tokenize.untokenize(),
and tokenized again from the latter. The test fails if the second tokenization
doesn't match the first.

    >>> def roundtrip(f):
    ...     if isinstance(f, str): f = StringIO(f)
    ...     token_list = list(generate_tokens(f.readline))
    ...     f.close()
    ...     tokens1 = [tok[:2] for tok in token_list]
    ...     new_text = untokenize(tokens1)
    ...     readline = iter(new_text.splitlines(1)).next
    ...     tokens2 = [tok[:2] for tok in generate_tokens(readline)]
    ...     return tokens1 == tokens2
    ...

There are some standard formatting practices that are easy to get right.

    >>> roundtrip("if x == 1:\\n"
    ...           "    print x\\n")
    True

    >>> roundtrip("# This is a comment\\n# This also")
    True

Some people use different formatting conventions, which makes
untokenize a little trickier. Note that this test involves trailing
whitespace after the colon. Note that we use hex escapes to make the
two trailing blanks apperant in the expected output.

    >>> roundtrip("if x == 1 : \\n"
    ...           "  print x\\n")
    True

    >>> f = test_support.findfile("tokenize_tests" + os.extsep + "txt")
    >>> roundtrip(open(f))
    True

    >>> roundtrip("if x == 1:\\n"
    ...           "    # A comment by itself.\\n"
    ...           "    print x # Comment here, too.\\n"
    ...           "    # Another comment.\\n"
    ...           "after_if = True\\n")
    True

    >>> roundtrip("if (x # The comments need to go in the right place\\n"
    ...           "    == 1):\\n"
    ...           "    print 'x==1'\\n")
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
    ...           "    print 'Can not import' # comment2\\n"
    ...           "else:   print 'Loaded'\\n")
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
    NUMBER     '0xff'        (1, 0) (1, 4)
    OP         '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    >>> dump_tokens("0b10 <= 255")
    NUMBER     '0b10'        (1, 0) (1, 4)
    OP         '<='          (1, 5) (1, 7)
    NUMBER     '255'         (1, 8) (1, 11)
    >>> dump_tokens("0o123 <= 0123")
    NUMBER     '0o123'       (1, 0) (1, 5)
    OP         '<='          (1, 6) (1, 8)
    NUMBER     '0123'        (1, 9) (1, 13)
    >>> dump_tokens("01234567 > ~0x15")
    NUMBER     '01234567'    (1, 0) (1, 8)
    OP         '>'           (1, 9) (1, 10)
    OP         '~'           (1, 11) (1, 12)
    NUMBER     '0x15'        (1, 12) (1, 16)
    >>> dump_tokens("2134568 != 01231515")
    NUMBER     '2134568'     (1, 0) (1, 7)
    OP         '!='          (1, 8) (1, 10)
    NUMBER     '01231515'    (1, 11) (1, 19)
    >>> dump_tokens("(-124561-1) & 0200000000")
    OP         '('           (1, 0) (1, 1)
    OP         '-'           (1, 1) (1, 2)
    NUMBER     '124561'      (1, 2) (1, 8)
    OP         '-'           (1, 8) (1, 9)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         ')'           (1, 10) (1, 11)
    OP         '&'           (1, 12) (1, 13)
    NUMBER     '0200000000'  (1, 14) (1, 24)
    >>> dump_tokens("0xdeadbeef != -1")
    NUMBER     '0xdeadbeef'  (1, 0) (1, 10)
    OP         '!='          (1, 11) (1, 13)
    OP         '-'           (1, 14) (1, 15)
    NUMBER     '1'           (1, 15) (1, 16)
    >>> dump_tokens("0xdeadc0de & 012345")
    NUMBER     '0xdeadc0de'  (1, 0) (1, 10)
    OP         '&'           (1, 11) (1, 12)
    NUMBER     '012345'      (1, 13) (1, 19)
    >>> dump_tokens("0xFF & 0x15 | 1234")
    NUMBER     '0xFF'        (1, 0) (1, 4)
    OP         '&'           (1, 5) (1, 6)
    NUMBER     '0x15'        (1, 7) (1, 11)
    OP         '|'           (1, 12) (1, 13)
    NUMBER     '1234'        (1, 14) (1, 18)

Long integers

    >>> dump_tokens("x = 0L")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '0L'          (1, 4) (1, 6)
    >>> dump_tokens("x = 0xfffffffffff")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '0xffffffffff (1, 4) (1, 17)
    >>> dump_tokens("x = 123141242151251616110l")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '123141242151 (1, 4) (1, 26)
    >>> dump_tokens("x = -15921590215012591L")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    OP         '-'           (1, 4) (1, 5)
    NUMBER     '159215902150 (1, 5) (1, 23)

Floating point numbers

    >>> dump_tokens("x = 3.14159")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3.14159'     (1, 4) (1, 11)
    >>> dump_tokens("x = 314159.")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '314159.'     (1, 4) (1, 11)
    >>> dump_tokens("x = .314159")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '.314159'     (1, 4) (1, 11)
    >>> dump_tokens("x = 3e14159")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3e14159'     (1, 4) (1, 11)
    >>> dump_tokens("x = 3E123")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3E123'       (1, 4) (1, 9)
    >>> dump_tokens("x+y = 3e-1230")
    NAME       'x'           (1, 0) (1, 1)
    OP         '+'           (1, 1) (1, 2)
    NAME       'y'           (1, 2) (1, 3)
    OP         '='           (1, 4) (1, 5)
    NUMBER     '3e-1230'     (1, 6) (1, 13)
    >>> dump_tokens("x = 3.14e159")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '3.14e159'    (1, 4) (1, 12)

String literals

    >>> dump_tokens("x = ''; y = \\\"\\\"")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "''"          (1, 4) (1, 6)
    OP         ';'           (1, 6) (1, 7)
    NAME       'y'           (1, 8) (1, 9)
    OP         '='           (1, 10) (1, 11)
    STRING     '""'          (1, 12) (1, 14)
    >>> dump_tokens("x = '\\\"'; y = \\\"'\\\"")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '\\'"\\''       (1, 4) (1, 7)
    OP         ';'           (1, 7) (1, 8)
    NAME       'y'           (1, 9) (1, 10)
    OP         '='           (1, 11) (1, 12)
    STRING     '"\\'"'        (1, 13) (1, 16)
    >>> dump_tokens("x = \\\"doesn't \\\"shrink\\\", does it\\\"")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     '"doesn\\'t "' (1, 4) (1, 14)
    NAME       'shrink'      (1, 14) (1, 20)
    STRING     '", does it"' (1, 20) (1, 31)
    >>> dump_tokens("x = u'abc' + U'ABC'")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "u'abc'"      (1, 4) (1, 10)
    OP         '+'           (1, 11) (1, 12)
    STRING     "U'ABC'"      (1, 13) (1, 19)
    >>> dump_tokens('y = u"ABC" + U"ABC"')
    NAME       'y'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     'u"ABC"'      (1, 4) (1, 10)
    OP         '+'           (1, 11) (1, 12)
    STRING     'U"ABC"'      (1, 13) (1, 19)
    >>> dump_tokens("x = ur'abc' + Ur'ABC' + uR'ABC' + UR'ABC'")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     "ur'abc'"     (1, 4) (1, 11)
    OP         '+'           (1, 12) (1, 13)
    STRING     "Ur'ABC'"     (1, 14) (1, 21)
    OP         '+'           (1, 22) (1, 23)
    STRING     "uR'ABC'"     (1, 24) (1, 31)
    OP         '+'           (1, 32) (1, 33)
    STRING     "UR'ABC'"     (1, 34) (1, 41)
    >>> dump_tokens('y = ur"abc" + Ur"ABC" + uR"ABC" + UR"ABC"')
    NAME       'y'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    STRING     'ur"abc"'     (1, 4) (1, 11)
    OP         '+'           (1, 12) (1, 13)
    STRING     'Ur"ABC"'     (1, 14) (1, 21)
    OP         '+'           (1, 22) (1, 23)
    STRING     'uR"ABC"'     (1, 24) (1, 31)
    OP         '+'           (1, 32) (1, 33)
    STRING     'UR"ABC"'     (1, 34) (1, 41)

    >>> dump_tokens("b'abc' + B'abc'")
    STRING     "b'abc'"      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     "B'abc'"      (1, 9) (1, 15)
    >>> dump_tokens('b"abc" + B"abc"')
    STRING     'b"abc"'      (1, 0) (1, 6)
    OP         '+'           (1, 7) (1, 8)
    STRING     'B"abc"'      (1, 9) (1, 15)
    >>> dump_tokens("br'abc' + bR'abc' + Br'abc' + BR'abc'")
    STRING     "br'abc'"     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     "bR'abc'"     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     "Br'abc'"     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     "BR'abc'"     (1, 30) (1, 37)
    >>> dump_tokens('br"abc" + bR"abc" + Br"abc" + BR"abc"')
    STRING     'br"abc"'     (1, 0) (1, 7)
    OP         '+'           (1, 8) (1, 9)
    STRING     'bR"abc"'     (1, 10) (1, 17)
    OP         '+'           (1, 18) (1, 19)
    STRING     'Br"abc"'     (1, 20) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    STRING     'BR"abc"'     (1, 30) (1, 37)

Operators

    >>> dump_tokens("def d22(a, b, c=2, d=2, *k): pass")
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
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    OP         '<<'          (1, 6) (1, 8)
    NUMBER     '1'           (1, 9) (1, 10)
    OP         '>>'          (1, 11) (1, 13)
    NUMBER     '5'           (1, 14) (1, 15)

Additive

    >>> dump_tokens("x = 1 - y + 15 - 01 + 0x124 + z + a[5]")
    NAME       'x'           (1, 0) (1, 1)
    OP         '='           (1, 2) (1, 3)
    NUMBER     '1'           (1, 4) (1, 5)
    OP         '-'           (1, 6) (1, 7)
    NAME       'y'           (1, 8) (1, 9)
    OP         '+'           (1, 10) (1, 11)
    NUMBER     '15'          (1, 12) (1, 14)
    OP         '-'           (1, 15) (1, 16)
    NUMBER     '01'          (1, 17) (1, 19)
    OP         '+'           (1, 20) (1, 21)
    NUMBER     '0x124'       (1, 22) (1, 27)
    OP         '+'           (1, 28) (1, 29)
    NAME       'z'           (1, 30) (1, 31)
    OP         '+'           (1, 32) (1, 33)
    NAME       'a'           (1, 34) (1, 35)
    OP         '['           (1, 35) (1, 36)
    NUMBER     '5'           (1, 36) (1, 37)
    OP         ']'           (1, 37) (1, 38)

Multiplicative

    >>> dump_tokens("x = 1//1*1/5*12%0x12")
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

    >>>
    >>> tempdir = os.path.dirname(f) or os.curdir
    >>> testfiles = glob.glob(os.path.join(tempdir, "test*.py"))

    >>> if not test_support.is_resource_enabled("cpu"):
    ...     testfiles = random.sample(testfiles, 10)
    ...
    >>> for testfile in testfiles:
    ...     if not roundtrip(open(testfile)):
    ...         print "Roundtrip failed for file %s" % testfile
    ...         break
    ... else: True
    True

Evil tabs
    >>> dump_tokens("def f():\\n\\tif x\\n        \\tpass")
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
"""


from test import test_support
from tokenize import (untokenize, generate_tokens, NUMBER, NAME, OP,
                     STRING, ENDMARKER, tok_name)
from StringIO import StringIO
import os

def dump_tokens(s):
    """Print out the tokens in s in a table format.

    The ENDMARKER is omitted.
    """
    f = StringIO(s)
    for type, token, start, end, line in generate_tokens(f.readline):
        if type == ENDMARKER:
            break
        type = tok_name[type]
        print("%(type)-10.10s %(token)-13.13r %(start)s %(end)s" % locals())

# This is an example from the docs, set up as a doctest.
def decistmt(s):
    """Substitute Decimals for floats in a string of statements.

    >>> from decimal import Decimal
    >>> s = 'print +21.3e-5*-.1234/81.7'
    >>> decistmt(s)
    "print +Decimal ('21.3e-5')*-Decimal ('.1234')/Decimal ('81.7')"

    The format of the exponent is inherited from the platform C library.
    Known cases are "e-007" (Windows) and "e-07" (not Windows).  Since
    we're only showing 12 digits, and the 13th isn't close to 5, the
    rest of the output should be platform-independent.

    >>> exec(s) #doctest: +ELLIPSIS
    -3.21716034272e-0...7

    Output from calculations with Decimal should be identical across all
    platforms.

    >>> exec(decistmt(s))
    -3.217160342717258261933904529E-7
    """

    result = []
    g = generate_tokens(StringIO(s).readline)   # tokenize the string
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
    return untokenize(result)


__test__ = {"doctests" : doctests, 'decistmt': decistmt}


def test_main():
    from test import test_tokenize
    test_support.run_doctest(test_tokenize, True)

if __name__ == "__main__":
    test_main()
