import unittest
from unittest.mock import patch
import builtins
import rlcompleter
from test.support import MISSING_C_DOCSTRINGS

class CompleteMe:
    """ Trivial class used in testing rlcompleter.Completer. """
    spam = 1
    _ham = 2


class TestRlcompleter(unittest.TestCase):
    def setUp(self):
        self.stdcompleter = rlcompleter.Completer()
        self.completer = rlcompleter.Completer(dict(spam=int,
                                                    egg=str,
                                                    CompleteMe=CompleteMe))

        # forces stdcompleter to bind builtins namespace
        self.stdcompleter.complete('', 0)

    def test_namespace(self):
        class A(dict):
            pass
        class B(list):
            pass

        self.assertTrue(self.stdcompleter.use_main_ns)
        self.assertFalse(self.completer.use_main_ns)
        self.assertFalse(rlcompleter.Completer(A()).use_main_ns)
        self.assertRaises(TypeError, rlcompleter.Completer, B((1,)))

    def test_global_matches(self):
        # test with builtins namespace
        self.assertEqual(sorted(self.stdcompleter.global_matches('di')),
                         [x+'(' for x in dir(builtins) if x.startswith('di')])
        self.assertEqual(sorted(self.stdcompleter.global_matches('st')),
                         [x+'(' for x in dir(builtins) if x.startswith('st')])
        self.assertEqual(self.stdcompleter.global_matches('akaksajadhak'), [])

        # test with a customized namespace
        self.assertEqual(self.completer.global_matches('CompleteM'),
                ['CompleteMe(' if MISSING_C_DOCSTRINGS else 'CompleteMe()'])
        self.assertEqual(self.completer.global_matches('eg'),
                         ['egg('])
        # XXX: see issue5256
        self.assertEqual(self.completer.global_matches('CompleteM'),
                ['CompleteMe(' if MISSING_C_DOCSTRINGS else 'CompleteMe()'])

    def test_attr_matches(self):
        # test with builtins namespace
        self.assertEqual(self.stdcompleter.attr_matches('str.s'),
                         ['str.{}('.format(x) for x in dir(str)
                          if x.startswith('s')])
        self.assertEqual(self.stdcompleter.attr_matches('tuple.foospamegg'), [])
        expected = sorted({'None.%s%s' % (x,
                                          '()' if x == '__init_subclass__'
                                          else '' if x == '__doc__'
                                          else '(')
                           for x in dir(None)})
        self.assertEqual(self.stdcompleter.attr_matches('None.'), expected)
        self.assertEqual(self.stdcompleter.attr_matches('None._'), expected)
        self.assertEqual(self.stdcompleter.attr_matches('None.__'), expected)

        # test with a customized namespace
        self.assertEqual(self.completer.attr_matches('CompleteMe.sp'),
                         ['CompleteMe.spam'])
        self.assertEqual(self.completer.attr_matches('Completeme.egg'), [])
        self.assertEqual(self.completer.attr_matches('CompleteMe.'),
                         ['CompleteMe.mro()', 'CompleteMe.spam'])
        self.assertEqual(self.completer.attr_matches('CompleteMe._'),
                         ['CompleteMe._ham'])
        matches = self.completer.attr_matches('CompleteMe.__')
        for x in matches:
            self.assertTrue(x.startswith('CompleteMe.__'), x)
        self.assertIn('CompleteMe.__name__', matches)
        self.assertIn('CompleteMe.__new__(', matches)

        with patch.object(CompleteMe, "me", CompleteMe, create=True):
            self.assertEqual(self.completer.attr_matches('CompleteMe.me.me.sp'),
                             ['CompleteMe.me.me.spam'])
            self.assertEqual(self.completer.attr_matches('egg.s'),
                             ['egg.{}('.format(x) for x in dir(str)
                              if x.startswith('s')])

    def test_excessive_getattr(self):
        """Ensure getattr() is invoked no more than once per attribute"""

        # note the special case for @property methods below; that is why
        # we use __dir__ and __getattr__ in class Foo to create a "magic"
        # class attribute 'bar'. This forces `getattr` to call __getattr__
        # (which is doesn't necessarily do).
        class Foo:
            calls = 0
            bar = ''
            def __getattribute__(self, name):
                if name == 'bar':
                    self.calls += 1
                    return None
                return super().__getattribute__(name)

        f = Foo()
        completer = rlcompleter.Completer(dict(f=f))
        self.assertEqual(completer.complete('f.b', 0), 'f.bar')
        self.assertEqual(f.calls, 1)

    def test_property_method_not_called(self):
        class Foo:
            _bar = 0
            property_called = False

            @property
            def bar(self):
                self.property_called = True
                return self._bar

        f = Foo()
        completer = rlcompleter.Completer(dict(f=f))
        self.assertEqual(completer.complete('f.b', 0), 'f.bar')
        self.assertFalse(f.property_called)


    def test_uncreated_attr(self):
        # Attributes like properties and slots should be completed even when
        # they haven't been created on an instance
        class Foo:
            __slots__ = ("bar",)
        completer = rlcompleter.Completer(dict(f=Foo()))
        self.assertEqual(completer.complete('f.', 0), 'f.bar')

    @unittest.mock.patch('rlcompleter._readline_available', False)
    def test_complete(self):
        completer = rlcompleter.Completer()
        self.assertEqual(completer.complete('', 0), '\t')
        self.assertEqual(completer.complete('a', 0), 'and ')
        self.assertEqual(completer.complete('a', 1), 'as ')
        self.assertEqual(completer.complete('as', 2), 'assert ')
        self.assertEqual(completer.complete('an', 0), 'and ')
        self.assertEqual(completer.complete('pa', 0), 'pass')
        self.assertEqual(completer.complete('Fa', 0), 'False')
        self.assertEqual(completer.complete('el', 0), 'elif ')
        self.assertEqual(completer.complete('el', 1), 'else')
        self.assertEqual(completer.complete('tr', 0), 'try:')
        self.assertEqual(completer.complete('_', 0), '_')
        self.assertEqual(completer.complete('match', 0), 'match ')
        self.assertEqual(completer.complete('case', 0), 'case ')

    def test_duplicate_globals(self):
        namespace = {
            'False': None,  # Keyword vs builtin vs namespace
            'assert': None,  # Keyword vs namespace
            'try': lambda: None,  # Keyword vs callable
            'memoryview': None,  # Callable builtin vs non-callable
            'Ellipsis': lambda: None,  # Non-callable builtin vs callable
        }
        completer = rlcompleter.Completer(namespace)
        self.assertEqual(completer.complete('False', 0), 'False')
        self.assertIsNone(completer.complete('False', 1))  # No duplicates
        # Space or colon added due to being a reserved keyword
        self.assertEqual(completer.complete('assert', 0), 'assert ')
        self.assertIsNone(completer.complete('assert', 1))
        self.assertEqual(completer.complete('try', 0), 'try:')
        self.assertIsNone(completer.complete('try', 1))
        # No opening bracket "(" because we overrode the built-in class
        self.assertEqual(completer.complete('memoryview', 0), 'memoryview')
        self.assertIsNone(completer.complete('memoryview', 1))
        self.assertEqual(completer.complete('Ellipsis', 0), 'Ellipsis()')
        self.assertIsNone(completer.complete('Ellipsis', 1))

if __name__ == '__main__':
    unittest.main()
