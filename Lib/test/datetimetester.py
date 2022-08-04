"""Test date/time type.

See https://www.zope.dev/Members/fdrake/DateTimeWiki/TestCases
"""
import io
import itertools
import bisect
import copy
import decimal
import sys
import os
import pickle
import random
import re
import struct
import unittest

from array import array

from operator import lt, le, gt, ge, eq, ne, truediv, floordiv, mod

from test import support
from test.support import is_resource_enabled, ALWAYS_EQ, LARGEST, SMALLEST

import datetime as datetime_module
from datetime import MINYEAR, MAXYEAR
from datetime import timedelta
from datetime import tzinfo
from datetime import time
from datetime import timezone
from datetime import date, datetime
import time as _time

import _testcapi

# Needed by test_datetime
import _strptime
#

pickle_loads = {pickle.loads, pickle._loads}

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

    def test_all(self):
        """Test that __all__ only points to valid attributes."""
        all_attrs = dir(datetime_module)
        for attr in datetime_module.__all__:
            self.assertIn(attr, all_attrs)

    def test_name_cleanup(self):
        if '_Pure' in self.__class__.__name__:
            self.skipTest('Only run for Fast C implementation')

        datetime = datetime_module
        names = set(name for name in dir(datetime)
                    if not name.startswith('__') and not name.endswith('__'))
        allowed = set(['MAXYEAR', 'MINYEAR', 'date', 'datetime',
                       'datetime_CAPI', 'time', 'timedelta', 'timezone',
                       'tzinfo', 'sys'])
        self.assertEqual(names - allowed, set([]))

    def test_divide_and_round(self):
        if '_Fast' in self.__class__.__name__:
            self.skipTest('Only run for Pure Python implementation')

        dar = datetime_module._divide_and_round

        self.assertEqual(dar(-10, -3), 3)
        self.assertEqual(dar(5, -2), -2)

        # four cases: (2 signs of a) x (2 signs of b)
        self.assertEqual(dar(7, 3), 2)
        self.assertEqual(dar(-7, 3), -2)
        self.assertEqual(dar(7, -3), -2)
        self.assertEqual(dar(-7, -3), 2)

        # ties to even - eight cases:
        # (2 signs of a) x (2 signs of b) x (even / odd quotient)
        self.assertEqual(dar(10, 4), 2)
        self.assertEqual(dar(-10, 4), -2)
        self.assertEqual(dar(10, -4), -2)
        self.assertEqual(dar(-10, -4), 2)

        self.assertEqual(dar(6, 4), 2)
        self.assertEqual(dar(-6, 4), -2)
        self.assertEqual(dar(6, -4), -2)
        self.assertEqual(dar(-6, -4), 2)


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

    def __getstate__(self):
        return self.__dict__

class _TZInfo(tzinfo):
    def utcoffset(self, datetime_module):
        return random.random()

class TestTZInfo(unittest.TestCase):

    def test_refcnt_crash_bug_22044(self):
        tz1 = _TZInfo()
        dt1 = datetime(2014, 7, 21, 11, 32, 3, 0, tz1)
        with self.assertRaises(TypeError):
            dt1.utcoffset()

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
        self.assertIs(type(orig), tzinfo)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertIs(type(derived), tzinfo)

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

    def test_issue23600(self):
        DSTDIFF = DSTOFFSET = timedelta(hours=1)

        class UKSummerTime(tzinfo):
            """Simple time zone which pretends to always be in summer time, since
                that's what shows the failure.
            """

            def utcoffset(self, dt):
                return DSTOFFSET

            def dst(self, dt):
                return DSTDIFF

            def tzname(self, dt):
                return 'UKSummerTime'

        tz = UKSummerTime()
        u = datetime(2014, 4, 26, 12, 1, tzinfo=tz)
        t = tz.fromutc(u)
        self.assertEqual(t - t.utcoffset(), u)


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
        for subminute in [timedelta(microseconds=1), timedelta(seconds=1)]:
            tz = timezone(subminute)
            self.assertNotEqual(tz.utcoffset(None) % timedelta(minutes=1), 0)
        # invalid offsets
        for invalid in [timedelta(1, 1), timedelta(1)]:
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
        self.assertEqual('UTC', timezone.utc.tzname(None))
        self.assertEqual('UTC', timezone(ZERO).tzname(None))
        self.assertEqual('UTC-05:00', timezone(-5 * HOUR).tzname(None))
        self.assertEqual('UTC+09:30', timezone(9.5 * HOUR).tzname(None))
        self.assertEqual('UTC-00:01', timezone(timedelta(minutes=-1)).tzname(None))
        self.assertEqual('XYZ', timezone(-5 * HOUR, 'XYZ').tzname(None))
        # bpo-34482: Check that surrogates are handled properly.
        self.assertEqual('\ud800', timezone(ZERO, '\ud800').tzname(None))

        # Sub-minute offsets:
        self.assertEqual('UTC+01:06:40', timezone(timedelta(0, 4000)).tzname(None))
        self.assertEqual('UTC-01:06:40',
                         timezone(-timedelta(0, 4000)).tzname(None))
        self.assertEqual('UTC+01:06:40.000001',
                         timezone(timedelta(0, 4000, 1)).tzname(None))
        self.assertEqual('UTC-01:06:40.000001',
                         timezone(-timedelta(0, 4000, 1)).tzname(None))

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
        self.assertTrue(timezone(ZERO) != None)
        self.assertFalse(timezone(ZERO) == None)

        tz = timezone(ZERO)
        self.assertTrue(tz == ALWAYS_EQ)
        self.assertFalse(tz != ALWAYS_EQ)
        self.assertTrue(tz < LARGEST)
        self.assertFalse(tz > LARGEST)
        self.assertTrue(tz <= LARGEST)
        self.assertFalse(tz >= LARGEST)
        self.assertFalse(tz < SMALLEST)
        self.assertTrue(tz > SMALLEST)
        self.assertFalse(tz <= SMALLEST)
        self.assertTrue(tz >= SMALLEST)

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

    def test_pickle(self):
        for tz in self.ACDT, self.EST, timezone.min, timezone.max:
            for pickler, unpickler, proto in pickle_choices:
                tz_copy = unpickler.loads(pickler.dumps(tz, proto))
                self.assertEqual(tz_copy, tz)
        tz = timezone.utc
        for pickler, unpickler, proto in pickle_choices:
            tz_copy = unpickler.loads(pickler.dumps(tz, proto))
            self.assertIs(tz_copy, tz)

    def test_copy(self):
        for tz in self.ACDT, self.EST, timezone.min, timezone.max:
            tz_copy = copy.copy(tz)
            self.assertEqual(tz_copy, tz)
        tz = timezone.utc
        tz_copy = copy.copy(tz)
        self.assertIs(tz_copy, tz)

    def test_deepcopy(self):
        for tz in self.ACDT, self.EST, timezone.min, timezone.max:
            tz_copy = copy.deepcopy(tz)
            self.assertEqual(tz_copy, tz)
        tz = timezone.utc
        tz_copy = copy.deepcopy(tz)
        self.assertIs(tz_copy, tz)

    def test_offset_boundaries(self):
        # Test timedeltas close to the boundaries
        time_deltas = [
            timedelta(hours=23, minutes=59),
            timedelta(hours=23, minutes=59, seconds=59),
            timedelta(hours=23, minutes=59, seconds=59, microseconds=999999),
        ]
        time_deltas.extend([-delta for delta in time_deltas])

        for delta in time_deltas:
            with self.subTest(test_type='good', delta=delta):
                timezone(delta)

        # Test timedeltas on and outside the boundaries
        bad_time_deltas = [
            timedelta(hours=24),
            timedelta(hours=24, microseconds=1),
        ]
        bad_time_deltas.extend([-delta for delta in bad_time_deltas])

        for delta in bad_time_deltas:
            with self.subTest(test_type='bad', delta=delta):
                with self.assertRaises(ValueError):
                    timezone(delta)

    def test_comparison_with_tzinfo(self):
        # Constructing tzinfo objects directly should not be done by users
        # and serves only to check the bug described in bpo-37915
        self.assertNotEqual(timezone.utc, tzinfo())
        self.assertNotEqual(timezone(timedelta(hours=1)), tzinfo())

#############################################################################
# Base class for testing a particular aspect of timedelta, time, date and
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

        # Comparison to objects of unsupported types should return
        # NotImplemented which falls back to the right hand side's __eq__
        # method. In this case, ALWAYS_EQ.__eq__ always returns True.
        # ALWAYS_EQ.__ne__ always returns False.
        self.assertTrue(me == ALWAYS_EQ)
        self.assertFalse(me != ALWAYS_EQ)

        # If the other class explicitly defines ordering
        # relative to our class, it is allowed to do so
        self.assertTrue(me < LARGEST)
        self.assertFalse(me > LARGEST)
        self.assertTrue(me <= LARGEST)
        self.assertFalse(me >= LARGEST)
        self.assertFalse(me < SMALLEST)
        self.assertTrue(me > SMALLEST)
        self.assertFalse(me <= SMALLEST)
        self.assertTrue(me >= SMALLEST)

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

        # Issue #23521
        eq(td(seconds=1) * 0.123456, td(microseconds=123456))
        eq(td(seconds=1) * 0.6112295, td(microseconds=611229))

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

        # Issue #23521
        eq(td(seconds=1) / (1 / 0.6112295), td(microseconds=611229))

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
        self.assertFalse(t1 != t2)
        self.assertFalse(t1 < t2)
        self.assertFalse(t1 > t2)

        for args in (3, 3, 3), (2, 4, 4), (2, 3, 5):
            t2 = timedelta(*args)   # this is larger than t1
            self.assertTrue(t1 < t2)
            self.assertTrue(t2 > t1)
            self.assertTrue(t1 <= t2)
            self.assertTrue(t2 >= t1)
            self.assertTrue(t1 != t2)
            self.assertTrue(t2 != t1)
            self.assertFalse(t1 == t2)
            self.assertFalse(t2 == t1)
            self.assertFalse(t1 > t2)
            self.assertFalse(t2 < t1)
            self.assertFalse(t1 >= t2)
            self.assertFalse(t2 <= t1)

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
                         "%s(days=1)" % name)
        self.assertEqual(repr(self.theclass(10, 2)),
                         "%s(days=10, seconds=2)" % name)
        self.assertEqual(repr(self.theclass(-10, 2, 400000)),
                         "%s(days=-10, seconds=2, microseconds=400000)" % name)
        self.assertEqual(repr(self.theclass(seconds=60)),
                         "%s(seconds=60)" % name)
        self.assertEqual(repr(self.theclass()),
                         "%s(0)" % name)
        self.assertEqual(repr(self.theclass(microseconds=100)),
                         "%s(microseconds=100)" % name)
        self.assertEqual(repr(self.theclass(days=1, microseconds=100)),
                         "%s(days=1, microseconds=100)" % name)
        self.assertEqual(repr(self.theclass(seconds=1, microseconds=100)),
                         "%s(seconds=1, microseconds=100)" % name)

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
        eq(td(milliseconds=0.5/1000), td(microseconds=0))
        eq(td(milliseconds=-0.5/1000), td(microseconds=-0))
        eq(td(milliseconds=0.6/1000), td(microseconds=1))
        eq(td(milliseconds=-0.6/1000), td(microseconds=-1))
        eq(td(milliseconds=1.5/1000), td(microseconds=2))
        eq(td(milliseconds=-1.5/1000), td(microseconds=-2))
        eq(td(seconds=0.5/10**6), td(microseconds=0))
        eq(td(seconds=-0.5/10**6), td(microseconds=-0))
        eq(td(seconds=1/2**7), td(microseconds=7812))
        eq(td(seconds=-1/2**7), td(microseconds=-7812))

        # Rounding due to contributions from more than one field.
        us_per_hour = 3600e6
        us_per_day = us_per_hour * 24
        eq(td(days=.4/us_per_day), td(0))
        eq(td(hours=.2/us_per_hour), td(0))
        eq(td(days=.4/us_per_day, hours=.2/us_per_hour), td(microseconds=1))

        eq(td(days=-.4/us_per_day), td(0))
        eq(td(hours=-.2/us_per_hour), td(0))
        eq(td(days=-.4/us_per_day, hours=-.2/us_per_hour), td(microseconds=-1))

        # Test for a patch in Issue 8860
        eq(td(microseconds=0.5), 0.5*td(microseconds=1.0))
        eq(td(microseconds=0.5)//td.resolution, 0.5*td.resolution//td.resolution)

    def test_massive_normalization(self):
        td = timedelta(microseconds=-1)
        self.assertEqual((td.days, td.seconds, td.microseconds),
                         (-1, 24*3600-1, 999999))

    def test_bool(self):
        self.assertTrue(timedelta(1))
        self.assertTrue(timedelta(0, 1))
        self.assertTrue(timedelta(0, 0, 1))
        self.assertTrue(timedelta(microseconds=1))
        self.assertFalse(timedelta(0))

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
        self.assertIs(type(t1), T)
        self.assertEqual(t1.as_hours(), 24)

        t2 = T(days=-1, seconds=-3600)
        self.assertIs(type(t2), T)
        self.assertEqual(t2.as_hours(), -25)

        t3 = t1 + t2
        self.assertIs(type(t3), timedelta)
        t4 = T.from_td(t3)
        self.assertIs(type(t4), T)
        self.assertEqual(t3.days, t4.days)
        self.assertEqual(t3.seconds, t4.seconds)
        self.assertEqual(t3.microseconds, t4.microseconds)
        self.assertEqual(str(t3), str(t4))
        self.assertEqual(t4.as_hours(), -1)

    def test_subclass_date(self):
        class DateSubclass(date):
            pass

        d1 = DateSubclass(2018, 1, 5)
        td = timedelta(days=1)

        tests = [
            ('add', lambda d, t: d + t, DateSubclass(2018, 1, 6)),
            ('radd', lambda d, t: t + d, DateSubclass(2018, 1, 6)),
            ('sub', lambda d, t: d - t, DateSubclass(2018, 1, 4)),
        ]

        for name, func, expected in tests:
            with self.subTest(name):
                act = func(d1, td)
                self.assertEqual(act, expected)
                self.assertIsInstance(act, DateSubclass)

    def test_subclass_datetime(self):
        class DateTimeSubclass(datetime):
            pass

        d1 = DateTimeSubclass(2018, 1, 5, 12, 30)
        td = timedelta(days=1, minutes=30)

        tests = [
            ('add', lambda d, t: d + t, DateTimeSubclass(2018, 1, 6, 13)),
            ('radd', lambda d, t: t + d, DateTimeSubclass(2018, 1, 6, 13)),
            ('sub', lambda d, t: d - t, DateTimeSubclass(2018, 1, 4, 12)),
        ]

        for name, func, expected in tests:
            with self.subTest(name):
                act = func(d1, td)
                self.assertEqual(act, expected)
                self.assertIsInstance(act, DateTimeSubclass)

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

    def test_issue31293(self):
        # The interpreter shouldn't crash in case a timedelta is divided or
        # multiplied by a float with a bad as_integer_ratio() method.
        def get_bad_float(bad_ratio):
            class BadFloat(float):
                def as_integer_ratio(self):
                    return bad_ratio
            return BadFloat()

        with self.assertRaises(TypeError):
            timedelta() / get_bad_float(1 << 1000)
        with self.assertRaises(TypeError):
            timedelta() * get_bad_float(1 << 1000)

        for bad_ratio in [(), (42, ), (1, 2, 3)]:
            with self.assertRaises(ValueError):
                timedelta() / get_bad_float(bad_ratio)
            with self.assertRaises(ValueError):
                timedelta() * get_bad_float(bad_ratio)

    def test_issue31752(self):
        # The interpreter shouldn't crash because divmod() returns negative
        # remainder.
        class BadInt(int):
            def __mul__(self, other):
                return Prod()
            def __rmul__(self, other):
                return Prod()
            def __floordiv__(self, other):
                return Prod()
            def __rfloordiv__(self, other):
                return Prod()

        class Prod:
            def __add__(self, other):
                return Sum()
            def __radd__(self, other):
                return Sum()

        class Sum(int):
            def __divmod__(self, other):
                return divmodresult

        for divmodresult in [None, (), (0, 1, 2), (0, -1)]:
            with self.subTest(divmodresult=divmodresult):
                # The following examples should not crash.
                try:
                    timedelta(microseconds=BadInt(1))
                except TypeError:
                    pass
                try:
                    timedelta(hours=BadInt(1))
                except TypeError:
                    pass
                try:
                    timedelta(weeks=BadInt(1))
                except (TypeError, ValueError):
                    pass
                try:
                    timedelta(1) * BadInt(1)
                except (TypeError, ValueError):
                    pass
                try:
                    BadInt(1) * timedelta(1)
                except TypeError:
                    pass
                try:
                    timedelta(1) // BadInt(1)
                except TypeError:
                    pass


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
        if today != todayagain:
            self.assertAlmostEqual(todayagain, today,
                                   delta=timedelta(seconds=0.5))

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
        week_mondays = [
                ((2003, 12, 22), (2003, 52, 1)),
                ((2003, 12, 29), (2004, 1, 1)),
                ((2004, 1, 5), (2004, 2, 1)),
                ((2009, 12, 21), (2009, 52, 1)),
                ((2009, 12, 28), (2009, 53, 1)),
                ((2010, 1, 4), (2010, 1, 1)),
        ]

        test_cases = []
        for cal_date, iso_date in week_mondays:
            base_date = self.theclass(*cal_date)
            # Adds one test case for every day of the specified weeks
            for i in range(7):
                new_date = base_date + timedelta(i)
                new_iso = iso_date[0:2] + (iso_date[2] + i,)
                test_cases.append((new_date, new_iso))

        for d, exp_iso in test_cases:
            with self.subTest(d=d, comparison="tuple"):
                self.assertEqual(d.isocalendar(), exp_iso)

            # Check that the tuple contents are accessible by field name
            with self.subTest(d=d, comparison="fields"):
                t = d.isocalendar()
                self.assertEqual((t.year, t.week, t.weekday), exp_iso)

    def test_isocalendar_pickling(self):
        """Test that the result of datetime.isocalendar() can be pickled.

        The result of a round trip should be a plain tuple.
        """
        d = self.theclass(2019, 1, 1)
        p = pickle.dumps(d.isocalendar())
        res = pickle.loads(p)
        self.assertEqual(type(res), tuple)
        self.assertEqual(res, (2019, 1, 2))

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
        #at least, exercise them to make sure that no crashes
        #are generated
        for f in ["%e", "%", "%#"]:
            try:
                t.strftime(f)
            except ValueError:
                pass

        # bpo-34482: Check that surrogates don't cause a crash.
        try:
            t.strftime('%y\ud800%m')
        except UnicodeEncodeError:
            pass

        #check that this standard extension works
        t.strftime("%f")

    def test_strftime_trailing_percent(self):
        # bpo-35066: Make sure trailing '%' doesn't cause datetime's strftime to
        # complain. Different libcs have different handling of trailing
        # percents, so we simply check datetime's strftime acts the same as
        # time.strftime.
        t = self.theclass(2005, 3, 2)
        try:
            _time.strftime('%')
        except ValueError:
            self.skipTest('time module does not support trailing %')
        self.assertEqual(t.strftime('%'), _time.strftime('%', t.timetuple()))
        self.assertEqual(
            t.strftime("m:%m d:%d y:%y %"),
            _time.strftime("m:03 d:02 y:05 %", t.timetuple()),
        )

    def test_format(self):
        dt = self.theclass(2007, 9, 10)
        self.assertEqual(dt.__format__(''), str(dt))

        with self.assertRaisesRegex(TypeError, 'must be str, not int'):
            dt.__format__(123)

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
        self.assertEqual(orig.__reduce__(), orig.__reduce_ex__(2))

    def test_compat_unpickle(self):
        tests = [
            b"cdatetime\ndate\n(S'\\x07\\xdf\\x0b\\x1b'\ntR.",
            b'cdatetime\ndate\n(U\x04\x07\xdf\x0b\x1btR.',
            b'\x80\x02cdatetime\ndate\nU\x04\x07\xdf\x0b\x1b\x85R.',
        ]
        args = 2015, 11, 27
        expected = self.theclass(*args)
        for data in tests:
            for loads in pickle_loads:
                derived = loads(data, encoding='latin1')
                self.assertEqual(derived, expected)

    def test_compare(self):
        t1 = self.theclass(2, 3, 4)
        t2 = self.theclass(2, 3, 4)
        self.assertEqual(t1, t2)
        self.assertTrue(t1 <= t2)
        self.assertTrue(t1 >= t2)
        self.assertFalse(t1 != t2)
        self.assertFalse(t1 < t2)
        self.assertFalse(t1 > t2)

        for args in (3, 3, 3), (2, 4, 4), (2, 3, 5):
            t2 = self.theclass(*args)   # this is larger than t1
            self.assertTrue(t1 < t2)
            self.assertTrue(t2 > t1)
            self.assertTrue(t1 <= t2)
            self.assertTrue(t2 >= t1)
            self.assertTrue(t1 != t2)
            self.assertTrue(t2 != t1)
            self.assertFalse(t1 == t2)
            self.assertFalse(t2 == t1)
            self.assertFalse(t1 > t2)
            self.assertFalse(t2 < t1)
            self.assertFalse(t1 >= t2)
            self.assertFalse(t2 <= t1)

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

    def test_subclass_replace(self):
        class DateSubclass(self.theclass):
            pass

        dt = DateSubclass(2012, 1, 1)
        self.assertIs(type(dt.replace(year=2013)), DateSubclass)

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

    def test_subclass_alternate_constructors(self):
        # Test that alternate constructors call the constructor
        class DateSubclass(self.theclass):
            def __new__(cls, *args, **kwargs):
                result = self.theclass.__new__(cls, *args, **kwargs)
                result.extra = 7

                return result

        args = (2003, 4, 14)
        d_ord = 731319              # Equivalent ordinal date
        d_isoformat = '2003-04-14'  # Equivalent isoformat()

        base_d = DateSubclass(*args)
        self.assertIsInstance(base_d, DateSubclass)
        self.assertEqual(base_d.extra, 7)

        # Timestamp depends on time zone, so we'll calculate the equivalent here
        ts = datetime.combine(base_d, time(0)).timestamp()

        test_cases = [
            ('fromordinal', (d_ord,)),
            ('fromtimestamp', (ts,)),
            ('fromisoformat', (d_isoformat,)),
        ]

        for constr_name, constr_args in test_cases:
            for base_obj in (DateSubclass, base_d):
                # Test both the classmethod and method
                with self.subTest(base_obj_type=type(base_obj),
                                  constr_name=constr_name):
                    constr = getattr(base_obj, constr_name)

                    dt = constr(*constr_args)

                    # Test that it creates the right subclass
                    self.assertIsInstance(dt, DateSubclass)

                    # Test that it's equal to the base object
                    self.assertEqual(dt, base_d)

                    # Test that it called the constructor
                    self.assertEqual(dt.extra, 7)

    def test_pickling_subclass_date(self):

        args = 6, 7, 23
        orig = SubclassDate(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)
            self.assertTrue(isinstance(derived, SubclassDate))

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
        if issubclass(self.theclass, datetime):
            # Good bytes, but bad tzinfo:
            with self.assertRaisesRegex(TypeError, '^bad tzinfo state arg$'):
                self.theclass(bytes([1] * len(base)), 'EST')

        for ord_byte in range(1, 13):
            # This shouldn't blow up because of the month byte alone.  If
            # the implementation changes to do more-careful checking, it may
            # blow up because other fields are insane.
            self.theclass(base[:2] + bytes([ord_byte]) + base[3:])

    def test_fromisoformat(self):
        # Test that isoformat() is reversible
        base_dates = [
            (1, 1, 1),
            (1000, 2, 14),
            (1900, 1, 1),
            (2000, 2, 29),
            (2004, 11, 12),
            (2004, 4, 3),
            (2017, 5, 30)
        ]

        for dt_tuple in base_dates:
            dt = self.theclass(*dt_tuple)
            dt_str = dt.isoformat()
            with self.subTest(dt_str=dt_str):
                dt_rt = self.theclass.fromisoformat(dt.isoformat())

                self.assertEqual(dt, dt_rt)

    def test_fromisoformat_subclass(self):
        class DateSubclass(self.theclass):
            pass

        dt = DateSubclass(2014, 12, 14)

        dt_rt = DateSubclass.fromisoformat(dt.isoformat())

        self.assertIsInstance(dt_rt, DateSubclass)

    def test_fromisoformat_fails(self):
        # Test that fromisoformat() fails on invalid values
        bad_strs = [
            '',                 # Empty string
            '\ud800',           # bpo-34454: Surrogate code point
            '009-03-04',        # Not 10 characters
            '123456789',        # Not a date
            '200a-12-04',       # Invalid character in year
            '2009-1a-04',       # Invalid character in month
            '2009-12-0a',       # Invalid character in day
            '2009-01-32',       # Invalid day
            '2009-02-29',       # Invalid leap day
            '20090228',         # Valid ISO8601 output not from isoformat()
            '2009\ud80002\ud80028',     # Separators are surrogate codepoints
        ]

        for bad_str in bad_strs:
            with self.assertRaises(ValueError):
                self.theclass.fromisoformat(bad_str)

    def test_fromisoformat_fails_typeerror(self):
        # Test that fromisoformat fails when passed the wrong type
        bad_types = [b'2009-03-01', None, io.StringIO('2009-03-01')]
        for bad_type in bad_types:
            with self.assertRaises(TypeError):
                self.theclass.fromisoformat(bad_type)

    def test_fromisocalendar(self):
        # For each test case, assert that fromisocalendar is the
        # inverse of the isocalendar function
        dates = [
            (2016, 4, 3),
            (2005, 1, 2),       # (2004, 53, 7)
            (2008, 12, 30),     # (2009, 1, 2)
            (2010, 1, 2),       # (2009, 53, 6)
            (2009, 12, 31),     # (2009, 53, 4)
            (1900, 1, 1),       # Unusual non-leap year (year % 100 == 0)
            (1900, 12, 31),
            (2000, 1, 1),       # Unusual leap year (year % 400 == 0)
            (2000, 12, 31),
            (2004, 1, 1),       # Leap year
            (2004, 12, 31),
            (1, 1, 1),
            (9999, 12, 31),
            (MINYEAR, 1, 1),
            (MAXYEAR, 12, 31),
        ]

        for datecomps in dates:
            with self.subTest(datecomps=datecomps):
                dobj = self.theclass(*datecomps)
                isocal = dobj.isocalendar()

                d_roundtrip = self.theclass.fromisocalendar(*isocal)

                self.assertEqual(dobj, d_roundtrip)

    def test_fromisocalendar_value_errors(self):
        isocals = [
            (2019, 0, 1),
            (2019, -1, 1),
            (2019, 54, 1),
            (2019, 1, 0),
            (2019, 1, -1),
            (2019, 1, 8),
            (2019, 53, 1),
            (10000, 1, 1),
            (0, 1, 1),
            (9999999, 1, 1),
            (2<<32, 1, 1),
            (2019, 2<<32, 1),
            (2019, 1, 2<<32),
        ]

        for isocal in isocals:
            with self.subTest(isocal=isocal):
                with self.assertRaises(ValueError):
                    self.theclass.fromisocalendar(*isocal)

    def test_fromisocalendar_type_errors(self):
        err_txformers = [
            str,
            float,
            lambda x: None,
        ]

        # Take a valid base tuple and transform it to contain one argument
        # with the wrong type. Repeat this for each argument, e.g.
        # [("2019", 1, 1), (2019, "1", 1), (2019, 1, "1"), ...]
        isocals = []
        base = (2019, 1, 1)
        for i in range(3):
            for txformer in err_txformers:
                err_val = list(base)
                err_val[i] = txformer(err_val[i])
                isocals.append(tuple(err_val))

        for isocal in isocals:
            with self.subTest(isocal=isocal):
                with self.assertRaises(TypeError):
                    self.theclass.fromisocalendar(*isocal)


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
        t = self.theclass(1, 2, 3, 4, 5, 1, 123)
        self.assertEqual(t.isoformat(),    "0001-02-03T04:05:01.000123")
        self.assertEqual(t.isoformat('T'), "0001-02-03T04:05:01.000123")
        self.assertEqual(t.isoformat(' '), "0001-02-03 04:05:01.000123")
        self.assertEqual(t.isoformat('\x00'), "0001-02-03\x0004:05:01.000123")
        # bpo-34482: Check that surrogates are handled properly.
        self.assertEqual(t.isoformat('\ud800'),
                         "0001-02-03\ud80004:05:01.000123")
        self.assertEqual(t.isoformat(timespec='hours'), "0001-02-03T04")
        self.assertEqual(t.isoformat(timespec='minutes'), "0001-02-03T04:05")
        self.assertEqual(t.isoformat(timespec='seconds'), "0001-02-03T04:05:01")
        self.assertEqual(t.isoformat(timespec='milliseconds'), "0001-02-03T04:05:01.000")
        self.assertEqual(t.isoformat(timespec='microseconds'), "0001-02-03T04:05:01.000123")
        self.assertEqual(t.isoformat(timespec='auto'), "0001-02-03T04:05:01.000123")
        self.assertEqual(t.isoformat(sep=' ', timespec='minutes'), "0001-02-03 04:05")
        self.assertRaises(ValueError, t.isoformat, timespec='foo')
        # bpo-34482: Check that surrogates are handled properly.
        self.assertRaises(ValueError, t.isoformat, timespec='\ud800')
        # str is ISO format with the separator forced to a blank.
        self.assertEqual(str(t), "0001-02-03 04:05:01.000123")

        t = self.theclass(1, 2, 3, 4, 5, 1, 999500, tzinfo=timezone.utc)
        self.assertEqual(t.isoformat(timespec='milliseconds'), "0001-02-03T04:05:01.999+00:00")

        t = self.theclass(1, 2, 3, 4, 5, 1, 999500)
        self.assertEqual(t.isoformat(timespec='milliseconds'), "0001-02-03T04:05:01.999")

        t = self.theclass(1, 2, 3, 4, 5, 1)
        self.assertEqual(t.isoformat(timespec='auto'), "0001-02-03T04:05:01")
        self.assertEqual(t.isoformat(timespec='milliseconds'), "0001-02-03T04:05:01.000")
        self.assertEqual(t.isoformat(timespec='microseconds'), "0001-02-03T04:05:01.000000")

        t = self.theclass(2, 3, 2)
        self.assertEqual(t.isoformat(),    "0002-03-02T00:00:00")
        self.assertEqual(t.isoformat('T'), "0002-03-02T00:00:00")
        self.assertEqual(t.isoformat(' '), "0002-03-02 00:00:00")
        # str is ISO format with the separator forced to a blank.
        self.assertEqual(str(t), "0002-03-02 00:00:00")
        # ISO format with timezone
        tz = FixedOffset(timedelta(seconds=16), 'XXX')
        t = self.theclass(2, 3, 2, tzinfo=tz)
        self.assertEqual(t.isoformat(), "0002-03-02T00:00:00+00:00:16")

    def test_isoformat_timezone(self):
        tzoffsets = [
            ('05:00', timedelta(hours=5)),
            ('02:00', timedelta(hours=2)),
            ('06:27', timedelta(hours=6, minutes=27)),
            ('12:32:30', timedelta(hours=12, minutes=32, seconds=30)),
            ('02:04:09.123456', timedelta(hours=2, minutes=4, seconds=9, microseconds=123456))
        ]

        tzinfos = [
            ('', None),
            ('+00:00', timezone.utc),
            ('+00:00', timezone(timedelta(0))),
        ]

        tzinfos += [
            (prefix + expected, timezone(sign * td))
            for expected, td in tzoffsets
            for prefix, sign in [('-', -1), ('+', 1)]
        ]

        dt_base = self.theclass(2016, 4, 1, 12, 37, 9)
        exp_base = '2016-04-01T12:37:09'

        for exp_tz, tzi in tzinfos:
            dt = dt_base.replace(tzinfo=tzi)
            exp = exp_base + exp_tz
            with self.subTest(tzi=tzi):
                assert dt.isoformat() == exp

    def test_format(self):
        dt = self.theclass(2007, 9, 10, 4, 5, 1, 123)
        self.assertEqual(dt.__format__(''), str(dt))

        with self.assertRaisesRegex(TypeError, 'must be str, not int'):
            dt.__format__(123)

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
        # precision to span microsecond resolution across years 1 through 9999,
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
        # bad fold
        self.assertRaises(ValueError, self.theclass,
                          2000, 1, 31, fold=-1)
        self.assertRaises(ValueError, self.theclass,
                          2000, 1, 31, fold=2)
        # Positional fold:
        self.assertRaises(TypeError, self.theclass,
                          2000, 1, 31, 23, 59, 59, 0, None, 1)

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
        self.assertEqual(orig.__reduce__(), orig.__reduce_ex__(2))

    def test_more_pickling(self):
        a = self.theclass(2003, 2, 7, 16, 48, 37, 444116)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            s = pickle.dumps(a, proto)
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
            self.assertTrue(isinstance(derived, SubclassDatetime))

    def test_compat_unpickle(self):
        tests = [
            b'cdatetime\ndatetime\n('
            b"S'\\x07\\xdf\\x0b\\x1b\\x14;\\x01\\x00\\x10\\x00'\ntR.",

            b'cdatetime\ndatetime\n('
            b'U\n\x07\xdf\x0b\x1b\x14;\x01\x00\x10\x00tR.',

            b'\x80\x02cdatetime\ndatetime\n'
            b'U\n\x07\xdf\x0b\x1b\x14;\x01\x00\x10\x00\x85R.',
        ]
        args = 2015, 11, 27, 20, 59, 1, 64**2
        expected = self.theclass(*args)
        for data in tests:
            for loads in pickle_loads:
                derived = loads(data, encoding='latin1')
                self.assertEqual(derived, expected)

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
        self.assertFalse(t1 != t2)
        self.assertFalse(t1 < t2)
        self.assertFalse(t1 > t2)

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
            self.assertFalse(t1 == t2)
            self.assertFalse(t2 == t1)
            self.assertFalse(t1 > t2)
            self.assertFalse(t2 < t1)
            self.assertFalse(t1 >= t2)
            self.assertFalse(t2 <= t1)


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
        # Missing hour
        t0 = self.theclass(2012, 3, 11, 2, 30)
        t1 = t0.replace(fold=1)
        self.assertEqual(self.theclass.fromtimestamp(t1.timestamp()),
                         t0 - timedelta(hours=1))
        self.assertEqual(self.theclass.fromtimestamp(t0.timestamp()),
                         t1 + timedelta(hours=1))
        # Ambiguous hour defaults to DST
        t = self.theclass(2012, 11, 4, 1, 30)
        self.assertEqual(self.theclass.fromtimestamp(t.timestamp()), t)

        # Timestamp may raise an overflow error on some platforms
        # XXX: Do we care to support the first and last year?
        for t in [self.theclass(2,1,1), self.theclass(9998,12,12)]:
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

    @support.run_with_tz('MSK-03')  # Something east of Greenwich
    def test_microsecond_rounding(self):
        for fts in [self.theclass.fromtimestamp,
                    self.theclass.utcfromtimestamp]:
            zero = fts(0)
            self.assertEqual(zero.second, 0)
            self.assertEqual(zero.microsecond, 0)
            one = fts(1e-6)
            try:
                minus_one = fts(-1e-6)
            except OSError:
                # localtime(-1) and gmtime(-1) is not supported on Windows
                pass
            else:
                self.assertEqual(minus_one.second, 59)
                self.assertEqual(minus_one.microsecond, 999999)

                t = fts(-1e-8)
                self.assertEqual(t, zero)
                t = fts(-9e-7)
                self.assertEqual(t, minus_one)
                t = fts(-1e-7)
                self.assertEqual(t, zero)
                t = fts(-1/2**7)
                self.assertEqual(t.second, 59)
                self.assertEqual(t.microsecond, 992188)

            t = fts(1e-7)
            self.assertEqual(t, zero)
            t = fts(9e-7)
            self.assertEqual(t, one)
            t = fts(0.99999949)
            self.assertEqual(t.second, 0)
            self.assertEqual(t.microsecond, 999999)
            t = fts(0.9999999)
            self.assertEqual(t.second, 1)
            self.assertEqual(t.microsecond, 0)
            t = fts(1/2**7)
            self.assertEqual(t.second, 0)
            self.assertEqual(t.microsecond, 7812)

    def test_timestamp_limits(self):
        with self.subTest("minimum UTC"):
            min_dt = self.theclass.min.replace(tzinfo=timezone.utc)
            min_ts = min_dt.timestamp()

            # This test assumes that datetime.min == 0000-01-01T00:00:00.00
            # If that assumption changes, this value can change as well
            self.assertEqual(min_ts, -62135596800)

        with self.subTest("maximum UTC"):
            # Zero out microseconds to avoid rounding issues
            max_dt = self.theclass.max.replace(tzinfo=timezone.utc,
                                               microsecond=0)
            max_ts = max_dt.timestamp()

            # This test assumes that datetime.max == 9999-12-31T23:59:59.999999
            # If that assumption changes, this value can change as well
            self.assertEqual(max_ts, 253402300799.0)

    def test_fromtimestamp_limits(self):
        try:
            self.theclass.fromtimestamp(-2**32 - 1)
        except (OSError, OverflowError):
            self.skipTest("Test not valid on this platform")

        # XXX: Replace these with datetime.{min,max}.timestamp() when we solve
        # the issue with gh-91012
        min_dt = self.theclass.min + timedelta(days=1)
        min_ts = min_dt.timestamp()

        max_dt = self.theclass.max.replace(microsecond=0)
        max_ts = ((self.theclass.max - timedelta(hours=23)).timestamp() +
                  timedelta(hours=22, minutes=59, seconds=59).total_seconds())

        for (test_name, ts, expected) in [
                ("minimum", min_ts, min_dt),
                ("maximum", max_ts, max_dt),
        ]:
            with self.subTest(test_name, ts=ts, expected=expected):
                actual = self.theclass.fromtimestamp(ts)

                self.assertEqual(actual, expected)

        # Test error conditions
        test_cases = [
            ("Too small by a little", min_ts - timedelta(days=1, hours=12).total_seconds()),
            ("Too small by a lot", min_ts - timedelta(days=400).total_seconds()),
            ("Too big by a little", max_ts + timedelta(days=1).total_seconds()),
            ("Too big by a lot", max_ts + timedelta(days=400).total_seconds()),
        ]

        for test_name, ts in test_cases:
            with self.subTest(test_name, ts=ts):
                with self.assertRaises((ValueError, OverflowError)):
                    # converting a Python int to C time_t can raise a
                    # OverflowError, especially on 32-bit platforms.
                    self.theclass.fromtimestamp(ts)

    def test_utcfromtimestamp_limits(self):
        try:
            self.theclass.utcfromtimestamp(-2**32 - 1)
        except (OSError, OverflowError):
            self.skipTest("Test not valid on this platform")

        min_dt = self.theclass.min.replace(tzinfo=timezone.utc)
        min_ts = min_dt.timestamp()

        max_dt = self.theclass.max.replace(microsecond=0, tzinfo=timezone.utc)
        max_ts = max_dt.timestamp()

        for (test_name, ts, expected) in [
                ("minimum", min_ts, min_dt.replace(tzinfo=None)),
                ("maximum", max_ts, max_dt.replace(tzinfo=None)),
        ]:
            with self.subTest(test_name, ts=ts, expected=expected):
                try:
                    actual = self.theclass.utcfromtimestamp(ts)
                except (OSError, OverflowError) as exc:
                    self.skipTest(str(exc))

                self.assertEqual(actual, expected)

        # Test error conditions
        test_cases = [
            ("Too small by a little", min_ts - 1),
            ("Too small by a lot", min_ts - timedelta(days=400).total_seconds()),
            ("Too big by a little", max_ts + 1),
            ("Too big by a lot", max_ts + timedelta(days=400).total_seconds()),
        ]

        for test_name, ts in test_cases:
            with self.subTest(test_name, ts=ts):
                with self.assertRaises((ValueError, OverflowError)):
                    # converting a Python int to C time_t can raise a
                    # OverflowError, especially on 32-bit platforms.
                    self.theclass.utcfromtimestamp(ts)

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
        self.assertLessEqual(abs(from_timestamp - from_now), tolerance)

    def test_strptime(self):
        string = '2004-12-01 13:02:47.197'
        format = '%Y-%m-%d %H:%M:%S.%f'
        expected = _strptime._strptime_datetime(self.theclass, string, format)
        got = self.theclass.strptime(string, format)
        self.assertEqual(expected, got)
        self.assertIs(type(expected), self.theclass)
        self.assertIs(type(got), self.theclass)

        # bpo-34482: Check that surrogates are handled properly.
        inputs = [
            ('2004-12-01\ud80013:02:47.197', '%Y-%m-%d\ud800%H:%M:%S.%f'),
            ('2004\ud80012-01 13:02:47.197', '%Y\ud800%m-%d %H:%M:%S.%f'),
            ('2004-12-01 13:02\ud80047.197', '%Y-%m-%d %H:%M\ud800%S.%f'),
        ]
        for string, format in inputs:
            with self.subTest(string=string, format=format):
                expected = _strptime._strptime_datetime(self.theclass, string,
                                                        format)
                got = self.theclass.strptime(string, format)
                self.assertEqual(expected, got)

        strptime = self.theclass.strptime

        self.assertEqual(strptime("+0002", "%z").utcoffset(), 2 * MINUTE)
        self.assertEqual(strptime("-0002", "%z").utcoffset(), -2 * MINUTE)
        self.assertEqual(
            strptime("-00:02:01.000003", "%z").utcoffset(),
            -timedelta(minutes=2, seconds=1, microseconds=3)
        )
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
        with self.assertRaises(ValueError): strptime("z", "%z")

    def test_strptime_single_digit(self):
        # bpo-34903: Check that single digit dates and times are allowed.

        strptime = self.theclass.strptime

        with self.assertRaises(ValueError):
            # %y does require two digits.
            newdate = strptime('01/02/3 04:05:06', '%d/%m/%y %H:%M:%S')
        dt1 = self.theclass(2003, 2, 1, 4, 5, 6)
        dt2 = self.theclass(2003, 1, 2, 4, 5, 6)
        dt3 = self.theclass(2003, 2, 1, 0, 0, 0)
        dt4 = self.theclass(2003, 1, 25, 0, 0, 0)
        inputs = [
            ('%d', '1/02/03 4:5:6', '%d/%m/%y %H:%M:%S', dt1),
            ('%m', '01/2/03 4:5:6', '%d/%m/%y %H:%M:%S', dt1),
            ('%H', '01/02/03 4:05:06', '%d/%m/%y %H:%M:%S', dt1),
            ('%M', '01/02/03 04:5:06', '%d/%m/%y %H:%M:%S', dt1),
            ('%S', '01/02/03 04:05:6', '%d/%m/%y %H:%M:%S', dt1),
            ('%j', '2/03 04am:05:06', '%j/%y %I%p:%M:%S',dt2),
            ('%I', '02/03 4am:05:06', '%j/%y %I%p:%M:%S',dt2),
            ('%w', '6/04/03', '%w/%U/%y', dt3),
            # %u requires a single digit.
            ('%W', '6/4/2003', '%u/%W/%Y', dt3),
            ('%V', '6/4/2003', '%u/%V/%G', dt4),
        ]
        for reason, string, format, target in inputs:
            reason = 'test single digit ' + reason
            with self.subTest(reason=reason,
                              string=string,
                              format=format,
                              target=target):
                newdate = strptime(string, format)
                self.assertEqual(newdate, target, msg=reason)

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
        for (s, us), z in [((33, 123), "33.000123"), ((33, 0), "33"),]:
            tz = timezone(-timedelta(hours=2, seconds=s, microseconds=us))
            t = t.replace(tzinfo=tz)
            self.assertEqual(t.strftime("%z"), "-0200" + z)

        # bpo-34482: Check that surrogates don't cause a crash.
        try:
            t.strftime('%y\ud800%m %H\ud800%M')
        except UnicodeEncodeError:
            pass

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
        self.assertRaises(TypeError, combine, d, t, 1) # wrong tzinfo type
        self.assertRaises(TypeError, combine, d, t, 1, 2)  # too many args
        self.assertRaises(TypeError, combine, "date", "time") # wrong types
        self.assertRaises(TypeError, combine, d, "time") # wrong type
        self.assertRaises(TypeError, combine, "date", t) # wrong type

        # tzinfo= argument
        dt = combine(d, t, timezone.utc)
        self.assertIs(dt.tzinfo, timezone.utc)
        dt = combine(d, t, tzinfo=timezone.utc)
        self.assertIs(dt.tzinfo, timezone.utc)
        t = time()
        dt = combine(dt, t)
        self.assertEqual(dt.date(), d)
        self.assertEqual(dt.time(), t)

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

    @support.run_with_tz('EDT4')
    def test_astimezone(self):
        dt = self.theclass.now()
        f = FixedOffset(44, "0044")
        dt_utc = dt.replace(tzinfo=timezone(timedelta(hours=-4), 'EDT'))
        self.assertEqual(dt.astimezone(), dt_utc) # naive
        self.assertRaises(TypeError, dt.astimezone, f, f) # too many args
        self.assertRaises(TypeError, dt.astimezone, dt) # arg wrong type
        dt_f = dt.replace(tzinfo=f) + timedelta(hours=4, minutes=44)
        self.assertEqual(dt.astimezone(f), dt_f) # naive
        self.assertEqual(dt.astimezone(tz=f), dt_f) # naive

        class Bogus(tzinfo):
            def utcoffset(self, dt): return None
            def dst(self, dt): return timedelta(0)
        bog = Bogus()
        self.assertRaises(ValueError, dt.astimezone, bog)   # naive
        self.assertEqual(dt.replace(tzinfo=bog).astimezone(f), dt_f)

        class AlsoBogus(tzinfo):
            def utcoffset(self, dt): return timedelta(0)
            def dst(self, dt): return None
        alsobog = AlsoBogus()
        self.assertRaises(ValueError, dt.astimezone, alsobog) # also naive

        class Broken(tzinfo):
            def utcoffset(self, dt): return 1
            def dst(self, dt): return 1
        broken = Broken()
        dt_broken = dt.replace(tzinfo=broken)
        with self.assertRaises(TypeError):
            dt_broken.astimezone()

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

    def test_subclass_alternate_constructors_datetime(self):
        # Test that alternate constructors call the constructor
        class DateTimeSubclass(self.theclass):
            def __new__(cls, *args, **kwargs):
                result = self.theclass.__new__(cls, *args, **kwargs)
                result.extra = 7

                return result

        args = (2003, 4, 14, 12, 30, 15, 123456)
        d_isoformat = '2003-04-14T12:30:15.123456'      # Equivalent isoformat()
        utc_ts = 1050323415.123456                      # UTC timestamp

        base_d = DateTimeSubclass(*args)
        self.assertIsInstance(base_d, DateTimeSubclass)
        self.assertEqual(base_d.extra, 7)

        # Timestamp depends on time zone, so we'll calculate the equivalent here
        ts = base_d.timestamp()

        test_cases = [
            ('fromtimestamp', (ts,), base_d),
            # See https://bugs.python.org/issue32417
            ('fromtimestamp', (ts, timezone.utc),
                               base_d.astimezone(timezone.utc)),
            ('utcfromtimestamp', (utc_ts,), base_d),
            ('fromisoformat', (d_isoformat,), base_d),
            ('strptime', (d_isoformat, '%Y-%m-%dT%H:%M:%S.%f'), base_d),
            ('combine', (date(*args[0:3]), time(*args[3:])), base_d),
        ]

        for constr_name, constr_args, expected in test_cases:
            for base_obj in (DateTimeSubclass, base_d):
                # Test both the classmethod and method
                with self.subTest(base_obj_type=type(base_obj),
                                  constr_name=constr_name):
                    constructor = getattr(base_obj, constr_name)

                    dt = constructor(*constr_args)

                    # Test that it creates the right subclass
                    self.assertIsInstance(dt, DateTimeSubclass)

                    # Test that it's equal to the base object
                    self.assertEqual(dt, expected)

                    # Test that it called the constructor
                    self.assertEqual(dt.extra, 7)

    def test_subclass_now(self):
        # Test that alternate constructors call the constructor
        class DateTimeSubclass(self.theclass):
            def __new__(cls, *args, **kwargs):
                result = self.theclass.__new__(cls, *args, **kwargs)
                result.extra = 7

                return result

        test_cases = [
            ('now', 'now', {}),
            ('utcnow', 'utcnow', {}),
            ('now_utc', 'now', {'tz': timezone.utc}),
            ('now_fixed', 'now', {'tz': timezone(timedelta(hours=-5), "EST")}),
        ]

        for name, meth_name, kwargs in test_cases:
            with self.subTest(name):
                constr = getattr(DateTimeSubclass, meth_name)
                dt = constr(**kwargs)

                self.assertIsInstance(dt, DateTimeSubclass)
                self.assertEqual(dt.extra, 7)

    def test_fromisoformat_datetime(self):
        # Test that isoformat() is reversible
        base_dates = [
            (1, 1, 1),
            (1900, 1, 1),
            (2004, 11, 12),
            (2017, 5, 30)
        ]

        base_times = [
            (0, 0, 0, 0),
            (0, 0, 0, 241000),
            (0, 0, 0, 234567),
            (12, 30, 45, 234567)
        ]

        separators = [' ', 'T']

        tzinfos = [None, timezone.utc,
                   timezone(timedelta(hours=-5)),
                   timezone(timedelta(hours=2))]

        dts = [self.theclass(*date_tuple, *time_tuple, tzinfo=tzi)
               for date_tuple in base_dates
               for time_tuple in base_times
               for tzi in tzinfos]

        for dt in dts:
            for sep in separators:
                dtstr = dt.isoformat(sep=sep)

                with self.subTest(dtstr=dtstr):
                    dt_rt = self.theclass.fromisoformat(dtstr)
                    self.assertEqual(dt, dt_rt)

    def test_fromisoformat_timezone(self):
        base_dt = self.theclass(2014, 12, 30, 12, 30, 45, 217456)

        tzoffsets = [
            timedelta(hours=5), timedelta(hours=2),
            timedelta(hours=6, minutes=27),
            timedelta(hours=12, minutes=32, seconds=30),
            timedelta(hours=2, minutes=4, seconds=9, microseconds=123456)
        ]

        tzoffsets += [-1 * td for td in tzoffsets]

        tzinfos = [None, timezone.utc,
                   timezone(timedelta(hours=0))]

        tzinfos += [timezone(td) for td in tzoffsets]

        for tzi in tzinfos:
            dt = base_dt.replace(tzinfo=tzi)
            dtstr = dt.isoformat()

            with self.subTest(tstr=dtstr):
                dt_rt = self.theclass.fromisoformat(dtstr)
                assert dt == dt_rt, dt_rt

    def test_fromisoformat_separators(self):
        separators = [
            ' ', 'T', '\u007f',     # 1-bit widths
            '\u0080', 'ʁ',          # 2-bit widths
            'ᛇ', '時',               # 3-bit widths
            '🐍',                    # 4-bit widths
            '\ud800',               # bpo-34454: Surrogate code point
        ]

        for sep in separators:
            dt = self.theclass(2018, 1, 31, 23, 59, 47, 124789)
            dtstr = dt.isoformat(sep=sep)

            with self.subTest(dtstr=dtstr):
                dt_rt = self.theclass.fromisoformat(dtstr)
                self.assertEqual(dt, dt_rt)

    def test_fromisoformat_ambiguous(self):
        # Test strings like 2018-01-31+12:15 (where +12:15 is not a time zone)
        separators = ['+', '-']
        for sep in separators:
            dt = self.theclass(2018, 1, 31, 12, 15)
            dtstr = dt.isoformat(sep=sep)

            with self.subTest(dtstr=dtstr):
                dt_rt = self.theclass.fromisoformat(dtstr)
                self.assertEqual(dt, dt_rt)

    def test_fromisoformat_timespecs(self):
        datetime_bases = [
            (2009, 12, 4, 8, 17, 45, 123456),
            (2009, 12, 4, 8, 17, 45, 0)]

        tzinfos = [None, timezone.utc,
                   timezone(timedelta(hours=-5)),
                   timezone(timedelta(hours=2)),
                   timezone(timedelta(hours=6, minutes=27))]

        timespecs = ['hours', 'minutes', 'seconds',
                     'milliseconds', 'microseconds']

        for ip, ts in enumerate(timespecs):
            for tzi in tzinfos:
                for dt_tuple in datetime_bases:
                    if ts == 'milliseconds':
                        new_microseconds = 1000 * (dt_tuple[6] // 1000)
                        dt_tuple = dt_tuple[0:6] + (new_microseconds,)

                    dt = self.theclass(*(dt_tuple[0:(4 + ip)]), tzinfo=tzi)
                    dtstr = dt.isoformat(timespec=ts)
                    with self.subTest(dtstr=dtstr):
                        dt_rt = self.theclass.fromisoformat(dtstr)
                        self.assertEqual(dt, dt_rt)

    def test_fromisoformat_fails_datetime(self):
        # Test that fromisoformat() fails on invalid values
        bad_strs = [
            '',                             # Empty string
            '\ud800',                       # bpo-34454: Surrogate code point
            '2009.04-19T03',                # Wrong first separator
            '2009-04.19T03',                # Wrong second separator
            '2009-04-19T0a',                # Invalid hours
            '2009-04-19T03:1a:45',          # Invalid minutes
            '2009-04-19T03:15:4a',          # Invalid seconds
            '2009-04-19T03;15:45',          # Bad first time separator
            '2009-04-19T03:15;45',          # Bad second time separator
            '2009-04-19T03:15:4500:00',     # Bad time zone separator
            '2009-04-19T03:15:45.2345',     # Too many digits for milliseconds
            '2009-04-19T03:15:45.1234567',  # Too many digits for microseconds
            '2009-04-19T03:15:45.123456+24:30',    # Invalid time zone offset
            '2009-04-19T03:15:45.123456-24:30',    # Invalid negative offset
            '2009-04-10ᛇᛇᛇᛇᛇ12:15',         # Too many unicode separators
            '2009-04\ud80010T12:15',        # Surrogate char in date
            '2009-04-10T12\ud80015',        # Surrogate char in time
            '2009-04-19T1',                 # Incomplete hours
            '2009-04-19T12:3',              # Incomplete minutes
            '2009-04-19T12:30:4',           # Incomplete seconds
            '2009-04-19T12:',               # Ends with time separator
            '2009-04-19T12:30:',            # Ends with time separator
            '2009-04-19T12:30:45.',         # Ends with time separator
            '2009-04-19T12:30:45.123456+',  # Ends with timzone separator
            '2009-04-19T12:30:45.123456-',  # Ends with timzone separator
            '2009-04-19T12:30:45.123456-05:00a',    # Extra text
            '2009-04-19T12:30:45.123-05:00a',       # Extra text
            '2009-04-19T12:30:45-05:00a',           # Extra text
        ]

        for bad_str in bad_strs:
            with self.subTest(bad_str=bad_str):
                with self.assertRaises(ValueError):
                    self.theclass.fromisoformat(bad_str)

    def test_fromisoformat_fails_surrogate(self):
        # Test that when fromisoformat() fails with a surrogate character as
        # the separator, the error message contains the original string
        dtstr = "2018-01-03\ud80001:0113"

        with self.assertRaisesRegex(ValueError, re.escape(repr(dtstr))):
            self.theclass.fromisoformat(dtstr)

    def test_fromisoformat_utc(self):
        dt_str = '2014-04-19T13:21:13+00:00'
        dt = self.theclass.fromisoformat(dt_str)

        self.assertIs(dt.tzinfo, timezone.utc)

    def test_fromisoformat_subclass(self):
        class DateTimeSubclass(self.theclass):
            pass

        dt = DateTimeSubclass(2014, 12, 14, 9, 30, 45, 457390,
                              tzinfo=timezone(timedelta(hours=10, minutes=45)))

        dt_rt = DateTimeSubclass.fromisoformat(dt.isoformat())

        self.assertEqual(dt, dt_rt)
        self.assertIsInstance(dt_rt, DateTimeSubclass)


class TestSubclassDateTime(TestDateTime):
    theclass = SubclassDatetime
    # Override tests not designed for subclass
    @unittest.skip('not appropriate for subclasses')
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
        self.assertFalse(t1 != t2)
        self.assertFalse(t1 < t2)
        self.assertFalse(t1 > t2)

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
            self.assertFalse(t1 == t2)
            self.assertFalse(t2 == t1)
            self.assertFalse(t1 > t2)
            self.assertFalse(t2 < t1)
            self.assertFalse(t1 >= t2)
            self.assertFalse(t2 <= t1)

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

        t = self.theclass(hour=12, minute=34, second=56, microsecond=123456)
        self.assertEqual(t.isoformat(timespec='hours'), "12")
        self.assertEqual(t.isoformat(timespec='minutes'), "12:34")
        self.assertEqual(t.isoformat(timespec='seconds'), "12:34:56")
        self.assertEqual(t.isoformat(timespec='milliseconds'), "12:34:56.123")
        self.assertEqual(t.isoformat(timespec='microseconds'), "12:34:56.123456")
        self.assertEqual(t.isoformat(timespec='auto'), "12:34:56.123456")
        self.assertRaises(ValueError, t.isoformat, timespec='monkey')
        # bpo-34482: Check that surrogates are handled properly.
        self.assertRaises(ValueError, t.isoformat, timespec='\ud800')

        t = self.theclass(hour=12, minute=34, second=56, microsecond=999500)
        self.assertEqual(t.isoformat(timespec='milliseconds'), "12:34:56.999")

        t = self.theclass(hour=12, minute=34, second=56, microsecond=0)
        self.assertEqual(t.isoformat(timespec='milliseconds'), "12:34:56.000")
        self.assertEqual(t.isoformat(timespec='microseconds'), "12:34:56.000000")
        self.assertEqual(t.isoformat(timespec='auto'), "12:34:56")

    def test_isoformat_timezone(self):
        tzoffsets = [
            ('05:00', timedelta(hours=5)),
            ('02:00', timedelta(hours=2)),
            ('06:27', timedelta(hours=6, minutes=27)),
            ('12:32:30', timedelta(hours=12, minutes=32, seconds=30)),
            ('02:04:09.123456', timedelta(hours=2, minutes=4, seconds=9, microseconds=123456))
        ]

        tzinfos = [
            ('', None),
            ('+00:00', timezone.utc),
            ('+00:00', timezone(timedelta(0))),
        ]

        tzinfos += [
            (prefix + expected, timezone(sign * td))
            for expected, td in tzoffsets
            for prefix, sign in [('-', -1), ('+', 1)]
        ]

        t_base = self.theclass(12, 37, 9)
        exp_base = '12:37:09'

        for exp_tz, tzi in tzinfos:
            t = t_base.replace(tzinfo=tzi)
            exp = exp_base + exp_tz
            with self.subTest(tzi=tzi):
                assert t.isoformat() == exp

    def test_1653736(self):
        # verify it doesn't accept extra keyword arguments
        t = self.theclass(second=1)
        self.assertRaises(TypeError, t.isoformat, foo=3)

    def test_strftime(self):
        t = self.theclass(1, 2, 3, 4)
        self.assertEqual(t.strftime('%H %M %S %f'), "01 02 03 000004")
        # A naive object replaces %z and %Z with empty strings.
        self.assertEqual(t.strftime("'%z' '%Z'"), "'' ''")

        # bpo-34482: Check that surrogates don't cause a crash.
        try:
            t.strftime('%H\ud800%M')
        except UnicodeEncodeError:
            pass

    def test_format(self):
        t = self.theclass(1, 2, 3, 4)
        self.assertEqual(t.__format__(''), str(t))

        with self.assertRaisesRegex(TypeError, 'must be str, not int'):
            t.__format__(123)

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
        self.assertEqual(orig.__reduce__(), orig.__reduce_ex__(2))

    def test_pickling_subclass_time(self):
        args = 20, 59, 16, 64**2
        orig = SubclassTime(*args)
        for pickler, unpickler, proto in pickle_choices:
            green = pickler.dumps(orig, proto)
            derived = unpickler.loads(green)
            self.assertEqual(orig, derived)
            self.assertTrue(isinstance(derived, SubclassTime))

    def test_compat_unpickle(self):
        tests = [
            (b"cdatetime\ntime\n(S'\\x14;\\x10\\x00\\x10\\x00'\ntR.",
             (20, 59, 16, 64**2)),
            (b'cdatetime\ntime\n(U\x06\x14;\x10\x00\x10\x00tR.',
             (20, 59, 16, 64**2)),
            (b'\x80\x02cdatetime\ntime\nU\x06\x14;\x10\x00\x10\x00\x85R.',
             (20, 59, 16, 64**2)),
            (b"cdatetime\ntime\n(S'\\x14;\\x19\\x00\\x10\\x00'\ntR.",
             (20, 59, 25, 64**2)),
            (b'cdatetime\ntime\n(U\x06\x14;\x19\x00\x10\x00tR.',
             (20, 59, 25, 64**2)),
            (b'\x80\x02cdatetime\ntime\nU\x06\x14;\x19\x00\x10\x00\x85R.',
             (20, 59, 25, 64**2)),
        ]
        for i, (data, args) in enumerate(tests):
            with self.subTest(i=i):
                expected = self.theclass(*args)
                for loads in pickle_loads:
                    derived = loads(data, encoding='latin1')
                    self.assertEqual(derived, expected)

    def test_bool(self):
        # time is always True.
        cls = self.theclass
        self.assertTrue(cls(1))
        self.assertTrue(cls(0, 1))
        self.assertTrue(cls(0, 0, 1))
        self.assertTrue(cls(0, 0, 0, 1))
        self.assertTrue(cls(0))
        self.assertTrue(cls())

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

    def test_subclass_replace(self):
        class TimeSubclass(self.theclass):
            pass

        ctime = TimeSubclass(12, 30)
        self.assertIs(type(ctime.replace(hour=10)), TimeSubclass)

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
        # Good bytes, but bad tzinfo:
        with self.assertRaisesRegex(TypeError, '^bad tzinfo state arg$'):
            self.theclass(bytes([1] * len(base)), 'EST')

# A mixin for classes with a tzinfo= argument.  Subclasses must define
# theclass as a class attribute, and theclass(1, 1, 1, tzinfo=whatever)
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
        self.assertIs(t.tzinfo, b)

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
            self.assertIsNone(t.utcoffset())
            self.assertIsNone(t.dst())
            self.assertIsNone(t.tzname())

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

        # Not a whole number of seconds.
        class C7(tzinfo):
            def utcoffset(self, dt): return timedelta(microseconds=61)
            def dst(self, dt): return timedelta(microseconds=-81)
        t = cls(1, 1, 1, tzinfo=C7())
        self.assertEqual(t.utcoffset(), timedelta(microseconds=61))
        self.assertEqual(t.dst(), timedelta(microseconds=-81))

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
        # Note that a time can't actually have an operand-dependent offset,
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
        self.assertIsNone(t.tzinfo)

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
        self.assertIsNone(t4.tzinfo)
        self.assertEqual(t5.tzinfo, utc)

        self.assertEqual(t1.utcoffset(), timedelta(minutes=-300))
        self.assertEqual(t2.utcoffset(), timedelta(minutes=0))
        self.assertEqual(t3.utcoffset(), timedelta(minutes=60))
        self.assertIsNone(t4.utcoffset())
        self.assertRaises(TypeError, t1.utcoffset, "no args")

        self.assertEqual(t1.tzname(), "EST")
        self.assertEqual(t2.tzname(), "UTC")
        self.assertEqual(t3.tzname(), "MET")
        self.assertIsNone(t4.tzname())
        self.assertRaises(TypeError, t1.tzname, "no args")

        self.assertEqual(t1.dst(), timedelta(minutes=1))
        self.assertEqual(t2.dst(), timedelta(minutes=-2))
        self.assertEqual(t3.dst(), timedelta(minutes=3))
        self.assertIsNone(t4.dst())
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
        if '_Fast' in self.__class__.__name__:
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
        self.assertEqual(orig.__reduce__(), orig.__reduce_ex__(2))

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
        self.assertEqual(orig.__reduce__(), orig.__reduce_ex__(2))

    def test_compat_unpickle(self):
        tests = [
            b"cdatetime\ntime\n(S'\\x05\\x06\\x07\\x01\\xe2@'\n"
            b"ctest.datetimetester\nPicklableFixedOffset\n(tR"
            b"(dS'_FixedOffset__offset'\ncdatetime\ntimedelta\n"
            b"(I-1\nI68400\nI0\ntRs"
            b"S'_FixedOffset__dstoffset'\nNs"
            b"S'_FixedOffset__name'\nS'cookie'\nsbtR.",

            b'cdatetime\ntime\n(U\x06\x05\x06\x07\x01\xe2@'
            b'ctest.datetimetester\nPicklableFixedOffset\n)R'
            b'}(U\x14_FixedOffset__offsetcdatetime\ntimedelta\n'
            b'(J\xff\xff\xff\xffJ0\x0b\x01\x00K\x00tR'
            b'U\x17_FixedOffset__dstoffsetN'
            b'U\x12_FixedOffset__nameU\x06cookieubtR.',

            b'\x80\x02cdatetime\ntime\nU\x06\x05\x06\x07\x01\xe2@'
            b'ctest.datetimetester\nPicklableFixedOffset\n)R'
            b'}(U\x14_FixedOffset__offsetcdatetime\ntimedelta\n'
            b'J\xff\xff\xff\xffJ0\x0b\x01\x00K\x00\x87R'
            b'U\x17_FixedOffset__dstoffsetN'
            b'U\x12_FixedOffset__nameU\x06cookieub\x86R.',
        ]

        tinfo = PicklableFixedOffset(-300, 'cookie')
        expected = self.theclass(5, 6, 7, 123456, tzinfo=tinfo)
        for data in tests:
            for loads in pickle_loads:
                derived = loads(data, encoding='latin1')
                self.assertEqual(derived, expected, repr(data))
                self.assertIsInstance(derived.tzinfo, PicklableFixedOffset)
                self.assertEqual(derived.utcoffset(), timedelta(minutes=-300))
                self.assertEqual(derived.tzname(), 'cookie')

    def test_more_bool(self):
        # time is always True.
        cls = self.theclass

        t = cls(0, tzinfo=FixedOffset(-300, ""))
        self.assertTrue(t)

        t = cls(5, tzinfo=FixedOffset(-300, ""))
        self.assertTrue(t)

        t = cls(5, tzinfo=FixedOffset(300, ""))
        self.assertTrue(t)

        t = cls(23, 59, tzinfo=FixedOffset(23*60 + 59, ""))
        self.assertTrue(t)

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
        self.assertIsNone(base2.tzinfo)
        self.assertIsNone(base2.tzname())

        # Ensure we can add one.
        base3 = base2.replace(tzinfo=z100)
        self.assertEqual(base, base3)
        self.assertIs(base.tzinfo, base3.tzinfo)

        # Out of bounds.
        base = cls(1)
        self.assertRaises(ValueError, base.replace, hour=24)
        self.assertRaises(ValueError, base.replace, minute=-1)
        self.assertRaises(ValueError, base.replace, second=100)
        self.assertRaises(ValueError, base.replace, microsecond=1000000)

    def test_mixed_compare(self):
        t1 = self.theclass(1, 2, 3)
        t2 = self.theclass(1, 2, 3)
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

    def test_fromisoformat(self):
        time_examples = [
            (0, 0, 0, 0),
            (23, 59, 59, 999999),
        ]

        hh = (9, 12, 20)
        mm = (5, 30)
        ss = (4, 45)
        usec = (0, 245000, 678901)

        time_examples += list(itertools.product(hh, mm, ss, usec))

        tzinfos = [None, timezone.utc,
                   timezone(timedelta(hours=2)),
                   timezone(timedelta(hours=6, minutes=27))]

        for ttup in time_examples:
            for tzi in tzinfos:
                t = self.theclass(*ttup, tzinfo=tzi)
                tstr = t.isoformat()

                with self.subTest(tstr=tstr):
                    t_rt = self.theclass.fromisoformat(tstr)
                    self.assertEqual(t, t_rt)

    def test_fromisoformat_timezone(self):
        base_time = self.theclass(12, 30, 45, 217456)

        tzoffsets = [
            timedelta(hours=5), timedelta(hours=2),
            timedelta(hours=6, minutes=27),
            timedelta(hours=12, minutes=32, seconds=30),
            timedelta(hours=2, minutes=4, seconds=9, microseconds=123456)
        ]

        tzoffsets += [-1 * td for td in tzoffsets]

        tzinfos = [None, timezone.utc,
                   timezone(timedelta(hours=0))]

        tzinfos += [timezone(td) for td in tzoffsets]

        for tzi in tzinfos:
            t = base_time.replace(tzinfo=tzi)
            tstr = t.isoformat()

            with self.subTest(tstr=tstr):
                t_rt = self.theclass.fromisoformat(tstr)
                assert t == t_rt, t_rt

    def test_fromisoformat_timespecs(self):
        time_bases = [
            (8, 17, 45, 123456),
            (8, 17, 45, 0)
        ]

        tzinfos = [None, timezone.utc,
                   timezone(timedelta(hours=-5)),
                   timezone(timedelta(hours=2)),
                   timezone(timedelta(hours=6, minutes=27))]

        timespecs = ['hours', 'minutes', 'seconds',
                     'milliseconds', 'microseconds']

        for ip, ts in enumerate(timespecs):
            for tzi in tzinfos:
                for t_tuple in time_bases:
                    if ts == 'milliseconds':
                        new_microseconds = 1000 * (t_tuple[-1] // 1000)
                        t_tuple = t_tuple[0:-1] + (new_microseconds,)

                    t = self.theclass(*(t_tuple[0:(1 + ip)]), tzinfo=tzi)
                    tstr = t.isoformat(timespec=ts)
                    with self.subTest(tstr=tstr):
                        t_rt = self.theclass.fromisoformat(tstr)
                        self.assertEqual(t, t_rt)

    def test_fromisoformat_fails(self):
        bad_strs = [
            '',                         # Empty string
            '12\ud80000',               # Invalid separator - surrogate char
            '12:',                      # Ends on a separator
            '12:30:',                   # Ends on a separator
            '12:30:15.',                # Ends on a separator
            '1',                        # Incomplete hours
            '12:3',                     # Incomplete minutes
            '12:30:1',                  # Incomplete seconds
            '1a:30:45.334034',          # Invalid character in hours
            '12:a0:45.334034',          # Invalid character in minutes
            '12:30:a5.334034',          # Invalid character in seconds
            '12:30:45.1234',            # Too many digits for milliseconds
            '12:30:45.1234567',         # Too many digits for microseconds
            '12:30:45.123456+24:30',    # Invalid time zone offset
            '12:30:45.123456-24:30',    # Invalid negative offset
            '12：30：45',                 # Uses full-width unicode colons
            '12:30:45․123456',          # Uses \u2024 in place of decimal point
            '12:30:45a',                # Extra at tend of basic time
            '12:30:45.123a',            # Extra at end of millisecond time
            '12:30:45.123456a',         # Extra at end of microsecond time
            '12:30:45.123456+12:00:30a',    # Extra at end of full time
        ]

        for bad_str in bad_strs:
            with self.subTest(bad_str=bad_str):
                with self.assertRaises(ValueError):
                    self.theclass.fromisoformat(bad_str)

    def test_fromisoformat_fails_typeerror(self):
        # Test the fromisoformat fails when passed the wrong type
        bad_types = [b'12:30:45', None, io.StringIO('12:30:45')]

        for bad_type in bad_types:
            with self.assertRaises(TypeError):
                self.theclass.fromisoformat(bad_type)

    def test_fromisoformat_subclass(self):
        class TimeSubclass(self.theclass):
            pass

        tsc = TimeSubclass(12, 14, 45, 203745, tzinfo=timezone.utc)
        tsc_rt = TimeSubclass.fromisoformat(tsc.isoformat())

        self.assertEqual(tsc, tsc_rt)
        self.assertIsInstance(tsc_rt, TimeSubclass)

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

        # Equal after adjustment.
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
        # and > comparison should fail
        with self.assertRaises(TypeError):
            t1 > t2

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
        self.assertEqual(orig.__reduce__(), orig.__reduce_ex__(2))

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
        self.assertEqual(orig.__reduce__(), orig.__reduce_ex__(2))

    def test_compat_unpickle(self):
        tests = [
            b'cdatetime\ndatetime\n'
            b"(S'\\x07\\xdf\\x0b\\x1b\\x14;\\x01\\x01\\xe2@'\n"
            b'ctest.datetimetester\nPicklableFixedOffset\n(tR'
            b"(dS'_FixedOffset__offset'\ncdatetime\ntimedelta\n"
            b'(I-1\nI68400\nI0\ntRs'
            b"S'_FixedOffset__dstoffset'\nNs"
            b"S'_FixedOffset__name'\nS'cookie'\nsbtR.",

            b'cdatetime\ndatetime\n'
            b'(U\n\x07\xdf\x0b\x1b\x14;\x01\x01\xe2@'
            b'ctest.datetimetester\nPicklableFixedOffset\n)R'
            b'}(U\x14_FixedOffset__offsetcdatetime\ntimedelta\n'
            b'(J\xff\xff\xff\xffJ0\x0b\x01\x00K\x00tR'
            b'U\x17_FixedOffset__dstoffsetN'
            b'U\x12_FixedOffset__nameU\x06cookieubtR.',

            b'\x80\x02cdatetime\ndatetime\n'
            b'U\n\x07\xdf\x0b\x1b\x14;\x01\x01\xe2@'
            b'ctest.datetimetester\nPicklableFixedOffset\n)R'
            b'}(U\x14_FixedOffset__offsetcdatetime\ntimedelta\n'
            b'J\xff\xff\xff\xffJ0\x0b\x01\x00K\x00\x87R'
            b'U\x17_FixedOffset__dstoffsetN'
            b'U\x12_FixedOffset__nameU\x06cookieub\x86R.',
        ]
        args = 2015, 11, 27, 20, 59, 1, 123456
        tinfo = PicklableFixedOffset(-300, 'cookie')
        expected = self.theclass(*args, **{'tzinfo': tinfo})
        for data in tests:
            for loads in pickle_loads:
                derived = loads(data, encoding='latin1')
                self.assertEqual(derived, expected)
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
        now = self.theclass.now()
        tz55 = FixedOffset(-330, "west 5:30")
        timeaware = now.time().replace(tzinfo=tz55)
        nowaware = self.theclass.combine(now.date(), timeaware)
        self.assertIs(nowaware.tzinfo, tz55)
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
        self.assertIs(nowaware.tzinfo, tz55)
        nowawareplus2 = delta + nowaware
        self.assertIs(nowawareplus2.tzinfo, tz55)
        self.assertEqual(nowawareplus, nowawareplus2)

        # that - delta should be what we started with, and that - what we
        # started with should be delta.
        diff = nowawareplus - delta
        self.assertIs(diff.tzinfo, tz55)
        self.assertEqual(nowaware, diff)
        self.assertRaises(TypeError, lambda: delta - nowawareplus)
        self.assertEqual(nowawareplus - nowaware, delta)

        # Make up a random timezone.
        tzr = FixedOffset(random.randrange(-1439, 1440), "randomtimezone")
        # Attach it to nowawareplus.
        nowawareplus = nowawareplus.replace(tzinfo=tzr)
        self.assertIs(nowawareplus.tzinfo, tzr)
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
        self.assertIs(another.tzinfo, again.tzinfo)
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
                self.assertIs(now.tzinfo, weirdtz)
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
        self.assertIs(another.tzinfo, again.tzinfo)
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
        self.assertIsNone(base2.tzinfo)
        self.assertIsNone(base2.tzname())

        # Ensure we can add one.
        base3 = base2.replace(tzinfo=z100)
        self.assertEqual(base, base3)
        self.assertIs(base.tzinfo, base3.tzinfo)

        # Out of bounds.
        base = cls(2000, 2, 29)
        self.assertRaises(ValueError, base.replace, year=2001)

    def test_more_astimezone(self):
        # The inherited test_astimezone covered some trivial and error cases.
        fnone = FixedOffset(None, "None")
        f44m = FixedOffset(44, "44")
        fm5h = FixedOffset(-timedelta(hours=5), "m300")

        dt = self.theclass.now(tz=f44m)
        self.assertIs(dt.tzinfo, f44m)
        # Replacing with degenerate tzinfo raises an exception.
        self.assertRaises(ValueError, dt.astimezone, fnone)
        # Replacing with same tzinfo makes no change.
        x = dt.astimezone(dt.tzinfo)
        self.assertIs(x.tzinfo, f44m)
        self.assertEqual(x.date(), dt.date())
        self.assertEqual(x.time(), dt.time())

        # Replacing with different tzinfo does adjust.
        got = dt.astimezone(fm5h)
        self.assertIs(got.tzinfo, fm5h)
        self.assertEqual(got.utcoffset(), timedelta(hours=-5))
        expected = dt - dt.utcoffset()  # in effect, convert to UTC
        expected += fm5h.utcoffset(dt)  # and from there to local time
        expected = expected.replace(tzinfo=fm5h) # and attach new tzinfo
        self.assertEqual(got.date(), expected.date())
        self.assertEqual(got.time(), expected.time())
        self.assertEqual(got.timetz(), expected.timetz())
        self.assertIs(got.tzinfo, expected.tzinfo)
        self.assertEqual(got, expected)

    @support.run_with_tz('UTC')
    def test_astimezone_default_utc(self):
        dt = self.theclass.now(timezone.utc)
        self.assertEqual(dt.astimezone(None), dt)
        self.assertEqual(dt.astimezone(), dt)

    # Note that offset in TZ variable has the opposite sign to that
    # produced by %z directive.
    @support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_astimezone_default_eastern(self):
        dt = self.theclass(2012, 11, 4, 6, 30, tzinfo=timezone.utc)
        local = dt.astimezone()
        self.assertEqual(dt, local)
        self.assertEqual(local.strftime("%z %Z"), "-0500 EST")
        dt = self.theclass(2012, 11, 4, 5, 30, tzinfo=timezone.utc)
        local = dt.astimezone()
        self.assertEqual(dt, local)
        self.assertEqual(local.strftime("%z %Z"), "-0400 EDT")

    @support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_astimezone_default_near_fold(self):
        # Issue #26616.
        u = datetime(2015, 11, 1, 5, tzinfo=timezone.utc)
        t = u.astimezone()
        s = t.astimezone()
        self.assertEqual(t.tzinfo, s.tzinfo)

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
        # class (like utc_real and utc_fake), but not for Eastern or Central.
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
        self.assertFalse(as_date == as_datetime)
        self.assertFalse(as_datetime == as_date)
        self.assertRaises(TypeError, lambda: as_date < as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime < as_date)
        self.assertRaises(TypeError, lambda: as_date <= as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime <= as_date)
        self.assertRaises(TypeError, lambda: as_date > as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime > as_date)
        self.assertRaises(TypeError, lambda: as_date >= as_datetime)
        self.assertRaises(TypeError, lambda: as_datetime >= as_date)

        # Nevertheless, comparison should work with the base-class (date)
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

    def test_extra_attributes(self):
        for x in [date.today(),
                  time(),
                  datetime.utcnow(),
                  timedelta(),
                  tzinfo(),
                  timezone(timedelta())]:
            with self.assertRaises(AttributeError):
                x.abc = 1

    def test_check_arg_types(self):
        class Number:
            def __init__(self, value):
                self.value = value
            def __int__(self):
                return self.value

        class Float(float):
            pass

        for xx in [10.0, Float(10.9),
                   decimal.Decimal(10), decimal.Decimal('10.9'),
                   Number(10), Number(10.9),
                   '10']:
            self.assertRaises(TypeError, datetime, xx, 10, 10, 10, 10, 10, 10)
            self.assertRaises(TypeError, datetime, 10, xx, 10, 10, 10, 10, 10)
            self.assertRaises(TypeError, datetime, 10, 10, xx, 10, 10, 10, 10)
            self.assertRaises(TypeError, datetime, 10, 10, 10, xx, 10, 10, 10)
            self.assertRaises(TypeError, datetime, 10, 10, 10, 10, xx, 10, 10)
            self.assertRaises(TypeError, datetime, 10, 10, 10, 10, 10, xx, 10)
            self.assertRaises(TypeError, datetime, 10, 10, 10, 10, 10, 10, xx)


#############################################################################
# Local Time Disambiguation

# An experimental reimplementation of fromutc that respects the "fold" flag.

class tzinfo2(tzinfo):

    def fromutc(self, dt):
        "datetime in UTC -> datetime in local time."

        if not isinstance(dt, datetime):
            raise TypeError("fromutc() requires a datetime argument")
        if dt.tzinfo is not self:
            raise ValueError("dt.tzinfo is not self")
        # Returned value satisfies
        #          dt + ldt.utcoffset() = ldt
        off0 = dt.replace(fold=0).utcoffset()
        off1 = dt.replace(fold=1).utcoffset()
        if off0 is None or off1 is None or dt.dst() is None:
            raise ValueError
        if off0 == off1:
            ldt = dt + off0
            off1 = ldt.utcoffset()
            if off0 == off1:
                return ldt
        # Now, we discovered both possible offsets, so
        # we can just try four possible solutions:
        for off in [off0, off1]:
            ldt = dt + off
            if ldt.utcoffset() == off:
                return ldt
            ldt = ldt.replace(fold=1)
            if ldt.utcoffset() == off:
                return ldt

        raise ValueError("No suitable local time found")

# Reimplementing simplified US timezones to respect the "fold" flag:

class USTimeZone2(tzinfo2):

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
        dt = dt.replace(tzinfo=None)
        if start + HOUR <= dt < end:
            # DST is in effect.
            return HOUR
        elif end <= dt < end + HOUR:
            # Fold (an ambiguous hour): use dt.fold to disambiguate.
            return ZERO if dt.fold else HOUR
        elif start <= dt < start + HOUR:
            # Gap (a non-existent hour): reverse the fold rule.
            return HOUR if dt.fold else ZERO
        else:
            # DST is off.
            return ZERO

Eastern2  = USTimeZone2(-5, "Eastern2",  "EST", "EDT")
Central2  = USTimeZone2(-6, "Central2",  "CST", "CDT")
Mountain2 = USTimeZone2(-7, "Mountain2", "MST", "MDT")
Pacific2  = USTimeZone2(-8, "Pacific2",  "PST", "PDT")

# Europe_Vilnius_1941 tzinfo implementation reproduces the following
# 1941 transition from Olson's tzdist:
#
# Zone NAME           GMTOFF RULES  FORMAT [UNTIL]
# ZoneEurope/Vilnius  1:00   -      CET    1940 Aug  3
#                     3:00   -      MSK    1941 Jun 24
#                     1:00   C-Eur  CE%sT  1944 Aug
#
# $ zdump -v Europe/Vilnius | grep 1941
# Europe/Vilnius  Mon Jun 23 20:59:59 1941 UTC = Mon Jun 23 23:59:59 1941 MSK isdst=0 gmtoff=10800
# Europe/Vilnius  Mon Jun 23 21:00:00 1941 UTC = Mon Jun 23 23:00:00 1941 CEST isdst=1 gmtoff=7200

class Europe_Vilnius_1941(tzinfo):
    def _utc_fold(self):
        return [datetime(1941, 6, 23, 21, tzinfo=self),  # Mon Jun 23 21:00:00 1941 UTC
                datetime(1941, 6, 23, 22, tzinfo=self)]  # Mon Jun 23 22:00:00 1941 UTC

    def _loc_fold(self):
        return [datetime(1941, 6, 23, 23, tzinfo=self),  # Mon Jun 23 23:00:00 1941 MSK / CEST
                datetime(1941, 6, 24, 0, tzinfo=self)]   # Mon Jun 24 00:00:00 1941 CEST

    def utcoffset(self, dt):
        fold_start, fold_stop = self._loc_fold()
        if dt < fold_start:
            return 3 * HOUR
        if dt < fold_stop:
            return (2 if dt.fold else 3) * HOUR
        # if dt >= fold_stop
        return 2 * HOUR

    def dst(self, dt):
        fold_start, fold_stop = self._loc_fold()
        if dt < fold_start:
            return 0 * HOUR
        if dt < fold_stop:
            return (1 if dt.fold else 0) * HOUR
        # if dt >= fold_stop
        return 1 * HOUR

    def tzname(self, dt):
        fold_start, fold_stop = self._loc_fold()
        if dt < fold_start:
            return 'MSK'
        if dt < fold_stop:
            return ('MSK', 'CEST')[dt.fold]
        # if dt >= fold_stop
        return 'CEST'

    def fromutc(self, dt):
        assert dt.fold == 0
        assert dt.tzinfo is self
        if dt.year != 1941:
            raise NotImplementedError
        fold_start, fold_stop = self._utc_fold()
        if dt < fold_start:
            return dt + 3 * HOUR
        if dt < fold_stop:
            return (dt + 2 * HOUR).replace(fold=1)
        # if dt >= fold_stop
        return dt + 2 * HOUR


class TestLocalTimeDisambiguation(unittest.TestCase):

    def test_vilnius_1941_fromutc(self):
        Vilnius = Europe_Vilnius_1941()

        gdt = datetime(1941, 6, 23, 20, 59, 59, tzinfo=timezone.utc)
        ldt = gdt.astimezone(Vilnius)
        self.assertEqual(ldt.strftime("%c %Z%z"),
                         'Mon Jun 23 23:59:59 1941 MSK+0300')
        self.assertEqual(ldt.fold, 0)
        self.assertFalse(ldt.dst())

        gdt = datetime(1941, 6, 23, 21, tzinfo=timezone.utc)
        ldt = gdt.astimezone(Vilnius)
        self.assertEqual(ldt.strftime("%c %Z%z"),
                         'Mon Jun 23 23:00:00 1941 CEST+0200')
        self.assertEqual(ldt.fold, 1)
        self.assertTrue(ldt.dst())

        gdt = datetime(1941, 6, 23, 22, tzinfo=timezone.utc)
        ldt = gdt.astimezone(Vilnius)
        self.assertEqual(ldt.strftime("%c %Z%z"),
                         'Tue Jun 24 00:00:00 1941 CEST+0200')
        self.assertEqual(ldt.fold, 0)
        self.assertTrue(ldt.dst())

    def test_vilnius_1941_toutc(self):
        Vilnius = Europe_Vilnius_1941()

        ldt = datetime(1941, 6, 23, 22, 59, 59, tzinfo=Vilnius)
        gdt = ldt.astimezone(timezone.utc)
        self.assertEqual(gdt.strftime("%c %Z"),
                         'Mon Jun 23 19:59:59 1941 UTC')

        ldt = datetime(1941, 6, 23, 23, 59, 59, tzinfo=Vilnius)
        gdt = ldt.astimezone(timezone.utc)
        self.assertEqual(gdt.strftime("%c %Z"),
                         'Mon Jun 23 20:59:59 1941 UTC')

        ldt = datetime(1941, 6, 23, 23, 59, 59, tzinfo=Vilnius, fold=1)
        gdt = ldt.astimezone(timezone.utc)
        self.assertEqual(gdt.strftime("%c %Z"),
                         'Mon Jun 23 21:59:59 1941 UTC')

        ldt = datetime(1941, 6, 24, 0, tzinfo=Vilnius)
        gdt = ldt.astimezone(timezone.utc)
        self.assertEqual(gdt.strftime("%c %Z"),
                         'Mon Jun 23 22:00:00 1941 UTC')

    def test_constructors(self):
        t = time(0, fold=1)
        dt = datetime(1, 1, 1, fold=1)
        self.assertEqual(t.fold, 1)
        self.assertEqual(dt.fold, 1)
        with self.assertRaises(TypeError):
            time(0, 0, 0, 0, None, 0)

    def test_member(self):
        dt = datetime(1, 1, 1, fold=1)
        t = dt.time()
        self.assertEqual(t.fold, 1)
        t = dt.timetz()
        self.assertEqual(t.fold, 1)

    def test_replace(self):
        t = time(0)
        dt = datetime(1, 1, 1)
        self.assertEqual(t.replace(fold=1).fold, 1)
        self.assertEqual(dt.replace(fold=1).fold, 1)
        self.assertEqual(t.replace(fold=0).fold, 0)
        self.assertEqual(dt.replace(fold=0).fold, 0)
        # Check that replacement of other fields does not change "fold".
        t = t.replace(fold=1, tzinfo=Eastern)
        dt = dt.replace(fold=1, tzinfo=Eastern)
        self.assertEqual(t.replace(tzinfo=None).fold, 1)
        self.assertEqual(dt.replace(tzinfo=None).fold, 1)
        # Out of bounds.
        with self.assertRaises(ValueError):
            t.replace(fold=2)
        with self.assertRaises(ValueError):
            dt.replace(fold=2)
        # Check that fold is a keyword-only argument
        with self.assertRaises(TypeError):
            t.replace(1, 1, 1, None, 1)
        with self.assertRaises(TypeError):
            dt.replace(1, 1, 1, 1, 1, 1, 1, None, 1)

    def test_comparison(self):
        t = time(0)
        dt = datetime(1, 1, 1)
        self.assertEqual(t, t.replace(fold=1))
        self.assertEqual(dt, dt.replace(fold=1))

    def test_hash(self):
        t = time(0)
        dt = datetime(1, 1, 1)
        self.assertEqual(hash(t), hash(t.replace(fold=1)))
        self.assertEqual(hash(dt), hash(dt.replace(fold=1)))

    @support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_fromtimestamp(self):
        s = 1414906200
        dt0 = datetime.fromtimestamp(s)
        dt1 = datetime.fromtimestamp(s + 3600)
        self.assertEqual(dt0.fold, 0)
        self.assertEqual(dt1.fold, 1)

    @support.run_with_tz('Australia/Lord_Howe')
    def test_fromtimestamp_lord_howe(self):
        tm = _time.localtime(1.4e9)
        if _time.strftime('%Z%z', tm) != 'LHST+1030':
            self.skipTest('Australia/Lord_Howe timezone is not supported on this platform')
        # $ TZ=Australia/Lord_Howe date -r 1428158700
        # Sun Apr  5 01:45:00 LHDT 2015
        # $ TZ=Australia/Lord_Howe date -r 1428160500
        # Sun Apr  5 01:45:00 LHST 2015
        s = 1428158700
        t0 = datetime.fromtimestamp(s)
        t1 = datetime.fromtimestamp(s + 1800)
        self.assertEqual(t0, t1)
        self.assertEqual(t0.fold, 0)
        self.assertEqual(t1.fold, 1)

    def test_fromtimestamp_low_fold_detection(self):
        # Ensure that fold detection doesn't cause an
        # OSError for really low values, see bpo-29097
        self.assertEqual(datetime.fromtimestamp(0).fold, 0)

    @support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_timestamp(self):
        dt0 = datetime(2014, 11, 2, 1, 30)
        dt1 = dt0.replace(fold=1)
        self.assertEqual(dt0.timestamp() + 3600,
                         dt1.timestamp())

    @support.run_with_tz('Australia/Lord_Howe')
    def test_timestamp_lord_howe(self):
        tm = _time.localtime(1.4e9)
        if _time.strftime('%Z%z', tm) != 'LHST+1030':
            self.skipTest('Australia/Lord_Howe timezone is not supported on this platform')
        t = datetime(2015, 4, 5, 1, 45)
        s0 = t.replace(fold=0).timestamp()
        s1 = t.replace(fold=1).timestamp()
        self.assertEqual(s0 + 1800, s1)

    @support.run_with_tz('EST+05EDT,M3.2.0,M11.1.0')
    def test_astimezone(self):
        dt0 = datetime(2014, 11, 2, 1, 30)
        dt1 = dt0.replace(fold=1)
        # Convert both naive instances to aware.
        adt0 = dt0.astimezone()
        adt1 = dt1.astimezone()
        # Check that the first instance in DST zone and the second in STD
        self.assertEqual(adt0.tzname(), 'EDT')
        self.assertEqual(adt1.tzname(), 'EST')
        self.assertEqual(adt0 + HOUR, adt1)
        # Aware instances with fixed offset tzinfo's always have fold=0
        self.assertEqual(adt0.fold, 0)
        self.assertEqual(adt1.fold, 0)

    def test_pickle_fold(self):
        t = time(fold=1)
        dt = datetime(1, 1, 1, fold=1)
        for pickler, unpickler, proto in pickle_choices:
            for x in [t, dt]:
                s = pickler.dumps(x, proto)
                y = unpickler.loads(s)
                self.assertEqual(x, y)
                self.assertEqual((0 if proto < 4 else x.fold), y.fold)

    def test_repr(self):
        t = time(fold=1)
        dt = datetime(1, 1, 1, fold=1)
        self.assertEqual(repr(t), 'datetime.time(0, 0, fold=1)')
        self.assertEqual(repr(dt),
                         'datetime.datetime(1, 1, 1, 0, 0, fold=1)')

    def test_dst(self):
        # Let's first establish that things work in regular times.
        dt_summer = datetime(2002, 10, 27, 1, tzinfo=Eastern2) - timedelta.resolution
        dt_winter = datetime(2002, 10, 27, 2, tzinfo=Eastern2)
        self.assertEqual(dt_summer.dst(), HOUR)
        self.assertEqual(dt_winter.dst(), ZERO)
        # The disambiguation flag is ignored
        self.assertEqual(dt_summer.replace(fold=1).dst(), HOUR)
        self.assertEqual(dt_winter.replace(fold=1).dst(), ZERO)

        # Pick local time in the fold.
        for minute in [0, 30, 59]:
            dt = datetime(2002, 10, 27, 1, minute, tzinfo=Eastern2)
            # With fold=0 (the default) it is in DST.
            self.assertEqual(dt.dst(), HOUR)
            # With fold=1 it is in STD.
            self.assertEqual(dt.replace(fold=1).dst(), ZERO)

        # Pick local time in the gap.
        for minute in [0, 30, 59]:
            dt = datetime(2002, 4, 7, 2, minute, tzinfo=Eastern2)
            # With fold=0 (the default) it is in STD.
            self.assertEqual(dt.dst(), ZERO)
            # With fold=1 it is in DST.
            self.assertEqual(dt.replace(fold=1).dst(), HOUR)


    def test_utcoffset(self):
        # Let's first establish that things work in regular times.
        dt_summer = datetime(2002, 10, 27, 1, tzinfo=Eastern2) - timedelta.resolution
        dt_winter = datetime(2002, 10, 27, 2, tzinfo=Eastern2)
        self.assertEqual(dt_summer.utcoffset(), -4 * HOUR)
        self.assertEqual(dt_winter.utcoffset(), -5 * HOUR)
        # The disambiguation flag is ignored
        self.assertEqual(dt_summer.replace(fold=1).utcoffset(), -4 * HOUR)
        self.assertEqual(dt_winter.replace(fold=1).utcoffset(), -5 * HOUR)

    def test_fromutc(self):
        # Let's first establish that things work in regular times.
        u_summer = datetime(2002, 10, 27, 6, tzinfo=Eastern2) - timedelta.resolution
        u_winter = datetime(2002, 10, 27, 7, tzinfo=Eastern2)
        t_summer = Eastern2.fromutc(u_summer)
        t_winter = Eastern2.fromutc(u_winter)
        self.assertEqual(t_summer, u_summer - 4 * HOUR)
        self.assertEqual(t_winter, u_winter - 5 * HOUR)
        self.assertEqual(t_summer.fold, 0)
        self.assertEqual(t_winter.fold, 0)

        # What happens in the fall-back fold?
        u = datetime(2002, 10, 27, 5, 30, tzinfo=Eastern2)
        t0 = Eastern2.fromutc(u)
        u += HOUR
        t1 = Eastern2.fromutc(u)
        self.assertEqual(t0, t1)
        self.assertEqual(t0.fold, 0)
        self.assertEqual(t1.fold, 1)
        # The tricky part is when u is in the local fold:
        u = datetime(2002, 10, 27, 1, 30, tzinfo=Eastern2)
        t = Eastern2.fromutc(u)
        self.assertEqual((t.day, t.hour), (26, 21))
        # .. or gets into the local fold after a standard time adjustment
        u = datetime(2002, 10, 27, 6, 30, tzinfo=Eastern2)
        t = Eastern2.fromutc(u)
        self.assertEqual((t.day, t.hour), (27, 1))

        # What happens in the spring-forward gap?
        u = datetime(2002, 4, 7, 2, 0, tzinfo=Eastern2)
        t = Eastern2.fromutc(u)
        self.assertEqual((t.day, t.hour), (6, 21))

    def test_mixed_compare_regular(self):
        t = datetime(2000, 1, 1, tzinfo=Eastern2)
        self.assertEqual(t, t.astimezone(timezone.utc))
        t = datetime(2000, 6, 1, tzinfo=Eastern2)
        self.assertEqual(t, t.astimezone(timezone.utc))

    def test_mixed_compare_fold(self):
        t_fold = datetime(2002, 10, 27, 1, 45, tzinfo=Eastern2)
        t_fold_utc = t_fold.astimezone(timezone.utc)
        self.assertNotEqual(t_fold, t_fold_utc)
        self.assertNotEqual(t_fold_utc, t_fold)

    def test_mixed_compare_gap(self):
        t_gap = datetime(2002, 4, 7, 2, 45, tzinfo=Eastern2)
        t_gap_utc = t_gap.astimezone(timezone.utc)
        self.assertNotEqual(t_gap, t_gap_utc)
        self.assertNotEqual(t_gap_utc, t_gap)

    def test_hash_aware(self):
        t = datetime(2000, 1, 1, tzinfo=Eastern2)
        self.assertEqual(hash(t), hash(t.replace(fold=1)))
        t_fold = datetime(2002, 10, 27, 1, 45, tzinfo=Eastern2)
        t_gap = datetime(2002, 4, 7, 2, 45, tzinfo=Eastern2)
        self.assertEqual(hash(t_fold), hash(t_fold.replace(fold=1)))
        self.assertEqual(hash(t_gap), hash(t_gap.replace(fold=1)))

SEC = timedelta(0, 1)

def pairs(iterable):
    a, b = itertools.tee(iterable)
    next(b, None)
    return zip(a, b)

class ZoneInfo(tzinfo):
    zoneroot = '/usr/share/zoneinfo'
    def __init__(self, ut, ti):
        """

        :param ut: array
            Array of transition point timestamps
        :param ti: list
            A list of (offset, isdst, abbr) tuples
        :return: None
        """
        self.ut = ut
        self.ti = ti
        self.lt = self.invert(ut, ti)

    @staticmethod
    def invert(ut, ti):
        lt = (array('q', ut), array('q', ut))
        if ut:
            offset = ti[0][0] // SEC
            lt[0][0] += offset
            lt[1][0] += offset
            for i in range(1, len(ut)):
                lt[0][i] += ti[i-1][0] // SEC
                lt[1][i] += ti[i][0] // SEC
        return lt

    @classmethod
    def fromfile(cls, fileobj):
        if fileobj.read(4).decode() != "TZif":
            raise ValueError("not a zoneinfo file")
        fileobj.seek(32)
        counts = array('i')
        counts.fromfile(fileobj, 3)
        if sys.byteorder != 'big':
            counts.byteswap()

        ut = array('i')
        ut.fromfile(fileobj, counts[0])
        if sys.byteorder != 'big':
            ut.byteswap()

        type_indices = array('B')
        type_indices.fromfile(fileobj, counts[0])

        ttis = []
        for i in range(counts[1]):
            ttis.append(struct.unpack(">lbb", fileobj.read(6)))

        abbrs = fileobj.read(counts[2])

        # Convert ttis
        for i, (gmtoff, isdst, abbrind) in enumerate(ttis):
            abbr = abbrs[abbrind:abbrs.find(0, abbrind)].decode()
            ttis[i] = (timedelta(0, gmtoff), isdst, abbr)

        ti = [None] * len(ut)
        for i, idx in enumerate(type_indices):
            ti[i] = ttis[idx]

        self = cls(ut, ti)

        return self

    @classmethod
    def fromname(cls, name):
        path = os.path.join(cls.zoneroot, name)
        with open(path, 'rb') as f:
            return cls.fromfile(f)

    EPOCHORDINAL = date(1970, 1, 1).toordinal()

    def fromutc(self, dt):
        """datetime in UTC -> datetime in local time."""

        if not isinstance(dt, datetime):
            raise TypeError("fromutc() requires a datetime argument")
        if dt.tzinfo is not self:
            raise ValueError("dt.tzinfo is not self")

        timestamp = ((dt.toordinal() - self.EPOCHORDINAL) * 86400
                     + dt.hour * 3600
                     + dt.minute * 60
                     + dt.second)

        if timestamp < self.ut[1]:
            tti = self.ti[0]
            fold = 0
        else:
            idx = bisect.bisect_right(self.ut, timestamp)
            assert self.ut[idx-1] <= timestamp
            assert idx == len(self.ut) or timestamp < self.ut[idx]
            tti_prev, tti = self.ti[idx-2:idx]
            # Detect fold
            shift = tti_prev[0] - tti[0]
            fold = (shift > timedelta(0, timestamp - self.ut[idx-1]))
        dt += tti[0]
        if fold:
            return dt.replace(fold=1)
        else:
            return dt

    def _find_ti(self, dt, i):
        timestamp = ((dt.toordinal() - self.EPOCHORDINAL) * 86400
             + dt.hour * 3600
             + dt.minute * 60
             + dt.second)
        lt = self.lt[dt.fold]
        idx = bisect.bisect_right(lt, timestamp)

        return self.ti[max(0, idx - 1)][i]

    def utcoffset(self, dt):
        return self._find_ti(dt, 0)

    def dst(self, dt):
        isdst = self._find_ti(dt, 1)
        # XXX: We cannot accurately determine the "save" value,
        # so let's return 1h whenever DST is in effect.  Since
        # we don't use dst() in fromutc(), it is unlikely that
        # it will be needed for anything more than bool(dst()).
        return ZERO if isdst else HOUR

    def tzname(self, dt):
        return self._find_ti(dt, 2)

    @classmethod
    def zonenames(cls, zonedir=None):
        if zonedir is None:
            zonedir = cls.zoneroot
        zone_tab = os.path.join(zonedir, 'zone.tab')
        try:
            f = open(zone_tab)
        except OSError:
            return
        with f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    yield line.split()[2]

    @classmethod
    def stats(cls, start_year=1):
        count = gap_count = fold_count = zeros_count = 0
        min_gap = min_fold = timedelta.max
        max_gap = max_fold = ZERO
        min_gap_datetime = max_gap_datetime = datetime.min
        min_gap_zone = max_gap_zone = None
        min_fold_datetime = max_fold_datetime = datetime.min
        min_fold_zone = max_fold_zone = None
        stats_since = datetime(start_year, 1, 1) # Starting from 1970 eliminates a lot of noise
        for zonename in cls.zonenames():
            count += 1
            tz = cls.fromname(zonename)
            for dt, shift in tz.transitions():
                if dt < stats_since:
                    continue
                if shift > ZERO:
                    gap_count += 1
                    if (shift, dt) > (max_gap, max_gap_datetime):
                        max_gap = shift
                        max_gap_zone = zonename
                        max_gap_datetime = dt
                    if (shift, datetime.max - dt) < (min_gap, datetime.max - min_gap_datetime):
                        min_gap = shift
                        min_gap_zone = zonename
                        min_gap_datetime = dt
                elif shift < ZERO:
                    fold_count += 1
                    shift = -shift
                    if (shift, dt) > (max_fold, max_fold_datetime):
                        max_fold = shift
                        max_fold_zone = zonename
                        max_fold_datetime = dt
                    if (shift, datetime.max - dt) < (min_fold, datetime.max - min_fold_datetime):
                        min_fold = shift
                        min_fold_zone = zonename
                        min_fold_datetime = dt
                else:
                    zeros_count += 1
        trans_counts = (gap_count, fold_count, zeros_count)
        print("Number of zones:       %5d" % count)
        print("Number of transitions: %5d = %d (gaps) + %d (folds) + %d (zeros)" %
              ((sum(trans_counts),) + trans_counts))
        print("Min gap:         %16s at %s in %s" % (min_gap, min_gap_datetime, min_gap_zone))
        print("Max gap:         %16s at %s in %s" % (max_gap, max_gap_datetime, max_gap_zone))
        print("Min fold:        %16s at %s in %s" % (min_fold, min_fold_datetime, min_fold_zone))
        print("Max fold:        %16s at %s in %s" % (max_fold, max_fold_datetime, max_fold_zone))


    def transitions(self):
        for (_, prev_ti), (t, ti) in pairs(zip(self.ut, self.ti)):
            shift = ti[0] - prev_ti[0]
            yield datetime.utcfromtimestamp(t), shift

    def nondst_folds(self):
        """Find all folds with the same value of isdst on both sides of the transition."""
        for (_, prev_ti), (t, ti) in pairs(zip(self.ut, self.ti)):
            shift = ti[0] - prev_ti[0]
            if shift < ZERO and ti[1] == prev_ti[1]:
                yield datetime.utcfromtimestamp(t), -shift, prev_ti[2], ti[2]

    @classmethod
    def print_all_nondst_folds(cls, same_abbr=False, start_year=1):
        count = 0
        for zonename in cls.zonenames():
            tz = cls.fromname(zonename)
            for dt, shift, prev_abbr, abbr in tz.nondst_folds():
                if dt.year < start_year or same_abbr and prev_abbr != abbr:
                    continue
                count += 1
                print("%3d) %-30s %s %10s %5s -> %s" %
                      (count, zonename, dt, shift, prev_abbr, abbr))

    def folds(self):
        for t, shift in self.transitions():
            if shift < ZERO:
                yield t, -shift

    def gaps(self):
        for t, shift in self.transitions():
            if shift > ZERO:
                yield t, shift

    def zeros(self):
        for t, shift in self.transitions():
            if not shift:
                yield t


class ZoneInfoTest(unittest.TestCase):
    zonename = 'America/New_York'

    def setUp(self):
        if sys.platform == "vxworks":
            self.skipTest("Skipping zoneinfo tests on VxWorks")
        if sys.platform == "win32":
            self.skipTest("Skipping zoneinfo tests on Windows")
        try:
            self.tz = ZoneInfo.fromname(self.zonename)
        except FileNotFoundError as err:
            self.skipTest("Skipping %s: %s" % (self.zonename, err))

    def assertEquivDatetimes(self, a, b):
        self.assertEqual((a.replace(tzinfo=None), a.fold, id(a.tzinfo)),
                         (b.replace(tzinfo=None), b.fold, id(b.tzinfo)))

    def test_folds(self):
        tz = self.tz
        for dt, shift in tz.folds():
            for x in [0 * shift, 0.5 * shift, shift - timedelta.resolution]:
                udt = dt + x
                ldt = tz.fromutc(udt.replace(tzinfo=tz))
                self.assertEqual(ldt.fold, 1)
                adt = udt.replace(tzinfo=timezone.utc).astimezone(tz)
                self.assertEquivDatetimes(adt, ldt)
                utcoffset = ldt.utcoffset()
                self.assertEqual(ldt.replace(tzinfo=None), udt + utcoffset)
                # Round trip
                self.assertEquivDatetimes(ldt.astimezone(timezone.utc),
                                          udt.replace(tzinfo=timezone.utc))


            for x in [-timedelta.resolution, shift]:
                udt = dt + x
                udt = udt.replace(tzinfo=tz)
                ldt = tz.fromutc(udt)
                self.assertEqual(ldt.fold, 0)

    def test_gaps(self):
        tz = self.tz
        for dt, shift in tz.gaps():
            for x in [0 * shift, 0.5 * shift, shift - timedelta.resolution]:
                udt = dt + x
                udt = udt.replace(tzinfo=tz)
                ldt = tz.fromutc(udt)
                self.assertEqual(ldt.fold, 0)
                adt = udt.replace(tzinfo=timezone.utc).astimezone(tz)
                self.assertEquivDatetimes(adt, ldt)
                utcoffset = ldt.utcoffset()
                self.assertEqual(ldt.replace(tzinfo=None), udt.replace(tzinfo=None) + utcoffset)
                # Create a local time inside the gap
                ldt = tz.fromutc(dt.replace(tzinfo=tz)) - shift + x
                self.assertLess(ldt.replace(fold=1).utcoffset(),
                                ldt.replace(fold=0).utcoffset(),
                                "At %s." % ldt)

            for x in [-timedelta.resolution, shift]:
                udt = dt + x
                ldt = tz.fromutc(udt.replace(tzinfo=tz))
                self.assertEqual(ldt.fold, 0)

    def test_system_transitions(self):
        if ('Riyadh8' in self.zonename or
            # From tzdata NEWS file:
            # The files solar87, solar88, and solar89 are no longer distributed.
            # They were a negative experiment - that is, a demonstration that
            # tz data can represent solar time only with some difficulty and error.
            # Their presence in the distribution caused confusion, as Riyadh
            # civil time was generally not solar time in those years.
                self.zonename.startswith('right/')):
            self.skipTest("Skipping %s" % self.zonename)
        tz = self.tz
        TZ = os.environ.get('TZ')
        os.environ['TZ'] = self.zonename
        try:
            _time.tzset()
            for udt, shift in tz.transitions():
                if udt.year >= 2037:
                    # System support for times around the end of 32-bit time_t
                    # and later is flaky on many systems.
                    break
                s0 = (udt - datetime(1970, 1, 1)) // SEC
                ss = shift // SEC   # shift seconds
                for x in [-40 * 3600, -20*3600, -1, 0,
                          ss - 1, ss + 20 * 3600, ss + 40 * 3600]:
                    s = s0 + x
                    sdt = datetime.fromtimestamp(s)
                    tzdt = datetime.fromtimestamp(s, tz).replace(tzinfo=None)
                    self.assertEquivDatetimes(sdt, tzdt)
                    s1 = sdt.timestamp()
                    self.assertEqual(s, s1)
                if ss > 0:  # gap
                    # Create local time inside the gap
                    dt = datetime.fromtimestamp(s0) - shift / 2
                    ts0 = dt.timestamp()
                    ts1 = dt.replace(fold=1).timestamp()
                    self.assertEqual(ts0, s0 + ss / 2)
                    self.assertEqual(ts1, s0 - ss / 2)
        finally:
            if TZ is None:
                del os.environ['TZ']
            else:
                os.environ['TZ'] = TZ
            _time.tzset()


class ZoneInfoCompleteTest(unittest.TestSuite):
    def __init__(self):
        tests = []
        if is_resource_enabled('tzdata'):
            for name in ZoneInfo.zonenames():
                Test = type('ZoneInfoTest[%s]' % name, (ZoneInfoTest,), {})
                Test.zonename = name
                for method in dir(Test):
                    if method.startswith('test_'):
                        tests.append(Test(method))
        super().__init__(tests)

# Iran had a sub-minute UTC offset before 1946.
class IranTest(ZoneInfoTest):
    zonename = 'Asia/Tehran'


class CapiTest(unittest.TestCase):
    def setUp(self):
        # Since the C API is not present in the _Pure tests, skip all tests
        if self.__class__.__name__.endswith('Pure'):
            self.skipTest('Not relevant in pure Python')

        # This *must* be called, and it must be called first, so until either
        # restriction is loosened, we'll call it as part of test setup
        _testcapi.test_datetime_capi()

    def test_utc_capi(self):
        for use_macro in (True, False):
            capi_utc = _testcapi.get_timezone_utc_capi(use_macro)

            with self.subTest(use_macro=use_macro):
                self.assertIs(capi_utc, timezone.utc)

    def test_timezones_capi(self):
        est_capi, est_macro, est_macro_nn = _testcapi.make_timezones_capi()

        exp_named = timezone(timedelta(hours=-5), "EST")
        exp_unnamed = timezone(timedelta(hours=-5))

        cases = [
            ('est_capi', est_capi, exp_named),
            ('est_macro', est_macro, exp_named),
            ('est_macro_nn', est_macro_nn, exp_unnamed)
        ]

        for name, tz_act, tz_exp in cases:
            with self.subTest(name=name):
                self.assertEqual(tz_act, tz_exp)

                dt1 = datetime(2000, 2, 4, tzinfo=tz_act)
                dt2 = datetime(2000, 2, 4, tzinfo=tz_exp)

                self.assertEqual(dt1, dt2)
                self.assertEqual(dt1.tzname(), dt2.tzname())

                dt_utc = datetime(2000, 2, 4, 5, tzinfo=timezone.utc)

                self.assertEqual(dt1.astimezone(timezone.utc), dt_utc)

    def test_PyDateTime_DELTA_GET(self):
        class TimeDeltaSubclass(timedelta):
            pass

        for klass in [timedelta, TimeDeltaSubclass]:
            for args in [(26, 55, 99999), (26, 55, 99999)]:
                d = klass(*args)
                with self.subTest(cls=klass, date=args):
                    days, seconds, microseconds = _testcapi.PyDateTime_DELTA_GET(d)

                    self.assertEqual(days, d.days)
                    self.assertEqual(seconds, d.seconds)
                    self.assertEqual(microseconds, d.microseconds)

    def test_PyDateTime_GET(self):
        class DateSubclass(date):
            pass

        for klass in [date, DateSubclass]:
            for args in [(2000, 1, 2), (2012, 2, 29)]:
                d = klass(*args)
                with self.subTest(cls=klass, date=args):
                    year, month, day = _testcapi.PyDateTime_GET(d)

                    self.assertEqual(year, d.year)
                    self.assertEqual(month, d.month)
                    self.assertEqual(day, d.day)

    def test_PyDateTime_DATE_GET(self):
        class DateTimeSubclass(datetime):
            pass

        for klass in [datetime, DateTimeSubclass]:
            for args in [(1993, 8, 26, 22, 12, 55, 99999),
                         (1993, 8, 26, 22, 12, 55, 99999,
                          timezone.utc)]:
                d = klass(*args)
                with self.subTest(cls=klass, date=args):
                    hour, minute, second, microsecond, tzinfo = \
                                            _testcapi.PyDateTime_DATE_GET(d)

                    self.assertEqual(hour, d.hour)
                    self.assertEqual(minute, d.minute)
                    self.assertEqual(second, d.second)
                    self.assertEqual(microsecond, d.microsecond)
                    self.assertIs(tzinfo, d.tzinfo)

    def test_PyDateTime_TIME_GET(self):
        class TimeSubclass(time):
            pass

        for klass in [time, TimeSubclass]:
            for args in [(12, 30, 20, 10),
                         (12, 30, 20, 10, timezone.utc)]:
                d = klass(*args)
                with self.subTest(cls=klass, date=args):
                    hour, minute, second, microsecond, tzinfo = \
                                              _testcapi.PyDateTime_TIME_GET(d)

                    self.assertEqual(hour, d.hour)
                    self.assertEqual(minute, d.minute)
                    self.assertEqual(second, d.second)
                    self.assertEqual(microsecond, d.microsecond)
                    self.assertIs(tzinfo, d.tzinfo)

    def test_timezones_offset_zero(self):
        utc0, utc1, non_utc = _testcapi.get_timezones_offset_zero()

        with self.subTest(testname="utc0"):
            self.assertIs(utc0, timezone.utc)

        with self.subTest(testname="utc1"):
            self.assertIs(utc1, timezone.utc)

        with self.subTest(testname="non_utc"):
            self.assertIsNot(non_utc, timezone.utc)

            non_utc_exp = timezone(timedelta(hours=0), "")

            self.assertEqual(non_utc, non_utc_exp)

            dt1 = datetime(2000, 2, 4, tzinfo=non_utc)
            dt2 = datetime(2000, 2, 4, tzinfo=non_utc_exp)

            self.assertEqual(dt1, dt2)
            self.assertEqual(dt1.tzname(), dt2.tzname())

    def test_check_date(self):
        class DateSubclass(date):
            pass

        d = date(2011, 1, 1)
        ds = DateSubclass(2011, 1, 1)
        dt = datetime(2011, 1, 1)

        is_date = _testcapi.datetime_check_date

        # Check the ones that should be valid
        self.assertTrue(is_date(d))
        self.assertTrue(is_date(dt))
        self.assertTrue(is_date(ds))
        self.assertTrue(is_date(d, True))

        # Check that the subclasses do not match exactly
        self.assertFalse(is_date(dt, True))
        self.assertFalse(is_date(ds, True))

        # Check that various other things are not dates at all
        args = [tuple(), list(), 1, '2011-01-01',
                timedelta(1), timezone.utc, time(12, 00)]
        for arg in args:
            for exact in (True, False):
                with self.subTest(arg=arg, exact=exact):
                    self.assertFalse(is_date(arg, exact))

    def test_check_time(self):
        class TimeSubclass(time):
            pass

        t = time(12, 30)
        ts = TimeSubclass(12, 30)

        is_time = _testcapi.datetime_check_time

        # Check the ones that should be valid
        self.assertTrue(is_time(t))
        self.assertTrue(is_time(ts))
        self.assertTrue(is_time(t, True))

        # Check that the subclass does not match exactly
        self.assertFalse(is_time(ts, True))

        # Check that various other things are not times
        args = [tuple(), list(), 1, '2011-01-01',
                timedelta(1), timezone.utc, date(2011, 1, 1)]

        for arg in args:
            for exact in (True, False):
                with self.subTest(arg=arg, exact=exact):
                    self.assertFalse(is_time(arg, exact))

    def test_check_datetime(self):
        class DateTimeSubclass(datetime):
            pass

        dt = datetime(2011, 1, 1, 12, 30)
        dts = DateTimeSubclass(2011, 1, 1, 12, 30)

        is_datetime = _testcapi.datetime_check_datetime

        # Check the ones that should be valid
        self.assertTrue(is_datetime(dt))
        self.assertTrue(is_datetime(dts))
        self.assertTrue(is_datetime(dt, True))

        # Check that the subclass does not match exactly
        self.assertFalse(is_datetime(dts, True))

        # Check that various other things are not datetimes
        args = [tuple(), list(), 1, '2011-01-01',
                timedelta(1), timezone.utc, date(2011, 1, 1)]

        for arg in args:
            for exact in (True, False):
                with self.subTest(arg=arg, exact=exact):
                    self.assertFalse(is_datetime(arg, exact))

    def test_check_delta(self):
        class TimeDeltaSubclass(timedelta):
            pass

        td = timedelta(1)
        tds = TimeDeltaSubclass(1)

        is_timedelta = _testcapi.datetime_check_delta

        # Check the ones that should be valid
        self.assertTrue(is_timedelta(td))
        self.assertTrue(is_timedelta(tds))
        self.assertTrue(is_timedelta(td, True))

        # Check that the subclass does not match exactly
        self.assertFalse(is_timedelta(tds, True))

        # Check that various other things are not timedeltas
        args = [tuple(), list(), 1, '2011-01-01',
                timezone.utc, date(2011, 1, 1), datetime(2011, 1, 1)]

        for arg in args:
            for exact in (True, False):
                with self.subTest(arg=arg, exact=exact):
                    self.assertFalse(is_timedelta(arg, exact))

    def test_check_tzinfo(self):
        class TZInfoSubclass(tzinfo):
            pass

        tzi = tzinfo()
        tzis = TZInfoSubclass()
        tz = timezone(timedelta(hours=-5))

        is_tzinfo = _testcapi.datetime_check_tzinfo

        # Check the ones that should be valid
        self.assertTrue(is_tzinfo(tzi))
        self.assertTrue(is_tzinfo(tz))
        self.assertTrue(is_tzinfo(tzis))
        self.assertTrue(is_tzinfo(tzi, True))

        # Check that the subclasses do not match exactly
        self.assertFalse(is_tzinfo(tz, True))
        self.assertFalse(is_tzinfo(tzis, True))

        # Check that various other things are not tzinfos
        args = [tuple(), list(), 1, '2011-01-01',
                date(2011, 1, 1), datetime(2011, 1, 1)]

        for arg in args:
            for exact in (True, False):
                with self.subTest(arg=arg, exact=exact):
                    self.assertFalse(is_tzinfo(arg, exact))

    def test_date_from_date(self):
        exp_date = date(1993, 8, 26)

        for macro in False, True:
            with self.subTest(macro=macro):
                c_api_date = _testcapi.get_date_fromdate(
                    macro,
                    exp_date.year,
                    exp_date.month,
                    exp_date.day)

                self.assertEqual(c_api_date, exp_date)

    def test_datetime_from_dateandtime(self):
        exp_date = datetime(1993, 8, 26, 22, 12, 55, 99999)

        for macro in False, True:
            with self.subTest(macro=macro):
                c_api_date = _testcapi.get_datetime_fromdateandtime(
                    macro,
                    exp_date.year,
                    exp_date.month,
                    exp_date.day,
                    exp_date.hour,
                    exp_date.minute,
                    exp_date.second,
                    exp_date.microsecond)

                self.assertEqual(c_api_date, exp_date)

    def test_datetime_from_dateandtimeandfold(self):
        exp_date = datetime(1993, 8, 26, 22, 12, 55, 99999)

        for fold in [0, 1]:
            for macro in False, True:
                with self.subTest(macro=macro, fold=fold):
                    c_api_date = _testcapi.get_datetime_fromdateandtimeandfold(
                        macro,
                        exp_date.year,
                        exp_date.month,
                        exp_date.day,
                        exp_date.hour,
                        exp_date.minute,
                        exp_date.second,
                        exp_date.microsecond,
                        exp_date.fold)

                    self.assertEqual(c_api_date, exp_date)
                    self.assertEqual(c_api_date.fold, exp_date.fold)

    def test_time_from_time(self):
        exp_time = time(22, 12, 55, 99999)

        for macro in False, True:
            with self.subTest(macro=macro):
                c_api_time = _testcapi.get_time_fromtime(
                    macro,
                    exp_time.hour,
                    exp_time.minute,
                    exp_time.second,
                    exp_time.microsecond)

                self.assertEqual(c_api_time, exp_time)

    def test_time_from_timeandfold(self):
        exp_time = time(22, 12, 55, 99999)

        for fold in [0, 1]:
            for macro in False, True:
                with self.subTest(macro=macro, fold=fold):
                    c_api_time = _testcapi.get_time_fromtimeandfold(
                        macro,
                        exp_time.hour,
                        exp_time.minute,
                        exp_time.second,
                        exp_time.microsecond,
                        exp_time.fold)

                    self.assertEqual(c_api_time, exp_time)
                    self.assertEqual(c_api_time.fold, exp_time.fold)

    def test_delta_from_dsu(self):
        exp_delta = timedelta(26, 55, 99999)

        for macro in False, True:
            with self.subTest(macro=macro):
                c_api_delta = _testcapi.get_delta_fromdsu(
                    macro,
                    exp_delta.days,
                    exp_delta.seconds,
                    exp_delta.microseconds)

                self.assertEqual(c_api_delta, exp_delta)

    def test_date_from_timestamp(self):
        ts = datetime(1995, 4, 12).timestamp()

        for macro in False, True:
            with self.subTest(macro=macro):
                d = _testcapi.get_date_fromtimestamp(int(ts), macro)

                self.assertEqual(d, date(1995, 4, 12))

    def test_datetime_from_timestamp(self):
        cases = [
            ((1995, 4, 12), None, False),
            ((1995, 4, 12), None, True),
            ((1995, 4, 12), timezone(timedelta(hours=1)), True),
            ((1995, 4, 12, 14, 30), None, False),
            ((1995, 4, 12, 14, 30), None, True),
            ((1995, 4, 12, 14, 30), timezone(timedelta(hours=1)), True),
        ]

        from_timestamp = _testcapi.get_datetime_fromtimestamp
        for case in cases:
            for macro in False, True:
                with self.subTest(case=case, macro=macro):
                    dtup, tzinfo, usetz = case
                    dt_orig = datetime(*dtup, tzinfo=tzinfo)
                    ts = int(dt_orig.timestamp())

                    dt_rt = from_timestamp(ts, tzinfo, usetz, macro)

                    self.assertEqual(dt_orig, dt_rt)


def load_tests(loader, standard_tests, pattern):
    standard_tests.addTest(ZoneInfoCompleteTest())
    return standard_tests


if __name__ == "__main__":
    unittest.main()
