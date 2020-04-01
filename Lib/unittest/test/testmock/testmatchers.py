import random
import re
import unittest
from unittest import mock
from unittest.mock import matchers

class TestHelpers(unittest.TestCase):

    def testNOTNONE(self):
        self.assertTrue(matchers.NOTNONE == 1)
        self.assertTrue(matchers.NOTNONE == 0)
        self.assertTrue(matchers.NOTNONE == False)
        self.assertTrue(matchers.NOTNONE != None)

    def testREGEXP(self):
        self.assertRaises(TypeError, matchers.REGEXP, 123)
        self.assertRaises(TypeError, matchers.REGEXP, _FakeRe1())
        self.assertRaises(TypeError, matchers.REGEXP, _FakeRe2())
        self.assertRaises(ValueError, matchers.REGEXP, '[')
        self.assertTrue(matchers.REGEXP('[123]') == '1')
        self.assertTrue(matchers.REGEXP('[123]') != '4')
        self.assertFalse(matchers.REGEXP(r'\d') == 'A')
        self.assertFalse(matchers.REGEXP(re.compile('[a-z]')) == 'A')

    def testONEOF(self):
        self.assertRaises(TypeError, matchers.ONEOF, 123)
        self.assertRaises(ValueError, matchers.ONEOF, [])
        self.assertTrue(matchers.ONEOF('abc') == 'a')
        self.assertFalse(matchers.ONEOF('abc') != 'b')
        self.assertFalse(matchers.ONEOF('abc') == 'd')
        self.assertTrue(matchers.ONEOF('abc') != 'e')
        self.assertTrue(matchers.ONEOF([1, 2, 3]) == 2)

    def testHAS(self):
        self.assertTrue(matchers.HAS('abc') == ['a', 'ab', 'abc'])
        self.assertTrue(
            matchers.HAS(2, lambda s: map(len, s)) == ['a', 'ab', 'abc'])
        self.assertRaises(TypeError, matchers.HAS, *(0, 1))

    def testHASALLOF(self):
        self.assertTrue(matchers.HASALLOF(1, 2, 3) == [1, 2, 3, 4])
        self.assertTrue(matchers.HASALLOF(1, 2, 3) == [1, 2, 3, [4, 5]])
        self.assertTrue(matchers.HASALLOF(1, 2, [4, 5]) == [1, 2, 3, [4, 5]])
        self.assertFalse(matchers.HASALLOF() == ['a', 'b'])
        self.assertFalse(matchers.HASALLOF(1, 2, [3, 4]) == [1, 2, 3])
        self.assertFalse(matchers.HASALLOF(1, 2, 10) == [1, 2, 3])
        with self.assertRaises(TypeError):
            matchers.HASALLOF('a') == 1

    def testIS(self):
        self.assertTrue(matchers.IS(callable) == len)
        self.assertTrue(matchers.IS(lambda x: x > 0) == 1)

    def testISForNonFunctions(self):

        class CustomAny1(object):

            def __call__(self, one_arg):
                return True

        class CallableNoArgs(object):

            def __call__(self):
                pass

        class NotCallable(object):

            __call__ = 'not callable'

        self.assertRaises(TypeError, matchers.IS, None)
        self.assertTrue(matchers.IS(CustomAny1()) == object())
        self.assertRaises(TypeError, matchers.IS, CallableNoArgs())
        self.assertRaises(TypeError, matchers.IS, NotCallable())

    def testFuncArgCount(self):
        self.assertEqual(1, matchers._FuncArgCount(lambda a: None))
        self.assertEqual(3, matchers._FuncArgCount(lambda a, b, c: None))

        class Foo(object):

            def method(self, arg):
                pass

        # Bound methods ignore the self arg.
        self.assertEqual(1, matchers._FuncArgCount(Foo().method))

        # Unbound methods need the self arg.
        self.assertEqual(2, matchers._FuncArgCount(Foo.method))

    def testFuncArgCountForChainedCallables(self):

        class Wrapper(object):

            def __init__(self, attr):
                pass

            def __call__(self, a, b, c):
                pass

        class Foo(object):

            @Wrapper
            def __call__(self, a):
                pass

        # Uses Wrapper's __call__, as bound to the instance created in the decorator
        self.assertEqual(3, matchers._FuncArgCount(Foo()))

    def testINSTANCEOF(self):
        self.assertRaises(TypeError, matchers.INSTANCEOF, (None))
        self.assertRaises(TypeError, matchers.INSTANCEOF, (1, 2))
        self.assertTrue(matchers.INSTANCEOF(int) == 1)
        self.assertTrue(matchers.INSTANCEOF((int, str)) == 1)
        self.assertTrue(matchers.INSTANCEOF(str) != 23)
        self.assertTrue(matchers.INSTANCEOF((int, str)) == 'string')
        self.assertTrue(matchers.INSTANCEOF(int) != b'bytes')

    def testEQUIV(self):
        self.assertTrue(matchers.EQUIV(sorted, [1, 2]) == [1, 2])
        self.assertFalse(matchers.EQUIV(sorted, [1, 2]) != [1, 2])
        self.assertTrue(matchers.EQUIV(sorted, [1, 2]) == [2, 1])
        self.assertFalse(matchers.EQUIV(sorted, [1, 2]) != [2, 1])
        self.assertFalse(matchers.EQUIV(sorted, [1, 2]) == [1, 2, 3])
        self.assertTrue(matchers.EQUIV(sorted, [1, 2]) != [1, 2, 3])
        self.assertTrue(matchers.EQUIV(set, [1, 2]) == [1, 2, 2])
        self.assertFalse(matchers.EQUIV(set, [1, 2]) != [1, 2, 2])
        # pow takes two args, not one
        self.assertRaises(TypeError, matchers.EQUIV, pow, 2)

    def testHASKEYVALUE(self):
        self.assertTrue(matchers.HASKEYVALUE('a', 1) == {'a': 1})
        self.assertTrue(matchers.HASKEYVALUE('a', 1) == {'a': 1, 'b': 2})
        self.assertFalse(matchers.HASKEYVALUE('a', 1) == {'a': 2})
        self.assertFalse(matchers.HASKEYVALUE('b', 1) == {'a': 2})

    def testHASATTRVALUE(self):
        t = type('T', (), {'a': 1})
        o = t()
        self.assertTrue(matchers.HASATTRVALUE('a', 1) == o)
        self.assertFalse(matchers.HASATTRVALUE('a', 2) == o)
        self.assertFalse(matchers.HASATTRVALUE('b', 1) == o)
        self.assertFalse(matchers.HASATTRVALUE(None, 1) == o)

    def testALLOF(self):
        self.assertTrue(
            matchers.ALLOF(
                matchers.HASKEYVALUE('a', 1), matchers.HASKEYVALUE('b', 2)) == {
                    'a': 1,
                    'b': 2
                })
        self.assertTrue(
            matchers.ALLOF(matchers.HAS('iter'), 'literal') == 'literal')
        self.assertFalse(matchers.ALLOF() == 'anything')
        self.assertFalse(
            matchers.ALLOF(
                matchers.HASKEYVALUE('a', 1), matchers.HASKEYVALUE('c', 3)) == {
                    'a': 1,
                    'b': 2
                })

    def testANYOF(self):
        self.assertTrue(
            matchers.ANYOF(
                matchers.HASKEYVALUE('a', 1), matchers.HASKEYVALUE('c', 3)) == {
                    'a': 1,
                    'b': 2
                })
        self.assertFalse(matchers.ANYOF() == 'anything')
        self.assertFalse(
            matchers.ANYOF(
                matchers.HASKEYVALUE('a', 1), matchers.HASKEYVALUE('b', 2)) == {
                    'c': 3,
                    'd': 4
                })

    def testNOT(self):
        self.assertTrue(matchers.NOT(matchers.HASKEYVALUE('a', 1)) == {'b': 2})
        self.assertFalse(matchers.NOT(matchers.HAS('abc')) == ['a', 'ab', 'abc'])
        self.assertFalse(matchers.NOT(matchers.NOTNONE) != None)

    def testHASMETHODVALUE(self):

        class Klass(object):

            def __init__(self, value):
                self._value = value

            def twice(self):
                return self._value * 2

            def err(self):
                return 1/0

        self.assertTrue(matchers.HASMETHODVALUE('twice', 42) == Klass(21))
        self.assertTrue(matchers.HASMETHODVALUE('twice', 42) != Klass(10))

        with self.assertRaises(ZeroDivisionError):
            _ = (matchers.HASMETHODVALUE('err', None) == Klass(0))
        with self.assertRaises(AttributeError):
            _ = (matchers.HASMETHODVALUE('foo', 1) == Klass(0))
        with self.assertRaises(AttributeError):
            _ = (matchers.HASMETHODVALUE('bar', 1) != Klass(0))

    def testAll(self):
        self.assertIn('NOTNONE', matchers.__all__)
        self.assertIn('REGEXP', matchers.__all__)
        self.assertIn('ONEOF', matchers.__all__)
        self.assertIn('HAS', matchers.__all__)
        self.assertIn('IS', matchers.__all__)
        self.assertIn('INSTANCEOF', matchers.__all__)
        self.assertIn('EQUIV', matchers.__all__)
        self.assertIn('HASKEYVALUE', matchers.__all__)
        self.assertIn('HASATTRVALUE', matchers.__all__)
        self.assertIn('ALLOF', matchers.__all__)
        self.assertIn('ANYOF', matchers.__all__)
        self.assertIn('NOT', matchers.__all__)
        self.assertIn('HASMETHODVALUE', matchers.__all__)

    def testCaptorAny(self):
        seq = [1, 2]
        captor = matchers.ArgCaptor()

        rand = random.Random()
        rand.choice = mock.Mock()
        rand.choice(seq)

        rand.choice.assert_called_with(captor)
        self.assertEqual(captor.arg, seq)

    def testCaptorMultiple(self):
        captor = matchers.ArgCaptor(matcher=matchers.INSTANCEOF(str))

        rand = random.Random()
        rand.choice = mock.Mock()
        rand.choice(1, 7)
        rand.choice('a', 7)
        rand.choice(1, 8)
        rand.choice('b', 8)
        rand.choice(1, 9)
        rand.choice('c', 9)

        rand.choice.assert_any_call(captor, 8)
        self.assertEqual(captor.arg, 'b')

    def testCaptorMatcher(self):
        captor = matchers.ArgCaptor(matcher=matchers.INSTANCEOF(str))

        rand = random.Random()
        rand.choice = mock.Mock()
        rand.choice([1, 2])

        with self.assertRaises(AssertionError):
            rand.choice.assert_called_with(captor)

    def testWithAssertEqual(self):
        expected = [matchers.REGEXP('c$'), matchers.REGEXP('f$')]
        actual = ['abc', 'def']
        self.assertEqual(actual, actual)
        self.assertEqual(expected, actual)

    def testWithAssertCountEqual(self):
        expected = [matchers.REGEXP('c$'), matchers.REGEXP('f$')]
        actual = ['abc', 'def']
        self.assertCountEqual(actual, expected)
        self.assertCountEqual(expected, actual)

    def testWithAssertCountEqualFail(self):
        expected = [matchers.REGEXP('z$'), matchers.REGEXP('f$')]
        actual = ['abc', 'def']
        with self.assertRaises(AssertionError):
            self.assertCountEqual(expected, actual)
        with self.assertRaises(AssertionError):
            self.assertCountEqual(actual, expected)

# Anything with 'search' and 'pattern' attributes considered as
# a compiled RE pattern for REGEXP test.


class _FakeRe1(object):
    """Having search attribute not enough to duck as a regexp object."""

    def search(self):
        pass


class _FakeRe2(object):
    """Having pattern attribute not enough to duck as a regexp object."""
    pattern = None


if __name__ == '__main__':
    unittest.main()
