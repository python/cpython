"Test the functionality of Python classes implementing operators."


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
    "del",
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

for method in testmeths:
    exec("""def __%(method)s__(self, *args):
                print "__%(method)s__:", args
"""%locals(), AllTests.__dict__);

# this also tests __init__ of course.
testme = AllTests()

# Binary operations

testme + 1
1 + testme

testme - 1
1 - testme

testme * 1
1 * testme

testme / 1
1 / testme

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

testme[:42]
testme[:42] = "The Answer"
del testme[:42]


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

