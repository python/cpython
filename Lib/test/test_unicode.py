""" Test script for the Unicode implementation.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
from test_support import verify, verbose, TestFailed
import sys, string

if not sys.platform.startswith('java'):
    # Test basic sanity of repr()
    verify(repr(u'abc') == "u'abc'")
    verify(repr(u'ab\\c') == "u'ab\\\\c'")
    verify(repr(u'ab\\') == "u'ab\\\\'")
    verify(repr(u'\\c') == "u'\\\\c'")
    verify(repr(u'\\') == "u'\\\\'")
    verify(repr(u'\n') == "u'\\n'")
    verify(repr(u'\r') == "u'\\r'")
    verify(repr(u'\t') == "u'\\t'")
    verify(repr(u'\b') == "u'\\x08'")
    verify(repr(u"'\"") == """u'\\'"'""")
    verify(repr(u"'\"") == """u'\\'"'""")
    verify(repr(u"'") == '''u"'"''')
    verify(repr(u'"') == """u'"'""")
    latin1repr = (
        "u'\\x00\\x01\\x02\\x03\\x04\\x05\\x06\\x07\\x08\\t\\n\\x0b\\x0c\\r"
        "\\x0e\\x0f\\x10\\x11\\x12\\x13\\x14\\x15\\x16\\x17\\x18\\x19\\x1a"
        "\\x1b\\x1c\\x1d\\x1e\\x1f !\"#$%&\\'()*+,-./0123456789:;<=>?@ABCDEFGHI"
        "JKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\\x7f"
        "\\x80\\x81\\x82\\x83\\x84\\x85\\x86\\x87\\x88\\x89\\x8a\\x8b\\x8c\\x8d"
        "\\x8e\\x8f\\x90\\x91\\x92\\x93\\x94\\x95\\x96\\x97\\x98\\x99\\x9a\\x9b"
        "\\x9c\\x9d\\x9e\\x9f\\xa0\\xa1\\xa2\\xa3\\xa4\\xa5\\xa6\\xa7\\xa8\\xa9"
        "\\xaa\\xab\\xac\\xad\\xae\\xaf\\xb0\\xb1\\xb2\\xb3\\xb4\\xb5\\xb6\\xb7"
        "\\xb8\\xb9\\xba\\xbb\\xbc\\xbd\\xbe\\xbf\\xc0\\xc1\\xc2\\xc3\\xc4\\xc5"
        "\\xc6\\xc7\\xc8\\xc9\\xca\\xcb\\xcc\\xcd\\xce\\xcf\\xd0\\xd1\\xd2\\xd3"
        "\\xd4\\xd5\\xd6\\xd7\\xd8\\xd9\\xda\\xdb\\xdc\\xdd\\xde\\xdf\\xe0\\xe1"
        "\\xe2\\xe3\\xe4\\xe5\\xe6\\xe7\\xe8\\xe9\\xea\\xeb\\xec\\xed\\xee\\xef"
        "\\xf0\\xf1\\xf2\\xf3\\xf4\\xf5\\xf6\\xf7\\xf8\\xf9\\xfa\\xfb\\xfc\\xfd"
        "\\xfe\\xff'")
    testrepr = repr(u''.join(map(unichr, range(256))))
    verify(testrepr == latin1repr)

def test(method, input, output, *args):
    if verbose:
        print '%s.%s%s =? %s... ' % (repr(input), method, args, repr(output)),
    try:
        f = getattr(input, method)
        value = apply(f, args)
    except:
        value = sys.exc_type
        exc = sys.exc_info()[:2]
    else:
        exc = None
    if value == output and type(value) is type(output):
        # if the original is returned make sure that
        # this doesn't happen with subclasses
        if value is input:
            class usub(unicode):
                def __repr__(self):
                    return 'usub(%r)' % unicode.__repr__(self)
            input = usub(input)
            try:
                f = getattr(input, method)
                value = apply(f, args)
            except:
                value = sys.exc_type
                exc = sys.exc_info()[:2]
            if value is input:
                if verbose:
                   print 'no'
                print '*',f, `input`, `output`, `value`
                return
    if value != output or type(value) is not type(output):
        if verbose:
            print 'no'
        print '*',f, `input`, `output`, `value`
        if exc:
            print '  value == %s: %s' % (exc)
    else:
        if verbose:
            print 'yes'

test('capitalize', u' hello ', u' hello ')
test('capitalize', u'Hello ', u'Hello ')
test('capitalize', u'hello ', u'Hello ')
test('capitalize', u'aaaa', u'Aaaa')
test('capitalize', u'AaAa', u'Aaaa')

test('count', u'aaa', 3, u'a')
test('count', u'aaa', 0, u'b')
test('count', 'aaa', 3, u'a')
test('count', 'aaa', 0, u'b')
test('count', u'aaa', 3, 'a')
test('count', u'aaa', 0, 'b')

test('title', u' hello ', u' Hello ')
test('title', u'Hello ', u'Hello ')
test('title', u'hello ', u'Hello ')
test('title', u"fOrMaT thIs aS titLe String", u'Format This As Title String')
test('title', u"fOrMaT,thIs-aS*titLe;String", u'Format,This-As*Title;String')
test('title', u"getInt", u'Getint')

test('find', u'abcdefghiabc', 0, u'abc')
test('find', u'abcdefghiabc', 9, u'abc', 1)
test('find', u'abcdefghiabc', -1, u'def', 4)

test('rfind', u'abcdefghiabc', 9, u'abc')

test('lower', u'HeLLo', u'hello')
test('lower', u'hello', u'hello')

test('upper', u'HeLLo', u'HELLO')
test('upper', u'HELLO', u'HELLO')

if 0:
    transtable = '\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`xyzdefghijklmnopqrstuvwxyz{|}~\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377'

    test('maketrans', u'abc', transtable, u'xyz')
    test('maketrans', u'abc', ValueError, u'xyzq')

test('split', u'this is the split function',
     [u'this', u'is', u'the', u'split', u'function'])
test('split', u'a|b|c|d', [u'a', u'b', u'c', u'd'], u'|')
test('split', u'a|b|c|d', [u'a', u'b', u'c|d'], u'|', 2)
test('split', u'a b c d', [u'a', u'b c d'], None, 1)
test('split', u'a b c d', [u'a', u'b', u'c d'], None, 2)
test('split', u'a b c d', [u'a', u'b', u'c', u'd'], None, 3)
test('split', u'a b c d', [u'a', u'b', u'c', u'd'], None, 4)
test('split', u'a b c d', [u'a b c d'], None, 0)
test('split', u'a  b  c  d', [u'a', u'b', u'c  d'], None, 2)
test('split', u'a b c d ', [u'a', u'b', u'c', u'd'])
test('split', u'a//b//c//d', [u'a', u'b', u'c', u'd'], u'//')
test('split', u'a//b//c//d', [u'a', u'b', u'c', u'd'], '//')
test('split', 'a//b//c//d', [u'a', u'b', u'c', u'd'], u'//')
test('split', u'endcase test', [u'endcase ', u''], u'test')
test('split', u'endcase test', [u'endcase ', u''], 'test')
test('split', 'endcase test', [u'endcase ', u''], u'test')


# join now works with any sequence type
class Sequence:
    def __init__(self, seq): self.seq = seq
    def __len__(self): return len(self.seq)
    def __getitem__(self, i): return self.seq[i]

test('join', u' ', u'a b c d', [u'a', u'b', u'c', u'd'])
test('join', u' ', u'a b c d', ['a', 'b', u'c', u'd'])
test('join', u'', u'abcd', (u'a', u'b', u'c', u'd'))
test('join', u' ', u'w x y z', Sequence('wxyz'))
test('join', u' ', TypeError, 7)
test('join', u' ', TypeError, Sequence([7, u'hello', 123L]))
test('join', ' ', u'a b c d', [u'a', u'b', u'c', u'd'])
test('join', ' ', u'a b c d', ['a', 'b', u'c', u'd'])
test('join', '', u'abcd', (u'a', u'b', u'c', u'd'))
test('join', ' ', u'w x y z', Sequence(u'wxyz'))
test('join', ' ', TypeError, 7)

result = u''
for i in range(10):
    if i > 0:
        result = result + u':'
    result = result + u'x'*10
test('join', u':', result, [u'x' * 10] * 10)
test('join', u':', result, (u'x' * 10,) * 10)

test('strip', u'   hello   ', u'hello')
test('lstrip', u'   hello   ', u'hello   ')
test('rstrip', u'   hello   ', u'   hello')
test('strip', u'hello', u'hello')

# strip/lstrip/rstrip with None arg
test('strip', u'   hello   ', u'hello', None)
test('lstrip', u'   hello   ', u'hello   ', None)
test('rstrip', u'   hello   ', u'   hello', None)
test('strip', u'hello', u'hello', None)

# strip/lstrip/rstrip with unicode arg
test('strip', u'xyzzyhelloxyzzy', u'hello', u'xyz')
test('lstrip', u'xyzzyhelloxyzzy', u'helloxyzzy', u'xyz')
test('rstrip', u'xyzzyhelloxyzzy', u'xyzzyhello', u'xyz')
test('strip', u'hello', u'hello', u'xyz')

# strip/lstrip/rstrip with str arg
test('strip', u'xyzzyhelloxyzzy', u'hello', 'xyz')
test('lstrip', u'xyzzyhelloxyzzy', u'helloxyzzy', 'xyz')
test('rstrip', u'xyzzyhelloxyzzy', u'xyzzyhello', 'xyz')
test('strip', u'hello', u'hello', 'xyz')

test('swapcase', u'HeLLo cOmpUteRs', u'hEllO CoMPuTErS')

if 0:
    test('translate', u'xyzabcdef', u'xyzxyz', transtable, u'def')

    table = string.maketrans('a', u'A')
    test('translate', u'abc', u'Abc', table)
    test('translate', u'xyz', u'xyz', table)

test('replace', u'one!two!three!', u'one@two!three!', u'!', u'@', 1)
test('replace', u'one!two!three!', u'onetwothree', '!', '')
test('replace', u'one!two!three!', u'one@two@three!', u'!', u'@', 2)
test('replace', u'one!two!three!', u'one@two@three@', u'!', u'@', 3)
test('replace', u'one!two!three!', u'one@two@three@', u'!', u'@', 4)
test('replace', u'one!two!three!', u'one!two!three!', u'!', u'@', 0)
test('replace', u'one!two!three!', u'one@two@three@', u'!', u'@')
test('replace', u'one!two!three!', u'one!two!three!', u'x', u'@')
test('replace', u'one!two!three!', u'one!two!three!', u'x', u'@', 2)

test('startswith', u'hello', True, u'he')
test('startswith', u'hello', True, u'hello')
test('startswith', u'hello', False, u'hello world')
test('startswith', u'hello', True, u'')
test('startswith', u'hello', False, u'ello')
test('startswith', u'hello', True, u'ello', 1)
test('startswith', u'hello', True, u'o', 4)
test('startswith', u'hello', False, u'o', 5)
test('startswith', u'hello', True, u'', 5)
test('startswith', u'hello', False, u'lo', 6)
test('startswith', u'helloworld', True, u'lowo', 3)
test('startswith', u'helloworld', True, u'lowo', 3, 7)
test('startswith', u'helloworld', False, u'lowo', 3, 6)

test('endswith', u'hello', True, u'lo')
test('endswith', u'hello', False, u'he')
test('endswith', u'hello', True, u'')
test('endswith', u'hello', False, u'hello world')
test('endswith', u'helloworld', False, u'worl')
test('endswith', u'helloworld', True, u'worl', 3, 9)
test('endswith', u'helloworld', True, u'world', 3, 12)
test('endswith', u'helloworld', True, u'lowo', 1, 7)
test('endswith', u'helloworld', True, u'lowo', 2, 7)
test('endswith', u'helloworld', True, u'lowo', 3, 7)
test('endswith', u'helloworld', False, u'lowo', 4, 7)
test('endswith', u'helloworld', False, u'lowo', 3, 8)
test('endswith', u'ab', False, u'ab', 0, 1)
test('endswith', u'ab', False, u'ab', 0, 0)

test('expandtabs', u'abc\rab\tdef\ng\thi', u'abc\rab      def\ng       hi')
test('expandtabs', u'abc\rab\tdef\ng\thi', u'abc\rab      def\ng       hi', 8)
test('expandtabs', u'abc\rab\tdef\ng\thi', u'abc\rab  def\ng   hi', 4)
test('expandtabs', u'abc\r\nab\tdef\ng\thi', u'abc\r\nab  def\ng   hi', 4)
test('expandtabs', u'abc\r\nab\r\ndef\ng\r\nhi', u'abc\r\nab\r\ndef\ng\r\nhi', 4)

if 0:
    test('capwords', u'abc def ghi', u'Abc Def Ghi')
    test('capwords', u'abc\tdef\nghi', u'Abc Def Ghi')
    test('capwords', u'abc\t   def  \nghi', u'Abc Def Ghi')

test('zfill', u'123', u'123', 2)
test('zfill', u'123', u'123', 3)
test('zfill', u'123', u'0123', 4)
test('zfill', u'+123', u'+123', 3)
test('zfill', u'+123', u'+123', 4)
test('zfill', u'+123', u'+0123', 5)
test('zfill', u'-123', u'-123', 3)
test('zfill', u'-123', u'-123', 4)
test('zfill', u'-123', u'-0123', 5)
test('zfill', u'', u'000', 3)
test('zfill', u'34', u'34', 1)
test('zfill', u'34', u'00034', 5)

# Comparisons:
print 'Testing Unicode comparisons...',
verify(u'abc' == 'abc')
verify('abc' == u'abc')
verify(u'abc' == u'abc')
verify(u'abcd' > 'abc')
verify('abcd' > u'abc')
verify(u'abcd' > u'abc')
verify(u'abc' < 'abcd')
verify('abc' < u'abcd')
verify(u'abc' < u'abcd')
print 'done.'

if 0:
    # Move these tests to a Unicode collation module test...

    print 'Testing UTF-16 code point order comparisons...',
    #No surrogates, no fixup required.
    verify(u'\u0061' < u'\u20ac')
    # Non surrogate below surrogate value, no fixup required
    verify(u'\u0061' < u'\ud800\udc02')

    # Non surrogate above surrogate value, fixup required
    def test_lecmp(s, s2):
        verify(s <  s2 , "comparison failed on %s < %s" % (s, s2))

    def test_fixup(s):
        s2 = u'\ud800\udc01'
        test_lecmp(s, s2)
        s2 = u'\ud900\udc01'
        test_lecmp(s, s2)
        s2 = u'\uda00\udc01'
        test_lecmp(s, s2)
        s2 = u'\udb00\udc01'
        test_lecmp(s, s2)
        s2 = u'\ud800\udd01'
        test_lecmp(s, s2)
        s2 = u'\ud900\udd01'
        test_lecmp(s, s2)
        s2 = u'\uda00\udd01'
        test_lecmp(s, s2)
        s2 = u'\udb00\udd01'
        test_lecmp(s, s2)
        s2 = u'\ud800\ude01'
        test_lecmp(s, s2)
        s2 = u'\ud900\ude01'
        test_lecmp(s, s2)
        s2 = u'\uda00\ude01'
        test_lecmp(s, s2)
        s2 = u'\udb00\ude01'
        test_lecmp(s, s2)
        s2 = u'\ud800\udfff'
        test_lecmp(s, s2)
        s2 = u'\ud900\udfff'
        test_lecmp(s, s2)
        s2 = u'\uda00\udfff'
        test_lecmp(s, s2)
        s2 = u'\udb00\udfff'
        test_lecmp(s, s2)

    test_fixup(u'\ue000')
    test_fixup(u'\uff61')

    # Surrogates on both sides, no fixup required
    verify(u'\ud800\udc02' < u'\ud84d\udc56')
    print 'done.'

test('ljust', u'abc',  u'abc       ', 10)
test('rjust', u'abc',  u'       abc', 10)
test('center', u'abc', u'   abc    ', 10)
test('ljust', u'abc',  u'abc   ', 6)
test('rjust', u'abc',  u'   abc', 6)
test('center', u'abc', u' abc  ', 6)
test('ljust', u'abc', u'abc', 2)
test('rjust', u'abc', u'abc', 2)
test('center', u'abc', u'abc', 2)

test('islower', u'a', True)
test('islower', u'A', False)
test('islower', u'\n', False)
test('islower', u'\u1FFc', False)
test('islower', u'abc', True)
test('islower', u'aBc', False)
test('islower', u'abc\n', True)

test('isupper', u'a', False)
test('isupper', u'A', True)
test('isupper', u'\n', False)
if sys.platform[:4] != 'java':
    test('isupper', u'\u1FFc', False)
test('isupper', u'ABC', True)
test('isupper', u'AbC', False)
test('isupper', u'ABC\n', True)

test('istitle', u'a', False)
test('istitle', u'A', True)
test('istitle', u'\n', False)
test('istitle', u'\u1FFc', True)
test('istitle', u'A Titlecased Line', True)
test('istitle', u'A\nTitlecased Line', True)
test('istitle', u'A Titlecased, Line', True)
test('istitle', u'Greek \u1FFcitlecases ...', True)
test('istitle', u'Not a capitalized String', False)
test('istitle', u'Not\ta Titlecase String', False)
test('istitle', u'Not--a Titlecase String', False)

test('isalpha', u'a', True)
test('isalpha', u'A', True)
test('isalpha', u'\n', False)
test('isalpha', u'\u1FFc', True)
test('isalpha', u'abc', True)
test('isalpha', u'aBc123', False)
test('isalpha', u'abc\n', False)

test('isalnum', u'a', True)
test('isalnum', u'A', True)
test('isalnum', u'\n', False)
test('isalnum', u'123abc456', True)
test('isalnum', u'a1b3c', True)
test('isalnum', u'aBc000 ', False)
test('isalnum', u'abc\n', False)

test('splitlines', u"abc\ndef\n\rghi", [u'abc', u'def', u'', u'ghi'])
test('splitlines', u"abc\ndef\n\r\nghi", [u'abc', u'def', u'', u'ghi'])
test('splitlines', u"abc\ndef\r\nghi", [u'abc', u'def', u'ghi'])
test('splitlines', u"abc\ndef\r\nghi\n", [u'abc', u'def', u'ghi'])
test('splitlines', u"abc\ndef\r\nghi\n\r", [u'abc', u'def', u'ghi', u''])
test('splitlines', u"\nabc\ndef\r\nghi\n\r", [u'', u'abc', u'def', u'ghi', u''])
test('splitlines', u"\nabc\ndef\r\nghi\n\r", [u'\n', u'abc\n', u'def\r\n', u'ghi\n', u'\r'], True)

test('translate', u"abababc", u'bbbc', {ord('a'):None})
test('translate', u"abababc", u'iiic', {ord('a'):None, ord('b'):ord('i')})
test('translate', u"abababc", u'iiix', {ord('a'):None, ord('b'):ord('i'), ord('c'):u'x'})

# Contains:
print 'Testing Unicode contains method...',
verify(('a' in u'abdb') == 1)
verify(('a' in u'bdab') == 1)
verify(('a' in u'bdaba') == 1)
verify(('a' in u'bdba') == 1)
verify(('a' in u'bdba') == 1)
verify((u'a' in u'bdba') == 1)
verify((u'a' in u'bdb') == 0)
verify((u'a' in 'bdb') == 0)
verify((u'a' in 'bdba') == 1)
verify((u'a' in ('a',1,None)) == 1)
verify((u'a' in (1,None,'a')) == 1)
verify((u'a' in (1,None,u'a')) == 1)
verify(('a' in ('a',1,None)) == 1)
verify(('a' in (1,None,'a')) == 1)
verify(('a' in (1,None,u'a')) == 1)
verify(('a' in ('x',1,u'y')) == 0)
verify(('a' in ('x',1,None)) == 0)
print 'done.'

# Formatting:
print 'Testing Unicode formatting strings...',
verify(u"%s, %s" % (u"abc", "abc") == u'abc, abc')
verify(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", 1, 2, 3) == u'abc, abc, 1, 2.000000,  3.00')
verify(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", 1, -2, 3) == u'abc, abc, 1, -2.000000,  3.00')
verify(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 3.5) == u'abc, abc, -1, -2.000000,  3.50')
verify(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 3.57) == u'abc, abc, -1, -2.000000,  3.57')
verify(u"%s, %s, %i, %f, %5.2f" % (u"abc", "abc", -1, -2, 1003.57) == u'abc, abc, -1, -2.000000, 1003.57')
verify(u"%c" % (u"a",) == u'a')
verify(u"%c" % ("a",) == u'a')
verify(u"%c" % (34,) == u'"')
verify(u"%c" % (36,) == u'$')
if sys.platform[:4] != 'java':
    value = u"%r, %r" % (u"abc", "abc")
    if value != u"u'abc', 'abc'":
        print '*** formatting failed for "%s"' % 'u"%r, %r" % (u"abc", "abc")'

verify(u"%(x)s, %(y)s" % {'x':u"abc", 'y':"def"} == u'abc, def')
try:
    value = u"%(x)s, %(ä)s" % {'x':u"abc", u'ä':"def"}
except KeyError:
    print '*** formatting failed for "%s"' % "u'abc, def'"
else:
    verify(value == u'abc, def')

# formatting jobs delegated from the string implementation:
verify('...%(foo)s...' % {'foo':u"abc"} == u'...abc...')
verify('...%(foo)s...' % {'foo':"abc"} == '...abc...')
verify('...%(foo)s...' % {u'foo':"abc"} == '...abc...')
verify('...%(foo)s...' % {u'foo':u"abc"} == u'...abc...')
verify('...%(foo)s...' % {u'foo':u"abc",'def':123} ==  u'...abc...')
verify('...%(foo)s...' % {u'foo':u"abc",u'def':123} == u'...abc...')
verify('...%s...%s...%s...%s...' % (1,2,3,u"abc") == u'...1...2...3...abc...')
verify('...%%...%%s...%s...%s...%s...%s...' % (1,2,3,u"abc") == u'...%...%s...1...2...3...abc...')
verify('...%s...' % u"abc" == u'...abc...')
verify('%*s' % (5,u'abc',) == u'  abc')
verify('%*s' % (-5,u'abc',) == u'abc  ')
verify('%*.*s' % (5,2,u'abc',) == u'   ab')
verify('%*.*s' % (5,3,u'abc',) == u'  abc')
verify('%i %*.*s' % (10, 5,3,u'abc',) == u'10   abc')
verify('%i%s %*.*s' % (10, 3, 5,3,u'abc',) == u'103   abc')
print 'done.'

print 'Testing builtin unicode()...',

# unicode(obj) tests (this maps to PyObject_Unicode() at C level)

verify(unicode(u'unicode remains unicode') == u'unicode remains unicode')

class UnicodeSubclass(unicode):
    pass

verify(unicode(UnicodeSubclass('unicode subclass becomes unicode'))
       == u'unicode subclass becomes unicode')

verify(unicode('strings are converted to unicode')
       == u'strings are converted to unicode')

class UnicodeCompat:
    def __init__(self, x):
        self.x = x
    def __unicode__(self):
        return self.x

verify(unicode(UnicodeCompat('__unicode__ compatible objects are recognized'))
       == u'__unicode__ compatible objects are recognized')

class StringCompat:
    def __init__(self, x):
        self.x = x
    def __str__(self):
        return self.x

verify(unicode(StringCompat('__str__ compatible objects are recognized'))
       == u'__str__ compatible objects are recognized')

# unicode(obj) is compatible to str():

o = StringCompat('unicode(obj) is compatible to str()')
verify(unicode(o) == u'unicode(obj) is compatible to str()')
verify(str(o) == 'unicode(obj) is compatible to str()')

for obj in (123, 123.45, 123L):
    verify(unicode(obj) == unicode(str(obj)))

# unicode(obj, encoding, error) tests (this maps to
# PyUnicode_FromEncodedObject() at C level)

if not sys.platform.startswith('java'):
    try:
        unicode(u'decoding unicode is not supported', 'utf-8', 'strict')
    except TypeError:
        pass
    else:
        raise TestFailed, "decoding unicode should NOT be supported"

verify(unicode('strings are decoded to unicode', 'utf-8', 'strict')
       == u'strings are decoded to unicode')

if not sys.platform.startswith('java'):
    verify(unicode(buffer('character buffers are decoded to unicode'),
                   'utf-8', 'strict')
           == u'character buffers are decoded to unicode')

print 'done.'

# Test builtin codecs
print 'Testing builtin codecs...',

# UTF-7 specific encoding tests:
utfTests = [(u'A\u2262\u0391.', 'A+ImIDkQ.'),  # RFC2152 example
 (u'Hi Mom -\u263a-!', 'Hi Mom -+Jjo--!'),     # RFC2152 example
 (u'\u65E5\u672C\u8A9E', '+ZeVnLIqe-'),        # RFC2152 example
 (u'Item 3 is \u00a31.', 'Item 3 is +AKM-1.'), # RFC2152 example
 (u'+', '+-'),
 (u'+-', '+--'),
 (u'+?', '+-?'),
 (u'\?', '+AFw?'),
 (u'+?', '+-?'),
 (ur'\\?', '+AFwAXA?'),
 (ur'\\\?', '+AFwAXABc?'),
 (ur'++--', '+-+---')]

for x,y in utfTests:
    verify( x.encode('utf-7') == y )

try:
    unicode('+3ADYAA-', 'utf-7') # surrogates not supported
except UnicodeError:
    pass
else:
    raise TestFailed, "unicode('+3ADYAA-', 'utf-7') failed to raise an exception"

verify(unicode('+3ADYAA-', 'utf-7', 'replace') == u'\ufffd')

# UTF-8 specific encoding tests:
verify(u''.encode('utf-8') == '')
verify(u'\u20ac'.encode('utf-8') == '\xe2\x82\xac')
verify(u'\ud800\udc02'.encode('utf-8') == '\xf0\x90\x80\x82')
verify(u'\ud84d\udc56'.encode('utf-8') == '\xf0\xa3\x91\x96')
verify(u'\ud800'.encode('utf-8') == '\xed\xa0\x80')
verify(u'\udc00'.encode('utf-8') == '\xed\xb0\x80')
verify((u'\ud800\udc02'*1000).encode('utf-8') ==
       '\xf0\x90\x80\x82'*1000)
verify(u'\u6b63\u78ba\u306b\u8a00\u3046\u3068\u7ffb\u8a33\u306f'
       u'\u3055\u308c\u3066\u3044\u307e\u305b\u3093\u3002\u4e00'
       u'\u90e8\u306f\u30c9\u30a4\u30c4\u8a9e\u3067\u3059\u304c'
       u'\u3001\u3042\u3068\u306f\u3067\u305f\u3089\u3081\u3067'
       u'\u3059\u3002\u5b9f\u969b\u306b\u306f\u300cWenn ist das'
       u' Nunstuck git und'.encode('utf-8') ==
       '\xe6\xad\xa3\xe7\xa2\xba\xe3\x81\xab\xe8\xa8\x80\xe3\x81'
       '\x86\xe3\x81\xa8\xe7\xbf\xbb\xe8\xa8\xb3\xe3\x81\xaf\xe3'
       '\x81\x95\xe3\x82\x8c\xe3\x81\xa6\xe3\x81\x84\xe3\x81\xbe'
       '\xe3\x81\x9b\xe3\x82\x93\xe3\x80\x82\xe4\xb8\x80\xe9\x83'
       '\xa8\xe3\x81\xaf\xe3\x83\x89\xe3\x82\xa4\xe3\x83\x84\xe8'
       '\xaa\x9e\xe3\x81\xa7\xe3\x81\x99\xe3\x81\x8c\xe3\x80\x81'
       '\xe3\x81\x82\xe3\x81\xa8\xe3\x81\xaf\xe3\x81\xa7\xe3\x81'
       '\x9f\xe3\x82\x89\xe3\x82\x81\xe3\x81\xa7\xe3\x81\x99\xe3'
       '\x80\x82\xe5\xae\x9f\xe9\x9a\x9b\xe3\x81\xab\xe3\x81\xaf'
       '\xe3\x80\x8cWenn ist das Nunstuck git und')

# UTF-8 specific decoding tests
verify(unicode('\xf0\xa3\x91\x96', 'utf-8') == u'\U00023456' )
verify(unicode('\xf0\x90\x80\x82', 'utf-8') == u'\U00010002' )
verify(unicode('\xe2\x82\xac', 'utf-8') == u'\u20ac' )

# Other possible utf-8 test cases:
# * strict decoding testing for all of the
#   UTF8_ERROR cases in PyUnicode_DecodeUTF8

verify(unicode('hello','ascii') == u'hello')
verify(unicode('hello','utf-8') == u'hello')
verify(unicode('hello','utf8') == u'hello')
verify(unicode('hello','latin-1') == u'hello')

# Error handling
try:
    u'Andr\202 x'.encode('ascii')
    u'Andr\202 x'.encode('ascii','strict')
except ValueError:
    pass
else:
    raise TestFailed, "u'Andr\202'.encode('ascii') failed to raise an exception"
verify(u'Andr\202 x'.encode('ascii','ignore') == "Andr x")
verify(u'Andr\202 x'.encode('ascii','replace') == "Andr? x")

try:
    unicode('Andr\202 x','ascii')
    unicode('Andr\202 x','ascii','strict')
except ValueError:
    pass
else:
    raise TestFailed, "unicode('Andr\202') failed to raise an exception"
verify(unicode('Andr\202 x','ascii','ignore') == u"Andr x")
verify(unicode('Andr\202 x','ascii','replace') == u'Andr\uFFFD x')

verify("\\N{foo}xx".decode("unicode-escape", "ignore") == u"xx")
try:
    "\\".decode("unicode-escape")
except ValueError:
    pass
else:
    raise TestFailed, '"\\".decode("unicode-escape") should fail'

verify(u'hello'.encode('ascii') == 'hello')
verify(u'hello'.encode('utf-7') == 'hello')
verify(u'hello'.encode('utf-8') == 'hello')
verify(u'hello'.encode('utf8') == 'hello')
verify(u'hello'.encode('utf-16-le') == 'h\000e\000l\000l\000o\000')
verify(u'hello'.encode('utf-16-be') == '\000h\000e\000l\000l\000o')
verify(u'hello'.encode('latin-1') == 'hello')

# Roundtrip safety for BMP (just the first 1024 chars)
u = u''.join(map(unichr, range(1024)))
for encoding in ('utf-7', 'utf-8', 'utf-16', 'utf-16-le', 'utf-16-be',
                 'raw_unicode_escape', 'unicode_escape', 'unicode_internal'):
    verify(unicode(u.encode(encoding),encoding) == u)

# Roundtrip safety for BMP (just the first 256 chars)
u = u''.join(map(unichr, range(256)))
for encoding in (
    'latin-1',
    ):
    try:
        verify(unicode(u.encode(encoding),encoding) == u)
    except TestFailed:
        print '*** codec "%s" failed round-trip' % encoding
    except ValueError,why:
        print '*** codec for "%s" failed: %s' % (encoding, why)

# Roundtrip safety for BMP (just the first 128 chars)
u = u''.join(map(unichr, range(128)))
for encoding in (
    'ascii',
    ):
    try:
        verify(unicode(u.encode(encoding),encoding) == u)
    except TestFailed:
        print '*** codec "%s" failed round-trip' % encoding
    except ValueError,why:
        print '*** codec for "%s" failed: %s' % (encoding, why)

# Roundtrip safety for non-BMP (just a few chars)
u = u'\U00010001\U00020002\U00030003\U00040004\U00050005'
for encoding in ('utf-8',
                 'utf-16', 'utf-16-le', 'utf-16-be',
                 #'raw_unicode_escape',
                 'unicode_escape', 'unicode_internal'):
    verify(unicode(u.encode(encoding),encoding) == u)

# UTF-8 must be roundtrip safe for all UCS-2 code points
u = u''.join(map(unichr, range(0x10000)))
for encoding in ('utf-8',):
    verify(unicode(u.encode(encoding),encoding) == u)

print 'done.'

print 'Testing standard mapping codecs...',

print '0-127...',
s = ''.join(map(chr, range(128)))
for encoding in (
    'cp037', 'cp1026',
    'cp437', 'cp500', 'cp737', 'cp775', 'cp850',
    'cp852', 'cp855', 'cp860', 'cp861', 'cp862',
    'cp863', 'cp865', 'cp866',
    'iso8859_10', 'iso8859_13', 'iso8859_14', 'iso8859_15',
    'iso8859_2', 'iso8859_3', 'iso8859_4', 'iso8859_5', 'iso8859_6',
    'iso8859_7', 'iso8859_9', 'koi8_r', 'latin_1',
    'mac_cyrillic', 'mac_latin2',

    'cp1250', 'cp1251', 'cp1252', 'cp1253', 'cp1254', 'cp1255',
    'cp1256', 'cp1257', 'cp1258',
    'cp856', 'cp857', 'cp864', 'cp869', 'cp874',

    'mac_greek', 'mac_iceland','mac_roman', 'mac_turkish',
    'cp1006', 'iso8859_8',

    ### These have undefined mappings:
    #'cp424',

    ### These fail the round-trip:
    #'cp875'

    ):
    try:
        verify(unicode(s,encoding).encode(encoding) == s)
    except TestFailed:
        print '*** codec "%s" failed round-trip' % encoding
    except ValueError,why:
        print '*** codec for "%s" failed: %s' % (encoding, why)

print '128-255...',
s = ''.join(map(chr, range(128,256)))
for encoding in (
    'cp037', 'cp1026',
    'cp437', 'cp500', 'cp737', 'cp775', 'cp850',
    'cp852', 'cp855', 'cp860', 'cp861', 'cp862',
    'cp863', 'cp865', 'cp866',
    'iso8859_10', 'iso8859_13', 'iso8859_14', 'iso8859_15',
    'iso8859_2', 'iso8859_4', 'iso8859_5',
    'iso8859_9', 'koi8_r', 'latin_1',
    'mac_cyrillic', 'mac_latin2',

    ### These have undefined mappings:
    #'cp1250', 'cp1251', 'cp1252', 'cp1253', 'cp1254', 'cp1255',
    #'cp1256', 'cp1257', 'cp1258',
    #'cp424', 'cp856', 'cp857', 'cp864', 'cp869', 'cp874',
    #'iso8859_3', 'iso8859_6', 'iso8859_7',
    #'mac_greek', 'mac_iceland','mac_roman', 'mac_turkish',

    ### These fail the round-trip:
    #'cp1006', 'cp875', 'iso8859_8',

    ):
    try:
        verify(unicode(s,encoding).encode(encoding) == s)
    except TestFailed:
        print '*** codec "%s" failed round-trip' % encoding
    except ValueError,why:
        print '*** codec for "%s" failed: %s' % (encoding, why)

print 'done.'

print 'Testing Unicode string concatenation...',
verify((u"abc" u"def") == u"abcdef")
verify(("abc" u"def") == u"abcdef")
verify((u"abc" "def") == u"abcdef")
verify((u"abc" u"def" "ghi") == u"abcdefghi")
verify(("abc" "def" u"ghi") == u"abcdefghi")
print 'done.'

print 'Testing Unicode printing...',
print u'abc'
print u'abc', u'def'
print u'abc', 'def'
print 'abc', u'def'
print u'abc\n'
print u'abc\n',
print u'abc\n',
print u'def\n'
print u'def\n'
print 'done.'
