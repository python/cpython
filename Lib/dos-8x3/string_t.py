"""Common tests shared by test_string and test_userstring"""

import string

transtable = '\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037 !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`xyzdefghijklmnopqrstuvwxyz{|}~\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377'

from UserList import UserList

class Sequence:
    def __init__(self): self.seq = 'wxyz'
    def __len__(self): return len(self.seq)
    def __getitem__(self, i): return self.seq[i]

class BadSeq1(Sequence):
    def __init__(self): self.seq = [7, 'hello', 123L]

class BadSeq2(Sequence):
    def __init__(self): self.seq = ['a', 'b', 'c']
    def __len__(self): return 8

def run_module_tests(test):
    """Run all tests that exercise a function in the string module"""

    test('atoi', " 1 ", 1)
    test('atoi', " 1x", ValueError)
    test('atoi', " x1 ", ValueError)
    test('atol', "  1  ", 1L)
    test('atol', "  1x ", ValueError)
    test('atol', "  x1 ", ValueError)
    test('atof', "  1  ", 1.0)
    test('atof', "  1x ", ValueError)
    test('atof', "  x1 ", ValueError)

    test('maketrans', 'abc', transtable, 'xyz')
    test('maketrans', 'abc', ValueError, 'xyzq')

    # join now works with any sequence type
    test('join', ['a', 'b', 'c', 'd'], 'a b c d')
    test('join', ('a', 'b', 'c', 'd'), 'abcd', '')
    test('join', Sequence(), 'w x y z')
    test('join', 7, TypeError)

    test('join', BadSeq1(), TypeError)
    test('join', BadSeq2(), 'a b c')

    # try a few long ones
    print string.join(['x' * 100] * 100, ':')
    print string.join(('x' * 100,) * 100, ':')


def run_method_tests(test):
    """Run all tests that exercise a method of a string object"""

    test('capitalize', ' hello ', ' hello ')
    test('capitalize', 'hello ', 'Hello ')
    test('find', 'abcdefghiabc', 0, 'abc')
    test('find', 'abcdefghiabc', 9, 'abc', 1)
    test('find', 'abcdefghiabc', -1, 'def', 4)
    test('rfind', 'abcdefghiabc', 9, 'abc')
    test('lower', 'HeLLo', 'hello')
    test('lower', 'hello', 'hello')
    test('upper', 'HeLLo', 'HELLO')
    test('upper', 'HELLO', 'HELLO')

    test('title', ' hello ', ' Hello ')
    test('title', 'hello ', 'Hello ')
    test('title', "fOrMaT thIs aS titLe String", 'Format This As Title String')
    test('title', "fOrMaT,thIs-aS*titLe;String", 'Format,This-As*Title;String')
    test('title', "getInt", 'Getint')

    test('expandtabs', 'abc\rab\tdef\ng\thi', 'abc\rab      def\ng       hi')
    test('expandtabs', 'abc\rab\tdef\ng\thi', 'abc\rab      def\ng       hi', 8)
    test('expandtabs', 'abc\rab\tdef\ng\thi', 'abc\rab  def\ng   hi', 4)
    test('expandtabs', 'abc\r\nab\tdef\ng\thi', 'abc\r\nab  def\ng   hi', 4)

    test('islower', 'a', 1)
    test('islower', 'A', 0)
    test('islower', '\n', 0)
    test('islower', 'abc', 1)
    test('islower', 'aBc', 0)
    test('islower', 'abc\n', 1)

    test('isupper', 'a', 0)
    test('isupper', 'A', 1)
    test('isupper', '\n', 0)
    test('isupper', 'ABC', 1)
    test('isupper', 'AbC', 0)
    test('isupper', 'ABC\n', 1)

    test('istitle', 'a', 0)
    test('istitle', 'A', 1)
    test('istitle', '\n', 0)
    test('istitle', 'A Titlecased Line', 1)
    test('istitle', 'A\nTitlecased Line', 1)
    test('istitle', 'A Titlecased, Line', 1)
    test('istitle', 'Not a capitalized String', 0)
    test('istitle', 'Not\ta Titlecase String', 0)
    test('istitle', 'Not--a Titlecase String', 0)

    test('isalpha', 'a', 1)
    test('isalpha', 'A', 1)
    test('isalpha', '\n', 0)
    test('isalpha', 'abc', 1)
    test('isalpha', 'aBc123', 0)
    test('isalpha', 'abc\n', 0)

    test('isalnum', 'a', 1)
    test('isalnum', 'A', 1)
    test('isalnum', '\n', 0)
    test('isalnum', '123abc456', 1)
    test('isalnum', 'a1b3c', 1)
    test('isalnum', 'aBc000 ', 0)
    test('isalnum', 'abc\n', 0)

    # join now works with any sequence type
    test('join', ' ', 'a b c d', ['a', 'b', 'c', 'd'])
    test('join', '', 'abcd', ('a', 'b', 'c', 'd'))
    test('join', ' ', 'w x y z', Sequence())
    test('join', 'a', 'abc', ('abc',))
    test('join', 'a', 'z', UserList(['z']))
    test('join', u'.', u'a.b.c', ['a', 'b', 'c'])
    test('join', '.', u'a.b.c', [u'a', 'b', 'c'])
    test('join', '.', u'a.b.c', ['a', u'b', 'c'])
    test('join', '.', u'a.b.c', ['a', 'b', u'c'])
    test('join', '.', TypeError, ['a', u'b', 3])
    for i in [5, 25, 125]:
        test('join', '-', ((('a' * i) + '-') * i)[:-1],
             ['a' * i] * i)

    test('join', ' ', TypeError, BadSeq1())
    test('join', ' ', 'a b c', BadSeq2())

    test('splitlines', "abc\ndef\n\rghi", ['abc', 'def', '', 'ghi'])
    test('splitlines', "abc\ndef\n\r\nghi", ['abc', 'def', '', 'ghi'])
    test('splitlines', "abc\ndef\r\nghi", ['abc', 'def', 'ghi'])
    test('splitlines', "abc\ndef\r\nghi\n", ['abc', 'def', 'ghi'])
    test('splitlines', "abc\ndef\r\nghi\n\r", ['abc', 'def', 'ghi', ''])
    test('splitlines', "\nabc\ndef\r\nghi\n\r", ['', 'abc', 'def', 'ghi', ''])
    test('splitlines', "\nabc\ndef\r\nghi\n\r", ['\n', 'abc\n', 'def\r\n', 'ghi\n', '\r'], 1)

    test('split', 'this is the split function',
         ['this', 'is', 'the', 'split', 'function'])
    test('split', 'a|b|c|d', ['a', 'b', 'c', 'd'], '|')
    test('split', 'a|b|c|d', ['a', 'b', 'c|d'], '|', 2)
    test('split', 'a b c d', ['a', 'b c d'], None, 1)
    test('split', 'a b c d', ['a', 'b', 'c d'], None, 2)
    test('split', 'a b c d', ['a', 'b', 'c', 'd'], None, 3)
    test('split', 'a b c d', ['a', 'b', 'c', 'd'], None, 4)
    test('split', 'a b c d', ['a b c d'], None, 0)
    test('split', 'a  b  c  d', ['a', 'b', 'c  d'], None, 2)
    test('split', 'a b c d ', ['a', 'b', 'c', 'd'])

    test('strip', '   hello   ', 'hello')
    test('lstrip', '   hello   ', 'hello   ')
    test('rstrip', '   hello   ', '   hello')
    test('strip', 'hello', 'hello')

    test('swapcase', 'HeLLo cOmpUteRs', 'hEllO CoMPuTErS')
    test('translate', 'xyzabcdef', 'xyzxyz', transtable, 'def')

    table = string.maketrans('a', 'A')
    test('translate', 'abc', 'Abc', table)
    test('translate', 'xyz', 'xyz', table)

    test('replace', 'one!two!three!', 'one@two!three!', '!', '@', 1)
    test('replace', 'one!two!three!', 'onetwothree', '!', '')
    test('replace', 'one!two!three!', 'one@two@three!', '!', '@', 2)
    test('replace', 'one!two!three!', 'one@two@three@', '!', '@', 3)
    test('replace', 'one!two!three!', 'one@two@three@', '!', '@', 4)
    test('replace', 'one!two!three!', 'one!two!three!', '!', '@', 0)
    test('replace', 'one!two!three!', 'one@two@three@', '!', '@')
    test('replace', 'one!two!three!', 'one!two!three!', 'x', '@')
    test('replace', 'one!two!three!', 'one!two!three!', 'x', '@', 2)

    test('startswith', 'hello', 1, 'he')
    test('startswith', 'hello', 1, 'hello')
    test('startswith', 'hello', 0, 'hello world')
    test('startswith', 'hello', 1, '')
    test('startswith', 'hello', 0, 'ello')
    test('startswith', 'hello', 1, 'ello', 1)
    test('startswith', 'hello', 1, 'o', 4)
    test('startswith', 'hello', 0, 'o', 5)
    test('startswith', 'hello', 1, '', 5)
    test('startswith', 'hello', 0, 'lo', 6)
    test('startswith', 'helloworld', 1, 'lowo', 3)
    test('startswith', 'helloworld', 1, 'lowo', 3, 7)
    test('startswith', 'helloworld', 0, 'lowo', 3, 6)

    test('endswith', 'hello', 1, 'lo')
    test('endswith', 'hello', 0, 'he')
    test('endswith', 'hello', 1, '')
    test('endswith', 'hello', 0, 'hello world')
    test('endswith', 'helloworld', 0, 'worl')
    test('endswith', 'helloworld', 1, 'worl', 3, 9)
    test('endswith', 'helloworld', 1, 'world', 3, 12)
    test('endswith', 'helloworld', 1, 'lowo', 1, 7)
    test('endswith', 'helloworld', 1, 'lowo', 2, 7)
    test('endswith', 'helloworld', 1, 'lowo', 3, 7)
    test('endswith', 'helloworld', 0, 'lowo', 4, 7)
    test('endswith', 'helloworld', 0, 'lowo', 3, 8)
    test('endswith', 'ab', 0, 'ab', 0, 1)
    test('endswith', 'ab', 0, 'ab', 0, 0)
