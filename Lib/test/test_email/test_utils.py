import datetime
from email import utils
import test.support
import time
import unittest

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


class LocaltimeTests(unittest.TestCase):

    def test_localtime_is_tz_aware_daylight_true(self):
        test.support.patch(self, time, 'daylight', True)
        t = utils.localtime()
        self.assertIsNot(t.tzinfo, None)

    def test_localtime_is_tz_aware_daylight_false(self):
        test.support.patch(self, time, 'daylight', False)
        t = utils.localtime()
        self.assertIsNot(t.tzinfo, None)

    def test_localtime_daylight_true_dst_false(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0, isdst=-1)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    def test_localtime_daylight_false_dst_false(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0, isdst=-1)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    def test_localtime_daylight_true_dst_true(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0, isdst=1)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    def test_localtime_daylight_false_dst_true(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(2012, 3, 12, 1, 1)
        t1 = utils.localtime(t0, isdst=1)
        t2 = utils.localtime(t1)
        self.assertEqual(t1, t2)

    def test_localtime_epoch_utc_daylight_true(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(1970, 1, 1, tzinfo = datetime.timezone.utc)
        t1 = utils.localtime(t0)
        self.assertEqual(t0, t1)

    def test_localtime_epoch_utc_daylight_false(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(1970, 1, 1, tzinfo = datetime.timezone.utc)
        t1 = utils.localtime(t0)
        self.assertEqual(t0, t1)

    def test_localtime_epoch_notz_daylight_true(self):
        test.support.patch(self, time, 'daylight', True)
        t0 = datetime.datetime(1970, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t0.replace(tzinfo=None))
        self.assertEqual(t1, t2)

    def test_localtime_epoch_notz_daylight_false(self):
        test.support.patch(self, time, 'daylight', False)
        t0 = datetime.datetime(1970, 1, 1)
        t1 = utils.localtime(t0)
        t2 = utils.localtime(t0.replace(tzinfo=None))
        self.assertEqual(t1, t2)


if __name__ == '__main__':
    unittest.main()
