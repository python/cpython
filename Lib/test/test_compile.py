from test_support import verbose, TestFailed

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
