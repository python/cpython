from collections import deque
import unittest
from test import test_support
from weakref import proxy
import copy
import cPickle as pickle
from cStringIO import StringIO
import random
import os

BIG = 100000

def fail():
    raise SyntaxError
    yield 1

class TestBasic(unittest.TestCase):

    def test_basics(self):
        d = deque(xrange(100))
        d.__init__(xrange(100, 200))
        for i in xrange(200, 400):
            d.append(i)
        for i in reversed(xrange(-200, 0)):
            d.appendleft(i)
        self.assertEqual(list(d), range(-200, 400))
        self.assertEqual(len(d), 600)

        left = [d.popleft() for i in xrange(250)]
        self.assertEqual(left, range(-200, 50))
        self.assertEqual(list(d), range(50, 400))

        right = [d.pop() for i in xrange(250)]
        right.reverse()
        self.assertEqual(right, range(150, 400))
        self.assertEqual(list(d), range(50, 150))

    def test_comparisons(self):
        d = deque('xabc'); d.popleft()
        for e in [d, deque('abc'), deque('ab'), deque(), list(d)]:
            self.assertEqual(d==e, type(d)==type(e) and list(d)==list(e))
            self.assertEqual(d!=e, not(type(d)==type(e) and list(d)==list(e)))

        args = map(deque, ('', 'a', 'b', 'ab', 'ba', 'abc', 'xba', 'xabc', 'cba'))
        for x in args:
            for y in args:
                self.assertEqual(x == y, list(x) == list(y), (x,y))
                self.assertEqual(x != y, list(x) != list(y), (x,y))
                self.assertEqual(x <  y, list(x) <  list(y), (x,y))
                self.assertEqual(x <= y, list(x) <= list(y), (x,y))
                self.assertEqual(x >  y, list(x) >  list(y), (x,y))
                self.assertEqual(x >= y, list(x) >= list(y), (x,y))
                self.assertEqual(cmp(x,y), cmp(list(x),list(y)), (x,y))

    def test_extend(self):
        d = deque('a')
        self.assertRaises(TypeError, d.extend, 1)
        d.extend('bcd')
        self.assertEqual(list(d), list('abcd'))

    def test_extendleft(self):
        d = deque('a')
        self.assertRaises(TypeError, d.extendleft, 1)
        d.extendleft('bcd')
        self.assertEqual(list(d), list(reversed('abcd')))
        d = deque()
        d.extendleft(range(1000))
        self.assertEqual(list(d), list(reversed(range(1000))))
        self.assertRaises(SyntaxError, d.extendleft, fail())

    def test_getitem(self):
        n = 200
        d = deque(xrange(n))
        l = range(n)
        for i in xrange(n):
            d.popleft()
            l.pop(0)
            if random.random() < 0.5:
                d.append(i)
                l.append(i)
            for j in xrange(1-len(l), len(l)):
                assert d[j] == l[j]

        d = deque('superman')
        self.assertEqual(d[0], 's')
        self.assertEqual(d[-1], 'n')
        d = deque()
        self.assertRaises(IndexError, d.__getitem__, 0)
        self.assertRaises(IndexError, d.__getitem__, -1)

    def test_setitem(self):
        n = 200
        d = deque(xrange(n))
        for i in xrange(n):
            d[i] = 10 * i
        self.assertEqual(list(d), [10*i for i in xrange(n)])
        l = list(d)
        for i in xrange(1-n, 0, -1):
            d[i] = 7*i
            l[i] = 7*i
        self.assertEqual(list(d), l)

    def test_delitem(self):
        n = 500         # O(n**2) test, don't make this too big
        d = deque(xrange(n))
        self.assertRaises(IndexError, d.__delitem__, -n-1)
        self.assertRaises(IndexError, d.__delitem__, n)
        for i in xrange(n):
            self.assertEqual(len(d), n-i)
            j = random.randrange(-len(d), len(d))
            val = d[j]
            self.assert_(val in d)
            del d[j]
            self.assert_(val not in d)
        self.assertEqual(len(d), 0)

    def test_rotate(self):
        s = tuple('abcde')
        n = len(s)

        d = deque(s)
        d.rotate(1)             # verify rot(1)
        self.assertEqual(''.join(d), 'eabcd')

        d = deque(s)
        d.rotate(-1)            # verify rot(-1)
        self.assertEqual(''.join(d), 'bcdea')
        d.rotate()              # check default to 1
        self.assertEqual(tuple(d), s)

        for i in xrange(n*3):
            d = deque(s)
            e = deque(d)
            d.rotate(i)         # check vs. rot(1) n times
            for j in xrange(i):
                e.rotate(1)
            self.assertEqual(tuple(d), tuple(e))
            d.rotate(-i)        # check that it works in reverse
            self.assertEqual(tuple(d), s)
            e.rotate(n-i)       # check that it wraps forward
            self.assertEqual(tuple(e), s)

        for i in xrange(n*3):
            d = deque(s)
            e = deque(d)
            d.rotate(-i)
            for j in xrange(i):
                e.rotate(-1)    # check vs. rot(-1) n times
            self.assertEqual(tuple(d), tuple(e))
            d.rotate(i)         # check that it works in reverse
            self.assertEqual(tuple(d), s)
            e.rotate(i-n)       # check that it wraps backaround
            self.assertEqual(tuple(e), s)

        d = deque(s)
        e = deque(s)
        e.rotate(BIG+17)        # verify on long series of rotates
        dr = d.rotate
        for i in xrange(BIG+17):
            dr()
        self.assertEqual(tuple(d), tuple(e))

        self.assertRaises(TypeError, d.rotate, 'x')   # Wrong arg type
        self.assertRaises(TypeError, d.rotate, 1, 10) # Too many args

        d = deque()
        d.rotate()              # rotate an empty deque
        self.assertEqual(d, deque())

    def test_len(self):
        d = deque('ab')
        self.assertEqual(len(d), 2)
        d.popleft()
        self.assertEqual(len(d), 1)
        d.pop()
        self.assertEqual(len(d), 0)
        self.assertRaises(IndexError, d.pop)
        self.assertEqual(len(d), 0)
        d.append('c')
        self.assertEqual(len(d), 1)
        d.appendleft('d')
        self.assertEqual(len(d), 2)
        d.clear()
        self.assertEqual(len(d), 0)

    def test_underflow(self):
        d = deque()
        self.assertRaises(IndexError, d.pop)
        self.assertRaises(IndexError, d.popleft)

    def test_clear(self):
        d = deque(xrange(100))
        self.assertEqual(len(d), 100)
        d.clear()
        self.assertEqual(len(d), 0)
        self.assertEqual(list(d), [])
        d.clear()               # clear an emtpy deque
        self.assertEqual(list(d), [])

    def test_repr(self):
        d = deque(xrange(200))
        e = eval(repr(d))
        self.assertEqual(list(d), list(e))
        d.append(d)
        self.assert_('...' in repr(d))

    def test_print(self):
        d = deque(xrange(200))
        d.append(d)
        try:
            fo = open(test_support.TESTFN, "wb")
            print >> fo, d,
            fo.close()
            fo = open(test_support.TESTFN, "rb")
            self.assertEqual(fo.read(), repr(d))
        finally:
            fo.close()
            os.remove(test_support.TESTFN)

    def test_init(self):
        self.assertRaises(TypeError, deque, 'abc', 2);
        self.assertRaises(TypeError, deque, 1);

    def test_hash(self):
        self.assertRaises(TypeError, hash, deque('abc'))

    def test_long_steadystate_queue_popleft(self):
        for size in (0, 1, 2, 100, 1000):
            d = deque(xrange(size))
            append, pop = d.append, d.popleft
            for i in xrange(size, BIG):
                append(i)
                x = pop()
                if x != i - size:
                    self.assertEqual(x, i-size)
            self.assertEqual(list(d), range(BIG-size, BIG))

    def test_long_steadystate_queue_popright(self):
        for size in (0, 1, 2, 100, 1000):
            d = deque(reversed(xrange(size)))
            append, pop = d.appendleft, d.pop
            for i in xrange(size, BIG):
                append(i)
                x = pop()
                if x != i - size:
                    self.assertEqual(x, i-size)
            self.assertEqual(list(reversed(list(d))), range(BIG-size, BIG))

    def test_big_queue_popleft(self):
        pass
        d = deque()
        append, pop = d.append, d.popleft
        for i in xrange(BIG):
            append(i)
        for i in xrange(BIG):
            x = pop()
            if x != i:
                self.assertEqual(x, i)

    def test_big_queue_popright(self):
        d = deque()
        append, pop = d.appendleft, d.pop
        for i in xrange(BIG):
            append(i)
        for i in xrange(BIG):
            x = pop()
            if x != i:
                self.assertEqual(x, i)

    def test_big_stack_right(self):
        d = deque()
        append, pop = d.append, d.pop
        for i in xrange(BIG):
            append(i)
        for i in reversed(xrange(BIG)):
            x = pop()
            if x != i:
                self.assertEqual(x, i)
        self.assertEqual(len(d), 0)

    def test_big_stack_left(self):
        d = deque()
        append, pop = d.appendleft, d.popleft
        for i in xrange(BIG):
            append(i)
        for i in reversed(xrange(BIG)):
            x = pop()
            if x != i:
                self.assertEqual(x, i)
        self.assertEqual(len(d), 0)

    def test_roundtrip_iter_init(self):
        d = deque(xrange(200))
        e = deque(d)
        self.assertNotEqual(id(d), id(e))
        self.assertEqual(list(d), list(e))

    def test_pickle(self):
        d = deque(xrange(200))
        for i in (0, 1, 2):
            s = pickle.dumps(d, i)
            e = pickle.loads(s)
            self.assertNotEqual(id(d), id(e))
            self.assertEqual(list(d), list(e))

    def test_pickle_recursive(self):
        d = deque('abc')
        d.append(d)
        for i in (0, 1, 2):
            e = pickle.loads(pickle.dumps(d, i))
            self.assertNotEqual(id(d), id(e))
            self.assertEqual(id(e), id(e[-1]))

    def test_deepcopy(self):
        mut = [10]
        d = deque([mut])
        e = copy.deepcopy(d)
        self.assertEqual(list(d), list(e))
        mut[0] = 11
        self.assertNotEqual(id(d), id(e))
        self.assertNotEqual(list(d), list(e))

    def test_copy(self):
        mut = [10]
        d = deque([mut])
        e = copy.copy(d)
        self.assertEqual(list(d), list(e))
        mut[0] = 11
        self.assertNotEqual(id(d), id(e))
        self.assertEqual(list(d), list(e))

    def test_reversed(self):
        for s in ('abcd', xrange(2000)):
            self.assertEqual(list(reversed(deque(s))), list(reversed(s)))

    def test_gc_doesnt_blowup(self):
        import gc
        # This used to assert-fail in deque_traverse() under a debug
        # build, or run wild with a NULL pointer in a release build.
        d = deque()
        for i in xrange(100):
            d.append(1)
            gc.collect()

def R(seqn):
    'Regular generator'
    for i in seqn:
        yield i

class G:
    'Sequence using __getitem__'
    def __init__(self, seqn):
        self.seqn = seqn
    def __getitem__(self, i):
        return self.seqn[i]

class I:
    'Sequence using iterator protocol'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def next(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class Ig:
    'Sequence using iterator protocol defined with a generator'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        for val in self.seqn:
            yield val

class X:
    'Missing __getitem__ and __iter__'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def next(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class N:
    'Iterator missing next()'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self

class E:
    'Test propagation of exceptions'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def next(self):
        3 // 0

class S:
    'Test immediate stop'
    def __init__(self, seqn):
        pass
    def __iter__(self):
        return self
    def next(self):
        raise StopIteration

from itertools import chain, imap
def L(seqn):
    'Test multiple tiers of iterators'
    return chain(imap(lambda x:x, R(Ig(G(seqn)))))


class TestVariousIteratorArgs(unittest.TestCase):

    def test_constructor(self):
        for s in ("123", "", range(1000), ('do', 1.2), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                self.assertEqual(list(deque(g(s))), list(g(s)))
            self.assertRaises(TypeError, deque, X(s))
            self.assertRaises(TypeError, deque, N(s))
            self.assertRaises(ZeroDivisionError, deque, E(s))

    def test_iter_with_altered_data(self):
        d = deque('abcdefg')
        it = iter(d)
        d.pop()
        self.assertRaises(RuntimeError, it.next)

class Deque(deque):
    pass

class DequeWithBadIter(deque):
    def __iter__(self):
        raise TypeError

class TestSubclass(unittest.TestCase):

    def test_basics(self):
        d = Deque(xrange(100))
        d.__init__(xrange(100, 200))
        for i in xrange(200, 400):
            d.append(i)
        for i in reversed(xrange(-200, 0)):
            d.appendleft(i)
        self.assertEqual(list(d), range(-200, 400))
        self.assertEqual(len(d), 600)

        left = [d.popleft() for i in xrange(250)]
        self.assertEqual(left, range(-200, 50))
        self.assertEqual(list(d), range(50, 400))

        right = [d.pop() for i in xrange(250)]
        right.reverse()
        self.assertEqual(right, range(150, 400))
        self.assertEqual(list(d), range(50, 150))

        d.clear()
        self.assertEqual(len(d), 0)

    def test_copy_pickle(self):

        d = Deque('abc')

        e = d.__copy__()
        self.assertEqual(type(d), type(e))
        self.assertEqual(list(d), list(e))

        e = Deque(d)
        self.assertEqual(type(d), type(e))
        self.assertEqual(list(d), list(e))

        s = pickle.dumps(d)
        e = pickle.loads(s)
        self.assertNotEqual(id(d), id(e))
        self.assertEqual(type(d), type(e))
        self.assertEqual(list(d), list(e))

    def test_pickle(self):
        d = Deque('abc')
        d.append(d)

        e = pickle.loads(pickle.dumps(d))
        self.assertNotEqual(id(d), id(e))
        self.assertEqual(type(d), type(e))
        dd = d.pop()
        ee = e.pop()
        self.assertEqual(id(e), id(ee))
        self.assertEqual(d, e)

        d.x = d
        e = pickle.loads(pickle.dumps(d))
        self.assertEqual(id(e), id(e.x))

        d = DequeWithBadIter('abc')
        self.assertRaises(TypeError, pickle.dumps, d)

    def test_weakref(self):
        d = deque('gallahad')
        p = proxy(d)
        self.assertEqual(str(p), str(d))
        d = None
        self.assertRaises(ReferenceError, str, p)

    def test_strange_subclass(self):
        class X(deque):
            def __iter__(self):
                return iter([])
        d1 = X([1,2,3])
        d2 = X([4,5,6])
        d1 == d2   # not clear if this is supposed to be True or False,
                   # but it used to give a SystemError

#==============================================================================

libreftest = """
Example from the Library Reference:  Doc/lib/libcollections.tex

>>> from collections import deque
>>> d = deque('ghi')                 # make a new deque with three items
>>> for elem in d:                   # iterate over the deque's elements
...     print elem.upper()
G
H
I
>>> d.append('j')                    # add a new entry to the right side
>>> d.appendleft('f')                # add a new entry to the left side
>>> d                                # show the representation of the deque
deque(['f', 'g', 'h', 'i', 'j'])
>>> d.pop()                          # return and remove the rightmost item
'j'
>>> d.popleft()                      # return and remove the leftmost item
'f'
>>> list(d)                          # list the contents of the deque
['g', 'h', 'i']
>>> d[0]                             # peek at leftmost item
'g'
>>> d[-1]                            # peek at rightmost item
'i'
>>> list(reversed(d))                # list the contents of a deque in reverse
['i', 'h', 'g']
>>> 'h' in d                         # search the deque
True
>>> d.extend('jkl')                  # add multiple elements at once
>>> d
deque(['g', 'h', 'i', 'j', 'k', 'l'])
>>> d.rotate(1)                      # right rotation
>>> d
deque(['l', 'g', 'h', 'i', 'j', 'k'])
>>> d.rotate(-1)                     # left rotation
>>> d
deque(['g', 'h', 'i', 'j', 'k', 'l'])
>>> deque(reversed(d))               # make a new deque in reverse order
deque(['l', 'k', 'j', 'i', 'h', 'g'])
>>> d.clear()                        # empty the deque
>>> d.pop()                          # cannot pop from an empty deque
Traceback (most recent call last):
  File "<pyshell#6>", line 1, in -toplevel-
    d.pop()
IndexError: pop from an empty deque

>>> d.extendleft('abc')              # extendleft() reverses the input order
>>> d
deque(['c', 'b', 'a'])



>>> def delete_nth(d, n):
...     d.rotate(-n)
...     d.popleft()
...     d.rotate(n)
...
>>> d = deque('abcdef')
>>> delete_nth(d, 2)   # remove the entry at d[2]
>>> d
deque(['a', 'b', 'd', 'e', 'f'])



>>> def roundrobin(*iterables):
...     pending = deque(iter(i) for i in iterables)
...     while pending:
...         task = pending.popleft()
...         try:
...             yield task.next()
...         except StopIteration:
...             continue
...         pending.append(task)
...

>>> for value in roundrobin('abc', 'd', 'efgh'):
...     print value
...
a
d
e
b
f
c
g
h


>>> def maketree(iterable):
...     d = deque(iterable)
...     while len(d) > 1:
...         pair = [d.popleft(), d.popleft()]
...         d.append(pair)
...     return list(d)
...
>>> print maketree('abcdefgh')
[[[['a', 'b'], ['c', 'd']], [['e', 'f'], ['g', 'h']]]]

"""


#==============================================================================

__test__ = {'libreftest' : libreftest}

def test_main(verbose=None):
    import sys
    test_classes = (
        TestBasic,
        TestVariousIteratorArgs,
        TestSubclass,
    )

    test_support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

    # doctests
    from test import test_deque
    test_support.run_doctest(test_deque, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
