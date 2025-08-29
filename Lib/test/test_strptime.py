"""PyUnit testing against strptime"""

import unittest
import time
import locale
import re
import os
import platform
import sys
from test import support
from test.support import warnings_helper
from test.support import skip_if_buggy_ucrt_strfptime, run_with_locales
from datetime import date as datetime_date

import _strptime

libc_ver = platform.libc_ver()
if libc_ver[0] == 'glibc':
    glibc_ver = tuple(map(int, libc_ver[1].split('.')))
else:
    glibc_ver = None


class getlang_Tests(unittest.TestCase):
    """Test _getlang"""
    def test_basic(self):
        self.assertEqual(_strptime._getlang(), locale.getlocale(locale.LC_TIME))

class LocaleTime_Tests(unittest.TestCase):
    """Tests for _strptime.LocaleTime.

    All values are lower-cased when stored in LocaleTime, so make sure to
    compare values after running ``lower`` on them.

    """

    def setUp(self):
        """Create time tuple based on current time."""
        self.time_tuple = time.localtime()
        self.LT_ins = _strptime.LocaleTime()

    def compare_against_time(self, testing, directive, tuple_position,
                             error_msg):
        """Helper method that tests testing against directive based on the
        tuple_position of time_tuple.  Uses error_msg as error message.

        """
        strftime_output = time.strftime(directive, self.time_tuple).lower()
        comparison = testing[self.time_tuple[tuple_position]]
        self.assertIn(strftime_output, testing,
                      "%s: not found in tuple" % error_msg)
        self.assertEqual(comparison, strftime_output,
                         "%s: position within tuple incorrect; %s != %s" %
                         (error_msg, comparison, strftime_output))

    def test_weekday(self):
        # Make sure that full and abbreviated weekday names are correct in
        # both string and position with tuple
        self.compare_against_time(self.LT_ins.f_weekday, '%A', 6,
                                  "Testing of full weekday name failed")
        self.compare_against_time(self.LT_ins.a_weekday, '%a', 6,
                                  "Testing of abbreviated weekday name failed")

    def test_month(self):
        # Test full and abbreviated month names; both string and position
        # within the tuple
        self.compare_against_time(self.LT_ins.f_month, '%B', 1,
                                  "Testing against full month name failed")
        self.compare_against_time(self.LT_ins.a_month, '%b', 1,
                                  "Testing against abbreviated month name failed")

    def test_am_pm(self):
        # Make sure AM/PM representation done properly
        strftime_output = time.strftime("%p", self.time_tuple).lower()
        self.assertIn(strftime_output, self.LT_ins.am_pm,
                      "AM/PM representation not in tuple")
        if self.time_tuple[3] < 12: position = 0
        else: position = 1
        self.assertEqual(self.LT_ins.am_pm[position], strftime_output,
                         "AM/PM representation in the wrong position within the tuple")

    def test_timezone(self):
        # Make sure timezone is correct
        timezone = time.strftime("%Z", self.time_tuple).lower()
        if timezone:
            self.assertTrue(timezone in self.LT_ins.timezone[0] or
                            timezone in self.LT_ins.timezone[1],
                            "timezone %s not found in %s" %
                            (timezone, self.LT_ins.timezone))

    def test_date_time(self):
        # Check that LC_date_time, LC_date, and LC_time are correct
        # the magic date is used so as to not have issues with %c when day of
        #  the month is a single digit and has a leading space.  This is not an
        #  issue since strptime still parses it correctly.  The problem is
        #  testing these directives for correctness by comparing strftime
        #  output.
        magic_date = (1999, 3, 17, 22, 44, 55, 2, 76, 0)
        strftime_output = time.strftime("%c", magic_date)
        self.assertEqual(time.strftime(self.LT_ins.LC_date_time, magic_date),
                         strftime_output, "LC_date_time incorrect")
        strftime_output = time.strftime("%x", magic_date)
        self.assertEqual(time.strftime(self.LT_ins.LC_date, magic_date),
                         strftime_output, "LC_date incorrect")
        strftime_output = time.strftime("%X", magic_date)
        self.assertEqual(time.strftime(self.LT_ins.LC_time, magic_date),
                         strftime_output, "LC_time incorrect")
        LT = _strptime.LocaleTime()
        LT.am_pm = ('', '')
        self.assertTrue(LT.LC_time, "LocaleTime's LC directives cannot handle "
                                    "empty strings")

    def test_lang(self):
        # Make sure lang is set to what _getlang() returns
        # Assuming locale has not changed between now and when self.LT_ins was created
        self.assertEqual(self.LT_ins.lang, _strptime._getlang())


class TimeRETests(unittest.TestCase):
    """Tests for TimeRE."""

    def setUp(self):
        """Construct generic TimeRE object."""
        self.time_re = _strptime.TimeRE()
        self.locale_time = _strptime.LocaleTime()

    def test_pattern(self):
        # Test TimeRE.pattern
        pattern_string = self.time_re.pattern(r"%a %A %d %Y")
        self.assertTrue(pattern_string.find(self.locale_time.a_weekday[2]) != -1,
                        "did not find abbreviated weekday in pattern string '%s'" %
                         pattern_string)
        self.assertTrue(pattern_string.find(self.locale_time.f_weekday[4]) != -1,
                        "did not find full weekday in pattern string '%s'" %
                         pattern_string)
        self.assertTrue(pattern_string.find(self.time_re['d']) != -1,
                        "did not find 'd' directive pattern string '%s'" %
                         pattern_string)

    def test_pattern_escaping(self):
        # Make sure any characters in the format string that might be taken as
        # regex syntax is escaped.
        pattern_string = self.time_re.pattern(r"\d+")
        self.assertIn(r"\\d\+", pattern_string,
                      "%s does not have re characters escaped properly" %
                      pattern_string)

    @skip_if_buggy_ucrt_strfptime
    def test_compile(self):
        # Check that compiled regex is correct
        found = self.time_re.compile(r"%A").match(self.locale_time.f_weekday[6])
        self.assertTrue(found and found.group('A') == self.locale_time.f_weekday[6],
                        "re object for '%A' failed")
        compiled = self.time_re.compile(r"%a %b")
        found = compiled.match("%s %s" % (self.locale_time.a_weekday[4],
                               self.locale_time.a_month[4]))
        self.assertTrue(found,
            "Match failed with '%s' regex and '%s' string" %
             (compiled.pattern, "%s %s" % (self.locale_time.a_weekday[4],
                                           self.locale_time.a_month[4])))
        self.assertTrue(found.group('a') == self.locale_time.a_weekday[4] and
                         found.group('b') == self.locale_time.a_month[4],
                        "re object couldn't find the abbreviated weekday month in "
                         "'%s' using '%s'; group 'a' = '%s', group 'b' = %s'" %
                         (found.string, found.re.pattern, found.group('a'),
                          found.group('b')))
        for directive in ('a','A','b','B','c','d','G','H','I','j','m','M','p',
                          'S','u','U','V','w','W','x','X','y','Y','Z','%'):
            fmt = "%d %Y" if directive == 'd' else "%" + directive
            compiled = self.time_re.compile(fmt)
            found = compiled.match(time.strftime(fmt))
            self.assertTrue(found, "Matching failed on '%s' using '%s' regex" %
                                    (time.strftime(fmt),
                                     compiled.pattern))

    def test_blankpattern(self):
        # Make sure when tuple or something has no values no regex is generated.
        # Fixes bug #661354
        test_locale = _strptime.LocaleTime()
        test_locale.timezone = (frozenset(), frozenset())
        self.assertEqual(_strptime.TimeRE(test_locale).pattern("%Z"), '',
                         "with timezone == ('',''), TimeRE().pattern('%Z') != ''")

    def test_matching_with_escapes(self):
        # Make sure a format that requires escaping of characters works
        compiled_re = self.time_re.compile(r"\w+ %m")
        found = compiled_re.match(r"\w+ 10")
        self.assertTrue(found, r"Escaping failed of format '\w+ 10'")

    def test_locale_data_w_regex_metacharacters(self):
        # Check that if locale data contains regex metacharacters they are
        # escaped properly.
        # Discovered by bug #1039270 .
        locale_time = _strptime.LocaleTime()
        locale_time.timezone = (frozenset(("utc", "gmt",
                                            "Tokyo (standard time)")),
                                frozenset("Tokyo (daylight time)"))
        time_re = _strptime.TimeRE(locale_time)
        self.assertTrue(time_re.compile("%Z").match("Tokyo (standard time)"),
                        "locale data that contains regex metacharacters is not"
                        " properly escaped")

    def test_whitespace_substitution(self):
        # When pattern contains whitespace, make sure it is taken into account
        # so as to not allow subpatterns to end up next to each other and
        # "steal" characters from each other.
        pattern = self.time_re.pattern('%j %H')
        self.assertFalse(re.match(pattern, "180"))
        self.assertTrue(re.match(pattern, "18 0"))


class StrptimeTests(unittest.TestCase):
    """Tests for _strptime.strptime."""

    def setUp(self):
        """Create testing time tuples."""
        self.time_tuple = time.localtime()

    def test_ValueError(self):
        # Make sure ValueError is raised when match fails or format is bad
        self.assertRaises(ValueError, _strptime._strptime_time, data_string="%d",
                          format="%A")
        for bad_format in ("%", "% ", "%\n"):
            with (self.subTest(format=bad_format),
                  self.assertRaisesRegex(ValueError, "stray % in format ")):
                _strptime._strptime_time("2005", bad_format)
        for bad_format in ("%i", "%Oi", "%O", "%O ", "%Ee", "%E", "%E ",
                           "%.", "%+", "%~", "%\\",
                           "%O.", "%O+", "%O_", "%O~", "%O\\"):
            directive = bad_format[1:].rstrip()
            with (self.subTest(format=bad_format),
                  self.assertRaisesRegex(ValueError,
                    f"'{re.escape(directive)}' is a bad directive in format ")):
                _strptime._strptime_time("2005", bad_format)

        msg_week_no_year_or_weekday = r"ISO week directive '%V' must be used with " \
            r"the ISO year directive '%G' and a weekday directive " \
            r"\('%A', '%a', '%w', or '%u'\)."
        msg_week_not_compatible = r"ISO week directive '%V' is incompatible with " \
            r"the year directive '%Y'. Use the ISO year '%G' instead."
        msg_julian_not_compatible = r"Day of the year directive '%j' is not " \
            r"compatible with ISO year directive '%G'. Use '%Y' instead."
        msg_year_no_week_or_weekday = r"ISO year directive '%G' must be used with " \
            r"the ISO week directive '%V' and a weekday directive " \
            r"\('%A', '%a', '%w', or '%u'\)."

        locale_time = _strptime.LocaleTime()

        # Ambiguous or incomplete cases using ISO year/week/weekday directives
        subtests = [
            # 1. ISO week (%V) is specified, but the year is specified with %Y
            # instead of %G
            ("1999 50", "%Y %V", msg_week_no_year_or_weekday),
            ("1999 50 5", "%Y %V %u", msg_week_not_compatible),
            # 2. ISO year (%G) and ISO week (%V) are specified, but weekday is not
            ("1999 51", "%G %V", msg_year_no_week_or_weekday),
            # 3. ISO year (%G) and weekday are specified, but ISO week (%V) is not
            ("1999 {}".format(locale_time.f_weekday[5]), "%G %A",
                msg_year_no_week_or_weekday),
            ("1999 {}".format(locale_time.a_weekday[5]), "%G %a",
                msg_year_no_week_or_weekday),
            ("1999 5", "%G %w", msg_year_no_week_or_weekday),
            ("1999 5", "%G %u", msg_year_no_week_or_weekday),
            # 4. ISO year is specified alone (e.g. time.strptime('2015', '%G'))
            ("2015", "%G", msg_year_no_week_or_weekday),
            # 5. Julian/ordinal day (%j) is specified with %G, but not %Y
            ("1999 256", "%G %j", msg_julian_not_compatible),
            ("1999 50 5 256", "%G %V %u %j", msg_julian_not_compatible),
            # ISO week specified alone
            ("50", "%V", msg_week_no_year_or_weekday),
            # ISO year is unspecified, falling back to year
            ("50 5", "%V %u", msg_week_no_year_or_weekday),
            # 6. Invalid ISO weeks
            ("2019-00-1", "%G-%V-%u",
             "time data '2019-00-1' does not match format '%G-%V-%u'"),
            ("2019-54-1", "%G-%V-%u",
             "time data '2019-54-1' does not match format '%G-%V-%u'"),
            ("2021-53-1", "%G-%V-%u", "Invalid week: 53"),
        ]

        for (data_string, format, message) in subtests:
            with self.subTest(data_string=data_string, format=format):
                with self.assertRaisesRegex(ValueError, message):
                    _strptime._strptime(data_string, format)

    def test_strptime_exception_context(self):
        # check that this doesn't chain exceptions needlessly (see #17572)
        with self.assertRaises(ValueError) as e:
            _strptime._strptime_time('', '%D')
        self.assertTrue(e.exception.__suppress_context__)
        # additional check for stray % branch
        with self.assertRaises(ValueError) as e:
            _strptime._strptime_time('%', '%')
        self.assertTrue(e.exception.__suppress_context__)

    def test_unconverteddata(self):
        # Check ValueError is raised when there is unconverted data
        self.assertRaises(ValueError, _strptime._strptime_time, "10 12", "%m")

    def roundtrip(self, fmt, position, time_tuple=None):
        """Helper fxn in testing."""
        if time_tuple is None:
            time_tuple = self.time_tuple
        strf_output = time.strftime(fmt, time_tuple)
        strp_output = _strptime._strptime_time(strf_output, fmt)
        self.assertEqual(strp_output[position], time_tuple[position],
                        "testing of %r format failed; %r -> %r != %r" %
                         (fmt, strf_output, strp_output[position],
                          time_tuple[position]))
        if support.verbose >= 3:
            print("testing of %r format: %r -> %r" %
                  (fmt, strf_output, strp_output[position]))

    def test_year(self):
        # Test that the year is handled properly
        self.roundtrip('%Y', 0)
        self.roundtrip('%y', 0)
        self.roundtrip('%Y', 0, (1900, 1, 1, 0, 0, 0, 0, 1, 0))

        # Must also make sure %y values are correct for bounds set by Open Group
        strptime = _strptime._strptime_time
        self.assertEqual(strptime('00', '%y')[0], 2000)
        self.assertEqual(strptime('68', '%y')[0], 2068)
        self.assertEqual(strptime('69', '%y')[0], 1969)
        self.assertEqual(strptime('99', '%y')[0], 1999)

    def test_month(self):
        # Test for month directives
        self.roundtrip('%m', 1)

    @run_with_locales('LC_TIME', 'C', 'en_US', 'fr_FR', 'de_DE', 'ja_JP', 'he_IL', '')
    def test_month_locale(self):
        # Test for month directives
        self.roundtrip('%B', 1)
        self.roundtrip('%b', 1)
        for m in range(1, 13):
            self.roundtrip('%B', 1, (1900, m, 1, 0, 0, 0, 0, 1, 0))
            self.roundtrip('%b', 1, (1900, m, 1, 0, 0, 0, 0, 1, 0))

    @run_with_locales('LC_TIME', 'az_AZ', 'ber_DZ', 'ber_MA', 'crh_UA')
    def test_month_locale2(self):
        # Test for month directives
        # Month name contains 'İ' ('\u0130')
        self.roundtrip('%B', 1, (2025, 6, 1, 0, 0, 0, 6, 152, 0))
        self.roundtrip('%b', 1, (2025, 6, 1, 0, 0, 0, 6, 152, 0))
        self.roundtrip('%B', 1, (2025, 7, 1, 0, 0, 0, 1, 182, 0))
        self.roundtrip('%b', 1, (2025, 7, 1, 0, 0, 0, 1, 182, 0))

    def test_day(self):
        # Test for day directives
        self.roundtrip('%d %Y', 2)

    def test_hour(self):
        # Test hour directives
        self.roundtrip('%H', 3)

    # NB: Only works on locales with AM/PM
    @run_with_locales('LC_TIME', 'C', 'en_US', 'ja_JP')
    def test_hour_locale(self):
        # Test hour directives
        self.roundtrip('%I %p', 3)

    def test_minute(self):
        # Test minute directives
        self.roundtrip('%M', 4)

    def test_second(self):
        # Test second directives
        self.roundtrip('%S', 5)

    def test_fraction(self):
        # Test microseconds
        import datetime
        d = datetime.datetime(2012, 12, 20, 12, 34, 56, 78987)
        tup, frac, _ = _strptime._strptime(str(d), format="%Y-%m-%d %H:%M:%S.%f")
        self.assertEqual(frac, d.microsecond)

    def test_weekday(self):
        # Test weekday directives
        self.roundtrip('%w', 6)
        self.roundtrip('%u', 6)

    @run_with_locales('LC_TIME', 'C', 'en_US', 'fr_FR', 'de_DE', 'ja_JP', '')
    def test_weekday_locale(self):
        # Test weekday directives
        self.roundtrip('%A', 6)
        self.roundtrip('%a', 6)

    def test_julian(self):
        # Test julian directives
        self.roundtrip('%j', 7)

    def test_offset(self):
        one_hour = 60 * 60
        half_hour = 30 * 60
        half_minute = 30
        (*_, offset), _, offset_fraction = _strptime._strptime("+0130", "%z")
        self.assertEqual(offset, one_hour + half_hour)
        self.assertEqual(offset_fraction, 0)
        (*_, offset), _, offset_fraction = _strptime._strptime("-0100", "%z")
        self.assertEqual(offset, -one_hour)
        self.assertEqual(offset_fraction, 0)
        (*_, offset), _, offset_fraction = _strptime._strptime("-013030", "%z")
        self.assertEqual(offset, -(one_hour + half_hour + half_minute))
        self.assertEqual(offset_fraction, 0)
        (*_, offset), _, offset_fraction = _strptime._strptime("-013030.000001", "%z")
        self.assertEqual(offset, -(one_hour + half_hour + half_minute))
        self.assertEqual(offset_fraction, -1)
        (*_, offset), _, offset_fraction = _strptime._strptime("+01:00", "%z")
        self.assertEqual(offset, one_hour)
        self.assertEqual(offset_fraction, 0)
        (*_, offset), _, offset_fraction = _strptime._strptime("-01:30", "%z")
        self.assertEqual(offset, -(one_hour + half_hour))
        self.assertEqual(offset_fraction, 0)
        (*_, offset), _, offset_fraction = _strptime._strptime("-01:30:30", "%z")
        self.assertEqual(offset, -(one_hour + half_hour + half_minute))
        self.assertEqual(offset_fraction, 0)
        (*_, offset), _, offset_fraction = _strptime._strptime("-01:30:30.000001", "%z")
        self.assertEqual(offset, -(one_hour + half_hour + half_minute))
        self.assertEqual(offset_fraction, -1)
        (*_, offset), _, offset_fraction = _strptime._strptime("+01:30:30.001", "%z")
        self.assertEqual(offset, one_hour + half_hour + half_minute)
        self.assertEqual(offset_fraction, 1000)
        (*_, offset), _, offset_fraction = _strptime._strptime("Z", "%z")
        self.assertEqual(offset, 0)
        self.assertEqual(offset_fraction, 0)

    def test_bad_offset(self):
        with self.assertRaises(ValueError):
            _strptime._strptime("-01:30:30.", "%z")
        with self.assertRaises(ValueError):
            _strptime._strptime("-0130:30", "%z")
        with self.assertRaises(ValueError):
            _strptime._strptime("-01:30:30.1234567", "%z")
        with self.assertRaises(ValueError):
            _strptime._strptime("-01:30:30:123456", "%z")
        with self.assertRaises(ValueError) as err:
            _strptime._strptime("-01:3030", "%z")
        self.assertEqual("Inconsistent use of : in -01:3030", str(err.exception))

    @skip_if_buggy_ucrt_strfptime
    def test_timezone(self):
        # Test timezone directives.
        # When gmtime() is used with %Z, entire result of strftime() is empty.
        # Check for equal timezone names deals with bad locale info when this
        # occurs; first found in FreeBSD 4.4.
        strp_output = _strptime._strptime_time("UTC", "%Z")
        self.assertEqual(strp_output.tm_isdst, 0)
        strp_output = _strptime._strptime_time("GMT", "%Z")
        self.assertEqual(strp_output.tm_isdst, 0)
        time_tuple = time.localtime()
        strf_output = time.strftime("%Z")  #UTC does not have a timezone
        strp_output = _strptime._strptime_time(strf_output, "%Z")
        locale_time = _strptime.LocaleTime()
        if time.tzname[0] != time.tzname[1] or not time.daylight:
            self.assertTrue(strp_output[8] == time_tuple[8],
                            "timezone check failed; '%s' -> %s != %s" %
                             (strf_output, strp_output[8], time_tuple[8]))
        else:
            self.assertTrue(strp_output[8] == -1,
                            "LocaleTime().timezone has duplicate values and "
                             "time.daylight but timezone value not set to -1")

    @unittest.skipUnless(
        hasattr(time, "tzset"), "time module has no attribute tzset"
        )
    def test_bad_timezone(self):
        # Explicitly test possibility of bad timezone;
        # when time.tzname[0] == time.tzname[1] and time.daylight
        tz_name = time.tzname[0]
        if tz_name.upper() in ("UTC", "GMT"):
            self.skipTest('need non-UTC/GMT timezone')

        with support.swap_attr(time, 'tzname', (tz_name, tz_name)), \
             support.swap_attr(time, 'daylight', 1), \
             support.swap_attr(time, 'tzset', lambda: None):
            time.tzname = (tz_name, tz_name)
            time.daylight = 1
            tz_value = _strptime._strptime_time(tz_name, "%Z")[8]
            self.assertEqual(tz_value, -1,
                    "%s lead to a timezone value of %s instead of -1 when "
                    "time.daylight set to %s and passing in %s" %
                    (time.tzname, tz_value, time.daylight, tz_name))

    # NB: Does not roundtrip in some locales due to the ambiguity of
    # the date and time representation (bugs in locales?):
    # * Seconds are not included: bem_ZM, bokmal, ff_SN, nb_NO, nn_NO,
    #   no_NO, norwegian, nynorsk.
    # * Hours are in 12-hour notation without AM/PM indication: hy_AM,
    #   id_ID, ms_MY.
    # * Year is not included: ha_NG.
    # * Use non-Gregorian calendar: lo_LA, thai, th_TH.
    #   On Windows: ar_IN, ar_SA, fa_IR, ps_AF.
    @run_with_locales('LC_TIME', 'C', 'en_US', 'fr_FR', 'de_DE', 'ja_JP',
                      'he_IL', 'eu_ES', 'ar_AE', 'mfe_MU', 'yo_NG',
                      'csb_PL', 'br_FR', 'gez_ET', 'brx_IN',
                      'my_MM', 'or_IN', 'shn_MM', 'az_IR',
                      'byn_ER', 'wal_ET', 'lzh_TW')
    def test_date_time_locale(self):
        # Test %c directive
        loc = locale.getlocale(locale.LC_TIME)[0]
        if glibc_ver and glibc_ver < (2, 31) and loc == 'br_FR':
            self.skipTest('%c in locale br_FR does not include time')
        now = time.time()
        self.roundtrip('%c', slice(0, 6), time.localtime(now))
        # 1 hour 20 minutes 30 seconds ago
        self.roundtrip('%c', slice(0, 6), time.localtime(now - 4830))
        # 12 hours ago
        self.roundtrip('%c', slice(0, 6), time.localtime(now - 12*3600))
        # different days of the week
        for i in range(1, 7):
            self.roundtrip('%c', slice(0, 6), time.localtime(now - i*24*3600))
        # different months
        for i in range(1, 12):
            self.roundtrip('%c', slice(0, 6), time.localtime(now - i*30*24*3600))
        # different year
        self.roundtrip('%c', slice(0, 6), time.localtime(now - 366*24*3600))

    # NB: Dates before 1969 do not roundtrip on some locales:
    # az_IR, bo_CN, bo_IN, dz_BT, eu_ES, eu_FR, fa_IR, or_IN.
    @support.run_with_tz('STD-1DST,M4.1.0,M10.1.0')
    @run_with_locales('LC_TIME', 'C', 'en_US', 'fr_FR', 'de_DE', 'ja_JP',
                      'he_IL', 'ar_AE', 'mfe_MU', 'yo_NG',
                      'csb_PL', 'br_FR', 'gez_ET', 'brx_IN',
                      'my_MM', 'shn_MM')
    def test_date_time_locale2(self):
        # Test %c directive
        loc = locale.getlocale(locale.LC_TIME)[0]
        if sys.platform.startswith('sunos'):
            if loc in ('ar_AE',):
                self.skipTest(f'locale {loc!r} may not work on this platform')
        self.roundtrip('%c', slice(0, 6), (1900, 1, 1, 0, 0, 0, 0, 1, 0))
        self.roundtrip('%c', slice(0, 6), (1800, 1, 1, 0, 0, 0, 0, 1, 0))

    # NB: Does not roundtrip because use non-Gregorian calendar:
    # lo_LA, thai, th_TH. On Windows: ar_IN, ar_SA, fa_IR, ps_AF.
    @run_with_locales('LC_TIME', 'C', 'en_US', 'fr_FR', 'de_DE', 'ja_JP',
                      'he_IL', 'eu_ES', 'ar_AE',
                      'az_IR', 'my_MM', 'or_IN', 'shn_MM', 'lzh_TW')
    def test_date_locale(self):
        # Test %x directive
        now = time.time()
        self.roundtrip('%x', slice(0, 3), time.localtime(now))
        # different days of the week
        for i in range(1, 7):
            self.roundtrip('%x', slice(0, 3), time.localtime(now - i*24*3600))
        # different months
        for i in range(1, 12):
            self.roundtrip('%x', slice(0, 3), time.localtime(now - i*30*24*3600))
        # different year
        self.roundtrip('%x', slice(0, 3), time.localtime(now - 366*24*3600))

    # NB: Dates before 1969 do not roundtrip on many locales, including C.
    @unittest.skipIf(support.linked_to_musl(), "musl libc issue, bpo-46390")
    @run_with_locales('LC_TIME', 'en_US', 'fr_FR', 'de_DE', 'ja_JP',
                      'eu_ES', 'ar_AE', 'my_MM', 'shn_MM', 'lzh_TW')
    def test_date_locale2(self):
        # Test %x directive
        loc = locale.getlocale(locale.LC_TIME)[0]
        if sys.platform.startswith('sunos'):
            if loc in ('en_US', 'de_DE', 'ar_AE'):
                self.skipTest(f'locale {loc!r} may not work on this platform')
        self.roundtrip('%x', slice(0, 3), (1900, 1, 1, 0, 0, 0, 0, 1, 0))
        self.roundtrip('%x', slice(0, 3), (1800, 1, 1, 0, 0, 0, 0, 1, 0))

    # NB: Does not roundtrip in some locales due to the ambiguity of
    # the time representation (bugs in locales?):
    # * Seconds are not included: bokmal, ff_SN, nb_NO, nn_NO, no_NO,
    #   norwegian, nynorsk.
    # * Hours are in 12-hour notation without AM/PM indication: hy_AM,
    #   ms_MY, sm_WS.
    @run_with_locales('LC_TIME', 'C', 'en_US', 'fr_FR', 'de_DE', 'ja_JP',
                      'aa_ET', 'am_ET', 'az_IR', 'byn_ER', 'fa_IR', 'gez_ET',
                      'my_MM', 'om_ET', 'or_IN', 'shn_MM', 'sid_ET', 'so_SO',
                      'ti_ET', 'tig_ER', 'wal_ET', 'lzh_TW',
                      'ar_SA', 'bg_BG')
    def test_time_locale(self):
        # Test %X directive
        loc = locale.getlocale(locale.LC_TIME)[0]
        pos = slice(3, 6)
        if glibc_ver and glibc_ver < (2, 29) and loc in {
                'aa_ET', 'am_ET', 'byn_ER', 'gez_ET', 'om_ET',
                'sid_ET', 'so_SO', 'ti_ET', 'tig_ER', 'wal_ET'}:
            # Hours are in 12-hour notation without AM/PM indication.
            # Ignore hours.
            pos = slice(4, 6)
        now = time.time()
        self.roundtrip('%X', pos, time.localtime(now))
        # 1 hour 20 minutes 30 seconds ago
        self.roundtrip('%X', pos, time.localtime(now - 4830))
        # 12 hours ago
        self.roundtrip('%X', pos, time.localtime(now - 12*3600))

    def test_percent(self):
        # Make sure % signs are handled properly
        strf_output = time.strftime("%m %% %Y", self.time_tuple)
        strp_output = _strptime._strptime_time(strf_output, "%m %% %Y")
        self.assertTrue(strp_output[0] == self.time_tuple[0] and
                         strp_output[1] == self.time_tuple[1],
                        "handling of percent sign failed")

    def test_caseinsensitive(self):
        # Should handle names case-insensitively.
        strf_output = time.strftime("%B", self.time_tuple)
        self.assertTrue(_strptime._strptime_time(strf_output.upper(), "%B"),
                        "strptime does not handle ALL-CAPS names properly")
        self.assertTrue(_strptime._strptime_time(strf_output.lower(), "%B"),
                        "strptime does not handle lowercase names properly")
        self.assertTrue(_strptime._strptime_time(strf_output.capitalize(), "%B"),
                        "strptime does not handle capword names properly")

    def test_defaults(self):
        # Default return value should be (1900, 1, 1, 0, 0, 0, 0, 1, 0)
        defaults = (1900, 1, 1, 0, 0, 0, 0, 1, -1)
        strp_output = _strptime._strptime_time('1', '%m')
        self.assertTrue(strp_output == defaults,
                        "Default values for strptime() are incorrect;"
                        " %s != %s" % (strp_output, defaults))

    def test_escaping(self):
        # Make sure all characters that have regex significance are escaped.
        # Parentheses are in a purposeful order; will cause an error of
        # unbalanced parentheses when the regex is compiled if they are not
        # escaped.
        # Test instigated by bug #796149 .
        need_escaping = r".^$*+?{}\[]|)("
        self.assertTrue(_strptime._strptime_time(need_escaping, need_escaping))

    @warnings_helper.ignore_warnings(category=DeprecationWarning)  # gh-70647
    def test_feb29_on_leap_year_without_year(self):
        time.strptime("Feb 29", "%b %d")

    @warnings_helper.ignore_warnings(category=DeprecationWarning)  # gh-70647
    def test_mar1_comes_after_feb29_even_when_omitting_the_year(self):
        self.assertLess(
                time.strptime("Feb 29", "%b %d"),
                time.strptime("Mar 1", "%b %d"))

class Strptime12AMPMTests(unittest.TestCase):
    """Test a _strptime regression in '%I %p' at 12 noon (12 PM)"""

    def test_twelve_noon_midnight(self):
        eq = self.assertEqual
        eq(time.strptime('12 PM', '%I %p')[3], 12)
        eq(time.strptime('12 AM', '%I %p')[3], 0)
        eq(_strptime._strptime_time('12 PM', '%I %p')[3], 12)
        eq(_strptime._strptime_time('12 AM', '%I %p')[3], 0)


class JulianTests(unittest.TestCase):
    """Test a _strptime regression that all julian (1-366) are accepted"""

    def test_all_julian_days(self):
        eq = self.assertEqual
        for i in range(1, 367):
            # use 2004, since it is a leap year, we have 366 days
            eq(_strptime._strptime_time('%d 2004' % i, '%j %Y')[7], i)

class CalculationTests(unittest.TestCase):
    """Test that strptime() fills in missing info correctly"""

    def setUp(self):
        self.time_tuple = time.gmtime()

    @skip_if_buggy_ucrt_strfptime
    def test_julian_calculation(self):
        # Make sure that when Julian is missing that it is calculated
        format_string = "%Y %m %d %H %M %S %w %Z"
        result = _strptime._strptime_time(time.strftime(format_string, self.time_tuple),
                                    format_string)
        self.assertTrue(result.tm_yday == self.time_tuple.tm_yday,
                        "Calculation of tm_yday failed; %s != %s" %
                         (result.tm_yday, self.time_tuple.tm_yday))

    @skip_if_buggy_ucrt_strfptime
    def test_gregorian_calculation(self):
        # Test that Gregorian date can be calculated from Julian day
        format_string = "%Y %H %M %S %w %j %Z"
        result = _strptime._strptime_time(time.strftime(format_string, self.time_tuple),
                                    format_string)
        self.assertTrue(result.tm_year == self.time_tuple.tm_year and
                         result.tm_mon == self.time_tuple.tm_mon and
                         result.tm_mday == self.time_tuple.tm_mday,
                        "Calculation of Gregorian date failed; "
                         "%s-%s-%s != %s-%s-%s" %
                         (result.tm_year, result.tm_mon, result.tm_mday,
                          self.time_tuple.tm_year, self.time_tuple.tm_mon,
                          self.time_tuple.tm_mday))

    @skip_if_buggy_ucrt_strfptime
    def test_day_of_week_calculation(self):
        # Test that the day of the week is calculated as needed
        format_string = "%Y %m %d %H %S %j %Z"
        result = _strptime._strptime_time(time.strftime(format_string, self.time_tuple),
                                    format_string)
        self.assertTrue(result.tm_wday == self.time_tuple.tm_wday,
                        "Calculation of day of the week failed; "
                         "%s != %s" % (result.tm_wday, self.time_tuple.tm_wday))

    if support.is_android:
        # Issue #26929: strftime() on Android incorrectly formats %V or %G for
        # the last or the first incomplete week in a year.
        _ymd_excluded = ((1905, 1, 1), (1906, 12, 31), (2008, 12, 29),
                        (1917, 12, 31))
        _formats_excluded = ('%G %V',)
    else:
        _ymd_excluded = ()
        _formats_excluded = ()

    @unittest.skipIf(sys.platform.startswith('aix'),
                     'bpo-29972: broken test on AIX')
    def test_week_of_year_and_day_of_week_calculation(self):
        # Should be able to infer date if given year, week of year (%U or %W)
        # and day of the week
        def test_helper(ymd_tuple, test_reason):
            for year_week_format in ('%Y %W', '%Y %U', '%G %V'):
                if (year_week_format in self._formats_excluded and
                        ymd_tuple in self._ymd_excluded):
                    return
                for weekday_format in ('%w', '%u', '%a', '%A'):
                    format_string = year_week_format + ' ' + weekday_format
                    with self.subTest(test_reason,
                                      date=ymd_tuple,
                                      format=format_string):
                        dt_date = datetime_date(*ymd_tuple)
                        strp_input = dt_date.strftime(format_string)
                        strp_output = _strptime._strptime_time(strp_input,
                                                               format_string)
                        msg = "%r: %s != %s" % (strp_input,
                                                strp_output[7],
                                                dt_date.timetuple()[7])
                        self.assertEqual(strp_output[:3], ymd_tuple, msg)
        test_helper((1901, 1, 3), "week 0")
        test_helper((1901, 1, 8), "common case")
        test_helper((1901, 1, 13), "day on Sunday")
        test_helper((1901, 1, 14), "day on Monday")
        test_helper((1905, 1, 1), "Jan 1 on Sunday")
        test_helper((1906, 1, 1), "Jan 1 on Monday")
        test_helper((1906, 1, 7), "first Sunday in a year starting on Monday")
        test_helper((1905, 12, 31), "Dec 31 on Sunday")
        test_helper((1906, 12, 31), "Dec 31 on Monday")
        test_helper((2008, 12, 29), "Monday in the last week of the year")
        test_helper((2008, 12, 22), "Monday in the second-to-last week of the "
                                    "year")
        test_helper((1978, 10, 23), "randomly chosen date")
        test_helper((2004, 12, 18), "randomly chosen date")
        test_helper((1978, 10, 23), "year starting and ending on Monday while "
                                        "date not on Sunday or Monday")
        test_helper((1917, 12, 17), "year starting and ending on Monday with "
                                        "a Monday not at the beginning or end "
                                        "of the year")
        test_helper((1917, 12, 31), "Dec 31 on Monday with year starting and "
                                        "ending on Monday")
        test_helper((2007, 1, 7), "First Sunday of 2007")
        test_helper((2007, 1, 14), "Second Sunday of 2007")
        test_helper((2006, 12, 31), "Last Sunday of 2006")
        test_helper((2006, 12, 24), "Second to last Sunday of 2006")

    def test_week_0(self):
        def check(value, format, *expected):
            self.assertEqual(_strptime._strptime_time(value, format)[:-1], expected)
        check('2015 0 0', '%Y %U %w', 2014, 12, 28, 0, 0, 0, 6, 362)
        check('2015 0 0', '%Y %W %w', 2015, 1, 4, 0, 0, 0, 6, 4)
        check('2015 1 1', '%G %V %u', 2014, 12, 29, 0, 0, 0, 0, 363)
        check('2015 0 1', '%Y %U %w', 2014, 12, 29, 0, 0, 0, 0, 363)
        check('2015 0 1', '%Y %W %w', 2014, 12, 29, 0, 0, 0, 0, 363)
        check('2015 1 2', '%G %V %u', 2014, 12, 30, 0, 0, 0, 1, 364)
        check('2015 0 2', '%Y %U %w', 2014, 12, 30, 0, 0, 0, 1, 364)
        check('2015 0 2', '%Y %W %w', 2014, 12, 30, 0, 0, 0, 1, 364)
        check('2015 1 3', '%G %V %u', 2014, 12, 31, 0, 0, 0, 2, 365)
        check('2015 0 3', '%Y %U %w', 2014, 12, 31, 0, 0, 0, 2, 365)
        check('2015 0 3', '%Y %W %w', 2014, 12, 31, 0, 0, 0, 2, 365)
        check('2015 1 4', '%G %V %u', 2015, 1, 1, 0, 0, 0, 3, 1)
        check('2015 0 4', '%Y %U %w', 2015, 1, 1, 0, 0, 0, 3, 1)
        check('2015 0 4', '%Y %W %w', 2015, 1, 1, 0, 0, 0, 3, 1)
        check('2015 1 5', '%G %V %u', 2015, 1, 2, 0, 0, 0, 4, 2)
        check('2015 0 5', '%Y %U %w', 2015, 1, 2, 0, 0, 0, 4, 2)
        check('2015 0 5', '%Y %W %w', 2015, 1, 2, 0, 0, 0, 4, 2)
        check('2015 1 6', '%G %V %u', 2015, 1, 3, 0, 0, 0, 5, 3)
        check('2015 0 6', '%Y %U %w', 2015, 1, 3, 0, 0, 0, 5, 3)
        check('2015 0 6', '%Y %W %w', 2015, 1, 3, 0, 0, 0, 5, 3)
        check('2015 1 7', '%G %V %u', 2015, 1, 4, 0, 0, 0, 6, 4)

        check('2009 0 0', '%Y %U %w', 2008, 12, 28, 0, 0, 0, 6, 363)
        check('2009 0 0', '%Y %W %w', 2009, 1, 4, 0, 0, 0, 6, 4)
        check('2009 1 1', '%G %V %u', 2008, 12, 29, 0, 0, 0, 0, 364)
        check('2009 0 1', '%Y %U %w', 2008, 12, 29, 0, 0, 0, 0, 364)
        check('2009 0 1', '%Y %W %w', 2008, 12, 29, 0, 0, 0, 0, 364)
        check('2009 1 2', '%G %V %u', 2008, 12, 30, 0, 0, 0, 1, 365)
        check('2009 0 2', '%Y %U %w', 2008, 12, 30, 0, 0, 0, 1, 365)
        check('2009 0 2', '%Y %W %w', 2008, 12, 30, 0, 0, 0, 1, 365)
        check('2009 1 3', '%G %V %u', 2008, 12, 31, 0, 0, 0, 2, 366)
        check('2009 0 3', '%Y %U %w', 2008, 12, 31, 0, 0, 0, 2, 366)
        check('2009 0 3', '%Y %W %w', 2008, 12, 31, 0, 0, 0, 2, 366)
        check('2009 1 4', '%G %V %u', 2009, 1, 1, 0, 0, 0, 3, 1)
        check('2009 0 4', '%Y %U %w', 2009, 1, 1, 0, 0, 0, 3, 1)
        check('2009 0 4', '%Y %W %w', 2009, 1, 1, 0, 0, 0, 3, 1)
        check('2009 1 5', '%G %V %u', 2009, 1, 2, 0, 0, 0, 4, 2)
        check('2009 0 5', '%Y %U %w', 2009, 1, 2, 0, 0, 0, 4, 2)
        check('2009 0 5', '%Y %W %w', 2009, 1, 2, 0, 0, 0, 4, 2)
        check('2009 1 6', '%G %V %u', 2009, 1, 3, 0, 0, 0, 5, 3)
        check('2009 0 6', '%Y %U %w', 2009, 1, 3, 0, 0, 0, 5, 3)
        check('2009 0 6', '%Y %W %w', 2009, 1, 3, 0, 0, 0, 5, 3)
        check('2009 1 7', '%G %V %u', 2009, 1, 4, 0, 0, 0, 6, 4)


class CacheTests(unittest.TestCase):
    """Test that caching works properly."""

    def test_time_re_recreation(self):
        # Make sure cache is recreated when current locale does not match what
        # cached object was created with.
        _strptime._strptime_time("10 2004", "%d %Y")
        _strptime._strptime_time("2005", "%Y")
        _strptime._TimeRE_cache.locale_time.lang = "Ni"
        original_time_re = _strptime._TimeRE_cache
        _strptime._strptime_time("10 2004", "%d %Y")
        self.assertIsNot(original_time_re, _strptime._TimeRE_cache)
        self.assertEqual(len(_strptime._regex_cache), 1)

    def test_regex_cleanup(self):
        # Make sure cached regexes are discarded when cache becomes "full".
        try:
            del _strptime._regex_cache['%d %Y']
        except KeyError:
            pass
        bogus_key = 0
        while len(_strptime._regex_cache) <= _strptime._CACHE_MAX_SIZE:
            _strptime._regex_cache[bogus_key] = None
            bogus_key += 1
        _strptime._strptime_time("10 2004", "%d %Y")
        self.assertEqual(len(_strptime._regex_cache), 1)

    def test_new_localetime(self):
        # A new LocaleTime instance should be created when a new TimeRE object
        # is created.
        locale_time_id = _strptime._TimeRE_cache.locale_time
        _strptime._TimeRE_cache.locale_time.lang = "Ni"
        _strptime._strptime_time("10 2004", "%d %Y")
        self.assertIsNot(locale_time_id, _strptime._TimeRE_cache.locale_time)

    def test_TimeRE_recreation_locale(self):
        # The TimeRE instance should be recreated upon changing the locale.
        with support.run_with_locale('LC_TIME', 'en_US.UTF8'):
            _strptime._strptime_time('10 2004', '%d %Y')
            # Get id of current cache object.
            first_time_re = _strptime._TimeRE_cache
            try:
                # Change the locale and force a recreation of the cache.
                locale.setlocale(locale.LC_TIME, ('de_DE', 'UTF8'))
                _strptime._strptime_time('10 2004', '%d %Y')
                # Get the new cache object's id.
                second_time_re = _strptime._TimeRE_cache
                # They should not be equal.
                self.assertIsNot(first_time_re, second_time_re)
            # Possible test locale is not supported while initial locale is.
            # If this is the case just suppress the exception and fall-through
            # to the resetting to the original locale.
            except locale.Error:
                self.skipTest('test needs de_DE.UTF8 locale')

    @support.run_with_tz('STD-1DST,M4.1.0,M10.1.0')
    def test_TimeRE_recreation_timezone(self):
        # The TimeRE instance should be recreated upon changing the timezone.
        oldtzname = time.tzname
        tm = _strptime._strptime_time(time.tzname[0], '%Z')
        self.assertEqual(tm.tm_isdst, 0)
        tm = _strptime._strptime_time(time.tzname[1], '%Z')
        self.assertEqual(tm.tm_isdst, 1)
        # Get id of current cache object.
        first_time_re = _strptime._TimeRE_cache
        # Change the timezone and force a recreation of the cache.
        os.environ['TZ'] = 'EST+05EDT,M3.2.0,M11.1.0'
        time.tzset()
        tm = _strptime._strptime_time(time.tzname[0], '%Z')
        self.assertEqual(tm.tm_isdst, 0)
        tm = _strptime._strptime_time(time.tzname[1], '%Z')
        self.assertEqual(tm.tm_isdst, 1)
        # Get the new cache object's id.
        second_time_re = _strptime._TimeRE_cache
        # They should not be equal.
        self.assertIsNot(first_time_re, second_time_re)
        # Make sure old names no longer accepted.
        with self.assertRaises(ValueError):
            _strptime._strptime_time(oldtzname[0], '%Z')
        with self.assertRaises(ValueError):
            _strptime._strptime_time(oldtzname[1], '%Z')


if __name__ == '__main__':
    unittest.main()
