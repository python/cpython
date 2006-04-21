import os, glob, random
from cStringIO import StringIO
from test.test_support import (verbose, findfile, is_resource_enabled,
                               TestFailed)
from tokenize import (tokenize, generate_tokens, untokenize,
                      NUMBER, NAME, OP, STRING)

# Test roundtrip for `untokenize`.  `f` is a file path.  The source code in f
# is tokenized, converted back to source code via tokenize.untokenize(),
# and tokenized again from the latter.  The test fails if the second
# tokenization doesn't match the first.
def test_roundtrip(f):
    ## print 'Testing:', f
    fobj = open(f)
    try:
        fulltok = list(generate_tokens(fobj.readline))
    finally:
        fobj.close()

    t1 = [tok[:2] for tok in fulltok]
    newtext = untokenize(t1)
    readline = iter(newtext.splitlines(1)).next
    t2 = [tok[:2] for tok in generate_tokens(readline)]
    if t1 != t2:
        raise TestFailed("untokenize() roundtrip failed for %r" % f)

# This is an example from the docs, set up as a doctest.
def decistmt(s):
    """Substitute Decimals for floats in a string of statements.

    >>> from decimal import Decimal
    >>> s = 'print +21.3e-5*-.1234/81.7'
    >>> decistmt(s)
    "print +Decimal ('21.3e-5')*-Decimal ('.1234')/Decimal ('81.7')"

    The format of the exponent is inherited from the platform C library.
    Known cases are "e-007" (Windows) and "e-07" (not Windows).  Since
    we're only showing 12 digits, and the 13th isn't close to 5, the
    rest of the output should be platform-independent.

    >>> exec(s) #doctest: +ELLIPSIS
    -3.21716034272e-0...7

    Output from calculations with Decimal should be identical across all
    platforms.

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

def test_main():
    if verbose:
        print 'starting...'

    # This displays the tokenization of tokenize_tests.py to stdout, and
    # regrtest.py checks that this equals the expected output (in the
    # test/output/ directory).
    f = open(findfile('tokenize_tests' + os.extsep + 'txt'))
    tokenize(f.readline)
    f.close()

    # Now run test_roundtrip() over tokenize_test.py too, and over all
    # (if the "compiler" resource is enabled) or a small random sample (if
    # "compiler" is not enabled) of the test*.py files.
    f = findfile('tokenize_tests' + os.extsep + 'txt')
    test_roundtrip(f)

    testdir = os.path.dirname(f) or os.curdir
    testfiles = glob.glob(testdir + os.sep + 'test*.py')
    if not is_resource_enabled('compiler'):
        testfiles = random.sample(testfiles, 10)

    for f in testfiles:
        test_roundtrip(f)

    # Test detecton of IndentationError.
    sampleBadText = """\
def foo():
    bar
  baz
"""

    try:
        for tok in generate_tokens(StringIO(sampleBadText).readline):
            pass
    except IndentationError:
        pass
    else:
        raise TestFailed("Did not detect IndentationError:")

    # Run the doctests in this module.
    from test import test_tokenize  # i.e., this module
    from test.test_support import run_doctest
    run_doctest(test_tokenize)

    if verbose:
        print 'finished'

if __name__ == "__main__":
    test_main()
