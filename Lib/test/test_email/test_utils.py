import datetime
from email import utils
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
