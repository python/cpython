from test import test_support
import time
import unittest


class TimeTestCase(unittest.TestCase):

    def setUp(self):
        self.t = time.time()

    def test_data_attributes(self):
        time.altzone
        time.daylight
        time.timezone
        time.tzname

    def test_clock(self):
        time.clock()

    def test_conversions(self):
        self.assert_(time.ctime(self.t)
                     == time.asctime(time.localtime(self.t)))
        self.assert_(long(time.mktime(time.localtime(self.t)))
                     == long(self.t))

    def test_sleep(self):
        time.sleep(1.2)

    def test_strftime(self):
        tt = time.gmtime(self.t)
        for directive in ('a', 'A', 'b', 'B', 'c', 'd', 'H', 'I',
                          'j', 'm', 'M', 'p', 'S',
                          'U', 'w', 'W', 'x', 'X', 'y', 'Y', 'Z', '%'):
            format = ' %' + directive
            try:
                time.strftime(format, tt)
            except ValueError:
                self.fail('conversion specifier: %r failed.' % format)

    def test_strptime(self):
        tt = time.gmtime(self.t)
        for directive in ('a', 'A', 'b', 'B', 'c', 'd', 'H', 'I',
                          'j', 'm', 'M', 'p', 'S',
                          'U', 'w', 'W', 'x', 'X', 'y', 'Y', 'Z', '%'):
            format = ' %' + directive
            try:
                time.strptime(time.strftime(format, tt), format)
            except ValueError:
                self.fail('conversion specifier: %r failed.' % format)

    def test_asctime(self):
        time.asctime(time.gmtime(self.t))
        self.assertRaises(TypeError, time.asctime, 0)

    def test_tzset(self):
        if not hasattr(time, "tzset"):
            return # Can't test this; don't want the test suite to fail

        from os import environ

        # Epoch time of midnight Dec 25th 2002. Never DST in northern
        # hemisphere.
        xmas2002 = 1040774400.0

        # These formats are correct for 2002, and possibly future years
        # This format is the 'standard' as documented at:
        # http://www.opengroup.org/onlinepubs/007904975/basedefs/xbd_chap08.html
        # They are also documented in the tzset(3) man page on most Unix
        # systems.
        eastern = 'EST+05EDT,M4.1.0,M10.5.0'
        victoria = 'AEST-10AEDT-11,M10.5.0,M3.5.0'
        utc='UTC+0'

        org_TZ = environ.get('TZ',None)
        try:
            # Make sure we can switch to UTC time and results are correct
            # Note that unknown timezones default to UTC.
            # Note that altzone is undefined in UTC, as there is no DST
            environ['TZ'] = eastern
            time.tzset()
            environ['TZ'] = utc
            time.tzset()
            self.failUnlessEqual(
                time.gmtime(xmas2002), time.localtime(xmas2002)
                )
            self.failUnlessEqual(time.daylight, 0)
            self.failUnlessEqual(time.timezone, 0)
            self.failUnlessEqual(time.localtime(xmas2002).tm_isdst, 0)

            # Make sure we can switch to US/Eastern
            environ['TZ'] = eastern
            time.tzset()
            self.failIfEqual(time.gmtime(xmas2002), time.localtime(xmas2002))
            self.failUnlessEqual(time.tzname, ('EST', 'EDT'))
            self.failUnlessEqual(len(time.tzname), 2)
            self.failUnlessEqual(time.daylight, 1)
            self.failUnlessEqual(time.timezone, 18000)
            self.failUnlessEqual(time.altzone, 14400)
            self.failUnlessEqual(time.localtime(xmas2002).tm_isdst, 0)
            self.failUnlessEqual(len(time.tzname), 2)

            # Now go to the southern hemisphere.
            environ['TZ'] = victoria
            time.tzset()
            self.failIfEqual(time.gmtime(xmas2002), time.localtime(xmas2002))
            self.failUnless(time.tzname[0] == 'AEST', str(time.tzname[0]))
            self.failUnless(time.tzname[1] == 'AEDT', str(time.tzname[1]))
            self.failUnlessEqual(len(time.tzname), 2)
            self.failUnlessEqual(time.daylight, 1)
            self.failUnlessEqual(time.timezone, -36000)
            self.failUnlessEqual(time.altzone, -39600)
            self.failUnlessEqual(time.localtime(xmas2002).tm_isdst, 1)

        finally:
            # Repair TZ environment variable in case any other tests
            # rely on it.
            if org_TZ is not None:
                environ['TZ'] = org_TZ
            elif environ.has_key('TZ'):
                del environ['TZ']
            time.tzset()


def test_main():
    test_support.run_unittest(TimeTestCase)


if __name__ == "__main__":
    test_main()
