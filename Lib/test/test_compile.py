from test_support import verbose, TestFailed

if verbose:
    print "Testing whether compiler catches assignment to __debug__"

try:
    compile('__debug__ = 1', '?', 'single')
except SyntaxError:
    pass

import __builtin__
prev = __builtin__.__debug__
setattr(__builtin__, '__debug__', 'sure')
setattr(__builtin__, '__debug__', prev)

if verbose:
    print 'Running tests on argument handling'

try:
    exec 'def f(a, a): pass'
    raise TestFailed, "duplicate arguments"
except SyntaxError:
    pass

try:
    exec 'def f(a = 0, a = 1): pass'
    raise TestFailed, "duplicate keyword arguments"
except SyntaxError:
    pass

try:
    exec 'def f(a): global a; a = 1'
    raise TestFailed, "variable is global and local"
except SyntaxError:
    pass

if verbose:
    print "testing complex args"

def comp_args((a, b)):
    print a,b

comp_args((1, 2))

def comp_args((a, b)=(3, 4)):
    print a, b

comp_args((1, 2))
comp_args()

def comp_args(a, (b, c)):
    print a, b, c

comp_args(1, (2, 3))

def comp_args(a=2, (b, c)=(3, 4)):
    print a, b, c

comp_args(1, (2, 3))
comp_args()

try:
    exec 'def f(a=1, (b, c)): pass'
    raise TestFailed, "non-default args after default"
except SyntaxError:
    pass

if verbose:
    print "testing bad float literals"

def expect_error(s):
    try:
        eval(s)
        raise TestFailed("%r accepted" % s)
    except SyntaxError:
        pass

expect_error("2e")
expect_error("2.0e+")
expect_error("1e-")
expect_error("3-4e/21")


if verbose:
    print "testing literals with leading zeroes"

def expect_same(test_source, expected):
    got = eval(test_source)
    if got != expected:
        raise TestFailed("eval(%r) gave %r, but expected %r" %
                         (test_source, got, expected))

expect_error("077787")
expect_error("0xj")
expect_error("0x.")
expect_error("0e")
expect_same("0777", 511)
expect_same("0777L", 511)
expect_same("000777", 511)
expect_same("0xff", 255)
expect_same("0xffL", 255)
expect_same("0XfF", 255)
expect_same("0777.", 777)
expect_same("0777.0", 777)
expect_same("000000000000000000000000000000000000000000000000000777e0", 777)
expect_same("0777e1", 7770)
expect_same("0e0", 0)
expect_same("0000E-012", 0)
expect_same("09.5", 9.5)
expect_same("0777j", 777j)
expect_same("00j", 0j)
expect_same("00.0", 0)
expect_same("0e3", 0)
expect_same("090000000000000.", 90000000000000.)
expect_same("090000000000000.0000000000000000000000", 90000000000000.)
expect_same("090000000000000e0", 90000000000000.)
expect_same("090000000000000e-0", 90000000000000.)
expect_same("090000000000000j", 90000000000000j)
expect_error("090000000000000")  # plain octal literal w/ decimal digit
expect_error("080000000000000")  # plain octal literal w/ decimal digit
expect_error("000000000000009")  # plain octal literal w/ decimal digit
expect_error("000000000000008")  # plain octal literal w/ decimal digit
expect_same("000000000000007", 7)
expect_same("000000000000008.", 8.)
expect_same("000000000000009.", 9.)

# Verify treatment of unary minus on negative numbers SF bug #660455
expect_same("0xffffffff", -1)
expect_same("-0xffffffff", 1)
