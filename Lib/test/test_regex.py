from test_support import verbose, sortdict
import warnings
warnings.filterwarnings("ignore", "the regex module is deprecated",
                        DeprecationWarning, __name__)
import regex
from regex_syntax import *

re = 'a+b+c+'
print 'no match:', regex.match(re, 'hello aaaabcccc world')
print 'successful search:', regex.search(re, 'hello aaaabcccc world')
try:
    cre = regex.compile('\(' + re)
except regex.error:
    print 'caught expected exception'
else:
    print 'expected regex.error not raised'

print 'failed awk syntax:', regex.search('(a+)|(b+)', 'cdb')
prev = regex.set_syntax(RE_SYNTAX_AWK)
print 'successful awk syntax:', regex.search('(a+)|(b+)', 'cdb')
regex.set_syntax(prev)
print 'failed awk syntax:', regex.search('(a+)|(b+)', 'cdb')

re = '\(<one>[0-9]+\) *\(<two>[0-9]+\)'
print 'matching with group names and compile()'
cre = regex.compile(re)
print cre.match('801 999')
try:
    print cre.group('one')
except regex.error:
    print 'caught expected exception'
else:
    print 'expected regex.error not raised'

print 'matching with group names and symcomp()'
cre = regex.symcomp(re)
print cre.match('801 999')
print cre.group(0)
print cre.group('one')
print cre.group(1, 2)
print cre.group('one', 'two')
print 'realpat:', cre.realpat
print 'groupindex:', sortdict(cre.groupindex)

re = 'world'
cre = regex.compile(re)
print 'not case folded search:', cre.search('HELLO WORLD')
cre = regex.compile(re, regex.casefold)
print 'case folded search:', cre.search('HELLO WORLD')

print '__members__:', cre.__members__
print 'regs:', cre.regs
print 'last:', cre.last
print 'translate:', len(cre.translate)
print 'givenpat:', cre.givenpat

print 'match with pos:', cre.match('hello world', 7)
print 'search with pos:', cre.search('hello world there world', 7)
print 'bogus group:', cre.group(0, 1, 3)
try:
    print 'no name:', cre.group('one')
except regex.error:
    print 'caught expected exception'
else:
    print 'expected regex.error not raised'

from regex_tests import *
if verbose: print 'Running regex_tests test suite'

for t in tests:
    pattern=s=outcome=repl=expected=None
    if len(t)==5:
        pattern, s, outcome, repl, expected = t
    elif len(t)==3:
        pattern, s, outcome = t
    else:
        raise ValueError, ('Test tuples should have 3 or 5 fields',t)

    try:
        obj=regex.compile(pattern)
    except regex.error:
        if outcome==SYNTAX_ERROR: pass    # Expected a syntax error
        else:
            # Regex syntax errors aren't yet reported, so for
            # the official test suite they'll be quietly ignored.
            pass
            #print '=== Syntax error:', t
    else:
        try:
            result=obj.search(s)
        except regex.error, msg:
            print '=== Unexpected exception', t, repr(msg)
        if outcome==SYNTAX_ERROR:
            # This should have been a syntax error; forget it.
            pass
        elif outcome==FAIL:
            if result==-1: pass   # No match, as expected
            else: print '=== Succeeded incorrectly', t
        elif outcome==SUCCEED:
            if result!=-1:
                # Matched, as expected, so now we compute the
                # result string and compare it to our expected result.
                start, end = obj.regs[0]
                found=s[start:end]
                groups=obj.group(1,2,3,4,5,6,7,8,9,10)
                vardict=vars()
                for i in range(len(groups)):
                    vardict['g'+str(i+1)]=str(groups[i])
                repl=eval(repl)
                if repl!=expected:
                    print '=== grouping error', t, repr(repl)+' should be '+repr(expected)
            else:
                print '=== Failed incorrectly', t
