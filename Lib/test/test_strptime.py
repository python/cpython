"""PyUnit testing against strptime >= 2.1.0."""

import sys
sys.path.append('..')
import unittest
import time
import locale
import re

import _strptime

__version__ = (1,0,5)

class LocaleTime_Tests(unittest.TestCase):
    """Contains all tests for _strptime.LocaleTime."""

    def setUp(self):
        """Create time tuple based on current time."""
        self.time_tuple = time.localtime()
        self.LT_ins = _strptime.LocaleTime()

    def compare_against_time(self, testing, directive, tuple_position, error_msg):
        """Helper method that tests testing against directive based on the 
        tuple_position of time_tuple.  Uses error_msg as error message.

        """
        strftime_output = time.strftime(directive, self.time_tuple)
        comparison = testing[self.time_tuple[tuple_position]]
        self.failUnless(strftime_output in testing, "%s: not found in tuple" % error_msg)
        self.failUnless(comparison == strftime_output, "%s: position within tuple incorrect; %s != %s" % (error_msg, comparison, strftime_output))
    
    def test_weekday(self):
        """Make sure that full and abbreviated weekday names are correct in 
        both string and position with tuple.
        
        """
        self.compare_against_time(self.LT_ins.f_weekday, '%A', 6, "Testing of full weekday name failed")
        self.compare_against_time(self.LT_ins.a_weekday, '%a', 6, "Testing of abbreviated weekday name failed")

    def test_month(self):
        """Test full and abbreviated month names; both string and position 
        within the tuple.

        """
        self.compare_against_time(self.LT_ins.f_month, '%B', 1, "Testing against full month name failed")
        self.compare_against_time(self.LT_ins.a_month, '%b', 1, "Testing against abbreviated month name failed")

    def test_am_pm(self):
        """Make sure AM/PM representation done properly."""
        strftime_output = time.strftime("%p", self.time_tuple)
        self.failUnless(strftime_output in self.LT_ins.am_pm, "AM/PM representation not in tuple")
        if self.time_tuple[3] < 12: position = 0
        else: position = 1
        self.failUnless(strftime_output == self.LT_ins.am_pm[position], "AM/PM representation in the wrong position within the tuple")

    def test_timezone(self):
        """Make sure timezone is correct."""
        if time.strftime("%Z", self.time_tuple):
            self.compare_against_time(self.LT_ins.timezone, '%Z', 8, "Testing against timezone failed")

    def test_date_time(self):
        """Check that LC_date_time, LC_date, and LC_time are correct."""
        strftime_output = time.strftime("%c", self.time_tuple)
        self.failUnless(strftime_output == time.strftime(self.LT_ins.LC_date_time, self.time_tuple), "LC_date_time incorrect")
        strftime_output = time.strftime("%x", self.time_tuple)
        self.failUnless(strftime_output == time.strftime(self.LT_ins.LC_date, self.time_tuple), "LC_date incorrect")
        strftime_output = time.strftime("%X", self.time_tuple)
        self.failUnless(strftime_output == time.strftime(self.LT_ins.LC_time, self.time_tuple), "LC_time incorrect")

    def test_lang(self):
        """Make sure lang is set."""
        self.failUnless(self.LT_ins.lang in (locale.getdefaultlocale()[0], locale.getlocale(locale.LC_TIME)), "Setting of lang failed")

    def test_by_hand_input(self):
        """Test passed-in initialization value checks."""
        self.failUnless(_strptime.LocaleTime(f_weekday=range(7)), "Argument size check for f_weekday failed")
        self.assertRaises(TypeError, _strptime.LocaleTime, f_weekday=range(8))
        self.assertRaises(TypeError, _strptime.LocaleTime, f_weekday=range(6))
        self.failUnless(_strptime.LocaleTime(a_weekday=range(7)), "Argument size check for a_weekday failed")
        self.assertRaises(TypeError, _strptime.LocaleTime, a_weekday=range(8))
        self.assertRaises(TypeError, _strptime.LocaleTime, a_weekday=range(6))
        self.failUnless(_strptime.LocaleTime(f_month=range(12)), "Argument size check for f_month failed")
        self.assertRaises(TypeError, _strptime.LocaleTime, f_month=range(11))
        self.assertRaises(TypeError, _strptime.LocaleTime, f_month=range(13))
        self.failUnless(len(_strptime.LocaleTime(f_month=range(12)).f_month) == 13, "dummy value for f_month not added")
        self.failUnless(_strptime.LocaleTime(a_month=range(12)), "Argument size check for a_month failed")
        self.assertRaises(TypeError, _strptime.LocaleTime, a_month=range(11))
        self.assertRaises(TypeError, _strptime.LocaleTime, a_month=range(13))
        self.failUnless(len(_strptime.LocaleTime(a_month=range(12)).a_month) == 13, "dummy value for a_month not added")
        self.failUnless(_strptime.LocaleTime(am_pm=range(2)), "Argument size check for am_pm failed")
        self.assertRaises(TypeError, _strptime.LocaleTime, am_pm=range(1))
        self.assertRaises(TypeError, _strptime.LocaleTime, am_pm=range(3))
        self.failUnless(_strptime.LocaleTime(timezone=range(2)), "Argument size check for timezone failed")
        self.assertRaises(TypeError, _strptime.LocaleTime, timezone=range(1))
        self.assertRaises(TypeError, _strptime.LocaleTime, timezone=range(3))

class TimeRETests(unittest.TestCase):
    """Tests for TimeRE."""

    def setUp(self):
        """Construct generic TimeRE object."""
        self.time_re = _strptime.TimeRE()
        self.locale_time = _strptime.LocaleTime()

    def test_getitem(self):
        """Make sure that __getitem__ works properly."""
        self.failUnless(self.time_re['m'], "Fetching 'm' directive (built-in) failed")
        self.failUnless(self.time_re['b'], "Fetching 'b' directive (built w/ __tupleToRE) failed")
        for name in self.locale_time.a_month:
            self.failUnless(self.time_re['b'].find(name) != -1, "Not all abbreviated month names in regex")
        self.failUnless(self.time_re['c'], "Fetching 'c' directive (built w/ format) failed")
        self.failUnless(self.time_re['c'].find('%') == -1, "Conversion of 'c' directive failed; '%' found")
        self.assertRaises(KeyError, self.time_re.__getitem__, '1')

    def test_pattern(self):
        """Test TimeRE.pattern."""
        pattern_string = self.time_re.pattern(r"%a %A %d")
        self.failUnless(pattern_string.find(self.locale_time.a_weekday[2]) != -1, "did not find abbreviated weekday in pattern string '%s'" % pattern_string)
        self.failUnless(pattern_string.find(self.locale_time.f_weekday[4]) != -1, "did not find full weekday in pattern string '%s'" % pattern_string)
        self.failUnless(pattern_string.find(self.time_re['d']) != -1, "did not find 'd' directive pattern string '%s'" % pattern_string)

    def test_compile(self):
        """Check that compiled regex is correct."""
        found = self.time_re.compile(r"%A").match(self.locale_time.f_weekday[6])
        self.failUnless(found and found.group('A') == self.locale_time.f_weekday[6], "re object for '%A' failed")
        compiled = self.time_re.compile(r"%a %b")
        found = compiled.match("%s %s" % (self.locale_time.a_weekday[4], self.locale_time.a_month[4]))
        self.failUnless(found, 
            "Match failed with '%s' regex and '%s' string" % (compiled.pattern, "%s %s" % (self.locale_time.a_weekday[4], self.locale_time.a_month[4])))
        self.failUnless(found.group('a') == self.locale_time.a_weekday[4] and found.group('b') == self.locale_time.a_month[4], 
            "re object couldn't find the abbreviated weekday month in '%s' using '%s'; group 'a' = '%s', group 'b' = %s'" % (found.string, found.re.pattern, found.group('a'), found.group('b')))
        for directive in ('a','A','b','B','c','d','H','I','j','m','M','p','S','U','w','W','x','X','y','Y','Z','%'):
            compiled = self.time_re.compile("%%%s"% directive)
            found = compiled.match(time.strftime("%%%s" % directive))
            self.failUnless(found, "Matching failed on '%s' using '%s' regex" % (time.strftime("%%%s" % directive), compiled.pattern))

class StrptimeTests(unittest.TestCase):
    """Tests for _strptime.strptime."""

    def setUp(self):
        """Create testing time tuple."""
        self.time_tuple = time.gmtime()

    def test_TypeError(self):
        """Make sure ValueError is raised when match fails."""
        self.assertRaises(ValueError,_strptime.strptime, data_string="%d", format="%A")

    def test_returning_RE(self):
        """Make sure that an re can be returned."""
        strp_output = _strptime.strptime(False, "%Y")
        self.failUnless(isinstance(strp_output, type(re.compile(''))), "re object not returned correctly")
        self.failUnless(_strptime.strptime("1999", strp_output), "Use or re object failed")
        bad_locale_time = _strptime.LocaleTime(lang="gibberish")
        self.assertRaises(TypeError, _strptime.strptime, data_string='1999', format=strp_output, locale_time=bad_locale_time)

    def helper(self, directive, position):
        """Helper fxn in testing."""
        strf_output = time.strftime("%%%s" % directive, self.time_tuple)
        strp_output = _strptime.strptime(strf_output, "%%%s" % directive)
        self.failUnless(strp_output[position] == self.time_tuple[position], "testing of '%s' directive failed; '%s' -> %s != %s" % (directive, strf_output, strp_output[position], self.time_tuple[position]))

    def test_year(self):
        """Test that the year is handled properly."""
        for directive in ('y', 'Y'):
            self.helper(directive, 0)

    def test_month(self):
        """Test for month directives."""
        for directive in ('B', 'b', 'm'):
            self.helper(directive, 1)

    def test_day(self):
        """Test for day directives."""
        self.helper('d', 2)

    def test_hour(self):
        """Test hour directives."""
        self.helper('H', 3)
        strf_output = time.strftime("%I %p", self.time_tuple)
        strp_output = _strptime.strptime(strf_output, "%I %p")
        self.failUnless(strp_output[3] == self.time_tuple[3], "testing of '%%I %%p' directive failed; '%s' -> %s != %s" % (strf_output, strp_output[3], self.time_tuple[3]))

    def test_minute(self):
        """Test minute directives."""
        self.helper('M', 4)

    def test_second(self):
        """Test second directives."""
        self.helper('S', 5)

    def test_weekday(self):
        """Test weekday directives."""
        for directive in ('A', 'a', 'w'):
            self.helper(directive,6)

    def test_julian(self):
        """Test julian directives."""
        self.helper('j', 7)

    def test_timezone(self):
        """Test timezone directives.
        
        When gmtime() is used with %Z, entire result of strftime() is empty.
        
        """
        time_tuple = time.localtime()
        strf_output = time.strftime("%Z")  #UTC does not have a timezone
        strp_output = _strptime.strptime(strf_output, "%Z")
        self.failUnless(strp_output[8] == time_tuple[8], "timezone check failed; '%s' -> %s != %s" % (strf_output, strp_output[8], time_tuple[8]))

    def test_date_time(self):
        """*** Test %c directive. ***"""
        for position in range(6):
            self.helper('c', position)

    def test_date(self):
        """*** Test %x directive. ***"""
        for position in range(0,3):
            self.helper('x', position)

    def test_time(self):
        """*** Test %X directive. ***"""
        for position in range(3,6):
            self.helper('X', position)

    def test_percent(self):
        """Make sure % signs are handled properly."""
        strf_output = time.strftime("%m %% %Y", self.time_tuple)
        strp_output = _strptime.strptime(strf_output, "%m %% %Y")
        self.failUnless(strp_output[0] == self.time_tuple[0] and strp_output[1] == self.time_tuple[1], "handling of percent sign failed")

class FxnTests(unittest.TestCase):
    """Test functions that fill in info by validating result and are triggered properly."""

    def setUp(self):
        """Create an initial time tuple."""
        self.time_tuple = time.gmtime()

    def test_julianday_result(self):
        """Test julianday."""
        result = _strptime.julianday(self.time_tuple[0], self.time_tuple[1], self.time_tuple[2])
        self.failUnless(result == self.time_tuple[7], "julianday failed; %s != %s" % (result, self.time_tuple[7]))

    def test_julianday_trigger(self):
        """Make sure julianday is called."""
        strf_output = time.strftime("%Y-%m-%d", self.time_tuple)
        strp_output = _strptime.strptime(strf_output, "%Y-%m-%d")
        self.failUnless(strp_output[7] == self.time_tuple[7], "strptime did not trigger julianday(); %s != %s" % (strp_output[7], self.time_tuple[7]))
        
    def test_gregorian_result(self):
        """Test gregorian."""
        result = _strptime.gregorian(self.time_tuple[7], self.time_tuple[0])
        comparison = [self.time_tuple[0], self.time_tuple[1], self.time_tuple[2]]
        self.failUnless(result == comparison, "gregorian() failed; %s != %s" % (result, comparison))

    def test_gregorian_trigger(self):
        """Test that gregorian() is triggered."""
        strf_output = time.strftime("%j %Y", self.time_tuple)
        strp_output = _strptime.strptime(strf_output, "%j %Y")
        self.failUnless(strp_output[1] == self.time_tuple[1] and strp_output[2] == self.time_tuple[2], "gregorian() not triggered; month -- %s != %s, day -- %s != %s" % (strp_output[1], self.time_tuple[1], strp_output[2], self.time_tuple[2]))

    def test_dayofweek_result(self):
        """Test dayofweek."""
        result = _strptime.dayofweek(self.time_tuple[0], self.time_tuple[1], self.time_tuple[2])
        comparison = self.time_tuple[6]
        self.failUnless(result == comparison, "dayofweek() failed; %s != %s" % (result, comparison))

    def test_dayofweek_trigger(self):
        """Make sure dayofweek() gets triggered."""
        strf_output = time.strftime("%Y-%m-%d", self.time_tuple)
        strp_output = _strptime.strptime(strf_output, "%Y-%m-%d")
        self.failUnless(strp_output[6] == self.time_tuple[6], "triggering of dayofweek() failed; %s != %s" % (strp_output[6], self.time_tuple[6]))





if __name__ == '__main__':
    unittest.main()
