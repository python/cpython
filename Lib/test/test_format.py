from test_support import verbose
import string, sys

# test string formatting operator (I am not sure if this is being tested
# elsewhere but, surely, some of the given cases are *not* tested because
# they crash python)
# test on unicode strings as well

overflowok = 1

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
        if not overflowok:
            raise
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

# Formatting of long integers. Overflow is not ok
overflowok = 0
testboth("%x", 10L, "a")
testboth("%x", 100000000000L, "174876e800")
testboth("%o", 10L, "12")
testboth("%o", 100000000000L, "1351035564000")
testboth("%d", 10L, "10")
testboth("%d", 100000000000L, "100000000000")

# Make sure big is too big to fit in a 64-bit int, else the unbounded
# int formatting will be sidestepped on some machines.  That's vital,
# because bitwise (x, X, o) formats of regular Python ints never
# produce a sign ("+" or "-").

big = 123456789012345678901234567890L
testboth("%d", big, "123456789012345678901234567890")
testboth("%d", -big, "-123456789012345678901234567890")
testboth("%5d", -big, "-123456789012345678901234567890")
testboth("%31d", -big, "-123456789012345678901234567890")
testboth("%32d", -big, " -123456789012345678901234567890")
testboth("%-32d", -big, "-123456789012345678901234567890 ")
testboth("%032d", -big, "-0123456789012345678901234567890")
testboth("%-032d", -big, "-123456789012345678901234567890 ")
testboth("%034d", -big, "-000123456789012345678901234567890")
testboth("%034d", big, "0000123456789012345678901234567890")
testboth("%0+34d", big, "+000123456789012345678901234567890")
testboth("%+34d", big, "   +123456789012345678901234567890")
testboth("%34d", big, "    123456789012345678901234567890")
testboth("%.2d", big, "123456789012345678901234567890")
testboth("%.30d", big, "123456789012345678901234567890")
testboth("%.31d", big, "0123456789012345678901234567890")
testboth("%32.31d", big, " 0123456789012345678901234567890")

big = 0x1234567890abcdef12345L  # 21 hex digits
testboth("%x", big, "1234567890abcdef12345")
testboth("%x", -big, "-1234567890abcdef12345")
testboth("%5x", -big, "-1234567890abcdef12345")
testboth("%22x", -big, "-1234567890abcdef12345")
testboth("%23x", -big, " -1234567890abcdef12345")
testboth("%-23x", -big, "-1234567890abcdef12345 ")
testboth("%023x", -big, "-01234567890abcdef12345")
testboth("%-023x", -big, "-1234567890abcdef12345 ")
testboth("%025x", -big, "-0001234567890abcdef12345")
testboth("%025x", big, "00001234567890abcdef12345")
testboth("%0+25x", big, "+0001234567890abcdef12345")
testboth("%+25x", big, "   +1234567890abcdef12345")
testboth("%25x", big, "    1234567890abcdef12345")
testboth("%.2x", big, "1234567890abcdef12345")
testboth("%.21x", big, "1234567890abcdef12345")
testboth("%.22x", big, "01234567890abcdef12345")
testboth("%23.22x", big, " 01234567890abcdef12345")
testboth("%-23.22x", big, "01234567890abcdef12345 ")
testboth("%X", big, "1234567890ABCDEF12345")
testboth("%#X", big, "0X1234567890ABCDEF12345")
testboth("%#x", big, "0x1234567890abcdef12345")
testboth("%#x", -big, "-0x1234567890abcdef12345")
testboth("%#.23x", -big, "-0x001234567890abcdef12345")
testboth("%#+.23x", big, "+0x001234567890abcdef12345")
testboth("%# .23x", big, " 0x001234567890abcdef12345")
testboth("%#+.23X", big, "+0X001234567890ABCDEF12345")
testboth("%#-+.23X", big, "+0X001234567890ABCDEF12345")
testboth("%#-+26.23X", big, "+0X001234567890ABCDEF12345")
testboth("%#-+27.23X", big, "+0X001234567890ABCDEF12345 ")
testboth("%#+27.23X", big, " +0X001234567890ABCDEF12345")
# next one gets two leading zeroes from precision, and another from the
# 0 flag and the width
testboth("%#+027.23X", big, "+0X0001234567890ABCDEF12345")
# same, except no 0 flag
testboth("%#+27.23X", big, " +0X001234567890ABCDEF12345")

big = 012345670123456701234567012345670L  # 32 octal digits
testboth("%o", big, "12345670123456701234567012345670")
testboth("%o", -big, "-12345670123456701234567012345670")
testboth("%5o", -big, "-12345670123456701234567012345670")
testboth("%33o", -big, "-12345670123456701234567012345670")
testboth("%34o", -big, " -12345670123456701234567012345670")
testboth("%-34o", -big, "-12345670123456701234567012345670 ")
testboth("%034o", -big, "-012345670123456701234567012345670")
testboth("%-034o", -big, "-12345670123456701234567012345670 ")
testboth("%036o", -big, "-00012345670123456701234567012345670")
testboth("%036o", big, "000012345670123456701234567012345670")
testboth("%0+36o", big, "+00012345670123456701234567012345670")
testboth("%+36o", big, "   +12345670123456701234567012345670")
testboth("%36o", big, "    12345670123456701234567012345670")
testboth("%.2o", big, "12345670123456701234567012345670")
testboth("%.32o", big, "12345670123456701234567012345670")
testboth("%.33o", big, "012345670123456701234567012345670")
testboth("%34.33o", big, " 012345670123456701234567012345670")
testboth("%-34.33o", big, "012345670123456701234567012345670 ")
testboth("%o", big, "12345670123456701234567012345670")
testboth("%#o", big, "012345670123456701234567012345670")
testboth("%#o", -big, "-012345670123456701234567012345670")
testboth("%#.34o", -big, "-0012345670123456701234567012345670")
testboth("%#+.34o", big, "+0012345670123456701234567012345670")
testboth("%# .34o", big, " 0012345670123456701234567012345670")
testboth("%#+.34o", big, "+0012345670123456701234567012345670")
testboth("%#-+.34o", big, "+0012345670123456701234567012345670")
testboth("%#-+37.34o", big, "+0012345670123456701234567012345670  ")
testboth("%#+37.34o", big, "  +0012345670123456701234567012345670")
# next one gets one leading zero from precision
testboth("%.33o", big, "012345670123456701234567012345670")
# base marker shouldn't change that, since "0" is redundant
testboth("%#.33o", big, "012345670123456701234567012345670")
# but reduce precision, and base marker should add a zero
testboth("%#.32o", big, "012345670123456701234567012345670")
# one leading zero from precision, and another from "0" flag & width
testboth("%034.33o", big, "0012345670123456701234567012345670")
# base marker shouldn't change that
testboth("%0#34.33o", big, "0012345670123456701234567012345670")
