from test import support
import time
import unittest
import locale
import sysconfig
import sys
import platform
try:
    import threading
except ImportError:
    threading = None

# Max year is only limited by the size of C int.
SIZEOF_INT = sysconfig.get_config_var('SIZEOF_INT') or 4
TIME_MAXYEAR = (1 << 8 * SIZEOF_INT - 1) - 1
TIME_MINYEAR = -TIME_MAXYEAR - 1


class TimeTestCase(unittest.TestCase):

    def setUp(self):
        self.t = time.time()

    def test_data_attributes(self):
        time.altzone
        time.daylight
        time.timezone
        time.tzname

    def test_time(self):
        time.time()
        info = time.get_clock_info('time')
        self.assertFalse(info.monotonic)
        self.assertTrue(info.adjustable)

    def test_clock(self):
        time.clock()

        info = time.get_clock_info('clock')
        self.assertTrue(info.monotonic)
        self.assertFalse(info.adjustable)

    @unittest.skipUnless(hasattr(time, 'clock_gettime'),
                         'need time.clock_gettime()')
    def test_clock_realtime(self):
        time.clock_gettime(time.CLOCK_REALTIME)

    @unittest.skipUnless(hasattr(time, 'clock_gettime'),
                         'need time.clock_gettime()')
    @unittest.skipUnless(hasattr(time, 'CLOCK_MONOTONIC'),
                         'need time.CLOCK_MONOTONIC')
    def test_clock_monotonic(self):
        a = time.clock_gettime(time.CLOCK_MONOTONIC)
        b = time.clock_gettime(time.CLOCK_MONOTONIC)
        self.assertLessEqual(a, b)

    @unittest.skipUnless(hasattr(time, 'clock_getres'),
                         'need time.clock_getres()')
    def test_clock_getres(self):
        res = time.clock_getres(time.CLOCK_REALTIME)
        self.assertGreater(res, 0.0)
        self.assertLessEqual(res, 1.0)

    @unittest.skipUnless(hasattr(time, 'clock_settime'),
                         'need time.clock_settime()')
    def test_clock_settime(self):
        t = time.clock_gettime(time.CLOCK_REALTIME)
        try:
            time.clock_settime(time.CLOCK_REALTIME, t)
        except PermissionError:
            pass

        if hasattr(time, 'CLOCK_MONOTONIC'):
            self.assertRaises(OSError,
                              time.clock_settime, time.CLOCK_MONOTONIC, 0)

    def test_conversions(self):
        self.assertEqual(time.ctime(self.t),
                         time.asctime(time.localtime(self.t)))
        self.assertEqual(int(time.mktime(time.localtime(self.t))),
                         int(self.t))

    def test_sleep(self):
        self.assertRaises(ValueError, time.sleep, -2)
        self.assertRaises(ValueError, time.sleep, -1)
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

        # Issue #10762: Guard against invalid/non-supported format string
        # so that Python don't crash (Windows crashes when the format string
        # input to [w]strftime is not kosher.
        if sys.platform.startswith('win'):
            with self.assertRaises(ValueError):
                time.strftime('%f')

    def _bounds_checking(self, func):
        # Make sure that strftime() checks the bounds of the various parts
        # of the time tuple (0 is valid for *all* values).

        # The year field is tested by other test cases above

        # Check month [1, 12] + zero support
        func((1900, 0, 1, 0, 0, 0, 0, 1, -1))
        func((1900, 12, 1, 0, 0, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, -1, 1, 0, 0, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 13, 1, 0, 0, 0, 0, 1, -1))
        # Check day of month [1, 31] + zero support
        func((1900, 1, 0, 0, 0, 0, 0, 1, -1))
        func((1900, 1, 31, 0, 0, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, -1, 0, 0, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 32, 0, 0, 0, 0, 1, -1))
        # Check hour [0, 23]
        func((1900, 1, 1, 23, 0, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, -1, 0, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 24, 0, 0, 0, 1, -1))
        # Check minute [0, 59]
        func((1900, 1, 1, 0, 59, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 0, -1, 0, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 0, 60, 0, 0, 1, -1))
        # Check second [0, 61]
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 0, 0, -1, 0, 1, -1))
        # C99 only requires allowing for one leap second, but Python's docs say
        # allow two leap seconds (0..61)
        func((1900, 1, 1, 0, 0, 60, 0, 1, -1))
        func((1900, 1, 1, 0, 0, 61, 0, 1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 0, 0, 62, 0, 1, -1))
        # No check for upper-bound day of week;
        #  value forced into range by a ``% 7`` calculation.
        # Start check at -2 since gettmarg() increments value before taking
        #  modulo.
        self.assertEqual(func((1900, 1, 1, 0, 0, 0, -1, 1, -1)),
                         func((1900, 1, 1, 0, 0, 0, +6, 1, -1)))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 0, 0, 0, -2, 1, -1))
        # Check day of the year [1, 366] + zero support
        func((1900, 1, 1, 0, 0, 0, 0, 0, -1))
        func((1900, 1, 1, 0, 0, 0, 0, 366, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 0, 0, 0, 0, -1, -1))
        self.assertRaises(ValueError, func,
                            (1900, 1, 1, 0, 0, 0, 0, 367, -1))

    def test_strftime_bounding_check(self):
        self._bounds_checking(lambda tup: time.strftime('', tup))

    def test_default_values_for_zero(self):
        # Make sure that using all zeros uses the proper default
        # values.  No test for daylight savings since strftime() does
        # not change output based on its value and no test for year
        # because systems vary in their support for year 0.
        expected = "2000 01 01 00 00 00 1 001"
        with support.check_warnings():
            result = time.strftime("%Y %m %d %H %M %S %w %j", (2000,)+(0,)*8)
        self.assertEqual(expected, result)

    def test_strptime(self):
        # Should be able to go round-trip from strftime to strptime without
        # raising an exception.
        tt = time.gmtime(self.t)
        for directive in ('a', 'A', 'b', 'B', 'c', 'd', 'H', 'I',
                          'j', 'm', 'M', 'p', 'S',
                          'U', 'w', 'W', 'x', 'X', 'y', 'Y', 'Z', '%'):
            format = '%' + directive
            strf_output = time.strftime(format, tt)
            try:
                time.strptime(strf_output, format)
            except ValueError:
                self.fail("conversion specifier %r failed with '%s' input." %
                          (format, strf_output))

    def test_strptime_bytes(self):
        # Make sure only strings are accepted as arguments to strptime.
        self.assertRaises(TypeError, time.strptime, b'2009', "%Y")
        self.assertRaises(TypeError, time.strptime, '2009', b'%Y')

    def test_strptime_exception_context(self):
        # check that this doesn't chain exceptions needlessly (see #17572)
        with self.assertRaises(ValueError) as e:
            time.strptime('', '%D')
        self.assertIs(e.exception.__suppress_context__, True)
        # additional check for IndexError branch (issue #19545)
        with self.assertRaises(ValueError) as e:
            time.strptime('19', '%Y %')
        self.assertIs(e.exception.__suppress_context__, True)

    def test_asctime(self):
        time.asctime(time.gmtime(self.t))

        # Max year is only limited by the size of C int.
        for bigyear in TIME_MAXYEAR, TIME_MINYEAR:
            asc = time.asctime((bigyear, 6, 1) + (0,) * 6)
            self.assertEqual(asc[-len(str(bigyear)):], str(bigyear))
        self.assertRaises(OverflowError, time.asctime,
                          (TIME_MAXYEAR + 1,) + (0,) * 8)
        self.assertRaises(OverflowError, time.asctime,
                          (TIME_MINYEAR - 1,) + (0,) * 8)
        self.assertRaises(TypeError, time.asctime, 0)
        self.assertRaises(TypeError, time.asctime, ())
        self.assertRaises(TypeError, time.asctime, (0,) * 10)

    def test_asctime_bounding_check(self):
        self._bounds_checking(time.asctime)

    def test_ctime(self):
        t = time.mktime((1973, 9, 16, 1, 3, 52, 0, 0, -1))
        self.assertEqual(time.ctime(t), 'Sun Sep 16 01:03:52 1973')
        t = time.mktime((2000, 1, 1, 0, 0, 0, 0, 0, -1))
        self.assertEqual(time.ctime(t), 'Sat Jan  1 00:00:00 2000')
        for year in [-100, 100, 1000, 2000, 10000]:
            try:
                testval = time.mktime((year, 1, 10) + (0,)*6)
            except (ValueError, OverflowError):
                # If mktime fails, ctime will fail too.  This may happen
                # on some platforms.
                pass
            else:
                self.assertEqual(time.ctime(testval)[20:], str(year))

    @unittest.skipUnless(hasattr(time, "tzset"),
                         "time module has no attribute tzset")
    def test_tzset(self):

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
            self.assertEqual(
                time.gmtime(xmas2002), time.localtime(xmas2002)
                )
            self.assertEqual(time.daylight, 0)
            self.assertEqual(time.timezone, 0)
            self.assertEqual(time.localtime(xmas2002).tm_isdst, 0)

            # Make sure we can switch to US/Eastern
            environ['TZ'] = eastern
            time.tzset()
            self.assertNotEqual(time.gmtime(xmas2002), time.localtime(xmas2002))
            self.assertEqual(time.tzname, ('EST', 'EDT'))
            self.assertEqual(len(time.tzname), 2)
            self.assertEqual(time.daylight, 1)
            self.assertEqual(time.timezone, 18000)
            self.assertEqual(time.altzone, 14400)
            self.assertEqual(time.localtime(xmas2002).tm_isdst, 0)
            self.assertEqual(len(time.tzname), 2)

            # Now go to the southern hemisphere.
            environ['TZ'] = victoria
            time.tzset()
            self.assertNotEqual(time.gmtime(xmas2002), time.localtime(xmas2002))

            # Issue #11886: Australian Eastern Standard Time (UTC+10) is called
            # "EST" (as Eastern Standard Time, UTC-5) instead of "AEST"
            # (non-DST timezone), and "EDT" instead of "AEDT" (DST timezone),
            # on some operating systems (e.g. FreeBSD), which is wrong. See for
            # example this bug:
            # http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=93810
            self.assertIn(time.tzname[0], ('AEST' 'EST'), time.tzname[0])
            self.assertTrue(time.tzname[1] in ('AEDT', 'EDT'), str(time.tzname[1]))
            self.assertEqual(len(time.tzname), 2)
            self.assertEqual(time.daylight, 1)
            self.assertEqual(time.timezone, -36000)
            self.assertEqual(time.altzone, -39600)
            self.assertEqual(time.localtime(xmas2002).tm_isdst, 1)

        finally:
            # Repair TZ environment variable in case any other tests
            # rely on it.
            if org_TZ is not None:
                environ['TZ'] = org_TZ
            elif 'TZ' in environ:
                del environ['TZ']
            time.tzset()

    def test_insane_timestamps(self):
        # It's possible that some platform maps time_t to double,
        # and that this test will fail there.  This test should
        # exempt such platforms (provided they return reasonable
        # results!).
        for func in time.ctime, time.gmtime, time.localtime:
            for unreasonable in -1e200, 1e200:
                self.assertRaises(OverflowError, func, unreasonable)

    def test_ctime_without_arg(self):
        # Not sure how to check the values, since the clock could tick
        # at any time.  Make sure these are at least accepted and
        # don't raise errors.
        time.ctime()
        time.ctime(None)

    def test_gmtime_without_arg(self):
        gt0 = time.gmtime()
        gt1 = time.gmtime(None)
        t0 = time.mktime(gt0)
        t1 = time.mktime(gt1)
        self.assertAlmostEqual(t1, t0, delta=0.2)

    def test_localtime_without_arg(self):
        lt0 = time.localtime()
        lt1 = time.localtime(None)
        t0 = time.mktime(lt0)
        t1 = time.mktime(lt1)
        self.assertAlmostEqual(t1, t0, delta=0.2)

    def test_mktime(self):
        # Issue #1726687
        for t in (-2, -1, 0, 1):
            try:
                tt = time.localtime(t)
            except (OverflowError, OSError):
                pass
            else:
                self.assertEqual(time.mktime(tt), t)

    # Issue #13309: passing extreme values to mktime() or localtime()
    # borks the glibc's internal timezone data.
    @unittest.skipUnless(platform.libc_ver()[0] != 'glibc',
                         "disabled because of a bug in glibc. Issue #13309")
    def test_mktime_error(self):
        # It may not be possible to reliably make mktime return error
        # on all platfom.  This will make sure that no other exception
        # than OverflowError is raised for an extreme value.
        tt = time.gmtime(self.t)
        tzname = time.strftime('%Z', tt)
        self.assertNotEqual(tzname, 'LMT')
        try:
            time.mktime((-1, 1, 1, 0, 0, 0, -1, -1, -1))
        except OverflowError:
            pass
        self.assertEqual(time.strftime('%Z', tt), tzname)

    @unittest.skipUnless(hasattr(time, 'monotonic'),
                         'need time.monotonic')
    def test_monotonic(self):
        # monotonic() should not go backward
        times = [time.monotonic() for n in range(100)]
        t1 = times[0]
        for t2 in times[1:]:
            self.assertGreaterEqual(t2, t1, "times=%s" % times)
            t1 = t2

        # monotonic() includes time elapsed during a sleep
        t1 = time.monotonic()
        time.sleep(0.5)
        t2 = time.monotonic()
        dt = t2 - t1
        self.assertGreater(t2, t1)
        # Issue #20101: On some Windows machines, dt may be slightly low
        self.assertTrue(0.45 <= dt <= 1.0, dt)

        # monotonic() is a monotonic but non adjustable clock
        info = time.get_clock_info('monotonic')
        self.assertTrue(info.monotonic)
        self.assertFalse(info.adjustable)

    def test_perf_counter(self):
        time.perf_counter()

    def test_process_time(self):
        # process_time() should not include time spend during a sleep
        start = time.process_time()
        time.sleep(0.100)
        stop = time.process_time()
        # use 20 ms because process_time() has usually a resolution of 15 ms
        # on Windows
        self.assertLess(stop - start, 0.020)

        info = time.get_clock_info('process_time')
        self.assertTrue(info.monotonic)
        self.assertFalse(info.adjustable)

    @unittest.skipUnless(hasattr(time, 'monotonic'),
                         'need time.monotonic')
    @unittest.skipUnless(hasattr(time, 'clock_settime'),
                         'need time.clock_settime')
    def test_monotonic_settime(self):
        t1 = time.monotonic()
        realtime = time.clock_gettime(time.CLOCK_REALTIME)
        # jump backward with an offset of 1 hour
        try:
            time.clock_settime(time.CLOCK_REALTIME, realtime - 3600)
        except PermissionError as err:
            self.skipTest(err)
        t2 = time.monotonic()
        time.clock_settime(time.CLOCK_REALTIME, realtime)
        # monotonic must not be affected by system clock updates
        self.assertGreaterEqual(t2, t1)

    def test_localtime_failure(self):
        # Issue #13847: check for localtime() failure
        invalid_time_t = None
        for time_t in (-1, 2**30, 2**33, 2**60):
            try:
                time.localtime(time_t)
            except OverflowError:
                self.skipTest("need 64-bit time_t")
            except OSError:
                invalid_time_t = time_t
                break
        if invalid_time_t is None:
            self.skipTest("unable to find an invalid time_t value")

        self.assertRaises(OSError, time.localtime, invalid_time_t)
        self.assertRaises(OSError, time.ctime, invalid_time_t)

    def test_get_clock_info(self):
        clocks = ['clock', 'perf_counter', 'process_time', 'time']
        if hasattr(time, 'monotonic'):
            clocks.append('monotonic')

        for name in clocks:
            info = time.get_clock_info(name)
            #self.assertIsInstance(info, dict)
            self.assertIsInstance(info.implementation, str)
            self.assertNotEqual(info.implementation, '')
            self.assertIsInstance(info.monotonic, bool)
            self.assertIsInstance(info.resolution, float)
            # 0.0 < resolution <= 1.0
            self.assertGreater(info.resolution, 0.0)
            self.assertLessEqual(info.resolution, 1.0)
            self.assertIsInstance(info.adjustable, bool)

        self.assertRaises(ValueError, time.get_clock_info, 'xxx')


class TestLocale(unittest.TestCase):
    def setUp(self):
        self.oldloc = locale.setlocale(locale.LC_ALL)

    def tearDown(self):
        locale.setlocale(locale.LC_ALL, self.oldloc)

    def test_bug_3061(self):
        try:
            tmp = locale.setlocale(locale.LC_ALL, "fr_FR")
        except locale.Error:
            self.skipTest('could not set locale.LC_ALL to fr_FR')
        # This should not cause an exception
        time.strftime("%B", (2009,2,1,0,0,0,0,0,0))


class _TestAsctimeYear:
    _format = '%d'

    def yearstr(self, y):
        return time.asctime((y,) + (0,) * 8).split()[-1]

    def test_large_year(self):
        # Check that it doesn't crash for year > 9999
        self.assertEqual(self.yearstr(12345), '12345')
        self.assertEqual(self.yearstr(123456789), '123456789')

class _TestStrftimeYear:

    # Issue 13305:  For years < 1000, the value is not always
    # padded to 4 digits across platforms.  The C standard
    # assumes year >= 1900, so it does not specify the number
    # of digits.

    if time.strftime('%Y', (1,) + (0,) * 8) == '0001':
        _format = '%04d'
    else:
        _format = '%d'

    def yearstr(self, y):
        return time.strftime('%Y', (y,) + (0,) * 8)

    def test_4dyear(self):
        # Check that we can return the zero padded value.
        if self._format == '%04d':
            self.test_year('%04d')
        else:
            def year4d(y):
                return time.strftime('%4Y', (y,) + (0,) * 8)
            self.test_year('%04d', func=year4d)

    def skip_if_not_supported(y):
        msg = "strftime() is limited to [1; 9999] with Visual Studio"
        # Check that it doesn't crash for year > 9999
        try:
            time.strftime('%Y', (y,) + (0,) * 8)
        except ValueError:
            cond = False
        else:
            cond = True
        return unittest.skipUnless(cond, msg)

    @skip_if_not_supported(10000)
    def test_large_year(self):
        return super().test_large_year()

    @skip_if_not_supported(0)
    def test_negative(self):
        return super().test_negative()

    del skip_if_not_supported


class _Test4dYear:
    _format = '%d'

    def test_year(self, fmt=None, func=None):
        fmt = fmt or self._format
        func = func or self.yearstr
        self.assertEqual(func(1),    fmt % 1)
        self.assertEqual(func(68),   fmt % 68)
        self.assertEqual(func(69),   fmt % 69)
        self.assertEqual(func(99),   fmt % 99)
        self.assertEqual(func(999),  fmt % 999)
        self.assertEqual(func(9999), fmt % 9999)

    def test_large_year(self):
        self.assertEqual(self.yearstr(12345), '12345')
        self.assertEqual(self.yearstr(123456789), '123456789')
        self.assertEqual(self.yearstr(TIME_MAXYEAR), str(TIME_MAXYEAR))
        self.assertRaises(OverflowError, self.yearstr, TIME_MAXYEAR + 1)

    def test_negative(self):
        self.assertEqual(self.yearstr(-1), self._format % -1)
        self.assertEqual(self.yearstr(-1234), '-1234')
        self.assertEqual(self.yearstr(-123456), '-123456')
        self.assertEqual(self.yearstr(-123456789), str(-123456789))
        self.assertEqual(self.yearstr(-1234567890), str(-1234567890))
        self.assertEqual(self.yearstr(TIME_MINYEAR + 1900), str(TIME_MINYEAR + 1900))
        # Issue #13312: it may return wrong value for year < TIME_MINYEAR + 1900
        # Skip the value test, but check that no error is raised
        self.yearstr(TIME_MINYEAR)
        # self.assertEqual(self.yearstr(TIME_MINYEAR), str(TIME_MINYEAR))
        self.assertRaises(OverflowError, self.yearstr, TIME_MINYEAR - 1)


class TestAsctime4dyear(_TestAsctimeYear, _Test4dYear, unittest.TestCase):
    pass

class TestStrftime4dyear(_TestStrftimeYear, _Test4dYear, unittest.TestCase):
    pass


class TestPytime(unittest.TestCase):
    def setUp(self):
        self.invalid_values = (
            -(2 ** 100), 2 ** 100,
            -(2.0 ** 100.0), 2.0 ** 100.0,
        )

    @support.cpython_only
    def test_time_t(self):
        from _testcapi import pytime_object_to_time_t
        for obj, time_t in (
            (0, 0),
            (-1, -1),
            (-1.0, -1),
            (-1.9, -1),
            (1.0, 1),
            (1.9, 1),
        ):
            self.assertEqual(pytime_object_to_time_t(obj), time_t)

        for invalid in self.invalid_values:
            self.assertRaises(OverflowError, pytime_object_to_time_t, invalid)

    @support.cpython_only
    def test_timeval(self):
        from _testcapi import pytime_object_to_timeval
        for obj, timeval in (
            (0, (0, 0)),
            (-1, (-1, 0)),
            (-1.0, (-1, 0)),
            (1e-6, (0, 1)),
            (-1e-6, (-1, 999999)),
            (-1.2, (-2, 800000)),
            (1.1234560, (1, 123456)),
            (1.1234569, (1, 123456)),
            (-1.1234560, (-2, 876544)),
            (-1.1234561, (-2, 876543)),
        ):
            self.assertEqual(pytime_object_to_timeval(obj), timeval)

        for invalid in self.invalid_values:
            self.assertRaises(OverflowError, pytime_object_to_timeval, invalid)

    @support.cpython_only
    def test_timespec(self):
        from _testcapi import pytime_object_to_timespec
        for obj, timespec in (
            (0, (0, 0)),
            (-1, (-1, 0)),
            (-1.0, (-1, 0)),
            (1e-9, (0, 1)),
            (-1e-9, (-1, 999999999)),
            (-1.2, (-2, 800000000)),
            (1.1234567890, (1, 123456789)),
            (1.1234567899, (1, 123456789)),
            (-1.1234567890, (-2, 876543211)),
            (-1.1234567891, (-2, 876543210)),
        ):
            self.assertEqual(pytime_object_to_timespec(obj), timespec)

        for invalid in self.invalid_values:
            self.assertRaises(OverflowError, pytime_object_to_timespec, invalid)

    @unittest.skipUnless(time._STRUCT_TM_ITEMS == 11, "needs tm_zone support")
    def test_localtime_timezone(self):

        # Get the localtime and examine it for the offset and zone.
        lt = time.localtime()
        self.assertTrue(hasattr(lt, "tm_gmtoff"))
        self.assertTrue(hasattr(lt, "tm_zone"))

        # See if the offset and zone are similar to the module
        # attributes.
        if lt.tm_gmtoff is None:
            self.assertTrue(not hasattr(time, "timezone"))
        else:
            self.assertEqual(lt.tm_gmtoff, -[time.timezone, time.altzone][lt.tm_isdst])
        if lt.tm_zone is None:
            self.assertTrue(not hasattr(time, "tzname"))
        else:
            self.assertEqual(lt.tm_zone, time.tzname[lt.tm_isdst])

        # Try and make UNIX times from the localtime and a 9-tuple
        # created from the localtime. Test to see that the times are
        # the same.
        t = time.mktime(lt); t9 = time.mktime(lt[:9])
        self.assertEqual(t, t9)

        # Make localtimes from the UNIX times and compare them to
        # the original localtime, thus making a round trip.
        new_lt = time.localtime(t); new_lt9 = time.localtime(t9)
        self.assertEqual(new_lt, lt)
        self.assertEqual(new_lt.tm_gmtoff, lt.tm_gmtoff)
        self.assertEqual(new_lt.tm_zone, lt.tm_zone)
        self.assertEqual(new_lt9, lt)
        self.assertEqual(new_lt.tm_gmtoff, lt.tm_gmtoff)
        self.assertEqual(new_lt9.tm_zone, lt.tm_zone)

    @unittest.skipUnless(time._STRUCT_TM_ITEMS == 11, "needs tm_zone support")
    def test_strptime_timezone(self):
        t = time.strptime("UTC", "%Z")
        self.assertEqual(t.tm_zone, 'UTC')
        t = time.strptime("+0500", "%z")
        self.assertEqual(t.tm_gmtoff, 5 * 3600)

    @unittest.skipUnless(time._STRUCT_TM_ITEMS == 11, "needs tm_zone support")
    def test_short_times(self):

        import pickle

        # Load a short time structure using pickle.
        st = b"ctime\nstruct_time\np0\n((I2007\nI8\nI11\nI1\nI24\nI49\nI5\nI223\nI1\ntp1\n(dp2\ntp3\nRp4\n."
        lt = pickle.loads(st)
        self.assertIs(lt.tm_gmtoff, None)
        self.assertIs(lt.tm_zone, None)


if __name__ == "__main__":
    unittest.main()
