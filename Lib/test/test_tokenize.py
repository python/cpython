from test.test_support import verbose, findfile, is_resource_enabled
import os, glob, random
from tokenize import (tokenize, generate_tokens, untokenize,
                      NUMBER, NAME, OP, STRING)

if verbose:
    print 'starting...'

f = file(findfile('tokenize_tests' + os.extsep + 'txt'))
tokenize(f.readline)
f.close()



###### Test roundtrip for untokenize ##########################

def test_roundtrip(f):
    ## print 'Testing:', f
    f = file(f)
    try:
        fulltok = list(generate_tokens(f.readline))
    finally:
        f.close()

    t1 = [tok[:2] for tok in fulltok]
    newtext = untokenize(t1)
    readline = iter(newtext.splitlines(1)).next
    t2 = [tok[:2] for tok in generate_tokens(readline)]
    assert t1 == t2


f = findfile('tokenize_tests' + os.extsep + 'txt')
test_roundtrip(f)

testdir = os.path.dirname(f) or os.curdir
testfiles = glob.glob(testdir + os.sep + 'test*.py')
if not is_resource_enabled('compiler'):
    testfiles = random.sample(testfiles, 10)

for f in testfiles:
    test_roundtrip(f)



###### Test example in the docs ###############################

from decimal import Decimal
from cStringIO import StringIO

def decistmt(s):
    """Substitute Decimals for floats in a string of statements.

    >>> from decimal import Decimal
    >>> s = 'print +21.3e-5*-.1234/81.7'
    >>> decistmt(s)
    "print +Decimal ('21.3e-5')*-Decimal ('.1234')/Decimal ('81.7')"

    >>> exec(s)
    -3.21716034272e-007
    >>> exec(decistmt(s))
    -3.217160342717258261933904529E-7

    """
    result = []
    g = generate_tokens(StringIO(s).readline)   # tokenize the string
    for toknum, tokval, _, _, _  in g:
        if toknum == NUMBER and '.' in tokval:  # replace NUMBER tokens
            result.extend([
                (NAME, 'Decimal'),
                (OP, '('),
                (STRING, repr(tokval)),
                (OP, ')')
            ])
        else:
            result.append((toknum, tokval))
    return untokenize(result)

import doctest
doctest.testmod()

if verbose:
    print 'finished'
