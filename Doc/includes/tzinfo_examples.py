import datetime as dt

# A class capturing the platform's idea of local time.
# (May result in wrong values on historical times in
#  timezones where UTC offset and/or the DST rules had
#  changed in the past.)
import time as _time

ZERO = dt.timedelta(0)
HOUR = dt.timedelta(hours=1)
SECOND = dt.timedelta(seconds=1)

STDOFFSET = dt.timedelta(seconds=-_time.timezone)
if _time.daylight:
    DSTOFFSET = dt.timedelta(seconds=-_time.altzone)
else:
    DSTOFFSET = STDOFFSET

DSTDIFF = DSTOFFSET - STDOFFSET


class LocalTimezone(dt.tzinfo):

    def fromutc(self, when):
        assert dt.tzinfo is self
        stamp = (when - dt.datetime(1970, 1, 1, tzinfo=self)) // SECOND
        args = _time.localtime(stamp)[:6]
        dst_diff = DSTDIFF // SECOND
        # Detect fold
        fold = (args == _time.localtime(stamp - dst_diff))
        return dt.datetime(*args, microsecond=when.microsecond,
                           tzinfo=self, fold=fold)

    def utcoffset(self, when):
        if self._isdst(when):
            return DSTOFFSET
        else:
            return STDOFFSET

    def dst(self, when):
        if self._isdst(when):
            return DSTDIFF
        else:
            return ZERO

    def tzname(self, when):
        return _time.tzname[self._isdst(when)]

    def _isdst(self, when):
        tt = (when.year, when.month, when.day,
              when.hour, when.minute, when.second,
              when.weekday(), 0, 0)
        stamp = _time.mktime(tt)
        tt = _time.localtime(stamp)
        return tt.tm_isdst > 0


Local = LocalTimezone()


# A complete implementation of current DST rules for major US time zones.

def first_sunday_on_or_after(when):
    days_to_go = 6 - when.weekday()
    if days_to_go:
        when += dt.timedelta(days_to_go)
    return when


# US DST Rules
#
# This is a simplified (i.e., wrong for a few cases) set of rules for US
# DST start and end times. For a complete and up-to-date set of DST rules
# and timezone definitions, visit the Olson Database (or try pytz):
# http://www.twinsun.com/tz/tz-link.htm
# https://sourceforge.net/projects/pytz/ (might not be up-to-date)
#
# In the US, since 2007, DST starts at 2am (standard time) on the second
# Sunday in March, which is the first Sunday on or after Mar 8.
DSTSTART_2007 = dt.datetime(1, 3, 8, 2)
# and ends at 2am (DST time) on the first Sunday of Nov.
DSTEND_2007 = dt.datetime(1, 11, 1, 2)
# From 1987 to 2006, DST used to start at 2am (standard time) on the first
# Sunday in April and to end at 2am (DST time) on the last
# Sunday of October, which is the first Sunday on or after Oct 25.
DSTSTART_1987_2006 = dt.datetime(1, 4, 1, 2)
DSTEND_1987_2006 = dt.datetime(1, 10, 25, 2)
# From 1967 to 1986, DST used to start at 2am (standard time) on the last
# Sunday in April (the one on or after April 24) and to end at 2am (DST time)
# on the last Sunday of October, which is the first Sunday
# on or after Oct 25.
DSTSTART_1967_1986 = dt.datetime(1, 4, 24, 2)
DSTEND_1967_1986 = DSTEND_1987_2006


def us_dst_range(year):
    # Find start and end times for US DST. For years before 1967, return
    # start = end for no DST.
    if 2006 < year:
        dststart, dstend = DSTSTART_2007, DSTEND_2007
    elif 1986 < year < 2007:
        dststart, dstend = DSTSTART_1987_2006, DSTEND_1987_2006
    elif 1966 < year < 1987:
        dststart, dstend = DSTSTART_1967_1986, DSTEND_1967_1986
    else:
        return (dt.datetime(year, 1, 1), ) * 2

    start = first_sunday_on_or_after(dststart.replace(year=year))
    end = first_sunday_on_or_after(dstend.replace(year=year))
    return start, end


class USTimeZone(dt.tzinfo):

    def __init__(self, hours, reprname, stdname, dstname):
        self.stdoffset = dt.timedelta(hours=hours)
        self.reprname = reprname
        self.stdname = stdname
        self.dstname = dstname

    def __repr__(self):
        return self.reprname

    def tzname(self, when):
        if self.dst(when):
            return self.dstname
        else:
            return self.stdname

    def utcoffset(self, when):
        return self.stdoffset + self.dst(when)

    def dst(self, when):
        if when is None or when.tzinfo is None:
            # An exception may be sensible here, in one or both cases.
            # It depends on how you want to treat them.  The default
            # fromutc() implementation (called by the default astimezone()
            # implementation) passes a datetime with when.tzinfo is self.
            return ZERO
        assert when.tzinfo is self
        start, end = us_dst_range(when.year)
        # Can't compare naive to aware objects, so strip the timezone from
        # when first.
        when = when.replace(tzinfo=None)
        if start + HOUR <= dt < end - HOUR:
            # DST is in effect.
            return HOUR
        if end - HOUR <= when < end:
            # Fold (an ambiguous hour): use when.fold to disambiguate.
            return ZERO if when.fold else HOUR
        if start <= when < start + HOUR:
            # Gap (a non-existent hour): reverse the fold rule.
            return HOUR if when.fold else ZERO
        # DST is off.
        return ZERO

    def fromutc(self, when):
        assert when.tzinfo is self
        start, end = us_dst_range(when.year)
        start = start.replace(tzinfo=self)
        end = end.replace(tzinfo=self)
        std_time = when + self.stdoffset
        dst_time = std_time + HOUR
        if end <= dst_time < end + HOUR:
            # Repeated hour
            return std_time.replace(fold=1)
        if std_time < start or dst_time >= end:
            # Standard time
            return std_time
        if start <= std_time < end - HOUR:
            # Daylight saving time
            return dst_time


Eastern  = USTimeZone(-5, "Eastern",  "EST", "EDT")
Central  = USTimeZone(-6, "Central",  "CST", "CDT")
Mountain = USTimeZone(-7, "Mountain", "MST", "MDT")
Pacific  = USTimeZone(-8, "Pacific",  "PST", "PDT")
