from test_support import verbose
import string, sys

# test string formatting operator (I am not sure if this is being tested
# elsewhere but, surely, some of the given cases are *not* tested because
# they crash python)
# test on unicode strings as well

def testformat(formatstr, args, output=None):
	if verbose:
		if output:
			print "%s %% %s =? %s ..." %\
				(repr(formatstr), repr(args), repr(output)),
		else:
			print "%s %% %s works? ..." % (repr(formatstr), repr(args)),
	try:
		result = formatstr % args
	except OverflowError:
		if verbose:
			print 'overflow (this is fine)'
	else:
		if output and result != output:
			if verbose:
				print 'no'
			print "%s %% %s == %s != %s" %\
				(repr(formatstr), repr(args), repr(result), repr(output))
		else:
			if verbose:
				print 'yes'

def testboth(formatstr, *args):
	testformat(formatstr, *args)
	testformat(unicode(formatstr), *args)
		

testboth("%.1d", (1,), "1")
testboth("%.*d", (sys.maxint,1))  # expect overflow
testboth("%.100d", (1,), '0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001')
testboth("%#.117x", (1,), '0x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001')
testboth("%#.118x", (1,), '0x0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001')

testboth("%f", (1.0,), "1.000000")
# these are trying to test the limits of the internal magic-number-length
# formatting buffer, if that number changes then these tests are less
# effective
testboth("%#.*g", (109, -1.e+49/3.))
testboth("%#.*g", (110, -1.e+49/3.))
testboth("%#.*g", (110, -1.e+100/3.))

# test some ridiculously large precision, expect overflow
testboth('%12.*f', (123456, 1.0))

