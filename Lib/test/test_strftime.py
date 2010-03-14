"""
Unittest for time.strftime
"""

import calendar
import sys
import re
from test import support
import time
import unittest


# helper functions
def fixasctime(s):
    if s[8] == ' ':
        s = s[:8] + '0' + s[9:]
    return s

def escapestr(text, ampm):
    """
    Escape text to deal with possible locale values that have regex
    syntax while allowing regex syntax used for comparison.
    """
    new_text = re.escape(text)
    new_text = new_text.replace(re.escape(ampm), ampm)
    new_text = new_text.replace('\%', '%')
    new_text = new_text.replace('\:', ':')
    new_text = new_text.replace('\?', '?')
    return new_text


class StrftimeTest(unittest.TestCase):

    def _update_variables(self, now):
        # we must update the local variables on every cycle
        self.gmt = time.gmtime(now)
        now = time.localtime(now)

        if now[3] < 12: self.ampm='(AM|am)'
        else: self.ampm='(PM|pm)'

        self.jan1 = time.localtime(time.mktime((now[0], 1, 1, 0, 0, 0, 0, 1, 0)))

        try:
            if now[8]: self.tz = time.tzname[1]
            else: self.tz = time.tzname[0]
        except AttributeError:
            self.tz = ''

        if now[3] > 12: self.clock12 = now[3] - 12
        elif now[3] > 0: self.clock12 = now[3]
        else: self.clock12 = 12

        self.now = now

    def setUp(self):
        try:
            import java
            java.util.Locale.setDefault(java.util.Locale.US)
        except ImportError:
            import locale
            locale.setlocale(locale.LC_TIME, 'C')

    def test_strftime(self):
        now = time.time()
        self._update_variables(now)
        self.strftest1(now)
        self.strftest2(now)

        if support.verbose:
            print("Strftime test, platform: %s, Python version: %s" % \
                  (sys.platform, sys.version.split()[0]))

        for j in range(-5, 5):
            for i in range(25):
                arg = now + (i+j*100)*23*3603
                self._update_variables(arg)
                self.strftest1(arg)
                self.strftest2(arg)

    def strftest1(self, now):
        if support.verbose:
            print("strftime test for", time.ctime(now))
        now = self.now
        # Make sure any characters that could be taken as regex syntax is
        # escaped in escapestr()
        expectations = (
            ('%a', calendar.day_abbr[now[6]], 'abbreviated weekday name'),
            ('%A', calendar.day_name[now[6]], 'full weekday name'),
            ('%b', calendar.month_abbr[now[1]], 'abbreviated month name'),
            ('%B', calendar.month_name[now[1]], 'full month name'),
            # %c see below
            ('%d', '%02d' % now[2], 'day of month as number (00-31)'),
            ('%H', '%02d' % now[3], 'hour (00-23)'),
            ('%I', '%02d' % self.clock12, 'hour (01-12)'),
            ('%j', '%03d' % now[7], 'julian day (001-366)'),
            ('%m', '%02d' % now[1], 'month as number (01-12)'),
            ('%M', '%02d' % now[4], 'minute, (00-59)'),
            ('%p', self.ampm, 'AM or PM as appropriate'),
            ('%S', '%02d' % now[5], 'seconds of current time (00-60)'),
            ('%U', '%02d' % ((now[7] + self.jan1[6])//7),
             'week number of the year (Sun 1st)'),
            ('%w', '0?%d' % ((1+now[6]) % 7), 'weekday as a number (Sun 1st)'),
            ('%W', '%02d' % ((now[7] + (self.jan1[6] - 1)%7)//7),
            'week number of the year (Mon 1st)'),
            # %x see below
            ('%X', '%02d:%02d:%02d' % (now[3], now[4], now[5]), '%H:%M:%S'),
            ('%y', '%02d' % (now[0]%100), 'year without century'),
            ('%Y', '%d' % now[0], 'year with century'),
            # %Z see below
            ('%%', '%', 'single percent sign'),
        )

        for e in expectations:
            # musn't raise a value error
            try:
                result = time.strftime(e[0], now)
            except ValueError as error:
                self.fail("strftime '%s' format gave error: %s" % (e[0], error))
            if re.match(escapestr(e[1], self.ampm), result):
                continue
            if not result or result[0] == '%':
                self.fail("strftime does not support standard '%s' format (%s)"
                          % (e[0], e[2]))
            else:
                self.fail("Conflict for %s (%s): expected %s, but got %s"
                          % (e[0], e[2], e[1], result))

    def strftest2(self, now):
        nowsecs = str(int(now))[:-1]
        now = self.now

        nonstandard_expectations = (
        # These are standard but don't have predictable output
            ('%c', fixasctime(time.asctime(now)), 'near-asctime() format'),
            ('%x', '%02d/%02d/%02d' % (now[1], now[2], (now[0]%100)),
            '%m/%d/%y %H:%M:%S'),
            ('%Z', '%s' % self.tz, 'time zone name'),

            # These are some platform specific extensions
            ('%D', '%02d/%02d/%02d' % (now[1], now[2], (now[0]%100)), 'mm/dd/yy'),
            ('%e', '%2d' % now[2], 'day of month as number, blank padded ( 0-31)'),
            ('%h', calendar.month_abbr[now[1]], 'abbreviated month name'),
            ('%k', '%2d' % now[3], 'hour, blank padded ( 0-23)'),
            ('%n', '\n', 'newline character'),
            ('%r', '%02d:%02d:%02d %s' % (self.clock12, now[4], now[5], self.ampm),
            '%I:%M:%S %p'),
            ('%R', '%02d:%02d' % (now[3], now[4]), '%H:%M'),
            ('%s', nowsecs, 'seconds since the Epoch in UCT'),
            ('%t', '\t', 'tab character'),
            ('%T', '%02d:%02d:%02d' % (now[3], now[4], now[5]), '%H:%M:%S'),
            ('%3y', '%03d' % (now[0]%100),
            'year without century rendered using fieldwidth'),
        )


        for e in nonstandard_expectations:
            try:
                result = time.strftime(e[0], now)
            except ValueError as result:
                msg = "Error for nonstandard '%s' format (%s): %s" % \
                      (e[0], e[2], str(result))
                if support.verbose:
                    print(msg)
                continue
            if re.match(escapestr(e[1], self.ampm), result):
                if support.verbose:
                    print("Supports nonstandard '%s' format (%s)" % (e[0], e[2]))
            elif not result or result[0] == '%':
                if support.verbose:
                    print("Does not appear to support '%s' format (%s)" % \
                           (e[0], e[2]))
            else:
                if support.verbose:
                    print("Conflict for nonstandard '%s' format (%s):" % \
                           (e[0], e[2]))
                    print("  Expected %s, but got %s" % (e[1], result))

def test_main():
    support.run_unittest(StrftimeTest)

if __name__ == '__main__':
    test_main()
