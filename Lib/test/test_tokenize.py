from test_support import verify, verbose, findfile
import tokenize, os, sys

if verbose:
    print 'starting...'
file = open(findfile('tokenize_tests.py'))
tokenize.tokenize(file.readline)
if verbose:
    print 'finished'
