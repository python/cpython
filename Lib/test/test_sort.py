from test import support
import random
import sys
import unittest

verbose = support.verbose
nerrors = 0

def CmpToKey(mycmp):
    'Convert a cmp= function into a key= function'
    class K(object):
        def __init__(self, obj):
            self.obj = obj
        def __lt__(self, other):
            return mycmp(self.obj, other.obj) == -1
    return K

def check(tag, expected, raw, compare=None):
    global nerrors

    if verbose:
        print("    checking", tag)

    orig = raw[:]   # save input in case of error
    if compare:
        raw.sort(key=CmpToKey(compare))
    else:
        raw.sort()

    if len(expected) != len(raw):
        print("error in", tag)
        print("length mismatch;", len(expected), len(raw))
        print(expected)
        print(orig)
        print(raw)
        nerrors += 1
        return

    for i, good in enumerate(expected):
        maybe = raw[i]
        if good is not maybe:
            print("error in", tag)
            print("out of order at index", i, good, maybe)
            print(expected)
            print(orig)
            print(raw)
            nerrors += 1
            return

class TestBase(unittest.TestCase):
    def testStressfully(self):
        # Try a variety of sizes at and around powers of 2, and at powers of 10.
        sizes = [0]
        for power in range(1, 10):
            n = 2 ** power
            sizes.extend(range(n-1, n+2))
        sizes.extend([10, 100, 1000])

        class Complains(object):
            maybe_complain = True

            def __init__(self, i):
                self.i = i

            def __lt__(self, other):
                if Complains.maybe_complain and random.random() < 0.001:
                    if verbose:
                        print("        complaining at", self, other)
                    raise RuntimeError
                return self.i < other.i

            def __repr__(self):
                return "Complains(%d)" % self.i

        class Stable(object):
            def __init__(self, key, i):
                self.key = key
                self.index = i

            def __lt__(self, other):
                return self.key < other.key

            def __repr__(self):
                return "Stable(%d, %d)" % (self.key, self.index)

        for n in sizes:
            x = list(range(n))
            if verbose:
                print("Testing size", n)

            s = x[:]
            check("identity", x, s)

            s = x[:]
            s.reverse()
            check("reversed", x, s)

            s = x[:]
            random.shuffle(s)
            check("random permutation", x, s)

            y = x[:]
            y.reverse()
            s = x[:]
            check("reversed via function", y, s, lambda a, b: (b>a)-(b<a))

            if verbose:
                print("    Checking against an insane comparison function.")
                print("        If the implementation isn't careful, this may segfault.")
            s = x[:]
            s.sort(key=CmpToKey(lambda a, b:  int(random.random() * 3) - 1))
            check("an insane function left some permutation", x, s)

            if len(x) >= 2:
                def bad_key(x):
                    raise RuntimeError
                s = x[:]
                self.assertRaises(RuntimeError, s.sort, key=bad_key)

            x = [Complains(i) for i in x]
            s = x[:]
            random.shuffle(s)
            Complains.maybe_complain = True
            it_complained = False
            try:
                s.sort()
            except RuntimeError:
                it_complained = True
            if it_complained:
                Complains.maybe_complain = False
                check("exception during sort left some permutation", x, s)

            s = [Stable(random.randrange(10), i) for i in range(n)]
            augmented = [(e, e.index) for e in s]
            augmented.sort()    # forced stable because ties broken by index
            x = [e for e, i in augmented] # a stable sort of s
            check("stability", x, s)

#==============================================================================

class TestBugs(unittest.TestCase):

    def test_bug453523(self):
        # bug 453523 -- list.sort() crasher.
        # If this fails, the most likely outcome is a core dump.
        # Mutations during a list sort should raise a ValueError.

        class C:
            def __lt__(self, other):
                if L and random.random() < 0.75:
                    L.pop()
                else:
                    L.append(3)
                return random.random() < 0.5

        L = [C() for i in range(50)]
        self.assertRaises(ValueError, L.sort)

    def test_undetected_mutation(self):
        # Python 2.4a1 did not always detect mutation
        memorywaster = []
        for i in range(20):
            def mutating_cmp(x, y):
                L.append(3)
                L.pop()
                return (x > y) - (x < y)
            L = [1,2]
            self.assertRaises(ValueError, L.sort, key=CmpToKey(mutating_cmp))
            def mutating_cmp(x, y):
                L.append(3)
                del L[:]
                return (x > y) - (x < y)
            self.assertRaises(ValueError, L.sort, key=CmpToKey(mutating_cmp))
            memorywaster = [memorywaster]

#==============================================================================

class TestDecorateSortUndecorate(unittest.TestCase):

    def test_decorated(self):
        data = 'The quick Brown fox Jumped over The lazy Dog'.split()
        copy = data[:]
        random.shuffle(data)
        data.sort(key=str.lower)
        def my_cmp(x, y):
            xlower, ylower = x.lower(), y.lower()
            return (xlower > ylower) - (xlower < ylower)
        copy.sort(key=CmpToKey(my_cmp))

    def test_baddecorator(self):
        data = 'The quick Brown fox Jumped over The lazy Dog'.split()
        self.assertRaises(TypeError, data.sort, key=lambda x,y: 0)

    def test_stability(self):
        data = [(random.randrange(100), i) for i in range(200)]
        copy = data[:]
        data.sort(key=lambda t: t[0])   # sort on the random first field
        copy.sort()                     # sort using both fields
        self.assertEqual(data, copy)    # should get the same result

    def test_key_with_exception(self):
        # Verify that the wrapper has been removed
        data = list(range(-2, 2))
        dup = data[:]
        self.assertRaises(ZeroDivisionError, data.sort, key=lambda x: 1/x)
        self.assertEqual(data, dup)

    def test_key_with_mutation(self):
        data = list(range(10))
        def k(x):
            del data[:]
            data[:] = range(20)
            return x
        self.assertRaises(ValueError, data.sort, key=k)

    def test_key_with_mutating_del(self):
        data = list(range(10))
        class SortKiller(object):
            def __init__(self, x):
                pass
            def __del__(self):
                del data[:]
                data[:] = range(20)
            def __lt__(self, other):
                return id(self) < id(other)
        self.assertRaises(ValueError, data.sort, key=SortKiller)

    def test_key_with_mutating_del_and_exception(self):
        data = list(range(10))
        ## dup = data[:]
        class SortKiller(object):
            def __init__(self, x):
                if x > 2:
                    raise RuntimeError
            def __del__(self):
                del data[:]
                data[:] = list(range(20))
        self.assertRaises(RuntimeError, data.sort, key=SortKiller)
        ## major honking subtlety: we *can't* do:
        ##
        ## self.assertEqual(data, dup)
        ##
        ## because there is a reference to a SortKiller in the
        ## traceback and by the time it dies we're outside the call to
        ## .sort() and so the list protection gimmicks are out of
        ## date (this cost some brain cells to figure out...).

    def test_reverse(self):
        data = list(range(100))
        random.shuffle(data)
        data.sort(reverse=True)
        self.assertEqual(data, list(range(99,-1,-1)))

    def test_reverse_stability(self):
        data = [(random.randrange(100), i) for i in range(200)]
        copy1 = data[:]
        copy2 = data[:]
        def my_cmp(x, y):
            x0, y0 = x[0], y[0]
            return (x0 > y0) - (x0 < y0)
        def my_cmp_reversed(x, y):
            x0, y0 = x[0], y[0]
            return (y0 > x0) - (y0 < x0)
        data.sort(key=CmpToKey(my_cmp), reverse=True)
        copy1.sort(key=CmpToKey(my_cmp_reversed))
        self.assertEqual(data, copy1)
        copy2.sort(key=lambda x: x[0], reverse=True)
        self.assertEqual(data, copy2)

#==============================================================================

def test_main(verbose=None):
    test_classes = (
        TestBase,
        TestDecorateSortUndecorate,
        TestBugs,
    )

    support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)

if __name__ == "__main__":
    test_main(verbose=True)
