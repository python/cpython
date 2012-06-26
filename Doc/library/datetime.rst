:mod:`datetime` --- Basic date and time types
=============================================

.. module:: datetime
   :synopsis: Basic date and time types.
.. moduleauthor:: Tim Peters <tim@zope.com>
.. sectionauthor:: Tim Peters <tim@zope.com>
.. sectionauthor:: A.M. Kuchling <amk@amk.ca>

.. XXX what order should the types be discussed in?

The :mod:`datetime` module supplies classes for manipulating dates and times in
both simple and complex ways.  While date and time arithmetic is supported, the
focus of the implementation is on efficient attribute extraction for output
formatting and manipulation. For related functionality, see also the
:mod:`time` and :mod:`calendar` modules.

There are two kinds of date and time objects: "naive" and "aware".

An aware object has sufficient knowledge of applicable algorithmic and
political time adjustments, such as time zone and daylight saving time
information, to locate itself relative to other aware objects.  An aware object
is used to represent a specific moment in time that is not open to
interpretation [#]_.

A naive object does not contain enough information to unambiguously locate
itself relative to other date/time objects.  Whether a naive object represents
Coordinated Universal Time (UTC), local time, or time in some other timezone is
purely up to the program, just like it is up to the program whether a
particular number represents metres, miles, or mass.  Naive objects are easy to
understand and to work with, at the cost of ignoring some aspects of reality.

For applications requiring aware objects, :class:`.datetime` and :class:`.time`
objects have an optional time zone information attribute, :attr:`tzinfo`, that
can be set to an instance of a subclass of the abstract :class:`tzinfo` class.
These :class:`tzinfo` objects capture information about the offset from UTC
time, the time zone name, and whether Daylight Saving Time is in effect.  Note
that only one concrete :class:`tzinfo` class, the :class:`timezone` class, is
supplied by the :mod:`datetime` module.  The :class:`timezone` class can
represent simple timezones with fixed offset from UTC, such as UTC itself or
North American EST and EDT timezones.  Supporting timezones at deeper levels of
detail is up to the application.  The rules for time adjustment across the
world are more political than rational, change frequently, and there is no
standard suitable for every application aside from UTC.

The :mod:`datetime` module exports the following constants:

.. data:: MINYEAR

   The smallest year number allowed in a :class:`date` or :class:`.datetime` object.
   :const:`MINYEAR` is ``1``.


.. data:: MAXYEAR

   The largest year number allowed in a :class:`date` or :class:`.datetime` object.
   :const:`MAXYEAR` is ``9999``.


.. seealso::

   Module :mod:`calendar`
      General calendar related functions.

   Module :mod:`time`
      Time access and conversions.


Available Types
---------------

.. class:: date
   :noindex:

   An idealized naive date, assuming the current Gregorian calendar always was, and
   always will be, in effect. Attributes: :attr:`year`, :attr:`month`, and
   :attr:`day`.


.. class:: time
   :noindex:

   An idealized time, independent of any particular day, assuming that every day
   has exactly 24\*60\*60 seconds (there is no notion of "leap seconds" here).
   Attributes: :attr:`hour`, :attr:`minute`, :attr:`second`, :attr:`microsecond`,
   and :attr:`tzinfo`.


.. class:: datetime
   :noindex:

   A combination of a date and a time. Attributes: :attr:`year`, :attr:`month`,
   :attr:`day`, :attr:`hour`, :attr:`minute`, :attr:`second`, :attr:`microsecond`,
   and :attr:`tzinfo`.


.. class:: timedelta
   :noindex:

   A duration expressing the difference between two :class:`date`, :class:`.time`,
   or :class:`.datetime` instances to microsecond resolution.


.. class:: tzinfo

   An abstract base class for time zone information objects.  These are used by the
   :class:`.datetime` and :class:`.time` classes to provide a customizable notion of
   time adjustment (for example, to account for time zone and/or daylight saving
   time).

.. class:: timezone

   A class that implements the :class:`tzinfo` abstract base class as a
   fixed offset from the UTC.

   .. versionadded:: 3.2


Objects of these types are immutable.

Objects of the :class:`date` type are always naive.

An object of type :class:`.time` or :class:`.datetime` may be naive or aware.
A :class:`.datetime` object *d* is aware if ``d.tzinfo`` is not ``None`` and
``d.tzinfo.utcoffset(d)`` does not return ``None``.  If ``d.tzinfo`` is
``None``, or if ``d.tzinfo`` is not ``None`` but ``d.tzinfo.utcoffset(d)``
returns ``None``, *d* is naive.  A :class:`.time` object *t* is aware
if ``t.tzinfo`` is not ``None`` and ``t.tzinfo.utcoffset(None)`` does not return
``None``.  Otherwise, *t* is naive.

The distinction between naive and aware doesn't apply to :class:`timedelta`
objects.

Subclass relationships::

   object
       timedelta
       tzinfo
           timezone
       time
       date
           datetime


.. _datetime-timedelta:

:class:`timedelta` Objects
--------------------------

A :class:`timedelta` object represents a duration, the difference between two
dates or times.

.. class:: timedelta(days=0, seconds=0, microseconds=0, milliseconds=0, minutes=0, hours=0, weeks=0)

   All arguments are optional and default to ``0``.  Arguments may be integers
   or floats, and may be positive or negative.

   Only *days*, *seconds* and *microseconds* are stored internally.  Arguments are
   converted to those units:

   * A millisecond is converted to 1000 microseconds.
   * A minute is converted to 60 seconds.
   * An hour is converted to 3600 seconds.
   * A week is converted to 7 days.

   and days, seconds and microseconds are then normalized so that the
   representation is unique, with

   * ``0 <= microseconds < 1000000``
   * ``0 <= seconds < 3600*24`` (the number of seconds in one day)
   * ``-999999999 <= days <= 999999999``

   If any argument is a float and there are fractional microseconds, the fractional
   microseconds left over from all arguments are combined and their sum is rounded
   to the nearest microsecond.  If no argument is a float, the conversion and
   normalization processes are exact (no information is lost).

   If the normalized value of days lies outside the indicated range,
   :exc:`OverflowError` is raised.

   Note that normalization of negative values may be surprising at first. For
   example,

      >>> from datetime import timedelta
      >>> d = timedelta(microseconds=-1)
      >>> (d.days, d.seconds, d.microseconds)
      (-1, 86399, 999999)


Class attributes are:

.. attribute:: timedelta.min

   The most negative :class:`timedelta` object, ``timedelta(-999999999)``.


.. attribute:: timedelta.max

   The most positive :class:`timedelta` object, ``timedelta(days=999999999,
   hours=23, minutes=59, seconds=59, microseconds=999999)``.


.. attribute:: timedelta.resolution

   The smallest possible difference between non-equal :class:`timedelta` objects,
   ``timedelta(microseconds=1)``.

Note that, because of normalization, ``timedelta.max`` > ``-timedelta.min``.
``-timedelta.max`` is not representable as a :class:`timedelta` object.

Instance attributes (read-only):

+------------------+--------------------------------------------+
| Attribute        | Value                                      |
+==================+============================================+
| ``days``         | Between -999999999 and 999999999 inclusive |
+------------------+--------------------------------------------+
| ``seconds``      | Between 0 and 86399 inclusive              |
+------------------+--------------------------------------------+
| ``microseconds`` | Between 0 and 999999 inclusive             |
+------------------+--------------------------------------------+

Supported operations:

.. XXX this table is too wide!

+--------------------------------+-----------------------------------------------+
| Operation                      | Result                                        |
+================================+===============================================+
| ``t1 = t2 + t3``               | Sum of *t2* and *t3*. Afterwards *t1*-*t2* == |
|                                | *t3* and *t1*-*t3* == *t2* are true. (1)      |
+--------------------------------+-----------------------------------------------+
| ``t1 = t2 - t3``               | Difference of *t2* and *t3*. Afterwards *t1*  |
|                                | == *t2* - *t3* and *t2* == *t1* + *t3* are    |
|                                | true. (1)                                     |
+--------------------------------+-----------------------------------------------+
| ``t1 = t2 * i or t1 = i * t2`` | Delta multiplied by an integer.               |
|                                | Afterwards *t1* // i == *t2* is true,         |
|                                | provided ``i != 0``.                          |
+--------------------------------+-----------------------------------------------+
|                                | In general, *t1* \* i == *t1* \* (i-1) + *t1* |
|                                | is true. (1)                                  |
+--------------------------------+-----------------------------------------------+
| ``t1 = t2 * f or t1 = f * t2`` | Delta multiplied by a float. The result is    |
|                                | rounded to the nearest multiple of            |
|                                | timedelta.resolution using round-half-to-even.|
+--------------------------------+-----------------------------------------------+
| ``f = t2 / t3``                | Division (3) of *t2* by *t3*.  Returns a      |
|                                | :class:`float` object.                        |
+--------------------------------+-----------------------------------------------+
| ``t1 = t2 / f or t1 = t2 / i`` | Delta divided by a float or an int. The result|
|                                | is rounded to the nearest multiple of         |
|                                | timedelta.resolution using round-half-to-even.|
+--------------------------------+-----------------------------------------------+
| ``t1 = t2 // i`` or            | The floor is computed and the remainder (if   |
| ``t1 = t2 // t3``              | any) is thrown away.  In the second case, an  |
|                                | integer is returned. (3)                      |
+--------------------------------+-----------------------------------------------+
| ``t1 = t2 % t3``               | The remainder is computed as a                |
|                                | :class:`timedelta` object. (3)                |
+--------------------------------+-----------------------------------------------+
| ``q, r = divmod(t1, t2)``      | Computes the quotient and the remainder:      |
|                                | ``q = t1 // t2`` (3) and ``r = t1 % t2``.     |
|                                | q is an integer and r is a :class:`timedelta` |
|                                | object.                                       |
+--------------------------------+-----------------------------------------------+
| ``+t1``                        | Returns a :class:`timedelta` object with the  |
|                                | same value. (2)                               |
+--------------------------------+-----------------------------------------------+
| ``-t1``                        | equivalent to :class:`timedelta`\             |
|                                | (-*t1.days*, -*t1.seconds*,                   |
|                                | -*t1.microseconds*), and to *t1*\* -1. (1)(4) |
+--------------------------------+-----------------------------------------------+
| ``abs(t)``                     | equivalent to +\ *t* when ``t.days >= 0``, and|
|                                | to -*t* when ``t.days < 0``. (2)              |
+--------------------------------+-----------------------------------------------+
| ``str(t)``                     | Returns a string in the form                  |
|                                | ``[D day[s], ][H]H:MM:SS[.UUUUUU]``, where D  |
|                                | is negative for negative ``t``. (5)           |
+--------------------------------+-----------------------------------------------+
| ``repr(t)``                    | Returns a string in the form                  |
|                                | ``datetime.timedelta(D[, S[, U]])``, where D  |
|                                | is negative for negative ``t``. (5)           |
+--------------------------------+-----------------------------------------------+

Notes:

(1)
   This is exact, but may overflow.

(2)
   This is exact, and cannot overflow.

(3)
   Division by 0 raises :exc:`ZeroDivisionError`.

(4)
   -*timedelta.max* is not representable as a :class:`timedelta` object.

(5)
  String representations of :class:`timedelta` objects are normalized
  similarly to their internal representation.  This leads to somewhat
  unusual results for negative timedeltas.  For example:

  >>> timedelta(hours=-5)
  datetime.timedelta(-1, 68400)
  >>> print(_)
  -1 day, 19:00:00

In addition to the operations listed above :class:`timedelta` objects support
certain additions and subtractions with :class:`date` and :class:`.datetime`
objects (see below).

.. versionchanged:: 3.2
   Floor division and true division of a :class:`timedelta` object by another
   :class:`timedelta` object are now supported, as are remainder operations and
   the :func:`divmod` function.  True division and multiplication of a
   :class:`timedelta` object by a :class:`float` object are now supported.


Comparisons of :class:`timedelta` objects are supported with the
:class:`timedelta` object representing the smaller duration considered to be the
smaller timedelta. In order to stop mixed-type comparisons from falling back to
the default comparison by object address, when a :class:`timedelta` object is
compared to an object of a different type, :exc:`TypeError` is raised unless the
comparison is ``==`` or ``!=``.  The latter cases return :const:`False` or
:const:`True`, respectively.

:class:`timedelta` objects are :term:`hashable` (usable as dictionary keys), support
efficient pickling, and in Boolean contexts, a :class:`timedelta` object is
considered to be true if and only if it isn't equal to ``timedelta(0)``.

Instance methods:

.. method:: timedelta.total_seconds()

   Return the total number of seconds contained in the duration. Equivalent to
   ``td / timedelta(seconds=1)``.

   Note that for very large time intervals (greater than 270 years on
   most platforms) this method will lose microsecond accuracy.

   .. versionadded:: 3.2


Example usage:

    >>> from datetime import timedelta
    >>> year = timedelta(days=365)
    >>> another_year = timedelta(weeks=40, days=84, hours=23,
    ...                          minutes=50, seconds=600)  # adds up to 365 days
    >>> year.total_seconds()
    31536000.0
    >>> year == another_year
    True
    >>> ten_years = 10 * year
    >>> ten_years, ten_years.days // 365
    (datetime.timedelta(3650), 10)
    >>> nine_years = ten_years - year
    >>> nine_years, nine_years.days // 365
    (datetime.timedelta(3285), 9)
    >>> three_years = nine_years // 3;
    >>> three_years, three_years.days // 365
    (datetime.timedelta(1095), 3)
    >>> abs(three_years - ten_years) == 2 * three_years + year
    True


.. _datetime-date:

:class:`date` Objects
---------------------

A :class:`date` object represents a date (year, month and day) in an idealized
calendar, the current Gregorian calendar indefinitely extended in both
directions.  January 1 of year 1 is called day number 1, January 2 of year 1 is
called day number 2, and so on.  This matches the definition of the "proleptic
Gregorian" calendar in Dershowitz and Reingold's book Calendrical Calculations,
where it's the base calendar for all computations.  See the book for algorithms
for converting between proleptic Gregorian ordinals and many other calendar
systems.


.. class:: date(year, month, day)

   All arguments are required.  Arguments may be integers, in the following
   ranges:

   * ``MINYEAR <= year <= MAXYEAR``
   * ``1 <= month <= 12``
   * ``1 <= day <= number of days in the given month and year``

   If an argument outside those ranges is given, :exc:`ValueError` is raised.


Other constructors, all class methods:

.. classmethod:: date.today()

   Return the current local date.  This is equivalent to
   ``date.fromtimestamp(time.time())``.


.. classmethod:: date.fromtimestamp(timestamp)

   Return the local date corresponding to the POSIX timestamp, such as is returned
   by :func:`time.time`.  This may raise :exc:`ValueError`, if the timestamp is out
   of the range of values supported by the platform C :c:func:`localtime` function.
   It's common for this to be restricted to years from 1970 through 2038.  Note
   that on non-POSIX systems that include leap seconds in their notion of a
   timestamp, leap seconds are ignored by :meth:`fromtimestamp`.


.. classmethod:: date.fromordinal(ordinal)

   Return the date corresponding to the proleptic Gregorian ordinal, where January
   1 of year 1 has ordinal 1.  :exc:`ValueError` is raised unless ``1 <= ordinal <=
   date.max.toordinal()``. For any date *d*, ``date.fromordinal(d.toordinal()) ==
   d``.


Class attributes:

.. attribute:: date.min

   The earliest representable date, ``date(MINYEAR, 1, 1)``.


.. attribute:: date.max

   The latest representable date, ``date(MAXYEAR, 12, 31)``.


.. attribute:: date.resolution

   The smallest possible difference between non-equal date objects,
   ``timedelta(days=1)``.


Instance attributes (read-only):

.. attribute:: date.year

   Between :const:`MINYEAR` and :const:`MAXYEAR` inclusive.


.. attribute:: date.month

   Between 1 and 12 inclusive.


.. attribute:: date.day

   Between 1 and the number of days in the given month of the given year.


Supported operations:

+-------------------------------+----------------------------------------------+
| Operation                     | Result                                       |
+===============================+==============================================+
| ``date2 = date1 + timedelta`` | *date2* is ``timedelta.days`` days removed   |
|                               | from *date1*.  (1)                           |
+-------------------------------+----------------------------------------------+
| ``date2 = date1 - timedelta`` | Computes *date2* such that ``date2 +         |
|                               | timedelta == date1``. (2)                    |
+-------------------------------+----------------------------------------------+
| ``timedelta = date1 - date2`` | \(3)                                         |
+-------------------------------+----------------------------------------------+
| ``date1 < date2``             | *date1* is considered less than *date2* when |
|                               | *date1* precedes *date2* in time. (4)        |
+-------------------------------+----------------------------------------------+

Notes:

(1)
   *date2* is moved forward in time if ``timedelta.days > 0``, or backward if
   ``timedelta.days < 0``.  Afterward ``date2 - date1 == timedelta.days``.
   ``timedelta.seconds`` and ``timedelta.microseconds`` are ignored.
   :exc:`OverflowError` is raised if ``date2.year`` would be smaller than
   :const:`MINYEAR` or larger than :const:`MAXYEAR`.

(2)
   This isn't quite equivalent to date1 + (-timedelta), because -timedelta in
   isolation can overflow in cases where date1 - timedelta does not.
   ``timedelta.seconds`` and ``timedelta.microseconds`` are ignored.

(3)
   This is exact, and cannot overflow.  timedelta.seconds and
   timedelta.microseconds are 0, and date2 + timedelta == date1 after.

(4)
   In other words, ``date1 < date2`` if and only if ``date1.toordinal() <
   date2.toordinal()``. In order to stop comparison from falling back to the
   default scheme of comparing object addresses, date comparison normally raises
   :exc:`TypeError` if the other comparand isn't also a :class:`date` object.
   However, ``NotImplemented`` is returned instead if the other comparand has a
   :meth:`timetuple` attribute.  This hook gives other kinds of date objects a
   chance at implementing mixed-type comparison. If not, when a :class:`date`
   object is compared to an object of a different type, :exc:`TypeError` is raised
   unless the comparison is ``==`` or ``!=``.  The latter cases return
   :const:`False` or :const:`True`, respectively.

Dates can be used as dictionary keys. In Boolean contexts, all :class:`date`
objects are considered to be true.

Instance methods:

.. method:: date.replace(year, month, day)

   Return a date with the same value, except for those parameters given new
   values by whichever keyword arguments are specified.  For example, if ``d ==
   date(2002, 12, 31)``, then ``d.replace(day=26) == date(2002, 12, 26)``.


.. method:: date.timetuple()

   Return a :class:`time.struct_time` such as returned by :func:`time.localtime`.
   The hours, minutes and seconds are 0, and the DST flag is -1. ``d.timetuple()``
   is equivalent to ``time.struct_time((d.year, d.month, d.day, 0, 0, 0,
   d.weekday(), yday, -1))``, where ``yday = d.toordinal() - date(d.year, 1,
   1).toordinal() + 1`` is the day number within the current year starting with
   ``1`` for January 1st.


.. method:: date.toordinal()

   Return the proleptic Gregorian ordinal of the date, where January 1 of year 1
   has ordinal 1.  For any :class:`date` object *d*,
   ``date.fromordinal(d.toordinal()) == d``.


.. method:: date.weekday()

   Return the day of the week as an integer, where Monday is 0 and Sunday is 6.
   For example, ``date(2002, 12, 4).weekday() == 2``, a Wednesday. See also
   :meth:`isoweekday`.


.. method:: date.isoweekday()

   Return the day of the week as an integer, where Monday is 1 and Sunday is 7.
   For example, ``date(2002, 12, 4).isoweekday() == 3``, a Wednesday. See also
   :meth:`weekday`, :meth:`isocalendar`.


.. method:: date.isocalendar()

   Return a 3-tuple, (ISO year, ISO week number, ISO weekday).

   The ISO calendar is a widely used variant of the Gregorian calendar. See
   http://www.phys.uu.nl/~vgent/calendar/isocalendar.htm for a good
   explanation.

   The ISO year consists of 52 or 53 full weeks, and where a week starts on a
   Monday and ends on a Sunday.  The first week of an ISO year is the first
   (Gregorian) calendar week of a year containing a Thursday. This is called week
   number 1, and the ISO year of that Thursday is the same as its Gregorian year.

   For example, 2004 begins on a Thursday, so the first week of ISO year 2004
   begins on Monday, 29 Dec 2003 and ends on Sunday, 4 Jan 2004, so that
   ``date(2003, 12, 29).isocalendar() == (2004, 1, 1)`` and ``date(2004, 1,
   4).isocalendar() == (2004, 1, 7)``.


.. method:: date.isoformat()

   Return a string representing the date in ISO 8601 format, 'YYYY-MM-DD'.  For
   example, ``date(2002, 12, 4).isoformat() == '2002-12-04'``.


.. method:: date.__str__()

   For a date *d*, ``str(d)`` is equivalent to ``d.isoformat()``.


.. method:: date.ctime()

   Return a string representing the date, for example ``date(2002, 12,
   4).ctime() == 'Wed Dec 4 00:00:00 2002'``. ``d.ctime()`` is equivalent to
   ``time.ctime(time.mktime(d.timetuple()))`` on platforms where the native C
   :c:func:`ctime` function (which :func:`time.ctime` invokes, but which
   :meth:`date.ctime` does not invoke) conforms to the C standard.


.. method:: date.strftime(format)

   Return a string representing the date, controlled by an explicit format string.
   Format codes referring to hours, minutes or seconds will see 0 values. See
   section :ref:`strftime-strptime-behavior`.


Example of counting days to an event::

    >>> import time
    >>> from datetime import date
    >>> today = date.today()
    >>> today
    datetime.date(2007, 12, 5)
    >>> today == date.fromtimestamp(time.time())
    True
    >>> my_birthday = date(today.year, 6, 24)
    >>> if my_birthday < today:
    ...     my_birthday = my_birthday.replace(year=today.year + 1)
    >>> my_birthday
    datetime.date(2008, 6, 24)
    >>> time_to_birthday = abs(my_birthday - today)
    >>> time_to_birthday.days
    202

Example of working with :class:`date`:

.. doctest::

    >>> from datetime import date
    >>> d = date.fromordinal(730920) # 730920th day after 1. 1. 0001
    >>> d
    datetime.date(2002, 3, 11)
    >>> t = d.timetuple()
    >>> for i in t:     # doctest: +SKIP
    ...     print(i)
    2002                # year
    3                   # month
    11                  # day
    0
    0
    0
    0                   # weekday (0 = Monday)
    70                  # 70th day in the year
    -1
    >>> ic = d.isocalendar()
    >>> for i in ic:    # doctest: +SKIP
    ...     print(i)
    2002                # ISO year
    11                  # ISO week number
    1                   # ISO day number ( 1 = Monday )
    >>> d.isoformat()
    '2002-03-11'
    >>> d.strftime("%d/%m/%y")
    '11/03/02'
    >>> d.strftime("%A %d. %B %Y")
    'Monday 11. March 2002'


.. _datetime-datetime:

:class:`datetime` Objects
-------------------------

A :class:`.datetime` object is a single object containing all the information
from a :class:`date` object and a :class:`.time` object.  Like a :class:`date`
object, :class:`.datetime` assumes the current Gregorian calendar extended in
both directions; like a time object, :class:`.datetime` assumes there are exactly
3600\*24 seconds in every day.

Constructor:

.. class:: datetime(year, month, day, hour=0, minute=0, second=0, microsecond=0, tzinfo=None)

   The year, month and day arguments are required.  *tzinfo* may be ``None``, or an
   instance of a :class:`tzinfo` subclass.  The remaining arguments may be integers,
   in the following ranges:

   * ``MINYEAR <= year <= MAXYEAR``
   * ``1 <= month <= 12``
   * ``1 <= day <= number of days in the given month and year``
   * ``0 <= hour < 24``
   * ``0 <= minute < 60``
   * ``0 <= second < 60``
   * ``0 <= microsecond < 1000000``

   If an argument outside those ranges is given, :exc:`ValueError` is raised.

Other constructors, all class methods:

.. classmethod:: datetime.today()

   Return the current local datetime, with :attr:`tzinfo` ``None``. This is
   equivalent to ``datetime.fromtimestamp(time.time())``. See also :meth:`now`,
   :meth:`fromtimestamp`.


.. classmethod:: datetime.now(tz=None)

   Return the current local date and time.  If optional argument *tz* is ``None``
   or not specified, this is like :meth:`today`, but, if possible, supplies more
   precision than can be gotten from going through a :func:`time.time` timestamp
   (for example, this may be possible on platforms supplying the C
   :c:func:`gettimeofday` function).

   Else *tz* must be an instance of a class :class:`tzinfo` subclass, and the
   current date and time are converted to *tz*'s time zone.  In this case the
   result is equivalent to ``tz.fromutc(datetime.utcnow().replace(tzinfo=tz))``.
   See also :meth:`today`, :meth:`utcnow`.


.. classmethod:: datetime.utcnow()

   Return the current UTC date and time, with :attr:`tzinfo` ``None``. This is like
   :meth:`now`, but returns the current UTC date and time, as a naive
   :class:`.datetime` object.  An aware current UTC datetime can be obtained by
   calling ``datetime.now(timezone.utc)``.  See also :meth:`now`.

.. classmethod:: datetime.fromtimestamp(timestamp, tz=None)

   Return the local date and time corresponding to the POSIX timestamp, such as is
   returned by :func:`time.time`. If optional argument *tz* is ``None`` or not
   specified, the timestamp is converted to the platform's local date and time, and
   the returned :class:`.datetime` object is naive.

   Else *tz* must be an instance of a class :class:`tzinfo` subclass, and the
   timestamp is converted to *tz*'s time zone.  In this case the result is
   equivalent to
   ``tz.fromutc(datetime.utcfromtimestamp(timestamp).replace(tzinfo=tz))``.

   :meth:`fromtimestamp` may raise :exc:`ValueError`, if the timestamp is out of
   the range of values supported by the platform C :c:func:`localtime` or
   :c:func:`gmtime` functions.  It's common for this to be restricted to years in
   1970 through 2038. Note that on non-POSIX systems that include leap seconds in
   their notion of a timestamp, leap seconds are ignored by :meth:`fromtimestamp`,
   and then it's possible to have two timestamps differing by a second that yield
   identical :class:`.datetime` objects. See also :meth:`utcfromtimestamp`.


.. classmethod:: datetime.utcfromtimestamp(timestamp)

   Return the UTC :class:`.datetime` corresponding to the POSIX timestamp, with
   :attr:`tzinfo` ``None``. This may raise :exc:`ValueError`, if the timestamp is
   out of the range of values supported by the platform C :c:func:`gmtime` function.
   It's common for this to be restricted to years in 1970 through 2038. See also
   :meth:`fromtimestamp`.


.. classmethod:: datetime.fromordinal(ordinal)

   Return the :class:`.datetime` corresponding to the proleptic Gregorian ordinal,
   where January 1 of year 1 has ordinal 1. :exc:`ValueError` is raised unless ``1
   <= ordinal <= datetime.max.toordinal()``.  The hour, minute, second and
   microsecond of the result are all 0, and :attr:`tzinfo` is ``None``.


.. classmethod:: datetime.combine(date, time)

   Return a new :class:`.datetime` object whose date components are equal to the
   given :class:`date` object's, and whose time components and :attr:`tzinfo`
   attributes are equal to the given :class:`.time` object's. For any
   :class:`.datetime` object *d*,
   ``d == datetime.combine(d.date(), d.timetz())``.  If date is a
   :class:`.datetime` object, its time components and :attr:`tzinfo` attributes
   are ignored.


.. classmethod:: datetime.strptime(date_string, format)

   Return a :class:`.datetime` corresponding to *date_string*, parsed according to
   *format*.  This is equivalent to ``datetime(*(time.strptime(date_string,
   format)[0:6]))``. :exc:`ValueError` is raised if the date_string and format
   can't be parsed by :func:`time.strptime` or if it returns a value which isn't a
   time tuple. See section :ref:`strftime-strptime-behavior`.



Class attributes:

.. attribute:: datetime.min

   The earliest representable :class:`.datetime`, ``datetime(MINYEAR, 1, 1,
   tzinfo=None)``.


.. attribute:: datetime.max

   The latest representable :class:`.datetime`, ``datetime(MAXYEAR, 12, 31, 23, 59,
   59, 999999, tzinfo=None)``.


.. attribute:: datetime.resolution

   The smallest possible difference between non-equal :class:`.datetime` objects,
   ``timedelta(microseconds=1)``.


Instance attributes (read-only):

.. attribute:: datetime.year

   Between :const:`MINYEAR` and :const:`MAXYEAR` inclusive.


.. attribute:: datetime.month

   Between 1 and 12 inclusive.


.. attribute:: datetime.day

   Between 1 and the number of days in the given month of the given year.


.. attribute:: datetime.hour

   In ``range(24)``.


.. attribute:: datetime.minute

   In ``range(60)``.


.. attribute:: datetime.second

   In ``range(60)``.


.. attribute:: datetime.microsecond

   In ``range(1000000)``.


.. attribute:: datetime.tzinfo

   The object passed as the *tzinfo* argument to the :class:`.datetime` constructor,
   or ``None`` if none was passed.


Supported operations:

+---------------------------------------+--------------------------------+
| Operation                             | Result                         |
+=======================================+================================+
| ``datetime2 = datetime1 + timedelta`` | \(1)                           |
+---------------------------------------+--------------------------------+
| ``datetime2 = datetime1 - timedelta`` | \(2)                           |
+---------------------------------------+--------------------------------+
| ``timedelta = datetime1 - datetime2`` | \(3)                           |
+---------------------------------------+--------------------------------+
| ``datetime1 < datetime2``             | Compares :class:`.datetime` to |
|                                       | :class:`.datetime`. (4)        |
+---------------------------------------+--------------------------------+

(1)
   datetime2 is a duration of timedelta removed from datetime1, moving forward in
   time if ``timedelta.days`` > 0, or backward if ``timedelta.days`` < 0.  The
   result has the same :attr:`tzinfo` attribute as the input datetime, and
   datetime2 - datetime1 == timedelta after. :exc:`OverflowError` is raised if
   datetime2.year would be smaller than :const:`MINYEAR` or larger than
   :const:`MAXYEAR`. Note that no time zone adjustments are done even if the
   input is an aware object.

(2)
   Computes the datetime2 such that datetime2 + timedelta == datetime1. As for
   addition, the result has the same :attr:`tzinfo` attribute as the input
   datetime, and no time zone adjustments are done even if the input is aware.
   This isn't quite equivalent to datetime1 + (-timedelta), because -timedelta
   in isolation can overflow in cases where datetime1 - timedelta does not.

(3)
   Subtraction of a :class:`.datetime` from a :class:`.datetime` is defined only if
   both operands are naive, or if both are aware.  If one is aware and the other is
   naive, :exc:`TypeError` is raised.

   If both are naive, or both are aware and have the same :attr:`tzinfo` attribute,
   the :attr:`tzinfo` attributes are ignored, and the result is a :class:`timedelta`
   object *t* such that ``datetime2 + t == datetime1``.  No time zone adjustments
   are done in this case.

   If both are aware and have different :attr:`tzinfo` attributes, ``a-b`` acts
   as if *a* and *b* were first converted to naive UTC datetimes first.  The
   result is ``(a.replace(tzinfo=None) - a.utcoffset()) - (b.replace(tzinfo=None)
   - b.utcoffset())`` except that the implementation never overflows.

(4)
   *datetime1* is considered less than *datetime2* when *datetime1* precedes
   *datetime2* in time.

   If one comparand is naive and the other is aware, :exc:`TypeError` is raised.
   If both comparands are aware, and have the same :attr:`tzinfo` attribute, the
   common :attr:`tzinfo` attribute is ignored and the base datetimes are
   compared.  If both comparands are aware and have different :attr:`tzinfo`
   attributes, the comparands are first adjusted by subtracting their UTC
   offsets (obtained from ``self.utcoffset()``).

   .. note::

      In order to stop comparison from falling back to the default scheme of comparing
      object addresses, datetime comparison normally raises :exc:`TypeError` if the
      other comparand isn't also a :class:`.datetime` object.  However,
      ``NotImplemented`` is returned instead if the other comparand has a
      :meth:`timetuple` attribute.  This hook gives other kinds of date objects a
      chance at implementing mixed-type comparison.  If not, when a :class:`.datetime`
      object is compared to an object of a different type, :exc:`TypeError` is raised
      unless the comparison is ``==`` or ``!=``.  The latter cases return
      :const:`False` or :const:`True`, respectively.

:class:`.datetime` objects can be used as dictionary keys. In Boolean contexts,
all :class:`.datetime` objects are considered to be true.

Instance methods:

.. method:: datetime.date()

   Return :class:`date` object with same year, month and day.


.. method:: datetime.time()

   Return :class:`.time` object with same hour, minute, second and microsecond.
   :attr:`tzinfo` is ``None``.  See also method :meth:`timetz`.


.. method:: datetime.timetz()

   Return :class:`.time` object with same hour, minute, second, microsecond, and
   tzinfo attributes.  See also method :meth:`time`.


.. method:: datetime.replace([year[, month[, day[, hour[, minute[, second[, microsecond[, tzinfo]]]]]]]])

   Return a datetime with the same attributes, except for those attributes given
   new values by whichever keyword arguments are specified.  Note that
   ``tzinfo=None`` can be specified to create a naive datetime from an aware
   datetime with no conversion of date and time data.


.. method:: datetime.astimezone(tz)

   Return a :class:`.datetime` object with new :attr:`tzinfo` attribute *tz*,
   adjusting the date and time data so the result is the same UTC time as
   *self*, but in *tz*'s local time.

   *tz* must be an instance of a :class:`tzinfo` subclass, and its
   :meth:`utcoffset` and :meth:`dst` methods must not return ``None``.  *self* must
   be aware (``self.tzinfo`` must not be ``None``, and ``self.utcoffset()`` must
   not return ``None``).

   If ``self.tzinfo`` is *tz*, ``self.astimezone(tz)`` is equal to *self*:  no
   adjustment of date or time data is performed. Else the result is local
   time in time zone *tz*, representing the same UTC time as *self*:  after
   ``astz = dt.astimezone(tz)``, ``astz - astz.utcoffset()`` will usually have
   the same date and time data as ``dt - dt.utcoffset()``. The discussion
   of class :class:`tzinfo` explains the cases at Daylight Saving Time transition
   boundaries where this cannot be achieved (an issue only if *tz* models both
   standard and daylight time).

   If you merely want to attach a time zone object *tz* to a datetime *dt* without
   adjustment of date and time data, use ``dt.replace(tzinfo=tz)``.  If you
   merely want to remove the time zone object from an aware datetime *dt* without
   conversion of date and time data, use ``dt.replace(tzinfo=None)``.

   Note that the default :meth:`tzinfo.fromutc` method can be overridden in a
   :class:`tzinfo` subclass to affect the result returned by :meth:`astimezone`.
   Ignoring error cases, :meth:`astimezone` acts like::

      def astimezone(self, tz):
          if self.tzinfo is tz:
              return self
          # Convert self to UTC, and attach the new time zone object.
          utc = (self - self.utcoffset()).replace(tzinfo=tz)
          # Convert from UTC to tz's local time.
          return tz.fromutc(utc)


.. method:: datetime.utcoffset()

   If :attr:`tzinfo` is ``None``, returns ``None``, else returns
   ``self.tzinfo.utcoffset(self)``, and raises an exception if the latter doesn't
   return ``None``, or a :class:`timedelta` object representing a whole number of
   minutes with magnitude less than one day.


.. method:: datetime.dst()

   If :attr:`tzinfo` is ``None``, returns ``None``, else returns
   ``self.tzinfo.dst(self)``, and raises an exception if the latter doesn't return
   ``None``, or a :class:`timedelta` object representing a whole number of minutes
   with magnitude less than one day.


.. method:: datetime.tzname()

   If :attr:`tzinfo` is ``None``, returns ``None``, else returns
   ``self.tzinfo.tzname(self)``, raises an exception if the latter doesn't return
   ``None`` or a string object,


.. method:: datetime.timetuple()

   Return a :class:`time.struct_time` such as returned by :func:`time.localtime`.
   ``d.timetuple()`` is equivalent to ``time.struct_time((d.year, d.month, d.day,
   d.hour, d.minute, d.second, d.weekday(), yday, dst))``, where ``yday =
   d.toordinal() - date(d.year, 1, 1).toordinal() + 1`` is the day number within
   the current year starting with ``1`` for January 1st. The :attr:`tm_isdst` flag
   of the result is set according to the :meth:`dst` method: :attr:`tzinfo` is
   ``None`` or :meth:`dst` returns ``None``, :attr:`tm_isdst` is set to ``-1``;
   else if :meth:`dst` returns a non-zero value, :attr:`tm_isdst` is set to ``1``;
   else :attr:`tm_isdst` is set to ``0``.


.. method:: datetime.utctimetuple()

   If :class:`.datetime` instance *d* is naive, this is the same as
   ``d.timetuple()`` except that :attr:`tm_isdst` is forced to 0 regardless of what
   ``d.dst()`` returns.  DST is never in effect for a UTC time.

   If *d* is aware, *d* is normalized to UTC time, by subtracting
   ``d.utcoffset()``, and a :class:`time.struct_time` for the
   normalized time is returned.  :attr:`tm_isdst` is forced to 0. Note
   that an :exc:`OverflowError` may be raised if *d*.year was
   ``MINYEAR`` or ``MAXYEAR`` and UTC adjustment spills over a year
   boundary.


.. method:: datetime.toordinal()

   Return the proleptic Gregorian ordinal of the date.  The same as
   ``self.date().toordinal()``.


.. method:: datetime.weekday()

   Return the day of the week as an integer, where Monday is 0 and Sunday is 6.
   The same as ``self.date().weekday()``. See also :meth:`isoweekday`.


.. method:: datetime.isoweekday()

   Return the day of the week as an integer, where Monday is 1 and Sunday is 7.
   The same as ``self.date().isoweekday()``. See also :meth:`weekday`,
   :meth:`isocalendar`.


.. method:: datetime.isocalendar()

   Return a 3-tuple, (ISO year, ISO week number, ISO weekday).  The same as
   ``self.date().isocalendar()``.


.. method:: datetime.isoformat(sep='T')

   Return a string representing the date and time in ISO 8601 format,
   YYYY-MM-DDTHH:MM:SS.mmmmmm or, if :attr:`microsecond` is 0,
   YYYY-MM-DDTHH:MM:SS

   If :meth:`utcoffset` does not return ``None``, a 6-character string is
   appended, giving the UTC offset in (signed) hours and minutes:
   YYYY-MM-DDTHH:MM:SS.mmmmmm+HH:MM or, if :attr:`microsecond` is 0
   YYYY-MM-DDTHH:MM:SS+HH:MM

   The optional argument *sep* (default ``'T'``) is a one-character separator,
   placed between the date and time portions of the result.  For example,

      >>> from datetime import tzinfo, timedelta, datetime
      >>> class TZ(tzinfo):
      ...     def utcoffset(self, dt): return timedelta(minutes=-399)
      ...
      >>> datetime(2002, 12, 25, tzinfo=TZ()).isoformat(' ')
      '2002-12-25 00:00:00-06:39'


.. method:: datetime.__str__()

   For a :class:`.datetime` instance *d*, ``str(d)`` is equivalent to
   ``d.isoformat(' ')``.


.. method:: datetime.ctime()

   Return a string representing the date and time, for example ``datetime(2002, 12,
   4, 20, 30, 40).ctime() == 'Wed Dec  4 20:30:40 2002'``. ``d.ctime()`` is
   equivalent to ``time.ctime(time.mktime(d.timetuple()))`` on platforms where the
   native C :c:func:`ctime` function (which :func:`time.ctime` invokes, but which
   :meth:`datetime.ctime` does not invoke) conforms to the C standard.


.. method:: datetime.strftime(format)

   Return a string representing the date and time, controlled by an explicit format
   string.  See section :ref:`strftime-strptime-behavior`.


Examples of working with datetime objects:

.. doctest::

    >>> from datetime import datetime, date, time
    >>> # Using datetime.combine()
    >>> d = date(2005, 7, 14)
    >>> t = time(12, 30)
    >>> datetime.combine(d, t)
    datetime.datetime(2005, 7, 14, 12, 30)
    >>> # Using datetime.now() or datetime.utcnow()
    >>> datetime.now()   # doctest: +SKIP
    datetime.datetime(2007, 12, 6, 16, 29, 43, 79043)   # GMT +1
    >>> datetime.utcnow()   # doctest: +SKIP
    datetime.datetime(2007, 12, 6, 15, 29, 43, 79060)
    >>> # Using datetime.strptime()
    >>> dt = datetime.strptime("21/11/06 16:30", "%d/%m/%y %H:%M")
    >>> dt
    datetime.datetime(2006, 11, 21, 16, 30)
    >>> # Using datetime.timetuple() to get tuple of all attributes
    >>> tt = dt.timetuple()
    >>> for it in tt:   # doctest: +SKIP
    ...     print(it)
    ...
    2006    # year
    11      # month
    21      # day
    16      # hour
    30      # minute
    0       # second
    1       # weekday (0 = Monday)
    325     # number of days since 1st January
    -1      # dst - method tzinfo.dst() returned None
    >>> # Date in ISO format
    >>> ic = dt.isocalendar()
    >>> for it in ic:   # doctest: +SKIP
    ...     print(it)
    ...
    2006    # ISO year
    47      # ISO week
    2       # ISO weekday
    >>> # Formatting datetime
    >>> dt.strftime("%A, %d. %B %Y %I:%M%p")
    'Tuesday, 21. November 2006 04:30PM'

Using datetime with tzinfo:

    >>> from datetime import timedelta, datetime, tzinfo
    >>> class GMT1(tzinfo):
    ...     def utcoffset(self, dt):
    ...         return timedelta(hours=1) + self.dst(dt)
    ...     def dst(self, dt):
    ...         # DST starts last Sunday in March
    ...         d = datetime(dt.year, 4, 1)   # ends last Sunday in October
    ...         self.dston = d - timedelta(days=d.weekday() + 1)
    ...         d = datetime(dt.year, 11, 1)
    ...         self.dstoff = d - timedelta(days=d.weekday() + 1)
    ...         if self.dston <=  dt.replace(tzinfo=None) < self.dstoff:
    ...             return timedelta(hours=1)
    ...         else:
    ...             return timedelta(0)
    ...     def tzname(self,dt):
    ...          return "GMT +1"
    ...
    >>> class GMT2(tzinfo):
    ...     def utcoffset(self, dt):
    ...         return timedelta(hours=2) + self.dst(dt)
    ...     def dst(self, dt):
    ...         d = datetime(dt.year, 4, 1)
    ...         self.dston = d - timedelta(days=d.weekday() + 1)
    ...         d = datetime(dt.year, 11, 1)
    ...         self.dstoff = d - timedelta(days=d.weekday() + 1)
    ...         if self.dston <=  dt.replace(tzinfo=None) < self.dstoff:
    ...             return timedelta(hours=1)
    ...         else:
    ...             return timedelta(0)
    ...     def tzname(self,dt):
    ...         return "GMT +2"
    ...
    >>> gmt1 = GMT1()
    >>> # Daylight Saving Time
    >>> dt1 = datetime(2006, 11, 21, 16, 30, tzinfo=gmt1)
    >>> dt1.dst()
    datetime.timedelta(0)
    >>> dt1.utcoffset()
    datetime.timedelta(0, 3600)
    >>> dt2 = datetime(2006, 6, 14, 13, 0, tzinfo=gmt1)
    >>> dt2.dst()
    datetime.timedelta(0, 3600)
    >>> dt2.utcoffset()
    datetime.timedelta(0, 7200)
    >>> # Convert datetime to another time zone
    >>> dt3 = dt2.astimezone(GMT2())
    >>> dt3     # doctest: +ELLIPSIS
    datetime.datetime(2006, 6, 14, 14, 0, tzinfo=<GMT2 object at 0x...>)
    >>> dt2     # doctest: +ELLIPSIS
    datetime.datetime(2006, 6, 14, 13, 0, tzinfo=<GMT1 object at 0x...>)
    >>> dt2.utctimetuple() == dt3.utctimetuple()
    True



.. _datetime-time:

:class:`time` Objects
---------------------

A time object represents a (local) time of day, independent of any particular
day, and subject to adjustment via a :class:`tzinfo` object.

.. class:: time(hour=0, minute=0, second=0, microsecond=0, tzinfo=None)

   All arguments are optional.  *tzinfo* may be ``None``, or an instance of a
   :class:`tzinfo` subclass.  The remaining arguments may be integers, in the
   following ranges:

   * ``0 <= hour < 24``
   * ``0 <= minute < 60``
   * ``0 <= second < 60``
   * ``0 <= microsecond < 1000000``.

   If an argument outside those ranges is given, :exc:`ValueError` is raised.  All
   default to ``0`` except *tzinfo*, which defaults to :const:`None`.

Class attributes:


.. attribute:: time.min

   The earliest representable :class:`.time`, ``time(0, 0, 0, 0)``.


.. attribute:: time.max

   The latest representable :class:`.time`, ``time(23, 59, 59, 999999)``.


.. attribute:: time.resolution

   The smallest possible difference between non-equal :class:`.time` objects,
   ``timedelta(microseconds=1)``, although note that arithmetic on
   :class:`.time` objects is not supported.


Instance attributes (read-only):

.. attribute:: time.hour

   In ``range(24)``.


.. attribute:: time.minute

   In ``range(60)``.


.. attribute:: time.second

   In ``range(60)``.


.. attribute:: time.microsecond

   In ``range(1000000)``.


.. attribute:: time.tzinfo

   The object passed as the tzinfo argument to the :class:`.time` constructor, or
   ``None`` if none was passed.


Supported operations:

* comparison of :class:`.time` to :class:`.time`, where *a* is considered less
  than *b* when *a* precedes *b* in time.  If one comparand is naive and the other
  is aware, :exc:`TypeError` is raised.  If both comparands are aware, and have
  the same :attr:`tzinfo` attribute, the common :attr:`tzinfo` attribute is
  ignored and the base times are compared.  If both comparands are aware and
  have different :attr:`tzinfo` attributes, the comparands are first adjusted by
  subtracting their UTC offsets (obtained from ``self.utcoffset()``). In order
  to stop mixed-type comparisons from falling back to the default comparison by
  object address, when a :class:`.time` object is compared to an object of a
  different type, :exc:`TypeError` is raised unless the comparison is ``==`` or
  ``!=``.  The latter cases return :const:`False` or :const:`True`, respectively.

* hash, use as dict key

* efficient pickling

* in Boolean contexts, a :class:`.time` object is considered to be true if and
  only if, after converting it to minutes and subtracting :meth:`utcoffset` (or
  ``0`` if that's ``None``), the result is non-zero.


Instance methods:

.. method:: time.replace([hour[, minute[, second[, microsecond[, tzinfo]]]]])

   Return a :class:`.time` with the same value, except for those attributes given
   new values by whichever keyword arguments are specified.  Note that
   ``tzinfo=None`` can be specified to create a naive :class:`.time` from an
   aware :class:`.time`, without conversion of the time data.


.. method:: time.isoformat()

   Return a string representing the time in ISO 8601 format, HH:MM:SS.mmmmmm or, if
   self.microsecond is 0, HH:MM:SS If :meth:`utcoffset` does not return ``None``, a
   6-character string is appended, giving the UTC offset in (signed) hours and
   minutes: HH:MM:SS.mmmmmm+HH:MM or, if self.microsecond is 0, HH:MM:SS+HH:MM


.. method:: time.__str__()

   For a time *t*, ``str(t)`` is equivalent to ``t.isoformat()``.


.. method:: time.strftime(format)

   Return a string representing the time, controlled by an explicit format string.
   See section :ref:`strftime-strptime-behavior`.


.. method:: time.utcoffset()

   If :attr:`tzinfo` is ``None``, returns ``None``, else returns
   ``self.tzinfo.utcoffset(None)``, and raises an exception if the latter doesn't
   return ``None`` or a :class:`timedelta` object representing a whole number of
   minutes with magnitude less than one day.


.. method:: time.dst()

   If :attr:`tzinfo` is ``None``, returns ``None``, else returns
   ``self.tzinfo.dst(None)``, and raises an exception if the latter doesn't return
   ``None``, or a :class:`timedelta` object representing a whole number of minutes
   with magnitude less than one day.


.. method:: time.tzname()

   If :attr:`tzinfo` is ``None``, returns ``None``, else returns
   ``self.tzinfo.tzname(None)``, or raises an exception if the latter doesn't
   return ``None`` or a string object.


Example:

    >>> from datetime import time, tzinfo
    >>> class GMT1(tzinfo):
    ...     def utcoffset(self, dt):
    ...         return timedelta(hours=1)
    ...     def dst(self, dt):
    ...         return timedelta(0)
    ...     def tzname(self,dt):
    ...         return "Europe/Prague"
    ...
    >>> t = time(12, 10, 30, tzinfo=GMT1())
    >>> t                               # doctest: +ELLIPSIS
    datetime.time(12, 10, 30, tzinfo=<GMT1 object at 0x...>)
    >>> gmt = GMT1()
    >>> t.isoformat()
    '12:10:30+01:00'
    >>> t.dst()
    datetime.timedelta(0)
    >>> t.tzname()
    'Europe/Prague'
    >>> t.strftime("%H:%M:%S %Z")
    '12:10:30 Europe/Prague'


.. _datetime-tzinfo:

:class:`tzinfo` Objects
-----------------------

:class:`tzinfo` is an abstract base class, meaning that this class should not be
instantiated directly.  You need to derive a concrete subclass, and (at least)
supply implementations of the standard :class:`tzinfo` methods needed by the
:class:`.datetime` methods you use.  The :mod:`datetime` module supplies
a simple concrete subclass of :class:`tzinfo` :class:`timezone` which can reprsent
timezones with fixed offset from UTC such as UTC itself or North American EST and
EDT.

An instance of (a concrete subclass of) :class:`tzinfo` can be passed to the
constructors for :class:`.datetime` and :class:`.time` objects. The latter objects
view their attributes as being in local time, and the :class:`tzinfo` object
supports methods revealing offset of local time from UTC, the name of the time
zone, and DST offset, all relative to a date or time object passed to them.

Special requirement for pickling:  A :class:`tzinfo` subclass must have an
:meth:`__init__` method that can be called with no arguments, else it can be
pickled but possibly not unpickled again.  This is a technical requirement that
may be relaxed in the future.

A concrete subclass of :class:`tzinfo` may need to implement the following
methods.  Exactly which methods are needed depends on the uses made of aware
:mod:`datetime` objects.  If in doubt, simply implement all of them.


.. method:: tzinfo.utcoffset(dt)

   Return offset of local time from UTC, in minutes east of UTC.  If local time is
   west of UTC, this should be negative.  Note that this is intended to be the
   total offset from UTC; for example, if a :class:`tzinfo` object represents both
   time zone and DST adjustments, :meth:`utcoffset` should return their sum.  If
   the UTC offset isn't known, return ``None``.  Else the value returned must be a
   :class:`timedelta` object specifying a whole number of minutes in the range
   -1439 to 1439 inclusive (1440 = 24\*60; the magnitude of the offset must be less
   than one day).  Most implementations of :meth:`utcoffset` will probably look
   like one of these two::

      return CONSTANT                 # fixed-offset class
      return CONSTANT + self.dst(dt)  # daylight-aware class

   If :meth:`utcoffset` does not return ``None``, :meth:`dst` should not return
   ``None`` either.

   The default implementation of :meth:`utcoffset` raises
   :exc:`NotImplementedError`.


.. method:: tzinfo.dst(dt)

   Return the daylight saving time (DST) adjustment, in minutes east of UTC, or
   ``None`` if DST information isn't known.  Return ``timedelta(0)`` if DST is not
   in effect. If DST is in effect, return the offset as a :class:`timedelta` object
   (see :meth:`utcoffset` for details). Note that DST offset, if applicable, has
   already been added to the UTC offset returned by :meth:`utcoffset`, so there's
   no need to consult :meth:`dst` unless you're interested in obtaining DST info
   separately.  For example, :meth:`datetime.timetuple` calls its :attr:`tzinfo`
   attribute's :meth:`dst` method to determine how the :attr:`tm_isdst` flag
   should be set, and :meth:`tzinfo.fromutc` calls :meth:`dst` to account for
   DST changes when crossing time zones.

   An instance *tz* of a :class:`tzinfo` subclass that models both standard and
   daylight times must be consistent in this sense:

   ``tz.utcoffset(dt) - tz.dst(dt)``

   must return the same result for every :class:`.datetime` *dt* with ``dt.tzinfo ==
   tz``  For sane :class:`tzinfo` subclasses, this expression yields the time
   zone's "standard offset", which should not depend on the date or the time, but
   only on geographic location.  The implementation of :meth:`datetime.astimezone`
   relies on this, but cannot detect violations; it's the programmer's
   responsibility to ensure it.  If a :class:`tzinfo` subclass cannot guarantee
   this, it may be able to override the default implementation of
   :meth:`tzinfo.fromutc` to work correctly with :meth:`astimezone` regardless.

   Most implementations of :meth:`dst` will probably look like one of these two::

      def dst(self, dt):
          # a fixed-offset class:  doesn't account for DST
          return timedelta(0)

   or ::

      def dst(self, dt):
          # Code to set dston and dstoff to the time zone's DST
          # transition times based on the input dt.year, and expressed
          # in standard local time.  Then

          if dston <= dt.replace(tzinfo=None) < dstoff:
              return timedelta(hours=1)
          else:
              return timedelta(0)

   The default implementation of :meth:`dst` raises :exc:`NotImplementedError`.


.. method:: tzinfo.tzname(dt)

   Return the time zone name corresponding to the :class:`.datetime` object *dt*, as
   a string. Nothing about string names is defined by the :mod:`datetime` module,
   and there's no requirement that it mean anything in particular.  For example,
   "GMT", "UTC", "-500", "-5:00", "EDT", "US/Eastern", "America/New York" are all
   valid replies.  Return ``None`` if a string name isn't known.  Note that this is
   a method rather than a fixed string primarily because some :class:`tzinfo`
   subclasses will wish to return different names depending on the specific value
   of *dt* passed, especially if the :class:`tzinfo` class is accounting for
   daylight time.

   The default implementation of :meth:`tzname` raises :exc:`NotImplementedError`.


These methods are called by a :class:`.datetime` or :class:`.time` object, in
response to their methods of the same names.  A :class:`.datetime` object passes
itself as the argument, and a :class:`.time` object passes ``None`` as the
argument.  A :class:`tzinfo` subclass's methods should therefore be prepared to
accept a *dt* argument of ``None``, or of class :class:`.datetime`.

When ``None`` is passed, it's up to the class designer to decide the best
response.  For example, returning ``None`` is appropriate if the class wishes to
say that time objects don't participate in the :class:`tzinfo` protocols.  It
may be more useful for ``utcoffset(None)`` to return the standard UTC offset, as
there is no other convention for discovering the standard offset.

When a :class:`.datetime` object is passed in response to a :class:`.datetime`
method, ``dt.tzinfo`` is the same object as *self*.  :class:`tzinfo` methods can
rely on this, unless user code calls :class:`tzinfo` methods directly.  The
intent is that the :class:`tzinfo` methods interpret *dt* as being in local
time, and not need worry about objects in other timezones.

There is one more :class:`tzinfo` method that a subclass may wish to override:


.. method:: tzinfo.fromutc(dt)

   This is called from the default :class:`datetime.astimezone()`
   implementation.  When called from that, ``dt.tzinfo`` is *self*, and *dt*'s
   date and time data are to be viewed as expressing a UTC time.  The purpose
   of :meth:`fromutc` is to adjust the date and time data, returning an
   equivalent datetime in *self*'s local time.

   Most :class:`tzinfo` subclasses should be able to inherit the default
   :meth:`fromutc` implementation without problems.  It's strong enough to handle
   fixed-offset time zones, and time zones accounting for both standard and
   daylight time, and the latter even if the DST transition times differ in
   different years.  An example of a time zone the default :meth:`fromutc`
   implementation may not handle correctly in all cases is one where the standard
   offset (from UTC) depends on the specific date and time passed, which can happen
   for political reasons. The default implementations of :meth:`astimezone` and
   :meth:`fromutc` may not produce the result you want if the result is one of the
   hours straddling the moment the standard offset changes.

   Skipping code for error cases, the default :meth:`fromutc` implementation acts
   like::

      def fromutc(self, dt):
          # raise ValueError error if dt.tzinfo is not self
          dtoff = dt.utcoffset()
          dtdst = dt.dst()
          # raise ValueError if dtoff is None or dtdst is None
          delta = dtoff - dtdst  # this is self's standard offset
          if delta:
              dt += delta   # convert to standard local time
              dtdst = dt.dst()
              # raise ValueError if dtdst is None
          if dtdst:
              return dt + dtdst
          else:
              return dt

Example :class:`tzinfo` classes:

.. literalinclude:: ../includes/tzinfo-examples.py

Note that there are unavoidable subtleties twice per year in a :class:`tzinfo`
subclass accounting for both standard and daylight time, at the DST transition
points.  For concreteness, consider US Eastern (UTC -0500), where EDT begins the
minute after 1:59 (EST) on the second Sunday in March, and ends the minute after
1:59 (EDT) on the first Sunday in November::

     UTC   3:MM  4:MM  5:MM  6:MM  7:MM  8:MM
     EST  22:MM 23:MM  0:MM  1:MM  2:MM  3:MM
     EDT  23:MM  0:MM  1:MM  2:MM  3:MM  4:MM

   start  22:MM 23:MM  0:MM  1:MM  3:MM  4:MM

     end  23:MM  0:MM  1:MM  1:MM  2:MM  3:MM

When DST starts (the "start" line), the local wall clock leaps from 1:59 to
3:00.  A wall time of the form 2:MM doesn't really make sense on that day, so
``astimezone(Eastern)`` won't deliver a result with ``hour == 2`` on the day DST
begins.  In order for :meth:`astimezone` to make this guarantee, the
:meth:`tzinfo.dst` method must consider times in the "missing hour" (2:MM for
Eastern) to be in daylight time.

When DST ends (the "end" line), there's a potentially worse problem: there's an
hour that can't be spelled unambiguously in local wall time: the last hour of
daylight time.  In Eastern, that's times of the form 5:MM UTC on the day
daylight time ends.  The local wall clock leaps from 1:59 (daylight time) back
to 1:00 (standard time) again. Local times of the form 1:MM are ambiguous.
:meth:`astimezone` mimics the local clock's behavior by mapping two adjacent UTC
hours into the same local hour then.  In the Eastern example, UTC times of the
form 5:MM and 6:MM both map to 1:MM when converted to Eastern.  In order for
:meth:`astimezone` to make this guarantee, the :meth:`tzinfo.dst` method must
consider times in the "repeated hour" to be in standard time.  This is easily
arranged, as in the example, by expressing DST switch times in the time zone's
standard local time.

Applications that can't bear such ambiguities should avoid using hybrid
:class:`tzinfo` subclasses; there are no ambiguities when using :class:`timezone`,
or any other fixed-offset :class:`tzinfo` subclass (such as a class representing
only EST (fixed offset -5 hours), or only EDT (fixed offset -4 hours)).

.. seealso::

   `pytz <http://pypi.python.org/pypi/pytz/>`_
      The standard library has no :class:`tzinfo` instances except for UTC, but
      there exists a third-party library which brings the *IANA timezone
      database* (also known as the Olson database) to Python: *pytz*.

      *pytz* contains up-to-date information and its usage is recommended.

   `IANA timezone database <http://www.iana.org/time-zones>`_
      The Time Zone Database (often called tz or zoneinfo) contains code and
      data that represent the history of local time for many representative
      locations around the globe. It is updated periodically to reflect changes
      made by political bodies to time zone boundaries, UTC offsets, and
      daylight-saving rules.


.. _datetime-timezone:

:class:`timezone` Objects
--------------------------

A :class:`timezone` object represents a timezone that is defined by a
fixed offset from UTC.  Note that objects of this class cannot be used
to represent timezone information in the locations where different
offsets are used in different days of the year or where historical
changes have been made to civil time.


.. class:: timezone(offset[, name])

  The *offset* argument must be specified as a :class:`timedelta`
  object representing the difference between the local time and UTC.  It must
  be strictly between ``-timedelta(hours=24)`` and
  ``timedelta(hours=24)`` and represent a whole number of minutes,
  otherwise :exc:`ValueError` is raised.

  The *name* argument is optional.  If specified it must be a string that
  is used as the value returned by the ``tzname(dt)`` method.  Otherwise,
  ``tzname(dt)`` returns a string 'UTCsHH:MM', where s is the sign of
  *offset*, HH and MM are two digits of ``offset.hours`` and
  ``offset.minutes`` respectively.

.. method:: timezone.utcoffset(dt)

  Return the fixed value specified when the :class:`timezone` instance is
  constructed.  The *dt* argument is ignored.  The return value is a
  :class:`timedelta` instance equal to the difference between the
  local time and UTC.

.. method:: timezone.tzname(dt)

  Return the fixed value specified when the :class:`timezone` instance is
  constructed or a string 'UTCsHH:MM', where s is the sign of
  *offset*, HH and MM are two digits of ``offset.hours`` and
  ``offset.minutes`` respectively.

.. method:: timezone.dst(dt)

  Always returns ``None``.

.. method:: timezone.fromutc(dt)

  Return ``dt + offset``.  The *dt* argument must be an aware
  :class:`.datetime` instance, with ``tzinfo`` set to ``self``.

Class attributes:

.. attribute:: timezone.utc

   The UTC timezone, ``timezone(timedelta(0))``.


.. _strftime-strptime-behavior:

:meth:`strftime` and :meth:`strptime` Behavior
----------------------------------------------

:class:`date`, :class:`.datetime`, and :class:`.time` objects all support a
``strftime(format)`` method, to create a string representing the time under the
control of an explicit format string.  Broadly speaking, ``d.strftime(fmt)``
acts like the :mod:`time` module's ``time.strftime(fmt, d.timetuple())``
although not all objects support a :meth:`timetuple` method.

Conversely, the :meth:`datetime.strptime` class method creates a
:class:`.datetime` object from a string representing a date and time and a
corresponding format string. ``datetime.strptime(date_string, format)`` is
equivalent to ``datetime(*(time.strptime(date_string, format)[0:6]))``.

For :class:`.time` objects, the format codes for year, month, and day should not
be used, as time objects have no such values.  If they're used anyway, ``1900``
is substituted for the year, and ``1`` for the month and day.

For :class:`date` objects, the format codes for hours, minutes, seconds, and
microseconds should not be used, as :class:`date` objects have no such
values.  If they're used anyway, ``0`` is substituted for them.

For a naive object, the ``%z`` and ``%Z`` format codes are replaced by empty
strings.

For an aware object:

``%z``
   :meth:`utcoffset` is transformed into a 5-character string of the form +HHMM or
   -HHMM, where HH is a 2-digit string giving the number of UTC offset hours, and
   MM is a 2-digit string giving the number of UTC offset minutes.  For example, if
   :meth:`utcoffset` returns ``timedelta(hours=-3, minutes=-30)``, ``%z`` is
   replaced with the string ``'-0330'``.

``%Z``
   If :meth:`tzname` returns ``None``, ``%Z`` is replaced by an empty string.
   Otherwise ``%Z`` is replaced by the returned value, which must be a string.

The full set of format codes supported varies across platforms, because Python
calls the platform C library's :func:`strftime` function, and platform
variations are common.

The following is a list of all the format codes that the C standard (1989
version) requires, and these work on all platforms with a standard C
implementation.  Note that the 1999 version of the C standard added additional
format codes.

+-----------+--------------------------------+-------+
| Directive | Meaning                        | Notes |
+===========+================================+=======+
| ``%a``    | Locale's abbreviated weekday   |       |
|           | name.                          |       |
+-----------+--------------------------------+-------+
| ``%A``    | Locale's full weekday name.    |       |
+-----------+--------------------------------+-------+
| ``%b``    | Locale's abbreviated month     |       |
|           | name.                          |       |
+-----------+--------------------------------+-------+
| ``%B``    | Locale's full month name.      |       |
+-----------+--------------------------------+-------+
| ``%c``    | Locale's appropriate date and  |       |
|           | time representation.           |       |
+-----------+--------------------------------+-------+
| ``%d``    | Day of the month as a decimal  |       |
|           | number [01,31].                |       |
+-----------+--------------------------------+-------+
| ``%f``    | Microsecond as a decimal       | \(1)  |
|           | number [0,999999], zero-padded |       |
|           | on the left                    |       |
+-----------+--------------------------------+-------+
| ``%H``    | Hour (24-hour clock) as a      |       |
|           | decimal number [00,23].        |       |
+-----------+--------------------------------+-------+
| ``%I``    | Hour (12-hour clock) as a      |       |
|           | decimal number [01,12].        |       |
+-----------+--------------------------------+-------+
| ``%j``    | Day of the year as a decimal   |       |
|           | number [001,366].              |       |
+-----------+--------------------------------+-------+
| ``%m``    | Month as a decimal number      |       |
|           | [01,12].                       |       |
+-----------+--------------------------------+-------+
| ``%M``    | Minute as a decimal number     |       |
|           | [00,59].                       |       |
+-----------+--------------------------------+-------+
| ``%p``    | Locale's equivalent of either  | \(2)  |
|           | AM or PM.                      |       |
+-----------+--------------------------------+-------+
| ``%S``    | Second as a decimal number     | \(3)  |
|           | [00,59].                       |       |
+-----------+--------------------------------+-------+
| ``%U``    | Week number of the year        | \(4)  |
|           | (Sunday as the first day of    |       |
|           | the week) as a decimal number  |       |
|           | [00,53].  All days in a new    |       |
|           | year preceding the first       |       |
|           | Sunday are considered to be in |       |
|           | week 0.                        |       |
+-----------+--------------------------------+-------+
| ``%w``    | Weekday as a decimal number    |       |
|           | [0(Sunday),6].                 |       |
+-----------+--------------------------------+-------+
| ``%W``    | Week number of the year        | \(4)  |
|           | (Monday as the first day of    |       |
|           | the week) as a decimal number  |       |
|           | [00,53].  All days in a new    |       |
|           | year preceding the first       |       |
|           | Monday are considered to be in |       |
|           | week 0.                        |       |
+-----------+--------------------------------+-------+
| ``%x``    | Locale's appropriate date      |       |
|           | representation.                |       |
+-----------+--------------------------------+-------+
| ``%X``    | Locale's appropriate time      |       |
|           | representation.                |       |
+-----------+--------------------------------+-------+
| ``%y``    | Year without century as a      |       |
|           | decimal number [00,99].        |       |
+-----------+--------------------------------+-------+
| ``%Y``    | Year with century as a decimal | \(5)  |
|           | number [0001,9999] (strptime), |       |
|           | [1000,9999] (strftime).        |       |
+-----------+--------------------------------+-------+
| ``%z``    | UTC offset in the form +HHMM   | \(6)  |
|           | or -HHMM (empty string if the  |       |
|           | the object is naive).          |       |
+-----------+--------------------------------+-------+
| ``%Z``    | Time zone name (empty string   |       |
|           | if the object is naive).       |       |
+-----------+--------------------------------+-------+
| ``%%``    | A literal ``'%'`` character.   |       |
+-----------+--------------------------------+-------+

Notes:

(1)
   When used with the :meth:`strptime` method, the ``%f`` directive
   accepts from one to six digits and zero pads on the right.  ``%f`` is
   an extension to the set of format characters in the C standard (but
   implemented separately in datetime objects, and therefore always
   available).

(2)
   When used with the :meth:`strptime` method, the ``%p`` directive only affects
   the output hour field if the ``%I`` directive is used to parse the hour.

(3)
   Unlike :mod:`time` module, :mod:`datetime` module does not support
   leap seconds.

(4)
   When used with the :meth:`strptime` method, ``%U`` and ``%W`` are only used in
   calculations when the day of the week and the year are specified.

(5)
   For technical reasons, :meth:`strftime` method does not support
   dates before year 1000: ``t.strftime(format)`` will raise a
   :exc:`ValueError` when ``t.year < 1000`` even if ``format`` does
   not contain ``%Y`` directive.  The :meth:`strptime` method can
   parse years in the full [1, 9999] range, but years < 1000 must be
   zero-filled to 4-digit width.

   .. versionchanged:: 3.2
      In previous versions, :meth:`strftime` method was restricted to
      years >= 1900.

(6)
   For example, if :meth:`utcoffset` returns ``timedelta(hours=-3, minutes=-30)``,
   ``%z`` is replaced with the string ``'-0330'``.

.. versionchanged:: 3.2
   When the ``%z`` directive is provided to the :meth:`strptime` method, an
   aware :class:`.datetime` object will be produced.  The ``tzinfo`` of the
   result will be set to a :class:`timezone` instance.

.. rubric:: Footnotes

.. [#] If, that is, we ignore the effects of Relativity
