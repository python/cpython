""" Test Iterator Length Transparency

Some functions or methods which accept general iterable arguments have
optional, more efficient code paths if they know how many items to expect.
For instance, map(func, iterable), will pre-allocate the exact amount of
space required whenever the iterable can report its length.

The desired invariant is:  len(it)==len(list(it)).

A complication is that an iterable and iterator can be the same object. To
maintain the invariant, an iterator needs to dynamically update its length.
For instance, an iterable such as xrange(10) always reports its length as ten,
but it=iter(xrange(10)) starts at ten, and then goes to nine after it.next().
Having this capability means that map() can ignore the distinction between
map(func, iterable) and map(func, iter(iterable)).

When the iterable is immutable, the implementation can straight-forwardly
report the original length minus the cumulative number of calls to next().
This is the case for tuples, xrange objects, and itertools.repeat().

Some containers become temporarily immutable during iteration.  This includes
dicts, sets, and collections.deque.  Their implementation is equally simple
though they need to permantently set their length to zero whenever there is
an attempt to iterate after a length mutation.

The situation slightly more involved whenever an object allows length mutation
during iteration.  Lists and sequence iterators are dynanamically updatable.
So, if a list is extended during iteration, the iterator will continue through
the new items.  If it shrinks to a point before the most recent iteration,
then no further items are available and the length is reported at zero.

Reversed objects can also be wrapped around mutable objects; however, any
appends after the current position are ignored.  Any other approach leads
to confusion and possibly returning the same item more than once.

The iterators not listed above, such as enumerate and the other itertools,
are not length transparent because they have no way to distinguish between
iterables that report static length and iterators whose length changes with
each call (i.e. the difference between enumerate('abc') and
enumerate(iter('abc')).

"""

import unittest
from test import test_support
from itertools import repeat
from collections import deque
from __builtin__ import len as _len

n = 10

def len(obj):
    try:
        return _len(obj)
    except TypeError:
        try:
            # note: this is an internal undocumented API,
            # don't rely on it in your own programs
            return obj.__length_hint__()
        except AttributeError:
            raise TypeError

class TestInvariantWithoutMutations(unittest.TestCase):

    def test_invariant(self):
        it = self.it
        for i in reversed(xrange(1, n+1)):
            self.assertEqual(len(it), i)
            it.next()
        self.assertEqual(len(it), 0)
        self.assertRaises(StopIteration, it.next)
        self.assertEqual(len(it), 0)

class TestTemporarilyImmutable(TestInvariantWithoutMutations):

    def test_immutable_during_iteration(self):
        # objects such as deques, sets, and dictionaries enforce
        # length immutability  during iteration

        it = self.it
        self.assertEqual(len(it), n)
        it.next()
        self.assertEqual(len(it), n-1)
        self.mutate()
        self.assertRaises(RuntimeError, it.next)
        self.assertEqual(len(it), 0)

## ------- Concrete Type Tests -------

class TestRepeat(TestInvariantWithoutMutations):

    def setUp(self):
        self.it = repeat(None, n)

    def test_no_len_for_infinite_repeat(self):
        # The repeat() object can also be infinite
        self.assertRaises(TypeError, len, repeat(None))

class TestXrange(TestInvariantWithoutMutations):

    def setUp(self):
        self.it = iter(xrange(n))

class TestXrangeCustomReversed(TestInvariantWithoutMutations):

    def setUp(self):
        self.it = reversed(xrange(n))

class TestTuple(TestInvariantWithoutMutations):

    def setUp(self):
        self.it = iter(tuple(xrange(n)))

## ------- Types that should not be mutated during iteration -------

class TestDeque(TestTemporarilyImmutable):

    def setUp(self):
        d = deque(xrange(n))
        self.it = iter(d)
        self.mutate = d.pop

class TestDequeReversed(TestTemporarilyImmutable):

    def setUp(self):
        d = deque(xrange(n))
        self.it = reversed(d)
        self.mutate = d.pop

class TestDictKeys(TestTemporarilyImmutable):

    def setUp(self):
        d = dict.fromkeys(xrange(n))
        self.it = iter(d)
        self.mutate = d.popitem

class TestDictItems(TestTemporarilyImmutable):

    def setUp(self):
        d = dict.fromkeys(xrange(n))
        self.it = d.iteritems()
        self.mutate = d.popitem

class TestDictValues(TestTemporarilyImmutable):

    def setUp(self):
        d = dict.fromkeys(xrange(n))
        self.it = d.itervalues()
        self.mutate = d.popitem

class TestSet(TestTemporarilyImmutable):

    def setUp(self):
        d = set(xrange(n))
        self.it = iter(d)
        self.mutate = d.pop

## ------- Types that can mutate during iteration -------

class TestList(TestInvariantWithoutMutations):

    def setUp(self):
        self.it = iter(range(n))

    def test_mutation(self):
        d = range(n)
        it = iter(d)
        it.next()
        it.next()
        self.assertEqual(len(it), n-2)
        d.append(n)
        self.assertEqual(len(it), n-1)  # grow with append
        d[1:] = []
        self.assertEqual(len(it), 0)
        self.assertEqual(list(it), [])
        d.extend(xrange(20))
        self.assertEqual(len(it), 0)

class TestListReversed(TestInvariantWithoutMutations):

    def setUp(self):
        self.it = reversed(range(n))

    def test_mutation(self):
        d = range(n)
        it = reversed(d)
        it.next()
        it.next()
        self.assertEqual(len(it), n-2)
        d.append(n)
        self.assertEqual(len(it), n-2)  # ignore append
        d[1:] = []
        self.assertEqual(len(it), 0)
        self.assertEqual(list(it), [])  # confirm invariant
        d.extend(xrange(20))
        self.assertEqual(len(it), 0)

## -- Check to make sure exceptions are not suppressed by __length_hint__()


class BadLen(object):
    def __iter__(self): return iter(range(10))
    def __len__(self):
        raise RuntimeError('hello')

class BadLengthHint(object):
    def __iter__(self): return iter(range(10))
    def __length_hint__(self):
        raise RuntimeError('hello')

class NoneLengthHint(object):
    def __iter__(self): return iter(range(10))
    def __length_hint__(self):
        return None

class TestLengthHintExceptions(unittest.TestCase):

    def test_issue1242657(self):
        self.assertRaises(RuntimeError, list, BadLen())
        self.assertRaises(RuntimeError, list, BadLengthHint())
        self.assertRaises(RuntimeError, [].extend, BadLen())
        self.assertRaises(RuntimeError, [].extend, BadLengthHint())
        self.assertRaises(RuntimeError, zip, BadLen())
        self.assertRaises(RuntimeError, zip, BadLengthHint())
        self.assertRaises(RuntimeError, filter, None, BadLen())
        self.assertRaises(RuntimeError, filter, None, BadLengthHint())
        self.assertRaises(RuntimeError, map, chr, BadLen())
        self.assertRaises(RuntimeError, map, chr, BadLengthHint())
        b = bytearray(range(10))
        self.assertRaises(RuntimeError, b.extend, BadLen())
        self.assertRaises(RuntimeError, b.extend, BadLengthHint())

    def test_invalid_hint(self):
        # Make sure an invalid result doesn't muck-up the works
        self.assertEqual(list(NoneLengthHint()), list(range(10)))


def test_main():
    unittests = [
        TestRepeat,
        TestXrange,
        TestXrangeCustomReversed,
        TestTuple,
        TestDeque,
        TestDequeReversed,
        TestDictKeys,
        TestDictItems,
        TestDictValues,
        TestSet,
        TestList,
        TestListReversed,
        TestLengthHintExceptions,
    ]
    test_support.run_unittest(*unittests)

if __name__ == "__main__":
    test_main()
