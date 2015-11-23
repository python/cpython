from test import test_support as support
import unittest
import __builtin__ as builtins
import rlcompleter

class CompleteMe(object):
    """ Trivial class used in testing rlcompleter.Completer. """
    spam = 1


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
                         ['CompleteMe('])
        self.assertEqual(self.completer.global_matches('eg'),
                         ['egg('])
        # XXX: see issue5256
        self.assertEqual(self.completer.global_matches('CompleteM'),
                         ['CompleteMe('])

    def test_attr_matches(self):
        # test with builtins namespace
        self.assertEqual(self.stdcompleter.attr_matches('str.s'),
                         ['str.{}('.format(x) for x in dir(str)
                          if x.startswith('s')])
        self.assertEqual(self.stdcompleter.attr_matches('tuple.foospamegg'), [])

        # test with a customized namespace
        self.assertEqual(self.completer.attr_matches('CompleteMe.sp'),
                         ['CompleteMe.spam'])
        self.assertEqual(self.completer.attr_matches('Completeme.egg'), [])

        CompleteMe.me = CompleteMe
        self.assertEqual(self.completer.attr_matches('CompleteMe.me.me.sp'),
                         ['CompleteMe.me.me.spam'])
        self.assertEqual(self.completer.attr_matches('egg.s'),
                         ['egg.{}('.format(x) for x in dir(str)
                          if x.startswith('s')])

    def test_excessive_getattr(self):
        # Ensure getattr() is invoked no more than once per attribute
        class Foo:
            calls = 0
            @property
            def bar(self):
                self.calls += 1
                return None
        f = Foo()
        completer = rlcompleter.Completer(dict(f=f))
        self.assertEqual(completer.complete('f.b', 0), 'f.bar')
        self.assertEqual(f.calls, 1)

def test_main():
    support.run_unittest(TestRlcompleter)

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
        self.assertEqual(completer.complete('assert', 0), 'assert')
        self.assertIsNone(completer.complete('assert', 1))
        self.assertEqual(completer.complete('try', 0), 'try')
        self.assertIsNone(completer.complete('try', 1))
        # No opening bracket "(" because we overrode the built-in class
        self.assertEqual(completer.complete('memoryview', 0), 'memoryview')
        self.assertIsNone(completer.complete('memoryview', 1))
        self.assertEqual(completer.complete('Ellipsis', 0), 'Ellipsis(')
        self.assertIsNone(completer.complete('Ellipsis', 1))


if __name__ == '__main__':
    test_main()
