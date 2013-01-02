import sys
from test.test_support import verbose, have_unicode, TestFailed
import test.test_support as test_support
import unittest

maxsize = test_support.MAX_Py_ssize_t

# test string formatting operator (I am not sure if this is being tested
# elsewhere but, surely, some of the given cases are *not* tested because
# they crash python)
# test on unicode strings as well

def testformat(formatstr, args, output=None, limit=None, overflowok=False):
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
        if output and limit is None and result != output:
            if verbose:
                print 'no'
            raise AssertionError("%r %% %r == %r != %r" %
                                (formatstr, args, result, output))
        # when 'limit' is specified, it determines how many characters
        # must match exactly; lengths must always match.
        # ex: limit=5, '12345678' matches '12345___'
        # (mainly for floating point format tests for which an exact match
        # can't be guaranteed due to rounding and representation errors)
        elif output and limit is not None and (
                len(result)!=len(output) or result[:limit]!=output[:limit]):
            if verbose:
                print 'no'
            print "%s %% %s == %s != %s" % \
                  (repr(formatstr), repr(args), repr(result), repr(output))
        else:
            if verbose:
                print 'yes'


def testboth(formatstr, *args, **kwargs):
    testformat(formatstr, *args, **kwargs)
    if have_unicode:
        testformat(unicode(formatstr), *args, **kwargs)


class FormatTest(unittest.TestCase):
    def test_format(self):
        testboth("%.1d", (1,), "1")
        testboth("%.*d", (sys.maxint,1), overflowok=True)  # expect overflow
        testboth("%.100d", (1,), '00000000000000000000000000000000000000'
                 '000000000000000000000000000000000000000000000000000000'
                 '00000001', overflowok=True)
        testboth("%#.117x", (1,), '0x00000000000000000000000000000000000'
                 '000000000000000000000000000000000000000000000000000000'
                 '0000000000000000000000000001',
                 overflowok=True)
        testboth("%#.118x", (1,), '0x00000000000000000000000000000000000'
                 '000000000000000000000000000000000000000000000000000000'
                 '00000000000000000000000000001',
                 overflowok=True)

        testboth("%f", (1.0,), "1.000000")
        # these are trying to test the limits of the internal magic-number-length
        # formatting buffer, if that number changes then these tests are less
        # effective
        testboth("%#.*g", (109, -1.e+49/3.))
        testboth("%#.*g", (110, -1.e+49/3.))
        testboth("%#.*g", (110, -1.e+100/3.))

        # test some ridiculously large precision, expect overflow
        testboth('%12.*f', (123456, 1.0))

        # check for internal overflow validation on length of precision
        # these tests should no longer cause overflow in Python
        # 2.7/3.1 and later.
        testboth("%#.*g", (110, -1.e+100/3.))
        testboth("%#.*G", (110, -1.e+100/3.))
        testboth("%#.*f", (110, -1.e+100/3.))
        testboth("%#.*F", (110, -1.e+100/3.))

        # Formatting of long integers. Overflow is not ok
        testboth("%x", 10L, "a")
        testboth("%x", 100000000000L, "174876e800")
        testboth("%o", 10L, "12")
        testboth("%o", 100000000000L, "1351035564000")
        testboth("%d", 10L, "10")
        testboth("%d", 100000000000L, "100000000000")

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
        testboth("%d", float(big), "123456________________________", 6)

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
        testboth("%x", float(big), "123456_______________", 6)

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
        testboth("%o", float(big), "123456__________________________", 6)

        # Some small ints, in both Python int and long flavors).
        testboth("%d", 42, "42")
        testboth("%d", -42, "-42")
        testboth("%d", 42L, "42")
        testboth("%d", -42L, "-42")
        testboth("%d", 42.0, "42")
        testboth("%#x", 1, "0x1")
        testboth("%#x", 1L, "0x1")
        testboth("%#X", 1, "0X1")
        testboth("%#X", 1L, "0X1")
        testboth("%#x", 1.0, "0x1")
        testboth("%#o", 1, "01")
        testboth("%#o", 1L, "01")
        testboth("%#o", 0, "0")
        testboth("%#o", 0L, "0")
        testboth("%o", 0, "0")
        testboth("%o", 0L, "0")
        testboth("%d", 0, "0")
        testboth("%d", 0L, "0")
        testboth("%#x", 0, "0x0")
        testboth("%#x", 0L, "0x0")
        testboth("%#X", 0, "0X0")
        testboth("%#X", 0L, "0X0")

        testboth("%x", 0x42, "42")
        testboth("%x", -0x42, "-42")
        testboth("%x", 0x42L, "42")
        testboth("%x", -0x42L, "-42")
        testboth("%x", float(0x42), "42")

        testboth("%o", 042, "42")
        testboth("%o", -042, "-42")
        testboth("%o", 042L, "42")
        testboth("%o", -042L, "-42")
        testboth("%o", float(042), "42")

        # alternate float formatting
        testformat('%g', 1.1, '1.1')
        testformat('%#g', 1.1, '1.10000')

        # Regression test for http://bugs.python.org/issue15516.
        class IntFails(object):
            def __int__(self):
                raise TestFailed
            def __long__(self):
                return 0

        fst = IntFails()
        testformat("%x", fst, '0')

        # Test exception for unknown format characters
        if verbose:
            print 'Testing exceptions'

        def test_exc(formatstr, args, exception, excmsg):
            try:
                testformat(formatstr, args)
            except exception, exc:
                if str(exc) == excmsg:
                    if verbose:
                        print "yes"
                else:
                    if verbose: print 'no'
                    print 'Unexpected ', exception, ':', repr(str(exc))
            except:
                if verbose: print 'no'
                print 'Unexpected exception'
                raise
            else:
                raise TestFailed, 'did not get expected exception: %s' % excmsg

        test_exc('abc %a', 1, ValueError,
                 "unsupported format character 'a' (0x61) at index 5")
        if have_unicode:
            test_exc(unicode('abc %\u3000','raw-unicode-escape'), 1, ValueError,
                     "unsupported format character '?' (0x3000) at index 5")

        test_exc('%d', '1', TypeError, "%d format: a number is required, not str")
        test_exc('%g', '1', TypeError, "float argument required, not str")
        test_exc('no format', '1', TypeError,
                 "not all arguments converted during string formatting")
        test_exc('no format', u'1', TypeError,
                 "not all arguments converted during string formatting")
        test_exc(u'no format', '1', TypeError,
                 "not all arguments converted during string formatting")
        test_exc(u'no format', u'1', TypeError,
                 "not all arguments converted during string formatting")

        class Foobar(long):
            def __oct__(self):
                # Returning a non-string should not blow up.
                return self + 1

        test_exc('%o', Foobar(), TypeError,
                 "expected string or Unicode object, long found")

        if maxsize == 2**31-1:
            # crashes 2.2.1 and earlier:
            try:
                "%*d"%(maxsize, -127)
            except MemoryError:
                pass
            else:
                raise TestFailed, '"%*d"%(maxsize, -127) should fail'

def test_main():
    test_support.run_unittest(FormatTest)


if __name__ == "__main__":
    unittest.main()
