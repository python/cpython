import datetime
from email import utils
import test.support
import time
import unittest
import sys
import os.path
import zoneinfo

class DateTimeTests(unittest.TestCase):

    datestring = 'Sun, 23 Sep 2001 20:10:55'
    dateargs = (2001, 9, 23, 20, 10, 55)
    offsetstring = ' -0700'
    utcoffset = datetime.timedelta(hours=-7)
    tz = datetime.timezone(utcoffset)
    naive_dt = datetime.datetime(*dateargs)
    aware_dt = datetime.datetime(*dateargs, tzinfo=tz)

    def test_naive_datetime(self):
        self.assertEqual(utils.format_datetime(self.naive_dt),
                         self.datestring + ' -0000')

    def test_aware_datetime(self):
        self.assertEqual(utils.format_datetime(self.aware_dt),
                         self.datestring + self.offsetstring)

    def test_usegmt(self):
        utc_dt = datetime.datetime(*self.dateargs,
                                   tzinfo=datetime.timezone.utc)
        self.assertEqual(utils.format_datetime(utc_dt, usegmt=True),
                         self.datestring + ' GMT')

    def test_usegmt_with_naive_datetime_raises(self):
        with self.assertRaises(ValueError):
            utils.format_datetime(self.naive_dt, usegmt=True)

    def test_usegmt_with_non_utc_datetime_raises(self):
        with self.assertRaises(ValueError):
            utils.format_datetime(self.aware_dt, usegmt=True)

    def test_parsedate_to_datetime(self):
        self.assertEqual(
            utils.parsedate_to_datetime(self.datestring + self.offsetstring),
            self.aware_dt)

    def test_parsedate_to_datetime_naive(self):
        self.assertEqual(
            utils.parsedate_to_datetime(self.datestring + ' -0000'),
            self.naive_dt)

    def test_parsedate_to_datetime_with_invalid_raises_valueerror(self):
        # See also test_parsedate_returns_None_for_invalid_strings in test_email.
        invalid_dates = [
            '',
            ' ',
            '0',
            'A Complete Waste of Time',
            'Wed, 3 Apr 2002 12.34.56.78+0800'
            'Tue, 06 Jun 2017 27:39:33 +0600',
            'Tue, 06 Jun 2017 07:39:33 +2600',
            'Tue, 06 Jun 2017 27:39:33',
            '17 June , 2022',
            'Friday, -Nov-82 16:14:55 EST',
            'Friday, Nov--82 16:14:55 EST',
            'Friday, 19-Nov- 16:14:55 EST',
        ]
        for dtstr in invalid_dates:
            with self.subTest(dtstr=dtstr):
                self.assertRaises(ValueError, utils.parsedate_to_datetime, dtstr)

class LocaltimeTests(unittest.TestCase):

    def test_localtime_is_tz_aware_daylight_true(self):
        test.support.patch(self, time, 'daylight', True)
        t = utils.localtime()
        self.assertIsNotNone(t.tzinfo)

    def test_localtime_is_tz_aware_daylight_false(self):
        test.support.patch(self, time, 'daylight', False)
        t = utils.localtime()
        self.assertIsNotNone(t.tzinfo)

    def test_localtime_daylight_true_dst_false(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    def test_localtime_daylight_false_dst_false(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    @test.support.run_with_tz('Europe/Minsk')
    def test_localtime_daylight_true_dst_true(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    @test.support.run_with_tz('Europe/Minsk')
    def test_localtime_daylight_false_dst_true(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    @test.support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_localtime_epoch_utc_daylight_true(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(1990, 1, 1, tzinfo = datetime.timezone.utc)
        t1 = utils.localtime(t0)
        t2 = t0 - datetime.timedelta(hours=5)
        t2 = t2.replace(tzinfo = datetime.timezone(datetime.timedelta(hours=-5)))
        self.assertEqual(t1, t2)

    @test.support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_localtime_epoch_utc_daylight_false(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(1990, 1, 1, tzinfo = datetime.timezone.utc)
        t1 = utils.localtime(t0)
        t2 = t0 - datetime.timedelta(hours=5)
        t2 = t2.replace(tzinfo = datetime.timezone(datetime.timedelta(hours=-5)))
        self.assertEqual(t1, t2)

    def test_localtime_epoch_notz_daylight_true(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(1990, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t0.replace(tzinfo=None))
        self.assertEqual(t1, t2)

    def test_localtime_epoch_notz_daylight_false(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(1990, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t0.replace(tzinfo=None))
        self.assertEqual(t1, t2)

    @test.support.run_with_tz('Europe/Kyiv')
    def test_variable_tzname(self):
        t0 = datetime.datetime(1984, 1, 1, tzinfo=datetime.timezone.utc)
        t1 = utils.localtime(t0)
        if t1.tzname() == 'Europe':
            self.skipTest("Can't find a Kyiv timezone database")
        self.assertEqual(t1.tzname(), 'MSK')
        t0 = datetime.datetime(1994, 1, 1, tzinfo=datetime.timezone.utc)
        t1 = utils.localtime(t0)
        self.assertEqual(t1.tzname(), 'EET')

    def test_isdst_deprecation(self):
        with self.assertWarns(DeprecationWarning):
            t0 = datetime.datetime(1990, 1, 1)
            t1 = utils.localtime(t0, isdst=True)

# Issue #24836: The timezone files are out of date (pre 2011k)
# on Mac OS X Snow Leopard.
@test.support.requires_mac_ver(10, 7)
class FormatDateTests(unittest.TestCase):

    @test.support.run_with_tz('Europe/Minsk')
    def test_formatdate(self):
        timeval = time.mktime((2011, 12, 1, 18, 0, 0, 4, 335, 0))
        string = utils.formatdate(timeval, localtime=False, usegmt=False)
        self.assertEqual(string, 'Thu, 01 Dec 2011 15:00:00 -0000')
        string = utils.formatdate(timeval, localtime=False, usegmt=True)
        self.assertEqual(string, 'Thu, 01 Dec 2011 15:00:00 GMT')

    @test.support.run_with_tz('Europe/Minsk')
    def test_formatdate_with_localtime(self):
        timeval = time.mktime((2011, 1, 1, 18, 0, 0, 6, 1, 0))
        string = utils.formatdate(timeval, localtime=True)
        self.assertEqual(string, 'Sat, 01 Jan 2011 18:00:00 +0200')
        # Minsk moved from +0200 (with DST) to +0300 (without DST) in 2011
        timeval = time.mktime((2011, 12, 1, 18, 0, 0, 4, 335, 0))
        string = utils.formatdate(timeval, localtime=True)
        self.assertEqual(string, 'Thu, 01 Dec 2011 18:00:00 +0300')

if __name__ == '__main__':
    unittest.main()
