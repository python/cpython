"""
Common tests shared by test_str, test_unicode, test_userstring and test_string.
"""

import unittest, string, sys
from test import test_support
from UserList import UserList

class Sequence:
    def __init__(self, seq='wxyz'): self.seq = seq
    def __len__(self): return len(self.seq)
    def __getitem__(self, i): return self.seq[i]

class BadSeq1(Sequence):
    def __init__(self): self.seq = [7, 'hello', 123L]

class BadSeq2(Sequence):
    def __init__(self): self.seq = ['a', 'b', 'c']
    def __len__(self): return 8

class CommonTest(unittest.TestCase):
    # This testcase contains test that can be used in all
    # stringlike classes. Currently this is str, unicode
    # UserString and the string module.

    # The type to be tested
    # Change in subclasses to change the behaviour of fixtesttype()
    type2test = None

    # All tests pass their arguments to the testing methods
    # as str objects. fixtesttype() can be used to propagate
    # these arguments to the appropriate type
    def fixtype(self, obj):
        if isinstance(obj, str):
            return self.__class__.type2test(obj)
        elif isinstance(obj, list):
            return [self.fixtype(x) for x in obj]
        elif isinstance(obj, tuple):
            return tuple([self.fixtype(x) for x in obj])
        elif isinstance(obj, dict):
            return dict([
               (self.fixtype(key), self.fixtype(value))
               for (key, value) in obj.iteritems()
            ])
        else:
            return obj

    # check that object.method(*args) returns result
    def checkequal(self, result, object, methodname, *args):
        result = self.fixtype(result)
        object = self.fixtype(object)
        args = self.fixtype(args)
        realresult = getattr(object, methodname)(*args)
        self.assertEqual(
            result,
            realresult
        )
        # if the original is returned make sure that
        # this doesn't happen with subclasses
        if object == realresult:
            class subtype(self.__class__.type2test):
                pass
            object = subtype(object)
            realresult = getattr(object, methodname)(*args)
            self.assert_(object is not realresult)

    # check that object.method(*args) raises exc
    def checkraises(self, exc, object, methodname, *args):
        object = self.fixtype(object)
        args = self.fixtype(args)
        self.assertRaises(
            exc,
            getattr(object, methodname),
            *args
        )

    # call object.method(*args) without any checks
    def checkcall(self, object, methodname, *args):
        object = self.fixtype(object)
        args = self.fixtype(args)
        getattr(object, methodname)(*args)

    def test_capitalize(self):
        self.checkequal(' hello ', ' hello ', 'capitalize')
        self.checkequal('Hello ', 'Hello ','capitalize')
        self.checkequal('Hello ', 'hello ','capitalize')
        self.checkequal('Aaaa', 'aaaa', 'capitalize')
        self.checkequal('Aaaa', 'AaAa', 'capitalize')

        self.checkraises(TypeError, 'hello', 'capitalize', 42)

    def test_count(self):
        self.checkequal(3, 'aaa', 'count', 'a')
        self.checkequal(0, 'aaa', 'count', 'b')
        self.checkequal(3, 'aaa', 'count', 'a')
        self.checkequal(0, 'aaa', 'count', 'b')
        self.checkequal(3, 'aaa', 'count', 'a')
        self.checkequal(0, 'aaa', 'count', 'b')
        self.checkequal(0, 'aaa', 'count', 'b')
        self.checkequal(1, 'aaa', 'count', 'a', -1)
        self.checkequal(3, 'aaa', 'count', 'a', -10)
        self.checkequal(2, 'aaa', 'count', 'a', 0, -1)
        self.checkequal(0, 'aaa', 'count', 'a', 0, -10)

        self.checkraises(TypeError, 'hello', 'count')
        self.checkraises(TypeError, 'hello', 'count', 42)

    def test_find(self):
        self.checkequal(0, 'abcdefghiabc', 'find', 'abc')
        self.checkequal(9, 'abcdefghiabc', 'find', 'abc', 1)
        self.checkequal(-1, 'abcdefghiabc', 'find', 'def', 4)

        self.checkraises(TypeError, 'hello', 'find')
        self.checkraises(TypeError, 'hello', 'find', 42)

    def test_rfind(self):
        self.checkequal(9,  'abcdefghiabc', 'rfind', 'abc')
        self.checkequal(12, 'abcdefghiabc', 'rfind', '')
        self.checkequal(0, 'abcdefghiabc', 'rfind', 'abcd')
        self.checkequal(-1, 'abcdefghiabc', 'rfind', 'abcz')

        self.checkraises(TypeError, 'hello', 'rfind')
        self.checkraises(TypeError, 'hello', 'rfind', 42)

    def test_index(self):
        self.checkequal(0, 'abcdefghiabc', 'index', '')
        self.checkequal(3, 'abcdefghiabc', 'index', 'def')
        self.checkequal(0, 'abcdefghiabc', 'index', 'abc')
        self.checkequal(9, 'abcdefghiabc', 'index', 'abc', 1)

        self.checkraises(ValueError, 'abcdefghiabc', 'index', 'hib')
        self.checkraises(ValueError, 'abcdefghiab', 'index', 'abc', 1)
        self.checkraises(ValueError, 'abcdefghi', 'index', 'ghi', 8)
        self.checkraises(ValueError, 'abcdefghi', 'index', 'ghi', -1)

        self.checkraises(TypeError, 'hello', 'index')
        self.checkraises(TypeError, 'hello', 'index', 42)

    def test_rindex(self):
        self.checkequal(12, 'abcdefghiabc', 'rindex', '')
        self.checkequal(3,  'abcdefghiabc', 'rindex', 'def')
        self.checkequal(9,  'abcdefghiabc', 'rindex', 'abc')
        self.checkequal(0,  'abcdefghiabc', 'rindex', 'abc', 0, -1)

        self.checkraises(ValueError, 'abcdefghiabc', 'rindex', 'hib')
        self.checkraises(ValueError, 'defghiabc', 'rindex', 'def', 1)
        self.checkraises(ValueError, 'defghiabc', 'rindex', 'abc', 0, -1)
        self.checkraises(ValueError, 'abcdefghi', 'rindex', 'ghi', 0, 8)
        self.checkraises(ValueError, 'abcdefghi', 'rindex', 'ghi', 0, -1)

        self.checkraises(TypeError, 'hello', 'rindex')
        self.checkraises(TypeError, 'hello', 'rindex', 42)

    def test_lower(self):
        self.checkequal('hello', 'HeLLo', 'lower')
        self.checkequal('hello', 'hello', 'lower')
        self.checkraises(TypeError, 'hello', 'lower', 42)

    def test_upper(self):
        self.checkequal('HELLO', 'HeLLo', 'upper')
        self.checkequal('HELLO', 'HELLO', 'upper')
        self.checkraises(TypeError, 'hello', 'upper', 42)

    def test_expandtabs(self):
        self.checkequal('abc\rab      def\ng       hi', 'abc\rab\tdef\ng\thi', 'expandtabs')
        self.checkequal('abc\rab      def\ng       hi', 'abc\rab\tdef\ng\thi', 'expandtabs', 8)
        self.checkequal('abc\rab  def\ng   hi', 'abc\rab\tdef\ng\thi', 'expandtabs', 4)
        self.checkequal('abc\r\nab  def\ng   hi', 'abc\r\nab\tdef\ng\thi', 'expandtabs', 4)
        self.checkequal('abc\rab      def\ng       hi', 'abc\rab\tdef\ng\thi', 'expandtabs')
        self.checkequal('abc\rab      def\ng       hi', 'abc\rab\tdef\ng\thi', 'expandtabs', 8)
        self.checkequal('abc\r\nab\r\ndef\ng\r\nhi', 'abc\r\nab\r\ndef\ng\r\nhi', 'expandtabs', 4)

        self.checkraises(TypeError, 'hello', 'expandtabs', 42, 42)

    def test_split(self):
        self.checkequal(['this', 'is', 'the', 'split', 'function'],
            'this is the split function', 'split')
        self.checkequal(['a', 'b', 'c', 'd'], 'a|b|c|d', 'split', '|')
        self.checkequal(['a', 'b', 'c|d'], 'a|b|c|d', 'split', '|', 2)
        self.checkequal(['a', 'b c d'], 'a b c d', 'split', None, 1)
        self.checkequal(['a', 'b', 'c d'], 'a b c d', 'split', None, 2)
        self.checkequal(['a', 'b', 'c', 'd'], 'a b c d', 'split', None, 3)
        self.checkequal(['a', 'b', 'c', 'd'], 'a b c d', 'split', None, 4)
        self.checkequal(['a b c d'], 'a b c d', 'split', None, 0)
        self.checkequal(['a', 'b', 'c  d'], 'a  b  c  d', 'split', None, 2)
        self.checkequal(['a', 'b', 'c', 'd'], 'a b c d ', 'split')
        self.checkequal(['a', 'b', 'c', 'd'], 'a//b//c//d', 'split', '//')
        self.checkequal(['endcase ', ''], 'endcase test', 'split', 'test')

        self.checkraises(TypeError, 'hello', 'split', 42, 42, 42)

    def test_strip(self):
        self.checkequal('hello', '   hello   ', 'strip')
        self.checkequal('hello   ', '   hello   ', 'lstrip')
        self.checkequal('   hello', '   hello   ', 'rstrip')
        self.checkequal('hello', 'hello', 'strip')

        # strip/lstrip/rstrip with None arg
        self.checkequal('hello', '   hello   ', 'strip', None)
        self.checkequal('hello   ', '   hello   ', 'lstrip', None)
        self.checkequal('   hello', '   hello   ', 'rstrip', None)
        self.checkequal('hello', 'hello', 'strip', None)

        # strip/lstrip/rstrip with str arg
        self.checkequal('hello', 'xyzzyhelloxyzzy', 'strip', 'xyz')
        self.checkequal('helloxyzzy', 'xyzzyhelloxyzzy', 'lstrip', 'xyz')
        self.checkequal('xyzzyhello', 'xyzzyhelloxyzzy', 'rstrip', 'xyz')
        self.checkequal('hello', 'hello', 'strip', 'xyz')

        # strip/lstrip/rstrip with unicode arg
        if test_support.have_unicode:
            self.checkequal(unicode('hello', 'ascii'), 'xyzzyhelloxyzzy',
                 'strip', unicode('xyz', 'ascii'))
            self.checkequal(unicode('helloxyzzy', 'ascii'), 'xyzzyhelloxyzzy',
                 'lstrip', unicode('xyz', 'ascii'))
            self.checkequal(unicode('xyzzyhello', 'ascii'), 'xyzzyhelloxyzzy',
                 'rstrip', unicode('xyz', 'ascii'))
            self.checkequal(unicode('hello', 'ascii'), 'hello',
                 'strip', unicode('xyz', 'ascii'))

        self.checkraises(TypeError, 'hello', 'strip', 42, 42)
        self.checkraises(TypeError, 'hello', 'lstrip', 42, 42)
        self.checkraises(TypeError, 'hello', 'rstrip', 42, 42)

    def test_ljust(self):
        self.checkequal('abc       ', 'abc', 'ljust', 10)
        self.checkequal('abc   ', 'abc', 'ljust', 6)
        self.checkequal('abc', 'abc', 'ljust', 3)
        self.checkequal('abc', 'abc', 'ljust', 2)

        self.checkraises(TypeError, 'abc', 'ljust')

    def test_rjust(self):
        self.checkequal('       abc', 'abc', 'rjust', 10)
        self.checkequal('   abc', 'abc', 'rjust', 6)
        self.checkequal('abc', 'abc', 'rjust', 3)
        self.checkequal('abc', 'abc', 'rjust', 2)

        self.checkraises(TypeError, 'abc', 'rjust')

    def test_center(self):
        self.checkequal('   abc    ', 'abc', 'center', 10)
        self.checkequal(' abc  ', 'abc', 'center', 6)
        self.checkequal('abc', 'abc', 'center', 3)
        self.checkequal('abc', 'abc', 'center', 2)

        self.checkraises(TypeError, 'abc', 'center')

    def test_swapcase(self):
        self.checkequal('hEllO CoMPuTErS', 'HeLLo cOmpUteRs', 'swapcase')

        self.checkraises(TypeError, 'hello', 'swapcase', 42)

    def test_replace(self):
        self.checkequal('one@two!three!', 'one!two!three!', 'replace', '!', '@', 1)
        self.checkequal('onetwothree', 'one!two!three!', 'replace', '!', '')
        self.checkequal('one@two@three!', 'one!two!three!', 'replace', '!', '@', 2)
        self.checkequal('one@two@three@', 'one!two!three!', 'replace', '!', '@', 3)
        self.checkequal('one@two@three@', 'one!two!three!', 'replace', '!', '@', 4)
        self.checkequal('one!two!three!', 'one!two!three!', 'replace', '!', '@', 0)
        self.checkequal('one@two@three@', 'one!two!three!', 'replace', '!', '@')
        self.checkequal('one!two!three!', 'one!two!three!', 'replace', 'x', '@')
        self.checkequal('one!two!three!', 'one!two!three!', 'replace', 'x', '@', 2)
        self.checkequal('-a-b-c-', 'abc', 'replace', '', '-')
        self.checkequal('-a-b-c', 'abc', 'replace', '', '-', 3)
        self.checkequal('abc', 'abc', 'replace', '', '-', 0)
        self.checkequal('', '', 'replace', '', '')
        self.checkequal('abc', 'abc', 'replace', 'ab', '--', 0)
        self.checkequal('abc', 'abc', 'replace', 'xy', '--')
        # Next three for SF bug 422088: [OSF1 alpha] string.replace(); died with
        # MemoryError due to empty result (platform malloc issue when requesting
        # 0 bytes).
        self.checkequal('', '123', 'replace', '123', '')
        self.checkequal('', '123123', 'replace', '123', '')
        self.checkequal('x', '123x123', 'replace', '123', '')

        self.checkraises(TypeError, 'hello', 'replace')
        self.checkraises(TypeError, 'hello', 'replace', 42)
        self.checkraises(TypeError, 'hello', 'replace', 42, 'h')
        self.checkraises(TypeError, 'hello', 'replace', 'h', 42)

    def test_zfill(self):
        self.checkequal('123', '123', 'zfill', 2)
        self.checkequal('123', '123', 'zfill', 3)
        self.checkequal('0123', '123', 'zfill', 4)
        self.checkequal('+123', '+123', 'zfill', 3)
        self.checkequal('+123', '+123', 'zfill', 4)
        self.checkequal('+0123', '+123', 'zfill', 5)
        self.checkequal('-123', '-123', 'zfill', 3)
        self.checkequal('-123', '-123', 'zfill', 4)
        self.checkequal('-0123', '-123', 'zfill', 5)
        self.checkequal('000', '', 'zfill', 3)
        self.checkequal('34', '34', 'zfill', 1)
        self.checkequal('0034', '34', 'zfill', 4)

        self.checkraises(TypeError, '123', 'zfill')

class MixinStrUnicodeUserStringTest:
    # additional tests that only work for
    # stringlike objects, i.e. str, unicode, UserString
    # (but not the string module)

    def test_islower(self):
        self.checkequal(False, '', 'islower')
        self.checkequal(True, 'a', 'islower')
        self.checkequal(False, 'A', 'islower')
        self.checkequal(False, '\n', 'islower')
        self.checkequal(True, 'abc', 'islower')
        self.checkequal(False, 'aBc', 'islower')
        self.checkequal(True, 'abc\n', 'islower')
        self.checkraises(TypeError, 'abc', 'islower', 42)

    def test_isupper(self):
        self.checkequal(False, '', 'isupper')
        self.checkequal(False, 'a', 'isupper')
        self.checkequal(True, 'A', 'isupper')
        self.checkequal(False, '\n', 'isupper')
        self.checkequal(True, 'ABC', 'isupper')
        self.checkequal(False, 'AbC', 'isupper')
        self.checkequal(True, 'ABC\n', 'isupper')
        self.checkraises(TypeError, 'abc', 'isupper', 42)

    def test_istitle(self):
        self.checkequal(False, '', 'istitle')
        self.checkequal(False, 'a', 'istitle')
        self.checkequal(True, 'A', 'istitle')
        self.checkequal(False, '\n', 'istitle')
        self.checkequal(True, 'A Titlecased Line', 'istitle')
        self.checkequal(True, 'A\nTitlecased Line', 'istitle')
        self.checkequal(True, 'A Titlecased, Line', 'istitle')
        self.checkequal(False, 'Not a capitalized String', 'istitle')
        self.checkequal(False, 'Not\ta Titlecase String', 'istitle')
        self.checkequal(False, 'Not--a Titlecase String', 'istitle')
        self.checkequal(False, 'NOT', 'istitle')
        self.checkraises(TypeError, 'abc', 'istitle', 42)

    def test_isspace(self):
        self.checkequal(False, '', 'isspace')
        self.checkequal(False, 'a', 'isspace')
        self.checkequal(True, ' ', 'isspace')
        self.checkequal(True, '\t', 'isspace')
        self.checkequal(True, '\r', 'isspace')
        self.checkequal(True, '\n', 'isspace')
        self.checkequal(True, ' \t\r\n', 'isspace')
        self.checkequal(False, ' \t\r\na', 'isspace')
        self.checkraises(TypeError, 'abc', 'isspace', 42)

    def test_isalpha(self):
        self.checkequal(False, '', 'isalpha')
        self.checkequal(True, 'a', 'isalpha')
        self.checkequal(True, 'A', 'isalpha')
        self.checkequal(False, '\n', 'isalpha')
        self.checkequal(True, 'abc', 'isalpha')
        self.checkequal(False, 'aBc123', 'isalpha')
        self.checkequal(False, 'abc\n', 'isalpha')
        self.checkraises(TypeError, 'abc', 'isalpha', 42)

    def test_isalnum(self):
        self.checkequal(False, '', 'isalnum')
        self.checkequal(True, 'a', 'isalnum')
        self.checkequal(True, 'A', 'isalnum')
        self.checkequal(False, '\n', 'isalnum')
        self.checkequal(True, '123abc456', 'isalnum')
        self.checkequal(True, 'a1b3c', 'isalnum')
        self.checkequal(False, 'aBc000 ', 'isalnum')
        self.checkequal(False, 'abc\n', 'isalnum')
        self.checkraises(TypeError, 'abc', 'isalnum', 42)

    def test_isdigit(self):
        self.checkequal(False, '', 'isdigit')
        self.checkequal(False, 'a', 'isdigit')
        self.checkequal(True, '0', 'isdigit')
        self.checkequal(True, '0123456789', 'isdigit')
        self.checkequal(False, '0123456789a', 'isdigit')

        self.checkraises(TypeError, 'abc', 'isdigit', 42)

    def test_title(self):
        self.checkequal(' Hello ', ' hello ', 'title')
        self.checkequal('Hello ', 'hello ', 'title')
        self.checkequal('Hello ', 'Hello ', 'title')
        self.checkequal('Format This As Title String', "fOrMaT thIs aS titLe String", 'title')
        self.checkequal('Format,This-As*Title;String', "fOrMaT,thIs-aS*titLe;String", 'title', )
        self.checkequal('Getint', "getInt", 'title')
        self.checkraises(TypeError, 'hello', 'title', 42)

    def test_splitlines(self):
        self.checkequal(['abc', 'def', '', 'ghi'], "abc\ndef\n\rghi", 'splitlines')
        self.checkequal(['abc', 'def', '', 'ghi'], "abc\ndef\n\r\nghi", 'splitlines')
        self.checkequal(['abc', 'def', 'ghi'], "abc\ndef\r\nghi", 'splitlines')
        self.checkequal(['abc', 'def', 'ghi'], "abc\ndef\r\nghi\n", 'splitlines')
        self.checkequal(['abc', 'def', 'ghi', ''], "abc\ndef\r\nghi\n\r", 'splitlines')
        self.checkequal(['', 'abc', 'def', 'ghi', ''], "\nabc\ndef\r\nghi\n\r", 'splitlines')
        self.checkequal(['\n', 'abc\n', 'def\r\n', 'ghi\n', '\r'], "\nabc\ndef\r\nghi\n\r", 'splitlines', 1)

        self.checkraises(TypeError, 'abc', 'splitlines', 42, 42)

    def test_startswith(self):
        self.checkequal(True, 'hello', 'startswith', 'he')
        self.checkequal(True, 'hello', 'startswith', 'hello')
        self.checkequal(False, 'hello', 'startswith', 'hello world')
        self.checkequal(True, 'hello', 'startswith', '')
        self.checkequal(False, 'hello', 'startswith', 'ello')
        self.checkequal(True, 'hello', 'startswith', 'ello', 1)
        self.checkequal(True, 'hello', 'startswith', 'o', 4)
        self.checkequal(False, 'hello', 'startswith', 'o', 5)
        self.checkequal(True, 'hello', 'startswith', '', 5)
        self.checkequal(False, 'hello', 'startswith', 'lo', 6)
        self.checkequal(True, 'helloworld', 'startswith', 'lowo', 3)
        self.checkequal(True, 'helloworld', 'startswith', 'lowo', 3, 7)
        self.checkequal(False, 'helloworld', 'startswith', 'lowo', 3, 6)

        # test negative indices
        self.checkequal(True, 'hello', 'startswith', 'he', 0, -1)
        self.checkequal(True, 'hello', 'startswith', 'he', -53, -1)
        self.checkequal(False, 'hello', 'startswith', 'hello', 0, -1)
        self.checkequal(False, 'hello', 'startswith', 'hello world', -1, -10)
        self.checkequal(False, 'hello', 'startswith', 'ello', -5)
        self.checkequal(True, 'hello', 'startswith', 'ello', -4)
        self.checkequal(False, 'hello', 'startswith', 'o', -2)
        self.checkequal(True, 'hello', 'startswith', 'o', -1)
        self.checkequal(True, 'hello', 'startswith', '', -3, -3)
        self.checkequal(False, 'hello', 'startswith', 'lo', -9)

        self.checkraises(TypeError, 'hello', 'startswith')
        self.checkraises(TypeError, 'hello', 'startswith', 42)

    def test_endswith(self):
        self.checkequal(True, 'hello', 'endswith', 'lo')
        self.checkequal(False, 'hello', 'endswith', 'he')
        self.checkequal(True, 'hello', 'endswith', '')
        self.checkequal(False, 'hello', 'endswith', 'hello world')
        self.checkequal(False, 'helloworld', 'endswith', 'worl')
        self.checkequal(True, 'helloworld', 'endswith', 'worl', 3, 9)
        self.checkequal(True, 'helloworld', 'endswith', 'world', 3, 12)
        self.checkequal(True, 'helloworld', 'endswith', 'lowo', 1, 7)
        self.checkequal(True, 'helloworld', 'endswith', 'lowo', 2, 7)
        self.checkequal(True, 'helloworld', 'endswith', 'lowo', 3, 7)
        self.checkequal(False, 'helloworld', 'endswith', 'lowo', 4, 7)
        self.checkequal(False, 'helloworld', 'endswith', 'lowo', 3, 8)
        self.checkequal(False, 'ab', 'endswith', 'ab', 0, 1)
        self.checkequal(False, 'ab', 'endswith', 'ab', 0, 0)

        # test negative indices
        self.checkequal(True, 'hello', 'endswith', 'lo', -2)
        self.checkequal(False, 'hello', 'endswith', 'he', -2)
        self.checkequal(True, 'hello', 'endswith', '', -3, -3)
        self.checkequal(False, 'hello', 'endswith', 'hello world', -10, -2)
        self.checkequal(False, 'helloworld', 'endswith', 'worl', -6)
        self.checkequal(True, 'helloworld', 'endswith', 'worl', -5, -1)
        self.checkequal(True, 'helloworld', 'endswith', 'worl', -5, 9)
        self.checkequal(True, 'helloworld', 'endswith', 'world', -7, 12)
        self.checkequal(True, 'helloworld', 'endswith', 'lowo', -99, -3)
        self.checkequal(True, 'helloworld', 'endswith', 'lowo', -8, -3)
        self.checkequal(True, 'helloworld', 'endswith', 'lowo', -7, -3)
        self.checkequal(False, 'helloworld', 'endswith', 'lowo', 3, -4)
        self.checkequal(False, 'helloworld', 'endswith', 'lowo', -8, -2)

        self.checkraises(TypeError, 'hello', 'endswith')
        self.checkraises(TypeError, 'hello', 'endswith', 42)

    def test___contains__(self):
        self.checkequal(True, '', '__contains__', '')         # vereq('' in '', True)
        self.checkequal(True, 'abc', '__contains__', '')      # vereq('' in 'abc', True)
        self.checkequal(False, 'abc', '__contains__', '\0')   # vereq('\0' in 'abc', False)
        self.checkequal(True, '\0abc', '__contains__', '\0')  # vereq('\0' in '\0abc', True)
        self.checkequal(True, 'abc\0', '__contains__', '\0')  # vereq('\0' in 'abc\0', True)
        self.checkequal(True, '\0abc', '__contains__', 'a')   # vereq('a' in '\0abc', True)
        self.checkequal(True, 'asdf', '__contains__', 'asdf') # vereq('asdf' in 'asdf', True)
        self.checkequal(False, 'asd', '__contains__', 'asdf') # vereq('asdf' in 'asd', False)
        self.checkequal(False, '', '__contains__', 'asdf')    # vereq('asdf' in '', False)

    def test_subscript(self):
        self.checkequal(u'a', 'abc', '__getitem__', 0)
        self.checkequal(u'c', 'abc', '__getitem__', -1)
        self.checkequal(u'a', 'abc', '__getitem__', 0L)
        self.checkequal(u'abc', 'abc', '__getitem__', slice(0, 3))
        self.checkequal(u'abc', 'abc', '__getitem__', slice(0, 1000))
        self.checkequal(u'a', 'abc', '__getitem__', slice(0, 1))
        self.checkequal(u'', 'abc', '__getitem__', slice(0, 0))
        # FIXME What about negative indizes? This is handled differently by [] and __getitem__(slice)

        self.checkraises(TypeError, 'abc', '__getitem__', 'def')

    def test_slice(self):
        self.checkequal('abc', 'abc', '__getslice__', 0, 1000)
        self.checkequal('abc', 'abc', '__getslice__', 0, 3)
        self.checkequal('ab', 'abc', '__getslice__', 0, 2)
        self.checkequal('bc', 'abc', '__getslice__', 1, 3)
        self.checkequal('b', 'abc', '__getslice__', 1, 2)
        self.checkequal('', 'abc', '__getslice__', 2, 2)
        self.checkequal('', 'abc', '__getslice__', 1000, 1000)
        self.checkequal('', 'abc', '__getslice__', 2000, 1000)
        self.checkequal('', 'abc', '__getslice__', 2, 1)
        # FIXME What about negative indizes? This is handled differently by [] and __getslice__

        self.checkraises(TypeError, 'abc', '__getslice__', 'def')

    def test_mul(self):
        self.checkequal('', 'abc', '__mul__', -1)
        self.checkequal('', 'abc', '__mul__', 0)
        self.checkequal('abc', 'abc', '__mul__', 1)
        self.checkequal('abcabcabc', 'abc', '__mul__', 3)
        self.checkraises(TypeError, 'abc', '__mul__')
        self.checkraises(TypeError, 'abc', '__mul__', '')
        self.checkraises(OverflowError, 10000*'abc', '__mul__', 2000000000)

    def test_join(self):
        # join now works with any sequence type
        # moved here, because the argument order is
        # different in string.join (see the test in
        # test.test_string.StringTest.test_join)
        self.checkequal('a b c d', ' ', 'join', ['a', 'b', 'c', 'd'])
        self.checkequal('abcd', '', 'join', ('a', 'b', 'c', 'd'))
        self.checkequal('w x y z', ' ', 'join', Sequence())
        self.checkequal('abc', 'a', 'join', ('abc',))
        self.checkequal('z', 'a', 'join', UserList(['z']))
        if test_support.have_unicode:
            self.checkequal(unicode('a.b.c'), unicode('.'), 'join', ['a', 'b', 'c'])
            self.checkequal(unicode('a.b.c'), '.', 'join', [unicode('a'), 'b', 'c'])
            self.checkequal(unicode('a.b.c'), '.', 'join', ['a', unicode('b'), 'c'])
            self.checkequal(unicode('a.b.c'), '.', 'join', ['a', 'b', unicode('c')])
            self.checkraises(TypeError, '.', 'join', ['a', unicode('b'), 3])
        for i in [5, 25, 125]:
            self.checkequal(((('a' * i) + '-') * i)[:-1], '-', 'join',
                 ['a' * i] * i)
            self.checkequal(((('a' * i) + '-') * i)[:-1], '-', 'join',
                 ('a' * i,) * i)

        self.checkraises(TypeError, ' ', 'join', BadSeq1())
        self.checkequal('a b c', ' ', 'join', BadSeq2())

        self.checkraises(TypeError, ' ', 'join')
        self.checkraises(TypeError, ' ', 'join', 7)
        self.checkraises(TypeError, ' ', 'join', Sequence([7, 'hello', 123L]))

    def test_formatting(self):
        self.checkequal('+hello+', '+%s+', '__mod__', 'hello')
        self.checkequal('+10+', '+%d+', '__mod__', 10)
        self.checkequal('a', "%c", '__mod__', "a")
        self.checkequal('a', "%c", '__mod__', "a")
        self.checkequal('"', "%c", '__mod__', 34)
        self.checkequal('$', "%c", '__mod__', 36)
        self.checkequal('10', "%d", '__mod__', 10)
        self.checkequal('\x7f', "%c", '__mod__', 0x7f)

        for ordinal in (-100, 0x200000):
            # unicode raises ValueError, str raises OverflowError
            self.checkraises((ValueError, OverflowError), '%c', '__mod__', ordinal)

        self.checkequal(' 42', '%3ld', '__mod__', 42)
        self.checkequal('0042.00', '%07.2f', '__mod__', 42)
        self.checkequal('0042.00', '%07.2F', '__mod__', 42)

        self.checkraises(TypeError, 'abc', '__mod__')
        self.checkraises(TypeError, '%(foo)s', '__mod__', 42)
        self.checkraises(TypeError, '%s%s', '__mod__', (42,))
        self.checkraises(TypeError, '%c', '__mod__', (None,))
        self.checkraises(ValueError, '%(foo', '__mod__', {})
        self.checkraises(TypeError, '%(foo)s %(bar)s', '__mod__', ('foo', 42))

        # argument names with properly nested brackets are supported
        self.checkequal('bar', '%((foo))s', '__mod__', {'(foo)': 'bar'})

        # 100 is a magic number in PyUnicode_Format, this forces a resize
        self.checkequal(103*'a'+'x', '%sx', '__mod__', 103*'a')

        self.checkraises(TypeError, '%*s', '__mod__', ('foo', 'bar'))
        self.checkraises(TypeError, '%10.*f', '__mod__', ('foo', 42.))
        self.checkraises(ValueError, '%10', '__mod__', (42,))

    def test_floatformatting(self):
        # float formatting
        for prec in xrange(100):
            format = '%%.%if' % prec
            value = 0.01
            for x in xrange(60):
                value = value * 3.141592655 / 3.0 * 10.0
                # The formatfloat() code in stringobject.c and
                # unicodeobject.c uses a 120 byte buffer and switches from
                # 'f' formatting to 'g' at precision 50, so we expect
                # OverflowErrors for the ranges x < 50 and prec >= 67.
                if x < 50 and prec >= 67:
                    self.checkraises(OverflowError, format, "__mod__", value)
                else:
                    self.checkcall(format, "__mod__", value)

class MixinStrStringUserStringTest:
    # Additional tests for 8bit strings, i.e. str, UserString and
    # the string module

    def test_maketrans(self):
        self.assertEqual(
           ''.join(map(chr, xrange(256))).replace('abc', 'xyz'),
           string.maketrans('abc', 'xyz')
        )
        self.assertRaises(ValueError, string.maketrans, 'abc', 'xyzw')

    def test_translate(self):
        table = string.maketrans('abc', 'xyz')
        self.checkequal('xyzxyz', 'xyzabcdef', 'translate', table, 'def')

        table = string.maketrans('a', 'A')
        self.checkequal('Abc', 'abc', 'translate', table)
        self.checkequal('xyz', 'xyz', 'translate', table)
        self.checkequal('yz', 'xyz', 'translate', table, 'x')
        self.checkraises(ValueError, 'xyz', 'translate', 'too short', 'strip')
        self.checkraises(ValueError, 'xyz', 'translate', 'too short')


class MixinStrUserStringTest:
    # Additional tests that only work with
    # 8bit compatible object, i.e. str and UserString

    def test_encoding_decoding(self):
        codecs = [('rot13', 'uryyb jbeyq'),
                  ('base64', 'aGVsbG8gd29ybGQ=\n'),
                  ('hex', '68656c6c6f20776f726c64'),
                  ('uu', 'begin 666 <data>\n+:&5L;&\\@=V]R;&0 \n \nend\n')]
        for encoding, data in codecs:
            self.checkequal(data, 'hello world', 'encode', encoding)
            self.checkequal('hello world', data, 'decode', encoding)
        # zlib is optional, so we make the test optional too...
        try:
            import zlib
        except ImportError:
            pass
        else:
            data = 'x\x9c\xcbH\xcd\xc9\xc9W(\xcf/\xcaI\x01\x00\x1a\x0b\x04]'
            self.checkequal(data, 'hello world', 'encode', 'zlib')
            self.checkequal('hello world', data, 'decode', 'zlib')

        self.checkraises(TypeError, 'xyz', 'decode', 42)
        self.checkraises(TypeError, 'xyz', 'encode', 42)
