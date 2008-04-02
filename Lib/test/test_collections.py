import unittest, doctest
from test import test_support
from collections import namedtuple
from collections import Hashable, Iterable, Iterator
from collections import Sized, Container, Callable
from collections import Set, MutableSet
from collections import Mapping, MutableMapping
from collections import Sequence, MutableSequence


class TestNamedTuple(unittest.TestCase):

    def test_factory(self):
        Point = namedtuple('Point', 'x y')
        self.assertEqual(Point.__name__, 'Point')
        self.assertEqual(Point.__doc__, 'Point(x, y)')
        self.assertEqual(Point.__slots__, ())
        self.assertEqual(Point.__module__, __name__)
        self.assertEqual(Point.__getitem__, tuple.__getitem__)
        self.assertEqual(Point._fields, ('x', 'y'))

        self.assertRaises(ValueError, namedtuple, 'abc%', 'efg ghi')       # type has non-alpha char
        self.assertRaises(ValueError, namedtuple, 'class', 'efg ghi')      # type has keyword
        self.assertRaises(ValueError, namedtuple, '9abc', 'efg ghi')       # type starts with digit

        self.assertRaises(ValueError, namedtuple, 'abc', 'efg g%hi')       # field with non-alpha char
        self.assertRaises(ValueError, namedtuple, 'abc', 'abc class')      # field has keyword
        self.assertRaises(ValueError, namedtuple, 'abc', '8efg 9ghi')      # field starts with digit
        self.assertRaises(ValueError, namedtuple, 'abc', '_efg ghi')       # field with leading underscore
        self.assertRaises(ValueError, namedtuple, 'abc', 'efg efg ghi')    # duplicate field

        namedtuple('Point0', 'x1 y2')   # Verify that numbers are allowed in names
        namedtuple('_', 'a b c')        # Test leading underscores in a typename

        self.assertRaises(TypeError, Point._make, [11])                     # catch too few args
        self.assertRaises(TypeError, Point._make, [11, 22, 33])             # catch too many args

    def test_instance(self):
        Point = namedtuple('Point', 'x y')
        p = Point(11, 22)
        self.assertEqual(p, Point(x=11, y=22))
        self.assertEqual(p, Point(11, y=22))
        self.assertEqual(p, Point(y=22, x=11))
        self.assertEqual(p, Point(*(11, 22)))
        self.assertEqual(p, Point(**dict(x=11, y=22)))
        self.assertRaises(TypeError, Point, 1)                              # too few args
        self.assertRaises(TypeError, Point, 1, 2, 3)                        # too many args
        self.assertRaises(TypeError, eval, 'Point(XXX=1, y=2)', locals())   # wrong keyword argument
        self.assertRaises(TypeError, eval, 'Point(x=1)', locals())          # missing keyword argument
        self.assertEqual(repr(p), 'Point(x=11, y=22)')
        self.assert_('__dict__' not in dir(p))                              # verify instance has no dict
        self.assert_('__weakref__' not in dir(p))
        self.assertEqual(p, Point._make([11, 22]))                          # test _make classmethod
        self.assertEqual(p._fields, ('x', 'y'))                             # test _fields attribute
        self.assertEqual(p._replace(x=1), (1, 22))                          # test _replace method
        self.assertEqual(p._asdict(), dict(x=11, y=22))                     # test _asdict method

        try:
            p._replace(x=1, error=2)
        except ValueError:
            pass
        else:
            self._fail('Did not detect an incorrect fieldname')

        # verify that field string can have commas
        Point = namedtuple('Point', 'x, y')
        p = Point(x=11, y=22)
        self.assertEqual(repr(p), 'Point(x=11, y=22)')

        # verify that fieldspec can be a non-string sequence
        Point = namedtuple('Point', ('x', 'y'))
        p = Point(x=11, y=22)
        self.assertEqual(repr(p), 'Point(x=11, y=22)')

    def test_tupleness(self):
        Point = namedtuple('Point', 'x y')
        p = Point(11, 22)

        self.assert_(isinstance(p, tuple))
        self.assertEqual(p, (11, 22))                                       # matches a real tuple
        self.assertEqual(tuple(p), (11, 22))                                # coercable to a real tuple
        self.assertEqual(list(p), [11, 22])                                 # coercable to a list
        self.assertEqual(max(p), 22)                                        # iterable
        self.assertEqual(max(*p), 22)                                       # star-able
        x, y = p
        self.assertEqual(p, (x, y))                                         # unpacks like a tuple
        self.assertEqual((p[0], p[1]), (11, 22))                            # indexable like a tuple
        self.assertRaises(IndexError, p.__getitem__, 3)

        self.assertEqual(p.x, x)
        self.assertEqual(p.y, y)
        self.assertRaises(AttributeError, eval, 'p.z', locals())

    def test_odd_sizes(self):
        Zero = namedtuple('Zero', '')
        self.assertEqual(Zero(), ())
        self.assertEqual(Zero._make([]), ())
        self.assertEqual(repr(Zero()), 'Zero()')
        self.assertEqual(Zero()._asdict(), {})
        self.assertEqual(Zero()._fields, ())

        Dot = namedtuple('Dot', 'd')
        self.assertEqual(Dot(1), (1,))
        self.assertEqual(Dot._make([1]), (1,))
        self.assertEqual(Dot(1).d, 1)
        self.assertEqual(repr(Dot(1)), 'Dot(d=1)')
        self.assertEqual(Dot(1)._asdict(), {'d':1})
        self.assertEqual(Dot(1)._replace(d=999), (999,))
        self.assertEqual(Dot(1)._fields, ('d',))

        n = 10000
        import string, random
        names = [''.join([random.choice(string.letters) for j in range(10)]) for i in range(n)]
        Big = namedtuple('Big', names)
        b = Big(*range(n))
        self.assertEqual(b, tuple(range(n)))
        self.assertEqual(Big._make(range(n)), tuple(range(n)))
        for pos, name in enumerate(names):
            self.assertEqual(getattr(b, name), pos)
        repr(b)                                 # make sure repr() doesn't blow-up
        d = b._asdict()
        d_expected = dict(zip(names, range(n)))
        self.assertEqual(d, d_expected)
        b2 = b._replace(**dict([(names[1], 999),(names[-5], 42)]))
        b2_expected = range(n)
        b2_expected[1] = 999
        b2_expected[-5] = 42
        self.assertEqual(b2, tuple(b2_expected))
        self.assertEqual(b._fields, tuple(names))

class TestOneTrickPonyABCs(unittest.TestCase):

    def test_Hashable(self):
        # Check some non-hashables
        non_samples = [list(), set(), dict()]
        for x in non_samples:
            self.failIf(isinstance(x, Hashable), repr(x))
            self.failIf(issubclass(type(x), Hashable), repr(type(x)))
        # Check some hashables
        samples = [None,
                   int(), float(), complex(),
                   str(),
                   tuple(), frozenset(),
                   int, list, object, type,
                   ]
        for x in samples:
            self.failUnless(isinstance(x, Hashable), repr(x))
            self.failUnless(issubclass(type(x), Hashable), repr(type(x)))
        self.assertRaises(TypeError, Hashable)
        # Check direct subclassing
        class H(Hashable):
            def __hash__(self):
                return super(H, self).__hash__()
        self.assertEqual(hash(H()), 0)
        self.failIf(issubclass(int, H))

    def test_Iterable(self):
        # Check some non-iterables
        non_samples = [None, 42, 3.14, 1j]
        for x in non_samples:
            self.failIf(isinstance(x, Iterable), repr(x))
            self.failIf(issubclass(type(x), Iterable), repr(type(x)))
        # Check some iterables
        samples = [str(),
                   tuple(), list(), set(), frozenset(), dict(),
                   dict().keys(), dict().items(), dict().values(),
                   (lambda: (yield))(),
                   (x for x in []),
                   ]
        for x in samples:
            self.failUnless(isinstance(x, Iterable), repr(x))
            self.failUnless(issubclass(type(x), Iterable), repr(type(x)))
        # Check direct subclassing
        class I(Iterable):
            def __iter__(self):
                return super(I, self).__iter__()
        self.assertEqual(list(I()), [])
        self.failIf(issubclass(str, I))

    def test_Iterator(self):
        non_samples = [None, 42, 3.14, 1j, "".encode('ascii'), "", (), [],
            {}, set()]
        for x in non_samples:
            self.failIf(isinstance(x, Iterator), repr(x))
            self.failIf(issubclass(type(x), Iterator), repr(type(x)))
        samples = [iter(str()),
                   iter(tuple()), iter(list()), iter(dict()),
                   iter(set()), iter(frozenset()),
                   iter(dict().keys()), iter(dict().items()),
                   iter(dict().values()),
                   (lambda: (yield))(),
                   (x for x in []),
                   ]
        for x in samples:
            self.failUnless(isinstance(x, Iterator), repr(x))
            self.failUnless(issubclass(type(x), Iterator), repr(type(x)))

    def test_Sized(self):
        non_samples = [None, 42, 3.14, 1j,
                       (lambda: (yield))(),
                       (x for x in []),
                       ]
        for x in non_samples:
            self.failIf(isinstance(x, Sized), repr(x))
            self.failIf(issubclass(type(x), Sized), repr(type(x)))
        samples = [str(),
                   tuple(), list(), set(), frozenset(), dict(),
                   dict().keys(), dict().items(), dict().values(),
                   ]
        for x in samples:
            self.failUnless(isinstance(x, Sized), repr(x))
            self.failUnless(issubclass(type(x), Sized), repr(type(x)))

    def test_Container(self):
        non_samples = [None, 42, 3.14, 1j,
                       (lambda: (yield))(),
                       (x for x in []),
                       ]
        for x in non_samples:
            self.failIf(isinstance(x, Container), repr(x))
            self.failIf(issubclass(type(x), Container), repr(type(x)))
        samples = [str(),
                   tuple(), list(), set(), frozenset(), dict(),
                   dict().keys(), dict().items(),
                   ]
        for x in samples:
            self.failUnless(isinstance(x, Container), repr(x))
            self.failUnless(issubclass(type(x), Container), repr(type(x)))

    def test_Callable(self):
        non_samples = [None, 42, 3.14, 1j,
                       "", "".encode('ascii'), (), [], {}, set(),
                       (lambda: (yield))(),
                       (x for x in []),
                       ]
        for x in non_samples:
            self.failIf(isinstance(x, Callable), repr(x))
            self.failIf(issubclass(type(x), Callable), repr(type(x)))
        samples = [lambda: None,
                   type, int, object,
                   len,
                   list.append, [].append,
                   ]
        for x in samples:
            self.failUnless(isinstance(x, Callable), repr(x))
            self.failUnless(issubclass(type(x), Callable), repr(type(x)))

    def test_direct_subclassing(self):
        for B in Hashable, Iterable, Iterator, Sized, Container, Callable:
            class C(B):
                pass
            self.failUnless(issubclass(C, B))
            self.failIf(issubclass(int, C))

    def test_registration(self):
        for B in Hashable, Iterable, Iterator, Sized, Container, Callable:
            class C:
                __metaclass__ = type
                __hash__ = None  # Make sure it isn't hashable by default
            self.failIf(issubclass(C, B), B.__name__)
            B.register(C)
            self.failUnless(issubclass(C, B))


class TestCollectionABCs(unittest.TestCase):

    # XXX For now, we only test some virtual inheritance properties.
    # We should also test the proper behavior of the collection ABCs
    # as real base classes or mix-in classes.

    def test_Set(self):
        for sample in [set, frozenset]:
            self.failUnless(isinstance(sample(), Set))
            self.failUnless(issubclass(sample, Set))

    def test_MutableSet(self):
        self.failUnless(isinstance(set(), MutableSet))
        self.failUnless(issubclass(set, MutableSet))
        self.failIf(isinstance(frozenset(), MutableSet))
        self.failIf(issubclass(frozenset, MutableSet))

    def test_Mapping(self):
        for sample in [dict]:
            self.failUnless(isinstance(sample(), Mapping))
            self.failUnless(issubclass(sample, Mapping))

    def test_MutableMapping(self):
        for sample in [dict]:
            self.failUnless(isinstance(sample(), MutableMapping))
            self.failUnless(issubclass(sample, MutableMapping))

    def test_Sequence(self):
        for sample in [tuple, list, str]:
            self.failUnless(isinstance(sample(), Sequence))
            self.failUnless(issubclass(sample, Sequence))
        self.failUnless(issubclass(basestring, Sequence))

    def test_MutableSequence(self):
        for sample in [tuple, str]:
            self.failIf(isinstance(sample(), MutableSequence))
            self.failIf(issubclass(sample, MutableSequence))
        for sample in [list]:
            self.failUnless(isinstance(sample(), MutableSequence))
            self.failUnless(issubclass(sample, MutableSequence))
        self.failIf(issubclass(basestring, MutableSequence))

import doctest, collections

def test_main(verbose=None):
    NamedTupleDocs = doctest.DocTestSuite(module=collections)
    test_classes = [TestNamedTuple, NamedTupleDocs, TestOneTrickPonyABCs, TestCollectionABCs]
    test_support.run_unittest(*test_classes)
    test_support.run_doctest(collections, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
