from test.support import verbose, TestFailed
import locale
import sys
import test.support as support
import unittest

maxsize = support.MAX_Py_ssize_t

# test string formatting operator (I am not sure if this is being tested
# elsewhere but, surely, some of the given cases are *not* tested because
# they crash python)
# test on unicode strings as well

def testformat(formatstr, args, output=None, limit=None, overflowok=False):
    if verbose:
        if output:
            print("{!a} % {!a} =? {!a} ...".format(formatstr, args, output),
                  end=' ')
        else:
            print("{!a} % {!a} works? ...".format(formatstr, args), end=' ')
    try:
        result = formatstr % args
    except OverflowError:
        if not overflowok:
            raise
        if verbose:
            print('overflow (this is fine)')
    else:
        if output and limit is None and result != output:
            if verbose:
                print('no')
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
                print('no')
            print("%s %% %s == %s != %s" % \
                  (repr(formatstr), repr(args), repr(result), repr(output)))
        else:
            if verbose:
                print('yes')


class FormatTest(unittest.TestCase):
    def test_format(self):
        testformat("%.1d", (1,), "1")
        testformat("%.*d", (sys.maxsize,1), overflowok=True)  # expect overflow
        testformat("%.100d", (1,), '00000000000000000000000000000000000000'
                 '000000000000000000000000000000000000000000000000000000'
                 '00000001', overflowok=True)
        testformat("%#.117x", (1,), '0x00000000000000000000000000000000000'
                 '000000000000000000000000000000000000000000000000000000'
                 '0000000000000000000000000001',
                 overflowok=True)
        testformat("%#.118x", (1,), '0x00000000000000000000000000000000000'
                 '000000000000000000000000000000000000000000000000000000'
                 '00000000000000000000000000001',
                 overflowok=True)

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
        # these tests should no longer cause overflow in Python
        # 2.7/3.1 and later.
        testformat("%#.*g", (110, -1.e+100/3.))
        testformat("%#.*G", (110, -1.e+100/3.))
        testformat("%#.*f", (110, -1.e+100/3.))
        testformat("%#.*F", (110, -1.e+100/3.))
        # Formatting of integers. Overflow is not ok
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
        testformat("%d", float(big), "123456________________________", 6)
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
        testformat("%x", float(big), "123456_______________", 6)
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
        testformat("%#+.34o", big, "+0o0012345670123456701234567012345670")
        testformat("%#-+.34o", big, "+0o0012345670123456701234567012345670")
        testformat("%#-+37.34o", big, "+0o0012345670123456701234567012345670")
        testformat("%#+37.34o", big, "+0o0012345670123456701234567012345670")
        # next one gets one leading zero from precision
        testformat("%.33o", big, "012345670123456701234567012345670")
        # base marker shouldn't change that, since "0" is redundant
        testformat("%#.33o", big, "0o012345670123456701234567012345670")
        # but reduce precision, and base marker should add a zero
        testformat("%#.32o", big, "0o12345670123456701234567012345670")
        # one leading zero from precision, and another from "0" flag & width
        testformat("%034.33o", big, "0012345670123456701234567012345670")
        # base marker shouldn't change that
        testformat("%0#34.33o", big, "0o012345670123456701234567012345670")
        testformat("%o", float(big), "123456__________________________", 6)
        # Some small ints, in both Python int and flavors).
        testformat("%d", 42, "42")
        testformat("%d", -42, "-42")
        testformat("%d", 42, "42")
        testformat("%d", -42, "-42")
        testformat("%d", 42.0, "42")
        testformat("%#x", 1, "0x1")
        testformat("%#x", 1, "0x1")
        testformat("%#X", 1, "0X1")
        testformat("%#X", 1, "0X1")
        testformat("%#x", 1.0, "0x1")
        testformat("%#o", 1, "0o1")
        testformat("%#o", 1, "0o1")
        testformat("%#o", 0, "0o0")
        testformat("%#o", 0, "0o0")
        testformat("%o", 0, "0")
        testformat("%o", 0, "0")
        testformat("%d", 0, "0")
        testformat("%d", 0, "0")
        testformat("%#x", 0, "0x0")
        testformat("%#x", 0, "0x0")
        testformat("%#X", 0, "0X0")
        testformat("%#X", 0, "0X0")
        testformat("%x", 0x42, "42")
        testformat("%x", -0x42, "-42")
        testformat("%x", 0x42, "42")
        testformat("%x", -0x42, "-42")
        testformat("%x", float(0x42), "42")
        testformat("%o", 0o42, "42")
        testformat("%o", -0o42, "-42")
        testformat("%o", 0o42, "42")
        testformat("%o", -0o42, "-42")
        testformat("%o", float(0o42), "42")
        testformat("%r", "\u0378", "'\\u0378'")  # non printable
        testformat("%a", "\u0378", "'\\u0378'")  # non printable
        testformat("%r", "\u0374", "'\u0374'")   # printable
        testformat("%a", "\u0374", "'\\u0374'")  # printable

        # alternate float formatting
        testformat('%g', 1.1, '1.1')
        testformat('%#g', 1.1, '1.10000')

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
        test_exc('abc %b', 1, ValueError,
                 "unsupported format character 'b' (0x62) at index 5")
        #test_exc(unicode('abc %\u3000','raw-unicode-escape'), 1, ValueError,
        #         "unsupported format character '?' (0x3000) at index 5")
        test_exc('%d', '1', TypeError, "%d format: a number is required, not str")
        test_exc('%g', '1', TypeError, "a float is required")
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

    def test_non_ascii(self):
        testformat("\u20ac=%f", (1.0,), "\u20ac=1.000000")

        self.assertEqual(format("abc", "\u2007<5"), "abc\u2007\u2007")
        self.assertEqual(format(123, "\u2007<5"), "123\u2007\u2007")
        self.assertEqual(format(12.3, "\u2007<6"), "12.3\u2007\u2007")
        self.assertEqual(format(0j, "\u2007<4"), "0j\u2007\u2007")
        self.assertEqual(format(1+2j, "\u2007<8"), "(1+2j)\u2007\u2007")

        self.assertEqual(format("abc", "\u2007>5"), "\u2007\u2007abc")
        self.assertEqual(format(123, "\u2007>5"), "\u2007\u2007123")
        self.assertEqual(format(12.3, "\u2007>6"), "\u2007\u200712.3")
        self.assertEqual(format(1+2j, "\u2007>8"), "\u2007\u2007(1+2j)")
        self.assertEqual(format(0j, "\u2007>4"), "\u2007\u20070j")

        self.assertEqual(format("abc", "\u2007^5"), "\u2007abc\u2007")
        self.assertEqual(format(123, "\u2007^5"), "\u2007123\u2007")
        self.assertEqual(format(12.3, "\u2007^6"), "\u200712.3\u2007")
        self.assertEqual(format(1+2j, "\u2007^8"), "\u2007(1+2j)\u2007")
        self.assertEqual(format(0j, "\u2007^4"), "\u20070j\u2007")

    def test_locale(self):
        try:
            oldloc = locale.setlocale(locale.LC_ALL)
            locale.setlocale(locale.LC_ALL, '')
        except locale.Error as err:
            self.skipTest("Cannot set locale: {}".format(err))
        try:
            localeconv = locale.localeconv()
            sep = localeconv['thousands_sep']
            point = localeconv['decimal_point']

            text = format(123456789, "n")
            self.assertIn(sep, text)
            self.assertEqual(text.replace(sep, ''), '123456789')

            text = format(1234.5, "n")
            self.assertIn(sep, text)
            self.assertIn(point, text)
            self.assertEqual(text.replace(sep, ''), '1234' + point + '5')
        finally:
            locale.setlocale(locale.LC_ALL, oldloc)



def test_main():
    support.run_unittest(FormatTest)


if __name__ == "__main__":
    unittest.main()
