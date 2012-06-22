"""Test date/time type.

See http://www.zope.org/Members/fdrake/DateTimeWiki/TestCases
"""

import sys
import pickle
import unittest

from operator import lt, le, gt, ge, eq, ne, truediv, floordiv, mod

from test import support

import datetime as datetime_module
from datetime import MINYEAR, MAXYEAR
from datetime import timedelta
from datetime import tzinfo
from datetime import time
from datetime import timezone
from datetime import date, datetime
import time as _time

# Needed by test_datetime
import _strptime
#


pickle_choices = [(pickle, pickle, proto)
                  for proto in range(pickle.HIGHEST_PROTOCOL + 1)]
assert len(pickle_choices) == pickle.HIGHEST_PROTOCOL + 1

# An arbitrary collection of objects of non-datetime types, for testing
# mixed-type comparisons.
OTHERSTUFF = (10, 34.5, "abc", {}, [], ())


# XXX Copied from test_float.
INF = float("inf")
NAN = float("nan")


#############################################################################
# module tests

class TestModule(unittest.TestCase):

    def test_constants(self):
        datetime = datetime_module
        self.assertEqual(datetime.MINYEAR, 1)
        self.assertEqual(datetime.MAXYEAR, 9999)

#############################################################################
# tzinfo tests

class FixedOffset(tzinfo):

    def __init__(self, offset, name, dstoffset=42):
        if isinstance(offset, int):
            offset = timedelta(minutes=offset)
        if isinstance(dstoffset, int):
            dstoffset = timedelta(minutes=dstoffset)
        self.__offset = offset
        self.__name = name
        self.__dstoffset = dstoffset
    def __repr__(self):
        return self.__name.lower()
    def utcoffset(self, dt):
        return self.__offset
    def tzname(self, dt):
        return self.__name
    def dst(self, dt):
        return self.__dstoffset

class PicklableFixedOffset(FixedOffset):

    def __init__(self, offset=None, name=None, dstoffset=None):
        FixedOffset.__init__(self, offset, name, dstoffset)

class TestTZInfo(unittest.TestCase):

    def test_non_abstractness(self):
        # In order to allow subclasses to get pickled, the C implementation
        # wasn't able to get away with having __init__ raise
        # NotImplementedError.
        useless = tzinfo()
        dt = datetime.max
        self.assertRaises(NotImplementedError, useless.tzname, dt)
        self.assertRaises(NotImplementedError, useless.utcoffset, dt)
        self.assertRaises(NotImplementedError, useless.dst, dt)

    def test_subclass_must_override(self):
        class NotEnough(tzinfo):
            def __init__(self, offset, name):
                self.__offset = offset
                self.__name = name
        self.assertTrue(issubclass(NotEnough, tzinfo))
        ne = NotEnough(3, "NotByALongShot")
        self.assertIsInstance(ne, tzinfo)

        dt = datetime.now()
        self.assertRaises(NotImplementedError, ne.tzname, dt)
        self.assertRaises(NotImplementedError, ne.utcoffset, dt)
        self.assertRaises(NotImplementedError, ne.dst, dt)

    def test_normal(self):
        fo = FixedOffset(3, "Three")
        self.assertIsInstance(fo, tzinfo)
        for dt in datetime.now(), None:
            self.assertEqual(fo.utcoffset(dt), timedelta(minutes=3))
            self.assertEqual(fo.tzname(dt), "Three")
            self.assertEqual(fo.dst(dt), timedelta(minutes=42))

    def test_pickling_base(self):
        # There's no point to pickling tzinfo objects on their own (they
        # carry no data), but they need to be picklable anyway else
        # concrete subclasses can't be pickled.
        orig = tzinfo.__new__(tzinfo)
        self.assertTrue(type(orig) is tzinfo)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertTrue(type(derived) is tzinfo)

    def test_pickling_subclass(self):
        # Make sure we can pickle/unpickle an instance of a subclass.
        offset = timedelta(minutes=-300)
        for otype, args in [
            (PicklableFixedOffset, (offset, 'cookie')),
            (timezone, (offset,)),
            (timezone, (offset, "EST"))]:
            orig = otype(*args)
            oname = orig.tzname(None)
            self.assertIsInstance(orig, tzinfo)
            self.assertIs(type(orig), otype)
            self.assertEqual(orig.utcoffset(None), offset)
            self.assertEqual(orig.tzname(None), oname)
            for pickler, unpickler, proto in pickle_choices:
                green = pickler.dumps(orig, proto)
                derived = unpickler.loads(green)
                self.assertIsInstance(derived, tzinfo)
                self.assertIs(type(derived), otype)
                self.assertEqual(derived.utcoffset(None), offset)
                self.assertEqual(derived.tzname(None), oname)

class TestTimeZone(unittest.TestCase):

    def setUp(self):
        self.ACDT = timezone(timedelta(hours=9.5), 'ACDT')
        self.EST = timezone(-timedelta(hours=5), 'EST')
        self.DT = datetime(2010, 1, 1)

    def test_str(self):
        for tz in [self.ACDT, self.EST, timezone.utc,
                   timezone.min, timezone.max]:
            self.assertEqual(str(tz), tz.tzname(None))

    def test_repr(self):
        datetime = datetime_module
        for tz in [self.ACDT, self.EST, timezone.utc,
                   timezone.min, timezone.max]:
            # test round-trip
            tzrep = repr(tz)
            self.assertEqual(tz, eval(tzrep))


    def test_class_members(self):
        limit = timedelta(hours=23, minutes=59)
        self.assertEqual(timezone.utc.utcoffset(None), ZERO)
        self.assertEqual(timezone.min.utcoffset(None), -limit)
        self.assertEqual(timezone.max.utcoffset(None), limit)


    def test_constructor(self):
        self.assertIs(timezone.utc, timezone(timedelta(0)))
        self.assertIsNot(timezone.utc, timezone(timedelta(0), 'UTC'))
        self.assertEqual(timezone.utc, timezone(timedelta(0), 'UTC'))
        # invalid offsets
        for invalid in [timedelta(microseconds=1), timedelta(1, 1),
                        timedelta(seconds=1), timedelta(1), -timedelta(1)]:
            self.assertRaises(ValueError, timezone, invalid)
            self.assertRaises(ValueError, timezone, -invalid)

        with self.assertRaises(TypeError): timezone(None)
        with self.assertRaises(TypeError): timezone(42)
        with self.assertRaises(TypeError): timezone(ZERO, None)
        with self.assertRaises(TypeError): timezone(ZERO, 42)
        with self.assertRaises(TypeError): timezone(ZERO, 'ABC', 'extra')

    def test_inheritance(self):
        self.assertIsInstance(timezone.utc, tzinfo)
        self.assertIsInstance(self.EST, tzinfo)

    def test_utcoffset(self):
        dummy = self.DT
        for h in [0, 1.5, 12]:
            offset = h * HOUR
            self.assertEqual(offset, timezone(offset).utcoffset(dummy))
            self.assertEqual(-offset, timezone(-offset).utcoffset(dummy))

        with self.assertRaises(TypeError): self.EST.utcoffset('')
        with self.assertRaises(TypeError): self.EST.utcoffset(5)


    def test_dst(self):
        self.assertIsNone(timezone.utc.dst(self.DT))

        with self.assertRaises(TypeError): self.EST.dst('')
        with self.assertRaises(TypeError): self.EST.dst(5)

    def test_tzname(self):
        self.assertEqual('UTC+00:00', timezone(ZERO).tzname(None))
        self.assertEqual('UTC-05:00', timezone(-5 * HOUR).tzname(None))
        self.assertEqual('UTC+09:30', timezone(9.5 * HOUR).tzname(None))
        self.assertEqual('UTC-00:01', timezone(timedelta(minutes=-1)).tzname(None))
        self.assertEqual('XYZ', timezone(-5 * HOUR, 'XYZ').tzname(None))

        with self.assertRaises(TypeError): self.EST.tzname('')
        with self.assertRaises(TypeError): self.EST.tzname(5)

    def test_fromutc(self):
        with self.assertRaises(ValueError):
            timezone.utc.fromutc(self.DT)
        with self.assertRaises(TypeError):
            timezone.utc.fromutc('not datetime')
        for tz in [self.EST, self.ACDT, Eastern]:
            utctime = self.DT.replace(tzinfo=tz)
            local = tz.fromutc(utctime)
            self.assertEqual(local - utctime, tz.utcoffset(local))
            self.assertEqual(local,
                             self.DT.replace(tzinfo=timezone.utc))

    def test_comparison(self):
        self.assertNotEqual(timezone(ZERO), timezone(HOUR))
        self.assertEqual(timezone(HOUR), timezone(HOUR))
        self.assertEqual(timezone(-5 * HOUR), timezone(-5 * HOUR, 'EST'))
        with self.assertRaises(TypeError): timezone(ZERO) < timezone(ZERO)
        self.assertIn(timezone(ZERO), {timezone(ZERO)})

    def test_aware_datetime(self):
        # test that timezone instances can be used by datetime
        t = datetime(1, 1, 1)
        for tz in [timezone.min, timezone.max, timezone.utc]:
            self.assertEqual(tz.tzname(t),
                             t.replace(tzinfo=tz).tzname())
            self.assertEqual(tz.utcoffset(t),
                             t.replace(tzinfo=tz).utcoffset())
            self.assertEqual(tz.dst(t),
                             t.replace(tzinfo=tz).dst())

#############################################################################
# Base clase for testing a particular aspect of timedelta, time, date and
# datetime comparisons.

class HarmlessMixedComparison:
    # Test that __eq__ and __ne__ don't complain for mixed-type comparisons.

    # Subclasses must define 'theclass', and theclass(1, 1, 1) must be a
    # legit constructor.

    def test_harmless_mixed_comparison(self):
        me = self.theclass(1, 1, 1)

        self.assertFalse(me == ())
        self.assertTrue(me != ())
        self.assertFalse(() == me)
        self.assertTrue(() != me)

        self.assertIn(me, [1, 20, [], me])
        self.assertIn([], [me, 1, 20, []])

    def test_harmful_mixed_comparison(self):
        me = self.theclass(1, 1, 1)

        self.assertRaises(TypeError, lambda: me < ())
        self.assertRaises(TypeError, lambda: me <= ())
        self.assertRaises(TypeError, lambda: me > ())
        self.assertRaises(TypeError, lambda: me >= ())

        self.assertRaises(TypeError, lambda: () < me)
        self.assertRaises(TypeError, lambda: () <= me)
        self.assertRaises(TypeError, lambda: () > me)
        self.assertRaises(TypeError, lambda: () >= me)

#############################################################################
# timedelta tests

class TestTimeDelta(HarmlessMixedComparison, unittest.TestCase):

    theclass = timedelta

    def test_constructor(self):
        eq = self.assertEqual
        td = timedelta

        # Check keyword args to constructor
        eq(td(), td(weeks=0, days=0, hours=0, minutes=0, seconds=0,
                    milliseconds=0, microseconds=0))
        eq(td(1), td(days=1))
        eq(td(0, 1), td(seconds=1))
        eq(td(0, 0, 1), td(microseconds=1))
        eq(td(weeks=1), td(days=7))
        eq(td(days=1), td(hours=24))
        eq(td(hours=1), td(minutes=60))
        eq(td(minutes=1), td(seconds=60))
        eq(td(seconds=1), td(milliseconds=1000))
        eq(td(milliseconds=1), td(microseconds=1000))

        # Check float args to constructor
        eq(td(weeks=1.0/7), td(days=1))
        eq(td(days=1.0/24), td(hours=1))
        eq(td(hours=1.0/60), td(minutes=1))
        eq(td(minutes=1.0/60), td(seconds=1))
        eq(td(seconds=0.001), td(milliseconds=1))
        eq(td(milliseconds=0.001), td(microseconds=1))

    def test_computations(self):
        eq = self.assertEqual
        td = timedelta

        a = td(7) # One week
        b = td(0, 60) # One minute
        c = td(0, 0, 1000) # One millisecond
        eq(a+b+c, td(7, 60, 1000))
        eq(a-b, td(6, 24*3600 - 60))
        eq(b.__rsub__(a), td(6, 24*3600 - 60))
        eq(-a, td(-7))
        eq(+a, td(7))
        eq(-b, td(-1, 24*3600 - 60))
        eq(-c, td(-1, 24*3600 - 1, 999000))
        eq(abs(a), a)
        eq(abs(-a), a)
        eq(td(6, 24*3600), a)
        eq(td(0, 0, 60*1000000), b)
        eq(a*10, td(70))
        eq(a*10, 10*a)
        eq(a*10, 10*a)
        eq(b*10, td(0, 600))
        eq(10*b, td(0, 600))
        eq(b*10, td(0, 600))
        eq(c*10, td(0, 0, 10000))
        eq(10*c, td(0, 0, 10000))
        eq(c*10, td(0, 0, 10000))
        eq(a*-1, -a)
        eq(b*-2, -b-b)
        eq(c*-2, -c+-c)
        eq(b*(60*24), (b*60)*24)
        eq(b*(60*24), (60*b)*24)
        eq(c*1000, td(0, 1))
        eq(1000*c, td(0, 1))
        eq(a//7, td(1))
        eq(b//10, td(0, 6))
        eq(c//1000, td(0, 0, 1))
        eq(a//10, td(0, 7*24*360))
        eq(a//3600000, td(0, 0, 7*24*1000))
        eq(a/0.5, td(14))
        eq(b/0.5, td(0, 120))
        eq(a/7, td(1))
        eq(b/10, td(0, 6))
        eq(c/1000, td(0, 0, 1))
        eq(a/10, td(0, 7*24*360))
        eq(a/3600000, td(0, 0, 7*24*1000))

        # Multiplication by float
        us = td(microseconds=1)
        eq((3*us) * 0.5, 2*us)
        eq((5*us) * 0.5, 2*us)
        eq(0.5 * (3*us), 2*us)
        eq(0.5 * (5*us), 2*us)
        eq((-3*us) * 0.5, -2*us)
        eq((-5*us) * 0.5, -2*us)

        # Division by int and float
        eq((3*us) / 2, 2*us)
        eq((5*us) / 2, 2*us)
        eq((-3*us) / 2.0, -2*us)
        eq((-5*us) / 2.0, -2*us)
        eq((3*us) / -2, -2*us)
        eq((5*us) / -2, -2*us)
        eq((3*us) / -2.0, -2*us)
        eq((5*us) / -2.0, -2*us)
        for i in range(-10, 10):
            eq((i*us/3)//us, round(i/3))
        for i in range(-10, 10):
            eq((i*us/-3)//us, round(i/-3))

        # Issue #11576
        eq(td(999999999, 86399, 999999) - td(999999999, 86399, 999998),
           td(0, 0, 1))
        eq(td(999999999, 1, 1) - td(999999999, 1, 0),
           td(0, 0, 1))

    def test_disallowed_computations(self):
        a = timedelta(42)

        # Add/sub ints or floats should be illegal
        for i in 1, 1.0:
            self.assertRaises(TypeError, lambda: a+i)
            self.assertRaises(TypeError, lambda: a-i)
            self.assertRaises(TypeError, lambda: i+a)
            self.assertRaises(TypeError, lambda: i-a)

        # Division of int by timedelta doesn't make sense.
        # Division by zero doesn't make sense.
        zero = 0
        self.assertRaises(TypeError, lambda: zero // a)
        self.assertRaises(ZeroDivisionError, lambda: a // zero)
        self.assertRaises(ZeroDivisionError, lambda: a / zero)
        self.assertRaises(ZeroDivisionError, lambda: a / 0.0)
        self.assertRaises(TypeError, lambda: a / '')

    @support.requires_IEEE_754
    def test_disallowed_special(self):
        a = timedelta(42)
        self.assertRaises(ValueError, a.__mul__, NAN)
        self.assertRaises(ValueError, a.__truediv__, NAN)

    def test_basic_attributes(self):
        days, seconds, us = 1, 7, 31
        td = timedelta(days, seconds, us)
        self.assertEqual(td.days, days)
        self.assertEqual(td.seconds, seconds)
        self.assertEqual(td.microseconds, us)

    def test_total_seconds(self):
        td = timedelta(days=365)
        self.assertEqual(td.total_seconds(), 31536000.0)
        for total_seconds in [123456.789012, -123456.789012, 0.123456, 0, 1e6]:
            td = timedelta(seconds=total_seconds)
            self.assertEqual(td.total_seconds(), total_seconds)
        # Issue8644: Test that td.total_seconds() has the same
        # accuracy as td / timedelta(seconds=1).
        for ms in [-1, -2, -123]:
            td = timedelta(microseconds=ms)
            self.assertEqual(td.total_seconds(), td / timedelta(seconds=1))

    def test_carries(self):
        t1 = timedelta(days=100,
                       weeks=-7,
                       hours=-24*(100-49),
                       minutes=-3,
                       seconds=12,
                       microseconds=(3*60 - 12) * 1e6 + 1)
        t2 = timedelta(microseconds=1)
        self.assertEqual(t1, t2)

    def test_hash_equality(self):
        t1 = timedelta(days=100,
                       weeks=-7,
                       hours=-24*(100-49),
                       minutes=-3,
                       seconds=12,
                       microseconds=(3*60 - 12) * 1000000)
        t2 = timedelta()
        self.assertEqual(hash(t1), hash(t2))

        t1 += timedelta(weeks=7)
        t2 += timedelta(days=7*7)
        self.assertEqual(t1, t2)
        self.assertEqual(hash(t1), hash(t2))

        d = {t1: 1}
        d[t2] = 2
        self.assertEqual(len(d), 1)
        self.assertEqual(d[t1], 2)

    def test_pickling(self):
        args = 12, 34, 56
        orig = timedelta(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

    def test_compare(self):
        t1 = timedelta(2, 3, 4)
        t2 = timedelta(2, 3, 4)
        self.assertEqual(t1, t2)
        self.assertTrue(t1 <= t2)
        self.assertTrue(t1 >= t2)
        self.assertTrue(not t1 != t2)
        self.assertTrue(not t1 < t2)
        self.assertTrue(not t1 > t2)

        for args in (3, 3, 3), (2, 4, 4), (2, 3, 5):
            t2 = timedelta(*args)   # this is larger than t1
            self.assertTrue(t1 < t2)
            self.assertTrue(t2 > t1)
            self.assertTrue(t1 <= t2)
            self.assertTrue(t2 >= t1)
            self.assertTrue(t1 != t2)
            self.assertTrue(t2 != t1)
            self.assertTrue(not t1 == t2)
            self.assertTrue(not t2 == t1)
            self.assertTrue(not t1 > t2)
            self.assertTrue(not t2 < t1)
            self.assertTrue(not t1 >= t2)
            self.assertTrue(not t2 <= t1)

        for badarg in OTHERSTUFF:
            self.assertEqual(t1 == badarg, False)
            self.assertEqual(t1 != badarg, True)
            self.assertEqual(badarg == t1, False)
            self.assertEqual(badarg != t1, True)

            self.assertRaises(TypeError, lambda: t1 <= badarg)
            self.assertRaises(TypeError, lambda: t1 < badarg)
            self.assertRaises(TypeError, lambda: t1 > badarg)
            self.assertRaises(TypeError, lambda: t1 >= badarg)
            self.assertRaises(TypeError, lambda: badarg <= t1)
            self.assertRaises(TypeError, lambda: badarg < t1)
            self.assertRaises(TypeError, lambda: badarg > t1)
            self.assertRaises(TypeError, lambda: badarg >= t1)

    def test_str(self):
        td = timedelta
        eq = self.assertEqual

        eq(str(td(1)), "1 day, 0:00:00")
        eq(str(td(-1)), "-1 day, 0:00:00")
        eq(str(td(2)), "2 days, 0:00:00")
        eq(str(td(-2)), "-2 days, 0:00:00")

        eq(str(td(hours=12, minutes=58, seconds=59)), "12:58:59")
        eq(str(td(hours=2, minutes=3, seconds=4)), "2:03:04")
        eq(str(td(weeks=-30, hours=23, minutes=12, seconds=34)),
           "-210 days, 23:12:34")

        eq(str(td(milliseconds=1)), "0:00:00.001000")
        eq(str(td(microseconds=3)), "0:00:00.000003")

        eq(str(td(days=999999999, hours=23, minutes=59, seconds=59,
                   microseconds=999999)),
           "999999999 days, 23:59:59.999999")

    def test_repr(self):
        name = 'datetime.' + self.theclass.__name__
        self.assertEqual(repr(self.theclass(1)),
                         "%s(1)" % name)
        self.assertEqual(repr(self.theclass(10, 2)),
                         "%s(10, 2)" % name)
        self.assertEqual(repr(self.theclass(-10, 2, 400000)),
                         "%s(-10, 2, 400000)" % name)

    def test_roundtrip(self):
        for td in (timedelta(days=999999999, hours=23, minutes=59,
                             seconds=59, microseconds=999999),
                   timedelta(days=-999999999),
                   timedelta(days=-999999999, seconds=1),
                   timedelta(days=1, seconds=2, microseconds=3)):

            # Verify td -> string -> td identity.
            s = repr(td)
            self.assertTrue(s.startswith('datetime.'))
            s = s[9:]
            td2 = eval(s)
            self.assertEqual(td, td2)

            # Verify identity via reconstructing from pieces.
            td2 = timedelta(td.days, td.seconds, td.microseconds)
            self.assertEqual(td, td2)

    def test_resolution_info(self):
        self.assertIsInstance(timedelta.min, timedelta)
        self.assertIsInstance(timedelta.max, timedelta)
        self.assertIsInstance(timedelta.resolution, timedelta)
        self.assertTrue(timedelta.max > timedelta.min)
        self.assertEqual(timedelta.min, timedelta(-999999999))
        self.assertEqual(timedelta.max, timedelta(999999999, 24*3600-1, 1e6-1))
        self.assertEqual(timedelta.resolution, timedelta(0, 0, 1))

    def test_overflow(self):
        tiny = timedelta.resolution

        td = timedelta.min + tiny
        td -= tiny  # no problem
        self.assertRaises(OverflowError, td.__sub__, tiny)
        self.assertRaises(OverflowError, td.__add__, -tiny)

        td = timedelta.max - tiny
        td += tiny  # no problem
        self.assertRaises(OverflowError, td.__add__, tiny)
        self.assertRaises(OverflowError, td.__sub__, -tiny)

        self.assertRaises(OverflowError, lambda: -timedelta.max)

        day = timedelta(1)
        self.assertRaises(OverflowError, day.__mul__, 10**9)
        self.assertRaises(OverflowError, day.__mul__, 1e9)
        self.assertRaises(OverflowError, day.__truediv__, 1e-20)
        self.assertRaises(OverflowError, day.__truediv__, 1e-10)
        self.assertRaises(OverflowError, day.__truediv__, 9e-10)

    @support.requires_IEEE_754
    def _test_overflow_special(self):
        day = timedelta(1)
        self.assertRaises(OverflowError, day.__mul__, INF)
        self.assertRaises(OverflowError, day.__mul__, -INF)

    def test_microsecond_rounding(self):
        td = timedelta
        eq = self.assertEqual

        # Single-field rounding.
        eq(td(milliseconds=0.4/1000), td(0))    # rounds to 0
        eq(td(milliseconds=-0.4/1000), td(0))    # rounds to 0
        eq(td(milliseconds=0.6/1000), td(microseconds=1))
        eq(td(milliseconds=-0.6/1000), td(microseconds=-1))

        # Rounding due to contributions from more than one field.
        us_per_hour = 3600e6
        us_per_day = us_per_hour * 24
        eq(td(days=.4/us_per_day), td(0))
        eq(td(hours=.2/us_per_hour), td(0))
        eq(td(days=.4/us_per_day, hours=.2/us_per_hour), td(microseconds=1))

        eq(td(days=-.4/us_per_day), td(0))
        eq(td(hours=-.2/us_per_hour), td(0))
        eq(td(days=-.4/us_per_day, hours=-.2/us_per_hour), td(microseconds=-1))

    def test_massive_normalization(self):
        td = timedelta(microseconds=-1)
        self.assertEqual((td.days, td.seconds, td.microseconds),
                         (-1, 24*3600-1, 999999))

    def test_bool(self):
        self.assertTrue(timedelta(1))
        self.assertTrue(timedelta(0, 1))
        self.assertTrue(timedelta(0, 0, 1))
        self.assertTrue(timedelta(microseconds=1))
        self.assertTrue(not timedelta(0))

    def test_subclass_timedelta(self):

        class T(timedelta):
            @staticmethod
            def from_td(td):
                return T(td.days, td.seconds, td.microseconds)

            def as_hours(self):
                sum = (self.days * 24 +
                       self.seconds / 3600.0 +
                       self.microseconds / 3600e6)
                return round(sum)

        t1 = T(days=1)
        self.assertTrue(type(t1) is T)
        self.assertEqual(t1.as_hours(), 24)

        t2 = T(days=-1, seconds=-3600)
        self.assertTrue(type(t2) is T)
        self.assertEqual(t2.as_hours(), -25)

        t3 = t1 + t2
        self.assertTrue(type(t3) is timedelta)
        t4 = T.from_td(t3)
        self.assertTrue(type(t4) is T)
        self.assertEqual(t3.days, t4.days)
        self.assertEqual(t3.seconds, t4.seconds)
        self.assertEqual(t3.microseconds, t4.microseconds)
        self.assertEqual(str(t3), str(t4))
        self.assertEqual(t4.as_hours(), -1)

    def test_division(self):
        t = timedelta(hours=1, minutes=24, seconds=19)
        second = timedelta(seconds=1)
        self.assertEqual(t / second, 5059.0)
        self.assertEqual(t // second, 5059)

        t = timedelta(minutes=2, seconds=30)
        minute = timedelta(minutes=1)
        self.assertEqual(t / minute, 2.5)
        self.assertEqual(t // minute, 2)

        zerotd = timedelta(0)
        self.assertRaises(ZeroDivisionError, truediv, t, zerotd)
        self.assertRaises(ZeroDivisionError, floordiv, t, zerotd)

        # self.assertRaises(TypeError, truediv, t, 2)
        # note: floor division of a timedelta by an integer *is*
        # currently permitted.

    def test_remainder(self):
        t = timedelta(minutes=2, seconds=30)
        minute = timedelta(minutes=1)
        r = t % minute
        self.assertEqual(r, timedelta(seconds=30))

        t = timedelta(minutes=-2, seconds=30)
        r = t %  minute
        self.assertEqual(r, timedelta(seconds=30))

        zerotd = timedelta(0)
        self.assertRaises(ZeroDivisionError, mod, t, zerotd)

        self.assertRaises(TypeError, mod, t, 10)

    def test_divmod(self):
        t = timedelta(minutes=2, seconds=30)
        minute = timedelta(minutes=1)
        q, r = divmod(t, minute)
        self.assertEqual(q, 2)
        self.assertEqual(r, timedelta(seconds=30))

        t = timedelta(minutes=-2, seconds=30)
        q, r = divmod(t, minute)
        self.assertEqual(q, -2)
        self.assertEqual(r, timedelta(seconds=30))

        zerotd = timedelta(0)
        self.assertRaises(ZeroDivisionError, divmod, t, zerotd)

        self.assertRaises(TypeError, divmod, t, 10)


#############################################################################
# date tests

class TestDateOnly(unittest.TestCase):
    # Tests here won't pass if also run on datetime objects, so don't
    # subclass this to test datetimes too.

    def test_delta_non_days_ignored(self):
        dt = date(2000, 1, 2)
        delta = timedelta(days=1, hours=2, minutes=3, seconds=4,
                          microseconds=5)
        days = timedelta(delta.days)
        self.assertEqual(days, timedelta(1))

        dt2 = dt + delta
        self.assertEqual(dt2, dt + days)

        dt2 = delta + dt
        self.assertEqual(dt2, dt + days)

        dt2 = dt - delta
        self.assertEqual(dt2, dt - days)

        delta = -delta
        days = timedelta(delta.days)
        self.assertEqual(days, timedelta(-2))

        dt2 = dt + delta
        self.assertEqual(dt2, dt + days)

        dt2 = delta + dt
        self.assertEqual(dt2, dt + days)

        dt2 = dt - delta
        self.assertEqual(dt2, dt - days)

class SubclassDate(date):
    sub_var = 1

class TestDate(HarmlessMixedComparison, unittest.TestCase):
    # Tests here should pass for both dates and datetimes, except for a
    # few tests that TestDateTime overrides.

    theclass = date

    def test_basic_attributes(self):
        dt = self.theclass(2002, 3, 1)
        self.assertEqual(dt.year, 2002)
        self.assertEqual(dt.month, 3)
        self.assertEqual(dt.day, 1)

    def test_roundtrip(self):
        for dt in (self.theclass(1, 2, 3),
                   self.theclass.today()):
            # Verify dt -> string -> date identity.
            s = repr(dt)
            self.assertTrue(s.startswith('datetime.'))
            s = s[9:]
            dt2 = eval(s)
            self.assertEqual(dt, dt2)

            # Verify identity via reconstructing from pieces.
            dt2 = self.theclass(dt.year, dt.month, dt.day)
            self.assertEqual(dt, dt2)

    def test_ordinal_conversions(self):
        # Check some fixed values.
        for y, m, d, n in [(1, 1, 1, 1),      # calendar origin
                           (1, 12, 31, 365),
                           (2, 1, 1, 366),
                           # first example from "Calendrical Calculations"
                           (1945, 11, 12, 710347)]:
            d = self.theclass(y, m, d)
            self.assertEqual(n, d.toordinal())
            fromord = self.theclass.fromordinal(n)
            self.assertEqual(d, fromord)
            if hasattr(fromord, "hour"):
            # if we're checking something fancier than a date, verify
            # the extra fields have been zeroed out
                self.assertEqual(fromord.hour, 0)
                self.assertEqual(fromord.minute, 0)
                self.assertEqual(fromord.second, 0)
                self.assertEqual(fromord.microsecond, 0)

        # Check first and last days of year spottily across the whole
        # range of years supported.
        for year in range(MINYEAR, MAXYEAR+1, 7):
            # Verify (year, 1, 1) -> ordinal -> y, m, d is identity.
            d = self.theclass(year, 1, 1)
            n = d.toordinal()
            d2 = self.theclass.fromordinal(n)
            self.assertEqual(d, d2)
            # Verify that moving back a day gets to the end of year-1.
            if year > 1:
                d = self.theclass.fromordinal(n-1)
                d2 = self.theclass(year-1, 12, 31)
                self.assertEqual(d, d2)
                self.assertEqual(d2.toordinal(), n-1)

        # Test every day in a leap-year and a non-leap year.
        dim = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
        for year, isleap in (2000, True), (2002, False):
            n = self.theclass(year, 1, 1).toordinal()
            for month, maxday in zip(range(1, 13), dim):
                if month == 2 and isleap:
                    maxday += 1
                for day in range(1, maxday+1):
                    d = self.theclass(year, month, day)
                    self.assertEqual(d.toordinal(), n)
                    self.assertEqual(d, self.theclass.fromordinal(n))
                    n += 1

    def test_extreme_ordinals(self):
        a = self.theclass.min
        a = self.theclass(a.year, a.month, a.day)  # get rid of time parts
        aord = a.toordinal()
        b = a.fromordinal(aord)
        self.assertEqual(a, b)

        self.assertRaises(ValueError, lambda: a.fromordinal(aord - 1))

        b = a + timedelta(days=1)
        self.assertEqual(b.toordinal(), aord + 1)
        self.assertEqual(b, self.theclass.fromordinal(aord + 1))

        a = self.theclass.max
        a = self.theclass(a.year, a.month, a.day)  # get rid of time parts
        aord = a.toordinal()
        b = a.fromordinal(aord)
        self.assertEqual(a, b)

        self.assertRaises(ValueError, lambda: a.fromordinal(aord + 1))

        b = a - timedelta(days=1)
        self.assertEqual(b.toordinal(), aord - 1)
        self.assertEqual(b, self.theclass.fromordinal(aord - 1))

    def test_bad_constructor_arguments(self):
        # bad years
        self.theclass(MINYEAR, 1, 1)  # no exception
        self.theclass(MAXYEAR, 1, 1)  # no exception
        self.assertRaises(ValueError, self.theclass, MINYEAR-1, 1, 1)
        self.assertRaises(ValueError, self.theclass, MAXYEAR+1, 1, 1)
        # bad months
        self.theclass(2000, 1, 1)    # no exception
        self.theclass(2000, 12, 1)   # no exception
        self.assertRaises(ValueError, self.theclass, 2000, 0, 1)
        self.assertRaises(ValueError, self.theclass, 2000, 13, 1)
        # bad days
        self.theclass(2000, 2, 29)   # no exception
        self.theclass(2004, 2, 29)   # no exception
        self.theclass(2400, 2, 29)   # no exception
        self.assertRaises(ValueError, self.theclass, 2000, 2, 30)
        self.assertRaises(ValueError, self.theclass, 2001, 2, 29)
        self.assertRaises(ValueError, self.theclass, 2100, 2, 29)
        self.assertRaises(ValueError, self.theclass, 1900, 2, 29)
        self.assertRaises(ValueError, self.theclass, 2000, 1, 0)
        self.assertRaises(ValueError, self.theclass, 2000, 1, 32)

    def test_hash_equality(self):
        d = self.theclass(2000, 12, 31)
        # same thing
        e = self.theclass(2000, 12, 31)
        self.assertEqual(d, e)
        self.assertEqual(hash(d), hash(e))

        dic = {d: 1}
        dic[e] = 2
        self.assertEqual(len(dic), 1)
        self.assertEqual(dic[d], 2)
        self.assertEqual(dic[e], 2)

        d = self.theclass(2001,  1,  1)
        # same thing
        e = self.theclass(2001,  1,  1)
        self.assertEqual(d, e)
        self.assertEqual(hash(d), hash(e))

        dic = {d: 1}
        dic[e] = 2
        self.assertEqual(len(dic), 1)
        self.assertEqual(dic[d], 2)
        self.assertEqual(dic[e], 2)

    def test_computations(self):
        a = self.theclass(2002, 1, 31)
        b = self.theclass(1956, 1, 31)
        c = self.theclass(2001,2,1)

        diff = a-b
        self.assertEqual(diff.days, 46*365 + len(range(1956, 2002, 4)))
        self.assertEqual(diff.seconds, 0)
        self.assertEqual(diff.microseconds, 0)

        day = timedelta(1)
        week = timedelta(7)
        a = self.theclass(2002, 3, 2)
        self.assertEqual(a + day, self.theclass(2002, 3, 3))
        self.assertEqual(day + a, self.theclass(2002, 3, 3))
        self.assertEqual(a - day, self.theclass(2002, 3, 1))
        self.assertEqual(-day + a, self.theclass(2002, 3, 1))
        self.assertEqual(a + week, self.theclass(2002, 3, 9))
        self.assertEqual(a - week, self.theclass(2002, 2, 23))
        self.assertEqual(a + 52*week, self.theclass(2003, 3, 1))
        self.assertEqual(a - 52*week, self.theclass(2001, 3, 3))
        self.assertEqual((a + week) - a, week)
        self.assertEqual((a + day) - a, day)
        self.assertEqual((a - week) - a, -week)
        self.assertEqual((a - day) - a, -day)
        self.assertEqual(a - (a + week), -week)
        self.assertEqual(a - (a + day), -day)
        self.assertEqual(a - (a - week), week)
        self.assertEqual(a - (a - day), day)
        self.assertEqual(c - (c - day), day)

        # Add/sub ints or floats should be illegal
        for i in 1, 1.0:
            self.assertRaises(TypeError, lambda: a+i)
            self.assertRaises(TypeError, lambda: a-i)
            self.assertRaises(TypeError, lambda: i+a)
            self.assertRaises(TypeError, lambda: i-a)

        # delta - date is senseless.
        self.assertRaises(TypeError, lambda: day - a)
        # mixing date and (delta or date) via * or // is senseless
        self.assertRaises(TypeError, lambda: day * a)
        self.assertRaises(TypeError, lambda: a * day)
        self.assertRaises(TypeError, lambda: day // a)
        self.assertRaises(TypeError, lambda: a // day)
        self.assertRaises(TypeError, lambda: a * a)
        self.assertRaises(TypeError, lambda: a // a)
        # date + date is senseless
        self.assertRaises(TypeError, lambda: a + a)

    def test_overflow(self):
        tiny = self.theclass.resolution

        for delta in [tiny, timedelta(1), timedelta(2)]:
            dt = self.theclass.min + delta
            dt -= delta  # no problem
            self.assertRaises(OverflowError, dt.__sub__, delta)
            self.assertRaises(OverflowError, dt.__add__, -delta)

            dt = self.theclass.max - delta
            dt += delta  # no problem
            self.assertRaises(OverflowError, dt.__add__, delta)
            self.assertRaises(OverflowError, dt.__sub__, -delta)

    def test_fromtimestamp(self):
        import time

        # Try an arbitrary fixed value.
        year, month, day = 1999, 9, 19
        ts = time.mktime((year, month, day, 0, 0, 0, 0, 0, -1))
        d = self.theclass.fromtimestamp(ts)
        self.assertEqual(d.year, year)
        self.assertEqual(d.month, month)
        self.assertEqual(d.day, day)

    def test_insane_fromtimestamp(self):
        # It's possible that some platform maps time_t to double,
        # and that this test will fail there.  This test should
        # exempt such platforms (provided they return reasonable
        # results!).
        for insane in -1e200, 1e200:
            self.assertRaises(OverflowError, self.theclass.fromtimestamp,
                              insane)

    def test_today(self):
        import time

        # We claim that today() is like fromtimestamp(time.time()), so
        # prove it.
        for dummy in range(3):
            today = self.theclass.today()
            ts = time.time()
            todayagain = self.theclass.fromtimestamp(ts)
            if today == todayagain:
                break
            # There are several legit reasons that could fail:
            # 1. It recently became midnight, between the today() and the
            #    time() calls.
            # 2. The platform time() has such fine resolution that we'll
            #    never get the same value twice.
            # 3. The platform time() has poor resolution, and we just
            #    happened to call today() right before a resolution quantum
            #    boundary.
            # 4. The system clock got fiddled between calls.
            # In any case, wait a little while and try again.
            time.sleep(0.1)

        # It worked or it didn't.  If it didn't, assume it's reason #2, and
        # let the test pass if they're within half a second of each other.
        self.assertTrue(today == todayagain or
                        abs(todayagain - today) < timedelta(seconds=0.5))

    def test_weekday(self):
        for i in range(7):
            # March 4, 2002 is a Monday
            self.assertEqual(self.theclass(2002, 3, 4+i).weekday(), i)
            self.assertEqual(self.theclass(2002, 3, 4+i).isoweekday(), i+1)
            # January 2, 1956 is a Monday
            self.assertEqual(self.theclass(1956, 1, 2+i).weekday(), i)
            self.assertEqual(self.theclass(1956, 1, 2+i).isoweekday(), i+1)

    def test_isocalendar(self):
        # Check examples from
        # http://www.phys.uu.nl/~vgent/calendar/isocalendar.htm
        for i in range(7):
            d = self.theclass(2003, 12, 22+i)
            self.assertEqual(d.isocalendar(), (2003, 52, i+1))
            d = self.theclass(2003, 12, 29) + timedelta(i)
            self.assertEqual(d.isocalendar(), (2004, 1, i+1))
            d = self.theclass(2004, 1, 5+i)
            self.assertEqual(d.isocalendar(), (2004, 2, i+1))
            d = self.theclass(2009, 12, 21+i)
            self.assertEqual(d.isocalendar(), (2009, 52, i+1))
            d = self.theclass(2009, 12, 28) + timedelta(i)
            self.assertEqual(d.isocalendar(), (2009, 53, i+1))
            d = self.theclass(2010, 1, 4+i)
            self.assertEqual(d.isocalendar(), (2010, 1, i+1))

    def test_iso_long_years(self):
        # Calculate long ISO years and compare to table from
        # http://www.phys.uu.nl/~vgent/calendar/isocalendar.htm
        ISO_LONG_YEARS_TABLE = """
              4   32   60   88
              9   37   65   93
             15   43   71   99
             20   48   76
             26   54   82

            105  133  161  189
            111  139  167  195
            116  144  172
            122  150  178
            128  156  184

            201  229  257  285
            207  235  263  291
            212  240  268  296
            218  246  274
            224  252  280

            303  331  359  387
            308  336  364  392
            314  342  370  398
            320  348  376
            325  353  381
        """
        iso_long_years = sorted(map(int, ISO_LONG_YEARS_TABLE.split()))
        L = []
        for i in range(400):
            d = self.theclass(2000+i, 12, 31)
            d1 = self.theclass(1600+i, 12, 31)
            self.assertEqual(d.isocalendar()[1:], d1.isocalendar()[1:])
            if d.isocalendar()[1] == 53:
                L.append(i)
        self.assertEqual(L, iso_long_years)

    def test_isoformat(self):
        t = self.theclass(2, 3, 2)
        self.assertEqual(t.isoformat(), "0002-03-02")

    def test_ctime(self):
        t = self.theclass(2002, 3, 2)
        self.assertEqual(t.ctime(), "Sat Mar  2 00:00:00 2002")

    def test_strftime(self):
        t = self.theclass(2005, 3, 2)
        self.assertEqual(t.strftime("m:%m d:%d y:%y"), "m:03 d:02 y:05")
        self.assertEqual(t.strftime(""), "") # SF bug #761337
        self.assertEqual(t.strftime('x'*1000), 'x'*1000) # SF bug #1556784

        self.assertRaises(TypeError, t.strftime) # needs an arg
        self.assertRaises(TypeError, t.strftime, "one", "two") # too many args
        self.assertRaises(TypeError, t.strftime, 42) # arg wrong type

        # test that unicode input is allowed (issue 2782)
        self.assertEqual(t.strftime("%m"), "03")

        # A naive object replaces %z and %Z w/ empty strings.
        self.assertEqual(t.strftime("'%z' '%Z'"), "'' ''")

        #make sure that invalid format specifiers are handled correctly
        #self.assertRaises(ValueError, t.strftime, "%e")
        #self.assertRaises(ValueError, t.strftime, "%")
        #self.assertRaises(ValueError, t.strftime, "%#")

        #oh well, some systems just ignore those invalid ones.
        #at least, excercise them to make sure that no crashes
        #are generated
        for f in ["%e", "%", "%#"]:
            try:
                t.strftime(f)
            except ValueError:
                pass

        #check that this standard extension works
        t.strftime("%f")


    def test_format(self):
        dt = self.theclass(2007, 9, 10)
        self.assertEqual(dt.__format__(''), str(dt))

        # check that a derived class's __str__() gets called
        class A(self.theclass):
            def __str__(self):
                return 'A'
        a = A(2007, 9, 10)
        self.assertEqual(a.__format__(''), 'A')

        # check that a derived class's strftime gets called
        class B(self.theclass):
            def strftime(self, format_spec):
                return 'B'
        b = B(2007, 9, 10)
        self.assertEqual(b.__format__(''), str(dt))

        for fmt in ["m:%m d:%d y:%y",
                    "m:%m d:%d y:%y H:%H M:%M S:%S",
                    "%z %Z",
                    ]:
            self.assertEqual(dt.__format__(fmt), dt.strftime(fmt))
            self.assertEqual(a.__format__(fmt), dt.strftime(fmt))
            self.assertEqual(b.__format__(fmt), 'B')

    def test_resolution_info(self):
        # XXX: Should min and max respect subclassing?
        if issubclass(self.theclass, datetime):
            expected_class = datetime
        else:
            expected_class = date
        self.assertIsInstance(self.theclass.min, expected_class)
        self.assertIsInstance(self.theclass.max, expected_class)
        self.assertIsInstance(self.theclass.resolution, timedelta)
        self.assertTrue(self.theclass.max > self.theclass.min)

    def test_extreme_timedelta(self):
        big = self.theclass.max - self.theclass.min
        # 3652058 days, 23 hours, 59 minutes, 59 seconds, 999999 microseconds
        n = (big.days*24*3600 + big.seconds)*1000000 + big.microseconds
        # n == 315537897599999999 ~= 2**58.13
        justasbig = timedelta(0, 0, n)
        self.assertEqual(big, justasbig)
        self.assertEqual(self.theclass.min + big, self.theclass.max)
        self.assertEqual(self.theclass.max - big, self.theclass.min)

    def test_timetuple(self):
        for i in range(7):
            # January 2, 1956 is a Monday (0)
            d = self.theclass(1956, 1, 2+i)
            t = d.timetuple()
            self.assertEqual(t, (1956, 1, 2+i, 0, 0, 0, i, 2+i, -1))
            # February 1, 1956 is a Wednesday (2)
            d = self.theclass(1956, 2, 1+i)
            t = d.timetuple()
            self.assertEqual(t, (1956, 2, 1+i, 0, 0, 0, (2+i)%7, 32+i, -1))
            # March 1, 1956 is a Thursday (3), and is the 31+29+1 = 61st day
            # of the year.
            d = self.theclass(1956, 3, 1+i)
            t = d.timetuple()
            self.assertEqual(t, (1956, 3, 1+i, 0, 0, 0, (3+i)%7, 61+i, -1))
            self.assertEqual(t.tm_year, 1956)
            self.assertEqual(t.tm_mon, 3)
            self.assertEqual(t.tm_mday, 1+i)
            self.assertEqual(t.tm_hour, 0)
            self.assertEqual(t.tm_min, 0)
            self.assertEqual(t.tm_sec, 0)
            self.assertEqual(t.tm_wday, (3+i)%7)
            self.assertEqual(t.tm_yday, 61+i)
            self.assertEqual(t.tm_isdst, -1)

    def test_pickling(self):
        args = 6, 7, 23
        orig = self.theclass(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

    def test_compare(self):
        t1 = self.theclass(2, 3, 4)
        t2 = self.theclass(2, 3, 4)
        self.assertEqual(t1, t2)
        self.assertTrue(t1 <= t2)
        self.assertTrue(t1 >= t2)
        self.assertTrue(not t1 != t2)
        self.assertTrue(not t1 < t2)
        self.assertTrue(not t1 > t2)

        for args in (3, 3, 3), (2, 4, 4), (2, 3, 5):
            t2 = self.theclass(*args)   # this is larger than t1
            self.assertTrue(t1 < t2)
            self.assertTrue(t2 > t1)
            self.assertTrue(t1 <= t2)
            self.assertTrue(t2 >= t1)
            self.assertTrue(t1 != t2)
            self.assertTrue(t2 != t1)
            self.assertTrue(not t1 == t2)
            self.assertTrue(not t2 == t1)
            self.assertTrue(not t1 > t2)
            self.assertTrue(not t2 < t1)
            self.assertTrue(not t1 >= t2)
            self.assertTrue(not t2 <= t1)

        for badarg in OTHERSTUFF:
            self.assertEqual(t1 == badarg, False)
            self.assertEqual(t1 != badarg, True)
            self.assertEqual(badarg == t1, False)
            self.assertEqual(badarg != t1, True)

            self.assertRaises(TypeError, lambda: t1 < badarg)
            self.assertRaises(TypeError, lambda: t1 > badarg)
            self.assertRaises(TypeError, lambda: t1 >= badarg)
            self.assertRaises(TypeError, lambda: badarg <= t1)
            self.assertRaises(TypeError, lambda: badarg < t1)
            self.assertRaises(TypeError, lambda: badarg > t1)
            self.assertRaises(TypeError, lambda: badarg >= t1)

    def test_mixed_compare(self):
        our = self.theclass(2000, 4, 5)

        # Our class can be compared for equality to other classes
        self.assertEqual(our == 1, False)
        self.assertEqual(1 == our, False)
        self.assertEqual(our != 1, True)
        self.assertEqual(1 != our, True)

        # But the ordering is undefined
        self.assertRaises(TypeError, lambda: our < 1)
        self.assertRaises(TypeError, lambda: 1 < our)

        # Repeat those tests with a different class

        class SomeClass:
            pass

        their = SomeClass()
        self.assertEqual(our == their, False)
        self.assertEqual(their == our, False)
        self.assertEqual(our != their, True)
        self.assertEqual(their != our, True)
        self.assertRaises(TypeError, lambda: our < their)
        self.assertRaises(TypeError, lambda: their < our)

        # However, if the other class explicitly defines ordering
        # relative to our class, it is allowed to do so

        class LargerThanAnything:
            def __lt__(self, other):
                return False
            def __le__(self, other):
                return isinstance(other, LargerThanAnything)
            def __eq__(self, other):
                return isinstance(other, LargerThanAnything)
            def __ne__(self, other):
                return not isinstance(other, LargerThanAnything)
            def __gt__(self, other):
                return not isinstance(other, LargerThanAnything)
            def __ge__(self, other):
                return True

        their = LargerThanAnything()
        self.assertEqual(our == their, False)
        self.assertEqual(their == our, False)
        self.assertEqual(our != their, True)
        self.assertEqual(their != our, True)
        self.assertEqual(our < their, True)
        self.assertEqual(their < our, False)

    def test_bool(self):
        # All dates are considered true.
        self.assertTrue(self.theclass.min)
        self.assertTrue(self.theclass.max)

    def test_strftime_y2k(self):
        for y in (1, 49, 70, 99, 100, 999, 1000, 1970):
            d = self.theclass(y, 1, 1)
            # Issue 13305:  For years < 1000, the value is not always
            # padded to 4 digits across platforms.  The C standard
            # assumes year >= 1900, so it does not specify the number
            # of digits.
            if d.strftime("%Y") != '%04d' % y:
                # Year 42 returns '42', not padded
                self.assertEqual(d.strftime("%Y"), '%d' % y)
                # '0042' is obtained anyway
                self.assertEqual(d.strftime("%4Y"), '%04d' % y)

    def test_replace(self):
        cls = self.theclass
        args = [1, 2, 3]
        base = cls(*args)
        self.assertEqual(base, base.replace())

        i = 0
        for name, newval in (("year", 2),
                             ("month", 3),
                             ("day", 4)):
            newargs = args[:]
            newargs[i] = newval
            expected = cls(*newargs)
            got = base.replace(**{name: newval})
            self.assertEqual(expected, got)
            i += 1

        # Out of bounds.
        base = cls(2000, 2, 29)
        self.assertRaises(ValueError, base.replace, year=2001)

    def test_subclass_date(self):

        class C(self.theclass):
            theAnswer = 42

            def __new__(cls, *args, **kws):
                temp = kws.copy()
                extra = temp.pop('extra')
                result = self.theclass.__new__(cls, *args, **temp)
                result.extra = extra
                return result

            def newmeth(self, start):
                return start + self.year + self.month

        args = 2003, 4, 14

        dt1 = self.theclass(*args)
        dt2 = C(*args, **{'extra': 7})

        self.assertEqual(dt2.__class__, C)
        self.assertEqual(dt2.theAnswer, 42)
        self.assertEqual(dt2.extra, 7)
        self.assertEqual(dt1.toordinal(), dt2.toordinal())
        self.assertEqual(dt2.newmeth(-7), dt1.year + dt1.month - 7)

    def test_pickling_subclass_date(self):

        args = 6, 7, 23
        orig = SubclassDate(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

    def test_backdoor_resistance(self):
        # For fast unpickling, the constructor accepts a pickle byte string.
        # This is a low-overhead backdoor.  A user can (by intent or
        # mistake) pass a string directly, which (if it's the right length)
        # will get treated like a pickle, and bypass the normal sanity
        # checks in the constructor.  This can create insane objects.
        # The constructor doesn't want to burn the time to validate all
        # fields, but does check the month field.  This stops, e.g.,
        # datetime.datetime('1995-03-25') from yielding an insane object.
        base = b'1995-03-25'
        if not issubclass(self.theclass, datetime):
            base = base[:4]
        for month_byte in b'9', b'\0', b'\r', b'\xff':
            self.assertRaises(TypeError, self.theclass,
                                         base[:2] + month_byte + base[3:])
        # Good bytes, but bad tzinfo:
        self.assertRaises(TypeError, self.theclass,
                          bytes([1] * len(base)), 'EST')

        for ord_byte in range(1, 13):
            # This shouldn't blow up because of the month byte alone.  If
            # the implementation changes to do more-careful checking, it may
            # blow up because other fields are insane.
            self.theclass(base[:2] + bytes([ord_byte]) + base[3:])

#############################################################################
# datetime tests

class SubclassDatetime(datetime):
    sub_var = 1

class TestDateTime(TestDate):

    theclass = datetime

    def test_basic_attributes(self):
        dt = self.theclass(2002, 3, 1, 12, 0)
        self.assertEqual(dt.year, 2002)
        self.assertEqual(dt.month, 3)
        self.assertEqual(dt.day, 1)
        self.assertEqual(dt.hour, 12)
        self.assertEqual(dt.minute, 0)
        self.assertEqual(dt.second, 0)
        self.assertEqual(dt.microsecond, 0)

    def test_basic_attributes_nonzero(self):
        # Make sure all attributes are non-zero so bugs in
        # bit-shifting access show up.
        dt = self.theclass(2002, 3, 1, 12, 59, 59, 8000)
        self.assertEqual(dt.year, 2002)
        self.assertEqual(dt.month, 3)
        self.assertEqual(dt.day, 1)
        self.assertEqual(dt.hour, 12)
        self.assertEqual(dt.minute, 59)
        self.assertEqual(dt.second, 59)
        self.assertEqual(dt.microsecond, 8000)

    def test_roundtrip(self):
        for dt in (self.theclass(1, 2, 3, 4, 5, 6, 7),
                   self.theclass.now()):
            # Verify dt -> string -> datetime identity.
            s = repr(dt)
            self.assertTrue(s.startswith('datetime.'))
            s = s[9:]
            dt2 = eval(s)
            self.assertEqual(dt, dt2)

            # Verify identity via reconstructing from pieces.
            dt2 = self.theclass(dt.year, dt.month, dt.day,
                                dt.hour, dt.minute, dt.second,
                                dt.microsecond)
            self.assertEqual(dt, dt2)

    def test_isoformat(self):
        t = self.theclass(2, 3, 2, 4, 5, 1, 123)
        self.assertEqual(t.isoformat(),    "0002-03-02T04:05:01.000123")
        self.assertEqual(t.isoformat('T'), "0002-03-02T04:05:01.000123")
        self.assertEqual(t.isoformat(' '), "0002-03-02 04:05:01.000123")
        self.assertEqual(t.isoformat('\x00'), "0002-03-02\x0004:05:01.000123")
        # str is ISO format with the separator forced to a blank.
        self.assertEqual(str(t), "0002-03-02 04:05:01.000123")

        t = self.theclass(2, 3, 2)
        self.assertEqual(t.isoformat(),    "0002-03-02T00:00:00")
        self.assertEqual(t.isoformat('T'), "0002-03-02T00:00:00")
        self.assertEqual(t.isoformat(' '), "0002-03-02 00:00:00")
        # str is ISO format with the separator forced to a blank.
        self.assertEqual(str(t), "0002-03-02 00:00:00")

    def test_format(self):
        dt = self.theclass(2007, 9, 10, 4, 5, 1, 123)
        self.assertEqual(dt.__format__(''), str(dt))

        # check that a derived class's __str__() gets called
        class A(self.theclass):
            def __str__(self):
                return 'A'
        a = A(2007, 9, 10, 4, 5, 1, 123)
        self.assertEqual(a.__format__(''), 'A')

        # check that a derived class's strftime gets called
        class B(self.theclass):
            def strftime(self, format_spec):
                return 'B'
        b = B(2007, 9, 10, 4, 5, 1, 123)
        self.assertEqual(b.__format__(''), str(dt))

        for fmt in ["m:%m d:%d y:%y",
                    "m:%m d:%d y:%y H:%H M:%M S:%S",
                    "%z %Z",
                    ]:
            self.assertEqual(dt.__format__(fmt), dt.strftime(fmt))
            self.assertEqual(a.__format__(fmt), dt.strftime(fmt))
            self.assertEqual(b.__format__(fmt), 'B')

    def test_more_ctime(self):
        # Test fields that TestDate doesn't touch.
        import time

        t = self.theclass(2002, 3, 2, 18, 3, 5, 123)
        self.assertEqual(t.ctime(), "Sat Mar  2 18:03:05 2002")
        # Oops!  The next line fails on Win2K under MSVC 6, so it's commented
        # out.  The difference is that t.ctime() produces " 2" for the day,
        # but platform ctime() produces "02" for the day.  According to
        # C99, t.ctime() is correct here.
        # self.assertEqual(t.ctime(), time.ctime(time.mktime(t.timetuple())))

        # So test a case where that difference doesn't matter.
        t = self.theclass(2002, 3, 22, 18, 3, 5, 123)
        self.assertEqual(t.ctime(), time.ctime(time.mktime(t.timetuple())))

    def test_tz_independent_comparing(self):
        dt1 = self.theclass(2002, 3, 1, 9, 0, 0)
        dt2 = self.theclass(2002, 3, 1, 10, 0, 0)
        dt3 = self.theclass(2002, 3, 1, 9, 0, 0)
        self.assertEqual(dt1, dt3)
        self.assertTrue(dt2 > dt3)

        # Make sure comparison doesn't forget microseconds, and isn't done
        # via comparing a float timestamp (an IEEE double doesn't have enough
        # precision to span microsecond resolution across years 1 thru 9999,
        # so comparing via timestamp necessarily calls some distinct values
        # equal).
        dt1 = self.theclass(MAXYEAR, 12, 31, 23, 59, 59, 999998)
        us = timedelta(microseconds=1)
        dt2 = dt1 + us
        self.assertEqual(dt2 - dt1, us)
        self.assertTrue(dt1 < dt2)

    def test_strftime_with_bad_tzname_replace(self):
        # verify ok if tzinfo.tzname().replace() returns a non-string
        class MyTzInfo(FixedOffset):
            def tzname(self, dt):
                class MyStr(str):
                    def replace(self, *args):
                        return None
                return MyStr('name')
        t = self.theclass(2005, 3, 2, 0, 0, 0, 0, MyTzInfo(3, 'name'))
        self.assertRaises(TypeError, t.strftime, '%Z')

    def test_bad_constructor_arguments(self):
        # bad years
        self.theclass(MINYEAR, 1, 1)  # no exception
        self.theclass(MAXYEAR, 1, 1)  # no exception
        self.assertRaises(ValueError, self.theclass, MINYEAR-1, 1, 1)
        self.assertRaises(ValueError, self.theclass, MAXYEAR+1, 1, 1)
        # bad months
        self.theclass(2000, 1, 1)    # no exception
        self.theclass(2000, 12, 1)   # no exception
        self.assertRaises(ValueError, self.theclass, 2000, 0, 1)
        self.assertRaises(ValueError, self.theclass, 2000, 13, 1)
        # bad days
        self.theclass(2000, 2, 29)   # no exception
        self.theclass(2004, 2, 29)   # no exception
        self.theclass(2400, 2, 29)   # no exception
        self.assertRaises(ValueError, self.theclass, 2000, 2, 30)
        self.assertRaises(ValueError, self.theclass, 2001, 2, 29)
        self.assertRaises(ValueError, self.theclass, 2100, 2, 29)
        self.assertRaises(ValueError, self.theclass, 1900, 2, 29)
        self.assertRaises(ValueError, self.theclass, 2000, 1, 0)
        self.assertRaises(ValueError, self.theclass, 2000, 1, 32)
        # bad hours
        self.theclass(2000, 1, 31, 0)    # no exception
        self.theclass(2000, 1, 31, 23)   # no exception
        self.assertRaises(ValueError, self.theclass, 2000, 1, 31, -1)
        self.assertRaises(ValueError, self.theclass, 2000, 1, 31, 24)
        # bad minutes
        self.theclass(2000, 1, 31, 23, 0)    # no exception
        self.theclass(2000, 1, 31, 23, 59)   # no exception
        self.assertRaises(ValueError, self.theclass, 2000, 1, 31, 23, -1)
        self.assertRaises(ValueError, self.theclass, 2000, 1, 31, 23, 60)
        # bad seconds
        self.theclass(2000, 1, 31, 23, 59, 0)    # no exception
        self.theclass(2000, 1, 31, 23, 59, 59)   # no exception
        self.assertRaises(ValueError, self.theclass, 2000, 1, 31, 23, 59, -1)
        self.assertRaises(ValueError, self.theclass, 2000, 1, 31, 23, 59, 60)
        # bad microseconds
        self.theclass(2000, 1, 31, 23, 59, 59, 0)    # no exception
        self.theclass(2000, 1, 31, 23, 59, 59, 999999)   # no exception
        self.assertRaises(ValueError, self.theclass,
                          2000, 1, 31, 23, 59, 59, -1)
        self.assertRaises(ValueError, self.theclass,
                          2000, 1, 31, 23, 59, 59,
                          1000000)

    def test_hash_equality(self):
        d = self.theclass(2000, 12, 31, 23, 30, 17)
        e = self.theclass(2000, 12, 31, 23, 30, 17)
        self.assertEqual(d, e)
        self.assertEqual(hash(d), hash(e))

        dic = {d: 1}
        dic[e] = 2
        self.assertEqual(len(dic), 1)
        self.assertEqual(dic[d], 2)
        self.assertEqual(dic[e], 2)

        d = self.theclass(2001,  1,  1,  0,  5, 17)
        e = self.theclass(2001,  1,  1,  0,  5, 17)
        self.assertEqual(d, e)
        self.assertEqual(hash(d), hash(e))

        dic = {d: 1}
        dic[e] = 2
        self.assertEqual(len(dic), 1)
        self.assertEqual(dic[d], 2)
        self.assertEqual(dic[e], 2)

    def test_computations(self):
        a = self.theclass(2002, 1, 31)
        b = self.theclass(1956, 1, 31)
        diff = a-b
        self.assertEqual(diff.days, 46*365 + len(range(1956, 2002, 4)))
        self.assertEqual(diff.seconds, 0)
        self.assertEqual(diff.microseconds, 0)
        a = self.theclass(2002, 3, 2, 17, 6)
        millisec = timedelta(0, 0, 1000)
        hour = timedelta(0, 3600)
        day = timedelta(1)
        week = timedelta(7)
        self.assertEqual(a + hour, self.theclass(2002, 3, 2, 18, 6))
        self.assertEqual(hour + a, self.theclass(2002, 3, 2, 18, 6))
        self.assertEqual(a + 10*hour, self.theclass(2002, 3, 3, 3, 6))
        self.assertEqual(a - hour, self.theclass(2002, 3, 2, 16, 6))
        self.assertEqual(-hour + a, self.theclass(2002, 3, 2, 16, 6))
        self.assertEqual(a - hour, a + -hour)
        self.assertEqual(a - 20*hour, self.theclass(2002, 3, 1, 21, 6))
        self.assertEqual(a + day, self.theclass(2002, 3, 3, 17, 6))
        self.assertEqual(a - day, self.theclass(2002, 3, 1, 17, 6))
        self.assertEqual(a + week, self.theclass(2002, 3, 9, 17, 6))
        self.assertEqual(a - week, self.theclass(2002, 2, 23, 17, 6))
        self.assertEqual(a + 52*week, self.theclass(2003, 3, 1, 17, 6))
        self.assertEqual(a - 52*week, self.theclass(2001, 3, 3, 17, 6))
        self.assertEqual((a + week) - a, week)
        self.assertEqual((a + day) - a, day)
        self.assertEqual((a + hour) - a, hour)
        self.assertEqual((a + millisec) - a, millisec)
        self.assertEqual((a - week) - a, -week)
        self.assertEqual((a - day) - a, -day)
        self.assertEqual((a - hour) - a, -hour)
        self.assertEqual((a - millisec) - a, -millisec)
        self.assertEqual(a - (a + week), -week)
        self.assertEqual(a - (a + day), -day)
        self.assertEqual(a - (a + hour), -hour)
        self.assertEqual(a - (a + millisec), -millisec)
        self.assertEqual(a - (a - week), week)
        self.assertEqual(a - (a - day), day)
        self.assertEqual(a - (a - hour), hour)
        self.assertEqual(a - (a - millisec), millisec)
        self.assertEqual(a + (week + day + hour + millisec),
                         self.theclass(2002, 3, 10, 18, 6, 0, 1000))
        self.assertEqual(a + (week + day + hour + millisec),
                         (((a + week) + day) + hour) + millisec)
        self.assertEqual(a - (week + day + hour + millisec),
                         self.theclass(2002, 2, 22, 16, 5, 59, 999000))
        self.assertEqual(a - (week + day + hour + millisec),
                         (((a - week) - day) - hour) - millisec)
        # Add/sub ints or floats should be illegal
        for i in 1, 1.0:
            self.assertRaises(TypeError, lambda: a+i)
            self.assertRaises(TypeError, lambda: a-i)
            self.assertRaises(TypeError, lambda: i+a)
            self.assertRaises(TypeError, lambda: i-a)

        # delta - datetime is senseless.
        self.assertRaises(TypeError, lambda: day - a)
        # mixing datetime and (delta or datetime) via * or // is senseless
        self.assertRaises(TypeError, lambda: day * a)
        self.assertRaises(TypeError, lambda: a * day)
        self.assertRaises(TypeError, lambda: day // a)
        self.assertRaises(TypeError, lambda: a // day)
        self.assertRaises(TypeError, lambda: a * a)
        self.assertRaises(TypeError, lambda: a // a)
        # datetime + datetime is senseless
        self.assertRaises(TypeError, lambda: a + a)

    def test_pickling(self):
        args = 6, 7, 23, 20, 59, 1, 64**2
        orig = self.theclass(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

    def test_more_pickling(self):
        a = self.theclass(2003, 2, 7, 16, 48, 37, 444116)
        s = pickle.dumps(a)
        b = pickle.loads(s)
        self.assertEqual(b.year, 2003)
        self.assertEqual(b.month, 2)
        self.assertEqual(b.day, 7)

    def test_pickling_subclass_datetime(self):
        args = 6, 7, 23, 20, 59, 1, 64**2
        orig = SubclassDatetime(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

    def test_more_compare(self):
        # The test_compare() inherited from TestDate covers the error cases.
        # We just want to test lexicographic ordering on the members datetime
        # has that date lacks.
        args = [2000, 11, 29, 20, 58, 16, 999998]
        t1 = self.theclass(*args)
        t2 = self.theclass(*args)
        self.assertEqual(t1, t2)
        self.assertTrue(t1 <= t2)
        self.assertTrue(t1 >= t2)
        self.assertTrue(not t1 != t2)
        self.assertTrue(not t1 < t2)
        self.assertTrue(not t1 > t2)

        for i in range(len(args)):
            newargs = args[:]
            newargs[i] = args[i] + 1
            t2 = self.theclass(*newargs)   # this is larger than t1
            self.assertTrue(t1 < t2)
            self.assertTrue(t2 > t1)
            self.assertTrue(t1 <= t2)
            self.assertTrue(t2 >= t1)
            self.assertTrue(t1 != t2)
            self.assertTrue(t2 != t1)
            self.assertTrue(not t1 == t2)
            self.assertTrue(not t2 == t1)
            self.assertTrue(not t1 > t2)
            self.assertTrue(not t2 < t1)
            self.assertTrue(not t1 >= t2)
            self.assertTrue(not t2 <= t1)


    # A helper for timestamp constructor tests.
    def verify_field_equality(self, expected, got):
        self.assertEqual(expected.tm_year, got.year)
        self.assertEqual(expected.tm_mon, got.month)
        self.assertEqual(expected.tm_mday, got.day)
        self.assertEqual(expected.tm_hour, got.hour)
        self.assertEqual(expected.tm_min, got.minute)
        self.assertEqual(expected.tm_sec, got.second)

    def test_fromtimestamp(self):
        import time

        ts = time.time()
        expected = time.localtime(ts)
        got = self.theclass.fromtimestamp(ts)
        self.verify_field_equality(expected, got)

    def test_utcfromtimestamp(self):
        import time

        ts = time.time()
        expected = time.gmtime(ts)
        got = self.theclass.utcfromtimestamp(ts)
        self.verify_field_equality(expected, got)

    # Run with US-style DST rules: DST begins 2 a.m. on second Sunday in
    # March (M3.2.0) and ends 2 a.m. on first Sunday in November (M11.1.0).
    @support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_timestamp_naive(self):
        t = self.theclass(1970, 1, 1)
        self.assertEqual(t.timestamp(), 18000.0)
        t = self.theclass(1970, 1, 1, 1, 2, 3, 4)
        self.assertEqual(t.timestamp(),
                         18000.0 + 3600 + 2*60 + 3 + 4*1e-6)
        # Missing hour may produce platform-dependent result
        t = self.theclass(2012, 3, 11, 2, 30)
        self.assertIn(self.theclass.fromtimestamp(t.timestamp()),
                      [t - timedelta(hours=1), t + timedelta(hours=1)])
        # Ambiguous hour defaults to DST
        t = self.theclass(2012, 11, 4, 1, 30)
        self.assertEqual(self.theclass.fromtimestamp(t.timestamp()), t)

        # Timestamp may raise an overflow error on some platforms
        for t in [self.theclass(1,1,1), self.theclass(9999,12,12)]:
            try:
                s = t.timestamp()
            except OverflowError:
                pass
            else:
                self.assertEqual(self.theclass.fromtimestamp(s), t)

    def test_timestamp_aware(self):
        t = self.theclass(1970, 1, 1, tzinfo=timezone.utc)
        self.assertEqual(t.timestamp(), 0.0)
        t = self.theclass(1970, 1, 1, 1, 2, 3, 4, tzinfo=timezone.utc)
        self.assertEqual(t.timestamp(),
                         3600 + 2*60 + 3 + 4*1e-6)
        t = self.theclass(1970, 1, 1, 1, 2, 3, 4,
                          tzinfo=timezone(timedelta(hours=-5), 'EST'))
        self.assertEqual(t.timestamp(),
                         18000 + 3600 + 2*60 + 3 + 4*1e-6)
    def test_microsecond_rounding(self):
        for fts in [self.theclass.fromtimestamp,
                    self.theclass.utcfromtimestamp]:
            zero = fts(0)
            self.assertEqual(zero.second, 0)
            self.assertEqual(zero.microsecond, 0)
            try:
                minus_one = fts(-1e-6)
            except OSError:
                # localtime(-1) and gmtime(-1) is not supported on Windows
                pass
            else:
                self.assertEqual(minus_one.second, 59)
                self.assertEqual(minus_one.microsecond, 999999)

                t = fts(-1e-8)
                self.assertEqual(t, minus_one)
                t = fts(-9e-7)
                self.assertEqual(t, minus_one)
                t = fts(-1e-7)
                self.assertEqual(t, minus_one)

            t = fts(1e-7)
            self.assertEqual(t, zero)
            t = fts(9e-7)
            self.assertEqual(t, zero)
            t = fts(0.99999949)
            self.assertEqual(t.second, 0)
            self.assertEqual(t.microsecond, 999999)
            t = fts(0.9999999)
            self.assertEqual(t.second, 0)
            self.assertEqual(t.microsecond, 999999)

    def test_insane_fromtimestamp(self):
        # It's possible that some platform maps time_t to double,
        # and that this test will fail there.  This test should
        # exempt such platforms (provided they return reasonable
        # results!).
        for insane in -1e200, 1e200:
            self.assertRaises(OverflowError, self.theclass.fromtimestamp,
                              insane)

    def test_insane_utcfromtimestamp(self):
        # It's possible that some platform maps time_t to double,
        # and that this test will fail there.  This test should
        # exempt such platforms (provided they return reasonable
        # results!).
        for insane in -1e200, 1e200:
            self.assertRaises(OverflowError, self.theclass.utcfromtimestamp,
                              insane)
    @unittest.skipIf(sys.platform == "win32", "Windows doesn't accept negative timestamps")
    def test_negative_float_fromtimestamp(self):
        # The result is tz-dependent; at least test that this doesn't
        # fail (like it did before bug 1646728 was fixed).
        self.theclass.fromtimestamp(-1.05)

    @unittest.skipIf(sys.platform == "win32", "Windows doesn't accept negative timestamps")
    def test_negative_float_utcfromtimestamp(self):
        d = self.theclass.utcfromtimestamp(-1.05)
        self.assertEqual(d, self.theclass(1969, 12, 31, 23, 59, 58, 950000))

    def test_utcnow(self):
        import time

        # Call it a success if utcnow() and utcfromtimestamp() are within
        # a second of each other.
        tolerance = timedelta(seconds=1)
        for dummy in range(3):
            from_now = self.theclass.utcnow()
            from_timestamp = self.theclass.utcfromtimestamp(time.time())
            if abs(from_timestamp - from_now) <= tolerance:
                break
            # Else try again a few times.
        self.assertTrue(abs(from_timestamp - from_now) <= tolerance)

    def test_strptime(self):
        string = '2004-12-01 13:02:47.197'
        format = '%Y-%m-%d %H:%M:%S.%f'
        expected = _strptime._strptime_datetime(self.theclass, string, format)
        got = self.theclass.strptime(string, format)
        self.assertEqual(expected, got)
        self.assertIs(type(expected), self.theclass)
        self.assertIs(type(got), self.theclass)

        strptime = self.theclass.strptime
        self.assertEqual(strptime("+0002", "%z").utcoffset(), 2 * MINUTE)
        self.assertEqual(strptime("-0002", "%z").utcoffset(), -2 * MINUTE)
        # Only local timezone and UTC are supported
        for tzseconds, tzname in ((0, 'UTC'), (0, 'GMT'),
                                 (-_time.timezone, _time.tzname[0])):
            if tzseconds < 0:
                sign = '-'
                seconds = -tzseconds
            else:
                sign ='+'
                seconds = tzseconds
            hours, minutes = divmod(seconds//60, 60)
            dtstr = "{}{:02d}{:02d} {}".format(sign, hours, minutes, tzname)
            dt = strptime(dtstr, "%z %Z")
            self.assertEqual(dt.utcoffset(), timedelta(seconds=tzseconds))
            self.assertEqual(dt.tzname(), tzname)
        # Can produce inconsistent datetime
        dtstr, fmt = "+1234 UTC", "%z %Z"
        dt = strptime(dtstr, fmt)
        self.assertEqual(dt.utcoffset(), 12 * HOUR + 34 * MINUTE)
        self.assertEqual(dt.tzname(), 'UTC')
        # yet will roundtrip
        self.assertEqual(dt.strftime(fmt), dtstr)

        # Produce naive datetime if no %z is provided
        self.assertEqual(strptime("UTC", "%Z").tzinfo, None)

        with self.assertRaises(ValueError): strptime("-2400", "%z")
        with self.assertRaises(ValueError): strptime("-000", "%z")

    def test_more_timetuple(self):
        # This tests fields beyond those tested by the TestDate.test_timetuple.
        t = self.theclass(2004, 12, 31, 6, 22, 33)
        self.assertEqual(t.timetuple(), (2004, 12, 31, 6, 22, 33, 4, 366, -1))
        self.assertEqual(t.timetuple(),
                         (t.year, t.month, t.day,
                          t.hour, t.minute, t.second,
                          t.weekday(),
                          t.toordinal() - date(t.year, 1, 1).toordinal() + 1,
                          -1))
        tt = t.timetuple()
        self.assertEqual(tt.tm_year, t.year)
        self.assertEqual(tt.tm_mon, t.month)
        self.assertEqual(tt.tm_mday, t.day)
        self.assertEqual(tt.tm_hour, t.hour)
        self.assertEqual(tt.tm_min, t.minute)
        self.assertEqual(tt.tm_sec, t.second)
        self.assertEqual(tt.tm_wday, t.weekday())
        self.assertEqual(tt.tm_yday, t.toordinal() -
                                     date(t.year, 1, 1).toordinal() + 1)
        self.assertEqual(tt.tm_isdst, -1)

    def test_more_strftime(self):
        # This tests fields beyond those tested by the TestDate.test_strftime.
        t = self.theclass(2004, 12, 31, 6, 22, 33, 47)
        self.assertEqual(t.strftime("%m %d %y %f %S %M %H %j"),
                                    "12 31 04 000047 33 22 06 366")

    def test_extract(self):
        dt = self.theclass(2002, 3, 4, 18, 45, 3, 1234)
        self.assertEqual(dt.date(), date(2002, 3, 4))
        self.assertEqual(dt.time(), time(18, 45, 3, 1234))

    def test_combine(self):
        d = date(2002, 3, 4)
        t = time(18, 45, 3, 1234)
        expected = self.theclass(2002, 3, 4, 18, 45, 3, 1234)
        combine = self.theclass.combine
        dt = combine(d, t)
        self.assertEqual(dt, expected)

        dt = combine(time=t, date=d)
        self.assertEqual(dt, expected)

        self.assertEqual(d, dt.date())
        self.assertEqual(t, dt.time())
        self.assertEqual(dt, combine(dt.date(), dt.time()))

        self.assertRaises(TypeError, combine) # need an arg
        self.assertRaises(TypeError, combine, d) # need two args
        self.assertRaises(TypeError, combine, t, d) # args reversed
        self.assertRaises(TypeError, combine, d, t, 1) # too many args
        self.assertRaises(TypeError, combine, "date", "time") # wrong types
        self.assertRaises(TypeError, combine, d, "time") # wrong type
        self.assertRaises(TypeError, combine, "date", t) # wrong type

    def test_replace(self):
        cls = self.theclass
        args = [1, 2, 3, 4, 5, 6, 7]
        base = cls(*args)
        self.assertEqual(base, base.replace())

        i = 0
        for name, newval in (("year", 2),
                             ("month", 3),
                             ("day", 4),
                             ("hour", 5),
                             ("minute", 6),
                             ("second", 7),
                             ("microsecond", 8)):
            newargs = args[:]
            newargs[i] = newval
            expected = cls(*newargs)
            got = base.replace(**{name: newval})
            self.assertEqual(expected, got)
            i += 1

        # Out of bounds.
        base = cls(2000, 2, 29)
        self.assertRaises(ValueError, base.replace, year=2001)

    def test_astimezone(self):
        # Pretty boring!  The TZ test is more interesting here.  astimezone()
        # simply can't be applied to a naive object.
        dt = self.theclass.now()
        f = FixedOffset(44, "")
        self.assertRaises(ValueError, dt.astimezone) # naive
        self.assertRaises(TypeError, dt.astimezone, f, f) # too many args
        self.assertRaises(TypeError, dt.astimezone, dt) # arg wrong type
        self.assertRaises(ValueError, dt.astimezone, f) # naive
        self.assertRaises(ValueError, dt.astimezone, tz=f)  # naive

        class Bogus(tzinfo):
            def utcoffset(self, dt): return None
            def dst(self, dt): return timedelta(0)
        bog = Bogus()
        self.assertRaises(ValueError, dt.astimezone, bog)   # naive
        self.assertRaises(ValueError,
                          dt.replace(tzinfo=bog).astimezone, f)

        class AlsoBogus(tzinfo):
            def utcoffset(self, dt): return timedelta(0)
            def dst(self, dt): return None
        alsobog = AlsoBogus()
        self.assertRaises(ValueError, dt.astimezone, alsobog) # also naive

    def test_subclass_datetime(self):

        class C(self.theclass):
            theAnswer = 42

            def __new__(cls, *args, **kws):
                temp = kws.copy()
                extra = temp.pop('extra')
                result = self.theclass.__new__(cls, *args, **temp)
                result.extra = extra
                return result

            def newmeth(self, start):
                return start + self.year + self.month + self.second

        args = 2003, 4, 14, 12, 13, 41

        dt1 = self.theclass(*args)
        dt2 = C(*args, **{'extra': 7})

        self.assertEqual(dt2.__class__, C)
        self.assertEqual(dt2.theAnswer, 42)
        self.assertEqual(dt2.extra, 7)
        self.assertEqual(dt1.toordinal(), dt2.toordinal())
        self.assertEqual(dt2.newmeth(-7), dt1.year + dt1.month +
                                          dt1.second - 7)

class TestSubclassDateTime(TestDateTime):
    theclass = SubclassDatetime
    # Override tests not designed for subclass
    def test_roundtrip(self):
        pass

class SubclassTime(time):
    sub_var = 1

class TestTime(HarmlessMixedComparison, unittest.TestCase):

    theclass = time

    def test_basic_attributes(self):
        t = self.theclass(12, 0)
        self.assertEqual(t.hour, 12)
        self.assertEqual(t.minute, 0)
        self.assertEqual(t.second, 0)
        self.assertEqual(t.microsecond, 0)

    def test_basic_attributes_nonzero(self):
        # Make sure all attributes are non-zero so bugs in
        # bit-shifting access show up.
        t = self.theclass(12, 59, 59, 8000)
        self.assertEqual(t.hour, 12)
        self.assertEqual(t.minute, 59)
        self.assertEqual(t.second, 59)
        self.assertEqual(t.microsecond, 8000)

    def test_roundtrip(self):
        t = self.theclass(1, 2, 3, 4)

        # Verify t -> string -> time identity.
        s = repr(t)
        self.assertTrue(s.startswith('datetime.'))
        s = s[9:]
        t2 = eval(s)
        self.assertEqual(t, t2)

        # Verify identity via reconstructing from pieces.
        t2 = self.theclass(t.hour, t.minute, t.second,
                           t.microsecond)
        self.assertEqual(t, t2)

    def test_comparing(self):
        args = [1, 2, 3, 4]
        t1 = self.theclass(*args)
        t2 = self.theclass(*args)
        self.assertEqual(t1, t2)
        self.assertTrue(t1 <= t2)
        self.assertTrue(t1 >= t2)
        self.assertTrue(not t1 != t2)
        self.assertTrue(not t1 < t2)
        self.assertTrue(not t1 > t2)

        for i in range(len(args)):
            newargs = args[:]
            newargs[i] = args[i] + 1
            t2 = self.theclass(*newargs)   # this is larger than t1
            self.assertTrue(t1 < t2)
            self.assertTrue(t2 > t1)
            self.assertTrue(t1 <= t2)
            self.assertTrue(t2 >= t1)
            self.assertTrue(t1 != t2)
            self.assertTrue(t2 != t1)
            self.assertTrue(not t1 == t2)
            self.assertTrue(not t2 == t1)
            self.assertTrue(not t1 > t2)
            self.assertTrue(not t2 < t1)
            self.assertTrue(not t1 >= t2)
            self.assertTrue(not t2 <= t1)

        for badarg in OTHERSTUFF:
            self.assertEqual(t1 == badarg, False)
            self.assertEqual(t1 != badarg, True)
            self.assertEqual(badarg == t1, False)
            self.assertEqual(badarg != t1, True)

            self.assertRaises(TypeError, lambda: t1 <= badarg)
            self.assertRaises(TypeError, lambda: t1 < badarg)
            self.assertRaises(TypeError, lambda: t1 > badarg)
            self.assertRaises(TypeError, lambda: t1 >= badarg)
            self.assertRaises(TypeError, lambda: badarg <= t1)
            self.assertRaises(TypeError, lambda: badarg < t1)
            self.assertRaises(TypeError, lambda: badarg > t1)
            self.assertRaises(TypeError, lambda: badarg >= t1)

    def test_bad_constructor_arguments(self):
        # bad hours
        self.theclass(0, 0)    # no exception
        self.theclass(23, 0)   # no exception
        self.assertRaises(ValueError, self.theclass, -1, 0)
        self.assertRaises(ValueError, self.theclass, 24, 0)
        # bad minutes
        self.theclass(23, 0)    # no exception
        self.theclass(23, 59)   # no exception
        self.assertRaises(ValueError, self.theclass, 23, -1)
        self.assertRaises(ValueError, self.theclass, 23, 60)
        # bad seconds
        self.theclass(23, 59, 0)    # no exception
        self.theclass(23, 59, 59)   # no exception
        self.assertRaises(ValueError, self.theclass, 23, 59, -1)
        self.assertRaises(ValueError, self.theclass, 23, 59, 60)
        # bad microseconds
        self.theclass(23, 59, 59, 0)        # no exception
        self.theclass(23, 59, 59, 999999)   # no exception
        self.assertRaises(ValueError, self.theclass, 23, 59, 59, -1)
        self.assertRaises(ValueError, self.theclass, 23, 59, 59, 1000000)

    def test_hash_equality(self):
        d = self.theclass(23, 30, 17)
        e = self.theclass(23, 30, 17)
        self.assertEqual(d, e)
        self.assertEqual(hash(d), hash(e))

        dic = {d: 1}
        dic[e] = 2
        self.assertEqual(len(dic), 1)
        self.assertEqual(dic[d], 2)
        self.assertEqual(dic[e], 2)

        d = self.theclass(0,  5, 17)
        e = self.theclass(0,  5, 17)
        self.assertEqual(d, e)
        self.assertEqual(hash(d), hash(e))

        dic = {d: 1}
        dic[e] = 2
        self.assertEqual(len(dic), 1)
        self.assertEqual(dic[d], 2)
        self.assertEqual(dic[e], 2)

    def test_isoformat(self):
        t = self.theclass(4, 5, 1, 123)
        self.assertEqual(t.isoformat(), "04:05:01.000123")
        self.assertEqual(t.isoformat(), str(t))

        t = self.theclass()
        self.assertEqual(t.isoformat(), "00:00:00")
        self.assertEqual(t.isoformat(), str(t))

        t = self.theclass(microsecond=1)
        self.assertEqual(t.isoformat(), "00:00:00.000001")
        self.assertEqual(t.isoformat(), str(t))

        t = self.theclass(microsecond=10)
        self.assertEqual(t.isoformat(), "00:00:00.000010")
        self.assertEqual(t.isoformat(), str(t))

        t = self.theclass(microsecond=100)
        self.assertEqual(t.isoformat(), "00:00:00.000100")
        self.assertEqual(t.isoformat(), str(t))

        t = self.theclass(microsecond=1000)
        self.assertEqual(t.isoformat(), "00:00:00.001000")
        self.assertEqual(t.isoformat(), str(t))

        t = self.theclass(microsecond=10000)
        self.assertEqual(t.isoformat(), "00:00:00.010000")
        self.assertEqual(t.isoformat(), str(t))

        t = self.theclass(microsecond=100000)
        self.assertEqual(t.isoformat(), "00:00:00.100000")
        self.assertEqual(t.isoformat(), str(t))

    def test_1653736(self):
        # verify it doesn't accept extra keyword arguments
        t = self.theclass(second=1)
        self.assertRaises(TypeError, t.isoformat, foo=3)

    def test_strftime(self):
        t = self.theclass(1, 2, 3, 4)
        self.assertEqual(t.strftime('%H %M %S %f'), "01 02 03 000004")
        # A naive object replaces %z and %Z with empty strings.
        self.assertEqual(t.strftime("'%z' '%Z'"), "'' ''")

    def test_format(self):
        t = self.theclass(1, 2, 3, 4)
        self.assertEqual(t.__format__(''), str(t))

        # check that a derived class's __str__() gets called
        class A(self.theclass):
            def __str__(self):
                return 'A'
        a = A(1, 2, 3, 4)
        self.assertEqual(a.__format__(''), 'A')

        # check that a derived class's strftime gets called
        class B(self.theclass):
            def strftime(self, format_spec):
                return 'B'
        b = B(1, 2, 3, 4)
        self.assertEqual(b.__format__(''), str(t))

        for fmt in ['%H %M %S',
                    ]:
            self.assertEqual(t.__format__(fmt), t.strftime(fmt))
            self.assertEqual(a.__format__(fmt), t.strftime(fmt))
            self.assertEqual(b.__format__(fmt), 'B')

    def test_str(self):
        self.assertEqual(str(self.theclass(1, 2, 3, 4)), "01:02:03.000004")
        self.assertEqual(str(self.theclass(10, 2, 3, 4000)), "10:02:03.004000")
        self.assertEqual(str(self.theclass(0, 2, 3, 400000)), "00:02:03.400000")
        self.assertEqual(str(self.theclass(12, 2, 3, 0)), "12:02:03")
        self.assertEqual(str(self.theclass(23, 15, 0, 0)), "23:15:00")

    def test_repr(self):
        name = 'datetime.' + self.theclass.__name__
        self.assertEqual(repr(self.theclass(1, 2, 3, 4)),
                         "%s(1, 2, 3, 4)" % name)
        self.assertEqual(repr(self.theclass(10, 2, 3, 4000)),
                         "%s(10, 2, 3, 4000)" % name)
        self.assertEqual(repr(self.theclass(0, 2, 3, 400000)),
                         "%s(0, 2, 3, 400000)" % name)
        self.assertEqual(repr(self.theclass(12, 2, 3, 0)),
                         "%s(12, 2, 3)" % name)
        self.assertEqual(repr(self.theclass(23, 15, 0, 0)),
                         "%s(23, 15)" % name)

    def test_resolution_info(self):
        self.assertIsInstance(self.theclass.min, self.theclass)
        self.assertIsInstance(self.theclass.max, self.theclass)
        self.assertIsInstance(self.theclass.resolution, timedelta)
        self.assertTrue(self.theclass.max > self.theclass.min)

    def test_pickling(self):
        args = 20, 59, 16, 64**2
        orig = self.theclass(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

    def test_pickling_subclass_time(self):
        args = 20, 59, 16, 64**2
        orig = SubclassTime(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

    def test_bool(self):
        cls = self.theclass
        self.assertTrue(cls(1))
        self.assertTrue(cls(0, 1))
        self.assertTrue(cls(0, 0, 1))
        self.assertTrue(cls(0, 0, 0, 1))
        self.assertTrue(not cls(0))
        self.assertTrue(not cls())

    def test_replace(self):
        cls = self.theclass
        args = [1, 2, 3, 4]
        base = cls(*args)
        self.assertEqual(base, base.replace())

        i = 0
        for name, newval in (("hour", 5),
                             ("minute", 6),
                             ("second", 7),
                             ("microsecond", 8)):
            newargs = args[:]
            newargs[i] = newval
            expected = cls(*newargs)
            got = base.replace(**{name: newval})
            self.assertEqual(expected, got)
            i += 1

        # Out of bounds.
        base = cls(1)
        self.assertRaises(ValueError, base.replace, hour=24)
        self.assertRaises(ValueError, base.replace, minute=-1)
        self.assertRaises(ValueError, base.replace, second=100)
        self.assertRaises(ValueError, base.replace, microsecond=1000000)

    def test_subclass_time(self):

        class C(self.theclass):
            theAnswer = 42

            def __new__(cls, *args, **kws):
                temp = kws.copy()
                extra = temp.pop('extra')
                result = self.theclass.__new__(cls, *args, **temp)
                result.extra = extra
                return result

            def newmeth(self, start):
                return start + self.hour + self.second

        args = 4, 5, 6

        dt1 = self.theclass(*args)
        dt2 = C(*args, **{'extra': 7})

        self.assertEqual(dt2.__class__, C)
        self.assertEqual(dt2.theAnswer, 42)
        self.assertEqual(dt2.extra, 7)
        self.assertEqual(dt1.isoformat(), dt2.isoformat())
        self.assertEqual(dt2.newmeth(-7), dt1.hour + dt1.second - 7)

    def test_backdoor_resistance(self):
        # see TestDate.test_backdoor_resistance().
        base = '2:59.0'
        for hour_byte in ' ', '9', chr(24), '\xff':
            self.assertRaises(TypeError, self.theclass,
                                         hour_byte + base[1:])

# A mixin for classes with a tzinfo= argument.  Subclasses must define
# theclass as a class atribute, and theclass(1, 1, 1, tzinfo=whatever)
# must be legit (which is true for time and datetime).
class TZInfoBase:

    def test_argument_passing(self):
        cls = self.theclass
        # A datetime passes itself on, a time passes None.
        class introspective(tzinfo):
            def tzname(self, dt):    return dt and "real" or "none"
            def utcoffset(self, dt):
                return timedelta(minutes = dt and 42 or -42)
            dst = utcoffset

        obj = cls(1, 2, 3, tzinfo=introspective())

        expected = cls is time and "none" or "real"
        self.assertEqual(obj.tzname(), expected)

        expected = timedelta(minutes=(cls is time and -42 or 42))
        self.assertEqual(obj.utcoffset(), expected)
        self.assertEqual(obj.dst(), expected)

    def test_bad_tzinfo_classes(self):
        cls = self.theclass
        self.assertRaises(TypeError, cls, 1, 1, 1, tzinfo=12)

        class NiceTry(object):
            def __init__(self): pass
            def utcoffset(self, dt): pass
        self.assertRaises(TypeError, cls, 1, 1, 1, tzinfo=NiceTry)

        class BetterTry(tzinfo):
            def __init__(self): pass
            def utcoffset(self, dt): pass
        b = BetterTry()
        t = cls(1, 1, 1, tzinfo=b)
        self.assertTrue(t.tzinfo is b)

    def test_utc_offset_out_of_bounds(self):
        class Edgy(tzinfo):
            def __init__(self, offset):
                self.offset = timedelta(minutes=offset)
            def utcoffset(self, dt):
                return self.offset

        cls = self.theclass
        for offset, legit in ((-1440, False),
                              (-1439, True),
                              (1439, True),
                              (1440, False)):
            if cls is time:
                t = cls(1, 2, 3, tzinfo=Edgy(offset))
            elif cls is datetime:
                t = cls(6, 6, 6, 1, 2, 3, tzinfo=Edgy(offset))
            else:
                assert 0, "impossible"
            if legit:
                aofs = abs(offset)
                h, m = divmod(aofs, 60)
                tag = "%c%02d:%02d" % (offset < 0 and '-' or '+', h, m)
                if isinstance(t, datetime):
                    t = t.timetz()
                self.assertEqual(str(t), "01:02:03" + tag)
            else:
                self.assertRaises(ValueError, str, t)

    def test_tzinfo_classes(self):
        cls = self.theclass
        class C1(tzinfo):
            def utcoffset(self, dt): return None
            def dst(self, dt): return None
            def tzname(self, dt): return None
        for t in (cls(1, 1, 1),
                  cls(1, 1, 1, tzinfo=None),
                  cls(1, 1, 1, tzinfo=C1())):
            self.assertTrue(t.utcoffset() is None)
            self.assertTrue(t.dst() is None)
            self.assertTrue(t.tzname() is None)

        class C3(tzinfo):
            def utcoffset(self, dt): return timedelta(minutes=-1439)
            def dst(self, dt): return timedelta(minutes=1439)
            def tzname(self, dt): return "aname"
        t = cls(1, 1, 1, tzinfo=C3())
        self.assertEqual(t.utcoffset(), timedelta(minutes=-1439))
        self.assertEqual(t.dst(), timedelta(minutes=1439))
        self.assertEqual(t.tzname(), "aname")

        # Wrong types.
        class C4(tzinfo):
            def utcoffset(self, dt): return "aname"
            def dst(self, dt): return 7
            def tzname(self, dt): return 0
        t = cls(1, 1, 1, tzinfo=C4())
        self.assertRaises(TypeError, t.utcoffset)
        self.assertRaises(TypeError, t.dst)
        self.assertRaises(TypeError, t.tzname)

        # Offset out of range.
        class C6(tzinfo):
            def utcoffset(self, dt): return timedelta(hours=-24)
            def dst(self, dt): return timedelta(hours=24)
        t = cls(1, 1, 1, tzinfo=C6())
        self.assertRaises(ValueError, t.utcoffset)
        self.assertRaises(ValueError, t.dst)

        # Not a whole number of minutes.
        class C7(tzinfo):
            def utcoffset(self, dt): return timedelta(seconds=61)
            def dst(self, dt): return timedelta(microseconds=-81)
        t = cls(1, 1, 1, tzinfo=C7())
        self.assertRaises(ValueError, t.utcoffset)
        self.assertRaises(ValueError, t.dst)

    def test_aware_compare(self):
        cls = self.theclass

        # Ensure that utcoffset() gets ignored if the comparands have
        # the same tzinfo member.
        class OperandDependentOffset(tzinfo):
            def utcoffset(self, t):
                if t.minute < 10:
                    # d0 and d1 equal after adjustment
                    return timedelta(minutes=t.minute)
                else:
                    # d2 off in the weeds
                    return timedelta(minutes=59)

        base = cls(8, 9, 10, tzinfo=OperandDependentOffset())
        d0 = base.replace(minute=3)
        d1 = base.replace(minute=9)
        d2 = base.replace(minute=11)
        for x in d0, d1, d2:
            for y in d0, d1, d2:
                for op in lt, le, gt, ge, eq, ne:
                    got = op(x, y)
                    expected = op(x.minute, y.minute)
                    self.assertEqual(got, expected)

        # However, if they're different members, uctoffset is not ignored.
        # Note that a time can't actually have an operand-depedent offset,
        # though (and time.utcoffset() passes None to tzinfo.utcoffset()),
        # so skip this test for time.
        if cls is not time:
            d0 = base.replace(minute=3, tzinfo=OperandDependentOffset())
            d1 = base.replace(minute=9, tzinfo=OperandDependentOffset())
            d2 = base.replace(minute=11, tzinfo=OperandDependentOffset())
            for x in d0, d1, d2:
                for y in d0, d1, d2:
                    got = (x > y) - (x < y)
                    if (x is d0 or x is d1) and (y is d0 or y is d1):
                        expected = 0
                    elif x is y is d2:
                        expected = 0
                    elif x is d2:
                        expected = -1
                    else:
                        assert y is d2
                        expected = 1
                    self.assertEqual(got, expected)


# Testing time objects with a non-None tzinfo.
class TestTimeTZ(TestTime, TZInfoBase, unittest.TestCase):
    theclass = time

    def test_empty(self):
        t = self.theclass()
        self.assertEqual(t.hour, 0)
        self.assertEqual(t.minute, 0)
        self.assertEqual(t.second, 0)
        self.assertEqual(t.microsecond, 0)
        self.assertTrue(t.tzinfo is None)

    def test_zones(self):
        est = FixedOffset(-300, "EST", 1)
        utc = FixedOffset(0, "UTC", -2)
        met = FixedOffset(60, "MET", 3)
        t1 = time( 7, 47, tzinfo=est)
        t2 = time(12, 47, tzinfo=utc)
        t3 = time(13, 47, tzinfo=met)
        t4 = time(microsecond=40)
        t5 = time(microsecond=40, tzinfo=utc)

        self.assertEqual(t1.tzinfo, est)
        self.assertEqual(t2.tzinfo, utc)
        self.assertEqual(t3.tzinfo, met)
        self.assertTrue(t4.tzinfo is None)
        self.assertEqual(t5.tzinfo, utc)

        self.assertEqual(t1.utcoffset(), timedelta(minutes=-300))
        self.assertEqual(t2.utcoffset(), timedelta(minutes=0))
        self.assertEqual(t3.utcoffset(), timedelta(minutes=60))
        self.assertTrue(t4.utcoffset() is None)
        self.assertRaises(TypeError, t1.utcoffset, "no args")

        self.assertEqual(t1.tzname(), "EST")
        self.assertEqual(t2.tzname(), "UTC")
        self.assertEqual(t3.tzname(), "MET")
        self.assertTrue(t4.tzname() is None)
        self.assertRaises(TypeError, t1.tzname, "no args")

        self.assertEqual(t1.dst(), timedelta(minutes=1))
        self.assertEqual(t2.dst(), timedelta(minutes=-2))
        self.assertEqual(t3.dst(), timedelta(minutes=3))
        self.assertTrue(t4.dst() is None)
        self.assertRaises(TypeError, t1.dst, "no args")

        self.assertEqual(hash(t1), hash(t2))
        self.assertEqual(hash(t1), hash(t3))
        self.assertEqual(hash(t2), hash(t3))

        self.assertEqual(t1, t2)
        self.assertEqual(t1, t3)
        self.assertEqual(t2, t3)
        self.assertNotEqual(t4, t5) # mixed tz-aware & naive
        self.assertRaises(TypeError, lambda: t4 < t5) # mixed tz-aware & naive
        self.assertRaises(TypeError, lambda: t5 < t4) # mixed tz-aware & naive

        self.assertEqual(str(t1), "07:47:00-05:00")
        self.assertEqual(str(t2), "12:47:00+00:00")
        self.assertEqual(str(t3), "13:47:00+01:00")
        self.assertEqual(str(t4), "00:00:00.000040")
        self.assertEqual(str(t5), "00:00:00.000040+00:00")

        self.assertEqual(t1.isoformat(), "07:47:00-05:00")
        self.assertEqual(t2.isoformat(), "12:47:00+00:00")
        self.assertEqual(t3.isoformat(), "13:47:00+01:00")
        self.assertEqual(t4.isoformat(), "00:00:00.000040")
        self.assertEqual(t5.isoformat(), "00:00:00.000040+00:00")

        d = 'datetime.time'
        self.assertEqual(repr(t1), d + "(7, 47, tzinfo=est)")
        self.assertEqual(repr(t2), d + "(12, 47, tzinfo=utc)")
        self.assertEqual(repr(t3), d + "(13, 47, tzinfo=met)")
        self.assertEqual(repr(t4), d + "(0, 0, 0, 40)")
        self.assertEqual(repr(t5), d + "(0, 0, 0, 40, tzinfo=utc)")

        self.assertEqual(t1.strftime("%H:%M:%S %%Z=%Z %%z=%z"),
                                     "07:47:00 %Z=EST %z=-0500")
        self.assertEqual(t2.strftime("%H:%M:%S %Z %z"), "12:47:00 UTC +0000")
        self.assertEqual(t3.strftime("%H:%M:%S %Z %z"), "13:47:00 MET +0100")

        yuck = FixedOffset(-1439, "%z %Z %%z%%Z")
        t1 = time(23, 59, tzinfo=yuck)
        self.assertEqual(t1.strftime("%H:%M %%Z='%Z' %%z='%z'"),
                                     "23:59 %Z='%z %Z %%z%%Z' %z='-2359'")

        # Check that an invalid tzname result raises an exception.
        class Badtzname(tzinfo):
            tz = 42
            def tzname(self, dt): return self.tz
        t = time(2, 3, 4, tzinfo=Badtzname())
        self.assertEqual(t.strftime("%H:%M:%S"), "02:03:04")
        self.assertRaises(TypeError, t.strftime, "%Z")

        # Issue #6697:
        if '_Fast' in str(type(self)):
            Badtzname.tz = '\ud800'
            self.assertRaises(ValueError, t.strftime, "%Z")

    def test_hash_edge_cases(self):
        # Offsets that overflow a basic time.
        t1 = self.theclass(0, 1, 2, 3, tzinfo=FixedOffset(1439, ""))
        t2 = self.theclass(0, 0, 2, 3, tzinfo=FixedOffset(1438, ""))
        self.assertEqual(hash(t1), hash(t2))

        t1 = self.theclass(23, 58, 6, 100, tzinfo=FixedOffset(-1000, ""))
        t2 = self.theclass(23, 48, 6, 100, tzinfo=FixedOffset(-1010, ""))
        self.assertEqual(hash(t1), hash(t2))

    def test_pickling(self):
        # Try one without a tzinfo.
        args = 20, 59, 16, 64**2
        orig = self.theclass(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

        # Try one with a tzinfo.
        tinfo = PicklableFixedOffset(-300, 'cookie')
        orig = self.theclass(5, 6, 7, tzinfo=tinfo)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)
            self.assertIsInstance(derived.tzinfo, PicklableFixedOffset)
            self.assertEqual(derived.utcoffset(), timedelta(minutes=-300))
            self.assertEqual(derived.tzname(), 'cookie')

    def test_more_bool(self):
        # Test cases with non-None tzinfo.
        cls = self.theclass

        t = cls(0, tzinfo=FixedOffset(-300, ""))
        self.assertTrue(t)

        t = cls(5, tzinfo=FixedOffset(-300, ""))
        self.assertTrue(t)

        t = cls(5, tzinfo=FixedOffset(300, ""))
        self.assertTrue(not t)

        t = cls(23, 59, tzinfo=FixedOffset(23*60 + 59, ""))
        self.assertTrue(not t)

        # Mostly ensuring this doesn't overflow internally.
        t = cls(0, tzinfo=FixedOffset(23*60 + 59, ""))
        self.assertTrue(t)

        # But this should yield a value error -- the utcoffset is bogus.
        t = cls(0, tzinfo=FixedOffset(24*60, ""))
        self.assertRaises(ValueError, lambda: bool(t))

        # Likewise.
        t = cls(0, tzinfo=FixedOffset(-24*60, ""))
        self.assertRaises(ValueError, lambda: bool(t))

    def test_replace(self):
        cls = self.theclass
        z100 = FixedOffset(100, "+100")
        zm200 = FixedOffset(timedelta(minutes=-200), "-200")
        args = [1, 2, 3, 4, z100]
        base = cls(*args)
        self.assertEqual(base, base.replace())

        i = 0
        for name, newval in (("hour", 5),
                             ("minute", 6),
                             ("second", 7),
                             ("microsecond", 8),
                             ("tzinfo", zm200)):
            newargs = args[:]
            newargs[i] = newval
            expected = cls(*newargs)
            got = base.replace(**{name: newval})
            self.assertEqual(expected, got)
            i += 1

        # Ensure we can get rid of a tzinfo.
        self.assertEqual(base.tzname(), "+100")
        base2 = base.replace(tzinfo=None)
        self.assertTrue(base2.tzinfo is None)
        self.assertTrue(base2.tzname() is None)

        # Ensure we can add one.
        base3 = base2.replace(tzinfo=z100)
        self.assertEqual(base, base3)
        self.assertTrue(base.tzinfo is base3.tzinfo)

        # Out of bounds.
        base = cls(1)
        self.assertRaises(ValueError, base.replace, hour=24)
        self.assertRaises(ValueError, base.replace, minute=-1)
        self.assertRaises(ValueError, base.replace, second=100)
        self.assertRaises(ValueError, base.replace, microsecond=1000000)

    def test_mixed_compare(self):
        t1 = time(1, 2, 3)
        t2 = time(1, 2, 3)
        self.assertEqual(t1, t2)
        t2 = t2.replace(tzinfo=None)
        self.assertEqual(t1, t2)
        t2 = t2.replace(tzinfo=FixedOffset(None, ""))
        self.assertEqual(t1, t2)
        t2 = t2.replace(tzinfo=FixedOffset(0, ""))
        self.assertNotEqual(t1, t2)

        # In time w/ identical tzinfo objects, utcoffset is ignored.
        class Varies(tzinfo):
            def __init__(self):
                self.offset = timedelta(minutes=22)
            def utcoffset(self, t):
                self.offset += timedelta(minutes=1)
                return self.offset

        v = Varies()
        t1 = t2.replace(tzinfo=v)
        t2 = t2.replace(tzinfo=v)
        self.assertEqual(t1.utcoffset(), timedelta(minutes=23))
        self.assertEqual(t2.utcoffset(), timedelta(minutes=24))
        self.assertEqual(t1, t2)

        # But if they're not identical, it isn't ignored.
        t2 = t2.replace(tzinfo=Varies())
        self.assertTrue(t1 < t2)  # t1's offset counter still going up

    def test_subclass_timetz(self):

        class C(self.theclass):
            theAnswer = 42

            def __new__(cls, *args, **kws):
                temp = kws.copy()
                extra = temp.pop('extra')
                result = self.theclass.__new__(cls, *args, **temp)
                result.extra = extra
                return result

            def newmeth(self, start):
                return start + self.hour + self.second

        args = 4, 5, 6, 500, FixedOffset(-300, "EST", 1)

        dt1 = self.theclass(*args)
        dt2 = C(*args, **{'extra': 7})

        self.assertEqual(dt2.__class__, C)
        self.assertEqual(dt2.theAnswer, 42)
        self.assertEqual(dt2.extra, 7)
        self.assertEqual(dt1.utcoffset(), dt2.utcoffset())
        self.assertEqual(dt2.newmeth(-7), dt1.hour + dt1.second - 7)


# Testing datetime objects with a non-None tzinfo.

class TestDateTimeTZ(TestDateTime, TZInfoBase, unittest.TestCase):
    theclass = datetime

    def test_trivial(self):
        dt = self.theclass(1, 2, 3, 4, 5, 6, 7)
        self.assertEqual(dt.year, 1)
        self.assertEqual(dt.month, 2)
        self.assertEqual(dt.day, 3)
        self.assertEqual(dt.hour, 4)
        self.assertEqual(dt.minute, 5)
        self.assertEqual(dt.second, 6)
        self.assertEqual(dt.microsecond, 7)
        self.assertEqual(dt.tzinfo, None)

    def test_even_more_compare(self):
        # The test_compare() and test_more_compare() inherited from TestDate
        # and TestDateTime covered non-tzinfo cases.

        # Smallest possible after UTC adjustment.
        t1 = self.theclass(1, 1, 1, tzinfo=FixedOffset(1439, ""))
        # Largest possible after UTC adjustment.
        t2 = self.theclass(MAXYEAR, 12, 31, 23, 59, 59, 999999,
                           tzinfo=FixedOffset(-1439, ""))

        # Make sure those compare correctly, and w/o overflow.
        self.assertTrue(t1 < t2)
        self.assertTrue(t1 != t2)
        self.assertTrue(t2 > t1)

        self.assertEqual(t1, t1)
        self.assertEqual(t2, t2)

        # Equal afer adjustment.
        t1 = self.theclass(1, 12, 31, 23, 59, tzinfo=FixedOffset(1, ""))
        t2 = self.theclass(2, 1, 1, 3, 13, tzinfo=FixedOffset(3*60+13+2, ""))
        self.assertEqual(t1, t2)

        # Change t1 not to subtract a minute, and t1 should be larger.
        t1 = self.theclass(1, 12, 31, 23, 59, tzinfo=FixedOffset(0, ""))
        self.assertTrue(t1 > t2)

        # Change t1 to subtract 2 minutes, and t1 should be smaller.
        t1 = self.theclass(1, 12, 31, 23, 59, tzinfo=FixedOffset(2, ""))
        self.assertTrue(t1 < t2)

        # Back to the original t1, but make seconds resolve it.
        t1 = self.theclass(1, 12, 31, 23, 59, tzinfo=FixedOffset(1, ""),
                           second=1)
        self.assertTrue(t1 > t2)

        # Likewise, but make microseconds resolve it.
        t1 = self.theclass(1, 12, 31, 23, 59, tzinfo=FixedOffset(1, ""),
                           microsecond=1)
        self.assertTrue(t1 > t2)

        # Make t2 naive and it should differ.
        t2 = self.theclass.min
        self.assertNotEqual(t1, t2)
        self.assertEqual(t2, t2)

        # It's also naive if it has tzinfo but tzinfo.utcoffset() is None.
        class Naive(tzinfo):
            def utcoffset(self, dt): return None
        t2 = self.theclass(5, 6, 7, tzinfo=Naive())
        self.assertNotEqual(t1, t2)
        self.assertEqual(t2, t2)

        # OTOH, it's OK to compare two of these mixing the two ways of being
        # naive.
        t1 = self.theclass(5, 6, 7)
        self.assertEqual(t1, t2)

        # Try a bogus uctoffset.
        class Bogus(tzinfo):
            def utcoffset(self, dt):
                return timedelta(minutes=1440) # out of bounds
        t1 = self.theclass(2, 2, 2, tzinfo=Bogus())
        t2 = self.theclass(2, 2, 2, tzinfo=FixedOffset(0, ""))
        self.assertRaises(ValueError, lambda: t1 == t2)

    def test_pickling(self):
        # Try one without a tzinfo.
        args = 6, 7, 23, 20, 59, 1, 64**2
        orig = self.theclass(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)

        # Try one with a tzinfo.
        tinfo = PicklableFixedOffset(-300, 'cookie')
        orig = self.theclass(*args, **{'tzinfo': tinfo})
        derived = self.theclass(1, 1, 1, tzinfo=FixedOffset(0, "", 0))
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)
            self.assertIsInstance(derived.tzinfo, PicklableFixedOffset)
            self.assertEqual(derived.utcoffset(), timedelta(minutes=-300))
            self.assertEqual(derived.tzname(), 'cookie')

    def test_extreme_hashes(self):
        # If an attempt is made to hash these via subtracting the offset
        # then hashing a datetime object, OverflowError results.  The
        # Python implementation used to blow up here.
        t = self.theclass(1, 1, 1, tzinfo=FixedOffset(1439, ""))
        hash(t)
        t = self.theclass(MAXYEAR, 12, 31, 23, 59, 59, 999999,
                          tzinfo=FixedOffset(-1439, ""))
        hash(t)

        # OTOH, an OOB offset should blow up.
        t = self.theclass(5, 5, 5, tzinfo=FixedOffset(-1440, ""))
        self.assertRaises(ValueError, hash, t)

    def test_zones(self):
        est = FixedOffset(-300, "EST")
        utc = FixedOffset(0, "UTC")
        met = FixedOffset(60, "MET")
        t1 = datetime(2002, 3, 19,  7, 47, tzinfo=est)
        t2 = datetime(2002, 3, 19, 12, 47, tzinfo=utc)
        t3 = datetime(2002, 3, 19, 13, 47, tzinfo=met)
        self.assertEqual(t1.tzinfo, est)
        self.assertEqual(t2.tzinfo, utc)
        self.assertEqual(t3.tzinfo, met)
        self.assertEqual(t1.utcoffset(), timedelta(minutes=-300))
        self.assertEqual(t2.utcoffset(), timedelta(minutes=0))
        self.assertEqual(t3.utcoffset(), timedelta(minutes=60))
        self.assertEqual(t1.tzname(), "EST")
        self.assertEqual(t2.tzname(), "UTC")
        self.assertEqual(t3.tzname(), "MET")
        self.assertEqual(hash(t1), hash(t2))
        self.assertEqual(hash(t1), hash(t3))
        self.assertEqual(hash(t2), hash(t3))
        self.assertEqual(t1, t2)
        self.assertEqual(t1, t3)
        self.assertEqual(t2, t3)
        self.assertEqual(str(t1), "2002-03-19 07:47:00-05:00")
        self.assertEqual(str(t2), "2002-03-19 12:47:00+00:00")
        self.assertEqual(str(t3), "2002-03-19 13:47:00+01:00")
        d = 'datetime.datetime(2002, 3, 19, '
        self.assertEqual(repr(t1), d + "7, 47, tzinfo=est)")
        self.assertEqual(repr(t2), d + "12, 47, tzinfo=utc)")
        self.assertEqual(repr(t3), d + "13, 47, tzinfo=met)")

    def test_combine(self):
        met = FixedOffset(60, "MET")
        d = date(2002, 3, 4)
        tz = time(18, 45, 3, 1234, tzinfo=met)
        dt = datetime.combine(d, tz)
        self.assertEqual(dt, datetime(2002, 3, 4, 18, 45, 3, 1234,
                                        tzinfo=met))

    def test_extract(self):
        met = FixedOffset(60, "MET")
        dt = self.theclass(2002, 3, 4, 18, 45, 3, 1234, tzinfo=met)
        self.assertEqual(dt.date(), date(2002, 3, 4))
        self.assertEqual(dt.time(), time(18, 45, 3, 1234))
        self.assertEqual(dt.timetz(), time(18, 45, 3, 1234, tzinfo=met))

    def test_tz_aware_arithmetic(self):
        import random

        now = self.theclass.now()
        tz55 = FixedOffset(-330, "west 5:30")
        timeaware = now.time().replace(tzinfo=tz55)
        nowaware = self.theclass.combine(now.date(), timeaware)
        self.assertTrue(nowaware.tzinfo is tz55)
        self.assertEqual(nowaware.timetz(), timeaware)

        # Can't mix aware and non-aware.
        self.assertRaises(TypeError, lambda: now - nowaware)
        self.assertRaises(TypeError, lambda: nowaware - now)

        # And adding datetime's doesn't make sense, aware or not.
        self.assertRaises(TypeError, lambda: now + nowaware)
        self.assertRaises(TypeError, lambda: nowaware + now)
        self.assertRaises(TypeError, lambda: nowaware + nowaware)

        # Subtracting should yield 0.
        self.assertEqual(now - now, timedelta(0))
        self.assertEqual(nowaware - nowaware, timedelta(0))

        # Adding a delta should preserve tzinfo.
        delta = timedelta(weeks=1, minutes=12, microseconds=5678)
        nowawareplus = nowaware + delta
        self.assertTrue(nowaware.tzinfo is tz55)
        nowawareplus2 = delta + nowaware
        self.assertTrue(nowawareplus2.tzinfo is tz55)
        self.assertEqual(nowawareplus, nowawareplus2)

        # that - delta should be what we started with, and that - what we
        # started with should be delta.
        diff = nowawareplus - delta
        self.assertTrue(diff.tzinfo is tz55)
        self.assertEqual(nowaware, diff)
        self.assertRaises(TypeError, lambda: delta - nowawareplus)
        self.assertEqual(nowawareplus - nowaware, delta)

        # Make up a random timezone.
        tzr = FixedOffset(random.randrange(-1439, 1440), "randomtimezone")
        # Attach it to nowawareplus.
        nowawareplus = nowawareplus.replace(tzinfo=tzr)
        self.assertTrue(nowawareplus.tzinfo is tzr)
        # Make sure the difference takes the timezone adjustments into account.
        got = nowaware - nowawareplus
        # Expected:  (nowaware base - nowaware offset) -
        #            (nowawareplus base - nowawareplus offset) =
        #            (nowaware base - nowawareplus base) +
        #            (nowawareplus offset - nowaware offset) =
        #            -delta + nowawareplus offset - nowaware offset
        expected = nowawareplus.utcoffset() - nowaware.utcoffset() - delta
        self.assertEqual(got, expected)

        # Try max possible difference.
        min = self.theclass(1, 1, 1, tzinfo=FixedOffset(1439, "min"))
        max = self.theclass(MAXYEAR, 12, 31, 23, 59, 59, 999999,
                            tzinfo=FixedOffset(-1439, "max"))
        maxdiff = max - min
        self.assertEqual(maxdiff, self.theclass.max - self.theclass.min +
                                  timedelta(minutes=2*1439))
        # Different tzinfo, but the same offset
        tza = timezone(HOUR, 'A')
        tzb = timezone(HOUR, 'B')
        delta = min.replace(tzinfo=tza) - max.replace(tzinfo=tzb)
        self.assertEqual(delta, self.theclass.min - self.theclass.max)

    def test_tzinfo_now(self):
        meth = self.theclass.now
        # Ensure it doesn't require tzinfo (i.e., that this doesn't blow up).
        base = meth()
        # Try with and without naming the keyword.
        off42 = FixedOffset(42, "42")
        another = meth(off42)
        again = meth(tz=off42)
        self.assertTrue(another.tzinfo is again.tzinfo)
        self.assertEqual(another.utcoffset(), timedelta(minutes=42))
        # Bad argument with and w/o naming the keyword.
        self.assertRaises(TypeError, meth, 16)
        self.assertRaises(TypeError, meth, tzinfo=16)
        # Bad keyword name.
        self.assertRaises(TypeError, meth, tinfo=off42)
        # Too many args.
        self.assertRaises(TypeError, meth, off42, off42)

        # We don't know which time zone we're in, and don't have a tzinfo
        # class to represent it, so seeing whether a tz argument actually
        # does a conversion is tricky.
        utc = FixedOffset(0, "utc", 0)
        for weirdtz in [FixedOffset(timedelta(hours=15, minutes=58), "weirdtz", 0),
                        timezone(timedelta(hours=15, minutes=58), "weirdtz"),]:
            for dummy in range(3):
                now = datetime.now(weirdtz)
                self.assertTrue(now.tzinfo is weirdtz)
                utcnow = datetime.utcnow().replace(tzinfo=utc)
                now2 = utcnow.astimezone(weirdtz)
                if abs(now - now2) < timedelta(seconds=30):
                    break
                # Else the code is broken, or more than 30 seconds passed between
                # calls; assuming the latter, just try again.
            else:
                # Three strikes and we're out.
                self.fail("utcnow(), now(tz), or astimezone() may be broken")

    def test_tzinfo_fromtimestamp(self):
        import time
        meth = self.theclass.fromtimestamp
        ts = time.time()
        # Ensure it doesn't require tzinfo (i.e., that this doesn't blow up).
        base = meth(ts)
        # Try with and without naming the keyword.
        off42 = FixedOffset(42, "42")
        another = meth(ts, off42)
        again = meth(ts, tz=off42)
        self.assertTrue(another.tzinfo is again.tzinfo)
        self.assertEqual(another.utcoffset(), timedelta(minutes=42))
        # Bad argument with and w/o naming the keyword.
        self.assertRaises(TypeError, meth, ts, 16)
        self.assertRaises(TypeError, meth, ts, tzinfo=16)
        # Bad keyword name.
        self.assertRaises(TypeError, meth, ts, tinfo=off42)
        # Too many args.
        self.assertRaises(TypeError, meth, ts, off42, off42)
        # Too few args.
        self.assertRaises(TypeError, meth)

        # Try to make sure tz= actually does some conversion.
        timestamp = 1000000000
        utcdatetime = datetime.utcfromtimestamp(timestamp)
        # In POSIX (epoch 1970), that's 2001-09-09 01:46:40 UTC, give or take.
        # But on some flavor of Mac, it's nowhere near that.  So we can't have
        # any idea here what time that actually is, we can only test that
        # relative changes match.
        utcoffset = timedelta(hours=-15, minutes=39) # arbitrary, but not zero
        tz = FixedOffset(utcoffset, "tz", 0)
        expected = utcdatetime + utcoffset
        got = datetime.fromtimestamp(timestamp, tz)
        self.assertEqual(expected, got.replace(tzinfo=None))

    def test_tzinfo_utcnow(self):
        meth = self.theclass.utcnow
        # Ensure it doesn't require tzinfo (i.e., that this doesn't blow up).
        base = meth()
        # Try with and without naming the keyword; for whatever reason,
        # utcnow() doesn't accept a tzinfo argument.
        off42 = FixedOffset(42, "42")
        self.assertRaises(TypeError, meth, off42)
        self.assertRaises(TypeError, meth, tzinfo=off42)

    def test_tzinfo_utcfromtimestamp(self):
        import time
        meth = self.theclass.utcfromtimestamp
        ts = time.time()
        # Ensure it doesn't require tzinfo (i.e., that this doesn't blow up).
        base = meth(ts)
        # Try with and without naming the keyword; for whatever reason,
        # utcfromtimestamp() doesn't accept a tzinfo argument.
        off42 = FixedOffset(42, "42")
        self.assertRaises(TypeError, meth, ts, off42)
        self.assertRaises(TypeError, meth, ts, tzinfo=off42)

    def test_tzinfo_timetuple(self):
        # TestDateTime tested most of this.  datetime adds a twist to the
        # DST flag.
        class DST(tzinfo):
            def __init__(self, dstvalue):
                if isinstance(dstvalue, int):
                    dstvalue = timedelta(minutes=dstvalue)
                self.dstvalue = dstvalue
            def dst(self, dt):
                return self.dstvalue

        cls = self.theclass
        for dstvalue, flag in (-33, 1), (33, 1), (0, 0), (None, -1):
            d = cls(1, 1, 1, 10, 20, 30, 40, tzinfo=DST(dstvalue))
            t = d.timetuple()
            self.assertEqual(1, t.tm_year)
            self.assertEqual(1, t.tm_mon)
            self.assertEqual(1, t.tm_mday)
            self.assertEqual(10, t.tm_hour)
            self.assertEqual(20, t.tm_min)
            self.assertEqual(30, t.tm_sec)
            self.assertEqual(0, t.tm_wday)
            self.assertEqual(1, t.tm_yday)
            self.assertEqual(flag, t.tm_isdst)

        # dst() returns wrong type.
        self.assertRaises(TypeError, cls(1, 1, 1, tzinfo=DST("x")).timetuple)

        # dst() at the edge.
        self.assertEqual(cls(1,1,1, tzinfo=DST(1439)).timetuple().tm_isdst, 1)
        self.assertEqual(cls(1,1,1, tzinfo=DST(-1439)).timetuple().tm_isdst, 1)

        # dst() out of range.
        self.assertRaises(ValueError, cls(1,1,1, tzinfo=DST(1440)).timetuple)
        self.assertRaises(ValueError, cls(1,1,1, tzinfo=DST(-1440)).timetuple)

    def test_utctimetuple(self):
        class DST(tzinfo):
            def __init__(self, dstvalue=0):
                if isinstance(dstvalue, int):
                    dstvalue = timedelta(minutes=dstvalue)
                self.dstvalue = dstvalue
            def dst(self, dt):
                return self.dstvalue

        cls = self.theclass
        # This can't work:  DST didn't implement utcoffset.
        self.assertRaises(NotImplementedError,
                          cls(1, 1, 1, tzinfo=DST(0)).utcoffset)

        class UOFS(DST):
            def __init__(self, uofs, dofs=None):
                DST.__init__(self, dofs)
                self.uofs = timedelta(minutes=uofs)
            def utcoffset(self, dt):
                return self.uofs

        for dstvalue in -33, 33, 0, None:
            d = cls(1, 2, 3, 10, 20, 30, 40, tzinfo=UOFS(-53, dstvalue))
            t = d.utctimetuple()
            self.assertEqual(d.year, t.tm_year)
            self.assertEqual(d.month, t.tm_mon)
            self.assertEqual(d.day, t.tm_mday)
            self.assertEqual(11, t.tm_hour) # 20mm + 53mm = 1hn + 13mm
            self.assertEqual(13, t.tm_min)
            self.assertEqual(d.second, t.tm_sec)
            self.assertEqual(d.weekday(), t.tm_wday)
            self.assertEqual(d.toordinal() - date(1, 1, 1).toordinal() + 1,
                             t.tm_yday)
            # Ensure tm_isdst is 0 regardless of what dst() says: DST
            # is never in effect for a UTC time.
            self.assertEqual(0, t.tm_isdst)

        # For naive datetime, utctimetuple == timetuple except for isdst
        d = cls(1, 2, 3, 10, 20, 30, 40)
        t = d.utctimetuple()
        self.assertEqual(t[:-1], d.timetuple()[:-1])
        self.assertEqual(0, t.tm_isdst)
        # Same if utcoffset is None
        class NOFS(DST):
            def utcoffset(self, dt):
                return None
        d = cls(1, 2, 3, 10, 20, 30, 40, tzinfo=NOFS())
        t = d.utctimetuple()
        self.assertEqual(t[:-1], d.timetuple()[:-1])
        self.assertEqual(0, t.tm_isdst)
        # Check that bad tzinfo is detected
        class BOFS(DST):
            def utcoffset(self, dt):
                return "EST"
        d = cls(1, 2, 3, 10, 20, 30, 40, tzinfo=BOFS())
        self.assertRaises(TypeError, d.utctimetuple)

        # Check that utctimetuple() is the same as
        # astimezone(utc).timetuple()
        d = cls(2010, 11, 13, 14, 15, 16, 171819)
        for tz in [timezone.min, timezone.utc, timezone.max]:
            dtz = d.replace(tzinfo=tz)
            self.assertEqual(dtz.utctimetuple()[:-1],
                             dtz.astimezone(timezone.utc).timetuple()[:-1])
        # At the edges, UTC adjustment can produce years out-of-range
        # for a datetime object.  Ensure that an OverflowError is
        # raised.
        tiny = cls(MINYEAR, 1, 1, 0, 0, 37, tzinfo=UOFS(1439))
        # That goes back 1 minute less than a full day.
        self.assertRaises(OverflowError, tiny.utctimetuple)

        huge = cls(MAXYEAR, 12, 31, 23, 59, 37, 999999, tzinfo=UOFS(-1439))
        # That goes forward 1 minute less than a full day.
        self.assertRaises(OverflowError, huge.utctimetuple)
        # More overflow cases
        tiny = cls.min.replace(tzinfo=timezone(MINUTE))
        self.assertRaises(OverflowError, tiny.utctimetuple)
        huge = cls.max.replace(tzinfo=timezone(-MINUTE))
        self.assertRaises(OverflowError, huge.utctimetuple)

    def test_tzinfo_isoformat(self):
        zero = FixedOffset(0, "+00:00")
        plus = FixedOffset(220, "+03:40")
        minus = FixedOffset(-231, "-03:51")
        unknown = FixedOffset(None, "")

        cls = self.theclass
        datestr = '0001-02-03'
        for ofs in None, zero, plus, minus, unknown:
            for us in 0, 987001:
                d = cls(1, 2, 3, 4, 5, 59, us, tzinfo=ofs)
                timestr = '04:05:59' + (us and '.987001' or '')
                ofsstr = ofs is not None and d.tzname() or ''
                tailstr = timestr + ofsstr
                iso = d.isoformat()
                self.assertEqual(iso, datestr + 'T' + tailstr)
                self.assertEqual(iso, d.isoformat('T'))
                self.assertEqual(d.isoformat('k'), datestr + 'k' + tailstr)
                self.assertEqual(d.isoformat('\u1234'), datestr + '\u1234' + tailstr)
                self.assertEqual(str(d), datestr + ' ' + tailstr)

    def test_replace(self):
        cls = self.theclass
        z100 = FixedOffset(100, "+100")
        zm200 = FixedOffset(timedelta(minutes=-200), "-200")
        args = [1, 2, 3, 4, 5, 6, 7, z100]
        base = cls(*args)
        self.assertEqual(base, base.replace())

        i = 0
        for name, newval in (("year", 2),
                             ("month", 3),
                             ("day", 4),
                             ("hour", 5),
                             ("minute", 6),
                             ("second", 7),
                             ("microsecond", 8),
                             ("tzinfo", zm200)):
            newargs = args[:]
            newargs[i] = newval
            expected = cls(*newargs)
            got = base.replace(**{name: newval})
            self.assertEqual(expected, got)
            i += 1

        # Ensure we can get rid of a tzinfo.
        self.assertEqual(base.tzname(), "+100")
        base2 = base.replace(tzinfo=None)
        self.assertTrue(base2.tzinfo is None)
        self.assertTrue(base2.tzname() is None)

        # Ensure we can add one.
        base3 = base2.replace(tzinfo=z100)
        self.assertEqual(base, base3)
        self.assertTrue(base.tzinfo is base3.tzinfo)

        # Out of bounds.
        base = cls(2000, 2, 29)
        self.assertRaises(ValueError, base.replace, year=2001)

    def test_more_astimezone(self):
        # The inherited test_astimezone covered some trivial and error cases.
        fnone = FixedOffset(None, "None")
        f44m = FixedOffset(44, "44")
        fm5h = FixedOffset(-timedelta(hours=5), "m300")

        dt = self.theclass.now(tz=f44m)
        self.assertTrue(dt.tzinfo is f44m)
        # Replacing with degenerate tzinfo raises an exception.
        self.assertRaises(ValueError, dt.astimezone, fnone)
        # Replacing with same tzinfo makes no change.
        x = dt.astimezone(dt.tzinfo)
        self.assertTrue(x.tzinfo is f44m)
        self.assertEqual(x.date(), dt.date())
        self.assertEqual(x.time(), dt.time())

        # Replacing with different tzinfo does adjust.
        got = dt.astimezone(fm5h)
        self.assertTrue(got.tzinfo is fm5h)
        self.assertEqual(got.utcoffset(), timedelta(hours=-5))
        expected = dt - dt.utcoffset()  # in effect, convert to UTC
        expected += fm5h.utcoffset(dt)  # and from there to local time
        expected = expected.replace(tzinfo=fm5h) # and attach new tzinfo
        self.assertEqual(got.date(), expected.date())
        self.assertEqual(got.time(), expected.time())
        self.assertEqual(got.timetz(), expected.timetz())
        self.assertTrue(got.tzinfo is expected.tzinfo)
        self.assertEqual(got, expected)

    @support.run_with_tz('UTC')
    def test_astimezone_default_utc(self):
        dt = self.theclass.now(timezone.utc)
        self.assertEqual(dt.astimezone(None), dt)
        self.assertEqual(dt.astimezone(), dt)

    @support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_astimezone_default_eastern(self):
        dt = self.theclass(2012, 11, 4, 6, 30, tzinfo=timezone.utc)
        local = dt.astimezone()
        self.assertEqual(dt, local)
        self.assertEqual(local.strftime("%z %Z"), "+0500 EST") 
        dt = self.theclass(2012, 11, 4, 5, 30, tzinfo=timezone.utc)
        local = dt.astimezone()
        self.assertEqual(dt, local)
        self.assertEqual(local.strftime("%z %Z"), "+0400 EDT") 

    def test_aware_subtract(self):
        cls = self.theclass

        # Ensure that utcoffset() is ignored when the operands have the
        # same tzinfo member.
        class OperandDependentOffset(tzinfo):
            def utcoffset(self, t):
                if t.minute < 10:
                    # d0 and d1 equal after adjustment
                    return timedelta(minutes=t.minute)
                else:
                    # d2 off in the weeds
                    return timedelta(minutes=59)

        base = cls(8, 9, 10, 11, 12, 13, 14, tzinfo=OperandDependentOffset())
        d0 = base.replace(minute=3)
        d1 = base.replace(minute=9)
        d2 = base.replace(minute=11)
        for x in d0, d1, d2:
            for y in d0, d1, d2:
                got = x - y
                expected = timedelta(minutes=x.minute - y.minute)
                self.assertEqual(got, expected)

        # OTOH, if the tzinfo members are distinct, utcoffsets aren't
        # ignored.
        base = cls(8, 9, 10, 11, 12, 13, 14)
        d0 = base.replace(minute=3, tzinfo=OperandDependentOffset())
        d1 = base.replace(minute=9, tzinfo=OperandDependentOffset())
        d2 = base.replace(minute=11, tzinfo=OperandDependentOffset())
        for x in d0, d1, d2:
            for y in d0, d1, d2:
                got = x - y
                if (x is d0 or x is d1) and (y is d0 or y is d1):
                    expected = timedelta(0)
                elif x is y is d2:
                    expected = timedelta(0)
                elif x is d2:
                    expected = timedelta(minutes=(11-59)-0)
                else:
                    assert y is d2
                    expected = timedelta(minutes=0-(11-59))
                self.assertEqual(got, expected)

    def test_mixed_compare(self):
        t1 = datetime(1, 2, 3, 4, 5, 6, 7)
        t2 = datetime(1, 2, 3, 4, 5, 6, 7)
        self.assertEqual(t1, t2)
        t2 = t2.replace(tzinfo=None)
        self.assertEqual(t1, t2)
        t2 = t2.replace(tzinfo=FixedOffset(None, ""))
        self.assertEqual(t1, t2)
        t2 = t2.replace(tzinfo=FixedOffset(0, ""))
        self.assertNotEqual(t1, t2)

        # In datetime w/ identical tzinfo objects, utcoffset is ignored.
        class Varies(tzinfo):
            def __init__(self):
                self.offset = timedelta(minutes=22)
            def utcoffset(self, t):
                self.offset += timedelta(minutes=1)
                return self.offset

        v = Varies()
        t1 = t2.replace(tzinfo=v)
        t2 = t2.replace(tzinfo=v)
        self.assertEqual(t1.utcoffset(), timedelta(minutes=23))
        self.assertEqual(t2.utcoffset(), timedelta(minutes=24))
        self.assertEqual(t1, t2)

        # But if they're not identical, it isn't ignored.
        t2 = t2.replace(tzinfo=Varies())
        self.assertTrue(t1 < t2)  # t1's offset counter still going up

    def test_subclass_datetimetz(self):

        class C(self.theclass):
            theAnswer = 42

            def __new__(cls, *args, **kws):
                temp = kws.copy()
                extra = temp.pop('extra')
                result = self.theclass.__new__(cls, *args, **temp)
                result.extra = extra
                return result

            def newmeth(self, start):
                return start + self.hour + self.year

        args = 2002, 12, 31, 4, 5, 6, 500, FixedOffset(-300, "EST", 1)

        dt1 = self.theclass(*args)
        dt2 = C(*args, **{'extra': 7})

        self.assertEqual(dt2.__class__, C)
        self.assertEqual(dt2.theAnswer, 42)
        self.assertEqual(dt2.extra, 7)
        self.assertEqual(dt1.utcoffset(), dt2.utcoffset())
        self.assertEqual(dt2.newmeth(-7), dt1.hour + dt1.year - 7)

# Pain to set up DST-aware tzinfo classes.

def first_sunday_on_or_after(dt):
    days_to_go = 6 - dt.weekday()
    if days_to_go:
        dt += timedelta(days_to_go)
    return dt

ZERO = timedelta(0)
MINUTE = timedelta(minutes=1)
HOUR = timedelta(hours=1)
DAY = timedelta(days=1)
# In the US, DST starts at 2am (standard time) on the first Sunday in April.
DSTSTART = datetime(1, 4, 1, 2)
# and ends at 2am (DST time; 1am standard time) on the last Sunday of Oct,
# which is the first Sunday on or after Oct 25.  Because we view 1:MM as
# being standard time on that day, there is no spelling in local time of
# the last hour of DST (that's 1:MM DST, but 1:MM is taken as standard time).
DSTEND = datetime(1, 10, 25, 1)

class USTimeZone(tzinfo):

    def __init__(self, hours, reprname, stdname, dstname):
        self.stdoffset = timedelta(hours=hours)
        self.reprname = reprname
        self.stdname = stdname
        self.dstname = dstname

    def __repr__(self):
        return self.reprname

    def tzname(self, dt):
        if self.dst(dt):
            return self.dstname
        else:
            return self.stdname

    def utcoffset(self, dt):
        return self.stdoffset + self.dst(dt)

    def dst(self, dt):
        if dt is None or dt.tzinfo is None:
            # An exception instead may be sensible here, in one or more of
            # the cases.
            return ZERO
        assert dt.tzinfo is self

        # Find first Sunday in April.
        start = first_sunday_on_or_after(DSTSTART.replace(year=dt.year))
        assert start.weekday() == 6 and start.month == 4 and start.day <= 7

        # Find last Sunday in October.
        end = first_sunday_on_or_after(DSTEND.replace(year=dt.year))
        assert end.weekday() == 6 and end.month == 10 and end.day >= 25

        # Can't compare naive to aware objects, so strip the timezone from
        # dt first.
        if start <= dt.replace(tzinfo=None) < end:
            return HOUR
        else:
            return ZERO

Eastern  = USTimeZone(-5, "Eastern",  "EST", "EDT")
Central  = USTimeZone(-6, "Central",  "CST", "CDT")
Mountain = USTimeZone(-7, "Mountain", "MST", "MDT")
Pacific  = USTimeZone(-8, "Pacific",  "PST", "PDT")
utc_real = FixedOffset(0, "UTC", 0)
# For better test coverage, we want another flavor of UTC that's west of
# the Eastern and Pacific timezones.
utc_fake = FixedOffset(-12*60, "UTCfake", 0)

class TestTimezoneConversions(unittest.TestCase):
    # The DST switch times for 2002, in std time.
    dston = datetime(2002, 4, 7, 2)
    dstoff = datetime(2002, 10, 27, 1)

    theclass = datetime

    # Check a time that's inside DST.
    def checkinside(self, dt, tz, utc, dston, dstoff):
        self.assertEqual(dt.dst(), HOUR)

        # Conversion to our own timezone is always an identity.
        self.assertEqual(dt.astimezone(tz), dt)

        asutc = dt.astimezone(utc)
        there_and_back = asutc.astimezone(tz)

        # Conversion to UTC and back isn't always an identity here,
        # because there are redundant spellings (in local time) of
        # UTC time when DST begins:  the clock jumps from 1:59:59
        # to 3:00:00, and a local time of 2:MM:SS doesn't really
        # make sense then.  The classes above treat 2:MM:SS as
        # daylight time then (it's "after 2am"), really an alias
        # for 1:MM:SS standard time.  The latter form is what
        # conversion back from UTC produces.
        if dt.date() == dston.date() and dt.hour == 2:
            # We're in the redundant hour, and coming back from
            # UTC gives the 1:MM:SS standard-time spelling.
            self.assertEqual(there_and_back + HOUR, dt)
            # Although during was considered to be in daylight
            # time, there_and_back is not.
            self.assertEqual(there_and_back.dst(), ZERO)
            # They're the same times in UTC.
            self.assertEqual(there_and_back.astimezone(utc),
                             dt.astimezone(utc))
        else:
            # We're not in the redundant hour.
            self.assertEqual(dt, there_and_back)

        # Because we have a redundant spelling when DST begins, there is
        # (unfortunately) an hour when DST ends that can't be spelled at all in
        # local time.  When DST ends, the clock jumps from 1:59 back to 1:00
        # again.  The hour 1:MM DST has no spelling then:  1:MM is taken to be
        # standard time.  1:MM DST == 0:MM EST, but 0:MM is taken to be
        # daylight time.  The hour 1:MM daylight == 0:MM standard can't be
        # expressed in local time.  Nevertheless, we want conversion back
        # from UTC to mimic the local clock's "repeat an hour" behavior.
        nexthour_utc = asutc + HOUR
        nexthour_tz = nexthour_utc.astimezone(tz)
        if dt.date() == dstoff.date() and dt.hour == 0:
            # We're in the hour before the last DST hour.  The last DST hour
            # is ineffable.  We want the conversion back to repeat 1:MM.
            self.assertEqual(nexthour_tz, dt.replace(hour=1))
            nexthour_utc += HOUR
            nexthour_tz = nexthour_utc.astimezone(tz)
            self.assertEqual(nexthour_tz, dt.replace(hour=1))
        else:
            self.assertEqual(nexthour_tz - dt, HOUR)

    # Check a time that's outside DST.
    def checkoutside(self, dt, tz, utc):
        self.assertEqual(dt.dst(), ZERO)

        # Conversion to our own timezone is always an identity.
        self.assertEqual(dt.astimezone(tz), dt)

        # Converting to UTC and back is an identity too.
        asutc = dt.astimezone(utc)
        there_and_back = asutc.astimezone(tz)
        self.assertEqual(dt, there_and_back)

    def convert_between_tz_and_utc(self, tz, utc):
        dston = self.dston.replace(tzinfo=tz)
        # Because 1:MM on the day DST ends is taken as being standard time,
        # there is no spelling in tz for the last hour of daylight time.
        # For purposes of the test, the last hour of DST is 0:MM, which is
        # taken as being daylight time (and 1:MM is taken as being standard
        # time).
        dstoff = self.dstoff.replace(tzinfo=tz)
        for delta in (timedelta(weeks=13),
                      DAY,
                      HOUR,
                      timedelta(minutes=1),
                      timedelta(microseconds=1)):

            self.checkinside(dston, tz, utc, dston, dstoff)
            for during in dston + delta, dstoff - delta:
                self.checkinside(during, tz, utc, dston, dstoff)

            self.checkoutside(dstoff, tz, utc)
            for outside in dston - delta, dstoff + delta:
                self.checkoutside(outside, tz, utc)

    def test_easy(self):
        # Despite the name of this test, the endcases are excruciating.
        self.convert_between_tz_and_utc(Eastern, utc_real)
        self.convert_between_tz_and_utc(Pacific, utc_real)
        self.convert_between_tz_and_utc(Eastern, utc_fake)
        self.convert_between_tz_and_utc(Pacific, utc_fake)
        # The next is really dancing near the edge.  It works because
        # Pacific and Eastern are far enough apart that their "problem
        # hours" don't overlap.
        self.convert_between_tz_and_utc(Eastern, Pacific)
        self.convert_between_tz_and_utc(Pacific, Eastern)
        # OTOH, these fail!  Don't enable them.  The difficulty is that
        # the edge case tests assume that every hour is representable in
        # the "utc" class.  This is always true for a fixed-offset tzinfo
        # class (lke utc_real and utc_fake), but not for Eastern or Central.
        # For these adjacent DST-aware time zones, the range of time offsets
        # tested ends up creating hours in the one that aren't representable
        # in the other.  For the same reason, we would see failures in the
        # Eastern vs Pacific tests too if we added 3*HOUR to the list of
        # offset deltas in convert_between_tz_and_utc().
        #
        # self.convert_between_tz_and_utc(Eastern, Central)  # can't work
        # self.convert_between_tz_and_utc(Central, Eastern)  # can't work

    def test_tricky(self):
        # 22:00 on day before daylight starts.
        fourback = self.dston - timedelta(hours=4)
        ninewest = FixedOffset(-9*60, "-0900", 0)
        fourback = fourback.replace(tzinfo=ninewest)
        # 22:00-0900 is 7:00 UTC == 2:00 EST == 3:00 DST.  Since it's "after
        # 2", we should get the 3 spelling.
        # If we plug 22:00 the day before into Eastern, it "looks like std
        # time", so its offset is returned as -5, and -5 - -9 = 4.  Adding 4
        # to 22:00 lands on 2:00, which makes no sense in local time (the
        # local clock jumps from 1 to 3).  The point here is to make sure we
        # get the 3 spelling.
        expected = self.dston.replace(hour=3)
        got = fourback.astimezone(Eastern).replace(tzinfo=None)
        self.assertEqual(expected, got)

        # Similar, but map to 6:00 UTC == 1:00 EST == 2:00 DST.  In that
        # case we want the 1:00 spelling.
        sixutc = self.dston.replace(hour=6, tzinfo=utc_real)
        # Now 6:00 "looks like daylight", so the offset wrt Eastern is -4,
        # and adding -4-0 == -4 gives the 2:00 spelling.  We want the 1:00 EST
        # spelling.
        expected = self.dston.replace(hour=1)
        got = sixutc.astimezone(Eastern).replace(tzinfo=None)
        self.assertEqual(expected, got)

        # Now on the day DST ends, we want "repeat an hour" behavior.
        #  UTC  4:MM  5:MM  6:MM  7:MM  checking these
        #  EST 23:MM  0:MM  1:MM  2:MM
        #  EDT  0:MM  1:MM  2:MM  3:MM
        # wall  0:MM  1:MM  1:MM  2:MM  against these
        for utc in utc_real, utc_fake:
            for tz in Eastern, Pacific:
                first_std_hour = self.dstoff - timedelta(hours=2) # 23:MM
                # Convert that to UTC.
                first_std_hour -= tz.utcoffset(None)
                # Adjust for possibly fake UTC.
                asutc = first_std_hour + utc.utcoffset(None)
                # First UTC hour to convert; this is 4:00 when utc=utc_real &
                # tz=Eastern.
                asutcbase = asutc.replace(tzinfo=utc)
                for tzhour in (0, 1, 1, 2):
                    expectedbase = self.dstoff.replace(hour=tzhour)
                    for minute in 0, 30, 59:
                        expected = expectedbase.replace(minute=minute)
                        asutc = asutcbase.replace(minute=minute)
                        astz = asutc.astimezone(tz)
                        self.assertEqual(astz.replace(tzinfo=None), expected)
                    asutcbase += HOUR


    def test_bogus_dst(self):
        class ok(tzinfo):
            def utcoffset(self, dt): return HOUR
            def dst(self, dt): return HOUR

        now = self.theclass.now().replace(tzinfo=utc_real)
        # Doesn't blow up.
        now.astimezone(ok())

        # Does blow up.
        class notok(ok):
            def dst(self, dt): return None
        self.assertRaises(ValueError, now.astimezone, notok())

        # Sometimes blow up. In the following, tzinfo.dst()
        # implementation may return None or not None depending on
        # whether DST is assumed to be in effect.  In this situation,
        # a ValueError should be raised by astimezone().
        class tricky_notok(ok):
            def dst(self, dt):
                if dt.year == 2000:
                    return None
                else:
                    return 10*HOUR
        dt = self.theclass(2001, 1, 1).replace(tzinfo=utc_real)
        self.assertRaises(ValueError, dt.astimezone, tricky_notok())

    def test_fromutc(self):
        self.assertRaises(TypeError, Eastern.fromutc)   # not enough args
        now = datetime.utcnow().replace(tzinfo=utc_real)
        self.assertRaises(ValueError, Eastern.fromutc, now) # wrong tzinfo
        now = now.replace(tzinfo=Eastern)   # insert correct tzinfo
        enow = Eastern.fromutc(now)         # doesn't blow up
        self.assertEqual(enow.tzinfo, Eastern) # has right tzinfo member
        self.assertRaises(TypeError, Eastern.fromutc, now, now) # too many args
        self.assertRaises(TypeError, Eastern.fromutc, date.today()) # wrong type

        # Always converts UTC to standard time.
        class FauxUSTimeZone(USTimeZone):
            def fromutc(self, dt):
                return dt + self.stdoffset
        FEastern  = FauxUSTimeZone(-5, "FEastern",  "FEST", "FEDT")

        #  UTC  4:MM  5:MM  6:MM  7:MM  8:MM  9:MM
        #  EST 23:MM  0:MM  1:MM  2:MM  3:MM  4:MM
        #  EDT  0:MM  1:MM  2:MM  3:MM  4:MM  5:MM

        # Check around DST start.
        start = self.dston.replace(hour=4, tzinfo=Eastern)
        fstart = start.replace(tzinfo=FEastern)
        for wall in 23, 0, 1, 3, 4, 5:
            expected = start.replace(hour=wall)
            if wall == 23:
                expected -= timedelta(days=1)
            got = Eastern.fromutc(start)
            self.assertEqual(expected, got)

            expected = fstart + FEastern.stdoffset
            got = FEastern.fromutc(fstart)
            self.assertEqual(expected, got)

            # Ensure astimezone() calls fromutc() too.
            got = fstart.replace(tzinfo=utc_real).astimezone(FEastern)
            self.assertEqual(expected, got)

            start += HOUR
            fstart += HOUR

        # Check around DST end.
        start = self.dstoff.replace(hour=4, tzinfo=Eastern)
        fstart = start.replace(tzinfo=FEastern)
        for wall in 0, 1, 1, 2, 3, 4:
            expected = start.replace(hour=wall)
            got = Eastern.fromutc(start)
            self.assertEqual(expected, got)

            expected = fstart + FEastern.stdoffset
            got = FEastern.fromutc(fstart)
            self.assertEqual(expected, got)

            # Ensure astimezone() calls fromutc() too.
            got = fstart.replace(tzinfo=utc_real).astimezone(FEastern)
            self.assertEqual(expected, got)

            start += HOUR
            fstart += HOUR


#############################################################################
# oddballs

class Oddballs(unittest.TestCase):

    def test_bug_1028306(self):
        # Trying to compare a date to a datetime should act like a mixed-
        # type comparison, despite that datetime is a subclass of date.
        as_date = date.today()
        as_datetime = datetime.combine(as_date, time())
        self.assertTrue(as_date != as_datetime)
        self.assertTrue(as_datetime != as_date)
        self.assertTrue(not as_date == as_datetime)
        self.assertTrue(not as_datetime == as_date)
        self.assertRaises(TypeError, lambda: as_date < as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime < as_date)
        self.assertRaises(TypeError, lambda: as_date <= as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime <= as_date)
        self.assertRaises(TypeError, lambda: as_date > as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime > as_date)
        self.assertRaises(TypeError, lambda: as_date >= as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime >= as_date)

        # Neverthelss, comparison should work with the base-class (date)
        # projection if use of a date method is forced.
        self.assertEqual(as_date.__eq__(as_datetime), True)
        different_day = (as_date.day + 1) % 20 + 1
        as_different = as_datetime.replace(day= different_day)
        self.assertEqual(as_date.__eq__(as_different), False)

        # And date should compare with other subclasses of date.  If a
        # subclass wants to stop this, it's up to the subclass to do so.
        date_sc = SubclassDate(as_date.year, as_date.month, as_date.day)
        self.assertEqual(as_date, date_sc)
        self.assertEqual(date_sc, as_date)

        # Ditto for datetimes.
        datetime_sc = SubclassDatetime(as_datetime.year, as_datetime.month,
                                       as_date.day, 0, 0, 0)
        self.assertEqual(as_datetime, datetime_sc)
        self.assertEqual(datetime_sc, as_datetime)

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
