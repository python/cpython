from test.test_support import verbose
import random

nerrors = 0

def check(tag, expected, raw, compare=None):
    global nerrors

    if verbose:
        print "    checking", tag

    orig = raw[:]   # save input in case of error
    if compare:
        raw.sort(compare)
    else:
        raw.sort()

    if len(expected) != len(raw):
        print "error in", tag
        print "length mismatch;", len(expected), len(raw)
        print expected
        print orig
        print raw
        nerrors += 1
        return

    for i, good in enumerate(expected):
        maybe = raw[i]
        if good is not maybe:
            print "error in", tag
            print "out of order at index", i, good, maybe
            print expected
            print orig
            print raw
            nerrors += 1
            return

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
                print "        complaining at", self, other
            raise RuntimeError
        return self.i < other.i

    def __repr__(self):
        return "Complains(%d)" % self.i

class Stable(object):
    def __init__(self, key, i):
        self.key = key
        self.index = i

    def __cmp__(self, other):
        return cmp(self.key, other.key)

    def __repr__(self):
        return "Stable(%d, %d)" % (self.key, self.index)

for n in sizes:
    x = range(n)
    if verbose:
        print "Testing size", n

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
    check("reversed via function", y, s, lambda a, b: cmp(b, a))

    if verbose:
        print "    Checking against an insane comparison function."
        print "        If the implementation isn't careful, this may segfault."
    s = x[:]
    s.sort(lambda a, b:  int(random.random() * 3) - 1)
    check("an insane function left some permutation", x, s)

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

    s = [Stable(random.randrange(10), i) for i in xrange(n)]
    augmented = [(e, e.index) for e in s]
    augmented.sort()    # forced stable because ties broken by index
    x = [e for e, i in augmented] # a stable sort of s
    check("stability", x, s)

def bug453523():
    global nerrors
    from random import random

    # If this fails, the most likely outcome is a core dump.
    if verbose:
        print "Testing bug 453523 -- list.sort() crasher."

    class C:
        def __lt__(self, other):
            if L and random() < 0.75:
                pop()
            else:
                push(3)
            return random() < 0.5

    L = [C() for i in range(50)]
    pop = L.pop
    push = L.append
    try:
        L.sort()
    except ValueError:
        pass
    else:
        print "    Mutation during list.sort() wasn't caught."
        nerrors += 1

bug453523()

def cmpNone():
    global nerrors

    if verbose:
        print "Testing None as a comparison function."

    L = range(50)
    random.shuffle(L)
    try:
        L.sort(None)
    except TypeError:
        print "    Passing None as cmpfunc failed."
        nerrors += 1
    else:
        if L != range(50):
            print "    Passing None as cmpfunc failed."
            nerrors += 1

cmpNone()

if nerrors:
    print "Test failed", nerrors
elif verbose:
    print "Test passed -- no errors."
