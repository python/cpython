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

        org_TZ = environ.get('TZ',None)
        try:

            # Make sure we can switch to UTC time and results are correct
            # Note that unknown timezones default to UTC.
            for tz in ('UTC','GMT','Luna/Tycho'):
                environ['TZ'] = 'US/Eastern'
                time.tzset()
                environ['TZ'] = tz
                time.tzset()
                self.failUnlessEqual(
                    time.gmtime(xmas2002),time.localtime(xmas2002)
                    )
                self.failUnlessEqual(time.timezone,time.altzone)
                self.failUnlessEqual(time.daylight,0) 
                self.failUnlessEqual(time.timezone,0)
                self.failUnlessEqual(time.altzone,0)
                self.failUnlessEqual(time.localtime(xmas2002).tm_isdst,0)

            # Make sure we can switch to US/Eastern
            environ['TZ'] = 'US/Eastern'
            time.tzset()
            self.failIfEqual(time.gmtime(xmas2002),time.localtime(xmas2002))
            self.failUnlessEqual(time.tzname,('EST','EDT'))
            self.failUnlessEqual(len(time.tzname),2)
            self.failUnlessEqual(time.daylight,1)
            self.failUnlessEqual(time.timezone,18000)
            self.failUnlessEqual(time.altzone,14400)
            self.failUnlessEqual(time.localtime(xmas2002).tm_isdst,0)
            self.failUnlessEqual(len(time.tzname),2)

            # Now go to the southern hemisphere. We want somewhere all OS's
            # know about that has DST.
            environ['TZ'] = 'Australia/Melbourne'
            time.tzset()
            self.failIfEqual(time.gmtime(xmas2002),time.localtime(xmas2002))
            self.failUnless(time.tzname[0] in ('EST','AEST'))
            self.failUnless(time.tzname[1] in ('EST','EDT','AEDT'))
            self.failUnlessEqual(len(time.tzname),2)
            self.failUnlessEqual(time.daylight,1)
            self.failUnlessEqual(time.timezone,-36000)
            self.failUnlessEqual(time.altzone,-39600)
            self.failUnlessEqual(time.localtime(xmas2002).tm_isdst,1)

            # Get some times from a timezone that isn't wallclock timezone
            del environ['TZ']
            time.tzset()
            if time.timezone == 0:
                environ['TZ'] = 'US/Eastern'
            else:
                environ['TZ'] = 'UTC'
            time.tzset()
            nonlocal = time.localtime(xmas2002)

            # Then the same time in wallclock timezone
            del environ['TZ']
            time.tzset()
            local = time.localtime(xmas2002)

            # And make sure they arn't the same
            self.failIfEqual(local,nonlocal) 

            # Do some basic sanity checking after wallclock time set
            self.failUnlessEqual(len(time.tzname),2)
            time.daylight
            time.timezone
            time.altzone
        finally:
            # Repair TZ environment variable in case any other tests
            # rely on it.
            if org_TZ is not None:
                environ['TZ'] = org_TZ
            elif environ.has_key('TZ'):
                del environ['TZ']
            

def test_main():
    test_support.run_unittest(TimeTestCase)


if __name__ == "__main__":
    test_main()
