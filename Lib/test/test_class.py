"Test the functionality of Python classes implementing operators."

from test_support import TestFailed

testmeths = [

# Binary operations
    "add",
    "radd",
    "sub",
    "rsub",
    "mul",
    "rmul",
    "div",
    "rdiv",
    "mod",
    "rmod",
    "divmod",
    "rdivmod",
    "pow",
    "rpow",
    "rshift",
    "rrshift",
    "lshift",
    "rlshift",
    "and",
    "rand",
    "or",
    "ror",
    "xor",
    "rxor",

# List/dict operations
    "contains",
    "getitem",
    "getslice",
    "setitem",
    "setslice",
    "delitem",
    "delslice",

# Unary operations
    "neg",
    "pos",
    "abs",
    "int",
    "long",
    "float",
    "oct",
    "hex",

# generic operations
    "init",
    ]

# These need to return something other than None
#    "coerce",
#    "hash",
#    "str",
#    "repr",

# These are separate because they can influence the test of other methods.
#    "getattr",
#    "setattr",
#    "delattr",

class AllTests:
    def __coerce__(self, *args):
        print "__coerce__:", args
        return (self,) + args

    def __hash__(self, *args):
        print "__hash__:", args
        return hash(id(self))

    def __str__(self, *args):
        print "__str__:", args
        return "AllTests"

    def __repr__(self, *args):
        print "__repr__:", args
        return "AllTests"

    def __cmp__(self, *args):
        print "__cmp__:", args
        return 0

    def __del__(self, *args):
        print "__del__:", args

# Synthesize AllTests methods from the names in testmeths.

method_template = """\
def __%(method)s__(self, *args):
    print "__%(method)s__:", args
"""

for method in testmeths:
    exec method_template % locals() in AllTests.__dict__

del method, method_template

# this also tests __init__ of course.
testme = AllTests()

# Binary operations

testme + 1
1 + testme

testme - 1
1 - testme

testme * 1
1 * testme

if 1/2 == 0:
    testme / 1
    1 / testme
else:
    # True division is in effect, so "/" doesn't map to __div__ etc; but
    # the canned expected-output file requires that __div__ etc get called.
    testme.__coerce__(1)
    testme.__div__(1)
    testme.__coerce__(1)
    testme.__rdiv__(1)

testme % 1
1 % testme

divmod(testme,1)
divmod(1, testme)

testme ** 1
1 ** testme

testme >> 1
1 >> testme

testme << 1
1 << testme

testme & 1
1 & testme

testme | 1
1 | testme

testme ^ 1
1 ^ testme


# List/dict operations

1 in testme

testme[1]
testme[1] = 1
del testme[1]

testme[:42]
testme[:42] = "The Answer"
del testme[:42]

testme[2:1024:10]
testme[2:1024:10] = "A lot"
del testme[2:1024:10]

testme[:42, ..., :24:, 24, 100]
testme[:42, ..., :24:, 24, 100] = "Strange"
del testme[:42, ..., :24:, 24, 100]


# Now remove the slice hooks to see if converting normal slices to slice
# object works.

del AllTests.__getslice__
del AllTests.__setslice__
del AllTests.__delslice__

import sys
if sys.platform[:4] != 'java':
    testme[:42]
    testme[:42] = "The Answer"
    del testme[:42]
else:
    # This works under Jython, but the actual slice values are
    # different.
    print "__getitem__: (slice(0, 42, None),)"
    print "__setitem__: (slice(0, 42, None), 'The Answer')"
    print "__delitem__: (slice(0, 42, None),)"

# Unary operations

-testme
+testme
abs(testme)
if sys.platform[:4] != 'java':
    int(testme)
    long(testme)
    float(testme)
    oct(testme)
    hex(testme)
else:
    # Jython enforced that the these methods return
    # a value of the expected type.
    print "__int__: ()"
    print "__long__: ()"
    print "__float__: ()"
    print "__oct__: ()"
    print "__hex__: ()"


# And the rest...

hash(testme)
repr(testme)
str(testme)

testme == 1
testme < 1
testme > 1
testme <> 1
testme != 1
1 == testme
1 < testme
1 > testme
1 <> testme
1 != testme

# This test has to be last (duh.)

del testme
if sys.platform[:4] == 'java':
    import java
    java.lang.System.gc()

# Interfering tests

class ExtraTests:
    def __getattr__(self, *args):
        print "__getattr__:", args
        return "SomeVal"

    def __setattr__(self, *args):
        print "__setattr__:", args

    def __delattr__(self, *args):
        print "__delattr__:", args

testme = ExtraTests()
testme.spam
testme.eggs = "spam, spam, spam and ham"
del testme.cardinal


# Test correct errors from hash() on objects with comparisons but no __hash__

class C0:
    pass

hash(C0()) # This should work; the next two should raise TypeError

class C1:
    def __cmp__(self, other): return 0

try: hash(C1())
except TypeError: pass
else: raise TestFailed, "hash(C1()) should raise an exception"

class C2:
    def __eq__(self, other): return 1

try: hash(C2())
except TypeError: pass
else: raise TestFailed, "hash(C2()) should raise an exception"


# Test for SF bug 532646

class A:
    pass
A.__call__ = A()
a = A()
try:
    a() # This should not segfault
except RuntimeError:
    pass
else:
    raise TestFailed, "how could this not have overflowed the stack?"


# Tests for exceptions raised in instance_getattr2().

def booh(self):
    raise AttributeError, "booh"

class A:
    a = property(booh)
try:
    A().a # Raised AttributeError: A instance has no attribute 'a'
except AttributeError, x:
    if str(x) is not "booh":
        print "attribute error for A().a got masked:", str(x)

class E:
    __eq__ = property(booh)
E() == E() # In debug mode, caused a C-level assert() to fail

class I:
    __init__ = property(booh)
try:
    I() # In debug mode, printed XXX undetected error and raises AttributeError
except AttributeError, x:
    pass
else:
    print "attribute error for I.__init__ got masked"
