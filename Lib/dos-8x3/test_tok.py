from test_support import verbose
import tokenize, os, sys

def findfile(file):
	if os.path.isabs(file): return file
	path = sys.path
	try:
	    path = [os.path.dirname(__file__)] + path
	except NameError:
	    pass
	for dn in path:
		fn = os.path.join(dn, file)
		if os.path.exists(fn): return fn
	return file

if verbose:
    print 'starting...'
file = open(findfile('tokenize_tests.py'))
tokenize.tokenize(file.readline)
if verbose:
    print 'finished'

