from test_support import verbose
import re
import sys, os, string, traceback

from re_tests import *
if verbose: print 'Running re_tests test suite'

for t in tests:
    print t
    pattern=s=outcome=repl=expected=None
    if len(t)==5:
	pattern, s, outcome, repl, expected = t
    elif len(t)==3:
	pattern, s, outcome = t 
    else:
	raise ValueError, ('Test tuples should have 3 or 5 fields',t)

    try:
	obj=re.compile(pattern)
    except re.error:
	if outcome==SYNTAX_ERROR: pass	# Expected a syntax error
	else: 
	    print '=== Syntax error:', t
    except:
	print '*** Unexpected error ***'
	if verbose:
	    traceback.print_exc(file=sys.stdout)
    else:
	try:
	    result=obj.search(s)
	except regex.error, msg:
	    print '=== Unexpected exception', t, repr(msg)
	if outcome==SYNTAX_ERROR:
	    # This should have been a syntax error; forget it.
	    pass
	elif outcome==FAIL:
	    if result is None: pass   # No match, as expected
	    else: print '=== Succeeded incorrectly', t
	elif outcome==SUCCEED:
	    if result is not None:
		# Matched, as expected, so now we compute the
		# result string and compare it to our expected result.
		start, end = result.span(0)
		vardict={'found': result.group(0), 'groups': result.group()}
		for i in range(1, 100):
		    try:
			gi = result.group(i)
			# Special hack because else the string concat fails:
			if gi is None: gi = "None"
		    except IndexError:
			gi = "Error"
		    vardict['g%d' % i] = gi
		for i in result.re.groupindex.keys():
		    try:
			gi = result.group(i)
		    except IndexError:
			pass
		    else:
			vardict[i] = str(gi)
		repl=eval(repl, vardict)
		if repl!=expected:
		    print '=== grouping error', t,
		    print repr(repl)+' should be '+repr(expected)
	    else:
		print '=== Failed incorrectly', t
