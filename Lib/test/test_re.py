#!/usr/local/bin/python
# -*- mode: python -*-
# $Id$

from test_support import verbose, TestFailed
import re
import reop
import sys, os, string, traceback

# Misc tests from Tim Peters' re.doc

if verbose:
    print 'Running tests on re.sub'

try:
    assert re.sub("(?i)b+", "x", "bbbb BBBB") == 'x x'
    
    def bump_num(matchobj):
	int_value = int(matchobj.group(0))
	return str(int_value + 1)

    assert re.sub(r'\d+', bump_num, '08.2 -2 23x99y') == '9.3 -3 24x100y'
    
    assert re.sub('.', lambda m: r"\n", 'x') == '\\n'
    assert re.sub('.', r"\n", 'x') == '\n'
    
    s = r"\1\1"
    assert re.sub('(.)', s, 'x') == 'xx'
    assert re.sub('(.)', re.escape(s), 'x') == s
    assert re.sub('(.)', lambda m: s, 'x') == s

    assert re.sub('(?P<a>x)', '\g<a>\g<a>', 'xx') == 'xxxx'

    assert re.sub('a', r'\t\n\v\r\f\a\b\B\Z\a\A\w\W\s\S\d\D', 'a') == '\t\n\v\r\f\a\bBZ\aAwWsSdD'
    assert re.sub('a', '\t\n\v\r\f\a', 'a') == '\t\n\v\r\f\a'
    assert re.sub('a', '\t\n\v\r\f\a', 'a') == (chr(9)+chr(10)+chr(11)+chr(13)+chr(12)+chr(7))

except AssertionError:
    raise TestFailed, "re.sub"

if verbose:
    print 'Running tests on symbolic references'

try:
    re.sub('(?P<a>x)', '\g<a', 'xx')
except re.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    re.sub('(?P<a>x)', '\g<', 'xx')
except re.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    re.sub('(?P<a>x)', '\g', 'xx')
except re.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    re.sub('(?P<a>x)', '\g<a a>', 'xx')
except re.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    re.sub('(?P<a>x)', '\g<ab>', 'xx')
except IndexError, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    re.sub('(?P<a>x)|(?P<b>y)', '\g<b>', 'xx')
except re.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

try:
    re.sub('(?P<a>x)|(?P<b>y)', '\\2', 'xx')
except re.error, reason:
    pass
else:
    raise TestFailed, "symbolic reference"

if verbose:
    print 'Running tests on re.subn'

try:
    assert re.subn("(?i)b+", "x", "bbbb BBBB") == ('x x', 2)
    assert re.subn("b+", "x", "bbbb BBBB") == ('x BBBB', 1)
    assert re.subn("b+", "x", "xyz") == ('xyz', 0)
    assert re.subn("b*", "x", "xyz") == ('xxxyxzx', 4)
    
except AssertionError:
    raise TestFailed, "re.subn"

if verbose:
    print 'Running tests on re.split'

try:
    assert re.split(":", ":a:b::c") == ['', 'a', 'b', '', 'c']
    assert re.split(":*", ":a:b::c") == ['', 'a', 'b', 'c']
    assert re.split("(:*)", ":a:b::c") == ['', ':', 'a', ':', 'b', '::', 'c']
    assert re.split("(?::*)", ":a:b::c") == ['', 'a', 'b', 'c']
    assert re.split("(:)*", ":a:b::c") == ['', ':', 'a', ':', 'b', ':', 'c']
    assert re.split("([b:]+)", ":a:b::c") == ['', ':', 'a', ':b::', 'c']
    assert re.split("(b)|(:+)", ":a:b::c") == \
           ['', None, ':', 'a', None, ':', '', 'b', None, '', None, '::', 'c']
    assert re.split("(?:b)|(?::+)", ":a:b::c") == ['', 'a', '', '', 'c']
    
except AssertionError:
    raise TestFailed, "re.split"

from re_tests import *
if verbose:
    print 'Running re_tests test suite'

for t in tests:
    sys.stdout.flush()
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
    except KeyboardInterrupt: raise KeyboardInterrupt
    except:
	print '*** Unexpected error ***'
	if verbose:
	    traceback.print_exc(file=sys.stdout)
    else:
	try:
	    result=obj.search(s)
	except (re.error, reop.error), msg:
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
		vardict={'found': result.group(0),
			 'groups': result.group(),
			 'flags': result.re.flags}
		for i in range(1, 100):
		    try:
			gi = result.group(i)
			# Special hack because else the string concat fails:
			if gi is None:
			    gi = "None"
		    except IndexError:
			gi = "Error"
		    vardict['g%d' % i] = gi
		for i in result.re.groupindex.keys():
		    try:
			gi = result.group(i)
			if gi is None:
			    gi = "None"
		    except IndexError:
			gi = "Error"
		    vardict[i] = gi
		repl=eval(repl, vardict)
		if repl!=expected:
		    print '=== grouping error', t,
		    print repr(repl)+' should be '+repr(expected)
	    else:
		print '=== Failed incorrectly', t

            # Try the match with IGNORECASE enabled, and check that it
	    # still succeeds.
            obj=re.compile(pattern, re.IGNORECASE)
            result=obj.search(s)
            if result==None:
                print '=== Fails on case-insensitive match', t
