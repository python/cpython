from test.test_support import verbose, findfile, TestFailed
import tokenize, os, sys

if verbose:
    print 'starting...'

f = file(findfile('tokenize_tests' + os.extsep + 'txt'))
tokenize.tokenize(f.readline)
f.close()

if verbose:
    print 'finished'

###### Test detecton of IndentationError ######################

from cStringIO import StringIO

sampleBadText = """
def foo():
    bar
  baz
"""

try:
    for tok in tokenize.generate_tokens(StringIO(sampleBadText).readline):
        pass
except IndentationError:
    pass
else:
    raise TestFailed("Did not detect IndentationError:")
