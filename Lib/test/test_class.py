"Test the functionality of Python classes implementing operators."

from test.test_support import TestFailed

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

# generic operations
    "init",
    ]

# These need to return something other than None
#    "coerce",
#    "hash",
#    "str",
#    "repr",
#    "int",
#    "long",
#    "float",
#    "oct",
#    "hex",

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

    def __int__(self, *args):
        print "__int__:", args
        return 1

    def __float__(self, *args):
        print "__float__:", args
        return 1.0

    def __long__(self, *args):
        print "__long__:", args
        return 1L

    def __oct__(self, *args):
        print "__oct__:", args
        return '01'

    def __hex__(self, *args):
        print "__hex__:", args
        return '0x1'

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
int(testme)
long(testme)
float(testme)
oct(testme)
hex(testme)

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


# return values of some method are type-checked
class BadTypeClass:
    def __int__(self):
        return None
    __float__ = __int__
    __long__ = __int__
    __str__ = __int__
    __repr__ = __int__
    __oct__ = __int__
    __hex__ = __int__

def check_exc(stmt, exception):
    """Raise TestFailed if executing 'stmt' does not raise 'exception'
    """
    try:
        exec stmt
    except exception:
        pass
    else:
        raise TestFailed, "%s should raise %s" % (stmt, exception)

check_exc("int(BadTypeClass())", TypeError)
check_exc("float(BadTypeClass())", TypeError)
check_exc("long(BadTypeClass())", TypeError)
check_exc("str(BadTypeClass())", TypeError)
check_exc("repr(BadTypeClass())", TypeError)
check_exc("oct(BadTypeClass())", TypeError)
check_exc("hex(BadTypeClass())", TypeError)

# mixing up ints and longs is okay
class IntLongMixClass:
    def __int__(self):
        return 0L

    def __long__(self):
        return 0

try:
    int(IntLongMixClass())
except TypeError:
    raise TestFailed, "TypeError should not be raised"

try:
    long(IntLongMixClass())
except TypeError:
    raise TestFailed, "TypeError should not be raised"


# Test correct errors from hash() on objects with comparisons but no __hash__

class C0:
    pass

hash(C0()) # This should work; the next two should raise TypeError

class C1:
    def __cmp__(self, other): return 0

check_exc("hash(C1())", TypeError)

class C2:
    def __eq__(self, other): return 1

check_exc("hash(C2())", TypeError)

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
