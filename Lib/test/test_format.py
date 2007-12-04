from test.test_support import verbose, TestFailed
import sys
from test.test_support import MAX_Py_ssize_t
maxsize = MAX_Py_ssize_t

# test string formatting operator (I am not sure if this is being tested
# elsewhere but, surely, some of the given cases are *not* tested because
# they crash python)
# test on unicode strings as well

overflowok = 1
overflowrequired = 0

def testformat(formatstr, args, output=None):
    if verbose:
        if output:
            print("%r %% %r =? %r ..." %\
                (formatstr, args, output), end=' ')
        else:
            print("%r %% %r works? ..." % (formatstr, args), end=' ')
    try:
        result = formatstr % args
    except OverflowError:
        if not overflowok:
            raise
        if verbose:
            print('overflow (this is fine)')
    else:
        if overflowrequired:
            if verbose:
                print('no')
            print("overflow expected on %r %% %r" % (formatstr, args))
        elif output and result != output:
            if verbose:
                print('no')
            print("%r %% %r == %r != %r" %\
                (formatstr, args, result, output))
        else:
            if verbose:
                print('yes')

testformat("%.1d", (1,), "1")
testformat("%.*d", (sys.maxsize,1))  # expect overflow
testformat("%.100d", (1,), '0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001')
testformat("%#.117x", (1,), '0x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001')
testformat("%#.118x", (1,), '0x0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001')

testformat("%f", (1.0,), "1.000000")
# these are trying to test the limits of the internal magic-number-length
# formatting buffer, if that number changes then these tests are less
# effective
testformat("%#.*g", (109, -1.e+49/3.))
testformat("%#.*g", (110, -1.e+49/3.))
testformat("%#.*g", (110, -1.e+100/3.))

# test some ridiculously large precision, expect overflow
testformat('%12.*f', (123456, 1.0))

# check for internal overflow validation on length of precision
overflowrequired = 1
testformat("%#.*g", (110, -1.e+100/3.))
testformat("%#.*G", (110, -1.e+100/3.))
testformat("%#.*f", (110, -1.e+100/3.))
testformat("%#.*F", (110, -1.e+100/3.))
overflowrequired = 0

# Formatting of long integers. Overflow is not ok
overflowok = 0
testformat("%x", 10, "a")
testformat("%x", 100000000000, "174876e800")
testformat("%o", 10, "12")
testformat("%o", 100000000000, "1351035564000")
testformat("%d", 10, "10")
testformat("%d", 100000000000, "100000000000")

big = 123456789012345678901234567890
testformat("%d", big, "123456789012345678901234567890")
testformat("%d", -big, "-123456789012345678901234567890")
testformat("%5d", -big, "-123456789012345678901234567890")
testformat("%31d", -big, "-123456789012345678901234567890")
testformat("%32d", -big, " -123456789012345678901234567890")
testformat("%-32d", -big, "-123456789012345678901234567890 ")
testformat("%032d", -big, "-0123456789012345678901234567890")
testformat("%-032d", -big, "-123456789012345678901234567890 ")
testformat("%034d", -big, "-000123456789012345678901234567890")
testformat("%034d", big, "0000123456789012345678901234567890")
testformat("%0+34d", big, "+000123456789012345678901234567890")
testformat("%+34d", big, "   +123456789012345678901234567890")
testformat("%34d", big, "    123456789012345678901234567890")
testformat("%.2d", big, "123456789012345678901234567890")
testformat("%.30d", big, "123456789012345678901234567890")
testformat("%.31d", big, "0123456789012345678901234567890")
testformat("%32.31d", big, " 0123456789012345678901234567890")

big = 0x1234567890abcdef12345  # 21 hex digits
testformat("%x", big, "1234567890abcdef12345")
testformat("%x", -big, "-1234567890abcdef12345")
testformat("%5x", -big, "-1234567890abcdef12345")
testformat("%22x", -big, "-1234567890abcdef12345")
testformat("%23x", -big, " -1234567890abcdef12345")
testformat("%-23x", -big, "-1234567890abcdef12345 ")
testformat("%023x", -big, "-01234567890abcdef12345")
testformat("%-023x", -big, "-1234567890abcdef12345 ")
testformat("%025x", -big, "-0001234567890abcdef12345")
testformat("%025x", big, "00001234567890abcdef12345")
testformat("%0+25x", big, "+0001234567890abcdef12345")
testformat("%+25x", big, "   +1234567890abcdef12345")
testformat("%25x", big, "    1234567890abcdef12345")
testformat("%.2x", big, "1234567890abcdef12345")
testformat("%.21x", big, "1234567890abcdef12345")
testformat("%.22x", big, "01234567890abcdef12345")
testformat("%23.22x", big, " 01234567890abcdef12345")
testformat("%-23.22x", big, "01234567890abcdef12345 ")
testformat("%X", big, "1234567890ABCDEF12345")
testformat("%#X", big, "0X1234567890ABCDEF12345")
testformat("%#x", big, "0x1234567890abcdef12345")
testformat("%#x", -big, "-0x1234567890abcdef12345")
testformat("%#.23x", -big, "-0x001234567890abcdef12345")
testformat("%#+.23x", big, "+0x001234567890abcdef12345")
testformat("%# .23x", big, " 0x001234567890abcdef12345")
testformat("%#+.23X", big, "+0X001234567890ABCDEF12345")
testformat("%#-+.23X", big, "+0X001234567890ABCDEF12345")
testformat("%#-+26.23X", big, "+0X001234567890ABCDEF12345")
testformat("%#-+27.23X", big, "+0X001234567890ABCDEF12345 ")
testformat("%#+27.23X", big, " +0X001234567890ABCDEF12345")
# next one gets two leading zeroes from precision, and another from the
# 0 flag and the width
testformat("%#+027.23X", big, "+0X0001234567890ABCDEF12345")
# same, except no 0 flag
testformat("%#+27.23X", big, " +0X001234567890ABCDEF12345")

big = 0o12345670123456701234567012345670  # 32 octal digits
testformat("%o", big, "12345670123456701234567012345670")
testformat("%o", -big, "-12345670123456701234567012345670")
testformat("%5o", -big, "-12345670123456701234567012345670")
testformat("%33o", -big, "-12345670123456701234567012345670")
testformat("%34o", -big, " -12345670123456701234567012345670")
testformat("%-34o", -big, "-12345670123456701234567012345670 ")
testformat("%034o", -big, "-012345670123456701234567012345670")
testformat("%-034o", -big, "-12345670123456701234567012345670 ")
testformat("%036o", -big, "-00012345670123456701234567012345670")
testformat("%036o", big, "000012345670123456701234567012345670")
testformat("%0+36o", big, "+00012345670123456701234567012345670")
testformat("%+36o", big, "   +12345670123456701234567012345670")
testformat("%36o", big, "    12345670123456701234567012345670")
testformat("%.2o", big, "12345670123456701234567012345670")
testformat("%.32o", big, "12345670123456701234567012345670")
testformat("%.33o", big, "012345670123456701234567012345670")
testformat("%34.33o", big, " 012345670123456701234567012345670")
testformat("%-34.33o", big, "012345670123456701234567012345670 ")
testformat("%o", big, "12345670123456701234567012345670")
testformat("%#o", big, "0o12345670123456701234567012345670")
testformat("%#o", -big, "-0o12345670123456701234567012345670")
testformat("%#.34o", -big, "-0o0012345670123456701234567012345670")
testformat("%#+.34o", big, "+0o0012345670123456701234567012345670")
testformat("%# .34o", big, " 0o0012345670123456701234567012345670")
testformat("%#-+.34o", big, "+0o0012345670123456701234567012345670")
testformat("%#-+39.34o", big, "+0o0012345670123456701234567012345670  ")
testformat("%#+39.34o", big, "  +0o0012345670123456701234567012345670")
# next one gets one leading zero from precision
testformat("%.33o", big, "012345670123456701234567012345670")
# one leading zero from precision
testformat("%#.33o", big, "0o012345670123456701234567012345670")
# leading zero vanishes
testformat("%#.32o", big, "0o12345670123456701234567012345670")
# one leading zero from precision, and another from '0' flag & width
testformat("%034.33o", big, "0012345670123456701234567012345670")
# max width includes base marker; padding zeroes come after marker
testformat("%0#38.33o", big, "0o000012345670123456701234567012345670")
# padding spaces come before marker
testformat("%#36.33o", big, " 0o012345670123456701234567012345670")

# Some small ints, in both Python int and long flavors).
testformat("%d", 42, "42")
testformat("%d", -42, "-42")
testformat("%#x", 1, "0x1")
testformat("%#X", 1, "0X1")
testformat("%#o", 1, "0o1")
testformat("%#o", 1, "0o1")
testformat("%#o", 0, "0o0")
testformat("%#o", 0, "0o0")
testformat("%o", 0, "0")
testformat("%d", 0, "0")
testformat("%#x", 0, "0x0")
testformat("%#X", 0, "0X0")

testformat("%x", 0x42, "42")
testformat("%x", -0x42, "-42")

testformat("%o", 0o42, "42")
testformat("%o", -0o42, "-42")
testformat("%o", 0o42, "42")
testformat("%o", -0o42, "-42")

# Test exception for unknown format characters
if verbose:
    print('Testing exceptions')

def test_exc(formatstr, args, exception, excmsg):
    try:
        testformat(formatstr, args)
    except exception as exc:
        if str(exc) == excmsg:
            if verbose:
                print("yes")
        else:
            if verbose: print('no')
            print('Unexpected ', exception, ':', repr(str(exc)))
    except:
        if verbose: print('no')
        print('Unexpected exception')
        raise
    else:
        raise TestFailed('did not get expected exception: %s' % excmsg)

test_exc('abc %a', 1, ValueError,
         "unsupported format character 'a' (0x61) at index 5")
test_exc(str(b'abc %\u3000', 'raw-unicode-escape'), 1, ValueError,
         "unsupported format character '?' (0x3000) at index 5")

test_exc('%d', '1', TypeError, "an integer is required")
test_exc('%g', '1', TypeError, "a float is required")
test_exc('no format', '1', TypeError,
         "not all arguments converted during string formatting")
test_exc('no format', '1', TypeError,
         "not all arguments converted during string formatting")
test_exc('no format', '1', TypeError,
         "not all arguments converted during string formatting")
test_exc('no format', '1', TypeError,
         "not all arguments converted during string formatting")

if maxsize == 2**31-1:
    # crashes 2.2.1 and earlier:
    try:
        "%*d"%(maxsize, -127)
    except MemoryError:
        pass
    else:
        raise TestFailed('"%*d"%(maxsize, -127) should fail')
