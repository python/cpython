from test.test_support import verbose, TESTFN
import random
import os

# From SF bug #422121:  Insecurities in dict comparison.

# Safety of code doing comparisons has been an historical Python weak spot.
# The problem is that comparison of structures written in C *naturally*
# wants to hold on to things like the size of the container, or "the
# biggest" containee so far, across a traversal of the container; but
# code to do containee comparisons can call back into Python and mutate
# the container in arbitrary ways while the C loop is in midstream.  If the
# C code isn't extremely paranoid about digging things out of memory on
# each trip, and artificially boosting refcounts for the duration, anything
# from infinite loops to OS crashes can result (yes, I use Windows <wink>).
#
# The other problem is that code designed to provoke a weakness is usually
# white-box code, and so catches only the particular vulnerabilities the
# author knew to protect against.  For example, Python's list.sort() code
# went thru many iterations as one "new" vulnerability after another was
# discovered.
#
# So the dict comparison test here uses a black-box approach instead,
# generating dicts of various sizes at random, and performing random
# mutations on them at random times.  This proved very effective,
# triggering at least six distinct failure modes the first 20 times I
# ran it.  Indeed, at the start, the driver never got beyond 6 iterations
# before the test died.

# The dicts are global to make it easy to mutate tham from within functions.
dict1 = {}
dict2 = {}

# The current set of keys in dict1 and dict2.  These are materialized as
# lists to make it easy to pick a dict key at random.
dict1keys = []
dict2keys = []

# Global flag telling maybe_mutate() whether to *consider* mutating.
mutate = 0

# If global mutate is true, consider mutating a dict.  May or may not
# mutate a dict even if mutate is true.  If it does decide to mutate a
# dict, it picks one of {dict1, dict2} at random, and deletes a random
# entry from it; or, more rarely, adds a random element.

def maybe_mutate():
    global mutate
    if not mutate:
        return
    if random.random() < 0.5:
        return

    if random.random() < 0.5:
        target, keys = dict1, dict1keys
    else:
        target, keys = dict2, dict2keys

    if random.random() < 0.2:
        # Insert a new key.
        mutate = 0   # disable mutation until key inserted
        while 1:
            newkey = Horrid(random.randrange(100))
            if newkey not in target:
                break
        target[newkey] = Horrid(random.randrange(100))
        keys.append(newkey)
        mutate = 1

    elif keys:
        # Delete a key at random.
        mutate = 0   # disable mutation until key deleted
        i = random.randrange(len(keys))
        key = keys[i]
        del target[key]
        del keys[i]
        mutate = 1

# A horrid class that triggers random mutations of dict1 and dict2 when
# instances are compared.

class Horrid:
    def __init__(self, i):
        # Comparison outcomes are determined by the value of i.
        self.i = i

        # An artificial hashcode is selected at random so that we don't
        # have any systematic relationship between comparison outcomes
        # (based on self.i and other.i) and relative position within the
        # hash vector (based on hashcode).
        self.hashcode = random.randrange(1000000000)

    def __hash__(self):
        return 42
        return self.hashcode

    def __cmp__(self, other):
        maybe_mutate()   # The point of the test.
        return cmp(self.i, other.i)

    def __eq__(self, other):
        maybe_mutate()   # The point of the test.
        return self.i == other.i

    def __repr__(self):
        return "Horrid(%d)" % self.i

# Fill dict d with numentries (Horrid(i), Horrid(j)) key-value pairs,
# where i and j are selected at random from the candidates list.
# Return d.keys() after filling.

def fill_dict(d, candidates, numentries):
    d.clear()
    for i in xrange(numentries):
        d[Horrid(random.choice(candidates))] = \
            Horrid(random.choice(candidates))
    return d.keys()

# Test one pair of randomly generated dicts, each with n entries.
# Note that dict comparison is trivial if they don't have the same number
# of entires (then the "shorter" dict is instantly considered to be the
# smaller one, without even looking at the entries).

def test_one(n):
    global mutate, dict1, dict2, dict1keys, dict2keys

    # Fill the dicts without mutating them.
    mutate = 0
    dict1keys = fill_dict(dict1, range(n), n)
    dict2keys = fill_dict(dict2, range(n), n)

    # Enable mutation, then compare the dicts so long as they have the
    # same size.
    mutate = 1
    if verbose:
        print "trying w/ lengths", len(dict1), len(dict2),
    while dict1 and len(dict1) == len(dict2):
        if verbose:
            print ".",
        if random.random() < 0.5:
            c = cmp(dict1, dict2)
        else:
            c = dict1 == dict2
    if verbose:
        print

# Run test_one n times.  At the start (before the bugs were fixed), 20
# consecutive runs of this test each blew up on or before the sixth time
# test_one was run.  So n doesn't have to be large to get an interesting
# test.
# OTOH, calling with large n is also interesting, to ensure that the fixed
# code doesn't hold on to refcounts *too* long (in which case memory would
# leak).

def test(n):
    for i in xrange(n):
        test_one(random.randrange(1, 100))

# See last comment block for clues about good values for n.
test(100)

##########################################################################
# Another segfault bug, distilled by Michael Hudson from a c.l.py post.

class Child:
    def __init__(self, parent):
        self.__dict__['parent'] = parent
    def __getattr__(self, attr):
        self.parent.a = 1
        self.parent.b = 1
        self.parent.c = 1
        self.parent.d = 1
        self.parent.e = 1
        self.parent.f = 1
        self.parent.g = 1
        self.parent.h = 1
        self.parent.i = 1
        return getattr(self.parent, attr)

class Parent:
    def __init__(self):
        self.a = Child(self)

# Hard to say what this will print!  May vary from time to time.  But
# we're specifically trying to test the tp_print slot here, and this is
# the clearest way to do it.  We print the result to a temp file so that
# the expected-output file doesn't need to change.

f = open(TESTFN, "w")
print >> f, Parent().__dict__
f.close()
os.unlink(TESTFN)

##########################################################################
# And another core-dumper from Michael Hudson.

dict = {}

# Force dict to malloc its table.
for i in range(1, 10):
    dict[i] = i

f = open(TESTFN, "w")

class Machiavelli:
    def __repr__(self):
        dict.clear()

        # Michael sez:  "doesn't crash without this.  don't know why."
        # Tim sez:  "luck of the draw; crashes with or without for me."
        print >> f

        return `"machiavelli"`

    def __hash__(self):
        return 0

dict[Machiavelli()] = Machiavelli()

print >> f, str(dict)
f.close()
os.unlink(TESTFN)
del f, dict


##########################################################################
# And another core-dumper from Michael Hudson.

dict = {}

# let's force dict to malloc its table
for i in range(1, 10):
    dict[i] = i

class Machiavelli2:
    def __eq__(self, other):
        dict.clear()
        return 1

    def __hash__(self):
        return 0

dict[Machiavelli2()] = Machiavelli2()

try:
    dict[Machiavelli2()]
except KeyError:
    pass

del dict

##########################################################################
# And another core-dumper from Michael Hudson.

dict = {}

# let's force dict to malloc its table
for i in range(1, 10):
    dict[i] = i

class Machiavelli3:
    def __init__(self, id):
        self.id = id

    def __eq__(self, other):
        if self.id == other.id:
            dict.clear()
            return 1
        else:
            return 0

    def __repr__(self):
        return "%s(%s)"%(self.__class__.__name__, self.id)

    def __hash__(self):
        return 0

dict[Machiavelli3(1)] = Machiavelli3(0)
dict[Machiavelli3(2)] = Machiavelli3(0)

f = open(TESTFN, "w")
try:
    try:
        print >> f, dict[Machiavelli3(2)]
    except KeyError:
        pass
finally:
    f.close()
    os.unlink(TESTFN)

del dict
del dict1, dict2, dict1keys, dict2keys
