from test_support import verbose, TestFailed

if verbose:
    print 'Running test on duplicate arguments'

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
