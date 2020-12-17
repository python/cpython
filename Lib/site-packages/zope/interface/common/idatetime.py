##############################################################################
# Copyright (c) 2002 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
##############################################################################
"""Datetime interfaces.

This module is called idatetime because if it were called datetime the import
of the real datetime would fail.
"""
from datetime import timedelta, date, datetime, time, tzinfo

from zope.interface import Interface, Attribute
from zope.interface import classImplements


class ITimeDeltaClass(Interface):
    """This is the timedelta class interface.

    This is symbolic; this module does **not** make
    `datetime.timedelta` provide this interface.
    """

    min = Attribute("The most negative timedelta object")

    max = Attribute("The most positive timedelta object")

    resolution = Attribute(
        "The smallest difference between non-equal timedelta objects")


class ITimeDelta(ITimeDeltaClass):
    """Represent the difference between two datetime objects.

    Implemented by `datetime.timedelta`.

    Supported operators:

    - add, subtract timedelta
    - unary plus, minus, abs
    - compare to timedelta
    - multiply, divide by int/long

    In addition, `.datetime` supports subtraction of two `.datetime` objects
    returning a `.timedelta`, and addition or subtraction of a `.datetime`
    and a `.timedelta` giving a `.datetime`.

    Representation: (days, seconds, microseconds).
    """

    days = Attribute("Days between -999999999 and 999999999 inclusive")

    seconds = Attribute("Seconds between 0 and 86399 inclusive")

    microseconds = Attribute("Microseconds between 0 and 999999 inclusive")


class IDateClass(Interface):
    """This is the date class interface.

    This is symbolic; this module does **not** make
    `datetime.date` provide this interface.
    """

    min = Attribute("The earliest representable date")

    max = Attribute("The latest representable date")

    resolution = Attribute(
        "The smallest difference between non-equal date objects")

    def today():
        """Return the current local time.

        This is equivalent to ``date.fromtimestamp(time.time())``"""

    def fromtimestamp(timestamp):
        """Return the local date from a POSIX timestamp (like time.time())

        This may raise `ValueError`, if the timestamp is out of the range of
        values supported by the platform C ``localtime()`` function. It's common
        for this to be restricted to years from 1970 through 2038. Note that
        on non-POSIX systems that include leap seconds in their notion of a
        timestamp, leap seconds are ignored by `fromtimestamp`.
        """

    def fromordinal(ordinal):
        """Return the date corresponding to the proleptic Gregorian ordinal.

         January 1 of year 1 has ordinal 1. `ValueError` is raised unless
         1 <= ordinal <= date.max.toordinal().

         For any date *d*, ``date.fromordinal(d.toordinal()) == d``.
         """


class IDate(IDateClass):
    """Represents a date (year, month and day) in an idealized calendar.

    Implemented by `datetime.date`.

    Operators:

    __repr__, __str__
    __cmp__, __hash__
    __add__, __radd__, __sub__ (add/radd only with timedelta arg)
    """

    year = Attribute("Between MINYEAR and MAXYEAR inclusive.")

    month = Attribute("Between 1 and 12 inclusive")

    day = Attribute(
        "Between 1 and the number of days in the given month of the given year.")

    def replace(year, month, day):
        """Return a date with the same value.

        Except for those members given new values by whichever keyword
        arguments are specified. For example, if ``d == date(2002, 12, 31)``, then
        ``d.replace(day=26) == date(2000, 12, 26)``.
        """

    def timetuple():
        """Return a 9-element tuple of the form returned by `time.localtime`.

        The hours, minutes and seconds are 0, and the DST flag is -1.
        ``d.timetuple()`` is equivalent to
        ``(d.year, d.month, d.day, 0, 0, 0, d.weekday(), d.toordinal() -
        date(d.year, 1, 1).toordinal() + 1, -1)``
        """

    def toordinal():
        """Return the proleptic Gregorian ordinal of the date

        January 1 of year 1 has ordinal 1. For any date object *d*,
        ``date.fromordinal(d.toordinal()) == d``.
        """

    def weekday():
        """Return the day of the week as an integer.

        Monday is 0 and Sunday is 6. For example,
        ``date(2002, 12, 4).weekday() == 2``, a Wednesday.

        .. seealso:: `isoweekday`.
        """

    def isoweekday():
        """Return the day of the week as an integer.

        Monday is 1 and Sunday is 7. For example,
        date(2002, 12, 4).isoweekday() == 3, a Wednesday.

        .. seealso:: `weekday`, `isocalendar`.
        """

    def isocalendar():
        """Return a 3-tuple, (ISO year, ISO week number, ISO weekday).

        The ISO calendar is a widely used variant of the Gregorian calendar.
        See http://www.phys.uu.nl/~vgent/calendar/isocalendar.htm for a good
        explanation.

        The ISO year consists of 52 or 53 full weeks, and where a week starts
        on a Monday and ends on a Sunday. The first week of an ISO year is the
        first (Gregorian) calendar week of a year containing a Thursday. This
        is called week number 1, and the ISO year of that Thursday is the same
        as its Gregorian year.

        For example, 2004 begins on a Thursday, so the first week of ISO year
        2004 begins on Monday, 29 Dec 2003 and ends on Sunday, 4 Jan 2004, so
        that ``date(2003, 12, 29).isocalendar() == (2004, 1, 1)`` and
        ``date(2004, 1, 4).isocalendar() == (2004, 1, 7)``.
        """

    def isoformat():
        """Return a string representing the date in ISO 8601 format.

        This is 'YYYY-MM-DD'.
        For example, ``date(2002, 12, 4).isoformat() == '2002-12-04'``.
        """

    def __str__():
        """For a date *d*, ``str(d)`` is equivalent to ``d.isoformat()``."""

    def ctime():
        """Return a string representing the date.

        For example date(2002, 12, 4).ctime() == 'Wed Dec 4 00:00:00 2002'.
        d.ctime() is equivalent to time.ctime(time.mktime(d.timetuple()))
        on platforms where the native C ctime() function
        (which `time.ctime` invokes, but which date.ctime() does not invoke)
        conforms to the C standard.
        """

    def strftime(format):
        """Return a string representing the date.

        Controlled by an explicit format string. Format codes referring to
        hours, minutes or seconds will see 0 values.
        """


class IDateTimeClass(Interface):
    """This is the datetime class interface.

    This is symbolic; this module does **not** make
    `datetime.datetime` provide this interface.
    """

    min = Attribute("The earliest representable datetime")

    max = Attribute("The latest representable datetime")

    resolution = Attribute(
        "The smallest possible difference between non-equal datetime objects")

    def today():
        """Return the current local datetime, with tzinfo None.

        This is equivalent to ``datetime.fromtimestamp(time.time())``.

        .. seealso:: `now`, `fromtimestamp`.
        """

    def now(tz=None):
        """Return the current local date and time.

        If optional argument *tz* is None or not specified, this is like `today`,
        but, if possible, supplies more precision than can be gotten from going
        through a `time.time` timestamp (for example, this may be possible on
        platforms supplying the C ``gettimeofday()`` function).

        Else tz must be an instance of a class tzinfo subclass, and the current
        date and time are converted to tz's time zone. In this case the result
        is equivalent to tz.fromutc(datetime.utcnow().replace(tzinfo=tz)).

        .. seealso:: `today`, `utcnow`.
        """

    def utcnow():
        """Return the current UTC date and time, with tzinfo None.

        This is like `now`, but returns the current UTC date and time, as a
        naive datetime object.

        .. seealso:: `now`.
        """

    def fromtimestamp(timestamp, tz=None):
        """Return the local date and time corresponding to the POSIX timestamp.

        Same as is returned by time.time(). If optional argument tz is None or
        not specified, the timestamp is converted to the platform's local date
        and time, and the returned datetime object is naive.

        Else tz must be an instance of a class tzinfo subclass, and the
        timestamp is converted to tz's time zone. In this case the result is
        equivalent to
        ``tz.fromutc(datetime.utcfromtimestamp(timestamp).replace(tzinfo=tz))``.

        fromtimestamp() may raise `ValueError`, if the timestamp is out of the
        range of values supported by the platform C localtime() or gmtime()
        functions. It's common for this to be restricted to years in 1970
        through 2038. Note that on non-POSIX systems that include leap seconds
        in their notion of a timestamp, leap seconds are ignored by
        fromtimestamp(), and then it's possible to have two timestamps
        differing by a second that yield identical datetime objects.

        .. seealso:: `utcfromtimestamp`.
        """

    def utcfromtimestamp(timestamp):
        """Return the UTC datetime from the POSIX timestamp with tzinfo None.

        This may raise `ValueError`, if the timestamp is out of the range of
        values supported by the platform C ``gmtime()`` function. It's common for
        this to be restricted to years in 1970 through 2038.

        .. seealso:: `fromtimestamp`.
        """

    def fromordinal(ordinal):
        """Return the datetime from the proleptic Gregorian ordinal.

        January 1 of year 1 has ordinal 1. `ValueError` is raised unless
        1 <= ordinal <= datetime.max.toordinal().
        The hour, minute, second and microsecond of the result are all 0, and
        tzinfo is None.
        """

    def combine(date, time):
        """Return a new datetime object.

        Its date members are equal to the given date object's, and whose time
        and tzinfo members are equal to the given time object's. For any
        datetime object *d*, ``d == datetime.combine(d.date(), d.timetz())``.
        If date is a datetime object, its time and tzinfo members are ignored.
        """


class IDateTime(IDate, IDateTimeClass):
    """Object contains all the information from a date object and a time object.

    Implemented by `datetime.datetime`.
    """

    year = Attribute("Year between MINYEAR and MAXYEAR inclusive")

    month = Attribute("Month between 1 and 12 inclusive")

    day = Attribute(
        "Day between 1 and the number of days in the given month of the year")

    hour = Attribute("Hour in range(24)")

    minute = Attribute("Minute in range(60)")

    second = Attribute("Second in range(60)")

    microsecond = Attribute("Microsecond in range(1000000)")

    tzinfo = Attribute(
        """The object passed as the tzinfo argument to the datetime constructor
        or None if none was passed""")

    def date():
         """Return date object with same year, month and day."""

    def time():
        """Return time object with same hour, minute, second, microsecond.

        tzinfo is None.

        .. seealso:: Method :meth:`timetz`.
        """

    def timetz():
        """Return time object with same hour, minute, second, microsecond,
        and tzinfo.

        .. seealso:: Method :meth:`time`.
        """

    def replace(year, month, day, hour, minute, second, microsecond, tzinfo):
        """Return a datetime with the same members, except for those members
        given new values by whichever keyword arguments are specified.

        Note that ``tzinfo=None`` can be specified to create a naive datetime from
        an aware datetime with no conversion of date and time members.
        """

    def astimezone(tz):
        """Return a datetime object with new tzinfo member tz, adjusting the
        date and time members so the result is the same UTC time as self, but
        in tz's local time.

        tz must be an instance of a tzinfo subclass, and its utcoffset() and
        dst() methods must not return None. self must be aware (self.tzinfo
        must not be None, and self.utcoffset() must not return None).

        If self.tzinfo is tz, self.astimezone(tz) is equal to self: no
        adjustment of date or time members is performed. Else the result is
        local time in time zone tz, representing the same UTC time as self:

            after astz = dt.astimezone(tz), astz - astz.utcoffset()

        will usually have the same date and time members as dt - dt.utcoffset().
        The discussion of class `datetime.tzinfo` explains the cases at Daylight Saving
        Time transition boundaries where this cannot be achieved (an issue only
        if tz models both standard and daylight time).

        If you merely want to attach a time zone object *tz* to a datetime *dt*
        without adjustment of date and time members, use ``dt.replace(tzinfo=tz)``.
        If you merely want to remove the time zone object from an aware
        datetime dt without conversion of date and time members, use
        ``dt.replace(tzinfo=None)``.

        Note that the default `tzinfo.fromutc` method can be overridden in a
        tzinfo subclass to effect the result returned by `astimezone`.
        """

    def utcoffset():
        """Return the timezone offset in minutes east of UTC (negative west of
        UTC)."""

    def dst():
        """Return 0 if DST is not in effect, or the DST offset (in minutes
        eastward) if DST is in effect.
        """

    def tzname():
        """Return the timezone name."""

    def timetuple():
        """Return a 9-element tuple of the form returned by `time.localtime`."""

    def utctimetuple():
        """Return UTC time tuple compatilble with `time.gmtime`."""

    def toordinal():
        """Return the proleptic Gregorian ordinal of the date.

        The same as self.date().toordinal().
        """

    def weekday():
        """Return the day of the week as an integer.

        Monday is 0 and Sunday is 6. The same as self.date().weekday().
        See also isoweekday().
        """

    def isoweekday():
        """Return the day of the week as an integer.

        Monday is 1 and Sunday is 7. The same as self.date().isoweekday.

        .. seealso:: `weekday`, `isocalendar`.
        """

    def isocalendar():
        """Return a 3-tuple, (ISO year, ISO week number, ISO weekday).

        The same as self.date().isocalendar().
        """

    def isoformat(sep='T'):
        """Return a string representing the date and time in ISO 8601 format.

        YYYY-MM-DDTHH:MM:SS.mmmmmm or YYYY-MM-DDTHH:MM:SS if microsecond is 0

        If `utcoffset` does not return None, a 6-character string is appended,
        giving the UTC offset in (signed) hours and minutes:

        YYYY-MM-DDTHH:MM:SS.mmmmmm+HH:MM or YYYY-MM-DDTHH:MM:SS+HH:MM
        if microsecond is 0.

        The optional argument sep (default 'T') is a one-character separator,
        placed between the date and time portions of the result.
        """

    def __str__():
        """For a datetime instance *d*, ``str(d)`` is equivalent to ``d.isoformat(' ')``.
        """

    def ctime():
        """Return a string representing the date and time.

        ``datetime(2002, 12, 4, 20, 30, 40).ctime() == 'Wed Dec 4 20:30:40 2002'``.
        ``d.ctime()`` is equivalent to ``time.ctime(time.mktime(d.timetuple()))`` on
        platforms where the native C ``ctime()`` function (which `time.ctime`
        invokes, but which `datetime.ctime` does not invoke) conforms to the
        C standard.
        """

    def strftime(format):
        """Return a string representing the date and time.

        This is controlled by an explicit format string.
        """


class ITimeClass(Interface):
    """This is the time class interface.

    This is symbolic; this module does **not** make
    `datetime.time` provide this interface.

    """

    min = Attribute("The earliest representable time")

    max = Attribute("The latest representable time")

    resolution = Attribute(
        "The smallest possible difference between non-equal time objects")


class ITime(ITimeClass):
    """Represent time with time zone.

    Implemented by `datetime.time`.

    Operators:

    __repr__, __str__
    __cmp__, __hash__
    """

    hour = Attribute("Hour in range(24)")

    minute = Attribute("Minute in range(60)")

    second = Attribute("Second in range(60)")

    microsecond = Attribute("Microsecond in range(1000000)")

    tzinfo = Attribute(
        """The object passed as the tzinfo argument to the time constructor
        or None if none was passed.""")

    def replace(hour, minute, second, microsecond, tzinfo):
        """Return a time with the same value.

        Except for those members given new values by whichever keyword
        arguments are specified. Note that tzinfo=None can be specified
        to create a naive time from an aware time, without conversion of the
        time members.
        """

    def isoformat():
        """Return a string representing the time in ISO 8601 format.

        That is HH:MM:SS.mmmmmm or, if self.microsecond is 0, HH:MM:SS
        If utcoffset() does not return None, a 6-character string is appended,
        giving the UTC offset in (signed) hours and minutes:
        HH:MM:SS.mmmmmm+HH:MM or, if self.microsecond is 0, HH:MM:SS+HH:MM
        """

    def __str__():
        """For a time t, str(t) is equivalent to t.isoformat()."""

    def strftime(format):
        """Return a string representing the time.

        This is controlled by an explicit format string.
        """

    def utcoffset():
        """Return the timezone offset in minutes east of UTC (negative west of
        UTC).

        If tzinfo is None, returns None, else returns
        self.tzinfo.utcoffset(None), and raises an exception if the latter
        doesn't return None or a timedelta object representing a whole number
        of minutes with magnitude less than one day.
        """

    def dst():
        """Return 0 if DST is not in effect, or the DST offset (in minutes
        eastward) if DST is in effect.

        If tzinfo is None, returns None, else returns self.tzinfo.dst(None),
        and raises an exception if the latter doesn't return None, or a
        timedelta object representing a whole number of minutes with
        magnitude less than one day.
        """

    def tzname():
        """Return the timezone name.

        If tzinfo is None, returns None, else returns self.tzinfo.tzname(None),
        or raises an exception if the latter doesn't return None or a string
        object.
        """


class ITZInfo(Interface):
    """Time zone info class.
    """

    def utcoffset(dt):
        """Return offset of local time from UTC, in minutes east of UTC.

        If local time is west of UTC, this should be negative.
        Note that this is intended to be the total offset from UTC;
        for example, if a tzinfo object represents both time zone and DST
        adjustments, utcoffset() should return their sum. If the UTC offset
        isn't known, return None. Else the value returned must be a timedelta
        object specifying a whole number of minutes in the range -1439 to 1439
        inclusive (1440 = 24*60; the magnitude of the offset must be less
        than one day).
        """

    def dst(dt):
        """Return the daylight saving time (DST) adjustment, in minutes east
        of UTC, or None if DST information isn't known.
        """

    def tzname(dt):
        """Return the time zone name corresponding to the datetime object as
        a string.
        """

    def fromutc(dt):
        """Return an equivalent datetime in self's local time."""


classImplements(timedelta, ITimeDelta)
classImplements(date, IDate)
classImplements(datetime, IDateTime)
classImplements(time, ITime)
classImplements(tzinfo, ITZInfo)

## directlyProvides(timedelta, ITimeDeltaClass)
## directlyProvides(date, IDateClass)
## directlyProvides(datetime, IDateTimeClass)
## directlyProvides(time, ITimeClass)
