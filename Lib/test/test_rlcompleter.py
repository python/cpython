import unittest
import builtins
import rlcompleter

class CompleteMe:
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

    def test_complete(self):
        completer = rlcompleter.Completer()
        self.assertEqual(completer.complete('', 0), '\t')
        self.assertEqual(completer.complete('a', 0), 'and')
        self.assertEqual(completer.complete('a', 1), 'as')
        self.assertEqual(completer.complete('as', 2), 'assert')
        self.assertEqual(completer.complete('an', 0), 'and')

if __name__ == '__main__':
    unittest.main()
