from test_support import verbose, TestFailed

if verbose:
    print 'Running tests on argument handling'

try:
    exec('def f(a, a): pass')
    raise TestFailed, "duplicate arguments"
except SyntaxError:
    pass

try:
    exec('def f(a = 0, a = 1): pass')
    raise TestFailed, "duplicate keyword arguments"
except SyntaxError:
    pass

try:
    exec('def f(a): global a; a = 1')
    raise TestFailed, "variable is global and local"
except SyntaxError:
    pass
