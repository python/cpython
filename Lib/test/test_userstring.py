#!/usr/bin/env python
import sys, string
from test_support import verbose
# UserString is a wrapper around the native builtin string type.
# UserString instances should behave similar to builtin string objects.
# The test cases were in part derived from 'test_string.py'.
from UserString import UserString

if __name__ == "__main__":
    verbose = 0

tested_methods = {}

def test(methodname, input, *args):
    global tested_methods
    tested_methods[methodname] = 1
    if verbose:
        print '%s.%s(%s) ' % (input, methodname, args),
    u = UserString(input)
    objects = [input, u, UserString(u)]
    res = [""] * 3
    for i in range(3):
        object = objects[i]
        try:
            f = getattr(object, methodname)
            res[i] = apply(f, args)
        except:
            res[i] = sys.exc_type
    if res[0] != res[1]:
        if verbose:
            print 'no'
        print `input`, f, `res[0]`, "<>", `res[1]`
    else:
        if verbose:
            print 'yes'
    if res[1] != res[2]:
        if verbose:
            print 'no'
        print `input`, f, `res[1]`, "<>", `res[2]`
    else:
        if verbose:
            print 'yes'

test('capitalize', ' hello ')
test('capitalize', 'hello ')

test('center', 'foo', 0)
test('center', 'foo', 3)
test('center', 'foo', 16)

test('ljust', 'foo', 0)
test('ljust', 'foo', 3)
test('ljust', 'foo', 16)

test('rjust', 'foo', 0)
test('rjust', 'foo', 3)
test('rjust', 'foo', 16)

test('count', 'abcabcabc', 'abc')
test('count', 'abcabcabc', 'abc', 1)
test('count', 'abcabcabc', 'abc', -1)
test('count', 'abcabcabc', 'abc', 7)
test('count', 'abcabcabc', 'abc', 0, 3)
test('count', 'abcabcabc', 'abc', 0, 333)

test('find', 'abcdefghiabc', 'abc')
test('find', 'abcdefghiabc', 'abc', 1)
test('find', 'abcdefghiabc', 'def', 4)
test('rfind', 'abcdefghiabc', 'abc')

test('index', 'abcabcabc', 'abc')
test('index', 'abcabcabc', 'abc', 1)
test('index', 'abcabcabc', 'abc', -1)
test('index', 'abcabcabc', 'abc', 7)
test('index', 'abcabcabc', 'abc', 0, 3)
test('index', 'abcabcabc', 'abc', 0, 333)

test('rindex', 'abcabcabc', 'abc')
test('rindex', 'abcabcabc', 'abc', 1)
test('rindex', 'abcabcabc', 'abc', -1)
test('rindex', 'abcabcabc', 'abc', 7)
test('rindex', 'abcabcabc', 'abc', 0, 3)
test('rindex', 'abcabcabc', 'abc', 0, 333)


test('lower', 'HeLLo')
test('lower', 'hello')
test('upper', 'HeLLo')
test('upper', 'HELLO')

test('title', ' hello ')
test('title', 'hello ')
test('title', "fOrMaT thIs aS titLe String")
test('title', "fOrMaT,thIs-aS*titLe;String")
test('title', "getInt")

test('expandtabs', 'abc\rab\tdef\ng\thi')
test('expandtabs', 'abc\rab\tdef\ng\thi', 8)
test('expandtabs', 'abc\rab\tdef\ng\thi', 4)
test('expandtabs', 'abc\r\nab\tdef\ng\thi', 4)

test('islower', 'a')
test('islower', 'A')
test('islower', '\n')
test('islower', 'abc')
test('islower', 'aBc')
test('islower', 'abc\n')

test('isupper', 'a')
test('isupper', 'A')
test('isupper', '\n')
test('isupper', 'ABC')
test('isupper', 'AbC')
test('isupper', 'ABC\n')

test('isdigit', '  0123456789')
test('isdigit', '56789')
test('isdigit', '567.89')
test('isdigit', '0123456789abc')

test('isspace', '')
test('isspace', ' ')
test('isspace', ' \t')
test('isspace', ' \t\f\n')

test('istitle', 'a')
test('istitle', 'A')
test('istitle', '\n')
test('istitle', 'A Titlecased Line')
test('istitle', 'A\nTitlecased Line')
test('istitle', 'A Titlecased, Line')
test('istitle', 'Not a capitalized String')
test('istitle', 'Not\ta Titlecase String')
test('istitle', 'Not--a Titlecase String')

test('splitlines', "abc\ndef\n\rghi")
test('splitlines', "abc\ndef\n\r\nghi")
test('splitlines', "abc\ndef\r\nghi")
test('splitlines', "abc\ndef\r\nghi\n")
test('splitlines', "abc\ndef\r\nghi\n\r")
test('splitlines', "\nabc\ndef\r\nghi\n\r")
test('splitlines', "\nabc\ndef\r\nghi\n\r")
test('splitlines', "\nabc\ndef\r\nghi\n\r")

test('split', 'this is the split function')
test('split', 'a|b|c|d', '|')
test('split', 'a|b|c|d', '|', 2)
test('split', 'a b c d', None, 1)
test('split', 'a b c d', None, 2)
test('split', 'a b c d', None, 3)
test('split', 'a b c d', None, 4)
test('split', 'a b c d', None, 0)
test('split', 'a  b  c  d', None, 2)
test('split', 'a b c d ')

# join now works with any sequence type
class Sequence:
    def __init__(self): self.seq = 'wxyz'
    def __len__(self): return len(self.seq)
    def __getitem__(self, i): return self.seq[i]

test('join', '', ('a', 'b', 'c', 'd'))
test('join', '', Sequence())
test('join', '', 7)

class BadSeq(Sequence):
    def __init__(self): self.seq = [7, 'hello', 123L]

test('join', '', BadSeq())

test('strip', '   hello   ')
test('lstrip', '   hello   ')
test('rstrip', '   hello   ')
test('strip', 'hello')

test('swapcase', 'HeLLo cOmpUteRs')
transtable = string.maketrans("abc", "xyz")
test('translate', 'xyzabcdef', transtable, 'def')

transtable = string.maketrans('a', 'A')
test('translate', 'abc', transtable)
test('translate', 'xyz', transtable)

test('replace', 'one!two!three!', '!', '@', 1)
test('replace', 'one!two!three!', '!', '')
test('replace', 'one!two!three!', '!', '@', 2)
test('replace', 'one!two!three!', '!', '@', 3)
test('replace', 'one!two!three!', '!', '@', 4)
test('replace', 'one!two!three!', '!', '@', 0)
test('replace', 'one!two!three!', '!', '@')
test('replace', 'one!two!three!', 'x', '@')
test('replace', 'one!two!three!', 'x', '@', 2)

test('startswith', 'hello', 'he')
test('startswith', 'hello', 'hello')
test('startswith', 'hello', 'hello world')
test('startswith', 'hello', '')
test('startswith', 'hello', 'ello')
test('startswith', 'hello', 'ello', 1)
test('startswith', 'hello', 'o', 4)
test('startswith', 'hello', 'o', 5)
test('startswith', 'hello', '', 5)
test('startswith', 'hello', 'lo', 6)
test('startswith', 'helloworld', 'lowo', 3)
test('startswith', 'helloworld', 'lowo', 3, 7)
test('startswith', 'helloworld', 'lowo', 3, 6)

test('endswith', 'hello', 'lo')
test('endswith', 'hello', 'he')
test('endswith', 'hello', '')
test('endswith', 'hello', 'hello world')
test('endswith', 'helloworld', 'worl')
test('endswith', 'helloworld', 'worl', 3, 9)
test('endswith', 'helloworld', 'world', 3, 12)
test('endswith', 'helloworld', 'lowo', 1, 7)
test('endswith', 'helloworld', 'lowo', 2, 7)
test('endswith', 'helloworld', 'lowo', 3, 7)
test('endswith', 'helloworld', 'lowo', 4, 7)
test('endswith', 'helloworld', 'lowo', 3, 8)
test('endswith', 'ab', 'ab', 0, 1)
test('endswith', 'ab', 'ab', 0, 0)

# TODO: test cases for: int, long, float, complex, +, * and cmp
s = ""
for builtin_method in dir(s):
    if not tested_methods.has_key(builtin_method):
        print "no regression test case for method '"+builtin_method+"'"
