from test.test_support import verbose, findfile
import tokenize, os, sys

if verbose:
    print 'starting...'

f = file(findfile('tokenize_tests' + os.extsep + 'txt'))
tokenize.tokenize(f.readline)
f.close()

if verbose:
    print 'finished'
