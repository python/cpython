
import unittest, doctest, operator
import inspect
from test import test_support
from collections import namedtuple, Counter, OrderedDict
from test import mapping_tests
import pickle, cPickle, copy
from random import randrange, shuffle
import keyword
import re
import sys
from collections import Hashable, Iterable, Iterator
from collections import Sized, Container, Callable
from collections import Set, MutableSet
from collections import Mapping, MutableMapping
from collections import Sequence, MutableSequence
# Silence deprecation warning
sets = test_support.import_module('sets', deprecated=True)

TestNT = namedtuple('TestNT', 'x y z')    # type used for pickle tests

py273_named_tuple_pickle = '''\
ccopy_reg
_reconstructor
p0
(ctest.test_collections
TestNT
p1
c__builtin__
tuple
p2
(I10
I20
I30
tp3
tp4
Rp5
ccollections
OrderedDict
p6
((lp7
(lp8
S'x'
p9
aI10
aa(lp10
S'y'
p11
aI20
aa(lp12
S'z'
p13
aI30
aatp14
Rp15
b.
'''

class TestNamedTuple(unittest.TestCase):

    def test_factory(self):
        Point = namedtuple('Point', 'x y')
        self.assertEqual(Point.__name__, 'Point')
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

        nt = namedtuple('nt', u'the quick brown fox')                       # check unicode input
        self.assertNotIn("u'", repr(nt._fields))
        nt = namedtuple('nt', (u'the', u'quick'))                           # check unicode input
        self.assertNotIn("u'", repr(nt._fields))

        self.assertRaises(TypeError, Point._make, [11])                     # catch too few args
        self.assertRaises(TypeError, Point._make, [11, 22, 33])             # catch too many args

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_factory_doc_attr(self):
        Point = namedtuple('Point', 'x y')
        self.assertEqual(Point.__doc__, 'Point(x, y)')

    def test_name_fixer(self):
        for spec, renamed in [
            [('efg', 'g%hi'),  ('efg', '_1')],                              # field with non-alpha char
            [('abc', 'class'), ('abc', '_1')],                              # field has keyword
            [('8efg', '9ghi'), ('_0', '_1')],                               # field starts with digit
            [('abc', '_efg'), ('abc', '_1')],                               # field with leading underscore
            [('abc', 'efg', 'efg', 'ghi'), ('abc', 'efg', '_2', 'ghi')],    # duplicate field
            [('abc', '', 'x'), ('abc', '_1', 'x')],                         # fieldname is a space
        ]:
            self.assertEqual(namedtuple('NT', spec, rename=True)._fields, renamed)

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
        self.assertNotIn('__weakref__', dir(p))
        self.assertEqual(p, Point._make([11, 22]))                          # test _make classmethod
        self.assertEqual(p._fields, ('x', 'y'))                             # test _fields attribute
        self.assertEqual(p._replace(x=1), (1, 22))                          # test _replace method
        self.assertEqual(p._asdict(), dict(x=11, y=22))                     # test _asdict method
        self.assertEqual(vars(p), p._asdict())                              # verify that vars() works

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

        self.assertIsInstance(p, tuple)
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

        n = 5000
        import string, random
        names = list(set(''.join([random.choice(string.ascii_letters)
                                  for j in range(10)]) for i in range(n)))
        n = len(names)
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

    def test_pickle(self):
        p = TestNT(x=10, y=20, z=30)
        for module in pickle, cPickle:
            loads = getattr(module, 'loads')
            dumps = getattr(module, 'dumps')
            for protocol in -1, 0, 1, 2:
                q = loads(dumps(p, protocol))
                self.assertEqual(p, q)
                self.assertEqual(p._fields, q._fields)

    def test_copy(self):
        p = TestNT(x=10, y=20, z=30)
        for copier in copy.copy, copy.deepcopy:
            q = copier(p)
            self.assertEqual(p, q)
            self.assertEqual(p._fields, q._fields)

    def test_name_conflicts(self):
        # Some names like "self", "cls", "tuple", "itemgetter", and "property"
        # failed when used as field names.  Test to make sure these now work.
        T = namedtuple('T', 'itemgetter property self cls tuple')
        t = T(1, 2, 3, 4, 5)
        self.assertEqual(t, (1,2,3,4,5))
        newt = t._replace(itemgetter=10, property=20, self=30, cls=40, tuple=50)
        self.assertEqual(newt, (10,20,30,40,50))

        # Broader test of all interesting names in a template
        with test_support.captured_stdout() as template:
            T = namedtuple('T', 'x', verbose=True)
        words = set(re.findall('[A-Za-z]+', template.getvalue()))
        words -= set(keyword.kwlist)
        T = namedtuple('T', words)
        # test __new__
        values = tuple(range(len(words)))
        t = T(*values)
        self.assertEqual(t, values)
        t = T(**dict(zip(T._fields, values)))
        self.assertEqual(t, values)
        # test _make
        t = T._make(values)
        self.assertEqual(t, values)
        # exercise __repr__
        repr(t)
        # test _asdict
        self.assertEqual(t._asdict(), dict(zip(T._fields, values)))
        # test _replace
        t = T._make(values)
        newvalues = tuple(v*10 for v in values)
        newt = t._replace(**dict(zip(T._fields, newvalues)))
        self.assertEqual(newt, newvalues)
        # test _fields
        self.assertEqual(T._fields, tuple(words))
        # test __getnewargs__
        self.assertEqual(t.__getnewargs__(), values)

    def test_pickling_bug_18015(self):
        # http://bugs.python.org/issue18015
        pt = pickle.loads(py273_named_tuple_pickle)
        self.assertEqual(pt.x, 10)

class ABCTestCase(unittest.TestCase):

    def validate_abstract_methods(self, abc, *names):
        methodstubs = dict.fromkeys(names, lambda s, *args: 0)

        # everything should work will all required methods are present
        C = type('C', (abc,), methodstubs)
        C()

        # instantiation should fail if a required method is missing
        for name in names:
            stubs = methodstubs.copy()
            del stubs[name]
            C = type('C', (abc,), stubs)
            self.assertRaises(TypeError, C, name)

    def validate_isinstance(self, abc, name):
        stub = lambda s, *args: 0

        # new-style class
        C = type('C', (object,), {name: stub})
        self.assertIsInstance(C(), abc)
        self.assertTrue(issubclass(C, abc))
        # old-style class
        class C: pass
        setattr(C, name, stub)
        self.assertIsInstance(C(), abc)
        self.assertTrue(issubclass(C, abc))

        # new-style class
        C = type('C', (object,), {'__hash__': None})
        self.assertNotIsInstance(C(), abc)
        self.assertFalse(issubclass(C, abc))
        # old-style class
        class C: pass
        self.assertNotIsInstance(C(), abc)
        self.assertFalse(issubclass(C, abc))

    def validate_comparison(self, instance):
        ops = ['lt', 'gt', 'le', 'ge', 'ne', 'or', 'and', 'xor', 'sub']
        operators = {}
        for op in ops:
            name = '__' + op + '__'
            operators[name] = getattr(operator, name)

        class Other:
            def __init__(self):
                self.right_side = False
            def __eq__(self, other):
                self.right_side = True
                return True
            __lt__ = __eq__
            __gt__ = __eq__
            __le__ = __eq__
            __ge__ = __eq__
            __ne__ = __eq__
            __ror__ = __eq__
            __rand__ = __eq__
            __rxor__ = __eq__
            __rsub__ = __eq__

        for name, op in operators.items():
            if not hasattr(instance, name):
                continue
            other = Other()
            op(instance, other)
            self.assertTrue(other.right_side,'Right side not called for %s.%s'
                            % (type(instance), name))

class TestOneTrickPonyABCs(ABCTestCase):

    def test_Hashable(self):
        # Check some non-hashables
        non_samples = [list(), set(), dict()]
        for x in non_samples:
            self.assertNotIsInstance(x, Hashable)
            self.assertFalse(issubclass(type(x), Hashable), repr(type(x)))
        # Check some hashables
        samples = [None,
                   int(), float(), complex(),
                   str(),
                   tuple(), frozenset(),
                   int, list, object, type,
                   ]
        for x in samples:
            self.assertIsInstance(x, Hashable)
            self.assertTrue(issubclass(type(x), Hashable), repr(type(x)))
        self.assertRaises(TypeError, Hashable)
        # Check direct subclassing
        class H(Hashable):
            def __hash__(self):
                return super(H, self).__hash__()
            __eq__ = Hashable.__eq__ # Silence Py3k warning
        self.assertEqual(hash(H()), 0)
        self.assertFalse(issubclass(int, H))
        self.validate_abstract_methods(Hashable, '__hash__')
        self.validate_isinstance(Hashable, '__hash__')

    def test_Iterable(self):
        # Check some non-iterables
        non_samples = [None, 42, 3.14, 1j]
        for x in non_samples:
            self.assertNotIsInstance(x, Iterable)
            self.assertFalse(issubclass(type(x), Iterable), repr(type(x)))
        # Check some iterables
        samples = [str(),
                   tuple(), list(), set(), frozenset(), dict(),
                   dict().keys(), dict().items(), dict().values(),
                   (lambda: (yield))(),
                   (x for x in []),
                   ]
        for x in samples:
            self.assertIsInstance(x, Iterable)
            self.assertTrue(issubclass(type(x), Iterable), repr(type(x)))
        # Check direct subclassing
        class I(Iterable):
            def __iter__(self):
                return super(I, self).__iter__()
        self.assertEqual(list(I()), [])
        self.assertFalse(issubclass(str, I))
        self.validate_abstract_methods(Iterable, '__iter__')
        self.validate_isinstance(Iterable, '__iter__')

    def test_Iterator(self):
        non_samples = [None, 42, 3.14, 1j, "".encode('ascii'), "", (), [],
            {}, set()]
        for x in non_samples:
            self.assertNotIsInstance(x, Iterator)
            self.assertFalse(issubclass(type(x), Iterator), repr(type(x)))
        samples = [iter(str()),
                   iter(tuple()), iter(list()), iter(dict()),
                   iter(set()), iter(frozenset()),
                   iter(dict().keys()), iter(dict().items()),
                   iter(dict().values()),
                   (lambda: (yield))(),
                   (x for x in []),
                   ]
        for x in samples:
            self.assertIsInstance(x, Iterator)
            self.assertTrue(issubclass(type(x), Iterator), repr(type(x)))
        self.validate_abstract_methods(Iterator, 'next', '__iter__')

        # Issue 10565
        class NextOnly:
            def __next__(self):
                yield 1
                raise StopIteration
        self.assertNotIsInstance(NextOnly(), Iterator)
        class NextOnlyNew(object):
            def __next__(self):
                yield 1
                raise StopIteration
        self.assertNotIsInstance(NextOnlyNew(), Iterator)

    def test_Sized(self):
        non_samples = [None, 42, 3.14, 1j,
                       (lambda: (yield))(),
                       (x for x in []),
                       ]
        for x in non_samples:
            self.assertNotIsInstance(x, Sized)
            self.assertFalse(issubclass(type(x), Sized), repr(type(x)))
        samples = [str(),
                   tuple(), list(), set(), frozenset(), dict(),
                   dict().keys(), dict().items(), dict().values(),
                   ]
        for x in samples:
            self.assertIsInstance(x, Sized)
            self.assertTrue(issubclass(type(x), Sized), repr(type(x)))
        self.validate_abstract_methods(Sized, '__len__')
        self.validate_isinstance(Sized, '__len__')

    def test_Container(self):
        non_samples = [None, 42, 3.14, 1j,
                       (lambda: (yield))(),
                       (x for x in []),
                       ]
        for x in non_samples:
            self.assertNotIsInstance(x, Container)
            self.assertFalse(issubclass(type(x), Container), repr(type(x)))
        samples = [str(),
                   tuple(), list(), set(), frozenset(), dict(),
                   dict().keys(), dict().items(),
                   ]
        for x in samples:
            self.assertIsInstance(x, Container)
            self.assertTrue(issubclass(type(x), Container), repr(type(x)))
        self.validate_abstract_methods(Container, '__contains__')
        self.validate_isinstance(Container, '__contains__')

    def test_Callable(self):
        non_samples = [None, 42, 3.14, 1j,
                       "", "".encode('ascii'), (), [], {}, set(),
                       (lambda: (yield))(),
                       (x for x in []),
                       ]
        for x in non_samples:
            self.assertNotIsInstance(x, Callable)
            self.assertFalse(issubclass(type(x), Callable), repr(type(x)))
        samples = [lambda: None,
                   type, int, object,
                   len,
                   list.append, [].append,
                   ]
        for x in samples:
            self.assertIsInstance(x, Callable)
            self.assertTrue(issubclass(type(x), Callable), repr(type(x)))
        self.validate_abstract_methods(Callable, '__call__')
        self.validate_isinstance(Callable, '__call__')

    def test_direct_subclassing(self):
        for B in Hashable, Iterable, Iterator, Sized, Container, Callable:
            class C(B):
                pass
            self.assertTrue(issubclass(C, B))
            self.assertFalse(issubclass(int, C))

    def test_registration(self):
        for B in Hashable, Iterable, Iterator, Sized, Container, Callable:
            class C:
                __metaclass__ = type
                __hash__ = None  # Make sure it isn't hashable by default
            self.assertFalse(issubclass(C, B), B.__name__)
            B.register(C)
            self.assertTrue(issubclass(C, B))

class WithSet(MutableSet):

    def __init__(self, it=()):
        self.data = set(it)

    def __len__(self):
        return len(self.data)

    def __iter__(self):
        return iter(self.data)

    def __contains__(self, item):
        return item in self.data

    def add(self, item):
        self.data.add(item)

    def discard(self, item):
        self.data.discard(item)

class TestCollectionABCs(ABCTestCase):

    # XXX For now, we only test some virtual inheritance properties.
    # We should also test the proper behavior of the collection ABCs
    # as real base classes or mix-in classes.

    def test_Set(self):
        for sample in [set, frozenset]:
            self.assertIsInstance(sample(), Set)
            self.assertTrue(issubclass(sample, Set))
        self.validate_abstract_methods(Set, '__contains__', '__iter__', '__len__')
        class MySet(Set):
            def __contains__(self, x):
                return False
            def __len__(self):
                return 0
            def __iter__(self):
                return iter([])
        self.validate_comparison(MySet())

    def test_hash_Set(self):
        class OneTwoThreeSet(Set):
            def __init__(self):
                self.contents = [1, 2, 3]
            def __contains__(self, x):
                return x in self.contents
            def __len__(self):
                return len(self.contents)
            def __iter__(self):
                return iter(self.contents)
            def __hash__(self):
                return self._hash()
        a, b = OneTwoThreeSet(), OneTwoThreeSet()
        self.assertTrue(hash(a) == hash(b))

    def test_MutableSet(self):
        self.assertIsInstance(set(), MutableSet)
        self.assertTrue(issubclass(set, MutableSet))
        self.assertNotIsInstance(frozenset(), MutableSet)
        self.assertFalse(issubclass(frozenset, MutableSet))
        self.validate_abstract_methods(MutableSet, '__contains__', '__iter__', '__len__',
            'add', 'discard')

    def test_issue_5647(self):
        # MutableSet.__iand__ mutated the set during iteration
        s = WithSet('abcd')
        s &= WithSet('cdef')            # This used to fail
        self.assertEqual(set(s), set('cd'))

    def test_issue_4920(self):
        # MutableSet.pop() method did not work
        class MySet(collections.MutableSet):
            __slots__=['__s']
            def __init__(self,items=None):
                if items is None:
                    items=[]
                self.__s=set(items)
            def __contains__(self,v):
                return v in self.__s
            def __iter__(self):
                return iter(self.__s)
            def __len__(self):
                return len(self.__s)
            def add(self,v):
                result=v not in self.__s
                self.__s.add(v)
                return result
            def discard(self,v):
                result=v in self.__s
                self.__s.discard(v)
                return result
            def __repr__(self):
                return "MySet(%s)" % repr(list(self))
        s = MySet([5,43,2,1])
        self.assertEqual(s.pop(), 1)

    def test_issue8750(self):
        empty = WithSet()
        full = WithSet(range(10))
        s = WithSet(full)
        s -= s
        self.assertEqual(s, empty)
        s = WithSet(full)
        s ^= s
        self.assertEqual(s, empty)
        s = WithSet(full)
        s &= s
        self.assertEqual(s, full)
        s |= s
        self.assertEqual(s, full)

    def test_issue16373(self):
        # Recursion error comparing comparable and noncomparable
        # Set instances
        class MyComparableSet(Set):
            def __contains__(self, x):
                return False
            def __len__(self):
                return 0
            def __iter__(self):
                return iter([])
        class MyNonComparableSet(Set):
            def __contains__(self, x):
                return False
            def __len__(self):
                return 0
            def __iter__(self):
                return iter([])
            def __le__(self, x):
                return NotImplemented
            def __lt__(self, x):
                return NotImplemented

        cs = MyComparableSet()
        ncs = MyNonComparableSet()

        # Run all the variants to make sure they don't mutually recurse
        ncs < cs
        ncs <= cs
        ncs > cs
        ncs >= cs
        cs < ncs
        cs <= ncs
        cs > ncs
        cs >= ncs

    def assertSameSet(self, s1, s2):
        # coerce both to a real set then check equality
        self.assertEqual(set(s1), set(s2))

    def test_Set_interoperability_with_real_sets(self):
        # Issue: 8743
        class ListSet(Set):
            def __init__(self, elements=()):
                self.data = []
                for elem in elements:
                    if elem not in self.data:
                        self.data.append(elem)
            def __contains__(self, elem):
                return elem in self.data
            def __iter__(self):
                return iter(self.data)
            def __len__(self):
                return len(self.data)
            def __repr__(self):
                return 'Set({!r})'.format(self.data)

        r1 = set('abc')
        r2 = set('bcd')
        r3 = set('abcde')
        f1 = ListSet('abc')
        f2 = ListSet('bcd')
        f3 = ListSet('abcde')
        l1 = list('abccba')
        l2 = list('bcddcb')
        l3 = list('abcdeedcba')
        p1 = sets.Set('abc')
        p2 = sets.Set('bcd')
        p3 = sets.Set('abcde')

        target = r1 & r2
        self.assertSameSet(f1 & f2, target)
        self.assertSameSet(f1 & r2, target)
        self.assertSameSet(r2 & f1, target)
        self.assertSameSet(f1 & p2, target)
        self.assertSameSet(p2 & f1, target)
        self.assertSameSet(f1 & l2, target)

        target = r1 | r2
        self.assertSameSet(f1 | f2, target)
        self.assertSameSet(f1 | r2, target)
        self.assertSameSet(r2 | f1, target)
        self.assertSameSet(f1 | p2, target)
        self.assertSameSet(p2 | f1, target)
        self.assertSameSet(f1 | l2, target)

        fwd_target = r1 - r2
        rev_target = r2 - r1
        self.assertSameSet(f1 - f2, fwd_target)
        self.assertSameSet(f2 - f1, rev_target)
        self.assertSameSet(f1 - r2, fwd_target)
        self.assertSameSet(f2 - r1, rev_target)
        self.assertSameSet(r1 - f2, fwd_target)
        self.assertSameSet(r2 - f1, rev_target)
        self.assertSameSet(f1 - p2, fwd_target)
        self.assertSameSet(f2 - p1, rev_target)
        self.assertSameSet(p1 - f2, fwd_target)
        self.assertSameSet(p2 - f1, rev_target)
        self.assertSameSet(f1 - l2, fwd_target)
        self.assertSameSet(f2 - l1, rev_target)

        target = r1 ^ r2
        self.assertSameSet(f1 ^ f2, target)
        self.assertSameSet(f1 ^ r2, target)
        self.assertSameSet(r2 ^ f1, target)
        self.assertSameSet(f1 ^ p2, target)
        self.assertSameSet(p2 ^ f1, target)
        self.assertSameSet(f1 ^ l2, target)

        # proper subset
        self.assertTrue(f1 < f3)
        self.assertFalse(f1 < f1)
        self.assertFalse(f1 < f2)
        self.assertTrue(r1 < f3)
        self.assertFalse(r1 < f1)
        self.assertFalse(r1 < f2)
        self.assertTrue(r1 < r3)
        self.assertFalse(r1 < r1)
        self.assertFalse(r1 < r2)

        with test_support.check_py3k_warnings():
            # python 2 only, cross-type compares will succeed
            f1 < l3
            f1 < l1
            f1 < l2

        # any subset
        self.assertTrue(f1 <= f3)
        self.assertTrue(f1 <= f1)
        self.assertFalse(f1 <= f2)
        self.assertTrue(r1 <= f3)
        self.assertTrue(r1 <= f1)
        self.assertFalse(r1 <= f2)
        self.assertTrue(r1 <= r3)
        self.assertTrue(r1 <= r1)
        self.assertFalse(r1 <= r2)

        with test_support.check_py3k_warnings():
            # python 2 only, cross-type compares will succeed
            f1 <= l3
            f1 <= l1
            f1 <= l2

        # proper superset
        self.assertTrue(f3 > f1)
        self.assertFalse(f1 > f1)
        self.assertFalse(f2 > f1)
        self.assertTrue(r3 > r1)
        self.assertFalse(f1 > r1)
        self.assertFalse(f2 > r1)
        self.assertTrue(r3 > r1)
        self.assertFalse(r1 > r1)
        self.assertFalse(r2 > r1)

        with test_support.check_py3k_warnings():
            # python 2 only, cross-type compares will succeed
            f1 > l3
            f1 > l1
            f1 > l2

        # any superset
        self.assertTrue(f3 >= f1)
        self.assertTrue(f1 >= f1)
        self.assertFalse(f2 >= f1)
        self.assertTrue(r3 >= r1)
        self.assertTrue(f1 >= r1)
        self.assertFalse(f2 >= r1)
        self.assertTrue(r3 >= r1)
        self.assertTrue(r1 >= r1)
        self.assertFalse(r2 >= r1)

        with test_support.check_py3k_warnings():
            # python 2 only, cross-type compares will succeed
            f1 >= l3
            f1 >=l1
            f1 >= l2

        # equality
        self.assertTrue(f1 == f1)
        self.assertTrue(r1 == f1)
        self.assertTrue(f1 == r1)
        self.assertFalse(f1 == f3)
        self.assertFalse(r1 == f3)
        self.assertFalse(f1 == r3)
        # python 2 only, cross-type compares will succeed
        f1 == l3
        f1 == l1
        f1 == l2

        # inequality
        self.assertFalse(f1 != f1)
        self.assertFalse(r1 != f1)
        self.assertFalse(f1 != r1)
        self.assertTrue(f1 != f3)
        self.assertTrue(r1 != f3)
        self.assertTrue(f1 != r3)
        # python 2 only, cross-type compares will succeed
        f1 != l3
        f1 != l1
        f1 != l2

    def test_Mapping(self):
        for sample in [dict]:
            self.assertIsInstance(sample(), Mapping)
            self.assertTrue(issubclass(sample, Mapping))
        self.validate_abstract_methods(Mapping, '__contains__', '__iter__', '__len__',
            '__getitem__')
        class MyMapping(collections.Mapping):
            def __len__(self):
                return 0
            def __getitem__(self, i):
                raise IndexError
            def __iter__(self):
                return iter(())
        self.validate_comparison(MyMapping())

    def test_MutableMapping(self):
        for sample in [dict]:
            self.assertIsInstance(sample(), MutableMapping)
            self.assertTrue(issubclass(sample, MutableMapping))
        self.validate_abstract_methods(MutableMapping, '__contains__', '__iter__', '__len__',
            '__getitem__', '__setitem__', '__delitem__')

    def test_Sequence(self):
        for sample in [tuple, list, str]:
            self.assertIsInstance(sample(), Sequence)
            self.assertTrue(issubclass(sample, Sequence))
        self.assertTrue(issubclass(basestring, Sequence))
        self.assertIsInstance(range(10), Sequence)
        self.assertTrue(issubclass(xrange, Sequence))
        self.assertTrue(issubclass(str, Sequence))
        self.validate_abstract_methods(Sequence, '__contains__', '__iter__', '__len__',
            '__getitem__')

    def test_MutableSequence(self):
        for sample in [tuple, str]:
            self.assertNotIsInstance(sample(), MutableSequence)
            self.assertFalse(issubclass(sample, MutableSequence))
        for sample in [list]:
            self.assertIsInstance(sample(), MutableSequence)
            self.assertTrue(issubclass(sample, MutableSequence))
        self.assertFalse(issubclass(basestring, MutableSequence))
        self.validate_abstract_methods(MutableSequence, '__contains__', '__iter__',
            '__len__', '__getitem__', '__setitem__', '__delitem__', 'insert')

class TestCounter(unittest.TestCase):

    def test_basics(self):
        c = Counter('abcaba')
        self.assertEqual(c, Counter({'a':3 , 'b': 2, 'c': 1}))
        self.assertEqual(c, Counter(a=3, b=2, c=1))
        self.assertIsInstance(c, dict)
        self.assertIsInstance(c, Mapping)
        self.assertTrue(issubclass(Counter, dict))
        self.assertTrue(issubclass(Counter, Mapping))
        self.assertEqual(len(c), 3)
        self.assertEqual(sum(c.values()), 6)
        self.assertEqual(sorted(c.values()), [1, 2, 3])
        self.assertEqual(sorted(c.keys()), ['a', 'b', 'c'])
        self.assertEqual(sorted(c), ['a', 'b', 'c'])
        self.assertEqual(sorted(c.items()),
                         [('a', 3), ('b', 2), ('c', 1)])
        self.assertEqual(c['b'], 2)
        self.assertEqual(c['z'], 0)
        with test_support.check_py3k_warnings():
            self.assertEqual(c.has_key('c'), True)
            self.assertEqual(c.has_key('z'), False)
        self.assertEqual(c.__contains__('c'), True)
        self.assertEqual(c.__contains__('z'), False)
        self.assertEqual(c.get('b', 10), 2)
        self.assertEqual(c.get('z', 10), 10)
        self.assertEqual(c, dict(a=3, b=2, c=1))
        self.assertEqual(repr(c), "Counter({'a': 3, 'b': 2, 'c': 1})")
        self.assertEqual(c.most_common(), [('a', 3), ('b', 2), ('c', 1)])
        for i in range(5):
            self.assertEqual(c.most_common(i),
                             [('a', 3), ('b', 2), ('c', 1)][:i])
        self.assertEqual(''.join(sorted(c.elements())), 'aaabbc')
        c['a'] += 1         # increment an existing value
        c['b'] -= 2         # sub existing value to zero
        del c['c']          # remove an entry
        del c['c']          # make sure that del doesn't raise KeyError
        c['d'] -= 2         # sub from a missing value
        c['e'] = -5         # directly assign a missing value
        c['f'] += 4         # add to a missing value
        self.assertEqual(c, dict(a=4, b=0, d=-2, e=-5, f=4))
        self.assertEqual(''.join(sorted(c.elements())), 'aaaaffff')
        self.assertEqual(c.pop('f'), 4)
        self.assertNotIn('f', c)
        for i in range(3):
            elem, cnt = c.popitem()
            self.assertNotIn(elem, c)
        c.clear()
        self.assertEqual(c, {})
        self.assertEqual(repr(c), 'Counter()')
        self.assertRaises(NotImplementedError, Counter.fromkeys, 'abc')
        self.assertRaises(TypeError, hash, c)
        c.update(dict(a=5, b=3))
        c.update(c=1)
        c.update(Counter('a' * 50 + 'b' * 30))
        c.update()          # test case with no args
        c.__init__('a' * 500 + 'b' * 300)
        c.__init__('cdc')
        c.__init__()
        self.assertEqual(c, dict(a=555, b=333, c=3, d=1))
        self.assertEqual(c.setdefault('d', 5), 1)
        self.assertEqual(c['d'], 1)
        self.assertEqual(c.setdefault('e', 5), 5)
        self.assertEqual(c['e'], 5)

    def test_copying(self):
        # Check that counters are copyable, deepcopyable, picklable, and
        #have a repr/eval round-trip
        words = Counter('which witch had which witches wrist watch'.split())
        update_test = Counter()
        update_test.update(words)
        for i, dup in enumerate([
                    words.copy(),
                    copy.copy(words),
                    copy.deepcopy(words),
                    pickle.loads(pickle.dumps(words, 0)),
                    pickle.loads(pickle.dumps(words, 1)),
                    pickle.loads(pickle.dumps(words, 2)),
                    pickle.loads(pickle.dumps(words, -1)),
                    cPickle.loads(cPickle.dumps(words, 0)),
                    cPickle.loads(cPickle.dumps(words, 1)),
                    cPickle.loads(cPickle.dumps(words, 2)),
                    cPickle.loads(cPickle.dumps(words, -1)),
                    eval(repr(words)),
                    update_test,
                    Counter(words),
                    ]):
            msg = (i, dup, words)
            self.assertTrue(dup is not words)
            self.assertEqual(dup, words)
            self.assertEqual(len(dup), len(words))
            self.assertEqual(type(dup), type(words))

    def test_copy_subclass(self):
        class MyCounter(Counter):
            pass
        c = MyCounter('slartibartfast')
        d = c.copy()
        self.assertEqual(d, c)
        self.assertEqual(len(d), len(c))
        self.assertEqual(type(d), type(c))

    def test_conversions(self):
        # Convert to: set, list, dict
        s = 'she sells sea shells by the sea shore'
        self.assertEqual(sorted(Counter(s).elements()), sorted(s))
        self.assertEqual(sorted(Counter(s)), sorted(set(s)))
        self.assertEqual(dict(Counter(s)), dict(Counter(s).items()))
        self.assertEqual(set(Counter(s)), set(s))

    def test_invariant_for_the_in_operator(self):
        c = Counter(a=10, b=-2, c=0)
        for elem in c:
            self.assertTrue(elem in c)
            self.assertIn(elem, c)

    def test_multiset_operations(self):
        # Verify that adding a zero counter will strip zeros and negatives
        c = Counter(a=10, b=-2, c=0) + Counter()
        self.assertEqual(dict(c), dict(a=10))

        elements = 'abcd'
        for i in range(1000):
            # test random pairs of multisets
            p = Counter(dict((elem, randrange(-2,4)) for elem in elements))
            p.update(e=1, f=-1, g=0)
            q = Counter(dict((elem, randrange(-2,4)) for elem in elements))
            q.update(h=1, i=-1, j=0)
            for counterop, numberop in [
                (Counter.__add__, lambda x, y: max(0, x+y)),
                (Counter.__sub__, lambda x, y: max(0, x-y)),
                (Counter.__or__, lambda x, y: max(0,x,y)),
                (Counter.__and__, lambda x, y: max(0, min(x,y))),
            ]:
                result = counterop(p, q)
                for x in elements:
                    self.assertEqual(numberop(p[x], q[x]), result[x],
                                     (counterop, x, p, q))
                # verify that results exclude non-positive counts
                self.assertTrue(x>0 for x in result.values())

        elements = 'abcdef'
        for i in range(100):
            # verify that random multisets with no repeats are exactly like sets
            p = Counter(dict((elem, randrange(0, 2)) for elem in elements))
            q = Counter(dict((elem, randrange(0, 2)) for elem in elements))
            for counterop, setop in [
                (Counter.__sub__, set.__sub__),
                (Counter.__or__, set.__or__),
                (Counter.__and__, set.__and__),
            ]:
                counter_result = counterop(p, q)
                set_result = setop(set(p.elements()), set(q.elements()))
                self.assertEqual(counter_result, dict.fromkeys(set_result, 1))

    def test_subtract(self):
        c = Counter(a=-5, b=0, c=5, d=10, e=15,g=40)
        c.subtract(a=1, b=2, c=-3, d=10, e=20, f=30, h=-50)
        self.assertEqual(c, Counter(a=-6, b=-2, c=8, d=0, e=-5, f=-30, g=40, h=50))
        c = Counter(a=-5, b=0, c=5, d=10, e=15,g=40)
        c.subtract(Counter(a=1, b=2, c=-3, d=10, e=20, f=30, h=-50))
        self.assertEqual(c, Counter(a=-6, b=-2, c=8, d=0, e=-5, f=-30, g=40, h=50))
        c = Counter('aaabbcd')
        c.subtract('aaaabbcce')
        self.assertEqual(c, Counter(a=-1, b=0, c=-1, d=1, e=-1))

class TestOrderedDict(unittest.TestCase):

    def test_init(self):
        with self.assertRaises(TypeError):
            OrderedDict([('a', 1), ('b', 2)], None)                                 # too many args
        pairs = [('a', 1), ('b', 2), ('c', 3), ('d', 4), ('e', 5)]
        self.assertEqual(sorted(OrderedDict(dict(pairs)).items()), pairs)           # dict input
        self.assertEqual(sorted(OrderedDict(**dict(pairs)).items()), pairs)         # kwds input
        self.assertEqual(list(OrderedDict(pairs).items()), pairs)                   # pairs input
        self.assertEqual(list(OrderedDict([('a', 1), ('b', 2), ('c', 9), ('d', 4)],
                                          c=3, e=5).items()), pairs)                # mixed input

        # make sure no positional args conflict with possible kwdargs
        self.assertEqual(inspect.getargspec(OrderedDict.__dict__['__init__']).args,
                         ['self'])

        # Make sure that direct calls to __init__ do not clear previous contents
        d = OrderedDict([('a', 1), ('b', 2), ('c', 3), ('d', 44), ('e', 55)])
        d.__init__([('e', 5), ('f', 6)], g=7, d=4)
        self.assertEqual(list(d.items()),
            [('a', 1), ('b', 2), ('c', 3), ('d', 4), ('e', 5), ('f', 6), ('g', 7)])

    def test_update(self):
        with self.assertRaises(TypeError):
            OrderedDict().update([('a', 1), ('b', 2)], None)                        # too many args
        pairs = [('a', 1), ('b', 2), ('c', 3), ('d', 4), ('e', 5)]
        od = OrderedDict()
        od.update(dict(pairs))
        self.assertEqual(sorted(od.items()), pairs)                                 # dict input
        od = OrderedDict()
        od.update(**dict(pairs))
        self.assertEqual(sorted(od.items()), pairs)                                 # kwds input
        od = OrderedDict()
        od.update(pairs)
        self.assertEqual(list(od.items()), pairs)                                   # pairs input
        od = OrderedDict()
        od.update([('a', 1), ('b', 2), ('c', 9), ('d', 4)], c=3, e=5)
        self.assertEqual(list(od.items()), pairs)                                   # mixed input

        # Issue 9137: Named argument called 'other' or 'self'
        # shouldn't be treated specially.
        od = OrderedDict()
        od.update(self=23)
        self.assertEqual(list(od.items()), [('self', 23)])
        od = OrderedDict()
        od.update(other={})
        self.assertEqual(list(od.items()), [('other', {})])
        od = OrderedDict()
        od.update(red=5, blue=6, other=7, self=8)
        self.assertEqual(sorted(list(od.items())),
                         [('blue', 6), ('other', 7), ('red', 5), ('self', 8)])

        # Make sure that direct calls to update do not clear previous contents
        # add that updates items are not moved to the end
        d = OrderedDict([('a', 1), ('b', 2), ('c', 3), ('d', 44), ('e', 55)])
        d.update([('e', 5), ('f', 6)], g=7, d=4)
        self.assertEqual(list(d.items()),
            [('a', 1), ('b', 2), ('c', 3), ('d', 4), ('e', 5), ('f', 6), ('g', 7)])

    def test_abc(self):
        self.assertIsInstance(OrderedDict(), MutableMapping)
        self.assertTrue(issubclass(OrderedDict, MutableMapping))

    def test_clear(self):
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        shuffle(pairs)
        od = OrderedDict(pairs)
        self.assertEqual(len(od), len(pairs))
        od.clear()
        self.assertEqual(len(od), 0)

    def test_delitem(self):
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        od = OrderedDict(pairs)
        del od['a']
        self.assertNotIn('a', od)
        with self.assertRaises(KeyError):
            del od['a']
        self.assertEqual(list(od.items()), pairs[:2] + pairs[3:])

    def test_setitem(self):
        od = OrderedDict([('d', 1), ('b', 2), ('c', 3), ('a', 4), ('e', 5)])
        od['c'] = 10           # existing element
        od['f'] = 20           # new element
        self.assertEqual(list(od.items()),
                         [('d', 1), ('b', 2), ('c', 10), ('a', 4), ('e', 5), ('f', 20)])

    def test_iterators(self):
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        shuffle(pairs)
        od = OrderedDict(pairs)
        self.assertEqual(list(od), [t[0] for t in pairs])
        self.assertEqual(od.keys()[:], [t[0] for t in pairs])
        self.assertEqual(od.values()[:], [t[1] for t in pairs])
        self.assertEqual(od.items()[:], pairs)
        self.assertEqual(list(od.iterkeys()), [t[0] for t in pairs])
        self.assertEqual(list(od.itervalues()), [t[1] for t in pairs])
        self.assertEqual(list(od.iteritems()), pairs)
        self.assertEqual(list(reversed(od)),
                         [t[0] for t in reversed(pairs)])

    def test_popitem(self):
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        shuffle(pairs)
        od = OrderedDict(pairs)
        while pairs:
            self.assertEqual(od.popitem(), pairs.pop())
        with self.assertRaises(KeyError):
            od.popitem()
        self.assertEqual(len(od), 0)

    def test_pop(self):
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        shuffle(pairs)
        od = OrderedDict(pairs)
        shuffle(pairs)
        while pairs:
            k, v = pairs.pop()
            self.assertEqual(od.pop(k), v)
        with self.assertRaises(KeyError):
            od.pop('xyz')
        self.assertEqual(len(od), 0)
        self.assertEqual(od.pop(k, 12345), 12345)

        # make sure pop still works when __missing__ is defined
        class Missing(OrderedDict):
            def __missing__(self, key):
                return 0
        m = Missing(a=1)
        self.assertEqual(m.pop('b', 5), 5)
        self.assertEqual(m.pop('a', 6), 1)
        self.assertEqual(m.pop('a', 6), 6)
        with self.assertRaises(KeyError):
            m.pop('a')

    def test_equality(self):
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        shuffle(pairs)
        od1 = OrderedDict(pairs)
        od2 = OrderedDict(pairs)
        self.assertEqual(od1, od2)          # same order implies equality
        pairs = pairs[2:] + pairs[:2]
        od2 = OrderedDict(pairs)
        self.assertNotEqual(od1, od2)       # different order implies inequality
        # comparison to regular dict is not order sensitive
        self.assertEqual(od1, dict(od2))
        self.assertEqual(dict(od2), od1)
        # different length implied inequality
        self.assertNotEqual(od1, OrderedDict(pairs[:-1]))

    def test_copying(self):
        # Check that ordered dicts are copyable, deepcopyable, picklable,
        # and have a repr/eval round-trip
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        od = OrderedDict(pairs)
        update_test = OrderedDict()
        update_test.update(od)
        for i, dup in enumerate([
                    od.copy(),
                    copy.copy(od),
                    copy.deepcopy(od),
                    pickle.loads(pickle.dumps(od, 0)),
                    pickle.loads(pickle.dumps(od, 1)),
                    pickle.loads(pickle.dumps(od, 2)),
                    pickle.loads(pickle.dumps(od, -1)),
                    eval(repr(od)),
                    update_test,
                    OrderedDict(od),
                    ]):
            self.assertTrue(dup is not od)
            self.assertEqual(dup, od)
            self.assertEqual(list(dup.items()), list(od.items()))
            self.assertEqual(len(dup), len(od))
            self.assertEqual(type(dup), type(od))

    def test_yaml_linkage(self):
        # Verify that __reduce__ is setup in a way that supports PyYAML's dump() feature.
        # In yaml, lists are native but tuples are not.
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        od = OrderedDict(pairs)
        # yaml.dump(od) -->
        # '!!python/object/apply:__main__.OrderedDict\n- - [a, 1]\n  - [b, 2]\n'
        self.assertTrue(all(type(pair)==list for pair in od.__reduce__()[1]))

    def test_reduce_not_too_fat(self):
        # do not save instance dictionary if not needed
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        od = OrderedDict(pairs)
        self.assertEqual(len(od.__reduce__()), 2)
        od.x = 10
        self.assertEqual(len(od.__reduce__()), 3)

    def test_repr(self):
        od = OrderedDict([('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)])
        self.assertEqual(repr(od),
            "OrderedDict([('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)])")
        self.assertEqual(eval(repr(od)), od)
        self.assertEqual(repr(OrderedDict()), "OrderedDict()")

    def test_repr_recursive(self):
        # See issue #9826
        od = OrderedDict.fromkeys('abc')
        od['x'] = od
        self.assertEqual(repr(od),
            "OrderedDict([('a', None), ('b', None), ('c', None), ('x', ...)])")

    def test_setdefault(self):
        pairs = [('c', 1), ('b', 2), ('a', 3), ('d', 4), ('e', 5), ('f', 6)]
        shuffle(pairs)
        od = OrderedDict(pairs)
        pair_order = list(od.items())
        self.assertEqual(od.setdefault('a', 10), 3)
        # make sure order didn't change
        self.assertEqual(list(od.items()), pair_order)
        self.assertEqual(od.setdefault('x', 10), 10)
        # make sure 'x' is added to the end
        self.assertEqual(list(od.items())[-1], ('x', 10))

        # make sure setdefault still works when __missing__ is defined
        class Missing(OrderedDict):
            def __missing__(self, key):
                return 0
        self.assertEqual(Missing().setdefault(5, 9), 9)

    def test_reinsert(self):
        # Given insert a, insert b, delete a, re-insert a,
        # verify that a is now later than b.
        od = OrderedDict()
        od['a'] = 1
        od['b'] = 2
        del od['a']
        od['a'] = 1
        self.assertEqual(list(od.items()), [('b', 2), ('a', 1)])

    def test_views(self):
        s = 'the quick brown fox jumped over a lazy dog yesterday before dawn'.split()
        od = OrderedDict.fromkeys(s)
        self.assertEqual(list(od.viewkeys()),  s)
        self.assertEqual(list(od.viewvalues()),  [None for k in s])
        self.assertEqual(list(od.viewitems()),  [(k, None) for k in s])

    def test_override_update(self):
        # Verify that subclasses can override update() without breaking __init__()
        class MyOD(OrderedDict):
            def update(self, *args, **kwds):
                raise Exception()
        items = [('a', 1), ('c', 3), ('b', 2)]
        self.assertEqual(list(MyOD(items).items()), items)

class GeneralMappingTests(mapping_tests.BasicTestMappingProtocol):
    type2test = OrderedDict

    def test_popitem(self):
        d = self._empty_mapping()
        self.assertRaises(KeyError, d.popitem)

class MyOrderedDict(OrderedDict):
    pass

class SubclassMappingTests(mapping_tests.BasicTestMappingProtocol):
    type2test = MyOrderedDict

    def test_popitem(self):
        d = self._empty_mapping()
        self.assertRaises(KeyError, d.popitem)

import collections

def test_main(verbose=None):
    NamedTupleDocs = doctest.DocTestSuite(module=collections)
    test_classes = [TestNamedTuple, NamedTupleDocs, TestOneTrickPonyABCs,
                    TestCollectionABCs, TestCounter,
                    TestOrderedDict, GeneralMappingTests, SubclassMappingTests]
    test_support.run_unittest(*test_classes)
    test_support.run_doctest(collections, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
